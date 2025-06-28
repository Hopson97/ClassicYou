#include "Mesh.h"

#include <numeric>

#include <SFML/Graphics/Image.hpp>

#include "../Editor/EditConstants.h"

const InstanceBatch& InstanceBatch::bind() const
{
    mesh.bind();
    return *this;
}

void InstanceBatch::buffer()
{
    mesh.buffer();
    transform_vbo_.reset();
    transform_vbo_.buffer_data(transforms);
    mesh.vao()
        .add_vertex_buffer(transform_vbo_, sizeof(glm::mat4))
        .add_instance_attribute(4, GL_FLOAT, sizeof(glm::vec4), 1, 4);
}

void InstanceBatch::draw_elements(GLenum draw_mode) const
{
    glDrawElementsInstanced(draw_mode, mesh.indices_count(), GL_UNSIGNED_INT, nullptr,
                            transforms.size());
}

void InstanceBatch::update()
{
}

// -----------------------------------
// ==== MESH GENERATION FUNCTIONS ====
// -----------------------------------

Mesh3D generate_quad_mesh(float w, float h)
{
    Mesh3D mesh;
    mesh.vertices = {{{w, h, 0.0f}, {0.0f, 1.0f}, {0, 0, 1}},
                     {{0, h, 0.0f}, {1.0f, 1.0f}, {0, 0, 1}},
                     {{0, 0, 0.0f}, {1.0f, 0.0f}, {0, 0, 1}},
                     {{w, 0, 0.0f}, {0.0f, 0.0f}, {0, 0, 1}}

    };

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

Mesh3D generate_cube_mesh(const glm::vec3& dimensions, bool repeat_texture)
{
    Mesh3D mesh;

    float w = dimensions.x;
    float h = dimensions.y;
    float d = dimensions.z;

    float txrx = repeat_texture ? w : 1.0f;
    float txry = repeat_texture ? h : 1.0f;

    // clang-format off
    mesh.vertices = {
        {{w, h, d}, {txrx, 0.0f}, {0.0f, 0.0f, 1.0f}},  
        {{0, h, d}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{0, 0, d}, {0.0f, txry}, {0.0f, 0.0f, 1.0f}},  
        {{w, 0, d}, {txrx, txry}, {0.0f, 0.0f, 1.0f}},

        {{0, h, d}, {txrx, 0.0f}, {-1.0f, 0.0f, 0.0f}}, 
        {{0, h, 0}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{0, 0, 0}, {0.0f, txry}, {-1.0f, 0.0f, 0.0f}}, 
        {{0, 0, d}, {txrx, txry}, {-1.0f, 0.0f, 0.0f}},

        {{0, h, 0}, {txrx, 0.0f}, {0.0f, 0.0f, -1.0f}}, 
        {{w, h, 0}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{w, 0, 0}, {0.0f, txry}, {0.0f, 0.0f, -1.0f}}, 
        {{0, 0, 0}, {txrx, txry}, {0.0f, 0.0f, -1.0f}},

        {{w, h, 0}, {txrx, 0.0f}, {1.0f, 0.0f, 0.0f}},  
        {{w, h, d}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{w, 0, d}, {0.0f, txry}, {1.0f, 0.0f, 0.0f}},  
        {{w, 0, 0}, {txrx, txry}, {1.0f, 0.0f, 0.0f}},

        {{w, h, 0}, {txrx, 0.0f}, {0.0f, 1.0f, 0.0f}},  
        {{0, h, 0}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0, h, d}, {0.0f, txry}, {0.0f, 1.0f, 0.0f}},  
        {{w, h, d}, {txrx, txry}, {0.0f, 1.0f, 0.0f}},

        {{0, 0, 0}, {txrx, 0.0f}, {0.0f, -1.0f, 0.0f}}, 
        {{w, 0, 0}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{w, 0, d}, {0.0f, txry}, {0.0f, -1.0f, 0.0f}}, 
        {{0, 0, d}, {txrx, txry}, {0.0f, -1.0f, 0.0f}},
    };
    // clang-format on

    int index = 0;
    for (int i = 0; i < 6; i++)
    {
        mesh.indices.push_back(index);
        mesh.indices.push_back(index + 1);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 3);
        mesh.indices.push_back(index);
        index += 4;
    }

    return mesh;
}

