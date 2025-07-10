#pragma once

#include <glad/glad.h>

class LevelTextures;

enum class PropType
{
    Texture
};

using TextureProp = int;

int display_texture_gui(const char* title, TextureProp current_texture,
                        const LevelTextures& textures);
