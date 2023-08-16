/*
 *  utils/list_utils.h - header for 2d list
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

#ifndef UTILS_LIST_UTILS_H_
#define UTILS_LIST_UTILS_H_

#include <stdlib.h>

typedef struct _list {
    size_t len;
    size_t capacity;
    void **array;
} list2;

/*
 * Initialize a list2 with initial size len
 * returns NULL on error
 */
extern list2 *init_list(size_t len);

/*
 * Free the memory allocated to a list2 *lst
 */
extern void free_list(list2 *lst);

/*
 * Appends the element elem to the list lst.
 * Allocates more space if needed.
 */
extern void append(list2 *lst, void *elem);

#endif  // UTILS_LIST_UTILS_H_
