#include "shader_sources.h"

namespace ShaderSources
{
    // Transforms refractive geometry and provides world-space position and normals.
    const std::string refractionVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 tex_coord;
        layout(location = 2) in vec3 normal;

        out vec3 v_frag_coord;
        out vec3 v_normal;
        out vec2 v_tex_coord;

        uniform mat4 M;
        uniform mat4 itM;
        uniform mat4 V;
        uniform mat4 P;

        void main()
        {
            vec4 frag_coord = M * vec4(position, 1.0);

            gl_Position = P * V * frag_coord;

            v_normal = normalize(mat3(itM) * normal);
            v_frag_coord = frag_coord.xyz;
            v_tex_coord = tex_coord;
        }
        )GLSL";

    // Samples the opaque scene texture to render fading spectral refraction.
    const std::string refractionFragment = R"GLSL(#version 400 core
        out vec4 FragColor;
        precision mediump float; 
        in vec3 v_frag_coord; 
        in vec3 v_normal; 
        uniform vec3 u_view_pos; 
        uniform mat4 V;
        uniform sampler2D sceneTexture;
        uniform float distortionStrength;
        uniform vec3 tintColour;
        uniform float tintStrength;
        uniform float fadeNearDistance;
        uniform float fadeFarDistance;
        void main() {
            vec2 screenSize = vec2(textureSize(sceneTexture, 0));
            vec2 screenUV = gl_FragCoord.xy / screenSize;
            vec3 N = normalize(v_normal);
            vec3 viewNormal = normalize(mat3(V) * N);
            float cameraDistance = length(u_view_pos - v_frag_coord);
            float visibility = 1.0 - smoothstep(fadeNearDistance, fadeFarDistance, cameraDistance);
            float currentDistortion = distortionStrength * visibility;
            float currentTint = tintStrength * visibility;
            vec2 distortion = viewNormal.xy * currentDistortion;
            vec2 refractedUV = clamp(screenUV + distortion, vec2(0.001), vec2(0.999));
            vec3 sceneColour = texture(sceneTexture, refractedUV).rgb;
            vec3 finalColour = mix(sceneColour, tintColour, currentTint);
            float alpha = mix(0.1, 0.9, visibility);
            FragColor = vec4(finalColour, alpha);
        }
        )GLSL";

    // Renders the skybox without applying camera translation.
    const std::string cubeMapVertex = R"GLSL(#version 330 core
        in vec3 position; 
        in vec2 tex_coords; 
        in vec3 normal; 
        uniform mat4 V; 
        uniform mat4 P; 
        out vec3 texCoord_v; 
        void main(){ 
        texCoord_v = position;
        mat4 V_no_translation = mat4(mat3(V)); 
        vec4 pos = P * V_no_translation * vec4(position, 1.0); 
        gl_Position = pos.xyww;

        }
        )GLSL";

    // Samples the skybox cubemap.
    const std::string cubeMapFragment = R"GLSL(#version 330 core
        out vec4 FragColor;
        precision mediump float; 
        uniform samplerCube cubemapSampler; 
        in vec3 texCoord_v; 
        void main() { 
        FragColor = texture(cubemapSampler,texCoord_v); 
        } 
        )GLSL";

    // Transforms textured geometry and prepares lighting inputs.
    const std::string texturedLightingVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 tex_coord;
        layout(location = 2) in vec3 normal;

        out vec3 v_frag_coord;
        out vec3 v_normal;
        out vec2 v_tex_coord;
        uniform mat4 M;
        uniform mat4 itM;
        uniform mat4 V;
        uniform mat4 P;
        uniform float textureRepeat;
        void main()
        {
            vec4 frag_coord = M * vec4(position, 1.0);
            gl_Position = P * V * frag_coord;
            v_normal = normalize(mat3(itM) * normal);
            v_frag_coord = frag_coord.xyz;
            v_tex_coord = tex_coord * textureRepeat;
        }
        )GLSL";

    // Applies directional lighting, texturing and distance fog.
    const std::string texturedLightingFragment = R"GLSL(#version 330 core
        out vec4 FragColor;
        in vec3 v_frag_coord;
        in vec3 v_normal;
        in vec2 v_tex_coord;
        uniform vec3 u_view_pos;
        uniform sampler2D diffuseTexture;
        struct Light
        {
            vec3 direction;
            vec3 color;
            float ambient_strength;
            float diffuse_strength;
            float specular_strength;
        };
        uniform Light light;
        uniform float shininess;
        struct Fog
        {
            vec3 color;
            float density;
        };
        uniform Fog fog;
        float specularCalculation(vec3 N, vec3 L, vec3 V)
        {
            vec3 R = reflect(-L, N);
            float cosTheta = dot(R, V);
            float spec = pow(max(cosTheta, 0.0), shininess);
            return light.specular_strength * spec;
        }
        void main()
        {
            vec3 N = normalize(v_normal);
            vec3 L = normalize(light.direction);
            vec3 V = normalize(u_view_pos - v_frag_coord);
            float diffuse = light.diffuse_strength * max(dot(N, L), 0.0);
            float specular = specularCalculation(N, L, V);
            float illumination = light.ambient_strength + diffuse + specular;
            vec3 textureColour = texture(diffuseTexture, v_tex_coord).rgb;
            vec3 litColour = textureColour * light.color * illumination;
            float fogDistance = length(u_view_pos - v_frag_coord);
            float fogFactor = exp(-pow(fog.density * fogDistance, 2.0));
            fogFactor = clamp(fogFactor, 0.0, 1.0);
            vec3 finalColour = mix(fog.color, litColour, fogFactor);
            FragColor = vec4(finalColour, 1.0);
        }
        )GLSL";

    // Applies discrete diffuse lighting bands and a sharp specular highlight to the FPS weapon.
    const std::string weaponToonFragment = R"GLSL(#version 330 core
        out vec4 FragColor;

        in vec3 v_frag_coord;
        in vec3 v_normal;
        in vec2 v_tex_coord;

        uniform vec3 u_view_pos;
        uniform sampler2D diffuseTexture;

        struct Light
        {
            vec3 direction;
            vec3 color;
            float ambient_strength;
            float diffuse_strength;
            float specular_strength;
        };

        uniform Light light;

        uniform float shininess;
        uniform float toonLevels;
        uniform float toonSpecularThreshold;

        struct Fog
        {
            vec3 color;
            float density;
        };

        uniform Fog fog;

        void main()
        {
            vec3 N = normalize(v_normal);
            vec3 L = normalize(light.direction);
            vec3 V = normalize(u_view_pos - v_frag_coord);

            float rawDiffuse = max(dot(N, L), 0.0);

            float levelCount = max(toonLevels, 2.0);
            float toonDiffuse = floor(rawDiffuse * levelCount) / (levelCount - 1.0);
            toonDiffuse = clamp(toonDiffuse, 0.0, 1.0);

            vec3 R = reflect(-L, N);
            float rawSpecular = pow(max(dot(R, V), 0.0), shininess);
            float toonSpecular = rawSpecular >= toonSpecularThreshold ? light.specular_strength : 0.0;

            float illumination = light.ambient_strength + light.diffuse_strength * toonDiffuse + toonSpecular;

            vec3 textureColour = texture(diffuseTexture, v_tex_coord).rgb;
            vec3 litColour = textureColour * light.color * illumination;

            float fogDistance = length(u_view_pos - v_frag_coord);
            float fogFactor = exp(-pow(fog.density * fogDistance, 2.0));
            fogFactor = clamp(fogFactor, 0.0, 1.0);

            vec3 finalColour = mix(fog.color, litColour, fogFactor);

            FragColor = vec4(finalColour, 1.0);
        }
        )GLSL";

    // Transforms vegetation with one model matrix per instance.
    const std::string instancedVegetationVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 tex_coord;
        layout(location = 2) in vec3 normal;
        layout(location = 3) in mat4 instanceModel;
        out vec3 v_frag_coord;
        out vec3 v_normal;
        out vec2 v_tex_coord;
        uniform mat4 V;
        uniform mat4 P;
        uniform float textureRepeat;
        void main()
        {
            vec4 frag_coord = instanceModel * vec4(position, 1.0);
            gl_Position = P * V * frag_coord;
            v_normal = normalize(mat3(instanceModel) * normal);
            v_frag_coord = frag_coord.xyz;
            v_tex_coord = tex_coord * textureRepeat;
        }
        )GLSL";

    // Combines the monolith base color with cubemap reflection and optional procedural bump mapping.
    const std::string reflectionFragment = R"GLSL(#version 400 core
        out vec4 FragColor;

        in vec3 v_frag_coord;
        in vec3 v_normal;
        in vec2 v_tex_coord;

        uniform vec3 u_view_pos;
        uniform samplerCube cubemapSampler;

        uniform vec3 baseColour;
        uniform float reflectionStrength;

        uniform bool useBumpMapping;
        uniform float bumpScale;
        uniform float bumpStrength;
        uniform float bumpSampleStep;

        // Generates a deterministic pseudo-random value for one 3D grid point.
        float hash3D(vec3 p)
        {
            p = fract(p * 0.1031);
            p += dot(p, p.yzx + 33.33);
            return fract((p.x + p.y) * p.z);
        }

        // Generates smooth 3D value noise.
        float valueNoise(vec3 p)
        {
            vec3 cell = floor(p);
            vec3 local = fract(p);

            vec3 smoothLocal = local * local * (3.0 - 2.0 * local);

            float n000 = hash3D(cell + vec3(0.0, 0.0, 0.0));
            float n100 = hash3D(cell + vec3(1.0, 0.0, 0.0));
            float n010 = hash3D(cell + vec3(0.0, 1.0, 0.0));
            float n110 = hash3D(cell + vec3(1.0, 1.0, 0.0));
            float n001 = hash3D(cell + vec3(0.0, 0.0, 1.0));
            float n101 = hash3D(cell + vec3(1.0, 0.0, 1.0));
            float n011 = hash3D(cell + vec3(0.0, 1.0, 1.0));
            float n111 = hash3D(cell + vec3(1.0, 1.0, 1.0));

            float nx00 = mix(n000, n100, smoothLocal.x);
            float nx10 = mix(n010, n110, smoothLocal.x);
            float nx01 = mix(n001, n101, smoothLocal.x);
            float nx11 = mix(n011, n111, smoothLocal.x);

            float nxy0 = mix(nx00, nx10, smoothLocal.y);
            float nxy1 = mix(nx01, nx11, smoothLocal.y);

            return mix(nxy0, nxy1, smoothLocal.z);
        }

        // Combines several noise frequencies to create irregular rocky detail.
        float getBumpHeight(vec3 position)
        {
            vec3 p = position * bumpScale;

            float height = 0.0;

            height += valueNoise(p) * 0.55;
            height += valueNoise(p * 2.07 + vec3(7.3, 1.7, 4.1)) * 0.30;
            height += valueNoise(p * 4.13 + vec3(2.8, 9.2, 5.6)) * 0.15;

            return height;
        }

        // Perturbs the geometric normal using the procedural 3D height field.
        vec3 getBumpedNormal(vec3 surfaceNormal)
        {
            vec3 N = normalize(surfaceNormal);

            vec3 referenceAxis = abs(N.y) < 0.999
                ? vec3(0.0, 1.0, 0.0)
                : vec3(1.0, 0.0, 0.0);

            vec3 tangent = normalize(cross(referenceAxis, N));
            vec3 bitangent = normalize(cross(N, tangent));

            float stepSize = max(bumpSampleStep, 0.0001);

            float heightLeft =
                getBumpHeight(v_frag_coord - tangent * stepSize);

            float heightRight =
                getBumpHeight(v_frag_coord + tangent * stepSize);

            float heightDown =
                getBumpHeight(v_frag_coord - bitangent * stepSize);

            float heightUp =
                getBumpHeight(v_frag_coord + bitangent * stepSize);

            float tangentSlope =
                (heightRight - heightLeft) / (2.0 * stepSize);

            float bitangentSlope =
                (heightUp - heightDown) / (2.0 * stepSize);

            vec3 bumpedNormal =
                N
                - tangent * tangentSlope * bumpStrength
                - bitangent * bitangentSlope * bumpStrength;

            return normalize(bumpedNormal);
        }

        void main()
        {
            vec3 N = normalize(v_normal);

            if (useBumpMapping) {
                N = getBumpedNormal(N);
            }

            vec3 V = normalize(u_view_pos - v_frag_coord);
            vec3 R = reflect(-V, N);

            vec3 reflectedColour = texture(cubemapSampler, R).rgb;
            vec3 finalColour = mix(baseColour, reflectedColour, reflectionStrength);

            FragColor = vec4(finalColour, 1.0);
        }
        )GLSL";

    // Renders two-dimensional HUD vertices in normalized device coordinates.
    const std::string hudColorVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec2 position;
        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
        )GLSL";

    // Applies a uniform color and opacity to HUD geometry.
    const std::string hudColorFragment = R"GLSL(#version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        uniform float alpha;
        void main()
        {
            FragColor = vec4(color, alpha);
        }
        )GLSL";

    // Transforms the energy beam volume into clip space.
    const std::string energyBeamVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        uniform mat4 V;
        uniform mat4 P;
        void main()
        {
            gl_Position = P * V * vec4(position, 1.0);
        }
        )GLSL";

    // Applies the selected beam color and opacity.
    const std::string energyBeamFragment = R"GLSL(#version 330 core
        out vec4 FragColor;
        uniform vec3 beamColor;
        uniform float alpha;
        void main()
        {
            FragColor = vec4(beamColor, alpha);
        }
        )GLSL";

    // Transforms the resistant-enemy shield and prepares its animated surface data.
    const std::string energyShieldVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 tex_coord;
        layout(location = 2) in vec3 normal;
        out vec2 v_tex_coord;
        out vec3 v_world_position;
        out vec3 v_world_normal;
        uniform mat4 M;
        uniform mat4 itM;
        uniform mat4 V;
        uniform mat4 P;
        void main()
        {
            vec4 worldPosition = M * vec4(position, 1.0);
            gl_Position = P * V * worldPosition;
            v_tex_coord = tex_coord;
            v_world_position = worldPosition.xyz;
            v_world_normal = normalize(mat3(itM) * normal);
        }
        )GLSL";

    // Renders the animated energy texture, rim effect and shield pulse.
    const std::string energyShieldFragment = R"GLSL(#version 330 core
        out vec4 FragColor;
        in vec2 v_tex_coord;
        in vec3 v_world_position;
        in vec3 v_world_normal;
        uniform sampler2D shieldTexture;
        uniform vec3 u_view_pos;
        uniform vec3 tintColour;
        uniform float time;
        uniform float baseAlpha;
        void main()
        {
            vec2 firstUV = v_tex_coord * 1.8 + vec2(time * 0.035, time * 0.060);
            vec2 secondUV = v_tex_coord * 2.7 + vec2(-time * 0.055, time * 0.025);
            vec3 firstSample = texture(shieldTexture, firstUV).rgb;
            vec3 secondSample = texture(shieldTexture, secondUV).rgb;
            vec3 textureColour = mix(firstSample, secondSample, 0.45);
            float firstBrightness = dot(firstSample, vec3(0.299, 0.587, 0.114));
            float secondBrightness = dot(secondSample, vec3(0.299, 0.587, 0.114));
            float energyPattern = firstBrightness * 0.65 + secondBrightness * 0.35;
            vec3 N = normalize(v_world_normal);
            vec3 V = normalize(u_view_pos - v_world_position);
            float rim = 1.0 - max(dot(N, V), 0.0);
            rim = pow(rim, 2.2);
            float pulse = 0.92 + 0.08 * sin(time * 4.0);
            vec3 finalColour = mix(textureColour, tintColour, 0.45);
            finalColour *= 0.70 + energyPattern * 0.65 + rim * 1.25;
            float alpha = baseAlpha * (0.18 + energyPattern * 0.60 + rim * 0.95);
            alpha = clamp(alpha * pulse, 0.0, 0.85);
            FragColor = vec4(finalColour, alpha);
        }
        )GLSL";

            // Generates a full-screen triangle from the vertex identifier.
            const std::string damageVignetteVertex = R"GLSL(#version 330 core
        out vec2 v_uv;
        void main()
        {
            vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
            vec2 position = positions[gl_VertexID];
            v_uv = position * 0.5 + 0.5;
            gl_Position = vec4(position, 0.0, 1.0);
        }
        )GLSL";

    // Renders the fading red damage effect around the screen edges.
    const std::string damageVignetteFragment = R"GLSL(#version 330 core
        in vec2 v_uv;
        out vec4 FragColor;
        uniform vec3 damageColor;
        uniform float intensity;
        uniform float edgeWidth;
        void main()
        {
            float horizontalEdgeDistance = min(v_uv.x, 1.0 - v_uv.x);
            float verticalEdgeDistance = min(v_uv.y, 1.0 - v_uv.y);
            float edgeDistance = min(horizontalEdgeDistance, verticalEdgeDistance);
            float vignette = 1.0 - smoothstep(0.0, edgeWidth, edgeDistance);
            vignette = pow(vignette, 1.35);
            float fade = intensity * intensity;
            FragColor = vec4(damageColor, vignette * fade * 0.72);
        }
        )GLSL";

    // Transforms point particles and forwards their appearance attributes.
    const std::string deathParticleVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 particleColor;
        layout(location = 2) in float particleAlpha;
        layout(location = 3) in float particleSize;
        layout(location = 4) in float particleShape;
        out vec3 v_color;
        out float v_alpha;
        flat out float v_shape;
        uniform mat4 V;
        uniform mat4 P;
        void main()
        {
            gl_Position = P * V * vec4(position, 1.0);
            gl_PointSize = particleSize;
            v_color = particleColor;
            v_alpha = particleAlpha;
            v_shape = particleShape;
        }
        )GLSL";

    // Renders soft discs, rings and smoke from point coordinates.
    const std::string deathParticleFragment = R"GLSL(#version 330 core
        in vec3 v_color;
        in float v_alpha;
        flat in float v_shape;
        out vec4 FragColor;
        void main()
        {
            vec2 centeredCoordinate = gl_PointCoord * 2.0 - 1.0;
            float distanceFromCenter = length(centeredCoordinate);
            if (distanceFromCenter > 1.0)
                discard;
            float particleMask = 0.0;
            if (v_shape > 2.5)
            {
                // Very soft disc used for smoke.
                particleMask = pow(max(1.0 - distanceFromCenter, 0.0), 1.35);
            }
            else if (v_shape > 1.5)
            {
                // Expanding circular ring used for the shockwave.
                float ringDistance = abs(distanceFromCenter - 0.72);
                particleMask = 1.0 - smoothstep(0.045, 0.15, ringDistance);
            }
            else
            {
                // Soft disc used by sparks and the flash in the original renderer.
                particleMask = 1.0 - smoothstep(0.20, 1.0, distanceFromCenter);
            }
            FragColor = vec4(v_color, v_alpha * particleMask);
        }
        )GLSL";


    // Transforms one particle center and forwards its appearance attributes to the geometry shader.
    const std::string deathParticleGeometryVertex = R"GLSL(#version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 particleColor;
        layout(location = 2) in float particleAlpha;
        layout(location = 3) in float particleSize;
        layout(location = 4) in float particleShape;
        layout(location = 5) in vec3 particleVelocity;


        out ParticleData
        {
            vec3 color;
            float alpha;
            float size;
            float shape;
            vec2 screenDirection;
        } particleData;

        uniform mat4 V;
        uniform mat4 P;

        void main()
        {
            gl_Position = P * V * vec4(position, 1.0);

            vec3 viewVelocity = mat3(V) * particleVelocity;

            particleData.color = particleColor;
            particleData.alpha = particleAlpha;
            particleData.size = particleSize;
            particleData.shape = particleShape;
            particleData.screenDirection = vec2(viewVelocity.x * P[0][0], viewVelocity.y * P[1][1]);
        }
        )GLSL";

    // Expands one point into a four-vertex screen-aligned quad.
    const std::string deathParticleGeometry = R"GLSL(#version 330 core
        layout(points) in;
        layout(triangle_strip, max_vertices = 4) out;

        uniform vec2 framebufferSize;
        uniform float sparkHalfWidthMultiplier;
        uniform float sparkHalfLengthMultiplier;

        in ParticleData
        {
            vec3 color;
            float alpha;
            float size;
            float shape;
            vec2 screenDirection;
        } particleData[];

        out vec3 v_color;
        out float v_alpha;
        out vec2 v_uv;
        flat out float v_shape;

        const vec2 quadCorners[4] = vec2[4](
            vec2(-1.0, -1.0),
            vec2( 1.0, -1.0),
            vec2(-1.0,  1.0),
            vec2( 1.0,  1.0)
        );

        const vec2 quadTextureCoordinates[4] = vec2[4](
            vec2(0.0, 0.0),
            vec2(1.0, 0.0),
            vec2(0.0, 1.0),
            vec2(1.0, 1.0)
        );

        void main()
        {
            vec4 centerPosition = gl_in[0].gl_Position;
            vec2 safeFramebufferSize = max(framebufferSize, vec2(1.0));

            bool isSpark = particleData[0].shape > 0.5 && particleData[0].shape < 1.5;

            vec2 direction = particleData[0].screenDirection;

            if (length(direction) < 0.0001) {
                direction = vec2(0.0, 1.0);
            }
            else {
                direction = normalize(direction);
            }

            vec2 perpendicular = vec2(-direction.y, direction.x);

            float halfWidthPixels = particleData[0].size * 0.5;
            float halfLengthPixels = particleData[0].size * 0.5;

            if (isSpark) {
                halfWidthPixels = particleData[0].size * sparkHalfWidthMultiplier;
                halfLengthPixels = particleData[0].size * sparkHalfLengthMultiplier;
            }

            for (int cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
            {
                vec2 corner = quadCorners[cornerIndex];

                vec2 pixelOffset =
                    perpendicular * corner.x * halfWidthPixels +
                    direction * corner.y * halfLengthPixels;

                vec2 ndcOffset = vec2(
                    2.0 * pixelOffset.x / safeFramebufferSize.x,
                    2.0 * pixelOffset.y / safeFramebufferSize.y
                );

                gl_Position = centerPosition + vec4(ndcOffset * centerPosition.w, 0.0, 0.0);

                v_color = particleData[0].color;
                v_alpha = particleData[0].alpha;
                v_shape = particleData[0].shape;
                v_uv = quadTextureCoordinates[cornerIndex];

                EmitVertex();
            }

            EndPrimitive();
        }
        )GLSL";

    // Renders the same particle shapes as the original point-sprite fragment shader.
    const std::string deathParticleGeometryFragment = R"GLSL(#version 330 core
        in vec3 v_color;
        in float v_alpha;
        in vec2 v_uv;
        flat in float v_shape;

        out vec4 FragColor;

        void main()
        {
            vec2 centeredCoordinate = v_uv * 2.0 - 1.0;
            float distanceFromCenter = length(centeredCoordinate);

            if (distanceFromCenter > 1.0) {
                discard;
            }

            float particleMask = 0.0;

            if (v_shape > 2.5)
            {
                // Very soft disc used for smoke.
                particleMask = pow(max(1.0 - distanceFromCenter, 0.0), 1.35);
            }
            else if (v_shape > 1.5)
            {
                // Expanding circular ring used for the shockwave.
                float ringDistance = abs(distanceFromCenter - 0.72);
                particleMask = 1.0 - smoothstep(0.045, 0.15, ringDistance);
            }
            else
            {
                // Soft shape used for flash and stretched sparks.
                particleMask = 1.0 - smoothstep(0.20, 1.0, distanceFromCenter);
            }

            FragColor = vec4(v_color, v_alpha * particleMask);
        }
        )GLSL";

}
