
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



#define MAX_COMMENT_1   6

static cptr comment_1[MAX_COMMENT_1] =
{
	"Okay.",
	"Fine.",
	"Accepted!",
	"Agreed!",
	"Done!",
	"Taken!"
};

#define MAX_COMMENT_2A  2

static cptr comment_2a[MAX_COMMENT_2A] =
{
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
};

#define MAX_COMMENT_2B  12

static cptr comment_2b[MAX_COMMENT_2B] =
{
	"I can take no less than %s gold pieces.",
	"I will accept no less than %s gold pieces.",
	"Ha!  No less than %s gold pieces.",
	"You knave!  No less than %s gold pieces.",
	"That's a pittance!  I want %s gold pieces.",
	"That's an insult!  I want %s gold pieces.",
	"As if!  How about %s gold pieces?",
	"My arse!  How about %s gold pieces?",
	"May the fleas of 1000 orcs molest you!  Try %s gold pieces.",
	"May your most favourite parts go moldy!  Try %s gold pieces.",
	"May Morgoth find you tasty!  Perhaps %s gold pieces?",
	"Your mother was an Ogre!  Perhaps %s gold pieces?"
};

#define MAX_COMMENT_3A  2

static cptr comment_3a[MAX_COMMENT_3A] =
{
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
};


#define MAX_COMMENT_3B  12

static cptr comment_3b[MAX_COMMENT_3B] =
{
	"Perhaps %s gold pieces?",
	"How about %s gold pieces?",
	"I will pay no more than %s gold pieces.",
	"I can afford no more than %s gold pieces.",
	"Be reasonable.  How about %s gold pieces?",
	"I'll buy it as scrap for %s gold pieces.",
	"That is too much!  How about %s gold pieces?",
	"That looks war surplus!  Say %s gold pieces?",
	"Never!  %s is more like it.",
	"That's an insult!  %s is more like it.",
	"%s gold pieces and be thankful for it!",
	"%s gold pieces and not a copper more!"
};

#define MAX_COMMENT_4A  4

static cptr comment_4a[MAX_COMMENT_4A] =
{
	"Enough!  You have abused me once too often!",
	"Arghhh!  I have had enough abuse for one day!",
	"That does it!  You shall waste my time no more!",
	"This is getting nowhere!  I'm going to Londis!"
};

#define MAX_COMMENT_4B  4

static cptr comment_4b[MAX_COMMENT_4B] =
{
	"Leave my store!",
	"Get out of my sight!",
	"Begone, you scoundrel!",
	"Out, out, out!"
};

#define MAX_COMMENT_5   8

static cptr comment_5[MAX_COMMENT_5] =
{
	"Try again.",
	"Ridiculous!",
	"You will have to do better than that!",
	"Do you wish to do business or not?",
	"You've got to be kidding!",
	"You'd better be kidding!",
	"You try my patience.",
	"Hmmm, nice weather we're having."
};

#define MAX_COMMENT_6   4

static cptr comment_6[MAX_COMMENT_6] =
{
	"I must have heard you wrong.",
	"I'm sorry, I missed that.",
	"I'm sorry, what was that?",
	"Sorry, what was that again?"
};



/*
 * Successful haggle.
 */
static void say_comment_1(void)
{
	msg_print(comment_1[rand_int(MAX_COMMENT_1)]);
}


/*
 * Continue haggling (player is buying)
 */
static void say_comment_2(s32b value, int annoyed)
{
	char tmp_val[80];

	/* Prepare a string to insert */
	sprintf(tmp_val, "%ld", (long)value);

	/* Final offer */
	if (annoyed > 0)
	{
		/* Formatted message */
		msg_format(comment_2a[rand_int(MAX_COMMENT_2A)], tmp_val);
	}

	/* Normal offer */
	else
	{
		/* Formatted message */
		msg_format(comment_2b[rand_int(MAX_COMMENT_2B)], tmp_val);
	}

	/* Pause */
	anykey();
}


/*
 * Continue haggling (player is selling)
 */
static void say_comment_3(s32b value, int annoyed)
{
	char tmp_val[80];

	/* Prepare a string to insert */
	sprintf(tmp_val, "%ld", (long)value);

	/* Final offer */
	if (annoyed > 0)
	{
		/* Formatted message */
		msg_format(comment_3a[rand_int(MAX_COMMENT_3A)], tmp_val);
	}

	/* Normal offer */
	else
	{
		/* Formatted message */
		msg_format(comment_3b[rand_int(MAX_COMMENT_3B)], tmp_val);
	}

    /* Pause */
    anykey();
}


/*
 * You must leave my store
 */
static void say_comment_4(void)
{
	msg_print(comment_4a[rand_int(MAX_COMMENT_4A)]);
	msg_print(comment_4b[rand_int(MAX_COMMENT_4B)]);

    /* Pause */
    anykey();
}


/*
 * You are insulting me
 */
static void say_comment_5(void)
{
	msg_print(comment_5[rand_int(MAX_COMMENT_5)]);
}


/*
 * You are making no sense.
 */
static void say_comment_6(void)
{
	msg_print(comment_6[rand_int(MAX_COMMENT_6)]);

    /* Pause */
    anykey();
}



/*
 * Messages for reacting to purchase prices.
 */

#define MAX_COMMENT_7A  4

static cptr comment_7a[MAX_COMMENT_7A] =
{
	"Arrgghh!",
	"You bastard!",
	"You hear someone sobbing...",
	"The shopkeeper howls in agony!"
};

#define MAX_COMMENT_7B  4

static cptr comment_7b[MAX_COMMENT_7B] =
{
	"Damn!",
	"You fiend!",
	"The shopkeeper curses at you.",
	"The shopkeeper glares at you."
};

#define MAX_COMMENT_7C  4

static cptr comment_7c[MAX_COMMENT_7C] =
{
	"Cool!",
	"You've made my day!",
	"The shopkeeper giggles.",
	"The shopkeeper laughs loudly."
};

#define MAX_COMMENT_7D  4

static cptr comment_7d[MAX_COMMENT_7D] =
{
	"Yipee!",
	"I think I'll retire!",
	"The shopkeeper jumps for joy.",
	"The shopkeeper smiles gleefully."
};


/*
 * Let a shop-keeper React to a purchase
 *
 * We paid "price", it was worth "value", and we thought it was worth "guess"
 */
static void purchase_analyze(s32b price, s32b value, s32b guess)
{
	/* Item was worthless, but we bought it */
	if ((value <= 0) && (price > value))
	{
		/* Comment */
		message(MSG_STORE1, 0, comment_7a[rand_int(MAX_COMMENT_7A)]);
	}

	/* Item was cheaper than we thought, and we paid more than necessary */
	else if ((value < guess) && (price > value))
	{
		/* Comment */
		message(MSG_STORE2, 0, comment_7b[rand_int(MAX_COMMENT_7B)]);
	}

	/* Item was a good bargain, and we got away with it */
	else if ((value > guess) && (value < (4 * guess)) && (price < value))
	{
		/* Comment */
		message(MSG_STORE3, 0, comment_7c[rand_int(MAX_COMMENT_7C)]);
	}

	/* Item was a great bargain, and we got away with it */
	else if ((value > guess) && (price < value))
	{
		/* Comment */
		message(MSG_STORE4, 0, comment_7d[rand_int(MAX_COMMENT_7D)]);
	}
}


/*
 * We store the current "store page" top most item here so everyone can access it
 */
static int store_top = 0;

/*
 * We store the current "store page" size here so everyone can access it
 */
static int store_size = 12;


/*
 * Determine the price of an object (qty one) in a store.
 *
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 *
 * The "greed" value should exceed 100 when the player is "buying" the
 * object, and should be less than 100 when the player is "selling" it.
 *
 * Hack -- the black market always charges twice as much as it should.
 *
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 *
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 */
static s32b price_item(object_type *o_ptr, int greed, bool flip, int store_index)
{
	int factor;
	int adjust;
	s32b price;

	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	/* Get the value of one of the items */
	price = object_value(o_ptr);

	/* Worthless items */
	if (price <= 0) return (0L);

	/* Compute the racial factor */
	factor = g_info[(ot_ptr->owner_race * z_info->g_max) + p_ptr->pshape];

	/* Add in the charisma factor */
	factor += adj_chr_gold[p_ptr->stat_ind[A_CHR]];


	/* Shop is buying */
	if (flip)
	{
		/* Adjust for greed */
		adjust = 100 + (300 - (greed + factor));

		/* Never get "silly" */
		if (adjust > 100) adjust = 100;

		/* Mega-Hack -- Black market sucks */
		if (st_ptr->base == STORE_B_MARKET) price = price / 2;
	}

	/* Shop is selling */
	else
	{
		/* Adjust for greed */
		adjust = 100 + ((greed + factor) - 300);

		/* Never get "silly" */
		if (adjust < 100) adjust = 100;

		/* Mega-Hack -- Black market sucks */
		if (st_ptr->base == STORE_B_MARKET) price = price * 2;
	}

	/* Compute the final price (with rounding) */
	price = (price * adjust + 50L) / 100L;

	/* Note -- Never become "free" */
	if (price <= 0L) return (1L);

	/* Return the price */
	return (price);
}


/*
 * Special "mass production" computation.
 */
static int mass_roll(int num, int max)
{
	int i, t = 0;
	for (i = 0; i < num; i++)
	{
		t += ((max > 1) ? rand_int(max) : 1);
	}
	return (t);
}


/*
 * Certain "cheap" objects should be created in "piles".
 *
 * Some objects can be sold at a "discount" (in smaller piles).
 *
 * Standard percentage discounts include 10, 25, 50, 75, and 90.
 */
static void mass_produce(object_type *o_ptr, bool allow_discount)
{
	int size = 1;

	int discount = 0;

	s32b cost = object_value(o_ptr);


	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Food, Flasks, and Lites */
		case TV_FOOD:
		case TV_MUSHROOM:
		case TV_FLASK:
		case TV_LITE:
		{
			if (cost <= 5L) size += mass_roll(3, 5);
			if (cost <= 20L) size += mass_roll(3, 5);
			break;
		}

/* XXX Changed these limits to make detect magic/identify a little
more frequent while expensive */
		case TV_POTION:
		case TV_SCROLL:
		{
			if (cost <= 140L) size += mass_roll(1, 5);
			if (cost <= 280L) size += mass_roll(1, 3);
			break;
		}

		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:
		{
			if (cost <= 50L) size += mass_roll(2, 3);
			if (cost <= 500L) size += mass_roll(1, 3);
			break;
		}

		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SHIELD:
		case TV_GLOVES:
		case TV_BOOTS:
		case TV_CLOAK:
		case TV_HELM:
		case TV_CROWN:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_HAFTED:
		case TV_DIGGING:
		case TV_BOW:
		{
			if (o_ptr->name2) break;
			if (cost <= 10L) size += mass_roll(3, 5);
			if (cost <= 100L) size += mass_roll(3, 5);
			break;
		}

		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (cost <= 5L) size += mass_roll(5, 5);
			if (cost <= 50L) size += mass_roll(5, 5);
			if (cost <= 500L) size += mass_roll(5, 5);
			break;
		}

		case TV_SERVICE:
			/* Never discount services */
			return;
	}

	if (!allow_discount) return;

	/* Pick a discount */
	if (cost < 5)
	{
		discount = 0;
	}
	else if (rand_int(25) == 0)
	{
		discount = 10;
	}
	else if (rand_int(50) == 0)
	{
		discount = 25;
	}
	else if (rand_int(150) == 0)
	{
		discount = 50;
	}
	else if (rand_int(300) == 0)
	{
		discount = 75;
	}
	else if (rand_int(500) == 0)
	{
		discount = 90;
	}


	/* Save the discount */
	o_ptr->discount = discount;

	/* Save the total pile size */
	o_ptr->number = size - (size * discount / 100);
}



