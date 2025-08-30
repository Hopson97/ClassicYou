#include "LevelFileIO.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <fstream>
#include <zlib.h>

#include "../Util/ImGuiExtras.h"
#include "../Util/Util.h"

namespace
{
    std::optional<std::string> compress_data(const std::string& json_dump)
    {
        // Get the size of the compressed data
        std::string compressed;
        auto size = static_cast<uLong>(json_dump.size());
        auto compressed_length = compressBound(size);
        compressed.resize(compressed_length);

        int result = compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressed_length,
                              reinterpret_cast<const Bytef*>(json_dump.c_str()), size);
        if (result != Z_OK)
        {
            std::cerr << "Error compressing data" << std::endl;
            return {};
        }
        compressed.resize(compressed_length);
        return compressed;
    }

    std::optional<std::string> decompress_from_file(const std::filesystem::path& path,
                                                    std::size_t size)
    {
        // Open the binary file
        std::ifstream compressed_file(path.string(), std::ios::binary);
        if (!compressed_file.is_open())
        {
            std::println(std::cerr, "Could not open level file for {}", path.string());
            return {};
        }

        // Read the contents to a string
        std::string contents((std::istreambuf_iterator<char>(compressed_file)),
                             std::istreambuf_iterator<char>());

        // Decompress the data a string
        std::string decompressed;
        decompressed.resize(size);
        auto size_zlib = static_cast<uLongf>(size);
        int result = uncompress(reinterpret_cast<Bytef*>(&decompressed[0]), &size_zlib,
                                reinterpret_cast<const Bytef*>(contents.c_str()),
                                static_cast<uLongf>(contents.size()));
        // Checks
        if (result != Z_OK)
        {
            std::println(std::cerr, "Error decompressing file: {}", path.string());
            return {};
        }
        return decompressed;
    }

    std::filesystem::path make_level_directory_path(const std::string& level_name)
    {
        return "levels/" + level_name + "/";
    }

    std::filesystem::path make_level_path(const std::string& level_name)
    {
        return make_level_directory_path(level_name) / std::string(level_name + ".cly");
    }

    std::filesystem::path make_uncompressed_level_path(const std::string& level_name)
    {
        return make_level_directory_path(level_name) / std::string(level_name + ".cly.json");
    }

    std::filesystem::path make_meta_path(const std::string& level_name)
    {
        return make_level_directory_path(level_name) / std::string(level_name + "_meta.json");
    }

    std::optional<nlohmann::json> get_metafile_content(const std::string level_name)
    {
        // Load metadata from the metafile
        std::ifstream meta_file(make_meta_path(level_name));
        if (!meta_file.is_open())
        {
            std::println(std::cerr, "Could not open metafile for {}", level_name);
            return {};
        }
        return nlohmann::json::parse(meta_file);
    }

} // namespace

bool level_file_exists(const std::string level_name)
{
    return std::filesystem::exists(make_level_directory_path(level_name));
}

bool level_file_exists(const std::filesystem::path level_name)
{
    return level_file_exists(level_name.stem().string());
}

bool LevelFileIO::open(const std::string& level_name, bool load_uncompressed)
{
    //==========
    //  Helpers
    // =========
    size_t size = 0;
    auto load_metadata = [&](const nlohmann::json& meta)
    {
        version_ = meta["version"];
        size = meta["size"];
    };

    auto load_colours = [&]()
    {
        for (auto& object : json_["colours"])
        {
            colours_.push_back({object[0], object[1], object[2], object[3]});
        }
    };

    //====================================
    //  Load from the uncompressed format
    // ===================================
    if (load_uncompressed)
    {
        auto path = make_uncompressed_level_path(level_name);
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::println(std::cerr, "Could not open file {}", path.string());
            return false;
        }
        json_ = nlohmann::json::parse(file);
        load_metadata(json_["meta"]);
        load_colours();

        return true;
    }

    //====================================
    //  Load from the compressed format
    // ===================================
    auto path = make_level_path(level_name);

    // Load metadata from the metafile
    if (auto metadata = get_metafile_content(level_name))
    {
        load_metadata(*metadata);

        // Load objects from the binary file
        if (auto decompressed = decompress_from_file(path, size))
        {
            json_ = nlohmann::json::parse(*decompressed);

            // Load additional data
            load_colours();

            std::println("Successfully opened {}", path.string());
            return true;
        }
        return false;
    }
    else
    {
        std::println(std::cerr, "Could not open metafile for {}", level_name);
        return false;
    }
}

