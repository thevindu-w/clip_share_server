/*
 *  xclib.h - header file for functions in xclib.c
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

#ifndef XCLIP_XCLIB_H_
#define XCLIP_XCLIB_H_

#include <X11/Xlib.h>

/* xcout() contexts */
#define XCLIB_XCOUT_NONE 0              /* no context */
#define XCLIB_XCOUT_SENTCONVSEL 1       /* sent a request */
#define XCLIB_XCOUT_INCR 2              /* in an incr loop */
#define XCLIB_XCOUT_BAD_TARGET 3        /* given target failed */
#define XCLIB_XCOUT_SELECTION_REFUSED 4 /* owner signaled an error */

/* xcin() contexts */
#define XCLIB_XCIN_NONE 0
#define XCLIB_XCIN_SELREQ 1
#define XCLIB_XCIN_INCR 2

/* functions in xclib.c */
extern int xcout(Display *, Window, XEvent, Atom, Atom, Atom *, void **, unsigned long *, unsigned int *);
extern int xcin(Display *, Window *, XEvent, Atom *, Atom, const unsigned char *, unsigned long, unsigned long *,
                unsigned int *);
extern void *xcmalloc(size_t) __attribute__((__malloc__));
extern void *xcrealloc(void *, size_t) __attribute__((__malloc__));

#endif  // XCLIP_XCLIB_H_
