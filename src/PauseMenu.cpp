#include "PauseMenu.hpp"

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelYellow});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
    corner_buttons.push_back(
        Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelGray});
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    ButtonAction action = menu.run(window, renderer, width, height, true);
    return action == ButtonAction::Resume;
}
