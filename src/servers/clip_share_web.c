/*
 * servers/clip_share_web.c - web server of the application
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

#ifdef WEB_ENABLED
#define __STDC_FORMAT_MACROS
#include <globals.h>
#include <inttypes.h>
#include <servers/servers.h>
#include <stdio.h>
#include <string.h>
#include <utils/config.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#include <windows.h>
#endif

extern char blob_page[];
extern int blob_size_page;

static int say(const char *, socket_t *);
static void receiver_web(socket_t *);

static int say(const char *msg, socket_t *sock) { return write_sock(sock, msg, strlen(msg)); }

static void receiver_web(socket_t *sock) {
    char method[8];
    int ind = 0;
    do {
        if (read_sock(sock, method + ind, 1) != EXIT_SUCCESS) return;
        ind++;
    } while (method[ind - 1] != ' ');
    method[ind - 1] = 0;
#ifdef DEBUG_MODE
    puts(method);
#endif

    char path[2049];
    ind = 0;
    do {
        if (read_sock(sock, path + ind, 1) != EXIT_SUCCESS) return;
        ind++;
    } while (path[ind - 1] != ' ');
    path[ind - 1] = 0;
#ifdef DEBUG_MODE
    puts(path);
#endif

    if (!strcmp(method, "GET")) {
        char buf[128];
        while (read_sock_no_wait(sock, buf, 128) > 0) {
        }
        if (!strcmp(path, "/")) {
            if (say("HTTP/1.0 200 OK\r\n", sock) != EXIT_SUCCESS) return;
            char tmp[96];
            if (snprintf_check(
                    tmp, 96,
                    "Content-Type: text/html; charset=utf-8\r\nContent-Length: %i\r\nConnection: close\r\n\r\n",
                    blob_size_page))
                return;
            if (say(tmp, sock) != EXIT_SUCCESS) return;
            if (write_sock(sock, blob_page, (size_t)blob_size_page) != EXIT_SUCCESS) return;
        } else if (!strcmp(path, "/clip")) {
            uint32_t len;
            char *clip_buf;
            if (get_clipboard_text(&clip_buf, &len) != EXIT_SUCCESS || len <= 0) {  // do not change the order
                say("HTTP/1.0 500 Internal Server Error\r\n\r\n", sock);
                return;
            }
            if (say("HTTP/1.0 200 OK\r\n", sock) != EXIT_SUCCESS) return;
            if (say("Content-Type: text/plain; charset=utf-8\r\n", sock) != EXIT_SUCCESS) return;
            char tmp[64];
            if (snprintf_check(tmp, 64, "Content-Length: %" PRIu32 "\r\nConnection: close\r\n\r\n", len)) return;
            if (say(tmp, sock) != EXIT_SUCCESS) return;
            if (write_sock(sock, clip_buf, len) != EXIT_SUCCESS) return;
            free(clip_buf);
        } else if (!strcmp(path, "/img")) {
            uint32_t len = 0;
            char *clip_buf;
            if (get_image(&clip_buf, &len, IMG_ANY, 0) != EXIT_SUCCESS || len <= 0) {
                say("HTTP/1.0 404 Not Found\r\n\r\n", sock);
                return;
            }
            if (say("HTTP/1.0 200 OK\r\n", sock) != EXIT_SUCCESS) return;
            if (say("Content-Type: image/png\r\nContent-Disposition: attachment; filename=\"clip.png\"\r\n", sock) !=
                EXIT_SUCCESS)
                return;
            char tmp[64];
            if (snprintf_check(tmp, 64, "Content-Length: %" PRIu32 "\r\nConnection: close\r\n\r\n", len)) return;
            if (say(tmp, sock) != EXIT_SUCCESS) return;
            if (write_sock(sock, clip_buf, len) != EXIT_SUCCESS) return;
            free(clip_buf);
        } else {
            say("HTTP/1.0 404 Not Found\r\n\r\n", sock);
            return;
        }
    } else if (!strcmp(method, "POST")) {
        const unsigned int len = 2048;
        char *headers = (char *)malloc(len);
        *headers = 0;
        int r = 0;
        int cnt = 0;
        char buf[256];
        char *ptr = headers;
        char *check = ptr;
        while (1) {
            r = read_sock_no_wait(sock, buf, 256);
            if (r > 0) {
                memcpy(ptr, buf, (size_t)r);
                ptr += r;
                *ptr = 0;
                if ((size_t)(ptr - headers) >= len - 256) break;
                cnt = 0;
            } else if (cnt == 0) {
                if (strstr(check, "\r\n\r\n")) break;
                check = ptr - 3;
                check = check > headers ? check : headers;
            } else if (cnt++ > 50) {
                break;
            }
        }
        unsigned long data_len;
        const char *cont_len_header = strstr(headers, "Content-Length: ");
        if (cont_len_header == NULL) {
            free(headers);
            return;
        }
        data_len = strtoul(cont_len_header + 16, NULL, 10);
        if (data_len <= 0 || 1048576UL < data_len) {
            free(headers);
            return;
        }
        char *data = strstr(headers, "\r\n\r\n");
        if (data == NULL) {
            free(headers);
            return;
        }
        data += 4;
        *(data - 1) = '\0';
        unsigned int header_len = (unsigned int)(data - headers);
        headers = (char *)realloc_or_free(headers, header_len + data_len + 1);
        data = headers + header_len;
        cnt = 0;
        char *data_end_ptr = data + strnlen(data, data_len + 1);
        while (data_end_ptr - data < (long)data_len) {
            if ((r = read_sock_no_wait(sock, buf, 256)) > 0) {
                memcpy(data_end_ptr, buf, (size_t)r);
                data_end_ptr += r;
                *data_end_ptr = 0;
                cnt = 0;
            } else if (cnt++ > 50) {
                free(headers);
                return;
            }
        }

        if (say("HTTP/1.0 204 No Content\r\n\r\n", sock) != EXIT_SUCCESS) {
            free(headers);
            return;
        }
        put_clipboard_text(data, (uint32_t)data_len);
        free(headers);
        return;
    }
}

#ifdef _WIN32
static DWORD WINAPI webServerThreadFn(void *arg) {
    socket_t socket;
    memcpy(&socket, arg, sizeof(socket_t));
    free(arg);
    receiver_web(&socket);
    close_socket(&socket);
    return 0;
}
#endif

int web_server(void) {
    if (configuration.allowed_clients == NULL || configuration.allowed_clients->len <= 0 ||
        configuration.app_port_secure <= 0 || configuration.server_cert.data == NULL ||
        configuration.ca_cert.data == NULL) {
#ifdef DEBUG_MODE
        puts("Invalid config for web server");
#endif
        return EXIT_FAILURE;
    }

    listener_t listener;
    open_listener_socket(&listener, (configuration.bind_addr.af == AF_INET ? IPv4 : IPv6) | SSL_SOCK | VALID_SOCK,
                         &(configuration.server_cert), &(configuration.ca_cert));
    if (bind_socket(listener, configuration.bind_addr, configuration.web_port) != EXIT_SUCCESS) {
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }
    if (listen(listener.socket, 10) == -1) {
        error("Can\'t listen");
        close_listener_socket(&listener);
        return EXIT_FAILURE;
    }
    while (1) {
        socket_t connect_sock;
        get_connection(&connect_sock, listener, configuration.allowed_clients);
        if (IS_NULL_SOCK(connect_sock.type)) {
            close_socket(&connect_sock);
            continue;
        }
#ifdef __linux__
        fflush(stdout);
        fflush(stderr);
        pid_t p1 = fork();
        if (p1) {
            close_socket_no_shdn(&connect_sock);
        } else {
            close_listener_socket(&listener);
            receiver_web(&connect_sock);
            close_socket(&connect_sock);
            break;
        }
#elif defined(_WIN32)
        socket_t *connect_ptr = malloc(sizeof(socket_t));
        memcpy(connect_ptr, &connect_sock, sizeof(socket_t));
        HANDLE serveThread = CreateThread(NULL, 0, webServerThreadFn, (LPDWORD)connect_ptr, 0, NULL);
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
#endif
