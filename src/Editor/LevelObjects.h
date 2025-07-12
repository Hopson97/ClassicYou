#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <variant>

#include "../Graphics/Mesh.h"
#include "ObjectProperties.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class DrawingPad;

// =======================================
//      Wall Object Types
// =======================================
struct WallProps
{
    TextureProp texture_front{0};
    TextureProp texture_back{0};
    float base_height;
    float wall_height;
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

    [[nodiscard]] LevelObjectsMesh3D to_geometry() const;
    [[nodiscard]] std::string to_string() const;

    void render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object);
    bool try_select_2d(glm::vec2 selection_tile, const LevelObject* p_active_object);
};

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_front = 0,
        .texture_back = 0,
        .base_height = 0,
        .wall_height = 2,
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

[[nodiscard]] LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall);
[[nodiscard]] LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform);