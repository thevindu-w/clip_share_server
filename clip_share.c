/*
 *  clip_share.c - server which communicates with client app to share data
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
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"
#include "xclip/xclip.h"
#include "xscreenshot/screenshot.h"

// protocol version status
#define PROTOCOL_SUPPORTED 1
#define PROTOCOL_OBSOLETE 2
#define PROTOCOL_UNKNOWN 3

// methods
#define METHOD_GET_TEXT 1
#define METHOD_SEND_TEXT 2
#define METHOD_GET_FILE 3
#define METHOD_SEND_FILE 4
#define METHOD_GET_IMAGE 5
#define METHOD_INFO 253

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2
#define STATUS_UNKNOWN_METHOD 3
#define STATUS_METHOD_NOT IMPLEMENTED 4

#define FILE_BUF_SZ 65536

static void version_1(int);
static void server(int);
static int read_sock(int, char *, size_t);
static int get_image(char **, size_t *);
static int send_size(int, size_t);
static long read_size(int socket);
static int url_decode(char *);

static int get_image(char **buf_ptr, size_t *len_ptr)
{
    if (xclip_util(1, "image/png", len_ptr, buf_ptr) || *len_ptr == 0) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("xclip failed to get image/png. len = %lu\n", *len_ptr);
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

static int read_sock(int socket, char *buf, size_t size)
{
    int cnt = 0;
    size_t read = 0;
    char *ptr = buf;
    while (read < size)
    {
        ssize_t r = recv(socket, ptr, size - read, 0);
        if (r > 0)
        {
            read += r;
            cnt = 0;
            ptr += r;
        }
        else
        {
            if (cnt++ > 50)
            {
#ifdef DEBUG_MODE
                fputs("Read sock failed\n", stderr);
#endif
                return EXIT_FAILURE;
            }
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

/**
 * Sends a 64-bit long integer
 * encodes in big-endian byte order
 */
