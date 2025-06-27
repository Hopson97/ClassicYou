#pragma once

#include "Graphics/Lights.h"

struct Settings
{
    Settings()
    {
        lights.directional.direction = glm::normalize(-glm::vec4{15, 40, 40, 0.0f});
        lights.directional.ambient_intensity = 0.32f;
        lights.directional.diffuse_intensity = 0.65f;
        lights.directional.specular_intensity = 0.0f;

        lights.pointlight.ambient_intensity = 0.5f;
        lights.pointlight.diffuse_intensity = 1.00f;
        lights.pointlight.specular_intensity = 1.0f;
        lights.pointlight.att.constant = 1.0f;
        lights.pointlight.att.linear = 0.045f;
        lights.pointlight.att.exponent = 0.0075f;
        // lights.pointlight.colour = {0.8f,0.2f, 1.0f, 1.0f};
        lights.pointlight.colour = {0.8f, 0.8f, 1.0f, 1.0f};

        lights.spotlight.ambient_intensity = 0.012f;
        lights.spotlight.diffuse_intensity = 0.35f;
        lights.spotlight.specular_intensity = 1.0f;
        lights.spotlight.att.constant = 0.2f;
        lights.spotlight.att.linear = 0.016f;
        lights.spotlight.att.exponent = 0.003f;
    }
    Lights lights;

    bool wireframe = false;
    bool use_grass_texture = true;
    bool use_flashlight = true;
};