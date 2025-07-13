#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "../Util/Maths.h"

class LevelTextures;

using TextureProp = int;

// =======================================
//      Wall Object Types
// =======================================
struct WallProps
{
    TextureProp texture_front{0};
    TextureProp texture_back{0};
    float base_height{0};
    float wall_height{2};
};

struct WallParameters
{
    Line line;
};

inline bool operator==(const WallProps& lhs, const WallProps& rhs)
{
    return lhs.texture_front == rhs.texture_front && lhs.texture_back == rhs.texture_back &&
           lhs.base_height == rhs.base_height && lhs.wall_height == rhs.wall_height;
}

inline bool operator!=(const WallProps& lhs, const WallProps& rhs)
{
    return !(lhs == rhs);
}

// =======================================
//      Platform Object Types
// =======================================
enum class PlatformStyle
{
    Quad,
    // Triagle,
    Diamond
};

struct PlatformProps
{
    TextureProp texture_top;
    TextureProp texture_bottom;
    float width = 1;
    float depth = 1;
    float base = 0;

    PlatformStyle style = PlatformStyle::Quad;
    // int direction = 0;
};

inline bool operator==(const PlatformProps& lhs, const PlatformProps& rhs)
{
    return lhs.texture_top == rhs.texture_top && lhs.texture_bottom == rhs.texture_bottom &&
           lhs.width == rhs.width && lhs.depth == rhs.depth && lhs.base == rhs.base &&
           lhs.style == rhs.style;
    // &&lhs.direction == rhs.direction;
}

inline bool operator!=(const PlatformProps& lhs, const PlatformProps& rhs)
{
    return !(lhs == rhs);
}

struct PlatformParameters
{
    glm::vec2 position{0};
};
