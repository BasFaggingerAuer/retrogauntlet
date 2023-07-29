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
TARGET_STEAM := retrogauntletsteam
BUILD_DIR := ./build
INCLUDE_DIR := ./include
SOURCE_DIR := ./src
STEAMWORKS_SDK := /home/zuhli/git/steamsdk

# Dependencies of the targets.
RG_SOURCES := src/files.c src/core.c src/retrogauntlet.c src/net.c src/menu.c src/sdlglcoreinterface.c src/stringextra.c src/glcheck.c src/ini.c src/gauntletgame.c src/gauntlet.c src/blowfish.c src/glvideo.c
TARGET_SOURCES := $(RG_SOURCES) src/main.c
TARGET_STEAM_SOURCES := $(RG_SOURCES) src/mainsteam.cpp

# Tools.
CC := gcc
CXX := g++
MKDIRP := mkdir -p
RM := rm
CP := cp
CFLAGS := -O2 -g -Wall -Wextra -Wshadow -pedantic
CSTD := -std=c99
CXXSTD := -std=c++11
LDFLAGS := 

# External dependencies.
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -DPOSIX
CXXFLAGS += -I$(INCLUDE_DIR)

ALL_TARGETS := $(BUILD_DIR)/$(TARGET)

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

# Steam.
ifneq ($(STEAMWORKS_SDK),)
    #TODO: Add Windows support.
    CFLAGS += -DUSE_STEAM
    CFLAGS += -isystem $(STEAMWORKS_SDK)/public
    LDFLAGS += -L$(STEAMWORKS_SDK)/public/steam/lib/linux64 -L$(STEAMWORKS_SDK)/redistributable_bin/linux64 -lsteam_api
    STEAM_API := libsteam_api.so
    STEAM_APPID := steam_appid.txt
    STEAM_LAUNCHER := steamlauncher.sh
    ALL_TARGETS += $(BUILD_DIR)/$(STEAM_API) $(BUILD_DIR)/$(TARGET_STEAM) $(BUILD_DIR)/$(STEAM_APPID) $(BUILD_DIR)/$(STEAM_LAUNCHER)
endif

# Compilation of source files.
SOURCES_C := $(shell find $(SOURCE_DIR) -name '*.c')
OBJECTS_C := $(SOURCES_C:%=$(BUILD_DIR)/%.o)
SOURCES_CXX := $(shell find $(SOURCE_DIR) -name '*.cpp')
OBJECTS_CXX := $(SOURCES_CXX:%=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(ALL_TARGETS)

clean:
	$(RM) -r $(BUILD_DIR)

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIRP) $(dir $@)
	$(CC) $(CFLAGS) $(CSTD) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIRP) $(dir $@)
	$(CXX) $(CFLAGS) $(CXXSTD) -c $< -o $@

$(BUILD_DIR)/$(TARGET): $(TARGET_SOURCES:%=$(BUILD_DIR)/%.o)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/$(TARGET_STEAM): $(TARGET_STEAM_SOURCES:%=$(BUILD_DIR)/%.o)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/$(STEAM_API): $(STEAMWORKS_SDK)/redistributable_bin/linux64/$(STEAM_API)
	$(CP) -v $< $@

$(BUILD_DIR)/$(STEAM_APPID): data/$(STEAM_APPID)
	$(CP) -v $< $@

$(BUILD_DIR)/$(STEAM_LAUNCHER): data/$(STEAM_LAUNCHER)
	$(CP) -v $< $@

