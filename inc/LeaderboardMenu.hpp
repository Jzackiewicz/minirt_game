#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
