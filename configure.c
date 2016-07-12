/****************************************************************************
 * Some parts of this module are based on Twm code, but have been 
 * siginificantly modified by Rob Nation (nation@rocket.sanders.lockheed.com)
 * Much of the code is new.
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


/***************************************************************************
 * 
 * Configure.c: reads the .fvwmrc or system.fvwmrc file, interprets it,
 * and sets up menus, bindings, colors, and fonts as specified
 *
 ***************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

char *white = "white";
char *black = "black";
char *Stdback;
char *Stdfore;
char *Hiback;
char *Hifore;

void     GetColors();
Pixel    GetShadow(Pixel);
Pixel    GetHilite(Pixel);
Pixel    GetColor(char *);
void	 ParsePopupEntry(char *,FILE *, char **);
void     ParseMouseEntry(char *,FILE *, char **);
void     ParseKeyEntry(char *, FILE *, char **);
void     MakeMenu(MenuRoot *);
void     AddToMenu(MenuRoot *, char *, char *,int, int, int);
void     AddToList(char *text,FILE *,name_list **);
void     AddToList2(char *text,FILE *,name_list **);
MenuRoot *NewMenuRoot(char *name);
void     AddFuncKey (char *, int, int, int, char *, int, int, MenuRoot *);
char     *safemalloc(int length);
char     *stripcpy(char *);
char     *stripcpy2(char *);
char     *stripcpy3(char *);
void     initialize_pager(int x, int y);

int contexts;
int mods,func,func_val_1,func_val_2;

int pager_x,pager_y;
Window Pager_w = (Window)0;
Bool pager = FALSE;

#define MAXPOPUPS 16

unsigned PopupCount = 0;
MenuRoot *PopupTable[MAXPOPUPS];

struct config
{
  char *keyword;
  void (*action)();
  char **arg;
};

void assign_string(char *text, FILE *fd, char **arg);
void EdgeFix(char *text, FILE *fd, char **arg);
void SetFlag(char *text, FILE *fd, Bool *arg);
void ActivatePager(char *text, FILE *fd, char **arg);
void SetOneInt(char *text, FILE *fd, int *arg);
void SetSize(char *text, FILE *fd, char **arg);
void set_func(char *, FILE *, int);
void match_string(struct config *, char *, char *, FILE *);

struct config main_config[] =
{
  {"Font",              assign_string,  &Scr.StdFont.name},
  {"StdForeColor",      assign_string,  &Stdfore},
  {"StdBackColor",      assign_string,  &Stdback},
  {"HiForeColor",       assign_string,  &Hifore},
  {"HiBackColor",       assign_string,  &Hiback},
  {"NoTitle",           AddToList,      (char **)&Scr.NoTitle},
  {"Sticky",            AddToList,      (char **)&Scr.Sticky},
  {"StaysOnTop",        AddToList,      (char **)&Scr.OnTop},
  {"CirculateSkip",     AddToList,      (char **)&Scr.CirculateSkip},
  {"Icon",              AddToList2,      (char **)&Scr.Icons},
  {"NoEdgeScroll",      EdgeFix,        (char **)0},
  {"EdgePage",          EdgeFix,        (char **)1},
  {"RandomPlacement",   SetFlag,        (char **)&Scr.RandomPlacement},
  {"DontMoveOff",       SetFlag,        (char **)&Scr.DontMoveOff},
  {"DecorateTransients",SetFlag,        (char **)&Scr.DecorateTransients},
  {"CenterOnCirculate", SetFlag,        (char **)&Scr.CenterOnCirculate},
  {"AutoPlaceIcons",    SetFlag,        (char **)&Scr.AutoPlaceIcons},
  {"Pager",             ActivatePager,  (char **)1},
  {"DeskTopScale",      SetOneInt,      (char **)&Scr.VScale},
  {"BoundaryWidth",     SetOneInt,      (char **)&Scr.BoundaryWidth},
  {"DeskTopSize",       SetSize,        (char **)1},
  {"Mouse",             ParseMouseEntry,(char **)1},
  {"Popup",             ParsePopupEntry,(char **)1},
  {"Key",               ParseKeyEntry,  (char **)1},
  {"ClickToFocus",      SetFlag,        (char **)&Scr.ClickToFocus},
  {"",                  0,              (char **)0}
};

struct config func_config[] =
{
  {"Nop",          set_func,(char **)F_NOP},
  {"Title",        set_func,(char **)F_TITLE},
  {"Beep",         set_func,(char **)F_BEEP},
  {"Quit",         set_func,(char **)F_QUIT},
  {"Refresh",      set_func,(char **)F_REFRESH},
  {"Move",         set_func,(char **)F_MOVE},
  {"Iconify",      set_func,(char **)F_ICONIFY},
  {"Resize",       set_func,(char **)F_RESIZE},
  {"RaiseLower",   set_func,(char **)F_RAISELOWER},
  {"Raise",        set_func,(char **)F_RAISE},
  {"Lower",        set_func,(char **)F_LOWER},
  {"Delete",       set_func,(char **)F_DELETE},
  {"Destroy",      set_func,(char **)F_DESTROY},
  {"PopUp",        set_func,(char **)F_POPUP},
  {"Scroll",       set_func,(char **)F_SCROLL},
  {"Stick",        set_func,(char **)F_STICK},
  {"CirculateUp",  set_func,(char **)F_CIRCULATE_UP},
  {"CirculateDown",set_func,(char **)F_CIRCULATE_DOWN},
  {"NextPage",     set_func,(char **)F_NEXT_PAGE},
  {"PrevPage",     set_func,(char **)F_PREV_PAGE},
  {"Exec",         set_func,(char **)F_EXEC},
  {"Restart",      set_func,(char **)F_RESTART},
  {"",                    0,(char **)0}
};
  
struct charstring 
{
  char key;
  int  value;
};

struct charstring win_contexts[]=
{
  {'w',C_WINDOW},
  {'t',C_TITLE},
  {'i',C_ICON},
  {'r',C_ROOT},
  {'f',C_FRAME},
  {'s',C_SIDEBAR},
  {'1',C_SYS},
  {'2',C_ICONIFY},
  {'a',C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|C_SYS|C_ICONIFY},
  {0,0}
};

struct charstring key_modifiers[]=
{
  {'s',ShiftMask},
  {'c',ControlMask},
  {'m',Mod1Mask},
  {'n',0},
  {0,0}
};

void     find_context(char *, int *, struct charstring *);

/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void MakeMenus()
{
  char *system_file = FVWMRC;
  char *home_name = "/.fvwmrc";
  char *home_file;
  FILE *fd = (FILE *)0;
  char line[256],*tline;
  char *Home;			/* the HOME environment variable */
  int HomeLen;			/* length of Home */
  MouseButton *mbtmp;
  
  Stdback = white;
  Stdfore = black;
  Hiback = white;
  Hifore = black;

  /* initialize some lists */
  Scr.MouseButtonRoot = NULL;
  Scr.FuncKeyRoot.next = NULL;
  Scr.NoTitle = NULL;
  Scr.OnTop = NULL;
  Scr.CirculateSkip = NULL;

  /* find the home directory to look in */
  Home = getenv("HOME");
  if (Home == NULL)
    Home = "./";
  HomeLen = strlen(Home);

  home_file = safemalloc(HomeLen+strlen(home_name)+1);
  strcpy(home_file,Home);
  strcat(home_file,home_name);

  fd = fopen(home_file,"r");
  if(fd == (FILE *)0)
    fd = fopen(system_file,"r");
  if(fd == (FILE *)0)
    {
      fprintf(stderr,"fvwm: can't open file %s or %s\n",
	      system_file,home_file);
      exit(1);
    }
  free(home_file);

  tline = fgets(line,(sizeof line)-1,fd);
  while(tline != (char *)0)
    {
      while(isspace(*tline))tline++;
      if((strlen(&tline[0])>1)&&(tline[0]!='#'))
	match_string(main_config,tline,"error in config:",fd);
      tline = fgets(line,255,fd);
    }
  GetColors();
  
  if(pager)initialize_pager(pager_x,pager_y);
  return;
}

