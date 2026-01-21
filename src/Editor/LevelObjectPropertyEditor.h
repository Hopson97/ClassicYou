#pragma once

#include "EditConstants.h"
#include <SFML/Window/Event.hpp>
#include <glm/glm.hpp>

#include "../Graphics/Mesh.h"
#include "../Util/Maths.h"
#include "EditorGUI.h"
#include "LevelObjects/LevelObject.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class EditorLevel;

class PropertyEditor
{
  public:
    virtual bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map) = 0;
    virtual void display_2d_editor() = 0;

    // virtual bool display_gui() = 0;
};

/// Class for updating the size of certain objects via the world views
class ObjectSizeEditor : public PropertyEditor
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
    ObjectSizeEditor(glm::vec2 position, glm::vec2 size, int object_floor);

    bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                      const LevelTextures& drawing_pad_texture_map) override;
    void display_2d_editor() override;

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
    // Fields modified by the editor
    glm::vec2 position_;
    glm::vec2 size_;

    // The floor of the object to prevent resize when on a different floor to it
    int object_floor_ = 0;

    // Direction the edges are being pulled in
    int pull_direction_;
    bool active_dragging_ = false;

    glm::ivec2 start_drag_position_;

    Mesh2DWorld left_line_preview_;
    Mesh2DWorld right_line_preview_;
    Mesh2DWorld top_line_preview_;
    Mesh2DWorld bottom_line_preview_;
};

// struct HeightOffsetEditor : public PropertyEditor
//{
//     const glm::vec2* p_position = nullptr;
//     float* p_offset = nullptr;
//
//     HeightOffsetEditor(ObjectId object_id, const glm::vec2& position, float offset)
//         : PropertyEditor(object_id)
//     {
//     }
//
//     bool display_gui() override;
// };
//

struct LevelObjectPropertyEditor
{
    std::vector<std::unique_ptr<PropertyEditor>> editors;
};