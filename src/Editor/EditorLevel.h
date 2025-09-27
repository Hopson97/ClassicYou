#pragma once

#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"
#include "EditorState.h"
#include "FloorManager.h"
#include "LevelObjects/LevelObject.h"

class LevelFileIO;
class LevelTextures;

/**
 * @brief The editor representation of a level.
 *
 * Objects are stored on floors, and each floor can have multiple objects. Hence most functions take
 * in the current_floor number, which is the floor number where objects should be manipulated.
 */
class EditorLevel
{
  public:
    EditorLevel(const LevelTextures& drawing_pad_texture_map);

    LevelObject& add_object(const LevelObject& object, int floor_number);
    LevelObject& add_object(const LevelObject& object, Floor& floor);

    /// Update an object. This looks up the matching object ID and replaces it with the given
    /// 'object'
    void update_object(const LevelObject& object, int floor_number);

    void remove_object(ObjectId id);

    void set_object_id(ObjectId current_id, ObjectId new_id);

    /// Renders the level in 3D using the given shader and highlights the active object.
    /// Assumes the camera, shader, and other OpenGL states are set up correctly.
    /// The "selected_offset" can be used to offset the selected objects
    void render(gl::Shader& scene_shader, const std::vector<ObjectId>& active_objects,
                int current_floor, const glm::vec3& selected_offset, bool is_debug_render);

    /// Render the 2D view of the level using the given shader and highlight the active object.
    /// Assumes the camera, shader, and other OpenGL states are set up correctly.
    void render_2d(gl::Shader& scene_shader, const std::vector<ObjectId>& active_objects,
                   int current_floor, const glm::vec2& selected_offset);

    // Render the scene to the picker texture, using the object ID as the single-colour channel
    void render_to_picker(gl::Shader& picker_shader, int current_floor);

    /// Try to select a level object at the given tile position. Returns nullptr if no object is
    /// found.
    LevelObject* try_select(glm::vec2 selection_tile, const LevelObject* p_active_object,
                            int current_floor);

    /// Try to select all objects at the given position, and put into the given set
    void select_within(const Rectangle& selection_area, Selection& selection, int floor_number);

    std::vector<LevelObject*> get_objects(const std::vector<ObjectId>& object_ids);
    LevelObject* get_object(ObjectId object_id);
    std::optional<int> get_object_floor(ObjectId object_id);

    std::pair<std::vector<LevelObject>, std::vector<int>>
    copy_objects_and_floors(const std::vector<ObjectId>& object_ids) const;

    int get_min_floor() const;
    int get_max_floor() const;
    size_t get_floor_count() const;
    void ensure_floor_exists(int floor_number);

    /// @brief  Clears all floors and their objects, and resetting the level.
    void clear_level();

    bool serialise(LevelFileIO& level_file_io);
    bool deserialise(const LevelFileIO& level_file_io);

    bool changes_made_since_last_save() const;

    /// Returns the ID of the last object placed
    int last_placed_id() const;

  private:
    std::optional<std::pair<LevelObject*, int>> find_object_and_floor(ObjectId object_id);

    bool do_serialise(LevelFileIO& level_file_io) const;

    /// Loads the level from the given JSON object, where "LoadFunc" should be a function
    /// deserialises the given json to an object.
    template <typename LoadFunc>
    void load_objects(nlohmann::json& json, const char* object_key, Floor& floor, LoadFunc func)
    {
        if (json.find(object_key) != json.end())
        {
            for (auto& object : json[object_key])
            {
                LevelObject level_object{0};
                func(level_object, object);
                add_object(level_object, floor);
            }
        }
    }

  private:
    FloorManager floors_manager_;

    /// Keeps track of the current object id, increments with each object added
    ObjectId current_id_ = 0;

    bool changes_made_since_last_save_ = false;

    const LevelTextures* p_drawing_pad_texture_map_ = nullptr;
};
