#pragma once

#include <memory>
#include <string>
#include <vector>

#include "LevelObjects.h"

class EditorLevel;

struct ActionStrings
{
    std::string title;
    std::string body;
};

class Action
{
  public:
    virtual ~Action() = default;
    virtual void execute(EditorState& state, EditorLevel& level) = 0;
    virtual void undo(EditorState& state, EditorLevel& level) = 0;

    virtual ActionStrings to_string() const = 0;
};


 class DeleteObjectAction final : public Action
{
  public:
    DeleteObjectAction(const LevelObject& object);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    LevelObject object_;
};

class AddObjectAction final : public Action
{
  public:
    AddObjectAction(const LevelObject& object);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    LevelObject object_;
    int id_ = -1;

    // Flag for when re-doing this action, it uses the stored props rather than the default
    bool executed_ = false;
};

class UpdateObjectAction final : public Action
{
  public:
    UpdateObjectAction(const LevelObject& old_object, const LevelObject& new_object);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    const LevelObject old_object_;
    const LevelObject new_object_;
};

class ActionManager
{
  public:
    ActionManager(EditorState& state, EditorLevel& level);

    void push_action(std::unique_ptr<Action> action, bool store_action = true);
    void undo_action();
    void redo_action();

    void display_action_history();

  private:
    EditorState* p_state_ = nullptr;
    EditorLevel* p_level_ = nullptr;

    std::vector<std::unique_ptr<Action>> action_stack_;
    size_t action_index_ = 0;
};