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


#ifndef _MISC_
#define _MISC_

typedef struct name_list_struct
{
  struct name_list_struct *next;   /* pointer to the next name */
  char *name;		  	   /* the name of the window */
  char *value;
} name_list;

int LookInList(name_list *, char *, XClassHint *, char **value);

extern void	MoveOutline();

extern void StartResize();
extern void DoResize();
extern void DisplaySize();
extern void EndResize();
extern void SetupFrame();

void CreateGCs();

void InstallWindowColormaps();
void InstallRootColormap();
extern void UninstallRootColormap();
extern void FetchWmProtocols();
extern void PaintEntry();
extern void PaintMenu();
extern void UpdateMenu();
extern void MakeMenus();
extern void InitEvents();
extern void DispatchEvent();
extern void HandleEvents();
extern void HandleExpose();
extern void HandleDestroyNotify();
extern void HandleMapRequest();
extern void HandleMapNotify();
extern void HandleUnmapNotify();
extern void HandleMotionNotify();
extern void HandleButtonRelease();
extern void HandleButtonPress();
extern void HandleEnterNotify();
extern void HandleLeaveNotify();
extern void HandleConfigureRequest();
extern void HandleClientMessage();
extern void HandlePropertyNotify();
extern void HandleKeyPress();
extern void HandleVisibilityNotify();
extern void SetTitleBar(FvwmWindow *, Bool);
extern void RestoreWithdrawnLocation(FvwmWindow *);
extern void Destroy(FvwmWindow *);
extern void GetGravityOffsets (FvwmWindow *, int *, int *);
extern void MoveViewport(int newx, int newy);

extern Time lastTimestamp;
extern XEvent Event;

extern char NoName[];

extern FvwmWindow *AddWindow();
extern int MappedNotOverride();
extern void GrabButtons();
extern void GrabKeys();
extern void GetWindowSizeHints();
extern void RedrawPager();
extern void SwitchPages(Bool);
extern void NextPage();
extern void PrevPage();
extern void ShowCurrentPort();

void moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY);
void Keyboard_shortcuts(XEvent *, int);
void RedoIconName(FvwmWindow *);
void DrawIconWindow(FvwmWindow *);
void CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y);

#endif /* _MISC_ */

