#include "gameplay.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>
#include <iostream>

// GLAD must be included before GLFW because GLFW otherwise includes the system OpenGL header.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../camera.h"
#include "collision.h"
#include "enemy.h"
#include "game_settings.h"
#include "game_state.h"

namespace
{
    // Creates the flash, shockwave, sparks and smoke associated with one exact enemy impact.
    void spawnEnemyDeathParticles(
        const EnemyDeathEvent& event, // Exact impact position, enemy center, type and scale.
        std::mt19937& randomGenerator, // Deterministic generator used by every random particle property.
        std::vector<DeathParticle>& particles // Shared particle container receiving the new entries.
    )
    {
        if (particles.size() >= GameSettings::DeathParticles::maximumCount) return;

        const glm::vec3 lightColor = GameSettings::DeathParticles::lightColor;
        const glm::vec3 smokeColor = event.enemyType == EnemyVectorType::Translucent ? GameSettings::DeathParticles::cyanSmokeColor : GameSettings::DeathParticles::darkPurpleSmokeColor;

        DeathParticle flash;
        flash.type = DeathParticleType::Flash;
        flash.position = event.impactPosition;
        flash.velocity = glm::vec3(0.0f);
        flash.color = lightColor;
        flash.lifetime = GameSettings::DeathParticles::flashLifetime;
        flash.startSize = GameSettings::DeathParticles::flashStartSize * event.enemyScale;
        flash.endSize = GameSettings::DeathParticles::flashEndSize;
        particles.push_back(flash);

        if (particles.size() < GameSettings::DeathParticles::maximumCount)
        {
            DeathParticle shockwave;
            shockwave.type = DeathParticleType::Shockwave;
            shockwave.position = event.impactPosition;
            shockwave.velocity = glm::vec3(0.0f);
            shockwave.color = lightColor;
            shockwave.lifetime = GameSettings::DeathParticles::shockwaveLifetime;
            shockwave.startSize = GameSettings::DeathParticles::shockwaveStartSize * event.enemyScale;
            shockwave.endSize = GameSettings::DeathParticles::shockwaveEndSize * event.enemyScale;
            particles.push_back(shockwave);
        }

        std::uniform_real_distribution<float> horizontalDirectionDistribution(GameSettings::DeathParticles::sparkMinimumHorizontalDirection, GameSettings::DeathParticles::sparkMaximumHorizontalDirection);
        std::uniform_real_distribution<float> verticalDirectionDistribution(GameSettings::DeathParticles::sparkMinimumVerticalDirection, GameSettings::DeathParticles::sparkMaximumVerticalDirection);
        std::uniform_real_distribution<float> sparkSpeedDistribution(GameSettings::DeathParticles::sparkMinimumSpeed, GameSettings::DeathParticles::sparkMaximumSpeed);
        std::uniform_real_distribution<float> sparkLifetimeDistribution(GameSettings::DeathParticles::sparkMinimumLifetime, GameSettings::DeathParticles::sparkMaximumLifetime);
        std::uniform_real_distribution<float> sparkSizeDistribution(GameSettings::DeathParticles::sparkMinimumStartSize, GameSettings::DeathParticles::sparkMaximumStartSize);
        std::uniform_real_distribution<float> sparkPositionDistribution(-GameSettings::DeathParticles::sparkPositionOffset, GameSettings::DeathParticles::sparkPositionOffset);

        for (int particleIndex = 0; particleIndex < GameSettings::DeathParticles::sparkCount; ++particleIndex)
        {
            if (particles.size() >= GameSettings::DeathParticles::maximumCount) break;

            glm::vec3 direction(horizontalDirectionDistribution(randomGenerator), verticalDirectionDistribution(randomGenerator), horizontalDirectionDistribution(randomGenerator));
            float directionLengthSquared = glm::dot(direction, direction);

            if (directionLengthSquared <= 0.000001f) direction = glm::vec3(0.0f, 1.0f, 0.0f);
            else direction /= std::sqrt(directionLengthSquared);

            DeathParticle spark;
            spark.type = DeathParticleType::Spark;
            spark.position = event.impactPosition + glm::vec3(sparkPositionDistribution(randomGenerator), sparkPositionDistribution(randomGenerator), sparkPositionDistribution(randomGenerator)) * event.enemyScale;
            spark.velocity = direction * sparkSpeedDistribution(randomGenerator) * event.enemyScale;
            spark.color = lightColor;
            spark.lifetime = sparkLifetimeDistribution(randomGenerator);
            spark.startSize = sparkSizeDistribution(randomGenerator) * event.enemyScale;
            spark.endSize = GameSettings::DeathParticles::sparkEndSize;
            particles.push_back(spark);
        }

        std::uniform_real_distribution<float> smokeHorizontalPositionDistribution(-GameSettings::DeathParticles::smokeHorizontalPositionOffset, GameSettings::DeathParticles::smokeHorizontalPositionOffset);
        std::uniform_real_distribution<float> smokeVerticalPositionDistribution(GameSettings::DeathParticles::smokeMinimumVerticalPosition, GameSettings::DeathParticles::smokeMaximumVerticalPosition);
        std::uniform_real_distribution<float> smokeHorizontalVelocityDistribution(-GameSettings::DeathParticles::smokeHorizontalVelocity, GameSettings::DeathParticles::smokeHorizontalVelocity);
        std::uniform_real_distribution<float> smokeVerticalVelocityDistribution(GameSettings::DeathParticles::smokeMinimumVerticalVelocity, GameSettings::DeathParticles::smokeMaximumVerticalVelocity);
        std::uniform_real_distribution<float> smokeLifetimeDistribution(GameSettings::DeathParticles::smokeMinimumLifetime, GameSettings::DeathParticles::smokeMaximumLifetime);
        std::uniform_real_distribution<float> smokeStartSizeDistribution(GameSettings::DeathParticles::smokeMinimumStartSize, GameSettings::DeathParticles::smokeMaximumStartSize);
        std::uniform_real_distribution<float> smokeEndSizeDistribution(GameSettings::DeathParticles::smokeMinimumEndSize, GameSettings::DeathParticles::smokeMaximumEndSize);

        for (int particleIndex = 0; particleIndex < GameSettings::DeathParticles::smokeCount; ++particleIndex)
        {
            if (particles.size() >= GameSettings::DeathParticles::maximumCount) break;

            glm::vec3 randomOffset(smokeHorizontalPositionDistribution(randomGenerator), smokeVerticalPositionDistribution(randomGenerator), smokeHorizontalPositionDistribution(randomGenerator));
            glm::vec3 smokeVelocity(smokeHorizontalVelocityDistribution(randomGenerator), smokeVerticalVelocityDistribution(randomGenerator), smokeHorizontalVelocityDistribution(randomGenerator));

            DeathParticle smoke;
            smoke.type = DeathParticleType::Smoke;
            smoke.position = event.enemyCenter + randomOffset * event.enemyScale;
            smoke.velocity = smokeVelocity * event.enemyScale;
            smoke.color = smokeColor;
            smoke.lifetime = smokeLifetimeDistribution(randomGenerator);
            smoke.startSize = smokeStartSizeDistribution(randomGenerator) * event.enemyScale;
            smoke.endSize = smokeEndSizeDistribution(randomGenerator) * event.enemyScale;
            particles.push_back(smoke);
        }
    }

