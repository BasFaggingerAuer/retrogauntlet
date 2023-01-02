/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include "sdlglcoreinterface.h"

#include "stringextra.h"

//Setup a mapping from SDL_GameControllerButton to libretro controller buttons.
unsigned *create_sdl_controller_button_to_retro_pad_map() {
    unsigned *map = calloc(SDL_CONTROLLER_BUTTON_MAX, sizeof(unsigned));

    if (!map) return NULL;
    
    map[SDL_CONTROLLER_BUTTON_A] = RETRO_DEVICE_ID_JOYPAD_A;
    map[SDL_CONTROLLER_BUTTON_B] = RETRO_DEVICE_ID_JOYPAD_B;
    map[SDL_CONTROLLER_BUTTON_X] = RETRO_DEVICE_ID_JOYPAD_X;
    map[SDL_CONTROLLER_BUTTON_Y] = RETRO_DEVICE_ID_JOYPAD_Y;
    map[SDL_CONTROLLER_BUTTON_BACK] = RETRO_DEVICE_ID_JOYPAD_SELECT;
    map[SDL_CONTROLLER_BUTTON_START] = RETRO_DEVICE_ID_JOYPAD_START;
    map[SDL_CONTROLLER_BUTTON_DPAD_UP] = RETRO_DEVICE_ID_JOYPAD_UP;
    map[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    map[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    map[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    map[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = RETRO_DEVICE_ID_JOYPAD_L;
    map[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = RETRO_DEVICE_ID_JOYPAD_R;
#ifdef SDL_CONTROLLER_BUTTON_PADDLE1
    //Fix for older SDL2 versions.
    map[SDL_CONTROLLER_BUTTON_PADDLE1] = RETRO_DEVICE_ID_JOYPAD_L2;
    map[SDL_CONTROLLER_BUTTON_PADDLE2] = RETRO_DEVICE_ID_JOYPAD_R2;
    map[SDL_CONTROLLER_BUTTON_PADDLE3] = RETRO_DEVICE_ID_JOYPAD_L3;
    map[SDL_CONTROLLER_BUTTON_PADDLE4] = RETRO_DEVICE_ID_JOYPAD_R3;
#endif

    return map;
}

//Setup a mapping from SDL scancodes to libretro keys.
unsigned *create_sdl_scancode_to_retro_key_map() {
    unsigned *map = calloc(SDL_NUM_SCANCODES, sizeof(unsigned));

    if (!map) return NULL;

    map[SDL_SCANCODE_BACKSPACE] = RETROK_BACKSPACE;
    map[SDL_SCANCODE_TAB] = RETROK_TAB;
    map[SDL_SCANCODE_CLEAR] = RETROK_CLEAR;
    map[SDL_SCANCODE_RETURN] = RETROK_RETURN;
    map[SDL_SCANCODE_PAUSE] = RETROK_PAUSE;
    map[SDL_SCANCODE_ESCAPE] = RETROK_ESCAPE;
    map[SDL_SCANCODE_SPACE] = RETROK_SPACE;
    map[SDL_SCANCODE_COMMA] = RETROK_COMMA;
    map[SDL_SCANCODE_MINUS] = RETROK_MINUS;
    map[SDL_SCANCODE_PERIOD] = RETROK_PERIOD;
    map[SDL_SCANCODE_SLASH] = RETROK_SLASH;
    map[SDL_SCANCODE_0] = RETROK_0;
    map[SDL_SCANCODE_1] = RETROK_1;
    map[SDL_SCANCODE_2] = RETROK_2;
    map[SDL_SCANCODE_3] = RETROK_3;
    map[SDL_SCANCODE_4] = RETROK_4;
    map[SDL_SCANCODE_5] = RETROK_5;
    map[SDL_SCANCODE_6] = RETROK_6;
    map[SDL_SCANCODE_7] = RETROK_7;
    map[SDL_SCANCODE_8] = RETROK_8;
    map[SDL_SCANCODE_9] = RETROK_9;
    map[SDL_SCANCODE_SEMICOLON] = RETROK_SEMICOLON;
    map[SDL_SCANCODE_EQUALS] = RETROK_EQUALS;
    map[SDL_SCANCODE_LEFTBRACKET] = RETROK_LEFTBRACKET;
    map[SDL_SCANCODE_BACKSLASH] = RETROK_BACKSLASH;
    map[SDL_SCANCODE_RIGHTBRACKET] = RETROK_RIGHTBRACKET;
    map[SDL_SCANCODE_A] = RETROK_a;
    map[SDL_SCANCODE_B] = RETROK_b;
    map[SDL_SCANCODE_C] = RETROK_c;
    map[SDL_SCANCODE_D] = RETROK_d;
    map[SDL_SCANCODE_E] = RETROK_e;
    map[SDL_SCANCODE_F] = RETROK_f;
    map[SDL_SCANCODE_G] = RETROK_g;
    map[SDL_SCANCODE_H] = RETROK_h;
    map[SDL_SCANCODE_I] = RETROK_i;
    map[SDL_SCANCODE_J] = RETROK_j;
    map[SDL_SCANCODE_K] = RETROK_k;
    map[SDL_SCANCODE_L] = RETROK_l;
    map[SDL_SCANCODE_M] = RETROK_m;
    map[SDL_SCANCODE_N] = RETROK_n;
    map[SDL_SCANCODE_O] = RETROK_o;
    map[SDL_SCANCODE_P] = RETROK_p;
    map[SDL_SCANCODE_Q] = RETROK_q;
    map[SDL_SCANCODE_R] = RETROK_r;
    map[SDL_SCANCODE_S] = RETROK_s;
    map[SDL_SCANCODE_T] = RETROK_t;
    map[SDL_SCANCODE_U] = RETROK_u;
    map[SDL_SCANCODE_V] = RETROK_v;
    map[SDL_SCANCODE_W] = RETROK_w;
    map[SDL_SCANCODE_X] = RETROK_x;
    map[SDL_SCANCODE_Y] = RETROK_y;
    map[SDL_SCANCODE_Z] = RETROK_z;
    map[SDL_SCANCODE_DELETE] = RETROK_DELETE;

    map[SDL_SCANCODE_KP_0] = RETROK_KP0;
    map[SDL_SCANCODE_KP_1] = RETROK_KP1;
    map[SDL_SCANCODE_KP_2] = RETROK_KP2;
    map[SDL_SCANCODE_KP_3] = RETROK_KP3;
    map[SDL_SCANCODE_KP_4] = RETROK_KP4;
    map[SDL_SCANCODE_KP_5] = RETROK_KP5;
    map[SDL_SCANCODE_KP_6] = RETROK_KP6;
    map[SDL_SCANCODE_KP_7] = RETROK_KP7;
    map[SDL_SCANCODE_KP_8] = RETROK_KP8;
    map[SDL_SCANCODE_KP_9] = RETROK_KP9;
    map[SDL_SCANCODE_KP_PERIOD] = RETROK_KP_PERIOD;
    map[SDL_SCANCODE_KP_DIVIDE] = RETROK_KP_DIVIDE;
    map[SDL_SCANCODE_KP_MULTIPLY] = RETROK_KP_MULTIPLY;
    map[SDL_SCANCODE_KP_MINUS] = RETROK_KP_MINUS;
    map[SDL_SCANCODE_KP_PLUS] = RETROK_KP_PLUS;
    map[SDL_SCANCODE_KP_ENTER] = RETROK_KP_ENTER;
    map[SDL_SCANCODE_KP_EQUALS] = RETROK_KP_EQUALS;

    map[SDL_SCANCODE_UP] = RETROK_UP;
    map[SDL_SCANCODE_DOWN] = RETROK_DOWN;
    map[SDL_SCANCODE_RIGHT] = RETROK_RIGHT;
    map[SDL_SCANCODE_LEFT] = RETROK_LEFT;
    map[SDL_SCANCODE_INSERT] = RETROK_INSERT;
    map[SDL_SCANCODE_HOME] = RETROK_HOME;
    map[SDL_SCANCODE_END] = RETROK_END;
    map[SDL_SCANCODE_PAGEUP] = RETROK_PAGEUP;
    map[SDL_SCANCODE_PAGEDOWN] = RETROK_PAGEDOWN;

    map[SDL_SCANCODE_F1] = RETROK_F1;
    map[SDL_SCANCODE_F2] = RETROK_F2;
    map[SDL_SCANCODE_F3] = RETROK_F3;
    map[SDL_SCANCODE_F4] = RETROK_F4;
    map[SDL_SCANCODE_F5] = RETROK_F5;
    map[SDL_SCANCODE_F6] = RETROK_F6;
    map[SDL_SCANCODE_F7] = RETROK_F7;
    map[SDL_SCANCODE_F8] = RETROK_F8;
    map[SDL_SCANCODE_F9] = RETROK_F9;
    map[SDL_SCANCODE_F10] = RETROK_F10;
    map[SDL_SCANCODE_F11] = RETROK_F11;
    map[SDL_SCANCODE_F12] = RETROK_F12;
    map[SDL_SCANCODE_F13] = RETROK_F13;
    map[SDL_SCANCODE_F14] = RETROK_F14;
    map[SDL_SCANCODE_F15] = RETROK_F15;

    map[SDL_SCANCODE_NUMLOCKCLEAR] = RETROK_NUMLOCK;
    map[SDL_SCANCODE_CAPSLOCK] = RETROK_CAPSLOCK;
    map[SDL_SCANCODE_SCROLLLOCK] = RETROK_SCROLLOCK;
    map[SDL_SCANCODE_RSHIFT] = RETROK_RSHIFT;
    map[SDL_SCANCODE_LSHIFT] = RETROK_LSHIFT;
    map[SDL_SCANCODE_RCTRL] = RETROK_RCTRL;
    map[SDL_SCANCODE_LCTRL] = RETROK_LCTRL;
    map[SDL_SCANCODE_RALT] = RETROK_RALT;
    map[SDL_SCANCODE_LALT] = RETROK_LALT;
    map[SDL_SCANCODE_RGUI] = RETROK_RMETA;
    map[SDL_SCANCODE_LGUI] = RETROK_LMETA;
    map[SDL_SCANCODE_LGUI] = RETROK_LSUPER;
    map[SDL_SCANCODE_RGUI] = RETROK_RSUPER;
    map[SDL_SCANCODE_MODE] = RETROK_MODE;
    map[SDL_SCANCODE_APPLICATION] = RETROK_COMPOSE;

    map[SDL_SCANCODE_HELP] = RETROK_HELP;
    map[SDL_SCANCODE_PRINTSCREEN] = RETROK_PRINT;
    map[SDL_SCANCODE_SYSREQ] = RETROK_SYSREQ;
    map[SDL_SCANCODE_PAUSE] = RETROK_BREAK;
    map[SDL_SCANCODE_MENU] = RETROK_MENU;
    map[SDL_SCANCODE_POWER] = RETROK_POWER;
    map[SDL_SCANCODE_UNDO] = RETROK_UNDO;

    return map;
}

//Setup a mapping from SDL scancodes to libretro game contoller buttons. Per the snes9x default controls.
unsigned *create_sdl_scancode_to_retro_pad_map() {
    unsigned *map = calloc(SDL_NUM_SCANCODES, sizeof(unsigned));

    if (!map) return NULL;

    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) map[i] = RETRO_DEVICE_JOYPAD_NR_BUTTONS;

    map[SDL_SCANCODE_D] = RETRO_DEVICE_ID_JOYPAD_A;
    map[SDL_SCANCODE_C] = RETRO_DEVICE_ID_JOYPAD_B;
    map[SDL_SCANCODE_S] = RETRO_DEVICE_ID_JOYPAD_X;
    map[SDL_SCANCODE_X] = RETRO_DEVICE_ID_JOYPAD_Y;
    map[SDL_SCANCODE_SPACE] = RETRO_DEVICE_ID_JOYPAD_SELECT;
    map[SDL_SCANCODE_RETURN] = RETRO_DEVICE_ID_JOYPAD_START;
    map[SDL_SCANCODE_UP] = RETRO_DEVICE_ID_JOYPAD_UP;
    map[SDL_SCANCODE_DOWN] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    map[SDL_SCANCODE_LEFT] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    map[SDL_SCANCODE_RIGHT] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    map[SDL_SCANCODE_A] = RETRO_DEVICE_ID_JOYPAD_L;
    map[SDL_SCANCODE_Z] = RETRO_DEVICE_ID_JOYPAD_R;

    return map;
}

//Setup a mapping from libretro keys to SDL scancodes.
SDL_Scancode *create_retro_key_to_sdl_scancode_map() {
    SDL_Scancode *map = calloc(RETROK_LAST, sizeof(SDL_Scancode));

    if (!map) return NULL;

    map[RETROK_BACKSPACE] = SDL_SCANCODE_BACKSPACE;
    map[RETROK_TAB] = SDL_SCANCODE_TAB;
    map[RETROK_CLEAR] = SDL_SCANCODE_CLEAR;
    map[RETROK_RETURN] = SDL_SCANCODE_RETURN;
    map[RETROK_PAUSE] = SDL_SCANCODE_PAUSE;
    map[RETROK_ESCAPE] = SDL_SCANCODE_ESCAPE;
    map[RETROK_SPACE] = SDL_SCANCODE_SPACE;
    map[RETROK_COMMA] = SDL_SCANCODE_COMMA;
    map[RETROK_MINUS] = SDL_SCANCODE_MINUS;
    map[RETROK_PERIOD] = SDL_SCANCODE_PERIOD;
    map[RETROK_SLASH] = SDL_SCANCODE_SLASH;
    map[RETROK_0] = SDL_SCANCODE_0;
    map[RETROK_1] = SDL_SCANCODE_1;
    map[RETROK_2] = SDL_SCANCODE_2;
    map[RETROK_3] = SDL_SCANCODE_3;
    map[RETROK_4] = SDL_SCANCODE_4;
    map[RETROK_5] = SDL_SCANCODE_5;
    map[RETROK_6] = SDL_SCANCODE_6;
    map[RETROK_7] = SDL_SCANCODE_7;
    map[RETROK_8] = SDL_SCANCODE_8;
    map[RETROK_9] = SDL_SCANCODE_9;
    map[RETROK_SEMICOLON] = SDL_SCANCODE_SEMICOLON;
    map[RETROK_EQUALS] = SDL_SCANCODE_EQUALS;
    map[RETROK_LEFTBRACKET] = SDL_SCANCODE_LEFTBRACKET;
    map[RETROK_BACKSLASH] = SDL_SCANCODE_BACKSLASH;
    map[RETROK_RIGHTBRACKET] = SDL_SCANCODE_RIGHTBRACKET;
    map[RETROK_a] = SDL_SCANCODE_A;
    map[RETROK_b] = SDL_SCANCODE_B;
    map[RETROK_c] = SDL_SCANCODE_C;
    map[RETROK_d] = SDL_SCANCODE_D;
    map[RETROK_e] = SDL_SCANCODE_E;
    map[RETROK_f] = SDL_SCANCODE_F;
    map[RETROK_g] = SDL_SCANCODE_G;
    map[RETROK_h] = SDL_SCANCODE_H;
    map[RETROK_i] = SDL_SCANCODE_I;
    map[RETROK_j] = SDL_SCANCODE_J;
    map[RETROK_k] = SDL_SCANCODE_K;
    map[RETROK_l] = SDL_SCANCODE_L;
    map[RETROK_m] = SDL_SCANCODE_M;
    map[RETROK_n] = SDL_SCANCODE_N;
    map[RETROK_o] = SDL_SCANCODE_O;
    map[RETROK_p] = SDL_SCANCODE_P;
    map[RETROK_q] = SDL_SCANCODE_Q;
    map[RETROK_r] = SDL_SCANCODE_R;
    map[RETROK_s] = SDL_SCANCODE_S;
    map[RETROK_t] = SDL_SCANCODE_T;
    map[RETROK_u] = SDL_SCANCODE_U;
    map[RETROK_v] = SDL_SCANCODE_V;
    map[RETROK_w] = SDL_SCANCODE_W;
    map[RETROK_x] = SDL_SCANCODE_X;
    map[RETROK_y] = SDL_SCANCODE_Y;
    map[RETROK_z] = SDL_SCANCODE_Z;
    map[RETROK_DELETE] = SDL_SCANCODE_DELETE;

    map[RETROK_KP0] = SDL_SCANCODE_KP_0;
    map[RETROK_KP1] = SDL_SCANCODE_KP_1;
    map[RETROK_KP2] = SDL_SCANCODE_KP_2;
    map[RETROK_KP3] = SDL_SCANCODE_KP_3;
    map[RETROK_KP4] = SDL_SCANCODE_KP_4;
    map[RETROK_KP5] = SDL_SCANCODE_KP_5;
    map[RETROK_KP6] = SDL_SCANCODE_KP_6;
    map[RETROK_KP7] = SDL_SCANCODE_KP_7;
    map[RETROK_KP8] = SDL_SCANCODE_KP_8;
    map[RETROK_KP9] = SDL_SCANCODE_KP_9;
    map[RETROK_KP_PERIOD] = SDL_SCANCODE_KP_PERIOD;
    map[RETROK_KP_DIVIDE] = SDL_SCANCODE_KP_DIVIDE;
    map[RETROK_KP_MULTIPLY] = SDL_SCANCODE_KP_MULTIPLY;
    map[RETROK_KP_MINUS] = SDL_SCANCODE_KP_MINUS;
    map[RETROK_KP_PLUS] = SDL_SCANCODE_KP_PLUS;
    map[RETROK_KP_ENTER] = SDL_SCANCODE_KP_ENTER;
    map[RETROK_KP_EQUALS] = SDL_SCANCODE_KP_EQUALS;

    map[RETROK_UP] = SDL_SCANCODE_UP;
    map[RETROK_DOWN] = SDL_SCANCODE_DOWN;
    map[RETROK_RIGHT] = SDL_SCANCODE_RIGHT;
    map[RETROK_LEFT] = SDL_SCANCODE_LEFT;
    map[RETROK_INSERT] = SDL_SCANCODE_INSERT;
    map[RETROK_HOME] = SDL_SCANCODE_HOME;
    map[RETROK_END] = SDL_SCANCODE_END;
    map[RETROK_PAGEUP] = SDL_SCANCODE_PAGEUP;
    map[RETROK_PAGEDOWN] = SDL_SCANCODE_PAGEDOWN;

    map[RETROK_F1] = SDL_SCANCODE_F1;
    map[RETROK_F2] = SDL_SCANCODE_F2;
    map[RETROK_F3] = SDL_SCANCODE_F3;
    map[RETROK_F4] = SDL_SCANCODE_F4;
    map[RETROK_F5] = SDL_SCANCODE_F5;
    map[RETROK_F6] = SDL_SCANCODE_F6;
    map[RETROK_F7] = SDL_SCANCODE_F7;
    map[RETROK_F8] = SDL_SCANCODE_F8;
    map[RETROK_F9] = SDL_SCANCODE_F9;
    map[RETROK_F10] = SDL_SCANCODE_F10;
    map[RETROK_F11] = SDL_SCANCODE_F11;
    map[RETROK_F12] = SDL_SCANCODE_F12;
    map[RETROK_F13] = SDL_SCANCODE_F13;
    map[RETROK_F14] = SDL_SCANCODE_F14;
    map[RETROK_F15] = SDL_SCANCODE_F15;

    map[RETROK_NUMLOCK] = SDL_SCANCODE_NUMLOCKCLEAR;
    map[RETROK_CAPSLOCK] = SDL_SCANCODE_CAPSLOCK;
    map[RETROK_SCROLLOCK] = SDL_SCANCODE_SCROLLLOCK;
    map[RETROK_RSHIFT] = SDL_SCANCODE_RSHIFT;
    map[RETROK_LSHIFT] = SDL_SCANCODE_LSHIFT;
    map[RETROK_RCTRL] = SDL_SCANCODE_RCTRL;
    map[RETROK_LCTRL] = SDL_SCANCODE_LCTRL;
    map[RETROK_RALT] = SDL_SCANCODE_RALT;
    map[RETROK_LALT] = SDL_SCANCODE_LALT;
    map[RETROK_RMETA] = SDL_SCANCODE_RGUI;
    map[RETROK_LMETA] = SDL_SCANCODE_LGUI;
    map[RETROK_LSUPER] = SDL_SCANCODE_LGUI;
    map[RETROK_RSUPER] = SDL_SCANCODE_RGUI;
    map[RETROK_MODE] = SDL_SCANCODE_MODE;
    map[RETROK_COMPOSE] = SDL_SCANCODE_APPLICATION;

    map[RETROK_HELP] = SDL_SCANCODE_HELP;
    map[RETROK_PRINT] = SDL_SCANCODE_PRINTSCREEN;
    map[RETROK_SYSREQ] = SDL_SCANCODE_SYSREQ;
    map[RETROK_BREAK] = SDL_SCANCODE_PAUSE;
    map[RETROK_MENU] = SDL_SCANCODE_MENU;
    map[RETROK_POWER] = SDL_SCANCODE_POWER;
    map[RETROK_UNDO] = SDL_SCANCODE_UNDO;
    map[RETROK_OEM_102] = SDL_SCANCODE_BACKSLASH;

    return map;
}

bool create_sdl_gl_if(struct sdl_gl_core_interface *sgci) {
    if (!sgci) {
        fprintf(ERROR_FILE, "create_sdl_gl_if: Invalid interface!\n");
        return false;
    }
    
    memset(sgci, 0, sizeof(struct sdl_gl_core_interface));

    sgci->core_keep_running = true;
    sgci->enable_mouse = false;
    sgci->mouse_button_mask = SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK | SDL_BUTTON_X1MASK | SDL_BUTTON_X2MASK;

    //Setup keymaps.
    sgci->sdl_controller_button_to_retro_pad_map = create_sdl_controller_button_to_retro_pad_map();
    sgci->retro_key_to_sdl_scancode_map = create_retro_key_to_sdl_scancode_map();
    sgci->sdl_scancode_to_retro_key_map = create_sdl_scancode_to_retro_key_map();
    sgci->sdl_scancode_to_retro_pad_map = create_sdl_scancode_to_retro_pad_map();
    sgci->sdl_scancode_override_key_map = calloc(SDL_NUM_SCANCODES, 1);

    if (!sgci->sdl_controller_button_to_retro_pad_map ||
        !sgci->retro_key_to_sdl_scancode_map ||
        !sgci->sdl_scancode_to_retro_key_map ||
        !sgci->sdl_scancode_to_retro_pad_map ||
        !sgci->sdl_scancode_override_key_map) {
        fprintf(ERROR_FILE, "create_sdl_gl_if: Unable to allocate controller/keyboard map!\n");
        return false;
    }

    memset(sgci->sdl_scancode_override_key_map, SCANCODE_NO_OVERRIDE, SDL_NUM_SCANCODES);

    //Setup controller(s).
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sgci->controller = SDL_GameControllerOpen(i);

            if (sgci->controller) {
                const char *name = SDL_GameControllerNameForIndex(i);

                if (name) {
                    fprintf(INFO_FILE, "Opened controller '%s'.\n", name);
                }
                else {
                    fprintf(INFO_FILE, "Opened controller.\n");
                }
                break;
            }
        }
    }

    memset(sgci->controller_buttons, 0, sizeof(sgci->controller_buttons));

    //Setup video.
    if (!create_video(&sgci->video)) return false;

    return true;
}

uint32_t next_pow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

size_t audio_refresh(struct sdl_gl_core_interface *sgci, const int16_t *data, size_t frames) {
    if (!sgci) {
        fprintf(ERROR_FILE, "audio_refresh: Invalid interface!\n");
        return 0;
    }

    if (!data || frames == 0 || sgci->audio_device_id == 0) return 0;

    SDL_LockAudioDevice(sgci->audio_device_id);

    size_t nr_write = 2*sizeof(int16_t)*frames;
    size_t first_write = nr_write;
    size_t rest_write = 0;
    size_t audio_buffer_end = (sgci->audio_buffer_start + sgci->audio_buffer_available) % sgci->nr_audio_buffer;
    
    if (audio_buffer_end + nr_write > sgci->nr_audio_buffer) {
        first_write = sgci->nr_audio_buffer - audio_buffer_end;
        rest_write = nr_write - first_write;
    }

    memcpy(sgci->audio_buffer + audio_buffer_end, (const uint8_t *)data, first_write);
    memcpy(sgci->audio_buffer, (const uint8_t *)data + first_write, rest_write);

    sgci->audio_buffer_available += nr_write;

    //Catch up if needed.
    if (sgci->audio_buffer_available > sgci->nr_audio_buffer) {
        sgci->audio_buffer_start = (sgci->audio_buffer_start + sgci->audio_buffer_available - sgci->nr_audio_buffer) % sgci->nr_audio_buffer;
        sgci->audio_buffer_available = sgci->nr_audio_buffer;
    }

    SDL_UnlockAudioDevice(sgci->audio_device_id);

    return frames;
}

void sdl_audio_cb(void *data, Uint8 *stream, int len) {
    struct sdl_gl_core_interface *sgci = (struct sdl_gl_core_interface *)data;
    size_t nr_write = min((size_t)len, sgci->audio_buffer_available);
    size_t first_write = nr_write;
    size_t rest_write = 0;

    if (sgci->audio_buffer_start + nr_write > sgci->nr_audio_buffer) {
        first_write = sgci->nr_audio_buffer - sgci->audio_buffer_start;
        rest_write = nr_write - first_write;
    }
    
    memcpy(stream, sgci->audio_buffer + sgci->audio_buffer_start, first_write);
    memcpy((uint8_t *)stream + first_write, sgci->audio_buffer, rest_write);
    memset((uint8_t *)stream + nr_write, 0, (size_t)len - nr_write);

    sgci->audio_buffer_start = (sgci->audio_buffer_start + nr_write) % sgci->nr_audio_buffer;
    sgci->audio_buffer_available -= nr_write;
}

bool sdl_gl_if_reset_audio(struct sdl_gl_core_interface *sgci) {
    if (!sgci || !sgci->audio_buffer || !sgci->sdl_scancode_override_key_map) {
        fprintf(ERROR_FILE, "sdl_gl_if_reset_audio: Invalid interface or no audio buffer initialized!\n");
        return false;
    }
    
    sgci->audio_buffer_start = 0;
    sgci->audio_buffer_available = 0;
    memset(sgci->audio_buffer, 0, sgci->nr_audio_buffer);
    return true;
}

bool sdl_gl_if_create_core_buffers(struct sdl_gl_core_interface *sgci) {
    if (!sgci || !sgci->core.retro_get_system_av_info) {
        fprintf(ERROR_FILE, "sdl_gl_if_create_buffers: Invalid interface or no core loaded!\n");
        return false;
    }
    
    struct retro_system_av_info core_av;
    
    memset(&core_av, 0, sizeof(struct retro_system_av_info));
    sgci->core.retro_get_system_av_info(&core_av);
    sgci->core.sample_rate = core_av.timing.sample_rate;
    sgci->core.frames_per_second = core_av.timing.fps;

    if (!video_set_geometry(&sgci->video, &core_av.geometry)) return false;
    if (!create_video_buffers(&sgci->video)) return false;

    //Audio buffers.
    SDL_AudioSpec audio_spec = {0};

    audio_spec.freq = (int)sgci->core.sample_rate;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 2;
    audio_spec.samples = 2048; //next_pow2((uint32_t)(0.25e-3*sgci->core.sample_rate*(double)sgci->audio_latency));
    audio_spec.callback = sdl_audio_cb;
    audio_spec.userdata = sgci;
    
    sgci->audio_device_id = SDL_OpenAudioDevice(NULL, 0, &audio_spec, &sgci->audio_spec, 0);
    
    if (sgci->audio_device_id == 0) {
        fprintf(ERROR_FILE, "sdl_gl_if_create_buffers: Unable to open SDL audio device: %s!\n", SDL_GetError());
        return false;
    }

    sgci->nr_audio_buffer = 2*sizeof(int16_t)*(size_t)sgci->audio_spec.channels*(size_t)sgci->audio_spec.samples;
    sgci->audio_buffer_start = 0;
    sgci->audio_buffer_available = 0;
    sgci->audio_buffer = (uint8_t *)calloc(sgci->nr_audio_buffer, 1);

    if (!sgci->audio_buffer) {
        fprintf(ERROR_FILE, "sdl_gl_if_create_buffers: Unable to allocate audio buffer of %zu samples!\n", sgci->nr_audio_buffer);
        return false;
    }

    SDL_PauseAudioDevice(sgci->audio_device_id, 0);

    fprintf(INFO_FILE, "Opened audio device with frequency %d, %d channels, and %u samples.\n", sgci->audio_spec.freq, sgci->audio_spec.channels, sgci->audio_spec.samples);
    
    return true;
}

bool free_sdl_gl_if(struct sdl_gl_core_interface *sgci) {
    if (!sgci) {
        fprintf(ERROR_FILE, "free_sdl_gl_if: Invalid interface!\n");
        return false;
    }
    
    free_core(&sgci->core);
    free_video(&sgci->video);
    if (sgci->audio_buffer) free(sgci->audio_buffer);
    if (sgci->sdl_scancode_override_key_map) free(sgci->sdl_scancode_override_key_map);
    if (sgci->sdl_scancode_to_retro_key_map) free(sgci->sdl_scancode_to_retro_key_map);
    if (sgci->sdl_scancode_to_retro_pad_map) free(sgci->sdl_scancode_to_retro_pad_map);
    if (sgci->retro_key_to_sdl_scancode_map) free(sgci->retro_key_to_sdl_scancode_map);
    if (sgci->sdl_controller_button_to_retro_pad_map) free(sgci->sdl_controller_button_to_retro_pad_map);
    if (sgci->controller) SDL_GameControllerClose(sgci->controller);
    if (sgci->audio_device_id) {
        SDL_PauseAudioDevice(sgci->audio_device_id, 1);
        SDL_CloseAudioDevice(sgci->audio_device_id);
    }

    memset(sgci, 0, sizeof(struct sdl_gl_core_interface));

    return true;
}

//Callback for libretro input queries.
int16_t sdl_gl_if_get_input_state(struct sdl_gl_core_interface *sgci, const unsigned device, const unsigned id) {
    if (!sgci) return 0;

    switch (device) {
        case RETRO_DEVICE_MOUSE:
            if (!sgci->enable_mouse) return 0;
            
            switch (id) {
                case RETRO_DEVICE_ID_MOUSE_X:
                    return sgci->mouse.relative_x;
                case RETRO_DEVICE_ID_MOUSE_Y:
                    return sgci->mouse.relative_y;
                case RETRO_DEVICE_ID_MOUSE_LEFT:
                    return ((sgci->mouse.buttons & sgci->mouse_button_mask & SDL_BUTTON_LMASK) != 0);
                case RETRO_DEVICE_ID_MOUSE_RIGHT:
                    return ((sgci->mouse.buttons & sgci->mouse_button_mask & SDL_BUTTON_RMASK) != 0);
                case RETRO_DEVICE_ID_MOUSE_MIDDLE:
                    return ((sgci->mouse.buttons & sgci->mouse_button_mask & SDL_BUTTON_MMASK) != 0);
                case RETRO_DEVICE_ID_MOUSE_BUTTON_4:
                    return ((sgci->mouse.buttons & sgci->mouse_button_mask & SDL_BUTTON_X1MASK) != 0);
                case RETRO_DEVICE_ID_MOUSE_BUTTON_5:
                    return ((sgci->mouse.buttons & sgci->mouse_button_mask & SDL_BUTTON_X2MASK) != 0);
                case RETRO_DEVICE_ID_MOUSE_WHEELUP:
                    return (sgci->mouse.wheel_y > 0);
                case RETRO_DEVICE_ID_MOUSE_WHEELDOWN:
                    return (sgci->mouse.wheel_y < 0);
                case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP:
                    return (sgci->mouse.wheel_x > 0);
                case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN:
                    return (sgci->mouse.wheel_x < 0);
                default:
                    return 0;
            }

            break;
        case RETRO_DEVICE_KEYBOARD:
            if (id < RETROK_LAST && sgci->retro_key_to_sdl_scancode_map) {
                const SDL_Scancode sc = sgci->retro_key_to_sdl_scancode_map[id];
                const uint8_t o = sgci->sdl_scancode_override_key_map[sc];
                
                if (o == SCANCODE_NO_OVERRIDE) return SDL_GetKeyboardState(NULL)[sc];

                return (o == SCANCODE_OVERRIDE_DOWN);
            }

            return 0;
        case RETRO_DEVICE_JOYPAD:
            if (!sgci->enable_controller) return 0;

            return (id < RETRO_DEVICE_JOYPAD_NR_BUTTONS ? sgci->controller_buttons[id] : 0);
        case RETRO_DEVICE_POINTER:
            if (!sgci->enable_mouse) return 0;

            //Determine scaled mouse position.
            const int edge = 32700;
            int x = -0x8000;
            int y = -0x8000;

            if (sgci->mouse.absolute_x >= 0 && sgci->mouse.absolute_x <= (int)sgci->video.window_width)
                x = ((2*sgci->mouse.absolute_x*0x7fff)/sgci->video.window_width) - 0x7fff;
            if (sgci->mouse.absolute_y >= 0 && sgci->mouse.absolute_y <= (int)sgci->video.window_height)
                y = ((2*sgci->mouse.absolute_y*0x7fff)/sgci->video.window_height) - 0x7fff;

            switch (id) {
                case RETRO_DEVICE_ID_POINTER_X:
                    return x;
                case RETRO_DEVICE_ID_POINTER_Y:
                    return y;
                case RETRO_DEVICE_ID_POINTER_PRESSED:
                    return ((sgci->mouse.buttons & SDL_BUTTON_LMASK) != 0);
                case RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN:
                    return !((x >= -edge) && (x <= edge) && (y >= -edge) && (y <= edge));
                default:
                    return 0;
            }

            break;
        case RETRO_DEVICE_LIGHTGUN:
            if (!sgci->enable_mouse) return 0;

            switch (id) {
                case RETRO_DEVICE_ID_LIGHTGUN_X:
                    return sgci->mouse.relative_x;
                case RETRO_DEVICE_ID_LIGHTGUN_Y:
                    return sgci->mouse.relative_y;
                case RETRO_DEVICE_ID_LIGHTGUN_TRIGGER:
                    return ((sgci->mouse.buttons & SDL_BUTTON_LMASK) != 0);
                case RETRO_DEVICE_ID_LIGHTGUN_TURBO:
                    return ((sgci->mouse.buttons & SDL_BUTTON_RMASK) != 0);
                case RETRO_DEVICE_ID_LIGHTGUN_CURSOR:
                    return ((sgci->mouse.buttons & SDL_BUTTON_MMASK) != 0);
                case RETRO_DEVICE_ID_LIGHTGUN_START:
                    return ((sgci->mouse.buttons & SDL_BUTTON_X1MASK) != 0);
                case RETRO_DEVICE_ID_LIGHTGUN_PAUSE:
                    return ((sgci->mouse.buttons & SDL_BUTTON_X2MASK) != 0);
            }

            break;
    }

    return 0;
}

//Convert SDL keyboard events to libretro callbacks.
void sdl_gl_if_keyboard_callback(struct sdl_gl_core_interface *sgci, const SDL_KeyboardEvent key) {
    if (!sgci) return;

    if (sgci->core_keyboard_callback && sgci->sdl_scancode_to_retro_key_map && key.keysym.scancode < SDL_NUM_SCANCODES) {
        SDL_Keymod sdl_mod = SDL_GetModState();
        uint16_t core_mod = 0;

        if (sdl_mod & KMOD_SHIFT) core_mod |= RETROKMOD_SHIFT;
        if (sdl_mod & KMOD_CTRL) core_mod |= RETROKMOD_CTRL;
        if (sdl_mod & KMOD_ALT) core_mod |= RETROKMOD_ALT;
        if (sdl_mod & KMOD_GUI) core_mod |= RETROKMOD_META;
        if (sdl_mod & KMOD_NUM) core_mod |= RETROKMOD_NUMLOCK;
        if (sdl_mod & KMOD_CAPS) core_mod |= RETROKMOD_CAPSLOCK;
#ifdef KMOD_SCROLL
        //Fix for older SDL2 versions.
        if (sdl_mod & KMOD_SCROLL) core_mod |= RETROKMOD_SCROLLOCK;
#endif

        if (sgci->sdl_scancode_override_key_map[key.keysym.scancode] == SCANCODE_NO_OVERRIDE) {
            sgci->core_keyboard_callback(key.state == SDL_PRESSED, sgci->sdl_scancode_to_retro_key_map[key.keysym.scancode], key.keysym.sym, core_mod);
        }
    }
}

bool sdl_gl_if_handle_event(struct sdl_gl_core_interface *sgci, const SDL_Event event) {
    if (!sgci) {
        fprintf(ERROR_FILE, "sdl_gl_if_handle_event: Invalid interface!\n");
        return false;
    }
    
    if (!sgci->core_keep_running) {
        fprintf(ERROR_FILE, "sdl_gl_if_handle_event: Core is not running!\n");
        return false;
    }

    switch (event.type) {
        case SDL_QUIT:
            sgci->core_keep_running = false;
            break;
        case SDL_KEYUP:
            sdl_gl_if_keyboard_callback(sgci, event.key);

            //Emulate controller with the keyboard if needed.
            if (!sgci->controller && sgci->sdl_scancode_to_retro_pad_map) sgci->controller_buttons[sgci->sdl_scancode_to_retro_pad_map[event.key.keysym.scancode]] = false;
            break;
        case SDL_KEYDOWN:
            sdl_gl_if_keyboard_callback(sgci, event.key);

            //Emulate controller with the keyboard if needed.
            if (!sgci->controller && sgci->sdl_scancode_to_retro_pad_map) sgci->controller_buttons[sgci->sdl_scancode_to_retro_pad_map[event.key.keysym.scancode]] = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            sgci->mouse.buttons |= event.button.button;
            break;
            break;
        case SDL_MOUSEBUTTONUP:
            sgci->mouse.buttons &= ~event.button.button;
            break;
        case SDL_MOUSEMOTION:
            sgci->mouse.buttons = event.motion.state;
            sgci->mouse.absolute_x = event.motion.x;
            sgci->mouse.absolute_y = event.motion.y;
            break;
        case SDL_MOUSEWHEEL:
            sgci->mouse.wheel_x = event.wheel.x;
            sgci->mouse.wheel_y = event.wheel.y;
            break;
        case SDL_CONTROLLERDEVICEADDED:
            //Immediately switch to newly plugged in controllers.
            if (sgci->controller) SDL_GameControllerClose(sgci->controller);
            
            sgci->controller = SDL_GameControllerOpen(event.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (sgci->controller) SDL_GameControllerClose(sgci->controller);
            sgci->controller = NULL;
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (sgci->sdl_controller_button_to_retro_pad_map) sgci->controller_buttons[sgci->sdl_controller_button_to_retro_pad_map[event.cbutton.button]] = true;
            break;
        case SDL_CONTROLLERBUTTONUP:
            if (sgci->sdl_controller_button_to_retro_pad_map) sgci->controller_buttons[sgci->sdl_controller_button_to_retro_pad_map[event.cbutton.button]] = false;
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    sgci->core_keep_running = false;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    video_set_window(&sgci->video, event.window.data1, event.window.data2);
                    break;
            }

            break;
    }

    return true;
}

bool sdl_gl_if_apply_command(struct sdl_gl_core_interface *sgci, const char *cmd, const char *var) {
    if (!sgci || !cmd || !var) {
        fprintf(ERROR_FILE, "sdl_gl_if_apply_command: Invalid interface or command or variable!\n");
        return false;
    }

    const int vari = atoi(var);
    const SDL_Keycode key = SDL_GetKeyFromName(var);
    
    if (strcmp(cmd, "run") == 0) {
        for (int i = 0; i < vari; ++i) {
            sgci->core.retro_run();
            SDL_Delay(1);
        }
    }
    else if (strcmp(cmd, "keydown") == 0 || strcmp(cmd, "keyup") == 0) {
        if (key == SDLK_UNKNOWN) {
            fprintf(ERROR_FILE, "sdl_gl_if_apply_command: Unknown key '%s' for '%s'!\n", var, cmd);
            return false;
        }

        sgci->core_keyboard_callback(strcmp(cmd, "keydown") == 0, sgci->sdl_scancode_to_retro_key_map[SDL_GetScancodeFromKey(key)], key, 0); 
    }
    else if (strcmp(cmd, "keyoverridedown") == 0) {
        if (key == SDLK_UNKNOWN) {
            fprintf(ERROR_FILE, "sdl_gl_if_apply_command: Unknown key '%s' for '%s'!\n", var, cmd);
            return false;
        }
        
        sgci->sdl_scancode_override_key_map[SDL_GetScancodeFromKey(key)] = SCANCODE_OVERRIDE_DOWN;
    }
    else if (strcmp(cmd, "keyoverrideup") == 0) {
        if (key == SDLK_UNKNOWN) {
            fprintf(ERROR_FILE, "sdl_gl_if_apply_command: Unknown key '%s' for '%s'!\n", var, cmd);
            return false;
        }
        
        sgci->sdl_scancode_override_key_map[SDL_GetScancodeFromKey(key)] = SCANCODE_OVERRIDE_UP;
    }
    else {
        fprintf(ERROR_FILE, "core_apply_command: Unknown command '%s'!\n", cmd);
        return false;
    }
    
    return true;
}

#define NR_COMMAND_LINE 4096

bool sdl_gl_if_run_commands_from_file(struct sdl_gl_core_interface *sgci, const char *commands_file) {
    if (!sgci || !commands_file) {
        fprintf(ERROR_FILE, "sdl_gl_if_run_commands_from_file: Invalid core or file!\n");
        return false;
    }

    fprintf(CORE_FILE, "Running commands from '%s'...\n", commands_file);

    FILE *f = fopen(commands_file, "r");
    char line[NR_COMMAND_LINE];
    char var_key[NR_COMMAND_LINE];
    char var_value[NR_COMMAND_LINE];

    if (!f) {
        fprintf(ERROR_FILE, "sdl_gl_if_run_commands_from_file: Unable to read commands file '%s'!\n", commands_file);
        return false;
    }

    while (fgets(line, NR_COMMAND_LINE, f)) {
        char *tok, *saveptr = NULL;

        if (IS_COMMENT_LINE(line)) continue;

        fprintf(CORE_FILE, "    %s", line);

        if ((tok = strtok_r(line, " ", &saveptr))) {
            strncpy_trim(var_key, tok, NR_COMMAND_LINE);

            if ((tok = strtok_r(NULL, " ", &saveptr))) {
                strncpy_trim(var_value, tok, NR_COMMAND_LINE);
                
                if (strlen(var_value) > 0) {
                    if (!sdl_gl_if_apply_command(sgci, var_key, var_value)) {
                        fprintf(ERROR_FILE, "sdl_gl_if_run_commands_from_file: Invalid command '%s'!\n", line);
                        fclose(f);
                        return false;
                    }
                }
            }
        }
    }

    fprintf(CORE_FILE, "\n");

    fclose(f);

    return true;
}

