/* File: xtra2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.1
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"



/*
 * Set a timed event (except timed resists, cutting and stunning).
 */
bool set_timed(int idx, int v, bool notify)
{
	const timed_effect *effect;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
	if ((idx < 0) || (idx > TMD_MAX)) return FALSE;

	/* No change */
	if (p_ptr->timed[idx] == v) return FALSE;

	/* Hack -- call other functions */
	switch (idx)
	{
		case TMD_STUN:
			return set_stun(v);

		case TMD_CUT:
			return set_cut(v);

		case TMD_POISONED:
			return set_poisoned(v);

		case TMD_SLOW_POISON:
			return set_slow_poison(v);

		case TMD_AFRAID:
			return set_afraid(v);

		case TMD_MSLEEP:
			return set_msleep(v);

		case TMD_PSLEEP:
			return set_psleep(v);

		case TMD_STASTIS:
			return set_stastis(v);
	}


	/* Don't mention some effects. */
	if (idx == TMD_OPP_ACID && p_ptr->cur_flags2 & (TR2_IM_ACID)) notify = FALSE;
	else if (idx == TMD_OPP_ELEC && p_ptr->cur_flags2 & (TR2_IM_ELEC)) notify = FALSE;
	else if (idx == TMD_OPP_FIRE && p_ptr->cur_flags2 & (TR2_IM_FIRE)) notify = FALSE;
	else if (idx == TMD_OPP_COLD && p_ptr->cur_flags2 & (TR2_IM_COLD)) notify = FALSE;
	else if (idx == TMD_OPP_POIS && p_ptr->cur_flags2 & (TR4_IM_POIS)) notify = FALSE;
	else if (idx == TMD_OPP_CONF && p_ptr->cur_flags2 & (TR2_RES_CONFU)) notify = FALSE;
	else if (idx == TMD_FREE_ACT && p_ptr->cur_flags2 & (TR3_FREE_ACT)) notify = FALSE;


	/* Find the effect */
	effect = &timed_effects[idx];

	/* Turning off, always mention */
	if (v == 0)
	{
		message(MSG_RECOVER, 0, effect->on_end);
		notify = TRUE;
	}

	/* Turning on, always mention */
	else if (p_ptr->timed[idx] == 0)
	{
		message(effect->msg, 0, effect->on_begin);
		notify = TRUE;
	}

	else if (notify)
	{
		/* Decrementing */
		if (p_ptr->timed[idx] > v && effect->on_decrease)
			message(effect->msg, 0, effect->on_decrease);

		/* Incrementing */
		else if (v > p_ptr->timed[idx] && effect->on_decrease)
			message(effect->msg, 0, effect->on_increase);
	}

	/* Use the value */
	p_ptr->timed[idx] = v;

	/* Sort out the sprint effect */
	if (idx == TMD_SPRINT && v == 0)
		inc_timed(TMD_SLOW, 100, TRUE);


	/* Nothing to notice */
	if (!notify) return FALSE;

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Update the visuals, as appropriate. */
	p_ptr->update |= effect->flag_update;
	p_ptr->redraw |= effect->flag_redraw;

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return TRUE;
}

/**
 * Increase the timed effect `idx` by `v`.  Mention this if `notify` is TRUE.
 */
bool inc_timed(int idx, int v, bool notify)
{
	/* Check we have a valid effect */
	if ((idx < 0) || (idx > TMD_MAX)) return FALSE;

	/* Set v */
	v = v + p_ptr->timed[idx];

	return set_timed(idx, v, notify);
}

/**
 * Decrease the timed effect `idx` by `v`.  Mention this if `notify` is TRUE.
 */
bool dec_timed(int idx, int v, bool notify)
{
	/* Check we have a valid effect */
	if ((idx < 0) || (idx > TMD_MAX)) return FALSE;

	/* Set v */
	v = p_ptr->timed[idx] - v;

	return set_timed(idx, v, notify);
}

/**
 * Clear the timed effect `idx`.  Mention this if `notify` is TRUE.
 */
bool clear_timed(int idx, bool notify)
{
	return set_timed(idx, 0, notify);
}

/**
 * Halves the timed effect `idx`.  Mention this if `notify` is TRUE.
 */
bool pfix_timed(int idx, bool notify)
{
	return set_timed(idx, p_ptr->timed[idx] / 2, notify);
}



/*
 * Set "p_ptr->timed[TMD_POISONED]", notice observable changes
 */
bool set_poisoned(int v)
{
	bool notice = FALSE;
	bool really_notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->timed[TMD_POISONED])
		{
			if (p_ptr->timed[TMD_SLOW_POISON])
			{
				/* Deliberately ambiguous message */
				msg_print("Whew.");
				msg_print("You thought you saw poison but you feel fine.");
			}
			else
			{
				msg_print("You are poisoned!");
				really_notice = TRUE;
			}
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->timed[TMD_POISONED])
		{
			msg_print("You are no longer poisoned.");
			notice = TRUE;
			really_notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->timed[TMD_POISONED] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw the "poisoned" */
	p_ptr->redraw |= (PR_POISONED);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (really_notice);
}


/*
 * Set "p_ptr->timed[TMD_SLOW_POISON]", notice observable changes
 */
bool set_slow_poison(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if ((p_ptr->timed[TMD_POISONED]) && !(p_ptr->timed[TMD_SLOW_POISON]))
		{
			msg_print("The poison in your veins is slowed.");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->timed[TMD_POISONED])
		{
			msg_print("You are poisoned!");
			msg_print("How did you not notice?");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->timed[TMD_SLOW_POISON] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw the "poisoned" */
	p_ptr->redraw |= (PR_POISONED);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->timed[TMD_AFRAID]", notice observable changes
 */
bool set_afraid(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->timed[TMD_AFRAID])
		{
			msg_print("You are terrified!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->timed[TMD_AFRAID])
		{
			msg_print("You feel bolder now.");
			notice = TRUE;
		}
	}

	/* Hack -- petrify the player if over 100 */
	if (v > 109)
	{
		p_ptr->timed[TMD_AFRAID] = 100;
		inc_timed(TMD_PETRIFY, (v - 100) / 10, TRUE);
	}

	/* Use the value */
	else p_ptr->timed[TMD_AFRAID] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw the "afraid" */
	p_ptr->redraw |= (PR_AFRAID);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->timed[TMD_MSLEEP]", notice observable changes
 */
bool set_msleep(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->timed[TMD_MSLEEP])
		{
			msg_print("Your eyelids feel heavy.");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if ((p_ptr->timed[TMD_MSLEEP]) && !(p_ptr->timed[TMD_PSLEEP]))
		{
			msg_print("You no longer feel sleepy.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->timed[TMD_MSLEEP] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->timed[TMD_PSLEEP]", notice observable changes
 */
bool set_psleep(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Hack -- Wake player when finished sleeping */
	if (v >= PY_SLEEP_MAX) v = 0;

	/* Recover stats */
	if (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_RECOVER)
	{
		int i;

		for (i = 0; i < A_MAX; i++)
		{
			if (p_ptr->stat_cur[i]<p_ptr->stat_max[i])
			{
				if (p_ptr->stat_cur[i] < 18) p_ptr->stat_cur[i]++;
				else p_ptr->stat_cur[i] += 10;

				if (p_ptr->stat_cur[i] > p_ptr->stat_max[i]) p_ptr->stat_cur[i] = p_ptr->stat_max[i];

				p_ptr->redraw |= (PR_STATS);

				v = 0;

				notice = TRUE;
			}
		}

		if (notice) msg_print("You recover somewhat.");
	}

	/* Open */
	if (v)
	{
		if ((p_ptr->timed[TMD_PSLEEP] < PY_SLEEP_ASLEEP) && (v >= PY_SLEEP_ASLEEP))
		{
			msg_print("You fall asleep.");
			notice = TRUE;
		}
		else if ((p_ptr->timed[TMD_PSLEEP] < PY_SLEEP_DROWSY) && (v >= PY_SLEEP_DROWSY))
		{
			msg_print("You feel drowsy.");
			/* notice = TRUE; */
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->timed[TMD_PSLEEP])
		{
			msg_print("You wake up.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->timed[TMD_PSLEEP] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Redraw state */
	p_ptr->redraw |= (PR_STATE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}




/*
 * Set "p_ptr->timed[TMD_STASTIS]", notice observable changes
 */
bool set_stastis(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* MegaHack -- alter reality if too high */
	if (v > 100)
	{
		msg_print("You are thrown into an alternate reality!");
		p_ptr->leaving = TRUE;
		p_ptr->create_stair = 0;
		notice = TRUE;
		v = 0;
	}

	/* Open */
	if (v)
	{
		if (!p_ptr->timed[TMD_STASTIS])
		{
			msg_print("You are stuck in a time-loop!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if ((p_ptr->timed[TMD_STASTIS]) && !(p_ptr->timed[TMD_PARALYZED]) && !(p_ptr->timed[TMD_PETRIFY]))
		{
			msg_print("You can move again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->timed[TMD_STASTIS] = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->timed[TMD_STUN]", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_stun(int v)
{
	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Knocked out */
	if (p_ptr->timed[TMD_STUN] > 100)
	{
		old_aux = 3;
	}

	/* Heavy stun */
	else if (p_ptr->timed[TMD_STUN] > 50)
	{
		old_aux = 2;
	}

	/* Stun */
	else if (p_ptr->timed[TMD_STUN] > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Knocked out */
	if (v > 100)
	{
		new_aux = 3;
	}

	/* Heavy stun */
	else if (v > 50)
	{
		new_aux = 2;
	}

	/* Stun */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Stun */
			case 1:
			{
				msg_print("You have been stunned.");
				break;
			}

			/* Heavy stun */
			case 2:
			{
				msg_print("You have been heavily stunned.");
				break;
			}

			/* Knocked out */
			case 3:
			{
				msg_print("You have been knocked out.");
				break;
			}
		}

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
			{
				msg_print("You are no longer stunned.");
				if (disturb_state) disturb(0, 0);
				break;
			}
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->timed[TMD_STUN] = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "stun" */
	p_ptr->redraw |= (PR_STUN);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->timed[TMD_CUT]", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_cut(int v)
{
	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Mortal wound */
	if (p_ptr->timed[TMD_CUT] > 1000)
	{
		old_aux = 7;
	}

	/* Deep gash */
	else if (p_ptr->timed[TMD_CUT] > 200)
	{
		old_aux = 6;
	}

	/* Severe cut */
	else if (p_ptr->timed[TMD_CUT] > 100)
	{
		old_aux = 5;
	}

	/* Nasty cut */
	else if (p_ptr->timed[TMD_CUT] > 50)
	{
		old_aux = 4;
	}

	/* Bad cut */
	else if (p_ptr->timed[TMD_CUT] > 25)
	{
		old_aux = 3;
	}

	/* Light cut */
	else if (p_ptr->timed[TMD_CUT] > 10)
	{
		old_aux = 2;
	}

	/* Graze */
	else if (p_ptr->timed[TMD_CUT] > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Mortal wound */
	if (v > 1000)
	{
		new_aux = 7;
	}

	/* Deep gash */
	else if (v > 200)
	{
		new_aux = 6;
	}

	/* Severe cut */
	else if (v > 100)
	{
		new_aux = 5;
	}

	/* Nasty cut */
	else if (v > 50)
	{
		new_aux = 4;
	}

	/* Bad cut */
	else if (v > 25)
	{
		new_aux = 3;
	}

	/* Light cut */
	else if (v > 10)
	{
		new_aux = 2;
	}

	/* Graze */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Graze */
			case 1:
			{
				msg_print("You have been given a graze.");
				break;
			}

			/* Light cut */
			case 2:
			{
				msg_print("You have been given a light cut.");
				break;
			}

			/* Bad cut */
			case 3:
			{
				msg_print("You have been given a bad cut.");
				break;
			}

			/* Nasty cut */
			case 4:
			{
				msg_print("You have been given a nasty cut.");
				break;
			}

			/* Severe cut */
			case 5:
			{
				msg_print("You have been given a severe cut.");
				break;
			}

			/* Deep gash */
			case 6:
			{
				msg_print("You have been given a deep gash.");
				break;
			}

			/* Mortal wound */
			case 7:
			{
				msg_print("You have been given a mortal wound.");
				break;
			}
		}

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
			{
				msg_print("You are no longer bleeding.");
				if (disturb_state) disturb(0, 0);
				break;
			}
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->timed[TMD_CUT] = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "cut" */
	p_ptr->redraw |= (PR_CUT);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->food", notice observable changes
 *
 * The "p_ptr->food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.
 *
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  XXX XXX XXX
 *
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.
 *
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).
 */
bool set_food(int v)
{
	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;

	/* Starving */
	if (p_ptr->food < PY_FOOD_STARVE)
	{
		old_aux = 0;
	}

	/* Fainting */
	else if (p_ptr->food < PY_FOOD_FAINT)
	{
		old_aux = 1;
	}

	/* Weak */
	else if (p_ptr->food < PY_FOOD_WEAK)
	{
		old_aux = 2;
	}

	/* Hungry */
	else if (p_ptr->food < PY_FOOD_ALERT)
	{
		old_aux = 3;
	}

	/* Normal */
	else if (p_ptr->food < PY_FOOD_FULL)
	{
		old_aux = 4;
	}

	/* Full */
	else if (p_ptr->food < PY_FOOD_MAX)
	{
		old_aux = 5;
	}

	/* Gorged */
	else
	{
		old_aux = 6;
	}

	/* Starving */
	if (v < PY_FOOD_STARVE)
	{
		new_aux = 0;
	}

	/* Fainting */
	else if (v < PY_FOOD_FAINT)
	{
		new_aux = 1;
	}

	/* Weak */
	else if (v < PY_FOOD_WEAK)
	{
		new_aux = 2;
	}

	/* Hungry */
	else if (v < PY_FOOD_ALERT)
	{
		new_aux = 3;
	}

	/* Normal */
	else if (v < PY_FOOD_FULL)
	{
		new_aux = 4;
	}

	/* Full */
	else if (v < PY_FOOD_MAX)
	{
		new_aux = 5;
	}

	/* Gorged */
	else
	{
		new_aux = 6;
	}

	/* Food increase */
	if ((new_aux > old_aux) || ((v > p_ptr->food) && (new_aux == old_aux)))
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Weak */
			case 0:
			case 1:
			case 2:
			{
				msg_print("You are still starving.");
				break;
			}

			/* Hungry */
			case 3:
			{
				msg_print("You are still hungry.");
				break;
			}

			/* Normal */
			case 4:
			{
				if (new_aux != old_aux) msg_print("You are no longer hungry.");
				break;
			}

			/* Full */
			case 5:
			{
				msg_print("You are full!");
				break;
			}

			/* Bloated */
			case 6:
			{
				msg_print("You have gorged yourself!");
				break;
			}
		}

		/* Change */
		notice = TRUE;
	}

	/* Food decrease */
	else if (!new_aux || new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
		        /* A step from hunger death */
			case 0:
			{
			  /* Disturb */
			  disturb(1, 1);

			  /* Hack -- bell on first notice */
			  if (new_aux < old_aux)
			    bell("You are dying from hunger!");

			  /* Message */
			  msg_print("You are dying from hunger!");
			  msg_print(NULL);
			  break;
			}

			/* Fainting / Starving */
			case 1:
			{
				msg_print("You are getting faint from hunger!");
				break;
			}

			/* Weak */
			case 2:
			{
				msg_print("You are getting weak from hunger!");
				break;
			}

			/* Hungry */
			case 3:
			{
				msg_print("You are getting hungry.");
				break;
			}

			/* Normal */
			case 4:
			{
				msg_print("You are no longer full.");
				break;
			}

			/* Full */
			case 5:
			{
				msg_print("You are no longer gorged.");
				break;
			}
		}

		/* Change */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->food = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb - wake player if hungry */
	if (disturb_state) disturb(0, new_aux <= 2 ? TRUE : FALSE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw hunger */
	p_ptr->redraw |= (PR_HUNGER);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->rest", notice observable changes
 *
 * Tiring is handled in "dungeon.c", but computation of the rate of
 * tiring is handled in "xtra1.c". The player tires at a rate dependent
 * on their constitution, but this is impacted if they are slowed by
 * heavy equipment, and by moving through shallow, deep or filled
 * locations.
 *
 * Note the player rests to "catch their breath", but may not do so in
 * locations that are filled with a terrain type.
 *
 * Note that the player automatically catches their breath when searching,
 * (With the same caveat), but searching takes a full turns energy, rather
 * than a partial turn.
 *
 */

bool set_rest(int v)
{
	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > PY_REST_MAX) ? PY_REST_MAX : (v < 0) ? 0 : v;

	/* Fainting / Starving */
	if (p_ptr->rest < PY_REST_FAINT)
	{
		old_aux = 0;
	}

	/* Weak */
	else if (p_ptr->rest < PY_REST_WEAK)
	{
		old_aux = 1;
	}

	/* Hungry */
	else if (p_ptr->rest < PY_REST_ALERT)
	{
		old_aux = 2;
	}

	/* Normal */
	else
	{
		old_aux = 3;
	}

	/* Fainting / Starving */
	if (v < PY_REST_FAINT)
	{
		new_aux = 0;
	}

	/* Weak */
	else if (v < PY_REST_WEAK)
	{
		new_aux = 1;
	}

	/* Hungry */
	else if (v < PY_REST_ALERT)
	{
		new_aux = 2;
	}

	/* Normal */
	else
	{
		new_aux = 3;
	}

	/* Rest increase */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{

			/* Normal */
			case 3:
			{
				msg_print("You are no longer tired.");

				/* Change */
				notice = TRUE;

				break;
			}

		}

	}

	/* Food decrease */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Fainting / Starving */
			case 0:
			{
				msg_print("You are getting faint from exhaustion!");
				break;
			}

			/* Weak */
			case 1:
			{
				msg_print("You are getting weak from exhaustion!");
				break;
			}

			/* Hungry */
			case 2:
			{
				msg_print("You are getting short of breath.");
				break;
			}

		}

		/* Change */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->rest = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw hunger */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Mark items as aware as a result of gaining a level.
 */
void improve_aware(void)
{
	int i, ii;
	int awareness = -1;

	/* Hack -- Check for id'ed */
	for (i=1;i<z_info->w_max;i++)
	{
		if (w_info[i].benefit != WB_ID) continue;

		if (w_info[i].class != p_ptr->pclass) continue;

		if (w_info[i].level > p_ptr->lev) continue;

		if ((w_info[i].styles & (1L << p_ptr->pstyle)) == 0) continue;

		awareness = 2*(p_ptr->lev - w_info[i].level)+1;
	}

	/* Hack -- Check for id'ed */
	for (i=1;i<z_info->k_max;i++)
	{
		/* Check for awareness */
		if (k_info[i].level > awareness) continue;

		/* Check awareness */
		if (k_info[i].tval == style2tval[p_ptr->pstyle])
		{
			/* Aware next time player sees an object */
			k_info[i].aware |= (AWARE_CLASS);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW);

			/* Check inventory */
			for (ii = 0; ii < INVEN_TOTAL; ii++)
			{
				if (inventory[ii].k_idx == i)
				{
					/* Become aware of object */
					object_aware(&inventory[ii], FALSE);
				}

				/* Notice stuff */
				p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			}
		}
	}
}


static int stat_gain_selection[PY_MAX_STAT_GAIN];
static int stat_gain_selected;


/*
 * Print a list of stats (for improvement).
 */
void print_stats(const s16b *sn, int num, int y, int x)
{
	int i, j;

	byte attr;

	/* Clear the screen */
	clear_from(1);

	/* Add labels */
	for (i = 0; i < num; i++)
	{
		attr = TERM_WHITE;

		if (p_ptr->stat_cur[sn[i]] < p_ptr->stat_max[sn[i]]) attr = TERM_YELLOW;
		if (p_ptr->stat_max[sn[i]] == 18 + 999) attr = TERM_L_DARK;

		for (j = 0; j < stat_gain_selected; j++)
		{
			if (stat_gain_selection[j] == i) attr = TERM_L_BLUE;
		}
		
		/* Display the label */
		c_prt(attr, format("  %c) ", I2A(i)), y + i + 1, x);

		/* Display the stats */
		display_player_stat_info(y + 1, x + 5, i, i + 1, attr);
	}

	/* Display extra info */
	display_player_xtra_info(1);
}


/*
 * Other stat functions.
 */
bool stat_commands(char choice, const s16b *sn, int i, bool *redraw)
{
	(void)sn;
	(void)i;

	switch (choice)
	{
		case '?':
		{
			/* Save the screen */
			if (!(*redraw)) screen_save();

			/* Show stats help */
			(void)show_file("stats.txt", NULL, 0, 0);

			/* Load the screen */
			screen_load();

			break;
		}

		default:
		{
			return (FALSE);
		}
	}
	return (TRUE);
}

/*
 * Improve a player-chosen set of stats.
 * Note the hack to always improve the maximal value of a stat.
 */
static void improve_stat(void)
{
	int tmp = 0;
	int i;

	s16b table[A_MAX+1];
	s16b old_max[A_MAX+1];
	s16b old_cur[A_MAX+1];

	char buf[32];

	cptr p = "";

	int count = 0;

#ifdef ALLOW_BORG
	if (count_stop) return;
#endif

	/* Flush messages */
	if (easy_more) messages_easy(FALSE);

	/* Check which stats can still be improved and store old stats*/
	for (i = 0; i < A_MAX; i++)
	{
		table[i] = i;
		if (p_ptr->stat_max[i] < 18 + 999) count++;
		old_max[i] = p_ptr->stat_max[i];
		old_cur[i] = p_ptr->stat_cur[i];
	}

	/* No stats left to improve */
	if (!count) return;

	/* Reduce count to number of abilities allowed improvements */
	if (count > stat_gains[p_ptr->lev -1]) count = stat_gains[p_ptr->lev -1];

	if (adult_rand_stats)
	{
		/* Improve how many stats with level gain */
		for (stat_gain_selected = 0; stat_gain_selected < count; stat_gain_selected++)
		{
			/* Pick a random stat */
			stat_gain_selection[stat_gain_selected] = rand_int(A_MAX);
			
			/* Valid choice? */
			if (p_ptr->stat_max[stat_gain_selection[stat_gain_selected]] < 18 + 999)
			{
				bool okay = TRUE;
				
				/* Check we are not improving another stat */
				for (i = 0; i < stat_gain_selected; i++)
				{
					if (stat_gain_selection[i] == stat_gain_selection[stat_gain_selected]) okay = FALSE;
				}
				
				/* Retry */
				if (!okay) stat_gain_selected--;
			}
			else
			{
				/* Retry */
				stat_gain_selected--;
			}
		}
		
        /* Improve how many stats with level gain */
        for (stat_gain_selected = 0; stat_gain_selected < count; stat_gain_selected++)
        {
			/* Display */
			if (p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]] < p_ptr->stat_max[stat_gain_selection[stat_gain_selected]])
			{
				/* Set description */
				p = "you could be ";
				
				/* Hack --- store stat */
				tmp = p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]];
				p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]] = p_ptr->stat_max[stat_gain_selection[stat_gain_selected]];
			}
			else
			{
				p = "";
				tmp = 0;
			}
			
			/* Increase */
			inc_stat(stat_gain_selection[stat_gain_selected]);
#if 0
			/* Message */
			msg_format("You feel %s%s.",p,desc_stat_imp[stat_gain_selection[stat_gain_selected]]);
#endif
			/* Hack --- restore stat */
			if (tmp) p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]] = tmp;
        }
		
		/* end adult_rand_stats */
		return;
	}
	
	/* Save screen */
	screen_save();
	
	/* Update stats */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);
	update_stuff();

	/* Confirm stat selection */
	while (count)
	{
		/* Improve how many stats with level gain */
		for (stat_gain_selected = 0; stat_gain_selected < count; stat_gain_selected++)
		{
			/* Should be paranoid here */
			while (TRUE)
			{
				sprintf(buf,"Improve which attribute%s (%d)", count > 1 ? "s" : "", count - stat_gain_selected);
				
				/* Select stat to improve */
				if (get_list(print_stats, table, A_MAX, "Attribute", buf, ", ?=help", 1, 36, stat_commands, &(stat_gain_selection[stat_gain_selected])))
				{
					/* Check if stat at maximum */
					if (p_ptr->stat_max[stat_gain_selection[stat_gain_selected]] >= 18 + 999)
					{
						msg_format("You cannot get any better.");
					}

					/* Good stat_gain_selection? */
					else
					{
						bool okay = TRUE;

						/* Check we are not improving another stat */
						for (i = 0; i < stat_gain_selected; i++)
						{
							if (stat_gain_selection[i] == stat_gain_selection[stat_gain_selected]) okay = FALSE;
						}

						if (okay)
						{
							/* Check for drained stats */
							if (p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]]
								   < p_ptr->stat_max[stat_gain_selection[stat_gain_selected]])
								tmp = p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]];
							else
								tmp = 0;
							
							/* Temporarily restore the stat before increasing */
							p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]] = p_ptr->stat_max[stat_gain_selection[stat_gain_selected]];
							
							/* Increase */
							inc_stat(stat_gain_selection[stat_gain_selected]);
							
							/* Restore drained stats */
							if (tmp) p_ptr->stat_cur[stat_gain_selection[stat_gain_selected]] = tmp;

							/* Update stats */
							p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);
							update_stuff();
							
							/* Redisplay stats */
							print_stats(table, A_MAX, 1, 36);
														
							break;
						}
					}
				}
				else
				{
					/* Clear the attributes */
					stat_gain_selected = 0;
					
					/* Restore old stats */
					for (i = 0; i < A_MAX; i++)
					{
						p_ptr->stat_max[i] = old_max[i];
						p_ptr->stat_cur[i] = old_cur[i];
					}

					/* Update stats */
					p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);
					update_stuff();
				}
			}
		}

		/* Redisplay stats */			
		print_stats(table, A_MAX, 1, 36);

		/* Confirm? */
		if (get_check("Increasing highlighted stats. Are you sure? "))
		{
			break;
		}

		/* Otherwise restore old stats */
		for (i = 0; i < A_MAX; i++)
		{
			p_ptr->stat_max[i] = old_max[i];
			p_ptr->stat_cur[i] = old_cur[i];
		}

		/* Update stats */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);
		update_stuff();
	}
	
	/* Load screen */
	screen_load();	
}


