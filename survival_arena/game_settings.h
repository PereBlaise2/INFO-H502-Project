#pragma once

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

// Centralizes gameplay, scene and visual parameters that are intentionally adjustable.
// Algorithm-specific constants such as numerical epsilons remain next to the algorithms that use them.
namespace GameSettings
{
    namespace Application
    {
        constexpr int swapInterval = 1; // Enables vertical synchronization when set to 1; use 0 to disable it.
        constexpr double fpsRefreshInterval = 0.5; // Controls how often the FPS value is recalculated and displayed, in seconds.
    }

    namespace Camera
    {
        const glm::vec3 initialPosition = glm::vec3(0.0f, 1.2f, 0.1f); // Defines the camera position at game start and after restarting.
        constexpr float verticalFovDegrees = 45.0f; // Controls the camera's vertical field of view, in degrees.
        constexpr float nearClipDistance = 0.05f; // Defines the nearest distance rendered by the perspective projection.
        constexpr float farClipDistance = 300.0f; // Defines the farthest distance rendered by the perspective projection.
        constexpr float movementSpeedMultiplier = 1.7f; // Multiplies the base movement speed provided by the camera class.
        constexpr float enemyViewMarginDegrees = 15.0f; // Extends the horizontal camera angle used to decide whether enemies chase the player directly.
    }

    namespace Arena
    {
        constexpr float halfSize = 80.0f; // Defines half the arena width and depth measured from its center.
        constexpr float groundY = -1.0f; // Defines the world-space height of the arena ground.
        constexpr float floorHalfHeight = 0.1f; // Controls half the thickness of the floor cube.
        constexpr float wallHeight = 5.0f; // Controls the height of the four arena walls.
        constexpr float wallThickness = 0.2f; // Controls the thickness of the four arena walls.

        constexpr float playerCollisionRadius = 0.45f; // Defines the player's horizontal collision radius.
        constexpr float treeCollisionRadius = 0.55f; // Defines the base circular collision radius assigned to trees.
        constexpr float rockCollisionRadius = 0.80f; // Defines the base circular collision radius assigned to rocks.
        constexpr float monolithCollisionRadius = 3.6f; // Defines the monolith's circular collision radius.

        const glm::vec3 monolithPosition = glm::vec3(0.0f, 2.0f, -14.0f); // Defines the monolith position in world space.
        constexpr float monolithRotationYDegrees = 12.0f; // Controls the monolith rotation around the vertical axis, in degrees.
        constexpr float monolithScale = 0.025f; // Controls the uniform scale applied to the monolith model.
    }

    namespace Vegetation
    {
        constexpr unsigned int treeSeed = 666u; // Fixes the random tree layout so it remains reproducible.
        constexpr unsigned int bushSeed = 667u; // Fixes the random bush layout so it remains reproducible.
        constexpr unsigned int rockSeed = 668u; // Fixes the random rock layout so it remains reproducible.
        constexpr unsigned int grassSeed = 669u; // Fixes the random grass layout so it remains reproducible.

        constexpr int treeCount = 2000; // Controls the target number of generated tree instances.
        constexpr int bushCount = 550; // Controls the target number of generated bush instances.
        constexpr int rockCount = 200; // Controls the target number of generated rock instances.
        constexpr int grassCount = 2000; // Controls the target number of generated grass instances.

        constexpr int maximumGenerationAttempts = 150000; // Limits placement attempts to prevent vegetation generation from looping indefinitely.
        constexpr float maximumRotationDegrees = 360.0f; // Defines the maximum random Y rotation used for vegetation instances.

