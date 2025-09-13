#include "Grids.h"

#include "../Graphics/Camera.h"
#include "../Graphics/OpenGL/GLUtils.h"
#include "EditConstants.h"

bool InfiniteGrid::init()
{
    grid_mesh_.buffer();

    // Load the shader for the infinite grid
    if (!grid_shader_.load_stage("assets/shaders/Grid/Grid3DVertex.glsl", gl::ShaderType::Vertex) ||
        !grid_shader_.load_stage("assets/shaders/Grid/Grid3DFragment.glsl",
                                 gl::ShaderType::Fragment) ||
        !grid_shader_.link_shaders())
    {
        return false;
    }
    grid_shader_.set_uniform("floor_height", FLOOR_HEIGHT);

    if (!scene_grid_shader_.load_stage("assets/shaders/Grid/SceneGridVertex.glsl",
                                       gl::ShaderType::Vertex) ||
        !scene_grid_shader_.load_stage("assets/shaders/Grid/SceneGridFragment.glsl",
                                       gl::ShaderType::Fragment) ||
        !scene_grid_shader_.link_shaders())
    {
        return false;
    }
    scene_grid_shader_.set_uniform("floor_height", FLOOR_HEIGHT);
    return true;
}

void InfiniteGrid::render(const glm::vec3& camera_position, int current_floor)
{
    if (camera_position.y > current_floor * FLOOR_HEIGHT)
    {

        gl::disable(gl::Capability::CullFace);
        gl::enable(gl::Capability::Blend);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        grid_shader_.bind();
        grid_vao_.bind();
        grid_shader_.set_uniform("camera_position", camera_position);
        grid_shader_.set_uniform("floor_number", current_floor);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        gl::disable(gl::Capability::Blend);
        gl::enable(gl::Capability::CullFace);
    }
    else
    {
        // For some reason the "infinite" grid goes all strange when the camera is under it - so
        // fall back to using a mesh-grid in this case
        glLineWidth(2);
        scene_grid_shader_.bind();
        scene_grid_shader_.set_uniform("camera_position", camera_position);
        scene_grid_shader_.set_uniform("floor_number", current_floor);
        grid_mesh_.bind().draw_elements(gl::PrimitiveType::Lines);
    }
}

bool Grid2D::init()
{
    if (!grid_shader_.load_stage("assets/shaders/Grid/Grid2DVertex.glsl", gl::ShaderType::Vertex) ||
        !grid_shader_.load_stage("assets/shaders/Grid/SceneGridFragment.glsl",
                                 gl::ShaderType::Fragment) ||
        !grid_shader_.link_shaders())
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

    return true;
}

void Grid2D::render(const Camera& camera)
{
    grid_shader_.bind();
    grid_shader_.set_uniform("projection_matrix", camera.get_projection_matrix());
    grid_shader_.set_uniform("view_matrix", camera.get_view_matrix());
    grid_shader_.set_uniform("camera_position", camera.transform.position);
    glLineWidth(1);
    grid_mesh_.sub_grid.bind().draw_elements(gl::PrimitiveType::Lines);
    grid_mesh_.main_grid.bind().draw_elements(gl::PrimitiveType::Lines);
}