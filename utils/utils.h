/*
 *  utils/utils.h - header for utils
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

#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/list_utils.h>

#ifdef __linux__
#include <png.h>
#elif _WIN32
#include <libpng16/png.h>
#endif

#ifdef __linux__
#define PATH_SEP '/'
#elif _WIN32
#define PATH_SEP '\\'
#endif

/*
 * In-memory file to write png image
 */
struct mem_file {
    char *buffer;
    size_t capacity;
    size_t size;
};

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
int snprintf_check(char *dest, size_t size, const char *fmt, ...) __attribute__((__format__(gnu_printf, 3, 4)));

/*
 * Append error message to error log file
 */
extern void error(const char *msg);

/*
 * Append error message to error log file and exit
 */
extern void error_exit(const char *msg) __attribute__((noreturn));

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
extern int get_clipboard_text(char **bufptr, size_t *lenptr);

/*
 * Reads len bytes of text from the data buffer and copies it into the clipboard.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int put_clipboard_text(char *data, size_t len);

/*
 * Get copied image from clipboard. If there is no image, get a screenshot instead.
 * Places the image data in a buffer and sets the buf_ptr to point the buffer.
 * buf_ptr must be a valid pointer to a char * variable.
 * Caller should free the buffer after using.
 * Places data length in the memory location pointed to by len_ptr.
 * len_ptr must be a valid pointer to a size_t variable.
 * On failure, buffer is set to NULL.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int get_image(char **buf_ptr, size_t *len_ptr);

/*
 * Get a list of copied files from the clipboard.
 * Only regular files are included in the list.
 * returns the file list on success and NULL on failure.
 */
extern list2 *get_copied_files(void);

/*
 * Get the file size of the file from the given file pointer fp.
 * returns the file size on success and -1 on failure.
 */
extern ssize_t get_file_size(FILE *fp);

/*
 * Check if a file exists at the path given by file_name.
 * returns 1 if a file or directory or other special file type exists or 0 otherwise.
 */
extern int file_exists(const char *file_name);

/*
 * Check if the file at path is a directory.
 * returns 1 if its a directory or 0 otherwise
 */
extern int is_directory(const char *path, int follow_symlinks);

/*
 * The function to be used as png write data function in libpng to write the
 * image into a memory buffer instead of a file. This will allocate memory for
 * the struct mem_file buffer.
 */
void png_mem_write_data(png_structp png_ptr, png_bytep data, png_size_t length);

/*
 * A wrapper for fopen() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern FILE *open_file(const char *filename, const char *mode);

/*
 * Converts line endings to LF or CRLF based on the platform.
 * param str_p is a valid pointer to malloced, null-terminated char * which may be realloced and returned.
 * If force_lf is non-zero, convert EOL to LF regardless of the platform
 * Else, convert EOL of str to LF
 * Returns the length of the new string without the terminating '\0'.
 * If an error occured, this will free() the *str_p and return -1.
 */
extern ssize_t convert_eol(char **str_p, int force_lf);

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

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
extern void get_copied_dirs_files(dir_files *dfiles_p);

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

#endif  // UTILS_UTILS_H_
