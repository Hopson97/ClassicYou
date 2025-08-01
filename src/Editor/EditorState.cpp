#include "EditorState.h"

#include "LevelObjects/LevelObject.h"
#include "Tool.h"

namespace
{
    template <typename T>
    bool exists(T object, std::vector<T> array)
    {
        for (auto item : array)
        {
            if (item == object)
            {
                return true;
            }
        }
        return false;
    }
} // namespace

void Selection::set_selection(LevelObject* object, int floor)

{
    p_active_object = object;

    objects.clear();
    if (object && !exists(object->object_id, objects))
    {
        objects.push_back(object->object_id);
        object_floors.push_back(floor);
    }
}

void Selection::add_to_selection(LevelObject* object, int floor)
{
    if (objects.empty())
    {
        p_active_object = objects.empty() ? object : nullptr;
    }

    if (object && !exists(object->object_id, objects))
    {
        objects.push_back(object->object_id);
        object_floors.push_back(floor);
    }
}

void Selection::add_to_selection(ObjectId id, int floor)
{
    if (!exists(id, objects))
    {
        objects.push_back(id);
        object_floors.push_back(floor);
    }
}

void Selection::clear_selection()
{

    object_floors.clear();
    objects.clear();
    p_active_object = nullptr;
}

bool Selection::single_object_is_selected() const
{
    return p_active_object && objects.size() == 1;
}

bool Selection::has_selection() const
{
    return p_active_object || !objects.empty();
}
