#include "PauseMenu.hpp"
#include <SDL.h>
#include <string>

namespace
{
const uint8_t *get_glyph(char character)
{
    switch (character)
    {
    case 'A':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x1F,
                                        0x11, 0x11, 0x11};
        return data;
    }
    case 'B':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E,
                                        0x11, 0x11, 0x1E};
        return data;
    }
    case 'D':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x11,
                                        0x11, 0x11, 0x1E};
        return data;
    }
    case 'E':
    {
        static const uint8_t data[7] = {0x1F, 0x10, 0x10, 0x1E,
                                        0x10, 0x10, 0x1F};
        return data;
    }
    case 'G':
    {
        static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x13,
                                        0x11, 0x11, 0x0F};
        return data;
    }
    case 'I':
    {
        static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04,
                                        0x04, 0x04, 0x1F};
        return data;
    }
    case 'L':
    {
        static const uint8_t data[7] = {0x10, 0x10, 0x10, 0x10,
                                        0x10, 0x10, 0x1F};
        return data;
    }
    case 'M':
    {
        static const uint8_t data[7] = {0x11, 0x1B, 0x15, 0x11,
                                        0x11, 0x11, 0x11};
        return data;
    }
    case 'N':
    {
        static const uint8_t data[7] = {0x11, 0x19, 0x15, 0x13,
                                        0x11, 0x11, 0x11};
        return data;
    }
    case 'O':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x11,
                                        0x11, 0x11, 0x0E};
        return data;
    }
    case 'P':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E,
                                        0x10, 0x10, 0x10};
        return data;
    }
    case 'Q':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x11,
                                        0x11, 0x13, 0x0F};
        return data;
    }
    case 'R':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E,
                                        0x14, 0x12, 0x11};
        return data;
    }
    case 'S':
    {
        static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x0E,
                                        0x01, 0x01, 0x1E};
        return data;
    }
    case 'T':
    {
        static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04,
                                        0x04, 0x04, 0x04};
        return data;
    }
    case 'U':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x11,
                                        0x11, 0x11, 0x0E};
        return data;
    }
    case 'Y':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x0A, 0x04,
                                        0x04, 0x04, 0x04};
        return data;
    }
    default:
        return nullptr;
    }
}

void draw_character(SDL_Renderer *renderer, char character, int x, int y,
                    SDL_Color color, int scale)
{
    const uint8_t *glyph = get_glyph(character);
    if (!glyph)
        return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int row = 0; row < 7; ++row)
    {
        for (int column = 0; column < 5; ++column)
        {
            if (glyph[row] & (1 << (4 - column)))
            {
                SDL_Rect rect = {x + column * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void draw_text(SDL_Renderer *renderer, const std::string &text, int x, int y,
               SDL_Color color, int scale)
{
    for (char character : text)
    {
        draw_character(renderer, character, x, y, color, scale);
        x += (5 + 1) * scale;
    }
}

int text_width(const std::string &text, int scale)
{
    return (static_cast<int>(text.size()) * (5 + 1) - 1) * scale;
}
} // namespace

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(255 * 0.35));
    SDL_Rect overlay = {0, 0, width, height};
    SDL_RenderFillRect(renderer, &overlay);

    int button_width = 300;
    int button_height = 100;
    int margin = (height - 4 * button_height) / 5;

    SDL_Rect resume_rect = {width / 2 - button_width / 2, margin, button_width, button_height};
    SDL_Rect leaderboard_rect = {width / 2 - button_width / 2,
                                 margin * 2 + button_height, button_width, button_height};
    SDL_Rect settings_rect = {width / 2 - button_width / 2,
                              margin * 3 + 2 * button_height, button_width, button_height};
    SDL_Rect quit_rect = {width / 2 - button_width / 2,
                          margin * 4 + 3 * button_height, button_width, button_height};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &resume_rect);
    SDL_RenderFillRect(renderer, &leaderboard_rect);
    SDL_RenderFillRect(renderer, &settings_rect);
    SDL_RenderFillRect(renderer, &quit_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &resume_rect);
    SDL_RenderDrawRect(renderer, &leaderboard_rect);
    SDL_RenderDrawRect(renderer, &settings_rect);
    SDL_RenderDrawRect(renderer, &quit_rect);

    int scale = 4;
    SDL_Color white = {255, 255, 255, 255};
    int text_x = width / 2 - text_width("PAUSE", scale) / 2;
    int text_y = margin / 2 - (7 * scale) / 2;
    draw_text(renderer, "PAUSE", text_x, text_y, white, scale);

    text_x = resume_rect.x + (resume_rect.w - text_width("RESUME", scale)) / 2;
    text_y = resume_rect.y + (resume_rect.h - 7 * scale) / 2;
    draw_text(renderer, "RESUME", text_x, text_y, white, scale);

    text_x = leaderboard_rect.x + (leaderboard_rect.w - text_width("LEADERBOARD", scale)) / 2;
    text_y = leaderboard_rect.y + (leaderboard_rect.h - 7 * scale) / 2;
    draw_text(renderer, "LEADERBOARD", text_x, text_y, white, scale);

    text_x = settings_rect.x + (settings_rect.w - text_width("SETTINGS", scale)) / 2;
    text_y = settings_rect.y + (settings_rect.h - 7 * scale) / 2;
    draw_text(renderer, "SETTINGS", text_x, text_y, white, scale);

    text_x = quit_rect.x + (quit_rect.w - text_width("QUIT", scale)) / 2;
    text_y = quit_rect.y + (quit_rect.h - 7 * scale) / 2;
    draw_text(renderer, "QUIT", text_x, text_y, white, scale);

    SDL_RenderPresent(renderer);

    bool resume = false;
    bool waiting = true;
    while (waiting)
    {
        SDL_Event e;
        if (SDL_WaitEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                waiting = false;
                resume = false;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                waiting = false;
                resume = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x;
                int my = e.button.y;
                if (mx >= resume_rect.x && mx < resume_rect.x + resume_rect.w &&
                    my >= resume_rect.y && my < resume_rect.y + resume_rect.h)
                {
                    waiting = false;
                    resume = true;
                }
                else if (mx >= quit_rect.x && mx < quit_rect.x + quit_rect.w &&
                         my >= quit_rect.y && my < quit_rect.y + quit_rect.h)
                {
                    waiting = false;
                    resume = false;
                }
            }
        }
    }

    return resume;
}

