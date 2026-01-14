#include "FloorManager.h"

#include <fstream>
#include <unordered_map>

#include "LevelFileIO.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <ctre.hpp>

#include "../Util/Util.h"

namespace
{
    // Must be global to work with the "from_json" functions (only visible in this file)
    LevelFileIO g_level_file_io;

    // Map for colour from the legacy format to the new format
    // For Platforms, TriPlats, DiaPlats, Floors, Ramps
    // Mapping is (Legacy <-> ClassicYou)
    const std::unordered_map<int, int> TEXTURE_MAP = {
        {1, 6},   // Grass
        {2, 14},  // Stucco
        {3, 0},   // Red Brick
        {4, 1},   // Stone Brick
        {5, 12},  // Wood
        {6, 17},  // "happy"
        {7, 15},  // Egypt texture
        {8, 8},   // Glass
        {9, 10},  // Bark
        {10, 18}, // Sci-Fi
        {11, 19}, // Tiles
        {13, 13}, // Rock
        {15, 21}, // Parquet
    };

    // Map for colour from the legacy format to the new format
    // For Walls, Pillars, TriWall
    // Mapping is (Legacy <-> ClassicYou)
    const std::unordered_map<int, int> TEXTURE_MAP_WALL = {
        {1, 0},   // Red bricks
        {2, 4},   // Bars
        {3, 1},   // Stone Brick
        {4, 6},   // Grass
        {5, 12},  // Wood
        {6, 17},  // "happy"
        {7, 15},  // Egypt texture
        {8, 8},   // Glass
        {9, 14},  // Stucco
        {10, 10}, // Bark
        {11, 18}, // Sci-Fi
        {12, 19}, // Tiles
        {13, 13}, // Rock
        {14, 20}, // Books
        {16, 21}, // Parquet
    };

    constexpr std::array<float, 10> MIN_WALL_HEIGHTS = {0, 0, 0, 0, 1, 2, 3, 2, 1, 1};
    constexpr std::array<float, 10> MAX_WALL_HEIGHTS = {4, 3, 2, 1, 2, 3, 4, 4, 4, 3};
    constexpr std::array<float, 10> PLATFORM_HEIGHTS = {0, 1, 2, 3};

    // Mappings using the TriWall direction property
    constexpr std::array<glm::vec2, 4> TRI_WALL_OFFSETS = {
        glm::vec2{0, -4 * TILE_SIZE_F},
        {0, 4 * TILE_SIZE_F},
        {-4 * TILE_SIZE_F, 0},
        {4 * TILE_SIZE_F, 0},
    };
    constexpr std::array<glm::vec2, 4> TRIWALL_START_OFFSETS = {
        glm::vec2{0, 4 * TILE_SIZE_F},
        {0, -4 * TILE_SIZE_F},
        {4 * TILE_SIZE_F, 0},
        {-4 * TILE_SIZE_F, 0},
    };

    // In the new versions have a height and an offset, but in the legacy version these were
    // "hardcoded IDs" from 0 to 9, so these must be converted into a {base, height} pair
    std::pair<float, float> extract_wall_base_and_height(const nlohmann::json& height)
    {
        float min = MIN_WALL_HEIGHTS[height - 1] / 4.0f;
        float max = MAX_WALL_HEIGHTS[height - 1] / 4.0f;

        return {min, max - min};
    }

    TextureProp map_texture(const std::unordered_map<int, int>& mapping,
                            const nlohmann::json& legacy_texture)
    {
        if (legacy_texture.is_array())
        {
            return {
                .id = 16,
                .colour = {legacy_texture[0], legacy_texture[1], legacy_texture[2], 255},
            };
        }
        else
        {
            auto itr = mapping.find(legacy_texture);
            if (itr != mapping.cend())
            {
                return {.id = itr->second};
            }
        }

        return {.id = legacy_texture};
    }

    TextureProp map_texture(const nlohmann::json& legacy_texture)
    {
        return map_texture(TEXTURE_MAP, legacy_texture);
    }

    TextureProp map_wall_texture(const nlohmann::json& legacy_texture)
    {
        return map_texture(TEXTURE_MAP_WALL, legacy_texture);
    }

    Direction map_triangle_platform_direction(int legacy_direction)
    {
        // clang-format off
        switch (legacy_direction)
        {
            case 1:     return Direction::Left;
            case 2:     return Direction::Back;
            case 3:     return Direction::Forward;
            case 4:     return Direction::Right;
            default:    return Direction::Right;
        }
        // clang-format on
    }

