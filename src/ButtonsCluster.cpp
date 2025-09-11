#include "ButtonsCluster.hpp"

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &names)
    : selected(0) {
    for (const auto &n : names) {
        buttons.emplace_back(n, ButtonAction::None, SDL_Color{255, 255, 255, 255});
    }
}

void ButtonsCluster::layout(int x, int y, int width, int height) {
    if (buttons.empty())
        return;
    int gap = 5;
    int btn_width = (width - gap * static_cast<int>(buttons.size() - 1)) /
                    static_cast<int>(buttons.size());
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].rect = {x + static_cast<int>(i) * (btn_width + gap), y,
                           btn_width, height};
    }
}

void ButtonsCluster::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            const SDL_Rect &r = buttons[i].rect;
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                selected = static_cast<int>(i);
                break;
            }
        }
    }
}

void ButtonsCluster::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    SDL_Color black{0, 0, 0, 255};
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const SDL_Rect &r = buttons[i].rect;
        bool sel = static_cast<int>(i) == selected;
        if (sel) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        SDL_RenderDrawRect(renderer, &r);
        SDL_Color text_color = sel ? black : white;
        int text_x = r.x + (r.w - CustomCharacter::text_width(buttons[i].text, scale)) / 2;
        int text_y = r.y + (r.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y,
                                   text_color, scale);
    }
}
