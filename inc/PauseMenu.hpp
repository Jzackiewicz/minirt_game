#pragma once

/**
 * Displays the in-game pause menu.
 *
 * @param width Window width.
 * @param height Window height.
 * @return True if the user chose to resume the game. False if they chose to quit.
 */
class PauseMenu
{
public:
        static bool show(int width, int height);
};

