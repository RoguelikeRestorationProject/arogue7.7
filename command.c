/*
 * command.c  -  Read and execute the user commands
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
 * Read and execute the user commands
 *
 */

#include "curses.h"
#include <ctype.h>
#include <signal.h>
#include "mach_dep.h"
#include "rogue.h"
#ifdef PC7300
#include "sys/window.h"
extern struct uwdata wdata;
#endif

/*
 * command:
 *	Process the user commands
 */

command()
{
    unsigned char ch;
    struct linked_list *item;
    unsigned char countch, direction, newcount = FALSE;
    int segment = 1;
    int monst_limit, monst_current;

    monst_limit = monst_current = 1;
    while (playing) {
	/*
	 * Let the daemons start up, but only do them once a round
	 * (round = 10 segments).
	 */
	if (segment >= 10) {
	    do_daemons(BEFORE);
	    do_fuses(BEFORE);
	}

	after = TRUE;
	do {
	    /* One more tick of the clock. */
	    if (segment >= 10 && after && (++turns % DAYLENGTH) == 0) {
		daytime ^= TRUE;
		if (levtype == OUTSIDE) {
		    if (daytime) msg("The sun rises above the horizon");
		    else msg("The sun sinks below the horizon");
		}
		light(&hero);
	    }

	    /*
	     * Don't bother with these updates unless the player's going
	     * to do something.
	     */
	    if (player.t_action == A_NIL && player.t_no_move <= 1) {
		look(after, FALSE);
		lastscore = purse;
		wmove(cw, hero.y, hero.x);
		if (!((running || count) && jump)) {
		    status(FALSE);
		}
	    }

	    /* Draw the screen */
	    if (!((running || count) && jump)) {
		wmove(cw, hero.y, hero.x);
		draw(cw);
	    }

	    after = TRUE;

	    /*
	     * Read command or continue run
	     */
	    if (--player.t_no_move <= 0) {
		take = 0;		/* Nothing here to start with */
		player.t_no_move = 0;	/* Be sure we don't go too negative */
		if (!running) door_stop = FALSE;

		/* Was the player being held? */
		if (player.t_action == A_FREEZE) {
		    player.t_action = A_NIL;
		    msg("You can move again.");
		}

		if (player.t_action != A_NIL) ch = (char) player.t_action;
		else if (running) {
		    char scratch;

		    /* If in a corridor or maze, if we are at a turn with
		     * only one way to go, turn that way.
		     */
		    scratch = CCHAR( winat(hero.y, hero.x) );
		    if ((scratch==PASSAGE||scratch==DOOR||levtype==MAZELEV)  &&
			off(player, ISHUH)				     && 
			off(player, ISBLIND)) {
			int y, x;
			if (getdelta(runch, &y, &x) == TRUE) {
			    corr_move(y, x);
			}
		    }
		    ch = runch;
		}
		else if (count) ch = countch;
		else {
		    ch = readchar();
		    if (mpos != 0 && !running)	/* Erase message if its there */
			msg("");
		}

		/*
		 * check for prefixes
		 */
		if (isdigit(ch))
		{
		    count = 0;
		    newcount = TRUE;
		    while (isdigit(ch))
		    {
			count = count * 10 + (ch - '0');
			if (count > 255)
				count = 255;
			ch = readchar();
		    }
		    countch = ch;
		    /*
		     * turn off count for commands which don't make sense
		     * to repeat
		     */
		    switch (ch) {
			case 'h': case 'j': case 'k': case 'l':
			case 'y': case 'u': case 'b': case 'n':
			case 'H': case 'J': case 'K': case 'L':
			case 'Y': case 'U': case 'B': case 'N':
			case C_SEARCH: case '.':
			    break;
			default:
			    count = 0;
		    }
		}

		/* Save current direction */
		if (!running) { /* If running, it is already saved */
		    switch (ch) {
			case 'h': case 'j': case 'k': case 'l':
			case 'y': case 'u': case 'b': case 'n':
			case 'H': case 'J': case 'K': case 'L':
			case 'Y': case 'U': case 'B': case 'N':
			    runch = tolower(ch);
		    }
		}

		/* Perform the action */
		switch (ch) {
		    case 'f':
			if (!on(player, ISBLIND))
			{
			    door_stop = TRUE;
			    firstmove = TRUE;
			}
			if (count && !newcount)
			    ch = direction;
			else
			    ch = readchar();
			switch (ch)
			{
			    case 'h': case 'j': case 'k': case 'l':
			    case 'y': case 'u': case 'b': case 'n':
				ch = toupper(ch);
			}
			direction = ch;
		}
		newcount = FALSE;

		/*
		 * execute a command
		 */
		if (count && !running)
		    count--;
		switch (ch) {
		    case '!' : shell();
		    when 'h' : do_move(0, -1);
		    when 'j' : do_move(1, 0);
		    when 'k' : do_move(-1, 0);
		    when 'l' : do_move(0, 1);
		    when 'y' : do_move(-1, -1);
		    when 'u' : do_move(-1, 1);
		    when 'b' : do_move(1, -1);
		    when 'n' : do_move(1, 1);
		    when 'H' : do_run('h');
		    when 'J' : do_run('j');
		    when 'K' : do_run('k');
		    when 'L' : do_run('l');
		    when 'Y' : do_run('y');
		    when 'U' : do_run('u');
		    when 'B' : do_run('b');
		    when 'N' : do_run('n');
		    when A_PICKUP:
			player.t_action = A_NIL;
			if (add_pack(NULL, FALSE, NULL)) {
			    char tch;

			    tch = CCHAR( mvwinch(stdscr, hero.y, hero.x) );
			    if (tch != FLOOR && tch != PASSAGE) {
			        player.t_action = A_PICKUP; /*get more */
			        player.t_no_move += 2 * movement(&player);
			    }
			}
		    when A_ATTACK:
			/* Is our attackee still there? */
			if (isalpha(winat(player.t_newpos.y,
					  player.t_newpos.x))) {
			    /* Our friend is still here */
			    player.t_action = A_NIL;
			    fight(&player.t_newpos, cur_weapon, FALSE);
			}
			else {	/* Our monster has moved */
			    player.t_action = A_NIL;
			}
		    when A_THROW:
			if (player.t_action == A_NIL) {
			    item = get_item(pack, "throw", ALL, FALSE, FALSE);
			    if (item != NULL && get_dir(&player.t_newpos)) {
				player.t_action = A_THROW;
				player.t_using = item;
				player.t_no_move = 2 * movement(&player);
			    }
			    else
				after = FALSE;
			}
			else {
			    missile(player.t_newpos.y, player.t_newpos.x, 
				    player.t_using, &player);
			    player.t_action = A_NIL;
			    player.t_using = 0;
			}
		    when 'Q' : after = FALSE; quit(0);
		    when 'i' : after = FALSE; inventory(pack, ALL);
		    when 'I' : after = FALSE; picky_inven();
		    when C_DROP : player.t_action = C_DROP; drop(NULL);
		    when 'P' :
			if (levtype != POSTLEV) {
			    /* We charge 2 movement units per item */
			    player.t_no_move =
				2 * grab(hero.y, hero.x) * movement(&player);
			}
			else {
			    /* Let's quote the wise guy a price */
			    buy_it();
			    after = FALSE;
			}
		    when C_COUNT : count_gold();
		    when C_QUAFF : quaff(-1, NULL, NULL, TRUE);
		    when C_READ : read_scroll(-1, NULL, TRUE);
		    when C_EAT : eat();
		    when C_WIELD : wield();
		    when C_WEAR : wear();
		    when C_TAKEOFF : take_off();
		    when 'o' : option();
		    when CTRL('N') : nameit();
		    when '=' : after = FALSE; display();
		    when 'm' : nameitem(NULL, TRUE);
		    when '>' : after = FALSE; d_level();
		    when '<' : after = FALSE; u_level();
		    when '?' : after = FALSE; help();
		    when '/' : after = FALSE; identify(NULL);
		    when C_USE : use_mm(-1);
		    when CTRL('T') :
			if (player.t_action == A_NIL) {
			    if (get_dir(&player.t_newpos)) {
				player.t_action = CTRL('T');
				player.t_no_move = 2 * movement(&player);
			    }
			    else
				after = FALSE;
			}
			else {
			    steal();
			    player.t_action = A_NIL;
			}
		    when C_DIP : dip_it();
		    when 'G' :
			if (player.t_action == A_NIL) {
			    player.t_action = 'G';
			    player.t_no_move = movement(&player);
			}
			else {
			    gsense();
			    player.t_action = A_NIL;
			}
		    when C_SETTRAP : set_trap(&player, hero.y, hero.x);
		    when C_SEARCH :
			if (player.t_action == A_NIL) {
			    player.t_action = C_SEARCH;
			    player.t_no_move = 5 * movement(&player);
			}
			else {
			    search(FALSE, FALSE);
			    player.t_action = A_NIL;
			}
		    when C_ZAP : if (!player_zap(NULL, FALSE))
				    after=FALSE;
		    when C_PRAY : pray();
		    when C_CHANT : chant();
		    when C_CAST : cast();
		    when 'a' :
			if (player.t_action == A_NIL) {
			    if (get_dir(&player.t_newpos)) {
				player.t_action = 'a';
				player.t_no_move = 2 * movement(&player);
			    }
			    else
				after = FALSE;
			}
			else {
			    affect();
			    player.t_action = A_NIL;
			}
		    when 'v' : after = FALSE;
			       msg("Advanced Rogue Version %s.",
				    release);
		    when CTRL('L') : after = FALSE; clearok(curscr, TRUE);
				    touchwin(cw); /* MMMMMMMMMM */
		    when CTRL('R') : after = FALSE; msg(huh);
		    when 'S' : 
			after = FALSE;
			if (save_game())
			{
			    wclear(cw);
			    draw(cw);
			    endwin();
#ifdef PC7300
			    endhardwin();
#endif
			    exit(0);
			}
		    when '.' :
			player.t_no_move = movement(&player);  /* Rest */
			player.t_action = A_NIL;
		    when ' ' : after = FALSE;	/* Do Nothing */
#ifdef WIZARD
		    when CTRL('P') :
			after = FALSE;
			if (wizard)
			{
			    wizard = FALSE;
			    trader = 0;
			    msg("Not wizard any more");
			}
			else
			{
			    if (waswizard || passwd())
			    {
				msg("Welcome, oh mighty wizard.");
				wizard = waswizard = TRUE;
			    }
			    else
				msg("Sorry");
			}
#endif
		    when ESCAPE :	/* Escape */
			door_stop = FALSE;
			count = 0;
			after = FALSE;
		    when '#':
			if (levtype == POSTLEV)		/* buy something */
			    buy_it();
			after = FALSE;
		    when '$':
			if (levtype == POSTLEV)		/* price something */
			    price_it();
			after = FALSE;
		    when '%':
			if (levtype == POSTLEV)		/* sell something */
			    sell_it();
			after = FALSE;
		    otherwise :
			after = FALSE;
#ifdef WIZARD
			if (wizard) switch (ch)
			{
			    case 'M' : create_obj(TRUE, 0, 0);
			    when CTRL('W') : wanderer();
			    when CTRL('I') : inventory(lvl_obj, ALL);
			    when CTRL('Z') : whatis(NULL);
			    when CTRL('D') : level++; new_level(NORMLEV);
			    when CTRL('F') : overlay(stdscr,cw);
			    when CTRL('X') : overlay(mw,cw);
			    when CTRL('J') : teleport();
			    when CTRL('E') : msg("food left: %d\tfood level: %d", 
						    food_left, foodlev);
			    when CTRL('A') : activity();
			    when CTRL('C') : 
			    {
				int tlev;
				prbuf[0] = 0;
				msg("Which level? ");
				if(get_str(prbuf,msgw) == NORM) {
				    tlev = atoi(prbuf);
				    if(tlev < 1) {
					mpos = 0;
					msg("Illegal level.");
				    }
				    else if (tlev > 199) {
					    levtype = MAZELEV;
					    level = tlev - 200 + 1;
				    }
				    else if (tlev > 99) {
					    levtype = POSTLEV;
					    level = tlev - 100 + 1;
				    } 
				    else {
					    levtype = NORMLEV;
					    level = tlev;
				    }
				    new_level(levtype);
				}
			    }
			    when CTRL('G') :
			    {
				item=get_item(pack,"charge",STICK,FALSE,FALSE);
				if (item != NULL) {
				    (OBJPTR(item))->o_charges=10000;
				}
			    }
			    when CTRL('H') :
			    {
				register int i;
				register struct object *obj;

				for (i = 0; i < 9; i++)
				    raise_level();
				/*
				 * Give the rogue a sword 
				 */
				if (cur_weapon==NULL || cur_weapon->o_type !=
				    RELIC) {
				    item = spec_item(WEAPON, TWOSWORD, 5, 5);
				    add_pack(item, TRUE, NULL);
				    cur_weapon = OBJPTR(item);
				    (OBJPTR(item))->o_flags |= (ISKNOW|ISPROT);
				}
				/*
				 * And his suit of armor
				 */
				if (player.t_ctype != C_MONK) {
				    item = spec_item(ARMOR, PLATE_ARMOR, 10, 0);
				    obj = OBJPTR(item);
				    if (player.t_ctype == C_THIEF)
				        obj->o_which = STUDDED_LEATHER;
				    obj->o_flags |= (ISKNOW | ISPROT);
				    obj->o_weight = armors[PLATE_ARMOR].a_wght;
				    cur_armor = obj;
				    add_pack(item, TRUE, NULL);
				}
				purse += 20000;
			    }
			    otherwise :
				msg("Illegal command '%s'.", unctrl(ch));
				count = 0;
			}
			else
#endif
			{
			    msg("Illegal command '%s'.", unctrl(ch));
			    count = 0;
			    after = FALSE;
			}
		}

		/*
		 * If he ran into something to take, let him pick it up.
		 * unless it's a trading post
		 */
		if (auto_pickup && take != 0 && levtype != POSTLEV) {
		    /* get ready to pick it up */
		    player.t_action = A_PICKUP;
		    player.t_no_move += 2 * movement(&player);
		}
	    }

	    /* If he was fighting, let's stop (for now) */
	    if (player.t_quiet < 0) player.t_quiet = 0;

	    if (!running)
		door_stop = FALSE;

	    if (after && segment >= 10) {
		/*
		 * Kick off the rest if the daemons and fuses
		 */

		/* 
		 * If player is infested, take off a hit point 
		 */
		if (on(player, HASINFEST)) {
		    if ((pstats.s_hpt -= infest_dam) <= 0) death(D_INFESTATION);
		}

		/*
		 * The eye of Vecna is a constant drain on the player
		 */
		if (cur_relic[EYE_VECNA]) {
		    if ((--pstats.s_hpt) <= 0) death(D_RELIC);
		}

		/* 
		 * if player has body rot then take off five hits 
		 */
		if (on(player, DOROT)) {
		     if ((pstats.s_hpt -= 5) <= 0) death(D_ROT);
		}
		do_daemons(AFTER);
		do_fuses(AFTER);
	    }
	} while (after == FALSE);

	/* Make the monsters go */
	if (--monst_current <= 0)
	    monst_current = monst_limit = runners(monst_limit);

#ifdef NEEDLOOK
	/* Shall we take a look? */
	if (player.t_no_move <= 1 &&
	    !((running || count) && jump))
	    look(FALSE, FALSE);
#endif

	if (++segment > 10) segment = 1;
	t_free_list(monst_dead);
    }
}

