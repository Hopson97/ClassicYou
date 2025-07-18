#include "LevelObject.h"

#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>
#include <print>

#include "../Util/ImGuiExtras.h"
#include "../Util/Maths.h"
#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "EditorGUI.h"
#include "LevelTextures.h"

namespace
{

    template <typename T>
    void property_gui(GUIFunction<T> function, const LevelTextures& textures,
                      ActionManager& action_manager, const T& object, LevelObject& current,
                      typename T::PropertiesType& object_default, int current_floor)
    {

        // Caching the object is a work-around due to an ImGui limitation that when the mouse is
        // held on a slider element, it will continuously trigger that input, returning true. This
        // means that when the object gets updated, it creates many events - even if nothing has
        // changed.
        // This also means that storing the history can be incorrect.
        //
        // For example, sliding the wall height from 0.2m to 0.8m creates 10s of updates for every
        // incremental change. The history should only store the main update though, 0.2 -> 0.8m
        // However as many events get created, it means ,without caching, the update would be stored
        // as 0.8m -> 0.8m
        //
        // So by caching the object the first time the slider is clicked, it means the correct
        // history can be stored, meaning undo/redo functionality actually works.

        static auto cached_object = current;

        // Was the last update one where the mouse was released - storing it in the
        // ActionManagerHistory?
        static auto last_store_action = false;

        auto [update, new_props] = function(textures, object);

        if (update.always_update)
        {
            // Inputs such a button clicks (textures, styles etc) should always cause an update to
            // happen and always trigger history to be recorded to enable undo/redo
            LevelObject new_object = current;
            std::get<T>(new_object.object_type).properties = new_props;
            action_manager.push_action(
                std::make_unique<UpdateObjectAction>(current, new_object, current_floor));
            last_store_action = true;
        }
        else if (update.continuous_update)
        {

            // If the last update was when the user released the mouse, it means this is a new input
            // event to a property editor element
            // This means the object state must be cached at this point
            if (last_store_action && !update.action)
            {
                std::println("Cached object: {}", cached_object.to_string());
                cached_object = current;
            }
            LevelObject new_object = current;
            std::get<T>(new_object.object_type).properties = new_props;

            // When the mouse is released (so update.action is true), the cached object should be
            // used to update the object such that only the "major" history is recorded (before and
            // after a slider was used) to enable proper undo and redo
            //
            // Otherwise, the non-cached version can be used.
            action_manager.push_action(
                std::make_unique<UpdateObjectAction>(update.action ? cached_object : current,
                                                     new_object, current_floor),
                update.action);

            last_store_action = update.action;
        }

        ImGui::Separator();
        if (ImGui::Button("Set As Default"))
        {
            object_default = object.properties;
        }
    }
} // namespace

void LevelObject::property_gui(EditorState& state, const LevelTextures& textures,
                               ActionManager& action_manager)
{

    ImGui::Text("Properties");
    ImGui::Separator();
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        ::property_gui<WallObject>(&wall_gui, textures, action_manager, *wall, *this,
                                   state.wall_default, state.current_floor);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        ::property_gui<PlatformObject>(&platform_gui, textures, action_manager, *platform, *this,
                                       state.platform_default, state.current_floor);
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        ::property_gui<PolygonPlatformObject>(&polygon_platform_gui, textures, action_manager,
                                              *poly, *this, state.polygon_platform_default,
                                              state.current_floor);
    }
}

LevelObjectsMesh3D LevelObject::to_geometry(int floor_number) const
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return generate_wall_mesh(*wall, floor_number);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return generate_platform_mesh(*platform, floor_number);
    }
    else if (auto polygon_platform = std::get_if<PolygonPlatformObject>(&object_type))
    {
        return generate_polygon_platform_mesh(*polygon_platform, floor_number);
    }

    throw std::runtime_error("Must implement getting geometry for all types!");
}

