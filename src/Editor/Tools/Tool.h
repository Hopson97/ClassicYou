#pragma once

#include <unordered_set>

#include <SFML/Window/Event.hpp>

#include "../../Graphics/Mesh.h"
#include "../../Util/Maths.h"
#include "../LevelObjects/LevelObject.h"

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
    AreaSelectTool,
    UpdatePolygon
};

class VertexPuller
{
};

/// Base interface for all tool types
class ITool
{
  public:
    virtual ~ITool() = default;

    /// Handles a given return. Returns true if an event was consumed and should prevent further
    /// processing of that event
    [[nodiscard]] virtual bool on_event(const sf::Event& event, EditorState& state,
                                        ActionManager& actions,
                                        const LevelTextures& drawing_pad_texture_map) = 0;
    virtual void render_preview() = 0;
    virtual void render_preview_2d(gl::Shader& scene_shader_2d) {};

    [[nodiscard]] virtual ToolType get_tool_type() const = 0;

    virtual void show_gui(EditorState& state) {};
    virtual void cancel_events() {};
};

class CreateWallTool : public ITool
{
  public:
    CreateWallTool(const LevelTextures& drawing_pad_texture_map);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

    void cancel_events() override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    LevelObjectsMesh3D wall_preview_;
    Mesh2DWorld wall_preview_2d_;
    Mesh2DWorld selection_node_;
    Line wall_line_;
    glm::vec2 selected_node_{0};
    bool active_dragging_ = false;
};

class UpdateWallTool : public ITool
{
  public:
    UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor,
                   const LevelTextures& drawing_pad_texture_map);
    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    LevelObjectsMesh3D wall_preview_;
    Mesh2DWorld vertex_selector_mesh_;
    Mesh2DWorld wall_preview_2d_;
    LevelObject object_;
    WallObject wall_;

    /// Used to ensure walls can only be resized on the current floor as the editor
    const int wall_floor_;
    int state_floor_ = 0;

    Line wall_line_;

    bool active_dragging_ = false;

    enum class DragTarget
    {
        None,
        Start,
        End
    } target_ = DragTarget::None;
};

class CreateObjectTool : public ITool
{
  public:
    CreateObjectTool(ObjectTypeName object_type);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
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
    Mesh2DWorld object_preview_2d_;
    gl::PrimitiveType preview_2d_primitive_ = gl::PrimitiveType::Triangles;
    glm::vec2 tile_{0.0f};
};

class AreaSelectTool : public ITool
{
  public:
    AreaSelectTool(EditorLevel& level);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
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
    Mesh2DWorld selection_quad_;
    glm::ivec3 selection_cube_start_{0};
    glm::ivec3 selection_cube_size_{0};

    /// The floors selected - Default is the current floor but this can be extended with Q and E for
    /// lower and upper
    int start_floor_ = 0;
    int max_floor_ = 0;
    int min_floor_ = 0;
};

class UpdatePolygonTool : public ITool
{
    enum class PolygonUpdateAction
    {
        MovePoint,
        AddOrDeletePoint
    };

  public:
    UpdatePolygonTool(LevelObject object, PolygonPlatformObject& wall, int wall_floor,
                      const LevelTextures& drawing_pad_texture_map);
    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(PolygonUpdateAction action);
    void update_polygon(int current_floor, ActionManager& actions, PolygonUpdateAction action);

    /// Deletes holes that are partially or fully outside of the outer region of the polygon.
    void delete_holes_outside_polygon();

    /// Gets the index of the vertex closest to the given 2D world position if there is one within
    /// MIN_SELECT_DISTANCE (defined in UpdatePolygonPlatformTool.cpp
    std::optional<size_t> closest_point_index(const std::vector<glm::vec2>& points,
                                              const glm::vec2& world_position) const;

  private:
    /// When adding a vertex to a line of the polygon, this represents the point
    struct SelectedLine
    {
        glm::vec2 world_point{0};
        glm::ivec2 node_point{0};
        size_t index;
        bool is_selected_;
    } line_;

    LevelObjectsMesh3D polygon_preview_;
    Mesh2DWorld polygon_preview_2d_;
    Mesh2DWorld vertex_selector_mesh_;
    LevelObject object_;
    PolygonPlatformObject polygon_;

    /// Used to ensure polygons can only be resized on the current floor as the editor
    const int floor_;
    int state_floor_ = 0;

    glm::vec2 state_world_position_hovered{0};
    glm::ivec2 state_node_hovered_{0};

    std::optional<size_t> target_index_;
    glm::vec2 target_new_position_{0};

    bool active_dragging_ = false;
};