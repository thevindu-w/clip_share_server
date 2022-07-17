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
    SOCKET socket = (SOCKET)arg;
    server(socket);
    close(socket);
    return 0;
}
#endif

int clip_share(const int port)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        error("failed WSAStartup");
        return EXIT_FAILURE;
    }
#endif
    int listener_d = open_listener_socket();
    bind_port(listener_d, port);
    if (listen(listener_d, 3) == -1)
        error("Can\'t listen");
#ifdef __linux__
    signal(SIGCHLD, SIG_IGN);
#endif
    while (1)
    {
        int connect_d = get_connection(listener_d);
#ifdef __linux__
#ifndef DEBUG_MODE
        pid_t p1 = fork();
        if (p1)
        {
            close(connect_d);
        }
        else
        {
            close(listener_d);
#endif
            server(connect_d);
            close(connect_d);
#ifndef DEBUG_MODE
            break;
        }
#endif
#elif _WIN32
        HANDLE serveThread = CreateThread(NULL, 0, serverThreadFn, (LPDWORD)connect_d, 0, NULL);
#ifdef DEBUG_MODE
        if (serveThread == NULL)
        {
            error("Thread creation failed");
        }
#else
        (void)serveThread;
#endif
#endif
    }
    return 0;
}
