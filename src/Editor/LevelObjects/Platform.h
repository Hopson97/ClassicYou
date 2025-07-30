#pragma once

#include "LevelObjectTypes.h"

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
    Direction direction = Direction::Left;
};

using PlatformObject = ObjectType<PlatformProps, PlatformParameters>;

bool operator==(const PlatformProps& lhs, const PlatformProps& rhs);
bool operator!=(const PlatformProps& lhs, const PlatformProps& rhs);

// =======================================
//     Free functions
// =======================================
[[nodiscard]] LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform,
                                                        int floor_number);
[[nodiscard]] bool object_deserialise(PlatformObject& pillar, const nlohmann::json& json);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<PlatformObject>(const PlatformObject& platform,
                                                                    int floor_number);

template <>
[[nodiscard]] std::string object_to_string<PlatformObject>(const PlatformObject& platform);

template <>
void render_object_2d<PlatformObject>(const PlatformObject& platform, DrawingPad& drawing_pad,
                                      const glm::vec4& colour, bool is_selected);

template <>
[[nodiscard]] bool object_try_select_2d<PlatformObject>(const PlatformObject& platform,
                                                        glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<PlatformObject>(const PlatformObject& platform,
                                                  const Rectangle& selection_area);

template <>
void object_move<PlatformObject>(PlatformObject& platform, glm::vec2 offset);

template <>
SerialiseResponse object_serialise<PlatformObject>(const PlatformObject& platform);