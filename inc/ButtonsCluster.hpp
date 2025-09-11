#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include "Button.hpp"
#include "CustomCharacter.hpp"

// Cluster of buttons acting as a switch group
class ButtonsCluster {
private:
    std::vector<Button> buttons;
    int selected;

public:
    explicit ButtonsCluster(const std::vector<std::string> &names);

    // Arrange buttons within given rectangle
    void layout(int x, int y, int width, int height);

    // Handle mouse clicks
    void handle_event(const SDL_Event &e);

    // Draw buttons
    void draw(SDL_Renderer *renderer, int scale) const;

    int get_selected() const { return selected; }
};
