/* File: save.c */

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
#include "option.h"

/*
 * XXX XXX XXX Ignore this for now...
 *
 * The basic format of Angband 2.8.0 (and later) savefiles is simple.
 *
 * The savefile itself is a "header" (4 bytes) plus a series of "blocks",
 * plus, perhaps, some form of "footer" at the end.
 *
 * The "header" contains information about the "version" of the savefile.
 * Conveniently, pre-2.8.0 savefiles also use a 4 byte header, though the
 * interpretation of the "sf_extra" byte is very different.  Unfortunately,
 * savefiles from Angband 2.5.X reverse the sf_major and sf_minor fields,
 * and must be handled specially, until we decide to start ignoring them.
 *
 * Each "block" is a "type" (2 bytes), plus a "size" (2 bytes), plus "data",
 * plus a "check" (2 bytes), plus a "stamp" (2 bytes).  The format of the
 * "check" and "stamp" bytes is still being contemplated, but it would be
 * nice for one to be a simple byte-checksum, and the other to be a complex
 * additive checksum of some kind.  Both should be zero if the block is empty.
 *
 * Standard types:
 *   TYPE_BIRTH --> creation info
 *   TYPE_OPTIONS --> option settings
 *   TYPE_MESSAGES --> message recall
 *   TYPE_PLAYER --> player information
 *   TYPE_SPELLS --> spell information
 *   TYPE_INVEN --> player inven/equip
 *   TYPE_STORES --> store information
 *   TYPE_RACES --> monster race data
 *   TYPE_KINDS --> object kind data
 *   TYPE_UNIQUES --> unique info
 *   TYPE_ARTIFACTS --> artifact info
 *   TYPE_QUESTS --> quest info
 *
 * Dungeon information:
 *   TYPE_DUNGEON --> dungeon info
 *   TYPE_FEATURES --> dungeon features
 *   TYPE_OBJECTS --> dungeon objects
 *   TYPE_MONSTERS --> dungeon monsters
 *
 * Conversions:
 *   Break old "races" into normals/uniques
 *   Extract info about the "unique" monsters
 *
 * Question:
 *   Should there be a single "block" for info about all the stores, or one
 *   "block" for each store?  Or one "block", which contains "sub-blocks" of
 *   some kind?  Should we dump every "sub-block", or just the "useful" ones?
 *
 * Question:
 *   Should the normals/uniques be broken for 2.8.0, or should 2.8.0 simply
 *   be a "fixed point" into which older savefiles are converted, and then
 *   future versions could ignore older savefiles, and the "conversions"
 *   would be much simpler.
 */


/*
 * XXX XXX XXX
 */
#define TYPE_OPTIONS 17362


/*
 * Some "local" parameters, used to help write savefiles
 */

static FILE	*fff;		/* Current save "file" */

static byte	xor_byte;	/* Simple encryption */

static u32b	v_stamp = 0L;	/* A simple "checksum" on the actual values */
static u32b	x_stamp = 0L;	/* A simple "checksum" on the encoded bytes */



/*
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v)
{
	/* Encode the value, write a character */
	xor_byte ^= v;
	(void)putc((int)xor_byte, fff);

	/* Maintain the checksum info */
	v_stamp += v;
	x_stamp += xor_byte;
}

static void wr_byte(byte v)
{
	sf_put(v);
}

static void wr_u16b(u16b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
}

static void wr_s16b(s16b v)
{
	wr_u16b((u16b)v);
}

static void wr_u32b(u32b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
	sf_put((byte)((v >> 16) & 0xFF));
	sf_put((byte)((v >> 24) & 0xFF));
}

static void wr_s32b(s32b v)
{
	wr_u32b((u32b)v);
}

static void wr_string(cptr str)
{
	while (*str)
	{
		wr_byte(*str);
		str++;
	}
	wr_byte(*str);
}


/*
 * These functions write info in larger logical records
 */


/*
 * Write an "item" record
 */
