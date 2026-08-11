#include "collision.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t maximumTrianglesPerBvhLeaf = 8;
constexpr float directionEpsilon = 0.000001f;
constexpr float triangleEpsilon = 0.00001f;
constexpr float arenaCollisionMargin = 0.5f;

// Stores one pending BVH node and the ray distance at which its box is entered.
struct BVHStackEntry
{
    int nodeIndex = -1;
    float entryDistance = 0.0f;
};

// Builds a conservative sphere around a point collection.
BoundingSphere buildBoundingSphere(
    const std::vector<glm::vec3>& points // Points enclosed by the resulting sphere.
)
{
    BoundingSphere sphere;

    if (points.empty())
        return sphere;

    glm::vec3 minimumPoint(std::numeric_limits<float>::max());
    glm::vec3 maximumPoint(std::numeric_limits<float>::lowest());

    for (const glm::vec3& point : points)
    {
        minimumPoint = glm::min(minimumPoint, point);
        maximumPoint = glm::max(maximumPoint, point);
    }

    sphere.center = (minimumPoint + maximumPoint) * 0.5f;

    float maximumDistanceSquared = 0.0f;

    for (const glm::vec3& point : points)
    {
        const glm::vec3 difference = point - sphere.center;
        const float distanceSquared = glm::dot(difference, difference);
        maximumDistanceSquared = std::max(maximumDistanceSquared, distanceSquared);
    }

    sphere.radius = std::sqrt(maximumDistanceSquared);
    return sphere;
}

// Tests a ray against an axis-aligned box using the slab method.
bool intersectRayAABB(
    const Ray& ray, // Ray expressed in the same coordinate space as the box.
    const AxisAlignedBoundingBox& box, // Axis-aligned box to test.
    float maximumDistance, // Furthest accepted distance along the ray.
    float& entryDistance // Output distance at which the ray enters the box.
)
{
    float minimumRayDistance = 0.0f;
    float maximumRayDistance = maximumDistance;

    for (int axis = 0; axis < 3; ++axis)
    {
        const float origin = ray.origin[axis];
        const float direction = ray.direction[axis];

        if (std::abs(direction) <= directionEpsilon)
        {
            if (origin < box.minimum[axis] || origin > box.maximum[axis])
                return false;

            continue;
        }

        const float inverseDirection = 1.0f / direction;
        float firstDistance = (box.minimum[axis] - origin) * inverseDirection;
        float secondDistance = (box.maximum[axis] - origin) * inverseDirection;

        if (firstDistance > secondDistance)
            std::swap(firstDistance, secondDistance);

        minimumRayDistance = std::max(minimumRayDistance, firstDistance);
        maximumRayDistance = std::min(maximumRayDistance, secondDistance);

        if (minimumRayDistance > maximumRayDistance)
            return false;
    }

    entryDistance = minimumRayDistance;
    return true;
}

// Tests one horizontal player position against every circular obstacle.
bool checkPlayerCollision(
    const glm::vec3& playerPosition, // Player position tested in world space.
    float playerRadius, // Horizontal player collision radius.
    const std::vector<CircleCollider>& colliders // Static circular obstacles in the XZ plane.
)
{
    for (const CircleCollider& collider : colliders)
    {
        const float differenceX = playerPosition.x - collider.position.x;
        const float differenceZ = playerPosition.z - collider.position.y;
        const float minimumDistance = playerRadius + collider.radius;
        const float distanceSquared = differenceX * differenceX + differenceZ * differenceZ;

        if (distanceSquared < minimumDistance * minimumDistance)
            return true;
    }

    return false;
}

// Creates an empty box that can be expanded with points.
AxisAlignedBoundingBox makeEmptyAABB()
{
    AxisAlignedBoundingBox box;
    box.minimum = glm::vec3(std::numeric_limits<float>::max());
    box.maximum = glm::vec3(std::numeric_limits<float>::lowest());
    return box;
}

// Expands an axis-aligned box to contain one point.
void expandAABB(
    AxisAlignedBoundingBox& box, // Box modified by the expansion.
    const glm::vec3& point // Point that must be contained by the box.
)
{
    box.minimum = glm::min(box.minimum, point);
    box.maximum = glm::max(box.maximum, point);
}

