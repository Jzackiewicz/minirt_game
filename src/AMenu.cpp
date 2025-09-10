#include "AMenu.hpp"

AMenu::AMenu(const std::string &t) : title(t) {}

ButtonAction AMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    (void)window; // unused
    int button_width = 300;
    int button_height = 100;
    int button_gap = 10;
    int scale = 4;
    int title_scale = scale * 2;
    int title_gap = 80;

    int total_buttons_height = static_cast<int>(buttons.size()) * button_height +
                               (static_cast<int>(buttons.size()) - 1) * button_gap;
    int title_height = 7 * title_scale;
    int top_margin = (height - title_height - title_gap - total_buttons_height) / 2;
    if (top_margin < 0)
        top_margin = 0;

    int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
    int title_y = top_margin;

    int center_x = width / 2 - button_width / 2;
    int start_y = title_y + title_height + title_gap;
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].rect = {center_x, start_y + static_cast<int>(i) * (button_height + button_gap),
                           button_width, button_height};
    }

    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                for (auto &btn : buttons) {
                    if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                        if (btn.action != ButtonAction::Settings &&
                            btn.action != ButtonAction::Leaderboard) {
                            result = btn.action;
                            running = false;
                        }
                        break;
                    }
                }
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color colors[] = {
            {0, 0, 255, 255},
            {255, 255, 0, 255},
            {0, 255, 0, 255},
            {255, 0, 0, 255},
            {0, 255, 255, 255},
            {128, 0, 128, 255}
        };
        int tx = title_x;
        for (std::size_t i = 0; i < title.size(); ++i) {
            SDL_Color c = white;
            if (i < sizeof(colors) / sizeof(colors[0]))
                c = colors[i];
            CustomCharacter::draw_character(renderer, title[i], tx, title_y, c, title_scale);
            tx += (5 + 1) * title_scale;
        }

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x = btn.rect.x + (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white, scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}
