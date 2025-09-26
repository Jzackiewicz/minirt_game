#include "CommandLine.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>

namespace
{

std::string to_lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

} // namespace

bool parse_arguments(int argc, char **argv, std::string &scene_path, bool &skip_main_menu)
{
    namespace fs = std::filesystem;

    scene_path = "scenes/level_1.toml";
    skip_main_menu = false;

    if (argc <= 1)
    {
        return true;
    }

    if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [path/to/scene.toml]\n";
        return false;
    }

    std::string provided_path = argv[1];
    if (provided_path.empty())
    {
        std::cerr << "Error: Scene path cannot be empty.\n";
        return false;
    }

    fs::path scene_file(provided_path);
    std::string extension = to_lower(scene_file.extension().string());
    if (extension != ".toml")
    {
        std::cerr << "Error: Scene file must have a .toml extension.\n";
        return false;
    }

    std::error_code ec;
    if (!fs::exists(scene_file, ec) || !fs::is_regular_file(scene_file, ec))
    {
        std::cerr << "Error: Scene file '" << provided_path << "' was not found.\n";
        return false;
    }

    scene_path = scene_file.string();
    skip_main_menu = true;
    return true;
}
