#pragma once
#include "AMenu.hpp"
#include <map>
#include <utility>
#include <functional>

struct SDL_Window;
struct SDL_Renderer;

// Menu showing the leaderboard
class LeaderboardMenu : public AMenu {
private:
    // Ordered map storing score-name pairs sorted by score in descending order
    std::multimap<double, std::string, std::greater<>> records;

    // Load leaderboard records from a YAML file
    void load_records(const std::string &path);

public:
    LeaderboardMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

    // Run the leaderboard menu loop
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
