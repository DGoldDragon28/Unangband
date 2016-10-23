/* File: dungeon.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001 - 2009 Andrew Doull. Modifications to the Angband 2.9.1
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"


/*
 * Ensure that the components required for any active quests are on the level.
 */
void ensure_quest(void)
{
	int i, n, j, k;
	int idx, x_idx, y_idx;
	int y, x;

	bool used_race;
	bool used_item;

	/* Hack -- ensure quest components */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		quest_type *q_ptr = &q_list[i];
		quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);

		/* Hack -- player's actions don't change level */
		if (q_ptr->stage == QUEST_ACTION) continue;

		/* Special handling for pay out */
		if (q_ptr->stage == QUEST_PAYOUT)
		{
			/* Hack -- Don't pay out while recalling */
			if (p_ptr->word_recall || p_ptr->word_return) continue;
		}

		/* Hack: Quest occurs anywhere - don't force quest items on the level. */
		else if (!qe_ptr->dungeon) continue;

		/* Quest doesn't occur on this level */
		else if ((qe_ptr->dungeon != p_ptr->dungeon) ||
			(qe_ptr->level != (p_ptr->depth - min_depth(p_ptr->dungeon)))) continue;

		/* Initialise count */
		used_race = FALSE;
		used_item = FALSE;

		/* Hack -- don't generate items on the level if we need it from a store or are giving it instead. */
		if (qe_ptr->flags & (EVENT_GET_STORE | EVENT_GIVE_STORE | EVENT_LOSE_ITEM |
				EVENT_GIVE_RACE | EVENT_STOCK_STORE | EVENT_BUY_STORE | EVENT_SELL_STORE |
				EVENT_GIVE_STORE)) used_item = TRUE;

		/* Require features */
		if (qe_ptr->feat)
		{
			n = 0;
			y_idx = 0;
			x_idx = 0;
			k = 0;

			/* Check for feature type */
			while (n < qe_ptr->number)
			{
				/* Count quest features */
				for (y = 0; y < DUNGEON_HGT; y++)
				{
					for (x = 0; x < DUNGEON_WID; x++)
					{
						/* Check if feat okay */
						if (cave_feat[y][x] == qe_ptr->feat)
							if (!rand_int(++n)) { y_idx = y; x_idx = x; }
					}
				}

				/* Alter feat if required */
				if ((q_ptr->stage > QUEST_ACTION) && (qe_ptr->flags & (EVENT_ALTER_FEAT)))
				{
					/* Alter the feature if at least one found */
					if (n) cave_alter_feat(y_idx, x_idx, qe_ptr->action);

					/* Set to number of features altered */
					n = ++k;

					continue;
				}

				/* Try placing remaining features */
				for ( ; n < qe_ptr->number; n++)
				{
					int num = 0;

					/* Hack -- place features close if we are paying out */
					bool close = (q_ptr->stage == QUEST_PAYOUT) || (q_ptr->stage == QUEST_FORFEIT);

					while (num++ < 2000)
					{
						if (!close || (num > 50))
						{
							/* Location */
							y = rand_int(DUNGEON_HGT);
							x = rand_int(DUNGEON_WID);
						}
						else
						{
							scatter(&y, &x, p_ptr->py, p_ptr->px, (num / 4) + 3, CAVE_XLOF);
						}

						/* Require empty, clean, floor grid */
						if (!cave_naked_bold(y, x)) continue;

						/* Check for distance from player */
						if ((num < 500) && (close != (distance(p_ptr->py, p_ptr->px, y, x) <= MAX_SIGHT))) continue;

						/* Accept it */
						break;
					}

					/* Create the feature */
					cave_set_feat(y, x, qe_ptr->feat);

					/* Guard the feature */
					if ((qe_ptr->race) && !(qe_ptr->flags & (EVENT_DEFEND_FEAT)))
					{
						used_race = TRUE;
						race_near(qe_ptr->race, y, x);
					}

					/* XXX Hide item in the feature */
					used_item = TRUE;
				}
			}

			/* Amend quest numbers */
			if (n > qe_ptr->number) qe_ptr->number = n;
		}

		/* Require race */
		if ((qe_ptr->race) && !(used_race))
		{
			n = 0;
			idx = 0;
			k = 0;

			/* Check for monster race */
			while (n < qe_ptr->number)
			{
				u32b flg;

				/* Count quest races */
				for (j = 0; j < z_info->m_max; j++)
				{
					/* Check if monster okay */
					if (m_list[j].r_idx == qe_ptr->race)
						if (!rand_int(++n)) idx = j;
				}

				/* Alter monster if required */
				if ((q_ptr->stage > QUEST_ACTION) && (qe_ptr->flags & (EVENT_KILL_RACE | EVENT_BANISH_RACE)))
				{
					/* Alter the feature if at least one found */
					if (n)
					{
						if (qe_ptr->flags & (EVENT_KILL_RACE))
						{
							/* Generate treasure etc */
							monster_death(idx);
						}

						/* Delete monster */
						delete_monster_idx(idx);
					}

					/* Set to number of monsters altered */
					n = ++k;

					continue;
				}

				/* Try placing remaining monsters */
				for ( ; n < qe_ptr->number; n++)
				{
					int num = 0;

					/* Hack -- place allies close if we are not questing to ally with them */
					bool ally = (q_ptr->stage > QUEST_ACTION) && (qe_ptr->flags & (EVENT_ALLY_RACE));

					/* Hack -- place features close if we are paying out */
					bool close = (q_ptr->stage == QUEST_PAYOUT) || (q_ptr->stage == QUEST_FORFEIT) || ally;

					/* Pick a "legal" spot */
					while (num++ < 2000)
					{
						if (!close || (num > 50))
						{
							/* Location */
							y = rand_int(DUNGEON_HGT);
							x = rand_int(DUNGEON_WID);
						}
						else
						{
							scatter(&y, &x, p_ptr->py, p_ptr->px, (num / 4) + 2, CAVE_XLOF);
						}

						/* Require empty grid */
						if (!cave_empty_bold(y, x)) continue;

						/* Require monster can pass and survive on terrain unless desperate */
						if ((num < 1000) && (place_monster_here(y, x, qe_ptr->race) > MM_FAIL)) continue;

						/* Check for distance from player */
						if ((num < 500) && (close != (distance(p_ptr->py, p_ptr->px, y, x) <= MAX_SIGHT))) continue;

						/* Accept it */
						break;
					}

					/* Nothing to start with */
					flg = 0L;

					/* Mark as allies */
					if (qe_ptr->flags & (EVENT_ALLY_RACE | EVENT_DEFEND_RACE))
					{
						/* Ally */
						flg |= (MFLAG_ALLY);

						/* Not a natural ally */
						if (!ally)
						{
							/* Ignoring the player */
							flg |= (MFLAG_IGNORE);
						}
					}

					/* Create a new monster (awake, no groups) */
					(void)place_monster_aux(y, x, qe_ptr->race, FALSE, FALSE, flg);

					/* XXX Monster should carry item */
					used_item = TRUE;
				}
			}

			/* Amend quest numbers */
			if (n > qe_ptr->number) qe_ptr->number = n;
		}

		/* Require object */
		if (((qe_ptr->artifact) || (qe_ptr->ego_item_type) || (qe_ptr->kind)) && !(used_item))
		{
			n = 0;
			idx = 0;
			k = 0;

			/* Check for object kind */
			while (n < qe_ptr->number)
			{
				/* Count quest objects */
				for (j = 0; j < z_info->m_max; j++)
				{
					/* Check if object okay */
					if (o_list[j].k_idx)
					{
						if ((qe_ptr->artifact) && (o_list[j].name1 != qe_ptr->artifact)) continue;
						if ((qe_ptr->ego_item_type) && (o_list[j].name2 != qe_ptr->ego_item_type)) continue;
						if ((qe_ptr->kind) && (o_list[j].k_idx != qe_ptr->kind)) continue;

						if (!rand_int(++n)) idx = j;
					}
				}

				/* Alter object if required */
				if ((q_ptr->stage > QUEST_ACTION) && (qe_ptr->flags & (EVENT_LOSE_ITEM | EVENT_DESTROY_ITEM)))
				{
					/* Alter the feature if at least one found */
					if (n)
					{
						/* Count objects */
						k  += o_list[idx].number - 1;

						/* Delete monster */
						delete_object_idx(idx);

						/* XXX Destroy artifact */
					}

					/* Set to number of objects altered */
					n = ++k;

					continue;
				}

				/* Try placing remaining objects */
				for ( ; n < qe_ptr->number; n++)
				{
					object_type object_type_body;
					object_type *o_ptr = &object_type_body;

					/* Hack -- place features close if we are paying out */
					bool close = (q_ptr->stage == QUEST_PAYOUT) || (q_ptr->stage == QUEST_FORFEIT);

					int num = 0;

					/* Pick a "legal" spot */
					while (num++ < 2000)
					{
						if (!close || (num > 50))
						{
							/* Location */
							y = rand_int(DUNGEON_HGT);
							x = rand_int(DUNGEON_WID);
						}
						else
						{
							scatter(&y, &x, p_ptr->py, p_ptr->px, (num / 4) + 2, CAVE_XLOF);
						}

						/* Require empty grid */
						if (!cave_naked_bold(y, x)) continue;

						/* Check for distance from player */
						if ((num < 500) && (close != (distance(p_ptr->py, p_ptr->px, y, x) <= MAX_SIGHT))) continue;

						/* Prepare artifact */
						if (qe_ptr->artifact) qe_ptr->kind = lookup_kind(a_info[qe_ptr->artifact].tval, a_info[qe_ptr->artifact].sval);

						/* Prepare ego item */
						if ((qe_ptr->ego_item_type) && !(qe_ptr->kind)) qe_ptr->kind =
							lookup_kind(e_info[qe_ptr->ego_item_type].tval[0],
								e_info[qe_ptr->ego_item_type].min_sval[0]);

						/* Prepare object */
						object_prep(o_ptr, qe_ptr->kind);

						/* Prepare artifact */
						o_ptr->name1 = qe_ptr->artifact;

						/* Prepare ego item */
						o_ptr->name2 = qe_ptr->ego_item_type;

						/* Apply magic -- hack: use player level as reward level */
						apply_magic(o_ptr, p_ptr->max_lev * 2, FALSE, FALSE, FALSE);

						/* Several objects */
						if (o_ptr->number > 1) n += o_ptr->number -1;

						/* Accept it */
						break;
					}
				}
			}

			/* Amend quest numbers */
			if (n > qe_ptr->number) qe_ptr->number = n;
		}

		/* Transition some quests once paid out */
		if (q_ptr->stage == QUEST_PAYOUT) q_ptr->stage = QUEST_FINISH;
		if (q_ptr->stage == QUEST_FORFEIT) q_ptr->stage = QUEST_PENALTY;
	}
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 * TODO: a lot of code is here duplicated with value_check_aux3 in spells2.c
 */
int value_check_aux1(const object_type *o_ptr)
{
	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_TERRIBLE);

		/* Normal */
		return (INSCRIP_SPECIAL);
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_WORTHLESS);

		/* Superb */
		if (o_ptr->xtra1) return (INSCRIP_SUPERB);

		/* Normal */
		return (INSCRIP_EXCELLENT);
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return (INSCRIP_CURSED);

	/* Broken items */
	if (broken_p(o_ptr)) return (INSCRIP_BROKEN);

	/* Coated items */
	if (coated_p(o_ptr)) return (INSCRIP_COATED);

	/* Magic item */
	if ((o_ptr->xtra1) && (object_power(o_ptr) > 0)) return (INSCRIP_EXCELLENT);

	/* Great "armor" bonus */
	if (o_ptr->to_a > 9) return (INSCRIP_GREAT);

	/* Great to_h bonus */
	if (o_ptr->to_h > 9) return (INSCRIP_GREAT);

	/* Great to_d bonus */
	if (o_ptr->to_d >
	    MAX(7, k_info[o_ptr->k_idx].dd * k_info[o_ptr->k_idx].ds))
	  return (INSCRIP_GREAT);

	/* Great "weapon" dice */
	if (o_ptr->dd > k_info[o_ptr->k_idx].dd) return (INSCRIP_GREAT);

	/* Great "weapon" sides */
	if (o_ptr->ds > k_info[o_ptr->k_idx].ds) return (INSCRIP_GREAT);

	/* Very good "armor" bonus */
	if (o_ptr->to_a > 5) return (INSCRIP_VERY_GOOD);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 7) return (INSCRIP_VERY_GOOD);

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return (INSCRIP_GOOD);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return (INSCRIP_GOOD);

	/* Default to "average" */
	return (INSCRIP_AVERAGE);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */
