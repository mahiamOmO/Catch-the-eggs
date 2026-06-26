# Installation Guide — Catch-the-eggs 🥚

Step-by-step guide to set up and run the game on your system.

---

## ✅ Requirements

| Tool | Purpose |
|------|---------|
| MinGW (g++) | C++ compiler |
| FreeGLUT | OpenGL window/input library |
| OpenGL 1.1+ | Graphics rendering |
| Windows OS | Supported platform |

---

## 🪟 Windows Setup (MinGW + FreeGLUT)

### Step 1 — Install MinGW
1. Download MinGW from: https://www.mingw-w64.org/downloads/
2. Run the installer and select `g++` during setup
3. Add MinGW to your system PATH:
   - Search **"Environment Variables"** in Windows
   - Under **System Variables**, find `Path` → click **Edit**
   - Add the path to MinGW's `bin` folder (e.g., `C:\mingw64\bin`)
4. Verify installation:
   ```bash
   g++ --version
   ```

### Step 2 — Install FreeGLUT
1. Download FreeGLUT from: https://www.transmissionzero.co.uk/software/freeglut-devel/
2. Extract the zip file
3. Copy files to your MinGW directory:
   - `freeglut/bin/freeglut.dll` → `C:\mingw64\bin\`
   - `freeglut/lib/libfreeglut.a` → `C:\mingw64\lib\`
   - `freeglut/include/GL/` folder → `C:\mingw64\include\GL\`

### Step 3 — Clone the Repository
```bash
git clone https://github.com/mahiamOmO/Catch-the-eggs.git
cd Catch-the-eggs/Catch-the-eggs
```

### Step 4 — Compile the Game

**Using Makefile (recommended):**
```bash
make
```

**Or manually:**
```bash
g++ catch_the_eggs.cpp -o game.exe -lfreeglut -lopengl32 -lglu32 -lm
```

### Step 5 — Run the Game
```bash
make run
# or
./game.exe
```

---

## 🐧 Linux / WSL Setup

### Step 1 — Install Dependencies
```bash
sudo apt update
sudo apt install g++ freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Step 2 — Clone and Build
```bash
git clone https://github.com/mahiamOmO/Catch-the-eggs.git
cd Catch-the-eggs/Catch-the-eggs
make
```

### Step 3 — Run
```bash
make run
# or
./game
```

---

## 🔧 Troubleshooting

**`g++ not found`**
→ Install MinGW and make sure it's added to PATH.

**`freeglut.dll not found` at runtime**
→ Copy `freeglut.dll` into the same folder as `game.exe`.

**Black screen / window doesn't appear**
→ Ensure your graphics drivers support OpenGL 1.1+. Try updating GPU drivers.

**`make` command not found**
→ Install make via MinGW: select `mingw32-make` during MinGW setup, then use `mingw32-make` instead of `make`.

---

## ✅ Quick Test

After running the game, you should see an 800×600 window titled **"Catch The Eggs"** with the main menu. If you see this, setup is complete! 🎉
