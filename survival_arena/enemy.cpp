#include "enemy.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "collision.h"
#include "game_settings.h"

namespace
{
// Stores an enemy selected by the broad-phase ray corridor test.
struct EnemyRayCandidate
{
    EnemyVectorType enemyType = EnemyVectorType::Normal;
    std::size_t enemyIndex = 0;
    float forwardDistance = 0.0f;
};

// Stores an exact enemy mesh or shield impact before global distance sorting.
struct EnemyRayHit
{
    EnemyVectorType enemyType = EnemyVectorType::Normal;
    std::size_t enemyIndex = 0;
    float distance = 0.0f;
    bool shieldHit = false;
    glm::vec3 impactPosition = glm::vec3(0.0f);
    glm::vec3 enemyCenter = glm::vec3(0.0f);
};

// Groups the enemy and collider selected from one candidate without taking ownership.
struct EnemyRayTarget
{
    const Enemy* enemy = nullptr;
    const TriangleMeshCollider* collider = nullptr;
};

// Computes the animated enemy pivot used by the broad-phase corridor test.
glm::vec3 buildEnemyBroadPhaseCenter(
    const Enemy& enemy, // Enemy whose animated pivot is required.
    float time // Current animation time in seconds.
)
{
    glm::vec3 center = enemy.position;
    const float hoverOffset = std::sin(time * enemy.hoverSpeed + enemy.hoverPhase) * enemy.hoverAmplitude;
    center.y += hoverOffset;
    return center;
}

// Computes a conservative local broad-phase radius around the enemy pivot.
float buildEnemyLocalBroadPhaseRadius(
    const TriangleMeshCollider& collider, // Mesh collider providing the local bounding sphere.
    EnemyVectorType enemyType // Enemy variant used to include the resistant shield size.
)
{
    float localSphereRadius = collider.localSphere.radius;

    if (enemyType == EnemyVectorType::Resistant)
        localSphereRadius *= GameSettings::Enemy::resistantShieldRadiusMultiplier;

    // The center offset keeps the pivot-centered radius conservative during rotation and sway.
    return glm::length(collider.localSphere.center) + localSphereRadius;
}

// Adds enemies intersecting the ray corridor to the shared candidate list.
void collectEnemyRayCandidates(
    const Ray& ray, // Normalized gameplay ray in world space.
    const std::vector<Enemy>& enemies, // Enemy vector tested by the broad phase.
    const TriangleMeshCollider& collider, // Collider used to derive a conservative radius.
    EnemyVectorType enemyType, // Type associated with the tested enemy vector.
    float time, // Current animation time in seconds.
    std::vector<EnemyRayCandidate>& candidates // Shared output vector receiving accepted candidates.
)
{
    const float localBroadPhaseRadius = buildEnemyLocalBroadPhaseRadius(collider, enemyType);

    for (std::size_t enemyIndex = 0; enemyIndex < enemies.size(); ++enemyIndex)
    {
        const Enemy& enemy = enemies[enemyIndex];
        const glm::vec3 center = buildEnemyBroadPhaseCenter(enemy, time);
        const float worldRadius = localBroadPhaseRadius * enemy.scale;

        float forwardDistance = 0.0f;

        if (!isPointWithinRayCorridor(ray, center, worldRadius, forwardDistance))
            continue;

        EnemyRayCandidate candidate;
        candidate.enemyType = enemyType;
        candidate.enemyIndex = enemyIndex;
        candidate.forwardDistance = forwardDistance;
        candidates.push_back(candidate);
    }
}

// Resolves the enemy vector and collider associated with one broad-phase candidate.
EnemyRayTarget resolveEnemyRayTarget(
    const EnemyRayCandidate& candidate, // Candidate whose source enemy must be resolved.
    const std::vector<Enemy>& normalEnemies, // Normal enemy vector.
    const std::vector<Enemy>& translucentEnemies, // Translucent enemy vector.
    const std::vector<Enemy>& resistantEnemies, // Resistant enemy vector.
    const TriangleMeshCollider& normalCollider, // Collider shared by normal and translucent enemies.
    const TriangleMeshCollider& resistantCollider // Collider used by resistant enemies.
)
{
    EnemyRayTarget target;

    switch (candidate.enemyType)
    {
        case EnemyVectorType::Normal:
            if (candidate.enemyIndex < normalEnemies.size())
            {
                target.enemy = &normalEnemies[candidate.enemyIndex];
                target.collider = &normalCollider;
            }
            break;

        case EnemyVectorType::Translucent:
            if (candidate.enemyIndex < translucentEnemies.size())
            {
                target.enemy = &translucentEnemies[candidate.enemyIndex];
                target.collider = &normalCollider;
            }
            break;

        case EnemyVectorType::Resistant:
            if (candidate.enemyIndex < resistantEnemies.size())
            {
                target.enemy = &resistantEnemies[candidate.enemyIndex];
                target.collider = &resistantCollider;
            }
            break;
    }

    return target;
}

// Reconstructs one exact shield or mesh impact from a broad-phase candidate.
bool buildEnemyRayHit(
    const Ray& ray, // Normalized gameplay ray in world space.
    const EnemyRayCandidate& candidate, // Candidate providing the enemy type and vector index.
    const EnemyRayTarget& target, // Resolved enemy and collider references.
    float time, // Current animation time in seconds.
    EnemyRayHit& hit // Output impact written when an exact intersection is found.
)
{
    if (target.enemy == nullptr || target.collider == nullptr)
        return false;

    const float infiniteDistance = std::numeric_limits<float>::max();
    const glm::mat4 model = buildEnemyModelMatrix(*target.enemy, time);
    const glm::vec3 worldEnemyCenter = glm::vec3(model * glm::vec4(target.collider->localSphere.center, 1.0f));

    // Resistant enemies keep the shield sphere as an additional gate even after shield destruction.
    if (candidate.enemyType == EnemyVectorType::Resistant)
    {
        BoundingSphere shieldSphere = transformBoundingSphere(target.collider->localSphere, model);
        shieldSphere.radius *= GameSettings::Enemy::resistantShieldRadiusMultiplier;

        RaySphereHit shieldSphereHit;

        if (!intersectRaySphere(ray, shieldSphere, infiniteDistance, shieldSphereHit))
            return false;

        if (target.enemy->shieldActive)
        {
            hit.enemyType = candidate.enemyType;
            hit.enemyIndex = candidate.enemyIndex;
            hit.distance = shieldSphereHit.distance;
            hit.shieldHit = true;
            hit.impactPosition = shieldSphereHit.position;
            hit.enemyCenter = worldEnemyCenter;
            return true;
        }
    }

    const Ray localRay = transformRay(ray, glm::inverse(model));
    RayTriangleHit triangleHit;

    if (!intersectRayMesh(localRay, *target.collider, infiniteDistance, triangleHit))
        return false;

    // The barycentric coordinates preserve the exact point hit on the rendered triangle.
    const std::size_t firstVertex = triangleHit.triangleIndex * 3;
    const glm::vec3& p0 = target.collider->triangleVertices[firstVertex];
    const glm::vec3& p1 = target.collider->triangleVertices[firstVertex + 1];
    const glm::vec3& p2 = target.collider->triangleVertices[firstVertex + 2];
    const float barycentricW = 1.0f - triangleHit.u - triangleHit.v;
    const glm::vec3 localImpactPosition = p0 * barycentricW + p1 * triangleHit.u + p2 * triangleHit.v;
    const glm::vec3 worldImpactPosition = glm::vec3(model * glm::vec4(localImpactPosition, 1.0f));

    hit.enemyType = candidate.enemyType;
    hit.enemyIndex = candidate.enemyIndex;
    hit.distance = glm::dot(worldImpactPosition - ray.origin, ray.direction);
    hit.shieldHit = false;
    hit.impactPosition = worldImpactPosition;
    hit.enemyCenter = worldEnemyCenter;
    return true;
}

// Removes the requested enemies with descending indices and swap plus pop_back.
void removeEnemiesAtIndices(
    std::vector<Enemy>& enemies, // Enemy vector modified by the removals.
    std::vector<std::size_t>& indices // Indices sorted in descending order before removal.
)
{
    std::sort(indices.begin(), indices.end(), [](std::size_t firstIndex, std::size_t secondIndex) { return firstIndex > secondIndex; });

    for (std::size_t index : indices)
    {
        if (index >= enemies.size())
            continue;

        if (index != enemies.size() - 1)
            std::swap(enemies[index], enemies.back());

        enemies.pop_back();
    }
}
}

