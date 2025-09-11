#include "Slider.hpp"
#include <cmath>

Slider::Slider(const std::string &l, const std::vector<std::string> &vals, int default_index)
    : label(l), values(vals), index(default_index), rect{0, 0, 0, 0}, dragging(false) {
    if (index < 0 || index >= static_cast<int>(values.size()))
        index = 0;
}

void Slider::handle_event(const SDL_Event &e) {
    auto update_index = [this](int mx) {
        if (rect.w <= 0 || values.empty())
            return;
        int rel = mx - rect.x;
        if (rel < 0)
            rel = 0;
        if (rel > rect.w)
            rel = rect.w;
        double ratio = static_cast<double>(rel) / static_cast<double>(rect.w);
        index = static_cast<int>(std::round(ratio * (values.size() - 1)));
    };
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y - 5 && my <= rect.y + rect.h + 5) {
            dragging = true;
            update_index(mx);
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (e.type == SDL_MOUSEMOTION && dragging) {
        update_index(e.motion.x);
    }
}

void Slider::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int track_y = rect.y + rect.h / 2;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, rect.x, track_y, rect.x + rect.w, track_y);
    // ticks
    if (values.size() > 1) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            int x = rect.x + static_cast<int>((static_cast<double>(i) / (values.size() - 1)) * rect.w);
            SDL_RenderDrawLine(renderer, x, track_y - scale, x, track_y + scale);
        }
    }
    // handle
    int handle_x = rect.x;
    if (values.size() > 1)
        handle_x += static_cast<int>((static_cast<double>(index) / (values.size() - 1)) * rect.w);
    SDL_Rect handle_rect{handle_x - scale, track_y - scale, 2 * scale, 2 * scale};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &handle_rect);
    // label
    int label_x = rect.x + (rect.w - CustomCharacter::text_width(label, scale)) / 2;
    int label_y = rect.y - 7 * scale - scale;
    CustomCharacter::draw_text(renderer, label, label_x, label_y, white, scale);
    // value
    std::string val = values[index];
    int val_x = rect.x + rect.w + scale * 2;
    int val_y = track_y - 3 * scale;
    CustomCharacter::draw_text(renderer, val, val_x, val_y, white, scale);
}

