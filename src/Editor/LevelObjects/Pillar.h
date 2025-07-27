#pragma once

#include "../DrawingPad.h"
#include "LevelObjectTypes.h"

#include "../EditConstants.h"

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

using PillarObject = ObjectType<PillarProps, PillarParameters>;

bool operator==(const PillarProps& lhs, const PillarProps& rhs);
bool operator!=(const PillarProps& lhs, const PillarProps& rhs);

// =======================================
//     Free functions
// =======================================
[[nodiscard]] LevelObjectsMesh3D generate_pillar_mesh(const PillarObject& pillar, int floor_number);
[[nodiscard]] bool object_deserialise(PillarObject& pillar, const nlohmann::json& json);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<PillarObject>(const PillarObject& pillar,
                                                                  int floor_number);

template <>
[[nodiscard]] std::string object_to_string<PillarObject>(const PillarObject& pillar);

template <>
void render_object_2d<PillarObject>(const PillarObject& pillar, DrawingPad& drawing_pad,
                                    const glm::vec4& colour, bool is_selected);

template <>
[[nodiscard]] bool object_try_select_2d<PillarObject>(const PillarObject& pillar,
                                                      glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<PillarObject>(const PillarObject& pillar,
                                                  const Rectangle& selection_area);

template <>
void object_move<PillarObject>(PillarObject& pillar, glm::vec2 offset);

template <>
SerialiseResponse object_serialise<PillarObject>(const PillarObject& pillar);