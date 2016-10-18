/* File: load.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
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


/*
 * This file loads savefiles from Angband 2.9.X.
 *
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * This file assumes that the monster/object records are initialized
 * to zero, and the race/kind tables have been loaded correctly.  The
 * order of object stacks is currently not saved in the savefiles, but
 * the "next" pointers are saved, so all necessary knowledge is present.
 *
 * We should implement simple "savefile extenders" using some form of
 * "sized" chunks of bytes, with a {size,type,data} format, so everyone
 * can know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
 *
 * Consider changing the "globe of invulnerability" code so that it
 * takes some form of "maximum damage to protect from" in addition to
 * the existing "number of turns to protect for", and where each hit
 * by a monster will reduce the shield by that amount.  XXX XXX XXX
 */





/*
 * Local "savefile" pointer
 */
static FILE	*fff;

/*
 * Hack -- old "encryption" byte
 */
static byte	xor_byte;

/*
 * Hack -- simple "checksum" on the actual values
 */
static u32b	v_check = 0L;

/*
 * Hack -- simple "checksum" on the encoded bytes
 */
static u32b	x_check = 0L;


/*
 * Hack -- Show information on the screen, one line at a time.
 *
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
static void note(cptr msg)
{
	static int y = 2;

	/* Draw the message */
	prt(msg, y, 0);

	/* Advance one line (wrap if needed) */
	if (++y >= 24) y = 2;

	/* Flush it */
	Term_fresh();
}


/*
 * This function determines if the version of the savefile
 * currently being read is older than version "x.y.z".
 *
 * Warning: DO NOT attempt to check sf_extra, as this is
 * used as an xor byte, as opposed to version control.
 */
bool older_than(int x, int y, int z, int w)
{
	/* Much older, or much more recent */
	if (sf_major < x) return (TRUE);
	if (sf_major > x) return (FALSE);

	/* Distinctly older, or distinctly more recent */
	if (sf_minor < y) return (TRUE);
	if (sf_minor > y) return (FALSE);

	/* Barely older, or barely more recent */
	if (sf_patch < z) return (TRUE);
	if (sf_patch > z) return (FALSE);

	/* The maintainer is lazy and doesn't want to bump the version number in the edit files */
	if (sf_extra < w) return (TRUE);
	if (sf_extra > w) return (FALSE);

	/* Identical versions */
	return (FALSE);
}


/*
 * Hack -- determine if an item is "wearable" (or a missile)
 */
static bool wearable_p(const object_type *o_ptr)
{
	/* Valid "tval" codes */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_INSTRUMENT:
		case TV_STAFF:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_LITE:
		case TV_AMULET:
		case TV_RING:
		{
			return (TRUE);
		}
	}

	/* Nope */
	return (FALSE);
}


/*
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info.
 */

static byte sf_get(void)
{
	byte c, v;

	/* Get a character, decode the value */
	c = getc(fff) & 0xFF;
	v = c ^ xor_byte;
	xor_byte = c;

	/* Maintain the checksum info */
	v_check += v;
	x_check += xor_byte;

	/* Return the value */
	return (v);
}

static void rd_byte(byte *ip)
{
	*ip = sf_get();
}

static void rd_u16b(u16b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u16b)(sf_get()) << 8);
}

static void rd_s16b(s16b *ip)
{
	rd_u16b((u16b*)ip);
}

static void rd_u32b(u32b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u32b)(sf_get()) << 8);
	(*ip) |= ((u32b)(sf_get()) << 16);
	(*ip) |= ((u32b)(sf_get()) << 24);
}

static void rd_s32b(s32b *ip)
{
	rd_u32b((u32b*)ip);
}


/*
 * Hack -- read a string
 */
static void rd_string(char *str, int max)
{
	int i;

	/* Read the string */
	for (i = 0; TRUE; i++)
	{
		byte tmp8u;

		/* Read a byte */
		rd_byte(&tmp8u);

		/* Collect string while legal */
		if (i < max) str[i] = tmp8u;

		/* End of string */
		if (!tmp8u) break;
	}

	/* Terminate */
	str[max-1] = '\0';
}


/*
 * Hack -- strip some bytes
 */
static void strip_bytes(int n)
{
	byte tmp8u;

	/* Strip the bytes */
	while (n--) rd_byte(&tmp8u);
}


/*
 * Read an object
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 */
static errr rd_item(object_type *o_ptr)
{
	u32b f1, f2, f3, f4;

	object_kind *k_ptr;

	char buf[128];

	/* Kind */
	rd_s16b(&o_ptr->k_idx);

	/* Paranoia */
	if ((o_ptr->k_idx < 0) || (o_ptr->k_idx >= z_info->k_max))
		return (-1);

	/* Location */
	rd_byte(&o_ptr->iy);
	rd_byte(&o_ptr->ix);

	/* Type/Subtype */
	rd_byte(&o_ptr->tval);
	rd_byte(&o_ptr->sval);

	/* Special pval */
	rd_s16b(&o_ptr->pval);

	/* Special stack counter */
	rd_byte(&o_ptr->stackc);

	rd_byte(&o_ptr->show_idx);
	rd_byte(&o_ptr->discount);

	rd_byte(&o_ptr->feeling);
	rd_byte(&o_ptr->spare);

	rd_byte(&o_ptr->number);
	rd_s16b(&o_ptr->weight);

	rd_byte(&o_ptr->name1);
	rd_byte(&o_ptr->name2);

	rd_s16b(&o_ptr->timeout);

	/* Get charges */
	rd_s16b(&o_ptr->charges);

	/* Hack -- fix missing charges from flasks of oil */
	if ((o_ptr->tval == TV_FLASK
		 || o_ptr->tval == TV_FOOD || o_ptr->tval == TV_MUSHROOM)
		&& !o_ptr->charges)
		o_ptr->charges = k_info[o_ptr->k_idx].charges;

	rd_s16b(&o_ptr->to_h);
	rd_s16b(&o_ptr->to_d);
	rd_s16b(&o_ptr->to_a);

	rd_s16b(&o_ptr->ac);

	rd_byte(&o_ptr->dd);
	rd_byte(&o_ptr->ds);

	rd_u16b(&o_ptr->ident);

	/* Hack -- remove chests */
	if (o_ptr->tval == TV_CHEST) o_ptr->k_idx = 0;

	rd_byte(&o_ptr->origin);
	rd_byte(&o_ptr->origin_depth);
	rd_u16b(&o_ptr->origin_xtra);

	/* Old flags */
	strip_bytes(8);

	/* Monster holding object */
	rd_s16b(&o_ptr->held_m_idx);

	/* Special powers */
	rd_byte(&o_ptr->xtra1);
	rd_byte(&o_ptr->xtra2);

	/* Flags we have learnt about an item */
	rd_u32b(&o_ptr->can_flags1);
	rd_u32b(&o_ptr->can_flags2);
	rd_u32b(&o_ptr->can_flags3);
	rd_u32b(&o_ptr->can_flags4);

	rd_u32b(&o_ptr->may_flags1);
	rd_u32b(&o_ptr->may_flags2);
	rd_u32b(&o_ptr->may_flags3);
	rd_u32b(&o_ptr->may_flags4);

	rd_u32b(&o_ptr->not_flags1);
	rd_u32b(&o_ptr->not_flags2);
	rd_u32b(&o_ptr->not_flags3);
	rd_u32b(&o_ptr->not_flags4);

	/* Times we have used an item */
	rd_s16b(&o_ptr->usage);

	/* Guessed an item as */
	rd_byte(&o_ptr->guess1);
	rd_byte(&o_ptr->guess2);

	/* Item has a monster 'flavor' */
	rd_s16b(&o_ptr->name3);

	/* Inscription */
	rd_string(buf, 128);

	/* Save the inscription */
	if (buf[0]) o_ptr->note = quark_add(buf);

	/* Obtain the "kind" template */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Obtain tval/sval from k_info */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;


	/* Hack -- notice "broken" items */
	/*if (k_ptr->cost <= 0) o_ptr->ident |= (IDENT_BROKEN);*/


	/* Repair non "wearable" items */
	if (!wearable_p(o_ptr))
	{
		/* Get the correct fields */
		o_ptr->to_h = k_ptr->to_h;
		o_ptr->to_d = k_ptr->to_d;
		o_ptr->to_a = k_ptr->to_a;

		/* Get the correct fields */
		o_ptr->ac = k_ptr->ac;
		o_ptr->dd = k_ptr->dd;
		o_ptr->ds = k_ptr->ds;

		/* Get the correct weight */
		o_ptr->weight = k_ptr->weight;

		/* Paranoia */
		o_ptr->name1 = o_ptr->name2 = 0;

		/* All done */
		return (0);
	}


	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);


	/* Paranoia */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr;

		/* Obtain the artifact info */
		a_ptr = &a_info[o_ptr->name1];

		/* Verify that artifact */
		if (!a_ptr->name) o_ptr->name1 = 0;

		/* Ensure that one of this exists */
		a_ptr->cur_num = 1;
	}

	/* Paranoia */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr;

		/* Paranoia */
		if (o_ptr->name2 >= z_info->e_max) return (-1);

		/* Obtain the ego-item info */
		e_ptr = &e_info[o_ptr->name2];

		/* Verify that ego-item */
		if (!e_ptr->name) o_ptr->name2 = 0;
	}

	/* Get the standard weight */
	o_ptr->weight = k_ptr->weight;

	/* Hack -- extract the "broken" flag */
	/*if (o_ptr->pval < 0) o_ptr->ident |= (IDENT_BROKEN);*/

#if 0
/* Do not overwrite randarts from old versions! */

	/* Artifacts */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr;

		/* Obtain the artifact info */
		a_ptr = &a_info[o_ptr->name1];

		/* Get the new artifact "pval" */
		o_ptr->pval = a_ptr->pval;

		/* Get the new artifact fields */
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;

		/* Get the new artifact weight */
		o_ptr->weight = a_ptr->weight;

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);
	}
