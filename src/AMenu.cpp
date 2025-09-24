#include "AMenu.hpp"
#include "HowToPlayMenu.hpp"
#include "LeaderboardMenu.hpp"
#include "SettingsMenu.hpp"
#include <algorithm>
#include "Settings.hpp"

AMenu::AMenu(const std::string &t)
    : title(t), buttons_align_bottom(false), buttons_bottom_margin(-1),
      title_top_margin(-1) {}

int AMenu::button_rows() const { return static_cast<int>(buttons.size()); }

void AMenu::layout_buttons(std::vector<Button> &buttons_list, int width, int height,
                           float scale_factor, int button_width, int button_height,
                           int button_gap, int start_y, int center_x) {
    (void)width;
    (void)height;
    (void)scale_factor;
    for (std::size_t i = 0; i < buttons_list.size(); ++i) {
        buttons_list[i].rect = {center_x,
                                start_y + static_cast<int>(i) * (button_height + button_gap),
                                button_width, button_height};
    }
}

void AMenu::adjust_layout_metrics(float scale_factor, int &button_width, int &button_height,
                                  int &button_gap) {
    (void)scale_factor;
    (void)button_width;
    (void)button_height;
    (void)button_gap;
}

ButtonAction AMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                       bool transparent) {
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

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int button_width = static_cast<int>(300 * scale_factor);
        int button_height = static_cast<int>(100 * scale_factor);
        int button_gap = static_cast<int>(10 * scale_factor);
        adjust_layout_metrics(scale_factor, button_width, button_height, button_gap);
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_gap = static_cast<int>(80 * scale_factor);

        int corner_button_width = static_cast<int>(220 * scale_factor);
        int corner_button_height = static_cast<int>(70 * scale_factor);
        int corner_text_scale = static_cast<int>(3 * scale_factor);
        int corner_margin = static_cast<int>(20 * scale_factor);
        if (corner_button_width < 160)
            corner_button_width = 160;
        if (corner_button_height < 50)
            corner_button_height = 50;
        if (corner_text_scale < 1)
            corner_text_scale = 1;
        if (corner_margin < 10)
            corner_margin = 10;

        int total_buttons_height = 0;
        int rows = button_rows();
        if (rows > 0) {
            total_buttons_height = rows * button_height + (rows - 1) * button_gap;
        }
        int title_height = 7 * title_scale;
        int top_margin = (height - title_height - title_gap - total_buttons_height) / 2;
        if (title_top_margin >= 0) {
            top_margin = static_cast<int>(title_top_margin * scale_factor);
        }
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int center_x = width / 2 - button_width / 2;
        int bottom_margin = top_margin;
        if (buttons_bottom_margin >= 0) {
            bottom_margin = static_cast<int>(buttons_bottom_margin * scale_factor);
        }
        int start_y = title_y + title_height + title_gap;
        if (buttons_align_bottom) {
            start_y = height - bottom_margin - total_buttons_height;
            int min_start = title_y + title_height + title_gap;
            if (start_y < min_start)
                start_y = min_start;
        }
        layout_buttons(buttons, width, height, scale_factor, button_width, button_height,
                       button_gap, start_y, center_x);

        for (std::size_t i = 0; i < corner_buttons.size(); ++i) {
            int offset = static_cast<int>(i) * (corner_button_height + corner_margin);
            corner_buttons[i].rect = {width - corner_button_width - corner_margin,
                                      height - corner_button_height - corner_margin - offset,
                                      corner_button_width, corner_button_height};
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_KEYDOWN &&
                       event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                auto resume_btn = std::find_if(buttons.begin(), buttons.end(),
                                               [](const Button &b) {
                                                   return b.action ==
                                                          ButtonAction::Resume;
                                               });
                if (resume_btn != buttons.end()) {
                    result = ButtonAction::Resume;
                    running = false;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                auto present_background = [&]() {
                    if (transparent && background) {
                        SDL_RenderCopy(renderer, background, nullptr, nullptr);
                        SDL_RenderPresent(renderer);
                    } else {
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderClear(renderer);
                        SDL_RenderPresent(renderer);
                    }
                };

                auto handle_button_click = [&](Button &btn) {
                    if (btn.action == ButtonAction::Settings) {
                        present_background();
                        SettingsMenu::show(window, renderer, width, height, transparent);
                    } else if (btn.action == ButtonAction::Leaderboard) {
                        present_background();
                        LeaderboardMenu::show(window, renderer, width, height, transparent);
                    } else if (btn.action == ButtonAction::HowToPlay) {
                        present_background();
                        HowToPlayMenu::show(window, renderer, width, height, transparent);
                    } else if (btn.action == ButtonAction::Tutorial) {
                        // Tutorial button is a placeholder and does not trigger an action yet.
                    } else {
                        result = btn.action;
                        running = false;
                    }
                };

                bool handled = false;
                for (auto &btn : buttons) {
                    if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                        handle_button_click(btn);
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    for (auto &btn : corner_buttons) {
                        if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                            my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                            handle_button_click(btn);
                            break;
                        }
                    }
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

        SDL_Color default_colors[] = {
            {0, 0, 255, 255},
            {255, 255, 0, 255},
            {0, 255, 0, 255},
            {255, 0, 0, 255},
            {0, 255, 255, 255},
            {128, 0, 128, 255}
        };
        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = white;
            if (!title_colors.empty()) {
                c = i < title_colors.size() ? title_colors[i] : title_colors.back();
            } else if (i < sizeof(default_colors) / sizeof(default_colors[0])) {
                c = default_colors[i];
            }
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c, title_scale);
            tx += (5 + 1) * title_scale;
        }

        draw_content(renderer, width, height, scale, title_scale, title_x, title_y, title_height,
                     title_gap, start_y);

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

        for (auto &btn : corner_buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x = btn.rect.x +
                         (btn.rect.w -
                          CustomCharacter::text_width(btn.text, corner_text_scale)) /
                             2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * corner_text_scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white,
                                       corner_text_scale);
        }
        if (g_developer_mode) {
            SDL_Color red{255, 0, 0, 255};
            std::string text = "DEVELOPER MODE";
            int tw = CustomCharacter::text_width(text, scale);
            CustomCharacter::draw_text(renderer, text, width - tw - 5, 5, red, scale);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (background)
        SDL_DestroyTexture(background);
    return result;
}

void AMenu::draw_content(SDL_Renderer *, int, int, int, int, int, int, int, int, int) {}
