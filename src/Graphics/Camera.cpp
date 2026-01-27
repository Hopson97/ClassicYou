#include "Camera.h"

#include <imgui.h>
#include <print>

#include "Mesh.h"
#include "OpenGL/VertexArrayObject.h"

Camera::Camera(const CameraConfig& config)
    : aspect_{config.viewport_size.x / config.viewport_size.y}
    , config_(config)
{
    set_projection();
}

void Camera::update()
{
    if (config_.type == CameraType::OrthographicScreen)
    {
        view_matrix_ = glm::lookAt(transform.position,
                                   {transform.position.x, transform.position.y, -10}, {0, 1, 0});
    }
    else
    {
        forwards_ = forward_vector(transform.rotation);
        glm::vec3 centre = transform.position + glm::normalize(forwards_);
        view_matrix_ = glm::lookAt(transform.position, centre, {0, 1, 0});
    }
}

const glm::mat4& Camera::get_view_matrix() const
{
    return view_matrix_;
}

const glm::mat4& Camera::get_projection_matrix() const
{
    return projection_matrix_;
}

const glm::vec3& Camera::get_forwards() const
{
    return forwards_;
}

CameraType Camera::get_type() const
{
    return config_.type;
}

float Camera::get_orthographic_scale() const
{
    return config_.orthographic_scale;
}

float Camera::near() const

{
    return config_.near;
}

float Camera::far() const
{
    return config_.far;
}

void Camera::set_orthographic_scale(float scale)
{
    config_.orthographic_scale = scale;
    set_projection();
}

float Camera::fov() const
{
    return config_.fov;
}

float Camera::aspect() const
{
    return aspect_;
}

void Camera::set_type(CameraType type)
{
    config_.type = type;
    set_projection();
}

void Camera::set_viewport(glm::vec2 viewport_position, glm::vec2 viewport_size)
{
    config_.viewport_size = viewport_size;
    config_.viewport_position = viewport_position;
    aspect_ = config_.viewport_size.x / config_.viewport_size.y;
    set_projection();
}

void Camera::use_viewport()
{
    auto x = static_cast<GLint>(config_.viewport_position.x);
    auto y = static_cast<GLint>(config_.viewport_position.y);
    auto w = static_cast<GLint>(config_.viewport_size.x);
    auto h = static_cast<GLint>(config_.viewport_size.y);
    glViewport(x, y, w, h);
}

const CameraConfig& Camera::get_config() const
{
    return config_;
}

void Camera::set_projection()
{
    if (config_.type == CameraType::Perspective)
    {
        projection_matrix_ =
            glm::perspective(glm::radians(config_.fov), aspect_, config_.near, config_.far);
    }

    else if (config_.type == CameraType::OrthographicWorld)
    {
        projection_matrix_ = glm::ortho(
            -config_.orthographic_scale * aspect_, config_.orthographic_scale * aspect_,
            -config_.orthographic_scale, config_.orthographic_scale, config_.near, config_.far);
    }

    else if (config_.type == CameraType::OrthographicScreen)
    {
        projection_matrix_ = glm::ortho(0.0f, config_.viewport_size.x * config_.orthographic_scale,
                                        config_.viewport_size.y * config_.orthographic_scale, 0.0f,
                                        config_.near, config_.far);
    }
}

void Camera::gui(const char* name)
{
    bool should_update = false;
    if (ImGui::Begin(name))
    {
        auto p = transform.position;
        auto r = transform.rotation;
        ImGui::Text("Position: (%f, %f, %f)", p.x, p.y, p.z);
        ImGui::Text("Rotation: (%f, %f, %f)", r.x, r.y, r.z);

        if (config_.type == CameraType::Perspective)
        {

            if (ImGui::SliderFloat("FOV", &config_.fov, 1.0f, 179.0f))
            {
                should_update = true;
            }
        }

        if (config_.type == CameraType::OrthographicWorld ||
            config_.type == CameraType::OrthographicScreen)
        {

            if (ImGui::SliderFloat("Orthographic Scale", &config_.orthographic_scale, 1.0f, 100.0f))
            {
                should_update = true;
            }
        }

        if (ImGui::SliderFloat("Near", &config_.near, 1.0f, 10.0f))
        {
            should_update = true;
        }
        if (ImGui::SliderFloat("Far", &config_.far, 100.0f, 10000.0f))
        {
            should_update = true;
        }
    }

    ImGui::End();

    if (should_update)
    {
        set_projection();
    }
}