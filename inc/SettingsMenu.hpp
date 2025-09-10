#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu for adjusting settings
class SettingsMenu : public AMenu {
public:
    SettingsMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
