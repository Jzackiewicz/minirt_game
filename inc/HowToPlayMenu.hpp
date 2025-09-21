#pragma once

#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu displaying how to play information
class HowToPlayMenu : public AMenu {
public:
    HowToPlayMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent = false);
};
