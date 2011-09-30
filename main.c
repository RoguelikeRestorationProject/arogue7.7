/*
 * main.c  -  setup code
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985, 1986 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * Based on "Rogue: Exploring the Dungeons of Doom"
 * Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "curses.h"
#include <signal.h>
#ifdef BSD
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "mach_dep.h"
#include "network.h"
#include "rogue.h"
#ifdef PC7300
#include "sys/window.h"
#include <ctype.h>
extern struct uwdata wdata, oldwin;
extern char oldtext[WTXTNUM][WTXTLEN];
#endif

main(argc, argv, envp)
char **argv;
char **envp;
{
    register char *env;
    int lowtime;
    time_t now;
#ifdef PC7300
    int hardwindow;	/* Do we have a hardware window? */
#endif

    md_init();

    /*
     * get home and options from environment
     */

    strncpy(home, md_gethomedir(), LINELEN);

    /* Get default save file */
    strcpy(file_name, home);
    strcat(file_name, "arogue77.sav");

    /* Get default score file */
    strcpy(score_file, md_getroguedir());

    if (*score_file)
        strcat(score_file,"/");

    strcat(score_file,"arogue77.scr");

    if ((env = getenv("ROGUEOPTS")) != NULL)
	parse_opts(env);

    if (whoami[0] == '\0')
        strucpy(whoami, md_getusername(), strlen(md_getusername()));

    /*
     * check for print-score option
     */
    if (argc == 2 && strcmp(argv[1], "-s") == 0)
    {
	waswizard = TRUE;
	score(0, SCOREIT, 0);
	exit(0);
    }

#ifdef NUMNET
    /*
     * Check for a network update
     */
    if (argc == 2 && strcmp(argv[1], "-u") == 0) {
	unsigned long netread();
	int errcheck, errors = 0;
	unsigned long amount;
	short monster;

	/* Read in the amount and monster values to pass to score */
	amount = netread(&errcheck, sizeof(unsigned long), stdin);
	if (errcheck) errors++;

	monster = (short) netread(&errcheck, sizeof(short), stdin);
	if (errcheck) errors++;

	/* Now do the update if there were no errors */
	if (errors) exit(1);
	else {
	    score(amount, UPDATE, monster);
	    exit(0);
	}
    }
#endif

#ifdef WIZARD
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, md_crypt(md_getpass("Wizard's password: "), "mT")) == 0)
	{
	    wizard = TRUE;
	    argv++;
	    argc--;
	}
#endif

    if (!wizard && !author() && !holiday()) {
	printf("Sorry, %s, but you can't play during working hours.\n", whoami);
	printf("Try again later.\n");
	exit(1);
    }
    if (!wizard && !author() && too_much()) {
	printf("Sorry, %s, but the system is too loaded now.\n", whoami);
	printf("Try again later.\n");
	exit(1);
    }

#if NICE
    if (!wizard)
        nice(19);	/* nice the max amount */
#endif

    if (argc == 2)
	if (!restore(argv[1], envp)) /* Note: restore will never return */
	    exit(1);
    lowtime = (int) time(&now);
    dnum = (wizard && getenv("SEED") != NULL ?
	atoi(getenv("SEED")) :
	lowtime + getpid());
    if (wizard)
	printf("Hello %s, welcome to dungeon #%d", whoami, dnum);
    else
	printf("Hello %s, just a moment while I dig the dungeon...", whoami);
    fflush(stdout);
    seed = dnum;
    md_srand(seed);

