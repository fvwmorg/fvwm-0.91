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
 * window resizing borrowed from the "wm" window manager
 *
 ***********************************************************************/

#include <stdio.h>
#include "fvwm.h"
#include "misc.h"
#include "screen.h"

static int dragx;       /* all these variables are used */
static int dragy;       /* in resize operations */
static int dragWidth;
static int dragHeight;

static int origx;
static int origy;
static int origWidth;
static int origHeight;

static int ymotion, xmotion;
static int last_width,last_height;

void ConstrainSize (FvwmWindow *, int *, int *);

extern Bool KeyboardGrabbed;

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void resize_window(XEvent *eventp,Window w,FvwmWindow *tmp_win)
{
  Bool finished = FALSE, done = FALSE;
  int x,y,orig_Vx,orig_Vy,delta_x,delta_y;

  /* can't resize icons */
  if(w == tmp_win->icon_w)
    return;

  ResizeWindow = tmp_win->lead_w;

  if(!Scr.ClickToFocus)
    Scr.Focus = NULL;
  InstallRootColormap();

  while(XGrabPointer(dpy, Scr.Root, True,
		     ButtonPressMask | ButtonReleaseMask |
		     ButtonMotionMask | PointerMotionMask | 
		     PointerMotionHintMask, GrabModeAsync, GrabModeAsync,
		     Scr.Root, Scr.ResizeCursor,CurrentTime)!=GrabSuccess);
  if(!KeyboardGrabbed)
    while(XGrabKeyboard(dpy, Scr.Root, True,GrabModeAsync, GrabModeAsync, 
			CurrentTime)!=GrabSuccess);
  XGrabServer(dpy);

  XGetGeometry(dpy, (Drawable) ResizeWindow, &JunkRoot,
	       &dragx, &dragy, (unsigned int *)&dragWidth, 
	       (unsigned int *)&dragHeight, &JunkBW,&JunkDepth);

  dragx += tmp_win->frame_bw;
  dragy += tmp_win->frame_bw;
  origx = dragx;
  origy = dragy;
  origWidth = dragWidth;
  origHeight = dragHeight;
  ymotion=xmotion=0;

  /* pop up a resize dimensions window */
  Scr.SizeStringOffset = SIZE_HINDENT;
  XResizeWindow (dpy, Scr.SizeWindow,
		 Scr.SizeStringWidth + SIZE_HINDENT * 2, 
		 Scr.StdFont.height + SIZE_VINDENT * 2);
  XMapRaised(dpy, Scr.SizeWindow);
  last_width = 0;
  last_height = 0;
  DisplaySize(tmp_win, origWidth, origHeight);

  /* draw the rubber-band window */
  MoveOutline (Scr.Root, dragx - tmp_win->frame_bw,
	       dragy - tmp_win->frame_bw, dragWidth + 2 * tmp_win->frame_bw,
	       dragHeight + 2 * tmp_win->frame_bw);
  
  /* loop to resize */
  while(!finished)
    {
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 EnterWindowMask | LeaveWindowMask |
		 ButtonMotionMask | PointerMotionMask,  &Event);
      if (Event.type == MotionNotify) 
	/* discard any extra motion events before a release */
	while(XCheckMaskEvent(dpy, ButtonMotionMask |	ButtonReleaseMask |
			      PointerMotionMask,&Event))
	  if (Event.type == ButtonRelease) break;

      done = FALSE;
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
      switch(Event.type)
	{
	case ButtonPress:
	case KeyPress:
	case EnterNotify:
	case LeaveNotify:
	  done = TRUE;
	  break;

	case ButtonRelease:
	  finished = TRUE;
	  done = TRUE;
	  break;

	case MotionNotify:
	  XQueryPointer( dpy, Event.xany.window,
			&JunkRoot, &JunkChild,
			&x, &y,&JunkX, &JunkY,&JunkMask);
	  /* need to move the viewport */
	  if((x <2)||(x > Scr.MyDisplayWidth-2)||
	     (y <2)||(y > Scr.MyDisplayHeight-2))
	    {
	      /* Turn off the rubberband */
	      MoveOutline(Scr.Root,0,0,0,0);
	      /* Move the viewport */
	      if(x<2)
		delta_x = - Scr.MyDisplayWidth;
	      else if (x > Scr.MyDisplayWidth-2)
		delta_x = Scr.MyDisplayWidth;
	      else
		delta_x = 0;
	      if(y<2)
		delta_y =  - Scr.MyDisplayHeight;
	      else if (y > Scr.MyDisplayHeight-2)
		delta_y = Scr.MyDisplayHeight;
	      else
		delta_y = 0;
	      orig_Vx= Scr.Vx;
	      orig_Vy = Scr.Vy;
	      MoveViewport(Scr.Vx + delta_x,Scr.Vy + delta_y);
	      /* move the cursor back to the approximate correct location */
	      /* that is, the same place on the virtual desktop that it */
	      /* started at */
	      x -= Scr.Vx - orig_Vx;
	      y -= Scr.Vy - orig_Vy;
	      if(x < 2)x = 2;
	      if(y < 2)y = 2;
	      if(x > Scr.MyDisplayWidth-2)x = Scr.MyDisplayWidth-2;
	      if(y > Scr.MyDisplayHeight-2)y = Scr.MyDisplayHeight-2;
	      origx -= Scr.Vx - orig_Vx;
	      origy -= Scr.Vy- orig_Vy;
	      dragx -= Scr.Vx - orig_Vx;
	      dragy -= Scr.Vy - orig_Vy;
	      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,x,y);
	      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			    &x, &y,&JunkX, &JunkY, &JunkMask);
	    }
	  DoResize(x, y, tmp_win);
	  done = TRUE;
	default:
	  break;
	}
      if(!done)DispatchEvent();
    } 

  /* erase the rubber-band */
  MoveOutline(Scr.Root, 0, 0, 0, 0);

  /* pop down the size window */
  XUnmapWindow(dpy, Scr.SizeWindow);

  ConstrainSize (tmp_win, &dragWidth, &dragHeight);
  SetupFrame (tmp_win, dragx - tmp_win->frame_bw, 
	      dragy - tmp_win->frame_bw, dragWidth, dragHeight,FALSE);

  Scr.LastWindowRaised = tmp_win;      
  XRaiseWindow(dpy, ResizeWindow);
  KeepOnTop();

  UninstallRootColormap();
  ResizeWindow = None; 
  XUngrabServer(dpy);
  XUngrabPointer(dpy,0);
  if(!KeyboardGrabbed)
    XUngrabKeyboard(dpy,CurrentTime);
  RedrawPager();
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root  - the X corrdinate in the root window
 *      y_root  - the Y corrdinate in the root window
 *      tmp_win - the current fvwm window
 *
 ************************************************************************/
