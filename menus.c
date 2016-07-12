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

int menu_on=0, resize_on=0, move_on=0;

MenuRoot *ActiveMenu = NULL;		/* the active menu */
MenuItem *ActiveItem = NULL;		/* the active menu item */

int menuFromFrameOrWindowOrTitlebar = FALSE;

extern char *Action;
extern int Context;
extern FvwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event, ButtonEvent;
extern char **g_argv,**g_envp;
extern Window Pager_w;
extern Bool KeyboardGrabbed;

/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 ***************************************************************************/
void do_menu (MenuRoot *menu)
{
  int x = Event.xbutton.x_root;
  int y = Event.xbutton.y_root;
  while(XGrabPointer(dpy, Scr.Root, True,
	       ButtonPressMask | ButtonReleaseMask |
	       ButtonMotionMask | PointerMotionHintMask,
	       GrabModeAsync, GrabModeAsync,
	       Scr.Root, Scr.MenuCursor, CurrentTime)!=GrabSuccess);
  if(!KeyboardGrabbed)
    while(XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
			CurrentTime)!=GrabSuccess);    
  XGrabServer(dpy);
  if (PopUpMenu (menu, x, y))
    {
      UpdateMenu();
    }
  else
    {
      XBell (dpy, 0);
    }
  XUngrabServer(dpy);
  XUngrabPointer(dpy,CurrentTime);
  if(!KeyboardGrabbed)
    XUngrabKeyboard(dpy,CurrentTime);
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	PaintEntry - draws a single entry in a poped up menu
 *
 ***********************************************************************/
