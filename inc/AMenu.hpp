#pragma once
#include <SDL.h>
#include <string>
#include <utility>
#include <vector>
#include "Button.hpp"
#include "CustomCharacter.hpp"

// Abstract base class for all menus
class AMenu {
protected:
    std::string title;
    std::vector<Button> buttons;
    std::vector<Button> corner_buttons;
    std::vector<SDL_Color> title_colors;
    bool buttons_align_bottom;
    int buttons_bottom_margin;
    int title_top_margin;

    virtual int button_rows() const;
    virtual void button_metrics(float scale_factor, int &button_width,
                                int &button_height, int &button_gap);
    virtual void layout_buttons(std::vector<Button> &buttons, int width, int height,
                                float scale_factor, int button_width, int button_height,
                                int button_gap, int start_y, int center_x);
    void layout_two_column(std::vector<Button> &buttons, int width, int start_y,
                           const std::vector<std::pair<int, int>> &row_widths,
                           int button_height, int vertical_gap, int column_gap);
    virtual void draw_content(SDL_Renderer *renderer, int width, int height, int scale,
                              int title_scale, int title_x, int title_y, int title_height,
                              int title_gap, int buttons_start_y);

public:
    explicit AMenu(const std::string &t);
    virtual ~AMenu() = default;

    // Run the menu loop and return the action corresponding to the selected button
    // If transparent is true, the current renderer contents are preserved and
    // a semi-transparent overlay is drawn behind the menu.
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent = false);
};
