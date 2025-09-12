#pragma once
#include "AMenu.hpp"
#include <map>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
    std::multimap<double, std::string, std::greater<double>> scores;

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

    // Run loop for the leaderboard menu (hides AMenu::run)
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
