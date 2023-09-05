/*
 *  xclip.c - command line interface to X server selections
 *  Copyright (C) 2001 Kim Saunders
 *  Copyright (C) 2007-2008 Peter Åstrand
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
 *
 * 2022 Modified by H. Thevindu J. Wijesekera
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_ICONV
#include <errno.h>
#include <iconv.h>
#endif
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmu/Atoms.h>

#include "../utils/utils.h"
#include "./xcdef.h"
#include "./xclib.h"
#include "./xclip.h"

typedef struct _xclip_options {
    Atom sseln; /* X selection to work with */
    Atom target;
    int fdiri;    /* direction is in */
    Display *dpy; /* connection to X11 display */
    char is_targets;
} xclip_options;

static int doIn(Window win, unsigned long len, const char *buf, xclip_options *options) {
    unsigned char *sel_buf = NULL; /* buffer for selection data */
    unsigned long sel_len = len;   /* length of sel_buf */
    XEvent evt;                    /* X Event Structures */

    /* in mode */
    sel_buf = xcmalloc(sel_len * sizeof(char));
    memcpy(sel_buf, buf, sel_len);

    /* Handle cut buffer if needed */
    if (options->sseln == XA_STRING) {
        XStoreBuffer(options->dpy, (char *)sel_buf, (int)sel_len, 0);
        free(sel_buf);
        return EXIT_SUCCESS;
    }

    /* take control of the selection so that we receive
     * SelectionRequest events from other windows
     */
    /* FIXME: Should not use CurrentTime, according to ICCCM section 2.1 */
    XSetSelectionOwner(options->dpy, options->sseln, win, CurrentTime);

    /* Avoid making the current directory in use, in case it will need to be umounted */
    if (chdir("/") == -1) {
        free(sel_buf);
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

            finished = xcin(options->dpy, &cwin, evt, &pty, options->target, sel_buf, sel_len, &sel_pos, &context);

            if (evt.type == SelectionClear) clear = 1;

            if ((context == XCLIB_XCIN_NONE) && clear) {
                free(sel_buf);
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
    unsigned char *sel_buf = NULL; /* buffer for selection data */
    unsigned long sel_len = 0;     /* length of sel_buf */
    XEvent evt;                    /* X Event Structures */
    unsigned int context = XCLIB_XCOUT_NONE;

    while (1) {
        /* only get an event if xcout() is doing something */
        if (context != XCLIB_XCOUT_NONE) XNextEvent(options->dpy, &evt);

        /* fetch the selection, or part of it */
        xcout(options->dpy, win, evt, options->sseln, options->target, &sel_type, &sel_buf, &sel_len, &context);

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
                *len_ptr = 0;
                return EXIT_FAILURE;
            }
        }

        /* only continue if xcout() is doing something */
        if (context == XCLIB_XCOUT_NONE) break;
    }

    *len_ptr = sel_len;
    if (sel_len > 0) {
        *buf_ptr = malloc(sel_len + 1);
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

int xclip_util(int io, const char *atom_name, unsigned long *len_ptr, char **buf_ptr) {
    /* Declare variables */
    Window win; /* Window */
    int exit_code;
    xclip_options options;

    options.fdiri = (io == XCLIP_IN) ? T : F;
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
        *len_ptr = 0;
        return EXIT_FAILURE;
    }

    // Xmu

    /* parse selection command line option */
    options.sseln = XA_CLIPBOARD(options.dpy);

    /* parse noutf8 and target command line options */
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

    if (options.fdiri) {
        exit_code = doIn(win, *len_ptr, *buf_ptr, &options);
    } else {
        exit_code = doOut(win, len_ptr, buf_ptr, &options);
        if ((exit_code != EXIT_SUCCESS) ||
            (atom_name && !strcmp(atom_name, "image/png") &&
             (*len_ptr < 8 || memcmp(*buf_ptr, "\x89PNG\r\n\x1a\n", 8))) ||
            (atom_name && !strcmp(atom_name, "image/jpeg") &&
             (*len_ptr < 3 || memcmp(*buf_ptr, "\xff\xd8\xff", 3)))) {  // invalid jpeg
            *len_ptr = 0;
            if (*buf_ptr) free(*buf_ptr);
            exit_code = EXIT_FAILURE;
        }
    }

    /* Disconnect from the X server */
    XCloseDisplay(options.dpy);

    /* exit */
    return exit_code;
}
