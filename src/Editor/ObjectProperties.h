#pragma once

#include <glad/glad.h>

#include <functional>
#include <vector>

class LevelTextures;

struct Wall;

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

struct PropertyEditor
{
    bool display_texture_gui(const char* title, TextureProp& current_texure,
                             const LevelTextures& textures);

    std::vector<std::function<void(const Wall& wall)>> on_property_update;
};
