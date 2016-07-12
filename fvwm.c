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
 * fvwm - "Feeble Virtual Window Manager"
 ***********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include "fvwm.h"
#include "misc.h"
#include "menus.h"
#include "screen.h"
#include <X11/Xproto.h>
#include <X11/Xatom.h>

ScreenInfo Scr;		        /* structures for the screen */
Display *dpy;			/* which display are we talking to */
Window ResizeWindow;		/* the window we are resizing */

static int CatchRedirectError();	/* for settting RedirectError */
static int FvwmErrorHandler();
void newhandler(int sig);
void CreateCursors();
void NoisyExit();

XContext FvwmContext;		/* context for fvwm windows */

XClassHint NoClass;		/* for applications with no class */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;		/* junk window */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = {0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = {0x08, 0x02};
Bool debugging = False,PPosOverride = TRUE;
extern Window Pager_w;

char **g_argv,**g_envp;
Window keyboardRevert;
extern Bool KeyboardGrabbed;
#define USAGE "Fvwm Version 0.91\n\nusage:  fvwm [-d dpy] [-debug]\n"

/***********************************************************************
 *
 *  Procedure:
 *	main - start of fvwm
 *
 ***********************************************************************
 */

void main(int argc, char **argv, char **envp)
{
    Window root, parent, *children;
    unsigned int nchildren;
    int i, j;
    char *display_name = NULL;
    unsigned long valuemask;	/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    extern XEvent Event;
    void InternUsefulAtoms ();
    void InitVariables();

    for (i = 1; i < argc; i++) 
      {
	if (strncasecmp(argv[i],"-debug",6)==0)
	  debugging = True;
	else if (strncasecmp(argv[i],"-d",2)==0)
	  {
	    if (++i >= argc)
	      fprintf (stderr,USAGE);	      
	    display_name = argv[i];
	  }
	else 
	  fprintf (stderr,USAGE);
      }

    g_argv = argv;
    g_envp = envp;

    newhandler (SIGINT);
    newhandler (SIGHUP);
    newhandler (SIGQUIT);
    newhandler (SIGTERM);

    if (signal (SIGSEGV, SIG_IGN) != SIG_IGN)
      signal (SIGSEGV, NoisyExit);

    if (!(dpy = XOpenDisplay(display_name))) 
      {
	fprintf (stderr, "fvwm:  can't open display \"%s\"\n",
		 XDisplayName(display_name));
	exit (1);
      }
    
    if (fcntl(ConnectionNumber(dpy), F_SETFD, 1) == -1) 
      {
	fprintf(stderr,"fvwm: can't mark dpy conn close-on-exec\n");
	exit (1);
      }
    
    Scr.screen = DefaultScreen(dpy);

    InternUsefulAtoms ();

    /* Make sure property priority colors is empty */
    XChangeProperty (dpy, RootWindow(dpy,Scr.screen), _XA_MIT_PRIORITY_COLORS,
		     XA_CARDINAL, 32, PropModeReplace, NULL, 0);

    XSetErrorHandler(CatchRedirectError);
    XSelectInput(dpy, RootWindow (dpy, Scr.screen),
		 ColormapChangeMask | EnterWindowMask | PropertyChangeMask | 
		 SubstructureRedirectMask | KeyPressMask |
		 ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XSync(dpy, 0);

    XSetErrorHandler(FvwmErrorHandler);
    CreateCursors();
    InitVariables();

    Scr.gray = XCreateBitmapFromData(dpy,Scr.Root,g_bits,g_width,g_height);
    Scr.light_gray = 
      XCreateBitmapFromData(dpy,Scr.Root,l_g_bits,l_g_width, l_g_height);

    /* read config file, set up menus, colors, fonts */
    MakeMenus();
    if(Scr.ClickToFocus)
      keyboardRevert = None;
    else
      keyboardRevert = Scr.Root;
    XSetInputFocus (dpy, Scr.Root, RevertToPointerRoot, CurrentTime);
    XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
		  CurrentTime);
    KeyboardGrabbed = True;    
    Scr.TitleHeight = Scr.StdFont.height+2;

    XGrabServer(dpy);
    XSync(dpy, 0);
    if(debugging)
      XSynchronize(dpy,1);

    XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren);
    /*
     * weed out icon windows
     */
    for (i = 0; i < nchildren; i++) 
      {
	if (children[i]) 
	  {
	    XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
	    if (wmhintsp) 
	      {
		if (wmhintsp->flags & IconWindowHint) 
		  {
		    for (j = 0; j < nchildren; j++) 
		      {
			if (children[j] == wmhintsp->icon_window) 
			  {
			    children[j] = None;
			    break;
			  }
		      }
		  }
		XFree ((char *) wmhintsp);
	      }
	  }
      }
    /*
     * map all of the non-override windows
     */

    for (i = 0; i < nchildren; i++)
      {
	if (children[i] && MappedNotOverride(children[i]))
	  {
	    XUnmapWindow(dpy, children[i]);
	    Event.xmaprequest.window = children[i];
	    HandleMapRequest ();
	  }
      }

    XFree(children);
    /* after the windows already on the screen are in place,
     * don't use PPosition */
    PPosOverride = FALSE;
    Scr.SizeStringWidth = XTextWidth (Scr.StdFont.font,
				      " 8888 x 8888 ", 13);
    attributes.border_pixel = Scr.StdColors.fore;
    attributes.background_pixel = Scr.StdColors.back;
    attributes.bit_gravity = NorthWestGravity;
    valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity);
    Scr.SizeWindow = XCreateWindow (dpy, Scr.Root, 0, 0, 
				     (unsigned int) Scr.SizeStringWidth,
				     (unsigned int) (Scr.StdFont.height +
						     SIZE_VINDENT*2),
				     (unsigned int) BW, 0,
				     (unsigned int) CopyFromParent,
				     (Visual *) CopyFromParent,
				     valuemask, &attributes);
    XUngrabServer(dpy);
    HandleEvents();
    return;
}


