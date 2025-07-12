#include "ObjectProperties.h"

#include <imgui.h>

#include "LevelTextures.h"

int display_texture_gui(const char* title, TextureProp current_texture,
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
