#include "EditorUtils.h"


#include "../Graphics/Camera.h"

[[nodiscard]] Rectangle to_world_rectangle(const glm::vec2& position, const glm::vec2& size)
{
    return {
        .position = position,
        .size = size * TILE_SIZE_F,
    };
}

glm::vec2 snap_to_world_half_tile(const glm::vec3& world_position)
{
    glm::ivec2 scaled{world_position.x * TILE_SIZE_F, world_position.z * TILE_SIZE_F};
    return {
        std::round(scaled.x / HALF_TILE_SIZE_F) * HALF_TILE_SIZE_F,
        std::round(scaled.y / HALF_TILE_SIZE_F) * HALF_TILE_SIZE_F,
    };
}

glm::vec2 get_mouse_floor_snapped_intersect(const Camera& camera_3d,
                                            const sf::Vector2i& mouse_position, int floor)
{
    auto intersect = camera_3d.find_mouse_floor_intersect({mouse_position.x, mouse_position.y},
                                                          floor * FLOOR_HEIGHT);

    return snap_to_world_half_tile(intersect);
}
