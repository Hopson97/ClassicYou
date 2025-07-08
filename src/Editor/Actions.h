#pragma once

#include <memory>

#include "WorldGeometry.h"

class EditorLevel;

class Action
{
  public:
    virtual void execute(EditorState& state, EditorLevel& level) = 0;
    virtual void undo(EditorState& state, EditorLevel& level) = 0;

    virtual void display_as_gui() = 0;
};

class AddWallAction final : public Action
{
  public:
    AddWallAction(const WallParameters& params);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    void display_as_gui() override;

  private:
    const WallParameters params_;

    WallProps props_;
    int id_ = -1;
};

class ActionManager
{
  public:
    ActionManager(EditorState& state, EditorLevel& level);

    void push_action(std::unique_ptr<Action> action);
    void undo_action();
    void redo_action();

    void execute_pending();

    void display_action_history();

  private:
    EditorState* p_state_ = nullptr;
    EditorLevel* p_level_ = nullptr;

    std::vector<std::unique_ptr<Action>> action_stack_;
    std::unique_ptr<Action> pending_action_;
    size_t action_index_ = 0;
};