        constexpr float treeArenaMargin = 1.5f; // Keeps tree placement away from the arena's outer boundary.
        constexpr float treeInnerEdge = 30.0f; // Defines the distance from the arena center where the forest begins.
        constexpr float treeMinimumScale = 0.85f; // Defines the smallest random scale applied to a tree.
        constexpr float treeMaximumScale = 1.15f; // Defines the largest random scale applied to a tree.
        constexpr float treeBaseDensity = 0.08f; // Controls the minimum tree spawn probability near the inner forest edge.
        constexpr float treeDepthDensity = 0.92f; // Controls the additional tree spawn probability deeper inside the forest.
        constexpr float treeMaximumSpacing = 3.2f; // Defines the largest minimum spacing used between trees near the inner forest edge.
        constexpr float treeSpacingReduction = 1.4f; // Reduces required tree spacing as placement moves deeper into the forest.
        constexpr float firBaseLimit = 0.50f; // Defines the initial random-selection threshold for fir tree models.
        constexpr float firDepthLimit = 0.15f; // Defines how the fir-selection threshold changes deeper inside the forest.
        constexpr float squareBaseLimit = 0.85f; // Defines the initial random-selection threshold for square-canopy tree models.
        constexpr float squareDepthLimit = 0.05f; // Defines how the square-canopy selection threshold changes with forest depth.
        constexpr float ovalTypeLimit = 0.95f; // Defines the random-selection threshold separating oval tree models from later types.
        constexpr float roundTypeLimit = 0.985f; // Defines the random-selection threshold separating round trees from bare trees.

        constexpr float bushArenaMargin = 1.5f; // Keeps bush placement away from the arena's outer boundary.
        constexpr float bushInnerEdge = 26.0f; // Defines the distance from the arena center where bush generation begins.
        constexpr float bushMinimumScale = 0.75f; // Defines the smallest random scale applied to a bush.
        constexpr float bushMaximumScale = 1.30f; // Defines the largest random scale applied to a bush.
        constexpr float bushBaseDensity = 0.15f; // Controls the minimum bush spawn probability near the forest edge.
        constexpr float bushDepthDensity = 0.85f; // Controls the additional bush spawn probability deeper inside the forest.

        constexpr float rockArenaMargin = 3.0f; // Keeps rock placement away from the arena's outer boundary.
        constexpr float rockInnerEdge = 39.0f; // Defines the distance from the arena center where forest-rock generation begins.
        constexpr float rockMinimumScale = 0.70f; // Defines the smallest random scale applied to a rock.
        constexpr float rockMaximumScale = 1.40f; // Defines the largest random scale applied to a rock.
        constexpr float rockBaseDensity = 0.20f; // Controls the minimum rock spawn probability near the rock-generation edge.
        constexpr float rockDepthDensity = 0.80f; // Controls the additional rock spawn probability deeper inside the forest.
        constexpr float rockMinimumSpacing = 4.0f; // Defines the minimum allowed distance between generated rocks.

        constexpr float grassArenaMargin = 2.0f; // Keeps grass placement away from the arena's outer boundary.
        constexpr float grassMinimumScale = 0.75f; // Defines the smallest random scale applied to a grass instance.
        constexpr float grassMaximumScale = 1.25f; // Defines the largest random scale applied to a grass instance.
        constexpr float grassInfluenceStart = 25.0f; // Defines where forest proximity starts increasing grass spawn probability.
        constexpr float grassInfluenceRange = 33.0f; // Defines the distance over which forest proximity affects grass density.
        constexpr float grassBaseSpawnProbability = 0.30f; // Controls the grass spawn probability in the central arena area.
        constexpr float grassForestSpawnProbability = 0.70f; // Controls the additional grass spawn probability near the forest.
    }

    namespace Enemy
    {
        constexpr unsigned int randomSeed = 666u; // Fixes enemy type, spawn and animation randomization so runs remain reproducible.
        constexpr int normalTypeThreshold = 60; // Sets the upper percentage threshold for spawning normal enemies.
        constexpr int translucentTypeThreshold = 90; // Sets the cumulative percentage threshold for spawning translucent enemies.

