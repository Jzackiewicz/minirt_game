#include "SettingsMenu.hpp"

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}
