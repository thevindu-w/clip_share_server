/*
 *  utils/win_image.h - get screenshot in windows (header)
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

#ifndef UTILS_WIN_IMAGE_H_
#define UTILS_WIN_IMAGE_H_
#ifdef _WIN32

#include <stdlib.h>

extern void screenCapture(char **, size_t *);
extern void getCopiedImage(char **, size_t *);

#endif
#endif  // UTILS_WIN_IMAGE_H_
