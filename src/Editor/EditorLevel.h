#pragma once

#include <functional>
#include <memory>

#include "../Editor/WorldGeometry.h"
#include "../Graphics/Mesh.h"

class DrawingPad;

class EditorLevel
{
    struct LevelMesh
    {
        int id;
        WorldGeometryMesh3D mesh;
    };

  public:
    EditorLevel(const EditorState& state);

    Wall& add_wall(const WallParameters& parameters);

    void update_object(const Wall& wall);
    void remove_object(std::size_t id);

    // void on_add_object(std::function<void(LevelObject& object)> callback);
    void on_add_object(std::function<void(Wall& wall)> callback);

    /**
     * @brief Render the level in 3D.
     * Assumes the camera, shader, and other OpenGL states are set up correctly.
     */
    void render();

    /**
     * @brief Render the level in 2D using the given drawing pad.
     * Assumes the camera, shader, and other OpenGL states are set up correctly.
     *
     * @param drawing_pad The drawing pad to render the level on.
     */
    void render_2d(DrawingPad& drawing_pad);

    /**
     * @brief Try to select a level object at the given tile position.
     *
     * @param selection_tile The tile position to select an object at.
     * @return LevelObject* if an object is found at the given tile, otherwise nullptr.
     */
    LevelObject* try_select(glm::vec2 selection_tile);

  private:
    std::vector<Wall> walls_;
    std::vector<LevelMesh> wall_meshes_;
    int current_id_ = 0;
    const EditorState* p_editor_state_;

    std::vector<std::function<void(LevelObject& object)>> on_create_object_;
    std::vector<std::function<void(Wall& object)>> on_create_wall_;
    // std::vector<std::unique_ptr<LevelObject>> level_objects;
};
