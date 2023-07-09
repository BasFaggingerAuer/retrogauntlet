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
struct gauntlet_game game;

//Connect SDL to libretro.
const char *env_get_system_directory() {
    return "system";
}

const char *env_get_save_directory() {
    return "saves";
}

bool env_shutdown() {
    game.sgci.core_keep_running = false;
    return true;
}

bool env_set_pixel_format(const enum retro_pixel_format fmt) {
    return video_set_pixel_format(&game.sgci.video, fmt);
}

bool env_set_input_descriptors(const struct retro_input_descriptor *desc) {
    for ( ; desc->description; desc++) {
        fprintf(INFO_FILE, "CORE input: port %u, device %u, index %u, id %u, %s\n", desc->port, desc->device, desc->index, desc->id, desc->description);
    }
    
    return true;
}

uintptr_t env_get_current_framebuffer() {
    return game.sgci.video.frame_buffer;
}

bool env_set_hw_render(struct retro_hw_render_callback *cb) {
    if (cb->context_type != RETRO_HW_CONTEXT_OPENGL &&
        cb->context_type != RETRO_HW_CONTEXT_OPENGL_CORE) {
        return false;
    }
    
    cb->get_current_framebuffer = env_get_current_framebuffer;
    cb->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
    game.sgci.video.core_callback = *cb;
    return true;
}

bool env_set_variables(struct retro_variable *vars) {
    return set_core_variables(&game.sgci.core, vars);
}

bool env_get_variable(struct retro_variable *var) {
    var->value = get_core_variable(&game.sgci.core, var);
    return (var->value != NULL);
}

bool env_set_frame_time_callback(struct retro_frame_time_callback *cb) {
    game.sgci.core_frame_time_callback = cb->callback;
    game.sgci.core.reference_frame_time = cb->reference;
    return true;
}

bool env_set_audio_callback(const struct retro_audio_callback * UNUSED(cb)) {
    //game.sgci.core_audio_callback = cb->callback;
    //game.sgci.core_audio_set_state_callback = cb->set_state;
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
    video_set_geometry(&game.sgci.video, &info->geometry);
    game.sgci.core.frames_per_second = info->timing.fps;
    game.sgci.core.sample_rate = info->timing.sample_rate;
    return true;
}

bool env_set_controller_info(struct retro_controller_info *info) {
    return set_core_controller_infos(&game.sgci.core, info);
}

bool env_set_core_options_update_display_callback(struct retro_core_options_update_display_callback *cb) {
    game.sgci.core_options_update_display_callback = cb->callback;
    return true;
}

bool env_set_variable(const struct retro_variable *var) {
    if (!var) return true;
    return set_core_variable(&game.sgci.core, var->key, var->value);
}

bool env_set_memory_maps(const struct retro_memory_map *maps) {
    return set_core_memory_maps(&game.sgci.core, maps);
}

bool env_set_minimum_audio_latency(const unsigned latency) {
    game.sgci.audio_latency = latency;
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
            game.sgci.core_keyboard_callback = ((const struct retro_keyboard_callback *)data)->callback;
            return true;
        case RETRO_ENVIRONMENT_SET_HW_RENDER:
            return env_set_hw_render((struct retro_hw_render_callback *)data);
        case RETRO_ENVIRONMENT_GET_VARIABLE:
            return env_get_variable((struct retro_variable *)data);
        case RETRO_ENVIRONMENT_SET_VARIABLES:
            return env_set_variables((struct retro_variable *)data);
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            return was_any_core_variable_updated(&game.sgci.core);
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
            game.sgci.core.can_load_null_game = *(bool *)data;
            return true;
        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
            *(const char **)data = game.sgci.core.full_path;
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
            video_set_geometry(&game.sgci.video, (struct retro_game_geometry *)data);
            return true;
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
            return env_set_controller_info((struct retro_controller_info *)data);
        case RETRO_ENVIRONMENT_GET_USERNAME:
            *(const char **)data = game.menu.player_name;
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
    video_refresh_from_libretro(&game.sgci.video, data, width, height, pitch);
}

size_t sdl_audio_sample_batch(const int16_t *data, size_t frames) {
    audio_refresh(&game.sgci, data, frames);
    return 0;
}

