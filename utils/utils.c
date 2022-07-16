#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "utils.h"
#include "../xclip/xclip.h"
#include "../xscreenshot/screenshot.h"

int get_image(char **buf_ptr, size_t *len_ptr)
{
    if (xclip_util(1, "image/png", len_ptr, buf_ptr) || *len_ptr == 0) // do not change the order
    {
#ifdef DEBUG_MODE
        printf("xclip failed to get image/png. len = %lu\n", *len_ptr);
#endif
        *len_ptr = 0;
        if (screenshot_util(len_ptr, buf_ptr) || *len_ptr == 0) // do not change the order
        {
#ifdef DEBUG_MODE
            fputs("Get screenshot failed\n", stderr);
#endif
            *len_ptr = 0;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static inline char hex2char(char h)
{
    if ('0' <= h && h <= '9')
        return h - '0';
    if ('A' <= h && h <= 'F')
        return h - 'A' + 10;
    if ('a' <= h && h <= 'f')
        return h - 'a' + 10;
    return -1;
}

int url_decode(char *str)
{
    if (strncmp("file://", str, 7))
        return EXIT_FAILURE;
    char *ptr1 = str, *ptr2 = strstr(str, "://");
    if (!ptr2)
        return EXIT_FAILURE;
    ptr2 += 3;
    do
    {
        char c;
        if (*ptr2 == '%')
        {
            c = 0;
            ptr2++;
            char tmp = *ptr2;
            char c1 = hex2char(tmp);
            if (c1 < 0)
                return EXIT_FAILURE; // invalid url
            c = c1 << 4;
            ptr2++;
            tmp = *ptr2;
            c1 = hex2char(tmp);
            if (c1 < 0)
                return EXIT_FAILURE; // invalid url
            c |= c1;
        }
        else
        {
            c = *ptr2;
        }
        *ptr1 = c;
        ptr1++;
        ptr2++;
    } while (*ptr2);
    *ptr1 = 0;
    return EXIT_SUCCESS;
}