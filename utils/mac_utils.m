/*
 * utils/mac_utils.h - utility functions for macOS
 * Copyright (C) 2024 H. Thevindu J. Wijesekera
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

#ifdef __APPLE__

#import <AppKit/AppKit.h>
#import <AppKit/NSPasteboard.h>
#import <globals.h>
#import <objc/Object.h>
#import <stddef.h>
#import <string.h>
#import <utils/utils.h>

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC.
#endif

static inline CGDirectDisplayID get_display_id(int disp);
static inline NSBitmapImageRep *get_copied_image(void);

int get_clipboard_text(char **bufptr, size_t *lenptr) {
    NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
    NSString *copiedString = [pasteBoard stringForType:NSPasteboardTypeString];
    if (!copiedString) {
        *lenptr = 0;
        if (*bufptr) free(*bufptr);
        *bufptr = NULL;
        return EXIT_FAILURE;
    }
    const char *cstring = [copiedString UTF8String];
    *bufptr = strndup(cstring, configuration.max_text_length);
    *lenptr = strnlen(*bufptr, configuration.max_text_length + 1);
    (*bufptr)[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    char c = data[len];
    data[len] = 0;
    NSString *str_data = @(data);
    data[len] = c;
    NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
    [pasteBoard clearContents];
    BOOL status = [pasteBoard setString:str_data forType:NSPasteboardTypeString];
    if (status != YES) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

char *get_copied_files_as_str(int *offset) {
    *offset = 0;
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classes = [NSArray arrayWithObject:[NSURL class]];
    NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                        forKey:NSPasteboardURLReadingFileURLsOnlyKey];
    NSArray *fileURLs = [pasteboard readObjectsForClasses:classes options:options];
    size_t tot_len = 0;
    for (NSURL *fileURL in fileURLs) {
        const char *cstring = [[fileURL absoluteString] UTF8String];
        tot_len += strnlen(cstring, 2047) + 1;
    }

    char *all_files = malloc(tot_len);
    if (!all_files) return NULL;
    char *ptr = all_files;
    for (NSURL *fileURL in fileURLs) {
        const char *cstring = [[fileURL absoluteString] UTF8String];
        strncpy(ptr, cstring, MIN(tot_len, 2047));
        size_t url_len = strnlen(cstring, 2047);
        ptr += strnlen(cstring, 2047);
        *ptr = '\n';
        ptr++;
        tot_len -= url_len + 1;
    }
    ptr--;
    *ptr = 0;
#ifdef DEBUG_MODE
    puts(all_files);
#endif
    return all_files;
}

int set_clipboard_cut_files(list2 *paths) {
    NSMutableArray *fileURLsMutable = [[NSMutableArray alloc] init];
    for (size_t i = 0; i < paths->len; i++) {
        const char *fname = (char *)(paths->array[i]);
        NSURL *file_url = [NSURL fileURLWithPath:@(fname)];
        [fileURLsMutable addObject:file_url];
    }
    NSMutableArray *fileURLs = [fileURLsMutable copy];
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard writeObjects:fileURLs];
    return EXIT_SUCCESS;
}

static inline NSBitmapImageRep *get_copied_image(void) {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSImage *img = [[NSImage alloc] initWithPasteboard:pasteboard];
    if (!img) return NULL;
    CGImageRef cgRef = [img CGImageForProposedRect:NULL context:NULL hints:NULL];
    NSBitmapImageRep *imgRep = [[NSBitmapImageRep alloc] initWithCGImage:cgRef];
    if (!imgRep) {
        CGImageRelease(cgRef);
        return NULL;
    }
    [imgRep setSize:[img size]];
    CGImageRelease(cgRef);
    return imgRep;
}

static inline CGDirectDisplayID get_display_id(int disp) {
    CGDirectDisplayID disp_ids[65536];
    uint32_t disp_cnt;
    if (CGGetOnlineDisplayList(65536, disp_ids, &disp_cnt)) {
        return (CGDirectDisplayID)-1;
    }
    if (disp_cnt >= (uint32_t)disp) {
        return disp_ids[disp - 1];
    }
    return (CGDirectDisplayID)-1;
}

int get_image(char **buf_ptr, size_t *len_ptr, int mode, int disp) {
    *len_ptr = 0;
    *buf_ptr = NULL;
    NSBitmapImageRep *bitmap = NULL;
    if (mode != IMG_SCRN_ONLY) {
        bitmap = get_copied_image();
    }
    if (mode != IMG_COPIED_ONLY && !bitmap) {
        // If configured to force use the display from conf, override the disp value
        if (disp <= 0 || !configuration.client_selects_display) disp = (int)configuration.display;
        CGDirectDisplayID disp_id = get_display_id(disp);
        CGImageRef screenshot = NULL;
        if (disp_id != (CGDirectDisplayID)-1) screenshot = CGDisplayCreateImage(disp_id);
        if (!screenshot) return EXIT_FAILURE;
        bitmap = [[NSBitmapImageRep alloc] initWithCGImage:screenshot];
        CGImageRelease(screenshot);
    }
    if (!bitmap) return EXIT_FAILURE;
    NSData *data = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    NSUInteger size = [data length];
    char *buf = malloc((size_t)size);
    if (!buf) return EXIT_FAILURE;
    [data getBytes:buf length:size];
    *buf_ptr = buf;
    *len_ptr = (size_t)size;
    return EXIT_SUCCESS;
}

#endif
