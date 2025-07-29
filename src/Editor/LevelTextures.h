#pragma once

#include <filesystem>
#include <unordered_map>

#include "../Graphics/OpenGL/Texture.h"

/// Manager for the texture array used for texturing the level objects
class LevelTextures
{
  public:
    bool register_texture(const std::string& name, const std::filesystem::path& texture_file_path,
                          gl::Texture2DArray& textures)
    {
        if (texture_map.find(name) != texture_map.end())
        {
            return true;
        }

        auto [loaded, id] = textures.add_texture_from_file(texture_file_path, 4, true, false);
        if (loaded)
        {
            texture_map.emplace(name, id);

            // Also load the "TEXTURE_2D" for GUIs
            gl::Texture2D texture;
            if (!texture.load_from_file(texture_file_path, 4, false, false,
                                        gl::TEXTURE_PARAMS_NEAREST))
            {
                return false;
            }

            texture_2d_map.emplace(name, std::move(texture));
            return true;
        }

        return false;
    }

    std::optional<GLuint> get_texture(const std::string& name) const
    {
        auto itr = texture_map.find(name);
        if (itr == texture_map.end())
        {
            return {};
        }
        return itr->second;
    }

    // For use in rendering 3D objects, this maps a name the "layer" within a GL_TEXTURE_2D_ARRAY
    std::unordered_map<std::string, GLuint> texture_map;

    // For use in rendering ImGUI, this maps the name to a 2D texture
    std::unordered_map<std::string, gl::Texture2D> texture_2d_map;
};
