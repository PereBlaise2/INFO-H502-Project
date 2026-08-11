#include "render_resources.h"

#include <cstddef>
#include <stdexcept>

#include "game_settings.h"

// Creates the reusable OpenGL geometry used by the crosshair, HUD rectangles, reload circle and damage vignette.
HudResources createHudResources(
    int framebufferWidth, // Current framebuffer width in pixels.
    int framebufferHeight // Current framebuffer height in pixels.
)
{
    HudResources resources;

    glGenVertexArrays(1, &resources.damageVignetteVAO);

    const float crosshairHalfWidth = 2.0f * GameSettings::Hud::crosshairHalfWidthPixels / static_cast<float>(framebufferWidth);
    const float crosshairHalfHeight = 2.0f * GameSettings::Hud::crosshairHalfHeightPixels / static_cast<float>(framebufferHeight);
    const float crosshairThicknessX = 2.0f * GameSettings::Hud::crosshairHalfThicknessPixels / static_cast<float>(framebufferWidth);
    const float crosshairThicknessY = 2.0f * GameSettings::Hud::crosshairHalfThicknessPixels / static_cast<float>(framebufferHeight);

    const float crosshairVertices[] =
    {
        -crosshairHalfWidth, -crosshairThicknessY,
        crosshairHalfWidth, -crosshairThicknessY,
        crosshairHalfWidth, crosshairThicknessY,
        -crosshairHalfWidth, -crosshairThicknessY,
        crosshairHalfWidth, crosshairThicknessY,
        -crosshairHalfWidth, crosshairThicknessY,

        -crosshairThicknessX, -crosshairHalfHeight,
        crosshairThicknessX, -crosshairHalfHeight,
        crosshairThicknessX, crosshairHalfHeight,
        -crosshairThicknessX, -crosshairHalfHeight,
        crosshairThicknessX, crosshairHalfHeight,
        -crosshairThicknessX, crosshairHalfHeight
    };

    glGenVertexArrays(1, &resources.crosshairVAO);
    glGenBuffers(1, &resources.crosshairVBO);
    glBindVertexArray(resources.crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &resources.rectangleVAO);
    glGenBuffers(1, &resources.rectangleVBO);
    glBindVertexArray(resources.rectangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.rectangleVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    constexpr std::size_t reloadCircleMaximumFloatCount = static_cast<std::size_t>(GameSettings::Hud::reloadCircleSegments + 1) * 2 * 2;

    glGenVertexArrays(1, &resources.reloadCircleVAO);
    glGenBuffers(1, &resources.reloadCircleVBO);
    glBindVertexArray(resources.reloadCircleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.reloadCircleVBO);
    glBufferData(GL_ARRAY_BUFFER, reloadCircleMaximumFloatCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return resources;
}

// Deletes every OpenGL object owned by the supplied HUD resource group.
void deleteHudResources(
    HudResources& resources // HUD resource group to delete and reset.
)
{
    if (resources.crosshairVBO != 0) glDeleteBuffers(1, &resources.crosshairVBO);
    if (resources.crosshairVAO != 0) glDeleteVertexArrays(1, &resources.crosshairVAO);
    if (resources.rectangleVBO != 0) glDeleteBuffers(1, &resources.rectangleVBO);
    if (resources.rectangleVAO != 0) glDeleteVertexArrays(1, &resources.rectangleVAO);
    if (resources.reloadCircleVBO != 0) glDeleteBuffers(1, &resources.reloadCircleVBO);
    if (resources.reloadCircleVAO != 0) glDeleteVertexArrays(1, &resources.reloadCircleVAO);
    if (resources.damageVignetteVAO != 0) glDeleteVertexArrays(1, &resources.damageVignetteVAO);

    resources = HudResources();
}

// Creates the color and depth-stencil attachments used to capture the opaque scene.
SceneFramebufferResources createSceneFramebufferResources(
    int framebufferWidth, // Width of the scene capture framebuffer in pixels.
    int framebufferHeight // Height of the scene capture framebuffer in pixels.
)
{
    SceneFramebufferResources resources;

    glGenFramebuffers(1, &resources.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.framebuffer);

    glGenTextures(1, &resources.colorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, resources.colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, framebufferWidth, framebufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources.colorTexture, 0);

    glGenRenderbuffers(1, &resources.depthStencilRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, resources.depthStencilRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebufferWidth, framebufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, resources.depthStencilRenderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        deleteSceneFramebufferResources(resources);
        throw std::runtime_error("Scene framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return resources;
}

// Deletes every OpenGL object owned by the supplied scene framebuffer group.
void deleteSceneFramebufferResources(
    SceneFramebufferResources& resources // Scene framebuffer resource group to delete and reset.
)
{
    if (resources.depthStencilRenderbuffer != 0) glDeleteRenderbuffers(1, &resources.depthStencilRenderbuffer);
    if (resources.colorTexture != 0) glDeleteTextures(1, &resources.colorTexture);
    if (resources.framebuffer != 0) glDeleteFramebuffers(1, &resources.framebuffer);

    resources = SceneFramebufferResources();
}
