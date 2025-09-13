#include "CommandLine.hpp"
#include <iostream>

bool parse_arguments(int argc, char **argv, std::string &scene_path)
{
        if (argc != 2)
        {
                std::cerr << "Usage: minirt <scene.rt>\n";
                return false;
        }
        scene_path = argv[1];
        return true;
}
