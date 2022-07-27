/*
 *  proto/v1.c - platform independent implementation of protocol version 1
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

#ifdef PROTO_V1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "versions.h"
#include "../utils/utils.h"
#include "../utils/net_utils.h"

// methods
#define METHOD_GET_TEXT 1
#define METHOD_SEND_TEXT 2
#define METHOD_GET_FILE 3
#define METHOD_SEND_FILE 4
#define METHOD_GET_IMAGE 5
#define METHOD_INFO 125

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2
#define STATUS_UNKNOWN_METHOD 3
#define STATUS_METHOD_NOT IMPLEMENTED 4

#define FILE_BUF_SZ 65536

#ifdef __linux__
#define PATH_SEP '/'
#elif _WIN32
#define PATH_SEP '\\'
#endif

static int get_files_fn(socket_t socket, list2 *file_list)
{
    size_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%zu file(s)\n", file_cnt);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (send_size(socket, file_cnt) == EXIT_FAILURE)
        return EXIT_FAILURE;

    int status = EXIT_SUCCESS;
    for (size_t i = 0; i < file_cnt; i++)
    {
        char *file_path = files[i];
#ifdef DEBUG_MODE
        printf("file name = %s\n", file_path);
#endif

        char *tmp_fname = strrchr(file_path, PATH_SEP);
        if (tmp_fname == NULL)
        {
            tmp_fname = file_path;
        }
        else
        {
            tmp_fname++; // remove '/'
        }
        char filename[strlen(tmp_fname) + 1];
        strcpy(filename, tmp_fname);

        FILE *fp = fopen(file_path, "rb");
        if (!fp)
        {
#ifdef DEBUG_MODE
            printf("File open failed\n");
#endif
            status = EXIT_FAILURE;
            continue;
        }
        ssize_t file_size = get_file_size(fp);
        if (file_size < 0)
        {
#ifdef DEBUG_MODE
            printf("file size < 0\n");
#endif
            fclose(fp);
            status = EXIT_FAILURE;
            continue;
        }

        size_t fname_len = strlen(filename);
        if (send_size(socket, fname_len) == EXIT_FAILURE)
            return EXIT_FAILURE;
        if (write_sock(socket, filename, fname_len) == EXIT_FAILURE)
            return EXIT_FAILURE;
        if (send_size(socket, file_size) == EXIT_FAILURE)
            return EXIT_FAILURE;

        char data[FILE_BUF_SZ];
        while (file_size > 0)
        {
            size_t read = fread(data, 1, FILE_BUF_SZ, fp);
            if (read)
            {
                if (write_sock(socket, data, read) == EXIT_FAILURE)
                    return EXIT_FAILURE;
                file_size -= read;
            }
        }
        fclose(fp);
    }
    return status;
}

int version_1(socket_t socket)
{
    unsigned char method;
    if (read_sock(socket, (char *)&method, 1) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    switch (method)
    {
    case METHOD_GET_TEXT:
    {
        size_t length = 0;
        char *buf;
        if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0) // do not change the order
        {
#ifdef DEBUG_MODE
            printf("clipboard read text failed. len = %zu\n", length);
#endif
            write_sock(socket, &(char){STATUS_NO_DATA}, 1);
            return EXIT_SUCCESS;
        }
#ifdef DEBUG_MODE
        printf("Len = %zu\n", length);
        if (length < 1024)
        {
            fwrite(buf, 1, length, stdout);
            puts("");
        }
#endif
        if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        {
            free(buf);
            return EXIT_FAILURE;
        }
        if (send_size(socket, length) == EXIT_FAILURE)
        {
            free(buf);
            return EXIT_FAILURE;
        }
        if (write_sock(socket, buf, length) == EXIT_FAILURE)
        {
            free(buf);
            return EXIT_FAILURE;
        }
        free(buf);
        break;
    }
    case METHOD_SEND_TEXT:
    {
        if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
            return EXIT_FAILURE;
        ssize_t length = read_size(socket);
#ifdef DEBUG_MODE
        printf("Len = %zi\n", length);
#endif
        if (length <= 0)
        {
            return EXIT_FAILURE;
        }

        char *data = malloc(length + 1);
        if (read_sock(socket, data, length) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fputs("Read data failed\n", stderr);
#endif
            free(data);
            return EXIT_FAILURE;
        }
        close_socket(&socket);
        data[length] = 0;
#ifdef DEBUG_MODE
        if (length < 1024)
            puts(data);
#endif
        put_clipboard_text(data, length);
        free(data);
        break;
    }
    case METHOD_GET_FILE:
    {
        list2 *file_list = get_copied_files();
        if (!file_list)
        {
            write_sock(socket, &(char){STATUS_NO_DATA}, 1);
            return EXIT_SUCCESS;
        }

        int status = get_files_fn(socket, file_list);
        free_list(file_list);
        return status;
        break;
    }
    case METHOD_SEND_FILE:
    {
        if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
            return EXIT_FAILURE;
        ssize_t name_length = read_size(socket);
#ifdef DEBUG_MODE
        printf("name_len = %zi\n", name_length);
#endif
        if (name_length < 0)
        {
            return EXIT_FAILURE;
        }

        char file_name[name_length + 16];
        if (read_sock(socket, file_name, name_length) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fputs("Read file name failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
        file_name[name_length] = 0;
        // get only the base name
        {
            char *base_name = strrchr(file_name, PATH_SEP);
            if (base_name)
            {
                base_name++;                                          // don't want the '/' before the program name
                memmove(file_name, base_name, strlen(base_name) + 1); // overlapping memory area
            }
        }

        ssize_t length = read_size(socket);
#ifdef DEBUG_MODE
        printf("data len = %zi\n", length);
#endif
        if (length < 0)
            return EXIT_FAILURE;

        // if file already exists, use a different file name
        {
            char tmp_fname[name_length + 16];
            strcpy(tmp_fname, file_name);
            int n = 1;
            while (file_exists(tmp_fname) == 0)
            {
                sprintf(tmp_fname, "%i_%s", n++, file_name);
            }
            strcpy(file_name, tmp_fname);
        }

        FILE *file = fopen(file_name, "wb");
        if (!file)
            return EXIT_FAILURE;
        char data[FILE_BUF_SZ];
        while (length)
        {
            size_t read_len = length < FILE_BUF_SZ ? length : FILE_BUF_SZ;
            if (read_sock(socket, data, read_len) == EXIT_FAILURE)
            {
#ifdef DEBUG_MODE
                puts("recieve error");
#endif
                fclose(file);
                remove(file_name);
                return EXIT_FAILURE;
            }
            if (fwrite(data, 1, read_len, file) < read_len)
            {
                fclose(file);
                remove(file_name);
                return EXIT_FAILURE;
            }
            length -= read_len;
        }

        fclose(file);
#ifdef DEBUG_MODE
        puts("file saved");
#endif
        break;
    }
    case METHOD_GET_IMAGE:
    {
        size_t length = 0;
        char *buf;
        if (get_image(&buf, &length) == EXIT_FAILURE || length == 0) // do not change the order
        {
#ifdef DEBUG_MODE
            printf("get image failed. len = %zu\n", length);
#endif
            write_sock(socket, &(char){STATUS_NO_DATA}, 1);
            return EXIT_SUCCESS;
        }
#ifdef DEBUG_MODE
        printf("Len = %zu\n", length);
#endif
        if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
            return EXIT_FAILURE;
        if (send_size(socket, length) == EXIT_FAILURE)
            return EXIT_FAILURE;
        if (write_sock(socket, buf, length) == EXIT_FAILURE)
            return EXIT_FAILURE;
        free(buf);
        break;
    }
    case METHOD_INFO:
    {
        if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
            return EXIT_FAILURE;
        size_t len = strlen(INFO_NAME);
        if (send_size(socket, len) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send length failed\n");
#endif
            return EXIT_FAILURE;
        }
        if (write_sock(socket, INFO_NAME, len) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send name failed\n");
#endif
            return EXIT_FAILURE;
        }
        break;
    }
    default: // unknown method
    {
#ifdef DEBUG_MODE
        fprintf(stderr, "Unknown method\n");
#endif
        write_sock(socket, &(char){STATUS_UNKNOWN_METHOD}, 1);
        return EXIT_FAILURE;
        break;
    }
    }
    return EXIT_SUCCESS;
}
#endif