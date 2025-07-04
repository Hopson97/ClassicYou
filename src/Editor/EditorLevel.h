#pragma once

#include <functional>
#include <memory>

#include "../Editor/WorldGeometry.h"
#include "../Graphics/Mesh.h"

struct EditorLevel
{
  public:
    std::vector<Wall> walls;

  public:
    EditorLevel(const EditorState& state);

    Wall& add_wall(const WallParameters& paramters);

    //void on_add_object(std::function<void(LevelObject& object)> callback);
    void on_add_object(std::function<void(Wall& wall)> callback);

    int current_id_ = 0;

  private:
    const EditorState* p_editor_state_;

    std::vector<std::function<void(LevelObject& object)>> on_create_object_;
    std::vector<std::function<void(Wall& object)>> on_create_wall_;
    // std::vector<std::unique_ptr<LevelObject>> level_objects;
};
