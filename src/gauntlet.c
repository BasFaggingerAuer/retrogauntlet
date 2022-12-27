/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>

#include "retrogauntlet.h"
#include "stringextra.h"
#include "files.h"
#include "ini.h"

#include "glvideo.h"
#include "gauntlet.h"

static int gauntlet_ini_handler(void *user, const char *section, const char *name, const char *value) {
    struct gauntlet *g = (struct gauntlet *)user;

    if (strcmp(section, "core") == 0 && strcmp(name, "library_win64") == 0) g->core_library_file_win64 = combine_paths(g->data_directory, value);
    if (strcmp(section, "core") == 0 && strcmp(name, "library_linux64") == 0) g->core_library_file_linux64 = combine_paths(g->data_directory, value);
    if (strcmp(section, "core") == 0 && strcmp(name, "variables") == 0) g->core_variables_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "rom") == 0 && strcmp(name, "rom") == 0) g->rom_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "rom") == 0 && strcmp(name, "startup") == 0) g->rom_startup_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "rom") == 0 && strcmp(name, "mouse") == 0) g->enable_mouse = (strcmp(value, "yes") == 0);
    if (strcmp(section, "rom") == 0 && strcmp(name, "controller") == 0) g->enable_controller = (strcmp(value, "yes") == 0);
    
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "save") == 0) g->core_save_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "win") == 0) g->win_condition_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "lose") == 0) g->lose_condition_file = combine_paths(g->data_directory, value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "par_time_ms") == 0) g->par_time = atoi(value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "title") == 0) g->title = strdup(value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "description") == 0) g->description = strdup(value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "controls") == 0) g->controls = strdup(value);
    if (strcmp(section, "gauntlet") == 0 && strcmp(name, "debug") == 0) g->enable_debug = (strcmp(value, "yes") == 0);

    return 1;
}

bool free_gauntlet(struct gauntlet *g) {
    if (!g) {
        fprintf(ERROR_FILE, "free_gauntlet: Invalid gauntlet!\n");
        return false;
    }
    
    if (g->data_directory) free(g->data_directory);
    if (g->ini_file) free(g->ini_file);
    if (g->core_library_file_win64) free(g->core_library_file_win64);
    if (g->core_library_file_linux64) free(g->core_library_file_linux64);
    if (g->core_variables_file) free(g->core_variables_file);
    if (g->core_save_file) free(g->core_save_file);
    if (g->win_condition_file) free(g->win_condition_file);
    if (g->lose_condition_file) free(g->lose_condition_file);
    if (g->rom_file) free(g->rom_file);
    if (g->rom_startup_file) free(g->rom_startup_file);
    if (g->title) free(g->title);
    if (g->description) free(g->description);
    if (g->controls) free(g->controls);
    if (g->win_conditions) free(g->win_conditions);
    if (g->lose_conditions) free(g->lose_conditions);

    memset(g, 0, sizeof(struct gauntlet));

    return true;
}

#define NR_GAUNTLET_PLAYLIST_LINE 4096

bool read_gauntlet_playlist(struct gauntlet **gauntlets_p, size_t *nr_gauntlets_p, const char *data_directory) {
    if (!gauntlets_p || !nr_gauntlets_p || !data_directory) {
        fprintf(ERROR_FILE, "read_gauntlet_playlist: Invalid arguments!\n");
        return false;
    }

    struct gauntlet *gauntlets = NULL;
    size_t nr_gauntlets = 0;
    char *playlist_file = combine_paths(data_directory, "playlist.txt");

    if (!playlist_file) {
        fprintf(ERROR_FILE, "read_gauntlet_playlist: Insufficient memory!\n");
        return false;
    }
    
    FILE *f = fopen(playlist_file, "r");
    char line[NR_GAUNTLET_PLAYLIST_LINE];
    char line2[NR_GAUNTLET_PLAYLIST_LINE];

    if (!f) {
        fprintf(ERROR_FILE, "read_gauntlet_playlist: Unable to read playlist file '%s'!\n", playlist_file);
        free(playlist_file);
        return false;
    }

    while (fgets(line2, NR_GAUNTLET_PLAYLIST_LINE, f)) {
        strncpy_trim(line, line2, NR_GAUNTLET_PLAYLIST_LINE);
        if (IS_COMMENT_LINE(line)) continue;
        
        nr_gauntlets++;

        if (!(gauntlets = (struct gauntlet *)realloc(gauntlets, nr_gauntlets*sizeof(struct gauntlet)))) {
            fprintf(ERROR_FILE, "read_gauntlet_playlist: Unable to allocate memory!\n");
            free(playlist_file);
            fclose(f);
            return false;
        }

        struct gauntlet *g = gauntlets + (nr_gauntlets - 1);
        char *ini_file = combine_paths(data_directory, line);
        
        if (!create_gauntlet(g, ini_file, data_directory)) {
            fprintf(ERROR_FILE, "read_gauntlet_playlist: Unable to read gauntlet!\n");
            free_gauntlet(g);
            nr_gauntlets--;
        }

        if (ini_file) free(ini_file);
    }

    fclose(f);

    if (nr_gauntlets == 0) {
        fprintf(ERROR_FILE, "read_gauntlet_playlist: Unable to read any gauntlets!\n");
        free(playlist_file);
        if (gauntlets) free(gauntlets);
        return false;
    }

    fprintf(INFO_FILE, "Read %zu gauntlets from '%s'.\n", nr_gauntlets, playlist_file);
    
    free(playlist_file);
    *gauntlets_p = gauntlets;
    *nr_gauntlets_p = nr_gauntlets;
    
    return true;
}

