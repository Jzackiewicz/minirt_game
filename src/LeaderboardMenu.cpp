#include "LeaderboardMenu.hpp"
#include <SDL.h>
#include <fstream>
#include <iomanip>
#include <sstream>

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    load_records("leaderboard.yaml");
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void LeaderboardMenu::load_records(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(file, line) && records.size() < 10) {
        if (line.empty())
            continue;
        std::size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string name = line.substr(0, colon);
        std::string score_str = line.substr(colon + 1);
        auto ltrim = [](std::string &s) {
            s.erase(0, s.find_first_not_of(" \t"));
        };
        auto rtrim = [](std::string &s) {
            s.erase(s.find_last_not_of(" \t") + 1);
        };
        ltrim(name);
        rtrim(name);
        ltrim(score_str);
        rtrim(score_str);
        std::stringstream ss(score_str);
        double score;
        if (ss >> score) {
            records.emplace_back(name, score);
        }
    }
}

SDL_Color LeaderboardMenu::color_for_place(std::size_t index) {
    if (index == 0)
        return SDL_Color{255, 215, 0, 255}; // Gold
    if (index == 1)
        return SDL_Color{192, 192, 192, 255}; // Silver
    if (index == 2)
        return SDL_Color{205, 127, 50, 255}; // Bronze
    return SDL_Color{255, 255, 255, 255};
}

std::string LeaderboardMenu::format_score(double score) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << score;
    return oss.str();
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
        int title_gap = static_cast<int>(20 * scale_factor);

        int total_buttons_height = static_cast<int>(buttons.size()) * button_height +
                                   (static_cast<int>(buttons.size()) - 1) * button_gap;
        int title_height = 7 * title_scale;
        int line_height = 7 * scale;
        int line_gap = scale;
        int list_height = static_cast<int>(records.size()) * line_height +
                          (static_cast<int>(records.size()) - 1) * line_gap;

        int total_height = title_height + title_gap + list_height + title_gap +
                           total_buttons_height;
        int top_margin = (height - total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int list_start_y = title_y + title_height + title_gap;
        int start_y = list_start_y + list_height + title_gap;

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

        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = title_colors[i];
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c, title_scale);
            tx += (5 + 1) * title_scale;
        }

        for (std::size_t i = 0; i < records.size() && i < 10; ++i) {
            int line_y = list_start_y + static_cast<int>(i) * (line_height + line_gap);
            std::string index = std::to_string(i + 1) + ".";
            std::string rest = " " + records[i].first + " " + format_score(records[i].second);
            int total_w = CustomCharacter::text_width(index + rest, scale);
            int x = width / 2 - total_w / 2;
            SDL_Color col = color_for_place(i);
            CustomCharacter::draw_text(renderer, index, x, line_y, col, scale);
            int x2 = x + CustomCharacter::text_width(index + " ", scale);
            CustomCharacter::draw_text(renderer, rest.substr(1), x2, line_y, white, scale);
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
