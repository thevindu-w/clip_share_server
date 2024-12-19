/*
 * utils/net_utils.h - headers for socket connections
 * Copyright (C) 2022-2024 H. Thevindu J. Wijesekera
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

#ifndef UTILS_NET_UTILS_H_
#define UTILS_NET_UTILS_H_

#ifndef NO_SSL
#include <openssl/ssl.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/list_utils.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
typedef int sock_t;
#elif defined(_WIN32)
typedef SOCKET sock_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define NULL_SOCK 0
#define PLAIN_SOCK 1
#define SSL_SOCK 2
#define UDP_SOCK 127

typedef struct _socket_t {
    union {
        sock_t plain;
#ifndef NO_SSL
        SSL *ssl;
#endif
    } socket;
    unsigned char type;
} socket_t;

typedef struct _listener_socket_t {
    sock_t socket;
    unsigned char type;
#ifndef NO_SSL
    SSL_CTX *ctx;
#endif
} listener_t;

/*
 * Opens a socket for listening.
 * If sock_type is PLAIN_SOCK, an unencrypted TCP socket is created and SSL context is not initialized.
 * server_certificate and ca_certificate are not required in that case.
 * If sock_type is SSL_SOCK, an TLS encrypted TCP socket is created and SSL context is initialized with the provided
 * server_certificate and ca_certificate.
 * Otherwise, a UDP socket is created and SSL context is not initialized. server_certificate and ca_certificate are
 * not required in that case.
 */
extern void open_listener_socket(listener_t *listener, const unsigned char sock_type, const data_buffer *server_cert,
                                 const data_buffer *ca_cert);

/*
 * Converts a ipv4 address in dotted decimal into in_addr_t.
 * If address_str is NULL, address is interpretted as INADDR_ANY.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int ipv4_aton(const char *address_str, uint32_t *address_ptr);

/*
 * Binds a listener socket to a port.
 */
extern int bind_port(listener_t listener, uint16_t port);

/*
 * Accepts a TCP connection.
 * If SSL is enabled, Initialize SSL and authenticates the client,
 * allowed_clients is a list of Common Names of allowed clients.
 */
extern void get_connection(socket_t *sock, listener_t listener, const list2 *allowed_clients);

/*
 * Closes a socket.
 */
extern void _close_socket(socket_t *socket, int await);

#define close_socket(socket) _close_socket(socket, 1)
#define close_socket_no_wait(socket) _close_socket(socket, 0)

/*
 * Closes a listener socket.
 */
extern void close_listener_socket(listener_t *socket);

/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * Waits until all the bytes are read. If reading failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int read_sock(socket_t *socket, char *buf, uint64_t num);

#ifdef WEB_ENABLED
/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * returns the number of bytes read.
 * Do not wait for all the bytes to be read. Therefore the number of bytes read may be less than num.
 * returns -1 on error.
 */
extern int read_sock_no_wait(socket_t *socket, char *buf, size_t num);
#endif

/*
 * Writes num bytes from buf to the socket.
 * At least num bytes of the buf should be readable.
 * Waits until all the bytes are written. If writing failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int write_sock(socket_t *socket, const char *buf, uint64_t num);

/*
 * Sends a 64-bit signed integer num to socket as big-endian encoded 8 bytes.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int send_size(socket_t *socket, int64_t num);

/*
 * Reads a 64-bit signed integer from socket as big-endian encoded 8 bytes.
 * Stores the value of the read integer in the address given by size_ptr.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int read_size(socket_t *socket, int64_t *size_ptr);

#endif  // UTILS_NET_UTILS_H_