// Builds the exact model matrix shared by rendering and collision calculations.
glm::mat4 buildEnemyModelMatrix(
    const Enemy& enemy, // Enemy state used to reproduce its current visual transform.
    float time // Current animation time in seconds.
)
{
    glm::vec3 visualPosition = enemy.position;
    const float hoverOffset = std::sin(time * enemy.hoverSpeed + enemy.hoverPhase) * enemy.hoverAmplitude;
    visualPosition.y += hoverOffset;

    float swayAngle = 0.0f;

    if (enemy.isMoving)
        swayAngle = std::sin(time * enemy.swaySpeed + enemy.swayPhase) * enemy.swayAmplitude;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, visualPosition);
    model = glm::rotate(model, enemy.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, swayAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(enemy.scale));
    return model;
}

// Updates one enemy's target selection, orientation and horizontal movement.
void updateEnemyMovement(
    Enemy& enemy, // Enemy state modified by the update.
    const glm::vec3& playerPosition, // Current player position in world space.
    const glm::vec3& cameraForwardXZ, // Normalized horizontal camera direction.
    float cameraHalfViewCosineSquared, // Squared cosine used by the horizontal view-angle test.
    float directChaseRadius, // Distance below which the enemy directly targets the player.
    float deltaTime // Duration of the current frame in seconds.
)
{
    constexpr float minimumLengthSquared = 0.000001f;

    glm::vec3 playerToEnemy = enemy.position - playerPosition;
    playerToEnemy.y = 0.0f;

    const float playerDistanceSquared = glm::dot(playerToEnemy, playerToEnemy);
    const float directChaseRadiusSquared = directChaseRadius * directChaseRadius;
    const bool closeToPlayer = playerDistanceSquared <= directChaseRadiusSquared;

    bool insideCameraAngle = false;

    if (playerDistanceSquared <= minimumLengthSquared)
    {
        insideCameraAngle = true;
    }
    else
    {
        const float forwardDistance = glm::dot(playerToEnemy, cameraForwardXZ);
        insideCameraAngle = forwardDistance > 0.0f && forwardDistance * forwardDistance >= playerDistanceSquared * cameraHalfViewCosineSquared;
    }

    // A distant enemy outside the camera angle uses its personal approach point to reduce stacking.
    glm::vec3 targetPosition = playerPosition;

    if (!insideCameraAngle && !closeToPlayer)
    {
        targetPosition.x += enemy.approachOffset.x;
        targetPosition.z += enemy.approachOffset.y;
    }

    glm::vec3 lookDirection = playerPosition - enemy.position;
    lookDirection.y = 0.0f;

    const float lookLengthSquared = glm::dot(lookDirection, lookDirection);

    if (lookLengthSquared > minimumLengthSquared)
    {
        const float targetRotation = std::atan2(lookDirection.x, lookDirection.z);
        float angleDifference = targetRotation - enemy.rotationY;
        angleDifference = std::atan2(std::sin(angleDifference), std::cos(angleDifference));

        const float maximumRotationStep = enemy.rotationSpeed * deltaTime;
        angleDifference = glm::clamp(angleDifference, -maximumRotationStep, maximumRotationStep);
        enemy.rotationY += angleDifference;
    }

    glm::vec3 movementDirection = targetPosition - enemy.position;
    movementDirection.y = 0.0f;

    const float movementLengthSquared = glm::dot(movementDirection, movementDirection);
    enemy.isMoving = false;

    if (movementLengthSquared > minimumLengthSquared)
    {
        movementDirection /= std::sqrt(movementLengthSquared);
        enemy.position += movementDirection * enemy.speed * deltaTime;
        enemy.isMoving = true;
    }
}