// Computes the centroid of one triangle from the collider's ordered vertex array.
glm::vec3 getTriangleCentroid(
    const TriangleMeshCollider& collider, // Collider containing the triangle vertices.
    std::size_t triangleIndex // Triangle whose centroid is requested.
)
{
    const std::size_t firstVertex = triangleIndex * 3;
    const glm::vec3& p0 = collider.triangleVertices[firstVertex];
    const glm::vec3& p1 = collider.triangleVertices[firstVertex + 1];
    const glm::vec3& p2 = collider.triangleVertices[firstVertex + 2];
    return (p0 + p1 + p2) / 3.0f;
}

// Computes the bounds of an ordered triangle range.
AxisAlignedBoundingBox computeTriangleRangeBounds(
    const TriangleMeshCollider& collider, // Collider containing triangle vertices and ordered indices.
    std::size_t firstTriangle, // First position in the ordered triangle-index array.
    std::size_t triangleCount // Number of ordered triangles included in the range.
)
{
    AxisAlignedBoundingBox bounds = makeEmptyAABB();

    for (std::size_t offset = 0; offset < triangleCount; ++offset)
    {
        const std::size_t orderedIndex = firstTriangle + offset;
        const std::size_t triangleIndex = collider.triangleIndices[orderedIndex];
        const std::size_t firstVertex = triangleIndex * 3;
        expandAABB(bounds, collider.triangleVertices[firstVertex]);
        expandAABB(bounds, collider.triangleVertices[firstVertex + 1]);
        expandAABB(bounds, collider.triangleVertices[firstVertex + 2]);
    }

    return bounds;
}

// Computes the centroid bounds of an ordered triangle range.
AxisAlignedBoundingBox computeCentroidRangeBounds(
    const TriangleMeshCollider& collider, // Collider containing triangle vertices and ordered indices.
    std::size_t firstTriangle, // First position in the ordered triangle-index array.
    std::size_t triangleCount // Number of ordered triangles included in the range.
)
{
    AxisAlignedBoundingBox bounds = makeEmptyAABB();

    for (std::size_t offset = 0; offset < triangleCount; ++offset)
    {
        const std::size_t orderedIndex = firstTriangle + offset;
        const std::size_t triangleIndex = collider.triangleIndices[orderedIndex];
        expandAABB(bounds, getTriangleCentroid(collider, triangleIndex));
    }

    return bounds;
}

// Returns the axis with the largest extent.
int findLargestExtentAxis(
    const glm::vec3& extent // Three-dimensional extent to compare.
)
{
    if (extent.x >= extent.y && extent.x >= extent.z)
        return 0;

    if (extent.y >= extent.z)
        return 1;

    return 2;
}

// Recursively builds one BVH node and returns its index.
int buildBVHNode(
    TriangleMeshCollider& collider, // Collider whose ordered triangle indices and nodes are modified.
    std::size_t firstTriangle, // First ordered triangle owned by the node.
    std::size_t triangleCount // Number of ordered triangles owned by the node.
)
{
    const int nodeIndex = static_cast<int>(collider.bvhNodes.size());
    collider.bvhNodes.push_back(BVHNode());
    collider.bvhNodes[nodeIndex].bounds = computeTriangleRangeBounds(collider, firstTriangle, triangleCount);
    collider.bvhNodes[nodeIndex].firstTriangle = firstTriangle;
    collider.bvhNodes[nodeIndex].triangleCount = triangleCount;

    if (triangleCount <= maximumTrianglesPerBvhLeaf)
        return nodeIndex;

    const AxisAlignedBoundingBox centroidBounds = computeCentroidRangeBounds(collider, firstTriangle, triangleCount);
    const glm::vec3 centroidExtent = centroidBounds.maximum - centroidBounds.minimum;
    const int splitAxis = findLargestExtentAxis(centroidExtent);

    if (centroidExtent[splitAxis] <= directionEpsilon)
        return nodeIndex;

    const std::size_t middleTriangle = firstTriangle + triangleCount / 2;
    const auto rangeBegin = collider.triangleIndices.begin() + firstTriangle;
    const auto rangeMiddle = collider.triangleIndices.begin() + middleTriangle;
    const auto rangeEnd = collider.triangleIndices.begin() + firstTriangle + triangleCount;

    std::nth_element(rangeBegin, rangeMiddle, rangeEnd, [&collider, splitAxis](std::size_t firstIndex, std::size_t secondIndex) { return getTriangleCentroid(collider, firstIndex)[splitAxis] < getTriangleCentroid(collider, secondIndex)[splitAxis]; });

    const std::size_t leftTriangleCount = middleTriangle - firstTriangle;
    const std::size_t rightTriangleCount = triangleCount - leftTriangleCount;
    const int leftChild = buildBVHNode(collider, firstTriangle, leftTriangleCount);
    const int rightChild = buildBVHNode(collider, middleTriangle, rightTriangleCount);

    // Recursive push_back calls may reallocate bvhNodes, so the node is accessed again by index.
    collider.bvhNodes[nodeIndex].leftChild = leftChild;
    collider.bvhNodes[nodeIndex].rightChild = rightChild;
    collider.bvhNodes[nodeIndex].firstTriangle = 0;
    collider.bvhNodes[nodeIndex].triangleCount = 0;
    return nodeIndex;
}

