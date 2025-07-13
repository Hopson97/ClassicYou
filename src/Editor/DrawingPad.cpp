#include "DrawingPad.h"

#include <glad/glad.h>
#include <imgui.h>

#include "../Graphics/OpenGL/GLUtils.h"
#include "EditConstants.h"

namespace
{
    void add_line_to_mesh(Mesh2D& mesh, glm::vec2 from, glm::vec2 to, const glm::vec4& colour)
    {
        mesh.vertices.push_back(Vertex2D{.position = from, .colour = colour});
        mesh.vertices.push_back(Vertex2D{.position = to, .colour = colour});

        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    }

} // namespace

DrawingPad::DrawingPad(glm::vec2 size, const glm::ivec2& selected_node)
    : camera_(CameraConfig{
          .type = CameraType::OrthographicScreen,
          .viewport_size = size,
          .near = 0.5f,
          .far = 1000.0f,
      })
    , keybinds_2d_{
          .foward = sf::Keyboard::Key::Up,
          .left = sf::Keyboard::Key::Left,
          .right = sf::Keyboard::Key::Right,
          .back = sf::Keyboard::Key::Down,
      }
      ,selected_node_(selected_node)
{
    camera_.transform.position = {0, 0, 10};
}

bool DrawingPad::init()
{
    if (!shader_.load_stage("assets/shaders/Scene/Scene2DVertex.glsl", gl::ShaderType::Vertex) ||
        !shader_.load_stage("assets/shaders/Scene/Scene2DFragment.glsl",
                            gl::ShaderType::Fragment) ||
        !shader_.link_shaders())
    {
        return false;
    }

    if (!selection_texture_.load_from_file("assets/textures/selection.png", 1, false, false))
    {
        return false;
    }

    // Create the "sub" grid
    for (int x = 0; x <= WORLD_SIZE - 1; x++)
    {
        add_line_to_mesh(grid_mesh_.sub_grid, {TILE_SIZE * x + HALF_TILE_SIZE, 0},
                         {TILE_SIZE * x + HALF_TILE_SIZE, TILE_SIZE * WORLD_SIZE}, SUB_GRID_COLOUR);
    }
    for (int y = 0; y <= WORLD_SIZE - 1; y++)
    {
        add_line_to_mesh(grid_mesh_.sub_grid, {0, TILE_SIZE * y + HALF_TILE_SIZE},
                         {TILE_SIZE * WORLD_SIZE, TILE_SIZE * y + HALF_TILE_SIZE}, SUB_GRID_COLOUR);
    }

    // Create the main grid
    for (int x = 0; x <= WORLD_SIZE; x++)
    {
        add_line_to_mesh(grid_mesh_.main_grid, {TILE_SIZE * x, 0},
                         {TILE_SIZE * x, TILE_SIZE * WORLD_SIZE}, MAIN_GRID_COLOUR);
    }

    for (int y = 0; y <= WORLD_SIZE; y++)
    {
        add_line_to_mesh(grid_mesh_.main_grid, {0, TILE_SIZE * y},
                         {TILE_SIZE * WORLD_SIZE, TILE_SIZE * y}, MAIN_GRID_COLOUR);
    }
    grid_mesh_.sub_grid.buffer();
    grid_mesh_.main_grid.buffer();

    // Create the mesh for displaying whatever node is being selected
    selection_mesh_.vertices = {
        {.position = {0.0f, 0.0f}, .texture_coord = {0.0f, 0.0f}, .colour = glm::vec4(1.0f)},
        {.position = {0.0f, 8.0f}, .texture_coord = {0.0f, 1.0f}, .colour = glm::vec4(1.0f)},
        {.position = {8.0f, 8.0f}, .texture_coord = {1.0f, 1.0f}, .colour = glm::vec4(1.0f)},
        {.position = {8.0f, 0.0f}, .texture_coord = {1.0f, 0.0f}, .colour = glm::vec4(1.0f)},
    };

    selection_mesh_.indices = {0, 1, 2, 2, 3, 0};
    selection_mesh_.buffer();

    return true;
}

