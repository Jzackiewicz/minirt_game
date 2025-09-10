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
        case 'H':
        {
                static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x1F,
                                                                                0x11, 0x11, 0x11};
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
        const uint8_t *glyph;
        glyph = get_glyph(character);
        if (!glyph)
        {
                return;
        }
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (int row = 0; row < 7; ++row)
        {
                for (int column = 0; column < 5; ++column)
                {
                        if (glyph[row] & (1 << (4 - column)))
                        {
                                SDL_Rect rectangle;
                                rectangle = {x + column * scale, y + row * scale, scale, scale};
                                SDL_RenderFillRect(renderer, &rectangle);
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

bool PauseMenu::show(int width, int height)
{
        SDL_Window *window;
        window = SDL_CreateWindow("Paused", SDL_WINDOWPOS_CENTERED,
                                                          SDL_WINDOWPOS_CENTERED, width, height, 0);
        if (!window)
                return true;
        SDL_Renderer *renderer;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer)
        {
                SDL_DestroyWindow(window);
                return true;
        }
        int button_width = 300;
        int button_height = 100;
        int scale = 4;
        int title_scale = scale * 2;
        std::string title = "PAUSED";
        int button_gap = 10;
        int title_gap = 80;
        int total_buttons_height = 3 * button_height + 2 * button_gap;
        int title_height = 7 * title_scale;
        int top_margin = (height - title_height - title_gap - total_buttons_height) / 2;
        if (top_margin < 0)
                top_margin = 0;
        int title_x = width / 2 - text_width(title, title_scale) / 2;
        int title_y = top_margin;
        int resume_y = title_y + title_height + title_gap;
        int center_x = width / 2 - button_width / 2;
        SDL_Rect resume_rect = {center_x, resume_y, button_width, button_height};
        SDL_Rect settings_rect = {center_x, resume_rect.y + button_height + button_gap,
                                                        button_width, button_height};
        SDL_Rect quit_rect = {center_x, settings_rect.y + button_height + button_gap,
                                      button_width, button_height};
        bool running = true;
        bool resume_selected = true;
        while (running)
        {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                        if (event.type == SDL_QUIT)
                        {
                                running = false;
                                resume_selected = true;
                        }
                        else if (event.type == SDL_MOUSEBUTTONDOWN &&
                                         event.button.button == SDL_BUTTON_LEFT)
                        {
                                int mouse_x = event.button.x;
                                int mouse_y = event.button.y;
                                if (mouse_x >= resume_rect.x &&
                                        mouse_x < resume_rect.x + resume_rect.w &&
                                        mouse_y >= resume_rect.y &&
                                        mouse_y < resume_rect.y + resume_rect.h)
                                {
                                        running = false;
                                        resume_selected = true;
                                }
                                else if (mouse_x >= quit_rect.x &&
                                                 mouse_x < quit_rect.x + quit_rect.w &&
                                                 mouse_y >= quit_rect.y &&
                                                 mouse_y < quit_rect.y + quit_rect.h)
                                {
                                        running = false;
                                        resume_selected = false;
                                }
                        }
                }
                int mouse_x;
                int mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                bool hover_resume = mouse_x >= resume_rect.x &&
                                                  mouse_x < resume_rect.x + resume_rect.w &&
                                                  mouse_y >= resume_rect.y &&
                                                  mouse_y < resume_rect.y + resume_rect.h;
                bool hover_settings = mouse_x >= settings_rect.x &&
                                                   mouse_x < settings_rect.x + settings_rect.w &&
                                                   mouse_y >= settings_rect.y &&
                                                   mouse_y < settings_rect.y + settings_rect.h;
                bool hover_quit = mouse_x >= quit_rect.x &&
                                              mouse_x < quit_rect.x + quit_rect.w &&
                                              mouse_y >= quit_rect.y &&
                                              mouse_y < quit_rect.y + quit_rect.h;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                SDL_Color white = {255, 255, 255, 255};
                draw_text(renderer, title, title_x, title_y, white, title_scale);
                SDL_Color fill_resume = hover_resume ? SDL_Color{0, 255, 0, 255}
                                                     : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill_resume.r, fill_resume.g,
                                       fill_resume.b, fill_resume.a);
                SDL_RenderFillRect(renderer, &resume_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &resume_rect);
                int text_x = resume_rect.x +
                                      (resume_rect.w - text_width("RESUME", scale)) / 2;
                int text_y = resume_rect.y + (resume_rect.h - 7 * scale) / 2;
                draw_text(renderer, "RESUME", text_x, text_y, white, scale);
                SDL_Color fill_settings = hover_settings ? SDL_Color{255, 255, 0, 255}
                                                         : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill_settings.r, fill_settings.g,
                                       fill_settings.b, fill_settings.a);
                SDL_RenderFillRect(renderer, &settings_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &settings_rect);
                text_x = settings_rect.x +
                                 (settings_rect.w - text_width("SETTINGS", scale)) / 2;
                text_y = settings_rect.y + (settings_rect.h - 7 * scale) / 2;
                draw_text(renderer, "SETTINGS", text_x, text_y, white, scale);
                SDL_Color fill_quit = hover_quit ? SDL_Color{255, 0, 0, 255}
                                                  : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill_quit.r, fill_quit.g, fill_quit.b,
                                       fill_quit.a);
                SDL_RenderFillRect(renderer, &quit_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &quit_rect);
                text_x = quit_rect.x + (quit_rect.w - text_width("QUIT", scale)) / 2;
                text_y = quit_rect.y + (quit_rect.h - 7 * scale) / 2;
                draw_text(renderer, "QUIT", text_x, text_y, white, scale);
                SDL_RenderPresent(renderer);
                SDL_Delay(16);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return resume_selected;
}

