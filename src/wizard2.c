/* File: wizard2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.6
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"

#ifdef ALLOW_DEBUG

/*
 * Output a long int in binary format.
 */
static void prt_binary(u32b flags, int row, int col)
{
	int i;
	u32b bitmask;

	/* Scan the flags */
	for (i = bitmask = 1; i <= 32; i++, bitmask *= 2)
	{
		/* Dump set bits */
		if (flags & bitmask)
		{
			Term_putch(col++, row, TERM_BLUE, '*');
		}

		/* Dump unset bits */
		else
		{
			Term_putch(col++, row, TERM_WHITE, '-');
		}
	}
}


/*
 * Hack -- Teleport to the target
 */
static void do_cmd_wiz_bamf(void)
{
	/* Must have a target */
	if (target_okay())
	{
		/* Teleport to the target */
		teleport_player_to(p_ptr->target_row, p_ptr->target_col);
	}
}



/*
 * Aux function for "do_cmd_wiz_change()"
 */
static void do_cmd_wiz_change_aux(void)
{
	int i;

	int tmp_int;

	long tmp_long;

	char tmp_val[160];

	char ppp[80];


	/* Query the stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Prompt */
		sprintf(ppp, "%s (3-118): ", stat_names[i]);

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->stat_max[i]);

		/* Query */
		if (!get_string(ppp, tmp_val, 4)) return;

		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Verify */
		if (tmp_int > 18+999) tmp_int = 18+999;
		else if (tmp_int < 3) tmp_int = 3;

		/* Save it */
		p_ptr->stat_cur[i] = p_ptr->stat_max[i] = tmp_int;
	}


	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->au));

	/* Query */
	if (!get_string("Gold: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->au = tmp_long;


	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->exp));

	/* Query */
	if (!get_string("Experience: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->exp = tmp_long;

	/* Update */
	check_experience();

	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->max_exp));

	/* Query */
	if (!get_string("Max Exp: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->max_exp = tmp_long;

	/* Update */
	check_experience();
}


/*
 * Change various "permanent" player variables.
 */
static void do_cmd_wiz_change(void)
{
	/* Interact */
	do_cmd_wiz_change_aux();

	/* Redraw everything */
	do_cmd_redraw();
}


/*
 * Wizard routines for creating objects and modifying them
 *
 * This has been rewritten to make the whole procedure
 * of debugging objects much easier and more comfortable.
 *
 * Here are the low-level functions
 *
 * - wiz_display_item()
 *     display an item's debug-info
 * - wiz_create_itemtype()
 *     specify tval and sval (type and subtype of object)
 * - wiz_tweak_item()
 *     specify pval, +AC, +tohit, +todam
 *     Note that the wizard can leave this function anytime,
 *     thus accepting the default-values for the remaining values.
 *     pval comes first now, since it is most important.
 * - wiz_reroll_item()
 *     apply some magic to the item or turn it into an artifact.
 * - wiz_roll_item()
 *     Get some statistics about the rarity of an item:
 *     We create a lot of fake items and see if they are of the
 *     same type (tval and sval), then we compare pval and +AC.
 *     If the fake-item is better or equal it is counted.
 *     Note that cursed items that are better or equal (absolute values)
 *     are counted, too.
 *     HINT: This is *very* useful for balancing the game!
 * - wiz_quantity_item()
 *     change the quantity of an item, but be sane about it.
 *
 * And now the high-level functions
 * - do_cmd_wiz_play()
 *     play with an existing object
 * - wiz_create_item()
 *     create a new object
 *
 * Note -- You do not have to specify "pval" and other item-properties
 * directly. Just apply magic until you are satisfied with the item.
 *
 * Note -- For some items (such as wands, staffs, some rings, etc), you
 * must apply magic, or you will get "broken" or "uncharged" objects.
 *
 * Note -- Redefining artifacts via "do_cmd_wiz_play()" may destroy
 * the artifact.  Be careful.
 *
 * Hack -- this function will allow you to create multiple artifacts.
 * This "feature" may induce crashes or other nasty effects.
 */


/*
 * Display an item's properties
 */
static void wiz_display_item(const object_type *o_ptr)
{
	int j = 0;

	u32b f1, f2, f3, f4;

	char buf[256];


	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Clear screen */
	Term_clear();

	/* Describe fully */
	object_desc_spoil(buf, sizeof(buf), o_ptr, TRUE, 3);
	prt(buf, 2, j);

	prt(format("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d",
	           o_ptr->k_idx, k_info[o_ptr->k_idx].level,
	           o_ptr->tval, o_ptr->sval), 4, j);

	prt(format("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d",
	           o_ptr->number, o_ptr->weight,
	           o_ptr->ac, o_ptr->dd, o_ptr->ds), 5, j);

	prt(format("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d",
	           o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), 6, j);

	prt(format("name1 = %-4d  name2 = %-4d  cost = %ld  charges = %-5d power = %-5d",
	           o_ptr->name1, o_ptr->name2, (long)object_value(o_ptr), o_ptr->charges, object_power(o_ptr)), 7, j);

	prt(format("ident = %04x  timeout = %-d   xtra1 = %-4d  xtra2 = %-4d",
	           o_ptr->ident, o_ptr->timeout, o_ptr->xtra1, o_ptr->xtra2), 8, j);

	prt("+------------FLAGS1------------+", 10, j);
	prt("AFFECT..........SLAY.......BRAND", 11, j);
	prt("                ae      x  paefc", 12, j);
	prt("siwdcc  ssidsasmnvudotgdd  oclio", 13, j);
	prt("tnieoh  trnipthgiinmrrnrr  iierl", 14, j);
	prt("rtsxna..lcfgdkttmldncltgg..sdced", 15, j);
	prt_binary(f1, 16, j);

	prt("+------------FLAGS2------------+", 17, j);
	prt("SUST........IMM.RESIST.........", 18, j);
	prt("            afecaefcpfldbc s n  ", 19, j);
	prt("siwdcc      cilocliooeialoshnecd", 20, j);
	prt("tnieoh      irelierliatrnnnrethi", 21, j);
	prt("rtsxna......decddcedsrekdfddxhss", 22, j);
	prt_binary(f2, 23, j);

	prt("+------------FLAGS3------------+", 10, j+32);
	prt("s   ts h     tadiiii   aiehs  hp", 11, j+32);
	prt("lf  eefo     egrgggg  bcnaih  vr", 12, j+32);
	prt("we  lerl    ilgannnn  ltssdo  ym", 13, j+32);
	prt("da reied    merirrrr  eityew ccc", 14, j+32);
	prt("itlepnel    ppanaefc  svaktm uuu", 15, j+32);
	prt("ghigavai    aoveclio  saanyo rrr", 16, j+32);
	prt("seteticf    craxierl  etropd sss", 17, j+32);
	prt("trenhste    tttpdced  detwes eee", 18, j+32);
	prt_binary(f3, 19, j+32);
}


/*
 * Get an object kind for creation (or zero)
 *
 * List up to 60 choices in three columns
 */
static int wiz_create_itemtype(void)
{
	int i, num, max_num;
	int col, row;
	int tval;

	cptr tval_desc;
	char ch;

	int choice[78];
	static const char choice_name[] = "abcdefghijklmnopqrstuvwxyz"
	                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	                                  "0123456789:;<=>?@%&*(){}[]";
	const char *cp;

	char buf[160];


	/* Clear screen */
	Term_clear();

	/* Print all tval's and their descriptions */
        for (num = 0; (num < 78) && object_group[num].tval; num++)
	{
		row = 2 + (num % 26);
		col = 30 * (num / 26);
		ch  = choice_name[num];
                prt(format("[%c] %s", ch, object_group[num].text), row, col);
	}

	/* We need to know the maximal possible tval_index */
	max_num = num;

	/* Choose! */
	if (!get_com("Get what type of object? ", &ch)) return (0);

	/* Analyze choice */
	num = -1;
	if ((cp = strchr(choice_name, ch)) != NULL)
		num = cp - choice_name;

	/* Bail out if choice is illegal */
	if ((num < 0) || (num >= max_num)) return (0);

	/* Base object type chosen, fill in tval */
        tval = object_group[num].tval;
        tval_desc = object_group[num].text;


	/*** And now we go for k_idx ***/

	/* Clear screen */
	Term_clear();

	/* We have to search the whole item list. */
	for (num = 0, i = 1; (num < 78) && (i < z_info->k_max); i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Analyze matching items */
		if (k_ptr->tval == tval)
		{
			/* Hack -- Skip instant artifacts */
			if (k_ptr->flags5 & (TR5_INSTA_ART)) continue;

			/* Prepare it */
			row = 2 + (num % 26);
			col = 30 * (num / 26);
			ch  = choice_name[num];

			/* Get the "name" of object "i" */
			strip_name(buf, sizeof(buf), i);

			/* Print it */
			prt(format("[%c] %s", ch, buf), row, col);

			/* Remember the object index */
			choice[num++] = i;
		}
	}

	/* Me need to know the maximal possible remembered object_index */
	max_num = num;

	/* Choose! */
	if (!get_com(format("What Kind of %s? ", tval_desc), &ch)) return (0);

	/* Analyze choice */
	num = -1;
	if ((cp = strchr(choice_name, ch)) != NULL)
		num = cp - choice_name;

	/* Bail out if choice is "illegal" */
	if ((num < 0) || (num >= max_num)) return (0);

	/* And return successful */
	return (choice[num]);
}


/*
 * Tweak an item
 */
static void wiz_tweak_item(object_type *o_ptr)
{
	cptr p;
	char tmp_val[80];


	/* Hack -- leave artifacts alone */
	if (artifact_p(o_ptr)) return;

	p = "Enter new 'pval' setting: ";
	sprintf(tmp_val, "%d", o_ptr->pval);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->pval = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_a' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_a);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_a = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_h' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_h);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_h = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_d' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_d);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_d = atoi(tmp_val);
	wiz_display_item(o_ptr);
}


