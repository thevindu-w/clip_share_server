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
#import <stdint.h>
#import <string.h>
#import <utils/utils.h>

#ifdef USE_SCREEN_CAPTURE_KIT
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#endif

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC.
#endif

static inline NSBitmapImageRep *get_copied_image(void);

int get_clipboard_text(char **bufptr, uint32_t *lenptr) {
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
    *lenptr = (uint32_t)strnlen(*bufptr, configuration.max_text_length + 1);
    (*bufptr)[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, uint32_t len) {
    char c = data[len];
    data[len] = 0;
    NSString *str_data = @(data);
    data[len] = c;

    create_temp_file();
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
    if (!fileURLs) return NULL;
    size_t tot_len = 0;
    for (NSURL *fileURL in fileURLs) {
        NSURL *pathURL = [fileURL filePathURL];
        if (!pathURL) continue;
        NSString *absString = [pathURL absoluteString];
        if (!absString) continue;
        const char *cstring = [absString UTF8String];
        tot_len += strnlen(cstring, MAX_FILE_NAME_LEN) + 1;  // +1 for separator (\n) or terminator (\0)
    }

    char *all_files = malloc(tot_len);
    if (!all_files) return NULL;
    char *ptr = all_files;
    for (NSURL *fileURL in fileURLs) {
        NSURL *pathURL = [fileURL filePathURL];
        if (!pathURL) continue;
        NSString *absString = [pathURL absoluteString];
        if (!absString) continue;
        const char *cstring = [absString UTF8String];
        strncpy(ptr, cstring, MIN(tot_len, MAX_FILE_NAME_LEN));
        size_t url_len = strnlen(cstring, MAX_FILE_NAME_LEN);
        ptr += strnlen(cstring, MAX_FILE_NAME_LEN);
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

int set_clipboard_cut_files(const list2 *paths) {
    NSMutableArray *fileURLsMutable = [[NSMutableArray alloc] init];
    for (size_t i = 0; i < paths->len; i++) {
        const char *fname = (char *)(paths->array[i]);
        NSURL *file_url = [NSURL fileURLWithPath:@(fname)];
        [fileURLsMutable addObject:file_url];
    }
    NSMutableArray *fileURLs = [fileURLsMutable copy];

    create_temp_file();
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

#ifdef USE_SCREEN_CAPTURE_KIT

static NSBitmapImageRep *get_screen_image(uint16_t disp) {
    __block NSBitmapImageRep *bitmap = NULL;
    @autoreleasepool {
        if (@available(macOS 14.0, *)) {
            dispatch_semaphore_t sema = dispatch_semaphore_create(0);

            [SCShareableContent
                getShareableContentWithCompletionHandler:^(SCShareableContent *shareableContent, NSError *error) {
                    if (error) {
                        dispatch_semaphore_signal(sema);
                        return;
                    }

                    NSArray<SCDisplay *> *displays = [shareableContent displays];
                    if (!displays || (unsigned long)[displays count] < disp) {
                        dispatch_semaphore_signal(sema);
                        return;
                    }
                    SCDisplay *display = [displays objectAtIndex:(disp - 1)];
                    SCContentFilter *filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
                    SCStreamConfiguration *scConf = [[SCStreamConfiguration alloc] init];
                    scConf.capturesAudio = NO;
                    scConf.excludesCurrentProcessAudio = YES;
                    scConf.preservesAspectRatio = YES;
                    scConf.showsCursor = NO;
                    scConf.captureResolution = SCCaptureResolutionBest;
                    scConf.width = (size_t)(NSWidth(filter.contentRect) * (CGFloat)filter.pointPixelScale);
                    scConf.height = (size_t)(NSHeight(filter.contentRect) * (CGFloat)filter.pointPixelScale);
                    [SCScreenshotManager captureImageWithFilter:filter
                                                  configuration:scConf
                                              completionHandler:^(CGImageRef cgImage, NSError *err) {
                                                  if (err) {
                                                      dispatch_semaphore_signal(sema);
                                                      return;
                                                  }
                                                  bitmap = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
                                                  dispatch_semaphore_signal(sema);
                                                  return;
                                              }];
                }];
            dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
        }
        return bitmap;
    }
}

#else

static inline CGDirectDisplayID get_display_id(uint16_t disp) {
    CGDirectDisplayID disp_ids[65536UL];
    uint32_t disp_cnt;
    if (CGGetOnlineDisplayList(65536UL, disp_ids, &disp_cnt)) {
        return (CGDirectDisplayID)-1;
    }
    if (disp_cnt >= (uint32_t)disp) {
        return disp_ids[disp - 1];
    }
    return (CGDirectDisplayID)-1;
}

static NSBitmapImageRep *get_screen_image(uint16_t disp) {
    CGDirectDisplayID disp_id = get_display_id(disp);
    CGImageRef screenshot = NULL;
    if (disp_id != (CGDirectDisplayID)-1) {
        screenshot = CGDisplayCreateImage(disp_id);
    }
    if (!screenshot) {
        return NULL;
    }
    NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithCGImage:screenshot];
    CGImageRelease(screenshot);
    return bitmap;
}

#endif

int get_image(char **buf_ptr, uint32_t *len_ptr, int mode, uint16_t disp) {
    *len_ptr = 0;
    *buf_ptr = NULL;
    NSBitmapImageRep *bitmap = NULL;
    if (mode != IMG_SCRN_ONLY) {
        bitmap = get_copied_image();
    }
    if (mode != IMG_COPIED_ONLY && !bitmap) {
        // If configured to force use the display from conf, override the disp value
        if (disp <= 0 || !configuration.client_selects_display) {
            disp = (uint16_t)configuration.display;
        }
        bitmap = get_screen_image(disp);
    }
    if (!bitmap) {
        return EXIT_FAILURE;
    }
    NSData *data = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    NSUInteger size = [data length];
    char *buf = malloc((size_t)size);
    if (!buf) return EXIT_FAILURE;
    [data getBytes:buf length:size];
    *buf_ptr = buf;
    *len_ptr = (uint32_t)size;
    return EXIT_SUCCESS;
}

#endif
