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
        load_settings();
        int width = g_settings.width;
        int height = g_settings.height;
        char quality = g_settings.quality;
        bool parsed;
        parsed = parse_arguments(argc, argv, scene_path, width, height, quality);
	if (!parsed)
	{
		return 1;
	}
        g_settings.width = width;
        g_settings.height = height;
        g_settings.quality = quality;
        bool play;
        play = MainMenu::show(width, height);
	if (!play)
	{
		return 0;
	}
	run_application(scene_path, width, height, quality);
	return 0;
}
