#pragma once

#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include "../Editor/LevelObject.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"
#include "FloorManager.h"

class DrawingPad;

class EditorLevel
{
  public:
    EditorLevel();

    LevelObject& add_object(const LevelObject& object, int floor_number);
    LevelObject& add_object(const LevelObject& object, Floor& floor);

    void update_object(const LevelObject& object, int floor_number);
    void remove_object(std::size_t id);

    void set_object_id(ObjectId current_id, ObjectId new_id);

    /**
     * @brief Render the level in 3D.
     * Assumes the camera, shader, and other OpenGL states are set up correctly.
     * @param current_floor The current floor being used in the editor
     */
    void render(gl::Shader& scene_shader, const LevelObject* p_active_object, int current_floor);

    /**
     * @brief Render the level in 2D using the given drawing pad.
     * Assumes the camera, shader, and other OpenGL states are set up correctly.
     *
     * @param drawing_pad The drawing pad to render the level on.
     * @param current_floor The current floor being used in the editor
     */
    void render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object, int current_floor);

    /**
     * @brief Try to select a level object at the given tile position.
     *
     * @param selection_tile The tile position to select an object at.
     * @param p_active_object The currently active object. Used to ensure the object does not get
     * selected twice
     * @param current_floor The current floor being used in the editor
     * @return LevelObject* if an object is found at the given tile, otherwise nullptr.
     */
    LevelObject* try_select(glm::vec2 selection_tile, const LevelObject* p_active_object,
                            int current_floor);

    int get_min_floor() const;
    int get_max_floor() const;
    size_t get_floor_count() const;
    void ensure_floor_exists(int floor_number);

    void clear_level();

    bool save(const std::filesystem::path& path);
    bool load(const std::filesystem::path& path);

    bool changes_made_since_last_save() const;

  private:
    bool do_save(const std::filesystem::path& path) const;

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
