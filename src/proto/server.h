/*
 * proto/server.h - header of server
 * Copyright (C) 2022-2023 H. Thevindu J. Wijesekera
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

#ifndef PROTO_SERVER_H_
#define PROTO_SERVER_H_

#include <utils/net_utils.h>

/*
 * Runs the server after the socket connection is established.
 * Accepts a socket, negotiates the protocol version, and passes the control
 * to the respective version handler.
 */
extern void server(socket_t *socket);

#endif  // PROTO_SERVER_H_
