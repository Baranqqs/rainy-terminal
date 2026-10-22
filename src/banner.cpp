#include "banner.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "version.h"

namespace {

constexpr int kLetterRows = 5;

// The intro animation, in seconds.
constexpr double kWipeSeconds = 0.9;
constexpr double kHoldSeconds = 4.2;
constexpr double kDissolveSeconds = 1.6;
constexpr double kShineSeconds = 3.4;

// One letter: five rows, one code point per screen cell.
using Letter = std::array<std::u32string_view, kLetterRows>;

// Wide lettering for large windows.
const Letter& bigLetter(char32_t character) {
    static const Letter blank = {U"      ", U"      ", U"      ", U"      ", U"      "};
    static const Letter r = {U"████  ", U"██ ██ ", U"████  ", U"██ ██ ", U"██  ██"};
    static const Letter a = {U" ████ ", U"██  ██", U"██████", U"██  ██", U"██  ██"};
    static const Letter i = {U"██████", U"  ██  ", U"  ██  ", U"  ██  ", U"██████"};
    static const Letter n = {U"██   ██", U"███  ██", U"████ ██", U"██ ████", U"██  ███"};
    static const Letter y = {U"██  ██", U"██  ██", U" ████ ", U"  ██  ", U"  ██  "};
    static const Letter t = {U"██████", U"  ██  ", U"  ██  ", U"  ██  ", U"  ██  "};
    static const Letter e = {U"██████", U"██    ", U"████  ", U"██    ", U"██████"};
    static const Letter m = {U"██   ██", U"███ ███", U"███████", U"██ █ ██", U"██   ██"};
    static const Letter l = {U"██    ", U"██    ", U"██    ", U"██    ", U"██████"};

    switch (character) {
    case U'R': return r;
    case U'A': return a;
    case U'I': return i;
    case U'N': return n;
    case U'Y': return y;
    case U'T': return t;
    case U'E': return e;
    case U'M': return m;
    case U'L': return l;
    default: return blank;
    }
}

// Narrow lettering for smaller windows.
const Letter& smallLetter(char32_t character) {
    static const Letter blank = {U"     ", U"     ", U"     ", U"     ", U"     "};
    static const Letter r = {U"████ ", U"█   █", U"████ ", U"█  █ ", U"█   █"};
    static const Letter a = {U" ███ ", U"█   █", U"█████", U"█   █", U"█   █"};
    static const Letter i = {U"█████", U"  █  ", U"  █  ", U"  █  ", U"█████"};
    static const Letter n = {U"█   █", U"██  █", U"█ █ █", U"█  ██", U"█   █"};
    static const Letter y = {U"█   █", U"█   █", U" ███ ", U"  █  ", U"  █  "};
    static const Letter t = {U"█████", U"  █  ", U"  █  ", U"  █  ", U"  █  "};
    static const Letter e = {U"█████", U"█    ", U"████ ", U"█    ", U"█████"};
    static const Letter m = {U"█   █", U"██ ██", U"█ █ █", U"█   █", U"█   █"};
    static const Letter l = {U"█    ", U"█    ", U"█    ", U"█    ", U"█████"};

    switch (character) {
    case U'R': return r;
    case U'A': return a;
    case U'I': return i;
    case U'N': return n;
    case U'Y': return y;
    case U'T': return t;
    case U'E': return e;
    case U'M': return m;
    case U'L': return l;
    default: return blank;
    }
}

// Joins single letters into the rows of a word.
std::array<std::u32string, kLetterRows> renderWord(std::u32string_view word, bool large) {
    std::array<std::u32string, kLetterRows> rows;

    for (std::size_t index = 0; index < word.size(); ++index) {
        const Letter& letter = large ? bigLetter(word[index]) : smallLetter(word[index]);
        for (int row = 0; row < kLetterRows; ++row) {
            if (index > 0) {
                rows[static_cast<std::size_t>(row)] += U' ';
            }
            rows[static_cast<std::size_t>(row)] += letter[static_cast<std::size_t>(row)];
        }
    }

    return rows;
}

// The logo fades from bright cyan on the left to bright green on the right.
// The lower rows of the lettering sit one step deeper in the palette.
Color titleColor(float gradient, float shade) {
    static const Color ramp[4] = {
        Color::BrightCyan, Color::Cyan, Color::Green, Color::BrightGreen
    };

    int index = static_cast<int>(gradient * 4.0f);
    index = std::clamp(index, 0, 3);
    Color color = ramp[index];

    if (shade > 0.55f) {
        if (color == Color::BrightCyan) {
            color = Color::Cyan;
        } else if (color == Color::BrightGreen) {
            color = Color::Green;
        }
    }
    return color;
}

// The hint is shortened until it fits the window.
std::u32string_view hintFor(int width) {
    if (width >= 29) {
        return U"press esc or ctrl+c to quit";
    }
    if (width >= 22) {
        return U"esc or ctrl+c quits";
    }
    if (width >= 14) {
        return U"esc quits";
    }
    return U"";
}

}  // namespace

