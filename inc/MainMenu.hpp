#pragma once

#include <cstdint>

/**
 * Displays the game's main menu.
 *
 * @param width Window width.
 * @param height Window height.
 * @return True if the user chose to play. False if they quit.
*/
class MainMenu
{
        public:
        static bool show(int width, int height);
};
