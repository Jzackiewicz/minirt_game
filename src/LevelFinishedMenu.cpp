#include "LevelFinishedMenu.hpp"
#include "LeaderboardMenu.hpp"

#include <SDL.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
std::string make_title(const LevelFinishedStats &stats) {
    std::ostringstream oss;
    oss << "LEVEL " << std::max(0, stats.completed_levels) << "/"
        << std::max(0, stats.total_levels) << " FINISHED";
    return oss.str();
}

std::string format_score(double value) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(2);
    oss << value;
    return oss.str();
}

bool point_in_rect(const SDL_Rect &rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}
} // namespace

LevelFinishedMenu::LevelFinishedMenu(const LevelFinishedStats &stats, std::string &player_name)
    : AMenu(make_title(stats)), stats_(stats), player_name_(player_name) {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
}

ButtonAction LevelFinishedMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                                     int height, const LevelFinishedStats &stats,
                                     std::string &player_name, bool transparent) {
    LevelFinishedMenu menu(stats, player_name);
    return menu.run(window, renderer, width, height, transparent);
}

ButtonAction LevelFinishedMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width,
                                    int height, bool transparent) {
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};
    SDL_Texture *background = nullptr;
    if (transparent) {
        SDL_Surface *surface =
            SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
        if (surface) {
            if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels,
                                     surface->pitch) == 0) {
                background = SDL_CreateTextureFromSurface(renderer, surface);
            }
            SDL_FreeSurface(surface);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    const bool show_name_input = !stats_.has_next_level;
    bool name_field_active = show_name_input;
    const int max_name_length = 20;

    Button next_button{"NEXT LEVEL", ButtonAction::NextLevel, SDL_Color{96, 255, 128, 255}};
    Button leaderboard_button{"LEADERBOARD", ButtonAction::Leaderboard,
                              SDL_Color{96, 128, 255, 255}};
    Button quit_button{"QUIT", ButtonAction::Quit, SDL_Color{255, 96, 96, 255}};
    Button submit_button{"SUBMIT", ButtonAction::None, SDL_Color{200, 200, 200, 255}};

    auto restore_background = [&]() {
        if (transparent && background) {
            SDL_RenderCopy(renderer, background, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }
    };

    if (show_name_input)
        SDL_StartTextInput();
    else
        SDL_StopTextInput();

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int base_scale = std::max(1, static_cast<int>(4 * scale_factor));
        int title_scale = base_scale * 2;
        int text_scale = base_scale;
        int line_gap = std::max(8, static_cast<int>(24 * scale_factor));
        int margin = std::max(10, static_cast<int>(40 * scale_factor));
        int button_width = std::max(160, static_cast<int>(300 * scale_factor));
        int button_height = std::max(60, static_cast<int>(90 * scale_factor));
        int button_gap = std::max(8, static_cast<int>(20 * scale_factor));

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = margin;
        int title_height = 7 * title_scale;

        std::string score_text = "YOUR SCORE: " + format_score(stats_.current_score) + "/" +
                                 format_score(stats_.required_score);
        int score_x = width / 2 - CustomCharacter::text_width(score_text, text_scale) / 2;
        int score_y = title_y + title_height + line_gap;

        std::string general_text;
        int general_x = 0;
        int general_y = 0;
        if (show_name_input) {
            general_text = "GENERAL SCORE: " + format_score(stats_.total_score);
            general_x = width / 2 - CustomCharacter::text_width(general_text, text_scale) / 2;
            general_y = score_y + 7 * text_scale + line_gap;
        }

        int bottom_y = height - button_height - margin;
        int bottom_total_width = button_width * 2 + button_gap;
        int bottom_start_x = width / 2 - bottom_total_width / 2;
        leaderboard_button.rect = {bottom_start_x, bottom_y, button_width, button_height};
        quit_button.rect = {bottom_start_x + button_width + button_gap, bottom_y, button_width,
                            button_height};

        SDL_Rect name_rect{0, 0, 0, 0};
        SDL_Rect submit_rect{0, 0, 0, 0};
        bool submit_enabled = show_name_input && !player_name_.empty();
        if (show_name_input) {
            int input_width = std::max(button_width, std::min(width - 2 * margin, button_width * 2));
            int submit_width = std::max(140, static_cast<int>(220 * scale_factor));
            int name_y = bottom_y - button_height - button_gap;
            int total_input_width = input_width + button_gap + submit_width;
            int input_start_x = width / 2 - total_input_width / 2;
            name_rect = {input_start_x, name_y, input_width, button_height};
            submit_rect = {input_start_x + input_width + button_gap, name_y, submit_width,
                           button_height};
            submit_button.rect = submit_rect;
        } else if (stats_.has_next_level) {
            int next_y = bottom_y - button_height - button_gap;
            next_button.rect = {width / 2 - button_width / 2, next_y, button_width, button_height};
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    running = false;
                    result = ButtonAction::Resume;
                } else if (!stats_.has_next_level &&
                           event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    // Ignore enter on final screen.
                } else if (stats_.has_next_level &&
                           (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                            event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
                    running = false;
                    result = ButtonAction::NextLevel;
                } else if (show_name_input && event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    if (!player_name_.empty()) {
                        player_name_.pop_back();
                        submit_enabled = !player_name_.empty();
                    }
                }
            } else if (event.type == SDL_TEXTINPUT && show_name_input && name_field_active) {
                for (const char *p = event.text.text; *p; ++p) {
                    if (player_name_.size() >= static_cast<size_t>(max_name_length))
                        break;
                    unsigned char ch = static_cast<unsigned char>(*p);
                    if (ch < 32)
                        continue;
                    player_name_.push_back(static_cast<char>(ch));
                }
                submit_enabled = !player_name_.empty();
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                if (stats_.has_next_level && point_in_rect(next_button.rect, mx, my)) {
                    running = false;
                    result = ButtonAction::NextLevel;
                } else if (point_in_rect(leaderboard_button.rect, mx, my)) {
                    bool was_active = SDL_IsTextInputActive();
                    if (was_active)
                        SDL_StopTextInput();
                    restore_background();
                    LeaderboardMenu::show(window, renderer, width, height, transparent);
                    if (was_active)
                        SDL_StartTextInput();
                } else if (point_in_rect(quit_button.rect, mx, my)) {
                    running = false;
                    result = ButtonAction::Quit;
                } else if (show_name_input && point_in_rect(name_rect, mx, my)) {
                    name_field_active = true;
                    if (!SDL_IsTextInputActive())
                        SDL_StartTextInput();
                } else if (show_name_input && point_in_rect(submit_rect, mx, my)) {
                    name_field_active = false;
                    if (SDL_IsTextInputActive())
                        SDL_StopTextInput();
                    // Submit button reserved for future use.
                } else if (show_name_input) {
                    name_field_active = false;
                    if (SDL_IsTextInputActive())
                        SDL_StopTextInput();
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

        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = i < title_colors.size() ? title_colors[i] : white;
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c, title_scale);
            tx += (5 + 1) * title_scale;
        }

        CustomCharacter::draw_text(renderer, score_text, score_x, score_y, white, text_scale);
        if (show_name_input)
            CustomCharacter::draw_text(renderer, general_text, general_x, general_y, white,
                                       text_scale);

        auto draw_button = [&](const Button &btn, bool enabled) {
            SDL_Color base_color = enabled ? SDL_Color{20, 20, 20, 220}
                                           : SDL_Color{60, 60, 60, 220};
            bool hover = point_in_rect(btn.rect, mx, my);
            SDL_Color fill = (enabled && hover) ? btn.hover_color : base_color;
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            SDL_Color text_color = enabled ? white : SDL_Color{180, 180, 180, 255};
            int text_x = btn.rect.x +
                         (btn.rect.w - CustomCharacter::text_width(btn.text, text_scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * text_scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, text_color, text_scale);
        };

        if (stats_.has_next_level)
            draw_button(next_button, true);
        draw_button(leaderboard_button, true);
        draw_button(quit_button, true);

        if (show_name_input) {
            SDL_Color bg_color{20, 20, 20, 220};
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &name_rect);
            SDL_Color border_color = name_field_active ? SDL_Color{96, 255, 128, 255}
                                                       : SDL_Color{255, 255, 255, 255};
            SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b,
                                   border_color.a);
            SDL_RenderDrawRect(renderer, &name_rect);

            std::string display_text = player_name_;
            SDL_Color text_color = player_name_.empty() ? SDL_Color{150, 150, 150, 255} : white;
            if (display_text.empty())
                display_text = "YOUR NAME...";
            int padding = 3 * text_scale;
            int text_x = name_rect.x + padding;
            int text_y = name_rect.y + (name_rect.h - 7 * text_scale) / 2;
            CustomCharacter::draw_text(renderer, display_text, text_x, text_y, text_color,
                                       text_scale);

            draw_button(submit_button, submit_enabled);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (show_name_input && SDL_IsTextInputActive())
        SDL_StopTextInput();
    if (background)
        SDL_DestroyTexture(background);
    return result;
}
