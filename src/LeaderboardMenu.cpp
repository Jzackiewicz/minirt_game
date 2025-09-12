#include "LeaderboardMenu.hpp"
#include "CustomCharacter.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>

// Simple trim helper
static std::string trim(const std::string &s) {
    const char *ws = " \t\n\r";
    size_t start = s.find_first_not_of(ws);
    size_t end = s.find_last_not_of(ws);
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

static std::multimap<double, std::string, std::greater<double>>
load_scores(const std::string &filename) {
    std::multimap<double, std::string, std::greater<double>> result;
    std::ifstream file(filename);
    if (!file)
        return result;
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        double score = std::strtod(value.c_str(), nullptr);
        result.emplace(score, name);
    }
    return result;
}

LeaderboardMenu::LeaderboardMenu() : AMenu("LEADERBOARD"), scores(load_scores("leaderboard.yaml")) {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void LeaderboardMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                           bool transparent) {
    LeaderboardMenu menu;
    menu.run(window, renderer, width, height, transparent);
}

static SDL_Color rank_color(int index) {
    if (index == 1)
        return SDL_Color{255, 215, 0, 255};
    if (index == 2)
        return SDL_Color{192, 192, 192, 255};
    if (index == 3)
        return SDL_Color{205, 127, 50, 255};
    return SDL_Color{255, 255, 255, 255};
}

ButtonAction LeaderboardMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width,
                                  int height, bool transparent) {
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    SDL_Texture *background = nullptr;
    if (transparent) {
        SDL_Surface *surface =
            SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
        if (surface) {
            if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32,
                                     surface->pixels, surface->pitch) == 0) {
                background = SDL_CreateTextureFromSurface(renderer, surface);
            }
            SDL_FreeSurface(surface);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    const int name_len = 21; // fixed length for name column

    // Pre-format records for easier drawing
    std::vector<std::pair<std::string, double>> records;
    for (const auto &p : scores)
        records.emplace_back(p.second, p.first);

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_height = 7 * title_scale;
        int title_gap = static_cast<int>(40 * scale_factor);
        int line_gap = static_cast<int>(5 * scale_factor);
        if (line_gap < 1)
            line_gap = 1;
        int line_height = 7 * scale;

        int button_width = static_cast<int>(300 * scale_factor);
        int button_height = static_cast<int>(80 * scale_factor);
        int button_gap = static_cast<int>(20 * scale_factor);

        int total_lines_height = 10 * line_height + 9 * line_gap;
        int total_height = title_height + title_gap + total_lines_height + button_gap + button_height;
        int top_margin = (height - total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;
        int lines_start_y = title_y + title_height + title_gap;

        // Precompute max line width for centering
        auto make_line = [&](int idx) {
            std::ostringstream oss;
            std::string name;
            double score = 0.0;
            if (idx <= static_cast<int>(records.size())) {
                name = records[idx - 1].first;
                score = records[idx - 1].second;
                if (static_cast<int>(name.size()) > name_len)
                    name = name.substr(0, name_len - 3) + "...";
            } else {
                name.assign(name_len, '.');
            }
            if (idx <= static_cast<int>(records.size())) {
                if (static_cast<int>(name.size()) < name_len)
                    name += std::string(name_len - static_cast<int>(name.size()), ' ');
                oss << idx << '.' << name << ' ' << std::fixed << std::setprecision(1) << score;
            } else {
                oss << idx << '.' << name << ' ' << "....";
            }
            return oss.str();
        };

        int max_line_width = 0;
        for (int i = 1; i <= 10; ++i) {
            std::string line = make_line(i);
            int w = CustomCharacter::text_width(line, scale);
            if (w > max_line_width)
                max_line_width = w;
        }
        int lines_x = width / 2 - max_line_width / 2;

        buttons[0].rect = {width / 2 - button_width / 2,
                           lines_start_y + total_lines_height + button_gap,
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
                const SDL_Rect &r = buttons[0].rect;
                if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                    running = false;
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        if (transparent && background) {
            SDL_RenderCopy(renderer, background, nullptr, nullptr);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 153);
            SDL_Rect overlay{0, 0, width, height};
            SDL_RenderFillRect(renderer, &overlay);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
        }

        // Draw title
        CustomCharacter::draw_text(renderer, title, title_x, title_y, white, title_scale);

        // Draw lines
        for (int i = 1; i <= 10; ++i) {
            std::string line = make_line(i);
            int y = lines_start_y + (i - 1) * (line_height + line_gap);
            std::string idx = std::to_string(i) + '.';
            int idx_width = CustomCharacter::text_width(idx, scale);
            SDL_Color c = rank_color(i);
            CustomCharacter::draw_text(renderer, idx, lines_x, y, c, scale);
            CustomCharacter::draw_text(renderer, line.substr(idx.size()), lines_x + idx_width, y, white, scale);
        }

        // Draw back button
        bool hover = mx >= buttons[0].rect.x && mx < buttons[0].rect.x + buttons[0].rect.w &&
                     my >= buttons[0].rect.y && my < buttons[0].rect.y + buttons[0].rect.h;
        SDL_Color fill = hover ? buttons[0].hover_color : SDL_Color{0, 0, 0, 255};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &buttons[0].rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &buttons[0].rect);
        int text_x = buttons[0].rect.x + (buttons[0].rect.w - CustomCharacter::text_width(buttons[0].text, scale)) / 2;
        int text_y = buttons[0].rect.y + (buttons[0].rect.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[0].text, text_x, text_y, white, scale);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (background)
        SDL_DestroyTexture(background);
    return result;
}
