/*
 * utils/net_utils.c - platform specific implementation for socket connections
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

#include <ctype.h>
#include <errno.h>
#include <globals.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

#ifdef _WIN32
typedef u_short in_port_t;
#endif

static SSL_CTX *InitServerCTX(void) {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms(); /* load & register all cryptos, etc. */
    SSL_load_error_strings();     /* load all error messages */
    method = TLS_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);    /* create new context from method */
    if (!ctx) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        return NULL;
    }
    return ctx;
}

static int LoadCertificates(SSL_CTX *ctx, const char *priv_key_buf, const char *server_cert_buf,
                            const char *ca_cert_buf) {
    BIO *cbio = BIO_new_mem_buf((const void *)server_cert_buf, -1);
    X509 *cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
    if (SSL_CTX_use_certificate(ctx, cert) <= 0) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    BIO *kbio = BIO_new_mem_buf((const void *)priv_key_buf, -1);
    EVP_PKEY *key = PEM_read_bio_PrivateKey(kbio, NULL, 0, NULL);
    if (SSL_CTX_use_PrivateKey(ctx, key) <= 0) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx)) {
#ifdef DEBUG_MODE
        puts("Private key does not match the public certificate");
#endif
        return EXIT_FAILURE;
    }

    BIO *cabio = BIO_new_mem_buf((const void *)ca_cert_buf, -1);
    X509 *ca_cert = PEM_read_bio_X509(cabio, NULL, 0, NULL);
    X509_STORE *x509store = SSL_CTX_get_cert_store(ctx);
    if (X509_STORE_add_cert(x509store, ca_cert) != 1) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);
    return EXIT_SUCCESS;
}

static int getClientCerts(const SSL *ssl, const list2 *allowed_clients) {
    X509 *cert;

    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if (!cert) {
        return EXIT_FAILURE;
    }
    int verified = 0;
    char buf[256];
    X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, 255);
#ifdef DEBUG_MODE
    printf("Client Common Name: %s\n", buf);
#endif
    for (size_t i = 0; i < allowed_clients->len; i++) {
        if (!strcmp(buf, allowed_clients->array[i])) {
            verified = 1;
#ifdef DEBUG_MODE
            puts("client verified");
#endif
            break;
        }
    }
    X509_free(cert);
    return verified ? EXIT_SUCCESS : EXIT_FAILURE;
}

void open_listener_socket(listener_t *listener, const unsigned char sock_type, const char *priv_key,
                          const char *server_cert, const char *ca_cert) {
    listener->type = NULL_SOCK;

    sock_t listener_d = socket(PF_INET, (sock_type == UDP_SOCK ? SOCK_DGRAM : SOCK_STREAM), 0);
    if (listener_d == INVALID_SOCKET) {
        error("Can\'t open socket");
        return;
    }
    listener->socket = listener_d;
    if (sock_type != SSL_SOCK) {
        listener->type = sock_type;
        return;
    }

    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitServerCTX();
    if (!ctx) {
        return;
    }
    /* load certs and keys */
    if (LoadCertificates(ctx, priv_key, server_cert, ca_cert) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Loading certificates failed\n", stderr);
#endif
        return;
    }
    listener->ctx = ctx;
    listener->type = SSL_SOCK;
}

