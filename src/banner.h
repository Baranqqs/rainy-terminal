#pragma once

#include <random>
#include <string>
#include <vector>

#include "terminal.h"

// The animated title that is drawn over the rain when the program starts.
class Banner {
public:
    // Picks the largest logo that fits the window and centres it.
    void build(int screenWidth, int screenHeight, std::mt19937& random);

    // Draws the logo over the cells. `seconds` is the time since the program
    // started. With `dissolved` the intro jumps straight to the dissolve.
    void paint(std::vector<Cell>& cells, int width, int height, double seconds,
               bool keepForever, bool dissolved) const;

    // Plain text version of the layout, used by --dump-banner.
    std::string describe() const;

private:
    enum class Style {
        Title,     // the big lettering: gradient, shine and wipe
        Rule,      // the thin line under the logo
        Subtitle,  // the line with the version
        Hint       // "press esc ..."
    };

    struct Glyph {
        char32_t character = U' ';
        Style style = Style::Title;
        float seed = 0.0f;  // 0..1, decides when the glyph dissolves
    };

    struct Row {
        int left = 0;      // column offset inside the logo box
        float shade = 0;   // 0 = top row of its block, 1 = bottom
        std::vector<Glyph> glyphs;
    };

    void addRow(std::u32string_view text, Style style, float shade);
    void buildLetterBanner(int screenWidth, bool large);
    void buildTextBanner(int screenWidth, int screenHeight);
    void centre(int screenWidth, int screenHeight);

    std::vector<Row> rows_;
    int left_ = 0;
    int top_ = 0;
    int width_ = 0;
};
