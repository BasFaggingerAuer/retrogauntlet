/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

#include <unistd.h>
#include <errno.h>

#include "retrogauntlet.h"
#include "net.h"

//Implementation based on examples from https://www.lugod.org/presentations/ipv6programming/ and https://learn.microsoft.com/en-us/windows/win32/winsock/appendix-b-ip-version-agnostic-source-code-2/.

//Turn an address into a string that can be read by users.
const char *get_addr_string(const struct sockaddr *addr, char *text, size_t max_text) {
    if (!addr || !text || max_text == 0) return NULL;

    switch (addr->sa_family) {
        case AF_INET:
            return inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), text, max_text);
        case AF_INET6:
            return inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr), text, max_text);
        default:
            return NULL;
    }

    return NULL;
}

//Get flags for sending/receiving from sockets.
#ifdef _WIN32
int get_block_flags(const bool UNUSED(blocking)) {
    return 0;
}
#else
int get_block_flags(const bool blocking) {
    return (blocking ? 0 : MSG_DONTWAIT);
}
#endif

//Enable or disable blocking for a network socket.
bool set_socket_blocking(const int sock, const bool blocking) {
    if (sock < 0) return false;

#ifdef _WIN32
    u_long mode = (blocking ? 0 : 1);

    if (ioctlsocket(sock, FIONBIO, &mode) == 0) return true;
#else
    int flags = fcntl(sock, F_GETFL, 0);

    flags = (blocking ? (flags & (~O_NONBLOCK)) : (flags | O_NONBLOCK));

    if (fcntl(sock, F_SETFL, flags) != -1)  return true;
#endif

    return false;
}

//Reduce TCP delays for small packets.
bool set_socket_no_delay(const int sock) {
    if (sock < 0) return false;

#ifdef TCP_NODELAY
    int yes = 1;

    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(yes));
#endif

    return true;
}

void close_socket_gen(const int sock) {
    if (sock < 0) return;

#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
}

bool free_client(struct client *c) {
    if (!c) {
        fprintf(ERROR_FILE, "free_client: Invalid client!\n");
        return false;
    }

    if (c->sock >= 0) close_socket_gen(c->sock);

    memset(c, 0, sizeof(struct client));
    c->sock = -1;
    
    return true;
}

bool create_client(struct client *c) {
    if (!c) {
        fprintf(ERROR_FILE, "create_client: Invalid client!\n");
        return false;
    }

    memset(c, 0, sizeof(struct client));
    c->sock = -1;
    c->blocking = true;
    
    return true;
}

