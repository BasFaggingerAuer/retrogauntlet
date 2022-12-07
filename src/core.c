/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <string.h>

#include "stringextra.h"
#include "files.h"
#include "core.h"

//For load_core_from_file.
#ifdef _WIN32
#define SET_CORE_FUNCTION(core, func) do { \
    FARPROC f = GetProcAddress(core->dynamic_library, #func); \
    memcpy(&core->func, &f, sizeof(f)); \
    if (!(core->func)) { \
        fprintf(ERROR_FILE, "load_core_from_file: Function '%s' cannot be found!\n", #func); \
        return false; \
    } \
} while (false);
#else
#define SET_CORE_FUNCTION(core, func) do { \
    void *f = dlsym(core->dynamic_library, #func); \
    memcpy(&core->func, &f, sizeof(f)); \
    if (!(core->func)) { \
        fprintf(ERROR_FILE, "load_core_from_file: Function '%s' cannot be found!\n", #func); \
        return false; \
    } \
} while (false);
#endif

#define NR_CORE_OPTION_LINE 4096

bool load_core_from_file(struct retro_core *core,
                         const char *core_file, const char *rom_file, const char *options_file,
                         retro_environment_t setup_function,
                         retro_video_refresh_t video_refresh_function,
                         retro_audio_sample_t audio_sample_function,
                         retro_audio_sample_batch_t audio_sample_batch_function,
                         retro_input_poll_t input_poll_function,
                         retro_input_state_t input_state_function) {
    if (!core || !core_file) {
        fprintf(ERROR_FILE, "load_core_from_file: Invalid core or file!\n");
        return false;
    }

    memset(core, 0, sizeof(struct retro_core));
    
    core->frames_per_second = 30.0;
    core->sample_rate = 44100.0;
    
    //Get full path to core.
    core->full_path = expand_to_full_path(core_file);

    //Read dynamic library.
#ifdef _WIN32
    core->dynamic_library = LoadLibrary(core_file);
    
    if (!core->dynamic_library) {
        fprintf(ERROR_FILE, "load_core_from_file: Unable to open '%s'!\n", core_file);
        return false;
    }
#else
    core->dynamic_library = dlopen(core_file, RTLD_LAZY | RTLD_LOCAL);

    if (!core->dynamic_library) {
        fprintf(ERROR_FILE, "load_core_from_file: Unable to open '%s': %s!\n", core_file, dlerror());
        return false;
    }
#endif
    
    //Get library symbols.
    SET_CORE_FUNCTION(core, retro_init);
    SET_CORE_FUNCTION(core, retro_deinit);
    SET_CORE_FUNCTION(core, retro_api_version);
    SET_CORE_FUNCTION(core, retro_get_system_info);
    SET_CORE_FUNCTION(core, retro_get_system_av_info);
    SET_CORE_FUNCTION(core, retro_set_environment);
    SET_CORE_FUNCTION(core, retro_set_video_refresh);
    SET_CORE_FUNCTION(core, retro_set_audio_sample);
    SET_CORE_FUNCTION(core, retro_set_audio_sample_batch);
    SET_CORE_FUNCTION(core, retro_set_input_poll);
    SET_CORE_FUNCTION(core, retro_set_input_state);
    SET_CORE_FUNCTION(core, retro_set_controller_port_device);
    SET_CORE_FUNCTION(core, retro_reset);
    SET_CORE_FUNCTION(core, retro_run);
    SET_CORE_FUNCTION(core, retro_serialize_size);
    SET_CORE_FUNCTION(core, retro_serialize);
    SET_CORE_FUNCTION(core, retro_unserialize);
    SET_CORE_FUNCTION(core, retro_load_game);
    SET_CORE_FUNCTION(core, retro_load_game_special);
    SET_CORE_FUNCTION(core, retro_unload_game);
    SET_CORE_FUNCTION(core, retro_get_region);
    SET_CORE_FUNCTION(core, retro_get_memory_data);
    SET_CORE_FUNCTION(core, retro_get_memory_size);

    //Get core information.
    struct retro_system_info core_info;

    core->retro_get_system_info(&core_info);

    fprintf(CORE_FILE, "Setting up core '%s %s' from '%s', valid extensions '%s'...\n", core_info.library_name, core_info.library_version, core_file, core_info.valid_extensions);

    //Set up callbacks.
    fprintf(CORE_FILE, "    retro_set_environment()...\n");
    core->retro_set_environment(setup_function);
    
    fprintf(CORE_FILE, "    retro_init()...\n");
    core->retro_init();

    core->retro_set_video_refresh(video_refresh_function);
    core->retro_set_input_poll(input_poll_function);
    core->retro_set_input_state(input_state_function);
    core->retro_set_audio_sample(audio_sample_function);
    core->retro_set_audio_sample_batch(audio_sample_batch_function);

    if (options_file) {
        fprintf(CORE_FILE, "Loading options from '%s'...\n", options_file);

        FILE *f = fopen(options_file, "r");
        char line[NR_CORE_OPTION_LINE];
        char var_key[NR_CORE_OPTION_LINE];
        char var_value[NR_CORE_OPTION_LINE];

        if (!f) {
            fprintf(ERROR_FILE, "load_core_from_file: Unable to read options file '%s'!\n", options_file);
            free_core(core);
            return false;
        }

        while (fgets(line, NR_CORE_OPTION_LINE, f)) {
            char *tok, *saveptr = NULL;

            if (IS_COMMENT_LINE(line)) continue;
            
            if ((tok = strtok_r(line, " ", &saveptr))) {
                strncpy_trim(var_key, tok, NR_CORE_OPTION_LINE);

                if ((tok = strtok_r(NULL, " ", &saveptr))) {
                    strncpy_trim(var_value, tok, NR_CORE_OPTION_LINE);
                    
                    if (strlen(var_value) > 0) {
                        if (!set_core_variable(core, var_key, var_value)) {
                            fprintf(ERROR_FILE, "load_core_from_file: Unable to set variable '%s' to '%s'!\n", var_key, var_value);
                        }
                    }
                }
            }
        }

        fclose(f);
    }

    if (rom_file) {
        fprintf(CORE_FILE, "Loading ROM from '%s'...\n", rom_file);

        struct retro_game_info rom_info;

        memset(&rom_info, 0, sizeof(rom_info));
        rom_info.path = rom_file;

        if (!core_info.need_fullpath) {
            FILE *f = fopen(rom_file, "rb");

            if (!f) {
                fprintf(ERROR_FILE, "load_core_from_file: Unable to read ROM file '%s'!\n", rom_file);
                free_core(core);
                return false;
            }
            
            fseek(f, 0L, SEEK_END);
            rom_info.size = ftell(f);
            rewind(f);
            core->rom_data = calloc(rom_info.size, 1);
            rom_info.data = core->rom_data;

            if (!rom_info.data) {
                fprintf(ERROR_FILE, "load_core_from_file: Unable to allocate memory for ROM file '%s'!\n", rom_file);
                fclose(f);
                free_core(core);
                return false;
            }

            if (fread((void *)rom_info.data, 1, rom_info.size, f) != rom_info.size) {
                fprintf(ERROR_FILE, "load_core_from_file: Unable to read all data from ROM file '%s'!\n", rom_file);
            }

            fclose(f);
        }

        if (!core->retro_load_game(&rom_info)) {
            fprintf(ERROR_FILE, "load_core_from_file: retro_load_game() failed for '%s'!\n", rom_file);
            free_core(core);
            return false;
        }
    }
    else if (core->can_load_null_game) {
        fprintf(CORE_FILE, "Running core without ROM.\n");
        if (!core->retro_load_game(NULL)) {
            fprintf(ERROR_FILE, "load_core_from_file: retro_load_game() failed without ROM!\n");
            free_core(core);
            return false;
        }
    }
    else {
        fprintf(ERROR_FILE, "load_core_from_file: No ROM provided while core cannot run without!\n");
        free_core(core);
        return false;
    }

    return true;
}

bool free_core_variables(struct retro_core *core) {
    //Free only the core's variables.
    if (!core) {
        fprintf(ERROR_FILE, "free_core_variables: Invalid core!\n");
        return false;
    }

    if (core->variables) {
        for (struct retro_core_var *var = core->variables; var->key; var++) {
            free(var->key);
            if (var->description) free(var->description);
            if (var->value) free(var->value);
        }

        free(core->variables);
    }

    core->variables = NULL;
    core->nr_variables = 0;

    return true;
}

bool set_core_variables(struct retro_core *core, const struct retro_variable *vars) {
    if (!core || !vars) {
        fprintf(ERROR_FILE, "set_core_variables: Invalid core or variables!\n");
        return false;
    }

    free_core_variables(core);
    
    for (const struct retro_variable *v = vars; v->key; v++) {
        core->nr_variables++;
    }
    
    if (core->nr_variables == 0) {
        return false;
    }

    core->variables = (struct retro_core_var *)calloc(core->nr_variables + 1, sizeof(struct retro_core_var));
    struct retro_core_var *out_vars = core->variables;

    for (size_t i = 0; i < core->nr_variables; i++, vars++, out_vars++) {
        out_vars->key = strdup(vars->key);
        out_vars->description = strdup(vars->value);
        out_vars->value = NULL;
        out_vars->updated = false;
        fprintf(CORE_FILE, "Added core variable '%s': %s\n", vars->key, vars->value);
    }
    
    return true;
}

bool set_core_variable(struct retro_core *core, const char *key, const char *value) {
    if (!core || !key || !value || !core->variables) {
        fprintf(ERROR_FILE, "set_core_variable: Invalid core or no variables!\n");
        return false;
    }

    for (struct retro_core_var *vars = core->variables; vars->key; vars++) {
        if (strcmp(vars->key, key) == 0) {
            if (vars->value) free(vars->value);
            vars->value = strdup(value);
            vars->updated = true;
            fprintf(CORE_FILE, "Set core variable '%s' to '%s'.\n", vars->key, vars->value);
            return true;
        }
    }

    fprintf(ERROR_FILE, "set_core_variable: Variable '%s' could not be found!\n", key);

    return false;
}

const char *get_core_variable(struct retro_core *core, const struct retro_variable *var) {
    if (!core || !var || !core->variables) {
        fprintf(ERROR_FILE, "get_core_variable: Invalid core or no variables!\n");
        return NULL;
    }

    for (struct retro_core_var *vars = core->variables; vars->key; vars++) {
        if (strcmp(vars->key, var->key) == 0) {
            vars->updated = false;
            return vars->value;
        }
    }

    fprintf(ERROR_FILE, "get_core_variable: Variable '%s' could not be found!\n", var->key);

    return NULL;
}

bool was_any_core_variable_updated(const struct retro_core *core) {
    if (!core || !core->variables) {
        fprintf(ERROR_FILE, "was_any_core_variable_updated: Invalid core or no variables!\n");
        return false;
    }

    for (const struct retro_core_var *vars = core->variables; vars->key; vars++) {
        if (vars->updated) {
            return true;
        }
    }

    return false;
}

bool set_core_controller_infos(struct retro_core *core, const struct retro_controller_info *info) {
    if (!core || !info) {
        fprintf(ERROR_FILE, "set_core_controller_infos: Invalid core or info!\n");
        return false;
    }
    
    for (unsigned i = 0; info[i].types; i++) {
        fprintf(CORE_FILE, "Controller port %u:\n", i);

        for (unsigned j = 0; j < info[i].num_types; j++) {
            fprintf(CORE_FILE, "    %04u: %s\n", info[i].types[j].id, info[i].types[j].desc);
        }
    }

    return true;
}

bool free_core(struct retro_core *core) {
    if (!core) {
        fprintf(ERROR_FILE, "free_core: Invalid core!\n");
        return false;
    }
    
    //Core was already freed.
    if (!core->dynamic_library) return false;
    
    core->retro_unload_game();
    core->retro_deinit();
#ifdef _WIN32
    FreeLibrary(core->dynamic_library);
#else
    dlclose(core->dynamic_library);
#endif
    free_core_variables(core);
    free_core_memory_maps(core);
    free_core_snapshots(core);
    if (core->full_path) free(core->full_path);
    if (core->rom_data) free(core->rom_data);

    memset(core, 0, sizeof(struct retro_core));

    fprintf(CORE_FILE, "Freed core.\n");

    return false;
}

bool core_serialize_to_file(const char *file, struct retro_core *core) {
    if (!core || !file) {
        fprintf(ERROR_FILE, "core_serialize_to_file: Invalid core or file!\n");
        return false;
    }

    size_t nr_bytes = core->retro_serialize_size();

    if (nr_bytes == 0) {
        fprintf(ERROR_FILE, "core_serialize_to_file: No data to serialize!\n");
        return false;
    }

    void *data = calloc(nr_bytes, 1);

    if (!data) {
        fprintf(ERROR_FILE, "core_serialize_to_file: Unable to allocate data array!\n");
        return false;
    }

    if (!core->retro_serialize(data, nr_bytes)) {
        fprintf(ERROR_FILE, "core_serialize_to_file: Unable to serialize state!\n");
        free(data);
        return false;
    }

    FILE *f = fopen(file, "wb");

    if (!f) {
        fprintf(ERROR_FILE, "core_serialize_to_file: Unable to open '%s' for writing!\n", file);
        free(data);
        return false;
    }
    
    if (fwrite(data, 1, nr_bytes, f) != nr_bytes) {
        fprintf(ERROR_FILE, "core_serialize_to_file: Unable to write all data to '%s'!\n", file);
        fclose(f);
        free(data);
        return false;
    }

    fclose(f);
    free(data);
    
    fprintf(CORE_FILE, "Serialized core as %zu bytes to '%s'.\n", nr_bytes, file);

    return true;
}

bool free_core_snapshots(struct retro_core *core) {
    if (!core) {
        fprintf(ERROR_FILE, "free_core_snapshots: Invalid core!\n");
        return false;
    }
    
    if (core->snapshot_data) {
        for (size_t i = 0; i < core->nr_snapshots; ++i) {
            if (core->snapshot_data[i]) free(core->snapshot_data[i]);
        }

        free(core->snapshot_data);
    }

    if (core->snapshot_mask) {
        for (size_t i = 0; i < core->nr_snapshots; ++i) {
            if (core->snapshot_mask[i]) free(core->snapshot_mask[i]);
        }

        free(core->snapshot_mask);
    }

    if (core->nr_snapshot_data) free(core->nr_snapshot_data);

    core->snapshot_data = NULL;
    core->snapshot_mask = NULL;
    core->nr_snapshot_data = NULL;
    core->nr_snapshots = 0;
    
    return true;
}

//Make memory masking more efficient. Templates would have made this slightly neater.
#define UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(action) do { \
    for (size_t i = 0; i < nr_data; i++) { \
        action; \
    } \
    done = true; \
} while (false);

#define UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, action) do { \
    for (size_t i = 0; i <= nr_data - sizeof(type); i++) { \
        const type o = *(const type *)(o_p + i); \
        const type n = *(const type *)(n_p + i); \
        \
        if (condition_mem) { \
            action; \
        } \
    } \
    done = true; \
} while (false);