/*
 * Apply magic to an item or turn it into an artifact. -Bernd-
 */
static void wiz_reroll_item(object_type *o_ptr)
{
	object_type *i_ptr;
	object_type object_type_body;

	char ch;

	bool changed = FALSE;


	/* Hack -- leave artifacts alone */
	if (artifact_p(o_ptr)) return;


	/* Get local object */
	i_ptr = &object_type_body;

	/* Copy the object */
	object_copy(i_ptr, o_ptr);


	/* Main loop. Ask for magification and artifactification */
	while (TRUE)
	{
		/* Display full item debug information */
		wiz_display_item(i_ptr);

		/* Ask wizard what to do. */
		if (!get_com("[a]ccept, [n]ormal, [g]ood, [e]xcellent? ", &ch))
			break;

		/* Create/change it! */
		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		/* Apply normal magic, but first clear object */
		else if (ch == 'n' || ch == 'N')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);
		}

		/* Apply good magic, but first clear object */
		else if (ch == 'g' || ch == 'G')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, FALSE);
		}

		/* Apply great magic, but first clear object */
		else if (ch == 'e' || ch == 'E')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, TRUE);
		}
	}


	/* Notice change */
	if (changed)
	{
		/* Mark as cheat */
		i_ptr->origin = ORIGIN_CHEAT;

		/* Restore the position information */
		i_ptr->iy = o_ptr->iy;
		i_ptr->ix = o_ptr->ix;
		i_ptr->next_o_idx = o_ptr->next_o_idx;
		i_ptr->ident = o_ptr->ident;

		/* Apply changes */
		object_copy(o_ptr, i_ptr);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
	}
}