#endif

	/* Ego items */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr;

		/* Obtain the ego-item info */
		e_ptr = &e_info[o_ptr->name2];

		/* Hack -- extract the "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- enforce legal pval */
		if ((e_ptr->flags1 & (TR1_PVAL_MASK)) || (e_ptr->flags3 & (TR3_PVAL_MASK)))
		{
			/* Force a meaningful pval */
			if (!o_ptr->pval) o_ptr->pval = 1;
		}



		/* Mega-Hack - Enforce the special broken items */
		if ((o_ptr->name2 == EGO_BLASTED) ||
			(o_ptr->name2 == EGO_SHATTERED))
		{
			/* These were set to k_info values by preceding code */
			o_ptr->ac = 0;
			o_ptr->dd = 0;
			o_ptr->ds = 0;
		}
	}

	/* Hack -- clear empty slots */
	if (!o_ptr->k_idx) object_wipe(o_ptr);

	/* Success */
	return (0);
}


/*
 * Read a "region" record
 */
static void rd_region(region_type *r_ptr)
{
	/* Read the region */
	rd_s16b(&r_ptr->type);

	rd_s16b(&r_ptr->who);          	/* Source of effect - 'who'. */
	rd_s16b(&r_ptr->what);			/* Source of effect - 'what'. */

	rd_s16b(&r_ptr->effect);
	rd_s16b(&r_ptr->method);
	rd_byte(&r_ptr->level);
	rd_byte(&r_ptr->facing);		/* Region facing */

	rd_s16b(&r_ptr->damage);

	if (!older_than(0,6,3,7))
	{
		rd_s16b(&r_ptr->child_region);
	}

	rd_s16b(&r_ptr->delay);		     /* Number of turns effect has left */
	rd_s16b(&r_ptr->delay_reset);			/* Number of turns to reset counter to when countdown has finished */
	rd_s16b(&r_ptr->age);			/* Number of turns effect has been alive */
	rd_s16b(&r_ptr->lifespan);		/* Number of turns effect will be alive */

	rd_byte(&r_ptr->y0);			/* Source y location */
	rd_byte(&r_ptr->x0);			/* Source x location */
	rd_byte(&r_ptr->y1);			/* Destination y location */
	rd_byte(&r_ptr->x1);			/* Destination x location */

	rd_u32b(&r_ptr->flags1);		/* Ongoing effect bitflags */
}


/*
 * Read a "region piece" record
 */
static void rd_region_piece(region_piece_type *rp_ptr)
{
	/* Read the region piece */
	rd_byte(&rp_ptr->y);
	rd_byte(&rp_ptr->x);
	rd_s16b(&rp_ptr->d);
	rd_s16b(&rp_ptr->region);

}


/*
 * Read a monster
 */
static void rd_monster(monster_type *m_ptr)
{
	/* Read the monster race */
	rd_s16b(&m_ptr->r_idx);

	/* Increase count of this race */
	r_info[m_ptr->r_idx].cur_num++;

	/* Read the other information */
	rd_byte(&m_ptr->fy);
	rd_byte(&m_ptr->fx);
	rd_s16b(&m_ptr->hp);
	rd_s16b(&m_ptr->maxhp);
	rd_s16b(&m_ptr->csleep);
	rd_byte(&m_ptr->mspeed);
	rd_byte(&m_ptr->energy);
	rd_byte(&m_ptr->stunned);
	rd_byte(&m_ptr->confused);
	rd_byte(&m_ptr->monfear);
	rd_byte(&m_ptr->slowed);
	rd_byte(&m_ptr->hasted);
	rd_byte(&m_ptr->cut);
	rd_byte(&m_ptr->poisoned);
	rd_byte(&m_ptr->blind);
	if (!older_than(0, 6, 4, 3))
	{
		rd_byte(&m_ptr->dazed);
		rd_byte(&m_ptr->image);
		rd_byte(&m_ptr->amnesia);
		rd_byte(&m_ptr->terror);
	}
	rd_byte(&m_ptr->tim_invis);
	rd_byte(&m_ptr->tim_passw);
	rd_byte(&m_ptr->bless);
	rd_byte(&m_ptr->berserk);
	rd_byte(&m_ptr->shield);
	rd_byte(&m_ptr->oppose_elem);
	rd_byte(&m_ptr->summoned);
	rd_byte(&m_ptr->petrify);
	rd_byte(&m_ptr->facing);

	rd_u32b(&m_ptr->mflag);
	rd_u32b(&m_ptr->smart);

	rd_byte(&m_ptr->min_range);
	rd_byte(&m_ptr->best_range);
	rd_byte(&m_ptr->ty);
	rd_byte(&m_ptr->tx);

}





/*
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
	byte tmp8u;

	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[r_idx];


	/* Count sights/deaths/kills */
	rd_s16b(&l_ptr->sights);
	rd_s16b(&l_ptr->deaths);
	rd_s16b(&l_ptr->pkills);
	rd_s16b(&l_ptr->tkills);
	
	/* Count player attacks */
	if (older_than(0,6,4,1))
	{
		l_ptr->tblows = l_ptr->tkills;
		l_ptr->tdamage = l_ptr->tkills;
	}
	else
	{
		rd_s16b(&l_ptr->tblows);
		rd_s16b(&l_ptr->tdamage);
	}

	/* Count wakes and ignores */
	rd_byte(&l_ptr->wake);
	rd_byte(&l_ptr->ignore);

	/* Extra stuff */
	rd_byte(&l_ptr->xtra1);
	rd_byte(&l_ptr->xtra2);

	/* Count drops */
	rd_byte(&l_ptr->drop_gold);
	rd_byte(&l_ptr->drop_item);

	/* Count spells */
	rd_byte(&l_ptr->cast_innate);
	rd_byte(&l_ptr->cast_spell);

	/* Count blows of each type */
	rd_byte(&l_ptr->blows[0]);
	rd_byte(&l_ptr->blows[1]);
	rd_byte(&l_ptr->blows[2]);
	rd_byte(&l_ptr->blows[3]);

	/* Memorize flags */
	rd_u32b(&l_ptr->flags1);
	rd_u32b(&l_ptr->flags2);
	rd_u32b(&l_ptr->flags3);
	rd_u32b(&l_ptr->flags4);
	rd_u32b(&l_ptr->flags5);
	rd_u32b(&l_ptr->flags6);
	rd_u32b(&l_ptr->flags7);
	rd_u32b(&l_ptr->flags8);
	rd_u32b(&l_ptr->flags9);

	/* Read the "Racial" monster limit per level */
	rd_byte(&r_ptr->max_num);

	/* Later (?) */
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);


	/* Repair the lore flags */
	l_ptr->flags1 &= r_ptr->flags1;
	l_ptr->flags2 &= r_ptr->flags2;
	l_ptr->flags3 &= r_ptr->flags3;
	l_ptr->flags4 &= r_ptr->flags4;
	l_ptr->flags5 &= r_ptr->flags5;
	l_ptr->flags6 &= r_ptr->flags6;
	l_ptr->flags7 &= r_ptr->flags7;
	l_ptr->flags8 &= r_ptr->flags8;
	l_ptr->flags9 &= r_ptr->flags9;

}




/*
 * Read a store
 */
static errr rd_store()
{
	store_type *st_ptr;
	int j,k;

	byte own, num;

	owner_type *ot_ptr;

	/* Paranoia */
	if (total_store_count >= max_store_count)
	{
		/* Oops */
		/*note("Too many stores");*/

		/* Error */
		/*return (-1);*/

		/* Fixing */
		msg_print("Forgetting about some stores.");

		/* Hack -- free the store inventories. Except home. */
		for (j = 1; j < total_store_count; j++)
		{
			/* Get the store */
			store_type *st_ptr = store[j];

			/* Free the store inventory */
			FREE(st_ptr->stock);

			/* Free the store */
			FREE(st_ptr);

			/* Disassociate store */
			store[j] = NULL;
		}

		/* Just home left after fix */
		total_store_count = 1;
	}

	/* Make a new store */
	st_ptr = C_ZNEW(1, store_type);

	/* Read the store index */
	rd_byte(&st_ptr->index);

	/* Copy basic store information to it */
	COPY(st_ptr, &u_info[st_ptr->index], store_type);

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

	/* Add it to the list of stores */
	store[total_store_count++] = st_ptr;

	/* Read the basic info */
	rd_s32b(&st_ptr->store_open);
	rd_s16b(&st_ptr->insult_cur);
	rd_byte(&own);
	rd_byte(&num);
	rd_s16b(&st_ptr->good_buy);
	rd_s16b(&st_ptr->bad_buy);
	rd_byte(&st_ptr->level);

	/* Paranoia */
	if (own >= z_info->b_max)
	{
		note("Illegal store owner!");
		return (-1);
	}

	st_ptr->owner = own;

	/* Activate the owner, if any */
	if (st_ptr->base >= STORE_MIN_BUY_SELL)
	{
		ot_ptr = &b_info[((st_ptr->base - STORE_MIN_BUY_SELL) * z_info->b_max)
							  + st_ptr->owner];
		ot_ptr->busy = TRUE;
	}

	/* Read the items */
	for (j = 0; j < num; j++)
	{
		object_type *i_ptr;
		object_type object_type_body;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Read the item */
		if (rd_item(i_ptr))
		{
			note("Error reading item");
			return (-1);
		}

		/* Accept any valid items */
		if (st_ptr->stock_num < STORE_INVEN_MAX)
		{
			int k = st_ptr->stock_num++;

			/* Accept the item */
			object_copy(&st_ptr->stock[k], i_ptr);
		}
	}

	/* Success */
	return (0);
}



/*
 * Read a random quest
 */
