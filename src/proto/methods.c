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

#ifdef DEBUG_MODE
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

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
#ifndef _LIBUNISTRING_NO_CONST_GENERICS
#define _LIBUNISTRING_NO_CONST_GENERICS
#endif
#include <unistr.h>

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2

#define FILE_BUF_SZ 65536L           // 64 KiB
#define MAX_IMAGE_SIZE 1073741824UL  // 1 GiB

#define MIN(x, y) (x < y ? x : y)

const char bad_path[] = {PATH_SEP, '.', '.', PATH_SEP, '\0'};  // /../

/*
 * Send a data buffer to the peer.
 * Sends the length first and then the data buffer.
 */
static inline int _send_data(socket_t *socket, int64_t length, const char *data) {
    if (send_size(socket, length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send length failed\n");
#endif
        return EXIT_FAILURE;
    }
    if (length < 0 || write_sock(socket, data, (uint64_t)length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send data failed\n");
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 * Common function to get files in v1 and v2.
 */
static int _get_files_common(int version, socket_t *socket, list2 *file_list, size_t path_len);

/*
 * Common function to save files.
 */
static int _save_file_common(int version, socket_t *socket, const char *file_name);

/*
 * Common function to get image in v1 and v3.
 */
static inline int _get_image_common(socket_t *socket, int mode, uint16_t disp);

/*
 * Check if the file name is valid.
 * A file name is valid only if it's a valid UTF-8 non-empty string, and contains no invalid characters \x00 to \x1f.
 * returns EXIT_SUCCESS if the file name is valid.
 * Otherwise, returns EXIT_FAILURE.
 */
static inline int _is_valid_fname(const char *fname, size_t name_length);

static int _transfer_single_file(int version, socket_t *socket, const char *file_path, size_t path_len);

int get_text_v1(socket_t *socket) {
    uint32_t length = 0;
    char *buf = NULL;
    if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0 ||
        length > configuration.max_text_length) {  // do not change the order
#ifdef DEBUG_MODE
        printf("clipboard read text failed. len = %" PRIu32 "\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf) free(buf);
        close_socket_no_wait(socket);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %" PRIu32 "\n", length);
    if (length < 1024) {
        fwrite(buf, 1, length, stdout);
        puts("");
    }
#endif
    int64_t new_len = convert_eol(&buf, 1);
    if (new_len <= 0 || !buf) {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        return EXIT_FAILURE;
    }
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (_send_data(socket, new_len, buf) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    free(buf);
    return EXIT_SUCCESS;
}

int send_text_v1(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) return EXIT_FAILURE;
    int64_t length;
    if (read_size(socket, &length) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("Len = %" PRIi64 "\n", length);
#endif
    // limit maximum length to max_text_length
    if (length <= 0 || length > configuration.max_text_length) {
        return EXIT_FAILURE;
    }

    char *data = malloc((size_t)length + 1);
    if (read_sock(socket, data, (uint64_t)length) != EXIT_SUCCESS) {
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
    if (length <= 0 || !data) return EXIT_FAILURE;
    close_socket_no_wait(socket);
    put_clipboard_text(data, (uint32_t)length);
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
        printf("file size = %" PRIi64 "\n", file_size);
#endif
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (_send_data(socket, (int64_t)fname_len, filename) != EXIT_SUCCESS) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (send_size(socket, file_size) != EXIT_SUCCESS) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size > 0) {
        size_t read = fread(data, 1, FILE_BUF_SZ, fp);
        if (read == 0) continue;
        if (write_sock(socket, data, read) != EXIT_SUCCESS) {
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
    if (_send_data(socket, (int64_t)fname_len, filename) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (send_size(socket, -1) != EXIT_SUCCESS) {
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

    const size_t fname_len = strnlen(tmp_fname, MAX_FILE_NAME_LEN + 1);
    if (fname_len <= 0 || fname_len > MAX_FILE_NAME_LEN) {
        error("Invalid file name length.");
        return EXIT_FAILURE;
    }
    char filename[fname_len + 1];
    strncpy(filename, tmp_fname, fname_len);
    filename[fname_len] = 0;

#if (PATH_SEP != '/') && (PROTOCOL_MAX > 1)
    if (version > 1) {
        // path separator is always / when communicating
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
    if ((!file_list) || file_list->len == 0 || file_list->len > configuration.max_file_count) {
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (file_list) free_list(file_list);
        close_socket_no_wait(socket);
        return EXIT_FAILURE;
    }

    uint32_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%" PRIu32 "file(s)\n", file_cnt);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) {
        free_list(file_list);
        return EXIT_FAILURE;
    }

    if (send_size(socket, (int64_t)file_cnt) != EXIT_SUCCESS) {
        free_list(file_list);
        return EXIT_FAILURE;
    }

    for (uint32_t i = 0; i < file_cnt; i++) {
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
    if (read_size(socket, &file_size) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("data len = %" PRIi64 "\n", file_size);
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
        if (read_sock(socket, data, read_len) != EXIT_SUCCESS) {
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
    const char *base_name = strrchr(file_name, '/');  // path separator is / when communicating
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
    if (configuration.working_dir != NULL || strcmp(file_name, CONFIG_FILE)) {
        if (snprintf_check(tmp_fname, max_len, ".%c%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    } else {
        // do not create file named clipshare.conf
        if (snprintf_check(tmp_fname, max_len, ".%c1_%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    }
    int n = 1;
    while (file_exists(tmp_fname)) {
        if (n > 999999L) return EXIT_FAILURE;
        if (snprintf_check(tmp_fname, max_len, ".%c%i_%s", PATH_SEP, n, file_name)) return EXIT_FAILURE;
        n++;
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
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) return EXIT_FAILURE;
    int64_t name_length;
    if (read_size(socket, &name_length) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("name_len = %zi\n", (ssize_t)name_length);
#endif
    // limit file name length to 1024 chars
    if (name_length <= 0 || name_length > MAX_FILE_NAME_LEN) return EXIT_FAILURE;

    const size_t name_max_len = (size_t)(name_length + 16);
    if (name_max_len > MAX_FILE_NAME_LEN) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char file_name[name_max_len + 1];
    if (read_sock(socket, file_name, (uint64_t)name_length) != EXIT_SUCCESS) {
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
    close_socket_no_wait(socket);

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

static inline int _get_image_common(socket_t *socket, int mode, uint16_t disp) {
    uint32_t length = 0;
    char *buf = NULL;
    if (get_image(&buf, &length, mode, disp) != EXIT_SUCCESS || length == 0 ||
        length > MAX_IMAGE_SIZE) {  // do not change the order
#ifdef DEBUG_MODE
        printf("get image failed. len = %" PRIu32 "\n", length);
#endif
        write_sock(socket, &(char){STATUS_NO_DATA}, 1);
        if (buf) free(buf);
        close_socket_no_wait(socket);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %" PRIu32 "\n", length);
#endif
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (_send_data(socket, (int64_t)length, buf) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    free(buf);
    return EXIT_SUCCESS;
}

int get_image_v1(socket_t *socket) { return _get_image_common(socket, IMG_ANY, 0); }

int info_v1(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) return EXIT_FAILURE;
    const size_t len = sizeof(INFO_NAME) - 1;
    if (_send_data(socket, (int64_t)len, INFO_NAME) != EXIT_SUCCESS) {
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
    if (read_size(socket, &fname_size) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("name_len = %" PRIi64 "\n", fname_size);
#endif
    // limit file name length to MAX_FILE_NAME_LEN chars
    if (fname_size <= 0 || fname_size > MAX_FILE_NAME_LEN) {
        return EXIT_FAILURE;
    }

    const size_t name_length = (size_t)fname_size;
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
    if (file_name[name_length - 1] == '/') file_name[name_length - 1] = 0;  // remove trailing /

#if PATH_SEP != '/'
    // replace '/' with PATH_SEP
    for (size_t ind = 0; ind < name_length; ind++) {
        if (file_name[ind] == '/') {
            file_name[ind] = PATH_SEP;
            if (ind > 0 && file_name[ind - 1] == PATH_SEP) {  // "//" in path not allowed
                return EXIT_FAILURE;
            }
        }
    }
#endif

    char new_path[name_length + 20];
    if (file_name[0] == PATH_SEP) {
        if (snprintf_check(new_path, name_length + 20, "%s%s", dirname, file_name)) return EXIT_FAILURE;
    } else {
        if (snprintf_check(new_path, name_length + 20, "%s%c%s", dirname, PATH_SEP, file_name)) return EXIT_FAILURE;
    }

    // path must not contain /../ (go to parent dir)
    if (strstr(new_path, bad_path)) return EXIT_FAILURE;

    // make parent directories
    if (_make_directories(new_path) != EXIT_SUCCESS) return EXIT_FAILURE;

    // check if file exists
    if (file_exists(new_path)) return EXIT_FAILURE;

    return _save_file_common(version, socket, new_path);
}

static char *_check_and_rename(const char *filename, const char *dirname) {
    const size_t name_len = strnlen(filename, MAX_FILE_NAME_LEN + 1);
    if (name_len > MAX_FILE_NAME_LEN) {
        error("Too long file name.");
        return NULL;
    }
    const size_t name_max_len = name_len + 20;
    char old_path[name_max_len];
    if (snprintf_check(old_path, name_max_len, "%s%c%s", dirname, PATH_SEP, filename)) return NULL;

    char new_path[name_max_len];
    if (configuration.working_dir != NULL || strcmp(filename, CONFIG_FILE)) {
        // "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c%s", PATH_SEP, filename)) return NULL;
    } else {
        // do not create file named clipshare.conf. "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c1_%s", PATH_SEP, filename)) return NULL;
    }

    // if new_path already exists, use a different file name
    int n = 1;
    while (file_exists(new_path)) {
        if (n > 999999L) return NULL;
        if (snprintf_check(new_path, name_max_len, ".%c%i_%s", PATH_SEP, n, filename)) return NULL;
        n++;
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

static inline list2 *clean_temp_dir(const char *tmp_dir, list2 *files) {
    list2 *dest_files = init_list(files->len);
    if (!dest_files) {
        return NULL;
    }

    int status = EXIT_SUCCESS;
    for (uint32_t i = 0; i < files->len; i++) {
        const char *filename = files->array[i];
        char *new_path = _check_and_rename(filename, tmp_dir);
        if (!new_path) {
            status = EXIT_FAILURE;
            continue;
        }
        append(dest_files, new_path);
    }
    if (status == EXIT_SUCCESS && remove_directory(tmp_dir)) {
        status = EXIT_FAILURE;
    }
    if (status == EXIT_SUCCESS) {
        return dest_files;
    }
    free_list(dest_files);
    return NULL;
}

static int _send_files_dirs(int version, socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) return EXIT_FAILURE;
    int64_t cnt;
    if (read_size(socket, &cnt) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (cnt <= 0 || (uint64_t)cnt > configuration.max_file_count) {
        return EXIT_FAILURE;
    }
    char dirname[17];
    unsigned id = (unsigned)time(NULL);
    do {
        if (snprintf_check(dirname, 17, ".%c%x", PATH_SEP, id)) return EXIT_FAILURE;
        id = (unsigned)rand();
    } while (file_exists(dirname));

    if (mkdirs(dirname) != EXIT_SUCCESS) return EXIT_FAILURE;

    int status = EXIT_SUCCESS;
    for (int64_t file_num = 0; file_num < cnt; file_num++) {
        if (save_file(version, socket, dirname) != EXIT_SUCCESS) {
            status = EXIT_FAILURE;
            break;
        }
    }
    close_socket_no_wait(socket);

    list2 *files = list_dir(dirname);
    if (!files) {
        status = EXIT_FAILURE;
    } else if (files->len == 0) {
        status = EXIT_FAILURE;
        remove_directory(dirname);
    }
    list2 *dest_files = NULL;
    if (status == EXIT_SUCCESS) {
        dest_files = clean_temp_dir(dirname, files);
    }
    free_list(files);
    if (dest_files) {
        if (configuration.cut_sent_files && set_clipboard_cut_files(dest_files) != EXIT_SUCCESS) {
            status = EXIT_FAILURE;
        }
        free_list(dest_files);
    }
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
int get_copied_image_v3(socket_t *socket) { return _get_image_common(socket, IMG_COPIED_ONLY, 0); }

int get_screenshot_v3(socket_t *socket) {
    if (write_sock(socket, &(char){STATUS_OK}, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    int64_t disp;
    if (read_size(socket, &disp) != EXIT_SUCCESS) return EXIT_FAILURE;
    if (disp <= 0 || disp > 65536L) disp = 0;
    return _get_image_common(socket, IMG_SCRN_ONLY, (uint16_t)disp);
}

int get_files_v3(socket_t *socket) {
    dir_files copied_dir_files;
    get_copied_dirs_files(&copied_dir_files, 1);
    return _get_files_common(3, socket, copied_dir_files.lst, copied_dir_files.path_len);
}

int send_files_v3(socket_t *socket) { return _send_files_dirs(3, socket); }
#endif
