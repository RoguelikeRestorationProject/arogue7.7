/*
 * sticks.c - Functions to implement the various sticks one might find
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
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 */

#include "curses.h"
#include <ctype.h>
#include "rogue.h"


/*
 * zap a stick and see what happens
 */
do_zap(zapper, obj, direction, which, flags)
struct thing *zapper;
struct object *obj;
coord *direction;
int which;
int flags;
{
    register struct linked_list *item;
    register struct thing *tp;
    register int y, x, bonus;
    struct linked_list *nitem;
    struct object *nobj;
    bool cursed, blessed, is_player;
    char *mname;

    cursed = flags & ISCURSED;
    blessed = flags & ISBLESSED;

    if (obj && obj->o_type != RELIC) { /* all relics are chargeless */
	if (obj->o_charges < 1) {
	    msg(nothing);
	    return;
	}
        obj->o_charges--;
    }
    if (which == WS_WONDER) {
	switch (rnd(14)) {
	    case  0: which = WS_ELECT;
	    when  1: which = WS_FIRE;
	    when  2: which = WS_COLD;
	    when  3: which = WS_POLYMORPH;
	    when  4: which = WS_MISSILE;
	    when  5: which = WS_SLOW_M;
	    when  6: which = WS_TELMON;
	    when  7: which = WS_CANCEL;
	    when  8: which = WS_CONFMON;
	    when  9: which = WS_DISINTEGRATE;
	    when 10: which = WS_PETRIFY;
	    when 11: which = WS_PARALYZE;
	    when 12: which = WS_MDEG;
	    when 13: which = WS_FEAR;
	}
	if(ws_magic[which].mi_curse>0 && rnd(100)<=ws_magic[which].mi_curse){
	    cursed = TRUE;
	    blessed = FALSE;
	}
    }

    tp = NULL;
    switch (which) {
	case WS_POLYMORPH:
	case WS_SLOW_M:
	case WS_TELMON:
	case WS_CANCEL:
	case WS_CONFMON:
	case WS_DISINTEGRATE:
	case WS_PETRIFY:
	case WS_PARALYZE:
	case WS_MDEG:
	case WS_FEAR:
	    y = zapper->t_pos.y;
	    x = zapper->t_pos.x;

	    do {
		y += direction->y;
		x += direction->x;
	    }
	    while (shoot_ok(winat(y, x)) && !(y == hero.y && x == hero.x));

	    if (y == hero.y && x == hero.x)
		is_player = TRUE;
	    else if (isalpha(mvwinch(mw, y, x))) {
		item = find_mons(y, x);
		tp = THINGPTR(item);
		runto(tp, &hero);
		turn_off(*tp, CANSURPRISE);
		mname = monster_name(tp);
		is_player = FALSE;

		/* The monster may not like being shot at */
		if ((zapper == &player)	&&
		    on(*tp, ISCHARMED)	&&
		    save(VS_MAGIC, tp, 0)) {
		    msg("The eyes of %s turn clear.", prname(mname, FALSE));
		    turn_off(*tp, ISCHARMED);
		    mname = monster_name(tp);
		}
	    }
	    else {
		/*
		 * if monster misses player because the player dodged then lessen
		 * the chances he will use the wand again since the player appears
		 * to be rather dextrous
		 */
		if (zapper != &player) 
		    zapper->t_wand = zapper->t_wand * 3 / 4;
	    }
    }
    switch (which) {
	case WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room
	     */
	    blue_light(blessed, cursed);
	when WS_DRAIN:
	    /*
	     * Take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room or next to hero
	     * if he is in a passage (but leave the monsters alone
	     * if the stick is cursed)
	     */
	    if (pstats.s_hpt < 2) {
		msg("You are too weak to use it.");
	    }
	    else if (cursed)
		pstats.s_hpt /= 2;
	    else
		drain(hero.y-1, hero.y+1, hero.x-1, hero.x+1);

	when WS_POLYMORPH:
	{
	    register char oldch;
	    register struct room *rp;
	    register struct linked_list *pitem;
	    coord delta;

	    if (tp == NULL)
		break;
	    if (save(VS_MAGIC, tp, 0)) {
		msg(nothing);
		break;
	    }
	    rp = roomin(&tp->t_pos);
	    check_residue(tp);
	    delta.x = x;
	    delta.y = y;
	    detach(mlist, item);
	    oldch = tp->t_oldch;
	    pitem = tp->t_pack; /* save his pack */
	    tp->t_pack = NULL;
	    new_monster(item,rnd(NUMMONST-NUMUNIQUE-1)+1,&delta,FALSE);
	    if (tp->t_pack != NULL) 
		o_free_list (tp->t_pack);
	    tp->t_pack = pitem;
	    if (isalpha(mvwinch(cw, y, x)))
		mvwaddch(cw, y, x, tp->t_type);
	    tp->t_oldch = oldch;
	    /*
	     * should the room light up?
	     */
	    if (on(*tp, HASFIRE)) {
		if (rp) {
		    register struct linked_list *fire_item;

		    fire_item = creat_item();
		    ldata(fire_item) = (char *) tp;
		    attach(rp->r_fires, fire_item);
		    rp->r_flags |= HASFIRE;
		    if (cansee(tp->t_pos.y,tp->t_pos.x) &&
			next(rp->r_fires) == NULL) light(&hero);
		}
	    }
	    runto(tp, &hero);
	    msg(terse ? "A new %s!" 
		      : "You have created a new %s!",
		      monster_name(tp));
        }

	when WS_PETRIFY:
	    if (tp == NULL)
		break;
	    if (save(VS_MAGIC, tp, 0)) {
		msg(nothing);
		break;
	    }
	    check_residue(tp);
	    turn_on(*tp, ISSTONE);
	    turn_on(*tp, NOSTONE);
	    turn_off(*tp, ISRUN);
	    turn_off(*tp, ISINVIS);
	    turn_off(*tp, CANSURPRISE);
	    turn_off(*tp, ISDISGUISE);
	    tp->t_action = A_NIL;
	    tp->t_no_move = 0;
	    msg("%s is turned to stone!",prname(mname, TRUE));

	when WS_TELMON:
	{
	    register int rm;
	    register struct room *rp;

	    if (tp == NULL)
		break;
	    if (save(VS_MAGIC, tp, 0)) {
		msg(nothing);
		break;
	    }
	    rp = NULL;
	    check_residue(tp);
	    tp->t_action = A_FREEZE; /* creature is disoriented */
	    tp->t_no_move = 2;
	    if (cursed) {	/* Teleport monster to player */
		if ((y == (hero.y + direction->y)) &&
		    (x == (hero.x + direction->x)))
			msg(nothing);
		else {
		    tp->t_pos.y = hero.y + direction->y;
		    tp->t_pos.x = hero.x + direction->x;
		}
	    }
	    else if (blessed) {	/* Get rid of monster */
		killed(item, FALSE, TRUE, TRUE);
		return;
	    }
	    else {
		register int i=0;

		do {	/* Move monster to another room */
		    rm = rnd_room();
		    rnd_pos(&rooms[rm], &tp->t_pos);
		}until(winat(tp->t_pos.y,tp->t_pos.x)==FLOOR ||i++>500);
		rp = &rooms[rm];
	    }

	    /* Now move the monster */
	    if (isalpha(mvwinch(cw, y, x)))
		mvwaddch(cw, y, x, tp->t_oldch);
	    mvwaddch(mw, y, x, ' ');
	    mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, tp->t_type);
	    if (tp->t_pos.y != y || tp->t_pos.x != x)
		tp->t_oldch = CCHAR( mvwinch(cw, tp->t_pos.y, tp->t_pos.x) );
	    /*
	     * check to see if room that creature appears in should
	     * light up
	     */
	    if (on(*tp, HASFIRE)) {
		if (rp) {
		    register struct linked_list *fire_item;

		    fire_item = creat_item();
		    ldata(fire_item) = (char *) tp;
		    attach(rp->r_fires, fire_item);
		    rp->r_flags |= HASFIRE;
		    if(cansee(tp->t_pos.y, tp->t_pos.x) && 
		       next(rp->r_fires) == NULL)
			light(&hero);
		}
	    }
	}
	when WS_CANCEL:
	    if (tp == NULL)
		break;
	    if (save(VS_MAGIC, tp, 0)) {
		msg(nothing);
		break;
	    }
	    check_residue(tp);
	    tp->t_flags[0] &= CANC0MASK;
	    tp->t_flags[1] &= CANC1MASK;
	    tp->t_flags[2] &= CANC2MASK;
	    tp->t_flags[3] &= CANC3MASK;
	    tp->t_flags[4] &= CANC4MASK;
	    tp->t_flags[5] &= CANC5MASK;
	    tp->t_flags[6] &= CANC6MASK;
	    tp->t_flags[7] &= CANC7MASK;
	    tp->t_flags[8] &= CANC8MASK;
	    tp->t_flags[9] &= CANC9MASK;
	    tp->t_flags[10] &= CANCAMASK;
	    tp->t_flags[11] &= CANCBMASK;
	    tp->t_flags[12] &= CANCCMASK;
	    tp->t_flags[13] &= CANCDMASK;
	    tp->t_flags[14] &= CANCEMASK;
	    tp->t_flags[15] &= CANCFMASK;

	when WS_MISSILE:
	{
	    int dice;
	    static struct object bolt =
	    {
		MISSILE , {0, 0}, "", 0, "", "1d4 " , NULL, 0, WS_MISSILE, 50, 1
	    };

	    if (!obj)
		dice = zapper->t_stats.s_lvl;
	    if (obj->o_type == RELIC)
		 dice = 15;
	    else if (EQUAL(ws_type[which], "staff"))
		 dice = 10;
	    else
		 dice = 6;
	    sprintf(bolt.o_hurldmg, "%dd4", dice);
	    do_motion(&bolt, direction->y, direction->x, zapper);
	    if (!hit_monster(unc(bolt.o_pos), &bolt, zapper))
	       msg("The missile vanishes with a puff of smoke");
	}
	when WS_HIT:
	{
	    register char ch;
	    struct object strike; /* don't want to change sticks attributes */

	    direction->y += hero.y;
	    direction->x += hero.x;
	    ch = CCHAR( winat(direction->y, direction->x) );
	    if (isalpha(ch))
	    {
		strike = *obj;
		strike.o_hplus  = 6;
		if (EQUAL(ws_type[which], "staff"))
		    strncpy(strike.o_damage,"3d8",sizeof(strike.o_damage));
		else
		    strncpy(strike.o_damage,"2d8",sizeof(strike.o_damage));
		fight(direction, &strike, FALSE);
	    }
	}
	when WS_SLOW_M:
	    if (is_player) {
		add_slow();
		break;
	    }
	    if (tp == NULL)
		break;
	    if (cursed) {
		if (on(*tp, ISSLOW))
		    turn_off(*tp, ISSLOW);
		else
		    turn_on(*tp, ISHASTE);
		break;
	    }
	    if ((on(*tp,ISUNIQUE) && save(VS_MAGIC,tp,0)) || on(*tp,NOSLOW)) {
		msg(nothing);
		break;
	    }
	    else if (blessed) {
		turn_off(*tp, ISRUN);
		turn_on(*tp, ISHELD);
	    }
	    /*
	     * always slow in case he breaks free of HOLD
	     */
	    if (on(*tp, ISHASTE))
		turn_off(*tp, ISHASTE);
	    else
		turn_on(*tp, ISSLOW);

	when WS_CHARGE:
	    if (ws_know[WS_CHARGE] != TRUE && obj)
		msg("This is a wand of charging.");
	    nitem = get_item(pack, "charge", STICK, FALSE, FALSE);
	    if (nitem != NULL) {
		nobj = OBJPTR(nitem);
	        if ((++(nobj->o_charges) == 1) && (nobj->o_which == WS_HIT))
		    fix_stick(nobj);
		if (blessed) ++(nobj->o_charges);
	        if (EQUAL(ws_type[nobj->o_which], "staff")) {
		    if (nobj->o_charges > 100) 
		        nobj->o_charges = 100;
		}
	        else {
		    if (nobj->o_charges > 50)
		        nobj->o_charges = 50;
		}
	    }
	when WS_ELECT:
	    shoot_bolt(	zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
			"lightning bolt", roll(zapper->t_stats.s_lvl,6));

	when WS_FIRE:
	    shoot_bolt(	zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
			"flame", roll(zapper->t_stats.s_lvl,6));

	when WS_COLD:
	    shoot_bolt(	zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
			"ice", roll(zapper->t_stats.s_lvl,6));

	when WS_CONFMON:
	    if (cursed || is_player) { 
		if (!save(VS_WAND, &player, 0)) {
		    dsrpt_player();
		    confus_player();
		}
		else {
		    if (zapper != &player) zapper->t_wand /= 2;
		    msg(nothing);
		}
	    }
	    else {
		if (tp == NULL)
		    break;
		if (save(VS_MAGIC, tp, 0) || on(*tp, ISCLEAR))
		     msg(nothing);
		else
		     turn_on (*tp, ISHUH);
	    }
	when WS_PARALYZE:
	    if (is_player || cursed) {
		if ((obj && obj->o_type==RELIC) || !save(VS_WAND, &player, 0)){
		    player.t_no_move += 2 * movement(&player) * FREEZETIME;
		    player.t_action = A_FREEZE;
		    msg("You can't move.");
		}
		else {
		    if (zapper != &player) zapper->t_wand /= 2;
		    msg(nothing);
		}
	    }
	    else {
		if (tp == NULL)
		    break;
		bonus = 0;
		if (blessed) bonus = -3;
		if (((obj && obj->o_type==RELIC) || !save(VS_WAND,tp,bonus)) &&
		    off(*tp, NOPARALYZE)) {
		    tp->t_no_move += 2 * movement(tp) * FREEZETIME;
		    tp->t_action = A_FREEZE;
		}
		else {
		    msg(nothing);
		}
	    }
	when WS_FEAR:
	    if (is_player) {
		if (!on(player, ISFLEE)		|| 
		    ISWEARING(R_HEROISM)	|| 
		    save(VS_WAND, &player, 0)) {
			msg(nothing);
			zapper->t_wand /= 2;
		}
		else {
		    turn_on(player, ISFLEE);
		    player.t_dest = &zapper->t_pos;
		    msg("The sight of %s terrifies you.", prname(mname, FALSE));
		}
		break;
	    }
	    if (tp == NULL)
		break;
	    bonus = 0;
	    if (blessed) bonus = -3;
	    if(save(VS_WAND, tp,bonus) || on(*tp,ISUNDEAD) || on(*tp,NOFEAR)){
		    msg(nothing);
		    break;
	    }
	    turn_on(*tp, ISFLEE);
	    turn_on(*tp, WASTURNED);

	    /* Stop it from attacking us */
	    dsrpt_monster(tp, TRUE, cansee(tp->t_pos.y, tp->t_pos.x));

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

	when WS_MDEG:
	    if (is_player) {
		if (save(VS_WAND, &player, 0)) {
		    msg (nothing);
		    zapper->t_wand /= 2;
		    break;
		}
		pstats.s_hpt /= 2;
		if (pstats.s_hpt <= 0) {
		    msg("Your life has been sucked from you -- More --");
		    wait_for(' ');
		    death(zapper);
		}
		else
		    msg("You feel a great drain on your system");
	    }
	    if (tp == NULL)
		break;
	    if (cursed) {
		 tp->t_stats.s_hpt *= 2;
		 msg("%s appears to be stronger now!", prname(mname, TRUE));
	    }
	    else if (on(*tp, ISUNIQUE) && save(VS_WAND, tp, 0))
		 msg (nothing);
	    else {
		 tp->t_stats.s_hpt /= 2;
		 msg("%s appears to be weaker now", prname(mname, TRUE));
	    }
	    if (tp->t_stats.s_hpt < 1)
		 killed(item, TRUE, TRUE, TRUE);
	when WS_DISINTEGRATE:
	    if (tp == NULL)
		break;
	    if (cursed) {
		register int m1, m2;
		coord mp;
		struct linked_list *titem;
		char ch;
		struct thing *th;

		if (on(*tp, ISUNIQUE)) {
		    msg (nothing);
		    break;
		}
		for (m1=tp->t_pos.x-1 ; m1 <= tp->t_pos.x+1 ; m1++) {
		    for(m2=tp->t_pos.y-1 ; m2<=tp->t_pos.y+1 ; m2++) {
			if (m1 == hero.x && m2 == hero.y)
			    continue;
			ch = CCHAR( winat(m2,m1) );
			if (shoot_ok(ch)) {
			    mp.x = m1;	/* create it */
			    mp.y = m2;
			    titem = new_item(sizeof(struct thing));
			    new_monster(titem,(short)tp->t_index,&mp,FALSE);
			    th = THINGPTR(titem);
			    turn_on (*th, ISMEAN);
			    runto(th,&hero);
			    if (on(*th, HASFIRE)) {
				register struct room *rp;

				rp = roomin(&th->t_pos);
				if (rp) {
				    register struct linked_list *fire_item;

				    fire_item = creat_item();
				    ldata(fire_item) = (char *) th;
				    attach(rp->r_fires, fire_item);
				    rp->r_flags |= HASFIRE;
				    if (cansee(th->t_pos.y, th->t_pos.x) &&
					next(rp->r_fires) == NULL)
					light(&hero);
				}
			    }
			}
		    }
		}
	    }
	    else { /* if its a UNIQUE it might still live */
		if (on(*tp, ISUNIQUE) && save(VS_MAGIC, tp, 0)) {
		    tp->t_stats.s_hpt /= 2;
		    if (tp->t_stats.s_hpt < 1) {
			 killed(item, FALSE, TRUE, TRUE);
			 msg("You have disintegrated %s", prname(mname, FALSE));
		    }
		    else {
			msg("%s appears wounded", prname(mname, TRUE));
		    }
		}
		else {
		    msg("You have disintegrated %s", prname(mname, FALSE));
		    killed (item, FALSE, TRUE, TRUE);
		}
	    }
	when WS_CURING:
	    if (cursed) {
		if (!save(VS_POISON, &player, 0)) {
		    msg("You feel extremely sick now");
		    pstats.s_hpt /=2;
		    if (pstats.s_hpt == 0) death (D_POISON);
		}
		if (!save(VS_WAND, &player, 0) && !ISWEARING(R_HEALTH)) {
		    turn_on(player, HASDISEASE);
		    turn_on(player, HASINFEST);
		    turn_on(player, DOROT);
		    fuse(cure_disease, 0, roll(HEALTIME,SICKTIME), AFTER);
		    infest_dam++;
		}
		else msg("You fell momentarily sick");
	    }
	    else {
		if (on(player, HASDISEASE) || on(player, HASINFEST)) {
		    extinguish(cure_disease);
		    turn_off(player, HASINFEST);
		    infest_dam = 0;
		    cure_disease(); /* this prints message */
		}
		if (on(player, DOROT)) {
		    msg("You feel your skin returning to normal.");
		    turn_off(player, DOROT);
		}
		pstats.s_hpt += roll(pstats.s_lvl, blessed ? 6 : 4);
		if (pstats.s_hpt > max_stats.s_hpt)
		    pstats.s_hpt = max_stats.s_hpt;
		msg("You begin to feel %sbetter.", blessed ? "much " : "");
		    
	    }
	otherwise:
	    msg("What a bizarre schtick!");
    }
}


