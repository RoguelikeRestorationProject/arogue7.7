/* 
 * wizard.c - Special wizard commands
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
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 */

#include "curses.h"
#include <ctype.h>
#include "rogue.h"
#ifdef PC7300
#include "menu.h"
#endif


/*
 * create_obj:
 *	Create any object for wizard, scroll, magician, or cleric
 */
create_obj(prompt, which_item, which_type)
bool prompt;
int which_item, which_type;
{
    reg struct linked_list *item;
    reg struct object *obj;
    reg int wh;
    reg char ch, newitem, newtype, whc, msz, *pt;
    WINDOW *thiswin;

    thiswin = cw;
    if (prompt) {
	bool nogood = TRUE;

	thiswin = hw;
	wclear(hw);
	wprintw(hw,"Item\t\t\tKey\n\n");
	wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_RING].mi_name,RING,
		things[TYP_STICK].mi_name,STICK);
	wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_POTION].mi_name,POTION,
		things[TYP_SCROLL].mi_name,SCROLL);
	wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_ARMOR].mi_name,ARMOR,
		things[TYP_WEAPON].mi_name,WEAPON);
	wprintw(hw,"%s\t%c\n",things[TYP_MM].mi_name,MM);
	wprintw(hw,"%s\t\t\t%c\n",things[TYP_FOOD].mi_name,FOOD);
	if (wizard) {
	    wprintw(hw,"%s\t\t%c\n",things[TYP_RELIC].mi_name,RELIC);
	    waddstr(hw,"monster\t\t\tm");
	}
	wprintw(hw,"\n\nWhat do you want to create? ");
	draw(hw);
	do {
	    ch = wgetch(hw);
	    if (ch == ESCAPE) {
		restscr(cw);
		return;
	    }
	    switch (ch) {
		case RING:
		case STICK:	
		case POTION:
		case SCROLL:	
		case ARMOR:	
		case WEAPON:
		case FOOD:
		case MM:
		    nogood = FALSE;
		    break;
		case RELIC:
		case 'm':
		    if (wizard) 
			nogood = FALSE;
		    break;
		default:
		    nogood = TRUE;
	    }
	} while (nogood);
	newitem = ch;
    }
    else
	newitem = which_item;

    pt = "those";
    msz = 0;
    if(newitem == 'm') {
	/* make monster and be done with it */
	wh = makemonster(TRUE, "Creation", "create");
	if (wh > 0) {
	    creat_mons (&player, wh, TRUE);
	    light(&hero);
	}
	return;
    }
    if(newitem == GOLD)
	pt = "gold";
    /* else if(isatrap(newitem))
	pt = "traps";
    */
    switch(newitem) {
	case POTION:	whc = TYP_POTION;	msz = MAXPOTIONS;
	when SCROLL:	whc = TYP_SCROLL;	msz = MAXSCROLLS;
	when WEAPON:	whc = TYP_WEAPON;	msz = MAXWEAPONS;
	when ARMOR:	whc = TYP_ARMOR;	msz = MAXARMORS;
	when RING:	whc = TYP_RING;		msz = MAXRINGS;
	when STICK:	whc = TYP_STICK;	msz = MAXSTICKS;
	when MM:	whc = TYP_MM;		msz = MAXMM;
	when RELIC:	whc = TYP_RELIC;	msz = MAXRELIC;
	when FOOD:	whc = TYP_FOOD;		msz = MAXFOODS;
	otherwise:
	    if (thiswin == hw)
		restscr(cw);
	    mpos = 0;
	    msg("Even wizards can't create %s !!",pt);
	    return;
    }
    if(msz == 1) {		/* if only one type of item */
	ch = 'a';
    }
    else if (prompt) {
	register struct magic_item *wmi;
	char wmn;
	register int ii;
	int old_prob;

	mpos = 0;
	wmi = NULL;
	wmn = 0;
	switch(newitem) {
		case POTION:	wmi = &p_magic[0];
		when SCROLL:	wmi = &s_magic[0];
		when RING:	wmi = &r_magic[0];
		when STICK:	wmi = &ws_magic[0];
		when MM:	wmi = &m_magic[0];
		when RELIC:	wmi = &rel_magic[0];
		when FOOD:	wmi = &foods[0];
		when WEAPON:	wmn = 1;
		when ARMOR:	wmn = 2;
	}
	wclear(hw);
	thiswin = hw;
	if (wmi != NULL) {
	    ii = old_prob = 0;
	    while (ii < msz) {
		if(wmi->mi_prob == old_prob && wizard == FALSE) { 
		    msz--; /* can't make a unique item */
		}
		else {
		    mvwaddch(hw,ii % 13,ii > 12 ? cols/2 : 0, ii + 'a');
		    waddstr(hw,") ");
		    waddstr(hw,wmi->mi_name);
		    ii++;
		}
		old_prob = wmi->mi_prob;
	        wmi++;
	    }
	}
	else if (wmn != 0) {
	    for(ii = 0 ; ii < msz ; ii++) {
	        mvwaddch(hw,ii % 13,ii > 12 ? cols/2 : 0, ii + 'a');
	        waddstr(hw,") ");
	        if(wmn == 1)
		    waddstr(hw,weaps[ii].w_name);
	        else
		    waddstr(hw,armors[ii].a_name);
	    }
	}
	sprintf(prbuf,"Which %s? ",things[whc].mi_name);
	mvwaddstr(hw,lines - 1, 0, prbuf);
	draw(hw);
	do {
	    ch = wgetch(hw);
	    if (ch == ESCAPE) {
	        restscr(cw);
	        msg("");
	        return;
	    }
	} until (isalpha(ch));
        if (thiswin == hw)			/* restore screen if need be */
	    restscr(cw);
        newtype = tolower(ch) - 'a';
        if(newtype < 0 || newtype >= msz) {	/* if an illegal value */
	    mpos = 0;
	    msg("There is no such %s",things[whc].mi_name);
	    return;
        }
    }
    else 
	newtype = which_type;
    item = new_item(sizeof *obj);	/* get some memory */
    obj = OBJPTR(item);
    obj->o_type = newitem;		/* store the new items */
    obj->o_mark[0] = '\0';
    obj->o_which = newtype;
    obj->o_group = 0;
    obj->contents = NULL;
    obj->o_count = 1;
    obj->o_flags = 0;
    obj->o_dplus = obj->o_hplus = 0;
    obj->o_weight = 0;
    wh = obj->o_which;
    mpos = 0;
    if (!wizard)			/* users get 0 to +3 */
	whc = rnd(4);
    else			/* wizard gets to choose */
	whc = getbless();
    if (whc < 0)
	obj->o_flags |= ISCURSED;
    switch (obj->o_type) {
	case WEAPON:
	case ARMOR:
	    if (obj->o_type == WEAPON) {
		init_weapon(obj, wh);
		obj->o_hplus += whc;
		obj->o_dplus += whc;
	    }
	    else {				/* armor here */
		obj->o_weight = armors[wh].a_wght;
		obj->o_ac = armors[wh].a_class - whc;
	    }
	when RING:
	    if (whc > 1 && r_magic[wh].mi_bless != 0)
		obj->o_flags |= ISBLESSED;
	    r_know[wh] = TRUE;
	    switch(wh) {
		case R_ADDSTR:
		case R_ADDWISDOM:
		case R_ADDINTEL:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		case R_DIGEST:
		    obj->o_ac = whc + 1;
		    break;
		default: 
		    obj->o_ac = 0;
	    }
	    obj->o_weight = things[TYP_RING].mi_wght;
	when MM:
	    if (whc > 1 && m_magic[wh].mi_bless != 0)
		obj->o_flags |= ISBLESSED;
	    m_know[wh] = TRUE;
	    switch(wh) {
		case MM_JUG:
		    switch(rnd(11)) {
			case 0: obj->o_ac = P_PHASE;
			when 1: obj->o_ac = P_CLEAR;
			when 2: obj->o_ac = P_SEEINVIS;
			when 3: obj->o_ac = P_HEALING;
			when 4: obj->o_ac = P_MFIND;
			when 5: obj->o_ac = P_TFIND;
			when 6: obj->o_ac = P_HASTE;
			when 7: obj->o_ac = P_RESTORE;
			when 8: obj->o_ac = P_FLY;
			when 9: obj->o_ac = P_SKILL;
			when 10:obj->o_ac = P_FFIND;
		    }
		when MM_OPEN:
		case MM_HUNGER:
		case MM_DRUMS:
		case MM_DISAPPEAR:
		case MM_CHOKE:
		case MM_KEOGHTOM:
		    if (whc < 0)
			whc = -whc; 	/* these cannot be negative */
		    obj->o_ac = (whc + 1) * 5;
		    break;
		when MM_BRACERS:
		    obj->o_ac = whc * 2 + 1;
		when MM_DISP:
		    obj->o_ac = 2;
		when MM_PROTECT:
		    obj->o_ac = whc;
		when MM_SKILLS:
		    if (wizard && whc != 0)
			obj->o_ac = rnd(NUM_CHARTYPES-1);
		    else
			obj->o_ac = player.t_ctype;
		otherwise: 
		    obj->o_ac = 0;
	    }
	    obj->o_weight = things[TYP_MM].mi_wght;
	when STICK:
	    if (whc > 1 && ws_magic[wh].mi_bless != 0)
		obj->o_flags |= ISBLESSED;
	    ws_know[wh] = TRUE;
	    fix_stick(obj);
	when SCROLL:
	    if (whc > 1 && s_magic[wh].mi_bless != 0)
		obj->o_flags |= ISBLESSED;
	    obj->o_weight = things[TYP_SCROLL].mi_wght;
	    s_know[wh] = TRUE;
	when POTION:
	    if (whc > 1 && p_magic[wh].mi_bless != 0)
		obj->o_flags |= ISBLESSED;
	    obj->o_weight = things[TYP_POTION].mi_wght;
	    if (wh == P_ABIL) obj->o_kind = rnd(NUMABILITIES);
	    p_know[wh] = TRUE;
	when RELIC:
	    obj->o_weight = things[TYP_RELIC].mi_wght;
	    switch (obj->o_which) {
	        case QUILL_NAGROM: obj->o_charges = QUILLCHARGES;
	        when EMORI_CLOAK:  obj->o_charges = 1;
	        otherwise: break;
	    }
	when FOOD:
	    obj->o_weight = things[TYP_FOOD].mi_wght;
    }
    mpos = 0;
    obj->o_flags |= ISKNOW;
    if (add_pack(item, FALSE, NULL) == FALSE) {
	obj->o_pos = hero;
	fall(item, TRUE);
    }
}