/*
 * Print a list of abilities that can be gained by a familiar.
 */
void print_familiars(const s16b *sn, int num, int y, int x)
{
	int i;

	char out_val[60];

	byte line_attr;

	/* Title the list */
	prt("", y, x);
	put_str("Ability", y, x + 5);

	/* Dump the familiar abilities */
	for (i = 0; i < num; i++)
	{
		line_attr = TERM_WHITE;

		/* Dump the route --(-- */
		sprintf(out_val, "  %c) %s",
			I2A(i), familiar_ability[sn[i]].text);
		c_prt(line_attr, out_val, y + i + 1, x);
	}

	/* Clear the bottom line */
	prt("", y + i + 1, x);

}


/*
 * Other stat functions.
 */
bool familiar_commands(char choice, const s16b *sn, int i, bool *redraw)
{
	(void)sn;
	(void)i;

	switch (choice)
	{
		case '?':
		{
			/* Save the screen */
			if (!(*redraw)) screen_save();

			/* Show stats help */
			(void)show_file("familiar.txt", NULL, 0, 0);

			/* Load the screen */
			screen_load();

			break;
		}

		default:
		{
			return (FALSE);
		}
	}
	return (TRUE);
}


/*
 * Improve the player's familiar by adding a player chosen ability.
 */
void improve_familiar(void)
{
	int number_allowed = p_ptr->lev < 40 ? (p_ptr->lev / 2) : 20 + (p_ptr->lev - 40);
	int i, j, num;
	int base, slot, choice;
	int blows_left = 4;
	int last_blow_effect = FAMILIAR_BLOW + GF_HURT;

	s16b table[FAMILIAR_CHOICES];

#ifdef ALLOW_BORG
	if (count_stop) return;
#endif

	/* Hack -- at level 10, the familiar gets a blow for free, and their third attribute */
	if (p_ptr->lev >= FAMILIAR_FREE_BLOW) number_allowed += 2;
	
	/* Hack -- at levels 15 and 25, the familiar gets additional abilities */
	if (p_ptr->lev >= 15) number_allowed++;
	if (p_ptr->lev >= 25) number_allowed++;

	/* Flush messages */
	if (easy_more) messages_easy(FALSE);

	/* Does not yet have a familiar */
	if (!p_ptr->familiar) return;

	/* Familiar is dead */
	if (r_info[FAMILIAR_IDX].max_num <= 0) return;

	/* Keep filling slots if required */
	while (TRUE)
	{
		/* Find the next available slot */
		for (slot = 0; p_ptr->familiar_attr[slot]; slot++);

		/* No more slots */
		if (slot >= number_allowed) break;

		/* Hack - fill free blow slot with blow */
		if (slot == FAMILIAR_FREE_BLOW / FAMILIAR_PICKS)
		{
			p_ptr->familiar_attr[FAMILIAR_FREE_BLOW / FAMILIAR_PICKS] = FAMILIAR_BLOW;
			blows_left--;
			last_blow_effect = FAMILIAR_BLOW + GF_HURT;
			continue;
		}
		
		/* Hack - fill free attribute slot with third attribute */
		if (slot == FAMILIAR_FREE_BLOW / FAMILIAR_PICKS + 1)
		{
			p_ptr->familiar_attr[FAMILIAR_FREE_BLOW / FAMILIAR_PICKS + 1] = familiar_race[p_ptr->familiar].attr3;
			continue;
		}
		
		/* Record last blow effect */
		if (familiar_ability[slot].attr > FAMILIAR_BLOW) last_blow_effect = familiar_ability[slot].attr;

		/* Count blows and clear last blow effect if we get an extra blow */
		else if (familiar_ability[slot].attr == FAMILIAR_BLOW)
		{
			blows_left--;
			last_blow_effect = FAMILIAR_BLOW + GF_HURT;
		}

		/* Find the location in the table */
		base = (slot / FAMILIAR_PICKS) - 1;
		
		/* Hack -- the following saves us having to have a hole in the familiar ability table */
		if (slot >= FAMILIAR_FREE_BLOW / FAMILIAR_PICKS) base--;
		
		/* Continue finding the location in the table */
		base *= FAMILIAR_CHOICES;
		base++;

		/* Paranoia */
		if (base < 1) base = 1;
		
		/* No abilities yet */
		num = 0;

		/* Initialise table of choices */
		for (i = base; (i < base + FAMILIAR_CHOICES) && (i < MAX_FAMILIAR_ABILITIES); i++)
		{
			bool okay = TRUE;
			bool preq = FALSE;

			/* Check to see if the player familiar already has this ability */
			for (j = 0; j < slot; j++)
			{
				/* Not allowed to choose most normal abilities, or spikes more than once */
				if (((familiar_ability[i].attr < FAMILIAR_AC) || (familiar_ability[i].attr == FAMILIAR_SPIKE)) &&
						(p_ptr->familiar_attr[j] == familiar_ability[i].attr)) okay = FALSE;

				/* Notice if we have pre-requisite */
				if (p_ptr->familiar_attr[j] == familiar_ability[i].preq) preq = TRUE;

				/* Hack to allow other helpful spells to be cast on the player */
				if (familiar_ability[i].preq == 181)
				{
					switch (p_ptr->familiar_attr[j])
					{
						case 184: /* Oppose elements */
						case 183: /* Shield */
						case 163: /* Cure */
						case 162: /* Heal */
						case 160: /* Haste */
							preq = TRUE;
					}
				}
			}

			/* Can only choose blows 4 times */
			if ((!blows_left) && (familiar_ability[i].attr == FAMILIAR_BLOW)) okay = FALSE;

			/* Can't pick the same blow improvement twice in a row */
			if ((slot) && (familiar_ability[i].attr == last_blow_effect)) okay = FALSE;
			
			/* Ability allowed */
			if (okay && (preq || !familiar_ability[i].preq))
			{
				table[num++] = i;
			}
		}

		/* Get a choice */
		if (get_list(print_familiars, table, num, "Ability", "Familiar gains which ability", ", ?=help", 1, 26, familiar_commands, &choice))
		{
			p_ptr->familiar_attr[slot] = familiar_ability[choice].attr;
		}
	}

	/* Revise familiar stats */
	generate_familiar();

	/* Revise familiar hit points */
	for (i = 0; i < z_info->m_max; i++)
	{
		if (m_list[i].r_idx == FAMILIAR_IDX)
		{
			calc_monster_hp(i);
		}
	}
}


/*
 * Advance experience levels and print experience
 */
void check_experience(void)
{
	/* Hack -- lower limit */
	if (p_ptr->exp < 0) p_ptr->exp = 0;

	/* Hack -- lower limit */
	if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;

	/* Hack -- upper limit */
	if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;

	/* Hack -- upper limit */
	if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;

	/* Hack -- maintain "max" experience */
	if (p_ptr->exp > p_ptr->max_exp)
	{
		int adjust = -1;
		s32b new_exp = p_ptr->exp;
		s32b new_exp_frac = p_ptr->exp_frac;

		/* Gain levels while possible */
		while ((p_ptr->max_lev < PY_MAX_LEVEL) &&
		       (p_ptr->exp >= (player_exp[p_ptr->max_lev+adjust] *
				       p_ptr->expfact / 100L)))
		{

			/* Reduce outstanding experience after level gain */
			p_ptr->exp = (player_exp[p_ptr->max_lev+adjust] * p_ptr->expfact / 100L) +
				(p_ptr->exp - (player_exp[p_ptr->max_lev+adjust] * p_ptr->expfact / 100L)) * (p_ptr->max_lev + adjust + 1) / (p_ptr->max_lev + adjust + 2);

			/* Modify adjustment */
			adjust++;
		}

		/* Add fractional experience */
		/* Handle fractional experience */
		if (adjust > -1) new_exp_frac = (((new_exp * (p_ptr->max_lev + adjust + 1)) % (p_ptr->max_lev + adjust + 2))
				* 0x10000L / (p_ptr->max_lev + adjust + 2)) + p_ptr->exp_frac;

		/* Keep track of experience */
		if (new_exp_frac >= 0x10000L)
		{
			p_ptr->exp++;
			p_ptr->exp_frac = (u16b)(new_exp_frac - 0x10000L);
		}
		else
		{
			p_ptr->exp_frac = (u16b)new_exp_frac;
		}

		/* Set new maximum experience */
		p_ptr->max_exp = p_ptr->exp;

	}

	/* Redraw experience */
	p_ptr->redraw |= (PR_EXP);

	/* Handle stuff */
	handle_stuff();

	/* Lose levels while possible */
	while ((p_ptr->lev > 1) &&
	       (p_ptr->exp < (player_exp[p_ptr->lev-2] *
			      p_ptr->expfact / 100L)))
	{
		/* Lose a level */
		p_ptr->lev--;

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

		/* Handle stuff */
		handle_stuff();
	}

	/* Gain levels while possible */
	while ((p_ptr->lev < PY_MAX_LEVEL) &&
	       (p_ptr->exp >= (player_exp[p_ptr->lev-1] *
			       p_ptr->expfact / 100L)))
	{
		int i;

		/* Gain a level */
		p_ptr->lev++;

		/* Improve a stat */
		if (p_ptr->lev > p_ptr->max_lev) improve_stat();

		/* Improve awareness */
		if (p_ptr->lev > p_ptr->max_lev) improve_aware();

		/* Improve familiar */
		if (p_ptr->lev > p_ptr->max_lev) improve_familiar();

		/* Message */
		message_format(MSG_LEVEL, p_ptr->lev, "Welcome to level %d.", p_ptr->lev);

		/* Show all tips for intermediate levels */
		for (i = p_ptr->max_lev; i <= p_ptr->lev; i++)
		{
			/* Level tips */
			queue_tip(format("level%d.txt", i));

			/* Race tips */
			queue_tip(format("race%d-%d.txt", p_ptr->prace, i));

			/* Class tips */
			queue_tip(format("class%d-%d.txt", p_ptr->pclass, i));

			/* Style tips */
			queue_tip(format("spec%d-%d-%d.txt", p_ptr->pclass, p_ptr->pstyle, i));
			
			/* School tips */
			queue_tip(format("scho%d-%d-%d.txt", p_ptr->pclass, p_ptr->pschool, i));
			
		}

		/* Save the highest level */
		if (p_ptr->lev > p_ptr->max_lev)
		{
			/* Increase max level */
			p_ptr->max_lev = p_ptr->lev;
		
			/* Queue level tips for beginners */
			if (adult_beginner)
			{
				/* Assume the player is no longer a beginner after reaching level 10 */
				if (p_ptr->max_lev >= 10)
				{
					/* No longer a beginner */
					birth_beginner = FALSE;
					
					/* Save changes */
					dump_startup_prefs();
				}
				
				/* Queue tip */
				queue_tip(format("begin%d.txt", p_ptr->max_lev));
			}
		}

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_EXP | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

		/* Handle stuff */
		handle_stuff();
	}
}


/*
 * Gain experience
 */
void gain_exp(s32b amount)
{
	/* Gain some experience */
	p_ptr->exp += amount;

	/* Slowly recover from experience drainage */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Gain max experience (10%) */
		p_ptr->max_exp += amount / 10;
	}

	/* Check Experience */
	check_experience();
}


/*
 * Lose experience
 */
void lose_exp(s32b amount)
{
	/* Never drop below zero experience */
	if (amount > p_ptr->exp) amount = p_ptr->exp;

	/* Lose some experience */
	p_ptr->exp -= amount;

	/* Check Experience */
	check_experience();
}


/*
 * Hack -- Return the "automatic food type" of a monster race
 * Used to allocate proper treasure when "Mushrooms" die
 *
 * Note the use of actual "monster names".  XXX XXX XXX
 */