static void wr_item(const object_type *o_ptr)
{
	wr_s16b(o_ptr->k_idx);

	/* Location */
	wr_byte(o_ptr->iy);
	wr_byte(o_ptr->ix);

	wr_byte(o_ptr->tval);
	wr_byte(o_ptr->sval);
	wr_s16b(o_ptr->pval);

	wr_byte(o_ptr->stackc);

	wr_byte(o_ptr->show_idx);
	wr_byte(o_ptr->discount);
	wr_byte(o_ptr->feeling);
	wr_byte(o_ptr->spare);

	wr_byte(o_ptr->number);
	wr_s16b(o_ptr->weight);

	wr_byte(o_ptr->name1);
	wr_byte(o_ptr->name2);

	wr_s16b(o_ptr->timeout);
	wr_s16b(o_ptr->charges);

	wr_s16b(o_ptr->to_h);
	wr_s16b(o_ptr->to_d);
	wr_s16b(o_ptr->to_a);
	wr_s16b(o_ptr->ac);
	wr_byte(o_ptr->dd);
	wr_byte(o_ptr->ds);

	wr_u16b(o_ptr->ident);

	wr_byte(o_ptr->origin);
	wr_byte(o_ptr->origin_depth);
	wr_u16b(o_ptr->origin_xtra);

	/* Old flags */
	wr_u32b(0L);
	wr_u32b(0L);

	/* Held by monster index */
	wr_s16b(o_ptr->held_m_idx);

	/* Extra information */
	wr_byte(o_ptr->xtra1);
	wr_byte(o_ptr->xtra2);

	wr_u32b(o_ptr->can_flags1);
	wr_u32b(o_ptr->can_flags2);
	wr_u32b(o_ptr->can_flags3);
	wr_u32b(o_ptr->can_flags4);

	wr_u32b(o_ptr->may_flags1);
	wr_u32b(o_ptr->may_flags2);
	wr_u32b(o_ptr->may_flags3);
	wr_u32b(o_ptr->may_flags4);

	wr_u32b(o_ptr->not_flags1);
	wr_u32b(o_ptr->not_flags2);
	wr_u32b(o_ptr->not_flags3);
	wr_u32b(o_ptr->not_flags4);

	wr_s16b(o_ptr->usage);

	wr_byte(o_ptr->guess1);
	wr_byte(o_ptr->guess2);

	wr_s16b(o_ptr->name3);

	/* Save the inscription (if any) */
	if (o_ptr->note)
	{
		wr_string(quark_str(o_ptr->note));
	}
	else
	{
		wr_string("");
	}
}


/*
 * Write a "region" record
 */
static void wr_region(const region_type *r_ptr)
{
	/* Write the region */
	wr_s16b(r_ptr->type);

	wr_s16b(r_ptr->who);          	/* Source of effect - 'who'. */
	wr_s16b(r_ptr->what);			/* Source of effect - 'what'. */

	wr_s16b(r_ptr->effect);
	wr_s16b(r_ptr->method);
	wr_byte(r_ptr->level);
	wr_byte(r_ptr->facing);		/* Region facing */

	wr_s16b(r_ptr->damage);
	wr_s16b(r_ptr->child_region);

	wr_s16b(r_ptr->delay);     /* Number of turns effect has left */
	wr_s16b(r_ptr->delay_reset);			/* Number of turns to reset counter to when countdown has finished */
	wr_s16b(r_ptr->age);			/* Number of turns effect has been alive */
	wr_s16b(r_ptr->lifespan);		/* Number of turns effect will be alive */

	wr_byte(r_ptr->y0);			/* Source y location */
	wr_byte(r_ptr->x0);			/* Source x location */
	wr_byte(r_ptr->y1);			/* Destination y location */
	wr_byte(r_ptr->x1);			/* Destination x location */

	wr_u32b(r_ptr->flags1);		/* Ongoing effect bitflags */
}


/*
 * Write a "region piece" record
 */
static void wr_region_piece(const region_piece_type *rp_ptr)
{
	/* Write the region piece */
	wr_byte(rp_ptr->y);
	wr_byte(rp_ptr->x);
	wr_s16b(rp_ptr->d);
	wr_s16b(rp_ptr->region);

}


/*
 * Write a "monster" record
 */
static void wr_monster(const monster_type *m_ptr)
{
	/* Read the monster race */
	wr_s16b(m_ptr->r_idx);

	/* Read the other information */
	wr_byte(m_ptr->fy);
	wr_byte(m_ptr->fx);
	wr_s16b(m_ptr->hp);
	wr_s16b(m_ptr->maxhp);
	wr_s16b(m_ptr->csleep);
	wr_byte(m_ptr->mspeed);
	wr_byte(m_ptr->energy);
	wr_byte(m_ptr->stunned);
	wr_byte(m_ptr->confused);
	wr_byte(m_ptr->monfear);
	wr_byte(m_ptr->slowed);
	wr_byte(m_ptr->hasted);
	wr_byte(m_ptr->cut);
	wr_byte(m_ptr->poisoned);
	wr_byte(m_ptr->blind);
	wr_byte(m_ptr->dazed);
	wr_byte(m_ptr->image);
	wr_byte(m_ptr->amnesia);
	wr_byte(m_ptr->terror);
	wr_byte(m_ptr->tim_invis);
	wr_byte(m_ptr->tim_passw);
	wr_byte(m_ptr->bless);
	wr_byte(m_ptr->berserk);
	wr_byte(m_ptr->shield);
	wr_byte(m_ptr->oppose_elem);
	wr_byte(m_ptr->summoned);
	wr_byte(m_ptr->petrify);
	wr_byte(m_ptr->facing);

	wr_u32b(m_ptr->mflag);
	wr_u32b(m_ptr->smart);

	wr_byte(m_ptr->min_range);
	wr_byte(m_ptr->best_range);
	wr_byte(m_ptr->ty);
	wr_byte(m_ptr->tx);

}


