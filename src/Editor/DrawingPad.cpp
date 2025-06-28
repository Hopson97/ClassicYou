#include "DrawingPad.h"

#include <glad/glad.h>
#include <imgui.h>

namespace
{
    constexpr glm::vec4 rgb_to_normalised(const glm::vec3& rgb)
    {
        return glm::vec4(rgb / 255.0f, 1.0f);
    }

    void add_line_to_mesh(Mesh2D& mesh, glm::vec2 from, glm::vec2 to, const glm::vec4& colour)
    {
        mesh.vertices.push_back(Vertex2D{.position = from, .colour = colour});
        mesh.vertices.push_back(Vertex2D{.position = to, .colour = colour});

        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    }

} // namespace

DrawingPad::DrawingPad(glm::vec2 size)
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

    auto main_colour = rgb_to_normalised({69, 103, 137});
    auto sub_colour = rgb_to_normalised({18, 52, 86});
    for (int x = 0; x <= WORLD_SIZE - 1; x++)
    {
        add_line_to_mesh(grid_mesh_.sub_grid, {TILE_SIZE * x + HALF_TILE_SIZE, 0},
                         {TILE_SIZE * x + HALF_TILE_SIZE, TILE_SIZE * WORLD_SIZE}, sub_colour);
    }
    for (int y = 0; y <= WORLD_SIZE - 1; y++)
    {
        add_line_to_mesh(grid_mesh_.sub_grid, {0, TILE_SIZE * y + HALF_TILE_SIZE},
                         {TILE_SIZE * WORLD_SIZE, TILE_SIZE * y + HALF_TILE_SIZE}, sub_colour);
    }

    // Render the main grid
    for (int x = 0; x <= WORLD_SIZE - 1; x++)
    {
        add_line_to_mesh(grid_mesh_.main_grid, {TILE_SIZE * x, 0},
                         {TILE_SIZE * x, TILE_SIZE * WORLD_SIZE}, main_colour);
    }

    for (int y = 0; y <= WORLD_SIZE - 1; y++)
    {
        add_line_to_mesh(grid_mesh_.main_grid, {0, TILE_SIZE * y},
                         {TILE_SIZE * WORLD_SIZE, TILE_SIZE * y}, main_colour);
    }

    grid_mesh_.sub_grid.buffer();
    grid_mesh_.main_grid.buffer();

    return true;
}

void DrawingPad::render_line(glm::vec2 from, glm::vec2 to, const glm::vec4& colour, int thickness)
{
    if (line_meshes_.find(thickness) == line_meshes_.end())
    {
        line_meshes_.emplace(thickness, Mesh2D{});
    }

    auto& mesh = line_meshes_.find(thickness)->second;

    mesh.vertices.push_back(Vertex2D{.position = from, .colour = colour});
    mesh.vertices.push_back(Vertex2D{.position = to, .colour = colour});

    mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
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
    auto p = camera_.transform.position;

    camera_.gui("2D Camera");
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
        // camera_.transform.position.y += 1.0f;
    }

    shader_.bind();
    shader_.set_uniform("projection_matrix", camera_.get_projection_matrix());
    shader_.set_uniform("view_matrix", camera_.get_view_matrix());
    shader_.set_uniform("model_matrix", create_model_matrix({}));

    
    for (auto& [thickness, mesh] : line_meshes_)
    {
        glLineWidth(thickness);
        mesh.update();
        mesh.bind().draw_elements(GL_LINES);
        mesh.vertices.clear();
        mesh.indices.clear();
    }
    

    // Render the background grid
    glLineWidth(1);
    grid_mesh_.sub_grid.bind().draw_elements(GL_LINES);

    glLineWidth(1);
    grid_mesh_.main_grid.bind().draw_elements(GL_LINES);
}
