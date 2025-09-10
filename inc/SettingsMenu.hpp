#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu for adjusting settings
class SettingsMenu : public AMenu {
public:
    SettingsMenu();
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