/*
 * Convert a store item index into a one character label
 *
 * We use labels "a"-"l" for page 1, and labels "m"-"x" for page 2.
 */
static s16b store_to_label(int i, int store_index)
{
	(void)store_index;

	/* Assume legal */
	return (I2A(i));
}


/*
 * Convert a one character label into a store item index.
 *
 * Return "-1" if the label does not indicate a real store item.
 */
static s16b label_to_store(int c, int store_index)
{
	int i;
	store_type *st_ptr = store[store_index];

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i >= st_ptr->stock_num)) return (-1);

	/* Return the index */
	return (i);
}


/*
 * Determine if a store object can "absorb" another object.
 *
 * See "object_similar()" for the same function for the "player".
 *
 * This function can ignore many of the checks done for the player,
 * since stores (but not the home) only get objects under certain
 * restricted circumstances.
 */
static bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
	/* Hack -- Identical items cannot be stacked */
	if (o_ptr == j_ptr) return (0);

	/* Different objects cannot be stacked */
	if (o_ptr->k_idx != j_ptr->k_idx) return (0);

	/* MegaHack -- services are simple */
	if ((o_ptr->tval == TV_SERVICE) && (o_ptr->sval == j_ptr->sval)) return (1);

	/* Different pval cannot be stacked */
	if (o_ptr->pval != j_ptr->pval) return (0);

	/* Different charges (etc) cannot be stacked */
	if (o_ptr->charges != j_ptr->charges) return (0);

	/* Require many identical values */
	if (o_ptr->to_h != j_ptr->to_h) return (0);
	if (o_ptr->to_d != j_ptr->to_d) return (0);
	if (o_ptr->to_a != j_ptr->to_a) return (0);

	/* Require identical "artifact" names */
	if (o_ptr->name1 != j_ptr->name1) return (0);

	/* Require identical "ego-item" names */
	if (o_ptr->name2 != j_ptr->name2) return (0);

	/* Require identical "hidden powers" */
	if (o_ptr->xtra1 != j_ptr->xtra1) return (0);

	/* Require identical "hidden powers" */
	if (o_ptr->xtra2 != j_ptr->xtra2) return (0);

	/* Hack -- Never stack recharging items */
	if (o_ptr->timeout || j_ptr->timeout) return (0);

	/* Require many identical values */
	if (o_ptr->ac != j_ptr->ac) return (0);
	if (o_ptr->dd != j_ptr->dd) return (0);
	if (o_ptr->ds != j_ptr->ds) return (0);

	/* Require matching "discount" fields */
	if (o_ptr->discount != j_ptr->discount) return (0);

	/* They match, so they must be similar */
	return (TRUE);
}


/*
 * Allow a store object to absorb another object
 */
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr, int store_index)
{
	int total = o_ptr->number + j_ptr->number;

	store_type *st_ptr = store[store_index];

	/* Combine quantity, lose excess items */
	o_ptr->number = (total > 99) ? 99 : total;

	/* Mega-hack - services */
	if ((o_ptr->tval == TV_SERVICE) && (st_ptr->base >= STORE_MIN_BUY_SELL))
	{
		o_ptr->number = 1;
	}

	/* Merge the origin */
	if ((o_ptr->origin != j_ptr->origin) ||
		(o_ptr->origin_depth != j_ptr->origin_depth) ||
		(o_ptr->origin_xtra != j_ptr->origin_xtra))
	{
		int act = 2;

		if ((o_ptr->origin == ORIGIN_DROP) && (o_ptr->origin == j_ptr->origin))
		{
			monster_race *r_ptr = &r_info[o_ptr->origin_xtra];
			monster_race *s_ptr = &r_info[j_ptr->origin_xtra];

			bool r_uniq = (r_ptr->flags1 & RF1_UNIQUE) ? TRUE : FALSE;
			bool s_uniq = (s_ptr->flags1 & RF1_UNIQUE) ? TRUE : FALSE;

			if (r_uniq && !s_uniq) act = 0;
			else if (s_uniq && !r_uniq) act = 1;
			else act = 2;
		}

		switch (act)
		{
			/* Overwrite with j_ptr */
			case 1:
			{
				o_ptr->origin = j_ptr->origin;
				o_ptr->origin_depth = j_ptr->origin_depth;
				o_ptr->origin_xtra = j_ptr->origin_xtra;
			}

			/* Set as "mixed" */
			case 2:
			{
				o_ptr->origin = ORIGIN_MIXED;
			}
		}
	}
}


/*
 * Check to see if the shop will be carrying too many objects
 *
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.  Before, one could "nuke" objects this way, by
 * adding them to a pile which was already full.
 */
static bool store_check_num(object_type *o_ptr, int store_index)
{
	int i;
	object_type *j_ptr;

	store_type *st_ptr = store[store_index];

	/* Free space is always usable */
	if (st_ptr->stock_num < st_ptr->stock_size) return TRUE;

	/* The "home" acts like the player */
	if (st_ptr->base < STORE_MIN_BUY_SELL)
	{
		/* Check all the objects */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			/* Get the existing object */
			j_ptr = &st_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (object_similar(j_ptr, o_ptr)) return (TRUE);
		}
	}

	/* Normal stores do special stuff */
	else
	{
		/* Check all the objects */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			/* Get the existing object */
			j_ptr = &st_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (store_object_similar(j_ptr, o_ptr)) return (TRUE);
		}
	}

	/* But there was no room at the inn... */
	return (FALSE);
}


static int store_index_buy;

/*
 * Determine if the current store will purchase the given object
 *
 * Note that a shop-keeper must refuse to buy "worthless" objects
 */
static bool store_will_buy(const object_type *o_ptr)
{
	int i;
	bool blessed_weapons = FALSE;

	store_type *st_ptr = store[store_index_buy];

	/* Hack -- The Home is simple */
	if (st_ptr->base < STORE_MIN_BUY_SELL) return (TRUE);
	
	/* Hack -- No selling at all */
	if (adult_no_selling) return (FALSE);

	/* Hack -- Every store will buy books for services */
	if ((o_ptr->tval == TV_MAGIC_BOOK) || (o_ptr->tval == TV_PRAYER_BOOK) || (o_ptr->tval == TV_SONG_BOOK)) return (TRUE);

	/* Hack -- Every store will buy statues */
	if (o_ptr->tval == TV_STATUE) return (TRUE);
	
	/* Ignore "worthless" items XXX XXX XXX */
	if (object_value(o_ptr) <= 0) return (FALSE);

	/* Buy tvals that the store will buy */
	for (i = 0; i < STORE_WILL_BUY; i++)
	{
		if (st_ptr->tvals_will_buy[i] == o_ptr->tval) return (TRUE);
		if (st_ptr->tvals_will_buy[i] == TV_BLESSED) blessed_weapons = TRUE;
	}

	/* Hack -- temples buy blessed items */
	if (blessed_weapons)
	{
		u32b f1 = 0L, f2 = 0L, f3 = 0L, f4 = 0L;

		object_flags_known(o_ptr, &f1, &f2, &f3, &f4);

		if (f3 & (TR3_BLESSED)) return (TRUE);
	}

	/* Assume not okay */
	return (FALSE);
}



/*
 * Add an object to the inventory of the "Home"
 *
 * In all cases, return the slot (or -1) where the object was placed.
 *
 * Note that this is a hacked up version of "inven_carry()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" becoming
 * known, the player may have to pick stuff up and drop it again.
 */
static int home_carry(object_type *o_ptr, int store_index)
{
	int i, slot;
	s32b value, j_value;
	object_type *j_ptr;

	store_type *st_ptr = store[store_index];

	/* Check each existing object (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing object */
		j_ptr = &st_ptr->stock[slot];

		/* The home acts just like the player */
		if (object_similar(j_ptr, o_ptr))
		{
			/* Save the new number of items */
			object_absorb(j_ptr, o_ptr, TRUE);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= st_ptr->stock_size) return (-1);


	/* Determine the "value" of the object */
	value = object_value(o_ptr);

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get that object */
		j_ptr = &st_ptr->stock[slot];

		/* Hack -- readable books always come first */

		/* Objects sort by decreasing type */
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;

		/* Can happen in the home */
		if (!object_aware_p(o_ptr)) continue;
		if (!object_aware_p(j_ptr)) break;

		/* Objects sort by increasing sval */
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;

		/* Objects in the home can be unknown */
		if (!object_known_p(o_ptr)) continue;
		if (!object_known_p(j_ptr)) break;

		/* Objects sort by decreasing value */
		j_value = object_value(j_ptr);
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		/* Hack -- slide the objects */
		object_copy(&st_ptr->stock[i], &st_ptr->stock[i-1]);
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Hack -- Insert the new object */
	object_copy(&st_ptr->stock[slot], o_ptr);

	/* Return the location */
	return (slot);
}


/*
 * Add an object to a real stores inventory.
 *
 * If the object is "worthless", it is thrown away (except in the home).
 *
 * If the object cannot be combined with an object already in the inventory,
 * make a new slot for it, and calculate its "per item" price.  Note that
 * this price will be negative, since the price will not be "fixed" yet.
 * Adding an object to a "fixed" price stack will not change the fixed price.
 *
 * In all cases, return the slot (or -1) where the object was placed
 *
 * IDENT_STORE should be set by the calling routine if required.
 */
static int store_carry(object_type *o_ptr, int store_index)
{
	int i, slot;
	s32b value, j_value;
	object_type *j_ptr;

	store_type *st_ptr = store[store_index];

	/* Evaluate the object */
	value = object_value(o_ptr);

	/* Cursed/Worthless items "disappear" when sold */
	if (value <= 0) return (-1);

	/* Hack -- statues disappear when sold */
	if (o_ptr->tval == TV_STATUE) return (-1);

	/* Erase the inscription */
	o_ptr->note = 0;

	/* Remove special inscription, if any */
	o_ptr->feeling = 0;

	/* All obvious flags are learnt about the object - important this happens after IDENT_STORE set */
	object_obvious_flags(o_ptr, TRUE);

	/* Check each existing object (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing object */
		j_ptr = &st_ptr->stock[slot];

		/* Can the existing items be incremented? */
		if (store_object_similar(j_ptr, o_ptr))
		{
			/* Absorb (some of) the object */
			store_object_absorb(j_ptr, o_ptr, store_index);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= st_ptr->stock_size) return (-1);

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get that object */
		j_ptr = &st_ptr->stock[slot];

		/* Objects sort by decreasing type */
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;

		/* Objects sort by increasing sval */
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;

		/* Evaluate that slot */
		j_value = object_value(j_ptr);

		/* Objects sort by decreasing value */
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		/* Hack -- slide the objects */
		object_copy(&st_ptr->stock[i], &st_ptr->stock[i-1]);
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Hack -- Insert the new object */
	object_copy(&st_ptr->stock[slot], o_ptr);

	/* Return the location */
	return (slot);
}