        constexpr float spawnEdgeOffset = 5.0f; // Moves enemy spawn positions inward from the arena boundary.
        constexpr float spawnHeight = -0.5f; // Defines the base Y position used when enemies are created.
        constexpr float initialSpawnInterval = 3.0f; // Defines the delay between enemy spawns at the start of a game, in seconds.
        constexpr float minimumSpawnInterval = 0.6f; // Defines the shortest possible delay between enemy spawns, in seconds.
        constexpr float spawnIntervalDecreasePerSecond = 0.015f; // Controls how quickly the spawn delay decreases as survival time increases.
        constexpr float additionalEnemyInterval = 45.0f; // Defines how often another enemy is added to each spawn wave, in seconds.
        constexpr int maximumEnemiesPerSpawn = 3; // Limits the number of enemies that can be created in one spawn wave.

        constexpr float directChaseRadius = 7.0f; // Makes enemies inside this horizontal radius chase the player directly even when off-screen.
        constexpr float contactBaseRadius = 0.65f; // Defines the base enemy contact radius before multiplying it by enemy scale.
        constexpr int normalContactDamage = 10; // Defines damage dealt when a normal enemy touches the player.
        constexpr int translucentContactDamage = 15; // Defines damage dealt when a translucent enemy touches the player.
        constexpr int resistantContactDamage = 25; // Defines damage dealt when a resistant enemy touches the player.

        constexpr float approachMinimumRadius = 5.0f; // Defines the minimum distance of an off-screen enemy's personal approach point from the player.
        constexpr float approachMaximumRadius = 20.0f; // Defines the maximum distance of an off-screen enemy's personal approach point from the player.

        constexpr float normalSpeed = 5.0f; // Controls the movement speed of normal enemies.
        constexpr float normalScale = 1.0f; // Controls the uniform model scale of normal enemies.
        constexpr float normalRotationSpeed = 4.0f; // Controls how quickly normal enemies turn toward the player.
        constexpr float normalHoverAmplitude = 0.20f; // Controls the vertical hover distance of normal enemies.
        constexpr float normalHoverSpeed = 2.2f; // Controls the hover oscillation speed of normal enemies.
        constexpr float normalSwayAmplitudeDegrees = 5.0f; // Controls the maximum side-to-side sway angle of normal enemies.
        constexpr float normalSwaySpeed = 3.0f; // Controls the sway oscillation speed of normal enemies.

        constexpr float translucentSpeed = 4.0f; // Controls the movement speed of translucent enemies.
        constexpr float translucentScale = 1.0f; // Controls the uniform model scale of translucent enemies.
        constexpr float translucentRotationSpeed = 5.0f; // Controls how quickly translucent enemies turn toward the player.
        constexpr float translucentHoverAmplitude = 0.32f; // Controls the vertical hover distance of translucent enemies.
        constexpr float translucentHoverSpeed = 3.0f; // Controls the hover oscillation speed of translucent enemies.
        constexpr float translucentSwayAmplitudeDegrees = 8.0f; // Controls the maximum side-to-side sway angle of translucent enemies.
        constexpr float translucentSwaySpeed = 4.0f; // Controls the sway oscillation speed of translucent enemies.

        constexpr float resistantSpeed = 3.0f; // Controls the movement speed of resistant enemies.
        constexpr float resistantScale = 1.8f; // Controls the uniform model scale of resistant enemies.
        constexpr float resistantRotationSpeed = 2.5f; // Controls how quickly resistant enemies turn toward the player.
        constexpr float resistantHoverAmplitude = 0.12f; // Controls the vertical hover distance of resistant enemies.
        constexpr float resistantHoverSpeed = 1.6f; // Controls the hover oscillation speed of resistant enemies.
        constexpr float resistantSwayAmplitudeDegrees = 3.0f; // Controls the maximum side-to-side sway angle of resistant enemies.
        constexpr float resistantSwaySpeed = 2.0f; // Controls the sway oscillation speed of resistant enemies.

        constexpr float resistantShieldRadiusMultiplier = 1.10f; // Enlarges the resistant enemy's bounding sphere to define its shield radius.

        constexpr std::uint64_t normalScore = 10; // Defines the base score value of a normal enemy killed by a shot.
        constexpr std::uint64_t translucentScore = 15; // Defines the base score value of a translucent enemy killed by a shot.
        constexpr std::uint64_t resistantScore = 30; // Defines the base score value of a resistant enemy killed by a shot.
    }

