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

void flush_expose();
void RelieveWindow(Window, int, int);
void DrawPattern(Window, GC, GC, int, int, int, int);
GC MyStippleGC,ReliefGC,ShadowGC;
extern Window keyboardRevert;
extern Bool KeyboardGrabbed;

/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void SetBorder (FvwmWindow *t, Bool onoroff,Bool force,Bool Mapped)
{
  int y;
  static FvwmWindow *last_window;
  XRectangle rect[4];
  XSegment seg[14];
  int u, b, l, r;

  /* these are used for filling the corner panels */
  rect[0].x = 0; rect[0].y = 0; 
  rect[0].width = t->frame_width; rect[0].height = t->boundary_width;

  rect[1].x = 0; rect[1].y = 0;
  rect[1].width = t->boundary_width; rect[1].height = t->frame_height;
  
  rect[2].x = t->frame_width - t->boundary_width+1; rect[2].y = 0;
  rect[2].width = t->boundary_width; rect[2].height = t->frame_height;
  
  rect[3].x = 0; rect[3].y = t->frame_height - t->boundary_width+1;
  rect[3].width = t->frame_width; rect[3].height = t->boundary_width;

  if (onoroff) 
    {
      /* don't re-draw just for kicks */
      if((!force)&&(Scr.Focus == t))
	return;
      /* make sure that the previously highlighted window got unhighlighted */
      if((Scr.Focus != t)&&(Scr.Focus != NULL))
	SetBorder(Scr.Focus,False,False,True);
      /* make sure that the window is registered */
      if(!t)
	return;
      /* if we do click to focus, remove the grab on mouse events that
       * was made to detect the focus change */
      if(Scr.ClickToFocus)
	{
	  if(Scr.buttons2grab & 1) XUngrabButton(dpy,Button1,0,t->w);
	  if(Scr.buttons2grab & 2) XUngrabButton(dpy,Button2,0,t->w);
	  if(Scr.buttons2grab & 4) XUngrabButton(dpy,Button3,0,t->w);
	}
      /* set the keyboard focus */
      if((Mapped)&&(t->flags&MAPPED)&&(t->wmhints && t->wmhints->input)&&
	 (Scr.Focus != t))
	{
	  XSetInputFocus (dpy, t->w, RevertToPointerRoot, CurrentTime);
	  XUngrabKeyboard(dpy, CurrentTime);
	  KeyboardGrabbed = False;
	}
      Scr.Focus = t;

      if (t->protocols & DoesWmTakeFocus)    
	send_clientmessage (t->w,_XA_WM_TAKE_FOCUS, CurrentTime);
      
      last_window = t;
      MyStippleGC = Scr.HiStippleGC;
      ReliefGC = Scr.HiReliefGC;
      ShadowGC = Scr.HiShadowGC;
    }
  else
    {
      /* don't re-draw just for kicks */
      if((!force)&&(Scr.Focus != t))
	return;
      /* remove keyboard focus */
      if((Scr.Focus == t)&&(t!= NULL))
	{
	  XSetInputFocus (dpy, Scr.Root, RevertToPointerRoot, CurrentTime);
	  XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
			CurrentTime);
	  KeyboardGrabbed = True;
	  if(Scr.ClickToFocus)
	    {
	      /* need to grab all buttons for window that we are about to
	       * unhighlight */
	      if(Scr.buttons2grab & 1)
		XGrabButton(dpy,Button1,0,Scr.Focus->w,True,ButtonPressMask,
			    GrabModeAsync,GrabModeAsync,None,Scr.FrameCursor);
	      if(Scr.buttons2grab & 2)
		XGrabButton(dpy, Button2,0,Scr.Focus->w,True, ButtonPressMask,
			    GrabModeAsync,GrabModeAsync,None,Scr.FrameCursor);
	      if(Scr.buttons2grab & 4)
		XGrabButton(dpy, Button3, 0,Scr.Focus->w,True,ButtonPressMask,
			    GrabModeAsync,GrabModeAsync,None,Scr.FrameCursor);
	    }
	  Scr.Focus = NULL;
	}
      MyStippleGC = Scr.NormalStippleGC;
      ReliefGC = Scr.StdReliefGC;
      ShadowGC = Scr.StdShadowGC;
    }
  
  if(t->title_w)
    {
      flush_expose (t->frame);
      flush_expose (t->left_side_w);
      flush_expose (t->right_side_w);
      flush_expose (t->top_w);
      flush_expose (t->bottom_w);
      flush_expose (t->title_w);
      flush_expose (t->sys_w);
      flush_expose (t->iconify_w);
      flush_expose (t->w);  
      if(onoroff)
	{
	  XSetWindowBorder(dpy,t->lead_w,Scr.HiRelief.back);
	  XSetWindowBorder(dpy,t->w,Scr.HiRelief.back);
	}
      else
	{
	  XSetWindowBorder(dpy,t->lead_w,Scr.StdRelief.back);
	  XSetWindowBorder(dpy,t->w,Scr.StdRelief.back);
	}
      
      XFillRectangles(dpy, t->frame, MyStippleGC, rect, 4);

      /* draw relief lines */
      RelieveWindow(t->bottom_w,t->title_width,t->boundary_width);
      RelieveWindow(t->top_w,t->title_width,t->boundary_width);
      
      y= t->frame_height - 2*t->corner_width - 2*t->frame_bw+2;
      RelieveWindow(t->left_side_w,t->boundary_width,y);
      RelieveWindow(t->right_side_w,t->boundary_width,y);

      RelieveWindow(t->sys_w,t->title_height,t->title_height);
      RelieveWindow(t->iconify_w,t->title_height,t->title_height);

      y= t->corner_width-1;

      /* draw white hilites */
      seg[0].x1 = t->boundary_width - 2;
      seg[0].y1 = t->frame_height - t->boundary_width+2;
      seg[0].x2 = t->frame_width - t->boundary_width+2; 
      seg[0].y2 = t->frame_height - t->boundary_width+2;

      seg[1].x1 = 1; 
      seg[1].y1 = t->frame_height - t->corner_width+1;
      seg[1].x2 = t->boundary_width - 2;
      seg[1].y2 = t->frame_height - t->corner_width+1;

      seg[2].x1 = t->frame_width - t->boundary_width-1;
      seg[2].y1 = t->frame_height - t->corner_width+1;
      seg[2].x2 = t->frame_width-1; 
      seg[2].y2 = t->frame_height - t->corner_width+1;

      seg[3].x1 = t->frame_width - t->corner_width+2;
      seg[3].y1 = 1;
      seg[3].x2 = t->frame_width - t->corner_width+2; 
      seg[3].y2 = t->boundary_width - 2;
      
      seg[4].x1 = t->frame_width - t->corner_width+2; 
      seg[4].y1 = t->frame_height - t->boundary_width-1;
      seg[4].x2 = t->frame_width - t->corner_width+2; 
      seg[4].y2 = t->frame_height-1;

      seg[5].x1 = t->frame_width - t->boundary_width+2; 
      seg[5].y1 = t->boundary_width-2;
      seg[5].x2 = t->frame_width - t->boundary_width+2;
      seg[5].y2 = t->frame_height - t->boundary_width+1;

      seg[6].x1 = 1; seg[6].y1 = 1;
      seg[6].x2 = 1; seg[6].y2 = t->frame_height-1;
	
      seg[7].x1 = 1; seg[7].y1 = 1;
      seg[7].x2 = t->frame_width-1; seg[7].y2 = 1;

      XDrawSegments(dpy,t->frame,ReliefGC,seg,8);

      seg[0].x1 = t->boundary_width-2;
      seg[0].y1 = t->boundary_width-2;
      seg[0].x2 = t->frame_width-t->boundary_width+1;
      seg[0].y2 = t->boundary_width-2;

      seg[1].x1 = t->boundary_width - 2; 
      seg[1].y1 = t->boundary_width - 2;
      seg[1].x2 = t->boundary_width - 2;
      seg[1].y2 = t->frame_height - t->boundary_width+1;

      seg[2].x1 = t->corner_width - 2; 
      seg[2].y1 = t->frame_height - t->boundary_width+2;
      seg[2].x2 = t->corner_width - 2; 
      seg[2].y2 = t->frame_height - 1;

      seg[3].x1 = t->corner_width - 2; seg[3].y1 = 1;
      seg[3].x2 = t->corner_width - 2; seg[3].y2 = t->boundary_width - 1;

      seg[4].x1 = t->frame_width - t->boundary_width; seg[4].y1 = y-1;
      seg[4].x2 = t->frame_width - 1; seg[4].y2 = y-1;

      seg[5].x1 = 1; seg[5].y1 = t->frame_height - 1;
      seg[5].x2 = t->frame_width - 1; seg[5].y2 = t->frame_height - 1;

      seg[6].x1 = t->frame_width - 1; seg[6].y1 = 1;
      seg[6].x2 = t->frame_width - 1; seg[6].y2 = t->frame_height - 1;

      XDrawSegments(dpy, t->frame, ShadowGC, seg, 7);

      /* draw white hilites */
      seg[0].x1 = t->boundary_width - 1;
      seg[0].y1 = t->frame_height - t->boundary_width+1;
      seg[0].x2 = t->frame_width - t->boundary_width+1; 
      seg[0].y2 = t->frame_height - t->boundary_width+1;

      seg[1].x1 = 0; seg[1].y1 = t->frame_height - t->corner_width;
      seg[1].x2 = t->boundary_width - 1;
      seg[1].y2 = t->frame_height - t->corner_width;

      seg[2].x1 = t->frame_width - t->boundary_width;
      seg[2].y1 = t->frame_height - t->corner_width;
      seg[2].x2 = t->frame_width; 
      seg[2].y2 = t->frame_height - t->corner_width;

      seg[3].x1 = t->frame_width - t->corner_width+1; seg[3].y1 = 0;
      seg[3].x2 = t->frame_width - t->corner_width+1; 
      seg[3].y2 = t->boundary_width - 1;
      
      seg[4].x1 = t->frame_width - t->corner_width+1; 
      seg[4].y1 = t->frame_height - t->boundary_width;
      seg[4].x2 = t->frame_width - t->corner_width+1; 
      seg[4].y2 = t->frame_height;

      seg[5].x1 = t->frame_width - t->boundary_width+1; 
      seg[5].y1 = t->boundary_width-1;
      seg[5].x2 = t->frame_width - t->boundary_width+1;
      seg[5].y2 = t->frame_height - t->boundary_width;

      seg[6].x1 = 0; seg[6].y1 = 0;
      seg[6].x2 = 0; seg[6].y2 = t->frame_height;
	
      seg[7].x1 = 0; seg[7].y1 = 0;
      seg[7].x2 = t->frame_width; seg[7].y2 = 0;

      XDrawSegments(dpy,t->frame,ReliefGC,seg,8);

      seg[0].x1 = 0; seg[0].y1 = y;
      seg[0].x2 = t->boundary_width - 1; seg[0].y2 = y;
      
      seg[1].x1 = 1; seg[1].y1 = y-1;
      seg[1].x2 = t->boundary_width - 1; seg[1].y2 = y-1;
      
      seg[2].x1 = t->frame_width - t->boundary_width; seg[2].y1 = y;
      seg[2].x2 = t->frame_width - 1; seg[2].y2 = y;

      seg[3].x1 = t->boundary_width-1;
      seg[3].y1 = t->boundary_width-1;
      seg[3].x2 = t->frame_width-t->boundary_width;
      seg[3].y2 = t->boundary_width-1;
      
      seg[4].x1 = t->boundary_width - 1; 
      seg[4].y1 = t->boundary_width - 1;
      seg[4].x2 = t->boundary_width - 1;
      seg[4].y2 = t->frame_height - t->boundary_width;

      seg[5].x1 = t->corner_width - 1; seg[5].y1 = 0;
      seg[5].x2 = t->corner_width - 1; seg[5].y2 = t->boundary_width - 1;
      
      seg[6].x1 = t->corner_width - 1; 
      seg[6].y1 = t->frame_height - t->boundary_width+1;
      seg[6].x2 = t->corner_width - 1; 
      seg[6].y2 = t->frame_height - 1;

      XDrawSegments(dpy, t->frame, ShadowGC, seg, 7);

      /* draw patterns for title-bar buttons */
      u = 4;
      b = t->title_height-5;
      l = 4;
      r = t->title_height-5;
      DrawPattern(t->sys_w,ReliefGC,ShadowGC,l,u,r,b);
      DrawPattern(t->iconify_w,ReliefGC,ShadowGC,l,u,r,b);
      SetTitleBar(t,onoroff);
      XSync(dpy,0);
    }
  else      /* no decorative border */
    {
      flush_expose (t->lead_w);
      if(onoroff)
	{
	  if(Scr.d_depth <2)
	    XSetWindowBorder(dpy,t->lead_w,Scr.HiColors.fore);
	  else
	    XSetWindowBorder(dpy,t->lead_w,Scr.HiColors.back);
	}
      else
	XSetWindowBorder(dpy,t->lead_w,Scr.StdColors.back);
      XSync(dpy,0);
    }
}


