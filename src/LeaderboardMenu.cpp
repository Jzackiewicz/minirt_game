#include "LeaderboardMenu.hpp"

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height);
}
