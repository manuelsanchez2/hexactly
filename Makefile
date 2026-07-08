# ============================================================================
#  Makefile for Hexactly
#  Usage:  make run   |   make   |   make clean
# ============================================================================

CXX = clang++
# Where Homebrew installed raylib (Apple Silicon). Intel Macs: /usr/local
RAYLIB = /opt/homebrew/opt/raylib

CXXFLAGS = -std=c++17 -Wall -Iinclude -I$(RAYLIB)/include
LDFLAGS  = -L$(RAYLIB)/lib -lraylib \
           -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio

TARGET = hexactly
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) src/*.o

.PHONY: run clean
