#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include "Button.hpp"
#include "CustomCharacter.hpp"

// Group of buttons where only one button can be active at a time.
class ButtonsCluster {
public:
    std::vector<Button> buttons;
    int active; // index of currently active button
    ButtonsCluster(const std::vector<std::string> &texts, int default_active = 0);

    // Set rectangles for buttons placed horizontally starting at (x,y)
    void set_layout(int x, int y, int button_width, int button_height, int gap);

    // Handle mouse button events to change active button
    void handle_event(const SDL_Event &e);

    // Render the buttons cluster
    void render(SDL_Renderer *renderer, int scale) const;

    int active_index() const { return active; }
    std::string active_text() const { return buttons[active].text; }
};

