#pragma once

#include <glad/glad.h>

class LevelTextures;

enum class PropType
{
    Texture
};

template <PropType Type, typename Value>
struct Prop
{
    static constexpr PropType type = Type;
    Value value;
};

using TextureProp = Prop<PropType::Texture, GLuint>;

int display_texture_gui(const char* title, const TextureProp& current_texure,
                        const LevelTextures& textures);
