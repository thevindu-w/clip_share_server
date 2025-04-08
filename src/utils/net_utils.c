/*
 * utils/net_utils.c - platform specific implementation for socket connections
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

#include <ctype.h>
#include <errno.h>
#include <globals.h>
#ifndef NO_SSL
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef _WIN32
typedef u_short in_port_t;
#endif

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif

#define MULTICAST_ADDR "ff05::4567"

#if defined(__linux__) || defined(__APPLE__)
#define close_sock(sock) close(sock)
#elif defined(_WIN32)
#define close_sock(sock) closesocket(sock);
#endif

#define CAST_SOCKADDR_IN (struct sockaddr_in *)(void *)
#define CAST_SOCKADDR_IN6 (struct sockaddr_in6 *)(void *)

static int iterate_interfaces(in_addr_common interface_addr, listener_t listener);

#ifndef NO_SSL
static SSL_CTX *InitServerCTX(void) {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms(); /* load & register all cryptos, etc. */
    SSL_load_error_strings();     /* load all error messages */
    method = TLS_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);    /* create new context from method */
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    if (!ctx) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        return NULL;
    }
    return ctx;
}

static int LoadCertificates(SSL_CTX *ctx, const data_buffer *server_cert, const data_buffer *ca_cert) {
    BIO *sbio = BIO_new_mem_buf(server_cert->data, server_cert->len);
    PKCS12 *p12 = d2i_PKCS12_bio(sbio, NULL);
    if (!p12) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        BIO_free(sbio);
        return EXIT_FAILURE;
    }
    EVP_PKEY *key;
    X509 *server_x509;
    if (!PKCS12_parse(p12, "", &key, &server_x509, NULL)) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        BIO_free(sbio);
        PKCS12_free(p12);
        return EXIT_FAILURE;
    }
    BIO_free(sbio);
    PKCS12_free(p12);
    if (SSL_CTX_use_cert_and_key(ctx, server_x509, key, NULL, 1) != 1) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        EVP_PKEY_free(key);
        X509_free(server_x509);
        return EXIT_FAILURE;
    }
    EVP_PKEY_free(key);
    X509_free(server_x509);

    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx)) {
#ifdef DEBUG_MODE
        puts("Private key does not match the public certificate");
#endif
        return EXIT_FAILURE;
    }

    BIO *cabio = BIO_new_mem_buf(ca_cert->data, ca_cert->len);
    X509 *ca_x509 = PEM_read_bio_X509(cabio, NULL, 0, NULL);
    BIO_free(cabio);
    X509_STORE *x509store = SSL_CTX_get_cert_store(ctx);
    if (X509_STORE_add_cert(x509store, ca_x509) != 1) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stdout);
#endif
        X509_free(ca_x509);
        return EXIT_FAILURE;
    }
    X509_free(ca_x509);

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
#endif

void open_listener_socket(listener_t *listener, const unsigned char sock_type, const data_buffer *server_cert,
                          const data_buffer *ca_cert) {
    listener->type = 0;
    if (IS_NULL_SOCK(sock_type)) return;
    sock_t listener_d =
        socket((IS_IPv6(sock_type) ? PF_INET6 : PF_INET), (IS_UDP(sock_type) ? SOCK_DGRAM : SOCK_STREAM), 0);
    if (listener_d == INVALID_SOCKET) {
        error("Can\'t open socket");
        return;
    }
    listener->socket = listener_d;
    if (IS_UDP(sock_type) || !IS_SSL(sock_type)) {
        listener->type = sock_type;
        return;
    }

#ifndef NO_SSL
    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitServerCTX();
    if (!ctx) {
        return;
    }
    /* load certs and keys */
    if (LoadCertificates(ctx, server_cert, ca_cert) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Loading certificates failed\n", stderr);
#endif
        return;
    }
    listener->ctx = ctx;
    listener->type = sock_type;
#else
    listener->type = PLAIN_SOCK | VALID_SOCK;
    close_listener_socket(listener);
    (void)server_cert;
    (void)ca_cert;
#endif
}

