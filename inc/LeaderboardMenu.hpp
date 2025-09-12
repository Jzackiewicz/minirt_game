#pragma once
#include "AMenu.hpp"
#include <map>
#include <functional>
#include <string>
#include <utility>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
    std::map<double, std::string, std::greater<double>> records;

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
