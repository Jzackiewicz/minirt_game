#include "LeaderboardMenu.hpp"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <vector>
#include <utility>
#include <cstdlib>

static std::string trim(const std::string &s) {
    std::string result = s;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    return result;
}

static std::vector<std::pair<std::string, double>> load_leaderboard(const std::string &path) {
    std::vector<std::pair<std::string, double>> records;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        std::size_t pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        char *endptr = nullptr;
        double score = std::strtod(value.c_str(), &endptr);
        if (endptr != value.c_str())
            records.emplace_back(name, score);
    }
    return records;
}

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height);
}

ButtonAction LeaderboardMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    auto records = load_leaderboard("leaderboard.yaml");
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int button_width = static_cast<int>(300 * scale_factor);
        int button_height = static_cast<int>(100 * scale_factor);
        int button_gap = static_cast<int>(10 * scale_factor);
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_gap = static_cast<int>(80 * scale_factor);
        int record_gap = static_cast<int>(5 * scale_factor);

        int total_buttons_height = static_cast<int>(buttons.size()) * button_height +
                                   (static_cast<int>(buttons.size()) - 1) * button_gap;
        int record_height = 7 * scale;
        int total_records_height = static_cast<int>(records.size()) * record_height +
                                   (static_cast<int>(records.size()) - 1) * record_gap;
        int title_height = 7 * title_scale;
        int top_margin = (height - title_height - title_gap - total_records_height -
                          button_gap - total_buttons_height) /
                         2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int list_start_y = title_y + title_height + title_gap;
        int button_start_y = list_start_y + total_records_height + button_gap;
        int center_x = width / 2 - button_width / 2;
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            buttons[i].rect = {center_x,
                               button_start_y + static_cast<int>(i) *
                                                  (button_height + button_gap),
                               button_width, button_height};
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                for (auto &btn : buttons) {
                    if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                        if (btn.action != ButtonAction::Settings &&
                            btn.action != ButtonAction::Leaderboard) {
                            result = btn.action;
                            running = false;
                        }
                        break;
                    }
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color default_colors[] = {
            {0, 0, 255, 255}, {255, 255, 0, 255}, {0, 255, 0, 255},
            {255, 0, 0, 255}, {0, 255, 255, 255}, {128, 0, 128, 255}};
        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = white;
            if (!title_colors.empty()) {
                c = i < title_colors.size() ? title_colors[i] : title_colors.back();
            } else if (i < sizeof(default_colors) / sizeof(default_colors[0])) {
                c = default_colors[i];
            }
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c,
                                            title_scale);
            tx += (5 + 1) * title_scale;
        }

        int entry_y = list_start_y;
        for (std::size_t i = 0; i < records.size(); ++i) {
            SDL_Color num_color = white;
            if (i == 0)
                num_color = SDL_Color{255, 215, 0, 255};
            else if (i == 1)
                num_color = SDL_Color{192, 192, 192, 255};
            else if (i == 2)
                num_color = SDL_Color{205, 127, 50, 255};

            std::string idx = std::to_string(i + 1) + ". ";
            CustomCharacter::draw_text(renderer, idx, center_x, entry_y, num_color,
                                       scale);

            int name_x = center_x + CustomCharacter::text_width(idx, scale);
            CustomCharacter::draw_text(renderer, records[i].first, name_x, entry_y,
                                       white, scale);

            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << records[i].second;
            std::string score = ss.str();
            int score_x =
                center_x + button_width - CustomCharacter::text_width(score, scale);
            CustomCharacter::draw_text(renderer, score, score_x, entry_y, white,
                                       scale);

            entry_y += record_height + record_gap;
        }

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x = btn.rect.x +
                         (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white,
                                       scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}
