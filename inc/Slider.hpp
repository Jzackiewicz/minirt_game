#pragma once
#include <SDL.h>
#include <string>
#include <vector>

class Slider {
    std::string label;
    std::vector<std::string> values;
    int index;
    SDL_Rect bar;
    bool dragging;
public:
    Slider(const std::string &label, const std::vector<std::string> &vals, int start = 0);
    void set_rect(int x, int y, int w, int h);
    void handle_event(const SDL_Event &e);
    void render(SDL_Renderer *renderer, int scale) const;
    const std::string &current() const { return values[index]; }
    int current_index() const { return index; }
};
