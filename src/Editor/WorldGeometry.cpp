#include "WorldGeometry.h"

#include <print>

#include <imgui.h>

#include "LevelTextures.h"

namespace
{
    bool prop_gui(const char* name, TextureProp& texture_prop, const LevelTextures& textures)
    {
        bool update = false;
        ImGui::Text(name);
        constexpr float image_size = 64.0f;
        int id = 0;
        for (const auto& [name, texture] : textures.texture_2d_map)
        {
            ImGui::PushID(id++);
            ImGui::SameLine();
            if (ImGui::ImageButton(name.c_str(), static_cast<ImTextureID>(texture.id), {32, 32}))
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
        ImGui::Separator();
        update |= prop_gui("Side 1", props.texture_side_1, textures);

        ImGui::Separator();
        // update |= prop_gui("Side 2", props.texture_side_2, textures);

        ImGui::Separator();
        if (ImGui::Button("Set As Default"))
        {
            state.wall_default.texture_side_1 = props.texture_side_1;
           // state.wall_default.texture_side_2 = props.texture_side_2;
        }
    }
    ImGui::End();
    std::print("Update: {}", update);
    return update;
}
