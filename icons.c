/****************************************************************************
 * This module is all new
 * by Rob Nation (nation@rocket.sanders.lockheed.com 
 ****************************************************************************/
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

void AutoPlace(int *, int *,int,int);
/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
void CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y)
{
  int final_x, final_y;
  int HotX,HotY;
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  /* first, try to get icon bitmap from the application */
  if((tmp_win->wmhints)&&(tmp_win->wmhints->flags & IconPixmapHint))
    {
      XGetGeometry(dpy,   tmp_win->wmhints->icon_pixmap,
		   &JunkRoot, &JunkX, &JunkY,
		   (unsigned int *)&tmp_win->icon_p_width, 
		   (unsigned int *)&tmp_win->icon_p_height, 
		   &JunkBW, &JunkDepth);
      
      tmp_win->iconPixmap = tmp_win->wmhints->icon_pixmap;
    }
  /* Next, see if it was specified in the .fvwmrc */
  else if(tmp_win->icon_bitmap_file != (char *)0)
    {
      if(XReadBitmapFile (dpy, Scr.Root,tmp_win->icon_bitmap_file,
			  &tmp_win->icon_p_width, &tmp_win->icon_p_height,
			  &tmp_win->iconPixmap,
			  &HotX, &HotY) != BitmapSuccess)
	{
	  tmp_win->iconPixmap=None;
	  tmp_win->icon_p_height = 0;
	  tmp_win->icon_p_width = 0;
	}
    }
  /* no bitmap */
  else
    {
      tmp_win->icon_p_height = 0;
      tmp_win->icon_p_width = 0;
      tmp_win->iconPixmap = None;
    }

  /* figure out the icon window size */
  tmp_win->icon_t_width = XTextWidth(Scr.StdFont.font,tmp_win->icon_name, 
				     strlen(tmp_win->icon_name));
  tmp_win->icon_w_width =   tmp_win->icon_t_width +6;
  if(tmp_win->icon_p_width > tmp_win->icon_w_width)
    {
      tmp_win->icon_w_width = tmp_win->icon_p_width;
    }
  tmp_win->icon_w_height = tmp_win->icon_p_height + ICON_HEIGHT;

  /* need to figure out where to put the icon window now */
  if (tmp_win->wmhints &&
      tmp_win->wmhints->flags & IconPositionHint)
    {
      final_x = tmp_win->wmhints->icon_x;
      final_y = tmp_win->wmhints->icon_y;
    }
  else
    {
      final_x = def_x;
      final_y = def_y;
      if(Scr.AutoPlaceIcons)
	AutoPlace(&final_x,&final_y,tmp_win->icon_w_width,
		  tmp_win->icon_w_height);
    }
  
  /* make sure it winds up on the screen */
  if (final_x > Scr.MyDisplayWidth)
    final_x = Scr.MyDisplayWidth - tmp_win->icon_w_width-2;
  
  if (final_y > Scr.MyDisplayHeight)
    final_y = Scr.MyDisplayHeight - 
      Scr.StdFont.height - 4 - 2;

  tmp_win->icon_x_loc = final_x;
  tmp_win->icon_y_loc = final_y;

  attributes.background_pixel = Scr.StdColors.back;
  valuemask =  CWBorderPixel | CWCursor | CWEventMask | CWBackPixel;
  attributes.border_pixel = Scr.StdColors.fore;
  attributes.cursor = Scr.FrameCursor;
  attributes.event_mask = (ButtonPressMask | ButtonReleaseMask |
			   ExposureMask | KeyPressMask);
  tmp_win->icon_w = 
    XCreateWindow(dpy, Scr.Root, final_x, final_y, tmp_win->icon_w_width,
		  tmp_win->icon_w_height, Scr.BorderWidth, Scr.d_depth, 
		  CopyFromParent,Scr.d_visual,valuemask,&attributes);

  XSaveContext(dpy, tmp_win->icon_w, FvwmContext, (caddr_t)tmp_win);
  XDefineCursor(dpy, tmp_win->icon_w, Scr.IconCursor);
  return;
}

/****************************************************************************
 *
 * Draws the icon window
 *
 ****************************************************************************/
void DrawIconWindow(FvwmWindow *Tmp_win)
{
  if(Tmp_win->iconPixmap != None)
    XCopyPlane(dpy, Tmp_win->iconPixmap, Tmp_win->icon_w, 
	       Scr.NormalGC,0,0,
	       Tmp_win->icon_p_width, 
	       Tmp_win->icon_p_height,
	       (Tmp_win->icon_w_width-Tmp_win->icon_p_width)/2,0, 1 );
  
  XDrawString (dpy, Tmp_win->icon_w, Scr.NormalGC,
	       (Tmp_win->icon_w_width - Tmp_win->icon_t_width)/2,
	       Tmp_win->icon_w_height-Scr.StdFont.height+Scr.StdFont.y-2,
	       Tmp_win->icon_name, strlen(Tmp_win->icon_name));
  
}