static bool store_services(object_type *i_ptr, int store_index)
{
	int i, ii;
	object_type *j_ptr;
	object_type object_type_body;
	bool service = FALSE;

	store_type *st_ptr = store[store_index];

	/* No services in black market */
	if (st_ptr->base == STORE_B_MARKET) return (FALSE);

	/* No services in library */
	if (st_ptr->base == STORE_LIBRARY) return (FALSE);

	/* No services in quest locations */
	if (st_ptr->base < STORE_MIN_BUY_SELL) return (FALSE);

	/* Must be a book */
	if ((i_ptr->tval != TV_PRAYER_BOOK) && (i_ptr->tval != TV_MAGIC_BOOK) && (i_ptr->tval != TV_SONG_BOOK)) return (FALSE);

	/* Hack -- Check for services -- except library */
	for (i = 0; i < z_info->s_max; i++)
	{
		bool in_item = FALSE;
		int service_sval = 0;

		for (ii = 0; ii < MAX_SPELL_APPEARS; ii++)
		{
			spell_appears *s_object = &s_info[i].appears[ii];

			if ((s_object->tval == i_ptr->tval) && (s_object->sval == i_ptr->sval)) in_item = TRUE;

			if (s_object->tval == TV_SERVICE)
			{
				service_sval = s_object->sval;
			}
		}

		/* Create service object */
		if ((in_item == TRUE) && (service_sval > 0))
		{
			int k_idx = lookup_kind(TV_SERVICE,service_sval);

			/* Get local object */
			j_ptr = &object_type_body;

			/* Create a new object of the chosen kind */
			object_prep(j_ptr, k_idx);

			/* The object is "known" */
			object_aware(j_ptr, TRUE);
			object_known(j_ptr);

			/* The object belongs to the store */
			j_ptr->ident |= (IDENT_STORE);

			/* Carry the object in the store */
			if (store_carry(j_ptr, store_index) >= 0) service = TRUE;
		}
	}

	return (service);
}


/*
 * Increase, by a given amount, the number of a certain item
 * in a certain store.  This can result in zero items.
 */
static void store_item_increase(int item, int num, int store_index)
{
	int cnt;
	object_type *o_ptr;

	store_type *st_ptr = store[store_index];

	/* Get the object */
	o_ptr = &st_ptr->stock[item];

	/* Mega-hack - services */
	if ((o_ptr->tval == TV_SERVICE) && (st_ptr->base >= STORE_MIN_BUY_SELL))
	{
		o_ptr->number = 1;
		return;
	}

	/* Verify the number */
	cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;
	num = cnt - o_ptr->number;

	/* Save the new number */
	o_ptr->number += num;
}


/*
 * Remove a slot if it is empty
 */
static void store_item_optimize(int item, int store_index)
{
	int j;
	object_type *o_ptr;

	store_type *st_ptr = store[store_index];

	/* Get the object */
	o_ptr = &st_ptr->stock[item];

	/* Must exist */
	if (!o_ptr->k_idx) return;

	/* Must have no items */
	if (o_ptr->number) return;

	/* One less object */
	st_ptr->stock_num--;

	/* Slide everyone */
	for (j = item; j < st_ptr->stock_num; j++)
	{
		st_ptr->stock[j] = st_ptr->stock[j + 1];
	}

	/* Nuke the final slot */
	object_wipe(&st_ptr->stock[j]);
}


/*
 * Attempt to delete (some of) a random object from the store
 * Hack -- we attempt to "maintain" piles of items when possible.
 */
static void store_delete(int store_index)
{
	int what, num;

	store_type *st_ptr = store[store_index];

	/* Paranoia */
	if (st_ptr->stock_num <= 0) return;

	/* Pick a random slot */
	what = rand_int(st_ptr->stock_num);

	/* Determine how many objects are in the slot */
	num = st_ptr->stock[what].number;

	if (st_ptr->base == STORE_B_MARKET)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &st_ptr->stock[what];

		/* keep expensive stuff */
		if (rand_int(k_info[o_ptr->k_idx].cost) > 500
			 && rand_int(k_info[o_ptr->k_idx].cost) > 500
			 && rand_int(k_info[o_ptr->k_idx].cost) > 500)
			return;
	}
	else
	{
		/* Hack -- sometimes, only destroy half the objects */
		if (rand_int(100) < 50)
			num = (num + 1) / 2;

		/* Hack -- sometimes, only destroy a single object */
		if (rand_int(100) < 50)
			num = 1;
	}

	/* Actually destroy (part of) the object */
	store_item_increase(what, -num, store_index);
	store_item_optimize(what, store_index);
}


/*
 * Creates a random object and gives it to a store
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 *
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 *
 * Should we check for "permission" to have the given object?
 *
 * Hack -- if the store frequency of an item is 100 or greater,
 * we guarantee it will be created, if the store does not already
 * carry it.
 */
static void store_create(int store_index)
{
	int k_idx = 0;
	int tries, level;
	int choice, total, i;

	object_type *i_ptr;
	object_type object_type_body;

	store_type *st_ptr = store[store_index];

	/* Paranoia -- no room left */
	if (st_ptr->stock_num >= st_ptr->stock_size) return;

	/* Fake level for apply_magic() */
	level = rand_range(1, STORE_OBJ_LEVEL + st_ptr->level);

	/* Total choices in store */
	total = 0;

	/* Build item choices or choose an unstocked item */
	for (i = 0;i < STORE_CHOICES;i++)
	{
		/* Hack -- attempt to stock if item not available */
		if ((st_ptr->count[i]) && (rand_int(adj_chr_stock[p_ptr->stat_ind[A_CHR]]) < st_ptr->count[i]))
		{
			int j;

			for (j = 0; j < st_ptr->stock_num; j++)
			{
				i_ptr = &(st_ptr->stock[j]);

				if ((st_ptr->tval[i] == i_ptr->tval) &&
					(st_ptr->sval[i] == i_ptr->sval)) break;
			}

			if (j >= st_ptr->stock_num)
			{
					k_idx = lookup_kind(st_ptr->tval[i],st_ptr->sval[i]);

				if (k_idx) break;
			}
		}
	}

	/* Build item choices */
	for (i = 0;i < STORE_CHOICES;i++)
	{
		total += st_ptr->count[i];
	}

	/* Hack -- consider up to four items */
	for (tries = 0; tries < 4; tries++)
	{
		/* Paranoia */
		race_drop_idx = 0;

		/* Paranoia */
		tval_drop_idx = 0;
                
                /* Hack -- handle fixed artifacts */
                if(st_ptr->auto_artifact) {
                    artifact_type art = a_info[st_ptr->auto_artifact];
                    if(art.cur_num == 0) {
                        /* Set pointer */
                        i_ptr = &object_type_body;
                        /* Get base item */
                        k_idx = lookup_kind(art.tval, art.sval);
                        /* Prep the object */
                        object_prep(i_ptr, k_idx);
                        /* Set as artifact */
                        i_ptr->name1 = st_ptr->auto_artifact;
                        /* Apply artifact attributes */
                        apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);
                        /* Get store origina */
                        i_ptr->origin = ORIGIN_STORE_REWARD;
                    
                        /* Its now safe to identify quest rewards */
                        object_aware(i_ptr, TRUE);
                        object_known(i_ptr);

                        /* Attempt to carry the (known) object */
                        (void)store_carry(i_ptr, store_index);

                        return;
                    }
                }
		/* Quest rewards */
		if ((total == 0) && (st_ptr->base == STORE_QUEST_REWARD))
		{
			/* Mega-hack -- fiddle depth */
			int depth = p_ptr->depth;

			p_ptr->depth = st_ptr->level;
			object_level = st_ptr->level;

			/* Get local object */
			i_ptr = &object_type_body;

			if (!make_object(i_ptr, TRUE, FALSE)) continue;

			/* Reset depth */
			p_ptr->depth = depth;
			object_level = depth;

			/* Its now safe to identify quest rewards */
			object_aware(i_ptr, TRUE);
			object_known(i_ptr);

			/* Get store origina */
			i_ptr->origin = ORIGIN_STORE_REWARD;

			/* Attempt to carry the (known) object */
			(void)store_carry(i_ptr, store_index);

			return;
		}

		/* Black Market, storage areas */
		else if (total == 0)
		{
			/* Pick a level for object/magic */
			level = st_ptr->level + rand_int(st_ptr->level);

			if (level > 100) level = 100;

			/* Random object kind (usually of given level) */
			k_idx = get_obj_num(level);

			/* Handle failure */
			if (!k_idx) continue;
		}

		/* Normal Store */
		else if (!k_idx)
		{
			choice = rand_int(total);

			for (i = 0; i < STORE_CHOICES;i++)
			{
				choice -= st_ptr->count[i];

				if (choice >= 0) continue;

				/* Hack -- Pick an object kind to sell */
				k_idx = lookup_kind(st_ptr->tval[i],st_ptr->sval[i]);

				break;
			}

			if (!k_idx) continue;
		}

		/* Get local object */
		i_ptr = &object_type_body;

		/* Create a new object of the chosen kind */
		object_prep(i_ptr, k_idx);

		/* Apply some "low-level" magic (no artifacts unless reward) */
		apply_magic(i_ptr, level, FALSE, FALSE, FALSE);

		/* Hack -- Charge lite's */
		if (i_ptr->tval == TV_LITE)
		{
			if (i_ptr->sval == SV_LITE_TORCH) i_ptr->charges = FUEL_TORCH / 2;
			if (i_ptr->sval == SV_LITE_LANTERN) i_ptr->charges = FUEL_LAMP / 2;
		}

		/* Item belongs to storage */
		if (st_ptr->base == STORE_STORAGE)
		{
			i_ptr->origin = ORIGIN_STORE_STORAGE;

			/* The object is "known" */
			object_aware(i_ptr, TRUE);
			object_known(i_ptr);
		}
		/* Item belongs to a store */
		else
		{
			i_ptr->ident |= IDENT_STORE;
			i_ptr->origin = ORIGIN_STORE;

			/* The object is "known" */
			object_known(i_ptr);
		}

		/* No "worthless" items */
		if (object_value(i_ptr) <= 0) continue;

		/* Mass produce and/or Apply discount */
		mass_produce(i_ptr, (st_ptr->base != STORE_STORAGE));

		/* Attempt to carry the (known) object */
		(void)store_carry(i_ptr, store_index);

		/* Check for services */
		(void)store_services(i_ptr, store_index);

		/* Definitely done */
		break;
	}
}



/*
 * Eliminate need to bargain if player has haggled well in the past
 */
static bool noneedtobargain(s32b minprice, int store_index)
{
	store_type *st_ptr = store[store_index];

	s32b good = st_ptr->good_buy;
	s32b bad = st_ptr->bad_buy;

	/* Cheap items are "boring" */
	if (minprice < 10L) return (TRUE);

	/* Perfect haggling */
	if (good == MAX_SHORT) return (TRUE);

	/* Reward good haggles, punish bad haggles, notice price */
	if (good > ((3 * bad) + (5 + (minprice/50)))) return (TRUE);

	/* Return the flag */
	return (FALSE);
}


/*
 * Update the bargain info
 */
static void updatebargain(s32b price, s32b minprice, int store_index)
{
	store_type *st_ptr = store[store_index];

	/* Hack -- auto-haggle */
	if (!adult_haggle) return;

	/* Cheap items are "boring" */
	if (minprice < 10L) return;

	/* Count the successful haggles */
	if (price == minprice)
	{
		/* Just count the good haggles */
		if (st_ptr->good_buy < MAX_SHORT)
		{
			st_ptr->good_buy++;
		}
	}

	/* Count the failed haggles */
	else
	{
		/* Just count the bad haggles */
		if (st_ptr->bad_buy < MAX_SHORT)
		{
			st_ptr->bad_buy++;
		}
	}
}


/*
 * Guesstimate a cost.
 */
static long guess_cost(int item, int store_index)
{
	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	int x;

	/* Get the object */
	object_type *o_ptr = &st_ptr->stock[item];

	/* Display a "fixed" cost */
	if (o_ptr->ident & (IDENT_FIXED))
	{
		/* Extract the "minimum" price */
		x = price_item(o_ptr, ot_ptr->min_inflate, FALSE, store_index);
	}

	/* Display a "haggle" cost */
	else if (adult_haggle)
	{
		/* Extrect the "maximum" price */
		x = price_item(o_ptr, ot_ptr->max_inflate, FALSE, store_index);
	}

	/* Display a "taxed" cost */
	else
	{
		/* Extract the "minimum" price */
		x = price_item(o_ptr, ot_ptr->min_inflate, FALSE, store_index);

		/* Hack -- Apply Sales Tax if needed */
		if (!noneedtobargain(x, store_index)) x += x / 10;
	}

	return (x);
}