/*
 * Write a "lore" record
 */
static void wr_lore(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[r_idx];

	/* Count sights/deaths/kills */
	wr_s16b(l_ptr->sights);
	wr_s16b(l_ptr->deaths);
	wr_s16b(l_ptr->pkills);
	wr_s16b(l_ptr->tkills);
	
	/* Count player attacks on monster */
	wr_s16b(l_ptr->tblows);
	wr_s16b(l_ptr->tdamage);

	/* Count wakes and ignores */
	wr_byte(l_ptr->wake);
	wr_byte(l_ptr->ignore);

	/* Extra stuff */
	wr_byte(l_ptr->xtra1);
	wr_byte(l_ptr->xtra2);

	/* Count drops */
	wr_byte(l_ptr->drop_gold);
	wr_byte(l_ptr->drop_item);

	/* Count spells */
	wr_byte(l_ptr->cast_innate);
	wr_byte(l_ptr->cast_spell);

	/* Count blows of each type */
	wr_byte(l_ptr->blows[0]);
	wr_byte(l_ptr->blows[1]);
	wr_byte(l_ptr->blows[2]);
	wr_byte(l_ptr->blows[3]);

	/* Memorize flags */
	wr_u32b(l_ptr->flags1);
	wr_u32b(l_ptr->flags2);
	wr_u32b(l_ptr->flags3);
	wr_u32b(l_ptr->flags4);
	wr_u32b(l_ptr->flags5);
	wr_u32b(l_ptr->flags6);
	wr_u32b(l_ptr->flags7);
	wr_u32b(l_ptr->flags8);
	wr_u32b(l_ptr->flags9);

	/* Monster limit per level */
	wr_byte(r_ptr->max_num);

	/* Later (?) */
	wr_byte(0);
	wr_byte(0);
	wr_byte(0);
}


/*
 * Read a random quest
 */
static void wr_event(int n, int s)
{
	quest_event *qe_ptr = &q_list[n].event[s];

	/* Read the quest flags */
	wr_u32b(qe_ptr->flags);

	/* Read the quest  */
	wr_byte(qe_ptr->dungeon);
	wr_byte(qe_ptr->level);
	wr_byte(qe_ptr->room);
	wr_byte(qe_ptr->action);
	wr_s16b(qe_ptr->feat);
	wr_s16b(qe_ptr->store);
	wr_s16b(qe_ptr->race);
	wr_s16b(qe_ptr->kind);
	wr_s16b(qe_ptr->number);
	wr_byte(qe_ptr->artifact);
	wr_byte(qe_ptr->ego_item_type);
	wr_s16b(qe_ptr->room_type_a);
	wr_s16b(qe_ptr->room_type_b);
	wr_u32b(qe_ptr->room_flags);
	wr_s16b(qe_ptr->owner);
	wr_s16b(qe_ptr->power);
	wr_s16b(qe_ptr->experience);
	wr_s16b(qe_ptr->gold);
}



/*
 * Write an "xtra" record
 */
static void wr_xtra(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Write awareness */
	wr_u16b(k_ptr->aware);

	/* Write sval guess */
	wr_byte(k_ptr->guess);

	/* Activations ever */
	wr_u16b(k_ptr->ever_used);

	/* Activations */
	wr_u16b(k_ptr->used);
}


/*
 * Write a "store" record
 */
static void wr_store(const store_type *st_ptr)
{
	int j;

	/* Save the store index */
	wr_byte(st_ptr->index);

	/* Save the "open" counter */
	wr_u32b(st_ptr->store_open);

	/* Save the "insults" */
	wr_s16b(st_ptr->insult_cur);

	/* Save the current owner */
	wr_byte(st_ptr->owner);

	/* Save the stock size */
	wr_byte(st_ptr->stock_num);

	/* Save the "haggle" info */
	wr_s16b(st_ptr->good_buy);
	wr_s16b(st_ptr->bad_buy);

	/* Save the store generation level */
	wr_byte(st_ptr->level);

	/* Save the stock */
	for (j = 0; j < st_ptr->stock_num; j++)
	{
		/* Save each item in stock */
		wr_item(&st_ptr->stock[j]);
	}
}


/*
 * Write RNG state
 */
static errr wr_randomizer(void)
{
	int i;

	/* Zero */
	wr_u16b(0);

	/* Place */
	wr_u16b(Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		wr_u32b(Rand_state[i]);
	}

	/* Success */
	return (0);
}


