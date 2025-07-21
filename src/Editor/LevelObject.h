#pragma once

#include <variant>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../Graphics/Mesh.h"
#include "ObjectProperties.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class DrawingPad;

// =======================================
//     Object Type Template
// =======================================
enum class ObjectTypeName
{
    Wall,
    Platform,
    PolygonPlatform,
    Pillar,
};

/// All objects in the level editor have a set of properties and parameters.
/// The properties should be things editable by the properties GUI, such as textures, height, etc.
/// The parameters are things that are not editable by the properties GUI, such as the object
/// position.
template <typename Properties, typename Parameters>
struct ObjectType
{
    using PropertiesType = Properties;
    using ParametersType = Parameters;

    Properties properties;
    Parameters parameters;
};

// =======================================
//     Object Types
// =======================================
using WallObject = ObjectType<WallProps, WallParameters>;
using PlatformObject = ObjectType<PlatformProps, PlatformParameters>;
using PolygonPlatformObject = ObjectType<PolygonPlatformProps, PolygonPlatformParameters>;
using PillarObject = ObjectType<PillarProps, PillarParameters>;

using ObjectId = std::int32_t;

/**
 * @brief A single object in the level editor.
 */
struct LevelObject
{
    /// Unique identifier for the object.
    ObjectId object_id = 0;

    /// Variant type that holds the properties and parameters of the object.
    /// A variant is used rather than using inheritance such that the objects can be much easily
    /// copied around while maintaining type safety, and avoiding issues with inheritance such as
    /// object slicing - Most notable in the actions manager where the state is copied around for
    /// easy undo and redo functionality.
    std::variant<WallObject, PlatformObject, PolygonPlatformObject, PillarObject> object_type;

    template <typename Properties, typename Parameters>
    explicit LevelObject(const ObjectType<Properties, Parameters>& object)
        : object_type(object)
    {
    }

    explicit LevelObject(int object_id)
        : object_id(object_id)
    {
    }

    /// Displays a GUI for updating the properties of the object.
    void property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager);

    /// Converts the object to a 3D mesh for rendering.
    [[nodiscard]] LevelObjectsMesh3D to_geometry(int floor_number) const;

    /// Converts the object to a string representation.
    [[nodiscard]] std::string to_string() const;

    /// Renders the object to the 2D drawing pad. The selected object is highlighted in red, and
    /// objects on the floor below are rendered slightly greyed out for easier editing.
    void render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object,
                   bool is_current_floor) const;

    /// Try to select the object in the 2D view based on the given selection tile.
    bool try_select_2d(glm::vec2 selection_tile) const;

    /// Checks if the object is entirely within the given selection area.
    bool is_within(const Rectangle& selection_area);

    /// Moves the object by the given offset.
    void move(glm::vec2 offset);

    /// Serialise the object to JSON format.
    std::pair<nlohmann::json, std::string> serialise() const;

    bool deserialise_as_wall(const nlohmann::json& wall);
    bool deserialise_as_platform(const nlohmann::json& platform);
    bool deserialise_as_polygon_platform(const nlohmann::json& platform);
    bool deserialise_as_pillar(const nlohmann::json& pillar);
};

// =======================================
//     Editor State
// =======================================

/// @brief The state of the editor input, such as the currently hovered node, the selected object
/// and default object properties.
struct EditorState
{
    WallProps wall_default = {
        .texture_front = {.id = 0},
        .texture_back = {.id = 0},
        .base_height = 0,
        .height = 1,
    };

    PlatformProps platform_default = {
        .texture_top = {.id = 0},
        .texture_bottom = {.id = 0},
        .width = 1,
        .depth = 1,
        .base = 0,
    };

    PolygonPlatformProps polygon_platform_default = {
        .texture_top = {.id = 0},
        .texture_bottom = {.id = 0},
        .visible = true,
    };

    PillarProps pillar_default;

    /// The currently hovered node/tile in the editor.
    glm::ivec2 node_hovered{0};

    /// @brief The currently selected object in the editor.
    /// When not nullptr, this object is highlighted in the editor and its properties are displayed
    /// in the properties GUI.
    LevelObject* p_active_object_ = nullptr;

    /// The current floor number that the editor is working on.
    int current_floor = 0;
};

// =======================================
//     Level Object Geometry Generation
// =======================================

// Note: These are defined in LevelObjectGeometry.cpp
[[nodiscard]] LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall, int floor_number);
[[nodiscard]] LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform,
                                                        int floor_number);

[[nodiscard]] LevelObjectsMesh3D generate_pillar_mesh(const PillarObject& platform,
                                                      int floor_number);

[[nodiscard]] LevelObjectsMesh3D
generate_polygon_platform_mesh(const PolygonPlatformObject& polygon_platform, int floor_number);