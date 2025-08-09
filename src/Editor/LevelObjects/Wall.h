#pragma once

#include "../DrawingPad.h"
#include "LevelObjectTypes.h"

// =======================================
//      Wall Object Types
// =======================================
struct WallProps
{
    TextureProp texture_front;
    TextureProp texture_back;

    float start_base_height = 0.0f;
    float start_height = 1.0f;

    float end_base_height = 0.0f;
    float end_height = 1.0f;

    bool tri_wall = false;
    bool flip_wall = false;
};

struct WallParameters
{
    Line line;
};
using WallObject = ObjectType<WallProps, WallParameters>;

bool operator==(const WallProps& lhs, const WallProps& rhs);
bool operator!=(const WallProps& lhs, const WallProps& rhs);

// =======================================
//     Free functions
// =======================================
[[nodiscard]] LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall, int floor_number);
[[nodiscard]] bool object_deserialise(WallObject& wall, const nlohmann::json& json);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<WallObject>(const WallObject& wall,
                                                                int floor_number);

template <>
[[nodiscard]] std::string object_to_string<WallObject>(const WallObject& wall);

template <>
void render_object_2d<WallObject>(const WallObject& wall, DrawingPad& drawing_pad,
                                  const glm::vec4& colour, const glm::vec2& selected_offset);

template <>
[[nodiscard]] bool object_try_select_2d<WallObject>(const WallObject& wall,
                                                    glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<WallObject>(const WallObject& wall,
                                                const Rectangle& selection_area);

template <>
void object_move<WallObject>(WallObject& wall, glm::vec2 offset);

template <>
SerialiseResponse object_serialise<WallObject>(const WallObject& wall);