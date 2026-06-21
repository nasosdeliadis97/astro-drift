# Astro Drift

**Astro Drift** is a small arcade space dodger game built with **C++17** and **SFML 3**.

The player controls a spaceship, avoids incoming asteroids and debris, collects power-ups, shoots obstacles, and tries to survive as long as possible while chasing a high score.

## Features

- Fullscreen gameplay with fixed 16:9 scaling
- Smooth spaceship movement
- Shooting system with laser projectiles
- Asteroids, rare asteroids, and rare spaceship debris
- Shield and rapid-fire power-ups
- Explosions, fragments, glow effects, and screen shake
- Main menu, ship selection, pause menu, and game over screen
- Sound effects and main menu music
- Persistent high score saving
- Optional sci-fi font support

## Controls

| Action | Key |
|---|---|
| Move left | `A` or `Left Arrow` |
| Move right | `D` or `Right Arrow` |
| Shoot | `Space` |
| Pause / Resume | `Esc` |
| Menu navigation | Arrow keys |
| Select menu option | `Enter` |
| Toggle sound | `S` |

## Screens

The game includes:

- Main menu
- Ship selection screen
- High score screen
- Pause menu
- Game over screen

## Requirements

To build the project locally, you need:

- C++17 compatible compiler
- CMake 3.22 or newer
- SFML 3

The recommended setup is to use CMake with SFML through `FetchContent`, so SFML can be downloaded automatically during the build process.

## Build Instructions

### macOS / Linux

```bash
cmake -S . -B build
cmake --build build
./build/AstroDrift

Depending on your generator, the executable may also be placed directly inside the build folder.

cmake -S . -B build
cmake --build build --config Release

The executable will usually be created here:
build/Release/AstroDrift.exe

Project Structure

astro-drift/
├── assets/
├── main.cpp
├── CMakeLists.txt
├── README.md
├── .gitignore
└── .github/
    └── workflows/
        └── windows-build.yml

Assets

The game expects the following assets:

assets/
├── background.png
├── logo.png
├── asteroid.png
├── spaceship_1.png
├── spaceship_2.png
├── spaceship_3.png
├── laser.png
├── powerup_shield.png
├── powerup_rapid.png
├── shoot.wav
├── explosion.wav
├── powerup.wav
├── hit.wav
├── death.wav
├── menu_select.wav
├── menu_music.ogg
└── orbitron.ttf

Some assets are required for the game to start, while sound and music files are optional. If optional audio files are missing, the game can still run without them.

Windows Release

A Windows build can be generated automatically with GitHub Actions.

After pushing to GitHub, go to:

Actions → Windows Build → Latest run → Artifacts

Download:

AstroDrift_Windows.zip

AstroDrift/
├── AstroDrift.exe
├── assets/
└── openal32.dll

Run AstroDrift.exe from inside the extracted folder. Do not move the executable away from the assets folder.

Notes

This project started as a small C++/SFML learning project and evolved into a complete arcade mini-game.

The current version is designed as a polished v1.0 release. Future improvements could include:

* Refactoring into multiple source files/classes
* More enemy types
* Difficulty modes
* Controller support
* Better release packaging

## Asset Credits

This project uses a mix of custom, generated, and open/free assets.

### Font

- **Orbitron** — Google Fonts  
  License: SIL Open Font License 1.1  
  Used for the sci-fi menu/UI font.  
  Source: Google Fonts / Orbitron.  [oai_citation:0‡Google Fonts](https://fonts.google.com/specimen/Orbitron?utm_source=chatgpt.com)

### Music

- **Outer Space Loop** — wipics, OpenGameArt  
  License: CC0 / Public Domain  
  Used as the main menu music, saved locally as `assets/menu_music.ogg`.  [oai_citation:1‡OpenGameArt.org](https://opengameart.org/content/outer-space-loop?utm_source=chatgpt.com)

### Sound Effects

Sound effects were sourced from free sci-fi sound packs:

- **Kenney Sci-fi Sounds** — Kenney  
  License: Creative Commons CC0  
  Used for sci-fi/game sound effects such as shooting, explosions, hits, menu sounds, and power-up style effects.  [oai_citation:2‡kenney.nl](https://kenney.nl/assets/sci-fi-sounds?utm_source=chatgpt.com)

- **50 CC0 Sci-Fi SFX** — OpenGameArt  
  License: CC0  
  Used as an additional source/reference for sci-fi sound effects.  [oai_citation:3‡OpenGameArt.org](https://opengameart.org/content/50-cc0-sci-fi-sfx?utm_source=chatgpt.com)

### Visual Assets

- **Spaceships, asteroid, power-ups, background, logo, and laser sprites**  
  These assets were created/customized for this project during development.

- **Rare asteroid and spaceship debris variants**  
  These are generated in-game from existing textures:
  - `RareAsteroid` uses `assets/asteroid.png` with a red/orange tint.
  - `ShipDebris` uses the existing spaceship textures with smaller scale and grey tint.

### Notes

Some assets may have been renamed locally to match the file names expected by the game:

```text
assets/
├── background.png
├── logo.png
├── asteroid.png
├── spaceship_1.png
├── spaceship_2.png
├── spaceship_3.png
├── laser.png
├── powerup_shield.png
├── powerup_rapid.png
├── shoot.wav
├── explosion.wav
├── powerup.wav
├── hit.wav
├── death.wav
├── menu_select.wav
├── menu_music.ogg
└── orbitron.ttf