void PaintEntry(MenuRoot *mr, MenuItem *mi)
{
  int y_offset,text_y,y,d;
  
  y_offset = mi->item_num * Scr.EntryHeight;
  text_y = y_offset + Scr.StdFont.y;

  if ((mi->state)&&(mi->func != F_TITLE))
    {
      XFillRectangle(dpy, mr->w, Scr.NormalGC, 0, y_offset,
		     mr->width, Scr.EntryHeight);
      XDrawString(dpy, mr->w, Scr.InvNormalGC, mi->x,
		  text_y, mi->item, mi->strlen);
      if(mi->func == F_POPUP)
	{
	  d=(Scr.EntryHeight-7)/2;
	  DrawPattern(mr->w, Scr.InvNormalGC, Scr.InvNormalGC, 
		      mr->width-d-7,
		      (mi->item_num*Scr.EntryHeight+d),
		      mr->width-d,
		      (mi->item_num*Scr.EntryHeight+7+d));
	}
    }
  else
    {
      XFillRectangle(dpy, mr->w, Scr.InvNormalGC, 0, y_offset,
		     mr->width, Scr.EntryHeight);
      XDrawString(dpy, mr->w, Scr.NormalGC, mi->x,
		  text_y, mi->item, mi->strlen);
      if(mi->func == F_TITLE)
	{
	  /* now draw the dividing line(s) */
	  if (mi->item_num != 0)
	  {
	    y = ((mi->item_num) * Scr.EntryHeight);
	    XDrawLine(dpy, mr->w, Scr.NormalGC, 0, y, mr->width, y);
	  }
	  y = ((mi->item_num+1) * Scr.EntryHeight)-1;
	  XDrawLine(dpy, mr->w, Scr.NormalGC, 0, y, mr->width, y);
	}
      if(mi->func == F_POPUP)
	{
	  d=(Scr.EntryHeight-7)/2;
	  if(Scr.d_depth <2)
	    DrawPattern(mr->w, Scr.StdShadowGC, Scr.StdShadowGC, 
			mr->width-d-7,
			(mi->item_num*Scr.EntryHeight+d),
			mr->width-d,
			(mi->item_num*Scr.EntryHeight+7+d));	    
	  else 
	    DrawPattern(mr->w, Scr.StdReliefGC, Scr.StdShadowGC, 
			mr->width-d-7,
			(mi->item_num*Scr.EntryHeight+d),
			mr->width-d,
			(mi->item_num*Scr.EntryHeight+7+d));
	}

    }
  return;
}
    

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/
void PaintMenu(MenuRoot *mr, XEvent *e)
{
  MenuItem *mi;
  
  for (mi = mr->first; mi != NULL; mi = mi->next)
    {
      int y_offset = mi->item_num * Scr.EntryHeight;
      /* be smart about handling the expose, redraw only the entries
       * that we need to
       */
      if (e->xexpose.y < (y_offset + Scr.EntryHeight) &&
	  (e->xexpose.y + e->xexpose.height) > y_offset)
	{
	  PaintEntry(mr, mi);
	}
    }
  XSync(dpy, 0);
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 ***********************************************************************/
void UpdateMenu()
{
  MenuItem *mi;
  int i, x, y, x_root, y_root, entry;
  int done,func;

  while (TRUE)
    {
      /* block until there is an event */
      XMaskEvent(dpy,
		 ButtonPressMask | ButtonReleaseMask |
		 EnterWindowMask | ExposureMask | KeyPressMask |
		 VisibilityChangeMask | LeaveWindowMask |
		 ButtonMotionMask, &Event);
      done = 0;
      if (Event.type == MotionNotify) 
	{
	  /* discard any extra motion events before a release */
	  while((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
				 &Event))&&(Event.type != ButtonRelease));
	}
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
      switch(Event.type)
	{
	case ButtonRelease:
	  PopDownMenu();
	  if(ActiveItem)
	    {
	      func = ActiveItem->func;
	      Action = ActiveItem->action;
	      done = 1;
	      if(ButtonWindow)
		{
		  ExecuteFunction(func, Action,ButtonWindow->lead_w,
				  ButtonWindow, &Event, Context,
				  ActiveItem->val1,ActiveItem->val2,
				  ActiveItem->menu);
		}
	      else
		{
		  ExecuteFunction(func,Action,None,ButtonWindow, &Event, 
				  Context,ActiveItem->val1,ActiveItem->val2,ActiveItem->menu);
		}
	    }
	  ActiveItem = NULL;
	  ActiveMenu = NULL;
	  menuFromFrameOrWindowOrTitlebar = FALSE;
	  return;

	case KeyPress:
	case ButtonPress:
	case EnterNotify:
	  done=1;
	  break;

	case MotionNotify:
	  done = 1;
	  XQueryPointer( dpy, ActiveMenu->w, &JunkRoot, &JunkChild,
			&x_root, &y_root, &x, &y, &JunkMask);
	  /* look for the entry that the mouse is in */
	  entry = y / Scr.EntryHeight;
	  mi = ActiveMenu->first;
	  i=0;
	  while((i!=entry)&&(mi!=NULL))
	    {
	      i++;
	      mi=mi->next;
	    }
	  /* if we weren't on the active entry, let's turn the old
	   * active one off */
	  if ((ActiveItem)&&(ActiveItem->func != F_TITLE)&&(mi!=ActiveItem))
	    {
	      ActiveItem->state = 0;
	      PaintEntry(ActiveMenu, ActiveItem);
	    }
	  
	  /* if we weren't on the active item, change the active item
	   * and turn it on */
	  if ((mi!=ActiveItem)&&(mi != NULL))
	    {
	      if (mi->func != F_TITLE && !mi->state)
		{
		  mi->state = 1;
		  PaintEntry(ActiveMenu, mi);
		}
	    }
	  ActiveItem = mi;
	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if(Event.xany.window == ActiveMenu->w)
	    {
	      PaintMenu(ActiveMenu,&Event);
	      done = 1;
	    }
	  break;

	case VisibilityNotify:
	  done=1;
	  break;

	default:
	  break;
	}
      
      if(!done)DispatchEvent();
      XFlush(dpy);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *      center	- whether or not to center horizontally over position
 *
 ***********************************************************************/
int Stashed_X;
int Stashed_Y;
Bool PopUpMenu (MenuRoot *menu, int x, int y)
{
  if ((!menu)||(menu->w == None)||(menu->items == 0))
    return False;
  
  menu_on = 1;
  InstallRootColormap();

  /* In case we wind up with a move from a menu which is
   * from a window border, we'll return to here to start
   * the move */
  Stashed_X = x;
  Stashed_Y = y;
  
  /* pop up the menu */
  ActiveMenu = menu;
  
  x -= (menu->width / 2);
  y -= (Scr.EntryHeight / 2);
  
  /* clip to screen */
  if (x + menu->width > Scr.MyDisplayWidth) 
    x = Scr.MyDisplayWidth - menu->width;
  if (x < 0) x = 0;

  if (y + menu->height > Scr.MyDisplayHeight) 
    y = Scr.MyDisplayHeight - menu->height;
  if (y < 0) y = 0;

  XMoveWindow(dpy, menu->w, x, y);
  XMapRaised(dpy, menu->w);
  XUngrabPointer(dpy,CurrentTime);
  XGrabPointer(dpy, Scr.Root, True,
	       ButtonPressMask | ButtonReleaseMask |
	       ButtonMotionMask | PointerMotionHintMask,
	       GrabModeAsync, GrabModeAsync,
	       menu->w, Scr.MenuCursor, CurrentTime);
  XSync(dpy, 0);
  return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************/
void PopDownMenu()
{
  if (ActiveMenu == NULL)
    return;
  
  menu_on = 0;
  if (ActiveItem)
    {
      ActiveItem->state = 0;
      PaintEntry(ActiveMenu, ActiveItem);
    }
  
  XUngrabPointer(dpy,CurrentTime);
  XUnmapWindow(dpy, ActiveMenu->w);
  UninstallRootColormap();
  XFlush(dpy);
  if (Context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    menuFromFrameOrWindowOrTitlebar = TRUE;
  else
    menuFromFrameOrWindowOrTitlebar = FALSE;
}

/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a fvwm built in function
 *
 *  Inputs:
 *	func	- the function to execute
 *	action	- the menu action to execute 
 *	w	- the window to execute this function on
 *	tmp_win	- the fvwm window structure
 *	event	- the event that caused the function
 *	context - the context in which the button was pressed
 *      val1,val2 - the distances to move in a scroll operation 
 *
 ***********************************************************************/
void ExecuteFunction(int func,char *action, Window w, FvwmWindow *tmp_win, 
		    XEvent *eventp, int context,int val1, int val2,MenuRoot *menu)
{
  FvwmWindow *t;
  Bool found;
  char *junk;

  switch (func)
    {
    case F_NOP:
    case F_TITLE:
      break;
      
    case F_BEEP:
      XBell(dpy, 0);
      break;
      
    case F_RESIZE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.ResizeCursor,ButtonPress))
	return;
      resize_on = 1;
      resize_window(eventp,w,tmp_win);
      resize_on = 0;
      break;
      
    case F_MOVE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.MoveCursor,ButtonPress))
	return;
      move_on = 1;
      move_window(eventp,w,tmp_win,context);
      move_on = 0;
      break;
      
    case F_SCROLL:
      MoveViewport(Scr.Vx + val1, Scr.Vy+val2);
      break;
    case F_ICONIFY:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.SelectCursor,ButtonRelease))
	return;
      
      if (tmp_win->flags & ICON)
	DeIconify(tmp_win);
      else
	Iconify(tmp_win, eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
      break;
      
    case F_RAISE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.SelectCursor,ButtonRelease))
	return;
      
      if (w == tmp_win->icon_w )
	XRaiseWindow(dpy, tmp_win->icon_w);
      XRaiseWindow(dpy,tmp_win->lead_w);
      if (LookInList(Scr.OnTop,tmp_win->name, &tmp_win->class, &junk))
	tmp_win->flags |= ONTOP;
      KeepOnTop();
      Scr.LastWindowRaised = tmp_win;
      break;
      
    case F_LOWER:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.SelectCursor,ButtonRelease))
	return;
      
      if (w == tmp_win->icon_w)
	XLowerWindow(dpy, tmp_win->icon_w);	
      else
	XLowerWindow(dpy, tmp_win->lead_w);

      if(Scr.LastWindowRaised == tmp_win)
	Scr.LastWindowRaised = (FvwmWindow *)0;

      tmp_win->flags &= ~ONTOP;
      break;
      
    case F_DESTROY:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.DestroyCursor,ButtonRelease))
	return;

      XKillClient(dpy, tmp_win->w);
      break;
      
    case F_DELETE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, Scr.DestroyCursor,ButtonRelease))
	return;
      
      if (tmp_win->protocols & DoesWmDeleteWindow)
	send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
      else
	XBell (dpy, 0);
      break;
      
    case F_RESTART:
      MoveViewport(0,0);
      if(Pager_w != (Window)0)
	XDestroyWindow(dpy,Pager_w);
      Reborder ();
      XSync(dpy,0);
      XCloseDisplay(dpy);
      execve(g_argv[0],g_argv,g_envp);
      break;

    case F_EXEC:
      XGrabPointer(dpy, Scr.Root, True,
		   ButtonPressMask | ButtonReleaseMask,
		   GrabModeAsync, GrabModeAsync,
		   Scr.Root, Scr.WaitCursor, CurrentTime);
      XSync (dpy, 0);
      system(action);
      XUngrabPointer(dpy,CurrentTime);
      break;
      
    case F_REFRESH:
      {
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	
	valuemask = (CWBackPixel);
	attributes.background_pixel = Scr.StdColors.fore;
	attributes.backing_store = NotUseful;
	w = XCreateWindow (dpy, Scr.Root, 0, 0,
			   (unsigned int) Scr.MyDisplayWidth,
			   (unsigned int) Scr.MyDisplayHeight,
			   (unsigned int) 0,
			   CopyFromParent, (unsigned int) CopyFromParent,
			   (Visual *) CopyFromParent, valuemask,
			   &attributes);
	XMapWindow (dpy, w);
	XDestroyWindow (dpy, w);
	XFlush (dpy);
      }
      break;

    case F_STICK:
      /* stick/unstick a window */
      if (DeferExecution(eventp,&w,&tmp_win,&context, 
			 Scr.SelectCursor,ButtonRelease))
	return;
      if(tmp_win->flags & STICKY)
	tmp_win->flags &= ~STICKY;
      else
	tmp_win->flags |=STICKY;
      break;

    case F_NEXT_PAGE:
      /* Move forward 1 virtual desktop page */
      NextPage();
      break;

    case F_PREV_PAGE:
      /* back up 1 virtual desktop page */
      PrevPage();
      break;

    case F_CIRCULATE_UP:
      /* move focus to the previous window */
      found = FALSE;
      t = Scr.Focus;
      while(!found)
	{
	  if ((t == (FvwmWindow *)0)||(t == &Scr.FvwmRoot)||
	      (t->prev == &Scr.FvwmRoot)||(t->prev == (FvwmWindow *)NULL))
	    for(t=Scr.FvwmRoot.next;t->next != (FvwmWindow *)NULL;t=t->next);
	  else
	    t=t->prev;
	  found = TRUE;
	  if (LookInList(Scr.CirculateSkip,t->name, &t->class,&junk))
	    found = FALSE;
	  if (LookInList(Scr.CirculateSkip,t->icon_name, &t->class,&junk))
	    found = FALSE;
	}
      FocusOn(t);
	  
      break;

    case F_CIRCULATE_DOWN:
      /* move focus to the next window */
      found = FALSE;
      t  = Scr.Focus;
      while(!found)
	{
	  if((t == (FvwmWindow *)0)||(t->next == NULL))
	    t = Scr.FvwmRoot.next;
	  else
	    t =t->next;
	  found = TRUE;
	  if (LookInList(Scr.CirculateSkip,t->name, &t->class,&junk))
	    found = FALSE;
	  if (LookInList(Scr.CirculateSkip,t->icon_name, &t->class,&junk))
	    found = FALSE;
	}
      FocusOn(t);
	  
      break;

    case F_RAISELOWER:
      if (DeferExecution(eventp,&w,&tmp_win,&context, 
			 Scr.SelectCursor,ButtonRelease))
	return;

      if((tmp_win == Scr.LastWindowRaised)||
	 (tmp_win->flags & VISIBLE))
	{
	  if (w == tmp_win->icon_w)
	    XLowerWindow(dpy, tmp_win->icon_w);	
	  else
	    XLowerWindow(dpy, tmp_win->lead_w);
	  tmp_win->flags &= ~ONTOP;
	  if(Scr.LastWindowRaised == tmp_win)
	    Scr.LastWindowRaised = (FvwmWindow *)0;
	}
      else
	{
	  if (w == tmp_win->icon_w)
	    XRaiseWindow(dpy, tmp_win->icon_w);	
	  else
	    XRaiseWindow(dpy, tmp_win->lead_w);
	  if (LookInList(Scr.OnTop,tmp_win->name, &tmp_win->class,&junk))
	    tmp_win->flags |= ONTOP;	    
	    KeepOnTop();
	  Scr.LastWindowRaised = tmp_win;

	}
      break;

    case F_POPUP:
      ActiveItem = NULL;
      ActiveMenu = NULL;
      menuFromFrameOrWindowOrTitlebar = FALSE;
      do_menu(menu);
      break;

    case F_QUIT:
      Done();
      break;
    }
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to FvwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(XEvent *eventp, Window *w,FvwmWindow **tmp_win,
		   int *context, Cursor cursor, int FinishEvent)