int get_food_type(const monster_race *r_ptr)
{
	cptr name = (r_name + r_ptr->name);

	/* Analyze "food" monsters */
	if ((r_ptr->d_char == ',') && (strstr(name,"mushroom")))
	{
		/* Look for textual clues */
		if (strstr(name, "Spotted ")) return (SV_FOOD_POISON+1);
		if (strstr(name, "Silver ")) return (SV_FOOD_BLINDNESS+1);
		if (strstr(name, "Yellow ")) return (SV_FOOD_PARANOIA+1);
		if (strstr(name, "Grey ")) return (SV_FOOD_CONFUSION+1);
		if (strstr(name, "Copper ")) return (SV_FOOD_PARALYSIS+1);
		if (strstr(name, "Pink ")) return (SV_FOOD_WEAKNESS+1);
		if (strstr(name, "Purple ")) return (SV_FOOD_SICKNESS+1);
		if (strstr(name, "Black ")) return (SV_FOOD_STUPIDITY+1);
		if (strstr(name, "Green ")) return (SV_FOOD_NAIVETY+1);
		if (strstr(name, "Rotting ")) return (SV_FOOD_UNHEALTH+1);
		if (strstr(name, "Brown ")) return (SV_FOOD_DISEASE+1);
		if (strstr(name, "Shrieker ")) return ((rand_int(100)<30?SV_FOOD_HASTE+1:SV_FOOD_CURE_PARANOIA+1));
		if (strstr(name, "Noxious ")) return (SV_FOOD_DISEASE+1);
		if (strstr(name, "Magic ")) return ((rand_int(100)<30?SV_FOOD_MANA+1:SV_FOOD_HALLUCINATION+1));

		/* Force nothing */
		return (-1);
	}
	/* Analyze "food" monsters */
	else if (strstr(name,", Farmer Maggot"))
	{
		return (SV_FOOD_MANA+1);
	}
	/* Analyze "slime mold" monsters */
	else if (strstr(name,"lime mold"))
	{
		return (SV_FOOD_SLIME_MOLD+1);
	}


	/* Assume nothing */
	return (0);
}


/*
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * Note the use of actual "monster names".  XXX XXX XXX
 */
int get_coin_type(const monster_race *r_ptr)
{
	int i;
	cptr name = (r_name + r_ptr->name);

	/* Analyze "coin" monsters */
	if (strchr("$gdDAaI", r_ptr->d_char))
	{
		for (i = 0; i < MAX_GOLD; i++)
		{
			object_kind *k_ptr = &k_info[i + OBJ_GOLD_LIST];

			/* Look for textual clues */
			if (strstr(name, format(" %s ",k_name + k_ptr->name))) return (i);
			if (strstr(name, format("%^s ", k_name + k_ptr->name))) return (i);
			if (strstr(name, format(" %ss",k_name + k_ptr->name))) return (i);
		}

		/* Hack -- rubies */
		if (strstr(name, " rubies")) return(13);
	}

	/* Assume nothing */
	return (0);
}


void scatter_objects_under_feat(int y, int x)
{
  s16b this_o_idx, next_o_idx = 0;

  object_type *o_ptr;

  assert (in_bounds(y, x));

  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Get the object */
      o_ptr = &o_list[this_o_idx];

      /* Get the next object */
      next_o_idx = o_ptr->next_o_idx;

      /* Drop the object */
      drop_near(o_ptr, -1, y, x, TRUE);
    }

  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Get the object */
      o_ptr = &o_list[this_o_idx];

      /* Get the next object */
      next_o_idx = o_ptr->next_o_idx;

		/* Delete object */
		delete_object_idx(this_o_idx);
    }

  /* Visual update */
  lite_spot(y, x);
}


/*
 * Handle a quest event.
 */
bool check_quest(quest_event *qe1_ptr, bool advance)
{
	int i, j;
	bool questor = FALSE;

	return (FALSE);

	for (i = 0; i < MAX_Q_IDX; i++)
	{
		quest_type *q_ptr = &(q_list[i]);

		int next_stage = 0;
		bool partial = FALSE;

		/* Check the next possible stages */
		for (j = 0; j < MAX_QUEST_EVENTS ; j++)
		{
			quest_event *qe2_ptr = &(q_ptr->event[j]);
			quest_event *qe3_ptr = &(q_ptr->event[QUEST_ACTION]);

			/* Not all quests advance */
			switch (q_ptr->stage)
			{
				/* We can succeed or fail quests at any time once the player has activated them. */
				case QUEST_ACTIVE:
					if (j == QUEST_ACTIVE) break;
					if (j == QUEST_FAILED) break;
					continue;
				/* Track what the player has done / changed in the world */
				case QUEST_ACTION: /* A quest should never get to this stage... */
				case QUEST_FINISH:
				case QUEST_PENALTY:
				/* We don't do pay outs until the start of the next player turn */
				case QUEST_PAYOUT:
				case QUEST_FORFEIT:
					continue;
				/* We just advance otherwise */
				default:
					if (q_ptr->stage != j) continue;
			}

			/* We support quests with blank transitions */
			if (qe2_ptr->flags)
			{
				/* Check for quest match */
				if ((qe2_ptr->flags & (qe1_ptr->flags)) == 0) continue;

				/* Check for level match */
				if ((qe2_ptr->dungeon) && ((qe2_ptr->dungeon != qe1_ptr->dungeon) ||
						(qe2_ptr->level != qe2_ptr->level))) continue;

				/* Check for race match */
				if (qe1_ptr->flags & (EVENT_GIVE_RACE | EVENT_GET_RACE | EVENT_FIND_RACE | EVENT_KILL_RACE |
						EVENT_ALLY_RACE | EVENT_HATE_RACE | EVENT_FEAR_RACE | EVENT_HEAL_RACE |
						EVENT_BANISH_RACE | EVENT_DEFEND_RACE | EVENT_DEFEND_STORE | EVENT_DEFEND_FEAT))
				{
					/* Match any monster or specific race */
					if ((qe2_ptr->race) && (qe2_ptr->race != qe1_ptr->race)) continue;

					/* Have to check monster states? */
					if ((qe1_ptr->flags & (EVENT_GIVE_RACE | EVENT_GET_RACE | EVENT_DEFEND_FEAT)) == 0)
					{
						/* Add flags */
						qe3_ptr->flags |= qe1_ptr->flags;

						/* Hack -- we accumulate banishes and kills in the QUEST_ACTION. For others,
						 * we only check if the number provided >= the number required.
						 */
						if (qe1_ptr->flags & (EVENT_BANISH_RACE | EVENT_KILL_RACE | EVENT_DEFEND_RACE |
								EVENT_DEFEND_STORE))
							qe3_ptr->number = qe1_ptr->number;
						else if (qe1_ptr->number + qe3_ptr->number >= qe2_ptr->number)
							qe3_ptr->number = qe2_ptr->number;
					}
				}

				/* Check for store match */
				if ((qe1_ptr->flags & (EVENT_BUY_STORE | EVENT_SELL_STORE | EVENT_GIVE_STORE |
						EVENT_STOCK_STORE | EVENT_GET_STORE | EVENT_DEFEND_STORE)) &&
						(qe2_ptr->store != qe1_ptr->store)) continue;

				/* Check for item match */
				if (qe1_ptr->flags & (EVENT_GIVE_RACE | EVENT_GET_RACE | EVENT_BUY_STORE |
						EVENT_SELL_STORE | EVENT_GIVE_STORE | EVENT_STOCK_STORE | EVENT_GET_STORE |
						EVENT_GET_ITEM | EVENT_FIND_ITEM | EVENT_DESTROY_ITEM | EVENT_LOSE_ITEM))
				{
					/* Match artifact, ego_item_type or kind of item or any item */
					if (((qe2_ptr->artifact) && (qe2_ptr->artifact != qe1_ptr->artifact)) ||
					 ((qe2_ptr->ego_item_type) && (qe2_ptr->ego_item_type != qe1_ptr->ego_item_type)) ||
					 ((qe2_ptr->kind) && (qe2_ptr->kind != qe1_ptr->kind))) continue;

					/* XXX Paranoia around artifacts */
					if ((qe2_ptr->artifact) && (qe2_ptr->number)) qe2_ptr->number = 1;

					/* Hack -- we accumulate item destructions in the QUEST_ACTION. For others,
					   we only check if the number provided >= the number required.
					 */
					if (qe1_ptr->flags & (EVENT_DESTROY_ITEM))
						qe3_ptr->number = qe1_ptr->number;
					else if	(qe1_ptr->number >= qe2_ptr->number)
						qe3_ptr->number = qe2_ptr->number;
				}

				/* Check for feature match */
				if (qe1_ptr->flags & (EVENT_ALTER_FEAT | EVENT_DEFEND_FEAT))
				{
					/* Match feature or any feature */
					if ((qe2_ptr->feat) && (qe2_ptr->feat != qe1_ptr->feat)) continue;

					/* Accumulate features */
					qe3_ptr->number = qe1_ptr->number;
				}

				/* Check for room type match */
				if (((qe1_ptr->flags & (EVENT_FIND_ROOM | EVENT_FLAG_ROOM | EVENT_UNFLAG_ROOM))) &&
					(((qe2_ptr->room_type_a) && (qe2_ptr->room_type_a != qe1_ptr->room_type_a)) ||
					((qe2_ptr->room_type_b) && (qe2_ptr->room_type_b != qe1_ptr->room_type_b))))
						continue;

				/* Check for room flag match */
				if (((qe1_ptr->flags & (EVENT_FLAG_ROOM | EVENT_UNFLAG_ROOM))) &&
						((qe2_ptr->room_flags & (qe1_ptr->room_flags)) == 0))
					continue;

				/* Do we have to stay on this level a set amount of time? */
				if (qe2_ptr->flags & (EVENT_STAY | EVENT_DEFEND_RACE | EVENT_DEFEND_FEAT | EVENT_DEFEND_STORE))
				{
					if (old_turn + qe2_ptr->time < turn) continue;
				}

				/* Do we have to succeed at a quest? */
				if (qe2_ptr->flags & (EVENT_PASS_QUEST))
				{
					if (q_info[qe2_ptr->quest].stage != QUEST_FINISH) continue;
				}

				/* Check for defensive failure */
				if (qe1_ptr->flags & (EVENT_DEFEND_RACE | EVENT_DEFEND_FEAT))
				{
					/* Hack - fail the quest */
					if (!qe1_ptr->number)
					{
						next_stage = QUEST_FAILED;
						break;
					}
				}

				/* Check for completion */
				else if ((qe3_ptr->number) && (qe3_ptr->number < qe2_ptr->number))
				{
					/* We at least have a partial match */
					partial = TRUE;
				}
			}

			/* We have qualified for the next stage of the quest */
			next_stage = (j == QUEST_ACTIVE ? QUEST_REWARD : j + 1);
		}

		/* Advance the quest */
		if (next_stage)
		{
			const char *prefix = ( next_stage == QUEST_REWARD ? "To claim your reward, " : NULL);

			/* Not advancing */
			if (!advance)
			{
				/* Voluntarily fail quest? */
				if (next_stage == QUEST_FAILED)
				{
					return (get_check(format("Fail %s?", q_name + q_ptr->name)));
				}
				else
				{
					return (FALSE);
				}
			}

			/* Describe the quest if assigned it */
			if ((next_stage == QUEST_ACTIVE) &&
					strlen(q_text + q_ptr->text)) msg_format("%s", q_text + q_ptr->text);

			/* Tell the player the next step of the quest. */
			if ((next_stage < QUEST_PAYOUT) &&
				(q_info[i].event[next_stage].flags))
			{
				/* Display the event 'You must...' */
				print_event(&q_info[i].event[next_stage], 2, 3, prefix);
			}
			/* Tell the player they have succeeded */
			else if (next_stage == QUEST_FINISH)
			{
				msg_format("You have completed %s!", q_name + q_ptr->name);
			}
			/* Tell the player they have failed */
			else if (next_stage == QUEST_PENALTY)
			{
				msg_format("You have failed %s.", q_name + q_ptr->name);
			}

			/* Advance quest to the next stage */
			q_ptr->stage = next_stage;

			/* Something done? */
			questor = TRUE;
		}
		/* Tell the player they have partially advanced the quest */
		else if (partial)
		{
			quest_event event;

			COPY(&event, &q_ptr->event[q_ptr->stage], quest_event);

			/* Tell the player the action they have completed */
			print_event(qe1_ptr, 2, 1, NULL);

			/* Tell the player the action(s) they must still do */
			print_event(&event, 2, 3, NULL);
		}
	}

	return(questor);
}


/*
 * Generate items which a monster carries into the monster's inventory.
 */
bool monster_drop(int m_idx)
{
	int j;

	int dump_item = 0;
	int dump_gold = 0;
	int dump_chest = 0;

	int number = 0;

	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	bool visible = (m_ptr->ml || (r_ptr->flags1 & (RF1_UNIQUE)));
	bool good = (r_ptr->flags1 & (RF1_DROP_GOOD)) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & (RF1_DROP_GREAT)) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & (RF1_ONLY_ITEM)));
	bool do_item = (!(r_ptr->flags1 & (RF1_ONLY_GOLD)));
	bool do_chest = (r_ptr->flags8 & (RF8_DROP_CHEST) ? TRUE : FALSE);

	int force_food = ((r_ptr->flags9 & (RF9_DROP_MUSHROOM)) != 0 ? get_food_type(r_ptr) : 0);
	int force_coin = ((r_ptr->flags9 & (RF9_DROP_MINERAL)) != 0 ? get_coin_type(r_ptr) : 0);

	object_type *i_ptr;
	object_type object_type_body;

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & (RF1_DROP_30)) && (rand_int(100) < 30)) number++;
	if ((r_ptr->flags1 & (RF1_DROP_60)) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & (RF1_DROP_90)) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags1 & (RF1_DROP_1D2)) number += damroll(1, 2);
	if (r_ptr->flags1 & (RF1_DROP_1D3)) number += damroll(1, 3);
	if (r_ptr->flags1 & (RF1_DROP_1D4)) number += damroll(1, 4);

	/* Hack -- handle mushroom patches */
	food_type = force_food;

	/* Hack -- handle creeping coins */
	coin_type = force_coin;

	/* Hack -- handle monster drops */
	race_drop_idx = m_ptr->r_idx;

	/* Average dungeon and monster levels */
	object_level = (p_ptr->depth + r_ptr->level) / 2;

	/* Clear monster equipment */
	hack_monster_equip = 0L;

	/* Drop some objects */
	for (j = 0; j < number; j++)
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Make Chest */
		if (do_chest && do_item && (rand_int(100) < 5))
		{
			int chest;

			/* Drop it in the dungeon */
			if (make_chest(&chest))
			{
				feat_near(chest,m_ptr->fy,m_ptr->fx);

				l_ptr->flags8 |= (RF8_DROP_CHEST);

				hack_monster_equip |= (RF8_DROP_CHEST);

				dump_chest++;
			}

			continue;
		}

		/* Hack - Have applied all items in monster equipment. We force gold, then clear. */
		if ((r_ptr->flags8 & (RF8_DROP_MASK)) &&
				((r_ptr->flags8 & (RF8_DROP_MASK)) == (hack_monster_equip & (RF8_DROP_MASK))))
		{
			if (do_gold && do_item)
			{
				do_item = FALSE;
			}
			else
			{
				hack_monster_equip = 0L;
			}
		}

		/* Make Gold */
		if (do_gold && (!do_item || (rand_int(100) < 50) ))
		{
			/* Make some gold */
			if (!make_gold(i_ptr, good, great)) continue;

			if (coin_type) l_ptr->flags9 |= (RF9_DROP_MINERAL);

			/* Assume seen XXX XXX XXX */
			dump_gold++;
		}

		/* Make Object */
		else
		{
			/* Make an object */
			if (!make_object(i_ptr, good, great)) continue;

			/* Hack -- chest/bag mimics drop matching tvals */
			if ((r_ptr->flags1 & (RF1_CHAR_MULTI)) && (r_ptr->d_char == '&')) tval_drop_idx = i_ptr->tval;

			/* Hack -- mimics */
			if (r_ptr->flags1 & (RF1_CHAR_MULTI)) l_ptr->flags1 |= (RF1_CHAR_MULTI);

			/* Learn about drops */
			else if (food_type) l_ptr->flags9 |= (RF9_DROP_MUSHROOM);

			/* Hack -- ignore bodies */
			else switch (i_ptr->tval)
			{
				case TV_JUNK:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_JUNK);
					l_ptr->flags8 |= (RF8_DROP_JUNK);
					break;
				}

				case TV_BOW:
				{
					hack_monster_equip |= (RF8_DROP_MISSILE);
				}
				case TV_SHOT:
				case TV_ARROW:
				case TV_BOLT:
				{
					l_ptr->flags8 |= (RF8_DROP_MISSILE);
					break;
				}

				case TV_DIGGING:
				case TV_SPIKE:
				case TV_FLASK:
				{
					hack_monster_equip |= (RF8_DROP_TOOL);
					l_ptr->flags8 |= (RF8_DROP_TOOL);
					break;
				}

				case TV_HAFTED:
				case TV_POLEARM:
				case TV_SWORD:
				{
					hack_monster_equip |= (RF8_DROP_WEAPON);
					l_ptr->flags8 |= (RF8_DROP_WEAPON);
					break;
				}

				case TV_SONG_BOOK:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_MUSIC);
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_WRITING);
				}
				case TV_INSTRUMENT:
				{
					hack_monster_equip |= (RF8_DROP_MUSIC);
					l_ptr->flags8 |= (RF8_DROP_MUSIC);
					break;
				}

				case TV_BOOTS:
				{
					hack_monster_equip |= (RF8_HAS_LEG);
					l_ptr->flags8 |= (RF8_DROP_CLOTHES);
					break;
				}
				case TV_GLOVES:
				{
					hack_monster_equip |= (RF8_HAS_HAND);
					l_ptr->flags8 |= (RF8_DROP_CLOTHES);
					break;
				}
				case TV_CLOAK:
				{
					hack_monster_equip |= (RF8_HAS_CORPSE);
					l_ptr->flags8 |= (RF8_DROP_CLOTHES);
					break;
				}
				case TV_SOFT_ARMOR:
				{
					hack_monster_equip |= (RF8_DROP_ARMOR);
					l_ptr->flags8 |= (RF8_DROP_CLOTHES);
					break;
				}

				case TV_HELM:
				{
					hack_monster_equip |= (RF8_HAS_HEAD);
					l_ptr->flags8 |= (RF8_DROP_ARMOR);
					break;
				}
				case TV_SHIELD:
				{
					hack_monster_equip |= (RF8_HAS_ARM);
					l_ptr->flags8 |= (RF8_DROP_ARMOR);
					break;
				}
				case TV_DRAG_ARMOR:
				case TV_HARD_ARMOR:
				{
					hack_monster_equip |= (RF8_DROP_ARMOR);
					l_ptr->flags8 |= (RF8_DROP_ARMOR);
					break;
				}

				case TV_CROWN:
				{
					hack_monster_equip |= (RF8_HAS_HEAD);
					l_ptr->flags8 |= (RF8_DROP_JEWELRY);
					break;
				}
				case TV_AMULET:
				{
					hack_monster_equip |= (RF8_HAS_SKULL);
					l_ptr->flags8 |= (RF8_DROP_JEWELRY);
					break;
				}
				case TV_RING:
				{
					hack_monster_equip |= (RF8_HAS_HAND);
					l_ptr->flags8 |= (RF8_DROP_JEWELRY);
					break;
				}

				case TV_LITE:
				{
					hack_monster_equip |= (RF8_DROP_LITE);
					l_ptr->flags8 |= (RF8_DROP_LITE);
					break;
				}

				case TV_STAFF:
				{
					hack_monster_equip |= (RF8_DROP_WEAPON);
				}
				case TV_ROD:
				case TV_WAND:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_RSW);
					l_ptr->flags8 |= (RF8_DROP_RSW);
					break;
				}

				case TV_SCROLL:
				case TV_MAP:
				case TV_MAGIC_BOOK:
				case TV_PRAYER_BOOK:
				case TV_RUNESTONE:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_WRITING);
					l_ptr->flags8 |= (RF8_DROP_WRITING);
					break;
				}

				case TV_POTION:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_POTION);
					l_ptr->flags8 |= (RF8_DROP_POTION);
					break;
				}

				case TV_MUSHROOM:
				case TV_FOOD:
				{
					if (rand_int(100) < 50) hack_monster_equip |= (RF8_DROP_FOOD);
					l_ptr->flags8 |= (RF8_DROP_FOOD);
					break;
				}
			}

			/* Assume seen XXX XXX XXX */
			dump_item++;
		}

		/* Set origin */
		i_ptr->origin = visible ? ORIGIN_DROP : ORIGIN_DROP_UNKNOWN;
		i_ptr->origin_depth = p_ptr->depth;
		i_ptr->origin_xtra = m_ptr->r_idx;

		/* Drop it in the dungeon */
		monster_carry(m_idx, i_ptr);
	}

	/* Reset monster equipment */
	hack_monster_equip = 0L;

	/* Reset the object level */
	object_level = p_ptr->depth;

	/* Reset "food" type */
	food_type = 0;

	/* Reset "coin" type */
	coin_type = 0;

	/* Reset "tval drop" type */
	tval_drop_idx = 0;

	/* Reset "monster drop" type */
	race_drop_idx = 0;

	/* Take note of any dropped treasure */
	if (visible && (dump_item || dump_gold))
	{
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}

	/* We've made the inventory for this monster */
	m_ptr->mflag |= (MFLAG_MADE);

	/* Was anything dropped? */
	return (dump_chest || dump_item || dump_gold);
}


