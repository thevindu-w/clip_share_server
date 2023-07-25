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

#include "utils/net_utils.h"
#include "utils/utils.h"
#include "servers.h"

#ifdef _WIN32
typedef int socklen_t;
#endif

void udp_server(const unsigned short port)
{
    if (port <= 0)
        return;
    sock_t sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        error("UDP socket creation failed");
        return;
    }

#ifdef DEBUG_MODE
    puts("UDP socket created");
#endif

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        char errmsg[32];
        snprintf_check(errmsg, 32, "Can\'t bind to UDP port %hu", port);
        error(errmsg);
        return;
    }

#ifdef DEBUG_MODE
    puts("UDP bind completed");
#endif

    const size_t info_len = strlen(INFO_NAME);

    int n;
    socklen_t len;
    const int buf_sz = 8;
    char buffer[buf_sz];
#ifdef DEBUG_MODE
    puts("UDP server started");
#endif
    while (1)
    {
        len = sizeof(cliaddr);
#ifdef __linux__
        n = recvfrom(sockfd, (char *)buffer, 2, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
#elif _WIN32
        n = recvfrom(sockfd, (char *)buffer, 2, 0, (struct sockaddr *)&cliaddr, &len);
#endif
        if (n <= 0)
        {
            continue;
        }
        if (n >= 8)
            n = 7;
        buffer[n] = '\0';

#ifdef DEBUG_MODE
        printf("Client UDP message : %s\n", buffer);
#endif

        if (strcmp(buffer, "in"))
        {
            continue;
        }
#ifdef __linux__
        sendto(sockfd, INFO_NAME, info_len, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
#elif _WIN32
        sendto(sockfd, INFO_NAME, info_len, 0, (const struct sockaddr *)&cliaddr, len);
#endif
    }
}
