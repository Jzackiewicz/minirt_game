#include "PauseMenu.hpp"
#include "SettingsMenu.hpp"
#include "LeaderboardMenu.hpp"

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, SDL_Color{0, 255, 0, 255}});
    buttons.push_back(Button{"LEADERBOARD", ButtonAction::Leaderboard, SDL_Color{0, 0, 255, 255}});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, SDL_Color{255, 255, 0, 255}});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, SDL_Color{255, 0, 0, 255}});
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    bool resume = false;
    bool open = true;
    while (open) {
        ButtonAction action = menu.run(window, renderer, width, height);
        if (action == ButtonAction::Resume) {
            resume = true;
            open = false;
        } else if (action == ButtonAction::Quit) {
            resume = false;
            open = false;
        } else if (action == ButtonAction::Settings) {
            SettingsMenu::show(window, renderer, width, height);
            SDL_GetWindowSize(window, &width, &height);
        } else if (action == ButtonAction::Leaderboard) {
            LeaderboardMenu::show(window, renderer, width, height);
        } else if (action == ButtonAction::Back) {
            open = false;
        }
    }
    return resume;
}