/*
 * Handle the "death" of a monster.
 *
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 *
 * Check for "Quest" completion when a quest monster is killed.
 *
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 *
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 */
bool monster_death(int m_idx)
{
	int y, x, ny, nx;

	s16b this_o_idx, next_o_idx = 0;

	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	object_type *i_ptr;
	object_type object_type_body;

	quest_event quest_check;

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;

	/* Incur summoning debt */
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && ((m_ptr->mflag & (MFLAG_TOWN)) == 0) && (m_ptr->summoned))
	{
		int debt = r_ptr->level;
		
		/* Group monsters incur less debt */
		if (r_ptr->flags1 & (RF1_FRIENDS)) debt -= 10;
		else if (r_ptr->flags1 & (RF1_FRIEND)) debt -= 5;
		
		/* Weak monsters incur less debt */
		if (r_ptr->level < p_ptr->depth - 5) debt -= 5;
		
		/* Ensure minimum debt */
		if (debt < 3) debt = 3;
		
		/* Summoning debt requires blood */
		if (debt > p_ptr->csp)
		{
			/* Incur blood debt */
			take_hit(SOURCE_BLOOD_DEBT, m_ptr->r_idx, damroll(debt - p_ptr->csp, 3));

			/* No mana left */
			p_ptr->csp = 0;
			p_ptr->csp_frac = 0;
		}

		/* Debt can be met by mana */
		else if (debt > 0)
		{
			p_ptr->csp -= debt;
		}

		/* Update mana */
		p_ptr->update |= (PU_MANA);
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1 | PW_PLAYER_2 | PW_PLAYER_3);

		/* Player death */
		if (p_ptr->is_dead) return (TRUE);
	}

	/* Extinguish lite */
	delete_monster_lite(m_idx);

	/* Clear the quest */
	WIPE(&quest_check, quest_event);

	/* Set the quest */
	quest_check.flags = EVENT_KILL_RACE;
	quest_check.race = m_ptr->r_idx;
	quest_check.number = 1;

	/* Check quest events */
	check_quest(&quest_check, TRUE);

	/* Do we drop more treasure? */
	if ((m_ptr->mflag & (MFLAG_MADE)) == 0)
	{
		/* Give the monster the items to carry */
		monster_drop(m_idx);
	}

	/* Drop objects being carried */
	for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Paranoia */
		o_ptr->held_m_idx = 0;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Copy the object */
		object_copy(i_ptr, o_ptr);

		/* Delete the object */
		delete_object_idx(this_o_idx);

		/* Drop it */
		drop_near(i_ptr, -1, y, x, TRUE);
	}

	/* Forget objects */
	m_ptr->hold_o_idx = 0;

	/* Mega-Hack -- drop "winner" treasures */
	if (r_ptr->flags1 & (RF1_DROP_CHOSEN))
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Mega-Hack -- Prepare to make "Grond" */
		object_prep(i_ptr, lookup_kind(TV_HAFTED, SV_GROND));

		/* Mega-Hack -- Mark this item as "Grond" */
		i_ptr->name1 = ART_GROND;

		/* Mega-Hack -- Actually create "Grond" */
		apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);

		/* Mark origin */
		i_ptr->origin = ORIGIN_DROP;
		i_ptr->origin_depth = p_ptr->depth;
		i_ptr->origin_xtra = m_ptr->r_idx;

		/* Drop it in the dungeon */
		drop_near(i_ptr, -1, y, x, TRUE);

		/* Get local object */
		i_ptr = &object_type_body;

		/* Mega-Hack -- Prepare to make "Morgoth" */
		object_prep(i_ptr, lookup_kind(TV_CROWN, SV_MORGOTH));

		/* Mega-Hack -- Mark this item as "Morgoth" */
		i_ptr->name1 = ART_MORGOTH;

		/* Mega-Hack -- Actually create "Morgoth" */
		apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);

		/* Mark origin */
		i_ptr->origin = ORIGIN_DROP;
		i_ptr->origin_depth = p_ptr->depth;
		i_ptr->origin_xtra = m_ptr->r_idx;

		/* Drop it in the dungeon */
		drop_near(i_ptr, -1, y, x, TRUE);

		/* Hack -- this is temporary */
		/* Total winner */
		p_ptr->total_winner = TRUE;

		/* Redraw the "title" */
		p_ptr->redraw |= (PR_TITLE);

		/* Congratulations */
		msg_print("*** CONGRATULATIONS ***");
		msg_print("You have won the game!");
		msg_print("You may retire (commit suicide) when you are ready.");

	}

	/* Hack -- only sometimes drop bodies */
	if (((r_ptr->flags1 & (RF1_UNIQUE)) != 0) ||
		((r_ptr->flags2 & (RF2_REGENERATE)) != 0) ||
		((r_ptr->flags8 & (RF8_ASSEMBLY)) != 0) ||
		(r_ptr->level > p_ptr->depth) ||
		(rand_int(100) < 30))
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Drop a body? */
		if (make_body(i_ptr, m_ptr->r_idx))
		{
			/* Note who dropped it */
			i_ptr->name3 = m_ptr->r_idx;

			/* I'll be back, Bennett */
			if (r_ptr->flags2 & (RF2_REGENERATE)) i_ptr->timeout = damroll(3,6);

			/* Drop it in the dungeon */
			drop_near(i_ptr, -1, y, x, TRUE);
		}
	}

	/* Monster death updates visible monsters */
	p_ptr->window |= (PW_MONLIST);

	/* Add some residue */
	if (r_ptr->flags3 & (RF3_DEMON)) feat_near(FEAT_FLOOR_FIRE_T,m_ptr->fy,m_ptr->fx);
	if (r_ptr->flags3 & (RF3_UNDEAD)) feat_near(FEAT_FLOOR_DUST_T,m_ptr->fy,m_ptr->fx);
	if (r_ptr->flags8 & (RF8_HAS_SLIME)) feat_near(FEAT_FLOOR_SLIME_T,m_ptr->fy,m_ptr->fx);

	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & (RF1_QUESTOR | RF1_GUARDIAN)))
		return (TRUE);

	/* Sauron forms */
	if ((m_ptr->r_idx >= SAURON_FORM) && (m_ptr->r_idx < SAURON_FORM + MAX_SAURON_FORMS))
	{
		/* Killing this form means there is a chance of the true form being revealed */
		p_ptr->sauron_forms |= (1 << (m_ptr->r_idx - SAURON_FORM));

		/* XXX We have to manually mark it dead here */
		r_info[m_ptr->r_idx].max_num = 0;

		/* Sauron changes form */
		m_ptr->r_idx = sauron_shape(m_ptr->r_idx);

		/* Paranoia */
		if (!m_ptr->r_idx) m_ptr->r_idx = SAURON_TRUE;

		/* And gets back his hitpoints / mana */
		m_ptr->hp = m_ptr->maxhp;
		m_ptr->mana = r_ptr->mana;

		/* Message for the player */
		if (m_ptr->r_idx == SAURON_TRUE)
		{
			msg_print("Sauron's form shifts, revealing his true shape.");
			msg_print("He is vulnerable and can be killed permanently while in this form.");
		}
		else
		{
			msg_print("Sauron's form shifts fluidly.");
			msg_print("You have destroyed this shape, but must continue the fight!");
		}

		return (FALSE);
	}

	/* Dungeon guardian defeated - need some stairs except on surface */
	if (r_ptr->flags1 & (RF1_GUARDIAN))
	{
		/* Generate stairs if path is opened from this location */
		if ((p_ptr->depth != min_depth(p_ptr->dungeon)) &&
				/* Last quest on level */
				(p_ptr->depth == max_depth(p_ptr->dungeon)))
		{
			/* Stagger around */
			while (!cave_valid_bold(y, x) && !cave_clean_bold(y,x))
		    {
				int d = 1;

				/* Pick a location */
				scatter(&ny, &nx, y, x, d, 0);

		    	/* Stagger */
				y = ny; x = nx;
		    }

		  /* Explain the staircase */
		  msg_print("A magical staircase appears...");

		  /* Create stairs */
		  if (level_flag & (LF1_TOWER))
			  cave_set_feat(y, x, FEAT_LESS);
		  else
			  cave_set_feat(y, x, FEAT_MORE);

		  /* Save any objects in that place */
		  scatter_objects_under_feat(y, x);
		}
	}

	/* Hack -- Finishing quest 1 completes the game */
	/* Nothing left, game over... */
	if (q_list[1].stage == QUEST_REWARD)
	{
		/* Total winner */
		p_ptr->total_winner = TRUE;

		/* Redraw the "title" */
		p_ptr->redraw |= (PR_TITLE);

		/* Congratulations */
		msg_print("*** CONGRATULATIONS ***");
		msg_print("You have won the game!");
		msg_print("You may retire (commit suicide) when you are ready.");
	}

	return (TRUE);
}




/*
 * Decrease a monster's hit points, handle monster death.
 *
 * We return TRUE if the monster has been killed (and deleted).
 *
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 *
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 *
 * Invisible monsters induce a special "You have killed it." message.
 *
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 *
 * Consider decreasing monster experience over time, say, by using
 * "(m_exp * m_lev * (m_lev)) / (p_lev * (m_lev + n_killed))" instead
 * of simply "(m_exp * m_lev) / (p_lev)", to make the first monster
 * worth more than subsequent monsters.  This would also need to
 * induce changes in the monster recall code.  XXX XXX XXX
 */
bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note)
{
	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	/* Redraw (later) if needed */
	if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Are we hurting it badly? */
	if (((m_ptr->maxhp / 3) < dam) && (m_ptr->maxhp > rand_int(100)))
	{
		object_type *i_ptr;
		object_type object_type_body;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Rip off a head */
		if ((m_ptr->hp < dam) && make_head(i_ptr,m_ptr->r_idx))
		{
			/* Note who dropped it */
			i_ptr->name3 = m_ptr->r_idx;

			/* Drop it in the dungeon */
			drop_near(i_ptr, -1, m_ptr->fy, m_ptr->fx, TRUE);
		}
		/* Rip off a limb */
		else if ((m_ptr->hp - dam < m_ptr->maxhp / 10) && make_part(i_ptr,m_ptr->r_idx))
		{
			/* Note who dropped it */
			i_ptr->name3 = m_ptr->r_idx;

			/* Drop it in the dungeon */
			drop_near(i_ptr, -1, m_ptr->fy, m_ptr->fx, TRUE);
		}

		/* Rip off some skin */
		else if (make_skin(i_ptr,m_idx))
		{
			/* Note who dropped it */
			i_ptr->name3 = m_ptr->r_idx;

			/* Drop it in the dungeon */
			drop_near(i_ptr, -1, m_ptr->fy, m_ptr->fx, TRUE);
		}

		/* Add some blood */
		if (r_ptr->flags8 & (RF8_HAS_BLOOD)) feat_near(FEAT_FLOOR_BLOOD_T,m_ptr->fy,m_ptr->fx);

		/* Add some slime */
		if (r_ptr->flags8 & (RF8_HAS_SLIME)) feat_near(FEAT_FLOOR_SLIME_T,m_ptr->fy,m_ptr->fx);
	}

	/* Hurt it */
	m_ptr->hp -= dam;
	
	/* We are damaging - count damage */
	if ((l_ptr->tdamage < 10000) && (m_ptr->ml)) l_ptr->tdamage++;

	/* It is dead now */
	if (m_ptr->hp < 0)
	{
		char m_name[80];

		/* Extract monster name */
		monster_desc(m_name, sizeof(m_name), m_idx, 0);

		/* Death by Missile/Spell attack */
		if (note)
		{
			message_format(MSG_KILL, m_ptr->r_idx, "%^s%s", m_name, note);
		}

		/* Death by physical attack -- invisible monster */
		else if (!m_ptr->ml)
		{
			message_format(MSG_KILL, m_ptr->r_idx, "You have killed %s.", m_name);
		}

		/* Death by Physical attack -- non-living monster */
		else if ((r_ptr->flags3 & (RF3_NONLIVING)) ||
			 (r_ptr->flags2 & (RF2_STUPID)))
		{
			message_format(MSG_KILL, m_ptr->r_idx, "You have destroyed %s.", m_name);
		}

		/* Death by Physical attack -- living monster */
		else
		{
			message_format(MSG_KILL, m_ptr->r_idx, "You have slain %s.", m_name);
		}

		/* Death by Physical attack -- non-living monster */
		if ((r_ptr->flags3 & (RF3_NONLIVING)) ||
			 (r_ptr->flags2 & (RF2_STUPID)))
		{
			/* Warn allies */
			tell_allies_death(m_ptr->fy, m_ptr->fx, "& has destroyed one of us!");
		}
		else
		{
			/* Warn allies */
			tell_allies_death(m_ptr->fy, m_ptr->fx, "& has killed one of us!");
		}

		/* Death of player allies doesn't provide experience */
		if ((m_ptr->mflag & (MFLAG_ALLY)) == 0)
		{
			s32b mult, div, new_exp, new_exp_frac;

			/* 10 + killed monster level */
			mult = 10 + r_ptr->level;

			/* 10 + maximum player level */
			div = 10 + p_ptr->max_lev;

			/* Give some experience for the kill */
			new_exp = ((long)r_ptr->mexp * mult) / div;

			/* Handle fractional experience */
			new_exp_frac = ((((long)r_ptr->mexp * mult) % div)
					* 0x10000L / div) + p_ptr->exp_frac;

			/* Keep track of experience */
			if (new_exp_frac >= 0x10000L)
			{
				new_exp++;
				p_ptr->exp_frac = (u16b)(new_exp_frac - 0x10000L);
			}
			else
			{
				p_ptr->exp_frac = (u16b)new_exp_frac;
			}

			/* Gain experience */
			gain_exp(new_exp);
		}

		/* Generate treasure */
		if (!monster_death(m_idx)) return (FALSE);

		/* When the player kills a Unique, it stays dead */
		if (r_ptr->flags1 & (RF1_UNIQUE)) r_ptr->max_num = 0;

		/* Recall even invisible uniques or winners */
		if (m_ptr->ml || (r_ptr->flags1 & (RF1_UNIQUE)))
		{
			/* Count kills this life */
			if (l_ptr->pkills < MAX_SHORT) l_ptr->pkills++;

			/* Show killer tips */
			if (!l_ptr->tkills) queue_tip(format("kill%d.txt", m_ptr->r_idx));

			/* Count kills in all lives */
			if (l_ptr->tkills < MAX_SHORT) l_ptr->tkills++;

			/* Hack -- Auto-recall */
			monster_race_track(m_ptr->r_idx);
		}

		/* Delete the monster */
		delete_monster_idx(m_idx);

		/* Not afraid */
		(*fear) = FALSE;

		/* Monster is dead */
		return (TRUE);
	}

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0))
	{
		int tmp = randint(dam);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear)
		{
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}

		/* Cure all the fear */
		else
		{
			/* Cure fear */
			set_monster_fear(m_ptr, 0, FALSE);

			/* No more fear */
			(*fear) = FALSE;

			/* Warn allies */
			tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), "& has attacked me!");
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!m_ptr->monfear && !(r_ptr->flags3 & (RF3_NO_FEAR)) && (dam > 0))
	{
		long oldhp, newhp, tmp;
		int percentage, fitness;

		/* Note -- subtle fix -CFT */
		newhp = (long)(m_ptr->hp);
		oldhp = newhp + (long)(dam);
		tmp = (100L * dam) / (oldhp);
		percentage = (int)(tmp);

		/* Percentage of fully healthy. Note maxhp can be zero. */
		fitness = (100L * (m_ptr->hp + 1)) / (m_ptr->maxhp + 1);

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (more often) when hit for 11% or more its current hit points
		 *
		 * Percentages depend on player's charisma.
		 */
		if ((randint(adj_chr_fear[p_ptr->stat_ind[A_CHR]]) >= fitness) ||
		    ((percentage > 20) && (rand_int(adj_chr_fear[p_ptr->stat_ind[A_CHR]] * percentage) > 100)) )
		{
			int fear_amt;

			/* Hack -- note fear */
			(*fear) = TRUE;

			/* Hack -- Add some timed fear */
			fear_amt = rand_range(20, 30);

			/* Get frightened */
			set_monster_fear(m_ptr, fear_amt, TRUE);
		}
	}

	/* Monster will always go active */
	m_ptr->mflag |= (MFLAG_ACTV);

	/* Recalculate desired minimum range */
	if (dam > 0) m_ptr->min_range = 0;

	/* Not dead yet */
	return (FALSE);
}


/*
 * Modify the current panel to the given coordinates, adjusting only to
 * ensure the coordinates are legal, and return TRUE if anything done.
 *
 * Hack -- The town should never be scrolled around.
 *
 * Note that monsters are no longer affected in any way by panel changes.
 *
 * As a total hack, whenever the current panel changes, we assume that
 * the "overhead view" window should be updated.
 */
bool modify_panel(int wy, int wx)
{
	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Verify wy, adjust if needed */
	if ((level_flag & (LF1_TOWN)) != 0)
	{
		if (wy > TOWN_HGT - SCREEN_HGT) wy = TOWN_HGT - SCREEN_HGT;
		else if (wy < 0) wy = 0;
	}
	else if (wy > DUNGEON_HGT - SCREEN_HGT) wy = DUNGEON_HGT - SCREEN_HGT;

	if (wy < 0) wy = 0;

	/* Verify wx, adjust if needed */
	if ((level_flag & (LF1_TOWN)) != 0)
	{
		if (wx > TOWN_WID - SCREEN_WID) wx = TOWN_WID - SCREEN_WID;
		else if (wx < 0) wx = 0;
	}
	else if (wx > DUNGEON_WID - SCREEN_WID) wx = DUNGEON_WID - SCREEN_WID;

	if (wx < 0) wx = 0;

	/* React to changes */
	if ((p_ptr->wy != wy) || (p_ptr->wx != wx))
	{
		/* Save wy, wx */
		p_ptr->wy = wy;
		p_ptr->wx = wx;

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Hack -- Window stuff */
		p_ptr->window |= (PW_OVERHEAD);

		/* Changed */
		return (TRUE);
	}

	/* No change */
	return (FALSE);
}


