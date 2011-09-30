/*
 * player.c  -  This file contains functions for dealing with special player 
 * abilities
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
 * This file contains functions for dealing with special player abilities
 */

#include <ctype.h>
#include "curses.h"
#include "rogue.h"
#ifdef PC7300
#include "menu.h"
#endif


/*
 * affect:
 *	cleric affecting undead
 */

affect()
{
    register struct linked_list *item;
    register struct thing *tp;
    register char *mname;
    bool see;
    coord new_pos;
    int lvl;

    if (!(player.t_ctype == C_CLERIC  ||
          (player.t_ctype == C_PALADIN && pstats.s_lvl > 4) ||
	  cur_relic[HEIL_ANKH] != 0)) {
	msg("You cannot affect undead.");
	return;
    }

    new_pos.y = hero.y + player.t_newpos.y;
    new_pos.x = hero.x + player.t_newpos.x;

    if (cansee(new_pos.y, new_pos.x)) see = TRUE;
    else see = FALSE;

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > lines-3 ||
	new_pos.x < 0 || new_pos.x > cols-1 ||
	mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
	msg("Nothing to affect.");
	return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == NULL) {
	debug("Affect what @ %d,%d?", new_pos.y, new_pos.x);
	return;
    }
    tp = THINGPTR(item);
    mname = monster_name(tp);

    if (on(player, ISINVIS) && off(*tp, CANSEE)) {
	msg("%s%s cannot see you", see ? "The " : "It",
	    see ? mname : "");
	return;
    }

    if (off(*tp, TURNABLE) || on(*tp, WASTURNED)) 
	goto annoy;
    turn_off(*tp, TURNABLE);

    lvl = pstats.s_lvl;
    if (player.t_ctype == C_PALADIN && cur_relic[HEIL_ANKH] == 0) {
	lvl -= 4;
    }
    /* Can cleric kill it? */
    if (lvl >= 3 * tp->t_stats.s_lvl) {
	unsigned long test;	/* For overflow check */

	msg("You have destroyed %s%s.", see ? "the " : "it", see ? mname : "");
	test = pstats.s_exp + tp->t_stats.s_exp;

	/* Be sure there is no overflow before increasing experience */
	if (test > pstats.s_exp) pstats.s_exp = test;
	killed(item, FALSE, TRUE, TRUE);
	check_level();
	return;
    }

    /* Can cleric turn it? */
    if (rnd(100) + 1 >
	 (100 * ((2 * tp->t_stats.s_lvl) - lvl)) / lvl) {
	unsigned long test;	/* Overflow test */

	/* Make the monster flee */
	turn_on(*tp, WASTURNED);	/* No more fleeing after this */
	turn_on(*tp, ISFLEE);
	runto(tp, &hero);

	/* Disrupt it */
	dsrpt_monster(tp, TRUE, TRUE);

	/* Let player know */
	msg("You have turned %s%s.", see ? "the " : "it", see ? mname : "");

	/* get points for turning monster -- but check overflow first */
	test = pstats.s_exp + tp->t_stats.s_exp/2;
	if (test > pstats.s_exp) pstats.s_exp = test;
	check_level();

	/* If monster was suffocating, stop it */
	if (on(*tp, DIDSUFFOCATE)) {
	    turn_off(*tp, DIDSUFFOCATE);
	    extinguish(suffocate);
	}

	/* If monster held us, stop it */
	if (on(*tp, DIDHOLD) && (--hold_count == 0))
		turn_off(player, ISHELD);
	turn_off(*tp, DIDHOLD);

	/* It is okay to turn tail */
	tp->t_oldpos = tp->t_pos;

	return;
    }

    /* Otherwise -- no go */
annoy:
    if (see && tp->t_stats.s_intel > 16)
	msg("%s laughs at you...", prname(mname, TRUE));
    else
	msg("You do not affect %s%s.", see ? "the " : "it", see ? mname : "");

    /* Annoy monster */
   if (off(*tp, ISFLEE)) runto(tp, &hero);
}

/*
 * the magic user is going to try and cast a spell
 */
