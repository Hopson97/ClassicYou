#pragma once

#include <glm/glm.hpp>

#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"

// The world size in tiles
constexpr auto WORLD_SIZE = 32;
constexpr auto TILE_SIZE = 32;
constexpr auto HALF_TILE_SIZE = TILE_SIZE / 2;

class DrawingPad
{
  public:
    DrawingPad(glm::vec2 size);

    bool init();
    void update(const Keyboard& keyboard, sf::Time dt);

    void render_line(glm::vec2 from, glm::vec2 to, const glm::vec4& colour, int thickness);

    void display();

    const Camera& get_camera() const;

  private:
    gl::Shader shader_;
    std::unordered_map<GLuint, Mesh2D> line_meshes_;

    struct GridMesh
    {
        Mesh2D sub_grid;
        Mesh2D main_grid;
    };
    GridMesh grid_mesh_;

    Camera camera_;
    CameraKeybinds keybinds_2d_;
};