#define UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, action) do { \
    for (size_t i = 0; i <= nr_data - sizeof(type); i++) { \
        const type o = *(const type *)(o_p + i); \
        const type n = *(const type *)(n_p + i); \
        const uint8_t m = m_p[i]; \
        \
        if ((condition_mask) && (condition_mem)) { \
            action; \
        } \
    } \
    done = true; \
} while (false);

#define UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, action) do { \
    for (size_t i = 0; i <= nr_data - sizeof(type); i++) { \
        const type n = *(const type *)(n_p + i); \
        \
        if (condition_mem) { \
            action; \
        } \
    } \
    done = true; \
} while (false);

#define UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, action) do { \
    for (size_t i = 0; i <= nr_data - sizeof(type); i++) { \
        const type n = *(const type *)(n_p + i); \
        const uint8_t m = m_p[i]; \
        \
        if ((condition_mask) && (condition_mem)) { \
            action; \
        } \
    } \
    done = true; \
} while (false);

#define UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_NO_MEM(type) do { \
    switch (mask_action) { \
        case MASK_THEN_SET_ZERO: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] = 0); \
            break; \
        case MASK_THEN_SET_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] = 1); \
            break; \
        case MASK_THEN_OR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] |= 1); \
            break; \
        case MASK_THEN_AND_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] &= 1); \
            break; \
        case MASK_THEN_XOR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] ^= 1); \
            break; \
        case MASK_THEN_ADD_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] += (m_p[i] < 0xff ? 1 : 0)); \
            break; \
        case MASK_THEN_SUB_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_NO_MEM(m_p[i] -= (m_p[i] > 0x00 ? 1 : 0)); \
            break; \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_CONST(type, condition_mem) do { \
    switch (mask_action) { \
        case MASK_THEN_SET_ZERO: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] = 0); \
            break; \
        case MASK_THEN_SET_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] = 1); \
            break; \
        case MASK_THEN_OR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] |= 1); \
            break; \
        case MASK_THEN_AND_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] &= 1); \
            break; \
        case MASK_THEN_XOR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] ^= 1); \
            break; \
        case MASK_THEN_ADD_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] += (m_p[i] < 0xff ? 1 : 0)); \
            break; \
        case MASK_THEN_SUB_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK_CONST(type, condition_mem, m_p[i] -= (m_p[i] > 0x00 ? 1 : 0)); \
            break; \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, condition_mask, condition_mem) do { \
    switch (mask_action) { \
        case MASK_THEN_SET_ZERO: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] = 0); \
            break; \
        case MASK_THEN_SET_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] = 1); \
            break; \
        case MASK_THEN_OR_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] |= 1); \
            break; \
        case MASK_THEN_AND_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] &= 1); \
            break; \
        case MASK_THEN_XOR_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] ^= 1); \
            break; \
        case MASK_THEN_ADD_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] += (m_p[i] < 0xff ? 1 : 0)); \
            break; \
        case MASK_THEN_SUB_ONE: \
            UPDATE_SNAPSHOT_LOOP_CONST(type, condition_mask, condition_mem, m_p[i] -= (m_p[i] > 0x00 ? 1 : 0)); \
            break; \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK(type, condition_mem) do { \
    switch (mask_action) { \
        case MASK_THEN_SET_ZERO: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] = 0); \
            break; \
        case MASK_THEN_SET_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] = 1); \
            break; \
        case MASK_THEN_OR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] |= 1); \
            break; \
        case MASK_THEN_AND_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] &= 1); \
            break; \
        case MASK_THEN_XOR_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] ^= 1); \
            break; \
        case MASK_THEN_ADD_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] += (m_p[i] < 0xff ? 1 : 0)); \
            break; \
        case MASK_THEN_SUB_ONE: \
            UPDATE_SNAPSHOT_LOOP_NO_MASK(type, condition_mem, m_p[i] -= (m_p[i] > 0x00 ? 1 : 0)); \
            break; \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_ACTION(type, condition_mask, condition_mem) do { \
    switch (mask_action) { \
        case MASK_THEN_SET_ZERO: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] = 0); \
            break; \
        case MASK_THEN_SET_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] = 1); \
            break; \
        case MASK_THEN_OR_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] |= 1); \
            break; \
        case MASK_THEN_AND_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] &= 1); \
            break; \
        case MASK_THEN_XOR_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] ^= 1); \
            break; \
        case MASK_THEN_ADD_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] += (m_p[i] < 0xff ? 1 : 0)); \
            break; \
        case MASK_THEN_SUB_ONE: \
            UPDATE_SNAPSHOT_LOOP(type, condition_mask, condition_mem, m_p[i] -= (m_p[i] > 0x00 ? 1 : 0)); \
            break; \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_CONDITION(type) do { \
    if (mask_condition == MASK_IF_MASK_ALWAYS && data_condition == MASK_IF_DATA_ALWAYS) { \
        UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_NO_MEM(type); \
    } \
    else if (mask_condition == MASK_IF_MASK_ALWAYS) { \
        switch (data_condition) { \
            case MASK_IF_DATA_CHANGED: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK(type, n != o); \
                break; \
            case MASK_IF_DATA_EQUAL_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK(type, n == o); \
                break; \
            case MASK_IF_DATA_GREATER_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK(type, n > o); \
                break; \
            case MASK_IF_DATA_LESS_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK(type, n < o); \
                break; \
            case MASK_IF_DATA_EQUAL_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_CONST(type, n == const_value); \
                break; \
            case MASK_IF_DATA_GREATER_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_CONST(type, n > const_value); \
                break; \
            case MASK_IF_DATA_LESS_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_NO_MASK_CONST(type, n < const_value); \
                break; \
        } \
    } \
    else if (mask_condition == MASK_IF_MASK_ONE) { \
        switch (data_condition) { \
            case MASK_IF_DATA_CHANGED: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 1, n != o); \
                break; \
            case MASK_IF_DATA_EQUAL_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 1, n == o); \
                break; \
            case MASK_IF_DATA_GREATER_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 1, n > o); \
                break; \
            case MASK_IF_DATA_LESS_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 1, n < o); \
                break; \
            case MASK_IF_DATA_EQUAL_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 1, n == const_value); \
                break; \
            case MASK_IF_DATA_GREATER_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 1, n > const_value); \
                break; \
            case MASK_IF_DATA_LESS_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 1, n < const_value); \
                break; \
        } \
    } \
    else if (mask_condition == MASK_IF_MASK_ZERO) { \
        switch (data_condition) { \
            case MASK_IF_DATA_CHANGED: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 0, n != o); \
                break; \
            case MASK_IF_DATA_EQUAL_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 0, n == o); \
                break; \
            case MASK_IF_DATA_GREATER_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 0, n > o); \
                break; \
            case MASK_IF_DATA_LESS_PREV: \
                UPDATE_SNAPSHOT_MASK_ACTION(type, m == 0, n < o); \
                break; \
            case MASK_IF_DATA_EQUAL_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 0, n == const_value); \
                break; \
            case MASK_IF_DATA_GREATER_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 0, n > const_value); \
                break; \
            case MASK_IF_DATA_LESS_CONST: \
                UPDATE_SNAPSHOT_MASK_ACTION_CONST(type, m == 0, n < const_value); \
                break; \
        } \
    } \
} while (false);

