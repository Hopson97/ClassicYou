#include "LevelTextures.h"

bool LevelTextures::register_texture(const std::string& name,
                                     const std::filesystem::path& texture_file_path,
                                     gl::Texture2DArray& textures)

{
    if (texture_map.find(name) != texture_map.end())
    {
        return true;
    }

    auto [loaded, id] = textures.add_texture_from_file(texture_file_path, 4, false, false);
    if (loaded)
    {
        texture_map.emplace(name, id);

        // Also load the "TEXTURE_2D" for GUIs
        gl::Texture2D texture;
        if (!texture.load_from_file(texture_file_path, 4, false, false, gl::TEXTURE_PARAMS_NEAREST))
        {
            return false;
        }

        texture_2d_map.emplace(name, std::move(texture));
        return true;
    }

    return false;
}

std::optional<GLuint> LevelTextures::get_texture(const std::string& name) const
{
    auto itr = texture_map.find(name);
    if (itr == texture_map.end())
    {
        return {};
    }
    return itr->second;
}