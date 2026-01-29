#include "ObjectSizePropertyEditor.h"

#include <imgui.h>

#include "../../Graphics/Camera.h"
#include "../../Graphics/MeshGeneration.h"
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
    state_floor_ = state.current_floor;

    auto rect = to_world_rectangle(position_, size_);

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (!active_dragging_ && !active_dragging_3d_ && pull_direction_ != PullDirection::None)
            {
                start_drag_position_ = state.node_hovered;
                active_dragging_ = true;
                update_previews();
                return true;
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        // If the mouse moves over one of the egdes in the 2D view, this enables it to be
        // highlighted
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

            // 3D picking for dragging edges - TODO the drag Y should be based on the edge, especially for ramps.
            glm::vec2 mouse_point = state.node_hovered;
            if (active_dragging_3d_)
            {
                auto intersect = camera_3d.find_mouse_floor_intersect(
                    {mouse->position.x, mouse->position.y}, base_y_);

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
                update_object(*state.selection.p_active_object, state.current_floor, actions, true);
                active_dragging_3d_ = false;
                active_dragging_ = false;
                return true;
            }
            save_update_ = false;
        }
        active_dragging_3d_ = false;
        active_dragging_ = false;
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
    if (state_floor_ != object_floor_)
    {
        return;
    }
    scene_shader_3d.set_uniform("use_texture", false);
    scene_shader_3d.set_uniform("use_colour", true);
    scene_shader_3d.set_uniform("model_matrix", create_model_matrix({}));
    scene_shader_3d.set_uniform("colour_multiplier", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
    /*

    selection_cube_3d_.bind();
    for (auto& offset : selector_3d_offsets_)
    {

        scene_shader_3d.set_uniform(
            "colour_multiplier", (pull_direction_ & offset.pull_direction) == offset.pull_direction
                                     ? glm::vec4{1, 0, 0, 1}
                                     : glm::vec4{1, 1, 1, 1});

        scene_shader_3d.set_uniform(
            "model_matrix", create_model_matrix({
                                {position_.x / TILE_SIZE_F + offset.x, object_floor_ * FLOOR_HEIGHT,
                                 position_.y / TILE_SIZE_F + offset.z},
                                {0, offset.angle, 0},
                            }));
        selection_cube_3d_.draw_elements();
    }

    */
    glLineWidth(5);

    top_line_preview_3d_.bind().draw_elements(gl::PrimitiveType::Lines);

    scene_shader_3d.set_uniform("use_colour", false);

    glLineWidth(3);
    if (active_dragging_3d_)
    {
        if (axis_line_x.indices.size() > 0)
        {
            axis_line_x.bind().draw_elements(gl::PrimitiveType::Lines);
        }
        if (axis_line_z.indices.size() > 0)
        {
            axis_line_z.bind().draw_elements(gl::PrimitiveType::Lines);
        }
    }
}

