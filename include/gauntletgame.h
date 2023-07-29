/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Retro Gauntlet overall game state linking libretro cores, gauntlet challenge descriptions, and SDL state.
#ifndef GAUNTLET_GAME_H__
#define GAUNTLET_GAME_H__

#include <SDL.h>

#include "retrogauntlet.h"
#include "glcheck.h"
#include "glvideo.h"
#include "net.h"
#include "blowfish.h"
#include "core.h"
#include "sdlglcoreinterface.h"
#include "gauntlet.h"
#include "menu.h"

//Network players and messages.
enum message_types {
    RETRO_GAUNTLET_MSG_NAME = 0,
    RETRO_GAUNTLET_MSG_LOBBY = 1,
    RETRO_GAUNTLET_MSG_START = 2,
    RETRO_GAUNTLET_MSG_FINISH = 3,
    RETRO_GAUNTLET_MSG_GET_FILES = 4,
    RETRO_GAUNTLET_MSG_FILE_START = 5,
    RETRO_GAUNTLET_MSG_FILE_DATA = 6,
    RETRO_GAUNTLET_MSG_FILE_END = 7,
    RETRO_GAUNTLET_MSG_MAX = 8
};

struct gauntlet_player {
    char name[NR_RETRO_GAUNTLET_NAME + 1];
    uint32_t finish_time;
    uint32_t points, last_points;
    enum gauntlet_status finish_state;
    
    uint8_t data[MAX_RETRO_GAUNTLET_MSG_DATA];
    size_t nr_data;
    size_t nr_data_expected;
};

struct gauntlet_game {
    //Global state variable for interfacing between SDL, OpenGL, and libretro.
    struct sdl_gl_core_interface sgci;
    struct retrogauntlet_menu menu;
    struct gauntlet gauntlet;
    struct gauntlet *gauntlets;
    size_t nr_gauntlets;
    size_t i_gauntlet;

    //Variables related to networking.
    struct client client;
    struct host host;
    struct blowfish fish;
    struct gauntlet_player players[MAX_RETRO_GAUNTLET_CLIENTS + 1];
    int player_indices[MAX_RETRO_GAUNTLET_CLIENTS + 1];
    uint8_t message_buffer[MAX_RETRO_GAUNTLET_MSG_DATA];
    FILE *client_recv_fid;
    char *client_recv_file;
    bool is_host_gauntlet_running;

    char lobby_text[NR_RETRO_GAUNTLET_MENU_TEXT + 1];
    char last_lobby_text[NR_RETRO_GAUNTLET_MENU_TEXT + 1];
    uint32_t last_lobby_update_time;

    //Variables related to setting up cores.
    unsigned snapshot_mask_condition;
    unsigned snapshot_data_condition;
    unsigned snapshot_mask_action;
    unsigned snapshot_mask_size;
    uint64_t snapshot_const_value;

    //SDL output window and related variables.
    SDL_Window *window;
    Uint32 start_ticks;
    Uint32 nr_frames;
    bool fullscreen;
    bool keep_running;
};

void game_draw_message_to_screen(struct gauntlet_game *, const char *, ...);
bool create_game(struct gauntlet_game *, const char *, SDL_Window *);
bool free_game(struct gauntlet_game *);

void game_change_gauntlet_selection(struct gauntlet_game *, const int);
void game_select_rand_gauntlet(struct gauntlet_game *);
bool game_stop_gauntlet(struct gauntlet_game *);
bool game_start_gauntlet(struct gauntlet_game *, const char *);
void game_update_menu_text(struct gauntlet_game *);
void game_update(struct gauntlet_game *);
void game_sdl_event(struct gauntlet_game *, const SDL_Event);

bool game_start_host(struct gauntlet_game *);
bool game_stop_host(struct gauntlet_game *);
bool game_update_host(struct gauntlet_game *);
bool game_host_start_gauntlet(struct gauntlet_game *);

bool game_start_client(struct gauntlet_game *, const char *);
bool game_stop_client(struct gauntlet_game *);
bool game_update_client(struct gauntlet_game *);

bool create_player(struct gauntlet_player *);

#endif