/*
 * drain:
 *	Do drain hit points from player shtick
 */

drain(ymin, ymax, xmin, xmax)
int ymin, ymax, xmin, xmax;
{
    register int i, j, count;
    register struct thing *ick;
    register struct linked_list *item;

    /*
     * First count how many things we need to spread the hit points among
     */
    count = 0;
    for (i = ymin; i <= ymax; i++) {
	if (i < 1 || i > lines - 3)
	    continue;
	for (j = xmin; j <= xmax; j++) {
	    if (j < 0 || j > cols - 1)
		continue;
	    if (isalpha(mvwinch(mw, i, j)))
		count++;
	}
    }
    if (count == 0)
    {
	msg("You have a tingling feeling");
	return;
    }
    count = pstats.s_hpt / count;
    pstats.s_hpt /= 2;
    /*
     * Now zot all of the monsters
     */
    for (i = ymin; i <= ymax; i++) {
	if (i < 1 || i > lines - 3)
	    continue;
	for (j = xmin; j <= xmax; j++) {
	    if (j < 0 || j > cols - 1)
		continue;
	    if (isalpha(mvwinch(mw, i, j)) &&
	        ((item = find_mons(i, j)) != NULL)) {
		ick = THINGPTR(item);
		if (on(*ick, ISUNIQUE) && save(VS_MAGIC, ick, 0)) 
		    ick->t_stats.s_hpt -= count / 2;
		else
		    ick->t_stats.s_hpt -= count;
		if (ick->t_stats.s_hpt < 1)
		    killed(item, 
			   cansee(i,j)&&(!on(*ick,ISINVIS)||on(player,CANSEE)),
			   TRUE, TRUE);
		else {
		    runto(ick, &hero);

		    /* 
		     * The monster may not like being shot at.  Since the
		     * shot is not aimed directly at the monster, we will
		     * give him a poorer save.
		     */
		    if (on(*ick, ISCHARMED) && save(VS_MAGIC, ick, -2)) {
			msg("The eyes of %s turn clear.",
			    prname(monster_name(ick), FALSE));
			turn_off(*ick, ISCHARMED);
		    }
		    if (cansee(i,j) && (!on(*ick,ISINVIS)||on(player,CANSEE)))
			    msg("%s appears wounded", 
				prname(monster_name(ick), TRUE));
		}
	    }
	}
    }
}

