/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>

#include "stringextra.h"
#include "files.h"
#include "gauntletgame.h"

//Create all subdirectories that do not yet exist up to a given filename. (As mkdir() does not work beyond depth 1.)
bool game_create_subdirectory_for_file(const char *base, const char *file) {
    if (!base || !file) return false;

    char *path = combine_paths(base, file);

    if (!path) return false;
    
    for (char *p = path + strlen(base) + 1; *p; ++p) {
        if (*p == '/') {
            *p = 0;
            create_directory(path);
            *p = '/';
        }
    }

    free(path);

    return true;
}

void game_sort_player_indices_by_finish_time(struct gauntlet_game *game) {
    //Use simple insertion sort.
    if (!game) return;

    for (int i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        game->player_indices[i] = i;
    }

    for (int i = 1; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        const int k = game->player_indices[i];
        int j;
        
        for (j = i - 1; j >= 0; --j) {
            const int l = game->player_indices[j];

            if (game->players[l].finish_time <= game->players[k].finish_time) break;
            
            game->player_indices[j + 1] = l;
        }

        game->player_indices[j + 1] = k;
    }
}

bool game_player_give_points(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_player_give_points: Invalid game!\n");
        return false;
    }

    //Are we currently in a gauntlet?
    if (!game->is_host_gauntlet_running) return false;

    //Has everyone finished?
    bool all_finished = true;
   
    for (int i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        if (game->players[i].finish_state == RETRO_GAUNTLET_RUNNING) {
            if (i == 0) all_finished = false;
            else if (i > 0 && game->host.clients[i - 1].sock >= 0) all_finished = false;
        }
    }
    
    if (!all_finished) return false;

    //The gauntlet is done --> distribute points.
    fprintf(INFO_FILE, "Distributing points after gauntlet.\n");

    //Player has won, distribute points.
    const int finish_points[] = {10, 5, 2, 1};
    const int nr_finish_points = sizeof(finish_points)/sizeof(int);

    game_sort_player_indices_by_finish_time(game);

    int i_points = 0;

    for (int i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS && i_points < nr_finish_points; ++i) {
        const int j = game->player_indices[i];

        if (game->players[j].finish_state == RETRO_GAUNTLET_WON && game->players[j].finish_time > 0 && (j == 0 || game->host.clients[j - 1].sock >= 0)) {
            game->players[j].points += (game->players[j].last_points = finish_points[i_points++]);
        }
        else {
            game->players[j].last_points = 0;
        }
    }

    game->is_host_gauntlet_running = false;

    return true;
}

bool game_player_apply_message(struct gauntlet_game *game, struct gauntlet_player *p) {
    if (!game || !p || p->nr_data < 8) {
        fprintf(ERROR_FILE, "game_player_apply_message: Invalid player, game, or message size!\n");
        return false;
    }
    
    //Apply message.
    const uint16_t msg_type = *(uint16_t *)(p->data + 2);
    
    //Did we receive any invalid messages for a host?
    if (game->host.sock >= 0) {
        switch (msg_type) {
            case RETRO_GAUNTLET_MSG_NAME:
            case RETRO_GAUNTLET_MSG_FINISH:
                //Valid to receive as host.
                break;
            default:
                fprintf(ERROR_FILE, "game_player_apply_message: Player sent host-only message %u!\n", msg_type);
                return false;
        }
    }

    switch (msg_type) {
        case RETRO_GAUNTLET_MSG_NAME:
            //Change player name.
            strncpy(p->name, (char *)(p->data + 8), NR_RETRO_GAUNTLET_NAME);
            break;
        case RETRO_GAUNTLET_MSG_LOBBY:
            //Update lobby text.
            strncpy(game->lobby_text, (char *)(p->data + 8), NR_RETRO_GAUNTLET_MENU_TEXT);
            break;
        case RETRO_GAUNTLET_MSG_START:
            //Start selected gauntlet.
            if (true) {
                char *ini_file = combine_paths(game->menu.data_directory, (const char *)(p->data + 8));
                bool ok = game_start_gauntlet(game, ini_file);

                if (ini_file) free(ini_file);
                return ok;
            }
            break;
        case RETRO_GAUNTLET_MSG_FINISH:
            //Finish gauntlet.
            p->finish_state = *(uint32_t *)(p->data + 8);
            p->finish_time = *(uint32_t *)(p->data + 12);
            break;
        case RETRO_GAUNTLET_MSG_GET_FILES:
            //Get ready to receive files, change sockets to blocking.
            game_draw_message_to_screen(game, "Receiving data from host...");

            if (game->client.sock >= 0) client_set_blocking(&game->client, true);
            break;    
        case RETRO_GAUNTLET_MSG_FILE_START:
            //Start receiving file data.
            if (game->client_recv_fid || game->client_recv_file) {
                fprintf(ERROR_FILE, "game_player_apply_message: Received file start without completing previous file!\n");
                return false;
            }

            game->client_recv_file = combine_paths(game->menu.data_directory, (const char *)(p->data + 8));
            
            if (!game->client_recv_file) {
                fprintf(ERROR_FILE, "game_player_apply_message: Insufficient memory for file start!\n");
                return false;
            }

            if (!game_create_subdirectory_for_file(game->menu.data_directory, (const char *)(p->data + 8))) {
                fprintf(ERROR_FILE, "game_player_apply_message: Unable to create folder!\n");
                return false;
            }

            game->client_recv_fid = fopen(game->client_recv_file, "wb");

            if (!game->client_recv_fid) {
                fprintf(ERROR_FILE, "game_player_apply_message: Unable to open '%s' for writing!\n", game->client_recv_file);
                return false;
            }

            break;
        case RETRO_GAUNTLET_MSG_FILE_END:
            //Stop receiving file data.
            if (!game->client_recv_fid || !game->client_recv_file) {
                fprintf(ERROR_FILE, "game_player_apply_message: Received unexpected file end!\n");
                return false;
            }

            fprintf(INFO_FILE, "Received '%s' (%ld bytes).\n", game->client_recv_file, ftell(game->client_recv_fid));
            fclose(game->client_recv_fid);
            game->client_recv_fid = NULL;
            free(game->client_recv_file);
            game->client_recv_file = NULL;
            break;
        case RETRO_GAUNTLET_MSG_FILE_DATA:
            //Receive data.
            if (!game->client_recv_fid || !game->client_recv_file) {
                fprintf(ERROR_FILE, "game_player_apply_message: Received unexpected file data!\n");
                return false;
            }
            
            if (*(uint32_t *)(p->data + 8) != (uint32_t)ftell(game->client_recv_fid)) {
                fprintf(ERROR_FILE, "game_player_apply_message: Unexpected file data offset!\n");
                return false;
            }

            if (*(uint32_t *)(p->data + 12) != fwrite(p->data + 16, 1, *(uint32_t *)(p->data + 12), game->client_recv_fid)) {
                fprintf(ERROR_FILE, "game_player_apply_message: Unable to write file data!\n");
                return false;
            }
            break;
        default:
            fprintf(WARN_FILE, "game_player_apply_message: Unknown message type %u!\n", msg_type);
            break;
    }

    return true;
}

