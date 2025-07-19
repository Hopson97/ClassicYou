#include "FloorManager.h"

#include <fstream>
#include <unordered_map>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <ctre.hpp>

#include "../Util/Util.h"

namespace
{
    nlohmann::json legacy_to_json(std::string& legacy_file_content)
    {
        while (auto match = ctre::search<R"(#([a-zA-Z_][a-zA-Z0-9_]*)\s*:)">(legacy_file_content))
        {
            std::string key = match.get<1>().to_string();
            legacy_file_content.replace(match.begin(), match.end(), "\"" + key + "\":");
        }

        legacy_file_content.front() = '{';
        legacy_file_content.back() = '}';

        return nlohmann::json::parse(legacy_file_content);
    }

    std::unordered_map<int, int> TEXTURE_MAP = {
        {1, 6},  // Grass
        {2, 7},  // Stucco
        {3, 0},  // ???
        {4, 2},  // Stone Wall/ Cobblestone
        {5, 0},  // ???
        {6, 0},  // ???
        {7, 0},  // ???
        {8, 0},  // ???
        {9, 0},  // ???
        {10, 0}, // ???
        {11, 0}, // ???
        {12, 0}, // ???
        {13, 0}, // ???
        {14, 0}, // ???
    };

} // namespace

// json conversion functions have to be defined outside of the anonamous namaepace to comply with
// Argument Dependent Lookup (ADL)

void from_json(const nlohmann::json& json, PolygonPlatformObject& poly)
{
    std::cout << "Parsing floor: " << json << std::endl;

    auto& points = json[0];
    auto& props = json[1];

    poly.parameters = {
        .corner_top_left = glm::vec2{points[3][0], points[3][1]} / 5.0f * (float)TILE_SIZE_F,
        .corner_top_right = glm::vec2{points[2][0], points[2][1]} / 5.0f * (float)TILE_SIZE_F,
        .corner_bottom_right = glm::vec2{points[1][0], points[1][1]} / 5.0f * (float)TILE_SIZE_F,
        .corner_bottom_left = glm::vec2{points[0][0], points[0][1]} / 5.0f * (float)TILE_SIZE_F,
    };

    std::cout << props << std::endl;

    poly.properties.base = 0;
    poly.properties.texture_bottom = TEXTURE_MAP.at(props[1]);
    poly.properties.texture_top = TEXTURE_MAP.at(props[0][0]);
    poly.properties.visible = true; // bool(attribs[0][1]);
}

struct LegacyWall
{
    WallObject wall;
    int floor = 0;
};

void from_json(const nlohmann::json& json, LegacyWall& object)
{
    Line line;
    line.start = glm::vec2{json[2], json[3]};
    line.end = glm::vec2{line.start.x + (float)json[0], line.end.x + (float)json[1]};

    line.start /= 5.0f;
    line.end /= 5.0f;

    line.start *= (float)TILE_SIZE;
    line.end *= (float)TILE_SIZE;

    object.wall.parameters.line = line;

    object.wall.properties.texture_front = TEXTURE_MAP.at(json[4][0]);
    object.wall.properties.texture_back = TEXTURE_MAP.at(json[4][1]);
    object.wall.properties.base_height = 0;
    object.wall.properties.wall_height = 1;
    //json[4][2];

    object.floor = json[5] - 1;
}

void convert_legacy_level(const std::filesystem::path& path)
{

    sf::Clock clock;

    auto legacy_file_content = read_file_to_string(path);
    auto legacy_json = legacy_to_json(legacy_file_content);

    auto time = clock.getElapsedTime().asSeconds();
    std::println("Conversion took {}s ({}ms) ", time, time * 1000.0f);

    FloorManager new_level;

    int floor_n = 0;
    std::vector<PolygonPlatformObject> legacy_floors;
    legacy_json["Floor"].get_to(legacy_floors);
    for (auto& legacy_floor : legacy_floors)
    {
        auto& floor = new_level.ensure_floor_exists(floor_n++);
        floor.objects.push_back(LevelObject{legacy_floor});
    }

    std::vector<LegacyWall> legacy_walls;
    legacy_json["walls"].get_to(legacy_walls);
    for (auto& legacy_wall : legacy_walls)
    {
        auto& floor = new_level.ensure_floor_exists(legacy_wall.floor);
        floor.objects.push_back(LevelObject{legacy_wall.wall});
    }

    auto output = new_level.serialise();

    if (output)
    {
        std::println("Succesfully serialised {} ", path.string());

        std::ofstream out_file((path.parent_path() / path.stem()).string() + ".cly");

        out_file << output;
    }

    // auto walls = legacy_json["walls"];
    // auto platforms = legacy_json["Plat"];
    // auto tri_platforms = legacy_json["TriPlat"];
    // auto dia_platforms = legacy_json["DiaPlat"];
}