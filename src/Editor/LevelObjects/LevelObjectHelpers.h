#pragma once

#include "../../Util/Maths.h"
#include "../EditConstants.h"

[[nodiscard]] inline Rectangle to_world_rectangle(const glm::vec2& position, const glm::vec2& size)
{
    return {
        .position = position,
        .size = size * TILE_SIZE_F,
    };
}

template <typename T>
[[nodiscard]] Rectangle object_to_rectangle(const T& object)
{
    return to_world_rectangle(object.parameters.position, object.properties.size);
}