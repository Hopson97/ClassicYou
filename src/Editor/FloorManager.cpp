#include "FloorManager.h"

Floor& FloorManager::ensure_floor_exists(int floor_number)
{
    if (floors.empty())
    {
        auto& floor = floors.emplace_back(floor_number);
        min_floor = floor_number;
        max_floor = floor_number;
        return floor;
    }
    else
    {
        if (floor_number < min_floor)
        {
            return floors.emplace_back(--min_floor);
        }
        else if (floor_number > max_floor)
        {
            return floors.emplace_back(++max_floor);
        }
    }

    auto floor = find_floor(floor_number);
    if (!floor)
    {
        throw std::runtime_error("Trying to add floors failed, but the floor doesn't exist");
    }
    return **floor;
}

std::optional<Floor*> FloorManager::find_floor(int floor_number)
{
    for (auto& floor : floors)
    {
        if (floor.real_floor == floor_number)
        {
            return &floor;
        }
    }
    return {};
}

std::optional<const Floor*> FloorManager::find_floor(int floor_number) const
{
    for (auto& floor : floors)
    {
        if (floor.real_floor == floor_number)
        {
            return &floor;
        }
    }
    return {};
}

void FloorManager::clear()
{
    floors.clear();
    min_floor = 0;
    max_floor = 0;
}

std::optional<nlohmann::json> FloorManager::serialise() const
{
    nlohmann::json output;
    output["version"] = 1;
    output["floors"] = {};

    // Floors are saved from bottom to top
    for (int floor_number = min_floor; floor_number < max_floor + 1; floor_number++)
    {
        auto floor_opt = find_floor(floor_number);
        if (!floor_opt)
        {
            std::println(std::cerr, "Could not save floor {} as it does not exist", floor_number);
            return false;
        }
        auto& floor = **floor_opt;

        // Objects are grouped together by their type to optimize the json
        std::unordered_map<std::string, nlohmann::json> object_map;

        // Create a json object for the current floor
        nlohmann::json current_floor;
        current_floor["floor"] = floor_number;
        current_floor["objects"] = {};

        // Iterate through all objects on the floor and group them by type
        for (auto& object : floor.objects)
        {
            auto [data, type] = object.serialise();
            if (object_map.find(type) == object_map.end())
            {
                object_map[type] = {};
            }
            object_map[type].push_back(data);
        }

        // Add the grouped objects to the current floor json
        current_floor["objects"] = object_map;
        output["floors"].push_back(current_floor);
    }

    return output;
}
