#include "CommandLine.hpp"
#include <cstdlib>
#include <iostream>

bool parse_arguments(int argc, char **argv, std::string &scene_path, int &width,
					 int &height, char &quality)
{
	if (argc < 2)
	{
		std::cerr << "Usage: minirt <scene.rt> [width height L|M|H]\n";
		return false;
	}
	scene_path = argv[1];
	quality = 'H';
	if (argc > 2)
	{
		std::string last;
		last = argv[argc - 1];
		if (last.size() == 1 && (last == "L" || last == "M" || last == "H" ||
								 last == "l" || last == "m" || last == "h"))
		{
			quality = last[0];
			--argc;
		}
	}
	if (argc > 2)
	{
		width = std::atoi(argv[2]);
	}
	else
	{
		width = 800;
	}
	if (argc > 3)
	{
		height = std::atoi(argv[3]);
	}
	else
	{
		height = 600;
	}
	return true;
}
