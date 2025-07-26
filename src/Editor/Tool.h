#pragma once

#include <unordered_set>

#include <SFML/Window/Event.hpp>

#include "../Graphics/Mesh.h"
#include "../Util/Maths.h"
#include "LevelObject.h"

class DrawingPad;
class LevelTextures;
struct EditorState;
class ActionManager;
class EditorLevel;

enum class ToolType
{
    CreateWall,
    UpdateWall,
    CreateObject,
};

class ITool
{
  public:
    virtual ~ITool() = default;

    virtual void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                          ActionManager& actions) = 0;
    virtual void render_preview() = 0;
    virtual void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) = 0;

    virtual ToolType get_tool_type() const = 0;
};

class CreateWallTool : public ITool
{
  public:
    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) override;

    ToolType get_tool_type() const override;

  private:
    LevelObjectsMesh3D wall_preview_;
    Line wall_line_;
    bool active_dragging_ = false;
};

class UpdateWallTool : public ITool
{
  public:
    UpdateWallTool(LevelObject object, WallObject& wall);
    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) override;

    ToolType get_tool_type() const override;

  private:
    LevelObjectsMesh3D wall_preview_;
    LevelObject object_;
    WallObject wall_;

    Line wall_line_;

    bool active_dragging_ = false;

    enum class DragTarget
    {
        Start,
        End
    } target_ = DragTarget::End;
};

class CreateObjectTool : public ITool
{
  public:
    CreateObjectTool(ObjectTypeName object_type);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) override;

    ToolType get_tool_type() const override;

  private:
    const ObjectTypeName object_type_;
    LevelObjectsMesh3D object_preview_;
    glm::vec2 tile_{0.0f};
};

/*
class SelectTool
{
  public:
    SelectTool(EditorLevel& level);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions);
    void render_preview();
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state);

    void move_all(glm::vec2 offset, ActionManager& actions, int floor);

    bool has_selection() const;

  private:
    EditorLevel* p_level_;
    std::unordered_set<LevelObject*> selected_objects_;

    bool active_dragging_ = false;
    Line selection_area_;
};
*/