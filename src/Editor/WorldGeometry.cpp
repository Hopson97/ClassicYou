#include "WorldGeometry.h"

#include <print>

#include <imgui.h>

#include "LevelTextures.h"

bool Wall::property_gui(EditorState& state, PropertyEditor& editor, const LevelTextures& textures)
{
    bool update = false;
    if (ImGui::Begin("Properties"))
    {
        ImGui::Separator();
        update |= editor.display_texture_gui("Side 1", props.texture_side_1, textures);
        ImGui::Separator();
        update |= editor.display_texture_gui("Side 2", props.texture_side_2, textures);

        ImGui::Separator();
        if (ImGui::Button("Set As Default"))
        {
            state.wall_default.texture_side_1 = props.texture_side_1;
            state.wall_default.texture_side_2 = props.texture_side_2;
        }
    }
    ImGui::End();

    if (update)
    {
        for (auto& callback : editor.on_property_update)
        {
            callback(*this);
        }
    }
    return true;
}
