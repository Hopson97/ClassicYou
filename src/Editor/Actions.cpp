#include "Actions.h"

#include "EditorLevel.h"


ActionManager::ActionManager(EditorState& state, EditorLevel& level)
    : p_state_(&state)
    , p_level_(&level)
{
}

void ActionManager::push_action(std::unique_ptr<Action> action)
{
    pending_action_ = std::move(action);
}

void ActionManager::undo_action()
{
    if (!action_stack_.empty() && action_stack_.size() >= action_index_ && action_index_ != 0)
    {
        action_stack_.at(action_index_ - 1)->undo(*p_state_, *p_level_);
        action_index_ -= 1;
    }
}

void ActionManager::redo_action()
{
    if (!action_stack_.empty() && action_stack_.size() > action_index_)
    {
        action_stack_.at(action_index_)->execute(*p_state_, *p_level_);
        action_index_ += 1;
    }
}

void ActionManager::execute_pending()
{
    if (pending_action_)
    {
        pending_action_->execute(*p_state_, *p_level_);
        action_stack_.push_back(std::move(pending_action_));
        action_index_ = action_stack_.size();
    }
}

AddWallAction::AddWallAction(
                             const WallParameters params)
    : params_(params)
{
}

void AddWallAction::execute(EditorState& state, EditorLevel& level)
{
    auto& wall = level.add_wall(params_);
    id_ = wall.object_id;
    props_ = state.wall_default;

    state.p_active_object_ = &wall;
}

void AddWallAction::undo(EditorState& state, EditorLevel& level)
{
    state.p_active_object_ = nullptr;
    level.remove_object(id_);
}

