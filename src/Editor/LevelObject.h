#pragma once

#include <variant>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../Graphics/Mesh.h"
#include "ObjectTypes/LevelObjectTypes.h"
#include "ObjectTypes/Pillar.h"
#include "ObjectTypes/Platform.h"
#include "ObjectTypes/PolygonPlatform.h"
#include "ObjectTypes/Wall.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class DrawingPad;

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
//     Level Object Geometry Generation
// =======================================

// Note: These are defined in LevelObjectGeometry.cpp

[[nodiscard]] LevelObjectsMesh3D generate_pillar_mesh(const PillarObject& platform,
                                                      int floor_number);

[[nodiscard]] LevelObjectsMesh3D
generate_polygon_platform_mesh(const PolygonPlatformObject& polygon_platform, int floor_number);