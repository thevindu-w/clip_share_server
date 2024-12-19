/*
 * proto/versions.c - platform independent implementation of protocol versions
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
#include <proto/methods.h>
#include <proto/versions.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/net_utils.h>

// methods
#define METHOD_GET_TEXT 1
#define METHOD_SEND_TEXT 2
#define METHOD_GET_FILE 3
#define METHOD_SEND_FILE 4
#define METHOD_GET_IMAGE 5
#define METHOD_GET_COPIED_IMAGE 6
#define METHOD_GET_SCREENSHOT 7
#define METHOD_INFO 125

// status codes
#define STATUS_UNKNOWN_METHOD 3
#define STATUS_METHOD_NOT_IMPLEMENTED 4

static int check_method_enabled(socket_t *socket, int method) {
    char disabled = 0;
    switch (method) {
        case METHOD_GET_TEXT: {
            if (!configuration.method_enabled.get_text) disabled = 1;
            break;
        }
        case METHOD_SEND_TEXT: {
            if (!configuration.method_enabled.send_text) disabled = 1;
            break;
        }
        case METHOD_GET_FILE: {
            if (!configuration.method_enabled.get_files) disabled = 1;
            break;
        }
        case METHOD_SEND_FILE: {
            if (!configuration.method_enabled.send_files) disabled = 1;
            break;
        }
        case METHOD_GET_IMAGE: {
            if (!configuration.method_enabled.get_image) disabled = 1;
            break;
        }
        case METHOD_GET_COPIED_IMAGE: {
            if (!configuration.method_enabled.get_copied_image) disabled = 1;
            break;
        }
        case METHOD_GET_SCREENSHOT: {
            if (!configuration.method_enabled.get_screenshot) disabled = 1;
            break;
        }
        case METHOD_INFO: {
            if (!configuration.method_enabled.info) disabled = 1;
            break;
        }
        default: {
            // Unknown method
        }
    }

    if (disabled) {
        write_sock(socket, &(char){STATUS_METHOD_NOT_IMPLEMENTED}, 1);
        close_socket_no_wait(socket);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if PROTOCOL_MIN <= 1

int version_1(socket_t *socket) {
    unsigned char method;
    if (read_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (check_method_enabled(socket, method) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v1(socket);
        }
        case METHOD_SEND_FILE: {
            return send_file_v1(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
#ifdef DEBUG_MODE
            fprintf(stderr, "Unknown method\n");
#endif
            write_sock(socket, &(char){STATUS_UNKNOWN_METHOD}, 1);
            close_socket_no_wait(socket);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

int version_2(socket_t *socket) {
    unsigned char method;
    if (read_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (check_method_enabled(socket, method) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v2(socket);
        }
        case METHOD_SEND_FILE: {
            return send_files_v2(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
#ifdef DEBUG_MODE
            fprintf(stderr, "Unknown method\n");
#endif
            write_sock(socket, &(char){STATUS_UNKNOWN_METHOD}, 1);
            close_socket_no_wait(socket);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)

int version_3(socket_t *socket) {
    unsigned char method;
    if (read_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (check_method_enabled(socket, method) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v3(socket);
        }
        case METHOD_SEND_FILE: {
            return send_files_v3(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_GET_COPIED_IMAGE: {
            return get_copied_image_v3(socket);
        }
        case METHOD_GET_SCREENSHOT: {
            return get_screenshot_v3(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            write_sock(socket, &(char){STATUS_UNKNOWN_METHOD}, 1);
            close_socket_no_wait(socket);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif
