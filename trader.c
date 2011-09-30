/*
 * trader.c - Anything to do with trading posts
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
 * Anything to do with trading posts
 */

#include "curses.h"
#include "rogue.h"





/*
 * buy_it:
 *	Buy the item on which the hero stands
 */
buy_it()
{
	reg int wh;
	struct linked_list *item;

	if (purse <= 0) {
	    msg("You have no money.");
	    return;
	}
	if (curprice < 0) {		/* if not yet priced */
	    wh = price_it();
	    if (!wh)			/* nothing to price */
		return;
	    msg("Do you want to buy it? ");
	    do {
		wh = tolower(readchar());
		if (wh == ESCAPE || wh == 'n') {
		    msg("");
		    return;
		}
	    } until(wh == 'y');
	}
	mpos = 0;
	if (curprice > purse) {
	    msg("You can't afford to buy that %s !",curpurch);
	    return;
	}
	/*
	 * See if the hero has done all his transacting
	 */
	if (!open_market())
	    return;
	/*
	 * The hero bought the item here
	 */
	item = find_obj(hero.y, hero.x);
	mpos = 0;
	if (add_pack(NULL, TRUE, &item)) {	/* try to put it in his pack */
	    purse -= curprice;		/* take his money */
	    ++trader;			/* another transaction */
	    trans_line();		/* show remaining deals */
	    curprice = -1;		/* reset stuff */
	    curpurch[0] = 0;
	    whatis (item);		/* identify it after purchase */
	    (OBJPTR(item))->o_flags &= ~ISPOST; /* turn off ISPOST */
	    msg("%s", inv_name(OBJPTR(item), TRUE));
	}
}

/*
 * do_post:
 *	Put a trading post room and stuff on the screen
 */
