#include "HowToPlayMenu.hpp"

#include "CustomCharacter.hpp"

#include <SDL.h>
#include <algorithm>
#include <sstream>

HowToPlayMenu::HowToPlayMenu() : AMenu("HOW TO PLAY") {
    title_colors = {SDL_Color{255, 255, 255, 255}};
    buttons_align_bottom = true;
    buttons_bottom_margin = 60;
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void HowToPlayMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                         bool transparent) {
    HowToPlayMenu menu;
    menu.run(window, renderer, width, height, transparent);
}

void HowToPlayMenu::draw_content(SDL_Renderer *renderer, int width, int height,
                                 float scale_factor, int content_top,
                                 int content_bottom) const {
    (void)height;
    if (!renderer || content_top >= content_bottom)
        return;

    SDL_Color white{255, 255, 255, 255};
    SDL_Color green{0, 255, 0, 255};
    SDL_Color yellow{255, 255, 0, 255};
    SDL_Color red{255, 0, 0, 255};
    SDL_Color grey{160, 160, 160, 255};

    int margin = static_cast<int>(80 * scale_factor);
    if (margin < 20)
        margin = 20;
    int text_right = width - margin;
    if (text_right <= margin)
        text_right = width;

    int text_scale = std::max(1, static_cast<int>(3 * scale_factor));
    int line_height = 7 * text_scale + text_scale;
    int small_gap = std::max(text_scale, line_height / 6);

    int y = content_top;

    auto clamp_y = [&]() {
        if (y > content_bottom)
            y = content_bottom;
    };

    auto draw_wrapped = [&](const std::string &text, int start_x, int indent_x,
                            SDL_Color color) {
        if (y >= content_bottom)
            return;
        std::istringstream stream(text);
        std::string word;
        std::string line;
        int line_x = start_x;
        int max_x = text_right;
        if (max_x <= start_x)
            max_x = start_x + CustomCharacter::text_width("W", text_scale);
        while (stream >> word) {
            std::string candidate = line.empty() ? word : line + " " + word;
            int width_px = CustomCharacter::text_width(candidate, text_scale);
            if (line_x + width_px > max_x && !line.empty()) {
                CustomCharacter::draw_text(renderer, line, line_x, y, color, text_scale);
                y += line_height;
                if (y >= content_bottom)
                    return;
                line = word;
                line_x = indent_x;
            } else {
                line = candidate;
            }
        }
        if (!line.empty() && y < content_bottom) {
            CustomCharacter::draw_text(renderer, line, line_x, y, color, text_scale);
        }
    };

    auto next_line = [&]() {
        y += line_height;
        clamp_y();
    };

    auto add_gap = [&](int gap) {
        y += gap;
        clamp_y();
    };

    auto draw_paragraph = [&](const std::string &text) {
        draw_wrapped(text, margin, margin, white);
        next_line();
        add_gap(small_gap);
    };

    auto draw_bullet = [&](const std::string &label, SDL_Color color,
                           const std::string &description) {
        if (y >= content_bottom)
            return;
        int x = margin;
        int bullet_width = CustomCharacter::text_width("- ", text_scale);
        CustomCharacter::draw_text(renderer, "- ", x, y, white, text_scale);
        x += bullet_width;
        CustomCharacter::draw_text(renderer, label, x, y, color, text_scale);
        x += CustomCharacter::text_width(label, text_scale);
        int space_width = CustomCharacter::text_width(" ", text_scale);
        CustomCharacter::draw_text(renderer, " ", x, y, white, text_scale);
        x += space_width;
        draw_wrapped(description, x, x, white);
        next_line();
        add_gap(small_gap / 2);
    };

    draw_paragraph(
        "Use the available objects to steer the laser beam from the white source sphere "
        "to the black target sphere.");
    draw_paragraph(
        "Earn points by illuminating object surfaces; the total score must reach each "
        "level's quota.");

    draw_wrapped("Object rules:", margin, margin, white);
    next_line();
    add_gap(small_gap / 2);

    draw_bullet("Green", green, "objects can both move and rotate.");
    draw_bullet("Yellow", yellow, "objects rotate but can't move.");
    draw_bullet("Red", red, "objects stay put - you can't move or rotate them.");
    draw_bullet("Grey", grey, "objects just block the beam; they don't score points.");

    add_gap(small_gap);

    draw_paragraph(
        "Remember that the laser fades with distance and stops once it reaches its set "
        "length.");
    draw_paragraph(
        "Hit the target and reach the quota to clear the level, or optimize your setup to "
        "climb the leaderboard.");
}