/*
 * Write the "options"
 */
static void wr_options(void)
{
	int i, k;

	u32b flag[8];
	u32b mask[8];
	u32b window_flag[ANGBAND_TERM_MAX];
	u32b window_mask[ANGBAND_TERM_MAX];


	/*** Oops ***/

	/* Oops */
	for (i = 0; i < 4; i++) wr_u32b(0L);


	/*** Special Options ***/

	/* Write "delay_factor" */
	wr_byte(op_ptr->delay_factor);

	/* Write "hitpoint_warn" */
	wr_byte(op_ptr->hitpoint_warn);

	/* Write "monster list display mode" */
	wr_byte(op_ptr->monlist_display);

	/* Write "monster list sort mode" */
	wr_byte(op_ptr->monlist_sort_by);

	/*** Normal options ***/

	/* Reset */
	for (i = 0; i < 8; i++)
	{
		flag[i] = 0L;
		mask[i] = 0L;
	}

	/* Analyze the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		int os = i / 32;
		int ob = i % 32;

		/* Process real entries */
		if (option_name(i))
		{
			/* Set flag */
			if (op_ptr->opt[i])
			{
				/* Set */
				flag[os] |= (1L << ob);
			}

			/* Set mask */
			mask[os] |= (1L << ob);
		}
	}

	/* Dump the flags */
	for (i = 0; i < 8; i++) wr_u32b(flag[i]);

	/* Dump the masks */
	for (i = 0; i < 8; i++) wr_u32b(mask[i]);


	/*** Window options ***/

	/* Reset */
	for (i = 0; i < ANGBAND_TERM_MAX; i++)
	{
		/* Flags */
		window_flag[i] = op_ptr->window_flag[i];

		/* Mask */
		window_mask[i] = 0L;

		/* Build the mask */
		for (k = 0; k < 32; k++)
		{
			/* Set mask */
			if (window_flag_desc[k])
			{
				window_mask[i] |= (1L << k);
			}
		}
	}

	/* Dump the flags */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) wr_u32b(window_flag[i]);

	/* Dump the masks */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) wr_u32b(window_mask[i]);
}


/*
 * Hack -- Write the "ghost" info
 */
static void wr_ghost(void)
{
	int i;

	/* Name */
	wr_string("Broken Ghost");

	/* Hack -- stupid data */
	for (i = 0; i < 60; i++) wr_byte(0);
}


/*
 * Write some "extra" info
 */
