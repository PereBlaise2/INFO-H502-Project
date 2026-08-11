#pragma once

#include <glad/glad.h>

// Groups the OpenGL objects shared by the HUD rendering functions.
struct HudResources
{
    GLuint crosshairVAO = 0;
    GLuint crosshairVBO = 0;

    GLuint rectangleVAO = 0;
    GLuint rectangleVBO = 0;

    GLuint reloadCircleVAO = 0;
    GLuint reloadCircleVBO = 0;

    GLuint damageVignetteVAO = 0;
};

// Groups the framebuffer objects used to capture the opaque scene before refraction.
struct SceneFramebufferResources
{
    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthStencilRenderbuffer = 0;
};

// Creates the reusable OpenGL geometry used by the crosshair, HUD rectangles, reload circle and damage vignette.
HudResources createHudResources(
    int framebufferWidth, // Current framebuffer width in pixels.
    int framebufferHeight // Current framebuffer height in pixels.
);

// Deletes every OpenGL object owned by the supplied HUD resource group.
void deleteHudResources(
    HudResources& resources // HUD resource group to delete and reset.
);

// Creates the color and depth-stencil attachments used to capture the opaque scene.
SceneFramebufferResources createSceneFramebufferResources(
    int framebufferWidth, // Width of the scene capture framebuffer in pixels.
    int framebufferHeight // Height of the scene capture framebuffer in pixels.
);

// Deletes every OpenGL object owned by the supplied scene framebuffer group.
void deleteSceneFramebufferResources(
    SceneFramebufferResources& resources // Scene framebuffer resource group to delete and reset.
);
