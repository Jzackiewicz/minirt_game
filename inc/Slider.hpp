#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include "CustomCharacter.hpp"

// Slider widget allowing selection among discrete values.
class Slider {
public:
    std::string label;
    std::vector<std::string> values;
    int index; // currently selected value index
    SDL_Rect rect; // slider track rectangle
    bool dragging;

    Slider(const std::string &label, const std::vector<std::string> &vals, int default_index = 0);

    // Update slider rect
    void set_rect(int x, int y, int w, int h) { rect = {x, y, w, h}; }

    // Handle SDL events to update slider position
    void handle_event(const SDL_Event &e);

    // Render slider with ticks and handle; value displayed to right
    void render(SDL_Renderer *renderer, int scale) const;

    std::string current_value() const { return values[index]; }
};

