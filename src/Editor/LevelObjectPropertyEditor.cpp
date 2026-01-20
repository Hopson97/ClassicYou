#include "LevelObjectPropertyEditor.h"

#include <imgui.h>

#include "Actions.h"
#include "EditorState.h"
#include "LevelObjects/LevelObjectHelpers.h"

ObjectSizeEditor::ObjectSizeEditor(LevelObject object, glm::vec2 position, glm::vec2 size)
    : PropertyEditor(object.object_id)
    , position_{position}
    , size_{size}
    , object_(object)
{
}

bool ObjectSizeEditor::handle_event(const sf::Event& event, EditorState& state,
                                    ActionManager& actions,
                                    const LevelTextures& drawing_pad_texture_map)
{
    auto rect = to_world_rectangle(position_, size_);

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (!active_dragging_ && pull_direction_ != PullDirection::None)
            {
                start_drag_position_ = state.node_hovered;
                active_dragging_ = true;
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (!active_dragging_)
        {
            // Find what line of the rectangle is being dragged in the 2D view
            Line top_line{
                .start = rect.position,
                .end = {rect.position.x + rect.size.x, rect.position.y},
            };

            Line right_line{
                .start = {rect.position.x + rect.size.x, rect.position.y},
                .end = {rect.position.x + rect.size.x, rect.position.y + rect.size.y},
            };

            pull_direction_ = PullDirection::None;
            if (distance_to_line(state.world_position_hovered, right_line) < TILE_SIZE)
            {
                pull_direction_ |= static_cast<int>(PullDirection::Right);
            }
        }

        if (active_dragging_)
        {
            if ((pull_direction_ & PullDirection::Right) == PullDirection::Right)
            {
                auto right_x = rect.position.x + rect.size.x;
                auto delta_x = (static_cast<float>(state.node_hovered.x) - right_x) / TILE_SIZE_F;
                auto new_size = size_.x + delta_x;

                // TODO Actually snap the value to 0.5 rather than just continuously adding
                if (new_size >= 0.5f)
                {
                    size_.x += delta_x;
                    update_object(state.current_floor, actions, false);
                }
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            active_dragging_)
        {
            active_dragging_ = false;
            update_object(state.current_floor, actions, true);
            return true;
        }
    }

    return false;
}

void ObjectSizeEditor::display_2d_editor()
{
}

void ObjectSizeEditor::update_object(int current_floor, ActionManager& actions, bool store_action)
{
    auto new_object = object_;
    if (auto platform = std::get_if<PlatformObject>(&new_object.object_type))
    {
        platform->properties.size = size_;
        platform->parameters.position = position_;
    }
    else if (auto ramp = std::get_if<RampObject>(&new_object.object_type))
    {
        ramp->properties.size = size_;
        ramp->parameters.position = position_;
    }

    actions.push_action(std::make_unique<UpdateObjectAction>(object_, new_object, current_floor),
                        store_action);
}