std::string LevelObject::to_string() const
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return std::format("Props:\n Texture 1/2: {} {}: \n Base: {}\n Height: {}\nParameters:\n "
                           "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                           wall->properties.texture_front, wall->properties.texture_back,
                           wall->properties.base_height, wall->properties.wall_height,
                           wall->parameters.line.start.x, wall->parameters.line.start.y,
                           wall->parameters.line.end.x, wall->parameters.line.end.y);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return std::format("Props:\n Texture Top: {}\n Texture Bottom: {}\n Width: {}\n Depth: "
                           "{} \n Height: {} \n Style: {}\n"
                           "Parameters:\n "
                           " Position: ({:.2f}, {:.2f})",
                           platform->properties.texture_top, platform->properties.texture_bottom,
                           platform->properties.width, platform->properties.depth,
                           platform->properties.base,
                           magic_enum::enum_name(platform->properties.style),
                           platform->parameters.position.x, platform->parameters.position.y);
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        return std::format(
            "Props:\n Texture Top: {}\n Texture Bottom: {}\nParameters:\n "
            "Corner Top Left: ({:.2f}, {:.2f})\n - Corner Top Right: ({:.2f}, {:.2f})\n "
            "Corner Bottom Right: ({:.2f}, {:.2f})\n - Corner Bottom Left: ({:.2f}",
            poly->properties.texture_top, poly->properties.texture_bottom,
            poly->parameters.corner_top_left.x, poly->parameters.corner_top_left.y,
            poly->parameters.corner_top_right.x, poly->parameters.corner_top_right.y,
            poly->parameters.corner_bottom_right.x, poly->parameters.corner_bottom_right.y,
            poly->parameters.corner_bottom_left.x, poly->parameters.corner_bottom_left.y);
    }
    return "";
}

