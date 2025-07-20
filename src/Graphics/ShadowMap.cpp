#include "ShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <vector>

namespace
{

    constexpr glm::vec3 UP = {0.0, 1.0, 0.0};
    constexpr glm::vec3 DOWN = {0.0, -1.0, 0.0};
    constexpr glm::vec3 LEFT = {-1.0, 0.0, 0.0};
    constexpr glm::vec3 RIGHT = {1.0, 0.0, 0.0};
    constexpr glm::vec3 FORWARDS = {0.0, 0.0, 1.0};
    constexpr glm::vec3 BACK = {0.0, 0.0, -1.0};

    bool setup_fbo(gl::Framebuffer& framebuffer)
    {
        // Set the texture parameters for the shadow map. This is important for shadow mapping to
        // work correctly, as without clamping it can cause shadows to "bleed" into areas they
        // should not
        auto& texture = framebuffer.get_texture(0);
        texture.set_filters(gl::TextureParameters{
            .min_filter = gl::TextureMinFilter::Linear,
            .mag_filter = gl::TextureMagFilter::Linear,
            .wrap_s = gl::TextureWrap::ClampToBorder,
            .wrap_t = gl::TextureWrap::ClampToBorder,
            .wrap_r = gl::TextureWrap::ClampToBorder,
        });

        // As the shaders use sampler2DShadow, we need to set the comparison function and mode.
        texture.set_compare_function(gl::TextureCompareFunction::LessThan);
        texture.set_compare_mode(gl::TextureCompareMode::CompareReferenceToTexture);

        // Setting the border to white ensures that the area outside of the shadow map is not
        // completely dark. This works with the "ClampToBorder" texture wrap mode.
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTextureParameterfv(framebuffer.get_texture(0).id, GL_TEXTURE_BORDER_COLOR, borderColor);
        if (!framebuffer.is_complete())
        {
            return false;
        }
        return true;
    }
} // namespace

namespace
{

} // namespace

DirectionalShadowMap::DirectionalShadowMap(sf::Vector2u resolution)
    : shadow_fbo_(resolution.x, resolution.y)
{
}

bool DirectionalShadowMap::create()
{
    shadow_fbo_.attach_depth_buffer();

    if (!setup_fbo(shadow_fbo_))
    {
        return false;
    }

    // clang-format off
    if (!shadow_map_shader_.load_stage("assets/shaders/Shadows/ShadowMapVertex.glsl", gl::ShaderType::Vertex)   ||
        !shadow_map_shader_.load_stage("assets/shaders/Empty.glsl",                   gl::ShaderType::Fragment) ||
        !shadow_map_shader_.link_shaders())
    {
        return false;
    }
    // clang-format on

    return true;
}

glm::mat4 DirectionalShadowMap::prepare_and_bind(const Camera& camera,
                                                 const glm::vec3& light_direction)
{
    static
        // First construct the orthographic projection matrix for the light space.
        float size = 10.0f;

    if (ImGui::Begin("name"))
    {
        ImGui::SliderFloat("Shadow map zoom", &size, 1.0f, 100.0f);
    }
    ImGui::End();

    glm::mat4 light_projection = glm::ortho(-size, size, -size, size, -10.0f, 150.0f);

    // Calculate the light view matrix.
    // This is used to transform the scene to the perspective of the light source.
    // Add to the camera position such that the shadow map "follows" the camera.
    auto origin = -light_direction + camera.transform.position;
    auto target = camera.transform.position;
    auto light_view = glm::lookAt(origin, target, UP);
    auto light_space_matrix = light_projection * light_view;

    // Bind the framebuffer, and set the light space matrix in the shader.
    shadow_fbo_.bind(gl::FramebufferTarget::Framebuffer, true);
    shadow_map_shader_.bind();
    shadow_map_shader_.set_uniform("light_space_matrix", light_space_matrix);

    // Return the light space matrix for use in the scene shader.
    return light_space_matrix;
}

void DirectionalShadowMap::bind_shadow_texture(GLuint unit)
{
    shadow_fbo_.bind_texture(0, unit);
}

void DirectionalShadowMap::render_preview(const char* name)
{
    if (ImGui::Begin(name))
    {
        ImTextureID imgui_id = static_cast<ImTextureID>(shadow_fbo_.get_texture_id(0));
        ImGui::Image(imgui_id, {256, 256});
    }
    ImGui::End();
}

gl::Shader& DirectionalShadowMap::get_shader()
{
    return shadow_map_shader_;
}
