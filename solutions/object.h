#pragma once

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// GLAD must be included before GLFW because GLFW otherwise includes the system OpenGL header.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../shader.h"

// Stores one interleaved vertex loaded from an OBJ file.
struct Vertex
{
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec2 Texture = glm::vec2(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f);
};

// Stores one material name, its source texture path and its owned OpenGL texture.
struct ObjectMaterial
{
    std::string name;
    std::string diffuseTexturePath;
    GLuint diffuseTexture = 0;
};

// Identifies the contiguous vertex range rendered with one material.
struct MaterialRange
{
    std::string materialName;
    GLint firstVertex = 0;
    GLsizei vertexCount = 0;
};

// Loads one OBJ mesh and owns the OpenGL resources created for that mesh and its materials.
class Object
{
public:
    // Loads the OBJ geometry and its referenced material descriptions into CPU memory.
    explicit Object(
        const char* path // Path of the OBJ file to load.
    )
    {
        std::string objectPath = path;
        std::size_t lastSlash = objectPath.find_last_of("/\\");
        std::string objectDirectory = lastSlash == std::string::npos ? "" : objectPath.substr(0, lastSlash + 1);

        std::string materialLibrary;
        std::string currentMaterial;
        std::ifstream inputFile(path);

        if (!inputFile)
        {
            std::cerr << "Failed to load OBJ file: " << path << std::endl;
            return;
        }

        std::string line;

        while (std::getline(inputFile, line))
        {
            std::istringstream lineStream(line);
            std::string command;
            lineStream >> command;

            if (command == "v")
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                lineStream >> x >> y >> z;
                positions.push_back(glm::vec3(x, y, z));
            }
            else if (command == "vn")
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                lineStream >> x >> y >> z;
                normals.push_back(glm::vec3(x, y, z));
            }
            else if (command == "vt")
            {
                float u = 0.0f;
                float v = 0.0f;
                lineStream >> u >> v;
                textures.push_back(glm::vec2(u, v));
            }
            else if (command == "mtllib")
            {
                lineStream >> materialLibrary;
            }
            else if (command == "usemtl")
            {
                lineStream >> currentMaterial;

                MaterialRange range;
                range.materialName = currentMaterial;
                range.firstVertex = static_cast<GLint>(vertices.size());
                materialRanges.push_back(range);
            }
            else if (command == "f")
            {
                std::vector<std::string> faceTokens;
                std::string faceToken;

                while (lineStream >> faceToken) faceTokens.push_back(faceToken);

                if (faceTokens.size() < 3) continue;

                Vertex firstVertex = parseObjectVertex(faceTokens[0], positions, textures, normals);

                // Faces with more than three vertices are expanded as a triangle fan.
                for (std::size_t tokenIndex = 1; tokenIndex + 1 < faceTokens.size(); ++tokenIndex)
                {
                    vertices.push_back(firstVertex);
                    vertices.push_back(parseObjectVertex(faceTokens[tokenIndex], positions, textures, normals));
                    vertices.push_back(parseObjectVertex(faceTokens[tokenIndex + 1], positions, textures, normals));

                    if (!materialRanges.empty()) materialRanges.back().vertexCount += 3;
                }
            }
        }

        std::cout << "Load model with " << vertices.size() << " vertices" << std::endl;

        if (!materialLibrary.empty()) loadMaterialLibrary(objectDirectory + materialLibrary);

        numVertices = static_cast<GLsizei>(vertices.size());
    }

    // Releases the VAO, VBO and material textures while an OpenGL context is still active.
    ~Object()
    {
        releaseOpenGLResources();
    }

    Object(
        const Object& other // Copying would duplicate ownership of the same OpenGL handles.
    ) = delete;

    Object& operator=(
        const Object& other // Copying would duplicate ownership of the same OpenGL handles.
    ) = delete;

    Object(
        Object&& other // Moving would invalidate pointers stored by instanced batches and render contexts.
    ) = delete;

    Object& operator=(
        Object&& other // Moving would invalidate pointers stored by instanced batches and render contexts.
    ) = delete;

    // Creates the VAO and VBO used to render the loaded CPU mesh.
    void makeObject(
        const Shader& shader, // Shader whose attribute locations configure the VAO.
        bool useTextureCoordinates = true // Whether the texture-coordinate attribute is enabled.
    )
    {
        releaseMeshResources();

        std::vector<float> data(static_cast<std::size_t>(numVertices) * 8);

        for (GLsizei vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
        {
            const Vertex& vertex = vertices.at(static_cast<std::size_t>(vertexIndex));
            std::size_t firstComponent = static_cast<std::size_t>(vertexIndex) * 8;

            data[firstComponent] = vertex.Position.x;
            data[firstComponent + 1] = vertex.Position.y;
            data[firstComponent + 2] = vertex.Position.z;
            data[firstComponent + 3] = vertex.Texture.x;
            data[firstComponent + 4] = vertex.Texture.y;
            data[firstComponent + 5] = vertex.Normal.x;
            data[firstComponent + 6] = vertex.Normal.y;
            data[firstComponent + 7] = vertex.Normal.z;
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);

        GLint positionAttribute = glGetAttribLocation(shader.ID, "position");

        if (positionAttribute >= 0)
        {
            glEnableVertexAttribArray(static_cast<GLuint>(positionAttribute));
            glVertexAttribPointer(static_cast<GLuint>(positionAttribute), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        }

        if (useTextureCoordinates)
        {
            GLint textureAttribute = glGetAttribLocation(shader.ID, "tex_coord");

            if (textureAttribute >= 0)
            {
                glEnableVertexAttribArray(static_cast<GLuint>(textureAttribute));
                glVertexAttribPointer(static_cast<GLuint>(textureAttribute), 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
            }
        }

        GLint normalAttribute = glGetAttribLocation(shader.ID, "normal");

        if (normalAttribute >= 0)
        {
            glEnableVertexAttribArray(static_cast<GLuint>(normalAttribute));
            glVertexAttribPointer(static_cast<GLuint>(normalAttribute), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Returns the OBJ triangles in local space for collision-mesh construction.
    std::vector<glm::vec3> getTrianglePositions() const
    {
        std::vector<glm::vec3> trianglePositions;
        trianglePositions.reserve(vertices.size());

        for (const Vertex& vertex : vertices) trianglePositions.push_back(vertex.Position);

        return trianglePositions;
    }

    // Draws every triangle of the object with the currently active shader and texture bindings.
    void draw() const
    {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
    }

    // Draws each material range with the diffuse texture stored by its material.
    void drawWithMaterials() const
    {
        if (materialRanges.empty())
        {
            draw();
            return;
        }

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE1);

        for (const MaterialRange& range : materialRanges)
        {
            GLuint textureID = 0;

            for (const ObjectMaterial& material : materials)
            {
                if (material.name != range.materialName) continue;

                textureID = material.diffuseTexture;
                break;
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glDrawArrays(GL_TRIANGLES, range.firstVertex, range.vertexCount);
        }

        glBindVertexArray(0);
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> textures;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> vertices;
    std::vector<ObjectMaterial> materials;
    std::vector<MaterialRange> materialRanges;

    GLsizei numVertices = 0;
    GLuint VBO = 0;
    GLuint VAO = 0;

    glm::mat4 model = glm::mat4(1.0f);

private:
    // Converts one OBJ face token into a fully expanded vertex.
    static Vertex parseObjectVertex(
        const std::string& faceToken, // OBJ token containing position, texture and normal indices.
        const std::vector<glm::vec3>& sourcePositions, // Position array referenced by the token.
        const std::vector<glm::vec2>& sourceTextures, // Texture-coordinate array referenced by the token.
        const std::vector<glm::vec3>& sourceNormals // Normal array referenced by the token.
    )
    {
        Vertex vertex;
        std::string remainingToken = faceToken;

        std::string positionIndex = remainingToken.substr(0, remainingToken.find('/'));
        remainingToken.erase(0, remainingToken.find('/') + 1);

        std::string textureIndex = remainingToken.substr(0, remainingToken.find('/'));
        remainingToken.erase(0, remainingToken.find('/') + 1);

        std::string normalIndex = remainingToken.substr(0, remainingToken.find('/'));

        vertex.Position = sourcePositions.at(std::stoul(positionIndex) - 1);
        vertex.Texture = sourceTextures.at(std::stoul(textureIndex) - 1);
        vertex.Normal = sourceNormals.at(std::stoul(normalIndex) - 1);

        return vertex;
    }

    // Loads the material names and diffuse texture paths referenced by one MTL file.
    void loadMaterialLibrary(
        const std::string& path // Path of the MTL file associated with the OBJ mesh.
    )
    {
        std::ifstream inputFile(path);

        if (!inputFile)
        {
            std::cerr << "Failed to load material library: " << path << std::endl;
            return;
        }

        std::size_t lastSlash = path.find_last_of("/\\");
        std::string materialDirectory = lastSlash == std::string::npos ? "" : path.substr(0, lastSlash + 1);
        ObjectMaterial* currentMaterial = nullptr;
        std::string line;

        while (std::getline(inputFile, line))
        {
            std::istringstream lineStream(line);
            std::string command;
            lineStream >> command;

            if (command == "newmtl")
            {
                ObjectMaterial material;
                lineStream >> material.name;
                materials.push_back(material);
                currentMaterial = &materials.back();
            }
            else if (command == "map_Kd" && currentMaterial != nullptr)
            {
                std::string texturePath;
                lineStream >> texturePath;
                currentMaterial->diffuseTexturePath = materialDirectory + texturePath;
            }
        }
    }

    // Deletes the VAO and VBO if they have already been created.
    void releaseMeshResources()
    {
        if (glfwGetCurrentContext() == nullptr)
        {
            VBO = 0;
            VAO = 0;
            return;
        }

        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);

        VBO = 0;
        VAO = 0;
    }

    // Deletes every diffuse texture assigned to the object's materials.
    void releaseMaterialTextures()
    {
        if (glfwGetCurrentContext() == nullptr)
        {
            for (ObjectMaterial& material : materials) material.diffuseTexture = 0;
            return;
        }

        for (ObjectMaterial& material : materials)
        {
            if (material.diffuseTexture == 0) continue;

            glDeleteTextures(1, &material.diffuseTexture);
            material.diffuseTexture = 0;
        }
    }

    // Deletes every OpenGL resource owned by this object.
    void releaseOpenGLResources()
    {
        releaseMeshResources();
        releaseMaterialTextures();
    }
};