    // Advances active death particles and removes expired entries with swap and pop_back.
    void updateDeathParticles(
        std::vector<DeathParticle>& particles, // Particle container updated and compacted in place.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        std::size_t particleIndex = 0;

        while (particleIndex < particles.size())
        {
            DeathParticle& particle = particles[particleIndex];
            particle.age += deltaTime;

            if (particle.age >= particle.lifetime)
            {
                if (particleIndex != particles.size() - 1) std::swap(particles[particleIndex], particles.back());
                particles.pop_back();
                continue;
            }

            switch (particle.type)
            {
                case DeathParticleType::Flash:
                case DeathParticleType::Shockwave:
                    break;

                case DeathParticleType::Spark:
                    particle.velocity.y += GameSettings::DeathParticles::sparkVerticalAcceleration * deltaTime;
                    particle.velocity *= std::exp(-GameSettings::DeathParticles::sparkDamping * deltaTime);
                    particle.position += particle.velocity * deltaTime;
                    break;

                case DeathParticleType::Smoke:
                    particle.velocity.y += GameSettings::DeathParticles::smokeVerticalAcceleration * deltaTime;
                    particle.velocity *= std::exp(-GameSettings::DeathParticles::smokeDamping * deltaTime);
                    particle.position += particle.velocity * deltaTime;
                    break;
            }

            ++particleIndex;
        }
    }

