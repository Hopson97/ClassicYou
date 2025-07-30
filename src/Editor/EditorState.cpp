#include "EditorState.h"

#include "LevelObjects/LevelObject.h"

void Selection::set_selection(LevelObject* object)

{
    p_active_object = object;

    objects.clear();
    if (object)
    {
        objects.insert(object->object_id);
    }
}

void Selection::add_to_selection(LevelObject* object)
{
    if (objects.empty())
    {
        p_active_object = objects.empty()  ? object : nullptr;
    }

    if (object)
    {
        objects.insert(object->object_id);
    }
}

void Selection::clear_selection()
{
    objects.clear();
    p_active_object = nullptr;
}

bool Selection::object_is_selected() const
{
    return p_active_object && objects.size() == 1;
}
