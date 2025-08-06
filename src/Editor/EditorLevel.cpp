#include "EditorLevel.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include "../Util/Maths.h"
#include "../Util/Util.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "EditorGUI.h"

EditorLevel::EditorLevel()
{
    floors_manager_.ensure_floor_exists(0);
}

LevelObject& EditorLevel::add_object(const LevelObject& object, int floor_number)
{
    auto floor_opt = floors_manager_.find_floor(floor_number);
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

    Floor::LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object.to_geometry(floor.real_floor),
    };
    level_mesh.mesh.update();
    floor.meshes.push_back(std::move(level_mesh));
    changes_made_since_last_save_ = true;

    return floor.objects.emplace_back(new_object);
}

void EditorLevel::update_object(const LevelObject& object, int floor_number)
{
    for (auto& floor : floors_manager_.floors)
    {
        for (auto& mesh : floor.meshes)
        {

            if (mesh.id == object.object_id)
            {
                mesh.mesh = object.to_geometry(floor.real_floor);
                mesh.mesh.update();
                break;
            }
        }

        // Copy the new object to the old object
        for (auto& old_object : floor.objects)
        {
            if (old_object.object_id == object.object_id)
            {
                old_object = object;
                break;
            }
        }
    }
    changes_made_since_last_save_ = true;
}

void EditorLevel::remove_object(ObjectId id)
{
    for (auto& floor : floors_manager_.floors)
    {
        std::erase_if(floor.meshes, [id](const auto& mesh) { return mesh.id == id; });
        std::erase_if(floor.objects, [id](const auto& object) { return object.object_id == id; });
    }
    changes_made_since_last_save_ = true;
}

