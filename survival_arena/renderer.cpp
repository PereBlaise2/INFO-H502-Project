#include "renderer.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../camera.h"
#include "../shader.h"
#include "../solutions/object.h"
#include "collision.h"
#include "enemy.h"
#include "game_settings.h"
#include "game_state.h"
#include "render_resources.h"

namespace
{
// Stores one vertex of the procedural resistant-enemy shield sphere.
struct ShieldVertex
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec2 texCoord = glm::vec2(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
};

// Draws every non-empty instanced vegetation batch.
void drawInstancedBatches(
    const std::vector<InstancedBatch>& batches // Batches to render.
)
{
    for (const InstancedBatch& batch : batches)
    {
        if (batch.instanceCount <= 0) continue;

        glBindVertexArray(batch.object->VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, batch.object->numVertices, batch.instanceCount);
    }

    glBindVertexArray(0);
}

// Draws a previously created procedural sphere.
void drawProceduralSphere(
    const ProceduralSphere& sphere // Sphere mesh to render.
)
{
    glBindVertexArray(sphere.VAO);
    glDrawArrays(GL_TRIANGLES, 0, sphere.vertexCount);
    glBindVertexArray(0);
}

// Builds two crossed quads between the beam endpoints.
void buildBeamVertices(
    const glm::vec3& start, // Visible beam start in world space.
    const glm::vec3& end, // Visible beam end in world space.
    const glm::vec3& cameraPosition, // Camera position used to orient the beam volume.
    float radius, // Half-thickness of the generated beam volume.
    float* vertices // Output array receiving 36 float components.
)
{
    glm::vec3 beamVector = end - start;
    float beamLength = glm::length(beamVector);

    if (beamLength < 0.0001f)
    {
        for (int index = 0; index < 36; ++index) vertices[index] = 0.0f;
        return;
    }

    glm::vec3 beamDirection = beamVector / beamLength;
    glm::vec3 beamMiddle = (start + end) * 0.5f;
    glm::vec3 directionToCamera = cameraPosition - beamMiddle;
    glm::vec3 beamRight = glm::cross(beamDirection, directionToCamera);

    // A world axis is used when the camera is almost aligned with the beam.
    if (glm::length(beamRight) < 0.0001f) beamRight = glm::cross(beamDirection, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(beamRight) < 0.0001f) beamRight = glm::cross(beamDirection, glm::vec3(1.0f, 0.0f, 0.0f));

    beamRight = glm::normalize(beamRight) * radius;
    glm::vec3 beamUp = glm::normalize(glm::cross(beamDirection, beamRight)) * radius;

    glm::vec3 quad1A = start - beamRight;
    glm::vec3 quad1B = start + beamRight;
    glm::vec3 quad1C = end + beamRight;
    glm::vec3 quad1D = end - beamRight;

    glm::vec3 quad2A = start - beamUp;
    glm::vec3 quad2B = start + beamUp;
    glm::vec3 quad2C = end + beamUp;
    glm::vec3 quad2D = end - beamUp;

    glm::vec3 beamVertices[12] = { quad1A, quad1B, quad1C, quad1A, quad1C, quad1D, quad2A, quad2B, quad2C, quad2A, quad2C, quad2D };

    for (int index = 0; index < 12; ++index)
    {
        vertices[index * 3] = beamVertices[index].x;
        vertices[index * 3 + 1] = beamVertices[index].y;
        vertices[index * 3 + 2] = beamVertices[index].z;
    }
}

// Draws one solid rectangle in normalized device coordinates.
void drawHudRectangle(
    GLuint vao, // Reusable HUD vertex array.
    GLuint vbo, // Reusable dynamic HUD vertex buffer.
    Shader& shader, // Flat-color HUD shader.
    float left, // Left coordinate in normalized device coordinates.
    float right, // Right coordinate in normalized device coordinates.
    float bottom, // Bottom coordinate in normalized device coordinates.
    float top, // Top coordinate in normalized device coordinates.
    const glm::vec3& color, // Rectangle RGB color.
    float alpha // Rectangle opacity.
)
{
    float vertices[] = { left, bottom, right, bottom, right, top, left, bottom, right, top, left, top };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    shader.setVector3f("color", color);
    shader.setFloat("alpha", alpha);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Draws one seven-segment digit using HUD rectangles.
void drawHudDigit(
    GLuint vao, // Reusable HUD vertex array.
    GLuint vbo, // Reusable dynamic HUD vertex buffer.
    Shader& shader, // Flat-color HUD shader.
    int digit, // Digit between zero and nine.
    float left, // Left coordinate of the digit.
    float bottom, // Bottom coordinate of the digit.
    float width, // Total digit width.
    float height, // Total digit height.
    float thickness, // Segment thickness.
    const glm::vec3& color, // Digit RGB color.
    float alpha // Digit opacity.
)
{
    if (digit < 0 || digit > 9) return;

    constexpr unsigned char segmentTop = 1 << 0;
    constexpr unsigned char segmentUpperRight = 1 << 1;
    constexpr unsigned char segmentLowerRight = 1 << 2;
    constexpr unsigned char segmentBottom = 1 << 3;
    constexpr unsigned char segmentLowerLeft = 1 << 4;
    constexpr unsigned char segmentUpperLeft = 1 << 5;
    constexpr unsigned char segmentMiddle = 1 << 6;

    static const unsigned char digitMasks[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

    unsigned char mask = digitMasks[digit];
    float right = left + width;
    float top = bottom + height;
    float middle = bottom + height * 0.5f;
    float horizontalLeft = left + thickness;
    float horizontalRight = right - thickness;
    float upperVerticalBottom = middle + thickness * 0.5f;
    float upperVerticalTop = top - thickness;
    float lowerVerticalBottom = bottom + thickness;
    float lowerVerticalTop = middle - thickness * 0.5f;

    if (mask & segmentTop) drawHudRectangle(vao, vbo, shader, horizontalLeft, horizontalRight, top - thickness, top, color, alpha);
    if (mask & segmentUpperRight) drawHudRectangle(vao, vbo, shader, right - thickness, right, upperVerticalBottom, upperVerticalTop, color, alpha);
    if (mask & segmentLowerRight) drawHudRectangle(vao, vbo, shader, right - thickness, right, lowerVerticalBottom, lowerVerticalTop, color, alpha);
    if (mask & segmentBottom) drawHudRectangle(vao, vbo, shader, horizontalLeft, horizontalRight, bottom, bottom + thickness, color, alpha);
    if (mask & segmentLowerLeft) drawHudRectangle(vao, vbo, shader, left, left + thickness, lowerVerticalBottom, lowerVerticalTop, color, alpha);
    if (mask & segmentUpperLeft) drawHudRectangle(vao, vbo, shader, left, left + thickness, upperVerticalBottom, upperVerticalTop, color, alpha);
    if (mask & segmentMiddle) drawHudRectangle(vao, vbo, shader, horizontalLeft, horizontalRight, middle - thickness * 0.5f, middle + thickness * 0.5f, color, alpha);
}

// Draws numeric text and colon characters with the seven-segment HUD system.
void drawHudNumericText(
    GLuint vao, // Reusable HUD vertex array.
    GLuint vbo, // Reusable dynamic HUD vertex buffer.
    Shader& shader, // Flat-color HUD shader.
    const std::string& text, // Numeric text to draw.
    float anchorX, // Horizontal anchor coordinate.
    float bottom, // Bottom coordinate of the text.
    float digitWidth, // Width of each digit.
    float digitHeight, // Height of each digit.
    float thickness, // Segment thickness.
    float spacing, // Horizontal spacing between characters.
    float alignment, // Alignment factor from zero for left to one for right.
    const glm::vec3& color, // Text RGB color.
    float alpha // Text opacity.
)
{
    const float colonWidth = digitWidth * 0.35f;
    auto getCharacterWidth = [digitWidth, colonWidth](char character) { return character == ':' ? colonWidth : digitWidth; };

    float totalWidth = 0.0f;

    for (std::size_t index = 0; index < text.size(); ++index)
    {
        totalWidth += getCharacterWidth(text[index]);
        if (index + 1 < text.size()) totalWidth += spacing;
    }

    float cursorX = anchorX - totalWidth * alignment;

    for (std::size_t index = 0; index < text.size(); ++index)
    {
        char character = text[index];
        float characterWidth = getCharacterWidth(character);

        if (character >= '0' && character <= '9')
        {
            drawHudDigit(vao, vbo, shader, character - '0', cursorX, bottom, digitWidth, digitHeight, thickness, color, alpha);
        }
        else if (character == ':')
        {
            float dotSize = thickness * 1.25f;
            float dotLeft = cursorX + (colonWidth - dotSize) * 0.5f;
            float lowerDotBottom = bottom + digitHeight * 0.30f - dotSize * 0.5f;
            float upperDotBottom = bottom + digitHeight * 0.70f - dotSize * 0.5f;

            drawHudRectangle(vao, vbo, shader, dotLeft, dotLeft + dotSize, lowerDotBottom, lowerDotBottom + dotSize, color, alpha);
            drawHudRectangle(vao, vbo, shader, dotLeft, dotLeft + dotSize, upperDotBottom, upperDotBottom + dotSize, color, alpha);
        }

        cursorX += characterWidth + spacing;
    }
}

// Draws the circular weapon cooldown indicator.
void drawReloadCircle(
    GLuint vao, // Reusable reload-circle vertex array.
    GLuint vbo, // Reusable dynamic reload-circle vertex buffer.
    Shader& shader, // Flat-color HUD shader.
    float progress, // Cooldown completion ratio between zero and one.
    int screenWidth, // Current framebuffer width in pixels.
    int screenHeight, // Current framebuffer height in pixels.
    float radiusPixels, // Outer circle radius in pixels.
    float thicknessPixels, // Circle thickness in pixels.
    const glm::vec3& color, // Circle RGB color.
    float alpha // Circle opacity.
)
{
    progress = glm::clamp(progress, 0.0f, 1.0f);
    if (progress <= 0.0f) return;

    constexpr float pi = 3.14159265358979323846f;
    float vertices[(GameSettings::Hud::reloadCircleSegments + 1) * 4];

    const float outerRadiusX = 2.0f * radiusPixels / static_cast<float>(screenWidth);
    const float outerRadiusY = 2.0f * radiusPixels / static_cast<float>(screenHeight);
    const float innerRadiusPixels = glm::max(radiusPixels - thicknessPixels, 0.0f);
    const float innerRadiusX = 2.0f * innerRadiusPixels / static_cast<float>(screenWidth);
    const float innerRadiusY = 2.0f * innerRadiusPixels / static_cast<float>(screenHeight);
    const float reachedSegmentPosition = progress * static_cast<float>(GameSettings::Hud::reloadCircleSegments);
    const int completeSegmentCount = static_cast<int>(std::floor(reachedSegmentPosition));
    const float fractionalSegment = reachedSegmentPosition - static_cast<float>(completeSegmentCount);
    const bool hasPartialSegment = completeSegmentCount < GameSettings::Hud::reloadCircleSegments && fractionalSegment > 0.000001f;

    int arcPointCount = completeSegmentCount + 1;
    if (hasPartialSegment) arcPointCount++;

    int floatIndex = 0;

    for (int pointIndex = 0; pointIndex < arcPointCount; ++pointIndex)
    {
        float segmentPosition = pointIndex > completeSegmentCount ? reachedSegmentPosition : static_cast<float>(pointIndex);
        float normalizedPosition = segmentPosition / static_cast<float>(GameSettings::Hud::reloadCircleSegments);
        float angle = pi * 0.5f - normalizedPosition * 2.0f * pi;
        float cosine = std::cos(angle);
        float sine = std::sin(angle);

        vertices[floatIndex++] = cosine * outerRadiusX;
        vertices[floatIndex++] = sine * outerRadiusY;
        vertices[floatIndex++] = cosine * innerRadiusX;
        vertices[floatIndex++] = sine * innerRadiusY;
    }

    const GLsizei vertexCount = static_cast<GLsizei>(arcPointCount * 2);

    shader.use();
    shader.setVector3f("color", color);
    shader.setFloat("alpha", alpha);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(floatIndex * sizeof(float)), vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Draws the opaque enemies, arena, vegetation, monolith and skybox into the scene framebuffer.
void renderOpaqueScene(
    const RenderContext& context, // Static rendering dependencies and transforms.
    const GameState& gameState, // Enemy containers used by the opaque enemy pass.
    const Camera& camera, // Camera position used by lighting.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective, // Current projection matrix.
    float currentTime // Current time used by animated enemy transforms.
)
{
    Shader& texturedLightingShader = *context.shaders.texturedLighting;
    Shader& instancedVegetationShader = *context.shaders.instancedVegetation;
    Shader& reflectionShader = *context.shaders.reflection;
    Shader& cubeMapShader = *context.shaders.cubeMap;

    glBindFramebuffer(GL_FRAMEBUFFER, context.sceneCapture->framebuffer);
    glClearColor(GameSettings::SceneRendering::clearColor.r, GameSettings::SceneRendering::clearColor.g, GameSettings::SceneRendering::clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    texturedLightingShader.use();
    texturedLightingShader.setMatrix4("V", view);
    texturedLightingShader.setMatrix4("P", perspective);
    texturedLightingShader.setVector3f("u_view_pos", camera.Position);
    texturedLightingShader.setFloat("textureRepeat", GameSettings::Lighting::defaultTextureRepeat);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.textures.monsterAtlas);

    for (const Enemy& enemy : gameState.enemies.normalEnemies)
    {
        glm::mat4 model = buildEnemyModelMatrix(enemy, currentTime);
        glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
        texturedLightingShader.setMatrix4("M", model);
        texturedLightingShader.setMatrix4("itM", inverseModel);
        context.objects.normalGhost->draw();
    }

    texturedLightingShader.setFloat("shininess", GameSettings::Lighting::resistantShininess);
    texturedLightingShader.setFloat("light.specular_strength", GameSettings::Lighting::resistantSpecularStrength);

    for (const Enemy& enemy : gameState.enemies.resistantEnemies)
    {
        glm::mat4 model = buildEnemyModelMatrix(enemy, currentTime);
        glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
        texturedLightingShader.setMatrix4("M", model);
        texturedLightingShader.setMatrix4("itM", inverseModel);
        context.objects.resistantGhost->draw();
    }

    texturedLightingShader.setFloat("shininess", GameSettings::Lighting::texturedShininess);
    texturedLightingShader.setFloat("light.specular_strength", GameSettings::Lighting::texturedSpecularStrength);

    texturedLightingShader.use();
    texturedLightingShader.setMatrix4("M", context.transforms.floorModel);
    texturedLightingShader.setMatrix4("itM", context.transforms.floorInverseModel);
    texturedLightingShader.setMatrix4("V", view);
    texturedLightingShader.setMatrix4("P", perspective);
    texturedLightingShader.setVector3f("u_view_pos", camera.Position);
    texturedLightingShader.setFloat("textureRepeat", GameSettings::Lighting::floorTextureRepeat);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.textures.ground);
    context.objects.arenaCube->draw();

    texturedLightingShader.use();
    texturedLightingShader.setMatrix4("V", view);
    texturedLightingShader.setMatrix4("P", perspective);
    texturedLightingShader.setVector3f("u_view_pos", camera.Position);
    texturedLightingShader.setFloat("textureRepeat", GameSettings::Lighting::wallTextureRepeat);
    texturedLightingShader.setFloat("light.specular_strength", GameSettings::Lighting::wallSpecularStrength);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.textures.forestWall);

    texturedLightingShader.setMatrix4("M", context.transforms.leftWallModel);
    texturedLightingShader.setMatrix4("itM", context.transforms.leftWallInverseModel);
    context.objects.arenaCube->draw();

    texturedLightingShader.setMatrix4("M", context.transforms.rightWallModel);
    texturedLightingShader.setMatrix4("itM", context.transforms.rightWallInverseModel);
    context.objects.arenaCube->draw();

    texturedLightingShader.setMatrix4("M", context.transforms.backWallModel);
    texturedLightingShader.setMatrix4("itM", context.transforms.backWallInverseModel);
    context.objects.arenaCube->draw();

    texturedLightingShader.setMatrix4("M", context.transforms.frontWallModel);
    texturedLightingShader.setMatrix4("itM", context.transforms.frontWallInverseModel);
    context.objects.arenaCube->draw();

    texturedLightingShader.setFloat("light.specular_strength", GameSettings::Lighting::texturedSpecularStrength);

    instancedVegetationShader.use();
    instancedVegetationShader.setMatrix4("V", view);
    instancedVegetationShader.setMatrix4("P", perspective);
    instancedVegetationShader.setVector3f("u_view_pos", camera.Position);
    instancedVegetationShader.setFloat("textureRepeat", GameSettings::Lighting::defaultTextureRepeat);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.textures.forest);

    drawInstancedBatches(*context.batches.forestTrees);
    drawInstancedBatches(*context.batches.forestRocks);
    drawInstancedBatches(*context.batches.forestBushes);
    drawInstancedBatches(*context.batches.grass);

    reflectionShader.use();
    reflectionShader.setMatrix4("M", context.transforms.monolithModel);
    reflectionShader.setMatrix4("itM", context.transforms.monolithInverseModel);
    reflectionShader.setMatrix4("V", view);
    reflectionShader.setMatrix4("P", perspective);
    reflectionShader.setVector3f("u_view_pos", camera.Position);

    reflectionShader.setInteger("useBumpMapping", gameState.advancedRendering.useMonolithBumpMapping ? 1 : 0);
    reflectionShader.setFloat("bumpScale", GameSettings::SceneRendering::monolithBumpScale);
    reflectionShader.setFloat("bumpStrength", GameSettings::SceneRendering::monolithBumpStrength);
    reflectionShader.setFloat("bumpSampleStep", GameSettings::SceneRendering::monolithBumpSampleStep);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, context.textures.cubeMap);
    context.objects.reflectiveMonolith->draw();

