/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>

#include "stringextra.h"
#include "files.h"
#include "ini.h"
#include "dosfont.h"
#include "menu.h"

bool create_soundboard(struct soundboard *board) {
    if (!board) {
        fprintf(ERROR_FILE, "create_soundboard: Invalid board!\n");
        return false;
    }

    memset(board, 0, sizeof(struct soundboard));

    return true;
}

bool free_soundboard(struct soundboard *board) {
    if (!board) {
        fprintf(ERROR_FILE, "free_soundboard: Invalid board!\n");
        return false;
    }
    
    if (board->files) {
        for (size_t i = 0; i < board->nr_files; ++i) {
            if (board->files[i]) free(board->files[i]);
        }

        free(board->files);
    }
    
    if (board->sample) Mix_FreeChunk(board->sample);

    memset(board, 0, sizeof(struct soundboard));

    return true;
}

bool soundboard_add_sample_file(struct soundboard *board, char *file) {
    if (!board || !file) {
        fprintf(ERROR_FILE, "soundboard_add_sample_file: Invalid board or file!\n");
        return false;
    }

    board->nr_files++;
    board->files = (char **)realloc(board->files, board->nr_files*sizeof(char *));

    if (!board->files) {
        fprintf(ERROR_FILE, "soundboard_add_sample_file: Insufficient memory!\n");
        free_soundboard(board);
        return false;
    }

    board->files[board->nr_files - 1] = file;
    
    return true;
}

bool soundboard_play(struct soundboard *board, int i) {
    if (!board) {
        fprintf(ERROR_FILE, "soundboard_play: Invalid board!\n");
        return false;
    }

    if (!board->files || board->nr_files == 0) return true;

    if (board->sample) Mix_FreeChunk(board->sample);
    
    //Select a random sample if the index is invalid.
    if (i < 0 || i >= (int)board->nr_files) i = rand() % board->nr_files;

    if (!board->files[i]) return true;
    
    if (!(board->sample = Mix_LoadWAV(board->files[i]))) {
        fprintf(ERROR_FILE, "Unable to load sample '%s': %s!\n", board->files[i], SDL_GetError());
        return false;
    }

    board->channel = Mix_PlayChannel(-1, board->sample, 0);

    if (board->channel < 0) {
        fprintf(ERROR_FILE, "Unable to play sample '%s': %s!\n", board->files[i], SDL_GetError());
        return false;
    }

    fprintf(INFO_FILE, "Playing sample '%s'.\n", board->files[i]);

    return true;
}

bool soundboard_stop(struct soundboard *board) {
    if (!board) {
        fprintf(ERROR_FILE, "create_soundboard: Invalid board!\n");
        return false;
    }
    
    //Nothing to stop.
    if (!board->sample || board->channel < 0) return true;

    Mix_HaltChannel(board->channel);
    Mix_FreeChunk(board->sample);
    board->sample = NULL;

    return true;
}

SDL_Color get_sdl_color_from_html_hex(const char *hex) {
    SDL_Color color = {0x00, 0x00, 0x00, 0xff};

    if (!hex) return color;
    
    long long v = strtoll(hex, NULL, 16);

    color.b = (v & 0xff);
    color.g = ((v >> 8) & 0xff);
    color.r = ((v >> 16) & 0xff);
    
    return color;
}