/*
 * Maximum number of rolls
 */
#define TEST_ROLL 100000


/*
 * Try to create an item again. Output some statistics.    -Bernd-
 *
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(object_type *o_ptr)
{
	long i, matches, better, worse, other;

	char ch;
	char *quality;

	bool good, great;

	object_type *i_ptr;
	object_type object_type_body;

	cptr q = "Rolls: %ld, Matches: %ld, Better: %ld, Worse: %ld, Other: %ld";


	/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
	if (artifact_p(o_ptr)) a_info[o_ptr->name1].cur_num = 0;


	/* Interact */
	while (TRUE)
	{
		cptr pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";

		/* Display item */
		wiz_display_item(o_ptr);

		/* Get choices */
		if (!get_com(pmt, &ch)) break;

		if (ch == 'n' || ch == 'N')
		{
			good = FALSE;
			great = FALSE;
			quality = "normal";
		}
		else if (ch == 'g' || ch == 'G')
		{
			good = TRUE;
			great = FALSE;
			quality = "good";
		}
		else if (ch == 'e' || ch == 'E')
		{
			good = TRUE;
			great = TRUE;
			quality = "excellent";
		}
		else
		{
#if 0 /* unused */
			good = FALSE;
			great = FALSE;
#endif /* unused */
			break;
		}

		/* Let us know what we are doing */
		msg_format("Creating a lot of %s items. Base level = %d.",
		           quality, p_ptr->depth);
		message_flush();

		/* Set counters to zero */
		matches = better = worse = other = 0;

		/* Let's rock and roll */
		for (i = 0; i <= TEST_ROLL; i++)
		{
			/* Output every few rolls */
			if ((i < 100) || (i % 100 == 0))
			{
				/* Do not wait */
				inkey_scan = TRUE;

				/* Allow interupt */
				if (inkey())
				{
					/* Flush */
					flush();

					/* Stop rolling */
					break;
				}

				/* Dump the stats */
				prt(format(q, i, matches, better, worse, other), 0, 0);
				Term_fresh();
			}


			/* Get local object */
			i_ptr = &object_type_body;

			/* Wipe the object */
			object_wipe(i_ptr);

			/* Create an object */
			make_object(i_ptr, good, great);


			/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
			if (artifact_p(i_ptr)) a_info[i_ptr->name1].cur_num = 0;


			/* Test for the same tval and sval. */
			if ((o_ptr->tval) != (i_ptr->tval)) continue;
			if ((o_ptr->sval) != (i_ptr->sval)) continue;

			/* Check for match */
			if ((i_ptr->pval == o_ptr->pval) &&
			    (i_ptr->to_a == o_ptr->to_a) &&
			    (i_ptr->to_h == o_ptr->to_h) &&
			    (i_ptr->to_d == o_ptr->to_d))
			{
				matches++;
			}

			/* Check for better */
			else if ((i_ptr->pval >= o_ptr->pval) &&
			         (i_ptr->to_a >= o_ptr->to_a) &&
			         (i_ptr->to_h >= o_ptr->to_h) &&
			         (i_ptr->to_d >= o_ptr->to_d))
			{
				better++;
			}

			/* Check for worse */
			else if ((i_ptr->pval <= o_ptr->pval) &&
			         (i_ptr->to_a <= o_ptr->to_a) &&
			         (i_ptr->to_h <= o_ptr->to_h) &&
			         (i_ptr->to_d <= o_ptr->to_d))
			{
				worse++;
			}

			/* Assume different */
			else
			{
				other++;
			}
		}

		/* Final dump */
		msg_format(q, i, matches, better, worse, other);
		message_flush();
	}


	/* Hack -- Normally only make a single artifact */
	if (artifact_p(o_ptr)) a_info[o_ptr->name1].cur_num = 1;
}


