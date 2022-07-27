/*
 *  utils/net_utils.h - headers for socket connections
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

#ifndef _NET_UTILS_
#define _NET_UTILS_

#include <stdlib.h>
#include <openssl/ssl.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "list_utils.h"

#ifdef __linux__
typedef int sock_t;
#elif _WIN32
typedef SOCKET sock_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define NULL_SOCK 0
#define PLAIN_SOCK 1
#define SSL_SOCK 2

typedef struct _socket_t
{
    sock_t plain;
    SSL *ssl;
    unsigned char type;
} socket_t;

typedef struct _listener_socket_t
{
    sock_t socket;
    unsigned char type;
    SSL_CTX *ctx;
} listener_t;

extern listener_t open_listener_socket(const int, const char *, const char *, const char *);
extern int bind_port(listener_t, int);
extern socket_t get_connection(listener_t, list2 *);
extern void close_socket(socket_t *);
extern int read_sock(socket_t *, char *, size_t);
extern int read_sock_no_wait(socket_t *, char *, size_t);
extern int write_sock(socket_t *, void *, size_t);
extern int send_size(socket_t *, ssize_t);
extern ssize_t read_size(socket_t *);

#endif