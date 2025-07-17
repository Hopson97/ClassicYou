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

    /// Displays a list of available textures as a list of buttons, returns the ID of the given
    /// texture if a button is clicked
    int texture_prop_gui(const char* label, TextureProp current_texture,
                         const LevelTextures& textures)
    {
        int new_texture = -1;
        ImGui::Text("%s", label);
        int imgui_id = 0;
        for (const auto& [name, texture] : textures.texture_2d_map)
        {

            if (imgui_id != 0)
            {
                if (imgui_id % 5 != 0)
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
                if (texture_id == old_texture)
                {
                    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BORDER_PADDING);
                }
            }

            if (ImGui::ImageButton(button_id.c_str(), static_cast<ImTextureID>(texture.id),
                                   {32, 32}))
            {
                if (auto texture_id = textures.get_texture(name))
                {
                    new_texture = *texture_id;
                }
            }

            // Pop given styles
            ImGui::PopStyleVar();
            if (auto texture_id = textures.get_texture(name))
            {
                if (texture_id == old_texture)
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
        return new_texture;
    }

    /// GUI for selecting a texture from the given "LevelTextures" object
    void texture_gui(UpdateResult& result, const char* label, const LevelTextures& textures,
                     TextureProp current, TextureProp& new_texture)
    {
        auto texture = texture_prop_gui(label, current, textures);
        if (texture >= 0)
        {

            new_texture = texture;
            result.always_update |= true;
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

                if (ImGui::RadioButton(magic_enum::enum_name(value).data(), &style,
                                       static_cast<int>(value)))
                {
                    result.always_update |= true;
                    new_value = value;
                }

                if (++n < magic_enum::enum_count<EnumType>() - 1)
                {
                    ImGui::SameLine();
                }
            });
    }

    /// Wrapper around stepped slider for updating numerical props
    void slider(UpdateResult& result, const char* label, float& value, float min, float max,
                float interval, const char* fmt = "%.1f")
    {
        if (ImGui::SliderFloatStepped(label, value, min, max, interval, fmt))
        {
            result.continuous_update |= true;
        }

        // Called once when the user releases the mouse after changing the value - used to prevent
        // creating history until a selection within the slider is made
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            result.action = true;
        }
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

    WallProps new_props = wall.properties;

    if (ImGui::BeginTabBar("Textures_wall"))
    {
        if (ImGui::BeginTabItem("Front Texture"))
        {
            texture_gui(result, "Front Texture", textures, wall.properties.texture_front,
                        new_props.texture_front);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Back Texture"))
        {
            texture_gui(result, "Back Texture", textures, wall.properties.texture_back,
                        new_props.texture_back);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // When the wall is generating the mesh, these values are multiplied by 2.0f
    slider(result, "Base Height", new_props.base_height, 0.0f, 0.9f, 0.1f);
    slider(result, "Wall Height", new_props.wall_height, 0.1f, 1.0f - new_props.base_height, 0.1f);

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

    if (ImGui::BeginTabBar("Textures_platform"))
    {
        if (ImGui::BeginTabItem("Top Texture"))
        {
            texture_gui(result, "Top Texture", textures, platform.properties.texture_top,
                        new_props.texture_top);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Bottom Texture"))
        {
            texture_gui(result, "Bottom Texture", textures, platform.properties.texture_bottom,
                        new_props.texture_bottom);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    enum_gui<PlatformStyle>(result, "Platform Style", platform.properties.style, new_props.style);

    slider(result, "Width", new_props.width, 0.5f, 20.0f, 0.5f);
    slider(result, "Depth", new_props.depth, 0.5f, 20.0f, 0.5f);

    // Multiplied by 2 when mesh is created
    slider(result, "Base Height", new_props.base, 0.0f, 0.9f, 0.1f);

    return {
        check_prop_updated(result, platform.properties, new_props),
        new_props,
    };
}

std::pair<UpdateResult, GroundProps> ground_gui(const LevelTextures& textures,
                                                const GroundObject& ground)
{
    UpdateResult result;

    GroundProps new_props = ground.properties;

    if (ImGui::Checkbox("Show Floor?", &new_props.visible))
    {
        result.continuous_update |= true;
    }

    if (new_props.visible)
    {
        if (ImGui::BeginTabBar("Textures_ground"))
        {
            if (ImGui::BeginTabItem("Top Texture"))
            {
                texture_gui(result, "Top Texture", textures, ground.properties.texture_top,
                            new_props.texture_top);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bottom Texture"))
            {
                texture_gui(result, "Bottom Texture", textures, ground.properties.texture_bottom,
                            new_props.texture_bottom);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    // For now the ground is stored seperately to the rest of the objects as it is one-per-floor
    // This means actions do not work, hence this work-around
    if (result.always_update)
    {
        result.always_update = false;
        result.continuous_update = true;
        result.action = false;
    }

    return {
        check_prop_updated(result, ground.properties, new_props),
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
