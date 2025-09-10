#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

void ButtonsCluster::add(const std::string &text) {
    items.push_back(Item{text, SDL_Rect{0,0,0,0}});
}

void ButtonsCluster::set_layout(int x, int y, int total_w, int h, int gap) {
    if (items.empty()) return;
    int w = (total_w - (static_cast<int>(items.size()) - 1) * gap) /
            static_cast<int>(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        items[i].rect = {x + static_cast<int>(i) * (w + gap), y, w, h};
    }
}

void ButtonsCluster::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        for (std::size_t i = 0; i < items.size(); ++i) {
            SDL_Rect r = items[i].rect;
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                selected = static_cast<int>(i);
                break;
            }
        }
    }
}

void ButtonsCluster::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255,255,255,255};
    for (std::size_t i = 0; i < items.size(); ++i) {
        bool press = static_cast<int>(i) == selected;
        SDL_Color fill = press ? SDL_Color{255,255,255,255} : SDL_Color{0,0,0,255};
        SDL_Color text_color = press ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &items[i].rect);
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        SDL_RenderDrawRect(renderer, &items[i].rect);
        int tx = items[i].rect.x + (items[i].rect.w - CustomCharacter::text_width(items[i].text, scale))/2;
        int ty = items[i].rect.y + (items[i].rect.h - 7*scale)/2;
        CustomCharacter::draw_text(renderer, items[i].text, tx, ty, text_color, scale);
    }
}

std::string ButtonsCluster::current_text() const {
    if (items.empty()) return std::string();
    return items[selected].text;
}
