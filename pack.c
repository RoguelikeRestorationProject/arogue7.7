/*
 * pack.c - Routines to deal with the pack.
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
#include <ctype.h>
#include "rogue.h"
#ifdef PC7300
#include "menu.h"
#endif

/*
 * Routines to deal with the pack
 */

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is non-null
 * use it as the linked_list pointer instead of gettting it off the ground.
 */
bool
add_pack(item, silent, packret)
register struct linked_list *item, **packret;
bool silent;
{
    register struct linked_list *ip, *lp, *ap;
    register struct object *obj, *op;
    register bool exact, from_floor;

    if (packret != NULL)
	*packret = NULL;

    if (item == NULL)
    {
	from_floor = TRUE;
	if ((item = find_obj(hero.y, hero.x)) == NULL)
	    return(FALSE);
    }
    else
	from_floor = FALSE;
    obj = OBJPTR(item);
    /*
     * If it is gold, just add its value to rogue's purse and get rid
     * of it.
     */
    if (obj->o_type == GOLD) {
	register struct linked_list *mitem;
	register struct thing *tp;

	if (!silent) {
	    if (!terse) addmsg("You found ");
	    msg("%d gold pieces.", obj->o_count);
	}

	/* First make sure no greedy monster is after this gold.
	 * If so, make the monster run after the rogue instead.
	 */
	 for (mitem = mlist; mitem != NULL; mitem = next(mitem)) {
	    tp = THINGPTR(mitem);
	    if (tp->t_dest == &obj->o_pos) tp->t_dest = &hero;
	}

	purse += obj->o_count;
	if (from_floor) {
	    detach(lvl_obj, item);
	    if ((ap = find_obj(hero.y, hero.x)) == NULL)
		mvaddch(hero.y,hero.x,(roomin(&hero)==NULL ? PASSAGE : FLOOR));
	    else 
		mvaddch(hero.y,hero.x,(OBJPTR(ap))->o_type);
	}
	o_discard(item);
	return(TRUE);
    }

    /*
     * see if he can carry any more weight
     */
    if (itemweight(obj) + pstats.s_pack > pstats.s_carry) {
	msg("Too much for you to carry.");
	return FALSE;
    }
    /*
     * Link it into the pack.  Search the pack for a object of similar type
     * if there isn't one, stuff it at the beginning, if there is, look for one
     * that is exactly the same and just increment the count if there is.
     * it  that.  Food is always put at the beginning for ease of access, but
     * is not ordered so that you can't tell good food from bad.  First check
     * to see if there is something in thr same group and if there is then
     * increment the count.
     */
    if (obj->o_group)
    {
	for (ip = pack; ip != NULL; ip = next(ip))
	{
	    op = OBJPTR(ip);
	    if (op->o_group == obj->o_group)
	    {
		/*
		 * Put it in the pack and notify the user
		 */
		op->o_count += obj->o_count;
		if (from_floor)
		{
		    detach(lvl_obj, item);
		    if ((ap = find_obj(hero.y, hero.x)) == NULL)
			mvaddch(hero.y,hero.x,
				(roomin(&hero)==NULL ? PASSAGE : FLOOR));
		    else 
			mvaddch(hero.y,hero.x,(OBJPTR(ap))->o_type);
		}
		o_discard(item);
		item = ip;
		goto picked_up;
	    }
	}
    }

    /*
     * Check for and deal with scare monster scrolls
     */
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	if (obj->o_flags & ISCURSED)
	{
	    msg("The scroll turns to dust as you pick it up.");
	    detach(lvl_obj, item);
	    if ((ap = find_obj(hero.y, hero.x)) == NULL)
		mvaddch(hero.y,hero.x,(roomin(&hero)==NULL ? PASSAGE : FLOOR));
	    else 
		mvaddch(hero.y,hero.x,(OBJPTR(ap))->o_type);
	    return(TRUE);
	}

    /*
     * Search for an object of the same type
     */
    exact = FALSE;
    for (ip = pack; ip != NULL; ip = next(ip))
    {
	op = OBJPTR(ip);
	if (obj->o_type == op->o_type)
	    break;
    }
    if (ip == NULL)
    {
	/*
	 * Put it at the end of the pack since it is a new type
	 */
	for (ip = pack; ip != NULL; ip = next(ip))
	{
	    op = OBJPTR(ip);
	    if (op->o_type != FOOD)
		break;
	    lp = ip;
	}
    }
    else
    {
	/*
	 * Search for an object which is exactly the same
	 */
	while (ip != NULL && op->o_type == obj->o_type)
	{
	    if (op->o_which == obj->o_which)
	    {
		exact = TRUE;
		break;
	    }
	    lp = ip;
	    if ((ip = next(ip)) == NULL)
		break;
	    op = OBJPTR(ip);
	}
    }
    /*
     * Check if there is room
     */
    if (ip == NULL || !exact || !ISMULT(obj->o_type)) {
	if (inpack == MAXPACK-1) {
	    msg(terse ? "No room." : "You can't carry anything else.");
	    return(FALSE);
	}
    }
    inpack++;
    if (from_floor)
    {
	detach(lvl_obj, item);
	if ((ap = find_obj(hero.y, hero.x)) == NULL)
	    mvaddch(hero.y,hero.x,(roomin(&hero)==NULL ? PASSAGE : FLOOR));
	else 
	    mvaddch(hero.y,hero.x,(OBJPTR(ap))->o_type);
    }
    if (ip == NULL)
    {
	/*
	 * Didn't find an exact match, just stick it here
	 */
	if (pack == NULL)
	    pack = item;
	else
	{
	    lp->l_next = item;
	    item->l_prev = lp;
	    item->l_next = NULL;
	}
    }
    else
    {
	/*
	 * If we found an exact match.  If it is food,
	 * increase the count, otherwise put it with its clones.
	 */
	if (exact && ISMULT(obj->o_type))
	{
	    op->o_count += obj->o_count;
	    inpack--;			/* adjust for previous addition */
	    o_discard(item);
	    item = ip;
	    goto picked_up;
	}
	if ((item->l_prev = prev(ip)) != NULL)
	    item->l_prev->l_next = item;
	else
	    pack = item;
	item->l_next = ip;
	ip->l_prev = item;
    }
picked_up:
    /*
     * Notify the user
     */
    obj = OBJPTR(item);
    if (!silent)
    {
	if (!terse)
	    addmsg("You now have ");
	msg("%s (%c)", inv_name(obj, !terse), pack_char(pack, obj));
    }

    /* Relics can do strange things when you pick them up */
    if (obj->o_type == RELIC) {
	switch (obj->o_which) {
	    /* the ankh of Heil gives you prayers */
	    case HEIL_ANKH:
		msg("The ankh welds itself into your hand.");
		if (player.t_ctype != C_CLERIC && player.t_ctype != C_PALADIN)
		    fuse(prayer_recovery, 0, SPELLTIME, AFTER);

	    /* A cloak must be worn. */
	    when EMORI_CLOAK:
		if (cur_armor != NULL || cur_misc[WEAR_CLOAK]) {
		    msg("The cloak insists you remove your current garments.");
		    if (!dropcheck(cur_armor != NULL ? cur_armor
						     : cur_misc[WEAR_CLOAK])) {
			pstats.s_hpt = -1;
			msg("The cloak constricts around you.");
			msg("It draws your life force from you!!! -- More --");
			wait_for(' ');
			death(D_RELIC);
		    }
		}
		if (obj->o_charges < 0) /* should never happen, but.... */
		    obj->o_charges = 0;
		if (obj->o_charges == 0)
			fuse(cloak_charge, obj, CLOAK_TIME, AFTER);

	    /* The amulet must be worn. */
	    when STONEBONES_AMULET:
	    case YENDOR_AMULET:
		if (cur_misc[WEAR_JEWEL]		|| 
		    cur_relic[STONEBONES_AMULET]	||
		    cur_relic[YENDOR_AMULET]) {
		        msg("You have an urge to remove your current amulet.");
		}
		if((cur_misc[WEAR_JEWEL] && !dropcheck(cur_misc[WEAR_JEWEL])) ||
		    cur_relic[STONEBONES_AMULET]			      ||
		    cur_relic[YENDOR_AMULET]) {
			pstats.s_hpt = -1;
			msg("The %s begins pulsing.", inv_name(obj, TRUE));
			msg("It fades away.... -- More --");
			wait_for(' ');
			death(D_RELIC);
		}
		msg("The %s welds itself into your chest.",inv_name(obj,TRUE));

	    /* The eye must be inserted in eye socket */
	    when EYE_VECNA:
		msg("The eye forces you to jam it into your eye socket!");
		pstats.s_hpt -= 75;
		if (pstats.s_hpt < 0) {
			msg ("The pain is too much for you! -- More --");
			wait_for(' ');
			death(D_RELIC);
		}
		waste_time();
		msg("The excrutiating pain slowly turns into a dull throb.");
		
	    when QUILL_NAGROM:
	        fuse(quill_charge,0,player.t_ctype==C_MAGICIAN ? 4 : 8,AFTER);

	    /* Weapons will insist on being wielded. */
	    when MUSTY_DAGGER:
	    case HRUGGEK_MSTAR:
	    case YEENOGHU_FLAIL:
	    case AXE_AKLAD:
		/* For the daggers start a fuse to change player to a thief. */
		/* and set a daemon to eat gold.			     */
		if (obj->o_which == MUSTY_DAGGER) {
		    fuse(changeclass, C_THIEF, roll(20, 20), AFTER);
		    if (purse > 0)
			msg("Your purse feels lighter");
		    else
			purse = 1; /* fudge to get right msg from eat_gold() */
		    eat_gold(obj);
		    daemon(eat_gold, obj, AFTER);
		}
		/* For the axe start a fuse to change player to a fighter. */
		if (obj->o_which == AXE_AKLAD)
		    fuse(changeclass, C_FIGHTER, roll(20, 20), AFTER);
		if (cur_weapon != NULL) {
		    msg("The artifact insists you release your current weapon.");
		    if (!dropcheck(cur_weapon)) {
			pstats.s_hpt = -1;
			msg("The artifact forces your weapon into your heart.");
			msg("It hums with satisfaction. -- More --");
			wait_for(' ');
			death(D_RELIC);
		    }
		}
		cur_weapon = obj;

	when SURTUR_RING:
		msg("The ring forces itself through your nose!");
		pstats.s_hpt -= 22;
		if (pstats.s_hpt < 0) {
			msg ("The pain is too much for you! -- More --");
			wait_for(' ');
			death(D_RELIC);
		}
		waste_time();
		turn_on(player, NOFIRE);
		msg("The pain slowly subsides.");
	otherwise:
		break;
	}
	cur_relic[obj->o_which]++;	/* Note that we have it */
    }

    updpack(FALSE, &player);
    if (packret != NULL)
	*packret = item;
    return(TRUE);
}

