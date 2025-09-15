#pragma once

#include "LevelObjectBase.h"
#include "LevelObjectTypes.h"

// =======================================
//      Ramp Object Types
// =======================================
struct RampParameters
{
    glm::vec2 position{0};
};

enum class RampStyle
{
    Full,
    TriRamp,
    FlippedTriRamp,
    Corner,
    InvertedCorner
};

struct RampProps
{
    TextureProp texture_top;
    TextureProp texture_bottom;
    float width = 2.0f;
    float depth = 2.0f;

    float start_height = 0.0f;
    float end_height = 1.0f;

    Direction direction = Direction::Right;
    RampStyle style = RampStyle::Full;
};

using RampObject = ObjectType<RampProps, RampParameters>;

bool operator==(const RampProps& lhs, const RampProps& rhs);
bool operator!=(const RampProps& lhs, const RampProps& rhs);

[[nodiscard]] bool object_deserialise(RampObject& pillar, const nlohmann::json& json,
                                      const LevelFileIO& level_file_io);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] std::string object_to_string<RampObject>(const RampObject& ramp);

template <>
[[nodiscard]] bool object_try_select_2d<RampObject>(const RampObject& ramp,
                                                    glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<RampObject>(const RampObject& ramp,
                                                const Rectangle& selection_area);

template <>
void object_move<RampObject>(RampObject& ramp, glm::vec2 offset);

template <>
void object_rotate(RampObject& ramp, glm::vec2 rotation_origin, float degrees);

template <>
[[nodiscard]] glm::vec2 object_get_position(const RampObject& ramp);

template <>
[[nodiscard]] SerialiseResponse object_serialise<RampObject>(const RampObject& ramp,
                                                             LevelFileIO& level_file_io);

template <>
[[nodiscard]] std::pair<Mesh2DWorld, gl::PrimitiveType>
object_to_geometry_2d<RampObject>(const RampObject& ramp,
                                  const LevelTextures& drawing_pad_texture_map);

template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<RampObject>(const RampObject& ramp,
                                                                int floor_number);