/*
 * Redisplay a single store entry
 */
static void display_entry(int item, int store_index)
{
	int y;
	object_type *o_ptr;

	char o_name[80];
	char out_val[160];
	int maxwid;

	store_type *st_ptr = store[store_index];

	/* Must be on current "page" to get displayed */
	if (!((item >= store_top) && (item < store_top + store_size))) return;


	/* Get the object */
	o_ptr = &st_ptr->stock[item];

	/* Get the row */
	y = (item % store_size) + 6;

	/* Label it, clear the line --(-- */
	sprintf(out_val, "%c) ", store_to_label(item, store_index));
	prt(out_val, y, 0);

	/* Describe an object in the home */
	if (st_ptr->base < STORE_MIN_BUY_SELL)
	{
		byte attr;
		int wgt;

		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		maxwid -= 10;

		/* Describe the object */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
		o_name[maxwid] = '\0';

		/* Get inventory color */
		attr = tval_to_attr[o_ptr->tval & 0x7F];

		/* Display the object */
		c_put_str(attr, o_name, y, 3);

		/* Class learning */
		if (k_info[o_ptr->k_idx].aware & (AWARE_CLASS))
		{
			/* Learn flavor */
			object_aware(o_ptr, TRUE);
		}
		else
		{
			/* Mark object seen */
			object_aware_tips(o_ptr, TRUE);
		}

		/* XXX XXX - Mark objects as "seen" (doesn't belong in this function) */
		if (o_ptr->name2)
		{
			if (!(e_info[o_ptr->name2].aware & (AWARE_EXISTS)))
			{
				queue_tip(format("ego%d.txt", o_ptr->name2));
			}

			e_info[o_ptr->name2].aware |= (AWARE_EXISTS);
		}

		/* Only show the weight of a single object */
		wgt = o_ptr->weight;
		sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
		put_str(out_val, y, 68);
	}

	/* Describe an object (fully) in a store */
	else
	{
		byte attr;
		int wgt;

		/* Must leave room for the "price" */
		maxwid = 65;

		/* Leave room for weights, if necessary -DRS- */
		maxwid -= 7;

		/* Describe the object (fully) */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
		o_name[maxwid] = '\0';

		/* Get inventory color */
		attr = tval_to_attr[o_ptr->tval & 0x7F];

		/* Display the object */
		c_put_str(attr, o_name, y, 3);

		/* Only show the weight of a single object */
		wgt = o_ptr->weight;
		sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
		put_str(out_val, y, 61);

		/* Make object aware - short routine */
		k_info[o_ptr->k_idx].aware |= (AWARE_EXISTS);

		/* XXX XXX - Mark objects as "seen" (doesn't belong in this function) */
		if (o_ptr->name2)
		{
			e_info[o_ptr->name2].aware |= (AWARE_EXISTS);

			queue_tip(format("ego%d.txt", o_ptr->name2));
		}

		/* Actually draw the price (not fixed) */
		sprintf(out_val, "%9ld %c", guess_cost(item, store_index), o_ptr->ident & (IDENT_FIXED) ? 'F' : ' ');
		put_str(out_val, y, 68);
	}
}


/*
 * Display a store's inventory
 *
 * All prices are listed as "per individual object"
 */
static void display_inventory(int store_index)
{
	int i, k;

	store_type *st_ptr = store[store_index];

	/* Display the next store size items */
	for (k = 0; k < store_size; k++)
	{
		/* Stop when we run out of items */
		if (store_top + k >= st_ptr->stock_num) break;

		/* Display that line */
		display_entry(store_top + k, store_index);
	}

	/* Erase the extra lines and the "more" prompt */
	for (i = k; i < Term->hgt - 11; i++) prt("", i + 6, 0);

	/* Assume "no current page" */
	put_str("        ", 5, 20);

	/* Visual reminder of "more items" */
	if (st_ptr->stock_num > store_size)
	{
		/* Show "more" reminder (after the last object ) */
		prt("-more-", k + 6, 3);

		/* Indicate the "current page" */
		put_str(format("(Page %d)", store_top/store_size + 1), 5, 20);
	}
}


/*
 * Display players gold
 */
static void store_prt_gold(void)
{
	char out_val[64];

	prt("Gold Remaining: ", 7 + store_size, 53);

	sprintf(out_val, "%9ld", (long)p_ptr->au);
	prt(out_val, 7 + store_size, 68);
}


/*
 * Display store (after clearing screen)
 */
