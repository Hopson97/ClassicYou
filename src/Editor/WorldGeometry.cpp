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

        auto texture_front = display_texture_gui("Side 1", props.texture_front, textures);
        if (texture_front >= 0)
        {
            new_props.texture_front = texture_front;
            update = true;
        }

        auto texture_back = display_texture_gui("Side 2", props.texture_back, textures);
        if (texture_back >= 0)
        {
            new_props.texture_back = texture_back;
            update = true;
        }

        ImGui::Separator();
        if (ImGui::Button("Set As Default"))
        {
            state.wall_default.texture_front = props.texture_front;
            state.wall_default.texture_back = props.texture_back;
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
