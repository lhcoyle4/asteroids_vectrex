# Asteroids - Vectrex Edition

A retro-accurate, premium Asteroids clone written in C using SDL2. This project features hardware-accelerated vector phosphor decay emulation, custom line-based typography, and procedurally synthesized sound effects.

## Controls
- **Left / Right / A / D**: Rotate Ship
- **Up / W**: Thrusters
- **Space**: Fire Laser
- **Down / Enter**: Hyperspace
- **Left/Right & Space/Enter (Menus)**: Choose and confirm high score initials

## Features
- **Vectrex Glow Emulation**: Uses texture accumulation to simulate a real vector display's persistence.
- **Pure C Audio Synthesis**: No external WAV files needed. Sound effects (firing, ship engine, heartbeat beats, explosions, UFO loop) are procedurally generated in memory using sine waves, frequency modulation, and white noise.
- **High Scores**: Saved automatically in `highscores.dat` with retro name input.
- **Original UFO Behavior**: Large UFOs shoot randomly, Small UFOs target the player and increase in accuracy as level progresses.

## Requirements
- MinGW GCC (64-bit)
- Make (supplied via Chocolatey or standard package managers)
- SDL2 & SDL2_mixer (packaged in `thirdparty/` and downloaded automatically)

## Compilation & Run
1. Open PowerShell/CMD in this directory.
2. Build the project:
   ```powershell
   make
   ```
3. Run:
   ```powershell
   .\asteroids_vectrex.exe
   ```
