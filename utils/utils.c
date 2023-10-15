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
#include <utils/win_image.h>
#include <windows.h>
#endif

#include <globals.h>
#include <utils/list_utils.h>
#include <utils/utils.h>
#include <xclip/xclip.h>
#include <xscreenshot/xscreenshot.h>

#define MAX(x, y) (x > y ? x : y)

#define ERROR_LOG_FILE "server_err.log"
#define MAX_RECURSE_DEPTH 256

#ifdef __linux__
static int url_decode(char *);
static char *get_copied_files_as_str(void);
#elif _WIN32
static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, int *wlen_p);
static int wchar_to_utf8_str(const wchar_t *wstr, char **utf8str_p, int *len_p);
static inline void _wappend(list2 *lst, const wchar_t *wstr);
#endif

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
    for (unsigned int i = 0; i < 4; i++) {
        if (f != NULL) break;
        struct timespec interval = {.tv_sec = 0, .tv_nsec = (long)(1 + i * 50) * 1000000l};
        if (nanosleep(&interval, NULL)) break;
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
    int f_ok;
#ifdef __linux__
    f_ok = access(file_name, F_OK);
#elif _WIN32
    wchar_t *wfname;
    if (utf8_to_wchar_str(file_name, &wfname, NULL) != EXIT_SUCCESS) return -1;
    f_ok = _waccess(wfname, F_OK);
    free(wfname);
#endif
    return f_ok == 0;
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
    int stat_result;
#ifdef __linux__
    if (follow_symlinks) {
        stat_result = stat(path, &sb);
    } else {
        stat_result = lstat(path, &sb);
    }
#elif _WIN32
    (void)follow_symlinks;
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    stat_result = wstat(wpath, &sb);
    free(wpath);
#endif
    if (stat_result == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

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
        char *new_buf = realloc(p->buffer, p->capacity);
        if (!new_buf) free(p->buffer);
        p->buffer = new_buf;
    }

    if (!p->buffer) png_error(png_ptr, "Write Error");

    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size += length;
}

FILE *open_file(const char *filename, const char *mode) {
#ifdef __linux__
    return fopen(filename, mode);
#elif _WIN32
    wchar_t *wfname;
    wchar_t *wmode;
    if (utf8_to_wchar_str(filename, &wfname, NULL) != EXIT_SUCCESS) return NULL;
    if (utf8_to_wchar_str(mode, &wmode, NULL) != EXIT_SUCCESS) {
        free(wfname);
        return NULL;
    }
    FILE *fp = _wfopen(wfname, wmode);
    free(wfname);
    free(wmode);
    return fp;
#endif
}

/*
 * Allocate the required capacity for the string with EOL=CRLF including the terminating '\0'.
 * Assign the realloced string to *str_p and the length after conversion to *len_p without the terminating '\0'.
 * Returns 0 if all \n are preceded by \r (i.e. no conversion needed).
 * Returns 1 if conversion is needed and it will increase the length.
 * Returns -1 if realloc failed.
 */
static inline int _realloc_for_crlf(char **str_p, size_t *len_p) {
    char *str = *str_p;
    size_t increase = 0;
    size_t ind;
    for (ind = 0; str[ind]; ind++) {
        if (str[ind] == '\n' && (ind == 0 || str[ind - 1] != '\r')) {
            // needs increasing
            increase++;
        }
    }
    if (!increase) {
        *len_p = ind;
        return 0;
    }
    size_t req_len = ind + increase;
    char *re_str = realloc(str, req_len + 1);  // +1 for terminating '\0'
    if (!re_str) {
        free(str);
        error("realloc failed");
        return -1;
    }
    *str_p = re_str;
    *len_p = req_len;
    return 1;
}

static inline void _convert_to_crlf(char *str, size_t new_len) {
    // converting to CRLF expands string. Therefore, start from the end to avoid overwriting
    size_t new_ind = new_len;
    for (size_t cur_ind = strnlen(str, new_len + 1);; cur_ind--) {
        char c = str[cur_ind];
        str[new_ind--] = c;
        if (c == '\n' && (cur_ind == 0 || str[cur_ind - 1] != '\n')) {
            // add the missing \r before \n
            str[new_ind--] = '\r';
        }
        if (cur_ind == 0) break;
    }
}

