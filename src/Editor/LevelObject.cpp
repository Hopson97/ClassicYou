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
                           wall->properties.texture_front.id, wall->properties.texture_back.id,
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
                           platform->properties.texture_top.id,
                           platform->properties.texture_bottom.id, platform->properties.width,
                           platform->properties.depth, platform->properties.base,
                           magic_enum::enum_name(platform->properties.style),
                           platform->parameters.position.x, platform->parameters.position.y);
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        return std::format(
            "Props:\n Texture Top: {}\n Texture Bottom: {}\nParameters:\n "
            "Corner Top Left: ({:.2f}, {:.2f})\n - Corner Top Right: ({:.2f}, {:.2f})\n "
            "Corner Bottom Right: ({:.2f}, {:.2f})\n - Corner Bottom Left: ({:.2f}",
            poly->properties.texture_top.id, poly->properties.texture_bottom.id,
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
        auto& bl = params.corner_bottom_left;

        drawing_pad.render_line(tl, tl + glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(tl, tl + glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(tr, tr - glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(tr, tr + glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(br, br - glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(br, br - glm::vec2(0, TILE_SIZE), colour, 5);

        drawing_pad.render_line(bl, bl + glm::vec2(TILE_SIZE, 0), colour, 5);
        drawing_pad.render_line(bl, bl - glm::vec2(0, TILE_SIZE), colour, 5);
    }
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile) const
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return distance_to_line(selection_tile, wall->parameters.line) < 15;
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        const auto& params = platform->parameters;
        const auto& props = platform->properties;

        return selection_tile.x >= params.position.x &&
               selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
               selection_tile.y >= params.position.y &&
               selection_tile.y <= params.position.y + props.depth * TILE_SIZE;
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        const auto& params = poly->parameters;

        return selection_tile.x >= params.corner_top_left.x &&
               selection_tile.x <= params.corner_top_right.x &&
               selection_tile.y >= params.corner_top_left.y &&
               selection_tile.y <= params.corner_bottom_left.y;
    }
    return false;
}

void LevelObject::move_to(glm::vec2 new_tile)
{
    if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        platform->parameters.position = new_tile;
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        auto& params = poly->parameters;

        // Poly platforms require moving all points, so they are moved relative to the first point
        auto trd = params.corner_top_right - params.corner_top_left;
        auto brd = params.corner_bottom_right - params.corner_top_left;
        auto bld = params.corner_bottom_left - params.corner_top_left;

        params.corner_top_left = new_tile;
        params.corner_top_right = new_tile + trd;
        params.corner_bottom_right = new_tile + brd;
        params.corner_bottom_left = new_tile + bld;
    }
}

void serialise_texture(nlohmann::json& object, const TextureProp& prop)
{
    object.push_back({prop.id, prop.colour.r, prop.colour.g, prop.colour.b, prop.colour.a});
}

TextureProp deserialise_texture(const nlohmann::json& object)
{
    return {
        .id = object[0],
        .colour = {object[1], object[2], object[3], object[4]},
    };
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

        nlohmann::json json_params = {params.line.start.x, params.line.start.y, params.line.end.x,
                                      params.line.end.y};

        nlohmann::json json_props = {};
        serialise_texture(json_props, props.texture_back);
        serialise_texture(json_props, props.texture_front);
        json_props.insert(json_props.end(), {props.wall_height, props.base_height});

        object = {json_params, json_props};
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        type = "platform";
        auto& params = platform->parameters;
        auto& props = platform->properties;

        nlohmann::json json_params = {params.position.x, params.position.y};

        nlohmann::json json_props = {};
        serialise_texture(json_props, props.texture_top);
        serialise_texture(json_props, props.texture_bottom);
        json_props.insert(json_props.end(),
                          {props.width, props.depth, props.base, (int)props.style});

        object = {json_params, json_props};
    }
    else if (auto polygon_platform = std::get_if<PolygonPlatformObject>(&object_type))
    {
        type = "polygon_platform";
        auto& params = polygon_platform->parameters;
        auto& props = polygon_platform->properties;

        nlohmann::json json_params = {params.corner_top_left.x,     params.corner_top_left.y,
                                      params.corner_top_right.x,    params.corner_top_right.y,
                                      params.corner_bottom_right.x, params.corner_bottom_right.y,
                                      params.corner_bottom_left.x,  params.corner_bottom_left.y};

        nlohmann::json json_props = {};
        serialise_texture(json_props, props.texture_top);
        serialise_texture(json_props, props.texture_bottom);
        json_props.push_back(props.visible);
        object = {json_params, json_props};
    }

    return {object, type};
}

bool LevelObject::deserialise_as_wall(const nlohmann::json& wall_json)
{
    WallObject wall;
    auto& params = wall.parameters;
    auto& props = wall.properties;

    auto jparams = wall_json[0];
    auto jprops = wall_json[1];
    if (jparams.size() < 4)
    {
        std::println("Invalid wall parameters, expected 4 values");
        return false;
    }
    if (jprops.size() < 4)
    {
        std::println("Invalid wall properties, expected 4 values");
        return false;
    }
    params.line.start = {jparams[0], jparams[1]};
    params.line.end = {jparams[2], jparams[3]};

    props.texture_back = deserialise_texture(jprops[0]);
    props.texture_front = deserialise_texture(jprops[1]);
    props.wall_height = jprops[2];
    props.base_height = jprops[3];

    object_type = wall;
    return true;
}

bool LevelObject::deserialise_as_platform(const nlohmann::json& platform_json)
{
    PlatformObject platform;
    auto& params = platform.parameters;
    auto& props = platform.properties;
    auto jparams = platform_json[0];
    auto jprops = platform_json[1];
    if (jparams.size() < 2)
    {
        std::println("Invalid platform parameters, expected 2 values");
    }
    if (jprops.size() < 6)
    {
        std::println("Invalid platform properties, expected 5 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};

    props.texture_top = deserialise_texture(jprops[0]);
    props.texture_bottom = deserialise_texture(jprops[1]);
    props.width = jprops[2];
    props.depth = jprops[3];
    props.base = jprops[4];
    props.style = (PlatformStyle)(jprops[5]);

    object_type = platform;
    return true;
}

bool LevelObject::deserialise_as_polygon_platform(const nlohmann::json& poly_json)
{
    PolygonPlatformObject polygon_platform;
    auto& params = polygon_platform.parameters;
    auto& props = polygon_platform.properties;

    auto jparams = poly_json[0];
    auto jprops = poly_json[1];
    if (jparams.size() < 8)
    {
        std::println("Invalid polygon_platform parameters, expected 8 values");
        return false;
    }
    if (jprops.size() < 3)
    {
        std::println("Invalid polygon_platform properties, expected 3 values");
        return false;
    }

    params.corner_top_left = {jparams[0], jparams[1]};
    params.corner_top_right = {jparams[2], jparams[3]};
    params.corner_bottom_right = {jparams[4], jparams[5]};
    params.corner_bottom_left = {jparams[6], jparams[7]};

    props.texture_top = deserialise_texture(jprops[0]);
    props.texture_bottom = deserialise_texture(jprops[1]);
    props.visible = jprops[2];

    object_type = polygon_platform;
    return true;
}