static int menu_ini_handler(void *user, const char *section, const char *name, const char *value) {
    struct retrogauntlet_menu *menu = (struct retrogauntlet_menu *)user;
    
    if (strcmp(section, "menu") == 0 && strcmp(name, "music") == 0) menu->music_file = combine_paths(menu->data_directory, value);
    if (strcmp(section, "menu") == 0 && strcmp(name, "fragment_shader") == 0) menu->fragment_shader_file = combine_paths(menu->data_directory, value);
    if (strcmp(section, "menu") == 0 && strcmp(name, "front_color") == 0) menu->front_color = get_sdl_color_from_html_hex(value);
    if (strcmp(section, "menu") == 0 && strcmp(name, "back_color") == 0) menu->back_color = get_sdl_color_from_html_hex(value);
    
    if (strcmp(section, "network") == 0 && strcmp(name, "password") == 0) strncpy_trim(menu->password, value, NR_RETRO_GAUNTLET_PASSWORD);
    if (strcmp(section, "network") == 0 && strcmp(name, "port") == 0) menu->network_port = atoi(value);
    if (strcmp(section, "network") == 0 && strcmp(name, "name") == 0) strncpy(menu->player_name, value, NR_RETRO_GAUNTLET_NAME);
    if (strcmp(section, "network") == 0 && strcmp(name, "sync") == 0) {
             if (strcmp(value, "none") == 0) menu->sync_level = RETRO_GAUNTLET_SYNC_NONE;
        else if (strcmp(value, "ini") == 0) menu->sync_level = RETRO_GAUNTLET_SYNC_INI;
        else if (strcmp(value, "save") == 0) menu->sync_level = RETRO_GAUNTLET_SYNC_SAVE;
        else if (strcmp(value, "rom") == 0) menu->sync_level = RETRO_GAUNTLET_SYNC_ROM;
        else if (strcmp(value, "all") == 0) menu->sync_level = RETRO_GAUNTLET_SYNC_ALL;
    }

    if (strcmp(section, "sound_win") == 0 && strcmp(name, "sample") == 0) soundboard_add_sample_file(&menu->win_board, combine_paths(menu->data_directory, value));
    if (strcmp(section, "sound_lose") == 0 && strcmp(name, "sample") == 0) soundboard_add_sample_file(&menu->lose_board, combine_paths(menu->data_directory, value));
    if (strcmp(section, "sound_login") == 0 && strcmp(name, "sample") == 0) soundboard_add_sample_file(&menu->login_board, combine_paths(menu->data_directory, value));

    return 1;
}

bool create_menu(struct retrogauntlet_menu *menu, const char *ini_file, const char *data_directory, const unsigned width, const unsigned height) {
    if (!menu || !ini_file) {
        fprintf(ERROR_FILE, "create_menu: Invalid menu or INI file!\n");
        return false;
    }

    memset(menu, 0, sizeof(struct retrogauntlet_menu));
    
    menu->data_directory = expand_to_full_path(data_directory);
    menu->front_color = (SDL_Color){0xb8, 0xb8, 0xb8, 0xff};
    menu->back_color =  (SDL_Color){0x00, 0x00, 0xa8, 0xff};
    menu->sync_level = RETRO_GAUNTLET_SYNC_NONE;
    menu->state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;
    menu->last_state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;
    strcpy(menu->password, "Retr0G4untlet!");
    strcpy(menu->player_name, "Player");
    menu->network_port = 1337;

    if (!create_soundboard(&menu->win_board) ||
        !create_soundboard(&menu->lose_board) ||
        !create_soundboard(&menu->login_board)) {
        fprintf(ERROR_FILE, "create_menu: Unable to create soundboards!\n");
        return false;
    }

    if (ini_parse(ini_file, menu_ini_handler, menu) < 0) {
        fprintf(ERROR_FILE, "create_menu: Unable to parse INI file '%s'!\n", ini_file);
        free_menu(menu);
        return false;
    }

    if (menu->fragment_shader_file) {
        if (!(menu->fragment_shader_code = read_file_as_string(menu->fragment_shader_file))) {
            fprintf(WARN_FILE, "create_menu: Unable to open fragment shader file '%s'!\n", menu->fragment_shader_file);
        }
    }
    
    //Initialize video menu.
    if (!create_video(&menu->video)) {
        fprintf(ERROR_FILE, "create_menu: Unable to initialize video!\n");
        free_menu(menu);
        return false;
    }

    struct retro_game_geometry menu_geom = {720, 400, 720, 400, 4.0f/3.0f};
    
    video_set_shaders(&menu->video, NULL, menu->fragment_shader_code);
    video_set_geometry(&menu->video, &menu_geom);
    video_set_window(&menu->video, width, height);
    video_set_pixel_format(&menu->video, RETRO_PIXEL_FORMAT_GAUNTLET);
    create_video_buffers(&menu->video);

    if (!(menu->surface = video_get_sdl_surface(&menu->video))) {
        fprintf(ERROR_FILE, "create_menu: Unable to create SDL surface: %s!\n", SDL_GetError());
        free_menu(menu);
        return false;
    }

    fprintf(INFO_FILE, "Created menu from '%s'.\n", ini_file);
    
    return true;
}

