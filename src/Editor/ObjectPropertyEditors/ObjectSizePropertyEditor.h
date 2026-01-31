#pragma once

#include "LevelObjectPropertyEditor.h"

#include <glm/glm.hpp>

#include "../../Util/Maths.h"
#include "../EditConstants.h"
#include "../EditorGUI.h"
#include "../LevelObjects/LevelObject.h"

/// Class for updating the size of certain objects via the world views
class ObjectSizePropertyEditor : public LevelObjectPropertyEditor
{
  public:
    /// The "face" being dragged. For corners this can be two faces
    enum PullDirection
    {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 4,
        Right = 8
    };

  public:
    ObjectSizePropertyEditor(const LevelObject& object, glm::vec2 position, glm::vec2 size,
                             int object_floor);

    bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                      const LevelTextures& drawing_pad_texture_map,
                      const Camera& camera_3d) override;

    void render_preview_2d(gl::Shader& scene_shader_2d) override;
    void render_preview_3d(gl::Shader& scene_shader_3d, bool always_show_gizmos) override;

    void render_to_picker_mouse_over(const MousePickingState& picker_state,
                                     gl::Shader& picker_shader) override;

  private:
    /// Try to resize the object when the mouse is being moved. It only resizes at half-tile
    /// intervals
    void try_resize(const Rectangle& object_rect, glm::vec2 intersect, glm::vec2& new_position,
                    glm::vec2& new_size);

    /// Drag the right or bottom side.
    /// Axis should be 0 for X (Width) and 1 for Y (Height)
    void drag_positive_direction(const Rectangle& object_rect, int axis, glm::vec2 intersect,
                                 glm::vec2& new_size);

    /// Drag the left or top side.
    /// Axis should be 0 for X (Width) and 1 for Y (Height)
    void drag_negative_direction(const Rectangle& object_rect, int axis, glm::vec2 intersect,
                                 glm::vec2& new_position, glm::vec2& new_size);

    /// Update the object being referenced from this editor, which updates its mesh and enables
    /// saving the update history for when the mouse is released such that it can undo/redo can be
    /// applied as needed.
    void update_object(const LevelObject& object, int current_floor, ActionManager& actions,
                       bool store_action);

    /// Updates the previews
    void update_previews();

    /// Checks if the given size is within the permitted size for this object
    bool size_within_bounds(float size) const;

    /// Generate the offsets used for correctly rendering the axis and corner preview gizmos
    void generate_3d_offsets();

    /// For better feel when dragging a given point, this maps each edge and corner to the base
    /// height used to calculate the intersect when dragging that edge/corner
    void calculate_pull_direction_to_height_mapping();

    // Fields modified by the editor
    glm::vec2 position_;
    glm::vec2 size_;

    // The floor of the object to prevent resize when on a different floor to it
    int object_floor_ = 0;
    int state_floor_ = 0;

    // Direction the edges are being pulled in
    int pull_direction_;

    /// True if the size is being modified in the 2D view
    bool active_dragging_ = false;

    /// True if the size is being modified in the 3D view
    bool active_dragging_3d_ = false;
    bool mouseover_edge_3d_ = false;

    glm::ivec2 start_drag_position_;

    // Preview for each side of the given shape to show which side is being re-sized
    Mesh2DWorld left_line_preview_;
    Mesh2DWorld right_line_preview_;
    Mesh2DWorld top_line_preview_;
    Mesh2DWorld bottom_line_preview_;

    /// Preview gizmos for the object edges that can be dragged to resize
    Mesh3D left_line_preview_3d_;
    Mesh3D right_line_preview_3d_;
    Mesh3D top_line_preview_3d_;
    Mesh3D bottom_line_preview_3d_;

    /// Preview gizmo for the corners
    Mesh3D cube_corner_preview_3d_;

    //// The axis lines shows which axis is being resized
    Mesh3D axis_line_x;
    Mesh3D axis_line_z;

    float min_size_ = 0.5f;
    float max_size_ = 100.0f;

    /// The object before any state changes to ensure history can be restored to the cached object
    /// state for "undo".
    LevelObject cached_object_;

    /// True if when the mouse is released, the update should be stored to the history. This
    /// prevents non-updates from being saved.
    bool save_update_ = false;

    struct Offset3D
    {
        float x = 0.0f;
        float z = 0.0f;
        float angle = 0.0f;
        int pull_direction = 0;
        float height = 0;
    };

    /// Offset used to render the axis lines
    std::array<Offset3D, 4> axis_offsets_;

    /// Offset used to "render" the corner cubes for selection
    std::array<Offset3D, 4> corner_offsets_;

    /// This is used such that when the line is being dragged, the mouse/floor intersect uses the
    /// correct offset when calculating where the ray intersects. Same applies for the corners.
    std::unordered_map<int, float> pull_direction_to_height_;

    /// The base of the object where the mouse should intersect the ground plane for dragging
    /// objects
    float base_y_ = 0;
};