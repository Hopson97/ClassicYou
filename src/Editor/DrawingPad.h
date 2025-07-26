#pragma once

#include <glm/glm.hpp>

#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/Shader.h"
#include "../Graphics/OpenGL/Texture.h"

class DrawingPad
{
  public:
    DrawingPad(glm::vec2 size, const glm::ivec2& selected_node);

    bool init();
    void update(const Keyboard& keyboard, sf::Time dt);

    void render_quad(glm::vec2 position, glm::vec2 size, const glm::vec4& colour);
    void render_line(glm::vec2 from, glm::vec2 to, const glm::vec4& colour, GLfloat thickness);
    void render_diamond(glm::vec2 position, glm::vec2 size, const glm::vec4& colour);

    void display(const Transform& camera_transform);

    const Camera& get_camera() const;

    void set_camera_position(glm::vec2 position);

  private:
    gl::Shader scene_shader_;
    gl::Shader grid_shader_;
    std::unordered_map<GLfloat, Mesh2D> line_meshes_;

    struct GridMesh
    {
        Mesh2D sub_grid;
        Mesh2D main_grid;
    } grid_mesh_;

    Camera camera_;
    CameraKeybinds keybinds_2d_;

    const glm::ivec2& selected_node_;
    gl::Texture2D selection_texture_;
    gl::Texture2D arrow_texture_;
    Mesh2D selection_mesh_;
};
