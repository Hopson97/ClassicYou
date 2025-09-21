#pragma once

#include <filesystem>
#include <iostream>
#include <string_view>
#include <deque>
#include <vector>

#include <glm/glm.hpp>


template <typename T, int Size>
struct CircularQueue
{
    void push_back(const T& new_data)
    {
        data.push_back(new_data);
        if (data.size() > Size)
        {
            data.pop_front();
        }
    }

    std::deque<T> data;
};


using epoch_t = long long;

[[nodiscard]] std::string read_file_to_string(const std::filesystem::path& file_path);
[[nodiscard]] std::vector<std::string> split_string(const std::string& string, char delim = ' ');

template <typename Range, typename T>
bool contains(const Range& r, const T& value)
{
    return std::ranges::find(r, value) != std::ranges::end(r);
}

constexpr inline auto rgb_to_normalised(const glm::vec3& rgb)
{
    return glm::vec4(rgb / 255.0f, 1.0f);
}

epoch_t get_epoch();
std::string epoch_to_datetime_string(epoch_t epoch);