bool game_player_append_client_data(struct gauntlet_game *game, struct gauntlet_player *p, struct client *c) {
    if (!p || !c || !game) {
        fprintf(ERROR_FILE, "game_player_append_client_data: Invalid game, player, or client!\n");
        return false;
    }

    const struct blowfish *b = &game->fish;
    
    //Are we currently expecting a new message?
    size_t i_buffer = 0;
    
    //We want to process all data in the client's buffer.
    while (i_buffer < c->nr_buffer) {
        if (p->nr_data_expected == 0) {
            assert(p->nr_data <= 8);
            
            //Try to complete the header up to 8 bytes.
            size_t nr_header = min(8u - p->nr_data, c->nr_buffer - i_buffer);

            memcpy(p->data + p->nr_data, c->buffer + i_buffer, nr_header);
            p->nr_data += nr_header;
            i_buffer += nr_header;
            
            if (nr_header >= 8) {
                //We have enough data to analyze the header.
                blowfish_decrypt(b, (uint32_t *)(p->data + 0), (uint32_t *)(p->data + 4));

                if (*(uint16_t *)(p->data + 0) != RETRO_GAUNTLET_NET_HEADER) {
                    fprintf(ERROR_FILE, "player_append_client_data: Invalid header for network data from %s port %d!\n", c->addr_text, c->port);
                    return false;
                }
                
                if (*(uint16_t *)(p->data + 2) >= RETRO_GAUNTLET_MSG_MAX) {
                    fprintf(WARN_FILE, "player_append_client_data: Invalid network message type from %s port %d!\n", c->addr_text, c->port);
                }
                
                p->nr_data_expected = *(uint32_t *)(p->data + 4);
                
                //Expect a multiple of 8 bytes for decryption containing at least the header.
                if (p->nr_data_expected >= MAX_RETRO_GAUNTLET_MSG_DATA ||
                    p->nr_data_expected < 8 ||
                    (p->nr_data_expected & 7) != 0) {
                    fprintf(ERROR_FILE, "player_append_client_data: Invalid data size %zu from %s port %d!\n", p->nr_data_expected, c->addr_text, c->port);
                    return false;
                }
            }
        }
        else {
            //Collect the expected data after the header.
            size_t nr_body = min(p->nr_data_expected - p->nr_data, c->nr_buffer - i_buffer);

            memcpy(p->data + p->nr_data, c->buffer + i_buffer, nr_body);
            p->nr_data += nr_body;
            i_buffer += nr_body;
            
            assert(p->nr_data <= p->nr_data_expected);

            if (p->nr_data == p->nr_data_expected) {
                //We have the data that we need, so decrypt it (skipping the header).
                for (size_t i = 8; i < p->nr_data; i += 8) {
                    blowfish_decrypt(b, (uint32_t *)(p->data + i), (uint32_t *)(p->data + i + 4));
                }

                //Zero any remaining data from previous messages.
                memset(p->data + p->nr_data, 0, MAX_RETRO_GAUNTLET_MSG_DATA - p->nr_data);
                
                //Apply message.
                if (!game_player_apply_message(game, p)) {
                    fprintf(ERROR_FILE, "player_append_client_data: Unable to apply message from %s port %d!\n", c->addr_text, c->port);
                    p->nr_data = 0;
                    p->nr_data_expected = 0;
                    return false;
                }

                //Data is no longer necessary.
                memset(p->data, 0, MAX_RETRO_GAUNTLET_MSG_DATA);
                p->nr_data = 0;
                p->nr_data_expected = 0;
            }
        }
    }
    
    //Should have handled all the available data.
    assert(i_buffer == c->nr_buffer);

    c->nr_buffer = 0;
    return true;
}

size_t net_message_package(uint8_t *data, size_t nr_data, const uint16_t msg_type, const struct blowfish *b) {
    //Assumes data is an array of MAX_RETRO_GAUNTLET_MSG_DATA bytes.
    if (!b || !data) {
        fprintf(ERROR_FILE, "net_message_package: Invalid data!\n");
        return 0;
    }
    
    if (nr_data + 8 > MAX_RETRO_GAUNTLET_MSG_DATA) {
        fprintf(ERROR_FILE, "net_message_package: Message size %zu is too large!\n", nr_data);
        return 0;
    }
    
    //Move data 8 bytes to make space for the header.
    memmove(data + 8, data, nr_data);
    nr_data += 8;

    //Zero data tail.
    memset(data + nr_data, 0, MAX_RETRO_GAUNTLET_MSG_DATA - nr_data);

    //Make the amount of data a multiple of 8.
    nr_data = (((nr_data - 1) >> 3) + 1) << 3;
    
    //Setup header.
    *(uint16_t *)(data + 0) = RETRO_GAUNTLET_NET_HEADER;
    *(uint16_t *)(data + 2) = msg_type;
    *(uint32_t *)(data + 4) = nr_data;

    //Encrypt header and data.
    for (size_t i = 0; i < nr_data; i += 8) {
        blowfish_encrypt(b, (uint32_t *)(data + i), (uint32_t *)(data + i + 4));
    }
    
    return nr_data;
}

