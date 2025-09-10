#pragma once
#include <SDL.h>
#include <string>
#include <vector>

class ButtonsCluster {
private:
    std::vector<std::string> texts;
    std::vector<SDL_Rect> rects;
    int selected;

public:
    ButtonsCluster(const std::vector<std::string> &labels, int default_index = 0);
    void set_position(int x, int y, int button_width, int button_height, int gap);
    bool handle_event(const SDL_Event &e);
    void draw(SDL_Renderer *renderer, int scale) const;
    int get_selected() const { return selected; }
};
