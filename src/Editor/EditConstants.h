#pragma once

#include <glm/glm.hpp>

#include "../Util/Util.h"


enum class EditMode
{
    // The same settings as ChallengeYou had
    Legacy,

    // Enables extra settings that CY did not have
    Extended,

    // Enables more advanced options, such as extending through multiple floors for a single object
    Advanced
};

static constexpr auto WORLD_SIZE = 80;
static constexpr auto TILE_SIZE = 32;
static constexpr auto HALF_TILE_SIZE = TILE_SIZE / 2;
static constexpr auto TILE_SIZE_F = static_cast<float>(TILE_SIZE);
static constexpr auto HALF_TILE_SIZE_F = static_cast<float>(TILE_SIZE_F);
static constexpr auto MAIN_GRID_COLOUR = rgb_to_normalised({69, 103, 137});
static constexpr auto SUB_GRID_COLOUR = rgb_to_normalised({18, 52, 86});

namespace Colour
{
    static constexpr auto RED = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
    static constexpr auto WHITE = glm::vec4{1.0f};
    static constexpr auto GREY = glm::vec4{0.5, 0.5, 0.5, 1.0f};
} // namespace Colour

static constexpr float FLOOR_HEIGHT = 2.0f;

using ObjectId = std::int32_t;