static void wr_extra(void)
{
	int i;

	u16b tmp16u;

	wr_string(op_ptr->full_name);

	wr_string(p_ptr->died_from);

	wr_string(p_ptr->history);

	/* Race/Class/Gender/Spells */
	wr_byte(p_ptr->prace);
	wr_byte(p_ptr->pclass);
	wr_byte(p_ptr->psex);
	wr_byte(p_ptr->pstyle);	/* Was oops */

	/* School */
	wr_byte(p_ptr->pschool);

	/* Forms of Sauron killed on current level */
	wr_byte(p_ptr->sauron_forms);

	wr_byte(p_ptr->expfact);

	wr_s16b(p_ptr->age);
	wr_s16b(p_ptr->ht);

	/* Unneeded */
	wr_s16b(p_ptr->wt);

	/* Dump the stats (maximum and current) */
	for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_max[i]);
	for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_cur[i]);
	for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_birth[i]);

	/* Ignore the transient stats */
	for (i = 0; i < 12; ++i) wr_s16b(0);

	wr_u32b(p_ptr->au);
	wr_u32b(p_ptr->birth_au);

	wr_u32b(p_ptr->max_exp);
	wr_u32b(p_ptr->exp);
	wr_u16b(p_ptr->exp_frac);
	wr_s16b(p_ptr->lev);

	wr_s16b(p_ptr->mhp);
	wr_s16b(p_ptr->chp);
	wr_u16b(p_ptr->chp_frac);

	wr_s16b(p_ptr->msp);
	wr_s16b(p_ptr->csp);
	wr_u16b(p_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(p_ptr->max_lev);
	wr_s16b(p_ptr->max_depth);

	/* More info */

	/* Hack --- save psval here.*/
	wr_byte(p_ptr->psval);

	/* Hack --- save shape here.*/
	wr_byte(p_ptr->pshape);

	/* Hack -- save familiar here */
	wr_byte(p_ptr->familiar);

	for (i = 0; i < MAX_FAMILIAR_GAINS; i++)
	{
		wr_u16b(p_ptr->familiar_attr[i]);
	}

	/* Write some important values */
	wr_s16b(p_ptr->sc);
	wr_s16b(p_ptr->food);
	wr_s16b(p_ptr->rest);
	wr_s16b(p_ptr->energy);

	/* Dump the "player timer" entries */
	tmp16u = TMD_MAX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_s16b(p_ptr->timed[i]);
	}

	/* Write some other player values */
	wr_byte(p_ptr->charging);
	wr_byte(p_ptr->climbing);
	wr_byte(p_ptr->searching);

	wr_byte(p_ptr->blocking);
	wr_byte(p_ptr->dodging);
	wr_byte(p_ptr->not_sneaking);

	wr_byte(p_ptr->sneaking);
	wr_byte(p_ptr->reserves);
	wr_byte(0);

	wr_u32b(p_ptr->disease);

	/* # of player turns */
	wr_s32b(p_ptr->player_turn);

	/* # of turns spent resting */
	wr_s32b(p_ptr->resting_turn);

	/* Held song */
	wr_s16b(p_ptr->held_song);
	wr_s16b(p_ptr->spell_trap);
	wr_s16b(p_ptr->delay_spell);

	/* Returning */
	wr_s16b(p_ptr->word_return);
	wr_s16b(p_ptr->return_y);
	wr_s16b(p_ptr->return_x);

	/* Write the "object seeds" */
	wr_u32b(seed_flavor);
	wr_u32b(seed_town);

	/* Write the "dungeon debugging seeds" */
	wr_u32b(seed_dungeon);
	wr_u32b(seed_last_dungeon);

	/* Special stuff */
	wr_u16b(p_ptr->panic_save);
	wr_u16b(p_ptr->total_winner);
	wr_u16b(p_ptr->noscore);

	/* Write death */
	wr_byte(p_ptr->is_dead);

	/* Write feeling */
	wr_byte(feeling);

	/* Turn of last "feeling" */
	wr_s32b(old_turn);

	/* Current turn */
	wr_s32b(turn);

	/* Dump the "player hp" entries */
	tmp16u = PY_MAX_LEVEL;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_s16b(p_ptr->player_hp[i]);
	}

	/* Write study data */
	wr_s16b(p_ptr->pack_size_reduce_study);
	for (i = 0; i <p_ptr->pack_size_reduce_study; i++)
	{
		wr_s16b(p_ptr->study_slot[i]);
	}

	/* Write spell data */
	wr_u32b(p_ptr->spell_learned1);
	wr_u32b(p_ptr->spell_learned2);
	wr_u32b(p_ptr->spell_learned3);
	wr_u32b(p_ptr->spell_learned4);
	wr_u32b(p_ptr->spell_worked1);
	wr_u32b(p_ptr->spell_worked2);
	wr_u32b(p_ptr->spell_worked3);
	wr_u32b(p_ptr->spell_worked4);
	wr_u32b(p_ptr->spell_forgotten1);
	wr_u32b(p_ptr->spell_forgotten2);
	wr_u32b(p_ptr->spell_forgotten3);
	wr_u32b(p_ptr->spell_forgotten4);

	/* Dump the ordered spells */
	for (i = 0; i < PY_MAX_SPELLS; i++)
	{
		wr_s16b(p_ptr->spell_order[i]);
	}

}


/*
 * Dump the random artifacts
 */
static void wr_randarts(void)
{
	int i;

	wr_u16b(z_info->a_max);

	for (i = 0; i < z_info->a_max; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		wr_byte(a_ptr->tval);
		wr_byte(a_ptr->sval);
		wr_s16b(a_ptr->pval);

		wr_s16b(a_ptr->to_h);
		wr_s16b(a_ptr->to_d);
		wr_s16b(a_ptr->to_a);
		wr_s16b(a_ptr->ac);

		wr_byte(a_ptr->dd);
		wr_byte(a_ptr->ds);

		wr_s16b(a_ptr->weight);

		wr_s32b(a_ptr->cost);

		wr_u32b(a_ptr->flags1);
		wr_u32b(a_ptr->flags2);
		wr_u32b(a_ptr->flags3);
		wr_u32b(a_ptr->flags4);

		wr_byte(a_ptr->level);
		wr_byte(a_ptr->rarity);

		wr_u16b(a_ptr->activation);
		wr_u16b(a_ptr->time);
		wr_u16b(a_ptr->randtime);

		wr_s32b(a_ptr->power);
	}
}


/*
 * The cave grid flags that get saved in the savefile
 */
#define CAVE_IMPORTANT_FLAGS (CAVE_GLOW | CAVE_ROOM)

/*
 * The player grid flags that get saved in the savefile
 */
#define PLAY_IMPORTANT_FLAGS (PLAY_MARK | PLAY_SAFE)


/*
 * Write the current dungeon
 */
