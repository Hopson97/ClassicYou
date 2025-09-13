#include "DrawingPad.h"

#include <glad/glad.h>
#include <imgui.h>

#include "../Graphics/OpenGL/GLUtils.h"
#include "EditConstants.h"

DrawingPad::DrawingPad(glm::vec2 size, const glm::ivec2& selected_node)
    : camera_(CameraConfig{
          .type = CameraType::OrthographicScreen,
          .viewport_size = size,
          .near = 0.5f,
          .far = 1000.0f,
      })
    , keybinds_2d_{
          .forward = sf::Keyboard::Key::Down,
          .left = sf::Keyboard::Key::Left,
          .right = sf::Keyboard::Key::Right,
          .back = sf::Keyboard::Key::Up,
      }
      ,selected_node_(selected_node)
{
    // * 4 is such that the 3D default view can be seen
    camera_.transform.position = {WORLD_SIZE * TILE_SIZE / 3,
                                  WORLD_SIZE * TILE_SIZE - TILE_SIZE * TILE_SIZE + TILE_SIZE * 4,
                                  10};
}

bool DrawingPad::init()
{
    if (!scene_shader_.load_stage("assets/shaders/Scene/Scene2DVertex.glsl",
                                  gl::ShaderType::Vertex) ||
        !scene_shader_.load_stage("assets/shaders/Scene/Scene2DFragment.glsl",
                                  gl::ShaderType::Fragment) ||
        !scene_shader_.link_shaders())
    {
        return false;
    }

    if (!grid_shader_.load_stage("assets/shaders/Grid/Grid2DVertex.glsl", gl::ShaderType::Vertex) ||
        !grid_shader_.load_stage("assets/shaders/Grid/SceneGridFragment.glsl",
                                 gl::ShaderType::Fragment) ||
        !grid_shader_.link_shaders())
    {
        return false;
    }

    if (!selection_texture_.load_from_file("assets/textures/Selection.png", 1, false, false))
    {
        return false;
    }

    if (!arrow_texture_.load_from_file("assets/textures/Arrow.png", 1, false, false))
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
    // selection_mesh_.vertices = {
    //    {.position = {0.0f, 0.0f}, .texture_coord = {0.0f, 0.0f}, .colour = glm::vec4(1.0f)},
    //    {.position = {0.0f, 8.0f}, .texture_coord = {0.0f, 1.0f}, .colour = glm::vec4(1.0f)},
    //    {.position = {8.0f, 8.0f}, .texture_coord = {1.0f, 1.0f}, .colour = glm::vec4(1.0f)},
    //    {.position = {8.0f, 0.0f}, .texture_coord = {1.0f, 0.0f}, .colour = glm::vec4(1.0f)},
    //};

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

void DrawingPad::set_camera_position(glm::vec2 position)
{
    const auto& viewport = camera_.get_config().viewport_size;
    camera_.transform.position = {
        position.x - viewport.x / 2,
        position.y - viewport.y / 2,
        camera_.transform.position.z,
    };
}

void DrawingPad::camera_gui()
{
    // auto scale = camera_.get_orthographic_scale();
    // if (ImGui::SliderFloat("Zoom", &scale, 0.5, 2.5))
    //{
    //     camera_.set_orthographic_scale(scale);
    // }
}

void DrawingPad::display(const Transform& camera_transform)
{
    // For 2D rendering, depth testing is not required
    gl::disable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);

    // Update the shaders
    scene_shader_.bind();
    scene_shader_.set_uniform("projection_matrix", camera_.get_projection_matrix());
    scene_shader_.set_uniform("view_matrix", camera_.get_view_matrix());
    scene_shader_.set_uniform("model_matrix", create_model_matrix({}));
    scene_shader_.set_uniform("use_texture", false);

    // Render lines (walls) of various thickness
    for (auto& [thickness, mesh] : line_meshes_)
    {
        glLineWidth(thickness);
        mesh.update();
        mesh.bind().draw_elements(gl::PrimitiveType::Lines);
        mesh.vertices.clear();
        mesh.indices.clear();
    }
    line_meshes_.clear();

    // Render the selected quad
    scene_shader_.set_uniform("use_texture", true);

    // Offset selection by half its size such it renders in the center
    selection_texture_.bind(0);
    scene_shader_.set_uniform(
        "model_matrix",
        create_model_matrix({.position = {selected_node_.x - 4.0f, selected_node_.y - 4.0f, 0}}));
    selection_mesh_.bind().draw_elements();

    // Render the 3D camera's position
    arrow_texture_.bind(0);
    auto model = create_model_matrix_orbit(
        {.position = {camera_transform.position.x * TILE_SIZE - TILE_SIZE / 4,
                      camera_transform.position.z * TILE_SIZE, 0},
         .rotation = {0, 0, camera_transform.rotation.y + 90.0f}},
        {8, 8, 0});
    model = glm::scale(model, {2, 2, 4});
    scene_shader_.set_uniform("model_matrix", model);
    selection_mesh_.bind().draw_elements();

    // Render the background grid
    // Update the shaders
    grid_shader_.bind();
    grid_shader_.set_uniform("projection_matrix", camera_.get_projection_matrix());
    grid_shader_.set_uniform("view_matrix", camera_.get_view_matrix());
    grid_shader_.set_uniform("camera_position", camera_.transform.position);
    glLineWidth(1);
    grid_mesh_.sub_grid.bind().draw_elements(gl::PrimitiveType::Lines);
    grid_mesh_.main_grid.bind().draw_elements(gl::PrimitiveType::Lines);
}
