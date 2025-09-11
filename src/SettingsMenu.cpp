#include "SettingsMenu.hpp"
#include <SDL.h>
#include <iomanip>
#include <sstream>

static std::vector<std::string> build_sens_values() {
    std::vector<std::string> vals;
    for (int i = 1; i <= 20; ++i) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << i / 10.0f;
        vals.push_back(ss.str());
    }
    return vals;
}

static int resolution_index_from_settings() {
    if (g_settings.width == 720 && g_settings.height == 480)
        return 0;
    if (g_settings.width == 1080 && g_settings.height == 720)
        return 1;
    if (g_settings.width == 1366 && g_settings.height == 768)
        return 2;
    return 3;
}

SettingsMenu::SettingsMenu()
    : AMenu("SETTINGS"),
      quality_cluster({"LOW", "MEDIUM", "HIGH"}, g_settings.quality == 'L' ? 0 : (g_settings.quality == 'M' ? 1 : 2)),
      sensitivity_slider("MOUSE SENSITIVITY", build_sens_values(),
                         static_cast<int>(g_settings.mouse_sensitivity * 10) - 1),
      resolution_slider("RESOLUTION", {"720x480", "1080x720", "1366x768", "1920x1080"},
                        resolution_index_from_settings()),
      back_button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}},
      apply_button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}} {}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}

void SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    bool running = true;
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
        int title_y = static_cast<int>(20 * scale_factor);
        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;

        int y = title_y + title_height + static_cast<int>(40 * scale_factor);
        int center_x = width / 2 - button_width / 2;

        // Quality section
        int quality_label_x = width / 2 - CustomCharacter::text_width("QUALITY", scale) / 2;
        int quality_label_y = y;
        y += 7 * scale + button_gap;
        int cluster_button_width = (button_width - 2 * button_gap) / 3;
        quality_cluster.set_layout(center_x, y, cluster_button_width, button_height, button_gap);
        y += button_height + button_gap * 2;

        // Sensitivity slider
        int sens_label_y = y;
        y += 7 * scale + button_gap;
        sensitivity_slider.set_rect(center_x, y + button_height / 2 - scale / 2, button_width, scale);
        y += button_height + button_gap * 2;

        // Resolution slider
        int res_label_y = y;
        y += 7 * scale + button_gap;
        resolution_slider.set_rect(center_x, y + button_height / 2 - scale / 2, button_width, scale);
        y += button_height + button_gap * 2;

        // Bottom buttons
        int half_width = (button_width - button_gap) / 2;
        back_button.rect = {center_x, y, half_width, button_height};
        apply_button.rect = {center_x + half_width + button_gap, y, half_width, button_height};

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            quality_cluster.handle_event(e);
            sensitivity_slider.handle_event(e);
            resolution_slider.handle_event(e);
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;
                if (mx >= back_button.rect.x && mx < back_button.rect.x + back_button.rect.w &&
                    my >= back_button.rect.y && my < back_button.rect.y + back_button.rect.h) {
                    running = false; // back
                } else if (mx >= apply_button.rect.x && mx < apply_button.rect.x + apply_button.rect.w &&
                           my >= apply_button.rect.y && my < apply_button.rect.y + apply_button.rect.h) {
                    // apply settings
                    int qidx = quality_cluster.active_index();
                    g_settings.quality = (qidx == 0 ? 'L' : (qidx == 1 ? 'M' : 'H'));
                    g_settings.mouse_sensitivity = std::stof(sensitivity_slider.current_value());
                    std::string res = resolution_slider.current_value();
                    std::size_t x = res.find('x');
                    if (x != std::string::npos) {
                        g_settings.width = std::stoi(res.substr(0, x));
                        g_settings.height = std::stoi(res.substr(x + 1));
                        SDL_SetWindowSize(window, g_settings.width, g_settings.height);
                    }
                    save_settings(g_settings);
                    running = false;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Title
        CustomCharacter::draw_text(renderer, title, title_x, title_y, white, title_scale);

        // Labels
        CustomCharacter::draw_text(renderer, "QUALITY", quality_label_x, quality_label_y, white, scale);
        CustomCharacter::draw_text(renderer, "MOUSE SENSITIVITY", center_x + (button_width - CustomCharacter::text_width("MOUSE SENSITIVITY", scale)) / 2,
                                   sens_label_y, white, scale);
        CustomCharacter::draw_text(renderer, "RESOLUTION", center_x + (button_width - CustomCharacter::text_width("RESOLUTION", scale)) / 2,
                                   res_label_y, white, scale);

        // Widgets
        quality_cluster.render(renderer, scale);
        sensitivity_slider.render(renderer, scale);
        resolution_slider.render(renderer, scale);

        // Back and Apply buttons
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        Button *btns[2] = {&back_button, &apply_button};
        for (Button *btn : btns) {
            bool hover = mx >= btn->rect.x && mx < btn->rect.x + btn->rect.w &&
                         my >= btn->rect.y && my < btn->rect.y + btn->rect.h;
            SDL_Color fill = hover ? btn->hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn->rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn->rect);
            int text_x = btn->rect.x + (btn->rect.w - CustomCharacter::text_width(btn->text, scale)) / 2;
            int text_y = btn->rect.y + (btn->rect.h - 7 * scale) / 2;
            SDL_Color tc = white;
            if (btn == &apply_button && hover)
                tc = white;
            CustomCharacter::draw_text(renderer, btn->text, text_x, text_y, tc, scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