int parse_ip(const char *address_str, in_addr_common *address_ptr) {
    if (!address_ptr) return EXIT_FAILURE;
    if (address_str == NULL) {
        address_ptr->af = AF_INET;
        address_ptr->addr.addr4.s_addr = htonl(INADDR_ANY);
        return EXIT_SUCCESS;
    }
    struct in_addr addr4;
    if (inet_pton(AF_INET, address_str, &addr4) == 1) {
        address_ptr->af = AF_INET;
        address_ptr->addr.addr4 = addr4;
        return EXIT_SUCCESS;
    }
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, address_str, &addr6) == 1) {
        address_ptr->af = AF_INET6;
        address_ptr->addr.addr6 = addr6;
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Invalid address %s\n", address_str);
#endif
    return EXIT_FAILURE;
}

int bind_socket(listener_t listener, in_addr_common bind_addr, uint16_t port) {
    if (IS_NULL_SOCK(listener.type)) return EXIT_FAILURE;
    if (bind_addr.af != (IS_IPv6(listener.type) ? AF_INET6 : AF_INET)) {
        error("Bind address and listener address family mismatch");
        return EXIT_FAILURE;
    }
    struct sockaddr_in6 server_addr6;
    struct sockaddr_in server_addr4;
    const struct sockaddr *server_addr;
    socklen_t addr_sz;
    if (IS_IPv6(listener.type)) {
        memset(&server_addr6, 0, sizeof(server_addr6));
        server_addr6.sin6_family = AF_INET6;
        server_addr6.sin6_port = htons(port);
        server_addr6.sin6_addr = bind_addr.addr.addr6;
        server_addr = (const struct sockaddr *)&server_addr6;
        addr_sz = sizeof(server_addr6);
    } else {
        memset(&server_addr4, 0, sizeof(server_addr4));
        server_addr4.sin_family = AF_INET;
        server_addr4.sin_port = htons(port);
        server_addr4.sin_addr = bind_addr.addr.addr4;
        server_addr = (const struct sockaddr *)&server_addr4;
        addr_sz = sizeof(server_addr4);
    }
    int reuse = 1;
    if (!IS_UDP(listener.type) && setsockopt(listener.socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))) {
        error("Can't set the reuse option on the socket");
        return EXIT_FAILURE;
    }
    if (bind(listener.socket, server_addr, addr_sz)) {
        char errmsg[32];
        const char *tcp_udp = IS_UDP(listener.type) ? "UDP" : "TCP";
        snprintf_check(errmsg, 32, "Can\'t bind to %s port %hu", tcp_udp, port);
        error(errmsg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int join_multicast_group(sock_t socket, unsigned int interface_index, in_addr_common multicast) {
    struct ipv6_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.ipv6mr_multiaddr = multicast.addr.addr6;
    mreq.ipv6mr_interface = interface_index;
    if (setsockopt(socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq)) < 0) {
        error("Join multicast group failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if defined(__linux__) || defined(__APPLE__)
static int iterate_interfaces(in_addr_common interface_addr, listener_t listener) {
    struct ifaddrs *ptr_ifaddrs = NULL;
    if (getifaddrs(&ptr_ifaddrs)) return EXIT_FAILURE;
    int is_any_addr = (interface_addr.af == AF_INET) ? (interface_addr.addr.addr4.s_addr == INADDR_ANY)
                                                     : IN6_ARE_ADDR_EQUAL(&interface_addr.addr.addr6, &in6addr_any);
    int status = EXIT_SUCCESS;
    char if_joined[32];
    memset(if_joined, 0, sizeof(if_joined));
    for (struct ifaddrs *ptr_entry = ptr_ifaddrs; ptr_entry; ptr_entry = ptr_entry->ifa_next) {
        if (!(ptr_entry->ifa_addr) || ptr_entry->ifa_addr->sa_family != interface_addr.af) continue;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif
        if (interface_addr.af == AF_INET) {
            if (!is_any_addr &&
                (CAST_SOCKADDR_IN(ptr_entry->ifa_addr))->sin_addr.s_addr != interface_addr.addr.addr4.s_addr)
                continue;
            in_addr_common broadcast = {.af = AF_INET};
            broadcast.addr.addr4.s_addr = (CAST_SOCKADDR_IN(ptr_entry->ifa_addr))->sin_addr.s_addr |
                                          ~(CAST_SOCKADDR_IN(ptr_entry->ifa_netmask))->sin_addr.s_addr;
            if (bind_socket(listener, broadcast, configuration.udp_port) != EXIT_SUCCESS) {
                status = EXIT_FAILURE;
                break;
            }
        } else {
            struct in6_addr ifaddr = (CAST_SOCKADDR_IN6(ptr_entry->ifa_addr))->sin6_addr;
            if (!is_any_addr && !IN6_ARE_ADDR_EQUAL(&ifaddr, &(interface_addr.addr.addr6))) continue;
            in_addr_common multicast_addr = {.af = AF_INET6};
            if (inet_pton(AF_INET6, MULTICAST_ADDR, &(multicast_addr.addr.addr6)) != 1) {
                status = EXIT_FAILURE;
                break;
            }
            unsigned int ifindex = if_nametoindex(ptr_entry->ifa_name);
            if (ifindex >= sizeof(if_joined) || if_joined[ifindex]) continue;
            if (join_multicast_group(listener.socket, ifindex, multicast_addr) != EXIT_SUCCESS) {
                status = EXIT_FAILURE;
                break;
            }
            if_joined[ifindex] = 1;
        }
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    freeifaddrs(ptr_ifaddrs);
    return status;
}
#elif defined(_WIN32)
static int iterate_interfaces(in_addr_common interface_addr, listener_t listener) {
    (void)listener;
    ULONG bufSz = 4096;
    PIP_ADAPTER_ADDRESSES pAddrs = NULL;
    int retries = 3;
    do {
        pAddrs = malloc(bufSz);
        if (!pAddrs) return EXIT_FAILURE;
        ULONG flags =
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
        ULONG ret = GetAdaptersAddresses((ULONG)interface_addr.af, flags, NULL, pAddrs, &bufSz);
        if (ret == NO_ERROR) break;
        free(pAddrs);
        pAddrs = NULL;
        if (ret != ERROR_BUFFER_OVERFLOW) return EXIT_FAILURE;
        retries--;
    } while (retries > 0 && bufSz < 1000000L);
    if (!pAddrs) return EXIT_FAILURE;
    int is_any_addr = (interface_addr.af == AF_INET)
                          ? (interface_addr.addr.addr4.s_addr == INADDR_ANY)
                          : IN6_ARE_ADDR_EQUAL((struct in6_addr *)&interface_addr.addr.addr6, &in6addr_any);
    int status = EXIT_SUCCESS;
    for (PIP_ADAPTER_ADDRESSES cur = pAddrs; cur; cur = cur->Next) {
        PIP_ADAPTER_UNICAST_ADDRESS unicast = cur->FirstUnicastAddress;
        if (!unicast) continue;
        SOCKET_ADDRESS socket_addr = unicast->Address;
        if (interface_addr.af == AF_INET) {
            if (!is_any_addr &&
                ((struct sockaddr_in *)(socket_addr.lpSockaddr))->sin_addr.s_addr != interface_addr.addr.addr4.s_addr)
                continue;
            if (bind_socket(listener, interface_addr, configuration.udp_port) != EXIT_SUCCESS) {
                status = EXIT_FAILURE;
                break;
            }
        } else {
            struct in6_addr ifaddr = ((struct sockaddr_in6 *)(socket_addr.lpSockaddr))->sin6_addr;
            if (!is_any_addr && !IN6_ARE_ADDR_EQUAL(&ifaddr, &(interface_addr.addr.addr6))) continue;
            in_addr_common multicast_addr = {.af = AF_INET6};
            if (inet_pton(AF_INET6, MULTICAST_ADDR, &(multicast_addr.addr.addr6)) != 1) {
                status = EXIT_FAILURE;
                break;
            }
            if (join_multicast_group(listener.socket, (unsigned int)cur->IfIndex, multicast_addr) != EXIT_SUCCESS) {
                status = EXIT_FAILURE;
                break;
            }
        }
    }
    free(pAddrs);
    return status;
}
#endif

int bind_udp(listener_t listener) {
    if (configuration.bind_addr_udp.af != (IS_IPv6(listener.type) ? AF_INET6 : AF_INET)) {
        error("UDP address family mismatch");
        return EXIT_FAILURE;
    }
    if (IS_IPv6(listener.type)) {
        if (iterate_interfaces(configuration.bind_addr_udp, listener) != EXIT_SUCCESS) {
            error("Interface iteration failed");
            return EXIT_FAILURE;
        }
        in_addr_common bind_addr = {.af = AF_INET6};
#if defined(_WIN32) || defined(__APPLE__)
        bind_addr.addr.addr6 = configuration.bind_addr_udp.addr.addr6;
#else
        if (inet_pton(AF_INET6, MULTICAST_ADDR, &(bind_addr.addr.addr6)) != 1) return EXIT_FAILURE;
#endif
        return bind_socket(listener, bind_addr, configuration.udp_port);
    }
    if (configuration.bind_addr_udp.addr.addr4.s_addr != INADDR_ANY) {
        return iterate_interfaces(configuration.bind_addr_udp, listener);
    }
    in_addr_common any_addr = {.af = AF_INET};
    any_addr.addr.addr4.s_addr = INADDR_ANY;
    return bind_socket(listener, any_addr, configuration.udp_port);
}

static inline sock_t _accept_connection4(sock_t listener_socket) {
    struct sockaddr_in client_addr;
#if defined(__linux__) || defined(__APPLE__)
    unsigned int address_size = sizeof(client_addr);
#elif defined(_WIN32)
    int address_size = (int)sizeof(client_addr);
#endif
    sock_t connect_d = accept(listener_socket, (struct sockaddr *)&client_addr, &address_size);
#ifdef DEBUG_MODE
    char addr_str[16];
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr_str, sizeof(addr_str));
    printf("\nConnection: %s:%d\n", addr_str, ntohs(client_addr.sin_port));
#endif
    return connect_d;
}

static inline sock_t _accept_connection6(sock_t listener_socket) {
    struct sockaddr_in6 client_addr;
#if defined(__linux__) || defined(__APPLE__)
    unsigned int address_size = sizeof(client_addr);
#elif defined(_WIN32)
    int address_size = (int)sizeof(client_addr);
#endif
    sock_t connect_d = accept(listener_socket, (struct sockaddr *)&client_addr, &address_size);
#ifdef DEBUG_MODE
    char addr_str[46];
    inet_ntop(AF_INET6, &(client_addr.sin6_addr), addr_str, sizeof(addr_str));
    printf("\nConnection: %s:%d\n", addr_str, ntohs(client_addr.sin6_port));
#endif
    return connect_d;
}

void get_connection(socket_t *sock, listener_t listener, const list2 *allowed_clients) {
    sock->type = 0;
    if (IS_NULL_SOCK(listener.type)) return;
    sock_t connect_d;
    if (IS_IPv6(listener.type)) {
        connect_d = _accept_connection6(listener.socket);
    } else {
        connect_d = _accept_connection4(listener.socket);
    }
    if (connect_d == INVALID_SOCKET) {
        error("Can\'t open secondary socket");
        return;
    }

    // set timeout option to 0.5s
    struct timeval tv = {0, 500000L};
    if (setsockopt(connect_d, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) ||
        setsockopt(connect_d, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv))) {
        close_sock(connect_d);
        error("Can't set the timeout option of the connection");
        return;
    }

    if (!IS_SSL(listener.type)) {
        sock->socket.plain = connect_d;
        sock->type = listener.type;
        return;
    }
#ifndef NO_SSL
    SSL_CTX *ctx = listener.ctx;
    SSL *ssl = SSL_new(ctx);
    SSL_set_min_proto_version(ssl, TLS1_2_VERSION);
    int ret;
#ifdef _WIN32
    ret = SSL_set_fd(ssl, (int)connect_d);
#else
    ret = SSL_set_fd(ssl, connect_d);
#endif
    if (ret != 1) {
        close_sock(connect_d);
        return;
    }
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
                close_sock(connect_d);
                return;
            }
        }
    }
    sock->socket.ssl = ssl;
    if (getClientCerts(ssl, allowed_clients) != EXIT_SUCCESS) {  // get client certificates if any
        close_socket_no_wait(sock);
        sock->type = 0;
        return;
    }
    sock->type = listener.type;
#else
    (void)allowed_clients;
#endif
}

