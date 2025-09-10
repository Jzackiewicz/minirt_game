#include "CustomCharacter.hpp"

const uint8_t *CustomCharacter::get_glyph(char character) {
    switch (character) {
    case ' ':
    {
        static const uint8_t data[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        return data;
    }
    case '0':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        return data;
    }
    case '1':
    {
        static const uint8_t data[7] = {0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F};
        return data;
    }
    case '2':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        return data;
    }
    case '3':
    {
        static const uint8_t data[7] = {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E};
        return data;
    }
    case '4':
    {
        static const uint8_t data[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        return data;
    }
    case '5':
    {
        static const uint8_t data[7] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
        return data;
    }
    case '6':
    {
        static const uint8_t data[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        return data;
    }
    case '7':
    {
        static const uint8_t data[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        return data;
    }
    case '8':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        return data;
    }
    case '9':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
        return data;
    }
    case 'A':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return data;
    }
    case 'B':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
        return data;
    }
    case 'C':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
        return data;
    }
    case 'D':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
        return data;
    }
    case 'E':
    {
        static const uint8_t data[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
        return data;
    }
    case 'F':
    {
        static const uint8_t data[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        return data;
    }
    case 'G':
    {
        static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0F};
        return data;
    }
    case 'H':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return data;
    }
    case 'I':
    {
        static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
        return data;
    }
    case 'J':
    {
        static const uint8_t data[7] = {0x1F, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C};
        return data;
    }
    case 'K':
    {
        static const uint8_t data[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
        return data;
    }
    case 'L':
    {
        static const uint8_t data[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
        return data;
    }
    case 'M':
    {
        static const uint8_t data[7] = {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11};
        return data;
    }
    case 'N':
    {
        static const uint8_t data[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
        return data;
    }
    case 'O':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return data;
    }
    case 'P':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
        return data;
    }
    case 'Q':
    {
        static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x13, 0x0F};
        return data;
    }
    case 'R':
    {
        static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
        return data;
    }
    case 'S':
    {
        static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
        return data;
    }
    case 'T':
    {
        static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
        return data;
    }
    case 'U':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return data;
    }
    case 'V':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
        return data;
    }
    case 'W':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11};
        return data;
    }
    case 'X':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
        return data;
    }
    case 'Y':
    {
        static const uint8_t data[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
        return data;
    }
    case 'Z':
    {
        static const uint8_t data[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
        return data;
    }
    default:
        return nullptr;
    }
}

void CustomCharacter::draw_character(SDL_Renderer *renderer, char character, int x, int y,
                                     SDL_Color color, int scale) {
    const uint8_t *glyph = get_glyph(character);
    if (!glyph)
        return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if (glyph[row] & (1 << (4 - column))) {
                SDL_Rect rect{ x + column * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void CustomCharacter::draw_text(SDL_Renderer *renderer, const std::string &text, int x, int y,
                                SDL_Color color, int scale) {
    for (char c : text) {
        draw_character(renderer, c, x, y, color, scale);
        x += (5 + 1) * scale;
    }
}

int CustomCharacter::text_width(const std::string &text, int scale) {
    return (static_cast<int>(text.size()) * (5 + 1) - 1) * scale;
}
