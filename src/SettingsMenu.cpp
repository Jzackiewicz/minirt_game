#include "SettingsMenu.hpp"
#include <SDL.h>

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}});
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
        int title_y = static_cast<int>(40 * scale_factor);
        int title_x =
            width / 2 - CustomCharacter::text_width(title, title_scale) / 2;

        int section_width = static_cast<int>(300 * scale_factor);
        int section_gap = static_cast<int>(40 * scale_factor);
        int label_height = 7 * scale;
        int gap = 5 * scale;
        int placeholder_height = static_cast<int>(20 * scale);

        int quality_y = title_y + title_height + section_gap;
        int after_quality = quality.layout(width / 2, quality_y, section_width, scale);
        int mouse_y = after_quality + section_gap;
        int resolution_y = mouse_y + label_height + gap + placeholder_height + section_gap;

        int button_width = static_cast<int>(140 * scale_factor);
        int button_height = static_cast<int>(60 * scale_factor);
        int button_gap = static_cast<int>(20 * scale_factor);
        int bottom_y = height - button_height - section_gap;
        buttons[0].rect = {width / 2 - button_width - button_gap / 2, bottom_y,
                           button_width, button_height};
        buttons[1].rect = {width / 2 + button_gap / 2, bottom_y, button_width,
                           button_height};

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                quality.handle_event(event);
                int mx = event.button.x;
                int my = event.button.y;
                for (auto &btn : buttons) {
                    if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                        if (btn.action == ButtonAction::Back) {
                            result = btn.action;
                            running = false;
                        }
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Title
        CustomCharacter::draw_text(renderer, title, title_x, title_y, white,
                                   title_scale);

        // Sections
        quality.draw(renderer, width / 2, quality_y, section_width, scale);
        mouse.draw(renderer, width / 2, mouse_y, section_width, scale);
        resolution.draw(renderer, width / 2, resolution_y, section_width, scale);

        // Bottom buttons
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x = btn.rect.x +
                         (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) /
                             2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white,
                                       scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                        int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}
