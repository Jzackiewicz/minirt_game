#pragma once
#include "AMenu.hpp"
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
private:
    // Vector holding pairs of player name and score
    std::vector<std::pair<std::string, double>> records;

    // Load leaderboard records from a YAML file
    void load_records(const std::string &path);

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

    // Run the leaderboard menu loop
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
