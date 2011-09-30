/*
 * eat.c  -  Functions for dealing with digestion
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
#include "rogue.h"

/*
 * eat:
 *	He wants to eat something, so let him try
 */

eat()
{
    register struct linked_list *item;
    int which;
    unsigned long temp;

    if (player.t_action != C_EAT) {
	if ((item = get_item(pack, "eat", FOOD, FALSE, FALSE)) == NULL)
	    return;

	player.t_using = item;	/* Remember what it is */
	player.t_action = C_EAT;	/* We are eating */
	which = (OBJPTR(item))->o_which;
	player.t_no_move = max(foods[which].mi_food/100, 1) * movement(&player);
	return;
    }

    /* We have waited our time, let's eat the food */
    item = player.t_using;
    player.t_using = NULL;
    player.t_action = A_NIL;

    which = (OBJPTR(item))->o_which;
    if ((food_left += foods[which].mi_food) > STOMACHSIZE)
	food_left = STOMACHSIZE;
    del_pack(item);
    if (hungry_state == F_SATIATED && food_left == STOMACHSIZE && rnd(4) == 1) {
	pstats.s_hpt = 0;
	msg ("You choke on all that food and die! -- More --");
	wait_for(' ');
	death(D_FOOD_CHOKE);
    }
    if (food_left >= STOMACHSIZE-MORETIME) {
	hungry_state = F_SATIATED;
	msg ("You have trouble getting all that food down.");
	msg ("Your stomach feels like its about to burst!");
    }
    else {
	hungry_state = F_OKAY;
	switch (rnd(3)) {
	case 0: msg("My, that was a yummy %s", foods[which].mi_name);
	when 1: msg("Mmmm, that was a tasty %s", foods[which].mi_name);
	when 2: msg("Wow, that was a scrumptious %s", foods[which].mi_name);
	}
    }
    updpack(TRUE, &player);
    switch(which) {
    case E_WHORTLEBERRY:
	add_abil[A_INTELLIGENCE](1);

    when E_SWEETSOP:
	add_abil[A_DEXTERITY](1);

    when E_SOURSOP:
	add_abil[A_STRENGTH](1);

    when E_SAPODILLA:
	add_abil[A_WISDOM](1);

    when E_RAMBUTAN:
	add_abil[A_CONSTITUTION](1);

    when E_PEACH:
	add_abil[A_CHARISMA](1);

    when E_STRAWBERRY:
	temp = pstats.s_exp/100;
	pstats.s_exp += temp;
	if (temp > 0)
	    msg("You feel slightly more experienced now");
	check_level();

    when E_PITANGA:
	max_stats.s_hpt++;
	pstats.s_hpt++;
	if (terse)
	    msg("You feel a bit tougher now");
	else
	    msg("You feel a bit tougher now. Go get 'em!");
    
    when E_HAGBERRY:
	pstats.s_arm--;
	msg("Your skin feels more resilient now");

    when E_DEWBERRY:
	if (chant_time > 0) {
	    chant_time -= 50;
	    if (chant_time < 0)
		chant_time = 0;
	    msg("You feel you have more chant ability");
	}
	if (pray_time > 0) {
	    pray_time -= 50;
	    if (pray_time < 0)
		pray_time = 0;
	    msg("You feel you have more prayer ability");
	}
	if (spell_power > 0) {
	    spell_power -= 50;
	    if (spell_power < 0)
		spell_power = 0;
	    msg("You feel you have more spell casting ability");
	}

    otherwise: /* all the foods don't have to do something */
	break;
    }
}
