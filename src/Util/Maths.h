#pragma once

#include <array>

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform
{
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct Rectangle
{
    glm::vec2 position{0};
    glm::vec2 size{0};

    bool is_entirely_within(const Rectangle& other);
};

struct Line
{
    glm::vec2 start{0};
    glm::vec2 end{0};

    Rectangle to_bounds() const;
};

constexpr inline auto rgb_to_normalised(const glm::vec3& rgb)
{
    return glm::vec4(rgb / 255.0f, 1.0f);
}

glm::mat4 create_model_matrix(const Transform& transform);
glm::mat4 create_model_matrix_orbit(const Transform& transform, const glm::vec3& origin);
glm::vec3 forward_vector(const glm::vec3& rotation);
glm::vec3 backward_vector(const glm::vec3& rotation);
glm::vec3 left_vector(const glm::vec3& rotation);
glm::vec3 right_vector(const glm::vec3& rotation);

glm::vec3 forward_vector_flat(const glm::vec3& rotation);
glm::vec3 backward_vector_flat(const glm::vec3& rotation);

float distance_to_line(const glm::vec2& point, const Line& line);