// Selects an enemy type using the probability thresholds defined in the settings.
EnemyVectorType randomEnemyType(
    std::mt19937& rng // Deterministic random generator used by the spawning system.
)
{
    std::uniform_int_distribution<int> typeDistribution(0, 99);
    const int roll = typeDistribution(rng);

    if (roll < GameSettings::Enemy::normalTypeThreshold)
        return EnemyVectorType::Normal;

    if (roll < GameSettings::Enemy::translucentTypeThreshold)
        return EnemyVectorType::Translucent;

    return EnemyVectorType::Resistant;
}

// Generates a random spawn position along one of the four arena sides.
glm::vec3 randomSpawnPositionAroundArena(
    std::mt19937& rng, // Deterministic random generator used by the spawning system.
    float arenaHalfSize, // Half-size of the square arena.
    float y // Vertical coordinate assigned to the spawn position.
)
{
    const float spawnDistance = arenaHalfSize - GameSettings::Enemy::spawnEdgeOffset;

    std::uniform_int_distribution<int> sideDistribution(0, 3);
    std::uniform_real_distribution<float> positionDistribution(-spawnDistance, spawnDistance);

    const int side = sideDistribution(rng);
    const float randomPosition = positionDistribution(rng);

    switch (side)
    {
        case 0:
            return glm::vec3(randomPosition, y, -spawnDistance);

        case 1:
            return glm::vec3(randomPosition, y, spawnDistance);

        case 2:
            return glm::vec3(-spawnDistance, y, randomPosition);

        default:
            return glm::vec3(spawnDistance, y, randomPosition);
    }
}

