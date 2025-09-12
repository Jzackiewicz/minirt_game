#include "LeaderboardMenu.hpp"
#include "CustomCharacter.hpp"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

static std::string trim(const std::string &s) {
    const char *ws = " \t\n\r";
    size_t start = s.find_first_not_of(ws);
    size_t end = s.find_last_not_of(ws);
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});

    std::ifstream file("leaderboard.yaml");
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        double score = std::strtod(value.c_str(), nullptr);
        records.emplace(score, name);
    }
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                           int height) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height);
}

ButtonAction LeaderboardMenu::run(SDL_Window *window, SDL_Renderer *renderer,
                                  int width, int height) {
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    const int NAME_LEN = 21;

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_height = 7 * title_scale;
        int title_gap = static_cast<int>(40 * scale_factor);
        int line_height = 7 * scale;
        int line_gap = static_cast<int>(5 * scale_factor);
        int list_height = 10 * line_height + 9 * line_gap;
        int button_width = static_cast<int>(150 * scale_factor);
        int button_height = static_cast<int>(80 * scale_factor);
        int total_height =
            title_height + title_gap + list_height + title_gap + button_height;
        int top_margin = (height - total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int list_start_y = title_y + title_height + title_gap;

        buttons[0].rect = {width / 2 - button_width / 2,
                           list_start_y + list_height + title_gap,
                           button_width, button_height};

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                const SDL_Rect &rect = buttons[0].rect;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y &&
                    my < rect.y + rect.h) {
                    running = false;
                    result = ButtonAction::Back;
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        CustomCharacter::draw_text(renderer, title, title_x, title_y, white,
                                   title_scale);

        auto it = records.begin();
        for (int i = 0; i < 10; ++i) {
            int y = list_start_y + i * (line_height + line_gap);
            std::string num = std::to_string(i + 1) + ".";
            SDL_Color num_color{255, 255, 255, 255};
            if (i == 0)
                num_color = SDL_Color{255, 215, 0, 255};
            else if (i == 1)
                num_color = SDL_Color{192, 192, 192, 255};
            else if (i == 2)
                num_color = SDL_Color{205, 127, 50, 255};

            std::string name_score;
            if (it != records.end()) {
                std::string name = it->second;
                double sc = it->first;
                ++it;
                if (static_cast<int>(name.size()) > NAME_LEN) {
                    if (NAME_LEN > 3)
                        name = name.substr(0, NAME_LEN - 3) + "...";
                    else
                        name = name.substr(0, NAME_LEN);
                } else {
                    name.append(NAME_LEN - name.size(), ' ');
                }
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(1) << sc;
                name_score = name + " " + ss.str();
            } else {
                name_score = std::string(NAME_LEN, '.') + " " + "....";
            }

            int line_width =
                CustomCharacter::text_width(num + name_score, scale);
            int x = width / 2 - line_width / 2;
            CustomCharacter::draw_text(renderer, num, x, y, num_color, scale);
            CustomCharacter::draw_text(renderer, name_score,
                                       x + CustomCharacter::text_width(num, scale),
                                       y, white, scale);
        }

        const SDL_Rect &rect = buttons[0].rect;
        bool hover = mx >= rect.x && mx < rect.x + rect.w && my >= rect.y &&
                     my < rect.y + rect.h;
        SDL_Color fill =
            hover ? buttons[0].hover_color : SDL_Color{0, 0, 0, 255};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
        int text_x = rect.x +
                     (rect.w - CustomCharacter::text_width(buttons[0].text, scale)) /
                         2;
        int text_y = rect.y + (rect.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[0].text, text_x, text_y,
                                   white, scale);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}

