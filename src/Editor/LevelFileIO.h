#pragma once

#include "LevelObjects/LevelObjectTypes.h"
#include <nlohmann/json.hpp>

/// Helper class for loading and saving editor level files.
class LevelFileIO
{
  public:
    /// Opens the given file ready to be deserialised
    bool open(const std::string& level_file_name, bool load_uncompressed);

    /// Save the current seriliased json to disk + metadata about the level. "write_floors" should
    /// be called first.
    bool save(const std::string& level_file_name, bool save_uncompressed);

    /// Writes the floors to the current json. This assumes floors is an arry of floors and their
    /// objects (See FloorManager)
    void write_floors(const nlohmann::json& floors);

    /// Gets the "floors" object from the json object.
    nlohmann::json get_floors() const;

    /// Write a TextureProp to the current JSON. This caches the colour if is a new one, otherwise
    /// it saves the index of a previously saved colour.
    void serialise_texture(nlohmann::json& object, const TextureProp& prop);

    /// Gets a TextureProp, loading the colour from the cache
    TextureProp deserialise_texture(const nlohmann::json& object) const;

  private:
    int find_colour_index(glm::u8vec4 colour) const;
    int add_colour(glm::u8vec4 colour);

    std::vector<glm::u8vec4> colours_;

    int version_ = 0;

    nlohmann::json json_;
};
