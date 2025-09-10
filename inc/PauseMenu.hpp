#pragma once

#include <cstdint>

/**
 * Displays the game's pause menu.
 *
 * @param width Window width.
 * @param height Window height.
 * @return True if the user chose to resume the game. False if they quit.
 */
class PauseMenu
{
public:
        static bool show(int width, int height);
};

