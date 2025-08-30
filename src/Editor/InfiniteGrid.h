#pragma once

#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"

/// For the infinite grid in the editor.
class InfiniteGrid
{
  public:
    bool init();

    void render(const glm::vec3& camera_position, int current_floor);

  private:
    Mesh3D grid_mesh_ = generate_grid_mesh(64, 64);
    gl::VertexArrayObject grid_vao_;
    gl::Shader grid_shader_;
    gl::Shader scene_grid_shader_;
};

    