/*
 * mach_dep.h  -  machine dependicies
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
 * define that the wizard commands exist
 */
#define	WIZARD	0

/*
 * define if you want to limit scores to one per class per userid
 */
#undef LIMITSCORE /* 1 */

/* 
 * define that rogue should "nice()" itself
 */
#undef NICE	/* 1 */

#ifdef NICE
#define	FUDGE_TIME	70	/* fudge factor allowed in time for saved game*/
#else
#define	FUDGE_TIME	50	/* fudge factor allowed in time for saved game*/
#endif

#undef	DUMP		/* 1 */	/* dump core rather than catch the signal     */
#undef	NOCHECKACCESS	/* 1 */	/* If set, then don't check time on save file */


/*
 * where scorefile should live
 */
#ifndef SCOREFILE
#define SCOREFILE	"/usr/games/lib/rogue_roll"
#endif

/*
 * Variables for checking to make sure the system isn't too loaded
 * for people to play
 */

#if u370
#	define	MAXUSERS	40	/* max number of users for this game */
#	define	MAXPROCESSES	140	/* number processes including system */
					/* processes but not including gettys*/
#endif
#if uts
#	define	MAXUSERS	45	/* max number of users for this game */
#	define	MAXPROCESSES	150	/* number processes including system */
					/* processes but not including gettys*/
#endif
#if vax
#	define	MAXUSERS	17	/* max number of users for this game */
#	define	MAXPROCESSES	85	/* number processes including system */
					/* processes but not including gettys*/
#endif
#if u3b
#	define	MAXUSERS	14	/* max number of users for this game */
#	define	MAXPROCESSES	75	/* number processes including system */
					/* processes but not including gettys*/
#endif

#undef	MAXUSERS
#undef	MAXPROCESSES

#undef	CHECKTIME	/* 15 *//* number of minutes between load checks     */
				/* if not defined checks are only on startup */
#define UTMP	"/etc/utmp"	/* where utmp file lives */
/*
 * define the current author user id of the program for "special handling"
 */
#ifndef AUTHOR
#define AUTHOR 0
#endif
