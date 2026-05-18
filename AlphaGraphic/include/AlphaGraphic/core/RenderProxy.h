#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace AG {

// Pure data structs — no OpenGL resources.
// AlphaScene produces these; AlphaGraphic's BufferManager consumes them.
// All fields use vec4 padding to match std430 layout (no vec3).

struct CameraProxy
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invView;
    glm::mat4 invProjection;
    glm::vec4 position;   // xyz = world pos, w = unused
};

struct LightProxy
{
    glm::vec4 position;      // xyz = pos,       w = type (0=dir,1=point,2=spot)
    glm::vec4 direction;     // xyz = dir,        w = intensity
    glm::vec4 color;         // rgb = color,      w = unused
    glm::vec4 attenuation;   // x=constant, y=linear, z=quadratic, w=unused
    glm::vec4 spotParams;    // x=cosInner, y=cosOuter, zw=unused
};

struct MeshProxy
{
    glm::mat4 modelMatrix;
    glm::mat4 normalMatrix;  // transpose(inverse(model)) — for normal transform
    glm::vec4 boundsCenter;  // xyz = AABB center, w = sphere radius
};

// One triangle in world space — std430 layout (vec4 only, no vec3).
struct TriangleProxy
{
    glm::vec4 v0, v1, v2;   // xyz = world position
    glm::vec4 n0, n1, n2;   // xyz = world normal (per-vertex, smooth shading)
    glm::vec4 uv01;          // xy = uv0,  zw = uv1
    glm::vec4 uv2mat;        // xy = uv2,  z = diffuseMatIdx,  w = specularMatIdx
};

// BVH node — std430 layout.
// Internal: left/right = child indices.
// Leaf:     left = -1, right = triCount, triOffset = start index.
struct BVHNodeProxy
{
    glm::vec4 aabbMin;   // xyz = AABB min corner
    glm::vec4 aabbMax;   // xyz = AABB max corner
    int left;            // child index, or -1 for leaf
    int right;           // child index (internal) or triCount (leaf)
    int triOffset;       // first triangle index (leaf only)
    int pad;
};

struct SceneProxy
{
    CameraProxy                 camera;
    std::vector<MeshProxy>      meshes;
    std::vector<LightProxy>     lights;
    std::vector<TriangleProxy>  triangles;   // flat world-space list (reordered by BVH build)
    std::vector<BVHNodeProxy>   bvhNodes;    // built from triangles; root = index 0
    std::vector<GLuint>         diffuseTexIDs;  // per-mesh, indexed by triangle's z
    std::vector<GLuint>         specularTexIDs; // per-mesh, indexed by triangle's w
};

} // namespace AG
