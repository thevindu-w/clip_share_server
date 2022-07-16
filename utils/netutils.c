#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "utils.h"
#include "netutils.h"

int open_listener_socket()
{
    int listener_d = socket(PF_INET, SOCK_STREAM, 0);
    if (listener_d == -1)
        error("Can\'t open socket");
    return listener_d;
}

void bind_port(int socket, int port)
{
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t)htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    int reuse = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
        error("Can't set the reuse option on the socket");
    int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
    if (c == -1){
    	char errmsg[32];
    	sprintf(errmsg, "Can\'t bind to port %i", port);
        error(errmsg);
    }
}

int getConnection(int socket)
{
    struct sockaddr_in client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(socket, (struct sockaddr *)&client_addr, &address_size);
    struct timeval tv = {0, 100000};
    if (setsockopt(connect_d, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1) // set timeout option to 100ms
        error("Can't set the timeout option of the connection");
    if (connect_d == -1)
        error("Can\'t open secondary socket");
#ifdef DEBUG_MODE
    printf("\nConnection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#endif
    return connect_d;
}

int read_sock(int socket, char *buf, size_t size)
{
    int cnt = 0;
    size_t read = 0;
    char *ptr = buf;
    while (read < size)
    {
        ssize_t r = recv(socket, ptr, size - read, 0);
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

int write_sock(int socket, void *buf, size_t size){
    return (send(socket, buf, size, 0) == -1) ? EXIT_FAILURE : EXIT_SUCCESS;
}

/**
 * Sends a 64-bit long integer
 * encodes in big-endian byte order
 */
int send_size(int socket, size_t size)
{
    unsigned char sz_buf[8];
    {
        long sz = (long)size;
        for (int i = sizeof(sz_buf) - 1; i >= 0; i--)
        {
            sz_buf[i] = sz & 0xff;
            sz >>= 8;
        }
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

/**
 * Reads a 64-bit long integer
 * decodes from big-endian byte order
 */
long read_size(int socket)
{
    unsigned char sz_buf[8];
    if (read_sock(socket, (char *)sz_buf, sizeof(sz_buf)) == EXIT_FAILURE)
    {
#ifdef DEBUG_MODE
        fputs("Read size failed\n", stderr);
#endif
        return -1;
    }
    long size = 0;
    for (unsigned i = 0; i < sizeof(sz_buf); i++)
    {
        size = (size << 8) | sz_buf[i];
    }
    return size;
}