/****************************************************************************
 *
 *  Redraws just the title bar
 *
 ****************************************************************************/
void SetTitleBar (FvwmWindow *t,Bool onoroff)
{
  int hor_off;
  int w;
  GC MyGC;

  if(t->title_height == 0)
    return;

  if (onoroff) 
    {
      MyGC = Scr.HiliteGC;
      MyStippleGC = Scr.HiStippleGC;
      ReliefGC = Scr.HiReliefGC;
      ShadowGC = Scr.HiShadowGC;
    }
  else
    {
      MyGC = Scr.NormalGC;
      MyStippleGC = Scr.NormalStippleGC;
      ReliefGC = Scr.StdReliefGC;
      ShadowGC = Scr.StdShadowGC;
    }
  flush_expose(t->title_w);
  RelieveWindow(t->title_w,t->title_width,t->title_height);
  
  w=XTextWidth(Scr.StdFont.font,t->name,strlen(t->name));
  hor_off = (t->title_width - w)/2;
  
  /* for mono, we clear an area in the title bar where the window
   * title goes, so that its more legible. For color, no need */
  if(Scr.d_depth<2)
    {
      XClearArea(dpy,t->title_w,hor_off - 4, 0, w+8,t->title_height,False);
      
      XDrawLine(dpy,t->title_w,ShadowGC,hor_off-4,0,hor_off-4,
		t->title_height);
      XDrawLine(dpy,t->title_w,ShadowGC,hor_off-5,1,hor_off-5,
		t->title_height);
      XDrawLine(dpy,t->title_w,ShadowGC,hor_off+w+2,0,hor_off+w+2,
		t->title_height);
    }
  XDrawString (dpy, t->title_w,MyGC,hor_off, Scr.StdFont.y, 
	       t->name, strlen(t->name));
}


