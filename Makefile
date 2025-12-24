CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2
PKG_CONFIG ?= pkg-config
SDL2_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image)
SDL2_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image)

TARGET := raycaster
SRC := main.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SDL2_CFLAGS) $(SRC) -o $@ $(SDL2_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
