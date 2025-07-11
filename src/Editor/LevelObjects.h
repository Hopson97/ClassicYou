#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <variant>

#include "../Graphics/Mesh.h"
#include "ObjectProperties.h"

class LevelTextures;
struct EditorState;
class ActionManager;

// =======================================
//      Wall Object Types
// =======================================
struct WallProps
{
    TextureProp texture_front{0};
    TextureProp texture_back{0};
};

struct WallParameters
{
    glm::vec2 start{0};
    glm::vec2 end{0};
};

// =======================================
//      Platform Object Types
// =======================================
struct PlatformProps
{
    TextureProp texture_top;
    TextureProp texture_bottom;
    float width = 1;
    float depth = 1;
    float base = 0;
};

struct PlatformParameters
{
    glm::vec2 position{0};
};

// =======================================
//     Object Type Template
// =======================================
template <typename Properties, typename Parameters>
struct ObjectType
{

    using PropertiesType = Properties;
    using ParametersType = Parameters;

    Properties properties;
    Parameters parameters;
};

using WallObject = ObjectType<WallProps, WallParameters>;
using PlatformObject = ObjectType<PlatformProps, PlatformParameters>;

struct LevelObject
{
    template <typename Properties, typename Parameters>
    LevelObject(const ObjectType<Properties, Parameters>& object)
        : object_type(object)
    {
    }

    LevelObject(int object_id)
        : object_id(object_id)
    {
    }

    int object_id = 0;

    std::variant<WallObject, PlatformObject> object_type;

    void property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager);
};

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_front = 0,
        .texture_back = 0,
    };

    PlatformProps platform_default = {
        .texture_top = 0,
        .texture_bottom = 0,
        .width = 1,
        .depth = 1,
        .base = 0,
    };

    LevelObject* p_active_object_ = nullptr;
};

LevelObjectsMesh3D object_to_geometry(const LevelObject& object);
std::string object_to_string(const LevelObject& object);

[[nodiscard]] LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall);
[[nodiscard]] LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform);