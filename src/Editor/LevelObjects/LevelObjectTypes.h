#pragma once

#include <string>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../../Graphics/Mesh.h"

class DrawingPad;
struct LevelObject;
struct Rectangle;
class LevelFileIO;

// =======================================
//     Object Type Template
// =======================================
enum class ObjectTypeName
{
    MissingTypeName,

    Wall,
    Platform,
    PolygonPlatform,
    Pillar,
    Ramp
};

/// All objects in the level editor have a set of properties and parameters.
/// The properties should be things editable by the properties GUI, such as textures, height, etc.
/// The parameters are things that are not editable by the properties GUI, such as the object
/// position.
template <typename Properties, typename Parameters>
struct ObjectType
{
    using PropertiesType = Properties;
    using ParametersType = Parameters;

    Properties properties;
    Parameters parameters;
};

class LevelTextures;

using TextureID = int;
struct TextureProp
{
    TextureID id = 0;
    glm::u8vec4 colour{255};
};

// For objects that have a direction
enum class Direction
{
    Right,
    Left,
    Forward,
    Back
};

inline bool operator==(const TextureProp& lhs, const TextureProp& rhs)
{
    return lhs.id == rhs.id && lhs.colour == rhs.colour;
}

using SerialiseResponse = std::pair<nlohmann::json, std::string>;

/**
 * All Level objects must specialize these functions. These are called via std::visit in
 * LevelObject.cpp
 */

template <typename T>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry(const T& object, int floor_number);

template <typename T>
[[nodiscard]] std::string object_to_string(const T& object);

template <typename T>
void render_object_2d(const T& object, DrawingPad& pad, const glm::vec4& colour,
                      const glm::vec2& selected_offset = {0, 0});

template <typename T>
[[nodiscard]] bool object_try_select_2d(const T& object, glm::vec2 selection_tile);

template <typename T>
[[nodiscard]] bool object_is_within(const T& object, const Rectangle& selection_area);

template <typename T>
void object_move(T& object, glm::vec2 offset);

template <typename T>
void object_rotate(T& object, glm::vec2 rotation_origin, float degrees);

template <typename T>
[[nodiscard]] glm::vec2 object_get_position(const T& object);

template <typename T>
SerialiseResponse object_serialise(const T& object, LevelFileIO& level_file_io);