static void wr_dungeon(void)
{
	int i, y, x;

	byte tmp8u;
	s16b tmp16u;

	byte count;
	s16b prev_char;


	/*** Basic info ***/

	/* Dungeon specific info follows */
	wr_u16b(p_ptr->depth);
	wr_u16b(p_ptr->dungeon);
	wr_u16b(p_ptr->py);
	wr_u16b(p_ptr->px);
	wr_u16b(DUNGEON_HGT);
	wr_u16b(DUNGEON_WID);
	wr_u16b(p_ptr->town);
	wr_u32b(level_flag);


	/*** Simple "Run-Length-Encoding" of cave ***/

	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;

	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Extract the important cave_info flags */
			tmp8u = (cave_info[y][x] & (CAVE_IMPORTANT_FLAGS));

			/* If the run is broken, or too full, flush it */
			if ((tmp8u != prev_char) || (count == MAX_UCHAR))
			{
				wr_byte((byte)count);
				wr_byte((byte)prev_char);
				prev_char = tmp8u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte((byte)count);
		wr_byte((byte)prev_char);
	}


	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;

	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Extract the important play_info flags */
			tmp8u = (play_info[y][x] & (PLAY_IMPORTANT_FLAGS));

			/* If the run is broken, or too full, flush it */
			if ((tmp8u != prev_char) || (count == MAX_UCHAR))
			{
				wr_byte((byte)count);
				wr_byte((byte)prev_char);
				prev_char = tmp8u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte((byte)count);
		wr_byte((byte)prev_char);
	}

	/*** Simple "Run-Length-Encoding" of cave ***/

	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;

	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{

			/* Extract the important cave_feats */
			tmp16u = cave_feat[y][x];

			/* If the run is broken, or too full, flush it */
			if ((tmp16u != prev_char) || (count == MAX_UCHAR))
			{
				wr_byte((byte)count);
				wr_s16b((s16b)prev_char);
				prev_char = tmp16u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte((byte)count);
		wr_s16b((s16b)prev_char);
	}

	/*** Dump room descriptions ***/
	for (x = 0; x < MAX_ROOMS_ROW; x++)
	{
		for (y = 0; y < MAX_ROOMS_COL; y++)
		{
			wr_byte(dun_room[x][y]);
		}
	}

	for (i = 1; i < DUN_ROOMS; i++)
	{
		int j;

		wr_s16b(room_info[i].type);
		wr_s16b(room_info[i].vault);
		wr_u32b(room_info[i].flags);
		wr_s16b(room_info[i].deepest_race);
		wr_u32b(room_info[i].ecology);

		for (j = 0; j < ROOM_DESC_SECTIONS; j++)
		{
			wr_s16b(room_info[i].section[j]);

			if (room_info[i].section[j] == -1) break;
		}
	}

	/*** Compact ***/

	/* Compact the objects */
	compact_objects(0);

	/* Compact the regions */
	compact_regions(0);

	/* Compact the regions */
	compact_region_pieces(0);

	/* Compact the monsters */
	compact_monsters(0);


	/*** Dump objects ***/

	/* Total objects */
	wr_u16b(o_max);

	/* Dump the objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Dump it */
		wr_item(o_ptr);
	}


	/*** Dump the regions ***/

	/* Total regions */
	wr_u16b(region_max);

	/* Dump the regions */
	for (i = 1; i < region_max; i++)
	{
		region_type *r_ptr = &region_list[i];

		/* Dump it */
		wr_region(r_ptr);
	}


	/*** Dump the region pieces ***/

	/* Total regions */
	wr_u16b(region_piece_max);

	/* Dump the regions */
	for (i = 1; i < region_piece_max; i++)
	{
		region_piece_type *rp_ptr = &region_piece_list[i];

		/* Dump it */
		wr_region_piece(rp_ptr);
	}


	/*** Dump the monsters ***/

	/* Total monsters */
	wr_u16b(m_max);

	/* Dump the monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Dump it */
		wr_monster(m_ptr);
	}

	/*** Dump the monster ecology ***/
	if (cave_ecology.ready)
	{
		/* Total races in ecology */
		wr_u16b(cave_ecology.num_races);

		/* Total number of ecologies. This is only used in wizard2.c, but we may need it later */
		wr_byte(cave_ecology.num_ecologies);

		/* Dump the monsters */
		for (i = 0; i < cave_ecology.num_races; i++)
		{
			/* Dump it */
			wr_u16b(cave_ecology.race[i]);
			wr_u32b(cave_ecology.race_ecologies[i]);
		}
	}
	else
	{
		/* Hack -- no ecology */
		wr_u16b(0);
		wr_byte(0);
	}
}



/*
 * Actually write a save-file
 */
