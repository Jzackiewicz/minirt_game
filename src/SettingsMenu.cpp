#include "SettingsMenu.hpp"

SettingsMenu::SettingsMenu() : AMenu("SETTINGS"), quality(), mouse(), resolution() {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}});
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                        int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}

ButtonAction SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer,
                               int width, int height) {
    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255, 255, 255, 255};

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int button_width = static_cast<int>(300 * scale_factor);
        int button_height = static_cast<int>(100 * scale_factor);
        int button_gap = static_cast<int>(10 * scale_factor);
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_height = 7 * title_scale;
        int title_gap = static_cast<int>(40 * scale_factor);
        int section_height = static_cast<int>(80 * scale_factor);
        int section_gap = static_cast<int>(20 * scale_factor);
        int sections_total_height = section_height * 3 + section_gap * 2 + button_height;
        int top_margin =
            (height - title_height - title_gap - sections_total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int section_width = static_cast<int>(width * 0.6f);
        int section_x = width / 2 - section_width / 2;
        int current_y = title_y + title_height + title_gap;

        quality.set_area(section_x, current_y, section_width, section_height);
        quality.layout(scale);
        current_y += section_height + section_gap;

        mouse.set_area(section_x, current_y, section_width, section_height);
        mouse.layout(scale);
        current_y += section_height + section_gap;

        resolution.set_area(section_x, current_y, section_width, section_height);
        resolution.layout(scale);
        current_y += section_height + section_gap;

        buttons[0].rect = {width / 2 - button_width - button_gap / 2, current_y,
                           button_width, button_height};
        buttons[1].rect = {width / 2 + button_gap / 2, current_y, button_width,
                           button_height};

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;
                if (mx >= buttons[0].rect.x &&
                    mx < buttons[0].rect.x + buttons[0].rect.w &&
                    my >= buttons[0].rect.y &&
                    my < buttons[0].rect.y + buttons[0].rect.h) {
                    result = ButtonAction::Back;
                    running = false;
                } else if (mx >= buttons[1].rect.x &&
                           mx < buttons[1].rect.x + buttons[1].rect.w &&
                           my >= buttons[1].rect.y &&
                           my < buttons[1].rect.y + buttons[1].rect.h) {
                    // Apply button placeholder
                }
            }
            quality.handle_event(event);
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int tx = title_x;
        for (char c : title) {
            CustomCharacter::draw_character(renderer, c, tx, title_y, white,
                                            title_scale);
            tx += (5 + 1) * title_scale;
        }

        quality.render(renderer, scale);
        mouse.render(renderer, scale);
        resolution.render(renderer, scale);

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x =
                btn.rect.x +
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

