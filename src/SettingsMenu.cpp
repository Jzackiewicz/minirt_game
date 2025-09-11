#include "SettingsMenu.hpp"
#include "CustomCharacter.hpp"
#include <cmath>
#include <cstdio>

// -----------------------------------------------------------------------------
// ButtonsCluster implementation
// -----------------------------------------------------------------------------

ButtonsCluster::ButtonsCluster(const std::vector<std::string> &labels)
    : selected(labels.empty() ? -1 : 0) {
    for (const auto &l : labels) {
        buttons.push_back(Button{l, ButtonAction::None, SDL_Color{0, 0, 0, 255}});
    }
}

void ButtonsCluster::layout(int x, int y, int width, int height, int gap) {
    if (buttons.empty())
        return;
    int btn_width = (width - static_cast<int>(buttons.size() - 1) * gap) /
                    static_cast<int>(buttons.size());
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].rect = {x + static_cast<int>(i) * (btn_width + gap), y, btn_width,
                           height};
    }
}

void ButtonsCluster::handle_event(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
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
        const SDL_Rect &rect = buttons[i].rect;
        bool pressed = static_cast<int>(i) == selected;
        SDL_Color fill = pressed ? white : black;
        SDL_Color text = pressed ? black : white;
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
        int text_x = rect.x + (rect.w - CustomCharacter::text_width(buttons[i].text, scale)) / 2;
        int text_y = rect.y + (rect.h - 7 * scale) / 2;
        CustomCharacter::draw_text(renderer, buttons[i].text, text_x, text_y, text, scale);
    }
}

// -----------------------------------------------------------------------------
// Slider implementation
// -----------------------------------------------------------------------------

Slider::Slider(const std::vector<std::string> &vals, int initial)
    : values(vals), selected(initial), dragging(false) {}

void Slider::layout(int x, int y, int width, int height, int scale) {
    int track_height = 10 * scale;
    int gap = 5 * scale;
    int max_text_width = 0;
    for (const auto &v : values) {
        int w = CustomCharacter::text_width(v, scale);
        if (w > max_text_width)
            max_text_width = w;
    }
    track_rect = {x, y + (height - track_height) / 2,
                  width - max_text_width - gap, track_height};
    value_pos = {track_rect.x + track_rect.w + gap,
                 track_rect.y + (track_rect.h - 7 * scale) / 2};
}

void Slider::update_from_position(int x) {
    if (track_rect.w <= 0 || values.empty())
        return;
    int relative = x - track_rect.x;
    if (relative < 0)
        relative = 0;
    if (relative > track_rect.w)
        relative = track_rect.w;
    double ratio = static_cast<double>(relative) / track_rect.w;
    int index = static_cast<int>(std::round(ratio * (values.size() - 1)));
    if (index < 0)
        index = 0;
    if (index >= static_cast<int>(values.size()))
        index = static_cast<int>(values.size()) - 1;
    selected = index;
}

void Slider::handle_event(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        if (mx >= track_rect.x && mx <= track_rect.x + track_rect.w &&
            my >= track_rect.y && my <= track_rect.y + track_rect.h) {
            dragging = true;
            update_from_position(mx);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP &&
               event.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (event.type == SDL_MOUSEMOTION && dragging) {
        update_from_position(event.motion.x);
    }
}

void Slider::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &track_rect);
    SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, white.a);
    SDL_RenderDrawRect(renderer, &track_rect);

    if (values.size() > 1) {
        double spacing = static_cast<double>(track_rect.w) /
                         static_cast<double>(values.size() - 1);
        for (std::size_t i = 0; i < values.size(); ++i) {
            int x = static_cast<int>(std::round(track_rect.x + i * spacing));
            SDL_RenderDrawLine(renderer, x, track_rect.y, x,
                               track_rect.y + track_rect.h);
        }
        int handle_width = 5 * scale;
        int center_x =
            static_cast<int>(std::round(track_rect.x + selected * spacing));
        SDL_Rect handle{center_x - handle_width / 2, track_rect.y, handle_width,
                        track_rect.h};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &handle);
    }

    CustomCharacter::draw_text(renderer, values[selected], value_pos.x,
                               value_pos.y, white, scale);
}

// -----------------------------------------------------------------------------
// SettingsSection implementation
// -----------------------------------------------------------------------------

SettingsSection::SettingsSection(const std::string &text) : label(text) {}

void SettingsSection::layout(int x, int y, int width, int height, int scale) {
    int label_height = 7 * scale;
    int gap = 2 * scale;
    content_rect = {x, y + label_height + gap, width,
                    height - label_height - gap};
}

void SettingsSection::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_height = 7 * scale;
    int label_x = content_rect.x + (content_rect.w - label_width) / 2;
    int label_y = content_rect.y - label_height - 2 * scale;
    CustomCharacter::draw_text(renderer, label, label_x, label_y, white, scale);

    // Placeholder rectangle for content
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &content_rect);
}

// -----------------------------------------------------------------------------
// QualitySection implementation
// -----------------------------------------------------------------------------

QualitySection::QualitySection()
    : SettingsSection("QUALITY"),
      cluster({"LOW", "MEDIUM", "HIGH"}) {
    cluster.selected = 2; // default to HIGH
}

void QualitySection::layout(int x, int y, int width, int height, int scale) {
    SettingsSection::layout(x, y, width, height, scale);
    int gap = 2 * scale;
    cluster.layout(content_rect.x, content_rect.y, content_rect.w, content_rect.h, gap);
}

void QualitySection::handle_event(const SDL_Event &event) {
    cluster.handle_event(event);
}

