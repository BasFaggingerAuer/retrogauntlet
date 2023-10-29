/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Mini TCP/IP network library.
#ifndef RETROGAUNTLET_NET_H__
#define RETROGAUNTLET_NET_H__

bool net_init();
bool net_quit();

bool allocate_clients(void **, const size_t);
bool free_clients(void **, const size_t);
bool client_is_client_active(void *);
bool client_is_client_new(void *);
bool client_connect_to_host(void *, const char *, const int);
bool client_listen(void *, const int);
bool client_send(void *, const void *, size_t);
bool client_set_blocking(void *, const bool);
size_t client_get_nr_data(void *);
size_t client_get_data(void *, void *, size_t);
void client_fprintf(FILE *, void *);
int client_sprintf(char *, void *);

bool free_host(void **);
bool allocate_host(void **, const int, const int);
bool host_send(void *, void *, const void *, size_t);
bool host_broadcast(void *, const void *, const size_t);
bool host_listen(void *, const int);
bool host_remove_client(void *, const int);
bool host_is_host_active(void *);
bool host_is_client_active(void *, const int);
bool host_is_client_new(void *, const int);
bool host_set_blocking(void *, const bool);
bool host_set_accepts_clients(void *, const bool);
int host_get_active_client_index(void *, int);
void *host_get_client(void *, int);
void host_fprintf(FILE *, void *);
int host_sprintf(char *, void *);

#endif

