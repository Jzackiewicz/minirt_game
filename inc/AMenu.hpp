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

public:
    explicit AMenu(const std::string &t);
    virtual ~AMenu() = default;

    // Run the menu loop and return the action corresponding to the selected button
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
