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


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "misc.h"
#include "screen.h"

extern Bool PPosOverride;
char NoName[] = "Untitled"; /* name if no name is specified */
extern Window Pager_w;
extern Bool KeyboardGrabbed;
/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 *  Returned Value:
 *	(FvwmWindow *) - pointer to the FvwmWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
FvwmWindow *AddWindow(Window w)
{
  FvwmWindow *tmp_win;			/* new fvwm window structure */
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytesafter;
  int gravx, gravy;			/* gravity signs for positioning */
  int DragWidth;
  int DragHeight,xl,yt;
  char *value;

  /* allocate space for the fvwm window */
  tmp_win = (FvwmWindow *)calloc(1, sizeof(FvwmWindow));
  if (tmp_win == (FvwmWindow *)0)
    {
      return NULL;
    }
  tmp_win->flags = 0;
  tmp_win->w = w;

  /*
   * Make sure the client window still exists.  We don't want to leave an
   * orphan frame window if it doesn't.  Since we now have the server 
   * grabbed, the window can't disappear later without having been 
   * reparented, so we'll get a DestroyNotify for it.  We won't have 
   * gotten one for anything up to here, however.
   */
  if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    {
      free((char *)tmp_win);
      return(NULL);
    }

  XSelectInput(dpy, tmp_win->w, PropertyChangeMask);
  XGetWindowAttributes(dpy, tmp_win->w, &tmp_win->attr);
  XFetchName(dpy, tmp_win->w, &tmp_win->name);
  if(tmp_win->name == NULL)
    tmp_win->name = NoName;
  tmp_win->class = NoClass;

  XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
  if (tmp_win->class.res_name == NULL)
    tmp_win->class.res_name = NoName;
  if (tmp_win->class.res_class == NULL)
    tmp_win->class.res_class = NoName;

  FetchWmProtocols (tmp_win);

  tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

  /*
   * The July 27, 1988 draft of the ICCCM ignores the size and position
   * fields in the WM_NORMAL_HINTS property.
   */
  if(XGetTransientForHint(dpy, tmp_win->w,  &tmp_win->transientfor))
    tmp_win->flags |= TRANSIENT;
  else
    tmp_win->flags &= ~TRANSIENT;

  tmp_win->old_bw = tmp_win->attr.border_width;

  /* if the window is in the NoTitle list, or is a transient,
   *  dont decorate it.
   * If its a transient, and DecorateTransients was specified,
   *  decorate anyway
   */
  if ((LookInList(Scr.NoTitle,tmp_win->name, &tmp_win->class, &value))||
      ((!Scr.DecorateTransients) && (tmp_win->flags & TRANSIENT)))
    {
      /* don't decorate */
      tmp_win->frame_bw = Scr.NoBorderWidth;
      tmp_win->title_height = tmp_win->title_width =
 	tmp_win->corner_width = tmp_win->boundary_width = 0;
    }
  else
    {
      /* go ahead and decorate */
      tmp_win->frame_bw = Scr.BorderWidth;
      tmp_win->title_height = Scr.TitleHeight + tmp_win->frame_bw;
      tmp_win->boundary_width = Scr.BoundaryWidth;
      tmp_win->corner_width = tmp_win->title_height + tmp_win->boundary_width;
      tmp_win->flags |= BORDER;
    }

  if (LookInList(Scr.Sticky,tmp_win->name, &tmp_win->class, &value))
    tmp_win->flags |= STICKY;

  if (LookInList(Scr.Icons,tmp_win->name, &tmp_win->class, &value))
    tmp_win->icon_bitmap_file = value;

  if (LookInList(Scr.OnTop,tmp_win->name, &tmp_win->class,&value))
    tmp_win->flags |= ONTOP;
  
  GetWindowSizeHints (tmp_win);

  GetGravityOffsets (tmp_win, &gravx, &gravy);

  /*
   *  If
   *     o  the window is a transient, or
   * 
   *     o  a USPosition was requested
   * 
   *   then put the window where requested.
   *
   *   If RandomPlacement was specified,
   *       then place the window in a psuedo-random location
   */
  if (!(tmp_win->flags & TRANSIENT) && !(tmp_win->hints.flags & USPosition)
      &&!PPosOverride)
    {
      /* Get user's window placement, unless RandomPlacement is specified */
      if(Scr.RandomPlacement)
	{
	  /* plase window in a random location */
	  if ((Scr.randomx += Scr.TitleHeight) > Scr.MyDisplayWidth / 2)
	    Scr.randomx = Scr.TitleHeight;
	  if ((Scr.randomy += 2 * Scr.TitleHeight) > Scr.MyDisplayHeight / 2)
	    Scr.randomy = 2 * Scr.TitleHeight;
	  tmp_win->attr.x = Scr.randomx;
	  tmp_win->attr.y = Scr.randomy;
	}
      else
	{	  
	  /* wait for a button press to place the window */
	  while(XGrabPointer(dpy, Scr.Root, True, ButtonPressMask | 
			     ButtonReleaseMask|ButtonMotionMask |
			     PointerMotionMask, GrabModeAsync, GrabModeAsync,
			     Scr.Root, Scr.PositionCursor,
			     CurrentTime) != GrabSuccess);
	  if(!KeyboardGrabbed)
	    while(XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, 
				GrabModeAsync, CurrentTime)!=GrabSuccess);
	  XGrabServer(dpy);
	  /*
	   * Make sure the client window still exists.  We don't want to leave an
	   * orphan frame window if it doesn't.  Since we now have the server
	   * grabbed, the window can't disappear later without having been
	   * reparented, so we'll get a DestroyNotify for it.  We won't have
	   * gotten one for anything up to here, however.
	   */
	  if(XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
			  (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, 
			  &JunkBW,  &JunkDepth) == 0)
	    {
	      free((char *)tmp_win);
	      XUngrabPointer(dpy,CurrentTime);
	      if(!KeyboardGrabbed)
		XUngrabKeyboard(dpy,CurrentTime);
	      XUngrabServer(dpy);
	      return(NULL);
	    }
	  
	  DragWidth += 2*tmp_win->boundary_width;
	  DragHeight += tmp_win->title_height + 2*tmp_win->boundary_width;
	  
	  moveLoop(tmp_win,0,0,DragWidth,DragHeight, &xl, &yt);
	  tmp_win->attr.y = yt;
	  tmp_win->attr.x = xl;
	  XUngrabPointer(dpy,CurrentTime);
	  if(!KeyboardGrabbed)
	    XUngrabKeyboard(dpy,CurrentTime);
	}
    }
  else 
    {
      /* the USPosition was specified, or the window is a transient, 
       * so place it automatically */
      /*
       * Make sure the client window still exists.  We don't want to leave an
       * orphan frame window if it doesn't.  Since we now have the server
       * grabbed, the window can't disappear later without having been
       * reparented, so we'll get a DestroyNotify for it.  We won't have
       * gotten one for anything up to here, however.
       */
      XGrabServer(dpy);
      if(XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
		   (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, 
		   &JunkBW,  &JunkDepth) == 0)
	{
	  free((char *)tmp_win);
	  XUngrabServer(dpy);
	  return(NULL);
	}

      /* put it where asked, mod title bar */
      /* if the gravity is towards the top, move it by the title height */
      tmp_win->attr.y -= gravy*tmp_win->frame_bw;
      tmp_win->attr.x -= gravx*tmp_win->frame_bw;
      if(gravy > 0)
	tmp_win->attr.y -= 2*tmp_win->boundary_width + tmp_win->title_height;
      if(gravx > 0)
	tmp_win->attr.x -= 2*tmp_win->boundary_width;
    }

  XSetWindowBorderWidth (dpy, tmp_win->w, tmp_win->frame_bw);
  XGetWindowProperty (dpy, tmp_win->w, XA_WM_ICON_NAME, 0L, 200L, False,
			  XA_STRING, &actual_type, &actual_format, &nitems,
			  &bytesafter,(unsigned char **)&tmp_win->icon_name);
  if(tmp_win->icon_name==(char *)NULL)
    tmp_win->icon_name = tmp_win->name;

  tmp_win->flags &= ~ICON;


  /* add the window into the fvwm list */
  tmp_win->next = Scr.FvwmRoot.next;
  if (Scr.FvwmRoot.next != NULL)
    Scr.FvwmRoot.next->prev = tmp_win;
  tmp_win->prev = &Scr.FvwmRoot;
  Scr.FvwmRoot.next = tmp_win;
  
  /* create windows */
  tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->frame_bw;
  tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->frame_bw;
  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;

  if (tmp_win->title_height)
    {

      valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
      attributes.background_pixmap = None;
      attributes.border_pixel = Scr.StdColors.fore;
      attributes.cursor = Scr.FrameCursor;
      attributes.event_mask = (SubstructureRedirectMask | 
			       ButtonPressMask | ButtonReleaseMask |
			       EnterWindowMask | ExposureMask | 
			       LeaveWindowMask);
      tmp_win->frame = 
	XCreateWindow (dpy, Scr.Root, tmp_win->frame_x,tmp_win->frame_y, 
		       tmp_win->frame_width, tmp_win->frame_height,
		       tmp_win->frame_bw,Scr.d_depth, CopyFromParent,
		       Scr.d_visual, valuemask, &attributes);
      tmp_win->lead_w = tmp_win->frame;

      XSetWindowBackground(dpy,tmp_win->frame,Scr.StdColors.back);

      valuemask = (CWEventMask | CWBorderPixel | CWBackPixel |
		   CWCursor);
      attributes.event_mask = (ButtonPressMask|ButtonReleaseMask|ExposureMask);
      attributes.border_pixel = Scr.StdColors.fore;
      attributes.background_pixel = Scr.StdColors.back;
      attributes.cursor = Scr.SideBarCursor;
      tmp_win->title_x = tmp_win->boundary_width +tmp_win->title_height+1;
      tmp_win->title_y = tmp_win->boundary_width;
      tmp_win->title_width = tmp_win->frame_width - 2*tmp_win->boundary_width 
	- 2*tmp_win->title_height-2;
      if(tmp_win->title_width < 1)
	tmp_win->title_width = 1;
      tmp_win->top_w = 
	XCreateWindow (dpy, tmp_win->frame,tmp_win->corner_width, 0,
		       (tmp_win->frame_width - 2*tmp_win->corner_width),
		       tmp_win->boundary_width, 0, Scr.d_depth,
		       CopyFromParent, Scr.d_visual, valuemask, &attributes);

      attributes.cursor = Scr.TitleCursor;
      tmp_win->title_w = 
	XCreateWindow (dpy, tmp_win->frame, tmp_win->title_x, tmp_win->title_y,
		       tmp_win->title_width, tmp_win->title_height,0,
		       Scr.d_depth, CopyFromParent, Scr.d_visual, valuemask,
		       &attributes);
      attributes.cursor = Scr.SysCursor;
      tmp_win->sys_w = 
	XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->title_height,
		       tmp_win->title_height, 0, Scr.d_depth, CopyFromParent,
		       Scr.d_visual, valuemask, &attributes);

      attributes.cursor = Scr.SysCursor;
      tmp_win->iconify_w = 
	XCreateWindow (dpy, tmp_win->frame,
		       (tmp_win->title_width - tmp_win->title_height), 0, 
		       tmp_win->title_height,tmp_win->title_height,0,
		       Scr.d_depth, CopyFromParent,Scr.d_visual, valuemask, 
		       &attributes);

      attributes.cursor = Scr.SideBarCursor;
      tmp_win->right_side_w = 
	XCreateWindow (dpy, tmp_win->frame, 
		       tmp_win->frame_width - tmp_win->boundary_width, 
		       tmp_win->corner_width, tmp_win->boundary_width,
		       tmp_win->frame_height - 2*tmp_win->corner_width,	
		       0, Scr.d_depth, CopyFromParent,
		       Scr.d_visual, valuemask, &attributes);
      attributes.cursor = Scr.SideBarCursor;
      tmp_win->left_side_w = 
	XCreateWindow (dpy, tmp_win->frame, 0, tmp_win->corner_width,
		       tmp_win->boundary_width-tmp_win->frame_bw,
		       tmp_win->frame_height - 2*tmp_win->corner_width,	
		       0, Scr.d_depth, CopyFromParent,
		       Scr.d_visual, valuemask, &attributes);
      attributes.cursor = Scr.SideBarCursor;
      tmp_win->bottom_w = 
	XCreateWindow (dpy, tmp_win->frame, tmp_win->corner_width,
		       tmp_win->frame_height - tmp_win->boundary_width,
		       (tmp_win->frame_width - 2*tmp_win->corner_width),
		       tmp_win->boundary_width, 0, Scr.d_depth,
		       CopyFromParent,Scr.d_visual, valuemask, &attributes);
    }
  else
    {
      tmp_win->title_w = 0;
      tmp_win->lead_w = tmp_win->w;
    }
  SetBorder (tmp_win, False,True,False);
  valuemask = (CWEventMask | CWDontPropagate);
  attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
			   ColormapChangeMask | VisibilityChangeMask |
			   EnterWindowMask | LeaveWindowMask);
  if(tmp_win->w == Pager_w)
    {
      attributes.event_mask |=ButtonReleaseMask|ExposureMask;
      attributes.do_not_propagate_mask = ButtonPressMask;
    }
  else
    attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
  XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);

  XMapSubwindows (dpy, tmp_win->lead_w);

  if(tmp_win->w != Pager_w)
    XAddToSaveSet(dpy, tmp_win->w);
  
  XReparentWindow(dpy, tmp_win->w, tmp_win->frame, tmp_win->boundary_width, 
		  tmp_win->boundary_width+tmp_win->title_height);

  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to FALSE to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
   * again in HandleMapNotify.
   */
  tmp_win->flags &= ~MAPPED;
  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,
	      tmp_win->frame_width, tmp_win->frame_height, True);

  /* wait until the window is iconified and the icon window is mapped
   * before creating the icon window 
   */
  tmp_win->icon_w = (int)NULL;
  GrabButtons(tmp_win);
  GrabKeys(tmp_win);

  XSaveContext(dpy, tmp_win->w, FvwmContext, (caddr_t) tmp_win);
  if (tmp_win->title_w)
    {
      XSaveContext(dpy, tmp_win->frame, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->title_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->left_side_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->right_side_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->bottom_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->top_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->sys_w, FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->iconify_w, FvwmContext, (caddr_t) tmp_win);
    }

  if(Scr.ClickToFocus)
    SetBorder (tmp_win, True,False,False);
  InstallWindowColormaps(EnterNotify, tmp_win);
  Scr.LastWindowRaised = tmp_win;
  XRaiseWindow(dpy,tmp_win->lead_w);
  KeepOnTop();
  XUngrabServer(dpy);
  RedrawPager();
  return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabButtons(FvwmWindow *tmp_win)
{
  MouseButton *MouseEntry;

  MouseEntry = Scr.MouseButtonRoot;
  while(MouseEntry != (MouseButton *)0)
    {
      if((MouseEntry->func != (int)0)&&(MouseEntry->Context & C_WINDOW))
	XGrabButton(dpy, MouseEntry->Button, MouseEntry->Modifier, 
		    tmp_win->w, 
		    True, ButtonPressMask | ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync, None, 
		    Scr.FrameCursor);
      
      MouseEntry = MouseEntry->NextButton;
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabKeys(tmp_win)
     FvwmWindow *tmp_win;
{
  FuncKey *tmp;
  for (tmp = Scr.FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
      if(tmp->cont & C_WINDOW)
	XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
		 GrabModeAsync, GrabModeAsync);
      if ((tmp->cont & C_ICON)&&(tmp_win->icon_w))
	XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->icon_w, True,
		 GrabModeAsync, GrabModeAsync);
      if((tmp->cont & C_TITLE)&&(tmp_win->title_w))
	XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->title_w, True,
		 GrabModeAsync, GrabModeAsync);
      if((tmp->cont & C_SYS)&&(tmp_win->sys_w))
	XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->sys_w, True,
		 GrabModeAsync, GrabModeAsync);
      if((tmp->cont & C_ICONIFY)&&(tmp_win->iconify_w))
	XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->iconify_w, True,
		 GrabModeAsync, GrabModeAsync);
      if((tmp->cont & C_SIDEBAR)&&(tmp_win->left_side_w))
	{
	  XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->left_side_w, True,
		   GrabModeAsync, GrabModeAsync);
	  XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->right_side_w, True,
		   GrabModeAsync, GrabModeAsync);
	  XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->bottom_w, True,
		   GrabModeAsync, GrabModeAsync);
	  XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->top_w, True,
		   GrabModeAsync, GrabModeAsync);
	}
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols (FvwmWindow *tmp)
{
  unsigned long flags = 0L;
  Atom *protocols = NULL, *ap;
  int i, n;
  
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n)) 
    {
      for (i = 0, ap = protocols; i < n; i++, ap++) 
	{
	  if (*ap == _XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	  if (*ap == _XA_WM_SAVE_YOURSELF) flags |= DoesWmSaveYourself;
	  if (*ap == _XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
      if (protocols) XFree ((char *) protocols);
    }
  tmp->protocols = flags;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void GetWindowSizeHints(FvwmWindow *tmp)
{
  long supplied = 0;
  static int gravs[] = { SouthEastGravity, SouthWestGravity,
			   NorthEastGravity, NorthWestGravity };
  int right,bottom;

  if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
    tmp->hints.flags = 0;
  
  if (tmp->hints.flags & PResizeInc) 
    {
      if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
      if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }
  
  if (!(supplied & PWinGravity) && 
      ((tmp->hints.flags & USPosition)||
       ((tmp->hints.flags & PPosition)&&(PPosOverride))))
    {
      right =  tmp->attr.x + tmp->attr.width + 2 * tmp->old_bw;
      bottom = tmp->attr.y + tmp->attr.height + 2 * tmp->old_bw;
      tmp->hints.win_gravity = 
	gravs[((Scr.MyDisplayHeight - bottom < (tmp->title_height+tmp->boundary_width)) ? 0 : 2) |
	      ((Scr.MyDisplayWidth - right   < tmp->boundary_width) ? 0 : 1)];
      tmp->hints.flags |= PWinGravity;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	LookInList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	the ptr field of the list structure or NULL if the name 
 *	or class was not found in the list
 *
 *  Inputs:
 *	list	- a pointer to the head of a list
 *	name	- a pointer to the name to look for
 *	class	- a pointer to the class to look for
 *
 ***********************************************************************/
int LookInList(name_list *list, char *name, XClassHint *class, char **value)
{
  name_list *nptr;

  /* look for the name first */
  for (nptr = list; nptr != NULL; nptr = nptr->next)
    if (strcmp(name, nptr->name) == 0)
      {
	*value = nptr->value;
	return True;
      }
  
  if (class)
    {
      /* look for the res_name next */
      for (nptr = list; nptr != NULL; nptr = nptr->next)
	if (strcmp(class->res_name, nptr->name) == 0)
	  {
	    *value = nptr->value;
	    return True;
	  }
      
      /* finally look for the res_class */
      for (nptr = list; nptr != NULL; nptr = nptr->next)
	if (strcmp(class->res_class, nptr->name) == 0)
	  {
	    *value = nptr->value;
	    return True;
	  }
    }
  *value = (char *)0;
  return False;
}


/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 * 
 ************************************************************************/
struct _gravity_offset 
{
  int x, y;
};

void GetGravityOffsets (FvwmWindow *tmp, int *xp, int *yp)
{
  static struct _gravity_offset gravity_offsets[11] = 
    {
      {  0,  0 },			/* ForgetGravity */
      { -1, -1 },			/* NorthWestGravity */
      {  0, -1 },			/* NorthGravity */
      {  1, -1 },			/* NorthEastGravity */
      { -1,  0 },			/* WestGravity */
      {  0,  0 },			/* CenterGravity */
      {  1,  0 },			/* EastGravity */
      { -1,  1 },			/* SouthWestGravity */
      {  0,  1 },			/* SouthGravity */
      {  1,  1 },			/* SouthEastGravity */
      {  0,  0 },			/* StaticGravity */
    };
  register int g = ((tmp->hints.flags & PWinGravity) 
		    ? tmp->hints.win_gravity : NorthWestGravity);
  
  if (g < ForgetGravity || g > StaticGravity) 
    *xp = *yp = 0;
  else 
    {
      *xp = gravity_offsets[g].x;
      *yp = gravity_offsets[g].y;
    }
  return;
}

