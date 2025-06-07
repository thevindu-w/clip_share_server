/*
 * utils/win_load_lib.h - header for win_load_lib
 * Copyright (C) 2025 H. Thevindu J. Wijesekera
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

#ifndef UTILS_WIN_LOAD_LIB_H_
#define UTILS_WIN_LOAD_LIB_H_

#ifdef _WIN64

extern int load_libs(void);
extern void cleanup_libs(void);

#endif

#endif  // UTILS_WIN_LOAD_LIB_H_
