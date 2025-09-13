#pragma once

#include "../../Util/Maths.h"
#include "LevelObjectBase.h"
#include "LevelObjectTypes.h"

// =======================================
//      Wall Object Types
// =======================================
enum class WallStyle
{
    Normal,
    TriWall,
    FlippedTriWall,
};

struct WallProps
{
    TextureProp texture_front;
    TextureProp texture_back;

    float start_base_height = 0.0f;
    float start_height = 1.0f;

    float end_base_height = 0.0f;
    float end_height = 1.0f;

    WallStyle style = WallStyle::Normal;
};

struct WallParameters
{
    Line line;
};

using WallObject = ObjectType<WallProps, WallParameters>;

bool operator==(const WallProps& lhs, const WallProps& rhs);
bool operator!=(const WallProps& lhs, const WallProps& rhs);

[[nodiscard]] bool object_deserialise(WallObject& wall, const nlohmann::json& json,
                                      const LevelFileIO& level_file_io);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] std::string object_to_string<WallObject>(const WallObject& wall);

template <>
[[nodiscard]] bool object_try_select_2d<WallObject>(const WallObject& wall,
                                                    glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<WallObject>(const WallObject& wall,
                                                const Rectangle& selection_area);

template <>
void object_move<WallObject>(WallObject& wall, glm::vec2 offset);

template <>
void object_rotate(WallObject& wall, glm::vec2 rotation_origin, float degrees);

template <>
[[nodiscard]] glm::vec2 object_get_position(const WallObject& wall);

template <>
[[nodiscard]] SerialiseResponse object_serialise<WallObject>(const WallObject& wall,
                                                             LevelFileIO& level_file_io);

template <>
[[nodiscard]] std::pair<Mesh2D, gl::PrimitiveType>
object_to_geometry_2d<WallObject>(const WallObject& wall,
                                  const LevelTextures& drawing_pad_texture_map);
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<WallObject>(const WallObject& wall,
                                                                int floor_number);