/*
 * Perform the minimum "whole panel" adjustment to ensure that the given
 * location is contained inside the current panel, and return TRUE if any
 * such adjustment was performed.
 */
bool adjust_panel(int y, int x)
{
	int wy = p_ptr->wy;
	int wx = p_ptr->wx;

	/* Adjust as needed */
	while (y >= wy + SCREEN_HGT) wy += SCREEN_HGT;
	while (y < wy) wy -= SCREEN_HGT;

	/* Adjust as needed */
	while (x >= wx + SCREEN_WID) wx += SCREEN_WID;
	while (x < wx) wx -= SCREEN_WID;

	/* Use "modify_panel" */
	return (modify_panel(wy, wx));
}


/*
 * Change the current panel to the panel lying in the given direction.
 *
 * Return TRUE if the panel was changed.
 */
bool change_panel(int dir)
{
	int wy = p_ptr->wy + ddy[dir] * PANEL_HGT;
	int wx = p_ptr->wx + ddx[dir] * PANEL_WID;

	/* Use "modify_panel" */
	return (modify_panel(wy, wx));
}

/*
 * Hack - generate the current room description
 */
static void get_room_desc(int room, char *name, int name_s, char *text_visible, int text_visible_s, char *text_always, int text_always_s)
{
	bool scan_name = FALSE;
	bool scan_desc = FALSE;
	bool beware = FALSE;

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	/* Initialize text */
	my_strcpy(name, "", name_s);
	if (text_always) my_strcpy(text_always,"", text_always_s);
	if (text_visible) my_strcpy(text_visible,  cheat_xtra ? format("Room %d:", room) : "", text_visible_s);

	/* Town or not in room */
	if (!room)
	{
		if (level_flag & (LF1_SURFACE))
		{
			current_long_level_name(name);

			if (!text_always) return;

			/* Describe location */
			my_strcpy(text_always, t_text + t_ptr->text, text_always_s);
		}
		else
		{
			my_strcpy(name, "empty room", name_s);
		}
		return;
	}

	/* In room */
	switch (room_info[room].type)
	{
		case (ROOM_TOWER):
		{
			my_strcpy(name, "the tower of ", name_s);
			/* only short dungeon name here, long usually has 'tower' in it already */
			my_strcat(name, t_name + t_ptr->name, name_s);

			/* Brief description */
			if (text_visible) my_strcpy(text_visible, "This tower is filled with monsters and traps.  ", text_visible_s);

			/* Describe height of tower */
			if (text_visible)
			{
				int height = (1 + max_depth(p_ptr->dungeon)
								  - min_depth(p_ptr->dungeon));
				my_strcpy(text_visible, format("It looks about %d %s tall.  ", depth_in_feet ? height * 50 : height, depth_in_feet ? "feet" : "stories"), text_visible_s);
			}
			break;
		}

		case (ROOM_LAIR):
		{
			monster_race *r_ptr = &r_info[room_info[room].deepest_race];

			my_strcpy(name, "the lair of ", name_s);
			my_strcat(name, r_name + r_ptr->name, name_s);
			if ((r_ptr->flags1 & (RF1_UNIQUE)) == 0) my_strcat(name, "s", name_s);

			if (text_visible) my_strcpy(text_visible, "This is the lair of ", text_visible_s);
			if (text_visible) my_strcat(text_visible, r_name + r_ptr->name, text_visible_s);
			if ((text_visible) && ((r_ptr->flags1 & (RF1_UNIQUE)) == 0)) my_strcat(name, "s", text_visible_s);
			if (text_visible) my_strcat(text_visible, ".  ", text_visible_s);
			beware = TRUE;

			break;
		}

		case (ROOM_GREATER_VAULT):
		{
			my_strcpy(name, "greater ", name_s);

			if (text_visible) my_strcpy(text_visible, "This vast sealed chamber is amongst the largest of its kind and is filled with ", text_visible_s);
			if (text_visible) my_strcat(text_visible, "deadly monsters and rich treasure.  ", text_visible_s);
			beware = TRUE;

			/* Fall through */
		}

		case (ROOM_LESSER_VAULT):
		{
			/* Display vault name */
			if ((v_name + v_info[room_info[room].vault].name)[0] == '\'')
			{
				my_strcat(name, "vault ", name_s);
				my_strcat(name, v_name + v_info[room_info[room].vault].name, name_s);
			}
			else
			{
				my_strcat(name, "vault", name_s);
			}

			scan_desc = TRUE;
			break;
		}

		case (ROOM_INTERESTING):
		{
			my_strcat(name, v_name + v_info[room_info[room].vault].name, name_s);
			if (text_visible) my_strcpy(text_visible, "There is something remarkable here.  ", text_visible_s);
			break;
		}

		case (ROOM_CHAMBERS):
		{
			if (text_visible) my_strcpy(text_visible, "This is one of many rooms crowded with monsters.  ", text_visible_s);
			scan_name = TRUE;
			scan_desc = TRUE;
			break;
		}

		default:
		{
			scan_name = TRUE;
			scan_desc = TRUE;
			break;
		}
	}

	/* Read through and display the description if required */
	if ((scan_name) || (scan_desc))
	{
		int i, j;

		char buf_name1[16];
		char buf_name2[16];
		char *last_buf = NULL;
		int last_buf_s = 0;

		/* Clear the name1 text */
		buf_name1[0] = '\0';

		/* Clear the name2 text */
		buf_name2[0] = '\0';

		i = 0;

		if (cheat_xtra && text_always) my_strcat(text_visible, format ("%s (%ld)", r_name + r_info[room_info[room].deepest_race].name, room_info[room].ecology), text_visible_s);

		while ((room >= 0) && (i < ROOM_DESC_SECTIONS))
		{
			/* Get description */
			j = room_info[room].section[i++];

			/* End of description */
			if (j < 0) break;

			/* Get the name1 text if needed */
			if (!strlen(buf_name1)) my_strcpy(buf_name1, (d_name + d_info[j].name1), sizeof(buf_name1));

			/* Get the name2 text if needed */
			if (!strlen(buf_name2)) my_strcpy(buf_name2, (d_name + d_info[j].name2), sizeof(buf_name2));

			/* Need description? */
			if (!scan_desc) continue;

			/* Must understand language? */
			if (d_info[j].flags & (ROOM_LANGUAGE))
			{
				/* Does the player understand the main language of the level? */
				if ((last_buf) && (cave_ecology.ready) && (cave_ecology.num_races)
					&& player_understands(monster_language(room_info[room].deepest_race)))
				{
					/* Get the textual history */
					my_strcat(last_buf, (d_text + d_info[j].text), last_buf_s);
				}
				else if (last_buf)
				{
					/* Fake it */
					my_strcat(last_buf, "nothing you can understand.  ", last_buf_s);

					/* Clear last buf to skip remaining language lines */
					last_buf = NULL;
				}

				/* Diagnostics */
				if ((cheat_xtra) && (last_buf)) my_strcat(last_buf, format("%d", i), last_buf_s);
			}

			/* Visible description */
			else if (d_info[j].flags & (ROOM_SEEN))
			{
				/* Get the textual history */
				if (text_visible) my_strcat(text_visible, (d_text + d_info[j].text), text_visible_s);

				/* Record last buffer for language */
				last_buf = text_visible;
				last_buf_s = text_visible_s;

				/* Diagnostics */
				if ((cheat_xtra) && (text_visible)) my_strcat(text_visible, format("%d", i), text_visible_s);
			}

			/* Description always present */
			else
			{
				/* Get the textual history */
				if (text_always) my_strcat(text_always, (d_text + d_info[j].text), text_always_s);

				/* Record last buffer for language */
				last_buf = text_always;
				last_buf_s = text_always_s;

				/* Diagnostics */
				if ((cheat_xtra) && (text_always)) my_strcat(text_always, format("%d", i), text_always_s);
			}
		}

		/* Set the name if required */
		if (scan_name)
		{

			/* Set room name */
			if (strlen(buf_name1)) my_strcpy(name, buf_name1, name_s);

			/* And add second room name if necessary */
			if (strlen(buf_name2))
			{
				if (strlen(buf_name1))
				{
					my_strcat(name, " ", name_s);
					my_strcat(name, buf_name2, name_s);
				}
				else
				{
					my_strcpy(name, buf_name2, name_s);
				}
			}
		}
	}

	/* Beware */
	if ((beware) && (text_always)) my_strcpy(text_always, "Beware!  ", text_always_s);

	if ((cheat_room) && (text_always))
	{
		if (room_info[room].flags & (ROOM_ICKY)) my_strcat(text_always,"This room cannot be teleported into.  ", text_always_s);
		if (room_info[room].flags & (ROOM_BLOODY)) my_strcat(text_always,"This room prevent you naturally healing your wounds.  ", text_always_s);
		if (room_info[room].flags & (ROOM_CURSED)) my_strcat(text_always,"This room makes you vulnerable to being hit.  ", text_always_s);
		if (room_info[room].flags & (ROOM_GLOOMY)) my_strcat(text_always,"This room cannot be magically lit.  ", text_always_s);
		if (room_info[room].flags & (ROOM_PORTAL)) my_strcat(text_always,"This room magically teleports you occasionally.  ", text_always_s);
		if (room_info[room].flags & (ROOM_SILENT)) my_strcat(text_always,"This room prevents you casting spells.  ", text_always_s);
		if (room_info[room].flags & (ROOM_STATIC)) my_strcat(text_always,"This room prevents you using wands, staffs or rods.  ", text_always_s);
	}

}


/*
 * Hack -- Display the "name" of a given room
 */
static void room_info_top(int room)
{
	char first[2];
	char name[70];
	char text_visible[1024];
	char text_always[1024];

	/* Hack -- handle "xtra" mode */
	if (!character_dungeon) return;

	/* Get the actual room description */
	get_room_desc(room, name, sizeof(name), text_visible, sizeof(text_visible), text_always, sizeof(text_always));

	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

	/* Hack - set first character to upper */
	if (name[0] >= 'a') first[0] = name[0]-32;
	else first[0] = name[0];
	first[1] = '\0';

	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, first);

	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, (name+1));

}

/*
 * Hack -- describe the given room at the top of the screen
 */
static void screen_room_info(int room)
{
	char name[62];
	char text_visible[1024];
	char text_always[1024];

	/* Hack -- handle "xtra" mode */
	if (!character_dungeon) return;

	/* Get the actual room description */
	get_room_desc(room, name, sizeof(name), text_visible, sizeof(text_visible), text_always, sizeof(text_always));

	/* Flush messages */
	msg_print(NULL);

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	/* Begin recall */
	Term_erase(0, 1, 255);

	/* Describe room */
	if ((strlen(text_visible)) && (room_info[room].flags & (ROOM_SEEN)))
	{
		/* Recall monster */
		text_out(text_visible);

		if (strlen(text_always))
		{
			text_out(text_always);
		}

	}
	else if (strlen(text_always))
	{
		text_out(text_always);
	}
	else
	{
		text_out("There is nothing remarkable about it.");
	}

	/* Describe room */
	room_info_top(room);
}


/*
 * Hack -- describe the given room info in the current "term" window
 */
void display_room_info(int room)
{
	int y;
	char name[62];
	char text_visible[1024];
	char text_always[1024];

	/* Hack -- handle "xtra" mode */
	if (!character_dungeon) return;

	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* Begin recall */
	Term_gotoxy(0, 1);

	/* Get the actual room description */
	get_room_desc(room, name, sizeof(name), text_visible, sizeof(text_visible), text_always, sizeof(text_always));

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	/* Describe room */
	if ((strlen(text_visible)) && (room_info[room].flags & (ROOM_SEEN)))
	{
		/* Recall monster */
		text_out(text_visible);

		if (strlen(text_always))
		{
			text_out(text_always);
		}

	}
	else if (strlen(text_always))
	{
		text_out(text_always);
	}
	else
	{
		text_out("There is nothing remarkable about it.");
	}

	/* Describe room */
	room_info_top(room);
}

/*
 * Hack -- describe players current location.
 */
void describe_room(void)
{
	int room = room_idx(p_ptr->py, p_ptr->px);
	char name[62];
	char text_visible[1024];
	char text_always[1024];


#ifdef ALLOW_BORG
	/* Hack -- No descriptions for the borg */
	if (count_stop) return;
#endif

	/* Hack -- handle "xtra" mode */
	if (!character_dungeon) return;

	/* Hack -- not a room */
	if (!(cave_info[p_ptr->py][p_ptr->px] & (CAVE_ROOM))) room = 0;

	/* Get the actual room description */
	get_room_desc(room, name, sizeof(name), text_visible, sizeof(text_visible), text_always, sizeof(text_always));

	if ((cave_info[p_ptr->py][p_ptr->px] & (CAVE_GLOW))
	 && (cave_info[p_ptr->py][p_ptr->px] & (CAVE_ROOM)))
	{
		if (room_names)
		{
			msg_format("You have entered %s%s.",
				 (prefix(name, "the ") ? "" :
				 (is_a_vowel(name[0]) ? "an " : "a ")),name);
		}

		if (!(room_descriptions) || (room_info[room].flags & (ROOM_ENTERED)))
		{
			/* Nothing more */
		}
		else if ((!easy_more) && (!auto_more) && (strlen(text_visible) + strlen(text_always) > 80))
		{
			message_flush();

			screen_save();

			/* Set text_out hook */
			text_out_hook = text_out_to_screen;

			text_out_c(TERM_L_BLUE, name);
			text_out("\n");

			if (strlen(text_visible))
			{
				/* Message */
				text_out(text_visible);

				/* Add it to the buffer */
				message_add(text_visible, MSG_GENERIC);
			}

			if (strlen(text_always))
			{
				/* Message */
				text_out(text_always);

				/* Add it to the buffer */
				message_add(text_always, MSG_GENERIC);
			}

			(void)anykey();

			/* Load screen */
			screen_load();
		}
		else if (strlen(text_visible) && strlen(text_always))
		{
			msg_format("%s %s", text_visible, text_always);
		}
		else if (strlen(text_visible))
		{
			msg_print(text_visible);
		}
		else if (strlen(text_always))
		{
			msg_print(text_always);
		}

		/* Room has been entered */
		if (room_descriptions) room_info[room].flags |= (ROOM_ENTERED);
	}
	else if ((strlen(text_always)) && !(room_info[room].flags & (ROOM_HEARD)) &&
	  (cave_info[p_ptr->py][p_ptr->px] & (CAVE_ROOM)))
	{
		/* Message */
		if (room_descriptions) msg_format("%s", text_always);

		/* Room has been heard */
		if (room_descriptions) room_info[room].flags |= (ROOM_HEARD);
	}
	else if (level_flag & (LF1_SURFACE))
	{
		msg_format("You have entered %s.",name);

		if (strlen(text_always))
		{
			if (easy_more || (auto_more && !easy_more))
			{
				msg_print(text_always);
			}
			else
			{
				message_flush();

				screen_save();

				/* Set text_out hook */
				text_out_hook = text_out_to_screen;

				text_out_c(TERM_L_BLUE, name);
				text_out("\n");

				/* Message */
				text_out(text_always);

				(void)anykey();

				screen_load();
			}
		}
	}

	/* Window stuff */
	p_ptr->window |= (PW_ROOM_INFO);

}



/*
 * Verify the current panel (relative to the player location).
 *
 * By default, when the player gets "too close" to the edge of the current
 * panel, the map scrolls one panel in that direction so that the player
 * is no longer so close to the edge.
 *
 * The "center_player" option allows the current panel to always be centered
 * around the player, which is very expensive, and also has some interesting
 * gameplay ramifications.
 */
void verify_panel(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int wy = p_ptr->wy;
	int wx = p_ptr->wx;


	/* Scroll screen vertically when off-center */
	if (center_player &&
	    (py != wy + SCREEN_HGT / 2))
	{
		wy = py - SCREEN_HGT / 2;
	}

	/* Scroll screen vertically when 2 grids from top/bottom edge */
	else if ((py < wy + 2) || (py >= wy + SCREEN_HGT - 2))
	{
		wy = ((py - PANEL_HGT / 2) / PANEL_HGT) * PANEL_HGT;
	}


	/* Scroll screen horizontally when off-center */
	if (center_player &&
	    (px != wx + SCREEN_WID / 2))
	{
		wx = px - SCREEN_WID / 2;
	}

	/* Scroll screen horizontally when 4 grids from left/right edge */
	else if ((px < wx + 4) || (px >= wx + SCREEN_WID - 4))
	{
		wx = ((px - PANEL_WID / 2) / PANEL_WID) * PANEL_WID;
	}

	/* Scroll if needed */
	modify_panel(wy, wx);
}

/*
 * This centers the panel on the player
 */
void center_panel(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int wy = p_ptr->wy;
	int wx = p_ptr->wx;


	/* Scroll screen vertically when off-center */
	if (py != wy + SCREEN_HGT / 2)
	{
		wy = py - SCREEN_HGT / 2;
	}

	/* Scroll screen horizontally when off-center */
	if (px != wx + SCREEN_WID / 2)
	{
		wx = px - SCREEN_WID / 2;
	}

	/* Scroll if needed */
	modify_panel(wy, wx);
}


/*
 * Monster health description
 */
cptr look_mon_desc(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	bool living = TRUE;

	/* Report status effects */
	if (m_ptr->csleep) return("asleep");
	if (m_ptr->confused) return("confused");
	if (m_ptr->monfear) return("afraid");
	if (m_ptr->stunned) return("stunned");
	if (m_ptr->cut) return("bleeding");
	if (m_ptr->poisoned) return("poisoned");
	if (find_monster_ammo(m_idx, -1, FALSE) < 0) return("out of ammo");

	/* Real townsfolk */
	if (((m_ptr->mflag & (MFLAG_ALLY | MFLAG_AGGR | MFLAG_TOWN)) == (MFLAG_TOWN))
			&& ((level_flag & (LF1_TOWN)) != 0))
	{
		return("townsfolk");
	}

	/* Determine if the monster is "living" (vs "undead") */
	if (r_ptr->flags3 & (RF3_UNDEAD)) living = FALSE;
	if (r_ptr->flags3 & (RF3_DEMON)) living = FALSE;
	if (r_ptr->flags3 & (RF3_NONLIVING)) living = FALSE;

	/* Healthy monsters */
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		/* No damage */
		return (living ? "unhurt" : "undamaged");
	}
	else
	{
		/* Calculate a health "percentage" */
		int perc = 100L * m_ptr->hp / m_ptr->maxhp;

		if (perc >= 60)
			return(living ? "somewhat wounded" : "somewhat damaged");
		else if (perc >= 25)
			return (living ? "wounded" : "damaged");
		else if (perc >= 10)
			return (living ? "badly wounded" : "badly damaged");
		else
			return(living ? "almost dead" : "almost destroyed");
	}
}