static bool wr_savefile_new(void)
{
	int i, j;

	u32b now;

	u16b tmp16u;

	s16b tmp16s;

	/* Guess at the current time */
	now = (u32b)time((time_t *)0);


	/* Note the operating system */
	sf_xtra = 0L;

	/* Note when the file was saved */
	sf_when = now;

	/* Note the number of saves */
	sf_saves++;


	/*** Actually write the file ***/

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(VERSION_MINOR);
	xor_byte = 0;
	wr_byte(VERSION_PATCH);
	xor_byte = 0;
	wr_byte(VERSION_EXTRA);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;


	/* Operating system */
	wr_u32b(sf_xtra);


	/* Time file last saved */
	wr_u32b(sf_when);

	/* Number of past lives */
	wr_u16b(sf_lives);

	/* Number of times saved */
	wr_u16b(sf_saves);


	/* Space */
	wr_u32b(0L);
	wr_u32b(0L);


	/* Write the RNG state */
	wr_randomizer();


	/* Write the boolean "options" */
	wr_options();


	/* Dump the number of "messages" */
	tmp16s = message_num();
	if (tmp16s > 800) tmp16s = 800;
	wr_s16b(tmp16s);

	/* Dump the messages (oldest first!) */
	for (i = tmp16s - 1; i >= 0; i--)
	{
		wr_string(message_str((s16b)i));
		wr_u16b(message_type((s16b)i));
	}

	/* Dump the number of help tip file names */
	wr_s16b(tips_end);

	/* Dump the help tip file names */
	for (i = 0; i < tips_end; i++)
	{
		/* Read the file name */
		wr_string(quark_str(tips[i]));
	}

	/* Dump the position of the first not shown tip */
	wr_s16b(tips_start);


	/* Dump the monster lore */
	tmp16u = z_info->r_max;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_lore(i);


	/* Dump the object memory */
	tmp16u = z_info->k_max;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_xtra(i);

	/* Hack -- Dump the quests */
	tmp16u = MAX_Q_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_byte(MAX_QUEST_EVENTS);

		for (j = 0; j < MAX_QUEST_EVENTS; j++)
		{

			wr_event(i, j);
		}

		wr_byte(q_list[i].stage);
	}

	/* Random artifact seed */
	wr_u32b(seed_randart);

	/* Hack -- Dump the artifact lore */
	tmp16u = z_info->a_max;
	wr_u16b(tmp16u);

	for (i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		object_info *n_ptr = &a_list[i];

		wr_byte(a_ptr->cur_num);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);

		wr_u32b(n_ptr->can_flags1);
		wr_u32b(n_ptr->can_flags2);
		wr_u32b(n_ptr->can_flags3);
		wr_u32b(n_ptr->can_flags4);

		wr_u32b(n_ptr->not_flags1);
		wr_u32b(n_ptr->not_flags2);
		wr_u32b(n_ptr->not_flags3);
		wr_u32b(n_ptr->not_flags4);

		/* Activations */
		wr_u16b(a_ptr->activated);

		/* Oops */
		wr_byte(0);
		wr_byte(0);
	}

	/* Hack -- Dump the ego items lore */
	tmp16u = z_info->e_max;
	wr_u16b(tmp16u);

        for (i = 0; i < tmp16u; i++)
        {
                object_lore *n_ptr = &e_list[i];

                wr_u32b(n_ptr->can_flags1);
                wr_u32b(n_ptr->can_flags2);
                wr_u32b(n_ptr->can_flags3);
                wr_u32b(n_ptr->can_flags4);

                wr_u32b(n_ptr->may_flags1);
                wr_u32b(n_ptr->may_flags2);
                wr_u32b(n_ptr->may_flags3);
                wr_u32b(n_ptr->may_flags4);

                wr_u32b(n_ptr->not_flags1);
                wr_u32b(n_ptr->not_flags2);
                wr_u32b(n_ptr->not_flags3);
                wr_u32b(n_ptr->not_flags4);

                wr_u16b(e_info[i].aware);

                /* Oops */
               	wr_byte(0);
                wr_byte(0);
        }

	/* Write the "extra" information */
	wr_extra();

	/* Don't bother saving the learnt flavor flags if dead */
	if (!p_ptr->is_dead)
	{
		/* Hack -- Dump the flavors */
		tmp16u =z_info->x_max;
		wr_u16b(tmp16u);

	        for (i = 0; i < tmp16u; i++)
		{
                	object_info *n_ptr = &x_list[i];

                	wr_u32b(n_ptr->can_flags1);
                	wr_u32b(n_ptr->can_flags2);
                	wr_u32b(n_ptr->can_flags3);
                	wr_u32b(n_ptr->can_flags4);

                	wr_u32b(n_ptr->not_flags1);
                	wr_u32b(n_ptr->not_flags2);
                	wr_u32b(n_ptr->not_flags3);
                	wr_u32b(n_ptr->not_flags4);
		}
        }

	wr_randarts();

	/* Write the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Dump index */
		wr_u16b((u16b)i);

		/* Dump object */
		wr_item(o_ptr);
	}

	/* Add a sentinel */
	wr_u16b(0xFFFF);

	/* Write the number of bag types */
	wr_u16b((u16b)SV_BAG_MAX_BAGS);

	/* Write the bag contents */
	for (i = 0; i < SV_BAG_MAX_BAGS; i++)
	{
		wr_byte(INVEN_BAG_TOTAL);

		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			wr_u16b((u16b)bag_contents[i][j]);
		}
	}

	/* Write the number of dungeon types */
	wr_u16b((u16b)z_info->t_max);

	/* Write the dungeon max depths */
	for (i = 0; i < z_info->t_max; i++)
	{
		wr_byte(t_info[i].attained_depth);
		wr_byte(t_info[i].visited);
		wr_u16b(t_info[i].guardian_ifvisited);
		wr_u16b(t_info[i].replace_guardian);

		/* Write the store indexes if alive */
		if (!p_ptr->is_dead)
		{
			wr_byte(MAX_STORES);

			/* Write the actual store indexes */
			for (j = 0; j < MAX_STORES; j++)
			{
				wr_u16b((u16b)t_info[i].store_index[j]);
			}
		}
	}

	/* Player is not dead, write the stores and the dungeon */
	if (!p_ptr->is_dead)
	{
		/* Note the stores */
		tmp16u = total_store_count;
		wr_u16b(tmp16u);

		/* Dump the stores */
		for (i = 0; i < tmp16u; i++) wr_store(store[i]);

		/* Dump the dungeon */
		wr_dungeon();

		/* Dump the ghost */
		wr_ghost();
	}


	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);


	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}


