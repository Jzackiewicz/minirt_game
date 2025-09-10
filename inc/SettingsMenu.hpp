#pragma once
#include "AMenu.hpp"
#include "Slider.hpp"
#include "ButtonsCluster.hpp"
#include "Settings.hpp"

struct SDL_Window;
struct SDL_Renderer;

class SettingsMenu : public AMenu {
public:
    SettingsMenu();
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