/*
 * initialize a stick
 */
fix_stick(cur)
register struct object *cur;
{
    if (EQUAL(ws_type[cur->o_which], "staff")) {
	cur->o_weight = 100;
	cur->o_charges = 5 + rnd(10);
	strncpy(cur->o_damage, "2d3", sizeof(cur->o_damage));
	cur->o_hplus = 1;
	cur->o_dplus = 0;
	switch (cur->o_which) {
	    case WS_HIT:
		cur->o_hplus = 3;
		cur->o_dplus = 3;
		strncpy(cur->o_damage, "2d8", sizeof(cur->o_damage));
	    when WS_LIGHT:
		cur->o_charges = 20 + rnd(10);
	    }
    }
    else {
	strncpy(cur->o_damage, "1d3", sizeof(cur->o_damage));
	cur->o_weight = 60;
	cur->o_hplus = 1;
	cur->o_dplus = 0;
	cur->o_charges = 3 + rnd(5);
	switch (cur->o_which) {
	    case WS_HIT:
		cur->o_hplus = 3;
		cur->o_dplus = 3;
		strncpy(cur->o_damage, "1d8", sizeof(cur->o_damage));
	    when WS_LIGHT:
		cur->o_charges = 10 + rnd(10);
	    }
    }
    strncpy(cur->o_hurldmg, "1d1", sizeof(cur->o_hurldmg));

}