/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(vptr u, vptr v, int p, int q)
{
	int z, a, b;

	/* Done sort */
	if (p >= q) return;

	/* Pivot */
	z = p;

	/* Begin */
	a = p;
	b = q;

	/* Partition */
	while (TRUE)
	{
		/* Slide i2 */
		while (!(*ang_sort_comp)(u, v, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_comp)(u, v, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_swap)(u, v, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_aux(u, v, p, b);

	/* Recurse right side */
	ang_sort_aux(u, v, b+1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(vptr u, vptr v, int n)
{
	/* Sort the array */
	ang_sort_aux(u, v, 0, n-1);
}





/*** Targetting Code ***/


/*
 * Given a "source" and "target" location, extract a "direction",
 * which will move one step from the "source" towards the "target".
 *
 * Note that we use "diagonal" motion whenever possible.
 *
 * We return "5" if no motion is needed.
 */
sint motion_dir(int y1, int x1, int y2, int x2)
{
	/* No movement required */
	if ((y1 == y2) && (x1 == x2)) return (5);

	/* South or North */
	if (x1 == x2) return ((y1 < y2) ? 2 : 8);

	/* East or West */
	if (y1 == y2) return ((x1 < x2) ? 6 : 4);

	/* South-east or South-west */
	if (y1 < y2) return ((x1 < x2) ? 3 : 1);

	/* North-east or North-west */
	if (y1 > y2) return ((x1 < x2) ? 9 : 7);

	/* Paranoia */
	return (5);
}


/*
 * Extract a direction (or zero) from a character
 */
sint target_dir(char ch)
{
	int d;

	int mode;

	cptr act;

	cptr s;


	/* Default direction */
	d = (isdigit(ch) ? D2I(ch) : 0);

	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}

	/* Extract the action (if any) */
	act = keymap_act[mode][(byte)(ch)];

	/* Analyze */
	if (act)
	{
		/* Convert to a direction */
		for (s = act; *s; ++s)
		{
			/* Use any digits in keymap */
			if (isdigit(*s)) d = D2I(*s);
		}
	}

	/* Paranoia */
	if (d == 5) d = 0;

	/* Return direction */
	return (d);
}


/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targetting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(int m_idx)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	monster_type *m_ptr;

	/* No monster */
	if (m_idx <= 0) return (FALSE);

	/* Get monster */
	m_ptr = &m_list[m_idx];

	/* Monster must be alive */
	if (!m_ptr->r_idx) return (FALSE);

	/* Monster must be visible */
	if (!m_ptr->ml) return (FALSE);

	/* Monster must be projectable */
	if (!projectable(py, px, m_ptr->fy, m_ptr->fx, 0)) return (FALSE);

	/* Hack -- no targeting hallucinations */
	if (p_ptr->timed[TMD_IMAGE]) return (FALSE);

	/* Hack -- Never target trappers XXX XXX XXX */
	/* if (CLEAR_ATTR && (CLEAR_CHAR)) return (FALSE); */

	/* Assume okay */
	return (TRUE);
}




/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(void)
{
	/* No target */
	if (!p_ptr->target_set) return (FALSE);

	/* Accept "location" targets */
	if (p_ptr->target_who == 0) return (TRUE);

	/* Check "monster" targets */
	if (p_ptr->target_who > 0)
	{
		int m_idx = p_ptr->target_who;

		/* Accept reasonable targets */
		if (target_able(m_idx))
		{
			monster_type *m_ptr = &m_list[m_idx];

			/* Get the monster location */
			p_ptr->target_row = m_ptr->fy;
			p_ptr->target_col = m_ptr->fx;

			/* Good target */
			return (TRUE);
		}
	}

	/* Assume no target */
	return (FALSE);
}


/*
 * Get allies to adopt the player's target.
 *
 * If order is true, we order allies to this.
 * If order is false, only allies without targets will go here.
 */
static void player_tell_allies_target(int y, int x, bool order)
{
	int i;

	/* Get allies to target this location */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip non-allies, or allies who ignore the player */
		if ( ((m_ptr->mflag & (MFLAG_ALLY)) == 0) || ((m_ptr->mflag & (MFLAG_IGNORE)) != 0) ) continue;

		/* Skip unseen monsters that the player cannot speak to or telepathically communicate with */
		if (!m_ptr->ml && ((r_info[m_ptr->r_idx].flags3 & (RF3_NONVOCAL)) == 0))
		{
			/* Cannot hear the player */
			if ((m_ptr->cdis > 3) && ((play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE)) == 0)) continue;

			/* Cannot understand the player */
			if (!player_understands(monster_language(m_ptr->r_idx))) continue;
		}

		/* Skip monsters with targets already */
		if ((!order) && (m_ptr->ty || m_ptr->tx)) continue;

		/* Set the monster target */
		m_ptr->ty = y;
		m_ptr->tx = x;
	}
}


/*
 * Set the target to a monster (or nobody)
 */
void target_set_monster(int m_idx, s16b flags)
{
	/* Acceptable target */
	if ((m_idx > 0) && target_able(m_idx))
	{
		monster_type *m_ptr = &m_list[m_idx];

		/* Save target info */
		p_ptr->target_set = flags | (TARGET_KILL);
		p_ptr->target_who = m_idx;
		p_ptr->target_row = m_ptr->fy;
		p_ptr->target_col = m_ptr->fx;

		/* Get allies to target this */
		player_tell_allies_target(m_ptr->fy, m_ptr->fx, FALSE);
	}

	/* Clear target */
	else
	{
		/* Reset target info */
		p_ptr->target_set = 0;
		p_ptr->target_who = 0;
		p_ptr->target_row = 0;
		p_ptr->target_col = 0;
	}
}


/*
 * Set the target to a location
 */
void target_set_location(int y, int x, s16b flags)
{
	/* Legal target */
	if (in_bounds_fully(y, x))
	{
		/* Save target info */
		p_ptr->target_set = flags | (TARGET_GRID);
		p_ptr->target_who = 0;
		p_ptr->target_row = y;
		p_ptr->target_col = x;

		/* Get allies to target this */
		player_tell_allies_target(y,x, FALSE);
	}

	/* Clear target */
	else
	{
		/* Reset target info */
		p_ptr->target_set = 0;
		p_ptr->target_who = 0;
		p_ptr->target_row = 0;
		p_ptr->target_col = 0;
	}
}


/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
static bool ang_sort_comp_distance(vptr u, vptr v, int a, int b)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	int da, db, kx, ky;

	/* Absolute distance components */
	kx = x[a]; kx -= px; kx = ABS(kx);
	ky = y[a]; ky -= py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Absolute distance components */
	kx = x[b]; kx -= px; kx = ABS(kx);
	ky = y[b]; ky -= py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Compare the distances */
	return (da <= db);
}


/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
static void ang_sort_swap_distance(vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	byte temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;
}



/*
 * Hack -- help "select" a location (see below)
 */
static s16b target_pick(int y1, int x1, int dy, int dx)
{
	int i, v;

	int x2, y2, x3, y3, x4, y4;

	int b_i = -1, b_v = 9999;


	/* Scan the locations */
	for (i = 0; i < temp_n; i++)
	{
		/* Point 2 */
		x2 = temp_x[i];
		y2 = temp_y[i];

		/* Directed distance */
		x3 = (x2 - x1);
		y3 = (y2 - y1);

		/* Verify quadrant */
		if (dx && (x3 * dx <= 0)) continue;
		if (dy && (y3 * dy <= 0)) continue;

		/* Absolute distance */
		x4 = ABS(x3);
		y4 = ABS(y3);

		/* Verify quadrant */
		if (dy && !dx && (x4 > y4)) continue;
		if (dx && !dy && (y4 > x4)) continue;

		/* Approximate Double Distance */
		v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

		/* Penalize location XXX XXX XXX */

		/* Track best */
		if ((b_i >= 0) && (v >= b_v)) continue;

		/* Track best */
		b_i = i; b_v = v;
	}

	/* Result */
	return (b_i);
}


/*
 * Hack -- determine if a given location is "interesting"
 */
static bool target_set_interactive_accept(int y, int x)
{
	s16b this_o_idx, next_o_idx = 0;

	s16b this_region_piece, next_region_piece = 0;

	/* Paranoia -- out of bounds is never interesting */
	if (!in_bounds_fully(y,x)) return (FALSE);


	/* Player grids are always interesting */
	if (cave_m_idx[y][x] < 0) return (TRUE);


	/* Handle hallucination */
	if (p_ptr->timed[TMD_IMAGE]) return (FALSE);


	/* Visible monsters */
	if (cave_m_idx[y][x] > 0)
	{
		monster_type *m_ptr = &m_list[cave_m_idx[y][x]];

		/* Visible monsters */
		if (m_ptr->ml) return (TRUE);
	}

	/* Scan all regions in the grid */
	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];
		region_type *r_ptr = &region_list[rp_ptr->region];

		/* Get the next region */
		next_region_piece = rp_ptr->next_in_grid;

		/* Skip dead regions */
		if (!r_ptr->type) continue;

		/* Displaying region */
		if (((cheat_hear) || ((r_ptr->flags1 & (RE1_NOTICE)) != 0)) &&
				((play_info[y][x] & (PLAY_REGN | PLAY_SEEN)) != 0) &&
						((r_ptr->flags1 & (RE1_DISPLAY)) != 0)) return (TRUE);
	}

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized object */
		if (o_ptr->ident & (IDENT_MARKED)) return (TRUE);
	}

	/* Interesting memorized features */
	if ((play_info[y][x] & (PLAY_MARK)) && (f_info[cave_feat[y][x]].flags1 & (FF1_NOTICE)))
	{
		return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


/*
 * Prepare the "temp" array for "target_interactive_set"
 *
 * Return the number of target_able monsters in the set.
 */
static void target_set_interactive_prepare(int mode)
{
	int y, x;

	/* Reset "temp" array */
	temp_n = 0;

	/* Scan the current panel */
	for (y = p_ptr->wy; ((y < p_ptr->wy + SCREEN_HGT) && (temp_n<TEMP_MAX)); y++)
	{
		for (x = p_ptr->wx; ((x < p_ptr->wx + SCREEN_WID)&&(temp_n<TEMP_MAX)); x++)
		{
			/* Require "interesting" contents */
			if (!target_set_interactive_accept(y, x)) continue;

			/* Special mode */
			if (mode & (TARGET_KILL))
			{
				/* Must contain a monster */
				if (!(cave_m_idx[y][x] > 0)) continue;

				/* Must be a targettable monster */
				if (!target_able(cave_m_idx[y][x])) continue;

				/* Must not be an ally , unless TARGET_ALLY set as well */
				if (((mode & (TARGET_ALLY)) == 0) && ((m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY)) != 0)) continue;
			}

			/* Special mode */
			else if (mode & (TARGET_ALLY))
			{
				/* Must contain a monster */
				if (!(cave_m_idx[y][x] > 0)) continue;

				/* Must be an ally */
				if ((m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY)) == 0) continue;
			}

			/* If matching race, must match race */
			if (p_ptr->target_race)
			{
				/* Must contain a monster */
				if (!(cave_m_idx[y][x] > 0)) continue;

				/* Must match the monster */
				if (m_list[cave_m_idx[y][x]].r_idx != p_ptr->target_race) continue;
			}

			/* Save the location */
			temp_x[temp_n] = x;
			temp_y[temp_n] = y;
			temp_n++;

		}
	}

	/* Set the sort hooks */
	ang_sort_comp = ang_sort_comp_distance;
	ang_sort_swap = ang_sort_swap_distance;

	/* Sort the positions */
	ang_sort(temp_x, temp_y, temp_n);
}


/*
 * Examine a grid, return a keypress.
 *
 * The "mode" argument contains the "TARGET_LOOK" bit flag, which
 * indicates that the "space" key should scan through the contents
 * of the grid, instead of simply returning immediately.  This lets
 * the "look" command get complete information, without making the
 * "target" command annoying.
 *
 * The "info" argument contains the "commands" which should be shown
 * inside the "[xxx]" text.  This string must never be empty, or grids
 * containing monsters will be displayed with an extra comma.
 *
 * Note that if a monster is in the grid, we update both the monster
 * recall info and the health bar info to track that monster.
 *
 * This function correctly handles multiple objects per grid, and objects
 * and terrain features in the same grid, though the latter never happens.
 *
 * This function must handle blindness/hallucination.
 *
 * TODO: rewrite this from Vanilla, especially the floor list
 */
