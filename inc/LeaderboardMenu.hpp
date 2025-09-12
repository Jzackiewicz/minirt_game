#pragma once
#include "AMenu.hpp"
#include <map>
#include <functional>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
private:
    // Ordered map of score to player name, sorted by score descending
    std::multimap<double, std::string, std::greater<double>> records;

    // Load leaderboard records from a YAML file
    void load_records(const std::string &path);

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

    // Run the leaderboard menu loop
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
