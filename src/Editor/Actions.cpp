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
//          AddObjectAction
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
        state.selection.set_selection(&level_object, floor_);
    }
    else
    {
        auto& level_object = level.add_object(object_, floor_);
        level.set_object_id(level_object.object_id, id_);
        state.selection.set_selection(&level_object, floor_);
    }
}

void AddObjectAction::undo(EditorState& state, EditorLevel& level)
{
    state.selection.clear_selection();
    level.remove_object(id_);
}

ActionStrings AddObjectAction::to_string() const
{
    return {
        .title = std::format("Create {}", object_.to_type_string()),
        .body = object_.to_string(),
    };
}

// =======================================
//          AddBulkObjectsAction
// =======================================
AddBulkObjectsAction::AddBulkObjectsAction(const std::vector<LevelObject>& objects,
                                           const std::vector<int>& floors)
    : objects_(objects)
    , floors_{floors}
{
}

void AddBulkObjectsAction::execute(EditorState& state, EditorLevel& level)
{
    // New objects are selected - so clear the old selection

    state.selection.clear_selection();
    // When redoing the action, this prevents using the default for this object type
    if (!executed_)
    {
        state.selection.clear_selection();
        object_ids.clear();
        for (int i = 0; i < objects_.size(); i++)
        {
            auto& level_object = level.add_object(objects_[i], floors_[i]);
            object_ids.push_back(level_object.object_id);
            state.selection.add_to_selection(level_object.object_id, floors_[i]);
        }
        executed_ = true;
    }
    else
    {
        state.selection.clear_selection();
        for (int i = 0; i < objects_.size(); i++)
        {
            auto& level_object = level.add_object(objects_[i], floors_[i]);
            state.selection.add_to_selection(level_object.object_id, floors_[i]);
            level.set_object_id(level_object.object_id, object_ids[i]);
        }
    }
}

void AddBulkObjectsAction::undo(EditorState& state, EditorLevel& level)
{
    state.selection.clear_selection();
    for (auto id : object_ids)
    {
        level.remove_object(id);
    }
}

ActionStrings AddBulkObjectsAction::to_string() const
{
    std::string body;
    for (auto& object : objects_)
    {
        body += object.to_string();
    }
    return {
        .title = std::format("Adding bulk {}", objects_.size()),
        .body = body,
    };
}

// =======================================
//          UpdateObjectAction
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

BulkUpdateObjectAction::BulkUpdateObjectAction(const std::vector<LevelObject>& old_objects,
                                               const std::vector<LevelObject>& new_objects)
    : old_objects_(old_objects)
    , new_objects_(new_objects)
{
}

void BulkUpdateObjectAction::execute([[maybe_unused]] EditorState& state, EditorLevel& level)
{
    for (auto& object : new_objects_)
    {
        level.update_object(object, 0);
    }
}

void BulkUpdateObjectAction::undo([[maybe_unused]] EditorState& state, EditorLevel& level)
{
    for (auto& object : old_objects_)
    {
        level.update_object(object, 0);
    }
}

ActionStrings BulkUpdateObjectAction::to_string() const
{
    std::string before;
    std::string after;

    for (auto& object : old_objects_)
    {
        before += object.to_string();
    }
    for (auto& object : new_objects_)
    {
        after += object.to_string();
    }
    return {.title = std::format("Update {}", old_objects_.size()), // to_type_string()),
            .body = std::format("Before:\n{}\n\nAfter:\n{}", before, after)};
}

// =======================================
//      DeleteObjectAction
// =======================================
DeleteObjectAction::DeleteObjectAction(const std::vector<LevelObject>& objects,
                                       const std::vector<int>& floors)
    : objects_(objects)
    , floors_{floors}
{
}

void DeleteObjectAction::execute(EditorState& state, EditorLevel& level)
{
    state.selection.clear_selection();
    for (auto& object : objects_)
    {
        level.remove_object(object.object_id);
    }
}

void DeleteObjectAction::undo(EditorState& state, EditorLevel& level)
{
    state.selection.clear_selection();
    for (int i = 0; i < objects_.size(); i++)
    {
        auto& new_object = level.add_object(objects_[i], floors_[i]);

        level.set_object_id(new_object.object_id, objects_[i].object_id);
        assert(new_object.object_id == objects_[i].object_id);

        objects_[i] = new_object;
        state.selection.add_to_selection(new_object.object_id, floors_[i]);
    }
}

ActionStrings DeleteObjectAction::to_string() const
{
    std::string body;

    for (auto& object : objects_)
    {
        body += object.to_string();
    }

    return {
        .title = std::format("Deleted {}", objects_.size()),
        .body = body,
    };
}
