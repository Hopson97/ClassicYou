#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <variant>

#include "../Graphics/Mesh.h"
#include "ObjectProperties.h"

class LevelTextures;
struct EditorState;
class ActionManager;

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

template <typename Properties, typename Parameters>
struct ObjectType
{
    using PropertiesType = Properties;
    using ParametersType = Parameters;

    Properties properties;
    Parameters parameters;
};

using WallObject = ObjectType<WallProps, WallParameters>;

struct LevelObjectV2
{
    template <typename Properties, typename Parameters>
    LevelObjectV2(const ObjectType<Properties, Parameters>& object)
        : object_type(object)
    {
    }

    LevelObjectV2(int object_id)
        : object_id(object_id)
    {
    }

    int object_id = 0;

    std::variant<WallObject> object_type;

    void property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager);
};

inline LevelObjectsMesh3D object_to_geometry(const LevelObjectV2& object)
{
    if (auto wall = std::get_if<WallObject>(&object.object_type))
    {
        return generate_wall_mesh(wall->parameters.start, wall->parameters.end,
                                  wall->properties.texture_back, wall->properties.texture_back);
    }

    throw std::runtime_error("Must implement getting geometry for all types!");
}

inline std::string object_to_string(const LevelObjectV2& object)
{
    if (auto wall = std::get_if<WallObject>(&object.object_type))
    {
        return

            std::format("Props:\n  From Texture 1/2: {} {}\nParameters:\n "
                        "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                        wall->properties.texture_front, wall->properties.texture_back,
                        wall->parameters.start.x, wall->parameters.start.y, wall->parameters.end.x,
                        wall->parameters.end.y);
    }
    return "";
}

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_front = 0,
        .texture_back = 0,
    };

    LevelObjectV2* p_active_object_ = nullptr;
};