#ifdef PC7300
static menu_t Display;				/* The menu structure */
static mitem_t Dispitems[MAXPACK+1];		/* Info for each line */
static char Displines[MAXPACK+1][LINELEN+1];	/* The lines themselves */
#endif

/*
 * inventory:
 *	list what is in the pack
 */
inventory(list, type)
register struct linked_list *list;
register int type;
{
    register struct object *obj;
    register char ch;
    register int n_objs, cnt, maxx, curx;
    char inv_temp[2*LINELEN+1];

    cnt = 0;
    n_objs = 0;
    for (ch = 'a'; list != NULL; ch++, list = next(list)) {
	obj = OBJPTR(list);
	if (!is_type(obj, type))
	    continue;
	switch (n_objs++) {
	    /*
	     * For the first thing in the inventory, just save the string
	     * in case there is only one.
	     */
	    case 0:
		sprintf(inv_temp, "%c) %s", ch, inv_name(obj, FALSE));
		break;
	    /*
	     * If there is more than one, clear the screen, print the
	     * saved message and fall through to ...
	     */
	    case 1:
		if (slow_invent)
		    msg(inv_temp);
		else
		{
		    wclear(hw);
		    waddstr(hw, inv_temp);
		    waddch(hw, '\n');

		    maxx = strlen(inv_temp);	/* Length of the listing */

#ifdef PC7300
		    /* Put it into the PC menu display */
		    strcpy(Displines[0], inv_temp);
		    Dispitems[0].mi_name = Displines[0];
		    Dispitems[0].mi_flags = 0;
		    Dispitems[0].mi_val = 0;
#endif
		}
	    /*
	     * Print the line for this object
	     */
	    default:
		if (ch > 'z')
		    ch = 'A';
		if (slow_invent)
		    msg("%c) %s", ch, inv_name(obj, FALSE));
		else {
		    if (++cnt >= lines - 2) { /* if bottom of screen */
			dbotline(hw, morestr);
			cnt = 0;
			wclear(hw);
		    }
		    sprintf(inv_temp, "%c) %s\n", ch, inv_name(obj, FALSE));
		    curx = strlen(inv_temp) - 1; /* Don't count new-line */
		    if (curx > maxx) maxx = curx;
		    waddstr(hw, inv_temp);
#ifdef PC7300
		    /* Put it into the PC menu display */
		    strcpy(Displines[n_objs-1], inv_temp);
		    Displines[n_objs-1][curx] = '\0'; /* Strip newline */
		    Dispitems[n_objs-1].mi_name = Displines[n_objs-1];
		    Dispitems[n_objs-1].mi_flags = 0;
		    Dispitems[n_objs-1].mi_val = 0;
#endif
		}
	}
    }
    if (n_objs == 0) {
	if (terse)
	    msg(type == ALL ? "Empty handed." :
			    "Nothing appropriate");
	else
	    msg(type == ALL ? "You are empty handed." :
			    "You don't have anything appropriate");
	return FALSE;
    }
    if (n_objs == 1) {
	msg(inv_temp);
	return TRUE;
    }
    if (!slow_invent)
    {
#ifdef PC7300
	/* Place an end marker for the items */
	Dispitems[n_objs].mi_name = 0;

	/* Set up the main menu structure */
	Display.m_label = "Inventory";
	Display.m_title = "Pack Contents";
	Display.m_prompt = "Press Cancl to continue.";
	Display.m_curptr = '\0';
	Display.m_markptr = '\0';
	Display.m_flags = 0;
	Display.m_selcnt = 0;
	Display.m_items = Dispitems;
	Display.m_curi = 0;

	/*
	 * Try to display the menu.  If we don't have a local terminal,
	 * the call will fail and we will just continue with the
	 * normal mode.
	 */
	if (menu(&Display) >= 0) return TRUE;
#endif
	waddstr(hw, spacemsg);
	curx = strlen(spacemsg);
	if (curx > maxx) maxx = curx;

	/*
	 * If we have fewer than half a screenful, don't clear the screen.
	 * Leave an extra blank line at the bottom and 3 blank columns
	 * to he right.
	 */
	if (menu_overlay && n_objs < lines / 2 + 2) {
	    over_win(cw, hw, n_objs + 2, maxx + 3, n_objs, curx, ' ');
	    return TRUE;
	}

	draw(hw);
	wait_for(' ');
	clearok(cw, TRUE);
	touchwin(cw);
    }
    return TRUE;
}

