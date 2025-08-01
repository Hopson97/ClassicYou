#include "EditorGUI.h"

#include <filesystem>

#include <SFML/Window/Mouse.hpp>
#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>

#include "../Util/ImGuiExtras.h"
#include "LevelTextures.h"

namespace
{
    // Padding for surrounding image buttons
    constexpr ImVec2 BORDER_PADDING = {4, 4};
    constexpr ImVec2 GUI_TEXTURE_SIZE = {32, 32};
    constexpr int TEXTURES_PER_LINE = 6;

    /// Displays a list of available textures as a list of buttons, returns the ID of the given
    /// texture if a button is clicked
    std::optional<TextureProp> texture_prop_gui(UpdateResult& result, const char* label,
                                                TextureProp current_texture,
                                                const LevelTextures& textures)
    {
        bool texture_changed = false;

        TextureProp new_texture = current_texture;

        ImGui::Text("%s", label);
        int imgui_id = 0;
        for (const auto& [name, texture] : textures.texture_2d_map)
        {

            if (imgui_id != 0)
            {
                if (imgui_id % TEXTURES_PER_LINE != 0)
                {
                    ImGui::SameLine();
                }
            }
            ImGui::PushID(imgui_id++);
            std::string button_id = name + "###" + label;
            auto old_texture = current_texture;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BORDER_PADDING);

            // Give selected textures a red border
            if (auto texture_id = textures.get_texture(name))
            {
                if (texture_id == old_texture.id)
                {
                    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BORDER_PADDING);
                }
            }

            if (ImGui::ImageButton(button_id.c_str(), static_cast<ImTextureID>(texture.id),
                                   GUI_TEXTURE_SIZE))
            {
                if (auto texture_id = textures.get_texture(name))
                {
                    new_texture.id = *texture_id;
                    result.always_update |= true;
                    texture_changed = true;
                }
            }

            // Pop given styles
            ImGui::PopStyleVar();
            if (auto texture_id = textures.get_texture(name))
            {
                if (texture_id == old_texture.id)
                {
                    ImGui::PopStyleVar(2);
                    ImGui::PopStyleColor();
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", name.c_str());
            }
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Colour"))
        {
            auto& c = current_texture.colour;
            ImVec4 colour{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};

            if (ImGui::ColorPicker4("Colour", (float*)&colour))
            {
                new_texture.colour.r = static_cast<uint8_t>(colour.x * 255.0f);
                new_texture.colour.g = static_cast<uint8_t>(colour.y * 255.0f);
                new_texture.colour.b = static_cast<uint8_t>(colour.z * 255.0f);
                new_texture.colour.a = static_cast<uint8_t>(colour.w * 255.0f);
                texture_changed = true;
                result.continuous_update |= true;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                result.action = true;
            }
        }

        if (texture_changed)
        {
            return new_texture;
        }
        return {};
    }

    /// GUI for selecting a texture from the given "LevelTextures" object
    void texture_gui(UpdateResult& result, const char* label, const LevelTextures& textures,
                     TextureProp current, TextureProp& new_texture)
    {
        auto texture = texture_prop_gui(result, label, current, textures);
        if (texture)
        {

            new_texture = *texture;
        }
    }

    struct TextureTab
    {
        const char* name;
        TextureProp current;
        TextureProp* new_texture;
    };

