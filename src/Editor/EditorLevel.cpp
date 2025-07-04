#include "EditorLevel.h"

#include <print>


EditorLevel::EditorLevel(const EditorState& state)
    : p_editor_state_(&state)
{
}


Wall& EditorLevel::add_wall(const WallParameters& paramters)
{
    //auto wall = std::make_unique<Wall>(current_id_++);
    //wall->parameters = paramters;
    //wall->props = p_editor_state_->wall_default;

    //auto& object = *level_objects.emplace_back(std::move(wall));
    //for (auto& callback : on_create_object_)
    //{
    //    callback(object);
    //}

    Wall wall{current_id_++};
    wall.parameters = paramters;
    wall.props = p_editor_state_->wall_default;

    for (auto& callback : on_create_wall_)
    {
        callback(wall);
    }
    return walls.emplace_back(wall);

}

void EditorLevel::on_add_object(std::function<void(Wall& object)> callback)
{
    on_create_wall_.push_back(callback);
}