size_t game_create_net_message_name(struct gauntlet_game *game, const char *name) {
    if (!game || !name) {
        fprintf(ERROR_FILE, "game_create_net_message_name: Invalid game or name!\n");
        return 0;
    }
    
    strncpy((char *)game->message_buffer, name, NR_RETRO_GAUNTLET_NAME);
    return net_message_package(game->message_buffer, min(NR_RETRO_GAUNTLET_NAME, strlen(name) + 1), RETRO_GAUNTLET_MSG_NAME, &game->fish);
}

size_t game_create_net_message_lobby(struct gauntlet_game *game, const char *lobby) {
    if (!game || !lobby) {
        fprintf(ERROR_FILE, "game_create_net_message_lobby: Invalid game or name!\n");
        return 0;
    }
    
    strncpy((char *)game->message_buffer, lobby, NR_RETRO_GAUNTLET_MENU_TEXT);
    return net_message_package(game->message_buffer, min(NR_RETRO_GAUNTLET_MENU_TEXT, strlen(lobby) + 1), RETRO_GAUNTLET_MSG_LOBBY, &game->fish);
}

size_t game_create_net_message_start(struct gauntlet_game *game, const char *ini_file) {
    if (!game || !ini_file) {
        fprintf(ERROR_FILE, "game_create_net_message_start: Invalid game!\n");
        return 0;
    }
    
    strncpy((char *)game->message_buffer, ini_file, MAX_RETRO_GAUNTLET_MSG_DATA - 9);
    game->message_buffer[MAX_RETRO_GAUNTLET_MSG_DATA - 9] = 0;
    return net_message_package(game->message_buffer, strlen((char *)game->message_buffer) + 1, RETRO_GAUNTLET_MSG_START, &game->fish);
}

size_t game_create_net_message_finish(struct gauntlet_game *game, const uint32_t status, const uint32_t time) {
    if (!game) {
        fprintf(ERROR_FILE, "game_create_net_message_finish: Invalid game!\n");
        return 0;
    }
    
    *(uint32_t *)(game->message_buffer + 0) = status;
    *(uint32_t *)(game->message_buffer + 4) = time;
    return net_message_package(game->message_buffer, 8, RETRO_GAUNTLET_MSG_FINISH, &game->fish);
}

size_t game_create_net_message_get_files(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_create_net_message_get_files: Invalid game!\n");
        return 0;
    }
    
    return net_message_package(game->message_buffer, 0, RETRO_GAUNTLET_MSG_GET_FILES, &game->fish);
}

size_t game_create_net_message_file_start(struct gauntlet_game *game, const char *file) {
    if (!game || !file) {
        fprintf(ERROR_FILE, "game_create_net_message_file_start: Invalid game!\n");
        return 0;
    }
    
    strncpy((char *)game->message_buffer, file, MAX_RETRO_GAUNTLET_MSG_DATA - 9);
    game->message_buffer[MAX_RETRO_GAUNTLET_MSG_DATA - 9] = 0;
    return net_message_package(game->message_buffer, strlen((char *)game->message_buffer) + 1, RETRO_GAUNTLET_MSG_FILE_START, &game->fish);
}

size_t game_create_net_message_file_end(struct gauntlet_game *game, const char *file) {
    if (!game || !file) {
        fprintf(ERROR_FILE, "game_create_net_message_file_end: Invalid game!\n");
        return 0;
    }
    
    strncpy((char *)game->message_buffer, file, MAX_RETRO_GAUNTLET_MSG_DATA - 9);
    game->message_buffer[MAX_RETRO_GAUNTLET_MSG_DATA - 9] = 0;
    return net_message_package(game->message_buffer, strlen((char *)game->message_buffer) + 1, RETRO_GAUNTLET_MSG_FILE_END, &game->fish);
}

size_t game_create_net_message_file_data(struct gauntlet_game *game, const size_t offset, const size_t len, const uint8_t *data) {
    if (!game || !data) {
        fprintf(ERROR_FILE, "game_create_net_message_file_data: Invalid game!\n");
        return 0;
    }

    if (len == 0 || len > MAX_RETRO_GAUNTLET_MSG_DATA - 16 || len > NR_RETRO_NET_FILE_DATA) {
        fprintf(ERROR_FILE, "game_create_net_message_file_data: Length %zu is invalid!\n", len);
        return 0;
    }
    
    *(uint32_t *)(game->message_buffer + 0) = offset;
    *(uint32_t *)(game->message_buffer + 4) = len;
    memcpy(game->message_buffer + 8, data, len);
    return net_message_package(game->message_buffer, 8 + len, RETRO_GAUNTLET_MSG_FILE_DATA, &game->fish);
}

bool game_expand_path_and_file_name(char **full_path_p, char **full_file_p, const char *path, const char *file) {
    if (!full_path_p || !full_file_p) return false;

    char *full_path = expand_to_full_path(path);
    char *full_file = expand_to_full_path(file);

    *full_path_p = NULL;
    *full_file_p = NULL;

    if (!full_path || !full_file) {
        fprintf(ERROR_FILE, "game_expand_path_and_file_name: Unable to expand file names!\n");
        return false;
    }

    //Get rid of file separator at the end of the full path if it is there.
    if (full_path[strlen(full_path) - 1] == '/') full_path[strlen(full_path) - 1] = 0;
    
    //Full file path should reside in the data directory.
    if (strncmp(full_path, full_file, strlen(full_path)) != 0) {
        fprintf(ERROR_FILE, "game_expand_path_and_file_name: File '%s' should be located in directory '%s'!\n", full_file, full_path);
        free(full_file);
        free(full_path);
        return false;
    }
    
    *full_path_p = full_path;
    *full_file_p = full_file;
    return true;
}