int value_check_aux2(const object_type *o_ptr)
{
	/* If sensed magical, have no more value to add */
	if ((o_ptr->feeling == INSCRIP_GOOD) || (o_ptr->feeling == INSCRIP_VERY_GOOD)
		|| (o_ptr->feeling == INSCRIP_GREAT) || (o_ptr->feeling == INSCRIP_EXCELLENT)
		|| (o_ptr->feeling == INSCRIP_SUPERB) || (o_ptr->feeling == INSCRIP_SPECIAL)
		|| (o_ptr->feeling == INSCRIP_MAGICAL)) return (0);

	/* Cursed items (all of them) */
	if (cursed_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_ARTIFACT) return (INSCRIP_TERRIBLE);
		else if (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM) return (INSCRIP_WORTHLESS);
		else if (o_ptr->feeling == INSCRIP_EGO_ITEM) return (INSCRIP_WORTHLESS);
		return (INSCRIP_CURSED);
	}

	/* Broken items (all of them) */
	if (broken_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_ARTIFACT) return (INSCRIP_TERRIBLE);
		else if (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM) return (INSCRIP_WORTHLESS);
		else if (o_ptr->feeling == INSCRIP_EGO_ITEM) return (INSCRIP_WORTHLESS);
		return (INSCRIP_BROKEN);
	}

	/* Coated items */
	if (coated_p(o_ptr)) return (INSCRIP_COATED);

	/* Artifacts -- except cursed/broken ones */
	if (artifact_p(o_ptr))
	{
		/* Known to be artifact strength */
		if ((o_ptr->feeling == INSCRIP_UNBREAKABLE)
			|| (o_ptr->feeling == INSCRIP_ARTIFACT)) return (INSCRIP_SPECIAL);

		return (INSCRIP_UNCURSED);
	}

	/* Ego-Items -- except cursed/broken ones */
	if (ego_item_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM) return (INSCRIP_SUPERB);
		else if (o_ptr->feeling == INSCRIP_EGO_ITEM) return (INSCRIP_EXCELLENT);
		return (INSCRIP_UNCURSED);
	}

	/* Magic-Items */
	if (o_ptr->xtra1)
	{
		if (o_ptr->feeling == INSCRIP_EGO_ITEM) return (INSCRIP_EXCELLENT);
		return (INSCRIP_UNCURSED);
	}

	/* Good armor bonus */
	if (o_ptr->to_a > 0)
	{
		if (o_ptr->feeling == INSCRIP_UNUSUAL) return (INSCRIP_MAGICAL);
		return (INSCRIP_UNCURSED);
	}

	/* Good weapon bonuses */
	if (o_ptr->to_h + o_ptr->to_d > 0)
	{
		if (o_ptr->feeling == INSCRIP_UNUSUAL) return (INSCRIP_MAGICAL);
		return (INSCRIP_UNCURSED);
	}

	/* No feeling */
	return (INSCRIP_AVERAGE);
}



/*
 * Sense the inventory
 *
 * Now instead of class-based sensing, this notices various
 * abilities on items that we don't notice any other way.
 */
static void sense_inventory(void)
{
	int i;

	int plev = p_ptr->lev;

	u32b f1=0x0L;
	u32b f2=0x0L;
	u32b f3=0x0L;
	u32b f4=0x0L;

	u32b af1=0x0L;
	u32b af2=0x0L;
	u32b af3=0x0L;
	u32b af4=0x0L;

	object_type *o_ptr;

	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->timed[TMD_CONFUSED]) return;

	/* No sensing when paralyzed */
	if (p_ptr->timed[TMD_PARALYZED]) return;

	/* No sensing when in stastis */
	if (p_ptr->timed[TMD_STASTIS]) return;

	/* No sensing when asleep */
	if (p_ptr->timed[TMD_PSLEEP] > PY_SLEEP_ASLEEP) return;

	/* No sensing when knocked out */
	if (p_ptr->timed[TMD_STUN] > 100) return;

	if (cp_ptr->sense_squared)
	{
		if (0 != rand_int(cp_ptr->sense_base / (plev * plev + cp_ptr->sense_div)))
			return;
	}
	else
	{
		if (0 != rand_int(cp_ptr->sense_base / (plev + cp_ptr->sense_div)))
			return;
	}


	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Hack -- we seem to get a source of corrupt objects that crash this routine. Putting this warning in. */
		if ((o_ptr->k_idx >= z_info->k_max) || (o_ptr->k_idx < 0))
		{
			msg_format("BUG: Object corruption detected (kind %d, held by %s). Please report.",o_ptr->k_idx, o_ptr->held_m_idx ? r_name + r_info[o_ptr->held_m_idx].name : "floor");

			o_ptr->k_idx = 0;
			continue;
		}

		/* Eggs become 'attuned' to the player if carried awhile, resulting in friendly monsters */
		if ((o_ptr->tval == TV_EGG) && (o_ptr->sval == SV_EGG_EGG) && (o_ptr->timeout)) o_ptr->ident = (IDENT_FORGED);

		/* Sense flags to see if we have ability */
		if ((i >= INVEN_WIELD) && !(IS_QUIVER_SLOT(i)))
		{
			u32b if1,if2,if3,if4;

			object_flags(o_ptr,&if1,&if2,&if3,&if4);

			af1 |= if1;
			af2 |= if2;
			af3 |= if3;
			af4 |= if4;

		}

		/* Sense flags to see if we gain ability */
		if (!(o_ptr->ident & (IDENT_MENTAL)) && (i >= INVEN_WIELD))
		{
			u32b if1,if2,if3,if4;

			object_flags(o_ptr,&if1,&if2,&if3,&if4);

			f1 |= (if1 & ~(o_ptr->may_flags1)) & ~(o_ptr->can_flags1);
			f2 |= (if2 & ~(o_ptr->may_flags2)) & ~(o_ptr->can_flags2);
			f3 |= (if3 & ~(o_ptr->may_flags3)) & ~(o_ptr->can_flags3);
			f4 |= (if4 & ~(o_ptr->may_flags4)) & ~(o_ptr->can_flags4);
		}
	}

	/* Hack --- silently notice stuff */
	if (f1 & (TR1_STEALTH)) equip_can_flags(TR1_STEALTH,0x0L,0x0L,0x0L);
	else if (!(af1 & (TR1_STEALTH)) ) equip_not_flags(TR1_STEALTH,0x0L,0x0L,0x0L);

	if (f1 & (TR1_SEARCH)) equip_can_flags(TR1_SEARCH,0x0L,0x0L,0x0L);
	else if (!(af1 & (TR1_SEARCH)) ) equip_not_flags(TR1_SEARCH,0x0L,0x0L,0x0L);

	if (f3 & (TR3_SLOW_DIGEST)) equip_can_flags(0x0L,0x0L,TR3_SLOW_DIGEST,0x0L);
	else if (!(af3 & (TR3_SLOW_DIGEST))) equip_not_flags(0x0L,0x0L,TR3_SLOW_DIGEST,0x0L);

	if (f3 & (TR3_REGEN_HP)) equip_can_flags(0x0L,0x0L,TR3_REGEN_HP,0x0L);
	else if (!(af3 & (TR3_REGEN_HP))) equip_not_flags(0x0L,0x0L,TR3_REGEN_HP,0x0L);

	if (f3 & (TR3_REGEN_MANA)) equip_can_flags(0x0L,0x0L,TR3_REGEN_MANA,0x0L);
	else if (!(af3 & (TR3_REGEN_MANA))) equip_not_flags(0x0L,0x0L,TR3_REGEN_MANA,0x0L);

	if (f3 & (TR3_HUNGER)) equip_can_flags(0x0L,0x0L,TR3_HUNGER,0x0L);
	else if (!(af3 & (TR3_HUNGER))) equip_not_flags(0x0L,0x0L,TR3_HUNGER,0x0L);

	/* Hack -- only notice the absence of this ability */
	if (f3 & (TR3_UNCONTROLLED)) /* Do nothing */;
	else if (!(af3 & (TR3_UNCONTROLLED))) equip_not_flags(0x0L,0x0L,TR3_UNCONTROLLED,0x0L);
}



/*
 * Regenerate hit points
 */
