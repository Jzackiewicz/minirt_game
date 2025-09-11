#include "SettingsSection.hpp"

SettingsSection::SettingsSection(const std::string &l) : label(l) {}

void SettingsSection::draw(SDL_Renderer *renderer, int x, int y, int width, int scale) {
    SDL_Color white{255, 255, 255, 255};
    int text_x = x + (width - CustomCharacter::text_width(label, scale)) / 2;
    CustomCharacter::draw_text(renderer, label, text_x, y, white, scale);
}

QualitySection::QualitySection()
    : SettingsSection("QUALITY"),
      cluster({Button{"LOW", ButtonAction::None, {0, 0, 0, 0}},
               Button{"MEDIUM", ButtonAction::None, {0, 0, 0, 0}},
               Button{"HIGH", ButtonAction::None, {0, 0, 0, 0}}}) {}

void QualitySection::handle_event(const SDL_Event &event) { cluster.handle_event(event); }

void QualitySection::draw(SDL_Renderer *renderer, int x, int y, int width, int scale) {
    SettingsSection::draw(renderer, x, y, width, scale);
    int content_y = y + 7 * scale + 2 * scale;
    cluster.draw(renderer, x, content_y, width, scale);
}

MouseSensitivitySection::MouseSensitivitySection()
    : SettingsSection("MOUSE SENSITIVITY") {}

void MouseSensitivitySection::draw(SDL_Renderer *renderer, int x, int y, int width,
                                   int scale) {
    SettingsSection::draw(renderer, x, y, width, scale);
    int content_y = y + 7 * scale + 2 * scale;
    SDL_Rect rect{ x, content_y, width, 7 * scale };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

ResolutionSection::ResolutionSection() : SettingsSection("RESOLUTION") {}

void ResolutionSection::draw(SDL_Renderer *renderer, int x, int y, int width, int scale) {
    SettingsSection::draw(renderer, x, y, width, scale);
    int content_y = y + 7 * scale + 2 * scale;
    SDL_Rect rect{ x, content_y, width, 7 * scale };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
}
