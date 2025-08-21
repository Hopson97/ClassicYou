#pragma once

#include "LevelObjects/LevelObjectTypes.h"
#include <nlohmann/json.hpp>


/// Suffix to determine folders that may be in the user data that are for internal use only
const static std::string INTERNAL_FILE_ID = "_#";

/// Checks if the given level file exists. Assumes the file is in the "levels" directory, and
/// without any file extension.
bool level_file_exists(const std::string level_file_name);

/// @brief  Checks if the given level file exists. Assumes the file is in the "levels" directory.
bool level_file_exists(const std::filesystem::path level_file_name);

/// Helper class for loading and saving editor level files.
class LevelFileIO
{
  public:
    /// Opens the given file ready to be deserialised
    bool open(const std::string& level_file_name, bool load_uncompressed);

    /// Save the current serialised json to disk + metadata about the level. "write_floors" should
    /// be called first.
    bool save(const std::string& level_file_name, bool save_uncompressed);

    /// Writes the floors to the current json. This assumes floors is an array of floors and their
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

class LevelFileSelectGUI
{
    struct LevelOptions
    {
        // Directory within the levels/ folder where the level data is
        const std::string directory;

        // Display name of the level - might be different from the directory name
        const std::string display_name;

        // Datetime string for when the level was last saved.
        const std::string saved_date;
    };

  public:
    void show();
    void hide();
    bool is_showing() const;

    std::optional<std::string> display_level_select_gui();

  private:
    std::vector<LevelOptions> levels_;

    bool is_showing_ = false;
};
