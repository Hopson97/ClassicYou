#pragma once

#include <string>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

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

