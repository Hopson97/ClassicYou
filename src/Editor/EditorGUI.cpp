#include "EditorGUI.h"

#include <SFML/Window/Mouse.hpp>
#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>

#include "../Util/ImGuiExtras.h"
#include "LevelTextures.h"

namespace
{
    /**
     * @brief UI for selecting a texture from a list of textures.
     *
     * @param title The title of the texture selection UI.
     * @param current_texture The currently selected texture.
     * @param textures The list of available textures.
     * @return int The index of the selected texture, or -1 if no texture is selected.
     */
    int texture_prop_gui(const char* title, TextureProp current_texture,
                         const LevelTextures& textures)
    {

        int new_texture = -1;
        ImGui::Text("%s", title);
        int imgui_id = 0;
        for (const auto& [name, texture] : textures.texture_2d_map)
        {

            if (imgui_id != 0)
            {
                ImGui::SameLine();
            }
            ImGui::PushID(imgui_id++);
            std::string button_id = name + "###" + title;
            auto old_texture = current_texture;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));

            // Give selected textures a red border
            if (auto texture_id = textures.get_texture(name))
            {
                if (texture_id == old_texture)
                {
                    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
                }
            }

            if (ImGui::ImageButton(button_id.c_str(), static_cast<ImTextureID>(texture.id),
                                   {32, 32}))
            {
                if (auto texture_id = textures.get_texture(name))
                {
                    new_texture = *texture_id;
                    std::println("Texture clicked: {}", name);
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

    bool texture_gui(const char* label, const LevelTextures& textures, TextureProp current,
                     TextureProp& new_prop)
    {
        auto texture = texture_prop_gui(label, current, textures);
        if (texture >= 0)
        {
            new_prop = texture;
            return true;
        }
        return false;
    }

    template <typename EnumType>
    bool enum_gui(const char* label, EnumType current, EnumType& new_prop)
    {
        ImGui::Text("%s", label);
        size_t n = 0;
        bool update = false;
        magic_enum::enum_for_each<EnumType>(
            [&](EnumType value)
            {
                int style = static_cast<int>(current);

                if (ImGui::RadioButton(magic_enum::enum_name(value).data(), &style,
                                       static_cast<int>(value)))
                {
                    update = true;
                    new_prop = value;
                }

                if (n++ < magic_enum::enum_count<EnumType>())
                {

                    ImGui::SameLine();
                }
            });
        return update;
    }
} // namespace

std::pair<ShouldUpdate, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall)

{
    bool update = false;
    WallProps new_props = wall.properties;

    update |= texture_gui("Front Texture", textures, wall.properties.texture_front,
                          new_props.texture_front);
    update |=
        texture_gui("Back Texture", textures, wall.properties.texture_back, new_props.texture_back);

    // When the wall is generating the mesh, these values are multiplied by 2.0f
    update |= ImGui::SliderFloatStepped("Base Height", new_props.base_height, 0.0f, 0.9f, 0.1f);
    update |= ImGui::SliderFloatStepped("Wall Height", new_props.wall_height, 0.1f,
                                        1.0f - new_props.base_height, 0.1f);

    return {
        ShouldUpdate{
            .value = update,
            .action = !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
        },
        new_props,
    };
}

std::pair<ShouldUpdate, PlatformProps> platform_gui(const LevelTextures& textures,
                                                    const PlatformObject& platform)

{
    bool update = false;
    PlatformProps new_props = platform.properties;

    update |= texture_gui("Top Texture", textures, platform.properties.texture_top,
                          new_props.texture_top);
    update |= texture_gui("Bottom Texture", textures, platform.properties.texture_bottom,
                          new_props.texture_bottom);
    update |= ImGui::SliderFloatStepped("Width", new_props.width, 0.5f, 100.0f, 0.5f);
    update |= ImGui::SliderFloatStepped("Depth", new_props.depth, 0.5f, 100.0f, 0.5f);

    // Multipled by 2 when mesh is created
    update |= ImGui::SliderFloatStepped("Base Height", new_props.base, 0.0f, 1.0f, 0.1f);

    update |= enum_gui<PlatformStyle>("Platform Style", platform.properties.style, new_props.style);

    return {
        ShouldUpdate{
            .value = update && platform.properties != new_props,
            .action = !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
        },
        new_props,
    };
}
