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
class Camera;

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

/// Base interface for all tool types
class ITool
{
  public:
    virtual ~ITool() = default;

    /// Handles a given return. Returns true if an event was consumed and should prevent further
    /// processing of that event
    [[nodiscard]] virtual bool on_event(const sf::Event& event, EditorState& state,
                                        ActionManager& actions,
                                        const LevelTextures& drawing_pad_texture_map,
                                        const Camera& camera_3d, 
                                        bool mouse_in_2d_view) = 0;
    virtual void render_preview() = 0;
    virtual void render_preview_2d(gl::Shader& scene_shader_2d) {};

    [[nodiscard]] virtual ToolType get_tool_type() const = 0;

    virtual void show_gui(EditorState& state) {};
    virtual void cancel_events() {};
};

/**
 * Tool for creating new walls in the level.
 */
class CreateWallTool : public ITool
{
  public:
    CreateWallTool(const LevelTextures& drawing_pad_texture_map);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map,
                  const Camera& camera_3d, bool mouse_in_2d_view) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

    void cancel_events() override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    /// Shows the current position in the 3D view where the mouse is
    LevelObjectsMesh3D selection_mesh_;

    /// The preview of the wall being created in the 3D View
    Mesh2DWorld wall_preview_2d_;

    /// The preview of the wall being created in the 2D View
    LevelObjectsMesh3D wall_preview_;

    /// Shows the node being selected in the 2D view
    Mesh2DWorld vertex_selector_mesh_;

    /// The line that describes the wall being created
    Line wall_line_;

    bool active_dragging_ = false;
    bool active_dragging_3d_ = false;
    glm::vec2 selected_node_{0};
};

/**
 * Tool for updating the start and end positions of an exsisting wall.
 */
class UpdateWallTool : public ITool
{
  public:
    UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor,
                   const LevelTextures& drawing_pad_texture_map);
    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                  bool mouse_in_2d_view) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    /// The preview of the wall being edited in the 3D View
    LevelObjectsMesh3D wall_preview_;
    /// The preview of the wall being edited in the 2D View
    Mesh2DWorld wall_preview_2d_;

    /// Mesh for the selection circle at either end of the wall
    Mesh2DWorld vertex_selector_mesh_;

    /// The object being updated
    LevelObject object_;

    /// The wall being updated, used for creating the previews
    WallObject wall_;

    /// Used to ensure walls can only be resized on the current floor as the editor
    const int wall_floor_;
    int state_floor_ = 0;

    /// The line that describes the wall updated created
    Line wall_line_;

    bool active_dragging_ = false;

    /// The target point in the wall being selected
    enum class DragTarget
    {
        None,
        Start,
        End
    } target_ = DragTarget::None;
};

/**
 * Tool for creating new objects in the level
 */
class CreateObjectTool : public ITool
{
  public:
    CreateObjectTool(ObjectTypeName object_type);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                  bool mouse_in_2d_view) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(const EditorState& state, const LevelTextures& drawing_pad_texture_map);

  private:
    /// The type of object being created, used to create the preview
    const ObjectTypeName object_type_;

    /// The object that will be created
    LevelObject object_;

    /// The 3D preview of the object that will be created
    LevelObjectsMesh3D object_preview_;

    /// The 2D preview of the object that will be created
    Mesh2DWorld object_preview_2d_;

    /// When rendering the object in the 2D view, this is used to tell OpenGL what primative type to
    /// use
    gl::PrimitiveType preview_2d_primitive_ = gl::PrimitiveType::Triangles;
};

/**
 * Tool for selecting a large number of objects across multiple floors, for the purpose of moving,
 * copying, rotating etc
 */
class AreaSelectTool : public ITool
{
  public:
    AreaSelectTool(EditorLevel& level);

    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                  bool mouse_in_2d_view) override;
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

    /// Cube shows the area that will be selected in the 3D view
    LevelObjectsMesh3D selection_cube_;

    /// Quad shows the area being selected in the 2D view
    Mesh2DWorld selection_quad_;

    /// Shows the current position in the 3D view where the mouse is
    LevelObjectsMesh3D selection_mesh_;

    /// The start position of the area being selected
    glm::vec3 selection_cube_start_{0};

    /// The end position of the area being selected
    glm::vec3 selection_cube_size_{0};

    /// The floors selected - Default is the current floor but this can be extended with Q and E for
    /// lower and upper
    int start_floor_ = 0;
    int max_floor_ = 0;
    int min_floor_ = 0;
};

/**
 * Tool for updating polygon objects, such as adding, moving, and deleting
 * verticies.
 */
class UpdatePolygonTool : public ITool
{
    enum class PolygonUpdateAction
    {
        None,
        MovePoint,
        AddOrDeletePoint
    };

  public:
    UpdatePolygonTool(LevelObject object, PolygonPlatformObject& polygon, int floor,
                      const LevelTextures& drawing_pad_texture_map);
    bool on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                  const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                  bool mouse_in_2d_view) override;
    void render_preview() override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

    ToolType get_tool_type() const override;

  private:
    void update_previews(PolygonUpdateAction action, const EditorState& state);
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
        size_t index = 0;
        bool is_selected_ = false;
    } line_;

    /// The preview of the "new" polygon being created, shown in the 3D view
    LevelObjectsMesh3D polygon_preview_;

    /// The preview of the "new" polygon being created, shown in the 2D view
    Mesh2DWorld polygon_preview_2d_;

    /// The "circle" mesh shown in the 2D view when a mouse is near the polygon's edges or vertices
    Mesh2DWorld vertex_selector_mesh_;

    /// The object that is currently being edited
    LevelObject object_;

    /// The polygon that is currently being edited, used to create preview geometry
    PolygonPlatformObject polygon_;

    /// Shows the current position in the 3D view where the mouse is
    LevelObjectsMesh3D selection_mesh_;

    /// Used to ensure polygons can only be resized on the current floor as the editor
    const int floor_;
    int state_floor_ = 0;

    /// The world position hovered by the mouse in the 2D view
    glm::vec2 state_world_position_hovered_{0};

    /// The index within the polygon's geometry array that is being selected
    std::optional<size_t> target_index_;

    /// The position that the vertex at the "target_index_" is being dragged to
    glm::vec2 target_new_position_{0};

    bool active_dragging_ = false;
};