/*
 * Use the wand that our monster is wielding.
 */
m_use_wand(monster)
register struct thing *monster;
{
    register struct object *obj;

    /* Make sure we really have it */
    if (monster->t_using) 
	obj = OBJPTR(monster->t_using);
    else {
	debug("Stick not set!");
	monster->t_action = A_NIL;
	return;
    }

    if (obj->o_type != STICK) {
	debug("Stick not selected!");
	monster->t_action = A_NIL;
	return;
    }
    /*
     * shoot the stick!
     * assume all blessed sticks are normal for now. 
     * Note that we don't get here if the wand is cursed.
     */
    msg("%s points a %s at you!", prname(monster_name(monster), TRUE),
	ws_type[obj->o_which]);
    do_zap(monster, obj, &monster->t_newpos, obj->o_which, NULL);
    monster->t_wand /= 2; /* chance lowers with each use */
}

bool
need_dir(type, which)
int	type,		/* type of item, NULL means stick */
	which;		/* which item			  */
{
    if (type == STICK || type == 0) {
	switch (which) {
	    case WS_LIGHT:
	    case WS_DRAIN: 
	    case WS_CHARGE:
	    case WS_CURING:
		return(FALSE);
	    default:
		return(TRUE);
	}
    }
    else if (type == RELIC) {
	switch (which) {
	    case MING_STAFF:
	    case ASMO_ROD:
	    case EMORI_CLOAK:
		return(TRUE);
	    default:
		return(FALSE);
	}
    }
return (FALSE); /* hope we don't get here */
}
/*
 * let the player zap a stick and see what happens
 */
