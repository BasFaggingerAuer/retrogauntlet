/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <GL/glew.h>
#include <GL/gl.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <SDL_mixer.h>

#include "retrogauntlet.h"
#include "stringextra.h"
#include "net.h"
#include "blowfish.h"
#include "glcheck.h"
#include "glvideo.h"
#include "core.h"
#include "sdlglcoreinterface.h"
#include "gauntlet.h"
#include "menu.h"
#include "gauntletgame.h"

//libretro core and related variables.
#define NR_PLAYERS 1

//Global state variable for interfacing between SDL, OpenGL, and libretro.
struct gauntlet_game _rg_state = {0};

//Connect SDL to libretro.
const char *env_get_system_directory() {
    return "system";
}

const char *env_get_save_directory() {
    return "saves";
}

bool env_shutdown() {
    _rg_state.sgci.core_keep_running = false;
    return true;
}

bool env_set_pixel_format(const enum retro_pixel_format fmt) {
    return video_set_pixel_format(&_rg_state.sgci.video, fmt);
}

bool env_set_input_descriptors(const struct retro_input_descriptor *desc) {
    for ( ; desc->description; desc++) {
        fprintf(INFO_FILE, "CORE input: port %u, device %u, index %u, id %u, %s\n", desc->port, desc->device, desc->index, desc->id, desc->description);
    }
    
    return true;
}

uintptr_t env_get_current_framebuffer() {
    return _rg_state.sgci.video.frame_buffer;
}

bool env_set_hw_render(struct retro_hw_render_callback *cb) {
    if (cb->context_type != RETRO_HW_CONTEXT_OPENGL &&
        cb->context_type != RETRO_HW_CONTEXT_OPENGL_CORE) {
        return false;
    }
    
    cb->get_current_framebuffer = env_get_current_framebuffer;
    cb->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
    _rg_state.sgci.video.core_callback = *cb;
    return true;
}

bool env_set_variables(struct retro_variable *vars) {
    return set_core_variables(&_rg_state.sgci.core, vars);
}

bool env_get_variable(struct retro_variable *var) {
    var->value = get_core_variable(&_rg_state.sgci.core, var);
    return (var->value != NULL);
}

bool env_set_frame_time_callback(struct retro_frame_time_callback *cb) {
    _rg_state.sgci.core_frame_time_callback = cb->callback;
    _rg_state.sgci.core.reference_frame_time = cb->reference;
    return true;
}

bool env_set_audio_callback(const struct retro_audio_callback * UNUSED(cb)) {
    //_rg_state.sgci.core_audio_callback = cb->callback;
    //_rg_state.sgci.core_audio_set_state_callback = cb->set_state;
    //FIXME: Disable for now (not used).
    return false;
}

void core_set_led_state(int led, int state) {
    fprintf(CORE_FILE, "LED %d: %d\n", led, state);
}

bool env_get_led_interface(struct retro_led_interface *led) {
    led->set_led_state = core_set_led_state;
    return true;
}

bool env_get_input_device_capabilities(uint64_t *cap) {
    *cap = (1 << RETRO_DEVICE_JOYPAD) |
           (1 << RETRO_DEVICE_MOUSE) |
           (1 << RETRO_DEVICE_KEYBOARD);
    return true;
}

#ifndef _WIN32
void core_log(enum retro_log_level UNUSED(level), const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(CORE_FILE, format, args);
    va_end(args);
}
#else
void core_log(enum retro_log_level UNUSED(level), const char *UNUSED(format), ...) {
    //Do not log anything in Windows to prevent slowdowns.
}
#endif

bool env_get_log_interface(struct retro_log_callback *cb) {
    cb->log = core_log;
    return true;
}

retro_time_t core_perf_get_time_usec() {
    return (retro_time_t)SDL_GetTicks()*1000;
}