#ifdef PC7300
    /* Store static window parameters */
    hardwindow = ioctl(0, WIOCGETD, &wdata);
    if (hardwindow >= 0) {	/* We have a hardware window */
	extern char **environ;

	/* Make sure our window is the right size */
	oldwin = wdata;
	if ((wdata.uw_height / wdata.uw_vs) < 23 ||
	    (wdata.uw_width / wdata.uw_hs) < 75) {
	    wdata.uw_width = 80 * wdata.uw_hs;
	    wdata.uw_height = 24 * wdata.uw_vs;
	    wdata.uw_x = 0;
	    wdata.uw_y = wdata.uw_vs;
	    wdata.uw_uflags = NBORDER;

	    /* Make the change */
	    if (ioctl(1, WIOCSETD, &wdata) >= 0 && environ) {
		char **eptr, *tptr, *nptr, *newenv, *lptr = 0, *cptr = 0;
		int i, nlines = -1, ncols = -1, nlindig = 0, ncoldig = 0;
		struct utdata labelbuf;

		/* Save and change window-associated text */
		for (i=0; i<WTXTNUM; i++) {
		    labelbuf.ut_num = i;
		    ioctl(1, WIOCGETTEXT, &labelbuf);
		    strncpy(oldtext[i], labelbuf.ut_text, WTXTLEN - 1);
		    if (*labelbuf.ut_text) {
			*labelbuf.ut_text = '\0';
			ioctl(1, WIOCSETTEXT, &labelbuf);
		    }
		}

		labelbuf.ut_num = WTXTLABEL;
		strcpy(labelbuf.ut_text, "Advanced Rogue");
		ioctl(1, WIOCSETTEXT, &labelbuf);

		/* We have to change the TERMCAP entry */
		eptr = environ;
		while (*eptr) {
		    if (strncmp(*eptr, "TERMCAP=", 8) == 0) break;
		    else eptr++;
		}

		/* We found a TERMCAP entry */
		if (*eptr) {
		    /* Search for li# and co# */
		    tptr = *eptr;
		    while (*tptr) {
			switch (*tptr) {
			    case 'l':
				if (nlines == -1 &&
				    strncmp(tptr, "li#", 3) == 0) {
				    tptr += 3;
				    lptr = tptr;
				    lines = atoi(tptr);
				    while (isdigit(*tptr)) {
					nlindig++;;
					tptr++;
				    }
				}
				else tptr++;
				break;
			    case 'c':
				if (ncols == -1 &&
				    strncmp(tptr, "co#", 3) == 0) {
				    tptr += 3;
				    cptr = tptr;
				    cols = atoi(tptr);
				    while (isdigit(*tptr)) {
					ncoldig++;
					tptr++;
				    }
				}
				else tptr++;
				break;
			    default:
				tptr++;
			}
		    }

		    /* Change the entry */
		    if (ncoldig != 2 || nlindig != 2) {
			int length;

			/* Add in difference in num lengths plus NULL */
			length = strlen(*eptr) - ncoldig - nlindig + 5;

			if (ncoldig == 0) length += 4; /* For :co# */
			if (nlindig == 0) length += 4; /* For :li# */

			newenv = malloc(length);
			tptr = *eptr;
			nptr = newenv;

			if (nlindig == 0 || ncoldig == 0) {
			    /* Copy up to the first : */
			    while (*tptr && *tptr != ':') *nptr++ = *tptr++;

			    /* Do we have to add a field? */
			    if (nlindig == 0) {
				strcpy(nptr, ":li#24");
				nptr += 6;
			    }
			    if (ncoldig == 0) {
				strcpy(nptr, ":co#80");
				nptr += 6;
			    }
			}
			while (*tptr) {
			    if (tptr == lptr) {
				strcpy(nptr, "24");
				nptr += 2;
				tptr += nlindig;
			    }
			    else if (tptr == cptr) {
				strcpy(nptr, "80");
				nptr += 2;
				tptr += ncoldig;
			    }
			    else *nptr++ = *tptr++;
			}

			*nptr = '\0';

			/* Replace the old one */
			free(*eptr);
			*eptr = newenv;
		    }
		    else {
			/* Just overwrite the old numbers */
			*lptr++ = '2';
			*lptr = '4';
			*cptr++ = '8';
			*cptr = '0';
		    }
		}
	    }
	}
    }
#endif
    init_things();			/* Set up probabilities of things */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    initscr();				/* Start up cursor package */
    init_names();			/* Set up names of scrolls */
    init_misc();			/* Set up miscellaneous magic */
    init_foods();			/* set up the food table */

    cols = COLS;
    lines = LINES;
    if (cols > 85) cols = 85;
    if (lines > 24) lines = 24;
    if (lines < 23 || cols < 75) { /* give player a break if larger font used */
	printf("\nERROR: screen size too small for rogue\n");
	byebye(0);
    }

    /*
     * Now that we have cols and lines, we can update our window
     * structure for non-hardware windows.
     */
#ifdef PC7300
    if (hardwindow < 0) {
	wdata.uw_x = 0;
	wdata.uw_y = 0;
	wdata.uw_width = COLS;
	wdata.uw_height = LINES;
	wdata.uw_uflags = 0;
	wdata.uw_hs = 1;
	wdata.uw_vs = 1;
	wdata.uw_baseline = 0;
    }
#endif
    setup();
    /*
     * Set up windows
     */
    cw = newwin(lines, cols, 0, 0);
    mw = newwin(lines, cols, 0, 0);
    hw = newwin(lines, cols, 0, 0);
    msgw = newwin(4, cols, 0, 0);
    keypad(cw,TRUE);
    keypad(msgw,TRUE);

    init_player();			/* Roll up the rogue */
    waswizard = wizard;

