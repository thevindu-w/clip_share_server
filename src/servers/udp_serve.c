/*
 * servers/udp_serve.c - UDP server for scanning
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
#include <servers/servers.h>
#include <stdio.h>
#include <string.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

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
    if (configuration.udp_port <= 0) return;
#if defined(__linux__) || defined(__APPLE__)
    // Keys and certificates are not needed for UDP
    clear_config_key_cert(&configuration);
#endif

    listener_t listener;
    open_listener_socket(
        &listener, (configuration.bind_addr_udp.af == AF_INET ? IPv4 : IPv6) | TRNSPRT_UDP | VALID_SOCK, NULL, NULL);
    if (IS_NULL_SOCK(listener.type)) {
        error("UDP socket creation failed");
        return;
    }
#ifdef DEBUG_MODE
    puts("UDP socket created");
#endif

    if (bind_udp(listener) != EXIT_SUCCESS) {
        close_listener_socket(&listener);
        return;
    }

#ifdef DEBUG_MODE
    puts("UDP bind completed");
#endif

    sock_t sockfd = listener.socket;
    struct sockaddr *addr_p;
    struct sockaddr_in client_addr4;
    struct sockaddr_in6 client_addr6;
    socklen_t addr_len;
    if (IS_IPv6(listener.type)) {
        memset(&client_addr6, 0, sizeof(client_addr6));
        addr_p = (struct sockaddr *)&client_addr6;
        addr_len = sizeof(client_addr6);
    } else {
        memset(&client_addr4, 0, sizeof(client_addr4));
        addr_p = (struct sockaddr *)&client_addr4;
        addr_len = sizeof(client_addr4);
    }

    const size_t info_len = sizeof(INFO_NAME) - 1;

    int n;
    socklen_t len;
    const int buf_sz = 4;
    char buffer[buf_sz];
#ifdef DEBUG_MODE
    puts("UDP server started");
#endif
    while (1) {
        len = addr_len;
        n = (int)recvfrom(sockfd, (char *)buffer, 2, MSG_WAITALL, addr_p, &len);
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
        sendto(sockfd, INFO_NAME, (int)info_len, MSG_CONFIRM, addr_p, len);
#else
        sendto(sockfd, INFO_NAME, info_len, MSG_CONFIRM, addr_p, len);
#endif
    }
}
