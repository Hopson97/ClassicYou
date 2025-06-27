#pragma once

#include <glm/glm.hpp>

// Note: The "W" component is unused for lights but rather for std140 alignment in the shaders

struct LightBase
{
    glm::vec4 colour = {1, 1, 1, 0};
    float ambient_intensity = 0.2f;
    float diffuse_intensity = 0.2f;
    float specular_intensity = 0.2f;
    float PADDING_ = 0.0;
};

struct Attenuation
{
    float constant = 1.0f;
    float linear = 0.045f;
    float exponent = 0.0075f;
    float PADDING_ = 0.0;
};

struct DirectionalLight : public LightBase
{
    glm::vec4 direction = {0, -1, 0, 0};
};

struct PointLight : public LightBase
{
    Attenuation att;
    glm::vec4 position = {0, 0, 0, 0};
};

struct SpotLight : public LightBase
{
    Attenuation att;
    glm::vec4 direction = {0, -1, 0, 0.0};
    glm::vec4 position = {0, 0, 0, 0.0};

    float cutoff = 12.5;

    // For std140 alignment
    float PADDING0_ = 0.0;
    float PADDING1_ = 0.0;
    float PADDING2_ = 0.0;
};

struct Lights
{
    SpotLight spotlight;
    DirectionalLight directional;
    PointLight pointlight;
};
