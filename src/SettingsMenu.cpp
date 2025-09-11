#include "SettingsMenu.hpp"

SettingsMenu::SettingsMenu()
    : AMenu("SETTINGS"), quality(), mouse_sensitivity(), resolution() {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}});
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                        int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}

ButtonAction SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width,
                               int height) {
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_height = 7 * title_scale;
        int section_gap = static_cast<int>(40 * scale_factor);
        int section_height = static_cast<int>(60 * scale_factor);
        int buttons_height = static_cast<int>(50 * scale_factor);
        int total_height = title_height + section_gap +
                           3 * section_height + 2 * section_gap + buttons_height;
        int top_margin = (height - total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;
        int center_x = width / 2;
        int section_width = static_cast<int>(400 * scale_factor);
        int section_x = center_x - section_width / 2;
        int y = top_margin;
        int title_x = center_x - CustomCharacter::text_width(title, title_scale) / 2;

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
                        if (btn.action == ButtonAction::Back) {
                            result = ButtonAction::Back;
                            running = false;
                        }
                        break;
                    }
                }
            }
            quality.handle_event(event);
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        CustomCharacter::draw_text(renderer, title, title_x, y, white, title_scale);
        y += title_height + section_gap;

        quality.draw(renderer, section_x, y, section_width, scale);
        y += section_height + section_gap;
        mouse_sensitivity.draw(renderer, section_x, y, section_width, scale);
        y += section_height + section_gap;
        resolution.draw(renderer, section_x, y, section_width, scale);
        y += section_height + section_gap;

        int button_gap = static_cast<int>(40 * scale_factor);
        int button_width = (section_width - button_gap) / 2;
        int button_height = buttons_height;
        buttons[0].rect = {section_x, y, button_width, button_height};
        buttons[1].rect =
            {section_x + button_width + button_gap, y, button_width, button_height};

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x = btn.rect.x +
                         (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white,
                                       scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}
