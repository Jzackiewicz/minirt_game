#include "LevelFinishedMenu.hpp"
#include "CustomCharacter.hpp"
#include "LeaderboardMenu.hpp"
#include "Settings.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdio>

namespace {

SDL_Texture *capture_background(SDL_Renderer *renderer, int width, int height)
{
        SDL_Surface *surface =
                SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface)
                return nullptr;
        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels,
                                 surface->pitch) != 0)
        {
                SDL_FreeSurface(surface);
                return nullptr;
        }
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
}

void restore_background(SDL_Renderer *renderer, SDL_Texture *background)
{
        SDL_RenderCopy(renderer, background, nullptr, nullptr);
        SDL_RenderPresent(renderer);
}

std::string format_score(double value)
{
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.2f", value);
        return std::string(buffer);
}

} // namespace

LevelFinishedMenu::LevelFinishedMenu() : AMenu("LEVEL FINISHED")
{
        title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
}

ButtonAction LevelFinishedMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width,
                                    int height, const LevelFinishedInfo &info,
                                    bool transparent)
{
        ButtonAction result = ButtonAction::None;
        SDL_Color white{255, 255, 255, 255};
        bool running = true;
        bool name_active = true;
        std::string player_name;
        const std::size_t max_name_length = 24;

        SDL_Texture *background = nullptr;
        if (transparent)
        {
                background = capture_background(renderer, width, height);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }

        SDL_StartTextInput();

        while (running)
        {
                SDL_GetWindowSize(window, &width, &height);
                width = std::max(width, 1);
                height = std::max(height, 1);

                float scale_factor = static_cast<float>(height) / 600.0f;
                int base_scale = std::max(1, static_cast<int>(4 * scale_factor));
                int title_scale = base_scale * 2;
                int text_scale = base_scale;
                int title_height = 7 * title_scale;
                int text_height = 7 * text_scale;
                int margin_top = static_cast<int>(60 * scale_factor);
                int spacing = static_cast<int>(30 * scale_factor);
                int overlay_margin = static_cast<int>(40 * scale_factor);

                bool click_pending = false;
                int click_x = 0;
                int click_y = 0;

                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                        if (event.type == SDL_QUIT)
                        {
                                running = false;
                                result = ButtonAction::Quit;
                        }
                        else if (event.type == SDL_KEYDOWN &&
                                 event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                        {
                                running = false;
                                result = ButtonAction::Quit;
                        }
                        else if (event.type == SDL_KEYDOWN &&
                                 (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                                  event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER))
                        {
                                if (info.has_next_level)
                                {
                                        running = false;
                                        result = ButtonAction::NextLevel;
                                }
                        }
                        else if (event.type == SDL_TEXTINPUT && name_active)
                        {
                                for (char ch : std::string(event.text.text))
                                {
                                        unsigned char uch = static_cast<unsigned char>(ch);
                                        if (uch < 32)
                                                continue;
                                        if (player_name.size() < max_name_length)
                                                player_name.push_back(ch);
                                }
                        }
                        else if (event.type == SDL_KEYDOWN && name_active &&
                                 event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
                        {
                                if (!player_name.empty())
                                        player_name.pop_back();
                        }
                        else if (event.type == SDL_MOUSEBUTTONDOWN &&
                                 event.button.button == SDL_BUTTON_LEFT)
                        {
                                click_pending = true;
                                click_x = event.button.x;
                                click_y = event.button.y;
                        }
                }

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                if (transparent && background)
                {
                        SDL_RenderCopy(renderer, background, nullptr, nullptr);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                        SDL_Rect overlay{0, 0, width, height};
                        SDL_RenderFillRect(renderer, &overlay);
                }
                else
                {
                        SDL_RenderClear(renderer);
                }

                std::string title_text = "LEVEL " + std::to_string(info.level_number) + "/" +
                                         std::to_string(std::max(1, info.total_levels)) +
                                         " FINISHED";
                int title_width = CustomCharacter::text_width(title_text, title_scale);
                int title_x = width / 2 - title_width / 2;
                int title_y = margin_top;
                CustomCharacter::draw_text(renderer, title_text, title_x, title_y, white,
                                           title_scale);

                std::string score_line = "YOUR SCORE: " + format_score(info.current_score) +
                                         "/" + format_score(info.required_score);
                int score_width = CustomCharacter::text_width(score_line, text_scale);
                int score_x = width / 2 - score_width / 2;
                int score_y = title_y + title_height + spacing;
                CustomCharacter::draw_text(renderer, score_line, score_x, score_y, white,
                                           text_scale);

                int current_y = score_y + text_height + spacing;
                bool final_level = !info.has_next_level;
                if (final_level)
                {
                        double total = info.accumulated_score + info.current_score;
                        std::string general_line = "GENERAL SCORE: " + format_score(total);
                        int general_width =
                                CustomCharacter::text_width(general_line, text_scale);
                        int general_x = width / 2 - general_width / 2;
                        CustomCharacter::draw_text(renderer, general_line, general_x,
                                                   current_y, white, text_scale);
                        current_y += text_height + spacing;
                }

                int name_box_width = static_cast<int>(420 * scale_factor);
                int name_box_height = static_cast<int>(80 * scale_factor);
                int submit_width = static_cast<int>(180 * scale_factor);
                int submit_height = name_box_height;
                int input_gap = static_cast<int>(20 * scale_factor);
                int total_input_width = name_box_width + input_gap + submit_width;
                int input_x = width / 2 - total_input_width / 2;
                int name_box_y = current_y;
                SDL_Rect name_rect{input_x, name_box_y, name_box_width, name_box_height};
                SDL_Rect submit_rect{input_x + name_box_width + input_gap, name_box_y,
                                     submit_width, submit_height};

                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                bool name_hover = mouse_x >= name_rect.x &&
                                  mouse_x < name_rect.x + name_rect.w && mouse_y >= name_rect.y &&
                                  mouse_y < name_rect.y + name_rect.h;
                bool submit_enabled = !player_name.empty();
                bool submit_hover = mouse_x >= submit_rect.x &&
                                    mouse_x < submit_rect.x + submit_rect.w &&
                                    mouse_y >= submit_rect.y &&
                                    mouse_y < submit_rect.y + submit_rect.h;

                // Process deferred mouse clicks now that layout is known.
                if (click_pending)
                {
                        if (name_hover)
                        {
                                name_active = true;
                                click_pending = false;
                        }
                        else if (click_x >= submit_rect.x &&
                                 click_x < submit_rect.x + submit_rect.w &&
                                 click_y >= submit_rect.y &&
                                 click_y < submit_rect.y + submit_rect.h)
                        {
                                if (submit_enabled)
                                {
                                        // Placeholder for future submit action
                                }
                                click_pending = false;
                        }
                        else
                        {
                                name_active = false;
                        }
                }

                SDL_Color name_border_color = name_active ? SDL_Color{96, 255, 128, 255}
                                                           : SDL_Color{255, 255, 255, 255};
                SDL_SetRenderDrawColor(renderer, 20, 20, 20, 200);
                SDL_RenderFillRect(renderer, &name_rect);
                SDL_SetRenderDrawColor(renderer, name_border_color.r, name_border_color.g,
                                       name_border_color.b, name_border_color.a);
                SDL_RenderDrawRect(renderer, &name_rect);

                int padding = static_cast<int>(12 * scale_factor);
                int text_x = name_rect.x + padding;
                int text_y = name_rect.y + (name_rect.h - text_height) / 2;
                if (player_name.empty())
                {
                        SDL_Color placeholder{180, 180, 180, 255};
                        CustomCharacter::draw_text(renderer, "YOUR NAME...", text_x, text_y,
                                                   placeholder, text_scale);
                }
                else
                {
                        CustomCharacter::draw_text(renderer, player_name, text_x, text_y,
                                                   white, text_scale);
                }

                SDL_Color submit_color = submit_enabled ? SDL_Color{96, 128, 255, 255}
                                                         : SDL_Color{80, 80, 80, 255};
                SDL_Color submit_border = submit_enabled ? SDL_Color{255, 255, 255, 255}
                                                          : SDL_Color{120, 120, 120, 255};
                if (submit_enabled && submit_hover)
                {
                        submit_color = SDL_Color{120, 160, 255, 255};
                }
                SDL_SetRenderDrawColor(renderer, submit_color.r, submit_color.g,
                                       submit_color.b, submit_color.a);
                SDL_RenderFillRect(renderer, &submit_rect);
                SDL_SetRenderDrawColor(renderer, submit_border.r, submit_border.g,
                                       submit_border.b, submit_border.a);
                SDL_RenderDrawRect(renderer, &submit_rect);
                int submit_text_x = submit_rect.x +
                                    (submit_rect.w -
                                     CustomCharacter::text_width("SUBMIT", text_scale)) /
                                            2;
                int submit_text_y = submit_rect.y + (submit_rect.h - text_height) / 2;
                CustomCharacter::draw_text(renderer, "SUBMIT", submit_text_x, submit_text_y,
                                           white, text_scale);

                std::vector<Button> bottom_buttons;
                if (info.has_next_level)
                {
                        bottom_buttons.push_back(
                                Button{"NEXT LEVEL", ButtonAction::NextLevel,
                                       SDL_Color{96, 255, 128, 255}});
                }
                bottom_buttons.push_back(
                        Button{"LEADERBOARD", ButtonAction::Leaderboard,
                               SDL_Color{96, 128, 255, 255}});
                bottom_buttons.push_back(
                        Button{"QUIT", ButtonAction::Quit, SDL_Color{255, 96, 96, 255}});

                int bottom_button_width = static_cast<int>(220 * scale_factor);
                int bottom_button_height = static_cast<int>(80 * scale_factor);
                int bottom_gap = static_cast<int>(20 * scale_factor);
                int total_bottom_width = static_cast<int>(bottom_buttons.size()) *
                                                 bottom_button_width +
                                         (static_cast<int>(bottom_buttons.size()) - 1) *
                                                 bottom_gap;
                int bottom_y = height - overlay_margin - bottom_button_height;
                int bottom_start_x = width / 2 - total_bottom_width / 2;

                for (std::size_t i = 0; i < bottom_buttons.size(); ++i)
                {
                        bottom_buttons[i].rect =
                                {bottom_start_x + static_cast<int>(i) *
                                                               (bottom_button_width + bottom_gap),
                                 bottom_y, bottom_button_width, bottom_button_height};
                }

                for (auto &btn : bottom_buttons)
                {
                        bool hover = mouse_x >= btn.rect.x &&
                                     mouse_x < btn.rect.x + btn.rect.w &&
                                     mouse_y >= btn.rect.y &&
                                     mouse_y < btn.rect.y + btn.rect.h;
                        SDL_Color fill = hover ? btn.hover_color : SDL_Color{20, 20, 20, 220};
                        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
                        SDL_RenderFillRect(renderer, &btn.rect);
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_RenderDrawRect(renderer, &btn.rect);
                        int text_w = CustomCharacter::text_width(btn.text, text_scale);
                        int btn_text_x = btn.rect.x + (btn.rect.w - text_w) / 2;
                        int btn_text_y = btn.rect.y + (btn.rect.h - text_height) / 2;
                        CustomCharacter::draw_text(renderer, btn.text, btn_text_x, btn_text_y,
                                                   white, text_scale);
                        if (click_pending && hover)
                        {
                                if (btn.action == ButtonAction::Leaderboard)
                                {
                                        if (transparent && background)
                                        {
                                                restore_background(renderer, background);
                                        }
                                        else
                                        {
                                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                                                SDL_RenderClear(renderer);
                                                SDL_RenderPresent(renderer);
                                        }
                                        LeaderboardMenu::show(window, renderer, width, height,
                                                              transparent);
                                        click_pending = false;
                                }
                                else if (btn.action == ButtonAction::Quit)
                                {
                                        result = ButtonAction::Quit;
                                        running = false;
                                        click_pending = false;
                                }
                                else if (btn.action == ButtonAction::NextLevel)
                                {
                                        result = ButtonAction::NextLevel;
                                        running = false;
                                        click_pending = false;
                                }
                        }
                }

                if (g_developer_mode)
                {
                        SDL_Color red{255, 0, 0, 255};
                        std::string text = "DEVELOPER MODE";
                        int tw = CustomCharacter::text_width(text, text_scale);
                        CustomCharacter::draw_text(renderer, text, width - tw - 5, 5, red,
                                                   text_scale);
                }

                SDL_RenderPresent(renderer);
                SDL_Delay(16);
        }

        SDL_StopTextInput();
        if (background)
                SDL_DestroyTexture(background);
        return result;
}

ButtonAction LevelFinishedMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                                     int height, const LevelFinishedInfo &info,
                                     bool transparent)
{
        LevelFinishedMenu menu;
        return menu.run(window, renderer, width, height, info, transparent);
}
