#pragma once

#include "AMenu.hpp"

class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu();
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                             bool transparent = true);
};
