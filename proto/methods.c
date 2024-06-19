/*
 * proto/methods.c - platform independent implementation of methods
 * Copyright (C) 2022-2024 H. Thevindu J. Wijesekera
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

#include <globals.h>
#include <proto/methods.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#ifndef __GLIBC__
#define __GLIBC__ 0
#endif
#ifndef __NO_INLINE__
#define __NO_INLINE__
#endif
#include <unistr.h>

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2

#define FILE_BUF_SZ 65536  // 64 KiB
#define MAX_FILE_NAME_LENGTH 2048
#define MAX_IMAGE_SIZE 1073741824  // 1 GiB

#define MIN(x, y) (x < y ? x : y)

/*
 * Check if path contains /../ (goto parent dir)
 */
static inline int _check_path(const char *path) {
    char bad_path[] = "/../";
    bad_path[0] = PATH_SEP;
    bad_path[3] = PATH_SEP;
    const char *ptr = strstr(path, bad_path);
    if (ptr) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 * Common function to get files in v1 and v2.
 */
static int _get_files_common(int version, socket_t *socket, list2 *file_list, size_t path_len);

/*
 * Common function to save files in send_files method of v1, v2, and v3.
 */
static int _save_file_common(int version, socket_t *socket, const char *file_name);

/*
 * Common function to get image in v1 and v3.
 */
static inline int _get_image_common(socket_t *socket, int mode, int disp);

/*
 * Check if the file name is valid.
 * A file name is valid only if it's a valid UTF-8 non-empty string, and contains no invalid characters \x00 to \x1f.
 * returns EXIT_SUCCESS if the file name is valid.
 * Otherwise, returns EXIT_FAILURE.
 */
static inline int _is_valid_fname(const char *fname, size_t name_length);

static int _transfer_single_file(int version, socket_t *socket, const char *file_path, size_t path_len);

int get_text_v1(socket_t *socket) {
    size_t length = 0;
    char *buf = NULL;
    if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0 ||
        length > configuration.max_text_length) {  // do not change the order
#ifdef DEBUG_MODE
        printf("clipboard read text failed. len = %zu\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf) free(buf);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %zu\n", length);
    if (length < 1024) {
        fwrite(buf, 1, length, stdout);
        puts("");
    }
#endif
    int64_t new_len = convert_eol(&buf, 1);
    if (new_len <= 0) {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        return EXIT_SUCCESS;
    }
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (send_size(socket, new_len) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (write_sock(socket, buf, (uint64_t)new_len) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    free(buf);
    return EXIT_SUCCESS;
}

int send_text_v1(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) return EXIT_FAILURE;
    int64_t length;
    if (read_size(socket, &length) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("Len = %zi\n", (ssize_t)length);
#endif
    // limit maximum length to 4 MiB
    if (length <= 0 || length > configuration.max_text_length) return EXIT_FAILURE;

    char *data = malloc((uint64_t)length + 1);
    if (read_sock(socket, data, (uint64_t)length) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
        fputs("Read data failed\n", stderr);
#endif
        free(data);
        return EXIT_FAILURE;
    }
    data[length] = 0;
    if (u8_check((uint8_t *)data, (size_t)length)) {
#ifdef DEBUG_MODE
        fputs("Invalid UTF-8\n", stderr);
#endif
        free(data);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    if (length < 1024) puts(data);
#endif
    length = convert_eol(&data, 0);
    if (length < 0) return EXIT_FAILURE;
    close_socket(socket);
    put_clipboard_text(data, (size_t)length);
    free(data);
    return EXIT_SUCCESS;
}

static int _transfer_regular_file(socket_t *socket, const char *file_path, const char *filename, size_t fname_len) {
    FILE *fp = open_file(file_path, "rb");
    if (!fp) {
        error("Couldn't open some files");
        return EXIT_FAILURE;
    }
    int64_t file_size = get_file_size(fp);
    if (file_size < 0 || file_size > configuration.max_file_size) {
#ifdef DEBUG_MODE
        printf("file size = %lli\n", (long long)file_size);
#endif
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (send_size(socket, (int64_t)fname_len) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (write_sock(socket, filename, fname_len) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }
    if (send_size(socket, file_size) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size > 0) {
        size_t read = fread(data, 1, FILE_BUF_SZ, fp);
        if (read == 0) continue;
        if (write_sock(socket, data, read) == EXIT_FAILURE) {
            fclose(fp);
            return EXIT_FAILURE;
        }
        file_size -= (ssize_t)read;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

#if PROTOCOL_MAX >= 3
static int _transfer_directory(socket_t *socket, const char *filename, size_t fname_len) {
    if (send_size(socket, (int64_t)fname_len) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (write_sock(socket, filename, fname_len) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    if (send_size(socket, -1) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
#endif

static int _transfer_single_file(int version, socket_t *socket, const char *file_path, size_t path_len) {
    const char *tmp_fname;
    switch (version) {
#if PROTOCOL_MIN <= 1
        case 1: {
            tmp_fname = strrchr(file_path, PATH_SEP);
            if (tmp_fname == NULL) {
                tmp_fname = file_path;
            } else {
                tmp_fname++;  // remove '/'
            }
            break;
        }
#endif
#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)
        case 2:
        case 3: {
            tmp_fname = file_path + path_len;
            break;
        }
#else
        (void)path_len;
#endif
        default: {
            return EXIT_FAILURE;
        }
    }

    const size_t _tmp_len = strnlen(tmp_fname, MAX_FILE_NAME_LENGTH);
    if (_tmp_len > MAX_FILE_NAME_LENGTH) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char filename[_tmp_len + 1];
    strncpy(filename, tmp_fname, _tmp_len);
    filename[_tmp_len] = 0;
    const size_t fname_len = strnlen(filename, _tmp_len);
    if (fname_len > MIN(_tmp_len, MAX_FILE_NAME_LENGTH)) {
        return EXIT_FAILURE;
    }

#if PATH_SEP != '/'
    if (version > 1) {
        // path separator is always / when communicating with the client
        for (size_t ind = 0; ind < fname_len; ind++) {
            if (filename[ind] == PATH_SEP) filename[ind] = '/';
        }
    }
#endif

#if PROTOCOL_MAX >= 3
    if (filename[fname_len - 1] == '/') {  // filename is converted to have / as path separator on all platforms
        filename[fname_len - 1] = 0;
        return _transfer_directory(socket, filename, fname_len - 1);
    }
#endif
    return _transfer_regular_file(socket, file_path, filename, fname_len);
}

static int _get_files_common(int version, socket_t *socket, list2 *file_list, size_t path_len) {
    if (!file_list || file_list->len == 0) {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (file_list) free_list(file_list);
        return EXIT_SUCCESS;
    }

    size_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%zu file(s)\n", file_cnt);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) {
        free_list(file_list);
        return EXIT_FAILURE;
    }

    if (send_size(socket, (int64_t)file_cnt) == EXIT_FAILURE) {
        free_list(file_list);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < file_cnt; i++) {
        const char *file_path = files[i];
#ifdef DEBUG_MODE
        printf("file name = %s\n", file_path);
#endif

        if (_transfer_single_file(version, socket, file_path, path_len) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            puts("Transfer failed");
#endif
            free_list(file_list);
            return EXIT_FAILURE;
        }
    }
    free_list(file_list);
    return EXIT_SUCCESS;
}

static int _save_file_common(int version, socket_t *socket, const char *file_name) {
    int64_t file_size;
    if (read_size(socket, &file_size) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("data len = %lli\n", (long long)file_size);
#endif
    if (file_size > configuration.max_file_size) {
        return EXIT_FAILURE;
    }

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
    if (file_size == -1 && version == 3) {
        return mkdirs(file_name);
    }
#else
    (void)version;
#endif
    if (file_size < 0) {
        return EXIT_FAILURE;
    }

    FILE *file = open_file(file_name, "wb");
    if (!file) {
        error("Couldn't create some files");
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size) {
        size_t read_len = file_size < FILE_BUF_SZ ? (size_t)file_size : FILE_BUF_SZ;
        if (read_sock(socket, data, read_len) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
            puts("recieve error");
#endif
            fclose(file);
            remove_file(file_name);
            return EXIT_FAILURE;
        }
        if (fwrite(data, 1, read_len, file) < read_len) {
            fclose(file);
            remove_file(file_name);
            return EXIT_FAILURE;
        }
        file_size -= (int64_t)read_len;
    }

    fclose(file);

#ifdef DEBUG_MODE
    printf("file saved : %s\n", file_name);
#endif
    return EXIT_SUCCESS;
}

static inline int _is_valid_fname(const char *fname, size_t name_length) {
    if (u8_check((const uint8_t *)fname, name_length)) {
#ifdef DEBUG_MODE
        fputs("Invalid UTF-8\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    do {
        if ((unsigned char)*fname < 32) return EXIT_FAILURE;
        fname++;
    } while (*fname);
    return EXIT_SUCCESS;
}

#if PROTOCOL_MIN <= 1
/*
 * Get only the base name.
 * Path seperator is assumed to be '/' regardless of the platform.
 */
static inline int _get_base_name(char *file_name, size_t name_length) {
    const char *base_name = strrchr(file_name, '/');  // path separator is / when communicating with the client
    if (base_name) {
        base_name++;                                                         // don't want the '/' before the file name
        memmove(file_name, base_name, strnlen(base_name, name_length) + 1);  // overlapping memory area
    }
    return EXIT_SUCCESS;
}

/*
 * Change file name if exists
 */
static inline int _rename_if_exists(char *file_name, size_t max_len) {
    char tmp_fname[max_len + 1];
    if (configuration.working_dir != NULL || strcmp(file_name, "clipshare.conf")) {
        if (snprintf_check(tmp_fname, max_len, ".%c%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    } else {
        // do not create file named clipshare.conf
        if (snprintf_check(tmp_fname, max_len, ".%c1_%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    }
    int n = 1;
    while (file_exists(tmp_fname)) {
        if (n > 999999) return EXIT_FAILURE;
        if (snprintf_check(tmp_fname, max_len, ".%c%i_%s", PATH_SEP, n++, file_name)) return EXIT_FAILURE;
    }
    strncpy(file_name, tmp_fname, max_len);
    file_name[max_len] = 0;
    return EXIT_SUCCESS;
}

int get_files_v1(socket_t *socket) {
    list2 *file_list = get_copied_files();
    return _get_files_common(1, socket, file_list, 0);
}

int send_file_v1(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) return EXIT_FAILURE;
    int64_t name_length;
    if (read_size(socket, &name_length) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("name_len = %zi\n", (ssize_t)name_length);
#endif
    // limit file name length to 1024 chars
    if (name_length <= 0 || name_length > MAX_FILE_NAME_LENGTH) return EXIT_FAILURE;

    const uint64_t name_max_len = (uint64_t)(name_length + 16);
    if (name_max_len > MAX_FILE_NAME_LENGTH) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char file_name[name_max_len + 1];
    if (read_sock(socket, file_name, (uint64_t)name_length) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
        fputs("Read file name failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    file_name[name_length] = 0;
    if (_is_valid_fname(file_name, (size_t)name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        printf("Invalid filename \'%s\'\n", file_name);
#endif
        return EXIT_FAILURE;
    }

    if (_get_base_name(file_name, (size_t)name_length) != EXIT_SUCCESS) return EXIT_FAILURE;

    // PATH_SEP is not allowed in file name
    if (strchr(file_name, PATH_SEP)) return EXIT_FAILURE;

    // if file already exists, use a different file name
    if (_rename_if_exists(file_name, name_max_len) != EXIT_SUCCESS) return EXIT_FAILURE;

    if (_save_file_common(1, socket, file_name) != EXIT_SUCCESS) return EXIT_FAILURE;
    close_socket(socket);

    int status = EXIT_SUCCESS;
    if (configuration.cut_sent_files) {
        char *path = (char *)malloc(cwd_len + name_max_len + 2);  // for PATH_SEP and null terminator
        strncpy(path, cwd, cwd_len + 1);
        size_t p_len = cwd_len;
        path[p_len++] = PATH_SEP;
        strncpy(path + p_len, file_name, name_max_len + 1);
        path[cwd_len + name_max_len + 1] = 0;
        list2 *files = init_list(1);
        append(files, path);
        status = set_clipboard_cut_files(files);
        free_list(files);
    }
    return status;
}
#endif

static inline int _get_image_common(socket_t *socket, int mode, int disp) {
    size_t length = 0;
    char *buf = NULL;
    if (get_image(&buf, &length, mode, disp) == EXIT_FAILURE || length == 0 ||
        length > MAX_IMAGE_SIZE) {  // do not change the order
#ifdef DEBUG_MODE
        printf("get image failed. len = %zu\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf) free(buf);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %zu\n", length);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (send_size(socket, (int64_t)length) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (write_sock(socket, buf, length) == EXIT_FAILURE) {
        free(buf);
        return EXIT_FAILURE;
    }
    free(buf);
    return EXIT_SUCCESS;
}

int get_image_v1(socket_t *socket) { return _get_image_common(socket, IMG_ANY, -1); }

int info_v1(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) return EXIT_FAILURE;
    const size_t len = sizeof(INFO_NAME) - 1;
    if (send_size(socket, (int64_t)len) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send length failed\n");
#endif
        return EXIT_FAILURE;
    }
    if (write_sock(socket, INFO_NAME, len) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send name failed\n");
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)
/*
 * Make parent directories for path
 */
static inline int _make_directories(const char *path) {
    char *base_name = strrchr(path, PATH_SEP);
    if (!base_name) return EXIT_FAILURE;
    *base_name = 0;
    if (mkdirs(path) != EXIT_SUCCESS) {
        *base_name = PATH_SEP;
        return EXIT_FAILURE;
    }
    *base_name = PATH_SEP;
    return EXIT_SUCCESS;
}

static int save_file(int version, socket_t *socket, const char *dirname) {
    int64_t fname_size;
    if (read_size(socket, &fname_size) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("name_len = %zi\n", (ssize_t)fname_size);
#endif
    // limit file name length to 1024 chars
    if (fname_size <= 0 || fname_size > MAX_FILE_NAME_LENGTH) return EXIT_FAILURE;

    const uint64_t name_length = (uint64_t)fname_size;
    char file_name[name_length + 1];
    if (read_sock(socket, file_name, name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read file name failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }

    file_name[name_length] = 0;
    if (_is_valid_fname(file_name, name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        printf("Invalid filename \'%s\'\n", file_name);
#endif
        return EXIT_FAILURE;
    }

#if PATH_SEP != '/'
    // replace '/' with PATH_SEP
    for (size_t ind = 0; ind < name_length; ind++) {
        if (file_name[ind] == '/') {
            file_name[ind] = PATH_SEP;
            if (ind > 0 && file_name[ind - 1] == PATH_SEP) return EXIT_FAILURE;  // "//" in path not allowed
        }
    }
#endif
    if (file_name[name_length - 1] == '/') file_name[name_length - 1] = 0;  // remove trailing /

    char new_path[name_length + 20];
    if (file_name[0] == PATH_SEP) {
        if (snprintf_check(new_path, name_length + 20, "%s%s", dirname, file_name)) return EXIT_FAILURE;
    } else {
        if (snprintf_check(new_path, name_length + 20, "%s%c%s", dirname, PATH_SEP, file_name)) return EXIT_FAILURE;
    }

    // path must not contain /../ (goto parent dir)
    if (_check_path(new_path) != EXIT_SUCCESS) return EXIT_FAILURE;

    // make parent directories
    if (_make_directories(new_path) != EXIT_SUCCESS) return EXIT_FAILURE;

    // check if file exists
    if (file_exists(new_path)) return EXIT_FAILURE;

    return _save_file_common(version, socket, new_path);
}

static char *_check_and_rename(const char *filename, const char *dirname) {
    const size_t name_len = strnlen(filename, MAX_FILE_NAME_LENGTH);
    if (name_len > MAX_FILE_NAME_LENGTH) {
        error("Too long file name.");
        return NULL;
    }
    const size_t name_max_len = name_len + 20;
    char old_path[name_max_len];
    if (snprintf_check(old_path, name_max_len, "%s%c%s", dirname, PATH_SEP, filename)) return NULL;

    char new_path[name_max_len];
    if (configuration.working_dir != NULL || strcmp(filename, "clipshare.conf")) {
        // "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c%s", PATH_SEP, filename)) return NULL;
    } else {
        // do not create file named clipshare.conf. "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c1_%s", PATH_SEP, filename)) return NULL;
    }

    // if new_path already exists, use a different file name
    int n = 1;
    while (file_exists(new_path)) {
        if (n > 999999) return NULL;
        if (snprintf_check(new_path, name_max_len, ".%c%i_%s", PATH_SEP, n++, filename)) return NULL;
    }

    if (rename_file(old_path, new_path)) {
#ifdef DEBUG_MODE
        printf("Rename failed : %s\n", new_path);
#endif
        return NULL;
    }

    char *path = (char *)malloc(cwd_len + name_max_len + 2);  // for PATH_SEP and null terminator
    strncpy(path, cwd, cwd_len + 1);
    size_t p_len = cwd_len;
    path[p_len++] = PATH_SEP;
    strncpy(path + p_len, new_path + 2, name_max_len + 1);  // +2 for ./ (new_path always starts with ./)
    path[cwd_len + name_max_len + 1] = 0;
    return path;
}

static int _send_files_dirs(int version, socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) return EXIT_FAILURE;
    int64_t cnt;
    if (read_size(socket, &cnt) != EXIT_SUCCESS) return EXIT_FAILURE;
    if (cnt <= 0) return EXIT_FAILURE;
    char dirname[17];
    unsigned id = (unsigned)time(NULL);
    do {
        if (snprintf_check(dirname, 17, ".%c%x", PATH_SEP, id)) return EXIT_FAILURE;
        id = (unsigned)rand();
    } while (file_exists(dirname));

    if (mkdirs(dirname) != EXIT_SUCCESS) return EXIT_FAILURE;

    for (int64_t file_num = 0; file_num < cnt; file_num++) {
        if (save_file(version, socket, dirname) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    close_socket(socket);
    list2 *files = list_dir(dirname);
    if (!files) return EXIT_FAILURE;
    int status = EXIT_SUCCESS;
    list2 *dest_files = init_list(files->len);
    if (!dest_files) {
        free_list(files);
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < files->len; i++) {
        const char *filename = files->array[i];
        char *new_path = _check_and_rename(filename, dirname);
        if (new_path)
            append(dest_files, new_path);
        else
            status = EXIT_FAILURE;
    }
    free_list(files);
    if (status == EXIT_SUCCESS && remove_directory(dirname)) status = EXIT_FAILURE;
    if (configuration.cut_sent_files) {
        if (status == EXIT_SUCCESS && set_clipboard_cut_files(dest_files) != EXIT_SUCCESS) status = EXIT_FAILURE;
    }
    free_list(dest_files);
    return status;
}

#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
int get_files_v2(socket_t *socket) {
    dir_files copied_dir_files;
    get_copied_dirs_files(&copied_dir_files, 0);
    return _get_files_common(2, socket, copied_dir_files.lst, copied_dir_files.path_len);
}

int send_files_v2(socket_t *socket) { return _send_files_dirs(2, socket); }
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
int get_copied_image_v3(socket_t *socket) { return _get_image_common(socket, IMG_COPIED_ONLY, -1); }

int get_screenshot_v3(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) == EXIT_FAILURE) return EXIT_FAILURE;
    int64_t disp;
    if (read_size(socket, &disp) != EXIT_SUCCESS) return EXIT_FAILURE;
    if (disp <= 0 || disp > 65536) disp = -1;
    return _get_image_common(socket, IMG_SCRN_ONLY, (int)disp);
}

int get_files_v3(socket_t *socket) {
    dir_files copied_dir_files;
    get_copied_dirs_files(&copied_dir_files, 1);
    return _get_files_common(2, socket, copied_dir_files.lst, copied_dir_files.path_len);
}

int send_files_v3(socket_t *socket) { return _send_files_dirs(3, socket); }
#endif
