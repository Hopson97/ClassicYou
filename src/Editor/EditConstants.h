#pragma once

#include <glm/glm.hpp>

#include "../Util/Util.h"
#include <glm/glm.hpp>

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
static constexpr auto HALF_TILE_SIZE_F = TILE_SIZE_F / 2.0f;

static constexpr glm::u8vec4 MAIN_GRID_COLOUR{69, 103, 137, 255};
static constexpr glm::u8vec4 SUB_GRID_COLOUR{18, 52, 86, 255};

static constexpr glm::vec3 DEFAULT_CLEAR_COLOUR{0.0f, 35.0f / 255.0f, 70.0f / 255.0f};

namespace Colour
{
    static constexpr auto WHITE = glm::u8vec4{255};
    static constexpr auto RED = glm::u8vec4{255, 0, 0, 255};
    static constexpr auto GREEN = glm::u8vec4{0, 255, 0, 255};
    static constexpr auto BLUE = glm::u8vec4{0, 0, 255, 255};
    static constexpr auto MAGENTA = glm::u8vec4{255, 0, 255, 255};
} // namespace Colour

static constexpr float FLOOR_HEIGHT = 2.0f;

using ObjectId = std::int32_t;