/*
 * picky_inven:
 *	Allow player to inventory a single item
 */
void
picky_inven()
{
    register struct linked_list *item;
    register char ch, mch;

    if (pack == NULL)
	msg("You aren't carrying anything");
    else if (next(pack) == NULL)
	msg("a) %s", inv_name(OBJPTR(pack), FALSE));
    else
    {
	msg(terse ? "Item: " : "Which item do you wish to inventory: ");
	mpos = 0;
	if ((mch = readchar()) == ESCAPE)
	{
	    msg("");
	    return;
	}

	/* Check for a special character */
	switch (mch) {
	    case FOOD:
	    case SCROLL:
	    case POTION:
	    case RING:
	    case STICK:
	    case RELIC:
	    case ARMOR:
	    case WEAPON:
	    case MM:
		msg("");
		if (get_item(pack, NULL, mch, FALSE, FALSE) == NULL) {
		    if (terse) msg("None in pack.");
		    else msg("You have no %c in your pack.", mch);
		}
		return;
	}

	for (ch = 'a', item = pack; item != NULL; item = next(item), ch++)
	    if (ch == mch)
	    {
		msg("%c) %s",ch,inv_name(OBJPTR(item), FALSE));
		return;
	    }
	if (!terse)
	    msg("'%s' not in pack.", unctrl(mch));
	msg("Range is 'a' to '%c'", --ch);
    }
}


