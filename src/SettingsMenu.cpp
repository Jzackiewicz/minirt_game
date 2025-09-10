#include "SettingsMenu.hpp"
#include "CustomCharacter.hpp"
#include <sstream>

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}});
}

ButtonAction SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width,
                               int height) {
    bool running = true;
    ButtonAction result = ButtonAction::Back;

    // Prepare slider values
    std::vector<std::string> sens_vals;
    for (int i = 1; i <= 20; ++i) {
        std::ostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(1);
        ss << (i / 10.0);
        sens_vals.push_back(ss.str());
    }
    int sens_index = static_cast<int>(g_settings.mouse_sensitivity / 0.1) - 1;
    if (sens_index < 0)
        sens_index = 0;
    if (sens_index > 19)
        sens_index = 19;
    Slider sens_slider("MOUSE SENSITIVITY", sens_vals, sens_index);

    std::vector<std::string> res_vals{"720x480", "1080x720", "1366x768", "1920x1080"};
    int res_index = 1;
    if (g_settings.width == 720 && g_settings.height == 480)
        res_index = 0;
    else if (g_settings.width == 1080 && g_settings.height == 720)
        res_index = 1;
    else if (g_settings.width == 1366 && g_settings.height == 768)
        res_index = 2;
    else if (g_settings.width == 1920 && g_settings.height == 1080)
        res_index = 3;
    Slider res_slider("RESOLUTION", res_vals, res_index);

    ButtonsCluster quality({"LOW", "MEDIUM", "HIGH"},
                           g_settings.quality == 'L'
                               ? 0
                               : (g_settings.quality == 'M' ? 1 : 2));

    while (running) {
        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int button_width = static_cast<int>(200 * scale_factor);
        int button_height = static_cast<int>(80 * scale_factor);
        int button_gap = static_cast<int>(10 * scale_factor);
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1)
            scale = 1;
        int title_scale = scale * 2;
        int title_height = 7 * title_scale;
        int title_gap = static_cast<int>(40 * scale_factor);

        int top_margin = static_cast<int>(40 * scale_factor);
        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int current_y = title_y + title_height + title_gap;

        int quality_label_x = width / 2 -
                              CustomCharacter::text_width("QUALITY", scale) / 2;
        int quality_label_y = current_y;
        current_y += 7 * scale + button_gap;
        int cluster_total_width = 3 * button_width + 2 * button_gap;
        int cluster_x = width / 2 - cluster_total_width / 2;
        quality.set_position(cluster_x, current_y, button_width, button_height,
                             button_gap);
        current_y += button_height + title_gap;

        int slider_width = static_cast<int>(400 * scale_factor);
        int slider_height = scale * 4;
        sens_slider.set_position(width / 2 - slider_width / 2, current_y, slider_width,
                                 scale);
        current_y += slider_height + title_gap;

        res_slider.set_position(width / 2 - slider_width / 2, current_y, slider_width,
                                scale);
        current_y += slider_height + title_gap;

        int bottom_total_width = 2 * button_width + button_gap;
        int bottom_y = height - button_height - button_gap;
        buttons[0].rect = {width / 2 - bottom_total_width / 2, bottom_y, button_width,
                           button_height};
        buttons[1].rect = {buttons[0].rect.x + button_width + button_gap, bottom_y,
                           button_width, button_height};

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            sens_slider.handle_event(event);
            res_slider.handle_event(event);
            quality.handle_event(event);

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
                        if (btn.text == "BACK") {
                            running = false;
                            result = ButtonAction::Back;
                        } else if (btn.text == "APPLY") {
                            int q = quality.get_selected();
                            g_settings.quality = (q == 0 ? 'L' : (q == 1 ? 'M' : 'H'));
                            g_settings.mouse_sensitivity =
                                (sens_slider.get_index() + 1) * 0.1;
                            int r = res_slider.get_index();
                            if (r == 0) {
                                g_settings.width = 720;
                                g_settings.height = 480;
                            } else if (r == 1) {
                                g_settings.width = 1080;
                                g_settings.height = 720;
                            } else if (r == 2) {
                                g_settings.width = 1366;
                                g_settings.height = 768;
                            } else {
                                g_settings.width = 1920;
                                g_settings.height = 1080;
                            }
                            g_settings.save();
                            running = false;
                            result = ButtonAction::Back;
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Color white{255, 255, 255, 255};
        CustomCharacter::draw_text(renderer, title, title_x, title_y, white,
                                   title_scale);
        CustomCharacter::draw_text(renderer, "QUALITY", quality_label_x,
                                   quality_label_y, white, scale);
        quality.draw(renderer, scale);
        sens_slider.draw(renderer, scale);
        res_slider.draw(renderer, scale);

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

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width,
                        int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}
