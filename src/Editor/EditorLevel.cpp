#include "EditorLevel.h"

#include "../Util/Maths.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include <numeric>

EditorLevel::EditorLevel()
{
    ensure_floor_exists(0);
}

LevelObject& EditorLevel::add_object(const LevelObject& object, int floor_number)
{
    auto floor_opt = find_floor(floor_number);
    if (!floor_opt)
    {
        throw std::runtime_error("Floor does not exist");
    }
    auto& floor = **floor_opt;

    LevelObject new_object = object;
    new_object.object_id = current_id_++;

    LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object.to_geometry(floor_number),
    };
    level_mesh.mesh.buffer();
    floor.meshes.push_back(std::move(level_mesh));
    return floor.objects.emplace_back(new_object);
}

void EditorLevel::update_object(const LevelObject& object, int floor_number)
{
    for (auto& floor : floors_)
    {
        for (auto& mesh : floor.meshes)
        {

            if (mesh.id == object.object_id)
            {
                auto new_mesh = object.to_geometry(floor_number);
                new_mesh.buffer();
                mesh.mesh = std::move(new_mesh);
                break;
            }
        }
        for (auto& w : floor.objects)
        {
            if (w.object_id == object.object_id)
            {
                w = object;
                break;
            }
        }
    }
}

void EditorLevel::remove_object(std::size_t id)
{
    for (auto& floor : floors_)
    {
        std::erase_if(floor.meshes, [id](const auto& mesh) { return mesh.id == id; });
        std::erase_if(floor.objects, [id](const auto& object) { return object.object_id == id; });
    }
}

void EditorLevel::set_object_id(ObjectId current_id, ObjectId new_id)
{
    for (auto& floor : floors_)
    {
        for (auto& mesh : floor.meshes)
        {
            if (mesh.id == current_id)
            {
                mesh.id = new_id;
                break;
            }
        }

        for (auto& object : floor.objects)
        {
            if (object.object_id == current_id)
            {
                object.object_id = new_id;
                break;
            }
        }
    }
}

void EditorLevel::render(gl::Shader& scene_shader, const LevelObject* p_active_object,
                         int current_floor)
{
    bool rendered_selected = false;
    scene_shader.set_uniform("selected", false);

    for (auto& floor : floors_)
    {
        // Render floors only from the current floor and below
        if (floor.real_floor > current_floor)
        {
            // continue;
        }

        for (auto& object : floor.meshes)
        {

            // Switch prevents setting the uniform needlessly many times
            if (!rendered_selected && p_active_object && object.id == p_active_object->object_id)
            {
                rendered_selected = true;
                scene_shader.set_uniform("selected", true);
                object.mesh.bind().draw_elements();
                scene_shader.set_uniform("selected", false);
            }
            else
            {
                object.mesh.bind().draw_elements();
            }
        }
    }
}

void EditorLevel::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object,
                            int current_floor)
{
    for (auto& floor : floors_)
    {
        // Only render the current floor and the floor below
        if (floor.real_floor == current_floor || (current_floor == floor.real_floor + 1))
        {
            for (auto& object : floor.objects)
            {
                // The current floor should be rendered using full colour, otherwise a more grey
                // colour is used to make it obvious it is the floor below
                object.render_2d(drawing_pad, p_active_object, floor.real_floor == current_floor);
            }
        }
    }
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile, const LevelObject* p_active_object,
                                     int current_floor)
{
    for (auto& floor : floors_)
    {
        // Only render the current floor and the floor below!
        if (floor.real_floor == current_floor)
        {
            for (auto& object : floor.objects)
            {
                if (object.try_select_2d(selection_tile, p_active_object))
                {
                    return &object;
                }
            }
        }
    }
    return nullptr;
}

void EditorLevel::ensure_floor_exists(int floor_number)
{
    if (floors_.empty())
    {
        auto& floor = floors_.emplace_back();
        floor.real_floor = 0;
    }
    else
    {
        if (floor_number < get_min_floor())
        {
            auto& floor = floors_.emplace_back();
            floor.real_floor = --min_floor_;
        }
        else if (floor_number > get_max_floor())
        {
            auto& floor = floors_.emplace_back();
            floor.real_floor = ++max_floor_;
        }
    }
}

int EditorLevel::get_min_floor() const
{
    return min_floor_;
}

int EditorLevel::get_max_floor() const
{
    return max_floor_;
}

size_t EditorLevel::get_floor_count() const
{
    return floors_.size();
}

std::optional<EditorLevel::Floor*> EditorLevel::find_floor(int floor_number)
{
    for (auto& floor : floors_)
    {
        if (floor.real_floor == floor_number)
        {
            return &floor;
        }
    }
    return {};
}
