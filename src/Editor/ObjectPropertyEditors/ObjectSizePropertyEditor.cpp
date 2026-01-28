#include "ObjectSizePropertyEditor.h"

#include <imgui.h>

#include "../../Graphics/Camera.h"
#include "../Actions.h"
#include "../EditorState.h"
#include "../LevelObjects/LevelObjectHelpers.h"

namespace
{
    constexpr auto MIN_SELECT_DISTANCE_SMALL = HALF_TILE_SIZE_F / 4 - 1;
    constexpr auto MIN_SELECT_DISTANCE_LARGE = HALF_TILE_SIZE_F / 2;

    struct LineToPullDirectionMapping
    {
        const Line line;
        const ObjectSizePropertyEditor::PullDirection pull_direction;
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
                ObjectSizePropertyEditor::PullDirection::Up,
                rect.size.y <= TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = {rect.position.x + rect.size.x, rect.position.y},
                 .end = {rect.position.x + rect.size.x, rect.position.y + rect.size.y}},
                ObjectSizePropertyEditor::PullDirection::Right,
                rect.size.x <= TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = rect.position, .end = {rect.position.x, rect.position.y + rect.size.y}},
                ObjectSizePropertyEditor::PullDirection::Left,
                rect.size.x <= TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
            {
                {.start = {rect.position.x, rect.position.y + rect.size.y},
                 .end = {rect.position.x + rect.size.x, rect.position.y + rect.size.y}},
                ObjectSizePropertyEditor::PullDirection::Down,
                rect.size.y <= TILE_SIZE ? MIN_SELECT_DISTANCE_SMALL : MIN_SELECT_DISTANCE_LARGE,
            },
        }};
    }
    constexpr float CUBE_SIZE = 0.25;
    constexpr float HALF_CUBE = CUBE_SIZE / 2.0f;
} // namespace

ObjectSizePropertyEditor::ObjectSizePropertyEditor(const LevelObject& object, glm::vec2 position,
                                                   glm::vec2 size, int object_floor)
    : position_{position}
    , size_{size}
    , object_floor_{object_floor}
    , cached_object_(object)
{
    update_previews();

    selection_cube_3d_ = generate_cube_mesh({CUBE_SIZE, CUBE_SIZE, CUBE_SIZE});
    selection_cube_3d_.buffer();

    generate_3d_offsets();
}

bool ObjectSizePropertyEditor::handle_event(const sf::Event& event, EditorState& state,
                                            ActionManager& actions,
                                            const LevelTextures& drawing_pad_texture_map,
                                            const Camera& camera_3d)
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
            if (!active_dragging_ && !active_dragging_3d_ && pull_direction_ != PullDirection::None)
            {
                start_drag_position_ = state.node_hovered;
                active_dragging_ = true;
                return true;
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        if (!active_dragging_ && !active_dragging_3d_)
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

        if (active_dragging_ || active_dragging_3d_)
        {
            glm::vec2 new_size = size_;
            glm::vec2 new_position = position_;

            glm::vec2 mouse_point = state.node_hovered;
            if (active_dragging_3d_)
            {
                auto intersect = camera_3d.find_mouse_floor_intersect(
                    {mouse->position.x, mouse->position.y}, state.current_floor * FLOOR_HEIGHT);

                mouse_point = glm::vec2{intersect.x, intersect.z} * TILE_SIZE_F;
            }

            try_resize(rect, mouse_point, new_position, new_size);

            if (new_size != size_ || new_position != position_)
            {
                size_ = new_size;
                position_ = new_position;
                update_object(*state.selection.p_active_object, state.current_floor, actions,
                              false);
                update_previews();
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        pull_direction_ = PullDirection::None;
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if ((active_dragging_ || active_dragging_3d_) && save_update_)
            {
                active_dragging_3d_ = false;
                active_dragging_ = false;
                update_object(*state.selection.p_active_object, state.current_floor, actions, true);
                return true;
            }
            save_update_ = false;
        }
    }

    return false;
}