cast()
{
    int		spell_ability,
		which_spell,
		num_spells;

    if (player.t_ctype != C_MAGICIAN && player.t_ctype != C_RANGER) {
	msg("You are not permitted to cast spells.");
	return;
    }
    spell_ability = pstats.s_lvl * pstats.s_intel;
    if (player.t_ctype != C_MAGICIAN)
	spell_ability /= 3;

    if (player.t_action != C_CAST) {
	/* 
	 * Get the number of avilable spells 
	 */
	num_spells = 0;
	if (pstats.s_intel >= 16) 
	    num_spells += pstats.s_intel - 15;
	num_spells += pstats.s_lvl;
	if (player.t_ctype != C_MAGICIAN) 
	    num_spells /= 3;
	if (num_spells > MAXSPELLS)	  
	    num_spells = MAXSPELLS;
	if (num_spells < 1) {
	    msg("You are not allowed to cast spells yet.");
	    return;
	}
	if (pick_spell(	magic_spells, 
			spell_ability, 
			num_spells, 
			spell_power,
			"cast",
			"spell"))
	    player.t_action = C_CAST;
	return;
    }

    /* We've waited our required casting time. */
    which_spell = player.t_selection;
    player.t_selection = 0;
    player.t_using = NULL;
    player.t_action = A_NIL;

    if ((spell_power + magic_spells[which_spell].s_cost) > spell_ability) {
	msg("Your attempt fails.");
	return;
    }

    msg("Your spell is successful.");

    if (magic_spells[which_spell].s_type == TYP_POTION)
        quaff(	magic_spells[which_spell].s_which,
		NULL,
        	magic_spells[which_spell].s_flag,
		FALSE);
    else if (magic_spells[which_spell].s_type == TYP_SCROLL)
        read_scroll(	magic_spells[which_spell].s_which,
        		magic_spells[which_spell].s_flag,
			FALSE);
    else if (magic_spells[which_spell].s_type == TYP_STICK) {
	 if (!player_zap(magic_spells[which_spell].s_which,
			 magic_spells[which_spell].s_flag)) {
	     after = FALSE;
	     return;
	 }
    }
    spell_power += magic_spells[which_spell].s_cost;
}

/* 
 * the druid asks his deity for a spell
 */
chant()
{
    register int	num_chants, 
			chant_ability, 
			which_chant;

    which_chant = num_chants = chant_ability = 0;

    if (player.t_ctype != C_DRUID && player.t_ctype != C_RANGER) {
	msg("You are not permitted to chant.");
	return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
	cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
	msg("You can't seem to chant!");
	return;
    }
    chant_ability = pstats.s_lvl * pstats.s_wisdom;
    if (player.t_ctype != C_DRUID)
	chant_ability /= 3;

    if (player.t_action != C_CHANT) {
	num_chants = 0;

	/* Get the number of avilable chants */
	if (pstats.s_wisdom > 16) 
	    num_chants = (pstats.s_wisdom - 15) / 2;

	num_chants += pstats.s_lvl;

	if (player.t_ctype != C_DRUID) 
	    num_chants /= 3;

	if (num_chants > MAXCHANTS) 
	    num_chants = MAXCHANTS;

	if (num_chants < 1) {
	    msg("You are not permitted to chant yet.");
	    return;
	}

	/* Prompt for chant */
	if (pick_spell(	druid_spells, 
			chant_ability, 
			num_chants, 
			chant_time,
			"sing",
			"chant"))
	    player.t_action = C_CHANT;

	return;
    }

    /* We've waited our required chanting time. */
    which_chant = player.t_selection;
    player.t_selection = 0;
    player.t_using = NULL;
    player.t_action = A_NIL;

    if (druid_spells[which_chant].s_cost + chant_time > chant_ability) {
	msg("Your chant fails.");
	return;
    }

    msg("Your chant has been granted.");

    if (druid_spells[which_chant].s_type == TYP_POTION)
	quaff(		druid_spells[which_chant].s_which,
			NULL,
			druid_spells[which_chant].s_flag,
			FALSE);
    else if (druid_spells[which_chant].s_type == TYP_SCROLL)
	read_scroll(	druid_spells[which_chant].s_which,
			druid_spells[which_chant].s_flag,
			FALSE);
    else if (druid_spells[which_chant].s_type == TYP_STICK) {
	 if (!player_zap(druid_spells[which_chant].s_which,
			 druid_spells[which_chant].s_flag)) {
	     after = FALSE;
	     return;
	 }
    }
    chant_time += druid_spells[which_chant].s_cost;
}