/*
 * get_item:
 *	pick something out of a pack for a purpose
 */
struct linked_list *
get_item(list, purpose, type, askfirst, showcost)
reg struct linked_list *list;
char *purpose;	/* NULL if we should be silent (no prompts) */
int type;
bool askfirst, showcost;
{
    reg struct linked_list *item;
    reg struct object *obj;
    reg int cnt, pagecnt, ch, och, maxx, curx, confused;
    struct linked_list *saveitem;
    char description[2*LINELEN+1];
    char cost[LINELEN/2];
#ifdef PC7300
    int menucount = 0;
    int usemenu = 1;
#endif

    /*
     * If this is the player's pack and the player is confused, we
     * might just take anything.
     */
    if (list == player.t_pack && on(player, ISHUH) && rnd(100) < 75)
	confused = 1;
    else confused = 0;

    cnt = 0;
    if (list == NULL) {
	msg("You aren't carrying anything.");
	return NULL;
    }
    /* see if we have any of the type requested */
    for(ch = 'a',item = list ; item != NULL ; item = next(item), ch++) {
	obj = OBJPTR(item);
	if (is_type(obj, type)) {
	    cnt++;
	    saveitem = item;
	}
    }
    if (cnt == 0) {
	if (purpose) msg("Nothing to %s", purpose);
	after = FALSE;
	return NULL;
    }
    else if (cnt == 1) {	/* only found one of 'em */
	obj = OBJPTR(saveitem);
	while(TRUE)  {
	    if (purpose) {	/* Should we prompt the player? */
		msg("%s what (* for the item)?",purpose);
		ch = tolower(readchar());
	    }
	    else {
		ch = pack_char(list, obj);
		msg("%c) %s", ch, inv_name(obj,FALSE));
	    }

	    if (ch == '*') {
		mpos = 0;
		msg("%c) %s",pack_char(list, obj),inv_name(obj,FALSE));
		continue;
	    }
	    if (ch == ESCAPE) {
		msg("");
		after = FALSE;
		return NULL;
	    }
	    for(item = list,och = 'a'; item != NULL; item = next(item),och++) {
		if (ch == och) break;
		if (och == 'z') och = 'A' - 1;
	    }
	    if (item == NULL) {
		msg("Please specify a letter between 'a' and '%c'",
		    och == 'A' ? 'z' : och-1);
		continue;
	    }
	    if (is_type (OBJPTR(item), type)) {
		if (purpose) mpos = 0;
		return item;
	    }
	    else
		msg ("You can't %s that!", purpose);

	} 
    }
    while (TRUE) {
	if (!askfirst && purpose) {
	    msg("%s what? (* for list): ",purpose);
	    ch = readchar();
	}
	else ch = '*';

	mpos = 0;
	if (ch == ESCAPE) {		/* abort if escape hit */
	    after = FALSE;
	    msg("");		/* clear display */
	    return NULL;
	}

	if (ch == '*') {
	    wclear(hw);
	    pagecnt = 0;
	    maxx = 0;
	    for(item = list,ch = 'a'; item != NULL ; item = next(item), ch++) {
		obj = OBJPTR(item);
		if (!is_type(OBJPTR(item), type))
		    continue;
		cost[0] = '\0';
		if (showcost) {
		    sprintf(description, "[%d] ", get_worth(obj));
		    sprintf(cost, "%8.8s", description);
		}
		sprintf(description,"%c) %s%s\n\r",ch,cost,inv_name(obj,FALSE));
		waddstr(hw, description);
		curx = strlen(description) - 2;	/* Don't count \n or \r */
		if (maxx < curx) maxx = curx;
#ifdef PC7300
		if (usemenu) {
		    /* Put it into the PC menu display */
		    strcpy(Displines[menucount], description);
		    Displines[menucount][curx] = '\0'; /* Strip newline */
		    Dispitems[menucount].mi_name = Displines[menucount];
		    Dispitems[menucount].mi_flags = 0;
		    Dispitems[menucount++].mi_val = (int) item;
		}
#endif
		if (++pagecnt >= lines - 2 && next(item) != NULL) {
		    pagecnt = 0;
		    dbotline(hw, spacemsg);
		    wclear(hw);
		}
		if (ch == 'z') ch = 'A' - 1;
	    }

	    /* Put in the prompt */
	    if (purpose) sprintf(description, "%s what? ", purpose);
	    else strcpy(description, spacemsg);
	    waddstr(hw, description);
	    curx = strlen(description);
	    if (maxx < curx) maxx = curx;

#ifdef PC7300
	    if (usemenu) {
		/* Place an end marker for the items */
		Dispitems[menucount].mi_name = 0;

		/* Set up the main menu structure */
		Display.m_label = "Sub-Inventory";
		if (purpose) {
		    Display.m_title = description;
		    Display.m_prompt =
			"Select an item or press Cancl for no selection.";
		    Display.m_selcnt = 1;
		}
		else {
		    Display.m_title = 0;
		    Display.m_prompt = "Press Cancl to continue.";
		    Display.m_selcnt = 0;
		}
		Display.m_curptr = '\0';
		Display.m_markptr = '\0';
		Display.m_flags = 0;
		Display.m_items = Dispitems;
		Display.m_curi = 0;

		/*
		 * Try to display the menu.  If we don't have a local terminal,
		 * the call will fail and we will just continue with the
		 * normal mode.
		 */
		if (menu(&Display) >= 0) {
		    msg("");
		    if (Display.m_selcnt == 0) {
			/* Menu was cancelled */
			if (purpose) {
			    after = FALSE;
			    return NULL;	/* all done if abort */
			}
			else return saveitem;
		    }
		    else return (struct linked_list *) Display.m_curi->mi_val;
		}
		else {
		    usemenu = 0;	/* Can't use the menu facilities */
		}
	    }
#endif
	    /* Write the screen */
	    if ((menu_overlay && cnt < lines / 2 + 2) || cnt == 1) {
		over_win(cw, hw, cnt + 2, maxx + 3, cnt, curx, NULL);
		cnt = -1;	/* Indicate we used over_win */
	    }
	    else draw(hw);

	    if (purpose) {
		do {
		    ch = tolower(readchar());
		} until (isalpha(ch) || ch == ESCAPE);
	    }
	    else {
		ch = pack_char(list, OBJPTR(saveitem)); /* Pick a valid item */
		wait_for(' ');
	    }

	    /* Redraw original screen */
	    if (cnt < 0) {
		clearok(cw, FALSE);	/* Setup to redraw current screen */
		touchwin(cw);	/* clearing first */
	    }
	    else restscr(cw);

	    if(ch == ESCAPE) {
		after = FALSE;
		msg("");		/* clear top line */
		return NULL;	/* all done if abort */
	    }
	    /* ch has item to get from list */
	}

	for (item = list,och = 'a'; item != NULL; item = next(item),och++) {
	    if (confused) {
		/*
		 * Confused is incremented each time so that if the rnd(cnt)
		 * clause keeps failing, confused will equal cnt for the
		 * last item of the correct type and rnd(cnt) < cnt will
		 * have to be true.
		 */
		if (is_type(OBJPTR(item), type) && rnd(cnt) < confused++)
		    break;
	    }
	    else if (ch == och) break;
	    if (och == 'z') och = 'A' - 1;
	}

	if (item == NULL) {
	    msg("Please specify a letter between 'a' and '%c'",
		och == 'A' ? 'z' : och-1);
	    continue;
	}

	if (is_type(OBJPTR(item), type))
	    return (item);
	else
	    msg ("You can't %s that!", purpose);
    }
}

