#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <png.h>

struct mem_file
{
	char *buffer;
	size_t capacity;
	size_t size;
};

static void my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct mem_file *p = (struct mem_file *)png_get_io_ptr(png_ptr);
	size_t nsize = p->size + length;

	/* allocate or grow buffer */
	if (p->buffer == NULL)
	{
		p->capacity = length > 1024 ? length : 1024;
		p->buffer = malloc(p->capacity);
	}
	else if (nsize > p->capacity)
	{
		p->capacity *= 2;
		p->capacity = nsize > p->capacity ? nsize : p->capacity;
		p->buffer = realloc(p->buffer, p->capacity);
	}

	if (!p->buffer)
		png_error(png_ptr, "Write Error");

	/* copy new bytes to end of buffer */
	memcpy(p->buffer + p->size, data, length);
	p->size += length;
}

/* LSBFirst: BGRA -> RGBA */
static void convertrow_lsb(unsigned char *drow, unsigned char *srow, XImage *img)
{
	int sx, dx;

	for (sx = 0, dx = 0; dx <= img->bytes_per_line - 3; sx += 4)
	{
		drow[dx++] = srow[sx + 2]; /* B -> R */
		drow[dx++] = srow[sx + 1]; /* G -> G */
		drow[dx++] = srow[sx];	   /* R -> B */
	}
}

/* MSBFirst: ARGB -> RGBA */
static void convertrow_msb(unsigned char *drow, unsigned char *srow, XImage *img)
{
	int sx, dx;

	for (sx = 0, dx = 0; dx <= img->bytes_per_line - 3; sx += 4)
	{
		drow[dx++] = srow[sx + 1]; /* G -> R */
		drow[dx++] = srow[sx + 2]; /* B -> G */
		drow[dx++] = srow[sx + 3]; /* A -> B */
	}
}

static int png_write_buf(XImage *img, char **buf_ptr, size_t *len)
{
	png_structp png_struct_p;
	png_infop png_info_p;
	void (*convert)(unsigned char *, unsigned char *, XImage *);
	unsigned char *drow = NULL, *srow;
	int h;

	png_struct_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info_p = png_create_info_struct(png_struct_p);

	if (!png_struct_p || !png_info_p || setjmp(png_jmpbuf(png_struct_p)))
	{
		*len = 0;
		return EXIT_FAILURE;
	}

	struct mem_file fake_file;
	fake_file.buffer = NULL;
	fake_file.capacity = 0;
	fake_file.size = 0;
	png_init_io(png_struct_p, (png_FILE_p)&fake_file);
	png_set_write_fn(png_struct_p, &fake_file, my_png_write_data, NULL);
	png_set_IHDR(png_struct_p, png_info_p, img->width, img->height, 8, PNG_COLOR_TYPE_RGB,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);
	png_write_info(png_struct_p, png_info_p);

	srow = (unsigned char *)img->data;
	drow = malloc(img->width * 4); /* output RGBA */
	if (!drow)
	{
		*len = 0;
		return EXIT_FAILURE;
	}

	if (img->byte_order == LSBFirst)
		convert = convertrow_lsb;
	else
		convert = convertrow_msb;

	for (h = 0; h < img->height; h++)
	{
		convert(drow, srow, img);
		srow += img->bytes_per_line;
		png_write_row(png_struct_p, drow);
	}
	png_write_end(png_struct_p, NULL);
	free(drow);

	fake_file.buffer = realloc(fake_file.buffer, fake_file.size);
	*buf_ptr = fake_file.buffer;
	*len = fake_file.size;
	png_free_data(png_struct_p, png_info_p, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_struct_p, NULL);
	return EXIT_SUCCESS;
}

int screenshot_util(size_t *len, char **buf)
{
	XImage *img;
	Display *dpy;
	Window win;
	XWindowAttributes attr;

	if (!(dpy = XOpenDisplay(NULL)))
	{
		*len = 0;
		return EXIT_FAILURE;
	}

	win = RootWindow(dpy, 0);

	XGrabServer(dpy);
	XGetWindowAttributes(dpy, win, &attr);

	img = XGetImage(dpy, win, 0, 0, attr.width, attr.height, 0xffffffff, ZPixmap);

	XUngrabServer(dpy);
	XCloseDisplay(dpy);

	if (!img)
	{
		*len = 0;
		return EXIT_FAILURE;
	}
	png_write_buf(img, buf, len);
	XDestroyImage(img);

	return EXIT_SUCCESS;
}
