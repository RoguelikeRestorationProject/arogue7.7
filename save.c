/*
 * save.c - save and restore routines
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

/*
 * save and restore routines
 *
 */

#include "curses.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "rogue.h"
#include <fcntl.h>
#include <errno.h>
#include "mach_dep.h"
#ifdef PC7300
#include "sys/window.h"
extern struct uwdata wdata;
#endif

#if u370 || uts
#define ENCREAD(b,n,fd) read(fd,b,n)
#define ENCWRITE(b,n,fd) write(fd,b,n)
#endif
#ifndef ENCREAD
#define ENCREAD encread
#define ENCWRITE encwrite
#endif

typedef struct stat STAT;

extern char version[], encstr[];
/* extern bool _endwin; */
extern int errno;

STAT sbuf;

bool
save_game()
{
    register int savefd;
    register int c;
    char buf[LINELEN];

    /*
     * get file name
     */
    mpos = 0;
    if (file_name[0] != '\0')
    {
	msg("Save file (%s)? ", file_name);
	do
	{
	    c = readchar();
	    if (c == ESCAPE) return(0);
	} while (c != 'n' && c != 'N' && c != 'y' && c != 'Y');
	mpos = 0;
	if (c == 'y' || c == 'Y')
	{
	    msg("File name: %s", file_name);
	    goto gotfile;
	}
    }

    do
    {
	msg("File name: ");
	mpos = 0;
	buf[0] = '\0';
	if (get_str(buf, msgw) == QUIT)
	{
	    msg("");
	    return FALSE;
	}
	strcpy(file_name, buf);
gotfile:
	if ((savefd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC,0666)) < 0)
	    msg(strerror(errno));	/* fake perror() */
    } while (savefd < 0);

    /*
     * write out encrpyted file (after a stat)
     */
    if (save_file(savefd) == FALSE) {
	msg("Cannot create save file.");
	unlink(file_name);
	return(FALSE);
    }
    else return(TRUE);
}

/*
 * automatically save a file.  This is used if a HUP signal is
 * recieved
 */
void
auto_save(sig)
int sig;
{
    register int savefd;
    register int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (file_name[0] != '\0'	&& 
	pstats.s_hpt > 0	&&
	(savefd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0600)) >= 0)
	save_file(savefd);
    endwin();
#ifdef PC7300
    endhardwin();
#endif
    exit(1);
}

/*
 * write the saved game on the file
 */
bool
save_file(savefd)
register int savefd;
{
    register unsigned num_to_write, num_written;
    FILE *savef;
    int ret;

    wmove(cw, lines-1, 0);
    draw(cw);
    lseek(savefd, 0L, 0);
    fstat(savefd, &sbuf);
    num_to_write = strlen(version) + 1;
    num_written = ENCWRITE(version, num_to_write, savefd);
    sprintf(prbuf,"%d x %d\n", LINES, COLS);
    ENCWRITE(prbuf,80,savefd);
    savef = (FILE *) fdopen(savefd,"wb");
    ret = rs_save_file(savef);
    fclose(savef);
    if (num_to_write == num_written && ret == 0) return(TRUE);
    else return(FALSE);
}

restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
    extern char **environ;
    char buf[LINELEN];
    STAT sbuf2;
    int oldcol, oldline;	/* Old number of columns and lines */

    if (strcmp(file, "-r") == 0)
	file = file_name;
    if ((inf = open(file, 0)) < 0)
    {
	perror(file);
	return FALSE;
    }

    fflush(stdout);
    ENCREAD(buf, strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }
    
    /*
     * Get the lines and columns from the previous game
     */

    ENCREAD(buf, 80, inf);
    sscanf(buf, "%d x %d\n", &oldline, &oldcol);
    fstat(inf, &sbuf2);
    fflush(stdout);

    initscr();
 
    if (COLS < oldcol || LINES < oldline) {
	endwin();
	printf("Cannot restart the game on a smaller screen.\n");
	return FALSE;
    }

    setup();
    /*
     * Set up windows
     */
    cw = newwin(lines, cols, 0, 0);
    mw = newwin(lines, cols, 0, 0);
    hw = newwin(lines, cols, 0, 0);
    msgw = newwin(4, cols, 0, 0);

    keypad(cw,1);
    keypad(msgw,1);

    if (rs_restore_file(inf) != 0)
    {
	printf("Cannot restore file\n");
	close(inf);
	return(FALSE);
    }

    cols = COLS;
    lines = LINES;
    if (cols > 85) cols = 85;
    if (lines > 24) lines = 24;

    mpos = 0;
    mvwprintw(msgw, 0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

    /*
     * defeat multiple restarting from the same place
     */
    if (!wizard && md_unlink_open_file(file, inf) < 0) {
	printf("Cannot unlink file\n");
	return FALSE;
    }

    environ = envp;
    strcpy(file_name, file);
    setup();
    clearok(curscr, TRUE);
    touchwin(cw);
    srand(getpid());
    playit();
    /*NOTREACHED*/
    return(0);
}

#define ENCWBSIZ	1024
/*
 * perform an encrypted write
 */
encwrite(start, size, outf)
register char *start;
register unsigned size;
register int outf;
{
    register char *ep;
    register int i = 0;
    int	num_written = 0;
    auto char buf[ENCWBSIZ];

    ep = encstr;

    while (size--)
    {
	buf[i++] = *start++ ^ *ep++ ;
	if (*ep == '\0')
 	   ep = encstr;

	if (i == ENCWBSIZ || size == 0) {
	    if (write(outf, buf, (unsigned)i) < i) 
		 return(num_written);
	    else {
		num_written += i;
		i = 0;
	    }
	}
    }
    return(num_written);
}

/*
 * perform an encrypted read
 */
encread(start, size, inf)
register char *start;
register unsigned size;
register int inf;
{
    register char *ep;
    register int read_size;

    if ((read_size = read(inf, start, size)) == -1 || read_size == 0)
	return read_size;

    ep = encstr;

    size = read_size;
    while (size--)
    {
	*start++ ^= *ep++;
	if (*ep == '\0')
	    ep = encstr;
    }
    return read_size;
}
