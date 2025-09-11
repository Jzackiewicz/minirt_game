#pragma once
#include <SDL.h>
#include <vector>
#include "Button.hpp"

// Cluster of toggle buttons placed horizontally
class ButtonsCluster {
    std::vector<Button> buttons;
    std::vector<SDL_Rect> rects;
    int selected;

public:
    ButtonsCluster() = default;
    explicit ButtonsCluster(std::vector<Button> btns);
    void handle_event(const SDL_Event &event);
    void draw(SDL_Renderer *renderer, int x, int y, int width, int scale);
};
