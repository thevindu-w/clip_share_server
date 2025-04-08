/*
 * utils/net_utils.h - headers for socket connections
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

#ifndef UTILS_NET_UTILS_H_
#define UTILS_NET_UTILS_H_

#ifndef NO_SSL
#include <openssl/ssl.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/list_utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#elif defined(_WIN32)
// clang-format off
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
// clang-format on
#endif

typedef struct _data_buffer data_buffer;

#if defined(__linux__) || defined(__APPLE__)
typedef int sock_t;
#elif defined(_WIN32)
typedef SOCKET sock_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

// Connection validity mask
#define MASK_VALID 0x1
#define NULL_SOCK 0x0
#define VALID_SOCK 0x1
#define IS_NULL_SOCK(type) ((type & MASK_VALID) == NULL_SOCK)  // NOLINT(runtime/references)

// Connection encryption mask
#define MASK_ENC 0x2
#define PLAIN_SOCK 0x0
#define SSL_SOCK 0x2
#define IS_SSL(type) ((type & MASK_ENC) == SSL_SOCK)  // NOLINT(runtime/references)

// Transport layer protocol mask
#define MASK_TRNSPRT_PROTO 0x4
#define TRNSPRT_TCP 0x0
#define TRNSPRT_UDP 0x4
#define IS_UDP(type) ((type & MASK_TRNSPRT_PROTO) == TRNSPRT_UDP)  // NOLINT(runtime/references)

// IP version mask
#define MASK_IP_VERSION 0x8
#define IPv4 0x0
#define IPv6 0x8
#define IS_IPv6(type) ((type & MASK_IP_VERSION) == IPv6)  // NOLINT(runtime/references)

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

typedef struct _in_addr_common {
    signed char af;
    union {
        struct in_addr addr4;
        struct in6_addr addr6;
    } addr;
} in_addr_common;

/*
 * Opens a socket for listening.
 * If sock_type is a plaintext socket, an unencrypted TCP socket is created and the SSL context is not initialized.
 * server_certificate and ca_certificate are not required in that case.
 * If sock_type has SSL_SOCK, a TLS encrypted TCP socket is created and SSL context is initialized with the provided
 * server_certificate and ca_certificate.
 * If sock_type has IPv6 set, an IPv6 socket is created. Otherwise, an IPv4 socket is created.
 * If sock_type has TRNSPRT_UDP set, a UDP socket is created and SSL context is not initialized. server_certificate
 * and ca_certificate are not required in that case.
 * Otherwise, a TCP socket is created.
 * If sock_type doesn't have VALID_SOCK set, a socket will not be created and the listener will not have VALID_SOCK.
 */
extern void open_listener_socket(listener_t *listener, const unsigned char sock_type, const data_buffer *server_cert,
                                 const data_buffer *ca_cert);

/*
 * Converts an IP address string into in_addr_common.
 * If address_str is NULL, address is interpretted as IPv4 INADDR_ANY.
 * address_str should be either an IPv4 address in dot-decimal notation or an IPv6 address.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int parse_ip(const char *address_str, in_addr_common *address_ptr);

/*
 * Binds a listener socket to a port.
 */
extern int bind_socket(listener_t listener, in_addr_common address, uint16_t port);

/*
 * Binds a UDP listener socket to a port.
 */
extern int bind_udp(listener_t listener);

/*
 * Accepts a TCP connection.
 * If SSL is enabled, Initialize SSL and authenticates the client,
 * allowed_clients is a list of Common Names of allowed clients.
 */
extern void get_connection(socket_t *sock, listener_t listener, const list2 *allowed_clients);

/*
 * Closes a socket.
 */
extern void _close_socket(socket_t *socket, int await, int shutdown);

#define close_socket(socket) _close_socket(socket, 1, 1)
#define close_socket_no_wait(socket) _close_socket(socket, 0, 1)
#define close_socket_no_shdn(socket) _close_socket(socket, 0, 0)

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
