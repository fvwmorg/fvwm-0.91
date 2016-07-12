/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation (nation@rocket.sanders.lockheed.com)
 ****************************************************************************/
/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * fvwm per-screen data include file
 *
 ***********************************************************************/

#ifndef _SCREEN_
#define _SCREEN_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "misc.h"
#include "menus.h"

#define SIZE_HINDENT 10
#define SIZE_VINDENT 2
#define MAX_WINDOW_WIDTH 32767
#define MAX_WINDOW_HEIGHT 32767

typedef struct ScreenInfo
{
  int screen;
  int d_depth;		/* copy of DefaultDepth(dpy, screen) */
  Visual *d_visual;		/* copy of DefaultVisual(dpy, screen) */
  int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;	/* my copy of DisplayHeight(dpy, screen) */
  
  FvwmWindow FvwmRoot;		/* the head of the fvwm window list */
  Window Root;		/* the root window */
  Window SizeWindow;		/* the resize dimensions window */
  
  Pixmap gray;                 /* gray pattern for borders */
  Pixmap light_gray;           /* light gray pattern for inactive borders */
  Pixmap border_gray;          /* gray pattern for borders */
  
  MouseButton *MouseButtonRoot;
  FuncKey FuncKeyRoot;

  int root_pushes;		/* current push level to install root
				   colormap windows */
  FvwmWindow *pushed_window;	/* saved window to install when pushes drops
				   to zero */
  
  ColorPair StdColors; 	/* standard fore/back colors */
  ColorPair HiColors; 	/* standard fore/back colors */
  ColorPair StdRelief;
  ColorPair HiRelief;
  MyFont StdFont;     	/* font structure */
  
  Cursor PositionCursor;		/* upper Left corner cursor */
  Cursor TitleCursor;		/* title bar cursor */
  Cursor FrameCursor;		/* frame cursor */
  Cursor SysCursor;		/* sys-menu and iconify boxes cursor */
  Cursor IconCursor;		/* icon cursor */
  Cursor MoveCursor;		/* move cursor */
  Cursor ResizeCursor;          /* resize cursor */
  Cursor WaitCursor;		/* wait a while cursor */
  Cursor MenuCursor;		/* menu cursor */
  Cursor SelectCursor;	        /* dot cursor for f.move, etc. from menus */
  Cursor DestroyCursor;		/* skull and cross bones, f.destroy */
  Cursor SideBarCursor;

  name_list *NoTitle;		/* list of window names with no title bar */
  name_list *Sticky;		/* list of window names which stick to glass */
  name_list *OnTop;		/* list of window names which stay on top */
  name_list *CirculateSkip;     /* list of window names to avoid when
				 * circulating */
  name_list *Icons;             /* list of icon pixmaps for various windows */
  
  GC NormalGC;		        /* normal GC for menus, pager, resize window */
  GC NormalStippleGC;	        /* GC stippled borders on mono screens */
  GC InvNormalGC;	        /* reverse normal GC for menus */
  GC HiliteGC;                  /* GC for border on selected window */
  GC HiStippleGC;	        /* GC stippled borders on mono screens */
  GC DrawGC;			/* GC to draw lines for move and resize */
  GC HiReliefGC;                /* GC for highlighted window relief */
  GC HiShadowGC;                /* GC for highlighted window shadow */
  GC StdReliefGC;                /* GC for unselected window relief */
  GC StdShadowGC;                /* GC for unselected window shadow */
  int SizeStringOffset;	        /* x offset in size window for drawing */
  int SizeStringWidth;	        /* minimum width of size window */
  int BorderWidth;		/* border width of fvwm windows */
  int CornerWidth;	        /* corner width for decoratedwindows */
  int BoundaryWidth;	        /* frame width for decorated windows */
  int NoBorderWidth;            /* border width for undecorated windows */
  int TitleHeight;		/* height of the title bar window */
  FvwmWindow *Focus;		/* the fvwm window that has focus */
  int EntryHeight;		/* menu entry height */
  int EdgeScrollX;              /* #pixels to scroll on screen edge */
  int EdgeScrollY;              /* #pixels to scroll on screen edge */
  Bool CenterOnCirculate;       /* Should we center the target window when 
				 * circulating? */
  Bool ClickToFocus;            /* Focus follows mouse, or click to focus?*/
  unsigned char buttons2grab;   /* buttons to grab in click to focus mode */
  Bool DecorateTransients;      /* decorate transient windows? */
  Bool DontMoveOff;             /* make sure that all windows stay on desktop*/
  Bool RandomPlacement;         /* place windows in random locations? */
  int randomx;                  /* values used for randomPlacement */
  int randomy;
  Bool AutoPlaceIcons;          /* should we try to auto-place icons? */
  unsigned VScale;              /* Panner scale factor */
  FvwmWindow *LastWindowRaised; /* Last window which was raised. Used for raise
				 * lower func. */
  int VxMax;                    /* Max location for top left of virt desk*/
  int VyMax;
  int Vx;                       /* Current loc for top left of virt desk */
  int Vy;

} ScreenInfo;

extern ScreenInfo Scr;

#endif /* _SCREEN_ */
