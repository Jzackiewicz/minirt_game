#include "ButtonsCluster.hpp"

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &texts, int default_active)
    : active(default_active) {
    for (auto &t : texts) {
        buttons.push_back(Button{t, ButtonAction::None, SDL_Color{0, 0, 0, 255}});
    }
    if (active < 0 || active >= static_cast<int>(buttons.size()))
        active = 0;
}

void ButtonsCluster::set_layout(int x, int y, int button_width, int button_height, int gap) {
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].rect = {x + static_cast<int>(i) * (button_width + gap), y, button_width, button_height};
    }
}

void ButtonsCluster::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            const SDL_Rect &r = buttons[i].rect;
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
                active = static_cast<int>(i);
                break;
            }
        }
    }
}

void ButtonsCluster::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    SDL_Color black{0, 0, 0, 255};
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const SDL_Rect &r = buttons[i].rect;
        bool hover = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
        SDL_Color fill = (static_cast<int>(i) == active) ? white : (hover ? buttons[i].hover_color : SDL_Color{0, 0, 0, 255});
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &r);
        SDL_Color text_color = (static_cast<int>(i) == active) ? black : white;
        int text_x = r.x + (r.w - CustomCharacter::text_width(buttons[i].text, scale)) / 2;
        int text_y = r.y + (r.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y, text_color, scale);
    }
}

