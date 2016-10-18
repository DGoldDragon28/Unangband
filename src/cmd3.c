/* File: cmd3.c */

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
 * Display inventory
 */
void do_cmd_inven(void)
{
	/* Hack -- Start in "inventory" mode */
	p_ptr->command_wrk = (USE_INVEN);

	/* Save screen */
	screen_save();

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	/* Display the inventory */
	show_inven();

	/* Hack -- hide empty slots */
	item_tester_full = FALSE;

	/* Prompt for a command */
	prt("(Inventory) Command: ", 0, 0);

	/* Hack -- Get a new command */
	p_ptr->command_new = inkey_ex();

	/* Load screen */
	screen_load();


	/* Hack -- Process "Escape" */
	if (p_ptr->command_new.key == ESCAPE)
	{
		/* Reset stuff */
		p_ptr->command_new.key = 0;
	}

	/* Hack -- Process normal keys */
	else
	{
		/* Hack -- Use "display" mode */
		p_ptr->command_see = TRUE;
	}
}


/*
 * Display equipment
 */
void do_cmd_equip(void)
{
	/* Hack -- Start in "equipment" mode */
	p_ptr->command_wrk = (USE_EQUIP);

	/* Save screen */
	screen_save();

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	/* Display the equipment */
	show_equip();

	/* Hack -- undo the hack above */
	item_tester_full = FALSE;

	/* Prompt for a command */
	prt("(Equipment) Command: ", 0, 0);

	/* Hack -- Get a new command */
	p_ptr->command_new = inkey_ex();

	/* Load screen */
	screen_load();


	/* Hack -- Process "Escape" */
	if (p_ptr->command_new.key == ESCAPE)
	{
		/* Reset stuff */
		p_ptr->command_new.key = 0;
	}

	/* Hack -- Process normal keys */
	else
	{
		/* Enter "display" mode */
		p_ptr->command_see = TRUE;
	}
}


/*
 * The "wearable" tester
 */
