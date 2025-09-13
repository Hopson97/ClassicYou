#include "Mesh.h"

#include <numeric>

#include <SFML/Graphics/Image.hpp>

#include "../Editor/EditConstants.h"
#include "../Editor/LevelObjects/LevelObjectTypes.h"

namespace
{
    constexpr glm::vec3 UP = {0.0f, 1.0f, 0.0f};
    constexpr glm::vec3 DOWN = {0.0f, -1.0f, 0.0f};
    constexpr glm::vec3 LEFT = {-1.0f, 0.0f, 0.0f};
    constexpr glm::vec3 RIGHT = {1.0f, 0.0f, 0.0f};
    constexpr glm::vec3 FORWARD = {0.0f, 0.0f, 1.0f};
    constexpr glm::vec3 BACKWARD = {0.0f, 0.0f, -1.0f};

    constexpr std::array QUAD_TEXTURE_COORDS_FORWARDS = {
        glm::vec2{0.0f, 0.0f},
        glm::vec2{0.0f, 1.0f},
        glm::vec2{1.0f, 1.0f},
        glm::vec2{1.0f, 0.0f},
    };

    constexpr std::array QUAD_TEXTURE_COORDS_LEFT = {
        glm::vec2{1.0f, 0.0f},
        glm::vec2{0.0f, 0.0f},
        glm::vec2{0.0f, 1.0f},
        glm::vec2{1.0f, 1.0f},
    };

    constexpr std::array QUAD_TEXTURE_COORDS_BACK = {
        glm::vec2{1.0f, 1.0f},
        glm::vec2{1.0f, 0.0f},
        glm::vec2{0.0f, 0.0f},
        glm::vec2{0.0f, 1.0f},
    };

    constexpr std::array QUAD_TEXTURE_COORDS_RIGHT = {
        glm::vec2{0.0f, 1.0f},
        glm::vec2{1.0f, 1.0f},
        glm::vec2{1.0f, 0.0f},
        glm::vec2{0.0f, 0.0f},
    };

    void add_line_to_mesh(Mesh2D& mesh, glm::vec2 from, glm::vec2 to, const glm::vec4& colour)
    {
        mesh.vertices.push_back(Vertex2D{.position = from, .colour = colour});
        mesh.vertices.push_back(Vertex2D{.position = to, .colour = colour});

        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    }

} // namespace

// -----------------------------------
// ==== MESH GENERATION FUNCTIONS ====
// -----------------------------------

