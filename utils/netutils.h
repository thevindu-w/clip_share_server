#ifndef _NETUTILS_
#define _NETUTILS_

#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

extern int open_listener_socket();
extern void bind_port(int, int);
extern int getConnection(int);

extern int read_sock(int, char *, size_t);
extern int write_sock(int, void *, size_t);

#endif