static errr rd_event(int n, int s)
{
	quest_event *qe_ptr = &q_list[n].event[s];

	/* Read the quest flags */
	rd_u32b(&qe_ptr->flags);

	/* Read the quest  */
	rd_byte(&qe_ptr->dungeon);
	rd_byte(&qe_ptr->level);
	rd_byte(&qe_ptr->room);
	rd_byte(&qe_ptr->action);
	rd_s16b(&qe_ptr->feat);
	rd_s16b(&qe_ptr->store);
	rd_s16b(&qe_ptr->race);
	rd_s16b(&qe_ptr->kind);
	rd_s16b(&qe_ptr->number);
	rd_byte(&qe_ptr->artifact);
	rd_byte(&qe_ptr->ego_item_type);
	rd_s16b(&qe_ptr->room_type_a);
	rd_s16b(&qe_ptr->room_type_b);
	rd_u32b(&qe_ptr->room_flags);
	rd_s16b(&qe_ptr->owner);
	rd_s16b(&qe_ptr->power);
	rd_s16b(&qe_ptr->experience);
	rd_s16b(&qe_ptr->gold);

	/* Success */
	return(0);

}


/*
 * Read RNG state
 */
static void rd_randomizer(void)
{
	int i;

	u16b tmp16u;


	/* Tmp */
	rd_u16b(&tmp16u);

	/* Place */
	rd_u16b(&Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		rd_u32b(&Rand_state[i]);
	}

	/* Accept */
	Rand_quick = FALSE;
}



/*
 * Read options
 *
 * Note that the normal options are stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
static void rd_options(void)
{
	int i, n;

	byte b;

	u32b flag[8];
	u32b mask[8];
	u32b window_flag[ANGBAND_TERM_MAX];
	u32b window_mask[ANGBAND_TERM_MAX];

	/*** Oops ***/

	/* Ignore old options */
	strip_bytes(16);


	/*** Special info */

	/* Read "delay_factor" */
	rd_byte(&b);
	op_ptr->delay_factor = b;

	/* Read "hitpoint_warn" */
	rd_byte(&b);
	op_ptr->hitpoint_warn = b;

	/* Old cheating options */
	rd_byte(&b);
	op_ptr->monlist_display = b;

	/* Old cheating options */
	rd_byte(&b);
	op_ptr->monlist_sort_by = b;

	/* Hack -- initialise options for the first time */
	if (!op_ptr->monlist_display)
	{
		op_ptr->monlist_display = 3;
		op_ptr->monlist_sort_by = 2;
		op_ptr->delay_factor = 9;
	}

	/*** Normal Options ***/

	/* Read the option flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the option masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		int os = i / 32;
		int ob = i % 32;

		/* Process real entries */
		if (option_name(i))
		{
			/* Process saved entries */
			if (mask[os] & (1L << ob))
			{
				/* Set flag */
				if (flag[os] & (1L << ob))
				{
					/* Set */
					op_ptr->opt[i] = TRUE;
				}

				/* Clear flag */
				else
				{
					/* Set */
					op_ptr->opt[i] = FALSE;
				}
			}
		}
	}

	/*** Window Options ***/

	/* Read the window flags */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		rd_u32b(&window_flag[n]);
	}

	/* Read the window masks */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		rd_u32b(&window_mask[n]);
	}

	/* Analyze the options */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (window_flag_desc[i])
			{
				/* Process valid flags */
				if (window_mask[n] & (1L << i))
				{
					/* Set */
					if (window_flag[n] & (1L << i))
					{
						/* Set */
						op_ptr->window_flag[n] |= (1L << i);
					}
				}
			}
		}
	}

	/* Hack -- for competition 70 */
	if (older_than(0, 6, 3, 9))
	{
		auto_monlist = FALSE;
	}
}





/*
 * Hack -- strip the "ghost" info
 *
 * XXX XXX XXX This is such a nasty hack it hurts.
 */
static void rd_ghost(void)
{
	char buf[64];

	/* Strip name */
	rd_string(buf, 64);

	/* Strip old data */
	strip_bytes(60);
}

/*
 * Read the "extra" information
 */
static errr rd_extra(void)
{
	int i;

	byte tmp8u;
	u16b tmp16u;

	int a_max = A_MAX;

	rd_string(op_ptr->full_name, 32);

	rd_string(p_ptr->died_from, 80);

	rd_string(p_ptr->history, 250);

	/* Player race */
	rd_byte(&p_ptr->prace);

	/* Verify player race */
	if (p_ptr->prace >= z_info->g_max)
	{
		note(format("Invalid player race (%d).", p_ptr->prace));
		return (-1);
	}

	/* Player class */
	rd_byte(&p_ptr->pclass);

	/* Verify player class */
	if (p_ptr->pclass >= z_info->c_max)
	{
		note(format("Invalid player class (%d).", p_ptr->pclass));
		return (-1);
	}

	/* Player gender */
	rd_byte(&p_ptr->psex);

	/* Player style */
	rd_byte(&p_ptr->pstyle);

	/* Player school */
	rd_byte(&p_ptr->pschool);

	/* Fix -- great book reorganisation */
	if ((older_than(0,6,3,10)) && ((p_ptr->pstyle == WS_MAGIC_BOOK)
			|| (p_ptr->pstyle == WS_PRAYER_BOOK)))
	{
		if (p_ptr->pschool >= 32) p_ptr->pschool = 52 + (p_ptr->pschool - 32) / 4 * 5;
	}

	/* XXX Hack -- should have saved this before */
	if (p_ptr->pschool < SV_BOOK_MAX_GOOD) p_ptr->pschool = SV_BOOK_MAX_GOOD;

	rd_byte(&p_ptr->sauron_forms);

	/* Special Race/Class info */
	rd_byte(&p_ptr->expfact);

	/* Age/Height/Weight */
	rd_s16b(&p_ptr->age);
	rd_s16b(&p_ptr->ht);

	/* Unneeded */
	rd_s16b(&p_ptr->wt);

	/* Read the stat info */
	for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->stat_max[i]);
	for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->stat_cur[i]);

	/* Read the old temporary information */
	if (older_than(0, 6, 3, 4))
	{
		for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->timed[i]);
		for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->timed[i + a_max]);
	}

	character_quickstart = TRUE;

	for (i = 0; i < a_max; i++)
	{
		rd_s16b(&p_ptr->stat_birth[i]);
		if (!p_ptr->stat_birth[i]) character_quickstart = FALSE;
	}

	strip_bytes(24);	/* oops */

	rd_s32b(&p_ptr->au);
	rd_s32b(&p_ptr->birth_au);

	rd_s32b(&p_ptr->max_exp);
	rd_s32b(&p_ptr->exp);
	rd_u16b(&p_ptr->exp_frac);

	rd_s16b(&p_ptr->lev);

	/* Verify player level */
	if ((p_ptr->lev < 1) || (p_ptr->lev > PY_MAX_LEVEL))
	{
		note(format("Invalid player level (%d).", p_ptr->lev));
		return (-1);
	}

	rd_s16b(&p_ptr->mhp);
	rd_s16b(&p_ptr->chp);
	rd_u16b(&p_ptr->chp_frac);

	rd_s16b(&p_ptr->msp);
	rd_s16b(&p_ptr->csp);
	rd_u16b(&p_ptr->csp_frac);

	rd_s16b(&p_ptr->max_lev);
	rd_s16b(&p_ptr->max_depth);

	/* Hack -- Repair maximum player level */
	if (p_ptr->max_lev < p_ptr->lev) p_ptr->max_lev = p_ptr->lev;

	/* Hack -- Repair maximum dungeon level */
	if (p_ptr->max_depth < 0) p_ptr->max_depth = 1;

	/* Hack --- Get psval information. */
	rd_byte(&p_ptr->psval);
	
	/* Fix -- great book reorganisation */
	if ((older_than(0,6,3,10)) && ((p_ptr->pstyle == WS_MAGIC_BOOK)
			|| (p_ptr->pstyle == WS_PRAYER_BOOK)))
	{
		if (p_ptr->psval >= 32) p_ptr->psval = 52 + (p_ptr->psval - 32) / 4 * 5;
	}

	/* Fix -- great book reorganisation */
	if ((older_than(0,6,3,10)) && (p_ptr->pstyle == WS_SONG_BOOK))
	{
		switch (p_ptr->psval)
		{
		case 0: case 1: case 13:
			p_ptr->psval = 52;
			break;
		}
	}

	/* Hack --- Get shape information. */
	rd_byte(&p_ptr->pshape);

	/* Fix -- old saves didn't have pshape set */
	if (!p_ptr->pshape)
	{
		p_ptr->pshape = p_ptr->prace;
	}

	/* Verify player shape */
	if (p_ptr->pshape >= z_info->p_max)
	{
		note(format("Invalid player shape (%d).", p_ptr->pshape));
		return (-1);
	}

	/* Read familiar */
	if (!older_than(0, 6, 3, 2))
	{
		rd_byte(&p_ptr->familiar);

		for (i = 0; i < MAX_FAMILIAR_GAINS; i++)
		{
			rd_u16b(&(p_ptr->familiar_attr[i]));
			
			/* Only bothering with this as it affects the current competition */
			if ((older_than(0, 6, 4, 0)) && (p_ptr->familiar_attr[i] >= FAMILIAR_SHOT))
			{
				p_ptr->familiar_attr[i]++;
			}
		}
	}

	/* Read the timers */
	if (!older_than(0, 6, 3, 4))
	{
		/* Read in some important values which were mixed in with the timers */
		rd_s16b(&p_ptr->sc);
		rd_s16b(&p_ptr->food);
		rd_s16b(&p_ptr->rest);
		rd_s16b(&p_ptr->energy);

		/* Read the player timer array */
		rd_u16b(&tmp16u);

		/* Incompatible save files */
		if (tmp16u > TMD_MAX)
		{
			note(format("Too many (%u) timer entries!", tmp16u));
			return (-1);
		}

		/* Read in the timers */
		for (i = 0; i < tmp16u; i++)
		{
			rd_s16b(&(p_ptr->timed[i]));
		}
	}
	/* Hack - just wipe everything */
	else
	{
		strip_bytes(38 * 2);

		/* Hack to prevent characters imported from previous version starving to death */
		p_ptr->food = PY_FOOD_FULL - 1;

		/* Hack to prevent characters imported from previous version being exhausted */
		p_ptr->rest = PY_REST_FULL - 1;

#if 0
		rd_s16b(&p_ptr->msleep);
		rd_s16b(&p_ptr->petrify);
		rd_s16b(&p_ptr->stastis);
		rd_s16b(&p_ptr->sc);
		rd_s16b(&p_ptr->cursed);
		rd_s16b(&p_ptr->amnesia);
		rd_s16b(&p_ptr->blind);
		rd_s16b(&p_ptr->paralyzed);
		rd_s16b(&p_ptr->confused);
		rd_s16b(&p_ptr->food);
		rd_s16b(&p_ptr->rest);
		rd_s16b(&p_ptr->psleep);
		rd_s16b(&p_ptr->energy);
		rd_s16b(&p_ptr->fast);
		rd_s16b(&p_ptr->slow);
		rd_s16b(&p_ptr->afraid);
		rd_s16b(&p_ptr->cut);
		rd_s16b(&p_ptr->stun);
		rd_s16b(&p_ptr->poisoned);
		rd_s16b(&p_ptr->image);
		rd_s16b(&p_ptr->protevil);
		rd_s16b(&p_ptr->invis);
		rd_s16b(&p_ptr->hero);
		rd_s16b(&p_ptr->shero);
		rd_s16b(&p_ptr->shield);
		rd_s16b(&p_ptr->blessed);
		rd_s16b(&p_ptr->tim_invis);
		rd_s16b(&p_ptr->word_recall);
		rd_s16b(&p_ptr->see_infra);
		rd_s16b(&p_ptr->tim_infra);
		rd_s16b(&p_ptr->oppose_fire);
		rd_s16b(&p_ptr->oppose_cold);
		rd_s16b(&p_ptr->oppose_acid);
		rd_s16b(&p_ptr->oppose_elec);
		rd_s16b(&p_ptr->oppose_pois);
		rd_s16b(&p_ptr->oppose_water);
		rd_s16b(&p_ptr->oppose_lava);
		rd_s16b(&p_ptr->free_act);
#endif
	}

	/* Read some other state values */
	rd_byte(&p_ptr->charging);
	rd_byte(&p_ptr->climbing);
	rd_byte(&p_ptr->searching);

	/* Oops */
	if (!older_than(0,6,3,11))
	{
		rd_byte(&p_ptr->blocking);
		rd_byte(&p_ptr->dodging);
		rd_byte(&p_ptr->not_sneaking);
	}

	rd_byte(&p_ptr->sneaking);
	rd_byte(&p_ptr->reserves);
	rd_byte(&tmp8u);	/* oops */

	/* Read in the disease */
	rd_u32b(&p_ptr->disease);

	/* # of player turns */
	rd_s32b(&p_ptr->player_turn);

	/* # of turns spent resting */
	rd_s32b(&p_ptr->resting_turn);

	/* Get held song */
	rd_s16b(&p_ptr->held_song);

	/* Oops */
	if (!older_than(0,6,3,11))
	{
		/* Get spell trap */
		rd_s16b(&p_ptr->spell_trap);

		/* Get delayed spell */
		rd_s16b(&p_ptr->delay_spell);
	}



	/* Returning */
	rd_s16b(&p_ptr->word_return);
	rd_s16b(&p_ptr->return_y);
	rd_s16b(&p_ptr->return_x);

	/* Hack -- the two "special seeds" */
	rd_u32b(&seed_flavor);
	rd_u32b(&seed_town);

	/* Hack - for debugging dungeon generation */
	if (!older_than(0,6,3,5))
	{
		rd_u32b(&seed_dungeon);
		rd_u32b(&seed_last_dungeon);
	}
	else
	{
		seed_dungeon = 0L;
		seed_last_dungeon = 0L;
	}

	/* Special stuff */
	rd_u16b(&p_ptr->panic_save);
	rd_u16b(&p_ptr->total_winner);
	rd_u16b(&p_ptr->noscore);

	/* Read "death" */
	rd_byte(&tmp8u);
	p_ptr->is_dead = tmp8u;

	/* Read "feeling" */
	rd_byte(&tmp8u);
	feeling = tmp8u;

	/* Turn of last "feeling" */
	rd_s32b(&old_turn);

	/* Current turn */
	rd_s32b(&turn);


	/* Read the player_hp array */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > PY_MAX_LEVEL)
	{
		note(format("Too many (%u) hitpoint entries!", tmp16u));
		return (-1);
	}

	/* Read the player_hp array */
	for (i = 0; i < tmp16u; i++)
	{
		rd_s16b(&p_ptr->player_hp[i]);
	}

	/* Read study info */
	if (!older_than(0, 6, 4, 3))
	{
		rd_s16b(&p_ptr->pack_size_reduce_study);

		for (i = 0; i < p_ptr->pack_size_reduce_study; i++)
		{
			rd_s16b(&p_ptr->study_slot[i]);
		}
	}

	/* Read spell info */
	rd_u32b(&p_ptr->spell_learned1);
	rd_u32b(&p_ptr->spell_learned2);
	rd_u32b(&p_ptr->spell_learned3);
	rd_u32b(&p_ptr->spell_learned4);
	rd_u32b(&p_ptr->spell_worked1);
	rd_u32b(&p_ptr->spell_worked2);
	rd_u32b(&p_ptr->spell_worked3);
	rd_u32b(&p_ptr->spell_worked4);
	rd_u32b(&p_ptr->spell_forgotten1);
	rd_u32b(&p_ptr->spell_forgotten2);
	rd_u32b(&p_ptr->spell_forgotten3);
	rd_u32b(&p_ptr->spell_forgotten4);

	/* Read in the spells */
	for (i = 0; i < PY_MAX_SPELLS; i++)
	{
		rd_s16b(&p_ptr->spell_order[i]);
	}

	return (0);
}