// Creates and configures one enemy using the centralized settings.
Enemy createEnemy(
    EnemyVectorType type, // Gameplay and visual variant to configure.
    const glm::vec3& position, // Initial enemy position in world space.
    std::mt19937& rng // Deterministic random generator used for animation phases and approach offset.
)
{
    constexpr float twoPi = 2.0f * 3.14159265f;

    std::uniform_real_distribution<float> phaseDistribution(0.0f, twoPi);
    std::uniform_real_distribution<float> approachAngleDistribution(0.0f, twoPi);
    std::uniform_real_distribution<float> approachRadiusDistribution(GameSettings::Enemy::approachMinimumRadius, GameSettings::Enemy::approachMaximumRadius);

    Enemy enemy;
    enemy.position = position;
    enemy.hoverPhase = phaseDistribution(rng);
    enemy.swayPhase = phaseDistribution(rng);

    const float approachAngle = approachAngleDistribution(rng);
    const float approachRadius = approachRadiusDistribution(rng);
    enemy.approachOffset = glm::vec2(std::cos(approachAngle) * approachRadius, std::sin(approachAngle) * approachRadius);

    switch (type)
    {
        case EnemyVectorType::Normal:
            enemy.speed = GameSettings::Enemy::normalSpeed;
            enemy.scale = GameSettings::Enemy::normalScale;
            enemy.rotationSpeed = GameSettings::Enemy::normalRotationSpeed;
            enemy.hoverAmplitude = GameSettings::Enemy::normalHoverAmplitude;
            enemy.hoverSpeed = GameSettings::Enemy::normalHoverSpeed;
            enemy.swayAmplitude = glm::radians(GameSettings::Enemy::normalSwayAmplitudeDegrees);
            enemy.swaySpeed = GameSettings::Enemy::normalSwaySpeed;
            break;

        case EnemyVectorType::Translucent:
            enemy.speed = GameSettings::Enemy::translucentSpeed;
            enemy.scale = GameSettings::Enemy::translucentScale;
            enemy.rotationSpeed = GameSettings::Enemy::translucentRotationSpeed;
            enemy.hoverAmplitude = GameSettings::Enemy::translucentHoverAmplitude;
            enemy.hoverSpeed = GameSettings::Enemy::translucentHoverSpeed;
            enemy.swayAmplitude = glm::radians(GameSettings::Enemy::translucentSwayAmplitudeDegrees);
            enemy.swaySpeed = GameSettings::Enemy::translucentSwaySpeed;
            break;

        case EnemyVectorType::Resistant:
            enemy.speed = GameSettings::Enemy::resistantSpeed;
            enemy.scale = GameSettings::Enemy::resistantScale;
            enemy.rotationSpeed = GameSettings::Enemy::resistantRotationSpeed;
            enemy.hoverAmplitude = GameSettings::Enemy::resistantHoverAmplitude;
            enemy.hoverSpeed = GameSettings::Enemy::resistantHoverSpeed;
            enemy.swayAmplitude = glm::radians(GameSettings::Enemy::resistantSwayAmplitudeDegrees);
            enemy.swaySpeed = GameSettings::Enemy::resistantSwaySpeed;
            enemy.shieldActive = true;
            break;
    }

    return enemy;
}