// Tests a ray against one triangle and returns barycentric coordinates.
bool intersectRayTriangle(
    const Ray& ray, // Ray expressed in the same coordinate space as the triangle.
    const glm::vec3& p0, // First triangle vertex.
    const glm::vec3& p1, // Second triangle vertex.
    const glm::vec3& p2, // Third triangle vertex.
    float maximumDistance, // Furthest accepted distance along the ray.
    RayTriangleHit& hit // Output intersection written when the test succeeds.
)
{
    const glm::vec3 edge1 = p1 - p0;
    const glm::vec3 edge2 = p2 - p0;
    const glm::vec3 perpendicular = glm::cross(ray.direction, edge2);
    const float determinant = glm::dot(edge1, perpendicular);

    if (determinant > -triangleEpsilon && determinant < triangleEpsilon)
        return false;

    const float inverseDeterminant = 1.0f / determinant;
    const glm::vec3 originOffset = ray.origin - p0;
    const float u = inverseDeterminant * glm::dot(originOffset, perpendicular);

    if (u < 0.0f)
        return false;

    const glm::vec3 crossOffset = glm::cross(originOffset, edge1);
    const float v = inverseDeterminant * glm::dot(ray.direction, crossOffset);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    const float distance = inverseDeterminant * glm::dot(edge2, crossOffset);

    if (distance < 0.0f || distance > maximumDistance)
        return false;

    hit.distance = distance;
    hit.u = u;
    hit.v = v;
    return true;
}
}

// Creates a ray and normalizes its direction when possible.
Ray makeRay(
    const glm::vec3& origin, // Ray origin in world space.
    const glm::vec3& direction // Direction to normalize.
)
{
    Ray ray;
    ray.origin = origin;

    const float directionLengthSquared = glm::dot(direction, direction);

    if (directionLengthSquared > directionEpsilon)
        ray.direction = direction / std::sqrt(directionLengthSquared);

    return ray;
}

// Transforms a local bounding sphere into world space using the largest model scale.
BoundingSphere transformBoundingSphere(
    const BoundingSphere& localSphere, // Sphere defined in model space.
    const glm::mat4& model // Model matrix applied to the sphere.
)
{
    BoundingSphere worldSphere;
    worldSphere.center = glm::vec3(model * glm::vec4(localSphere.center, 1.0f));

    const float scaleX = glm::length(glm::vec3(model[0]));
    const float scaleY = glm::length(glm::vec3(model[1]));
    const float scaleZ = glm::length(glm::vec3(model[2]));
    const float maximumScale = std::max(scaleX, std::max(scaleY, scaleZ));
    worldSphere.radius = localSphere.radius * maximumScale;
    return worldSphere;
}

