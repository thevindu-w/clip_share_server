/* see LICENSE / README for more info
 *
 * 2022 Modified by H. Thevindu J. Wijesekera
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/utils.h>
#include <xscreenshot/xscreenshot.h>

/* LSBFirst: BGRA -> RGBA */
static void convertrow_lsb(unsigned char *drow, const unsigned char *srow, const XImage *img) {
    const int bytes_per_line = img->bytes_per_line;
    const unsigned char bytes_per_pixel = (unsigned char)(img->bits_per_pixel / 8);
    int sx;
    int dx;

    for (sx = 0, dx = 0; sx + 3 <= bytes_per_line; sx += bytes_per_pixel) {
        drow[dx++] = srow[sx + 2]; /* B -> R */
        drow[dx++] = srow[sx + 1]; /* G -> G */
        drow[dx++] = srow[sx];     /* R -> B */
    }
}

/* MSBFirst: ARGB -> RGBA */
static void convertrow_msb(unsigned char *drow, const unsigned char *srow, const XImage *img) {
    const int bytes_per_line = img->bytes_per_line;
    const unsigned char bytes_per_pixel = (unsigned char)(img->bits_per_pixel / 8);
    int sx;
    int dx;

    for (sx = 0, dx = 0; sx + 3 <= bytes_per_line; sx += bytes_per_pixel) {
        drow[dx++] = srow[sx + 1]; /* G -> R */
        drow[dx++] = srow[sx + 2]; /* B -> G */
        drow[dx++] = srow[sx + 3]; /* A -> B */
    }
}

static int png_write_buf(XImage *img, char **buf_ptr, size_t *len) {
    png_structp png_write_p;
    png_infop png_info_p;
    unsigned char *drow = NULL;
    const unsigned char *srow;

    png_write_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_write_p) {
        *len = 0;
        return EXIT_FAILURE;
    }

    png_info_p = png_create_info_struct(png_write_p);
    if (!png_info_p || setjmp(png_jmpbuf(png_write_p))) {
        png_destroy_write_struct(&png_write_p, NULL);
        *len = 0;
        return EXIT_FAILURE;
    }

    struct mem_file fake_file;
    fake_file.buffer = NULL;
    fake_file.capacity = 0;
    fake_file.size = 0;
    png_init_io(png_write_p, (png_FILE_p)&fake_file);
    png_set_write_fn(png_write_p, &fake_file, png_mem_write_data, NULL);
    png_set_IHDR(png_write_p, png_info_p, (png_uint_32)(img->width), (png_uint_32)(img->height), 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);
    png_write_info(png_write_p, png_info_p);

    srow = (unsigned char *)img->data;
    drow = malloc((size_t)(img->width) * 4); /* output RGBA */
    if (!drow) {
        png_destroy_write_struct(&png_write_p, &png_info_p);
        *len = 0;
        return EXIT_FAILURE;
    }

    void (*convert)(unsigned char *, const unsigned char *, const XImage *);
    if (img->byte_order == LSBFirst)
        convert = convertrow_lsb;
    else
        convert = convertrow_msb;

    for (int h = 0; h < img->height; h++) {
        convert(drow, srow, img);
        srow += img->bytes_per_line;
        png_write_row(png_write_p, (png_const_bytep)drow);
    }
    png_write_end(png_write_p, NULL);
    free(drow);

    fake_file.buffer = realloc(fake_file.buffer, fake_file.size);
    *buf_ptr = fake_file.buffer;
    *len = fake_file.size;
    png_free_data(png_write_p, png_info_p, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_write_p, &png_info_p);
    return EXIT_SUCCESS;
}

int screenshot_util(size_t *len, char **buf) {
    XImage *img;
    Display *dpy;
    Window win;
    XWindowAttributes attr;

    if (!(dpy = XOpenDisplay(NULL))) {
        *len = 0;
        return EXIT_FAILURE;
    }

    win = RootWindow(dpy, 0);

    XGrabServer(dpy);
    XGetWindowAttributes(dpy, win, &attr);

    img = XGetImage(dpy, win, 0, 0, (unsigned int)(attr.width), (unsigned int)(attr.height), 0xffffffff, ZPixmap);

    XUngrabServer(dpy);
    XCloseDisplay(dpy);

    if (!img) {
        *len = 0;
        return EXIT_FAILURE;
    }
    png_write_buf(img, buf, len);
    XDestroyImage(img);

    return EXIT_SUCCESS;
}
