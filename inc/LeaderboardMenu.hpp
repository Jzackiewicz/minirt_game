#pragma once
#include "AMenu.hpp"
#include <utility>
#include <vector>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
    std::vector<std::pair<std::string, double>> records;

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

protected:
    int extra_height(int scale) const override;
    void draw_extra(SDL_Renderer *renderer, int width, int y, int scale) override;
};
