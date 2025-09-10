#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &labels,
                               int default_index)
    : texts(labels), rects(labels.size()), selected(default_index) {}

void ButtonsCluster::set_position(int x, int y, int button_width, int button_height,
                                  int gap) {
    for (std::size_t i = 0; i < rects.size(); ++i) {
        rects[i] = {x + static_cast<int>(i) * (button_width + gap), y, button_width,
                    button_height};
    }
}

bool ButtonsCluster::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        for (std::size_t i = 0; i < rects.size(); ++i) {
            const SDL_Rect &r = rects[i];
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                selected = static_cast<int>(i);
                return true;
            }
        }
    }
    return false;
}

void ButtonsCluster::draw(SDL_Renderer *renderer, int scale) const {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    for (std::size_t i = 0; i < rects.size(); ++i) {
        const SDL_Rect &r = rects[i];
        bool hover = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
        if (static_cast<int>(i) == selected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &r);
            int tx = r.x + (r.w - CustomCharacter::text_width(texts[i], scale)) / 2;
            int ty = r.y + (r.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, texts[i], tx, ty,
                                       SDL_Color{0, 0, 0, 255}, scale);
        } else {
            SDL_Color fill = hover ? SDL_Color{128, 128, 128, 255}
                                   : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &r);
            int tx = r.x + (r.w - CustomCharacter::text_width(texts[i], scale)) / 2;
            int ty = r.y + (r.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, texts[i], tx, ty,
                                       SDL_Color{255, 255, 255, 255}, scale);
        }
    }
}