// Tests a normalized ray against a sphere up to a maximum distance.
bool intersectRaySphere(
    const Ray& ray, // Normalized ray used by the intersection test.
    const BoundingSphere& sphere, // Sphere tested in the same coordinate space as the ray.
    float maximumDistance, // Furthest accepted distance along the ray.
    RaySphereHit& hit // Output intersection written when the test succeeds.
)
{
    const float directionLengthSquared = glm::dot(ray.direction, ray.direction);

    if (directionLengthSquared < 0.999f || directionLengthSquared > 1.001f)
        return false;

    const glm::vec3 sphereOffset = sphere.center - ray.origin;
    const float projectedDistance = glm::dot(sphereOffset, ray.direction);
    const float offsetLengthSquared = glm::dot(sphereOffset, sphereOffset);
    const float radiusSquared = sphere.radius * sphere.radius;

    if (projectedDistance < 0.0f && offsetLengthSquared > radiusSquared)
        return false;

    const float perpendicularDistanceSquared = offsetLengthSquared - projectedDistance * projectedDistance;

    if (perpendicularDistanceSquared > radiusSquared)
        return false;

    const float halfChord = std::sqrt(std::max(radiusSquared - perpendicularDistanceSquared, 0.0f));
    const float distance = offsetLengthSquared > radiusSquared ? projectedDistance - halfChord : projectedDistance + halfChord;

    if (distance < 0.0f || distance > maximumDistance)
        return false;

    hit.distance = distance;
    hit.position = ray.origin + ray.direction * distance;
    return true;
}

// Transforms a ray by a matrix without renormalizing its direction.
Ray transformRay(
    const Ray& ray, // Ray to transform.
    const glm::mat4& transform // Matrix applied to the origin and direction.
)
{
    Ray transformedRay;
    transformedRay.origin = glm::vec3(transform * glm::vec4(ray.origin, 1.0f));
    transformedRay.direction = glm::vec3(transform * glm::vec4(ray.direction, 0.0f));
    return transformedRay;
}

// Builds a triangle-mesh collider and its BVH from an ordered triangle-vertex list.
TriangleMeshCollider buildTriangleMeshCollider(
    std::vector<glm::vec3> triangleVertices // Triangle vertices moved into the completed collider.
)
{
    TriangleMeshCollider collider;
    collider.localSphere = buildBoundingSphere(triangleVertices);
    collider.triangleVertices = std::move(triangleVertices);

    const std::size_t triangleCount = collider.triangleVertices.size() / 3;
    collider.triangleIndices.resize(triangleCount);
    std::iota(collider.triangleIndices.begin(), collider.triangleIndices.end(), static_cast<std::size_t>(0));

    if (triangleCount > 0)
    {
        collider.bvhNodes.reserve(triangleCount * 2);
        buildBVHNode(collider, 0, triangleCount);
    }

    return collider;
}