/*
 * display
 * 	tell the player what is at a certain coordinates assuming
 *	it can be seen.
 */
display()
{
    coord c;
    struct linked_list *item;
    struct thing *tp;
    int what;

    msg("What do you want to display (* for help)?");
    c = get_coordinates();
    mpos = 0;
    if (!cansee(c.y, c.x)) {
	msg("You can't see what is there.");
	return;
    }
    what = mvwinch(cw, c.y, c.x);
    if (isalpha(what)) {
	item = find_mons(c.y, c.x);
	tp = THINGPTR(item);
	msg("%s", monster_name(tp));
	return;
    }
    if ((item = find_obj(c.y, c.x)) != NULL) {
	msg("%s", inv_name(OBJPTR(item), FALSE));
	return;
    }
    identify(what);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */

void
quit(sig)
int sig;
{
    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (signal(SIGINT, quit) != quit)
	mpos = 0;
    msg("Really quit? <yes or no> ");
    draw(cw);
    prbuf[0] = '\0';
    if ((get_str(prbuf, msgw) == NORM) && strcmp(prbuf, "yes") == 0) {
	clear();
	move(lines-1, 0);
	draw(stdscr);
	score(pstats.s_exp + (long) purse, CHICKEN, 0);
#ifdef PC7300
	endhardwin();
#endif
	exit(0);
    }
    else
    {
	signal(SIGINT, quit);
	wmove(cw, 0, 0);
	wclrtoeol(cw);
	status(FALSE);
	draw(cw);
	mpos = 0;
	count = 0;
	running = FALSE;
    }
}

/*
 * bugkill:
 *	killed by a program bug instead of voluntarily.
 */

void
bugkill(sig)
int sig;
{
    signal(sig, quit);	/* If we get it again, give up */
    death(D_SIGNAL);	/* Killed by a bug */
}


/*
 * search:
 *	Player gropes about him to find hidden things.
 */

search(is_thief, door_chime)
register bool is_thief, door_chime;
{
    register int x, y;
    register char ch,	/* The trap or door character */
		 sch,	/* Trap or door character (as seen on screen) */
		 mch;	/* Monster, if a monster is on the trap or door */
    register struct linked_list *item;
    register struct thing *mp; /* Status on surrounding monster */

    /*
     * Look all around the hero, if there is something hidden there,
     * give him a chance to find it.  If its found, display it.
     */
    if (on(player, ISBLIND))
	return;
    for (x = hero.x - 1; x <= hero.x + 1; x++)
	for (y = hero.y - 1; y <= hero.y + 1; y++)
	{
	    if (y==hero.y && x==hero.x)
		continue;

	    /* Mch and ch will be the same unless there is a monster here */
	    mch = CCHAR( winat(y, x) );
	    ch = CCHAR( mvwinch(stdscr, y, x) );
	    sch = CCHAR( mvwinch(cw, y, x) );	/* What's on the screen */

	    if (door_chime == FALSE && isatrap(ch)) {
		    register struct trap *tp;

		    /* Is there a monster on the trap? */
		    if (mch != ch && (item = find_mons(y, x)) != NULL) {
			mp = THINGPTR(item);
			if (sch == mch) sch = mp->t_oldch;
		    }
		    else mp = NULL;

		    /* 
		     * is this one found already?
		     */
		    if (isatrap(sch)) 
			continue;	/* give him chance for other traps */
		    tp = trap_at(y, x);
		    /* 
		     * if the thief set it then don't display it.
		     * if its not a thief he has 50/50 shot
		     */
		    if((tp->tr_flags&ISTHIEFSET) || (!is_thief && rnd(100)>50))
			continue;	/* give him chance for other traps */
		    tp->tr_flags |= ISFOUND;

		    /* Let's update the screen */
		    if (mp != NULL && mvwinch(cw, y, x) == mch)
			mp->t_oldch = ch; /* Will change when monst moves */
		    else mvwaddch(cw, y, x, ch);

		    count = 0;
		    running = FALSE;

		    /* Stop what we were doing */
		    player.t_no_move = movement(&player);
		    player.t_action = A_NIL;
		    player.t_using = NULL;

		    if (fight_flush) md_flushinp();
		    msg(tr_name(tp->tr_type));
	    }
	    else if (ch == SECRETDOOR) {
		if (door_chime == TRUE || (!is_thief && rnd(100) < 20)) {
		    struct room *rp;
		    coord cp;

		    /* Is there a monster on the door? */
		    if (mch != ch && (item = find_mons(y, x)) != NULL) {
			mp = THINGPTR(item);

			/* Screen will change when monster moves */
			if (sch == mch) mp->t_oldch = ch;
		    }
		    mvaddch(y, x, DOOR);
		    count = 0;
		    /*
		     * if its the entrance to a treasure room, wake it up
		     */
		    cp.y = y;
		    cp.x = x;
		    rp = roomin(&cp);
		    if (rp->r_flags & ISTREAS)
			wake_room(rp);

		    /* Make sure we don't shoot into the room */
		    if (door_chime == FALSE) {
			count = 0;
			running = FALSE;

			/* Stop what we were doing */
			player.t_no_move = movement(&player);
			player.t_action = A_NIL;
			player.t_using = NULL;
		    }
		}
	    }
	}
}


/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */

help()
{
    register struct h_list *strp = helpstr;
#ifdef WIZARD
    struct h_list *wizp = wiz_help;
#endif
    register char helpch;
    register int cnt;

    msg("Character you want help for (* for all): ");
    helpch = readchar();
    mpos = 0;
    /*
     * If its not a *, print the right help string
     * or an error if he typed a funny character.
     */
    if (helpch != '*') {
	wmove(msgw, 0, 0);
	while (strp->h_ch) {
	    if (strp->h_ch == helpch) {
		msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
		return;
	    }
	    strp++;
	}
#ifdef WIZARD
	if (wizard) {
	    while (wizp->h_ch) {
		if (wizp->h_ch == helpch) {
		    msg("%s%s", unctrl(wizp->h_ch), wizp->h_desc);
		    return;
		}
		wizp++;
	    }
	}
#endif

	msg("Unknown character '%s'", unctrl(helpch));
	return;
    }
    /*
     * Here we print help for everything.
     * Then wait before we return to command mode
     */
    wclear(hw);
    cnt = 0;
    while (strp->h_ch) {
	mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(strp->h_ch));
	waddstr(hw, strp->h_desc);
	strp++;
	if (++cnt >= 46 && strp->h_ch) {
	    wmove(hw, lines-1, 0);
	    wprintw(hw, morestr);
	    draw(hw);
	    wait_for(' ');
	    wclear(hw);
	    cnt = 0;
	}
    }
#ifdef WIZARD
    if (wizard) {
	while (wizp->h_ch) {
	    mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(wizp->h_ch));
	    waddstr(hw, wizp->h_desc);
	    wizp++;
	    if (++cnt >= 46 && wizp->h_ch) {
		wmove(hw, lines-1, 0);
		wprintw(hw, morestr);
		draw(hw);
		wait_for(' ');
		wclear(hw);
		cnt = 0;
	    }
	}
    }
