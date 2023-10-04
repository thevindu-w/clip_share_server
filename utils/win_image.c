/*
 *  utils/win_image.c - get screenshot in windows
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

#ifdef _WIN32

#include <globals.h>
#include <libpng16/png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/utils.h>
#include <utils/win_image.h>
#include <windows.h>

/* Returns pixel of bitmap at given point. */
#define RGBPixelAtPoint(image, x, y) \
    *(((image)->pixels) + (((image)->bytewidth * (y)) + ((x) * (image)->bytes_per_pixel)))

typedef struct _RGBPixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGBPixel;

/* Structure for containing decompressed bitmaps. */
typedef struct _RGBBitmap {
    RGBPixel *pixels;
    size_t width;
    size_t height;
    size_t bytewidth;
    uint8_t bytes_per_pixel;
} RGBBitmap;

typedef struct _buf_len_ptr {
    char **buf_ptr;
    size_t *len_ptr;
} buf_len_ptr;

static unsigned short current_display;

static int write_png_to_mem(RGBBitmap *, char **, size_t *);
static void write_image(HBITMAP, char **, size_t *);

static void write_image(HBITMAP hBitmap3, char **buf_ptr, size_t *len_ptr) {
    HDC hDC;
    int iBits;
    WORD wBitCount;
    DWORD dwPaletteSize = 0;
    DWORD dwBmBitsSize = 0;
    BITMAP Bitmap0;
    BITMAPINFOHEADER bi;
    LPBITMAPINFOHEADER lpbi;
    HANDLE hDib, hPal;
    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    DeleteDC(hDC);
    if (iBits <= 1)
        wBitCount = 1;
    else if (iBits <= 4)
        wBitCount = 4;
    else if (iBits <= 8)
        wBitCount = 8;
    else
        wBitCount = 24;
    GetObject(hBitmap3, sizeof(Bitmap0), (LPSTR)&Bitmap0);
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = Bitmap0.bmWidth;
    bi.biHeight = -Bitmap0.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = wBitCount;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrImportant = 0;
    bi.biClrUsed = 256;
    dwBmBitsSize = (DWORD)(((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8 * Bitmap0.bmHeight);
    hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    *lpbi = bi;

    hPal = GetStockObject(DEFAULT_PALETTE);
    if (hPal) {
        hDC = GetDC(NULL);
        RealizePalette(hDC);
    }

    GetDIBits(hDC, hBitmap3, 0, (UINT)Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
              (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

    RGBBitmap rgbBitmap;
    rgbBitmap.bytes_per_pixel = (uint8_t)(wBitCount / 8);
    rgbBitmap.width = (size_t)Bitmap0.bmWidth;
    rgbBitmap.height = (size_t)Bitmap0.bmHeight;
    rgbBitmap.bytewidth = (size_t)(((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8);
    rgbBitmap.pixels = (RGBPixel *)((LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize);
    write_png_to_mem(&rgbBitmap, buf_ptr, len_ptr);
    GlobalUnlock(hDib);
    GlobalFree(hDib);
}

static BOOL CALLBACK enumCallback(HMONITOR monitor, HDC hdcSource, LPRECT lprect, LPARAM lparam) {
    (void)lprect;
    if (current_display++ != configuration.display) return TRUE;

    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(monitor, &info)) return FALSE;

    HDC hdcMemory = CreateCompatibleDC(hdcSource);
    if (!hdcMemory) return FALSE;

    int capX = (int)(info.rcMonitor.right - info.rcMonitor.left);
    int capY = (int)(info.rcMonitor.bottom - info.rcMonitor.top);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, capX, capY);
    HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);

    BitBlt(hdcMemory, 0, 0, capX, capY, hdcSource, info.rcMonitor.left, info.rcMonitor.top, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);

    DeleteDC(hdcSource);
    DeleteDC(hdcMemory);
    buf_len_ptr *buf_len = (buf_len_ptr *)lparam;
    write_image(hBitmap, buf_len->buf_ptr, buf_len->len_ptr);
    return TRUE;
}

void screenCapture(char **buf_ptr, size_t *len_ptr) {
    HDC hdcSource = GetDC(NULL);
    buf_len_ptr buf_len = {.buf_ptr = buf_ptr, .len_ptr = len_ptr};
    current_display = 1;
    if (EnumDisplayMonitors(hdcSource, NULL, enumCallback, (LPARAM)&buf_len) == 0) {
        if (*buf_ptr) free(*buf_ptr);
        *buf_ptr = NULL;
        *len_ptr = 0;
    }
}

/* Attempts to save PNG to file; returns 0 on success, non-zero on error. */
static int write_png_to_mem(RGBBitmap *bitmap, char **buf_ptr, size_t *len_ptr) {
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte **row_pointers;

    /* Initialize the write struct. */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        *len_ptr = 0;
        return -1;
    }

    /* Initialize the info struct. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        *len_ptr = 0;
        return -1;
    }

    /* Set up error handling. */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        *len_ptr = 0;
        return -1;
    }

    /* Set image attributes. */
    png_set_IHDR(png_ptr, info_ptr, (png_uint_32)bitmap->width, (png_uint_32)bitmap->height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG. */
    row_pointers = png_malloc(png_ptr, bitmap->height * sizeof(png_byte *));
    for (y = 0; y < bitmap->height; ++y) {
        uint8_t *row = (uint8_t *)malloc(
            bitmap->bytes_per_pixel * bitmap->width);  // png_malloc(png_ptr, sizeof(uint8_t)* bitmap->bytes_per_pixel);
        row_pointers[y] = (png_byte *)row;
        for (x = 0; x < bitmap->width; ++x) {
            RGBPixel color = *(RGBPixel *)(((uint8_t *)(bitmap->pixels)) + ((bitmap->bytewidth) * y) +
                                           (bitmap->bytes_per_pixel) * x);  // RGBPixelAtPoint(bitmap, x, y);
            *row++ = color.red;
            *row++ = color.green;
            *row++ = color.blue;
        }
    }

    struct mem_file fake_file;
    fake_file.buffer = NULL;
    fake_file.capacity = 0;
    fake_file.size = 0;

    /* Actually write the image data. */
    png_init_io(png_ptr, (png_FILE_p)&fake_file);
    png_set_write_fn(png_ptr, (png_FILE_p)&fake_file, png_mem_write_data, NULL);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* Cleanup. */
    for (y = 0; y < bitmap->height; y++) {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);

    /* Finish writing. */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fake_file.buffer = realloc(fake_file.buffer, fake_file.size);
    *buf_ptr = fake_file.buffer;
    *len_ptr = fake_file.size;
    return 0;
}

void getCopiedImage(char **buf_ptr, size_t *len_ptr) {
    if (!OpenClipboard(0)) {
        *len_ptr = 0;
        return;
    }
    if (!IsClipboardFormatAvailable(CF_BITMAP)) {
        CloseClipboard();
        *len_ptr = 0;
        return;
    }
    HBITMAP hBitmap = GetClipboardData(CF_BITMAP);
    if (!hBitmap) {
        CloseClipboard();
        *len_ptr = 0;
        return;
    }
    CloseClipboard();
    write_image(hBitmap, buf_ptr, len_ptr);
}

#endif
