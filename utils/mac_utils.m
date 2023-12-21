#import <AppKit/AppKit.h>
#import <AppKit/NSPasteboard.h>
#import <globals.h>
#import <objc/Object.h>
#import <stddef.h>
#import <string.h>
#import <utils/utils.h>

#define MIN_OF(x, y) (x < y ? x : y)

int get_clipboard_text(char **bufptr, size_t *lenptr) {
    NSPasteboard* pasteBoard = [NSPasteboard generalPasteboard];
    NSString* copiedString = [pasteBoard stringForType:NSPasteboardTypeString];
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
    NSPasteboard* pasteBoard = [NSPasteboard generalPasteboard];
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
    NSDictionary *options =
        [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:NSPasteboardURLReadingFileURLsOnlyKey];
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
        strncpy(ptr, cstring, MIN_OF(tot_len, 2047));
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

int get_image(char **buf_ptr, size_t *len_ptr) {
    *len_ptr = 0;
    *buf_ptr = NULL;
    CGImageRef screenshot = CGWindowListCreateImage(CGRectInfinite, kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault);
    if (!screenshot) return EXIT_FAILURE;
    NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithCGImage:screenshot];
    NSData *data = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    NSUInteger size = [data length];
    char *buf = malloc((size_t)size);
    if (!buf) return EXIT_FAILURE;
    [data getBytes:buf length:size];
    [bitmap release];
    CGImageRelease(screenshot);
    *buf_ptr = buf;
    *len_ptr = (size_t)size;
    return EXIT_SUCCESS;
}
