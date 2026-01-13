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

    constexpr auto direction_to_texture_coords(Direction direction)
    {
        // clang-format off
        switch (direction)
        {
            case Direction::Forward:    return QUAD_TEXTURE_COORDS_FORWARDS;
            case Direction::Left:       return QUAD_TEXTURE_COORDS_LEFT;
            case Direction::Back:       return QUAD_TEXTURE_COORDS_BACK;
            case Direction::Right:      return QUAD_TEXTURE_COORDS_RIGHT;
        }
        // clang-format on
        return QUAD_TEXTURE_COORDS_FORWARDS;
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

Mesh2DWorld generate_line_mesh(glm::vec2 from, glm::vec2 to, glm::u8vec4 colour)
{
    Mesh2DWorld mesh;
    add_line_to_mesh(mesh, from, to, colour);
    return mesh;
}

Mesh2DWorld generate_2d_quad_mesh(glm::vec2 position, glm::vec2 size, float base_texture,
                                  float world_texture, glm::u8vec4 colour, Direction direction)
{
    Mesh2DWorld mesh;

    auto& p = position;
    auto& s = size;

    const auto& tex_coords = direction_to_texture_coords(direction);

    float width = size.x / TILE_SIZE;
    float depth = size.y / TILE_SIZE;

    // clang-format off
    mesh.vertices = {
        {.position = {p.x,       p.y      }, .texture_coord = {tex_coords[0].x, tex_coords[0].y, base_texture}, .world_texture_coord = {0,     0,     world_texture}, .colour = colour},
        {.position = {p.x,       p.y + s.y}, .texture_coord = {tex_coords[1].x, tex_coords[1].y, base_texture}, .world_texture_coord = {0,     depth, world_texture}, .colour = colour},
        {.position = {p.x + s.x, p.y + s.y}, .texture_coord = {tex_coords[2].x, tex_coords[2].y, base_texture}, .world_texture_coord = {width, depth, world_texture}, .colour = colour},
        {.position = {p.x + s.x, p.y      }, .texture_coord = {tex_coords[3].x, tex_coords[3].y, base_texture}, .world_texture_coord = {width, 0,     world_texture}, .colour = colour},
    };
    // clang-format on

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

std::array<glm::vec2, 3> generate_2d_triangle_vertex_positions(glm::vec2 position, glm::vec2 size,
                                                               Direction direction)
{
    auto& p = position;
    auto& s = size;
    return direction_to_triangle_vertices<glm::vec2>(
        NamedQuadVertices<glm::vec2>{.top_left = {p.x, p.y},
                                     .bottom_left = {p.x, p.y + s.y},
                                     .bottom_right = {p.x + s.x, p.y + s.y},
                                     .top_right = {p.x + s.x, p.y}},
        direction);
}

Mesh2DWorld generate_2d_triangle_mesh(glm::vec2 position, glm::vec2 size, float base_texture,
                                      float world_texture, glm::u8vec4 colour, Direction direction)
{
    Mesh2DWorld mesh;

    auto& p = position;

    const auto& tex_coords = direction_to_texture_coords(direction);

    auto v = generate_2d_triangle_vertex_positions(position, size, direction);

    // World texture coords must be remapped such that rotating the triangle verts does not rotate
    // the texture
    glm::vec3 world_tex0{(v[0].x - p.x) / TILE_SIZE, (v[0].y - p.y) / TILE_SIZE, world_texture};
    glm::vec3 world_tex1{(v[1].x - p.x) / TILE_SIZE, (v[1].y - p.y) / TILE_SIZE, world_texture};
    glm::vec3 world_tex2{(v[2].x - p.x) / TILE_SIZE, (v[2].y - p.y) / TILE_SIZE, world_texture};

    // clang-format off
    mesh.vertices = {
        {.position = v[0], .texture_coord = {tex_coords[0].x, tex_coords[0].y, base_texture}, .world_texture_coord = world_tex0, .colour = colour},
        {.position = v[1], .texture_coord = {tex_coords[1].x, tex_coords[1].y, base_texture}, .world_texture_coord = world_tex1, .colour = colour},
        {.position = v[2], .texture_coord = {tex_coords[2].x, tex_coords[2].y, base_texture}, .world_texture_coord = world_tex2, .colour = colour},
    };
    // clang-format on

    mesh.indices = {0, 1, 2};

    return mesh;
}

Mesh2DWorld generate_2d_diamond_mesh(glm::vec2 position, glm::vec2 size, float base_texture,
                                     float world_texture, glm::u8vec4 colour, Direction direction)
{
    Mesh2DWorld mesh;

    auto& p = position;
    auto& s = size;

    const auto& tex_coords = direction_to_texture_coords(direction);

    float width = size.x / TILE_SIZE;
    float depth = size.y / TILE_SIZE;

    // clang-format off
    mesh.vertices = {
        {.position = {p.x + s.x,     p.y + s.y / 2  }, .texture_coord = {tex_coords[0].x, tex_coords[0].y, base_texture}, .world_texture_coord = {width,     depth / 2, world_texture}, .colour = colour},
        {.position = {p.x + s.x / 2, p.y            }, .texture_coord = {tex_coords[1].x, tex_coords[1].y, base_texture}, .world_texture_coord = {width / 2, 0,         world_texture}, .colour = colour},
        {.position = {p.x,           p.y + s.y / 2  }, .texture_coord = {tex_coords[2].x, tex_coords[2].y, base_texture}, .world_texture_coord = {0,         depth / 2, world_texture}, .colour = colour},
        {.position = {p.x + s.x / 2, p.y + s.y      }, .texture_coord = {tex_coords[3].x, tex_coords[3].y, base_texture}, .world_texture_coord = {width / 2, depth,     world_texture}, .colour = colour},
    };
    // clang-format on

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

Mesh2DWorld generate_2d_outline_quad_mesh(glm::vec2 position, glm::vec2 size)
{
    glm::u8vec4 colour = {255, 255, 255, 255};
    Mesh2DWorld mesh;
    add_line_to_mesh(mesh, {position.x, position.y}, {position.x + size.x, position.y}, colour);
    add_line_to_mesh(mesh, {position.x + size.x, position.y},
                     {position.x + size.x, position.y + size.y}, colour);
    add_line_to_mesh(mesh, {position.x + size.x, position.y + size.y},
                     {position.x, position.y + size.y}, colour);
    add_line_to_mesh(mesh, {position.x, position.y + size.y}, {position.x, position.y}, colour);
    return mesh;
}