bool item_tester_hook_wear(const object_type *o_ptr)
{
	/* Check if wearable/wieldable */
	if (wield_slot(o_ptr) >= INVEN_WIELD) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


static int quiver_wield(int item, object_type *i_ptr)
{
  int slot = 0;
  bool use_new_slot = FALSE;
  int num;
  int i;

  /* was: num = get_quantity(NULL, i_ptr->number);*/
  num = i_ptr->number;

  /* Cancel */
  if (num <= 0) return 0;

  /* Check free space */
  if (!quiver_carry_okay(i_ptr, num, item))
    {
      msg_print("Your quiver needs more backpack space.");
      return 0;
    }

  /* Search for available slots. */
  for (i = INVEN_QUIVER; i < END_QUIVER; i++)
    {
      object_type *o_ptr;

      /* Get the item */
      o_ptr = &inventory[i];

      /* Accept empty slot, but keep searching for combining */
      if (!o_ptr->k_idx)
	{
	  slot = i;
	  use_new_slot = TRUE;
	  continue;
	}

      /* Accept slot that has space to absorb more */
      if (object_similar(o_ptr, i_ptr))
	{
	  slot = i;
	  use_new_slot = FALSE;
	  object_absorb(o_ptr, i_ptr, FALSE);
	  break;
	}
    }

  if (!slot)
    {
      /* No space. */
      msg_print("Your quiver is full.");
      return 0;
    }

  /* Use a new slot if needed */
  if (use_new_slot)
    object_copy(&inventory[slot], i_ptr);

  /* Update "p_ptr->pack_size_reduce" */
  find_quiver_size();

  /* Reorder the quiver and return the perhaps modified slot */
  return reorder_quiver(slot);
}

/*
 * Mark the item as known cursed.
 */
void mark_cursed_feeling(object_type *o_ptr)
{
  switch (o_ptr->feeling)
    {
    case INSCRIP_ARTIFACT: o_ptr->feeling = INSCRIP_TERRIBLE; break;
    case INSCRIP_HIGH_EGO_ITEM: o_ptr->feeling = INSCRIP_WORTHLESS; break;
    case INSCRIP_EGO_ITEM: o_ptr->feeling = INSCRIP_WORTHLESS; break;
    default: o_ptr->feeling = INSCRIP_CURSED;
    }

  /* The object has been "sensed" */
  o_ptr->ident |= (IDENT_SENSE);
}


/*
 * Wield or wear a single item from the pack or floor
 * Now supports wearing multiple rings.
 */
bool player_wield(int item)
{
	int slot;

	object_type *o_ptr;
	object_type *j_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	cptr act;

	char o_name[80];

	u32b f1, f2, f3, f4;
	u32b n1, n2, n3, n4;
	u32b k1, k2, k3, k4;

	bool get_feat = FALSE;

	/* Hack -- Prevent player from wielding conflicting items */
	bool burn = FALSE;
	bool curse = FALSE;

	/* Hack -- Allow multiple rings to be wielded */
	int amt = 1;
	int rings = 0;

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

	/* Got from feature */
	if (o_ptr->ident & (IDENT_STORE)) get_feat = TRUE;

	/* Check the slot */
	slot = wield_slot(o_ptr);

	/* Hack -- slot not allowed */
	if (slot < 0) return (FALSE);

	/* Hack -- don't dual wield */
	if (slot == INVEN_ARM
	    && o_ptr->tval != TV_SHIELD
	    && o_ptr->tval != TV_RING
	    && o_ptr->tval != TV_AMULET)
	  {
	    if (!get_check("Wield it in your off-hand? "))
	      {
		slot = INVEN_WIELD;
	      }
	  }

	/* Hack -- multiple rings */
	else if (o_ptr->tval == TV_RING)
	{
		i_ptr = &inventory[slot];

		/* Wear multiple rings */
		if (object_similar(o_ptr,i_ptr)) rings = 5-i_ptr->number;

		/* Get a quantity - take off existing rings */
		if (!rings) amt = get_quantity(NULL, (o_ptr->number < 5) ? o_ptr->number : 5);

		/* Get a quantity - add to existing rings */
		else amt = get_quantity(NULL, (o_ptr->number < rings) ? o_ptr->number : rings);

		/* Allow user abort */
		if (amt <= 0) return (FALSE);
	}

	/* Hack - Throwing weapons can we wielded in the quiver too.
	   Ask the player, unless he has already chosen the off-hand. */
	if ( is_known_throwing_item(o_ptr)
		 && !IS_QUIVER_SLOT(slot)
		 && slot != INVEN_ARM
		 && get_check("Do you want to put it in the quiver? ") )
		slot = INVEN_QUIVER;

	/* Source and destination identical */
	if (item == slot) return (FALSE);

	/* Adjust amount */
	if (IS_QUIVER_SLOT(slot)) amt = o_ptr->number;

	/* Prevent wielding over 'built-in' item */
	if (!IS_QUIVER_SLOT(slot) && ((inventory[slot].ident & (IDENT_STORE)) != 0))
	{
		/* Describe it */
		object_desc(o_name, sizeof(o_name), &inventory[slot], FALSE, 0);

		/* Oops */
		msg_format("You cannot remove the %s.", o_name);

		/* Nope */
		return (FALSE);
	}
	/* Prevent wielding into a cursed slot */
	else if (!IS_QUIVER_SLOT(slot) && cursed_p(&inventory[slot]))
	{
		/* Describe it */
		object_desc(o_name, sizeof(o_name), &inventory[slot], FALSE, 0);

		/* Message */
		msg_format("The %s you are %s appears to be cursed.",
		   o_name, describe_use(slot));

		mark_cursed_feeling(&inventory[slot]);

		/* Cancel the command */
		return (FALSE);
	}
	/* Prevent wielding from cursed slot */
	else if ((item >= INVEN_WIELD) && cursed_p(&inventory[item]))
	{
		/* Describe it */
		object_desc(o_name, sizeof(o_name), &inventory[item], FALSE, 0);

		/* Message */
		msg_format("The %s you are %s appears to be cursed.",
		   o_name, describe_use(item));

		mark_cursed_feeling(&inventory[item]);

		/* Cancel the command */
		return (FALSE);
	}

	/* Take a turn here so that racial object sensing is not free */
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		{
			if (o_ptr->weight >= 200) p_ptr->energy_use = 100;
			else p_ptr->energy_use = 50;
			break;
		}
		case TV_BOW:
		{
			if (o_ptr->sval >= SV_HAND_XBOW) p_ptr->energy_use = 100;
			else p_ptr->energy_use = 50;
			break;
		}
		default:
		{
			p_ptr->energy_use = 100;
			break;
		}
	}

	/* Get object flags */
	object_flags(o_ptr,&f1,&f2,&f3,&f4);

	/* Check for racial conflicts */
	burn |= (f1 & (TR1_SLAY_NATURAL)) & (p_ptr->cur_flags4 & (TR4_ANIMAL));
	burn |= (f1 & (TR1_SLAY_ORC)) & (p_ptr->cur_flags4 & (TR4_ORC));
	burn |= (f1 & (TR1_SLAY_TROLL)) & (p_ptr->cur_flags4 & (TR4_TROLL));
	burn |= (f1 & (TR1_SLAY_GIANT)) & (p_ptr->cur_flags4 & (TR4_GIANT));
	burn |= (f4 & (TR4_SLAY_MAN)) & (p_ptr->cur_flags4 & (TR4_MAN));
	burn |= (f4 & (TR4_SLAY_ELF)) & (p_ptr->cur_flags4 & (TR4_ELF));
	burn |= (f4 & (TR4_SLAY_DWARF)) & (p_ptr->cur_flags4 & (TR4_DWARF));
	burn |= (f4 & (TR4_ANIMAL)) & (p_ptr->cur_flags1 & (TR1_SLAY_NATURAL));
	burn |= (f4 & (TR4_ORC)) & (p_ptr->cur_flags1 & (TR1_SLAY_ORC));
	burn |= (f4 & (TR4_TROLL)) & (p_ptr->cur_flags1 & (TR1_SLAY_TROLL));
	burn |= (f4 & (TR4_GIANT)) & (p_ptr->cur_flags1 & (TR1_SLAY_GIANT));
	burn |= (f4 & (TR4_MAN)) & (p_ptr->cur_flags4 & (TR4_SLAY_MAN));
	burn |= (f4 & (TR4_DWARF)) & (p_ptr->cur_flags4 & (TR4_SLAY_DWARF));
	burn |= (f4 & (TR4_ELF)) & (p_ptr->cur_flags4 & (TR4_SLAY_ELF));

	/* Evil players can wield racial conflict items but get cursed instead of burning */
	if ((burn != 0) && ((f4 & (TR4_EVIL)) || ((p_ptr->cur_flags4 & (TR4_EVIL)) != 0)))
	{
		curse = TRUE;
		burn = FALSE;
	}

	/* Check for elemental conflicts and high slays */
	burn |= (f1 & (TR1_SLAY_DRAGON)) & (p_ptr->cur_flags4 & (TR4_DRAGON));
	burn |= (f1 & (TR1_SLAY_UNDEAD)) & (p_ptr->cur_flags4 & (TR4_UNDEAD));
	burn |= (f1 & (TR1_SLAY_DEMON)) & (p_ptr->cur_flags4 & (TR4_DEMON));
	burn |= (f1 & (TR1_KILL_DEMON)) & (p_ptr->cur_flags4 & (TR4_DEMON));
	burn |= (f1 & (TR1_KILL_UNDEAD)) & (p_ptr->cur_flags4 & (TR4_UNDEAD));
	burn |= (f1 & (TR1_KILL_DRAGON)) & (p_ptr->cur_flags4 & (TR4_DRAGON));
	burn |= (f1 & (TR1_BRAND_HOLY)) & (p_ptr->cur_flags4 & (TR4_EVIL));
	burn |= (f1 & (TR1_BRAND_POIS)) & (p_ptr->cur_flags4 & (TR4_HURT_POIS));
	burn |= (f1 & (TR1_BRAND_ACID)) & (p_ptr->cur_flags4 & (TR4_HURT_ACID));
	burn |= (f1 & (TR1_BRAND_COLD)) & (p_ptr->cur_flags4 & (TR4_HURT_COLD));
	burn |= (f1 & (TR1_BRAND_ELEC)) & (p_ptr->cur_flags4 & (TR4_HURT_ELEC));
	burn |= (f1 & (TR1_BRAND_FIRE)) & (p_ptr->cur_flags4 & (TR4_HURT_FIRE));
	burn |= (f4 & (TR4_BRAND_LITE)) & (p_ptr->cur_flags4 & (TR4_HURT_LITE));

	burn |= (f4 & (TR4_EVIL)) & (p_ptr->cur_flags1 & (TR1_BRAND_HOLY));
	burn |= (f4 & (TR4_DRAGON)) & (p_ptr->cur_flags1 & (TR1_SLAY_DRAGON));
	burn |= (f4 & (TR4_UNDEAD)) & (p_ptr->cur_flags1 & (TR1_SLAY_UNDEAD));
	burn |= (f4 & (TR4_DEMON)) & (p_ptr->cur_flags1 & (TR1_SLAY_DEMON));
	burn |= (f4 & (TR4_DRAGON)) & (p_ptr->cur_flags1 & (TR1_KILL_DRAGON));
	burn |= (f4 & (TR4_UNDEAD)) & (p_ptr->cur_flags1 & (TR1_KILL_UNDEAD));
	burn |= (f4 & (TR4_DEMON)) & (p_ptr->cur_flags1 & (TR1_KILL_DEMON));
	burn |= (f4 & (TR4_HURT_POIS)) & (p_ptr->cur_flags1 & (TR1_BRAND_POIS));
	burn |= (f4 & (TR4_HURT_ACID)) & (p_ptr->cur_flags1 & (TR1_BRAND_ACID));
	burn |= (f4 & (TR4_HURT_COLD)) & (p_ptr->cur_flags1 & (TR1_BRAND_COLD));
	burn |= (f4 & (TR4_HURT_ELEC)) & (p_ptr->cur_flags1 & (TR1_BRAND_ELEC));
	burn |= (f4 & (TR4_HURT_FIRE)) & (p_ptr->cur_flags1 & (TR1_BRAND_FIRE));
	burn |= (f4 & (TR4_HURT_LITE)) & (p_ptr->cur_flags4 & (TR4_BRAND_LITE));

	if (burn != 0)
	{
		/* Warn the player */
		msg_print("Aiee! It feels burning hot!");

		/* Mark object as ungettable? */
		if (!(o_ptr->feeling) &&
			!(o_ptr->ident & (IDENT_SENSE))
			&& !(object_named_p(o_ptr)))
		{

			/* Sense the object */
			o_ptr->feeling = INSCRIP_UNGETTABLE;

			/* The object has been "sensed" */
			o_ptr->ident |= (IDENT_SENSE);
		}

		return (TRUE); /* The turn already taken for racial sensing */
	}

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain local object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* Get a new show index */
	if (o_ptr->number > amt)
	{
		int j, k;

		/* Find the next free show index */
		for (j = 1; j < SHOWN_TOTAL; j++)
		{
			bool used = FALSE;

			/* Check all items */
			for (k = 0; k < INVEN_TOTAL; k++) if ((inventory[k].k_idx) && (inventory[k].show_idx == j)) used = TRUE;

			/* Already an item using this slot? */
			if (used) continue;

			/* Use this slot */
			break;
		}

		/* Set the show index for the item */
		if (j < SHOWN_TOTAL) i_ptr->show_idx = j;
		else i_ptr->show_idx = 0;

		/* Redraw stuff */
		p_ptr->redraw |= (PR_ITEM_LIST);
	}

	/* Ammo goes in quiver slots, which have special rules. */
	if (IS_QUIVER_SLOT(slot))
	  {
	    /* Put the ammo in the quiver */
	    slot = quiver_wield(item, i_ptr);

	    /* Can't do it; note the turn already wasted for racial sensing */
	    if (!slot) return (TRUE);
	  }

	/* Reset stackc; assume no wands in quiver */
	i_ptr->stackc = 0;

	/* Sometimes use lower stack object */
	if (!object_charges_p(o_ptr)
	    && (rand_int(o_ptr->number) < o_ptr->stackc))
	  {
	    if (amt >= o_ptr->stackc)
	      {
		i_ptr->stackc = o_ptr->stackc;

		o_ptr->stackc = 0;
	      }
	    else
	      {
		if (i_ptr->charges) i_ptr->charges--;
		if (i_ptr->timeout) i_ptr->timeout = 0;

		o_ptr->stackc -= amt;
	      }
	  }

	/* Decrease the item (from the pack); cancelling no longer possible */
	if (item >= 0)
	{
		/* Hack -- Forget what original stack may do */
		/* This allows us to identify the wielded item's function */
		if (o_ptr->number < amt) drop_may_flags(o_ptr);

		inven_item_increase(item, -amt);
		inven_item_optimize(item);

	}
	/* Decrease the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_optimize(0 - item);
		if (get_feat && (scan_feat(p_ptr->py,p_ptr->px) < 0)) cave_alter_feat(p_ptr->py,p_ptr->px,FS_GET_FEAT);
	}

	/* Get the wield slot */
	j_ptr = &inventory[slot];


	/* Special rules for similar rings */
	if (rings)
	  {
	    /* Wear the new rings */
	    object_absorb(j_ptr, i_ptr, FALSE);
	  }
	else if (!IS_QUIVER_SLOT(slot))
	/* Normal rules for non-ammo */
	  {
	    /* Take off existing item */
	    if (j_ptr->k_idx)
	      (void)inven_takeoff(slot, 255);

	    /* Wear the new stuff */
	    object_copy(j_ptr, i_ptr);

	    /* Increment the equip counter by hand */
	    p_ptr->equip_cnt++;
	  }

	/* Increase the weight */
	p_ptr->total_weight += i_ptr->weight * amt;

	/* Important - clear this flag */
	j_ptr->ident &= ~(IDENT_STORE | IDENT_MARKED);

	/* Hack - prevent dragon armour swap abuse */
	if (j_ptr->tval == TV_DRAG_ARMOR)
	{
		j_ptr->timeout = 2 * j_ptr->charges;
	}

	/* Where is the item now */
	if (slot == INVEN_WIELD)
	{
		act = "You are wielding";
	}
	else if (slot == INVEN_ARM)
	{
		if (j_ptr->tval == TV_SHIELD) act = "You are wearing";
		else act = "You are off-hand wielding";
	}
	else if (j_ptr->tval == TV_INSTRUMENT)
	{
		act = "You are playing music with";
	}
	else if (slot == INVEN_BOW)
	{
		act = "You are shooting with";
	}
	else if (slot == INVEN_LITE)
	{
		act = "Your light source is";

		/* Light torch if not lit already */
		if (!(artifact_p(j_ptr)) && !(j_ptr->timeout))
		{
			j_ptr->timeout = j_ptr->charges;
			j_ptr->charges = 0;
		}
	}
	else if (!IS_QUIVER_SLOT(slot))
	{
		act = "You are wearing";
	}
	else
	{
		act = "You have readied";
	}

	/* Describe the result */
	object_desc(o_name, sizeof(o_name), j_ptr, TRUE, 3);

	/* Message */
	msg_format("%s %s (%c).", act, o_name, index_to_label(slot));

	/* Cursed! */
	if (curse || cursed_p(j_ptr))
	{
		/* Warn the player */
		msg_print("Oops! It feels deathly cold!");

		mark_cursed_feeling(j_ptr);
	}
	/* Not cursed */
	else
	{
		switch (j_ptr->feeling)
		{
			case INSCRIP_ARTIFACT: j_ptr->feeling = INSCRIP_SPECIAL; break;
			case INSCRIP_HIGH_EGO_ITEM: j_ptr->feeling = INSCRIP_SUPERB; break;
			case INSCRIP_EGO_ITEM: j_ptr->feeling = INSCRIP_EXCELLENT; break;
			case INSCRIP_MAGIC_ITEM: j_ptr->feeling = INSCRIP_USEFUL; break;
			case INSCRIP_NONMAGICAL: j_ptr->feeling = INSCRIP_USELESS; break;
			case INSCRIP_UNUSUAL: j_ptr->feeling = INSCRIP_UNCURSED; break;
		}
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Update item list */
	p_ptr->redraw |= (PR_ITEM_LIST);

	/* Cannot learn quivered item effects */
	if (IS_QUIVER_SLOT(slot)) return (TRUE);

	/* Get known flags */
	k1 = j_ptr->can_flags1;
	k2 = j_ptr->can_flags2;
	k3 = j_ptr->can_flags3;
	k4 = j_ptr->can_flags4;

	/* Some flags are instantly known */
	object_flags(j_ptr, &f1, &f2, &f3, &f4);

	/* Hack -- identify pval */
	if (!object_pval_p(j_ptr) &&
		 f1 & (TR1_BLOWS | TR1_SHOTS | TR1_SPEED | TR1_STR |
				 TR1_INT | TR1_DEX | TR1_CON | TR1_CHR))
		j_ptr->ident |= (IDENT_PVAL);

	/* Hack -- the following are obvious from the displayed combat statistics */
	if (f1 & (TR1_BLOWS)) object_can_flags(j_ptr,TR1_BLOWS,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_BLOWS,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_SHOTS)) object_can_flags(j_ptr,TR1_SHOTS,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_SHOTS,0x0L,0x0L,0x0L, FALSE);

	/* Hack -- the following are obvious from the displayed combat statistics */
	if (f1 & (TR1_SPEED)) object_can_flags(j_ptr,TR1_SPEED,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_SPEED,0x0L,0x0L,0x0L, FALSE);

	/* Hack -- the following are obvious from the displayed stats */
	if (f1 & (TR1_STR)) object_can_flags(j_ptr,TR1_STR,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_STR,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_INT)) object_can_flags(j_ptr,TR1_INT,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_INT,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_WIS)) object_can_flags(j_ptr,TR1_WIS,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_WIS,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_DEX)) object_can_flags(j_ptr,TR1_DEX,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_DEX,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_CON)) object_can_flags(j_ptr,TR1_CON,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_CON,0x0L,0x0L,0x0L, FALSE);

	if (f1 & (TR1_CHR)) object_can_flags(j_ptr,TR1_CHR,0x0L,0x0L,0x0L, FALSE);
	else object_not_flags(j_ptr,TR1_CHR,0x0L,0x0L,0x0L, FALSE);

#ifndef ALLOW_OBJECT_INFO_MORE
	/* Hack --- we do these here, because they are too computationally expensive in the 'right' place */
	if (f1 & (TR1_INFRA))
	{
		object_can_flags(j_ptr,TR1_INFRA,0x0L,0x0L,0x0L, FALSE);
	}
	else object_not_flags(j_ptr,TR1_INFRA,0x0L,0x0L,0x0L, FALSE);

	if (f3 & (TR3_LITE))
	{
		object_can_flags(j_ptr,0x0L,0x0L,TR3_LITE,0x0L, FALSE);
	}
	else object_not_flags(j_ptr,0x0L,0x0L,TR3_LITE,0x0L, FALSE);

	/* Hack --- also computationally expensive. But note we notice these only if we don't already */
	/* have these abilities */
	if ((f3 & (TR3_TELEPATHY)) && !(p_ptr->cur_flags3 & (TR3_TELEPATHY)))) object_can_flags(j_ptr,0x0L,0x0L,TR3_TELEPATHY,0x0L, FALSE);
	else object_not_flags(j_ptr,0x0L,0x0L,TR3_TELEPATHY,0x0L, FALSE);

	if ((f3 & (TR3_SEE_INVIS)) && !(p_ptr->cur_flags3 & (TR3_SEE_INVIS)) && (!p_ptr->tim_invis)) object_can_flags(j_ptr,0x0L,0x0L,TR3_SEE_INVIS,0x0L, FALSE);
	else object_not_flags(j_ptr,0x0L,0x0L,TR3_SEE_INVIS,0x0L, FALSE);
