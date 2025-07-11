#include "LevelObjects.h"

#include <print>

#include <imgui.h>

#include "../Util/Maths.h"
#include "Actions.h"
#include "LevelTextures.h"

namespace
{
    std::pair<bool, WallProps> wall_gui(EditorState& state, const LevelTextures& textures,
                                        const WallObject& wall)
    {
        bool update = false;
        WallProps new_props = wall.properties;
        if (ImGui::Begin("Properties"))
        {
            ImGui::Separator();

            auto texture_front =
                display_texture_gui("Side 1", wall.properties.texture_front, textures);
            if (texture_front >= 0)
            {
                new_props.texture_front = texture_front;
                update = true;
            }

            auto texture_back =
                display_texture_gui("Side 2", wall.properties.texture_back, textures);
            if (texture_back >= 0)
            {
                new_props.texture_back = texture_back;
                update = true;
            }

            ImGui::Separator();
            if (ImGui::Button("Set As Default"))
            {
                state.wall_default.texture_front = wall.properties.texture_front;
                state.wall_default.texture_back = wall.properties.texture_back;
            }
        }
        ImGui::End();

        return {update, new_props};
    }
} // namespace

void LevelObjectV2::property_gui(EditorState& state, const LevelTextures& textures,
                                 ActionManager& action_manager)
{
    if (auto wall = std::get_if<WallObject>(&object_type))
    {
        auto [update, new_props] = wall_gui(state, textures, *wall);
        if (update)
        {
            LevelObjectV2 new_object = *this;

            std::get<WallObject>(new_object.object_type).properties = new_props;
            action_manager.push_action(std::make_unique<UpdateObjectAction>(*this, new_object));
        }
    }
}