#define UPDATE_SNAPSHOT_MASK_CONDITION_GENERIC(type) do { \
    for (size_t i = 0; i <= nr_data - sizeof(type); i++) { \
        const type o = *(const type *)(o_p + i); \
        const type n = *(const type *)(n_p + i); \
        const uint8_t m = m_p[i]; \
        \
        if ((mask_condition == MASK_IF_MASK_ALWAYS || \
             (mask_condition == MASK_IF_MASK_ZERO && m == 0) || \
             (mask_condition == MASK_IF_MASK_ONE && m == 1)) \
            && \
            (data_condition == MASK_IF_DATA_ALWAYS || \
             (data_condition == MASK_IF_DATA_CHANGED && n != o) || \
             (data_condition == MASK_IF_DATA_EQUAL_PREV && n == o) || \
             (data_condition == MASK_IF_DATA_GREATER_PREV && n > o) || \
             (data_condition == MASK_IF_DATA_LESS_PREV && n < o) || \
             (data_condition == MASK_IF_DATA_EQUAL_CONST && n == const_value) || \
             (data_condition == MASK_IF_DATA_GREATER_CONST && n > const_value) || \
             (data_condition == MASK_IF_DATA_LESS_CONST && n < const_value))) { \
            switch (mask_action) { \
                case MASK_THEN_SET_ZERO: \
                    m_p[i] = 0; \
                    break; \
                case MASK_THEN_SET_ONE: \
                    m_p[i] = 1; \
                    break; \
                case MASK_THEN_OR_ONE: \
                    m_p[i] |= 1; \
                    break; \
                case MASK_THEN_AND_ONE: \
                    m_p[i] &= 1; \
                    break; \
                case MASK_THEN_XOR_ONE: \
                    m_p[i] ^= 1; \
                    break; \
                case MASK_THEN_ADD_ONE: \
                    m_p[i] += (m_p[i] < 0xff ? 1 : 0); \
                    break; \
                case MASK_THEN_SUB_ONE: \
                    m_p[i] -= (m_p[i] > 0x00 ? 1 : 0); \
                    break; \
                case MASK_THEN_LOG: \
                    fprintf(MEM_FILE, "%08zx %08zx %02x %08zx %08zx\n", i_snapshot, i, m, (uint64_t)o, (uint64_t)n); \
                    break; \
            } \
        } \
    } \
    done = true; \
} while (false);

