#include "Maths.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <print>

Rectangle Line::to_bounds() const
{
    return Rectangle{
        .position = {std::min(start.x, end.x), std::min(start.y, end.y)},
        .size = {std::abs(start.x - end.x), std::abs(start.y - end.y)},
    };
}

bool Rectangle::is_entirely_within(const Rectangle& other) const
{
    return other.position.x <= position.x && other.position.y <= position.y &&
           other.position.x + other.size.x >= position.x + size.x &&
           other.position.y + other.size.y >= position.y + size.y;
}

bool Rectangle::contains(const glm::vec2& point) const
{
    return point.x >= position.x && point.x <= position.x + size.x && point.y >= position.y &&
           point.y <= position.y + size.y;
}

bool Rectangle::contains(const std::vector<glm::vec2>& points, const glm::vec2& points_offset) const
{

    for (auto& point : points)
    {
        if (!contains(point + points_offset))
        {
            return false;
        }
    }
    return true;
}

glm::mat4 create_model_matrix(const Transform& transform)
{
    glm::mat4 matrix{1.0f};

    matrix = glm::translate(matrix, transform.position);
    matrix = glm::scale(matrix, transform.scale);

    matrix = glm::rotate(matrix, glm::radians(transform.rotation.x), {1, 0, 0});
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.y), {0, 1, 0});
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.z), {0, 0, 1});

    return matrix;
}

glm::mat4 create_model_matrix_orbit(const Transform& transform, const glm::vec3& origin)
{
    glm::mat4 matrix{1.0f};

    matrix = glm::translate(matrix, transform.position);
    matrix = glm::scale(matrix, transform.scale);

    matrix = glm::translate(matrix, origin);
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.x), {1, 0, 0});
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.y), {0, 1, 0});
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.z), {0, 0, 1});
    matrix = glm::translate(matrix, -origin);

    return matrix;
}

glm::vec3 forward_vector(const glm::vec3& rotation)
{
    float yaw = glm::radians(rotation.y);
    float pitch = glm::radians(rotation.x);

    return {
        glm::cos(yaw) * glm::cos(pitch),
        glm::sin(pitch),
        glm::cos(pitch) * glm::sin(yaw),
    };
}

glm::vec3 backward_vector(const glm::vec3& rotation)
{
    return -forward_vector(rotation);
}

glm::vec3 left_vector(const glm::vec3& rotation)
{
    float yaw = glm::radians(rotation.y + 90);
    return {
        -glm::cos(yaw),
        0,
        -glm::sin(yaw),
    };
}

glm::vec3 right_vector(const glm::vec3& rotation)
{
    return -left_vector(rotation);
}

glm::vec3 forward_vector_flat(const glm::vec3& rotation)
{
    float yaw = glm::radians(rotation.y);

    return {
        glm::cos(yaw),
        0,
        glm::sin(yaw),
    };
}

glm::vec3 backward_vector_flat(const glm::vec3& rotation)
{
    return -forward_vector_flat(rotation);
}

glm::vec2 closest_point_on_line(const glm::vec2& point, const Line& line)
{
    auto line_length = glm::distance2(line.start, line.end);
    if (line_length == 0)
    {
        return {10000, 10000};
    }

    // Calculate the projection of the point onto the line segment
    auto v = line.end - line.start;
    auto t = glm::clamp(glm::dot(point - line.start, v) / line_length, 0.0f, 1.0f);


    return line.start + t * v;
}

float distance_to_line(const glm::vec2& point, const Line& line)
{
    return glm::distance(point, closest_point_on_line(point, line));
}

glm::vec2 rotate_around(glm::vec2 point, glm::vec2 rotation_origin, float degrees)
{
    // Note: to make anti-clockwise rotation, swap point and rotation_origin
    // glm::rotate(rotation_origin - point, glm::radians(degrees)) + rotation_origin;

    return glm::rotate(point - rotation_origin, glm::radians(degrees)) + rotation_origin;
}

bool point_in_triangle(glm::vec2 point, const std::array<glm::vec2, 3>& triangle)
{
    // clang-format off
    // Gets the distance of `point` from the given edge  
    // > 0 means left side 
    // < 0 means right side
    auto edge = [&](glm::vec2 a, glm::vec2 b)
    { 
        return (point.x - a.x) * (b.y - a.y) - 
               (point.y - a.y) * (b.x - a.x); 
    };
    // clang-format on

    // Calculate the position/distance the of point on each side of the triangle
    float e1 = edge(triangle[0], triangle[1]);
    float e2 = edge(triangle[1], triangle[2]);
    float e3 = edge(triangle[2], triangle[0]);

    // Check distances. All must be on the SAME side to be inside the triangle.
    bool has_negative = (e1 < 0) || (e2 < 0) || (e3 < 0);
    bool has_positive = (e1 > 0) || (e2 > 0) || (e3 > 0);

    return !(has_negative && has_positive);
}

bool point_in_polygon(const glm::vec2& point, const std::vector<glm::vec2>& points,
                      const glm::vec2& points_offset)
{
    // Raycast intersection test
    unsigned intersections = 0;
    for (size_t i = 0; i < points.size(); ++i)
    {
        auto p1 = points[i] + points_offset;
        auto p2 = points[(i + 1) % points.size()] + points_offset;

        if ((point.y > std::min(p1.y, p2.y)) && (point.y <= std::max(p1.y, p2.y)))
        {
            float x_intersect = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);

            if (x_intersect > point.x)
            {
                intersections++;
            }
        }
    }

    // Point is inside if intersections count is odd
    return intersections % 2 == 1;
}
