#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
#include "Renderer.hpp"
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

SessionProgress g_story_progress;
SessionProgress g_tutorial_progress;

} // namespace

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
        std::string default_scene_path;
        bool skip_main_menu = false;
        if (!parse_arguments(argc, argv, default_scene_path, skip_main_menu))
        {
                return 1;
        }
        load_settings();

        if (skip_main_menu)
        {
                g_story_progress.has_progress = false;
                g_story_progress.cumulative_score = 0.0;
                g_story_progress.player_name.clear();
                g_story_progress.completed_levels = 0;
                g_story_progress.tutorial_mode = false;
                g_story_progress.next_scene_path =
                        std::filesystem::absolute(default_scene_path).string();
                run_application(g_story_progress.next_scene_path, g_settings.width,
                                g_settings.height, g_settings.quality, false,
                                g_story_progress);
                load_settings();
                return 0;
        }

        bool keep_running = true;
        while (keep_running)
        {
                int menu_width = g_settings.width;
                int menu_height = g_settings.height;
                ButtonAction action = MainMenu::show(menu_width, menu_height);
                load_settings();
                if (action == ButtonAction::Quit || action == ButtonAction::None)
                {
                        break;
                }
                if (action != ButtonAction::Play && action != ButtonAction::Tutorial)
                {
                        continue;
                }
                bool tutorial_mode = (action == ButtonAction::Tutorial);
                SessionProgress &progress = tutorial_mode ? g_tutorial_progress
                                                          : g_story_progress;
                std::string scene_path;
                if (progress.has_progress && progress.tutorial_mode == tutorial_mode &&
                    !progress.next_scene_path.empty())
                {
                        scene_path = progress.next_scene_path;
                }
                else
                {
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
                        else
                        {
                                scene_path = default_scene_path;
                        }
                        scene_path = std::filesystem::absolute(scene_path).string();
                        progress.has_progress = false;
                        progress.cumulative_score = 0.0;
                        progress.player_name.clear();
                        progress.completed_levels = 0;
                        progress.tutorial_mode = tutorial_mode;
                        progress.next_scene_path = scene_path;
                }
                bool back_to_menu = run_application(scene_path, g_settings.width,
                                                                                g_settings.height,
                                                                                g_settings.quality,
                                                                                tutorial_mode,
                                                                                progress);
                load_settings();
                if (!back_to_menu)
                {
                        keep_running = false;
                }
        }
        return 0;
}
