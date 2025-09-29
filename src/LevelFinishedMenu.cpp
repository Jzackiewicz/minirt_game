#include "LevelFinishedMenu.hpp"
#include "LeaderboardMenu.hpp"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace {
constexpr std::size_t kMaxLeaderboardEntries = 10;
constexpr double kScoreComparisonEpsilon = 1e-6;

struct LeaderboardEntry {
    std::string name;
    double score = 0.0;
};

std::string trim_copy(const std::string &value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                      return std::isspace(ch) != 0;
                  }).base();
    if (begin >= end)
        return std::string();
    return std::string(begin, end);
}

std::vector<LeaderboardEntry> load_leaderboard(const std::string &path) {
    std::vector<LeaderboardEntry> entries;
    std::ifstream input(path);
    if (!input)
        return entries;
    std::string line;
    while (std::getline(input, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = trim_copy(line.substr(0, pos));
        std::string score_text = trim_copy(line.substr(pos + 1));
        if (name.empty() || score_text.empty())
            continue;
        char *endptr = nullptr;
        double score = std::strtod(score_text.c_str(), &endptr);
        if (!endptr || endptr == score_text.c_str())
            continue;
        entries.push_back({name, score});
    }
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry &lhs,
                                                const LeaderboardEntry &rhs) {
        if (std::abs(lhs.score - rhs.score) <= kScoreComparisonEpsilon)
            return lhs.name < rhs.name;
        return lhs.score > rhs.score;
    });
    if (entries.size() > kMaxLeaderboardEntries)
        entries.resize(kMaxLeaderboardEntries);
    return entries;
}

bool save_leaderboard(const std::string &path,
                      const std::vector<LeaderboardEntry> &entries) {
    std::ofstream output(path, std::ios::trunc);
    if (!output)
        return false;
    output.setf(std::ios::fixed);
    output << std::setprecision(1);
    for (const auto &entry : entries) {
        output << entry.name << ": " << entry.score << '\n';
    }
    return static_cast<bool>(output);
}

int insert_entry(std::vector<LeaderboardEntry> &entries, const LeaderboardEntry &entry) {
    auto it = std::find_if(entries.begin(), entries.end(), [&](const LeaderboardEntry &value) {
        return entry.score > value.score + kScoreComparisonEpsilon;
    });
    int placement = static_cast<int>(std::distance(entries.begin(), it)) + 1;
    entries.insert(it, entry);
    if (entries.size() > kMaxLeaderboardEntries)
        entries.resize(kMaxLeaderboardEntries);
    return placement;
}

std::string ordinal(int value) {
    int mod100 = value % 100;
    int mod10 = value % 10;
    std::string suffix = "th";
    if (mod100 < 11 || mod100 > 13) {
        if (mod10 == 1)
            suffix = "st";
        else if (mod10 == 2)
            suffix = "nd";
        else if (mod10 == 3)
            suffix = "rd";
    }
    return std::to_string(value) + suffix;
}