    cubeMapShader.use();
    cubeMapShader.setMatrix4("V", view);
    cubeMapShader.setMatrix4("P", perspective);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, context.textures.cubeMap);

    glDepthFunc(GL_LEQUAL);
    context.objects.skyboxCube->draw();
    glDepthFunc(GL_LESS);
}

// Copies the opaque scene color and depth to the default framebuffer.
void blitSceneCapture(
    const RenderContext& context // Scene framebuffer and output dimensions.
)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, context.framebufferWidth, context.framebufferHeight, 0, 0, context.framebufferWidth, context.framebufferHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

// Draws active resistant-enemy shields with additive blending.
void renderResistantShields(
    const RenderContext& context, // Shield shader, mesh, texture and resistant collider.
    const GameState& gameState, // Resistant enemies and their shield states.
    const Camera& camera, // Camera position used by the shield rim effect.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective, // Current projection matrix.
    float currentTime // Current time used by shield rotation and texture animation.
)
{
    Shader& energyShieldShader = *context.shaders.energyShield;

    energyShieldShader.use();
    energyShieldShader.setMatrix4("V", view);
    energyShieldShader.setMatrix4("P", perspective);
    energyShieldShader.setVector3f("u_view_pos", camera.Position);
    energyShieldShader.setFloat("time", currentTime);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.textures.shield);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (const Enemy& enemy : gameState.enemies.resistantEnemies)
    {
        if (!enemy.shieldActive) continue;

        BoundingSphere worldShieldSphere = buildEnemyShieldSphere(enemy, *context.resistantGhostCollider, currentTime);
        glm::mat4 shieldModel = glm::mat4(1.0f);
        shieldModel = glm::translate(shieldModel, worldShieldSphere.center);
        shieldModel = glm::rotate(shieldModel, currentTime * GameSettings::SceneRendering::shieldRotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
        shieldModel = glm::scale(shieldModel, glm::vec3(worldShieldSphere.radius));
        glm::mat4 shieldInverseModel = glm::transpose(glm::inverse(shieldModel));

        energyShieldShader.setMatrix4("M", shieldModel);
        energyShieldShader.setMatrix4("itM", shieldInverseModel);
        drawProceduralSphere(*context.shieldSphere);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draws spectral enemies by sampling the captured opaque scene texture.
void renderSpectralEnemies(
    const RenderContext& context, // Refraction shader, captured scene texture and spectral mesh.
    const GameState& gameState, // Spectral enemy container.
    const Camera& camera, // Camera position used by the distance fade.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective, // Current projection matrix.
    float currentTime // Current time used by animated enemy transforms.
)
{
    Shader& refractionShader = *context.shaders.refraction;

    refractionShader.use();
    refractionShader.setMatrix4("V", view);
    refractionShader.setMatrix4("P", perspective);
    refractionShader.setVector3f("u_view_pos", camera.Position);
    refractionShader.setFloat("distortionStrength", GameSettings::SceneRendering::spectralDistortionStrength);
    refractionShader.setFloat("tintStrength", GameSettings::SceneRendering::spectralTintStrength);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, context.sceneCapture->colorTexture);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const Enemy& enemy : gameState.enemies.translucentEnemies)
    {
        glm::mat4 model = buildEnemyModelMatrix(enemy, currentTime);
        glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
        refractionShader.setMatrix4("M", model);
        refractionShader.setMatrix4("itM", inverseModel);
        context.objects.translucentGhost->draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Converts one simulated particle into the shared GPU vertex format.
void appendDeathParticleVertex(
    const DeathParticle& particle, // Simulated particle to convert.
    std::vector<DeathParticleVertex>& particleVertices // Shared upload array receiving the converted vertex.
)
{
    float lifetimeProgress = glm::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
    float particleAlpha = 1.0f;
    float sizeProgress = lifetimeProgress;
    float particleShape = 0.0f;

    switch (particle.type)
    {
        case DeathParticleType::Flash:
            particleAlpha = std::pow(1.0f - lifetimeProgress, 3.0f);
            particleShape = 0.0f;
            break;

        case DeathParticleType::Spark:
            particleAlpha = (1.0f - lifetimeProgress) * (1.0f - lifetimeProgress);
            particleShape = 1.0f;
            break;

        case DeathParticleType::Shockwave:
            particleAlpha = std::pow(1.0f - lifetimeProgress, 1.6f) * GameSettings::DeathParticles::shockwaveMaximumAlpha;
            sizeProgress = 1.0f - std::pow(1.0f - lifetimeProgress, 3.0f);
            particleShape = 2.0f;
            break;

        case DeathParticleType::Smoke:
        {
            float fadeIn = glm::smoothstep(0.0f, GameSettings::DeathParticles::smokeFadeInEnd, lifetimeProgress);
            float fadeOut = 1.0f - lifetimeProgress;
            particleAlpha = fadeIn * fadeOut * fadeOut * GameSettings::DeathParticles::smokeAlpha;
            particleShape = 3.0f;
            break;
        }
    }

    float particleSize = glm::mix(particle.startSize, particle.endSize, sizeProgress);

    DeathParticleVertex vertex;
    vertex.positionX = particle.position.x;
    vertex.positionY = particle.position.y;
    vertex.positionZ = particle.position.z;
    vertex.colorR = particle.color.r;
    vertex.colorG = particle.color.g;
    vertex.colorB = particle.color.b;
    vertex.alpha = particleAlpha;
    vertex.size = particleSize;
    vertex.shape = particleShape;
    vertex.velocityX = particle.velocity.x;
    vertex.velocityY = particle.velocity.y;
    vertex.velocityZ = particle.velocity.z;
    particleVertices.push_back(vertex);
}

// Uploads all death particles once and renders smoke before additive particles.
void renderDeathParticles(
    const RenderContext& context, // Particle shader and shared VAO/VBO.
    GameState& gameState, // Simulated particles and reusable upload vector.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective // Current projection matrix.
)
{
    std::vector<DeathParticleVertex>& particleVertices = gameState.deathParticles.vertices;
    const std::vector<DeathParticle>& particles = gameState.deathParticles.particles;
    particleVertices.clear();

    for (const DeathParticle& particle : particles)
    {
        if (particle.type == DeathParticleType::Smoke) appendDeathParticleVertex(particle, particleVertices);
    }

    GLsizei smokeParticleCount = static_cast<GLsizei>(particleVertices.size());

    for (const DeathParticle& particle : particles)
    {
        if (particle.type != DeathParticleType::Smoke) appendDeathParticleVertex(particle, particleVertices);
    }

    GLsizei additiveParticleCount = static_cast<GLsizei>(particleVertices.size()) - smokeParticleCount;
    if (particleVertices.empty()) return;

    const bool useGeometryShader = gameState.advancedRendering.useParticleGeometryShader;
    Shader& deathParticleShader = useGeometryShader ? *context.shaders.deathParticleGeometry : *context.shaders.deathParticle;
    DeathParticleRenderResources& resources = *context.deathParticleResources;

    deathParticleShader.use();
    deathParticleShader.setMatrix4("V", view);
    deathParticleShader.setMatrix4("P", perspective);

    if (useGeometryShader) {
        glUniform2f(glGetUniformLocation(deathParticleShader.ID, "framebufferSize"), static_cast<float>(context.framebufferWidth), static_cast<float>(context.framebufferHeight));
        deathParticleShader.setFloat("sparkHalfWidthMultiplier", GameSettings::DeathParticles::geometrySparkHalfWidthMultiplier);
        deathParticleShader.setFloat("sparkHalfLengthMultiplier", GameSettings::DeathParticles::geometrySparkHalfLengthMultiplier);
    }

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBindVertexArray(resources.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particleVertices.size() * sizeof(DeathParticleVertex), particleVertices.data());

    if (smokeParticleCount > 0)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_POINTS, 0, smokeParticleCount);
    }

    if (additiveParticleCount > 0)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDrawArrays(GL_POINTS, smokeParticleCount, additiveParticleCount);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draws the three-phase energy beam with one glow pass and one core pass.
void renderEnergyBeam(
    const RenderContext& context, // Energy-beam shader and dynamic geometry.
    const GameState& gameState, // Current beam endpoints and visual timer.
    const Camera& camera, // Camera position used to orient the crossed quads.
    const glm::mat4& view, // Current view matrix and camera-to-world transform source.
    const glm::mat4& perspective // Current projection matrix.
)
{
    if (gameState.weapon.visualTimer <= 0.0f) return;

    glm::mat4 currentCameraToWorld = glm::inverse(view);
    glm::vec3 currentMuzzlePosition = glm::vec3(currentCameraToWorld * glm::vec4(GameSettings::Weapon::muzzleLocalOffset, 1.0f));
    float shotAge = GameSettings::Weapon::shotVisualDuration - gameState.weapon.visualTimer;
    glm::vec3 visibleStart = currentMuzzlePosition;
    glm::vec3 visibleEnd = gameState.weapon.beamEnd;
    float glowIntensity = 1.0f;
    float coreIntensity = 1.0f;

    if (shotAge < GameSettings::Weapon::beamGrowthDuration)
    {
        float growthProgress = glm::clamp(shotAge / GameSettings::Weapon::beamGrowthDuration, 0.0f, 1.0f);
        growthProgress = 1.0f - std::pow(1.0f - growthProgress, 3.0f);
        visibleEnd = glm::mix(currentMuzzlePosition, gameState.weapon.beamEnd, growthProgress);
        glowIntensity = growthProgress;
        coreIntensity = growthProgress;
    }
    else if (shotAge < GameSettings::Weapon::beamGrowthDuration + GameSettings::Weapon::beamHoldDuration)
    {
        visibleStart = currentMuzzlePosition;
        visibleEnd = gameState.weapon.beamEnd;
    }
    else
    {
        float fadeAge = shotAge - GameSettings::Weapon::beamGrowthDuration - GameSettings::Weapon::beamHoldDuration;
        float fadeProgress = glm::clamp(fadeAge / GameSettings::Weapon::beamFadeDuration, 0.0f, 1.0f);
        float tailProgress = fadeProgress * fadeProgress;
        visibleStart = glm::mix(currentMuzzlePosition, gameState.weapon.beamEnd, tailProgress);
        visibleEnd = gameState.weapon.beamEnd;
        glowIntensity = 1.0f - fadeProgress;
        coreIntensity = 1.0f - GameSettings::Weapon::beamCoreFadeReduction * fadeProgress;
    }

    if (glm::length(visibleEnd - visibleStart) <= 0.001f) return;

    Shader& energyBeamShader = *context.shaders.energyBeam;
    EnergyBeamResources& resources = *context.energyBeamResources;

    energyBeamShader.use();
    energyBeamShader.setMatrix4("V", view);
    energyBeamShader.setMatrix4("P", perspective);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glBindVertexArray(resources.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.VBO);

    float glowVertices[36];
    buildBeamVertices(visibleStart, visibleEnd, camera.Position, GameSettings::Weapon::beamGlowRadius, glowVertices);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glowVertices), glowVertices);
    energyBeamShader.setVector3f("beamColor", GameSettings::Weapon::beamGlowColor);
    energyBeamShader.setFloat("alpha", GameSettings::Weapon::beamGlowAlpha * glowIntensity);
    glDrawArrays(GL_TRIANGLES, 0, 12);

    float coreVertices[36];
    buildBeamVertices(visibleStart, visibleEnd, camera.Position, GameSettings::Weapon::beamCoreRadius, coreVertices);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(coreVertices), coreVertices);
    energyBeamShader.setVector3f("beamColor", GameSettings::Weapon::beamCoreColor);
    energyBeamShader.setFloat("alpha", GameSettings::Weapon::beamCoreAlpha * coreIntensity);
    glDrawArrays(GL_TRIANGLES, 0, 12);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Draws the first-person weapon after clearing the copied scene depth buffer.