/*****************************************************************************
 * 
 * Copies a text string from the config file to a specified location
 *
 ****************************************************************************/
void assign_string(char *text, FILE *fd, char **arg)
{
  *arg = stripcpy(text);
}

/*****************************************************************************
 * 
 * Sets the Scroll-on-edge size (0 or display width)
 *
 ****************************************************************************/
void EdgeFix(char *text, FILE *fd, char **arg)
{
  Scr.EdgeScrollX = (int)arg*Scr.MyDisplayWidth;
  Scr.EdgeScrollY = (int)arg*Scr.MyDisplayHeight;
}

/*****************************************************************************
 * 
 * Sets a boolean flag to true
 *
 ****************************************************************************/
void SetFlag(char *text, FILE *fd, Bool *arg)
{
  *arg = TRUE;
}

/*****************************************************************************
 * 
 * Activates the Pager, and reads its location
 *
 ****************************************************************************/
void ActivatePager(char *text, FILE *fd, char **arg)
{
  if(sscanf(text,"%d %d",&pager_x,&pager_y)!=2)
    {
      pager_x = -1;
      pager_y = -1;
    }
  pager = TRUE;
}

/*****************************************************************************
 * 
 * Reads an integer from the config file to set a specified variable
 *
 ****************************************************************************/
void SetOneInt(char *text, FILE *fd, int *arg)
{
  sscanf(text,"%d",arg);
}

