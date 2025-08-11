#include "LevelFileIO.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <fstream>
#include <zlib.h>

namespace
{
    std::filesystem::path make_level_path(const std::string& level_name)
    {
        return "levels/" + level_name + ".cly2";
    }

    std::filesystem::path make_meta_path(const std::string& level_name)
    {
        return "levels/" + level_name + "_meta.json";
    }

    void write_meta_file(const std::string& level_file_name, nlohmann::json& meta_object)
    {
        std::ofstream meta_file(make_meta_path(level_file_name));
        meta_file << meta_object;
    }

    std::optional<nlohmann::json> read_meta_file(const std::string& level_file_name)
    {
        auto path = make_meta_path(level_file_name);
        std::ifstream meta_file(path);
        if (!meta_file.is_open())
        {
            std::println(std::cerr, "Could not open metafile {}", path.string());
            return {};
        }
        return nlohmann::json::parse(meta_file);
    }
} // namespace

bool LevelFileIO::open(const std::string& level_file_name, bool load_uncompressed)
{
    auto path = make_level_path(level_file_name);
    if (load_uncompressed)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::println(std::cerr, "Could not open file {}", path.string());
            return false;
        }

        json_ = nlohmann::json::parse(file);

        // Load the metadata
        auto meta = json_["meta"];
        for (auto& object : meta["colours"])
        {
            colours_.push_back({object[0], object[1], object[2], object[3]});
        }
        return true;
    }

    // Load metadata from the metafile
    auto meta_json = read_meta_file(level_file_name);
    if (!meta_json)
    {
        return false;
    }
    auto& meta = *meta_json;
    version_ = meta["version"];
    size_t size = meta["size"];
    for (auto& object : meta["colours"])
    {
        colours_.push_back({object[0], object[1], object[2], object[3]});
    }

    // Open the binary file
    if (auto decompressed = decompress_from_file(path, size))
    {
        json_ = nlohmann::json::parse(*decompressed);
        std::println("Successfully opened {}", path.string());

        return true;
    }
    return false;
}

bool LevelFileIO::save(const std::string& level_file_name, bool save_uncompressed)
{
    auto path = make_level_path(level_file_name);
    auto json_dump = json_.dump();

    // Write the meta
    nlohmann::json meta;
    meta["colours"] = {};
    meta["version"] = version_;
    meta["size"] = json_dump.size();
    for (auto& colour : colours_)
    {
        meta["colours"].push_back({colour.r, colour.g, colour.b, colour.a});
    }
    json_["meta"] = meta;
    write_meta_file(level_file_name, meta);

    if (save_uncompressed)
    {
        std::ofstream basic_file(path.string());
        basic_file << json_;
    }

    auto compressed = compress_data(json_dump);
    if (compressed)
    {
        std::ofstream compressed_file(path.string(), std::ios::binary);
        compressed_file << *compressed;
        std::println("Succesfully saved to {}", path.string());
        return true;
    }
    return false;
}

void LevelFileIO::serialise_texture(nlohmann::json& object, const TextureProp& prop)
{
    // object.push_back({prop.id, prop.colour.r, prop.colour.g, prop.colour.b, prop.colour.a});
    auto colour_index = find_colour_index(prop.colour);
    if (colour_index == -1)
    {
        colour_index = add_colour(prop.colour);
    }
    object.push_back({prop.id, colour_index});
}

TextureProp LevelFileIO::deserialise_texture(const nlohmann::json& object) const
{
    // return {
    //     .id = object[0],
    //     .colour = {object[1], object[2], object[3], object[4]},
    // };
    auto texture_id = object[0];
    auto colour_index = (int)object[1];

    return {
        .id = object[0],
        .colour = colours_[colour_index],
    };
}

std::optional<std::string> LevelFileIO::compress_data(const std::string& json_dump)
{
    std::string compressed;
    auto compressed_length = compressBound(json_dump.size());
    compressed.resize(compressed_length);

    int result = compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressed_length,
                          reinterpret_cast<const Bytef*>(json_dump.c_str()), json_dump.size());
    if (result != Z_OK)
    {
        std::cerr << "Error compressing data" << std::endl;
        return {};
    }
    compressed.resize(compressed_length);
    return compressed;
}

std::optional<std::string> LevelFileIO::decompress_from_file(const std::filesystem::path& path,
                                                             std::size_t size)
{
    // Open the binary file
    std::ifstream compressed_file(path.string(), std::ios::binary);
    if (!compressed_file.is_open())
    {
        std::println(std::cerr, "Could not open level file for {}", path.string());
        return {};
    }

    std::stringstream buffer;
    buffer << compressed_file.rdbuf();
    std::string contents(buffer.str());

    std::string decompressed;
    decompressed.resize(size);

    uLongf size_zlib = size;
    int result = uncompress(reinterpret_cast<Bytef*>(&decompressed[0]), &size_zlib,
                            reinterpret_cast<const Bytef*>(contents.c_str()),
                            static_cast<uLongf>(contents.size()));

    if (result != Z_OK)
    {
        std::println(std::cerr, "Error decompressing file: {}", path.string());
        return {};
    }
    return decompressed;
}

int LevelFileIO::find_colour_index(glm::u8vec4 colour) const
{
    for (auto const [index, cached_colour] : std::views::enumerate(colours_))
    {
        if (cached_colour == colour)
        {
            return index;
        }
    }
    return -1;
}

int LevelFileIO::add_colour(glm::u8vec4 colour)
{
    colours_.push_back(colour);
    return colours_.size() - 1;
}

void LevelFileIO::write_floors(const nlohmann::json& floors)
{
    json_["floors"] = floors;
}

nlohmann::json LevelFileIO::get_floors() const
{
    return json_["floors"];
}