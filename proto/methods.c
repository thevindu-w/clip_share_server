/*
 *  proto/methods.c - platform independent implementation of methods
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../utils/utils.h"
#include "../utils/net_utils.h"
#include "methods.h"
#include "../globals.h"

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2

#define FILE_BUF_SZ 65536          // 64 KiB
#define MAX_FILE_SIZE 68719476736l // 64 GiB
#define MAX_FILE_NAME_LENGTH 2048
#define MAX_TEXT_LENGTH 4194304   // 4 MiB
#define MAX_IMAGE_SIZE 1073741824 // 1 GiB

int get_text_v1(socket_t *socket)
{
    size_t length = 0;
    char *buf = NULL;
    if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0 || length > MAX_TEXT_LENGTH) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("clipboard read text failed. len = %zu\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf)
            free(buf);
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
    return EXIT_SUCCESS;
}

int send_text_v1(socket_t *socket)
{
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        return EXIT_FAILURE;
    ssize_t length = read_size(socket);
#ifdef DEBUG_MODE
    printf("Len = %zi\n", length);
#endif
    if (length <= 0 || length > MAX_TEXT_LENGTH) // limit maximum length to 4 MiB
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
    close_socket(socket);
    data[length] = 0;
#ifdef DEBUG_MODE
    if (length < 1024)
        puts(data);
#endif
    put_clipboard_text(data, length);
    free(data);
    return EXIT_SUCCESS;
}

int get_files_v1(socket_t *socket)
{
    list2 *file_list = get_copied_files();
    if (!file_list || file_list->len == 0)
    {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        return EXIT_SUCCESS;
    }

    int status = EXIT_FAILURE;
    size_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%zu file(s)\n", file_cnt);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        goto END;

    if (send_size(socket, file_cnt) == EXIT_FAILURE)
        goto END;

    status = EXIT_SUCCESS;
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
        size_t _tmp_len = strlen(tmp_fname);
        char filename[_tmp_len + 1];
        strncpy(filename, tmp_fname, _tmp_len);
        filename[_tmp_len] = 0;
        size_t fname_len = strlen(filename);
        if (fname_len > MAX_FILE_NAME_LENGTH)
        {
            status = EXIT_FAILURE;
            goto END;
        }

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
        if (file_size < 0 || file_size > MAX_FILE_SIZE)
        {
#ifdef DEBUG_MODE
            printf("file size = %zi\n", file_size);
#endif
            fclose(fp);
            status = EXIT_FAILURE;
            continue;
        }

        if (send_size(socket, fname_len) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }
        if (write_sock(socket, filename, fname_len) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }
        if (send_size(socket, file_size) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }

        char data[FILE_BUF_SZ];
        while (file_size > 0)
        {
            size_t read = fread(data, 1, FILE_BUF_SZ, fp);
            if (read)
            {
                if (write_sock(socket, data, read) == EXIT_FAILURE)
                {
                    fclose(fp);
                    status = EXIT_FAILURE;
                    goto END;
                }
                file_size -= read;
            }
        }
        fclose(fp);
    }
END:
    free_list(file_list);
    return status;
}

int send_file_v1(socket_t *socket)
{
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        return EXIT_FAILURE;
    const ssize_t name_length = read_size(socket);
#ifdef DEBUG_MODE
    printf("name_len = %zi\n", name_length);
#endif
    if (name_length <= 0 || name_length > MAX_FILE_NAME_LENGTH) // limit file name length to 1024 chars
    {
        return EXIT_FAILURE;
    }

    const ssize_t name_max_len = name_length + 16;
    char file_name[name_max_len + 1];
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
        char *base_name = strrchr(file_name, '/'); // path separator is / when communicating with the client
        if (base_name)
        {
            base_name++;                                          // don't want the '/' before the file name
            memmove(file_name, base_name, strlen(base_name) + 1); // overlapping memory area
        }
        if (PATH_SEP != '/' && strchr(file_name, PATH_SEP)) // file name can't contain PATH_SEP
            return EXIT_FAILURE;
    }

    // PATH_SEP is not allowed in file name
    if (strchr(file_name, PATH_SEP))
        return EXIT_FAILURE;

    ssize_t file_size = read_size(socket);
#ifdef DEBUG_MODE
    printf("data len = %zi\n", file_size);
#endif
    if (file_size < 0 || file_size > MAX_FILE_SIZE)
        return EXIT_FAILURE;

    // if file already exists, use a different file name
    {
        char tmp_fname[name_max_len + 1];
        if (configuration.working_dir != NULL || strcmp(file_name, "clipshare.conf"))
        {
            if (snprintf_check(tmp_fname, name_max_len, ".%c%s", PATH_SEP, file_name))
                return EXIT_FAILURE;
        }
        else
        {
            if (snprintf_check(tmp_fname, name_max_len, ".%c1_%s", PATH_SEP, file_name)) // do not create file named clipshare.conf
                return EXIT_FAILURE;
        }
        int n = 1;
        while (file_exists(tmp_fname))
        {
            if (n > 999999999)
                return EXIT_FAILURE;
            if (snprintf_check(tmp_fname, name_max_len, ".%c%i_%s", PATH_SEP, n++, file_name))
                return EXIT_FAILURE;
        }
        strncpy(file_name, tmp_fname, name_max_len);
        file_name[name_max_len] = 0;
    }

    FILE *file = fopen(file_name, "wb");
    if (!file)
        return EXIT_FAILURE;
    char data[FILE_BUF_SZ];
    while (file_size)
    {
        size_t read_len = file_size < FILE_BUF_SZ ? file_size : FILE_BUF_SZ;
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
        file_size -= read_len;
    }

    fclose(file);
#ifdef DEBUG_MODE
    puts("file saved");
#endif
    return EXIT_SUCCESS;
}

int get_image_v1(socket_t *socket)
{
    size_t length = 0;
    char *buf = NULL;
    if (get_image(&buf, &length) == EXIT_FAILURE || length == 0 || length > MAX_IMAGE_SIZE) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("get image failed. len = %zu\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf)
            free(buf);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %zu\n", length);
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
    return EXIT_SUCCESS;
}

int info_v1(socket_t *socket)
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
    return EXIT_SUCCESS;
}

int get_files_v2(socket_t *socket)
{
    dir_files copied_dir_files = get_copied_dirs_files();
    list2 *file_list = copied_dir_files.lst;
    size_t path_len = copied_dir_files.path_len;
    if (!file_list)
    {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        return EXIT_SUCCESS;
    }

    int status = EXIT_FAILURE;
    size_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%zu file(s)\n", file_cnt);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        goto END;

    if (send_size(socket, file_cnt) == EXIT_FAILURE)
        goto END;

    status = EXIT_SUCCESS;
    for (size_t i = 0; i < file_cnt; i++)
    {
        char *file_path = files[i];
#ifdef DEBUG_MODE
        printf("file name = %s\n", file_path);
#endif

        char *tmp_fname = file_path + path_len;
        size_t _tmp_len = strlen(tmp_fname);
        char filename[_tmp_len + 1];
        strncpy(filename, tmp_fname, _tmp_len);
        filename[_tmp_len] = 0;
        size_t fname_len = strlen(filename);
        if (fname_len > MAX_FILE_NAME_LENGTH)
        {
            status = EXIT_FAILURE;
            goto END;
        }

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
        if (file_size < 0 || file_size > MAX_FILE_SIZE)
        {
#ifdef DEBUG_MODE
            printf("file size = %zi\n", file_size);
#endif
            fclose(fp);
            status = EXIT_FAILURE;
            continue;
        }

        if (send_size(socket, fname_len) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }

        // path separator is always / when communicating with the client
        if (PATH_SEP != '/')
        {
            for (size_t ind = 0; ind < fname_len; ind++)
            {
                if (filename[ind] == PATH_SEP)
                    filename[ind] = '/';
            }
        }

        if (write_sock(socket, filename, fname_len) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }
        if (send_size(socket, file_size) == EXIT_FAILURE)
        {
            fclose(fp);
            status = EXIT_FAILURE;
            goto END;
        }

        char data[FILE_BUF_SZ];
        while (file_size > 0)
        {
            size_t read = fread(data, 1, FILE_BUF_SZ, fp);
            if (read)
            {
                if (write_sock(socket, data, read) == EXIT_FAILURE)
                {
                    fclose(fp);
                    status = EXIT_FAILURE;
                    goto END;
                }
                file_size -= read;
            }
        }
        fclose(fp);
    }
END:
    free_list(file_list);
    return status;
}

static int save_file(socket_t *socket, const char *dirname)
{
    ssize_t name_length = read_size(socket);
#ifdef DEBUG_MODE
    printf("name_len = %zi\n", name_length);
#endif
    if (name_length < 0 || name_length > MAX_FILE_NAME_LENGTH) // limit file name length to 1024 chars
    {
        return EXIT_FAILURE;
    }

    char file_name[name_length + 1];
    if (read_sock(socket, file_name, name_length) != EXIT_SUCCESS)
    {
#ifdef DEBUG_MODE
        fputs("Read file name failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }

    file_name[name_length] = 0;

    // replace '/' with PATH_SEP
    if (PATH_SEP != '/')
    {
        for (ssize_t ind = 0; ind < name_length; ind++)
        {
            if (file_name[ind] == '/')
            {
                file_name[ind] = PATH_SEP;
                if (ind > 0 && file_name[ind - 1] == PATH_SEP)
                    return EXIT_FAILURE; // "//" in path not allowed
            }
        }
    }

    char new_path[name_length + 20];
    if (file_name[0] == PATH_SEP)
    {
        if (snprintf_check(new_path, name_length + 20, "%s%s", dirname, file_name))
            return EXIT_FAILURE;
    }
    else
    {
        if (snprintf_check(new_path, name_length + 20, "%s%c%s", dirname, PATH_SEP, file_name))
            return EXIT_FAILURE;
    }

    // path must not contain /../ (goto parent dir)
    {
        char bad_path[] = "/../";
        bad_path[0] = PATH_SEP;
        bad_path[3] = PATH_SEP;
        char *ptr = strstr(new_path, bad_path);
        if (ptr)
        {
            return EXIT_FAILURE;
        }
    }

    // make directories
    {
        char *base_name = strrchr(new_path, PATH_SEP);
        if (!base_name)
            return EXIT_FAILURE;
        *base_name = 0;
        if (mkdirs(new_path) != EXIT_SUCCESS)
        {
            return EXIT_FAILURE;
        }
        *base_name = PATH_SEP;
    }

    if (file_exists(new_path))
        return EXIT_FAILURE;

    ssize_t file_size = read_size(socket);
#ifdef DEBUG_MODE
    printf("data len = %zi\n", file_size);
#endif
    if (file_size < 0 || file_size > MAX_FILE_SIZE)
        return EXIT_FAILURE;

    FILE *file = fopen(new_path, "wb");
    if (!file)
        return EXIT_FAILURE;
    char data[FILE_BUF_SZ];
    while (file_size)
    {
        size_t read_len = file_size < FILE_BUF_SZ ? file_size : FILE_BUF_SZ;
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
        file_size -= read_len;
    }

    fclose(file);

#ifdef DEBUG_MODE
    printf("file saved : %s\n", new_path);
#endif
    return EXIT_SUCCESS;
}

int send_files_v2(socket_t *socket)
{
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE)
        return EXIT_FAILURE;
    ssize_t cnt = read_size(socket);
    if (cnt <= 0)
        return EXIT_FAILURE;
    char dirname[17];
    unsigned id = (unsigned)time(NULL);
    do
    {
        if (snprintf_check(dirname, 17, "./%x", id))
            return EXIT_FAILURE;
        id = (unsigned)rand();
    } while (file_exists(dirname));

    if (mkdirs(dirname) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    for (ssize_t file_num = 0; file_num < cnt; file_num++)
    {
        if (save_file(socket, dirname) != EXIT_SUCCESS)
        {
            return EXIT_FAILURE;
        }
    }
    list2 *files = list_dir(dirname);
    if (!files)
        return EXIT_FAILURE;
    int status = EXIT_SUCCESS;
    for (size_t i = 0; i < files->len; i++)
    {
        char *filename = files->array[i];
        size_t name_len = strlen(filename);
        char old_path[name_len + 20];
        if (snprintf_check(old_path, name_len + 20, "%s%c%s", dirname, PATH_SEP, filename))
        {
            status = EXIT_FAILURE;
            continue;
        }
        char new_path[name_len + 20];
        if (configuration.working_dir != NULL || strcmp(filename, "clipshare.conf"))
        {
            if (snprintf_check(new_path, name_len + 20, ".%c%s", PATH_SEP, filename)) // "./" is important to prevent file names like "C:\path"
            {
                status = EXIT_FAILURE;
                continue;
            }
        }
        else
        {
            if (snprintf_check(new_path, name_len + 20, ".%c1_%s", PATH_SEP, filename)) // do not create file named clipshare.conf. "./" is important to prevent file names like "C:\path"
            {
                status = EXIT_FAILURE;
                continue;
            }
        }

        // if new_path already exists, use a different file name
        int n = 1;
        while (file_exists(new_path))
        {
            if (n > 999999999)
                return EXIT_FAILURE;
            if (snprintf_check(new_path, name_len + 20, ".%c%i_%s", PATH_SEP, n++, filename))
            {
                status = EXIT_FAILURE;
                continue;
            }
        }

        if (rename(old_path, new_path))
        {
            status = EXIT_FAILURE;
#ifdef DEBUG_MODE
            printf("Rename failed : %s\n", new_path);
#endif
        }
    }
    free_list(files);
    if (status == EXIT_SUCCESS && rmdir(dirname))
        status = EXIT_FAILURE;
    return status;
}
