#pragma once

#include <optional>
#include <vector>

#include "../Graphics/Mesh.h"
#include "LevelObject.h"

using ObjectId = std::int32_t;

// Each level is made out of a several floors
struct Floor
{
    struct LevelMesh
    {
        ObjectId id;
        LevelObjectsMesh3D mesh;
    };

    Floor(int floor)
        : real_floor(floor)
    {
    }

    std::vector<LevelObject> objects;
    std::vector<LevelMesh> meshes;
    int real_floor = 0;
};

struct FloorManager
{
    std::vector<Floor> floors;
    int max_floor = 0;
    int min_floor = 0;

    std::optional<Floor*> find_floor(int floor_number);
    std::optional<const Floor*> find_floor(int floor_number) const;
    Floor& ensure_floor_exists(int floor_number);

    void clear();

    std::optional<nlohmann::json> serialise() const;
};

/**
 * @brief Converts legacy level files to the new format.
 * This is done by first converting the legacy format to JSON, and then the JSON is read by
 * nlohmann::json to be converted to the new format.
 * @param path
 */
void convert_legacy_level(const std::filesystem::path& path);