void QualitySection::draw(SDL_Renderer *renderer, int scale) const {
    // Draw label (without placeholder rectangle)
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_height = 7 * scale;
    int label_x = content_rect.x + (content_rect.w - label_width) / 2;
    int label_y = content_rect.y - label_height - 2 * scale;
    CustomCharacter::draw_text(renderer, label, label_x, label_y, white, scale);

    cluster.draw(renderer, scale);
}

// -----------------------------------------------------------------------------
// Slider based sections
// -----------------------------------------------------------------------------

MouseSensitivitySection::MouseSensitivitySection()
    : SettingsSection("MOUSE SENSITIVITY"), slider({}) {
    std::vector<std::string> values;
    for (int i = 1; i <= 20; ++i) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%.1f", i * 0.1f);
        values.emplace_back(buf);
    }
    slider = Slider(values, 9); // default 1.0
}

void MouseSensitivitySection::layout(int x, int y, int width, int height, int scale) {
    SettingsSection::layout(x, y, width, height, scale);
    slider.layout(content_rect.x, content_rect.y, content_rect.w, content_rect.h, scale);
}

void MouseSensitivitySection::handle_event(const SDL_Event &event) {
    slider.handle_event(event);
}

void MouseSensitivitySection::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_height = 7 * scale;
    int label_x = content_rect.x + (content_rect.w - label_width) / 2;
    int label_y = content_rect.y - label_height - 2 * scale;
    CustomCharacter::draw_text(renderer, label, label_x, label_y, white, scale);

    slider.draw(renderer, scale);
}

ResolutionSection::ResolutionSection()
    : SettingsSection("RESOLUTION"),
      slider({"720x480", "1080x720", "1366x768", "1920x1080"}, 3) {}

void ResolutionSection::layout(int x, int y, int width, int height, int scale) {
    SettingsSection::layout(x, y, width, height, scale);
    slider.layout(content_rect.x, content_rect.y, content_rect.w, content_rect.h, scale);
}

void ResolutionSection::handle_event(const SDL_Event &event) {
    slider.handle_event(event);
}

void ResolutionSection::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_height = 7 * scale;
    int label_x = content_rect.x + (content_rect.w - label_width) / 2;
    int label_y = content_rect.y - label_height - 2 * scale;
    CustomCharacter::draw_text(renderer, label, label_x, label_y, white, scale);

    slider.draw(renderer, scale);
}

// -----------------------------------------------------------------------------
// SettingsMenu implementation
// -----------------------------------------------------------------------------

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    // Bottom buttons
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
    buttons.push_back(Button{"APPLY", ButtonAction::None, SDL_Color{0, 255, 0, 255}});
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
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
        int title_gap = static_cast<int>(40 * scale_factor);

        // Section layout values
        int section_width = static_cast<int>(500 * scale_factor);
        int content_height = static_cast<int>(70 * scale_factor);
        int section_height = content_height + 7 * scale + 2 * scale; // content + label + gap
        int section_gap = static_cast<int>(30 * scale_factor);

        int button_width = static_cast<int>(150 * scale_factor);
        int button_height = static_cast<int>(80 * scale_factor);
        int button_gap = static_cast<int>(20 * scale_factor);

        int total_sections_height = 3 * section_height + 2 * section_gap;
        int total_bottom_width = 2 * button_width + button_gap;

        int total_height = title_height + title_gap + total_sections_height + section_gap +
                           button_height;
        int top_margin = (height - total_height) / 2;
        if (top_margin < 0)
            top_margin = 0;

        int title_x = width / 2 - CustomCharacter::text_width(title, title_scale) / 2;
        int title_y = top_margin;

        int section_x = width / 2 - section_width / 2;
        int section_start_y = title_y + title_height + title_gap;

        quality.layout(section_x, section_start_y, section_width, section_height, scale);
        mouse_sensitivity.layout(section_x,
                                 section_start_y + section_height + section_gap,
                                 section_width, section_height, scale);
        resolution.layout(section_x,
                          section_start_y + 2 * (section_height + section_gap),
                          section_width, section_height, scale);

        int buttons_start_y = section_start_y + 3 * section_height + 2 * section_gap + section_gap;
        int buttons_start_x = width / 2 - total_bottom_width / 2;
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            buttons[i].rect = {buttons_start_x + static_cast<int>(i) *
                                               (button_width + button_gap),
                               buttons_start_y, button_width, button_height};
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = ButtonAction::Quit;
            } else {
                quality.handle_event(event);
                mouse_sensitivity.handle_event(event);
                resolution.handle_event(event);

                if (event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    int mx = event.button.x;
                    int my = event.button.y;

                    // Bottom buttons
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
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        CustomCharacter::draw_text(renderer, title, title_x, title_y, white, title_scale);

        quality.draw(renderer, scale);
        mouse_sensitivity.draw(renderer, scale);
        resolution.draw(renderer, scale);

        for (auto &btn : buttons) {
            bool hover = mx >= btn.rect.x && mx < btn.rect.x + btn.rect.w &&
                         my >= btn.rect.y && my < btn.rect.y + btn.rect.h;
            SDL_Color fill = hover ? btn.hover_color : SDL_Color{0, 0, 0, 255};
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            int text_x =
                btn.rect.x + (btn.rect.w - CustomCharacter::text_width(btn.text, scale)) / 2;
            int text_y = btn.rect.y + (btn.rect.h - 7 * scale) / 2;
            CustomCharacter::draw_text(renderer, btn.text, text_x, text_y, white, scale);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return result;
}