// ---------------------------------------------------------------------------
// Building the layout
// ---------------------------------------------------------------------------

void Banner::build(int screenWidth, int screenHeight, std::mt19937& random) {
    rows_.clear();
    left_ = 0;
    top_ = 0;
    width_ = 0;

    if (screenWidth < 8 || screenHeight < 3) {
        return;
    }

    if (screenWidth >= 50 && screenHeight >= 14) {
        buildLetterBanner(screenWidth, screenWidth >= 58);
    } else {
        buildTextBanner(screenWidth, screenHeight);
    }

    centre(screenWidth, screenHeight);

    std::uniform_real_distribution<float> seed(0.0f, 1.0f);
    for (Row& row : rows_) {
        for (Glyph& glyph : row.glyphs) {
            glyph.seed = seed(random);
        }
    }
}

void Banner::buildLetterBanner(int screenWidth, bool large) {
    const auto rainy = renderWord(U"RAINY", large);
    const auto terminal = renderWord(U"TERMINAL", large);

    for (int row = 0; row < kLetterRows; ++row) {
        addRow(rainy[static_cast<std::size_t>(row)], Style::Title,
               static_cast<float>(row) / static_cast<float>(kLetterRows - 1));
    }
    for (int row = 0; row < kLetterRows; ++row) {
        addRow(terminal[static_cast<std::size_t>(row)], Style::Title,
               static_cast<float>(row) / static_cast<float>(kLetterRows - 1));
    }

    addRow(U"", Style::Title, 0.0f);  // empty line between logo and rule

    const int titleWidth = std::max(static_cast<int>(rainy[0].size()),
                                    static_cast<int>(terminal[0].size()));
    const int ruleWidth = std::min(titleWidth, screenWidth - 4);
    addRow(std::u32string(static_cast<std::size_t>(ruleWidth), U'─'), Style::Rule, 0.0f);

    std::u32string subtitle = U"animated console rain · v";
    subtitle += toUtf32(kAppVersion);
    addRow(subtitle, Style::Subtitle, 0.0f);

    addRow(U"press esc or ctrl+c to quit", Style::Hint, 0.0f);
}

void Banner::buildTextBanner(int screenWidth, int screenHeight) {
    const int ruleWidth = std::clamp(screenWidth - 4, 4, 40);
    addRow(std::u32string(static_cast<std::size_t>(ruleWidth), U'─'), Style::Rule, 0.0f);

    addRow(screenWidth >= 16 ? U"RAINY TERMINAL" : U"RAINY", Style::Title, 0.0f);

    if (screenHeight >= 4) {
        const std::u32string_view hint = hintFor(screenWidth);
        if (!hint.empty()) {
            addRow(hint, Style::Hint, 0.0f);
        }
    }
}

void Banner::addRow(std::u32string_view text, Style style, float shade) {
    Row row;
    row.shade = shade;
    row.glyphs.reserve(text.size());

    for (const char32_t character : text) {
        row.glyphs.push_back(Glyph{character, style, 0.0f});
    }

    rows_.push_back(std::move(row));
}