/*
 * Medium level player saver
 *
 * XXX XXX XXX Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(cptr name)
{
	bool ok = FALSE;

	int fd;

	int mode = 0644;


	/* No file yet */
	fff = NULL;

	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);

	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	fd = fd_make(name, mode);

	/* Drop permissions */
	safe_setuid_drop();

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_savefile_new()) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove "broken" files */
		if (!ok) fd_kill(name);

		/* Drop permissions */
		safe_setuid_drop();
	}


	/* Failure */
	if (!ok) return (FALSE);

	/* Successful save */
	character_saved = TRUE;

	/* Success */
	return (TRUE);
}

/*
 * Attempt to save the player in a savefile
 */
bool save_player(void)
{
	return save_player_bkp(FALSE);
}


/*
 * Attempt to save the player in a savefile
 */
bool save_player_bkp(bool bkp)
{
	int result = FALSE;

	char safe[1024];

	char target_savefile[1024];

#ifdef SET_UID

# ifdef SECURE

	/* Get "games" permissions */
	beGames();

# endif

#endif

	if (bkp)
	{
		/* The target will be the backup savefile */
		my_strcpy(target_savefile, savefile, sizeof(safe));
		my_strcat(target_savefile, ".bkp", sizeof(safe));
	}
	else
	{
		/* The target will be the regular savefile */
		my_strcpy(target_savefile, savefile, sizeof(safe));
	}

	/* New savefile */
	my_strcpy(safe, savefile, sizeof(safe));
	my_strcat(safe, ".new", sizeof(safe));

#ifdef VM
	/* Hack -- support "flat directory" usage on VM/ESA */
	my_strcpy(safe, savefile, sizeof(new));
	my_strcat(safe, "n", sizeof(new));
#endif /* VM */

	/* Grab permissions */
	safe_setuid_grab();

	/* Remove it */
	fd_kill(safe);

	/* Drop permissions */
	safe_setuid_drop();

	/* Attempt to save the player */
	if (save_player_aux(safe))
	{
		char temp[1024];

		/* Old savefile */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".old", sizeof(temp));

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, "o", sizeof(temp));
#endif /* VM */

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove it */
		fd_kill(temp);

		/* Preserve old savefile */
		fd_move(target_savefile, temp);

		/* Activate new savefile */
		fd_move(safe, target_savefile);

		/* Remove preserved savefile */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

		/* Hack -- Pretend the character was loaded */
		character_loaded = TRUE;

#ifdef VERIFY_SAVEFILE

		/* Lock on savefile */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".lok", sizeof(temp));

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove lock file */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

#endif /* VERIFY_SAVEFILE */

		/* Success */
		result = TRUE;
	}


#ifdef SET_UID

# ifdef SECURE

	/* Drop "games" permissions */
	bePlayer();

# endif /* SECURE */

#endif /* SET_UID */


	/* Return the result */
	return (result);
}
