#pragma once

#include <string>

/**
 * Parses command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param scene_path Output path to scene file.
 * @param width Output width.
 * @param height Output height.
 * @param quality Output quality character.
 * @return True on success.
 */
bool parse_arguments(int argc, char **argv, std::string &scene_path, int &width,
					 int &height, char &quality);
