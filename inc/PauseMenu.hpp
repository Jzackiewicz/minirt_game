#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu shown when the game is paused
class PauseMenu : public AMenu {
public:
    PauseMenu();
    static bool show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
