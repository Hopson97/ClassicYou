#pragma once

#include <variant>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../../Graphics/Mesh.h"
#include "../../Graphics/OpenGL/GLUtils.h"
#include "../EditConstants.h"
#include "ObjectTypes.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class DrawingPad;
class LevelFileIO;

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
    std::variant<WallObject, PlatformObject, PolygonPlatformObject, PillarObject, RampObject>
        object_type;

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
    bool property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager);

    /// Convert the underlying "object_type" to a type name
    [[nodiscard]] ObjectTypeName to_type() const;
    [[nodiscard]] std::string to_type_string() const;

    /// Converts the object to a 3D mesh for rendering.
    [[nodiscard]] LevelObjectsMesh3D to_geometry(int floor_number) const;

    /// Converts the object to a 2D mesh for rendering.
    [[nodiscard]] std::pair<Mesh2D, gl::PrimitiveType>
    to_2d_geometry(int floor_number, const LevelTextures& drawing_pad_texture_map) const;

    /// Converts the object to a string representation.
    [[nodiscard]] std::string to_string() const;

    /// Renders the object to the 2D drawing pad. The selected object is highlighted in red, and
    /// objects on the floor below are rendered slightly greyed out for easier editing.
    void render_2d(DrawingPad& drawing_pad, bool is_current_floor, bool is_selected,
                   const glm::vec2& selected_offset) const;

    /// Try to select the object in the 2D view based on the given selection tile.
    [[nodiscard]] bool try_select_2d(glm::vec2 selection_tile) const;

    /// Checks if the object is entirely within the given selection area.
    [[nodiscard]] bool is_within(const Rectangle& selection_area);

    /// Moves the object by the given offset.
    void move(glm::vec2 offset);

    /// Moves the object 90-degrees clockwise around the given point
    void rotate(glm::vec2 point);

    /// Serialise the object to JSON format.
    [[nodiscard]] std::pair<nlohmann::json, std::string>
    serialise(LevelFileIO& level_file_io) const;

    template <typename T>
    bool deserialise_as(const nlohmann::json& json, const LevelFileIO& level_file_io)
    {
        T object;
        if (!object_deserialise(object, json, level_file_io))
        {
            return false;
        }
        object_type = object;
        return true;
    }
};
