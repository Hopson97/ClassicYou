#pragma once

#include "../Util/Maths.h"

enum class CameraType
{
    Perspective,
    OrthographicWorld,
    OrthographicScreen
};

struct CameraConfig
{
    CameraType type = CameraType::Perspective;

    // Shared
    glm::vec2 viewport_size;
    float near = 1.0f;
    float far = 100.0f;

    // Perspective camera only
    float fov = 90.0f;

    // Orthographic camera only
    float orthographic_scale = 1.0f;
};

struct Camera
{
  public:
    Transform transform;

  public:
    Camera(const CameraConfig& config);
    void update();
    void gui(const char* name);

    const glm::mat4& get_view_matrix() const;
    const glm::mat4& get_projection_matrix() const;
    const glm::vec3& get_forwards() const;

    CameraType get_type() const;

    float get_orthographic_scale() const;
    void set_orthographic_scale(float scale);

    float near() const;
    float far() const;
    float fov() const;
    float aspect() const;

  private:
    void set_projection();

    glm::mat4 projection_matrix_{1.0f};
    glm::mat4 view_matrix_{1.0f};
    glm::vec3 forwards_{0.0f};

    float aspect_ = 0;
    CameraConfig config_;
};