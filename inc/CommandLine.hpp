#pragma once

#include <string>

/**
 * Prepares the default scene path.
 *
 * @param argc Argument count (unused).
 * @param argv Argument values (unused).
 * @param scene_path Output path to scene file.
 * @return True on success.
 */
bool parse_arguments(int argc, char **argv, std::string &scene_path);