static inline ssize_t _convert_to_lf(char *str) {
    // converting to CRLF shrinks string. Therefore, start from the begining to avoid overwriting
    char *old_ptr = str;
    char *new_ptr = str;
    char c;
    while ((c = *old_ptr)) {
        old_ptr++;
        if (c != '\r' || *old_ptr != '\n') {
            *new_ptr = c;
            new_ptr++;
        }
    }
    *new_ptr = '\0';
    return new_ptr - str;
}

ssize_t convert_eol(char **str_p, int force_lf) {
    int crlf;
#ifdef __linux__
    crlf = 0;
#elif _WIN32
    crlf = 1;
#endif
    if (force_lf) crlf = 0;
    // realloc if available capacity is not enough
    if (crlf) {
        size_t new_len;
        int status = _realloc_for_crlf(str_p, &new_len);
        if (status == 0) return (ssize_t)new_len;  // no conversion needed
        if (status < 0 || !*str_p) return -1;      // realloc failed
        _convert_to_crlf(*str_p, new_len);
        return (ssize_t)new_len;
    }
    return _convert_to_lf(*str_p);
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

int rename_file(const char *old_name, const char *new_name) {
#ifdef __linux__
    return rename(old_name, new_name);
#elif _WIN32
    wchar_t *wold;
    wchar_t *wnew;
    if (utf8_to_wchar_str(old_name, &wold, NULL) != EXIT_SUCCESS) return -1;
    if (utf8_to_wchar_str(new_name, &wnew, NULL) != EXIT_SUCCESS) {
        free(wold);
        return -1;
    }
    int result = _wrename(wold, wnew);
    free(wold);
    free(wnew);
    return result;
#endif
}

int remove_directory(const char *path) {
#ifdef __linux__
    return rmdir(path);
#elif _WIN32
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    int result = (RemoveDirectoryW(wpath) == FALSE);
    free(wpath);
    return result;
#endif
}

/*
 * Try to create the directory at path.
 * If the path points to an existing directory or a new directory was created successfuly, returns EXIT_SUCCESS.
 * If the path points to an existing file which is not a directory or creating a directory failed, returns
 * EXIT_FAILURE.
 */
static int _mkdir_check(const char *path);

static int _mkdir_check(const char *path) {
    if (file_exists(path)) {
        if (!is_directory(path, 0)) return EXIT_FAILURE;
    } else {
        int status;  // success=0 and failure=non-zero
#ifdef __linux__
        status = mkdir(path, S_IRWXU | S_IRWXG);
#elif _WIN32
        wchar_t *wpath;
        if (utf8_to_wchar_str(path, &wpath, NULL) == EXIT_SUCCESS) {
            status = CreateDirectoryW(wpath, NULL) != TRUE;
            free(wpath);
        } else {
            status = 1;
        }
#endif
        if (status) {
#ifdef DEBUG_MODE
            printf("Error creating directory %s\n", path);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int mkdirs(const char *dir_path) {
    if (dir_path[0] != '.') return EXIT_FAILURE;  // path must be relative and start with .

    if (file_exists(dir_path)) {
        if (is_directory(dir_path, 0))
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }

    size_t len = strnlen(dir_path, 2050);
    if (len > 2048) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char path[len + 1];
    strncpy(path, dir_path, len);
    path[len] = 0;

    for (size_t i = 0; i <= len; i++) {
        if (path[i] != PATH_SEP && path[i] != 0) continue;
        path[i] = 0;
        if (_mkdir_check(path) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        if (i < len) path[i] = PATH_SEP;
    }
    return EXIT_SUCCESS;
}

list2 *list_dir(const char *dirname) {
#ifdef __linux__
    DIR *d = opendir(dirname);
#elif _WIN32
    wchar_t *wdname;
    if (utf8_to_wchar_str(dirname, &wdname, NULL) != EXIT_SUCCESS) {
        return NULL;
    }
    _WDIR *d = _wopendir(wdname);
    free(wdname);
#endif
    if (!d) {
#ifdef DEBUG_MODE
        puts("Error opening directory");
#endif
        return NULL;
    }
    list2 *lst = init_list(2);
    if (!lst) return NULL;
    while (1) {
#ifdef __linux__
        const struct dirent *dir = readdir(d);
        const char *filename;
#elif _WIN32
        const struct _wdirent *dir = _wreaddir(d);
        const wchar_t *filename;
#endif
        if (dir == NULL) break;
        filename = dir->d_name;
#ifdef __linux__
        if (!(strcmp(filename, ".") && strcmp(filename, ".."))) continue;
        append(lst, strdup(filename));
#elif _WIN32
        if (!(wcscmp(filename, L".") && wcscmp(filename, L".."))) continue;
        char *utf8fname;
        if (wchar_to_utf8_str(filename, &utf8fname, NULL) != EXIT_SUCCESS) continue;
        append(lst, utf8fname);
#endif
    }
#ifdef __linux__
    (void)closedir(d);
#elif _WIN32
    (void)_wclosedir(d);
#endif
    return lst;
}

#ifdef __linux__

/*
 * Check if the path is a file or a directory.
 * If the path is a directory, calls recurse_dir() on that.
 * Otherwise, appends the path to the list
 */
static void _process_path(const char *path, list2 *lst, int depth);

/*
 * Recursively append all file paths in the directory and its subdirectories
 * to the list.
 * maximum recursion depth is limited to MAX_RECURSE_DEPTH
 */
static void recurse_dir(const char *_path, list2 *lst, int depth);

static void _process_path(const char *path, list2 *lst, int depth) {
    struct stat sb;
    if (lstat(path, &sb) != 0) return;
    if (S_ISDIR(sb.st_mode)) {
        recurse_dir(path, lst, depth + 1);
    } else if (S_ISREG(sb.st_mode)) {
        append(lst, strdup(path));
    }
}

static void recurse_dir(const char *_path, list2 *lst, int depth) {
    if (depth > MAX_RECURSE_DEPTH) return;
    DIR *d = opendir(_path);
    if (!d) {
#ifdef DEBUG_MODE
        printf("Error opening directory %s", _path);
#endif
        return;
    }
    size_t p_len = strnlen(_path, 2050);
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
        _process_path(pathname, lst, depth);
    }
    (void)closedir(d);
}

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

#elif _WIN32

/*
 * Check if the path is a file or a directory.
 * If the path is a directory, calls recurse_dir() on that.
 * Otherwise, appends the path to the list
 */
static void _process_path(const wchar_t *path, list2 *lst, int depth);

/*
 * Recursively append all file paths in the directory and its subdirectories
 * to the list.
 * maximum recursion depth is limited to MAX_RECURSE_DEPTH
 */
static void recurse_dir(const wchar_t *_path, list2 *lst, int depth);

static void _process_path(const wchar_t *path, list2 *lst, int depth) {
    struct stat sb;
    if (wstat(path, &sb) != 0) return;
    if (S_ISDIR(sb.st_mode)) {
        recurse_dir(path, lst, depth + 1);
    } else if (S_ISREG(sb.st_mode)) {
        _wappend(lst, path);
    }
}

static void recurse_dir(const wchar_t *_path, list2 *lst, int depth) {
    if (depth > MAX_RECURSE_DEPTH) return;
    _WDIR *d = _wopendir(_path);
    if (!d) {
#ifdef DEBUG_MODE
        wprintf(L"Error opening directory %s", _path);
#endif
        return;
    }
    size_t p_len = wcsnlen(_path, 2050);
    if (p_len > 2048) {
        error("Too long file name.");
        (void)_wclosedir(d);
        return;
    }
    wchar_t path[p_len + 2];
    wcsncpy(path, _path, p_len + 1);
    path[p_len + 1] = 0;
    if (path[p_len - 1] != PATH_SEP) {
        path[p_len++] = PATH_SEP;
        path[p_len] = '\0';
    }
    const struct _wdirent *dir;
    while ((dir = _wreaddir(d)) != NULL) {
        const wchar_t *filename = dir->d_name;
        if (!(wcscmp(filename, L".") && wcscmp(filename, L".."))) continue;
        const size_t _fname_len = wcslen(filename);
        if (_fname_len + p_len > 2048) {
            error("Too long file name.");
            (void)_wclosedir(d);
            return;
        }
        wchar_t pathname[_fname_len + p_len + 1];
        wcsncpy(pathname, path, p_len);
        wcsncpy(pathname + p_len, filename, _fname_len + 1);
        pathname[p_len + _fname_len] = 0;
        _process_path(pathname, lst, depth);
    }
    (void)_wclosedir(d);
}

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
    wchar_t fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = 0;
        DragQueryFileW(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributesW(fileName);
        DWORD dontWant = FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            wprintf(L"not a file or dir : %s\n", fileName);
#endif
            continue;
        }
        if (i == 0) {
            wchar_t *sep_ptr = wcsrchr(fileName, PATH_SEP);
            if (sep_ptr > fileName) {
                dfiles_p->path_len = (size_t)(sep_ptr - fileName + 1);
            }
        }
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            recurse_dir(fileName, lst, 1);
        } else {  // regular file
            _wappend(lst, fileName);
        }
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
}

#endif

#endif  // (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

#ifdef __linux__

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

#elif _WIN32

static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, int *wlen_p) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL, 0);
    if (wlen <= 0) return EXIT_FAILURE;
    wchar_t *wstr = malloc((size_t)wlen * sizeof(wchar_t));
    if (!wstr) return EXIT_FAILURE;
    MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wstr, wlen);
    wstr[wlen - 1] = 0;
    *wstr_p = wstr;
    if (wlen_p) *wlen_p = wlen - 1;
    return EXIT_SUCCESS;
}

