#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "../Util/Maths.h"
#include "EditConstants.h"

class LevelTextures;

using TextureID = int;
struct TextureProp
{
    TextureID id = 0;
    glm::u8vec4 colour{255};
};

inline bool operator==(const TextureProp& lhs, const TextureProp& rhs)
{
    return lhs.id == rhs.id && lhs.colour == rhs.colour;
}

// =======================================
//      Wall Object Types
// =======================================
struct WallProps
{
    TextureProp texture_front;
    TextureProp texture_back;
    float base_height = 0.0f;
    float height = 1.0f;
};

struct WallParameters
{
    Line line;
};

inline bool operator==(const WallProps& lhs, const WallProps& rhs)
{
    return lhs.texture_front == rhs.texture_front && lhs.texture_back == rhs.texture_back &&
           lhs.base_height == rhs.base_height && lhs.height == rhs.height;
}

inline bool operator!=(const WallProps& lhs, const WallProps& rhs)
{
    return !(lhs == rhs);
}

// =======================================
//      Platform Object Types
// =======================================
struct PlatformParameters
{
    glm::vec2 position{0};
};

enum class PlatformStyle
{
    Quad,
    Diamond,
    Triangle,
};

struct PlatformProps
{
    TextureProp texture_top;
    TextureProp texture_bottom;
    float width = 1.0f;
    float depth = 1.0f;
    float base = 0.0f;

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

// =======================================
//      PolyGon Platform Object Types
// =======================================
struct PolygonPlatformParameters
{
    // std::vector<glm::vec2> points{4};
    glm::vec2 corner_top_left{0};
    glm::vec2 corner_top_right{0, WORLD_SIZE* TILE_SIZE};
    glm::vec2 corner_bottom_right{WORLD_SIZE * TILE_SIZE, WORLD_SIZE* TILE_SIZE};
    glm::vec2 corner_bottom_left{WORLD_SIZE * TILE_SIZE, 0};
};

struct PolygonPlatformProps
{
    TextureProp texture_top;
    TextureProp texture_bottom;
    float base = 0;
    bool visible = true;
};

inline bool operator==(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs)
{
    return lhs.texture_top == rhs.texture_top && lhs.texture_bottom == rhs.texture_bottom &&
           lhs.visible == rhs.visible && lhs.base == rhs.base;
}

inline bool operator!=(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs)
{
    return !(lhs == rhs);
}

// =======================================
//      Pillar Object Types
// =======================================
enum class PillarStyle
{
    Vertical,
    Diagonal,
    Horizontal
};

struct PillarParameters
{
    glm::vec2 position{0};
};

struct PillarProps
{
    TextureProp texture;
    PillarStyle style = PillarStyle::Vertical;
    float size = 0.2f;
    float base_height = 0.0f;
    float height = 1.0f;
    bool angled = false;
};

inline bool operator==(const PillarProps& lhs, const PillarProps& rhs)
{
    return lhs.texture == rhs.texture && lhs.style == rhs.style && lhs.size == rhs.size &&
           lhs.base_height == rhs.base_height && lhs.height == rhs.height &&
           lhs.angled == rhs.angled;
}

inline bool operator!=(const PillarProps& lhs, const PillarProps& rhs)
{
    return !(lhs == rhs);
}