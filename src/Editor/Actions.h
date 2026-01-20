#pragma once

#include <memory>
#include <string>
#include <vector>

#include "LevelObjects/LevelObject.h"

class EditorLevel;

/// For displaying action history in the UI
struct ActionStrings
{
    std::string title;
    std::string body;
};

/// Base class for all actions that can be performed in the editor.
class Action
{
  public:
    virtual ~Action() = default;
    virtual void execute(EditorState& state, EditorLevel& level) = 0;
    virtual void undo(EditorState& state, EditorLevel& level) = 0;

    virtual ActionStrings to_string() const = 0;
};

/// Action to add a new object to the level.
class AddObjectAction final : public Action
{
  public:
    AddObjectAction(const LevelObject& object, int floor);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    /// The object to add
    LevelObject object_;

    /// The ID of the object added to the level. This for undo/redo to ensure the ID is preserved.
    int id_ = -1;

    // Flag for when re-doing this action, it uses the stored props rather than the default
    bool executed_ = false;

    const int floor_;
};

// For copy-paste functionality
class AddBulkObjectsAction final : public Action
{
  public:
    AddBulkObjectsAction(const std::vector<LevelObject>& objects, const std::vector<int>& floors);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    /// The objects to add
    std::vector<LevelObject> objects_;
    std::vector<int> floors_;

    /// The ID of the object added to the level. This for undo/redo to ensure the ID is preserved.
    std::vector<ObjectId> object_ids_;

    // Flag for when re-doing this action, it uses the stored props rather than the default
    bool executed_ = false;
};

class UpdateObjectAction final : public Action
{
  public:
    UpdateObjectAction(const LevelObject& old_object, const LevelObject& new_object, int floor);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    /// The object before the update
    const LevelObject old_object_;

    /// The object after the update
    const LevelObject new_object_;

    const int floor_;
};
class BulkUpdateObjectAction final : public Action
{
  public:
    BulkUpdateObjectAction(const std::vector<LevelObject>& old_objects,
                           const std::vector<LevelObject>& new_objects);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    /// The object before the update
    const std::vector<LevelObject> old_objects_;

    /// The object after the update
    const std::vector<LevelObject> new_objects_;
};

/// Action to delete an existing object from the level.
class DeleteObjectAction final : public Action
{
  public:
    DeleteObjectAction(const std::vector<LevelObject>& objects, const std::vector<int>& floors);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    /// The object to delete
    std::vector<LevelObject> objects_;
    std::vector<int> floors_;
};

/// Manager for storing the actions and handling undo/redo functionality.
class ActionManager
{
  public:
    ActionManager(EditorState& state, EditorLevel& level);

    void push_action(std::unique_ptr<Action> action, bool store_action = true);
    void undo_action();
    void redo_action();

    void display_action_history();

    void clear();

  private:
    EditorState* p_state_ = nullptr;
    EditorLevel* p_level_ = nullptr;

    std::vector<std::unique_ptr<Action>> action_stack_;
    size_t action_index_ = 0;
};