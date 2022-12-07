# Copyright 2022 Bas Fagginger Auer.
# This file is part of Retro Gauntlet.
# 
# Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
# 
# Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
#
# RetroGauntlet Makefile
# Based on Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)

TARGET := retrogauntlet
BUILD_DIR := ./build
INCLUDE_DIR := ./include
SOURCE_DIR := ./src

CC := gcc
MKDIRP := mkdir -p
RM := rm
CFLAGS := -O2 -g -Wall -Wextra -Wshadow -std=c99 -pedantic
LDFLAGS := 

# External dependencies.
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += -D_POSIX_C_SOURCE=200809L

ifeq ($(OS), Windows_NT)
	# Windows OS, builds using msys2/mingw. Have fun.
	CFLAGS += -IC:\msys64\mingw64\include\SDL2
	LDFLAGS += -LC:\msys64\mingw64\lib -lmingw32 -lws2_32 -liphlpapi -lSDL2_mixer -lSDL2main -lSDL2 -lglew32 -lopengl32 -lglu32 
else
	# Linux-based OS.
	PKG_CONFIG_DEPS := sdl2 SDL2_mixer glew

	CFLAGS += `pkg-config --cflags $(PKG_CONFIG_DEPS)`
	LDFLAGS += `pkg-config --libs $(PKG_CONFIG_DEPS)`
	LDFLAGS += -ldl -lGL -lGLU -lm
endif

# Building script.
SOURCES := $(shell find $(SOURCE_DIR) -name '*.c')
OBJECTS := $(SOURCES:%=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET)

clean:
	$(RM) -r $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIRP) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