Mesh3D generate_quad_mesh(float w, float h)
{
    Mesh3D mesh;

    mesh.vertices = {{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, FORWARD},
                     {{w, 0.0f, 0.0f}, {1.0f, 0.0f}, FORWARD},
                     {{w, h, 0.0f}, {1.0f, 1.0f}, FORWARD},
                     {{0.0f, h, 0.0f}, {0.0f, 1.0f}, FORWARD}};

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

LevelObjectsMesh3D generate_cube_mesh_level(const glm::vec3& start, const glm::vec3& size,
                                            int texture, glm::u8vec4 colour)
{

    LevelObjectsMesh3D mesh;

    float w = size.x;
    float h = size.y;
    float d = size.z;

    float x = start.x;
    float y = start.y;
    float z = start.z;

    // clang-format off
    mesh.vertices = {
        {{x + w, y + h, z + d}, {w, 0.0f, texture}, FORWARD,  colour},
        {{x    , y + h, z + d}, {0.0f, 0.0f, texture}, FORWARD,  colour},
        {{x    , y    , z + d}, {0.0f, h, texture}, FORWARD,  colour},
        {{x + w, y    , z + d}, {w, h, texture}, FORWARD,  colour},

        {{x    , y + h, z + d}, {w, 0.0f, texture}, LEFT,     colour},
        {{x    , y + h, z    }, {0.0f, 0.0f, texture}, LEFT,     colour},
        {{x    , y    , z    }, {0.0f, h, texture}, LEFT,     colour},
        {{x    , y    , z + d}, {w, h, texture}, LEFT,     colour},

        {{x    , y + h, z    }, {w, 0.0f, texture}, BACKWARD, colour},
        {{x + w, y + h, z    }, {0.0f, 0.0f, texture}, BACKWARD, colour},
        {{x + w, y    , z    }, {0.0f, h, texture}, BACKWARD, colour},
        {{x    , y    , z    }, {w, h, texture}, BACKWARD, colour},

        {{x + w, y + h, z    }, {w, 0.0f, texture}, RIGHT,    colour},
        {{x + w, y + h, z + d}, {0.0f, 0.0f, texture}, RIGHT,    colour},
        {{x + w, y    , z + d}, {0.0f, h, texture}, RIGHT,    colour},
        {{x + w, y    , z    }, {w, h, texture}, RIGHT,    colour},

        {{x + w, y + h, z    }, {w, 0.0f, texture}, UP,       colour},
        {{x    , y + h, z    }, {0.0f, 0.0f, texture}, UP,       colour},
        {{x    , y + h, z + d}, {0.0f, h, texture}, UP,       colour},
        {{x + w, y + h, z + d}, {w, h, texture}, UP,       colour},

        {{x    , y    , z    }, {w, 0.0f, texture}, DOWN,     colour},
        {{x + w, y    , z    }, {0.0f, 0.0f, texture}, DOWN,     colour},
        {{x + w, y    , z + d}, {0.0f, h, texture}, DOWN,     colour},
        {{x    , y    , z + d}, {w, h, texture}, DOWN,     colour},
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

Mesh3D generate_cube_mesh(const glm::vec3& size, bool repeat_texture, glm::u8vec4 colour)
{
    Mesh3D mesh;

    float w = size.x;
    float h = size.y;
    float d = size.z;

    float txrx = repeat_texture ? w : 1.0f;
    float txry = repeat_texture ? h : 1.0f;

    // clang-format off
    mesh.vertices = {
        {{w, h, d}, {txrx, 0.0f}, FORWARD, colour},  
        {{0, h, d}, {0.0f, 0.0f}, FORWARD, colour},
        {{0, 0, d}, {0.0f, txry}, FORWARD, colour},  
        {{w, 0, d}, {txrx, txry}, FORWARD, colour},
                            
        {{0, h, d}, {txrx, 0.0f}, LEFT, colour}, 
        {{0, h, 0}, {0.0f, 0.0f}, LEFT, colour},
        {{0, 0, 0}, {0.0f, txry}, LEFT, colour}, 
        {{0, 0, d}, {txrx, txry}, LEFT, colour},
                               
        {{0, h, 0}, {txrx, 0.0f}, BACKWARD, colour}, 
        {{w, h, 0}, {0.0f, 0.0f}, BACKWARD, colour},
        {{w, 0, 0}, {0.0f, txry}, BACKWARD, colour}, 
        {{0, 0, 0}, {txrx, txry}, BACKWARD, colour},
                             
        {{w, h, 0}, {txrx, 0.0f}, RIGHT, colour},  
        {{w, h, d}, {0.0f, 0.0f}, RIGHT, colour},
        {{w, 0, d}, {0.0f, txry}, RIGHT, colour},  
        {{w, 0, 0}, {txrx, txry}, RIGHT, colour},
                              
        {{w, h, 0}, {txrx, 0.0f}, UP, colour},  
        {{0, h, 0}, {0.0f, 0.0f}, UP, colour},
        {{0, h, d}, {0.0f, txry}, UP, colour},  
        {{w, h, d}, {txrx, txry}, UP, colour},
                              
        {{0, 0, 0}, {txrx, 0.0f}, DOWN, colour}, 
        {{w, 0, 0}, {0.0f, 0.0f}, DOWN, colour},
        {{w, 0, d}, {0.0f, txry}, DOWN, colour}, 
        {{0, 0, d}, {txrx, txry}, DOWN, colour},
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
        {{w, h, d}, {1.0f, 0.0f}, FORWARD},  
        {{-w, h, d}, {0.0f, 0.0f}, FORWARD},
        {{-w, -h, d}, {0.0f, 1.0f}, FORWARD},  
        {{w, -h, d}, {1.0f, 1.0f}, FORWARD},

        {{-w, h, d}, {1.0f, 0.0f}, LEFT}, 
        {{-w, h, -d}, {0.0f, 0.0f}, LEFT},
        {{-w, -h, -d}, {0.0f, 1.0f}, LEFT}, 
        {{-w, -h, d}, {1.0f, 1.0f}, LEFT},

        {{-w, h, -d}, {1.0f, 0.0f}, BACKWARD}, 
        {{w, h, -d}, {0.0f, 0.0f}, BACKWARD},
        {{w, -h, -d}, {0.0f, 1.0f}, BACKWARD}, 
        {{-w, -h, -d}, {1.0f, 1.0f}, BACKWARD},

        {{w, h, -d}, {1.0f, 0.0f}, RIGHT},  
        {{w, h, d}, {0.0f, 0.0f}, RIGHT},
        {{w, -h, d}, {0.0f, 1.0f}, RIGHT},  
        {{w, -h, -d}, {1.0f, 1.0f}, RIGHT},

        {{w, h, -d}, {1.0f, 0.0f}, UP},  
        {{-w, h, -d}, {0.0f, 0.0f}, UP},
        {{-w, h, d}, {0.0f, 1.0f}, UP},  
        {{w, h, d}, {1.0f, 1.0f}, UP},

        {{-w, -h, -d}, {1.0f, 0.0f}, DOWN}, 
        {{w, -h, -d}, {0.0f, 0.0f}, DOWN},
        {{w, -h, d}, {0.0f, 1.0f}, DOWN}, 
        {{-w, -h, d}, {1.0f, 1.0f}, DOWN},
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

Mesh3D generate_grid_mesh(int width, int height)
{
    Mesh3D mesh;
    auto create_line = [&](const glm::vec3& begin, const glm::vec3& end, glm::vec4 colour)
    {
        mesh.vertices.push_back({.position = begin});
        mesh.vertices.push_back({.position = end});

        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
        mesh.indices.push_back(static_cast<GLuint>(mesh.indices.size()));
    };

    // Tiny offset prevents platforms/floors clipping with the grid
    auto y = -0.01f;

    for (int x = -width / 2; x <= width / 2; x++)
    {
        create_line({x, y, -width / 2}, {x, y, width / 2}, SUB_GRID_COLOUR);
    }

    for (int z = -height / 2; z <= height / 2; z++)
    {
        create_line({-height / 2, y, z}, {height / 2, y, z}, SUB_GRID_COLOUR);
    }

    return mesh;
}

Mesh2D generate_line_mesh(glm::vec2 from, glm::vec2 to)
{
    Mesh2D mesh;
    add_line_to_mesh(mesh, from, to, Colour::WHITE);
    return mesh;
}

Mesh2D generate_2d_quad_mesh(glm::vec2 position, glm::vec2 size, float texture, Direction direction)
{
    Mesh2D mesh;

    auto& p = position;
    auto& s = size;

    const auto& tex_coords = [&]()
    {
        switch (direction)
        {
            case Direction::Forward:
                return QUAD_TEXTURE_COORDS_FORWARDS;

            case Direction::Left:
                return QUAD_TEXTURE_COORDS_LEFT;

            case Direction::Back:
                return QUAD_TEXTURE_COORDS_BACK;

            case Direction::Right:
                return QUAD_TEXTURE_COORDS_RIGHT;
        }
        return QUAD_TEXTURE_COORDS_FORWARDS;
    }();

    // clang-format off
    mesh.vertices = {
        {.position = {p.x,       p.y        }, .texture_coord = {tex_coords[0].x, tex_coords[0].y, texture}, .colour = {1, 1, 1, 0.9}},
        {.position = {p.x,       p.y + s.y  }, .texture_coord = {tex_coords[1].x, tex_coords[1].y, texture}, .colour = {1, 1, 1, 0.9}},
        {.position = {p.x + s.x, p.y + s.y  }, .texture_coord = {tex_coords[2].x, tex_coords[2].y, texture}, .colour = {1, 1, 1, 0.9}},
        {.position = {p.x + s.x, p.y        }, .texture_coord = {tex_coords[3].x, tex_coords[3].y, texture}, .colour = {1, 1, 1, 0.9}},
    };
    // clang-format on

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

Mesh2D generate_2d_outline_quad_mesh(glm::vec2 position, glm::vec2 size)
{
    Mesh2D mesh;
    add_line_to_mesh(mesh, {position.x, position.y}, {position.x + size.x, position.y},
                     Colour::WHITE);
    add_line_to_mesh(mesh, {position.x + size.x, position.y},
                     {position.x + size.x, position.y + size.y}, Colour::WHITE);
    add_line_to_mesh(mesh, {position.x + size.x, position.y + size.y},
                     {position.x, position.y + size.y}, Colour::WHITE);
    add_line_to_mesh(mesh, {position.x, position.y + size.y}, {position.x, position.y},
                     Colour::WHITE);
    return mesh;
}
