/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Gauntlet describing a specific Retro Gauntlet challenge with its boundary conditions.
#ifndef GAUNTLET_H__
#define GAUNTLET_H__

#include <stdint.h>

#include "core.h"
#include "sdlglcoreinterface.h"

enum gauntlet_status {
    RETRO_GAUNTLET_OFF = 0,
    RETRO_GAUNTLET_RUNNING = 1,
    RETRO_GAUNTLET_WON = 2,
    RETRO_GAUNTLET_LOST = 3
};

struct gauntlet {
    char *data_directory;
    char *ini_file;
    
    //Do not free, refers to core library file for current OS.
    const char *core_library_file;

    char *core_library_file_win64;
    char *core_library_file_linux64;
    char *core_variables_file;
    char *core_save_file;
    char *win_condition_file;
    char *lose_condition_file;
    char *rom_file;
    char *rom_startup_file;

    char *title;
    char *description;
    char *controls;

    bool enable_mouse;
    int mouse_button_mask;
    bool enable_controller;
    bool enable_debug;
    
    enum gauntlet_status status;
    uint32_t par_time;
    uint32_t start_time, end_time;
    struct retro_core_memory_condition *win_conditions;
    size_t nr_win_conditions;
    struct retro_core_memory_condition *lose_conditions;
    size_t nr_lose_conditions;
};

bool free_gauntlet(struct gauntlet *);
bool create_gauntlet(struct gauntlet *, const char *, const char *);
bool gauntlet_start(struct gauntlet *, struct sdl_gl_core_interface *);
bool gauntlet_check_status(struct gauntlet *, struct sdl_gl_core_interface *);
bool gauntlet_stop(struct gauntlet *);
bool read_gauntlet_playlist(struct gauntlet **, size_t *, const char *);

#endif