//Helper function for core_task_and_compare_snapshots().
void update_snapshot(struct retro_core *core, const unsigned mask_condition, const unsigned data_condition, const unsigned mask_action, const unsigned size_value, const uint64_t const_value, const size_t i_snapshot, const size_t nr_data, const void *data) {
    if (!core) return;
    if (i_snapshot >= core->nr_snapshots) return;
    if (nr_data == 0 || !data) return;
    
    if (nr_data != core->nr_snapshot_data[i_snapshot] ||
        !core->snapshot_data[i_snapshot] ||
        !core->snapshot_mask[i_snapshot]) {
        fprintf(MEM_FILE, "Reallocating snapshot %zu from %zu to %zu bytes...\n", i_snapshot, core->nr_snapshot_data[i_snapshot], nr_data);

        //Reallocate arrays and clear masks on size changes.
        core->nr_snapshot_data[i_snapshot] = nr_data;
        core->snapshot_data[i_snapshot] = (uint8_t *)realloc(core->snapshot_data[i_snapshot], nr_data);
        core->snapshot_mask[i_snapshot] = (uint8_t *)realloc(core->snapshot_mask[i_snapshot], nr_data);
        
        if (!core->snapshot_data[i_snapshot] || !core->snapshot_mask[i_snapshot]) {
            fprintf(ERROR_FILE, "core_take_and_compare_snapshots: Unable to allocate snapshot data for %zu bytes!\n", nr_data);
            return;
        }

        memset(core->snapshot_mask[i_snapshot], 0, nr_data);
    }
    
    //Update masks if needed.
    if (mask_condition != MASK_IF_MASK_NEVER && data_condition != MASK_IF_DATA_NEVER && mask_action != MASK_THEN_NOP) {
        const uint8_t *n_p = (const uint8_t *)data;
        const uint8_t *o_p = core->snapshot_data[i_snapshot];
        uint8_t *m_p = core->snapshot_mask[i_snapshot];
        
        //Perform common masking operations more efficiently.
        bool done = false;
        
        //Any conditions we can handle in an optimized way?
        switch (size_value) {
            case MEMCON_VAR_8BIT:
                UPDATE_SNAPSHOT_MASK_CONDITION(uint8_t);
                break;
            case MEMCON_VAR_16BIT:
                UPDATE_SNAPSHOT_MASK_CONDITION(uint16_t);
                break;
            case MEMCON_VAR_32BIT:
                UPDATE_SNAPSHOT_MASK_CONDITION(uint32_t);
                break;
            case MEMCON_VAR_64BIT:
                UPDATE_SNAPSHOT_MASK_CONDITION(uint64_t);
                break;
            default:
                fprintf(ERROR_FILE, "core_take_and_compare_snapshots: Unknown data type!\n");
                break;
        };
        
        //Generic condition combination.
        if (!done) {
            switch (size_value) {
                case MEMCON_VAR_8BIT:
                    UPDATE_SNAPSHOT_MASK_CONDITION_GENERIC(uint8_t);
                    break;
                case MEMCON_VAR_16BIT:
                    UPDATE_SNAPSHOT_MASK_CONDITION_GENERIC(uint16_t);
                    break;
                case MEMCON_VAR_32BIT:
                    UPDATE_SNAPSHOT_MASK_CONDITION_GENERIC(uint32_t);
                    break;
                case MEMCON_VAR_64BIT:
                    UPDATE_SNAPSHOT_MASK_CONDITION_GENERIC(uint64_t);
                    break;
                default:
                    fprintf(ERROR_FILE, "core_take_and_compare_snapshots: Unknown data type!\n");
                    break;
            }
        }
    }

    //Update snapshot.
    memcpy(core->snapshot_data[i_snapshot], data, nr_data);
}

