#pragma once

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

struct SceneProxy
{
    CameraProxy             camera;
    std::vector<MeshProxy>  meshes;
    std::vector<LightProxy> lights;
};

} // namespace AG