/*
 * getbless:
 *	Get a blessing for a wizards object
 */
getbless()
{
	reg char bless;

	msg("Blessing? (+,-,n)");
	bless = wgetch(msgw);
	if (bless == '+')
		return (rnd(3) + 2);
	else if (bless == '-')
		return (-rnd(3) - 1);
	else
		return (0);
}

/*
 * get a non-monster death type
 */
getdeath()
{
    register int i;
    int which_death;
    char label[80];

    clear();
    for (i=0; i<DEATHNUM; i++) {
	sprintf(label, "[%d] %s", i+1, deaths[i].name);
	mvaddstr(i+2, 0, label);
    }
    mvaddstr(0, 0, "Which death? ");
    refresh();

    /* Get the death */
    for (;;) {
	get_str(label, stdscr);
	which_death = atoi(label);
	if ((which_death < 1 || which_death > DEATHNUM)) {
	    mvaddstr(0, 0, "Please enter a number in the displayed range -- ");
	    refresh();
	}
	else break;
    }
    return(deaths[which_death-1].reason);
}

#ifdef PC7300
static menu_t Display;				/* The menu structure */
static mitem_t Dispitems[NUMMONST+1];		/* Info for each line */
static char Displines[NUMMONST+1][LINELEN+1];	/* The lines themselves */
#endif