#endif
    wmove(hw, lines-1, 0);
    wprintw(hw, spacemsg);
    draw(hw);
    wait_for(' ');
    wclear(hw);
    draw(hw);
    wmove(cw, 0, 0);
    wclrtoeol(cw);
    status(FALSE);
    touchwin(cw);
}
/*
 * identify:
 *	Tell the player what a certain thing is.
 */

identify(ch)
register char ch;
{
    register char *str;

    if (ch == 0) {
	msg("What do you want identified? ");
	ch = readchar();
	mpos = 0;
	if (ch == ESCAPE)
	{
	    msg("");
	    return;
	}
    }
    if (isalpha(ch))
	str = monsters[id_monst(ch)].m_name;
    else switch(ch)
    {
	case '|':
	case '-':
	    str = (levtype == OUTSIDE) ? "boundary of sector"
				       : "wall of a room";
	when GOLD:	str = "gold";
	when STAIRS :	str = (levtype == OUTSIDE) ? "entrance to a dungeon"
						   : "passage leading down";
	when DOOR:	str = "door";
	when FLOOR:	str = (levtype == OUTSIDE) ? "meadow" : "room floor";
	when VPLAYER:	str = "the hero of the game ---> you";
	when IPLAYER:	str = "you (but invisible)";
	when PASSAGE:	str = "passage";
	when POST:	str = "trading post";
	when POOL:	str = (levtype == OUTSIDE) ? "lake"
						   : "a shimmering pool";
	when TRAPDOOR:	str = "trapdoor";
	when ARROWTRAP:	str = "arrow trap";
	when SLEEPTRAP:	str = "sleeping gas trap";
	when BEARTRAP:	str = "bear trap";
	when TELTRAP:	str = "teleport trap";
	when DARTTRAP:	str = "dart trap";
	when MAZETRAP:	str = "entrance to a maze";
	when FOREST:	str = "forest";
	when POTION:	str = "potion";
	when SCROLL:	str = "scroll";
	when FOOD:	str = "food";
	when WEAPON:	str = "weapon";
	when ' ' :	str = "solid rock";
	when ARMOR:	str = "armor";
	when MM:	str = "miscellaneous magic";
	when RING:	str = "ring";
	when STICK:	str = "wand or staff";
	when SECRETDOOR:str = "secret door";
	when RELIC:	str = "artifact";
	otherwise:	str = "unknown character";
    }
    msg("'%s' : %s", unctrl(ch), str);
}

