/*
 * network.h  -  networking setup
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
 * Note that networking is set up for machines that can communicate
 * via some system such as uucp.  The mechanism listed here uses
 * uux and assumes that the target machine allows access to the
 * game via the uux command.  NETCOMMAND must be defined if networking
 * is desired.
 */

/* #define	NETCOMMAND "uux - -n '%s!%s -u' >/dev/null 2>&1" */

/* Networking information -- should not vary among networking machines */
#define	SYSLEN 9
#define	LOGLEN 8
#undef	NUMNET /* 1 */
struct network {
    char *system;
    char *rogue;
};
extern struct network Network[];

/* This system's name -- should not be defined if uname() is available */
#undef SYSTEM 	/* "ihesa" */
