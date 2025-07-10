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
        ImGui::PushID(imgui_id++);

        ImGui::SameLine();
        std::string button_id = name + "###" + title;
        auto old_texture = current_texture;
        if (auto texture_id = textures.get_texture(name))
        {
            if (texture_id == old_texture)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {3.0f, 3.0f});
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

        if (auto texture_id = textures.get_texture(name))
        {
            if (texture_id == old_texture)
            {
                ImGui::PopStyleVar();
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