void renderWeapon(
    const RenderContext& context, // Textured shader and weapon object.
    const GameState& gameState, // Current recoil amount.
    const Camera& camera, // Camera position used by lighting.
    const glm::mat4& view, // Current view matrix used to attach the weapon to the camera.
    const glm::mat4& perspective // Current projection matrix.
)
{
    glm::mat4 weaponLocalModel = glm::mat4(1.0f);
    float recoilBackwardDistance = gameState.weapon.recoilAmount * GameSettings::Weapon::recoilBackwardDistance;
    float recoilDownDistance = gameState.weapon.recoilAmount * GameSettings::Weapon::recoilDownDistance;
    glm::vec3 weaponPosition = GameSettings::Weapon::basePosition;
    weaponPosition.z += recoilBackwardDistance;
    weaponPosition.y -= recoilDownDistance;

    weaponLocalModel = glm::translate(weaponLocalModel, weaponPosition);
    weaponLocalModel = glm::rotate(weaponLocalModel, glm::radians(GameSettings::Weapon::yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    weaponLocalModel = glm::rotate(weaponLocalModel, glm::radians(GameSettings::Weapon::pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    weaponLocalModel = glm::rotate(weaponLocalModel, glm::radians(GameSettings::Weapon::rollDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    weaponLocalModel = glm::rotate(weaponLocalModel, glm::radians(GameSettings::Weapon::recoilPitchDegrees) * gameState.weapon.recoilAmount, glm::vec3(1.0f, 0.0f, 0.0f));
    weaponLocalModel = glm::scale(weaponLocalModel, glm::vec3(GameSettings::Weapon::modelScale));

    glm::mat4 energyBlasterModel = glm::inverse(view) * weaponLocalModel;
    glm::mat4 energyBlasterInverseModel = glm::transpose(glm::inverse(energyBlasterModel));
    Shader& weaponShader = gameState.advancedRendering.useWeaponToonShading ? *context.shaders.weaponToon : *context.shaders.texturedLighting;

    glClear(GL_DEPTH_BUFFER_BIT);

    weaponShader.use();
    weaponShader.setMatrix4("M", energyBlasterModel);
    weaponShader.setMatrix4("itM", energyBlasterInverseModel);
    weaponShader.setMatrix4("V", view);
    weaponShader.setMatrix4("P", perspective);
    weaponShader.setVector3f("u_view_pos", camera.Position);
    weaponShader.setFloat("textureRepeat", GameSettings::Lighting::defaultTextureRepeat);
    context.objects.energyBlaster->drawWithMaterials();
}

// Draws the temporary full-screen damage vignette.
void renderDamageVignette(
    const RenderContext& context, // Vignette shader and fullscreen-triangle VAO.
    const GameState& gameState // Current damage-vignette intensity.
)
{
    if (gameState.player.damageVignetteIntensity <= 0.0f) return;

    Shader& damageVignetteShader = *context.shaders.damageVignette;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    damageVignetteShader.use();
    damageVignetteShader.setVector3f("damageColor", GameSettings::Player::damageVignetteColor);
    damageVignetteShader.setFloat("intensity", gameState.player.damageVignetteIntensity);
    damageVignetteShader.setFloat("edgeWidth", GameSettings::Player::damageVignetteEdgeWidth);

    glBindVertexArray(context.hudResources->damageVignetteVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draws the normal in-game HUD.
void renderNormalHud(
    const RenderContext& context, // HUD shader, geometry and framebuffer dimensions.
    const GameState& gameState, // Health, score and weapon cooldown state.
    const std::string& scoreText, // Preformatted score shared with the Game Over HUD.
    const std::string& timeText // Preformatted survival time shared with the Game Over HUD.
)
{
    Shader& hudColorShader = *context.shaders.hudColor;
    HudResources& resources = *context.hudResources;

    float healthRatio = glm::clamp(static_cast<float>(gameState.player.health) / static_cast<float>(GameSettings::Player::maximumHealth), 0.0f, 1.0f);
    float innerLeft = GameSettings::Hud::healthBarLeft + GameSettings::Hud::healthBarBorder;
    float innerRight = GameSettings::Hud::healthBarRight - GameSettings::Hud::healthBarBorder;
    float innerBottom = GameSettings::Hud::healthBarBottom + GameSettings::Hud::healthBarBorder;
    float innerTop = GameSettings::Hud::healthBarTop - GameSettings::Hud::healthBarBorder;
    float filledRight = innerLeft + (innerRight - innerLeft) * healthRatio;

    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, GameSettings::Hud::healthBarLeft, GameSettings::Hud::healthBarRight, GameSettings::Hud::healthBarBottom, GameSettings::Hud::healthBarTop, GameSettings::Hud::healthBarBorderColor, 1.0f);
    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, innerLeft, innerRight, innerBottom, innerTop, GameSettings::Hud::healthBarBackgroundColor, 1.0f);

    if (healthRatio > 0.0f)
    {
        glm::vec3 healthColor = glm::mix(GameSettings::Hud::lowHealthColor, GameSettings::Hud::highHealthColor, healthRatio);
        drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, innerLeft, filledRight, innerBottom, innerTop, healthColor, 1.0f);
    }

    drawHudNumericText(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, scoreText, GameSettings::Hud::scoreAnchorX, GameSettings::Hud::scoreBottom, GameSettings::Hud::scoreDigitWidth, GameSettings::Hud::scoreDigitHeight, GameSettings::Hud::scoreThickness, GameSettings::Hud::scoreSpacing, GameSettings::Hud::scoreAlignment, GameSettings::Hud::scoreColor, 1.0f);
    drawHudNumericText(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, timeText, GameSettings::Hud::timeAnchorX, GameSettings::Hud::timeBottom, GameSettings::Hud::timeDigitWidth, GameSettings::Hud::timeDigitHeight, GameSettings::Hud::timeThickness, GameSettings::Hud::timeSpacing, GameSettings::Hud::timeAlignment, GameSettings::Hud::timeColor, 1.0f);

    if (gameState.weapon.cooldownTimer <= 0.0f)
    {
        hudColorShader.setVector3f("color", GameSettings::Hud::crosshairColor);
        hudColorShader.setFloat("alpha", 1.0f);
        glBindVertexArray(resources.crosshairVAO);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        glBindVertexArray(0);
    }
    else
    {
        float reloadProgress = glm::clamp(1.0f - gameState.weapon.cooldownTimer / GameSettings::Weapon::shotCooldown, 0.0f, 1.0f);
        drawReloadCircle(resources.reloadCircleVAO, resources.reloadCircleVBO, hudColorShader, reloadProgress, context.framebufferWidth, context.framebufferHeight, GameSettings::Hud::reloadCircleRadiusPixels, GameSettings::Hud::reloadCircleThicknessPixels, GameSettings::Hud::reloadCircleColor, 1.0f);
    }
}

// Draws the dark overlay, pulsing frame, final score and final survival time.
void renderGameOverHud(
    const RenderContext& context, // HUD shader and reusable rectangle geometry.
    const GameState& gameState, // Game Over timer used by the border pulse.
    const std::string& scoreText, // Preformatted final score.
    const std::string& timeText // Preformatted final survival time.
)
{
    Shader& hudColorShader = *context.shaders.hudColor;
    HudResources& resources = *context.hudResources;
    float gameOverPulse = GameSettings::Hud::gameOverPulseBase + GameSettings::Hud::gameOverPulseAmplitude * std::sin(gameState.gameOverElapsedTime * GameSettings::Hud::gameOverPulseSpeed);

    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, -1.0f, 1.0f, -1.0f, 1.0f, GameSettings::Hud::gameOverOverlayColor, GameSettings::Hud::gameOverOverlayAlpha);
    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, -1.0f, 1.0f, 1.0f - GameSettings::Hud::gameOverBorderSize, 1.0f, GameSettings::Hud::gameOverBorderColor, gameOverPulse);
    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, -1.0f, 1.0f, -1.0f, -1.0f + GameSettings::Hud::gameOverBorderSize, GameSettings::Hud::gameOverBorderColor, gameOverPulse);
    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, -1.0f, -1.0f + GameSettings::Hud::gameOverBorderSize, -1.0f + GameSettings::Hud::gameOverBorderSize, 1.0f - GameSettings::Hud::gameOverBorderSize, GameSettings::Hud::gameOverBorderColor, gameOverPulse);
    drawHudRectangle(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, 1.0f - GameSettings::Hud::gameOverBorderSize, 1.0f, -1.0f + GameSettings::Hud::gameOverBorderSize, 1.0f - GameSettings::Hud::gameOverBorderSize, GameSettings::Hud::gameOverBorderColor, gameOverPulse);
    drawHudNumericText(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, scoreText, GameSettings::Hud::finalScoreAnchorX, GameSettings::Hud::finalScoreBottom, GameSettings::Hud::finalScoreDigitWidth, GameSettings::Hud::finalScoreDigitHeight, GameSettings::Hud::finalScoreThickness, GameSettings::Hud::finalScoreSpacing, GameSettings::Hud::finalScoreAlignment, GameSettings::Hud::finalScoreColor, 1.0f);
    drawHudNumericText(resources.rectangleVAO, resources.rectangleVBO, hudColorShader, timeText, GameSettings::Hud::finalTimeAnchorX, GameSettings::Hud::finalTimeBottom, GameSettings::Hud::finalTimeDigitWidth, GameSettings::Hud::finalTimeDigitHeight, GameSettings::Hud::finalTimeThickness, GameSettings::Hud::finalTimeSpacing, GameSettings::Hud::finalTimeAlignment, GameSettings::Hud::finalTimeColor, 1.0f);
}

// Formats and draws either the normal HUD or the Game Over HUD.
void renderHud(
    const RenderContext& context, // HUD shader, geometry and framebuffer dimensions.
    const GameState& gameState // Player, weapon and Game Over values displayed by the HUD.
)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    context.shaders.hudColor->use();

    std::string scoreText = std::to_string(gameState.player.score);
    int displayedTimeSeconds = static_cast<int>(gameState.player.survivalTime);
    int displayedMinutes = displayedTimeSeconds / 60;
    int displayedSeconds = displayedTimeSeconds % 60;
    std::string timeText = std::to_string(displayedMinutes) + ":";
    if (displayedSeconds < 10) timeText += "0";
    timeText += std::to_string(displayedSeconds);

    if (gameState.isGameOver) renderGameOverHud(context, gameState, scoreText, timeText);
    else renderNormalHud(context, gameState, scoreText, timeText);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
}