/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/
int MappedNotOverride(Window w)
{
  XWindowAttributes wa;
  
  XGetWindowAttributes(dpy, w, &wa);
  return ((wa.map_state != IsUnmapped) && (wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *	InternUsefulAtoms:
 *            Dont really know what it does
 *
 ***********************************************************************
 */
Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_SAVE_YOURSELF;
Atom _XA_WM_DELETE_WINDOW;

void InternUsefulAtoms ()
{
  /* 
   * Create priority colors if necessary.
   */
  _XA_MIT_PRIORITY_COLORS = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", False);   
  _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
  _XA_WM_SAVE_YOURSELF = XInternAtom (dpy, "WM_SAVE_YOURSELF", False);
  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	newhandler: Installs new signal handler
 *
 ***********************************************************************
 */
void newhandler(int sig)
{
  if (signal (sig, SIG_IGN) != SIG_IGN)
    signal (sig, Done);
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads fvwm cursors
 *
 ***********************************************************************
 */
void CreateCursors()
{
  /* define cursors */
  Scr.PositionCursor = XCreateFontCursor(dpy,XC_top_left_corner);
  Scr.FrameCursor = XCreateFontCursor(dpy, XC_top_left_arrow);
  Scr.SysCursor = XCreateFontCursor(dpy, XC_hand2);
  Scr.TitleCursor = XCreateFontCursor(dpy, XC_dotbox);
  Scr.IconCursor = XCreateFontCursor(dpy, XC_top_left_arrow);
  Scr.MoveCursor = XCreateFontCursor(dpy, XC_fleur);
  Scr.ResizeCursor = XCreateFontCursor(dpy, XC_fleur);
  Scr.MenuCursor = XCreateFontCursor(dpy, XC_sb_left_arrow);
  Scr.WaitCursor = XCreateFontCursor(dpy, XC_watch);
  Scr.SelectCursor = XCreateFontCursor(dpy, XC_dot);
  Scr.DestroyCursor = XCreateFontCursor(dpy, XC_pirate);
  Scr.SideBarCursor = XCreateFontCursor(dpy, XC_hand1);
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize fvwm variables
 *
 ***********************************************************************
 */

void InitVariables()
{
  FvwmContext = XUniqueContext();
  NoClass.res_name = NoName;
  NoClass.res_class = NoName;

  Scr.d_depth = DefaultDepth(dpy, 0);
  Scr.d_visual = DefaultVisual(dpy, 0);
  Scr.Root = RootWindow(dpy, 0);
  Scr.FvwmRoot.w = Scr.Root;
  Scr.root_pushes = 0;
  Scr.pushed_window = &Scr.FvwmRoot;

  Scr.MyDisplayWidth = DisplayWidth(dpy, 0);
  Scr.MyDisplayHeight = DisplayHeight(dpy, 0);
    
  Scr.SizeStringOffset = 0;
  Scr.BorderWidth = BW;
  Scr.NoBorderWidth = NOFRAME_BW;
  Scr.BoundaryWidth = BOUNDARY_WIDTH;
  Scr.CornerWidth = CORNER_WIDTH;
  Scr.Focus = NULL;
  
  Scr.StdFont.font = NULL;
  Scr.StdFont.name = "fixed";

  Scr.VScale = 32;
  Scr.VxMax = 2*Scr.MyDisplayWidth;
  Scr.VyMax = 2*Scr.MyDisplayHeight;
  Scr.Vx = Scr.Vy = 0;

  Scr.EdgeScrollX = Scr.EdgeScrollY = 32;
  Scr.CenterOnCirculate = FALSE;
  Scr.DontMoveOff = FALSE;
  Scr.RandomPlacement = False;
  Scr.randomx = Scr.randomy = 0;
  Scr.DecorateTransients = False;
  Scr.ClickToFocus = False;
  Scr.buttons2grab = 7;

  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ***********************************************************************
 */

void Reborder()
{
  FvwmWindow *tmp;			/* temp fvwm window structure */
  /* put a border back around all windows */
  
  XGrabServer (dpy);
  
  InstallWindowColormaps (0, &Scr.FvwmRoot);	/* force reinstall */
  for (tmp = Scr.FvwmRoot.next; tmp != NULL; tmp = tmp->next)
    {
      RestoreWithdrawnLocation (tmp); 
      XMapWindow (dpy, tmp->w);
    }
  XUngrabServer (dpy);
  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

/***********************************************************************
 *
 *  Procedure: NoisyExit
 *	Print error messages and die. (segmentation violation)
 *
 ***********************************************************************
 */
void NoisyExit()
{
  XErrorEvent event;
  fprintf(stderr,"fvwm: Segmentation Violation\n");
  event.error_code = 0;
  event.request_code = 0;
  FvwmErrorHandler(dpy, &event);
  exit(1);
}

/***********************************************************************
 *
 *  Procedure:
 *	Done - cleanup and exit fvwm
 *
 ***********************************************************************
 */
SIGNAL_T Done()
{

  MoveViewport(0,0);
  if(Pager_w != (Window)0)
      XDestroyWindow(dpy,Pager_w);
  Reborder ();
  XCloseDisplay(dpy);
  exit(0);
  SIGNAL_RETURN;
}

/***********************************************************************
 *
 *  Procedure:
 *	CatchRedirectError - Figures out if there's another WM running
 *
 ***********************************************************************
 */
static int CatchRedirectError(Display *dpy, XErrorEvent event)
{
  fprintf (stderr, "fvwm: another window manager is running\n");
  exit(1);
  return 0;
}


/***********************************************************************
 *
 *  Procedure:
 *	FvwmErrorHandler - displays info on internal errors
 *
 ***********************************************************************
 */
static int FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
  extern int move_on, resize_on,menu_on;
  extern Window last_event_window;
  extern FvwmWindow *last_event_Fvwmwindow;
  extern int last_event_type;

  /* some errors are acceptable, mostly they're caused by 
   * trying to update a lost  window */
  if((event->error_code == BadWindow)||
     (event->request_code == X_GetGeometry)||
     (event->error_code == BadDrawable))
    return 0 ;

  fprintf(stderr,"fvwm: internal error\n");
  fprintf(stderr,"      Request %x, Error %x\n", event->request_code,
	  event->error_code);
  fprintf(stderr,"      Handler: ");
  switch(last_event_type)
    {
    case Expose:
      fprintf(stderr, "Expose");
      break;
    case DestroyNotify:
      fprintf(stderr, "Destroy");
      break;
    case MapRequest:
      fprintf(stderr, "MapR");
      break;
    case MapNotify:
      fprintf(stderr,"MapN");
      break;
    case UnmapNotify:
      fprintf(stderr,"Unmap");
      break;
    case MotionNotify:
      fprintf(stderr,"Motion");
      break;
    case ButtonRelease:
      fprintf(stderr,"ButtonR");
      break;
    case ButtonPress:
      fprintf(stderr,"ButtonP");
      break;
    case EnterNotify:
      fprintf(stderr,"Enter");
      break;
    case LeaveNotify:
      fprintf(stderr,"Leave");
      break;
    case ConfigureRequest:
      fprintf(stderr,"ConfigureR");
      break;
    case ClientMessage:
      fprintf(stderr,"ClientM");
      break;
    case PropertyNotify:
      fprintf(stderr,"Property");
      break;
    case KeyPress:
      fprintf(stderr,"KeyP");
      break;
    default:
      fprintf(stderr,"Unknown %d",last_event_type);
      break;
    }
  if(menu_on)
    fprintf(stderr,"   Menu on\n");
  if(resize_on)
    fprintf(stderr,"   Resizing\n");
  if(move_on)
    fprintf(stderr,"   Moving\n");
  return 0;
}
