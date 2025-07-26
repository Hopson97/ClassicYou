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
#include "EditorState.h"
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
    else if (auto pillar = std::get_if<PillarObject>(&object_type))
    {
        ::property_gui<PillarObject>(&pillar_gui, textures, action_manager, *pillar, *this,
                                     state.pillar_default, state.current_floor);
    }
}

LevelObjectsMesh3D LevelObject::to_geometry(int floor_number) const
{
    return std::visit([&](const auto& object) -> LevelObjectsMesh3D
                      { return object_to_geometry(object, floor_number); }, object_type);
}

std::string LevelObject::to_string() const
{
    return std::visit([](auto&& object) -> std::string { return object_to_string(object); },
                      object_type);
}

void LevelObject::render_2d(DrawingPad& drawing_pad, const LevelObject* p_active_object,
                            bool is_current_floor) const
{
    auto is_selected = p_active_object && p_active_object->object_id == object_id;
    auto colour = is_selected ? Colour::RED : (is_current_floor ? Colour::WHITE : Colour::GREY);

    std::visit([&](auto&& object) { render_object_2d(object, drawing_pad, colour, is_selected); },
               object_type);
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile) const
{
    return std::visit([&](const auto& object)
                      { return object_try_select_2d(object, selection_tile); }, object_type);
}

bool LevelObject::is_within(const Rectangle& selection_area)
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return wall->parameters.line.to_bounds().is_entirely_within(selection_area);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return Rectangle{
            .position = {platform->parameters.position.x, platform->parameters.position.y},
            .size = {platform->properties.width * TILE_SIZE_F,
                     platform->properties.depth * TILE_SIZE_F},
        }
            .is_entirely_within(selection_area);
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        return Rectangle{
            .position = {poly->parameters.corner_top_left.x, poly->parameters.corner_top_left.y},
            .size = {poly->parameters.corner_top_right.x - poly->parameters.corner_top_left.x,
                     poly->parameters.corner_bottom_left.y - poly->parameters.corner_top_left.y},
        }
            .is_entirely_within(selection_area);
    }

    else if (auto pillar = std::get_if<PillarObject>(&object_type))
    {
        return Rectangle{
            .position = {pillar->parameters.position.x, pillar->parameters.position.y},
            .size = {pillar->properties.size * TILE_SIZE_F, pillar->properties.size * TILE_SIZE_F},
        }
            .is_entirely_within(selection_area);
    }
    return false;
}

void LevelObject::move(glm::vec2 offset)
{
    if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        platform->parameters.position += offset;
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        auto& params = poly->parameters;

        // Poly platforms require moving all points, so they are moved relative to the first point
        auto trd = params.corner_top_right - params.corner_top_left;
        auto brd = params.corner_bottom_right - params.corner_top_left;
        auto bld = params.corner_bottom_left - params.corner_top_left;

        params.corner_top_left += offset;
        params.corner_top_right += offset + trd;
        params.corner_bottom_right += offset + brd;
        params.corner_bottom_left += offset + bld;
    }
    else if (auto pillar = std::get_if<PillarObject>(&object_type))
    {
        pillar->parameters.position += offset;
    }
}

std::pair<nlohmann::json, std::string> LevelObject::serialise() const
{
    return std::visit([&](auto&& object) { return object_serialise(object); }, object_type);
}


bool LevelObject::deserialise_as_wall(const nlohmann::json& json)
{
    return deserialise_as<WallObject>(json);
}

bool LevelObject::deserialise_as_platform(const nlohmann::json& json)
{
    return deserialise_as<PlatformObject>(json);
}

bool LevelObject::deserialise_as_polygon_platform(const nlohmann::json& json)
{
    return deserialise_as<PolygonPlatformObject>(json);
}

bool LevelObject::deserialise_as_pillar(const nlohmann::json& json)
{
    return deserialise_as<PillarObject>(json);
}