/*
 * Change the quantity of a the item
 */
static void wiz_quantity_item(object_type *o_ptr, bool carried)
{
	int tmp_int;

	char tmp_val[3];


	/* Never duplicate artifacts */
	if (artifact_p(o_ptr)) return;


	/* Default */
	sprintf(tmp_val, "%d", o_ptr->number);

	/* Query */
	if (get_string("Quantity: ", tmp_val, 3))
	{
		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Paranoia */
		if (tmp_int < 1) tmp_int = 1;
		if (tmp_int > 99) tmp_int = 99;

		/* Adjust total weight being carried */
		if (carried)
		{
			/* Remove the weight of the old number of objects */
			p_ptr->total_weight -= (o_ptr->number * o_ptr->weight);

			/* Add the weight of the new number of objects */
			p_ptr->total_weight += (tmp_int * o_ptr->weight);
		}

		/* Accept modifications */
		o_ptr->number = tmp_int;
	}
}



/*
 * Play with an item. Options include:
 *   - Output statistics (via wiz_roll_item)
 *   - Reroll item (via wiz_reroll_item)
 *   - Change properties (via wiz_tweak_item)
 *   - Change the number of items (via wiz_quantity_item)
 */
static void do_cmd_wiz_play(void)
{
	int item;

	object_type *i_ptr;
	object_type object_type_body;

	object_type *o_ptr;

	char ch;

	cptr q, s;

	bool changed = FALSE;


	/* Get an item */
	q = "Play with which object? ";
	s = "You have nothing to play with.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* In a bag? */
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return;

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Save screen */
	screen_save();


	/* Get local object */
	i_ptr = &object_type_body;

	/* Copy object */
	object_copy(i_ptr, o_ptr);


	/* The main loop */
	while (TRUE)
	{
		/* Display the item */
		wiz_display_item(i_ptr);

		/* Get choice */
		if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch))
			break;

		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		if (ch == 's' || ch == 'S')
		{
			wiz_statistics(i_ptr);
		}

		if (ch == 'r' || ch == 'R')
		{
			wiz_reroll_item(i_ptr);
		}

		if (ch == 't' || ch == 'T')
		{
			wiz_tweak_item(i_ptr);
		}

		if (ch == 'q' || ch == 'Q')
		{
			bool carried = (item >= 0) ? TRUE : FALSE;
			wiz_quantity_item(i_ptr, carried);
		}
	}


	/* Load screen */
	screen_load();


	/* Accept change */
	if (changed)
	{
		/* Message */
		msg_print("Changes accepted.");

		/* Change */
		object_copy(o_ptr, i_ptr);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
	}

	/* Ignore change */
	else
	{
		msg_print("Changes ignored.");
	}
}


/*
 * Wizard routine for creating objects
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items. XXX XXX XXX
 */
static void wiz_create_item(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	object_type *i_ptr;
	object_type object_type_body;

	int k_idx;


	/* Save screen */
	screen_save();

	/* Get object base type */
	k_idx = wiz_create_itemtype();

	/* Load screen */
	screen_load();


	/* Return if failed */
	if (!k_idx) return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Create the item */
	object_prep(i_ptr, k_idx);

	/* Apply magic (no messages, no artifacts) */
	apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);

	/* Mark as cheat */
	i_ptr->origin = ORIGIN_CHEAT;

	/* Apply obvious flags */
	object_obvious_flags(i_ptr, TRUE);

	/* Hack -- use repeat count to specify quantity */
	if (p_ptr->command_arg) i_ptr->number = p_ptr->command_arg;

	/* Drop the object from heaven */
	drop_near(i_ptr, -1, py, px, TRUE);

	/* All done */
	msg_print("Allocated.");
}


/*
 * Create the artifact with the specified number
 */
