#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "enemy.h"
#include "game_settings.h"

// Identifies the visual behavior used by one enemy-death particle.
enum class DeathParticleType
{
    Flash,
    Spark,
    Shockwave,
    Smoke
};

// Stores the simulation data of one enemy-death particle.
struct DeathParticle
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);

    float age = 0.0f;
    float lifetime = 1.0f;
    float startSize = 10.0f;
    float endSize = 2.0f;

    DeathParticleType type = DeathParticleType::Spark;
};

// Stores one vertex uploaded to the shared enemy-death particle buffer.
struct DeathParticleVertex
{
    float positionX = 0.0f;
    float positionY = 0.0f;
    float positionZ = 0.0f;

    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;

    float alpha = 0.0f;
    float size = 0.0f;
    float shape = 0.0f; // 0 = flash, 1 = spark, 2 = shockwave, 3 = smoke.

    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
};

// Groups the three required enemy containers and their spawn state.
struct EnemyState
{
    std::vector<Enemy> normalEnemies;
    std::vector<Enemy> translucentEnemies;
    std::vector<Enemy> resistantEnemies;

    std::mt19937 randomGenerator{ GameSettings::Enemy::randomSeed };
    float spawnTimer = 0.0f;
};

// Groups the particle simulation data, upload data and deterministic generator.
struct DeathParticleState
{
    std::vector<DeathParticle> particles;
    std::vector<DeathParticleVertex> vertices;
    std::mt19937 randomGenerator{ GameSettings::DeathParticles::randomSeed };

    // Reserves the configured maximum capacity once during game initialization.
    DeathParticleState()
    {
        particles.reserve(GameSettings::DeathParticles::maximumCount);
        vertices.reserve(GameSettings::DeathParticles::maximumCount);
    }
};

// Groups every persistent value used by the weapon and its beam effect.
struct WeaponState
{
    float cooldownTimer = 0.0f;
    float visualTimer = 0.0f;

    float recoilAmount = 0.0f;
    float recoilVelocity = 0.0f;

    glm::vec3 beamStart = glm::vec3(0.0f);
    glm::vec3 beamEnd = glm::vec3(0.0f);
};

// Groups the player values shared by gameplay, damage feedback and the HUD.
struct PlayerState
{
    int health = GameSettings::Player::maximumHealth;
    std::uint64_t score = 0;

    float survivalTime = 0.0f;
    int awardedSurvivalSeconds = 0;

    float damageVignetteIntensity = 0.0f;
};

// Stores mutable options used to compare advanced rendering features at runtime.
struct AdvancedRenderingState
{
    bool useParticleGeometryShader = false;
    bool particleGeometryShaderKeyWasDown = false; // avoid toggles

    bool useWeaponToonShading = false;
    bool weaponToonShaderKeyWasDown = false;

    bool useMonolithBumpMapping = false;
    bool monolithBumpMappingKeyWasDown = false;
};

// Groups the complete mutable game state while keeping rendering resources separate.
struct GameState
{
    EnemyState enemies;
    DeathParticleState deathParticles;
    WeaponState weapon;
    PlayerState player;

    AdvancedRenderingState advancedRendering;

    float elapsedTime = 0.0f;

    bool isGameOver = false;
    float gameOverElapsedTime = 0.0f;
    bool restartKeyWasDown = false;
};
