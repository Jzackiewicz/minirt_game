#pragma once

#include <string>

/**
 * Loads scene, prepares renderer and displays window.
 *
 * @param scene_path Path to the scene file.
 * @param width Desired window width.
 * @param height Desired window height.
 * @param quality Render quality (L/M/H).
 */
void run_application(const std::string &scene_path, int width, int height, char quality);

