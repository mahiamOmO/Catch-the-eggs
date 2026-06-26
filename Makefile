# ============================================
#   Catch-the-eggs - Makefile
# ============================================

CXX      = g++
CXXFLAGS = -Wall -std=c++17
LIBS     = -lfreeglut -lopengl32 -lglu32 -lm

TARGET   = game
SRC      = catch_the_eggs.cpp

# Default: build the game
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Build + run
run: $(TARGET)
	./$(TARGET)

# Remove compiled files
clean:
	rm -f $(TARGET) $(TARGET).exe

# Clean and rebuild
rebuild: clean all

.PHONY: all run clean rebuild
