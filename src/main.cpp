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
        if (!parse_arguments(argc, argv, scene_path))
        {
                return 1;
        }
        load_settings();
        int width = g_settings.width;
        int height = g_settings.height;
        bool play = MainMenu::show(width, height);
        if (!play)
        {
                return 0;
        }
        run_application(scene_path, g_settings.width, g_settings.height, g_settings.quality);
        return 0;
}