static void display_store(int store_index)
{
	char buf[80];

	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	/* Clear screen */
	Term_clear();

	/* Initialise store size */
	store_size = Term->hgt - 12;

	/* The "Home" is special */
	if (st_ptr->base < STORE_MIN_BUY_SELL)
	{
		/* Put the owner name */
		put_str(u_name + u_info[st_ptr->index].name, 3, 30);

		/* Label the object descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		put_str("Weight", 5, 70);
	}

	/* Normal stores */
	else
	{
		cptr store_name = (u_name + u_info[st_ptr->index].name);
		cptr owner_name = &(b_name[ot_ptr->owner_name]);
		/* Maias and evil shapechangers hide their race */
		cptr race_name =
			(ot_ptr->owner_race == RACE_MAIA
			 || ot_ptr->owner_race == RACE_WEREWOLF
			 || ot_ptr->owner_race == RACE_VAMPIRE)
			? "Human"
			: p_name + p_info[ot_ptr->owner_race].name;
		int pos_store = 77 - strlen(store_name) - 8;
		int pos_owner = 10;

		/* Put the owner name and race */
		sprintf(buf, "%s (%s)", owner_name, race_name);
		if (pos_owner + (int) strlen(buf) + 2 > pos_store)
		  pos_owner = 1;
		put_str(buf, 3, pos_owner);

		/* Show the max price in the store (above prices) */
		sprintf(buf, "%s (%ld)", store_name, (long)(ot_ptr->max_cost));
		prt(buf, 3, pos_store);

		/* Label the object descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		put_str("Weight", 5, 60);

		/* Label the asking price (in stores) */
		put_str("Price", 5, 72);
	}

	/* Display the current gold */
	store_prt_gold();

	/* Draw in the inventory */
	display_inventory(store_index);
}



/*
 * Get the index of a store object
 *
 * Return TRUE if an object was selected
 */
static bool get_stock(int *com_val, cptr pmt, int store_index)
{
	int item;

	key_event ke;

	char buf[160];

	char o_name[80];

	char out_val[160];

	object_type *o_ptr;

	store_type *st_ptr = store[store_index];

#ifdef ALLOW_REPEAT

	/* Get the item index */
	if (repeat_pull(com_val))
	{
		/* Verify the item */
		if ((*com_val >= 0) && (*com_val <= (st_ptr->stock_num - 1)))
		{
			/* Success */
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT */

	/* Assume failure */
	*com_val = (-1);

	/* Build the prompt */
	sprintf(buf, "(Items %c-%c, ESC to exit) %s",
		store_to_label(0, store_index), store_to_label(st_ptr->stock_num - 1, store_index),
		pmt);

	/* Ask until done */
	while (TRUE)
	{
		bool verify;
		char which;

		/* Escape */
		if (!get_com_ex(buf, &ke)) return (FALSE);

		/* Hack -- handle mouse input */
		if (ke.key == '\xff')
		{
			if (ke.mousebutton)
			{
				/* Hack -- fake a letter */
				if ((ke.mousey >= 6) && (ke.mousey < 6 + store_size))
					ke.key = 'a' + ke.mousey - 6 + store_top;

				/* Hack -- force error */
				else ke.key = 'z';
			}
			/* Ignore anything else */
			else continue;
		}

		/* Get the choice */
		which = ke.key;

		/* Note verify */
		verify = (isupper(which) ? TRUE : FALSE);

		/* Lowercase */
		which = tolower(which);

		/* Convert response to item */
		item = label_to_store(which, store_index);

		/* Oops */
		if (item < 0)
		{
			/* Oops */
			bell("Illegal store object choice!");

			continue;
		}

		/* No verification */
		if (!verify) break;

		/* Object */
		o_ptr = &st_ptr->stock[item];

		/* Describe */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Prompt */
		sprintf(out_val, "Try %s? ", o_name);

		/* Query */
		if (!get_check(out_val)) return (FALSE);

		/* Done */
		break;
	}

	/* Save item */
	(*com_val) = item;

#ifdef ALLOW_REPEAT

	repeat_push(*com_val);

#endif /* ALLOW_REPEAT */

	/* Success */
	return (TRUE);
}


/*
 * Increase the insult counter and get angry if necessary
 */
static int increase_insults(int store_index)
{
	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	/* Increase insults */
	st_ptr->insult_cur++;

	/* Become insulted */
	if (st_ptr->insult_cur > ot_ptr->insult_max)
	{
		/* Complain */
		say_comment_4();

		/* Reset insults */
		st_ptr->insult_cur = 0;
		st_ptr->good_buy = 0;
		st_ptr->bad_buy = 0;

		/* Open tomorrow */
		st_ptr->store_open = turn + 25000 + randint(25000);

		/* Closed */
		return (TRUE);
	}

	/* Not closed */
	return (FALSE);
}


/*
 * Decrease the insult counter
 */
static void decrease_insults(int store_index)
{
	store_type *st_ptr = store[store_index];

	/* Decrease insults */
	if (st_ptr->insult_cur) st_ptr->insult_cur--;
}


/*
 * The shop-keeper has been insulted
 */
static int haggle_insults(int store_index)
{
	/* Increase insults */
	if (increase_insults(store_index)) return (TRUE);

	/* Display and flush insult */
	say_comment_5();

	/* Still okay */
	return (FALSE);
}


/*
 * Mega-Hack -- Enable "increments"
 */
static bool allow_inc = FALSE;

/*
 * Mega-Hack -- Last "increment" during haggling
 */
static s32b last_inc = 0L;


/*
 * Get a haggle
 */
static int get_haggle(cptr pmt, s32b *poffer, s32b price, int final)
{
	char buf[128];


	/* Clear old increment if necessary */
	if (!allow_inc) last_inc = 0L;


	/* Final offer */
	if (final)
	{
		sprintf(buf, "%s [accept] ", pmt);
	}

	/* Old (negative) increment, and not final */
	else if (last_inc < 0)
	{
		sprintf(buf, "%s [-%ld] ", pmt, (long)(ABS(last_inc)));
	}

	/* Old (positive) increment, and not final */
	else if (last_inc > 0)
	{
		sprintf(buf, "%s [+%ld] ", pmt, (long)(ABS(last_inc)));
	}

	/* Normal haggle */
	else
	{
		sprintf(buf, "%s ", pmt);
	}


	/* Paranoia XXX XXX XXX */
	msg_print(NULL);


	/* Ask until done */
	while (TRUE)
	{
		cptr p;

		char out_val[80];

		/* Default */
		my_strcpy(out_val, "", sizeof(out_val));

		/* Ask the user for a response */
		if (!get_string(buf, out_val, sizeof(out_val))) return (FALSE);

		/* Skip leading spaces */
		for (p = out_val; *p == ' '; p++) /* loop */;

		/* Empty response */
		if (*p == '\0')
		{
			/* Accept current price */
			if (final)
			{
				*poffer = price;
				last_inc = 0L;
				break;
			}

			/* Use previous increment */
			if (allow_inc && last_inc)
			{
				*poffer += last_inc;
				break;
			}
		}

		/* Normal response */
		else
		{
			s32b i;

			/* Extract a number */
			i = atol(p);

			/* Handle "incremental" number */
			if ((*p == '+' || *p == '-'))
			{
				/* Allow increments */
				if (allow_inc)
				{
					/* Use the given "increment" */
					*poffer += i;
					last_inc = i;
					break;
				}
			}

			/* Handle normal number */
			else
			{
				/* Use the given "number" */
				*poffer = i;
				last_inc = 0L;
				break;
			}
		}

		/* Warning */
		msg_print("Invalid response.");
		msg_print(NULL);
	}

	/* Success */
	return (TRUE);
}


/*
 * Receive an offer (from the player)
 *
 * Return TRUE if offer is NOT okay
 */
static bool receive_offer(cptr pmt, s32b *poffer,
			  s32b last_offer, int factor,
			  s32b price, int final, int store_index)
{
	/* Haggle till done */
	while (TRUE)
	{
		/* Get a haggle (or cancel) */
		if (!get_haggle(pmt, poffer, price, final)) return (TRUE);

		/* Acceptable offer */
		if (((*poffer) * factor) >= (last_offer * factor)) break;

		/* Insult, and check for kicked out */
		if (haggle_insults(store_index)) return (TRUE);

		/* Reject offer (correctly) */
		(*poffer) = last_offer;
	}

	/* Success */
	return (FALSE);
}


/*
 * Haggling routine XXX XXX
 *
 * Return TRUE if purchase is NOT successful
 */
static bool purchase_haggle(object_type *o_ptr, s32b *price, cptr buying, char label, int store_index)
{
	s32b cur_ask, final_ask;
	s32b last_offer, offer;
	s32b x1, x2, x3;
	s32b min_per, max_per;
	int flag, loop_flag, noneed;
	int annoyed = 0, final = FALSE;

	bool ignore = FALSE;

	bool cancel = FALSE;

	cptr pmt = "Asking";

	char out_val[160];

	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	*price = 0;


	/* Extract the starting offer and final offer */
	cur_ask = price_item(o_ptr, ot_ptr->max_inflate, FALSE, store_index);
	final_ask = price_item(o_ptr, ot_ptr->min_inflate, FALSE, store_index);

	/* Determine if haggling is necessary */
	noneed = noneedtobargain(final_ask, store_index);

	/* No need to haggle */
	if (!adult_haggle || noneed || (o_ptr->ident & (IDENT_FIXED)))
	{
		/* Auto-haggle */
		if (!adult_haggle)
		{
			/* Ignore haggling */
			ignore = TRUE;

			/* Apply sales tax */
			final_ask += (final_ask / 10);
		}

		/* Jump to final price */
		cur_ask = final_ask;

		/* Go to final offer */
		pmt = "Final Offer";
		final = TRUE;
	}


	/* Haggle for the whole pile */
	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;


	/* Haggle parameters */
	min_per = ot_ptr->haggle_per;
	max_per = min_per * 3;

	/* Mega-Hack -- artificial "last offer" value */
	last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer = last_offer * (200 - (int)(ot_ptr->max_inflate)) / 100L;
	if (last_offer <= 0) last_offer = 1;

	/* No offer yet */
	offer = 0;

	/* No incremental haggling yet */
	allow_inc = FALSE;

	/* Haggle until done */
	for (flag = FALSE; !flag; )
	{
		loop_flag = TRUE;

		while (!flag && loop_flag)
		{
			sprintf(out_val, "%s :  %ld for %s (%c)", pmt, (long)cur_ask, buying, label);
			put_str(out_val, 1, 0);
			cancel = receive_offer("What do you offer? ",
					       &offer, last_offer, 1, cur_ask, final, store_index);

			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer > cur_ask)
			{
				say_comment_6();
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}
		}

		if (!flag)
		{
			x1 = 100 * (offer - last_offer) / (cur_ask - last_offer);
			if (x1 < min_per)
			{
				if (haggle_insults(store_index))
				{
					flag = TRUE;
					cancel = TRUE;
				}
			}
			else if (x1 > max_per)
			{
				x1 = x1 * 3 / 4;
				if (x1 < max_per) x1 = max_per;
			}
			x2 = rand_range(x1-2, x1+2);
			x3 = ((cur_ask - offer) * x2 / 100L) + 1;
			/* don't let the price go up */
			if (x3 < 0) x3 = 0;
			cur_ask -= x3;

			/* Too little */
			if (cur_ask < final_ask)
			{
				final = TRUE;
				cur_ask = final_ask;
				pmt = "Final Offer";
				annoyed++;
				if (annoyed > 3)
				{
					(void)(increase_insults(store_index));
					cancel = TRUE;
					flag = TRUE;
				}
			}
			else if (offer >= cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}

			if (!flag)
			{
				int col = strlen(out_val) + 2;

				last_offer = offer;
				allow_inc = TRUE;
				prt("", 1, 0);
				sprintf(out_val, "Your last offer: %ld",
					      (long)last_offer);
				put_str(out_val, 1, col);
				say_comment_2(cur_ask, annoyed);
			}
		}
	}

	/* Mark price as fixed */
	if (!ignore && (*price == final_ask))
	{
		/* Mark as fixed price */
		o_ptr->ident |= (IDENT_FIXED);
	}

	/* Cancel */
	if (cancel) return (TRUE);

	/* Update haggling info */
	if (!ignore)
	{
		/* Update haggling info */
		updatebargain(*price, final_ask, store_index);
	}

	/* Do not cancel */
	return (FALSE);
}


/*
 * Haggling routine XXX XXX
 *
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(object_type *o_ptr, s32b *price, cptr selling, char label, int store_index)
{
	s32b purse, cur_ask, final_ask;
	s32b last_offer, offer = 0;
	s32b x1, x2, x3;
	s32b min_per, max_per;

	int flag, loop_flag, noneed;
	int annoyed = 0, final = FALSE;

	bool ignore = FALSE;

	bool cancel = FALSE;

	cptr pmt = "Offer";

	char out_val[160];

	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];

	*price = 0;


	/* Obtain the starting offer and the final offer */
	cur_ask = price_item(o_ptr, ot_ptr->max_inflate, TRUE, store_index);
	final_ask = price_item(o_ptr, ot_ptr->min_inflate, TRUE, store_index);

	/* Determine if haggling is necessary */
	noneed = noneedtobargain(final_ask, store_index);

	/* Get the owner's payout limit */
	purse = (s32b)(ot_ptr->max_cost);

	/* No need to haggle */
	if (noneed || !adult_haggle || (final_ask >= purse))
	{
		/* Apply Sales Tax (if needed) */
		if (!adult_haggle && !noneed)
		{
			final_ask -= final_ask / 10;
		}

		/* No haggle option */
		if (!adult_haggle)
		{
			/* Ignore haggling */
			ignore = TRUE;
		}

		/* Limit purse */
		if (final_ask > purse) final_ask = purse;

		/* Final price */
		cur_ask = final_ask;

		/* Final offer */
		final = TRUE;
		pmt = "Final Offer";
	}

	/* Haggle for the whole pile */
	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;


	/* Display commands XXX XXX XXX */

	/* Haggling parameters */
	min_per = ot_ptr->haggle_per;
	max_per = min_per * 3;

	/* Mega-Hack -- artificial "last offer" value */
	last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer *= ot_ptr->max_inflate / 100L;

	/* No offer yet */
	offer = 0;

	/* No incremental haggling yet */
	allow_inc = FALSE;

	/* Haggle */
	for (flag = FALSE; !flag; )
	{
		while (1)
		{
			loop_flag = TRUE;

			sprintf(out_val, "%s :  %ld for %s (%c)", pmt, (long)cur_ask, selling, label);
			put_str(out_val, 1, 0);
			cancel = receive_offer("What price do you ask? ",
					       &offer, last_offer, -1, cur_ask, final, store_index);

			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer < cur_ask)
			{
				say_comment_6();
				/* rejected, reset offer for incremental haggling */
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}

			/* Stop */
			if (flag || !loop_flag) break;
		}

		if (!flag)
		{
			x1 = 100 * (last_offer - offer) / (last_offer - cur_ask);
			if (x1 < min_per)
			{
				if (haggle_insults(store_index))
				{
					flag = TRUE;
					cancel = TRUE;
				}
			}
			else if (x1 > max_per)
			{
				x1 = x1 * 3 / 4;
				if (x1 < max_per) x1 = max_per;
			}
			x2 = rand_range(x1-2, x1+2);
			x3 = ((offer - cur_ask) * x2 / 100L) + 1;
			/* don't let the price go down */
			if (x3 < 0) x3 = 0;
			cur_ask += x3;

			if (cur_ask > final_ask)
			{
				cur_ask = final_ask;
				final = TRUE;
				pmt = "Final Offer";
				annoyed++;
				if (annoyed > 3)
				{
					flag = TRUE;
					(void)(increase_insults(store_index));
				}
			}
			else if (offer <= cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}

			if (!flag)
			{
				last_offer = offer;
				allow_inc = TRUE;
				prt("", 1, 0);
				sprintf(out_val,
					      "Your last bid %ld", (long)last_offer);
				put_str(out_val, 1, 39);
				say_comment_3(cur_ask, annoyed);
			}
		}
	}

	/* Cancel */
	if (cancel) return (TRUE);

	/* Update haggling info */
	if (!ignore)
	{
		/* Update haggling info */
		updatebargain(*price, final_ask, store_index);
	}

	/* Do not cancel */
	return (FALSE);
}





/*
 * Buy an object from a store
 */