std::string make_title(const LevelFinishedStats &stats) {
    if (stats.tutorial_mode)
        return "TUTORIAL FINISHED";
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

    if (stats_.tutorial_mode) {
        Button back_button{"BACK TO MENU", ButtonAction::BackToMenu,
                           SDL_Color{192, 160, 255, 255}};
        while (running) {
            SDL_GetWindowSize(window, &width, &height);
            float scale_factor = static_cast<float>(height) / 600.0f;
            int base_scale = std::max(1, static_cast<int>(4 * scale_factor));
            int title_scale = base_scale * 2;
            int button_width = std::max(180, static_cast<int>(320 * scale_factor));
            int button_height = std::max(60, static_cast<int>(90 * scale_factor));
            int margin = std::max(10, static_cast<int>(60 * scale_factor));

            int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
            int title_y = margin;
            back_button.rect = {width / 2 - button_width / 2,
                                height - button_height - margin, button_width, button_height};

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    result = ButtonAction::Quit;
                } else if (event.type == SDL_KEYDOWN &&
                           (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                            event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER ||
                            event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                    running = false;
                    result = ButtonAction::BackToMenu;
                } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                           event.button.button == SDL_BUTTON_LEFT) {
                    int mx = event.button.x;
                    int my = event.button.y;
                    if (point_in_rect(back_button.rect, mx, my)) {
                        running = false;
                        result = ButtonAction::BackToMenu;
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

            auto draw_button = [&](const Button &btn) {
                bool hover = point_in_rect(btn.rect, mx, my);
                SDL_Color fill = hover ? btn.hover_color : SDL_Color{96, 96, 160, 220};
                SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
                SDL_RenderFillRect(renderer, &btn.rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &btn.rect);
                int text_scale = base_scale;
                int text_width = CustomCharacter::text_width(btn.text, text_scale);
                int text_x = btn.rect.x + (btn.rect.w - text_width) / 2;
                int text_y = btn.rect.y + (btn.rect.h - 7 * text_scale) / 2;
                CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white, text_scale);
            };

            draw_button(back_button);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        if (background)
            SDL_DestroyTexture(background);
        return result;
    }

    const bool show_name_input = !stats_.has_next_level;
    bool allow_name_input = show_name_input;
    bool name_field_active = allow_name_input;
    bool submission_complete = false;
    bool show_feedback = false;
    bool show_checkmark = false;
    std::string feedback_text;
    const std::string leaderboard_path = "leaderboard.yaml";
    const int max_name_length = 20;

    Button next_button{"NEXT LEVEL", ButtonAction::NextLevel, SDL_Color{96, 255, 128, 255}};
    Button leaderboard_button{"LEADERBOARD", ButtonAction::Leaderboard,
                              SDL_Color{96, 128, 255, 255}};
    Button back_button{"BACK TO MENU", ButtonAction::BackToMenu,
                       SDL_Color{255, 96, 96, 255}};
    SDL_Color submit_idle_color{64, 160, 96, 220};
    Button submit_button{"SUBMIT", ButtonAction::None, SDL_Color{96, 255, 128, 255}};

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

    if (allow_name_input)
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
        std::string general_text;
        if (show_name_input)
            general_text = "GENERAL SCORE: " + format_score(stats_.total_score);

        int bottom_y = height - button_height - margin;
        int bottom_total_width = button_width * 2 + button_gap;
        int bottom_start_x = width / 2 - bottom_total_width / 2;
        leaderboard_button.rect = {bottom_start_x, bottom_y, button_width, button_height};
        back_button.rect = {bottom_start_x + button_width + button_gap, bottom_y, button_width,
                            button_height};

        SDL_Rect name_rect{0, 0, 0, 0};
        SDL_Rect submit_rect{0, 0, 0, 0};
        bool submit_enabled = allow_name_input && !player_name_.empty();
        auto update_submit_enabled = [&]() {
            submit_enabled = allow_name_input && !player_name_.empty();
        };
        int content_bottom = bottom_y;
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
            content_bottom = name_rect.y;
        } else if (stats_.has_next_level) {
            int next_y = bottom_y - button_height - button_gap;
            next_button.rect = {width / 2 - button_width / 2, next_y, button_width, button_height};
            content_bottom = next_button.rect.y;
        }

        int text_height = 7 * text_scale;
        int score_lines = show_name_input ? 2 : 1;
        int total_score_height = score_lines * text_height + (score_lines - 1) * line_gap;
        int top_limit = title_y + title_height + line_gap;
        int bottom_limit = content_bottom - line_gap;
        if (bottom_limit < top_limit)
            bottom_limit = top_limit;
        int available_height = bottom_limit - top_limit - total_score_height;
        if (available_height < 0)
            available_height = 0;
        int score_y = top_limit + available_height / 2;
        int score_x = width / 2 - CustomCharacter::text_width(score_text, text_scale) / 2;
        int general_y = 0;
        int general_x = 0;
        if (show_name_input) {
            general_y = score_y + text_height + line_gap;
            general_x = width / 2 - CustomCharacter::text_width(general_text, text_scale) / 2;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    if (stats_.has_next_level) {
                        running = false;
                        result = ButtonAction::Resume;
                    }
                } else if (!stats_.has_next_level &&
                           event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    // Ignore enter on final screen.
                } else if (stats_.has_next_level &&
                           (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                            event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
                    running = false;
                    result = ButtonAction::NextLevel;
                } else if (allow_name_input &&
                           event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    if (!player_name_.empty()) {
                        player_name_.pop_back();
                        update_submit_enabled();
                    }
                }
            } else if (event.type == SDL_TEXTINPUT && allow_name_input && name_field_active) {
                for (const char *p = event.text.text; *p; ++p) {
                    if (player_name_.size() >= static_cast<size_t>(max_name_length))
                        break;
                    unsigned char ch = static_cast<unsigned char>(*p);
                    if (ch < 32)
                        continue;
                    player_name_.push_back(static_cast<char>(ch));
                }
                update_submit_enabled();
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
                    if (was_active && allow_name_input)
                        SDL_StartTextInput();
                } else if (point_in_rect(back_button.rect, mx, my)) {
                    running = false;
                    result = ButtonAction::BackToMenu;
                } else if (allow_name_input && point_in_rect(name_rect, mx, my)) {
                    name_field_active = true;
                    if (!SDL_IsTextInputActive())
                        SDL_StartTextInput();
                } else if (point_in_rect(submit_rect, mx, my)) {
                    if (allow_name_input && submit_enabled && !submission_complete) {
                        std::string trimmed_name = trim_copy(player_name_);
                        if (trimmed_name.empty()) {
                            player_name_.clear();
                            update_submit_enabled();
                            name_field_active = true;
                            if (!SDL_IsTextInputActive())
                                SDL_StartTextInput();
                        } else {
                            player_name_ = trimmed_name;
                            auto entries = load_leaderboard(leaderboard_path);
                            bool qualifies = entries.size() < kMaxLeaderboardEntries ||
                                             entries.empty();
                            if (!qualifies && !entries.empty()) {
                                double lowest = entries.back().score;
                                qualifies = stats_.total_score >
                                            lowest + kScoreComparisonEpsilon;
                            }
                            bool saved = false;
                            if (qualifies) {
                                int placement = insert_entry(entries, {player_name_, stats_.total_score});
                                saved = save_leaderboard(leaderboard_path, entries);
                                if (saved)
                                    feedback_text = ordinal(placement) +
                                                    " place on the leaderboard!";
                            }
                            if (!qualifies || !saved) {
                                feedback_text = "Thank you for playing!";
                            }
                            show_feedback = true;
                            show_checkmark = true;
                            submission_complete = true;
                            allow_name_input = false;
                            name_field_active = false;
                            submit_button.text = "SUBMITED";
                            if (SDL_IsTextInputActive())
                                SDL_StopTextInput();
                            update_submit_enabled();
                        }
                    }
                } else if (allow_name_input) {
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

        auto draw_button = [&](const Button &btn, bool enabled, bool accent,
                               SDL_Color accent_color, bool render_checkmark) {
            SDL_Color base_color = enabled ? SDL_Color{20, 20, 20, 220}
                                           : SDL_Color{60, 60, 60, 220};
            bool hover = point_in_rect(btn.rect, mx, my);
            SDL_Color fill = base_color;
            if (enabled) {
                if (accent)
                    fill = hover ? btn.hover_color : accent_color;
                else if (hover)
                    fill = btn.hover_color;
            }
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            SDL_Color text_color = enabled ? white : SDL_Color{180, 180, 180, 255};
            int text_width = CustomCharacter::text_width(btn.text, text_scale);
            int padding = std::max(2, text_scale);
            int text_x = render_checkmark ? (btn.rect.x + padding)
                                          : (btn.rect.x + (btn.rect.w - text_width) / 2);
            int text_y = btn.rect.y + (btn.rect.h - 7 * text_scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, text_color, text_scale);
            if (render_checkmark) {
                int check_padding = std::max(2, text_scale);
                int check_width = std::max(6, text_scale * 5);
                int check_height = std::max(6, text_scale * 6);
                int check_left = btn.rect.x + btn.rect.w - check_width - check_padding;
                int check_center_y = btn.rect.y + btn.rect.h / 2;
                SDL_SetRenderDrawColor(renderer, 96, 255, 128, 255);
                for (int offset = -1; offset <= 1; ++offset) {
                    SDL_RenderDrawLine(renderer, check_left, check_center_y + offset,
                                       check_left + check_width / 2,
                                       check_center_y + check_height / 2 + offset);
                    SDL_RenderDrawLine(renderer, check_left + check_width / 2,
                                       check_center_y + check_height / 2 + offset,
                                       check_left + check_width,
                                       check_center_y - check_height / 2 + offset);
                }
            }
        };

        if (stats_.has_next_level)
            draw_button(next_button, true, false, SDL_Color{}, false);
        draw_button(leaderboard_button, true, false, SDL_Color{}, false);
        draw_button(back_button, true, false, SDL_Color{}, false);

        if (show_name_input) {
            SDL_Color bg_color{20, 20, 20, 220};
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &name_rect);
            SDL_Color border_color = (allow_name_input && name_field_active)
                                         ? SDL_Color{96, 255, 128, 255}
                                         : SDL_Color{255, 255, 255, 255};
            SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b,
                                   border_color.a);
            SDL_RenderDrawRect(renderer, &name_rect);

            std::string display_text;
            SDL_Color text_color = white;
            if (show_feedback) {
                display_text = feedback_text;
            } else {
                display_text = player_name_;
                if (display_text.empty()) {
                    display_text = "YOUR NAME...";
                    text_color = SDL_Color{150, 150, 150, 255};
                }
            }
            int padding = 3 * text_scale;
            int text_x = name_rect.x + padding;
            int text_y = name_rect.y + (name_rect.h - 7 * text_scale) / 2;
            CustomCharacter::draw_text(renderer, display_text, text_x, text_y, text_color,
                                       text_scale);

            draw_button(submit_button, submit_enabled,
                        allow_name_input && !submission_complete, submit_idle_color,
                        show_checkmark);
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
