/*
 *  utils/utils.c - platform specific implementation for utils
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

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>

#include "./win_image.h"
#endif

#include "../globals.h"
#include "../xclip/xclip.h"
#include "../xscreenshot/xscreenshot.h"
#include "./list_utils.h"
#include "./utils.h"

#define MAX(x, y) (x > y ? x : y)

#define ERROR_LOG_FILE "server_err.log"
#define RECURSE_DEPTH_MAX 256

__attribute__((__format__(gnu_printf, 3, 4))) int snprintf_check(char *dest, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return (ret < 0 || ret > (long)size);
}

void error(const char *msg) {
#ifdef DEBUG_MODE
    fprintf(stderr, "%s\n", msg);
#endif
    FILE *f = fopen(ERROR_LOG_FILE, "a");
    // retry with delays if failed
    for (unsigned int i = 0; (i < 4) && (f == NULL); i++) {
        if (usleep(1000 + i * 50000)) break;
        f = fopen(ERROR_LOG_FILE, "a");
    }
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
#ifdef __linux__
        chmod(ERROR_LOG_FILE, S_IWUSR | S_IWGRP | S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
#endif
    }
}

void error_exit(const char *msg) {
    error(msg);
    clear_config(&configuration);
    exit(1);
}

int file_exists(const char *file_name) {
    if (file_name[0] == 0) return 0;  // empty path
    return access(file_name, F_OK) == 0;
}

ssize_t get_file_size(FILE *fp) {
    struct stat statbuf;
    if (fstat(fileno(fp), &statbuf)) {
#ifdef DEBUG_MODE
        puts("fstat failed");
#endif
        return -1;
    }
    if (!S_ISREG(statbuf.st_mode)) {
#ifdef DEBUG_MODE
        puts("not a file");
#endif
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    ssize_t file_size = ftell(fp);
    rewind(fp);
    return file_size;
}

int is_directory(const char *path, int follow_symlinks) {
    if (path[0] == 0) return 0;  // empty path
    struct stat sb;
#ifdef __linux__
    int (*stat_fn)(const char *, struct stat *) = follow_symlinks ? &stat : &lstat;
    if (stat_fn(path, &sb) == 0) {
#elif _WIN32
    (void)follow_symlinks;
    if (stat(path, &sb) == 0) {
#endif
        if (S_ISDIR(sb.st_mode)) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
int mkdirs(const char *dir_path) {
    if (dir_path[0] != '.') return EXIT_FAILURE;  // path must be relative and start with .

    if (file_exists(dir_path)) {
        if (is_directory(dir_path, 0))
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }

    size_t len = strnlen(dir_path, 2047);
    if (len > 2048) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char path[len + 1];
    strncpy(path, dir_path, len);
    path[len] = 0;

    for (size_t i = 0; i <= len; i++) {
        if (path[i] == PATH_SEP || path[i] == 0) {
            path[i] = 0;
            if (file_exists(path)) {
                if (!is_directory(path, 0)) return EXIT_FAILURE;
            } else {
#ifdef __linux__
                if (mkdir(path, S_IRWXU | S_IRWXG)) {
#elif _WIN32
                if (mkdir(path)) {
#endif
#ifdef DEBUG_MODE
                    printf("Error creating directory %s\n", path);
#endif
                    path[i] = PATH_SEP;
                    return EXIT_FAILURE;
                }
            }
            if (i < len) path[i] = PATH_SEP;
        }
    }
    return EXIT_SUCCESS;
}

list2 *list_dir(const char *dirname) {
    DIR *d = opendir(dirname);
    if (d) {
        list2 *lst = init_list(2);
        if (!lst) return NULL;
        const struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            const char *filename = dir->d_name;
            if (!(strcmp(filename, ".") && strcmp(filename, ".."))) continue;
            append(lst, strdup(filename));
        }
        (void)closedir(d);
        return lst;
#ifdef DEBUG_MODE
    } else {
        puts("Error opening directory");
#endif
    }
    return NULL;
}
#endif

void png_mem_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    struct mem_file *p = (struct mem_file *)png_get_io_ptr(png_ptr);
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    if (p->buffer == NULL) {
        p->capacity = MAX(length, 1024);
        p->buffer = malloc(p->capacity);
    } else if (nsize > p->capacity) {
        p->capacity *= 2;
        p->capacity = MAX(nsize, p->capacity);
        p->buffer = realloc(p->buffer, p->capacity);
    }

    if (!p->buffer) png_error(png_ptr, "Write Error");

    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size += length;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
/*
 * Recursively append all file paths in the directory and its subdirectories
 * to the list.
 * maximum recursion depth is limited to RECURSE_DEPTH_MAX
 */
