#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

struct Ray;
struct TriangleMeshCollider;
struct BoundingSphere;

// Stores the gameplay and animation state of one enemy.
struct Enemy
{
    glm::vec3 position = glm::vec3(0.0f);

    float speed = 0.0f;
    float scale = 1.0f;

    float rotationY = 0.0f;
    float rotationSpeed = 0.0f;

    float hoverAmplitude = 0.0f;
    float hoverSpeed = 0.0f;
    float hoverPhase = 0.0f;

    float swayAmplitude = 0.0f;
    float swaySpeed = 0.0f;
    float swayPhase = 0.0f;

    glm::vec2 approachOffset = glm::vec2(0.0f);

    bool isMoving = false;
    bool shieldActive = false;
};

// Identifies the enemy vector and the corresponding gameplay variant.
enum class EnemyVectorType
{
    Normal,
    Translucent,
    Resistant
};

// Stores the information required to create one enemy-death visual effect.
struct EnemyDeathEvent
{
    glm::vec3 impactPosition = glm::vec3(0.0f);
    glm::vec3 enemyCenter = glm::vec3(0.0f);
    EnemyVectorType enemyType = EnemyVectorType::Normal;
    float enemyScale = 1.0f;
};

// Stores the complete result of one shot after all impacts have been processed.
struct ShotResult
{
    std::size_t enemiesDestroyed = 0;
    std::uint64_t scoreGained = 0;
    bool shieldDestroyed = false;
    float beamDistance = 0.0f;
    std::vector<EnemyDeathEvent> deathEvents;
};

// Builds the exact model matrix shared by rendering and collision calculations.
glm::mat4 buildEnemyModelMatrix(
    const Enemy& enemy, // Enemy state used to reproduce its current visual transform.
    float time // Current animation time in seconds.
);

// Updates one enemy's target selection, orientation and horizontal movement.
void updateEnemyMovement(
    Enemy& enemy, // Enemy state modified by the update.
    const glm::vec3& playerPosition, // Current player position in world space.
    const glm::vec3& cameraForwardXZ, // Normalized horizontal camera direction.
    float cameraHalfViewCosineSquared, // Squared cosine used by the horizontal view-angle test.
    float directChaseRadius, // Distance below which the enemy directly targets the player.
    float deltaTime // Duration of the current frame in seconds.
);

// Selects an enemy type using the configured probability distribution.
EnemyVectorType randomEnemyType(
    std::mt19937& rng // Deterministic random generator used by the spawning system.
);

// Generates a random spawn position along one of the four arena sides.
glm::vec3 randomSpawnPositionAroundArena(
    std::mt19937& rng, // Deterministic random generator used by the spawning system.
    float arenaHalfSize, // Half-size of the square arena.
    float y // Vertical coordinate assigned to the spawn position.
);

// Creates and configures one enemy using the centralized settings.
Enemy createEnemy(
    EnemyVectorType type, // Gameplay and visual variant to configure.
    const glm::vec3& position, // Initial enemy position in world space.
    std::mt19937& rng // Deterministic random generator used for animation phases and approach offset.
);

// Builds the world-space shield sphere from the resistant enemy model matrix.
BoundingSphere buildEnemyShieldSphere(
    const Enemy& enemy, // Resistant enemy owning the shield.
    const TriangleMeshCollider& collider, // Mesh collider providing the local bounding sphere.
    float time // Current animation time in seconds.
);

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
);

// Removes enemies touching the player and returns their accumulated contact damage.
int removeEnemiesTouchingPlayer(
    std::vector<Enemy>& enemies, // Enemy vector tested and modified by contact removals.
    const glm::vec3& playerPosition, // Current player position in world space.
    float playerRadius, // Horizontal collision radius of the player.
    float enemyBaseRadius, // Base enemy contact radius before applying enemy scale.
    int damagePerEnemy // Damage added for each enemy reaching the player.
);
