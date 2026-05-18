#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace AS {

enum class LightType { Directional, Point, Spot };

struct LightData {
    LightType type      = LightType::Directional;
    glm::vec3 direction { 0.0f, -1.0f, 0.0f };
    glm::vec3 position  { 0.0f };
    glm::vec3 color     { 1.0f };
    float intensity     = 1.0f;
    float constant      = 1.0f;
    float linear        = 0.09f;
    float quadratic     = 0.032f;
    // Spot light only
    float cutOff        = 12.5f;  // degrees
    float outerCutOff   = 17.5f;  // degrees
};

struct RenderContext {
    glm::mat4 view       { 1.0f };
    glm::mat4 projection { 1.0f };
    glm::vec3 viewPos    { 0.0f };
    std::vector<LightData> lights;
};

} // namespace AS