void DoResize(int x_root, int y_root, FvwmWindow *tmp_win)
{
  int action=0;
  
  if ((y_root <= origy)||((ymotion == 1)&&(y_root < origy+origHeight-1)))
    {
      dragy = y_root;
      dragHeight = origy + origHeight - y_root;
      action = 1;
      ymotion = 1;
    }
  else if ((y_root >= origy + origHeight - 1)||
	   ((ymotion == -1)&&(y_root > origy)))
    {
      dragy = origy;
      dragHeight = 1 + y_root - dragy;
      action = 1;
      ymotion = -1;
    }
  
  if ((x_root <= origx)||
      ((xmotion == 1)&&(x_root < origx + origWidth - 1)))
    {
      dragx = x_root;
      dragWidth = origx + origWidth - x_root;
      action = 1;
      xmotion = 1;
    }
  if ((x_root >= origx + origWidth - 1)||
    ((xmotion == -1)&&(x_root > origx)))
    {
      dragx = origx;
      dragWidth = 1 + x_root - origx;
      action = 1;
      xmotion = -1;
    }
  
  if (action) 
    {
      ConstrainSize (tmp_win, &dragWidth, &dragHeight);
      if (xmotion == 1)
	dragx = origx + origWidth - dragWidth;
      if (ymotion == 1)
	dragy = origy + origHeight - dragHeight;
      
      MoveOutline(Scr.Root,
		  dragx - tmp_win->frame_bw,
		  dragy - tmp_win->frame_bw,
		  dragWidth + 2 * tmp_win->frame_bw,
		  dragHeight + 2 * tmp_win->frame_bw);
    }
  DisplaySize(tmp_win, dragWidth, dragHeight);
}



