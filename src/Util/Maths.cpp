#include "Maths.h"

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

float distance_to_line(const glm::vec2& point, const Line& line)
{
    auto length = glm::distance2(line.start, line.end);
    if (length == 0)
    {
        return 1234.0f;
    }

    // Consider the line extline.ending the segment, parameterized as v + t (w - v).
    // Find projection of point p onto the line.
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    auto t = ((point.x - line.start.x) * (line.end.x - line.start.x) +
              (point.y - line.start.y) * (line.end.y - line.start.y)) /
             length;

    // Clamp to handle points that fall outside of the line
    t = glm::clamp(t, 0.0f, 1.0f);

    auto dist = glm::distance2(point, {line.start.x + t * (line.end.x - line.start.x),
                                       line.start.y + t * (line.end.y - line.start.y)});

    return glm::sqrt(dist);
}

Rectangle Line::to_bounds() const
{
    return Rectangle{
        .position = {std::min(start.x, end.x), std::min(start.y, end.y)},
        .size = {std::abs(start.x - end.x), std::abs(start.y - end.y)},
    };
}

bool Rectangle::is_entirely_within(const Rectangle& other)
{
    return other.position.x <= position.x && other.position.y <= position.y &&
           other.position.x + other.size.x >= position.x + size.x &&
           other.position.y + other.size.y >= position.y + size.y;
}
