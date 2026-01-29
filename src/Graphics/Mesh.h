#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <print>
#include <vector>

#include "../Editor/EditConstants.h"
#include "../Editor/LevelObjects/LevelObjectTypes.h"
#include "../Util/Maths.h"
#include "OpenGL/GLUtils.h"
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
    glm::u8vec4 colour{255};

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
            .add_attribute(4, GL_UNSIGNED_BYTE, offsetof(Vertex3D, colour), true);
    }
};

struct Vertex2DWorld
{
    glm::vec2 position{0.0f};
    glm::vec3 texture_coord{0.0f};

    // When displaying world textures in the 2D view, this enables the textures to repeat rather
    // than being stretched
    glm::vec3 world_texture_coord{0.0f};
    glm::u8vec4 colour{255};

    static void build_attribs(gl::VertexArrayObject& vao, gl::BufferObject& vbo)
    {
        vao.add_vertex_buffer(vbo, sizeof(Vertex2DWorld))
            .add_attribute(2, GL_FLOAT, offsetof(Vertex2DWorld, position))
            .add_attribute(3, GL_FLOAT, offsetof(Vertex2DWorld, texture_coord))
            .add_attribute(3, GL_FLOAT, offsetof(Vertex2DWorld, world_texture_coord))
            .add_attribute(4, GL_UNSIGNED_BYTE, offsetof(Vertex2DWorld, colour), true);
    }
};

struct Vertex2D
{
    glm::vec2 position{0.0f};
    glm::vec2 texture_coord{0.0f};
    glm::u8vec4 colour{255};

    static void build_attribs(gl::VertexArrayObject& vao, gl::BufferObject& vbo)
    {
        vao.add_vertex_buffer(vbo, sizeof(Vertex2D))
            .add_attribute(2, GL_FLOAT, offsetof(Vertex2D, position))
            .add_attribute(2, GL_FLOAT, offsetof(Vertex2D, texture_coord))
            .add_attribute(4, GL_UNSIGNED_BYTE, offsetof(Vertex2D, colour), true);
    }
};

template <typename Vertex>
class Mesh
{
  public:
    /// Buffer the mesh
    bool buffer();

    /// Update the mesh if it is already buffered, otherwise creates a new buffer
    bool update();

    /// Binf the mesh ready for drawing
    const Mesh& bind() const;

    /// Does glDrawElements. Fails if the indices are empty
    void draw_elements(gl::PrimitiveType primitive = gl::PrimitiveType::Triangles) const;

    gl::VertexArrayObject& vao()
    {
        return vao_;
    }

    GLuint indices_count() const
    {
        return indices_;
    }

    bool has_buffered() const
    {
        return has_buffered_;
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
    has_buffered_ = true;
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
void Mesh<Vertex>::draw_elements(gl::PrimitiveType primitive) const
{
    assert(indices_ > 0);

    glDrawElements(static_cast<GLenum>(primitive), indices_, GL_UNSIGNED_INT, nullptr);
}

/// 3D vertex with 2D texture coordinates.
using Vertex = Vertex3D<glm::vec2>;

/// 3D vertex with 3D texture coordinates.
using VertexLevelObjects = Vertex3D<glm::vec3>;

/// Mesh for 3D objects using Vertex (aka 2d texture coords)
using Mesh3D = Mesh<Vertex>;

/// Mesh for 3D objects using VertexLevelObjects (aka 3d texture coords)
using LevelObjectsMesh3D = Mesh<VertexLevelObjects>;

/// Mesh for 2D meshes representing world objects
using Mesh2DWorld = Mesh<Vertex2DWorld>;

/// Mesh for 2D meshes
using Mesh2D = Mesh<Vertex2D>;