void sdl_audio_sample(int16_t left, int16_t right) {
    int16_t data[] = {left, right};
    
    sdl_audio_sample_batch(data, 1);
}

void sdl_input_poll() {
    //Only poll mouse.
    game.sgci.mouse.buttons = SDL_GetRelativeMouseState(&game.sgci.mouse.relative_x, &game.sgci.mouse.relative_y);
    SDL_GetMouseState(&game.sgci.mouse.absolute_x, &game.sgci.mouse.absolute_x);
}

int16_t sdl_input_state(unsigned port, unsigned device, unsigned UNUSED(index), unsigned id) {
    if (port >= NR_PLAYERS) return 0;

    return sdl_gl_if_get_input_state(&game.sgci, device, id);
}

bool setup_sdl_app_for_gauntlet(const struct gauntlet *g) {
    //Initialize app for libretro.
    if (!create_sdl_gl_if(&game.sgci)) return false;

    video_set_window(&game.sgci.video, game.menu.video.window_width, game.menu.video.window_height);
    
    //Load libretro core and ROM.
    if (!load_core_from_file(&game.sgci.core,
            g->core_library_file, g->rom_file, g->core_variables_file,
            setup_sdl_opengl_environment,
            sdl_opengl_video_refresh,
            sdl_audio_sample,
            sdl_audio_sample_batch,
            sdl_input_poll,
            sdl_input_state)) return false;
    
    //Create video/audio buffers necessary for the current core.
    if (!sdl_gl_if_create_core_buffers(&game.sgci)) return false;

    //Read shaders.
    if (!video_set_shaders(&game.sgci.video, NULL, game.menu.fragment_shader_code)) return false;
    
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 1) {
        fprintf(ERROR_FILE, "Usage: %s data/\n", argv[0]);
        return EXIT_FAILURE;
    }

    fprintf(INFO_FILE, "Welcome to Retro Gauntlet version %s.\n", RETRO_GAUNTLET_VERSION);
    
    const char *data_directory = ".";

    if (argc > 1) data_directory = argv[1];
    
    //Window opening dimensions.
    unsigned width =  1440;
    unsigned height =  900;

    //Initialize SDL.
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(ERROR_FILE, "Unable to initialize SDL: %s!\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG) < 0) {
        fprintf(ERROR_FILE, "Unable to initialize SDL_mixer: %s!\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    //Set SDL OpenGL video settings.
    SDL_GL_LoadLibrary(NULL);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    //Create window.
    SDL_Window *window = SDL_CreateWindow("Retro Gauntlet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    //Initialize GLEW
    glewExperimental = GL_TRUE;

    const GLenum glew_init_result = glewInit();
    
    if (glew_init_result != GLEW_OK) {
        fprintf(ERROR_FILE, "Unable to initialize GLEW: %s!\n", glewGetErrorString(glew_init_result));
        return EXIT_FAILURE;
    }
    
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glViewport(0, 0, width, height));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    
    //Setup game.
    if (!create_game(&game, data_directory, window)) {
        fprintf(ERROR_FILE, "Unable to initialize game!\n");
        return EXIT_FAILURE;
    }

    //Initialize networking.
    if (!net_init()) {
        return EXIT_FAILURE;
    }
    
    //Main loop.
    bool keep_running = true;
    Uint32 start_ticks = 0;
    Uint32 nr_frames = 0;
    struct gauntlet *selected_gauntlet = game.gauntlets;
    bool fullscreen = false;
    
    srand(SDL_GetTicks());

    while (keep_running) {
        SDL_Event event;
        bool event_core = true;
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    keep_running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (game.menu.state) {
                        case RETRO_GAUNTLET_STATE_MESSAGE:
                            event_core = false;

                            switch (event.key.keysym.sym) {
                                case SDLK_RETURN:
                                case SDLK_ESCAPE:
                                    game.menu.state = game.menu.last_state;
                                    break;
                            }
                            break;
                        case RETRO_GAUNTLET_STATE_QUIT_CONFIRM:
                            event_core = false;

                            switch (event.key.keysym.sym) {
                                case SDLK_RETURN:
                                case SDLK_ESCAPE:
                                case SDLK_n:
                                    game.menu.state = game.menu.last_state;
                                    break;
                                case SDLK_y:
                                    game.menu.state = game.menu.last_state;
                                    
                                    switch (game.menu.last_state) {
                                        case RETRO_GAUNTLET_STATE_SELECT_GAUNTLET:
                                            keep_running = false;
                                            break;
                                        case RETRO_GAUNTLET_STATE_LOBBY_HOST:
                                            game_stop_host(&game);
                                            break;
                                        case RETRO_GAUNTLET_STATE_LOBBY_CLIENT:
                                            game_stop_client(&game);
                                            break;
                                        case RETRO_GAUNTLET_STATE_RUN_CORE:
                                            if (selected_gauntlet) selected_gauntlet->status = RETRO_GAUNTLET_LOST;
                                            break;
                                        default:
                                            break;
                                    }
                                    break;
                            }
                            break;
                        case RETRO_GAUNTLET_STATE_SELECT_GAUNTLET:
                            switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                    //menu_escape(&game.menu, "Are you sure you want to quit (y/n)?");
                                    keep_running = false;
                                    break;
                                case SDLK_F1:
                                    game_draw_message_to_screen(&game, "Setting up host...");
                                    game_start_host(&game);
                                    break;
                                case SDLK_F2:
                                    game_draw_message_to_screen(&game, "Joining game...");
                                    game_start_client(&game, SDL_GetClipboardText());
                                    break;
                                case SDLK_UP:
                                    game_change_gauntlet_selection(&game, -1);
                                    break;
                                case SDLK_DOWN:
                                    game_change_gauntlet_selection(&game, 1);
                                    break;
                                case SDLK_PAGEUP:
                                    game_change_gauntlet_selection(&game, -6);
                                    break;
                                case SDLK_PAGEDOWN:
                                    game_change_gauntlet_selection(&game, 6);
                                    break;
                                case SDLK_END:
                                    game_change_gauntlet_selection(&game, game.nr_gauntlets);
                                    break;
                                case SDLK_HOME:
                                    game_change_gauntlet_selection(&game, -game.nr_gauntlets);
                                    break;
                                case SDLK_r:
                                    game_select_rand_gauntlet(&game);
                                    break;
                                case SDLK_f:
                                    fullscreen = !fullscreen;
                                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                                    break;
                                case SDLK_RETURN:
                                    game_start_gauntlet(&game, game.gauntlets[game.i_gauntlet].ini_file);
                                    break;
                            }
                            break;
                        case RETRO_GAUNTLET_STATE_SETUP_GAUNTLET:
                            event_core = false;

                            switch (event.key.keysym.sym) {
                                case SDLK_PAUSE:
                                    SDL_PauseAudioDevice(game.sgci.audio_device_id, 0);
                                    start_ticks = 0;
                                    nr_frames = 0;
                                    sdl_gl_if_reset_audio(&game.sgci);
                                    game.menu.state = RETRO_GAUNTLET_STATE_RUN_CORE;
                                    break;
                                case SDLK_F5:
                                    if (selected_gauntlet && selected_gauntlet->core_save_file) core_serialize_to_file(selected_gauntlet->core_save_file, &game.sgci.core);
                                    break;
                                case SDLK_F9:
                                    if (selected_gauntlet && selected_gauntlet->core_save_file) core_unserialize_from_file(&game.sgci.core, selected_gauntlet->core_save_file);
                                    break;
                                case SDLK_F1:
                                    //Reset snapshot masks to 0.
                                    core_take_and_compare_snapshots(&game.sgci.core, MASK_IF_MASK_ALWAYS, MASK_IF_DATA_ALWAYS, MASK_THEN_SET_ZERO, MEMCON_VAR_8BIT, 0);
                                    break;
                                case SDLK_F2:
                                    //Reset snapshot masks to 1.
                                    core_take_and_compare_snapshots(&game.sgci.core, MASK_IF_MASK_ALWAYS, MASK_IF_DATA_ALWAYS, MASK_THEN_SET_ONE, MEMCON_VAR_8BIT, 0);
                                    break;
                                case SDLK_F3:
                                    game.snapshot_data_condition = MASK_IF_DATA_CHANGED;
                                    break;
                                case SDLK_F4:
                                    game.snapshot_mask_action = MASK_THEN_LOG;
                                    break;
                                case SDLK_a:
                                    game.snapshot_mask_condition = MASK_IF_MASK_ALWAYS;
                                    break;
                                case SDLK_n:
                                    game.snapshot_mask_condition = MASK_IF_MASK_NEVER;
                                    break;
                                case SDLK_0:
                                    game.snapshot_mask_condition = MASK_IF_MASK_ZERO;
                                    break;
                                case SDLK_1:
                                    game.snapshot_mask_condition = MASK_IF_MASK_ONE;
                                    break;
                                case SDLK_RIGHTBRACKET:
                                    game.snapshot_mask_action = MASK_THEN_AND_ONE;
                                    break;
                                case SDLK_LEFTBRACKET:
                                    game.snapshot_mask_action = MASK_THEN_XOR_ONE;
                                    break;
                                case SDLK_BACKSLASH:
                                    game.snapshot_mask_action = MASK_THEN_OR_ONE;
                                    break;
                                case SDLK_MINUS:
                                    game.snapshot_mask_action = MASK_THEN_SET_ZERO;
                                    break;
                                case SDLK_EQUALS:
                                    game.snapshot_mask_action = MASK_THEN_SET_ONE;
                                    break;
                                case SDLK_o:
                                    game.snapshot_mask_action = MASK_THEN_ADD_ONE;
                                    break;
                                case SDLK_p:
                                    game.snapshot_mask_action = MASK_THEN_SUB_ONE;
                                    break;
                                case SDLK_z:
                                    game.snapshot_data_condition = MASK_IF_DATA_ALWAYS;
                                    break;
                                case SDLK_x:
                                    game.snapshot_data_condition = MASK_IF_DATA_LESS_PREV;
                                    break;
                                case SDLK_c:
                                    game.snapshot_data_condition = MASK_IF_DATA_EQUAL_PREV;
                                    break;
                                case SDLK_v:
                                    game.snapshot_data_condition = MASK_IF_DATA_GREATER_PREV;
                                    break;
                                case SDLK_g:
                                    game.snapshot_data_condition = MASK_IF_DATA_LESS_CONST;
                                    break;
                                case SDLK_h:
                                    game.snapshot_data_condition = MASK_IF_DATA_EQUAL_CONST;
                                    break;
                                case SDLK_j:
                                    game.snapshot_data_condition = MASK_IF_DATA_GREATER_CONST;
                                    break;
                                case SDLK_4:
                                    game.snapshot_mask_size = MEMCON_VAR_8BIT;
                                    break;
                                case SDLK_5:
                                    game.snapshot_mask_size = MEMCON_VAR_16BIT;
                                    break;
                                case SDLK_6:
                                    game.snapshot_mask_size = MEMCON_VAR_32BIT;
                                    break;
                                case SDLK_7:
                                    game.snapshot_mask_size = MEMCON_VAR_64BIT;
                                    break;
                                case SDLK_RETURN:
                                    game.snapshot_const_value = atoi(SDL_GetClipboardText());
                                    break;
                                case SDLK_SPACE:
                                    core_take_and_compare_snapshots(&game.sgci.core, game.snapshot_mask_condition, game.snapshot_data_condition, game.snapshot_mask_action, game.snapshot_mask_size, game.snapshot_const_value);
                                    break;
                            }

                            break;
                        case RETRO_GAUNTLET_STATE_LOBBY_HOST:
                            switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                    menu_escape(&game.menu, "Are you sure you want to stop hosting (y/n)?");
                                    break;
                                case SDLK_UP:
                                    game_change_gauntlet_selection(&game, -1);
                                    break;
                                case SDLK_DOWN:
                                    game_change_gauntlet_selection(&game, 1);
                                    break;
                                case SDLK_r:
                                    game_select_rand_gauntlet(&game);
                                    break;
                                case SDLK_g:
                                    if (!game_host_start_gauntlet(&game)) menu_draw_message(&game.menu, "Unable to host gauntlet!");
                                    break;
                                case SDLK_f:
                                    fullscreen = !fullscreen;
                                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                                    break;
                            }
                            break;
                        case RETRO_GAUNTLET_STATE_LOBBY_CLIENT:
                            switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                    menu_escape(&game.menu, "Are you sure you want to leave this lobby (y/n)?");
                                    break;
                                case SDLK_f:
                                    fullscreen = !fullscreen;
                                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                                    break;
                            }
                            break;
                        case RETRO_GAUNTLET_STATE_RUN_CORE:
                            switch (event.key.keysym.sym) {
                                case SDLK_PAUSE:
                                    //Go to memory monitoring mode.
                                    if (game.host.sock < 0 && game.client.sock < 0) {
                                        SDL_PauseAudioDevice(game.sgci.audio_device_id, 1);
                                        game.menu.state = RETRO_GAUNTLET_STATE_SETUP_GAUNTLET;
                                    }
                                    event_core = false;
                                    break;
                                case SDLK_ESCAPE:
                                    menu_escape(&game.menu, "Are you sure you want to give up this gauntlet run (y/n)?");
                                    event_core = false;
                                    break;
                                //Restricted keys when a core is running.
                                case SDLK_F1:
                                case SDLK_F2:
                                case SDLK_F3:
                                case SDLK_F4:
                                case SDLK_F5:
                                case SDLK_F6:
                                case SDLK_F7:
                                case SDLK_F8:
                                case SDLK_F9:
                                case SDLK_F10:
                                case SDLK_F11:
                                case SDLK_F12:
                                    event_core = false;
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            keep_running = false;
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            width = event.window.data1;
                            height = event.window.data2;
                            video_set_window(&game.menu.video, width, height);
                            break;
                    }
                    break;
            }
            
            //Pass events to libretro core if core is running.
            if (event_core && game.menu.state == RETRO_GAUNTLET_STATE_RUN_CORE) sdl_gl_if_handle_event(&game.sgci, event);
        }

        //Are we going to start a gauntlet?
        if ((game.menu.state == RETRO_GAUNTLET_STATE_SELECT_GAUNTLET ||
             game.menu.state == RETRO_GAUNTLET_STATE_LOBBY_HOST ||
             game.menu.state == RETRO_GAUNTLET_STATE_LOBBY_CLIENT) && game.gauntlet.ini_file) {
            start_ticks = 0;
            nr_frames = 0;
            selected_gauntlet = &game.gauntlet;
            menu_stop_mixer(&game.menu);

            if (setup_sdl_app_for_gauntlet(selected_gauntlet) &&
                gauntlet_start(selected_gauntlet, &game.sgci)) {
                fprintf(INFO_FILE, "Set up '%s'.\n", game.gauntlet.title);

                //Gauntlet was started successfully.
                //core_take_and_compare_snapshots(&game.sgci.core, MASK_IF_MASK_ALWAYS, MASK_IF_DATA_ALWAYS, MASK_THEN_SET_ZERO, MEMCON_VAR_8BIT, 0);
                game.menu.state = RETRO_GAUNTLET_STATE_RUN_CORE;
            }
            else {
                //Failed to start gauntlet.
                selected_gauntlet = NULL;
                game_stop_gauntlet(&game);
                menu_start_mixer(&game.menu);
                menu_draw_message(&game.menu, "Unable to start gauntlet!");
            }
        }

        
        //Update and draw core or menu.
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        if (start_ticks == 0) start_ticks = SDL_GetTicks();
        game_update(&game);
        nr_frames++;
        SDL_GL_SwapWindow(window);

        switch (game.menu.state) {
            case RETRO_GAUNTLET_STATE_RUN_CORE:
                //Try to match the desired frames per second to avoid audio stuttering.
                if (true) {
                    Uint32 desired_ticks = (Uint32)(1000.0*(double)nr_frames/game.sgci.core.frames_per_second);
                    Uint32 ticks = SDL_GetTicks() - start_ticks;
            
                    if (desired_ticks > ticks) {
                        SDL_Delay(desired_ticks - ticks);
                    }
                    else {
                        //We are behind --> ask for another frame.
                        game.sgci.core.retro_run();
                        nr_frames++;
                    }
                }
                break;
            default:
                //100fps should be more than enough for the menu.
                SDL_Delay(10);
                break;
        }
    }

    net_quit();
    free_game(&game);

    //Free SDL data.
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_GL_UnloadLibrary();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    fprintf(INFO_FILE, "Shut down Retro Gauntlet version %s.\n", RETRO_GAUNTLET_VERSION);

    return EXIT_SUCCESS;
}

