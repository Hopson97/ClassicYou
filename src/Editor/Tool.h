#pragma once

#include <unordered_set>

#include <SFML/Window/Event.hpp>

#include "../Graphics/Mesh.h"
#include "../Util/Maths.h"
#include "LevelObjects/LevelObject.h"

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
    AreaSelectTool
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

    virtual void show_gui(EditorState& state) {};
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
    UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor);
    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) override;

    ToolType get_tool_type() const override;

  private:
    LevelObjectsMesh3D wall_preview_;
    LevelObject object_;
    WallObject wall_;

    /// Used to ensure walls can only be resized on the current floor as the editor
    const int wall_floor_;

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

class AreaSelectTool : public ITool
{
  public:
    AreaSelectTool(EditorLevel& level);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad, const EditorState& state) override;

    ToolType get_tool_type() const override;

    void show_gui(EditorState& state) override;

  private:
    void select(EditorState& state);

    bool active_dragging_ = false;
    bool render_preview_mesh_ = false;

    EditorLevel* p_level_;

    // The line refers to the start corner and end corner
    Line selection_area_;
    LevelObjectsMesh3D selection_cube_;
    glm::ivec3 selection_cube_start_{0};
    glm::ivec3 selection_cube_size_{0};

    /// The floors selected - Default is the current floor but this can be extended with Q and E for
    /// lower and upper
    int start_floor_ = 0;
    int max_floor_ = 0;
    int min_floor_ = 0;
};