#endif

	/* Hack --- the following are either obvious or (relatively) unimportant */
	if (f3 & (TR3_BLESSED))
	{
		object_can_flags(j_ptr,0x0L,0x0L,TR3_BLESSED,0x0L, FALSE);
	}
	else object_not_flags(j_ptr,0x0L,0x0L,TR3_BLESSED,0x0L, FALSE);

	if (f3 & (TR3_LIGHT_CURSE)) object_can_flags(j_ptr,0x0L,0x0L,TR3_LIGHT_CURSE,0x0L, FALSE);
	else object_not_flags(j_ptr,0x0L,0x0L,TR3_LIGHT_CURSE,0x0L, FALSE);

	/* Check for new flags */
	n1 = j_ptr->can_flags1 & ~(k1);
	n2 = j_ptr->can_flags2 & ~(k2);
	n3 = j_ptr->can_flags3 & ~(k3);
	n4 = j_ptr->can_flags4 & ~(k4);

	if (n1 || n2 || n3 || n4) update_slot_flags(slot, n1, n2, n3, n4);

	return (TRUE);
}


/*
 * The "drop" tester
 */
bool item_tester_hook_droppable(const object_type *o_ptr)
{
	/* Forbid 'built-in' */
	if (o_ptr->ident & (IDENT_STORE)) return (FALSE);

	return (TRUE);
}


/*
 * The "takeoff" tester
 */
bool item_tester_hook_removable(const object_type *o_ptr)
{
	/* Forbid not droppable */
	if (!item_tester_hook_droppable(o_ptr)) return (FALSE);

	/* Forbid cursed */
	if (o_ptr->ident & (IDENT_CURSED)) return (FALSE);

	return (TRUE);
}


/*
 * Take off an item
 */
bool player_takeoff(int item)
{
	object_type *o_ptr;

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

	/* Item is 'built-in' to shape*/
	if (!IS_QUIVER_SLOT(item) && ((inventory[item].ident & (IDENT_STORE)) != 0))
	{
		/* Oops */
		msg_print("You cannot remove this.");

		/* Nope */
		return (FALSE);
	}
	/* Item is cursed */
	else if (cursed_p(o_ptr))
	{
		/* Oops */
		msg_print("Hmmm, it seems to be cursed.");

		mark_cursed_feeling(o_ptr);

		/* Nope */
		return (FALSE);
	}
	/* Cursed quiver */
	else if (IS_QUIVER_SLOT(item) && p_ptr->cursed_quiver)
	{
		/* Oops */
		msg_print("Your quiver is cursed!");

		/* Nope */
		return (FALSE);
	}

	/* Take a partial turn */
	p_ptr->energy_use = 50;

	/* Take off the item */
	(void)inven_takeoff(item, 255);

	return (TRUE);
}


/*
 * Drop an item
 */
bool player_drop(int item)
{
	int amt;

	object_type *o_ptr;

	object_type object_type_body;
	
	/* Get gold */
	if (item == INVEN_GOLD)
	{
		o_ptr = &object_type_body;
		
		/* Get a quantity */
		amt = get_quantity(NULL, p_ptr->au);		
		
		/* Allow abort */
		if (amt <= 0) return (FALSE);
		
		/* Prepare the gold */
		object_prep(o_ptr, lookup_kind(TV_GOLD, 9));
		
		/* Make it worth the amount */
		o_ptr->charges = amt;
		
		/* Lose it */
		p_ptr->au -= amt;
		
		/* Update display */
		p_ptr->redraw |= (PR_GOLD);

		/* Give it to the floor */
		floor_carry(p_ptr->py, p_ptr->px, o_ptr);
		
		/* Take a partial turn */
		p_ptr->energy_use = 50;

		/* We are done */
		return (TRUE);
	}
	
	/* Get the item (in the pack) */
	else if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Get a quantity */
	amt = get_quantity(NULL, o_ptr->number);

	/* Allow user abort */
	if (amt <= 0) return (FALSE);

	/* Prevent wielding over 'built-in' item */
	if (!IS_QUIVER_SLOT(item) && ((inventory[item].ident & (IDENT_STORE)) != 0))
	{
		/* Oops */
		msg_print("You cannot remove this.");

		/* Nope */
		return (FALSE);
	}
	/* Hack -- Cannot remove cursed items */
	else if ((item >= INVEN_WIELD) && cursed_p(o_ptr))
	{
		/* Oops */
		msg_print("Hmmm, it seems to be cursed.");

		mark_cursed_feeling(o_ptr);

		/* Nope */
		return (FALSE);
	}
	/* Cursed quiver */
	else if (IS_QUIVER_SLOT(item) && p_ptr->cursed_quiver)
	{
		/* Oops */
		msg_print("Your quiver is cursed!");

		/* Nope */
		return (FALSE);
	}

	/* Take a partial turn */
	p_ptr->energy_use = 50;

	/* Drop (some of) the item */
	inven_drop(item, amt, 0);

	return (TRUE);
}


/*
 * Steal from a monster
 */
bool player_steal(int item)
{
	char m_name[80];
	char m_poss[80];
	
	/* Ensure we have a target already */
	monster_type *m_ptr = &m_list[p_ptr->target_who];
	
	/* Paranoia */
	if (p_ptr->target_who <= 0) return (FALSE);
	
	/* Acquire the monster name */
	monster_desc(m_name, sizeof(m_name), p_ptr->target_who, 0x00);
	
	/* Skill check if monster is awake */
	if (!m_ptr->csleep)
	{
		/* Disarm factor */
		int i = p_ptr->skills[SKILL_STEALTH] * 10;
		int j;
	
		/* Penalize some conditions */
		if (p_ptr->timed[TMD_BLIND] || no_lite()) i = i / 10;
		if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE]) i = i / 10;
	
		/* Extract the difficulty */
		j = r_info[m_ptr->r_idx].level;
	
		/* Extract the difficulty XXX XXX XXX */
		j = i - j * 2;
	
		/* Always have a small chance of success */
		if (j < 2) j = 2;

		/* Failure */
		if (rand_int(100) > j)
		{
			/* Acquire the monster poss */
			monster_desc(m_poss, sizeof(m_name), p_ptr->target_who, 0x22);
			
			/* Message */
			msg_format("%^s grabs a hold of %s possessions!", m_name, m_poss);
			
			/* Get aggrieved */
			m_ptr->mflag|= (MFLAG_AGGR);
			
			/* Disturb */
			disturb(0, 0);
			
			/* And tell allies */
			tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), "& has tried to steal from me!");
			
			/* Use up energy */
			p_ptr->energy_use = 100;
			
			return (FALSE);
		}
	}
	
	/* Monster has no more gold */
	if (m_ptr->mflag & (MFLAG_MADE))
	{
		/* Only get bonus gold theft once */
		if (item == INVEN_GOLD)
		{
			/* Message */
			msg_format("%^s has no more gold to steal.", m_name);
			
			return (FALSE);
		}
	}
	
	/* Stealing gold is a straight reward */
	if (item == INVEN_GOLD)
	{
		/* Monsters carries something */
		if (((r_info[m_ptr->r_idx].flags1 & (RF1_ONLY_ITEM)) == 0)
				&& (r_info[m_ptr->r_idx].flags1 >= RF1_DROP_30))
		{
			/* XXX Reward based on monster level - token amount for town */
			int au = randint(r_info[m_ptr->r_idx].level * 30 + 5);
			
			p_ptr->au += au;
			
			/* Update display */
			p_ptr->redraw |= (PR_GOLD);
			
			msg_format("You steal %d gold pieces.", au);
		}
		else
		{
			/* Message */
			msg_format("%^s has no gold to steal.", m_name);
		}
		
		/* TODO: Go through inventory and take gold / gems? */
	}
	else
	{
		object_type *o_ptr = &o_list[0 - item];
		
		/* Get a quantity */
		int amt = get_quantity(NULL, o_ptr->number);

		/* Get the item */
		inven_takeoff(item, amt);
	}
	
	/* Stealing monster first time reveals the rest of its inventory */
	if (((m_ptr->mflag & (MFLAG_MADE)) == 0) && (monster_drop(p_ptr->target_who)))
	{
		/* Message */
		msg_format("%^s has more to steal.", m_name);
	}
	
	/* Use up energy */
	p_ptr->energy_use = 100;
	
	return (TRUE);
}


static int trade_value = 0;
static int trade_amount = 0;
static int trade_m_idx = 0;
static int trade_highly_value = 0;

/*
 * The "trade" tester
 */
bool item_tester_hook_tradeable(const object_type *o_ptr)
{
	/* Forbid not droppable */
	if (!item_tester_hook_droppable(o_ptr)) return (FALSE);

	/* Ensure value - monsters always know real value */
	if (object_value_real(o_ptr) > trade_value) return (FALSE);

	return (TRUE);
}


/*
 * Give an item to a monster
 */
