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
             animated console rain · v1.1.0
               press esc or ctrl+c to quit
```

## Run

Open a terminal in the project folder and type:

```powershell
.\RainyTerminal.exe
```

Press `Esc` or `Ctrl+C` to quit. The logo pops up, shines, and after a few
seconds it dissolves and only the rain is left.

## Start with cmd.exe

Want the rain every time you open a normal `cmd.exe` window? Run this once:

```powershell
.\RainyTerminal.exe --install
```

Now every new command prompt opens with the rain and the logo.
Press `Esc` to get your prompt back.

To remove it again:

```powershell
.\RainyTerminal.exe --uninstall
```

The setting is stored in `HKCU\Software\Microsoft\Command Processor\AutoRun`
(no admin rights needed). Scripts that call `cmd /c ...` are skipped, so
builds and other tools are never blocked by the animation.

If you move the folder, run `--install` again from the new location. The old
entry is replaced automatically.

## Options

| Option | What it does |
| --- | --- |
| `--install` | start with every new cmd.exe window |
| `--uninstall` | undo `--install` |
| `--banner=hold` | keep the logo on screen |
| `--no-banner` | skip the logo |
| `-h`, `--help` | show all options |
| `-v`, `--version` | show the version |

## Build

You need [MinGW-w64](https://www.mingw-w64.org/) with g++:

```sh
g++ -std=c++17 -O2 src/main.cpp src/banner/banner.cpp src/rain/rain.cpp src/terminal/terminal.cpp src/terminal/startup.cpp -o RainyTerminal.exe
```

On Windows you can also just double-click `build.bat`.

## Code

| Folder | What it does |
| --- | --- |
| `src/main.cpp` | options and the main loop |
| `src/banner/` | the logo and its animation |
| `src/rain/` | the rain drops |
| `src/terminal/` | colours, cells, console setup, drawing |
| `src/terminal/startup.cpp` | cmd.exe AutoRun integration |
| `src/version/` | name and version |

The logo is plain block letters, the rain is a list of drops that move down
every frame, and each frame is written to the console in one go.

## License

MIT. See [LICENSE](LICENSE).
