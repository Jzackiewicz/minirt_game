#pragma once
#include "AMenu.hpp"
#include "SettingsSection.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu for adjusting settings
class SettingsMenu : public AMenu {
public:
    SettingsMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

private:
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    QualitySection quality;
    MouseSensitivitySection mouse;
    ResolutionSection resolution;
};
