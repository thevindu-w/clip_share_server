#ifdef PROTO_V1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "versions.h"
#include "../utils/utils.h"
#include "../utils/netutils.h"
#include "../xclip/xclip.h"

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

void version_1(int socket)
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
#endif