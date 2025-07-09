#pragma once

#include <vector>
#include <memory>
#include <string>

#include "WorldGeometry.h"

class EditorLevel;

struct ActionStrings
{
    std::string title;
    std::string body;
};

class Action
{
  public:
    virtual void execute(EditorState& state, EditorLevel& level) = 0;
    virtual void undo(EditorState& state, EditorLevel& level) = 0;

    virtual ActionStrings to_string() const = 0;
};

class AddWallAction final : public Action
{
  public:
    AddWallAction(const WallParameters& params);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    const WallParameters params_;

    WallProps props_{{0}};
    int id_ = -1;

    // Flag for when re-doing this action, it uses the stored props rather than the default
    bool executed_ = false;
};

// TODO Maybe the props should be a defined dict rather a class such that multiple classes do not
// need to be created for every object
class UpdateWallAction final : public Action
{
  public:
    UpdateWallAction(const Wall& old_wall, const Wall& new_wall);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    const Wall old_;
    const Wall new_;
};


class DeleteObjectAction final : public Action
{
  public:
    DeleteObjectAction(const Wall& object);

    void execute(EditorState& state, EditorLevel& level) override;
    void undo(EditorState& state, EditorLevel& level) override;

    ActionStrings to_string() const override;

  private:
    Wall wall;

};

class ActionManager
{
  public:
    ActionManager(EditorState& state, EditorLevel& level);

    void push_action(std::unique_ptr<Action> action);
    void undo_action();
    void redo_action();

    void display_action_history();

  private:
    EditorState* p_state_ = nullptr;
    EditorLevel* p_level_ = nullptr;

    std::vector<std::unique_ptr<Action>> action_stack_;
    size_t action_index_ = 0;
};