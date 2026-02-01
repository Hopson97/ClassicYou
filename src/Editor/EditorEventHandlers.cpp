#include "EditorEventHandlers.h"

#include <imgui.h>

#include "../Graphics/Camera.h"
#include "../Util/ImGuiExtras.h"
#include "Actions.h"
#include "EditorLevel.h"
#include "EditorUtils.h"

// =======================================
//          ObjectMoveHandler
// =======================================
ObjectMoveHandler::ObjectMoveHandler(EditorLevel& level, ActionManager& action_manager)
    : p_level_(&level)
    , p_action_manager_(&action_manager)
{
}

bool ObjectMoveHandler::handle_move_events(const sf::Event& event, const EditorState& state,
                                           ToolType current_tool, const Camera& camera_3d)
{
    // When dragging an object and the final placement is decided, this is set to true. This is to
    // prevent calls to the tool event fuctions, which can cause objects to unintentionally placed
    // where the object was moved to.
    bool finish_move = false;
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            // To avoid moving a wall that was just placed when placing an adjacent wall, this
            // checks if the last object was indeed a wall. If it is, then it cannot be moved.
            if (state.selection.objects.size() == 1 && current_tool == ToolType::CreateWall)
            {
                auto object = p_level_->get_object(state.selection.objects[0]);
                if (!object)
                {
                    return finish_move;
                }

                // Return early to prevent unintentioanl repositioning.
                if (std::get_if<WallObject>(&object->object_type))
                {
                    if (object->object_id == p_level_->last_placed_id())
                    {
                        return finish_move;
                    }
                }
            }

            // Check if any of the selected objects were clicked - if they were then it means
            // the whole group can be moved around
            bool moving_selection = false;
            for (auto object : p_level_->get_objects(state.selection.objects))
            {
                moving_selection |= object->try_select_2d(state.world_position_hovered);
            }

            // Start dragging the selected object
            if (moving_selection)
            {
                move_start_tile_ = state.node_hovered;
                moving_object_ = true;

                start_move(state.selection);
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        if (state.selection.has_selection())
        {
            if (moving_object_)
            {
                move_offset_ = state.node_hovered - move_start_tile_;
            }
            else if (moving_object_3d_)
            {
                move_offset_ = state.node_hovered - move_start_tile_;

                auto intersect = get_mouse_floor_snapped_intersect(camera_3d, mouse->position,
                                                                   state.current_floor);
                move_offset_ = glm::ivec2{intersect} - move_start_tile_;
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // Handle left button release
        if (mouse->button == sf::Mouse::Button::Left)
        {
            if (moving_object_ || moving_object_3d_)
            {
                std::vector<LevelObject> new_objects;
                new_objects.reserve(moving_objects_.size());

                for (auto object : moving_objects_)
                {
                    new_objects.push_back(*object);
                }

                for (auto& new_object : new_objects)
                {
                    new_object.move(move_offset_);
                }

                move_offset_ = glm::vec2{0};

                p_action_manager_->push_action(
                    std::make_unique<BulkUpdateObjectAction>(moving_object_cache_, new_objects),
                    true);

                finish_move = true;
            }

            moving_object_ = false;
            moving_object_3d_ = false;
        }
    }
    return finish_move;
}

glm::vec2 ObjectMoveHandler::get_move_offset() const
{
    return move_offset_;
}

bool ObjectMoveHandler::is_moving_objects() const
{
    return moving_object_ || moving_object_3d_;
}

bool ObjectMoveHandler::try_start_move_mouse_picker(const MousePickingState& picker_state,
                                                    gl::Shader& picker_shader,
                                                    const EditorLevel& level,
                                                    const EditorState& state,
                                                    const Camera& camera_3d)
{
    if (moving_object_3d_ || picker_state.button != sf::Mouse::Button::Left ||
        picker_state.action != MousePickingState::Action::ButtonPressed)
    {
        return false;
    }

    level.render_subset_to_picker(picker_shader, state.selection.objects);

    GLint picked_id = 0;
    glReadPixels(picker_state.point.x, picker_state.point.y, 1, 1, GL_RED_INTEGER, GL_INT,
                 &picked_id);
    if (picked_id > -1)
    {
        moving_object_3d_ = true;
        auto intersect = camera_3d.find_mouse_floor_intersect(
            {picker_state.unscaled_point.x, picker_state.unscaled_point.y},
            state.current_floor * FLOOR_HEIGHT);
        move_start_tile_ = {
            std::round((intersect.x * TILE_SIZE_F) / HALF_TILE_SIZE_F) * HALF_TILE_SIZE_F,
            std::round((intersect.z * TILE_SIZE_F) / HALF_TILE_SIZE_F) * HALF_TILE_SIZE_F};

        start_move(state.selection);
        return true;
    }
    return false;
}

void ObjectMoveHandler::start_move(const Selection& selection)
{
    moving_object_cache_.clear();
    moving_objects_.clear();
    moving_objects_ = p_level_->get_objects(selection.objects);
    for (auto object : moving_objects_)
    {
        moving_object_cache_.push_back(*object);
    }
}

void CopyPasteHandler::handle_event(const sf::Event& event, const Selection& selection,
                                    int current_floor)
{
    if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        switch (key->code)
        {
            // Copy functionality with CTRL+C
            case sf::Keyboard::Key::C:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    copy_selection(selection, current_floor);
                }
                break;

            // Paste functionality with CTRL+V
            case sf::Keyboard::Key::V:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    paste_selection(current_floor);
                }
                break;

            default:
                break;
        }
    }
}

// =======================================
//          CopyPasteHandler
// =======================================
CopyPasteHandler::CopyPasteHandler(EditorLevel& level, ActionManager& action_manager,
                                   MessagesManager& notifications)
    : p_level_(&level)
    , p_action_manager_(&action_manager)
    , p_messages_manager_(&notifications)
{
}

void CopyPasteHandler::copy_selection(const Selection& selection, int current_floor)
{
    copied_objects_.clear();
    copied_objects_floors_.clear();

    auto [objects, floors] = p_level_->copy_objects_and_floors(selection.objects);

    copied_objects_ = std::move(objects);
    copied_objects_floors_ = std::move(floors);
    copy_start_floor_ = current_floor;

    p_messages_manager_->add_message("Copied " + std::to_string(selection.objects.size()) +
                                     " object(s).");
}

void CopyPasteHandler::paste_selection(int current_floor)
{
    if (copied_objects_floors_.empty())
    {
        return;
    }
    int offset = current_floor - copy_start_floor_;

    // Copy the floors to create an offset if the floor has changed
    auto floors = copied_objects_floors_;
    for (auto& floor : floors)
    {
        floor += offset;
    }

    // When copy/pasting across multiple floors, the floors being pasted to must be ensured to exist
    int max_floor = *std::ranges::max_element(floors);
    int min_floor = *std::ranges::min_element(floors);
    if (max_floor > p_level_->get_max_floor())
    {
        for (int i = p_level_->get_max_floor(); i <= max_floor; i++)
        {
            p_level_->ensure_floor_exists(i);
        }
    }
    else if (min_floor < p_level_->get_min_floor())
    {
        for (int i = p_level_->get_min_floor(); i >= min_floor; --i)
        {
            p_level_->ensure_floor_exists(i);
        }
    }

    p_action_manager_->push_action(std::make_unique<AddBulkObjectsAction>(copied_objects_, floors),
                                   true);

    p_messages_manager_->add_message("Pasted " + std::to_string(copied_objects_.size()) +
                                     " object(s).");
}