/*****************************************************************************
 * 
 * Reads an 2 integers from the config file to set the desktop size
 *
 ****************************************************************************/
void SetSize(char *text, FILE *fd, char **arg)
{
  sscanf(text,"%dx%d",&Scr.VxMax,&Scr.VyMax);
  Scr.VxMax = Scr.VxMax*Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = Scr.VyMax*Scr.MyDisplayHeight - Scr.MyDisplayHeight;
}


/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow(Pixel background) 
{
  XColor bg_color;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);
  
  bg_color.red = (unsigned short)((bg_color.red*50)/100);
  bg_color.green = (unsigned short)((bg_color.green*50)/100);
  bg_color.blue = (unsigned short)((bg_color.blue*50)/100);
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    fprintf(stderr, "fvwm: can't alloc shadow color.\n");
  
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite(Pixel background) 
{
  XColor bg_color, white_p;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);

  white_p.pixel = GetColor(white);
  XQueryColor(dpy,attributes.colormap,&white_p);
  
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
  
  bg_color.red = max((white_p.red/5), bg_color.red);
  bg_color.green = max((white_p.green/5), bg_color.green);
  bg_color.blue = max((white_p.blue/5), bg_color.blue);
  
  bg_color.red = min(white_p.red, (bg_color.red*140)/100);
  bg_color.green = min(white_p.green, (bg_color.green*140)/100);
  bg_color.blue = min(white_p.blue, (bg_color.blue*140)/100);
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    fprintf(stderr, "fvwm: can't alloc hilight color.\n");
  
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine loads all needed colors, and fonts,
 * and creates the GC's
 *
 ***************************************************************************/
