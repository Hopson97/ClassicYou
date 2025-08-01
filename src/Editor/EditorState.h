#pragma once

#include <unordered_set>

#include "EditConstants.h"
#include "LevelObjects/ObjectTypes.h"

struct LevelObject;

struct Selection
{
    std::vector<ObjectId> objects;
    std::vector<int> object_floors;

    LevelObject* p_active_object = nullptr;

    void set_selection(LevelObject* object, int floor);
    void add_to_selection(LevelObject* object, int floor);
    void add_to_selection(ObjectId id, int floor);
    void clear_selection();

    bool single_object_is_selected() const;
    bool has_selection() const;
};

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
    RampProps ramp_default;

    /// The currently hovered node/tile in the editor.
    glm::ivec2 node_hovered{0};

    /// @brief The currently selected object in the editor.
    /// When not nullptr, this object is highlighted in the editor and its properties are displayed
    /// in the properties GUI.
    LevelObject* p_active_object = nullptr;

    /// The current floor number that the editor is working on.
    int current_floor = 0;

    Selection selection;
};
