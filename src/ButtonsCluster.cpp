#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

ButtonsCluster::ButtonsCluster(std::vector<Button> btns)
    : buttons(std::move(btns)), rects(buttons.size()), selected(0) {}

void ButtonsCluster::handle_event(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        for (std::size_t i = 0; i < rects.size(); ++i) {
            SDL_Rect r = rects[i];
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                selected = static_cast<int>(i);
            }
        }
    }
}

void ButtonsCluster::draw(SDL_Renderer *renderer, int x, int y, int width, int scale) {
    int gap = 2 * scale;
    int height = 7 * scale + 2 * scale;
    int btn_width =
        (width - gap * (static_cast<int>(buttons.size()) - 1)) /
        static_cast<int>(buttons.size());
    SDL_Color white{255, 255, 255, 255};

    for (std::size_t i = 0; i < buttons.size(); ++i) {
        SDL_Rect r{ x + static_cast<int>(i) * (btn_width + gap), y, btn_width,
                    height };
        rects[i] = r;
        if (static_cast<int>(i) == selected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        SDL_RenderDrawRect(renderer, &r);
        int text_x =
            r.x + (r.w - CustomCharacter::text_width(buttons[i].text, scale)) / 2;
        int text_y = r.y + (r.h - 7 * scale) / 2;
        SDL_Color text_color = static_cast<int>(i) == selected
                                    ? SDL_Color{0, 0, 0, 255}
                                    : SDL_Color{255, 255, 255, 255};
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y,
                                   text_color, scale);
    }
}
