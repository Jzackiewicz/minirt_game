#include "LevelFinishedMenu.hpp"

#include "LeaderboardMenu.hpp"
#include "Settings.hpp"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

constexpr int kMaxNameLength = 24;

std::string format_score(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    return oss.str();
}

std::string trim_name_to_fit(const std::string &name, int max_width, int scale) {
    if (CustomCharacter::text_width(name, scale) <= max_width)
        return name;

    std::string trimmed = name;
    while (!trimmed.empty() &&
           CustomCharacter::text_width(trimmed, scale) > max_width) {
        trimmed.erase(trimmed.begin());
    }
    return trimmed;
}

} // namespace

LevelFinishedMenu::LevelFinishedMenu(const LevelFinishedMenuConfig &config)
    : AMenu(""), cfg(config) {
    int level_index = std::max(1, cfg.current_level);
    int total_levels = std::max(1, cfg.total_levels);
    title = "LEVEL " + std::to_string(level_index) + "/" + std::to_string(total_levels) +
            " FINISHED";
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
}

ButtonAction LevelFinishedMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                                     int height, const LevelFinishedMenuConfig &config,
                                     bool transparent) {
    LevelFinishedMenu menu(config);
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
            if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32,
                                     surface->pixels, surface->pitch) == 0) {
                background = SDL_CreateTextureFromSurface(renderer, surface);
            }
            SDL_FreeSurface(surface);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    SDL_StartTextInput();

    Button next_button{"NEXT LEVEL", ButtonAction::NextLevel,
                       SDL_Color{96, 255, 128, 255}};
    Button quit_button{"QUIT", ButtonAction::Quit, SDL_Color{255, 96, 96, 255}};
    Button leaderboard_button{"LEADERBOARD", ButtonAction::Leaderboard,
                              SDL_Color{96, 128, 255, 255}};
    Button submit_button{"SUBMIT", ButtonAction::None, SDL_Color{255, 220, 96, 255}};

    SDL_Rect input_rect{0, 0, 0, 0};
    SDL_Rect submit_rect{0, 0, 0, 0};

    auto stop_input = [&]() {
        SDL_StopTextInput();
        if (background)
            SDL_DestroyTexture(background);
    };

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int line_scale = scale;
        int title_height = 7 * title_scale;
        int text_height = 7 * line_scale;
        int small_gap = static_cast<int>(10 * scale_factor);
        int line_gap = static_cast<int>(20 * scale_factor);
        int block_gap = static_cast<int>(40 * scale_factor);
        if (block_gap < 20)
            block_gap = 20;

        bool has_next_level = cfg.current_level < cfg.total_levels;
        double general_total = cfg.previous_total_score + cfg.score;

        int info_lines = 1 + (has_next_level ? 0 : 1);
        int info_height = info_lines * text_height + (info_lines - 1) * line_gap;
        int input_height = static_cast<int>(70 * scale_factor);
        if (input_height < 50)
            input_height = 50;
        int bottom_button_height = static_cast<int>(80 * scale_factor);
        if (bottom_button_height < 60)
            bottom_button_height = 60;
        int bottom_button_width = static_cast<int>(220 * scale_factor);
        if (bottom_button_width < 160)
            bottom_button_width = 160;
        int bottom_gap = static_cast<int>(20 * scale_factor);
        if (bottom_gap < 12)
            bottom_gap = 12;

        int content_height = title_height + line_gap + info_height + block_gap + input_height +
                             block_gap + bottom_button_height;
        int top_margin = (height - content_height) / 3;
        if (top_margin < static_cast<int>(40 * scale_factor))
            top_margin = static_cast<int>(40 * scale_factor);

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int current_y = top_margin;

        int score_y = current_y + title_height + line_gap;
        int general_y = score_y + text_height + line_gap;

        int input_y = score_y + info_height + block_gap;
        int horizontal_margin = static_cast<int>(width * 0.1f);
        int available_width = width - 2 * horizontal_margin;
        if (available_width < 0) {
            horizontal_margin = 10;
            available_width = width - 2 * horizontal_margin;
        }
        int min_submit_width = std::max(static_cast<int>(140 * scale_factor), 120);
        int min_input_width = std::max(static_cast<int>(300 * scale_factor), 220);
        if (available_width < min_submit_width + min_input_width + small_gap) {
            horizontal_margin = std::max(10, (width - (min_submit_width + min_input_width + small_gap)) / 2);
            available_width = width - 2 * horizontal_margin;
        }
        if (available_width < min_submit_width + min_input_width + small_gap)
            available_width = min_submit_width + min_input_width + small_gap;
        int input_width = available_width - min_submit_width - small_gap;
        if (input_width < min_input_width)
            input_width = min_input_width;
        if (input_width + min_submit_width + small_gap > available_width) {
            input_width = available_width - min_submit_width - small_gap;
        }
        if (input_width < 100)
            input_width = std::max(100, available_width - min_submit_width - small_gap);
        int submit_width = available_width - input_width - small_gap;
        if (submit_width < min_submit_width) {
            submit_width = min_submit_width;
            input_width = available_width - submit_width - small_gap;
        }
        if (input_width < 100)
            input_width = 100;
        if (submit_width < 100)
            submit_width = 100;
        input_rect = {horizontal_margin, input_y, input_width, input_height};
        submit_rect = {input_rect.x + input_rect.w + small_gap, input_y, submit_width,
                       input_height};

        int bottom_y = input_y + input_height + block_gap;
        std::vector<Button *> bottom_buttons;
        if (has_next_level)
            bottom_buttons.push_back(&next_button);
        bottom_buttons.push_back(&leaderboard_button);
        bottom_buttons.push_back(&quit_button);
        int button_count = static_cast<int>(bottom_buttons.size());
        int bottom_total_width = button_count * bottom_button_width +
                                 (button_count - 1) * bottom_gap;
        int bottom_start_x = width / 2 - bottom_total_width / 2;
        for (int i = 0; i < button_count; ++i) {
            bottom_buttons[i]->rect = {bottom_start_x + i * (bottom_button_width + bottom_gap),
                                       bottom_y, bottom_button_width, bottom_button_height};
        }
        submit_button.rect = submit_rect;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    if (!name_input.empty())
                        name_input.pop_back();
                }
            } else if (event.type == SDL_TEXTINPUT) {
                if (input_active) {
                    for (char c : std::string(event.text.text)) {
                        if (std::isprint(static_cast<unsigned char>(c)) &&
                            static_cast<int>(name_input.size()) < kMaxNameLength) {
                            name_input.push_back(c);
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                if (mx >= input_rect.x && mx < input_rect.x + input_rect.w && my >= input_rect.y &&
                    my < input_rect.y + input_rect.h) {
                    input_active = true;
                }

                bool submit_enabled = !name_input.empty();
                if (mx >= submit_rect.x && mx < submit_rect.x + submit_rect.w &&
                    my >= submit_rect.y && my < submit_rect.y + submit_rect.h &&
                    submit_enabled) {
                    // Submission functionality will be implemented later.
                }

                for (Button *btn : bottom_buttons) {
                    const SDL_Rect &r = btn->rect;
                    if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                        if (btn->action == ButtonAction::Leaderboard) {
                            if (transparent && background) {
                                SDL_RenderCopy(renderer, background, nullptr, nullptr);
                                SDL_RenderPresent(renderer);
                            } else {
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                                SDL_RenderClear(renderer);
                                SDL_RenderPresent(renderer);
                            }
                            LeaderboardMenu::show(window, renderer, width, height, transparent);
                        } else {
                            result = btn->action;
                            running = false;
                        }
                        break;
                    }
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);
        bool submit_enabled = !name_input.empty();

        if (transparent && background) {
            SDL_RenderCopy(renderer, background, nullptr, nullptr);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 153);
            SDL_Rect overlay{0, 0, width, height};
            SDL_RenderFillRect(renderer, &overlay);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
        }

        CustomCharacter::draw_text(renderer, title, title_x, current_y, white, title_scale);

        std::string score_text = "YOUR SCORE: " + format_score(cfg.score) + "/" +
                                 format_score(cfg.required_score);
        int score_x = width / 2 - CustomCharacter::text_width(score_text, line_scale) / 2;
        CustomCharacter::draw_text(renderer, score_text, score_x, score_y, white, line_scale);

        if (!has_next_level) {
            std::string general_text = "GENERAL SCORE: " + format_score(general_total);
            int general_x = width / 2 - CustomCharacter::text_width(general_text, line_scale) / 2;
            CustomCharacter::draw_text(renderer, general_text, general_x, general_y, white,
                                       line_scale);
        }

        SDL_Color input_bg{20, 20, 20, 255};
        SDL_SetRenderDrawColor(renderer, input_bg.r, input_bg.g, input_bg.b, input_bg.a);
        SDL_RenderFillRect(renderer, &input_rect);
        SDL_Color border_color = input_active ? SDL_Color{96, 128, 255, 255}
                                              : SDL_Color{255, 255, 255, 255};
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b,
                               border_color.a);
        SDL_RenderDrawRect(renderer, &input_rect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &submit_rect);
        SDL_Color submit_color = submit_enabled ? submit_button.hover_color
                                                : SDL_Color{96, 96, 96, 255};
        bool submit_hover = mx >= submit_rect.x && mx < submit_rect.x + submit_rect.w &&
                            my >= submit_rect.y && my < submit_rect.y + submit_rect.h &&
                            submit_enabled;
        SDL_Color submit_fill = submit_hover ? submit_color : SDL_Color{0, 0, 0, 255};
        SDL_SetRenderDrawColor(renderer, submit_fill.r, submit_fill.g, submit_fill.b,
                               submit_fill.a);
        SDL_RenderFillRect(renderer, &submit_rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &submit_rect);
        int submit_text_x = submit_rect.x +
                            (submit_rect.w - CustomCharacter::text_width(submit_button.text, line_scale)) /
                                2;
        int submit_text_y = submit_rect.y + (submit_rect.h - 7 * line_scale) / 2;
        CustomCharacter::draw_text(renderer, submit_button.text, submit_text_x, submit_text_y,
                                   white, line_scale);

        std::string display_name = name_input.empty()
                                       ? std::string("YOUR NAME...")
                                       : trim_name_to_fit(name_input, input_rect.w - 10, line_scale);
        SDL_Color name_color = name_input.empty() ? SDL_Color{128, 128, 128, 255} : white;
        int name_x = input_rect.x + 5;
        int name_y = input_rect.y + (input_rect.h - 7 * line_scale) / 2;
        CustomCharacter::draw_text(renderer, display_name, name_x, name_y, name_color, line_scale);

        for (Button *btn : bottom_buttons) {
            bool hover = mx >= btn->rect.x && mx < btn->rect.x + btn->rect.w &&
                         my >= btn->rect.y && my < btn->rect.y + btn->rect.h;
            SDL_Color fill = hover ? btn->hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn->rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn->rect);
            int text_x = btn->rect.x +
                         (btn->rect.w - CustomCharacter::text_width(btn->text, line_scale)) / 2;
            int text_y = btn->rect.y + (btn->rect.h - 7 * line_scale) / 2;
            CustomCharacter::draw_text(renderer, btn->text, text_x, text_y, white, line_scale);
        }

        if (g_developer_mode) {
            SDL_Color red{255, 0, 0, 255};
            std::string text = "DEVELOPER MODE";
            int tw = CustomCharacter::text_width(text, line_scale);
            CustomCharacter::draw_text(renderer, text, width - tw - 5, 5, red, line_scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    stop_input();
    return result;
}