bool game_host_broadcast_file(struct gauntlet_game *game, const char *file) {
    if (!game || !file) {
        fprintf(ERROR_FILE, "game_host_broadcast_file: Invalid game or file!\n");
        return false;
    }
    
    if (game->host.sock < 0) {
        fprintf(ERROR_FILE, "game_host_broadcast_file: Host is not running!\n");
        return false;
    }
    
    //Expand to full path, resolve symlinks.
    char *data_directory, *full_file;
    
    if (!game_expand_path_and_file_name(&data_directory, &full_file, game->menu.data_directory, file)) return false;
    
    //Open file for reading.
    FILE *fid = fopen(full_file, "rb");
    bool ok = true;

    if (!fid) {
        fprintf(ERROR_FILE, "game_host_broadcast_file: Unable to open '%s' for reading!\n", full_file);
        free(full_file);
        free(data_directory);
        return false;
    }
    
    ok = ok && host_broadcast(&game->host, game->message_buffer, game_create_net_message_file_start(game, full_file + strlen(data_directory) + 1));
    
    //Read and broadcast data.
    uint8_t buffer[NR_RETRO_NET_FILE_DATA];
    size_t offset = 0;
    size_t len = 0;

    while ((len = fread(buffer, 1, NR_RETRO_NET_FILE_DATA, fid)) > 0) {
        ok = ok && host_broadcast(&game->host, game->message_buffer, game_create_net_message_file_data(game, offset, len, buffer));
        offset += len;
    }

    ok = ok && host_broadcast(&game->host, game->message_buffer, game_create_net_message_file_end(game, full_file + strlen(data_directory) + 1));
    
    //Clean up.
    fclose(fid);
    free(full_file);
    free(data_directory);
    
    if (!ok) {
        fprintf(ERROR_FILE, "game_host_broadcast_file: Unable to broadcast '%s'!\n", full_file);
        return false;
    }

    fprintf(INFO_FILE, "Broadcasted '%s' (%zu bytes) to clients.\n", file, offset);

    return true;
}

void game_draw_message_to_screen(struct gauntlet_game *game, const char *format, ...) {
    if (!game || !format) return;

    va_list args;

    va_start(args, format);
    vsnprintf(game->menu.text, NR_RETRO_GAUNTLET_MENU_TEXT, format, args);
    va_end(args);

    fprintf(INFO_FILE, "%s\n", game->menu.text);
    
    menu_draw(&game->menu);
    video_refresh_from_sdl_surface(&game->menu.video, game->menu.surface);
    video_render(&game->menu.video);
    SDL_GL_SwapWindow(game->window);
}

bool create_game(struct gauntlet_game *game, const char *data_directory, SDL_Window *window) {
    if (!game || !data_directory || !window) {
        fprintf(ERROR_FILE, "create_game: Invalid game, data directory, or window!\n");
        return false;
    }

    memset(game, 0, sizeof(struct gauntlet_game));

    //Setup memory snapshotting settings.
    game->snapshot_mask_condition = MASK_IF_MASK_NEVER;
    game->snapshot_data_condition = MASK_IF_DATA_NEVER;
    game->snapshot_mask_action = MASK_THEN_SET_ZERO;
    game->snapshot_mask_size = MEMCON_VAR_8BIT;
    game->snapshot_const_value = 0;
    
    //Get window dimensions.
    int width = 640;
    int height = 480;

    SDL_GetWindowSize(window, &width, &height);
    game->window = window;

    //Load menu.
    char *menu_ini_file = combine_paths(data_directory, "menu.ini");

    if (!create_menu(&game->menu, menu_ini_file, data_directory, width, height)) {
        fprintf(ERROR_FILE, "create_game: Unable to create menu!\n");
        return false;
    }
    
    if (menu_ini_file) free(menu_ini_file);
    
    //Load gauntlets.
    game_draw_message_to_screen(game, "Loading...");
    menu_start_mixer(&game->menu);

    if (!read_gauntlet_playlist(&game->gauntlets, &game->nr_gauntlets, data_directory)) {
        fprintf(ERROR_FILE, "create_game: Unable to read gauntlet playlist!\n");
        return false;
    }

    game->i_gauntlet = 0;

    //Setup network.
    create_client(&game->client);
    free_host(&game->host);
    for (size_t i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) create_player(&game->players[i]);
    strcpy(game->players[0].name, game->menu.player_name);

    if (!create_blowfish(&game->fish, (uint8_t *)game->menu.password, strlen(game->menu.password))) {
        fprintf(ERROR_FILE, "create_game: Unable to initialize networking!\n");
        return false;
    }

    SDL_Delay(100);
    game->menu.state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;

    return true;
}

bool free_game(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "free_game: Invalid game!\n");
        return false;
    }
    
    //Stop gauntlet.
    game_stop_gauntlet(game);

    //Free networking.
    free_host(&game->host);
    free_client(&game->client);
    free_blowfish(&game->fish);

    //Free menu.
    menu_stop_mixer(&game->menu);
    free_menu(&game->menu);

    //Free gauntlets.
    free_sdl_gl_if(&game->sgci);
    
    if (game->gauntlets) {
        for (size_t i = 0; i < game->nr_gauntlets; ++i) free_gauntlet(&game->gauntlets[i]);
        free(game->gauntlets);
    }

    memset(game, 0, sizeof(struct gauntlet_game));

    return true;
}

bool game_start_host(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_start_hosting: Invalid game or port!\n");
        return false;
    }
    
    game_stop_client(game);
    game_stop_host(game);
    
    create_player(&game->players[0]);
    strcpy(game->players[0].name, game->menu.player_name);

    strcpy(game->lobby_text, "Waiting for lobby update...");

    if (!create_host(&game->host, game->menu.network_port, MAX_RETRO_GAUNTLET_CLIENTS)) {
        menu_draw_message(&game->menu, "Unable to host gauntlet!");
        return false;
    }
    
    game->menu.state = RETRO_GAUNTLET_STATE_LOBBY_HOST;

    return true;
}