int ipv4_aton(const char *address_str, uint32_t *address_ptr) {
    if (!address_ptr) return EXIT_FAILURE;
    if (address_str == NULL) {
        *address_ptr = htonl(INADDR_ANY);
        return EXIT_SUCCESS;
    }
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
    if (sscanf(address_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a >= 256 || b >= 256 || c >= 256 || d >= 256) {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
    struct in_addr addr;
#if defined(__linux__) || defined(__APPLE__)
    if (inet_aton(address_str, &addr) != 1) {
#elif defined(_WIN32)
    if ((addr.s_addr = inet_addr(address_str)) == INADDR_NONE) {
#endif
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
#if defined(__linux__) || defined(__APPLE__)
    *address_ptr = addr.s_addr;
#elif defined(_WIN32)
    *address_ptr = (uint32_t)addr.s_addr;
#endif
    return EXIT_SUCCESS;
}

int bind_port(listener_t listener, unsigned short port) {
    if (listener.type == NULL_SOCK) return EXIT_FAILURE;
    sock_t socket = listener.socket;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = configuration.bind_addr;
    int reuse = 1;
    if (listener.type != UDP_SOCK && setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))) {
        error("Can't set the reuse option on the socket");
        return EXIT_FAILURE;
    }
    if (bind(socket, (const struct sockaddr *)&server_addr, sizeof(server_addr))) {
        char errmsg[32];
        const char *tcp_udp = listener.type == UDP_SOCK ? "UDP" : "TCP";
        snprintf_check(errmsg, 32, "Can\'t bind to %s port %hu", tcp_udp, port);
        error(errmsg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void get_connection(socket_t *sock, listener_t listener, const list2 *allowed_clients) {
    sock->type = NULL_SOCK;
    if (listener.type == NULL_SOCK) return;
    sock_t listener_socket = listener.socket;
    struct sockaddr_in client_addr;
#if defined(__linux__) || defined(__APPLE__)
    unsigned int address_size = sizeof(client_addr);
#elif defined(_WIN32)
    int address_size = (int)sizeof(client_addr);
#endif
    sock_t connect_d = accept(listener_socket, (struct sockaddr *)&client_addr, &address_size);
    if (connect_d == INVALID_SOCKET) {
        error("Can\'t open secondary socket");
        return;
    }

    // set timeout option to 100ms
    struct timeval tv = {0, 100000};
    if (setsockopt(connect_d, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv))) {
        error("Can't set the timeout option of the connection");
        return;
    }
#ifdef DEBUG_MODE
    printf("\nConnection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#endif
    switch (listener.type) {
        case PLAIN_SOCK: {
            sock->socket.plain = connect_d;
            sock->type = PLAIN_SOCK;
            break;
        }

        case SSL_SOCK: {
            SSL_CTX *ctx = listener.ctx;
            SSL *ssl = SSL_new(ctx);
#ifdef _WIN32
            SSL_set_fd(ssl, (int)connect_d);
#else
            SSL_set_fd(ssl, connect_d);
#endif
            /* do SSL-protocol accept */
            int accept_st;
            while ((accept_st = SSL_accept(ssl)) != 1) { /* do SSL-protocol accept */
                if (accept_st == 0) {
#ifdef DEBUG_MODE
                    puts("SSL_accept error");
#endif
                    return;
                }
                switch (SSL_get_error(ssl, accept_st)) {
                    case SSL_ERROR_WANT_READ:
                        continue;
                    case SSL_ERROR_WANT_WRITE:
                        continue;
                    default: {
#ifdef DEBUG_MODE
                        ERR_print_errors_fp(stdout);
#endif
                        return;
                    }
                }
            }
            sock->socket.ssl = ssl;
            sock->type = SSL_SOCK;
            if (getClientCerts(ssl, allowed_clients) != EXIT_SUCCESS) {  // get client certificates if any
                close_socket(sock);
                sock->type = NULL_SOCK;
                return;
            }
            break;
        }

        default:
            break;
    }
}

void close_socket(socket_t *socket) {
    switch (socket->type) {
        case PLAIN_SOCK: {
#if defined(__linux__) || defined(__APPLE__)
            close(socket->socket.plain);
#elif defined(_WIN32)
            closesocket(socket->socket.plain);
#endif
            break;
        }

        case SSL_SOCK: {
            sock_t sd;
#ifdef _WIN32
            sd = (sock_t)SSL_get_fd(socket->socket.ssl); /* get socket connection */
#else
            sd = SSL_get_fd(socket->socket.ssl); /* get socket connection */
#endif
            SSL_free(socket->socket.ssl); /* release SSL state */
#if defined(__linux__) || defined(__APPLE__)
            close(sd);
#elif defined(_WIN32)
            closesocket(sd);
#endif
            break;
        }

        default:
            break;
    }
    socket->type = NULL_SOCK;
}

static inline ssize_t _read_plain(sock_t sock, char *buf, size_t size, int *fatal_p) {
    ssize_t sz_read;
#ifdef _WIN32
    sz_read = recv(sock, buf, (int)size, 0);
    if (sz_read == 0) {
        *fatal_p = 1;
    } else if (sz_read < 0) {
        int err_code = WSAGetLastError();
        if (err_code == WSAEBADF || err_code == WSAECONNREFUSED || err_code == WSAECONNRESET ||
            err_code == WSAECONNABORTED || err_code == WSAESHUTDOWN || err_code == WSAEDISCON ||
            err_code == WSAEHOSTDOWN || err_code == WSAEHOSTUNREACH || err_code == WSAENETRESET ||
            err_code == WSAENETDOWN || err_code == WSAENETUNREACH || err_code == WSAEFAULT || err_code == WSAEINVAL ||
            err_code == WSA_NOT_ENOUGH_MEMORY || err_code == WSAENOTCONN || err_code == WSANOTINITIALISED ||
            err_code == WSASYSCALLFAILURE || err_code == WSAEOPNOTSUPP || err_code == WSAENOTSOCK) {
            *fatal_p = 1;
        }
    }
#else
    errno = 0;
    sz_read = recv(sock, buf, size, 0);
    if (sz_read == 0 ||
        (sz_read < 0 && (errno == EBADF || errno == ECONNREFUSED || errno == ECONNRESET || errno == ECONNABORTED ||
                         errno == ESHUTDOWN || errno == EHOSTDOWN || errno == EHOSTUNREACH || errno == ENETRESET ||
                         errno == ENETDOWN || errno == ENETUNREACH || errno == EFAULT || errno == EINVAL ||
                         errno == ENOMEM || errno == ENOTCONN || errno == EOPNOTSUPP || errno == ENOTSOCK))) {
        *fatal_p = 1;
    }
#endif
    return sz_read;
}

static inline int _read_SSL(SSL *ssl, char *buf, int size, int *fatal_p) {
    int sz_read = SSL_read(ssl, buf, size);
    if (sz_read <= 0) {
        int err_code = SSL_get_error(ssl, sz_read);
        if (err_code == SSL_ERROR_ZERO_RETURN || err_code == SSL_ERROR_SYSCALL || err_code == SSL_ERROR_SSL) {
            *fatal_p = 1;
        }
    }
    return sz_read;
}

int read_sock(socket_t *socket, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        int fatal = 0;
        switch (socket->type) {
            case PLAIN_SOCK: {
                sz_read = _read_plain(socket->socket.plain, ptr, size - total_sz_read, &fatal);
                break;
            }

            case SSL_SOCK: {
                sz_read = _read_SSL(socket->socket.ssl, ptr, (int)(size - total_sz_read), &fatal);
                break;
            }

            default:
                return EXIT_FAILURE;
        }
        if (sz_read > 0) {
            total_sz_read += (uint64_t)sz_read;
            cnt = 0;
            ptr += sz_read;
        } else if (cnt++ > 50 || fatal) {
#ifdef DEBUG_MODE
            fputs("Read sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

#ifdef WEB_ENABLED
int read_sock_no_wait(socket_t *socket, char *buf, size_t size) {
    switch (socket->type) {
        case PLAIN_SOCK: {
#ifdef _WIN32
            return recv(socket->socket.plain, buf, (int)size, 0);
#else
            return (int)recv(socket->socket.plain, buf, size, 0);
#endif
        }

        case SSL_SOCK: {
            return SSL_read(socket->socket.ssl, buf, (int)size);
        }

        default:
            return -1;
    }
}
#endif

static inline ssize_t _write_plain(sock_t sock, const char *buf, size_t size, int *fatal_p) {
    ssize_t sz_written;
#ifdef _WIN32
    sz_written = send(sock, buf, (int)size, 0);
    if (sz_written < 0) {
        int err_code = WSAGetLastError();
        if (err_code == WSAEBADF || err_code == WSAECONNREFUSED || err_code == WSAECONNRESET ||
            err_code == WSAECONNABORTED || err_code == WSAESHUTDOWN || errno == EPIPE || errno == WSAEISCONN ||
            errno == WSAEDESTADDRREQ || err_code == WSAEDISCON || err_code == WSAEHOSTDOWN ||
            err_code == WSAEHOSTUNREACH || err_code == WSAENETRESET || err_code == WSAENETDOWN ||
            err_code == WSAENETUNREACH || err_code == WSAEFAULT || err_code == WSAEINVAL ||
            err_code == WSA_NOT_ENOUGH_MEMORY || err_code == WSAENOTCONN || err_code == WSANOTINITIALISED ||
            err_code == WSASYSCALLFAILURE || err_code == WSAEOPNOTSUPP || err_code == WSAENOTSOCK) {
            *fatal_p = 1;
        }
    }
#else
    errno = 0;
    sz_written = send(sock, buf, size, 0);
    if (sz_written < 0 &&
        (errno == EBADF || errno == ECONNREFUSED || errno == ECONNRESET || errno == ECONNABORTED ||
         errno == ESHUTDOWN || errno == EPIPE || errno == EISCONN || errno == EDESTADDRREQ || errno == EHOSTDOWN ||
         errno == EHOSTUNREACH || errno == ENETRESET || errno == ENETDOWN || errno == ENETUNREACH || errno == EFAULT ||
         errno == EINVAL || errno == ENOMEM || errno == ENOTCONN || errno == EOPNOTSUPP || errno == ENOTSOCK)) {
        *fatal_p = 1;
    }
#endif
    return sz_written;
}

static inline int _write_SSL(SSL *ssl, const char *buf, int size, int *fatal_p) {
    int sz_written = SSL_write(ssl, buf, size);
    if (sz_written <= 0) {
        int err_code = SSL_get_error(ssl, sz_written);
        if (err_code == SSL_ERROR_ZERO_RETURN || err_code == SSL_ERROR_SYSCALL || err_code == SSL_ERROR_SSL) {
            *fatal_p = 1;
        }
    }
    return sz_written;
}

int write_sock(socket_t *socket, const char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_written = 0;
    const char *ptr = buf;
    while (total_written < size) {
        ssize_t sz_written;
        int fatal = 0;
        switch (socket->type) {
            case PLAIN_SOCK: {
                sz_written = _write_plain(socket->socket.plain, ptr, size - total_written, &fatal);
                break;
            }

            case SSL_SOCK: {
                sz_written = _write_SSL(socket->socket.ssl, ptr, (int)(size - total_written), &fatal);
                break;
            }

            default:
                return EXIT_FAILURE;
        }
        if (sz_written > 0) {
            total_written += (uint64_t)sz_written;
            cnt = 0;
            ptr += sz_written;
        } else if (cnt++ > 50 || fatal) {
#ifdef DEBUG_MODE
            fputs("Write sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int send_size(socket_t *socket, int64_t size) {
    char sz_buf[8];
    int64_t sz = size;
    for (int i = sizeof(sz_buf) - 1; i >= 0; i--) {
        sz_buf[i] = (char)(sz & 0xff);
        sz >>= 8;
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

int read_size(socket_t *socket, int64_t *size_ptr) {
    unsigned char sz_buf[8];
    if (read_sock(socket, (char *)sz_buf, sizeof(sz_buf)) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read size failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    int64_t size = 0;
    for (unsigned i = 0; i < sizeof(sz_buf); i++) {
        size = (size << 8) | sz_buf[i];
    }
    *size_ptr = size;
    return EXIT_SUCCESS;
}
