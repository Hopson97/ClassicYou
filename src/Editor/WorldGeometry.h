#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <variant>

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
    TextureProp texture_side_1;
    TextureProp texture_side_2;
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

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_side_1 = {0},
        .texture_side_2 = {0},
    };

    LevelObject* p_active_object_ = nullptr;
};
