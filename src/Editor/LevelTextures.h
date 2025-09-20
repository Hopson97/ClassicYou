#pragma once

#include <filesystem>
#include <unordered_map>

#include "../Graphics/OpenGL/Texture.h"

/// Manager for the texture array used for texturing the level objects
class LevelTextures
{
  public:
    bool register_texture(const std::string& name, const std::filesystem::path& texture_file_path,
                          gl::Texture2DArray& textures);

    std::optional<GLuint> get_texture(const std::string& name) const;

    // For use in rendering 3D objects, this maps a name the "layer" within a GL_TEXTURE_2D_ARRAY
    std::unordered_map<std::string, GLuint> texture_map;

    // For use in rendering ImGUI, this maps the name to a 2D texture
    std::unordered_map<std::string, gl::Texture2D> texture_2d_map;
};