void ObjectSizePropertyEditor::render_preview_2d(gl::Shader& scene_shader_2d)
{
    auto draw_line_preview = [&](Mesh2DWorld& mesh, PullDirection direction)
    {
        if ((pull_direction_ & direction) == direction)
        {
            mesh.bind().draw_elements(gl::PrimitiveType::Lines);
        }
    };

    glLineWidth(3);

    scene_shader_2d.set_uniform("model_matrix", create_model_matrix({}));
    scene_shader_2d.set_uniform("use_texture", false);
    scene_shader_2d.set_uniform("is_selected", false);
    scene_shader_2d.set_uniform("on_floor_below", false);

    draw_line_preview(top_line_preview_, PullDirection::Up);
    draw_line_preview(right_line_preview_, PullDirection::Right);
    draw_line_preview(left_line_preview_, PullDirection::Left);
    draw_line_preview(bottom_line_preview_, PullDirection::Down);
}

void ObjectSizePropertyEditor::render_preview_3d(gl::Shader& scene_shader_3d)
{
    scene_shader_3d.set_uniform("use_texture", false);
    scene_shader_3d.set_uniform("use_colour", true);

    selection_cube_3d_.bind();
    for (auto& offset : selector_3d_offsets_)
    {

        scene_shader_3d.set_uniform(
            "colour_multiplier", (pull_direction_ & offset.pull_direction) == offset.pull_direction
                                     ? glm::vec4{1, 0, 0, 1}
                                     : glm::vec4{1, 1, 1, 1});

        scene_shader_3d.set_uniform("model_matrix", create_model_matrix({{
                                                        position_.x / TILE_SIZE_F + offset.x,
                                                        object_floor_ * FLOOR_HEIGHT,
                                                        position_.y / TILE_SIZE_F + offset.z,
                                                    }}));
        selection_cube_3d_.draw_elements();
    }
    scene_shader_3d.set_uniform("use_colour", false);
}

void ObjectSizePropertyEditor::render_to_picker(const MousePickingState& picker_state,
                                                gl::Shader& picker_shader)
{
    if (picker_state.button != sf::Mouse::Button::Left ||
        picker_state.action != MousePickingState::Action::ButtonPressed)
    {
        return;
    }
    selection_cube_3d_.bind();
    for (auto& offset : selector_3d_offsets_)
    {
        picker_shader.set_uniform("object_id", offset.pull_direction);
        picker_shader.set_uniform("model_matrix", create_model_matrix({{
                                                      position_.x / TILE_SIZE_F + offset.x,
                                                      object_floor_ * FLOOR_HEIGHT,
                                                      position_.y / TILE_SIZE_F + offset.z,
                                                  }}));
        selection_cube_3d_.draw_elements();
    }

    // Check which one was picked
    GLint picked_id = 0;
    glReadPixels(picker_state.point.x, picker_state.point.y, 1, 1, GL_RED_INTEGER, GL_INT,
                 &picked_id);

    std::println("Object Size Property Editor picked-> {}", picked_id);

    if (picked_id > -1)
    {
        active_dragging_3d_ = true;
        pull_direction_ = static_cast<PullDirection>(picked_id);
    }
}

void ObjectSizePropertyEditor::try_resize(const Rectangle& rect, glm::vec2 intersect,
                                          glm::vec2& new_position, glm::vec2& new_size)
{

    if ((pull_direction_ & PullDirection::Right) == PullDirection::Right)
    {
        drag_positive_direction(rect, 0, intersect, new_size);
    }
    else if ((pull_direction_ & PullDirection::Left) == PullDirection::Left)
    {
        drag_negative_direction(rect, 0, intersect, new_position, new_size);
    }

    if ((pull_direction_ & PullDirection::Down) == PullDirection::Down)
    {
        drag_positive_direction(rect, 1, intersect, new_size);
    }
    else if ((pull_direction_ & PullDirection::Up) == PullDirection::Up)
    {
        drag_negative_direction(rect, 1, intersect, new_position, new_size);
    }
}

