#include <iostream>

// GLAD must be included before GLFW to avoid OpenGL header conflicts.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <map>
#include <cstddef>
#include <string>
#include <cstdint>

#include "../camera.h"
#include "../shader.h"
#include "../solutions/object.h"
#include "enemy.h"
#include "collision.h"
#include "game_settings.h"
#include "game_state.h"
#include "shader_sources.h"
#include "render_resources.h"
#include "gameplay.h"
#include "renderer.h"

// Standard-library utilities used by procedural vegetation and instancing.
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

// Updates the FPS camera orientation from mouse movement.
void mouseCallback(
    GLFWwindow* window, // Window that triggered the callback.
    double xpos, // Current horizontal cursor position.
    double ypos // Current vertical cursor position.
);

// Loads one image into the currently bound cubemap face.
void loadCubemapFace(
    const char* path, // Path of the image to load.
    GLenum targetFace // Cubemap face receiving the image.
);

// Loads one 2D texture and generates its mipmaps.
GLuint loadTexture2D(
    const char* path // Path of the image to load.
);

// Loads every diffuse texture referenced by an object's materials.
void loadObjectDiffuseTextures(
    Object& object // Object whose material textures are loaded.
);

// Generates deterministic tree instances around the arena clearing.
std::vector<VegetationInstance> generateForest(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetTreeCount // Maximum number of tree instances to generate.
);

// Generates deterministic bush instances around the forest area.
std::vector<VegetationInstance> generateForestBushes(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of bush instances to generate.
);

// Generates deterministic rock instances around the forest area.
std::vector<VegetationInstance> generateForestRocks(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of rock instances to generate.
);

// Generates deterministic grass instances across the arena.
std::vector<VegetationInstance> generateArenaGrass(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of grass instances to generate.
);

