#include "SettingsMenu.hpp"
#include "CustomCharacter.hpp"
#include <SDL.h>
#include <sstream>
#include <iomanip>
#include <cmath>

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    buttons.clear();
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255,0,0,255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0,255,0,255}});
}

ButtonAction SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    ButtonsCluster quality;
    quality.add("LOW");
    quality.add("MEDIUM");
    quality.add("HIGH");
    if (g_settings.quality == 'L' || g_settings.quality == 'l') quality.selected = 0;
    else if (g_settings.quality == 'M' || g_settings.quality == 'm') quality.selected = 1;
    else quality.selected = 2;

    std::vector<std::string> mouse_vals;
    for (int i = 1; i <= 20; ++i) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << 0.1 * i;
        mouse_vals.push_back(ss.str());
    }
    Slider mouse_slider("MOUSE SENSITIVITY", mouse_vals);
    int mouse_index = static_cast<int>(std::round((g_settings.mouse_sensitivity - 0.1) / 0.1));
    if (mouse_index < 0) mouse_index = 0;
    if (mouse_index >= (int)mouse_vals.size()) mouse_index = (int)mouse_vals.size() - 1;
    mouse_slider.current = mouse_index;

    std::vector<std::string> res_vals = {"720x480","1080x720","1366x768","1920x1080"};
    Slider res_slider("RESOLUTION", res_vals);
    int res_index = 1;
    if (g_settings.width == 720 && g_settings.height == 480) res_index = 0;
    else if (g_settings.width == 1080 && g_settings.height == 720) res_index = 1;
    else if (g_settings.width == 1366 && g_settings.height == 768) res_index = 2;
    else if (g_settings.width == 1920 && g_settings.height == 1080) res_index = 3;
    res_slider.current = res_index;

    bool running = true;
    ButtonAction result = ButtonAction::None;
    SDL_Color white{255,255,255,255};

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            }
            mouse_slider.handle_event(e);
            res_slider.handle_event(e);
            quality.handle_event(e);
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;
                for (auto &btn : buttons) {
                    if (mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                        my >= btn.rect.y && my < btn.rect.y + btn.rect.h) {
                        if (btn.text == "BACK") {
                            running = false;
                            result = ButtonAction::Back;
                        } else if (btn.text == "APPLY") {
                            g_settings.mouse_sensitivity = std::stod(mouse_slider.current_value());
                            std::string q = quality.current_text();
                            if (!q.empty()) g_settings.quality = q[0];
                            std::string res = res_slider.current_value();
                            auto x = res.find('x');
                            if (x != std::string::npos) {
                                g_settings.width = std::stoi(res.substr(0, x));
                                g_settings.height = std::stoi(res.substr(x + 1));
                                SDL_SetWindowSize(window, g_settings.width, g_settings.height);
                            }
                            save_settings();
                            running = false;
                            result = ButtonAction::Back;
                        }
                    }
                }
            }
        }

        SDL_GetWindowSize(window, &width, &height);
        float scale_factor = static_cast<float>(height) / 600.0f;
        int button_width = static_cast<int>(300 * scale_factor);
        int button_height = static_cast<int>(80 * scale_factor);
        int gap = static_cast<int>(20 * scale_factor);
        int scale = static_cast<int>(4 * scale_factor);
        if (scale < 1) scale = 1;
        int title_scale = scale * 2;
        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = gap;
        int current_y = title_y + 7 * title_scale + gap * 2;
        int center_x = width / 2 - button_width / 2;
        int label_h = 7 * scale;

        // layout for quality cluster
        quality.set_layout(center_x, current_y + label_h + gap/2, button_width, button_height/2, gap/2);
        int after_quality = current_y + label_h + gap/2 + button_height/2 + gap;
        mouse_slider.set_bar(center_x, after_quality + label_h + gap/2, button_width, scale * 3);
        int after_mouse = after_quality + label_h + gap/2 + scale * 3 + gap;
        res_slider.set_bar(center_x, after_mouse + label_h + gap/2, button_width, scale * 3);
        int bottom_y = height - button_height - gap;
        int half_w = (button_width - gap) / 2;
        buttons[0].rect = {center_x, bottom_y, half_w, button_height};
        buttons[1].rect = {center_x + half_w + gap, bottom_y, half_w, button_height};

        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);
        CustomCharacter::draw_text(renderer, title, title_x, title_y, white, title_scale);
        CustomCharacter::draw_text(renderer, "QUALITY",
                                   center_x + (button_width - CustomCharacter::text_width("QUALITY", scale))/2,
                                   current_y, white, scale);
        quality.draw(renderer, scale);
        mouse_slider.draw(renderer, scale);
        res_slider.draw(renderer, scale);

        for (auto &btn : buttons) {
            SDL_Color fill{0,0,0,255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int tx = btn.rect.x + (btn.rect.w - CustomCharacter::text_width(btn.text, scale))/2;
            int ty = btn.rect.y + (btn.rect.h - 7*scale)/2;
            CustomCharacter::draw_text(renderer, btn.text, tx, ty, white, scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return result;
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}
