#include "LeaderboardMenu.hpp"
#include <SDL.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace {
static std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t");
    std::size_t end = s.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}
} // namespace

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    load_records("leaderboard.yaml");
}

void LeaderboardMenu::load_records(const std::string &path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string score_str = trim(line.substr(pos + 1));
        double score = std::strtod(score_str.c_str(), nullptr);
        records.emplace(score, name);
    }
}

ButtonAction LeaderboardMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
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
        int record_height = 7 * scale;
        int record_gap = static_cast<int>(5 * scale_factor);
        std::size_t max_display = std::min<std::size_t>(10, records.size());
        int total_records_height = static_cast<int>(max_display) * record_height +
                                   (static_cast<int>(max_display) - 1) * record_gap;
        int total_buttons_height = static_cast<int>(buttons.size()) * button_height +
                                   (static_cast<int>(buttons.size()) - 1) * button_gap;
        int title_height = 7 * title_scale;
        int top_margin = (height - title_height - title_gap - total_records_height -
                          title_gap - total_buttons_height) /
                         2;
        if (top_margin < 0)
            top_margin = 0;
        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;
        int records_start_y = title_y + title_height + title_gap;
        int start_y = records_start_y + total_records_height + title_gap;
        int center_x = width / 2 - button_width / 2;
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            buttons[i].rect = {center_x,
                               start_y + static_cast<int>(i) * (button_height + button_gap),
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
                        result = btn.action;
                        running = false;
                        break;
                    }
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = white;
            if (!title_colors.empty())
                c = i < title_colors.size() ? title_colors[i] : title_colors.back();
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c, title_scale);
            tx += (5 + 1) * title_scale;
        }

        std::size_t i = 0;
        for (const auto &entry : records) {
            if (i >= 10)
                break;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << entry.first;
            std::string score_str = oss.str();
            std::string idx = std::to_string(i + 1) + ".";
            int idx_width = CustomCharacter::text_width(idx, scale);
            std::string rest = " " + entry.second + " " + score_str;
            int line_width = idx_width + CustomCharacter::text_width(rest, scale);
            int line_x = width / 2 - line_width / 2;
            int line_y = records_start_y + static_cast<int>(i) * (record_height + record_gap);
            SDL_Color idx_color = white;
            if (i == 0)
                idx_color = SDL_Color{255, 215, 0, 255};
            else if (i == 1)
                idx_color = SDL_Color{192, 192, 192, 255};
            else if (i == 2)
                idx_color = SDL_Color{205, 127, 50, 255};
            CustomCharacter::draw_text(renderer, idx, line_x, line_y, idx_color, scale);
            CustomCharacter::draw_text(renderer, rest, line_x + idx_width, line_y, white, scale);
            ++i;
        }

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x =
                btn.rect.x + (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white, scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height);
}
