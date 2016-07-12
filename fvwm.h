/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation (nation@rocket.sanders.lockheed.com)
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 * fvwm include file
 ***********************************************************************/

#ifndef _FVWM_
#define _FVWM_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xfuncs.h>

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

typedef unsigned long Pixel;
#define PIXEL_ALREADY_TYPEDEFED		/* for Xmu/Drawing.h */

#ifdef SIGNALRETURNSINT
#define SIGNAL_T int
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_T void
#define SIGNAL_RETURN return
#endif

#define BW 1			/* border width */
#define NOFRAME_BW 2    	/* border width */
#define BOUNDARY_WIDTH 7    	/* border width */
#define CORNER_WIDTH 16    	/* border width */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif


#define ICON_X_TEXT 3
#define ICON_Y_TEXT Scr.StdFont.height
#define ICON_HEIGHT (Scr.StdFont.height+4)

#define NULLSTR ((char *) NULL)

/* contexts for button presses */
#define C_NO_CONTEXT	0
#define C_WINDOW	1
#define C_TITLE		2
#define C_ICON		4
#define C_ROOT		8
#define C_FRAME		16
#define C_SIDEBAR       32
#define C_SYS           64
#define C_ICONIFY      128
#define NUM_CONTEXTS    8

typedef struct MyFont
{
  char *name;			/* name of the font */
  XFontStruct *font;		/* font structure */
  int height;			/* height of the font */
  int y;			/* Y coordinate to draw characters */
} MyFont;

typedef struct ColorPair
{
  Pixel fore;
  Pixel back;
} ColorPair;


/* for each window that is on the display, one of these structures
 * is allocated and linked into a list 
 */
typedef struct FvwmWindow
{
    struct FvwmWindow *next;	/* next fvwm window */
    struct FvwmWindow *prev;	/* prev fvwm window */
    Window w;			/* the child window */
    int old_bw;			/* border width before reparenting */
    Window lead_w;              /* either the frame for decorated windows,
				 * or the window itself, for undecorated */
    Window frame;		/* the frame window */
    Window title_w;		/* the title bar window */
    Window left_side_w;
    Window right_side_w;
    Window bottom_w;
    Window top_w;
    Window sys_w;
    Window iconify_w;
    Window icon_w;		/* the icon window */
    int frame_x;		/* x position of frame */
    int frame_y;		/* y position of frame */
    int frame_width;		/* width of frame */
    int frame_height;		/* height of frame */
    int frame_bw;		/* borderwidth of frame */
    int boundary_width;
    int corner_width;
    int title_x;
    int title_y;
    int title_height;		/* height of the title bar */
    int title_width;		/* width of the title bar */
    int icon_x_loc;		/* icon window x coordinate */
    int icon_y_loc;		/* icon window y coordiante */
    int icon_w_width;		/* width of the icon window */
    int icon_t_width;		/* width of the icon window */
    int icon_w_height;		/* width of the icon window */
    int icon_p_width;		/* width of the icon window */
    int icon_p_height;		/* width of the icon window */
    Pixmap iconPixmap;		/* pixmap for the icon */
    char *name;			/* name of the window */
    char *icon_name;		/* name of the icon */
    XWindowAttributes attr;	/* the child window attributes */
    XSizeHints hints;		/* normal hints */
    XWMHints *wmhints;		/* WM hints */
    XClassHint class;

    Window transientfor;
    unsigned long protocols;

    unsigned char flags;
    char *icon_bitmap_file;

} FvwmWindow;

/***************************************************************************
 * window flags definitions 
 ***************************************************************************/
#define STICKY      1   /* Does window stick to glass? */
#define ONTOP       2   /* does window stay on top */
#define BORDER      4   /* Is this decorated */
#define MAPPED      8   /* is it mapped? */
#define ICON       16   /* is it an icon now? */
#define TRANSIENT  32   /* is it a transient window? */
#define RAISED     64   /* if its a sticky window, does it need to be raised */
#define VISIBLE   128   /* is the window fully visible */


#define DoesWmTakeFocus		(1L << 0)
#define DoesWmSaveYourself	(1L << 1)
#define DoesWmDeleteWindow	(1L << 2)

#include <X11/Xosdefs.h>
#include <stdlib.h>
extern void Reborder();
extern SIGNAL_T Done();

extern Display *dpy;
extern Window ResizeWindow;	/* the window we are resizing */

extern XClassHint NoClass;

extern XContext FvwmContext;

extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

extern Atom _XA_MIT_PRIORITY_COLORS;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_SAVE_YOURSELF;
extern Atom _XA_WM_DELETE_WINDOW;

#endif /* _FVWM_ */



