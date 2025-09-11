#pragma once
#include "AMenu.hpp"
#include "ButtonsCluster.hpp"
#include "Slider.hpp"
#include "Settings.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu for adjusting settings
class SettingsMenu : public AMenu {
private:
    ButtonsCluster quality_cluster;
    Slider sensitivity_slider;
    Slider resolution_slider;
    Button back_button;
    Button apply_button;

    void run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

public:
    SettingsMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};

