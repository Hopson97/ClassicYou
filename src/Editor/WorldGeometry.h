#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class LevelTextures;
struct EditorState;

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

struct LevelObject
{
    virtual bool property_gui(EditorState& state, const LevelTextures& textures) = 0;
};

struct WallProps
{
    TextureProp texture_side_1;
    TextureProp texture_side_2;
};

struct WallParmeters
{
    glm::vec2 start;
    glm::vec2 end;
};

struct Wall : public LevelObject
{
    bool property_gui(EditorState& state, const LevelTextures& textures) override;

        Wall(const WallParmeters& params, const WallProps& properties)
        : parameters(params)
        , props(properties)
    {
    }

    WallParmeters parameters;
    WallProps props;
};


struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {.texture_side_1 = 0, .texture_side_2 = 3};

    LevelObject* p_active_object_ = nullptr;
};