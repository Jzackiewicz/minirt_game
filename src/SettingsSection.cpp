#include "SettingsSection.hpp"

SettingsSection::SettingsSection(const std::string &l)
    : label(l), area{0, 0, 0, 0} {}

void SettingsSection::set_area(int x, int y, int w, int h) {
    area = SDL_Rect{x, y, w, h};
}

void SettingsSection::layout(int) {}

void SettingsSection::handle_event(const SDL_Event &) {}

QualitySection::QualitySection()
    : SettingsSection("QUALITY"), cluster({"LOW", "MEDIUM", "HIGH"}) {}

void QualitySection::layout(int scale) {
    int label_height = 7 * scale;
    int content_y = area.y + label_height + 10 * scale;
    int content_height = area.h - (content_y - area.y);
    cluster.layout(area.x, content_y, area.w, content_height);
}

void QualitySection::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_x = area.x + (area.w - label_width) / 2;
    CustomCharacter::draw_text(renderer, label, label_x, area.y, white, scale);
    cluster.render(renderer, scale);
}

void QualitySection::handle_event(const SDL_Event &event) {
    cluster.handle_event(event);
}

MouseSensitivitySection::MouseSensitivitySection()
    : SettingsSection("MOUSE SENSITIVITY") {}

void MouseSensitivitySection::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_x = area.x + (area.w - label_width) / 2;
    CustomCharacter::draw_text(renderer, label, label_x, area.y, white, scale);
    int label_height = 7 * scale;
    SDL_Rect placeholder{area.x, area.y + label_height + 10 * scale, area.w,
                         area.h - label_height - 10 * scale};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &placeholder);
}

ResolutionSection::ResolutionSection() : SettingsSection("RESOLUTION") {}

void ResolutionSection::render(SDL_Renderer *renderer, int scale) const {
    SDL_Color white{255, 255, 255, 255};
    int label_width = CustomCharacter::text_width(label, scale);
    int label_x = area.x + (area.w - label_width) / 2;
    CustomCharacter::draw_text(renderer, label, label_x, area.y, white, scale);
    int label_height = 7 * scale;
    SDL_Rect placeholder{area.x, area.y + label_height + 10 * scale, area.w,
                         area.h - label_height - 10 * scale};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &placeholder);
}

