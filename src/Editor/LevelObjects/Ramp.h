#pragma once

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

// =======================================
//     Free functions
// =======================================
[[nodiscard]] LevelObjectsMesh3D generate_ramp_mesh(const RampObject& ramp, int floor_number);
[[nodiscard]] bool object_deserialise(RampObject& pillar, const nlohmann::json& json);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<RampObject>(const RampObject& ramp,
                                                                int floor_number);

template <>
[[nodiscard]] std::string object_to_string<RampObject>(const RampObject& ramp);

template <>
void render_object_2d<RampObject>(const RampObject& ramp, DrawingPad& drawing_pad,
                                  const glm::vec4& colour, const glm::vec2& selected_offset);

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
SerialiseResponse object_serialise<RampObject>(const RampObject& ramp);