    // Converts a legacy ChallengeYou.com level format to JSON
    auto legacy_to_json(std::string& legacy_file_content)
    {
        // Replace the #name: to "name:" to be JSON compliant
        while (auto match = ctre::search<R"(#([a-zA-Z_][a-zA-Z0-9_]*)\s*:)">(legacy_file_content))
        {
            std::string key = match.get<1>().to_string();
            legacy_file_content.replace(match.begin(), match.end(), "\"" + key + "\":");
        }

        while (auto match =
                   ctre::search<R"(color\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\))">(
                       legacy_file_content))
        {
            std::string r = match.get<1>().to_string();
            std::string g = match.get<2>().to_string();
            std::string b = match.get<3>().to_string();

            legacy_file_content.replace(match.begin(), match.end(),
                                        "[" + r + "," + g + "," + b + "]");
        }

        // Wrap the JSON with { } to be read as an object
        legacy_file_content.front() = '{';
        legacy_file_content.back() = '}';

        // Parse the string
        return nlohmann::json::parse(legacy_file_content);
    }

    auto extract_vec2(float x, float y)
    {
        // The legacy format has each square as 5 units wide, so the values must be divided by this
        return glm::vec2{x, y} / 5.0f * TILE_SIZE_F;
    }

} // namespace

/*
 * The JSON conversion functions have to be defined outside of the anonymous namespace to comply
 * with Argument Dependent Lookup (ADL)
 */

// ===========================================================
//      Floor/ Ground Conversion (to PolygonPlatformObject)
// ============================================================
//  [[X0, Y0, X1, Y1, X2, Y2, X3, Y3], [[TextureTop, Visible], TextureBottom]
void from_json(const nlohmann::json& json, PolygonPlatformObject& poly)
{
    auto& points = json[0];
    auto& props = json[1];

    //poly.parameters = {.corner_top_left = extract_vec2(points[3][0], points[3][1]),
    //                   .corner_top_right = extract_vec2(points[2][0], points[2][1]),
    //                   .corner_bottom_right = extract_vec2(points[1][0], points[1][1]),
    //                   .corner_bottom_left = extract_vec2(points[0][0], points[0][1])};

    poly.properties.base = 0;
    poly.properties.texture_bottom = map_texture(props[1]);
    poly.properties.texture_top = map_texture(props[0][0]);

    poly.properties.visible = (props[0][1] == 1);
}

// =========================
//      Wall Conversion
// ==========================
struct LegacyWall
{
    WallObject object;
    int floor = 0;
};
//  [OffsetX, OffsetY, StartX, StartY, [TextureBack, TextureFront], Floor]
//  [OffsetX, OffsetY, StartX, StartY, [TextureBack, TextureFront, Height], Floor]
void from_json(const nlohmann::json& json, LegacyWall& wall)
{
    auto& wall_params = wall.object.parameters;
    auto& wall_props = wall.object.properties;

    auto start = extract_vec2(json[2], json[3]);
    auto offset = extract_vec2(json[0], json[1]);
    wall_params.line.start = start;
    wall_params.line.end = start + offset;

    wall_props.texture_front = map_wall_texture(json[4][1]);
    wall_props.texture_back = map_wall_texture(json[4][0]);

    wall_props.start_base_height = 0;
    wall_props.start_height = 1;

    wall_props.end_base_height = 0;
    wall_props.end_height = 1;

    if (json[4].size() == 3)
    {
        auto [base, height] = extract_wall_base_and_height(json[4][2]);
        wall_props.start_base_height = base;
        wall_props.end_base_height = base;
        wall_props.start_height = height;
        wall_props.end_height = height;
    }

    wall.floor = (int)json[5] - 1;
}

// =========================
//      TriWall Conversion
// ==========================
struct LegacyTriWall
{
    WallObject object;
    int floor = 0;
};

//  [X, Y], [Flipped, Texture, direction], Floor]
void from_json(const nlohmann::json& json, LegacyTriWall& wall)
{
    auto& wall_params = wall.object.parameters;
    auto& wall_props = wall.object.properties;

    auto start = extract_vec2(json[0][0], json[0][1]);

    // TODO: Directions 5,6,7,8 are diagonal and need handling
    int direction = json[1][2];
    auto offset = direction <= 4 ? TRI_WALL_OFFSETS[direction - 1] : glm::vec2{0, -2 * TILE_SIZE_F};
    start += direction <= 4 ? TRIWALL_START_OFFSETS[direction - 1] : glm::vec2{0, 0};

    wall_params.line.start = start;
    wall_params.line.end = start + offset;

    bool flip_wall = json[1][0] == 2;
    wall_props.style = flip_wall ? WallStyle::FlippedTriWall : WallStyle::TriWall;

    wall_props.texture_front = map_wall_texture(json[1][1]);
    wall_props.texture_back = map_wall_texture(json[1][1]);

    wall_props.start_base_height = 0;
    wall_props.start_height = 1;
    wall_props.end_base_height = 0;
    wall_props.end_height = 1;

    wall.floor = (int)json[2] - 1;
}