static void wiz_create_artifact(int a_idx)
{
	object_type *i_ptr;
	object_type object_type_body;
	int k_idx;

	artifact_type *a_ptr = &a_info[a_idx];

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Acquire the "kind" index */
	k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!k_idx) return;

	/* Create the artifact */
	object_prep(i_ptr, k_idx);

	/* Save the name */
	i_ptr->name1 = a_idx;

	/* Extract the fields */
	i_ptr->pval = a_ptr->pval;
	i_ptr->ac = a_ptr->ac;
	i_ptr->dd = a_ptr->dd;
	i_ptr->ds = a_ptr->ds;
	i_ptr->to_a = a_ptr->to_a;
	i_ptr->to_h = a_ptr->to_h;
	i_ptr->to_d = a_ptr->to_d;
	i_ptr->weight = a_ptr->weight;

	/* Mark as cheat */
	i_ptr->origin = ORIGIN_CHEAT;

	/* Drop the artifact from heaven */
	drop_near(i_ptr, -1, p_ptr->py, p_ptr->px, TRUE);

	/* All done */
	msg_print("Allocated.");
}


/*
 * Cure everything instantly
 */
static void do_cmd_wiz_cure_all(void)
{
	int i;

	/* Remove curses */
	(void)remove_all_curse();

	/* Restore stats */
	(void)res_stat(A_STR);
	(void)res_stat(A_INT);
	(void)res_stat(A_WIS);
	(void)res_stat(A_DEX);
	(void)res_stat(A_CON);
	(void)res_stat(A_CHR);
	(void)res_stat(A_AGI);
	(void)res_stat(A_SIZ);

	/* Restore the level */
	(void)restore_level();

	/* Remove diseases */
	p_ptr->disease = 0;

	/* Remove all timed effects */
	for (i = 0; i < TMD_MAX; i++)
	{
		set_timed(i, 0, TRUE);
	}

	/* Heal the player */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;
	
	/* Restore mana */
	p_ptr->csp = p_ptr->msp;
	p_ptr->csp_frac = 0;
	
	/* No longer hungry */
	(void)set_food(PY_FOOD_MAX - 1);

	/* No longer tired */
	(void)set_rest(PY_REST_MAX - 1);

	/* Redraw everything */
	do_cmd_redraw();
}


/*
 * Go to any level
 */
/*
 * Go to any level
 */
static void do_cmd_wiz_jump(void)
{
        /* Ask for a town */
        if (adult_campaign)
        {

                /* Ask for level */
                if (p_ptr->command_arg <= 0)
                {
                        char ppp[80];

                        char tmp_val[160];

                        /* Prompt */
                        sprintf(ppp, "Jump to dungeon (0-%d): ", z_info->t_max-1);

                        /* Default */
                        sprintf(tmp_val, "%d", p_ptr->dungeon);

                        /* Ask for a level */
                        if (!get_string(ppp, tmp_val, 10)) return;

                        /* Extract request */
                        p_ptr->command_arg = atoi(tmp_val);
                }

                /* Paranoia */
                if (p_ptr->command_arg < 0) p_ptr->command_arg = 0;

                /* Paranoia */
                if (p_ptr->command_arg >= z_info->t_max) p_ptr->command_arg = z_info->t_max -1;

                /* Accept request */
                msg_format("You jump to %s.", t_name + t_info[p_ptr->command_arg].name);

                /* New depth */
                p_ptr->dungeon = p_ptr->command_arg;

                p_ptr->command_arg = 0;

        }

	/* Ask for level */
	if (p_ptr->command_arg <= 0)
	{
		char ppp[80];

		char tmp_val[160];

		/* Prompt */
		sprintf(ppp, "Jump to level (%d-%d): ", min_depth(p_ptr->dungeon), max_depth(p_ptr->dungeon));

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->depth);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 10)) return;

		/* Extract request */
		p_ptr->command_arg = atoi(tmp_val);
	}

	/* Save the old dungeon in case something goes wrong */
	if (autosave_backup)
		do_cmd_save_bkp();

	if (p_ptr->command_arg < min_depth(p_ptr->dungeon))
		p_ptr->command_arg = min_depth(p_ptr->dungeon);
	if (p_ptr->command_arg > max_depth(p_ptr->dungeon))
		p_ptr->command_arg = max_depth(p_ptr->dungeon);

	/* Accept request */
	msg_format("You jump to dungeon level %d.", p_ptr->command_arg);

	/* Clear stairs */
	p_ptr->create_stair = 0;

	/* New depth */
	p_ptr->depth = p_ptr->command_arg;

	/* Leaving */
	p_ptr->leaving = TRUE;
}


/*
 * Become aware of a lot of objects
 */
