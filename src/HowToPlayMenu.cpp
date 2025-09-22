#include "HowToPlayMenu.hpp"

#include "CustomCharacter.hpp"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>
#include <vector>

HowToPlayMenu::HowToPlayMenu() : AMenu("HOW TO PLAY") {
    title_colors = {SDL_Color{255, 255, 255, 255}};
    buttons_align_bottom = true;
    title_top_margin = 60;
    buttons_bottom_margin = 40;
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255, 0, 0, 255}});
}

void HowToPlayMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                         bool transparent) {
    HowToPlayMenu menu;
    menu.run(window, renderer, width, height, transparent);
}

void HowToPlayMenu::draw_content(SDL_Renderer *renderer, int width, int height, int scale,
                                 int title_scale, int title_x, int title_y, int title_height,
                                 int title_gap, int buttons_start_y) {
    (void)scale;
    (void)title_scale;
    (void)title_x;
    (void)title_gap;

    SDL_Color white{255, 255, 255, 255};
    SDL_Color green{0, 255, 0, 255};
    SDL_Color yellow{255, 255, 0, 255};
    SDL_Color red{255, 0, 0, 255};
    SDL_Color grey{192, 192, 192, 255};

    float scale_factor = static_cast<float>(height) / 600.0f;
    int initial_content_scale = std::max(1, static_cast<int>(3 * scale_factor));

    struct Paragraph {
        bool bullet;
        int gap_before;
        std::vector<std::pair<std::string, SDL_Color>> segments;
    };

    struct LayoutData {
        int content_scale{};
        int margin{};
        int line_height{};
        int content_top{};
        int content_bottom{};
        int available_width{};
        int available_height{};
        int space_width{};
        int bullet_indent{};
        int separator_height{};
        int separator_gap{};
        int total_height{};
        bool valid{false};
        std::vector<Paragraph> paragraphs;
        std::vector<int> line_counts;
    };

    enum class TokenKind { Word, Space, Newline };

    auto visit_tokens = [&](const Paragraph &paragraph, auto &&callback) {
        for (const auto &segment : paragraph.segments) {
            const std::string &text = segment.first;
            SDL_Color color = segment.second;
            std::string word;
            bool previous_space = false;
            auto flush_word = [&]() {
                if (!word.empty()) {
                    callback(word, color, TokenKind::Word);
                    word.clear();
                }
            };

            for (char ch : text) {
                unsigned char uch = static_cast<unsigned char>(ch);
                if (ch == '\n') {
                    flush_word();
                    callback(std::string{}, color, TokenKind::Newline);
                    previous_space = false;
                } else if (std::isspace(uch)) {
                    flush_word();
                    if (!previous_space) {
                        callback(std::string{}, color, TokenKind::Space);
                        previous_space = true;
                    }
                } else {
                    word.push_back(ch);
                    previous_space = false;
                }
            }

            flush_word();
        }
    };

    auto build_layout = [&](int content_scale) -> LayoutData {
        LayoutData data{};
        if (content_scale <= 0)
            return data;

        data.content_scale = content_scale;
        data.line_height = 8 * content_scale;

        int margin = std::max(static_cast<int>(width * 0.08f), content_scale * 6);
        int medium_gap = std::max(data.line_height / 2, static_cast<int>(16 * scale_factor));
        int small_gap = std::max(data.line_height / 3, static_cast<int>(10 * scale_factor));
        int padding = std::max(medium_gap, data.line_height);

        data.content_top = title_y + title_height + padding;
        data.content_bottom = buttons_start_y - padding;
        if (data.content_bottom <= data.content_top)
            return data;

        data.margin = margin;
        data.available_width = width - 2 * margin;
        if (data.available_width <= 0)
            return data;

        data.space_width = CustomCharacter::text_width(" ", content_scale);
        data.bullet_indent = CustomCharacter::text_width("- ", content_scale);
        data.separator_height = std::max(2, content_scale);
        data.separator_gap = std::max(data.line_height / 3, static_cast<int>(12 * scale_factor));

        std::vector<Paragraph> paragraphs = {
            {false,
             0,
             {{"Use the available objects to steer the laser beam from the white source sphere to the black target sphere.\n"
               "Earn points by illuminating object surfaces; the total score must reach each level's quota.",
               white}}},
            {false, medium_gap, {{"Object rules:", white}}},
            {true, small_gap, {{"Green", green}, {" objects can both move and rotate.", white}}},
            {true, 0, {{"Yellow", yellow}, {" objects rotate but can't move.", white}}},
            {true, 0, {{"Red", red}, {" objects stay put - you can't move or rotate them.", white}}},
            {true, 0, {{"Grey", grey}, {" objects just block the beam; they don't score points.", white}}},
            {false, 0, {{"Aim at any object to display information about it.", white}}},
            {false, medium_gap,
             {{"Remember that the laser fades with distance and stops once it reaches its set length.",
               white}}},
            {false, small_gap,
             {{"Hit the target and reach the quota to clear the level, or optimize your setup to climb the leaderboard.",
               white}}}
        };

        auto count_lines = [&](const Paragraph &paragraph) {
            int indent = paragraph.bullet ? data.bullet_indent : 0;
            int usable_width = std::max(data.available_width - indent, 1);
            int x_offset = indent;
            bool need_space = false;
            bool have_word_on_line = false;
            int lines = 1;

            auto reset_line = [&]() {
                x_offset = indent;
                need_space = false;
                have_word_on_line = false;
            };

            visit_tokens(paragraph, [&](const std::string &token, SDL_Color, TokenKind kind) {
                if (kind == TokenKind::Newline) {
                    ++lines;
                    reset_line();
                    return;
                }

                if (kind == TokenKind::Space) {
                    if (have_word_on_line)
                        need_space = true;
                    return;
                }

                int word_width = CustomCharacter::text_width(token, content_scale);
                if (word_width > usable_width)
                    word_width = usable_width;
                int required = word_width + (need_space ? data.space_width : 0);
                if (x_offset + required > data.available_width) {
                    ++lines;
                    reset_line();
                } else if (need_space) {
                    x_offset += data.space_width;
                }
                x_offset += word_width;
                need_space = true;
                have_word_on_line = true;
            });

            return lines;
        };

        int total_height = 0;
        std::vector<int> line_counts;
        line_counts.reserve(paragraphs.size());
        for (std::size_t i = 0; i < paragraphs.size(); ++i) {
            const auto &paragraph = paragraphs[i];
            total_height += paragraph.gap_before;
            int lines = count_lines(paragraph);
            total_height += lines * data.line_height;
            if (i + 1 < paragraphs.size())
                total_height += 2 * data.separator_gap + data.separator_height;
            line_counts.push_back(lines);
        }

        data.available_height = data.content_bottom - data.content_top;
        data.total_height = total_height;
        data.paragraphs = std::move(paragraphs);
        data.line_counts = std::move(line_counts);
        data.valid = true;
        return data;
    };

    LayoutData layout = build_layout(initial_content_scale);
    if (!layout.valid) {
        for (int test_scale = initial_content_scale - 1; test_scale >= 1; --test_scale) {
            layout = build_layout(test_scale);
            if (layout.valid)
                break;
        }
    }

    if (layout.valid && layout.total_height > layout.available_height) {
        for (int test_scale = layout.content_scale - 1; test_scale >= 1; --test_scale) {
            LayoutData candidate = build_layout(test_scale);
            if (!candidate.valid)
                continue;
            layout = candidate;
            if (layout.total_height <= layout.available_height)
                break;
        }
    }

    if (layout.valid) {
        for (int test_scale = layout.content_scale + 1;; ++test_scale) {
            LayoutData candidate = build_layout(test_scale);
            if (!candidate.valid || candidate.total_height > candidate.available_height)
                break;
            layout = candidate;
        }
    }

    if (!layout.valid)
        return;

    int extra_space = layout.available_height - layout.total_height;
    int slots = static_cast<int>(layout.paragraphs.size()) + 1;
    int slot_base = slots > 0 ? extra_space / slots : 0;
    int slot_remainder = slots > 0 ? extra_space % slots : 0;
    auto take_slot_extra = [&]() {
        int extra = slot_base;
        if (slot_remainder > 0) {
            ++extra;
            --slot_remainder;
        }
        return extra;
    };

    int y = layout.content_top;
    if (slots > 0)
        y += take_slot_extra();

    for (std::size_t index = 0; index < layout.paragraphs.size(); ++index) {
        const auto &paragraph = layout.paragraphs[index];
        int lines = index < layout.line_counts.size() ? layout.line_counts[index] : 0;
        if (lines <= 0) {
            if (slots > 0)
                y += take_slot_extra();
            continue;
        }

        int required_height = paragraph.gap_before + lines * layout.line_height;
        if (index + 1 < layout.paragraphs.size())
            required_height += 2 * layout.separator_gap + layout.separator_height;
        if (y + required_height > layout.content_bottom)
            return;

        y += paragraph.gap_before;

        int indent = paragraph.bullet ? layout.bullet_indent : 0;
        int usable_width = std::max(layout.available_width - indent, 1);
        int x_offset = indent;
        bool need_space = false;
        bool have_word_on_line = false;
        bool first_line = true;
        bool overflow = false;

        auto start_line = [&]() {
            if (paragraph.bullet && first_line) {
                CustomCharacter::draw_text(renderer, "- ", layout.margin, y, white,
                                           layout.content_scale);
            }
            x_offset = indent;
            need_space = false;
            have_word_on_line = false;
        };

        start_line();

        visit_tokens(paragraph, [&](const std::string &token, SDL_Color color, TokenKind kind) {
            if (overflow)
                return;

            if (kind == TokenKind::Newline) {
                y += layout.line_height;
                if (y + layout.line_height > layout.content_bottom) {
                    overflow = true;
                    return;
                }
                first_line = false;
                start_line();
                return;
            }

            if (kind == TokenKind::Space) {
                if (have_word_on_line)
                    need_space = true;
                return;
            }

            int word_width = CustomCharacter::text_width(token, layout.content_scale);
            if (word_width > usable_width)
                word_width = usable_width;
            int required = word_width + (need_space ? layout.space_width : 0);
            if (x_offset + required > layout.available_width) {
                y += layout.line_height;
                if (y + layout.line_height > layout.content_bottom) {
                    overflow = true;
                    return;
                }
                first_line = false;
                start_line();
            } else if (need_space) {
                x_offset += layout.space_width;
            }

            CustomCharacter::draw_text(renderer, token, layout.margin + x_offset, y, color,
                                       layout.content_scale);
            x_offset += word_width;
            need_space = true;
            have_word_on_line = true;
        });

        if (overflow)
            return;

        y += layout.line_height;

        if (index + 1 < layout.paragraphs.size()) {
            y += layout.separator_gap;
            if (y + layout.separator_height > layout.content_bottom)
                return;
            SDL_Rect separator{layout.margin, y, layout.available_width, layout.separator_height};
            SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, white.a);
            SDL_RenderFillRect(renderer, &separator);
            y += layout.separator_height;
            y += layout.separator_gap;
        }

        if (slots > 0)
            y += take_slot_extra();
    }
}