void EditorLevel::set_object_id(ObjectId current_id, ObjectId new_id)
{
    for (auto& floor : floors_manager_.floors)
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

void EditorLevel::render(gl::Shader& scene_shader, const std::vector<ObjectId>& active_objects,
                         int current_floor, const glm::vec3& selected_offset)
{
    std::vector<LevelObjectsMesh3D*> p_active;

    scene_shader.set_uniform("selected", false);

    for (auto& floor : floors_manager_.floors)
    {
        // Render floors only from the current floor and below
        if (floor.real_floor > current_floor)
        {
            // continue;
        }

        for (auto& object : floor.meshes)
        {
            if (!object.mesh.has_buffered())
            {
                continue;
            }

            if (contains(active_objects, object.id))
            {
                p_active.push_back(&object.mesh);
            }
            else
            {
                object.mesh.bind().draw_elements();
            }
        }
    }

    // Render the selected object seperate
    scene_shader.set_uniform("selected", true);
    scene_shader.set_uniform("model_matrix",
                             create_model_matrix({.position = selected_offset / TILE_SIZE_F}));
    for (auto mesh : p_active)
    {
        mesh->bind().draw_elements();
    }
    scene_shader.set_uniform("selected", false);
}

void EditorLevel::render_2d(DrawingPad& drawing_pad, const std::vector<ObjectId>& active_objects,
                            int current_floor, const glm::vec2& selected_offset)
{
    // Group lower floor objects to ensure it is always rendered under current floor objects
    // and ensuring selected objects always render on-top
    static std::vector<const LevelObject*> below_group;
    static std::vector<const LevelObject*> below_group_selected;
    static std::vector<const LevelObject*> current_floor_group;
    static std::vector<const LevelObject*> current_floor_group_selected;
    below_group.clear();
    below_group_selected.clear();
    current_floor_group.clear();
    current_floor_group_selected.clear();

    // Render a group of objects, controlling the clour. Objects not on the current floor are
    // renderd using a greyed-out colour
    auto draw_group = [&](const std::vector<const LevelObject*>& group, bool is_current_floor,
                          bool is_selected_objects_group)
    {
        for (auto object : group)
        {
            auto offset = is_selected_objects_group ? selected_offset : glm::vec2{0};
            object->render_2d(drawing_pad, is_current_floor, is_selected_objects_group, offset);
        }
    };

    // Sort a floor of objects to determine if they are selected or not.
    auto sort_to_groups = [&](const std::vector<LevelObject>& objects,
                              std::vector<const LevelObject*>& not_selected,
                              std::vector<const LevelObject*>& selected)
    {
        for (auto& object : objects)
        {
            if (contains(active_objects, object.object_id))
            {
                selected.push_back(&object);
            }
            else
            {
                not_selected.push_back(&object);
            }
        }
    };

    for (auto& floor : floors_manager_.floors)
    {
        // Only render the current floor and the floor below
        if (floor.real_floor == current_floor)
        {
            sort_to_groups(floor.objects, current_floor_group, current_floor_group_selected);
        }
        else if (current_floor == floor.real_floor + 1)
        {
            sort_to_groups(floor.objects, below_group, below_group_selected);
        }
    }

    // In 2D, objects rendered first are rendered ontop of objects rendered after.
    // So the drawing order is:
    //  1. Current floor selected objects
    //  2. Current floor NON selected objects
    //  3. Below floor NON selected objects
    //  4. Below floor NON selected objects

    // The current floor is rendered using a white colour
    draw_group(current_floor_group_selected, true, true);
    draw_group(current_floor_group, true, false);

    // The floor below is rendered using a greyish colour is used to make it obvious it is the floor
    // below
    draw_group(below_group_selected, false, true);
    draw_group(below_group, false, false);
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile, const LevelObject* p_active_object,
                                     int current_floor)
{
    for (auto& floor : floors_manager_.floors)
    {
        // 2D Selection can only happen for objects on the current floor
        if (floor.real_floor != current_floor)
        {
            continue;
        }

        for (auto& object : floor.objects)
        {
            if (object.try_select_2d(selection_tile))
            {
                if (p_active_object && p_active_object->object_id == object.object_id)
                {
                    continue;
                }
                return &object;
            }
        }

        // Found the right floor so can exit early
        break;
    }
    return nullptr;
}

void EditorLevel::select_within(const Rectangle& selection_area, Selection& selection,
                                int floor_number)
{
    for (auto& floor : floors_manager_.floors)
    {
        if (floor.real_floor != floor_number)
        {
            continue;
        }

        for (auto& object : floor.objects)
        {
            if (object.is_within(selection_area))
            {
                selection.add_to_selection(object.object_id, floor_number);
            }
        }

        // Found the right floor so can exit early
        break;
    }
}

std::vector<LevelObject*> EditorLevel::get_objects(const std::vector<ObjectId>& object_ids)
{
    // Find objects with the given ID - maybe there is better way than a triple nested loop?
    std::vector<LevelObject*> objects;
    for (auto& id : object_ids)
    {
        bool found = false;
        for (auto& floor : floors_manager_.floors)
        {
            for (auto& object : floor.objects)
            {
                if (object.object_id == id)
                {
                    objects.push_back(&object);
                    found = true;
                    break;
                }
            }

            if (found)
            {
                break;
            }
        }
    }
    return objects;
}

LevelObject* EditorLevel::get_object(ObjectId object_id)
{
    auto object = get_objects({object_id});
    if (!object.empty())
    {
        return object[0];
    }
    return nullptr;
}

std::pair<std::vector<LevelObject>, std::vector<int>>
EditorLevel::copy_objects_and_floors(const std::vector<ObjectId>& object_ids) const
{
    // Copy objects with the given ID - maybe there is better way than a triple nested loop?
    std::vector<LevelObject> objects;
    std::vector<int> floors;
    for (auto& id : object_ids)
    {
        bool found = false;
        for (auto& floor : floors_manager_.floors)
        {
            for (auto& object : floor.objects)
            {
                if (object.object_id == id)
                {
                    objects.push_back(object);
                    floors.push_back(floor.real_floor);
                    found = true;
                    break;
                }
            }

            if (found)
            {
                break;
            }
        }
    }

    return {objects, floors};
}

int EditorLevel::get_min_floor() const
{
    return floors_manager_.min_floor;
}

int EditorLevel::get_max_floor() const
{
    return floors_manager_.max_floor;
}

size_t EditorLevel::get_floor_count() const
{
    return floors_manager_.floors.size();
}

void EditorLevel::ensure_floor_exists(int floor_number)
{
    floors_manager_.ensure_floor_exists(floor_number);
}

void EditorLevel::clear_level()
{

    current_id_ = 0;
    floors_manager_.clear();
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
    auto output = floors_manager_.serialise();

    if (output)
    {
        // Write the output to the file
        std::ofstream output_file(path);
        output_file << output << std::endl;
        std::println("Level has been saved to: {}", path.string());
        return true;
    }
    else
    {
        return false;
    }
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
        auto& floor = floors_manager_.ensure_floor_exists(floor_number);

        // Load the objects for the current floor
        auto object_types = floor_object["objects"];

        load_objects(object_types, "platform", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<PlatformObject>(json); });

        load_objects(object_types, "wall", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<WallObject>(json); });

        load_objects(object_types, "polygon_platform", floor,
                     [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<PolygonPlatformObject>(json); });

        load_objects(object_types, "pillar", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<PillarObject>(json); });

        load_objects(object_types, "ramp", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<RampObject>(json); });
    }

    std::println("Successfully loaded {} ", path.string());
    changes_made_since_last_save_ = false;
    return true;
}

bool EditorLevel::changes_made_since_last_save() const
{
    return changes_made_since_last_save_;
}

int EditorLevel::last_placed_id() const
{
    return current_id_ - 1;
}
