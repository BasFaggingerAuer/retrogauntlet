/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Menu describing the Retro Gauntlet game state and which plays menu sounds and music.
#ifndef RETRO_GAUNTLET_MENU_H__
#define RETRO_GAUNTLET_MENU_H__

#include <SDL.h>
#include <SDL_mixer.h>

#include "retrogauntlet.h"
#include "glvideo.h"

#define NR_RETRO_GAUNTLET_MENU_TEXT 4096

enum retrogauntlet_menu_state {
    RETRO_GAUNTLET_STATE_SELECT_GAUNTLET = 0,
    RETRO_GAUNTLET_STATE_RUN_CORE       = 1,
    RETRO_GAUNTLET_STATE_SETUP_GAUNTLET = 2,
    RETRO_GAUNTLET_STATE_MESSAGE        = 3,
    RETRO_GAUNTLET_STATE_LOBBY_HOST     = 4,
    RETRO_GAUNTLET_STATE_LOBBY_CLIENT   = 5
};

enum retrogauntlet_sync_level {
    RETRO_GAUNTLET_SYNC_NONE = 0,
    RETRO_GAUNTLET_SYNC_INI = 1,
    RETRO_GAUNTLET_SYNC_SAVE = 2,
    RETRO_GAUNTLET_SYNC_ROM = 3,
    RETRO_GAUNTLET_SYNC_ALL = 4
};

struct soundboard {
    char **files;
    size_t nr_files;
    Mix_Chunk *sample;
    int channel;
};

bool create_soundboard(struct soundboard *);
bool free_soundboard(struct soundboard *);
bool soundboard_add_sample_file(struct soundboard *, char *);
bool soundboard_play(struct soundboard *, int);
bool soundboard_stop(struct soundboard *);

struct retrogauntlet_menu {
    char *data_directory;
    char *music_file;
    char *fragment_shader_file;

    char *fragment_shader_code;
    char password[NR_RETRO_GAUNTLET_PASSWORD + 1];
    char player_name[NR_RETRO_GAUNTLET_NAME + 1];
    int network_port;

    bool mixer_enabled;
    struct soundboard win_board, lose_board, login_board;
    
    enum retrogauntlet_sync_level sync_level;
    enum retrogauntlet_menu_state state, last_state;
    Mix_Music *music;
    uint32_t music_position;
    uint32_t music_start_time;
    SDL_Surface *surface;
    struct gl_video video;
    SDL_Color front_color, back_color;
    char text[NR_RETRO_GAUNTLET_MENU_TEXT + 1];
    char last_text[NR_RETRO_GAUNTLET_MENU_TEXT + 1];
    uint8_t terminal[80*25];
};

bool create_menu(struct retrogauntlet_menu *, const char *, const char *, const unsigned, const unsigned);
bool free_menu(struct retrogauntlet_menu *);
void menu_draw(struct retrogauntlet_menu *);
void menu_draw_message(struct retrogauntlet_menu *, const char *, ...);
bool menu_start_mixer(struct retrogauntlet_menu *);
bool menu_stop_mixer(struct retrogauntlet_menu *);

#endif

