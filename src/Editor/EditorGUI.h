#pragma once
#include <utility>

#include "LevelObject.h"
#include "ObjectProperties.h"

class LevelTextures;

struct ShouldUpdate
{
    bool value = false;
    bool action = false;
};

template <typename T>
using GUIFunction = std::pair<ShouldUpdate, typename T::PropertiesType> (*)(
    const LevelTextures& textures, const T& object);

std::pair<ShouldUpdate, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall);
std::pair<ShouldUpdate, PlatformProps> platform_gui(const LevelTextures& textures,
                                                    const PlatformObject& platform);

int texture_prop_gui(const char* title, TextureProp current_texture, const LevelTextures& textures);
