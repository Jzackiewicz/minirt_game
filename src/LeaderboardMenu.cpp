#include "LeaderboardMenu.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

// Trim helper
static std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t");
    std::size_t end = s.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});

    std::ifstream file("leaderboard.yaml");
    std::string line;
    while (std::getline(file, line) && records.size() < 10) {
        std::size_t pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));
        std::istringstream ss(val);
        double score = 0.0;
        ss >> score;
        records.push_back({name, score});
    }
    std::sort(records.begin(), records.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

int LeaderboardMenu::extra_height(int scale) const {
    int line_height = 7 * scale;
    int gap = scale;
    if (records.empty())
        return 0;
    return static_cast<int>(records.size()) * line_height +
           (static_cast<int>(records.size()) - 1) * gap;
}

void LeaderboardMenu::draw_extra(SDL_Renderer *renderer, int width, int y, int scale) {
    SDL_Color number_colors[3] = {
        {255, 215, 0, 255},   // gold
        {192, 192, 192, 255}, // silver
        {205, 127, 50, 255}   // bronze
    };
    int line_height = 7 * scale;
    int gap = scale;
    for (std::size_t i = 0; i < records.size(); ++i) {
        SDL_Color num_color = {255, 255, 255, 255};
        if (i < 3)
            num_color = number_colors[i];

        std::string number = std::to_string(i + 1) + ".";
        std::string name = records[i].first;
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << records[i].second;
        std::string score = os.str();

        int number_width = CustomCharacter::text_width(number, scale);
        int name_width = CustomCharacter::text_width(name, scale);
        int score_width = CustomCharacter::text_width(score, scale);
        int gap1 = scale;
        int gap2 = 4 * scale;
        int total_width = number_width + gap1 + name_width + gap2 + score_width;
        int x = width / 2 - total_width / 2;

        CustomCharacter::draw_text(renderer, number, x, y, num_color, scale);
        x += number_width + gap1;
        CustomCharacter::draw_text(renderer, name, x, y, SDL_Color{255, 255, 255, 255}, scale);
        x += name_width + gap2;
        CustomCharacter::draw_text(renderer, score, x, y, SDL_Color{255, 255, 255, 255}, scale);

        y += line_height + gap;
    }
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height);
}