/*
 * d_level:
 *	He wants to go down a level
 */

d_level()
{
    bool no_phase=FALSE;
    char position = CCHAR( winat(hero.y, hero.x) );


    /* If we are on a trading post, go to a trading post level. */
    if (position == POST) {
	new_level(POSTLEV);
	return;
    }

    /* If we are at a top-level trading post, we probably can't go down */
    if (levtype == POSTLEV && level == 0 && position != STAIRS) {
	msg("I see no way down.");
	return;
    }

    if (winat(hero.y, hero.x) != STAIRS) {
	if (off(player, CANINWALL) ||	/* Must use stairs if can't phase */
	    (levtype == OUTSIDE && rnd(100) < 90)) {
	    msg("I see no way down.");
	    return;
	}

	/* Is there any dungeon left below? */
	if (level >= nfloors) {
	    msg("There is only solid rock below.");
	    return;
	}

	extinguish(unphase);	/* Using phase to go down gets rid of it */
	no_phase = TRUE;
    }

    /* Is this the bottom? */
    if (level >= nfloors) {
	msg("The stairway only goes up.");
	return;
    }

    level++;
    new_level(NORMLEV);
    if (no_phase) unphase();
}

/*
 * u_level:
 *	He wants to go up a level
 */

u_level()
{
    bool no_phase = FALSE;
    register struct linked_list *item;
    struct thing *tp;
    struct object *obj;

    if (winat(hero.y, hero.x) != STAIRS) {
	if (off(player, CANINWALL)) {	/* Must use stairs if can't phase */
	    msg("I see no way up.");
	    return;
	}

	extinguish(unphase);
	no_phase = TRUE;
    }

    if (level == 0) {
	msg("The stairway only goes down.");
	return;
    }

    /*
     * does he have the item he was quested to get?
     */
    if (level == 1) {
	for (item = pack; item != NULL; item = next(item)) {
	    obj = OBJPTR(item);
	    if (obj->o_type == RELIC && obj->o_which == quest_item)
		total_winner();
	}
    }
    /*
     * check to see if he trapped a UNIQUE, If he did then put it back
     * in the monster table for next time
     */
    for (item = tlist; item != NULL; item = next(item)) {
	tp = THINGPTR(item);
	if (on(*tp, ISUNIQUE)) 
	    monsters[tp->t_index].m_normal = TRUE;
    }
    t_free_list(tlist);	/* Monsters that fell below are long gone! */

    if (levtype != POSTLEV) level--;
    if (level > 0) new_level(NORMLEV);
    else {
	level = -1;	/* Indicate that we are new to the outside */
	msg("You emerge into the %s", daytime ? "light" : "night");
	new_level(OUTSIDE); 	/* Leaving the dungeon */
    }

    if (no_phase) unphase();
}