retro_perf_tick_t core_perf_get_counter() {
    return (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

uint64_t core_get_cpu_features() {
    uint64_t flags = 0;

    flags |= (SDL_HasAVX() ? RETRO_SIMD_AVX : 0);
#if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 4)
    flags |= (SDL_HasAVX2() ? RETRO_SIMD_AVX2 : 0);
#endif
    flags |= (SDL_HasMMX() ? RETRO_SIMD_MMX : 0);
    flags |= (SDL_HasSSE() ? RETRO_SIMD_SSE | RETRO_SIMD_MMXEXT : 0);
    flags |= (SDL_HasSSE2() ? RETRO_SIMD_SSE2 : 0);
    flags |= (SDL_HasSSE3() ? RETRO_SIMD_SSE3 : 0);
    flags |= (SDL_HasSSE41() ? RETRO_SIMD_SSE4 : 0);
    flags |= (SDL_HasSSE42() ? RETRO_SIMD_SSE42 : 0);
    //TODO: RETRO_SIMD_POPCNT | RETRO_SIMD_MOVBE | RETRO_SIMD_CMOV | RETRO_SIMD_ASIMD | RETRO_SIMD_CMOV

    return flags;
}

void core_perf_register(struct retro_perf_counter *counter) {
    counter->registered = true;
}

void core_perf_start(struct retro_perf_counter *counter) {
    counter->start = core_perf_get_counter();
}

void core_perf_stop(struct retro_perf_counter *counter) {
    counter->total = core_perf_get_counter() - counter->start;
}

void core_perf_log() {
    
}

bool env_get_perf_interface(struct retro_perf_callback *cb) {
    cb->get_time_usec = core_perf_get_time_usec;
    cb->get_cpu_features = core_get_cpu_features;
    cb->get_perf_counter = core_perf_get_counter;
    cb->perf_register = core_perf_register;
    cb->perf_start = core_perf_start;
    cb->perf_stop = core_perf_stop;
    cb->perf_log = core_perf_log;
    return true;
}

bool env_set_system_av_info(struct retro_system_av_info *info) {
    video_set_geometry(&_rg_state.sgci.video, &info->geometry);
    _rg_state.sgci.core.frames_per_second = info->timing.fps;
    _rg_state.sgci.core.sample_rate = info->timing.sample_rate;
    return true;
}

bool env_set_controller_info(struct retro_controller_info *info) {
    return set_core_controller_infos(&_rg_state.sgci.core, info);
}

bool env_set_core_options_update_display_callback(struct retro_core_options_update_display_callback *cb) {
    _rg_state.sgci.core_options_update_display_callback = cb->callback;
    return true;
}

bool env_set_variable(const struct retro_variable *var) {
    if (!var) return true;
    return set_core_variable(&_rg_state.sgci.core, var->key, var->value);
}

bool env_set_memory_maps(const struct retro_memory_map *maps) {
    return set_core_memory_maps(&_rg_state.sgci.core, maps);
}

bool env_set_minimum_audio_latency(const unsigned latency) {
    _rg_state.sgci.audio_latency = latency;
    return true;
}

bool setup_sdl_opengl_environment(unsigned cmd, void *data) {
   switch (cmd) {
        //These options are explicitly not supported.
        case RETRO_ENVIRONMENT_SET_ROTATION:
        case RETRO_ENVIRONMENT_GET_OVERSCAN:
        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE:
        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK:
        case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
        case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
        case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
        case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
        case RETRO_ENVIRONMENT_GET_MIDI_INTERFACE:
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
        case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
        case RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE:
        case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
        case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
        case RETRO_ENVIRONMENT_GET_THROTTLE_STATE:
        case RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT:
        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
            return false;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            *(bool *)data = true;
            return true;
        case RETRO_ENVIRONMENT_SET_MESSAGE:
            fprintf(CORE_FILE, "%s\n", ((const struct retro_message *)data)->msg);
            return true;
        case RETRO_ENVIRONMENT_SHUTDOWN:
            return env_shutdown();
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            *(const char **)data = env_get_system_directory();
            return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            return env_set_pixel_format(*(const enum retro_pixel_format *)data);
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
            return env_set_input_descriptors((const struct retro_input_descriptor *)data);
        case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
            _rg_state.sgci.core_keyboard_callback = ((const struct retro_keyboard_callback *)data)->callback;
            return true;
        case RETRO_ENVIRONMENT_SET_HW_RENDER:
            return env_set_hw_render((struct retro_hw_render_callback *)data);
        case RETRO_ENVIRONMENT_GET_VARIABLE:
            return env_get_variable((struct retro_variable *)data);
        case RETRO_ENVIRONMENT_SET_VARIABLES:
            return env_set_variables((struct retro_variable *)data);
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            return was_any_core_variable_updated(&_rg_state.sgci.core);
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
            _rg_state.sgci.core.can_load_null_game = *(bool *)data;
            return true;
        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
            *(const char **)data = _rg_state.sgci.core.full_path;
            return true;
        case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK:
            return env_set_frame_time_callback((struct retro_frame_time_callback *)data);
        case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK:
            return env_set_audio_callback((const struct retro_audio_callback *)data);
        case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
            return env_get_input_device_capabilities((uint64_t *)data);
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            return env_get_log_interface((struct retro_log_callback *)data);
        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
            return env_get_perf_interface((struct retro_perf_callback *)data);
        case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
            *(const char **)data = env_get_system_directory();
            return true;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            *(const char **)data = env_get_save_directory();
            return true;
        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
            return env_set_system_av_info((struct retro_system_av_info *)data);
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
            video_set_geometry(&_rg_state.sgci.video, (struct retro_game_geometry *)data);
            return true;
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
            return env_set_controller_info((struct retro_controller_info *)data);
        case RETRO_ENVIRONMENT_GET_USERNAME:
            *(const char **)data = _rg_state.menu.player_name;
            return (data != NULL);
        case RETRO_ENVIRONMENT_GET_LANGUAGE:
            *(unsigned *)data = RETRO_LANGUAGE_ENGLISH;
            return true;
        case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT:
            return true;
        case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
            return env_get_led_interface((struct retro_led_interface *)data);
        case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
            *(int *)data = 1 | 2;
            //FIXME: Fast save states 4?
            return true;
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
            *(bool *)data = false;
            return true;
        case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE:
            *(float *)data = 60.0f;
            return true;
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
            *(unsigned *)data = 0;
            return true;
        case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
            //Always use modern OpenGL.
            *(unsigned *)data = RETRO_HW_CONTEXT_OPENGL_CORE;
            return true;
        case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION:
            *(unsigned *)data = 1;
            return true;
        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
            *(unsigned *)data = 1;
            return true;
        case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
            fprintf(CORE_FILE, "%s\n", ((const struct retro_message_ext *)data)->msg);
            return true;
        case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS:
            *(unsigned *)data = NR_PLAYERS;
            return true;
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
            return env_set_core_options_update_display_callback((struct retro_core_options_update_display_callback *)data);
        case RETRO_ENVIRONMENT_SET_VARIABLE:
            return env_set_variable((const struct retro_variable *)data);
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
            return env_set_memory_maps((const struct retro_memory_map *)data);
        case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
            return env_set_minimum_audio_latency(*(unsigned *)data);
        default:
            fprintf(WARN_FILE, "environment_callback: Unknown cmd %u!\n", cmd);
            return false;
   }
}

void sdl_opengl_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    video_refresh_from_libretro(&_rg_state.sgci.video, data, width, height, pitch);
}

size_t sdl_audio_sample_batch(const int16_t *data, size_t frames) {
    audio_refresh(&_rg_state.sgci, data, frames);
    return 0;
}

void sdl_audio_sample(int16_t left, int16_t right) {
    int16_t data[] = {left, right};
    
    sdl_audio_sample_batch(data, 1);
}

void sdl_input_poll() {
    //Only poll mouse.
    _rg_state.sgci.mouse.buttons = SDL_GetRelativeMouseState(&_rg_state.sgci.mouse.relative_x, &_rg_state.sgci.mouse.relative_y);
    SDL_GetMouseState(&_rg_state.sgci.mouse.absolute_x, &_rg_state.sgci.mouse.absolute_x);
}

int16_t sdl_input_state(unsigned port, unsigned device, unsigned UNUSED(index), unsigned id) {
    if (port >= NR_PLAYERS) return 0;

    return sdl_gl_if_get_input_state(&_rg_state.sgci, device, id);
}

bool setup_sdl_app_for_gauntlet(const struct gauntlet *g) {
    //Initialize app for libretro.
    if (!create_sdl_gl_if(&_rg_state.sgci)) return false;

    video_set_window(&_rg_state.sgci.video, _rg_state.menu.video.window_width, _rg_state.menu.video.window_height);
    
    //Load libretro core and ROM.
    if (!load_core_from_file(&_rg_state.sgci.core,
            g->core_library_file, g->rom_file, g->core_variables_file,
            setup_sdl_opengl_environment,
            sdl_opengl_video_refresh,
            sdl_audio_sample,
            sdl_audio_sample_batch,
            sdl_input_poll,
            sdl_input_state)) return false;
    
    //Create video/audio buffers necessary for the current core.
    if (!sdl_gl_if_create_core_buffers(&_rg_state.sgci)) return false;

    //Read shaders.
    if (!video_set_shaders(&_rg_state.sgci.video, NULL, _rg_state.menu.fragment_shader_code)) return false;
    
    return true;
}

//Set up global retrogauntlet instance.
bool retrogauntlet_initialize(const char *data_directory, SDL_Window *window) {
    if (_rg_state.window) {
        fprintf(ERROR_FILE, "Retrogauntlet instance is already running!\n");
        return false;
    }

    int width = 640;
    int height = 480;

    SDL_GetWindowSize(window, &width, &height);

    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glViewport(0, 0, width, height));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    
    //Setup game.
    if (!create_game(&_rg_state, data_directory, window)) {
        fprintf(ERROR_FILE, "Unable to initialize game!\n");
        return false;
    }

    //Initialize networking.
    if (!net_init()) {
        return false;
    }
    
    return true;
}

