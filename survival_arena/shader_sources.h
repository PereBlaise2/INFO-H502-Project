#pragma once

#include <string>

// Stores every GLSL source used by the application outside main.cpp.
namespace ShaderSources
{
    // Transforms refractive geometry and provides world-space position and normals.
    extern const std::string refractionVertex;

    // Samples the opaque scene texture to render fading spectral refraction.
    extern const std::string refractionFragment;

    // Renders the skybox without applying camera translation.
    extern const std::string cubeMapVertex;

    // Samples the skybox cubemap.
    extern const std::string cubeMapFragment;

    // Transforms textured geometry and prepares lighting inputs.
    extern const std::string texturedLightingVertex;

    // Applies directional lighting, texturing and distance fog.
    extern const std::string texturedLightingFragment;

    // Transforms vegetation with one model matrix per instance.
    extern const std::string instancedVegetationVertex;

    // Combines the monolith base color with a cubemap reflection.
    extern const std::string reflectionFragment;

    // Renders two-dimensional HUD vertices in normalized device coordinates.
    extern const std::string hudColorVertex;

    // Applies a uniform color and opacity to HUD geometry.
    extern const std::string hudColorFragment;

    // Transforms the energy beam volume into clip space.
    extern const std::string energyBeamVertex;

    // Applies the selected beam color and opacity.
    extern const std::string energyBeamFragment;

    // Transforms the resistant-enemy shield and prepares its animated surface data.
    extern const std::string energyShieldVertex;

    // Renders the animated energy texture, rim effect and shield pulse.
    extern const std::string energyShieldFragment;

    // Generates a full-screen triangle from the vertex identifier.
    extern const std::string damageVignetteVertex;

    // Renders the fading red damage effect around the screen edges.
    extern const std::string damageVignetteFragment;

    // Transforms point particles for the original point-sprite rendering path.
    extern const std::string deathParticleVertex;

    // Renders soft discs, rings and smoke from OpenGL point coordinates.
    extern const std::string deathParticleFragment;

    // Transforms one particle center for the geometry-shader rendering path.
    extern const std::string deathParticleGeometryVertex;

    // Expands one particle point into a camera-facing quad.
    extern const std::string deathParticleGeometry;

    // Renders soft discs, rings and smoke on geometry-generated quads.
    extern const std::string deathParticleGeometryFragment;

    // Applies quantized lighting to the textured first-person weapon.
    extern const std::string weaponToonFragment;
}