bool game_stop_host(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_start_hosting: Invalid game!\n");
        return false;
    }
    
    game_stop_gauntlet(game);
    free_host(&game->host);
    game->menu.state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;

    return true;
}

bool game_host_start_gauntlet(struct gauntlet_game *game) {
    if (!game || game->host.sock < 0) {
        fprintf(ERROR_FILE, "game_host_start_gauntlet: Invalid game or host!\n");
        return false;
    }

    //Reset all player states.
    for (size_t i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        game->players[i].finish_state = RETRO_GAUNTLET_RUNNING;
        game->players[i].finish_time = 0;
        game->players[i].last_points = 0;
    }

    struct gauntlet *g = &game->gauntlets[game->i_gauntlet];

    if (!g->ini_file || !g->win_condition_file) {
        fprintf(ERROR_FILE, "game_host_start_gauntlet: Missing INI or win condition!\n");
        return false;
    }
    
    //Setup file syncing.
    game_draw_message_to_screen(game, "Synchronizing data with clients...");

    host_set_blocking(&game->host, true);
    
    if (!host_broadcast(&game->host, game->message_buffer, game_create_net_message_get_files(game))) {
        fprintf(ERROR_FILE, "game_host_start_gauntlet: Unable to broadcast get files!\n");
        return false;
    }

    //Send file updates.
    bool ok = true;
    
    if (game->menu.sync_level >= RETRO_GAUNTLET_SYNC_INI) {
        if (ok) ok = ok && game_host_broadcast_file(game, g->ini_file);
        if (ok && g->win_condition_file) ok = ok && game_host_broadcast_file(game, g->win_condition_file);
        if (ok && g->lose_condition_file) ok = ok && game_host_broadcast_file(game, g->lose_condition_file);
        if (ok && g->rom_startup_file) ok = ok && game_host_broadcast_file(game, g->rom_startup_file);
        if (ok && g->core_variables_file) ok = ok && game_host_broadcast_file(game, g->core_variables_file);
        
        if (game->menu.sync_level >= RETRO_GAUNTLET_SYNC_SAVE) {
            if (ok && g->core_save_file) ok = ok && game_host_broadcast_file(game, g->core_save_file);

            if (game->menu.sync_level >= RETRO_GAUNTLET_SYNC_ROM) {
                if (ok && g->rom_file) ok = ok && game_host_broadcast_file(game, g->rom_file);

                if (game->menu.sync_level >= RETRO_GAUNTLET_SYNC_ALL) {
                    if (ok && g->core_library_file_win64) ok = ok && game_host_broadcast_file(game, g->core_library_file_win64);
                    if (ok && g->core_library_file_linux64) ok = ok && game_host_broadcast_file(game, g->core_library_file_linux64);
                }
            }
        }
    }

    if (!ok) {
        fprintf(ERROR_FILE, "game_host_start_gauntlet: Unable to sync files!\n");
        return false;
    }

    //Send game start command.
    char *data_directory, *ini_file;

    if (!game_expand_path_and_file_name(&data_directory, &ini_file, game->menu.data_directory, g->ini_file)) return false;

    if (!host_broadcast(&game->host, game->message_buffer, game_create_net_message_start(game, ini_file + strlen(data_directory)))) {
        fprintf(ERROR_FILE, "game_host_start_gauntlet: Unable to broadcast gauntlet start!\n");
        free(data_directory);
        free(ini_file);
        return false;
    }

    free(data_directory);

    fprintf(INFO_FILE, "Starting gauntlet '%s' as host...\n", g->title);
    
    game->is_host_gauntlet_running = true;

    if (!game_start_gauntlet(game, ini_file)) {
        free(ini_file);
        return false;
    }
    
    free(ini_file);

    return true;
}

bool game_update_host(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_update_host: Invalid game or host!\n");
        return false;
    }
    
    //Gather data from clients and new connections.
    if (game->host.sock < 0 || !host_listen(&game->host, 0)) {
        game_stop_host(game);
        menu_draw_message(&game->menu, "Host network error!");
        return false;
    }

    for (int i = 0; i < game->host.max_clients; ++i) {
        if (game->host.clients[i].sock >= 0) {
            if (game->host.clients[i].newly_joined) {
                //A new player has joined.
                create_player(&game->players[i + 1]);
                game->host.clients[i].newly_joined = 0;
            }

            //Act on any data the clients provide.
            if (!game_player_append_client_data(game, &game->players[i + 1], &game->host.clients[i])) {
                host_remove_client(&game->host, i);
            }
        }
    }

    return true;
}

bool create_player(struct gauntlet_player *p) {
    if (!p) {
        fprintf(ERROR_FILE, "create_player: Invalid player!\n");
        return false;
    }

    memset(p, 0, sizeof(struct gauntlet_player));
    strncpy(p->name, "Unknown", NR_RETRO_GAUNTLET_NAME);
    p->points = 0;
    p->last_points = 0;

    return true;
}

char *player_strncat(char *str, const struct gauntlet_player *p, const size_t len) {
    if (!str || !p) return NULL;

    snprintf(str + strlen(str), len - strlen(str), "%16s: %4u ", p->name, p->points);
    snprintf(str + strlen(str), len - strlen(str), "(+%2u) points ", p->last_points);
    
    const uint32_t t = p->finish_time;

    if (t > 0u) {
        snprintf(str + strlen(str), len - strlen(str), "%02u:%02u:%02u.%03u ",
            t/3600000u, (t/60000u) % 60u, (t/1000u) % 60u, t % 1000u);
    }
    
    switch (p->finish_state) {
        case RETRO_GAUNTLET_RUNNING:
            strncat(str, "running\n", len - strlen(str));
            break;
        case RETRO_GAUNTLET_WON:
            strncat(str, "won\n", len - strlen(str));
            break;
        case RETRO_GAUNTLET_LOST:
            strncat(str, "lost\n", len - strlen(str));
            break;
        default:
            strncat(str, "none\n", len - strlen(str));
            break;
    }
    
    return str;
}