/*
 * make a monster for the wizard
 */
makemonster(showall, label, action) 
bool showall;	/* showall -> show uniques and genocided creatures */
char *label, *action;
{
#ifdef PC7300
    register int nextmonst;
#endif
    register int i;
    register short which_monst;
    register int num_monst = NUMMONST, pres_monst=1, num_lines=2*(lines-3);
    int max_monster;
    char monst_name[40];

    /* If we're not showing all, subtract out the UNIQUES and quartermaster */
    if (!showall) num_monst -= NUMUNIQUE + 1;
    max_monster = num_monst;

#ifdef PC7300
    nextmonst = 0;
    for (i=1; i<=num_monst; i++) {
	/* Only display existing monsters if we're not showing them all */
	if (showall || monsters[i].m_normal) {
	    strcpy(Displines[nextmonst], monsters[i]);
	    Dispitems[nextmonst].mi_name = Displines[nextmonst];
	    Dispitems[nextmonst].mi_flags = 0;
	    Dispitems[nextmonst++].mi_val = i;
	}
    }

    /* Place an end marker for the items */
    Dispitems[nextmonst].mi_name = 0;

    /* Set up the main menu structure */
    Display.m_label = label;
    Display.m_title = "Monster Listing";
    Display.m_prompt = "Select a monster or press Cancl.";
    Display.m_curptr = '\0';
    Display.m_markptr = '\0';
    Display.m_flags = 0;
    Display.m_selcnt = 1;
    Display.m_items = Dispitems;
    Display.m_curi = 0;

    /*
     * Try to display the menu.  If we don't have a local terminal,
     * the call will fail and we will just continue with the
     * normal mode.
     */
    if (menu(&Display) >= 0) {
	restscr(cw);
	touchwin(cw);
	return(Display.m_selcnt == 1 ? Display.m_curi->mi_val : -1);
    }
#endif

    /* Print out the monsters */
    while (num_monst > 0) {
	register left_limit;

	if (num_monst < num_lines) left_limit = (num_monst+1)/2;
	else left_limit = num_lines/2;

	wclear(hw);
	touchwin(hw);

	/* Print left column */
	wmove(hw, 2, 0);
	for (i=0; i<left_limit; i++) {
	    sprintf(monst_name, "[%d] %c%s\n",
				pres_monst,
				(showall || monsters[pres_monst].m_normal)
				    ? ' '
				    : '*',
				monsters[pres_monst].m_name);
	    waddstr(hw, monst_name);
	    pres_monst++;
	}

	/* Print right column */
	for (i=0; i<left_limit && pres_monst<=max_monster; i++) {
	    sprintf(monst_name, "[%d] %c%s",
				pres_monst,
				(showall || monsters[pres_monst].m_normal)
				    ? ' '
				    : '*',
				monsters[pres_monst].m_name);
	    wmove(hw, i+2, cols/2);
	    waddstr(hw, monst_name);
	    pres_monst++;
	}

	if ((num_monst -= num_lines) > 0) {
	    mvwaddstr(hw, lines-1, 0, morestr);
	    draw(hw);
	    wait_for(' ');
	}

	else {
	    mvwaddstr(hw, 0, 0, "Which monster");
	    if (!terse) {
		waddstr(hw, " do you wish to ");
		waddstr(hw, action);
	    }
	    waddstr(hw, "? ");
	    draw(hw);
	}
    }

get_monst:
    get_str(monst_name, hw);
    which_monst = atoi(monst_name);
    if ((which_monst < 1 || which_monst > max_monster)) {
	mvwaddstr(hw, 0, 0, "Please enter a number in the displayed range -- ");
	draw(hw);
	goto get_monst;
    }
    restscr(cw);
    touchwin(cw);
    return(which_monst);
}