bool player_offer(int item)
{
	int amt, m_idx;

	object_type *o_ptr = NULL; /* Assignment to suppress warning */
	monster_type *m_ptr;
	monster_race *r_ptr;
	
	char m_name[80];
	
	/* Get gold */
	if (item == INVEN_GOLD)
	{
		if (!p_ptr->au)
		{
			msg_print("You have no gold.");
			
			return (FALSE);
		}
		
		/* Get a quantity */
		amt = get_quantity_aux(NULL, p_ptr->au, (p_ptr->depth + 5) * (p_ptr->depth + 5) * (p_ptr->depth + 5));
	}
	
	/* Get the item (in the pack) */
	else if (item >= 0)
	{
		o_ptr = &inventory[item];
		
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
		
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);
	}

	/* Allow user abort */
	if (amt <= 0) return (FALSE);

	/* Get monster */
	m_idx = get_monster_by_aim((TARGET_KILL | TARGET_ALLY));
	
	/* Not a monster */
	if (m_idx <= 0) return (FALSE);

	/* Get monster */
	m_ptr = &m_list[m_idx];
	r_ptr = &r_info[m_ptr->r_idx];
	
	/* Ensure projectable */
	if (!player_can_fire_bold(m_ptr->fy, m_ptr->fx)) return (FALSE);

	/* Set target */
	p_ptr->target_who = m_idx;
	p_ptr->target_row = m_ptr->fy;
	p_ptr->target_col = m_ptr->fx;
	
	/* Check for allowed flags */
	if (item != INVEN_GOLD)
	{
		u32b f1, f2, f3, f4;

		u32b flg3 = 0L;
		
		char o_name[80];
			
		/* Extract some flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* React to objects that hurt the monster */
		if (f1 & (TR1_SLAY_DRAGON)) flg3 |= (RF3_DRAGON);
		if (f1 & (TR1_SLAY_TROLL)) flg3 |= (RF3_TROLL);
		if (f1 & (TR1_SLAY_GIANT)) flg3 |= (RF3_GIANT);
		if (f1 & (TR1_SLAY_ORC)) flg3 |= (RF3_ORC);
		if (f1 & (TR1_SLAY_DEMON)) flg3 |= (RF3_DEMON);
		if (f1 & (TR1_SLAY_UNDEAD)) flg3 |= (RF3_UNDEAD);
		if (f1 & (TR1_SLAY_NATURAL)) flg3 |= (RF3_ANIMAL | RF3_PLANT | RF3_INSECT);
		if (f1 & (TR1_BRAND_HOLY)) flg3 |= (RF3_EVIL);

		/* The object cannot be picked up by the monster */
		if (artifact_p(o_ptr) || (r_ptr->flags3 & flg3))
		{
			/* Acquire the object name */
			object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

			/* Acquire the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0x04);

			/* Don't accept */
			msg_format("%^s refuses %s.", m_name, o_name);
			
			/* Mark object as ungettable? */
			if (!(o_ptr->feeling) &&
				!(o_ptr->ident & (IDENT_SENSE))
				&& !(object_named_p(o_ptr)))
			{
				/* Sense the object */
				o_ptr->feeling = INSCRIP_UNGETTABLE;

				/* The object has been "sensed" */
				o_ptr->ident |= (IDENT_SENSE);
			}
		}
	}	
	
	/* Just give stuff to allies - except townsfolk */
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && ((m_ptr->mflag & (MFLAG_TOWN)) == 0))
	{
		/* Oops */
		if (item == INVEN_GOLD)
		{
			msg_print("You can't give gold to your allies to carry.");
			return (FALSE);
		}
		
		/* Drop (some of) the item */
		inven_drop(item, amt, m_idx);
		
		/* Take a partial turn */
		p_ptr->energy_use = 50;
	}
	/* Trade with monsters and townsfolk */
	else
	{
		trade_highly_value = 0;
		
		/* Get the monster name (or "it") */
		monster_desc(m_name, sizeof(m_name), m_idx, 0x04);

		/* Gollums wake monsters up */
		if (adult_gollum)
		{
			m_ptr->csleep = 0;
		}
		
		/* Sleeping monsters ignore you */
		if (m_ptr->csleep)
		{
			msg_format("%^s is asleep.", m_name);

			return (FALSE);
		}

		/* Guardians ignore you */
		if (r_ptr->flags1 & (RF1_GUARDIAN))
		{
			msg_format("%^s guards this place and cannot be bought.", m_name);
			
			return (FALSE);
		}

		/* Too stupid */
		if (r_ptr->flags2 & (RF2_STUPID))
		{
			msg_format("%^s ignores your offer.", m_name);
			
			return (FALSE);
		}

		/* Only undead/insect are cannibals */
		if ((item != INVEN_GOLD) && (o_ptr->name3) && (r_info[o_ptr->name3].d_char == r_ptr->d_char) && ((r_ptr->flags3 & (RF3_UNDEAD | RF3_INSECT)) == 0))
		{
			/* Oops */
			if (r_ptr->flags2 & (RF2_SMART))
			{
				msg_format("%^s is offended by your desecration of a corpse.", m_name);
				
				m_ptr->mflag |= (MFLAG_AGGR);
			}
			else
			{
				msg_format("%^s ignores your offer.", m_name);
			}
			
			return (FALSE);
		}
		
		/* Mustn't be aggressive */
		if (m_ptr->mflag & (MFLAG_AGGR))
		{
			msg_format("%^s is offended and seeking revenge.", m_name);
			return (FALSE);
		}
		
		/* Townsfolk are more mercenary */
		if ((m_ptr->mflag & (MFLAG_TOWN)) == 0)
		{
			/* Mustn't be injured -- unless critically injured and the player offering healing */
			/* This critically injured definition must match the critically injured and looking to feed monster definition in melee2.c so that the
			 * monster can use the item after being given it. */
			if ((m_ptr->hp < m_ptr->maxhp) && !((m_ptr->hp < m_ptr->maxhp / 10) || (m_ptr->mana < r_ptr->mana / 5)))
			{
				msg_format("%^s is hurt and not interested.", m_name);
				return (FALSE);
			}
			
			/* Looking for healing / mana recovery or not smart and doesn't speak the player's language */
			if ((m_ptr->hp < m_ptr->maxhp)
					|| (((r_ptr->flags2 & (RF2_SMART)) != 0) && !(player_understands(monster_language(m_ptr->r_idx)))))
			{
				/* Check kind */
				object_kind *k_ptr;
				
				/* Hack -- kind for gold. Injured monsters never accept gold, but we do want to respond with the correct message for what they want. */
				if (item == INVEN_GOLD)
				{
					k_ptr = &k_info[lookup_kind(TV_GOLD, 9)];
				}
				/* Get kind */
				else
				{
					k_ptr= &k_info[o_ptr->k_idx];
				}

				/* Edible - mana recovery */
				if ((k_ptr->flags6 & (TR6_EAT_MANA)) && (m_ptr->mana < r_ptr->mana / 5)) trade_highly_value = 4;

				/* Edible - healing */
				else if ((k_ptr->flags6 & (TR6_EAT_HEAL)) && ((m_ptr->hp < m_ptr->maxhp / 2) || (m_ptr->blind)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0)) trade_highly_value = 2;
				
				/* General food requirements */
				else if ((k_ptr->flags6 & (TR6_EAT_BODY)) && (r_ptr->flags2 & (RF2_EAT_BODY))) trade_highly_value = rand_int(3);
				else if ((k_ptr->flags6 & (TR6_EAT_INSECT)) && (r_ptr->flags3 & (RF3_INSECT))) trade_highly_value = rand_int(3);
				else if ((k_ptr->flags6 & (TR6_EAT_ANIMAL)) && (r_ptr->flags3 & (RF3_ANIMAL)) && ((r_ptr->flags2 & (RF2_EAT_BODY)) == 0)) trade_highly_value = rand_int(3);
				else if ((k_ptr->flags6 & (TR6_EAT_SMART)) && (r_ptr->flags2 & (RF2_SMART)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0)) trade_highly_value = rand_int(3);

				/* Alert the player to the monster's needs */
				else if (((m_ptr->hp < m_ptr->maxhp / 2) || (m_ptr->blind)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0))
				{
					msg_format("%^s is hurt and needs a way to heal.", m_name);
					return (FALSE);
				}
				
				/* Alert the player to the monster's needs */
				else if (m_ptr->mana < r_ptr->mana / 5)
				{
					msg_format("%^s is low on mana and needs a way to recover it.", m_name);
					return (FALSE);
				}

				/* Alert the player to the monster's needs */
				else if ((m_ptr->mflag & (MFLAG_WEAK)) != 0)
				{
					msg_format("%^s is hungry and needs food.", m_name);
					return (FALSE);
				}
				
				/* No needs */
				else
				{
					msg_format("%^s ignores you for the moment.", m_name);
					return (FALSE);
				}
			}
			
			/* XXX More reasons might be too annoying */
		}
		
		/* Cash is king */
		if (item == INVEN_GOLD)
		{
			trade_value = amt * (200 - adj_chr_gold[p_ptr->stat_ind[A_CHR]]) / 200;
		}
		else
		{
			/* Get the trade value - monsters always know real value */
			trade_value = amt * object_value_real(&inventory[item]) * (200 - adj_chr_gold[p_ptr->stat_ind[A_CHR]]) / 400;
		}
		
		/* Evil monsters are easier to buy, but more likely to betray you */
		if ((r_ptr->flags3 & (RF3_EVIL)) == 0) trade_value /= 2;
		
		/* Note the amount */
		trade_amount = amt;
		
		/* Note the recipient */
		trade_m_idx = m_idx;
		
		/* Set up for second command */
		p_ptr->command_trans = COMMAND_ITEM_OFFER;
		p_ptr->command_trans_item = item;
	}
	
	return (TRUE);
}



/* The normal sales acceptance text */
#define MAX_COMMENT_1a   6

static cptr comment_1a[MAX_COMMENT_1a] =
{
	"Okay.",
	"Fine.",
	"Accepted!",
	"Agreed!",
	"Done!",
	"Taken!"
};

/* Offer just distracts the monster momentarily from killing the player */

#define MAX_COMMENT_1b   6

static cptr comment_1b[MAX_COMMENT_1b] =
{
	"Hmmm...",
	"Humph!",
	"Huh?",
	"Heh heh heh...",
	"Mmmm...",
	"Maybe..."
};


/* Consider not killing the player for a short while */

#define MAX_COMMENT_1c   6

static cptr comment_1c[MAX_COMMENT_1c] =
{
	"Do you want to talk before I kill you, puny @?",
	"Why are you not fighting me? Are you a coward, @, like the rest?",
	"This feels like a typical @ trick!",
	"I've never met a @ I didn't hate. Why should you be any different?",
	"I find your insolence... amusing. Pray tell me, @, why you don't deserve to die.",
	"Let me put this away for safe keeping, @... you death is still a... little way off..."
};


/* Some terms of trade */

#define MAX_COMMENT_1d   6

static cptr comment_1d[MAX_COMMENT_1d] =
{
	"I might consider another offer, @.",
	"Maybe we can continue doing business, @.",
	"You might find further discussion more profitable, @.",
	"I'll scratch your back, @. You scratch... here, where I itch the most...",
	"Your stench taints what you touch @. It'll be hard to trade what you gave me. But keep trying...",
	"We seem to have come to terms, @. I might have something more for you."
};


/* Become (temporary) friends */

#define MAX_COMMENT_1e   6

static cptr comment_1e[MAX_COMMENT_1e] =
{
	"You're look pretty ugly... but not for a @, I'm sure!",
	"You're not so bad, after all... for a @.",
	"Tell me more about your # ways...",
	"You might learn from me, @, before you can become a better #.",
	"Forgive my friends, #. They see only a @.",
	"Parley with a @? A @ #? Who would have thought?"
};


/* Comments for when player gets cheated of item or money */

#define MAX_COMMENT_2   6

static cptr comment_2[MAX_COMMENT_2] =
{
	"The cheque is in the mail.",
	"Taken! Sorry, did you want something more?",
	"You'll get want you want soon enough.",
	"I've got it for you. Just try and get it off me.",
	"I've left it for you in Nowhere town.",
	"I've had some... supply issues at the moment and the merchandise has been delayed."
};


/* Comments for when monster doesn't have the cash on them in the dungeon */

#define MAX_COMMENT_3   6

static cptr comment_3[MAX_COMMENT_3] =
{
	"I'll have to take you to my master for the money.",
	"I keep what you want in a hiding place near where I sleep.",
	"Someone owes this to me. I need you to help collect the debt.",
	"It's just up ahead.  Not much further now...",
	"It's near the entrance to the next level. Let me take you there.",
	"I left it around here somewhere. Let me think..."
};


/* Comments for when monster speaks to his allies - when heard by the player (proposal) */

#define MAX_COMMENT_4a   6

static cptr comment_4a[MAX_COMMENT_4a] =
{
	"& proposes an interesting conundrum.",
	"& has set a challenge to us.",
	"& may make this worth our while to listen.",
	"& may have much wealth and goods to distribute.",
	"& has not tried to fight back. This is shows some... respect.",
	"& comes to us open-handed."
};


/* Comments for when monster speaks to his allies - when heard by the player (silver-tongued) */

#define MAX_COMMENT_4b   6

static cptr comment_4b[MAX_COMMENT_4b] =
{
	"I have not seen such a brave or noble endeavour.",
	"I would place much trust in such a @.",
	"We should all profit from this good work.",
	"We can turn the tide of battle through words, not war.",
	"There will be peace in our time.",
	"Let us walk together and talk on this further."
};


/* Comments for when monster speaks to his allies - when heard by the player (haughty) */

#define MAX_COMMENT_4c   6

static cptr comment_4c[MAX_COMMENT_4c] =
{
	"This is an omen, whether for good or ill I cannot yet tell.",
	"The gods work in mysterious ways.",
	"We must discuss this... without undue influence from you, @.",
	"But the @ may just be spreading lies with a silver tongue...",
	"But what has gold bought us until now?",
	"But a sharpened blade cuts both ways."
};


/* Comments for when monster speaks to his allies - when not heard by the player (proposal) */

#define MAX_COMMENT_5a   6

static cptr comment_5a[MAX_COMMENT_5a] =
{
	"& swaggers in here like the place was already bought and paid for.",
	"& disgraces the spirits of our ancestors.",
	"& tries to buy a way out of trouble.",
	"& is not worth the weight of gold.",
	"& is clearly a coward.",
	"& thinks to bribe us as if we were savages."
};


/* Comments for when monster speaks to his allies - when not heard by the player (quick betrayal) */