#ifdef WIZARD
    /* A super wizard doesn't have to get equipped */
    if (wizard && strcmp(getenv("SUPER"),"YES") == 0) {
	level = 1;
	new_level(NORMLEV);
    }
    else
#endif
    new_level(STARTLEV);		/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    daemon(doctor, &player, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    if (player.t_ctype == C_MAGICIAN || player.t_ctype == C_RANGER)
	    fuse(spell_recovery, 0, SPELLTIME, AFTER);
    if (player.t_ctype == C_DRUID || player.t_ctype == C_RANGER)
	    fuse(chant_recovery, 0, SPELLTIME, AFTER);
    if (player.t_ctype == C_CLERIC || player.t_ctype == C_PALADIN)
	    fuse(prayer_recovery, 0, SPELLTIME, AFTER);
    daemon(stomach, 0, AFTER);
    if (player.t_ctype == C_THIEF	||
	player.t_ctype == C_ASSASIN	||
	player.t_ctype == C_MONK)
	    daemon(trap_look, 0, AFTER);

    /* Does this character have any special knowledge? */
    switch (player.t_ctype) {
	case C_ASSASIN:
	    /* Assassins automatically recognize poison */
	    p_know[P_POISON] = TRUE;
    }

    /* Choose a quest item */
    quest_item = rnd(MAXRELIC);
    draw(cw);
    msg("You have been quested to retrieve the %s....",
	rel_magic[quest_item].mi_name);
    mpos = 0;
    playit();
}

/*
 * endit:
 *	Exit the program abnormally.
 */

