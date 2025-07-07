#pragma once

#include <functional>
#include <memory>

#include "../Editor/WorldGeometry.h"
#include "../Graphics/Mesh.h"

class EditorLevel
{
  public:
    std::vector<Wall> walls;

    struct LevelMesh
    {
        int id;
        WorldGeometryMesh3D mesh;
    };

  public:
    EditorLevel(const EditorState& state);

    Wall& add_wall(const WallParameters& paramters);

    void update_object(const Wall& wall);
    void remove_object(std::size_t id);

    // void on_add_object(std::function<void(LevelObject& object)> callback);
    void on_add_object(std::function<void(Wall& wall)> callback);

    void render();

  private:
    std::vector<LevelMesh> wall_meshes_;
    int current_id_ = 0;
    const EditorState* p_editor_state_;

    std::vector<std::function<void(LevelObject& object)>> on_create_object_;
    std::vector<std::function<void(Wall& object)>> on_create_wall_;
    // std::vector<std::unique_ptr<LevelObject>> level_objects;
};
