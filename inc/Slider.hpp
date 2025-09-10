#pragma once
#include <SDL.h>
#include <string>
#include <vector>

class Slider {
private:
    std::string label;
    std::vector<std::string> values;
    int index;
    SDL_Rect track;
    int knob_w;
    int knob_h;
    bool dragging;

public:
    Slider(const std::string &label, const std::vector<std::string> &vals,
           int default_index = 0);
    void set_position(int x, int y, int width, int scale);
    bool handle_event(const SDL_Event &e);
    void draw(SDL_Renderer *renderer, int scale) const;
    int get_index() const { return index; }
    const std::string &get_value() const { return values[index]; }
};