    namespace Player
    {
        constexpr int maximumHealth = 100; // Defines the player's health at game start and after restarting.
        constexpr std::uint64_t scorePerSurvivalSecond = 1; // Defines how many score points are awarded per complete second survived.
        constexpr float damageVignetteDuration = 0.65f; // Controls how long the damage vignette remains visible after contact.
        const glm::vec3 damageVignetteColor = glm::vec3(0.65f, 0.0f, 0.0f); // Defines the RGB color of the damage vignette.
        constexpr float damageVignetteEdgeWidth = 0.30f; // Controls how far the damage vignette extends inward from the screen edges.
    }

    namespace Weapon
    {
        constexpr float shotCooldown = 0.6f; // Defines the minimum delay between two shots, in seconds.
        constexpr float beamFallbackDistance = 240.0f; // Defines the visible beam length when no target or shield stops the shot.
        constexpr std::size_t maximumShotPenetration = 100; // Limits how many enemies one shot can destroy.

        constexpr float beamGrowthDuration = 0.05f; // Controls the duration of the beam's growth phase, in seconds.
        constexpr float beamHoldDuration = 0.045f; // Controls how long the complete beam remains fully visible, in seconds.
        constexpr float beamFadeDuration = 0.18f; // Controls the duration of the beam's disappearance phase, in seconds.
        constexpr float shotVisualDuration = beamGrowthDuration + beamHoldDuration + beamFadeDuration; // Defines the total duration of the three beam animation phases.

        constexpr float beamGlowRadius = 0.10f; // Controls the radius of the beam's outer glow volume.
        constexpr float beamCoreRadius = 0.05f; // Controls the radius of the beam's bright inner core.
        const glm::vec3 beamGlowColor = glm::vec3(0.05f, 0.55f, 1.00f); // Defines the RGB color of the beam's outer glow.
        const glm::vec3 beamCoreColor = glm::vec3(0.75f, 0.95f, 1.00f); // Defines the RGB color of the beam's inner core.
        constexpr float beamGlowAlpha = 0.28f; // Controls the base opacity of the beam's outer glow.
        constexpr float beamCoreAlpha = 0.95f; // Controls the base opacity of the beam's inner core.
        constexpr float beamCoreFadeReduction = 0.35f; // Controls how much faster the beam core opacity decreases during fading.

        const glm::vec3 muzzleLocalOffset = glm::vec3(0.26f, -0.19f, -0.7f); // Defines the beam's visual starting point relative to the camera-mounted weapon.
        const glm::vec3 basePosition = glm::vec3(0.5f, -0.6f, -1.3f); // Defines the weapon model's base position in camera-local space.
        constexpr float modelScale = 0.03f; // Controls the uniform scale of the weapon model.
        constexpr float yawDegrees = 8.0f; // Controls the weapon model's horizontal rotation in camera-local space.
        constexpr float pitchDegrees = 5.0f; // Controls the weapon model's vertical rotation in camera-local space.
        constexpr float rollDegrees = -5.0f; // Controls the weapon model's roll rotation in camera-local space.

        constexpr float recoilKickStrength = 5.5f; // Controls the velocity impulse applied to the recoil animation when firing.
        constexpr float recoilReturnStrength = 18.0f; // Controls how strongly the weapon is pulled back toward its resting position.
        constexpr float recoilDamping = 7.0f; // Controls how quickly recoil velocity is damped.
        constexpr float recoilBackwardDistance = 0.16f; // Controls the maximum backward displacement produced by recoil.
        constexpr float recoilDownDistance = 0.035f; // Controls the maximum downward displacement produced by recoil.
        constexpr float recoilPitchDegrees = 7.0f; // Controls the maximum upward pitch rotation produced by recoil.

        constexpr float toonLightingLevels = 4.0f; // Number of discrete diffuse-lighting bands used by the weapon toon shader.
        constexpr float toonSpecularThreshold = 0.45f; // Minimum specular intensity required to display the sharp toon highlight.
    }

