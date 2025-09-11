#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include <string>

/**
 * Program entry point.
 *
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
        Settings file_settings;
        load_settings(file_settings);
        width = file_settings.width;
        height = file_settings.height;
        quality = file_settings.quality;
        bool play;
        play = MainMenu::show(width, height);
        if (!play)
        {
                return 0;
        }
        width = g_settings.width;
        height = g_settings.height;
        quality = g_settings.quality;
        run_application(scene_path, width, height, quality);
        return 0;
}
