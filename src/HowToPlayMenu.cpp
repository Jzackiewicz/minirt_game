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
    SDL_Color separator_color{96, 96, 96, 255};

    float scale_factor = static_cast<float>(height) / 600.0f;
    int initial_content_scale = std::max(1, static_cast<int>(3 * scale_factor));

    struct Segment {
        std::string text;
        SDL_Color color;
        bool line_break;
    };

    enum class ParagraphAlign { Left, Center };

    struct Paragraph {
        bool bullet;
        bool separator;
        int gap_before;
        int custom_height;
        ParagraphAlign alignment;
        std::vector<Segment> segments;
    };

    struct WordRun {
        std::string word;
        SDL_Color color;
    };

    struct WrappedLine {
        std::vector<WordRun> words;
        int width{0};
        bool blank{true};
        bool first_line{false};
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
        std::vector<int> entry_heights;
        std::vector<std::vector<WrappedLine>> lines_per_paragraph;
    };

    auto build_layout = [&](int content_scale) -> LayoutData {
        LayoutData data{};
        if (content_scale <= 0)
            return data;

        data.content_scale = content_scale;
        data.line_height = 8 * content_scale;

        int margin = std::max(static_cast<int>(width * 0.08f), content_scale * 6);
        int medium_gap = std::max(data.line_height / 2, static_cast<int>(16 * scale_factor));
        int separator_gap = std::max(data.line_height / 3, static_cast<int>(12 * scale_factor));
        int post_separator_gap = std::max(data.line_height / 4, static_cast<int>(8 * scale_factor));
        int bullet_gap = std::max(data.line_height / 4, static_cast<int>(6 * scale_factor));
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
            separator_entry.custom_height = std::max(std::min(content_scale, 2), 1);
            separator_entry.alignment = ParagraphAlign::Left;
            paragraphs.push_back(std::move(separator_entry));
        };

        Paragraph intro{};
        intro.bullet = false;
        intro.separator = false;
        intro.gap_before = 0;
        intro.custom_height = -1;
        intro.alignment = ParagraphAlign::Left;
        intro.segments = {
            {"Use the available objects to steer the laser beam from the white source sphere to the black target sphere.",
             white, false},
            {"", white, true},
            {"Earn points by illuminating object surfaces; the total score must reach each level’s quota.", white,
             false}
        };
        paragraphs.push_back(std::move(intro));

        add_separator(medium_gap);

        Paragraph rules_header{};
        rules_header.bullet = false;
        rules_header.separator = false;
        rules_header.gap_before = post_separator_gap;
        rules_header.custom_height = -1;
        rules_header.alignment = ParagraphAlign::Left;
        rules_header.segments = {{"Object rules: (bullet points)", white, false}};
        paragraphs.push_back(std::move(rules_header));

        Paragraph green_rule{};
        green_rule.bullet = true;
        green_rule.separator = false;
        green_rule.gap_before = post_separator_gap;
        green_rule.custom_height = -1;
        green_rule.alignment = ParagraphAlign::Left;
        green_rule.segments = {{"Green", green, false}, {" objects can both move and rotate.", white, false}};
        paragraphs.push_back(std::move(green_rule));

        Paragraph yellow_rule{};
        yellow_rule.bullet = true;
        yellow_rule.separator = false;
        yellow_rule.gap_before = bullet_gap;
        yellow_rule.custom_height = -1;
        yellow_rule.alignment = ParagraphAlign::Left;
        yellow_rule.segments = {{"Yellow", yellow, false}, {" objects rotate but can’t move.", white, false}};
        paragraphs.push_back(std::move(yellow_rule));

        Paragraph red_rule{};
        red_rule.bullet = true;
        red_rule.separator = false;
        red_rule.gap_before = bullet_gap;
        red_rule.custom_height = -1;
        red_rule.alignment = ParagraphAlign::Left;
        red_rule.segments = {{"Red", red, false},
                              {" objects stay put - you can’t move or rotate them.", white, false}};
        paragraphs.push_back(std::move(red_rule));

        Paragraph grey_rule{};
        grey_rule.bullet = true;
        grey_rule.separator = false;
        grey_rule.gap_before = bullet_gap;
        grey_rule.custom_height = -1;
        grey_rule.alignment = ParagraphAlign::Left;
        grey_rule.segments = {{"Grey", grey, false},
                              {" objects just block the beam; they don’t score points.", white, false}};
        paragraphs.push_back(std::move(grey_rule));

        Paragraph aim_tip{};
        aim_tip.bullet = false;
        aim_tip.separator = false;
        aim_tip.gap_before = post_separator_gap;
        aim_tip.custom_height = -1;
        aim_tip.alignment = ParagraphAlign::Center;
        aim_tip.segments = {{"Aim at any object to display information about it.", white, false}};
        paragraphs.push_back(std::move(aim_tip));

        add_separator(separator_gap);

        Paragraph fade_tip{};
        fade_tip.bullet = false;
        fade_tip.separator = false;
        fade_tip.gap_before = post_separator_gap;
        fade_tip.custom_height = -1;
        fade_tip.alignment = ParagraphAlign::Left;
        fade_tip.segments = {{"Remember that the laser fades with distance and stops once it reaches its set length.",
                               white, false}};
        paragraphs.push_back(std::move(fade_tip));

        Paragraph quota_tip{};
        quota_tip.bullet = false;
        quota_tip.separator = false;
        quota_tip.gap_before = post_separator_gap;
        quota_tip.custom_height = -1;
        quota_tip.alignment = ParagraphAlign::Left;
        quota_tip.segments = {{"Hit the target and reach the quota to clear the level, or optimize your setup to climb the leaderboard.",
                                white, false}};
        paragraphs.push_back(std::move(quota_tip));

        data.paragraphs = std::move(paragraphs);

        auto wrap_paragraph = [&](const Paragraph &paragraph) {
            std::vector<WrappedLine> lines;
            if (paragraph.separator)
                return lines;

            int indent = paragraph.bullet ? data.bullet_indent : 0;
            int usable_width = std::max(data.available_width - indent, 1);
            bool first_line_flag = true;
            std::vector<WordRun> words;
            int current_width = 0;
            bool has_words = false;

            auto flush_line = [&](bool allow_blank) {
                if (!has_words && !allow_blank)
                    return;
                WrappedLine line{};
                line.words = std::move(words);
                line.width = current_width;
                line.blank = !has_words;
                line.first_line = first_line_flag;
                lines.push_back(std::move(line));
                words = {};
                current_width = 0;
                has_words = false;
                first_line_flag = false;
            };

            for (const auto &segment : paragraph.segments) {
                if (segment.line_break) {
                    flush_line(true);
                    continue;
                }
                std::istringstream iss(segment.text);
                std::string word;
                while (iss >> word) {
                    int word_width = CustomCharacter::text_width(word, data.content_scale);
                    if (word_width > usable_width)
                        word_width = usable_width;
                    int required = word_width + (has_words ? data.space_width : 0);
                    if (has_words && current_width + required > usable_width) {
                        flush_line(false);
                    }
                    if (has_words)
                        current_width += data.space_width;
                    words.push_back(WordRun{word, segment.color});
                    current_width += word_width;
                    has_words = true;
                }
            }

            flush_line(false);

            if (lines.empty()) {
                WrappedLine blank{};
                blank.blank = true;
                blank.first_line = true;
                lines.push_back(std::move(blank));
            }

            return lines;
        };

        int total_height = 0;
        std::vector<int> entry_heights;
        std::vector<std::vector<WrappedLine>> lines_per_paragraph;
        entry_heights.reserve(data.paragraphs.size());
        lines_per_paragraph.reserve(data.paragraphs.size());

        for (const auto &paragraph : data.paragraphs) {
            total_height += paragraph.gap_before;
            if (paragraph.separator) {
                int entry_height = paragraph.custom_height >= 0 ? paragraph.custom_height : std::max(content_scale, 1);
                total_height += entry_height;
                entry_heights.push_back(entry_height);
                lines_per_paragraph.emplace_back();
            } else {
                auto lines = wrap_paragraph(paragraph);
                int entry_height = static_cast<int>(lines.size()) * data.line_height;
                if (entry_height <= 0)
                    entry_height = data.line_height;
                total_height += entry_height;
                entry_heights.push_back(entry_height);
                lines_per_paragraph.push_back(std::move(lines));
            }
        }

        data.available_height = data.content_bottom - data.content_top;
        data.total_height = total_height;
        data.entry_heights = std::move(entry_heights);
        data.lines_per_paragraph = std::move(lines_per_paragraph);
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
        int entry_height = index < layout.entry_heights.size() ? layout.entry_heights[index] : 0;
        if (paragraph.separator) {
            if (y + entry_height > layout.content_bottom)
                return;
            Uint8 prev_r{}, prev_g{}, prev_b{}, prev_a{};
            SDL_GetRenderDrawColor(renderer, &prev_r, &prev_g, &prev_b, &prev_a);
            SDL_SetRenderDrawColor(renderer, separator_color.r, separator_color.g, separator_color.b,
                                   separator_color.a);
            SDL_Rect bar{layout.margin, y, layout.available_width, std::max(entry_height, 1)};
            SDL_RenderFillRect(renderer, &bar);
            SDL_SetRenderDrawColor(renderer, prev_r, prev_g, prev_b, prev_a);
            y += entry_height;
            if (slots > 0)
                y += take_slot_extra();
            continue;
        }

        if (index >= layout.lines_per_paragraph.size())
            return;

        if (y + entry_height > layout.content_bottom)
            return;

        const auto &lines = layout.lines_per_paragraph[index];
        int indent = paragraph.bullet ? layout.bullet_indent : 0;
        int line_y = y;

        for (const auto &line : lines) {
            if (line_y + layout.line_height > layout.content_bottom)
                return;
            if (!line.blank) {
                if (paragraph.bullet && line.first_line) {
                    CustomCharacter::draw_text(renderer, "- ", layout.margin, line_y, white,
                                               layout.content_scale);
                }

                int start_x = layout.margin + indent;
                if (paragraph.alignment == ParagraphAlign::Center) {
                    int centered_x = layout.margin + (layout.available_width - line.width) / 2;
                    if (centered_x < layout.margin)
                        centered_x = layout.margin;
                    start_x = centered_x;
                }

                int x = start_x;
                bool first_word = true;
                for (const auto &word : line.words) {
                    if (!first_word)
                        x += layout.space_width;
                    CustomCharacter::draw_text(renderer, word.word, x, line_y, word.color,
                                               layout.content_scale);
                    x += CustomCharacter::text_width(word.word, layout.content_scale);
                    first_word = false;
                }
            }
            line_y += layout.line_height;
        }

        y = line_y;
        if (slots > 0)
            y += take_slot_extra();
    }
}
