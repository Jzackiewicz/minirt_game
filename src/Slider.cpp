#include "Slider.hpp"
#include "CustomCharacter.hpp"
#include <cmath>

void Slider::set_bar(int x, int y, int w, int h) {
    bar = {x, y, w, h};
}

void Slider::handle_event(const SDL_Event &e) {
    if (values.empty()) return;
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (e.button.x >= bar.x && e.button.x <= bar.x + bar.w &&
            e.button.y >= bar.y && e.button.y <= bar.y + bar.h) {
            dragging = true;
            int pos = e.button.x - bar.x;
            int n = static_cast<int>(values.size()) - 1;
            current = static_cast<int>(std::round(static_cast<double>(pos) / bar.w * n));
            if (current < 0) current = 0;
            if (current > n) current = n;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (e.type == SDL_MOUSEMOTION && dragging) {
        int pos = e.motion.x - bar.x;
        if (pos < 0) pos = 0;
        if (pos > bar.w) pos = bar.w;
        int n = static_cast<int>(values.size()) - 1;
        current = static_cast<int>(std::round(static_cast<double>(pos) / bar.w * n));
        if (current < 0) current = 0;
        if (current > n) current = n;
    }
}

void Slider::draw(SDL_Renderer *renderer, int scale) const {
    if (values.empty()) return;
    SDL_Color white{255,255,255,255};
    int label_w = CustomCharacter::text_width(label, scale);
    CustomCharacter::draw_text(renderer, label, bar.x + (bar.w - label_w)/2,
                               bar.y - (7*scale + scale), white, scale);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bar);
    int n = static_cast<int>(values.size()) - 1;
    for (int i = 0; i <= n; ++i) {
        int x = bar.x + i * bar.w / n;
        SDL_RenderDrawLine(renderer, x, bar.y, x, bar.y + bar.h);
    }
    int knob_x = bar.x + current * bar.w / n - scale;
    SDL_Rect knob{knob_x, bar.y, scale*2, bar.h};
    SDL_SetRenderDrawColor(renderer, 255,0,0,255);
    SDL_RenderFillRect(renderer, &knob);
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    std::string val = current_value();
    CustomCharacter::draw_text(renderer, val,
                               bar.x + bar.w + scale*2,
                               bar.y + (bar.h - 7*scale)/2,
                               white, scale);
}