static void regenhp(int percent)
{
	s32b new_chp, new_chp_frac;
	int old_chp;

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/* Extract the new hitpoints */
	new_chp = ((long)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
	p_ptr->chp += (s16b)(new_chp >> 16);   /* div 65536 */

	/* check for overflow */
	if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;
	new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
	if (new_chp_frac >= 0x10000L)
	{
		p_ptr->chp_frac = (u16b)(new_chp_frac - 0x10000L);
		p_ptr->chp++;
	}
	else
	{
		p_ptr->chp_frac = (u16b)new_chp_frac;
	}

	/* Fully healed */
	if (p_ptr->chp >= p_ptr->mhp)
	{
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;
	}

	/* Notice changes */
	if (old_chp != p_ptr->chp)
	{
		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	}
}


/*
 * Regenerate mana points
 */
static void regenmana(int percent)
{
	s32b new_mana, new_mana_frac;
	int old_csp;

	old_csp = p_ptr->csp;
	new_mana = ((long)p_ptr->msp) * percent + PY_REGEN_MNBASE;
	p_ptr->csp += (s16b)(new_mana >> 16);	/* div 65536 */
	/* check for overflow */
	if ((p_ptr->csp < 0) && (old_csp > 0))
	{
		p_ptr->csp = MAX_SHORT;
	}
	new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
	if (new_mana_frac >= 0x10000L)
	{
		p_ptr->csp_frac = (u16b)(new_mana_frac - 0x10000L);
		p_ptr->csp++;
	}
	else
	{
		p_ptr->csp_frac = (u16b)new_mana_frac;
	}

	/* Must set frac to zero even if equal */
	if (p_ptr->csp >= p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
	}

	/* Redraw mana */
	if (old_csp != p_ptr->csp)
	{
		/* Update mana */
		p_ptr->update |= (PU_MANA);

		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	}
}




/*
 * Monster hook to use for 'wandering' monsters.
 */
bool dun_level_mon(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* If town, require a smart monster */
	if ((cave_ecology.town) && ((r_ptr->flags2 & (RF2_SMART)) == 0)) return (FALSE);
	
	/* If no restriction, accept anything */
	if (!(t_info[p_ptr->dungeon].r_char) && !(t_info[p_ptr->dungeon].r_flag)) return (TRUE);

	/* Hack -- Accept monsters with graphic */
	if ((t_info[p_ptr->dungeon].r_char) && (r_ptr->d_char == t_info[p_ptr->dungeon].r_char)) return (TRUE);

	/* Hack -- Accept monsters with flag */
	if (t_info[p_ptr->dungeon].r_flag)
	{
		int mon_flag = t_info[p_ptr->dungeon].r_flag-1;

		if ((mon_flag < 32) &&
			(r_ptr->flags1 & (1L << mon_flag))) return (TRUE);

		if ((mon_flag >= 32) &&
			(mon_flag < 64) &&
			(r_ptr->flags2 & (1L << (mon_flag -32)))) return (TRUE);

		if ((mon_flag >= 64) &&
			(mon_flag < 96) &&
			(r_ptr->flags3 & (1L << (mon_flag -64)))) return (TRUE);

		if ((mon_flag >= 96) &&
			(mon_flag < 128) &&
			(r_ptr->flags4 & (1L << (mon_flag -96)))) return (TRUE);
	}
	
	return (FALSE);

}


/*
 * Suffer from a disease.
 *
 * This now occurs whilst changing levels, and while in dungeon for
 * quick diseases only.
 *
 * allow_cure is set to true if the disease is a result of time
 * passing, and false if it's the result of monster blows.
 */
void suffer_disease(bool allow_cure)
{
	u32b old_disease = p_ptr->disease;

	/* Get hit by disease */
	if (p_ptr->disease & ((1 << DISEASE_BLOWS) - 1))
	{
		int i, n, effect = 0;

		msg_print("You feel an illness eat away at you.");

		disturb(0,0);

		n = 0;

		/* Select one of the possible effects that the player can suffer */
		for (i = 1; i < (1 << DISEASE_BLOWS); i <<=1)
		{
			if (!(p_ptr->disease & i)) continue;

			if (!rand_int(++n)) effect = i;
		}

		switch (effect)
		{
			case DISEASE_LOSE_STR:
			{
				dec_stat(A_STR, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				break;
			}

			case DISEASE_LOSE_INT:
			{
				dec_stat(A_INT, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				break;
			}

			case DISEASE_LOSE_WIS:
			{
				dec_stat(A_WIS, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				break;
			}

			case DISEASE_LOSE_DEX:
			{
				dec_stat(A_DEX, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				/* An exception --- disease does not glue DEX and AGI.
				   See DISEASE_SLOW below, however. */
				break;
			}

			case DISEASE_LOSE_CON:
			{
				dec_stat(A_CON, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				break;
			}

			case DISEASE_LOSE_CHR:
			{
				dec_stat(A_CHR, p_ptr->disease & (DISEASE_POWER) ? randint(6) : 1);
				break;
			}

			case DISEASE_HUNGER:
			{
				msg_print("You vomit up your food!");
				(void)set_food(PY_FOOD_STARVE - 1);
				disturb(0, 0);
				break;
			}

			case DISEASE_THIRST:
			{
				set_food(p_ptr->food - randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) - 10);
				break;
			}

			case DISEASE_CUT:
			{
				inc_timed(TMD_CUT, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_STUN:
			{
				inc_timed(TMD_STUN,randint(p_ptr->disease & (DISEASE_POWER) ? 40 : 10) + 2, FALSE);
				break;
			}

			case DISEASE_POISON:
			{
				inc_timed(TMD_POISONED, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_BLIND:
			{
				inc_timed(TMD_BLIND, randint(p_ptr->disease & (DISEASE_POWER) ? 40 : 10) + 2, FALSE);
				break;
			}

			case DISEASE_FEAR:
			{
				inc_timed(TMD_AFRAID, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_CONFUSE:
			{
				inc_timed(TMD_CONFUSED, randint(p_ptr->disease & (DISEASE_POWER) ? 10 : 3) + 1, FALSE);
				break;
			}

			case DISEASE_HALLUC:
			{
				inc_timed(TMD_IMAGE, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_AMNESIA:
			{
				inc_timed(TMD_AMNESIA, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_CURSE:
			{
				inc_timed(TMD_CURSED, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				break;
			}

			case DISEASE_SLOW:
			{
				inc_timed(TMD_SLOW, randint(p_ptr->disease & (DISEASE_POWER) ? 100 : 30) + 10, FALSE);
				/* Also slightly reduce agility. */
				dec_stat(A_AGI, p_ptr->disease & (DISEASE_POWER) ? randint(3) : 1);
				break;
			}

			case DISEASE_DISPEL:
			{
				/* Powerful diseases also drain mana */
				if (p_ptr->msp && (p_ptr->disease & (DISEASE_POWER)))
				{
					p_ptr->csp -= (s16b)randint(30);
					if (p_ptr->csp < 0) p_ptr->csp = 0;

					/* Update mana */
					p_ptr->update |= (PU_MANA);

					/* Redraw */
					p_ptr->redraw |= (PR_MANA);

					/* Window stuff */
					p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
				}

				(void)project_one(SOURCE_DISEASE, effect, p_ptr->py, p_ptr->px, 0, GF_DISPEL, (PROJECT_PLAY | PROJECT_HIDE));
				break;
			}

			case DISEASE_SLEEP:
			{
				inc_timed(TMD_MSLEEP, randint(p_ptr->disease & (DISEASE_POWER) ? 40 : 10) + 2, FALSE);
				break;
			}

			case DISEASE_PETRIFY:
			{
				inc_timed(TMD_PETRIFY, randint(p_ptr->disease & (DISEASE_POWER) ? 10 : 3) + 1, FALSE);
				break;
			}

			case DISEASE_PARALYZE:
			{
				inc_timed(TMD_PARALYZED, randint(p_ptr->disease & (DISEASE_POWER) ? 10 : 3) + 1, TRUE);
				break;
			}

			case DISEASE_STASTIS:
			{
				inc_timed(TMD_STASTIS, randint(p_ptr->disease & (DISEASE_POWER) ? 10 : 3) + 1, TRUE);
				break;
			}
		}

		/* The player is going to suffer further */
		if ((p_ptr->disease & (DISEASE_QUICK)) && !(rand_int(3)))
		{
			bool ecology_ready = cave_ecology.ready;

			/* Breakfast time... */
			msg_print("Something pushes through your skin.");
			msg_print("It's... hatching...");

			/* How many eggs? */
			n = randint(p_ptr->depth / 5) + 1;

			/* A nasty chest wound */
			take_hit(SOURCE_BIRTH, parasite_hack[effect], damroll(n, 8));

			/* Stop using ecology */
			cave_ecology.ready = FALSE;

			/* Set parasite race */
			summon_race_type = parasite_hack[effect];

			/* Drop lots of parasites */
			for (i = 0; i < n; i++) (void)summon_specific(p_ptr->py, p_ptr->py, 0, 99, SUMMON_FRIEND, FALSE, 0L);

			/* Start using ecology again */
			cave_ecology.ready = ecology_ready;

			/* Aggravate if not light */
			if (!(p_ptr->disease & (DISEASE_LIGHT))) aggravate_monsters(-1);

			/* Paralyze if heavy */
			if (p_ptr->disease & (DISEASE_HEAVY)) inc_timed(TMD_PARALYZED, randint(3) + 1, TRUE);

			/* Not a pleasant cure but nonetheless */
			p_ptr->disease &= (DISEASE_HEAVY | DISEASE_PERMANENT);
		}

		/* Mutate plague */
		if ((p_ptr->disease & (DISEASE_DISEASE)) && !(rand_int(3)))
		{
			if ((n < 3) && (p_ptr->disease & (DISEASE_HEAVY)))
				p_ptr->disease |= (1 << rand_int(DISEASE_TYPES_HEAVY));
			else if (n < 3)
				p_ptr->disease |= (1 << rand_int(DISEASE_TYPES));

			if (n > 1) p_ptr->disease &= ~(1 << rand_int(DISEASE_TYPES));
			if (!rand_int(20)) p_ptr->disease |= (DISEASE_LIGHT);
		}
	}

	/* All diseases mutate to get blows if they have no effect currently*/
	else if ((!rand_int(3)) && ((p_ptr->disease & ((1 << DISEASE_TYPES_HEAVY) -1 )) == 0))
	{
		if (p_ptr->disease & (DISEASE_HEAVY))
		{
			p_ptr->disease |= (1 << rand_int(DISEASE_TYPES_HEAVY));
		}
		else
		{
			p_ptr->disease |= (1 << rand_int(DISEASE_BLOWS));
		}
		if (!rand_int(20)) p_ptr->disease |= (DISEASE_LIGHT);
	}

	/* Worsen black breath */
	if ((p_ptr->disease & (1 << DISEASE_SPECIAL)) && !(rand_int(10)))
	{
		if (p_ptr->disease & (DISEASE_HEAVY))
			p_ptr->disease |= (1 << rand_int(DISEASE_TYPES_HEAVY));
		else
			p_ptr->disease |= (1 << rand_int(DISEASE_TYPES));
	}

	/* The player is on the mend */
	if ((allow_cure) && (p_ptr->disease & (DISEASE_LIGHT)) && !(rand_int(6)))
	{
		msg_print("The illness has subsided.");
		p_ptr->disease &= (DISEASE_HEAVY | DISEASE_PERMANENT);

		p_ptr->redraw |= (PR_DISEASE);

		if (disturb_state) disturb(0, 0);
	}

	/* Diseases? */
	else if (old_disease != p_ptr->disease)
	{
		char output[1024];

		disease_desc(output, sizeof(output), old_disease, p_ptr->disease);

		msg_print(output);

		p_ptr->redraw |= (PR_DISEASE);
	}
}



/*
 * Helper for process_world -- decrement p_ptr->timed[] fields.
 */
static void decrease_timeouts(void)
{
	int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);
	int i;

	/* Decrement all effects that can be done simply */
	for (i = 0; i < TMD_MAX; i++)
	{
		int decr = 1;
		if (!p_ptr->timed[i])
			continue;

		switch (i)
		{
			case TMD_CUT:
			{
				/* Hack -- check for truly "mortal" wound */
				decr = (p_ptr->timed[i] > 1000) ? 0 : adjust;

				/* Some rooms make wounds magically worse */
				if (room_has_flag(p_ptr->py, p_ptr->px, ROOM_BLOODY)) decr = -1;

				break;
			}

			case TMD_POISONED:
			{
				decr = p_ptr->timed[TMD_SLOW_POISON] ? 0 : adjust;

				/* Some rooms make wounds magically worse */
				if (room_has_flag(p_ptr->py, p_ptr->px, ROOM_BLOODY)) decr = -1;

				break;
			}
			case TMD_STUN:
			{
				decr = adjust;
				break;
			}

			case TMD_PSLEEP:
			{
				/* Goes in other direction */
				decr = -1;

				break;
			}
		}
		/* Decrement the effect */
		dec_timed(i, decr, FALSE);
	}

	return;
}


/*
 * Handle certain things once every 10 game turns
 */
static void process_world(void)
{
	int i, j, k;

	feature_type *f_ptr;

	int mimic;

	int regen_amount;

	object_type *o_ptr;

	cptr name;

	/* no shuffling for alien towns */
	town_type *t_ptr = &t_info[p_ptr->town];

	/* Process regions */
	process_regions();

	/* Every 10 game turns */
	if (turn % 10) return;

	/*** Check the Time and Load ***/

	if (!(turn % 1000))
	{
		/* Check time and load */
		if ((0 != check_time()) || (0 != check_load()))
		{
			/* Warning */
			if (closing_flag <= 2)
			{
				/* Disturb */
				disturb(0, 0);

				/* Count warnings */
				closing_flag++;

				/* Message */
				msg_print("The gates to ANGBAND are closing...");
				msg_print("Please finish up and/or save your game.");
			}

			/* Slam the gate */
			else
			{
				/* Message */
				msg_print("The gates to ANGBAND are now closed.");

				/* Stop playing */
				p_ptr->playing = FALSE;

				/* Leaving */
				p_ptr->leaving = TRUE;
			}
		}
	}

	/*** Handle the "town" (stores and sunshine) ***/

	/* While in town */
	if (level_flag & (LF1_SURFACE))
	{
		/* Hack -- Daybreak/Nighfall in town */
		if (!(turn % ((10L * TOWN_DAWN) / 2)))
		{
			bool dawn;

			/* Check for dawn */
			dawn = (!(turn % (10L * TOWN_DAWN)));

			/* Day breaks */
			if (dawn)
			{
				/* Message */
				msg_print("The sun has risen.");

			}

			/* Night falls */
			else
			{
				/* Message */
				msg_print("The sun has fallen.");
			}

			/* Illuminate */
			town_illuminate(dawn);
		}
	}


	/* While in the dungeon */
	else
	{
		/*** Update the Stores ***/

		/* Update the stores once a day (while in dungeon) */
		if (!(turn % (10L * STORE_TURNS)))
		{
		  int n, i;

			/* Message */
			if (cheat_xtra) msg_print("Updating Shops...");

			/* Maintain each shop */
			for (n = 0; n < total_store_count; n++)
			{
				/* no restocking for alien towns; at most 1 shop n in each town */
				town_type *t_ptr = &t_info[p_ptr->town];
				for (i = 0; i < MAX_STORES; i++)
				{
			      if (t_ptr->store_index[i] == n)
					{
						store_maint(n);
						break;
					}
				}
			}

			/* Sometimes, shuffle the shop-keepers */
			if ((total_store_count) && (rand_int(STORE_SHUFFLE) < 3))
			{
				/* Message */
				if (cheat_xtra) msg_print("Shuffling a Shopkeeper...");

				/* Pick a random shop (except home) */
				n = randint(total_store_count - 1);

				for (i = 0; i < MAX_STORES; i++)
				{
			      if (t_ptr->store_index[i] == n)
					{
						store_shuffle(n);
						break;
					}
				}
			}

			/* Message */
			if (cheat_xtra) msg_print("Done.");
		}
	}


	/*** Process the monsters ***/

	/* Create new monsters */
	if (p_ptr->depth)
	{
		/* Base odds against a new monster (200 to 1) */
		int odds = MAX_M_ALLOC_CHANCE;

		int max_m_cnt = (2 * p_ptr->depth / 3) + 20;

		/* Do not overpopulate the dungeon (important) */
		if (m_cnt > max_m_cnt) odds += 4 * (m_cnt - max_m_cnt);

		/* Fewer monsters if stealthy, more if deep */
		odds += p_ptr->skills[SKILL_STEALTH] * 2;
		odds -= p_ptr->depth / 2;

		/* Check for creature generation */
		if (!rand_int(odds))
		{
			bool slp = FALSE;

			/* Sneaky characters make monsters sleepy */
			if (p_ptr->skills[SKILL_STEALTH] > rand_int(100)) slp = TRUE;

			/* Make a new monster */
			(void)alloc_monster(MAX_SIGHT + 5, slp);
		}
	}

	/*** Stastis ***/
	if (p_ptr->timed[TMD_STASTIS])
	{
		(void)set_stastis(p_ptr->timed[TMD_STASTIS] - 1);

		/* Update dynamic terrain */
		update_dyna();

		/* Check quests */
		ensure_quest();

		return;
	}




	/*** Damage over Time ***/

	/* Get the feature */
	f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

	/* Get the mimiced feature */
	mimic = f_ptr->mimic;

	/* Use covered if necessary */
	if (f_ptr->flags2 & (FF2_COVERED))
	{
		f_ptr = &(f_info[f_ptr->mimic]);
	}

	/* Take damage from features */
	if (!(f_ptr->flags1 & (FF1_HIT_TRAP)) &&
	    ((f_ptr->blow.method) || (f_ptr->spell)))
	{
		/* Damage from terrain */
		hit_trap(p_ptr->py,p_ptr->px);
	}

	/* If paralyzed, we drown in shallow, deep or filled */
	if ((p_ptr->timed[TMD_PARALYZED] || (p_ptr->timed[TMD_STUN] >=100) || (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP)) &&
		(f_ptr->flags2 & (FF2_DEEP | FF2_SHALLOW | FF2_FILLED)))
	{
		/* Get the mimiced feature */
		mimic = f_ptr->mimic;

		/* Get the feature name */
		name = (f_name + f_info[mimic].name);

		msg_format("You are drowning %s%s!",(f_ptr->flags2 & (FF2_FILLED)?"":"in the "),name);

		/* Apply the blow */
		project_one(SOURCE_FEATURE, mimic, p_ptr->py, p_ptr->px, damroll(4,6), GF_WATER, (PROJECT_PLAY | PROJECT_HIDE));
	}

	/* Take damage from poison */
	if ((p_ptr->timed[TMD_POISONED]) && (!p_ptr->timed[TMD_SLOW_POISON]))
	{
		/* Take damage */
		take_hit(SOURCE_POISON, 0, 1);
	}

	/* Take damage from cuts */
	if (p_ptr->timed[TMD_CUT])
	{
		/* Mortal wound or Deep Gash */
		if (p_ptr->timed[TMD_CUT] > 200)
		{
			i = 3;
		}

		/* Severe cut */
		else if (p_ptr->timed[TMD_CUT] > 100)
		{
			i = 2;
		}

		/* Other cuts */
		else
		{
			i = 1;
		}

		/* Take damage */
		take_hit(SOURCE_CUTS, i, i);
	}

	/*** Check the Food, Rest, and Regenerate ***/

	/* Tire normally */
	/* XXX We exclude situations where we adjust this counter elsewhere */
	if (!(p_ptr->resting || p_ptr->searching || p_ptr->running || p_ptr->timed[TMD_PARALYZED] || p_ptr->timed[TMD_STASTIS] || (p_ptr->timed[TMD_STUN] >= 100)) ||
			(f_ptr->flags2 & (FF2_FILLED)))
	{
		(void)set_rest(p_ptr->rest - p_ptr->tiring);
	}

	/* Digest normally */
	if (p_ptr->food < PY_FOOD_MAX)
	{
		/* Every 100 game turns */
		if (!(turn % 100))
		{
			/* Basic digestion rate based on speed */
			i = extract_energy[p_ptr->pspeed] * 2;

			/* Hunger takes more food */
			if ((p_ptr->cur_flags3 & (TR3_HUNGER)) != 0) i += 100;

			/* Regeneration takes more food */
			if (p_ptr->regen_hp > 0) i += 15 * p_ptr->regen_hp;

			/* Regeneration takes more food */
			if (p_ptr->regen_mana > 0) i += 15 * p_ptr->regen_mana;

			/* Slow digestion takes less food */
			if ((p_ptr->timed[TMD_SLOW_DIGEST]) || (p_ptr->cur_flags3 & (TR3_SLOW_DIGEST)) != 0) i -= 10;

			/* Minimal digestion */
			if (i < 1) i = 1;

			/* Digest some food */
			(void)set_food(p_ptr->food - i);
		}
	}

	/* Digest quickly when gorged */
	else
	{
		/* Digest a lot of food */
		(void)set_food(p_ptr->food - 100);
	}

	/* Starve to death (slowly) */
	if (p_ptr->food < PY_FOOD_STARVE)
	{
		/* Calculate damage */
		i = (PY_FOOD_STARVE - p_ptr->food) / 10;

		/* Take damage */
		take_hit(SOURCE_HUNGER, 0, i);
	}

	/* Default regeneration */
	regen_amount = PY_REGEN_NORMAL;

	/* Getting Weak */
	if ((p_ptr->food < PY_FOOD_WEAK) || (p_ptr->rest < PY_REST_WEAK))
	{
		/* Lower regeneration */
		if (p_ptr->food < PY_FOOD_STARVE)
		{
			regen_amount = 0;
		}
		else if (p_ptr->food < PY_FOOD_FAINT)
		{
			regen_amount = PY_REGEN_FAINT;
		}
		else
		{
			regen_amount = PY_REGEN_WEAK;
		}

		/* Getting Faint */
		if (p_ptr->food < PY_FOOD_FAINT)
		{
			/* Faint occasionally */
			if (!p_ptr->timed[TMD_PARALYZED] && (rand_int(100) < 10))
			{
				/* Message */
				msg_print("You faint from the lack of food.");
				disturb(1, 0);

				/* Hack -- faint (bypass free action) */
				inc_timed(TMD_PARALYZED, 1 + rand_int(5), TRUE);
			}
		}

		/* Getting Faint - lack of rest */
		if (p_ptr->rest < PY_REST_FAINT)
		{
			/* Faint occasionally */
			if (!p_ptr->timed[TMD_PARALYZED] && (rand_int(100) < 10))
			{
				/* Message */
				msg_print("You faint from exhaustion.");
				disturb(1, 0);

				/* Hack -- faint (bypass free action) */
				inc_timed(TMD_PARALYZED, 1 + rand_int(5), TRUE);
			}
		}
	}

	/* Searching or Resting */
	if (p_ptr->searching || p_ptr->resting)
	{
		regen_amount = regen_amount * 2;
	}

	/* Regenerate the mana */
	if (p_ptr->csp < p_ptr->msp)
	{
		if (!(p_ptr->cur_flags3 & (TR3_DRAIN_MANA)) &&
			!(p_ptr->disease & (DISEASE_DRAIN_MANA))
			&& (p_ptr->regen_mana >= 0)) regenmana(regen_amount * (p_ptr->regen_mana + 1));
	}

	/* Various things interfere with healing */
	if (p_ptr->timed[TMD_PARALYZED]) regen_amount = 0;
	if ((p_ptr->timed[TMD_POISONED])  && (!p_ptr->timed[TMD_SLOW_POISON])) regen_amount = 0;
	if (p_ptr->timed[TMD_STUN]) regen_amount = 0;
	if (p_ptr->timed[TMD_CUT]) regen_amount = 0;
	if (p_ptr->disease & (DISEASE_DRAIN_HP)) regen_amount = 0;
	if ((p_ptr->cur_flags3 & (TR3_DRAIN_HP)) != 0) regen_amount = 0;
	if (room_has_flag(p_ptr->py, p_ptr->px, ROOM_BLOODY)) regen_amount = 0;

	/* Regenerate Hit Points if needed */
	if ((p_ptr->chp < p_ptr->mhp) && (p_ptr->regen_hp >= 0))
	{
		regenhp(regen_amount * (p_ptr->regen_hp + 1));
	}

	/*** Timeout Various Things ***/
	decrease_timeouts();

	/*** Process Light ***/
	/* Check for light being wielded */
	o_ptr = &inventory[INVEN_LITE];

	/* Calculate torch radius */
	p_ptr->update |= (PU_TORCH);

	/*** Process Inventory ***/

	/* Handle experience draining */
	if (((p_ptr->cur_flags3 & (TR3_DRAIN_EXP)) != 0) || (p_ptr->disease & (DISEASE_DRAIN_EXP)))
	{
		if ((rand_int(100) < 10) && (p_ptr->exp > 0))
		{
			/* Always notice */
			if (!(p_ptr->disease & (DISEASE_DRAIN_EXP))) equip_can_flags(0x0L,0x0L,TR3_DRAIN_EXP,0x0L);

			p_ptr->exp--;
			p_ptr->max_exp--;
			if (p_ptr->exp <= 0) p_ptr->exp = 0;

			check_experience();
		}
	}

	/* Handle hit point draining */
	if (((p_ptr->cur_flags3 & (TR3_DRAIN_HP)) != 0) || (p_ptr->disease & (DISEASE_DRAIN_HP)))
	{
		if ((rand_int(100) < 10) && (p_ptr->chp > 0))
		{
			/* Always notice */
			if (!(p_ptr->disease & (DISEASE_DRAIN_HP))) equip_can_flags(0x0L,0x0L,TR3_DRAIN_HP,0x0L);

			if (p_ptr->disease & (DISEASE_DRAIN_HP))
				take_hit(SOURCE_DISEASE, DISEASE_DRAINING, 1);
			else
			{
				take_hit(SOURCE_CURSED_ITEM, 0, 1);
			}
		}
	}

	/* Handle mana draining */
	if (((p_ptr->cur_flags3 & (TR3_DRAIN_MANA)) != 0) || (p_ptr->disease & (DISEASE_DRAIN_MANA)))
	{
		if ((rand_int(100) < 10) && (p_ptr->csp > 0))
		{
			/* Always notice */
			if (!(p_ptr->disease & (DISEASE_DRAIN_MANA))) equip_can_flags(0x0L,0x0L,TR3_DRAIN_MANA,0x0L);

			p_ptr->csp--;
			p_ptr->csp_frac = 0;
			if (p_ptr->exp <= 0) p_ptr->csp = 0;

			/* Update mana */
			p_ptr->update |= (PU_MANA);

			/* Redraw */
			p_ptr->redraw |= (PR_MANA);
		}
	}

	/* Process timeouts */
	for (k = 0, j = 0, i = 0; i < INVEN_TOTAL; i++)
	{
		/* Get the object */
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Recharge activateable objects/rods */
		/* Also check for mimics/regenerating bodies */
		if (o_ptr->timeout > 0)
		{
			/* Recharge */
			o_ptr->timeout--;

			/* Notice changes */
			/* Now if 3 rods charging and timeout == 2 => 1 rod has recharged */
			if (!(o_ptr->timeout) || ((o_ptr->stackc) ? (o_ptr->timeout < o_ptr->stackc) :
					(o_ptr->timeout < o_ptr->number) ))
			{
				char o_name[80];

				u32b f1, f2, f3, f4;

				/* Get the flags */
				object_flags(o_ptr,&f1, &f2, &f3, &f4);

				/* Get a description */
				object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

				/* Hack -- lites */
				if (o_ptr->tval == TV_LITE) my_strcpy(o_name,"light",sizeof(o_name));

				/* Hack -- update torch radius */
				if (i == INVEN_LITE) p_ptr->update |= (PU_TORCH);

				/* Lanterns run dry */
				if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
				{
					disturb(0, 0);
					msg_print("Your light has gone out!");
				}

				/* Torches / Spells run out */
				else if ((o_ptr->tval == TV_SPELL) || ((o_ptr->tval == TV_LITE) && !(artifact_p(o_ptr))))
				{
					/* Disturb */
					disturb(0, 0);

					/* Notice things */
					if (i < INVEN_PACK) j++;
					else k++;

					/* Message */
					if (o_ptr->timeout) msg_format("One of your %s has run out.",o_name);
					else msg_format("Your %s %s run out.", o_name,
					   (o_ptr->number == 1) ? "has" : "have");

					/* Destroy a spell if discharged */
					if (o_ptr->timeout)
					{
						if (o_ptr->number == 1) inven_drop_flags(o_ptr);

						inven_item_increase(i, -1);
					}
					else if (o_ptr->stackc)
					{
						if (o_ptr->number == o_ptr->stackc) inven_drop_flags(o_ptr);

						inven_item_increase(i, -(o_ptr->stackc));
					}
					else
					{
						inven_drop_flags(o_ptr);
						inven_item_increase(i,-(o_ptr->number));
					}

					inven_item_optimize(i);

				}

				/* Rods and activatible items charge */
				else if ((o_ptr->tval == TV_ROD) || (f3 & (TR3_ACTIVATE)))
				{
					/* Notice things */
					if (i < INVEN_PACK) j++;
					else k++;

					/* Message */
					if (o_ptr->timeout) msg_format("One of your %s has charged.",o_name);
					else msg_format("Your %s %s charged.", o_name,
					   (o_ptr->number == 1) ? "has" : "have");

					/* Reset stack */
					if ((o_ptr->timeout) && !(o_ptr->stackc)) o_ptr->stackc = o_ptr->number -1;
					else if (o_ptr->timeout) o_ptr->stackc--;
					else o_ptr->stackc = 0;

				}

				/* Bodies/mimics become a monster */
				else
				{
					/* Notice things */
					if (animate_object(i))
					{
						if (i < INVEN_PACK) j++;
						else k++;
					}
				}
			}

			/* The lantern / torch / spell is running low */
			else if ((o_ptr->timeout < 100) && (!(o_ptr->timeout % 10)) &&
				((o_ptr->tval == TV_SPELL) || (o_ptr->tval == TV_LITE)) &&
				!(artifact_p(o_ptr)))
			{
				char o_name[80];

				/* Get a description */
				object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

				disturb(0, 0);

				if (o_ptr->tval == TV_SPELL) msg_format("Your %s is running out.", o_name);
				else if (o_ptr->sval == SV_LITE_LANTERN) msg_print("Your light is growing faint.");
				else msg_format("Your %s flame dims.", o_name);

				/* Hack -- update torch radius */
				if (i == INVEN_LITE) p_ptr->update |= (PU_TORCH);
			}
		}
	}

	/* Notice changes -- equip */
	if (k)
	{
		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Disturb */
		disturb(1, 0);
	}

	/* Notice changes - inventory */
	if (j)
	{
		/* Combine pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN);

		/* Disturb */
		disturb(1,0);
	}

	/* Feel the inventory */
	sense_inventory();

	/* Show tips */
	if  (((turn < old_turn + 1000) && !(turn % 100)) ||
		(!p_ptr->command_rep
		 && ((p_ptr->searching && !(turn % 1000))
			 || (is_typical_town(p_ptr->dungeon, p_ptr->depth)
				 && !(turn % 100)))))
	{
		/* Show a tip */
		show_tip();
	}

	/*** Process Objects ***/

	/* Process objects */
	for (i = 1; i < o_max; i++)
	{
		/* Get the object */
		o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Timing out? */
		if (o_ptr->timeout > 0)
		{
			/* Check for light extinguishing */
			bool extinguish = ((o_ptr->timeout == 1) && check_object_lite(o_ptr));

			/* Recharge */
			o_ptr->timeout--;

			/* Extinguish lite */
			if ((extinguish) && (o_ptr->iy) && (o_ptr->ix))
			{
				/* Check for loss of light */
				check_attribute_lost(o_ptr->iy, o_ptr->ix, 2, CAVE_XLOS, require_halo, has_halo, redraw_halo_loss, remove_halo, reapply_halo);
			}

			/* Notice changes */
			if (!(o_ptr->timeout) || ((o_ptr->stackc) ? (o_ptr->timeout < o_ptr->stackc) :
				(o_ptr->timeout < o_ptr->number) ))
			{
				u32b f1, f2, f3, f4;

				/* Get the flags */
				object_flags(o_ptr,&f1, &f2, &f3, &f4);

				/* Spells run out */
				if (o_ptr->tval == TV_SPELL)
				{
					/* Destroy a spell if discharged */
					if (o_ptr->timeout) floor_item_increase(i, -1);
					else if (o_ptr->stackc) floor_item_increase(i, -(o_ptr->stackc));
					else
					{
						/* Was a terrain counter - note paranoia */
						if ((o_ptr->ident & (IDENT_STORE)) && (o_ptr->pval > 0) && !(o_ptr->held_m_idx))
						{
							/* Message */
							if (play_info[o_ptr->iy][o_ptr->ix] & (PLAY_MARK))
							{
								msg_format("The %s fades.", f_name + f_info[cave_feat[o_ptr->iy][o_ptr->ix]].name);
							}

							/* Revert to old feature */
							cave_set_feat(o_ptr->iy, o_ptr->ix, o_ptr->pval);
						}

						/* Destroy remaining spells */
						floor_item_increase(i,-(o_ptr->number));
					}

					floor_item_optimize(i);
				}

				/* Rods and activatible items charge */
				else if ((o_ptr->tval == TV_ROD) || (f3 & (TR3_ACTIVATE)))
				{
					/* Reset stack */
					if ((o_ptr->timeout) && !(o_ptr->stackc)) o_ptr->stackc = o_ptr->number -1;
					else if (o_ptr->timeout) o_ptr->stackc--;
					else o_ptr->stackc = 0;

				}

				/* Bodies/mimics become a monster */
				else
				{
					/* Notice count */
					j++;
				}
			}
		}

	}

	/*** Handle disease ***/
	if ((p_ptr->disease) && (p_ptr->disease & (DISEASE_QUICK)) && !(rand_int(100)))
	{
		suffer_disease(TRUE);
	}


	/*** Involuntary Movement/Activations ***/

	/* Uncontrolled items */
	if ((p_ptr->uncontrolled) && (rand_int(UNCONTROLLED_CHANCE) < 1))
	{
		int j = 0, k = 0;

		/* Scan inventory and pick uncontrolled item */
		for (i = INVEN_WIELD; i < END_EQUIPMENT; i++)
		{
			u32b f1, f2, f3, f4;

			o_ptr = &inventory[i];

			/* Skip non-objects */
			if (!o_ptr->k_idx) continue;

			object_flags(o_ptr, &f1, &f2, &f3, &f4);

			/* Pick item */
			if (((f3 & (TR3_UNCONTROLLED)) != 0) && (uncontrolled_p(o_ptr)) && !(rand_int(k++)))
			{
				j = i;
			}
		}

		/* Apply uncontrolled power */
		if (j)
		{
			bool dummy = FALSE;

			o_ptr = &inventory[j];

			/* Get power */
			get_spell(&k, "", &inventory[j], FALSE);

			/* Uncontrolled activation */
			if (rand_int(UNCONTROLLED_CONTROL) < (UNCONTROLLED_CONTROL - o_ptr->usage))
			{
				/* Process spell - involuntary effects */
				process_spell(SOURCE_OBJECT, o_ptr->k_idx, k, 25, &dummy, &dummy, 0);

				/* Warn the player */
				sound(MSG_CURSED);

				/* Curse the object */
				o_ptr->ident |= (IDENT_CURSED);
				mark_cursed_feeling(o_ptr);
			}
			else if (o_ptr->usage < UNCONTROLLED_CONTROL)
			{
				/* Message only - every 5 attempts */
				if (!(o_ptr->usage % (UNCONTROLLED_CONTROL / 10))) msg_print("You feel yourself gain a measure of control.");
			}
			else if (o_ptr->usage >= UNCONTROLLED_CONTROL)
			{
				char o_name[80];

				/* Uncurse the object */
				uncurse_object(o_ptr);

				/* Get a description */
				object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

				/* Message only */
				msg_print("Congratulations.");

				/* Describe victory */
				msg_format("You have mastered the %s.", o_name);

				/* Reveal functionality */
				msg_print("You may now activate it when you wish.");
			}

			/* Used the object */
			if (o_ptr->usage < MAX_SHORT) o_ptr->usage++;
		}

		/* Always notice */
		equip_can_flags(0x0L,0x0L,TR3_UNCONTROLLED,0x0L);
	}


	/* Mega-Hack -- Portal room */
	if ((room_has_flag(p_ptr->py, p_ptr->px, ROOM_PORTAL))
			&& ((p_ptr->cur_flags4 & (TR4_ANCHOR)) == 0) && (rand_int(100)<1))
	{
		/* Warn the player */
		msg_print("There is a brilliant flash of light.");

		/* Teleport player */
		teleport_player(40);
	}

	/* Delayed Word-of-Return */
	if (p_ptr->word_return)
	{
		/* Count down towards return */
		p_ptr->word_return--;

		/* Activate the return */
		if (!p_ptr->word_return)
		{
			/* Disturbing! */
			disturb(0, 0);

			msg_print("You feel yourself yanked sideways!");

			/* Teleport the player back to their original location */
			teleport_player_to(p_ptr->return_y, p_ptr->return_x);

			/* Clear the return coordinates */
			p_ptr->return_y = 0;
			p_ptr->return_x = 0;
		}
	}

	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall)
	{
		/* Count down towards recall */
		p_ptr->word_recall--;

		/* Activate the recall */
		if (!p_ptr->word_recall)
		{
			/* Disturbing! */
			disturb(0, 0);

			/* Sound */
			sound(MSG_TPLEVEL);

			/* Determine the level */
			if (p_ptr->depth > min_depth(p_ptr->dungeon))
			{
				msg_format("You feel yourself yanked %swards!",
					((level_flag & (LF1_TOWER)) ? "down" : "up"));

				/* New depth */
				p_ptr->depth = min_depth(p_ptr->dungeon);

				/* Leaving */
				p_ptr->leaving = TRUE;

				/* No stairs under the player */
				p_ptr->create_stair = 0;
			}
			else
			{
				msg_format("You feel yourself yanked %swards!",
					((level_flag & (LF1_TOWER)) ? "up" : "down"));

				/* New depth */
				p_ptr->depth = t_info[p_ptr->dungeon].attained_depth;
				/* Repair, in case of old savefile */
				if (p_ptr->depth <= min_depth(p_ptr->dungeon)) p_ptr->depth = min_depth(p_ptr->dungeon)+1;
				if (p_ptr->depth > max_depth(p_ptr->dungeon)) p_ptr->depth = max_depth(p_ptr->dungeon);

				/* Leaving */
				p_ptr->leaving = TRUE;

				/* No stairs under the player */
				p_ptr->create_stair = 0;
			}
		}
	}

	/* Update dynamic terrain */
	update_dyna();

	/* Check quests */
	ensure_quest();

#ifdef ALLOW_BORG
	if (count_stop)
	{
		/* The borg is always in perfect health */

		/* Restore stats */
		(void)res_stat(A_STR);
		(void)res_stat(A_INT);
		(void)res_stat(A_WIS);
		(void)res_stat(A_DEX);
		(void)res_stat(A_CON);
		(void)res_stat(A_CHR);
		(void)res_stat(A_AGI);
		(void)res_stat(A_SIZ);

		/* Restore experience. */
		p_ptr->exp = p_ptr->max_exp;

		/* No maladies */
		for (i = 0; i < TMD_MAX; i++)
		{
			p_ptr->timed[i] = 0;
		}

		/* Fully healed */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;

		/* No longer hungry */
		p_ptr->food = PY_FOOD_MAX - 1;

		/* No longer tired */
		p_ptr->rest = PY_REST_MAX - 1;
	}
#endif
}


/*
 * Verify use of "wizard" mode
 */
static bool enter_wizard_mode(void)
{
	/* Ask first time */
	if (!(p_ptr->noscore & 0x0002))
	{
		/* Mention effects */
		msg_print("You are about to enter 'wizard' mode for the very first time!");
		msg_print("This is a form of cheating, and your game will not be scored!");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to enter wizard mode? "))
		{
			return (FALSE);
		}
	}

	/* Mark savefile */
	p_ptr->noscore |= 0x0002;

	/* Success */
	return (TRUE);
}



#ifdef ALLOW_DEBUG

/*
 * Verify use of "debug" mode
 */
static bool verify_debug_mode(void)
{
	/* Ask first time */
	if (!(p_ptr->noscore & 0x0008))
	{
		/* Mention effects */
		msg_print("You are about to use the dangerous, unsupported, debug commands!");
		msg_print("Your machine may crash, and your savefile may become corrupted!");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to use the debug commands? "))
		{
			return (FALSE);
		}
	}

	/* Mark savefile */
	p_ptr->noscore |= 0x0008;

	/* Okay */
	return (TRUE);
}


/*
 * Hack -- Declare the Debug Routines
 */
extern void do_cmd_debug(void);

#endif



#ifdef ALLOW_BORG

/*
 * Verify use of "borg" mode
 */
static bool verify_borg_mode(void)
{
	/* Ask first time */
	if (!(p_ptr->noscore & 0x0010))
	{
		/* Mention effects */
		msg_print("You are about to use the dangerous, unsupported, borg commands!");
		msg_print("Your machine may crash, and your savefile may become corrupted!");
		msg_print(NULL);

		/* Verify request */
		if (!get_check("Are you sure you want to use the borg commands? "))
		{
			return (FALSE);
		}
	}

	/* Mark savefile */
	p_ptr->noscore |= 0x0010;

	/* Okay */
	return (TRUE);
}


/*
 * Hack -- Declare the Borg Routines
 */
extern void do_cmd_borg(void);

#endif




/*
 * Parse and execute the current command
 * Give "Warning" on illegal commands.
 */
static void process_command(void)
{

#ifdef ALLOW_REPEAT

	/* Handle repeating the last command */
	repeat_check();

#endif /* ALLOW_REPEAT */

	/* Parse the command */
	switch (p_ptr->command_cmd)
	{
		/* Ignore */
		case ESCAPE:
		{
			break;
		}


		/*** Cheating Commands ***/

		/* Toggle Wizard Mode */
		case KTRL('W'):
		{
			if (p_ptr->wizard)
			{
				p_ptr->wizard = FALSE;
				msg_print("Wizard mode off.");
			}
			else if (enter_wizard_mode())
			{
				p_ptr->wizard = TRUE;
				msg_print("Wizard mode on.");
			}

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Redraw "title" */
			p_ptr->redraw |= (PR_TITLE);

			break;
		}


#ifdef ALLOW_DEBUG

		/* Special "debug" commands */
		case KTRL('A'):
		{
			if (verify_debug_mode()) do_cmd_debug();
			break;
		}

#endif


#ifdef ALLOW_BORG

		/* Special "borg" commands */
		case KTRL('Z'):
		{
			if (verify_borg_mode()) do_cmd_borg();
			break;
		}

#endif



		/*** Inventory Commands ***/

		/* Handle something - this is the generic do anything to an object command */
		case 'h':
		{
			do_cmd_item(COMMAND_ITEM_HANDLE);
			break;
		}
		
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

		/* Drop an item */
		case 'd':
		{
			do_cmd_item(COMMAND_ITEM_DROP);
			break;
		}

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

		/* Offer an item */
		case 'O':
		{
			do_cmd_item(COMMAND_ITEM_OFFER);
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


		/*** Standard "Movement" Commands ***/

		/* Alter a grid */
		case '+':
		{
			do_cmd_alter();
			break;
		}

		/* Dig a tunnel */
		case 'T':
		{
			do_cmd_tunnel();
			break;
		}

		/* Walk */
		case ';':
		{
			do_cmd_walk();
			break;
		}

		/*** Running, Resting, Searching, Pickup */

		/* Begin Running -- Arg is Max Distance */
		case '.':
		{
			do_cmd_run();
			break;
		}

		/* Hold still */
		case ',':
		{
			do_cmd_hold();
			break;
		}

		/* Pickup */
		case 'g':
		{
			do_cmd_pickup();
			break;
		}

		/* Rest -- Arg is time */
		case 'R':
		{
			do_cmd_rest();
			break;
		}

		/* Search for traps/doors/ steal from monsters */
		case 's':
		{
			do_cmd_search_or_steal();
			break;
		}

		/* Toggle search mode */
		case 'S':
		{
			do_cmd_toggle_search();
			break;
		}


		/*** Stairs and Doors and Chests and Traps ***/

		/* Enter store */
		case '_':
		{
			do_cmd_store();
			break;
		}

		/* Go up staircase */
		case '<':
		{
			do_cmd_go_up();
			break;
		}

		/* Go down staircase */
		case '>':
		{
			do_cmd_go_down();
			break;
		}

		/* Open a door or chest */
		case 'o':
		{
			do_cmd_open();
			break;
		}

		/* Close a door */
		case 'c':
		{
			do_cmd_close();
			break;
		}

		/* Jam a door with spikes */
		case 'j':
		{
			do_cmd_item(COMMAND_ITEM_SET_TRAP_OR_SPIKE);
			break;
		}

		/* Bash a door */
		case 'B':
		{
			do_cmd_bash();
			break;
		}

		/* Disarm a trap or chest */
		case 'D':
		{
			do_cmd_disarm();
			break;
		}


		/*** Magic and Prayers ***/

		/* Gain new spells/prayers */
		case 'G':
		{
			do_cmd_item(COMMAND_ITEM_STUDY);
			break;
		}

		/* Browse a book */
		case 'b':
		{
			do_cmd_item(COMMAND_ITEM_BROWSE);
			break;
		}

		/* Cast a spell */
		case 'm':
		{
			do_cmd_item(COMMAND_ITEM_CAST_SPELL);
			break;
		}

		/*** Use various objects ***/

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

		/* Activate an artifact */
		case 'A':
		{
			do_cmd_item(COMMAND_ITEM_ACTIVATE);
			break;
		}

		/* Eat some food */
		case 'E':
		{
			do_cmd_item(COMMAND_ITEM_EAT);
			break;
		}

		/* Fuel your lantern/torch */
		case 'F':
		{
			do_cmd_item(COMMAND_ITEM_FUEL);
			break;
		}

		/* Fire an item */
		case 'f':
		{
			do_cmd_item(p_ptr->fire_command);
			break;
		}

		/* Throw an item */
		case 'v':
		{
			do_cmd_item(COMMAND_ITEM_THROW);
			break;
		}

		/* Aim a wand */
		case 'a':
		{
			do_cmd_item(COMMAND_ITEM_AIM);
			break;
		}

		/* Zap a rod */
		case 'z':
		{
			do_cmd_item(COMMAND_ITEM_ZAP);
			break;
		}

		/* Quaff a potion */
		case 'q':
		{
			do_cmd_item(COMMAND_ITEM_QUAFF);
			break;
		}

		/* Read a scroll */
		case 'r':
		{
			do_cmd_item(COMMAND_ITEM_READ);
			break;
		}

		/* Use a staff */
		case 'u':
		{
			do_cmd_item(COMMAND_ITEM_USE);
			break;
		}

		/* Apply a rune */
		case 'y':
		{
			do_cmd_item(COMMAND_ITEM_APPLY);
			break;
		}

		/* Assemble a mechanism */
		case 'Y':
		{
			do_cmd_item(COMMAND_ITEM_ASSEMBLE);
			break;
		}

		/* Light or douse a light source */
		case '|':
		{
			do_cmd_item(COMMAND_ITEM_LITE);
			break;
		}

		/*** Looking at Things (nearby or on map) ***/

		/* Full dungeon map */
		case 'M':
		{
			do_cmd_view_map();
			break;
		}

		/* Locate player on map */
		case 'L':
		{
			do_cmd_locate();
			break;
		}

		/* Center map on player */
		case KTRL('L'):
		{
			do_cmd_center_map();
			break;
		}

		/* Look around */
		case 'l':
		{
			do_cmd_look();
			break;
		}

		/* Target monster or location */
		case '*':
		{
			do_cmd_target();
			break;
		}

		/* Show visible monster list */
		case '[':
		{
			do_cmd_monlist();
			break;
		}



		/*** Help and Such ***/

		/* Help */
		case '\r':
		case '\n':
		{
			do_cmd_quick_help();
			break;
		}

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

		/* Interact with options */
		case '=':
		{
			do_cmd_menu(MENU_OPTIONS, "options");
			do_cmd_redraw();
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

		/* List quests */
		case KTRL('Q'):
		{
			do_cmd_quest();
			break;
		}

		/* Repeat level feeling */
		case KTRL('F'):
		{
			do_cmd_feeling();
			break;
		}

		/* Repeat time of day */
		case KTRL('T'):
		{
			do_cmd_timeofday();
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

		/* Redraw the screen */
		case KTRL('R'):
		{
			do_cmd_redraw();
			break;
		}

#ifndef VERIFY_SAVEFILE

		/* Hack -- Save and don't quit */
		case KTRL('S'):
		{
			do_cmd_save_game();
			break;
		}

#endif

		/* Save and quit */
		case KTRL('X'):
		{
			/* Stop playing */
			p_ptr->playing = FALSE;

			/* Leaving */
			p_ptr->leaving = TRUE;

			break;
		}

		/* Quit (commit suicide) */
		case 'Q':
		{
			do_cmd_suicide();
			break;
		}

		/* Check knowledge */
		case '~':
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

		/* Save "html screen dump" */
		case ']':
		{
			do_cmd_save_screen_html();
			break;
		}

		/* Save "toggle monster list on or off" */
		case ' ':
		{
			if (easy_monlist)
			{
				auto_monlist = !auto_monlist;
			}

			break;
		}

		/* Mouse interaction */
		case '\xff':
		{
			int y = KEY_GRID_Y(p_ptr->command_cmd_ex);
			int x = KEY_GRID_X(p_ptr->command_cmd_ex);
			int room;

			/* Paranoia */
			if (!in_bounds_fully(y, x)) break;

			/* Hack -- we could try various things here like travelling or going up/down stairs */
			if ((p_ptr->py == y) && (p_ptr->px == x) && (p_ptr->command_cmd_ex.mousebutton))
			{
				do_cmd_rest();
			}
			else if (p_ptr->command_cmd_ex.mousebutton == BUTTON_MOVE)
			{
				if (p_ptr->timed[TMD_CONFUSED])
				{
					do_cmd_walk();
				}
				else
				{
					do_cmd_pathfind(y, x);
				}
			}
			else if (p_ptr->command_cmd_ex.mousebutton == BUTTON_AIM)
			{
				target_set_location(y, x, 0);
				msg_print("Target set.");
			}
			else if (use_trackmouse && (easy_more || (auto_more && !easy_more)))
			{
				p_ptr->command_new = target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
			}
			break;
		}

		/* Hack -- Unknown command */
		default:
		{
			prt("Type '?' or press ENTER for help.", 0, 0);
			break;
		}
	}

	/* If there is a transitive component to the command, process it */
	if (p_ptr->command_trans)
	{
		const do_cmd_item_type *cmd = &cmd_item_list[p_ptr->command_trans];

		int cmd_next = cmd->next_command;

		/* Evaluate which command to do */
		if (cmd->next_command_eval)
		{
			/* Determine new command based on item selected */
			cmd_next = cmd->next_command_eval(p_ptr->command_trans_item);
		}

		/* Process next command */
		do_cmd_item(cmd_next);

		/* Processed */
		p_ptr->command_trans = 0;
	}
}



/*
 * Hack -- helper function for "process_player()"
 *
 * Check for changes in the "monster memory"
 */
static void process_player_aux(void)
{
	static int old_monster_race_idx = 0;

	static u32b     old_flags1 = 0L;
	static u32b     old_flags2 = 0L;
	static u32b     old_flags3 = 0L;
	static u32b     old_flags4 = 0L;
	static u32b     old_flags5 = 0L;
	static u32b     old_flags6 = 0L;
	static u32b     old_flags7 = 0L;
	static u32b     old_flags8 = 0L;
	static u32b     old_flags9 = 0L;

	static byte	old_r_blows0 = 0;
	static byte	old_r_blows1 = 0;
	static byte	old_r_blows2 = 0;
	static byte	old_r_blows3 = 0;

	static byte     old_r_cast_innate = 0;
	static byte	old_r_cast_spell = 0;


	/* Tracking a monster */
	if (p_ptr->monster_race_idx)
	{
		/* Get the monster lore */
		monster_lore *l_ptr = &l_list[p_ptr->monster_race_idx];

		/* Check for change of any kind */
		if ((old_monster_race_idx != p_ptr->monster_race_idx) ||
		    (old_flags1 != l_ptr->flags1) ||
		    (old_flags2 != l_ptr->flags2) ||
		    (old_flags3 != l_ptr->flags3) ||
		    (old_flags4 != l_ptr->flags4) ||
		    (old_flags5 != l_ptr->flags5) ||
		    (old_flags6 != l_ptr->flags6) ||
		    (old_flags7 != l_ptr->flags7) ||
		    (old_flags8 != l_ptr->flags8) ||
		    (old_flags9 != l_ptr->flags9) ||
		    (old_r_blows0 != l_ptr->blows[0]) ||
		    (old_r_blows1 != l_ptr->blows[1]) ||
		    (old_r_blows2 != l_ptr->blows[2]) ||
		    (old_r_blows3 != l_ptr->blows[3]) ||
		    (old_r_cast_innate != l_ptr->cast_innate) ||
		    (old_r_cast_spell != l_ptr->cast_spell))
		{
			/* Memorize old race */
			old_monster_race_idx = p_ptr->monster_race_idx;

			/* Memorize flags */
			old_flags1 = l_ptr->flags1;
			old_flags2 = l_ptr->flags2;
			old_flags3 = l_ptr->flags3;
			old_flags4 = l_ptr->flags4;
			old_flags5 = l_ptr->flags5;
			old_flags6 = l_ptr->flags6;
			old_flags6 = l_ptr->flags7;
			old_flags6 = l_ptr->flags8;
			old_flags6 = l_ptr->flags9;

			/* Memorize blows */
			old_r_blows0 = l_ptr->blows[0];
			old_r_blows1 = l_ptr->blows[1];
			old_r_blows2 = l_ptr->blows[2];
			old_r_blows3 = l_ptr->blows[3];

			/* Memorize castings */
			old_r_cast_innate = l_ptr->cast_innate;
			old_r_cast_spell = l_ptr->cast_spell;

			/* Window stuff */
			p_ptr->window |= (PW_MONSTER);

			/* Window stuff */
			window_stuff();
		}
	}
}





/*
 * Process the player
 *
 * Notice the annoying code to handle "pack overflow", which
 * must come first just in case somebody manages to corrupt
 * the savefiles by clever use of menu commands or something.
 *
 * Notice the annoying code to handle "monster memory" changes,
 * which allows us to avoid having to update the window flags
 * every time we change any internal monster memory field, and
 * also reduces the number of times that the recall window must
 * be redrawn.
 *
 * Note that the code to check for user abort during repeated commands
 * and running and resting can be disabled entirely with an option, and
 * even if not disabled, it will only check during every 128th game turn
 * while resting, for efficiency.
 */
static void process_player(void)
{
	int i;


	/*** Check for interrupts ***/

	/* Complete resting */
	if (p_ptr->resting < 0)
	{
		/* Check to see if on damaging terrain */
		/* XXX This may cause a big slow down, but is needed for 'correctness' */

		/*Get the feature */
		feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

		/* Use covered or bridged if necessary */
		if (f_ptr->flags2 & (FF2_COVERED))
		{
			f_ptr = &(f_info[f_ptr->mimic]);
		}

		/* Stop resting if would take damage */
		if (!(f_ptr->flags1 & (FF1_HIT_TRAP)) &&
		    ((f_ptr->blow.method) || (f_ptr->spell)))
		{

			disturb(0, 0);
		}

		/* Stop resting if tiring too fast */
		if ((p_ptr->tiring + 10) > PY_REST_RATE)
		{
			disturb(0, 0);
		}


		/* Basic resting */
		if (p_ptr->resting == -1)
		{
			/* Stop resting */
			if ((p_ptr->chp == p_ptr->mhp) &&
			    (p_ptr->csp == p_ptr->msp) &&
			    (p_ptr->rest > PY_REST_FULL))
			{
				disturb(0, 0);
			}
		}

		/* Complete resting */
		else if (p_ptr->resting == -2)
		{
			/* Stop resting */
			if ((p_ptr->chp == p_ptr->mhp) &&
			    (p_ptr->csp == p_ptr->msp) &&
			    !p_ptr->timed[TMD_BLIND] && !p_ptr->timed[TMD_CONFUSED] &&
			    !p_ptr->timed[TMD_POISONED] && !p_ptr->timed[TMD_AFRAID] &&
			    !p_ptr->timed[TMD_STUN] && !p_ptr->timed[TMD_CUT] &&
			    !p_ptr->timed[TMD_SLOW] && !p_ptr->timed[TMD_PARALYZED] &&
			    !p_ptr->timed[TMD_IMAGE] && !p_ptr->word_recall &&
			    (p_ptr->rest > PY_REST_FULL))
			{
				disturb(0, 0);
			}
		}
	}

	/* Check for "player abort" */
	if (p_ptr->running ||
		 p_ptr->command_rep ||
		 (p_ptr->resting && !(turn & 0x7F)))
	{
		/* Do not wait */
		inkey_scan = TRUE;

		/* Check for a key */
		if (anykey().key)
		{
			/* Flush input */
			flush();

			/* Disturb */
			disturb(0, 0);

			/* Hack -- Show a Message */
			msg_print("Cancelled.");
		}
	}

	/*** Start searching ***/
	if ((easy_search) && (!p_ptr->searching) && (p_ptr->last_disturb < (turn-20)))
	{
		/* Start searching */
		p_ptr->searching = TRUE;

		/* Stop sneaking */
		p_ptr->sneaking = FALSE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/*** Clear dodging ***/
	if (p_ptr->dodging)
	{
		/* Set charging -- reverse direction 180 degrees */
		p_ptr->charging = 10 - p_ptr->dodging;

		/* Clear dodging */
		p_ptr->dodging = 0;
	}

	/*** Clear blocking ***/
	if (p_ptr->blocking)
	{
		/* Reduce blocking */
		p_ptr->blocking--;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/*** Clear not sneaking ***/
	if (p_ptr->not_sneaking)
	{
		p_ptr->not_sneaking = FALSE;
	}

	/*** Handle actual user input ***/

	/* Repeat until energy is reduced */
	do
	{
		/* Notice stuff (if needed) */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff (if needed) */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff (if needed) */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff (if needed) */
		if (p_ptr->window) window_stuff();


		/* Place the cursor on the player */
		move_cursor_relative(p_ptr->py, p_ptr->px);

		/* Refresh (optional) */
		Term_fresh();

		/* Check for pack overflow */
		overflow_pack();

		/* Hack -- cancel "lurking browse mode" */
		if (!p_ptr->command_new.key) p_ptr->command_see = FALSE;


		/* Assume free turn */
		p_ptr->energy_use = 0;

#ifdef ALLOW_BORG
		/* Using the borg. */
		if (count_stop) do_cmd_borg();

		/* Paralyzed or Knocked Out */
		else
#endif
		if ((p_ptr->timed[TMD_PARALYZED]) || (p_ptr->timed[TMD_STUN] >= 100) || (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP))
		{
			/* Get the feature */
			feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

			/* Take a turn */
			p_ptr->energy_use = 100;

			/* Catch breath */
			if (!(f_ptr->flags2 & (FF2_FILLED)))
			{
				/* Rest the player */
				set_rest(p_ptr->rest + PY_REST_RATE - p_ptr->tiring);
			}
		}

		/* Picking up objects */
		else if (p_ptr->notice & (PN_PICKUP))
		{
			/* Recursively call the pickup function, use energy */
			p_ptr->energy_use = py_pickup(p_ptr->py, p_ptr->px, 0) * 10;
			p_ptr->notice &= ~(PN_PICKUP);
		}

		/* Resting */
		else if (p_ptr->resting)
		{
			/* Get the feature */
			feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

			/* Timed rest */
			if (p_ptr->resting > 0)
			{
				/* Reduce rest count */
				p_ptr->resting--;

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATE);
			}

			/* Catch breath */
			if (!(f_ptr->flags2 & (FF2_FILLED)))
			{
				/* Rest the player */
				set_rest(p_ptr->rest + PY_REST_RATE * 2 - p_ptr->tiring);
			}

			/* Take a turn */
			p_ptr->energy_use = 100;
		}

		/* Running */
		else if (p_ptr->running)
		{
			/* Take a step */
			run_step(0);
		}

		/* Repeated command */
		else if (p_ptr->command_rep)
		{
			/* Hack -- Assume messages were seen */
			msg_flag = FALSE;

			/* Clear the top line */
			prt("", 0, 0);

			/* Process the command */
			process_command();

			/* Count this execution */
			if (p_ptr->command_rep)
			{
				/* Count this execution */
				p_ptr->command_rep--;

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATE);

				/* Redraw stuff */
				/* redraw_stuff(); */
			}
		}

		/* Normal command */
		else
		{
			/* Check monster recall */
			process_player_aux();

			/* Display the monlist - unless a command queued */
			if ((auto_monlist) && !(p_ptr->command_new.key)) display_monlist(1, 0, 11, TRUE, FALSE);

			/* Place the cursor on the player */
			move_cursor_relative(p_ptr->py, p_ptr->px);

			/* Get a command (normal) */
			request_command(FALSE);

			/* Process the command */
			process_command();
		}

		/*** Clean up ***/

		/* Action is or was resting */
		if (p_ptr->resting)
		{
			/* Increment the resting counter */
			p_ptr->resting_turn++;
		}

		/* Significant */
		if (p_ptr->energy_use)
		{
			/* Hack -- sing song */
			if (p_ptr->held_song)
			{
				/* Cast the spell */
			    if (player_cast_spell(p_ptr->held_song, spell_power(p_ptr->held_song), ((p_ptr->pstyle == WS_INSTRUMENT)?"play":"sing"), "song"))
					/* Hack -- if not aborted, always use a full turn */
					p_ptr->energy_use = 100;
			}

			/* Use some energy */
			p_ptr->energy -= p_ptr->energy_use;

			/* Increment the player turn counter */
			p_ptr->player_turn++;

			/* Hack -- constant hallucination */
			if (p_ptr->timed[TMD_IMAGE]) p_ptr->redraw |= (PR_MAP);

			/* Shimmer monsters if needed */
			if (shimmer_monsters)
			{
				/* Clear the flag */
				shimmer_monsters = FALSE;

				/* Shimmer multi-hued monsters */
				for (i = 1; i < m_max; i++)
				{
					monster_type *m_ptr;
					monster_race *r_ptr;

					/* Get the monster */
					m_ptr = &m_list[i];

					/* Skip dead monsters */
					if (!m_ptr->r_idx) continue;

					/* Get the monster race */
					r_ptr = &r_info[m_ptr->r_idx];

					/* Skip non-multi-hued monsters */
					if (!(r_ptr->flags1 & (RF1_ATTR_MULTI)) && !(r_ptr->flags9 & (RF9_ATTR_METAL))) continue;

					/* Reset the flag */
					shimmer_monsters = TRUE;

					/* Redraw regardless */
					lite_spot(m_ptr->fy, m_ptr->fx);
				}
			}

			/* Repair "mark" flags */
			if (repair_mflag_mark)
			{
				/* Reset the flag */
				repair_mflag_mark = FALSE;

				/* Process the monsters */
				for (i = 1; i < m_max; i++)
				{
					monster_type *m_ptr;

					/* Get the monster */
					m_ptr = &m_list[i];

					/* Skip dead monsters */
					/* if (!m_ptr->r_idx) continue; */

					/* Repair "mark" flag */
					if (m_ptr->mflag & (MFLAG_MARK))
					{
						/* Skip "show" monsters */
						if (m_ptr->mflag & (MFLAG_SHOW))
						{
							/* Repair "mark" flag */
							repair_mflag_mark = TRUE;

							/* Skip */
							continue;
						}

						/* Forget flag */
						m_ptr->mflag &= ~(MFLAG_MARK);

						/* Update the monster */
						update_mon(i, FALSE);
					}
				}
			}
		}

		/* Repair "show" flags */
		if (repair_mflag_show)
		{
			/* Reset the flag */
			repair_mflag_show = FALSE;

			/* Process the monsters */
			for (i = 1; i < m_max; i++)
			{
				monster_type *m_ptr;

				/* Get the monster */
				m_ptr = &m_list[i];

				/* Skip dead monsters */
				/* if (!m_ptr->r_idx) continue; */

				/* Clear "show" flag */
				m_ptr->mflag &= ~(MFLAG_SHOW);
			}
		}
	}
	while (!p_ptr->energy_use && !p_ptr->leaving);

	/*** Clear charging ***/
	if (p_ptr->charging)
	{
		/* Clear charging */
		p_ptr->charging = 0;
	}

	/*** Clear dodging ***/
	if ((p_ptr->dodging) && (p_ptr->sneaking) && !(p_ptr->not_sneaking))
	{
		/* Lose benefits of dodging and be unable to charge */
		p_ptr->dodging = 0;
	}

	/* Update noise flow information */
	update_noise();

	/* Update scent trail */
	update_smell();


	/*
	 * Reset character vulnerability.  Will be calculated by
	 * the first member of an animal pack that has a use for it.
	 */
	p_ptr->vulnerability = 0;

}



/*
 * Interact with the current dungeon level.
 *
 * This function will not exit until the level is completed,
 * the user dies, or the game is terminated.
 */
static void dungeon(void)
{
	/* Hack -- enforce illegal panel */
	p_ptr->wy = DUNGEON_HGT;
	p_ptr->wx = DUNGEON_WID;


	/* Not leaving */
	p_ptr->leaving = FALSE;

	/* Reset the "command" vars */
	p_ptr->command_cmd = 0;
	p_ptr->command_new.key = 0;
	p_ptr->command_rep = 0;
	p_ptr->command_arg = 0;
	p_ptr->command_dir = 0;


	/* Cancel the target */
	target_set_monster(0, 0);

	/* Cancel the health bar */
	health_track(0);


	/* Reset shimmer flags */
	shimmer_monsters = TRUE;
	shimmer_objects = TRUE;

	/* Reset repair flags */
	repair_mflag_show = TRUE;
	repair_mflag_mark = TRUE;


	/* Disturb */
	disturb(1, 0);

	/* Track maximum dungeon level; surface does not count */
	if (p_ptr->max_depth < p_ptr->depth
		&& p_ptr->depth > min_depth(p_ptr->dungeon))
	{
		p_ptr->max_depth = p_ptr->depth;
	}

	/* Choose panel */
	verify_panel();


	/* Flush messages */
	msg_print(NULL);


	/* Hack -- Increase "xtra" depth */
	character_xtra++;


	/* Clear */
	Term_clear();

	/* Update stuff */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

	/* Calculate torch radius */
	p_ptr->update |= (PU_TORCH);

	/* Update stuff */
	update_stuff();

	/* Fully update the visuals (and monster distances) */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_DISTANCE);

	/* Redraw dungeon */
	p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP | PR_ITEM_LIST);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Window stuff */
	p_ptr->window |= (PW_MONSTER | PW_MONLIST);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP);

	/* Update stuff */
	update_stuff();

	/* Redraw stuff */
	redraw_stuff();

	/* Redraw stuff */
	window_stuff();

	/* Hack -- Decrease "xtra" depth */
	character_xtra--;

	/* Update stuff */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_ROOM_INFO);

	/* Combine / Reorder the pack */
/*	p_ptr->notice |= (PN_COMBINE | PN_REORDER);*/

	/* Window stuff */
/*	p_ptr->window |= (PW_ROOM_INFO);*/

	/* Notice stuff */
	notice_stuff();

	/* Update stuff */
	update_stuff();

	/* Redraw stuff */
	redraw_stuff();

	/* Window stuff */
	window_stuff();

	/* Refresh */
	Term_fresh();

	/* Handle delayed death */
	if (p_ptr->is_dead) return;

	/* Announce (or repeat) the feeling */
	if (p_ptr->depth > min_depth(p_ptr->dungeon))
		do_cmd_feeling();

	/*** Process this dungeon level ***/

	/* Reset the object generation level */
	object_level = p_ptr->depth;

	/* Beginners get a tip */
	if (adult_beginner) show_tip();

	/* Main loop */
	while (TRUE)
	{
		int i;

		/* Reset the monster generation level; make level feeling interesting */
		monster_level = p_ptr->depth >= 4 ? p_ptr->depth + 2 :
			(p_ptr->depth >= 2 ? p_ptr->depth + 1 : p_ptr->depth);

		/* Hack -- Compact the monster list occasionally */
		if (m_cnt + 32 > z_info->m_max) compact_monsters(64);

		/* Hack -- Compress the monster list occasionally */
		if (m_cnt + 32 < m_max) compact_monsters(0);

		/* Hack -- Compact the object list occasionally */
		if (o_cnt + 32 > z_info->o_max) compact_objects(64);

		/* Hack -- Compress the object list occasionally */
		if (o_cnt + 32 < o_max) compact_objects(0);

		/*** Verify the object list ***/
		/*
		 * This is required until we identify the source of object corruption.
		 */
		for (i = 1; i < z_info->o_max; i++)
		{
			/* Check for straightforward corruption */
			if (o_list[i].next_o_idx == i)
			{
				msg_format("Object %d (%s) corrupted. Fixing.", i, k_name + k_info[o_list[i].k_idx].name);
				o_list[i].next_o_idx = 0;

				if (o_list[i].held_m_idx)
				{
					msg_format("Was held by %d.", o_list[i].held_m_idx);
				}
				else
				{
					msg_format("Was held at (%d, %d).", o_list[i].iy, o_list[i].ix);
				}
			}
		}

		/*** Apply energy ***/

		/* Check speed */
		s16b delta_energy = extract_energy[p_ptr->pspeed];

		/* Apply "encumbrance" from weight */
		if (p_ptr->total_weight > (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 50))
				delta_energy -= (delta_energy * ((p_ptr->total_weight / (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 10)) - 5)) / 10;
		if(cheat_xtra) msg_format("Player gets energy %i", delta_energy);

		/* Hack -- player always gets at least one energy */
		p_ptr->energy += ((delta_energy > 0) ? delta_energy : 1);

		/* Can the player move? */
		while ((p_ptr->energy >= 100) && !p_ptr->leaving)
		{

			/* process monster with even more energy first */
			process_monsters((byte)(p_ptr->energy + 1));

			/* if still alive */
			if (!p_ptr->leaving)
			{
				/* Process the player */
				process_player();
			}
		}

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff */
		if (p_ptr->window) window_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->py, p_ptr->px);

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		/* Process monsters */
		process_monsters(0);

		/* Reset Monsters */
		reset_monsters();

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff */
		if (p_ptr->window) window_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->py, p_ptr->px);

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		/* Process the world */
		process_world();

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Window stuff */
		if (p_ptr->window) window_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->py, p_ptr->px);

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		/* Count game turns */
		turn++;
	}
}



/*
 * Process some user pref files
 *
 * Hack -- Allow players on UNIX systems to keep a ".angband.prf" user
 * pref file in their home directory.  Perhaps it should be loaded with
 * the "basic" user pref files instead of here.  This may allow bypassing
 * of some of the "security" compilation options.  XXX XXX XXX XXX XXX
 */
static void process_some_user_pref_files(void)
{
	char buf[1024];

#ifdef ALLOW_PREF_IN_HOME
#ifdef SET_UID

	char *homedir;

#endif /* SET_UID */
#endif /* ALLOW_PREF_IN_HOME */

	/* Process the "user.prf" file */
	(void)process_pref_file("user.prf");

	/* Get the "PLAYER.prf" filename */
	sprintf(buf, "%s.prf", op_ptr->base_name);

	/* Process the "PLAYER.prf" file */
	(void)process_pref_file(buf);

#ifdef ALLOW_PREF_IN_HOME
#ifdef SET_UID

	/* Process the "~/.angband.prf" file */
	if ((homedir = getenv("HOME")))
	{
		/* Get the ".angband.prf" filename */
		path_build(buf, 1024, homedir, ".angband.prf");

		/* Process the ".angband.prf" file */
		(void)process_pref_file(buf);
	}

#endif /* SET_UID */
#endif /* ALLOW_PREF_IN_HOME */
}


/*
 * Actually play a game.
 *
 * This function is called from a variety of entry points, since both
 * the standard "main.c" file, as well as several platform-specific
 * "main-xxx.c" files, call this function to start a new game with a
 * new savefile, start a new game with an existing savefile, or resume
 * a saved game with an existing savefile.
 *
 * If the "new_game" parameter is true, and the savefile contains a
 * living character, then that character will be killed, so that the
 * player may start a new game with that savefile.  This is only used
 * by the "-n" option in "main.c".
 *
 * If the savefile does not exist, cannot be loaded, or contains a dead
 * (non-wizard-mode) character, then a new game will be started.
 *
 * Several platforms (Windows, Macintosh, Amiga) start brand new games
 * with "savefile" and "op_ptr->base_name" both empty, and initialize
 * them later based on the player name.  To prevent weirdness, we must
 * initialize "op_ptr->base_name" to "PLAYER" if it is empty.
 *
 * Note that we load the RNG state from savefiles (2.8.0 or later) and
 * so we only initialize it if we were unable to load it.  The loading
 * code marks successful loading of the RNG state using the "Rand_quick"
 * flag, which is a hack, but which optimizes loading of savefiles.
 */
void play_game(bool new_game)
{
	int i;

	/* Hack -- Increase "icky" depth */
	character_icky++;

	/* Verify main term */
	if (!angband_term[0])
	{
		quit("main window does not exist");
	}

	/* Make sure main term is active */
	Term_activate(angband_term[0]);

	/* Verify minimum size */
	if ((Term->hgt < 24) || (Term->wid < 80))
	{
		quit("main window is too small");
	}

	/* Hack -- Turn off the cursor */
	(void)Term_set_cursor(0);

	/* Attempt to load */
	if (!load_player())
	{
		/* Oops */
		quit("broken savefile");
	}

	/* Nothing loaded */
	if (!character_loaded)
	{
		/* Make new player */
		new_game = TRUE;

		/* The dungeon is not ready */
		character_dungeon = FALSE;
	}

	/* Hack -- Default base_name */
	if (!op_ptr->base_name[0])
	{
		my_strcpy(op_ptr->base_name, "PLAYER", sizeof(op_ptr->base_name));
	}

	/* Hack -- initialise options for the first time */
	if (!op_ptr->monlist_display)
	{
		op_ptr->monlist_display = 3;
		op_ptr->monlist_sort_by = 2;
		op_ptr->delay_factor = 9;
	}

	/* Init RNG */
	if (Rand_quick)
	{
		u32b seed;

		/* Basic seed */
		seed = (u32b)(time(NULL));

#ifdef SET_UID

		/* Mutate the seed on Unix machines */
		seed = ((seed >> 3) * (getpid() << 1));

#endif

		/* Use the complex RNG */
		Rand_quick = FALSE;

		/* Seed the "complex" RNG */
		Rand_state_init(seed);
	}

	/* Roll new character */
	if (new_game)
	{
		/* The dungeon is not ready */
		character_dungeon = FALSE;

		/* Start in town */
		p_ptr->depth = 0;

		/* Hack -- seed for flavors */
		seed_flavor = rand_int(0x10000000);

		/* Hack -- seed for town layout */
		seed_town = rand_int(0x10000000);

		/* Hack -- don't do seeded dungeons */
		seed_dungeon = 0;

		/* Hack -- seed for random artifacts */
		if (adult_reseed_artifacts)
			seed_randart = rand_int(0x10000000);

		/* Hack -- clear artifact memory */
		/* Needs to be done even with no adult_reseed_artifacts,
		   because the state of adult_randarts may have changed either way */
		for (i = 0;i<z_info->a_max;i++)
		{
			object_info *n_ptr = &a_list[i];

			n_ptr->can_flags1 = 0x0L;
			n_ptr->can_flags2 = 0x0L;
			n_ptr->can_flags3 = 0x0L;
			n_ptr->can_flags4 = 0x0L;

			n_ptr->not_flags1 = 0x0L;
			n_ptr->not_flags2 = 0x0L;
			n_ptr->not_flags3 = 0x0L;
			n_ptr->not_flags4 = 0x0L;
		}

		/* Roll up a new character */
		player_birth();

		/* Generate random artifacts */
		/* Needs to be done even with no adult_reseed_artifacts,
		   because the state of adult_randarts may have changed either way */
		do_randart(seed_randart, TRUE);

		/* Hack -- enter the world */
		turn = 1;
		p_ptr->player_turn = 0;
		p_ptr->resting_turn = 0;
	}

	/* Normal machine (process player name) */
	if (savefile[0])
	{
		process_player_name(FALSE);
	}

	/* Weird machine (process player name, pick savefile name) */
	else
	{
		process_player_name(TRUE);
	}

	/* Flash a message */
	prt("Please wait...", 0, 0);

	/* Flush the message */
	Term_fresh();

	/* Hack -- Enter wizard mode */
	if (arg_wizard && enter_wizard_mode()) p_ptr->wizard = TRUE;

	/* Flavor the objects */
	flavor_init();

	/* Reset visuals */
	reset_visuals(TRUE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1 | PW_PLAYER_2 | PW_PLAYER_3);

	/* Window stuff */
	p_ptr->window |= (PW_MONSTER | PW_MESSAGE);

	/* Window stuff */
	window_stuff();

	/* Process some user pref files */
	process_some_user_pref_files();

	/* Set or clear "rogue_like_commands" if requested */
	if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;

	/* React to changes */
	Term_xtra(TERM_XTRA_REACT, 0);

	/* Mark the fixed monsters as quests; other checks */
	if (adult_campaign)
	{
		for (i = 0; i < z_info->t_max; i++)
		{
			int guard, ii;

			guard = t_info[i].quest_monster;
			/* Mark map quest monsters as 'unique guardians'. This allows
			 * the map quests to be completed successfully.
			 */
			if (guard)
			{
				assert (r_info[guard].flags1 & RF1_UNIQUE);
				r_info[guard].flags1 |= RF1_GUARDIAN;
				if (r_info[guard].max_num > 1) r_info[guard].max_num = 1;
			}

			/*
			 * The same for replacement guardians
			 */
			guard = t_info[i].replace_guardian;
			if (guard)
			{
				assert (r_info[guard].flags1 & RF1_UNIQUE);
				r_info[guard].flags1 |= RF1_GUARDIAN;
				if (r_info[guard].max_num > 1) r_info[guard].max_num = 1;
			}

			/* We must also ensure that town lockup monsters are
			 * unique to allow the town to be unlocked later.
			 * Not necessarily guardians, because they don't block dungeons.
			 */
			guard = t_info[i].town_lockup_monster;
			if (guard)
			{
				assert (r_info[guard].flags1 & RF1_UNIQUE);
				if (r_info[guard].max_num > 1) r_info[guard].max_num = 1;
			}

			for (ii = 0; ii < MAX_DUNGEON_ZONES; ii++)
			{
				/*
				 * And we ensure dungeon mini-bosses are marked as
				 * guardians, so that they do not appear elsewhere
				 */
				guard = t_info[i].zone[ii].guard;
				if (guard)
				{
					int zone_depth;

					assert (r_info[guard].flags1 & RF1_UNIQUE);
					r_info[guard].flags1 |= (RF1_GUARDIAN);
					if (r_info[guard].max_num > 1) r_info[guard].max_num = 1;

					/* Sane depth */
					if (ii == MAX_DUNGEON_ZONES - 1
						|| t_info[i].zone[ii+1].level == 0)
						/* last zone */
						zone_depth = t_info[i].zone[ii].level;
					else
						zone_depth = t_info[i].zone[ii+1].level - 1;

#if 0
					if (r_info[guard].level /* Maggot */
						&& r_info[guard].level < zone_depth)
						fputs(format("Warning: Guardian %d (%s) wimpy.\n", guard, r_info[guard].name + r_name), stderr);
					if (r_info[guard].calculated_level < MIN(58, zone_depth + 4))
						fputs(format("Warning: Guardian %d (%s) really wimpy.\n", guard, r_info[guard].name + r_name), stderr);
					if (r_info[guard].level > zone_depth + 12)
						fputs(format("Warning: Guardian %d (%s) deadly.\n", guard, r_info[guard].name + r_name), stderr);
					if (r_info[guard].calculated_level > zone_depth + 20)
						fputs(format("Warning: Guardian %d (%s) really deadly.\n", guard, r_info[guard].name + r_name), stderr);

#endif
				}
			}
		}
	}

	/* Generate familiar attributes */
	generate_familiar();

	/* Generate a dungeon level if needed */
	if (!character_dungeon) generate_cave();

	/* Character is now "complete" */
	character_generated = TRUE;

	/* Hack -- Decrease "icky" depth */
	character_icky--;

	/* Start playing */
	p_ptr->playing = TRUE;

	/* Hack -- Enforce "delayed death" */
	if (p_ptr->chp < 0) p_ptr->is_dead = TRUE;

	/* Process */
	while (TRUE)
	{

		/* Process the level */
		dungeon();

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Window stuff */
		if (p_ptr->window) window_stuff();


		/* Cancel the target */
		target_set_monster(0, 0);

		/* Cancel the health bar */
		health_track(0);


		/* Forget the view */
		forget_view();


		/* Handle "quit and save" */
		if (!p_ptr->playing && !p_ptr->is_dead) break;


		/* Erase the old cave */
		wipe_o_list();
		wipe_m_list();
		wipe_region_piece_list();
		wipe_region_list();

		/* XXX XXX XXX */
		msg_print(NULL);

		/* Accidental Death */
		if (p_ptr->playing && p_ptr->is_dead)
		{
			/* Mega-Hack -- Allow player to cheat death */
			if ((p_ptr->wizard || cheat_live) && !get_check("Die? "))
			{
				/* Mark social class, reset age, if needed */
				if (p_ptr->sc) p_ptr->sc = p_ptr->age = 0;

				/* Increase age */
				p_ptr->age++;

				/* Mark savefile */
				p_ptr->noscore |= 0x0001;

				/* Message */
				msg_print("You invoke wizard mode and cheat death.");
				msg_print(NULL);

				/* Cheat death */
				p_ptr->is_dead = FALSE;

				/* Restore hit points */
				p_ptr->chp = p_ptr->mhp;
				p_ptr->chp_frac = 0;

				/* Restore spell points */
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;

				/* Hack -- Clear all timers */
				for (i = 0; i < TMD_MAX; i++)
				{
					p_ptr->timed[i] = 0;
				}

				/* Hack -- Prevent starvation */
				(void)set_food(PY_FOOD_MAX - 1);

				/* Hack -- cancel recall */
				if (p_ptr->word_recall)
				{
					/* Message */
					msg_print("A tension leaves the air around you...");
					msg_print(NULL);

					/* Hack -- Prevent recall */
					p_ptr->word_recall = 0;
				}

				/* Note cause of death XXX XXX XXX */
				my_strcpy(p_ptr->died_from, "Cheating death", sizeof(p_ptr->died_from));
				/* New depth */
				p_ptr->depth = min_depth(p_ptr->dungeon);

				/* Leaving */
				p_ptr->leaving = TRUE;
			}
		}

		/* Handle "death" */
		if (p_ptr->is_dead) break;

		/* Make a new level */
		generate_cave();
	}

	/* Close stuff */
	close_game();

	/* Quit */
	quit(NULL);
}
