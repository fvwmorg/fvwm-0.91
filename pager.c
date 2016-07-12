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

extern XEvent Event;

extern int menu_on,move_on;

extern Window Pager_w;

void RedrawPager()
{
  int width,height,ww,wh;
  int wx,wy;
  int  x,y,x1,x2,y1,y2,X,Y;
  FvwmWindow *t;
  int MaxH,MaxW;

  if(Pager_w == 0)
    return;

  XGetGeometry(dpy, Pager_w, &JunkRoot, &X, &Y,
	       &width, &height, &JunkBW,  &JunkDepth);
  MaxW = Scr.VxMax + Scr.MyDisplayWidth;
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  XClearWindow(dpy,Pager_w);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if(!(t->flags & STICKY))
	{
	  if(t->flags & ICON)
	    {
	      /* show the icon loc */
	      wx = (t->icon_x_loc + Scr.Vx)*width/MaxW;;
	      wy = (t->icon_y_loc + Scr.Vy)*height/MaxH;
	      ww = t->icon_w_width*width/MaxW;
	      wh = (ICON_HEIGHT)*height/MaxH;
	    }
	  else 
	    {
	      /* show the actual window */
	      wx = (t->frame_x + Scr.Vx)*width/MaxW;
	      wy = (t->frame_y + Scr.Vy)*height/MaxH;
	      ww = t->frame_width*width/MaxW;
	      wh = t->frame_height*height/MaxH;
	    }
	  if(ww<2)ww=2;
	  if(wh<2)wh=2;
	  XFillRectangle(dpy,Pager_w,Scr.NormalGC,wx,wy,ww ,wh);
	}
    }
  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = height;
  while(x < MaxW)
    {
      x1 = x*width/MaxW;
      XDrawLine(dpy,Pager_w,Scr.NormalGC,x1,y1,x1,y2);
      x += Scr.MyDisplayWidth;
    }
  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = width;
  while(y < MaxH)
    {
      y1 = y*height/MaxH;
      XDrawLine(dpy,Pager_w,Scr.NormalGC,x1,y1,x2,y1);
      y += Scr.MyDisplayHeight;
    }
  ShowCurrentPort();
}

void ShowCurrentPort()
{
  int x1,y1,x2,y2,X,Y;
  int width,height;

  if(Pager_w == (Window)0)
    return;

  XGetGeometry(dpy, Pager_w, &JunkRoot, &X, &Y,
	       &width, &height, &JunkBW,  &JunkDepth);
  x1 = Scr.Vx * width/(Scr.VxMax+Scr.MyDisplayWidth)+2;
  y1 = Scr.Vy * height/(Scr.VyMax+Scr.MyDisplayHeight)+2;
  if(x1==2)x1=1;
  if(y1==2)y1=1;
  x2 = (Scr.Vx+Scr.MyDisplayWidth) * width/(Scr.VxMax+Scr.MyDisplayWidth) -2;
  y2 = (Scr.Vy+Scr.MyDisplayHeight) * height/(Scr.VyMax+Scr.MyDisplayHeight)-2;
  XDrawLine(dpy,Pager_w,Scr.DrawGC,x1,y1,x1,y2);
  XDrawLine(dpy,Pager_w,Scr.DrawGC,x1,y2,x2,y2);
  XDrawLine(dpy,Pager_w,Scr.DrawGC,x2,y2,x2,y1);
  XDrawLine(dpy,Pager_w,Scr.DrawGC,x2,y1,x1,y1);
  return;
}

void SwitchPages(Bool align)
{
  int x,y,width,height,X,Y;

  Window dumwin;
  if(Pager_w == 0)
    return;
  XTranslateCoordinates (dpy, Event.xbutton.window, Pager_w,
			 Event.xbutton.x, Event.xbutton.y, &x, &y, &dumwin);

  XGetGeometry(dpy, Pager_w, &JunkRoot, &X, &Y,
	       &width, &height, &JunkBW,  &JunkDepth);
  x = x * (Scr.VxMax+Scr.MyDisplayWidth)/width;
  y = y * (Scr.VyMax+Scr.MyDisplayHeight)/height;
  if(align)
    {
      x = (x/Scr.MyDisplayWidth)*Scr.MyDisplayWidth;
      y = (y/Scr.MyDisplayHeight)*Scr.MyDisplayHeight;
    }
  MoveViewport(x,y);
}


void NextPage()
{
  int x,y;

  x=((Scr.Vx + Scr.MyDisplayWidth)/Scr.MyDisplayWidth)*Scr.MyDisplayWidth;
  y=(Scr.Vy/Scr.MyDisplayHeight)*Scr.MyDisplayHeight;
  if(x >Scr.VxMax)
    {
      x=0;
      y=((Scr.Vy + Scr.MyDisplayHeight)/Scr.MyDisplayHeight)*
	Scr.MyDisplayHeight;
    }
  if(y>Scr.VyMax)
    {
      y=0;
    }
  MoveViewport(x,y);
}


void PrevPage()
{
  int x,y;

  x=((Scr.Vx - Scr.MyDisplayWidth)/Scr.MyDisplayWidth)*Scr.MyDisplayWidth;
  y=(Scr.Vy/Scr.MyDisplayHeight)*Scr.MyDisplayHeight;
  if(x < 0)
    {
      x=Scr.VxMax;
      y=((Scr.Vy - Scr.MyDisplayHeight)/Scr.MyDisplayHeight)*
	Scr.MyDisplayHeight;
    }
  if(y < 0)
    {
      y=Scr.VyMax;
    }
  MoveViewport(x,y);
}


