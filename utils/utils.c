/*
 *  utils/utils.c - platform specific implementation for utils
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/socket.h>
#elif _WIN32
#include <windows.h>
#include "win_image.h"
#endif
#include <unistd.h>

#include "utils.h"
#include "list_utils.h"
#include "../xclip/xclip.h"
#include "../xscreenshot/screenshot.h"

#define ERROR_LOG_FILE "server_err.log"

void error(const char *msg)
{
#ifdef DEBUG_MODE
    fprintf(stderr, "%s\n", msg);
#endif
    FILE *f = fopen(ERROR_LOG_FILE, "a");
    fprintf(f, "%s\n", msg);
    fclose(f);
#ifdef __linux__
    chmod(ERROR_LOG_FILE, S_IWUSR | S_IWGRP | S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
    exit(1);
#endif
}

int file_exists(const char *file_name)
{
    return access(file_name, F_OK);
}

ssize_t get_file_size(FILE *fp)
{
    struct stat statbuf;
    if (fstat(fileno(fp), &statbuf))
    {
#ifdef DEBUG_MODE
        printf("fstat failed\n");
#endif
        return -1;
    }
    if (!S_ISREG(statbuf.st_mode))
    {
#ifdef DEBUG_MODE
        printf("not a file\n");
#endif
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    ssize_t file_size = ftell(fp);
    rewind(fp);
    return file_size;
}

#ifdef __linux__

static int url_decode(char *);

int get_clipboard_text(char **bufptr, size_t *lenptr)
{
    if (xclip_util(1, NULL, lenptr, bufptr) != EXIT_SUCCESS || *lenptr <= 0) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("xclip read text failed. len = %zu\n", *lenptr);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len)
{
    if (xclip_util(0, NULL, (size_t *)&len, &data) != EXIT_SUCCESS)
    {
#ifdef DEBUG_MODE
        fputs("Failed to write to clipboard", stderr);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int get_image(char **buf_ptr, size_t *len_ptr)
{
    if (xclip_util(1, "image/png", len_ptr, buf_ptr) || *len_ptr == 0) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("xclip failed to get image/png. len = %zu\nCapturing screenshot ...\n", *len_ptr);
#endif
        *len_ptr = 0;
        if (screenshot_util(len_ptr, buf_ptr) || *len_ptr == 0) // do not change the order
        {
#ifdef DEBUG_MODE
            fputs("Get screenshot failed\n", stderr);
#endif
            *len_ptr = 0;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static inline char hex2char(char h)
{
    if ('0' <= h && h <= '9')
        return h - '0';
    if ('A' <= h && h <= 'F')
        return h - 'A' + 10;
    if ('a' <= h && h <= 'f')
        return h - 'a' + 10;
    return -1;
}

static int url_decode(char *str)
{
    if (strncmp("file://", str, 7))
        return EXIT_FAILURE;
    char *ptr1 = str, *ptr2 = strstr(str, "://");
    if (!ptr2)
        return EXIT_FAILURE;
    ptr2 += 3;
    do
    {
        char c;
        if (*ptr2 == '%')
        {
            c = 0;
            ptr2++;
            char tmp = *ptr2;
            char c1 = hex2char(tmp);
            if (c1 < 0)
                return EXIT_FAILURE; // invalid url
            c = c1 << 4;
            ptr2++;
            tmp = *ptr2;
            c1 = hex2char(tmp);
            if (c1 < 0)
                return EXIT_FAILURE; // invalid url
            c |= c1;
        }
        else
        {
            c = *ptr2;
        }
        *ptr1 = c;
        ptr1++;
        ptr2++;
    } while (*ptr2);
    *ptr1 = 0;
    return EXIT_SUCCESS;
}

list2 *get_copied_files()
{
    char *fnames;
    size_t fname_len;
    if (xclip_util(1, "x-special/gnome-copied-files", &fname_len, &fnames) || fname_len == 0) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("xclip read copied files. len = %zu\n", fname_len);
#endif
        return NULL;
    }
    fnames[fname_len] = 0;

    char *file_path = strchr(fnames, '\n');
    if (!file_path)
    {
        free(fnames);
        return NULL;
    }
    *file_path = 0;
    if (strcmp(fnames, "copy") && strcmp(fnames, "cut"))
    {
        free(fnames);
        return NULL;
    }

    size_t file_cnt = 1;
    for (char *ptr = file_path + 1; *ptr; ptr++)
    {
        if (*ptr == '\n')
        {
            file_cnt++;
            *ptr = 0;
        }
    }

    list2 *lst = init_list(file_cnt);
    if (!lst)
    {
        free(fnames);
        return NULL;
    }
    char *fname = file_path + 1;
    for (size_t i = 0; i < file_cnt; i++)
    {
        size_t off = strlen(fname) + 1;
        if (url_decode(fname) == EXIT_FAILURE)
            break;
        append(lst, strdup(fname));
        fname += off;
    }
    free(fnames);
    return lst;
}

#elif _WIN32

int get_clipboard_text(char **bufptr, size_t *lenptr)
{
    if (!OpenClipboard(0))
        return EXIT_FAILURE;
    if (!IsClipboardFormatAvailable(CF_TEXT))
    {
        CloseClipboard();
        return EXIT_FAILURE;
    }
    HANDLE h = GetClipboardData(CF_TEXT);
    char *data = strdup((char *)h);
    CloseClipboard();

    if (!data)
    {
#ifdef DEBUG_MODE
        fputs("clipboard data is null\n", stderr);
#endif
        *lenptr = 0;
        return EXIT_FAILURE;
    }
    *bufptr = data;
    *lenptr = strlen(data);
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    memcpy(GlobalLock(hMem), data, len + 1);
    GlobalUnlock(hMem);
    if (!OpenClipboard(0))
        return EXIT_FAILURE;
    EmptyClipboard();
    HANDLE res = SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
    return (res == NULL ? EXIT_FAILURE : EXIT_SUCCESS);
}

int get_image(char **buf_ptr, size_t *len_ptr)
{
    getCopiedImage(buf_ptr, len_ptr);
    if (*len_ptr > 8)
        return EXIT_SUCCESS;
    screenCapture(buf_ptr, len_ptr);
    if (*len_ptr > 8)
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

list2 *get_copied_files()
{
    if (!OpenClipboard(0))
        return NULL;
    if (!IsClipboardFormatAvailable(CF_HDROP))
    {
        CloseClipboard();
        return NULL;
    }
    HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
    if (!hGlobal)
    {
        CloseClipboard();
        return NULL;
    }
    HDROP hDrop = (HDROP)GlobalLock(hGlobal);
    if (!hDrop)
    {
        CloseClipboard();
        return NULL;
    }

    size_t file_cnt = DragQueryFile(hDrop, -1, NULL, MAX_PATH);

    if (file_cnt <= 0)
        return NULL;
    list2 *lst = init_list(file_cnt);
    if (!lst)
        return NULL;

    char fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++)
    {
        fileName[0] = '\0';
        DragQueryFile(hDrop, i, fileName, MAX_PATH);
        append(lst, strdup(fileName));
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
    return lst;
}

#endif