    namespace Lighting
    {
        const glm::vec3 moonDirection = glm::normalize(glm::vec3(-0.30f, 0.9f, 1.00f)); // Defines the normalized direction used by the scene's directional moon light.
        const glm::vec3 moonColor = glm::vec3(0.70f, 0.82f, 1.00f); // Defines the RGB color of the moon light.
        const glm::vec3 fogColor = glm::vec3(0.07f, 0.10f, 0.15f); // Defines the RGB color toward which distant scene fragments fade.
        constexpr float fogDensity = 0.015f; // Controls how quickly fog increases with distance.

        constexpr float texturedShininess = 16.0f; // Controls the specular highlight sharpness of normally textured objects.
        constexpr float texturedAmbientStrength = 0.26f; // Controls the ambient-light contribution for normally textured objects.
        constexpr float texturedDiffuseStrength = 0.72f; // Controls the diffuse-light contribution for normally textured objects.
        constexpr float texturedSpecularStrength = 0.08f; // Controls the specular-light contribution for normally textured objects.

        constexpr float vegetationShininess = 8.0f; // Controls the specular highlight sharpness of instanced vegetation.
        constexpr float vegetationAmbientStrength = 0.17f; // Controls the ambient-light contribution for instanced vegetation.
        constexpr float vegetationDiffuseStrength = 0.65f; // Controls the diffuse-light contribution for instanced vegetation.
        constexpr float vegetationSpecularStrength = 0.03f; // Controls the specular-light contribution for instanced vegetation.

        constexpr float resistantShininess = 48.0f; // Controls the specular highlight sharpness of resistant enemies.
        constexpr float resistantSpecularStrength = 0.40f; // Controls the specular-light contribution of resistant enemies.
        constexpr float wallSpecularStrength = 0.0f; // Controls the specular-light contribution of the arena walls.

        constexpr float defaultTextureRepeat = 1.0f; // Defines the default UV repetition factor for textured models.
        constexpr float floorTextureRepeat = 40.0f; // Defines how many times the floor texture repeats across the floor.
        constexpr float wallTextureRepeat = 16.0f; // Defines how many times the wall texture repeats across each wall.
    }

    namespace SceneRendering
    {
        const glm::vec3 clearColor = glm::vec3(0.01f, 0.015f, 0.03f); // Defines the RGB color used to clear the scene framebuffer.

        const glm::vec3 reflectionBaseColor = glm::vec3(0.01f, 0.015f, 0.025f); // Defines the non-reflective base color mixed into the monolith.
        constexpr float reflectionStrength = 0.30f; // Controls how strongly the cubemap reflection influences the monolith color.

        const glm::vec3 spectralTintColor = glm::vec3(0.20f, 0.90f, 1.00f); // Defines the cyan tint applied to spectral enemies.
        constexpr float spectralFadeNearDistance = 10.0f; // Defines the distance where spectral enemy fading begins.
        constexpr float spectralFadeFarDistance = 40.0f; // Defines the distance where spectral enemies become almost invisible.
        constexpr float spectralDistortionStrength = 0.08f; // Controls the strength of screen-space refraction around spectral enemies.
        constexpr float spectralTintStrength = 0.20f; // Controls how strongly the spectral tint is mixed with the refracted scene color.

        constexpr int shieldSectorCount = 48; // Controls the horizontal tessellation of the procedural shield sphere.
        constexpr int shieldStackCount = 24; // Controls the vertical tessellation of the procedural shield sphere.
        const glm::vec3 shieldTintColor = glm::vec3(0.18f, 1.00f, 0.40f); // Defines the RGB tint of resistant enemy shields.
        constexpr float shieldBaseAlpha = 0.38f; // Controls the base opacity of resistant enemy shields.
        constexpr float shieldRotationSpeed = 0.20f; // Controls the rotation speed of the visible shield texture animation.

