#pragma once

#include "Mesh.h"

[[nodiscard]] Mesh3D generate_quad_mesh(float w, float h);

[[nodiscard]] LevelObjectsMesh3D generate_cube_mesh_level(const glm::vec3& start,
                                                          const glm::vec3& size, int texture,
                                                          glm::u8vec4 colour = Colour::WHITE);

[[nodiscard]] Mesh3D generate_cube_mesh(const glm::vec3& size, bool repeat_texture = false,
                                        glm::u8vec4 colour = Colour::WHITE);
[[nodiscard]] Mesh3D generate_pyramid_mesh(const glm::vec3& size,
                                           glm::u8vec4 colour_a = Colour::WHITE,
                                           glm::u8vec4 colour_b = Colour::WHITE);

[[nodiscard]] Mesh3D generate_centered_cube_mesh(const glm::vec3& size);
[[nodiscard]] Mesh3D generate_terrain_mesh(int size, int edgeVertices);
[[nodiscard]] Mesh3D generate_grid_mesh(int width, int height);


void add_line_to_mesh_3d(Mesh3D& mesh, const Line3D& line, glm::vec4 colour);

template <typename MeshType>
void add_line_to_mesh(MeshType& mesh, const Line& line, glm::u8vec4 colour)
{
    mesh.vertices.push_back({.position = line.start, .colour = colour});
    mesh.vertices.push_back({.position = line.end, .colour = colour});

    mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
}

void generate_line_mesh(Mesh2DWorld& mesh, const Line& line, glm::u8vec4 colour);
void generate_line_mesh(Mesh3D& mesh, const Line3D& line, glm::vec4 colour);

[[nodiscard]] Mesh2DWorld generate_line_mesh(const Line& line, glm::u8vec4 colour);
[[nodiscard]] Mesh2DWorld generate_2d_quad_mesh(glm::vec2 position, glm::vec2 size,
                                                float base_texture, float world_texture = 0,
                                                glm::u8vec4 colour = Colour::WHITE,
                                                Direction direction = Direction::Forward);

[[nodiscard]] Mesh2DWorld generate_2d_triangle_mesh(glm::vec2 position, glm::vec2 size,
                                                    float base_texture, float world_texture = 0,
                                                    glm::u8vec4 colour = Colour::WHITE,
                                                    Direction direction = Direction::Forward);

[[nodiscard]] Mesh2DWorld generate_2d_diamond_mesh(glm::vec2 position, glm::vec2 size,
                                                   float base_texture, float world_texture = 0,
                                                   glm::u8vec4 colour = Colour::WHITE,
                                                   Direction direction = Direction::Forward);

[[nodiscard]] Mesh2DWorld generate_2d_outline_quad_mesh(glm::vec2 position, glm::vec2 size);

template <typename T>
struct NamedQuadVertices
{
    T top_left;
    T bottom_left;
    T bottom_right;
    T top_right;
};

template <typename T>
auto direction_to_triangle_vertices(const NamedQuadVertices<T>& vertices, Direction direction)
{
    std::array<T, 3> v;

    // clang-format off
    switch (direction)
    {
        case Direction::Right:   v = {vertices.top_left,     vertices.bottom_left,  vertices.bottom_right}; break;
        case Direction::Left:    v = {vertices.bottom_left,  vertices.bottom_right, vertices.top_right};    break;
        case Direction::Forward: v = {vertices.bottom_right, vertices.top_right,    vertices.top_left};     break;
        case Direction::Back:    v = {vertices.top_right,    vertices.top_left,     vertices.bottom_left};  break;
    }
    // clang-format on
    return v;
}

std::array<glm::vec2, 3> generate_2d_triangle_vertex_positions(glm::vec2 position, glm::vec2 size,
                                                               Direction direction);
