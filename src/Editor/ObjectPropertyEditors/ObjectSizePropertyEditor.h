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
                      const LevelTextures& drawing_pad_texture_map) override;
    void render_preview_2d(gl::Shader& scene_shader_2d) override;

  private:
    /// Drag the right or bottom side.
    /// Axis should be 0 for X (Width) and 1 for Y (Height)
    void drag_positive_direction_2d(const Rectangle& object_rect, int axis, glm::ivec2 node_hovered,
                                    glm::vec2& new_size);

    /// Drag the left or top side.
    /// Axis should be 0 for X (Width) and 1 for Y (Height)
    void drag_negative_direction_2d(const Rectangle& object_rect, int axis, glm::ivec2 node_hovered,
                                    glm::vec2& new_position, glm::vec2& new_size);

    void update_object(const LevelObject& object, int current_floor, ActionManager& actions,
                       bool store_action);

    void update_previews();

    // Fields modified by the editor
    glm::vec2 position_;
    glm::vec2 size_;

    // The floor of the object to prevent resize when on a different floor to it
    int object_floor_ = 0;

    // Direction the edges are being pulled in
    int pull_direction_;
    bool active_dragging_ = false;

    glm::ivec2 start_drag_position_;

    // Preview for each side of the given shape to show which side is being re-sized
    Mesh2DWorld left_line_preview_;
    Mesh2DWorld right_line_preview_;
    Mesh2DWorld top_line_preview_;
    Mesh2DWorld bottom_line_preview_;

    /// The object before any state changes to ensure history can be restored to the cached object
    /// state for "undo".
    LevelObject cached_object_;
};