/*
 * passwd:
 *	see if user knows password
 */

passwd()
{
    register char *sp, c;
    char buf[LINELEN], *crypt();

    msg("Wizard's Password:");
    mpos = 0;
    sp = buf;
    while ((c = readchar()) != '\n' && c != '\r' && c != '\033')
	if (c == md_killchar())
	    sp = buf;
	else if (c == md_erasechar() && sp > buf)
	    sp--;
	else
	    *sp++ = c;
    if (sp == buf)
	return FALSE;
    *sp = '\0';
    return (strcmp(PASSWD, md_crypt(buf, "mT")) == 0);
}


/*
 * teleport:
 *	Bamf the hero someplace else
 */

teleport()
{
    register struct room *new_rp, *old_rp = roomin(&hero);
    register int rm, which;
    coord old;
    bool got_position = FALSE;

    /* Disrupt whatever the hero was doing */
    dsrpt_player();

    /*
     * If the hero wasn't doing something disruptable, NULL out his
     * action anyway and let him know about it.  We don't want him
     * swinging or moving into his old place.
     */
    if (player.t_action != A_NIL) {
	player.t_action = A_NIL;
	msg("You feel momentarily disoriented.");
    }

    old = hero;
    mvwaddch(cw, hero.y, hero.x, mvwinch(stdscr, hero.y, hero.x));
    if (ISWEARING(R_TELCONTROL) || wizard) {
	got_position = move_hero(H_TELEPORT);
	if (!got_position)
	    msg("Your attempt fails.");
	else {
	    new_rp = roomin(&hero);
	    msg("You teleport successfully.");
	}
    }
    if (!got_position) {
	do {
	    rm = rnd_room();
	    rnd_pos(&rooms[rm], &hero);
	} until(winat(hero.y, hero.x) == FLOOR);
	new_rp = &rooms[rm];
    }
    player.t_oldpos = old;	/* Save last position */

    /* If hero gets moved, darken old room */
    if (old_rp && old_rp != new_rp) {
	old_rp->r_flags |= FORCEDARK;	/* Fake darkness */
	light(&old);
	old_rp->r_flags &= ~FORCEDARK; /* Restore light state */
    }

    /* Darken where we just came from */
    else if (levtype == MAZELEV) light(&old);

    light(&hero);
    mvwaddch(cw, hero.y, hero.x, PLAYER);
    /* if entering a treasure room, wake everyone up......Surprise! */
    if (new_rp->r_flags & ISTREAS)
	wake_room(new_rp);

    /* Reset current room and position */
    oldrp = new_rp;	/* Used in look() */
    player.t_oldpos = hero;
    /*
     * make sure we set/unset the ISINWALL on a teleport
     */
    which = winat(hero.y, hero.x);
    if (isrock(which)) turn_on(player, ISINWALL);
    else turn_off(player, ISINWALL);

    /*
     * turn off ISHELD in case teleportation was done while fighting
     * something that holds you
     */
    if (on(player, ISHELD)) {
	register struct linked_list *ip, *nip;
	register struct thing *mp;

	turn_off(player, ISHELD);
	hold_count = 0;
	for (ip = mlist; ip; ip = nip) {
	    mp = THINGPTR(ip);
	    nip = next(ip);
	    if (on(*mp, DIDHOLD)) {
		turn_off(*mp, DIDHOLD);
		turn_on(*mp, CANHOLD);
	    }
	    turn_off(*mp, DIDSUFFOCATE); /* Suffocation -- see below */
	}
    }

    /* Make sure player does not suffocate */
    extinguish(suffocate);

    count = 0;
    running = FALSE;
    md_flushinp();
    return rm;
}