// ============================
//      Platform Conversion
// ============================
namespace
{
    // All platforms are nearly indentical in their property layout, so this generic method works
    // for all
    template <typename LegacyPlatformType>
    void load_basic_platform(const nlohmann::json& json, LegacyPlatformType& platform,
                             int height_index)
    {
        auto& position = json[0];
        auto& props = json[1];

        auto& platform_params = platform.object.parameters;
        auto& platform_props = platform.object.properties;

        auto size = std::powf(2.0f, static_cast<float>(props[0]));
        platform_props.width = size;
        platform_props.depth = size;

        platform_params.position = extract_vec2(position[0], position[1]) -
                                   glm::vec2{(platform_props.width * TILE_SIZE_F) / 2.0f};

        if (props.size() > 1)
        {
            platform_props.texture_bottom = map_texture(props[1]);
            platform_props.texture_top = platform_props.texture_bottom;

            if (props.size() > 2)
            {
                platform_props.base = PLATFORM_HEIGHTS[(int)props[height_index] - 1] / 4.0f;
            }
        }
        else
        {
            // Platforms default to wood textures in older versions
            platform_props.texture_bottom.id = 12;
            platform_props.texture_top.id = 12;
        }

        platform.floor = (int)json[2] - 1;
    }
} // namespace

struct LegacyPlatform
{
    PlatformObject object;
    int floor = 0;
};

// [[X, Y], [Size], Floor]
// [[X, Y], [Size, Texture], Floor]
// [[X, Y], [Size, Texture, Height], Floor]
void from_json(const nlohmann::json& json, LegacyPlatform& platform)
{
    load_basic_platform(json, platform, 2);
}

// ============================
//      TriPlat Conversion
// ============================
struct LegacyTriPlatform
{
    PlatformObject object;
    int floor = 0;
};

// [[X, Y], [Size, Direction], Floor]
// [[X, Y], [Size, Texture, Direction], Floor]
// [[X, Y], [Size, Texture, Direction, Height], Floor]
void from_json(const nlohmann::json& json, LegacyTriPlatform& platform)
{
    load_basic_platform(json, platform, 3);
    auto& props = json[1];
    auto& platform_props = platform.object.properties;

    int direction_index = props.size() > 2 ? 2 : 1;
    platform_props.style = PlatformStyle::Triangle;
    platform_props.direction = map_triangle_platform_direction(props[direction_index]);
}

// ============================
//      DiaPlat Conversion
// ============================
struct LegacyDiaPlatform
{
    PlatformObject object;
    int floor = 0;
};

// [[X, Y], [Size], Floor]
// [[X, Y], [Size, Texture], Floor]
// [[X, Y], [Size, Texture, Height], Floor]
void from_json(const nlohmann::json& json, LegacyDiaPlatform& platform)
{
    load_basic_platform(json, platform, 2);
    platform.object.properties.style = PlatformStyle::Diamond;
}

// ============================
//      Pillar Conversion
// ============================
struct LegacyPillar
{
    PillarObject object;
    int floor = 0;
};

// [[X, Y], [Angled], Floor]
// [[X, Y], [Angled, Texture], Floor]
// [[X, Y], [Angled, Texture, Size, Height], Floor]
void from_json(const nlohmann::json& json, LegacyPillar& pillar)
{
    auto& position = json[0];
    auto& props = json[1];

    auto& pillar_params = pillar.object.parameters;
    auto& pillar_props = pillar.object.properties;

    pillar_params.position = extract_vec2(position[0], position[1]);

    pillar_props.angled = (props[0] == 1);
    if (props.size() > 1)
    {
        pillar_props.texture = map_wall_texture(props[1]);

        if (props.size() > 2)
        {
            // TODO: Is divide by 6 the correct factor?
            pillar_props.size = (float)props[2] / 6.0f;

            auto [base, height] = extract_wall_base_and_height(props[3]);
            pillar_props.base_height = base;
            pillar_props.height = height;
        }
    }
    else
    {
        // Pillars default to brick textures in older versions
        pillar_props.texture.id = 0;
    }

    pillar.floor = (int)json[2] - 1;
}

// ============================
//      Ramp Conversion
// ============================
struct LegacyRamp
{
    RampObject object;
    int floor = 0;
};

//[[X, Y], [Direction], Floor]
//[[X, Y], [Direction, Texture], Floor]
void from_json(const nlohmann::json& json, LegacyRamp& ramp)
{
    auto& position = json[0];
    auto& props = json[1];

    auto& ramp_params = ramp.object.parameters;
    auto& ramp_props = ramp.object.properties;

    ramp_params.position = extract_vec2(position[0], position[1]);

    int direction = props[0];
    switch (direction)
    {
        case 1:
            ramp_props.depth = 4;
            ramp_props.width = 2;
            ramp_props.direction = Direction::Forward; // confirmed
            ramp_params.position.x -= TILE_SIZE_F;
            break;
        case 2:
            ramp_props.depth = 4;
            ramp_props.width = 2;
            ramp_props.direction = Direction::Back;
            ramp_params.position.y -= 4 * TILE_SIZE_F;
            ramp_params.position.x -= TILE_SIZE_F;
            break;
        case 3:
            ramp_props.depth = 2;
            ramp_props.width = 4;
            ramp_params.position.y -= TILE_SIZE_F;
            ramp_props.direction = Direction::Left; // confirmed
            break;
        case 4:
            ramp_props.depth = 2;
            ramp_props.width = 4;
            ramp_props.direction = Direction::Right;
            ramp_params.position.x -= 4 * TILE_SIZE_F;
            ramp_params.position.y -= TILE_SIZE_F;

            break;

        default:
            ramp_props.depth = 1;
            ramp_props.width = 1;
            ramp_props.direction = Direction::Right;
            break;
    }

    if (props.size() > 1)
    {
        ramp_props.texture_top = map_texture(props[1]);
        ramp_props.texture_bottom = map_texture(props[1]);
    }
    else
    {
        // Default to wood plank
        ramp_props.texture_top.id = 12;
        ramp_props.texture_bottom.id = 12;
    }

    ramp.floor = (int)json[2] - 1;
}

// =========================================
//      End of object conversion
// =========================================
template <typename T>
void load_objects(const nlohmann::json& json, const char* json_key, FloorManager& new_level)
{
    if (json.find(json_key) == json.cend())
    {
        return;
    }

    std::vector<T> objects;
    objects.reserve(json[json_key].size());
    json[json_key].get_to(objects);
    for (auto& object : objects)
    {
        auto& floor = new_level.ensure_floor_exists(object.floor);
        floor.objects.push_back(LevelObject{object.object});
    }
}

void load_floors(const nlohmann::json& json, FloorManager& new_level)
{
    int floor_n = 0;
    std::vector<PolygonPlatformObject> legacy_floors;
    json["Floor"].get_to(legacy_floors);
    for (auto& legacy_floor : legacy_floors)
    {
        auto& floor = new_level.ensure_floor_exists(floor_n++);
        floor.objects.push_back(LevelObject{legacy_floor});
    }
}

void convert_legacy_level(const std::filesystem::path& path)
{
    // Reset the level file io object
    g_level_file_io = LevelFileIO{};

    std::println("Converting {}", path.string());
    sf::Clock clock;

    auto legacy_file_content = read_file_to_string(path);
    auto legacy_json = legacy_to_json(legacy_file_content);

    auto time = clock.restart().asSeconds();
    std::println("Converting to JSON took {}s ({}ms)", time, time * 1000.0f);

    // Uncomment to inspect the converted JSON
    // std::ofstream out_file_og((path.parent_path() / path.stem()).string() + ".json");
    // out_file_og << legacy_json;
    clock.restart();

    // Begin conversion from JSON to new format
    FloorManager new_level;

    // First load the floors
    load_floors(legacy_json, new_level);

    // Extract geometric object
    load_objects<LegacyWall>(legacy_json, "walls", new_level);
    load_objects<LegacyPlatform>(legacy_json, "Plat", new_level);
    load_objects<LegacyPillar>(legacy_json, "Pillar", new_level);
    load_objects<LegacyTriWall>(legacy_json, "TriWall", new_level);
    load_objects<LegacyRamp>(legacy_json, "Ramp", new_level); // TO-DO - diagonal ramps
    load_objects<LegacyTriPlatform>(legacy_json, "TriPlat", new_level);
    load_objects<LegacyDiaPlatform>(legacy_json, "DiaPlat", new_level);

    // After level is loaded, write it the JSON object
    auto output = new_level.serialise(g_level_file_io);

    time = clock.restart().asSeconds();
    std::println("Converting to ClassicYou format took {}s ({}ms)", time, time * 1000.0f);

    // And finally, save it to the file.
    if (output)
    {
        g_level_file_io.write_floors(output);
        g_level_file_io.save(path.stem().string(), false);

        std::println("Successfully serialised {}\n\n", path.string());
    }

    // auto tri_platforms = legacy_json["TriPlat"];
    // auto dia_platforms = legacy_json["DiaPlat"];
}