void ObjectSizePropertyEditor::render_to_picker(const MousePickingState& picker_state,
                                                gl::Shader& picker_shader)
{
    if (state_floor_ != object_floor_)
    {
        return;
    }

    if (picker_state.button != sf::Mouse::Button::Left ||
        picker_state.action != MousePickingState::Action::ButtonPressed)
    {
        return;
    }

    picker_shader.set_uniform("model_matrix", create_model_matrix({}));
    glLineWidth(10);

    picker_shader.set_uniform("object_id", PullDirection::Up);
    top_line_preview_3d_.bind().draw_elements(gl::PrimitiveType::Lines);

    /*
    selection_cube_3d_.bind();
    for (auto& offset : selector_3d_offsets_)
    {
        picker_shader.set_uniform("object_id", offset.pull_direction);
        picker_shader.set_uniform(
            "model_matrix", create_model_matrix({
                                {position_.x / TILE_SIZE_F + offset.x, object_floor_ * FLOOR_HEIGHT,
                                 position_.y / TILE_SIZE_F + offset.z},
                                {0, offset.angle, 0},
                            }));
        selection_cube_3d_.draw_elements();
    }*/

    // Check which one was picked
    GLint picked_id = 0;
    glReadPixels(picker_state.point.x, picker_state.point.y, 1, 1, GL_RED_INTEGER, GL_INT,
                 &picked_id);

    if (picked_id > -1)
    {
        active_dragging_3d_ = true;
        pull_direction_ = static_cast<PullDirection>(picked_id);
        update_previews();
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

    axis_line_x.clear();
    axis_line_z.clear();

    auto create_axis_line = [&](Mesh3D& mesh, PullDirection direction, const glm::vec3& axis,
                                glm::u8vec4 colour, float y)
    {
        for (const auto& offset : selector_3d_offsets_)
        {
            if (direction == offset.pull_direction)
            {
                glm::vec3 start{
                    position_.x / TILE_SIZE_F + offset.x,
                    y,
                    position_.y / TILE_SIZE_F + offset.z,
                };
                auto end = start + glm::vec3{250, 0, 250} * axis;

                add_line_to_mesh(mesh, start, end, colour);
            }
        }
    };

    auto create_axis_lines = [&](float y)
    {
        if ((pull_direction_ & PullDirection::Right) == PullDirection::Right)
        {
            create_axis_line(axis_line_x, PullDirection::Right, {1, 0, 0}, Colour::RED, y);
        }
        else if ((pull_direction_ & PullDirection::Left) == PullDirection::Left)
        {
            create_axis_line(axis_line_x, PullDirection::Left, {-1, 0, 0}, Colour::RED, y);
        }

        if ((pull_direction_ & PullDirection::Down) == PullDirection::Down)
        {
            create_axis_line(axis_line_z, PullDirection::Down, {0, 0, 1}, Colour::GREEN, y);
        }
        else if ((pull_direction_ & PullDirection::Up) == PullDirection::Up)
        {
            create_axis_line(axis_line_z, PullDirection::Up, {0, 0, -1}, Colour::GREEN, y);
        }
    };

    axis_line_x.update();
    axis_line_z.update();
    top_line_preview_3d_.clear();
    left_line_preview_3d_.clear();
    right_line_preview_3d_.clear();
    bottom_line_preview_3d_.clear();

    if (auto platform = std::get_if<PlatformObject>(&cached_object_.object_type))
    {
        float y = object_floor_ * FLOOR_HEIGHT + platform->properties.base * 2.0f;
        glm::vec3 start{
            position_.x / TILE_SIZE_F,
            y,
            position_.y / TILE_SIZE_F,
        };
        auto end = start + glm::vec3{size_.x, 0, 0};

        add_line_to_mesh(top_line_preview_3d_, start, end, Colour::GREEN);

        create_axis_lines(y);
        base_y_ = y;
    }
    else if (auto ramp = std::get_if<RampObject>(&cached_object_.object_type))
    {
        ramp->properties.size = size_;
        ramp->parameters.position = position_;
    }
    axis_line_x.update();
    axis_line_z.update();
    top_line_preview_3d_.update();
    left_line_preview_3d_.update();
    right_line_preview_3d_.update();
    bottom_line_preview_3d_.update();
}

bool ObjectSizePropertyEditor::size_within_bounds(float size) const
{
    return size >= 0.5 && size <= max_size_;
}

void ObjectSizePropertyEditor::generate_3d_offsets()
{
    float OFFSET = 0.025f;

    float right_edge = size_.x + OFFSET + HALF_CUBE;
    float left_edge = -OFFSET - HALF_CUBE;
    float bottom_edge = size_.y + OFFSET + HALF_CUBE;
    float top_edge = -OFFSET - HALF_CUBE;

    float mid_x = size_.x / 2.0f;
    float mid_z = size_.y / 2.0f;

    selector_3d_offsets_ = {
        Offset3D{
            .x = right_edge,
            .z = mid_z,
            .angle = 0.0f,
            .pull_direction = PullDirection::Right,
        },
        {
            .x = right_edge,
            .z = bottom_edge,
            .angle = -45.0f,
            .pull_direction = PullDirection::Down | PullDirection::Right,
        },
        {
            .x = mid_x,
            .z = bottom_edge,
            .angle = -90.0f,
            .pull_direction = PullDirection::Down,
        },
        {
            .x = left_edge,
            .z = bottom_edge,
            .angle = -135.0f,
            .pull_direction = PullDirection::Down | PullDirection::Left,
        },
        {
            .x = left_edge,
            .z = mid_z,
            .angle = -180.0f,
            .pull_direction = PullDirection::Left,
        },
        {
            .x = left_edge,
            .z = top_edge,
            .angle = -225.0f,
            .pull_direction = PullDirection::Up | PullDirection::Left,
        },
        {
            .x = mid_x,
            .z = top_edge,
            .angle = -270.0f,
            .pull_direction = PullDirection::Up,
        },
        {
            .x = right_edge,
            .z = top_edge,
            .angle = -315.0f,
            .pull_direction = PullDirection::Up | PullDirection::Right,
        },
    };
}
