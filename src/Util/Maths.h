#pragma once

#include <array>

#include <SFML/System/Angle.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Vector
{
    constexpr inline glm::vec3 UP = {0.0, 1.0, 0.0};
    constexpr inline glm::vec3 DOWN = {0.0, -1.0, 0.0};
    constexpr inline glm::vec3 LEFT = {-1.0, 0.0, 0.0};
    constexpr inline glm::vec3 RIGHT = {1.0, 0.0, 0.0};
    constexpr inline glm::vec3 FORWARDS = {0.0, 0.0, 1.0};
    constexpr inline glm::vec3 BACK = {0.0, 0.0, -1.0};
} // namespace Vector

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

    [[nodiscard]] bool is_entirely_within(const Rectangle& other);
    [[nodiscard]] bool contains(const glm::vec2& point);
};

struct Line
{
    glm::vec2 start{0};
    glm::vec2 end{0};

    [[nodiscard]] Rectangle to_bounds() const;
};

[[nodiscard]] glm::mat4 create_model_matrix(const Transform& transform);
[[nodiscard]] glm::mat4 create_model_matrix_orbit(const Transform& transform,
                                                  const glm::vec3& origin);
[[nodiscard]] glm::vec3 forward_vector(const glm::vec3& rotation);
[[nodiscard]] glm::vec3 backward_vector(const glm::vec3& rotation);
[[nodiscard]] glm::vec3 left_vector(const glm::vec3& rotation);
[[nodiscard]] glm::vec3 right_vector(const glm::vec3& rotation);

[[nodiscard]] glm::vec3 forward_vector_flat(const glm::vec3& rotation);
[[nodiscard]] glm::vec3 backward_vector_flat(const glm::vec3& rotation);

[[nodiscard]] float distance_to_line(const glm::vec2& point, const Line& line);

[[nodiscard]] glm::vec2 rotate_around(glm::vec2 point, glm::vec2 rotation_origin, float degrees);

[[nodiscard]] bool point_in_triangle(glm::vec2 point, const std::array<glm::vec2, 3>& triangle);