bool game_start_client(struct gauntlet_game *game, const char *host_name) {
    if (!game) {
        fprintf(ERROR_FILE, "game_start_client: Invalid game!\n");
        return false;
    }
    
    if (!host_name) {
        menu_draw_message(&game->menu, "Please copy a hostname to the clipboard before joining!");
        return false;
    }
    
    //Free existing client.
    game_stop_client(game);
    game_stop_host(game);

    strcpy(game->lobby_text, "Waiting for lobby update...");
    
    //Create new client.
    if (!create_client(&game->client)) {
        menu_draw_message(&game->menu, "Unable to create client!");
        return false;
    }
    
    //Connect to host.
    if (!client_connect_to_host(&game->client, host_name, game->menu.network_port)) {
        free_client(&game->client);
        menu_draw_message(&game->menu, "Unable to join host %s at port %d!", host_name, game->menu.network_port);
        return false;
    }
    
    //Send player name.
    create_player(&game->players[0]);
    strcpy(game->players[0].name, game->menu.player_name);

    client_send(&game->client, game->message_buffer,
        game_create_net_message_name(game, game->menu.player_name));
    
    //Transition to lobby.
    game->menu.state = RETRO_GAUNTLET_STATE_LOBBY_CLIENT;

    //Play login sound.
    soundboard_play(&game->menu.login_board, -1);

    return true;
}

bool game_stop_client(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_stop_client: Invalid game!\n");
        return false;
    }
    
    if (game->client_recv_fid) fclose(game->client_recv_fid);
    game->client_recv_fid = NULL;
    if (game->client_recv_file) free(game->client_recv_file);
    game->client_recv_file = NULL;

    game_stop_gauntlet(game);
    free_client(&game->client);
    game->menu.state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;

    return true;
}

bool game_update_client(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_update_client: Invalid game!\n");
        return false;
    }
    
    //Receive data.
    if (game->client.sock < 0 || !client_listen(&game->client, 0)) {
        game_stop_client(game);
        menu_draw_message(&game->menu, "Connection to host lost!");
    }

    //And process it.
    if (!game_player_append_client_data(game, &game->players[0], &game->client)) {
        game_stop_client(game);
        menu_draw_message(&game->menu, "Connection to host lost!");
    }

    return true;
}

void game_sort_player_indices_by_score(struct gauntlet_game *game) {
    //Use simple insertion sort.
    if (!game) return;

    for (int i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        game->player_indices[i] = i;
    }

    for (int i = 1; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        const int k = game->player_indices[i];
        int j;
        
        for (j = i - 1; j >= 0; --j) {
            const int l = game->player_indices[j];

            if (game->players[l].points >= game->players[k].points) break;
            
            game->player_indices[j + 1] = l;
        }

        game->player_indices[j + 1] = k;
    }
}

void game_update_lobby_text(struct gauntlet_game *game) {
    if (!game) return;
    
    //If we are a client we will receive the lobby from the host.
    if (game->host.sock < 0) return;

    //If we are the host, update the lobby state and send this out regularly.
    strcpy(game->lobby_text, "Lobby:\n");
    
    game_sort_player_indices_by_score(game);

    for (int i = 0; i <= MAX_RETRO_GAUNTLET_CLIENTS; ++i) {
        const int j = game->player_indices[i];

        if (j == 0 || game->host.clients[j - 1].sock >= 0) player_strncat(game->lobby_text, &game->players[j], NR_RETRO_GAUNTLET_MENU_TEXT);
    }
    
    //Send out lobby updates at ~4 [Hz].
    uint32_t t = SDL_GetTicks();

    if (t > game->last_lobby_update_time + 250 && strcmp(game->lobby_text, game->last_lobby_text) != 0) {
        game->last_lobby_update_time = t;
        strcpy(game->last_lobby_text, game->lobby_text);
        host_broadcast(&game->host, game->message_buffer,
            game_create_net_message_lobby(game, game->lobby_text));
    }
}