    // Toggles advanced rendering options once per physical key press.
    void updateAdvancedRenderingInput(
        GLFWwindow* window, // Window providing the current keyboard state.
        AdvancedRenderingState& renderingState // Mutable rendering options and previous key states.
    )
    {
        const bool particleGeometryKeyDown = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;

        if (particleGeometryKeyDown && !renderingState.particleGeometryShaderKeyWasDown) {
            renderingState.useParticleGeometryShader = !renderingState.useParticleGeometryShader;
            std::cout << "\nParticle geometry shader: " << (renderingState.useParticleGeometryShader ? "ON" : "OFF") << std::endl;
        }

        renderingState.particleGeometryShaderKeyWasDown = particleGeometryKeyDown;

        const bool weaponToonKeyDown = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;

        if (weaponToonKeyDown && !renderingState.weaponToonShaderKeyWasDown) {
            renderingState.useWeaponToonShading = !renderingState.useWeaponToonShading;
            std::cout << "\nWeapon toon shader: " << (renderingState.useWeaponToonShading ? "ON" : "OFF") << std::endl;
        }

        renderingState.weaponToonShaderKeyWasDown = weaponToonKeyDown;

        const bool monolithBumpKeyDown = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;

        if (monolithBumpKeyDown && !renderingState.monolithBumpMappingKeyWasDown)
        {
            renderingState.useMonolithBumpMapping = !renderingState.useMonolithBumpMapping;

            std::cout << "\nMonolith bump mapping: "
                    << (renderingState.useMonolithBumpMapping ? "ON" : "OFF")
                    << std::endl;
        }

        renderingState.monolithBumpMappingKeyWasDown = monolithBumpKeyDown;
    }

    // Applies keyboard movement and rotation commands to the FPS camera.
    void processCameraInput(
        GLFWwindow* window, // Window providing the current keyboard state.
        Camera& camera, // Camera receiving translation and rotation commands.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        const float inputDeltaTime = deltaTime * GameSettings::Camera::movementSpeedMultiplier;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboardMovement(LEFT, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboardMovement(RIGHT, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboardMovement(FORWARD, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboardMovement(BACKWARD, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.ProcessKeyboardRotation(1.0f, 0.0f, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.ProcessKeyboardRotation(-1.0f, 0.0f, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.ProcessKeyboardRotation(0.0f, 1.0f, inputDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.ProcessKeyboardRotation(0.0f, -1.0f, inputDeltaTime);
    }

    // Extracts and normalizes the camera forward direction projected onto the XZ plane.
    glm::vec3 buildCameraForwardXZ(
        const glm::mat4& view // Current world-to-camera matrix.
    )
    {
        glm::vec3 cameraForwardXZ(-view[0][2], 0.0f, -view[2][2]);
        float lengthSquared = glm::dot(cameraForwardXZ, cameraForwardXZ);

        if (lengthSquared > 0.000001f) return cameraForwardXZ / std::sqrt(lengthSquared);
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }

    // Awards score for each newly completed second of survival.
    void updateSurvivalScore(
        PlayerState& player, // Player timing and score values updated in place.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        player.survivalTime += deltaTime;
        int completedSurvivalSeconds = static_cast<int>(player.survivalTime);

        if (completedSurvivalSeconds <= player.awardedSurvivalSeconds) return;

        player.score += static_cast<std::uint64_t>(completedSurvivalSeconds - player.awardedSurvivalSeconds) * GameSettings::Player::scorePerSurvivalSecond;
        player.awardedSurvivalSeconds = completedSurvivalSeconds;
    }

