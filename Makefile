CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= $(SDL2_CFLAGS) -Iinclude
PKG_CONFIG ?= pkg-config
SDL2_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image)
SDL2_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image)

TARGET := raycaster
SRC := $(wildcard *.cpp)
OBJDIR := obj
OBJ := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(SDL2_LIBS)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -rf $(OBJDIR)