bool client_connect_to_host(struct client *c, const char *address, const int port) {
    if (!c || !address) {
        fprintf(ERROR_FILE, "client_connect_to_host: Invalid client or address!\n");
        return false;
    }

    if (c->sock >= 0) close_socket_gen(c->sock);

    c->sock = -1;
    c->blocking = true;
    c->nr_buffer = 0;
    
    char port_string[9] = {0};
    struct addrinfo info, *result = NULL;

    snprintf(port_string, 8, "%d", port);

    memset(&info, 0, sizeof(struct addrinfo));
    info.ai_family = PF_UNSPEC;
    info.ai_socktype = SOCK_STREAM;
    info.ai_protocol = IPPROTO_TCP;
    
    if (getaddrinfo(address, port_string, &info, &result) != 0) {
        fprintf(ERROR_FILE, "client_connect_to_host: Unable to get address info of '%s' port %d!\n", address, port);
        return false;
    }

    c->port = port;

    for (struct addrinfo *r = result; r; r = r->ai_next) {
        int sock = socket(r->ai_family, r->ai_socktype, r->ai_protocol);

#ifdef _WIN32
        if (sock == (int)INVALID_SOCKET) {
            fprintf(ERROR_FILE, "client_connect_to_host: Unable to create socket: %d!\n", WSAGetLastError());
#else
        if (sock == -1) {
            fprintf(ERROR_FILE, "client_connect_to_host: Unable to create socket: %s!\n", strerror(errno));
#endif
            return false;
        }

        get_addr_string((const struct sockaddr *)r->ai_addr, c->addr_text, INET6_ADDRSTRLEN);
        
        if (connect(sock, r->ai_addr, r->ai_addrlen) == 0) {
            c->sock = sock;
            fprintf(INFO_FILE, "Connected to host '%s' port %d.\n", c->addr_text, port);
        }
        else {
            close_socket_gen(sock);
#ifdef _WIN32
            fprintf(ERROR_FILE, "client_connect_to_host: Unable to connect to host '%s' port '%s': %d!\n", c->addr_text, port_string, WSAGetLastError());
#else
            fprintf(ERROR_FILE, "client_connect_to_host: Unable to connect to host '%s' port '%s': %s!\n", c->addr_text, port_string, strerror(errno));
#endif
        }
    }

    freeaddrinfo(result);

    if (c->sock < 0) {
        fprintf(ERROR_FILE, "client_connect_to_host: Unable to connect to '%s' port %d!\n", address, port);
        return false;
    }

    set_socket_blocking(c->sock, c->blocking);
    set_socket_no_delay(c->sock);

    return true;
}

bool client_listen(struct client *c, const int time_out_ms) {
    if (!c || c->sock < 0) {
        fprintf(ERROR_FILE, "client_listen: Invalid client!\n");
        return false;
    }

    fd_set read_fds, write_fds, except_fds;
    
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    FD_SET(c->sock, &read_fds);
    FD_SET(c->sock, &except_fds);
    
    struct timeval tv;

    tv.tv_sec = time_out_ms/1000;
    tv.tv_usec = 1000*(time_out_ms % 1000);
    
    //Check whether any new data is available.
    if (select(c->sock + 1, &read_fds, &write_fds, &except_fds, &tv) == -1) {
        fprintf(ERROR_FILE, "client_listen: Unable to run select()!\n");
        return false;
    }

    if (FD_ISSET(c->sock, &except_fds)) {
        fprintf(ERROR_FILE, "client_listen: Exception at client socket, closing connection!\n");
        free_client(c);
        return false;
    }
    
    if (FD_ISSET(c->sock, &read_fds)) {
        //Previous client data should have been processed.
        if (c->nr_buffer > 0) {
            fprintf(WARN_FILE, "client_listen: Client has non-processed data inside its buffer!\n");
        }

        ssize_t n = recv(c->sock, (char *)(c->buffer + c->nr_buffer), NR_NET_BUFFER - c->nr_buffer, get_block_flags(c->blocking));
        
        if (n <= 0) {
            fprintf(ERROR_FILE, "client_listen: Unable to receive data from host, closing connection!\n");
            free_client(c);
            return false;
        }
        else {
            c->nr_buffer += n;
        }
    }
    
    return true;
}

bool client_send(struct client *c, const void *buffer, size_t len) {
    if (!c || c->sock < 0 || !buffer || len == 0) {
        fprintf(ERROR_FILE, "client_send: Invalid client or buffer!\n");
        return false;
    }
    
    const uint8_t *buf = (const uint8_t *)buffer;
    ssize_t nr_sent = 0;
    
    while (len > 0) {
        if ((nr_sent = send(c->sock, (const char *)buf, len, get_block_flags(c->blocking))) < 0) {
#ifdef _WIN32
            fprintf(ERROR_FILE, "client_send: Unable to send data: %d!\n", WSAGetLastError());
#else
            fprintf(ERROR_FILE, "client_send: Unable to send data: %s!\n", strerror(errno));
#endif
            return false;
        }
        
        buf += nr_sent;
        len -= nr_sent;
    }
    
    return true;
}

bool client_set_blocking(struct client *c, const bool blocking) {
    if (!c || c->sock < 0) {
        fprintf(ERROR_FILE, "client_set_blocking: Invalid client!\n");
        return false;
    }

    c->blocking = blocking;

    if (!set_socket_blocking(c->sock, blocking)) {
        fprintf(ERROR_FILE, "client_set_blocking: Unable to change socket state!\n");
        return false;
    }

    return true;
}

bool free_host(struct host *h) {
    if (!h) {
        fprintf(ERROR_FILE, "free_host: Invalid host!\n");
        return false;
    }

    if (h->clients) {
        for (int i = 0; i < h->max_clients; ++i) {
            free_client(&h->clients[i]);
        }

        free(h->clients);
    }

    if (h->sock >= 0) close_socket_gen(h->sock);

    memset(h, 0, sizeof(struct host));
    h->sock = -1;
    
    return true;
}

bool net_init() {
    fprintf(INFO_FILE, "Initializing networking...\n");

#ifdef _WIN32
    static WSADATA wsa_data;
    const int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (result != 0) {
        fprintf(ERROR_FILE, "net_init: Unable to initialize networking!\n");
        return false;
    }
#endif

    return true;
}

bool net_quit() {
#ifdef _WIN32
    WSACleanup();
#endif

    return true;
}

bool create_host(struct host *h, const int port, const int max_clients) {
    if (!h) {
        fprintf(ERROR_FILE, "create_host: Invalid host!\n");
        return false;
    }

    memset(h, 0, sizeof(struct host));
    h->sock = -1;
    h->blocking = true;
    h->accepts_clients = 1;
    h->port = port;

    h->max_clients = max(1, max_clients);
    h->nr_clients = 0;
    h->clients = calloc(h->max_clients, sizeof(struct client));

    if (!h->clients) {
        fprintf(ERROR_FILE, "create_host: Unable to allocate arrays!\n");
        return false;
    }

    for (int i = 0; i < h->max_clients; i++) {
        if (!create_client(&h->clients[i])) {
            fprintf(ERROR_FILE, "create_host: Unable to initialize clients!\n");
            return false;
        }
    }
    
    char port_string[9] = {0};
    struct addrinfo info, *result = NULL;

    snprintf(port_string, 8, "%d", port);

    memset(&info, 0, sizeof(struct addrinfo));
#ifdef _WIN32
    //FIXME: Hosting ipv6 in Windows does not seem to work, so prefer ipv4.
    info.ai_family = PF_INET;
#else
    info.ai_family = PF_UNSPEC;
#endif
    info.ai_socktype = SOCK_STREAM;
    info.ai_protocol = IPPROTO_TCP;
    info.ai_flags = AI_PASSIVE;
    
    if (getaddrinfo(NULL, port_string, &info, &result) != 0) {
        fprintf(ERROR_FILE, "create_host: Unable to get address for port %d!\n", port);
        return false;
    }

    for (struct addrinfo *r = result; r; r = r->ai_next) {
        //Only support ipv4 and ipv6.
        if ((r->ai_family != PF_INET) && (r->ai_family != PF_INET6)) continue;

        int sock = socket(r->ai_family, r->ai_socktype, r->ai_protocol);

#ifdef _WIN32
        if (sock == (int)INVALID_SOCKET) {
            fprintf(ERROR_FILE, "create_host: Unable to create socket: %d!\n", WSAGetLastError());
#else
        if (sock == -1) {
            fprintf(ERROR_FILE, "create_host: Unable to create socket: %s!\n", strerror(errno));
#endif
            continue;
        }

        if (!set_socket_blocking(sock, h->blocking)) {
            fprintf(ERROR_FILE, "create_host: Unable to set socket blocking!\n");
            close_socket_gen(sock);
            continue;
        }
        
#ifndef _WIN32
        if (true) {
            //Disable SO_REUSEADDR per SDL net example.
            int yes = 1;

            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
                fprintf(ERROR_FILE, "create_host: Unable to set socket to reusable!\n");
                close_socket_gen(sock);
                continue;
            }
        }
#endif
        
        //No socket delay.
        set_socket_no_delay(sock);

        //Get host address name.
        get_addr_string((const struct sockaddr *)r->ai_addr, h->addr_text, INET6_ADDRSTRLEN);
        
        if (bind(sock, r->ai_addr, r->ai_addrlen) != 0) {
#ifdef _WIN32
            fprintf(ERROR_FILE, "create_host: Unable to bind socket: %d!\n", WSAGetLastError());
#else
            fprintf(ERROR_FILE, "create_host: Unable to bind socket: %s!\n", strerror(errno));
#endif
            close_socket_gen(sock);
            continue;
        }

        if (listen(sock, 16) != 0) {
#ifdef _WIN32
            fprintf(ERROR_FILE, "create_host: Unable to listen at socket: %d!\n", WSAGetLastError());
#else
            fprintf(ERROR_FILE, "create_host: Unable to listen at socket: %s!\n", strerror(errno));
#endif
            close_socket_gen(sock);
            continue;
        }
        
        //We successfully listen at a socket.
        h->sock = sock;
        break;
    }

    freeaddrinfo(result);

    if (h->sock < 0) {
        fprintf(ERROR_FILE, "create_host: Unable to listen at socket for incoming connections!\n");
        free_host(h);
        return false;
    }

    fprintf(INFO_FILE, "Host is listening at address %s network port %d...\n", h->addr_text, port);

    return true;
}

bool host_send(struct host *h, struct client *c, const void *buffer, size_t len) {
    if (!h || h->sock < 0 || !c || c->sock < 0 || !buffer || len == 0) {
        fprintf(ERROR_FILE, "host_send: Invalid host or client or buffer!\n");
        return false;
    }
    
    const uint8_t *buf = (const uint8_t *)buffer;
    ssize_t nr_sent = 0;
    
    while (len > 0) {
        if ((nr_sent = send(c->sock, (const char *)buf, len, get_block_flags(h->blocking))) < 0) {
#ifdef _WIN32
            fprintf(ERROR_FILE, "host_send: Unable to send data: %d!\n", WSAGetLastError());
#else
            fprintf(ERROR_FILE, "host_send: Unable to send data: %s!\n", strerror(errno));
#endif
            return false;
        }
        
        buf += nr_sent;
        len -= nr_sent;
    }

    return true;
}
    
bool host_broadcast(struct host *h, const void *buffer, const size_t len) {
    if (!h || h->sock < 0 || !h->clients || !buffer || len == 0) {
        fprintf(ERROR_FILE, "host_broadcast: Invalid host or buffer!\n");
        return false;
    }

    for (int i = 0; i < h->max_clients; i++) {
        if (h->clients[i].sock >= 0) {
            if (!host_send(h, &h->clients[i], buffer, len)) {
                fprintf(WARN_FILE, "host_broadcast: Unable to send to client %d/%d!\n", i, h->max_clients);
            }
        }
    }
    
    return true;
}

bool host_listen(struct host *h, const int time_out_ms) {
    if (!h || h->sock < 0 || !h->clients) {
        fprintf(ERROR_FILE, "host_listen: Invalid or non-listening host!\n");
        return false;
    }
    
    fd_set read_fds, write_fds, except_fds;
    int max_fd = h->sock;
    
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    FD_SET(h->sock, &read_fds);
    FD_SET(h->sock, &except_fds);

    for (int i = 0; i < h->max_clients; ++i) {
        if (h->clients[i].sock >= 0) {
            max_fd = max(max_fd, h->clients[i].sock);
            FD_SET(h->clients[i].sock, &read_fds);
            FD_SET(h->clients[i].sock, &write_fds);
        }
    }

    struct timeval tv;

    tv.tv_sec = time_out_ms/1000;
    tv.tv_usec = 1000*(time_out_ms % 1000);
    
    //Check whether any new data is available.
    if (select(max_fd + 1, &read_fds, &write_fds, &except_fds, &tv) == -1) {
        fprintf(ERROR_FILE, "host_listen: Unable to run select()!\n");
        return false;
    }

    if (FD_ISSET(h->sock, &except_fds)) {
        fprintf(ERROR_FILE, "host_listen: Exception at host socket!\n");
        return false;
    }
    
    //Any new connections?
    if (FD_ISSET(h->sock, &read_fds)) {
        if (!h->accepts_clients || h->nr_clients >= h->max_clients) {
            fprintf(INFO_FILE, "host_listen: Unable to accept connection as we are at the maximum number of clients %d/%d or not accepting new clients!\n", h->nr_clients, h->max_clients);
        }
        else {
            int i_client;
            
            for (i_client = 0; i_client < h->max_clients; i_client++) {
                if (h->clients[i_client].sock < 0) break;
            }

            if (i_client >= h->max_clients) {
                fprintf(ERROR_FILE, "host_listen: This should never happen!\n");
                return false;
            }

            memset(&h->clients[i_client], 0, sizeof(struct client));
            
            struct sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            
            memset(&addr, 0, sizeof(addr));
            h->clients[i_client].sock = accept(h->sock, (struct sockaddr *)&addr, &addr_len);
            get_addr_string((const struct sockaddr *)&addr, h->clients[i_client].addr_text, INET6_ADDRSTRLEN);
            h->clients[i_client].port = 0; //TODO: addr.sin6_port;
            h->clients[i_client].newly_joined = 0;

            if (h->clients[i_client].sock == -1) {
                fprintf(ERROR_FILE, "host_listen: Unable to accept connection from '%s' port %d!\n", h->clients[i_client].addr_text, h->clients[i_client].port);
            }
            else {
                set_socket_blocking(h->clients[i_client].sock, h->blocking);
                set_socket_no_delay(h->clients[i_client].sock);
                h->clients[i_client].newly_joined = 1;
                h->nr_clients++;
                fprintf(INFO_FILE, "Accepted connection from '%s' port %d, now at %d/%d clients.\n", h->clients[i_client].addr_text, h->clients[i_client].port, h->nr_clients, h->max_clients);
            }
        }
    }

    //Any data from the clients?
    for (int i = 0; i < h->max_clients; ++i) {
        if (h->clients[i].sock >= 0) {
            if (FD_ISSET(h->clients[i].sock, &except_fds)) {
                //Client dropped or disconnected.
                host_remove_client(h, i);
                fprintf(INFO_FILE, "Connection closed from '%s' port %d, now at %d/%d clients.\n", h->clients[i].addr_text, h->clients[i].port, h->nr_clients, h->max_clients);
            }
            else if (FD_ISSET(h->clients[i].sock, &read_fds)) {
                //Client has data.

                //Previous client data should have been processed.
                if (h->clients[i].nr_buffer > 0) {
                    fprintf(WARN_FILE, "host_listen: Client has non-processed data inside its buffer!\n");
                }

                ssize_t n = recv(h->clients[i].sock, (char *)(h->clients[i].buffer + h->clients[i].nr_buffer), NR_NET_BUFFER - h->clients[i].nr_buffer, get_block_flags(h->blocking));
                
                if (n <= 0) {
                    host_remove_client(h, i);
                    fprintf(ERROR_FILE, "host_listen: Unable to receive data from client!\n");
                    fprintf(INFO_FILE, "Connection closed from '%s' port %d, now at %d/%d clients.\n", h->clients[i].addr_text, h->clients[i].port, h->nr_clients, h->max_clients);
                }
                else {
                    h->clients[i].nr_buffer += n;
                }
            }
        }
    }

    return true;
}

bool host_remove_client(struct host *h, const int i) {
    if (!h || i < 0 || i >= h->max_clients) {
        fprintf(ERROR_FILE, "host_remove_client: Invalid host or client index!\n");
        return false;
    }
    
    if (h->clients[i].sock >= 0) {
        h->nr_clients--;
        close_socket_gen(h->clients[i].sock);
    }
    else {
        fprintf(WARN_FILE, "host_remove_client: Removing an already removed client!\n");
    }

    h->clients[i].sock = -1;
    h->clients[i].newly_joined = 0;

    fprintf(INFO_FILE, "Removed client %s port %d.\n", h->clients[i].addr_text, h->clients[i].port);
    
    return true;
}

bool host_set_blocking(struct host *h, const bool blocking) {
    if (!h || h->sock < 0) {
        fprintf(ERROR_FILE, "host_set_blocking: Invalid host!\n");
        return false;
    }

    h->blocking = blocking;

    if (!set_socket_blocking(h->sock, blocking)) {
        fprintf(ERROR_FILE, "host_set_blocking: Unable to change socket state!\n");
        return false;
    }

    for (int i = 0; i < h->max_clients; ++i) {
        if (h->clients[i].sock >= 0) {
            if (!set_socket_blocking(h->clients[i].sock, blocking)) {
                fprintf(WARN_FILE, "host_set_blocking: Unable to change client socket state!\n");
            }
        }
    }

    return true;
}