static void store_purchase(int store_index)
{
	int n = 99;
	int amt, choice;
	int item, item_new;

	s32b price;

	object_type *o_ptr;
	object_type *i_ptr;
	object_type *j_ptr;
	object_type object_type_body;

	char o_name[80];
	char j_name[80];

	char out_val[160];

	store_type *st_ptr = store[store_index];

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (st_ptr->base < STORE_MIN_BUY_SELL)
		{
			msg_print("There is nothing here.");
		}
		else
		{
			msg_print("I am currently out of stock.");
		}
		return;
	}


	/* Prompt */
	if (st_ptr->base < STORE_MIN_BUY_SELL)
	{
		sprintf(out_val, "Which item do you want to take? ");
	}
	else
	{
		sprintf(out_val, "Which item are you interested in? ");
	}

	/* Get the object number to be bought */
	if (!get_stock(&item, out_val, store_index)) return;

	/* Get the actual object */
	o_ptr = &st_ptr->stock[item];

	/* Try to restrict quantity based on what the player can afford */
	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		/* Guess maximum items can afford */
		n = p_ptr->au / guess_cost(item, store_index);

		/* Maybe able to haggle */
		if (!n)
		{
			if (adult_haggle)
			{
				n = 1;
			}
			else
			{
				msg_print("You can't afford that item.");
				return;
			}
		}
	}

	/* Get a quantity */
	amt = get_quantity(NULL, n > o_ptr->number ? o_ptr->number : n);

	/* Allow user abort */
	if (amt <= 0) return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Get desired object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* Hack -- require room in pack */
	if (i_ptr->tval != TV_SERVICE)
	{
		bool hack_ident_store = (i_ptr->ident & (IDENT_STORE)) != 0;
		bool can_carry;

		/* Hack -- manipulate store flag */
		if (hack_ident_store) i_ptr->ident &= ~(IDENT_STORE);

		can_carry = inven_carry_okay(i_ptr);

		/* Hack -- manipulate store flag */
		if (hack_ident_store) i_ptr->ident |= (IDENT_STORE);

		if (!can_carry)
		{
			msg_print("You cannot carry that many items.");

			return;
		}
	}

	/* Attempt to buy it */
	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		/* Describe the object (fully) */
		object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

		/* Haggle for a final price */
		choice = purchase_haggle(i_ptr, &price, o_name, (char)store_to_label(item, store_index), store_index);

		/* Hack -- Got kicked out */
		if (st_ptr->store_open >= turn) return;

		/* Hack -- Maintain fixed prices */
		if (i_ptr->ident & (IDENT_FIXED))
		{
			/* Mark as fixed price */
			o_ptr->ident |= (IDENT_FIXED);
		}

		/* Player wants a service */
		if ((choice == 0) && (i_ptr->tval == TV_SERVICE))
		{
			/* Player can afford it */
			if (p_ptr->au >= price)
			{
				int power;

				bool cancel = TRUE;
				bool known = TRUE;

				/* Get service effect */
				get_spell(&power, "use", o_ptr, FALSE);

				/* Paranoia */
				if (power <= 0) return;

				/* Apply service effect */
				(void)process_spell(SOURCE_PLAYER_SERVICE, o_ptr->k_idx, power, 0, &cancel, &known, FALSE);

				/* Hack -- allow certain services to be cancelled */
				if (cancel) return;

				/* Say "okay" */
				say_comment_1();

				/* Be happy */
				decrease_insults(store_index);

				/* Spend the money */
				p_ptr->au -= price;

				/* Update the display */
				store_prt_gold();

				/* Class learning */
				if (k_info[o_ptr->k_idx].aware & (AWARE_CLASS))
				{
					/* Learn flavor */
					object_aware(o_ptr, TRUE);
				}
				else
				{
					/* Mark object seen */
					object_aware_tips(o_ptr, TRUE);
				}
			}

			/* Player cannot afford it */
			else
			{
				/* Simple message (no insult) */
				msg_print("You do not have enough gold.");
			}
		}

		/* Player wants anything else */
		else if (choice == 0)
		{
			/* Player can afford it */
			if (p_ptr->au >= price)
			{
				/* Say "okay" */
				say_comment_1();

				/* Be happy */
				decrease_insults(store_index);

				/* Spend the money */
				p_ptr->au -= price;

				/* Update the display */
				store_prt_gold();

				/* Buying an object makes you aware of it */
				object_aware(i_ptr, FALSE);

				/* The object kind is not guessed */
				k_info[i_ptr->k_idx].guess = 0;

				/* Combine / Reorder the pack (later) */
				p_ptr->notice |= (PN_COMBINE | PN_REORDER);

				/* Clear the "fixed" flag from the object */
				i_ptr->ident &= ~(IDENT_FIXED);

				/* The object no longer belongs to the store */
				i_ptr->ident &= ~(IDENT_STORE);

				/* Describe the transaction */
				object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

				/* Message */
				msg_format("You bought %s (%c) for %ld gold.",
					   o_name, store_to_label(item, store_index),
					   (long)price);

				/* Erase the inscription */
				i_ptr->note = 0;

				/* Remove special inscription, if any */
				o_ptr->feeling = 0;

				/* Give it to the player */
				item_new = inven_carry(i_ptr);

				j_ptr = &inventory[item_new];
				
				/* Describe the purchase */
				if (o_ptr->tval != TV_BAG && j_ptr->tval == TV_BAG)
				{
					/* Describe the bag */
					object_desc(j_name, sizeof(j_name), j_ptr, FALSE, 3);
					
					/* Hack -- temporarily change the number to describe it */
					int old_number = o_ptr->number;
					o_ptr->number = amt;
					
					/* Describe the object */
					object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
					
					/* Then restore the number */
					o_ptr->number = old_number;
					
					/* Message */
					msg_format("You put %s in your %s (%c).", o_name, j_name, index_to_label(item_new));		
				}
				else
				{
					/* Describe the object */
					object_desc(j_name, sizeof(j_name), j_ptr, TRUE, 3);
					
					/* Message */
					msg_format("You have %s (%c).", j_name, index_to_label(item_new));
				}
								
				/* Handle stuff */
				handle_stuff();

				/* Note how many slots the store used to have */
				n = st_ptr->stock_num;

				/* Remove the bought objects from the store */
				store_item_increase(item, -amt, store_index);
				store_item_optimize(item, store_index);

				/* Store is empty. */
				/* Note that if the last item is a service, then
				 * all items in store are services. This simplifies things
				 * a lot */
				if ((st_ptr->stock_num == 0) || (st_ptr->stock[st_ptr->stock_num - 1].tval == TV_SERVICE))
				{
					int i;

					/* Shuffle */
					if (rand_int(STORE_SHUFFLE) == 0)
					{
						/* Message */
						msg_print("The shopkeeper retires.");

						/* Shuffle the store */
						store_shuffle(store_index);
					}

					/* Maintain */
					else
					{
						/* Message */
						msg_print("The shopkeeper brings out some new stock.");
					}

					/* New inventory */
					for (i = 0; i < 10; ++i)
					{
						/* Maintain the store */
						store_maint(store_index);
					}

					/* Start over */
					store_top = 0;

					/* Redraw everything */
					display_inventory(store_index);
				}

				/* The object is gone */
				else if (st_ptr->stock_num != n)
				{
					/* Only one screen left */
					if (st_ptr->stock_num <= store_size)
					{
						store_top = 0;
					}

					/* Redraw everything */
					display_inventory(store_index);
				}

				/* The object is still here */
				else
				{
					/* Redraw the object */
					display_entry(item, store_index);
				}
			}

			/* Player cannot afford it */
			else
			{
				/* Simple message (no insult) */
				msg_print("You do not have enough gold.");
			}
		}
	}

	/* Home is much easier */
	else
	{

#if 0

		/* Describe the object */
		object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

		/* Message */
		msg_format("You pick up %s (%c).",
			   o_name, store_to_label(item, store_index));

#endif
		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* The object no longer belongs to the store */
		i_ptr->ident &= ~(IDENT_STORE);

		/* Give it to the player */
		item_new = inven_carry(i_ptr);

		j_ptr = &inventory[item_new];
		
		/* Describe the purchase */
		if (o_ptr->tval != TV_BAG && j_ptr->tval == TV_BAG)
		{
			/* Describe the bag */
			object_desc(j_name, sizeof(j_name), j_ptr, FALSE, 3);
			
			/* Hack -- temporarily change the number to describe it */
			int old_number = o_ptr->number;
			o_ptr->number = amt;
			
			/* Describe the object */
			object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
			
			/* Then restore the number */
			o_ptr->number = old_number;
			
			/* Message */
			msg_format("You put %s in your %s (%c).", o_name, j_name, index_to_label(item_new));		
		}
		else
		{
			/* Describe the object */
			object_desc(j_name, sizeof(j_name), j_ptr, TRUE, 3);
			
			/* Message */
			msg_format("You have %s (%c).", j_name, index_to_label(item_new));
		}

		/* Handle stuff */
		handle_stuff();

		/* Take note if we take the last one */
		n = st_ptr->stock_num;

		/* Remove the items from the home */
		store_item_increase(item, -amt, store_index);
		store_item_optimize(item, store_index);

		/* The object is gone */
		if (st_ptr->stock_num != n)
		{
			/* Only one screen left */
			if (st_ptr->stock_num <= store_size)
			{
				store_top = 0;
			}

			/* Redraw everything */
			display_inventory(store_index);
		}

		/* The object is still here */
		else
		{
			/* Redraw the object */
			display_entry(item, store_index);
		}
	}

	/* Not kicked out */
	return;
}


/*
 * Sell an object to the store (or home)
 */
static void store_sell(int store_index)
{
	int choice;
	int item, item_pos;
	int amt;

	s32b price, value, dummy;

	object_type *o_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	cptr q, s;

	char o_name[80];

	store_type *st_ptr = store[store_index];

	/* Home */
	q = "Drop which item? ";

	/* Real store */
	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		/* New prompt */
		q = "Sell which item? ";

		/* Set store index to buy */
		store_index_buy = store_index;

		/* Only allow items the store will buy */
		item_tester_hook = store_will_buy;
	}

	/* Get an item */
	s = "You have nothing that I want.";
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
		/* Get item from bag -- cancel for the bag */
		if (get_item_from_bag(&item, q, s, o_ptr)) o_ptr = &inventory[item];
	}

	/* Hack -- Cannot remove cursed objects */
	if ((item >= INVEN_WIELD) && cursed_p(o_ptr))
	{
		/* Oops */
		msg_print("Hmmm, it seems to be cursed.");

		mark_cursed_feeling(o_ptr);

		/* Nope */
		return;
	}

	/* Get a quantity */
	amt = get_quantity(NULL, o_ptr->number);

	/* Allow user abort */
	if (amt <= 0) return;

	/* Forget about what item may do */
	if (amt == o_ptr->number) inven_drop_flags(o_ptr);

	/* Get local object */
	i_ptr = &object_type_body;

	/* Get a copy of the object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* Forget about object */
	drop_may_flags(i_ptr);

	/* Get a full description */
	object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

	/* Is there room in the store (or the home?) */
	if (!store_check_num(i_ptr, store_index))
	{
		if (st_ptr->base < STORE_MIN_BUY_SELL)
		{
			msg_print("There is no room here to drop it.");
		}
		else
		{
			msg_print("I have not the room in my store to keep it.");
		}
		return;
	}

	/* Real store */
	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		/* Haggle for it */
		choice = sell_haggle(i_ptr, &price, o_name, index_to_label(item), store_index);

		/* Kicked out */
		if (st_ptr->store_open >= turn) return;

		/* Sold... */
		if (choice == 0)
		{
			/* Say "okay" */
			say_comment_1();

			/* Be happy */
			decrease_insults(store_index);

			/* Get some money */
			p_ptr->au += price;

			/* Update the display */
			store_prt_gold();

			/* Reset the stack */
			i_ptr->stackc = 0;

			/* Sometimes reverse the stack object */
			if (!object_charges_p(o_ptr) && (rand_int(o_ptr->number)< o_ptr->stackc))
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

			/* Get stack count */
			else if (amt >= (o_ptr->number - o_ptr->stackc))
			{
				/* Set stack count */
				i_ptr->stackc = amt - (o_ptr->number - o_ptr->stackc);
			}

			/* Get the "apparent" value */
			dummy = object_value(i_ptr) * i_ptr->number;

			/* Erase the inscription */
			i_ptr->note = 0;

			/* Remove special inscription, if any */
			o_ptr->feeling = 0;

			/* Identify original object */
			object_aware(o_ptr, TRUE);
			object_known(o_ptr);

			/* Identify store object */
			object_aware(i_ptr, TRUE);
			object_known(i_ptr);

			/* Combine / Reorder the pack (later) */
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

			/* The object belongs to the store now */
			i_ptr->ident |= IDENT_STORE;

			/* The object is valued by the shopkeeper now */
			i_ptr->ident &= ~(IDENT_VALUE);

			/* Get the "actual" value */
			value = object_value(i_ptr) * i_ptr->number;

			/* Get the description all over again */
			object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

			/* Describe the result (in message buffer) */
			msg_format("You sold %s (%c) for %ld gold.",
				   o_name, index_to_label(item), (long)price);

			/* Analyze the prices (and comment verbally) */
			purchase_analyze(price, value, dummy);

			/* Take the object from the player */
			inven_item_increase(item, -amt);
			inven_item_describe(item);
			inven_item_optimize(item);

			/* Handle stuff */
			handle_stuff();

			/* The store gets that (known) object */
			item_pos = store_carry(i_ptr, store_index);

			/* (Maybe) Add store services */
			if (store_services(i_ptr, store_index)) item_pos = 0;

			/* Update the display */
			if (item_pos >= 0)
			{
				/* Redisplay wares */
				display_inventory(store_index);
			}
		}
	}

	/* Player is at home */
	else
	{
		/* Describe */
		msg_format("You drop %s (%c).", o_name, index_to_label(item));

		/* Take it from the players inventory */
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);

		/* Handle stuff */
		handle_stuff();

		/* Let the home carry it */
		item_pos = home_carry(i_ptr, store_index);

		/* Update store display */
		if (item_pos >= 0)
		{
			/* Redisplay wares */
			display_inventory(store_index);
		}
	}
}


/*
 * Examine an item in a store
 */