/***********************************************************************
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 ***********************************************************************/
void DisplaySize(FvwmWindow *tmp_win, int width, int height)
{
  char str[100];
  int dwidth;
  int dheight;
  
  if (last_width == width && last_height == height)
    return;
  
  last_width = width;
  last_height = height;
  
  dheight = height - tmp_win->title_height - 2*tmp_win->boundary_width;
  dwidth = width - 2*tmp_win->boundary_width;
  
  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */
  if (tmp_win->hints.flags&(PMinSize|PBaseSize) 
      && tmp_win->hints.flags & PResizeInc)
    {
      if (tmp_win->hints.flags & PBaseSize) 
	{
	  dwidth -= tmp_win->hints.base_width;
	  dheight -= tmp_win->hints.base_height;
	} 
      else 
	{
	  dwidth -= tmp_win->hints.min_width;
	  dheight -= tmp_win->hints.min_height;
	}
    }
  
  if (tmp_win->hints.flags & PResizeInc)
    {
      dwidth /= tmp_win->hints.width_inc;
      dheight /= tmp_win->hints.height_inc;
    }
  
  (void) sprintf (str, " %4d x %-4d ", dwidth, dheight);
  XRaiseWindow(dpy, Scr.SizeWindow);
  XDrawImageString (dpy, Scr.SizeWindow, Scr.NormalGC,
		    Scr.SizeStringOffset,
		    Scr.StdFont.font->ascent + SIZE_VINDENT,
		    str, 13);
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 * 
 ***********************************************************************/

void ConstrainSize (FvwmWindow *tmp_win, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))
    int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
    int baseWidth, baseHeight;
    int dwidth = *widthp, dheight = *heightp;

    dwidth -= 2 *tmp_win->boundary_width;
    dheight -= (tmp_win->title_height + 2*tmp_win->boundary_width);

    if (tmp_win->hints.flags & PMinSize) 
      {
        minWidth = tmp_win->hints.min_width;
        minHeight = tmp_win->hints.min_height;
      } 
    else if (tmp_win->hints.flags & PBaseSize) 
      {
        minWidth = tmp_win->hints.base_width;
        minHeight = tmp_win->hints.base_height;
      } 
    else
      minWidth = minHeight = 1;
    
    if (tmp_win->hints.flags & PBaseSize) 
      {
	baseWidth = tmp_win->hints.base_width;
	baseHeight = tmp_win->hints.base_height;
      } 
    else if (tmp_win->hints.flags & PMinSize) 
      {
	baseWidth = tmp_win->hints.min_width;
	baseHeight = tmp_win->hints.min_height;
      } 
    else
      baseWidth = baseHeight = 0;

    if (tmp_win->hints.flags & PMaxSize) 
      {
        maxWidth = tmp_win->hints.max_width;
        maxHeight =  tmp_win->hints.max_height;
      } 
    else 
      {
        maxWidth = MAX_WINDOW_WIDTH;
	maxHeight = MAX_WINDOW_HEIGHT;
      }
    
    if (tmp_win->hints.flags & PResizeInc) 
      {
        xinc = tmp_win->hints.width_inc;
        yinc = tmp_win->hints.height_inc;
      } 
    else
      xinc = yinc = 1;
    
    /*
     * First, clamp to min and max values
     */
    if (dwidth < minWidth) dwidth = minWidth;
    if (dheight < minHeight) dheight = minHeight;
    
    if (dwidth > maxWidth) dwidth = maxWidth;
    if (dheight > maxHeight) dheight = maxHeight;
    
    
    /*
     * Second, fit to base + N * inc
     */
    dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;
    
    
    /*
     * Third, adjust for aspect ratio
     */
