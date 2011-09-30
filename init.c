/*
 * init.c  -  global variable initializaton
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
 * global variable initializaton
 *
 */

#include "curses.h"
#include <ctype.h>
#include "rogue.h"
#include "mach_dep.h"


/*
 * If there is any news, put it in a character string and assign it to
 * rogue_news.  Otherwise, assign NULL to rogue_news.
 */
static char *rogue_news = "You no longer fall into trading posts. They are \
now entered like going down the stairs.";

char *rainbow[] = {

"amber",		"aquamarine",		"beige",
"black",		"blue",			"brown",
"clear",		"crimson",		"ecru",
"gold",			"green",		"grey",
"indigo",		"khaki",		"lavender",
"magenta",		"orange",		"pink",
"plaid",		"purple",		"red",
"silver",		"saffron",		"scarlet",
"tan",			"tangerine", 		"topaz",
"turquoise",		"vermilion",		"violet",
"white",		"yellow",
};
#define NCOLORS (sizeof rainbow / sizeof(char *))
int cNCOLORS = NCOLORS;

static char *sylls[] = {
    "a", "ab", "ag", "aks", "ala", "an", "ankh", "app", "arg", "arze",
    "ash", "ban", "bar", "bat", "bek", "bie", "bin", "bit", "bjor",
    "blu", "bot", "bu", "byt", "comp", "con", "cos", "cre", "dalf",
    "dan", "den", "do", "e", "eep", "el", "eng", "er", "ere", "erk",
    "esh", "evs", "fa", "fid", "for", "fri", "fu", "gan", "gar",
    "glen", "gop", "gre", "ha", "he", "hyd", "i", "ing", "ion", "ip",
    "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech",
    "man", "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur",
    "nej", "nelg", "nep", "ner", "nes", "net", "nih", "nin", "o", "od",
    "ood", "org", "orn", "ox", "oxy", "pay", "pet", "ple", "plu", "po",
    "pot", "prok", "re", "rea", "rhov", "ri", "ro", "rog", "rok", "rol",
    "sa", "san", "sat", "see", "sef", "seh", "shu", "ski", "sna",
    "sne", "snik", "sno", "so", "sol", "sri", "sta", "sun", "ta",
    "tab", "tem", "ther", "ti", "tox", "trol", "tue", "turs", "u",
    "ulk", "um", "un", "uni", "ur", "val", "viv", "vly", "vom", "wah",
    "wed", "werg", "wex", "whon", "wun", "xo", "y", "yot", "yu",
    "zant", "zap", "zeb", "zim", "zok", "zon", "zum",
};

char *stones[] = {
	"agate",		"alexandrite",		"amethyst",
	"azurite",		"bloodstone",		"cairngorm",
	"carnelian",		"chalcedony",		"chrysoberyl",
	"chrysolite",		"chrysoprase",		"citrine",
	"coral",		"diamond",		"emerald",
	"garnet",		"heliotrope",		"hematite",
	"hyacinth",		"jacinth",		"jade",
	"jargoon",		"jasper",		"kryptonite",
	"lapus lazuli",		"malachite",		"mocca stone",
	"moonstone",		"obsidian",		"olivine",
	"onyx",			"opal",			"pearl",
	"peridot",		"quartz",		"rhodochrosite",
	"rhodolite",		"ruby",			"sapphire",
	"sardonyx",		"serpintine",		"spinel",
	"tiger eye",		"topaz",		"tourmaline",
	"turquoise",		"zircon",
};
#define NSTONES (sizeof stones / sizeof(char *))
int cNSTONES = NSTONES;

char *wood[] = {
	"avocado wood",	"balsa",	"banyan",	"birch",
	"cedar",	"cherry",	"cinnibar",	"dogwood",
	"driftwood",	"ebony",	"eucalyptus",	"hemlock",
	"ironwood",	"mahogany",	"manzanita",	"maple",
	"oak",		"pine",		"redwood",	"rosewood",
	"teak",		"walnut",	"zebra wood", 	"persimmon wood",
};
#define NWOOD (sizeof wood / sizeof(char *))
int cNWOOD = NWOOD;