static void do_cmd_wiz_learn(void)
{
	int i;

	object_type *i_ptr;
	object_type object_type_body;

	/* Scan every object */
	for (i = 1; i < z_info->k_max; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Induce awareness */
		if (k_ptr->level <= p_ptr->command_arg)
		{
			/* Get local object */
			i_ptr = &object_type_body;

			/* Prepare object */
			object_prep(i_ptr, i);

			/* Awareness */
			object_aware(i_ptr, TRUE);
		}
	}
}


/*
 * Hack -- Rerate Hitpoints
 */
static void do_cmd_rerate(void)
{
	int min_value, max_value, i, percent;

	min_value = (PY_MAX_LEVEL * 3 * 9) / 8;
	min_value += PY_MAX_LEVEL;

	max_value = (PY_MAX_LEVEL * 5 * 9) / 8;
	max_value += PY_MAX_LEVEL;

	p_ptr->player_hp[0] = 10;

	/* Rerate */
	while (1)
	{
		/* Collect values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			p_ptr->player_hp[i] = (s16b)randint(10);
			p_ptr->player_hp[i] += p_ptr->player_hp[i - 1];
		}

		/* Legal values */
		if ((p_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) &&
		    (p_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value)) break;
	}

	percent = (int)(((long)p_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
	                (10 + ((PY_MAX_LEVEL - 1) * 10)));

	/* Update and redraw hitpoints */
	p_ptr->update |= (PU_HP);
	p_ptr->redraw |= (PR_HP);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

	/* Handle stuff */
	handle_stuff();

	/* Message */
	msg_format("Current Life Rating is %d/100.", percent);
}


/*
 * Summon some creatures
 */
static void do_cmd_wiz_summon(int num)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i;

	for (i = 0; i < num; i++)
	{
		(void)summon_specific(py, px, 0, p_ptr->depth, 0, TRUE, 0L);
	}
}


/*
 * Summon a creature of the specified type
 *
 * This function is rather dangerous XXX XXX XXX
 */
static void do_cmd_wiz_named(int r_idx, bool slp)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, x, y;

	/* Paranoia */
	if (!r_idx) return;
	if (r_idx >= z_info->r_max-1) return;

	/* Try 10 times */
	for (i = 0; i < 10; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(&y, &x, py, px, d, 0);

		/* Require empty grids */
		if (!cave_empty_bold(y, x)) continue;

		/* Place it (allow groups) */
		if (place_monster_aux(y, x, r_idx, slp, TRUE, 0L)) break;
	}
}



/*
 * Hack -- Delete all nearby monsters
 */
static void do_cmd_wiz_zap(int d)
{
	int i;

	/* Banishment everyone nearby */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d) continue;

		/* Delete the monster */
		delete_monster_idx(i);
	}

	/* Update monster list window */
	p_ptr->window |= PW_MONLIST;
}


/*
 * Un-hide all monsters
 */
static void do_cmd_wiz_unhide(int d)
{
	int i;

	/* Process monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d) continue;

		/* Optimize -- Repair flags */
		repair_mflag_mark = repair_mflag_show = TRUE;

		/* Detect the monster */
		m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}
}


/*
 * Reveal all of dungeon
 */
static void do_cmd_wiz_detect(void)
{
	int i, y, x;


	/* Memorize objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Memorize */
		o_ptr->ident |= (IDENT_MARKED);
	}

	/* Scan all normal grids */
	for (y = 1; y < DUNGEON_HGT-1; y++)
	{
		/* Scan all normal grids */
		for (x = 1; x < DUNGEON_WID-1; x++)
		{
			/* Process all non-basic granite */
			if (cave_feat[y][x] != 56)
			{
				/* Perma-lite the grid */
				cave_info[y][x] |= (CAVE_GLOW);

				/* Memorize normal features */
				if (f_info[cave_feat[y][x]].flags1 & (FF1_REMEMBER))
				{
					/* Memorize the grid */
					play_info[y][x] |= (PLAY_MARK);
				}

				/* Normally, memorize floors (see above) */
				if (view_perma_grids && !view_torch_grids)
				{
					/* Memorize the grid */
					play_info[y][x] |= (PLAY_MARK);
				}
			}
		}
	}

	/* Process monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Optimize -- Repair flags */
		repair_mflag_mark = repair_mflag_show = TRUE;

		/* Detect the monster */
		m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}


	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP | PW_MONLIST);
}


/*
 * Query the dungeon
 */