#define maxAspectX tmp_win->hints.max_aspect.x
#define maxAspectY tmp_win->hints.max_aspect.y
#define minAspectX tmp_win->hints.min_aspect.x
#define minAspectY tmp_win->hints.min_aspect.y
    /*
     * The math looks like this:
     *
     * minAspectX    dwidth     maxAspectX
     * ---------- <= ------- <= ----------
     * minAspectY    dheight    maxAspectY
     *
     * If that is multiplied out, then the width and height are
     * invalid in the following situations:
     *
     * minAspectX * dheight > minAspectY * dwidth
     * maxAspectX * dheight < maxAspectY * dwidth
     * 
     */
    
    if (tmp_win->hints.flags & PAspect)
      {
        if (minAspectX * dheight > minAspectY * dwidth)
	  {
            delta = makemult(minAspectX * dheight / minAspectY - dwidth,
                             xinc);
            if (dwidth + delta <= maxWidth) 
	      dwidth += delta;
            else
	      {
                delta = makemult(dheight - dwidth*minAspectY/minAspectX,
                                 yinc);
                if (dheight - delta >= minHeight) dheight -= delta;
	      }
	  }
	
        if (maxAspectX * dheight < maxAspectY * dwidth)
	  {
            delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
                             yinc);
            if (dheight + delta <= maxHeight)
	      dheight += delta;
            else
	      {
                delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
                                 xinc);
                if (dwidth - delta >= minWidth) dwidth -= delta;
	      }
	  }
      }
    

    /*
     * Fourth, account for border width and title height
     */
    *widthp = dwidth + 2*tmp_win->boundary_width;
    *heightp = dheight + tmp_win->title_height + 2*tmp_win->boundary_width;
    return;
}


/***********************************************************************
 *
 *  Procedure:
 *      Setupframe - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ***********************************************************************
 */

void SetupFrame(FvwmWindow *tmp_win, int x, int y, int w, int h, Bool sendEvent)
{
  XEvent client_event;
  XWindowChanges frame_wc, xwc;
  unsigned long frame_mask, xwcm;
  int bw;

  if(Scr.DontMoveOff)
    {
      if (x + Scr.Vx + w < 16)
	x = 16 - Scr.Vx - w;
      if (y + Scr.Vy + h < 16)
	y = 16 - Scr.Vy - h;
    }
  if (x >= Scr.MyDisplayWidth + Scr.VxMax - Scr.Vx)
    x = Scr.MyDisplayWidth + Scr.VxMax -Scr.Vx - 16;
  if (y >= Scr.MyDisplayHeight+Scr.VyMax - Scr.Vy)
    y = Scr.MyDisplayHeight + Scr.VyMax - Scr.Vy - 16;

  bw = tmp_win->frame_bw;		/* -1 means current frame width */
  
  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if ((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
      (w == tmp_win->frame_width && h == tmp_win->frame_height))
    sendEvent = TRUE;
  
  if (tmp_win->title_w) 
    {
      xwcm = CWWidth | CWX | CWY;

      tmp_win->title_width=w-2*tmp_win->boundary_width-
	2*tmp_win->title_height+1;
      if(tmp_win->title_width < 1) 
	tmp_win->title_width = 1;
      xwc.width = tmp_win->title_width;
      tmp_win->title_x = xwc.x = tmp_win->boundary_width+tmp_win->title_height;
      tmp_win->title_y = xwc.y = tmp_win->boundary_width;
      XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);

      xwcm = CWX | CWY;
      xwc.x = tmp_win->boundary_width;
      xwc.y = tmp_win->boundary_width;
      XConfigureWindow(dpy, tmp_win->sys_w, xwcm, &xwc);
      xwc.x=
	w-tmp_win->boundary_width-tmp_win->title_height+1;
      XConfigureWindow(dpy, tmp_win->iconify_w, xwcm, &xwc);

      xwcm = CWWidth | CWX | CWY;
      xwc.x = tmp_win->corner_width;
      xwc.y = 0;
      xwc.width = tmp_win->title_width;
      XConfigureWindow(dpy, tmp_win->top_w, xwcm, &xwc);

      xwc.y = h - tmp_win->boundary_width+1;
      XConfigureWindow(dpy, tmp_win->bottom_w, xwcm, &xwc);

      xwcm = CWHeight | CWX | CWY;
      xwc.x = w - tmp_win->boundary_width+1;
      xwc.y = tmp_win->corner_width;
      xwc.height = h - 2*tmp_win->corner_width;
      XConfigureWindow(dpy, tmp_win->right_side_w, xwcm, &xwc);

      xwc.x = 0;
      XConfigureWindow(dpy, tmp_win->left_side_w, xwcm, &xwc);
    }
  
  tmp_win->attr.width = w - 2*tmp_win->boundary_width;
  tmp_win->attr.height = h - tmp_win->title_height - 2*tmp_win->boundary_width;
  
  if(tmp_win->title_height)
    {
      XMoveResizeWindow(dpy, tmp_win->w, tmp_win->boundary_width-1, 
			tmp_win->title_height+tmp_win->boundary_width-1,
			tmp_win->attr.width, tmp_win->attr.height);
    }

  /* 
   * fix up frame and assign size/location values in tmp_win
   */
  frame_wc.x = tmp_win->frame_x = x;
  frame_wc.y = tmp_win->frame_y = y;
  frame_wc.width = tmp_win->frame_width = w;
  frame_wc.height = tmp_win->frame_height = h;
  frame_mask = (CWX | CWY | CWWidth | CWHeight);
  XConfigureWindow (dpy, tmp_win->lead_w, frame_mask, &frame_wc);
  
  if (sendEvent)
    {
      client_event.type = ConfigureNotify;
      client_event.xconfigure.display = dpy;
      client_event.xconfigure.event = tmp_win->w;
      client_event.xconfigure.window = tmp_win->w;
      client_event.xconfigure.x = (x + tmp_win->boundary_width);
      client_event.xconfigure.y = (y + tmp_win->title_height);
      client_event.xconfigure.width = w-2*tmp_win->boundary_width;
      client_event.xconfigure.height =h-2*tmp_win->boundary_width - tmp_win->title_height;
      client_event.xconfigure.border_width = tmp_win->old_bw;
      /* Real ConfigureNotify events say we're above title window, so ... */
      /* what if we don't have a title ????? */
      client_event.xconfigure.above = tmp_win->frame;
      client_event.xconfigure.override_redirect = False;
      XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
    }
}


