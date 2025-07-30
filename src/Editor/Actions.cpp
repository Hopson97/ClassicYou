#include "Actions.h"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <print>

#include "EditorLevel.h"
#include "EditorState.h"

ActionManager::ActionManager(EditorState& state, EditorLevel& level)
    : p_state_(&state)
    , p_level_(&level)
{
}

void ActionManager::push_action(std::unique_ptr<Action> action, bool store_action)
{
    action->execute(*p_state_, *p_level_);
    if (store_action)
    {

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
            ImGui::Text("%s", title.c_str());
            ImGui::Text("%s", body.c_str());

            ImGui::Separator();
        }
    }
    ImGui::End();
}

void ActionManager::clear()
{
    action_stack_.clear();
}

// =======================================
//    Add Action
// =======================================
AddObjectAction::AddObjectAction(const LevelObject& object, int floor)
    : object_(object)
    , floor_{floor}
{
}

void AddObjectAction::execute(EditorState& state, EditorLevel& level)
{

    // When redoing the action, this prevents using the default for this object type
    if (!executed_)
    {

        auto& level_object = level.add_object(object_, floor_);
        id_ = level_object.object_id;

        executed_ = true;
        state.p_active_object = &level_object;
    }
    else
    {
        auto& level_object = level.add_object(object_, floor_);
        level.set_object_id(level_object.object_id, id_);
        state.p_active_object = &level_object;
    }
}

void AddObjectAction::undo(EditorState& state, EditorLevel& level)
{
    state.p_active_object = nullptr;
    level.remove_object(id_);
}

ActionStrings AddObjectAction::to_string() const
{
    return {
        .title = std::format("Create {}", object_.to_type_string()),
        .body = object_.to_string(),
    };
}

AddObjectActionV2::AddObjectActionV2(const LevelObject& object, int floor)
    : object_(object)
    , floor_{floor}
{
}

void AddObjectActionV2::execute(EditorState& state, EditorLevel& level)
{

    // When redoing the action, this prevents using the default for this object type
    if (!executed_)
    {

        auto& level_object = level.add_object(object_, floor_);
        id_ = level_object.object_id;

        executed_ = true;
        state.selection.set_selection(&level_object);
    }
    else
    {
        auto& level_object = level.add_object(object_, floor_);
        level.set_object_id(level_object.object_id, id_);
        state.selection.set_selection(&level_object);
    }
}

void AddObjectActionV2::undo(EditorState& state, EditorLevel& level)
{
    state.p_active_object = nullptr;
    level.remove_object(id_);
}

ActionStrings AddObjectActionV2::to_string() const
{
    return {
        .title = std::format("Create {}", object_.to_type_string()),
        .body = object_.to_string(),
    };
}

// =======================================
//    Update Action
// =======================================
UpdateObjectAction::UpdateObjectAction(const LevelObject& old_object, const LevelObject& new_object,
                                       int floor)
    : old_object_(old_object)
    , new_object_(new_object)
    , floor_(floor)
{
}

void UpdateObjectAction::execute([[maybe_unused]] EditorState& state, EditorLevel& level)
{
    level.update_object(new_object_, floor_);
}

void UpdateObjectAction::undo([[maybe_unused]] EditorState& state, EditorLevel& level)
{
    level.update_object(old_object_, floor_);
}

ActionStrings UpdateObjectAction::to_string() const
{
    return {
        .title = std::format("Update {}", old_object_.to_type_string()),
        .body = std::format("Before:\n{}\n\nAfter:\n{}", old_object_.to_string(),
                            new_object_.to_string()),
    };
}

// =======================================
//    Delete Action
// =======================================
DeleteObjectAction::DeleteObjectAction(const LevelObject& object, int floor)
    : object_(object)
    , floor_{floor}
{
}

void DeleteObjectAction::execute(EditorState& state, EditorLevel& level)
{
    state.p_active_object = nullptr;
    level.remove_object(object_.object_id);
}

void DeleteObjectAction::undo(EditorState& state, EditorLevel& level)
{
    auto& new_object = level.add_object(object_, floor_);

    // new_object.p = object_.props;

    state.p_active_object = &new_object;

    level.set_object_id(new_object.object_id, object_.object_id);
    object_ = new_object;
}

ActionStrings DeleteObjectAction::to_string() const
{
    return {
        .title = std::format("Delete {}", object_.to_type_string()),
        .body = std::format("ID: {} ", object_.object_id),
    };
}