        constexpr float monolithBumpScale = 1.5f; // Frequency of the procedural rocky details.
        constexpr float monolithBumpStrength = 0.10f; // Strength of the normal perturbation.
        constexpr float monolithBumpSampleStep = 0.02f; // World-space sampling distance used to estimate the surface slope.
    }

    namespace Hud
    {
        constexpr float crosshairHalfWidthPixels = 10.0f; // Controls half the horizontal length of each crosshair arm, in pixels.
        constexpr float crosshairHalfHeightPixels = 10.0f; // Controls half the vertical length of each crosshair arm, in pixels.
        constexpr float crosshairHalfThicknessPixels = 1.5f; // Controls half the thickness of the crosshair lines, in pixels.
        const glm::vec3 crosshairColor = glm::vec3(0.9f, 0.9f, 0.9f); // Defines the RGB color of the ready-to-fire crosshair.

        constexpr int reloadCircleSegments = 64; // Controls the geometric smoothness of the reload circle.
        constexpr float reloadCircleRadiusPixels = 17.0f; // Controls the outer radius of the reload indicator, in pixels.
        constexpr float reloadCircleThicknessPixels = 3.0f; // Controls the thickness of the reload indicator arc, in pixels.
        const glm::vec3 reloadCircleColor = glm::vec3(0.9f, 0.9f, 0.9f); // Defines the RGB color of the reload indicator.

        constexpr float healthBarLeft = -0.94f; // Defines the left edge of the health bar in normalized device coordinates.
        constexpr float healthBarRight = -0.48f; // Defines the right edge of the health bar in normalized device coordinates.
        constexpr float healthBarBottom = -0.94f; // Defines the bottom edge of the health bar in normalized device coordinates.
        constexpr float healthBarTop = -0.88f; // Defines the top edge of the health bar in normalized device coordinates.
        constexpr float healthBarBorder = 0.008f; // Controls the thickness of the health bar border in normalized device coordinates.
        const glm::vec3 healthBarBorderColor = glm::vec3(0.03f, 0.03f, 0.04f); // Defines the RGB color of the health bar border.
        const glm::vec3 healthBarBackgroundColor = glm::vec3(0.20f, 0.035f, 0.035f); // Defines the RGB color of the empty health bar background.
        const glm::vec3 lowHealthColor = glm::vec3(0.90f, 0.08f, 0.04f); // Defines the RGB color of the health fill when health is low.
        const glm::vec3 highHealthColor = glm::vec3(0.03f, 0.45f, 0.10f); // Defines the RGB color of the health fill when health is high.

        constexpr float scoreAnchorX = 0.0f; // Defines the horizontal anchor of the in-game score text.
        constexpr float scoreBottom = -0.94f; // Defines the bottom position of the in-game score text.
        constexpr float scoreDigitWidth = 0.035f; // Controls the width of each in-game score digit.
        constexpr float scoreDigitHeight = 0.080f; // Controls the height of each in-game score digit.
        constexpr float scoreThickness = 0.007f; // Controls the thickness of the segments used to draw score digits.
        constexpr float scoreSpacing = 0.009f; // Controls the horizontal spacing between score characters.
        constexpr float scoreAlignment = 0.5f; // Controls score alignment around its anchor: 0 left, 0.5 centered, 1 right.
        const glm::vec3 scoreColor = glm::vec3(0.55f, 0.82f, 0.95f); // Defines the RGB color of the in-game score text.

        constexpr float timeAnchorX = 0.94f; // Defines the horizontal anchor of the survival-time text.
        constexpr float timeBottom = 0.86f; // Defines the bottom position of the survival-time text.
        constexpr float timeDigitWidth = 0.035f; // Controls the width of each survival-time digit.
        constexpr float timeDigitHeight = 0.080f; // Controls the height of each survival-time digit.
        constexpr float timeThickness = 0.007f; // Controls the thickness of the segments used to draw survival-time digits.
        constexpr float timeSpacing = 0.009f; // Controls the horizontal spacing between survival-time characters.
        constexpr float timeAlignment = 1.0f; // Right-aligns the survival-time text around its anchor.
        const glm::vec3 timeColor = glm::vec3(0.90f, 0.90f, 0.90f); // Defines the RGB color of the survival-time text.