key_event target_set_interactive_aux(int y, int x, int *room, int mode, cptr info)
{
	s16b this_o_idx, next_o_idx = 0;

	s16b this_region_piece, next_region_piece = 0;

	cptr s1, s2, s3;

	bool boring;

	bool floored;

	int feat;

	key_event query;

	char out_val[160];

	/* Repeat forever */
	while (1)
	{
		/* Paranoia */
		query.key = ' ';

		/* Assume boring */
		boring = TRUE;

		/* Default */
		s1 = "You see ";
		s2 = "";
		s3 = "";

		/* Out of bounds */
		if (!in_bounds_fully(y, x))
		{
			cptr name = "permanent rock";

			/* Display a message */
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey_ex();

			/* Stop on everything but "return" */
			if ((query.key != '\n') && (query.key != '\r')) return (query);

			/* Repeat forever */
			continue;
		}

		/* The player */
		if (cave_m_idx[y][x] < 0)
		{
			/* Description */
			s1 = "You are ";

			/* Preposition */
			s2 = "on ";
		}


		/* Hack -- hallucination */
		if (p_ptr->timed[TMD_IMAGE])
		{
			cptr name = "something strange";

			/* Display a message */
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey_ex();

			/* Stop on everything but "return" */
			if ((query.key != '\n') && (query.key != '\r')) return (query);

			/* Repeat forever */
			continue;
		}

		/* Actual monsters */
		if (cave_m_idx[y][x] > 0)
		{
			monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];

			/* Visible */
			if (m_ptr->ml)
			{
				bool recall = FALSE;

				char m_name[80];

				/* Not boring */
				boring = FALSE;

				/* Get the monster name ("a goblin") */
				monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0x08);

				/* Hack -- track this monster race */
				monster_race_track(m_ptr->r_idx);

				/* Hack -- health bar for this monster */
				health_track(cave_m_idx[y][x]);

				/* Hack -- handle stuff */
				handle_stuff();

				/* Interact */
				while (1)
				{
					/* Recall */
					if (recall)
					{
						/* Save screen */
						screen_save();

						/* Recall on screen */
						screen_monster_look(cave_m_idx[y][x]);

						/* Hack -- Complete the prompt (again) */
						Term_addstr(-1, TERM_WHITE, format("  [r,s,%s]", info));

						/* Command */
						query = inkey_ex();

						/* Load screen */
						screen_load();
					}

					/* Normal */
					else
					{
						/* Describe, and prompt for recall */
						sprintf(out_val, "%s%s%s%s (%s) [r,s,%s]",
							s1, s2, s3, m_name, look_mon_desc(cave_m_idx[y][x]), info);
						prt(out_val, 0, 0);

						/* Place cursor */
						move_cursor_relative(y, x);

						/* Command */
						query = inkey_ex();
					}

					/* Select any monsters similar to this */
					if (query.key == 's')
					{
						if (p_ptr->target_race)
						{
							p_ptr->target_race = 0;
						}
						else
						{
							p_ptr->target_race = m_list[cave_m_idx[y][x]].r_idx;
						}
					}

					/* Normal commands */
					if (query.key != 'r') break;

					/* Toggle recall */
					recall = !recall;
				}

				/* Stop on everything but "return"/"space" */
				if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ')) break;

				/* Sometimes stop at "space" key */
				if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;

				/* Change the intro */
				s1 = "It is ";

				/* Hack -- take account of gender */
				if (r_ptr->flags1 & (RF1_FEMALE)) s1 = "She is ";
				else if (r_ptr->flags1 & (RF1_MALE)) s1 = "He is ";

				/* Use a preposition */
				s2 = "carrying ";

				/* Scan all objects being carried */
				for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
				{
					char o_name[80];

					object_type *o_ptr;

					bool recall = FALSE;

					/* Get the object */
					o_ptr = &o_list[this_o_idx];

					/* Get the next object */
					next_o_idx = o_ptr->next_o_idx;

					/* Obtain an object description */
					object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

					/* Interact */
					while (1)
					{
						/* Recall */
						if (recall)
						{
							/* Save screen */
							screen_save();

							/* Recall on screen */
							screen_object(o_ptr);

							/* Hack -- Complete the prompt (again) */
							Term_addstr(-1, TERM_WHITE, format("  [r,%s]", info));

							/* Command */
							query = inkey_ex();

							/* Load screen */
							screen_load();
						}

						/* Normal */
						else
						{
							/* Describe the object */
							sprintf(out_val, "%s%s%s%s [r,%s]", s1, s2, s3, o_name, info);
							prt(out_val, 0, 0);

							/* Place cursor */
							move_cursor_relative(y, x);

							/* Command */
							query = inkey_ex();
						}

						/* Normal commands */
						if (query.key != 'r') break;

						/* Toggle recall */
						recall = !recall;
					}

					/* Stop on everything but "return"/"space" */
					if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ')) break;

					/* Sometimes stop at "space" key */
					if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;

					/* Change the intro */
					s2 = "also carrying ";
				}

				/* Double break */
				if (this_o_idx) break;

				/* Use a preposition */
				if (m_ptr->mflag & (MFLAG_OVER))
				{
					s2 = "above ";
				}
				else
				{
					/* Use a preposition */
					s2 = "on ";
				}
			}
		}

		/* Regions */
		for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
		{
			region_piece_type *rp_ptr = &region_piece_list[this_region_piece];
			region_type *r_ptr = &region_list[rp_ptr->region];

			/* Get the next region */
			next_region_piece = rp_ptr->next_in_grid;

			/* Skip dead regions */
			if (!r_ptr->type) continue;

			/* Displaying region */
			if (((cheat_hear) || ((r_ptr->flags1 & (RE1_NOTICE)) != 0)) &&
					((play_info[y][x] & (PLAY_REGN | PLAY_SEEN)) != 0) &&
							((r_ptr->flags1 & (RE1_DISPLAY)) != 0))
			{
				bool recall = FALSE;

				/* Skip because there is an underlying trap */
				if ((r_ptr->flags1 & (RE1_HIT_TRAP)) && (f_info[cave_feat[y][x]].flags1 & (FF1_TRAP)))
				{
					continue;
				}

				/* Not boring */
				boring = FALSE;

				/* Bring region to 'top' */
				region_highlight(rp_ptr->region);

				/* And refresh */
				region_refresh(rp_ptr->region);

				/* Interact */
				while (1)
				{
					/* Recall */
					if (recall)
					{
						/* Save screen */
						screen_save();

						/* Recall on screen */
						screen_region(r_ptr);

						/* Hack -- Complete the prompt (again) */
						Term_addstr(-1, TERM_WHITE, format("  [r,s,%s]", info));

						/* Command */
						query = inkey_ex();

						/* Load screen */
						screen_load();
					}

					/* Normal */
					else
					{
						/* Describe the object */
						sprintf(out_val, "%s%s%s%s [r,s,%s]", s1, s2, s3,
								format("%s %s",is_a_vowel((region_name + region_info[r_ptr->type].name)[0]) ? "an" : "a",
										region_name + region_info[r_ptr->type].name), info);
						prt(out_val, 0, 0);

						/* Place cursor */
						move_cursor_relative(y, x);

						/* Command */
						query = inkey_ex();
					}

					/* Normal commands */
					if (query.key != 'r') break;

					/* Toggle recall */
					recall = !recall;
				}

				/* Stop on everything but "return"/"space" */
				if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ') && (query.key != 's')) break;

				/* Sometimes stop at "space" key */
				if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;

				/* Hide this region */
				if (query.key == 's')
				{
					r_ptr->flags1 &= ~(RE1_DISPLAY);

					region_refresh(rp_ptr->region);
				}

				/* Change the intro */
				s1 = "It is ";

				/* Preposition */
				s2 = "on ";

			}
		}

		/* Double break */
		if (this_region_piece) break;

		/* Assume not floored */
		floored = FALSE;

		{
			int floor_list[MAX_FLOOR_STACK];
			int floor_num;

			/* Scan for floor objects */
			floor_num = scan_floor(floor_list, MAX_FLOOR_STACK, y, x, 0x02 | 0x08);

			/* Actual pile */
			if (floor_num > 1)
			{
				/* Not boring */
				boring = FALSE;

				/* Floored */
				floored = TRUE;

				/* Describe */
				while (1)
				{
					/* Describe the pile */
					sprintf(out_val, "%s%s%sa pile of %d objects [r,%s]",
						s1, s2, s3, floor_num, info);
					prt(out_val, 0, 0);
					move_cursor_relative(y, x);
					query = inkey_ex();

					/* Display objects */
					if (query.key == 'r')
					{
						/* Save screen */
						screen_save();

						/* Display */
						show_floor(floor_list, floor_num, FALSE);

						/* Describe the pile */
						prt(out_val, 0, 0);
						query = inkey_ex();

						/* Load screen */
						screen_load();

						/* Continue on 'r' only */
						if (query.key == 'r') continue;
					}

					/* Done */
					break;
				}

				/* Stop on everything but "return"/"space" */
				if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ')) break;

				/* Sometimes stop at "space" key */
				if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;

				/* Change the intro */
				s1 = "It is ";

				/* Preposition */
				s2 = "on ";
			}
		}

		/* Scan all objects in the grid */
		for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Skip objects if floored */
			if (floored) continue;

			/* Describe it */
			if (o_ptr->ident & (IDENT_MARKED))
			{
				bool recall = FALSE;

				char o_name[80];

				/* Not boring */
				boring = FALSE;

				/* Obtain an object description */
				object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

				/* Interact */
				while (1)
				{
					/* Recall */
					if (recall)
					{
						/* Save screen */
						screen_save();

						/* Recall on screen */
						screen_object(o_ptr);

						/* Hack -- Complete the prompt (again) */
						Term_addstr(-1, TERM_WHITE, format("  [r,%s]", info));

						/* Command */
						query = inkey_ex();

						/* Load screen */
						screen_load();
					}

					/* Normal */
					else
					{
						/* Describe the object */
						sprintf(out_val, "%s%s%s%s [r,%s]", s1, s2, s3, o_name, info);
						prt(out_val, 0, 0);

						/* Place cursor */
						move_cursor_relative(y, x);

						/* Command */
						query = inkey_ex();
					}

					/* Normal commands */
					if (query.key != 'r') break;

					/* Toggle recall */
					recall = !recall;
				}

				/* Stop on everything but "return"/"space" */
				if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ')) break;

				/* Sometimes stop at "space" key */
				if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;

				/* Change the intro */
				s1 = "It is ";

				/* Plurals */
				if (o_ptr->number != 1) s1 = "They are ";

				/* Preposition */
				s2 = "on ";
			}
		}

		/* Double break */
		if (this_o_idx) break;


		/* Feature (apply "mimic") */
		feat = f_info[cave_feat[y][x]].mimic;

		/* Require knowledge about grid, or ability to see grid */
		if (!(play_info[y][x] & (PLAY_MARK)) && !player_can_see_bold(y,x))
		{
			/* Forget feature */
			feat = FEAT_NONE;
		}

		/* Terrain feature if needed */
		if (boring || (feat > FEAT_INVIS))
		{
			cptr name = f_name + f_info[feat].name;

			bool recall = FALSE;

			/* Hack -- handle unknown grids */
			if (feat == FEAT_NONE) name = "unknown grid";

			/* Pick a prefix */
			if ((*s2) && ((!(f_info[feat].flags1 & (FF1_MOVE)) && !(f_info[feat].flags3 & (FF3_EASY_CLIMB))) ||
			    !(f_info[feat].flags1 & (FF1_LOS)) ||
			    (f_info[feat].flags1 & (FF1_ENTER)) ||
			    (f_info[feat].flags2 & (FF2_SHALLOW)) ||
			    (f_info[feat].flags2 & (FF2_DEEP)) ||
			    (f_info[feat].flags2 & (FF2_FILLED)) ||
			    (f_info[feat].flags2 & (FF2_CHASM)) ||
			    (f_info[feat].flags2 & (FF2_HIDE_SNEAK)) ||
			    (f_info[feat].flags3 & (FF3_NEED_TREE)) ))
			{
				s2 = "in ";
			}

			else if (*s2)
			{
				s2 = "on ";
			}

			/* Pick a prefix */
			if ((f_info[feat].flags2 & (FF2_SHALLOW)) ||
			    (f_info[feat].flags2 & (FF2_DEEP)) ||
			    (f_info[feat].flags2 & (FF2_FILLED)) ||
			    (f_info[feat].flags3 & (FF3_GROUND)) )
			{
				s3 = "";
			}

			else
			{
				/* Pick proper indefinite article */
				s3 = (is_a_vowel(name[0]) ? "an " : "a ");
			}

			/* Hack -- already a 'the' prefix */
			if (prefix(name, "the ")) s3 = "";

			/* Hack -- special introduction for filled areas */
			if (*s2 && (f_info[feat].flags2 & (FF2_FILLED))) s2 = "surrounded by ";

			/* Hack -- special introduction for store doors */
			if (f_info[feat].flags1 & (FF1_ENTER))
			{
				s3 = "the entrance to the ";
			}


			/* Interact */
			while (1)
			{
				/* Recall */
				if (recall)
				{
					/* Save screen */
					screen_save();

					/* Recall on screen */
					screen_feature_roff(feat);

					/* Hack -- Complete the prompt (again) */
					Term_addstr(-1, TERM_WHITE, format("  [r,%s]", info));

					/* Command */
					query = inkey_ex();

					/* Load screen */
					screen_load();
				}

				/* Normal */
				else
				{

					/* Display a message */
					sprintf(out_val, "%s%s%s%s [r,%s]", s1, s2, s3, name, info);
					prt(out_val, 0, 0);
					move_cursor_relative(y, x);
					query = inkey_ex();
				}

				/* Normal commands */
				if (query.key != 'r') break;

				/* Toggle recall */
				recall = !recall;
			}


			/* Stop on everything but "return"/"space" */
			if ((query.key != '\n') && (query.key != '\r') && (query.key != ' ')) break;

			/* Change the intro */
			s1 = "It is ";
		}

		/* Room description if needed */
		if (((play_info[y][x] & (PLAY_MARK)) != 0) &&
			((cave_info[y][x] & (CAVE_ROOM)) != 0) &&
			(room_has_flag(y, x, ROOM_SEEN) != 0) &&
			(room_names) && (*room != dun_room[y/BLOCK_HGT][x/BLOCK_HGT]))
		{
			int i;
			bool edge = FALSE;
			bool recall = FALSE;

			/* Get room location */
			int by = y/BLOCK_HGT;
			int bx = x/BLOCK_HGT;

			char name[62];
			*room = dun_room[by][bx];

			/* Get the actual room description */
			get_room_desc(*room, name, sizeof(name), NULL, 0, NULL, 0);

			/* Always in rooms */
			s2 = "in ";

			/* Hack --- edges of rooms */
			for (i = 0;i<8;i++)
			{
				int yy = y+ddy[i];
				int xx = x+ddx[i];

				if (!(cave_info[yy][xx] & (CAVE_ROOM))) edge = TRUE;
			}

			if (edge) s2 = "outside ";

			/* Pick proper indefinite article */
			s3 = (is_a_vowel(name[0])) ? "an " : "a ";

			/* Interact */
			while (1)
			{
				/* Recall */
				if (recall)
				{
					/* Save screen */
					screen_save();

					/* Recall on screen */
					screen_room_info(*room);

					/* Hack -- Complete the prompt (again) */
					Term_addstr(-1, TERM_WHITE, format("  [r,%s]", info));

					/* Command */
					query = inkey_ex();

					/* Load screen */
					screen_load();
				}

				/* Normal */
				else
				{
					/* Describe, and prompt for recall */
					sprintf(out_val, "%s%s%s%s [r,%s]",
						s1, s2, s3, name, info);
						prt(out_val, 0, 0);

					/* Place cursor */
					move_cursor_relative(y, x);

					/* Command */
					query = inkey_ex();
				}

				/* Normal commands */
				if (query.key != 'r') break;

				/* Toggle recall */
				recall = !recall;
		       }
		}


		/* Stop on everything but "return" */
		if ((query.key != '\n') && (query.key != '\r')) break;
	}

	/* Keep going */
	return (query);
}


/*
 * Modify a 'boring' grid appearance based on the 'projectability'
 */
void modify_grid_boring_project(byte *a, char *c, int y, int x, byte cinfo, byte pinfo)
{
	(void)y;
	(void)x;
	(void)cinfo;

	/* Handle "blind" first*/
	if (p_ptr->timed[TMD_BLIND])
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "fire" grids */
	else if (pinfo & (PLAY_FIRE))
	{
		int i;

		/* Paranoia */
		if (in_bounds_fully(y, x))

		for (i = 0; i < target_path_n; i++)
		{
			int path_y = GRID_Y(target_path_g[i]);
			int path_x = GRID_X(target_path_g[i]);

			/* X, Y on projection path? */
			if ((path_y == y) && (path_x == x))
			{
				/* Mega-hack */
				if (*a & 0x80)
				{
					/* Use a brightly lit tile */
					if (arg_graphics == GRAPHICS_DAVID_GERVAIS)
						*c -= 1;
					else if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
						*c += 2;
				}
				else
				{
					/* Use "yellow" */
					*a = get_color(*a, ATTR_LITE, 1);
				}

				/* Important -- exit loop */
				break;
			}
		}

		/* Use normal */
	}

	/* Handle "dark" grids */
	else
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark tile" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}
}

/*
 * Modify an 'unseen' grid appearance based on  'projectability'
 */
void modify_grid_unseen_project(byte *a, char *c)
{
	/* Handle "blind" first */
	if (p_ptr->timed[TMD_BLIND])
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "dark" grids */
	else
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark tile" */
			*a = get_color(*a, ATTR_DARK, 1);
		}
	}
}

/*
 * Modify an 'interesting' grid appearance based on 'projectability'
 */
void modify_grid_interesting_project(byte *a, char *c, int y, int x, byte cinfo, byte pinfo)
{
	(void)cinfo;

	/* Handle "blind" and night time*/
	if (p_ptr->timed[TMD_BLIND])
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_BLIND, 1);
		}
	}

	/* Handle "fire" grids */
	else if (pinfo & (PLAY_FIRE))
	{
		int i;

		/* Paranoia */
		if (in_bounds_fully(y, x))

		for (i = 0; i < target_path_n; i++)
		{
			int path_y = GRID_Y(target_path_g[i]);
			int path_x = GRID_X(target_path_g[i]);

			/* X, Y on projection path? */
			if ((path_y == y) && (path_x == x))
			{
				/* Mega-hack */
				if (*a & 0x80)
				{
					/* Use a brightly lit tile */
					if (arg_graphics == GRAPHICS_DAVID_GERVAIS)
						*c -= 1;
					else if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
						*c += 2;
				}
				else
				{
					/* Use "yellow" */
					*a = get_color(*a, ATTR_LITE, 1);
				}

				/* Important -- exit loop */
				break;
			}
		}

		/* Use normal */
	}

	/* Handle "dark" grids */
	else
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark tile" */
			*a = get_color(*a, ATTR_DARK, 1);
		}
	}
}


/*
 * Handle "target" and "look".
 *
 * Note that this code can be called from "get_aim_dir()".
 *
 * Currently, when "flag" is true, that is, when
 * "interesting" grids are being used, and a directional key is used, we
 * only scroll by a single panel, in the direction requested, and check
 * for any interesting grids on that panel.  The "correct" solution would
 * actually involve scanning a larger set of grids, including ones in
 * panels which are adjacent to the one currently scanned, but this is
 * overkill for this function.  XXX XXX
 *
 * Hack -- targetting/observing an "outer border grid" may induce
 * problems, so this is not currently allowed.
 *
 * The player can use the direction keys to move among "interesting"
 * grids in a heuristic manner, or the "space", "+", and "-" keys to
 * move through the "interesting" grids in a sequential manner, or
 * can enter "location" mode, and use the direction keys to move one
 * grid at a time in any direction.  The "t" (set target) command will
 * only target a monster (as opposed to a location) if the monster is
 * target_able and the "interesting" mode is being used.
 *
 * The current grid is described using the "look" method above, and
 * a new command may be entered at any time, but note that if the
 * "TARGET_LOOK" bit flag is set (or if we are in "location" mode,
 * where "space" has no obvious meaning) then "space" will scan
 * through the description of the current grid until done, instead
 * of immediately jumping to the next "interesting" grid.  This
 * allows the "target" command to retain its old semantics.
 *
 * The "*", "+", and "-" keys may always be used to jump immediately
 * to the next (or previous) interesting grid, in the proper mode.
 *
 * The "return" key may always be used to scan through a complete
 * grid description (forever).
 *
 * This command will cancel any old target, even if used from
 * inside the "look" command.
 */
