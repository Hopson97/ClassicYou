#pragma once
#include <utility>

#include "LevelObject.h"
#include "ObjectProperties.h"

class LevelTextures;

/**
 * @brief The return value for the GUI functions.
 */
struct UpdateResult
{
    /// Was there a continuous update in the GUI, such a slider being moved?
    bool continuous_update = false;

    /// Should an action be recorded? Only used for continuous update when the mouse is released off
    /// a slider element
    bool action = false;

    /// For discrete GUI elements eg buttons, elements being click should ALWAYS update the object
    /// and record its history (for undo etc)
    bool always_update = false;
};

/// @brief Alias for object GUI functions - used in LevelObject.h
/// @tparam T
template <typename T>
using GUIFunction = std::pair<UpdateResult, typename T::PropertiesType> (*)(
    const LevelTextures& textures, const T& object);

/// @brief Properties GUI for a platform object.
std::pair<UpdateResult, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall);

/// @brief Properties GUI for a platform object.
std::pair<UpdateResult, PlatformProps> platform_gui(const LevelTextures& textures,
                                                    const PlatformObject& platform);