/* Constitution bonus */

const_bonus()	/* Hit point adjustment for changing levels */
{
    register int bonus;
    if (pstats.s_const > 6 && pstats.s_const <= 14) 
	bonus = 0;
    else if (pstats.s_const > 14) 
	bonus = pstats.s_const-14;
    else if (pstats.s_const > 3) 
	bonus = -1;
    else
	bonus = -2;
    switch(player.t_ctype) {
	case C_FIGHTER:		bonus = min(bonus, 11);
	when C_MAGICIAN:	bonus = min(bonus, 4);
	when C_CLERIC:		bonus = min(bonus, 4);
	when C_THIEF:		bonus = min(bonus, 4);
	when C_RANGER:		bonus = min(bonus, 6);
	when C_PALADIN:		bonus = min(bonus, 8);
	when C_ASSASIN:		bonus = min(bonus, 4);
	when C_MONK:		bonus = min(bonus, 6);
	when C_DRUID:		bonus = min(bonus, 4);
	otherwise:		bonus = min(bonus, 4);
    }
    return(bonus);
}


/* Routines for thieves */

/*
 * gsense:
 *	Sense gold
 */

gsense()
{
    /* Only thieves can do this */
    if (player.t_ctype != C_THIEF && player.t_ctype != C_ASSASIN) {
	msg("You seem to have no gold sense.");
	return;
    }

    read_scroll(S_GFIND, NULL, FALSE);
}

/* 
 * the cleric asks his deity for a spell
 */
pray()
{
    register int	num_prayers, 
			prayer_ability, 
			which_prayer;

    which_prayer = num_prayers = prayer_ability =  0;

    if (player.t_ctype != C_CLERIC  && 
	player.t_ctype != C_PALADIN &&
	cur_relic[HEIL_ANKH] == 0) {
	    msg("You are not permitted to pray.");
	    return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
	cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
	msg("You can't seem to pray!");
	return;
    }

    prayer_ability = pstats.s_lvl * pstats.s_wisdom;
    if (player.t_ctype != C_CLERIC)
	prayer_ability /= 3;

    if (cur_relic[HEIL_ANKH]) prayer_ability *= 2;

    if (player.t_action != C_PRAY) {
	num_prayers = 0;

	/* Get the number of avilable prayers */
	if (pstats.s_wisdom > 16) 
	    num_prayers += (pstats.s_wisdom - 15) / 2;

	num_prayers += pstats.s_lvl;
	if (cur_relic[HEIL_ANKH]) num_prayers += 3;

	if (player.t_ctype != C_CLERIC) 
	    num_prayers /= 3;

	if (num_prayers > MAXPRAYERS) 
	    num_prayers = MAXPRAYERS;
	if (num_prayers < 1) {
	    msg("You are not permitted to pray yet.");
	    return;
	}

	/* Prompt for prayer */
	if (pick_spell(	cleric_spells, 
			prayer_ability, 
			num_prayers, 
			pray_time,
			"offer",
			"prayer"))
	    player.t_action = C_PRAY;

	return;
    }

    /* We've waited our required praying time. */
    which_prayer = player.t_selection;
    player.t_selection = 0;
    player.t_using = NULL;
    player.t_action = A_NIL;

    if (cleric_spells[which_prayer].s_cost + pray_time > prayer_ability) {
	msg("Your prayer fails.");
	return;
    }

    msg("Your prayer has been granted.");

    if (cleric_spells[which_prayer].s_type == TYP_POTION)
	quaff(		cleric_spells[which_prayer].s_which,
			NULL,
			cleric_spells[which_prayer].s_flag,
			FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_SCROLL)
	read_scroll(	cleric_spells[which_prayer].s_which,
			cleric_spells[which_prayer].s_flag,
			FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_STICK) {
	 if (!player_zap(cleric_spells[which_prayer].s_which,
			 cleric_spells[which_prayer].s_flag)) {
	     after = FALSE;
	     return;
	 }
    }
    pray_time += cleric_spells[which_prayer].s_cost;
}



