# ============================================================================
#  Makefile for Hexactly
#  Usage:  make run   |   make   |   make clean
# ============================================================================

CXX = clang++
# Where Homebrew installed raylib (Apple Silicon). Intel Macs: /usr/local
RAYLIB = /opt/homebrew/opt/raylib

# -DHEX_DEV enables local-only dev helpers (e.g. the S-key screenshot). The CI
# builds use src/Makefile, which does NOT define it, so those helpers are
# compiled out of the shipped desktop/web/windows builds.
CXXFLAGS = -std=c++17 -Wall -DHEX_DEV -Iinclude -I$(RAYLIB)/include
LDFLAGS  = -L$(RAYLIB)/lib -lraylib \
           -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio

TARGET = hexactly
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	cp -R src/resources ./resources

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) src/*.o
	rm -rf ./resources

.PHONY: run clean
