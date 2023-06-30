/*
 *
 *
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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>
#include "xcdef.h"
#include "xclib.h"
#include "xclip.h"

/* command line option table for XrmParseCommand() */
XrmOptionDescRec opt_tab[2];

/* Options that get set on the command line */
char *sdisp = NULL;		 /* X display to connect to */
Atom sseln = XA_PRIMARY; /* X selection to work with */
Atom target = XA_STRING;

/* Flags for command line options */
static int fdiri = T; /* direction is in */

Display *dpy;			   /* connection to X11 display */
XrmDatabase opt_db = NULL; /* database for options */

char **fil_names;	/* names of files to read */
int fil_number = 0; /* number of files to read */
int fil_current = 0;

/* variables to hold Xrm database record and type */
XrmValue rec_val;
char *rec_typ;

int tempi = 0;

static int
doIn(Window win, unsigned long len, const char *buf)
{
	unsigned char *sel_buf = NULL;		 /* buffer for selection data */
	unsigned long sel_len = len; /* length of sel_buf */
	XEvent evt;					 /* X Event Structures */
	int dloop = 0;				 /* done loops counter */

	/* in mode */
	sel_buf = xcmalloc(sel_len * sizeof(char));
	memcpy(sel_buf, buf, sel_len);

	/* Handle cut buffer if needed */
	if (sseln == XA_STRING)
	{
		XStoreBuffer(dpy, (char *)sel_buf, (int)sel_len, 0);
		return EXIT_SUCCESS;
	}

	/* take control of the selection so that we receive
	 * SelectionRequest events from other windows
	 */
	/* FIXME: Should not use CurrentTime, according to ICCCM section 2.1 */
	XSetSelectionOwner(dpy, sseln, win, CurrentTime);

	/* Avoid making the current directory in use, in case it will need to be umounted */
	if (chdir("/") == -1)
	{
		return EXIT_FAILURE;
	}

	/* loop and wait for the expected number of
	 * SelectionRequest events
	 */
	while (1)
	{
		/* wait for a SelectionRequest event */
		while (1)
		{
			static unsigned int clear = 0;
			static unsigned int context = XCLIB_XCIN_NONE;
			static unsigned long sel_pos = 0;
			static Window cwin;
			static Atom pty;
			int finished;

			XNextEvent(dpy, &evt);

			finished = xcin(dpy, &cwin, evt, &pty, target, sel_buf, sel_len, &sel_pos, &context);

			if (evt.type == SelectionClear)
				clear = 1;

			if ((context == XCLIB_XCIN_NONE) && clear)
				return EXIT_SUCCESS;

			if (finished)
				break;
		}

		dloop++; /* increment loop counter */
	}

	return EXIT_SUCCESS;
}

static int
doOut(Window win, unsigned long *length, char **buf)
{
	Atom sel_type = None;
	unsigned char *sel_buf = NULL;	   /* buffer for selection data */
	unsigned long sel_len = 0; /* length of sel_buf */
	XEvent evt;				   /* X Event Structures */
	unsigned int context = XCLIB_XCOUT_NONE;

	while (1)
	{
		/* only get an event if xcout() is doing something */
		if (context != XCLIB_XCOUT_NONE)
			XNextEvent(dpy, &evt);

		/* fetch the selection, or part of it */
		xcout(dpy, win, evt, sseln, target, &sel_type, &sel_buf, &sel_len, &context);

		if (context == XCLIB_XCOUT_BAD_TARGET)
		{
			if (target == XA_UTF8_STRING(dpy))
			{
				/* fallback is needed. set XA_STRING to target and restart the loop. */
				context = XCLIB_XCOUT_NONE;
				target = XA_STRING;
				continue;
			}
			else
			{
				/* no fallback available, exit with failure */
				*length = 0;
				return EXIT_FAILURE;
			}
		}

		/* only continue if xcout() is doing something */
		if (context == XCLIB_XCOUT_NONE)
			break;
	}

	*length = sel_len;
	if (sel_len > 0)
	{
		*buf = malloc(sel_len + 1);
		memcpy(*buf, sel_buf, sel_len);
	}

	if (sseln == XA_STRING)
	{
		XFree(sel_buf);
	}
	else
	{
		free(sel_buf);
	}

	return EXIT_SUCCESS;
}

int xclip_util(int io, const char *atom_name, unsigned long *len_ptr, char **buf_ptr)
{
	/* Declare variables */
	Window win; /* Window */
	int exit_code;

	if (io)
	{
		fdiri = F;
	}

	/* Connect to the X server. */
	if ((dpy = XOpenDisplay(sdisp)))
	{
/* successful */
#ifdef DEBUG_MODE
		fputs("Connected to X server\n", stderr);
#endif
	}
	else
	{
/* couldn't connect to X server. Print error and exit */
#ifdef DEBUG_MODE
		fputs("Connect X server failed\n", stderr);
#endif
		*len_ptr = 0;
		return EXIT_FAILURE;
	}

	/* parse selection command line option */
	sseln = XA_CLIPBOARD(dpy); // doOptSel();

	/* parse noutf8 and target command line options */
	if (atom_name == NULL)
	{
		target = XA_UTF8_STRING(dpy); // doOptTarget();
	}
	else
	{
		target = XInternAtom(dpy, atom_name, False);
	}

	/* Create a window to trap events */
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);

	/* get events about property changes */
	XSelectInput(dpy, win, PropertyChangeMask);

	if (fdiri)
	{
		exit_code = doIn(win, *len_ptr, *buf_ptr);
	}
	else
	{
		exit_code = doOut(win, len_ptr, buf_ptr);
		if (exit_code != EXIT_SUCCESS)
		{
			*len_ptr = 0;
		}
		if (atom_name && !strcmp(atom_name, "image/png") && (*len_ptr < 8 || memcmp(*buf_ptr, "\x89PNG\r\n\x1a\n", 8)))
		{ // invalid png
			*len_ptr = 0;
			exit_code = EXIT_FAILURE;
		}
		if (atom_name && !strcmp(atom_name, "image/jpeg") && (*len_ptr < 3 || memcmp(*buf_ptr, "\xff\xd8\xff", 3)))
		{ // invalid jpeg
			*len_ptr = 0;
			exit_code = EXIT_FAILURE;
		}
	}

	/* Disconnect from the X server */
	XCloseDisplay(dpy);

	/* exit */
	return exit_code;
}