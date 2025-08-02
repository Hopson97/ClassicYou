#pragma once

#include <vector>


#include <SFML/Window/Event.hpp>
#include <glm/glm.hpp>

#include "LevelObjects/LevelObject.h"

class EditorLevel;
class ActionManager;
struct EditorState;
struct Selection;

class ObjectMoveHandler
{
  public:
    ObjectMoveHandler(EditorLevel& level, ActionManager& action_manager);
    bool handle_move_events(const sf::Event& event, const EditorState& state);

    glm::vec2 get_move_offset() const;
    bool is_moving_objects() const;

  private:
    /// If the currently seleceted object being dragged?
    bool moving_object_ = false;

    /// When moving an object, this is how much the selection has been offset from the
    /// "select_position"
    glm::vec2 move_offset_{0};

    /// When moving an object, this is the position to offset from
    glm::ivec2 move_start_tile_{0};

    /// Capture the state of the object being moved at the start such that the inital state can be
    /// returned to when CTRL+Z is done
    std::vector<LevelObject> moving_object_cache_;
    std::vector<LevelObject*> moving_objects_;

    EditorLevel* p_level_;
    ActionManager* p_action_manager_;
};

class CopyPasteHandler
{
  public:
    CopyPasteHandler(EditorLevel& level, ActionManager& action_manager);

    void handle_events(const sf::Event& event, const Selection& selection, int current_floor);
    void copy_selection(const Selection& selection, int current_floor);
    void paste_selection(int current_floor);

  private:
    EditorLevel* p_level_;
    ActionManager* p_action_manager_;

    std::vector<LevelObject> copied_objects_;
    std::vector<int> copied_objects_floors_;
    int copy_start_floor_ = -1;
};