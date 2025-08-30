#include "LevelObject.h"

#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>
#include <print>

#include "../../Util/ImGuiExtras.h"
#include "../../Util/Maths.h"
#include "../Actions.h"
#include "../DrawingPad.h"
#include "../EditConstants.h"
#include "../EditorGUI.h"
#include "../EditorState.h"
#include "../LevelTextures.h"

namespace
{
    template <typename T>
    bool property_gui(GUIFunction<T> function, const LevelTextures& textures,
                      ActionManager& action_manager, const T& object, LevelObject& current,
                      typename T::PropertiesType& object_default, int current_floor,
                      EditMode edit_mode)
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

        auto [update, new_props] = function(textures, object, edit_mode);

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

        return update.always_update || update.action;
    }
} // namespace

bool LevelObject::property_gui(EditorState& state, const LevelTextures& textures,
                               ActionManager& action_manager)
{
    ImGui::Text("Properties");
    ImGui::Separator();

    // clang-format off
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        return ::property_gui<WallObject>(&wall_gui, textures, action_manager, *wall, *this,
                                          state.wall_default, state.current_floor, state.edit_mode);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return ::property_gui<PlatformObject>(&platform_gui, textures, action_manager, *platform, *this, 
                                               state.platform_default, state.current_floor, state.edit_mode);
    }
    else if (auto poly = std::get_if<PolygonPlatformObject>(&object_type))
    {
        return ::property_gui<PolygonPlatformObject>(&polygon_platform_gui, textures, action_manager, *poly, *this,
                                                      state.polygon_platform_default, state.current_floor, state.edit_mode);
    }
    else if (auto pillar = std::get_if<PillarObject>(&object_type))
    {
        return ::property_gui<PillarObject>(&pillar_gui, textures, action_manager, *pillar, *this,
                                            state.pillar_default, state.current_floor, state.edit_mode);
    }
    else if (auto ramp = std::get_if<RampObject>(&object_type))
    {
        return ::property_gui<RampObject>(&ramp_gui, textures, action_manager, *ramp, *this,
                                          state.ramp_default, state.current_floor, state.edit_mode);
    }
    // clang-format on
    return false;
}

ObjectTypeName LevelObject::to_type() const
{
    if (std::get_if<WallObject>(&object_type))
    {
        return ObjectTypeName::Wall;
    }
    else if (std::get_if<PlatformObject>(&object_type))
    {
        return ObjectTypeName::Platform;
    }
    else if (std::get_if<PolygonPlatformObject>(&object_type))
    {
        return ObjectTypeName::PolygonPlatform;
    }
    else if (std::get_if<PillarObject>(&object_type))
    {
        return ObjectTypeName::Pillar;
    }
    else if (std::get_if<RampObject>(&object_type))
    {
        return ObjectTypeName::Ramp;
    }

    return ObjectTypeName::MissingTypeName;
}

std::string LevelObject::to_type_string() const
{
    return magic_enum::enum_name(to_type()).data();
}

LevelObjectsMesh3D LevelObject::to_geometry(int floor_number) const
{
    return std::visit([&](const auto& object) -> LevelObjectsMesh3D
                      { return object_to_geometry(object, floor_number); }, object_type);
}

std::string LevelObject::to_string() const
{
    return std::format("Type: {} - ID: {}\n{}", to_type_string(), object_id,
                       std::visit([](auto&& object) -> std::string
                                  { return object_to_string(object); }, object_type));
}

void LevelObject::render_2d(DrawingPad& drawing_pad, bool is_current_floor, bool is_selected,
                            const glm::vec2& selected_offset) const
{
    auto colour = is_current_floor ? Colour::WHITE : Colour::GREY;
    if (is_selected)
    {
        if (is_current_floor)
        {
            colour = Colour::RED;
        }
        else
        {
            colour.r = 1.0f;
        }
    }
    std::visit([&](auto&& object)
               { render_object_2d(object, drawing_pad, colour, selected_offset); }, object_type);
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile) const
{
    return std::visit([&](const auto& object)
                      { return object_try_select_2d(object, selection_tile); }, object_type);
}

bool LevelObject::is_within(const Rectangle& selection_area)
{
    return std::visit([&](const auto& object) { return object_is_within(object, selection_area); },
                      object_type);
}

void LevelObject::move(glm::vec2 offset)
{
    std::visit([&](auto&& object) { object_move(object, offset); }, object_type);
}

void LevelObject::rotate(glm::vec2 point)
{
    float degrees = 90.0f;
    std::visit([&](auto&& object) { object_rotate(object, point, degrees); }, object_type);
}

std::pair<nlohmann::json, std::string> LevelObject::serialise(LevelFileIO& level_file_io) const
{
    return std::visit([&](auto&& object) { return object_serialise(object, level_file_io); },
                      object_type);
}
