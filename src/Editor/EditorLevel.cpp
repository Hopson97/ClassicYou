#include "EditorLevel.h"

#include <fstream>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "../Util/Maths.h"
#include "../Util/Util.h"
#include "EditConstants.h"
#include "EditorGUI.h"
#include "LevelFileIO.h"

EditorLevel::EditorLevel(const LevelTextures& drawing_pad_texture_map)
    : p_drawing_pad_texture_map_(&drawing_pad_texture_map)
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
    changes_made_since_last_save_ = true;

    LevelObject new_object = object;
    new_object.object_id = current_id_++;

    // Add the 3D mesh
    Floor::LevelMesh level_mesh = {
        .id = new_object.object_id,
        .mesh = object.to_geometry(floor.real_floor),
    };
    level_mesh.mesh.update();
    floor.meshes.push_back(std::move(level_mesh));

    // Add the 2D mesh
    auto [mesh, primitive] = object.to_2d_geometry(*p_drawing_pad_texture_map_);
    Floor::LevelMesh level_mesh_2d = {
        .id = new_object.object_id, .mesh = std::move(mesh), .primitive = primitive};
    level_mesh_2d.mesh.update();
    floor.meshes_2d.push_back(std::move(level_mesh_2d));

    // Return the new object
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

        for (auto& mesh : floor.meshes_2d)
        {
            if (mesh.id == object.object_id)
            {
                mesh.mesh = object.to_2d_geometry(*p_drawing_pad_texture_map_).first;
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
        std::erase_if(floor.meshes_2d, [id](const auto& mesh) { return mesh.id == id; });
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

        for (auto& mesh : floor.meshes_2d)
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
                         int current_floor, const glm::vec3& selected_offset, bool is_debug_render)
{

    std::vector<LevelObjectsMesh3D*> p_active;

    if (!is_debug_render)
    {
        scene_shader.set_uniform("selected", false);
    }

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
    if (!is_debug_render)
    {
        scene_shader.set_uniform("selected", true);
    }
    scene_shader.set_uniform("model_matrix",
                             create_model_matrix({.position = selected_offset / TILE_SIZE_F}));
    for (auto mesh : p_active)
    {
        mesh->bind().draw_elements();
    }
    if (!is_debug_render)
    {
        scene_shader.set_uniform("selected", false);
    }
}

void EditorLevel::render_2d(gl::Shader& scene_shader_2d,
                            const std::vector<ObjectId>& active_objects, int current_floor,
                            const glm::vec2& selected_offset)
{
    auto render_group = [&](const std::vector<Floor::LevelMesh<Mesh2DWorld>*>& group)
    {
        for (auto object : group)
        {
            object->mesh.bind().draw_elements(object->primitive);
        }
    };

    // Sort the objects into groups
    // "current" refers to objects on the current floor
    // "below" refers to objects on the floor below
    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_active;
    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_below;
    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_current;

    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_active_wall;
    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_below_wall;
    std::vector<Floor::LevelMesh<Mesh2DWorld>*> p_current_wall;

    for (auto& floor : floors_manager_.floors)
    {
        // Render floors only from the current floor and below
        if (floor.real_floor == current_floor - 1)
        {
            for (auto& object : floor.meshes_2d)
            {
                object.primitive == gl::PrimitiveType::Lines ? p_below_wall.push_back(&object)
                                                             : p_below.push_back(&object);
            }
        }
        else if (floor.real_floor == current_floor)
        {
            for (auto& object : floor.meshes_2d)
            {
                if (!object.mesh.has_buffered())
                {
                    continue;
                }

                if (contains(active_objects, object.id))
                {
                    object.primitive == gl::PrimitiveType::Lines ? p_active_wall.push_back(&object)
                                                                 : p_active.push_back(&object);
                }
                else
                {
                    object.primitive == gl::PrimitiveType::Lines ? p_current_wall.push_back(&object)
                                                                 : p_current.push_back(&object);
                }
            }
        }
    }

    // Render the objects on the floor below
    scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
    scene_shader_2d.set_uniform("model_matrix", create_model_matrix({}));
    glLineWidth(2);
    scene_shader_2d.set_uniform("is_selected", false);
    scene_shader_2d.set_uniform("on_floor_below", true);

    scene_shader_2d.set_uniform("use_texture", false);
    render_group(p_below_wall);
    scene_shader_2d.set_uniform("use_texture", true);
    render_group(p_below);

    // Render objects on the current floor
    scene_shader_2d.set_uniform("on_floor_below", false);

    scene_shader_2d.set_uniform("use_texture", true);
    render_group(p_current);

    scene_shader_2d.set_uniform("use_texture", false);
    render_group(p_current_wall);

    // Render the selected objects
    glLineWidth(3);
    scene_shader_2d.set_uniform("is_selected", true);
    scene_shader_2d.set_uniform(
        "model_matrix",
        create_model_matrix({.position = {selected_offset.x, selected_offset.y, 0}}));

    scene_shader_2d.set_uniform("use_texture", false);
    render_group(p_active_wall);

    scene_shader_2d.set_uniform("use_texture", true);
    render_group(p_active);
}

void EditorLevel::render_to_picker(gl::Shader& picker_shader, int current_floor)
{
    for (auto& floor : floors_manager_.floors)
    {
        for (auto& object : floor.meshes)
        {
            if (!object.mesh.has_buffered())
            {
                continue;
            }

            picker_shader.set_uniform("object_id", object.id);
            object.mesh.bind().draw_elements();
        }
    }
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
                selection.add_to_selection(object.object_id);
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
    objects.reserve(object_ids.size());
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
    if (auto object = find_object_and_floor(object_id))
    {
        return object->first;
    }
    return nullptr;
}

// TODO - is there is
std::optional<int> EditorLevel::get_object_floor(ObjectId object_id)
{
    if (auto object = find_object_and_floor(object_id))
    {
        return object->second;
    }
    return std::nullopt;
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
    reset_light_settings();
    current_id_ = 0;
    floors_manager_.clear();
}

bool EditorLevel::serialise(LevelFileIO& level_file_io)
{
    if (do_serialise(level_file_io))
    {
        changes_made_since_last_save_ = false;
        return true;
    }
    return false;
}

std::optional<std::pair<LevelObject*, int>> EditorLevel::find_object_and_floor(ObjectId object_id)
{
    for (auto& floor : floors_manager_.floors)
    {
        for (auto& object : floor.objects)
        {
            if (object.object_id == object_id)
                return {{&object, floor.real_floor}};
        }
    }

    return std::nullopt;
}

bool EditorLevel::do_serialise(LevelFileIO& level_file_io) const
{
    auto output = floors_manager_.serialise(level_file_io);

    if (output)
    {
        level_file_io.write_floors(output);
        return true;
    }
    else
    {
        return false;
    }
}

bool EditorLevel::deserialise(const LevelFileIO& level_file_io)
{
    // Clear the current level
    clear_level();

    // Iterate through the floors in the input json
    for (auto& floor_object : level_file_io.get_floors())
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
                     { level_object.deserialise_as<PlatformObject>(json, level_file_io); });

        load_objects(object_types, "wall", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<WallObject>(json, level_file_io); });

        load_objects(object_types, "polygon_platform", floor,
                     [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<PolygonPlatformObject>(json, level_file_io); });

        load_objects(object_types, "pillar", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<PillarObject>(json, level_file_io); });

        load_objects(object_types, "ramp", floor, [&](LevelObject& level_object, auto& json)
                     { level_object.deserialise_as<RampObject>(json, level_file_io); });
    }

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

void EditorLevel::display_settings_gui()
{
    if (ImGui::Begin("Level Settings"))
    {
        ImGui::Text("Main Light");
        ImGui::SliderFloat3("Position", &main_light_.position[0], -100, 100);
        ImGui::SliderFloat("Brightness", &main_light_.brightness, 0.1, 2.0);

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Level Colour Settings"))
        {
            ImGui::ColorPicker3("Main Light Colour", &main_light_.colour[0]);
            if (ImGui::ColorPicker3("Sky Colour", &main_light_.sky_colour[0]))
            {
                glClearColor(main_light_.sky_colour.r, main_light_.sky_colour.g,
                             main_light_.sky_colour.b, 1.0f);
            }
        }
    }
    ImGui::End();
}

void EditorLevel::reset_light_settings()
{
    main_light_ = MainLight{};
}

const EditorLevel::MainLight& EditorLevel::get_light_settings() const
{
    return main_light_;
}
