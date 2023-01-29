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

/*
 * Opens a socket for listening.
 * If ssl_enabled is 0, SSL context is not initialized. private_key, server_certificate and ca_certificate are not required in that case.
 * Otherwise, SSL context is initialized with the provided private_key, server_certificate and ca_certificate.
 */
extern listener_t open_listener_socket(const int ssl_enabled, const char *private_key, const char *server_certificate, const char *ca_certificate);

/*
 * Binds a listener socket to a port.
 */
extern int bind_port(listener_t listener, int port);

/*
 * Accepts a TCP connection.
 * If SSL is enabled, Initialize SSL and authenticates the client,
 * allowed_clients is a list of Common Names of allowed clients.
 */
extern socket_t get_connection(listener_t listener, list2 *allowed_clients);

/*
 * Closes a socket.
 */
extern void close_socket(socket_t *socket);

/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * Waits until all the bytes are read. If reading failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int read_sock(socket_t *socket, char *buf, size_t num);

/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * returns the number of bytes read.
 * Do not wait for all the bytes to be read. Therefore the number of bytes read may be less than num.
 * returns -1 on error.
 */
extern int read_sock_no_wait(socket_t *socket, char *buf, size_t num);

/*
 * Writes num bytes from buf to the socket.
 * At least num bytes of the buf should be readable.
 * Waits until all the bytes are written. If writing failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int write_sock(socket_t *socket, const char *buf, size_t num);

/*
 * Sends a 64-bit signed integer num to socket as big-endian encoded 8 bytes.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int send_size(socket_t *socket, ssize_t num);

/*
 * Reads a 64-bit signed integer from socket as big-endian encoded 8 bytes.
 * returns the read value on success. Otherwise, returns -1 on error.
 */
extern ssize_t read_size(socket_t *socket);

#endif