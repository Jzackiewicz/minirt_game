#include "SettingsSection.hpp"

QualitySection::QualitySection()
    : SettingsSection("QUALITY"),
      cluster(std::vector<std::string>{"LOW", "MEDIUM", "HIGH"}),
      btn_height(20) {}

int QualitySection::layout(int center_x, int y, int width, int scale) {
    btn_height = static_cast<int>(20 * scale);
    int label_height = 7 * scale;
    int gap = 5 * scale;
    int content_y = y + label_height + gap;
    cluster.layout(center_x - width / 2, content_y, width, btn_height);
    return content_y + btn_height;
}

int QualitySection::draw(SDL_Renderer *renderer, int center_x, int y, int width,
                         int scale) {
    int label_w = CustomCharacter::text_width(label, scale);
    SDL_Color white{255, 255, 255, 255};
    CustomCharacter::draw_text(renderer, label, center_x - label_w / 2, y, white,
                               scale);
    int label_height = 7 * scale;
    int gap = 5 * scale;
    int content_y = y + label_height + gap;
    cluster.draw(renderer, scale);
    return content_y + btn_height;
}

MouseSensitivitySection::MouseSensitivitySection()
    : SettingsSection("MOUSE SENSITIVITY") {}

int MouseSensitivitySection::draw(SDL_Renderer *renderer, int center_x, int y,
                                  int width, int scale) {
    int label_w = CustomCharacter::text_width(label, scale);
    SDL_Color white{255, 255, 255, 255};
    CustomCharacter::draw_text(renderer, label, center_x - label_w / 2, y, white,
                               scale);
    int label_height = 7 * scale;
    int gap = 5 * scale;
    SDL_Rect placeholder{center_x - width / 2, y + label_height + gap, width,
                         static_cast<int>(20 * scale)};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &placeholder);
    return placeholder.y + placeholder.h;
}

ResolutionSection::ResolutionSection() : SettingsSection("RESOLUTION") {}

int ResolutionSection::draw(SDL_Renderer *renderer, int center_x, int y, int width,
                            int scale) {
    int label_w = CustomCharacter::text_width(label, scale);
    SDL_Color white{255, 255, 255, 255};
    CustomCharacter::draw_text(renderer, label, center_x - label_w / 2, y, white,
                               scale);
    int label_height = 7 * scale;
    int gap = 5 * scale;
    SDL_Rect placeholder{center_x - width / 2, y + label_height + gap, width,
                         static_cast<int>(20 * scale)};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &placeholder);
    return placeholder.y + placeholder.h;
}
