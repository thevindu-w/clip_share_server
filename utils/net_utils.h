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

extern int open_listener_socket();
extern void bind_port(int, int);
#ifdef __linux__
extern int get_connection(int);
#elif _WIN32
extern SOCKET get_connection(int);
#endif
extern void close_socket(int);

extern int read_sock(int, char *, size_t);
extern int write_sock(int, void *, size_t);
extern int send_size(int, size_t);
extern long read_size(int);

#endif