static void recurse_dir(const char *_path, list2 *lst, int depth) {
    if (depth > RECURSE_DEPTH_MAX) return;
    DIR *d = opendir(_path);
    if (d) {
        size_t p_len = strnlen(_path, 2047);
        if (p_len > 2048) {
            error("Too long file name.");
            (void)closedir(d);
            return;
        }
        char path[p_len + 2];
        strncpy(path, _path, p_len + 1);
        path[p_len + 1] = 0;
        if (path[p_len - 1] != PATH_SEP) {
            path[p_len++] = PATH_SEP;
            path[p_len] = '\0';
        }
        const struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            const char *filename = dir->d_name;
            if (!(strcmp(filename, ".") && strcmp(filename, ".."))) continue;
            const size_t _fname_len = strlen(filename);
            if (_fname_len + p_len > 2048) {
                error("Too long file name.");
                (void)closedir(d);
                return;
            }
            char pathname[_fname_len + p_len + 1];
            strncpy(pathname, path, p_len);
            strncpy(pathname + p_len, filename, _fname_len + 1);
            pathname[p_len + _fname_len] = 0;
            struct stat sb;
#ifdef __linux__
            if (lstat(pathname, &sb) == 0) {
#elif _WIN32
            if (stat(pathname, &sb) == 0) {
#endif
                if (S_ISDIR(sb.st_mode)) {
                    recurse_dir(pathname, lst, depth + 1);
                } else if (S_ISREG(sb.st_mode)) {
                    append(lst, strdup(pathname));
                }
            }
        }
        (void)closedir(d);
#ifdef DEBUG_MODE
    } else {
        puts("Error opening directory");
#endif
    }
}
#endif

#ifdef __linux__

static int url_decode(char *);
static char *get_copied_files_as_str(void);

