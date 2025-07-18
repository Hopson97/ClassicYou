#include "EditorLevel.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include "../Util/Maths.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "EditorGUI.h"

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

    return add_object(object, floor);
}

LevelObject& EditorLevel::add_object(const LevelObject& object, Floor& floor)
{
    LevelObject new_object = object;
    new_object.object_id = current_id_++;

    LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object.to_geometry(floor.real_floor),
    };
    level_mesh.mesh.buffer();
    floor.meshes.push_back(std::move(level_mesh));
    changes_made_since_last_save_ = true;

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
    changes_made_since_last_save_ = true;
}

void EditorLevel::remove_object(std::size_t id)
{
    for (auto& floor : floors_)
    {
        std::erase_if(floor.meshes, [id](const auto& mesh) { return mesh.id == id; });
        std::erase_if(floor.objects, [id](const auto& object) { return object.object_id == id; });
    }
    changes_made_since_last_save_ = true;
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

EditorLevel::Floor& EditorLevel::ensure_floor_exists(int floor_number)
{
    if (floors_.empty())
    {
        auto& floor = floors_.emplace_back(floor_number);
        min_floor_ = floor_number;
        max_floor_ = floor_number;
        return floor;
    }
    else
    {
        if (floor_number < get_min_floor())
        {
            auto& floor = floors_.emplace_back(--min_floor_);
            return floor;
        }
        else if (floor_number > get_max_floor())
        {
            auto& floor = floors_.emplace_back(++max_floor_);
            return floor;
        }
    }

    auto floor = find_floor(floor_number);
    if (!floor)
    {
        throw std::runtime_error("Trying to add floors failed, but the floor doesn't exist");
    }
    return **floor;
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

void EditorLevel::clear_level()
{
    floors_.clear();
    current_id_ = 0;
    min_floor_ = 0;
    max_floor_ = 0;
}

bool EditorLevel::save(const std::filesystem::path& path)
{
    if (do_save(path))
    {
        changes_made_since_last_save_ = false;
        return true;
    }
    return false;
}

bool EditorLevel::do_save(const std::filesystem::path& path) const
{
    nlohmann::json output;
    output["version"] = 1;
    output["floors"] = {};

    // Floors are saved from bottom to top
    for (int floor_number = min_floor_; floor_number < max_floor_ + 1; floor_number++)
    {
        auto floor_opt = find_floor(floor_number);
        if (!floor_opt)
        {
            std::println(std::cerr, "Could not save floor {} as it does not exist", floor_number);
            return false;
        }
        auto& floor = **floor_opt;

        // Objects are grouped together by their type to optimize the json
        std::unordered_map<std::string, nlohmann::json> object_map;

        // Create a json object for the current floor
        nlohmann::json current_floor;
        current_floor["floor"] = floor_number;
        current_floor["objects"] = {};

        // Iterate through all objects on the floor and group them by type
        for (auto& object : floor.objects)
        {
            auto [data, type] = object.serialise();
            if (object_map.find(type) == object_map.end())
            {
                object_map[type] = {};
            }
            object_map[type].push_back(data);
        }

        // Add the grouped objects to the current floor json
        current_floor["objects"] = object_map;
        output["floors"].push_back(current_floor);
    }

    // Write the output to the file
    std::ofstream output_file(path);
    output_file << output << std::endl;
    std::println("Level has been saved to: {}", path.string());
    return true;
}

bool EditorLevel::load(const std::filesystem::path& path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        std::println(std::cerr, "Could not open file {}", path.string());
        return false;
    }

    auto input = nlohmann::json::parse(f);

    // Clear the current level
    clear_level();

    // Iterate through the floors in the input json
    for (auto& floor_object : input["floors"])
    {
        if (!floor_object.contains("floor") || !floor_object.contains("objects"))
        {
            std::println(std::cerr, "Invalid floor object in level file");
            return false;
        }

        int floor_number = floor_object["floor"];
        auto& floor = ensure_floor_exists(floor_number);

        // Load the objects for the current floor
        auto object_types = floor_object["objects"];

        load_objects(object_types, "platform", floor, [&](auto& level_object, auto& json)
                     { level_object.deserialise_as_platform(json); });

        load_objects(object_types, "wall", floor, [&](auto& level_object, auto& json)
                     { level_object.deserialise_as_wall(json); });

        load_objects(object_types, "polygon_platform", floor,
                     [&](auto& level_object, auto& json)
                     { level_object.deserialise_as_polygon_platform(json); });
    }

    changes_made_since_last_save_ = false;
    return true;
}

bool EditorLevel::changes_made_since_last_save() const
{
    return changes_made_since_last_save_;
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

std::optional<const EditorLevel::Floor*> EditorLevel::find_floor(int floor_number) const
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