void ObjectSizePropertyEditor::drag_positive_direction(const Rectangle& object_rect, int axis,
                                                       glm::vec2 node_hovered, glm::vec2& new_size)
{
    auto delta = (node_hovered[axis] - object_rect.position[axis]) / TILE_SIZE_F;
    auto snapped_delta = std::round(delta * 2.0f) / 2.0f;

    if (size_within_bounds(snapped_delta) && snapped_delta != size_[axis])
    {
        new_size[axis] = snapped_delta;
    }
}

void ObjectSizePropertyEditor::drag_negative_direction(const Rectangle& object_rect, int axis,
                                                       glm::vec2 node_hovered,
                                                       glm::vec2& new_position, glm::vec2& new_size)
{
    auto delta = (object_rect.position[axis] - node_hovered[axis]) / TILE_SIZE_F;
    auto snapped_delta = std::round(delta * 2.0f) / 2.0f;

    auto next_position = position_[axis] - snapped_delta * TILE_SIZE_F;
    auto next_size = (size_[axis] + snapped_delta);

    if (size_within_bounds(next_size) && next_size != size_[axis] &&
        next_position != position_[axis])
    {
        new_position[axis] = next_position;
        new_size[axis] = next_size;
    }
}

void ObjectSizePropertyEditor::update_object(const LevelObject& object, int current_floor,
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

    actions.push_action(
        std::make_unique<UpdateObjectAction>(cached_object_, new_object, current_floor),
        store_action);
    generate_3d_offsets();

    if (store_action)
    {
        cached_object_ = new_object;
    }
    save_update_ = true;
}

void ObjectSizePropertyEditor::update_previews()
{
    auto lines = create_line_to_direction_map(to_world_rectangle(position_, size_));

    generate_line_mesh(top_line_preview_, lines[0].line, glm::u8vec4{255, 0, 255, 255});
    generate_line_mesh(right_line_preview_, lines[1].line, glm::u8vec4{255, 0, 255, 255});
    generate_line_mesh(left_line_preview_, lines[2].line, glm::u8vec4{255, 0, 255, 255});
    generate_line_mesh(bottom_line_preview_, lines[3].line, glm::u8vec4{255, 0, 255, 255});

    top_line_preview_.update();
    right_line_preview_.update();
    left_line_preview_.update();
    bottom_line_preview_.update();
}

bool ObjectSizePropertyEditor::size_within_bounds(float size) const
{
    return size >= 0.5 && size <= max_size_;
}

void ObjectSizePropertyEditor::generate_3d_offsets()
{
    selector_3d_offsets_ = {
        Offset3D{
            .x = size_.x / 2.0f - HALF_CUBE,
            .z = -CUBE_SIZE,
            .pull_direction = PullDirection::Up,
        },
        {
            .x = -CUBE_SIZE,
            .z = size_.y / 2.0f - HALF_CUBE,
            .pull_direction = PullDirection::Left,
        },
        {
            .x = size_.x / 2.0f - HALF_CUBE,
            .z = size_.y,
            .pull_direction = PullDirection::Down,
        },
        {
            .x = size_.x,
            .z = size_.y / 2.0f - HALF_CUBE,
            .pull_direction = PullDirection::Right,
        },
        {
            .x = -CUBE_SIZE,
            .z = -CUBE_SIZE,
            .pull_direction = PullDirection::Up | PullDirection::Left,
        },
        {
            .x = size_.x,
            .z = -CUBE_SIZE,
            .pull_direction = PullDirection::Up | PullDirection::Right,
        },
        {
            .x = -CUBE_SIZE,
            .z = size_.y,
            .pull_direction = PullDirection::Down | PullDirection::Left,
        },
        {
            .x = size_.x,
            .z = size_.y,
            .pull_direction = PullDirection::Down | PullDirection::Right,
        },
    };
}