/*
 * Read the random artifacts
 */
static errr rd_randarts(void)
{

#ifdef GJW_RANDART

	int i;
	u16b artifact_count;

	/* Read the number of artifacts */
	rd_u16b(&artifact_count);

	/* Incompatible save files */
	if (artifact_count > z_info->a_max)
	{
		note(format("Too many (%u) random artifacts!", artifact_count));
		return (-1);
	}

	/* Read the artifacts */
	for (i = 0; i < artifact_count; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		rd_byte(&a_ptr->tval);
		rd_byte(&a_ptr->sval);

		/* Hack for removal of lucerne hammers */
		if ((a_ptr->tval == TV_HAFTED) && (a_ptr->sval == 10))
		{
			a_ptr->tval = TV_POLEARM;
			a_ptr->sval = 9;
		}

		rd_s16b(&a_ptr->pval);

		rd_s16b(&a_ptr->to_h);
		rd_s16b(&a_ptr->to_d);
		rd_s16b(&a_ptr->to_a);
		rd_s16b(&a_ptr->ac);

		rd_byte(&a_ptr->dd);
		rd_byte(&a_ptr->ds);

		rd_s16b(&a_ptr->weight);
		rd_s32b(&a_ptr->cost);

		rd_u32b(&a_ptr->flags1);
		rd_u32b(&a_ptr->flags2);
		rd_u32b(&a_ptr->flags3);
		rd_u32b(&a_ptr->flags4);

		rd_byte(&a_ptr->level);
		rd_byte(&a_ptr->rarity);

		rd_s16b(&a_ptr->activation);

		rd_u16b(&a_ptr->time);
		rd_u16b(&a_ptr->randtime);

		rd_s32b(&a_ptr->power);
	}

	return (0);

#else /* GJW_RANDART */

	note("Random artifacts are disabled in this binary.");
	return (-1);

#endif /* GJW_RANDART */

}




/*
 * Read the player inventory
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(void)
{
	int slot = 0;

	object_type *i_ptr;
	object_type object_type_body;

	/* Read until done */
	while (1)
	{
		u16b n;

		/* Get the next item index */
		rd_u16b(&n);

		/* Nope, we reached the end */
		if (n == 0xFFFF) break;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Read the item */
		if (rd_item(i_ptr))
		{
			note("Error reading item");
			return (-1);
		}

		/* Hack -- verify item */
		if (!i_ptr->k_idx) return (-1);

		/* Verify slot */
		if (n >= INVEN_TOTAL) return (-1);

		/* Wield equipment */
		if (n >= INVEN_WIELD)
		{
			/* Copy object */
			object_copy(&inventory[n], i_ptr);

			/* Add the weight */
			p_ptr->total_weight += (i_ptr->number * i_ptr->weight);

			/* One more item */
			if (!IS_QUIVER_SLOT(n)) p_ptr->equip_cnt++;
		}

		/* Warning -- backpack is full */
		else if (p_ptr->inven_cnt == INVEN_PACK)
		{
			/* Oops */
			note("Too many items in the inventory!");

			/* Fail */
			return (-1);
		}

		/* Carry inventory */
		else
		{
			/* Get a slot */
			n = slot++;

			/* Copy object */
			object_copy(&inventory[n], i_ptr);

			/* Hack -- clear the marked flag */
			inventory[n].ident &= ~(IDENT_MARKED);

			/* Hack -- in inventory, player must be aware it exists */
			if (!k_info[i_ptr->k_idx].flavor) k_info[i_ptr->k_idx].aware |= (AWARE_EXISTS);

			/* Hack -- count bags */
			if (i_ptr->tval == TV_BAG) p_ptr->pack_size_reduce_bags++;

			/* Add the weight */
			p_ptr->total_weight += (i_ptr->number * i_ptr->weight);

			/* One more item */
			p_ptr->inven_cnt++;
		}
	}

	/* Update "p_ptr->pack_size_reduce" */
	find_quiver_size();

	/* Success */
	return (0);
}



/*
 * Read the saved messages
 */
static void rd_messages(void)
{
	int i;
	char buf[128];
	u16b tmp16u;

	s16b num;

	/* Total */
	rd_s16b(&num);

	/* Read the messages */
	for (i = 0; i < num; i++)
	{
		/* Read the message */
		rd_string(buf, 128);

		rd_u16b(&tmp16u);

		/* Save the message */
		message_add(buf, tmp16u);
	}
}

/*
 * Read saved help tip file names
 */
static void rd_tip_files(void)
{
	int i;
	char buf[128];

	/* Total */
	rd_s16b(&tips_end);

	/* Read the help tip file names */
	for (i = 0; i < tips_end; i++)
	{
		/* Read the file name */
		rd_string(buf, 128);

		/* Add the tip, using quarks */
		tips[i] = quark_add(buf);
	}

	/* Read the position of the first not shown tip */
	rd_s16b(&tips_start);
}