#define MAX_COMMENT_5b   6

static cptr comment_5b[MAX_COMMENT_5b] =
{
	"I have need of the entrails of a @. Which cut do you want?",
	"The gods demand sacrifice of a @. Be quick about it!",
	"Wait a moment, and see what spoils we'll loot from this walking corpse.",
	"Bide your time. Strike when the @ least expects it.",
	"The walls whispered 'Blood for Morgoth' to me in my dreams. This @ answers that vision.",
	"Do we have a cooking pot big enough for a @?"
};


/* Comments for when monster speaks to his allies - when not heard by the player (intrigued/desperate) */

#define MAX_COMMENT_5c   6

static cptr comment_5c[MAX_COMMENT_5c] =
{
	"But we are poor and in need of money, so let us find some profit here.",
	"This may be a way for us to curry favour elsewhere.",
	"This morning, I saw a sign from our gods. Now I know why...",
	"But I am tired of fighting. Should we not rest a while?",
	"But that display of bravado impresses me, nonetheless.",
	"If nothing else, this @ # will be amusing sport."
};




/*
 * Trade with a monster
 */
bool player_trade(int item2)
{
	int item = p_ptr->command_trans_item;
	int amt, max, value;
	
	object_type *j_ptr;
	monster_type *m_ptr = &m_list[trade_m_idx];

	char m_name[80];

	/* Get gold */
	if (item2 == INVEN_GOLD)
	{
		/* Gold earned based on value of trade */
		value = amt = trade_value;
	}
	
	/* Get the item (in the pack) */
	else if (item2 >= 0)
	{
		j_ptr = &inventory[item2];
		
		/* Get value */
		value = object_value_real(j_ptr);

		/* Get max quantity */
		if (value)
		{
			max = trade_value / value;
		}
		else
		{
			max = 99;
		}
		
		/* Get a quantity */
		amt = get_quantity(NULL, MIN(j_ptr->number, max));
	}

	/* Get the item (on the floor) */
	else
	{
		j_ptr = &o_list[0 - item2];
		
		/* Get value */
		value = object_value_real(j_ptr);

		/* Get max quantity */
		if (value)
		{
			max = trade_value / value;
		}
		else
		{
			max = 99;
		}
		
		/* Get a quantity */
		amt = get_quantity(NULL, MIN(j_ptr->number, max));
	}

	/* Allow user abort */
	if (amt <= 0) return (FALSE);
	
	/* Get the monster name */
	monster_desc(m_name, sizeof(m_name), trade_m_idx, 0);
	
	/* Gifting the monster money */
	if ((item2 == INVEN_GOLD) && (item == INVEN_GOLD))
	{
		/* Will be giving services back later */
		value = 0;
	}

	/* Buying an item from the monster */
	else if (item2 == INVEN_GOLD)
	{
		/* Evil monsters betray the player 66% of the time */
		if (((r_info[m_ptr->r_idx].flags3 & (RF3_EVIL)) != 0) && (rand_int(3))) value = 0;

		/* In town, we can sell stuff */
		if ((room_near_has_flag(m_ptr->fy, m_ptr->fx, ROOM_TOWN)) && !(adult_no_selling))
		{
			/* Money well earned? */
			p_ptr->au += value;
			
			/* Update display */
			p_ptr->redraw |= (PR_GOLD);
		}
		else
		{
			/* In the dungeon, monsters don't have gold on them 'quite yet' */
			value = 0;
			
			/* If trying to sell stuff, the player is a sucker. */
			trade_value /= 2;
		}
	}

	/* Give up the item promised */
	if (item == INVEN_GOLD)
	{
		/* Money well spent? */
		if (p_ptr->au >= trade_amount) p_ptr->au -= trade_amount;
		
		/* Paranoia */
		else (p_ptr->au = 0);
		
		/* Update display */
		p_ptr->redraw |= (PR_GOLD);
	}
	else
	{
		/* Take off (some of) the item */
		inven_takeoff(item, trade_amount);
	}
	
	/* Cheated the player out of money */
	if ((!adult_gollum) && (item2 == INVEN_GOLD) && (item != INVEN_GOLD) && (!value) && ((m_ptr->mflag & (MFLAG_TOWN)) != 0))
	{
		/* Really ripping the player off */
		if (level_flag & (LF1_TOWN)) monster_speech(trade_m_idx, comment_2[rand_int(MAX_COMMENT_2)], FALSE);

		/* Otherwise it's just a day in the life of ripping the player off */
		else monster_speech(trade_m_idx, comment_3[rand_int(MAX_COMMENT_3)], FALSE);
	}

	/* The monster made some profit - let's do business */
	else
	{
		int level = MAX(r_info[m_ptr->r_idx].level, p_ptr->depth);
		int deal = ((trade_value - value) * 10 / ((level + 5) * (level + 5) * (level + 5))) + trade_highly_value;
		int hire = 150 - adj_chr_gold[p_ptr->stat_ind[A_CHR]];
		
		/*
		 * Deal boosters - as a player trades further with a monster, they have a small chance of getting a deal boosted
		 * to a higher level of effectiveness.
		 */
		if (deal >= 1)
		{
			/* Boost a deal */
			while (!rand_int((m_ptr->mflag & (MFLAG_ALLY)) ? 3 : 4)) deal++;
		}
		
		/* Gollums always make a minimum deal */
		if ((adult_gollum) && (deal < 2)) deal = 2;

		/* We can offer stuff to business associates to make them allies */
		if ((deal > 4) && ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN | MFLAG_MADE | MFLAG_AGGR)) == (MFLAG_TOWN | MFLAG_MADE)))
		{
			/* Make them an ally */
			m_ptr->mflag |= (MFLAG_ALLY);

			/* Clear old targets */
			m_ptr->ty = 0;
			m_ptr->tx = 0;

			/* Buy more time before they turn on us */
			m_ptr->summoned = 400 - 3 * adj_chr_gold[p_ptr->stat_ind[A_CHR]];

			/* Get moderately excited. */
			monster_speech(trade_m_idx, comment_1e[rand_int(MAX_COMMENT_1e)], FALSE);
		}

		/* We can offer stuff to neutral monsters to reveal their inventory */
		else if ((deal > 2) && ((m_ptr->mflag & (MFLAG_MADE | MFLAG_TOWN | MFLAG_AGGR)) == (MFLAG_TOWN))
				&& (monster_drop(trade_m_idx)))
		{
			/* Get quietly excited. */
			monster_speech(trade_m_idx, comment_1d[rand_int(MAX_COMMENT_1d)], FALSE);

			/* Get more respect from trading */
			m_ptr->summoned = 300 - 2 * adj_chr_gold[p_ptr->stat_ind[A_CHR]];
		}

		/* We get told useful information from bigger deals, if we're not actively fighting */
		else if ((deal > 3) && (player_understands(monster_language(trade_m_idx)))
				 && ((m_ptr->mflag & (MFLAG_TOWN | MFLAG_AGGR)) == (MFLAG_TOWN)))
		{
			/* Possible reveals */
			switch(rand_int(deal - 3))
			{
				/* Tells the player about home */
				case 3: case 8: case 10:
				{
					/* Only if we trust the player */
					if (((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN | MFLAG_MADE | MFLAG_AGGR))
							== (MFLAG_ALLY | MFLAG_TOWN | MFLAG_MADE))
							&& (map_home(trade_m_idx)))
					{
						monster_speech(trade_m_idx, "You should see the rest of my home.", FALSE);
						break;
					}
				}

				/* Tells the player about the most powerful monster in the ecology */
				case 2: case 7:
				{
					/* Do we have a deepest race? */
					if (room_info[room_idx_ignore_valid(m_ptr->fy, m_ptr->fx)].deepest_race)
					{
						monster_speech(trade_m_idx, "You are right to be cautious around here.", FALSE);

						lore_do_probe(room_info[room_idx_ignore_valid(m_ptr->fy, m_ptr->fx)].deepest_race);

						break;
					}
				}

				/* Tells the player about the local region. */
				case 1: case 5:
				{
					int i;
					u32b ecology = 0L;

					/* Check to see if monster is local to this region */
					if (cave_ecology.ready) for (i = 0; i < cave_ecology.num_races; i++)
					{
						/* Get the ecology membership */
						if (cave_ecology.race[i] == m_ptr->r_idx) ecology |= cave_ecology.race_ecologies[i];
					}

					/* Do we know this area? */
					if ((!cave_ecology.ready) || ((room_info[room_idx_ignore_valid(m_ptr->fy, m_ptr->fx)].ecology & (ecology)) != 0))
					{
						monster_speech(trade_m_idx, "This is an especially interesting area.", FALSE);
						map_area();
					}
					/* At least tell the player we're not local */
					else
					{
						monster_speech(trade_m_idx, "I don't know this area.", FALSE);
					}

					break;
				}

				/* Tell the player about yourself */
				default:
				{
					monster_speech(trade_m_idx, "You may not know everything about my family history.", FALSE);
					lore_do_probe(m_ptr->r_idx);
					break;
				}
			}

			/* Buy more time before they turn on us */
			m_ptr->summoned = 400 - 3 * adj_chr_gold[p_ptr->stat_ind[A_CHR]];
		}

		/* We can offer stuff to monsters to either slow them down or make them 'neutral'. */
		/* Gollums are wretched enough to make anything sound moderately attractive */
		else if ((deal > 0) && (((m_ptr->mflag & (MFLAG_TOWN)) == 0) || ((m_ptr->mflag & (MFLAG_AGGR)) != 0)))
		{
			/* Enough to at least make him not attack. */
			if (deal > 1)
			{
				m_ptr->mflag |= (MFLAG_TOWN);

				/* Buy more time before they turn on us */
				m_ptr->summoned = 70 - adj_chr_gold[p_ptr->stat_ind[A_CHR]] / 2;

				/* Handle aggressive response */
				if (m_ptr->mflag & (MFLAG_AGGR)) deal = 1;
			}

			/* At least block some other monsters */
			if (deal % 2) m_ptr->mflag |= (MFLAG_PUSH);

			/* Don't register much excitement. */
			/* But make sure harmless monsters don't threaten the player */
			monster_speech(trade_m_idx, (deal > 1) ?
					((r_info[m_ptr->r_idx].blow[0].effect == GF_NOTHING) ? comment_1a[rand_int(MAX_COMMENT_1a)] :
					comment_1c[rand_int(MAX_COMMENT_1c)]) : comment_1b[rand_int(MAX_COMMENT_1b)], FALSE);
		}
		else
		{
			/* Don't register much excitement */
			monster_speech(trade_m_idx, comment_1a[rand_int(MAX_COMMENT_1a)], FALSE);
		}

		/* We made a deal - minimal extension */
		if (deal)
		{
			/* Get grudging respect */
			if (m_ptr->summoned < hire) m_ptr->summoned = hire;
		}

		/* For valuable deals, we talk to our allies and assess the situation */
		if (deal >= 3)
		{
			/* If we don't think the player is listening */
			u32b hack_cur_flags4 = p_ptr->cur_flags4;

			/* Monster assumes based on the player shape what language they understand. */
			p_ptr->cur_flags4 = k_info[p_info[p_ptr->pshape].innate].flags4;

			/* Appear more innocuous */
			if (player_understands(monster_language(m_ptr->r_idx)))
			{
				/* Unhack */
				p_ptr->cur_flags4 = hack_cur_flags4;

				/* Tell allies to cool their heels */
				if (tell_allies_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), comment_4a[rand_int(MAX_COMMENT_4a)]))
				{
					/* Allies are impressed to varying degree - evil speakers are silver tongued */
					tell_allies_summoned(m_ptr->fy, m_ptr->fx, MIN_TOWN_WARNING + rand_int(m_ptr->summoned / 2 + 1),
							r_info[m_ptr->r_idx].flags3 & (RF3_EVIL) ? comment_4b[rand_int(MAX_COMMENT_4b)] : comment_4c[rand_int(MAX_COMMENT_4c)]);
				}
			}

			/* Appear more haughty, Outright betrayal, if evil */
			else
			{
				/* Unhack */
				p_ptr->cur_flags4 = hack_cur_flags4;

				/* If smart, we try to talk in common language to trick the player */
				if (r_info[m_ptr->r_idx].flags2 & (RF2_SMART)) monster_speech(trade_m_idx, comment_5a[rand_int(MAX_COMMENT_5a)], TRUE);

				/* Tell allies to cool their heels */
				if (tell_allies_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), comment_5a[rand_int(MAX_COMMENT_5a)]))
				{
					/* If evil, we schedule a random time to coordinate a quick betrayal */
					if (r_info[m_ptr->r_idx].flags3 & (RF3_EVIL)) m_ptr->summoned = rand_int(MIN_TOWN_WARNING);

					/* Coordinate a quick betrayal if evil, or risk of outright attack if allies sufficiently unimpressed */
					tell_allies_summoned(m_ptr->fy, m_ptr->fx,
							r_info[m_ptr->r_idx].flags3 & (RF3_EVIL) ? m_ptr->summoned : MIN_TOWN_WARNING / 2 + rand_int(m_ptr->summoned / 2 + 1) / 2,
									r_info[m_ptr->r_idx].flags3 & (RF3_EVIL) ? comment_5b[rand_int(MAX_COMMENT_5b)] : comment_5c[rand_int(MAX_COMMENT_5c)]);
				}
			}
		}
	}

	/* Prevent GTA style takedowns - or the player paying off people to leave town too easily */
	if (((level_flag & (LF1_TOWN)) != 0) && (item2 == INVEN_GOLD) && (item != INVEN_GOLD) && !(adult_no_selling) && (trade_value >= 100 + p_ptr->depth * p_ptr->depth * 10))
	{
		int y = m_ptr->fy;
		int x = m_ptr->fx;
		
		/* "Kill" the monster */
		delete_monster_idx(trade_m_idx);

		/* Give detailed messages if destroyed */
		msg_format("%^s leaves town.", m_name);

		/* Redraw the monster grid */
		lite_spot(y, x);
	}
	
	/* Take a turn */
	p_ptr->energy_use = 100;

	return (TRUE);
}


