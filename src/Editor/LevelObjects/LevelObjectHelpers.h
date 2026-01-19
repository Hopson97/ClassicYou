#pragma once

#include "../../Util/Maths.h"
#include "../EditConstants.h"

template <typename T>
[[nodiscard]] Rectangle object_to_rectangle(const T& object)
{
    return {
        .position = object.parameters.position,
        .size = object.properties.size * (float)(TILE_SIZE),
    };
}