/*
 * Hack -- never redraw gained attributes
 */
bool redraw_dummy(int y, int x)
{
	(void)y;
	(void)x;
	return FALSE;
}



/*
 * Read the dungeon
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 *
 * Note that the size of the dungeon is now hard-coded to
 * DUNGEON_HGT by DUNGEON_WID, and any dungeon with another
 * size will be silently discarded by this routine.
 *
 * Note that dungeon objects, including objects held by monsters, are
 * placed directly into the dungeon, using "object_copy()", which will
 * copy "iy", "ix", and "held_m_idx", leaving "next_o_idx" blank for
 * objects held by monsters, since it is not saved in the savefile.
 *
 * After loading the monsters, the objects being held by monsters are
 * linked directly into those monsters.
 *
 * Important note: There are several CAVE_ flags that cannot be determined
 * until the objects and monsters have been loaded. In particular,
 * CAVE_HALO depends on objects on the ground, and CAVE_TLIT depends on
 * the presence of glowing monsters.
 */
static errr rd_dungeon(void)
{
	int i, y, x;
	int by, bx;

	s16b town;
	s16b dungeon;
	s16b depth;
	s16b py, px;
	s16b ymax, xmax;

	byte count;
	byte tmp8u;
	u16b tmp16u;

	u16b limit;

	/*** Basic info ***/

	/* Header info */
	rd_s16b(&depth);
	rd_s16b(&dungeon);
	rd_s16b(&py);
	rd_s16b(&px);
	rd_s16b(&ymax);
	rd_s16b(&xmax);
	rd_s16b(&town);

	rd_u32b(&level_flag);

	/* Ignore illegal dungeons */
	if (town>=z_info->t_max)
	{
		note(format("Ignoring illegal dungeon (%d)", dungeon));
		return (0);
	}

	/* Ignore illegal dungeons */
	if ((ymax != DUNGEON_HGT) || (xmax != DUNGEON_WID))
	{
		/* XXX XXX XXX */
		note(format("Ignoring illegal dungeon size (%d,%d).", ymax, xmax));
		return (0);
	}

	/* Ignore illegal dungeons */
	if ((px < 0) || (px >= DUNGEON_WID) ||
	    (py < 0) || (py >= DUNGEON_HGT))
	{
		note(format("Ignoring illegal player location (%d,%d).", py, px));
		return (1);
	}


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Extract "info" */
			cave_info[y][x] = tmp8u;

			/* Advance/Wrap */
			if (++x >= DUNGEON_WID)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= DUNGEON_HGT) break;
			}
		}
	}

	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Extract "info" */
			play_info[y][x] = tmp8u;

			/* Advance/Wrap */
			if (++x >= DUNGEON_WID)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= DUNGEON_HGT) break;
			}
		}
	}

	/* Hack -- not fully dynamic */
	dyna_full = FALSE;

	/* No dynamic grids */
	dyna_n = 0;

	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
		/* Grab RLE info */
		rd_byte(&count);

		rd_u16b(&tmp16u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Save the feat */
			cave_feat[y][x] = tmp16u;

			/* Check for los flag set*/
			if (f_info[cave_feat[y][x]].flags1 & (FF1_LOS))
			{
				cave_info[y][x] &= ~(CAVE_XLOS);
			}

			/* Handle wall grids */
			else
			{
				cave_info[y][x] |= (CAVE_XLOS);
			}

			/* Check for los flag set*/
			if (f_info[cave_feat[y][x]].flags1 & (FF1_PROJECT))
			{
				cave_info[y][x] &= ~(CAVE_XLOF);
			}

			/* Handle wall grids */
			else
			{
				cave_info[y][x] |= (CAVE_XLOF);
			}

			/* Check for climbable terrain */
			if (f_info[cave_feat[y][x]].flags2 & (FF2_CAN_CLIMB))
			{
				gain_attribute(y, x, 1, 0, apply_climb, redraw_dummy);
			}

			/* Handle dynamic grids */
			if (f_info[cave_feat[y][x]].flags3 & (FF3_DYNAMIC_MASK))
			{
				if (dyna_n < (DYNA_MAX-1))
				{
					dyna_g[dyna_n++] = GRID(y,x);
				}
				else
				{
					dyna_full = TRUE;
					dyna_cent_y = 255;
					dyna_cent_x = 255;
				}
			}

			/* Advance/Wrap */
			if (++x >= DUNGEON_WID)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= DUNGEON_HGT) break;
			}
		}
	}

	/*** Player ***/

	/* Fix depth */
	if (depth < min_depth(dungeon))
	{
		note(format("Fixing too small dungeon depth (%d)", depth));
		depth = min_depth(dungeon);
	}
	else if (depth > max_depth(dungeon))
	{
		note(format("Fixing too big dungeon depth (%d)", depth));
		depth = max_depth(dungeon);
	}

	/* Load depth */
	p_ptr->depth = depth;
	p_ptr->dungeon = dungeon;
	p_ptr->town = town;

	/* Fix level flags */
	if (!level_flag) init_level_flags();

	/* Apply daylight */
	if ((level_flag & (LF1_SURFACE)) != 0) town_illuminate((level_flag & (LF1_DAYLIGHT)) != 0);

	/* Fix dungeon light */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			if (f_info[cave_feat[y][x]].flags2 & (FF2_GLOW))
			{
				gain_attribute(y, x, 2, CAVE_XLOS, apply_halo, redraw_dummy);
			}
		}
	}

	/* Place player in dungeon */
	if (!player_place(py, px, FALSE))
	{
		note(format("Cannot place player (%d,%d)!", py, px));
		return (-1);
	}


	/*** Room indexes ***/
	for (bx = 0; bx < MAX_ROOMS_ROW; bx++)
	{
		for (by = 0; by < MAX_ROOMS_COL; by++)
		{
			rd_byte(&dun_room[bx][by]);
		}
	}

	/*** Room descriptions ***/
	for (i = 1; i < DUN_ROOMS; i++)
	{
		int j;

		rd_s16b(&room_info[i].type);
		rd_s16b(&room_info[i].vault);
		rd_u32b(&room_info[i].flags);
		rd_s16b(&room_info[i].deepest_race);
		rd_u32b(&room_info[i].ecology);

		for (j = 0; j < ROOM_DESC_SECTIONS; j++)
		{
			rd_s16b(&room_info[i].section[j]);

			if (room_info[i].section[j] == -1) break;
		}

		/* Discard room sections for any old version */
		/* This is because the sections are so sensitive to re-ordering */
		if (older_than(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA)) room_info[i].section[0] = -1;
	}

	/*** Objects ***/

	/* Read the item count */
	rd_u16b(&limit);

	/* Verify maximum */
	if (limit > z_info->o_max)
	{
		note(format("Too many (%d) object entries!", limit));
		return (-1);
	}

	/* Read the dungeon items */
	for (i = 1; i < limit; i++)
	{
		object_type *i_ptr;
		object_type object_type_body;

		s16b o_idx;
		object_type *o_ptr;


		/* Get the object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Read the item */
		if (rd_item(i_ptr))
		{
			note("Error reading item");
			return (-1);
		}

		/* Make an object */
		o_idx = o_pop();

		/* Paranoia */
		if (o_idx != i)
		{
			note(format("Cannot place object %d!", i));
			return (-1);
		}

		/* Get the object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, i_ptr);

		/* Dungeon floor */
		if (!i_ptr->held_m_idx)
		{
			int x = i_ptr->ix;
			int y = i_ptr->iy;

			/* ToDo: Verify coordinates */

			/* Link the object to the pile */
			o_ptr->next_o_idx = cave_o_idx[y][x];

			/* Link the floor to the object */
			cave_o_idx[y][x] = o_idx;

			/* Check if object lights the dungeon */
			if (check_object_lite(i_ptr))
			{
				gain_attribute(y, x, 2, CAVE_XLOS, apply_halo, redraw_halo_gain);
			}
		}
	}


	/*** Regions ***/
	if (!older_than(0, 6, 3, 3))
	{
		/* Read the region piece count */
		rd_u16b(&limit);

		/* Verify maximum */
		if (limit > z_info->region_max)
		{
			note(format("Too many (%d) region entries!", limit));
			return (-1);
		}

		/* Read the regions */
		for (i = 1; i < limit; i++)
		{
			region_type *i_ptr;
			region_type region_type_body;

			s16b region;
			region_type *rp_ptr;


			/* Get the object */
			i_ptr = &region_type_body;

			/* Wipe the object */
			region_wipe(i_ptr);

			/* Read the item */
			rd_region(i_ptr);

			/* Make an object */
			region = region_pop();

			/* Paranoia */
			if (region != i)
			{
				note(format("Cannot place region %d!", i));
				return (-1);
			}

			/* Get the object */
			rp_ptr = &region_list[region];

			/* Structure Copy */
			region_copy(rp_ptr, i_ptr);
		}

		/* Read the region piece count */
		rd_u16b(&limit);

		/* Verify maximum */
		if (limit > z_info->region_piece_max)
		{
			note(format("Too many (%d) region pieces!", limit));
			return (-1);
		}

		/* Read the region_pieces */
		for (i = 1; i < limit; i++)
		{
			region_piece_type *i_ptr;
			region_piece_type region_piece_type_body;
			region_type *r_ptr;

			s16b region_piece;
			region_piece_type *rp_ptr;

			int x;
			int y;

			/* Get the object */
			i_ptr = &region_piece_type_body;

			/* Wipe the object */
			region_piece_wipe(i_ptr);

			/* Read the item */
			rd_region_piece(i_ptr);

			/* Make an object */
			region_piece = region_piece_pop();

			/* Paranoia */
			if (region_piece != i)
			{
				note(format("Cannot place region piece %d!", i));
				return (-1);
			}

			/* Get the object */
			rp_ptr = &region_piece_list[region_piece];

			/* Structure Copy */
			region_piece_copy(rp_ptr, i_ptr);

			/* Get the region */
			r_ptr = &region_list[rp_ptr->region];

			/* Paranoia */
			if (!r_ptr->type)
			{
				note(format("Inserting piece into expired region %d!", rp_ptr->region));
				return (-1);
			}

			/* Insert into the region */
			rp_ptr->next_in_region = r_ptr->first_piece;
			r_ptr->first_piece = region_piece;

			/* ToDo: Verify coordinates */
			y = i_ptr->y;
			x = i_ptr->x;

			/* Link the object to the pile */
			rp_ptr->next_in_grid = cave_region_piece[y][x];

			/* Link the floor to the object */
			cave_region_piece[y][x] = region_piece;
		}
	}


	/*** Monsters ***/

	/* Read the monster count */
	rd_u16b(&limit);

	/* Hack -- verify */
	if (limit > z_info->m_max)
	{
		note(format("Too many (%d) monster entries!", limit));
		return (-1);
	}

	/* Use the simple RNG to preserve seed from before save */
	/* TODO: Perhaps eliminate the RNG roll altogether in monster2.c */
	Rand_quick = TRUE;

	/* Read the monsters */
	for (i = 1; i < limit; i++)
	{
		monster_type *n_ptr;
		monster_type monster_type_body;


		/* Get local monster */
		n_ptr = &monster_type_body;

		/* Clear the monster */
		(void)WIPE(n_ptr, monster_type);

		/* Read the monster */
		rd_monster(n_ptr);

		/* Place monster in dungeon */
		if (monster_place(n_ptr->fy, n_ptr->fx, n_ptr) != i)
		{
			note(format("Cannot place monster %d", i));
			/* Use the complex RNG again */
			Rand_quick = FALSE;
			return (-1);
		}
	}

	/* Use the complex RNG again */
	Rand_quick = FALSE;

	/*** Holding ***/

	/* Reacquire objects */
	for (i = 1; i < o_max; ++i)
	{
		object_type *o_ptr;

		monster_type *m_ptr;

		/* Get the object */
		o_ptr = &o_list[i];

		/* Ignore dungeon objects */
		if (!o_ptr->held_m_idx) continue;

		/* Verify monster index */
		if (o_ptr->held_m_idx >= z_info->m_max)
		{
			note("Invalid monster index");
			return (-1);
		}

		/* Get the monster */
		m_ptr = &m_list[o_ptr->held_m_idx];

		/* Link the object to the pile */
		o_ptr->next_o_idx = m_ptr->hold_o_idx;

		/* Link the monster to the object */
		m_ptr->hold_o_idx = i;
	}

	/* Read the ecology count */
	rd_u16b(&limit);

	rd_byte(&cave_ecology.num_ecologies);

	/* Hack -- verify */
	if (limit > MAX_ECOLOGY_RACES)
	{
		note(format("Too many (%d) ecology entries!", limit));
		return (-1);
	}

	/* Read the ecology */
	for (i = 0; i < limit; i++)
	{
		rd_s16b(&(cave_ecology.race[i]));
		rd_u32b(&(cave_ecology.race_ecologies[i]));
	}

	/* Ecology ready? */
	cave_ecology.ready = (limit > 0);
	cave_ecology.num_races = (byte)limit;

	/*** Success ***/

	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Success */
	return (0);
}


