#pragma once

#include <string>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../../Graphics/Mesh.h"

class DrawingPad;
struct LevelObject;

// =======================================
//     Object Type Template
// =======================================
enum class ObjectTypeName
{
    Wall,
    Platform,
    PolygonPlatform,
    Pillar,
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

inline bool operator==(const TextureProp& lhs, const TextureProp& rhs)
{
    return lhs.id == rhs.id && lhs.colour == rhs.colour;
}

using SerialiseResponse = std::pair<nlohmann::json, std::string>;

template <typename T>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry(const T& object, int floor_number);

template <typename T>
[[nodiscard]] std::string object_to_string(const T& object);

template <typename T>
void render_object_2d(const T& object, DrawingPad& pad, const glm::vec4& colour, bool is_selected);

template <typename T>
bool object_try_select_2d(const T& object, glm::vec2 selection_tile);
