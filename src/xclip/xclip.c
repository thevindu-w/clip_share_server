/*
 *  xclip.c - command line interface to X server selections
 *  Copyright (C) 2001 Kim Saunders
 *  Copyright (C) 2007-2022 Peter Åstrand
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 *  2022-2025 Modified by H. Thevindu J. Wijesekera
 */

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmu/Atoms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/utils.h>
#include <xclip/xclib.h>
#include <xclip/xclip.h>

typedef struct _xclip_options {
    Atom sseln; /* X selection to work with */
    Atom target;
    Display *dpy; /* connection to X11 display */
    char is_targets;
} xclip_options;

static int doIn(Window win, unsigned long len, const char *buf, xclip_options *options) {
    XEvent evt; /* X Event Structures */

    /* Handle cut buffer if needed */
    if (options->sseln == XA_STRING) {
        XStoreBuffer(options->dpy, buf, (int)len, 0);
        return EXIT_SUCCESS;
    }

    /* take control of the selection so that we receive
     * SelectionRequest events from other windows
     */
    /* FIXME: Should not use CurrentTime, according to ICCCM section 2.1 */
    XSetSelectionOwner(options->dpy, options->sseln, win, CurrentTime);

    /* Avoid making the current directory in use, in case it will need to be umounted */
    if (chdir("/") == -1) {
        return EXIT_FAILURE;
    }

    /* loop and wait for the expected number of
     * SelectionRequest events
     */
    while (1) {
        /* wait for a SelectionRequest event */
        while (1) {
            static unsigned int clear = 0;
            static unsigned int context = XCLIB_XCIN_NONE;
            static unsigned long sel_pos = 0;
            static Window cwin;
            static Atom pty;
            int finished;

            XNextEvent(options->dpy, &evt);

            finished = xcin(options->dpy, &cwin, evt, &pty, options->target, (const unsigned char *)buf, len, &sel_pos,
                            &context);

            if (evt.type == SelectionClear) clear = 1;

            if ((context == XCLIB_XCIN_NONE) && clear) {
                return EXIT_SUCCESS;
            }

            if (finished) break;
        }
    }
}

static int doOut(Window win, unsigned long *len_ptr, char **buf_ptr, xclip_options *options) {
    *len_ptr = 0;
    *buf_ptr = NULL;
    Atom sel_type = None;
    void *sel_buf = NULL;       // buffer for selection data
    unsigned long sel_len = 0;  // length of sel_buf
    XEvent evt;                 // X Event Structures
    unsigned int context = XCLIB_XCOUT_NONE;

    while (1) {
        /* only get an event if xcout() is doing something */
        if (context != XCLIB_XCOUT_NONE) XNextEvent(options->dpy, &evt);

        /* fetch the selection, or part of it */
        xcout(options->dpy, win, evt, options->sseln, options->target, &sel_type, &sel_buf, &sel_len, &context);

        if (context == XCLIB_XCOUT_SELECTION_REFUSED) {
            if (sel_buf) free(sel_buf);
            return EXIT_FAILURE;
        }

        if (options->is_targets && sel_type == XA_ATOM) {
            const Atom *atom_buf = (Atom *)sel_buf;
            size_t atom_len = sel_len / sizeof(Atom);
            size_t out_len = 0;
            size_t out_capacity = 128;
            char *out_buf = xcmalloc(out_capacity);
            out_buf[0] = 0;
            while (atom_len--) {
                char *atom_name = XGetAtomName(options->dpy, *atom_buf++);
                const size_t atom_name_len = strnlen(atom_name, 511);
                while (atom_name_len + out_len + 1 >= out_capacity) {
                    out_capacity *= 2;
                    out_buf = xcrealloc(out_buf, out_capacity);
                }
                if ((ssize_t)(out_capacity - out_len) >= 0 &&
                    !snprintf_check(out_buf + out_len, out_capacity - out_len, "%s\n", atom_name))
                    out_len += atom_name_len + 1;
                XFree(atom_name);
            }
            *len_ptr = out_len;
            *buf_ptr = out_buf;
            if (sel_buf) free(sel_buf);
            return EXIT_SUCCESS;
        }

        if (context == XCLIB_XCOUT_BAD_TARGET) {
            if (options->target == XA_UTF8_STRING(options->dpy)) {
                /* fallback is needed. set XA_STRING to target and restart the loop. */
                context = XCLIB_XCOUT_NONE;
                options->target = XA_STRING;
                continue;
            } else {
                /* no fallback available, exit with failure */
                return EXIT_FAILURE;
            }
        }

        /* only continue if xcout() is doing something */
        if (context == XCLIB_XCOUT_NONE) break;
    }

    *len_ptr = sel_len;
    if (sel_len > 0) {
        *buf_ptr = xcmalloc(sel_len + 1);
        memcpy(*buf_ptr, sel_buf, sel_len);
        (*buf_ptr)[sel_len] = 0;
    }

    if (options->sseln == XA_STRING) {
        XFree(sel_buf);
    } else {
        free(sel_buf);
    }

    return EXIT_SUCCESS;
}

