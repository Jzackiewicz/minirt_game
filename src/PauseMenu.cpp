#include "PauseMenu.hpp"
#include "LeaderboardMenu.hpp"
#include "SettingsMenu.hpp"
#include "Settings.hpp"

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, SDL_Color{0, 255, 0, 255}});
    buttons.push_back(Button{"LEADERBOARD", ButtonAction::Leaderboard, SDL_Color{0, 0, 255, 255}});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, SDL_Color{255, 255, 0, 255}});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, SDL_Color{255, 0, 0, 255}});
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                     int height) {
    PauseMenu menu;
    bool running = true;
    while (running) {
        ButtonAction action = menu.run(window, renderer, width, height);
        if (action == ButtonAction::Resume) {
            return true;
        } else if (action == ButtonAction::Quit || action == ButtonAction::Back) {
            return false;
        } else if (action == ButtonAction::Settings) {
            SettingsMenu::show(window, renderer, width, height);
            width = g_settings.width;
            height = g_settings.height;
            SDL_SetWindowSize(window, width, height);
        } else if (action == ButtonAction::Leaderboard) {
            LeaderboardMenu::show(window, renderer, width, height);
        }
    }
    return false;
}