pack_char(list, obj)
register struct object *obj;
struct linked_list *list;
{
    register struct linked_list *item;
    register char c;

    c = 'a';
    for (item = list; item != NULL; item = next(item)) {
	if (OBJPTR(item) == obj)
	    return c;
	else {
	    if (c == 'z') c = 'A';
	    else c++;
	}
    }
    return 'z';
}


/*
 * cur_null:
 *	This updates cur_weapon etc for dropping things
 */
cur_null(op)
reg struct object *op;
{
	if (op == cur_weapon)			cur_weapon = NULL;
	else if (op == cur_armor)		cur_armor = NULL;
	else if (op == cur_ring[LEFT_1])	cur_ring[LEFT_1] = NULL;
	else if (op == cur_ring[LEFT_2])	cur_ring[LEFT_2] = NULL;
	else if (op == cur_ring[LEFT_3])	cur_ring[LEFT_3] = NULL;
	else if (op == cur_ring[LEFT_4])	cur_ring[LEFT_4] = NULL;
	else if (op == cur_ring[RIGHT_1])	cur_ring[RIGHT_1] = NULL;
	else if (op == cur_ring[RIGHT_2])	cur_ring[RIGHT_2] = NULL;
	else if (op == cur_ring[RIGHT_3])	cur_ring[RIGHT_3] = NULL;
	else if (op == cur_ring[RIGHT_4])	cur_ring[RIGHT_4] = NULL;
	else if (op == cur_misc[WEAR_BOOTS])	cur_misc[WEAR_BOOTS] = NULL;
	else if (op == cur_misc[WEAR_JEWEL])	cur_misc[WEAR_JEWEL] = NULL;
	else if (op == cur_misc[WEAR_GAUNTLET]) cur_misc[WEAR_GAUNTLET] = NULL;
	else if (op == cur_misc[WEAR_CLOAK])	cur_misc[WEAR_CLOAK] = NULL;
	else if (op == cur_misc[WEAR_BRACERS])	cur_misc[WEAR_BRACERS] = NULL;
	else if (op == cur_misc[WEAR_NECKLACE]) cur_misc[WEAR_NECKLACE] = NULL;
}

/*
 * idenpack:
 *	Identify all the items in the pack
 */
idenpack()
{
	reg struct linked_list *pc;

	for (pc = pack ; pc != NULL ; pc = next(pc))
		whatis(pc);
}