// Creates one instance buffer for each model used by a vegetation group.
std::vector<InstancedBatch> createInstancedBatches(
    const std::vector<Object*>& models, // Models that can receive generated instances.
    const std::vector<VegetationInstance>& instances // Generated transforms and model indices.
)
{
    std::vector<std::vector<glm::mat4>> matricesPerModel(models.size());

    for (const VegetationInstance& instance : instances)
    {
        if (instance.modelIndex < 0 || instance.modelIndex >= static_cast<int>(models.size()))
        {
            std::cerr << "Skipping vegetation instance with invalid model index: " << instance.modelIndex << std::endl;
            continue;
        }

        matricesPerModel[instance.modelIndex].push_back(instance.model);
    }

    std::vector<InstancedBatch> batches;
    batches.reserve(models.size());

    for (std::size_t modelIndex = 0; modelIndex < models.size(); ++modelIndex)
    {
        InstancedBatch batch;
        batch.object = models[modelIndex];
        batch.instanceCount = static_cast<GLsizei>(matricesPerModel[modelIndex].size());

        if (batch.instanceCount > 0)
        {
            glGenBuffers(1, &batch.instanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, matricesPerModel[modelIndex].size() * sizeof(glm::mat4), matricesPerModel[modelIndex].data(), GL_STATIC_DRAW);
            glBindVertexArray(batch.object->VAO);

            for (int column = 0; column < 4; ++column)
            {
                glEnableVertexAttribArray(3 + column);
                glVertexAttribPointer(3 + column, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(sizeof(glm::vec4) * column));
                glVertexAttribDivisor(3 + column, 1);
            }
        }

        batches.push_back(batch);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return batches;
}

// Deletes the instance buffers owned by the supplied batches.
void deleteInstancedBatches(
    std::vector<InstancedBatch>& batches // Batches whose instance buffers are deleted.
)
{
    for (InstancedBatch& batch : batches)
    {
        if (batch.instanceVBO == 0) continue;
        glDeleteBuffers(1, &batch.instanceVBO);
        batch.instanceVBO = 0;
    }
}

// Creates the procedural sphere mesh used by resistant enemy shields.
ProceduralSphere createProceduralSphere(
    int sectorCount, // Number of horizontal subdivisions.
    int stackCount // Number of vertical subdivisions.
)
{
    ProceduralSphere sphere;
    std::vector<ShieldVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(sectorCount * stackCount * 6));

    constexpr float pi = 3.14159265f;
    auto buildVertex = [](float phi, float theta, float u, float v)
    {
        ShieldVertex vertex;
        float cosPhi = std::cos(phi);
        vertex.position.x = cosPhi * std::cos(theta);
        vertex.position.y = std::sin(phi);
        vertex.position.z = cosPhi * std::sin(theta);
        vertex.texCoord = glm::vec2(u, v);
        vertex.normal = glm::normalize(vertex.position);
        return vertex;
    };

    for (int stack = 0; stack < stackCount; ++stack)
    {
        float stackFraction0 = static_cast<float>(stack) / static_cast<float>(stackCount);
        float stackFraction1 = static_cast<float>(stack + 1) / static_cast<float>(stackCount);
        float phi0 = pi * 0.5f - stackFraction0 * pi;
        float phi1 = pi * 0.5f - stackFraction1 * pi;

        for (int sector = 0; sector < sectorCount; ++sector)
        {
            float sectorFraction0 = static_cast<float>(sector) / static_cast<float>(sectorCount);
            float sectorFraction1 = static_cast<float>(sector + 1) / static_cast<float>(sectorCount);
            float theta0 = sectorFraction0 * 2.0f * pi;
            float theta1 = sectorFraction1 * 2.0f * pi;
            ShieldVertex topLeft = buildVertex(phi0, theta0, sectorFraction0, 1.0f - stackFraction0);
            ShieldVertex topRight = buildVertex(phi0, theta1, sectorFraction1, 1.0f - stackFraction0);
            ShieldVertex bottomLeft = buildVertex(phi1, theta0, sectorFraction0, 1.0f - stackFraction1);
            ShieldVertex bottomRight = buildVertex(phi1, theta1, sectorFraction1, 1.0f - stackFraction1);

            vertices.push_back(topLeft);
            vertices.push_back(bottomRight);
            vertices.push_back(bottomLeft);
            vertices.push_back(topLeft);
            vertices.push_back(topRight);
            vertices.push_back(bottomRight);
        }
    }

    sphere.vertexCount = static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1, &sphere.VAO);
    glGenBuffers(1, &sphere.VBO);
    glBindVertexArray(sphere.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ShieldVertex), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ShieldVertex), reinterpret_cast<void*>(offsetof(ShieldVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ShieldVertex), reinterpret_cast<void*>(offsetof(ShieldVertex, texCoord)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ShieldVertex), reinterpret_cast<void*>(offsetof(ShieldVertex, normal)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return sphere;
}

// Deletes the OpenGL resources owned by a procedural sphere.
void deleteProceduralSphere(
    ProceduralSphere& sphere // Sphere whose resources are deleted and reset.
)
{
    if (sphere.VBO != 0) glDeleteBuffers(1, &sphere.VBO);
    if (sphere.VAO != 0) glDeleteVertexArrays(1, &sphere.VAO);
    sphere.VAO = 0;
    sphere.VBO = 0;
    sphere.vertexCount = 0;
}

// Creates the dynamic geometry used by the energy-beam glow and core passes.
EnergyBeamResources createEnergyBeamResources()
{
    EnergyBeamResources resources;
    glGenVertexArrays(1, &resources.VAO);
    glGenBuffers(1, &resources.VBO);
    glBindVertexArray(resources.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.VBO);
    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return resources;
}

// Deletes the OpenGL resources owned by the energy beam.
void deleteEnergyBeamResources(
    EnergyBeamResources& resources // Energy-beam resource group to delete and reset.
)
{
    if (resources.VBO != 0) glDeleteBuffers(1, &resources.VBO);
    if (resources.VAO != 0) glDeleteVertexArrays(1, &resources.VAO);
    resources.VAO = 0;
    resources.VBO = 0;
}

// Creates the single dynamic buffer shared by all enemy-death particles.
DeathParticleRenderResources createDeathParticleRenderResources()
{
    DeathParticleRenderResources resources;
    glGenVertexArrays(1, &resources.VAO);
    glGenBuffers(1, &resources.VBO);
    glBindVertexArray(resources.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.VBO);
    glBufferData(GL_ARRAY_BUFFER, GameSettings::DeathParticles::maximumCount * sizeof(DeathParticleVertex), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(7 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(DeathParticleVertex), reinterpret_cast<void*>(9 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glEnable(GL_PROGRAM_POINT_SIZE);
    return resources;
}

// Deletes the OpenGL resources owned by the enemy-death particle renderer.
void deleteDeathParticleRenderResources(
    DeathParticleRenderResources& resources // Particle resource group to delete and reset.
)
{
    if (resources.VBO != 0) glDeleteBuffers(1, &resources.VBO);
    if (resources.VAO != 0) glDeleteVertexArrays(1, &resources.VAO);
    resources.VAO = 0;
    resources.VBO = 0;
}

// Renders one complete frame in the required opaque, transparent, weapon and HUD order.
void renderFrame(
    const RenderContext& context, // Long-lived scene, shader, texture and OpenGL resource references.
    GameState& gameState, // Mutable game state whose temporary particle vertices are rebuilt for upload.
    const Camera& camera, // Camera position used by lighting and billboard-like effects.
    const glm::mat4& view, // Current view matrix.
    const glm::mat4& perspective, // Current projection matrix.
    double currentTime // Absolute application time used by animated visual effects.
)
{
    float visualTime = static_cast<float>(currentTime);
    renderOpaqueScene(context, gameState, camera, view, perspective, visualTime);
    blitSceneCapture(context);
    renderResistantShields(context, gameState, camera, view, perspective, visualTime);
    renderSpectralEnemies(context, gameState, camera, view, perspective, visualTime);
    renderDeathParticles(context, gameState, view, perspective);
    renderEnergyBeam(context, gameState, camera, view, perspective);
    renderWeapon(context, gameState, camera, view, perspective);
    renderDamageVignette(context, gameState);
    renderHud(context, gameState);
}