static void store_examine(int store_index)
{
	int	 item;
	object_type *o_ptr;
	char	o_name[80];
	char	out_val[160];

	store_type *st_ptr = store[store_index];

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (st_ptr->base < STORE_MIN_BUY_SELL)
		{
			msg_print("There is nothing here.");
		}
		else
		{
			msg_print("I am currently out of stock.");
		}
		return;
	}


	/* Prompt */
	if (rogue_like_commands)
		sprintf(out_val, "Which item do you want to examine? ");
	else
		sprintf(out_val, "Which item do you want to look at? ");

	/* Get the item number to be examined */
	if (!get_stock(&item, out_val, store_index)) return;

	/* Get the actual object */
	o_ptr = &st_ptr->stock[item];

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	msg_format("Examining %s...", o_name);

	msg_print("");

	/* Save the screen */
	screen_save();

	/* Describe */
	screen_object(o_ptr);

	(void)anykey();

	/* Browse books */
	if (inven_book_okay(o_ptr)) player_browse_object(o_ptr);

	/* Load the screen */
	screen_load();

	return;
}



/*
 * Hack -- set this to leave the store
 */
static bool leave_store = FALSE;


/*
 * Process a command in a store
 *
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 *
 * Hack -- note the bizarre code to handle the "=" command,
 * which is needed to prevent the "redraw" from affecting
 * the display of the store.  XXX XXX XXX
 */
static void store_process_command(char *choice, int store_index)
{
	store_type *st_ptr = store[store_index];

#ifdef ALLOW_REPEAT

	/* Handle repeating the last command */
	repeat_check();

#endif /* ALLOW_REPEAT */

	/* MegaHack -- have sensitive areas of the store to click */
	if (p_ptr->command_cmd == '\xff')
	{
		int y = p_ptr->command_cmd_ex.mousey;
		int x = p_ptr->command_cmd_ex.mousex;

		if ((y >= 6) && (y < 6 + store_size) && (*choice != 'g') && (*choice != 'l')) *choice = 'g';
		else if ((y == 6 + store_size) && (x > 2) && (x < 10) && (!store_top)) *choice = ' ';
		else if ((y == 10 + store_size) && (x < 31)) *choice = ESCAPE;
		else if ((y == 10 + store_size) && (x < 55)) *choice = 'g';
		else if (y == 10 + store_size) *choice = 'l';
		else if ((y == 11 + store_size) && (x < 31)) *choice = ' ';
		else if (((st_ptr->base < STORE_MIN_BUY_SELL) || ((adult_no_selling) == 0)) && ((y == 11 + store_size) && (x < 55))) *choice = 'd';

		/* Hack -- execute command */
		if (p_ptr->command_cmd_ex.mousebutton)
		{
			p_ptr->command_cmd = *choice;
		}

		else return;
	}

	/* Parse the command */
	switch (p_ptr->command_cmd)
	{
		/* Leave */
		case ESCAPE:
		{
			leave_store = TRUE;
			break;
		}

		/* Browse */
		case ' ':
		{
			if (st_ptr->stock_num <= store_size)
			{
				/* Nothing to see */
				msg_print("Entire inventory is shown.");
			}

			else if (store_top == 0)
			{
				/* Page 2 */
				store_top = store_size;

				/* Redisplay wares */
				display_inventory(store_index);
			}

			else
			{
				/* Page 1 */
				store_top = 0;

				/* Redisplay wares */
				display_inventory(store_index);
			}

			break;
		}

		/* Ignore */
		case '\n':
		case '\r':
		{
			break;
		}


		/* Redraw */
		case KTRL('R'):
		{
			do_cmd_redraw();
			display_store(store_index);
			break;
		}

		/* Get (purchase) */
		case 'g':
		{
			store_purchase(store_index);
			*choice = 'g';
			break;
		}

		/* Drop (Sell) */
		case 'd':
		{
			store_sell(store_index);
			*choice = 'd';
			break;
		}

		/* Examine */
		case 'l':
		{
			store_examine(store_index);
			*choice = 'x';
			break;
		}


		/*** Inventory Commands ***/

		/* Wear/wield equipment */
		case 'w':
		{
			do_cmd_item(COMMAND_ITEM_WIELD);
			break;
		}

		/* Take off equipment */
		case 't':
		{
			do_cmd_item(COMMAND_ITEM_TAKEOFF);
			break;
		}

#if 0

		/* Drop an item */
		case 'd':
		{
			do_cmd_drop();
			break;
		}

#endif

		/* Destroy an item */
		case 'k':
		{
			do_cmd_item(COMMAND_ITEM_DESTROY);
			break;
		}

		/* Equipment list */
		case 'e':
		{
			do_cmd_equip();
			break;
		}

		/* Inventory list */
		case 'i':
		{
			do_cmd_inven();
			break;
		}


		/*** Various commands ***/

		/* Identify an object */
		case 'I':
		{
			do_cmd_item(COMMAND_ITEM_EXAMINE);
			break;
		}

		/* Hack -- toggle windows */
		case KTRL('E'):
		{
			toggle_inven_equip();
			break;
		}



		/*** Use various objects ***/

		/* Browse a book */
		case 'b':
		{
			do_cmd_item(COMMAND_ITEM_BROWSE);
			break;
		}

		/* Inscribe an object */
		case '{':
		{
			do_cmd_item(COMMAND_ITEM_INSCRIBE);
			break;
		}

		/* Uninscribe an object */
		case '}':
		{
			do_cmd_item(COMMAND_ITEM_UNINSCRIBE);
			break;
		}



		/*** Help and Such ***/

		/* Help */
		case '?':
		{
			do_cmd_help();
			break;
		}

		/* Identify symbol */
		case '/':
		{
			do_cmd_query_symbol();
			break;
		}

		/* Character description */
		case 'C':
		{
			do_cmd_change_name();
			break;
		}


		/*** System Commands ***/

		/* Hack -- User interface */
		case '!':
		{
			(void)Term_user(0);
			break;
		}

		/* Single line from a pref file */
		case '"':
		{
			do_cmd_pref();
			break;
		}

		/* Interact with macros */
		case '@':
		{
			do_cmd_macros();
			break;
		}

		/* Interact with visuals */
		case '%':
		{
			do_cmd_visuals();
			break;
		}

		/* Interact with colors */
		case '&':
		{
			do_cmd_colors();
			break;
		}

		/* Interact with options */
		case '=':
		{
			do_cmd_menu(MENU_OPTIONS, "options");
			do_cmd_redraw();
			display_store(store_index);
			break;
		}


		/*** Misc Commands ***/

		/* Take notes */
		case ':':
		{
			do_cmd_note();
			break;
		}

		/* Version info */
		case 'V':
		{
			do_cmd_version();
			break;
		}

		/* Repeat level feeling */
		case KTRL('F'):
		{
			do_cmd_feeling();
			break;
		}

		/* Show previous message */
		case KTRL('O'):
		{
			do_cmd_message_one();
			break;
		}

		/* Show previous messages */
		case KTRL('P'):
		{
			do_cmd_messages();
			break;
		}

		/* Check knowledge */
		case '~':
		case '|':
		{
			do_cmd_menu(MENU_KNOWLEDGE, "knowledge");
			break;
		}

		/* Load "screen dump" */
		case '(':
		{
			do_cmd_load_screen();
			break;
		}

		/* Save "screen dump" */
		case ')':
		{
			do_cmd_save_screen();
			break;
		}

		/* Hack -- Unknown command */
		default:
		{
			msg_print("That command does not work in stores.");
			break;
		}
	}
}