// Finds the closest triangle hit by traversing the mesh BVH.
bool intersectRayMesh(
    const Ray& localRay, // Ray transformed into the mesh local space.
    const TriangleMeshCollider& collider, // Collider containing the triangle data and BVH.
    float maximumDistance, // Furthest accepted distance in local ray space.
    RayTriangleHit& hit // Output closest triangle intersection.
)
{
    if (collider.bvhNodes.empty())
        return false;

    bool intersectionFound = false;
    float closestDistance = maximumDistance;
    float rootEntryDistance = 0.0f;

    if (!intersectRayAABB(localRay, collider.bvhNodes[0].bounds, closestDistance, rootEntryDistance))
        return false;

    std::vector<BVHStackEntry> nodeStack;
    nodeStack.reserve(64);
    nodeStack.push_back({ 0, rootEntryDistance });

    while (!nodeStack.empty())
    {
        const BVHStackEntry stackEntry = nodeStack.back();
        nodeStack.pop_back();

        if (stackEntry.entryDistance > closestDistance)
            continue;

        const BVHNode& node = collider.bvhNodes[stackEntry.nodeIndex];

        if (node.isLeaf())
        {
            for (std::size_t offset = 0; offset < node.triangleCount; ++offset)
            {
                const std::size_t orderedIndex = node.firstTriangle + offset;
                const std::size_t triangleIndex = collider.triangleIndices[orderedIndex];
                const std::size_t firstVertex = triangleIndex * 3;
                const glm::vec3& p0 = collider.triangleVertices[firstVertex];
                const glm::vec3& p1 = collider.triangleVertices[firstVertex + 1];
                const glm::vec3& p2 = collider.triangleVertices[firstVertex + 2];

                RayTriangleHit triangleHit;

                if (!intersectRayTriangle(localRay, p0, p1, p2, closestDistance, triangleHit))
                    continue;

                intersectionFound = true;
                closestDistance = triangleHit.distance;
                hit = triangleHit;
                hit.triangleIndex = triangleIndex;
            }

            continue;
        }

        const BVHNode& leftNode = collider.bvhNodes[node.leftChild];
        const BVHNode& rightNode = collider.bvhNodes[node.rightChild];

        float leftEntryDistance = 0.0f;
        float rightEntryDistance = 0.0f;
        const bool leftHit = intersectRayAABB(localRay, leftNode.bounds, closestDistance, leftEntryDistance);
        const bool rightHit = intersectRayAABB(localRay, rightNode.bounds, closestDistance, rightEntryDistance);

        // The stack is LIFO, so the farther child is pushed first.
        if (leftHit && rightHit)
        {
            if (leftEntryDistance <= rightEntryDistance)
            {
                nodeStack.push_back({ node.rightChild, rightEntryDistance });
                nodeStack.push_back({ node.leftChild, leftEntryDistance });
            }
            else
            {
                nodeStack.push_back({ node.leftChild, leftEntryDistance });
                nodeStack.push_back({ node.rightChild, rightEntryDistance });
            }
        }
        else if (leftHit)
        {
            nodeStack.push_back({ node.leftChild, leftEntryDistance });
        }
        else if (rightHit)
        {
            nodeStack.push_back({ node.rightChild, rightEntryDistance });
        }
    }

    return intersectionFound;
}

// Tests whether a point-centered radius intersects the infinite forward ray corridor.
bool isPointWithinRayCorridor(
    const Ray& ray, // Normalized ray defining the corridor axis.
    const glm::vec3& point, // Point tested against the corridor.
    float radius, // Accepted perpendicular distance from the ray.
    float& forwardDistance // Output signed distance along the ray direction.
)
{
    const glm::vec3 pointOffset = point - ray.origin;
    forwardDistance = glm::dot(pointOffset, ray.direction);

    if (forwardDistance <= 0.0f)
        return false;

    const float distanceToPointSquared = glm::dot(pointOffset, pointOffset);
    float perpendicularDistanceSquared = distanceToPointSquared - forwardDistance * forwardDistance;
    perpendicularDistanceSquared = std::max(perpendicularDistanceSquared, 0.0f);
    return perpendicularDistanceSquared <= radius * radius;
}

// Resolves horizontal player movement against the arena limit and circular obstacles.
glm::vec3 resolvePlayerMovement(
    const glm::vec3& previousPosition, // Last valid player position.
    const glm::vec3& desiredPosition, // Position requested by camera movement.
    float playerRadius, // Horizontal radius of the player collider.
    float arenaHalfSize, // Half-size of the square playable arena.
    const std::vector<CircleCollider>& colliders // Static circular obstacles in the XZ plane.
)
{
    glm::vec3 targetPosition = desiredPosition;
    const float arenaLimit = arenaHalfSize - playerRadius - arenaCollisionMargin;
    targetPosition.x = glm::clamp(targetPosition.x, -arenaLimit, arenaLimit);
    targetPosition.z = glm::clamp(targetPosition.z, -arenaLimit, arenaLimit);

    glm::vec3 resolvedPosition = previousPosition;

    // Axis-by-axis resolution preserves the existing simple sliding behavior.
    glm::vec3 testX = resolvedPosition;
    testX.x = targetPosition.x;

    if (!checkPlayerCollision(testX, playerRadius, colliders))
        resolvedPosition.x = targetPosition.x;

    glm::vec3 testZ = resolvedPosition;
    testZ.z = targetPosition.z;

    if (!checkPlayerCollision(testZ, playerRadius, colliders))
        resolvedPosition.z = targetPosition.z;

    resolvedPosition.y = targetPosition.y;
    return resolvedPosition;
}