bool LevelFileIO::save(const std::string& level_name, bool save_uncompressed)
{
    auto path = make_level_path(level_name);

    // Add additional data to the compressed JSON prior to saving
    json_["colours"] = {};
    for (auto& colour : colours_)
    {
        json_["colours"].push_back({colour.r, colour.g, colour.b, colour.a});
    }

    // Convert to a string ready for saving
    auto json_dump = json_.dump();

    // All level files are saved to a a directory.
    // TODO: Sanatise the level name to ensure the directory is a valid filename across all
    // platforms
    auto directory = path.parent_path();
    if (!std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
    }

    //===========================
    //      Write the metadata
    // ==========================
    nlohmann::json meta;
    meta["level_name"] = level_name;
    meta["version"] = version_;
    meta["size"] = json_dump.size();
    meta["saved_date"] = get_epoch();

    // Persistent data between saves
    if (auto current_meta = get_metafile_content(level_name))
    {
        meta["created_date"] = (*current_meta)["created_date"];
    }
    else
    {
        meta["created_date"] = meta["saved_date"];
    }

    std::ofstream meta_file(make_meta_path(level_name));
    meta_file << meta;
    json_["meta"] = meta;

    //==================================
    //  Save to the uncompressed format
    // =================================
    if (save_uncompressed)
    {
        std::ofstream basic_file(make_uncompressed_level_path(level_name.c_str()));
        basic_file << json_;
    }

    //================================
    //  Save to the compressed format
    // ===============================
    auto compressed = compress_data(json_dump);
    if (compressed)
    {
        std::ofstream compressed_file(path.string(), std::ios::binary);
        compressed_file << *compressed;
        std::println("Successfully saved to {}", path.string());
        return true;
    }
    return false;
}

void LevelFileIO::serialise_texture(nlohmann::json& object, const TextureProp& prop)
{
    auto colour_index = find_colour_index(prop.colour);
    if (colour_index == -1)
    {
        colour_index = add_colour(prop.colour);
    }
    object.push_back({prop.id, colour_index});
}

TextureProp LevelFileIO::deserialise_texture(const nlohmann::json& object) const
{
    auto texture_id = object[0];
    auto colour_index = (int)object[1];

    return {
        .id = object[0],
        .colour = colours_[colour_index],
    };
}

int LevelFileIO::find_colour_index(glm::u8vec4 colour) const
{
    for (auto const [index, cached_colour] : std::views::enumerate(colours_))
    {
        if (cached_colour == colour)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int LevelFileIO::add_colour(glm::u8vec4 colour)
{
    colours_.push_back(colour);
    return static_cast<int>(colours_.size() - 1);
}

void LevelFileIO::write_floors(const nlohmann::json& floors)
{
    json_["floors"] = floors;
}

nlohmann::json LevelFileIO::get_floors() const
{
    return json_["floors"];
}

void LevelFileSelectGUI::show()
{
    is_showing_ = true;

    // When showing, the level list must reset in case levels have been saved, deleted, renamed etc
    // since it was last opened
    levels_.clear();
    for (const auto& entry : std::filesystem::directory_iterator("./levels"))
    {
        auto dirname = entry.path().filename().string();
        if (entry.is_directory() && !dirname.starts_with(INTERNAL_FILE_ID))
        {
            if (auto meta = get_metafile_content(dirname))
            {
                levels_.push_back({.directory = dirname,
                                   .display_name = meta->at("level_name"),
                                   .saved_date = epoch_to_datetime_string(meta->at("saved_date"))});
            }
            else
            {
                std::println(std::cerr,
                             "Could not find a metafile for {} - defaulting to folder name.",
                             dirname.c_str());
                levels_.push_back(
                    {.directory = dirname, .display_name = dirname, .saved_date = "???"});
            }
        }
    }
}

void LevelFileSelectGUI::hide()
{
    is_showing_ = false;
}

bool LevelFileSelectGUI::is_showing() const
{
    return is_showing_;
}

std::optional<std::string> LevelFileSelectGUI::display_level_select_gui()
{
    if (!is_showing())
    {
        return std::nullopt;
    }

    std::optional<std::string> selection = std::nullopt;

    if (ImGuiExtras::BeginCentredWindow("Load Level", {800, 800}))
    {
        ImGui::Text("Select a level to load:");
        ImGui::Separator();

        for (const auto& level : levels_)
        {

            if (ImGui::Button(
                    std::format("Name: {} - Last Saved: {}", level.display_name, level.saved_date)
                        .c_str()))
            {
                selection = level.directory;
                hide();
            }
        }

        if (ImGui::Button("Cancel"))
        {
            hide();
        }
    }
    ImGui::End();
    return selection;
}
