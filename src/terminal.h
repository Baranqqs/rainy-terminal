#pragma once

// Kept in one place so every translation unit builds the same console headers.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------

// The eight terminal colours used by the program.
enum class Color : unsigned char {
    Default,
    Cyan,
    BrightCyan,
    Green,
    BrightGreen,
    Gray,
    BrightWhite,
    DimGreen
};

// ANSI escape sequence that switches the text colour.
const char* ansiColor(Color color);

// Same hue, one step brighter (blue -> bright blue and so on).
Color brighter(Color color);

// ---------------------------------------------------------------------------
// Screen cells
// ---------------------------------------------------------------------------

// One character cell of the screen.
struct Cell {
    char32_t glyph = U' ';
    Color color = Color::Default;
};

// Appends one code point to a UTF-8 string.
void appendUtf8(std::string& text, char32_t codePoint);

// Turns a plain ASCII string into UTF-32 (one code point per cell).
std::u32string toUtf32(std::string_view text);

// Writes UTF-8 text to the console. The wide API keeps block characters and
// box drawing characters readable on every code page.
bool writeText(HANDLE console, std::string_view text);

// ---------------------------------------------------------------------------
// Console window
// ---------------------------------------------------------------------------

// The visible part of the console window.
struct Viewport {
    int width = 0;
    int height = 0;
    int top = 1;   // first visible row in the screen buffer (1 based)
    int left = 1;  // first visible column in the screen buffer (1 based)
};

// Sets the console up for the animation and restores it again on destruction:
// virtual terminal processing, hidden cursor and the alternate screen buffer.
class ConsoleSession {
public:
    ConsoleSession();
    ~ConsoleSession();

    ConsoleSession(const ConsoleSession&) = delete;
    ConsoleSession& operator=(const ConsoleSession&) = delete;

    bool ready() const { return ready_; }
    HANDLE handle() const { return output_; }

    // Reads the current window size and position.
    bool readViewport(Viewport& viewport) const;

    // Clears the screen to black.
    void clear() const;

private:
    HANDLE output_ = INVALID_HANDLE_VALUE;
    DWORD originalMode_ = 0;
    CONSOLE_CURSOR_INFO originalCursor_{};
    bool modeSaved_ = false;
    bool cursorSaved_ = false;
    bool ready_ = false;
};

// Builds one complete frame out of the cell grid.
std::string composeFrame(const std::vector<Cell>& cells, const Viewport& viewport);
