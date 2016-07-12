/****************************************************************************
 * This module is all original code 
 * by Rob Nation (nation@rocket.sanders.lockheed.com 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for moving windows
 *
 ***********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

extern XEvent Event;
extern int menuFromFrameOrWindowOrTitlebar;
extern Bool KeyboardGrabbed;
/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,int context)
{
  extern int Stashed_X, Stashed_Y;
  int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset,FinalX,FinalY;

  /* gotta have a window */
  if(w == None)
    return;

  if(!Scr.ClickToFocus)
    Scr.Focus = NULL;
  InstallRootColormap();
  if (menuFromFrameOrWindowOrTitlebar) 
    {
      /* warp the pointer to the cursor position from before menu appeared*/
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 
		   Stashed_X,Stashed_Y);
      XFlush(dpy);
    }
  
  while(XGrabPointer(dpy, Scr.Root, True,
	       ButtonPressMask | ButtonReleaseMask | 
	       ButtonMotionMask | PointerMotionMask,
	       GrabModeAsync, GrabModeAsync,
	       Scr.Root, Scr.MoveCursor, CurrentTime)!=GrabSuccess);
  if(!KeyboardGrabbed)
    {
      while(XGrabKeyboard(dpy, Scr.Root, True,GrabModeAsync, GrabModeAsync, 
			  CurrentTime)!=GrabSuccess);
    }
  XGrabServer(dpy);

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		&DragX, &DragY,	&JunkX, &JunkY, &JunkMask);

  if ((context & C_ICON) && tmp_win->icon_w)
    w = tmp_win->icon_w;
  else if (w != tmp_win->icon_w)
    w = tmp_win->lead_w;
  
  XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	       (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, 
	       &JunkBW,  &JunkDepth);

  DragWidth += JunkBW;
  DragHeight+= JunkBW;
  
  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  moveLoop(tmp_win, XOffset,YOffset,DragWidth,DragHeight, &FinalX,&FinalY);
  if (w == tmp_win->lead_w)
    SetupFrame (tmp_win, FinalX, FinalY,
		tmp_win->frame_width, tmp_win->frame_height,FALSE);
  else /* icon window */
    {
      XMoveWindow (dpy, w, FinalX, FinalY);
      if(w == tmp_win->icon_w)
	{
	  tmp_win->icon_x_loc = FinalX;
	  tmp_win->icon_y_loc = FinalY;
	}
    }

  XRaiseWindow(dpy,w);
  Scr.LastWindowRaised = tmp_win;
  KeepOnTop();
  UninstallRootColormap();
  XUngrabPointer(dpy,CurrentTime);  
  if(!KeyboardGrabbed)
    {
      XUngrabKeyboard(dpy,CurrentTime);
    }
  XUngrabServer(dpy);
  RedrawPager();
  return;
}



/****************************************************************************
 *
 * Move the rubberband around, return with the new window location
 *
 ****************************************************************************/
void moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY)
{
  Bool finished = False;
  Bool done;
  int xl,yt,delta_x,delta_y,orig_Vx,orig_Vy;

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
		&JunkX, &JunkY, &JunkMask);
  xl += XOffset;
  yt += YOffset;
  MoveOutline(Scr.Root, xl, yt, Width,Height);
  while (!finished)
    {
      /* block until there is an interesting event */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 ExposureMask | PointerMotionMask | ButtonMotionMask |
		 VisibilityChangeMask, &Event);
      if (Event.type == MotionNotify) 
	/* discard any extra motion events before a logical release */
	while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask |
			      ButtonRelease, &Event))
	  if(Event.type == ButtonRelease) break;

      done = FALSE;
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
      switch(Event.type)
	{
	case EnterNotify:
	case KeyPress:
	case LeaveNotify:
	  /* throw away enter and leave events until release */
	  done = TRUE;
	  break;
	case ButtonRelease:
	  MoveOutline(Scr.Root, 0, 0, 0, 0);
	  *FinalX = Event.xmotion.x_root + XOffset;
	  *FinalY = Event.xmotion.y_root + YOffset;
	  done = TRUE;
	  finished = TRUE;
	  break;

	case MotionNotify:
	  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
			&JunkX, &JunkY, &JunkMask);
	  /* need to move the viewport */
	  if((xl <2)||(xl > Scr.MyDisplayWidth-2)||
	     (yt <2)||(yt > Scr.MyDisplayHeight-2))
	    {
	      /* Turn off the rubberband */
	      MoveOutline(Scr.Root,0,0,0,0);
	      /* Move the viewport */
	      if(xl<2)
		delta_x = - Scr.MyDisplayWidth;
	      else if (xl > Scr.MyDisplayWidth-2)
		delta_x = Scr.MyDisplayWidth;
	      else
		delta_x = 0;
	      if(yt<2)
		delta_y =  - Scr.MyDisplayHeight;
	      else if (yt > Scr.MyDisplayHeight-2)
		delta_y = Scr.MyDisplayHeight;
	      else
		delta_y = 0;
	      orig_Vx = Scr.Vx;
	      orig_Vy = Scr.Vy;
	      MoveViewport(Scr.Vx + delta_x,Scr.Vy + delta_y);
	      /* move the cursor back to the approximate correct location */
	      /* that is, the same place on the virtual desktop that it */
	      /* started at */
	      xl -= Scr.Vx - orig_Vx;
	      yt -= Scr.Vy - orig_Vy;
	      if(xl < 2)xl = 2;
	      if(yt < 2)yt = 2;
	      if(xl > Scr.MyDisplayWidth-2)xl = Scr.MyDisplayWidth-2;
	      if(yt > Scr.MyDisplayHeight-2)yt = Scr.MyDisplayHeight-2;

	      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,xl,yt);
	      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			    &xl, &yt,
			    &JunkX, &JunkY, &JunkMask);
	    }

	  /* redraw the rubberband */
	  xl += XOffset;
	  yt += YOffset;
	  MoveOutline(Scr.Root, xl, yt, Width,Height);

	  done = TRUE;
	  break;

	case ButtonPress:
	  done = TRUE;
	  break;

	default:
	  break;
	}
      if(!done)
	{
	  MoveOutline(Scr.Root,0,0,0,0);
	  DispatchEvent();
	  MoveOutline(Scr.Root, xl, yt, Width, Height);
	}
    }
}

/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard 
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void Keyboard_shortcuts(XEvent *Event, int ReturnEvent)  
{
  int x,y,x_root,y_root;
  int move_size,x_move,y_move;
  KeySym keysym;

  /* Pick the size of the cursor movement */
  move_size = Scr.EntryHeight;
  if(Event->xkey.state & ControlMask)
    move_size = 1;
  if(Event->xkey.state & ShiftMask)
    move_size = 100;

  keysym = XLookupKeysym(&Event->xkey,0);

  x_move = 0;
  y_move = 0;
  if(keysym==XK_Up)
    y_move = -move_size;
  else if(keysym == XK_Down)
    y_move = move_size;
  else if(keysym == XK_Left)
    x_move = -move_size;
  else if(keysym == XK_Right)
    x_move = move_size;
  else if(keysym == XK_Return)
    {
      /* beat up the event */
      Event->type = ReturnEvent;
    }


  if((x_move != 0)||(y_move != 0))
    {
      /* beat up the event */
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &x_root, &y_root, &x, &y, &JunkMask);
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x_root+x_move,
		   y_root+y_move);

      /* beat up the event */
      Event->type = MotionNotify;
      Event->xkey.x += x_move;
      Event->xkey.y += y_move;
    }
}