is_type (obj, type)
register struct object *obj;
register int type;
{
    register bool current;

    if (type == obj->o_type)
	return (TRUE);

    switch (type) {
	case ALL:
	    return (TRUE);
	when READABLE:
	    if (obj->o_type == SCROLL ||
		(obj->o_type == MM && obj->o_which == MM_SKILLS))
		    return (TRUE);
	when QUAFFABLE:
	    if (obj->o_type == POTION ||
		(obj->o_type == MM && obj->o_which == MM_JUG))
		    return (TRUE);
	when ZAPPABLE:
	    if (obj->o_type == STICK) return (TRUE);
	    if (obj->o_type == RELIC)
		switch (obj->o_which) {
		    case MING_STAFF:
		    case ASMO_ROD:
		    case ORCUS_WAND:
		    case EMORI_CLOAK:
			return (TRUE);
		}
	when WEARABLE:
	case REMOVABLE:
	    current = is_current(obj);

	    /*
	     * Don't wear thing we are already wearing or remove things
	     * we aren't wearing.
	     */
	    if (type == WEARABLE && current) return (FALSE);
	    else if (type == REMOVABLE && !current) return (FALSE);

	    switch (obj->o_type) {
		case RELIC:
		    switch (obj->o_which) {
			case HEIL_ANKH:
			case EMORI_CLOAK:
			    return (TRUE);
		    }
		when MM:
		    switch (obj->o_which) {
			case MM_ELF_BOOTS:
			case MM_DANCE:
			case MM_BRACERS:
			case MM_DISP:
			case MM_PROTECT:
			case MM_G_DEXTERITY:
			case MM_G_OGRE:
			case MM_JEWEL:
			case MM_R_POWERLESS:
			case MM_FUMBLE:
			case MM_STRANGLE:
			case MM_ADAPTION:
			    return (TRUE);
		    }
		when ARMOR:
		case RING:
		    return (TRUE);
	    }
	when CALLABLE:
	    switch (obj->o_type) {
	    case RING:		if (!r_know[obj->o_which]) return(TRUE);
	    when POTION:	if (!p_know[obj->o_which]) return(TRUE);
	    when STICK:		if (!ws_know[obj->o_which]) return(TRUE);
	    when SCROLL:	if (!s_know[obj->o_which]) return(TRUE);
	    when MM:		if (!m_know[obj->o_which]) return(TRUE);
	    }
	when WIELDABLE:
	    switch (obj->o_type) {
		case STICK:
		case WEAPON:
		    return(TRUE);
		when RELIC:
		    switch (obj->o_which) {
			case MUSTY_DAGGER:
			case HRUGGEK_MSTAR:
			case YEENOGHU_FLAIL:
			case AXE_AKLAD:
			case MING_STAFF:
			case ORCUS_WAND:
			case ASMO_ROD:
			    return(TRUE);
		    }
	    }
	when IDENTABLE:
	    if (!(obj->o_flags & ISKNOW) && obj->o_type != FOOD)
		return (TRUE);
	    if (obj->o_type == MM) {
	      switch (obj->o_which) {
		case MM_JUG:
		    /* Can still identify a jug if we don't know the potion */
		    if (obj->o_ac != JUG_EMPTY && !p_know[obj->o_ac])
			return (TRUE);
	      }
	    }
	when USEABLE:
	    if (obj->o_type == MM) {
		switch(obj->o_which) {
		case MM_BEAKER:
		case MM_BOOK:
		case MM_OPEN:
		case MM_HUNGER:
		case MM_DRUMS:
		case MM_DISAPPEAR:
		case MM_CHOKE:
		case MM_KEOGHTOM:
		    return (TRUE);
		}
	    }
	    else if (obj->o_type == RELIC) {
		switch (obj->o_which) {
		    case EMORI_CLOAK:
		    case BRIAN_MANDOLIN:
		    case HEIL_ANKH:
		    case YENDOR_AMULET:
		    case STONEBONES_AMULET:
		    case GERYON_HORN:
		    case EYE_VECNA:
		    case QUILL_NAGROM:
		    case SURTUR_RING:
			return (TRUE);
		}
	    }
	    else if (obj->o_type == POTION) {
		/*
		 * only assassins can use poison
		 */
		if (player.t_ctype == C_ASSASIN && obj->o_which == P_POISON)
		    return(TRUE);
	    }
	when PROTECTABLE:
	    switch (obj->o_type) {
		case WEAPON:
		    if ((obj->o_flags & ISMETAL) == 0) return (FALSE);

		    /* Fall through */
		case ARMOR:
		    return (TRUE);

		when MM:
		    if (obj->o_which == MM_BRACERS) return (TRUE);
	    }
    }
    return(FALSE);
}

del_pack(item)
register struct linked_list *item;
{
    register struct object *obj;

    obj = OBJPTR(item);
    if (obj->o_count > 1) {
	obj->o_count--;
    }
    else {
	cur_null(obj);
	detach(pack, item);
	o_discard(item);
	inpack--;
    }
}

/*
 * carry_obj:
 *	Check to see if a monster is carrying something and, if so, give
 * it to him.
 */

