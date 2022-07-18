#ifndef _XCLIP_
#define _XCLIP_

/**
 * Get or set clipboard data
 * this will allocate a memory buffer and set the pointer in get mode
 * this will read data from the provided memory buffer in set mode
 * this will get or set the size of the buffer in bytes
 * returns -1 if an error occured
 */
extern int xclip_util(int, const char *, unsigned long *, char **);

#endif