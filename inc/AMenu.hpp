#pragma once
#include <SDL.h>
#include <string>
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

public:
    explicit AMenu(const std::string &t);
    virtual ~AMenu() = default;

    // Run the menu loop and return the action corresponding to the selected button
    // If transparent is true, the current renderer contents are preserved and
    // a semi-transparent overlay is drawn behind the menu.
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent = false);
};
