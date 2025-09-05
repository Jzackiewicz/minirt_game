#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
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
	bool play;
	play = MainMenu::show(width, height);
	if (!play)
	{
		return 0;
	}
	run_application(scene_path, width, height, quality);
	return 0;
}