player_zap(which, flag)
int which;
int flag;
{
    register struct linked_list *item;
    register struct object *obj;

    obj = NULL;
    if (which == 0) {
	/* This is a stick.  It takes 2 movement periods to zap it */
	if (player.t_action != C_ZAP) {
	    if ((item = get_item(pack,"zap with",ZAPPABLE,FALSE,FALSE)) == NULL)
		return(FALSE);

	    obj = OBJPTR(item);

	    if (need_dir(obj->o_type, obj->o_which)) {
		if (!get_dir(&player.t_newpos))
		    return(FALSE);
	    }
	    player.t_using = item;	/* Remember what it is */
	    player.t_action = C_ZAP;	/* We are quaffing */
	    player.t_no_move = 2 * movement(&player);
	    return(TRUE);
	}

	item = player.t_using;
	/* We've waited our time, let's shoot 'em up! */
	player.t_using = NULL;
	player.t_action = A_NIL;

	obj = OBJPTR(item);

	/* Handle relics specially here */
	if (obj->o_type == RELIC) {
	    switch (obj->o_which) {
		case ORCUS_WAND:
		    msg(nothing);
		    return(TRUE);
		when MING_STAFF:
		    which = WS_MISSILE;
		when EMORI_CLOAK:
		    which = WS_PARALYZE;
		    obj->o_charges = 0; /* one zap/day(whatever that is) */
		    fuse(cloak_charge, obj, CLOAK_TIME, AFTER);
		when ASMO_ROD:
		    switch (rnd(3)) {
			case 0:		which = WS_ELECT;
			when 1:		which = WS_COLD;
			otherwise:	which = WS_FIRE;
		    }
	    }
	}
	else {
	    which = obj->o_which;
	    ws_know[which] = TRUE;
	    flag = obj->o_flags;
	}
    }
    do_zap(&player, obj, &player.t_newpos, which, flag);
    return(TRUE);
}


