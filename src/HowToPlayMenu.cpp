#include "HowToPlayMenu.hpp"

#include "CustomCharacter.hpp"

#include <SDL.h>
#include <algorithm>
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

    struct Segment {
        std::string text;
        SDL_Color color;
        bool line_break;
    };

    struct Paragraph {
        bool bullet;
        bool separator;
        int gap_before;
        int custom_height;
        std::vector<Segment> segments;
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
        int total_height{};
        bool valid{false};
        std::vector<Paragraph> paragraphs;
        std::vector<int> line_counts;
        std::vector<int> entry_heights;
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
        int separator_gap = std::max(data.line_height / 3, static_cast<int>(12 * scale_factor));
        int post_separator_gap = std::max(data.line_height / 4, static_cast<int>(8 * scale_factor));
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

        std::vector<Paragraph> paragraphs;

        auto add_separator = [&](int gap_before) {
            Paragraph separator_entry{};
            separator_entry.bullet = false;
            separator_entry.separator = true;
            separator_entry.gap_before = gap_before;
            separator_entry.custom_height = std::max(content_scale * 2, 3);
            paragraphs.push_back(std::move(separator_entry));
        };

        Paragraph intro{};
        intro.bullet = false;
        intro.separator = false;
        intro.gap_before = 0;
        intro.custom_height = -1;
        intro.segments = {
            {"Use the available objects to steer the laser beam from the white source sphere to the black target sphere.",
             white, false},
            {"", white, true},
            {"Earn points by illuminating object surfaces; the total score must reach each level's quota.", white,
             false}
        };
        paragraphs.push_back(std::move(intro));

        add_separator(medium_gap);

        Paragraph rules_header{};
        rules_header.bullet = false;
        rules_header.separator = false;
        rules_header.gap_before = post_separator_gap;
        rules_header.custom_height = -1;
        rules_header.segments = {{"Object rules:", white, false}};
        paragraphs.push_back(std::move(rules_header));

        add_separator(separator_gap);

        Paragraph green_rule{};
        green_rule.bullet = true;
        green_rule.separator = false;
        green_rule.gap_before = post_separator_gap;
        green_rule.custom_height = -1;
        green_rule.segments = {{"Green", green, false}, {" objects can both move and rotate.", white, false}};
        paragraphs.push_back(std::move(green_rule));

        add_separator(small_gap);

        Paragraph yellow_rule{};
        yellow_rule.bullet = true;
        yellow_rule.separator = false;
        yellow_rule.gap_before = post_separator_gap;
        yellow_rule.custom_height = -1;
        yellow_rule.segments = {{"Yellow", yellow, false}, {" objects rotate but can't move.", white, false}};
        paragraphs.push_back(std::move(yellow_rule));

        add_separator(small_gap);

        Paragraph red_rule{};
        red_rule.bullet = true;
        red_rule.separator = false;
        red_rule.gap_before = post_separator_gap;
        red_rule.custom_height = -1;
        red_rule.segments = {{"Red", red, false},
                              {" objects stay put - you can't move or rotate them.", white, false}};
        paragraphs.push_back(std::move(red_rule));

        add_separator(small_gap);

        Paragraph grey_rule{};
        grey_rule.bullet = true;
        grey_rule.separator = false;
        grey_rule.gap_before = post_separator_gap;
        grey_rule.custom_height = -1;
        grey_rule.segments = {{"Grey", grey, false},
                              {" objects just block the beam; they don't score points.", white, false}};
        paragraphs.push_back(std::move(grey_rule));

        add_separator(separator_gap);

        Paragraph aim_tip{};
        aim_tip.bullet = false;
        aim_tip.separator = false;
        aim_tip.gap_before = post_separator_gap;
        aim_tip.custom_height = -1;
        aim_tip.segments = {{"Aim at any object to display information about it.", white, false}};
        paragraphs.push_back(std::move(aim_tip));

        add_separator(separator_gap);

        Paragraph fade_tip{};
        fade_tip.bullet = false;
        fade_tip.separator = false;
        fade_tip.gap_before = post_separator_gap;
        fade_tip.custom_height = -1;
        fade_tip.segments = {{"Remember that the laser fades with distance and stops once it reaches its set length.",
                               white, false}};
        paragraphs.push_back(std::move(fade_tip));

        add_separator(separator_gap);

        Paragraph quota_tip{};
        quota_tip.bullet = false;
        quota_tip.separator = false;
        quota_tip.gap_before = post_separator_gap;
        quota_tip.custom_height = -1;
        quota_tip.segments = {{"Hit the target and reach the quota to clear the level, or optimize your setup to climb the leaderboard.",
                                white, false}};
        paragraphs.push_back(std::move(quota_tip));

        auto count_lines = [&](const Paragraph &paragraph) {
            if (paragraph.separator)
                return 1;
            int indent = paragraph.bullet ? data.bullet_indent : 0;
            int usable_width = std::max(data.available_width - indent, 1);
            int x_offset = indent;
            bool need_space = false;
            int lines = 1;

            for (const auto &segment : paragraph.segments) {
                if (segment.line_break) {
                    ++lines;
                    x_offset = indent;
                    need_space = false;
                    continue;
                }
                std::istringstream iss(segment.text);
                std::string word;
                while (iss >> word) {
                    int word_width = CustomCharacter::text_width(word, content_scale);
                    if (word_width > usable_width)
                        word_width = usable_width;
                    int required = word_width + (need_space ? data.space_width : 0);
                    if (x_offset + required > data.available_width) {
                        ++lines;
                        x_offset = indent;
                        need_space = false;
                    } else if (need_space) {
                        x_offset += data.space_width;
                    }
                    x_offset += word_width;
                    need_space = true;
                }
            }

            return lines;
        };

        int total_height = 0;
        std::vector<int> line_counts;
        std::vector<int> entry_heights;
        line_counts.reserve(paragraphs.size());
        entry_heights.reserve(paragraphs.size());
        for (const auto &paragraph : paragraphs) {
            total_height += paragraph.gap_before;
            int lines = count_lines(paragraph);
            int entry_height = paragraph.custom_height >= 0 ? paragraph.custom_height : lines * data.line_height;
            total_height += entry_height;
            line_counts.push_back(lines);
            entry_heights.push_back(entry_height);
        }

        data.available_height = data.content_bottom - data.content_top;
        data.total_height = total_height;
        data.paragraphs = std::move(paragraphs);
        data.line_counts = std::move(line_counts);
        data.entry_heights = std::move(entry_heights);
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
        y += paragraph.gap_before;
        int lines = index < layout.line_counts.size() ? layout.line_counts[index] : 0;
        int entry_height = index < layout.entry_heights.size() ? layout.entry_heights[index] : 0;
        if (lines <= 0 && !paragraph.separator) {
            if (slots > 0)
                y += take_slot_extra();
            continue;
        }
        if (paragraph.separator) {
            if (y + entry_height > layout.content_bottom)
                return;
            Uint8 prev_r{}, prev_g{}, prev_b{}, prev_a{};
            SDL_GetRenderDrawColor(renderer, &prev_r, &prev_g, &prev_b, &prev_a);
            SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, white.a);
            SDL_Rect bar{layout.margin, y, layout.available_width, std::max(entry_height, 1)};
            SDL_RenderFillRect(renderer, &bar);
            SDL_SetRenderDrawColor(renderer, prev_r, prev_g, prev_b, prev_a);
            y += entry_height;
            if (slots > 0)
                y += take_slot_extra();
            continue;
        }
        if (y + entry_height > layout.content_bottom)
            return;

        int indent = paragraph.bullet ? layout.bullet_indent : 0;
        int usable_width = std::max(layout.available_width - indent, 1);
        int x_offset = indent;
        bool need_space = false;
        bool first_line = true;

        auto start_line = [&]() {
            if (paragraph.bullet && first_line) {
                CustomCharacter::draw_text(renderer, "- ", layout.margin, y, white,
                                           layout.content_scale);
            }
            x_offset = indent;
            need_space = false;
        };

        start_line();

        for (const auto &segment : paragraph.segments) {
            if (segment.line_break) {
                y += layout.line_height;
                if (y + layout.line_height > layout.content_bottom)
                    return;
                first_line = false;
                start_line();
                continue;
            }
            std::istringstream iss(segment.text);
            std::string word;
            while (iss >> word) {
                int word_width = CustomCharacter::text_width(word, layout.content_scale);
                if (word_width > usable_width)
                    word_width = usable_width;
                int required = word_width + (need_space ? layout.space_width : 0);
                if (x_offset + required > layout.available_width) {
                    y += layout.line_height;
                    if (y + layout.line_height > layout.content_bottom)
                        return;
                    first_line = false;
                    start_line();
                } else if (need_space) {
                    x_offset += layout.space_width;
                }

                CustomCharacter::draw_text(renderer, word, layout.margin + x_offset, y,
                                           segment.color, layout.content_scale);
                x_offset += word_width;
                need_space = true;
            }
        }

        y += layout.line_height;
        if (slots > 0)
            y += take_slot_extra();
    }
}