void
endit(sig)
int sig;
{
    fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */

fatal(s)
char *s;
{
    clear();
    move(lines-2, 0);
    printw("%s", s);
    draw(stdscr);
    endwin();
#ifdef PC7300
    endhardwin();
#endif
    printf("\n");	/* So the curser doesn't stop at the end of the line */
    exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
rnd(range)
register int range;
{
    return(range <= 0 ? 0 : md_rand() % range);
}

/*
 * roll:
 *	roll a number of dice
 */

roll(number, sides)
register int number, sides;
{
    register int dtotal = 0;

    while(number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}
# ifdef SIGTSTP
/*
 * handle stop and start signals
 */
void
tstp(sig)
int sig;
{
    mvcur(0, cols - 1, lines - 1, 0);
    endwin();
    fflush(stdout);
    kill(0, SIGTSTP);
    signal(SIGTSTP, tstp);
    raw();
    noecho();
    keypad(cw,1);
    keypad(msgw,1);
    clearok(curscr, TRUE);
    touchwin(cw);
    draw(cw);
    md_flushinp();
}
# endif

setup()
{
#ifdef CHECKTIME
    int  checkout();

    if (!author()) {
	signal(SIGALRM, checkout);
	alarm(CHECKTIME * 60);
    }
#endif
/*
#ifndef DUMP
    signal(SIGILL, bugkill);
#ifdef SIGTRAP
    signal(SIGTRAP, bugkill);
#endif
#ifdef SIGIOT
    signal(SIGIOT, bugkill);
#endif
#ifdef SIGEMT
    signal(SIGEMT, bugkill);
#endif
    signal(SIGFPE, bugkill);
#ifdef SIGBUS
    signal(SIGBUS, bugkill);
#endif
    signal(SIGSEGV, bugkill);
#ifdef SIGSYS
    signal(SIGSYS, bugkill);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, bugkill);
#endif
#endif
*/
#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif

#ifdef SIGHUP
    signal(SIGHUP, auto_save);
#endif
    signal(SIGTERM, auto_save);
    signal(SIGINT, quit);
#ifdef SIGQUIT
    signal(SIGQUIT, endit);
#endif
    raw();				/* Cbreak mode */
    noecho();				/* Echo off */
}

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 * refreshing things and looking at the proper times.
 */

playit()
{
    register char *opts;


    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
	parse_opts(opts);


    player.t_oldpos = hero;
    oldrp = roomin(&hero);
    after = TRUE;
    command();			/* Command execution */
    endit(0);
}

/*
 * see if the system is being used too much for this game
 */
too_much()
{
#if MAXPROCESSES
	if (loadav() > MAXPROCESSES)
		return(TRUE);
#endif
#if MAXUSERS
	if (ucount() > MAXUSERS)
		return(TRUE);
#endif
	return(FALSE);
}

/*
 * author:
 *	See if a user is an author of the program
 */
author()
{
	switch (md_getuid()) {
#if AUTHOR
		case AUTHOR:
#endif
		case 0: /* always OK for root to play */
			return TRUE;
		default:
			return FALSE;
	}
}


#if CHECKTIME
static int num_checks = 0;	/* times we've gone over in checkout() */

checkout()
{
	static char *msgs[] = {
	"The system is too loaded for games. Please leave in %d minutes",
	"Please save your game.  You have %d minutes",
	"This is your last chance. You had better leave in %d minutes",
	};
	int checktime;

	signal(SIGALRM, checkout);
	if (!holiday() && !author()) {
	    msg("Game time is over. Your game is being saved.\n\n");
	    auto_save();		/* NO RETURN */
	}
	if (too_much())	{
	    if (num_checks >= 3)
		fatal("You didn't listen, so now you are DEAD !!\n");
	    checktime = CHECKTIME / (num_checks + 1);
	    chmsg(msgs[num_checks++], checktime);
	    alarm(checktime * 60);
	}
	else {
	    if (num_checks) {
		chmsg("The load has dropped. You have a reprieve.");
		num_checks = 0;
	    }
	    alarm(CHECKTIME * 60);
	}
}

/*
 * checkout()'s version of msg.  If we are in the middle of a shell, do a
 * printf instead of a msg to avoid the refresh.
 */
chmsg(fmt, arg)
char *fmt;
int arg;
{
	if (in_shell) {
		printf(fmt, arg);
		putchar('\n');
		fflush(stdout);
	}
	else
		msg(fmt, arg);
}
#endif

#ifdef MAXPROCESSES

#include <fcntl.h>

loadav()
{
	char *sarcmd = "sar -v | cut -c17-20 | tail -2";
	char *gettycmd = "grep getty /etc/inittab | wc -l";
	char sysbuffer[BUFSIZ];
	char tempfile[50];
	char inbuf[BUFSIZ];
	int fd, nprocess, ngetty;

	sprintf(tempfile, "/tmp/rg%d", getpid());
	sprintf(sysbuffer, "%s > %s", sarcmd, tempfile);
	if (system(sysbuffer) != 0) {
	    debug ("system() call failed");
	    return (MAXPROCESSES - 1);
	}
	if ((fd = open(tempfile, O_RDONLY)) == -1) {
	    debug ("open() call failed");
	    return (MAXPROCESSES - 1);
	}
	if (read(fd, inbuf, BUFSIZ) == -1) {
	    debug ("read() call failed");
	    return (MAXPROCESSES - 1);
	}
	close(fd);
	sscanf(inbuf, "%d", &nprocess);
	sprintf(sysbuffer, "%s > %s", gettycmd, tempfile);
	if (system(sysbuffer) != 0) {
	    debug ("system() call failed");
	    return (MAXPROCESSES - 1);
	}
	if ((fd = open(tempfile, O_RDONLY)) == -1) {
	    debug ("open() call failed");
	    return (MAXPROCESSES - 1);
	}
	if (read(fd, inbuf, BUFSIZ) == -1) {
	    debug ("read() call failed");
	    return (MAXPROCESSES - 1);
	}
	close(fd);
	sscanf(inbuf, "%d", &ngetty);
	unlink(tempfile);
	return(nprocess - ngetty);
}
#endif

#ifdef MAXUSERS
/*
 * ucount:
 *	Count the number of people on the system
 */
#include <sys/types.h>
#include <utmp.h>
struct utmp buf;
ucount()
{
	reg struct utmp *up;
	reg FILE *utmp;
	reg int count;

	if ((utmp = fopen(UTMP, "r")) == NULL)
	    return 0;

	up = &buf;
	count = 0;
	while (fread(up, 1, sizeof (*up), utmp) > 0)
#ifdef BSD
		if (buf.ut_line[0] == 't')	/* On a tty */
#else
		if (buf.ut_type == USER_PROCESS)
#endif
			count++;
	fclose(utmp);
	return count;
}
#endif

/*
 * holiday:
 *	Returns TRUE when it is a good time to play rogue
 */
holiday()
{
#ifdef CHECKTIME
	long now;
	struct tm *localtime();
	reg struct tm *ntime;


	time(&now);			/* get the current time */
	ntime = localtime(&now);
	if(ntime->tm_wday == 0 || ntime->tm_wday == 6)
		return TRUE;		/* OK on Sat & Sun */
	if(ntime->tm_hour < 8 || ntime->tm_hour >= 18)
		return TRUE;		/* OK before 8AM & after 6PM */
	if(ntime->tm_yday <= 7 || ntime->tm_yday >= 350)
		return TRUE;		/* OK during Christmas */
	return FALSE;			/* All other times are bad */
#else
	return TRUE;
#endif
}
