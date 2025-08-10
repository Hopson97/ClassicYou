#pragma once

#include <optional>
#include <vector>

#include "../Graphics/Mesh.h"
#include "LevelObjects/LevelObject.h"

class LevelFileIO;

/// Represents a single floor in a level.
struct Floor
{

    struct LevelMesh
    {
        /// Reference to the LevelObject's ID such that the mesh can be recreated if the object is
        /// modified.
        ObjectId id;

        /// The mesh for the level object.
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

/// Wrapper for managing multiple floors in a level.
struct FloorManager
{
    std::vector<Floor> floors;
    int max_floor = 0;
    int min_floor = 0;

    std::optional<Floor*> find_floor(int floor_number);
    std::optional<const Floor*> find_floor(int floor_number) const;

    /// @brief Adds a new floor to the manager if it does not already exist.
    /// This currently assumes that a new floor will only be 1 above/below the current max/min
    /// floor. (TODO: Change this?)
    Floor& ensure_floor_exists(int floor_number);

    /// @brief Clears all floors and resets the manager.
    void clear();

    /// Serialise all of the floors into a JSON object.
    std::optional<nlohmann::json> serialise(LevelFileIO& level_file_io) const;
};

/**
 * @brief Converts legacy level files to the new format.
 * This is done by first converting the legacy format to JSON, and then the JSON is read by
 * nlohmann::json to be converted to the new format.
 * @param path
 */
void convert_legacy_level(const std::filesystem::path& path);