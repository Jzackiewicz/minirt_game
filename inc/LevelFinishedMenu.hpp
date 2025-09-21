#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu displayed when the level quota has been completed.
class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu();
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width,
                             int height, bool transparent = true);
};
