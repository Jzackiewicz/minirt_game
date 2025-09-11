#include "Slider.hpp"
#include "CustomCharacter.hpp"
#include <algorithm>
#include <cmath>

Slider::Slider(const std::vector<std::string> &vals, int default_index)
    : values(vals), selected(default_index), dragging(false) {
    if (selected < 0 || selected >= static_cast<int>(values.size()))
        selected = 0;
}

void Slider::layout(int x, int y, int width, int height, int scale) {
    int bar_height = 2 * scale;
    // Reserve space on the right for the value text
    int max_text = 0;
    for (const auto &v : values)
        max_text = std::max(max_text, CustomCharacter::text_width(v, scale));
    int value_space = max_text + 2 * scale;
    if (value_space > width)
        value_space = 0;
    bar_rect = {x, y + (height - bar_height) / 2, width - value_space, bar_height};
}

void Slider::update_from_mouse(int mx) {
    if (values.empty() || bar_rect.w <= 0)
        return;
    float rel = static_cast<float>(mx - bar_rect.x) / static_cast<float>(bar_rect.w);
    rel = std::clamp(rel, 0.0f, 1.0f);
    int max_index = static_cast<int>(values.size()) - 1;
    selected = static_cast<int>(std::round(rel * max_index));
    selected = std::clamp(selected, 0, max_index);
}

void Slider::handle_event(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        SDL_Rect handle;
        int handle_width = 2 * bar_rect.h;
        if (values.size() > 1)
            handle.x = bar_rect.x + (bar_rect.w - handle_width) * selected /
                                      (static_cast<int>(values.size()) - 1);
        else
            handle.x = bar_rect.x;
        handle.y = bar_rect.y - bar_rect.h / 2;
        handle.w = handle_width;
        handle.h = bar_rect.h * 2;
        if (mx >= handle.x && mx < handle.x + handle.w && my >= handle.y &&
            my < handle.y + handle.h) {
            dragging = true;
        }
        if (mx >= bar_rect.x && mx < bar_rect.x + bar_rect.w && my >= bar_rect.y - bar_rect.h &&
            my < bar_rect.y + 2 * bar_rect.h) {
            update_from_mouse(mx);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (event.type == SDL_MOUSEMOTION && dragging) {
        update_from_mouse(event.motion.x);
    }
}

void Slider::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    SDL_Color red{255, 0, 0, 255};

    // Draw bar
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bar_rect);
    SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, white.a);
    SDL_RenderDrawRect(renderer, &bar_rect);

    // Draw handle
    int handle_width = 2 * bar_rect.h;
    int handle_x;
    if (values.size() > 1)
        handle_x = bar_rect.x + (bar_rect.w - handle_width) * selected /
                                     (static_cast<int>(values.size()) - 1);
    else
        handle_x = bar_rect.x;
    SDL_Rect handle{handle_x, bar_rect.y - bar_rect.h / 2, handle_width,
                    bar_rect.h * 2};
    SDL_SetRenderDrawColor(renderer, red.r, red.g, red.b, red.a);
    SDL_RenderFillRect(renderer, &handle);

    // Draw value text
    int value_x = bar_rect.x + bar_rect.w + scale;
    int value_y = bar_rect.y + bar_rect.h / 2 - 7 * scale / 2;
    CustomCharacter::draw_text(renderer, values[selected], value_x, value_y, white, scale);
}

