#pragma once

#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include "../Editor/LevelObject.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"

class DrawingPad;

using ObjectId = std::int32_t;

class EditorLevel
{
    struct LevelMesh
    {
        ObjectId id;
        LevelObjectsMesh3D mesh;
    };

    struct Floor
    {
        // The vector floors do not need to be in order, to allow easier "negative" floors.
        int real_floor = 0;

        std::vector<LevelObject> objects;
        std::vector<LevelMesh> meshes;
    };

  public:
    EditorLevel();

    // Wall& add_wall(const WallParameters& parameters, const WallProps& props);
    LevelObject& add_object(const LevelObject& object, int floor_number);

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

    Floor& ensure_floor_exists(int floor_number);

    int get_min_floor() const;
    int get_max_floor() const;
    size_t get_floor_count() const;

    /**
     *   @brief Reset the level back to its default state
     */
    void clear_level();

    bool save(const std::filesystem::path& path);
    bool load(const std::filesystem::path& path);

    bool changes_made_since_last_save() const;

  private:
    bool do_save(const std::filesystem::path& path) const;
    std::optional<Floor*> find_floor(int floor_number);
    std::optional<const Floor*> find_floor(int floor_number) const;

    template <typename LoadFunc>
    void load_objects(nlohmann::json& json, const char* object_key, int floor_number, LoadFunc func)
    {
        if (json.find(object_key) != json.end())
        {
            for (auto& object : json[object_key])
            {
                LevelObject level_object{0};
                func(level_object, object);
                add_object(level_object, floor_number);
            }
        }
    }



  private:
    std::vector<Floor> floors_;
    int max_floor_ = 0;
    int min_floor_ = 0;

    int current_id_ = 0;

    bool changes_made_since_last_save_ = false;
};