void game_update_menu_text(struct gauntlet_game *game) {
    if (!game) return;

    switch (game->menu.state) {
        case RETRO_GAUNTLET_STATE_SELECT_GAUNTLET:
            do {
                strcpy(game->menu.text, "<F1>    : Host network game\n<F2>    : Join network game from clipboard address\n<Arrows>: Select gauntlet\n<R>     : Select random gauntlet\n<F>     : Toggle fullscreen\n<Enter> : Run gauntlet single player\nPress <Pause> while running a gauntlet to perform memory inspection\n\n");
                const int i_start = max(0, (int)game->i_gauntlet - 6);
                const int i_end = min((int)game->nr_gauntlets, i_start + 14);

                for (int i = i_start; i < i_end; ++i) {
                    strcat(game->menu.text, (i == (int)game->i_gauntlet ? "> " : "  "));
                    strcat(game->menu.text, game->gauntlets[i].title);
                    strcat(game->menu.text, "\n");
                }
            } while (false);
            break;
        case RETRO_GAUNTLET_STATE_SETUP_GAUNTLET:
            strcpy(game->menu.text, "<F1>, <F2>: Set masks to 0, 1.\n<F3>: Data condition for frame-to-frame data changes.\n<F4>: Action to output memory locations.\n<F5>, <F9>: Write, load core save snapshot.\n<A>, <N>, <0>, <1>: Mask condition always, never, equal to 0, equal to 1.\n<Z>, <X>, <C>, <V>: Data condition always, less than, equal to, greater than previous frame data value.\n<G>, <H>, <J>: Data condition less than, equal to, greater than given constant.\n<Enter>: Get constant from clipboard.\n<\\>, <[>, <]>, -, =, <O>, <P>: Action to change mask values, OR, AND, XOR with 1, set to 0, set to 1, add 1, sub 1.\n<4>, <5>, <6>, <7>: Compare 8, 16, 32, 64 bit values.\n<Space>: Apply action.\n\n");
            strcat(game->menu.text, "If mask condition ");

            switch (game->snapshot_mask_condition) {
                case MASK_IF_MASK_ALWAYS:
                    strcat(game->menu.text, "ALWAYS");
                    break;
                case MASK_IF_MASK_NEVER:
                    strcat(game->menu.text, "NEVER");
                    break;
                case MASK_IF_MASK_ZERO:
                    strcat(game->menu.text, "IS ZERO");
                    break;
                case MASK_IF_MASK_ONE:
                    strcat(game->menu.text, "IS ONE");
                    break;
                default:
                    strcat(game->menu.text, "UNKNOWN");
                    break;
            }

            strcat(game->menu.text, " and ");

            switch (game->snapshot_mask_size) {
                case MEMCON_VAR_8BIT:
                    strcat(game->menu.text, "8BIT");
                    break;
                case MEMCON_VAR_16BIT:
                    strcat(game->menu.text, "16BIT");
                    break;
                case MEMCON_VAR_32BIT:
                    strcat(game->menu.text, "32BIT");
                    break;
                case MEMCON_VAR_64BIT:
                    strcat(game->menu.text, "64BIT");
                    break;
                default:
                    strcat(game->menu.text, "UNKNOWN");
                    break;
            }

            strcat(game->menu.text, " memory ");

            switch (game->snapshot_data_condition) {
                case MASK_IF_DATA_ALWAYS:
                    strcat(game->menu.text, "ALWAYS");
                    break;
                case MASK_IF_DATA_NEVER:
                    strcat(game->menu.text, "NEVER");
                    break;
                case MASK_IF_DATA_CHANGED:
                    strcat(game->menu.text, "CHANGED FROM LAST FRAME");
                    break;
                case MASK_IF_DATA_EQUAL_PREV:
                    strcat(game->menu.text, "EQUALS LAST FRAME");
                    break;
                case MASK_IF_DATA_LESS_PREV:
                    strcat(game->menu.text, "IS LESS THAN LAST FRAME");
                    break;
                case MASK_IF_DATA_GREATER_PREV:
                    strcat(game->menu.text, "IS GREATER THAN LAST FRAME");
                    break;
                case MASK_IF_DATA_EQUAL_CONST:
                    sprintf(game->menu.text + strlen(game->menu.text), "EQUALS %zu", (size_t)game->snapshot_const_value);
                    break;
                case MASK_IF_DATA_LESS_CONST:
                    sprintf(game->menu.text + strlen(game->menu.text), "IS LESS THAN %zu", (size_t)game->snapshot_const_value);
                    break;
                case MASK_IF_DATA_GREATER_CONST:
                    sprintf(game->menu.text + strlen(game->menu.text), "IS GREATER THAN %zu", (size_t)game->snapshot_const_value);
                    break;
                default:
                    strcat(game->menu.text, "UNKNOWN");
                    break;
            }

            strcat(game->menu.text, ", then ");

            switch (game->snapshot_mask_action) {
                case MASK_THEN_NOP:
                    strcat(game->menu.text, "DO NOTHING");
                    break;
                case MASK_THEN_SET_ZERO:
                    strcat(game->menu.text, "MASK = 0");
                    break;
                case MASK_THEN_SET_ONE:
                    strcat(game->menu.text, "MASK = 1");
                    break;
                case MASK_THEN_OR_ONE:
                    strcat(game->menu.text, "MASK |= 1");
                    break;
                case MASK_THEN_AND_ONE:
                    strcat(game->menu.text, "MASK &= 1");
                    break;
                case MASK_THEN_XOR_ONE:
                    strcat(game->menu.text, "MASK ^= 1");
                    break;
                case MASK_THEN_ADD_ONE:
                    strcat(game->menu.text, "MASK += 1");
                    break;
                case MASK_THEN_SUB_ONE:
                    strcat(game->menu.text, "MASK -= 1");
                    break;
                case MASK_THEN_LOG:
                    strcat(game->menu.text, "LOG OFFSET");
                    break;
                default:
                    strcat(game->menu.text, "UNKNOWN");
                    break;
            }

            strcat(game->menu.text, ".\n");
            break;
        case RETRO_GAUNTLET_STATE_MESSAGE:
        case RETRO_GAUNTLET_STATE_QUIT_CONFIRM:
            //Leave message as-is.
            break;
        case RETRO_GAUNTLET_STATE_LOBBY_HOST:
            strcpy(game->menu.text, "<ESC>   : Stop hosting\n<Arrows>: Select gauntlet\n<R>     : Select random gauntlet\n<G>     : Run gauntlet multi player\n<F>     : Toggle fullscreen\n\n");
            sprintf(game->menu.text + strlen(game->menu.text), "Hosting at local address %s port %d (%d/%d players have joined).\n\n", game->host.addr_text, game->host.port, game->host.nr_clients, game->host.max_clients);
            sprintf(game->menu.text + strlen(game->menu.text), "%s:\n%s\n%s\n\n", game->gauntlets[game->i_gauntlet].title, game->gauntlets[game->i_gauntlet].description, game->gauntlets[game->i_gauntlet].controls);
            if (strlen(game->menu.text) + strlen(game->lobby_text) + 1 < NR_RETRO_GAUNTLET_MENU_TEXT) strcat(game->menu.text, game->lobby_text);
            break;
        case RETRO_GAUNTLET_STATE_LOBBY_CLIENT:
            sprintf(game->menu.text, "Joined %s port %d.\n", game->client.addr_text, game->client.port);
            strcat(game->menu.text, "<ESC>   : Leave lobby\n<F>     : Toggle fullscreen\n\n");
            if (strlen(game->menu.text) + strlen(game->lobby_text) + 1 < NR_RETRO_GAUNTLET_MENU_TEXT) strcat(game->menu.text, game->lobby_text);
            break;
        default:
            strcpy(game->menu.text, "Unknown state?");
            break;
    }
    
    menu_draw(&game->menu);
}