/*
 * Enter a store, and interact with it.
 *
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "p_ptr->command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 */
void do_cmd_store(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int feat = cave_feat[py][px];

	int i;

	int tmp_chr;

	dungeon_zone *zone = &t_info[0].zone[0];

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	char choice = '\0';

	int store_index = 0;

	store_type *st_ptr;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Verify a store */
	if (!(f_info[feat].flags1 & (FF1_ENTER)))
	{
		msg_print("You see no store here.");
		return;
	}

	/* Hack -- Extract the store code */
	for (i = 0; i < MAX_STORES; i++)
	{
		if (t_ptr->store[i] == feat)
		{
			store_index = t_ptr->store_index[i];
			break;
		}
	}

	/* Hack -- Check the "locked doors". Restricted or closed store. */
	if (adult_no_stores ||
		(store[store_index]->store_open >= turn))
	{
		msg_print("The doors are locked.");
		return;
	}

	/* Hack -- Check the "locked doors". Restrict unusual shapes. */
	if (p_ptr->pshape >= z_info->g_max)
	{
		cptr closed_reason;

		switch(store_index % 4)
		{
			case 0:
				closed_reason = "You can't open the shop door in this form.";
				break;
			case 1:
				closed_reason =	"A sign reads 'No animals allowed'.";
				break;
			case 2:
				closed_reason =	"The shopkeeper chases you out with a broom.";
				break;
			default:
				closed_reason = "The shopkeeper feeds you a scrap, picks you up and puts you gently down outside.";
				break;
		}
		msg_print(closed_reason);
		return;
	}

	/* Doors locked if player has visited a location to release a unique and hasn't killed it */
	if ((t_ptr->town_lockup_monster) && (r_info[t_ptr->town_lockup_monster].max_num > 0)
		&& (!(t_ptr->town_lockup_ifvisited) || t_info[t_ptr->town_lockup_ifvisited].visited))
	{
		cptr closed_reason;
		char m_name[80];
		
		/* Get the name */
		race_desc(m_name, sizeof(m_name), t_ptr->town_lockup_monster, 0x08, 1);
		
		switch(store_index % 4)
		{
			case 0:
				closed_reason = "The shop front has been damaged by %s and boarded up.";
				break;
			case 1:
				closed_reason =	"A sign reads 'Closed for business due to %s.'";
				break;
			case 2:
				closed_reason =	"The shopkeeper yells from behind a barred door 'You bought %s upon us. Begone!'";
				break;
			default:
				closed_reason = "'It's not safe here with %s about.' the shopkeeper whispers before closing the shutters.";
				break;
		}

		msg_format(closed_reason, m_name);
		return;
	}

	/* Doors locked if guardian about */
	if ((actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone)) && (r_info[actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone)].cur_num > 0))
	{
		cptr closed_reason;
		char m_name[80];
		
		/* Get the name */
		race_desc(m_name, sizeof(m_name), actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone), 0x08, 1);
		
		switch(store_index % 4)
		{
			case 0:
				closed_reason = "A sign reads 'Trespassers not welcome. Signed %s.'";
				break;
			case 1:
				closed_reason =	"The door is locked. %s must be about.";
				break;
			case 2:
				closed_reason =	"A voice yells from behind a barred door 'Leave before I call for %s.'";
				break;
			default:
				closed_reason = "'Come back when %s isn't around.' a voice whispers before closing the shutters.";
				break;
		}

		msg_format(closed_reason, m_name);
		return;
	}

	/* Hack -- homes are special */
	if (f_info[feat].d_char == '8')
	{
		/* Change the home */
		p_ptr->town = p_ptr->dungeon;

		/* Set the new home */
		store[STORE_HOME]->index = f_info[feat].power;
	}

	/* Store has not been entered before -- skip home */
	else if (store_index == 0)
	{
		/* Initialise store */
		store_index = store_init(feat);

		/* Set the town index */
		t_ptr->store_index[i] = store_index;

		/* Maintain store */
		for (i = 0; i < 10; i++)
		{
			store_maint(store_index);
		}
	}

	/* Forget the view */
	forget_view();

	/* Hack -- Increase "icky" depth */
	character_icky++;

	/* No command argument */
	p_ptr->command_arg = 0;

	/* No repeated command */
	p_ptr->command_rep = 0;

	/* No automatic command */
	p_ptr->command_new.key = 0;

	/* Start at the beginning */
	store_top = 0;

	/* Get the store */
	st_ptr = store[store_index];

	/* Display the store */
	display_store(store_index);

	/* Do not leave */
	leave_store = FALSE;

	/* Interact with player */
	while (!leave_store)
	{
		/* Hack -- Clear line 1 */
		prt("", 1, 0);

		/* Hack -- Check the charisma */
		tmp_chr = p_ptr->stat_use[A_CHR];

		/* Clear */
		clear_from(9 + store_size);

		/* Basic commands */
		c_prt(choice == ESCAPE ? TERM_L_BLUE : TERM_WHITE, " ESC) Exit from Building.", 10 + store_size, 0);

		/* Browse if necessary */
		if (st_ptr->stock_num > store_size)
		{
			c_prt(choice == ' ' ? TERM_L_BLUE : TERM_WHITE, " SPACE) Next page of stock", 11 + store_size, 0);
		}

		/* Commands */
		c_prt(choice == 'g' ? TERM_L_BLUE : TERM_WHITE, " g) Get/Purchase an item.", 10 + store_size, 31);
		if ((st_ptr->base < STORE_MIN_BUY_SELL) || ((adult_no_selling) == 0)) c_prt(choice == 'd' ? TERM_L_BLUE : TERM_WHITE, " d) Drop/Sell an item.", 11 + store_size, 31);

		/* Add in the eXamine option */
		if (rogue_like_commands)
			c_prt(choice == 'l' ? TERM_L_BLUE : TERM_WHITE, " x) eXamine an item.", 10 + store_size, 56);
		else
			c_prt(choice == 'l' ? TERM_L_BLUE : TERM_WHITE, " l) Look at an item.", 10 + store_size, 56);

		/* Prompt */
		prt("You may: ", 9 + store_size, 0);

		/* Get a command */
		request_command(TRUE);

		/* Process the command */
		store_process_command(&choice, store_index);

		/* Notice stuff */
		notice_stuff();

		/* Handle stuff */
		handle_stuff();

		/* Pack Overflow XXX XXX XXX */
		if (inventory[INVEN_PACK].k_idx)
		{
			int item = INVEN_PACK;

			object_type *o_ptr = &inventory[item];

			/* Hack -- Flee from the store */
			if (st_ptr->base >= STORE_MIN_BUY_SELL)
			{
				/* Message */
				msg_print("Your pack is so full that you flee the store...");

				/* Leave */
				leave_store = TRUE;
			}

			/* Hack -- Flee from the home */
			else if (!store_check_num(o_ptr, store_index))
			{
				/* Message */
				msg_print("Your pack is so full that you flee from here...");

				/* Leave */
				leave_store = TRUE;
			}

			/* Hack -- Drop items into the home */
			else
			{
				int item_pos;

				object_type *i_ptr;
				object_type object_type_body;

				char o_name[80];

				/* Give a message */
				msg_print("Your pack overflows!");

				/* Forget about item */
				inven_drop_flags(o_ptr);

				/* Get local object */
				i_ptr = &object_type_body;

				/* Grab a copy of the object */
				object_copy(i_ptr, o_ptr);

				/* Describe it */
				object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

				/* Message */
				msg_format("You drop %s (%c).", o_name, index_to_label(item));

				/* Remove it from the players inventory */
				inven_item_increase(item, -255);
				inven_item_describe(item);
				inven_item_optimize(item);

				/* Handle stuff */
				handle_stuff();

				/* Let the home carry it */
				item_pos = home_carry(i_ptr, store_index);

				/* Redraw the home */
				if (item_pos >= 0)
				{
					/* Redisplay wares */
					display_inventory(store_index);
				}
			}
		}

		/* Hack -- Handle charisma changes */
		if (tmp_chr != p_ptr->stat_use[A_CHR])
		{
			/* Redisplay wares */
			display_inventory(store_index);
		}

		/* Hack -- get kicked out of the store */
		if (st_ptr->store_open >= turn) leave_store = TRUE;

		/* Hack -- leave store if leaving */
		if (p_ptr->leaving) leave_store = TRUE;
	}


	/* Take a turn */
	p_ptr->energy_use = 100;


	/* Hack -- Cancel automatic command */
	p_ptr->command_new.key = 0;

	/* Hack -- Cancel "see" mode */
	p_ptr->command_see = FALSE;

	/* Message from store */
	if (strlen(f_text + f_info[cave_feat[p_ptr->py][p_ptr->px]].text))
	{
		msg_print(f_text + f_info[cave_feat[p_ptr->py][p_ptr->px]].text);
	}

	/* Flush messages XXX XXX XXX */
	msg_print(NULL);


	/* Hack -- Decrease "icky" depth */
	character_icky--;


	/* Clear the screen */
	Term_clear();


	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw entire screen */
	p_ptr->redraw |= (PR_BASIC | PR_EXTRA);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}



/*
 * Shuffle one of the stores.
 */
void store_shuffle(int store_index)
{
	int i;

	store_type *st_ptr = store[store_index];
	owner_type *ot_ptr;

	/* Justifiable paranoia */
	if (!st_ptr) return;

	if (st_ptr->base < STORE_MIN_BUY_SELL)
	{
		return;
	}
	else
	{
		/* Release the previous owner */
		ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max) + st_ptr->owner];
		ot_ptr->busy = FALSE;
		{
			int j = st_ptr->owner;
			int count = 0;
			do
			{
				count++;

				/* Pick an owner */
				st_ptr->owner = (byte)rand_int(z_info->b_max);

				/* Activate the new owner */
				ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max)
								  + st_ptr->owner];
			}
			while (j == st_ptr->owner
					 || (count < 500 && ot_ptr->busy)
					 || (count < 1000 && !ot_ptr->owner_name));
		}
	}

	if (!ot_ptr || !ot_ptr->owner_name)
	{
		st_ptr->owner = 0;

		/* Activate the new owner */
		ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max)
							  + st_ptr->owner];
	}
	ot_ptr->busy = TRUE;

	/* Reset the owner data */
	st_ptr->insult_cur = 0;
	st_ptr->store_open = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;


	/* Discount all the items */
	for (i = 0; i < st_ptr->stock_num; i++)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &st_ptr->stock[i];

		/* Discount non-discounted items by 40 percent */
		if (o_ptr->discount == 0) o_ptr->discount = 40;

		/* Clear the "fixed price" flag */
		o_ptr->ident &= ~(IDENT_FIXED);
	}
}


/*
 * Maintain the inventory at the stores.
 */
void store_maint(int store_index)
{
	int j;

	int tries = 0;

	int old_rating = rating;

	int services = 0;

	store_type *st_ptr = store[store_index];

	if (st_ptr->base < STORE_MIN_BUY_SELL) return;

	/* Store keeper forgives the player */
	st_ptr->insult_cur = 0;

	/* Count services to exclude them later */
	for (j = 0; j < st_ptr->stock_num; j++)
	{
		object_type *i_ptr = &(st_ptr->stock[j]);

		if (i_ptr->tval == TV_SERVICE) services++;
	}

	/* Choose the number of slots to keep */
	j = st_ptr->stock_num;

	/* Sell a few items */
	j = j - randint(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP + services) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP + services) j = STORE_MIN_KEEP;

	/* Hack -- prevent "underflow" */
	if (j < 0) j = 0;

	/* Destroy objects until only "j" slots are left -- fail if tried too many times */
	while ((st_ptr->stock_num > j) && (tries++ < 100))
		store_delete(store_index);

	/* Reset tries */
	tries = 0;

	/* Choose the number of slots to fill */
	j = st_ptr->stock_num;

	/* Buy some more items */
	j = j + randint(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;

	/* Hack -- prevent "overflow" */
	if (j >= st_ptr->stock_size) j = st_ptr->stock_size - 1;

	/* Create some new items -- fail if tried too many times */
	while ((st_ptr->stock_num < j) && (tries++ < 100))
		store_create(store_index);

	/* Hack -- Restore the rating */
	rating = old_rating;
}


/*
 * Initialize a new store
 */
int store_init(int feat)
{
	int k;
	int store_index = 0;

	store_type *st_ptr;
	owner_type *ot_ptr = NULL;

	/* Paranoia */
	if (total_store_count >= max_store_count)
	{
		/* Oops */
		msg_print("BUG: Maximum number of stores reached.");

		/* Hack -- access home instead */
		return (0);
	}

	/* Make a new store */
	st_ptr = C_ZNEW(1, store_type);

	/* Copy basic store information to it */
	COPY(st_ptr, &u_info[f_info[feat].power], store_type);

	/* Hack -- ensure that school books are stocked */
	if (p_ptr->pschool)
	{
		for (k = 0; k < STORE_CHOICES; k++)
		{
			int tval = st_ptr->tval[k];

			/* Check books */
			if (((tval == c_info[p_ptr->pclass].spell_book)
				|| ((tval == TV_MAGIC_BOOK) && (p_ptr->pstyle == WS_MAGIC_BOOK))
				|| ((tval == TV_PRAYER_BOOK) && (p_ptr->pstyle == WS_PRAYER_BOOK))
				|| ((tval == TV_SONG_BOOK) && (p_ptr->pstyle == WS_SONG_BOOK)))
				&& (st_ptr->sval[k] >= SV_BOOK_MAX_GOOD))
			{
				/* Use school book instead */
				st_ptr->sval[k] += p_ptr->pschool - SV_BOOK_MAX_GOOD;
			}
		}
	}

	/* Assume full stock */
	st_ptr->stock_size = STORE_INVEN_MAX;

	/* Allocate the stock */
	st_ptr->stock = C_ZNEW(st_ptr->stock_size, object_type);

	/* Store index */
	store_index = total_store_count++;

	/* Add it to the list of stores */
	store[store_index] = st_ptr;

	/* Set the store level */
	st_ptr->level = f_info[feat].level;

	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		int count = 0;
		do
		{
			count++;

			/* Pick an owner */
			st_ptr->owner = (byte)rand_int(z_info->b_max);

			/* Activate the new owner */
			ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max)
								  + st_ptr->owner];
		}
		while ((count < 500 && ot_ptr->busy)
				 || (count < 1000 && !ot_ptr->owner_name));
	}

	if (!ot_ptr || !ot_ptr->owner_name)
	{
		st_ptr->owner = 0;

		/* Activate the new owner */
		ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max)
							  + st_ptr->owner];
	}
	ot_ptr->busy = TRUE;

	/* Initialize the store */
	st_ptr->store_open = 0;
	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;

	/* Nothing in stock */
	st_ptr->stock_num = 0;

	/* Clear any old items */
	for (k = 0; k < st_ptr->stock_size; k++)
	{
		object_wipe(&st_ptr->stock[k]);
	}

	/* Stock services */
	for (k = 0;k < STORE_CHOICES;k++)
	{
		int k_idx;
		object_type *i_ptr;
		object_type object_type_body;

		if (st_ptr->tval[k] == TV_SERVICE)
		{
			k_idx = lookup_kind(st_ptr->tval[k],st_ptr->sval[k]);

			/* Get local object */
			i_ptr = &object_type_body;

			/* Create a new object of the chosen kind */
			object_prep(i_ptr, k_idx);

			/* The object is "known" */
			object_aware(i_ptr, TRUE);
			object_known(i_ptr);

			/* The object belongs to a store */
			i_ptr->ident |= (IDENT_STORE);

			/* Carry the object in the store */
			(void) store_carry(i_ptr, store_index);

		}
	}

	/* Stock items */
	if (st_ptr->base > STORE_HOME)
	{
		/* Create some new items */
		while (st_ptr->stock_num < ((st_ptr->base == STORE_STORAGE) || (st_ptr->base == STORE_QUEST_REWARD) ? 3 + rand_int(3) : 8)) store_create(store_index);
	}


	return (store_index);
}
