#include "ButtonsCluster.hpp"

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &labels)
    : active_index(-1) {
    for (const auto &l : labels) {
        buttons.emplace_back(l, ButtonAction::None, SDL_Color{255, 255, 255, 255});
    }
}

void ButtonsCluster::layout(int x, int y, int width, int height) {
    int count = static_cast<int>(buttons.size());
    int gap = 10;
    int total_gap = gap * (count - 1);
    int button_w = (width - total_gap) / count;
    for (int i = 0; i < count; ++i) {
        buttons[i].rect = {x + i * (button_w + gap), y, button_w, height};
    }
}

void ButtonsCluster::render(SDL_Renderer *renderer, int scale) const {
    for (int i = 0; i < static_cast<int>(buttons.size()); ++i) {
        const SDL_Rect &r = buttons[i].rect;
        bool pressed = (i == active_index);
        SDL_Color fill = pressed ? SDL_Color{255, 255, 255, 255}
                                 : SDL_Color{0, 0, 0, 255};
        SDL_Color text_color = pressed ? SDL_Color{0, 0, 0, 255}
                                       : SDL_Color{255, 255, 255, 255};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &r);
        int text_x =
            r.x + (r.w - CustomCharacter::text_width(buttons[i].text, scale)) / 2;
        int text_y = r.y + (r.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y,
                                   text_color, scale);
    }
}

void ButtonsCluster::handle_event(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        for (int i = 0; i < static_cast<int>(buttons.size()); ++i) {
            const SDL_Rect &r = buttons[i].rect;
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                active_index = i;
            }
        }
    }
}

