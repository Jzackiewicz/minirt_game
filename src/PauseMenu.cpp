#include "PauseMenu.hpp"
#include "LeaderboardMenu.hpp"
#include "SettingsMenu.hpp"
#include <SDL.h>

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, SDL_Color{0, 255, 0, 255}});
    buttons.push_back(Button{"LEADERBOARD", ButtonAction::Leaderboard, SDL_Color{0, 0, 255, 255}});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, SDL_Color{255, 255, 0, 255}});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, SDL_Color{255, 0, 0, 255}});
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    while (true) {
        ButtonAction action = menu.run(window, renderer, width, height);
        if (action == ButtonAction::Resume)
            return true;
        if (action == ButtonAction::Quit)
            return false;
        if (action == ButtonAction::Settings) {
            SettingsMenu::show(window, renderer, width, height);
            SDL_GetWindowSize(window, &width, &height);
        } else if (action == ButtonAction::Leaderboard) {
            LeaderboardMenu::show(window, renderer, width, height);
        }
    }
}
