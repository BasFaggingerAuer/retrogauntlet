/*
Copyright 2023 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//This is a replacement of the sockets networking implementation in net.c using the ISteamNetworkingSockets interface, see https://partner.steamgames.com/doc/api/ISteamNetworkingSockets.

#include <string>
#include <memory>

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/isteamgameserver.h>
#include <steam/steam_gameserver.h>
#include <steam/isteamuser.h>
#include <steam/isteamfriends.h>
#include <steam/steam_api_common.h>
#include <steam/steam_api.h>

#include <cstdio>
#include <unistd.h>

extern "C" {
#include "net.h"
#include "retrogauntlet.h"
}

//The number of messages we retrieve at most when listening as host.
#define NR_CLIENT_STEAM_MESSAGES 32
#define NR_HOST_STEAM_MESSAGES 128

//The buffer size of a network client.
#define NR_NET_BUFFER 65536

struct client {
    CSteamID id;
    int port;
    HSteamNetConnection sock;
    uint8_t buffer[NR_NET_BUFFER + 1];
    size_t nr_buffer;
    bool newly_joined;
    bool blocking;
    bool active;
    bool callbacks;

    //STEAM_CALLBACK(client, OnLobbyGameCreated, LobbyGameCreated_t);
    STEAM_CALLBACK(client, OnGameJoinRequested, GameRichPresenceJoinRequested_t);
    //STEAM_CALLBACK(client, OnNewUrlLaunchParameters, NewUrlLaunchParameters_t);
    STEAM_CALLBACK(client, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);
    STEAM_CALLBACK(client, OnIPCFailure, IPCFailure_t);
    STEAM_CALLBACK(client, OnSteamShutdown, SteamShutdown_t);
};

struct host {
    int port;
    HSteamListenSocket sock;
    HSteamNetPollGroup poll_group;
    int max_clients;
    struct client *clients;
    int nr_clients;
    bool accepts_clients;
    bool blocking;
    bool active;

    STEAM_CALLBACK(host, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);
    STEAM_CALLBACK(host, OnIPCFailure, IPCFailure_t);
    STEAM_CALLBACK(host, OnSteamShutdown, SteamShutdown_t);
};

extern "C" bool __cdecl net_init() {
    SteamNetworkingUtils()->InitRelayNetworkAccess();

    //Wait until we have connected properly.
    ESteamNetworkingAvailability status;

    do {
        usleep(100000);
        status = SteamNetworkingUtils()->GetRelayNetworkStatus(nullptr);
    } while (status > 0 && status != k_ESteamNetworkingAvailability_Current);

    return (status == k_ESteamNetworkingAvailability_Current);
}

extern "C" bool __cdecl net_quit() {
    return true;
}

bool reset_client(void *_c) {
    if (!_c) {
        fprintf(ERROR_FILE, "reset_client: Invalid client!\n");
        return false;
    }

    struct client *c = (struct client *)_c;

    if (c->active && c->sock != k_HSteamNetConnection_Invalid) {
        SteamGameServerNetworkingSockets()->CloseConnection(c->sock, 0, nullptr, false);
        SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
    }

    //Destroy client object to get rid of steam callbacks.
    if (c->active && c->callbacks) {
        std::allocator<client> alloc;

        alloc.destroy(c);
    }
    
    memset(c, 0, sizeof(struct client));
    c->id = CSteamID();
    c->sock = k_HSteamNetConnection_Invalid;
    c->blocking = true;
    c->active = false;
    c->callbacks = false;
    c->nr_buffer = 0;

    return true;
}

extern "C" bool __cdecl allocate_clients(void **_c, const size_t nr_clients) {
    if (!_c) {
        fprintf(ERROR_FILE, "allocate_clients: Invalid memory address!\n");
        return false;
    }

    *_c = calloc(nr_clients, sizeof(struct client));

    if (!(*_c)) {
        fprintf(ERROR_FILE, "allocate_clients: Unable to allocate memory!\n");
        return false;
    }

    struct client *c = (struct client *)(*_c);
    
    for (size_t i = 0; i < nr_clients; ++i) {
        reset_client(&c[i]);
    }
    
    return true;
}

extern "C" bool __cdecl free_clients(void **_c, const size_t nr_clients) {
    if (!_c) {
        fprintf(ERROR_FILE, "free_clients: Invalid client!\n");
        return false;
    }

    struct client *c = (struct client *)(*_c);
    
    for (size_t i = 0; i < nr_clients; ++i) {
        reset_client(&c[i]);
    }

    free(*_c);
    *_c = NULL;
    
    return true;
}

extern "C" bool __cdecl client_is_client_active(void *_c) {
    struct client *c = (struct client *)_c;

    if (!_c) {
        //fprintf(WARN_FILE, "client_is_client_active: Invalid client!\n");
        return false;
    }
    
    return c->active;
}

extern "C" bool __cdecl client_is_client_new(void *_c) {
    struct client *c = (struct client *)_c;

    if (!_c) {
        fprintf(WARN_FILE, "client_is_client_new: Invalid client!\n");
        return false;
    }
    
    const bool n = c->newly_joined;

    //After checking the client is no longer new.
    c->newly_joined = false;

    return (c->active && n);
}

extern "C" bool __cdecl client_connect_to_host(void *_c, const char *address, const int) {
    if (!_c || !address) {
        fprintf(ERROR_FILE, "client_connect_to_host: Invalid client or address!\n");
        return false;
    }

    struct client *c = (struct client *)_c;
    
    reset_client(c);

    //FIXME: Use address as Steam friend SteamID for now to connect (must be a better way to do this...).
    CSteamID host_id(std::stoull(std::string(address)));

    if (!host_id.IsValid()) {
        fprintf(ERROR_FILE, "client_connect_to_host: Invalid address %s, should be a valid SteamID!\n", address);
        return false;
    }

    //Register object for steam callbacks.
    std::allocator<client> alloc;

    alloc.construct(c);
    c->callbacks = true;
    
    SteamNetworkingIdentity identity;

    memset(&identity, 0, sizeof(SteamNetworkingIdentity));
    identity.m_eType = k_ESteamNetworkingIdentityType_SteamID;
    identity.SetSteamID(host_id);

    /*
    //Do we need this?
    //https://stackoverflow.com/questions/75622790/steamworks-sdk-isteamnetworkingsockets-connectp2p
    bool found = false;

    const int nr_friends = SteamFriends()->GetFriendCount(k_EFriendFlagFriendshipRequested | k_EFriendFlagRequestingFriendship);

    for (int i = 0; i < nr_friends; ++i) {
        CSteamID id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagFriendshipRequested | k_EFriendFlagRequestingFriendship);

        if (strcmp(address, SteamFriends()->GetFriendPersonaName(id))) {
            identity.SetSteamID(id);
            found = true;
            break;
        }
    }

    if (!found) {
        fprintf(ERROR_FILE, "client_connect_to_host: Unable to find Steam friend %s!\n", address);
        return false;
    }
    */

    c->sock = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);

    if (c->sock == k_HSteamNetConnection_Invalid) {
        fprintf(ERROR_FILE, "client_connect_to_host: Unable to connect P2P to %s!\n", address);
        reset_client(c);
        return false;
    }

    SteamUser()->AdvertiseGame(identity.GetSteamID(), 0, 0);

    c->active = true;
    c->newly_joined = true;

    fprintf(INFO_FILE, "client_connect_to_host: Connected to %s (%llu).\n", address, identity.GetSteamID().ConvertToUint64());
    return true;
}

