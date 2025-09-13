#pragma once

#include <string>

/**
 * Parses command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param scene_path Output path to scene file.
 * @return True on success.
 */
bool parse_arguments(int argc, char **argv, std::string &scene_path);
