#
# Makefile for rogue
#
# Advanced Rogue
# Copyright (C) 1984, 1985, 1986 Michael Morgan, Ken Dalka and AT&T
# All rights reserved.
#
# Based on "Rogue: Exploring the Dungeons of Doom"
# Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#
# See the file LICENSE.TXT for full copyright and licensing information.
#

#
# Makefile for rogue
#


DISTNAME=arogue7.7.1
PROGRAM=arogue77

O=o

HDRS=	rogue.h mach_dep.h network.h

OBJS1 =	vers.$(O) actions.$(O) chase.$(O) command.$(O) daemon.$(O) \
	daemons.$(O) eat.$(O) effects.$(O) encumb.$(O) fight.$(O) init.$(O) \
        io.$(O) list.$(O) main.$(O) maze.$(O) mdport.$(O) misc.$(O) \
        monsters.$(O)
OBJS2 = move.$(O) new_level.$(O) options.$(O) outside.$(O) pack.$(O) \
        passages.$(O) player.$(O) potions.$(O) rings.$(O) rip.$(O) rogue.$(O) \
        rooms.$(O) save.$(O) scrolls.$(O) state.$(O) sticks.$(O) things.$(O) \
        trader.$(O) util.$(O) weapons.$(O) wear.$(O) wizard.$(O) xcrypt.$(O)
OBJS  =	$(OBJS1) $(OBJS2)

CFILES=	vers.c actions.c chase.c command.c daemon.c \
	daemons.c eat.c effects.c encumb.c fight.c init.c \
        io.c list.c main.c maze.c mdport.c misc.c monsters.c \
        move.c new_level.c options.c outside.c pack.c \
	passages.c player.c potions.c rings.c rip.c rogue.c \
	rooms.c save.c scrolls.c state.c sticks.c things.c \
	trader.c util.c weapons.c wear.c wizard.c xcrypt.c
MISC_C=	
DOCSRC= aguide.mm
DOCS  = $(PROGRAM).doc $(PROGRAM).html
MISC  =	Makefile $(MISC_C) LICENSE.TXT $(PROGRAM).sln $(PROGRAM).vcproj $(DOCS)\
        $(DOCSRC)

CC    = gcc
ROPTS =
COPTS = -O3
CFLAGS= $(COPTS) $(ROPTS) 
LIBS =	-lcurses
RM    = rm -f

.SUFFIXES: .obj

.c.obj:
	$(CC) $(CFLAGS) /c $*.c
    
$(PROGRAM): $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
    
clean:
	$(RM) $(OBJS1)
	$(RM) $(OBJS2)
	$(RM) core a.exe a.out a.exe.stackdump $(PROGRAM) $(PROGRAM).exe $(PROGRAM).lck
	$(RM) $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip 
    
dist.src:
	make clean
	tar cf $(DISTNAME)-src.tar $(CFILES) $(HDRS) $(MISC)
	gzip -f $(DISTNAME)-src.tar

doc.nroff:
	tbl aguide.mm | nroff -mm | colcrt - > arogue77.doc

doc.groff:
	groff -P-c -t -mm -Tascii aguide.mm | sed -e 's/.\x08//g' > arogue77.doc
	groff -t -mm -Thtml aguide.mm > arogue77.ht

dist.irix:
	make clean
	make CC=cc COPTS="-woff 1116 -O3" $(PROGRAM)
	tar cf $(DISTNAME)-irix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-irix.tar

dist.aix:
	make clean
	make CC=xlc COPTS="-qmaxmem=16768 -O3 -qstrict" $(PROGRAM)
	tar cf $(DISTNAME)-aix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-aix.tar

dist.linux:
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-linux.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-linux.tar
	
dist.interix:
	@$(MAKE) clean
	@$(MAKE) COPTS="-ansi" $(PROGRAM)
	tar cf $(DISTNAME)-interix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-interix.tar
	
dist.cygwin:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory $(PROGRAM)
	tar cf $(DISTNAME)-cygwin.tar $(PROGRAM).exe LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-cygwin.tar

#
# Use MINGW32-MAKE to build this target
#
dist.mingw32:
	@$(MAKE) --no-print-directory RM="cmd /c del" clean
	@$(MAKE) --no-print-directory LIBS="-lpdcurses" $(PROGRAM)
	cmd /c del $(DISTNAME)-mingw32.zip
	zip $(DISTNAME)-mingw32.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)
	
dist.msys:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory LIBS="-lcurses" $(PROGRAM)
	tar cf $(DISTNAME)-msys.tar $(PROGRAM).exe LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-msys.tar
	
dist.djgpp:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory LDFLAGS="-L$(DJDIR)/LIB" \
	LIBS="-lpdcurses" $(PROGRAM)
	rm -f $(DISTNAME)-djgpp.zip
	zip $(DISTNAME)-djgpp.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)

#
# Use NMAKE to build this targer
#
dist.win32:
	@$(MAKE) /NOLOGO O="obj" RM="-del" clean
	@$(MAKE) /NOLOGO O="obj" CC="CL" \
	    LIBS="..\pdcurses\pdcurses.lib shfolder.lib user32.lib Advapi32.lib" \
	    COPTS="-nologo -I..\pdcurses \
	    -Ox -wd4033 -wd4716" $(PROGRAM)
	-del $(DISTNAME)-win32.zip
	zip $(DISTNAME)-win32.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)


actions.o:	rogue.h
chase.o:	rogue.h
command.o:	rogue.h
command.o:	mach_dep.h
daemon.o:	rogue.h
daemons.o:	rogue.h
eat.o:		rogue.h
edit.o:		mach_dep.h
edit.o:		rogue.h
effects.o:	rogue.h
encumb.o:	rogue.h
fight.o:	rogue.h
init.o:		rogue.h
init.o:		mach_dep.h
io.o:		rogue.h
list.o:		rogue.h
main.o:		mach_dep.h
main.o:		network.h
main.o:		rogue.h
maze.o:		rogue.h
misc.o:		rogue.h
monsters.o:	rogue.h
move.o:		rogue.h
new_level.o:	rogue.h
options.o:	rogue.h
outside.o:	rogue.h
pack.o:		rogue.h
passages.o:	rogue.h
player.o:	rogue.h
potions.o:	rogue.h
rings.o:	rogue.h
rip.o:		mach_dep.h
rip.o:		network.h
rip.o:		rogue.h
rogue.o:	rogue.h
rooms.o:	rogue.h
save.o:		rogue.h
save.o:		mach_dep.h
scrolls.o:	rogue.h
sticks.o:	rogue.h
things.o:	rogue.h
trader.o:	rogue.h
util.o:		rogue.h
weapons.o:	rogue.h
wear.o:		rogue.h
wizard.o:	rogue.h
