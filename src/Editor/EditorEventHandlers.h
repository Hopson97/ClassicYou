#pragma once

#include <vector>

#include <SFML/Window/Event.hpp>
#include <glm/glm.hpp>

#include "LevelObjects/LevelObject.h"
#include "Tool.h"

class ActionManager;
struct EditorState;
class MessagesManager;
struct Selection;

/// Handles moving selected objects in the editor.
class ObjectMoveHandler
{
  public:
    ObjectMoveHandler(EditorLevel& level, ActionManager& action_manager);

    /// Handles events related to moving objects. Returns true if an object was moved and placed.
    bool handle_move_events(const sf::Event& event, const EditorState& state,
                            ToolType current_tool);

    glm::vec2 get_move_offset() const;
    bool is_moving_objects() const;

  private:
    /// If the currently selected object being dragged?
    bool moving_object_ = false;

    /// When moving an object, this is how much the selection has been offset from the
    /// "select_position"
    glm::vec2 move_offset_{0};

    /// When moving an object, this is the position to offset from
    glm::ivec2 move_start_tile_{0};

    /// Capture the state of the object being moved at the start such that the inital state can be
    /// returned to when CTRL+Z is done
    std::vector<LevelObject> moving_object_cache_;

    /// The objects being moved
    std::vector<LevelObject*> moving_objects_;

    EditorLevel* p_level_;
    ActionManager* p_action_manager_;
};

/// Handles copying and pasting selected objects in the editor.
class CopyPasteHandler
{
  public:
    CopyPasteHandler(EditorLevel& level, ActionManager& action_manager,
                     MessagesManager& messages_manager);

    void handle_events(const sf::Event& event, const Selection& selection, int current_floor);

    /// Copies the currently selected objects to the "clipboard".
    void copy_selection(const Selection& selection, int current_floor);
    void paste_selection(int current_floor);

  private:
    EditorLevel* p_level_;
    ActionManager* p_action_manager_;

    /// The level objects that are currently copied to the clipboard.
    std::vector<LevelObject> copied_objects_;

    /// The floors of the copied objects, used to determine where to paste them.
    std::vector<int> copied_objects_floors_;

    /// The floor where the copied objects will be pasted.
    int copy_start_floor_ = -1;

    MessagesManager* p_messages_manager_ = nullptr;
};