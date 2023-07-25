/*
 *  utils/net_utils.c - platform specific implementation for socket connections
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
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <unistd.h>
#elif _WIN32
#include <winsock2.h>
#endif
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>

#include "../globals.h"
#include "utils.h"
#include "net_utils.h"
#include "list_utils.h"

#ifdef _WIN32
typedef u_short in_port_t;
#endif

static SSL_CTX *InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms(); /* load & register all cryptos, etc. */
    SSL_load_error_strings();     /* load all error messages */
    method = TLS_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);    /* create new context from method */
    if (!ctx)
    {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        return NULL;
    }
    return ctx;
}

static int LoadCertificates(SSL_CTX *ctx, const char *priv_key_buf, const char *server_cert_buf, const char *ca_cert_buf)
{
    BIO *cbio = BIO_new_mem_buf((void *)server_cert_buf, -1);
    X509 *cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
    if (SSL_CTX_use_certificate(ctx, cert) <= 0)
    {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    BIO *kbio = BIO_new_mem_buf((void *)priv_key_buf, -1);
    EVP_PKEY *key = PEM_read_bio_PrivateKey(kbio, NULL, 0, NULL);
    if (SSL_CTX_use_PrivateKey(ctx, key) <= 0)
    {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
#ifdef DEBUG_MODE
        puts("Private key does not match the public certificate");
#endif
        return EXIT_FAILURE;
    }

    BIO *cabio = BIO_new_mem_buf((void *)ca_cert_buf, -1);
    X509 *ca_cert = PEM_read_bio_X509(cabio, NULL, 0, NULL);
    X509_STORE *x509store = SSL_CTX_get_cert_store(ctx);
    if (X509_STORE_add_cert(x509store, ca_cert) != 1)
    {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        return EXIT_FAILURE;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);
    return EXIT_SUCCESS;
}

static int getClientCerts(SSL *ssl, list2 *allowed_clients)
{
    X509 *cert;
    char *buf;

    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if (!cert)
    {
        return EXIT_FAILURE;
    }
    int verified = 0;
    buf = malloc(256);
    X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, 255);
#ifdef DEBUG_MODE
    printf("Client Common Name: %s\n", buf);
#endif
    for (size_t i = 0; i < allowed_clients->len; i++)
    {
        if (!strcmp(buf, allowed_clients->array[i]))
        {
            verified = 1;
#ifdef DEBUG_MODE
            puts("client verified");
#endif
            break;
        }
    }
    free(buf);
    X509_free(cert);
    return verified ? EXIT_SUCCESS : EXIT_FAILURE;
}

listener_t open_listener_socket(const int ssl_enabled, const char *priv_key, const char *server_cert, const char *ca_cert)
{
    listener_t listener;
    listener.type = NULL_SOCK;

    sock_t listener_d = socket(PF_INET, SOCK_STREAM, 0);
    if (listener_d == INVALID_SOCKET)
    {
        error("Can\'t open socket");
        return listener;
    }
    listener.socket = listener_d;
    if (!ssl_enabled)
    {
        listener.type = PLAIN_SOCK;
        return listener;
    }

    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitServerCTX();
    if (!ctx)
    {
        return listener;
    }
    /* load certs and keys */
    if (LoadCertificates(ctx, priv_key, server_cert, ca_cert) != EXIT_SUCCESS)
    {
#ifdef DEBUG_MODE
        fputs("Loading certificates failed\n", stderr);
#endif
        return listener;
    }
    listener.ctx = ctx;
    listener.type = SSL_SOCK;
    return listener;
}

