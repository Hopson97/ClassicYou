#pragma once

#include <unordered_set>

#include "EditConstants.h"
#include "LevelObjects/ObjectTypes.h"

struct LevelObject;

/// Stores the currently selected objects in the editor, along with their floors.
struct Selection
{
    /// List of object IDs that are currently selected.
    std::vector<ObjectId> objects;

    /// The floors of the selected objects, corresponding to the IDs in `objects`.
    std::vector<int> object_floors;

    /// Pointer to the FIRST object in the selection. Convenience for GUI display, and fast access.
    LevelObject* p_active_object = nullptr;

    /// Clear the selection, setting it to the given object.
    void set_selection(LevelObject* object, int floor);

    /// Add an object to the selection via the object itself
    void add_to_selection(LevelObject* object, int floor);

    /// Add an object to the selection via its ID.
    void add_to_selection(ObjectId id, int floor);

    /// Clear the selection, removing all objects and resetting the active object.
    void clear_selection();

    bool single_object_is_selected() const;
    bool has_selection() const;
};

/// @brief The state of the editor input, such as the currently hovered node, the selected object
/// and default object properties.
struct EditorState
{
    // When placing objects, these are the default values for the properties.

    WallProps wall_default;
    PlatformProps platform_default;
    PolygonPlatformProps polygon_platform_default;
    PillarProps pillar_default;
    RampProps ramp_default;

    /// The currently hovered node/tile in the editor.
    glm::ivec2 node_hovered{0};

    /// The current floor number that the editor is working on.
    int current_floor = 0;

    /// The currently selected objects.
    /// When not empty, these objects are highlighted in the editor
    /// If there is  a single oject selected, its properties are displayed in the properties GUI.
    Selection selection;
};
