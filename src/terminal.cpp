#include "terminal.h"

#include <algorithm>
#include <string>

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------

const char* ansiColor(Color color) {
    static const char* const codes[] = {
        "\x1b[39m",   // Default
        "\x1b[36m",   // Cyan
        "\x1b[96m",   // BrightCyan
        "\x1b[32m",   // Green
        "\x1b[92m",   // BrightGreen
        "\x1b[90m",   // Gray
        "\x1b[97m",   // BrightWhite
        "\x1b[2;32m"  // DimGreen
    };
    return codes[static_cast<std::size_t>(color)];
}

Color brighter(Color color) {
    switch (color) {
    case Color::Cyan: return Color::BrightCyan;
    case Color::Green: return Color::BrightGreen;
    default: return color;
    }
}

// ---------------------------------------------------------------------------
// Text conversion
// ---------------------------------------------------------------------------

void appendUtf8(std::string& text, char32_t codePoint) {
    if (codePoint < 0x80) {
        text += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        text += static_cast<char>(0xC0 | (codePoint >> 6));
        text += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        text += static_cast<char>(0xE0 | (codePoint >> 12));
        text += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        text += static_cast<char>(0xF0 | (codePoint >> 18));
        text += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        text += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

std::u32string toUtf32(std::string_view text) {
    std::u32string result;
    result.reserve(text.size());
    for (const char character : text) {
        result.push_back(static_cast<char32_t>(static_cast<unsigned char>(character)));
    }
    return result;
}

bool writeText(HANDLE console, std::string_view text) {
    if (text.empty()) {
        return true;
    }

    const int length = static_cast<int>(text.size());
    const int needed = MultiByteToWideChar(CP_UTF8, 0, text.data(), length, nullptr, 0);
    if (needed <= 0) {
        return false;
    }

    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, text.data(), length, wide.data(), needed) != needed) {
        return false;
    }

    const wchar_t* cursor = wide.c_str();
    std::size_t remaining = wide.size();
    while (remaining > 0) {
        const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(remaining, 4096));
        DWORD written = 0;
        if (WriteConsoleW(console, cursor, chunk, &written, nullptr) == 0 || written == 0) {
            return false;
        }
        cursor += written;
        remaining -= written;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Console session
// ---------------------------------------------------------------------------

ConsoleSession::ConsoleSession() {
    output_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output_ == nullptr || output_ == INVALID_HANDLE_VALUE) {
        return;
    }

    if (GetConsoleMode(output_, &originalMode_) == 0) {
        return;
    }
    modeSaved_ = true;

    const DWORD mode = originalMode_ | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(output_, mode) == 0) {
        return;
    }

    if (GetConsoleCursorInfo(output_, &originalCursor_) != 0) {
        cursorSaved_ = true;
        CONSOLE_CURSOR_INFO hidden = originalCursor_;
        hidden.bVisible = FALSE;
        SetConsoleCursorInfo(output_, &hidden);
    }

    // Alternate screen buffer: the rain never touches the user's scrollback.
    if (!writeText(output_, "\x1b[?1049h\x1b[40m\x1b[2J\x1b[H\x1b[?25l")) {
        return;
    }

    ready_ = true;
}

ConsoleSession::~ConsoleSession() {
    if (output_ == nullptr || output_ == INVALID_HANDLE_VALUE) {
        return;
    }

    if (ready_) {
        writeText(output_, "\x1b[39m\x1b[49m\x1b[?25h\x1b[?1049l");
    }
    if (cursorSaved_) {
        SetConsoleCursorInfo(output_, &originalCursor_);
    }
    if (modeSaved_) {
        SetConsoleMode(output_, originalMode_);
    }
}

bool ConsoleSession::readViewport(Viewport& viewport) const {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(output_, &info) == 0) {
        return false;
    }

    viewport.width = info.srWindow.Right - info.srWindow.Left + 1;
    viewport.height = info.srWindow.Bottom - info.srWindow.Top + 1;
    viewport.top = info.srWindow.Top + 1;
    viewport.left = info.srWindow.Left + 1;
    return true;
}

void ConsoleSession::clear() const {
    writeText(output_, "\x1b[40m\x1b[2J");
}

// ---------------------------------------------------------------------------
// Frame building
// ---------------------------------------------------------------------------

std::string composeFrame(const std::vector<Cell>& cells, const Viewport& viewport) {
    std::string frame;
    frame.reserve(cells.size() + static_cast<std::size_t>(viewport.height) * 16U + 32U);
    frame += "\x1b[40m";  // black background

    for (int row = 0; row < viewport.height; ++row) {
        // Every row is addressed explicitly, so the frame also lands correctly
        // when the screen buffer is larger than the window.
        frame += "\x1b[";
        frame += std::to_string(viewport.top + row);
        frame += ';';
        frame += std::to_string(viewport.left);
        frame += 'H';

        Color activeColor = Color::Default;
        const std::size_t rowStart = static_cast<std::size_t>(row) * static_cast<std::size_t>(viewport.width);
        for (int column = 0; column < viewport.width; ++column) {
            const Cell& cell = cells[rowStart + static_cast<std::size_t>(column)];
            if (cell.color != activeColor) {
                frame += ansiColor(cell.color);
                activeColor = cell.color;
            }
            appendUtf8(frame, cell.glyph);
        }

        if (activeColor != Color::Default) {
            frame += ansiColor(Color::Default);
        }
    }

    return frame;
}
