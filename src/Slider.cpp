#include "Slider.hpp"
#include "CustomCharacter.hpp"
#include <algorithm>

Slider::Slider(const std::string &lab, const std::vector<std::string> &vals, int start)
    : label(lab), values(vals), index(start), bar{0,0,100,10}, dragging(false) {}

void Slider::set_rect(int x, int y, int w, int h) {
    bar = {x, y, w, h};
}

void Slider::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (e.button.x >= bar.x && e.button.x <= bar.x + bar.w &&
            e.button.y >= bar.y - 5 && e.button.y <= bar.y + bar.h + 5) {
            dragging = true;
            double rel = double(e.button.x - bar.x) / double(bar.w);
            int idx = int(rel * (values.size() - 1) + 0.5);
            index = std::clamp(idx, 0, (int)values.size() - 1);
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (e.type == SDL_MOUSEMOTION && dragging) {
        double rel = double(e.motion.x - bar.x) / double(bar.w);
        int idx = int(rel * (values.size() - 1) + 0.5);
        index = std::clamp(idx, 0, (int)values.size() - 1);
    }
}

void Slider::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255,255,255,255};
    // label
    int label_width = CustomCharacter::text_width(label, scale);
    CustomCharacter::draw_text(renderer, label, bar.x + bar.w/2 - label_width/2,
                               bar.y - 20*scale, white, scale);
    // bar line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, bar.x, bar.y, bar.x + bar.w, bar.y);
    if (values.size() > 1) {
        double step = double(bar.w) / double(values.size() - 1);
        for (size_t i=0;i<values.size();++i) {
            int tx = bar.x + int(i*step);
            SDL_RenderDrawLine(renderer, tx, bar.y-5*scale, tx, bar.y+5*scale);
        }
    }
    // knob
    double step = values.size() > 1 ? double(bar.w)/(values.size()-1) : 0;
    int kx = bar.x + int(index * step);
    SDL_Rect knob{kx-3*scale, bar.y-5*scale, 6*scale, 10*scale};
    SDL_RenderFillRect(renderer, &knob);
    // value text to right
    const std::string &val = values[index];
    CustomCharacter::draw_text(renderer, val, bar.x + bar.w + 10*scale,
                               bar.y - 3*scale, white, scale);
}
