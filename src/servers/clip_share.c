/*
 * servers/clip_share.c - server which communicates with client app to share data
 * Copyright (C) 2022-2023 H. Thevindu J. Wijesekera
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

#include <globals.h>
#include <proto/server.h>
#include <servers/servers.h>
#include <utils/config.h>
#include <utils/net_utils.h>
#include <utils/utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#endif

#ifdef _WIN32
static DWORD WINAPI serverThreadFn(void *arg) {
    socket_t socket;
    memcpy(&socket, arg, sizeof(socket_t));
    free(arg);
    server(&socket);
    close_socket(&socket);
    return 0;
}
#endif

int clip_share(const int is_secure) {
    uint16_t port = 0;
    if (is_secure == SECURE) {
        if (configuration.allowed_clients == NULL || configuration.allowed_clients->len <= 0 ||
            configuration.app_port_secure <= 0 || configuration.server_cert.data == NULL ||
            configuration.ca_cert.data == NULL) {
#ifdef DEBUG_MODE
            puts("Invalid config for secure mode");
#endif
            return EXIT_FAILURE;
        }
        port = configuration.app_port_secure;
    } else {
        if (configuration.app_port <= 0) {
#ifdef DEBUG_MODE
            puts("Invalid port for insecure mode");
#endif
            return EXIT_FAILURE;
        }
        port = configuration.app_port;

#if defined(__linux__) || defined(__APPLE__)
        // Keys and certificates are not needed for plain sockets
        clear_config_key_cert(&configuration);
#endif
    }
    listener_t listener;
    open_listener_socket(&listener, (is_secure ? SSL_SOCK : PLAIN_SOCK), &(configuration.server_cert),
                         &(configuration.ca_cert));
    if (bind_port(listener, port) != EXIT_SUCCESS) {
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }
    if (listen(listener.socket, 3) == -1) {
        error("Can\'t listen");
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }

    while (1) {
        socket_t connect_sock;
        get_connection(&connect_sock, listener, configuration.allowed_clients);
        if (connect_sock.type == NULL_SOCK) {
            close_socket_no_wait(&connect_sock);
            continue;
        }
#if defined(__linux__) || defined(__APPLE__)
        fflush(stdout);
        fflush(stderr);
        pid_t pid = fork();
        if (pid) {
            close_socket_no_wait(&connect_sock);
        } else {
            close_listener_socket(&listener);
            server(&connect_sock);
            close_socket(&connect_sock);
            break;
        }
#elif defined(_WIN32)
        socket_t *connect_ptr = malloc(sizeof(socket_t));
        memcpy(connect_ptr, &connect_sock, sizeof(socket_t));
        HANDLE serveThread = CreateThread(NULL, 0, serverThreadFn, (LPDWORD)connect_ptr, 0, NULL);
#ifdef DEBUG_MODE
        if (serveThread == NULL) {
            error("Thread creation failed");
            return EXIT_FAILURE;
        }
#else
        (void)serveThread;
#endif
#endif
    }
    return EXIT_SUCCESS;
}
