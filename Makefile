CC = gcc

MANDIR=/usr/man/man1
BINDIR=/usr/bin/X11
USRLIBDIR=/usr/lib/X11
INCROOT=/usr/include

# for most systems
LIBS= -L$(USRLIBDIR) -lX11 
#for solaris 2
#LIBS= -lX11 -lsocket

INITDIR=$(USRLIBDIR)/fvwm
FFLAGS=-DFVWMRC=\"$(INITDIR)/system.fvwmrc\"

CFLAGS = -O2 -s -I$(INCROOT) $(FFLAGS)

SRCS = fvwm.c configure.c events.c borders.c menus.c functions.c resize.c \
        add_window.c pager.c move.c icons.c
OBJS = fvwm.o configure.o events.o borders.o menus.o functions.o resize.o \
	add_window.o pager.o move.o icons.o

fvwm: $(OBJS)
	$(CC) $(CFLAGS) -o fvwm $(OBJS) $(LIBS) 

install: $(BINDIR)/fvwm $(MANDIR)/fvwm.1 $(INITDIR)/system.fvwmrc
	@echo You may want to copy system.fvwmrc to each users ~/.fwmrc
	@echo

	@echo Also, remember to change the last line of $(USRLIBDIR)/xinit/xinitrc
	@echo  from \"twm\" or \"exec twm\" echo to \"exec fvwm\"
$(MANDIR)/fvwm.1: fvwm.1
	install -c -m 644 $? $@

$(BINDIR)/fvwm: fvwm
	install -c -m 755 $? $@

$(INITDIR)/system.fvwmrc: $(INITDIR) system.fvwmrc
	install -c -m 644 system.fvwmrc $@

$(INITDIR):
	mkdir $(INITDIR)

clean:
	rm -f $(OBJS)
	rm -f fvwm
	rm -f *~

add_window.o: add_window.c fvwm.h screen.h misc.h menus.h
events.o:     events.c     fvwm.h screen.h misc.h menus.h parse.h
menus.o:      menus.c      fvwm.h screen.h misc.h menus.h parse.h 
resize.o:     resize.c     fvwm.h screen.h misc.h menus.h
fvwm.o:       fvwm.c       fvwm.h screen.h misc.h menus.h
configure.o:  configure.c  fvwm.h screen.h misc.h menus.h
functions.o:  functions.c  fvwm.h screen.h misc.h menus.h parse.h 
move.o:       move.c       fvwm.h screen.h misc.h menus.h parse.h 
borders.o:    borders.c    fvwm.h screen.h misc.h menus.h parse.h 
pager.o:      pager.c      fvwm.h screen.h misc.h menus.h parse.h 
icons.o:      icons.c      fvwm.h screen.h misc.h menus.h parse.h 
