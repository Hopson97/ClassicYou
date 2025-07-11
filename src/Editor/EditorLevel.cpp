#include "EditorLevel.h"

#include "../Util/Maths.h"
#include "DrawingPad.h"
#include "EditConstants.h"

LevelObjectV2& EditorLevel::add_object(const LevelObjectV2& object)
{
    LevelObjectV2 new_object = object;
    new_object.object_id = current_id_++;

    LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object_to_geometry(object),
    };
    level_mesh.mesh.buffer();
    level_meshes_.push_back(std::move(level_mesh));

    std::println("Added object with id: {} and {}", new_object.object_id,
                 object_to_string(new_object));
    return level_objects_.emplace_back(new_object);
}

void EditorLevel::update_object(const LevelObjectV2& object)
{
    for (auto& w : level_objects_)
    {
        if (w.object_id == object.object_id)
        {
            w = object;
            break;
        }
    }

    for (auto& wall_mesh : level_meshes_)
    {
        if (wall_mesh.id == object.object_id)
        {
            auto new_mesh = object_to_geometry(object);
            new_mesh.buffer();
            wall_mesh.mesh = std::move(new_mesh);
        }
    }
}

void EditorLevel::remove_object(std::size_t id)
{
    std::erase_if(level_meshes_, [id](const LevelMesh& mesh) { return mesh.id == id; });
    std::erase_if(level_objects_,
                  [id](const LevelObjectV2& object) { return object.object_id == id; });
}

void EditorLevel::set_object_id(ObjectId current_id, ObjectId new_id)
{
    for (auto& object : level_objects_)
    {
        if (object.object_id == current_id)
        {
            object.object_id = new_id;
            break;
        }
    }
    for (auto& mesh : level_meshes_)
    {
        if (mesh.id == current_id)
        {
            mesh.id = new_id;
            break;
        }
    }
}

void EditorLevel::render()
{
    for (auto& object : level_meshes_)
    {
        object.mesh.bind().draw_elements();
    }
}

void EditorLevel::render_2d(DrawingPad& drawing_pad, const LevelObjectV2* p_active_object)
{
    for (auto& object : level_objects_)
    {
        if (auto wall = std::get_if<WallObject>(&object.object_type))
        {
            auto is_selected = p_active_object && p_active_object->object_id == object.object_id;

            auto colour = is_selected ? Colour::RED : Colour::WHITE;
            auto thickness = is_selected ? 3.0f : 2.0f;

            drawing_pad.render_line(wall->parameters.start, wall->parameters.end, colour,
                                    thickness);
        }
    }
}

LevelObjectV2* EditorLevel::try_select(glm::vec2 selection_tile,
                                       const LevelObjectV2* p_active_object)
{
    std::println("Trying to select object at tile: ({}, {}) with {} objects currently stored",
                 selection_tile.x, selection_tile.y, level_objects_.size());

    for (auto& object : level_objects_)
    {
        std::println("Checking object with id: {}", object.object_id);
        if (auto wall = std::get_if<WallObject>(&object.object_type))
        {
            std::println("Checking wall with id: {}", object.object_id);
            const auto& params = wall->parameters;
            if (distance_to_line(selection_tile, {params.start, params.end}) < 15)
            {
                // Allow selecting objects that may be overlapping
                if (!p_active_object || p_active_object->object_id != object.object_id)
                {
                    return &object;
                }
            }
        }
    }
    return nullptr;
}