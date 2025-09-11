#pragma once
#include <SDL.h>
#include <string>
#include <vector>

// Generic slider control selecting one of a set of values.
class Slider {
    std::vector<std::string> values; // possible values to choose
    int selected;                    // currently selected index
    SDL_Rect bar_rect{};             // rectangle of the slider bar
    bool dragging;                   // true while handle is dragged

    void update_from_mouse(int mx);

public:
    Slider(const std::vector<std::string> &vals, int default_index);

    // Arrange the slider within the given rectangle
    void layout(int x, int y, int width, int height, int scale);

    // Process mouse events
    void handle_event(const SDL_Event &event);

    // Draw slider bar, handle and current value
    void draw(SDL_Renderer *renderer, int scale) const;

    const std::string &current_value() const { return values[selected]; }
};

