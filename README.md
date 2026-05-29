# 🎮 CATCH THE EGGS - Game Graphics Project

> **A fun 2D egg-catching game built with OpenGL and C++**


## 🧑‍🏫 Faculty & Course Information

- **Faculty:** [Md. Rasheduzzaman](https://cse.uap-bd.edu/people/faculty/mr/)

- **Course:** CSE 426 – Computer Graphics Design Lab
  

## 📋 Project Overview

**Catch the Eggs** is a Computer Graphics (CSE 426) term project that implements a complete 2D game with:
- Real-time graphics rendering using OpenGL
- Interactive gameplay with keyboard & mouse controls
- Sound effects and background music using SDL2
- Multiple power-ups and game mechanics
- Menu system with high score tracking

**Status:** ✅ Complete with Bonus Features  
**Year:** Spring 2025

## 🎥 YouTube Video

**Watch the gameplay demo:**

https://github.com/user-attachments/assets/690d582f-5dab-4400-b8c2-89453c9ccf42

## 🎥 YouTube Video

**Watch the gameplay demo:**

[▶️ Watch on YouTube](https://youtu.be/j2XHgs4Okes)

- Duration: ~5-10 minutes
- Shows: Menu, gameplay, power-ups, scoring system, high score tracking


## 📊 Scoring System

| Item | Points | Color |
|------|--------|-------|
| Normal Egg 🥚 | +1 | White |
| Blue Egg 🔵 | +5 | Blue |
| Golden Egg ⭐ | +10 | Gold |
| Poop 💩 | -10 | Brown |

### Power-Ups

| Power-Up | Duration | Effect | Bonus Points |
|----------|----------|--------|--------------|
| 🧺 Bigger Basket | 10s | Double basket size | +3 |
| 🐢 Slow Motion | 8s | Eggs fall slower | +3 |
| ⏱️ Extra Time | ∞ | +20 seconds | +5 |
| 🛡️ Shield | 1 catch | Block 1 poop | +2 |

### Combo System
- Catch items consecutively = **Bonus multiplier**
- Floating score popups appear
- Combo breaks if you miss or catch poop


## 🎮 Controls

| Key | Action |
|-----|--------|
| **← / →** or **A / D** | Move basket (menu) |
| **Mouse Move** | Move basket (during play) |
| **Mouse Click** | Navigate menu |
| **P** or **ESC** | Pause / Resume game |
| **N** | Toggle Night Mode |
| **M** | Toggle Background Music |


## ✨ Features Implemented

### Base Features (20 marks)
- ✅ Chicken laying eggs
- ✅ 4+ item types (eggs, poop)
- ✅ Keyboard + Mouse basket control
- ✅ Game timer (60 seconds)
- ✅ Score tracking
- ✅ 3+ Power-up types

### Menu System (5 marks)
- ✅ Start Game
- ✅ Resume Game
- ✅ High Score Display
- ✅ Exit Game
- ✅ Page Navigation

### Pause System (5 marks)
- ✅ Pause (P/ESC key)
- ✅ Resume Game
- ✅ Exit anytime
- ✅ Pause doesn't steal time

### BONUS Features (+10 marks)
- ✅ **2 Sticks** with 2 Chickens (multiple layers)
- ✅ **Wind System** - eggs drift sideways
- ✅ **Shield Power-up** - negate next poop
- ✅ **Help/Controls Page** with BACK button
- ✅ **Day/Night Toggle** (N key)
- ✅ **House & Trees** on menu & game background
- ✅ **Combo System** - consecutive catches = bonus
- ✅ **Score Popups** - floating +/- numbers
- ✅ **Star Rating** on Game Over screen (1-5 stars)
- ✅ **Power-up Bonus Points** - extra points when caught
- ✅ **Sound Effects** (Windows + Linux support)
- ✅ **Background Music** with multiple tracks
- ✅ **Animated Menu** - bouncing eggs, twinkling stars, smoke
- ✅ **Procedural Audio** - SDL2 synthesis


## 🎬 Game States

```
┌─────────────┐
│   MENU      │──→ ST_MENU (Main Menu)
└─────────────┘
      │
      ↓
┌─────────────┐
│   PLAY      │──→ ST_PLAY (Active Gameplay)
└─────────────┘
      │
      ├─→ P/ESC ──→ ST_PAUSE (Paused)
      │              │
      │              └─→ Resume → ST_PLAY
      │
      └─→ Time=0 ──→ ST_OVER (Game Over)
                      │
                      ├─→ View High Score → ST_HISCORE
                      └─→ Replay / Menu → ST_MENU
```


## 📁 Project Structure

```
Catch-the-eggs/
├── catch_the_eggs.cpp          # Main source file (~2000+ lines)
├── game/                       # Compiled executable
├── Catch The Eggs.pdf          # Project requirements
├── README.md                   # This file
└── .gitignore
```



## 🛠️ Compilation

### Windows (Visual Studio/MinGW)
```bash
g++ catch_the_eggs.cpp -o game -lfreeglut -lopengl32 -lglu32 -lm -lSDL2
```

### Linux (Ubuntu/Debian)
```bash
g++ catch_the_eggs.cpp -o game -lGL -lGLU -lglut -lm -lSDL2
```

#### Install SDL2 (Linux):
```bash
sudo apt install libsdl2-dev
```

---

## ▶️ Running the Game

```bash
./game
```

Or:
```bash
./game/game
```

---

## 📐 Technical Details

### Game Constants
- **Window Size:** 800 × 600 pixels
- **Game Duration:** 60 seconds (1 minute)
- **Basket Speed:** 380 pixels/second
- **Normal Basket Width:** 110 pixels
- **Big Basket Width:** 210 pixels
- **Number of Sticks:** 2 (parallel levels)

### Audio
- **Sample Rate:** 44,100 Hz
- **Channels:** Mono
- **Synthesis:** Procedural (SDL2)
- **Music Tracks:** Menu, Game, Game-Over jingle
- **Sound Effects:** Catch, power-up, combo

### Game Mechanics
- **Item Spawn:** Random x-position from chickens
- **Wind System:** Periodic horizontal drift (0-15 pixels)
- **Combo Timer:** 2 seconds between catches
- **Star Rating:** Based on score percentage (1-5 stars)

---

## 🎨 Visual Elements

### Background
- **Day Mode:** Blue gradient sky with sun
- **Night Mode:** Dark sky with twinkling stars & moon
- **Clouds:** Animated clouds in day mode
- **Ground:** Green grass with individual blades
- **Scenery:** Houses with chimneys, trees

### Game Objects
- **Basket:** Interactive player-controlled container
- **Chickens:** Walking back & forth, laying eggs
- **Eggs:** Falling with physics (gravity + wind)
- **Bamboo Sticks:** Horizontal platforms (2 levels)
- **Popups:** Floating score text (yellow/red)

---

## 📊 Code Statistics

| Section | Lines |
|---------|-------|
| Header & Includes | 60 |
| Enums & Structs | 50 |
| Global Variables | 80 |
| Audio System | 300+ |
| Drawing Functions | 400+ |
| Game Logic | 600+ |
| Main Loop & Event Handling | 300+ |
| **Total** | **~2000+** |

---

## 🎯 Key Functions

```cpp
// Game Loop
void update(float dt);              // Update game logic
void render();                      // Render graphics

// Game States
void updateMenu();                  // Menu logic
void updatePlay();                  // Gameplay logic
void drawMenu();                    // Render menu

// Physics
void updateItems();                 // Item movement & physics
void updateChickens();              // Chicken AI
void checkCollisions();             // Basket-item collision

// Audio
void audioCallback(...);            // Real-time audio synthesis
void sfxTrigger(float freq, ...);   // Play sound effect
void setMusicTrack(MusicTrack);     // Change music

// Rendering
void drawBG();                      // Draw background & scenery
void drawTree(...);                 // Draw tree
void drawHouse(...);                // Draw house
void button(...);                   // Draw menu button
```

---

## 🎮 Gameplay Example

1. **Start Game** → Timer begins (60 seconds)
2. **Chickens lay eggs** → Items fall from top
3. **Move basket** → Catch items to score points
4. **Combo builds** → Each catch = higher multiplier
5. **Power-ups appear** → Catch for special abilities
6. **Timer ends** → Game Over screen with score & star rating
7. **Check High Score** → Is it a new record?
8. **Replay or Menu** → Play again or exit

---

## 🐛 Known Issues / Limitations

- Sound effects may be silent on Linux (system-dependent)
- Background music Windows-optimized (SDL2 cross-platform)
- Star rating based on final score only

---

## 👨‍💻 Developed By

<table align="center">
  <tr>
    <td align="center" style="padding: 10px;">
      <img src="screenshort/mahia.jpg" alt="Mahia Akter Momo" width="150" style="border-radius: 50%;" /><br>
      <strong>Mahia Akter Momo</strong><br>
      <em>Team Leader</em><br><br>
      <a href="https://github.com/mahiamOmO" target="_blank">
        <img src="https://img.shields.io/badge/GitHub-333333?style=for-the-badge&logo=github&logoColor=white" />
      </a><br>
      <a href="https://linkedin.com/in/mahiamomo12" target="_blank">
        <img src="https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white" />
      </a>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="screenshort/kamrul.jpg" alt="Md Kamrul Hasan" width="150" style="border-radius: 50%;" /><br>
      <strong>Md Kamrul Hasan</strong><br>
      <em>Team Member 1</em><br><br>
      <a href="https://github.com/KamrulHasan-creator" target="_blank">
        <img src="https://img.shields.io/badge/GitHub-333333?style=for-the-badge&logo=github&logoColor=white" />
      </a><br>
      <a href="https://linkedin.com/in/kamrul-hasan-a30a6633b" target="_blank">
        <img src="https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white" />
      </a>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="screenshort/popy.jpeg" alt="Farzana Hossain Popy" width="150" style="border-radius: 50%;" /><br>
      <strong>Farzana Hossain Popy</strong><br>
      <em>Team Member 2</em><br><br>
      <a href="https://github.com/Farzana-Popy" target="_blank">
        <img src="https://img.shields.io/badge/GitHub-333333?style=for-the-badge&logo=github&logoColor=white" />
      </a><br>
      <a href="https://linkedin.com/in/farzana-hossain-popy" target="_blank">
        <img src="https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white" />
      </a>
    </td>
  </tr>
</table>



## 📝 License

This is a university project for educational purposes.

---

## 🎓 Learning Outcomes

- ✅ OpenGL graphics programming
- ✅ Game state management
- ✅ Physics simulation (gravity, wind)
- ✅ Event handling (keyboard, mouse)
- ✅ Audio synthesis & mixing
- ✅ UI/UX design (menus, buttons)
- ✅ Game mechanics (scoring, combos, power-ups)
- ✅ Optimization techniques

---

## 📸 Screenshots & Gameplay

All screenshots are available in the repository folders:

### Input Screenshots
**See user input & controls in action:**
- 📁 [Input Screenshots Folder](https://github.com/mahiamOmO/Catch-the-eggs/tree/main/Input%20Screenshort)
- Shows keyboard/mouse input demonstrations
- Control examples for basket movement
- Menu navigation demonstrations

### Output Screenshots
**See game output & results:**
- 📁 [Output Screenshots Folder](https://github.com/mahiamOmO/Catch-the-eggs/tree/main/Output%20Screenshort)

**Screenshots include:**
- ✅ **Main Menu Screen** - All menu buttons & options visible
- ✅ **Gameplay Screen** - Basket, falling eggs, score & timer
- ✅ **Power-Up Activation** - Big basket, slow motion effects
- ✅ **Game Over Screen** - Final score & star rating
- ✅ **High Score Display** - Best scores achieved
- ✅ **Help/Controls Page** - Game instructions
- ✅ **Night Mode** - Dark theme with stars & moon
- ✅ **Pause Screen** - Paused gameplay state

---

### Quick Preview

**Main Menu**
```
Shows: Animated background with bouncing eggs, 
       twinkling stars, menu buttons
       (Start, Resume, High Score, Help, Exit)
```

**Active Gameplay**
```
Shows: Basket at bottom, falling eggs from top,
       Two bamboo sticks at different heights,
       Score & timer display, wind indicator
```

**Power-Up in Action**
```
Shows: Basket doubled in size or slow motion effect,
       Score popup floating (+points),
       Visual effect indication
```

**Game Over**
```
Shows: Final score achieved,
       Star rating (1-5 stars based on performance),
       High score comparison,
       Replay/Menu options
```

---


## ✅ Checklist for Completion

- [x] Game compiles without errors
- [x] All controls working
- [x] Sound effects & music
- [x] All power-ups functional
- [x] High score tracking
- [x] Menu system complete
- [x] Pause/Resume working
- [x] Star rating system
- [x] Day/Night mode
- [x] Wind system
- [x] Combo system
- [x] Score popups
- [x] Scenery (houses, trees)
- [x] Documentation complete
- [ ] YouTube video uploaded
- [ ] Screenshots added

---

## 🚀 Future Enhancements

- Sound volume control
- Difficulty levels
- Leaderboard system
- Mobile version
- Multiplayer mode
- More power-ups
- Custom skins

---