carry_obj(mp, chance)
register struct thing *mp;
int chance;
{
    reg struct linked_list *item;
    reg struct object *obj;

    /* 
     * If there is no chance, just return.
     * Note that this means there must be a "chance" in order for
     * the creature to carry a relic.
     */
    if (chance <= 0) return;

    /* 
     * check for the relic/artifacts 
     * Do the relics first so they end up last in the pack. Attach()
     * always adds things to the beginning. This way they will be the
     * last things dropped when the creature is killed. This will ensure
     * the relic will be on top if there is a stack of item lying on the
     * floor and so the hero will know where it is if he's trying to
     * avoid it. Note that only UNIQUEs carry relics.
     */
    if (on(*mp, ISUNIQUE)) {
	if (on(*mp, CARRYMDAGGER)) {
	    item = spec_item(RELIC, MUSTY_DAGGER, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYCLOAK)) {
	    item = spec_item(RELIC, EMORI_CLOAK, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYANKH)) {
	    item = spec_item(RELIC, HEIL_ANKH, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYSTAFF)) {
	    item = spec_item(RELIC, MING_STAFF, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYWAND)) {
	    item = spec_item(RELIC, ORCUS_WAND, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYROD)) {
	    item = spec_item(RELIC, ASMO_ROD, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYYAMULET)) {
	    item = spec_item(RELIC, YENDOR_AMULET, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYBAMULET)) {
	    item = spec_item(RELIC, STONEBONES_AMULET, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}

	if (on(*mp, CARRYMANDOLIN)) {
	    item = spec_item(RELIC, BRIAN_MANDOLIN, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYEYE)) {
	    item = spec_item(RELIC, EYE_VECNA, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYAXE)) {
	    item = spec_item(RELIC, AXE_AKLAD, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYQUILL)) {
	    register int i, howmany;

	    item = spec_item(RELIC, QUILL_NAGROM, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    obj->o_charges = rnd(QUILLCHARGES);
	    attach(mp->t_pack, item);
	    howmany = roll(4,3);
	    for (i=0; i<howmany; i++) {
		/*
		 * the quill writes scrolls so give him a bunch
		 */
		item = new_thing(TYP_SCROLL, FALSE);
		obj = OBJPTR(item);
		obj->o_pos = mp->t_pos;
		attach(mp->t_pack, item);
	    }
	}
	if (on(*mp, CARRYMSTAR)) {
	    item = spec_item(RELIC, HRUGGEK_MSTAR, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYFLAIL)) {
	    item = spec_item(RELIC, YEENOGHU_FLAIL, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYHORN)) {
	    item = spec_item(RELIC, GERYON_HORN, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
	if (on(*mp, CARRYSURTURRING)) {
	    item = spec_item(RELIC, SURTUR_RING, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
	}
    }
    /*
     * If it carries gold, give it some
     */
    if (on(*mp, CARRYGOLD) && rnd(100) < chance) {
	    item = spec_item(GOLD, NULL, NULL, NULL);
	    obj = OBJPTR(item);
	    obj->o_count = GOLDCALC + GOLDCALC;
	    obj->o_pos = mp->t_pos;
	    attach(mp->t_pack, item);
    }

    /*
     * If it carries food, give it some
     */
    if (on(*mp, CARRYFOOD) && rnd(100) < chance) {
	item = spec_item(FOOD, NULL, NULL, NULL);
	obj = OBJPTR(item);
	obj->o_weight = things[TYP_FOOD].mi_wght;
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries a weapon, give it one
     */
    if (on(*mp, CARRYWEAPON) && rnd(100) < chance) {
	int type, hit, dam;

	/* Get the "bonuses" */
	hit = rnd(5 + (vlevel / 5)) - 2;
	dam = rnd(5 + (vlevel / 5)) - 2;

	/* Only choose an appropriate type of weapon */
	switch (rnd(12)) {
	    case 0: type = DAGGER;
	    when 1: type = BATTLEAXE;
	    when 2: type = MACE;
	    when 3: type = SWORD;
	    when 4: type = PIKE;
	    when 5: type = HALBERD;
	    when 6: type = SPETUM;
	    when 7: type = BARDICHE;
	    when 8: type = TRIDENT;
	    when 9: type = BASWORD;
	    when 10:type = DART;
	    otherwise: type = TWOSWORD;
	}

	/* Create the item */
	item = spec_item(WEAPON, type, hit, dam);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries a dagger, give it one
     */
    if (on(*mp, CARRYDAGGER) && rnd(100) < chance) {
	int hit, dam;

	/* Get the "bonuses" */
	hit = rnd(3 + (vlevel / 5)) - 1;
	dam = rnd(3 + (vlevel / 5)) - 1;

	/* Create the item */
	item = spec_item(WEAPON, DAGGER, hit, dam);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries a scroll, give it one
     */
    if (on(*mp, CARRYSCROLL) && rnd(100) < chance) {
	item = new_thing(TYP_SCROLL, TRUE);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;

	/* Can the monster carry this scroll? */
	if (obj->o_which == S_SCARE && mp->t_stats.s_intel < 16)
	    fall(item, FALSE);	/* This would scare us! */
	else attach(mp->t_pack, item);
    }

    /*
     * If it carries a potion, give it one
     */
    if (on(*mp, CARRYPOTION) && rnd(100) < chance) {
	item = new_thing(TYP_POTION, TRUE);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries a ring, give it one
     */
    if (on(*mp, CARRYRING) && rnd(100) < chance) {
	item = new_thing(TYP_RING, TRUE);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries a wand or staff, give it one
     */
    if (on(*mp, CARRYSTICK) && rnd(100) < chance) {
	item = new_thing(TYP_STICK, TRUE);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /*
     * If it carries any miscellaneous magic, give it one
     */
    if (on(*mp, CARRYMISC) && rnd(100) < chance) {
	item = new_thing(TYP_MM, TRUE);
	obj = OBJPTR(item);
	obj->o_pos = mp->t_pos;
	attach(mp->t_pack, item);
    }

    /* Update the monster's encumberance */
    updpack(TRUE, mp);
}


/*
 * grab():
 *	See what is on the spot where the player is standing.  If
 *	nothing is there, do nothing.  If there is one thing, pick it
 *	up.  If there are multiple things, prompt the player for what
 *	he wants (* means everything).
 */

grab(y, x)
register y, x;
{
    register struct linked_list *next_item, *item;
    register struct object *obj;
    register int cnt, pagecnt;
    int num_there = 0, ch, och;

    /*
     * Count how many objects there are and move them to the front
     * of the level list.
     */
    for (item = lvl_obj; item != NULL; item = next_item) {
	obj = OBJPTR(item);
	next_item = next(item);
	if (obj->o_pos.y == y && obj->o_pos.x == x) {
	    num_there++;
	    detach(lvl_obj, item);	/* Remove it from the list */
	    attach(lvl_obj, item);	/* Place it at the front of the list */
	}
    }

    /* Nothing there. */
    if (num_there < 1) msg("Nothing %s", terse ? "there." : "to pick up.");

    /* Something or things there */
    else {
	char linebuf[2*LINELEN+1];
	int curlen, maxlen = 0;

	wclear(hw);
	cnt = 0;
	pagecnt = 0;
	for (item = lvl_obj, ch = 'a'; item != NULL && cnt < num_there;
	     item = next(item), ch++, cnt++) {
	    obj = OBJPTR(item);
	    /* Construct how the line will look */
	    sprintf(linebuf, "%c) %s\n\r", ch, inv_name(obj,FALSE));

	    /* See how long it is */
	    curlen = strlen(linebuf) - 2; /* Don't count \n or \r */
	    if (maxlen < curlen) maxlen = curlen;

	    /* Draw it in the window */
	    waddstr(hw, linebuf);

	    if (++pagecnt >= lines - 2 && next(item) != NULL) {
		pagecnt = 0;
		maxlen = 0;
		dbotline(hw, spacemsg);
		wclear(hw);
	    }
	    if (ch == 'z') ch = 'A' - 1;
	}

	strcpy(linebuf, "Pick up what? (* for all): ");

	/* See how long it is */
	curlen = strlen(linebuf); /* Don't count \n or \r */
	if (maxlen < curlen) maxlen = curlen;

	/* Draw it in the window */
	waddstr(hw, linebuf);

	/*
	 * If we have fewer than half a screenful, don't clear the screen.
	 * Leave an extra blank line at the bottom and 3 blank columns
	 * to he right.
	 */
	if (menu_overlay && num_there < lines / 2 + 2) {
	  over_win(cw, hw, num_there + 2, maxlen + 3, num_there, curlen, NULL);
	  pagecnt = -1;	/* Indicate we used over_win */
	}
	else draw(hw);		/* write screen */

	for (;;) {
	    do {
		ch = tolower(readchar());
	    } until (isalpha(ch) || ch == '*' || ch == ESCAPE);

	    /* Redraw original screen */
	    if (pagecnt < 0) {
		clearok(cw, FALSE);	/* Setup to redraw current screen */
		touchwin(cw);	/* clearing first */
	    }
	    else restscr(cw);

	    if (ch == ESCAPE) {
		after = FALSE;
		msg("");		/* clear top line */
		break;
	    }
	    if (ch == '*') {
		player.t_action = A_PICKUP;
		return(1); /* set action to PICKUP and delay for first one */
	    }
	    /* ch has item to get from list */

	    cnt = 0;
	    for (item = lvl_obj, och = 'a'; item != NULL && cnt < num_there;
		 item = next(item), och++, cnt++) {
		if (ch == och)
		    break;
		if (och == 'z') och = 'A' - 1;
	    }
	    if (item == NULL || cnt >= num_there) {
		wmove(hw, pagecnt < 0 ? num_there : pagecnt, 25);
		wprintw(hw, " [between 'a' and '%c']: ",
		    och == 'A' ? 'z' : och-1);
		if (maxlen < 49) maxlen = 49;

		/*
		 * If we have fewer than half a screenful, don't clear the
		 * screen.  Leave an extra blank line at the bottom and
		 * 3 blank columns to he right.
		 */
		if (menu_overlay && num_there < lines / 2 + 2) {
		    over_win(cw, hw, num_there + 2, maxlen + 3,
				num_there, 49, NULL);
		    cnt = -1;	/* Indicate we used over_win */
		}
		else draw(hw);		/* write screen */
		continue;
	    }
	    else {
		detach(lvl_obj, item);
		if (add_pack(item, FALSE, NULL)) {
		    /*
		     * We make sure here that the dungeon floor gets 
		     * updated with what's left on the vacated spot.
		     */
		    if ((item = find_obj(y, x)) == NULL) {
			coord roomcoord;	/* Needed to pass to roomin() */

			roomcoord.y = y;
			roomcoord.x = x;
			mvaddch(y, x,
				(roomin(&roomcoord) == NULL ? PASSAGE : FLOOR));
		    }
		    else mvaddch(y, x, (OBJPTR(item))->o_type);
		    return(1);
		}
		else attach(lvl_obj, item);	/* Couldn't pick it up! */
		break;
	    }
	}
    }

    return(0);
}

/*
 * make_sell_pack:
 *
 *	Create a pack for sellers (a la quartermaster)
 */

make_sell_pack(tp)
struct thing *tp;
{
    reg struct linked_list *item;
    reg struct object *obj;
    reg int sell_type, nitems, i;

    /* Select the items */
    nitems = rnd(6) + 5;

    switch (rnd(9)) {
	/* Armor */
	case 0:
	case 1:
	    turn_on(*tp, CARRYARMOR);
	    sell_type = TYP_ARMOR;
	    break;

	/* Weapon */
	case 2:
	case 3:
	    turn_on(*tp, CARRYWEAPON);
	    sell_type = TYP_WEAPON;
	    break;

	/* Staff or wand */
	case 4:
	    turn_on(*tp, CARRYSTICK);
	    sell_type = TYP_STICK;
	    break;

	/* Ring */
	case 5:
	    turn_on(*tp, CARRYRING);
	    sell_type = TYP_RING;
	    break;

	/* scroll */
	case 6:
	    turn_on(*tp, CARRYSCROLL);
	    sell_type = TYP_SCROLL;
	    break;

	/* potions */
	case 7:
	    turn_on(*tp, CARRYPOTION);
	    sell_type = TYP_POTION;
	    break;

	/* Miscellaneous magic */ 
	case 8:
	    turn_on(*tp, CARRYMISC);
	    sell_type = TYP_MM;
	    break;
    }
    for (i=0; i<nitems; i++) {
	item = new_thing(sell_type, FALSE);
	obj = OBJPTR(item);
	obj->o_pos = tp->t_pos;
	attach(tp->t_pack, item);
    }
}