/***********************************************************************
 *
 *  Procedure:
 *	RedoIconName - procedure to re-position the icon window and name
 *
 ************************************************************************/
void RedoIconName(FvwmWindow *Tmp_win)
{
  if (Tmp_win->icon_w == (int)NULL)
    return;
  
  Tmp_win->icon_t_width =  XTextWidth(Scr.StdFont.font, Tmp_win->icon_name, 
	       strlen(Tmp_win->icon_name));
  Tmp_win->icon_w_width = Tmp_win->icon_t_width + 6;
  if(Tmp_win->icon_p_width > Tmp_win->icon_w_width)
    {
      Tmp_win->icon_w_width = Tmp_win->icon_p_width;
      Tmp_win->icon_w_height = Tmp_win->icon_p_height + ICON_HEIGHT;
    }
  
  XResizeWindow(dpy, Tmp_win->icon_w, Tmp_win->icon_w_width,
		Tmp_win->icon_w_height);
  /* clear the icon window, and trigger a re-draw via an expose event */
  if (Tmp_win->flags & ICON)
    XClearArea(dpy, Tmp_win->icon_w, 0, 0, 0, 0, True);
  return;
}



  
/***********************************************************************
 *
 *  Procedure:
 *	AutoPlace - Find a home for an icon
 *
 ************************************************************************/
void AutoPlace(int *final_x, int *final_y,int width,int height)
{
  /* Search down the right side of the display, avoiding places where 
   * icons live, or active windows are open, then go accross the bottom
   * If no home is found, put it at the default location */
  int test_x, test_y;
  FvwmWindow *test_window;
  Bool loc_ok;

  /* first check the right side of the screen */
  test_x = Scr.MyDisplayWidth - width;
  test_y = 0;
  loc_ok = False;
  while((test_y <(Scr.MyDisplayHeight-height))&&(!loc_ok))
    {
      loc_ok = True;
      test_window = Scr.FvwmRoot.next;
      while((test_window != (FvwmWindow *)0)&&(loc_ok == True))
	{
	  if(test_window->icon_w)
	    {
	      if((test_window->icon_x_loc-5 < (test_x+width))&&
		 ((test_window->icon_x_loc+test_window->icon_w_width+5)>test_x)
		 &&(test_window->icon_y_loc-5 < (test_y+height))&&
		 ((test_window->icon_y_loc+test_window->icon_w_height+5)>test_y))
		{
		  loc_ok = False;
		}
	    }
	  if(!(test_window->flags & ICON))
	    {
	      /* window is not iconified */
	      if((test_window->frame_x-5 < (test_x+width))&&
		 ((test_window->frame_x+test_window->frame_width+5)>test_x)
		 &&(test_window->frame_y-5 < (test_y+height))&&
		 ((test_window->frame_y+test_window->frame_height+5)>test_y))
		{
		  loc_ok = False;
		}
	    }
	  test_window = test_window->next;
	}
      test_y +=10;
    }
  
  /* next check the bottom side of the screen */
  test_y -=10;
  if(!loc_ok)
    {
      test_y = Scr.MyDisplayHeight - height;
      loc_ok = False;
      test_x = Scr.MyDisplayWidth - width;
      while((test_x >0)&&(!loc_ok))
	{
	  loc_ok = True;
	  test_window = Scr.FvwmRoot.next;
	  while((test_window != (FvwmWindow *)0)&&(loc_ok == True))
	    {
	      if(test_window->icon_w)
		{
		  if((test_window->icon_x_loc-5 < (test_x+width))&&
		     ((test_window->icon_x_loc+test_window->icon_w_width+5)>test_x)
		     &&(test_window->icon_y_loc-5 < (test_y+height))&&
		     ((test_window->icon_y_loc+test_window->icon_w_height+5)>test_y))
		    {
		      loc_ok = False;
		    }
		}
	      if(!(test_window->flags & ICON))
		{
		  /* window is not iconified */
		  if((test_window->frame_x-5 < (test_x+width))&&
		     ((test_window->frame_x+test_window->frame_width+5)>test_x)
		     &&(test_window->frame_y-5 < (test_y+height))&&
		     ((test_window->frame_y+test_window->frame_height+5)>test_y))
		    {
		      loc_ok = False;
		    }
		}
	      test_window = test_window->next;
	    }
	  test_x -=10;
	}
      test_x +=10;
    }
  if(loc_ok == False)
    return;
  *final_x = test_x;
  *final_y = test_y;
}

