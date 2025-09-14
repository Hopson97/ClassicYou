#pragma once

#include <unordered_set>

#include <SFML/Window/Event.hpp>

#include "../Graphics/Mesh.h"
#include "../Util/Maths.h"
#include "LevelObjects/LevelObject.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class EditorLevel;

namespace gl
{
    class Shader;
}

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
                          ActionManager& actions, const LevelTextures& drawing_pad_texture_map) = 0;
    virtual void render_preview() = 0;
    virtual void render_preview_2d(gl::Shader& scene_shader_2d) {};

    virtual ToolType get_tool_type() const = 0;

    virtual void show_gui(EditorState& state) {};
};

class CreateWallTool : public ITool
{
  public:
    CreateWallTool(const LevelTextures& drawing_pad_texture_map);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    LevelObjectsMesh3D wall_preview_;
    Mesh2D wall_preview_2d_;
    Mesh2D selection_node_;
    Line wall_line_;
    glm::vec2 selected_node_{0};
    bool active_dragging_ = false;
};

class UpdateWallTool : public ITool
{
  public:
    UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor,
                   const LevelTextures& drawing_pad_texture_map);
    void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    LevelObjectsMesh3D wall_preview_;
    Mesh2D edge_mesh_;
    Mesh2D wall_preview_2d_;
    LevelObject object_;
    WallObject wall_;

    /// Used to ensure walls can only be resized on the current floor as the editor
    const int wall_floor_;
    int state_floor_;

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

    void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    const ObjectTypeName object_type_;
    LevelObject object_;
    LevelObjectsMesh3D object_preview_;
    Mesh2D object_preview_2d_;
    gl::PrimitiveType preview_2d_primitive_ = gl::PrimitiveType::Triangles;
    glm::vec2 tile_{0.0f};
};

class AreaSelectTool : public ITool
{
  public:
    AreaSelectTool(EditorLevel& level);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

    void show_gui(EditorState& state) override;

  private:
    void select(EditorState& state);
    void update_previews();

  private:
    bool active_dragging_ = false;
    bool render_preview_mesh_ = false;

    EditorLevel* p_level_;

    // The line refers to the start corner and end corner
    Line selection_area_;
    LevelObjectsMesh3D selection_cube_;
    Mesh2D selection_quad_;
    glm::ivec3 selection_cube_start_{0};
    glm::ivec3 selection_cube_size_{0};

    /// The floors selected - Default is the current floor but this can be extended with Q and E for
    /// lower and upper
    int start_floor_ = 0;
    int max_floor_ = 0;
    int min_floor_ = 0;
};
