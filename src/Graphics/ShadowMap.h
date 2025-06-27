#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "OpenGL/BufferObject.h"
#include "OpenGL/Framebuffer.h"
#include "OpenGL/Shader.h"

class DirectionalShadowMap
{
  public:
    DirectionalShadowMap(sf::Vector2u resolution);
    [[nodiscard]] bool create();

    glm::mat4 prepare_and_bind(const Camera& camera, const glm::vec3& light_direction);
    void bind_shadow_texture(GLuint unit);
    void render_preview(const char* name);

    gl::Shader& get_shader();

  private:
    gl::Shader shadow_map_shader_;
    gl::Framebuffer shadow_fbo_;
};

class PointlightShadowMap
{
  public:
    PointlightShadowMap(const sf::Vector2u window_size);
    [[nodiscard]] bool create();

    glm::mat4 prepare_and_bind(const Camera& camera, const glm::vec3& light_position);
    void bind_shadow_texture(GLuint unit);
    void render_preview(const char* name);

    gl::Shader& get_shader();

  private:
    gl::Shader shadow_map_shader_;
    gl::Framebuffer shadow_fbo_;
};

/*
class CascadingShadowMap
{
  public:
    CascadingShadowMap(const sf::Vector2u window_size);
    [[nodiscard]] bool create();

    void prepare_and_bind(const Camera& camera);

    gl::Shader shadow_map_shader;

    void bind_shadow_texture();

    // private:
    gl::BufferObject light_space_ubo_;
    gl::Framebuffer shadow_fbo_;

    glm::mat4 get_lightspace_matrix(const Camera& camera, float near_plane, float far_plane);
};
*/