/****************************************************************************
 *
 *  Draws the relief pattern around a window
 *
 ****************************************************************************/
void RelieveWindow(Window win, int w, int h)
{
  XFillRectangle(dpy,win,MyStippleGC,0,0,w,h);

  XDrawLine(dpy,win,ReliefGC,0,0,w,0);
  XDrawLine(dpy,win,ReliefGC,0,0,0,h);
  XDrawLine(dpy,win,ReliefGC,1,1,w-1,1);
  XDrawLine(dpy,win,ReliefGC,1,1,1,h-1);

  XDrawLine(dpy,win,ShadowGC,0,h-1,w,h-1);
  XDrawLine(dpy,win,ShadowGC,1,h-2,w,h-2);
  
  XDrawLine(dpy,win,ShadowGC,w-1,0, w-1,h);
  XDrawLine(dpy,win,ShadowGC,w-2,1, w-2,h);
  flush_expose (win);
}

/****************************************************************************
 *
 *  Draws a little pattern within a window
 *
 ****************************************************************************/
void DrawPattern(Window w, GC ShadowGC, GC ReliefGC, int l,int u,int r,int b)
{
  XDrawLine(dpy,w,ShadowGC,l,u,r,u);
  XDrawLine(dpy,w,ShadowGC,l,u,l,b);

  XDrawLine(dpy,w,ReliefGC,l,b,r,b);
  XDrawLine(dpy,w,ReliefGC,r,u,r,b);
}
