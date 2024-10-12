#ifndef XCLIP_XCLIP_H_
#define XCLIP_XCLIP_H_

#define XCLIP_IN 0
#define XCLIP_OUT 1

#include <stdint.h>

/*
 * Get or set clipboard data
 * Allocates a memory buffer and set the pointer to buf_ptr in get mode.
 * Reads data from the provided memory buffer pointed by buf_ptr in set mode.
 * Gets or sets the size of the buffer in bytes from/to len_ptr.
 * Returns 0 on success.
 * Returns -1 if an error occured.
 */
extern int xclip_util(int io, const char *atom_name, uint32_t *len_ptr, char **buf_ptr);

#endif  // XCLIP_XCLIP_H_