/*
 * Destroy an item
 */
bool player_destroy(int item)
{
	int amt;
	int old_number;

	object_type *o_ptr;

	char o_name[80];

	char out_val[160];

	bool get_feat = FALSE;

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

	/* Get feat */
	if (o_ptr->ident & (IDENT_STORE)) get_feat = TRUE;

	/* Get a quantity */
	amt = get_quantity(NULL, o_ptr->number);

	/* Allow user abort */
	if (amt <= 0) return (FALSE);

	/* Describe the object */
	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
	o_ptr->number = old_number;

	/* Verify destruction */
	if (!auto_pickup_ignore(o_ptr))
	{
		sprintf(out_val, "Really destroy %s? ", o_name);
		if (!get_check(out_val)) return (FALSE);
	}

	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Containers release contents */
	if ((o_ptr->tval == TV_HOLD) && (o_ptr->name3 > 0))
	{
		if (animate_object(item)) return (TRUE);

		/* Message */
		msg_format("You cannot destroy %s.", o_name);

		return (TRUE);
	}

	/* Artifacts cannot be destroyed */
	if (artifact_p(o_ptr))
	{

		/* Message */
		msg_format("You cannot destroy %s.", o_name);

		/* Sense the object if allowed, don't sense ID'ed stuff */
		if ((o_ptr->discount == 0)
		&& !(o_ptr->ident & (IDENT_SENSE))
		 && !(object_named_p(o_ptr)))
		{
			o_ptr->discount = INSCRIP_UNBREAKABLE;

			/* The object has been "sensed" */
			o_ptr->ident |= (IDENT_SENSE);

			/* Combine the pack */
			p_ptr->notice |= (PN_COMBINE);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

		}

		/* Done */
		return (TRUE);

	}

	/* Cursed equipments cannot be destroyed */
	if ((item >= INVEN_WIELD) && cursed_p(o_ptr))
	{
		/* Message */
		msg_format("The %s you are %s appears to be cursed.",
		           o_name, describe_use(item));

		mark_cursed_feeling(o_ptr);

		/* Done */
		return (TRUE);
	}

	/* Message */
	msg_format("You destroy %s.", o_name);

	/* Sometimes use lower stack object */
	if (!object_charges_p(o_ptr) && (rand_int(o_ptr->number)< o_ptr->stackc))
	{
		if (amt >= o_ptr->stackc)
		{
			o_ptr->stackc = 0;
		}
		else
		{
			o_ptr->stackc -= amt;
		}
	}

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		if (o_ptr->number == amt) inven_drop_flags(o_ptr);

		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}
	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
		if (get_feat && (scan_feat(p_ptr->py,p_ptr->px) < 0)) cave_alter_feat(p_ptr->py,p_ptr->px,FS_GET_FEAT);
	}

	return (TRUE);
}


/*
 * Observe an item which has been *identify*-ed
 */
bool player_observe(int item)
{
	object_type *o_ptr;

	char o_name[80];

	bool stored = FALSE;

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

	/* Hack - obviously interested enough in item */
	if (o_ptr->ident & (IDENT_STORE))
	{
		if (item < 0) o_ptr->ident |= (IDENT_MARKED);

		/* No longer 'stored' */
		o_ptr->ident &= ~(IDENT_STORE);

		stored = TRUE;
	}

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	msg_format("Examining %s...", o_name);

	msg_print("");

	/* Save the screen */
	screen_save();

	/* Describe part of the player */
	if ((item >= 0) && (stored)) screen_self_object(o_ptr, item);

	/* Describe */
	else screen_object(o_ptr);

	(void)anykey();

	/* Store item again */
	if (stored) o_ptr->ident |= (IDENT_STORE);

	/* Load the screen */
	screen_load();

	return (TRUE);
}



/*
 * Remove the inscription from an object
 * XXX Mention item (when done)?
 */
bool player_uninscribe(int item)
{
	object_type *o_ptr;

	int i;

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

	/* Nothing to remove */
	if (!o_ptr->note)
	{
		msg_print("That item had no inscription to remove.");
		return (FALSE);
	}

	/* Message */
	msg_print("Inscription removed.");

	/* Remove the incription */
	o_ptr->note = 0;

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Prompt to always inscribe? */
	if (!easy_autos) return (TRUE);

	/* Do we inscribe all these ego items? */
	if (object_named_p(o_ptr) && (o_ptr->name2) && (e_info[o_ptr->name2].note))
	{
		e_info[o_ptr->name2].note = 0;

		/* Process objects */
		for (i = 1; i < o_max; i++)
		{
			/* Get the object */
			object_type *i_ptr = &o_list[i];

			/* Skip dead objects */
			if (!i_ptr->k_idx) continue;

			/* Not matching ego item */
			if (i_ptr->name2 != o_ptr->name2) continue;

			/* Auto-inscribe */
			if (object_named_p(i_ptr) || adult_auto) i_ptr->note = 0;
		}
	}
	/* Do we inscribe all these object kinds? */
	else if (object_aware_p(o_ptr) && (k_info[o_ptr->k_idx].note))
	{
		k_info[o_ptr->k_idx].note = 0;

		/* Process objects */
		for (i = 1; i < o_max; i++)
		{
			/* Get the object */
			object_type *i_ptr = &o_list[i];

			/* Skip dead objects */
			if (!i_ptr->k_idx) continue;

			/* Not matching ego item */
			if (i_ptr->k_idx != o_ptr->k_idx) continue;

			/* Auto-inscribe */
			if (object_named_p(i_ptr) || adult_auto) i_ptr->note = 0;
		}
	}

	return (TRUE);
}


/*
 * Inscribe an object with a comment
 */
bool player_inscribe(int item)
{
	object_type *o_ptr;

	char o_name[80];

	char tmp[80];

	int i;

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

	/* Describe the activity */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Message */
	msg_format("Inscribing %s.", o_name);
	msg_print(NULL);

	/* Start with nothing */
	strcpy(tmp, "");

	/* Use old inscription */
	if (o_ptr->note)
	{
		/* Start with the old inscription */
		strnfmt(tmp, 80, "%s", quark_str(o_ptr->note));
	}

	/* Get a new inscription (possibly empty) */
	if (get_string("Inscription: ", tmp, 80))
	{
		/* Save the inscription */
		o_ptr->note = quark_add(tmp);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}

	/* Inscribe */
	if ((item < 0) && (auto_pickup_ignore(o_ptr))) o_ptr->ident &= ~(IDENT_MARKED);

	/* Prompt to always inscribe? */
	if (!easy_autos) return (TRUE);

	/* Do we inscribe all these ego items? */
	if (object_named_p(o_ptr) && (o_ptr->name2))
	{
		e_info[o_ptr->name2].note = o_ptr->note;

		/* Process objects */
		for (i = 1; i < o_max; i++)
		{
			/* Get the object */
			object_type *i_ptr = &o_list[i];

			/* Skip dead objects */
			if (!i_ptr->k_idx) continue;

			/* Not matching ego item */
			if (i_ptr->name2 != o_ptr->name2) continue;

			/* Already has inscription */
			if (i_ptr->note) continue;

			/* Auto-inscribe */
			if (object_named_p(i_ptr) || adult_auto) i_ptr->note = e_info[o_ptr->name2].note;

			/* Ignore */
			if (auto_pickup_ignore(o_ptr)) o_ptr->ident &= ~(IDENT_MARKED);

		}

	}
	/* Do we inscribe all these object kinds? */
	else if (object_aware_p(o_ptr))
	{
		k_info[o_ptr->k_idx].note = o_ptr->note;

		/* Process objects */
		for (i = 1; i < o_max; i++)
		{
			/* Get the object */
			object_type *i_ptr = &o_list[i];

			/* Skip dead objects */
			if (!i_ptr->k_idx) continue;

			/* Not matching ego item */
			if (i_ptr->k_idx != o_ptr->k_idx) continue;

			/* Already has inscription */
			if (i_ptr->note) continue;

			/* Auto-inscribe */
			if (object_named_p(i_ptr) || adult_auto) i_ptr->note = o_ptr->note;

			/* Ignore */
			if (auto_pickup_ignore(o_ptr)) o_ptr->ident &= ~(IDENT_MARKED);

		}
	}

	return (TRUE);
}

/*
 * An "item_tester_hook" for refilling lanterns
 */
bool item_tester_refill_lantern(const object_type *o_ptr)
{
	/* Flasks of oil are okay */
	if ((o_ptr->tval == TV_FLASK) && (o_ptr->sval == SV_FLASK_OIL)) return (TRUE);

	/* Non-empty lanterns are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_LANTERN) &&
	    (o_ptr->charges > 0))
	{
		return (TRUE);
	}

	/* Assume not okay */
	return (FALSE);
}


/*
 * An "item_tester_hook" for refilling torches
 */
bool item_tester_refill_torch(const object_type *o_ptr)
{
	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_TORCH)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}

/*
 * An "item_tester_hook" for empty flasks
 */
bool item_tester_empty_flask_or_lite(const object_type *o_ptr)
{
	/* Empty bottles/flasks are okay */
	if ((o_ptr->tval == TV_HOLD) &&
			((o_ptr->sval == SV_HOLD_BOTTLE) || (o_ptr->sval == SV_HOLD_FLASK))
		 && !(o_ptr->name3))
		return (TRUE);

	/* Lanterns are okay */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
		return (TRUE);

	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
		return (TRUE);

	/* Firearms are okay */
	if ((o_ptr->tval == TV_BOW) && (o_ptr->sval/10 == 3) && !(o_ptr->charges))
	{
		return(TRUE);
	}

	/* Assume not okay */
	return (FALSE);
}


/*
 * An "item_tester_hook" for refilling flasks
 */