static void do_cmd_wiz_query(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x;

	char cmd = '\0';

	u16b mask = 0x00;
	bool play_mask = FALSE;

	while (!cmd)
	{

		/* Get a "debug command" */
		if (!get_com(format("Debug Command Query (%s): ", play_mask ? "player" : "cave" ), &cmd)) return;

		/* Extract a flag */
		switch (cmd)
		{
			case '0': mask = (1 << 0); break;
			case '1': mask = (1 << 1); break;
			case '2': mask = (1 << 2); break;
			case '3': mask = (1 << 3); break;
			case '4': mask = (1 << 4); break;
			case '5': mask = (1 << 5); break;
			case '6': mask = (1 << 6); break;
			case '7': mask = (1 << 7); break;
		}

		if (play_mask) switch (cmd)
		{
			case 'm': mask |= (PLAY_MARK); break;
			case 'd': mask |= (PLAY_SAFE); break;
      	    case 'p': mask |= (PLAY_REGN); break;
			case 'l': mask |= (PLAY_LITE); break;
			case 's': mask |= (PLAY_SEEN); break;
			case 't': mask |= (PLAY_TEMP); break;
			case 'v': mask |= (PLAY_VIEW); break;
			case 'f': mask |= (PLAY_FIRE); break;
			case 'c': play_mask = FALSE; cmd = '\0'; break;
		}
		else switch (cmd)
		{
			case 'g': mask |= (CAVE_GLOW); break;
			case 'r': mask |= (CAVE_ROOM); break;
			case 'd': mask |= (CAVE_DLIT); break;
			case 'l': mask |= (CAVE_HALO); break;
			case 't': mask |= (CAVE_TLIT); break;
			case 'x': mask |= (CAVE_CLIM); break;
			case 'f': mask |= (CAVE_XLOF); break;
			case 's': mask |= (CAVE_XLOS); break;
			case 'p': play_mask = TRUE; cmd = '\0'; break;
		}
	}

	/* Scan map */
	for (y = p_ptr->wy; y < p_ptr->wy + SCREEN_HGT; y++)
	{
		for (x = p_ptr->wx; x < p_ptr->wx + SCREEN_WID; x++)
		{
			byte a = TERM_RED;

			if (!in_bounds_fully(y, x)) continue;

			/* Given mask, show only those grids */
			if (mask && play_mask &&!(play_info[y][x] & mask)) continue;
			else if (mask && !play_mask && !(cave_info[y][x] & mask)) continue;

			/* Given no mask, show unknown grids */
			if (!mask && (play_info[y][x] & (PLAY_MARK))) continue;

			/* Color */
			if (cave_floor_bold(y, x)) a = TERM_YELLOW;

			/* Display player/floors/walls */
			if ((y == py) && (x == px))
			{
				print_rel('@', a, y, x);
			}
			else if (cave_floor_bold(y, x))
			{
				print_rel('*', a, y, x);
			}
			else
			{
				print_rel('#', a, y, x);
			}
		}
	}

	/* Get keypress */
	msg_print("Press any key.");
	anykey();

	/* Redraw map */
	prt_map();
}



void do_cmd_wiz_ecology(void)
{
	int num, row, col, i;

	/* No ecology */
	if (!cave_ecology.get_mon)
	{
		msg_print("No ecology on this level.");

		return;
	}

	/* Save screen */
	screen_save();

	/* Print all members of the ecology and their descriptions */
        for (num = 0; num < cave_ecology.num_races; num++)
	{
		row = 2 + (num % 26);
		col = 30 * (num / 26);
		prt(r_name + r_info[cave_ecology.race[num]].name, row, col);

		col += 20;

		for (i = 0; i <= cave_ecology.num_ecologies; i++)
		{
			if (cave_ecology.race_ecologies[num] & (1L << i))
			{
				prt(format("%d", i), row, col += 1);
			}
		}

		for (i = 0; i <= cave_ecology.num_ecologies; i++)
		{
			if (cave_ecology.race_ecologies[num] & (1L << (i + MAX_ECOLOGIES)))
			{
				prt(format("c%d", i), row, col += 2);
			}
		}
	}

	/* Total monsters */
	msg_format("Ecology has %d races and %d sub-ecologies. The ecology is %sready.", cave_ecology.num_races, cave_ecology.num_ecologies,
			cave_ecology.ready ? "" : "not ");

	anykey();

	/* Screen_load */
	screen_load();
}




#ifdef ALLOW_SPOILERS

/*
 * External function
 */
extern void do_cmd_spoilers(void);

#endif



/*
 * Ask for and parse a "debug command"
 *
 * The "p_ptr->command_arg" may have been set.
 */
