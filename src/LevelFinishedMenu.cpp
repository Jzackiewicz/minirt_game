#include "LevelFinishedMenu.hpp"

LevelFinishedMenu::LevelFinishedMenu() : AMenu("LEVEL FINISHED") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(
        Button{"CONTINUE", ButtonAction::Resume, SDL_Color{96, 255, 128, 255}});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, SDL_Color{0, 128, 255, 255}});
    buttons.push_back(
        Button{"SETTINGS", ButtonAction::Settings, SDL_Color{255, 220, 96, 255}});
    buttons.push_back(
        Button{"QUIT", ButtonAction::Quit, SDL_Color{255, 96, 96, 255}});
}

ButtonAction LevelFinishedMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                                     int height, bool transparent) {
    LevelFinishedMenu menu;
    return menu.run(window, renderer, width, height, transparent);
}
