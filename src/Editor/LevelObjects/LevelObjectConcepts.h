#pragma once

#include <concepts>
#include <glm/glm.hpp>

template <typename T>
concept ResizableObject = requires(T t) {
    { t.properties.size } -> std::same_as<glm::vec2&>;
    { t.parameters.position } -> std::same_as<glm::vec2&>;
};