#define OLD_SV_BAG_MAX_BAGS		24


const s16b old_bag_holds[OLD_SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL][2] =
{
	/* Bag of Poisons - holds dangerous potions */
	{ {75, 4}, {75, 5}, {75, 7}, {75, 9},
	  {75, 11}, {75, 13}, {75, 15}, {75, 16},
	  {75, 17}, {75, 18}, {75, 19}, {75, 20},
	  {75, 21}, {75, 22}, {75, 73},	{75, 74},
	  {75, 76}, {75, 77}, {75, 78}, {75, 79},
	  {75, 80}, {75, 81}, {0, 0} },
	/* Bag of Harmful Mushrooms - holds harmful mushrooms */
	{ {79, 0}, {79, 1}, {79, 2}, {79, 3},
	  {79, 4}, {79, 5}, {79, 6}, {79, 7},
	  {79, 8}, {79, 9}, {79, 10}, {79, 11},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Black Magics -- holds cursed scrolls */
	{ {70, 0}, {70, 1}, {70, 2}, {70, 3},
	  {70, 4}, {70, 7}, {70, 5}, {70, 69},
	  {70, 74}, {70, 75}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of supplies -- torches, oil, spikes etc. Torches are handed specially. */
	{ {77, 0}, {2, 2}, {77, 2}, {39, 0},
	  {5, 0}, {2, 1}, {2, 3}, {2, 4},
	  {14, 1}, {14, 2}, {14, 4}, {77, 7},
	  {77, 8}, {77, 9}, {77, 10}, {77, 11},
	  {77, 12}, {77, 13}, {77, 14}, {77, 15},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of provisions -- foods of various types */
	{ {80, 30}, {80, 32}, {80, 33}, {80, 35}, {80, 36},
	  {80, 37}, {80, 38}, {80, 39}, {80, 40},
	  {80, 41}, {80, 42}, {75, 1}, {75, 2},
	  {75, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}},
	/* Bag of Antidotes - holds low powered curing potions */
	{ {75, 26}, {75, 27}, {75, 31}, {75, 30},
	  {75, 34}, {75, 35}, {75, 41}, {75, 47},
	  {75, 60}, {75, 61}, {75, 62}, {75, 64},
	  {75, 68}, {75, 71}, {75, 83}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Philtres - holds medium power potions */
	{ {75, 23}, {75, 24}, {75, 25}, {75, 32},
	  {75, 33}, {75, 35}, {75, 36}, {75, 48},
	  {75, 49}, {75, 51}, {75, 52}, {75, 55},
	  {75, 68}, {75, 69}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}  },
	/* Bag of Elixirs - holds high power potions */
	{ {75, 29}, {75, 37}, {75, 38}, {75, 39},
	  {75, 40}, {75, 54}, {75, 56}, {75, 57},
	  {75, 58}, {75, 59}, {75, 63}, {75, 65},
	  {75, 66}, {75, 67}, {75, 72}, {75, 75},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Helpful Mushrooms - holds helpful mushrooms */
	{ {79, 12}, {79, 13}, {79, 14}, {79, 15},
	  {79, 16}, {79, 17}, {79, 18}, {79, 19},
	  {79, 20}, {79, 21}, {79, 22}, {79, 23},
	  {79, 24}, {79, 25}, {79, 26}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Scrying - holds identify / detect scrolls */
	{ {70, 12}, {70, 19}, {70, 23}, {70, 25},
	  {70, 26}, {70, 27}, {70, 28}, {70, 29},
	  {70, 30}, {70, 31}, {70, 40}, {70, 43},
	  {70, 48}, {70, 61}, {70, 65}, {70, 66},
	  {70, 73}, {70, 77}, {70, 78}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Hedge Magics - holds less useful scrolls */
	{ {70, 8}, {70, 9}, {70, 10}, {70, 11},
	  {70, 22}, {70, 24}, {70, 32}, {70, 33},
	  {70, 34}, {70, 35}, {70, 37}, {70, 39},
	  {70, 59}, {70, 60}, {70, 62}, {70, 64},
	  {70, 76}, {70, 79}, {70, 80}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Enchantments - holds enchantment scrolls */
	{ {70, 14}, {70, 16}, {70, 17}, {70, 18},
	  {70, 20}, {70, 21}, {70, 49}, {70, 50},
	  {70, 51}, {70, 53}, {70, 54}, {70, 55},
	  {70, 56}, {70, 57}, {70, 58}, {70, 67},
	  {70, 68}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Eldritch Magics - holds powerful scrolls */
	{ {70, 13}, {70, 15}, {70, 38}, {70, 41},
	  {70, 42}, {70, 44}, {70, 45}, {70, 46},
	  {70, 47}, {70, 63}, {70, 70}, {70, 71},
	  {70, 72}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Lesser Runes - holds weaker runes */
	{ {93, 1}, {93, 3}, {93, 5}, {93, 7},
	  {93, 11}, {93, 12}, {93, 13}, {93, 14},
	  {93, 15}, {93, 16}, {93, 19}, {93, 21},
	  {93, 22}, {93, 25}, {93, 26}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Greater Runes - holds more powerful runes */
	{ {93, 2}, {93, 4}, {93, 6}, {93, 8},
	  {93, 9}, {93, 10}, {93, 17}, {93, 18},
	  {93, 20}, {93, 23}, {93, 24}, {93, 27},
	  {93, 28}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of Maps - holds maps */
	{ {72, 25}, {72, 31}, {72, 41}, {72, 47},
	  {72, 50}, {72, 56},  {72, 62}, {72, 64},
	  {72, 65}, {72, 66}, {72, 68}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of bewitchments -- low power non-elemental wands */
	{ {65, 0}, {65, 1}, {65, 2}, {65, 4},
	  {65, 7}, {65, 8}, {65, 9}, {65, 10},
	  {65, 11}, {65, 12}, {65, 13}, {65, 24},
	  {65, 32}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* XXX XXX Hack -- Wand bags need two slots */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of elements -- low power elemental wands */
	{ {65, 5}, {65, 6}, {65, 14}, {65, 15},
	  {65, 16}, {65, 17}, {65, 18}, {65, 19},
	  {65, 20}, {65, 21}, {65, 22}, {65, 23},
	  {65, 33}, {65, 34}, {65, 36}, {65, 38},
	  {65, 39}, {65, 40}, {65, 42}, {65, 43},
	  {65, 67}, {0, 0}, {0, 0} },
	/* XXX XXX Hack -- Wand bags need two slots */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of four winds -- medium power wands */
	{ {65, 3}, {65, 26}, {65, 27}, {65, 28},
	  {65, 29}, {65, 30}, {65, 34}, {65, 35},
	  {65, 37}, {65, 41}, {65, 45}, {65, 46},
	  {65, 47}, {65, 48}, {65, 49}, {65, 50},
	  {65, 53}, {65, 54}, {65, 55}, {65, 69},
	  {65, 70}, {65, 71}, {0, 0} },
	/* XXX XXX Hack -- Wand bags need two slots */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} },
	/* Bag of six demons -- high power wands */
	{ {65, 25}, {65, 27}, {65, 28}, {65, 31},
	  {65, 44}, {65, 51}, {65, 52}, {65, 56},
	  {65, 57}, {65, 58}, {65, 59}, {65, 60},
	  {65, 61}, {65, 62}, {65, 63}, {65, 64},
	  {65, 65}, {65, 66}, {65, 67}, {65, 68},
	  {0, 0}, {0, 0}, {0, 0} },
	/* XXX XXX Hack -- Wand bags need two slots */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0}, {0, 0},
	  {0, 0}, {0, 0}, {0, 0} }
};

/*
 * Fix up the bags from old versions.
 *
 * We attempt to maintain save file compatibility with older
 * versions of bags. This is an ugly hack on top of the general
 * bag hack.
 */
static void fix_bags(bool drop)
{
	int i, j, k, l;

	int old_bag_contents[OLD_SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL];

	/* If we ever stop storing an item type in a bag... */
	(void)drop;

	for (i = 0; i < OLD_SV_BAG_MAX_BAGS; i++)
	{
		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			old_bag_contents[i][j] = bag_contents[i][j];
			bag_contents[i][j] = 0;
		}
	}

#if 0
	/* Discard 'really' old bag layout */
	if (older_than(0, 6, 2, 0)) return;
#endif

	/* For each old bag slot, find the new location and populate it */
	for (i = 0; i < OLD_SV_BAG_MAX_BAGS; i++)
	{
		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			/* Efficiency */
			if ((old_bag_holds[i][j][0] == 0) &&
					(old_bag_holds[i][j][1] == 0)) continue;

			/* More efficiency */
			if ((old_bag_holds[i][j][0] == bag_holds[i][j][0]) &&
				(old_bag_holds[i][j][1] == bag_holds[i][j][1]))
			{
				bag_contents[i][j] += old_bag_contents[i][j];
				continue;
			}

			/* Find the new slot */
			for (k = 0; k < SV_BAG_MAX_BAGS; l++)
			{
				for (l = 0; l < INVEN_BAG_TOTAL; l++)
				{
					if ((old_bag_holds[i][j][0] == bag_holds[k][l][0]) &&
						(old_bag_holds[i][j][1] == bag_holds[k][l][1]))
					{
						bag_contents[k][l] += old_bag_contents[i][j];

						/* Hack - Break out of both loops */
						k = SV_BAG_MAX_BAGS;
						break;
					}
				}
			}

			/* Oops - haven't found a bag */
			if (l == INVEN_BAG_TOTAL)
			{
				/* TODO: Create item and drop it under the player */
			}
		}
	}
}


/*
 * Actually read the savefile
 */
static errr rd_savefile_new_aux(void)
{
	int i, j;

	byte tmp8u;
	u16b tmp16u;
	u32b tmp32u;

#ifdef VERIFY_CHECKSUMS
	u32b n_x_check, n_v_check;
	u32b o_x_check, o_v_check;
#endif


	/* Mention the savefile version */
	note(format("Loading a %d.%d.%d.%d savefile...",
	    sf_major, sf_minor, sf_patch, sf_extra));

	/* Strip the version bytes */
	strip_bytes(4);

	/* Hack -- decrypt */
	xor_byte = sf_extra;


	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;


	/* Operating system info */
	rd_u32b(&sf_xtra);

	/* Time of savefile creation */
	rd_u32b(&sf_when);

	/* Number of resurrections */
	rd_u16b(&sf_lives);

	/* Number of times played */
	rd_u16b(&sf_saves);


	/* Later use (always zero) */
	rd_u32b(&tmp32u);

	/* Later use (always zero) */
	rd_u32b(&tmp32u);


	/* Read RNG state */
	rd_randomizer();
	if (arg_fiddle) note("Loaded Randomizer Info");


	/* Then the options */
	rd_options();
	if (arg_fiddle) note("Loaded Option Flags");


	/* Then the "messages" */
	rd_messages();
	if (arg_fiddle) note("Loaded Messages");

	/* Then the help tips */
	rd_tip_files();
	if (arg_fiddle) note("Loaded Help Tips");

	/* Monster Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->r_max)
	{
		note(format("Too many (%u) monster races!", tmp16u));
		return (-1);
	}

	/* Read the available records */
	for (i = 0; i < tmp16u; i++)
	{
		/* Read the lore */
		rd_lore(i);
	}
	if (arg_fiddle) note("Loaded Monster Memory");


	/* Object Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->k_max)
	{
		note(format("Too many (%u) object kinds!", tmp16u));
		return (-1);
	}

	/* Read the object memory */
	for (i = 0; i < tmp16u; i++)
	{
		object_kind *k_ptr = &k_info[i];

		byte tmp8u;

		if (older_than(0,6,3,6))
		{
			rd_byte(&tmp8u);
			k_ptr->aware = tmp8u;
		}
		else
		{
			rd_u16b(&k_ptr->aware);
		}
		rd_byte(&k_ptr->guess);

		/* Activations */
		rd_s16b(&k_ptr->ever_used);

		/* Hack - reset activations this lifetime */
		if (!older_than(0,6,3,6))
		{
			/* Activations ever */
			rd_s16b(&k_ptr->used);
		}
	}
	if (arg_fiddle) note("Loaded Object Memory");


	/* Load the Quests */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_Q_IDX)
	{
		note(format("Too many (%u) quests!", tmp16u));
		return (-1);
	}

	/* Load the Quests */
	for (i = 0; i < tmp16u; i++)
	{
		int j;

		rd_byte(&tmp8u);

		if (tmp8u > MAX_QUEST_EVENTS)
		{
			note(format("Too many (%u) quest stages!", tmp16u));
			return (-1);
		}

		for (j = 0; j < tmp8u; j++)
		{

			rd_event(i, j);
		}

		rd_byte(&tmp8u);

		q_list[i].stage = tmp8u;
	}

	if (arg_fiddle) note("Loaded Quests");

	/* Read the randart seed */
	rd_u32b(&seed_randart);

	/* Load the Artifact lore */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->a_max)
	{
		note(format("Too many (%u) artifacts!", tmp16u));
		return (-1);
	}

	z_info->a_max = tmp16u;

	/* Read the artifact flags */
	for (i = 0; i < tmp16u; i++)
	{
		object_info *n_ptr = &a_list[i];

		rd_byte(&tmp8u);
		a_info[i].cur_num = (tmp8u != 0);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);

		/* Knowledge */
		rd_u32b(&n_ptr->can_flags1);
		rd_u32b(&n_ptr->can_flags2);
		rd_u32b(&n_ptr->can_flags3);
		rd_u32b(&n_ptr->can_flags4);

		rd_u32b(&n_ptr->not_flags1);
		rd_u32b(&n_ptr->not_flags2);
		rd_u32b(&n_ptr->not_flags3);
		rd_u32b(&n_ptr->not_flags4);

		/* Activations */
		rd_s16b(&a_info[i].activated);

		/* Oops */
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}

	if (arg_fiddle) note("Loaded Artifact Lore");

	/* Load the Ego items */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->e_max)
	{
		note(format("Too many (%u) ego items!", tmp16u));
		return (-1);
	}

	/* Read the ego item flags */
	for (i = 0; i < tmp16u; i++)
	{
		object_lore *n_ptr = &e_list[i];

		rd_u32b(&n_ptr->can_flags1);
		rd_u32b(&n_ptr->can_flags2);
		rd_u32b(&n_ptr->can_flags3);
		rd_u32b(&n_ptr->can_flags4);

		rd_u32b(&n_ptr->may_flags1);
		rd_u32b(&n_ptr->may_flags2);
		rd_u32b(&n_ptr->may_flags3);
		rd_u32b(&n_ptr->may_flags4);

		rd_u32b(&n_ptr->not_flags1);
		rd_u32b(&n_ptr->not_flags2);
		rd_u32b(&n_ptr->not_flags3);
		rd_u32b(&n_ptr->not_flags4);

		rd_u16b(&(e_info[i].aware));

		/* Oops */
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}

	if (arg_fiddle) note("Loaded Ego Item Lore");

	/* Read the extra stuff */
	if (rd_extra()) return (-1);
	if (arg_fiddle) note("Loaded extra information");

	/* Don't bother saving flavor flags if dead */
	if (!p_ptr->is_dead)
	{
		/* Load the Flavors */
		rd_u16b(&tmp16u);

		/* Incompatible save files */
		if (tmp16u > z_info->x_max)
		{
			note(format("Too many (%u) flavors!", tmp16u));
			return (-1);
		}

		/* Read the flavor flags */
		for (i = 0; i < tmp16u; i++)
		{
			object_info *n_ptr = &x_list[i];

			rd_u32b(&n_ptr->can_flags1);
			rd_u32b(&n_ptr->can_flags2);
			rd_u32b(&n_ptr->can_flags3);
			rd_u32b(&n_ptr->can_flags4);

			rd_u32b(&n_ptr->not_flags1);
			rd_u32b(&n_ptr->not_flags2);
			rd_u32b(&n_ptr->not_flags3);
			rd_u32b(&n_ptr->not_flags4);
		}

		if (arg_fiddle) note("Loaded Flavors");
	}

	/* Read random artifacts */
	if (rd_randarts()) return (-1);
	if (arg_fiddle) note("Loaded Random Artifacts");

	/* Generate artifact names (only, hence FALSE) according to the seed.
	   They are not stored in savefile but regenerated every time.
	   The rest of artifact info (except field 'text', which is empty)
	   is read from savefile later. */
	do_randart(seed_randart, FALSE);

	/* Important -- Initialize the sex */
	sp_ptr = &sex_info[p_ptr->psex];

	/* Important -- Initialize the race/class */
	rp_ptr = &p_info[p_ptr->prace];
	cp_ptr = &c_info[p_ptr->pclass];

	/* Read the inventory */
	if (rd_inventory())
	{
		note("Unable to read inventory");
		return (-1);
	}

	/* Read the bags */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > SV_BAG_MAX_BAGS)
	{
		note(format("Too many (%u) bag types!", tmp16u));
		return (-1);
	}

	/* Load the bag contents */
	for (i = 0; i < tmp16u; i++)
	{
		/* Load the Bags */
		rd_byte(&tmp8u);

		/* Incompatible save files */
		if (tmp8u > INVEN_BAG_TOTAL)
		{
			note(format("Too many (%u) bag slots!", tmp8u));
			return (-1);
		}

		for (j = 0; j < tmp8u; j++)
		{
			rd_s16b(&bag_contents[i][j]);
		}
	}

	/* Read the dungeons */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->t_max)
	{
		note(format("Too many (%u) dungeon types!", tmp16u));
		return (-1);
	}

	for (i = 0; i < tmp16u; i++)
	{
		/* Load the maximum depth */
		rd_byte(&tmp8u);

		/* Silently fix depth */
		if (tmp8u > max_depth(i))
			tmp8u = max_depth(i);
		else if (tmp8u < min_depth(i))
			tmp8u = min_depth(i);

		/* Set the maximal attained depth */
		t_info[i].attained_depth = tmp8u;

		rd_byte(&t_info[i].visited);

		if (!p_ptr->is_dead)
		{
			rd_u16b(&t_info[i].guardian_ifvisited);
			rd_u16b(&t_info[i].replace_guardian);
		}
		else
			strip_bytes(4);

		/* Read the store indexes */
		if (!p_ptr->is_dead)
		{
			/* Load the number of stores */
			rd_byte(&tmp8u);

			for (j = 0; j < tmp8u; j++)
			{
				/* Load the store index */
				rd_u16b(&t_info[i].store_index[j]);
			}
		}
	}

	/* Don't read stores anymore if dead */
	if (!p_ptr->is_dead)
	{
		/* Read the stores */
		rd_u16b(&tmp16u);

		for (i = 0; i < tmp16u; i++)
		{
			if (rd_store()) return (-1);
		}
	}

	/* I'm not dead yet... */
	if (!p_ptr->is_dead)
	{
		/* Dead players have no dungeon */
		note("Restoring Dungeon...");
		if (rd_dungeon())
		{
			note("Error reading dungeon data");
			note("--------");
			note("It looks like there's a problem that I can attempt to recover from in your save file.");
			note("However, I'd recommend that you make a backup of the save file first, in the unlikely");
			note("event that a root cause of the problem is found and a fix implemented in a later");
			note("version of Unangband.");
			note("Please note that recovery will involve generating a new dungeon level.");
			if (older_than(0, 6, 3, 3)) note("You will also lose some contents of your magic bags.");
			note("--------");
			if(!get_check("Attempt a recovery? ")) return(-1);
			if(!get_check("Have you made a backup of this save file? ")) return(-1);
			if(!get_check("Are you sure you wish to continue (All dungeon data will be lost)? ")) return(-1);

			p_ptr->leaving = TRUE;

			/* Fix bags */
			if (older_than(0, 6, 3, 3)) fix_bags(FALSE);

			/* Don't bother reading the rest of the file */
			return (0);
		}

		/* Read the ghost info */
		rd_ghost();
	}

	/* Fix bags */
	if (older_than(0, 6, 3, 3)) fix_bags(FALSE);

#ifdef VERIFY_CHECKSUMS

	/* Save the checksum */
	n_v_check = v_check;

	/* Read the old checksum */
	rd_u32b(&o_v_check);

	/* Verify */
	if (o_v_check != n_v_check)
	{
		note("Invalid checksum");
		return (-1);
	}

	/* Save the encoded checksum */
	n_x_check = x_check;

	/* Read the checksum */
	rd_u32b(&o_x_check);

	/* Verify */
	if (o_x_check != n_x_check)
	{
		note("Invalid encoded checksum");
		return (-1);
	}

#endif


	/* Hack -- no ghosts */
	r_info[z_info->r_max-1].max_num = 0;

	/* Success */
	return (0);
}


/*
 * Actually read the savefile
 */
errr rd_savefile(void)
{
	errr err;

	/* Grab permissions */
	safe_setuid_grab();

	/* The savefile is a binary file */
	fff = my_fopen(savefile, "rb");

	/* Drop permissions */
	safe_setuid_drop();

	/* Paranoia */
	if (!fff) return (-1);

	/* Call the sub-function */
	err = rd_savefile_new_aux();

	/* Check for errors */
	if (ferror(fff)) err = -1;

	/* Close the file */
	my_fclose(fff);

	/* Result */
	return (err);
}


/*
 * Attempt to Load a "savefile"
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(void)
{
	int fd = -1;

	errr err = 0;

	byte vvv[4];

#ifdef VERIFY_TIMESTAMP
	struct stat	statbuf;
#endif /* VERIFY_TIMESTAMP */

	cptr what = "generic";


	/* Paranoia */
	turn = 0;
	p_ptr->player_turn  = 0;
	p_ptr->resting_turn = 0;

	/* Paranoia */
	p_ptr->is_dead = FALSE;


	/* Allow empty savefile name */
	if (!savefile[0]) return (TRUE);

	/* Grab permissions */
	safe_setuid_grab();

	/* Open the savefile */
	fd = fd_open(savefile, O_RDONLY);

	/* Drop permissions */
	safe_setuid_drop();

	/* No file */
	if (fd < 0)
	{
		/* Give a message */
		msg_print("Savefile does not exist.");
		message_flush();

		/* Allow this */
		return (TRUE);
	}

	/* Close the file */
	fd_close(fd);


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (!err)
	{
		FILE *fkk;

		char temp[1024];

		/* Extract name of lock file */
		my_strcpy(temp, savefile);
		my_strcat(temp, ".lok");

		/* Grab permissions */
		safe_setuid_grab();

		/* Check for lock */
		fkk = my_fopen(temp, "r");

		/* Drop permissions */
		safe_setuid_drop();

		/* Oops, lock exists */
		if (fkk)
		{
			/* Close the file */
			my_fclose(fkk);

			/* Message */
			msg_print("Savefile is currently in use.");
			message_flush();

			/* Oops */
			return (FALSE);
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Create a lock file */
		fkk = my_fopen(temp, "w");

		/* Drop permissions */
		safe_setuid_drop();

		/* Dump a line of info */
		fprintf(fkk, "Lock file for savefile '%s'\n", savefile);

		/* Close the lock file */
		my_fclose(fkk);
	}

#endif /* VERIFY_SAVEFILE */


	/* Okay */
	if (!err)
	{
		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fd = fd_open(savefile, O_RDONLY);

		/* Drop permissions */
		safe_setuid_drop();

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open savefile";
	}

	/* Process file */
	if (!err)
	{

#ifdef VERIFY_TIMESTAMP

		/* Grab permissions */
		safe_setuid_grab();

		/* Get the timestamp */
		(void)fstat(fd, &statbuf);

		/* Drop permissions */
		safe_setuid_drop();

#endif /* VERIFY_TIMESTAMP */

		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), sizeof(vvv))) err = -1;

		/* What */
		if (err) what = "Cannot read savefile";

		/* Close the file */
		fd_close(fd);
	}

	/* Process file */
	if (!err)
	{
		/* Extract version */
		sf_major = vvv[0];
		sf_minor = vvv[1];
		sf_patch = vvv[2];
		sf_extra = vvv[3];

		/* Clear screen */
		Term_clear();

		if (older_than(OLD_VERSION_MAJOR, OLD_VERSION_MINOR, OLD_VERSION_PATCH, OLD_VERSION_EXTRA))
		{
			err = -1;
			what = "Savefile is too old";
		}
		else if (!older_than(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA + 1))
		{
			err = -1;
			what = "Savefile is from the future";
		}
		else
		{
			/* Attempt to load */
			err = rd_savefile();

			/* Message (below) */
			if (err) what = "Cannot parse savefile";
		}
	}

	/* Paranoia */
	if (!err)
	{
		/* Invalid turn */
		if (!turn) err = -1;

		/* Message (below) */
		if (err) what = "Broken savefile";
	}

