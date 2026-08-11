#pragma once

#include <vector>

#include <glm/glm.hpp>

struct GLFWwindow;
class Camera;
struct CircleCollider;
struct GameState;
struct TriangleMeshCollider;

// Restores every mutable gameplay value required to start a new session.
void restartGame(
    GLFWwindow* window, // Window whose title is restored after leaving the Game Over state.
    GameState& gameState, // Complete mutable state to reset.
    Camera& camera, // FPS camera returned to its configured starting position.
    bool& firstMouse // Mouse-callback flag reset to prevent a cursor jump.
);

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
);
