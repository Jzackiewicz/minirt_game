#include "Slider.hpp"
#include "CustomCharacter.hpp"
#include <cmath>

Slider::Slider(const std::string &lbl, const std::vector<std::string> &vals,
               int default_index)
    : label(lbl), values(vals), index(default_index), track{0, 0, 0, 0},
      knob_w(0), knob_h(0), dragging(false) {}

void Slider::set_position(int x, int y, int width, int scale) {
    track = {x, y, width, scale};
    knob_w = scale * 2;
    knob_h = scale * 4;
}

bool Slider::handle_event(const SDL_Event &e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        SDL_Rect kb = {track.x + (track.w - knob_w) * index / (int(values.size()) - 1),
                       track.y + track.h / 2 - knob_h / 2, knob_w, knob_h};
        SDL_Rect expanded = {track.x, track.y - knob_h / 2, track.w, knob_h};
        if ((mx >= kb.x && mx < kb.x + kb.w && my >= kb.y && my < kb.y + kb.h) ||
            (mx >= expanded.x && mx < expanded.x + expanded.w &&
             my >= expanded.y && my < expanded.y + expanded.h)) {
            dragging = true;
            // fall through to motion update
        } else {
            return false;
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    }
    if (e.type == SDL_MOUSEMOTION) {
        if (dragging) {
            int mx = e.motion.x;
            double pos = (mx - track.x) / static_cast<double>(track.w);
            if (pos < 0.0)
                pos = 0.0;
            if (pos > 1.0)
                pos = 1.0;
            int count = static_cast<int>(values.size()) - 1;
            index = static_cast<int>(std::round(pos * count));
        }
    }
    return dragging;
}

void Slider::draw(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    // label
    CustomCharacter::draw_text(renderer, label, track.x,
                               track.y - 10 * scale - 7 * scale, white, scale);
    // track
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &track);
    // ticks
    if (values.size() > 1) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            int tx = track.x + (track.w - 1) * i / (int(values.size()) - 1);
            SDL_RenderDrawLine(renderer, tx, track.y, tx, track.y + track.h);
        }
    }
    // knob
    SDL_Rect kb = {track.x + (track.w - knob_w) * index / (int(values.size()) - 1),
                   track.y + track.h / 2 - knob_h / 2, knob_w, knob_h};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &kb);

    // value
    int val_x = track.x + track.w + 10 * scale;
    int val_y = track.y + track.h / 2 - (7 * scale) / 2;
    CustomCharacter::draw_text(renderer, values[index], val_x, val_y, white, scale);
}
