/*
 *  udp_serve.c - UDP server for scanning
 *  Copyright (C) 2022-2023 H. Thevindu J. Wijesekera
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

#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <winsock2.h>
#endif

#include "./globals.h"
#include "./servers.h"
#include "utils/net_utils.h"
#include "utils/utils.h"

// MSG_WAITALL should not be used. Reset it to 0
#ifdef _WIN32
#ifdef MSG_WAITALL
#undef MSG_WAITALL
#endif
#define MSG_WAITALL 0
#endif

#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif

#ifdef _WIN32
typedef int socklen_t;
#endif

void udp_server(void) {
    if (configuration.app_port <= 0) return;

    listener_t listener;
    open_listener_socket(&listener, UDP_SOCK, NULL, NULL, NULL);
    if (listener.type == NULL_SOCK) {
        error("UDP socket creation failed");
        return;
    }
#ifdef DEBUG_MODE
    puts("UDP socket created");
#endif

    if (bind_port(listener, configuration.app_port) != EXIT_SUCCESS) {
        return;
    }

#ifdef DEBUG_MODE
    puts("UDP bind completed");
#endif

    sock_t sockfd = listener.socket;
    struct sockaddr_in cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));

    const size_t info_len = strlen(INFO_NAME);

    int n;
    socklen_t len;
    const int buf_sz = 4;
    char buffer[buf_sz];
#ifdef DEBUG_MODE
    puts("UDP server started");
#endif
    while (1) {
        len = sizeof(cliaddr);
        n = (int)recvfrom(sockfd, (char *)buffer, 2, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        if (n <= 0) {
            continue;
        }
        if (n >= buf_sz) n = buf_sz - 1;
        buffer[n] = '\0';

#ifdef DEBUG_MODE
        printf("Client UDP message : %s\n", buffer);
#endif

        if (strcmp(buffer, "in")) {
            continue;
        }
#ifdef _WIN32
        sendto(sockfd, INFO_NAME, (int)info_len, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
#else
        sendto(sockfd, INFO_NAME, info_len, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
#endif
    }
}
