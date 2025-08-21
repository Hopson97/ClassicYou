#include "Util.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>

std::string read_file_to_string(const std::filesystem::path& file_path)
{
    std::ifstream in_file(file_path);

    if (!in_file)
    {
        std::cerr << "Failed to open file " << file_path << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(in_file)),
                        (std::istreambuf_iterator<char>()));

    return content;
}

std::vector<std::string> split_string(const std::string& string, char delim)
{
    std::vector<std::string> tokens;

    std::stringstream stream(string);
    std::string token;
    while (std::getline(stream, token, delim))
    {
        tokens.push_back(token);
    }
    return tokens;
}

epoch_t get_epoch()
{
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

std::string epoch_to_datetime_string(epoch_t epoch)
{
    using namespace std::chrono;
    return std::format("{:%Y-%m-%d %H:%M:%S}",
                       floor<seconds>(system_clock::time_point{seconds{epoch}}));
}
