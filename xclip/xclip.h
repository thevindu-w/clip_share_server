#ifndef XCLIP_XCLIP_H_
#define XCLIP_XCLIP_H_

#define XCLIP_IN 0
#define XCLIP_OUT 1

/*
 * Get or set clipboard data
 * this will allocate a memory buffer and set the pointer in get mode
 * this will read data from the provided memory buffer in set mode
 * this will get or set the size of the buffer in bytes
 * returns -1 if an error occured
 */
extern int xclip_util(int, const char *, unsigned long *, char **);

#endif  // XCLIP_XCLIP_H_
