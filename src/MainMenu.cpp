#include "MainMenu.hpp"
#include "Leaderboard.hpp"
#include "Settings.hpp"
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
        case 'R':
        {
                static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E,
                                                                                0x14, 0x12, 0x11};
                return data;
        }
        case 'Q':
        {
                static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x11,
                                                                                0x11, 0x13, 0x0F};
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
        case 'H':
        {
                static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x1F,
                                                                                0x11, 0x11, 0x11};
                return data;
        }
        case 'M':
        {
                static const uint8_t data[7] = {0x11, 0x1B, 0x15, 0x11,
                                                                                0x11, 0x11, 0x11};
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

void draw_minirt(SDL_Renderer *renderer, int x, int y, int scale)
{
        const char letters[6] = {'M', 'I', 'N', 'I', 'R', 'T'};
        const SDL_Color colors[6] = {{0, 0, 255, 255},   {255, 255, 0, 255},
                                                                {0, 255, 0, 255},   {255, 0, 0, 255},
                                                                {0, 255, 255, 255}, {128, 0, 128, 255}};
        for (int i = 0; i < 6; ++i)
        {
                draw_character(renderer, letters[i], x, y, colors[i], scale);
                x += (5 + 1) * scale;
        }
}

int text_width(const std::string &text, int scale)
{
	return (static_cast<int>(text.size()) * (5 + 1) - 1) * scale;
}
} // namespace

bool MainMenu::show(int width, int height)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return false;
	}
	SDL_Window *window;
	window = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (!window)
	{
		SDL_Quit();
		return false;
	}
	SDL_Renderer *renderer;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
	{
		SDL_DestroyWindow(window);
		SDL_Quit();
		return false;
	}
        int button_width;
        button_width = 300;
        int button_height;
        button_height = 100;
        int title_scale;
        title_scale = 5;
        int title_height;
        title_height = 7 * title_scale;
        int margin;
        margin = (height - title_height - 4 * button_height) / 5;
        int title_width;
        title_width = text_width("MINIRT THE GAME", title_scale);
        int title_x;
        title_x = width / 2 - title_width / 2;
        int title_y;
        title_y = margin;
        SDL_Rect play_rect;
        play_rect = {width / 2 - button_width / 2,
                                  margin * 2 + title_height, button_width,
                                  button_height};
        SDL_Rect leaderboard_rect;
        leaderboard_rect = {width / 2 - button_width / 2,
                                        margin * 3 + title_height + button_height,
                                        button_width, button_height};
        SDL_Rect settings_rect;
        settings_rect = {width / 2 - button_width / 2,
                                         margin * 4 + title_height + 2 * button_height,
                                         button_width, button_height};
        SDL_Rect quit_rect;
        quit_rect = {width / 2 - button_width / 2,
                                      margin * 5 + title_height + 3 * button_height,
                                      button_width, button_height};
        bool running;
        running = true;
	bool play_selected;
	play_selected = false;
	while (running)
	{
		SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                        if (event.type == SDL_QUIT)
                        {
                                running = false;
                        }
                        else if (event.type == SDL_MOUSEBUTTONDOWN &&
                                         event.button.button == SDL_BUTTON_LEFT)
                        {
                                int mouse_x;
                                int mouse_y;
                                mouse_x = event.button.x;
                                mouse_y = event.button.y;
                                if (mouse_x >= play_rect.x &&
                                        mouse_x < play_rect.x + play_rect.w &&
                                        mouse_y >= play_rect.y &&
                                        mouse_y < play_rect.y + play_rect.h)
                                {
                                        play_selected = true;
                                        running = false;
                                }
                                else if (mouse_x >= leaderboard_rect.x &&
                                                 mouse_x < leaderboard_rect.x + leaderboard_rect.w &&
                                                 mouse_y >= leaderboard_rect.y &&
                                                 mouse_y < leaderboard_rect.y + leaderboard_rect.h)
                                {
                                        Leaderboard::show();
                                }
                                else if (mouse_x >= settings_rect.x &&
                                                 mouse_x < settings_rect.x + settings_rect.w &&
                                                 mouse_y >= settings_rect.y &&
                                                 mouse_y < settings_rect.y + settings_rect.h)
                                {
                                        Settings::show();
                                }
                                else if (mouse_x >= quit_rect.x &&
                                                 mouse_x < quit_rect.x + quit_rect.w &&
                                                 mouse_y >= quit_rect.y &&
                                                 mouse_y < quit_rect.y + quit_rect.h)
                                {
                                        running = false;
                                }
                        }
                }
                int mouse_x;
                int mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		bool hover_play;
		hover_play =
			mouse_x >= play_rect.x && mouse_x < play_rect.x + play_rect.w &&
			mouse_y >= play_rect.y && mouse_y < play_rect.y + play_rect.h;
                bool hover_leaderboard;
                hover_leaderboard = mouse_x >= leaderboard_rect.x &&
                                                   mouse_x < leaderboard_rect.x + leaderboard_rect.w &&
                                                   mouse_y >= leaderboard_rect.y &&
                                                   mouse_y < leaderboard_rect.y + leaderboard_rect.h;
                bool hover_settings;
                hover_settings = mouse_x >= settings_rect.x &&
                                                 mouse_x < settings_rect.x + settings_rect.w &&
                                                 mouse_y >= settings_rect.y &&
                                                 mouse_y < settings_rect.y + settings_rect.h;
                bool hover_quit;
                hover_quit = mouse_x >= quit_rect.x &&
                                         mouse_x < quit_rect.x + quit_rect.w &&
                                         mouse_y >= quit_rect.y &&
                                         mouse_y < quit_rect.y + quit_rect.h;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                SDL_Color white;
                white = {255, 255, 255, 255};
                draw_minirt(renderer, title_x, title_y, title_scale);
                draw_text(renderer, " THE GAME",
                                   title_x + text_width("MINIRT", title_scale), title_y,
                                   white, title_scale);
                SDL_Color fill;
                fill = hover_play ? SDL_Color{0, 255, 0, 255}
                                                  : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
                SDL_RenderFillRect(renderer, &play_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &play_rect);
                int scale;
                scale = 4;
                int text_x;
                int text_y;
		text_x = play_rect.x + (play_rect.w - text_width("PLAY", scale)) / 2;
		text_y = play_rect.y + (play_rect.h - 7 * scale) / 2;
                draw_text(renderer, "PLAY", text_x, text_y, white, scale);
                fill = hover_leaderboard ? SDL_Color{0, 0, 255, 255}
                                                          : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
                SDL_RenderFillRect(renderer, &leaderboard_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &leaderboard_rect);
                text_x = leaderboard_rect.x +
                                 (leaderboard_rect.w -
                                  text_width("LEADERBOARD", scale)) /
                                 2;
                text_y =
                        leaderboard_rect.y + (leaderboard_rect.h - 7 * scale) / 2;
                draw_text(renderer, "LEADERBOARD", text_x, text_y, white, scale);
                fill = hover_settings ? SDL_Color{255, 255, 0, 255}
                                                          : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
                SDL_RenderFillRect(renderer, &settings_rect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &settings_rect);
                text_x = settings_rect.x +
                                 (settings_rect.w - text_width("SETTINGS", scale)) / 2;
                text_y = settings_rect.y + (settings_rect.h - 7 * scale) / 2;
                draw_text(renderer, "SETTINGS", text_x, text_y, white, scale);
                fill = hover_quit ? SDL_Color{255, 0, 0, 255}
                                                  : SDL_Color{0, 0, 0, 255};
                SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
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
        SDL_Quit();
        return play_selected;
}
