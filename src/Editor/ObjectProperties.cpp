#include "ObjectProperties.h"

#include <imgui.h>

#include "LevelTextures.h"


//void TextureUpdateGUI::on_select(std::function<void(int, GLuint)>&& callback)
//{
//    on_texture_select_callbacks_.push_back(callback);
//}

bool PropertyEditor::display_texture_gui(const char* title, TextureProp& current_texure,
                                         const LevelTextures& textures)
{
    bool update = false;
    ImGui::Text("%s", title);
    int id = 0;
    for (const auto& [name, texture] : textures.texture_2d_map)
    {
        ImGui::PushID(id++);

        ImGui::SameLine();
        std::string button_id = name + "###" + title;
        auto old_texture = current_texure.value;
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
                current_texure.value = *texture_id;
                update = true;
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
    return update;
}