void DrawingPad::render_quad(glm::vec2 position, glm::vec2 size, const glm::vec4& colour)
{
    // TODO Actually render a quad
    float thickness = 2.0f;
    if (line_meshes_.find(thickness) == line_meshes_.end())
    {
        line_meshes_.emplace(thickness, Mesh2D{});
    }

    auto& mesh = line_meshes_.find(thickness)->second;

    add_line_to_mesh(mesh, {position.x, position.y}, {position.x + size.x, position.y}, colour);
    add_line_to_mesh(mesh, {position.x + size.x, position.y},
                     {position.x + size.x, position.y + size.y}, colour);
    add_line_to_mesh(mesh, {position.x + size.x, position.y + size.y},
                     {position.x, position.y + size.y}, colour);
    add_line_to_mesh(mesh, {position.x, position.y + size.y}, {position.x, position.y}, colour);
}

void DrawingPad::render_diamond(glm::vec2 position, glm::vec2 size, const glm::vec4& colour)
{
    // TODO Actually render a diamond
    float thickness = 2.0f;
    if (line_meshes_.find(thickness) == line_meshes_.end())
    {
        line_meshes_.emplace(thickness, Mesh2D{});
    }

    auto& mesh = line_meshes_.find(thickness)->second;

    add_line_to_mesh(mesh, {position.x + size.x / 2, position.y},
                     {position.x + size.x, position.y + size.y / 2}, colour);
    add_line_to_mesh(mesh, {position.x + size.x, position.y + size.y / 2},
                     {position.x + size.x / 2, position.y + size.y}, colour);
    add_line_to_mesh(mesh, {position.x + size.x / 2, position.y + size.y},
                     {position.x, position.y + size.y / 2}, colour);
    add_line_to_mesh(mesh, {position.x, position.y + size.y / 2},
                     {position.x + size.x / 2, position.y}, colour);
}

void DrawingPad::render_line(glm::vec2 from, glm::vec2 to, const glm::vec4& colour,
                             GLfloat thickness)
{
    if (line_meshes_.find(thickness) == line_meshes_.end())
    {
        line_meshes_.emplace(thickness, Mesh2D{});
    }
    add_line_to_mesh(line_meshes_.find(thickness)->second, from, to, colour);
}

void DrawingPad::update(const Keyboard& keyboard, sf::Time dt)
{
    free_camera_controller_2d(keyboard, camera_, dt, keybinds_2d_);
}

const Camera& DrawingPad::get_camera() const
{
    return camera_;
}

void DrawingPad::display()
{
    // For 2D rendering, depth testing is not required
    gl::disable(gl::Capability::DepthTest);
    gl::disable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);

    // Update the shaders
    shader_.bind();
    shader_.set_uniform("projection_matrix", camera_.get_projection_matrix());
    shader_.set_uniform("view_matrix", camera_.get_view_matrix());
    shader_.set_uniform("model_matrix", create_model_matrix({}));
    shader_.set_uniform("use_texture", false);

    // Render the background grid
    glLineWidth(1);
    grid_mesh_.sub_grid.bind().draw_elements(GL_LINES);
    grid_mesh_.main_grid.bind().draw_elements(GL_LINES);

    // Render lines (walls) of various thickenss
    for (auto& [thickness, mesh] : line_meshes_)
    {
        glLineWidth(thickness);
        mesh.update();
        mesh.bind().draw_elements(GL_LINES);
        mesh.vertices.clear();
        mesh.indices.clear();
    }
    line_meshes_.clear();

    // Reder the selected quad
    shader_.set_uniform("use_texture", true);

    // Offset selection by half its size such it renders in the center
    selection_texture_.bind(0);
    shader_.set_uniform(
        "model_matrix",
        create_model_matrix({.position = {selected_node_.x - 4.0f, selected_node_.y - 4.0f, 0}}));
    selection_mesh_.bind().draw_elements();
}
