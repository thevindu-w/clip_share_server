#import <AppKit/NSPasteboard.h>
#import <globals.h>
#import <objc/Object.h>
#import <string.h>
#import <utils/utils.h>

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