int get_clipboard_text(char **buf_ptr, size_t *len_ptr) {
    *buf_ptr = NULL;
    if (xclip_util(XCLIP_OUT, NULL, len_ptr, buf_ptr) != EXIT_SUCCESS || *len_ptr <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read text failed. len = %zu\n", *len_ptr);
#endif
        if (*buf_ptr) free(*buf_ptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    if (xclip_util(XCLIP_IN, NULL, &len, &data) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Failed to write to clipboard\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int get_image(char **buf_ptr, size_t *len_ptr) {
    *buf_ptr = NULL;
    if (xclip_util(XCLIP_OUT, "image/png", len_ptr, buf_ptr) || *len_ptr == 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip failed to get image/png. len = %zu\nCapturing screenshot ...\n", *len_ptr);
#endif
        *buf_ptr = NULL;
        *len_ptr = 0;
        if (screenshot_util(len_ptr, buf_ptr) || *len_ptr == 0) {  // do not change the order
#ifdef DEBUG_MODE
            fputs("Get screenshot failed\n", stderr);
#endif
            if (*buf_ptr) free(*buf_ptr);
            *len_ptr = 0;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static inline char hex2char(char h) {
    if ('0' <= h && h <= '9') return (char)((int)h - '0');
    if ('A' <= h && h <= 'F') return (char)((int)h - 'A' + 10);
    if ('a' <= h && h <= 'f') return (char)((int)h - 'a' + 10);
    return -1;
}

static char *get_copied_files_as_str(void) {
    const char *const expected_target = "x-special/gnome-copied-files";
    char *targets;
    size_t targets_len;
    if (xclip_util(XCLIP_OUT, "TARGETS", &targets_len, &targets) || targets_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read TARGETS. len = %zu\n", targets_len);
#endif
        if (targets) free(targets);
        return NULL;
    }
    char found = 0;
    char *copy = targets;
    const char *token;
    while ((token = strsep(&copy, "\n"))) {
        if (!strcmp(token, expected_target)) {
            found = 1;
            break;
        }
    }
    if (!found) {
#ifdef DEBUG_MODE
        puts("No copied files");
#endif
        free(targets);
        return NULL;
    }
    free(targets);

    char *fnames = NULL;
    size_t fname_len;
    if (xclip_util(XCLIP_OUT, expected_target, &fname_len, &fnames) || fname_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read copied files. len = %zu\n", fname_len);
#endif
        if (fnames) free(fnames);
        return NULL;
    }
    fnames[fname_len] = 0;

    char *file_path = strchr(fnames, '\n');
    if (!file_path) {
        free(fnames);
        return NULL;
    }
    *file_path = 0;
    if (strcmp(fnames, "copy") && strcmp(fnames, "cut")) {
        free(fnames);
        return NULL;
    }
    return fnames;
}

static int url_decode(char *str) {
    if (strncmp("file://", str, 7)) return EXIT_FAILURE;
    char *ptr1 = str;
    const char *ptr2 = strstr(str, "://");
    if (!ptr2) return EXIT_FAILURE;
    ptr2 += 3;
    do {
        char c;
        if (*ptr2 == '%') {
            ptr2++;
            char tmp = *ptr2;
            char c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
            c = (char)(c1 << 4);
            ptr2++;
            tmp = *ptr2;
            c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
            c |= c1;
        } else {
            c = *ptr2;
        }
        *ptr1 = c;
        ptr1++;
        ptr2++;
    } while (*ptr2);
    *ptr1 = 0;
    return EXIT_SUCCESS;
}

list2 *get_copied_files(void) {
    char *fnames = get_copied_files_as_str();
    if (!fnames) {
        return NULL;
    }
    char *file_path = fnames + strnlen(fnames, 8);

    size_t file_cnt = 1;
    for (char *ptr = file_path + 1; *ptr; ptr++) {
        if (*ptr == '\n') {
            file_cnt++;
            *ptr = 0;
        }
    }

    list2 *lst = init_list(file_cnt);
    if (!lst) {
        free(fnames);
        return NULL;
    }
    char *fname = file_path + 1;
    for (size_t i = 0; i < file_cnt; i++) {
        size_t off = strnlen(fname, 2047) + 1;
        if (url_decode(fname) == EXIT_FAILURE) break;

        struct stat statbuf;
        if (stat(fname, &statbuf)) {
#ifdef DEBUG_MODE
            puts("stat failed");
#endif
            fname += off;
            continue;
        }
        if (!S_ISREG(statbuf.st_mode)) {
#ifdef DEBUG_MODE
            printf("not a file : %s\n", fname);
#endif
            fname += off;
            continue;
        }
        append(lst, strdup(fname));
        fname += off;
    }
    free(fnames);
    return lst;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
void get_copied_dirs_files(dir_files *dfiles_p) {
    dfiles_p->lst = NULL;
    dfiles_p->path_len = 0;
    char *fnames = get_copied_files_as_str();
    if (!fnames) {
        return;
    }
    char *file_path = fnames + strnlen(fnames, 8);

    size_t file_cnt = 1;
    for (char *ptr = file_path + 1; *ptr; ptr++) {
        if (*ptr == '\n') {
            file_cnt++;
            *ptr = 0;
        }
    }

    list2 *lst = init_list(file_cnt);
    if (!lst) {
        free(fnames);
        return;
    }
    dfiles_p->lst = lst;
    char *fname = file_path + 1;
    for (size_t i = 0; i < file_cnt; i++) {
        const size_t off = strnlen(fname, 2047) + 1;
        if (url_decode(fname) == EXIT_FAILURE) break;

        if (i == 0) {
            const char *sep_ptr = strrchr(fname, PATH_SEP);
            if (sep_ptr > fname) {
                dfiles_p->path_len = (size_t)sep_ptr - (size_t)fname + 1;
            }
        }

        struct stat statbuf;
        if (stat(fname, &statbuf)) {
#ifdef DEBUG_MODE
            puts("stat failed");
#endif
            fname += off;
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            recurse_dir(fname, lst, 1);
            fname += off;
            continue;
        }
        if (!S_ISREG(statbuf.st_mode)) {
#ifdef DEBUG_MODE
            printf("not a file : %s\n", fname);
#endif
            fname += off;
            continue;
        }
        append(lst, strdup(fname));
        fname += off;
    }
    free(fnames);
}
#endif

#elif _WIN32

int get_clipboard_text(char **bufptr, size_t *lenptr) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        CloseClipboard();
        return EXIT_FAILURE;
    }
    HANDLE h = GetClipboardData(CF_TEXT);
    char *data = strdup((char *)h);
    CloseClipboard();

    if (!data) {
#ifdef DEBUG_MODE
        fputs("clipboard data is null\n", stderr);
#endif
        *lenptr = 0;
        return EXIT_FAILURE;
    }
    *bufptr = data;
    *lenptr = strnlen(data, 4194304);
    data[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    memcpy(GlobalLock(hMem), data, len + 1);
    GlobalUnlock(hMem);
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    EmptyClipboard();
    HANDLE res = SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
    return (res == NULL ? EXIT_FAILURE : EXIT_SUCCESS);
}

int get_image(char **buf_ptr, size_t *len_ptr) {
    getCopiedImage(buf_ptr, len_ptr);
    if (*len_ptr > 8) return EXIT_SUCCESS;
    screenCapture(buf_ptr, len_ptr);
    if (*len_ptr > 8) return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

list2 *get_copied_files(void) {
    if (!OpenClipboard(0)) return NULL;
    if (!IsClipboardFormatAvailable(CF_HDROP)) {
        CloseClipboard();
        return NULL;
    }
    HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
    if (!hGlobal) {
        CloseClipboard();
        return NULL;
    }
    HDROP hDrop = (HDROP)GlobalLock(hGlobal);
    if (!hDrop) {
        CloseClipboard();
        return NULL;
    }

    size_t file_cnt = DragQueryFile(hDrop, (UINT)(-1), NULL, MAX_PATH);

    if (file_cnt <= 0) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return NULL;
    }
    list2 *lst = init_list(file_cnt);
    if (!lst) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return NULL;
    }

    char fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = '\0';
        DragQueryFile(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributes(fileName);
        DWORD dontWant =
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            printf("not a file : %s\n", fileName);
#endif
            continue;
        }
        append(lst, strdup(fileName));
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
    return lst;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
void get_copied_dirs_files(dir_files *dfiles_p) {
    dfiles_p->lst = NULL;
    dfiles_p->path_len = 0;

    if (!OpenClipboard(0)) return;
    if (!IsClipboardFormatAvailable(CF_HDROP)) {
        CloseClipboard();
        return;
    }
    HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
    if (!hGlobal) {
        CloseClipboard();
        return;
    }
    HDROP hDrop = (HDROP)GlobalLock(hGlobal);
    if (!hDrop) {
        CloseClipboard();
        return;
    }

    size_t file_cnt = DragQueryFile(hDrop, (UINT)(-1), NULL, MAX_PATH);

    if (file_cnt <= 0) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return;
    }
    list2 *lst = init_list(file_cnt);
    if (!lst) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return;
    }
    dfiles_p->lst = lst;
    char fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = '\0';
        DragQueryFile(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributes(fileName);
        DWORD dontWant = FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            printf("not a file or dir : %s\n", fileName);
#endif
            continue;
        }
        if (i == 0) {
            char *sep_ptr = strrchr(fileName, PATH_SEP);
            if (sep_ptr > fileName) {
                dfiles_p->path_len = (size_t)(sep_ptr - fileName + 1);
            }
        }
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            recurse_dir(fileName, lst, 1);
        } else {  // regular file
            append(lst, strdup(fileName));
        }
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
}
#endif

#endif