static int wchar_to_utf8_str(const wchar_t *wstr, char **utf8str_p, int *len_p) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return EXIT_FAILURE;
    char *str = malloc((size_t)len);
    if (!str) return EXIT_FAILURE;
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    str[len - 1] = 0;
    *utf8str_p = str;
    if (len_p) *len_p = len - 1;
    return EXIT_SUCCESS;
}

/*
 * A wrapper to append() for wide strings.
 * Convert wchar_t * string to utf-8 and append to list
 */
static inline void _wappend(list2 *lst, const wchar_t *wstr) {
    char *utf8path;
    if (wchar_to_utf8_str(wstr, &utf8path, NULL) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        wprintf(L"Error while converting file path: %s\n", wstr);
#endif
        return;
    }
    append(lst, utf8path);
}

int get_clipboard_text(char **bufptr, size_t *lenptr) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        CloseClipboard();
        return EXIT_FAILURE;
    }
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    char *data;
    int len;
    if (wchar_to_utf8_str((wchar_t *)h, &data, &len) != EXIT_SUCCESS) {
        data = NULL;
        len = 0;
    }
    CloseClipboard();

    if (!data) {
#ifdef DEBUG_MODE
        fputs("clipboard data is null\n", stderr);
#endif
        *lenptr = 0;
        return EXIT_FAILURE;
    }
    *bufptr = data;
    *lenptr = (size_t)len;
    data[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    wchar_t *wstr;
    int wlen;
    char prev = data[len];
    data[len] = 0;
    if (utf8_to_wchar_str(data, &wstr, &wlen) != EXIT_SUCCESS) {
        data[len] = prev;
        CloseClipboard();
        return EXIT_FAILURE;
    }
    data[len] = prev;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (size_t)(wlen + 1) * sizeof(wchar_t));
    wcscpy_s(GlobalLock(hMem), (rsize_t)(wlen + 1), wstr);
    GlobalUnlock(hMem);
    free(wstr);
    EmptyClipboard();
    HANDLE res = SetClipboardData(CF_UNICODETEXT, hMem);
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

    wchar_t fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = '\0';
        DragQueryFileW(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributesW(fileName);
        DWORD dontWant =
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            wprintf(L"not a file : %s\n", fileName);
#endif
            continue;
        }
        _wappend(lst, fileName);
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
    return lst;
}

#endif