/*
 * whatis:
 *	What a certin object is
 */

whatis(what)
struct linked_list *what;
{
    register struct object *obj;
    register struct linked_list *item;

    if (what == NULL) {		/* do we need to ask which one? */
	if ((item = get_item(pack, "identify", IDENTABLE, FALSE, FALSE))==NULL)
	    return;
    }
    else
	item = what;
    obj = OBJPTR(item);
    switch (obj->o_type) {
        case SCROLL:
	    s_know[obj->o_which] = TRUE;
	    if (s_guess[obj->o_which]) {
		free(s_guess[obj->o_which]);
		s_guess[obj->o_which] = NULL;
	    }
        when POTION:
	    p_know[obj->o_which] = TRUE;
	    if (p_guess[obj->o_which]) {
		free(p_guess[obj->o_which]);
		p_guess[obj->o_which] = NULL;
	    }
	when STICK:
	    ws_know[obj->o_which] = TRUE;
	    if (ws_guess[obj->o_which]) {
		free(ws_guess[obj->o_which]);
		ws_guess[obj->o_which] = NULL;
	    }
        when RING:
	    r_know[obj->o_which] = TRUE;
	    if (r_guess[obj->o_which]) {
		free(r_guess[obj->o_which]);
		r_guess[obj->o_which] = NULL;
	    }
        when MM:
	    /* If it's an identified jug, identify its potion */
	    if (obj->o_which == MM_JUG && (obj->o_flags & ISKNOW)) {
		if (obj->o_ac != JUG_EMPTY)
		    p_know[obj->o_ac] = TRUE;
		break;
	    }

	    m_know[obj->o_which] = TRUE;
	    if (m_guess[obj->o_which]) {
		free(m_guess[obj->o_which]);
		m_guess[obj->o_which] = NULL;
	    }
	otherwise:
	    break;
    }
    obj->o_flags |= ISKNOW;
    if (what == NULL)
	msg(inv_name(obj, FALSE));
}