static int	lastx = 0;
static int	lasty = 0;
static int	lastWidth = 0;
static int	lastHeight = 0;
/***********************************************************************
 *
 *  Procedure:
 *	MoveOutline - move a window outline
 *
 *  Inputs:
 *	root	    - the window we are outlining
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *
 ***********************************************************************/
void MoveOutline(Window root, int x, int  y, int  width, int height)
{
  void drawit(Window);
  if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
    return;
    
  /* undraw the old one, if any */
  drawit(root);
  
  lastx = x;
  lasty = y;
  lastWidth = width;
  lastHeight = height;
  
  /* draw the new one, if any */
  drawit(root);

}


void drawit(Window root)
{
  int		xl, xr, yt, yb;
  int		xthird, ythird;
  XSegment	outline[10];
  register XSegment	*r;
  
  r = outline;

  if (lastWidth || lastHeight)	
    {				
      xl = lastx;	       
      xr = lastx + lastWidth - 1;		       
      yt = lasty;					
      yb = lasty + lastHeight - 1;			
      xthird = (xr - xl) / 3;		
      ythird = (yb - yt) / 3;		
      
      r->x1 = xl;					
      r->y1 = yt;					
      r->x2 = xr;					
      r->y2 = yt;					
      r++;						
      
      r->x1 = xl;					
      r->y1 = yb;					
      r->x2 = xr;					
      r->y2 = yb;					
      r++;						
      
      r->x1 = xl;					
      r->y1 = yt;					
      r->x2 = xl;					
      r->y2 = yb;					
      r++;						
      
      r->x1 = xr;					
      r->y1 = yt;					
      r->x2 = xr;					
      r->y2 = yb;					
      r++;						
      
      r->x1 = xl + xthird;			
      r->y1 = yt;				
      r->x2 = r->x1;					
      r->y2 = yb;				
      r++;						
      
      r->x1 = xl + (2 * xthird);			
      r->y1 = yt;				
      r->x2 = r->x1;					
      r->y2 = yb;				
      r++;						
      
      r->x1 = xl;				
      r->y1 = yt + ythird;			
      r->x2 = xr;				
      r->y2 = r->y1;					
      r++;						
      
      r->x1 = xl;				
      r->y1 = yt + (2 * ythird);			
      r->x2 = xr;				
      r->y2 = r->y1;					
      r++;						
      
      XDrawSegments(dpy, root, Scr.DrawGC, outline, r - outline);

    }
}