/*
 * Let him escape for a while
 */

shell()
{
    register char *sh;

    /*
     * Set the terminal back to original mode
     */
    sh = getenv("SHELL");
    wclear(hw);
    wmove(hw, lines-1, 0);
    draw(hw);
    endwin();
    in_shell = TRUE;
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    
    md_shellescape();

	printf(retstr);
	noecho();
	raw();
	keypad(cw,1);
	keypad(msgw,1);
	in_shell = FALSE;
	wait_for('\n');
	clearok(cw, TRUE);
	touchwin(cw);
}

/*
 * see what we want to name -- an item or a monster.
 */
nameit()
{
    char answer;

    msg("Name monster or item (m or i)? ");
    answer = readchar();
    mpos = 0;

    while (answer != 'm' && answer != 'i' && answer != ESCAPE) {
	mpos = 0;
	msg("Please specify m or i, for monster or item - ");
	answer = readchar();
    }

    switch (answer) {
	case 'm': namemonst();
	when 'i': nameitem(NULL, FALSE);
    }
}

/*
 * allow a user to call a potion, scroll, or ring something
 */
nameitem(item, mark)
struct linked_list *item;
bool mark;
{
    register struct object *obj;
    register char **guess, *elsewise;
    register bool *know;

    if (item == NULL) {
	if (mark) item = get_item(pack, "mark", ALL, FALSE, FALSE);
	else	  item = get_item(pack, "name", CALLABLE, FALSE, FALSE);
	if (item == NULL) return;
    }
    /*
     * Make certain that it is somethings that we want to wear
     */
    obj = OBJPTR(item);
    switch (obj->o_type)
    {
	case RING:
	    guess = r_guess;
	    know = r_know;
	    elsewise = (r_guess[obj->o_which] != NULL ?
			r_guess[obj->o_which] : r_stones[obj->o_which]);
	when POTION:
	    guess = p_guess;
	    know = p_know;
	    elsewise = (p_guess[obj->o_which] != NULL ?
			p_guess[obj->o_which] : p_colors[obj->o_which]);
	when SCROLL:
	    guess = s_guess;
	    know = s_know;
	    elsewise = (s_guess[obj->o_which] != NULL ?
			s_guess[obj->o_which] : s_names[obj->o_which]);
	when STICK:
	    guess = ws_guess;
	    know = ws_know;
	    elsewise = (ws_guess[obj->o_which] != NULL ?
			ws_guess[obj->o_which] : ws_made[obj->o_which]);
	when MM:
	    guess = m_guess;
	    know = m_know;
	    elsewise = (m_guess[obj->o_which] != NULL ?
			m_guess[obj->o_which] : "nothing");
	otherwise:
	    if (!mark) {
		msg("You can't call that anything.");
		return;
	    }
	    else know = (bool *) 0;
    }
    if ((obj->o_flags & ISPOST)	|| (know && know[obj->o_which]) && !mark) {
	msg("That has already been identified.");
	return;
    }
    if (mark) {
	if (obj->o_mark[0]) {
	    addmsg(terse ? "M" : "Was m");
	    msg("arked \"%s\"", obj->o_mark);
	}
	msg(terse ? "Mark it: " : "What do you want to mark it? ");
	prbuf[0] = '\0';
    }
    else {
	if (elsewise) {
	    addmsg(terse ? "N" : "Was n");
	    msg("amed \"%s\"", elsewise);
	    strcpy(prbuf, elsewise);
	}
	else prbuf[0] = '\0';
	msg(terse ? "Name it: " : "What do you want to name it? ");
    }
    if (get_str(prbuf, msgw) == NORM) {
	if (mark) {
	    strncpy(obj->o_mark, prbuf, MARKLEN-1);
	    obj->o_mark[MARKLEN-1] = '\0';
	}
	else if (prbuf[0] != '\0') {
	    if (guess[obj->o_which] != NULL)
		free(guess[obj->o_which]);
	    guess[obj->o_which] = new((unsigned int) strlen(prbuf) + 1);
	    strcpy(guess[obj->o_which], prbuf);
	}
    }
}

