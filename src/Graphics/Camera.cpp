#include "Camera.h"

#include <imgui.h>
#include <print>

#include "Mesh.h"
#include "OpenGL/VertexArrayObject.h"

Camera::Camera(const CameraConfig& config)
    : aspect_{static_cast<float>(config.viewport_size.x / config.viewport_size.y)}
    , config_{config}
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

glm::vec3 Camera::find_mouse_floor_intersect(glm::vec2 mouse_click, float floor_y) const
{
    // See https://antongerdelan.net/opengl/raycasting.html for a detailed explanation
    
    // The mouse position must be localised to the current viewport
    mouse_click.x = mouse_click.x - config_.viewport_position.x;
    mouse_click.y = mouse_click.y - config_.viewport_position.y;

    glm::vec2 ndc = {(2.0f * mouse_click.x) / config_.viewport_size.x - 1.0f,
                     1.0f - (2.0f * mouse_click.y) / config_.viewport_size.y};

    glm::vec4 clip_space = {ndc.x, ndc.y, 1.0f, 1.0f};
    glm::vec4 world_space = glm::inverse(projection_matrix_ * view_matrix_) * clip_space;

    // Where the ray begins and ends, must divide by W to get the 3D world coords
    glm::vec3 ray_origin = transform.position;
    glm::vec3 ray_end = glm::vec3(world_space) / world_space.w;

    // Points from the origin to the end of the ray
    glm::vec3 ray_dir = glm::normalize(ray_end - ray_origin);

    // When the Y is is near 0, it means it is parallel to the floor and won't intersect
    if (std::abs(ray_dir.y) < 0.0001f)
    {
        return ray_origin;
    }

    // Plane intersection
    float t = (floor_y - ray_origin.y) / ray_dir.y;

    // Ensure t is not behind the camera
    if (t < 0.0f)
    {
        return ray_origin;
    }

    // Scale the ray by the distance 't' and add to the origin position
    return ray_origin + ray_dir * t;
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