bool item_tester_refill_flask(const object_type *o_ptr)
{
	/* Flasks are okay */
	if (o_ptr->tval == TV_FLASK)
		return (TRUE);

	/* Potions are okay */
	if (o_ptr->tval == TV_POTION)
		return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*
 * An "item_tester_hook" for refilling lanterns
 */
bool item_tester_refill_firearm(const object_type *o_ptr)
{
	/* Flasks of gunpowder are okay */
	if ((o_ptr->tval == TV_FLASK) && (o_ptr->sval == SV_FLASK_GUNPOWDER)) return (TRUE);

	/* Firearms are okay */
	if ((o_ptr->tval == TV_BOW) && (o_ptr->sval/10 == 3) && (o_ptr->charges))
	{
		return(TRUE);
	}

	/* Assume okay */
	return (FALSE);
}



/*
 * Determine whether fuel or fill command based on current item
 */
int cmd_tester_fill_or_fuel(int item)
{
	object_type *o_ptr;

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

	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
	{
		return(COMMAND_ITEM_FUEL_LAMP);
	}
	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		return(COMMAND_ITEM_FUEL_TORCH);
	}
	else if ((o_ptr->tval == TV_BOW) && (o_ptr->sval/10 == 3))
	{
		return(COMMAND_ITEM_FILL_FIREARM);
	}
	else
	{
		return(COMMAND_ITEM_FILL);
	}
}


/*
 * Fill a flask/torch/lantern
 */
bool player_refill(int item)
{
	/* Set up for second command */
	p_ptr->command_trans = COMMAND_ITEM_FUEL;
	p_ptr->command_trans_item = item;

	return (TRUE);
}


/*
 * Fill a flask/torch/lantern
 */
bool player_refill2(int item2)
{
	int item = p_ptr->command_trans_item;

	object_type *o_ptr;
	object_type *i_ptr;
	object_type *j_ptr;

	object_type object_type_body;

	bool unstack = FALSE;
	bool use_feat = FALSE;
	bool get_feat = FALSE;
	bool relite = FALSE;

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

	/* Get the item (in the pack) */
	if (item2 >= 0)
	{
		j_ptr = &inventory[item2];
	}

	/* Get the item (on the floor) */
	else
	{
		j_ptr = &o_list[0 - item2];
	}

	/* Can't fuel self */
	if (item == item2) return (FALSE);

	/* Take a partial turn */
	p_ptr->energy_use = 50;

	/* Switch off light source */
	if (o_ptr->timeout)
	{
		o_ptr->charges = o_ptr->timeout;
		o_ptr->timeout = 0;
		relite = TRUE;
	}

	/* Hack: if the light is worn, light it up afterwards, anyway */
	if (item == INVEN_LITE && !artifact_p(o_ptr))
		relite = TRUE;

	if (o_ptr->number > 1)
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Obtain a local object */
		object_copy(i_ptr, o_ptr);

		/* Modify quantity */
		i_ptr->number = 1;

		/* Reset stack counter */
		i_ptr->stackc = 0;

		/* Unstack the used item */
		o_ptr->number--;

		/* Adjust the weight */
		p_ptr->total_weight -= i_ptr->weight;

		o_ptr = i_ptr;

		unstack = TRUE;
	}

	/* Fuel torch from source */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
	{

		/* Message */
		if (unstack) msg_print("You unstack your lantern.");

		/* Refuel */
		o_ptr->charges += j_ptr->charges;

		/* Message */
		msg_print("You fuel the lamp.");

		/* Comment */
		if (o_ptr->charges >= FUEL_LAMP)
		{
			o_ptr->charges = FUEL_LAMP;
			msg_print("The lamp is full.");
		}

		/* Get feat */
		if (j_ptr->ident & (IDENT_STORE)) use_feat = TRUE;
	}
	/* Fuel lantern from source */
	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{

		/* Message */
		if (unstack) msg_print("You unstack your torch.");

		/* Refuel */
		o_ptr->charges += j_ptr->charges + 5;


		/* Over-fuel message */
		if (o_ptr->charges >= FUEL_TORCH)
		{
			o_ptr->charges = FUEL_TORCH;
			msg_print("Your torch is fully fueled.");
		}

		/* Refuel message */
		else if (item == INVEN_LITE)
		{
			msg_print("Your torch glows more brightly.");
		}

		/* Get feat */
		if (j_ptr->ident & (IDENT_STORE)) get_feat = TRUE;
	}
	/* Fill weapon from source */
	else if ((o_ptr->tval == TV_BOW) && (o_ptr->sval/10 == 3))
	{
		int max_charge = k_info[o_ptr->k_idx].charges;

		/* Message */
		if (unstack) msg_print("You unstack your weapon.");

		/* Refuel */
		o_ptr->charges++;

		/* Over-fuel message */
		if (o_ptr->charges >= max_charge)
		{
			o_ptr->charges = max_charge;

			msg_format("The chamber%s %s fully loaded.", max_charge > 1 ? "s" : "", max_charge > 1 ? "are" : "is");
		}

		/* Get feat */
		if (j_ptr->ident & (IDENT_STORE)) get_feat = TRUE;
	}
	/* Fill an empty container */
	else
	{
		/* Fill empty bottle or flask with the potion */
		object_copy(o_ptr, j_ptr);

		/* Modify quantity */
		o_ptr->number = 1;

		/* Reset stack counter */
		o_ptr->stackc = 0;

		/* Use feat */
		if (o_ptr->ident & (IDENT_STORE)) use_feat = TRUE;

		/* No longer 'stored' */
		o_ptr->ident &= ~(IDENT_STORE);

		/* Adjust the weight */
		p_ptr->total_weight += o_ptr->weight;
	}

	if (unstack)
	{
		/* Carry */
		item = inven_carry(o_ptr);

		/* Don't relite */
		relite = FALSE;
	}

	/* Use a charge of gunpowder */
	if ((o_ptr->tval == TV_BOW) && (o_ptr->sval/10 == 3) && (j_ptr->charges > 1))
	{
		/* XXX Hack -- new unstacking code */
		j_ptr->stackc++;

		/* No spare charges */
		if (j_ptr->stackc >= j_ptr->number)
		{
			/* Use a charge off the stack */
			j_ptr->charges--;

			/* Reset the stack count */
			j_ptr->stackc = 0;
		}

		/* Describe charges in the pack */
		if (item2 >= 0)
		{
			inven_item_charges(item2);
		}

		/* Describe charges on the floor */
		else
		{
			floor_item_charges(0 - item2);
		}

	}
	/* Use fuel from a lantern */
	else if ((j_ptr->tval == TV_LITE) && (j_ptr->sval == SV_LITE_LANTERN))
	{
		if (j_ptr->number > 1)
		{
			/* Get local object */
			i_ptr = &object_type_body;

			/* Obtain a local object */
			object_copy(i_ptr, j_ptr);

			/* Modify quantity */
			i_ptr->number = 1;

			/* Reset stack counter */
			i_ptr->stackc = 0;

			/* Reset the charges */
			i_ptr->charges = 0;

			/* No longer 'stored' */
			i_ptr->ident &= ~(IDENT_STORE);

			/* Unstack the used item; will be merge later */
			j_ptr->number--;

			/* Adjust the weight and carry */
			p_ptr->total_weight -= i_ptr->weight;
			item = inven_carry(i_ptr);
		}
		else
		{
			/* Use up last of fuel */
			j_ptr->charges = 0;
		}
	}
	/* Decrease the item (in the pack) */
	else if (item2 >= 0)
	{
		if (j_ptr->number == 1) inven_drop_flags(j_ptr);

		inven_item_increase(item2, -1);
		inven_item_describe(item2);
		inven_item_optimize(item2);
	}
	/* Decrease the item (from the floor) */
	else
	{
		floor_item_increase(0 - item2, -1);
		floor_item_describe(0 - item2);
		floor_item_optimize(0 - item2);
		if (get_feat && (scan_feat(p_ptr->py,p_ptr->px) < 0))
			cave_alter_feat(p_ptr->py,p_ptr->px,FS_GET_FEAT);
		if (use_feat && (scan_feat(p_ptr->py,p_ptr->px) < 0))
			cave_alter_feat(p_ptr->py,p_ptr->px,FS_USE_FEAT);
	}

	/* Relite if necessary */
	if (relite)
	{
		o_ptr->timeout = o_ptr->charges;
		o_ptr->charges = 0;
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Lite if necessary */
	if ((item == INVEN_LITE) || (item2 == INVEN_LITE))
		p_ptr->update |= (PU_TORCH);

	return (TRUE);
}


/*
 * An "item_tester_hook" for light sources
 */
bool item_tester_light_source(const object_type *o_ptr)
{
	/* Return "is a non-artifact light source" */
	return ((o_ptr->tval == TV_LITE) && !(artifact_p(o_ptr)));
}

/*
 * Light or douse a light source.
 */
bool player_light_and_douse(int item)
{
	object_type *o_ptr;
	char o_name[80];

	cptr own_str = "";

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

	/* Get an object description */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 1);

	/* Get an ownership string */
	if (item >= 0) own_str = "your";
	else if (o_ptr->number != 1) own_str = "some";
	else own_str = "the";

	/* Take a partial turn */
	p_ptr->energy_use = 50;

	/* Light the light source */
	if (!(o_ptr->timeout))
	{
		if (!(o_ptr->charges))
		{
			msg_format("You try to light %s %s but it splutters and goes out!", own_str, o_name);			
		}
		else {
			msg_format("You light %s %s.", own_str, o_name);
			o_ptr->timeout = o_ptr->charges;
			o_ptr->charges = 0;

			if (item < 0)
			{
				gain_attribute(p_ptr->py, p_ptr->px, 2, CAVE_XLOS, apply_halo, redraw_halo_gain);
			}
		}
	}

	/* Douse the light source */
	else
	{
		msg_format("You douse %s %s.", own_str, o_name);
		o_ptr->charges = o_ptr->timeout;
		o_ptr->timeout = 0;

		if (item < 0)
		{
			check_attribute_lost(p_ptr->py, p_ptr->px, 2, CAVE_XLOS, require_halo, has_halo, redraw_halo_loss, remove_halo, reapply_halo);
		}
	}

	/* Update torch */
	p_ptr->update |= (PU_TORCH);

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	return (TRUE);
}


/*
 * Target command
 */
void do_cmd_target(void)
{
	/* Target set */
	if (target_set_interactive(TARGET_KILL, 0, 0, 0, 0, 0))
	{
		msg_print("Target Selected.");
	}

	/* Target aborted */
	else
	{
		msg_print("Target Aborted.");
	}
}



/*
 * Look command
 */
void do_cmd_look(void)
{
	/* Look around */
	if (target_set_interactive(TARGET_LOOK, 0, 0, 0, 0, 0))
	{
		msg_print("Target Selected.");
	}
}


