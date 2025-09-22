#include "CommandLine.hpp"

bool parse_arguments(int argc, char **argv, std::string &scene_path)
{
    (void)argc;
    (void)argv;
    scene_path = "scenes/level_1.toml";
    return true;
}