void GetColors()
{
  static int have_em = 0;

  if(have_em) return;

  have_em = 1;

  /* setup default colors */
  if(Scr.d_depth < 2)
    {
      /* black and white - override user choices */
      Scr.StdColors.back = GetColor(white);
      Scr.StdColors.fore = GetColor(black); 
      Scr.HiColors.back  = GetColor(white);
      Scr.HiColors.fore  = GetColor(black); 
      Scr.StdRelief.back = GetColor(black);
      Scr.StdRelief.fore = GetColor(white);
      Scr.HiRelief.back  = GetColor(black);
      Scr.HiRelief.fore  = GetColor(white);
    }
  else
    {
      /* color - accept user choices */
      Scr.StdColors.back = GetColor(Stdback);
      Scr.StdColors.fore = GetColor(Stdfore); 
      Scr.HiColors.back  =  GetColor(Hiback);
      Scr.HiColors.fore  = GetColor(Hifore); 
      Scr.StdRelief.back = GetShadow(Scr.StdColors.back);
      Scr.StdRelief.fore = GetHilite(Scr.StdColors.back);
      Scr.HiRelief.back  = GetShadow(Scr.HiColors.back);
      Scr.HiRelief.fore  = GetHilite(Scr.HiColors.back);
    }

  /* load the font */
  if ((Scr.StdFont.font = XLoadQueryFont(dpy, Scr.StdFont.name)) == NULL)
    {
      fprintf (stderr, "fvwm: can't get font %s\n", Scr.StdFont.name);
      if ((Scr.StdFont.font = XLoadQueryFont(dpy, "fixed")) == NULL)
	{
	  fprintf (stderr, "fvwm: can't get font %s\n", Scr.StdFont.name);
	  exit(1);
	}
    }
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
  Scr.StdFont.y = Scr.StdFont.font->ascent;
  Scr.EntryHeight = Scr.StdFont.height + 2;

  /* create graphics contexts */
  CreateGCs();

  return;
}

/****************************************************************************
 * 
 *  Processes a menu body definition
 *
 ****************************************************************************/ 
MenuRoot *ParseMenuBody(char *name,FILE *fd)
{
  MenuRoot *mr;
  char newline[256];
  register char *pline;
  char *ptr;

  pline = fgets(newline,(sizeof newline)-1,fd);
  if (pline == NULL)
    return 0;
  mr = NewMenuRoot(name);
  GetColors();

  while(isspace(*pline))pline++;
  while((pline != (char *)0)
      &&(strncasecmp("EndPopup",pline,8)!=0))
    {
      if((*pline!='#')&&(*pline != 0))
	{
	  char *ptr2 = 0;
	  match_string(func_config,pline, "bad menu body function:",fd);
	  if((func == F_EXEC)||(func == F_POPUP))
	    ptr2=stripcpy3(pline);
	  
	  AddToMenu(mr, stripcpy2(pline), ptr2, func,func_val_1,func_val_2);
	}
      
      pline = fgets(newline,(sizeof newline)-1,fd);

      while(isspace(*pline))pline++;
    }
  MakeMenu(mr);

  return mr;
}

/****************************************************************************
 * 
 *  Parses a popup definition 
 *
 ****************************************************************************/ 
void ParsePopupEntry(char *tline,FILE *fd, char **junk)
{
  MenuRoot *mr=0;
  mr = ParseMenuBody(stripcpy2(tline),fd);
  if (PopupCount < MAXPOPUPS)
    {
      PopupTable[PopupCount] = mr;
      PopupCount++;
    }
  else
    {
      fprintf(stderr,"Popup %s ignored, you have more than %u\n",
	      mr->name,MAXPOPUPS);
      free(mr);
    }
}

/****************************************************************************
 * 
 *  Parses a mouse binding
 *
 ****************************************************************************/ 
