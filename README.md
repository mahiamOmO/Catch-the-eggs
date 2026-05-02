# Catch-the-eggs

A fun OpenGL game where you catch falling eggs with a basket!

## Overview

Catch-the-eggs is a classic arcade-style game built in C++ using OpenGL and GLUT. Navigate your basket to catch falling eggs and maximize your score before time runs out.

## Game Features

- **Interactive Gameplay**: Control a basket to catch falling eggs
- **Score Tracking**: Accumulate points for each egg caught
- **Time Challenge**: Compete against the clock (1:45 time limit)
- **Best Score Recording**: Track your highest score achieved
- **Menu System**: Easy navigation between game, high scores, help, and settings
- **Dynamic Controls**: Move basket with mouse (click or drag)
- **Night Mode**: Toggle night mode with the 'N' key for a different visual experience
- **Pause Feature**: Press 'P' or 'ESC' to pause during gameplay

## System Requirements

- Windows OS
- MinGW compiler (g++) with OpenGL libraries
- FreeGLUT library
- OpenGL 1.1+ compatible graphics card

## Compilation

Compile the game using g++ with the following command:

```bash
g++ catch_the_eggs.cpp -o game -lfreeglut -lopengl32 -lglu32 -lm
```

Or on Windows (if g++ is installed):
```cmd
g++ catch_the_eggs.cpp -o game.exe -lfreeglut -lopengl32 -lglu32 -lm
```

## Running the Game

After compilation, run the executable:

```bash
./game        # Linux/WSL
game.exe      # Windows
```

## How to Play

### Menu Controls
- **START GAME**: Click to begin a new game
- **HIGH SCORE**: View your best scores
- **HELP**: Read game instructions
- **EXIT**: Close the game

### Gameplay Controls
- **Mouse Move**: Smoothly track basket position with your cursor
- **Mouse Click**: Jump basket to clicked position
- **Arrow Keys / A-D**: Alternative basket movement
- **P / ESC**: Pause/Resume game
- **N**: Toggle night mode
- **Click Egg Item**: Collect eggs in high-score mode

### Game States
- **Menu**: Main menu with 4 options
- **Play**: Active gameplay - catch eggs
- **Pause**: Temporarily stop the game
- **Game Over**: View final score and options
- **High Score**: View best scores achieved
- **Help**: Read gameplay instructions

## Game Mechanics

- Each caught egg = 1 point
- Falling eggs appear randomly at the top
- Missed eggs reduce your catch rate
- Game ends when time reaches 0:00
- Your best score is automatically saved

## File Structure

```
catch_the_eggs.cpp    # Main game source code
game / game.exe       # Compiled executable
README.md             # This file
```

## Game Window

- Resolution: 800x600 pixels
- Title: "Catch The Eggs"

## Development Notes

Built with:
- **Language**: C++
- **Graphics Library**: OpenGL with GLUT/FreeGLUT
- **Input Handling**: Mouse and keyboard callbacks
- **Game Loop**: Idle-based render loop with glutTimerFunc

## Troubleshooting

**Compilation fails with "g++ not found"**
- Install MinGW compiler
- Add g++ to your system PATH
- Or use WSL (Windows Subsystem for Linux)

**Window doesn't appear**
- Ensure you have OpenGL and FreeGLUT libraries installed
- On Windows, install the appropriate development libraries

**Mouse controls not working**
- Ensure GLUT callbacks are properly registered
- Check that your window manager supports mouse input

## Future Enhancements

- Difficulty levels
- Multiplayer mode
- Sound effects
- More egg varieties with different point values
- Power-ups and obstacles
- Leaderboard system

---

Enjoy playing Catch-the-eggs! 🥚
