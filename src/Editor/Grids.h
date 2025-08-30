#pragma once

#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"

struct Camera;

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

/// For the 2D grid
class Grid2D
{
  public:
    bool init();

    void render(const Camera& camera);

  private:
    gl::Shader grid_shader_;
    std::unordered_map<GLfloat, Mesh2D> line_meshes_;

    struct GridMesh
    {
        Mesh2D sub_grid;
        Mesh2D main_grid;
    } grid_mesh_;
};
