#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Util/Maths.h"
#include "EditConstants.h"

class Camera;

[[nodiscard]] Rectangle to_world_rectangle(const glm::vec2& position, const glm::vec2& size);

[[nodiscard]] glm::vec2 snap_to_world_half_tile(const glm::vec3& world_position);
[[nodiscard]] glm::vec2 get_mouse_floor_snapped_intersect(const Camera& camera_3d,
                                                          const sf::Vector2i& mouse_position,
                                                          int floor);

template <typename T>
[[nodiscard]] Rectangle object_to_rectangle(const T& object)
{
    return to_world_rectangle(object.parameters.position, object.properties.size);
}