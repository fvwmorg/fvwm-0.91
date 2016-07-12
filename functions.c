/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation (nation@rocket.sanders.lockheed.com 
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
 * fvwm menu code
 *
 ***********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <X11/Xos.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

extern XEvent Event;
extern int menuFromFrameOrWindowOrTitlebar;

extern FvwmWindow *Tmp_win;

/***********************************************************************
 *
 *  Procedure:
 *	DeIconify a window
 *
 ***********************************************************************/
void DeIconify(FvwmWindow *tmp_win)
{
  FvwmWindow *t;
  
  /* now de-iconify transients */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if ((t == tmp_win)|| 
	  ((t->flags & TRANSIENT) &&(t->transientfor == tmp_win->w)))
	{
	  t->flags |= MAPPED;
	  if(t->title_height)
	    XMapWindow(dpy, t->w);
	  XMapRaised(dpy, t->lead_w);
	  SetMapStateProp(t, NormalState);
	  
	  if (t->icon_w) 
	    XUnmapWindow(dpy, t->icon_w);
	  t->flags &= ~ICON;
	}
    }
  Scr.LastWindowRaised = t;

  KeepOnTop();
  RedrawPager();
  XSync (dpy, 0);
  return;
}


/****************************************************************************
 *
 * Iconifies the selected window
 *
 ****************************************************************************/
void Iconify(FvwmWindow *tmp_win, int def_x, int def_y)
{
  FvwmWindow *t;
  XWindowAttributes winattrs;
  unsigned long eventMask;
  
  XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  
  /* iconify transients first */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if ((t==tmp_win)||
	  ((t->flags & TRANSIENT) && (t->transientfor == tmp_win->w)))
	{
	  /*
	   * Prevent the receipt of an UnmapNotify, since that would
	   * cause a transition to the Withdrawn state.
	   */
	  t->flags &= ~MAPPED;
	  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
	  XUnmapWindow(dpy, t->w);
	  XSelectInput(dpy, t->w, eventMask);
	  if(t->title_height)
	    XUnmapWindow(dpy, t->frame);
	  if (t->icon_w)
	    XUnmapWindow(dpy, t->icon_w);
	  SetMapStateProp(t, IconicState);
	  SetBorder (t, False,False,False);
	  t->flags |= ICON;
	}
    } 
  if (tmp_win->icon_w == (int)NULL)
    CreateIconWindow(tmp_win, def_x, def_y);
  XMapRaised(dpy, tmp_win->icon_w);
  Scr.LastWindowRaised = tmp_win;      
  RedrawPager();
  KeepOnTop();
  XSync (dpy, 0);
  return;
}


/****************************************************************************
 *
 *  ??????????????????????Got this from twm?????????????????????????
 *
 ****************************************************************************/
void SetMapStateProp(FvwmWindow *tmp_win, int state)
{
  unsigned long data[2];		/* "suggested" by ICCCM version 1 */
  
  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_w;
  
  XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32, 
		   PropModeReplace, (unsigned char *) data, 2);
  return;
}


/****************************************************************************
 *
 * Keeps the "StaysOnTop" windows on the top of the pile.
 * This is achieved by clearing a flag for OnTop windows here, and waiting
 * for a visibility notify on the windows. Eception: OnTop windows which are
 * obscured by other OnTop windows, which need to be raised here.
 *
 ****************************************************************************/
void KeepOnTop()
{
  FvwmWindow *t;

  /* flag that on-top windows should be re-raised */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if((t->flags & ONTOP)&& !(t->flags & VISIBLE))
	{
	  if(t->icon_w)
	    XRaiseWindow(dpy,t->icon_w);
	  XRaiseWindow(dpy,t->lead_w);
	  t->flags &= ~RAISED;
	}
      else
	t->flags |= RAISED;
    }
}


/***************************************************************************
 *
 *  Moves the viewport within thwe virtual desktop
 *
 ***************************************************************************/
void MoveViewport(int newx, int newy)
{
  FvwmWindow *t;
  int deltax,deltay;

  if(newx > Scr.VxMax)
    newx = Scr.VxMax;
  if(newy > Scr.VyMax)
    newy = Scr.VyMax;
  if(newx <0)
    newx = 0;
  if(newy <0)
    newy = 0;

  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;
  ShowCurrentPort();
  if((deltax!=0)||(deltay!=0))
    {
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  if(!(t->flags & STICKY))
	    {
	      if(t->icon_w)
		{
		  t->icon_x_loc += deltax;
		  t->icon_y_loc += deltay;
		  XMoveWindow(dpy,t->icon_w,t->icon_x_loc,t->icon_y_loc);
		}
	      t->frame_x += deltax;
	      t->frame_y += deltay;
	      XMoveWindow(dpy,t->lead_w,t->frame_x,t->frame_y);
	    }
	}
    }
  Scr.Vx = newx;
  Scr.Vy = newy;
  ShowCurrentPort();
}



/**************************************************************************
 *
 * Moves focus to specified window 
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t)
{
  int dx,dy;
  int cx,cy,x,y;
  
  if(t == (FvwmWindow *)0)
    return;

  if(t->flags & ICON)
    {
      cx = t->icon_x_loc + t->icon_w_width/2;
      cy = t->icon_y_loc + ICON_HEIGHT/2;
    }
  else
    {
      cx = t->frame_x + t->frame_width/2;
      cy = t->frame_y + t->frame_height/2;
    }
  /* Put center of window on the visible screen */
  if(Scr.CenterOnCirculate)
    {
      dx = cx - Scr.MyDisplayWidth/2 + Scr.Vx;
      dy = cy - Scr.MyDisplayHeight/2 + Scr.Vy;
    }
  else
    {
      dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
      dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;
    }
  MoveViewport(dx,dy);
  if(t->flags & ICON)
    {
      x =  t->icon_x_loc;
      y = t->icon_y_loc;
    }
  else
    {
      x = t->frame_x;
      y = t->frame_y;
    }
  XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x+2,y+2);
  XRaiseWindow(dpy,t->lead_w);
  SetBorder(t,True,False,True);
}


   
