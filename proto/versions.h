/*
 *  proto/versions.h - header for declaring protocol versions
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
#ifndef _VERSIONS_
#define _VERSIONS_

#include "../utils/net_utils.h"

#if (PROTOCOL_MIN <= 1) && (1 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection after the protocol version 1 is selected
 * after the negotiation phase.
 * Reads the method code from the client and pass the control to the respective
 * method handler.
 */
extern int version_1(socket_t *socket);
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection after the protocol version 2 is selected
 * after the negotiation phase.
 * Reads the method code from the client and pass the control to the respective
 * method handler.
 */
extern int version_2(socket_t *socket);
#endif

#endif
