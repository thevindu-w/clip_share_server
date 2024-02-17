#ifndef XCLIP_XCLIP_H_
#define XCLIP_XCLIP_H_

#define XCLIP_IN 0
#define XCLIP_OUT 1

/*
 * Get or set clipboard data
 * Allocates a memory buffer and set the pointer in get mode.
 * Reads data from the provided memory buffer in set mode.
 * Gets or sets the size of the buffer in bytes.
 * Returns 0 on success.
 * Returns -1 if an error occured.
 */
extern int xclip_util(int, const char *, unsigned long *, char **);

#endif  // XCLIP_XCLIP_H_
