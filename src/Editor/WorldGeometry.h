#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <variant>

#include "../Graphics/Mesh.h"
#include "ObjectProperties.h"

class LevelTextures;
struct EditorState;
class ActionManager;

struct LevelObject
{
    LevelObject(int id)
        : object_id(id)
    {
    }

    int object_id = 0;

    virtual bool property_gui(EditorState& state, const LevelTextures& textures,
                              ActionManager& action_manager) = 0;

    /// Function that is called when right-clicking the 2D view.
    /// Returns true if the object was clicked.
    virtual bool try_select_2d(const glm::vec2& point) = 0;
};

struct WallProps
{
    TextureProp texture_front;
    TextureProp texture_back;
};

struct WallParameters
{
    glm::vec2 start{0};
    glm::vec2 end{0};
};

struct Wall : public LevelObject
{

    Wall(int id)
        : LevelObject(id)
    {
    }

    WallParameters parameters;
    WallProps props = {{0}};

    bool property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager) override;

    bool try_select_2d(const glm::vec2& point) override;
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
    LevelObjectV2(ObjectType<Properties, Parameters> object)
        : object_type(object)
    {
    }

    LevelObjectV2(int id)
        : object_id(id)
    {
    }

    int object_id = 0;

    std::variant<WallObject> object_type;

    // virtual bool property_gui(EditorState& state, const LevelTextures& textures,
    //                           ActionManager& action_manager) = 0;

    // /// Function that is called when right-clicking the 2D view.
    // /// Returns true if the object was clicked.
    // virtual bool try_select_2d(const glm::vec2& point) = 0;
};

inline WorldGeometryMesh3D object_to_geometry(const LevelObjectV2& object)
{
    if (auto wall = std::get_if<WallObject>(&object.object_type))
    {
        return generate_wall_mesh(wall->parameters.start, wall->parameters.end,
                                  wall->properties.texture_back, wall->properties.texture_back);
    }
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
}

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_front = {0},
        .texture_back = {0},
    };

    LevelObjectV2* p_active_object_ = nullptr;
};
