#pragma once

#include "LevelObjectBase.h"
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
    Direction direction = Direction::Right;
};

using PlatformObject = ObjectType<PlatformProps, PlatformParameters>;

bool operator==(const PlatformProps& lhs, const PlatformProps& rhs);
bool operator!=(const PlatformProps& lhs, const PlatformProps& rhs);

[[nodiscard]] bool object_deserialise(PlatformObject& pillar, const nlohmann::json& json,
                                      const LevelFileIO& level_file_io);

// =======================================
//      Specialised Functions
// =======================================
template <>
[[nodiscard]] std::string object_to_string<PlatformObject>(const PlatformObject& platform);

template <>
[[nodiscard]] bool object_try_select_2d<PlatformObject>(const PlatformObject& platform,
                                                        glm::vec2 selection_tile);

template <>
[[nodiscard]] bool object_is_within<PlatformObject>(const PlatformObject& platform,
                                                    const Rectangle& selection_area);

template <>
void object_move<PlatformObject>(PlatformObject& platform, glm::vec2 offset);

template <>
void object_rotate(PlatformObject& platform, glm::vec2 rotation_origin, float degrees);

template <>
[[nodiscard]] glm::vec2 object_get_position(const PlatformObject& platform);

template <>
[[nodiscard]] SerialiseResponse object_serialise<PlatformObject>(const PlatformObject& platform,
                                                                 LevelFileIO& level_file_io);
template <>
[[nodiscard]] std::pair<Mesh2DWorld, gl::PrimitiveType>
object_to_geometry_2d<PlatformObject>(const PlatformObject& platform,
                                      const LevelTextures& drawing_pad_texture_map);
template <>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry<PlatformObject>(const PlatformObject& platform,
                                                                    int floor_number);
