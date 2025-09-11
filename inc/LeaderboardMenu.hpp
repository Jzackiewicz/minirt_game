#pragma once
#include "AMenu.hpp"
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
private:
    std::vector<std::pair<std::string, double>> records;
    void load_records(const std::string &path);
    static SDL_Color color_for_place(std::size_t index);
    static std::string format_score(double score);

public:
    LeaderboardMenu();
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
