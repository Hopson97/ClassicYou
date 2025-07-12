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
        .mesh = object.to_geometry(),
    };
    level_mesh.mesh.buffer();
    level_meshes_.push_back(std::move(level_mesh));
    return level_objects_.emplace_back(new_object);
}

void EditorLevel::update_object(const LevelObject& object)
{
    for (auto& w : level_objects_)
    {
        std::println("Updating object: {} -> {}", w.object_id, object.object_id);
        if (w.object_id == object.object_id)
        {
            std::println("Found object ID: {}", w.object_id);
            w = object;
            break;
        }
    }

    for (auto& wall_mesh : level_meshes_)
    {
        std::println("Updating mesh: {} -> {}", wall_mesh.id, object.object_id);
        if (wall_mesh.id == object.object_id)
        {
            std::println("Found mesh for object ID: {}", wall_mesh.id);
            auto new_mesh = object.to_geometry();
            new_mesh.buffer();
            wall_mesh.mesh = std::move(new_mesh);
            break;
        }
    }
    std::println("Size of level objects: {}", level_objects_.size());
    std::println("Size of level meshes: {}", level_meshes_.size());
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
        object.render_2d(drawing_pad, p_active_object);
    }
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile, const LevelObject* p_active_object)
{
    for (auto& object : level_objects_)
    {
        if (object.try_select_2d(selection_tile, p_active_object))
        {
            return &object;
        }
    }
    return nullptr;
}