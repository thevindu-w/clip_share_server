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

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef __linux__
typedef int sock_t;
#elif _WIN32
typedef SOCKET sock_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

extern sock_t open_listener_socket();
extern void bind_port(sock_t, int);
extern sock_t get_connection(sock_t);
extern void close_socket(sock_t);

extern int read_sock(sock_t, char *, size_t);
extern int write_sock(sock_t, void *, size_t);
extern int send_size(sock_t, size_t);
extern long read_size(sock_t);

#endif