#include "EditorEventHandlers.h"

#include <imgui.h>

#include "Actions.h"
#include "EditorLevel.h"

ObjectMoveHandler::ObjectMoveHandler(EditorLevel& level, ActionManager& action_manager)
    : p_level_(&level)
    , p_action_manager_(&action_manager)
{
}

bool ObjectMoveHandler::handle_move_events(const sf::Event& event, const EditorState& state)
{

    // When dragging an object and the final placement is decided, this is set to true. This is to
    // prevent calls to the tool event fuctions, which can cause objects to unintentionally placed
    // where the object was moved to.
    bool finish_move = false;
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            bool moving_selection = false;
            for (auto object : p_level_->get_objects(state.selection.objects))
            {
                moving_selection |= object->try_select_2d(state.node_hovered);
            }

            // Start dragging the selected object
            if (moving_selection)
            {
                move_start_tile_ = state.node_hovered;
                moving_object_ = true;

                moving_object_cache_.clear();
                moving_objects_.clear();
                moving_objects_ = p_level_->get_objects(state.selection.objects);
                for (auto object : moving_objects_)
                {
                    moving_object_cache_.push_back(*object);
                }
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        if (state.selection.has_selection() && moving_object_)
        {
            move_offset_ = state.node_hovered - move_start_tile_;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // Handle left button release
        if (mouse->button == sf::Mouse::Button::Left)
        {
            if (moving_object_)
            {
                std::vector<LevelObject> old_objects;
                std::vector<LevelObject> new_objects;
                for (auto object : moving_objects_)
                {
                    old_objects.push_back(*object);
                    new_objects.push_back(*object);
                }
                for (auto& new_object : new_objects)
                {
                    new_object.move(state.node_hovered - move_start_tile_);
                }
                move_offset_ = glm::vec2{0};

                p_action_manager_->push_action(
                    std::make_unique<BulkUpdateObjectAction>(moving_object_cache_, new_objects),
                    true);

                finish_move = true;
            }

            moving_object_ = false;
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
    return moving_object_;
}

void CopyPasteHandler::handle_events(const sf::Event& event, const Selection& selection,
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

CopyPasteHandler::CopyPasteHandler(EditorLevel& level, ActionManager& action_manager)
    : p_level_(&level)
    , p_action_manager_(&action_manager)
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
            std::println("Ensuring floor {} exists (Above)", i);
            p_level_->ensure_floor_exists(i);
        }
    }
    else if (min_floor < p_level_->get_min_floor())
    {
        for (int i = p_level_->get_min_floor(); i >= min_floor; --i)
        {
            std::println("Ensuring floor {} exists (Below)", i);
            p_level_->ensure_floor_exists(i);
        }
    }

    p_action_manager_->push_action(std::make_unique<AddBulkObjectsAction>(copied_objects_, floors),
                                   true);
}