#include "Skybox.h"
#include "OpenGL/GLUtils.h"

bool Skybox::init(const std::filesystem::path& folder)
{
    skybox_mesh_.buffer();

    if (!shader_.load_stage("assets/shaders/SkyboxVertex.glsl", gl::ShaderType::Vertex) ||
        !shader_.load_stage("assets/shaders/SkyboxFragment.glsl", gl::ShaderType::Fragment) ||
        !shader_.link_shaders())
    {
        return false;
    }


    // Load skybox
    if (!texture_.load_from_folder("assets/textures/skybox"))
    {
        return false;
    }

    return true;
}

void Skybox::render()
{
    gl::cull_face(gl::Face::Front);
    shader_.bind();
    texture_.bind(0);
    skybox_mesh_.bind();
    skybox_mesh_.draw_elements();
}