bool core_take_and_compare_snapshots(struct retro_core *core, const unsigned mask_condition, const unsigned data_condition, const unsigned mask_action, const unsigned size_value, const uint64_t const_value) {
    if (!core) {
        fprintf(ERROR_FILE, "core_take_and_compare_snapshots: Invalid core!\n");
        return false;
    }
    
    //Allocate data if it is not there.
    const size_t nr_snapshots = 4 + core->mmap.num_descriptors;

    fprintf(MEM_FILE, "Updating %zu snapshots with masks %u, %u, %u...\n", nr_snapshots, mask_condition, data_condition, mask_action);

    if (core->nr_snapshots != nr_snapshots) {
        free_core_snapshots(core);
        
        fprintf(CORE_FILE, "Allocating %zu snapshot arrays...\n", nr_snapshots);

        core->nr_snapshots = nr_snapshots;
        core->nr_snapshot_data = (size_t *)calloc(nr_snapshots, sizeof(size_t));
        core->snapshot_data = (uint8_t **)calloc(nr_snapshots, sizeof(void *));
        core->snapshot_mask = (uint8_t **)calloc(nr_snapshots, sizeof(void *));

        if (!core->nr_snapshot_data || !core->snapshot_data || !core->snapshot_mask) {
            fprintf(ERROR_FILE, "core_take_and_compare_snapshots: Unable to allocate snapshot arrays!\n");
            return false;
        }
    }

    //Retrieve retro_get_memory_* snapshots.
    update_snapshot(core, mask_condition, data_condition, mask_action, size_value, const_value, 0, core->retro_get_memory_size(RETRO_MEMORY_SAVE_RAM), core->retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
    update_snapshot(core, mask_condition, data_condition, mask_action, size_value, const_value, 1, core->retro_get_memory_size(RETRO_MEMORY_RTC), core->retro_get_memory_data(RETRO_MEMORY_RTC));
    update_snapshot(core, mask_condition, data_condition, mask_action, size_value, const_value, 2, core->retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), core->retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM));
    update_snapshot(core, mask_condition, data_condition, mask_action, size_value, const_value, 3, core->retro_get_memory_size(RETRO_MEMORY_VIDEO_RAM), core->retro_get_memory_data(RETRO_MEMORY_VIDEO_RAM));

    //Retrieve memory mapped snapshots.
    for (size_t i = 0; i < core->mmap.num_descriptors; i++) {
        update_snapshot(core, mask_condition, data_condition, mask_action, size_value, const_value, 4 + i, core->mmap.descriptors[i].len, core->mmap.descriptors[i].ptr);
    }

    return true;
}

