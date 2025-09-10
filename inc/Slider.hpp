#pragma once
#include <SDL.h>
#include <vector>
#include <string>

class Slider {
public:
    std::string label;
    std::vector<std::string> values;
    int current = 0;
    SDL_Rect bar{0,0,0,0};
    bool dragging = false;

    Slider() = default;
    Slider(const std::string &l, const std::vector<std::string> &vals)
        : label(l), values(vals) {}

    void set_bar(int x, int y, int w, int h);
    void handle_event(const SDL_Event &e);
    void draw(SDL_Renderer *renderer, int scale) const;
    std::string current_value() const { return values.empty() ? std::string() : values[current]; }
};
