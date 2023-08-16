/*
 *  utils/list_utils.c - platform independent implementation of 2d list
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

#include "./list_utils.h"

#include <stdio.h>
#include <stdlib.h>

list2 *init_list(size_t len) {
    list2 *lst = (list2 *)malloc(sizeof(list2));
    if (!lst) return NULL;
    void **arr = (void **)malloc(len * sizeof(void *));
    if (!arr) {
        free(lst);
        return NULL;
    }
    lst->array = arr;
    lst->len = 0;
    lst->capacity = len;
    return lst;
}

void free_list(list2 *lst) {
    for (size_t i = 0; i < lst->len; i++) {
        if (lst->array[i]) free(lst->array[i]);
    }
    free(lst->array);
    free(lst);
}

void append(list2 *lst, void *elem) {
    if (lst->len >= lst->capacity) {
        lst->capacity *= 2;
        lst->array = (void **)realloc(lst->array, sizeof(void *) * lst->capacity);
    }
    lst->array[lst->len] = elem;
    lst->len++;
}
