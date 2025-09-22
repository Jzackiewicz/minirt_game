#pragma once

#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu displaying how to play information
class HowToPlayMenu : public AMenu {
public:
    HowToPlayMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent = false);

protected:
    void draw_content(SDL_Renderer *renderer, int width, int height, int scale,
                      int title_scale, int title_x, int title_y, int title_height,
                      int title_gap, int buttons_start_y) override;
};
