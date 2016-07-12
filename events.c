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
 *
 * fvwm event handling
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);
extern int menuFromFrameOrWindowOrTitlebar;

char *Action;
int Context = C_NO_CONTEXT;	/* current button press context */
FvwmWindow *ButtonWindow;	/* button press window structure */
XEvent ButtonEvent;		/* button press event */
XEvent Event;			/* the current event */
FvwmWindow *Tmp_win;		/* the current fvwm window */

void flush_expose();

int last_event_type=0;
FvwmWindow *last_event_Fvwmwindow=0;
Window last_event_window=0;

extern Window Pager_w;
extern Window keyboardRevert;
Bool KeyboardGrabbed;

/***********************************************************************
 *
 *  Procedure:
 *	DispatchEvent - handle a single X event stored in global var Event
 *
 ***********************************************************************
 */
void DispatchEvent()
{
    Window w = Event.xany.window;

    if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
      Tmp_win = NULL;
    last_event_type = Event.type;
    last_event_Fvwmwindow = Tmp_win;
    last_event_window = w;

    switch(Event.type)
      {
      case Expose:
	HandleExpose();
	break;
      case DestroyNotify:
	HandleDestroyNotify();
	break;
      case MapRequest:
	HandleMapRequest();
	break;
      case MapNotify:
	HandleMapNotify();
	break;
      case UnmapNotify:
	HandleUnmapNotify();
	break;
      case MotionNotify:
	HandleMotionNotify();
	break;
      case ButtonPress:
	HandleButtonPress();
	break;
      case ButtonRelease:
	HandleButtonRelease();
	break;
      case EnterNotify:
	HandleEnterNotify();
	break;
      case LeaveNotify:
	HandleLeaveNotify();
	break;
      case ConfigureRequest:
	HandleConfigureRequest();
	break;
      case ClientMessage:
	HandleClientMessage();
	break;
      case PropertyNotify:
	HandlePropertyNotify();
	break;
      case KeyPress:
	HandleKeyPress();
	break;
      case VisibilityNotify:
	HandleVisibilityNotify();
	break;
      default:
	break;
      }
    last_event_type =0;
    return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleEvents - handle X events
 *
 ************************************************************************/

void HandleEvents()
{
  while (TRUE)
    {
      XNextEvent(dpy, &Event);
      DispatchEvent ();
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleKeyPress - key press event handler
 *
 ************************************************************************/

void HandleKeyPress()
{
  FuncKey *key;
  unsigned int modifier;
  
  Context = C_NO_CONTEXT;
  
  if (Event.xany.window == Scr.Root)
    Context = C_ROOT;
  if (Tmp_win)
    {
      if (Event.xany.window == Tmp_win->title_w)
	Context = C_TITLE;
      if (Event.xany.window == Tmp_win->w)
	Context = C_WINDOW;
      if (Event.xany.window == Tmp_win->icon_w)
	Context = C_ICON;
      if (Event.xany.window == Tmp_win->frame)
	Context = C_FRAME;
      if ((Event.xany.window == Tmp_win->left_side_w)||
	  (Event.xany.window == Tmp_win->right_side_w)||
	  (Event.xany.window == Tmp_win->top_w)||
	  (Event.xany.window == Tmp_win->bottom_w))
	Context = C_SIDEBAR;
      if (Event.xany.window == Tmp_win->sys_w)
	Context = C_SYS;
      if (Event.xany.window == Tmp_win->iconify_w)
	Context = C_ICONIFY;
    }

  modifier = (Event.xkey.state & mods_used);
  for (key = Scr.FuncKeyRoot.next; key != NULL; key = key->next)
    {
      ButtonWindow = Tmp_win;
      if ((key->keycode == Event.xkey.keycode) &&
	  (key->mods == modifier) && (key->cont & Context))
	{
	  ExecuteFunction(key->func, key->action, Event.xany.window,Tmp_win,
			  &Event,Context,key->val1,key->val2,key->menu);
	  return;
	}
    }
  
  /* if we get here, no function key was bound to the key.  Send it
   * to the client if it was in a window we know about.
   */
  if (Tmp_win)
    {
      Event.xkey.window = Tmp_win->w;
      XSendEvent(dpy, Tmp_win->w, False, KeyPressMask, &Event);
    }
}


/**************************************************************************
 * 
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon)
{
  if (tmp->icon_name == tmp->name) nukename = False;
  
  if (nukename && tmp && tmp->name != NoName) XFree (tmp->name);
  if (nukeicon && tmp && tmp->icon_name != NoName) XFree (tmp->icon_name);
  return;
}




/***********************************************************************
 *
 *  Procedure:
 *	HandlePropertyNotify - property notify event handler
 *
 ***********************************************************************/
#define MAX_NAME_LEN 200L		/* truncate to this many */
#define MAX_ICON_NAME_LEN 200L		/* ditto */

void HandlePropertyNotify()
{
  char *prop = NULL;
  Atom actual = None;
  int actual_format;
  unsigned long nitems, bytesafter;
  switch (Event.xproperty.atom) 
    {
    case XA_WM_NAME:
      if (XGetWindowProperty (dpy, Tmp_win->w, Event.xproperty.atom, 0L, 
			      MAX_NAME_LEN, False, XA_STRING, &actual,
			      &actual_format, &nitems, &bytesafter,
			      (unsigned char **) &prop) != Success ||
	  actual == None)
	return;
      if (!prop) prop = NoName;
      free_window_names (Tmp_win, True, False);
      
      Tmp_win->name = prop;
      
      /* fix the name in the title bar */
      if(!(Tmp_win->flags & ICON))
	{
	  if(Scr.Focus == Tmp_win)
	    SetTitleBar(Tmp_win,True);
	  else
	    SetTitleBar(Tmp_win,False);
	}
      
      /*
       * if the icon name is NoName, set the name of the icon to be
       * the same as the window 
       */
      if (Tmp_win->icon_name == NoName) 
	{
	  Tmp_win->icon_name = Tmp_win->name;
	  RedoIconName(Tmp_win);
	}
      break;
      
    case XA_WM_ICON_NAME:
      if (XGetWindowProperty (dpy, Tmp_win->w, Event.xproperty.atom, 0, 
			      MAX_ICON_NAME_LEN, False, XA_STRING, &actual,
			      &actual_format, &nitems, &bytesafter,
			      (unsigned char **) &prop) != Success ||
	  actual == None)
	return;
      if (!prop) prop = NoName;
      free_window_names (Tmp_win, False, True);
      Tmp_win->icon_name = prop;
      RedoIconName(Tmp_win);
      break;
      
    case XA_WM_HINTS:
      if (Tmp_win->wmhints) 
	XFree ((char *) Tmp_win->wmhints);
      Tmp_win->wmhints = XGetWMHints(dpy, Event.xany.window);
      break;
      
    case XA_WM_NORMAL_HINTS:
      GetWindowSizeHints (Tmp_win);
      break;
      
    default:
      if (Event.xproperty.atom == _XA_WM_PROTOCOLS) 
	FetchWmProtocols (Tmp_win);
      break;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleClientMessage - client message event handler
 *
 ************************************************************************/

void HandleClientMessage()
{
  XEvent button;

  if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE)&&
      (Tmp_win != NULL)&&(Event.xclient.data.l[0]==IconicState)&&
      !(Tmp_win->flags & ICON))
    {
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &(button.xmotion.x_root),
		    &(button.xmotion.y_root),
		    &JunkX, &JunkY, &JunkMask);
      
      ExecuteFunction(F_ICONIFY, NULLSTR, Event.xany.window,
		      Tmp_win, &button, C_FRAME,0,0, (MenuRoot *)0);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleExpose - expose event handler
 *
 ***********************************************************************/
void HandleExpose()
{
  if (Event.xexpose.count != 0)
    return;

  if (Tmp_win != NULL)
    {
      if(Tmp_win->w == Pager_w)
	RedrawPager();

      if ((Event.xany.window == Tmp_win->title_w))
	{
	  if(Scr.Focus == Tmp_win)
	    SetTitleBar(Tmp_win,True);
	  else
	    SetTitleBar(Tmp_win,False);
	}
      else if (Event.xany.window == Tmp_win->icon_w)
	{
	  DrawIconWindow(Tmp_win);
	  flush_expose (Event.xany.window);
	}
      else
	{
	  if(Scr.Focus == Tmp_win)
	    SetBorder(Tmp_win,True,True,True);
	  else
	    SetBorder(Tmp_win,False,True,True);
	}
    }
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleDestroyNotify - DestroyNotify event handler
 *
 ***********************************************************************/
void Destroy(FvwmWindow *Tmp_win)
{ 
  XEvent dummy;
  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(Tmp_win->frame)
    XDestroyWindow(dpy, Tmp_win->frame);

  if (Tmp_win->icon_w) 
    XDestroyWindow(dpy, Tmp_win->icon_w);

  if (Tmp_win == NULL)
    return;

  if (Tmp_win == Scr.Focus)
    {
      Scr.Focus = NULL;
      XSetInputFocus (dpy, Scr.Root, RevertToPointerRoot, CurrentTime);
      XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
		    CurrentTime);
      KeyboardGrabbed = True;
      InstallWindowColormaps(0, &Scr.FvwmRoot);
    }
  XDeleteContext(dpy, Tmp_win->w, FvwmContext);
  if (Tmp_win->icon_w)
    XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
  if (Tmp_win->title_height)
  {
      XDeleteContext(dpy, Tmp_win->title_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->sys_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->iconify_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->left_side_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->right_side_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->bottom_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->top_w, FvwmContext);
      XDeleteContext(dpy, Tmp_win->frame, FvwmContext);
    }

  Tmp_win->prev->next = Tmp_win->next;
  if (Tmp_win->next != NULL)
    Tmp_win->next->prev = Tmp_win->prev;
  free_window_names (Tmp_win, True, True);		
  if (Tmp_win->wmhints)					
    XFree ((char *)Tmp_win->wmhints);
  if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoName)  
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoName) 
    XFree ((char *)Tmp_win->class.res_class);
  
  free((char *)Tmp_win);
  RedrawPager();
  return;
}


void HandleDestroyNotify()
{
  Destroy(Tmp_win);
}




/***********************************************************************
 *
 *  Procedure:
 *	HandleMapRequest - MapRequest event handler
 *
 ************************************************************************/
void HandleMapRequest()
{
  Event.xany.window = Event.xmaprequest.window;
  if(XFindContext(dpy, Event.xany.window, FvwmContext, 
		      (caddr_t *)&Tmp_win)==XCNOENT)
    Tmp_win = NULL;

  XFlush(dpy);

  /* If the window has never been mapped before ... */
  if(Tmp_win == NULL)
    {
      /* Add decorations. */
      Tmp_win = AddWindow(Event.xany.window);
      if (Tmp_win == NULL)
	return;
    }

  /* If it's not merely iconified, and we have hints, use them. */
  if ((!(Tmp_win->flags & ICON)) &&
      Tmp_win->wmhints && (Tmp_win->wmhints->flags & StateHint))
    {
      int state;

      state = Tmp_win->wmhints->initial_state;
      switch (state) 
	{
	case DontCareState:
	case NormalState:
	case InactiveState:
	  XMapWindow(dpy, Tmp_win->w);
	  XMapWindow(dpy, Tmp_win->frame);
	  Tmp_win->flags |= MAPPED;
	  SetMapStateProp(Tmp_win, NormalState);
	  break;
	  
	case IconicState:
	  Iconify(Tmp_win, 0, 0);
	  break;
	}
    }
  /* If no hints, or currently an icon, just "deiconify" */
  else
    {
      DeIconify(Tmp_win);
    }
  KeepOnTop();
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void HandleMapNotify()
{
  if (Tmp_win == NULL)
    return;
  /*
   * Need to do the grab to avoid race condition of having server send
   * MapNotify to client before the frame gets mapped; this is bad because
   * the client would think that the window has a chance of being viewable
   * when it really isn't.
   */
  XGrabServer (dpy);
  if (Tmp_win->icon_w)
    XUnmapWindow(dpy, Tmp_win->icon_w);
  XMapSubwindows(dpy, Tmp_win->lead_w);
  XMapWindow(dpy, Tmp_win->lead_w);
  if(Scr.Focus == Tmp_win)
    {
      XSetInputFocus (dpy, Tmp_win->w, RevertToPointerRoot, CurrentTime);
      XUngrabKeyboard(dpy, CurrentTime);
      KeyboardGrabbed = False;
    }
  XUngrabServer (dpy);
  XFlush (dpy);
  Tmp_win->flags |= MAPPED;
  Tmp_win->flags &= ~ICON;
  KeepOnTop();
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleUnmapNotify - UnmapNotify event handler
 *
 ************************************************************************/
void HandleUnmapNotify()
{
  int dstx, dsty;
  Window dumwin;
  XEvent dummy;

  /*
   * The July 27, 1988 ICCCM spec states that a client wishing to switch
   * to WithdrawnState should send a synthetic UnmapNotify with the
   * event field set to (pseudo-)root, in case the window is already
   * unmapped (which is the case for fvwm for IconicState).  Unfortunately,
   * we looked for the FvwmContext using that field, so try the window
   * field also.
   */
  if (Tmp_win == NULL)
    {
      Event.xany.window = Event.xunmap.window;
      if (XFindContext(dpy, Event.xany.window,
		       FvwmContext, (caddr_t *)&Tmp_win) == XCNOENT)
	Tmp_win = NULL;
    }
  
  if(Scr.Focus == Tmp_win)
    {
      XSetInputFocus (dpy, Scr.Root,RevertToPointerRoot, CurrentTime);
      XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
		    CurrentTime);
      KeyboardGrabbed = True;
      Scr.Focus = NULL;
    }
  if (Tmp_win == NULL || (!(Tmp_win->flags & MAPPED)&&!(Tmp_win->flags&ICON)))
    return;

  XGrabServer (dpy);
  if(XCheckTypedWindowEvent (dpy, Event.xunmap.window, DestroyNotify,&dummy)) 
    {
      Destroy(Tmp_win);
      XUngrabServer(dpy);
      return;
    }
      
  /*
   * The program may have unmapped the client window, from either
   * NormalState or IconicState.  Handle the transition to WithdrawnState.
   *
   * We need to reparent the window back to the root (so that fvwm exiting 
   * won't cause it to get mapped) and then throw away all state (pretend 
   * that we've received a DestroyNotify).
   */
  if (XTranslateCoordinates (dpy, Event.xunmap.window, Tmp_win->attr.root,
			     0, 0, &dstx, &dsty, &dumwin)) 
    {
      XEvent ev;
      Bool reparented;
      reparented = XCheckTypedWindowEvent (dpy, Event.xunmap.window, 
						ReparentNotify, &ev);
      SetMapStateProp (Tmp_win, WithdrawnState);
      if (reparented) 
	{
	  if (Tmp_win->old_bw)
	    XSetWindowBorderWidth (dpy, Event.xunmap.window, Tmp_win->old_bw);
	  if (Tmp_win->wmhints && (Tmp_win->wmhints->flags & IconWindowHint))
	    XUnmapWindow (dpy, Tmp_win->wmhints->icon_window);
	} 
      else
	{
	  XReparentWindow (dpy, Event.xunmap.window, Tmp_win->attr.root,
			   dstx, dsty);
	  RestoreWithdrawnLocation (Tmp_win);
	}
      XRemoveFromSaveSet (dpy, Event.xunmap.window);
      XSelectInput (dpy, Event.xunmap.window, NoEventMask);
      HandleDestroyNotify ();		/* do not need to mash event before */
    } /* else window no longer exists and we'll get a destroy notify */
  XUngrabServer (dpy);
  XFlush (dpy);
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMotionNotify - MotionNotify event handler
 *
 **********************************************************************/
void HandleMotionNotify()
{
  int delta_x, delta_y;
  int warp_x,warp_y;

  while(XCheckTypedEvent(dpy,MotionNotify,&Event));
  delta_x = delta_y = warp_x = warp_y = 0;
  if(Event.xmotion.x_root >= Scr.MyDisplayWidth -2)
    {
      delta_x = Scr.EdgeScrollX;
      warp_x = Scr.EdgeScrollX - 4;
    }
  if(Event.xmotion.y_root >= Scr.MyDisplayHeight -2)
    {
      delta_y = Scr.EdgeScrollY;
      warp_y = Scr.EdgeScrollY - 4;      
    }
  if(Event.xmotion.x_root < 2)
    {
      delta_x = -Scr.EdgeScrollX;
      warp_x =  -Scr.EdgeScrollX + 4;
    }
  if(Event.xmotion.y_root < 2)
    {
      delta_y = -Scr.EdgeScrollY;
      warp_y =  -Scr.EdgeScrollY + 4;
    }
  if(Scr.Vx + delta_x < 0)
    delta_x = -Scr.Vx;
  if(Scr.Vy + delta_y < 0)
    delta_y = -Scr.Vy;
  if(Scr.Vx + delta_x > Scr.VxMax)
    delta_x = Scr.VxMax - Scr.Vx;
  if(Scr.Vy + delta_y > Scr.VyMax)
    delta_y = Scr.VyMax - Scr.Vy;
  if((delta_x!=0)||(delta_y!=0))
    {
      XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		   Scr.MyDisplayHeight, 
		   Event.xmotion.x_root - warp_x,
		   Event.xmotion.y_root - warp_y);
      MoveViewport(Scr.Vx + delta_x,Scr.Vy+delta_y);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 *
 ***********************************************************************/
void HandleButtonPress()
{
  unsigned int modifier;
  MouseButton *MouseEntry;

  /* click to focus stuff goes here */
  if((Scr.ClickToFocus)&&(Tmp_win != Scr.Focus))
    {
      if(Tmp_win)
	{
	  SetBorder (Tmp_win, True,False,True);
	  XRaiseWindow(dpy,Tmp_win->lead_w);
	  KeepOnTop();
	}
      else
	{
	  SetBorder (Scr.Focus, False,False,True);
	  Scr.Focus = NULL;
	}
    }


  /* regular button press stuff goes here */
  Context = C_NO_CONTEXT;
  if (Event.xany.window == Scr.Root)
    Context = C_ROOT;
  if (Tmp_win)
    {
      if (Event.xany.window == Tmp_win->title_w)
	Context = C_TITLE;
      if ((Event.xany.window == Tmp_win->left_side_w)||
	  (Event.xany.window == Tmp_win->right_side_w)||
	  (Event.xany.window == Tmp_win->top_w)||
	  (Event.xany.window == Tmp_win->bottom_w))
	Context = C_SIDEBAR;
      else if (Event.xany.window == Tmp_win->w) 
	Context = C_WINDOW;
      else if (Event.xany.window == Tmp_win->icon_w)
	Context = C_ICON;
      else if (Event.xany.window == Tmp_win->sys_w)
	Context = C_SYS;
      else if (Event.xany.window == Tmp_win->iconify_w)
	Context = C_ICONIFY;
      else if (Event.xany.window == Tmp_win->frame) 
	Context = C_FRAME;
    }
  ButtonEvent = Event;
  ButtonWindow = Tmp_win;
  
  /* we have to execute a function or pop up a menu
   */
  modifier = (Event.xbutton.state & mods_used);
  /* need to search for an appropriate mouse binding */
  MouseEntry = Scr.MouseButtonRoot;
  while(MouseEntry != (MouseButton *)0)
    {
      if((MouseEntry->Button == Event.xbutton.button)&&
	 (MouseEntry->Context & Context)&&
	 ((MouseEntry->Modifier == -1)||
	  (MouseEntry->Modifier == modifier)))
	{
	  /* got a match, now process it */
	  if (MouseEntry->func != (int)NULL)
	    {
	      Action = MouseEntry->item ? MouseEntry->item->action : NULL;
	      ExecuteFunction(MouseEntry->func, Action, Event.xany.window, 
			      Tmp_win, &Event, Context,MouseEntry->val1,
			      MouseEntry->val2,MouseEntry->menu);
	      break;
	    }
	}
      MouseEntry = MouseEntry->NextButton;
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonRelease - ButtonRelease event handler
 *
 ***********************************************************************/
void HandleButtonRelease()
{
  if((Tmp_win)&&(Event.xany.window == Pager_w))
    {
      switch(Event.xbutton.button)
	{
	default:
	case Button1:
	  SwitchPages(TRUE);
	  break;
	case Button2:
	  NextPage();
	  break;
	case Button3:
	  SwitchPages(FALSE);
	  break;
	}
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleEnterNotify - EnterNotify event handler
 *
 ************************************************************************/
void HandleEnterNotify()
{
  XEnterWindowEvent *ewp = &Event.xcrossing;
  XEvent d;

  /* look for a matching leaveNotify which would nullify this enterNotify */
  if(XCheckTypedWindowEvent (dpy, ewp->window, LeaveNotify, &d))
    if((d.xcrossing.mode==NotifyNormal)&&(d.xcrossing.detail!=NotifyInferior))
      return;

  /* make sure its for one of our windows */
  if (!Tmp_win) 
    return;
  
  if(!Scr.ClickToFocus)
    SetBorder (Tmp_win, True,False,True); 	      
  if(!(Tmp_win->flags & ICON))
    if(Tmp_win->w == Event.xcrossing.window)
      InstallWindowColormaps(EnterNotify, Tmp_win);
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleLeaveNotify - LeaveNotify event handler
 *
 ************************************************************************/
void HandleLeaveNotify()
{
  XEvent dummy;

  /* look for a matching EnterNotify which would nullify this LeaveNotify*/
  if(XCheckTypedWindowEvent (dpy, Event.xcrossing.window, EnterNotify, &dummy))
    if (dummy.xcrossing.mode != NotifyGrab)
      return;
  
  if (Tmp_win == NULL) 
    return;
  /*
   * We're not interested in pseudo Enter/Leave events generated
   * from grab initiations and terminations.
   */
  if (Event.xcrossing.mode != NotifyNormal)
    return;
  if (Event.xcrossing.detail != NotifyInferior) 
    {
      if((Event.xcrossing.window==Tmp_win->frame)||
	 ((Event.xcrossing.window==Tmp_win->w)&&
	  (Tmp_win->title_height == 0)))
	{
	  if(!Scr.ClickToFocus)
	    SetBorder (Tmp_win, False,False,True);
	  InstallWindowColormaps (LeaveNotify, &Scr.FvwmRoot);
	}
      else if(Event.xcrossing.window == Tmp_win->w)
	{
	  InstallWindowColormaps (LeaveNotify, &Scr.FvwmRoot);
	}
    }
  XSync (dpy, 0);
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	HandleConfigureRequest - ConfigureRequest event handler
 *
 ************************************************************************/
void HandleConfigureRequest()
{
  XWindowChanges xwc;
  unsigned long xwcm;
  int x, y, width, height;
  int gravx, gravy;
  XConfigureRequestEvent *cre = &Event.xconfigurerequest;
  
  /*
   * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
   * be wrong
   */
  Event.xany.window = cre->window;	/* mash parent field */
  if (XFindContext (dpy, cre->window, FvwmContext, (caddr_t *) &Tmp_win) ==
      XCNOENT)
    Tmp_win = NULL;
  
  /*
   * According to the July 27, 1988 ICCCM draft, we should ignore size and
   * position fields in the WM_NORMAL_HINTS property when we map a window.
   * Instead, we'll read the current geometry.  Therefore, we should respond
   * to configuration requests for windows which have never been mapped.
   */
  if (!Tmp_win || Tmp_win->icon_w == cre->window) 
    {
      xwcm = cre->value_mask & 
	(CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
      xwc.x = cre->x;
      xwc.y = cre->y;
      if((Tmp_win)&&(Tmp_win->icon_w == cre->window))
	{
	  Tmp_win->icon_x_loc = cre->x;
	  Tmp_win->icon_y_loc = cre->y;
	}
      
      xwc.width = cre->width;
      xwc.height = cre->height;
      xwc.border_width = cre->border_width;
      XConfigureWindow(dpy, Event.xany.window, xwcm, &xwc);
      return;
    }
  
  if (cre->value_mask & CWStackMode) 
    {
      FvwmWindow *otherwin;
      
      xwc.sibling = (((cre->value_mask & CWSibling) &&
		      (XFindContext (dpy, cre->above, FvwmContext,
				     (caddr_t *) &otherwin) == XCSUCCESS))
		     ? otherwin->frame : cre->above);
      xwc.stack_mode = cre->detail;
      XConfigureWindow (dpy, Tmp_win->lead_w,
			cre->value_mask & (CWSibling | CWStackMode), &xwc);
    }

  
  /* Don't modify frame_XXX fields before calling SetupWindow! */
  x = Tmp_win->frame_x;
  y = Tmp_win->frame_y;
  width = Tmp_win->frame_width;
  height = Tmp_win->frame_height;
  
  /*
   * Section 4.1.5 of the ICCCM states that the (x,y) coordinates in the
   * configure request are for the upper-left outer corner of the window.
   * This means that we need to adjust for the additional title height as
   * well as for any border width changes that we decide to allow.  The
   * current window gravity is to be used in computing the adjustments, just
   * as when initially locating the window.  Note that if we do decide to 
   * allow border width changes, we will need to send the synthetic 
   * ConfigureNotify event.
   */
  GetGravityOffsets (Tmp_win, &gravx, &gravy);

 /* for restoring */  
  if (cre->value_mask & CWBorderWidth) 
    Tmp_win->old_bw = cre->border_width; 
  /* override even if border change */
  if (cre->value_mask & CWX)
    x = cre->x + Tmp_win->old_bw - Tmp_win->frame_bw;
  if (cre->value_mask & CWY) 
    y = cre->y - ((gravy < 0) ? 0 : Tmp_win->title_height)-Tmp_win->old_bw;
  if (cre->value_mask & CWWidth) 
    width = cre->width + 2*Tmp_win->boundary_width;
  if (cre->value_mask & CWHeight) 
    height = cre->height + Tmp_win->title_height + 2*Tmp_win->boundary_width;
  
  /*
   * SetupWindow (x,y) are the location of the upper-left outer corner and
   * are passed directly to XMoveResizeWindow (frame).  The (width,height)
   * are the inner size of the frame.  The inner width is the same as the 
   * requested client window width; the inner height is the same as the
   * requested client window height plus any title bar slop.
   */
  SetupFrame (Tmp_win, x, y, width, height,FALSE);
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleVisibilityNotify - record fully visible windows for
 *      use in the RaiseLower function and the OnTop type windows.
 *
 ************************************************************************/
void HandleVisibilityNotify()
{
  XVisibilityEvent *vevent = (XVisibilityEvent *) &Event;

  if(Tmp_win)
    {
      if(vevent->state == VisibilityUnobscured)
	Tmp_win->flags |= VISIBLE;
      else
	Tmp_win->flags &= ~VISIBLE;

      /* For the most part, we'll raised partially obscured ONTOP windows
       * here. The exception is ONTOP windows that are obscured by
       * other ONTOP windows, which are raised in KeepOnTop(). This
       * complicated set-up saves us from continually re-raising
       * every on top window */
      if(((vevent->state == VisibilityPartiallyObscured)||
	  (vevent->state == VisibilityFullyObscured))&&
	 (Tmp_win->flags&ONTOP)&&(Tmp_win->flags & RAISED))
	{
	  if(Tmp_win->icon_w)
	    XRaiseWindow(dpy,Tmp_win->icon_w);
	  XRaiseWindow(dpy,Tmp_win->lead_w);
	  Tmp_win->flags &= ~RAISED;
	}
    }
}
      
/**************************************************************************
 *
 * Removes expose events for a specific window from the queue 
 *
 *************************************************************************/
void flush_expose (Window w)
{
  XEvent dummy;
  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy)) ;
}


/***********************************************************************
 *
 *  Procedure:
 *	InstallWindowColormaps - install the colormaps for one fvwm window
 *
 *  Inputs:
 *	type	- type of event that caused the installation
 *	tmp	- for a subset of event types, the address of the
 *		  window structure, whose colormaps are to be installed.
 *
 ************************************************************************/
void InstallWindowColormaps (int type, FvwmWindow *tmp)
{
  XWindowAttributes attributes;
  static Colormap last_cmap;

  /*
   * Make sure the client window still exists. 
   */
  if (XGetGeometry(dpy, tmp->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    {
      return;
    }
  switch (type) 
    {
    case PropertyNotify:
    default:
      /* Save the colormap to be loaded for when force loading of
       * root colormap(s) ends.
       */
      Scr.pushed_window = tmp;
      /* Don't load any new colormap if root colormap(s) has been
       * force loaded.
       */
      if (Scr.root_pushes)
	{
	  return;
	}
      break;
    }
  if(tmp == &Scr.FvwmRoot)
    {
      XGetWindowAttributes(dpy,Scr.Root,&attributes);
    }
  else
    {
      XGetWindowAttributes(dpy,tmp->w,&attributes);
    }
  
  if(last_cmap != attributes.colormap)
    {
      last_cmap = attributes.colormap;
      XInstallColormap(dpy,attributes.colormap);    
    }
}


/***********************************************************************
 *
 *  Procedures:
 *	<Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 *	   These matching routines provide a mechanism to insure that
 *	   the root colormap(s) is installed during operations like
 *	   rubber banding or menu display that require colors from
 *	   that colormap.  Calls may be nested arbitrarily deeply,
 *	   as long as there is one UninstallRootColormap call per
 *	   InstallRootColormap call.
 *
 *	   The final UninstallRootColormap will cause the colormap list
 *	   which would otherwise have be loaded to be loaded, unless
 *	   Enter or Leave Notify events are queued, indicating some
 *	   other colormap list would potentially be loaded anyway.
 ***********************************************************************/
void InstallRootColormap()
{
    FvwmWindow *tmp;
    if (Scr.root_pushes == 0) 
      {
	/*
	 * The saving and restoring of cmapInfo.pushed_window here
	 * is a slimy way to remember the actual pushed list and
	 * not that of the root window.
	 */
	tmp = Scr.pushed_window;
	InstallWindowColormaps(0, &Scr.FvwmRoot);
	Scr.pushed_window = tmp;
      }
    Scr.root_pushes++;
    return;
}

/***************************************************************************
 * 
 * Unstacks one layer of root colormap pushing 
 * If we peel off the last layer, re-install th e application colormap
 * 
 ***************************************************************************/
void UninstallRootColormap()
{
  if (Scr.root_pushes)
    Scr.root_pushes--;
  
  if (!Scr.root_pushes) 
    {
     XSync (dpy, 0);
      InstallWindowColormaps(0, Scr.pushed_window);
    }
  return;
}

  
/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 * 
 *  Puts windows back where they were before fvwm took over 
 *
 ************************************************************************/
void RestoreWithdrawnLocation (FvwmWindow *tmp)
{
  int gravx, gravy;
  unsigned int bw, mask;
  XWindowChanges xwc;
  int decorated;

  if(tmp->title_w)
    decorated = 1;
  else
    decorated = 0;
  if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y, 
		    &JunkWidth, &JunkHeight, &bw, &JunkDepth)) 
    {
      GetGravityOffsets (tmp, &gravx, &gravy);
      if (gravy < 0) 
	xwc.y -= tmp->title_height+tmp->boundary_width+decorated;
      else if (gravy > 0)
	xwc.y += tmp->boundary_width+decorated;
      
      mask = (CWX | CWY);
      if (bw != tmp->old_bw) 
	{
	  xwc.x -= (gravx+1) * tmp->old_bw;
	  xwc.y -= (gravy+1) * tmp->old_bw;
	  xwc.border_width = tmp->old_bw;
	  mask |= CWBorderWidth;
	}
      xwc.x += gravx * (tmp->boundary_width+decorated);
      XConfigureWindow (dpy, tmp->w, mask, &xwc);
      if (tmp->wmhints && (tmp->wmhints->flags & IconWindowHint)) 
	XUnmapWindow (dpy, tmp->wmhints->icon_window);
    }
}

/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;
  
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
}


