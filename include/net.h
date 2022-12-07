/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Mini ipv6 network library.
#ifndef RETROGAUNTLET_NET_H__
#define RETROGAUNTLET_NET_H__

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <netinet/in.h>
#endif

#define NR_NET_BUFFER 65536

struct client {
    char addr_text[INET6_ADDRSTRLEN];
    int port;
    int sock;
    uint8_t buffer[NR_NET_BUFFER + 1];
    size_t nr_buffer;
    int newly_joined;
    bool blocking;
};

struct host {
    char addr_text[INET6_ADDRSTRLEN];
    int port;
    int sock;
    int max_clients;
    struct client *clients;
    int nr_clients;
    int accepts_clients;
    bool blocking;
};

bool net_init();
bool net_quit();

bool free_client(struct client *);
bool create_client(struct client *);
bool client_connect_to_host(struct client *, const char *, const int);
bool client_listen(struct client *, const int);
bool client_send(struct client *, const void *, size_t);
bool client_set_blocking(struct client *, const bool);

bool free_host(struct host *);
bool create_host(struct host *, const int, const int);
bool host_send(struct host *, struct client *, const void *, size_t);
bool host_broadcast(struct host *, const void *, const size_t);
bool host_listen(struct host *, const int);
bool host_remove_client(struct host *, const int);
bool host_set_blocking(struct host *, const bool);

#endif

