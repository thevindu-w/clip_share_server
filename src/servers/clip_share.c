/*
 * servers/clip_share.c - server which communicates with client app to share data
 * Copyright (C) 2022-2025 H. Thevindu J. Wijesekera
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
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#endif

#ifdef _WIN32
static volatile int16_t req_cnt = 0;

static DWORD WINAPI serverThreadFn(void *arg) {
    socket_t socket;
    memcpy(&socket, arg, sizeof(socket_t));
    free(arg);
    server(&socket);
    close_socket(&socket);
    req_cnt--;
    return 0;
}
#endif

#if defined(__linux__) || defined(__APPLE__)
static volatile sig_atomic_t req_cnt = 0;

static void decrement_req_cnt(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        req_cnt--;
    }
}
#endif

int clip_share(const int is_secure) {
    uint16_t port = 0;
    if (is_secure == SECURE) {
#ifndef NO_SSL
        if (configuration.allowed_clients == NULL || configuration.allowed_clients->len <= 0 ||
            configuration.app_port_secure <= 0 || configuration.server_cert.data == NULL ||
            configuration.ca_cert.data == NULL) {
#ifdef DEBUG_MODE
            puts("Invalid config for secure mode");
#endif
            return EXIT_FAILURE;
        }
        port = configuration.app_port_secure;
#else
        error("Secure mode cannot be enabled");
        return EXIT_FAILURE;
#endif
    } else {
        if (configuration.app_port <= 0) {
#ifdef DEBUG_MODE
            puts("Invalid port for insecure mode");
#endif
            return EXIT_FAILURE;
        }
        port = configuration.app_port;

#if (defined(__linux__) || defined(__APPLE__)) && !defined(NO_SSL)
        // Keys and certificates are not needed for plain sockets
        clear_config_key_cert(&configuration);
#endif
    }
    listener_t listener;
    int sock_type = VALID_SOCK;
    sock_type |= (is_secure ? SSL_SOCK : PLAIN_SOCK);
    sock_type |= (configuration.bind_addr.af == AF_INET ? IPv4 : IPv6);
    open_listener_socket(&listener, (unsigned char)sock_type, &(configuration.server_cert), &(configuration.ca_cert));
    if (bind_socket(listener, configuration.bind_addr, port) != EXIT_SUCCESS) {
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }
    if (listen(listener.socket, 3) == -1) {
        error("Can\'t listen");
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }

#if defined(__linux__) || defined(__APPLE__)
    signal(SIGCHLD, &decrement_req_cnt);
#endif
    while (1) {
        socket_t connect_sock;
        get_connection(&connect_sock, listener, configuration.allowed_clients);
        if (IS_NULL_SOCK(connect_sock.type)) {
            close_socket_no_wait(&connect_sock);
            continue;
        }
        if (req_cnt >= 64) {
            close_socket_no_wait(&connect_sock);
            continue;
        }
        req_cnt++;
#if defined(__linux__) || defined(__APPLE__)
        fflush(stdout);
        fflush(stderr);
        pid_t pid = fork();
        if (pid > 0) {
            close_socket_no_shdn(&connect_sock);
        } else if (pid == 0) {
            close_listener_socket(&listener);
            server(&connect_sock);
            close_socket(&connect_sock);
            break;
        } else {
            close_socket_no_wait(&connect_sock);
            req_cnt--;
        }
#elif defined(_WIN32)
        socket_t *connect_ptr = malloc(sizeof(socket_t));
        memcpy(connect_ptr, &connect_sock, sizeof(socket_t));
        HANDLE serveThread = CreateThread(NULL, 0, serverThreadFn, (LPDWORD)connect_ptr, 0, NULL);
        if (serveThread == NULL) {
            close_socket_no_wait(&connect_sock);
            req_cnt--;
#ifdef DEBUG_MODE
            error("Thread creation failed");
            return EXIT_FAILURE;
#endif
        }
#endif
    }
    return EXIT_SUCCESS;
}
