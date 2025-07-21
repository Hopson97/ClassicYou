#pragma once

#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include "../Editor/LevelObject.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"
#include "FloorManager.h"
#include <unordered_set>

class DrawingPad;

/**
 * @brief The editor representation of a level.
 *
 * Objects are stored on floors, and each floor can have multiple objects. Hence most functions take
 * in the current_floor number, which is the floor number where objects should be manipulated.
 */
class EditorLevel
{
  public:
    EditorLevel();

    LevelObject& add_object(const LevelObject& object, int floor_number);
    LevelObject& add_object(const LevelObject& object, Floor& floor);

    void update_object(const LevelObject& object, int floor_number);
    void remove_object(ObjectId id);

    void set_object_id(ObjectId current_id, ObjectId new_id);

    /// Renders the level in 3D using the given shader and highlights the active object.
    /// Assumes the camera, shader, and other OpenGL states are set up correctly.
    void render(gl::Shader& scene_shader, const LevelObject* p_active_object, int current_floor);

    /// Render the level in 2D using the given drawing pad, highlighting the active object.
    /// Assumes the camera, shader, and other OpenGL states are set up correctly.
    void render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object, int current_floor);

    /// Try to select a level object at the given tile position. Returns nullptr if no object is
    /// found.
    LevelObject* try_select(glm::vec2 selection_tile, const LevelObject* p_active_object,
                            int current_floor);

    /// Try to select all objects at the given position, and put into the given set
    void try_select_all(const Rectangle& selection_area, int current_floor,
                        std::unordered_set<LevelObject*>& objects);

    int get_min_floor() const;
    int get_max_floor() const;
    size_t get_floor_count() const;
    void ensure_floor_exists(int floor_number);

    /// @brief  Clears all floors and their objects, and resetting the level.
    void clear_level();

    bool save(const std::filesystem::path& path);
    bool load(const std::filesystem::path& path);

    bool changes_made_since_last_save() const;

  private:
    bool do_save(const std::filesystem::path& path) const;

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
    int current_id_ = 0;

    bool changes_made_since_last_save_ = false;
};
