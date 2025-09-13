#pragma once

#include "../EditConstants.h"
#include "LevelObjectBase.h"
#include "LevelObjectTypes.h"

// =======================================
//      Platform Object Types
// =======================================
struct PolygonPlatformParameters
{
    constexpr static auto DEFAULT_SIZE = 10.0f * TILE_SIZE_F;
    // std::vector<glm::vec2> points{4};
    glm::vec2 corner_top_left{0};
    glm::vec2 corner_top_right{0, DEFAULT_SIZE};
    glm::vec2 corner_bottom_right{DEFAULT_SIZE, DEFAULT_SIZE};
    glm::vec2 corner_bottom_left{DEFAULT_SIZE, 0};
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

[[nodiscard]] bool object_deserialise(PolygonPlatformObject& poly, const nlohmann::json& json,
                                      const LevelFileIO& level_file_io);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] std::string
object_to_string<PolygonPlatformObject>(const PolygonPlatformObject& poly);

template <>
[[nodiscard]] bool object_try_select_2d<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                                               glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                                           const Rectangle& selection_area);

template <>
void object_move<PolygonPlatformObject>(PolygonPlatformObject& poly, glm::vec2 offset);

template <>
void object_rotate(PolygonPlatformObject& poly, glm::vec2 rotation_origin, float degrees);

template <>
[[nodiscard]] glm::vec2 object_get_position(const PolygonPlatformObject& poly);

template <>
[[nodiscard]] SerialiseResponse
object_serialise<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                        LevelFileIO& level_file_io);

template <>
[[nodiscard]] std::pair<Mesh2D, gl::PrimitiveType>
object_to_geometry_2d<PolygonPlatformObject>(const PolygonPlatformObject& poly,
                                             const LevelTextures& drawing_pad_texture_map);

template <>
[[nodiscard]] LevelObjectsMesh3D
object_to_geometry<PolygonPlatformObject>(const PolygonPlatformObject& poly, int floor_number);
