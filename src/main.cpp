#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace
{

std::optional<std::string> find_first_tutorial_scene()
{
        namespace fs = std::filesystem;
        const std::string prefix = "tutorial_";
        fs::path scenes_dir = "scenes";
        std::error_code ec;
        if (!fs::exists(scenes_dir, ec) || !fs::is_directory(scenes_dir, ec))
                return std::nullopt;
        std::vector<fs::path> tutorials;
        for (auto &entry : fs::directory_iterator(scenes_dir, ec))
        {
                if (ec)
                        break;
                if (!entry.is_regular_file(ec))
                        continue;
                fs::path path = entry.path();
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                });
                if (ext != ".toml")
                        continue;
                std::string stem = path.stem().string();
                std::string lowered;
                lowered.reserve(stem.size());
                std::transform(stem.begin(), stem.end(), std::back_inserter(lowered),
                               [](unsigned char ch) {
                                       return static_cast<char>(std::tolower(ch));
                               });
                if (lowered.rfind(prefix, 0) != 0 || lowered.size() <= prefix.size())
                        continue;
                bool numeric_suffix = true;
                for (std::size_t i = prefix.size(); i < lowered.size(); ++i)
                {
                        if (!std::isdigit(static_cast<unsigned char>(lowered[i])))
                        {
                                numeric_suffix = false;
                                break;
                        }
                }
                if (!numeric_suffix)
                        continue;
                tutorials.push_back(path);
        }
        if (tutorials.empty())
                return std::nullopt;
        auto suffix_value = [&](const fs::path &p) {
                std::string stem = p.stem().string();
                int value = 0;
                for (std::size_t i = prefix.size(); i < stem.size(); ++i)
                {
                        value = value * 10 + (stem[i] - '0');
                }
                return value;
        };
        std::sort(tutorials.begin(), tutorials.end(), [&](const fs::path &a, const fs::path &b) {
                int value_a = suffix_value(a);
                int value_b = suffix_value(b);
                if (value_a != value_b)
                        return value_a < value_b;
                return a.string() < b.string();
        });
        return tutorials.front().string();
}

} // namespace

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
        std::string scene_path;
        if (!parse_arguments(argc, argv, scene_path))
        {
                return 1;
        }
        load_settings();
        int width = g_settings.width;
        int height = g_settings.height;
        ButtonAction action = MainMenu::show(width, height);
        if (action != ButtonAction::Play && action != ButtonAction::Tutorial)
        {
                return 0;
        }
        bool tutorial_mode = (action == ButtonAction::Tutorial);
        if (tutorial_mode)
        {
                auto tutorial_scene = find_first_tutorial_scene();
                if (!tutorial_scene)
                {
                        std::cerr << "No tutorial maps available.\n";
                        return 1;
                }
                scene_path = *tutorial_scene;
        }
        run_application(scene_path, g_settings.width, g_settings.height, g_settings.quality,
                                        tutorial_mode);
        return 0;
}