        constexpr float gameOverPulseBase = 0.65f; // Defines the minimum brightness contribution of the Game Over pulse.
        constexpr float gameOverPulseAmplitude = 0.20f; // Controls the brightness variation of the Game Over pulse.
        constexpr float gameOverPulseSpeed = 5.0f; // Controls how quickly the Game Over pulse oscillates.
        const glm::vec3 gameOverOverlayColor = glm::vec3(0.015f, 0.0f, 0.0f); // Defines the RGB color of the Game Over screen overlay.
        constexpr float gameOverOverlayAlpha = 0.72f; // Controls the opacity of the Game Over screen overlay.
        constexpr float gameOverBorderSize = 0.035f; // Controls the thickness of the Game Over border.
        const glm::vec3 gameOverBorderColor = glm::vec3(0.55f, 0.0f, 0.0f); // Defines the RGB color of the Game Over border.

        constexpr float finalScoreAnchorX = 0.0f; // Defines the horizontal anchor of the final score text.
        constexpr float finalScoreBottom = -0.02f; // Defines the bottom position of the final score text.
        constexpr float finalScoreDigitWidth = 0.070f; // Controls the width of each final-score digit.
        constexpr float finalScoreDigitHeight = 0.150f; // Controls the height of each final-score digit.
        constexpr float finalScoreThickness = 0.013f; // Controls the thickness of the segments used to draw final-score digits.
        constexpr float finalScoreSpacing = 0.017f; // Controls the horizontal spacing between final-score characters.
        constexpr float finalScoreAlignment = 0.5f; // Centers the final score text around its anchor.
        const glm::vec3 finalScoreColor = glm::vec3(0.90f, 0.12f, 0.08f); // Defines the RGB color of the final score text.

        constexpr float finalTimeAnchorX = 0.0f; // Defines the horizontal anchor of the final survival-time text.
        constexpr float finalTimeBottom = -0.25f; // Defines the bottom position of the final survival-time text.
        constexpr float finalTimeDigitWidth = 0.042f; // Controls the width of each final-time digit.
        constexpr float finalTimeDigitHeight = 0.090f; // Controls the height of each final-time digit.
        constexpr float finalTimeThickness = 0.008f; // Controls the thickness of the segments used to draw final-time digits.
        constexpr float finalTimeSpacing = 0.011f; // Controls the horizontal spacing between final-time characters.
        constexpr float finalTimeAlignment = 0.5f; // Centers the final time text around its anchor.
        const glm::vec3 finalTimeColor = glm::vec3(0.75f, 0.75f, 0.75f); // Defines the RGB color of the final survival-time text.
    }

    namespace DeathParticles
    {
        constexpr unsigned int randomSeed = 777u; // Fixes death-particle randomization so effects remain reproducible.
        constexpr std::size_t maximumCount = 4000; // Limits the total number of active death particles.
        constexpr int sparkCount = 24; // Controls the number of spark particles created for each enemy death.
        constexpr int smokeCount = 60; // Controls the number of smoke particles created for each enemy death.

        const glm::vec3 lightColor = glm::vec3(0.25f, 0.65f, 1.00f); // Defines the shared RGB color of flashes, sparks and shockwaves.
        const glm::vec3 darkPurpleSmokeColor = glm::vec3(0.34f, 0.10f, 0.48f); // Defines the smoke color used for normal and resistant enemies.
        const glm::vec3 cyanSmokeColor = glm::vec3(0.18f, 0.85f, 1.00f); // Defines the smoke color used for translucent enemies.

        constexpr float flashLifetime = 0.20f; // Controls how long the central impact flash remains visible.
        constexpr float flashStartSize = 72.0f; // Defines the initial point size of the central impact flash.
        constexpr float flashEndSize = 8.0f; // Defines the final point size of the central impact flash.

