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
    std::vector<SDL_Color> title_colors;

    // Hooks for derived menus to add custom drawings
    virtual int extra_height(int scale) const { return 0; }
    virtual void draw_extra(SDL_Renderer *renderer, int width, int y, int scale) {}

public:
    explicit AMenu(const std::string &t);
    virtual ~AMenu() = default;

    // Run the menu loop and return the action corresponding to the selected button
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
