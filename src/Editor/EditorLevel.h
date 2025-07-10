#pragma once

#include <functional>
#include <memory>

#include "../Editor/WorldGeometry.h"
#include "../Graphics/Mesh.h"

class DrawingPad;

using ObjectId = std::int32_t;

class EditorLevel
{
    struct LevelMesh
    {
        ObjectId id;
        WorldGeometryMesh3D mesh;
    };

  public:
    // Wall& add_wall(const WallParameters& parameters, const WallProps& props);
    LevelObjectV2& add_object(const LevelObjectV2& object);

    void update_object(const LevelObjectV2& object);
    void remove_object(std::size_t id);

    void set_object_id(ObjectId current_id, ObjectId new_id);

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
    void render_2d(DrawingPad& drawing_pad, const LevelObjectV2* p_active_object);

    /**
     * @brief Try to select a level object at the given tile position.
     *
     * @param selection_tile The tile position to select an object at.
     * @return LevelObject* if an object is found at the given tile, otherwise nullptr.
     */
    LevelObjectV2* try_select(glm::vec2 selection_tile, const LevelObjectV2* p_active_object);

  private:
    std::vector<LevelObjectV2> level_objects_;
    std::vector<LevelMesh> level_meshes_;
    int current_id_ = 0;
};