int ipv4_aton(const char *address_str, uint32_t *address_ptr)
{
    if (!address_ptr)
        return EXIT_FAILURE;
    if (address_str == NULL)
    {
        *address_ptr = htonl(INADDR_ANY);
        return EXIT_SUCCESS;
    }
    unsigned int a, b, c, d;
    if (sscanf(address_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a >= 256 || b >= 256 || c >= 256 || d >= 256)
    {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
    struct in_addr addr;
    if (inet_aton(address_str, &addr) != 1)
    {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
    *address_ptr = (uint32_t)addr.s_addr;
    return EXIT_SUCCESS;
}

int bind_port(listener_t listener, unsigned short port)
{
    if (listener.type == NULL_SOCK)
        return EXIT_FAILURE;
    sock_t socket = listener.socket;
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t)htons(port);
    name.sin_addr.s_addr = configuration.bind_addr;
    int reuse = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
    {
        error("Can't set the reuse option on the socket");
        return EXIT_FAILURE;
    }
    int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
    if (c == -1)
    {
        char errmsg[32];
        snprintf_check(errmsg, 32, "Can\'t bind to TCP port %hu", port);
        error(errmsg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

socket_t get_connection(listener_t listener, list2 *allowed_clients)
{
    socket_t sock;
    sock.type = NULL_SOCK;
    if (listener.type == NULL_SOCK)
        return sock;
    sock_t listener_socket = listener.socket;
    struct sockaddr_in client_addr;
#ifdef __linux__
    unsigned int address_size = sizeof(client_addr);
#elif _WIN32
    int address_size = (int)sizeof(client_addr);
#endif
    sock_t connect_d = accept(listener_socket, (struct sockaddr *)&client_addr, &address_size);
    struct timeval tv = {0, 100000};
    if (setsockopt(connect_d, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1)
    { // set timeout option to 100ms
        error("Can't set the timeout option of the connection");
        return sock;
    }
    if (connect_d == INVALID_SOCKET)
    {
        error("Can\'t open secondary socket");
        return sock;
    }
#ifdef DEBUG_MODE
    printf("\nConnection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#endif
    switch (listener.type)
    {
    case PLAIN_SOCK:
    {
        sock.socket.plain = connect_d;
        sock.type = PLAIN_SOCK;
        break;
    }

    case SSL_SOCK:
    {
        SSL_CTX *ctx = listener.ctx;
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, connect_d);
        /* do SSL-protocol accept */
        int accept_st;
        while ((accept_st = SSL_accept(ssl)) != 1)
        { /* do SSL-protocol accept */
            if (accept_st == 0)
            {
#ifdef DEBUG_MODE
                puts("SSL_accept error");
#endif
                return sock;
            }
            switch (SSL_get_error(ssl, accept_st))
            {
            case SSL_ERROR_WANT_READ:
                continue;
            case SSL_ERROR_WANT_WRITE:
                continue;
            default:
            {
#ifdef DEBUG_MODE
                ERR_print_errors_fp(stdout);
#endif
                return sock;
            }
            }
        }
        sock.socket.ssl = ssl;
        sock.type = SSL_SOCK;
        if (getClientCerts(ssl, allowed_clients) != EXIT_SUCCESS) /* get any certificates */
        {
            close_socket(&sock);
            sock.type = NULL_SOCK;
            return sock;
        }
        break;
    }

    default:
        break;
    }
    return sock;
}

void close_socket(socket_t *socket)
{
    switch (socket->type)
    {
    case PLAIN_SOCK:
    {
#ifdef __linux__
        close(socket->socket.plain);
#elif _WIN32
        closesocket(socket->socket.plain);
#endif
        break;
    }

    case SSL_SOCK:
    {
        sock_t sd = SSL_get_fd(socket->socket.ssl); /* get socket connection */
        SSL_free(socket->socket.ssl);               /* release SSL state */
#ifdef __linux__
        close(sd);
#elif _WIN32
        closesocket(sd);
#endif
        break;
    }

    default:
        break;
    }
    socket->type = NULL_SOCK;
}

int read_sock(socket_t *socket, char *buf, size_t size)
{
    int cnt = 0;
    size_t read = 0;
    char *ptr = buf;
    while (read < size)
    {
        ssize_t r;
        switch (socket->type)
        {
        case PLAIN_SOCK:
        {
            r = recv(socket->socket.plain, ptr, size - read, 0);
            break;
        }

        case SSL_SOCK:
        {
            r = SSL_read(socket->socket.ssl, ptr, size - read);
            break;
        }

        default:
            return EXIT_FAILURE;
        }
        if (r > 0)
        {
            read += r;
            cnt = 0;
            ptr += r;
        }
        else
        {
            if (cnt++ > 50)
            {
#ifdef DEBUG_MODE
                fputs("Read sock failed\n", stderr);
#endif
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}

int read_sock_no_wait(socket_t *socket, char *buf, size_t size)
{
    switch (socket->type)
    {
    case PLAIN_SOCK:
    {
        return (int)recv(socket->socket.plain, buf, size, 0);
    }

    case SSL_SOCK:
    {
        return (int)SSL_read(socket->socket.ssl, buf, size);
    }

    default:
        return -1;
    }
    return 0;
}

int write_sock(socket_t *socket, const char *buf, size_t size)
{
    int cnt = 0;
    size_t written = 0;
    const char *ptr = buf;
    while (written < size)
    {
        ssize_t r;
        switch (socket->type)
        {
        case PLAIN_SOCK:
        {
            r = send(socket->socket.plain, ptr, size, 0);
            break;
        }

        case SSL_SOCK:
        {
            r = SSL_write(socket->socket.ssl, ptr, size);
            break;
        }

        default:
            return EXIT_FAILURE;
        }
        if (r > 0)
        {
            written += r;
            cnt = 0;
            ptr += r;
        }
        else if (r == 0)
        {
            if (cnt++ > 50)
            {
#ifdef DEBUG_MODE
                fputs("Write sock failed\n", stderr);
#endif
                return EXIT_FAILURE;
            }
        }
        else
        {
#ifdef DEBUG_MODE
            fputs("Write sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int send_size(socket_t *socket, ssize_t size)
{
    char sz_buf[8];
    {
        ssize_t sz = (ssize_t)size;
        for (int i = sizeof(sz_buf) - 1; i >= 0; i--)
        {
            sz_buf[i] = sz & 0xff;
            sz >>= 8;
        }
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

ssize_t read_size(socket_t *socket)
{
    unsigned char sz_buf[8];
    if (read_sock(socket, (char *)sz_buf, sizeof(sz_buf)) == EXIT_FAILURE)
    {
#ifdef DEBUG_MODE
        fputs("Read size failed\n", stderr);
#endif
        return -1;
    }
    ssize_t size = 0;
    for (unsigned i = 0; i < sizeof(sz_buf); i++)
    {
        size = (size << 8) | sz_buf[i];
    }
    return size;
}