//Free global retrogauntlet instance.
bool retrogauntlet_quit() {
    net_quit();
    free_game(&_rg_state);
    return true;
}

bool retrogauntlet_keep_running() {
    return _rg_state.keep_running;
}

bool retrogauntlet_fullscreen() {
    return _rg_state.fullscreen;
}

//Handle SDL keydown events.
bool retrogauntlet_sdl_event(const SDL_Event event) {
    if (!_rg_state.window) return false;
    
    game_sdl_event(&_rg_state, event);
    return true;
}

//Create the next frame.
bool retrogauntlet_frame_update() {
    if (!_rg_state.window) return false;
    
    //Are we going to start a gauntlet?
    if ((_rg_state.menu.state == RETRO_GAUNTLET_STATE_SELECT_GAUNTLET ||
         _rg_state.menu.state == RETRO_GAUNTLET_STATE_LOBBY_HOST ||
         _rg_state.menu.state == RETRO_GAUNTLET_STATE_LOBBY_CLIENT) && _rg_state.gauntlet.ini_file) {
        _rg_state.start_ticks = 0;
        _rg_state.nr_frames = 0;
        menu_stop_mixer(&_rg_state.menu);
        
        //Set up global interface with SDL.
        if (setup_sdl_app_for_gauntlet(&_rg_state.gauntlet) &&
            gauntlet_start(&_rg_state.gauntlet, &_rg_state.sgci)) {
            fprintf(INFO_FILE, "Set up '%s'.\n", _rg_state.gauntlet.title);

            //Gauntlet was started successfully.
            _rg_state.menu.state = RETRO_GAUNTLET_STATE_RUN_CORE;
        }
        else {
            //Failed to start gauntlet.
            game_stop_gauntlet(&_rg_state);
            menu_start_mixer(&_rg_state.menu);
            menu_draw_message(&_rg_state.menu, "Unable to start gauntlet!");
        }
    }
    
    game_update(&_rg_state);
    return true;
}

