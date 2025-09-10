#pragma once
#include <SDL.h>
#include <cstdint>
#include <string>

// Utility class for drawing ASCII text using SDL
class CustomCharacter {
private:
    CustomCharacter() = delete; // Non-instantiable

public:
    static const uint8_t *get_glyph(char character);
    static void draw_character(SDL_Renderer *renderer, char character, int x, int y,
                               SDL_Color color, int scale);
    static void draw_text(SDL_Renderer *renderer, const std::string &text, int x, int y,
                          SDL_Color color, int scale);
    static int text_width(const std::string &text, int scale);
};