char *metal[] = {
	"aluminium",	"bone",		"brass",	"bronze",
	"copper",	"chromium",	"iron",		"lead",
	"magnesium",	"pewter",	"platinum",	"silver",
	"steel",	"tin",		"titanium",	"zinc",
};
#define NMETAL (sizeof metal / sizeof(char *))
int cNMETAL = NMETAL;
#define MAX3(a,b,c)     (a > b ? (a > c ? a : c) : (b > c ? b : c))

static bool used[MAX3(NCOLORS, NSTONES, NWOOD)];


/*
 * make sure all the percentages specified in the tables add up to the
 * right amounts
 */
badcheck(name, magic, bound)
char *name;
register struct magic_item *magic;
register int bound;
{
    register struct magic_item *end;

    if (magic[bound - 1].mi_prob == 1000)
	return;
    printf("\nBad percentages for %s:\n", name);
    for (end = &magic[bound] ; magic < end ; magic++)
	printf("%4d%% %s\n", magic->mi_prob, magic->mi_name);
    printf(retstr);
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */

init_colors()
{
    register int i, j;

    for (i = 0; i < NCOLORS; i++)
	used[i] = FALSE;

    for (i = 0 ; i < MAXPOTIONS ; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!used[j]);
	used[j] = TRUE;
	p_colors[i] = rainbow[j];
	p_know[i] = FALSE;
	p_guess[i] = NULL;
	if (i > 0)
		p_magic[i].mi_prob += p_magic[i-1].mi_prob;
    }
    badcheck("potions", p_magic, MAXPOTIONS);
}

/*
 * do any initialization for food
 */

init_foods()
{
    register int i;

    for (i=0; i < MAXFOODS; i++) {
	if (i > 0)
	    foods[i].mi_prob += foods[i-1].mi_prob;
    }
    badcheck("foods", foods, MAXFOODS);
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */

init_materials()
{
    register int i, j;
    register char *str;
    static bool metused[NMETAL];

    for (i = 0; i < NWOOD; i++)
	used[i] = FALSE;
    for (i = 0; i < NMETAL; i++)
	metused[i] = FALSE;

    for (i = 0 ; i < MAXSTICKS ; i++)
    {
	for (;;)
	    if (rnd(100) > 50)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    ws_type[i] = "wand";
		    str = metal[j];
		    metused[j] = TRUE;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!used[j])
		{
		    ws_type[i] = "staff";
		    str = wood[j];
		    used[j] = TRUE;
		    break;
		}
	    }

	ws_made[i] = str;
	ws_know[i] = FALSE;
	ws_guess[i] = NULL;
	if (i > 0)
		ws_magic[i].mi_prob += ws_magic[i-1].mi_prob;
    }
    badcheck("sticks", ws_magic, MAXSTICKS);
}

/*
 * do any initialization for miscellaneous magic
 */

