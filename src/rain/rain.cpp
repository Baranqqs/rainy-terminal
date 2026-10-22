#include "rain.h"

#include <algorithm>
#include <cmath>

namespace {

// Creates a single drop. Spread across the screen when the scene is first
// built, otherwise above the top edge so it falls in after a short delay.
Drop makeDrop(int width, int height, std::mt19937& random, bool spread) {
    std::uniform_int_distribution<int> length(4, 12);
    std::uniform_real_distribution<float> speed(10.0f, 30.0f);
    std::uniform_real_distribution<float> column(0.0f, static_cast<float>(width));
    std::uniform_int_distribution<int> tint(0, 1);

    Drop drop;
    drop.length = length(random);
    drop.column = std::floor(column(random));
    drop.speed = speed(random);
    drop.tint = tint(random) == 0 ? Color::Cyan : Color::Green;

    if (spread) {
        std::uniform_real_distribution<float> row(-static_cast<float>(drop.length),
                                                  static_cast<float>(height));
        drop.row = row(random);
    } else {
        std::uniform_real_distribution<float> delay(0.0f, 12.0f);
        drop.row = -static_cast<float>(drop.length) - delay(random);
    }

    return drop;
}

}  // namespace

void createRain(std::vector<Drop>& drops, int width, int height, std::mt19937& random) {
    const std::size_t area = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const std::size_t count = std::clamp<std::size_t>(area / 55U, 8U, 500U);

    drops.clear();
    drops.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        drops.push_back(makeDrop(width, height, random, true));
    }
}

void updateRain(std::vector<Drop>& drops, int width, int height, float seconds, std::mt19937& random) {
    const float bottom = static_cast<float>(height);

    for (Drop& drop : drops) {
        drop.row += drop.speed * seconds;
        if (drop.row - static_cast<float>(drop.length) >= bottom) {
            drop = makeDrop(width, height, random, false);
        }
    }
}

void paintRain(std::vector<Cell>& cells, const std::vector<Drop>& drops, int width, int height) {
    std::fill(cells.begin(), cells.end(), Cell{});

    for (const Drop& drop : drops) {
        const int column = static_cast<int>(drop.column);
        if (column < 0 || column >= width) {
            continue;
        }

        const int headRow = static_cast<int>(std::floor(drop.row));
        const Color headColor = drop.tint == Color::Cyan ? Color::BrightCyan : Color::BrightGreen;

        for (int offset = 0; offset < drop.length; ++offset) {
            const int row = headRow - offset;
            if (row < 0 || row >= height) {
                continue;
            }

            const char32_t glyph = offset <= drop.length / 2 ? U'|' : U':';
            const Color color = offset == 0 ? headColor : drop.tint;
            const std::size_t index = static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
                                      static_cast<std::size_t>(column);
            cells[index] = Cell{glyph, color};
        }
    }
}