/*
 * steal:
 *	Steal in direction given in delta
 */

steal()
{
    register struct linked_list *item;
    register struct thing *tp;
    register char *mname;
    coord new_pos;
    int thief_bonus = -50;
    bool isinvisible = FALSE;

    if (player.t_ctype != C_THIEF && player.t_ctype != C_ASSASIN) {
	msg("Only thieves and assassins can steal.");
	return;
    }
    if (on(player, ISBLIND)) {
	msg("You can't see anything.");
	return;
    }

    new_pos.y = hero.y + player.t_newpos.y;
    new_pos.x = hero.x + player.t_newpos.x;

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > lines-3 ||
	new_pos.x < 0 || new_pos.x > cols-1 ||
	mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
	msg("Nothing to steal from.");
	return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == NULL)
	debug("Steal from what @ %d,%d?", new_pos.y, new_pos.x);
    tp = THINGPTR(item);
    if (on(*tp, ISSTONE)) {
	msg ("You can't steal from stone!");
	return;
    }
    if (isinvisible = invisible(tp)) mname = "creature";
    else mname = monster_name(tp);

    /* Can player steal something unnoticed? */
    if (player.t_ctype == C_THIEF) thief_bonus = 10;
    if (player.t_ctype == C_ASSASIN) thief_bonus = 0;
    if (on(*tp, ISUNIQUE)) thief_bonus -= 15;
    if (isinvisible) thief_bonus -= 20;
    if (on(*tp, ISINWALL) && off(player, CANINWALL)) thief_bonus -= 50;

    if (on(*tp, ISHELD) || tp->t_action == A_FREEZE ||
	rnd(100) <
	(thief_bonus + 2*dex_compute() + 5*pstats.s_lvl -
	 5*(tp->t_stats.s_lvl - 3))) {
	register struct linked_list *s_item, *pack_ptr;
	int count = 0;
	unsigned long test;	/* Overflow check */

	s_item = NULL;	/* Start stolen goods out as nothing */

	/* Find a good item to take */
	for (pack_ptr=tp->t_pack; pack_ptr != NULL; pack_ptr=next(pack_ptr))
	    if ((OBJPTR(pack_ptr))->o_type != RELIC &&
		pack_ptr != tp->t_using &&  /* Monster can't be using it */
		rnd(++count) == 0)
		s_item = pack_ptr;

	/* 
	 * Find anything?
	 */
	if (s_item == NULL) {
	    msg("%s apparently has nothing to steal.", prname(mname, TRUE));
	    return;
	}

	/* Take it from monster */
	if (tp->t_pack) detach(tp->t_pack, s_item);

	/* Recalculate the monster's encumberance */
	updpack(TRUE, tp);

	/* Give it to player */
	if (add_pack(s_item, FALSE, NULL) == FALSE) {
	   (OBJPTR(s_item))->o_pos = hero;
	   fall(s_item, TRUE);
	}

	/* Get points for stealing -- but first check for overflow */
	test = pstats.s_exp + tp->t_stats.s_exp/2;
	if (test > pstats.s_exp) pstats.s_exp = test;

	/*
	 * Do adjustments if player went up a level
	 */
	check_level();
    }

    else {
	msg("Your attempt fails.");

	/* Annoy monster (maybe) */
	if (rnd(35) >= dex_compute() + thief_bonus) {
	    /*
	     * If this is a charmed creature, there is a chance it
	     * will become uncharmed.
	     */
	    if (on(*tp, ISCHARMED) && save(VS_MAGIC, tp, 0)) {
		msg("The eyes of %s turn clear.", prname(mname, FALSE));
		turn_off(*tp, ISCHARMED);
	    }
	    if (on(*tp, CANSELL)) {
		turn_off(*tp, CANSELL);
		tp->t_action = A_NIL;
		tp->t_movement = 0;
		if (rnd(100) < 50) /* make him steal something */
		    turn_on(*tp, STEALMAGIC);
		else
		    turn_on(*tp, STEALGOLD);
		if (!isinvisible)
		    msg("%s looks insulted.", prname(mname, TRUE));
	    }
	    runto(tp, &hero);
	}
    }
}