bool free_core_memory_maps(struct retro_core *core) {
    if (!core) {
        fprintf(ERROR_FILE, "free_core_memory_maps: Invalid core!\n");
        return false;
    }

    if (core->mmap.descriptors) free((struct retro_memory_descriptor *)core->mmap.descriptors);

    memset(&core->mmap, 0, sizeof(struct retro_memory_map));

    return true;
}

bool set_core_memory_maps(struct retro_core *core, const struct retro_memory_map *mmap) {
    if (!core || !mmap) {
        fprintf(ERROR_FILE, "set_core_memory_maps: Invalid core or maps!\n");
        return false;
    }
    
    free_core_memory_maps(core);

    core->mmap.descriptors = (const struct retro_memory_descriptor *)calloc(mmap->num_descriptors, sizeof(struct retro_memory_descriptor));

    if (!core->mmap.descriptors) {
        fprintf(ERROR_FILE, "set_core_memory_maps: Unable to allocate descriptors!\n");
        return false;
    }

    core->mmap.num_descriptors = mmap->num_descriptors;
    
    for (unsigned i = 0; i < mmap->num_descriptors; i++) {
        const struct retro_memory_descriptor d = mmap->descriptors[i];
        
        //FIXME: Not so elegant, see what RetroArch does with this...
        *(struct retro_memory_descriptor *)&core->mmap.descriptors[i] = d;

        fprintf(CORE_FILE, "Added memory descriptor %u (%s): flags %lu, pointer %p, offset %zu, start %zu, select %zu, disconnect %zu, len %zu.\n", i, d.addrspace, d.flags, d.ptr, d.offset, d.start, d.select, d.disconnect, d.len);
    }
    
    return true;
}

