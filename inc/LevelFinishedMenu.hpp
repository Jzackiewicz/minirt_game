#pragma once
#include "AMenu.hpp"

// Menu displayed when the player completes the current level.
class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu();

    // Show the menu using the provided window and renderer.
    // Returns the action selected by the player.
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