/* Name a monster */

namemonst()
{
    register struct thing *tp;
    struct linked_list *item;
    coord c;

    /* Find the monster */
    msg("Choose the monster (* for help)");
    c = get_coordinates();

    /* Make sure we can see it and that it is a monster. */
    mpos = 0;
    if (!cansee(c.y, c.x)) {
	msg("You can't see what is there.");
	return;
    }
    
    if (isalpha(mvwinch(cw, c.y, c.x))) {
	item = find_mons(c.y, c.x);
	if (item != NULL) {
	    tp = THINGPTR(item);
	    if (tp->t_name == NULL)
		strcpy(prbuf, monsters[tp->t_index].m_name);
	    else
		strcpy(prbuf, tp->t_name);

	    addmsg(terse ? "N" : "Was n");
	    msg("amed \"%s\"", prbuf);
	    msg(terse ? "Name it: " : "What do you want to name it? ");

	    if (get_str(prbuf, msgw) == NORM) {
		if (prbuf[0] != '\0') {
		    if (tp->t_name != NULL)
			free(tp->t_name);
		    tp->t_name = new((unsigned int) strlen(prbuf) + 1);
		    strcpy(tp->t_name, prbuf);
		}
	    }
	    return;
	}
    }

    msg("There is no monster there to name.");
}

count_gold()
{
	if (player.t_action != C_COUNT) {
	    msg("You take a break to count your money...");
	    player.t_using = NULL;
	    player.t_action = C_COUNT;	/* We are counting */
	    player.t_no_move = (purse/300 + 1) * movement(&player);
	    return;
	}
	if (purse > 100)
		msg("You think you have about %d gold pieces.", purse);
	else
		msg("You have %d gold pieces.", purse);
	player.t_action = A_NIL;
}