void LevelObject::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object,
                            bool is_current_floor) const
{
    auto is_selected = p_active_object && p_active_object->object_id == object_id;
    auto colour = is_selected ? Colour::RED : (is_current_floor ? Colour::WHITE : Colour::GREY);

    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        auto thickness = is_selected ? 3.0f : 2.0f;

        drawing_pad.render_line(wall->parameters.line.start, wall->parameters.line.end, colour,
                                thickness);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        const auto& position = platform->parameters.position;
        const auto& width = platform->properties.width * TILE_SIZE;
        const auto& depth = platform->properties.depth * TILE_SIZE;

        if (platform->properties.style == PlatformStyle::Quad)
        {
            drawing_pad.render_quad(position, {width, depth}, colour);
        }
        else if (platform->properties.style == PlatformStyle::Diamond)
        {
            drawing_pad.render_diamond(position, {width, depth}, colour);
        }
    }
    else if (auto polygon_platform = std::get_if<PolygonPlatformObject>(&object_type))
    {
        const auto& params = polygon_platform->parameters;
        auto& tl = params.corner_top_left;
        auto& tr = params.corner_top_right;
        auto& br = params.corner_bottom_right;
        auto& bl = params.corner_bottom_right;

        drawing_pad.render_line(tl, tl + glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(tl, tl - glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(tr, tr - glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(tr, tr - glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(br, br - glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(br, br + glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(bl, bl + glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(bl, bl + glm::vec2(0, TILE_SIZE), colour, 5);
    }
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile, const LevelObject* p_active_object) const
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        const auto& params = wall->parameters;
        if (distance_to_line(selection_tile, {params.line.start, params.line.end}) < 15)
        {
            // Allow selecting objects that may be overlapping
            if (!p_active_object || p_active_object->object_id != object_id)
            {
                return true;
            }
        }
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        const auto& params = platform->parameters;
        const auto& props = platform->properties;

        if (selection_tile.x >= params.position.x &&
            selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
            selection_tile.y >= params.position.y &&
            selection_tile.y <= params.position.y + props.depth * TILE_SIZE)
        {
            // Allow selecting objects that may be overlapping
            if (!p_active_object || p_active_object->object_id != object_id)
            {
                return true;
            }
        }
    }
    return false;
}

std::pair<nlohmann::json, std::string> LevelObject::serialise() const
{
    std::string type = "";
    nlohmann::json object;
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        type = "wall";
        auto& params = wall->parameters;
        auto& props = wall->properties;

        object["params"] = {params.line.start.x, params.line.start.y, params.line.end.x,
                            params.line.end.y};

        object["props"] = {props.texture_back, props.texture_front, props.wall_height,
                           props.base_height};
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        type = "platform";
        auto& params = platform->parameters;
        auto& props = platform->properties;

        object["params"] = {params.position.x, params.position.y};

        object["props"] = {props.texture_bottom, props.texture_top, props.width,
                           props.depth,          props.base,        (int)props.style};
    }
    else if (auto polygon_platform = std::get_if<PolygonPlatformObject>(&object_type))
    {
        type = "polygon_platform";
        auto& params = polygon_platform->parameters;
        auto& props = polygon_platform->properties;

        object["params"] = {params.corner_top_left.x,     params.corner_top_left.y,
                            params.corner_top_right.x,    params.corner_top_right.y,
                            params.corner_bottom_right.x, params.corner_bottom_right.y,
                            params.corner_bottom_left.x,  params.corner_bottom_left.y};

        object["props"] = {props.texture_top, props.texture_bottom, props.visible};
    }

    return {object, type};
}

bool LevelObject::deserialise_as_wall(const nlohmann::json& wall_json)
{
    WallObject wall;

    auto params = wall_json["params"];
    auto props = wall_json["props"];
    if (params.size() < 4)
    {
        std::println("Invalid wall parameters, expected 4 values");
        return false;
    }
    if (props.size() < 4)
    {
        std::println("Invalid wall properties, expected 4 values");
        return false;
    }
    wall.parameters.line.start = {params[0], params[1]};
    wall.parameters.line.end = {params[2], params[3]};

    wall.properties.texture_back = props[0];
    wall.properties.texture_front = props[1];
    wall.properties.wall_height = props[2];
    wall.properties.base_height = props[3];

    object_type = wall;
    return true;
}

bool LevelObject::deserialise_as_platform(const nlohmann::json& platform_json)
{
    PlatformObject platform;

    auto params = platform_json["params"];
    auto props = platform_json["props"];
    if (params.size() < 2)
    {
        std::println("Invalid platform parameters, expected 2 values");
    }
    if (props.size() < 6)
    {
        std::println("Invalid platform properties, expected 5 values");
        return false;
    }

    platform.parameters.position = {params[0], params[1]};

    platform.properties.texture_bottom = props[0];
    platform.properties.texture_top = props[1];
    platform.properties.width = props[2];
    platform.properties.depth = props[3];
    platform.properties.base = props[4];
    platform.properties.style = (PlatformStyle)(props[5]);

    object_type = platform;
    return true;
}

bool LevelObject::deserialise_as_polygon_platform(const nlohmann::json& platform)
{
    PolygonPlatformObject polygon_platform;

    auto params = platform["params"];
    auto props = platform["props"];
    if (params.size() < 8)
    {
        std::println("Invalid polygon_platform parameters, expected 8 values");
        return false;
    }
    if (props.size() < 3)
    {
        std::println("Invalid polygon_platform properties, expected 3 values");
        return false;
    }

    polygon_platform.parameters.corner_top_left = {params[0], params[1]};
    polygon_platform.parameters.corner_top_right = {params[2], params[3]};
    polygon_platform.parameters.corner_bottom_right = {params[4], params[5]};
    polygon_platform.parameters.corner_bottom_left = {params[6], params[7]};

    polygon_platform.properties.texture_top = props[0];
    polygon_platform.properties.texture_bottom = props[1];
    polygon_platform.properties.visible = props[2];

    object_type = polygon_platform;
    return true;
}
