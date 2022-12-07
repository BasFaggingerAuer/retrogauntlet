/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//libretro core wrapper with memory inspection routines.
#ifndef CORE_H__
#define CORE_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "retrogauntlet.h"
#include "libretro.h"

#ifdef _WIN32
typedef HMODULE dl_t;
#else
typedef void * dl_t;
#endif

/** Struct describing a libretro core variable. */
struct retro_core_var {
    char *key;
    char *description;
    char *value;
    bool updated;
};

/** Enum describing the size of a memory condition to be checked for a libretro core. */
enum retro_core_memory_var_type {
    MEMCON_VAR_8BIT = 0,
    MEMCON_VAR_16BIT = 1,
    MEMCON_VAR_32BIT = 2,
    MEMCON_VAR_64BIT = 3
};

/** Enum describing the type of memory condition to be performed for a libretro core. */
enum retro_core_memory_var_compare {
    MEMCON_CMP_CHANGED = 0,
    MEMCON_CMP_EQUAL = 1,
    MEMCON_CMP_NOT_EQUAL = 2,
    MEMCON_CMP_GREATER = 3,
    MEMCON_CMP_LESS = 4
};

/** Enum describing the mask action to perform for libretro core memory inspection when @see retro_core_memory_mask_condition and @see retro_core_memory_data_condition are satisfied. */
enum retro_core_memory_mask_action {
    MASK_THEN_NOP = 0,
    MASK_THEN_SET_ZERO = 1,
    MASK_THEN_SET_ONE = 2,
    MASK_THEN_OR_ONE = 3,
    MASK_THEN_AND_ONE = 4,
    MASK_THEN_XOR_ONE = 5,
    MASK_THEN_ADD_ONE = 6,
    MASK_THEN_SUB_ONE = 7,
    MASK_THEN_LOG = 8
};

#define MEMCON_VALUE_UNSET 0xffffffffffffffffLL

/** Struct describing a memory condition to check for a libretro core. The condition is satisfied if the value of size @see type at offset @see offset in memory bank @see snapshot satisfies condition @see compare when compared to @see value. */
struct retro_core_memory_condition {
    size_t snapshot;
    size_t offset;
    unsigned type; /**< @see retro_core_memory_var_type */
    unsigned compare; /**< @see retro_core_memory_var_compare */
    uint64_t value;
    uint64_t last_value;
};

/** Enum describing the mask condition for libretro core memory inspection. */
enum retro_core_memory_mask_condition {
    MASK_IF_MASK_ALWAYS = 0,
    MASK_IF_MASK_NEVER = 1,
    MASK_IF_MASK_ZERO = 2,
    MASK_IF_MASK_ONE = 3
};

/** Enum describing the memory value condition for libretro core memory inspection. */
enum retro_core_memory_data_condition {
    MASK_IF_DATA_ALWAYS = 0,
    MASK_IF_DATA_NEVER = 1,
    MASK_IF_DATA_CHANGED = 2,
    MASK_IF_DATA_EQUAL_PREV = 3,
    MASK_IF_DATA_GREATER_PREV = 4,
    MASK_IF_DATA_LESS_PREV = 5,
    MASK_IF_DATA_EQUAL_CONST = 6,
    MASK_IF_DATA_GREATER_CONST = 7,
    MASK_IF_DATA_LESS_CONST = 8
};

/** Struct wrapping a libretro core, based on RetroArch's dynamic.h. */
struct retro_core {
    void (*retro_init)(void);
    void (*retro_deinit)(void);
    unsigned (*retro_api_version)(void);
    void (*retro_get_system_info)(struct retro_system_info*);
    void (*retro_get_system_av_info)(struct retro_system_av_info*);
    void (*retro_set_environment)(retro_environment_t);
    void (*retro_set_video_refresh)(retro_video_refresh_t);
    void (*retro_set_audio_sample)(retro_audio_sample_t);
    void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*retro_set_input_poll)(retro_input_poll_t);
    void (*retro_set_input_state)(retro_input_state_t);
    void (*retro_set_controller_port_device)(unsigned, unsigned);
    void (*retro_reset)(void);
    void (*retro_run)(void);
    size_t (*retro_serialize_size)(void);
    bool (*retro_serialize)(void*, size_t);
    bool (*retro_unserialize)(const void*, size_t);
    void (*retro_cheat_reset)(void);
    void (*retro_cheat_set)(unsigned, bool, const char*);
    bool (*retro_load_game)(const struct retro_game_info*);
    bool (*retro_load_game_special)(unsigned, const struct retro_game_info*, size_t);
    void (*retro_unload_game)(void);
    unsigned (*retro_get_region)(void);
    void *(*retro_get_memory_data)(unsigned);
    size_t (*retro_get_memory_size)(unsigned);

    struct retro_memory_map mmap;

    uint8_t **snapshot_data;
    uint8_t **snapshot_mask;
    size_t *nr_snapshot_data;
    size_t nr_snapshots;

    char *full_path;
    void *rom_data;
    struct retro_core_var *variables;
    size_t nr_variables;
    bool can_load_null_game;
    dl_t dynamic_library;
    
    retro_usec_t reference_frame_time;
    double frames_per_second;
    double sample_rate;
};

bool load_core_from_file(struct retro_core *core,
                         const char *core_file, const char *rom_file, const char *options_file,
                         retro_environment_t setup_function,
                         retro_video_refresh_t video_refresh_function,
                         retro_audio_sample_t audio_sample_function,
                         retro_audio_sample_batch_t audio_sample_batch_function,
                         retro_input_poll_t input_poll_function,
                         retro_input_state_t input_state_function);

bool core_serialize_to_file(const char *, struct retro_core *);
bool core_unserialize_from_file(struct retro_core *, const char *);
bool core_load_conditions_from_file(struct retro_core_memory_condition **, size_t *, const char *);
bool core_check_conditions(const struct retro_core *, struct retro_core_memory_condition *, const size_t, const bool);
bool set_core_variables(struct retro_core *, const struct retro_variable *);
bool set_core_variable(struct retro_core *, const char *, const char *);
const char *get_core_variable(struct retro_core *, const struct retro_variable *);
bool was_any_core_variable_updated(const struct retro_core *);
bool free_core_variables(struct retro_core *);
bool set_core_controller_infos(struct retro_core *, const struct retro_controller_info *);
bool set_core_memory_maps(struct retro_core *, const struct retro_memory_map *);
bool free_core_memory_maps(struct retro_core *);
bool free_core(struct retro_core *);

bool free_core_snapshots(struct retro_core *);
bool core_take_and_compare_snapshots(struct retro_core *, const unsigned, const unsigned, const unsigned, const unsigned, const uint64_t);

#endif