#ifdef VERIFY_TIMESTAMP
	/* Verify timestamp */
	if (!err && !arg_wizard)
	{
		/* Hack -- Verify the timestamp */
		if (sf_when > (statbuf.st_ctime + 100) ||
		    sf_when < (statbuf.st_ctime - 100))
		{
			/* Message */
			what = "Invalid timestamp";

			/* Oops */
			err = -1;
		}
	}
#endif /* VERIFY_TIMESTAMP */


	/* Okay */
	if (!err)
	{
		/* Give a conversion warning */
		if ((version_major != sf_major) ||
		    (version_minor != sf_minor) ||
		    (version_patch != sf_patch))
		{
			/* Message */
			msg_format("Converted a %d.%d.%d savefile.",
			   sf_major, sf_minor, sf_patch);
			message_flush();
		}

		/* Player is dead */
		if (p_ptr->is_dead)
		{
			/* Forget death */
			p_ptr->is_dead = FALSE;

			/* Cheat death */
			if (arg_wizard)
			{
				/* A character was loaded */
				character_loaded = TRUE;

				/* Done */
				return (TRUE);
			}

			/* Count lives */
			sf_lives++;

			/* Forget turns */
			turn = 0;
			p_ptr->player_turn = p_ptr->resting_turn = 0;

			/* Done */
			return (TRUE);
		}

		/* A character was loaded */
		character_loaded = TRUE;

		/* Still alive */
		if (p_ptr->chp >= 0)
		{
			/* Reset cause of death */
			my_strcpy(p_ptr->died_from, "(alive and well)", sizeof(p_ptr->died_from));
		}

		/* Success */
		return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (TRUE)
	{
		char temp[1024];

		/* Extract name of lock file */
		my_strcpy(temp, savefile);
		my_strcat(temp, ".lok");

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove lock */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();
	}

#endif /* VERIFY_SAVEFILE */


	/* Message */
	msg_format("Error (%s) reading %d.%d.%d savefile.",
	   what, sf_major, sf_minor, sf_patch);
	message_flush();

	/* Oops */
	return (FALSE);
}
