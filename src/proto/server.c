/*
 * proto/server.c - implementation of server
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
#include <proto/versions.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>

// protocol version status
#define PROTOCOL_SUPPORTED 1
#define PROTOCOL_OBSOLETE 2
#define PROTOCOL_UNKNOWN 3

void server(socket_t *socket) {
    unsigned char version;
    const uint16_t min_version = configuration.min_proto_version;
    const uint16_t max_version = configuration.max_proto_version;

    if (read_sock(socket, (char *)&version, 1) != EXIT_SUCCESS) {
        return;
    }

    if (version < min_version) {  // the protocol version used by the client is obsolete and not
                                  // supported by the server
        if (write_sock(socket, &(char){PROTOCOL_OBSOLETE}, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
        }
        close_socket_no_wait(socket);
        return;
    } else if (version <= max_version) {  // the protocol version used by the client is supported by the server
        if (write_sock(socket, &(char){PROTOCOL_SUPPORTED}, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
    } else {  // the protocol version used by the client is newer than the latest protocol version supported by the
              // server
        if (write_sock(socket, &(char){PROTOCOL_UNKNOWN}, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
        if (write_sock(socket, (const char *)(&max_version), 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version failed\n");
#endif
            return;
        }
        if (read_sock(socket, (char *)&version, 1) != EXIT_SUCCESS) {
            return;
        }
        if (version != max_version) {  // client is not going to continue with a supported version
            close_socket_no_wait(socket);
            return;
        }
    }

    switch (version) {
        case 0:  // version 0 is for testing purposes
            break;
#if PROTOCOL_MIN <= 1
        case 1: {
            version_1(socket);
            break;
        }
#endif
#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
        case 2: {
            version_2(socket);
            break;
        }
#endif
#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
        case 3: {
            version_3(socket);
            break;
        }
#endif
        default:  // invalid or unknown version
            break;
    }
}