    void texture_gui_tabs(UpdateResult& result, const char* tab_names,
                          const LevelTextures& textures, TextureTab tab_a, TextureTab tab_b)
    {
        if (ImGui::BeginTabBar(tab_names))
        {
            if (ImGui::BeginTabItem(tab_a.name))
            {
                texture_gui(result, tab_a.name, textures, tab_a.current, *tab_a.new_texture);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(tab_b.name))
            {
                texture_gui(result, tab_b.name, textures, tab_b.current, *tab_b.new_texture);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    /// GUI for a enum property such a styles etc
    template <typename EnumType>
    void enum_gui(UpdateResult& result, const char* label, EnumType current, EnumType& new_value)
    {
        ImGui::Text("%s", label);
        size_t n = 0;
        magic_enum::enum_for_each<EnumType>(
            [&](EnumType value)
            {
                int style = static_cast<int>(current);

                if (n != 0)
                {
                    if (++n % 3 != 0)
                    {
                        ImGui::SameLine();
                    }
                }
                n++;

                if (ImGui::RadioButton(magic_enum::enum_name(value).data(), &style,
                                       static_cast<int>(value)))
                {
                    result.always_update |= true;
                    new_value = value;
                }
            });
    }

    /// Wrapper around stepped slider for updating numerical props
    bool slider(UpdateResult& result, const char* label, float& value, float min, float max,
                float interval, const char* fmt = "%.1f")
    {
        bool updated = false;
        if (ImGui::SliderFloatStepped(label, value, min, max, interval, fmt))
        {
            updated = true;
            result.continuous_update |= true;
        }

        // Called once when the user releases the mouse after changing the value - used to prevent
        // creating history until a selection within the slider is made
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            result.action = true;
        }
        return updated;
    }

    /// To avoid creating too many mesh updates, this prevents updates from happening if the props
    /// have not actually changed. For example, clicking the same texture, or holding the mouse down
    /// on a slider element.
    template <typename T>
    UpdateResult check_prop_updated(UpdateResult result, const T& current_props, const T& new_props)
    {
        result.continuous_update =
            (result.continuous_update && current_props != new_props) || result.action;

        result.always_update = (result.always_update && current_props != new_props);

        return result;
    }

} // namespace

std::pair<UpdateResult, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall)

{
    UpdateResult result;
    auto new_props = wall.properties;

    // When the wall is generating the mesh, these values are multiplied by 2.0f
    auto base_height = new_props.start_base_height;
    auto height = new_props.start_height;
    if (slider(result, "Base Height", base_height, 0.0f, 0.9f, 0.1f))
    {
        new_props.start_base_height = base_height;
        new_props.end_base_height = base_height;
    }

    if (slider(result, "Height", height, 0.1f, 1.0f - base_height, 0.1f))
    {
        new_props.start_height = height;
        new_props.end_height = height;
    }

    if (ImGui::Checkbox("Tri-wall", &new_props.tri_wall))
    {
        result.always_update |= true;
    }
    if (new_props.tri_wall)
    {
        if (ImGui::Checkbox("Flip tri-wall?", &new_props.flip_wall))
        {
            result.always_update |= true;
        }
    }
    if (ImGui::CollapsingHeader("Advanced Wall Options"))
    {
        slider(result, "Start Base Height", new_props.start_base_height, 0.0f, 0.9f, 0.1f);
        slider(result, "Start Wall Height", new_props.start_height, 0.1f,
               1.0f - new_props.start_base_height, 0.1f);

        slider(result, "End Base Height", new_props.end_base_height, 0.0f, 0.9f, 0.1f);
        slider(result, "End Wall Height", new_props.end_height, 0.1f,
               1.0f - new_props.end_base_height, 0.1f);
    }

    texture_gui_tabs(result, "Textures_wall", textures,
                     {.name = "Front Texture",
                      .current = wall.properties.texture_front,
                      .new_texture = &new_props.texture_front},
                     {.name = "Back Texture",
                      .current = wall.properties.texture_back,
                      .new_texture = &new_props.texture_back});

    return {
        check_prop_updated(result, wall.properties, new_props),
        new_props,
    };
}

std::pair<UpdateResult, PlatformProps> platform_gui(const LevelTextures& textures,
                                                    const PlatformObject& platform)

{
    UpdateResult result;

    PlatformProps new_props = platform.properties;


    slider(result, "Width", new_props.width, 0.5f, 20.0f, 0.5f);
    slider(result, "Depth", new_props.depth, 0.5f, 20.0f, 0.5f);

    // Multiplied by 2 when mesh is created
    slider(result, "Base Height", new_props.base, 0.0f, 0.9f, 0.1f);

    enum_gui<PlatformStyle>(result, "Platform Style", platform.properties.style, new_props.style);


    texture_gui_tabs(result, "Textures_platform", textures,
                     {.name = "Top Texture",
                      .current = platform.properties.texture_top,
                      .new_texture = &new_props.texture_top},
                     {.name = "Bottom Texture",
                      .current = platform.properties.texture_bottom,
                      .new_texture = &new_props.texture_bottom});

    return {
        check_prop_updated(result, platform.properties, new_props),
        new_props,
    };
}

std::pair<UpdateResult, PolygonPlatformProps>
polygon_platform_gui(const LevelTextures& textures, const PolygonPlatformObject& poly)
{
    UpdateResult result;

    PolygonPlatformProps new_props = poly.properties;

    if (ImGui::Checkbox("Show Floor?", &new_props.visible))
    {
        result.always_update |= true;
    }

    if (new_props.visible)
    {

        // Multiplied by 2 when mesh is created
        slider(result, "Base Height", new_props.base, 0.0f, 0.9f, 0.1f);
        texture_gui_tabs(result, "Textures_platform", textures,
                         {.name = "Top Texture",
                          .current = poly.properties.texture_top,
                          .new_texture = &new_props.texture_top},
                         {.name = "Bottom Texture",
                          .current = poly.properties.texture_bottom,
                          .new_texture = &new_props.texture_bottom});
    }

    return {
        check_prop_updated(result, poly.properties, new_props),
        new_props,
    };
}

std::pair<UpdateResult, PillarProps> pillar_gui(const LevelTextures& textures,
                                                const PillarObject& pillar)
{
    UpdateResult result;
    auto new_props = pillar.properties;

    slider(result, "Size", new_props.size, 0.1f, 0.8f, 0.1f);
    slider(result, "Base Height", new_props.base_height, 0.0f, 0.9f, 0.1f);
    slider(result, "Height", new_props.height, 0.1f, 1.0f - new_props.base_height, 0.1f);

    // enum_gui(result, "Style", pillar.properties.style, new_props.style);
    ImGui::Separator();
    /*
    if (ImGui::Checkbox("Angled?", &new_props.angled))
    {
        result.always_update |= true;
    }*/

    texture_gui(result, "Texture", textures, pillar.properties.texture, new_props.texture);

    return {
        check_prop_updated(result, pillar.properties, new_props),
        new_props,
    };
}

std::pair<UpdateResult, RampProps> ramp_gui(const LevelTextures& textures, const RampObject& ramp)
{
    UpdateResult result;

    auto new_props = ramp.properties;

    slider(result, "Width", new_props.width, 0.5f, 20.0f, 0.5f);
    slider(result, "Depth", new_props.depth, 0.5f, 20.0f, 0.5f);

    slider(result, "Start Height", new_props.start_height, 0.0f, 0.9f, 0.1f);
    slider(result, "End Height", new_props.end_height, new_props.start_height + 0.1f, 1.0f, 0.1f);

    enum_gui(result, "Style", ramp.properties.style, new_props.style);
    ImGui::Separator();
    enum_gui(result, "Direction", ramp.properties.direction, new_props.direction);

    texture_gui_tabs(result, "Textures_Ramp", textures,
                     {.name = "Top Texture",
                      .current = ramp.properties.texture_top,
                      .new_texture = &new_props.texture_top},
                     {.name = "Bottom Texture",
                      .current = ramp.properties.texture_bottom,
                      .new_texture = &new_props.texture_bottom});

    return {
        check_prop_updated(result, ramp.properties, new_props),
        new_props,
    };
}

bool display_level_list(bool& show_load_dialog, std::string& name)
{
    bool result = false;

    if (ImGui::BeginCentredWindow("Load Level", {800, 800}))
    {
        ImGui::Text("Select a level to load:");
        ImGui::Separator();

        for (const auto& entry : std::filesystem::directory_iterator("./levels"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".cly")
            {
                std::string filename = entry.path().stem().string();
                if (ImGui::Button(filename.c_str()))
                {
                    name = filename;
                    result = true;
                    break;
                }
            }
        }

        if (ImGui::Button("Cancel"))
        {
            show_load_dialog = false;
        }
    }
    ImGui::End();
    return result;
}