{
  int done;
  int finished = 0;

  if(*context != C_ROOT)
    return FALSE;

  XUngrabPointer(dpy,CurrentTime);
  while(XGrabPointer(dpy, Scr.Root, True,
	       ButtonPressMask | ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync,
	       Scr.Root, cursor, CurrentTime)!=GrabSuccess);
  if(!KeyboardGrabbed)
    while(XGrabKeyboard(dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, 
			CurrentTime)!=GrabSuccess);
  
  while (!finished)
    {
      done = 0;
      /* block until there is an event */
      XMaskEvent(dpy,
		 ButtonPressMask | ButtonReleaseMask |
		 EnterWindowMask | ExposureMask |KeyPressMask |
		 VisibilityChangeMask | LeaveWindowMask |
		 ButtonMotionMask, eventp);
      if(eventp->type == KeyPress)
	Keyboard_shortcuts(eventp,FinishEvent);	
      if(eventp->type == FinishEvent)
	finished = 1;
      if(eventp->type == ButtonPress)
	done = 1;
      if(eventp->type == ButtonRelease)
	done = 1;
      if(eventp->type == EnterNotify)
	done = 1;
      if(eventp->type == LeaveNotify)
	done = 1;
      if(!done)DispatchEvent();
      
    }

  *w = eventp->xany.window;
  if((*w == Scr.Root) && (eventp->xbutton.subwindow != (Window)0))
    *w = eventp->xbutton.subwindow;
  if (*w == Scr.Root)
    {
      *context = C_ROOT;
      XBell(dpy,0);
      XUngrabPointer(dpy,CurrentTime);
      if(!KeyboardGrabbed)
	{
	  XUngrabKeyboard(dpy,CurrentTime);
	}
      return TRUE;
    }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
    {
      *tmp_win = NULL;
      XBell(dpy,0);
      XUngrabPointer(dpy,CurrentTime);
      if(!KeyboardGrabbed)
	{
	  XUngrabKeyboard(dpy,CurrentTime);
	}
      return (TRUE);
    }
  
  if(*w == (*tmp_win)->title_w)
    *context = C_TITLE;
  else if(*w == (*tmp_win)->w)
    *context = C_WINDOW;
  else if(*w == (*tmp_win)->icon_w)
    *context = C_ICON;
  else if(*w == (*tmp_win)->frame)
    *context = C_FRAME;
  else if((*w == (*tmp_win)->left_side_w)||(*w == (*tmp_win)->right_side_w)
	  ||(*w == (*tmp_win)->bottom_w)||(*w == (*tmp_win)->top_w))
    *context = C_SIDEBAR;

  XUngrabPointer(dpy,CurrentTime);
  if(!KeyboardGrabbed)
    {
      XUngrabKeyboard(dpy,CurrentTime);
    }
  return FALSE;
}