void ParseMouseEntry(char *tline,FILE *fd, char **junk)
{
  char context[256],modifiers[256],function[256],*ptr;
  MenuRoot *mr=0;
  MenuItem *mi=0;
  MouseButton *temp;
  int button;

  func_val_1 = 0;
  func_val_2 = 0;

  sscanf(tline,"%d %s %s %s %d %d",&button,context,modifiers,function,
	 &func_val_1,&func_val_2);
  
  find_context(context,&contexts,win_contexts);
  find_context(modifiers,&mods,key_modifiers);
  if((contexts == C_WINDOW)&&(mods==0))
    {
      Scr.buttons2grab &= ~(1<<(button-1));
    }
  match_string(func_config,function,"bad mouse function:",fd);
  if(func == F_POPUP)
    {
      unsigned i,len;
      ptr = stripcpy2(tline);
      len = strlen(ptr);
      for (i = 0; i < PopupCount; i++)
	if (strncasecmp(PopupTable[i]->name,ptr,len) == 0)
	  {
	    mr = PopupTable[i];
	    break;
	  }
      if (!mr)
	{
	  fprintf(stderr,"Popup '%s' not defined\n",ptr);
	  func = F_NOP;
	}
    }
  else if(func == F_EXEC)
    {
      mi = (MenuItem *)safemalloc(sizeof(MenuItem));
      
      mi->next = (MenuItem *)NULL;
      mi->prev = (MenuItem *)NULL;
      mi->item_num = 0;
      mi->item = stripcpy2(tline);
      mi->action = stripcpy3(tline);
      mi->state = 0;
      mi->func = func;
      mi->strlen = strlen(mi->item);
      mi->val1 = 0;
      mi->val2 = 0;
    }
  
  temp = Scr.MouseButtonRoot;
  Scr.MouseButtonRoot = (MouseButton *)safemalloc(sizeof(MouseButton));
  Scr.MouseButtonRoot->func = func;
  Scr.MouseButtonRoot->menu = mr;
  Scr.MouseButtonRoot->item = mi;
  Scr.MouseButtonRoot->Button = button;
  Scr.MouseButtonRoot->Context = contexts;
  Scr.MouseButtonRoot->Modifier = mods;
  Scr.MouseButtonRoot->NextButton = temp;
  Scr.MouseButtonRoot->val1 = func_val_1;
  Scr.MouseButtonRoot->val2 = func_val_2;
  return;
}


/****************************************************************************
 * 
 *  Processes a line with a key binding
 *
 ****************************************************************************/ 
void ParseKeyEntry(char *tline, FILE *fd,char **junk)
{
  char context[256],modifiers[256],function[256],*ptr;
  char name[256];
  MenuRoot *mr;

  func_val_1 = 0;
  func_val_2 = 0;
  sscanf(tline,"%s %s %s %s %d %d",name,context,modifiers,function,
	 &func_val_1,&func_val_2);
  find_context(context,&contexts,win_contexts);
  find_context(modifiers,&mods,key_modifiers);
  match_string(func_config,function,"bad key function:",fd);

  if(func == F_EXEC)
    ptr = stripcpy3(tline);
  else if(func == F_POPUP)
    {
      unsigned i,len;
      ptr = stripcpy2(tline);
      if(ptr != (char *)0)
	{
	  len = strlen(ptr);
	  for (i = 0; i < PopupCount; i++)
	    if (strncasecmp(PopupTable[i]->name,ptr,len) == 0)
	      {
		mr = PopupTable[i];
		break;
	      }
	}
      if (!mr)
	{
	  fprintf(stderr,"Popup '%s' not defined\n",ptr);
	  func = F_NOP;
	}
    }

  AddFuncKey(name,contexts,mods,func,ptr,func_val_1,func_val_2,mr);
}

/****************************************************************************
 * 
 * Sets menu/keybinding/mousebinding function to specified value
 *
 ****************************************************************************/ 
void set_func(char *text, FILE *fd, int value)
{
  func = value;
}

/****************************************************************************
 * 
 * Turns a  string context of context or modifier values into an array of 
 * true/false values (bits)
 *
 ****************************************************************************/ 
void find_context(char *string, int *output, struct charstring *table)
{
  int i=0,j=0;
  Bool matched;

  *output=0;
  i=0;
  while(i<strlen(string))
    {
      j=0;
      matched = FALSE;
      while((!matched)&&(table[j].key != 0))
	{
	  if(tolower(string[i]) == tolower(table[j].key))
	    {
	      *output |= table[j].value;
	      matched = TRUE;
	    }
	  j++;
	}
      if(!matched)
	fprintf(stderr,"fvwm: bad charstring entry %c\n",string[i]);
      i++;
    }
  return;
}

/****************************************************************************
 * 
 * Matches text from config to a table of strings, calls routine
 * indicated in table.
 *
 ****************************************************************************/ 