int xclip_util(int io, const char *atom_name, uint32_t *len_ptr, char **buf_ptr) {
    if (io == XCLIP_OUT) {
        *len_ptr = 0;
        *buf_ptr = NULL;
    }

    /* Declare variables */
    Window win; /* Window */
    int exit_code;
    xclip_options options;

    options.target = XA_STRING;

    /* Connect to the X server. */
    if ((options.dpy = XOpenDisplay(NULL))) {
/* successful */
#ifdef DEBUG_MODE
        fputs("Connected to X server\n", stderr);
#endif
    } else {
/* couldn't connect to X server. Print error and exit */
#ifdef DEBUG_MODE
        fputs("Connect X server failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }

    /* parse selection command line option */
    options.sseln = XA_CLIPBOARD(options.dpy);

    /* parse target options */
    if (atom_name == NULL) {
        options.target = XA_UTF8_STRING(options.dpy);
    } else {
        options.target = XInternAtom(options.dpy, atom_name, False);
    }

    if (atom_name && !strcmp("TARGETS", atom_name)) {
        options.is_targets = 1;
    } else {
        options.is_targets = 0;
    }

    /* Create a window to trap events */
    win = XCreateSimpleWindow(options.dpy, DefaultRootWindow(options.dpy), 0, 0, 1, 1, 0, 0, 0);

    /* get events about property changes */
    XSelectInput(options.dpy, win, PropertyChangeMask);

    unsigned long len = 0;
    if (io == XCLIP_IN) {
        exit_code = doIn(win, *len_ptr, *buf_ptr, &options);
    } else {
        exit_code = doOut(win, &len, buf_ptr, &options);
    }

    /* Disconnect from the X server */
    XCloseDisplay(options.dpy);

    if (io == XCLIP_IN) return exit_code;

    if (exit_code != EXIT_SUCCESS || len >= 0xFFFFFFFFUL || !*buf_ptr) {
        exit_code = EXIT_FAILURE;
    }
    if ((exit_code != EXIT_SUCCESS) ||
        (atom_name && !strcmp(atom_name, "image/png") &&
         (len < 8 || memcmp(*buf_ptr, "\x89PNG\r\n\x1a\n", 8))) ||  // invalid png
        (atom_name && !strcmp(atom_name, "image/jpeg") &&
         (len < 3 || memcmp(*buf_ptr, "\xff\xd8\xff", 3)))) {  // invalid jpeg
        *len_ptr = 0;
        exit_code = EXIT_FAILURE;
    }
    if (exit_code == EXIT_SUCCESS) {
        *len_ptr = (uint32_t)len;
    } else {
        if (*buf_ptr) free(*buf_ptr);
        *buf_ptr = NULL;
    }

    return exit_code;
}
