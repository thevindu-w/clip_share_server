/*
 *  clip_share.c - server which communicates with client app to share data
 *  Copyright (C) 2022 H. Thevindu J. Wijesekera
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/utils.h"
#include "utils/net_utils.h"
#include "proto/server.h"
#include "servers.h"
#include "config.h"

#include <stdlib.h>
#ifdef __linux__
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#elif _WIN32
#include <io.h>
#include <windows.h>
#endif

#ifdef _WIN32
static DWORD WINAPI serverThreadFn(void *arg)
{
    socket_t socket;
    memcpy(&socket, arg, sizeof(socket_t));
    free(arg);
    server(socket);
    close_socket(&socket);
    return 0;
}
#endif

int clip_share(const int port, const int secure, config cfg)
{
    listener_t listener = open_listener_socket(secure, cfg.priv_key, cfg.server_cert, cfg.ca_cert);
    if (bind_port(listener, port) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    if (listen(listener.socket, 3) == -1)
    {
        error("Can\'t listen");
        return EXIT_FAILURE;
    }
#ifdef __linux__
    signal(SIGCHLD, SIG_IGN);
#endif
    while (1)
    {
        socket_t connect_sock = get_connection(listener, cfg.allowed_clients);
        if (connect_sock.type == NULL_SOCK){
            close_socket(&connect_sock);
            continue;
        }
#ifdef __linux__
#ifndef DEBUG_MODE
        pid_t p1 = fork();
        if (p1)
        {
            close_socket(&connect_sock);
        }
        else
        {
            close(listener.socket);
#endif
            server(connect_sock);
            close_socket(&connect_sock);
#ifndef DEBUG_MODE
            break;
        }
#endif
#elif _WIN32
        socket_t *connect_ptr = malloc(sizeof(socket_t));
        memcpy(connect_ptr, &connect_sock, sizeof(socket_t));
        HANDLE serveThread = CreateThread(NULL, 0, serverThreadFn, (LPDWORD)connect_ptr, 0, NULL);
#ifdef DEBUG_MODE
        if (serveThread == NULL)
        {
            error("Thread creation failed");
            return EXIT_FAILURE;
        }
#else
        (void)serveThread;
#endif
#endif
    }
    return 0;
}
