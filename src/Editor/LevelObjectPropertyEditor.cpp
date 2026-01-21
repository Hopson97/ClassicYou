#include "LevelObjectPropertyEditor.h"

#include <imgui.h>

#include "Actions.h"
#include "EditorState.h"
#include "LevelObjects/LevelObjectHelpers.h"

namespace
{
    constexpr auto MIN_SELECT_DISTANCE_SMALL = HALF_TILE_SIZE_F / 4 - 1;
    constexpr auto MIN_SELECT_DISTANCE_LARGE = HALF_TILE_SIZE_F / 2;

    struct LineToPullDirectionMapping
    {
        const Line line;
        const ObjectSizeEditor::PullDirection pull_direction;
        const float min_select_dist = MIN_SELECT_DISTANCE_LARGE;
    };

    /// Creates the lines the make up the given rectangle, and their corresponding edge direction.
    /// As small sized objects could be hard to move around without accidently selecting an edge,
    /// the minimum select distance is updated based on this per-edge
    auto create_line_to_direction_map(const Rectangle& rect)
    {
        return std::array<struct LineToPullDirectionMapping, 4>{{
            {
                {.start = rect.position, .end = {rect.position.x + rect.size.x, rect.position.y}},
                ObjectSizeEditor::PullDirection::Up,
                rect.size.y < TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = {rect.position.x + rect.size.x, rect.position.y},
                 .end = {rect.position.x + rect.size.x, rect.position.y + rect.size.y}},
                ObjectSizeEditor::PullDirection::Right,
                rect.size.x < TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = rect.position, .end = {rect.position.x, rect.position.y + rect.size.y}},
                ObjectSizeEditor::PullDirection::Left,
                rect.size.x < TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = {rect.position.x, rect.position.y + rect.size.y},
                 .end = {rect.position.x + rect.size.x, rect.position.y + rect.size.y}},
                ObjectSizeEditor::PullDirection::Down,
                rect.size.y < TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
        }};
    }
} // namespace

ObjectSizeEditor::ObjectSizeEditor(glm::vec2 position, glm::vec2 size, int object_floor)
    : position_{position}
    , size_{size}
    , object_floor_{object_floor}

{
}

bool ObjectSizeEditor::handle_event(const sf::Event& event, EditorState& state,
                                    ActionManager& actions,
                                    const LevelTextures& drawing_pad_texture_map)
{
    if (object_floor_ != state.current_floor)
    {
        return false;
    }

    auto rect = to_world_rectangle(position_, size_);

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (!active_dragging_ && pull_direction_ != PullDirection::None)
            {
                start_drag_position_ = state.node_hovered;
                active_dragging_ = true;
                return true;
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (!active_dragging_)
        {
            pull_direction_ = PullDirection::None;

            // Using | enables dragging corners "out of the box". As the min distance is much less
            // than the minimum size of an object, it should be impossible to select left and right/
            // up and down at the same time.
            auto line_to_direction_map = create_line_to_direction_map(rect);
            for (auto& [line, direction, min_select_distance] : line_to_direction_map)
            {
                if (distance_to_line(state.world_position_hovered, line) < min_select_distance)
                {
                    pull_direction_ |= static_cast<int>(direction);
                }
            }
        }

        if (active_dragging_)
        {
            glm::vec2 new_size = size_;
            glm::vec2 new_position = position_;
            if ((pull_direction_ & PullDirection::Right) == PullDirection::Right)
            {
                drag_positive_direction_2d(rect, 0, state.node_hovered, new_size);
            }
            else if ((pull_direction_ & PullDirection::Left) == PullDirection::Left)
            {
                drag_negative_direction_2d(rect, 0, state.node_hovered, new_position, new_size);
            }

            if ((pull_direction_ & PullDirection::Down) == PullDirection::Down)
            {
                drag_positive_direction_2d(rect, 1, state.node_hovered, new_size);
            }
            else if ((pull_direction_ & PullDirection::Up) == PullDirection::Up)
            {
                drag_negative_direction_2d(rect, 1, state.node_hovered, new_position, new_size);
            }

            if (new_size != size_ || new_position != position_)
            {
                size_ = new_size;
                position_ = new_position;
                update_object(*state.selection.p_active_object, state.current_floor, actions,
                              false);
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            active_dragging_)
        {
            active_dragging_ = false;
            update_object(*state.selection.p_active_object, state.current_floor, actions, true);
            return true;
        }
    }

    return false;
}

void ObjectSizeEditor::display_2d_editor()
{
}

void ObjectSizeEditor::drag_positive_direction_2d(const Rectangle& object_rect, int axis,
                                                  glm::ivec2 node_hovered, glm::vec2& new_size)
{
    auto delta = (node_hovered[axis] - object_rect.position[axis]) / TILE_SIZE_F;
    auto snapped_delta = std::round(delta * 2.0f) / 2.0f;

    if (snapped_delta >= 0.5 && snapped_delta != size_[axis])
    {
        new_size[axis] = snapped_delta;
    }
}

void ObjectSizeEditor::drag_negative_direction_2d(const Rectangle& object_rect, int axis,
                                                  glm::ivec2 node_hovered, glm::vec2& new_position,
                                                  glm::vec2& new_size)
{
    auto delta = (object_rect.position[axis] - node_hovered[axis]) / TILE_SIZE_F;
    auto snapped_delta = std::round(delta * 2.0f) / 2.0f;

    auto next_position = position_[axis] - snapped_delta * TILE_SIZE_F;
    auto next_size = (size_[axis] + snapped_delta);

    if (next_size >= 0.5 && next_size != size_[axis] && next_position != position_[axis])
    {
        new_position[axis] = next_position;
        new_size[axis] = next_size;
    }
}

void ObjectSizeEditor::update_object(const LevelObject& object, int current_floor,
                                     ActionManager& actions, bool store_action)
{
    auto new_object = object;
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

    actions.push_action(std::make_unique<UpdateObjectAction>(object, new_object, current_floor),
                        store_action);
}
