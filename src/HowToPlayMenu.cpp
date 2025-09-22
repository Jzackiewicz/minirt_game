#include "HowToPlayMenu.hpp"

#include "CustomCharacter.hpp"

#include <SDL.h>
#include <algorithm>
#include <utility>
#include <vector>

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

void HowToPlayMenu::draw_content(SDL_Renderer *renderer, int width, int height, int scale,
                                 int title_scale, int title_x, int title_y, int title_height,
                                 int title_gap, int buttons_start_y) {
    (void)title_scale;
    (void)title_x;
    (void)title_gap;

    SDL_Color white{255, 255, 255, 255};
    SDL_Color green{0, 255, 0, 255};
    SDL_Color yellow{255, 255, 0, 255};
    SDL_Color red{255, 0, 0, 255};
    SDL_Color grey{192, 192, 192, 255};

    int content_scale = std::max(1, scale - 1);
    int margin = std::max(width / 10, content_scale * 6);
    int line_height = 8 * content_scale;
    int y = title_y + title_height + std::max(20, content_scale * 4);

    auto draw_segments = [&](const std::vector<std::pair<std::string, SDL_Color>> &segments) {
        int x = margin;
        for (const auto &segment : segments) {
            CustomCharacter::draw_text(renderer, segment.first, x, y, segment.second,
                                       content_scale);
            x += CustomCharacter::text_width(segment.first, content_scale);
        }
        y += line_height;
    };

    auto draw_bullet = [&](const std::vector<std::pair<std::string, SDL_Color>> &segments) {
        std::vector<std::pair<std::string, SDL_Color>> bullet_segments = segments;
        bullet_segments.insert(bullet_segments.begin(), {"- ", white});
        draw_segments(bullet_segments);
    };

    draw_segments({{"Use the available objects to steer the laser beam", white}});
    draw_segments({{"from the white source sphere to the black target sphere.", white}});
    draw_segments({{"Earn points by illuminating object surfaces; the total score must reach",
                    white}});
    draw_segments({{"each level's quota.", white}});

    y += line_height / 2;
    draw_segments({{"Object rules:", white}});
    draw_bullet({{"Green", green}, {" objects can both move and rotate.", white}});
    draw_bullet({{"Yellow", yellow}, {" objects rotate but can't move.", white}});
    draw_bullet({{"Red", red}, {" objects stay put - you can't move or rotate them.", white}});
    draw_bullet({{"Grey", grey}, {" objects just block the beam; they don't score points.", white}});

    y += line_height / 2;
    draw_segments({{"Remember that the laser fades with distance and stops once it reaches", white}});
    draw_segments({{"its set length.", white}});
    draw_segments({{"Hit the target and reach the quota to clear the level, or optimize your",
                    white}});
    draw_segments({{"setup to climb the leaderboard.", white}});

    (void)height;
    (void)buttons_start_y;
}
