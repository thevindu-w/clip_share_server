#ifndef XSCREENSHOT_XSCREENSHOT_H_
#define XSCREENSHOT_XSCREENSHOT_H_

#include <stdlib.h>

/*
 * Get a screenshot and save it to a memory buffer
 * Allocates a memory buffer and set the pointer to *buf_p.
 * Sets the size of the buffer in bytes to *len_p.
 * Returns 0 on success.
 * Returns -1 if an error occured.
 */
extern int screenshot_util(int display, size_t *len_p, char **buf_p);

#endif  // XSCREENSHOT_XSCREENSHOT_H_
