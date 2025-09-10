#pragma once
#include <SDL.h>
#include <string>
#include <vector>

class ButtonsCluster {
public:
    struct Item {
        std::string text;
        SDL_Rect rect{0,0,0,0};
    };

    std::vector<Item> items;
    int selected = 0;

    void add(const std::string &text);
    void set_layout(int x, int y, int total_w, int h, int gap);
    void handle_event(const SDL_Event &e);
    void draw(SDL_Renderer *renderer, int scale) const;
    std::string current_text() const;
};
