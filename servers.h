/*
 *  servers.h - header defining supported servers
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

#ifndef SERVERS_H_
#define SERVERS_H_

#include <config.h>

#define INSECURE 0
#define SECURE 1

/*
 * Start TCP application server.
 * Use TLS if 'is_secure' is is non-zero.
 */
extern int clip_share(const int is_secure);

#ifndef NO_WEB
/*
 * Start TCP web server.
 */
extern int web_server(void);
#endif

/*
 * start UDP server listening for broadcast packets
 */
extern void udp_server(void);

#endif  // SERVERS_H_
