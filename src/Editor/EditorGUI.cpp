#include "EditorGUI.h"

#include <SFML/Window/Mouse.hpp>
#include <imgui.h>

#include "../Util/ImGuiExtras.h"
#include "LevelTextures.h"

std::pair<ShouldUpdate, WallProps> wall_gui(const LevelTextures& textures, const WallObject& wall)

{
    bool update = false;
    WallProps new_props = wall.properties;

    auto texture_front = texture_prop_gui("Front Texture", wall.properties.texture_front, textures);
    if (texture_front >= 0)
    {
        new_props.texture_front = texture_front;
        update = true;
    }

    auto texture_back = texture_prop_gui("Back Texture", wall.properties.texture_back, textures);
    if (texture_back >= 0)
    {
        new_props.texture_back = texture_back;
        update = true;
    }

    update |= ImGui::SliderFloatStepped("Base Height", new_props.base_height, 0.0f, 1.8f, 0.2f);
    update |= ImGui::SliderFloatStepped("Wall Height", new_props.wall_height, 0.2f,
                                        2.0f - new_props.base_height, 0.2f);

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

    auto texture_top = texture_prop_gui("Top Texture", platform.properties.texture_top, textures);
    if (texture_top >= 0)
    {
        new_props.texture_top = texture_top;
        update = true;
    }

    auto texture_bottom =
        texture_prop_gui("Bottom Texture", platform.properties.texture_bottom, textures);
    if (texture_bottom >= 0)
    {
        new_props.texture_bottom = texture_bottom;
        update = true;
    }

    update |= ImGui::SliderFloatStepped("Width", new_props.width, 0.5f, 100.0f, 0.5f);
    update |= ImGui::SliderFloatStepped("Depth", new_props.depth, 0.5f, 100.0f, 0.5f);
    update |= ImGui::SliderFloatStepped("Base Height", new_props.base, 0.0f, 2.0f, 0.2f);

    return {
        ShouldUpdate{
            .value = update && platform.properties != new_props,
            .action = !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
        },
        new_props,
    };
}

int texture_prop_gui(const char* title, TextureProp current_texture, const LevelTextures& textures)
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
        if (auto texture_id = textures.get_texture(name))
        {

            if (texture_id == old_texture)
            {
                ImGui::PushStyleColor(ImGuiCol_Border,
                                      IM_COL32(255, 0, 0, 255));          // Set border color to red
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f); // Enable border
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
            }
        }

        if (ImGui::ImageButton(button_id.c_str(), static_cast<ImTextureID>(texture.id), {32, 32}))
        {
            if (auto texture_id = textures.get_texture(name))
            {
                new_texture = *texture_id;
                std::println("Texture clicked: {}", name);
            }
        }

        ImGui::PopStyleVar(); // Pop FramePadding and FrameBorderSize
        if (auto texture_id = textures.get_texture(name))
        {

            if (texture_id == old_texture)
            {
                ImGui::PopStyleVar(2);  // Pop FramePadding and FrameBorderSize
                ImGui::PopStyleColor(); // Pop Border color
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
