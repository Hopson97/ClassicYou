#pragma once

#include "LevelObjects/LevelObjectTypes.h"
#include <nlohmann/json.hpp>

/// Helper class for loading and saving editor level files.
class LevelFileIO
{
  public:
    bool open(const std::string& level_file_name, bool load_uncompressed);
    bool save(const std::string& level_file_name, bool save_uncompressed);

    void write_floors(const nlohmann::json& floors);
    nlohmann::json get_floors() const;

    void serialise_texture(nlohmann::json& object, const TextureProp& prop);
    TextureProp deserialise_texture(const nlohmann::json& object) const;

  private:
    std::optional<std::string> compress_data(const std::string& json_dump);
    std::optional<std::string> decompress_from_file(const std::filesystem::path& path, std::size_t size);

    int find_colour_index(glm::u8vec4 colour) const;
    int add_colour(glm::u8vec4 colour);

    std::vector<glm::u8vec4> colours_;

    int version_ = 0;

    nlohmann::json json_;
};