// Builds the world-space shield sphere from the resistant enemy model matrix.
BoundingSphere buildEnemyShieldSphere(
    const Enemy& enemy, // Resistant enemy owning the shield.
    const TriangleMeshCollider& collider, // Mesh collider providing the local bounding sphere.
    float time // Current animation time in seconds.
)
{
    const glm::mat4 model = buildEnemyModelMatrix(enemy, time);
    BoundingSphere shieldSphere = transformBoundingSphere(collider.localSphere, model);
    shieldSphere.radius *= GameSettings::Enemy::resistantShieldRadiusMultiplier;
    return shieldSphere;
}

// Applies one shot to all enemy vectors in exact global impact order.
ShotResult applyShotToEnemies(
    const Ray& ray, // Normalized gameplay ray in world space.
    std::vector<Enemy>& normalEnemies, // Normal enemy vector modified by confirmed kills.
    std::vector<Enemy>& translucentEnemies, // Translucent enemy vector modified by confirmed kills.
    std::vector<Enemy>& resistantEnemies, // Resistant enemy vector modified by shield hits and confirmed kills.
    const TriangleMeshCollider& normalCollider, // Collider shared by normal and translucent enemies.
    const TriangleMeshCollider& resistantCollider, // Collider used by resistant enemies and their shields.
    float time, // Current animation time in seconds.
    float beamFallbackDistance, // Beam distance used when no impact stops the shot.
    std::size_t maximumPenetration // Maximum number of enemies destroyed by one shot.
)
{
    std::vector<EnemyRayCandidate> candidates;
    candidates.reserve(normalEnemies.size() + translucentEnemies.size() + resistantEnemies.size());
    collectEnemyRayCandidates(ray, normalEnemies, normalCollider, EnemyVectorType::Normal, time, candidates);
    collectEnemyRayCandidates(ray, translucentEnemies, normalCollider, EnemyVectorType::Translucent, time, candidates);
    collectEnemyRayCandidates(ray, resistantEnemies, resistantCollider, EnemyVectorType::Resistant, time, candidates);

    std::vector<EnemyRayHit> hits;
    hits.reserve(candidates.size());

    // Every exact impact is collected before sorting all three enemy types together.
    for (const EnemyRayCandidate& candidate : candidates)
    {
        const EnemyRayTarget target = resolveEnemyRayTarget(candidate, normalEnemies, translucentEnemies, resistantEnemies, normalCollider, resistantCollider);
        EnemyRayHit hit;

        if (buildEnemyRayHit(ray, candidate, target, time, hit))
            hits.push_back(hit);
    }

    std::sort(hits.begin(), hits.end(), [](const EnemyRayHit& firstHit, const EnemyRayHit& secondHit) { return firstHit.distance < secondHit.distance; });

    std::vector<std::size_t> normalEnemiesToRemove;
    std::vector<std::size_t> translucentEnemiesToRemove;
    std::vector<std::size_t> resistantEnemiesToRemove;

    ShotResult result;
    result.beamDistance = beamFallbackDistance;

    std::uint64_t baseShotScore = 0;

    // Nearest impacts are processed first so shields and penetration stop the beam physically.
    for (const EnemyRayHit& hit : hits)
    {
        if (hit.shieldHit)
        {
            if (hit.enemyIndex < resistantEnemies.size())
                resistantEnemies[hit.enemyIndex].shieldActive = false;

            result.shieldDestroyed = true;
            result.beamDistance = hit.distance;
            break;
        }

        const Enemy* destroyedEnemy = nullptr;

        switch (hit.enemyType)
        {
            case EnemyVectorType::Normal:
                if (hit.enemyIndex >= normalEnemies.size())
                    continue;

                destroyedEnemy = &normalEnemies[hit.enemyIndex];
                normalEnemiesToRemove.push_back(hit.enemyIndex);
                baseShotScore += GameSettings::Enemy::normalScore;
                break;

            case EnemyVectorType::Translucent:
                if (hit.enemyIndex >= translucentEnemies.size())
                    continue;

                destroyedEnemy = &translucentEnemies[hit.enemyIndex];
                translucentEnemiesToRemove.push_back(hit.enemyIndex);
                baseShotScore += GameSettings::Enemy::translucentScore;
                break;

            case EnemyVectorType::Resistant:
                if (hit.enemyIndex >= resistantEnemies.size())
                    continue;

                destroyedEnemy = &resistantEnemies[hit.enemyIndex];
                resistantEnemiesToRemove.push_back(hit.enemyIndex);
                baseShotScore += GameSettings::Enemy::resistantScore;
                break;
        }

        if (destroyedEnemy == nullptr)
            continue;

        // Death data is captured before swap plus pop_back can replace the destroyed element.
        EnemyDeathEvent deathEvent;
        deathEvent.impactPosition = hit.impactPosition;
        deathEvent.enemyCenter = hit.enemyCenter;
        deathEvent.enemyType = hit.enemyType;
        deathEvent.enemyScale = destroyedEnemy->scale;
        result.deathEvents.push_back(deathEvent);
        result.enemiesDestroyed++;

        if (result.enemiesDestroyed >= maximumPenetration)
        {
            result.beamDistance = hit.distance;
            break;
        }
    }

    result.scoreGained = baseShotScore * static_cast<std::uint64_t>(result.enemiesDestroyed);

    removeEnemiesAtIndices(normalEnemies, normalEnemiesToRemove);
    removeEnemiesAtIndices(translucentEnemies, translucentEnemiesToRemove);
    removeEnemiesAtIndices(resistantEnemies, resistantEnemiesToRemove);
    return result;
}

