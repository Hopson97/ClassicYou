#include "WorldGeometry.h"

#include <print>

#include <imgui.h>

#include "LevelTextures.h"

namespace
{
    bool prop_gui(const char* name, TextureProp& texture_prop, const LevelTextures& textures,
                  int& id)
    {
        bool update = false;
        ImGui::Text("%s", name);
        for (const auto& [name, texture] : textures.texture_2d_map)
        {
            ImGui::PushID(id++);

            ImGui::SameLine();
            std::string button_id = name + "###" + std::to_string(id);

            if (ImGui::ImageButton(button_id.c_str(), static_cast<ImTextureID>(texture.id),
                                   {32, 32}))
            {
                if (auto texture_id = textures.get_texture(name))
                {
                    texture_prop.value = *texture_id;
                    update = true;
                }

                std::println("Texture clicked: {}", name);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", name.c_str());
            }
            ImGui::PopID();
        }
        return update;
    }
} // namespace

bool Wall::property_gui(EditorState& state, const LevelTextures& textures)
{
    bool update = false;
    if (ImGui::Begin("Wall Properties"))
    {
        int button_id = 0;
        ImGui::Separator();
        update |= prop_gui("Side 1", props.texture_side_1, textures, button_id);

        ImGui::Separator();
        update |= prop_gui("Side 2", props.texture_side_2, textures, button_id);

        ImGui::Separator();
        if (ImGui::Button("Set As Default"))
        {
            state.wall_default.texture_side_1 = props.texture_side_1;
            state.wall_default.texture_side_2 = props.texture_side_2;
        }
    }
    ImGui::End();

    ImGui::ShowDemoWindow();

    return update;
}
