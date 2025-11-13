/*
 * globals.h - header containing global variables
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

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <utils/config.h>

#define CONFIG_FILE "clipshare.conf"
#define MAX_FILE_NAME_LEN 2048

extern config configuration;
extern char *error_log_file;
extern char *cwd;
extern size_t cwd_len;

#ifdef __APPLE__
extern const char *global_prog_name;
#endif

#endif  // GLOBALS_H_