        constexpr float shockwaveLifetime = 0.34f; // Controls how long the expanding shockwave remains visible.
        constexpr float shockwaveStartSize = 18.0f; // Defines the initial point size of the shockwave ring.
        constexpr float shockwaveEndSize = 115.0f; // Defines the final point size of the shockwave ring.
        constexpr float shockwaveMaximumAlpha = 0.90f; // Controls the maximum opacity of the shockwave ring.

        constexpr float smokeMinimumLifetime = 0.65f; // Defines the shortest possible smoke-particle lifetime.
        constexpr float smokeMaximumLifetime = 1.0f; // Defines the longest possible smoke-particle lifetime.
        constexpr float smokeMinimumStartSize = 20.0f; // Defines the smallest possible initial smoke-particle size.
        constexpr float smokeMaximumStartSize = 35.0f; // Defines the largest possible initial smoke-particle size.
        constexpr float smokeMinimumEndSize = 50.0f; // Defines the smallest possible final smoke-particle size.
        constexpr float smokeMaximumEndSize = 80.0f; // Defines the largest possible final smoke-particle size.
        constexpr float smokeAlpha = 0.80f; // Controls the base opacity of smoke particles.
        constexpr float smokeFadeInEnd = 0.14f; // Defines the normalized lifetime point where smoke finishes fading in.

        constexpr float sparkMinimumHorizontalDirection = -1.0f; // Defines the minimum random X and Z component used for spark directions.
        constexpr float sparkMaximumHorizontalDirection = 1.0f; // Defines the maximum random X and Z component used for spark directions.
        constexpr float sparkMinimumVerticalDirection = 0.10f; // Defines the minimum upward component used for spark directions.
        constexpr float sparkMaximumVerticalDirection = 1.10f; // Defines the maximum upward component used for spark directions.
        constexpr float sparkMinimumSpeed = 2.5f; // Defines the slowest possible initial spark speed.
        constexpr float sparkMaximumSpeed = 6.0f; // Defines the fastest possible initial spark speed.
        constexpr float sparkMinimumLifetime = 0.45f; // Defines the shortest possible spark lifetime.
        constexpr float sparkMaximumLifetime = 0.80f; // Defines the longest possible spark lifetime.
        constexpr float sparkMinimumStartSize = 8.0f; // Defines the smallest possible initial spark size.
        constexpr float sparkMaximumStartSize = 16.0f; // Defines the largest possible initial spark size.
        constexpr float sparkEndSize = 1.5f; // Defines the size reached by sparks at the end of their lifetime.
        constexpr float sparkPositionOffset = 0.12f; // Controls the random initial displacement of sparks around the impact point.
        constexpr float sparkVerticalAcceleration = 1.20f; // Controls the vertical acceleration applied to sparks during updates.
        constexpr float sparkDamping = 2.80f; // Controls how quickly spark velocity decreases over time.
        constexpr float geometrySparkHalfWidthMultiplier = 0.30f; // Controls the half-width of spark trails generated by the geometry shader relative to particle size.
        constexpr float geometrySparkHalfLengthMultiplier = 2.0f; // Controls the half-length of spark trails generated by the geometry shader relative to particle size.

        constexpr float smokeHorizontalPositionOffset = 0.65f; // Controls the random horizontal spawn spread of smoke around the enemy center.
        constexpr float smokeMinimumVerticalPosition = -0.50f; // Defines the lowest random vertical smoke spawn offset.
        constexpr float smokeMaximumVerticalPosition = 0.75f; // Defines the highest random vertical smoke spawn offset.
        constexpr float smokeHorizontalVelocity = 0.65f; // Controls the maximum random horizontal starting velocity of smoke.
        constexpr float smokeMinimumVerticalVelocity = 0.40f; // Defines the slowest upward starting velocity of smoke.
        constexpr float smokeMaximumVerticalVelocity = 1.10f; // Defines the fastest upward starting velocity of smoke.
        constexpr float smokeVerticalAcceleration = 0.40f; // Controls the upward acceleration applied to smoke during updates.
        constexpr float smokeDamping = 1.35f; // Controls how quickly smoke velocity decreases over time.
    }
}