void Banner::centre(int screenWidth, int screenHeight) {
    for (const Row& row : rows_) {
        width_ = std::max(width_, static_cast<int>(row.glyphs.size()));
    }

    const int height = static_cast<int>(rows_.size());
    left_ = std::max(0, (screenWidth - width_) / 2);
    top_ = std::max(0, (screenHeight - height) / 2);

    for (Row& row : rows_) {
        row.left = std::max(0, (width_ - static_cast<int>(row.glyphs.size())) / 2);
    }
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

void Banner::paint(std::vector<Cell>& cells, int width, int height, double seconds,
                   bool keepForever, bool dissolved) const {
    if (rows_.empty()) {
        return;
    }

    if (dissolved) {
        seconds = std::max(seconds, kHoldSeconds);
    }

    // The wipe reveals the logo from left to right.
    const double wipe = std::clamp(seconds / kWipeSeconds, 0.0, 1.0);
    const double eased = 1.0 - std::pow(1.0 - wipe, 3.0);
    const double wipeColumn = -2.0 + eased * (static_cast<double>(width) + 6.0);

    // After the hold the glyphs disappear one by one.
    const bool fading = !keepForever && seconds > kHoldSeconds;
    const double dissolve = fading
        ? std::clamp((seconds - kHoldSeconds) / kDissolveSeconds, 0.0, 1.0)
        : 0.0;

    // A bright highlight travels across the lettering.
    const double shinePhase = std::fmod(seconds, kShineSeconds) / kShineSeconds;
    const double shineColumn = static_cast<double>(left_) - 8.0 +
                               shinePhase * (static_cast<double>(width_) + 16.0);

    for (std::size_t rowIndex = 0; rowIndex < rows_.size(); ++rowIndex) {
        const Row& row = rows_[rowIndex];
        const int screenRow = top_ + static_cast<int>(rowIndex);
        if (screenRow < 0 || screenRow >= height) {
            continue;
        }

        for (std::size_t index = 0; index < row.glyphs.size(); ++index) {
            const Glyph& glyph = row.glyphs[index];
            const int screenColumn = left_ + row.left + static_cast<int>(index);

            if (screenColumn < 0 || screenColumn >= width) {
                continue;
            }
            if (static_cast<double>(screenColumn) > wipeColumn) {
                continue;  // not revealed yet
            }
            if (fading && dissolve >= glyph.seed) {
                continue;  // already dissolved
            }
            // Spaces between the letters stay transparent so the rain shows
            // through the logo.
            if (glyph.style == Style::Title && glyph.character == U' ') {
                continue;
            }

            Color color = Color::Default;
            switch (glyph.style) {
            case Style::Title: {
                const float gradient = width_ > 1
                    ? static_cast<float>(row.left + static_cast<int>(index)) /
                      static_cast<float>(width_ - 1)
                    : 0.0f;
                color = titleColor(gradient, row.shade);

                const double distance = std::abs(shineColumn - static_cast<double>(screenColumn));
                if (distance < 1.2) {
                    color = Color::BrightWhite;
                } else if (distance < 4.0) {
                    color = brighter(color);
                }
                if (wipeColumn - static_cast<double>(screenColumn) < 1.5) {
                    color = Color::BrightWhite;  // glowing edge of the wipe
                }
                break;
            }
            case Style::Rule:
                color = Color::DimGreen;
                break;
            case Style::Subtitle:
                color = Color::BrightCyan;
                break;
            case Style::Hint:
                color = Color::Gray;
                break;
            }

            if (fading && glyph.seed - dissolve < 0.18) {
                color = Color::DimGreen;  // glyphs fade out just before they vanish
            }

            const std::size_t cell = static_cast<std::size_t>(screenRow) * static_cast<std::size_t>(width) +
                                     static_cast<std::size_t>(screenColumn);
            cells[cell] = Cell{glyph.character, color};
        }
    }
}

std::string Banner::describe() const {
    std::string text;

    for (const Row& row : rows_) {
        text.append(static_cast<std::size_t>(row.left), ' ');
        for (const Glyph& glyph : row.glyphs) {
            appendUtf8(text, glyph.character);
        }
        text += '\n';
    }

    return text;
}
