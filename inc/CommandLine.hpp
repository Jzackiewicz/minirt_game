#pragma once

#include <string>

/**
 * Parses command line arguments and determines the starting scene.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param scene_path Output path to scene file.
 * @param skip_main_menu Set to true when a scene is provided on the command line.
 * @return True on success.
 */
bool parse_arguments(int argc, char **argv, std::string &scene_path, bool &skip_main_menu);