/*
 * shoot_bolt fires a bolt from the given starting point in the
 * 	      given direction
 */

shoot_bolt(shooter, start, dir, get_points, reason, name, damage)
struct thing *shooter;
coord start, dir;
bool get_points;
short reason;
char *name;
int damage;
{
    register char dirch, ch;
    register bool used, change;
    register short y, x, bounces;
    coord pos;
    struct linked_list *target=NULL;
    struct {
	coord place;
	char oldch;
    } spotpos[BOLT_LENGTH];

    switch (dir.y + dir.x) {
	case 0: dirch = '/';
	when 1: case -1: dirch = (dir.y == 0 ? '-' : '|');
	when 2: case -2: dirch = '\\';
    }
    pos.y = start.y + dir.y;
    pos.x = start.x + dir.x;
    used = FALSE;
    change = FALSE;

    bounces = 0;	/* No bounces yet */
    for (y = 0; y < BOLT_LENGTH && !used; y++)
    {
	ch = CCHAR( winat(pos.y, pos.x) );
	spotpos[y].place = pos;
	spotpos[y].oldch = CCHAR( mvwinch(cw, pos.y, pos.x) );

	/* Are we at hero? */
	if (ce(pos, hero)) goto at_hero;

	switch (ch)
	{
	    case SECRETDOOR:
	    case '|':
	    case '-':
	    case ' ':
		if (dirch == '-' || dirch == '|') {
		    dir.y = -dir.y;
		    dir.x = -dir.x;
		}
		else {
		    char chx = CCHAR( mvinch(pos.y-dir.y, pos.x) ),
			 chy = CCHAR( mvinch(pos.y, pos.x-dir.x) );
		    bool anychange = FALSE; /* Did we change anthing */

		    if (chy == WALL || chy == SECRETDOOR ||
			chy == '-' || chy == '|') {
			dir.y = -dir.y;
			change ^= TRUE;	/* Change at least one direction */
			anychange = TRUE;
		    }
		    if (chx == WALL || chx == SECRETDOOR ||
			chx == '-' || chx == '|') {
			dir.x = -dir.x;
			change ^= TRUE;	/* Change at least one direction */
			anychange = TRUE;
		    }

		    /* If we didn't make any change, make both changes */
		    if (!anychange) {
			dir.x = -dir.x;
			dir.y = -dir.y;
		    }
		}

		/* Do we change how the bolt looks? */
		if (change) {
		    change = FALSE;
		    if (dirch == '\\') dirch = '/';
		    else if (dirch == '/') dirch = '\\';
		}

		y--;	/* The bounce doesn't count as using up the bolt */

		/* Make sure we aren't in an infinite bounce */
		if (++bounces > BOLT_LENGTH) used = TRUE;
		msg("The %s bounces", name);
		break;
	    default:
		if (isalpha(ch)) {
		    register struct linked_list *item;
		    register struct thing *tp;
		    register char *mname;
		    bool see_monster = cansee(pos.y, pos.x);

		    item = find_mons(unc(pos));
		    tp = THINGPTR(item);
		    mname = monster_name(tp);

		    /*
		     * If our prey shot this, let's record the fact that
		     * he can shoot, regardless of whether he hits us.
		     */
		    if ((tp->t_dest != NULL) && ce(*tp->t_dest, shooter->t_pos)) tp->t_wasshot = TRUE;

		    if (!save(VS_BREATH, tp, -(shooter->t_stats.s_lvl/10))) {
			if (see_monster) {
			    if (on(*tp, ISDISGUISE) &&
				(tp->t_type != tp->t_disguise)) {
				msg("Wait! That's a %s!", mname);
				turn_off(*tp, ISDISGUISE);
			    }

			    turn_off(*tp, CANSURPRISE);
			    msg("The %s hits %s", name, prname(mname, FALSE));
			}

			/* Should we start to chase the shooter? */
			if (shooter != &player			&&
			    shooter != tp			&&
			    shooter->t_index != tp->t_index	&&
			    (tp->t_dest == NULL || rnd(100) < 25)) {
			    /*
			     * If we're intelligent enough to realize that this
			     * is a friendly monster, we will attack the hero
			     * instead.
			     */
			    if (on(*shooter, ISFRIENDLY) &&
				 roll(3,6) < tp->t_stats.s_intel)
				 runto(tp, &hero);

			    /* Otherwise, let's chase the monster */
			    else runto(tp, &shooter->t_pos);
			}
			else if (shooter == &player) {
			    runto(tp, &hero);

			    /*
			     * If the player shot a charmed monster, it may
			     * not like being shot at.
			     */
			    if (on(*tp, ISCHARMED) && save(VS_MAGIC, tp, 0)) {
				msg("The eyes of %s turn clear.", 
				    prname(mname, FALSE));
				turn_off(*tp, ISCHARMED);
				mname = monster_name(tp);
			    }
			}

			/*
			 * Let the defender know that the attacker has
			 * missiles!
			 */
			if (ce(*tp->t_dest, shooter->t_pos))
			    tp->t_wasshot = TRUE;

			used = TRUE;

			/* Hit the monster -- does it do anything? */
			if ((EQUAL(name,"ice")           &&
			     (on(*tp, NOCOLD) || on(*tp, ISUNDEAD)))          ||
			    (EQUAL(name,"flame")         && on(*tp, NOFIRE))  ||
			    (EQUAL(name,"acid")          && on(*tp, NOACID))  ||
			    (EQUAL(name,"lightning bolt")&& on(*tp,NOBOLT))   ||
			    (EQUAL(name,"nerve gas")     &&on(*tp,NOPARALYZE))||
			    (EQUAL(name,"sleeping gas")  &&
			     (on(*tp, NOSLEEP) || on(*tp, ISUNDEAD)))         ||
			    (EQUAL(name,"slow gas")      && on(*tp,NOSLOW))   ||
			    (EQUAL(name,"fear gas")      && on(*tp,NOFEAR))   ||
			    (EQUAL(name,"confusion gas") && on(*tp,ISCLEAR))  ||
			    (EQUAL(name,"chlorine gas")  && on(*tp,NOGAS))) {
			    if (see_monster)
				msg("The %s has no effect on %s.",
					name, prname(mname, FALSE));
			}

			else {
			    bool see_him;

			    see_him = 
				    off(player, ISBLIND)		      &&
				    cansee(unc(tp->t_pos))		      &&
				    (off(*tp, ISINVIS) || on(player, CANSEE)) &&
				    (off(*tp, ISSHADOW)|| on(player, CANSEE)) &&
				    (off(*tp, CANSURPRISE)||ISWEARING(R_ALERT));

			    /* Did a spell get disrupted? */
			    dsrpt_monster(tp, FALSE, see_him);

			    /* 
			     * Check for gas with special effects 
			     */
			    if (EQUAL(name, "nerve gas")) {
				tp->t_no_move = movement(tp) * FREEZETIME;
				tp->t_action = A_FREEZE;
			    }
			    else if (EQUAL(name, "sleeping gas")) {
				tp->t_no_move = movement(tp) * SLEEPTIME;
				tp->t_action = A_FREEZE;
			    }
			    else if (EQUAL(name, "slow gas")) {
				if (on(*tp, ISHASTE))
				    turn_off(*tp, ISHASTE);
				else
				    turn_on(*tp, ISSLOW);
			    }
			    else if (EQUAL(name, "fear gas")) {
				turn_on(*tp, ISFLEE);
				tp->t_dest = &hero;

				/* It is okay to turn tail */
				tp->t_oldpos = tp->t_pos;
			    }
			    else if (EQUAL(name, "confusion gas")) {
				turn_on(*tp, ISHUH);
				tp->t_dest = &hero;
			    }
			    else if ((EQUAL(name, "lightning bolt")) &&
				     on(*tp, BOLTDIVIDE)) {
				    if (creat_mons(tp, tp->t_index, FALSE)) {
				      if (see_monster)
				       msg("The %s divides %s.",
					   name,prname(mname, FALSE));
				      light(&hero);
				    }
				    else if (see_monster)
					msg("The %s has no effect on %s.",
					    name, prname(mname, FALSE));
			    }
			    else {
				if (save(VS_BREATH, tp,
				         -(shooter->t_stats.s_lvl/10)))
				    damage /= 2;

				/* The poor fellow got killed! */
				if ((tp->t_stats.s_hpt -= damage) <= 0) {
				    if (see_monster)
					msg("The %s kills %s", 
					    name, prname(mname, FALSE));
				    else
				     msg("You hear a faint groan in the distance");
				    /*
				     * Instead of calling killed() here, we
				     * will record that the monster was killed
				     * and call it at the end of the routine,
				     * after we restore what was under the bolt.
				     * We have to do this because in the case
				     * of a bolt that first misses the monster
				     * and then gets it on the bounce.  If we
				     * call killed here, the 'missed' space in
				     * spotpos puts the monster back on the
				     * screen
				     */
				    target = item;
				}
				else {	/* Not dead, so just scream */
				     if (!see_monster)
				       msg("You hear a scream in the distance");
				}
			    }
			}
		    }
		    else if (isalpha(show(pos.y, pos.x))) {
			if (see_monster) {
			    if (terse)
				msg("%s misses", name);
			    else
				msg("The %s whizzes past %s",
					    name, prname(mname, FALSE));
			}
			if (get_points) runto(tp, &hero);
		    }
		}
		else if (pos.y == hero.y && pos.x == hero.x) {
at_hero: 	    if (!save(VS_BREATH, &player,
				-(shooter->t_stats.s_lvl/10))){
			if (terse)
			    msg("The %s hits you", name);
			else
			    msg("You are hit by the %s", name);
			used = TRUE;

			/* 
			 * The Amulet of Yendor protects against all "breath" 
			 *
			 * The following two if statements could be combined 
			 * into one, but it makes the compiler barf, so split 
			 * it up
			 */
			if (cur_relic[YENDOR_AMULET]			    ||
			    (EQUAL(name,"chlorine gas")&&on(player, NOGAS)) ||
			    (EQUAL(name,"sleeping gas")&&ISWEARING(R_ALERT))){
			     msg("The %s has no affect", name);
			}
			else if((EQUAL(name, "flame") && on(player, NOFIRE)) ||
			        (EQUAL(name, "ice")   && on(player, NOCOLD)) ||
			        (EQUAL(name,"lightning bolt")&& 
							 on(player,NOBOLT))  ||
			        (EQUAL(name,"fear gas")&&ISWEARING(R_HEROISM))){
			     msg("The %s has no affect", name);
			}

			else {
			    dsrpt_player();

			    /* 
			     * Check for gas with special effects 
			     */
			    if (EQUAL(name, "nerve gas")) {
				msg("The nerve gas paralyzes you.");
				player.t_no_move +=
					movement(&player) * FREEZETIME;
				player.t_action = A_FREEZE;
			    }
			    else if (EQUAL(name, "sleeping gas")) {
				msg("The sleeping gas puts you to sleep.");
				player.t_no_move +=
					movement(&player) * SLEEPTIME;
				player.t_action = A_FREEZE;
			    }
			    else if (EQUAL(name, "confusion gas")) {
				if (off(player, ISCLEAR)) {
				    if (on(player, ISHUH))
					lengthen(unconfuse,
					         rnd(20)+HUHDURATION);
				    else {
					turn_on(player, ISHUH);
					fuse(unconfuse, 0,
					     rnd(20)+HUHDURATION, AFTER);
					msg(
					 "The confusion gas has confused you.");
				    }
				}
				else msg("You feel dizzy for a moment, but it quickly passes.");
			    }
			    else if (EQUAL(name, "slow gas")) {
				add_slow();
			    }
			    else if (EQUAL(name, "fear gas")) {
				turn_on(player, ISFLEE);
				player.t_dest = &shooter->t_pos;
				msg("The fear gas terrifies you.");
			    }
			    else {
				if (EQUAL(name, "acid")			&&
				    cur_armor != NULL			&&
				    !(cur_armor->o_flags & ISPROT)	&&
				    !save(VS_BREATH, &player, -2)	&&
				    cur_armor->o_ac < pstats.s_arm+1) {
				       msg("Your armor corrodes from the acid");
				       cur_armor->o_ac++;
				}
				if (save(VS_BREATH, &player,
					 -(shooter->t_stats.s_lvl/10)))
				    damage /= 2;
				if ((pstats.s_hpt -= damage) <= 0) 
				    death(reason);
			    }
			}
		    }
		    else
			msg("The %s whizzes by you", name);
		}

		mvwaddch(cw, pos.y, pos.x, dirch);
		draw(cw);
	}

	pos.y += dir.y;
	pos.x += dir.x;
    }

    /* Restore what was under the bolt */
    for (x = y - 1; x >= 0; x--)
	mvwaddch(cw, spotpos[x].place.y, spotpos[x].place.x, spotpos[x].oldch);

    /* If we killed something, do so now.  This will also blank the monster. */
    if (target) killed(target, FALSE, get_points, TRUE);
    return;
}