bool target_set_interactive(int mode, int range, int radius, u32b flg, byte arc, byte diameter_of_source)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, d, m, t, bd;

	int y = py;
	int x = px;

	int ty = y;
	int tx = x;

	bool done = FALSE;

	bool flag = TRUE;

	key_event query;

	char info[80];

	int room = -1;

	s16b this_region;

	/* Get the real range */
	if (!range) range = MAX_SIGHT;

	/* Always show beam if just a missile */
	if ((flg & (PROJECT_4WAY | PROJECT_4WAX | PROJECT_BOOM)) == 0) flg |= (PROJECT_BEAM);
	
	/* Cancel target */
	target_set_monster(0, 0);


	/* Cancel tracking */
	/* health_track(0); */


	/* Prepare the "temp" array */
	target_set_interactive_prepare(mode);

	/* Nothing in it. Clear race filter if set and try again. */
	if (!temp_n && (p_ptr->target_race))
	{
		p_ptr->target_race = 0;

		target_set_interactive_prepare(mode);
	}

	/* If targetting monsters, display projectable grids */
	if (mode & (TARGET_KILL))
	{
		/* Set path */
		target_path_n = 0;

		/* Use the projection hook functions */
		modify_grid_boring_hook = modify_grid_boring_project;
		modify_grid_unseen_hook = modify_grid_unseen_project;
		modify_grid_interesting_hook = modify_grid_interesting_project;

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Hack -- Window stuff */
		p_ptr->window |= (PW_OVERHEAD);

		/* Handle stuff */
		handle_stuff();
	}

	/* Start near the player */
	m = 0;

	/* Interact */
	while (!done)
	{
		/* Interesting grids */
		if (flag && temp_n)
		{
			ty = y = temp_y[m];
			tx = x = temp_x[m];

			/* Calculate the path */
			if (mode & (TARGET_KILL))
			{
				/* Get the projection grids */
				project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
							 flg | (PROJECT_CHCK), arc, diameter_of_source);

				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Hack -- Window stuff */
				p_ptr->window |= (PW_OVERHEAD);

				/* Handle stuff */
				handle_stuff();
			}

			/* Allow target */
			if ((cave_m_idx[y][x] > 0) && target_able(cave_m_idx[y][x]))
			{
				my_strcpy(info, "q,t,a,p,o,x,g,+,-,.,@,?,<dir>", sizeof (info));
			}

			/* Dis-allow target */
			else
			{
				my_strcpy(info, "q,a,p,o,x,g,+,-,.,@,?,<dir>", sizeof (info));
			}

			/* Describe and Prompt */
			query = target_set_interactive_aux(y, x, &room, mode, info);

			/* Cancel tracking */
			/* health_track(0); */

			/* Assume no "direction" */
			d = 0;

			/* Analyze */
			switch (query.key)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case ' ':
				case '*':
				case '+':
				{
					if (++m == temp_n)
					{
						m = 0;
					}
					break;
				}

				case '-':
				{
					if (m-- == 0)
					{
						m = temp_n - 1;
					}
					break;
				}

				case 'p':
				{
					y = py;
					x = px;

					/* Calculate the path */
					if (mode & (TARGET_KILL))
					{
						/* Get the projection grids */
						project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
									 flg | (PROJECT_CHCK), arc, diameter_of_source);

						/* Redraw map */
						p_ptr->redraw |= (PR_MAP);

						/* Hack -- Window stuff */
						p_ptr->window |= (PW_OVERHEAD);

						/* Handle stuff */
						handle_stuff();
					}

					/* Recenter around player */
					verify_panel();

					/* Handle stuff */
					handle_stuff();
				}

				case 'o':
				{
					flag = FALSE;
					break;
				}

				case 'm':
				{
					break;
				}

				case '!':
				case '\xff':
				{
					/* Bounds check */
					if (in_bounds(KEY_GRID_Y(query), KEY_GRID_X(query)))
					{
						ty = y = KEY_GRID_Y(query);
						tx = x = KEY_GRID_X(query);

						/* Go to if clicked with BUTTON_MOVE */
						if (query.mousebutton == BUTTON_MOVE)
						{
							do_cmd_pathfind(y,x);
							done = TRUE;
						}
						/* Set target if clicked. ! is mainly for macro use. */
						else if ((query.mousebutton == BUTTON_AIM) || (query.key == '!'))
						{
							target_set_location(y, x, mode);
							done = TRUE;
						}
						else
						{
							flag = FALSE;

							/* Calculate the path */
							if (mode & (TARGET_KILL))
							{
								/* Get the projection grids */
								project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
											 flg | (PROJECT_CHCK), arc, diameter_of_source);

								/* Redraw map */
								p_ptr->redraw |= (PR_MAP);

								/* Hack -- Window stuff */
								p_ptr->window |= (PW_OVERHEAD);

								/* Handle stuff */
								handle_stuff();

								/* Force an update */
								Term_fresh();
							}
						}
					}
					break;
				}

				case 'g':
				{
					do_cmd_pathfind(y,x);
					done = TRUE;
					break;
				}

				case '?':
				{
					screen_save();

					/* Help file depends on mode */
					if (mode & (TARGET_KILL))
					{
						(void)show_file("cmdkill.txt", NULL, 0, 0);
					}
					else
					{
						(void)show_file("cmdlook.txt", NULL, 0, 0);
					}

					screen_load();
					break;
				}

				/* Get allies to attack anything near here */
				case 'a':
				{
					target_set_location(y, x, mode | (TARGET_NEAR));
					done = TRUE;
					break;
				}
				
				/* Toggle targetting of allies in kill mode */
				case 'x':
				{
					if (mode & (TARGET_KILL))
					{
						if (mode & (TARGET_ALLY)) mode &= ~(TARGET_ALLY);
						else mode |= (TARGET_ALLY);
						
						/* Forget */
						temp_n = 0;

						/* Re-prepare targets */
						target_set_interactive_prepare(mode);

						/* Start near the player again and fake movement */
						m = 0;
						d = 5;
					}
					
					break;
				}

				case '.':
				case 't':
				case '5':
				case '0':
				case '@':
				{
					int m_idx = cave_m_idx[y][x];

					if ((m_idx > 0) && target_able(m_idx))
					{
						health_track(m_idx);
						target_set_monster(m_idx, mode);
						done = TRUE;
					}
					else
					{
						bell("Illegal target!");
					}
					break;
				}

				/* Have set or changed race filter */
				case 's':
				{
					/* Forget */
					temp_n = 0;

					/* Re-prepare targets */
					target_set_interactive_prepare(mode);

					/* Start near the player again and fake movement */
					m = 0;
					d = 5;

					break;
				}

				default:
				{
					/* Extract direction */
					d = target_dir(query.key);

					/* Oops */
					if (!d) bell("Illegal command for target mode!");

					break;
				}
			}

			/* Hack -- move around */
			if (d)
			{
				int old_y = temp_y[m];
				int old_x = temp_x[m];

				/* Find a new monster */
				i = target_pick(old_y, old_x, ddy[d], ddx[d]);

				/* Scroll to find interesting grid */
				if (i < 0)
				{
					int old_wy = p_ptr->wy;
					int old_wx = p_ptr->wx;

					/* Change if legal */
					if (change_panel(d))
					{
						/* Recalculate interesting grids */
						target_set_interactive_prepare(mode);

						/* Find a new monster */
						i = target_pick(old_y, old_x, ddy[d], ddx[d]);

						/* Restore panel if needed */
						if ((i < 0) && modify_panel(old_wy, old_wx))
						{

							/* Recalculate interesting grids */
							target_set_interactive_prepare(mode);
						}

						/* Handle stuff */
						handle_stuff();
					}
				}

				/* Use interesting grid if found */
				if (i >= 0) m = i;
			}
		}

		/* Arbitrary grids */
		else
		{
			/* Default prompt */
			my_strcpy(info, "q,t,a,p,m,+,-,@,?,<dir>", sizeof (info));

			/* Describe and Prompt (enable "TARGET_LOOK") */
			query = target_set_interactive_aux(y, x, &room, mode | TARGET_LOOK, info);

			/* Cancel tracking */
			/* health_track(0); */

			/* Assume no direction */
			d = 0;

			/* Analyze the keypress */
			switch (query.key)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case ' ':
				case '*':
				case '+':
				case '-':
				{
					break;
				}

				case 'p':
				{
					y = py;
					x = px;

					/* Calculate the path */
					if (mode & (TARGET_KILL))
					{
						/* Get the projection grids */
						project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
									 flg | (PROJECT_CHCK), arc, diameter_of_source);

						/* Redraw map */
						p_ptr->redraw |= (PR_MAP);

						/* Hack -- Window stuff */
						p_ptr->window |= (PW_OVERHEAD);

						/* Handle stuff */
						handle_stuff();
					}

					/* Recenter around player */
					verify_panel();

					/* Handle stuff */
					handle_stuff();
				}

				case 'o':
				{
					break;
				}

				case 'm':
				{
					flag = TRUE;

					m = 0;
					bd = 999;

					/* Pick a nearby monster */
					for (i = 0; i < temp_n; i++)
					{
						t = distance(y, x, temp_y[i], temp_x[i]);

						/* Pick closest */
						if (t < bd)
						{
							m = i;
							bd = t;
						}
					}

					/* Nothing interesting */
					if (bd == 999) flag = FALSE;

					break;
				}

				case '!':
				case '\xff':
				{
					/* Bounds check */
					if (in_bounds(KEY_GRID_Y(query), KEY_GRID_X(query)))
					{
						ty = y = KEY_GRID_Y(query);
						tx = x = KEY_GRID_X(query);

						/* Go to if clicked with BUTTON_MOVE */
						if (query.mousebutton == BUTTON_MOVE)
						{
							do_cmd_pathfind(y,x);
							done = TRUE;
						}
						/* Set target if clicked. ! is mainly for macro use. */
						else if ((query.mousebutton == BUTTON_AIM) || (query.key == '!'))
						{
							target_set_location(y, x, mode);
							done = TRUE;
						}
						else
						{
							/* Calculate the path */
							if (mode & (TARGET_KILL))
							{
								/* Get the projection grids */
								project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
											 flg | (PROJECT_CHCK), arc, diameter_of_source);

								/* Redraw map */
								p_ptr->redraw |= (PR_MAP);

								/* Hack -- Window stuff */
								p_ptr->window |= (PW_OVERHEAD);

								/* Handle stuff */
								handle_stuff();

								/* Force an update */
								Term_fresh();
							}
						}
					}
					break;
				}

				case 'g':
				{
					do_cmd_pathfind(y,x);
					done = TRUE;
					break;
				}

				case '?':
				{
					screen_save();
					(void)show_file("target.txt", NULL, 0, 0);
					screen_load();
					break;
				}

				case 'a':
				{
					target_set_location(y, x, mode | (TARGET_NEAR));
					done = TRUE;
					break;
				}

				/* Toggle targetting of allies in kill mode */
				case 'x':
				{
					if (mode & (TARGET_KILL))
					{
						if (mode & (TARGET_ALLY)) mode &= ~(TARGET_ALLY);
						else mode |= (TARGET_ALLY);
						
						/* Forget */
						temp_n = 0;

						/* Re-prepare targets */
						target_set_interactive_prepare(mode);

						/* Start near the player again and fake movement */
						m = 0;
						d = 5;
					}
					
					break;
				}

				case '.':
				case 't':
				case '5':
				case '0':
				{
					/* Prevent easy suicide */
					if ((p_ptr->py == y) && (p_ptr->px == x))
					{
						bell("Illegal target! Use @ to target yourself.");
						break;
					}
				}
				case '@':
				{
					target_set_location(y, x, mode);
					done = TRUE;
					break;
				}

				/* Have set or changed race filter */
				case 's':
				{
					/* Forget */
					temp_n = 0;

					/* Re-prepare targets */
					target_set_interactive_prepare(mode);

					/* Start near the player again and fake movement */
					m = 0;
					d = 5;

					break;
				}

				default:
				{
					/* Extract a direction */
					d = target_dir(query.key);

					/* Oops */
					if (!d) bell("Illegal command for target mode!");

					break;
				}
			}

			/* Handle "direction" */
			if (d)
			{
				/* Move */
				x += ddx[d];
				y += ddy[d];

				/* Slide into legality */
				if (x >= DUNGEON_WID - 1) x--;
				else if (x <= 0) x++;

				/* Slide into legality */
				if (y >= DUNGEON_HGT - 1) y--;
				else if (y <= 0) y++;

				/* Adjust panel if needed */
				if (adjust_panel(y, x))
				{
					/* Handle stuff */
					handle_stuff();

					/* Recalculate interesting grids */
					target_set_interactive_prepare(mode);
				}

				else
				{
					/* Slide into legality */
					if (x >= p_ptr->wx + SCREEN_WID) x--;
					else if (x < p_ptr->wx) x++;

					/* Slide into legality */
					if (y >= p_ptr->wy + SCREEN_HGT) y--;
					else if (y < p_ptr->wy) y++;
				}

				ty = y;
				tx = x;

				/* Calculate the path */
				if (mode & (TARGET_KILL))
				{
					/* Get the projection grids */
					project(-1, 0, radius, range, py, px, ty, tx, 0, GF_NOTHING,
								 flg | (PROJECT_CHCK), arc, diameter_of_source);

					/* Redraw map */
					p_ptr->redraw |= (PR_MAP);

					/* Hack -- Window stuff */
					p_ptr->window |= (PW_OVERHEAD);

					/* Handle stuff */
					handle_stuff();
				}
			}
		}
	}

	/* Forget */
	temp_n = 0;

	/* Clear the top line */
	prt("", 0, 0);

	/* Paranoia - reset the modify_grid functions */
	modify_grid_boring_hook = modify_grid_boring_view;
	modify_grid_unseen_hook = modify_grid_unseen_view;
	modify_grid_interesting_hook = modify_grid_interesting_view;

	if (mode & (TARGET_KILL))
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Hack -- Window stuff */
		p_ptr->window |= (PW_OVERHEAD);

		/* Handle stuff */
		handle_stuff();
	}

	/* Redisplay all regions */
	for (this_region = 0; this_region < region_max; this_region++)
	{
		region_list[this_region].flags1 |= (RE1_DISPLAY);
	}

	/* Refresh all regions */
	for (this_region = 0; this_region < region_max; this_region++)
	{
		region_refresh(this_region);
	}

	/* Recenter around player */
	verify_panel();

	/* Handle stuff */
	handle_stuff();

	/* Failure to set target */
	if (!p_ptr->target_set) return (FALSE);

	/* Success */
	return (TRUE);
}



/*
 * Get an "aiming direction" (1,2,3,4,6,7,8,9 or 5) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * The direction "5" is special, and means "use current target".
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Currently this function applies confusion directly.
 */
bool get_aim_dir(int *dp, int mode, int range, int radius, u32b flg, byte arc, byte diameter_of_source)
{
	int dir;

	key_event ke;

	cptr p;

#ifdef ALLOW_REPEAT

	if (repeat_pull(dp))
	{
		/* Verify */
		if (!(*dp == 5 && !target_okay()))
		{
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT */

	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = p_ptr->command_dir;

	/* Hack -- auto-target if requested */
	if (use_old_target && target_okay()) dir = 5;

	/* Ask until satisfied */
	while (!dir)
	{
		/* Choose a prompt */
		if (!target_okay())
		{
			p = "Direction ('*' to choose a target, Escape to cancel)? ";
		}
		else
		{
			p = "Direction ('5' for target, '*' to re-target, Escape to cancel)? ";
		}

		/* Get a command (or Cancel) */
		if (!get_com_ex(p, &ke)) break;

		/* Analyze */
		switch (ke.key)
		{
			/* Mouse aiming */
			case '\xff':
			{
				if (ke.mousebutton)
				{
					int y = KEY_GRID_Y(ke);
					int x = KEY_GRID_X(ke);

					if (!in_bounds_fully(y, x)) break;

					target_set_location(y, x, 0);
					dir = 5;
					break;
				}
				else continue;
			}

			/* Set new target, use target if legal */
			case '*':
			{
				if (target_set_interactive(mode, range, radius, flg, arc, diameter_of_source)) dir = 5;
				break;
			}

			/* Use current target, if set and legal */
			case 't':
			case '5':
			case '0':
			case '.':
			{
				if (target_okay()) dir = 5;
				break;
			}

			/* Possible direction */
			default:
			{
				dir = target_dir(ke.key);
				break;
			}
		}

		/* Error */
		if (!dir) bell("Illegal aim direction!");
	}

	/* No direction */
	if (!dir) return (FALSE);

	/* Save the direction */
	p_ptr->command_dir = dir;

	/* Check for confusion */
	if (p_ptr->timed[TMD_CONFUSED])
	{
		/* Random direction */
		dir = ddd[rand_int(8)];
	}

	/* Notice confusion */
	if (p_ptr->command_dir != dir)
	{
		/* Warn the user */
		msg_print("You are confused.");
	}

	/* Save direction */
	(*dp) = dir;

#ifdef ALLOW_REPEAT

	repeat_push(dir);

#endif /* ALLOW_REPEAT */

	/* A "valid" direction was entered */
	return (TRUE);
}


/*
 * Lets the player select a known monster by aiming.
 */
int get_monster_by_aim(int mode)
{
	int ty = p_ptr->py;
	int tx = p_ptr->px;
	int dir;
	
	/* Get direction */
	if (!get_aim_dir(&dir, mode, MAX_SIGHT, 0, 0, 0, 0))
	{
		return (FALSE);
	}

	/* Check for "target request" */
	if (dir == 5 && target_okay())
	{
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}
	else
	{
		while (in_bounds(ty, tx))
		{
			/* Predict the "target" location */
			ty += ddy[dir];
			tx += ddx[dir];
			
			/* Require projection */
			if (!cave_project_bold(ty, tx)) break;
			
			/* No moster */
			if (cave_m_idx[ty][tx] <= 0) continue;
			
			/* Not known */
			if (m_list[cave_m_idx[ty][tx]].ml) continue;
			
			/* Not allied if only accepting enemies */
			if (((m_list[cave_m_idx[ty][tx]].mflag & (MFLAG_ALLY)) != 0) && ((mode & (MFLAG_ALLY)) == 0)) continue;
			
			/* Not enemy if only accepting allies */
			if (((m_list[cave_m_idx[ty][tx]].mflag & (MFLAG_ALLY | MFLAG_TOWN)) == 0) && ((mode & (MFLAG_ALLY)) == 0)) continue;
			
			break;
		}
	}
	
	/* Not in bounds */
	if (!in_bounds(ty, tx)) return (FALSE);

	/* Require projection */
	if (!cave_project_bold(ty, tx)) return(FALSE);
	
	/* Monster has to be visible and known */
	if ((cave_m_idx[ty][tx] > 0) && (m_list[cave_m_idx[ty][tx]].ml)) return (cave_m_idx[ty][tx]);
	
	return (0);
}


/*
 * Lets the player select a known grid by aiming.
 */
bool get_grid_by_aim(int mode, int *ty, int *tx)
{
	int dir;
	
	/* Get direction */
	if (!get_aim_dir(&dir, mode, MAX_SIGHT, 0, 0, 0, 0))
	{
		return (FALSE);
	}

	/* Check for "target request" */
	if (dir == 5 && target_okay())
	{
		*tx = p_ptr->target_col;
		*ty = p_ptr->target_row;
	}
	else
	{
		*ty = p_ptr->py;
		*tx = p_ptr->px;
		
		while (in_bounds(*ty, *tx))
		{
			/* Predict the "target" location */
			int y = *ty + ddy[dir];
			int x = *tx + ddx[dir];
			
			/* Require projection */
			if (!cave_project_bold(y, x)) break;
			
			/* Advance */
			*ty = y;
			*tx = x;
			
			/* No moster */
			if (cave_m_idx[*ty][*tx] <= 0) continue;
			
			/* Not known */
			if (m_list[cave_m_idx[*ty][*tx]].ml) continue;
			
			/* Not allied if only accepting enemies */
			if (((m_list[cave_m_idx[*ty][*tx]].mflag & (MFLAG_ALLY)) != 0) && ((mode & (MFLAG_ALLY)) == 0)) continue;
			
			/* Not enemy if only accepting allies */
			if (((m_list[cave_m_idx[*ty][*tx]].mflag & (MFLAG_ALLY | MFLAG_TOWN)) == 0) && ((mode & (MFLAG_ALLY)) == 0)) continue;
			
			break;
		}
	}
	
	/* Not in bounds */
	if (!in_bounds(*ty, *tx)) return (FALSE);

	/* Require projection */
	if (!cave_project_bold(*ty, *tx)) return(FALSE);
	
	return (TRUE);
}


/*
 * Take an angle and returns a direction.
 */
int get_angle_to_dir(int angle)
{
	int dir;

	/* Convert angle to direction */
	if (angle < 15) dir = 6;
	else if (angle < 33) dir = 9;
	else if (angle < 59) dir = 8;
	else if (angle < 78) dir = 7;
	else if (angle < 104) dir = 4;
	else if (angle < 123) dir = 1;
	else if (angle < 149) dir = 2;
	else if (angle < 168) dir = 3;
	else dir = 6;

	return dir;
}


/*
 * Request a "movement" direction (1,2,3,4,6,7,8,9) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 */
bool get_rep_dir(int *dp)
{
	int dir;

	key_event ke;

	cptr p;

#ifdef ALLOW_REPEAT

	if (repeat_pull(dp))
	{
		return (TRUE);
	}

#endif /* ALLOW_REPEAT */

	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = p_ptr->command_dir;

	/* Get a direction */
	while (!dir)
	{
		/* Choose a prompt */
		p = "Direction (Escape to cancel)? ";

		/* Get a command (or Cancel) */
		if (!get_com_ex(p, &ke)) break;

		/* Check mouse coordinates */
		if (ke.key == '\xff')
		{
			if (ke.mousebutton)
			{
				int y = KEY_GRID_Y(ke);
				int x = KEY_GRID_X(ke);

				int angle;

				if (!in_bounds_fully(y, x)) break;

				/* Calculate approximate angle */
				angle = get_angle_to_target(p_ptr->py, p_ptr->px,y, x, 0);

				/* Convert angle to direction */
				dir = get_angle_to_dir(angle);
			}
			else continue;
		}

		/* Convert keypress into a direction */
		else dir = target_dir(ke.key);

		/* Oops */
		if (!dir) bell("Illegal repeatable direction!");
	}

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save desired direction */
	p_ptr->command_dir = dir;

	/* Save direction */
	(*dp) = dir;

#ifdef ALLOW_REPEAT

	repeat_push(dir);

#endif /* ALLOW_REPEAT */

	/* Success */
	return (TRUE);
}


/*
 * Apply confusion, if needed, to a direction
 *
 * Display a message and return TRUE if direction changes.
 */
bool confuse_dir(int *dp)
{
	int dir;

	/* Default */
	dir = (*dp);

	/* Apply "confusion" */
	if ((p_ptr->timed[TMD_CONFUSED]) && (p_ptr->timed[TMD_CONFUSED] > rand_int(33)))
	{
		/* Apply confusion XXX XXX XXX */
		if ((dir == 5) || (rand_int(100) < 75))
		{
			/* Random direction */
			dir = ddd[rand_int(8)];
		}
	}

	/* Notice confusion */
	if ((*dp) != dir)
	{
		/* Warn the user */
		msg_print("You are confused.");

		/* Save direction */
		(*dp) = dir;

		/* Confused */
		return (TRUE);
	}

	/* Not confused */
	return (FALSE);
}

int min_depth(int dungeon)
{
	town_type dun = t_info[dungeon];

	return (dun.zone[0].level);
}

int max_depth(int dungeon)
{
	town_type *t_ptr=&t_info[dungeon];
	dungeon_zone *zone = &t_ptr->zone[0];
	int i;

	/* Get the zone */
	for (i = 0;i<MAX_DUNGEON_ZONES;i++)
	{
		if ((i) && (t_ptr->zone[i].level <= t_ptr->zone[i-1].level)) break;

		zone = &t_info[dungeon].zone[i];
	}

	return (zone->level);
}

bool is_typical_town(int dungeon, int depth)
{
	dungeon_zone *zone;

	/* Get the zone */
	get_zone(&zone, dungeon, depth);

	return (level_flag & (LF1_SURFACE)
			&& !zone->fill
			&& zone->level <= 10
			&& t_info[dungeon].store[3]);
}

int town_depth(int dungeon)
{
	town_type *t_ptr=&t_info[dungeon];
	dungeon_zone *zone = &t_ptr->zone[0];
	int i;

	/* Get the zone */
	for (i = 0;i<MAX_DUNGEON_ZONES;i++)
	{
		zone = &t_info[dungeon].zone[i];
		if (!zone->fill) break;
	}

	return (zone->level);
}

void get_zone(dungeon_zone **zone_handle, int dungeon, int depth)
{
	town_type *t_ptr = &t_info[dungeon];
	dungeon_zone *zone = &t_ptr->zone[0];
	int i;

	/* Get the zone */
	for (i = 0;i<MAX_DUNGEON_ZONES;i++)
	{
		if ((i) && (t_ptr->zone[i].level <= t_ptr->zone[i-1].level)) break;

		if (t_ptr->zone[i].level > depth) break;

		zone = &t_ptr->zone[i];
	}

	*zone_handle = zone;

}


/*
 * Use scalar
 */
int scale_method(method_level_scalar_type scalar, int level)
{
	int result = scalar.base;

	/* Scale radius up */
	if (scalar.levels)
	{
		result += scalar.gain * level / scalar.levels;
	}

	return (result);
}
