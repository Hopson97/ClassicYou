#pragma once
#include <utility>

#include "LevelObject.h"
#include "ObjectProperties.h"

class LevelTextures;

/**
 * @brief The return value for the GUI functions.
 */
struct ShouldUpdate
{
    /// @brief Whether the user has changed a value in the GUI.
    bool value = false;

    /// @brief Whether an action history should be created for the update.
    bool action = false;
};

/// @brief Alias for object GUI functions - used in LevelObject.h
/// @tparam T
template <typename T>
using GUIFunction = std::pair<ShouldUpdate, typename T::PropertiesType> (*)(
    const LevelTextures& textures, const T& object);

/// @brief Properties GUI for a platform object.
std::pair<ShouldUpdate, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall);

/// @brief Properties GUI for a platform object.
std::pair<ShouldUpdate, PlatformProps> platform_gui(const LevelTextures& textures,
                                                    const PlatformObject& platform);

/**
 * @brief UI for selecting a texture from a list of textures.
 *
 * @param title The title of the texture selection UI.
 * @param current_texture The currently selected texture.
 * @param textures The list of available textures.
 * @return int The index of the selected texture, or -1 if no texture is selected.
 */
int texture_prop_gui(const char* title, TextureProp current_texture, const LevelTextures& textures);