void match_string(struct config *table, char *text, char *error_msg, FILE *fd)
{
  int j;
  Bool matched;

  j=0;
  matched = FALSE;
  while((!matched)&&(strlen(table[j].keyword)>0))
    {
      if(strncasecmp(text,table[j].keyword,strlen(table[j].keyword))==0)
	{
	  matched=TRUE;
	  /* found key word */
	  table[j].action(&text[strlen(table[j].keyword)],
				fd,table[j].arg);
	}
      else
	j++;
    }
  if(!matched)
    fprintf(stderr,"fvwm: %s %s\n",error_msg,text);
}

  

/****************************************************************************
 * 
 * Generates the window for a menu
 *
 ****************************************************************************/ 
void MakeMenu(MenuRoot *mr)
{
  MenuItem *cur;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  
  /* lets first size the window accordingly */
  mr->width += 10;
  
  for (cur = mr->first; cur != NULL; cur = cur->next)
    cur->x = (mr->width - XTextWidth(Scr.StdFont.font, cur->item,
				     cur->strlen))/2;
  mr->height = mr->items * Scr.EntryHeight;
   
  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask|CWCursor);
  attributes.background_pixel = Scr.StdColors.back;
  attributes.border_pixel = Scr.StdColors.fore;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FrameCursor;
  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) mr->width,
			 (unsigned int) mr->height, (unsigned int) 1,
			 CopyFromParent, (unsigned int) CopyFromParent,
			 (Visual *) CopyFromParent,
			 valuemask, &attributes);
  
  
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *	(MenuItem *)
 *
 *  Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *	func	- the numeric function
 *
 ***********************************************************************/
void AddToMenu(MenuRoot *menu, char *item, char *action,int func, 
	       int func_val_1,int func_val_2)
{
  MenuItem *tmp;
  int width;

  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  if (menu->first == NULL)
    {
      menu->first = tmp;
      tmp->prev = NULL;
    }
  else
    {
      menu->last->next = tmp;
      tmp->prev = menu->last;
    }
  menu->last = tmp;
  
  tmp->item = item;
  if(item != (char *)0)
    tmp->strlen = strlen(item);
  else
    tmp->strlen = 0;
  tmp->menu = 0;
  if(func == F_POPUP)
    {
      unsigned i,len;
      if(action != (char *)0)
	{
	  len = strlen(action);
	  for (i = 0; i < PopupCount; i++)
	    if (strncasecmp(PopupTable[i]->name,action,len) == 0)
	      {
		tmp->menu = PopupTable[i];
		break;
	      }
	}
      if(tmp->menu == (MenuRoot *)0)
	{
	  fprintf(stderr,"Popup '%s' not defined\n",action);
	  func = F_NOP;
	}
    }
  tmp->action = action;
  tmp->next = NULL;
  tmp->state = 0;
  tmp->func = func;
  tmp->val1 = func_val_1;
  tmp->val2 = func_val_2;

  width = XTextWidth(Scr.StdFont.font, item, tmp->strlen);
  if (width <= 0)
    width = 1;
  if(tmp->func == F_POPUP)
    width += 2*Scr.EntryHeight;
  if (width > menu->width)
    menu->width = width;
  
  tmp->item_num = menu->items++;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name)
{
  MenuRoot *tmp;
  
  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));
  tmp->name = name;
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->w = None;
  return (tmp);
}



/***********************************************************************
 *
 *  Procedure:
 *	AddFuncKey - add a function key to the list
 *
 *  Inputs:
 *	name	- the name of the key
 *	cont	- the context to look for the key press in
 *	mods	- modifier keys that need to be pressed
 *	func	- the function to perform
 *	action	- the action string associated with the function (if any)
 *
 ***********************************************************************/
void AddFuncKey (char *name, int cont, int mods, int func,  char *action,
		 int val1, int val2,MenuRoot *mr)
{
  FuncKey *tmp;
  KeySym keysym;
  KeyCode keycode;

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym(name)) == NoSymbol ||
      (keycode = XKeysymToKeycode(dpy, keysym)) == 0)
    return;
  
  tmp = (FuncKey *) safemalloc(sizeof(FuncKey));
  tmp->next = Scr.FuncKeyRoot.next;
  Scr.FuncKeyRoot.next = tmp;
  
  tmp->name = name;
  tmp->keycode = keycode;
  tmp->cont = cont;
  tmp->mods = mods;
  tmp->func = func;
  tmp->action = action;
  tmp->val1 = val1;
  tmp->val2 = val2;
  tmp->menu = mr;
  return;
}

