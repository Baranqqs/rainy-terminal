#pragma once

#include <random>
#include <vector>

#include "terminal.h"

// A falling rain streak.
struct Drop {
    float column = 0.0f;  // screen column of the streak
    float row = 0.0f;     // row of the streak's head (the bright end)
    float speed = 0.0f;   // rows per second
    int length = 0;       // number of characters in the streak
    Color tint = Color::Cyan;
};

// Fills the vector with drops spread over the whole screen.
void createRain(std::vector<Drop>& drops, int width, int height, std::mt19937& random);

// Moves every drop down and recycles the ones that left the screen.
void updateRain(std::vector<Drop>& drops, int width, int height, float seconds, std::mt19937& random);

// Clears the cell grid and draws every drop on it.
void paintRain(std::vector<Cell>& cells, const std::vector<Drop>& drops, int width, int height);
