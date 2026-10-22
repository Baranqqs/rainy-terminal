# Rainy Terminal

Rain falls in your Windows console. An animated logo starts the show, then
dissolves into the rain.

```text
           ████    ████  ██████ ██   ██ ██  ██
           ██ ██  ██  ██   ██   ███  ██ ██  ██
           ████   ██████   ██   ████ ██  ████
           ██ ██  ██  ██   ██   ██ ████   ██
           ██  ██ ██  ██ ██████ ██  ███   ██

██████ ██████ ████   ██   ██ ██████ ██   ██  ████  ██
  ██   ██     ██ ██  ███ ███   ██   ███  ██ ██  ██ ██
  ██   ████   ████   ███████   ██   ████ ██ ██████ ██
  ██   ██     ██ ██  ██ █ ██   ██   ██ ████ ██  ██ ██
  ██   ██████ ██  ██ ██   ██ ██████ ██  ███ ██  ██ ██████

─────────────────────────────────────────────────────────
             animated console rain · v1.0.0
               press esc or ctrl+c to quit
```

## Run

Open a terminal in the project folder and type:

```powershell
.\RainyTerminal.exe
```

Press `Esc` or `Ctrl+C` to quit. The logo pops up, shines, and after a few
seconds it dissolves and only the rain is left.

## Options

| Option | What it does |
| --- | --- |
| `--banner=hold` | keep the logo on screen |
| `--no-banner` | skip the logo |
| `-h`, `--help` | show all options |
| `-v`, `--version` | show the version |

## Build

You need [MinGW-w64](https://www.mingw-w64.org/) with g++:

```sh
g++ -std=c++17 -O2 src/*.cpp -o RainyTerminal.exe
```

On Windows you can also just double-click `build.bat`.

## Code

| File | What it does |
| --- | --- |
| `src/main.cpp` | options and the main loop |
| `src/terminal.cpp` | colours, cells, console setup, drawing |
| `src/rain.cpp` | the rain drops |
| `src/banner.cpp` | the logo and its animation |

The logo is plain block letters, the rain is a list of drops that move down
every frame, and each frame is written to the console in one go.

## License

MIT. See [LICENSE](LICENSE).
