#pragma once

#include "ObjectProperties.h"

struct LevelObject;

/// @brief The state of the editor input, such as the currently hovered node, the selected object
/// and default object properties.
struct EditorState
{
    // When placing objects, these are the default values for the properties. Default set in the
    // structs.
    WallProps wall_default;
    PlatformProps platform_default;
    PolygonPlatformProps polygon_platform_default;
    PillarProps pillar_default;

    /// The currently hovered node/tile in the editor.
    glm::ivec2 node_hovered{0};

    /// @brief The currently selected object in the editor.
    /// When not nullptr, this object is highlighted in the editor and its properties are displayed
    /// in the properties GUI.
    LevelObject* p_active_object = nullptr;

    /// The current floor number that the editor is working on.
    int current_floor = 0;
};