bool core_unserialize_from_file(struct retro_core *core, const char *file) {
    if (!core || !file) {
        fprintf(ERROR_FILE, "core_unserialize_from_file: Invalid core or file!\n");
        return false;
    }

    FILE *f = fopen(file, "rb");

    if (!f) {
        fprintf(ERROR_FILE, "core_unserialize_from_file: Unable to open '%s'!\n", file);
        return false;
    }
    
    fseek(f, 0L, SEEK_END);

    size_t nr_bytes = ftell(f);

    rewind(f);

    void *data = calloc(nr_bytes, 1);

    if (!data) {
        fprintf(ERROR_FILE, "core_unserialize_from_file: Unable to allocate data array!\n");
        fclose(f);
        return false;
    }

    const size_t nr_read = fread(data, 1, nr_bytes, f);

    if (nr_read != nr_bytes) {
        fprintf(ERROR_FILE, "core_unserialize_from_file: Unable to read all data from '%s' (only %zu of %zu bytes)!\n", file, nr_read, nr_bytes);
        fclose(f);
        return false;
    }

    fclose(f);

    if (!core->retro_unserialize(data, nr_bytes)) {
        fprintf(ERROR_FILE, "core_unserialize_from_file: Unable to unserialize data!\n");
        free(data);
        return false;
    }

    free(data);
    
    fprintf(CORE_FILE, "Unserialized core as %zu bytes from '%s'.\n", nr_bytes, file);

    return true;
}

#define COND_GET_VALUE_TYPED(type) do { \
    if (c->offset <= nr_data - sizeof(type)) value = *(const type *)(data + c->offset); \
} while (false);

