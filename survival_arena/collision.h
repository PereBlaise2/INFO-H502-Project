#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

// Stores a horizontal circular collider in the XZ plane.
struct CircleCollider
{
    glm::vec2 position = glm::vec2(0.0f);
    float radius = 0.0f;
};

// Stores a ray origin and normalized direction.
struct Ray
{
    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f);
};

// Stores a sphere used for conservative broad-phase tests.
struct BoundingSphere
{
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;
};

// Stores an axis-aligned bounding box.
struct AxisAlignedBoundingBox
{
    glm::vec3 minimum = glm::vec3(0.0f);
    glm::vec3 maximum = glm::vec3(0.0f);
};

// Stores one node of the triangle-mesh bounding-volume hierarchy.
struct BVHNode
{
    AxisAlignedBoundingBox bounds;
    std::size_t firstTriangle = 0;
    std::size_t triangleCount = 0;
    int leftChild = -1;
    int rightChild = -1;

    // Returns whether the node directly owns a triangle range.
    bool isLeaf() const
    {
        return leftChild < 0 && rightChild < 0;
    }
};

// Stores the closest local triangle intersection and its barycentric coordinates.
struct RayTriangleHit
{
    float distance = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    std::size_t triangleIndex = 0;
};

// Stores the triangle data, local sphere and BVH required for exact mesh intersections.
struct TriangleMeshCollider
{
    BoundingSphere localSphere;
    std::vector<glm::vec3> triangleVertices;
    std::vector<std::size_t> triangleIndices;
    std::vector<BVHNode> bvhNodes;
};

// Stores the distance and world position of a ray-sphere intersection.
struct RaySphereHit
{
    float distance = 0.0f;
    glm::vec3 position = glm::vec3(0.0f);
};

// Creates a ray and normalizes its direction when possible.
Ray makeRay(
    const glm::vec3& origin, // Ray origin in world space.
    const glm::vec3& direction // Direction to normalize.
);

// Transforms a local bounding sphere into world space using the largest model scale.
BoundingSphere transformBoundingSphere(
    const BoundingSphere& localSphere, // Sphere defined in model space.
    const glm::mat4& model // Model matrix applied to the sphere.
);

// Tests a normalized ray against a sphere up to a maximum distance.
bool intersectRaySphere(
    const Ray& ray, // Normalized ray used by the intersection test.
    const BoundingSphere& sphere, // Sphere tested in the same coordinate space as the ray.
    float maximumDistance, // Furthest accepted distance along the ray.
    RaySphereHit& hit // Output intersection written when the test succeeds.
);

// Transforms a ray by a matrix without renormalizing its direction.
Ray transformRay(
    const Ray& ray, // Ray to transform.
    const glm::mat4& transform // Matrix applied to the origin and direction.
);

// Builds a triangle-mesh collider and its BVH from an ordered triangle-vertex list.
TriangleMeshCollider buildTriangleMeshCollider(
    std::vector<glm::vec3> triangleVertices // Triangle vertices moved into the completed collider.
);

// Finds the closest triangle hit by traversing the mesh BVH.
bool intersectRayMesh(
    const Ray& localRay, // Ray transformed into the mesh local space.
    const TriangleMeshCollider& collider, // Collider containing the triangle data and BVH.
    float maximumDistance, // Furthest accepted distance in local ray space.
    RayTriangleHit& hit // Output closest triangle intersection.
);

// Tests whether a point-centered radius intersects the infinite forward ray corridor.
bool isPointWithinRayCorridor(
    const Ray& ray, // Normalized ray defining the corridor axis.
    const glm::vec3& point, // Point tested against the corridor.
    float radius, // Accepted perpendicular distance from the ray.
    float& forwardDistance // Output signed distance along the ray direction.
);

// Resolves horizontal player movement against the arena limit and circular obstacles.
glm::vec3 resolvePlayerMovement(
    const glm::vec3& previousPosition, // Last valid player position.
    const glm::vec3& desiredPosition, // Position requested by camera movement.
    float playerRadius, // Horizontal radius of the player collider.
    float arenaHalfSize, // Half-size of the square playable arena.
    const std::vector<CircleCollider>& colliders // Static circular obstacles in the XZ plane.
);
