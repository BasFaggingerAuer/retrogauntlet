# Copyright 2022 Bas Fagginger Auer.
# This file is part of Retro Gauntlet.
# 
# Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
# 
# Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
cmake_minimum_required(VERSION 3.16)
project(RG C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-O2 -g -Wall -Wextra -Wshadow -std=c99 -pedantic")
# set(CMAKE_C_FLAGS "-Os -DNDEBUG -Wall -Wextra -Wshadow -std=c99 -pedantic")

set(CMAKE_MODULE_PATH ${RG_SOURCE_DIR})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)

include(FindPkgConfig)
pkg_check_modules(SDL2MIXER REQUIRED SDL2_mixer>=2.0.0)

include_directories(${GLEW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ${SDL2MIXER_INCLUDE_DIRS} ${RG_SOURCE_DIR}/include/)
add_executable(retrogauntlet src/main.c src/retrogauntlet.c src/gauntletgame.c src/files.c src/stringextra.c src/net.c src/blowfish.c src/ini.c src/menu.c src/gauntlet.c src/core.c src/glcheck.c src/glvideo.c src/sdlglcoreinterface.c)

if (WIN32)
    target_link_libraries(retrogauntlet ws2_32 iphlpapi)
else()
    target_link_libraries(retrogauntlet dl)
endif()

target_link_libraries(retrogauntlet ${SDL2MIXER_LIBRARIES} ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
