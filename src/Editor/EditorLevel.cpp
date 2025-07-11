#include "EditorLevel.h"

#include "../Util/Maths.h"
#include "DrawingPad.h"
#include "EditConstants.h"

LevelObject& EditorLevel::add_object(const LevelObject& object)
{
    LevelObject new_object = object;
    new_object.object_id = current_id_++;

    LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object_to_geometry(object),
    };
    level_mesh.mesh.buffer();
    level_meshes_.push_back(std::move(level_mesh));
    return level_objects_.emplace_back(new_object);
}

void EditorLevel::update_object(const LevelObject& object)
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
                  [id](const LevelObject& object) { return object.object_id == id; });
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

void EditorLevel::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object)
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
        else if (auto platform = std::get_if<PlatformObject>(&object.object_type))
        {
            auto is_selected = p_active_object && p_active_object->object_id == object.object_id;

            auto colour = is_selected ? Colour::RED : Colour::WHITE;

            drawing_pad.render_quad(platform->parameters.position,
                {platform->properties.width * TILE_SIZE, platform->properties.depth * TILE_SIZE},
                colour);

        }
    }
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile, const LevelObject* p_active_object)
{
    for (auto& object : level_objects_)
    {
        if (auto wall = std::get_if<WallObject>(&object.object_type))
        {
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
        else if (auto platform = std::get_if<PlatformObject>(&object.object_type))
        {
            const auto& params = platform->parameters;
            const auto& props = platform->properties;

            if (selection_tile.x >= params.position.x &&
                selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
                selection_tile.y >= params.position.y &&
                selection_tile.y <= params.position.y + props.depth * TILE_SIZE)
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