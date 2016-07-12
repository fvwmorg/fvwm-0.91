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
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _MENUS_
#define _MENUS_

#define MAX_FILE_SIZE 4096	/* max chars to read from file for cut */

typedef struct MenuItem
{
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */
    char *item;			/* the character string displayed */
    char *action;		/* action to be performed */
    short item_num;		/* item number of this menu */
    short x;			/* x coordinate for text */
    short func;			/* twm built in function */
    int val1;                   /* values needed for F_SCROLL */
    int val2;
    short state;		/* video state, 0 = normal, 1 = reversed */
    short strlen;		/* strlen(item) */
    struct MenuRoot *menu;      /* sub-menu */
} MenuItem;

typedef struct MenuRoot
{
    struct MenuItem *first;	/* first item in menu */
    struct MenuItem *last;	/* last item in menu */
    struct MenuRoot *next;	/* next in list of root menus */
    char *name;			/* name of root */
    Window w;			/* the window of the menu */
    short height;		/* height of the menu */
    short width;		/* width of the menu */
    short items;		/* number of items in the menu */
} MenuRoot;

typedef struct MouseButton
{
    int func;			/* the function number */
    MenuRoot *menu;		/* menu if func is F_POPUP */
    MenuItem *item;		/* action to perform if func != F_POPUP */
    int Button;
    int Context;
    int Modifier;
    int val1;
    int val2;
    struct MouseButton *NextButton;
} MouseButton;

typedef struct FuncKey
{
    struct FuncKey *next;	/* next in the list of function keys */
    char *name;			/* key name */
    KeyCode keycode;		/* X keycode */
    int cont;			/* context */
    int mods;			/* modifiers */
    int func;			/* function to perform */
    int val1;                   /* values needed for F_SCROLL */
    int val2;
    MenuRoot *menu;		/* menu if func is F_POPUP */
    char *action;		/* action string (if any) */
} FuncKey;

extern MenuRoot *ActiveMenu;
extern MenuItem *ActiveItem;

extern Bool PopUpMenu();
extern void ExecuteFunction();
extern int DeferExecution();
void send_clientmessage (Window, Atom, Time);
void SetBorder (FvwmWindow *, Bool,Bool,Bool);
void move_window(XEvent *,Window,FvwmWindow *,int);
void resize_window(XEvent *,Window,FvwmWindow *);
void CreateIconWindow(FvwmWindow *, int, int);
void send_clientmessage (Window, Atom, Time);
void SetMapStateProp(FvwmWindow *, int);
void Iconify(FvwmWindow *, int, int);
void DeIconify(FvwmWindow *);
void PopDownMenu();
void KeepOnTop();
void show_panner();
void do_menu (MenuRoot *menu);
#endif /* _MENUS_ */