bool free_menu(struct retrogauntlet_menu *menu) {
    if (!menu) {
        fprintf(ERROR_FILE, "free_menu: Invalid menu!\n");
        return false;
    }
    
    if (menu->data_directory) free(menu->data_directory);
    if (menu->music_file) free(menu->music_file);
    if (menu->fragment_shader_file) free(menu->fragment_shader_file);
    if (menu->fragment_shader_code) free(menu->fragment_shader_code);

    if (menu->music) Mix_FreeMusic(menu->music);
    if (menu->surface) SDL_FreeSurface(menu->surface);
    free_video(&menu->video);
    free_soundboard(&menu->win_board);
    free_soundboard(&menu->lose_board);
    free_soundboard(&menu->login_board);

    memset(menu, 0, sizeof(struct retrogauntlet_menu));

    return true;
}

#define TERM_W 80
#define TERM_H 25

void menu_draw(struct retrogauntlet_menu *menu) {
    if (!menu || !menu->surface) return;

    //No change --> no need to update.
    if (strcmp(menu->text, menu->last_text) == 0) return;

    strcpy(menu->last_text, menu->text);

    SDL_Surface * const surf = menu->surface;
    const uint32_t fg = SDL_MapRGBA(surf->format, menu->front_color.r, menu->front_color.g, menu->front_color.b, 0xff);
    const uint32_t bg = SDL_MapRGBA(surf->format, menu->back_color.r, menu->back_color.g, menu->back_color.b, 0xff);

    if (SDL_MUSTLOCK(surf) == SDL_TRUE) SDL_LockSurface(surf);
    
    //Pitch in dwords.
    const int pitch = surf->pitch/sizeof(uint32_t);
    uint32_t *p;

    //Clear screen.
    p = (uint32_t *)surf->pixels;
    for (int i = 0; i < surf->h*pitch; ++i) *p++ = bg;

    //Clear terminal.
    memset(menu->terminal, 0, TERM_W*TERM_H);
    
    //Create box.
    menu->terminal[0] = 218;
    for (int x = 1; x < TERM_W - 1; ++x) menu->terminal[x] = 196;
    menu->terminal[TERM_W - 1] = 191;
    for (int y = 1; y < TERM_H - 1; ++y) {
        menu->terminal[TERM_W*y + 0] = 179;
        menu->terminal[TERM_W*y + TERM_W - 1] = 179;
    }
    menu->terminal[TERM_W*(TERM_H - 1) + 0] = 192;
    for (int x = 1; x < TERM_W - 1; ++x) menu->terminal[TERM_W*(TERM_H - 1) + x] = 196;
    menu->terminal[TERM_W*TERM_H - 1] = 217;
    
    //And crossbar.
    menu->terminal[TERM_W*2 + 0] = 198;
    for (int x = 1; x < TERM_W - 1; ++x) menu->terminal[TERM_W*2 + x] = 205;
    menu->terminal[TERM_W*2 + TERM_W - 1] = 181;

    //Add Retro Gauntlet logo.
    for (int x = 0; x < TERM_W - 2 && x < (int)strlen(RETRO_GAUNTLET_LOGO); ++x) menu->terminal[TERM_W + 1 + x] = RETRO_GAUNTLET_LOGO[x];

    //Draw menu text inside terminal box.
    const uint8_t *t = (const uint8_t *)menu->text;

    for (int y = 3; y < TERM_H - 1; ++y) {
        uint8_t c;

        for (int x = 1; x < TERM_W - 1; ++x) {
           c = *t++;

           if (c == 0 || c == '\n') break;

           menu->terminal[TERM_W*y + x] = c;
        }

        if (c == 0) break;
    }
    
    //Fill 720x400 screen with 80x25 text consisting of 9x16 character glyphs.
    t = menu->terminal;

    p = (uint32_t *)surf->pixels;
    for (int y = 0; y < TERM_H; ++y) {
        uint32_t *p2 = p;
        uint8_t c;

        for (int x = 0; x < TERM_W; ++x, p2 += 9) {
            //Get character.
            c = *t++;

            //Draw single character.
            uint32_t *p3 = p2;
            const uint8_t *f = &dos_font[16*(int)c];

            for (int y2 = 0; y2 < 16; ++y2, p3 += pitch - 9) {
                uint8_t l = *f++;

                for (int x2 = 0; x2 < 7; ++x2, l <<= 1) *p3++ = ((l & 128) != 0 ? fg : bg);
                
                //http://support.microsoft.com/kb/59953: The width of CGA, EGA, MCGA, and VGA text characters is 8 pixels. In the case of VGA however, 9 pixels are actually used for displaying the characters. The 9th pixel is appended to the right end of each pixel row. If the character being displayed has an ASCII code ranging from 192 to 223 and the 8th pixel in a given pixel row is on, the 9th pixel in that row will be on also. If the 8th pixel in the row is off or the ASCII code for the character is not in the range 192 to 223, the 9th pixel will not be turned on.
                *p3++ = ((l & 128) != 0 ? fg : bg);
                *p3++ = ((l & 128) != 0 && c >= 192 && c < 224 ? fg : bg);
            }
        }

        p += 16*pitch;
    }
    
    if (SDL_MUSTLOCK(surf) == SDL_TRUE) SDL_UnlockSurface(surf);
}