void game_update(struct gauntlet_game *game) {
    if (!game) return;
    
    //Always perform host updates.
    if (game->host.sock >= 0) {
        game_update_host(game);
        game_player_give_points(game);
        game_update_lobby_text(game);
    }

    if (game->menu.state == RETRO_GAUNTLET_STATE_RUN_CORE) {
        //Check whether we satisfy win/lose conditions.
        gauntlet_check_status(&game->gauntlet, &game->sgci);
        
        //Did we stop running?
        if (game->gauntlet.status != RETRO_GAUNTLET_RUNNING) {
            const uint32_t t = game->gauntlet.end_time - game->gauntlet.start_time;
            const uint32_t pt = game->gauntlet.par_time;
            
            if (game->host.sock >= 0) game->menu.state = RETRO_GAUNTLET_STATE_LOBBY_HOST;
            else if (game->client.sock >= 0) game->menu.state = RETRO_GAUNTLET_STATE_LOBBY_CLIENT;
            else game->menu.state = RETRO_GAUNTLET_STATE_SELECT_GAUNTLET;

            const bool win = (game->gauntlet.status == RETRO_GAUNTLET_WON);
            
            if (game->host.sock < 0 && game->client.sock < 0) {
                switch (game->gauntlet.status) {
                    case RETRO_GAUNTLET_WON:
                        menu_draw_message(&game->menu, "You won!\n\nTime %02u:%02u:%02u.%03u (par %02u:%02u:%02u.%03u)\n",
                            t/3600000u, (t/60000u) % 60u, (t/1000u) % 60u, t % 1000u,
                            pt/3600000u, (pt/60000u) % 60u, (pt/1000u) % 60u, pt % 1000u);
                        break;
                    case RETRO_GAUNTLET_LOST:
                        menu_draw_message(&game->menu, "You lost!\n");
                        break;
                    default:
                        menu_draw_message(&game->menu, "An error occurred!\n");
                        break;
                }
            }

            game->players[0].finish_state = game->gauntlet.status;
            game->players[0].finish_time = t;

            if (game->client.sock >= 0) {
                //Update host the we completed the gauntlet.
                client_send(&game->client, game->message_buffer,
                    game_create_net_message_finish(game, game->players[0].finish_state, game->players[0].finish_time));
            }
            
            if (game->host.sock >= 0) {
                //Accept new connections again.
                game->host.accepts_clients = 1;
            }

            game_stop_gauntlet(game);
            menu_start_mixer(&game->menu);
            soundboard_play(win ? &game->menu.win_board : &game->menu.lose_board, -1);
        }
        else {
            //Draw libretro core output.
            video_bind_frame_buffer(&game->sgci.video);
            game->sgci.core.retro_run();
            video_unbind_frame_buffer(&game->sgci.video);
            video_render(&game->sgci.video);
            
            //Compare memory snapshot if desired.
            if (game->snapshot_data_condition == MASK_IF_DATA_CHANGED) core_take_and_compare_snapshots(&game->sgci.core, game->snapshot_mask_condition, game->snapshot_data_condition, game->snapshot_mask_action, game->snapshot_mask_size, game->snapshot_const_value);
        }
    }
    else {
        //Client network updates are performed only when we are not running a core.
        if (game->client.sock >= 0) game_update_client(game);
        
        //Update menu text.
        game_update_menu_text(game);
        
        //Draw menu.
        video_refresh_from_sdl_surface(&game->menu.video, game->menu.surface);
        video_render(&game->menu.video);
    }
}

void game_change_gauntlet_selection(struct gauntlet_game *game, const int delta) {
    if (!game) return;
    
    if (delta < 0 && delta < -(int)game->i_gauntlet) game->i_gauntlet = 0;
    else game->i_gauntlet += delta;
    
    if (game->i_gauntlet >= game->nr_gauntlets) game->i_gauntlet = game->nr_gauntlets - 1;
}

void game_select_rand_gauntlet(struct gauntlet_game *game) {
    if (!game) return;

    game->i_gauntlet = rand() % game->nr_gauntlets;
}

bool game_stop_gauntlet(struct gauntlet_game *game) {
    if (!game) {
        fprintf(ERROR_FILE, "game_stop_gauntlet: Invalid game!\n");
        return false;
    }
    
    SDL_ShowCursor(SDL_ENABLE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    gauntlet_stop(&game->gauntlet);
    free_sdl_gl_if(&game->sgci);
    free_gauntlet(&game->gauntlet);

    //Set networking to be blocking.
    if (game->client.sock >= 0) client_set_blocking(&game->client, true);
    if (game->host.sock >= 0) host_set_blocking(&game->host, true);

    return true;
}

bool game_start_gauntlet(struct gauntlet_game *game, const char *gauntlet_ini_file) {
    if (!game || !gauntlet_ini_file) {
        fprintf(ERROR_FILE, "game_start_gauntlet: Invalid game!\n");
        return false;
    }
    
    game_stop_gauntlet(game);
    game->snapshot_mask_condition = MASK_IF_MASK_NEVER;
    game->snapshot_data_condition = MASK_IF_DATA_NEVER;

    if (!create_gauntlet(&game->gauntlet, gauntlet_ini_file, game->menu.data_directory)) {
        fprintf(ERROR_FILE, "game_start_gauntlet: Unable to create gauntlet!\n");
        return false;
    }

    fprintf(INFO_FILE, "Opened gauntlet '%s' from '%s'.\n", game->gauntlet.title, gauntlet_ini_file);

    //Set networking to be non-blocking.
    if (game->client.sock >= 0) client_set_blocking(&game->client, false);
    if (game->host.sock >= 0) host_set_blocking(&game->host, false);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
    
    return true;
}


