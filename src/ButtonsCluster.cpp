#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &names, int initial)
    : selected(initial) {
    for (const auto &n : names)
        buttons.emplace_back(n, ButtonAction::None, SDL_Color{255,255,255,255});
    if (selected < 0 || selected >= (int)buttons.size())
        selected = 0;
}

void ButtonsCluster::set_rect(int x, int y, int w, int h, int gap) {
    for (size_t i=0;i<buttons.size();++i) {
        buttons[i].rect = {x + int(i)*(w+gap), y, w, h};
    }
}

void ButtonsCluster::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx=e.button.x, my=e.button.y;
        for (size_t i=0;i<buttons.size();++i) {
            const SDL_Rect &r = buttons[i].rect;
            if (mx>=r.x && mx<r.x+r.w && my>=r.y && my<r.y+r.h) {
                selected = i;
                break;
            }
        }
    }
}

void ButtonsCluster::render(SDL_Renderer *renderer, int scale) const {
    for (size_t i=0;i<buttons.size();++i) {
        const SDL_Rect &r = buttons[i].rect;
        bool sel = (int)i==selected;
        SDL_Color fill = sel ? SDL_Color{255,255,255,255} : SDL_Color{0,0,0,255};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        SDL_RenderDrawRect(renderer, &r);
        SDL_Color text_col = sel ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
        int text_x = r.x + (r.w - CustomCharacter::text_width(buttons[i].text, scale))/2;
        int text_y = r.y + (r.h - 7*scale)/2;
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y, text_col, scale);
    }
}