void client::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *callback) {
    if ((callback->m_eOldState == k_ESteamNetworkingConnectionState_Connecting ||
         callback->m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
        callback->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer) {
        fprintf(ERROR_FILE, "client::OnNetConnectionStatusChanged: Host rejected our connection attempt!\n");
        reset_client(this);
        return;
    }

    if ((callback->m_eOldState == k_ESteamNetworkingConnectionState_Connecting ||
         callback->m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
        callback->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
        fprintf(ERROR_FILE, "client::OnNetConnectionStatusChanged: Lost connection to host!\n");
        reset_client(this);
        return;
    }

    if (callback->m_eOldState == k_ESteamNetworkingConnectionState_Connecting &&
        callback->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected) {
        fprintf(INFO_FILE, "client::OnNetConnectionStatusChanged: Connection was accepted by host.\n");
        return;
    }
}

void client::OnGameJoinRequested(GameRichPresenceJoinRequested_t *callback) {
    //Act on game join request via Steam.
    fprintf(INFO_FILE, "client::OnGameJoinRequested: Requesting to join game from %s.\n", SteamFriends()->GetFriendPersonaName(callback->m_steamIDFriend));

    //FIXME: Should not tie this to a client()-instance --> need to separate out higher-level callbacks.
    client_connect_to_host(this, std::to_string(callback->m_steamIDFriend.ConvertToUint64()).c_str(), 0);
}

void client::OnIPCFailure(IPCFailure_t *callback) {
    fprintf(ERROR_FILE, "client::OnIPCFailure: Steam IPC failure!\n");
    reset_client(this);
}

void client::OnSteamShutdown(SteamShutdown_t *callback) {
    fprintf(ERROR_FILE, "client::OnSteamShutdown: Steam is shutting down!\n");
    reset_client(this);
}

extern "C" bool __cdecl client_listen(void *_c, const int) {
    struct client *c = (struct client *)_c;

    if (!_c || !c->active) {
        fprintf(ERROR_FILE, "client_listen: Invalid client!\n");
        return false;
    }

    //Run callbacks.
    SteamNetworkingSockets()->RunCallbacks();

    //Any data from the host?
    SteamNetworkingMessage_t *messages[NR_CLIENT_STEAM_MESSAGES];
	const int nr_messages = SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(c->sock, messages, NR_CLIENT_STEAM_MESSAGES);
	
    for (int i_msg = 0; i_msg < nr_messages; ++i_msg) {
        SteamNetworkingMessage_t *message = messages[i_msg];
        
        //Append data to client buffer.
        if (message->GetSize() + c->nr_buffer <= NR_NET_BUFFER) {
            memcpy(c->buffer + c->nr_buffer, message->GetData(), message->GetSize());
            c->nr_buffer += message->GetSize();
        }
        else {
            fprintf(ERROR_FILE, "client_listen: Unable to receive message of size %u from host!\n", message->GetSize());
            return false;
        }

        message->Release();
    }

    return true;
}

extern "C" bool __cdecl client_send(void *_c, const void *buffer, size_t len) {
    struct client *c = (struct client *)_c;

    if (!_c || !c->active || !buffer || len == 0) {
        fprintf(ERROR_FILE, "client_send: Invalid client or buffer!\n");
        return false;
    }

    return (SteamNetworkingSockets()->SendMessageToConnection(c->sock, buffer, len, k_nSteamNetworkingSend_Reliable, nullptr) == k_EResultOK);
}

extern "C" bool __cdecl client_set_blocking(void *, const bool); //TODO

extern "C" size_t __cdecl client_get_nr_data(void *_c) {
    struct client *c = (struct client *)_c;

    if (!_c || !c->active) {
        fprintf(ERROR_FILE, "client_get_nr_data: Invalid client!\n");
        return 0;
    }

    return c->nr_buffer;
}

extern "C" size_t __cdecl client_get_data(void *dest, void *_c, size_t nr) {
    struct client *c = (struct client *)_c;

    if (!dest || !c || !c->active) {
        fprintf(ERROR_FILE, "client_get_data: Invalid destination or client!\n");
        return 0;
    }

    //Do not retrieve more data than available.
    nr = min(nr, c->nr_buffer);

    if (nr == 0) {
        return 0;
    }

    //Copy data to external buffer.
    memcpy(dest, c->buffer, nr);

    //Decrease size of client buffer and move remaining data to start of buffer.
    c->nr_buffer -= nr;

    if (c->nr_buffer > 0) {
        memmove(c->buffer, c->buffer + nr, c->nr_buffer);
        memset(c->buffer + c->nr_buffer, 0, nr);
    }

    return nr;
}

extern "C" void __cdecl client_fprintf(FILE *file, void *_c) {
    struct client *c = (struct client *)_c;

    if (!_c || !file) return;

    fprintf(file, "%s:%d", SteamFriends()->GetFriendPersonaName(c->id), c->port);
}

extern "C" int __cdecl client_sprintf(char *str, void *_c) {
    struct client *c = (struct client *)_c;

    if (!_c || !str) return 0;

    return sprintf(str, "%s:%d", SteamFriends()->GetFriendPersonaName(c->id), c->port);
}

extern "C" bool __cdecl free_host(void **_h) {
    if (!_h || !(*_h)) {
        fprintf(ERROR_FILE, "free_host: Invalid host!\n");
        return false;
    }
    
    struct host *h = (struct host *)(*_h);

    if (h->active) {
        SteamGameServerNetworkingSockets()->CloseListenSocket(h->sock);
        SteamGameServerNetworkingSockets()->DestroyPollGroup(h->poll_group);

        if (SteamGameServer()) {
            SteamGameServer()->LogOff();
            SteamGameServer_Shutdown();
        }
    }

    //De-register object for steam callbacks.
    std::allocator<host> alloc;

    alloc.destroy(h);

    free(*_h);
    *_h = NULL;

    return true;
}

extern "C" bool __cdecl allocate_host(void **_h, const int port, const int max_clients) {
    //Allocate host memory.
    *_h = calloc(1, sizeof(struct host));
    
    if (!(*_h)) {
        fprintf(ERROR_FILE, "allocate_host: Unable to allocate memory!\n");
        return false;
    }

    struct host *h = (struct host *)(*_h);

    memset(h, 0, sizeof(struct host));

    //Register object for steam callbacks.
    //FIXME: This is rather horrendous, better to move this to a proper constructor.
    std::allocator<host> alloc;

    alloc.construct(h);
    
    h->blocking = true;
    h->accepts_clients = true;
    h->port = port;
    h->active = false;
    
    h->max_clients = max(1, max_clients);
    h->nr_clients = 0;
    h->sock = k_HSteamNetConnection_Invalid;

    if (!allocate_clients((void **)&h->clients, h->max_clients)) {
        fprintf(ERROR_FILE, "allocate_host: Unable to allocate clients!");
        return false;
    }
    
    //Authenticate clients using steam and VAC.
    //FIXME: Better control of steam port, game port, and query port.
    //FIXME: INADDR_ANY == 0?
    if (!SteamGameServer_Init(0, port, port + 1, eServerModeAuthenticationAndSecure, RETRO_GAUNTLET_VERSION)) {
        fprintf(ERROR_FILE, "allocate_host: Unable to initialize steam game server!\n");
        return false;
    }

    if (!SteamGameServer()) {
        fprintf(ERROR_FILE, "allocate_host: Unable to access steam game server!\n");
        return false;
    }

    SteamGameServer()->SetModDir("retrogauntlet"); //FIXME
    SteamGameServer()->SetProduct("RetroGauntlet");
    SteamGameServer()->SetGameDescription("Race libretro games online.");
    SteamGameServer()->SetDedicatedServer(false);
    SteamGameServer()->SetMaxPlayerCount(h->max_clients);
    SteamGameServer()->SetPasswordProtected(false);
	SteamGameServer()->SetServerName((std::string(SteamFriends()->GetPersonaName()) + std::string("'s game")).c_str());
	SteamGameServer()->SetBotPlayerCount(0);
	//TODO (maybe?): SteamGameServer()->SetMapName("current game");
    //TODO (maybe?): SteamGameServer()->BUpdateUserData(id, "name", score);
    //TODO: SteamGameServer()->SetSpectatorPort( ... );
    //TODO: SteamGameServer()->SetSpectatorServerName( ... );
    SteamGameServer()->LogOnAnonymous();
    //Make sure we are findable within steam.
    SteamGameServer()->SetAdvertiseServerActive(true);

    //Start listening for incoming connections.
    h->sock = SteamGameServerNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
    h->poll_group = SteamGameServerNetworkingSockets()->CreatePollGroup();
    
    if (h->sock == k_HSteamNetConnection_Invalid) {
        fprintf(ERROR_FILE, "allocate_host: Unable to open listening socket!\n");
        return false;
    }

    h->active = true;

    fprintf(INFO_FILE, "Host is listening at network port %d...\n", port);
    
    return true;
}

void host::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *callback) {
    //Any new connecting clients?
    if (callback->m_info.m_hListenSocket &&
        callback->m_eOldState == k_ESteamNetworkingConnectionState_None &&
        callback->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting) {
        
        if (!this->accepts_clients || this->nr_clients >= this->max_clients) {
            fprintf(INFO_FILE, "host::OnNetConnectionStatusChanged: Unable to accept connection as we are at the maximum number of clients %d/%d or not accepting new clients!\n", this->nr_clients, this->max_clients);
            SteamGameServerNetworkingSockets()->CloseConnection(callback->m_hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Server full or not accepting new clients!", false);
        }
        else {
            EResult result = SteamGameServerNetworkingSockets()->AcceptConnection(callback->m_hConn);

            if (result != k_EResultOK) {
                fprintf(INFO_FILE, "host::OnNetConnectionStatusChanged: Unable to accept incoming connection: %d!\n", result);
                SteamGameServerNetworkingSockets()->CloseConnection(callback->m_hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Unable to accept connection!", false);
            }
            else {
                SteamGameServerNetworkingSockets()->SetConnectionPollGroup(callback->m_hConn, this->poll_group);

                int i_client;
                
                for (i_client = 0; i_client < this->max_clients; i_client++) {
                    if (!this->clients[i_client].active) break;
                }

                if (i_client >= this->max_clients) {
                    fprintf(ERROR_FILE, "host::OnNetConnectionStatusChanged: This should never happen!\n");
                    return;
                }

                memset(&this->clients[i_client], 0, sizeof(struct client));

                this->clients[i_client].id = callback->m_info.m_identityRemote.GetSteamID();
                this->clients[i_client].sock = callback->m_hConn;
                this->clients[i_client].newly_joined = true;
                this->clients[i_client].callbacks = false;
                this->clients[i_client].active = true;

                fprintf(INFO_FILE, "Accepted connection from %s (%llu), now at %d/%d clients.\n", SteamFriends()->GetFriendPersonaName(this->clients[i_client].id), this->clients[i_client].id.ConvertToUint64(), this->nr_clients, this->max_clients);
            }
        }
    }

    //Any disconnected clients?
    if ((callback->m_eOldState == k_ESteamNetworkingConnectionState_Connecting ||
         callback->m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
        (callback->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer ||
         callback->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)) {
        int i_client;
        
        for (i_client = 0; i_client < this->max_clients; i_client++) {
            if (this->clients[i_client].active && this->clients[i_client].id == callback->m_info.m_identityRemote.GetSteamID()) break;
        }

        if (i_client >= this->max_clients) {
            fprintf(WARN_FILE, "host::OnNetConnectionStatusChanged: Received disconnect from unconnected client!\n");
            return;
        }
        else {
            fprintf(INFO_FILE, "Client %s (%llu) disconnected, now at %d/%d clients.\n", SteamFriends()->GetFriendPersonaName(this->clients[i_client].id), this->clients[i_client].id.ConvertToUint64(), this->nr_clients - 1, this->max_clients);
            host_remove_client(this, i_client);
        }
    }
}

void host::OnIPCFailure(IPCFailure_t *callback) {
    fprintf(ERROR_FILE, "host::OnIPCFailure: Steam IPC failure!\n");
    this->active = false;
}

void host::OnSteamShutdown(SteamShutdown_t *callback) {
    fprintf(ERROR_FILE, "host::OnSteamShutdown: Steam is shutting down!\n");
    this->active = false;
}

extern "C" bool __cdecl host_listen(void *_h, const int) {
    struct host *h = (struct host *)_h;

    if (!_h || !h->active) {
        fprintf(ERROR_FILE, "host_listen: Invalid host!\n");
        return false;
    }

    //Any new or dropped connections are handled by STEAM_GAMESERVER_CALLBACK().
    SteamGameServer_RunCallbacks();

    //Any data from clients?
    SteamNetworkingMessage_t *messages[NR_HOST_STEAM_MESSAGES];
	const int nr_messages = SteamGameServerNetworkingSockets()->ReceiveMessagesOnPollGroup(h->poll_group, messages, NR_HOST_STEAM_MESSAGES);
	
    for (int i_msg = 0; i_msg < nr_messages; ++i_msg) {
        SteamNetworkingMessage_t *message = messages[i_msg];
        const CSteamID client_id = message->m_identityPeer.GetSteamID();
        
        //Append data to client buffer.
        for (int i_cli = 0; i_cli < h->max_clients; ++i_cli) {
            if (h->clients[i_cli].active && h->clients[i_cli].id == client_id) {
                if (message->GetSize() + h->clients[i_cli].nr_buffer <= NR_NET_BUFFER) {
                    memcpy(h->clients[i_cli].buffer + h->clients[i_cli].nr_buffer, message->GetData(), message->GetSize());
                    h->clients[i_cli].nr_buffer += message->GetSize();
                }
                else {
                    fprintf(ERROR_FILE, "host_listen: Unable to receive message of size %u from client %d!\n", message->GetSize(), i_cli);
                }
            }
        }

        message->Release();
    }

    return true;
}

extern "C" bool __cdecl host_send(void *_h, void *_c, const void *buffer, size_t len) {
    struct host *h = (struct host *)_h;
    struct client *c = (struct client *)_c;
    
    if (!_h || !h->active || !_c || !c->active || !buffer || len == 0) {
        fprintf(ERROR_FILE, "host_send: Invalid host or client or buffer!\n");
        return false;
    }

    return (SteamNetworkingSockets()->SendMessageToConnection(c->sock, buffer, len, k_nSteamNetworkingSend_Reliable, nullptr) == k_EResultOK);
}

extern "C" bool __cdecl host_broadcast(void *_h, const void *buffer, const size_t len) {
    struct host *h = (struct host *)_h;
    
    if (!_h || !h->active || !buffer || len == 0) {
        fprintf(ERROR_FILE, "host_broadcast: Invalid host or buffer!\n");
        return false;
    }

    for (int i = 0; i < h->max_clients; i++) {
        if (h->clients[i].active) {
            SteamNetworkingSockets()->SendMessageToConnection(h->clients[i].sock, buffer, len, k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }

    return true;
}

extern "C" bool __cdecl host_remove_client(void *_h, const int i) {
    struct host *h = (struct host *)_h;

    if (!_h || i < 0 || i >= h->max_clients) {
        fprintf(ERROR_FILE, "host_remove_client: Invalid host or client index!\n");
        return false;
    }
    
    if (h->clients[i].active) {
        h->nr_clients--;
        fprintf(INFO_FILE, "Removed client %s port %d.\n", SteamFriends()->GetFriendPersonaName(h->clients[i].id), h->clients[i].port);
        
        SteamGameServerNetworkingSockets()->CloseConnection(h->clients[i].sock, 0, nullptr, false);
    }
    else {
        fprintf(WARN_FILE, "host_remove_client: Removing an already removed client!\n");
    }

    h->clients[i].active = false;
    h->clients[i].newly_joined = false;
    
    return true;
}

extern "C" bool __cdecl host_is_host_active(void *_h) {
    if (!_h) {
        //fprintf(WARN_FILE, "host_is_host_active: Invalid host!\n");
        return false;
    }

    struct host *h = (struct host *)_h;
    
    return h->active;
}

extern "C" bool __cdecl host_is_client_active(void *_h, const int i) {
    struct host *h = (struct host *)_h;

    if (!_h || i < 0 || i >= h->max_clients) {
        fprintf(WARN_FILE, "host_is_client_active: Invalid host or client index!\n");
        return false;
    }
    
    return client_is_client_active((void *)&h->clients[i]);
}

extern "C" bool __cdecl host_is_client_new(void *_h, const int i) {
    struct host *h = (struct host *)_h;

    if (!_h || i < 0 || i >= h->max_clients) {
        fprintf(WARN_FILE, "host_is_client_new: Invalid host or client index!\n");
        return false;
    }

    return client_is_client_new((void *)&h->clients[i]);
}

extern "C" bool __cdecl host_set_blocking(void *, const bool); //TODO

extern "C" bool __cdecl host_set_accepts_clients(void *_h, const bool accepts) {
    struct host *h = (struct host *)_h;

    if (!_h || !h->active) {
        fprintf(ERROR_FILE, "host_set_accepts_clients: Invalid host!\n");
        return false;
    }

    h->accepts_clients = accepts;
    return true;
}

extern "C" int __cdecl host_get_active_client_index(void *_h, int i) {
    struct host *h = (struct host *)_h;

    if (!_h || !h->active) {
        fprintf(ERROR_FILE, "host_get_nr_clients: Invalid host!\n");
        return -1;
    }

    if (i < 0 || i >= h->max_clients) {
        return -1;
    }

    for ( ; i < h->max_clients; ++i) {
        if (client_is_client_active((void *)&h->clients[i])) {
            return i;
        }
    }

    return -1;
}

extern "C" void __cdecl *host_get_client(void *_h, int i) {
    struct host *h = (struct host *)_h;

    if (!_h || !h->active || i < 0 || i >= h->max_clients) {
        fprintf(ERROR_FILE, "host_get_client: Invalid host or client index!\n");
        return NULL;
    }

    return (void *)&h->clients[i];
}

extern "C" void __cdecl host_fprintf(FILE *file, void *_h) {
    struct host *h = (struct host *)_h;

    if (!_h || !file) return;

    fprintf(file, "%s:%d (%d/%d)", SteamFriends()->GetPersonaName(), h->port, h->nr_clients, h->max_clients);
}

extern "C" int __cdecl host_sprintf(char *str, void *_h) {
    struct host *h = (struct host *)_h;

    if (!_h || !str) return 0;

    return sprintf(str, "%s:%d (%d/%d)", SteamFriends()->GetPersonaName(), h->port, h->nr_clients, h->max_clients);
}