/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/ 
Pixel GetColor(char *name)
{
  XColor color, junkcolor;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  if(!XAllocNamedColor (dpy, attributes.colormap, name, &color, &junkcolor))
    fprintf(stderr,"fvwm: can't alloc color %s\n",name);
  
  return color.pixel;
}



/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/ 
char *stripcpy(char *source)
{
  char *tmp,*ptr;
  int len;

  while(isspace(*source))
    source++;
  len = strlen(source);
  tmp = source + len -1;
  while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=source))
    {
      tmp--;
      len--;
    }
  ptr = safemalloc(len+1);
  strncpy(ptr,source,len);
  ptr[len]=0;
  return ptr;
}
  


/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips all data before the first quote and after the second
 *
 ****************************************************************************/
char *stripcpy2(char *source)
{
  char *ptr;
  int count;
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source == 0)
    {
      fprintf(stderr,"fvwm: 2: bad mouse binding\n");
      return 0;
    }
  source++;
  ptr = source;
  count = 0;
  while((*ptr!='"')&&(*ptr != 0))
    {
      ptr++;  
      count++;
    }
  ptr = safemalloc(count+1);
  strncpy(ptr,source,count);
  ptr[count]=0;
  return ptr;
}


/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips all data before the second quote. and strips trailing spaces and
 * new lines
 *
 ****************************************************************************/
char *stripcpy3(char *source)
{
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source != 0)
    source++;
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source == 0)
    {
      fprintf(stderr,"fvwm: 3: bad mouse binding\n");
      return 0;
    }
  source++;
  return stripcpy(source);
}
  


/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void CreateGCs()
{
  XGCValues gcv;
  unsigned long gcm;
  
  /* create GC's */
  gcm = GCFunction|GCLineWidth|GCForeground|GCSubwindowMode; 
  gcv.function = GXxor;
  gcv.line_width = 0;
  gcv.foreground = 1;
  gcv.subwindow_mode = IncludeInferiors;
  Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.foreground = Scr.StdColors.fore;
  gcv.background = Scr.StdColors.back;
  gcv.font =  Scr.StdFont.font->fid;
  /*
   * Prevent GraphicsExpose and NoExpose events.  We'd only get NoExpose
   * events anyway;  they cause BadWindow errors from XGetWindowAttributes
   * call in FindScreenInfo (events.c) (since drawable is a pixmap).
   */
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  
  Scr.NormalGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  /* reverse video GC for menus */
  gcv.background = Scr.StdColors.fore;
  gcv.foreground = Scr.StdColors.back;
  Scr.InvNormalGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  /* Hilite GC for selected window border */
  gcv.foreground = Scr.HiColors.fore;
  gcv.background = Scr.HiColors.back;
  Scr.HiliteGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  /* Stippled GCs for mono screens */
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont|GCFillStyle|GCStipple;
  gcv.stipple = Scr.gray;	  
  gcv.foreground = Scr.HiColors.back;
  gcv.background = Scr.HiColors.fore;
  if(Scr.d_depth < 2)
    gcv.fill_style = FillOpaqueStippled;  
  else
    gcv.fill_style = FillSolid;
  Scr.HiStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.stipple = Scr.light_gray;	  
  if(Scr.d_depth < 2)
    {
      gcv.foreground = Scr.StdColors.fore;
      gcv.background = Scr.StdColors.back;
      gcv.fill_style = FillOpaqueStippled;  
    }
  else
    {
      gcv.foreground = Scr.StdColors.back;
      gcv.background = Scr.StdColors.fore;
      gcv.fill_style = FillOpaqueStippled;  
      gcv.fill_style = FillSolid;
    }
  Scr.NormalStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont;
  gcv.foreground = Scr.HiRelief.fore;
  gcv.background = Scr.HiRelief.back;
  gcv.fill_style = FillSolid;
  Scr.HiReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.HiRelief.back;
  gcv.background = Scr.HiRelief.fore;
  Scr.HiShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.StdRelief.fore;
  gcv.background = Scr.StdRelief.back;
  Scr.StdReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.StdRelief.back;
  gcv.background = Scr.StdRelief.fore;
  Scr.StdShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  
}