Mesh3D generate_centered_cube_mesh(const glm::vec3& dimensions)
{
    Mesh3D mesh;

    float w = dimensions.x;
    float h = dimensions.y;
    float d = dimensions.z;

    // clang-format off
    mesh.vertices = {
        {{w, h, d}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  
        {{-w, h, d}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-w, -h, d}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  
        {{w, -h, d}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

        {{-w, h, d}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, 
        {{-w, h, -d}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-w, -h, -d}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}}, 
        {{-w, -h, d}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},

        {{-w, h, -d}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, 
        {{w, h, -d}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{w, -h, -d}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, 
        {{-w, -h, -d}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

        {{w, h, -d}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},  
        {{w, h, d}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{w, -h, d}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},  
        {{w, -h, -d}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        {{w, h, -d}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  
        {{-w, h, -d}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-w, h, d}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  
        {{w, h, d}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},

        {{-w, -h, -d}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, 
        {{w, -h, -d}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{w, -h, d}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}}, 
        {{-w, -h, d}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
    };
    // clang-format on

    int index = 0;
    for (int i = 0; i < 6; i++)
    {
        mesh.indices.push_back(index);
        mesh.indices.push_back(index + 1);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 3);
        mesh.indices.push_back(index);
        index += 4;
    }

    return mesh;
}

Mesh3D generate_terrain_mesh(int size, int edgeVertices)
{
    float fEdgeVertexCount = static_cast<float>(edgeVertices);

    Mesh3D mesh;
    for (int z = 0; z < edgeVertices; z++)
    {
        for (int x = 0; x < edgeVertices; x++)
        {
            GLfloat fz = static_cast<GLfloat>(z);
            GLfloat fx = static_cast<GLfloat>(x);

            Vertex vertex;
            vertex.position.x = fx / fEdgeVertexCount * size;
            vertex.position.y = 0.0f;
            vertex.position.z = fz / fEdgeVertexCount * size;

            vertex.texture_coord.s = (fx / fEdgeVertexCount) * edgeVertices / 4.0f;
            vertex.texture_coord.t = (fz / fEdgeVertexCount) * edgeVertices / 4.0f;

            vertex.normal = {0, 1, 0};

            mesh.vertices.push_back(vertex);
        }
    }

    for (int z = 0; z < edgeVertices - 1; z++)
    {
        for (int x = 0; x < edgeVertices - 1; x++)
        {
            int topLeft = (z * edgeVertices) + x;
            int topRight = topLeft + 1;
            int bottomLeft = ((z + 1) * edgeVertices) + x;
            int bottomRight = bottomLeft + 1;

            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    return mesh;
}

Mesh3D generate_wall_mesh(glm::vec2 from, glm::vec2 to)
{
    // Begin
    auto b = glm::vec3{from.x, 0, from.y} / static_cast<float>(TILE_SIZE);

    // End
    auto e = glm::vec3{to.x, 0, to.y} / static_cast<float>(TILE_SIZE);

    Mesh3D mesh;
    auto ox = 0.0f;
    auto oz = 0.0f;
    auto ob = 0.0f;
    auto h = 1.0f;

    const auto length = glm::length(b - e);

    mesh.vertices = {
        // Front
        {{b.x + ox, ob, b.z + oz}, {0.0f, ob}, {0, 0, 1}},
        {{b.x + ox, h, b.z + oz}, {0.0, h}, {0, 0, 1}},
        {{e.x + ox, h, e.z + oz}, {length, h}, {0, 0, 1}},
        {{e.x + ox, ob, e.z + oz}, {length, ob}, {0, 0, 1}},

        // Back
        {{b.x - ox, ob, b.z - oz}, {0.0f, ob}, {0, 0, 1}},
        {{b.x - ox, h, b.z - oz}, {0.0, h}, {0, 0, 1}},
        {{e.x - ox, h, e.z - oz}, {length, h}, {0, 0, 1}},
        {{e.x - ox, ob, e.z - oz}, {length, ob}, {0, 0, 1}},

    };

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}

Mesh3D generate_grid_mesh(int width, int height)
{
    Mesh3D mesh;
    auto create_line = [&](const glm::vec3& begin, const glm::vec3& end)
    {
        mesh.vertices.push_back({.position = begin, .colour = {1, 1, 1, 1}});
        mesh.vertices.push_back({.position = end, .colour = {1, 1, 1, 1}});

        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    };

    // Tiny offset prevents platforms/floors clipping with the grid
    auto y = -0.01f;

    for (int x = 0; x <= width; x++)
    {
        create_line({x, y, 0}, {x, y, width});
    }

    for (int z = 0; z <= height; z++)
    {
        create_line({0, y, z}, {height, y, z});
    }
    return mesh;
}
