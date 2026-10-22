#include <conio.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "banner/banner.h"
#include "rain/rain.h"
#include "terminal/startup.h"
#include "terminal/terminal.h"
#include "version/version.h"

namespace {

std::atomic<bool> g_running{true};

// Windows calls this when the user presses Ctrl+C or closes the window.
BOOL WINAPI onConsoleEvent(DWORD event) {
    switch (event) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_running.store(false, std::memory_order_relaxed);
        return TRUE;
    default:
        return FALSE;
    }
}

// ---------------------------------------------------------------------------
// Command line
// ---------------------------------------------------------------------------

struct Options {
    bool showBanner = true;
    bool keepBanner = false;
    bool help = false;
    bool version = false;
    bool dumpBanner = false;
    bool startup = false;   // started by cmd.exe through AutoRun
    bool install = false;
    bool uninstall = false;
    bool invalid = false;
    int dumpWidth = 80;
    int dumpHeight = 40;
};

void printHelp() {
    std::printf("%s %s\n", kAppName, kAppVersion);
    std::printf("Rain falls in your Windows console.\n\n");
    std::printf("usage: RainyTerminal [options]\n\n");
    std::printf("  --install             start with every new cmd.exe window\n");
    std::printf("  --uninstall           undo --install\n");
    std::printf("  --banner=hold         keep the logo on screen\n");
    std::printf("  --banner=once         logo at start, then it dissolves (default)\n");
    std::printf("  --no-banner           do not show the logo\n");
    std::printf("  --dump-banner[=WxH]   print the logo layout and exit\n");
    std::printf("  -h, --help            show this help\n");
    std::printf("  -v, --version         show the version\n\n");
    std::printf("Press ESC or Ctrl+C to quit.\n");
}

Options parseOptions(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);

        if (argument == "--install") {
            options.install = true;
        } else if (argument == "--uninstall") {
            options.uninstall = true;
        } else if (argument == "--startup") {
            options.startup = true;
        } else if (argument == "--no-banner") {
            options.showBanner = false;
        } else if (argument == "--banner" || argument == "--banner=hold") {
            options.showBanner = true;
            options.keepBanner = true;
        } else if (argument == "--banner=once") {
            options.showBanner = true;
            options.keepBanner = false;
        } else if (argument.rfind("--dump-banner", 0) == 0) {
            options.dumpBanner = true;
            const std::size_t equals = argument.find('=');
            if (equals != std::string_view::npos) {
                const std::string size(argument.substr(equals + 1));
                int width = 0;
                int height = 0;
                if (std::sscanf(size.c_str(), "%dx%d", &width, &height) == 2) {
                    options.dumpWidth = std::max(8, width);
                    options.dumpHeight = std::max(3, height);
                }
            }
        } else if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--version" || argument == "-v") {
            options.version = true;
        } else if (!argument.empty() && argument.front() == '-') {
            options.invalid = true;
        }
    }

    return options;
}

int runInstall() {
    std::string message;
    if (!startup::install(message)) {
        std::fprintf(stderr, "%s\n", message.c_str());
        return 1;
    }

    std::printf("%s\n\n", message.c_str());
    std::printf("Open a new cmd.exe window to see the rain.\n");
    std::printf("Press ESC there to get the normal prompt back.\n");
    std::printf("Run \"RainyTerminal.exe --uninstall\" to remove it again.\n");
    return 0;
}

int runUninstall() {
    std::string message;
    if (!startup::uninstall(message)) {
        std::fprintf(stderr, "%s\n", message.c_str());
        return 1;
    }

    std::printf("%s\n", message.c_str());
    return 0;
}

void printBanner(int width, int height) {
    std::mt19937 random(1234u);
    Banner banner;
    banner.build(width, height, random);
    std::fputs(banner.describe().c_str(), stdout);
}

// ---------------------------------------------------------------------------
// Animation loop
// ---------------------------------------------------------------------------

void run(const ConsoleSession& console, const Options& options) {
    std::mt19937 random(std::random_device{}());

    Banner banner;
    std::vector<Drop> drops;
    std::vector<Cell> cells;
    Viewport viewport;

    const auto start = std::chrono::steady_clock::now();
    auto previousFrame = start;
    bool dismissed = false;

    while (g_running.load(std::memory_order_relaxed)) {
        Viewport current;
        if (!console.readViewport(current)) {
            break;
        }

        if (current.width < 1 || current.height < 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            previousFrame = std::chrono::steady_clock::now();
            continue;
        }

        // Rebuild the scene whenever the window size changes.
        if (current.width != viewport.width || current.height != viewport.height) {
            viewport = current;
            cells.assign(static_cast<std::size_t>(viewport.width) * static_cast<std::size_t>(viewport.height),
                         Cell{});
            createRain(drops, viewport.width, viewport.height, random);
            banner.build(viewport.width, viewport.height, random);
            console.clear();
            previousFrame = std::chrono::steady_clock::now();
        }

        // ESC and Ctrl+C quit, any other key dismisses the banner.
        while (_kbhit() != 0) {
            const int key = _getch();
            if (key == 27 || key == 3) {
                g_running.store(false, std::memory_order_relaxed);
                break;
            }
            dismissed = true;
        }
        if (!g_running.load(std::memory_order_relaxed)) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(now - previousFrame).count();
        previousFrame = now;
        delta = std::min(delta, 0.1f);

        updateRain(drops, viewport.width, viewport.height, delta, random);
        paintRain(cells, drops, viewport.width, viewport.height);

        if (options.showBanner) {
            const double seconds = std::chrono::duration<double>(now - start).count();
            banner.paint(cells, viewport.width, viewport.height, seconds, options.keepBanner, dismissed);
        }

        if (!writeText(console.handle(), composeFrame(cells, viewport))) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

}  // namespace

int main(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);

    if (options.invalid) {
        std::fprintf(stderr, "Unknown option. Run --help to see the options.\n");
        return 2;
    }
    if (options.help) {
        printHelp();
        return 0;
    }
    if (options.version) {
        std::printf("%s %s\n", kAppName, kAppVersion);
        return 0;
    }
    if (options.install) {
        return runInstall();
    }
    if (options.uninstall) {
        return runUninstall();
    }
    if (options.dumpBanner) {
        printBanner(options.dumpWidth, options.dumpHeight);
        return 0;
    }

    ConsoleSession console;
    if (!console.ready()) {
        if (!options.startup) {
            std::fprintf(stderr, "Rainy Terminal needs a real Windows console.\n");
        }
        return options.startup ? 0 : 1;
    }

    if (SetConsoleCtrlHandler(onConsoleEvent, TRUE) == 0) {
        return 1;
    }

    run(console, options);

    SetConsoleCtrlHandler(onConsoleEvent, FALSE);
    return 0;
}
