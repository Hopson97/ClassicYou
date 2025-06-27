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

PointlightShadowMap::PointlightShadowMap(const sf::Vector2u window_size)
    : shadow_fbo_(window_size.x / 2, window_size.y / 2)
{
}

bool PointlightShadowMap::create()
{
    shadow_fbo_.attach_depth_buffer_cube_map();
    if (!setup_fbo(shadow_fbo_))
    {
        return false;
    }

    // clang-format off
    if (!shadow_map_shader_.load_stage("assets/shaders/PointLightShadowMapVertex.glsl",   gl::ShaderType::Vertex)   ||
        !shadow_map_shader_.load_stage("assets/shaders/PointLightShadowMapGeometry.glsl", gl::ShaderType::Geometry) ||
        !shadow_map_shader_.load_stage("assets/shaders/PointLightShadowMapFragment.glsl", gl::ShaderType::Fragment) ||
        !shadow_map_shader_.link_shaders())
    {
        return false;
    }
    // clang-format on

    return true;
}

glm::mat4 PointlightShadowMap::prepare_and_bind(const Camera& camera,
                                                const glm::vec3& light_position)
{
    // Construct the projection matrix for each 6 directions of the cube fbo
    // Setting to 90 degrees ensures 100% coverage for each of the faces, as they will align at
    // the edges
    float aspect = static_cast<float>(shadow_fbo_.width) / static_cast<float>(shadow_fbo_.height);
    float near = 1.0f;
    float far = 30.0f;
    glm::mat4 light_projection = glm::perspective(glm::radians(90.0f), aspect, near, far);

    // Create a shadow transform that faces perpendicular to all 6 directions of the point light
    std::array<glm::mat4, 6> transforms = {
        light_projection * glm::lookAt(light_position, light_position + RIGHT, DOWN),
        light_projection * glm::lookAt(light_position, light_position + LEFT, DOWN),
        light_projection * glm::lookAt(light_position, light_position + UP, FORWARDS),
        light_projection * glm::lookAt(light_position, light_position + DOWN, BACK),
        light_projection * glm::lookAt(light_position, light_position + FORWARDS, DOWN),
        light_projection * glm::lookAt(light_position, light_position + BACK, DOWN),
    };



    // Bind the framebuffer, and set the light space matrix in the shader.
    shadow_fbo_.bind(gl::FramebufferTarget::Framebuffer, true);
    shadow_map_shader_.bind();
    for (int i = 0; i < 6; i++)
    {
        shadow_map_shader_.set_uniform("shadow_matrices[" + std::to_string(i) + "]", transforms[i]);
    }

    // Return the light space matrix for use in the scene shader.
    return transforms[0];
}

void PointlightShadowMap::bind_shadow_texture(GLuint unit)
{
    shadow_fbo_.bind_texture(0, unit);
}

void PointlightShadowMap::render_preview(const char* name)
{
    //if (ImGui::Begin(name))
    //{
    //    ImTextureID imgui_id = static_cast<ImTextureID>(shadow_fbo_.get_texture_id(0));
    //    ImGui::Image(imgui_id, {256, 256});
    //}
    //ImGui::End();
}

gl::Shader& PointlightShadowMap::get_shader()
{
    return shadow_map_shader_;
}

/*

CascadingShadowMap::CascadingShadowMap(const sf::Vector2u window_size)
    : shadow_fbo_(window_size.x / 4, window_size.y / 4)
{
}

bool CascadingShadowMap::create()
{
    shadow_fbo_.attach_depth_buffer_array();

    float bordercolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(shadow_fbo_.get_texture(0).id, GL_TEXTURE_BORDER_COLOR, bordercolor);

    if (!shadow_fbo_.is_complete())
    {
        return false;
    }

    // clang-format off
    if (!shadow_map_shader.load_stage("assets/shaders/CascadingShadowMapVertex.glsl",
gl::ShaderType::Vertex)   ||
        !shadow_map_shader.load_stage("assets/shaders/CascadingShadowMapGeometry.glsl",
gl::ShaderType::Geometry) || !shadow_map_shader.load_stage("assets/shaders/Empty.glsl",
gl::ShaderType::Fragment) || !shadow_map_shader.link_shaders())
    {
        return false;
    }
    // clang-format on

    light_space_ubo_.create_as_ubo<glm::mat4>(0, 16);
    shadow_map_shader.bind_uniform_block_index("LightSpaceMatrices", 0);

    return true;
}

void CascadingShadowMap::prepare_and_bind(const Camera& camera)
{
    shadow_fbo_.bind(gl::FramebufferTarget::Framebuffer, true);
    shadow_map_shader.bind();

    auto near = camera.far();
    auto far = camera.far();
    std::array<float, 4> shadow_cascade_levels{far / 50.0f, far / 25.0f, far / 10.0f, far
/ 2.0f};

    std::array<glm::mat4, 5> light_space_matrices;
    for (size_t i = 0; i < shadow_cascade_levels.size() + 1; ++i)
    {
        if (i == 0)
        {
            light_space_matrices.at(i) =
                get_lightspace_matrix(camera, near, shadow_cascade_levels[i]);
        }
        else if (i < shadow_cascade_levels.size())
        {
            light_space_matrices.at(i) = get_lightspace_matrix(camera, shadow_cascade_levels[i -
1], shadow_cascade_levels[i]);
        }
        else
        {
            light_space_matrices.at(i) =
                get_lightspace_matrix(camera, shadow_cascade_levels[i - 1], far);
        }
    }

    for (size_t i = 0; i < light_space_matrices.size(); ++i)
    {
        light_space_ubo_.buffer_sub_data(i * sizeof(glm::mat4), light_space_matrices[i]);
    }

    // light_space_ubo_.buffer_sub_data(0, light_space_matrices);
}

void CascadingShadowMap::bind_shadow_texture()
{
    shadow_fbo_.bind_texture(0, 0);
}

glm::mat4 CascadingShadowMap::get_lightspace_matrix(const Camera& camera, float near_plane,
                                                    float far_plane)
{
    auto projection = glm::perspective(camera.fov(), camera.aspect(), near_plane, far_plane);
    auto corners = getFrustumCornersWorldSpace(projection, camera.get_view_matrix());

    // Find the centre of the frustum - this is where the light map will be centered to
    glm::vec3 center = glm::vec3(0, 0, 0);

    for (const auto& v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();

    auto light_view_matrix = glm::lookAt(center + light_direction, center, {0.0f, 1.0f, 0.0f});

    // Find the orthographic min/max
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const auto& corner : corners)
    {
        const auto transform = light_view_matrix * corner;
        min_x = std::min(min_x, transform.x);
        max_x = std::max(max_x, transform.x);
        min_y = std::min(min_y, transform.y);
        max_y = std::max(max_y, transform.y);
        min_z = std::min(min_z, transform.z);
        max_z = std::max(max_z, transform.z);
    }

    // Construct the orthographic matrix
    constexpr float z_factor = 10.0f;
    if (min_z < 0)
    {
        min_z *= z_factor;
    }
    else
    {
        min_z /= z_factor;
    }

    if (max_z < 0)
    {
        max_z /= z_factor;
    }
    else
    {
        max_z *= z_factor;
    }

    return glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z) * light_view_matrix;
}

*/
