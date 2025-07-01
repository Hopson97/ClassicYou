#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <print>
#include <vector>

#include "OpenGL/VertexArrayObject.h"

/**
 * @brief A vertex structure for 3D meshes
 *
 * @tparam TextureCoordType The type of texture coordinates used in the vertex - e.g., glm::vec2 or
 * glm::vec3.
 */
template <typename TextureCoordType>
struct Vertex3D
{
    using T = TextureCoordType;

    glm::vec3 position{0.0f};
    TextureCoordType texture_coord{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec4 colour{0.0f};

    /**
     * @brief Builds the vertex attributes for the given VertexArrayObject and BufferObject.
     *
     * @param vao The VertexArrayObject to which the attributes will be added.
     * @param vbo The BufferObject containing the vertex data.
     */
    static constexpr void build_attribs(gl::VertexArrayObject& vao, gl::BufferObject& vbo)
    {
        vao.add_vertex_buffer(vbo, sizeof(Vertex3D))
            .add_attribute(3, GL_FLOAT, offsetof(Vertex3D, position))
            .add_attribute(T::length(), GL_FLOAT, offsetof(Vertex3D, texture_coord))
            .add_attribute(3, GL_FLOAT, offsetof(Vertex3D, normal))
            .add_attribute(4, GL_FLOAT, offsetof(Vertex3D, colour));
    }
};

struct Vertex2D
{
    glm::vec2 position{0.0f};
    glm::vec2 texture_coord{0.0f};
    glm::vec4 colour{0.0f};

    static void build_attribs(gl::VertexArrayObject& vao, gl::BufferObject& vbo)
    {
        vao.add_vertex_buffer(vbo, sizeof(Vertex2D))
            .add_attribute(2, GL_FLOAT, offsetof(Vertex2D, position))
            .add_attribute(2, GL_FLOAT, offsetof(Vertex2D, texture_coord))
            .add_attribute(4, GL_FLOAT, offsetof(Vertex2D, colour));
    }
};

template <typename Vertex>
class Mesh
{
  public:
    bool buffer();
    bool update();
    const Mesh& bind() const;
    void draw_elements(GLenum draw_mode = GL_TRIANGLES) const;

    gl::VertexArrayObject& vao()
    {
        return vao_;
    }

    GLuint indices_count() const
    {
        return indices_;
    }

  public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

  private:
    gl::VertexArrayObject vao_;
    gl::BufferObject vbo_;
    gl::BufferObject ebo_;
    GLuint indices_ = 0;

    bool has_buffered_ = false;
};

template <typename Vertex>
bool Mesh<Vertex>::buffer()
{
    if (indices.empty())
    {
        return false;
    }

    vao_.reset();
    vbo_.reset();
    ebo_.reset();
    indices_ = static_cast<GLuint>(indices.size());

    // Upload the EBO indices data to the GPU and link it to the VAO
    ebo_.buffer_data(indices);
    glVertexArrayElementBuffer(vao_.id, ebo_.id);

    // Upload the data to the GPU and set up the attributes
    vbo_.buffer_data(vertices);
    Vertex::build_attribs(vao_, vbo_);
    return true;
}

template <typename Vertex>
bool Mesh<Vertex>::update()
{
    if (indices.empty())
    {
        return false;
    }

    // Ensure the indices count being updated matches what is currently in the buffer
    if (indices_ != static_cast<GLuint>(indices.size()))
    {
        std::println("Indides mis-match. Current: {} - New: {}\nRecreating mesh...", indices_,
                     indices.size());
        has_buffered_ = false;
    }

    if (!has_buffered_)
    {
        return buffer();
    }
    ebo_.buffer_sub_data(0, indices);
    vbo_.buffer_sub_data(0, vertices);
    return true;
}

template <typename Vertex>
const Mesh<Vertex>& Mesh<Vertex>::bind() const
{
    vao_.bind();
    return *this;
}

template <typename Vertex>
void Mesh<Vertex>::draw_elements(GLenum draw_mode) const
{
    assert(indices_ > 0);

    glDrawElements(draw_mode, indices_, GL_UNSIGNED_INT, nullptr);
}

/**
 * @brief Type aliases for commonly used vertex types and meshes.
 *
 */

/// @brief 3D vertex with 2D texture coordinates.
using Vertex = Vertex3D<glm::vec2>;

/// @brief 3D vertex with 3D texture coordinates.
using VertexWorldGeometry = Vertex3D<glm::vec3>;

/// @brief Mesh for 3D objects using Vertex (aka 2d texture coords)
using Mesh3D = Mesh<Vertex>;

/// @brief Mesh for 3D objects using VertexWorldGeometry (aka 3d texture coords)
using WorldGeometryMesh3D = Mesh<VertexWorldGeometry>;

/// @brief
using Mesh2D = Mesh<Vertex2D>;

[[nodiscard]] Mesh3D generate_quad_mesh(float w, float h);
[[nodiscard]] Mesh3D generate_cube_mesh(const glm::vec3& size, bool repeat_texture = false);
[[nodiscard]] Mesh3D generate_centered_cube_mesh(const glm::vec3& size);
[[nodiscard]] Mesh3D generate_terrain_mesh(int size, int edgeVertices);

[[nodiscard]] WorldGeometryMesh3D generate_wall_mesh(glm::vec2 from, glm::vec2 to,
                                                     GLuint texture_id_1, GLuint texture_id_2);
[[nodiscard]] Mesh3D generate_grid_mesh(int width, int height);