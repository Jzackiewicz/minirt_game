#pragma once

#include "AMenu.hpp"

// Menu shown when the player finishes the current level.
class LevelFinishedMenu : public AMenu
{
        public:
        LevelFinishedMenu();

        // Display the menu and return the action picked by the player.
        static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer,
                                 int width, int height);
};

