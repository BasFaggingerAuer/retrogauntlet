/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//SDL and OpenGL interface for libretro cores.
#ifndef SDL_GL_APPLICATION_H__
#define SDL_GL_APPLICATION_H__

#include <SDL.h>

#include "libretro.h"
#include "glvideo.h"
#include "core.h"

#define RETRO_DEVICE_JOYPAD_NR_BUTTONS 16

struct sdl_gl_core_interface {
    //Video state.
    struct gl_video video;

    //Audio state.
    SDL_AudioDeviceID audio_device_id;
    unsigned audio_latency;
    SDL_AudioSpec audio_spec;
    uint8_t *audio_buffer;
    size_t nr_audio_buffer;
    size_t audio_buffer_start, audio_buffer_available;

    //Core state.
    bool core_keep_running;
    struct retro_core core;
    retro_keyboard_event_t core_keyboard_callback;
    retro_frame_time_callback_t core_frame_time_callback;
    retro_audio_callback_t core_audio_callback;
    retro_audio_set_state_callback_t core_audio_set_state_callback;
    retro_core_options_update_display_callback_t core_options_update_display_callback;

    //Game input devices.

    //Mouse
    struct retrogauntlet_mouse_state {
        int buttons;
        int clicks;
        int absolute_x;
        int absolute_y;
        int relative_x;
        int relative_y;
        int wheel_x;
        int wheel_y;
    } mouse;
    bool enable_mouse;

    //Keyboard.
    SDL_Scancode *retro_key_to_sdl_scancode_map;
    unsigned *sdl_scancode_to_retro_key_map;
    uint8_t *sdl_scancode_override_key_map;

    //Controller.
    unsigned *sdl_controller_button_to_retro_pad_map;
    bool controller_buttons[RETRO_DEVICE_JOYPAD_NR_BUTTONS];
    SDL_GameController *controller;
};

#define SCANCODE_NO_OVERRIDE 0
#define SCANCODE_OVERRIDE_DOWN 1
#define SCANCODE_OVERRIDE_UP 2

unsigned *create_sdl_controller_button_to_retro_pad_map();
unsigned *create_sdl_scancode_to_retro_key_map();
SDL_Scancode *create_retro_key_to_sdl_scancode_map();

bool create_sdl_gl_if(struct sdl_gl_core_interface *);
bool sdl_gl_if_create_core_buffers(struct sdl_gl_core_interface *);
bool sdl_gl_if_reset_audio(struct sdl_gl_core_interface *);
int16_t sdl_gl_if_get_input_state(struct sdl_gl_core_interface *, const unsigned, const unsigned);
bool sdl_gl_if_handle_event(struct sdl_gl_core_interface *, const SDL_Event);
size_t audio_refresh(struct sdl_gl_core_interface *, const int16_t *, size_t);
bool free_sdl_gl_if(struct sdl_gl_core_interface *);
bool sdl_gl_if_apply_command(struct sdl_gl_core_interface *, const char *, const char *);
bool sdl_gl_if_run_commands_from_file(struct sdl_gl_core_interface *, const char *);

#endif