void _close_socket(socket_t *socket, int await, int shutdown) {
    if (IS_NULL_SOCK(socket->type)) return;
    if (!IS_SSL(socket->type)) {
        if (await) {
            char tmp;
            recv(socket->socket.plain, &tmp, 1, 0);
        }
        close_sock(socket->socket.plain);
#ifndef NO_SSL
    } else {
        sock_t sd;
#ifdef _WIN32
        sd = (sock_t)SSL_get_fd(socket->socket.ssl);
#else
        sd = SSL_get_fd(socket->socket.ssl);
#endif
        int err;
        if (await) {
            char tmp;
            int ret = SSL_read(socket->socket.ssl, &tmp, 1);
            err = SSL_get_error(socket->socket.ssl, ret);
        } else {
            err = SSL_ERROR_NONE;
        }
        if (shutdown && err != SSL_ERROR_SYSCALL && err != SSL_ERROR_SSL) {
            SSL_shutdown(socket->socket.ssl);
        }
        SSL_free(socket->socket.ssl);
        close_sock(sd);
#else
    (void)shutdown;
#endif
    }
    socket->type = NULL_SOCK;
}

void close_listener_socket(listener_t *socket) {
    if (IS_NULL_SOCK(socket->type)) return;
#ifndef NO_SSL
    if (IS_SSL(socket->type)) {
        SSL_CTX_free(socket->ctx); /* release SSL state */
    }
#endif  // NO_SSL
    close_sock(socket->socket);
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

#ifndef NO_SSL
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
#endif

int read_sock(socket_t *socket, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        int fatal = 0;
        uint64_t read_req_sz = size - total_sz_read;
        if (read_req_sz > 0x7FFFFFFFL) read_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
        if (!IS_SSL(socket->type)) {
            sz_read = _read_plain(socket->socket.plain, ptr, (uint32_t)read_req_sz, &fatal);
#ifndef NO_SSL
        } else {
            sz_read = _read_SSL(socket->socket.ssl, ptr, (int)read_req_sz, &fatal);
#else
        } else {
            return EXIT_FAILURE;
#endif
        }
        if (sz_read > 0) {
            total_sz_read += (uint64_t)sz_read;
            cnt = 0;
            ptr += sz_read;
        } else if (cnt++ > 10 || fatal) {
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
    if (!IS_SSL(socket->type)) {
#ifdef _WIN32
        return recv(socket->socket.plain, buf, (int)size, 0);
#else
        return (int)recv(socket->socket.plain, buf, size, 0);
#endif
#ifndef NO_SSL
    } else {
        return SSL_read(socket->socket.ssl, buf, (int)size);
#endif
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

#ifndef NO_SSL
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
#endif

int write_sock(socket_t *socket, const char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_written = 0;
    const char *ptr = buf;
    while (total_written < size) {
        ssize_t sz_written;
        int fatal = 0;
        uint64_t write_req_sz = size - total_written;
        if (write_req_sz > 0x7FFFFFFFL) write_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
        if (!IS_SSL(socket->type)) {
            sz_written = _write_plain(socket->socket.plain, ptr, (uint32_t)write_req_sz, &fatal);
#ifndef NO_SSL
        } else {
            sz_written = _write_SSL(socket->socket.ssl, ptr, (int)write_req_sz, &fatal);
#else
        } else {
            return EXIT_FAILURE;
#endif
        }
        if (sz_written > 0) {
            total_written += (uint64_t)sz_written;
            cnt = 0;
            ptr += sz_written;
        } else if (cnt++ > 10 || fatal) {
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