bool core_check_conditions(const struct retro_core *core, struct retro_core_memory_condition *conds, const size_t nr_conds, const bool debug) {
    if (!core || !conds) {
        fprintf(ERROR_FILE, "core_check_conditions: Invalid core or conditions!\n");
        return false;
    }

    if (nr_conds == 0) return false;
    
    struct retro_core_memory_condition *c = conds;
    const unsigned map_snapshot_to_mem[] = {RETRO_MEMORY_SAVE_RAM, RETRO_MEMORY_RTC, RETRO_MEMORY_SYSTEM_RAM, RETRO_MEMORY_VIDEO_RAM};
    
    for (size_t i = 0; i < nr_conds; ++i, ++c) {
        const uint8_t *data = NULL;
        size_t nr_data = 0;
        
        if (c->snapshot < 4) {
            data = (const uint8_t *)core->retro_get_memory_data(map_snapshot_to_mem[c->snapshot]);
            nr_data = core->retro_get_memory_size(map_snapshot_to_mem[c->snapshot]);
        }
        else if (c->snapshot < 4 + core->mmap.num_descriptors) {
            data = (const uint8_t *)core->mmap.descriptors[c->snapshot - 4].ptr;
            nr_data = core->mmap.descriptors[c->snapshot - 4].len;
        }

        if (data && nr_data > 0) {
            uint64_t value = MEMCON_VALUE_UNSET;
            
            //Retrieve desired data size.
            switch (c->type) {
                case MEMCON_VAR_8BIT:
                    COND_GET_VALUE_TYPED(uint8_t);
                    break;
                case MEMCON_VAR_16BIT:
                    COND_GET_VALUE_TYPED(uint16_t);
                    break;
                case MEMCON_VAR_32BIT:
                    COND_GET_VALUE_TYPED(uint32_t);
                    break;
                case MEMCON_VAR_64BIT:
                    COND_GET_VALUE_TYPED(uint64_t);
                    break;
                default:
                    fprintf(ERROR_FILE, "core_check_conditions: Invalid data type %u!\n", c->type);
            }
            
            //Check desired condition.
            if (value != MEMCON_VALUE_UNSET) {
                //Do not return true if we are debugging such that all conditions are monitored.
                bool triggered = false;

                switch (c->compare) {
                    case MEMCON_CMP_CHANGED:
                        if (value != c->last_value && c->last_value != MEMCON_VALUE_UNSET) triggered = true;
                        break;
                    case MEMCON_CMP_EQUAL:
                        if (value == c->value) triggered = true;
                        break;
                    case MEMCON_CMP_NOT_EQUAL:
                        if (value != c->value) triggered = true;
                        break;
                    case MEMCON_CMP_GREATER:
                        if (value > c->value) triggered = true;
                        break;
                    case MEMCON_CMP_LESS:
                        if (value < c->value) triggered = true;
                        break;
                    default:
                        fprintf(ERROR_FILE, "core_check_conditions: Invalid data comparison %u!\n", c->compare);
                }
                
                if (triggered) {
                    if (!debug) return true;
                    else if (value != c->last_value) fprintf(MEM_FILE, "Debug met condition: %08zx %08zx: %08zx --> %08zx\n", c->snapshot, c->offset, c->last_value, value);
                }

                c->last_value = value;
            }
            else {
                fprintf(ERROR_FILE, "core_check_conditions: Invalid snapshot offset %zx (> %zx) in %zx!\n", c->offset, nr_data, c->snapshot);
            }
        }
        else {
            fprintf(ERROR_FILE, "core_check_conditions: Invalid snapshot index %zx!\n", c->snapshot);
        }
    }

    return false;
}

bool core_load_conditions_from_file(struct retro_core_memory_condition **conds_p, size_t *nr_conds_p, const char *file) {
    if (!conds_p || !nr_conds_p || !file) {
        fprintf(ERROR_FILE, "core_load_conditions_from_file: Invalid conditions or file!\n");
        return false;
    }

    struct retro_core_memory_condition *conds = NULL;
    size_t nr_conds = 0;
    
    FILE *f = fopen(file, "r");
    char line[NR_CORE_OPTION_LINE];

    if (!f) {
        fprintf(ERROR_FILE, "core_load_conditions_from_file: Unable to read conditions file '%s'!\n", file);
        return false;
    }

    while (fgets(line, NR_CORE_OPTION_LINE, f)) {
        if (IS_COMMENT_LINE(line)) continue;
        
        nr_conds++;

        if (!(conds = (struct retro_core_memory_condition *)realloc(conds, nr_conds*sizeof(struct retro_core_memory_condition)))) {
            fprintf(ERROR_FILE, "core_load_conditions_from_file: Unable to allocate memory!\n");
            fclose(f);
            return false;
        }

        struct retro_core_memory_condition *c = conds + (nr_conds - 1);

        memset(c, 0, sizeof(struct retro_core_memory_condition));
        c->last_value = MEMCON_VALUE_UNSET;
        
        if (sscanf(line, "%zx %zx %x %x %lx", &c->snapshot, &c->offset, &c->type, &c->compare, &c->value) != 5) {
            fprintf(ERROR_FILE, "core_load_conditions_from_file: Unable to process line '%s'!\n", line);
            nr_conds--;
        }
    }

    fclose(f);

    *conds_p = conds;
    *nr_conds_p = nr_conds;

    fprintf(CORE_FILE, "Read %zu conditions from '%s'.\n", nr_conds, file);
    
    return true;
}