/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a 
 *		     problem
 *
 ***********************************************************************/
char *safemalloc(int length)
{
  char *ptr;
  
  ptr = malloc(length);
  if(ptr == (char *)0)
    {
      fprintf(stderr,"fvwm: Can't malloc temp space\n");
      exit(1);
    }
  return ptr;
}


/***********************************************************************
 *
 *  Procedure:
 *	AddToList - add a window name to the no title list
 *
 *  Inputs:
 *	name	- a pointer to the name of the window 
 *
 ***********************************************************************/
void AddToList(char *text, FILE *fd, name_list **list)
{
  name_list *nptr;
  char *name;
  
  name = stripcpy(text);
  nptr = (name_list *)safemalloc(sizeof(name_list));
  nptr->next = *list;
  nptr->name = name;
  nptr->value = (char *)0;
  *list = nptr;
}    

void AddToList2(char *text, FILE *fd, name_list **list)
{
  name_list *nptr;
  char *name;
  char *value;

  name = stripcpy2(text);
  value = stripcpy3(text);
  nptr = (name_list *)safemalloc(sizeof(name_list));
  nptr->next = *list;
  nptr->name = name;
  nptr->value = value;
  *list = nptr;
}    

/***********************************************************************
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 ***********************************************************************/
char *pager_name = "Fvwm Pager";
XSizeHints sizehints = {
  PMinSize | PResizeInc | PBaseSize | PWinGravity,
  0, 0, 100, 100,	                /* x, y, width and height */
  1, 1,		                        /* Min width and height */
  0, 0,		                        /* Max width and height */
  1, 1,	                         	/* Width and height increments */
  0, 0, 0, 0,	                        /* Aspect ratio - not used */
  1, 1,                 		/* base size */
  NorthWestGravity                      /* gravity */
};

void initialize_pager(int x, int y)
{
  XTextProperty name;
  int width,height,window_x,window_y;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  FILE *fd = 0;

  width = (Scr.VxMax + Scr.MyDisplayWidth)/Scr.VScale;
  height = (Scr.VyMax + Scr.MyDisplayHeight)/Scr.VScale;
  
  if(x >=0)
    window_x = x;
  else
    {
      sizehints.win_gravity = NorthEastGravity;
      window_x = Scr.MyDisplayWidth - width + x;
    }

  if(y >=0)
    window_y = y;
  else
    {
      window_y = Scr.MyDisplayHeight - height + y;
      if(x<0)
	sizehints.win_gravity = SouthEastGravity;
      else
	sizehints.win_gravity = SouthWestGravity;
    }
  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask|CWCursor);
  attributes.background_pixel = Scr.StdColors.back;
  attributes.border_pixel = Scr.StdColors.fore;
  attributes.cursor = Scr.FrameCursor;
  attributes.event_mask = (ExposureMask | EnterWindowMask|ButtonReleaseMask);
  sizehints.width = width;
  sizehints.height = height;
  sizehints.x = window_x;
  sizehints.y = window_y;

  Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, width,
			    height, (unsigned int) 1,
			    CopyFromParent, (unsigned int) CopyFromParent,
			    (Visual *) CopyFromParent,
			    valuemask, &attributes);
  XSetWMNormalHints(dpy,Pager_w,&sizehints);
  XStringListToTextProperty(&pager_name,1,&name);
  XSetWMName(dpy,Pager_w,&name);
  XSetWMIconName(dpy,Pager_w,&name);
  XFree(name.value);

  XMapRaised(dpy, Pager_w);
  AddToList(pager_name, fd, &Scr.Sticky);  
}


