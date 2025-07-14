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
                      typename T::PropertiesType& object_default)
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
            action_manager.push_action(std::make_unique<UpdateObjectAction>(current, new_object));
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
            // Otherwise, the non-cached version can be used
            std::println("Update Action: {}", update.action ? "True" : "False");
            action_manager.push_action(std::make_unique<UpdateObjectAction>(
                                           update.action ? cached_object : current, new_object),
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
                                   state.wall_default);
    }
    if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        ::property_gui<PlatformObject>(&platform_gui, textures, action_manager, *platform, *this,
                                       state.platform_default);
    }
}

LevelObjectsMesh3D LevelObject::to_geometry() const
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return generate_wall_mesh(*wall);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return generate_platform_mesh(*platform);
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
    return "";
}

void LevelObject::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object)
{
    auto is_selected = p_active_object && p_active_object->object_id == object_id;
    auto colour = is_selected ? Colour::RED : Colour::WHITE;

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
            drawing_pad.render_quad(position, {width, depth}, Colour::RED);
        }
        else if (platform->properties.style == PlatformStyle::Diamond)
        {
            drawing_pad.render_diamond(position, {width, depth}, Colour::RED);
        }
    }
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile, const LevelObject* p_active_object)
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