/*
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(void)
{
	int dir, y1, x1, y2, x2;

	char tmp_val[80];

	char out_val[160];


	/* Start at current panel */
	y2 = y1 = p_ptr->wy;
	x2 = x1 = p_ptr->wx;

	/* Show panels until done */
	while (1)
	{
		/* Describe the location */
		if ((y2 == y1) && (x2 == x1))
		{
			tmp_val[0] = '\0';
		}
		else
		{
			sprintf(tmp_val, "%s%s of",
			((y2 < y1) ? " North" : (y2 > y1) ? " South" : ""),
			((x2 < x1) ? " West" : (x2 > x1) ? " East" : ""));
		}

		/* Prepare to ask which way to look */
		sprintf(out_val,
		"Map sector [%d,%d], which is%s your sector.  Direction?",
		(y2 / PANEL_HGT), (x2 / PANEL_WID), tmp_val);

		/* More detail */
		if (center_player)
		{
			sprintf(out_val,
			"Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?",
			(y2 / PANEL_HGT), (y2 % PANEL_HGT),
			(x2 / PANEL_WID), (x2 % PANEL_WID), tmp_val);
		}

		/* Assume no direction */
		dir = 0;

		/* Get a direction */
		while (!dir)
		{
			char command;

			/* Get a command (or Cancel) */
			if (!get_com(out_val, &command)) break;

			/* Extract direction */
			dir = target_dir(command);

			/* Error */
			if (!dir) bell("Illegal direction for locate!");
		}

		/* No direction */
		if (!dir) break;

		/* Apply the motion */
		y2 += (ddy[dir] * PANEL_HGT);
		x2 += (ddx[dir] * PANEL_WID);

		/* Verify the row */
		if (y2 < 0) y2 = 0;
		if (y2 > DUNGEON_HGT - SCREEN_HGT) y2 = DUNGEON_HGT - SCREEN_HGT;

		/* Verify the col */
		if (x2 < 0) x2 = 0;
		if (x2 > DUNGEON_WID - SCREEN_WID) x2 = DUNGEON_WID - SCREEN_WID;

		/* Handle "changes" */
		if ((p_ptr->wy != y2) || (p_ptr->wx != x2))
		{
			/* Update panel */
			p_ptr->wy = y2;
			p_ptr->wx = x2;

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);

			/* Handle stuff */
			handle_stuff();
		}
	}

	/* Verify panel */
	p_ptr->update |= (PU_PANEL);

	/* Handle stuff */
	handle_stuff();
}






/*
 * The table of "symbol info" -- each entry is a string of the form
 * "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
static cptr ident_info[] =
{
	" :A dark grid",
	"!:A potion",
	"\":An amulet (or necklace)",
	"#:A wall, secret door or impassable terrain",
	"$:Treasure (gold or gems)",
	"%:A mineral vein, crack or vent",
	"&:A chest",
	"':An open door or cloud",
	"(:Soft armor",
	"):A shield",
	"*:A vein with treasure or interesting wall",
	"+:A closed/trapped door",
	",:Food (or mushroom patch)",
	"-:A wand (or rod)",
	".:Floor (or shallow water/oil)",
	"/:A polearm (Axe/Pike/etc)",
	"0:Fountain/well/altar/throne",
	"1:Entrance to General Store",
	"2:Entrance to Armory",
	"3:Entrance to Weaponsmith",
	"4:Entrance to Temple",
	"5:Entrance to Alchemy shop",
	"6:Entrance to Magic store",
	"7:Entrance to Black Market",
	"8:Entrance to your home",
	"9:Statue",
	"::Trees, rubble or deep water/lava/acid etc",
	";:Plants, broken floor or shallow lava/acid/mud etc",
	"<:An up staircase or falls",
	"=:A ring",
	">:A down staircase or chasm",
	"?:A scroll",
	"@:You",
	"A:Ancient Dragon/Wyrm",
	"B:Bird",
	"C:Canine",
	"D:mature Dragons",
	"E:Elemental",
	"F:Fish/Amphibian",
	"G:Ghost",
	"H:Hybrid",
	"I:Insect",
	"J:Snake",
	"K:Killer Beetle",
	"L:ghouls/Liches",
	"M:Maiar",
	"N:Nightsbane",
	"O:Ogre",
	"P:Giant",
	"Q:Quadrepeds",
	"R:Reptile",
	"S:Spider/Scorpion/Tick",
	"T:Troll",
	"U:Major Demon",
	"V:Vampire",
	"W:Wight/Wraith",
	"X:Xorn/Xaren/etc",
	"Y:apes/Yeti",
	"Z:Zephyr Hound",
	"[:Hard armor",
	"\\:A hafted weapon (mace/whip/etc)",
	"]:Misc. armor",
	"^:A trap",
	"_:A staff",
	/* "`:unused", */
	"a:Ant",
	"b:Bat",
	"c:Centipede",
	"d:Young Dragon",
	"e:Floating Eye",
	"f:Feline",
	"g:Golem",
	"h:Hobbit/Dwarf",
	"i:Icky Thing",
	"j:Jelly",
	"k:Goblin",
	"l:Elf",
	"m:Mold",
	"n:Naga",
	"o:Orc",
	"p:Priest",
	"q:Mage",
	"r:Rodent",
	"s:Skeleton",
	"t:Thief/Warrior",
	"u:Minor Demon",
	"v:Vortex",
	"w:Worm/Worm-Mass",
	"x:A bridge or unknown grid",
	"y:Hydra",
	"z:Zombie/Mummy",
	"{:A missile (arrow/bolt/shot)",
	"|:An edged weapon (sword/dagger/etc)",
	"}:A missle launcher (bow/crossbow/sling)",
	"~:A tool or miscellaneous item",
	NULL
};



/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_comp_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b *why = (u16b*)(v);

	int w1 = who[a];
	int w2 = who[b];

	int z1, z2;


	/* Sort by player kills */
	if (*why >= 4)
	{
		/* Extract player kills */
		z1 = l_list[w1].pkills;
		z2 = l_list[w2].pkills;

		/* Compare player kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by total kills */
	if (*why >= 3)
	{
		/* Extract total kills */
		z1 = l_list[w1].tkills;
		z2 = l_list[w2].tkills;

		/* Compare total kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}

	/* Sort by monster level */
	if (*why >= 2)
	{
		/* Extract levels */
		z1 = r_info[w1].level;
		z2 = r_info[w2].level;

		/* Compare levels */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}

	/* Sort by monster experience */
	if (*why >= 1)
	{
		/* Extract experience */
		z1 = r_info[w1].mexp;
		z2 = r_info[w2].mexp;

		/* Compare experience */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}

	/* Compare indexes */
	return (w1 <= w2);
}


/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_swap_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b holder;

	(void)v;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}



/*
 * Hack -- Display the "name" and "attr/chars" of a monster race
 */
static void roff_top(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	char m_name[80];
	
	byte a1, a2;
	char c1, c2;


	/* Get the chars */
	c1 = r_ptr->d_char;
	c2 = r_ptr->x_char;

	/* Get the attrs */
	a1 = r_ptr->d_attr;
	a2 = r_ptr->x_attr;


	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

	/* Get the name */
	race_desc(m_name, sizeof(m_name), r_idx, 0x400, 1);
	
	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, m_name);

	if (!use_dbltile && !use_trptile)
	{
		/* Append the "standard" attr/char info */
		Term_addstr(-1, TERM_WHITE, " ('");
		Term_addch(a1, c1);
		Term_addstr(-1, TERM_WHITE, "')");

		/* Append the "optional" attr/char info */
		Term_addstr(-1, TERM_WHITE, "/('");
		Term_addch(a2, c2);
		if (use_bigtile && (a2 & 0x80)) Term_addch(255, -1);
		Term_addstr(-1, TERM_WHITE, "'):");
	}
}


/*
 * Identify a character, allow recall of monsters
 *
 * Several "special" responses recall "mulitple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 * Note that the player ghosts are ignored, since they do not exist.
 */
void do_cmd_query_symbol(void)
{
	int i, n, r_idx;
	char sym, query;
	char buf[128];

	bool all = FALSE;
	bool uniq = FALSE;
	bool norm = FALSE;

	bool recall = FALSE;

	u16b why = 0;
	u16b *who;


	/* Get a character, or abort */
	if (!get_com("Enter character to be identified: ", &sym)) return;

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

	/* Describe */
	if (sym == KTRL('A'))
	{
		all = TRUE;
		strcpy(buf, "Full monster list.");
	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
		strcpy(buf, "Unique monster list.");
	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
		strcpy(buf, "Non-unique monster list.");
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
		sprintf(buf, "%c - %s.", sym, "Unknown Symbol");
	}

	/* Display the result */
	prt(buf, 0, 0);


	/* Allocate the "who" array */
	who = C_ZNEW(z_info->r_max, u16b);

	/* Collect matching monsters */
	for (n = 0, i = 1; i < z_info->r_max - 1; i++)
	{
		monster_race *r_ptr = &r_info[i];
		monster_lore *l_ptr = &l_list[i];

		/* Nothing to recall */
		if (!cheat_know && !l_ptr->sights) continue;

		/* Require non-unique monsters if needed */
		if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Require unique monsters if needed */
		if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Collect "appropriate" monsters */
		if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		/* XXX XXX Free the "who" array */
		FREE(who);

		return;
	}


	/* Prompt */
	put_str("Recall details? (k/p/y/n): ", 0, 40);

	/* Query */
	query = anykey().key;

	/* Restore */
	prt(buf, 0, 0);


	/* Sort by kills (and level) */
	if (query == 'k')
	{
		why = 4;
		query = 'y';
	}

	/* Sort by level */
	if (query == 'p')
	{
		why = 2;
		query = 'y';
	}

	/* Catch "escape" */
	if (query != 'y')
	{
		/* XXX XXX Free the "who" array */
		FREE(who);

		return;
	}

	/* Sort if needed */
	if (why)
	{
		/* Select the sort method */
		ang_sort_comp = ang_sort_comp_hook;
		ang_sort_swap = ang_sort_swap_hook;

		/* Sort the array */
		ang_sort(who, &why, n);
	}


	/* Start at the end */
	i = n - 1;

	/* Scan the monster memory */
	while (1)
	{
		/* Extract a race */
		r_idx = who[i];

		/* Hack -- Auto-recall */
		monster_race_track(r_idx);

		/* Hack -- Handle stuff */
		handle_stuff();

		/* Hack -- Begin the prompt */
		roff_top(r_idx);

		/* Hack -- Complete the prompt */
		Term_addstr(-1, TERM_WHITE, " [(r)ecall, ESC]");

		/* Interact */
		while (1)
		{
			/* Recall */
			if (recall)
			{
				/* Save screen */
				screen_save();

				/* Recall on screen */
				screen_roff(who[i], &l_list[who[i]]);

				/* Hack -- Complete the prompt (again) */
				Term_addstr(-1, TERM_WHITE, " [(r)ecall, ESC]");
			}

			/* Command */
			query = anykey().key;

			/* Unrecall */
			if (recall)
			{
				/* Load screen */
				screen_load();
			}

			/* Normal commands */
			if (query != 'r') break;

			/* Toggle recall */
			recall = !recall;
		}

		/* Stop scanning */
		if (query == ESCAPE) break;

		/* Move to "prev" monster */
		if (query == '-')
		{
			if (++i == n)
			{
				i = 0;
			}
		}

		/* Move to "next" monster */
		else
		{
			if (i-- == 0)
			{
				i = n - 1;
			}
		}
	}


	/* Re-display the identity */
	prt(buf, 0, 0);

	/* Free the "who" array */
	FREE(who);
}


/*
 * Display the main-screen monster list.
 */
void do_cmd_monlist(void)
{
	/* Hack -- cycling monster list sorting */
	if (auto_monlist)
	{
		if (!op_ptr->monlist_sort_by)
		{
			op_ptr->monlist_sort_by = 2;
			if (op_ptr->monlist_display == 3) op_ptr->monlist_display = 1;
			else op_ptr->monlist_display = 3;
		}
		else op_ptr->monlist_sort_by--;

		p_ptr->window |= (PW_MONLIST | PW_ITEMLIST);

		window_stuff();
	}

	/* Display the monster list */
	display_monlist(0, 0, op_ptr->monlist_display, TRUE, TRUE);

	/* Hack -- cycling monster list sorting */
	if ((!auto_monlist) && (p_ptr->command_new.key == '['))
	{
		if (!op_ptr->monlist_sort_by)
		{
			op_ptr->monlist_sort_by = 2;
			if (op_ptr->monlist_display == 3) op_ptr->monlist_display = 1;
			else op_ptr->monlist_display = 3;
		}
		else op_ptr->monlist_sort_by--;

		p_ptr->window |= (PW_MONLIST | PW_ITEMLIST);

		window_stuff();
	}
}

/* Centers the map on the player */
void do_cmd_center_map(void)
{
	center_panel();
}
