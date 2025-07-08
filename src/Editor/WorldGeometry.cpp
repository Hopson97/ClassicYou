#include "WorldGeometry.h"

#include <print>

#include <imgui.h>

#include "../Util/Maths.h"
#include "Actions.h"
#include "LevelTextures.h"

bool Wall::property_gui(EditorState& state, const LevelTextures& textures,
                        ActionManager& action_manager)
{
    bool update = false;
    WallProps new_props = props;
    if (ImGui::Begin("Properties"))
    {
        ImGui::Separator();

        auto texture_side_1 = display_texture_gui("Side 1", props.texture_side_1, textures);
        if (texture_side_1 >= 0)
        {
            new_props.texture_side_1.value = texture_side_1;
            update = true;
        }

        auto texture_side_2 = display_texture_gui("Side 2", props.texture_side_2, textures);
        if (texture_side_2 >= 0)
        {
            new_props.texture_side_2.value = texture_side_2;
            update = true;
        }

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
        Wall new_wall = *this;
        new_wall.props = new_props;
        action_manager.push_action(std::make_unique<UpdateWallAction>(*this, new_wall));
    }

    return true;
}

bool Wall::try_select_2d(const glm::vec2& point)
{
    return distance_to_line(point, {parameters.start, parameters.end}) < 15;
}