static int send_size(int socket, size_t length)
{
    unsigned char data_len_buf[8];
    {
        long len = (long)length;
        for (int i = sizeof(data_len_buf) - 1; i >= 0; i--)
        {
            data_len_buf[i] = len & 0xff;
            len >>= 8;
        }
    }
    if (send(socket, data_len_buf, sizeof(data_len_buf), 0) == -1)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

/**
 * Reads a 64-bit long integer
 * decodes from big-endian byte order
 */
static long read_size(int socket)
{
    unsigned char data_len_buf[8];
    if (read_sock(socket, (char *)data_len_buf, sizeof(data_len_buf)) == EXIT_FAILURE)
    {
#ifdef DEBUG_MODE
        fputs("Read length failed\n", stderr);
#endif
        return -1;
    }
    long length = 0;
    for (unsigned i = 0; i < sizeof(data_len_buf); i++)
    {
        length = (length << 8) | data_len_buf[i];
    }
    return length;
}

static void version_1(int socket)
{
    unsigned char method;
    if (read_sock(socket, (char *)&method, 1) == EXIT_FAILURE)
    {
        return;
    }

    switch (method)
    {
    case METHOD_GET_TEXT:
    {
        size_t length = 0;
        char *buf;
        if (xclip_util(1, NULL, &length, &buf) || length <= 0) // do not change the order
        {
#ifdef DEBUG_MODE
            printf("xclip read text failed. len = %lu\n", length);
#endif
            send(socket, &(char){STATUS_NO_DATA}, 1, 0);
            return;
        }
#ifdef DEBUG_MODE
        printf("Len = %lu\n", length);
        if (length < 1024)
        {
            fwrite(buf, 1, length, stdout);
            puts("");
        }
#endif
        if (send(socket, &(char){STATUS_OK}, 1, 0) == -1)
            error("send status failed");
        if (send_size(socket, length) == EXIT_FAILURE)
            error("send length failed");
        if (send(socket, buf, length, 0) == -1)
            error("send text failed");
        free(buf);
        break;
    }
    case METHOD_SEND_TEXT:
    {
        if (send(socket, &(char){STATUS_OK}, 1, 0) == -1)
            error("send status failed");
        long length = read_size(socket);
#ifdef DEBUG_MODE
        printf("Len = %lu\n", length);
#endif
        if (length <= 0)
        {
            return;
        }

        char *data = malloc(length + 1);
        if (read_sock(socket, data, length) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fputs("Read data failed\n", stderr);
#endif
            free(data);
            return;
        }
        close(socket);
        data[length] = 0;
#ifdef DEBUG_MODE
        if (length < 1024)
            puts(data);
#endif
        if (xclip_util(0, NULL, (size_t *)&length, &data))
        {
#ifdef DEBUG_MODE
            fputs("Failed to write to clip", stderr);
#endif
        }
        free(data);
        return;
        break;
    }
    case METHOD_GET_FILE:
    {
        char *fname;
        size_t fname_len;
        if (xclip_util(1, "x-special/gnome-copied-files", &fname_len, &fname) || fname_len == 0) // do not change the order
        {
#ifdef DEBUG_MODE
            printf("xclip read file name. file name len = %lu\n", fname_len);
#endif
            send(socket, &(char){STATUS_NO_DATA}, 1, 0);
            return;
        }
        fname[fname_len] = 0;

        char *file_path = strchr(fname, '\n');
        if (!file_path)
        {
            free(fname);
            return;
        }
        *file_path = 0;
        if (strcmp(fname, "copy") && strcmp(fname, "cut"))
        {
            free(fname);
            return;
        }

        size_t file_cnt = 1;
        for (char *ptr = file_path + 1; *ptr; ptr++)
        {
            if (*ptr == '\n')
            {
                file_cnt++;
            }
        }
#ifdef DEBUG_MODE
        printf("%lu file(s)\n", file_cnt);
#endif
        if (file_cnt < 1)
        {
            send(socket, &(char){STATUS_NO_DATA}, 1, 0);
            return;
        }
        if (send(socket, &(char){STATUS_OK}, 1, 0) == -1)
            error("send status failed");

        if (send_size(socket, file_cnt) == EXIT_FAILURE)
            error("send file count failed");

        int has_next_file = 1;
        char *nl;
        do
        {
            file_path++;
            nl = strchr(file_path, '\n');
            if (nl)
            {
                *nl = 0;
            }
            else
            {
                has_next_file = 0;
            }

            if (url_decode(file_path) == EXIT_FAILURE)
            {
#ifdef DEBUG_MODE
                puts("invalid file path");
#endif
                continue;
            }
#ifdef DEBUG_MODE
            printf("file name = %s\n", file_path);
#endif

            char *tmp_fname = strrchr(file_path, '/');
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
                continue;
            }
            struct stat statbuf;
            if (fstat(fileno(fp), &statbuf))
            {
#ifdef DEBUG_MODE
                printf("fstat failed\n");
#endif
                fclose(fp);
                continue;
            }
            if (!S_ISREG(statbuf.st_mode))
            {
#ifdef DEBUG_MODE
                printf("not a file\n");
#endif
                fclose(fp);
                continue;
            }
            fseek(fp, 0L, SEEK_END);
            long file_size = ftell(fp);
            rewind(fp);
            if (file_size < 0)
            {
#ifdef DEBUG_MODE
                printf("file size < 0\n");
#endif
                fclose(fp);
                continue;
            }
            fname_len = strlen(filename);
            if (send_size(socket, fname_len) == EXIT_FAILURE)
                error("send file name length failed");
            if (send(socket, filename, fname_len, 0) == -1)
                error("send filename failed");
            if (send_size(socket, file_size) == EXIT_FAILURE)
                error("send file size failed");
            char data[FILE_BUF_SZ];
            while (file_size > 0)
            {
                size_t read = fread(data, 1, FILE_BUF_SZ, fp);
                if (read)
                {
                    if (send(socket, data, read, 0) == -1)
                        error("send failed");
                    file_size -= read;
                }
            }
            fclose(fp);
            file_path = nl;
        } while (has_next_file);
        free(fname);
        return;
        break;
    }
    case METHOD_SEND_FILE:
    {
        if (send(socket, &(char){STATUS_OK}, 1, 0) == -1)
            error("send status failed");
        long name_length = read_size(socket);
#ifdef DEBUG_MODE
        printf("name_len = %lu\n", name_length);
#endif
        if (name_length < 0)
        {
            return;
        }

        char *file_name = (char *)malloc(name_length + 1);
        if (read_sock(socket, file_name, name_length) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fputs("Read file name failed\n", stderr);
#endif
            free(file_name);
            return;
        }
        file_name[name_length] = 0;
        // get only the base name
        {
            char *base_name = strrchr(file_name, '/');
            if (base_name)
            {
                base_name++;                                          // don't want the '/' before the program name
                memmove(file_name, base_name, strlen(base_name) + 1); // overlapping memory areas
                file_name = (char *)realloc(file_name, strlen(file_name) + 1);
            }
        }

        long length = read_size(socket);
#ifdef DEBUG_MODE
        printf("data len = %lu\n", length);
#endif
        if (length < 0)
        {
            free(file_name);
            return;
        }

        // if file already exists, use a different file name
        {
            char *tmp_fname = (char *)malloc(name_length + 16);
            strcpy(tmp_fname, file_name);
            int n = 1;
            while (access(tmp_fname, F_OK) == 0)
            {
                sprintf(tmp_fname, "%i_%s", n++, file_name);
            }
            file_name = (char *)realloc(file_name, strlen(tmp_fname) + 1);
            strcpy(file_name, tmp_fname);
            free(tmp_fname);
        }

        FILE *file = fopen(file_name, "wb");
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
                free(file_name);
                return;
            }
            fwrite(data, 1, read_len, file);
            length -= read_len;
        }

        fclose(file);
#ifdef DEBUG_MODE
        puts("file closed");
#endif
        free(file_name);
        break;
    }
    case METHOD_GET_IMAGE:
    {
        size_t length = 0;
        char *buf;
        if (get_image(&buf, &length) == EXIT_FAILURE || length == 0) // do not change the order
        {
#ifdef DEBUG_MODE
            printf("get image failed. len = %lu\n", length);
#endif
            send(socket, &(char){STATUS_NO_DATA}, 1, 0);
            return;
        }
#ifdef DEBUG_MODE
        printf("Len = %lu\n", length);
#endif
        if (send(socket, &(char){STATUS_OK}, 1, 0) == -1)
            error("send status failed");
        if (send_size(socket, length) == EXIT_FAILURE)
            error("send length failed");
        if (send(socket, buf, length, 0) == -1)
            error("send image failed");
        free(buf);
        break;
    }
    case METHOD_INFO:
    {
        size_t len = strlen(INFO_NAME);
        if (send_size(socket, len) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send length failed\n");
#endif
            return;
        }
        if (send(socket, INFO_NAME, len, 0) == -1)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send name failed\n");
#endif
            return;
        }
        break;
    }
    default: // unknown method
    {
        send(socket, &(char){STATUS_UNKNOWN_METHOD}, 1, 0);
        break;
    }
    }
}

