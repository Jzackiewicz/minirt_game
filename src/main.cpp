#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include <string>

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
        std::string scene_path;
        int width;
        int height;
        char quality;
        bool parsed;
        parsed = parse_arguments(argc, argv, scene_path, width, height, quality);
        if (!parsed)
        {
                return 1;
        }
        load_settings();
        width = g_settings.width;
        height = g_settings.height;
        quality = g_settings.quality;
        bool play;
        play = MainMenu::show(width, height);
        if (!play)
        {
                return 0;
        }
        std::string current_scene = scene_path;
        do
        {
                g_reload_requested = false;
                run_application(current_scene, g_settings.width, g_settings.height,
                                g_settings.quality);
                if (g_reload_requested && !g_reload_scene.empty())
                {
                        current_scene = g_reload_scene;
                        g_reload_scene.clear();
                }
        } while (g_reload_requested);
        return 0;
}