    // Adds one newly created enemy to the vector matching its visual and gameplay type.
    void storeEnemy(
        EnemyState& enemies, // Three separate enemy vectors receiving the new enemy.
        EnemyVectorType type, // Type determining the destination vector.
        const Enemy& enemy // Fully initialized enemy instance to store.
    )
    {
        switch (type)
        {
            case EnemyVectorType::Normal:
                enemies.normalEnemies.push_back(enemy);
                break;

            case EnemyVectorType::Translucent:
                enemies.translucentEnemies.push_back(enemy);
                break;

            case EnemyVectorType::Resistant:
                enemies.resistantEnemies.push_back(enemy);
                break;
        }
    }

    // Updates every enemy in one of the three required enemy vectors.
    void updateEnemyCollection(
        std::vector<Enemy>& enemies, // Enemy vector whose members are moved and rotated.
        const glm::vec3& playerPosition, // Current player position targeted by the enemies.
        const glm::vec3& cameraForwardXZ, // Horizontal camera direction used by the visibility rule.
        float enemyHalfViewCosineSquared, // Squared cosine threshold for the horizontal camera cone.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        for (Enemy& enemy : enemies) updateEnemyMovement(enemy, playerPosition, cameraForwardXZ, enemyHalfViewCosineSquared, GameSettings::Enemy::directChaseRadius, deltaTime);
    }

    // Advances difficulty, creates due enemies and updates all enemy movements.
    void updateEnemies(
        GameState& gameState, // Game state containing difficulty time, spawn state and enemy vectors.
        const glm::vec3& playerPosition, // Current camera position used as the player position.
        const glm::vec3& cameraForwardXZ, // Horizontal camera direction used by enemy targeting.
        float enemyHalfViewCosineSquared, // Squared cosine threshold for the horizontal camera cone.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        gameState.elapsedTime += deltaTime;
        gameState.enemies.spawnTimer += deltaTime;

        float spawnInterval = GameSettings::Enemy::initialSpawnInterval - gameState.elapsedTime * GameSettings::Enemy::spawnIntervalDecreasePerSecond;
        spawnInterval = glm::max(spawnInterval, GameSettings::Enemy::minimumSpawnInterval);

        int enemiesPerSpawn = 1 + static_cast<int>(gameState.elapsedTime / GameSettings::Enemy::additionalEnemyInterval);
        enemiesPerSpawn = glm::min(enemiesPerSpawn, GameSettings::Enemy::maximumEnemiesPerSpawn);

        if (gameState.enemies.spawnTimer >= spawnInterval)
        {
            gameState.enemies.spawnTimer = 0.0f;

            for (int enemyIndex = 0; enemyIndex < enemiesPerSpawn; ++enemyIndex)
            {
                EnemyVectorType type = randomEnemyType(gameState.enemies.randomGenerator);
                glm::vec3 spawnPosition = randomSpawnPositionAroundArena(gameState.enemies.randomGenerator, GameSettings::Arena::halfSize, GameSettings::Enemy::spawnHeight);
                Enemy enemy = createEnemy(type, spawnPosition, gameState.enemies.randomGenerator);
                storeEnemy(gameState.enemies, type, enemy);
            }
        }

        updateEnemyCollection(gameState.enemies.normalEnemies, playerPosition, cameraForwardXZ, enemyHalfViewCosineSquared, deltaTime);
        updateEnemyCollection(gameState.enemies.translucentEnemies, playerPosition, cameraForwardXZ, enemyHalfViewCosineSquared, deltaTime);
        updateEnemyCollection(gameState.enemies.resistantEnemies, playerPosition, cameraForwardXZ, enemyHalfViewCosineSquared, deltaTime);
    }