#ifndef NDEBUG
// Prints OpenGL debug messages while ignoring known non-significant notifications.
void APIENTRY glDebugOutput(
    GLenum source, // OpenGL subsystem that produced the message.
    GLenum type, // Category of the debug message.
    unsigned int id, // Driver-specific message identifier.
    GLenum severity, // Severity assigned by the driver.
    GLsizei length, // Length of the supplied message.
    const char* message, // Null-terminated debug message.
    const void* userParam // Optional callback user data.
)
{
	// Ignore known non-significant driver notifications.
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif

Camera camera(GameSettings::Camera::initialPosition);

// Prevents the first mouse event from producing a camera jump.
bool firstMouse = true;
float lastX = 0.0f;
float lastY = 0.0f;

int main(
    int argc, // Number of command-line arguments.
    char* argv[] // Command-line argument values.
)
{
	std::cout << "Survival Arena - graphics base" << std::endl;


	// ============================================================
	// 1. WINDOW / OPENGL INITIALIZATION
	// ============================================================
	// Create the OpenGL context
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	// Request a debug context in non-release builds.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	// Create the fullscreen window
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
	GLFWwindow* window = glfwCreateWindow(videoMode->width, videoMode->height, "Survival Arena", primaryMonitor, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);

	// Capture mouse movement for FPS camera
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouseCallback);

	// Load OpenGL functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glViewport(0, 0, videoMode->width, videoMode->height);

	glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	// Keeps every OpenGL-owning local object inside the lifetime of the active context.
	{
		// ============================================================
		// 2. SHADERS
		// ============================================================
		Shader refractionShader(ShaderSources::refractionVertex, ShaderSources::refractionFragment);
		Shader cubeMapShader(ShaderSources::cubeMapVertex, ShaderSources::cubeMapFragment);
		Shader texturedLightingShader(ShaderSources::texturedLightingVertex, ShaderSources::texturedLightingFragment);
		Shader weaponToonShader(ShaderSources::texturedLightingVertex, ShaderSources::weaponToonFragment);
		Shader instancedVegetationShader(ShaderSources::instancedVegetationVertex, ShaderSources::texturedLightingFragment);
		Shader reflectionShader(ShaderSources::refractionVertex, ShaderSources::reflectionFragment);
		Shader hudColorShader(ShaderSources::hudColorVertex, ShaderSources::hudColorFragment);
		Shader energyBeamShader(ShaderSources::energyBeamVertex, ShaderSources::energyBeamFragment);
		Shader energyShieldShader(ShaderSources::energyShieldVertex, ShaderSources::energyShieldFragment);
		Shader damageVignetteShader(ShaderSources::damageVignetteVertex, ShaderSources::damageVignetteFragment);
		Shader deathParticleShader(ShaderSources::deathParticleVertex, ShaderSources::deathParticleFragment);
		Shader deathParticleGeometryShader(ShaderSources::deathParticleGeometryVertex, ShaderSources::deathParticleGeometry, ShaderSources::deathParticleGeometryFragment);

		// ============================================================
		// 3. SCENE OBJECTS
		// ============================================================
		// Skybox cube
		char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
		Object skyboxCube(pathCube);
		skyboxCube.makeObject(cubeMapShader, false);

		// Reusable cube mesh for the arena floor and outer walls
		Object arenaCube(pathCube);
		arenaCube.makeObject(texturedLightingShader, true);

		// ------------------------------------------------------------
		// Energy blaster
		// ------------------------------------------------------------
		char pathEnergyBlaster[] = PATH_TO_OBJECTS "/EnergyBlaster/energy_blaster/lightning_gun_original.obj";

		Object energyBlaster(pathEnergyBlaster);
		energyBlaster.makeObject(texturedLightingShader, true);
		loadObjectDiffuseTextures(energyBlaster);

		// ------------------------------------------------------------
		// Normal ghost
		// ------------------------------------------------------------
		char pathGhost[] = PATH_TO_OBJECTS "/Ghosts/Ghost.obj";
		Object normalGhost(pathGhost);
		normalGhost.makeObject(texturedLightingShader, true);

		// ------------------------------------------------------------
		// Translucent / refractive ghost
		// ------------------------------------------------------------
		Object translucentGhost(pathGhost);
		translucentGhost.makeObject(refractionShader, false);

		// ------------------------------------------------------------
		// Resistant ghost
		// ------------------------------------------------------------
		char pathGhostSkull[] = PATH_TO_OBJECTS "/Ghosts/Ghost_Skull.obj";
		Object resistantGhost(pathGhostSkull);
		resistantGhost.makeObject(texturedLightingShader, true);

		// ------------------------------------------------------------
		// Enemy triangle-mesh colliders
		// ------------------------------------------------------------
		TriangleMeshCollider normalGhostCollider = buildTriangleMeshCollider(normalGhost.getTrianglePositions());
		TriangleMeshCollider resistantGhostCollider = buildTriangleMeshCollider(resistantGhost.getTrianglePositions());

		// ------------------------------------------------------------
		// Forest trees - KayKit Forest Nature Pack
		// ------------------------------------------------------------

		char pathTree1A[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_1_A_Color1.obj";
		Object tree1A(pathTree1A);
		tree1A.makeObject(instancedVegetationShader, true);

		char pathTree1B[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_1_B_Color1.obj";
		Object tree1B(pathTree1B);
		tree1B.makeObject(instancedVegetationShader, true);

		char pathTree1C[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_1_C_Color1.obj";
		Object tree1C(pathTree1C);
		tree1C.makeObject(instancedVegetationShader, true);

		char pathTree2A[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_2_A_Color1.obj";
		Object tree2A(pathTree2A);
		tree2A.makeObject(instancedVegetationShader, true);

		char pathTree2B[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_2_B_Color1.obj";
		Object tree2B(pathTree2B);
		tree2B.makeObject(instancedVegetationShader, true);

		char pathTree2C[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_2_C_Color1.obj";
		Object tree2C(pathTree2C);
		tree2C.makeObject(instancedVegetationShader, true);

		char pathTree2D[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_2_D_Color1.obj";
		Object tree2D(pathTree2D);
		tree2D.makeObject(instancedVegetationShader, true);

		char pathTree2E[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_2_E_Color1.obj";
		Object tree2E(pathTree2E);
		tree2E.makeObject(instancedVegetationShader, true);

		char pathTree3A[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_3_A_Color1.obj";
		Object tree3A(pathTree3A);
		tree3A.makeObject(instancedVegetationShader, true);

		char pathTree3B[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_3_B_Color1.obj";
		Object tree3B(pathTree3B);
		tree3B.makeObject(instancedVegetationShader, true);

		char pathTree3C[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_3_C_Color1.obj";
		Object tree3C(pathTree3C);
		tree3C.makeObject(instancedVegetationShader, true);

		char pathTree4A[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_4_A_Color1.obj";
		Object tree4A(pathTree4A);
		tree4A.makeObject(instancedVegetationShader, true);

		char pathTree4B[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_4_B_Color1.obj";
		Object tree4B(pathTree4B);
		tree4B.makeObject(instancedVegetationShader, true);

		char pathTree4C[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_4_C_Color1.obj";
		Object tree4C(pathTree4C);
		tree4C.makeObject(instancedVegetationShader, true);

		char pathTreeBare1[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_Bare_1_B_Color1.obj";
		Object treeBare1(pathTreeBare1);
		treeBare1.makeObject(instancedVegetationShader, true);

		char pathTreeBare2[] = PATH_TO_OBJECTS "/Nature_Pack/Tree_Bare_2_B_Color1.obj";
		Object treeBare2(pathTreeBare2);
		treeBare2.makeObject(instancedVegetationShader, true);

		std::vector<Object*> forestTreeModels = { &tree1A, &tree1B, &tree1C, &tree2A, &tree2B, 
			&tree2C, &tree2D, &tree2E, &tree3A, &tree3B, &tree3C, &tree4A, &tree4B, &tree4C, 
			&treeBare1, &treeBare2 };
		std::vector<VegetationInstance> forestTrees = generateForest(GameSettings::Vegetation::treeSeed, GameSettings::Vegetation::treeCount);
		std::vector<InstancedBatch> forestTreeBatches = createInstancedBatches(forestTreeModels, forestTrees);

		// ------------------------------------------------------------
		// Forest bushes
		// ------------------------------------------------------------
		char pathBush1A[] = PATH_TO_OBJECTS "/Nature_Pack/Bush_1_A_Color1.obj";
		Object bush1A(pathBush1A);
		bush1A.makeObject(instancedVegetationShader, true);

		char pathBush2B[] = PATH_TO_OBJECTS "/Nature_Pack/Bush_2_B_Color1.obj";
		Object bush2B(pathBush2B);
		bush2B.makeObject(instancedVegetationShader, true);

		char pathBush3B[] = PATH_TO_OBJECTS "/Nature_Pack/Bush_3_B_Color1.obj";
		Object bush3B(pathBush3B);
		bush3B.makeObject(instancedVegetationShader, true);

		char pathBush4C[] = PATH_TO_OBJECTS "/Nature_Pack/Bush_4_C_Color1.obj";
		Object bush4C(pathBush4C);
		bush4C.makeObject(instancedVegetationShader, true);

		std::vector<Object*> forestBushModels = { &bush1A, &bush2B, &bush3B, &bush4C };
		std::vector<VegetationInstance> forestBushes = generateForestBushes(GameSettings::Vegetation::bushSeed, GameSettings::Vegetation::bushCount);
		std::vector<InstancedBatch> forestBushBatches = createInstancedBatches(forestBushModels, forestBushes);

		// ------------------------------------------------------------
		// Forest rocks
		// ------------------------------------------------------------
		char pathRock1B[] = PATH_TO_OBJECTS "/Nature_Pack/Rock_1_B_Color1.obj";
		Object rock1B(pathRock1B);
		rock1B.makeObject(instancedVegetationShader, true);

		char pathRock1F[] = PATH_TO_OBJECTS "/Nature_Pack/Rock_1_F_Color1.obj";
		Object rock1F(pathRock1F);
		rock1F.makeObject(instancedVegetationShader, true);

		char pathRock2B[] = PATH_TO_OBJECTS "/Nature_Pack/Rock_2_B_Color1.obj";
		Object rock2B(pathRock2B);
		rock2B.makeObject(instancedVegetationShader, true);

		char pathRock3C[] = PATH_TO_OBJECTS "/Nature_Pack/Rock_3_C_Color1.obj";
		Object rock3C(pathRock3C);
		rock3C.makeObject(instancedVegetationShader, true);

		std::vector<Object*> forestRockModels = { &rock1B, &rock1F, &rock2B, &rock3C };
		std::vector<VegetationInstance> forestRocks = generateForestRocks(GameSettings::Vegetation::rockSeed, GameSettings::Vegetation::rockCount);
		std::vector<InstancedBatch> forestRockBatches = createInstancedBatches(forestRockModels, forestRocks);

		// ------------------------------------------------------------
		// Grass
		// ------------------------------------------------------------
		char pathGrass1C[] = PATH_TO_OBJECTS "/Nature_Pack/Grass_1_C_Color1.obj";
		Object grass1C(pathGrass1C);
		grass1C.makeObject(instancedVegetationShader, true);

		char pathGrass1D[] = PATH_TO_OBJECTS "/Nature_Pack/Grass_1_D_Color1.obj";
		Object grass1D(pathGrass1D);
		grass1D.makeObject(instancedVegetationShader, true);

		char pathGrass2C[] = PATH_TO_OBJECTS "/Nature_Pack/Grass_2_C_Color1.obj";
		Object grass2C(pathGrass2C);
		grass2C.makeObject(instancedVegetationShader, true);

		char pathGrass2D[] = PATH_TO_OBJECTS "/Nature_Pack/Grass_2_D_Color1.obj";
		Object grass2D(pathGrass2D);
		grass2D.makeObject(instancedVegetationShader, true);

		std::vector<Object*> grassModels = { &grass1C, &grass1D, &grass2C, &grass2D };
		std::vector<VegetationInstance> arenaGrass = generateArenaGrass(GameSettings::Vegetation::grassSeed, GameSettings::Vegetation::grassCount);
		std::vector<InstancedBatch> grassBatches = createInstancedBatches(grassModels, arenaGrass);

		// ------------------------------------------------------------
		// Resistant ghost shield
		// ------------------------------------------------------------	
		ProceduralSphere shieldSphere = createProceduralSphere(GameSettings::SceneRendering::shieldSectorCount, GameSettings::SceneRendering::shieldStackCount);

		// ------------------------------------------------------------
		// Scenery objects
		// ------------------------------------------------------------
		char pathMonolith[] = PATH_TO_OBJECTS "/Scenery/Monolith.obj";
		Object reflectiveMonolith(pathMonolith);
		reflectiveMonolith.makeObject(reflectionShader, true);


		double prev = 0;
		int deltaFrame = 0;
		// Updates the window title with a periodically averaged frame rate.
		auto fps = [&](double now) {
			double deltaTime = now - prev;
			deltaFrame++;
			if (deltaTime > GameSettings::Application::fpsRefreshInterval) {
				prev = now;
				const double fpsCount = (double)deltaFrame / deltaTime;
				deltaFrame = 0;
				std::cout << "\r FPS: " << fpsCount;
				std::cout.flush();
			}
		};

		// ------------------------------------------------------------
		// Reusable rendering resources
		// ------------------------------------------------------------
		HudResources hudResources = createHudResources(videoMode->width, videoMode->height);
		EnergyBeamResources energyBeamResources = createEnergyBeamResources();
		DeathParticleRenderResources deathParticleResources = createDeathParticleRenderResources();

		// ============================================================
		// 4. MODEL TRANSFORMS
		// ============================================================
		glm::mat4 view = camera.GetViewMatrix();

		float aspectRatio = static_cast<float>(videoMode->width) / static_cast<float>(videoMode->height);

		glm::mat4 perspective = camera.GetProjectionMatrix(GameSettings::Camera::verticalFovDegrees, aspectRatio, GameSettings::Camera::nearClipDistance, GameSettings::Camera::farClipDistance);

		/*
		Convert the vertical field of view into a horizontal field
		of view. The test receives a small margin so that an enemy
		near the edge of the screen still counts as visible.
		*/
		float cameraHorizontalHalfFov = std::atan(std::tan(glm::radians(GameSettings::Camera::verticalFovDegrees) * 0.5f) * aspectRatio);
		const float enemyViewMargin = glm::radians(GameSettings::Camera::enemyViewMarginDegrees);
		float enemyHalfViewCosine = std::cos(cameraHorizontalHalfFov + enemyViewMargin);
		const float enemyHalfViewCosineSquared = enemyHalfViewCosine * enemyHalfViewCosine;
	
		// ------------------------------------------------------------
		// Arena floor
		// ------------------------------------------------------------
		glm::mat4 floorModel = glm::mat4(1.0f);
		floorModel = glm::translate(floorModel, glm::vec3(0.0f, GameSettings::Arena::groundY, 0.0f));
		floorModel = glm::scale(floorModel, glm::vec3(GameSettings::Arena::halfSize, GameSettings::Arena::floorHalfHeight, GameSettings::Arena::halfSize));
		glm::mat4 floorInverseModel = glm::transpose(glm::inverse(floorModel));

		// ------------------------------------------------------------
		// Arena outer walls
		// ------------------------------------------------------------
		const float wallCenterY = GameSettings::Arena::groundY + GameSettings::Arena::wallHeight * 0.5f;

		glm::mat4 leftWallModel = glm::mat4(1.0f);
		leftWallModel = glm::translate(leftWallModel, glm::vec3(-GameSettings::Arena::halfSize, wallCenterY, 0.0f));
		leftWallModel = glm::scale(leftWallModel, glm::vec3(GameSettings::Arena::wallThickness, GameSettings::Arena::wallHeight * 0.5f, GameSettings::Arena::halfSize));
		glm::mat4 leftWallInverseModel = glm::transpose(glm::inverse(leftWallModel));

		glm::mat4 rightWallModel = glm::mat4(1.0f);
		rightWallModel = glm::translate(rightWallModel, glm::vec3(GameSettings::Arena::halfSize, wallCenterY, 0.0f));
		rightWallModel = glm::scale(rightWallModel, glm::vec3(GameSettings::Arena::wallThickness, GameSettings::Arena::wallHeight * 0.5f, GameSettings::Arena::halfSize));
		glm::mat4 rightWallInverseModel = glm::transpose(glm::inverse(rightWallModel));

		glm::mat4 backWallModel = glm::mat4(1.0f);
		backWallModel = glm::translate(backWallModel, glm::vec3(0.0f, wallCenterY, -GameSettings::Arena::halfSize));
		backWallModel = glm::scale(backWallModel, glm::vec3(GameSettings::Arena::halfSize, GameSettings::Arena::wallHeight * 0.5f, GameSettings::Arena::wallThickness));
		glm::mat4 backWallInverseModel = glm::transpose(glm::inverse(backWallModel));

		glm::mat4 frontWallModel = glm::mat4(1.0f);
		frontWallModel = glm::translate(frontWallModel, glm::vec3(0.0f, wallCenterY, GameSettings::Arena::halfSize));
		frontWallModel = glm::scale(frontWallModel, glm::vec3(GameSettings::Arena::halfSize, GameSettings::Arena::wallHeight * 0.5f, GameSettings::Arena::wallThickness));
		glm::mat4 frontWallInverseModel = glm::transpose(glm::inverse(frontWallModel));

		// ------------------------------------------------------------
		// Runtime game state
		// ------------------------------------------------------------
		GameState gameState;

		// ------------------------------------------------------------
		// Black reflective monolith
		// ------------------------------------------------------------
		glm::mat4 monolithModel = glm::mat4(1.0f);
		monolithModel = glm::translate(monolithModel, GameSettings::Arena::monolithPosition);
		monolithModel = glm::rotate(monolithModel, glm::radians(GameSettings::Arena::monolithRotationYDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
		monolithModel = glm::scale(monolithModel, glm::vec3(GameSettings::Arena::monolithScale));
		glm::mat4 monolithInverseModel = glm::transpose(glm::inverse(monolithModel));

		// ------------------------------------------------------------
		// Scene colliders
		// ------------------------------------------------------------
		std::vector<CircleCollider> sceneColliders;
		sceneColliders.reserve(forestTrees.size() + forestRocks.size() + 1);

		// trees colliders
		for (const VegetationInstance& tree : forestTrees) {
			sceneColliders.push_back({ glm::vec2(tree.position.x, tree.position.z), tree.collisionRadius });}
	
		// rocks colliders
		for (const VegetationInstance& rock : forestRocks) {
	    	sceneColliders.push_back({ glm::vec2(rock.position.x, rock.position.z), rock.collisionRadius });}

		// monolith collider
		sceneColliders.push_back({ glm::vec2(GameSettings::Arena::monolithPosition.x, GameSettings::Arena::monolithPosition.z), GameSettings::Arena::monolithCollisionRadius });

	

		// ============================================================
		// 5. STATIC LIGHTING UNIFORMS
		// ============================================================

		// Moonlight - direction points from the scene toward the moon

		// Textured lighting shader (ground, walls and textured ghosts)
		texturedLightingShader.use();
		texturedLightingShader.setFloat("shininess", GameSettings::Lighting::texturedShininess);
		texturedLightingShader.setVector3f("light.direction", GameSettings::Lighting::moonDirection);
		texturedLightingShader.setVector3f("light.color", GameSettings::Lighting::moonColor);
		texturedLightingShader.setFloat("light.ambient_strength", GameSettings::Lighting::texturedAmbientStrength);
		texturedLightingShader.setFloat("light.diffuse_strength", GameSettings::Lighting::texturedDiffuseStrength);
		texturedLightingShader.setFloat("light.specular_strength", GameSettings::Lighting::texturedSpecularStrength);
		texturedLightingShader.setVector3f("fog.color", GameSettings::Lighting::fogColor);
		texturedLightingShader.setFloat("fog.density", GameSettings::Lighting::fogDensity);
		
		// Toon lighting shader (weapons)
		weaponToonShader.use();
		weaponToonShader.setInteger("diffuseTexture", 1);
		weaponToonShader.setFloat("shininess", GameSettings::Lighting::texturedShininess);
		weaponToonShader.setVector3f("light.direction", GameSettings::Lighting::moonDirection);
		weaponToonShader.setVector3f("light.color", GameSettings::Lighting::moonColor);
		weaponToonShader.setFloat("light.ambient_strength", GameSettings::Lighting::texturedAmbientStrength);
		weaponToonShader.setFloat("light.diffuse_strength", GameSettings::Lighting::texturedDiffuseStrength);
		weaponToonShader.setFloat("light.specular_strength", GameSettings::Lighting::texturedSpecularStrength);
		weaponToonShader.setVector3f("fog.color", GameSettings::Lighting::fogColor);
		weaponToonShader.setFloat("fog.density", GameSettings::Lighting::fogDensity);
		weaponToonShader.setFloat("toonLevels", GameSettings::Weapon::toonLightingLevels);
		weaponToonShader.setFloat("toonSpecularThreshold", GameSettings::Weapon::toonSpecularThreshold);

		// Instanced vegetation uses darker lighting with reduced specular highlights.
		instancedVegetationShader.use();
		instancedVegetationShader.setFloat("shininess", GameSettings::Lighting::vegetationShininess);
		instancedVegetationShader.setVector3f("light.direction", GameSettings::Lighting::moonDirection);
		instancedVegetationShader.setVector3f("light.color", GameSettings::Lighting::moonColor);
		instancedVegetationShader.setFloat("light.ambient_strength", GameSettings::Lighting::vegetationAmbientStrength);
		instancedVegetationShader.setFloat("light.diffuse_strength", GameSettings::Lighting::vegetationDiffuseStrength);
		instancedVegetationShader.setFloat("light.specular_strength", GameSettings::Lighting::vegetationSpecularStrength);
		instancedVegetationShader.setVector3f("fog.color", GameSettings::Lighting::fogColor);
		instancedVegetationShader.setFloat("fog.density", GameSettings::Lighting::fogDensity);

		// ============================================================
		// 6. CUBEMAP TEXTURE
		// ============================================================
		GLuint cubeMapTexture;
		glGenTextures(1, &cubeMapTexture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

		// texture parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/moonlit_night/";

		std::map<std::string, GLenum> facesToLoad = {
			{pathToCubeMap + "posx.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
			{pathToCubeMap + "posy.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
			{pathToCubeMap + "posz.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
			{pathToCubeMap + "negx.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
			{pathToCubeMap + "negy.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
			{pathToCubeMap + "negz.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
		};
		//load the six faces
		for (const std::pair<std::string, GLenum>& pair : facesToLoad) {
			loadCubemapFace(pair.first.c_str(), pair.second);
		}

		// ============================================================
		// 7. 2D TEXTURES
		// ============================================================

		// Forest clearing ground texture
		std::string groundTexturePath = PATH_TO_TEXTURE "/Ground/Grass/Grass_normal_up.png";
		GLuint groundTexture = loadTexture2D(groundTexturePath.c_str());

		// Nature Pack texture
		std::string forestTexturePath = PATH_TO_TEXTURE "/Forest/forest_texture.png";
		GLuint forestTexture = loadTexture2D(forestTexturePath.c_str());

		std::string forestWallTexturePath = PATH_TO_TEXTURE "/Ground/Grass/Grass_darked_up.png";
		GLuint forestWallTexture = loadTexture2D(forestWallTexturePath.c_str());

		// Ghost texture
		std::string monsterAtlasTexturePath = PATH_TO_TEXTURE "/Ghosts/Atlas_Monsters.png";
		GLuint monsterAtlasTexture = loadTexture2D(monsterAtlasTexturePath.c_str());

		// Resistant ghost shield
		std::string shieldTexturePath = PATH_TO_TEXTURE "/EnemyShield/GreenShield.jpg";
		GLuint shieldTexture = loadTexture2D(shieldTexturePath.c_str());

		// ============================================================
		// 8. SCENE FRAMEBUFFER
		// ============================================================
		SceneFramebufferResources sceneCapture = createSceneFramebufferResources(videoMode->width, videoMode->height);

		// ============================================================
		// 9. STATIC SHADER UNIFORMS
		// ============================================================
		refractionShader.use();
		refractionShader.setInteger("sceneTexture", 2);
		refractionShader.setVector3f("tintColour", GameSettings::SceneRendering::spectralTintColor);

		refractionShader.setFloat("fadeNearDistance", GameSettings::SceneRendering::spectralFadeNearDistance);
		refractionShader.setFloat("fadeFarDistance", GameSettings::SceneRendering::spectralFadeFarDistance);

		cubeMapShader.use();
		cubeMapShader.setInteger("cubemapSampler", 0);

		texturedLightingShader.use();
		texturedLightingShader.setInteger("diffuseTexture", 1);

		reflectionShader.use();
		reflectionShader.setInteger("cubemapSampler", 0);
		reflectionShader.setVector3f("baseColour", GameSettings::SceneRendering::reflectionBaseColor);
		reflectionShader.setFloat("reflectionStrength", GameSettings::SceneRendering::reflectionStrength);

		instancedVegetationShader.use();
		instancedVegetationShader.setInteger("diffuseTexture", 1);

		energyShieldShader.use();
		energyShieldShader.setInteger("shieldTexture", 1);
		energyShieldShader.setVector3f("tintColour", GameSettings::SceneRendering::shieldTintColor);
		energyShieldShader.setFloat("baseAlpha", GameSettings::SceneRendering::shieldBaseAlpha);

		RenderContext renderContext;
		renderContext.framebufferWidth = videoMode->width;
		renderContext.framebufferHeight = videoMode->height;
		renderContext.shaders.refraction = &refractionShader;
		renderContext.shaders.cubeMap = &cubeMapShader;
		renderContext.shaders.texturedLighting = &texturedLightingShader;
		renderContext.shaders.instancedVegetation = &instancedVegetationShader;
		renderContext.shaders.reflection = &reflectionShader;
		renderContext.shaders.hudColor = &hudColorShader;
		renderContext.shaders.energyBeam = &energyBeamShader;
		renderContext.shaders.energyShield = &energyShieldShader;
		renderContext.shaders.damageVignette = &damageVignetteShader;
		renderContext.shaders.deathParticle = &deathParticleShader;
		renderContext.shaders.deathParticleGeometry = &deathParticleGeometryShader;
		renderContext.objects.skyboxCube = &skyboxCube;
		renderContext.objects.arenaCube = &arenaCube;
		renderContext.objects.energyBlaster = &energyBlaster;
		renderContext.objects.normalGhost = &normalGhost;
		renderContext.objects.translucentGhost = &translucentGhost;
		renderContext.objects.resistantGhost = &resistantGhost;
		renderContext.objects.reflectiveMonolith = &reflectiveMonolith;
		renderContext.textures.cubeMap = cubeMapTexture;
		renderContext.textures.ground = groundTexture;
		renderContext.textures.forest = forestTexture;
		renderContext.textures.forestWall = forestWallTexture;
		renderContext.textures.monsterAtlas = monsterAtlasTexture;
		renderContext.textures.shield = shieldTexture;
		renderContext.batches.forestTrees = &forestTreeBatches;
		renderContext.batches.forestRocks = &forestRockBatches;
		renderContext.batches.forestBushes = &forestBushBatches;
		renderContext.batches.grass = &grassBatches;
		renderContext.transforms.floorModel = floorModel;
		renderContext.transforms.floorInverseModel = floorInverseModel;
		renderContext.transforms.leftWallModel = leftWallModel;
		renderContext.transforms.leftWallInverseModel = leftWallInverseModel;
		renderContext.transforms.rightWallModel = rightWallModel;
		renderContext.transforms.rightWallInverseModel = rightWallInverseModel;
		renderContext.transforms.backWallModel = backWallModel;
		renderContext.transforms.backWallInverseModel = backWallInverseModel;
		renderContext.transforms.frontWallModel = frontWallModel;
		renderContext.transforms.frontWallInverseModel = frontWallInverseModel;
		renderContext.transforms.monolithModel = monolithModel;
		renderContext.transforms.monolithInverseModel = monolithInverseModel;
		renderContext.shieldSphere = &shieldSphere;
		renderContext.resistantGhostCollider = &resistantGhostCollider;
		renderContext.hudResources = &hudResources;
		renderContext.sceneCapture = &sceneCapture;
		renderContext.energyBeamResources = &energyBeamResources;
		renderContext.deathParticleResources = &deathParticleResources;
		renderContext.shaders.weaponToon = &weaponToonShader;

		glfwSwapInterval(GameSettings::Application::swapInterval);

		double lastFrameTime = glfwGetTime();


		// --------------------------------------------------------
		// 11. MAIN RENDER LOOP
		// --------------------------------------------------------

		while (!glfwWindowShouldClose(window))
		{
			double now = glfwGetTime();
			float deltaTime = static_cast<float>(now - lastFrameTime);
			lastFrameTime = now;

			updateGameplay(window, gameState, camera, firstMouse, view, sceneColliders, normalGhostCollider, resistantGhostCollider, enemyHalfViewCosineSquared, now, deltaTime);
			glfwPollEvents();
			renderFrame(renderContext, gameState, camera, view, perspective, now);

			fps(now);
			glfwSwapBuffers(window);
		}

		// Clean up OpenGL resources before destroying the context.
		deleteInstancedBatches(forestTreeBatches);
		deleteInstancedBatches(forestRockBatches);
		deleteInstancedBatches(forestBushBatches);
		deleteInstancedBatches(grassBatches);
		deleteProceduralSphere(shieldSphere);

		deleteSceneFramebufferResources(sceneCapture);
		glDeleteTextures(1, &cubeMapTexture);
		glDeleteTextures(1, &groundTexture);
		glDeleteTextures(1, &forestTexture);
		glDeleteTextures(1, &forestWallTexture);
		glDeleteTextures(1, &monsterAtlasTexture);
		deleteEnergyBeamResources(energyBeamResources);
		glDeleteTextures(1, &shieldTexture);
		deleteHudResources(hudResources);
		deleteDeathParticleRenderResources(deathParticleResources);
	}

	// Clean up window/context after every owning object has released its OpenGL handles.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

// ============================================================
// 11. HELPER FUNCTIONS
// ============================================================

// Loads one image into the currently bound cubemap face.
void loadCubemapFace(
    const char* path, // Path of the image to load.
    GLenum targetFace // Cubemap face receiving the image.
)
{
	int imWidth, imHeight, imNrChannels;
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{

		glTexImage2D(targetFace, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}
	stbi_image_free(data);
}

// Updates the FPS camera orientation from mouse movement.
void mouseCallback(
    GLFWwindow* window, // Window that triggered the callback.
    double xpos, // Current horizontal cursor position.
    double ypos // Current vertical cursor position.
)
{
    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.ProcessMouseMovement(xoffset, yoffset);
}


// Loads one 2D texture and generates its mipmaps.
GLuint loadTexture2D(
    const char* path // Path of the image to load.
)
{
    GLuint texture;

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Repeat wrapping supports tiled materials.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Images usually have their vertical origin at the top.
    stbi_set_flip_vertically_on_load(true);

    int imageWidth;
    int imageHeight;
    int numberOfChannels;
    unsigned char* data = stbi_load(path, &imageWidth, &imageHeight, &numberOfChannels, 0);

	if (data)
	{
		GLenum format = (numberOfChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, imageWidth, imageHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load 2D texture: " << path << std::endl;
        std::cout << stbi_failure_reason() << std::endl;
    }

    stbi_image_free(data);

    // Avoid affecting a possible later cubemap load.
    stbi_set_flip_vertically_on_load(false);

    return texture;
}


// Loads every diffuse texture referenced by an object's materials.
void loadObjectDiffuseTextures(
    Object& object // Object whose material textures are loaded.
)
{
	for (ObjectMaterial& material : object.materials)
	{
		if (material.diffuseTexturePath.empty())
			continue;

		material.diffuseTexture = loadTexture2D(material.diffuseTexturePath.c_str());

		std::cout << "Loaded diffuse texture for "
			<< material.name << ": "
			<< material.diffuseTexturePath << std::endl;
	}
}


// Generates deterministic tree instances around the arena clearing.
std::vector<VegetationInstance> generateForest(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetTreeCount // Maximum number of tree instances to generate.
)
{
    std::vector<VegetationInstance> forest;
    std::vector<glm::vec2> acceptedPositions;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> positionDistribution(-(GameSettings::Arena::halfSize - GameSettings::Vegetation::treeArenaMargin), GameSettings::Arena::halfSize - GameSettings::Vegetation::treeArenaMargin);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    std::uniform_real_distribution<float> rotationDistribution(0.0f, GameSettings::Vegetation::maximumRotationDegrees);
    std::uniform_real_distribution<float> scaleDistribution(GameSettings::Vegetation::treeMinimumScale, GameSettings::Vegetation::treeMaximumScale);
    std::uniform_int_distribution<int> variant3Distribution(0, 2);
    std::uniform_int_distribution<int> variant5Distribution(0, 4);
    std::uniform_int_distribution<int> bareDistribution(0, 1);

    const float forestInnerEdge = GameSettings::Vegetation::treeInnerEdge;
    const float forestOuterEdge = GameSettings::Arena::halfSize - GameSettings::Vegetation::treeArenaMargin;
    const int maxAttempts = GameSettings::Vegetation::maximumGenerationAttempts;

    int attempts = 0;

    while (static_cast<int>(forest.size()) < targetTreeCount && attempts < maxAttempts)
    {
        attempts++;

        float x = positionDistribution(rng);
        float z = positionDistribution(rng);

        float edgeDistance = std::max(std::abs(x), std::abs(z));

        if (edgeDistance < forestInnerEdge || edgeDistance > forestOuterEdge)
            continue;

        float forestDepth = (edgeDistance - forestInnerEdge) / (forestOuterEdge - forestInnerEdge);

        float densityProbability = GameSettings::Vegetation::treeBaseDensity + GameSettings::Vegetation::treeDepthDensity * forestDepth * forestDepth;

        if (random01(rng) > densityProbability)
            continue;

        float minimumSpacing = GameSettings::Vegetation::treeMaximumSpacing - GameSettings::Vegetation::treeSpacingReduction * forestDepth;
        bool tooClose = false;

        for (const glm::vec2& existingPosition : acceptedPositions)
        {
            float dx = existingPosition.x - x;
            float dz = existingPosition.y - z;

            if (dx * dx + dz * dz < minimumSpacing * minimumSpacing)
            {
                tooClose = true;
                break;
            }
        }

        if (tooClose)
            continue;

        float typeRoll = random01(rng);
        int modelIndex = 0;

        float firLimit = GameSettings::Vegetation::firBaseLimit + GameSettings::Vegetation::firDepthLimit * forestDepth;
        float squareLimit = GameSettings::Vegetation::squareBaseLimit + GameSettings::Vegetation::squareDepthLimit * forestDepth;

        if (typeRoll < firLimit)
            modelIndex = 11 + variant3Distribution(rng);
        else if (typeRoll < squareLimit)
            modelIndex = 3 + variant5Distribution(rng);
        else if (typeRoll < GameSettings::Vegetation::ovalTypeLimit)
            modelIndex = 8 + variant3Distribution(rng);
        else if (typeRoll < GameSettings::Vegetation::roundTypeLimit)
            modelIndex = variant3Distribution(rng);
        else
            modelIndex = 14 + bareDistribution(rng);

        float rotationY = rotationDistribution(rng);
        float treeScale = scaleDistribution(rng);

        glm::mat4 treeModel = glm::mat4(1.0f);
        treeModel = glm::translate(treeModel, glm::vec3(x, GameSettings::Arena::groundY, z));
        treeModel = glm::rotate(treeModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        treeModel = glm::scale(treeModel, glm::vec3(treeScale));

        VegetationInstance instance;
        instance.model = treeModel;
        instance.modelIndex = modelIndex;
		instance.position = glm::vec3(x, GameSettings::Arena::groundY, z);
		instance.collisionRadius = GameSettings::Arena::treeCollisionRadius * treeScale;

        forest.push_back(instance);
        acceptedPositions.push_back(glm::vec2(x, z));
    }

    std::cout << "Generated forest with " << forest.size() << " trees" << std::endl;

    return forest;
}

// Generates deterministic bush instances around the forest area.
std::vector<VegetationInstance> generateForestBushes(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of bush instances to generate.
)
{
    std::vector<VegetationInstance> bushes;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> positionDistribution(-(GameSettings::Arena::halfSize - GameSettings::Vegetation::bushArenaMargin), GameSettings::Arena::halfSize - GameSettings::Vegetation::bushArenaMargin);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    std::uniform_real_distribution<float> rotationDistribution(0.0f, GameSettings::Vegetation::maximumRotationDegrees);
    std::uniform_real_distribution<float> scaleDistribution(GameSettings::Vegetation::bushMinimumScale, GameSettings::Vegetation::bushMaximumScale);
    std::uniform_int_distribution<int> modelDistribution(0, 3);

    const float forestInnerEdge = GameSettings::Vegetation::bushInnerEdge;
    const float forestOuterEdge = GameSettings::Arena::halfSize - GameSettings::Vegetation::bushArenaMargin;
    const int maxAttempts = GameSettings::Vegetation::maximumGenerationAttempts;

    int attempts = 0;

    while (static_cast<int>(bushes.size()) < targetCount && attempts < maxAttempts)
    {
        attempts++;

        float x = positionDistribution(rng);
        float z = positionDistribution(rng);
        float edgeDistance = std::max(std::abs(x), std::abs(z));

        if (edgeDistance < forestInnerEdge || edgeDistance > forestOuterEdge)
            continue;

        float forestDepth = (edgeDistance - forestInnerEdge) / (forestOuterEdge - forestInnerEdge);
        float densityProbability = GameSettings::Vegetation::bushBaseDensity + GameSettings::Vegetation::bushDepthDensity * forestDepth;

        if (random01(rng) > densityProbability)
            continue;

        float rotationY = rotationDistribution(rng);
        float propScale = scaleDistribution(rng);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, GameSettings::Arena::groundY, z));
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(propScale));

        VegetationInstance instance;
        instance.model = model;
        instance.modelIndex = modelDistribution(rng);

        bushes.push_back(instance);
    }

    std::cout << "Generated forest with " << bushes.size() << " bushes" << std::endl;

    return bushes;
}


// Generates deterministic rock instances around the forest area.
std::vector<VegetationInstance> generateForestRocks(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of rock instances to generate.
)
{
    std::vector<VegetationInstance> rocks;
    std::vector<glm::vec2> acceptedPositions;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> positionDistribution(-(GameSettings::Arena::halfSize - GameSettings::Vegetation::rockArenaMargin), GameSettings::Arena::halfSize - GameSettings::Vegetation::rockArenaMargin);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    std::uniform_real_distribution<float> rotationDistribution(0.0f, GameSettings::Vegetation::maximumRotationDegrees);
    std::uniform_real_distribution<float> scaleDistribution(GameSettings::Vegetation::rockMinimumScale, GameSettings::Vegetation::rockMaximumScale);
    std::uniform_int_distribution<int> modelDistribution(0, 3);

    const float forestInnerEdge = GameSettings::Vegetation::rockInnerEdge;
   	const float forestOuterEdge = GameSettings::Arena::halfSize - GameSettings::Vegetation::rockArenaMargin;
    const int maxAttempts = GameSettings::Vegetation::maximumGenerationAttempts;

    int attempts = 0;

    while (static_cast<int>(rocks.size()) < targetCount && attempts < maxAttempts)
    {
        attempts++;

        float x = positionDistribution(rng);
        float z = positionDistribution(rng);
        float edgeDistance = std::max(std::abs(x), std::abs(z));

        if (edgeDistance < forestInnerEdge || edgeDistance > forestOuterEdge)
            continue;

        float forestDepth = (edgeDistance - forestInnerEdge) / (forestOuterEdge - forestInnerEdge);
        float densityProbability = GameSettings::Vegetation::rockBaseDensity + GameSettings::Vegetation::rockDepthDensity * forestDepth;

        if (random01(rng) > densityProbability)
            continue;

        bool tooClose = false;

        for (const glm::vec2& existingPosition : acceptedPositions)
        {
            float dx = existingPosition.x - x;
            float dz = existingPosition.y - z;

            if (dx * dx + dz * dz < GameSettings::Vegetation::rockMinimumSpacing * GameSettings::Vegetation::rockMinimumSpacing)
            {
                tooClose = true;
                break;
            }
        }

        if (tooClose)
            continue;

        float rotationY = rotationDistribution(rng);
        float propScale = scaleDistribution(rng);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, GameSettings::Arena::groundY, z));
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(propScale));

        VegetationInstance instance;
        instance.model = model;
        instance.modelIndex = modelDistribution(rng);
		instance.position = glm::vec3(x, GameSettings::Arena::groundY, z);
		instance.collisionRadius = GameSettings::Arena::rockCollisionRadius * propScale;

        rocks.push_back(instance);
        acceptedPositions.push_back(glm::vec2(x, z));
    }

    std::cout << "Generated forest with " << rocks.size() << " rocks" << std::endl;

    return rocks;
}


// Generates deterministic grass instances across the arena.
std::vector<VegetationInstance> generateArenaGrass(
    unsigned int seed, // Seed controlling deterministic placement.
    int targetCount // Maximum number of grass instances to generate.
)
{
    std::vector<VegetationInstance> grass;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> positionDistribution(-(GameSettings::Arena::halfSize - GameSettings::Vegetation::grassArenaMargin), GameSettings::Arena::halfSize - GameSettings::Vegetation::grassArenaMargin);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    std::uniform_real_distribution<float> rotationDistribution(0.0f, GameSettings::Vegetation::maximumRotationDegrees);
    std::uniform_real_distribution<float> scaleDistribution(GameSettings::Vegetation::grassMinimumScale, GameSettings::Vegetation::grassMaximumScale);
    std::uniform_int_distribution<int> modelDistribution(0, 3);

    const int maxAttempts = GameSettings::Vegetation::maximumGenerationAttempts;
    int attempts = 0;

    while (static_cast<int>(grass.size()) < targetCount && attempts < maxAttempts)
    {
        attempts++;

        float x = positionDistribution(rng);
        float z = positionDistribution(rng);

        float edgeDistance = std::max(std::abs(x), std::abs(z));
        float forestInfluence = glm::clamp((edgeDistance - GameSettings::Vegetation::grassInfluenceStart) / GameSettings::Vegetation::grassInfluenceRange, 0.0f, 1.0f);

        float spawnProbability = GameSettings::Vegetation::grassBaseSpawnProbability + GameSettings::Vegetation::grassForestSpawnProbability * forestInfluence;

        if (random01(rng) > spawnProbability)
            continue;

        float rotationY = rotationDistribution(rng);
        float grassScale = scaleDistribution(rng);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, GameSettings::Arena::groundY, z));
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(grassScale));

        VegetationInstance instance;
        instance.model = model;
        instance.modelIndex = modelDistribution(rng);

        grass.push_back(instance);
    }

    std::cout << "Generated arena with " << grass.size() << " grass tufts" << std::endl;

    return grass;
}

