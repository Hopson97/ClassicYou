#include "EditorLevel.h"

#include "DrawingPad.h"
#include "EditConstants.h"

Wall& EditorLevel::add_wall(const WallParameters& parameters, const WallProps& props)
{
    Wall wall{current_id_++};
    wall.parameters = parameters;
    wall.props = props;

    LevelMesh level_mesh = {
        .id = wall.object_id,
        .mesh =
            generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                               wall.props.texture_side_1.value, wall.props.texture_side_2.value),
    };
    level_mesh.mesh.buffer();
    wall_meshes_.push_back(std::move(level_mesh));

    return walls_.emplace_back(wall);
}

void EditorLevel::update_object(const Wall& wall)
{
    std::println("Updating wall with id: {}", wall.object_id);
    for (auto& w : walls_)
    {
        std::println("Checking wall with id: {}", w.object_id);
        if (w.object_id == wall.object_id)
        {
            std::println("Found wall with id: {}", w.object_id);
            w = wall;
            break;
        }
    }

    std::println("Updating wall mesh for id: {}", wall.object_id);
    for (auto& wall_mesh : wall_meshes_)
    {
        std::println("Checking wall mesh with id: {}", wall_mesh.id);
        if (wall_mesh.id == wall.object_id)
        {
            std::println("Found wall mesh with id: {}", wall_mesh.id);
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
    std::erase_if(walls_, [id](const Wall& wall) { return wall.object_id == id; });
}

void EditorLevel::set_object_id(ObjectId current_id, ObjectId new_id)
{
    for (auto& wall : walls_)
    {
        std::println("Trying to set object id: {} -> {}", current_id, new_id);
        {
            wall.object_id = new_id;
            break;
        }
    }
    for (auto& wall_mesh : wall_meshes_)
    {
        if (wall_mesh.id == current_id)
        {
            wall_mesh.id = new_id;
            break;
        }
    }
}

void EditorLevel::render()
{
    for (auto& wall : wall_meshes_)
    {
        wall.mesh.bind().draw_elements();
    }
}

void EditorLevel::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object)
{
    for (auto& wall : walls_)
    {
        auto is_selected = p_active_object && p_active_object->object_id == wall.object_id;

        auto colour = is_selected ? Colour::RED : Colour::WHITE;
        auto thickness = is_selected ? 3 : 2;

        drawing_pad.render_line(wall.parameters.start, wall.parameters.end, colour, thickness);
    }
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile, const LevelObject* p_active_object)
{
    for (auto& wall : walls_)
    {
        if (wall.try_select_2d(selection_tile))
        {
            // Allow selecting objects that may be overlapping
            if (!p_active_object || p_active_object->object_id != wall.object_id)
            {
                return &wall;
            }
        }
    }
    return nullptr;
}
