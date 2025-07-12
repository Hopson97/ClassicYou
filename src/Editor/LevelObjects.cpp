#include "LevelObjects.h"

#include <print>

#include <imgui.h>

#include "../Util/Maths.h"
#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "LevelTextures.h"

namespace
{
    template <typename T>
    using GUIFunction = std::pair<bool, typename T::PropertiesType> (*)(
        EditorState& state, const LevelTextures& textures, const T& object);

    std::pair<bool, WallProps> wall_gui(EditorState& state, const LevelTextures& textures,
                                        const WallObject& wall)
    {
        ImGui::ShowDemoWindow();
        bool update = false;
        WallProps new_props = wall.properties;
        if (ImGui::Begin("Properties"))
        {
            ImGui::Separator();

            auto texture_front =
                display_texture_gui("Side 1", wall.properties.texture_front, textures);
            if (texture_front >= 0)
            {
                new_props.texture_front = texture_front;
                update = true;
            }

            auto texture_back =
                display_texture_gui("Side 2", wall.properties.texture_back, textures);
            if (texture_back >= 0)
            {
                new_props.texture_back = texture_back;
                update = true;
            }

            ImGui::Separator();
            if (ImGui::Button("Set As Default"))
            {
                state.wall_default.texture_front = wall.properties.texture_front;
                state.wall_default.texture_back = wall.properties.texture_back;
            }

            if (ImGui::SliderFloat("Base", &new_props.base_height, 0.2f, 1.8f, "%.2f"))
            {
                new_props.base_height =
                    round(new_props.base_height * 5.0f) / 5.0f; 
                update = true;
            }
            if (ImGui::SliderFloat("Height", &new_props.wall_height, 0.2f, 2.0f, "%.2f"))
            {
                new_props.wall_height =
                    round(new_props.wall_height * 5.0f) / 5.0f; 
                update = true;
            }

        }
        ImGui::End();

        return {update, new_props};
    }

    std::pair<bool, PlatformProps> platform_gui(EditorState& state, const LevelTextures& textures,
                                                const PlatformObject& platform)
    {
        bool update = false;
        PlatformProps new_props = platform.properties;
        if (ImGui::Begin("Properties"))
        {
            ImGui::Separator();

            auto texture_top =
                display_texture_gui("Top Texture", platform.properties.texture_top, textures);
            if (texture_top >= 0)
            {
                new_props.texture_top = texture_top;
                update = true;
            }

            auto texture_bottom =
                display_texture_gui("Bottom Texture", platform.properties.texture_bottom, textures);
            if (texture_bottom >= 0)
            {
                new_props.texture_bottom = texture_bottom;
                update = true;
            }

            if (ImGui::SliderFloat("Width", &new_props.width, 0.5f, 100.0f, "%.2f"))
            {
                new_props.width = round(new_props.width * 2.0f) / 2.0f; 
                update = true;
            }
            if (ImGui::SliderFloat("Depth", &new_props.depth, 0.5f, 100.0f, "%.2f"))
            {
                new_props.depth = round(new_props.depth * 2.0f) / 2.0f; 
                update = true;
            }
            if (ImGui::SliderFloat("Base Height", &new_props.base, 0.0f, 2.0f, "%.1f"))
            {
                new_props.base = round(new_props.base * 5.0f) / 5.0f; 
                update = true;
            }

            ImGui::Separator();
            if (ImGui::Button("Set As Default"))
            {
                state.platform_default = platform.properties;
            }
        }
        ImGui::End();

        return {update, new_props};
    }

    template <typename T>
    void property_gui(GUIFunction<T> function, EditorState& state, const LevelTextures& textures,
                      ActionManager& action_manager, const T& object)
    {
        auto [update, new_props] = function(state, textures, object);
        if (update)
        {
            LevelObject new_object = object;

            std::get<T>(new_object.object_type).properties = new_props;
            action_manager.push_action(std::make_unique<UpdateObjectAction>(object, new_object));
        }
    }
} // namespace

void LevelObject::property_gui(EditorState& state, const LevelTextures& textures,
                               ActionManager& action_manager)
{

    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        ::property_gui<WallObject>(&wall_gui, state, textures, action_manager, *wall);
    }
    if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        ::property_gui<PlatformObject>(&platform_gui, state, textures, action_manager, *platform);
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
        return std::format("Props:\n  From Texture 1/2: {} {}\nParameters:\n "
                           "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                           wall->properties.texture_front, wall->properties.texture_back,
                           wall->parameters.start.x, wall->parameters.start.y,
                           wall->parameters.end.x, wall->parameters.end.y);
    }
    else if (auto platform = std::get_if<PlatformObject>(&object_type))
    {
        return std::format("Props:\n  Texture Top: {}\n  Texture Bottom: {}\n  Width: {}\n  Depth: "
                           "{}\nParameters:\n "
                           "  Position: ({:.2f}, {:.2f})",
                           platform->properties.texture_top, platform->properties.texture_bottom,
                           platform->properties.width, platform->properties.depth,
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
    auto h = props.wall_height;

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
