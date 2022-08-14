/*
 *  servers.h - header defining supported servers
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

#ifndef _SERVERS_
#define _SERVERS_

#include "config.h"

/**
 * start TCP application server
 */
extern int clip_share(const int, config);

/**
 * start TCP web server
 */
extern int web_server(config);

/**
 * start UDP server listening for broadcast packets
 */
extern void udp_server(const unsigned short);

#endif