void menu_escape(struct retrogauntlet_menu *menu, const char *format, ...) {
    if (!menu || !format) return;

    if (menu->state != RETRO_GAUNTLET_STATE_MESSAGE && menu->state != RETRO_GAUNTLET_STATE_QUIT_CONFIRM) menu->last_state = menu->state;
    menu->state = RETRO_GAUNTLET_STATE_QUIT_CONFIRM;
    
    va_list args;

    va_start(args, format);
    vsnprintf(menu->text, NR_RETRO_GAUNTLET_MENU_TEXT, format, args);
    va_end(args);
    
    menu_draw(menu);
}

void menu_draw_message(struct retrogauntlet_menu *menu, const char *format, ...) {
    if (!menu || !format) return;
    
    if (menu->state != RETRO_GAUNTLET_STATE_MESSAGE && menu->state != RETRO_GAUNTLET_STATE_QUIT_CONFIRM) menu->last_state = menu->state;
    menu->state = RETRO_GAUNTLET_STATE_MESSAGE;
    
    va_list args;

    va_start(args, format);
    vsnprintf(menu->text, NR_RETRO_GAUNTLET_MENU_TEXT, format, args);
    va_end(args);
    
    menu_draw(menu);
}

bool menu_start_mixer(struct retrogauntlet_menu *menu) {
    if (!menu) {
        fprintf(ERROR_FILE, "menu_start_mixer: Invalid menu!\n");
        return false;
    }

    if (menu->mixer_enabled) {
        fprintf(WARN_FILE, "menu_start_mixer: Mixer is already enabled!\n");
        return true;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0) {
        fprintf(ERROR_FILE, "menu_start_mixer: Unable to open audio: %s!\n", SDL_GetError());
        return false;
    }

    //Play menu music.
    if (menu->music_file) {
        if (!(menu->music = Mix_LoadMUS(menu->music_file))) {
            fprintf(WARN_FILE, "menu_start_mixer: Unable to open music file '%s': %s!\n", menu->music_file, SDL_GetError());
        }
        
        if (Mix_PlayMusic(menu->music, -1) < 0) {
            fprintf(WARN_FILE, "menu_start_mixer: Unable to play music: %s!\n", SDL_GetError());
        }
        else {
            Mix_VolumeMusic(MIX_MAX_VOLUME/3);

            if (menu->music_position > 0) Mix_SetMusicPosition((double)menu->music_position/1000.0);
            
            menu->music_start_time = SDL_GetTicks();
        }
    }

    menu->mixer_enabled = true;

    return true;
}

bool menu_stop_mixer(struct retrogauntlet_menu *menu) {
    if (!menu) {
        fprintf(ERROR_FILE, "menu_stop_mixer: Invalid menu!\n");
        return false;
    }
    
    if (!menu->mixer_enabled) {
        fprintf(WARN_FILE, "menu_stop_mixer: Mixer is already disabled!\n");
        return true;
    }

    //Stop music.
    Mix_HaltMusic();
    
    if (menu->music) {
        menu->music_position += SDL_GetTicks() - menu->music_start_time;
        Mix_FreeMusic(menu->music);
        menu->music = NULL;
    }

    //Stop samples.
    Mix_HaltChannel(-1);
    
    //Give back audio.
    Mix_CloseAudio();

    menu->mixer_enabled = false;

    return true;
}

