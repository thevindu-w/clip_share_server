/*
 * utils/utils.h - header for utils
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

#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <globals.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <utils/list_utils.h>

#if defined(__linux__) || defined(_WIN32)
#include <libpng16/png.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#define PATH_SEP '/'
#elif defined(_WIN32)
#define PATH_SEP '\\'
#endif

#define IMG_ANY 0
#define IMG_COPIED_ONLY 1
#define IMG_SCRN_ONLY 2

#if defined(__linux__) || defined(_WIN32)
/*
 * In-memory file to write png image
 */
struct mem_file {
    char *buffer;
    size_t capacity;
    size_t size;
} __attribute__((aligned(__alignof__(FILE))));
#endif

/*
 * List of files and the length of the path of their parent directory
 */
typedef struct _dir_files {
    size_t path_len;
    list2 *lst;
} dir_files;

/*
 * A wrapper for snprintf.
 * returns 1 if snprintf failed or truncated
 * returns 0 otherwise
 */
int snprintf_check(char *dest, size_t size, const char *fmt, ...)
#if defined(_WIN32) && !defined(__clang__)
    __attribute__((__format__(gnu_printf, 3, 4)));
#else
    __attribute__((__format__(printf, 3, 4)));
#endif

/*
 * Append error message to error log file
 */
extern void error(const char *msg);

/*
 * Append error message to error log file and exit
 */
extern void error_exit(const char *msg) __attribute__((noreturn));

/*
 * Free up heap memory allocated to global variables
 */
extern void cleanup(void);

extern void create_temp_file(void);

/*
 * Wrapper to realloc. In case of failure, free() the ptr and return NULL.
 */
extern void *realloc_or_free(void *ptr, size_t size) __attribute__((__malloc__));

/*
 * Get copied text from clipboard.
 * Places the text in a buffer and sets the bufptr to point the buffer.
 * bufptr must be a valid pointer to a char * variable.
 * Caller should free the buffer after using.
 * Places text length in the memory location pointed to by lenptr.
 * lenptr must be a valid pointer to a size_t variable.
 * On failure, buffer is set to NULL.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int get_clipboard_text(char **bufptr, uint32_t *lenptr);

/*
 * Reads len bytes of text from the data buffer and copies it into the clipboard.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int put_clipboard_text(char *data, uint32_t len);

/*
 * Get copied image from clipboard or a screenshot.
 * If mode is IMG_ANY, get copied image from clipboard, and if there is no image, get a screenshot instead.
 * If mode is IMG_COPIED_ONLY, get copied image from clipboard if available.
 * If mode is IMG_SCRN_ONLY, get a screenshot.
 * If disp is positive and configuration.client_selects_display is set, use disp as display number instead of default or
 * configured value.
 * Places the image data in a buffer and sets the buf_ptr to point the buffer. buf_ptr must be a valid
 * pointer to a char * variable. Caller should free the buffer after using. Places data length in the memory location
 * pointed to by len_ptr. len_ptr must be a valid pointer to a size_t variable. On failure, buffer is set to NULL.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int get_image(char **buf_ptr, uint32_t *len_ptr, int mode, uint16_t disp);

/*
 * Cut the files given by paths to clipboard. Another application may paste them.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int set_clipboard_cut_files(const list2 *paths);

/*
 * Get the file size of the file from the given file pointer fp.
 * returns the file size on success and -1 on failure.
 */
extern int64_t get_file_size(FILE *fp);

/*
 * Check if a file exists at the path given by file_name.
 * returns 1 if a file or directory or other special file type exists or 0 otherwise.
 */
extern int file_exists(const char *file_name);

/*
 * Check if the file at path is a directory.
 * Returns 1 if it is a directory or 0 if it is not a directory.
 * Returns -1 on error.
 */
extern int is_directory(const char *path, int follow_symlinks);

#if defined(__linux__) || defined(_WIN32)

/*
 * The function to be used as png write data function in libpng to write the
 * image into a memory buffer instead of a file. This will allocate memory for
 * the struct mem_file buffer.
 */
void png_mem_write_data(png_structp png_ptr, png_bytep data, png_size_t length);

#endif

/*
 * Converts line endings to LF or CRLF based on the platform.
 * param str_p is a valid pointer to malloced, null-terminated char * which may be realloced and returned.
 * If force_lf is non-zero, convert EOL to LF regardless of the platform
 * Else, convert EOL of str to LF
 * Returns the length of the new string without the terminating '\0'.
 * If an error occured, this will free() the *str_p and return -1.
 */
extern int64_t convert_eol(char **str_p, int force_lf);

#if defined(__linux__) || defined(__APPLE__)

#define open_file(filename, mode) fopen(filename, mode)
#define remove_file(filename) remove(filename)
#define chdir_wrapper(path) chdir(path)
#define getcwd_wrapper(len) getcwd(NULL, len)

/**
 * Get a list of copied files and directories as a single string.
 * The output string is allocated with malloc, and contains the list of URL-encoded file/dir names, separated by '\n'
 * (new-line character). The offset is set to the starting offset (in bytes) of the file list.
 * Returns the string of file list on success or NULL on error.
 */
extern char *get_copied_files_as_str(int *offset);

#elif defined(_WIN32)

/*
 * A wrapper for chdir() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern int chdir_wrapper(const char *path);

/**
 * Get the pathname of the current working directory. If successful, returns an array which is allocated with malloc;
 * the array is at least len bytes long.
 * Returns NULL if the directory couldn't be determined.
 */
extern char *getcwd_wrapper(int len);

/*
 * A wrapper for fopen() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern FILE *open_file(const char *filename, const char *mode);

/*
 * A wrapper for remove() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern int remove_file(const char *filename);

extern int wchar_to_utf8_str(const wchar_t *wstr, char **utf8str_p, uint32_t *len_p);

#endif

#if PROTOCOL_MIN <= 1

/*
 * Get a list of copied files from the clipboard.
 * Only regular files are included in the list.
 * returns the file list on success and NULL on failure.
 */
extern list2 *get_copied_files(void);

#endif  // PROTOCOL_MIN <= 1

#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

/*
 * Creates the directory given by the path and all its parent directories if missing.
 * Will not delete any existing files or directories.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int mkdirs(const char *path);

/*
 * Get a list of files in the directory at the path given by dirname.
 * returns the list of files on success or NULL on failure
 */
extern list2 *list_dir(const char *dirname);

/*
 * Accepts a valid pointer to a dir_files structure
 * Get copied files and directories from the clipboard.
 * Only regular files are included in the file list.
 * Set the path_len to the length of path name of the directory which the files are copied.
 * Sets directories and files in dfiles_p on success and sets the path_len to 0 and file list to NULL on failure.
 */
extern void get_copied_dirs_files(dir_files *dfiles_p, int include_leaf_dirs);

#if defined(__linux__) || defined(__APPLE__)

#define rename_file(old_name, new_name) rename(old_name, new_name)
#define remove_directory(path) rmdir(path)

#elif defined(_WIN32)

/*
 * A wrapper for rename() to be platform independent.
 * Internally converts the path names to wide char on Windows.
 */
extern int rename_file(const char *old_name, const char *new_name);

/*
 * A wrapper for rmdir() to be platform independent.
 * Internally converts the path names to wide char on Windows.
 */
extern int remove_directory(const char *path);

#endif

#endif  // (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

#endif  // UTILS_UTILS_H_