bool create_gauntlet(struct gauntlet *g, const char *ini_file, const char *data_directory) {
    if (!g || !ini_file) {
        fprintf(ERROR_FILE, "create_gauntlet: Invalid gauntlet or INI file!\n");
        return false;
    }

    memset(g, 0, sizeof(struct gauntlet));

    g->data_directory = expand_to_full_path(data_directory);
    g->ini_file = strdup(ini_file);

    if (ini_parse(ini_file, gauntlet_ini_handler, g) < 0) {
        fprintf(ERROR_FILE, "create_gauntlet: Unable to parse INI file '%s'!\n", ini_file);
        free_gauntlet(g);
        return false;
    }

#ifdef _WIN32
    g->core_library_file = g->core_library_file_win64;
#else
    g->core_library_file = g->core_library_file_linux64;
#endif

    if (!g->core_library_file || !g->title) {
        fprintf(ERROR_FILE, "create_gauntlet: Missing crucial gauntlet information in '%s'!\n", ini_file);
        free_gauntlet(g);
        return false;
    }

    fprintf(INFO_FILE, "Read gauntlet INI for %s.\n", g->title);

    return true;
}

bool gauntlet_start(struct gauntlet *g, struct sdl_gl_core_interface *sgci) {
    if (!g || !sgci) {
        fprintf(ERROR_FILE, "gauntlet_start: Invalid gauntlet or interface!\n");
        return false;
    }

    if (g->status != RETRO_GAUNTLET_OFF) {
        fprintf(ERROR_FILE, "gauntlet_start: Gauntlet is not in off state!\n");
        return false;
    }
    
    fprintf(INFO_FILE, "Starting gauntlet %s...\n", g->title);

    //Free possibly existing data.
    gauntlet_stop(g);

    //Load conditions.
    if (g->win_condition_file) {
        if (!core_load_conditions_from_file(&g->win_conditions, &g->nr_win_conditions, g->win_condition_file)) return false;
    }

    if (g->lose_condition_file) {
        if (!core_load_conditions_from_file(&g->lose_conditions, &g->nr_lose_conditions, g->lose_condition_file)) return false;
    }
    
    //Enable desired controllers.
    sgci->enable_mouse = g->enable_mouse;
    sgci->enable_controller = g->enable_controller;

    //Run core for 1 iteration to initialize it.
    video_bind_frame_buffer(&sgci->video);
    sgci->core.retro_run();
    sdl_gl_if_reset_audio(sgci);
    
    //Run command file if it exists.
    if (g->rom_startup_file) sdl_gl_if_run_commands_from_file(sgci, g->rom_startup_file);
    
    video_unbind_frame_buffer(&sgci->video);
    
    //Restore save.
    if (g->core_save_file) core_unserialize_from_file(&sgci->core, g->core_save_file);
    
    sdl_gl_if_reset_audio(sgci);
    g->status = RETRO_GAUNTLET_RUNNING;
    g->start_time = SDL_GetTicks();
    g->end_time = g->start_time;

    return true;
}

bool gauntlet_check_status(struct gauntlet *g, struct sdl_gl_core_interface *sgci) {
    if (!g || !sgci) {
        fprintf(ERROR_FILE, "gauntlet_start: Invalid gauntlet or interface!\n");
        return false;
    }
    
    if (g->status == RETRO_GAUNTLET_RUNNING) {
        const uint32_t t = SDL_GetTicks();

        if (g->win_conditions && core_check_conditions(&sgci->core, g->win_conditions, g->nr_win_conditions, g->enable_debug)) {
            g->status = RETRO_GAUNTLET_WON;
            g->end_time = t;
        }

        if ((g->lose_conditions && core_check_conditions(&sgci->core, g->lose_conditions, g->nr_lose_conditions, g->enable_debug)) ||
            (g->status == RETRO_GAUNTLET_RUNNING && g->par_time > 0 && t > g->start_time + g->par_time)) {
            g->status = RETRO_GAUNTLET_LOST;
            g->end_time = t;
        }
    }

    return true;
}

bool gauntlet_stop(struct gauntlet *g) {
    if (!g) {
        fprintf(ERROR_FILE, "gauntlet_start: Invalid gauntlet!\n");
        return false;
    }

    if (g->win_conditions) free(g->win_conditions);
    g->win_conditions = NULL;
    g->nr_win_conditions = 0;
    
    if (g->lose_conditions) free(g->lose_conditions);
    g->lose_conditions = NULL;
    g->nr_lose_conditions = 0;

    g->status = RETRO_GAUNTLET_OFF;

    return true;
}