static void server(int socket)
{
    unsigned char version;
    if (read_sock(socket, (char *)&version, 1) == EXIT_FAILURE)
    {
        return;
    }
    if (version < PROTOCOL_MIN)
    { /* the protocol version used by the client is obsolete
        and not supported by the server */
        if (send(socket, &(char){PROTOCOL_OBSOLETE}, 1, 0) == -1)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
        }
        return;
    }
    else if (version <= PROTOCOL_MAX)
    { /* the protocol version used by the client is supported by the server */
        if (send(socket, &(char){PROTOCOL_SUPPORTED}, 1, 0) == -1)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
    }
    else
    { /* the protocol version used by the client is newer than the
        latest protocol version supported by the server */
        if (send(socket, &(char){PROTOCOL_UNKNOWN}, 1, 0) == -1)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
        if (send(socket, &(char){PROTOCOL_MAX}, 1, 0) == -1)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version failed\n");
#endif
            return;
        }
        if (read_sock(socket, (char *)&version, 1) == EXIT_FAILURE)
        {
            return;
        }
        if (version != PROTOCOL_MAX)
        { /* client is not going to continue with a supported version */
            return;
        }
    }
    switch (version)
    {
    case 0: // version 0 is for testing purposes
        break;
    case 1:
    {
        version_1(socket);
        break;
    }
    default: // invalid or unknown version
        break;
    }
}

int clip_server(const int port)
{
    int listener_d = open_listener_socket();
    bind_port(listener_d, port);
    if (listen(listener_d, 3) == -1)
        error("Can\'t listen");
    signal(SIGCHLD, SIG_IGN);
    while (1)
    {
        int connect_d = getConnection(listener_d);
#ifndef DEBUG_MODE
        pid_t p1 = fork();
        if (p1)
        {
            close(connect_d);
        }
        else
        {
            close(listener_d);
#endif
            server(connect_d);
            close(connect_d);
#ifndef DEBUG_MODE
            break;
        }
#endif
    }
    return 0;
}
