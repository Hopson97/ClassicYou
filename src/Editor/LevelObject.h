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
template <typename Properties, typename Parameters>
struct ObjectType
{

    using PropertiesType = Properties;
    using ParametersType = Parameters;

    Properties properties;
    Parameters parameters;
};

using WallObject = ObjectType<WallProps, WallParameters>;
using PlatformObject = ObjectType<PlatformProps, PlatformParameters>;

struct LevelObject
{
    template <typename Properties, typename Parameters>
    explicit LevelObject(const ObjectType<Properties, Parameters>& object)
        : object_type(object)
    {
    }

    explicit LevelObject(int object_id)
        : object_id(object_id)
    {
    }

    int object_id = 0;

    std::variant<WallObject, PlatformObject> object_type;

    void property_gui(EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager);

    [[nodiscard]] LevelObjectsMesh3D to_geometry(int floor_number) const;
    [[nodiscard]] std::string to_string() const;

    void render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object, bool is_current_floor);
    bool try_select_2d(glm::vec2 selection_tile, const LevelObject* p_active_object);

    std::pair<nlohmann::json, std::string> serialise() const;

    bool deserialise_as_wall(const nlohmann::json& wall);
    bool deserialise_as_platform(const nlohmann::json& platform);
};

struct EditorState
{
    glm::ivec2 node_hovered{0};

    WallProps wall_default = {
        .texture_front = 0,
        .texture_back = 0,
        .base_height = 0,
        .wall_height = 1,
    };

    PlatformProps platform_default = {
        .texture_top = 0,
        .texture_bottom = 0,
        .width = 1,
        .depth = 1,
        .base = 0,
    };

    LevelObject* p_active_object_ = nullptr;

    int current_floor = 0;
};

// Note: These are defined in LevelObjectGeometry.cpp
[[nodiscard]] LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall, int floor_number);
[[nodiscard]] LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform,
                                                        int floor_number);