init_misc()
{
    register int i;

    for (i=0; i < MAXMM; i++) {
	m_know[i] = FALSE;
	m_guess[i] = NULL;
	if (i > 0)
	    m_magic[i].mi_prob += m_magic[i-1].mi_prob;
    }
    badcheck("miscellaneous magic", m_magic, MAXMM);
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */

init_names()
{
    register int nsyl;
    register char *cp, *sp;
    register int i, nwords;

    for (i = 0 ; i < MAXSCROLLS ; i++)
    {
	cp = prbuf;
	nwords = rnd(cols/20) + 1 + (cols > 40 ? 1 : 0);
	while(nwords--)
	{
	    nsyl = rnd(3)+1;
	    while(nsyl--)
	    {
		sp = sylls[rnd((sizeof sylls) / (sizeof (char *)))];
		while(*sp)
		    *cp++ = *sp++;
	    }
	    *cp++ = ' ';
	}
	*--cp = '\0';
	s_names[i] = (char *) new(strlen(prbuf)+1);
	s_know[i] = FALSE;
	s_guess[i] = NULL;
	strcpy(s_names[i], prbuf);
	if (i > 0)
		s_magic[i].mi_prob += s_magic[i-1].mi_prob;
    }
    badcheck("scrolls", s_magic, MAXSCROLLS);
}

/*
 * init_player:
 *	roll up the rogue
 */

init_player()
{
    int stat_total, round, minimum, maximum, ch, i, j;
    short do_escape, *our_stats[NUMABILITIES-1];
    struct linked_list	*weap_item, *armor_item;
    struct object *obj;

    weap_item = armor_item = NULL;

    if (char_type == -1) {
	/* See what type character will be */
	wclear(hw);
	touchwin(hw);
	wmove(hw,2,0);
	for(i=1; i<=NUM_CHARTYPES-1; i++) {
	    wprintw(hw,"[%d] %s\n",i,char_class[i-1].name);
	}
	mvwaddstr(hw, 0, 0, "What character class do you desire? ");
	draw(hw);
	char_type = (wgetch(hw) - '0');
	while (char_type < 1 || char_type > NUM_CHARTYPES-1) {
	    wmove(hw,0,0);
	    wprintw(hw,"Please enter a character type between 1 and %d: ",
		    NUM_CHARTYPES-1);
	    draw(hw);
	    char_type = (wgetch(hw) - '0');
	}
	char_type--;
    }
    player.t_ctype = char_type;
    player.t_quiet = 0;
    pack = NULL;

    /* Select the gold */
    purse = 2700;
    switch (player.t_ctype) {
	case C_FIGHTER:
	    purse += 1800;
	when C_THIEF:
	case C_ASSASIN:
	    purse += 600;
    }
#ifdef WIZARD
    /* 
     * allow me to describe a super character 
     */
    if (wizard && strcmp(getenv("SUPER"),"YES") == 0) {
	    pstats.s_str = 25;
	    pstats.s_intel = 25;
	    pstats.s_wisdom = 25;
	    pstats.s_dext = 25;
	    pstats.s_const = 25;
	    pstats.s_charisma = 25;
	    pstats.s_exp = 10000000L;
	    pstats.s_lvladj = 0;
	    pstats.s_lvl = 1;
	    pstats.s_hpt = 500;
	    pstats.s_carry = totalenc(&player);
	    strncpy(pstats.s_dmg, "3d4", sizeof(pstats.s_dmg));
	    check_level();
	    mpos = 0;
	    if (player.t_ctype == C_FIGHTER ||
	        player.t_ctype == C_RANGER  ||
	        player.t_ctype == C_PALADIN)
		weap_item = spec_item(WEAPON, TWOSWORD, 5, 5);
	    else
		weap_item = spec_item(WEAPON, SWORD, 5, 5);
	    obj = OBJPTR(weap_item);
	    obj->o_flags |= ISKNOW;
	    add_pack(weap_item, TRUE, NULL);
	    cur_weapon = obj;
	    if (player.t_ctype != C_MONK) {
		j = PLATE_ARMOR;
		if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASIN)
		    j = STUDDED_LEATHER;
		armor_item = spec_item(ARMOR, j, 10, 0);
		obj = OBJPTR(armor_item);
		obj->o_flags |= (ISKNOW | ISPROT);
		obj->o_weight = armors[j].a_wght;
		add_pack(armor_item, TRUE, NULL);
		cur_armor = obj;
	    }
	    purse += 10000;
    }
    else 
#endif

    {
	switch(player.t_ctype) {
	    case C_MAGICIAN:round = A_INTELLIGENCE;
	    when C_FIGHTER:	round = A_STRENGTH;
	    when C_RANGER:	round = A_STRENGTH;
	    when C_PALADIN:	round = A_STRENGTH;
	    when C_CLERIC:	round = A_WISDOM;
	    when C_DRUID:	round = A_WISDOM;
	    when C_THIEF:	round = A_DEXTERITY;
	    when C_ASSASIN:	round = A_DEXTERITY;
	    when C_MONK:	round = A_DEXTERITY;
	}

	do {
	    wclear(hw);

	    /* If there is any news, display it */
	    if (rogue_news) {
		register int i;

		/* Print a separator line */
		wmove(hw, 12, 0);
		for (i=0; i<cols; i++) waddch(hw, '-');

		/* Print the news */
		mvwaddstr(hw, 14, 0, rogue_news);
	    }

	    stat_total = MAXSTATS;
	    do_escape = FALSE;	/* No escape seen yet */

	    /* Initialize abilities */
	    pstats.s_intel = 0;
	    pstats.s_str = 0;
	    pstats.s_wisdom = 0;
	    pstats.s_dext = 0;
	    pstats.s_const = 0;
	    pstats.s_charisma = 0;

	    /* Initialize pointer into abilities */
	    our_stats[A_INTELLIGENCE] = &pstats.s_intel;
	    our_stats[A_STRENGTH] = &pstats.s_str;
	    our_stats[A_WISDOM] = &pstats.s_wisdom;
	    our_stats[A_DEXTERITY] = &pstats.s_dext;
	    our_stats[A_CONSTITUTION] = &pstats.s_const;

	    /* Let player distribute attributes */
	    for (i=0; i<NUMABILITIES-1; i++) {
		wmove(hw, 2, 0);
		wprintw(hw, "You are creating a %s with %2d attribute points.",
				char_class[player.t_ctype].name, stat_total);

		/*
		 * Player must have a minimum of 7 in any attribute and 11 in
		 * the player's primary attribute.
		 */
		minimum = (round == i ? 11 : 7);

		/* Subtract out remaining minimums */
		maximum = stat_total - (7 * (NUMABILITIES-1 - i));

		/* Subtract out remainder of profession minimum (11 - 7) */
		if (round > i) maximum -= 4;

		/* Maximum can't be greater than 18 */
		if (maximum > 18) maximum = 18;

		wmove(hw, 4, 0);
		wprintw(hw,
		   "Minimum: %2d; Maximum: %2d (%s corrects previous entry)",
		   minimum, maximum, unctrl('\b'));

		wmove(hw, 6, 0);
		wprintw(hw, "    Int: %-2d", pstats.s_intel);
		wprintw(hw, "    Str: %-2d", pstats.s_str);
		wprintw(hw, "    Wis: %-2d", pstats.s_wisdom); 
		wprintw(hw, "    Dex: %-2d", pstats.s_dext);
		wprintw(hw, "    Con: %-2d", pstats.s_const);
		wprintw(hw, "    Cha: %-2d", pstats.s_charisma);
		wclrtoeol(hw);
		wmove(hw, 6, 11*i + 9);
		if (do_escape == FALSE) draw(hw);

		/* Get player's input */
		if (do_escape || maximum == minimum) {
		    *our_stats[i] = maximum;
		    stat_total -= maximum;
		}
		else for (;;) {
		    ch = wgetch(hw);
		    if (ch == '\b') {	/* Backspace */
			if (i == 0) continue;	/* Can't move back */
			else {
			    stat_total += *our_stats[i-1];
			    *our_stats[i] = 0;
			    *our_stats[i-1] = 0;
			    i -= 2;	/* Back out */
			    break;
			}
		    }
		    if (ch == '\033') {	/* Escape */
			/*
			 * Escape will result in using all maximums for
			 * remaining abilities.
			 */
			do_escape = TRUE;
			*our_stats[i] = maximum;
			stat_total -= maximum;
			break;
		    }

		    /* Do we have a legal digit? */
		    if (ch >= '0' && ch <= '9') {
			ch -= '0';	/* Convert it to a number */
			*our_stats[i] = 10 * *our_stats[i] + ch;

			/* Is the number in range? */
			if (*our_stats[i] >= minimum &&
			    *our_stats[i] <= maximum) {
			    stat_total -= *our_stats[i];
			    break;
			}

			/*
			 * If it's too small, get more - 1x is the only
			 * allowable case.
			 */
			if (*our_stats[i] < minimum && *our_stats[i] == 1) {
			    /* Print the player's one */
			    waddch(hw, '1');
			    draw(hw);
			    continue;
			}
		    }

		    /* Error condition */
		    putchar('\007');
		    *our_stats[i] = 0;
		    i--;	/* Rewind */
		    break;
		}
	    }

	    /* Discard extra points over 18 */
	    if (stat_total > 18) stat_total = 18;

	    /* Charisma gets what's left */
	    pstats.s_charisma = stat_total;

	    /* Intialize constants */
	    pstats.s_lvl = 1;
	    pstats.s_lvladj = 0;
	    pstats.s_exp = 0L;
	    strncpy(pstats.s_dmg, "1d4", sizeof(pstats.s_dmg));
	    pstats.s_carry = totalenc(&player);

	    /* Get the hit points. */
	    pstats.s_hpt = 12 + const_bonus();  /* Base plus bonus */

	    /* Add in the component that varies according to class */
	    pstats.s_hpt += char_class[player.t_ctype].hit_pts;

	    /* Display the character */
	    wmove(hw, 2, 0);
	    wprintw(hw,"You are creating a %s.",
			char_class[player.t_ctype].name);
	    wclrtoeol(hw);

	    /* Get rid of max/min line */
	    wmove(hw, 4, 0);
	    wclrtoeol(hw);

	    wmove(hw, 6, 0);
	    wprintw(hw, "    Int: %2d", pstats.s_intel);
	    wprintw(hw, "    Str: %2d", pstats.s_str);
	    wprintw(hw, "    Wis: %2d", pstats.s_wisdom); 
	    wprintw(hw, "    Dex: %2d", pstats.s_dext);
	    wprintw(hw, "    Con: %2d", pstats.s_const);
	    wprintw(hw, "    Cha: %2d", pstats.s_charisma);
	    wclrtoeol(hw);

	    wmove(hw, 8, 0);
	    wprintw(hw, "    Hp: %2d", pstats.s_hpt);
	    wclrtoeol(hw);

	    wmove(hw, 10, 0);
	    wprintw(hw, "    Gold: %d", purse);

	    mvwaddstr(hw, 0, 0, "Is this character okay? ");
	    draw(hw);
	} while(wgetch(hw) != 'y');
    }

    pstats.s_arm = 10;
    max_stats = pstats;

    /* Set up the initial movement rate */
    player.t_action = A_NIL;
    player.t_movement = 6;
    player.t_no_move = 0;
    player.t_using = NULL;
}






/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */

init_stones()
{
    register int i, j;

    for (i = 0; i < NSTONES; i++)
	used[i] = FALSE;

    for (i = 0 ; i < MAXRINGS ; i++)
    {
        do
            j = rnd(NSTONES);
	until (!used[j]);
	used[j] = TRUE;
	r_stones[i] = stones[j];
	r_know[i] = FALSE;
	r_guess[i] = NULL;
	if (i > 0)
		r_magic[i].mi_prob += r_magic[i-1].mi_prob;
    }
    badcheck("rings", r_magic, MAXRINGS);
}

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
init_things()
{
    register struct magic_item *mp;

    for (mp = &things[1] ; mp < &things[NUMTHINGS] ; mp++)
	mp->mi_prob += (mp-1)->mi_prob;
    badcheck("things", things, NUMTHINGS);
}