void do_cmd_debug(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	char cmd;


	/* Get a "debug command" */
	if (!get_com("Debug Command: ", &cmd)) return;

	/* Analyze the command */
	switch (cmd)
	{
		/* Ignore */
		case ESCAPE:
		case ' ':
		case '\n':
		case '\r':
		{
			break;
		}

#ifdef ALLOW_SPOILERS

		/* Hack -- Generate Spoilers */
		case '"':
		{
			do_cmd_spoilers();
			break;
		}

#endif


		/* Hack -- Help */
		case '?':
		{
			/* Save the screen */
			screen_save();

			/* Show stats help */
			(void)show_file("debug.txt", NULL, 0, 0);

			/* Load the screen */
			screen_load();
			break;
		}

		/* Cure all maladies */
		case 'a':
		{
			do_cmd_wiz_cure_all();
			break;
		}

		/* Teleport to target */
		case 'b':
		{
			do_cmd_wiz_bamf();
			break;
		}

		/* Create any object */
		case 'c':
		{
			wiz_create_item();
			break;
		}

		/* Create an artifact */
		case 'C':
		{
			wiz_create_artifact(p_ptr->command_arg);
			break;
		}

		/* Detect everything */
		case 'd':
		{
			bool cancel = FALSE;
			bool known = TRUE;

			process_spell_flags(SOURCE_PLAYER_WIZARD, 0, 92, 100, &cancel, &known);
			break;
		}

		/* Wizard detect */
		case 'D':
		{
			do_cmd_wiz_detect();
			break;
		}

		/* Edit character */
		case 'e':
		{
			do_cmd_wiz_change();
			break;
		}

		/* Show ecology */
		case 'E':
		{
			do_cmd_wiz_ecology();
			break;
		}

		/* View item info */
		case 'f':
		{
			(void)identify_fully();
			break;
		}

		/* Good Objects */
		case 'g':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			acquirement(py, px, p_ptr->command_arg, FALSE);
			break;
		}

		/* Hitpoint rerating */
		case 'h':
		{
			do_cmd_rerate();
			break;
		}

		/* Identify */
		case 'i':
		{
			(void)ident_spell();
			break;
		}

		/* Go up or down in the dungeon */
		case 'j':
		{
			do_cmd_wiz_jump();
			break;
		}

		/* Self-Knowledge */
		case 'k':
		{
			self_knowledge(TRUE);
			break;
		}

		/* Learn about objects */
		case 'l':
		{
			do_cmd_wiz_learn();
			break;
		}

		/* Regenerate level - Use last dungeon seed if specified */
		case 'L':
		{
			if (seed_last_dungeon) seed_dungeon = seed_last_dungeon;
			p_ptr->leaving = TRUE;

			msg_format("Generating %slevel.", seed_dungeon ? "last " : "");
			break;
		}

		/* Magic Mapping */
		case 'm':
		{
			map_area();
			break;
		}

		/* Summon Named Monster */
		case 'n':
		{
			do_cmd_wiz_named(p_ptr->command_arg, TRUE);
			break;
		}

		/* Object playing routines */
		case 'o':
		{
			do_cmd_wiz_play();
			break;
		}

		/* Phase Door */
		case 'p':
		{
			teleport_player(10);
			break;
		}

		/* Query the dungeon */
		case 'q':
		{
			do_cmd_wiz_query();
			break;
		}

		/* Summon Random Monster(s) */
		case 's':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			do_cmd_wiz_summon(p_ptr->command_arg);
			break;
		}

		/* Seed dungeon generation */
		case 'S':
		{
			char out_val[32];

			/* Default */
			sprintf(out_val, "%ld", seed_last_dungeon);

			/* Ask the user for a response */
			if (!get_string("Dungeon seed (0 to stop seeding, 1 for random): ", out_val, sizeof(out_val))) return;

			/* Extract a number */
			seed_dungeon = atol(out_val);

			/* Pick random value if requested */
			if (seed_dungeon == 1) seed_dungeon = rand_int(0x10000000);

			break;
		}


		/* Teleport */
		case 't':
		{
			teleport_player(100);
			break;
		}

		/* Un-hide all monsters */
		case 'u':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 255;
			do_cmd_wiz_unhide(p_ptr->command_arg);
			break;
		}

		/* Very Good Objects */
		case 'v':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			acquirement(py, px, p_ptr->command_arg, TRUE);
			break;
		}

		/* Wizard Light the Level */
		case 'w':
		{
			wiz_lite();
			break;
		}

		/* Increase Experience */
		case 'x':
		{
			if (p_ptr->command_arg)
			{
				gain_exp(p_ptr->command_arg);
			}
			else
			{
				gain_exp(p_ptr->exp + 1);
			}
			break;
		}

		/* Zap Monsters (Banishment) */
		case 'z':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 2 * MAX_SIGHT;
			do_cmd_wiz_zap(p_ptr->command_arg);
			break;
		}

		/* Nearest monster makes a magical attack */
		case 'Z':
		{
			int m_idx, y, x;

			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 110;

			/* XXX Note player is the closest, so we need the 2nd closest */
			get_closest_monster(2, p_ptr->py, p_ptr->px, &y, &x, 0x01, 0);

			m_idx = cave_m_idx[y][x];

			if (m_idx <= 0) break;
			make_attack_ranged(m_idx, p_ptr->command_arg, p_ptr->py, p_ptr->px);
			break;
		}

		/* Oops */
		default:
		{
			msg_print("That is not a valid debug command.");
			break;
		}
	}
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif


