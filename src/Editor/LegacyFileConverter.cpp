#include "FloorManager.h"

#include <fstream>
#include <unordered_map>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <ctre.hpp>

#include "../Util/Util.h"

namespace
{
    // Map for colour from the legacy format to the new format
    // For platforms, floors
    const std::unordered_map<int, int> TEXTURE_MAP = {
        {1, 6},  // Grass
        {2, 14}, // Stucco
        {3, 0},  // Red Brick
        {4, 1},  // Stone Brick
        {5, 12}, //
        // {6, 0}, // "happy"
        {7, 15}, // Egypt texture
        {8, 8},  // Glass
        {9, 10}, // Bark
        // {10, 10}, // Sci-Fi
        // {11, 11}, // Tiles
        {13, 13}, // Rock
                  //  {15, 13}, // Parquet
    };

    // Map for colour from the legacy format to the new format
    // For walls, pillars
    const std::unordered_map<int, int> TEXTURE_MAP_WALL = {
        {1, 0},  // Red bricks
        {2, 4},  // Bars
        {3, 1},  // Stone Brick
        {4, 6},  // Grass
        {5, 12}, // Wood
        // {6, 0}, // "happy"
        {7, 15},  // Egypt texture
        {8, 8},   // Glass
        {9, 14},  // Stucco
        {10, 10}, // Bark

        // {11, 11}, // Sci-Fi
        // {12, 12}, // Tiles
        {13, 13}, // Rock
        // {14, 14}, // Books
        //  {16, 16}, // Parquet

    };

    TextureProp map_texture(const std::unordered_map<int, int>& mapping,
                            nlohmann::json legacy_texture)
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

    TextureProp map_texture(nlohmann::json legacy_texture)
    {
        return map_texture(TEXTURE_MAP, legacy_texture);
    }

    TextureProp map_wall_texture(nlohmann::json legacy_texture)
    {
        return map_texture(TEXTURE_MAP_WALL, legacy_texture);
    }

    // Converts a legacy ChallengeYou.com level format to JSON
    auto legacy_to_json(std::string& legacy_file_content)
    {
        // Replace the #name: to "name":
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
        // before being multiplied to TILE_SIZE to map to the new format
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

    poly.parameters = {.corner_top_left = extract_vec2(points[3][0], points[3][1]),
                       .corner_top_right = extract_vec2(points[2][0], points[2][1]),
                       .corner_bottom_right = extract_vec2(points[1][0], points[1][1]),
                       .corner_bottom_left = extract_vec2(points[0][0], points[0][1])};

    poly.properties.base = 0;
    poly.properties.texture_bottom = map_texture(props[1]);
    poly.properties.texture_top = map_texture(props[0][0]);

    poly.properties.visible = (props[0][1] == 1);
}

// =========================
//      Wall Conversion
// ==========================

constexpr std::array<float, 10> MIN_WALL_HEIGHTS = {0, 0, 0, 0, 1, 2, 3, 2, 1, 1};
constexpr std::array<float, 10> MAX_WALL_HEIGHTS = {4, 3, 2, 1, 2, 3, 4, 4, 4, 3};


struct LegacyWall
{
    WallObject object;
    int floor = 0;
};

//  [OffsetX, OffsetY, StartX, StartY, [TextureBack, TextureFront], Floor]
//  [OffsetX, OffsetY, StartX, StartY, [TextureBack, TextureFront, Height], Floor]
void from_json(const nlohmann::json& json, LegacyWall& wall)
{
    auto start = extract_vec2(json[2], json[3]);
    auto offset = extract_vec2(json[0], json[1]);
    wall.object.parameters.line.start = start;
    wall.object.parameters.line.end = start + offset;

    wall.object.properties.texture_front = map_wall_texture(json[4][1]);
    wall.object.properties.texture_back = map_wall_texture(json[4][0]);
    wall.object.properties.base_height = 0;
    wall.object.properties.wall_height = 1;

    if (json[4].size() == 3)
    {
        // int height = json[4][2];
        // wall.object.properties.base_height = MIN_WALL_HEIGHTS[height] / 4.0f;
        // 
        // int max = MAX_WALL_HEIGHTS[height] / 4.0f;
        // wall.object.properties.wall_height = wall.object.properties.base_height - max;
    }

    wall.floor = (int)json[5] - 1;
}

// ============================
//      Platform Conversion
// ============================
struct LegacyPlatform
{
    PlatformObject object;
    int floor = 0;
};

// [[X, Y], [Size], Floor]
// [[X, Y], [Size, Texture, Height], Floor]
void from_json(const nlohmann::json& json, LegacyPlatform& platform)
{
    auto& position = json[0];
    auto& props = json[1];

    auto& platform_params = platform.object.parameters;
    auto& platform_props = platform.object.properties;

    platform_props.width = static_cast<float>(props[0]) * 2;
    platform_props.depth = static_cast<float>(props[0]) * 2;

    platform_params.position = extract_vec2(position[0], position[1]) -
                               glm::vec2{TILE_SIZE_F * platform_props.width / 2.0f};

    if (props.size() > 1)
    {
        platform_props.texture_bottom = map_texture(props[1]);
        platform_props.texture_top = platform_props.texture_bottom;

        if (props.size() > 2)
        {
            platform_props.base = 0; // 1 -> 0.0, 2 -> 0.25, 3 -> 0.5, 4 -> 0.75
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

// =========================================
//      End of object conversion
// =========================================
template <typename T>
void load_objects(const nlohmann::json& json, const char* json_key, FloorManager& new_level)
{
    std::vector<T> objects;
    objects.reserve(json[json_key].size());
    json[json_key].get_to(objects);
    for (auto& object : objects)
    {
        auto& floor = new_level.ensure_floor_exists(object.floor);
        floor.objects.push_back(LevelObject{object.object});
    }
}

void convert_legacy_level(const std::filesystem::path& path)
{
    std::println("Converting {}", path.string());
    sf::Clock clock;

    auto legacy_file_content = read_file_to_string(path);
    auto legacy_json = legacy_to_json(legacy_file_content);

    auto time = clock.restart().asSeconds();
    std::println("Converting to JSON took {}s ({}ms)", time, time * 1000.0f);
    std::ofstream out_file_og((path.parent_path() / path.stem()).string() + ".json");
    out_file_og << legacy_json;
    clock.restart();

    // Begin conversion from JSON to new format
    FloorManager new_level;

    int floor_n = 0;
    std::vector<PolygonPlatformObject> legacy_floors;
    legacy_json["Floor"].get_to(legacy_floors);
    for (auto& legacy_floor : legacy_floors)
    {
        auto& floor = new_level.ensure_floor_exists(floor_n++);
        floor.objects.push_back(LevelObject{legacy_floor});
    }

    load_objects<LegacyWall>(legacy_json, "walls", new_level);
    load_objects<LegacyPlatform>(legacy_json, "Plat", new_level);

    auto output = new_level.serialise();
    time = clock.restart().asSeconds();
    std::println("Converting to ClassicYou format took {}s ({}ms)\n", time, time * 1000.0f);

    if (output)
    {
        std::println("Successfully serialised {} ", path.string());

        std::ofstream out_file(("levels" / path.stem()).string() + ".cly");

        out_file << output;
    }

    // auto walls = legacy_json["walls"];
    // auto platforms = legacy_json["Plat"];
    // auto tri_platforms = legacy_json["TriPlat"];
    // auto dia_platforms = legacy_json["DiaPlat"];
}