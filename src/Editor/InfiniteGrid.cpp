#include "InfiniteGrid.h"

#include "EditConstants.h"
#include "../Graphics/OpenGL/GLUtils.h"

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
        grid_mesh_.bind().draw_elements(GL_LINES);
    }
}