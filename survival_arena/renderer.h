#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Object;
class Shader;
struct GameState;
struct HudResources;
struct SceneFramebufferResources;
struct TriangleMeshCollider;

// Stores one generated vegetation transform and its optional circular collider data.
struct VegetationInstance
{
    glm::mat4 model = glm::mat4(1.0f);
    int modelIndex = 0;

    glm::vec3 position = glm::vec3(0.0f);
    float collisionRadius = 0.0f;
};

// Stores the instance buffer associated with one vegetation model.
struct InstancedBatch
{
    Object* object = nullptr;
    GLuint instanceVBO = 0;
    GLsizei instanceCount = 0;
};

// Owns the procedural sphere geometry used to display resistant enemy shields.
struct ProceduralSphere
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLsizei vertexCount = 0;
};

// Owns the dynamic geometry used by the two-pass energy beam.
struct EnergyBeamResources
{
    GLuint VAO = 0;
    GLuint VBO = 0;
};

// Owns the shared dynamic buffer used by every enemy-death particle.
struct DeathParticleRenderResources
{
    GLuint VAO = 0;
    GLuint VBO = 0;
};

// Groups the model matrices required by the opaque scene pass.
struct SceneTransforms
{
    glm::mat4 floorModel = glm::mat4(1.0f);
    glm::mat4 floorInverseModel = glm::mat4(1.0f);

    glm::mat4 leftWallModel = glm::mat4(1.0f);
    glm::mat4 leftWallInverseModel = glm::mat4(1.0f);
    glm::mat4 rightWallModel = glm::mat4(1.0f);
    glm::mat4 rightWallInverseModel = glm::mat4(1.0f);
    glm::mat4 backWallModel = glm::mat4(1.0f);
    glm::mat4 backWallInverseModel = glm::mat4(1.0f);
    glm::mat4 frontWallModel = glm::mat4(1.0f);
    glm::mat4 frontWallInverseModel = glm::mat4(1.0f);

    glm::mat4 monolithModel = glm::mat4(1.0f);
    glm::mat4 monolithInverseModel = glm::mat4(1.0f);
};

// Groups non-owning shader references used during one frame.
struct RenderShaders
{
    Shader* refraction = nullptr;
    Shader* cubeMap = nullptr;
    Shader* texturedLighting = nullptr;
    Shader* instancedVegetation = nullptr;
    Shader* reflection = nullptr;
    Shader* hudColor = nullptr;
    Shader* energyBeam = nullptr;
    Shader* energyShield = nullptr;
    Shader* damageVignette = nullptr;
    Shader* deathParticle = nullptr;
    Shader* deathParticleGeometry = nullptr;
    Shader* weaponToon = nullptr;
};

// Groups non-owning object references used during one frame.
struct RenderObjects
{
    Object* skyboxCube = nullptr;
    Object* arenaCube = nullptr;
    Object* energyBlaster = nullptr;
    Object* normalGhost = nullptr;
    Object* translucentGhost = nullptr;
    Object* resistantGhost = nullptr;
    Object* reflectiveMonolith = nullptr;
};

// Groups texture handles used by the scene and transparent-effect passes.
struct RenderTextures
{
    GLuint cubeMap = 0;
    GLuint ground = 0;
    GLuint forest = 0;
    GLuint forestWall = 0;
    GLuint monsterAtlas = 0;
    GLuint shield = 0;
};

// Groups the four instanced vegetation collections without taking ownership of them.
struct RenderBatches
{
    const std::vector<InstancedBatch>* forestTrees = nullptr;
    const std::vector<InstancedBatch>* forestRocks = nullptr;
    const std::vector<InstancedBatch>* forestBushes = nullptr;
    const std::vector<InstancedBatch>* grass = nullptr;
};

// Groups every long-lived dependency required by the frame renderer.
struct RenderContext
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;

    RenderShaders shaders;
    RenderObjects objects;
    RenderTextures textures;
    RenderBatches batches;
    SceneTransforms transforms;

    const ProceduralSphere* shieldSphere = nullptr;
    const TriangleMeshCollider* resistantGhostCollider = nullptr;

    HudResources* hudResources = nullptr;
    SceneFramebufferResources* sceneCapture = nullptr;
    EnergyBeamResources* energyBeamResources = nullptr;
    DeathParticleRenderResources* deathParticleResources = nullptr;
};

// Creates one instance buffer for each model used by a vegetation group.
std::vector<InstancedBatch> createInstancedBatches(
    const std::vector<Object*>& models, // Models that can receive generated instances.
    const std::vector<VegetationInstance>& instances // Generated transforms and model indices.
);

// Deletes the instance buffers owned by the supplied batches.
void deleteInstancedBatches(
    std::vector<InstancedBatch>& batches // Batches whose instance buffers are deleted.
);

// Creates the procedural sphere mesh used by resistant enemy shields.
ProceduralSphere createProceduralSphere(
    int sectorCount, // Number of horizontal subdivisions.
    int stackCount // Number of vertical subdivisions.
);

// Deletes the OpenGL resources owned by a procedural sphere.
void deleteProceduralSphere(
    ProceduralSphere& sphere // Sphere whose resources are deleted and reset.
);

// Creates the dynamic geometry used by the energy-beam glow and core passes.
EnergyBeamResources createEnergyBeamResources();

// Deletes the OpenGL resources owned by the energy beam.
void deleteEnergyBeamResources(
    EnergyBeamResources& resources // Energy-beam resource group to delete and reset.
);

// Creates the single dynamic buffer shared by all enemy-death particles.
DeathParticleRenderResources createDeathParticleRenderResources();

// Deletes the OpenGL resources owned by the enemy-death particle renderer.
void deleteDeathParticleRenderResources(
    DeathParticleRenderResources& resources // Particle resource group to delete and reset.
);

// Renders one complete frame in the required opaque, transparent, weapon and HUD order.
void renderFrame(
    const RenderContext& context, // Long-lived scene, shader, texture and OpenGL resource references.
    GameState& gameState, // Mutable game state whose temporary particle vertices are rebuilt for upload.
    const Camera& camera, // Camera position used by lighting and billboard-like effects.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective, // Current projection matrix.
    double currentTime // Absolute application time used by animated visual effects.
);