#ifdef PC7300
/* Use MAXSPELLS or whatever is the biggest number of spells/prayers/etc */
static menu_t Display;				/* The menu structure */
static mitem_t Dispitems[MAXSPELLS+1];		/* Info for each line */
static char Displines[MAXSPELLS+1][LINELEN+1];	/* The lines themselves */
#endif

/*
 * this routine lets the player pick the spell that they
 * want to cast regardless of character class
 */
pick_spell(spells, ability, num_spells, power, prompt, type)
struct spells	spells[];	/* spell list				 */
int		ability;	/* spell ability			 */
int		num_spells;	/* number of spells that can be cast	 */
int		power;		/* spell power				 */
char		*prompt;	/* prompt for spell list		 */
char		*type;		/* type of thing--> spell, prayer, chant */
{
    bool		nohw = FALSE;
    register int	i;
    int			curlen,
			maxlen,
			dummy,
			which_spell,
			spell_left;
#ifdef PC7300
    char label[LINELEN],	/* For menu label */
	 title[LINELEN];	/* For menu title */
#endif

    if (cur_misc[WEAR_CLOAK] != NULL &&
	cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
	msg("You can't seem to start a %s!", type);
	return(FALSE);
    }

    /* Prompt for spells */
    msg("Which %s are you %sing? (* for list): ", type, prompt);

    which_spell = (int) (readchar() - 'a');
    msg("");	/* Get rid of the prompt */
    if (which_spell == (int) ESCAPE - (int) 'a') {
	after = FALSE;
	return(FALSE);
    }
    if (which_spell >= 0 && which_spell < num_spells) nohw = TRUE;

    else if (slow_invent) {
	register char c;

	nohw = TRUE;
	do {
	    for (i=0; i<num_spells; i++) {
		msg("");
		mvwaddch(msgw, 0, 0, '[');
		waddch(msgw, (char) ((int) 'a' + i));
		wprintw(msgw, "] A %s of ", type);
		if (spells[i].s_type == TYP_POTION)
		    waddstr(msgw, p_magic[spells[i].s_which].mi_name);
		else if (spells[i].s_type == TYP_SCROLL)
		    waddstr(msgw, s_magic[spells[i].s_which].mi_name);
		else if (spells[i].s_type == TYP_STICK)
		    waddstr(msgw, ws_magic[spells[i].s_which].mi_name);
		waddstr(msgw, morestr);
	        wclrtobot(msgw);
		clearok(msgw, FALSE);
		draw(msgw);
		do {
		    c = readchar();
		} while (c != ' ' && c != ESCAPE);
		if (c == ESCAPE)
		    break;
	    }
	    msg("");
	    wmove(msgw, 0, 0);
	    wprintw(msgw, "Which %s are you %sing? ", type, prompt);
	    clearok(msgw, FALSE);
	    draw(msgw);

	    which_spell = (int) (readchar() - 'a');
	} while (which_spell != (int) (ESCAPE - 'a') &&
		 (which_spell < 0 || which_spell >= num_spells));

	if (which_spell == (int) (ESCAPE - 'a')) {
	    mpos = 0;
	    msg("");
	    after = FALSE;
	    return(FALSE);
	}
    }
    else {
	/* Now display the possible spells */
	wclear(hw);
	touchwin(hw);
	wmove(hw, 2, 0);
	*type = toupper(*type);
	wprintw(hw, "	Cost	%s", type);
	*type = tolower(*type);
	mvwaddstr(hw, 3, 0,
		"-----------------------------------------------");
	maxlen = 47;	/* Maximum width of header */

	for (i=0; i<num_spells; i++) {
	    sprintf(prbuf, "[%c]	%3d	A %s of ",
			(char) ((int) 'a' + i), spells[i].s_cost, type);
	    if (spells[i].s_type == TYP_POTION)
		strcat(prbuf, p_magic[spells[i].s_which].mi_name);
	    else if (spells[i].s_type == TYP_SCROLL)
		strcat(prbuf, s_magic[spells[i].s_which].mi_name);
	    else if (spells[i].s_type == TYP_STICK)
		strcat(prbuf, ws_magic[spells[i].s_which].mi_name);
	    mvwaddstr(hw, i+4, 0, prbuf);

	    /* Get the length of the line */
	    getyx(hw, dummy, curlen);
	    if (maxlen < curlen) maxlen = curlen;

#ifdef PC7300
	    /* Put it into the PC menu display */
	    strcpy(Displines[i], prbuf);
	    Dispitems[i].mi_name = Displines[i];
	    Dispitems[i].mi_flags = 0;
	    Dispitems[i].mi_val = i;
#endif
	}

	spell_left = ability - power;
	if (spell_left < 0) {
	    spell_left = 0;
	    if (spell_left < -20) power = ability + 20;
	}
	sprintf(prbuf, "[Current %s power = %d]", type, spell_left);

	mvwaddstr(hw, 0, 0, prbuf);
	wprintw(hw, " Which %s are you %sing? ", type, prompt);
	getyx(hw, dummy, curlen);
	if (maxlen < curlen) maxlen = curlen;

#ifdef PC7300
	/* Place an end marker for the items */
	Dispitems[num_spells].mi_name = 0;

	/* Design prompts */
	sprintf(label, "Current %s power is %d", type, spell_left);
	*type = toupper(*type);
	sprintf(title, "	Cost	%s", type);
	*type = tolower(*type);
	sprintf(prbuf, "Select a %s or press Cancl to continue.", type);

	/* Set up the main menu structure */
	Display.m_label = label;
	Display.m_title = title;
	Display.m_prompt = prbuf;
	Display.m_curptr = '\0';
	Display.m_markptr = '\0';
	Display.m_flags = M_ASISTITLE;
	Display.m_selcnt = 1;
	Display.m_items = Dispitems;
	Display.m_curi = 0;

	/*
	 * Try to display the menu.  If we don't have a local terminal,
	 * the call will fail and we will just continue with the
	 * normal mode.
	 */
	if (menu(&Display) >= 0) {
	    if (Display.m_selcnt == 0) {
		/* Menu was cancelled */
		after = FALSE;
		return FALSE;	/* all done if abort */
	    }
	    else which_spell = (int) Display.m_curi->mi_val;
	    goto got_spell;
	}
#endif
	/* Should we overlay? */
	if (menu_overlay && num_spells + 3 < lines / 2) {
	    over_win(cw, hw, num_spells + 5, maxlen + 3, 0, curlen, NULL);
	}
	else draw(hw);
    }

    if (!nohw) {
	which_spell = (int) (readchar() - 'a');
	while (which_spell < 0 || which_spell >= num_spells) {
	    if (which_spell == (int) ESCAPE - (int) 'a') {
		after = FALSE;

		/* Restore the screen */
		touchwin(cw);
		if (num_spells + 3 < lines / 2) clearok(cw, FALSE);
		else clearok(cw, TRUE);
		return(FALSE);
	    }
	    wmove(hw, 0, 0);
	    wclrtoeol(hw);
	    wprintw(hw, "Please enter one of the listed %ss. ", type);
	    getyx(hw, dummy, curlen);
	    if (maxlen < curlen) maxlen = curlen;

	    /* Should we overlay? */
	    if (menu_overlay && num_spells + 3 < lines / 2) {
		over_win(cw, hw, num_spells + 5, maxlen + 3,
			    0, curlen, NULL);
	    }
	    else draw(hw);

	    which_spell = (int) (readchar() - 'a');
	}
    }

    /* Now restore the screen if we have to */
    if (!nohw) {
	touchwin(cw);
	if (num_spells + 3 < lines / 2) clearok(cw, FALSE);
	else clearok(cw, TRUE);
    }

#ifdef PC7300
got_spell:
#endif
    if (spells[which_spell].s_type == TYP_STICK && 
	need_dir(STICK, spells[which_spell].s_which)) {
	    if (!get_dir(&player.t_newpos)) {
		after = FALSE;
		return(FALSE);
	    }
    }
    player.t_selection = which_spell;
    player.t_using = NULL;
    player.t_no_move = (which_spell/3 + 1) * movement(&player);
    return(TRUE);
}
