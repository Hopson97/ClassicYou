#pragma once

#include "../../Graphics/Mesh.h"

class LevelTextures;

/**
 * All Level objects must specialize these functions. These are called via std::visit in
 * LevelObject.cpp
 */
template <typename T>
[[nodiscard]] LevelObjectsMesh3D object_to_geometry(const T& object, int floor_number);

template <typename T>
[[nodiscard]] std::pair<Mesh2D, gl::PrimitiveType>
object_to_geometry_2d(const T& object, const LevelTextures& drawing_pad_texture_map);

template <typename T>
[[nodiscard]] std::string object_to_string(const T& object);

template <typename T>
void render_object_2d(const T& object, DrawingPad& pad, const glm::vec4& colour,
                      const glm::vec2& selected_offset = {0, 0});

template <typename T>
[[nodiscard]] bool object_try_select_2d(const T& object, glm::vec2 selection_tile);

template <typename T>
[[nodiscard]] bool object_is_within(const T& object, const Rectangle& selection_area);

template <typename T>
void object_move(T& object, glm::vec2 offset);

template <typename T>
void object_rotate(T& object, glm::vec2 rotation_origin, float degrees);

template <typename T>
[[nodiscard]] glm::vec2 object_get_position(const T& object);

template <typename T>
SerialiseResponse object_serialise(const T& object, LevelFileIO& level_file_io);
