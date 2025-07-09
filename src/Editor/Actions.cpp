#include "Actions.h"

#include <print>
#include <imgui.h>

#include "EditorLevel.h"

ActionManager::ActionManager(EditorState& state, EditorLevel& level)
    : p_state_(&state)
    , p_level_(&level)
{
}

void ActionManager::push_action(std::unique_ptr<Action> action)
{
    action->execute(*p_state_, *p_level_);
    if (action_index_ < action_stack_.size())
    {
        action_stack_.resize(action_index_);
    }
    auto& moved_action = action_stack_.emplace_back(std::move(action));
    action_index_ = action_stack_.size();

    auto [title, body] = moved_action->to_string();
    std::println("Execute: {}\n{}\n\n Index {} ", title, body, action_index_);
    std::println("=======================================================");

}

void ActionManager::undo_action()
{
    if (!action_stack_.empty() && action_stack_.size() >= action_index_ && action_index_ != 0)
    {
        auto& action = action_stack_.at(action_index_ - 1);
        action->undo(*p_state_, *p_level_);
        action_index_ -= 1;

        auto [title, body] = action->to_string();
        std::println("Undo: {}\n{}\n\n Index {} ", title, body, action_index_);
    }
    else
    {
        std::println("Did not undo action as nothing left to undo.");
    }
    std::println("=======================================================");
}

void ActionManager::redo_action()
{
    if (!action_stack_.empty() && action_stack_.size() > action_index_)
    {
        auto& action = action_stack_.at(action_index_);
        action->execute(*p_state_, *p_level_);
        action_index_ += 1;

        auto [title, body] = action->to_string();
        std::println("Redo: {}\n{}\n\n Index {} ", title, body, action_index_);
    }
    else
    {
        std::println("Did not redo action as nothing to redo.");
    }
    std::println("=======================================================");
}

void ActionManager::display_action_history()
{
    if (ImGui::Begin("History"))
    {
        int i = 0;
        for (auto& item : action_stack_)
        {
            auto [title, body] = item->to_string();

            ImGui::Text("#%d:", i++);
            ImGui::SameLine();
            ImGui::Text("%s:", title.c_str());
            ImGui::Text("%s:", body.c_str());

            ImGui::Separator();
        }
    }
    ImGui::End();
}

AddWallAction::AddWallAction(const WallParameters& params)
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

ActionStrings AddWallAction::to_string() const
{
    return {
        .title = "Add Wall",
        .body = std::format("Props:\n  From Texture 1/2: {} {}\nParams:\n "
                            "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                            props_.texture_side_1.value, props_.texture_side_2.value,
                            params_.start.x, params_.start.y, params_.end.x, params_.end.y),
    };
}

UpdateWallAction::UpdateWallAction(const Wall& old_wall, const Wall& new_wall)
    : old_(old_wall)
    , new_(new_wall)
{
}

void UpdateWallAction::execute(EditorState& state, EditorLevel& level)
{
    level.update_object(new_);
}

void UpdateWallAction::undo(EditorState& state, EditorLevel& level)
{
    level.update_object(old_);
}

ActionStrings UpdateWallAction::to_string() const
{
    return {
        .title = "Update Wall",
        .body = std::format(
            "Props:\n From Texture 1/2: {} {}\n To Texture 1/2: {} {}\nParams:\n "
            "From Start position: ({:.2f}, {:.2f})\n End Position: ({:.2f}, {:.2f})\n To Start "
            "position: ({:.2f}, {:.2f})\n End Position: ({:.2f}, {:.2f})",
            old_.props.texture_side_1.value, old_.props.texture_side_2.value,
            new_.props.texture_side_1.value, new_.props.texture_side_2.value,
            old_.parameters.start.x, old_.parameters.start.y, old_.parameters.end.x,
            old_.parameters.end.y, new_.parameters.start.x, new_.parameters.start.y,
            new_.parameters.end.x, new_.parameters.end.y),
    };
}

DeleteObjectAction::DeleteObjectAction(const Wall& object)
    : wall(object)
{
}

void DeleteObjectAction::execute(EditorState& state, EditorLevel& level)
{
    state.p_active_object_ = nullptr;
    level.remove_object(wall.object_id);
}

void DeleteObjectAction::undo(EditorState& state, EditorLevel& level)
{
    auto& new_wall = level.add_wall(wall.parameters);
    new_wall.props = wall.props;

    state.p_active_object_ = &new_wall;

    level.set_object_id(new_wall.object_id, wall.object_id);
    wall = new_wall;
}

ActionStrings DeleteObjectAction::to_string() const
{
    return {
        .title = "Delete Wall",
        .body = std::format("Deleted wall with ID: {} ", wall.object_id),
    };
}
