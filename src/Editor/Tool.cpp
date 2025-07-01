#include "Tool.h"

#include "DrawingPad.h"

void CreateWallTool::on_event(sf::Event event, glm::vec2 node)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Left)
        {

            active_dragging_ = true;
            start_ = node;
            end_ = node;
            wall_preview_ = generate_wall_mesh(start_, end_, 0, 2);
            wall_preview_.buffer();
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        end_ = node;
        wall_preview_ = generate_wall_mesh(start_, end_, 0, 2);
        wall_preview_.buffer();
    }
    else if (event.is<sf::Event::MouseButtonReleased>())
    {
        active_dragging_ = false;
        if (glm::length(start_ - end_) > 0.25f)
        {
        }
    }
}

void CreateWallTool::render_preview()
{
    if (active_dragging_)
    {
        wall_preview_.bind().draw_elements();
    }
}

void CreateWallTool::render_preview_2d(DrawingPad& drawing_pad)
{
    if (active_dragging_)
    {
        drawing_pad.render_line(start_, end_, {1, 0, 0, 1}, 4);
    }
}
