#include "LevelObject.h"

#include <print>

#include <imgui.h>

#include "../Util/ImGuiExtras.h"
#include "../Util/Maths.h"
#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "LevelTextures.h"
#include "EditorGUI.h"

namespace
{

    template <typename T>
    void property_gui(GUIFunction<T> function, EditorState& state, const LevelTextures& textures,
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
        // However as many events get created, it means ,without caching, the uodate would be stored
        // as 0.8m -> 0.8m
        //
        // So by caching the object the first time the slider is clicked, it means the correct
        // history can be stored, meaning undo/redo functionality actually works.
        static auto cached_object = current;

        // Was the last update one where the mouse was released - storing it in the
        // ActionManagerHistory>
        static auto last_store_action = false;

        auto [update, new_props] = function(textures, object);
        if (update.value)
        {
            // If the last update was when the user released the mouse, it means this is a new input
            // event to a property editor element
            // This means the object state must be cached at this point
            if (last_store_action && !update.action)
            {
                std::println("Cached object with id: {}", cached_object.object_id);
                cached_object = current;
            }
            LevelObject new_object = current;
            std::get<T>(new_object.object_type).properties = new_props;

            // When the mouse is released (so update.action is true), the cached object should be
            // used to update the object.
            // Otherwise, the non-cached version can be used
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

    ImGui::ShowDemoWindow();
    if (ImGui::Begin("Properties"))
    {
        ImGui::Separator();
        if (auto wall = std::get_if<WallObject>(&object_type))
        {
            ::property_gui<WallObject>(&wall_gui, state, textures, action_manager, *wall, *this,
                                       state.wall_default);
        }
        if (auto platform = std::get_if<PlatformObject>(&object_type))
        {
            ::property_gui<PlatformObject>(&platform_gui, state, textures, action_manager,
                                           *platform, *this, state.platform_default);
        }
    }
    ImGui::End();
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
                           wall->parameters.start.x, wall->parameters.start.y,
                           wall->parameters.end.x, wall->parameters.end.y);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return std::format("Props:\n Texture Top: {}\n Texture Bottom: {}\n Width: {}\n Depth: "
                           "{} \n Height: {}\n"
                           "Parameters:\n "
                           " Position: ({:.2f}, {:.2f})",
                           platform->properties.texture_top, platform->properties.texture_bottom,
                           platform->properties.width, platform->properties.depth,
                           platform->properties.base, platform->parameters.position.x,
                           platform->parameters.position.y);
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

        drawing_pad.render_line(wall->parameters.start, wall->parameters.end, colour, thickness);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        drawing_pad.render_quad(
            platform->parameters.position,
            {platform->properties.width * TILE_SIZE, platform->properties.depth * TILE_SIZE},
            colour);
    }
}

bool LevelObject::try_select_2d(glm::vec2 selection_tile, const LevelObject* p_active_object)
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        const auto& params = wall->parameters;
        if (distance_to_line(selection_tile, {params.start, params.end}) < 15)
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

LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall)
{
    const auto& params = wall.parameters;
    const auto& props = wall.properties;
    // Begin
    auto b = glm::vec3{params.start.x, 0, params.start.y} / static_cast<float>(TILE_SIZE);

    // End
    auto e = glm::vec3{params.end.x, 0, params.end.y} / static_cast<float>(TILE_SIZE);

    // Offset x, y, bottom (TODO: Top)
    auto ox = 0.0f;
    auto oz = 0.0f;
    auto ob = props.base_height;
    auto h = std::min(ob + props.wall_height, 2.0f);

    const auto length = glm::length(b - e);

    GLfloat tex1 = static_cast<float>(props.texture_front);
    GLfloat tex2 = static_cast<float>(props.texture_back);

    LevelObjectsMesh3D mesh;
    mesh.vertices = {
        // Front
        {{b.x + ox, ob, b.z + oz}, {0.0f, ob, tex1}, {0, 0, 1}},
        {{b.x + ox, h, b.z + oz}, {0.0, h, tex1}, {0, 0, 1}},
        {{e.x + ox, h, e.z + oz}, {length, h, tex1}, {0, 0, 1}},
        {{e.x + ox, ob, e.z + oz}, {length, ob, tex1}, {0, 0, 1}},

        // Back
        {{b.x - ox, ob, b.z - oz}, {0.0f, ob, tex2}, {0, 0, 1}},
        {{b.x - ox, h, b.z - oz}, {0.0, h, tex2}, {0, 0, 1}},
        {{e.x - ox, h, e.z - oz}, {length, h, tex2}, {0, 0, 1}},
        {{e.x - ox, ob, e.z - oz}, {length, ob, tex2}, {0, 0, 1}},

    };

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}

LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform)
{
    const auto& params = platform.parameters;
    const auto& props = platform.properties;

    float width = props.width;
    float depth = props.depth;
    float bo = props.base;

    GLfloat tex1 = static_cast<float>(props.texture_top);
    GLfloat tex2 = static_cast<float>(props.texture_bottom);

    auto p = glm::vec3{params.position.x, 0, params.position.y} / static_cast<float>(TILE_SIZE);

    LevelObjectsMesh3D mesh;
    // clang-format off
    mesh.vertices = {
        // Top
        {{p.x, bo, p.z,}, {0, 0, tex1}, {0, 1, 0}},
        {{p.x, bo, p.z + depth,}, {0, depth, tex1}, {0, 1, 0}},
        {{p.x + width, bo, p.z + depth,}, {width, depth, tex1}, {0, 1, 0}},
        {{p.x + width, bo, p.z,}, {width, 0, tex1}, {0, 1, 0}},

        // Bottom
        {{p.x, bo, p.z,}, {0, 0, tex2}, {0, 1, 0}},
        {{p.x, bo, p.z + depth,}, {0, depth, tex2}, {0, 1, 0}},
        {{p.x + width, bo, p.z + depth,}, {width, depth, tex2}, {0, 1, 0}},
        {{p.x + width, bo, p.z,}, {width, 0, tex2}, {0, 1, 0}},
    };
    // clang-format on

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}
