#pragma once

#include <string>

struct SessionProgress;

/**
 * Loads scene, prepares renderer and displays window.
 *
 * @param scene_path Path to the scene file.
 * @param width Desired window width.
 * @param height Desired window height.
 * @param quality Render quality (L/M/H).
 * @param progress Persistent data shared between sessions.
 */
bool run_application(const std::string &scene_path, int width, int height,
                                        char quality, bool tutorial_mode,
                                        SessionProgress &progress);
