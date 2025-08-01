#pragma once

#include "../EditConstants.h"
#include "LevelObjectTypes.h"

// =======================================
//      Platform Object Types
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

using PolygonPlatformObject = ObjectType<PolygonPlatformProps, PolygonPlatformParameters>;

bool operator==(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs);
bool operator!=(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs);

// =======================================
//     Free functions
// =======================================
[[nodiscard]] LevelObjectsMesh3D generate_polygon_platform_mesh(const PolygonPlatformObject& poly,
                                                                int floor_number);
[[nodiscard]] bool object_deserialise(PolygonPlatformObject& poly, const nlohmann::json& json);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] LevelObjectsMesh3D
object_to_geometry<PolygonPlatformObject>(const PolygonPlatformObject& poly, int floor_number);

template <>
[[nodiscard]] std::string
object_to_string<PolygonPlatformObject>(const PolygonPlatformObject& poly);

template <>
void render_object_2d<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                             DrawingPad& drawing_pad, const glm::vec4& colour);

template <>
[[nodiscard]] bool object_try_select_2d<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                                               glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                                           const Rectangle& selection_area);

template <>
void object_move<PolygonPlatformObject>(PolygonPlatformObject& poly, glm::vec2 offset);

template <>
SerialiseResponse object_serialise<PolygonPlatformObject>(const PolygonPlatformObject& poly);