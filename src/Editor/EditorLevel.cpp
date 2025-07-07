#include "EditorLevel.h"

#include <print>

EditorLevel::EditorLevel(const EditorState& state)
    : p_editor_state_(&state)
{
}

Wall& EditorLevel::add_wall(const WallParameters& paramters)
{
    // auto wall = std::make_unique<Wall>(current_id_++);
    // wall->parameters = paramters;
    // wall->props = p_editor_state_->wall_default;

    // auto& object = *level_objects.emplace_back(std::move(wall));
    // for (auto& callback : on_create_object_)
    //{
    //     callback(object);
    // }

    Wall wall{current_id_++};
    wall.parameters = paramters;
    wall.props = p_editor_state_->wall_default;

    LevelMesh level_mesh = {
        .id = wall.object_id,
        .mesh =
            generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                               wall.props.texture_side_1.value, wall.props.texture_side_2.value),
    };
    level_mesh.mesh.buffer();
    wall_meshes_.push_back(std::move(level_mesh));

    for (auto& callback : on_create_wall_)
    {
        callback(wall);
    }
    return walls.emplace_back(wall);
}

void EditorLevel::update_object(const Wall& wall)
{
    for (auto& wall_mesh : wall_meshes_)
    {
        if (wall_mesh.id == wall.object_id)
        {
            auto new_mesh = generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                                               wall.props.texture_side_1.value,
                                               wall.props.texture_side_2.value);
            new_mesh.buffer();
            wall_mesh.mesh = std::move(new_mesh);
        }
    }
}

void EditorLevel::remove_object(std::size_t id)
{

    std::erase_if(wall_meshes_, [id](const LevelMesh& mesh) { return mesh.id == id; });
    std::erase_if(walls, [id](const Wall& wall) { return wall.object_id == id; });
}

void EditorLevel::on_add_object(std::function<void(Wall& object)> callback)
{
    on_create_wall_.push_back(callback);
}

void EditorLevel::render()
{
    for (auto& wall : wall_meshes_)
    {
        wall.mesh.bind().draw_elements();
    }
}