// Removes enemies touching the player and returns their accumulated contact damage.
int removeEnemiesTouchingPlayer(
    std::vector<Enemy>& enemies, // Enemy vector tested and modified by contact removals.
    const glm::vec3& playerPosition, // Current player position in world space.
    float playerRadius, // Horizontal collision radius of the player.
    float enemyBaseRadius, // Base enemy contact radius before applying enemy scale.
    int damagePerEnemy // Damage added for each enemy reaching the player.
)
{
    int totalDamage = 0;

    // Backward traversal allows immediate swap plus pop_back without invalidating untested elements.
    for (std::size_t remaining = enemies.size(); remaining > 0; --remaining)
    {
        const std::size_t enemyIndex = remaining - 1;
        const Enemy& enemy = enemies[enemyIndex];
        const float differenceX = enemy.position.x - playerPosition.x;
        const float differenceZ = enemy.position.z - playerPosition.z;
        const float distanceSquared = differenceX * differenceX + differenceZ * differenceZ;
        const float enemyRadius = enemyBaseRadius * enemy.scale;
        const float contactDistance = playerRadius + enemyRadius;

        if (distanceSquared > contactDistance * contactDistance)
            continue;

        totalDamage += damagePerEnemy;

        if (enemyIndex != enemies.size() - 1)
            std::swap(enemies[enemyIndex], enemies.back());

        enemies.pop_back();
    }

    return totalDamage;
}