do_post(startup)
bool startup;	/* True if equipping the player at the beginning of the game */
{
	coord tp;
	reg int i, j, k;
	reg struct room *rp;
	reg struct object *op;
	reg struct linked_list *ll;

	o_free_list(lvl_obj);		/* throw old items away */

	for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
	    rp->r_flags = ISGONE;		/* kill all rooms */

	rp = &rooms[0];				/* point to only room */
	rp->r_flags = 0;			/* this room NOT gone */
	rp->r_max.x = 40;
	rp->r_max.y = 10;			/* 10 * 40 room */
	rp->r_pos.x = (cols - rp->r_max.x) / 2;	/* center horizontal */
	rp->r_pos.y = 1;			/* 2nd line */
	draw_room(rp);				/* draw the only room */

	/* Are we equipping the player? */
	if (startup) {
	    int wpt;

	    /*
	     * Give the rogue some weaponry.  
	     */
	    for (wpt=0; wpt<MAXWEAPONS; wpt++) {
		ll = spec_item(WEAPON, wpt, rnd(2), rnd(2)+1);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_flags |= (ISPOST | ISKNOW);
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /*
	     * And his suit of armor.......
	     * Thieves can only wear leather armor,
	     * so make sure some is available for them.
	     */
	    for (i=0; i<MAXARMORS; i++) {
		ll = spec_item(ARMOR, i, 0, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_flags |= (ISPOST | ISKNOW);
		op->o_weight = armors[i].a_wght;
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /* Now create some wands */
	    for (i=rnd(4)+2; i>0; i--) {
		switch (rnd(8)) {
		    case 0: j = WS_SLOW_M;
		    when 1: j = WS_TELMON;
		    when 2: j = WS_CONFMON;
		    when 3: j = WS_PARALYZE;
		    when 4: j = WS_MDEG;
		    when 5: j = WS_WONDER;
		    when 6: j = WS_FEAR;
		    when 7: j = WS_LIGHT;
		}
		ll = spec_item(STICK, j, 0, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);

		/* Let clerics and MU'S know what kind they are */
		switch (player.t_ctype) {
		    case C_MAGICIAN:
		    case C_CLERIC:
		    case C_DRUID:
			op->o_flags |= (ISPOST | ISKNOW);
		    otherwise:
			op->o_flags |= ISPOST;
		}
		fix_stick(op);
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /* Now let's make some rings */
	    for (i=rnd(4)+3; i>0; i--) {
		k = 0;
		switch (rnd(15)) {
		case 0: j = R_PROTECT;	k = roll(1,2);
		when 1: j = R_ADDSTR;	k = roll(1,3);
		when 2: j = R_ADDHIT;	k = roll(1,3);
		when 3: j = R_ADDDAM;	k = roll(1,3);
		when 4: j = R_DIGEST;	k = 1;
		when 5: j = R_ADDINTEL;	k = roll(1,3);
		when 6: j = R_ADDWISDOM;k = roll(1,3);
		when 7: j = R_SUSABILITY;
		when 8: j = R_SEEINVIS;
		when 9: j = R_ALERT;
		when 10:j = R_HEALTH;
		when 11:j = R_HEROISM;
		when 12:j = R_FIRE;
		when 13:j = R_WARMTH;
		when 14:j = R_FREEDOM;
		}
		ll = spec_item(RING, j, k, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);

		/*
		 * Let fighters, assassins, monks, and thieves know what kind
		 * of rings these are.
		 */
		switch (player.t_ctype) {
		    case C_FIGHTER:
		    case C_THIEF:
		    case C_ASSASIN:
		    case C_MONK:
			op->o_flags |= (ISPOST | ISKNOW);
		    otherwise:
			op->o_flags |= ISPOST;
		}
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /* Let's offer some potions */
	    for (i=rnd(3)+1; i>0; i--) {
		/* Choose the type of potion */
		if (i == 1 && player.t_ctype == C_ASSASIN) j = P_POISON;
		else switch (rnd(8)) {
		    case 0:	j = P_CLEAR;
		    when 1:	j = P_HEALING;
		    when 2:	j = P_MFIND;
		    when 3:	j = P_TFIND;
		    when 4:	j = P_HASTE;
		    when 5:	j = P_RESTORE;
		    when 6:	j = P_FLY;
		    when 7:	j = P_FFIND;
		}

		/* Make the potion */
		ll = spec_item(POTION, j, 0, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_flags |= ISPOST;

		/* Place the potion */
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /* Let's offer some scrolls */
	    for (i=rnd(3)+1; i>0; i--) {
		/* Choose the type of scrolls */
		switch (rnd(8)) {
		    case 0:	j = S_CONFUSE;
		    when 1:	j = S_MAP;
		    when 2:	j = S_LIGHT;
		    when 3:	j = S_SLEEP;
		    when 4:	j = S_IDENT;
		    when 5:	j = S_GFIND;
		    when 6:	j = S_REMOVE;
		    when 7:	j = S_CURING;
		}

		/* Make the scroll */
		ll = spec_item(SCROLL, j, 0, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_flags |= ISPOST;

		/* Place the scroll */
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }

	    /* And finally, let's get some food */
	    for (i=rnd(3)+1; i>0; i--) {
		ll = spec_item(FOOD, 0, 0, 0);
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_weight = things[TYP_FOOD].mi_wght;
		op->o_flags |= ISPOST;
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }
	}
	else {
	    i = roll(10, 4);			/* 10 to 40 items */
	    for (; i > 0 ; i--) {		/* place all the items */
		ll = new_thing(ALL, TRUE);		/* get something */
		attach(lvl_obj, ll);
		op = OBJPTR(ll);
		op->o_flags |= ISPOST;		/* object in trading post */
		do {
		    rnd_pos(rp,&tp);
		} until (mvinch(tp.y, tp.x) == FLOOR);
		op->o_pos = tp;
		mvaddch(tp.y,tp.x,op->o_type);
	    }
	}
	wmove(cw,12,0);
	trader = 0;
	if (startup) {
	    waddstr(cw,"Welcome to Friendly Fiend's Equipage\n\r");
	    waddstr(cw,"====================================\n\r");
	}
	else {
	    waddstr(cw,"Welcome to Friendly Fiend's Flea Market\n\r");
	    waddstr(cw,"=======================================\n\r");
	}
	waddstr(cw,"$: Prices object that you stand upon.\n\r");
	waddstr(cw,"#: Buys the object that you stand upon.\n\r");
	waddstr(cw,"%: Trades in something in your pack for gold.\n\r");
	trans_line();
}


/*
 * get_worth:
 *	Calculate an objects worth in gold
 */
get_worth(obj)
reg struct object *obj;
{
	reg int worth, wh;

	worth = 0;
	wh = obj->o_which;
	switch (obj->o_type) {
	    case FOOD:
		worth = 2;
	    when WEAPON:
		if (wh < MAXWEAPONS) {
		    worth = weaps[wh].w_worth;
		    worth += s_magic[S_ALLENCH].mi_worth * 
		   		 (obj->o_hplus + obj->o_dplus);
		}
	    when ARMOR:
		if (wh < MAXARMORS) {
		    worth = armors[wh].a_worth;
		    worth += s_magic[S_ALLENCH].mi_worth * 
				(armors[wh].a_class - obj->o_ac);
		}
	    when SCROLL:
		if (wh < MAXSCROLLS)
		    worth = s_magic[wh].mi_worth;
	    when POTION:
		if (wh < MAXPOTIONS)
		    worth = p_magic[wh].mi_worth;
	    when RING:
		if (wh < MAXRINGS) {
		    worth = r_magic[wh].mi_worth;
		    worth += obj->o_ac * 40;
		}
	    when STICK:
		if (wh < MAXSTICKS) {
		    worth = ws_magic[wh].mi_worth;
		    worth += 20 * obj->o_charges;
		}
	    when MM:
		if (wh < MAXMM) {
		    worth = m_magic[wh].mi_worth;
		    switch (wh) {
			case MM_BRACERS:	worth += 40  * obj->o_ac;
			when MM_PROTECT:	worth += 60  * obj->o_ac;
			when MM_DISP:		/* ac already figured in price*/
			otherwise:		worth += 20  * obj->o_ac;
		    }
		}
	    when RELIC:
		if (wh < MAXRELIC) {
		    worth = rel_magic[wh].mi_worth;
		    if (wh == quest_item) worth *= 10;
		}
	    otherwise:
		worth = 0;
	}
	if (obj->o_flags & ISPROT)	/* 300% more for protected */
	    worth *= 3;
	if (obj->o_flags &  ISBLESSED)	/* 50% more for blessed */
	    worth = worth * 3 / 2;
	if (obj->o_flags & ISCURSED)	/* half for cursed */
	    worth /= 2;
	if (worth < 0)
	    worth = 0;
	return worth;
}

/*
 * open_market:
 *	Retruns TRUE when ok do to transacting
 */
open_market()
{
	if (trader >= MAXPURCH && !wizard && level != 0) {
	    msg("The market is closed. The stairs are that-a-way.");
	    return FALSE;
	}
	else {
	    return TRUE;
	}
}

/*
 * price_it:
 *	Price the object that the hero stands on
 */
price_it()
{
	reg struct linked_list *item;
	reg struct object *obj;
	reg int worth;
	reg char *str;

	if (!open_market())		/* after buying hours */
	    return FALSE;
	if ((item = find_obj(hero.y,hero.x)) == NULL) {
	    debug("Can't find the item");
	    return FALSE;
	}
	obj = OBJPTR(item);
	worth = get_worth(obj);
	if (worth < 0) {
	    msg("That's not for sale.");
	    return FALSE;
	}
	if (worth < 25)
	    worth = 25;

	/* Our shopkeeper is affected by the person's charisma */
	worth = (int) ((float) worth * (17. / (float)pstats.s_charisma));
	str = inv_name(obj, TRUE);
	msg("%s for only %d pieces of gold", str, worth);
	curprice = worth;		/* save price */
	strcpy(curpurch,str);		/* save item */
	return TRUE;
}



/*
 * sell_it:
 *	Sell an item to the trading post
 */
sell_it()
{
	reg struct linked_list *item;
	reg struct object *obj;
	reg int wo, ch;

	if (!open_market())		/* after selling hours */
	    return;

	if ((item = get_item(pack, "sell", ALL, FALSE, FALSE)) == NULL)
	    return;
	obj = OBJPTR(item);
	wo = get_worth(obj);
	if (wo <= 0) {
	    mpos = 0;
	    msg("We don't buy those.");
	    return;
	}
	if (wo < 25)
	    wo = 25;
	msg("Your %s is worth %d pieces of gold.",typ_name(obj),wo);
	msg("Do you want to sell it? ");
	do {
	    ch = tolower(readchar());
	    if (ch == ESCAPE || ch == 'n') {
		msg("");
		return;
	    }
	} until (ch == 'y');
	mpos = 0;
	if (drop(item) == TRUE) {		/* drop this item */	
	    purse += wo;			/* give him his money */
	    ++trader;				/* another transaction */
	    wo = obj->o_count;
	    if (obj->o_group == 0) 		/* dropped one at a time */
		obj->o_count = 1;
	    msg("Sold %s",inv_name(obj,TRUE));
	    obj->o_count = wo;
	    trans_line();			/* show remaining deals */
	}
}

/*
 * trans_line:
 *	Show how many transactions the hero has left
 */
trans_line()
{
	if (level == 0)
	    sprintf(prbuf, "You are welcome to spend whatever you have.");
	else if (!wizard)
	    sprintf(prbuf,"You have %d transactions remaining.",
		    MAXPURCH - trader);
	else
	    sprintf(prbuf,
		"You have infinite transactions remaining oh great wizard.");
	mvwaddstr(cw,lines - 3,0,prbuf);
}



/*
 * typ_name:
 * 	Return the name for this type of object
 */
char *
typ_name(obj)
reg struct object *obj;
{
	static char buff[20];
	reg int wh;

	switch (obj->o_type) {
		case POTION:  wh = TYP_POTION;
		when SCROLL:  wh = TYP_SCROLL;
		when STICK:   wh = TYP_STICK;
		when RING:    wh = TYP_RING;
		when ARMOR:   wh = TYP_ARMOR;
		when WEAPON:  wh = TYP_WEAPON;
		when MM:      wh = TYP_MM;
		when FOOD:    wh = TYP_FOOD;
		when RELIC:   wh = TYP_RELIC;
		otherwise:    wh = -1;
	}
	if (wh < 0)
		strcpy(buff,"unknown");
	else
		strcpy(buff,things[wh].mi_name);
	return (buff);
}


