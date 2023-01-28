/*
 *  proto/methods.h - declarations of methods
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

#include "../utils/net_utils.h"

// Version 1 methods

extern int get_text_v1(socket_t *socket);
extern int send_text_v1(socket_t *socket);
extern int get_files_v1(socket_t *socket);
extern int send_file_v1(socket_t *socket);
extern int get_image_v1(socket_t *socket);
extern int info_v1(socket_t *socket);

// Version 2 methods

extern int get_files_v2(socket_t *socket);
extern int send_files_v2(socket_t *socket);