    // Applies contact damage and removes every enemy currently touching the player.
    void updateEnemyContacts(
        GameState& gameState, // Game state containing the player and three enemy vectors.
        const glm::vec3& playerPosition // Current camera position used as the player position.
    )
    {
        int contactDamage = 0;
        contactDamage += removeEnemiesTouchingPlayer(gameState.enemies.normalEnemies, playerPosition, GameSettings::Arena::playerCollisionRadius, GameSettings::Enemy::contactBaseRadius, GameSettings::Enemy::normalContactDamage);
        contactDamage += removeEnemiesTouchingPlayer(gameState.enemies.translucentEnemies, playerPosition, GameSettings::Arena::playerCollisionRadius, GameSettings::Enemy::contactBaseRadius, GameSettings::Enemy::translucentContactDamage);
        contactDamage += removeEnemiesTouchingPlayer(gameState.enemies.resistantEnemies, playerPosition, GameSettings::Arena::playerCollisionRadius, GameSettings::Enemy::contactBaseRadius, GameSettings::Enemy::resistantContactDamage);

        if (contactDamage <= 0) return;

        gameState.player.health = std::max(0, gameState.player.health - contactDamage);
        gameState.player.damageVignetteIntensity = 1.0f;
    }

    // Updates cooldowns, resolves a possible shot and advances weapon recoil.
    void updateWeapon(
        GLFWwindow* window, // Window providing the current mouse-button state.
        GameState& gameState, // Game state containing weapon, score, enemies and death particles.
        const Camera& camera, // Camera defining the logical shot origin.
        const glm::mat4& view, // Current view matrix converted to camera-to-world space for the shot direction and muzzle.
        const TriangleMeshCollider& normalGhostCollider, // Exact collider shared by normal and spectral enemies.
        const TriangleMeshCollider& resistantGhostCollider, // Exact collider used by resistant enemies.
        double currentTime, // Absolute application time used by animated enemy transforms.
        float deltaTime // Duration of the current frame in seconds.
    )
    {
        gameState.weapon.cooldownTimer = glm::max(0.0f, gameState.weapon.cooldownTimer - deltaTime);
        gameState.weapon.visualTimer = glm::max(0.0f, gameState.weapon.visualTimer - deltaTime);

        bool shootButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (!gameState.isGameOver && shootButtonPressed && gameState.weapon.cooldownTimer <= 0.0f)
        {
            glm::mat4 cameraToWorld = glm::inverse(view);
            glm::vec3 shotDirection = glm::vec3(cameraToWorld * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
            Ray shotRay = makeRay(camera.Position, shotDirection);

            gameState.weapon.beamStart = glm::vec3(cameraToWorld * glm::vec4(GameSettings::Weapon::muzzleLocalOffset, 1.0f));

            ShotResult shotResult = applyShotToEnemies(shotRay, gameState.enemies.normalEnemies, gameState.enemies.translucentEnemies, gameState.enemies.resistantEnemies, normalGhostCollider, resistantGhostCollider, static_cast<float>(currentTime), GameSettings::Weapon::beamFallbackDistance, GameSettings::Weapon::maximumShotPenetration);
            gameState.player.score += shotResult.scoreGained;

            for (const EnemyDeathEvent& deathEvent : shotResult.deathEvents) spawnEnemyDeathParticles(deathEvent, gameState.deathParticles.randomGenerator, gameState.deathParticles.particles);

            gameState.weapon.beamEnd = shotRay.origin + shotRay.direction * shotResult.beamDistance;
            gameState.weapon.cooldownTimer = GameSettings::Weapon::shotCooldown;
            gameState.weapon.visualTimer = GameSettings::Weapon::shotVisualDuration;
            gameState.weapon.recoilVelocity += GameSettings::Weapon::recoilKickStrength;
        }

        float recoilAcceleration = -gameState.weapon.recoilAmount * GameSettings::Weapon::recoilReturnStrength;
        gameState.weapon.recoilVelocity += recoilAcceleration * deltaTime;
        gameState.weapon.recoilVelocity *= std::exp(-GameSettings::Weapon::recoilDamping * deltaTime);
        gameState.weapon.recoilAmount += gameState.weapon.recoilVelocity * deltaTime;
        gameState.weapon.recoilAmount = glm::clamp(gameState.weapon.recoilAmount, 0.0f, 1.0f);
    }
}

// Restores every mutable gameplay value required to start a new session.
void restartGame(
    GLFWwindow* window, // Window whose title is restored after leaving the Game Over state.
    GameState& gameState, // Complete mutable state to reset.
    Camera& camera, // FPS camera returned to its configured starting position.
    bool& firstMouse // Mouse-callback flag reset to prevent a cursor jump.
)
{
    gameState.enemies.normalEnemies.clear();
    gameState.enemies.translucentEnemies.clear();
    gameState.enemies.resistantEnemies.clear();
    gameState.deathParticles.particles.clear();

    gameState.player.health = GameSettings::Player::maximumHealth;
    gameState.player.score = 0;
    gameState.player.survivalTime = 0.0f;
    gameState.player.awardedSurvivalSeconds = 0;
    gameState.player.damageVignetteIntensity = 0.0f;

    gameState.elapsedTime = 0.0f;
    gameState.enemies.spawnTimer = 0.0f;

    gameState.weapon.cooldownTimer = 0.0f;
    gameState.weapon.visualTimer = 0.0f;
    gameState.weapon.recoilAmount = 0.0f;
    gameState.weapon.recoilVelocity = 0.0f;

    gameState.gameOverElapsedTime = 0.0f;
    gameState.enemies.randomGenerator.seed(GameSettings::Enemy::randomSeed);
    gameState.deathParticles.randomGenerator.seed(GameSettings::DeathParticles::randomSeed);

    camera.Position = GameSettings::Camera::initialPosition;
    firstMouse = true;

    gameState.isGameOver = false;
    glfwSetWindowTitle(window, "Survival Arena");
}

// Updates one complete gameplay frame before rendering begins.
void updateGameplay(
    GLFWwindow* window, // Window providing keyboard and mouse-button states.
    GameState& gameState, // Complete mutable state updated during the frame.
    Camera& camera, // FPS camera moved by player input and used as the player position.
    bool& firstMouse, // Mouse-callback flag reset when a new session starts.
    glm::mat4& view, // View matrix refreshed after camera movement for gameplay and rendering.
    const std::vector<CircleCollider>& sceneColliders, // Static circular obstacles used by player movement.
    const TriangleMeshCollider& normalGhostCollider, // Exact mesh collider shared by normal and spectral enemies.
    const TriangleMeshCollider& resistantGhostCollider, // Exact mesh collider used by resistant enemies.
    float enemyHalfViewCosineSquared, // Squared cosine of the horizontal camera half-angle with its visibility margin.
    double currentTime, // Absolute application time used by animated enemy transforms.
    float deltaTime // Duration of the current frame in seconds.
)
{
    if (!gameState.isGameOver && gameState.player.health <= 0) gameState.isGameOver = true;
    if (gameState.isGameOver) gameState.gameOverElapsedTime += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
    updateAdvancedRenderingInput(window, gameState.advancedRendering);

    bool restartKeyDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (gameState.isGameOver && restartKeyDown && !gameState.restartKeyWasDown) restartGame(window, gameState, camera, firstMouse);
    gameState.restartKeyWasDown = restartKeyDown;

    gameState.player.damageVignetteIntensity = glm::max(0.0f, gameState.player.damageVignetteIntensity - deltaTime / GameSettings::Player::damageVignetteDuration);
    updateDeathParticles(gameState.deathParticles.particles, deltaTime);

    if (!gameState.isGameOver)
    {
        glm::vec3 previousCameraPosition = camera.Position;
        processCameraInput(window, camera, deltaTime);
        glm::vec3 desiredCameraPosition = camera.Position;
        camera.Position = resolvePlayerMovement(previousCameraPosition, desiredCameraPosition, GameSettings::Arena::playerCollisionRadius, GameSettings::Arena::halfSize, sceneColliders);
    }

    view = camera.GetViewMatrix();
    glm::vec3 cameraForwardXZ = buildCameraForwardXZ(view);

    if (!gameState.isGameOver)
    {
        updateSurvivalScore(gameState.player, deltaTime);
        updateEnemies(gameState, camera.Position, cameraForwardXZ, enemyHalfViewCosineSquared, deltaTime);
    }

    updateEnemyContacts(gameState, camera.Position);
    updateWeapon(window, gameState, camera, view, normalGhostCollider, resistantGhostCollider, currentTime, deltaTime);
}
