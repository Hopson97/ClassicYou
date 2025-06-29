#pragma once

#include <glm/glm.hpp>

constexpr inline auto rgb_to_normalised(const glm::vec3& rgb)
{
    return glm::vec4(rgb / 255.0f, 1.0f);
}

constexpr auto WORLD_SIZE = 16;
constexpr auto TILE_SIZE = 32;
constexpr auto HALF_TILE_SIZE = TILE_SIZE / 2;

constexpr auto MAIN_GRID_COLOUR = rgb_to_normalised({69, 103, 137});
constexpr auto SUB_GRID_COLOUR = rgb_to_normalised({18, 52, 86});