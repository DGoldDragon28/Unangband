/* File: monster2.c */

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
 * We use this routine to detect when a monster race is a quest race.
 *
 * If so, we don't place this monster randomly.
 */
bool is_quest_race(int r_idx)
{
	int i;

	/* Unique quest targets are never placed randomly */
	if (r_info[r_idx].flags1 & (RF1_UNIQUE)) return (TRUE);

	/* Other quest monsters may be found outside of the quest location */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		quest_type *q_ptr = &(q_list[i]);
		quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);

		if (q_ptr->stage == QUEST_ACTION) qe_ptr = &(q_ptr->event[QUEST_ACTIVE]);

		if ((qe_ptr->dungeon) && ((qe_ptr->dungeon != p_ptr->dungeon) ||
			(qe_ptr->level != p_ptr->depth - min_depth(p_ptr->dungeon)))) continue;

		if (!(qe_ptr->race) || (qe_ptr->race != r_idx)) continue;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Return another race for a monster to polymorph into.  -LM-
 *
 * Perform a modified version of "get_mon_num()", with exact minimum and
 * maximum depths and preferred monster types.
 *
 * We know have to ensure that target race can survive on the intended
 * terrain.
 */
s16b poly_r_idx(int y, int x, int base_idx, bool require_char, bool harmless, bool chaotic)
{
	monster_race *r_ptr = &r_info[base_idx];

	alloc_entry *table = alloc_race_table;

	int i, min_lev, max_lev, r_idx;
	long total, value;

	/* int q_idx = q_info[quest_num(p_ptr->depth)].r_idx; */

	/* Source monster's level and symbol */
	int r_lev = r_ptr->level;
	char d_char = r_ptr->d_char;


	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags1 & (RF1_UNIQUE))
	{
		return (base_idx);
	}
	
	/* Hack - harmless monsters */
	if (harmless) r_lev /= 10;

	/* Allowable level of new monster */
	min_lev = (MAX(        1, r_lev - 1 - r_lev / 5));
	max_lev = (MIN(MAX_DEPTH, r_lev + 1 + r_lev / 5));

	/* Hack -- harmless monsters */
	if (harmless) min_lev = 0;
	
	/* Reset sum */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Assume no probability */
		table[i].prob3 = 0;

		/* Ignore illegal monsters - only those that don't get generated. */
		if (!table[i].prob1) continue;

		/* Not below the minimum base depth */
		if (table[i].level < min_lev) continue;

		/* Not above the maximum base depth */
		if (table[i].level > max_lev) continue;

		/* Get the monster index */
		r_idx = table[i].index;

		/* We're polymorphing -- we don't want the same monster */
		if (r_idx == base_idx) continue;

		/* Get the actual race */
		r_ptr = &r_info[r_idx];

		/* Hack -- No uniques */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Hack -- Never polymorph into a quest monster. */
		if ((r_ptr->flags1 & (RF1_QUESTOR)) && (is_quest_race(r_idx))) continue;

		/* Hack -- Ensure monster can survive on intended terrain */
		if ((!harmless) && (!require_char) && (place_monster_here(y, x, r_idx) <= MM_FAIL)) continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Bias against monsters far from initial monster's depth */
		if (table[i].level < (min_lev + r_lev) / 2) table[i].prob3 /= 4;
		if (table[i].level > (max_lev + r_lev) / 2) table[i].prob3 /= 4;
		
		/* Special checks for harmless */
		if (harmless)
		{
			if ((r_ptr->flags3 & (RF3_ANIMAL)) == 0) table[i].prob3 = 0;
		}
		
		/* Special checks for chaotic */
		if (chaotic)
		{
			if ((r_ptr->flags4 & (RF4_BRTH_CHAOS)) == 0) table[i].prob3 /= 4;
		}
		
		/* Bias against monsters not of the same symbol */
		if (r_ptr->d_char != d_char)
		{
			if ((require_char) && !(harmless)) table[i].prob3 = 0;
			else table[i].prob3 /= 4;
		}

		/* Sum up probabilities */
		total += table[i].prob3;
	}

	/* No legal monsters */
	if (total == 0)
	{
		return (base_idx);
	}


	/* Pick a monster */
	value = rand_int(total);

	/* Find the monster */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}

	/* Result */
	return (table[i].index);
}


/*
 * Check whether monster has a lite
 */
bool check_monster_lite(int m_idx)
{
	/* Get monster */
	monster_type *m_ptr = &m_list[m_idx];

	/* Monster has a lite */
	return ((m_ptr->mflag & (MFLAG_LITE)) != 0);
}


/*
 * Monster light functions
 */
bool require_torch_lit(int y, int x)
{
	return ((cave_info[y][x] & (CAVE_TLIT)) != 0);
}

bool has_torch_lit(int y, int x)
{
	/* No monster, no torch lite */
	if (cave_m_idx[y][x] <= 0) return FALSE;

	/* Check monster lite */
	if (check_monster_lite(cave_m_idx[y][x])) return TRUE;

	return FALSE;
}

bool redraw_torch_lit_loss(int y, int x)
{
	return ((play_info[y][x] & (PLAY_VIEW)) && ((cave_info[y][x] & (CAVE_TLIT)) == 0));
}

bool redraw_torch_lit_gain(int y, int x)
{
	return ((play_info[y][x] & (PLAY_VIEW)) && ((cave_info[y][x] & (CAVE_TLIT)) != 0));
}

void apply_torch_lit(int y, int x)
{
	cave_info[y][x] |= (CAVE_TLIT);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}

int remove_torch_lit(int y, int x)
{
	cave_info[y][x] &= ~(CAVE_TLIT);

	if (!(play_info[y][x] & (PLAY_LITE)) && !(cave_info[y][x] & (CAVE_LITE))) play_info[y][x] &= ~(PLAY_SEEN);

	return (0);
}

void reapply_torch_lit(int y, int x, int r)
{
	(void)r;
	
	cave_info[y][x] |= (CAVE_TLIT);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}


/*
 * Delete a monster by index.
 *
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(int i)
{
	int x, y;

	monster_type *m_ptr = &m_list[i];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	s16b this_o_idx, next_o_idx = 0;


	/* Get location */
	y = m_ptr->fy;
	x = m_ptr->fx;


	/* Hack -- Reduce the racial counter */
	r_ptr->cur_num--;

	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags2 & (RF2_MULTIPLY)) num_repro--;

	/* Hack -- remove target monster */
	if (p_ptr->target_who == i) target_set_monster(0, 0);

	/* Hack -- remove tracked monster */
	if (p_ptr->health_who == i) health_track(0);


	/* Monster is gone */
	cave_m_idx[y][x] = 0;

	/* Delete objects */
	for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Hack -- efficiency */
		o_ptr->held_m_idx = 0;

		/* Delete the object */
		delete_object_idx(this_o_idx);
	}

	/* Extinguish lite */
	if (check_monster_lite(i))
	{
		/* Recheck surrounding lite */
		check_attribute_lost(y, x, 2, CAVE_XLOS, require_torch_lit, has_torch_lit, redraw_torch_lit_loss, remove_torch_lit, reapply_torch_lit);
	}

	/* Wipe the Monster */
	(void)WIPE(m_ptr, monster_type);

	/* Count monsters */
	m_cnt--;


	/* Visual update */
	lite_spot(y, x);
}


/*
 * Delete the monster, if any, at a given location
 */
void delete_monster(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Delete the monster (if any) */
	if (cave_m_idx[y][x] > 0) delete_monster_idx(cave_m_idx[y][x]);
}


/*
 * Delete the monster lite, if any, at a given location
 */
void delete_monster_lite(int i)
{
	int x, y;

	monster_type *m_ptr = &m_list[i];

	/* Get location */
	y = m_ptr->fy;
	x = m_ptr->fx;

	/* Extinguish lite */
	if (check_monster_lite(i))
	{
		/* Recheck surrounding lite */
		check_attribute_lost(y, x, 2, CAVE_XLOS, require_torch_lit, has_torch_lit, redraw_torch_lit_loss, remove_torch_lit, reapply_torch_lit);
	}
}


/*
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_monsters_aux(int i1, int i2)
{
	int y, x;

	monster_type *m_ptr;

	s16b this_o_idx, next_o_idx = 0;


	/* Do nothing */
	if (i1 == i2) return;


	/* Old monster */
	m_ptr = &m_list[i1];

	/* Location */
	y = m_ptr->fy;
	x = m_ptr->fx;

	/* Update the cave */
	cave_m_idx[y][x] = i2;

	/* Repair objects being carried by monster */
	for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Reset monster pointer */
		o_ptr->held_m_idx = i2;
	}

	/* Hack -- Update the target */
	if (p_ptr->target_who == i1) p_ptr->target_who = i2;

	/* Hack -- Update the health bar */
	if (p_ptr->health_who == i1) p_ptr->health_who = i2;

	/* Hack -- move monster */
	COPY(&m_list[i2], &m_list[i1], monster_type);

	/* Hack -- wipe hole */
	(void)WIPE(&m_list[i1], monster_type);
}


/*
 * Compact and Reorder the monster list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(int size)
{
	int i, num, cnt;

	int cur_lev, cur_dis, chance;


	/* Message (only if compacting) */
	if (size) msg_print("Compacting monsters...");


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Check all the monsters */
		for (i = 1; i < m_max; i++)
		{
			monster_type *m_ptr = &m_list[i];

			monster_race *r_ptr = &r_info[m_ptr->r_idx];

			/* Paranoia -- skip "dead" monsters */
			if (!m_ptr->r_idx) continue;

			/* Hack -- High level monsters start out "immune" */
			if (r_ptr->level > cur_lev) continue;

			/* Ignore nearby monsters */
			if ((cur_dis > 0) && (m_ptr->cdis < cur_dis)) continue;

			/* Saving throw chance */
			chance = 90;

			/* Only compact "Quest" Monsters in emergencies */
			if ((r_ptr->flags1 & (RF1_QUESTOR)) && (cnt < 1000)) chance = 100;

			/* Try not to compact Unique Monsters */
			if (r_ptr->flags1 & (RF1_UNIQUE)) chance = 99;

			/* All monsters get a saving throw */
			if (rand_int(100) < chance) continue;

			/* Delete the monster */
			delete_monster_idx(i);

			/* Count the monster */
			num++;
		}
	}


	/* Excise dead monsters (backwards!) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Get the i'th monster */
		monster_type *m_ptr = &m_list[i];

		/* Skip real monsters */
		if (m_ptr->r_idx) continue;

		/* Move last monster into open hole */
		compact_monsters_aux(m_max - 1, i);

		/* Compress "m_max" */
		m_max--;
	}
}


/*
 * Delete/Remove all the monsters when the player leaves the level
 *
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 */
void wipe_m_list(void)
{
	int i;

	/* Delete all the monsters */
	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Mega-Hack -- preserve Unique's XXX XXX XXX */

		/* Hack -- Reduce the racial counter */
		r_ptr->cur_num--;

		/* Monster is gone */
		cave_m_idx[m_ptr->fy][m_ptr->fx] = 0;

		/* Wipe the Monster */
		(void)WIPE(m_ptr, monster_type);
	}

	/* No longer a monster on the level */
	for (i = 0; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];

		r_ptr->cur_num = 0;
	}

	/* Reset "m_max" */
	m_max = 1;

	/* Reset "m_cnt" */
	m_cnt = 0;

	/* Hack -- reset "reproducer" count */
	num_repro = 0;

	/* Hack -- no more target */
	target_set_monster(0, 0);

	/* Hack -- no more tracking */
	health_track(0);
}


/*
 * Get and return the index of a "free" monster.
 *
 * This routine should almost never fail, but it *can* happen.
 */
s16b m_pop(void)
{
	int i;


	/* Normal allocation */
	if (m_max < z_info->m_max)
	{
		/* Get the next hole */
		i = m_max;

		/* Expand the array */
		m_max++;

		/* Count monsters */
		m_cnt++;

		/* Return the index */
		return (i);
	}


	/* Recycle dead monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr;

		/* Get the monster */
		m_ptr = &m_list[i];

		/* Skip live monsters */
		if (m_ptr->r_idx) continue;

		/* Count monsters */
		m_cnt++;

		/* Use this monster */
		return (i);
	}


	/* Warn the player (except during dungeon creation) */
	if (character_dungeon) msg_print("Too many monsters!");

	/* Try not to crash */
	return (0);
}

/*
 * Apply a "monster restriction function" to the "monster allocation table"
 */
errr get_mon_num_prep(void)
{
	int i;

	/* No ecology creatures valid yet */
	cave_ecology.valid_hook = FALSE;

	/* Using the ecology model */
	for (i = 0; i < cave_ecology.num_races; i++)
	{
		/* Accept monsters which pass the restriction, if any */
		if (!get_mon_num_hook || (*get_mon_num_hook)(cave_ecology.race[i]))
		{
			/* Accept this monster */
			cave_ecology.get_mon[i] = TRUE;

			/* Have a valid monster */
			cave_ecology.valid_hook = TRUE;
		}

		/* Do not use this monster */
		else
		{
			/* Decline this monster */
			cave_ecology.get_mon[i] = FALSE;
		}
	}

		/*
		 * Have a valid choice -- we should uncomment this for efficiency if we are sure we don't change value of
		 * cave_ecology.ready between calling get_mon_num_prep() and get_mon_num()
		 */
		/* if ((cave_ecology.ready) && (cave_ecology.valid_hook)) return (0); */

	/* Scan the allocation table */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Get the entry */
		alloc_entry *entry = &alloc_race_table[i];

		/* Accept monsters which pass the restriction, if any */
		if (!get_mon_num_hook || (*get_mon_num_hook)(entry->index))
		{
			/* Accept this monster */
			entry->prob2 = entry->prob1;
		}

		/* Do not use this monster */
		else
		{
			/* Decline this monster */
			entry->prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/*
 * Restrict a monster based on the level flags.
 *
 * Note we only apply 'hard' requirements here, such
 * as monsters needing to be immune to an element
 * because its splashed around the level, and no
 * 'soft' requirements, because a level may be
 * 'thematically' interesting.
 *
 * The exception is for multipliers and non-movers
 * because it'd be unfair on the player to fill
 * the level with multipliers, and on the monster
 * for the player to pick them off from a distance.
 */
bool check_level_flags_race(int r_idx)
{
	/* Get the actual race */
	monster_race *r_ptr = &r_info[r_idx];

	/* Must swim monsters only appear on water levels */
	if (((r_ptr->flags2 & (RF2_MUST_SWIM)) != 0) &&
		((level_flag & (LF1_WATER)) == 0))
	{
		return (FALSE);
	}

	/* Lava level */
	if (level_flag & (LF1_LAVA))
	{
		/* Require monsters immune to fire */
		if (!(r_ptr->flags3 & (RF3_IM_FIRE))) return (FALSE);
	}

	/* Acid level */
	if (level_flag & (LF1_ACID))
	{
		/* Require monsters immune to acid */
		if (!(r_ptr->flags3 & (RF3_IM_ACID))) return (FALSE);
	}

	/* Surface level */
	if (level_flag & (LF1_SURFACE))
	{
		/* No multiplying monsters on surface */
		if (r_ptr->flags2 & (RF2_MULTIPLY)) return (FALSE);

		/* No never moving monsters on surface */
		if (r_ptr->flags1 & (RF1_NEVER_MOVE)) return (FALSE);
	}

	/* XXX Note we have to move this check to get_mon_num, to allow a nighttime
	 * ecology to be created.
	 */
#if 0
	/* Daytime */
	if (level_flag & (LF1_DAYLIGHT))
	{
		/* No hurt lite monsters in daytime */
		if (r_ptr->flags3 & (RF3_HURT_LITE)) return (FALSE);
	}
#endif

	return (TRUE);
}



/*
 * Choose a monster race that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "monster allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" monster, in
 * a relatively efficient manner.
 *
 * Note that "town" monsters will *only* be created in the town, and
 * "normal" monsters will *never* be created in the town, unless the
 * "level" is "modified", for example, by polymorph or summoning.
 *
 * There is a small chance "boosting" the given depth by
 * a small amount (up to four levels), except in the town.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no monsters are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_mon_num(int level)
{
	int i, j, p;

	int r_idx;
	int closest_miss_r_idx = 0;
	int closest_miss_level = 0;

	long value, total;

	monster_race *r_ptr;

	alloc_entry *table = alloc_race_table;

	int local_monster_level = MIN(59, MAX(0, monster_level));

	level = MIN(59, MAX(0, level));

	/*
	 * Use the ecology model
	 *
	 * This is a completely different model of monster distribution.
	 *
	 * We initialise a small table at the start of level generation and then
	 * pick monsters from it. We ignore all rarity models and only accept
	 * monsters from the table.
	 */
	if ((cave_ecology.ready) && (cave_ecology.valid_hook))
	{
		int choice = -1;

		/* Count of choices */
		j = 0;

		/* Get monster */
		for (i = 0; i < cave_ecology.num_races; i++)
		{
			/* Get the actual race */
			r_ptr = &r_info[cave_ecology.race[i]];

			/* Hack -- "unique" monsters must be "unique" */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) &&
			    (r_ptr->cur_num >= r_ptr->max_num))
			{
				continue;
			}

			/* Sauron musn't be summoned with his forms already present */
			/* XXX This only occurs because we generate him in the ecology. */
			if (i == SAURON_TRUE)
			{
				for (j = SAURON_FORM; j < SAURON_FORM + MAX_SAURON_FORMS; j++)
				{
					if (r_info[j].cur_num) break;
				}

				/* Another form present */
				if (j < SAURON_FORM + MAX_SAURON_FORMS) continue;
			}

			/* Hack -- Never place quest monsters randomly. */
			if ((r_ptr->flags1 & (RF1_QUESTOR)) && (is_quest_race(i))) continue;

			/* Hack -- we are picking a random monster or doing initial
			 * placement of monsters in rooms.
			 */
			if (cave_ecology.use_ecology)
			{
				/* Doesn't exist in current ecology */
				if ((cave_ecology.race_ecologies[i] & (cave_ecology.use_ecology)) == 0) continue;
			}

			/*
			 * No "hurt" lite monsters allowed in daytime
			 *
			 * We have to check this here because it it 'time-dependent'
			 */
			if (level_flag & (LF1_DAYLIGHT))
			{
				/* No hurt lite monsters in daytime */
				if (r_ptr->flags3 & (RF3_HURT_LITE)) continue;
			}

			/* Can choose this monster? */
			if ((cave_ecology.get_mon[i] != 0) && (one_in_(++j))) choice = i;
		}

		/* Made a choice */
		if (choice >= 0)
			return (cave_ecology.race[choice]);

		/* Warn that we are not selecting from ecology */
		if (cheat_hear) msg_print("Unable to find a valid monster in the ecology");
	}

	/* Boost the level */
	if (level > 0)
	{
		/* Occasional "nasty" monster */
		if (rand_int(NASTY_MON) == 0)
		{
			/* Pick a level bonus */
			int d = level / 2 + 1;

			/* Boost the level */
			level += ((d < 5) ? d : 5);
		}

		/* Occasional "nasty" monster */
		if (rand_int(NASTY_MON) == 0)
		{
			/* Pick a level bonus */
			int d = level / 2;

			/* Boost the level */
			level += ((d < 5) ? d : 5);
		}
	}


	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Monsters are sorted by depth */
		if (table[i].level > level) break;

		/* Default */
		table[i].prob3 = 0;

		/* No town monsters in dungeon */
		if ((level > 0) && (table[i].level <= 0)) continue;

		/* Skip if no chance of monster appearing */
		if (!table[i].prob2) continue;

		/* Get the "r_idx" of the chosen monster */
		r_idx = table[i].index;

		/* Get the actual race */
		r_ptr = &r_info[r_idx];

		/* Hack -- "unique" monsters must be "unique" */
		if ((r_ptr->flags1 & (RF1_UNIQUE)) &&
		    (r_ptr->cur_num >= r_ptr->max_num))
		{
			continue;
		}

		/* Hack -- guardians must be placed specifically */
		if (r_ptr->flags1 & (RF1_GUARDIAN)) continue;

		/* Depth Monsters never appear at a different depth */
		if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && (r_ptr->level != p_ptr->depth)) continue;

		/* Hack -- Never place quest monsters randomly. */
		if ((r_ptr->flags1 & (RF1_QUESTOR)) && (is_quest_race(r_idx))) continue;
		
		/* Hack -- Never place stupid monsters in towns. */
		if ((r_ptr->flags2 & (RF2_STUPID)) && (cave_ecology.town)) continue;

		/* Check monster against level flags */
		if (!check_level_flags_race(r_idx)) continue;

		/* Allow monsters to be generated 'nearly' in-depth */
		if (table[i].level < MIN(local_monster_level - 4, level - 3))
		{
			int miss_level = table[i].level;
			int count = 0;

			/* Modify up for leveled monsters */
			if (r_ptr->flags9 & RF9_LEVEL_MASK) miss_level += 15;

			/* Allow a best effort choice in the event we can't find anything */
			/* Hack -- have a soft boundary, so we don't always get
			   the same monster very deep */
			if (closest_miss_level + 5 < miss_level
				|| (closest_miss_level <= miss_level
					&& one_in_(++count)))
			{
				closest_miss_r_idx = table[i].index;
				if (closest_miss_level < miss_level)
					closest_miss_level = miss_level;
			}

			/* Ensure minimum depth for monsters,
			   except those that have friends or level up */
			if ((r_ptr->flags1 & RF1_FRIENDS) == 0
				&& (r_ptr->flags9 & RF9_LEVEL_MASK) == 0)
				continue;

			/* Ensure hard minimum depth for monsters */
			if (table[i].level < MIN(local_monster_level - 19, level - 18))
				continue;
		}

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Prefer monsters closer to the actual level */
		if (table[i].level < level - 4) table[i].prob3 /= 2;
		else if (table[i].level < level - 9) table[i].prob3 /= 3;
		else if (table[i].level < level - 14) table[i].prob3 /= 4;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal monsters */
	if (total <= 0)
	{
		/* We've found a near-miss */
		if (closest_miss_level)
		{
			if (cheat_hear) msg_format("Picking closest miss (%s).", r_name + r_info[closest_miss_r_idx].name);

			return (closest_miss_r_idx);
		}
		else
		{
			if (cheat_hear) msg_print("No legal monsters.");

			return (0);
		}
	}

	/* Report range of level monsters */
	if (cheat_hear) msg_format("Picking from probably range of %d.", total);

	/* Pick a monster */
	value = rand_int(total);

	/* Find the monster */
	for (i = 0; i < alloc_race_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}

	/* Power boost */
	p = rand_int(100);

	/* Try for a "harder" monster once or twice */
	if (p < 70)
	{
		/* Save old */
		j = i;

		/* Pick a monster */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_race_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "harder" monster twice */
	if (p < 30)
	{
		/* Save old */
		j = i;

		/* Pick a monster */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_race_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Oops */
	if ((cave_ecology.ready) && (cheat_hear)) msg_format("Picking non-ecology monster (%s).", r_name + r_info[table[i].index].name);

	/* Result */
	return (table[i].index);
}



static const char *sort_by_name[]=
{
		"distance",
		"depth",
		"vengeance"
};


/*
 * Display visible monsters and/or objects in a window
 *
 * Mode defines whether we see monsters, objects, features,
 * and whether we show or hide stuff we are only aware of.
 *
 * 1 	- monsters
 * 2	- objects
 * 4	- features (not implemented)
 * 8	- hide objects we're only aware of
 *
 * Sort by values:
 *
 * 0	- distance
 * 1	- depth
 * 2	- number of times your ancestor has been killed
 *
 * Returns the width of the monster and/or object lists, or 0 if no monsters/objects are seen.
 */
void display_monlist(int row, unsigned int width, int mode, bool command, bool force)
{
	int idx, max;
	int line;
	int total_count, disp_count, status_count;
	int forreal;

	char m_name[80];
	char buf[80];
	monster_type *m_ptr;
	monster_race *r_ptr;
	object_type *o_ptr;

	u16b *race_counts;
	u16b *sleep_counts;
	u16b *kind_counts;
	u16b *artifact_counts;
	u16b *unknown_counts;

	int i, j;

	int max_sort;
	unsigned int n = 0;

	bool intro;
	bool done = FALSE;

	key_event ke;

	/* Paranoia */
	if (!mode) return;

	/* Clear the term if in a subwindow, set x otherwise */
	if (Term != angband_term[0])
	{
		clear_from(0);
		max = Term->hgt - 1;
	}
	else
	{
		max = Term->hgt - (show_sidebar ? 3 : 2);

		screen_save();
	}

	/* If hallucinating, we can't see any monsters */
	if (p_ptr->timed[TMD_IMAGE])
	{
		Term_putstr(0, 0, 36, TERM_ORANGE, "You're too confused to see straight! ");
		return;
	}

	/*
	 * Iterate through once to compute width, then second time to display
	 */
	for (forreal = 0; forreal < 2; forreal++)
	{
		/* Allocate the array */
		race_counts = C_ZNEW(z_info->r_max, u16b);
		sleep_counts = C_ZNEW(z_info->r_max, u16b);
		kind_counts = C_ZNEW(z_info->k_max, u16b);
		unknown_counts = C_ZNEW(z_info->k_max, u16b);
		artifact_counts = C_ZNEW(z_info->a_max, u16b);

		/* Reset some values */
		total_count = 0;
		disp_count = 0;
		line = row;
		intro = TRUE;

		/*
		 * Iterate multiple times. We put monsters we can project to in the first list, then monsters we can see,
		 * then monsters we are aware of through other means.
		 */
		if (mode & 1) for (i = 0; !done && i < 3; i++)
		{
			/* Reset status count */
			status_count = 0;
			max_sort = 0;

			/* Iterate over mon_list */
			for (idx = 1; idx < z_info->m_max; idx++)
			{
				m_ptr = &m_list[idx];

				/* Only visible monsters */
				if (!m_ptr->ml) continue;

				/* Check which type we're collecting */
				if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE))
				{
					if (i != 0) continue;
				}
				else if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_SEEN))
				{
					if (i != 1) continue;
				}
				else if ((i != 2) || (mode & 8)) continue;

				/* Bump the count for this race */
				race_counts[m_ptr->r_idx]++;
				if (m_ptr->csleep) sleep_counts[m_ptr->r_idx]++;
				total_count++;
				status_count++;

				/* Get maximum sort by */
				if (!op_ptr->monlist_sort_by) max_sort = MAX(max_sort, m_ptr->cdis);
				else if ((op_ptr->monlist_sort_by == 1) || (!l_list[m_ptr->r_idx].deaths))
				{
					max_sort = MAX(max_sort,r_info[m_ptr->r_idx].level);
				}
				else
				{
					max_sort = MAX(max_sort, l_list[m_ptr->r_idx].deaths + 60);
				}
			}

			/* Hack -- no ancestor deaths - demote to difficulty */
			if ((op_ptr->monlist_sort_by == 2) && (max_sort <= 60)) op_ptr->monlist_sort_by = 1;

			/* No visible monsters */
			if (!status_count) continue;

			/* Message */
			sprintf(buf, "You %s%s %d monster%s:%s",
				(i < 2) ? "can see" : "are aware of", (i == 1) ? " but not shoot" : "",
				status_count, (status_count > 1 ? "s" : ""),
				intro ? format(" (by %s)", sort_by_name[op_ptr->monlist_sort_by]) : "");

			if (forreal)
			{
				Term_putstr(0, line, strlen(buf), TERM_WHITE, buf);
				Term_erase(strlen(buf), line, width + 1 - strlen(buf));
			}
			else width = MAX(width, strlen(buf));

			/* Increase line number */
			line++;

			/* Iterate through sort */
			for (j = op_ptr->monlist_sort_by ? max_sort : 0; !done && (op_ptr->monlist_sort_by ? j >= 0 : j <= max_sort); op_ptr->monlist_sort_by ? j-- : j++)
			{
				/* Iterate over mon_list ( again :-/ ) */
				for (idx = 1; !done && idx < z_info->m_max && (line < max); idx++)
				{
					int attr;

					m_ptr = &m_list[idx];

					/* Colour text based on wakefulness */
					attr = sleep_counts[m_ptr->r_idx] == race_counts[m_ptr->r_idx] ? TERM_SLATE :
											(sleep_counts[m_ptr->r_idx] ? TERM_L_WHITE : TERM_WHITE);

					/* Only visible monsters */
					if (!m_ptr->ml) continue;

					/* Do each race only once */
					if (!race_counts[m_ptr->r_idx]) continue;

					/* Skip races at the sort distance */
					if (!op_ptr->monlist_sort_by && (m_ptr->cdis != j)) continue;
					else if ((op_ptr->monlist_sort_by == 2) && (l_list[m_ptr->r_idx].deaths && (l_list[m_ptr->r_idx].deaths != j - 60))) continue;
					else if ((op_ptr->monlist_sort_by) && (r_info[m_ptr->r_idx].level != j)) continue;

					/* Get monster race */
					r_ptr = &r_info[m_ptr->r_idx];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), idx, race_counts[m_ptr->r_idx] > 1 ? 0x300 : 0x208);

					/* Obtain the length of the description */
					n = strlen(m_name);

					/* Display multiple monsters */
					if (race_counts[m_ptr->r_idx] > 1)
					{
						/* Add race count */
						sprintf(buf, "%d", race_counts[m_ptr->r_idx]);

						if (forreal)
						{
							Term_putstr(0, line, strlen(buf), attr, buf);
							Term_addstr(-1, attr, " ");

							/* Display the entry itself */
							Term_addstr(-1, attr, m_name);
						}

						n+= strlen(buf) + 1;

						if ((sleep_counts[m_ptr->r_idx]) && (sleep_counts[m_ptr->r_idx] < race_counts[m_ptr->r_idx]))
						{
							/* Add race count */
							sprintf(buf, format(" (%d awake)", race_counts[m_ptr->r_idx] - sleep_counts[m_ptr->r_idx]));

							/* Display the entry itself */
							if (forreal) Term_addstr(-1, attr, buf);

							n += strlen(buf);
						}
					}
					/* Display single monsters */
					else if (forreal)
					{
						/* Display the entry itself */
						Term_putstr(0, line, n, attr, m_name);
					}


					/* Append the "standard" attr/char info */
					if (forreal)
					{
						Term_addstr(-1, attr, " ('");
						Term_addch(r_ptr->d_attr, r_ptr->d_char);
						Term_addstr(-1, attr, "')");
					}

					n += 6;

					/* Monster graphic on one line */
					if (!(use_dbltile) && !(use_trptile))
					{
						if (forreal)
						{
							/* Append the "optional" attr/char info */
							Term_addstr(-1, attr, "/('");

							Term_addch(r_ptr->x_attr, r_ptr->x_char);
						}

						if (use_bigtile)
						{
							if (forreal)
							{
								if (r_ptr->x_attr & 0x80)
									Term_addch(255, -1);
								else
									Term_addch(0, ' ');
							}

							n++;
						}

						if (forreal) Term_addstr(-1, attr, "')");
						n += 6;
					}

					/* Erase the rest of the line */
					if (forreal) Term_erase(n, line, width - n + 1);

					/* Add to monster counter */
					disp_count += race_counts[m_ptr->r_idx];

					/* Don't display again */
					race_counts[m_ptr->r_idx] = 0;
					sleep_counts[m_ptr->r_idx] = 0;

					/* Increase required width */
					width = MAX(width, n);

					/* Bump line counter */
					line++;

					/* Page wrap */
					if ((Term == angband_term[0]) && (line == max) && (disp_count != total_count) && forreal)
					{
						Term_putstr(0, line, width+1, TERM_WHITE, "-- more --");

						/* Get an acceptable keypress. */
						ke = force ? anykey() : inkey_ex();

						while ((ke.key == '\xff') && !(ke.mousebutton))
						{
							int y = KEY_GRID_Y(ke);
							int x = KEY_GRID_X(ke);

							int room = dun_room[p_ptr->py/BLOCK_HGT][p_ptr->px/BLOCK_WID];

							ke = target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
						}

						screen_load();

						/* Tried a command - avoid rest of list */
						if (ke.key != ' ')
						{
							done = TRUE;
							break;
						}

						screen_save();

						/* Reprint Message */
						sprintf(buf, "You %s%s %d monster%s:%s",
							(i < 2) ? "can see" : "are aware of", (i == 1) ? " but not shoot" : "",
							status_count, (status_count > 1 ? "s" : ""),
							intro ? format(" (by %s)", sort_by_name[op_ptr->monlist_sort_by]) : "");

						Term_putstr(0, row, strlen(buf), TERM_WHITE, buf);
						Term_erase(strlen(buf), row, width + 1 - strlen(buf));

						width = MAX(width, strlen(buf));

						/* Reset */
						line = row + 1;
					}
				}
			}

			/* Others to be displayed */
			if (!done && forreal)
			{
				/* Print "and others" message if we're out of space */
				if (disp_count != total_count)
				{
					sprintf(buf, format("  ...and %d others.", total_count - disp_count));
					Term_putstr(0, line, width+1, TERM_WHITE, buf);
					Term_erase(strlen(buf), line, width + 1 - strlen(buf));

					/* We've displayed the others */
					disp_count = total_count;
				}

				/* Put a shadow */
				else
					Term_erase(0, line, width + 1);

				/* Increase line number */
				line++;
			}

			/* Introduced? */
			intro = FALSE;
		}

		/* Display items */
		if (mode & 2) for (i = 0; !done && i < 2; i++)
		{
			/* Reset status count */
			status_count = 0;
			max_sort = 0;

			/* Iterate over item list */
			for (idx = 1; idx < z_info->o_max; idx++)
			{
				o_ptr = &o_list[idx];

				/* Only visible objects */
				if ((o_ptr->ident & (IDENT_MARKED)) == 0) continue;

				/* Only objects on the floor */
				if (o_ptr->held_m_idx) continue;

				/* Check which type we're collecting */
				if (play_info[o_ptr->iy][o_ptr->ix] & (PLAY_SEEN))
				{
					if (i != 0) continue;
				}
				else if ((i != 1) || (mode & 8)) continue;

				/* Bump the count for this artifact or kind */
				if ((object_named_p(o_ptr)) && (o_ptr->name1))
				{
					total_count++;
					status_count++;
					artifact_counts[o_ptr->name1]++;
				}
				else
				{
					kind_counts[o_ptr->k_idx] += o_ptr->number;
					total_count += o_ptr->number;
					status_count += o_ptr->number;
					if (!object_named_p(o_ptr) && !uninteresting_p(o_ptr)) unknown_counts[o_ptr->k_idx] += o_ptr->number;
				}

				/* Get maximum sort by */
				if (!op_ptr->monlist_sort_by) max_sort = MAX(max_sort, distance(p_ptr->py, p_ptr->px, o_ptr->iy, o_ptr->ix));
				else
				{
					max_sort = MAX(max_sort, !(object_named_p(o_ptr)) ? 60 + o_ptr->tval : k_info[o_ptr->k_idx].level);
				}
			}

			/* Nothing */
			if (!status_count) continue;

			/* Message */
			sprintf(buf, "You %s %d object%s:%s",
				i ? "are aware of" : "can see",
				status_count, (status_count > 1 ? "s" : ""),
				intro ? format(" (by %s)", sort_by_name[op_ptr->monlist_sort_by]) : "");

			if (forreal)
			{
				Term_putstr(0, line, strlen(buf), TERM_WHITE, buf);
				Term_erase(strlen(buf), line, width + 1 - strlen(buf));
			}
			else width = MAX(width, strlen(buf));

			/* Increase line number */
			line++;

			/* Iterate through sort */
			for (j = op_ptr->monlist_sort_by ? max_sort : 0; !done && (op_ptr->monlist_sort_by ? j >= 0 : j <= max_sort); op_ptr->monlist_sort_by ? j-- : j++)
			{
				/* Iterate over object_list ( again :-/ ) */
				for (idx = 1; !done && (idx < z_info->o_max) && (line < max); idx++)
				{
					int attr;
					object_type object_type_body;

					o_ptr = &o_list[idx];

					/* Colour text based on knowledge */
					attr = unknown_counts[o_ptr->k_idx] == kind_counts[o_ptr->k_idx] ? TERM_WHITE :
											(unknown_counts[o_ptr->k_idx] ? TERM_L_WHITE : TERM_SLATE);

					/* Only visible objects */
					if ((o_ptr->ident & (IDENT_MARKED)) == 0) continue;

					/* Only objects on the floor */
					if (o_ptr->held_m_idx) continue;

					/* Have we seen the artifact */
					if (object_named_p(o_ptr) && o_ptr->name1 && !artifact_counts[o_ptr->name1]) continue;

					/* Do each race only once */
					else if (!kind_counts[o_ptr->k_idx]) continue;

					/* Check whether we group object */
					if (!op_ptr->monlist_sort_by)
					{
						if (j != distance(p_ptr->py, p_ptr->px, o_ptr->iy, o_ptr->ix)) continue;
					}
					else if ((op_ptr->monlist_sort_by) && !(object_named_p(o_ptr)))
					{
						if (j != 60 + o_ptr->tval) continue;
					}
					else if (op_ptr->monlist_sort_by)
					{
						if (j != k_info[o_ptr->k_idx].level) continue;
					}

					/* Prepare a fake object */
					object_prep(&object_type_body, o_ptr->k_idx);

					/* Fake the artifact */
					if (object_named_p(o_ptr) && o_ptr->name1)
					{
						object_type_body.name1 = o_ptr->name1;
						attr = TERM_YELLOW;
					}

					/* Fake the number */
					else
					{
						if (kind_counts[o_ptr->k_idx] > 99) object_type_body.number = 99;
						else object_type_body.number = kind_counts[o_ptr->k_idx];
					}

					/* Describe the object */
					object_desc(buf, sizeof(buf), &object_type_body, TRUE, 0);

					if (forreal)
					{
						Term_putstr(0, line, strlen(buf), attr, buf);
					}

					n= strlen(buf);

					/* Show unknown number of kinds */
					if ((!object_named_p(o_ptr) || !o_ptr->name1) &&
							(unknown_counts[o_ptr->k_idx]) && (unknown_counts[o_ptr->k_idx] < kind_counts[o_ptr->k_idx]))
					{
						/* Add race count */
						sprintf(buf, format(" (%d unknown)", unknown_counts[o_ptr->k_idx]));

						/* Display the entry itself */
						if (forreal) Term_addstr(-1, attr, buf);

						n += strlen(buf);
					}

					/* Append the "standard" attr/char info */
					if (forreal)
					{
						Term_addstr(-1, attr, " ('");
						Term_addch(k_info[o_ptr->k_idx].flavor ? TERM_WHITE : k_info[o_ptr->k_idx].d_attr, k_info[o_ptr->k_idx].d_char);
						Term_addstr(-1, attr, "')");
					}

					n += 6;

					/* Monster graphic on one line */
					if (!(use_dbltile) && !(use_trptile))
					{
						if (forreal)
						{
							/* Append the "optional" attr/char info */
							Term_addstr(-1, attr, "/('");

							Term_addch(object_attr(o_ptr), object_char(o_ptr));
						}

						if (use_bigtile)
						{
							if (forreal)
							{
								if (object_attr(o_ptr) & 0x80)
									Term_addch(255, -1);
								else
									Term_addch(0, ' ');
							}

							n++;
						}

						if (forreal) Term_addstr(-1, attr, "')");
						n += 6;
					}

					/* Erase the rest of the line */
					if (forreal) Term_erase(n, line, width - n + 1);

					/* Visible artifact */
					if (object_named_p(o_ptr) && o_ptr->name1)
					{
						disp_count++;
						artifact_counts[o_ptr->name1] = 0;
					}
					else
					{
						/* Add to monster counter */
						disp_count += kind_counts[o_ptr->k_idx];

						/* Don't display again */
						kind_counts[o_ptr->k_idx] = 0;
						unknown_counts[o_ptr->k_idx] = 0;
					}

					/* Increase required width */
					width = MAX(width, n);

					/* Bump line counter */
					line++;

					/* Page wrap */
					if ((Term == angband_term[0]) && (line == max) && (disp_count != total_count) && forreal)
					{
						Term_putstr(0, line, width+1, TERM_WHITE, "-- more --");

						/* Get an acceptable keypress. */
						ke = force ? anykey() : inkey_ex();

						while ((ke.key == '\xff') && !(ke.mousebutton))
						{
							int y = KEY_GRID_Y(ke);
							int x = KEY_GRID_X(ke);

							int room = dun_room[p_ptr->py/BLOCK_HGT][p_ptr->px/BLOCK_WID];

							ke = target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
						}

						screen_load();

						/* Tried a command - avoid rest of list */
						if (ke.key != ' ')
						{
							done = TRUE;
							break;
						}

						screen_save();

						/* Reprint Message */
						sprintf(buf, "You %s %d object%s:%s",
							i ? "are aware of" : "can see",
							status_count, (status_count > 1 ? "s" : ""),
							intro ? format(" (by %s)", sort_by_name[op_ptr->monlist_sort_by]) : "");

						Term_putstr(0, row, strlen(buf), TERM_WHITE, buf);
						Term_erase(strlen(buf), row, width + 1 - strlen(buf));

						width = MAX(width, strlen(buf));

						/* Reset */
						line = row + 1;
					}
				}

			}


			/* Others to be displayed */
			if (!done && forreal)
			{
				/* Print "and others" message if we're out of space */
				if (disp_count != total_count)
				{
					sprintf(buf, format("  ...and %d others.", total_count - disp_count));
					Term_putstr(0, line, width+1, TERM_WHITE, buf);
					Term_erase(strlen(buf), line, width + 1 - strlen(buf));

					/* We've displayed the others */
					disp_count = total_count;
				}

				/* Put a shadow */
				else
					Term_erase(0, line, width + 1);

				/* Increase line number */
				line++;
			}

			/* Introduced? */
			intro = FALSE;
		}

		/* If displaying on the terminal using the '[' command, recenter on the player,
		 * taking away the used up width on the left hand side. Otherwise, recenter when
		 * the player walks next to the display box. */
		if (!forreal && (Term == angband_term[0]) && ((signed)width < SCREEN_WID) &&
				(((force) && !(center_player)) ||
				((p_ptr->px - p_ptr->wx <= (signed)width + 1) && (p_ptr->py - p_ptr->wy <= (signed)line + 1))))
		{
			screen_load();

			/* Use "modify_panel" */
			if (modify_panel(p_ptr->py - (SCREEN_HGT) / 2, p_ptr->px - (SCREEN_WID + width) / 2))
			{
				/* Force redraw */
				redraw_stuff();
			}

			/* Unable to place the player */
			if ((!force) && (p_ptr->px - p_ptr->wx <= (signed)width + 1) && (p_ptr->py - p_ptr->wy <= (signed)7)) return;

			if ((!force) && (p_ptr->py - p_ptr->wy <= (signed)line + 1)) max = p_ptr->py - p_ptr->wy - 1;

			screen_save();
		}

		/* Free the race counters */
		FREE(race_counts);
		FREE(sleep_counts);

		/* Free the object counters */
		FREE(kind_counts);
		FREE(unknown_counts);
		FREE(artifact_counts);
	}

	/* Reload the screen if we got to end of the list */
	if (!done)
	{
		if (!total_count)
		{
			if (force)
			{
				prt(format("You see no %s%s%s. (by %s)",
						mode % 2 ? "monsters" : "",
						mode == 3 ? " or " : "",
						mode / 2 ? "objects" : "",
						sort_by_name[op_ptr->monlist_sort_by]), 0, 0);
			}
			else
			{
				screen_load();
				return;
			}
		}

		if (Term == angband_term[0])
		{
			/* Get an acceptable keypress. */
			ke = force ? anykey() : inkey_ex();

			while ((ke.key == '\xff') && !(ke.mousebutton))
			{
				int y = KEY_GRID_Y(ke);
				int x = KEY_GRID_X(ke);

				int room = dun_room[p_ptr->py/BLOCK_HGT][p_ptr->px/BLOCK_WID];

				ke = target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
			}

			screen_load();
		}
	}

	/* Display command prompt */
	if (command)
	{
		/*Term_putstr(0, 0, -1, TERM_WHITE, "Command:");*/

		/* Requeue command just pressed */
		p_ptr->command_new = ke;

		/* Hack -- Process "Escape"/"Spacebar"/"Return" */
		if ((p_ptr->command_new.key == ESCAPE) ||
			/*(p_ptr->command_new.key == ' ') ||*/
			(p_ptr->command_new.key == '\r') ||
			(p_ptr->command_new.key == '\n'))
		{
			/* Reset stuff */
			p_ptr->command_new.key = 0;
		}
	}
}




/*
 * Build a string describing a monster race in some way
 * 
 * We correctly handle masculine, feminine and pluralisation.
 * Note the mode flags are designed for compatibility with
 * monster_desc below, which is why there are duplicates.
 * 
 * Mode Flags:
 *   0x01 --> Unused
 *   0x02 --> Unused
 *   0x04 --> Unused
 *   0x08 --> Use indefinites for monsters ("a goblin" or "16 goblins")
 *   0x10 --> Unused
 *   0x20 --> Unused
 *   0x40 --> Unused
 *   0x80 --> Unused
 *   0x100 --> Unused
 *   0x200 --> Unused
 *   0x400 --> Describe all possible monsters ("the goblin priest or goblin priestess")
 *   0x800 --> Describe without an article ("goblins")
 *
 * Useful Modes:
 *   0x88 --> Killing name ("a goblin")
 *   0x400 --> Recall name ("goblin priest or goblin priestess")
 */
void race_desc(char *desc, size_t max, int r_idx, int mode, int number)
{
	monster_race *r_ptr = &r_info[r_idx];

	cptr name = (r_name + r_ptr->name);

	int state = 0;
	
	/* Assume male - primarily to prevent descration of female corpses */
	int match = 1;
	char *s, *t;

	bool append_s = number != 1;

	/* Extract pluralized if requested */
	if (number != 1) match = 3;
	
	/* Hack -- we try to allow pluralisation of male and female genders */
	if (mode & (0x400))
	{
		bool differs = FALSE;
		
		/* Check if male and female description identical */
		for (t = s = desc; *s; s++)
		{
			if ((state == 2) && (*s != *t))
			{
				differs = TRUE;
				break;
			}
			
			if (state != 1) t++;
			
			if (*s == '|')
			{
				state++;
				if (state == 4) state = 0;
			}
		}
		
		/* We have a difference */
		if (differs)
		{
			/* Display "goblin priest or goblin priestess" */
			match = 1;
		}
		else
		{
			/* Display "mice" */
			mode &= ~(0x400);
		}
		
		/* Restart state */
		state = 0;
	}

	/* It could be a Unique */
	if (r_ptr->flags1 & (RF1_UNIQUE))
	{
		/* Start with the name (thus nominative and objective) */
		my_strcpy(desc, name, max);
		
		/* Only one possible unique */
		mode &= ~(0x400);
	}

	/* It could be an indefinite monster */
	else
	{
		/* XXX Check plurality for "some" */
		if (mode & 0x100) my_strcpy(desc, "", max);

		/* Indefinite monsters need an indefinite article */
		else if (mode & 0x08) my_strcpy(desc, !number ? "no " : (number != 1 ? format("%d ", number) :
			(is_a_vowel(name[0]) ? "an " : "a ")), max);

		/* It could be a normal, definite, monster */
		else if ((mode & 0x800) == 0) my_strcpy(desc, !number ? "no " : (number != 1 ? format("the %d ", number) : "the "), max);
		
		/* Clear string */
		else my_strcpy(desc, "", max);
		
		/* Otherwise */
		my_strcat(desc, name, max);
	}

	/* Remove gender sensitivity */
	for (t = s = desc; *s; s++)
	{
		if (*s == '|')
		{
			state++;
			if (state == 4) state = 0;
			append_s = FALSE;
		}
		else if (!state || (state == match))
		{
			*t++ = *s;
		}
	}

	/* Terminate buffer */
	*t = '\0';

	/* Pluralize */
	if ((number != 1) && (((mode & (0x400)) != 0) || (append_s)))
	{
		/* We're hacking pluralisation - check for final s */
		if (mode & (0x400))
		{
			if ((desc[strlen(desc) - 1] == 's') || 
				(desc[strlen(desc) - 1] == 'x') ||
				((desc[strlen(desc) - 1] == 'h') &&
					(desc[strlen(desc) - 2] == 'c')))
			{					
				/* Hack -- add an 'e' for final 's', 'x' or 'ch' */
				my_strcat(desc, "e", max);
			}
		}
		
		/* Simply append "s" for plural */
		my_strcat(desc, "s", max);
	}
	
	/* Describe feminine as well if requested */
	if (mode & 0x400)
	{
		/* Add 'and' */
		if (number > 1)
		{
			my_strcat(desc, " and ", max);
		}
		/* Add 'or */
		else
		{
			my_strcat(desc, " or ", max);
		}
		
		/* Otherwise */
		my_strcat(desc, name, max);
		
		/* Describe feminine */
		state = 0;
		match = 2;
		append_s = TRUE;

		/* Remove gender sensitivity */
		for (t = s = desc; *s; s++)
		{
			if (*s == '|')
			{
				state++;
				if (state == 4) state = 0;
				append_s = FALSE;
			}
			else if (!state || (state == match))
			{
				*t++ = *s;
			}
		}

		/* Terminate buffer */
		*t = '\0';
		
		/* Pluralize */
		if ((number != 1) && (((mode & (0x400)) != 0) || (append_s)))
		{
			/* We're hacking pluralisation - check for final s */
			if (mode & (0x400))
			{
				if ((desc[strlen(desc) - 1] == 's') || 
					(desc[strlen(desc) - 1] == 'x') ||
					((desc[strlen(desc) - 1] == 'h') &&
						(desc[strlen(desc) - 2] == 'c')))
				{					
					/* Hack -- add an 'e' for final 's', 'x' or 'ch' */
					my_strcat(desc, "e", max);
				}
			}
			
			/* Simply append "s" for plural */
			my_strcat(desc, "s", max);
		}
	}
}



/*
 * Build a string describing a monster in some way.
 *
 * We can correctly describe monsters based on their visibility.
 * We can force all monsters to be treated as visible or invisible.
 * We can build nominatives, objectives, possessives, or reflexives.
 * We can selectively pronominalize hidden, visible, or all monsters.
 * We can use definite or indefinite descriptions for hidden monsters.
 * We can use definite or indefinite descriptions for visible monsters.
 *
 * Pronominalization involves the gender whenever possible and allowed,
 * so that by cleverly requesting pronominalization / visibility, you
 * can get messages like "You hit someone.  She screams in agony!".
 *
 * Reflexives are acquired by requesting Objective plus Possessive.
 *
 * I am assuming that no monster name is more than 65 characters long,
 * so that "char desc[80];" is sufficiently large for any result, even
 * when the "offscreen" notation is added.
 *
 * Note that the "possessive" for certain unique monsters will look
 * really silly, as in "Morgoth, King of Darkness's".  We should
 * perhaps add a flag to "remove" any "descriptives" in the name.
 *
 * Note that "offscreen" monsters will get a special "(offscreen)"
 * notation in their name if they are visible but offscreen.  This
 * may look silly with possessives, as in "the rat's (offscreen)".
 * Perhaps the "offscreen" descriptor should be abbreviated.
 *
 * Mode Flags:
 *   0x01 --> Objective (or Reflexive)
 *   0x02 --> Possessive (or Reflexive)
 *   0x04 --> Use indefinites for hidden monsters ("something")
 *   0x08 --> Use indefinites for visible monsters ("a goblin")
 *   0x10 --> Pronominalize hidden monsters
 *   0x20 --> Pronominalize visible monsters
 *   0x40 --> Assume the monster is hidden
 *   0x80 --> Assume the monster is visible
 *   0x100 --> Pluralize the monster
 *   0x200 --> No special suffixes
 *
 * Useful Modes:
 *   0x00 --> Full nominative name ("the goblin") or "it"
 *   0x04 --> Full nominative name ("the goblin") or "something"
 *   0x80 --> Banishment resistance name ("the goblin")
 *   0x88 --> Killing name ("a goblin")
 *   0x22 --> Possessive, genderized if visable ("his") or "its"
 *   0x23 --> Reflexive, genderized if visable ("himself") or "itself"
 */
void monster_desc(char *desc, size_t max, int m_idx, int mode)
{
	cptr res;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	cptr name = (r_name + r_ptr->name);

	bool seen, pron;

	int state = 0;
	int match = 1;
	char *s, *t;

	bool append_s = TRUE;

	/* Can we "see" it (forced, or not hidden + visible) */
	seen = ((mode & (0x80)) || (!(mode & (0x40)) && m_ptr->ml));

	/* Sexed Pronouns (seen and forced, or unseen and allowed) */
	pron = ((seen && (mode & (0x20))) || (!seen && (mode & (0x10))));

	/* Extract the gender if female */
	if ((r_ptr->flags1 & (RF1_FEMALE)) && (!(r_ptr->flags1 & (RF1_MALE)) || (m_idx % 2))) match = 2;

	/* Extract pluralized if requested */
	if (mode & (0x100)) match = 3;

	/* First, try using pronouns, or describing hidden monsters */
	if (!seen || pron)
	{
		/* Two encodings of the monster "sex" */
		int kind = 0x00;

		/* Extract the gender (if applicable) */
		if ((r_ptr->flags1 & (RF1_FEMALE)) && (!(r_ptr->flags1 & (RF1_MALE)) || (m_idx % 2))) kind = 0x20;
		else if ((r_ptr->flags1 & (RF1_MALE)) && (!(r_ptr->flags1 & (RF1_FEMALE)) || !(m_idx % 2)))  kind = 0x10;

		/* Ignore the gender (if desired) */
		if (!m_ptr || !pron) kind = 0x00;


		/* Assume simple result */
		res = "it";

		/* Brute force: split on the possibilities */
		switch (kind + (mode & 0x07))
		{
			/* Neuter, or unknown */
			case 0x00: res = "it"; break;
			case 0x01: res = "it"; break;
			case 0x02: res = "its"; break;
			case 0x03: res = "itself"; break;
			case 0x04: res = "something"; break;
			case 0x05: res = "something"; break;
			case 0x06: res = "something's"; break;
			case 0x07: res = "itself"; break;

			/* Male (assume human if vague) */
			case 0x10: res = "he"; break;
			case 0x11: res = "him"; break;
			case 0x12: res = "his"; break;
			case 0x13: res = "himself"; break;
			case 0x14: res = "someone"; break;
			case 0x15: res = "someone"; break;
			case 0x16: res = "someone's"; break;
			case 0x17: res = "himself"; break;

			/* Female (assume human if vague) */
			case 0x20: res = "she"; break;
			case 0x21: res = "her"; break;
			case 0x22: res = "her"; break;
			case 0x23: res = "herself"; break;
			case 0x24: res = "someone"; break;
			case 0x25: res = "someone"; break;
			case 0x26: res = "someone's"; break;
			case 0x27: res = "herself"; break;
		}

		/* Copy the result */
		my_strcpy(desc, res, max);
	}

	/* Handle visible monsters, "reflexive" request */
	else if ((mode & 0x02) && (mode & 0x01))
	{
		/* The monster is visible, so use its gender */
		if ((r_ptr->flags1 & (RF1_FEMALE)) && (!(r_ptr->flags1 & (RF1_MALE)) || (m_idx % 2))) my_strcpy(desc, "herself", max);
		else if ((r_ptr->flags1 & (RF1_MALE)) && (!(r_ptr->flags1 & (RF1_FEMALE)) || !(m_idx % 2))) my_strcpy(desc, "himself", max);
		else my_strcpy(desc, "itself", max);
	}

	/* Handle all other visible monster requests */
	else
	{
		monster_race monster_race_scaled;
		cptr prefix = NULL, suffix = NULL, infix = NULL;
		int level = r_ptr->level;

		/* Hack - scale up a leader */
		if (m_ptr->mflag & (MFLAG_LEADER)) level -= 5;

		/* Scale a monster */
		if (((r_ptr->flags9 & RF9_LEVEL_MASK) != 0) &&
			monster_scale(&monster_race_scaled, m_idx, p_ptr->depth))
		{
			r_ptr = &monster_race_scaled;
		}

		/* Add a class suffix to some monsters */
		if (/* r_ptr->flags9 & (RF9_LEVEL_CLASS) */ FALSE)
		{
			if (((r_ptr->flags2 & (RF2_MAGE)) != 0) && ((r_ptr->flags2 & (RF2_PRIEST)) != 0)) suffix = "shaman";
			else if (((r_ptr->flags2 & (RF2_MAGE)) != 0) && ((r_ptr->flags2 & (RF2_ARCHER)) != 0)) suffix = "ranger";
			else if (((r_ptr->flags2 & (RF2_MAGE)) != 0) && ((r_ptr->flags2 & (RF2_ARMOR)) != 0)) suffix = "warrior mage";
			else if ((r_ptr->flags2 & (RF2_MAGE)) != 0) suffix = "mage";
			else if (((r_ptr->flags2 & (RF2_PRIEST)) != 0) && ((r_ptr->flags2 & (RF2_ARMOR)) != 0)) suffix = "|knight|princess|knights|";
			else if ((r_ptr->flags2 & (RF2_PRIEST)) != 0) suffix = "priest||ess|s|";
			else if ((r_ptr->flags2 & (RF2_ARCHER)) != 0) suffix = "archer";
			else if ((r_ptr->flags2 & (RF2_SNEAKY)) != 0) suffix = "scout";
			else if ((r_ptr->flags2 & (RF2_ARMOR)) != 0) suffix = "warrior";

			/* Hack -- reduce rank */
			if (suffix) level += 5;
		}

		/* Add prefixes to levelled monsters */
		if (r_ptr->flags9 & (RF9_LEVEL_AGE))
		{
			if (p_ptr->depth >= level + 15) prefix = "Ancient ";
			else if (p_ptr->depth >= level + 10) prefix = "Old ";
			else if (p_ptr->depth >= level + 5) prefix = "Mature ";
			else prefix = "Young ";
		}
		if (r_ptr->flags9 & (RF9_LEVEL_SIZE))
		{
			if (p_ptr->depth >= level + 15) prefix = "Giant ";
			else if (p_ptr->depth >= level + 10) prefix = "Huge ";
			else if (p_ptr->depth >= level + 5) prefix = "Large ";
		}
		else if (r_ptr->flags9 & (RF9_LEVEL_SPEED))
		{
			if (p_ptr->depth >= level + 15) prefix = "Deadly ";
			else if (p_ptr->depth >= level + 10) prefix = "Furious ";
			else if (p_ptr->depth >= level + 5) prefix = "Fast ";
		}
		else if (r_ptr->flags9 & (RF9_LEVEL_POWER))
		{
			/* Lesser / greater / master */
			if (((r_ptr->flags3 & (RF3_UNDEAD | RF3_DRAGON | RF3_DEMON | RF3_ANIMAL | RF3_PLANT | RF3_INSECT)) != 0) ||
				(((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT)) == 0) &&
				((r_ptr->flags9 & (RF9_MAN | RF9_ELF | RF9_DWARF)) == 0)))
			{
				if (p_ptr->depth >= level + 15) prefix = "Master ";
				else if (p_ptr->depth >= level + 10) prefix = "Greater ";
				else prefix = "Lesser ";
			}
			else
			{
				/* Powerful monster ranks */
				if (p_ptr->depth >= level + 15)
				{
					if (((r_ptr->flags2 & (RF2_ARMOR)) != 0) ||
						((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT)) != 0))
							prefix = "Elite ";
					else if (r_ptr->flags2 & (RF2_MAGE | RF2_PRIEST)) prefix = "Arch ";
					else prefix = "Master ";
				}
				/* Moderately powerful monster ranks */
				else if (p_ptr->depth >= level + 10)
				{
					if (((r_ptr->flags2 & (RF2_ARMOR)) != 0) ||
						((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT)) != 0))
							prefix = "Veteran ";
					else prefix = "Senior ";
				}
				/* Somewhat powerful monster ranks */
				else if (p_ptr->depth >= level + 5)
				{
					if (((r_ptr->flags2 & (RF2_ARMOR)) != 0) ||
						((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT)) != 0))
							prefix = "Hardened ";
					else if (r_ptr->flags2 & (RF2_MAGE)) prefix = "Wily ";
					else if (r_ptr->flags2 & (RF2_PRIEST)) prefix = "Superior ";
					else prefix = "Experienced ";
				}
			}
		}

		/* Add prefixes to non-evil warrior allies in battle. */
		if ((level_flag & (LF1_BATTLE)) && (m_ptr->mflag & (MFLAG_ALLY)) &&
				(r_ptr->d_char == 't') && ((r_ptr->flags3 & (RF3_EVIL)) ==0))
		{
			/* Use current home if a monstrous race */
			if (rp_ptr->r_idx)
			{
				infix = t_name + t_info[p_ptr->town].name;
			}
			/* Use adjective if available */
			else if (strlen(p_text + rp_ptr->text))
			{
				infix = p_text + rp_ptr->text;
			}
			/* Otherwise use race name */
			else
			{
				infix = p_name + rp_ptr->name;
			}
		}

		/* It could be a Unique */
		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			/* Start with the name (thus nominative and objective) */
			my_strcpy(desc, name, max);
		}

		/* It could be an indefinite monster */
		else
		{
			/* XXX Check plurality for "some" */
			if (mode & 0x100) my_strcpy(desc, "", max);

			/* Indefinite monsters need an indefinite article */
			else if (mode & 0x08) my_strcpy(desc, is_a_vowel(prefix ? prefix[0] : name[0]) ? "an " : "a ", max);

			/* It could be a normal, definite, monster */
			else my_strcpy(desc, "the ", max);

			/* Otherwise */
			if (prefix) my_strcat(desc, prefix, max);
			if (infix) { my_strcat(desc, infix, max); my_strcat(desc, " ", max); }
			my_strcat(desc, name, max);
			if (suffix) { my_strcat(desc, " ", max); my_strcat(desc, suffix, max); }
		}

		/* Remove gender sensitivity */
		for (t = s = desc; *s; s++)
		{
			if (*s == '|')
			{
				state++;
				if (state == 4) state = 0;
				append_s = FALSE;
			}
			else if (!state || (state == match))
			{
				*t++ = *s;
			}
		}

		/* Terminate buffer */
		*t = '\0';

		/* Pluralize */
		if ((append_s) && ((mode & (0x100)) != 0))
		{
			/* Simply append "s" for plural */
			my_strcat(desc, "s", max);
		}

		/* Handle the Possessive as a special afterthought */
		if (mode & 0x02)
		{
			/* XXX Check for trailing "s" */

			/* Simply append "apostrophe" and "s" */
			my_strcat(desc, "'s", max);
		}

		/* Don't affix special comments if requested */
		if (mode & 0x200) return;

		/* Mention "hidden" monsters XXX XXX */
		/* Note we only see "hidden" monsters with detection,
		   or telepathy, and this is different to non-visible
		   monsters handled above.
		   We need to warn players because otherwise they
		   will try and target these monsters.*/

		/* XXX Perhaps we should use a different attr/char */
		if (m_ptr->mflag & (MFLAG_ALLY))
		{
			/* Append special notation */
			if (m_ptr->mflag & (MFLAG_TOWN)) my_strcat(desc, " (hired)", max);
			else my_strcat(desc, " (allied)", max);
		}

		/* XXX Perhaps we should use a different attr/char */
		else if (m_ptr->mflag & (MFLAG_TOWN))
		{
			/* Append special notation */
			if (r_ptr->flags9 & (RF9_TOWNSFOLK)) /* Do nothing */;
			else if (m_ptr->mflag & (MFLAG_AGGR)) my_strcat(desc, " (offended)", max);
			else my_strcat(desc, " (neutral)", max);
		}

		/* XXX Perhaps we should use a different attr/char */
		if (m_ptr->mflag & (MFLAG_HIDE))
		{
			/* Append special notation */
			my_strcat(desc, " (hidden)", max);
		}

		/* Mention "offscreen" monsters XXX XXX */
		if (!panel_contains(m_ptr->fy, m_ptr->fx))
		{
			/* Append special notation */
			my_strcat(desc, " (offscreen)", max);
		}

		/* Show additional flags */
		if (cheat_xtra)
		{
			if (m_ptr->mflag & (MFLAG_AGGR))
			{
				/* Append special notation */
				my_strcat(desc, " (aggro)", max);
			}

			if (m_ptr->mflag & (MFLAG_IGNORE))
			{
				/* Append special notation */
				my_strcat(desc, " (ignore)", max);
			}

			if (m_ptr->ty && m_ptr->tx)
			{
				/* Append special notation */
				my_strcat(desc, format(" (t:%d, %d)", m_ptr->ty - m_ptr->fy, m_ptr->tx - m_ptr->fx) , max);
			}

			if (m_ptr->min_range)
			{
				/* Append special notation */
				my_strcat(desc, format(" (mr %d, br %d)", m_ptr->min_range, m_ptr->best_range) , max);
			}
		}

		/* Show additional info */
		if (cheat_hear)
		{
			if (m_ptr->hp < m_ptr->maxhp)
			{
				/* Append special notation */
				my_strcat(desc, format(" (hp:%d/%d)", m_ptr->hp, m_ptr->maxhp) , max);
			}
			else
			{
				/* Append special notation */
				my_strcat(desc, format(" (hp:%d)", m_ptr->maxhp) , max);
			}

			if (m_ptr->mana < r_ptr->mana)
			{
				/* Append special notation */
				my_strcat(desc, format(" (sp:%d/%d)", m_ptr->mana, r_ptr->mana) , max);
			}
			else if (r_ptr->mana)
			{
				/* Append special notation */
				my_strcat(desc, format(" (sp:%d)", r_ptr->mana) , max);
			}
		}
	}
}


/*
 * Learn about a monster (by "probing" it)
 */
void lore_do_probe(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[r_idx];

	char race_name[80];

	/* Hack -- Memorize some flags */
	l_ptr->flags1 = r_ptr->flags1;
	l_ptr->flags2 = r_ptr->flags2;
	l_ptr->flags3 = r_ptr->flags3;
	/* Skip spells */
	l_ptr->flags7 = r_ptr->flags7;
	l_ptr->flags8 = r_ptr->flags8;
	l_ptr->flags9 = r_ptr->flags9;

	/* Update monster recall window */
	if (p_ptr->monster_race_idx == r_idx)
	{
		/* Window stuff */
		p_ptr->window |= (PW_MONSTER);
	}

	/* Describe the race */
	race_desc(race_name, sizeof(race_name), r_idx, 0x400, 1);

	msg_format("You learn information about %s.", race_name);
}


/*
 * Take note that the given monster just dropped some treasure
 *
 * Note that learning the "GOOD"/"GREAT" flags gives information
 * about the treasure (even when the monster is killed for the first
 * time, such as uniques, and the treasure has not been examined yet).
 *
 * This "indirect" method is used to prevent the player from learning
 * exactly how much treasure a monster can drop from observing only
 * a single example of a drop.  This method actually observes how much
 * gold and items are dropped, and remembers that information to be
 * described later by the monster recall code.
 */
void lore_treasure(int m_idx, int num_item, int num_gold)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];


	/* Note the number of things dropped */
	if (num_item > l_ptr->drop_item) l_ptr->drop_item = num_item;
	if (num_gold > l_ptr->drop_gold) l_ptr->drop_gold = num_gold;

	/* Hack -- memorize the good/great flags */
	if (r_ptr->flags1 & (RF1_DROP_GOOD)) l_ptr->flags1 |= (RF1_DROP_GOOD);
	if (r_ptr->flags1 & (RF1_DROP_GREAT)) l_ptr->flags1 |= (RF1_DROP_GREAT);

	/* Update monster recall window */
	if (p_ptr->monster_race_idx == m_ptr->r_idx)
	{
		/* Window stuff */
		p_ptr->window |= (PW_MONSTER);
	}
}



/*
 * This function updates the monster record of the given monster
 *
 * This involves extracting the distance to the player (if requested),
 * and then checking for visibility (natural, infravision, see-invis,
 * telepathy), updating the monster visibility flag, redrawing (or
 * erasing) the monster when its visibility changes, and taking note
 * of any interesting monster flags (cold-blooded, invisible, etc).
 *
 * Note the new "mflag" field which encodes several monster state flags,
 * including "view" for when the monster is currently in line of sight,
 * and "mark" for when the monster is currently visible via detection.
 *
 * The only monster fields that are changed here are "cdis" (the
 * distance from the player), "ml" (visible to the player), and
 * "mflag" (to maintain the "MFLAG_VIEW" flag).
 *
 * Note the special "update_monsters()" function which can be used to
 * call this function once for every monster.
 *
 * Note the "full" flag which requests that the "cdis" field be updated,
 * this is only needed when the monster (or the player) has moved.
 *
 * Every time a monster moves, we must call this function for that
 * monster, and update the distance, and the visibility.  Every time
 * the player moves, we must call this function for every monster, and
 * update the distance, and the visibility.  Whenever the player "state"
 * changes in certain ways ("blindness", "infravision", "telepathy",
 * and "see invisible"), we must call this function for every monster,
 * and update the visibility.
 *
 * Routines that change the "illumination" of a grid must also call this
 * function for any monster in that grid, since the "visibility" of some
 * monsters may be based on the illumination of their grid.
 *
 * Note that this function is called once per monster every time the
 * player moves.  When the player is running, this function is one
 * of the primary bottlenecks, along with "update_view()" and the
 * "process_monsters()" code, so efficiency is important.
 *
 * Note the optimized "inline" version of the "distance()" function.
 *
 * A monster is "visible" to the player if (1) it has been detected
 * by the player, (2) it is close to the player and the player has
 * telepathy, or (3) it is close to the player, and in line of sight
 * of the player, and it is "illuminated" by some combination of
 * infravision, torch light, or permanent light (invisible monsters
 * are only affected by "light" if the player can see invisible), and
 * it is not hidden in a feature somehow.
 *
 * Monsters which are not on the current panel may be "visible" to
 * the player, and their descriptions will include an "offscreen"
 * reference.  Currently, offscreen monsters cannot be targetted
 * or viewed directly, but old targets will remain set.  XXX XXX
 *
 * The player can choose to be disturbed by several things, including
 * "disturb_move" (monster which is viewable moves in some way), and
 * "disturb_near" (monster which is "easily" viewable moves in some
 * way).  Note that "moves" includes "appears" and "disappears".
 */
void update_mon(int m_idx, bool full)
{
	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int d;

	/* Current location */
	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	/* Seen at all */
	bool flag = FALSE;

	/* Seen by vision */
	bool easy = FALSE;

	/* Compute distance */
	if (full)
	{
		int py = p_ptr->py;
		int px = p_ptr->px;

		/* Distance components */
		int dy = (py > fy) ? (py - fy) : (fy - py);
		int dx = (px > fx) ? (px - fx) : (fx - px);

		/* Approximate distance */
		d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

		/* Restrict distance */
		if (d > 255) d = 255;

		/* Save the distance */
		m_ptr->cdis = d;
	}

	/* Extract distance */
	else
	{
		/* Extract the distance */
		d = m_ptr->cdis;
	}

	/* Detected */
	if (m_ptr->mflag & (MFLAG_MARK)) flag = TRUE;


	/* Nearby */
	if (d <= MAX_SIGHT)
	{
		/* Basic telepathy */
		if ((p_ptr->cur_flags3 & (TR3_TELEPATHY)) != 0)
		{
			/* Empty mind, no telepathy */
			if (r_ptr->flags2 & (RF2_EMPTY_MIND))
			{
				/* Memorize flags */
				l_ptr->flags2 |= (RF2_EMPTY_MIND);
			}

			/* Weird mind, occasional telepathy */
			else if (r_ptr->flags2 & (RF2_WEIRD_MIND))
			{
				/* One in ten individuals are detectable */
				if ((m_idx % 10) == 5)
				{

					/* Detectable */
					flag = TRUE;

					/* Memorize flags */
					l_ptr->flags2 |= (RF2_WEIRD_MIND);

					/* Hack -- Memorize mental flags */
					if (r_ptr->flags2 & (RF2_SMART)) l_ptr->flags2 |= (RF2_SMART);
					if (r_ptr->flags2 & (RF2_STUPID)) l_ptr->flags2 |= (RF2_STUPID);
					if (r_ptr->flags3 & (RF3_NONVOCAL)) l_ptr->flags2 |= (RF3_NONVOCAL);
				}
			}

			/* Normal mind, allow telepathy */
			else
			{
				/* Detectable */
				flag = TRUE;

				/* Hack -- Memorize mental flags */
				if (r_ptr->flags2 & (RF2_SMART)) l_ptr->flags2 |= (RF2_SMART);
				if (r_ptr->flags2 & (RF2_STUPID)) l_ptr->flags2 |= (RF2_STUPID);
				if (r_ptr->flags3 & (RF3_NONVOCAL)) l_ptr->flags2 |= (RF3_NONVOCAL);

			}

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			if (flag) equip_can_flags(0x0L,0x0L,TR3_TELEPATHY,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_ORC)) != 0) && (r_ptr->flags3 & (RF3_ORC)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_ORC);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_ORC,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_GIANT)) != 0) && (r_ptr->flags3 & (RF3_GIANT)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_GIANT);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_GIANT,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_TROLL)) != 0) && (r_ptr->flags3 & (RF3_TROLL)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_TROLL);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_TROLL,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_DRAGON)) != 0) && (r_ptr->flags3 & (RF3_DRAGON)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_DRAGON);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_DRAGON,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_DRAGON)) != 0) && (r_ptr->flags3 & (RF3_DEMON)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_DEMON);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_DEMON,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_UNDEAD)) != 0) && (r_ptr->flags3 & (RF3_UNDEAD)))
		{
			flag = TRUE;
			l_ptr->flags3 |= (RF3_UNDEAD);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_UNDEAD,0x0L);
#endif
		}

		/* Magical sensing */
		if (((p_ptr->cur_flags3 & (TR3_ESP_NATURE)) != 0) && (r_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT)))
		{
			flag = TRUE;
			if (r_ptr->flags3 & (RF3_ANIMAL)) l_ptr->flags3 |= (RF3_ANIMAL);
			if (r_ptr->flags3 & (RF3_PLANT)) l_ptr->flags3 |= (RF3_PLANT);
			if (r_ptr->flags3 & (RF3_INSECT)) l_ptr->flags3 |= (RF3_INSECT);

#ifdef ALLOW_OBJECT_INFO_MORE
			/* Visible */
			equip_can_flags(0x0L,0x0L,TR3_ESP_NATURE,0x0L);
#endif
		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_ORC) && (r_ptr->flags3 & (RF3_ORC))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_ORC);
		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_GIANT) && (r_ptr->flags3 & (RF3_GIANT))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_GIANT);
		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_TROLL) && (r_ptr->flags3 & (RF3_TROLL))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_TROLL);

		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_DRAGON) && (r_ptr->flags3 & (RF3_DRAGON))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_DRAGON);

		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_DEMON) && (r_ptr->flags3 & (RF3_DEMON))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_DEMON);
		}


		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_UNDEAD) && (r_ptr->flags3 & (RF3_UNDEAD))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_UNDEAD);

		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_ANIMAL) && (r_ptr->flags3 & (RF3_ANIMAL))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_ANIMAL);
		}

		/* Style sensing */
		if ((p_ptr->pstyle == WS_SLAY_EVIL) && (r_ptr->flags3 & (RF3_EVIL))
		 && ((m_idx % 10) == 5))
		{
			flag = TRUE;

			l_ptr->flags3 |= (RF3_EVIL);
		}

		/* Normal line of sight, and not blind */
		if ((!p_ptr->timed[TMD_BLIND]) && (player_has_los_bold(fy, fx)))
		{
			bool do_invisible = FALSE;
			bool do_cold_blood = FALSE;
#ifdef ALLOW_OBJECT_INFO_MORE
			bool do_warm_blood = FALSE;
#endif
			/* Hidden */
			if (m_ptr->mflag & (MFLAG_HIDE))
			{
			}
			/* Use "infravision" */
			else if (d <= p_ptr->see_infra)
			{
				/* Handle "cold blooded" monsters */
				if (r_ptr->flags2 & (RF2_COLD_BLOOD))
				{
					/* Take note */
					do_cold_blood = TRUE;
				}

				/* Handle "warm blooded" monsters */
				else
				{
#ifdef ALLOW_OBJECT_INFO_MORE
					/* Take note */
					do_warm_blood = TRUE;
#endif

#ifdef ALLOW_OBJECT_INFO_MORE
					if (!flag && !(r_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND))) equip_not_flags(0x0L,0x0L,TR3_TELEPATHY,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_ORC))) equip_not_flags(0x0L,0x0L,TR3_ESP_ORC,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_TROLL))) equip_not_flags(0x0L,0x0L,TR3_ESP_TROLL,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_GIANT))) equip_not_flags(0x0L,0x0L,TR3_ESP_GIANT,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_DRAGON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DRAGON,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_DEMON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DEMON,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_UNDEAD))) equip_not_flags(0x0L,0x0L,TR3_ESP_UNDEAD,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT))) equip_not_flags(0x0L,0x0L,TR3_ESP_NATURE,0x0L);
#endif
					/* Easy to see */
					easy = flag = TRUE;
				}
			}

			/* Use "illumination" */
			if (player_can_see_bold(fy, fx))
			{
				/* Hidden */
				if (m_ptr->mflag & (MFLAG_HIDE))
				{
				}

				/* Handle "invisible" monsters */
				else if ((r_ptr->flags2 & (RF2_INVISIBLE)) || (m_ptr->tim_invis))
				{
					/* Take note */
					do_invisible = TRUE;

					/* See invisible */
					if ((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0)
					{

#ifdef ALLOW_OBJECT_INFO_MORE
						if (!flag && !(r_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND)) &&
							!(l_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND))) equip_not_flags(0x0L,0x0L,TR3_TELEPATHY,0x0L);

						if (!flag && (l_ptr->flags3 & (RF3_ORC))) equip_not_flags(0x0L,0x0L,TR3_ESP_ORC,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_TROLL))) equip_not_flags(0x0L,0x0L,TR3_ESP_TROLL,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_GIANT))) equip_not_flags(0x0L,0x0L,TR3_ESP_GIANT,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_DRAGON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DRAGON,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_DEMON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DEMON,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_UNDEAD))) equip_not_flags(0x0L,0x0L,TR3_ESP_UNDEAD,0x0L);
						if (!flag && (l_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT))) equip_not_flags(0x0L,0x0L,TR3_ESP_NATURE,0x0L);
#endif
						/* Easy to see */
						easy = flag = TRUE;
					}
				}

				/* Handle "normal" monsters */
				else
				{
#ifdef ALLOW_OBJECT_INFO_MORE
					if (!flag && !(r_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND)) &&
						!(l_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND))) equip_not_flags(0x0L,0x0L,TR3_TELEPATHY,0x0L);

					if (!flag && (l_ptr->flags3 & (RF3_ORC))) equip_not_flags(0x0L,0x0L,TR3_ESP_ORC,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_TROLL))) equip_not_flags(0x0L,0x0L,TR3_ESP_TROLL,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_GIANT))) equip_not_flags(0x0L,0x0L,TR3_ESP_GIANT,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_DRAGON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DRAGON,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_DEMON))) equip_not_flags(0x0L,0x0L,TR3_ESP_DEMON,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_UNDEAD))) equip_not_flags(0x0L,0x0L,TR3_ESP_UNDEAD,0x0L);
					if (!flag && (l_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT))) equip_not_flags(0x0L,0x0L,TR3_ESP_NATURE,0x0L);
#endif

					/* Easy to see */
					easy = flag = TRUE;
				}
			}

			/* Visible */
			if (flag)
			{
				/* Memorize flags */
				if (do_invisible)
				{
					if (r_ptr->flags2 & RF2_INVISIBLE) l_ptr->flags2 |= (RF2_INVISIBLE);
#ifdef ALLOW_OBJECT_INFO_MORE
					if ((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0) equip_can_flags(0x0L,0x0L,TR3_SEE_INVIS,0x0L);
#endif
				}
				if (do_cold_blood) l_ptr->flags2 |= (RF2_COLD_BLOOD);
#ifdef ALLOW_OBJECT_INFO_MORE
				if (do_warm_blood)
				{
					if (rp_ptr->infra < d) equip_can_flags(TR1_INFRA,0x0L,0x0L,0x0L);
				}
#endif
				if (r_ptr->flags2 & (RF2_HAS_LITE)) l_ptr->flags2 |= (RF2_HAS_LITE);

			}
		}
	}


	/* The monster is now visible */
	if (flag)
	{
		/* It was previously unseen */
		if (!m_ptr->ml)
		{
			/* Mark as visible */
			m_ptr->ml = TRUE;

			/* Draw the monster */
			lite_spot(fy, fx);

			/* Update health bar as needed */
			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

			/* Mega Hack -- If not seen before, disturb */
			if (!l_ptr->sights) disturb(1, 0);

			/* Hack -- Count "fresh" sightings */
			if (l_ptr->sights < MAX_SHORT) l_ptr->sights++;

			/* Disturb on visibility change */
			if ((disturb_move)
	      		&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
	      		disturb(1, 0);

			/* Window stuff */
			p_ptr->window |= PW_MONLIST;
		}
	}

	/* The monster is not visible */
	else
	{
		/* It was previously seen */
		if (m_ptr->ml)
		{
			/* Mark as not visible */
			m_ptr->ml = FALSE;

			/* Erase the monster */
			lite_spot(fy, fx);

			/* Update health bar as needed */
			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

			/* Disturb on visibility change */
			if ((disturb_move)
		      		&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
				disturb (1, 0);

			/* Window stuff */
			p_ptr->window |= PW_MONLIST;
		}
	}


	/* The monster is now easily visible */
	if (easy)
	{
		/* Change */
		if (!(m_ptr->mflag & (MFLAG_VIEW)))
		{
			/* Mark as easily visible */
			m_ptr->mflag |= (MFLAG_VIEW);

			/* Disturb on visibility change */
			if ((disturb_near)
				&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
					disturb(1, 0);
		}
	}

	/* The monster is not easily visible */
	else
	{
		/* Change */
		if (m_ptr->mflag & (MFLAG_VIEW))
		{
			/* Mark as not easily visible */
			m_ptr->mflag &= ~(MFLAG_VIEW);

			/* Disturb on visibility change */
			if ((disturb_near)
				&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
					disturb(1, 0);
		}
	}
}




/*
 * This function simply updates all the (non-dead) monsters (see above).
 */
void update_monsters(bool full)
{
	int i;

	/* Update each (live) monster */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Update the monster */
		update_mon(i, full);
	}
}


/*
 * Make a monster carry an object
 */
s16b monster_carry(int m_idx, object_type *j_ptr)
{
	s16b o_idx;

	s16b this_o_idx, next_o_idx = 0;

	monster_type *m_ptr = &m_list[m_idx];

	/* Scan objects already being held for combination */
	for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(o_ptr, j_ptr))
		{
			/* Combine the items */
			object_absorb(o_ptr, j_ptr, TRUE);

			/* Result */
			return (this_o_idx);
		}
	}

	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;

		/* Get new object */
		o_ptr = &o_list[o_idx];

		/* Copy object */
		object_copy(o_ptr, j_ptr);

		/* Forget mark */
		o_ptr->ident &= ~(IDENT_MARKED);

		/* Forget location */
		o_ptr->iy = o_ptr->ix = 0;

		/* Link the object to the monster */
		o_ptr->held_m_idx = m_idx;

		/* Link the object to the pile */
		o_ptr->next_o_idx = m_ptr->hold_o_idx;

		/* Link the monster to the object */
		m_ptr->hold_o_idx = o_idx;
	}

	/* Result */
	return (o_idx);
}


/*
 * Position player on a grid and update information.
 *
 * Used by player_swap and player_place.
 *
 * TODO: Consider using when cave information (e.g. light) is under
 * player or when terrain is updated under the player.
 */
static void player_position()
{
	int y = p_ptr->py;
	int x = p_ptr->px;
	int by = y/BLOCK_HGT;
	int bx = x/BLOCK_WID;

	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	/* Update view if moved outside/inside */
	bool outside = (level_flag & (LF1_SURFACE))
		&& (f_ptr->flags3 & (FF3_OUTSIDE));

	/* Room is perma-lit */
	if (cave_info[y][x] & (CAVE_GLOW))
		room_info[dun_room[by][bx]].flags |= (ROOM_SEEN);

	/* Player has heard the room */
	room_info[dun_room[by][bx]].flags |= (ROOM_HEARD);

	/* Changed inside/outside */
	if (outside != p_ptr->outside)
	{
		p_ptr->redraw |= (PR_MAP);
	}

	/* Change state */
	p_ptr->outside = outside;

	/* Hack -- display 'furnishings' */
	if (((f_ptr->flags3 & (FF3_ALLOC)) != 0) &&
		((f_ptr->flags1 & (FF1_TRAP)) == 0))
	{
		msg_format("You %s %s %s.", p_ptr->timed[TMD_BLIND] || no_lite()? "feel" : "see", is_a_vowel((f_name + f_ptr->name)[0]) ? "an" : "a",
			f_name + f_ptr->name);

		play_info[y][x] |= (PLAY_MARK);

		lite_spot(y, x);
	}

	/* If blind, silently notice what the player is on */
	else if ((p_ptr->timed[TMD_BLIND] || no_lite()) && ((play_info[y][x] & (PLAY_MARK)) == 0) &&
		((f_info[cave_feat[y][x]].flags1 & (FF1_NOTICE)) != 0))
	{
		play_info[y][x] |= (PLAY_MARK);

		lite_spot(y, x);
	}
}


/*
 *  Helper function for monster swap. Update player based on moving y1, x1 to y2, x2
 */
static void player_swap(const int y1, const int x1, const int y2, const int x2)
{
	int by1 = y1/BLOCK_HGT;
	int bx1 = x1/BLOCK_WID;
	int by2 = y2/BLOCK_HGT;
	int bx2 = x2/BLOCK_WID;

	/* Move player */
	p_ptr->py = y2;
	p_ptr->px = x2;

	/* Update the panel */
	p_ptr->update |= (PU_PANEL);

	/* Update the visuals (and monster distances) */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_DISTANCE);

	/* Update the bonuses -- due to mud etc */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MONLIST | PW_ITEMLIST);

	/* Update room description (if needed) */
	/* Line 1 -- we are entering a room */
	/* Line 2 -- which is different from the last room */
	/* Line 3 -- or we were not in a room */
	/* Line 4 -- or we move to a lit portion of the room */
	if ((cave_info[y2][x2] & (CAVE_ROOM)) &&
	 ((dun_room[by1][bx1] != dun_room[by2][bx2]) ||
		!(cave_info[y1][x1] & (CAVE_ROOM)) ||
		((cave_info[y2][x2] & (CAVE_GLOW)) != (cave_info[y1][x1] & (CAVE_GLOW))) ))
	{
		p_ptr->window |= (PW_ROOM_INFO);
		p_ptr->update |= (PU_ROOM_INFO);
	}

	/* Update positional information */
	player_position();
}




/*
 * Swap the players/monsters (if any) at two locations XXX XXX XXX
 */
void monster_swap(int y1, int x1, int y2, int x2)
{
	int m1, m2;

	monster_type *m_ptr;
	monster_race *r_ptr;

	bool lite1 = FALSE, lite2 = FALSE, lite3 = FALSE, lite4 = FALSE;

	/* Monsters */
	m1 = cave_m_idx[y1][x1];
	m2 = cave_m_idx[y2][x2];

	/* Update grids */
	cave_m_idx[y1][x1] = m2;
	cave_m_idx[y2][x2] = m1;

	/* Monster 1 */
	if (m1 > 0)
	{
		m_ptr = &m_list[m1];
		r_ptr = &r_info[m_ptr->r_idx];

		/* Check monster lite at origin */
		if (m2 > 0) lite1 = check_monster_lite(m1);
		else lite1 = FALSE;

		/* Move monster */
		m_ptr->fy = y2;
		m_ptr->fx = x2;

		/* Some monsters radiate damage when moving */
		if (r_ptr->flags2 & (RF2_HAS_AURA))
		{
			int flg = (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_PLAY |
			PROJECT_HIDE | PROJECT_WALL);

			/* The target is attacked by a ball attack */
			(void)mon_blow_ranged(m1, 96 + 7, y2, x2, RBM_AURA, 2, flg);
		}

		/* Some monsters trail damage when moving */
		else if (r_ptr->flags2 & (RF2_TRAIL))
		{
			int flg = (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_PLAY |
			PROJECT_HIDE | PROJECT_WALL);

			/* The target is attacked by a ball attack */
			(void)mon_blow_ranged(m1, 96 + 7, y2, x2, RBM_TRAIL, 0, flg);
		}

		/* Update monster */
		update_mon(m1, TRUE);

		/* Check for lite again */
		lite2 = check_monster_lite(m1);
	}

	/* Player 1 */
	else if (m1 < 0)
	{
		player_swap(y1, x1, y2, x2);
	}

	/* Monster 2 */
	if (m2 > 0)
	{
		m_ptr = &m_list[m2];
		r_ptr = &r_info[m_ptr->r_idx];

		/* Check monster lite at origin */
		if (m1 > 0) lite3 = check_monster_lite(m1);
		else lite3 = FALSE;

		/* Move monster */
		m_ptr->fy = y1;
		m_ptr->fx = x1;

		/* Some monsters radiate lite when moving */
		if (r_ptr->flags2 & (RF2_HAS_LITE | RF2_NEED_LITE))
		{
			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
		}

		/* Some monsters radiate damage when moving */
		if (r_ptr->flags2 & (RF2_HAS_AURA))
		{
			int flg = (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_PLAY |
			PROJECT_HIDE | PROJECT_WALL);

			/* The target is attacked by a ball attack */
			(void)mon_blow_ranged(m2, 96 + 7, y1, x1, RBM_AURA, 2, flg);
		}

		/* Some monsters trail damage when moving */
		else if (r_ptr->flags2 & (RF2_TRAIL))
		{
			int flg = (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_PLAY |
			PROJECT_HIDE | PROJECT_WALL);

			/* The target is attacked by a ball attack */
			(void)mon_blow_ranged(m2, 96 + 7, y1, x1, RBM_TRAIL, 0, flg);
		}

		/* Update monster */
		update_mon(m2, TRUE);

		/* Check monster lite at origin */
		lite4 = check_monster_lite(m2);
	}

	/* Player 2 */
	else if (m2 < 0)
	{
		player_swap(y2, x2, y1, x1);
	}

	/* Check monster lites */
	if (lite1 && !lite4)
	{
		check_attribute_lost(y1, x1, 2, CAVE_XLOS, require_torch_lit, has_torch_lit, redraw_torch_lit_loss, remove_torch_lit, reapply_torch_lit);
	}

	/* Check monster lites */
	if (lite3 && !lite2)
	{
		check_attribute_lost(y2, x2, 2, CAVE_XLOS, require_torch_lit, has_torch_lit, redraw_torch_lit_loss, remove_torch_lit, reapply_torch_lit);
	}

	/* Handle creating "glowing" terrain */
	if (!lite1 && lite4)
	{
		gain_attribute(y1, x1, 2, CAVE_XLOS, apply_torch_lit, redraw_torch_lit_gain);
	}

	/* Handle creating "glowing" terrain */
	if (!lite3 && lite2)
	{
		gain_attribute(y2, x2, 2, CAVE_XLOS, apply_torch_lit, redraw_torch_lit_gain);
	}

	/* Redraw */
	lite_spot(y1, x1);
	lite_spot(y2, x2);
}


/*
 * Monster resists a particular effect type.
 *
 * Note that this should only be used for navigation purposes
 * to determine whether a monster can enter a particular grid because
 * of either the feature in the grid, or a region in the grid.
 *
 * Returns -1 if the monster resists naturally, 0 if the monster
 * does not resist, and 1 if the monster resists if they are
 * able to correctly navigate the terrain.
 */
int mon_resist_effect(int effect, int r_idx)
{
	monster_race *r_ptr;

	/* Paranoia */
	if (!r_idx) return (FALSE);

	/* Race */
	r_ptr = &r_info[r_idx];

	/* Monster resists effect */
	switch (effect)
	{
		case GF_ICE:
		case GF_HURT:
		case GF_UN_BONUS:
		case GF_UN_POWER:
		case GF_EAT_GOLD:
		case GF_EAT_ITEM:
		case GF_EAT_FOOD:
		case GF_EAT_LITE:
		return (-1);
		break;

		case GF_DELAY_POISON:
		case GF_POIS:
		case GF_POISON_WEAK:
		case GF_POISON_HALF:
		if (!(r_ptr->flags3 & (RF3_IM_POIS))) return (0);
		break;

		case GF_ACID:
		case GF_VAPOUR:
		if (!(r_ptr->flags3 & (RF3_IM_ACID))) return (0);
		break;

		case GF_ELEC:
		if (!(r_ptr->flags3 & (RF3_IM_ELEC))) return (0);
		break;

		case GF_FIRE:
		case GF_SMOKE:
		if (!(r_ptr->flags3 & (RF3_IM_FIRE))) return (0);
		break;

		case GF_LAVA:
		if (!(r_ptr->flags3 & (RF3_RES_LAVA))) return (0);
		break;

		case GF_POISON_WATER:
		if (!(r_ptr->flags3 & (RF3_IM_POIS))) return (0);
		/* Fall through */

		case GF_WATER_WEAK:
		case GF_WATER:
		if (r_ptr->flags3 & (RF3_NONLIVING))  return (-1);
		return (1);

		case GF_BWATER:
		case GF_STEAM:
		if ((r_ptr->flags3 & (RF3_NONLIVING)) && (r_ptr->flags3 & (RF3_IM_FIRE))) return (-1);
		if (!(r_ptr->flags3 & (RF3_IM_FIRE))) return (0);
		return (1);

		case GF_BMUD:
		if ((r_ptr->flags3 & (RF3_NONLIVING))&& (r_ptr->flags3 & (RF3_IM_FIRE))) return (TRUE);
		if (!(r_ptr->flags3 & (RF3_IM_FIRE))) return (0);
		return(1);

		case GF_COLD:
		if (!(r_ptr->flags3 & (RF3_IM_COLD))) return (0);
		break;

		case GF_BLIND:
		case GF_BLIND_WEAK:
		if (!(r_ptr->flags9 & (RF9_RES_BLIND))) return (0);
		break;

		case GF_CONFUSION:
		case GF_CONF_WEAK:
		if (!(r_ptr->flags3 & (RF3_NO_CONF))) return (0);
		break;

		case GF_TERRIFY:
		case GF_FEAR_WEAK:
		if (!(r_ptr->flags3 & (RF3_NO_FEAR))) return (0);
		break;

		case GF_SLEEP:
		case GF_PARALYZE:
		if (!(r_ptr->flags3 & (RF3_NO_SLEEP))) return (0);
		break;

		case GF_LOSE_STR:
		case GF_LOSE_INT:
		case GF_LOSE_WIS:
		case GF_LOSE_DEX:
		case GF_LOSE_CON:
		case GF_LOSE_CHR:
		case GF_LOSE_ALL:
		case GF_EXP_10:
		case GF_EXP_20:
		case GF_EXP_40:
		case GF_EXP_80:
		if (!(r_ptr->flags3 & (RF3_UNDEAD))) return (0);
		break;

		case GF_SHATTER:
		return (0);
		break;

		case GF_NOTHING:
		break;

		case GF_FALL_MORE:
		if (!(r_ptr->flags2 & (RF2_CAN_FLY))) return (0);
		break;

		case GF_FALL:
		case GF_FALL_SPIKE:
		case GF_FALL_POIS:
		if (!(r_ptr->flags1 & (RF1_NEVER_MOVE))) return (0);
		break;

		default:
		return (0);
		break;
	}

	return (TRUE);
}




/* XXX XXX Checking by blow method is broken currently, because we
 * historically damage monsters even when they are immune to fire etc. So we
 * should have an advanced flow by threat level for monsters. The borg does this
 * effectively, but is somewhat computationally expensive and still isn't
 * perfect. Besides, implementing borg code for monsters is Angband 3.0.0 work.
 *
 * Instead, we rely on marking the level whenever a particular
 * terrain type is generated on that level. Then we prefer to generate monsters
 * on that level that have immunity to that particular terrain type(s). Even
 * that is a bit broken because some immunities rely on knowing the prefix
 * to a monster flag, a) we don't want to restrict monster types too heavily.
 * ie water and lava, require "magma" and "water" elementals to have correct
 * immunity and b) checking for a prefix all the time would add to the CPU
 * flow cost.
 *
 * So we give monsters 'fake' immunity.
 * All swimming monsters and digging monsters, while swimming or digging are
 * IM_WATER.
 * All IM_FIRE swimming monsters are IM_BWATER.
 * All IM_FIRE digging monsters are IM_BMUD.
 * All BR_FIRE monsters are IM_LAVA.
 * All NEVER_MOVE monsters ignore falling damage from pits (But not trap doors
 * /falls).
 * All NONLIVING monsters are IM_WATER.
 * All NONLIVING IM_FIRE monsters are IM_BWATER and IM_BMUD.
 *
 * Note the difference between NONLIVING and CAN_SWIM/CAN_DIG monsters is that
 * non-living monsters are unable to 'pop-up' and attack or cast spells while
 * in deep terrain. They are instead silently 'pushed over'.
 *
 * We must make sure that we never apply blows to monsters from terrain
 * that qualify for fake immunity unless we need to (Say from a trap).
 *
 * This has the downside that we can allow a player to manipulate how monsters
 * are generated and move through judicious use of the flow options. This has
 * always been the case however. To minimise the risk of outright manipulation
 * we should confuse a monster whenever it is stuck in terrain it can't seem to
 * move out of. This is better than any attempt to use a flow algorithm that
 * is not sufficiently smart.
 *
 * By not sufficiently smart, we mean not taking in account of a case such as
 * being smartest to flow across lava to get to water which we can swim in
 * to attack the player in cases for creatures that are both capable and not
 * capable of traversing both, one or neither type of terrain.
 *
 * And don't use this an argument against terrain, because we fail to handle
 * exactly the same type of cases for doors and/or permanent features in
 * vanilla Angband. Witness the grief caused by vaults on levels with Morgoth
 * and numerous player exploits possible under this scenario.
 *
 * We then stop monsters flowing that can't flow through all non-wall terrain
 * on the level. This is currently unimplemented.
 */
bool mon_resist_feat(int feat, int r_idx)
{
	monster_race *r_ptr;
	feature_type *f_ptr = &f_info[feat];

	bool daytime = (bool)(level_flag & LF1_DAYLIGHT);
	bool outside = (level_flag & (LF1_SURFACE))
		&& (f_ptr->flags3 & (FF3_OUTSIDE));

	/* Paranoia */
	if (!r_idx) return (FALSE);

	/* Race */
	r_ptr = &r_info[r_idx];

	/* Always get burnt by daylight */
	if ((daytime && outside) && (r_ptr->flags3 & (RF3_HURT_LITE)))
		return (FALSE);

	/* Monsters which never move can never be hit by traps,
	 * as traps require movement to trigger. */
	if ((r_ptr->flags1 & (RF1_NEVER_MOVE)) && (f_ptr->flags1 & (FF1_HIT_TRAP))) return (TRUE);

	/* Always fail for spells */
	if (f_ptr->spell) return (FALSE);

	/* Check trap/feature attack */
	if (f_ptr->blow.method)
	{
		switch(mon_resist_effect(f_ptr->blow.effect, r_idx))
		{
			case -1:
				return (TRUE);
			case 0:
				return (FALSE);
			case 1:
				if ((r_ptr->flags2 & (RF2_CAN_SWIM)) && (f_ptr->flags2 & (FF2_CAN_SWIM))) return (TRUE);
				if ((r_ptr->flags2 & (RF2_CAN_DIG)) && (f_ptr->flags2 & (FF2_CAN_DIG))) return (TRUE);
				return (FALSE);
		}
	}


	return(TRUE);
}


/* ANDY - The routine we use to determine if we can place a monster
 * on a particular terrain type. This returns an integer defining
 * what mode of movement the monster is using (climbing, swimming etc.)
 * or a FALSE result if the monster cannot move (at all) on this terrain.
 *
 * Note the integer is negative if the monster doesn't 'like' the terrain
 * and will begin drowning in it if this returns MM_DROWN. Drowning is
 * used to force monsters to either move or die.
 */
int place_monster_here(int y, int x, int r_idx)
{
	feature_type *f_ptr;
	monster_race *r_ptr;

	/* Improve efficiency -- forced boolean */
	bool resist;
	bool trap = FALSE;

	/* Get feature info */
	f_ptr= &f_info[cave_feat[y][x]];

	/* Paranoia */
	if (!r_idx) return (MM_FAIL);

	/* Race */
	r_ptr = &r_info[r_idx];

	/* Monster resists terrain damage? */
	resist = mon_resist_feat(cave_feat[y][x],r_idx);

	/* Monster can't move through traps easily */
	if (f_ptr->flags1 & (FF1_HIT_TRAP))
	{
		/* Monster can't avoid trap */
		/*
		 * Note that traps are the only way flying/climbing monsters can
		 * take damage from terrain.
		 */
		if (f_ptr->flags1 & (FF1_TRAP))
		{
			/* Unavoidable */
			if (race_avoid_trap(r_idx, y, x, cave_feat[y][x]))
			{
				resist = TRUE;
			}
			/* Prevent flyers avoiding, but they may resist */
			else
			{
				trap = !resist;
			}
		}
	}

	/* Regions can also trap the monster */
	if (cave_region_piece[y][x])
	{
		/* XXX Iterate through the regions and see if the monster will be
		 * affected by any of them.
		 */
		s16b this_region_piece, next_region_piece = 0;

		for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
		{
			region_piece_type *rp_ptr = &region_piece_list[this_region_piece];
			region_type *re_ptr = &region_list[rp_ptr->region];

			/* Get the next object */
			next_region_piece = rp_ptr->next_in_grid;

			/* Skip dead regions */
			if (!re_ptr->type) continue;

			/* Trapped regions */
			if (re_ptr->flags1 & (RE1_HIT_TRAP))
			{
				if (!race_avoid_trap(r_idx, y, x, cave_feat[re_ptr->y0][re_ptr->y1]))
				{
					trap |= !mon_resist_feat(cave_feat[re_ptr->y0][re_ptr->x0], r_idx);
				}
			}

			/* Non-trapped regions */
			else if (!(mon_resist_effect(re_ptr->effect, r_idx)))
			{
				trap = TRUE;
			}
		}
	}


	/* Check for pass wall */
	if (resist &&
		((r_ptr->flags2 & (RF2_PASS_WALL)) != 0)) return (MM_PASS);

	/* Check for swimming */
	if (resist &&
		((r_ptr->flags2 & (RF2_CAN_SWIM | RF2_MUST_SWIM)) != 0) &&
		((f_ptr->flags2 & (FF2_CAN_SWIM)) != 0))
	{
		return(MM_SWIM);
	}
	else if ((r_ptr->flags2 & (RF2_MUST_SWIM)) != 0)
	{
		return(MM_DROWN);
	}

	/* Check for digging */
	if (resist &&
		((r_ptr->flags2 & (RF2_CAN_DIG)) != 0) &&
		((f_ptr->flags2 & (FF2_CAN_DIG)) != 0))
	{
		return(MM_DIG);
	}

	/* Check if we don't need to breath -- note move check because we now mark walls as 'filled' */
	if (resist &&
		((r_ptr->flags3 & (RF3_NONLIVING)) != 0) &&
		((f_ptr->flags1 & (FF1_MOVE)) != 0) &&
		((f_ptr->flags2 & (FF2_DEEP | FF2_FILLED)) != 0))
	{
		return (MM_UNDER);
	}

	/* Hack -- check for oozing */
	if (resist &&
		((r_ptr->flags3 & (RF3_OOZE)) != 0) &&
		((f_ptr->flags2 & (FF2_CAN_OOZE)) != 0))
	{
		return(MM_OOZE);
	}

	/* Hack -- check for flying. */
	if (((r_ptr->flags2 & (RF2_CAN_FLY | RF2_MUST_FLY)) != 0) &&
		((f_ptr->flags2 & (FF2_CAN_FLY)) != 0))
	{
		return(trap ? MM_TRAP : MM_FLY);
	}

	else if ((r_ptr->flags2 & (RF2_MUST_FLY)) != 0)
	{
		return(MM_DROWN);
	}

	/* Check for climbing - easily climb-able */
	if (((r_ptr->flags2 & (RF2_CAN_CLIMB)) != 0) &&
		((f_ptr->flags3 & (FF3_EASY_CLIMB)) != 0))
	{
		return(trap ? MM_TRAP : MM_CLIMB);
	}

	/* Check for climbing - adjacent to CAN_CLIMB terrain.
	 * Note the use of the CAVE_CLIM flag to optimise this. */
	if (((r_ptr->flags2 & (RF2_CAN_CLIMB)) != 0) &&
		(cave_info[y][x] & (CAVE_CLIM)) &&
		((f_ptr->flags2 & (FF2_CAN_FLY)) != 0))
	{
		return(trap ? MM_TRAP : MM_CLIMB);
	}

	/* Get mimiced feat if covered/bridged */
	if ((f_ptr->flags2 & (FF2_COVERED)) != 0)
	{
		resist = mon_resist_feat(f_ptr->mimic,r_idx);
	}

	/* Regular move/climb/drown */
	if (((f_ptr->flags1 & (FF1_MOVE)) != 0) || ((f_ptr->flags3 & (FF3_EASY_CLIMB)) != 0))
	{
		if (!resist) return ((f_ptr->flags1 & (FF1_HIT_TRAP)) ? MM_TRAP : MM_DROWN);
		if (f_ptr->flags2 & (FF2_DEEP | FF2_FILLED)) return (MM_DROWN);
		if (!(f_ptr->flags1 & (FF1_MOVE))) return (MM_CLIMB);

		return (MM_WALK);
	}

	return(MM_FAIL);

}

/*
 * Hide a monster in terrain or position it over terrain as necessary.
 *
 * If check is set to true, we just return whether or not the monster
 * would have hidden.
 */
void monster_hide(int y, int x, int mmove, monster_type *m_ptr)
{
	bool surface = (bool)(level_flag & (LF1_SURFACE));
	bool lite = (m_ptr->mflag & (MFLAG_LITE)) != 0;

	/* Get the feature */
	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	/* Get the race */
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Over the ceiling of a building */
	if ((m_ptr->mflag & (MFLAG_OVER)) &&
		(m_ptr->mflag & (MFLAG_HIDE)))
	{
		/* Don't change state or update further */
		return;
	}

	/* Update flags */
	if (((m_ptr->mflag & (MFLAG_OVER))==0)
	       && ((f_ptr->flags1 & (FF1_MOVE)) == 0)
		   && ((f_ptr->flags3 & (FF3_EASY_CLIMB)) == 0)
	       && (mmove != MM_PASS))
	{
		/* Nothing -- we are stuck inside a terrain feature */
	}
	else if ((f_ptr->flags3 & (FF3_EASY_CLIMB)) != 0)
	{
		m_ptr->mflag |= (MFLAG_OVER);
	}
	else if ((mmove == MM_FLY) || (mmove == MM_CLIMB))
	{
		m_ptr->mflag |= (MFLAG_OVER);
	}
	else if (!(surface) || ((f_ptr->flags3 & (FF3_OUTSIDE)) != 0))
	{
		m_ptr->mflag &= ~(MFLAG_OVER);
	}

	/* Never hide if over terrain, except on surface, and outside */
	if ((m_ptr->mflag & (MFLAG_OVER)) != 0)
	{
		m_ptr->mflag &= ~(MFLAG_HIDE);
	}
	/* Never hide in terrain we don't resist */
	else if (!mon_resist_feat(cave_feat[y][x],m_ptr->r_idx))
	{
		m_ptr->mflag &= ~(MFLAG_HIDE);
	}

	/* Hack -- cannot hide in open air */
	else if (cave_feat[y][x] == FEAT_CHASM)
	{
		m_ptr->mflag &= ~(MFLAG_HIDE);
	}

	/* Set hide flag if passing through floor/ceiling (phasing) */
	else if (mmove == MM_PASS)
	{
		m_ptr->mflag |= (MFLAG_HIDE);
	}

	/* Maintain hidden/unhidden state if covered */
	else if (f_ptr->flags2 & (FF2_COVERED))
	{
		/* Nothing */
	}

	/* Set hide flag if HIDE_SNEAK and monster is sneaky */
	else if (((f_ptr->flags2 & (FF2_HIDE_SNEAK)) != 0)
		&& ((r_ptr->flags2 & (RF2_SNEAKY)) != 0)
		&& ((m_ptr->mflag & (MFLAG_OVER)) == 0))
	{
		m_ptr->mflag |= (MFLAG_HIDE);
	}
	/* Set hide flag if digging and HIDE_DIG */
	else if (((f_ptr->flags2 & (FF2_HIDE_DIG)) != 0) && (mmove == MM_DIG))
	{
		m_ptr->mflag |= (MFLAG_HIDE);
	}
	/* Set hide flag if swimming and HIDE_SWIM */
	else if (((f_ptr->flags2 & (FF2_HIDE_SWIM)) != 0) && (mmove == MM_SWIM))
	{
		m_ptr->mflag |=(MFLAG_HIDE);
	}
	/* Set hide flag if EASY_HIDE, with conditions */
	else if ((f_ptr->flags3 & (FF3_EASY_HIDE)) != 0)
	{
		/* Covered/bridged features are special */
		if ((f_ptr->flags2 & (FF2_COVERED)) != 0)
		{
			/* Nothing */
		}
		else
		{
			m_ptr->mflag |=(MFLAG_HIDE);
		}
	}
	else
	{
		m_ptr->mflag &= ~(MFLAG_HIDE);
	}

	/* Hiding monsters gets rid of light */
	if (m_ptr->mflag & (MFLAG_HIDE))
	{
		m_ptr->mflag &= ~(MFLAG_LITE);

		if (lite)
		{
			check_attribute_lost(y, x, 2, CAVE_XLOS, require_torch_lit, has_torch_lit, redraw_torch_lit_loss, remove_torch_lit, reapply_torch_lit);
		}
	}

	/* Unhiding monsters which have light always show lite */
	else if (r_ptr->flags2 & (RF2_HAS_LITE))
	{
		m_ptr->mflag |= (MFLAG_LITE);

		if (!lite)
		{
			gain_attribute(y, x, 2, CAVE_XLOS, apply_torch_lit, redraw_torch_lit_gain);
		}
	}
}


/*
 * Place a copy of a monster in the dungeon XXX XXX
 */
s16b monster_place(int y, int x, monster_type *n_ptr)
{
	s16b m_idx;

	monster_type *m_ptr;
	monster_race *r_ptr;

	/* Paranoia XXX XXX */
	if (cave_m_idx[y][x] != 0) return (0);

	/* Get a new record */
	m_idx = m_pop();

	/* Oops */
	if (m_idx)
	{
		/* Make a new monster */
		cave_m_idx[y][x] = m_idx;

		/* Get the new monster */
		m_ptr = &m_list[m_idx];

		/* Copy the monster XXX */
		COPY(m_ptr, n_ptr, monster_type);

		/* Location */
		m_ptr->fy = y;
		m_ptr->fx = x;

		/* Clear flags */
		m_ptr->mflag &= ~(MFLAG_OVER | MFLAG_HIDE);

		/* Use the simple RNG to preserve seed from before save */
		/* TODO: Perhaps eliminate the RNG roll altogether here */
		/* Rand_quick = TRUE; */

		/* Place as hidden as appropriate */
		monster_hide(y, x, place_monster_here(y, x, m_ptr->r_idx), m_ptr);

		/* Use the complex RNG again */
		/* Rand_quick = FALSE; */

		/* Update the monster */
		update_mon(m_idx, TRUE);

		/* Get the new race */
		r_ptr = &r_info[m_ptr->r_idx];

		/* Hack -- Notice new multi-hued monsters */
		if ((r_ptr->flags1 & (RF1_ATTR_MULTI)) || !(r_ptr->flags9 & (RF9_ATTR_METAL))) shimmer_monsters = TRUE;

		/* Hack -- Count the number of "reproducers" */
		if (r_ptr->flags2 & (RF2_MULTIPLY)) num_repro++;

		/* Count racial occurances */
		r_ptr->cur_num++;
	}

	/* Result */
	return (m_idx);
}


/*
 *  Calculate monster armour class. Takes into account temporary conditions and stats.
 *
 *  We return two different values, depending on ranged or melee ac. Temporary
 *  shield spells and monsters with shields have twice as much effect at range.
 */
int calc_monster_ac(int m_idx, bool ranged)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int ac;

	monster_race monster_race_scaled;

	/* Scale a monster */
	if (((r_ptr->flags9 & RF9_LEVEL_MASK) != 0) &&
		monster_scale(&monster_race_scaled, m_idx, p_ptr->depth))
	{
		r_ptr = &monster_race_scaled;
	}

	/* Get the base ac */
	ac = r_ptr->ac;

	/* Modify by dexterity */
	if (m_ptr->mflag & (MFLAG_CLUMSY)) ac -= 5;
	else if (m_ptr->mflag & (MFLAG_SKILLFUL)) ac += 5;

	/* Hack -- Armoured monsters carry shields at range */
	if ((r_ptr->flags2 & (RF2_ARMOR)) && (ranged))
	{
		/* Notice this */
		if ((m_ptr->ml) && (!(l_ptr->flags2 & (RF2_ARMOR)))) l_ptr->flags2 |= (RF2_ARMOR);

		/* Hack -- assume a shield provides 1/4 of overall base ac */
		ac += r_ptr->ac / 4;
	}

	/* Shield spell counts for double at range */
	if ((m_ptr->shield) && (ranged)) ac += 100;
	else if (m_ptr->shield) ac += 50;

	/* Modify by temporary conditions */
	if (m_ptr->berserk) ac -= 10;
	if (m_ptr->bless) ac += 5;
	if (m_ptr->stunned) ac -= 10;
	if (m_ptr->blind) ac -= 10;

	if (ac <= 0) ac = 1;

	return(ac);
}


/*
 *  Calculate monster maximum hit points.
 */
int calc_monster_hp(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	int hp;
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race monster_race_scaled;

	/* Scale the monster */
	if (((r_ptr->flags9 & RF9_LEVEL_MASK) != 0) &&
		monster_scale(&monster_race_scaled, m_idx, p_ptr->depth))
	{
		r_ptr = &monster_race_scaled;
	}

	/* Hit points */
	hp = r_ptr->hp;

	/* Adjust for monster constitution stat */
	if (m_ptr->mflag & (MFLAG_SICK)) hp = hp * 9 / 10;
	else if (m_ptr->mflag & (MFLAG_HEALTHY)) hp = hp * 11 / 10;

	if (hp < 1) hp = 1;

	return(hp);
}


/* Calculate the monster_speed of a monster */
byte calc_monster_speed(int m_idx)
{
	int speed, i;
	monster_type *m_ptr = &m_list[m_idx];

	/* Get the monster race */
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_race monster_race_scaled;

	/* Scale the monster */
	if (((r_ptr->flags9 & RF9_LEVEL_MASK) != 0) &&
		monster_scale(&monster_race_scaled, m_idx, p_ptr->depth))
	{
		r_ptr = &monster_race_scaled;
	}

	/* Get the monster base speed */
	speed = r_ptr->speed;

	/*note: a monster should only have one of these flags*/
	if (m_ptr->mflag & (MFLAG_SLOW))
	{
		/* Allow some small variation each time to make pillar dancing harder */
		i = extract_energy[r_ptr->speed] / 10;
		speed -= rand_spread(0, i);
	}
	else if (m_ptr->mflag & (MFLAG_FAST))
	{
		/* Allow some small variation each time to make pillar dancing harder */
		i = extract_energy[r_ptr->speed] / 10;
		speed += rand_spread(0, i);
	}

	/* Factor in the hasting and slowing counters*/
	if (m_ptr->hasted) speed += 10;
	if (m_ptr->slowed) speed -= 10;

	return (speed);
}


/* Set the monster faster */
void set_monster_haste(s16b m_idx, s16b counter, bool message)
{
	/*get the monster at the given location*/
	monster_type *m_ptr = &m_list[m_idx];

	bool recalc = FALSE;

	char m_name[80];

	/* Get monster name*/
	monster_desc(m_name, sizeof(m_name), m_idx, 0);

	/*see if we need to recalculate speed*/
	if (m_ptr->hasted)
	{
		/*monster is no longer hasted and speed needs to be recalculated*/
		if (counter == 0)
		{
			recalc = TRUE;

			/*give a message*/
			if (message) msg_format("%^s slows down.", m_name);
		}
	}
	else
	{
		/*monster is now hasted and speed needs to be recalculated*/
		if (counter > 0)
		{
			recalc = TRUE;

			/*give a message*/
			if (message) msg_format("%^s starts moving faster.", m_name);
		}
	}

	/* Update the counter */
	m_ptr->hasted = counter > 255 ? 255 : (byte)counter;

	/*re-calculate speed if necessary*/
	if (recalc) m_ptr->mspeed = calc_monster_speed(m_idx);

	return;
}

/* Set the monster slower */
void set_monster_slow(s16b m_idx, s16b counter, bool message)
{
	/*get the monster at the given location*/
	monster_type *m_ptr = &m_list[m_idx];

	bool recalc = FALSE;

	char m_name[80];

	/* Get monster name*/
	monster_desc(m_name, sizeof(m_name), m_idx, 0);

	/*see if we need to recalculate speed*/
	if (m_ptr->slowed)
	{
		/*monster is no longer slowed and speed needs to be recalculated*/
		if (counter == 0)
		{
			recalc = TRUE;

			/*give a message*/
			if (message) msg_format("%^s speeds up.", m_name);
		}
	}
	else
	{
		/*monster is now slowed and speed needs to be recalculated*/
		if (counter > 0)
		{
			recalc = TRUE;

			/*give a message*/
			if (message) msg_format("%^s starts moving slower.", m_name);
		}
	}

	/* Update the counter */
	m_ptr->slowed = counter > 255 ? 255 : (byte)counter;

	/*re-calculate speed if necessary*/
	if (recalc) m_ptr->mspeed = calc_monster_speed(m_idx);

	return;
}

/*
 * Given a monster and possibly a blow number, return the
 * first object index that it can use as ammunition. If
 * created is set to true, create ammunition for the monster
 * if there is none.
 *
 * blow == -1: Check all blows. Return ammo slot for monster
 * ammunition. If created set true, create ammunition for
 * all blows which require it, and return slot of last
 * ammunition created. If created set false, return slot of
 * valid ammunition, return -1 if no ammunition found.
 * blow == 0..3: Check this one blow. Return -1 if there
 * is no ammunition for this blow. If created set true,
 * create ammunition for this one blow which requires it.
 * Return 0 if monster does not require ammunition for this
 * blow.
 *
 * XXX Hack: we used hard-coded kinds for efficiency
 * purposes to save unnecessary lookups to lookup_kind()
 *
 * XXX Maybe coat ammunition with an appropriate potion
 * in order that the player can use it later on.
 */
int find_monster_ammo(int m_idx, int blow, bool created)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int ammo = 0;
	int ammo_tval = 0;
	int ammo_sval = 0;
	int ammo_kind = 0;
	int this_o_idx, next_o_idx, i;

	object_type *o_ptr;
	object_type object_type_body;

	/* Examine the attacks */
	for (i = (blow < 0 ? 0 : blow); i < (blow < 0 ? 4 : blow + 1); i++)
	{
		int method, effect, d_dice, d_side;

		method_type *method_ptr;

		/* Skip non-attacks */
		if (!r_ptr->blow[i].method) continue;

		/* Extract the attack info */
		method = r_ptr->blow[i].method;
		effect = r_ptr->blow[i].effect;
		d_dice = r_ptr->blow[i].d_dice;
		d_side = r_ptr->blow[i].d_side;

		/* Get the blow pointer */
		method_ptr  = &method_info[method];
		ammo_tval = method_ptr->ammo_tval;

		/* No ammo requirement */
		if (!ammo_tval) continue;

		/* Pick an item kind */
		if (method_ptr->flags2 & (PR2_SCALE_AMMO))
		{
			/* Scale damage dice */
			if (ammo_tval == TV_ARROW) d_dice = d_dice * 4 / 3;
			else if (ammo_tval == TV_SHOT) d_dice = d_dice * 2;
			else if (ammo_tval == TV_FLASK) d_dice /= 3;

			/* Get the ammunition */
			if (d_dice <= 2) ammo_sval = SV_AMMO_LIGHT;
			else if (d_dice <= 4)  ammo_sval = SV_AMMO_NORMAL;
			else if (d_dice <= 16) ammo_sval = SV_AMMO_STEEL;
			else if (d_dice <= 36) ammo_sval = SV_AMMO_SPECIAL;
			else ammo_sval = SV_AMMO_HEAVY;
			
			/* Hack - arrows and bolts have no light ammo */
			if ((ammo_sval == SV_AMMO_LIGHT) && ((ammo_tval == TV_ARROW) || (ammo_tval == TV_BOLT))) ammo_sval++;
			
			/* Hack - heavy / special flasks become gunpowder */
			if ((d_dice > 16) && (ammo_tval == TV_FLASK)) ammo_sval = SV_FLASK_GUNPOWDER;
		}
		else
		{
			/* Get sval and kind */
			ammo_sval = method_ptr->ammo_sval;
			ammo_kind = method_ptr->ammo_kind;
		}

		/* Scan monster inventory */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Important -- skip artifacts */
			if (o_ptr->name1) continue;

			/* Check if ammo */
			if (o_ptr->tval == ammo_tval) ammo = this_o_idx;

			/* Prefer correct ammo */
			if (o_ptr->sval == ammo_sval) break;
		}

		/* Hack - Checking a single blow */
		if ((blow >= 0) && !(ammo)) return (-1);

		/* Monster has ammo */
		if (ammo != 0) continue;

		/* Monster not creating ammo */
		if (!created) continue;

		/* Create some ammo for the monster */
		if (!ammo_kind) ammo_kind = lookup_kind(ammo_tval, ammo_sval);

		/* Paranoia */
		if (!ammo_kind) return (-1);

		/* Get the object body */
		o_ptr = &object_type_body;

		/* Prepare the item */
		object_prep(o_ptr, ammo_kind);

		/* Give uniques maximum shots */
		if (r_ptr->flags1 & (RF1_UNIQUE)) o_ptr->number = MIN(59, (byte)r_ptr->level);

		/* Archers get more shots */
		else if (r_ptr->flags2 & (RF2_ARCHER)) o_ptr->number += (byte)MIN(59,damroll(2, (r_ptr->level + 1) / 2));

		/* Give the monster some shots */
		else o_ptr->number = (byte)rand_range(1, (r_ptr->level + 1) / 2);

		/* Flavour spores */
		if (o_ptr->tval == TV_EGG) o_ptr->name3 = m_ptr->r_idx;

		/* Boulder / flask throwers get less shots */
		if ((ammo_tval == TV_JUNK) || (ammo_tval == TV_FLASK) || (ammo_tval == TV_POTION)) o_ptr->number = (o_ptr->number + 1) / 2;

		/* Sense magic */
		sense_magic(o_ptr,cp_ptr->sense_type,TRUE, TRUE);

		/* Auto-id average items */
		if (o_ptr->feeling == INSCRIP_AVERAGE) object_bonus(o_ptr, TRUE);

		/* Auto-inscribe if necessary */
		if ((adult_auto) || (object_aware_p(o_ptr))) o_ptr->note = k_info[ammo_kind].note;

		/* Apply obvious flags */
		object_obvious_flags(o_ptr, TRUE);

		/* Monster carries the object */
		ammo = monster_carry(m_idx, o_ptr);

		return (ammo);
	}

	/* Monster has no ammo and needs it */
	if (ammo_tval && !ammo) return (-1);

	return (ammo);
}



/*
 * Attempt to place a monster of the given race at the given location.
 *
 * To give the player a sporting chance, any monster that appears in
 * line-of-sight and is extremely dangerous can be marked as
 * "FORCE_SLEEP", which will cause them to be placed with low energy,
 * which often (but not always) lets the player move before they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * XXX XXX XXX Use special "here" and "dead" flags for unique monsters,
 * remove old "cur_num" and "max_num" fields.
 *
 * XXX XXX XXX Actually, do something similar for artifacts, to simplify
 * the "preserve" mode, and to make the "what artifacts" flag more useful.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code.
 */
static bool place_monster_one(int y, int x, int r_idx, bool slp, u32b flg)
{
	monster_race *r_ptr;

	monster_type *n_ptr;
	monster_type monster_type_body;

	cptr name;

	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the feature */
	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	int i;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Paranoia */
	if (!in_bounds(y, x)) return (FALSE);

	/* Require empty space */
	if (!cave_empty_bold(y, x))
	{
		return (FALSE);
	}

	/* Require monster can pass and survive on terrain */
	if (place_monster_here(y, x, r_idx) <= MM_FAIL)
	{
		return (FALSE);
	}

	/* Hack -- no creation on glyph of warding */
	if (f_ptr->flags1 & (FF1_GLYPH)) return (FALSE);

	/* MegaHack: Handle forms of Sauron */
	if ((r_idx == SAURON_TRUE) ||
			((r_idx >= SAURON_FORM) &&
			(r_idx < SAURON_FORM + MAX_SAURON_FORMS)))
	{
		/* XXX We've already picked a form. Just verifying that we can use it */
		if (!sauron_shape(0)) return (FALSE);
	}


	if (!r_idx) msg_print("Bug: no monster!");

	/* Paranoia */
	if (!r_idx) return (FALSE);

	/* Race */
	r_ptr = &r_info[r_idx];

	/* Paranoia */
	if (!r_ptr->name) return (FALSE);

	/* Name */
	name = (r_name + r_ptr->name);


	/* Hack -- "unique" monsters must be "unique" */
	if ((r_ptr->flags1 & (RF1_UNIQUE)) && (r_ptr->cur_num >= r_ptr->max_num))
	{
		/* Cannot create */
		return (FALSE);
	}


	/* Depth monsters may NOT be created at a different depth */
	if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && (p_ptr->depth != r_ptr->level))
	{
		/* Cannot create */
		return (FALSE);
	}

	
	/* Powerful monster */
	if (r_ptr->level > p_ptr->depth)
	{
		/* Unique monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			/* Message for cheaters */
			if (cheat_hear) msg_format("Deep Unique (%s).", name);

			/* Boost rating by twice delta-depth */
			rating += (r_ptr->level - p_ptr->depth) * 2;
		}

		/* Normal monsters */
		else
		{
			/* Message for cheaters */
			if (cheat_hear) msg_format("Deep Monster (%s).", name);

			/* Boost rating by delta-depth */
			rating += (r_ptr->level - p_ptr->depth);
		}
	}

	/* Note the monster */
	else if (r_ptr->flags1 & (RF1_UNIQUE))
	{
		/* Unique monsters induce message */
		if (cheat_hear) msg_format("Unique (%s).", name);
	}


	/* Get local monster */
	n_ptr = &monster_type_body;

	/* Clean out the monster */
	(void)WIPE(n_ptr, monster_type);


	/* Save the race */
	n_ptr->r_idx = r_idx;

	/* Town levels have some special rules */
	if ((level_flag & (LF1_TOWN)) != 0)
	{
		/* Hack -- harmless townsmen are not threatening */
		if (r_ptr->flags9 & (RF9_TOWNSFOLK)) n_ptr->mflag |= (MFLAG_TOWN);

		/* Mega-hack -- no nasty town dwellers when starting out */
		else if ((turn < 1000) && ((flg & (MFLAG_ALLY)) == 0)) return (FALSE);
	}

	/* Enforce sleeping if needed */
	if (slp && r_ptr->sleep)
	{
		int val = r_ptr->sleep;
		n_ptr->csleep = ((val * 2) + (s16b)randint(val * 10));
	}

	/* Hack -- allow uniques to have their stats drained multiple times */
	if (r_ptr->flags1 & (RF1_UNIQUE))
	{
		/* Uniques always start out the best possible */
		n_ptr->mflag |= (MFLAG_STRONG | MFLAG_SMART | MFLAG_WISE | MFLAG_SKILLFUL | MFLAG_HEALTHY);
	}
	/* Hack -- small racial variety - see the mflag defines */
	else
	{
		u32b variety = rand_int(0x1000);

		/* Set initially variation */
		n_ptr->mflag |= variety << 16;

		/* Reroll */
		variety = rand_int(32);

		/* Adjust frequencies so 1/4 better, 1/2 average, 1/4 worse */
		if ((n_ptr->mflag & (MFLAG_SLOW)) && (variety & 0x01)) n_ptr->mflag &= ~(MFLAG_SLOW | MFLAG_FAST);
		if ((n_ptr->mflag & (MFLAG_WEAK)) && (variety & 0x02))  n_ptr->mflag &= ~(MFLAG_WEAK | MFLAG_STRONG);
		if ((n_ptr->mflag & (MFLAG_STUPID)) && (variety & 0x04))  n_ptr->mflag &= ~(MFLAG_STUPID | MFLAG_SMART);
		if ((n_ptr->mflag & (MFLAG_NAIVE)) && (variety & 0x08)) n_ptr->mflag &= ~(MFLAG_NAIVE | MFLAG_WISE);
		if ((n_ptr->mflag & (MFLAG_CLUMSY)) && (variety & 0x10))  n_ptr->mflag &= ~(MFLAG_CLUMSY | MFLAG_SKILLFUL);
		if ((n_ptr->mflag & (MFLAG_SICK))  && (variety & 0x20)) n_ptr->mflag &= ~(MFLAG_SICK | MFLAG_HEALTHY);
	}

	/* Apply flags from caller */
	n_ptr->mflag |= flg;

	/* Hack -- ensure quest monsters are generated appropriately */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		quest_type *q_ptr = &q_list[i];
		quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);

		/* Hack -- player must have ended up allying this monster */
		if (q_ptr->stage <= QUEST_ACTION) continue;

		/* Quest doesn't occur on this level */
		if ((qe_ptr->dungeon) && ((qe_ptr->dungeon != p_ptr->dungeon) ||
			(qe_ptr->level != (p_ptr->depth - min_depth(p_ptr->dungeon))))) continue;

		/* Mark monsters with the results of various quests */
		if ((qe_ptr->race) && (qe_ptr->race == n_ptr->r_idx))
		{
			if (qe_ptr->flags & (EVENT_ALLY_RACE)) n_ptr->mflag |= (MFLAG_ALLY);
			if (qe_ptr->flags & (EVENT_HATE_RACE)) n_ptr->mflag &= ~(MFLAG_ALLY | MFLAG_IGNORE);
			if (qe_ptr->flags & (EVENT_FEAR_RACE)) n_ptr->monfear = 100;
			if (qe_ptr->flags & (EVENT_HEAL_RACE)) n_ptr->mflag |= (MFLAG_STRONG | MFLAG_SMART | MFLAG_WISE | MFLAG_SKILLFUL | MFLAG_HEALTHY);
			if (qe_ptr->flags & (EVENT_BANISH_RACE)) return (FALSE);

			/* TODO: Drop bodies around if race killed */
			if (qe_ptr->flags & (EVENT_KILL_RACE)) return (FALSE);
		}
	}

	/* Notice these monsters turn they are created */
	if (flg & (MFLAG_MARK | MFLAG_SHOW))
	{
		/* Parnoia */
		flg |= (MFLAG_MARK | MFLAG_SHOW);
		
		/* Optimize -- Repair flags */
		repair_mflag_mark = repair_mflag_show = TRUE;
	}

	/* If in the dungeon, and not a town monster or created prevent summoning */
	if ((character_dungeon) && ((n_ptr->mflag & (MFLAG_TOWN | MFLAG_MADE)) == 0))
	{
		/* Monster must wait a while until summoning anything if summoned/wandering */
		/* Now uses charisma */
		n_ptr->summoned = 400 - 3 * adj_chr_gold[p_ptr->stat_ind[A_CHR]];
	}

	/* Created allies do not carry treasure */
	if (flg & (MFLAG_ALLY))
	{
		n_ptr->mflag |= (MFLAG_MADE);
	}
	/* Good monsters - note that these do carry treasure */
	else if (((r_ptr->flags9 & (RF9_GOOD)) != 0) && ((adult_evil) == 0))
	{
		n_ptr->mflag |= (MFLAG_ALLY | MFLAG_IGNORE);
		n_ptr->summoned = 0;
	}
	
	/* Force monster to wait for player */
	if (r_ptr->flags1 & (RF1_FORCE_SLEEP))
	{
		/* Give almost no starting energy (avoids clumped movement) */
		n_ptr->energy = (byte)rand_int(10);
	}

	else
	{
		/* Give a random starting energy */
		n_ptr->energy = (byte)rand_int(50);
	}

	/* Some monsters radiate lite when born */
	if ((r_ptr->flags2 & (RF2_HAS_LITE))
			|| ((r_ptr->flags2 & (RF2_NEED_LITE)) && !(p_ptr->cur_lite)))
	{
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Place the monster in the dungeon */
	if (!monster_place(y, x, n_ptr)) return (FALSE);

	/* Important - reget the n_ptr once placed */
	n_ptr = &m_list[cave_m_idx[y][x]];

	/* Calculate the monster hp */
	n_ptr->maxhp = calc_monster_hp(cave_m_idx[y][x]);

	/* And start out fully healthy */
	n_ptr->hp = n_ptr->maxhp;

	/* And start out with full mana */
	n_ptr->mana = r_ptr->mana;

	/* Calculate the monster_speed*/
	n_ptr->mspeed = calc_monster_speed(cave_m_idx[y][x]);

	/* Give the monster some ammunition */
	(void)find_monster_ammo(cave_m_idx[y][x], -1, TRUE);

	/* Success */
	return (TRUE);
}



/*
 * Attempt to place a group of monsters around the given location.
 *
 * Hack -- A group of monsters counts as a single individual for the
 * level rating.
 */
static bool place_monster_group(int y, int x, int r_idx, bool slp,
	s16b group_size, u32b flg)
{
	monster_race *r_ptr = &r_info[r_idx];

	int old, n, i;
	int start;
	int adjust;

	int hack_n = 0;

	byte hack_y[GROUP_MAX];
	byte hack_x[GROUP_MAX];

	/* Hack -- don't adjust if two monsters in group */
	if (group_size > 2)
	{
		/* Hard monsters, smaller groups */
		if (r_ptr->level > p_ptr->depth)
		{
			adjust = (r_ptr->level - p_ptr->depth) / 2;
			group_size -= (s16b)randint(adjust);
		}

		/* Easier monsters, bigger groups */
		if (p_ptr->depth > r_ptr->level)
		{
			adjust = (p_ptr->depth - r_ptr->level) / 2;
			group_size += (s16b)randint(adjust);
		}
	}

	/* Minimum size */
	if (group_size < 1) group_size = 1;

	/* Maximum size */
	if (group_size > GROUP_MAX) group_size = GROUP_MAX;


	/* Save the rating */
	old = rating;

	/* Start on the monster */
	hack_n = 1;
	hack_x[0] = x;
	hack_y[0] = y;

	/* Puddle monsters, breadth first, up to group_size */
	for (n = 0; (n < hack_n) && (hack_n < group_size); n++)
	{
		/* Grab the location */
		int hx = hack_x[n];
		int hy = hack_y[n];

		/* Random direction */
		start = rand_int(8);

		/* Check each direction, up to group_size */
		for (i = start; (i < 8 + start) && (hack_n < group_size); i++)
		{
			int mx = hx + ddx_ddd[i % 8];
			int my = hy + ddy_ddd[i % 8];

			/* Attempt to place another monster */
			if (place_monster_one(my, mx, r_idx, slp, flg))
			{
				/* Add it to the "hack" set */
				hack_y[hack_n] = my;
				hack_x[hack_n] = mx;
				hack_n++;
			}
		}
	}

	/* Hack -- restore the rating */
	rating = old;


	/* Success */
	return (TRUE);
}

/*
 * Hack -- escorts for the player on a BATTLE level.
 */
static bool comrade_okay(int r_idx)
{
	monster_race *z_ptr = &r_info[r_idx];
	byte a_char = t_info[p_ptr->dungeon].a_char;
	u32b a_flag = t_info[p_ptr->dungeon].a_flag;

	/* Uniques are never spawned as allies */
	if(z_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Evil creatures are spawned as allies only if the player is evil. */
	if(((p_ptr->cur_flags4 & (TR4_EVIL)) != 0) != ((z_ptr->flags3 & (RF3_EVIL)) != 0)) return (FALSE);

	/* No ally data, resort to townsfolk only */
	if(!a_char && !a_flag) return ((z_ptr->d_char == 't') &&
				((z_ptr->flags2 & (RF2_SNEAKY)) == 0));

	/* Graphical match */
	if(a_char == z_ptr->d_char)
	{
		if (cheat_hear) msg_format("Accepted monster %s as player ally in %s based on char.", r_name + z_ptr-> name, t_name + t_info[p_ptr->dungeon].name);
		return (TRUE);
	}
	u32b flagmatch = ((a_flag & z_ptr->flags3 & 0x0000FFFF) | (a_flag & z_ptr->flags9 & 0xFFFF0000));
	/* Racial flag match */
	if(flagmatch)
	{
		if (cheat_hear) msg_format("Criteria for flag match is %#010lx.", a_flag);
		if (cheat_hear) msg_format("Accepted monster %s as player ally in %s based on flags (%#010lx).",
				r_name + z_ptr-> name, t_name + t_info[p_ptr->dungeon].name,
				flagmatch);
		return (TRUE);
	}

	/* Disallowed */
	return (FALSE);
}

/*
 * Hack -- help pick an escort type
 */
static int place_monster_idx = 0;

/*
 * Hack -- help pick an escort type
 */
static bool place_monster_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[place_monster_idx ? place_monster_idx  : rp_ptr->r_idx];

	monster_race *z_ptr = &r_info[r_idx];

	/* Hack -- player escort for non-monster races */
	if ((!place_monster_idx) && (!rp_ptr->r_idx)) return comrade_okay(r_idx);

	/* Group monsters require similar "group" */
	if (r_ptr->grp_idx)
	{
		if (z_ptr->grp_idx != r_ptr->grp_idx) return (FALSE);
	}

	/* Require similar "race" */
	else if (z_ptr->d_char != r_ptr->d_char) return (FALSE);

	/* Skip more advanced monsters */
	if ((place_monster_idx) && (z_ptr->level > r_ptr->level)) return (FALSE);

	/* Skip unique monsters */
	if (z_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);

	/* Paranoia -- Skip identical monsters */
	if (place_monster_idx == r_idx) return (FALSE);

	/* Okay */
	return (TRUE);
}

/*
 * Attempt to place an escort of monsters around the given location
 */
static void place_monster_escort(int y, int x, int leader_idx, bool slp, u32b flg)
{
	int escort_size, escort_idx;
	int n, i;

	/* Random direction */
	int start;

	monster_race *r_ptr = &r_info[leader_idx];

	int hack_n = 0;

	byte hack_y[GROUP_MAX];
	byte hack_x[GROUP_MAX];

	/* Save previous monster restriction value. */
	bool (*get_mon_num_hook_temp)(int r_idx) = get_mon_num_hook;

	int old_monster_level = monster_level;

	/* Calculate the number of escorts we want. */
	if ((r_ptr->flags1 & (RF1_ESCORTS)) || (level_flag & (LF1_BATTLE))) escort_size = rand_range(12, 18);
	else escort_size = rand_range(4, 6);

	/* Can never have more escorts than maximum group size */
	if (escort_size > GROUP_MAX) escort_size = GROUP_MAX;


	/* Use the leader's monster type to restrict the escorts. */
	place_monster_idx = leader_idx;

	/* Set the escort hook */
	get_mon_num_hook = place_monster_okay;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Build monster table, get index of first escort */
	escort_idx = get_mon_num(monster_level);

	/* Start on the monster */
	hack_n = 1;
	hack_x[0] = x;
	hack_y[0] = y;


	/* Puddle monsters, breadth first, up to escort_size */
	for (n = 0; (n < hack_n) && (hack_n < escort_size); n++)
	{
		/* Grab the location */
		int hx = hack_x[n];
		int hy = hack_y[n];

		/* Random direction */
		start = rand_int(8);

		/* Check each direction, up to escort_size */
		for (i = start; (i < 8 + start) && (hack_n < escort_size); i++)
		{
			int mx = hx + ddx_ddd[i % 8];
			int my = hy + ddy_ddd[i % 8];

			/* Place a group of escorts if needed */
			if (((r_info[escort_idx].level < old_monster_level - 9) || (r_info[escort_idx].flags1 & (RF1_FRIENDS))) &&
				!place_monster_group(my, mx, escort_idx, slp, (s16b)(rand_range(3, 5)), flg))
			{
				continue;
			}

			/* Place a few escorts if needed */
			else if (((r_info[escort_idx].level < old_monster_level - 4) || (r_info[escort_idx].flags1 & (RF1_FRIEND))) &&
				!place_monster_group(my, mx, escort_idx, slp, 2, flg))
			{
				continue;
			}

			/* Attempt to place another monster */
			if (!place_monster_one(my, mx, escort_idx, slp, flg))
			{
				continue;
			}

			/* Add grid to the "hack" set */
			hack_y[hack_n] = my;
			hack_x[hack_n] = mx;
			hack_n++;

			/* Get index of the next escort */
			escort_idx = get_mon_num(monster_level);
		}

		/* No luck -- boost */
		if (!escort_idx)
			monster_level += 3;

		if (monster_level > 70)
			break;
	}

	monster_level = old_monster_level;

	/* Return to previous monster restrictions (usually none) */
	get_mon_num_hook = get_mon_num_hook_temp;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* XXX - rebuild monster table */
	(void)get_mon_num(monster_level);
}


/*
 * Place the player in the dungeon XXX XXX
 *
 * Note similarity to player_swap.
 */
s16b player_place(int y, int x, bool escort_allowed)
{
	/* Paranoia XXX XXX */
	if (cave_m_idx[y][x] != 0) return (0);

	/* Mark cave grid */
	cave_m_idx[y][x] = -1;

	/* Update player position */
	p_ptr->py = y;
	p_ptr->px = x;

	/* Update positional information*/
	player_position();

	/* Place escorts on battle-field */
	if ((escort_allowed) && (level_flag & (LF1_BATTLE)))
	{
		/* Hack -- Switch off ecology */
		bool old_ecology_ready = cave_ecology.ready;

		/* Ecology off */
		cave_ecology.ready = FALSE;

		/* Help the player out */
		monster_level += 5;

		place_monster_escort(y, x, 0, FALSE, (MFLAG_ALLY | MFLAG_TOWN));

		msg_print("You are joined by companions in battle.");

		monster_level -= 5;

		cave_ecology.ready = old_ecology_ready;
	}

	/* Success */
	return (-1);
}


/*
 * Hack -- the "type" of the current "summon specific"
 */
static int summon_specific_type = 0;


/*
 * Helper function summon a monster based on what level flags exist.
 *
 * XXX This function relies on a whole lot of horrible coincidences
 * such as no overlapping values for the LF1_ flags and relevant RF3_
 * flags, and (even worse) LF1_NATURAL matching RF3_ANIMAL.
 */
static bool summon_specific_level_match(int r_idx, u32b *summon_flag)
{
	/* Be careful not to actually set summon_flag_type here */
	u32b temp_flag = *summon_flag;
	u32b temp_flag2 = 0L;
	int k = 0;
	bool okay = FALSE;

	/* Get the monster */
	monster_race *r_ptr = &r_info[r_idx];

	/* Watery monsters - not on lava, ice levels */
	if ((!temp_flag) || ((temp_flag & (LF1_WATER | LF1_LAVA | LF1_ICE)) == (LF1_WATER)))
	{
		/*
		 * Water elementals, frogs and fish, monsters names
		 * starting with water, swamp, river or sea.
		 */
		if ((r_ptr->grp_idx == 12) ||
			(strchr("F", r_ptr->d_char)) ||
			(strstr(r_name + r_ptr->name, "Water")) ||
			(strstr(r_name + r_ptr->name, "Sea")) ||
			(strstr(r_name + r_ptr->name, "River")) ||
			(strstr(r_name + r_ptr->name, "Swamp")))
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_WATER);
		}
	}

	/* Lava monsters */
	if ((!temp_flag) || (temp_flag & (LF1_LAVA)))
	{
		/*
		 * Fire / mama elementals, fire breathers, first
		 * attack is fire or lava, monsters names
		 * starting with fire, hell (magma not needed).
		 */
		if ((r_ptr->grp_idx == 14) ||
			(r_ptr->grp_idx == 19) ||
			(r_ptr->flags4 & (RF4_BRTH_FIRE)) ||
			(r_ptr->blow[0].effect == (GF_FIRE)) ||
			(r_ptr->blow[0].effect == (GF_LAVA)) ||
			(strstr(r_name + r_ptr->name, "Fire")) ||
			(strstr(r_name + r_ptr->name, "Hell")) /* ||
			(strst(r_name + r_ptr->name, "Magma")) */)
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_LAVA);
		}
	}

	/* Ice monsters */
	if ((!temp_flag) || (temp_flag & (LF1_ICE)))
	{
		/*
		 * Ice elementals, frost breathers, first attack is
		 * cold or ice, monsters names starting with cold, ice.
		 */
		if ((r_ptr->grp_idx == 18) ||
			(r_ptr->flags4 & (RF4_BRTH_COLD)) ||
			(r_ptr->blow[0].effect == (GF_COLD)) ||
			(r_ptr->blow[0].effect == (GF_ICE)) ||
			(strstr(r_name + r_ptr->name, "Cold")) ||
			(strstr(r_name + r_ptr->name, "Ice")) /* ||
			(strst(r_name + r_ptr->name, "Magma")) */)
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_ICE);
		}
	}

	/* Acid monsters - not on lava levels */
	if ((!temp_flag) || ((temp_flag & (LF1_ACID | LF1_LAVA)) == (LF1_ACID)))
	{
		/*
		 * Ooze, acid breathers. Anything else would
		 * be too annoying.
		 */
		if ((r_ptr->grp_idx == 16) ||
			(r_ptr->flags4 & (RF4_BRTH_ACID)))
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_ACID);
		}
	}

	/* Earth monsters -- oops. We really need an earth level flag
	 * to indicate levels which are filled with sand, dirt, rubble.
	 * This would also be used in get_mon_prep to indicate levels
	 * we should favour CAN_DIG monsters on.
	 * We also add these to acid levels to ensure sufficient
	 * variety of monsters on acid levels.
	 */
	if ((!temp_flag) || ((temp_flag & (LF1_ACID | LF1_LAVA)) == (LF1_ACID)))
	{
		/*
		 * Ooze, earth elementals, acid, shard breathers, monsters
		 * names starting with earth, rock, crystal.
		 */
		if ((r_ptr->grp_idx == 13) ||
			(r_ptr->flags4 & (RF4_BRTH_SHARD)) ||
			(strstr(r_name + r_ptr->name, "Earth")) ||
			(strstr(r_name + r_ptr->name, "Rock")) ||
			(strstr(r_name + r_ptr->name, "Crystal")))
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_ACID);
		}
	}

	/* XXX To do: Electricity monsters */

	/* XXX To do: Fire monsters for burning oil levels */

	/* Oil monsters e.g. machines */
	if ((!temp_flag) || ((temp_flag & (LF1_WATER | LF1_LAVA | LF1_ICE | LF1_ACID | LF1_OIL)) == (LF1_OIL)))
	{
		/* Golems, animated weapons */
		if ((strchr("g|\\/~", r_ptr->d_char)))
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_OIL);
		}
	}

	/* Air monsters on chasm only levels */
	if ((!temp_flag) || ((temp_flag & (LF1_WATER | LF1_LAVA | LF1_ICE | LF1_ACID | LF1_OIL | LF1_CHASM)) == (LF1_CHASM)))
	{
		/*
		 * Air elementals, flyers, tornadoes, monsters
		 * names starting with air, wind, sky.
		 */
		if ((r_ptr->grp_idx == 11) ||
			(strchr("~", r_ptr->d_char)) ||
			(strstr(r_name + r_ptr->name, "Air")) ||
			(strstr(r_name + r_ptr->name, "Wind")) ||
			(strstr(r_name + r_ptr->name, "Sky")))
		{
			okay |= (TRUE);
			if (!rand_int(++k)) temp_flag2 = (LF1_CHASM);
		}

	}

	/* Plants, insects or animals on living levels */
	if ((!temp_flag) || ((temp_flag & (LF1_WATER | LF1_LAVA | LF1_ICE | LF1_ACID | LF1_OIL | LF1_LIVING)) == (LF1_LIVING)))
	{
		/* Hack -- we create different ecology types */
		if ((r_ptr->flags3 & (RF3_ANIMAL)) && ((temp_flag & (RF3_PLANT)) == 0))
		{
			if (r_ptr->flags8 & (RF8_HAS_FUR | RF8_HAS_FEATHER))
			{
				if ((temp_flag & (RF3_INSECT)) == 0)
				{
					okay |= (TRUE);
					/* 'Modern' ecology - feathered and furred allowed */
					if (!rand_int(++k)) temp_flag2 = (LF1_LIVING | RF3_ANIMAL);
				}
			}
			else
			{
				okay |= (TRUE);
				/* 'Prehistoric' ecology - scaley, skinned, plants and insects only */
				if (!rand_int(++k)) temp_flag2 = (LF1_LIVING | RF3_INSECT);
			}
		}

		/* Insects are always prehistoric or later */
		if ((r_ptr->flags3 & (RF3_INSECT)) && ((temp_flag & (RF3_PLANT)) == 0))
		{
			okay |= (TRUE);
			/* 'Prehistoric' ecology - scaley, skinned, plants and insects only */
			if (!rand_int(++k)) temp_flag2 = (LF1_LIVING | RF3_INSECT);
		}

		/* Most plants are prehistoric or later -- flowering plants require insects */
		if ((r_ptr->flags3 & (RF3_PLANT)) && ((temp_flag & (RF3_PLANT)) == 0))
		{
			okay |= (TRUE);
			/* 'Prehistoric' ecology - scaley, skinned, plants and insects only */
			if (!rand_int(++k)) temp_flag2 = (LF1_LIVING | RF3_INSECT);
		}

		/* Slime ecology? - Ugh. Not pleasant for the player... */
		if ((strchr("ijm,", r_ptr->d_char)) && ((temp_flag & (RF3_ANIMAL | RF3_INSECT)) == 0))
		{
			okay |= (TRUE);
			/* Slime ecology */
			if (!rand_int(++k)) temp_flag2 = (LF1_LIVING | RF3_PLANT);
		}
	}

	/* Return revised flag */
	if (okay) *summon_flag = temp_flag2;

	return (okay);
}


/*
 * Hack -- help decide if a monster race is "okay" to summon
 *
 * XXX We pass through a number of parameters as global
 * variables. If these are not set, it means we are looking
 * for a candidate race, which all subsequent summonings from
 * this spell will be based on. e.g. Find me a monster, and
 * summon one or more of its kin.
 */
static bool summon_specific_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	bool okay = FALSE;

	/* Hack -- try to minimise annoyance if monsters summon monsters */
	if (summoner && summon_strict)
	{
		/* This prevents 'circular' chain summoning */
		if ((r_info[r_idx].flags4 & (RF4_SUMMON_MASK) ||
			  r_info[r_idx].flags5 & (RF5_SUMMON_MASK) ||
			  r_info[r_idx].flags6 & (RF6_SUMMON_MASK) ||
			  r_info[r_idx].flags7 & (RF7_SUMMON_MASK))
			 && r_info[summoner].level <= r_info[r_idx].level)
			return (FALSE);
	}

	/* Hack -- no specific type specified */
	if (!summon_specific_type) return (TRUE);

	/* Check our requirements */
	switch (summon_specific_type)
	{
		case SUMMON_KIN:
		case RAISE_DEAD:
		{
			if (summon_char_type)
			{
				okay = ((r_ptr->d_char == summon_char_type) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));

				/* Hack - some dissimilar chars are actually similar */
				if (summon_specific_type == SUMMON_KIN)
				{
					/* Animated weapons */
					if (strchr ("\\/|~", summon_char_type)) okay |= (strchr ("\\/|~", summon_char_type)) != 0;

					/* Mimics */
					if (strchr ("!&=_?\"", summon_char_type)) okay |= (strchr ("\\/|~", summon_char_type)) != 0;

					/* Trees and plants  */
					if (strchr (":;", summon_char_type)) okay |= (strchr ("\\/|~", summon_char_type)) != 0;
				}
			}
			else
			{
				okay = TRUE;
			}
			break;
		}

		/* Hack -- we combine two different flag types */
		case ANIMATE_DEAD:
		{
			/* Hack -- animate skulls */
			if ((summon_char_type == '~') && (summon_attr_type == TERM_WHITE))
			{
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match on undead */
					(r_ptr->flags3 & (RF3_UNDEAD)) &&

					/* Match on name */
					((strstr(r_name + r_ptr->name, "Skull")) ||

					/* Match on name */
					(strstr(r_name + r_ptr->name, "skull")) ));

			}
			else if (summon_char_type && summon_flag_type)
			{
				u32b summon_flag3_type = summon_flag_type & (0x0000FFFFL);
				u32b summon_flag9_type = summon_flag_type & (0xFFFF0000L);

				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match char */
					(((r_ptr->d_char == summon_char_type) &&

					/* Match flag */
					((r_ptr->flags3 & (summon_flag3_type)) ||

					/* Match flag */
					(r_ptr->flags9 & (summon_flag9_type)) )) ||

					/* Also can match some high level undead */
					((summon_char_type == 's') && (r_ptr->d_char == 'L')) ||

					((summon_char_type == 'z') && (r_ptr->d_char == 'N'))) );
			}
			else if (summon_flag_type)
			{
				u32b summon_flag3_type = summon_flag_type & (0x0000FFFFL);
				u32b summon_flag9_type = summon_flag_type & (0xFFFF0000L);

				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match 'animated' undead */
					( ((strchr("sz", r_ptr->d_char)) &&

					/* Match any */
					((r_ptr->flags3 & (summon_flag3_type)) ||

					/* Match any */
					(r_ptr->flags9 & (summon_flag9_type)) )) ||

					/* Also can match some high level undead without restriction */
					(strchr("LN", r_ptr->d_char)) ));
			}
			else if (summon_char_type && summon_attr_type)
			{
				/* Match char */
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match attr */
					(r_ptr->d_attr == summon_attr_type) &&

					/* Match char */
					(r_ptr->d_char == summon_char_type));
			}
			else if (summon_char_type)
			{
				/* Match char */
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match char */
					(r_ptr->d_char == summon_char_type));
			}
			else
			{
				/* Match 'animated' undead */
				okay = ((strchr("szLN", r_ptr->d_char)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_PLANT:
		{
			/* Hack -- exclude trees */
			okay = ((r_ptr->flags3 & (RF3_PLANT)) &&
				(r_ptr->d_char != ':') &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_INSECT:
		{
			/* Hack -- exclude spiders */
			okay = ((r_ptr->flags3 & (RF3_INSECT)) &&
				(r_ptr->d_char != 'S') &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		/* Hack -- try to summon birds, beasts or reptiles based on summoner */
		case SUMMON_ANIMAL:
		{
			/* Hack for undead summoners */
			if (summon_flag_type & (RF8_HAS_SKELETON))
			{
				okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)) &&
					(r_ptr->flags3 & (RF3_UNDEAD)));
			}
			else if (summon_flag_type)
			{
				okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Hack -- exclude undead */
					!(r_ptr->flags3 & (RF3_UNDEAD)) &&

					/* Hack -- exclude hounds */
					(r_ptr->d_char != 'C') && (r_ptr->d_char != 'Z') &&

					/* Check 'skin' */
					((r_ptr->flags8 & (RF8_SKIN_MASK)) ?

					/* Has feathers, scales or fur? */
					((r_ptr->flags8 & (summon_flag_type)) ? TRUE : FALSE) :

					/* Has none of the above - treat as scales... */
					((summon_flag_type & (RF8_HAS_SCALE)) ? TRUE : FALSE)));
			}
			else
			{
				okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_HOUND:
		{
			okay = (((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z')) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_SPIDER:
		{
			okay = ((r_ptr->d_char == 'S') &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		/* Hack -- mage priests only hang out with mage priests, but
			warrior priests hang out with warriors and priests */
		case SUMMON_CLASS:
		{

			if (summon_flag_type)
			{
				u32b summon_flag2_type = summon_flag_type & (RF2_CLASS_MASK);
				u32b summon_flag3_type = summon_flag_type & (RF3_EVIL);

				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match evil or not evil */
					((r_ptr->flags3 & (RF3_EVIL)) == summon_flag3_type) &&

					/* Check 'class' is mage or priest and we want a mage and/or priest */
					((r_ptr->flags2 & (RF2_MAGE | RF2_PRIEST)) && (summon_flag2_type & (RF2_MAGE | RF2_PRIEST)) ?

					/* Exact match on mage / priest component */
					((r_ptr->flags2 & (RF2_MAGE | RF2_PRIEST)) == (summon_flag2_type & (RF2_MAGE | RF2_PRIEST)) ? TRUE : FALSE) :

					/* Match any other */
					((r_ptr->flags2 & (summon_flag2_type)) ? TRUE : FALSE) ));
			}
			else
			{
				okay = ((r_ptr->flags2 & (RF2_CLASS_MASK)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		/* Hack -- we combined two different flag types */
		case SUMMON_RACE:
		{
			if (summon_flag_type)
			{
				u32b summon_flag3_type = summon_flag_type & (RF3_RACE_MASK);
				u32b summon_flag9_type = summon_flag_type & (RF9_RACE_MASK);

				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match any */
					((r_ptr->flags3 & (RF3_RACE_MASK)) == summon_flag3_type) &&

					/* Match any */
					((r_ptr->flags9 & (RF9_RACE_MASK)) == summon_flag9_type));
			}
			else
			{
				okay = ((r_ptr->flags3 & (RF3_RACE_MASK)) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		/* Match on monsters specific to the type of level
		 * e.g. lava monsters on lava level etc. If no flags,
		 * we just match on the NONLIVING flag.
		 */
		case SUMMON_LEVEL:
		{
			/* Not used here */
			u32b dummy_flag = summon_flag_type;

			/* Use the helper function */
			okay = summon_specific_level_match(r_idx, &dummy_flag);

			/* We have a match */
			if (okay) break;

			/* Fall through */
		}

		/* Match on flag3 */
		case SUMMON_ALIGN:
		{
			if (summon_flag_type)
			{
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match any */
					((r_ptr->flags3 & (RF3_EVIL | RF3_NONLIVING)) == summon_flag_type));
			}
			else
			{
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_COLOUR:
		{
			if ((summon_attr_type) || (summon_flag_type))
			{
				u32b summon_flag1_type = summon_flag_type & (0x0000FFFFL);
				u32b summon_flag9_type = summon_flag_type & (0xFFFF0000L);

				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Hack - Reject index color */
					((r_ptr->flags9 & (RF9_ATTR_INDEX)) == 0) &&

					/* Match colour */
					(((r_ptr->d_attr == summon_attr_type) &&

					/* Match 'metallic-ness' */
					((r_ptr->flags9 & (RF9_ATTR_METAL | RF9_DROP_MINERAL)) == summon_flag9_type) &&

					/* Reject clear or multi-hued */
					((r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_ATTR_MULTI)) == 0)) ||

					/* Match 'clearness or multi-huedness */
					((summon_flag1_type) &&
						((r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_ATTR_MULTI)) == summon_flag1_type))));
			}
			else
			{
				okay = /* Hack - Reject index color */
					(((r_ptr->flags9 & (RF9_ATTR_INDEX)) == 0) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_PREFIX:
		{
			if (strlen(summon_word_type))
			{
				/* Match on prefix */
				okay = prefix(r_name + r_ptr->name, summon_word_type);
			}
			else
			{
				okay = /* Hack - Reject monsters without prefix */
					(!(strchr(r_name + r_ptr->name, ' ')) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_ALL_BUT_PREFIX:
		case SUMMON_SUFFIX:
		{
			if (strlen(summon_word_type))
			{
				/* Match on prefix */
				okay = suffix(r_name + r_ptr->name, summon_word_type);
			}
			else
			{
				okay = /* Hack - Reject monsters without suffix */
					(!(strchr(r_name + r_ptr->name, ' ')) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_INFIX_WYRM_OF:
		{
			if (strlen(summon_word_type))
			{
				/* Match on infix */
				okay = (strstr(r_name + r_ptr->name, summon_word_type) != 0);
			}
			else
			{
				okay = /* Hack - Reject monsters without suffix */
					(!(strchr(r_name + r_ptr->name, ' ')) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case SUMMON_DRAGON_BREATH:
		{
			if (summon_flag_type)
			{
				/* Match on dragon */
				okay = (strchr("AdD",r_ptr->d_char) &&

				/* Hack - Match on undead if requested */
				(((summon_flag_type & (0x0000001L)) &&
						(r_ptr->flags3 & (RF3_UNDEAD))) ||

				/* Match on breath */
				((summon_flag_type & (RF4_BREATH_MASK)) &&
						(r_ptr->flags4 & (RF4_BREATH_MASK)) == summon_flag_type)));
			}
			else
			{
				/* Reject non-dragons */
				okay = ((strchr("AdD",r_ptr->d_char)) != 0);
			}
		}

		case SUMMON_GROUP:
		case ANIMATE_ELEMENT:
		{
			if (summon_group_type)
			{
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&

					/* Match group index */
					(r_ptr->grp_idx == summon_group_type));
			}
			else
			{
				okay = ((r_ptr->grp_idx) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			break;
		}

		case ANIMATE_OBJECT:
		{
			if (summon_attr_type && summon_char_type)
			{
				okay = (((r_ptr->d_attr == summon_attr_type) ||
					(r_ptr->flags1 & (RF1_ATTR_CLEAR)) ||
					(r_ptr->flags9 & (RF9_ATTR_INDEX))) &&
					(r_ptr->d_char == summon_char_type) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else if (summon_attr_type)
			{
				okay = (((r_ptr->d_attr == summon_attr_type) ||
					(r_ptr->flags1 & (RF1_ATTR_CLEAR)) ||
					(r_ptr->flags9 & (RF9_ATTR_INDEX))) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else if (summon_char_type)
			{
				okay = ((r_ptr->d_char == summon_char_type) &&
					!(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else
			{
				okay = (!(r_ptr->flags1 & (RF1_UNIQUE)) &&
					strchr("&:;.,'!_-\\/[]~$%^*(){}+=<>?#",r_ptr->d_char));
			}
			break;
		}

		case ANIMATE_TREE:
		{
			okay = ((r_ptr->d_char == ':') &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_FRIEND:
		{
			if (summon_race_type)
			{
				okay = (r_idx == summon_race_type);
			}
			else
			{
				okay = TRUE;
			}
			break;
		}

		case SUMMON_UNIQUE_FRIEND:
		{
			if (summon_attr_type && summon_char_type)
			{
				okay = ((r_ptr->d_attr == summon_attr_type) &&
					(r_ptr->d_char == summon_char_type) &&
					(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else if (summon_attr_type)
			{
				okay = ((r_ptr->d_attr == summon_attr_type) &&
					(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else if (summon_char_type)
			{
				okay = ((r_ptr->d_char == summon_char_type) &&
					(r_ptr->flags1 & (RF1_UNIQUE)));
			}
			else
			{
				okay = (r_ptr->flags1 & (RF1_UNIQUE));
			}
			break;
		}

		case SUMMON_ORC:
		{
			okay = ((r_ptr->flags3 & (RF3_ORC)) && !(r_ptr->flags3 & ((RF3_RACE_MASK) & ~(RF3_ORC))) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_TROLL:
		{
			okay = ((r_ptr->flags3 & (RF3_TROLL)) && !(r_ptr->flags3 & ((RF3_RACE_MASK) & ~(RF3_TROLL))) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_GIANT:
		{
			okay = ((r_ptr->flags3 & (RF3_GIANT)) && !(r_ptr->flags3 & ((RF3_RACE_MASK) & ~(RF3_GIANT))) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_DEMON:
		{
			okay = ((r_ptr->flags3 & (RF3_DEMON)) && !(r_ptr->flags3 & ((RF3_RACE_MASK) & ~(RF3_DEMON))) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_UNDEAD:
		{
			okay = ((r_ptr->flags3 & (RF3_UNDEAD)) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));

			break;
		}

		case SUMMON_DRAGON:
		{
			okay = ((r_ptr->flags3 & (RF3_DRAGON)) && !(r_ptr->flags3 & ((RF3_RACE_MASK) & ~(RF3_DRAGON))) &&
				!(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_HI_DEMON:
		{
			okay = ((r_ptr->d_char == 'U') &&
					(r_ptr->flags3 & (RF3_DEMON)));
			break;
		}

		case SUMMON_HI_UNDEAD:
		{
			okay = ((r_ptr->d_char == 'L') ||
				(r_ptr->d_char == 'V') ||
				(r_ptr->d_char == 'W'));
			break;
		}

		case SUMMON_HI_DRAGON:
		{
			okay = ((r_ptr->d_char == 'D') ||
				(r_ptr->d_char == 'A'));
			break;
		}

		case SUMMON_WRAITH:
		{
			okay = ((r_ptr->d_char == 'W') &&
				(r_ptr->flags1 & (RF1_UNIQUE)));
			break;
		}

		case SUMMON_UNIQUE:
		{
			if ((r_ptr->flags1 & (RF1_UNIQUE)) != 0) okay = TRUE;
			break;
		}

		case SUMMON_HI_UNIQUE:
		{
			if (((r_ptr->flags1 & (RF1_UNIQUE)) != 0) &&
				(r_ptr->level > (MAX_DEPTH / 2))) okay = TRUE;
			break;
		}


		default:
		{
			break;
		}

	}

	/* Result */
	return (okay);
}


/*
 * Attempt to place a monster of the given race at the given location
 *
 * Monsters may have some friends, or lots of friends.  They may also
 * have a few escorts, or lots of escorts.
 *
 * Note the use of the new "monster allocation table" code to restrict
 * the "get_mon_num()" function to legal escort types.
 */
bool place_monster_aux(int y, int x, int r_idx, bool slp, bool grp, u32b flg)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* MegaHack: Handle forms of Sauron */
	if ((r_idx == SAURON_TRUE) ||
			((r_idx >= SAURON_FORM) &&
			(r_idx < SAURON_FORM + MAX_SAURON_FORMS)))
	{
		r_idx = sauron_shape(0);

		if (!r_idx) return (FALSE);
	}

	/* Place one monster, or fail */
	if (!place_monster_one(y, x, r_idx, slp, r_ptr->flags1 & (RF1_FRIENDS) ? flg | MFLAG_LEADER : flg)) return (FALSE);

	/* Require the "group" flag */
	if (!grp) return (TRUE);

	/* Friends for uniques - we have to reallocate separately to avoid recursion */
	if ((r_ptr->flags1 & (RF1_UNIQUE)) && (r_ptr->flags1 & (RF1_FRIEND | RF1_FRIENDS)))
	{
		int i, n;

		int group_size = (r_ptr->flags1 & (RF1_FRIENDS) ? rand_range(3, 5) : 2);
		int hack_n;

		byte hack_x[6];
		byte hack_y[6];
		s16b hack_r_idx[6];

		/* Start on the monster */
		hack_n = 1;
		hack_x[0] = x;
		hack_y[0] = y;
		hack_r_idx[0] = r_idx;

		/* Save the unique attr and char */
		summon_attr_type = r_ptr->d_attr;
		summon_char_type = r_ptr->d_char;

		/* Save the "summon" type */
		summon_specific_type = SUMMON_UNIQUE_FRIEND;

		/* Puddle monsters, breadth first, up to group_size */
		for (n = 0; (n < hack_n) && (hack_n < group_size); n++)
		{
			/* Grab the location */
			int hx = hack_x[n];
			int hy = hack_y[n];

			/* Random direction */
			int start = rand_int(8);

			/* Require "okay" monsters */
			get_mon_num_hook = summon_specific_okay;

			/* Prepare allocation table */
			get_mon_num_prep();

			/* Check each direction, up to group_size */
			for (i = start; (i < 8 + start) && (hack_n < group_size); i++)
			{
				int mx = hx + ddx_ddd[i % 8];
				int my = hy + ddy_ddd[i % 8];

				/* Pick a monster, using the monster race level */
				hack_r_idx[hack_n] = get_mon_num(r_ptr->level);

				/* Nothing picked */
				if (!hack_r_idx[hack_n]) break;

				/* Attempt to place another monster */
				if (place_monster_one(my, mx, hack_r_idx[hack_n], slp, flg))
				{
					/* Add it to the "hack" set */
					hack_y[hack_n] = my;
					hack_x[hack_n] = mx;
					hack_n++;
				}
			}
		}

		/* Remove restriction */
		get_mon_num_hook = NULL;

		/* Prepare allocation table */
		get_mon_num_prep();

		/* Place escorts after placing unique friends */
		for (n = 0; (n < hack_n) && (hack_n < group_size); n++)
		{
			/* Escorts for certain monsters */
			if ((r_info[hack_r_idx[n]].flags1 & (RF1_ESCORT)) || (r_info[hack_r_idx[n]].flags1 & (RF1_ESCORTS)))
			{
				place_monster_escort(hack_y[n], hack_x[n], hack_r_idx[n], slp, flg);
			}
		}
	}
	/* Friends for certain monsters and battlefields */
	else if (((r_ptr->flags1 & (RF1_FRIENDS)) != 0) || (((level_flag & (LF1_BATTLE)) != 0)))
	{
		/* Attempt to place a large group */
		(void)place_monster_group(y, x, r_idx, slp, (s16b)rand_range(3, 5), flg);
	}
	/* A friend for certain monsters */
	else if (r_ptr->flags1 & (RF1_FRIEND))
	{
		/* Attempt to place a second monster */
		(void)place_monster_group(y, x, r_idx, slp, 2, flg);
	}

	/* Escorts for certain monsters */
	if ((r_ptr->flags1 & (RF1_ESCORT)) || (r_ptr->flags1 & (RF1_ESCORTS)))
	{
		place_monster_escort(y, x, r_idx, slp, flg);
	}

	/* Certain monsters created in giant spider webs */
	if (r_ptr->flags2 & (RF2_HAS_WEB))
	{
		int flg = PROJECT_BOOM | PROJECT_4WAY | PROJECT_4WAX | PROJECT_GRID | PROJECT_THRU | PROJECT_HIDE;

		/* Shoot web out in 8 directions when placed */
		(void)project(SOURCE_BIRTH, r_idx, (r_ptr->level / 10) + 2, 0, y, x, y, x, 0, GF_WEB, flg, 0, 0);
	}

	/* Success */
	return (TRUE);
}


/*
 * Hack -- attempt to place a monster at the given location
 *
 * Attempt to find a monster appropriate to the "monster_level"
 */
bool place_monster(int y, int x, bool slp, bool grp)
{
	int r_idx;

	/* Pick a monster */
	r_idx = get_mon_num(monster_level);

	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster */
	if (place_monster_aux(y, x, r_idx, slp, grp, 0L)) return (TRUE);

	/* Oops */
	return (FALSE);
}




/*
 * XXX XXX XXX Player Ghosts are such a hack, they have been completely
 * removed until Angband 2.8.0, in which there will actually be a small
 * number of "unique" monsters which will serve as the "player ghosts".
 * Each will have a place holder for the "name" of a deceased player,
 * which will be extracted from a "bone" file, or replaced with a
 * "default" name if a real name is not available.  Each ghost will
 * appear exactly once and will not induce a special feeling.
 *
 * Possible methods:
 *   (s) 1 Skeleton
 *   (z) 1 Zombie
 *   (M) 1 Mummy
 *   (G) 1 Polterguiest, 1 Spirit, 1 Ghost, 1 Shadow, 1 Phantom
 *   (W) 1 Wraith
 *   (V) 1 Vampire, 1 Vampire Lord
 *   (L) 1 Lich
 *
 * Possible change: Lose 1 ghost, Add "Master Lich"
 *
 * Possible change: Lose 2 ghosts, Add "Wraith", Add "Master Lich"
 *
 * Possible change: Lose 4 ghosts, lose 1 vampire lord
 *
 * Note that ghosts should never sleep, should be very attentive, should
 * have maximal hitpoints, drop only good (or great) items, should be
 * cold blooded, evil, undead, immune to poison, sleep, confusion, fear.
 *
 * Base monsters:
 *   Skeleton
 *   Zombie
 *   Mummy
 *   Poltergeist
 *   Spirit
 *   Ghost
 *   Vampire
 *   Wraith
 *   Vampire Lord
 *   Shadow
 *   Phantom
 *   Lich
 *
 * This routine will simply extract ghost names from files, and
 * attempt to allocate a player ghost somewhere in the dungeon,
 * note that normal allocation may also attempt to place ghosts,
 * so we must work with some form of default names.
 *
 * XXX XXX XXX
 */



/*
 * Set additional summon_specific parameters based on monster race and summon specific type
 *
 * Hack_ecology is used to distinguish between when we are populating an ecology, against
 * summoning monsters.
 */
void summon_specific_params(int r_idx, int summon_specific_type, bool hack_ecology)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Handle no summoner or restrictions */
	if ((!r_idx) || (!summon_specific_type))
	{
		summon_char_type = '\0';
		summon_flag_type = 0L;
		summon_attr_type = 0;
		summon_group_type = 0;
		summon_race_type = 0;
		my_strcpy(summon_word_type, "", sizeof(summon_word_type));

		return;
	}

	/* Hack -- Set specific summoning parameters if not currently set */
	switch (summon_specific_type)
	{
		case SUMMON_KIN:
		{
			if (!summon_char_type) summon_char_type = r_ptr->d_char;
			break;
		}

		/* Hack -- we combine two different flag types */
		case SUMMON_RACE:
		{
			if (!summon_flag_type)
			{
				summon_flag_type |= r_ptr->flags3 & (RF3_RACE_MASK);
				summon_flag_type |= r_ptr->flags9 & (RF9_RACE_MASK);
			}
			break;
		}

		/* Hack -- try to summon birds, beasts or reptiles based on summoner */
		case SUMMON_ANIMAL:
		{
			if (!summon_flag_type)
			{
				/* Undead animals */
				if ((hack_ecology) && (r_ptr->flags3 & (RF3_UNDEAD))) summon_flag_type |= (RF8_HAS_SKELETON);
				else summon_flag_type |= r_ptr->flags8 & (RF8_SKIN_MASK);

				if (!summon_flag_type)
				{
					/* Everyone likes lions, tigers, wolves */
					summon_flag_type |= RF8_HAS_FUR;

					/* Surface dwellers like birds */
					if ((r_ptr->flags9 & (RF9_RACE_MASK)) && ! (r_ptr->flags3 & (RF3_RACE_MASK)))
						summon_flag_type |= RF8_HAS_FEATHER;

					/* Dungeon dwellers like reptiles, fish and worse */
					else summon_flag_type |= RF8_HAS_SCALE;
				}
			}
			break;
		}

		/* Hack -- mage priests only hang out with mage priests, but
			warrior priests hang out with warriors and priests. Evil and non-evil
			prefer each other. */
		case SUMMON_CLASS:
		{
			if (!summon_flag_type)
			{
				summon_flag_type |= r_ptr->flags2 & (RF2_CLASS_MASK);
				summon_flag_type |= r_ptr->flags3 & (RF3_EVIL);
			}
			break;
		}

		case SUMMON_COLOUR:
		{
			if (!summon_flag_type)
			{
				summon_flag_type |= r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_ATTR_MULTI);
				summon_flag_type |= r_ptr->flags9 & (RF9_ATTR_METAL | RF9_DROP_MINERAL);
			}
			if (!summon_attr_type)
			{
				summon_attr_type = r_ptr->d_attr;
			}
			break;
		}

		case SUMMON_PREFIX:
		{
			/* Copy prefix */
			char *t = r_name + r_ptr->name;
			char *s = summon_word_type;

			if (!strlen(summon_word_type))
			{
				/* Copy string */
				my_strcpy(s, t, sizeof(summon_word_type));

				/* Find first space */
				while ((*s) && (*s != ' ')) s++;

				/* Terminate string */
				*s = '\0';
			}
			break;
		}

		case SUMMON_SUFFIX:
		case SUMMON_ALL_BUT_PREFIX:
		{
			/* Copy prefix */
			char *t = r_name + r_ptr->name;
			char *s = summon_word_type;

			if (!strlen(summon_word_type))
			{
				/* Find space */
				while (strchr(t, ' '))
				{
					t++;

					if ((summon_specific_type == SUMMON_ALL_BUT_PREFIX) && (*t == ' ')) break;
				}

				/* Copy string */
				my_strcpy(s, t, sizeof(summon_word_type));
			}
			break;
		}

		case SUMMON_INFIX_WYRM_OF:
		{
			/* Copy prefix */
			char *u = NULL;
			char *t = r_name + r_ptr->name;
			char *s = summon_word_type;

			int k = 0;

			if (!strlen(summon_word_type))
			{
				/* Copy string. Note offset. */
				my_strcpy(s, t, sizeof(summon_word_type) - 1);

				/* Break up into words */
				while (*s)
				{
					if (*s == ' ') *s = '\0';
					if ((*s == '-') && (rand_int(100) < 30)) *s = '\0';
					s++;
				}

				/* Ensure double \0 at end of string */
				*(++s) = '\0';

				/* Scan words */
				s = summon_word_type;

				/* Skip unique names */
				while (strchr(s, ',')) { s++; t++; }

				while (*s && (s < summon_word_type + sizeof(summon_word_type)))
				{
					int n = strlen(s);

					/* Skip 'the', 'of', 'great', 'wyrm' */
					if ((my_stricmp(s, "the")) && (my_stricmp(s, "of")) && (my_stricmp(s, "Wyrm")) &&
							(my_stricmp(s,"Great ")))
					{
						/* Maybe get this word */
						if (one_in_(++k)) u = t;
					}

					s += n + 1;
					t += n + 1;
				}

				if (u)
				{
					my_strcpy(summon_word_type, u, sizeof(summon_word_type));

					s = summon_word_type;

					/* Find first space */
					while ((*s) && (*s != ' ')) s++;

					/* Terminate string */
					*s = '\0';
				}
				/* Paranoia */
				else
				{
					my_strcpy(s, "", sizeof(s));
				}
			}
			break;
		}

		case SUMMON_DRAGON_BREATH:
		{
			if (!summon_flag_type)
			{
				/* Hack - match on undead */
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					summon_flag_type = 0x00000001L;
				}
				/* Match on breath */
				else
				{
					summon_flag_type = r_ptr->flags4 & (RF4_BREATH_MASK);
				}
			}
			break;
		}

		case SUMMON_LEVEL:
		{
			if (!summon_flag_type)
			{
				summon_flag_type = level_flag & (LF1_TERRAIN);

				if (summon_specific_level_match(r_idx, &summon_flag_type)) break;

				summon_flag_type = 0L;
			}

			/* Fall through */
		}

		case SUMMON_ALIGN:
		{
			if (!summon_flag_type)
			{
				summon_flag_type |= r_ptr->flags3 & (RF3_EVIL | RF3_NONLIVING);
			}
			break;
		}

		case SUMMON_GROUP:
		case ANIMATE_ELEMENT:
		{
			if (!summon_group_type)
			{
				summon_group_type = r_ptr->grp_idx;
			}
			break;
		}

		case SUMMON_FRIEND:
		{
			if (!summon_race_type)
			{
				summon_race_type = r_idx;
			}
			break;
		}

		case SUMMON_UNIQUE_FRIEND:
		{
			if (!summon_attr_type && !summon_char_type)
			{
				summon_char_type = r_ptr->d_char;
				summon_attr_type = r_ptr->d_attr;
			}
			break;
		}
	}
}


/*
 * Attempt to allocate a random monster in the dungeon.
 *
 * Place the monster at least "dis" distance from the player.
 *
 * Use "slp" to choose the initial "sleep" status
 *
 * Use "monster_level" for the monster level
 */
bool alloc_monster(int dis, bool slp)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, c;

	c = 0;

	/* Find a legal, distant, unoccupied, space */
	while (1)
	{
		if (c++ > 2000) return (FALSE);

		/* Pick a location */
		y = rand_int(DUNGEON_HGT);
		x = rand_int(DUNGEON_WID);

		/* Require "empty" floor grid */
		if (!cave_empty_bold(y, x)) continue;

		/* Accept far away grids, or any grid if not yet placed character */
		if ((!py) || (distance(y, x, py, px) > dis)) break;
	}

	/* Cave ecology enforced */
	if (cave_ecology.ready)
	{
		cave_ecology.use_ecology = room_info[dun_room[y/BLOCK_HGT][x/BLOCK_WID]].ecology;
	}

	/* Attempt to place the monster, allow groups */
	if (place_monster(y, x, slp, TRUE)) return (TRUE);

	/* Cave ecology enforced */
	if (cave_ecology.ready)
	{
		cave_ecology.use_ecology = 0L;
	}

	/* Nope */
	return (FALSE);
}






/*
 * Place a monster (of the specified "type") near the given
 * location.  Return TRUE iff a monster was actually summoned.
 *
 * We will attempt to place the monster up to 10 times before giving up.
 *
 * Note: SUMMON_UNIQUE and SUMMON_WRAITH (XXX) will summon Uniques
 * Note: SUMMON_HI_UNDEAD and SUMMON_HI_DRAGON may summon Uniques
 * Note: None of the other summon codes will ever summon Uniques.
 *
 * This function has been changed.  We now take the "monster level"
 * of the summoning monster as a parameter, and use that, along with
 * the current dungeon level, to help determine the level of the
 * desired monster.  Note that this is an upper bound, and also
 * tends to "prefer" monsters of that level.  Currently, we use
 * the average of the dungeon and monster levels, and then add
 * five to allow slight increases in monster power.
 *
 * Note that we use the new "monster allocation table" creation code
 * to restrict the "get_mon_num()" function to the set of "legal"
 * monsters, making this function much faster and more reliable.
 *
 * Note that this function may not succeed, though this is very rare.
 */
bool summon_specific(int y1, int x1, int restrict_race, int lev, int type, bool grp, u32b flg)
{
	int i, x, y, r_idx;

	/* Look for a location */
	for (i = 0; i < 20; ++i)
	{
		/* Pick a distance */
		int d = (i / 15) + 1;

		/* Pick a location */
		scatter(&y, &x, y1, x1, d, CAVE_XLOF);

		if (!in_bounds_fully(y,x)) continue;

		/* Require "empty" floor grid */
		if (!cave_empty_bold(y, x)) continue;

		/* Hack -- no summoning on glyph of warding */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_GLYPH)) continue;

		/* Okay */
		break;
	}

	/* Failure */
	if (i == 20) return (FALSE);

	/* Save the "summon" type */
	summon_specific_type = type;

	/* Prevent 'chain summoning' if the summoner is a monster */
	if (restrict_race)
		summon_strict = TRUE;

	/* Restrict race as appropriate */
	summoner = restrict_race;

	/* Cave ecology enforced; however allow non-core monsters from other ecologies */
	if (cave_ecology.ready)
	{
		cave_ecology.use_ecology = room_info[dun_room[y/BLOCK_HGT][x/BLOCK_WID]].ecology;
		cave_ecology.use_ecology |= 0x0000FFFFL;
	}

	/* Require "okay" monsters */
	get_mon_num_hook = summon_specific_okay;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Pick a monster, using the level calculation */
	r_idx = get_mon_num(lev);

	/* Cave ecology enforced */
	if (cave_ecology.ready)
	{
		cave_ecology.use_ecology = 0L;
	}

	/* Remove restriction */
	get_mon_num_hook = NULL;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Reset summoner */
	summoner = 0;

	/* No longer strict */
	summon_strict = FALSE;

	/* Handle failure */
	if (!r_idx) return (FALSE);

	/* Attempt to place the monster (awake, groups dependent on caller) */
	if (!place_monster_aux(y, x, r_idx, FALSE, grp, flg)) return (FALSE);

	/* Set additional paramaters if we haven't yet set a restriction */
	if (summon_specific_type) summon_specific_params(r_idx, summon_specific_type, FALSE);

	/* Success */
	return (TRUE);
}


/*
 * Unlike above routine, we only place one of the monster, regardless
 * of type and we do allow uniques, but not if already created.
 */
bool summon_specific_one(int y1, int x1, int r_idx, bool slp, u32b flg)
{
	int i, x, y;

	/* Look for a location */
	for (i = 0; i < 20; ++i)
	{
		/* Pick a distance */
		int d = (i / 15) + 1;

		/* Pick a location */
		scatter(&y, &x, y1, x1, d, CAVE_XLOF);

		/* Require "empty" floor grid */
		if (!cave_empty_bold(y, x)) continue;

		/* Hack -- no summoning on glyph of warding */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_GLYPH)) continue;

		/* Okay */
		break;
	}

	/* Failure */
	if (i == 20) return (FALSE);

	/* Attempt to place the monster (awake or asleep, do not allow groups) */
	if (!place_monster_aux(y, x, r_idx, slp, FALSE, flg)) return (FALSE);

	/* Hack -- monster does not drop anything */
	m_list[cave_m_idx[y][x]].mflag |= (MFLAG_MADE);

	/* Success */
	return (TRUE);
}



/*
 * Let the given object become alive.
 *
 * Note that "animation" REQUIRES empty space.
 *
 */
bool animate_object(int item)
{
	cptr p;

	char o_name[80];

	object_type *o_ptr;

	bool result = FALSE;

	int y1, x1;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
		y1 = p_ptr->py;
		x1 = p_ptr->px;
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
		y1 = o_ptr->iy;
		x1 = o_ptr->ix;
	}

	/* Paranoia */
	if (o_ptr->name3 <= 0) return (FALSE);

	/* Summon the specific race */
	result = summon_specific_one(y1, x1, o_ptr->name3, FALSE, (o_ptr->ident & (IDENT_FORGED)) != 0 ? (MFLAG_ALLY | MFLAG_MADE) : 0L);

	/* Hack -- no result */
	if (!result)
	{
		/* Keep trying */
		o_ptr->timeout++;

		/* Return */
		return (FALSE);
	}

	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

	/* Correct message */
	switch (o_ptr->tval)
	{
		case TV_EGG:
			p = "hatched.";
			break;

		case TV_BODY:
			p = "come back from the dead!";
			break;

		case TV_HOLD:
			p = "broken open!";
			break;

		default:
			p = "come to life!";
			break;
	}

	/* Notify player if carrying */
	if (item >= 0)
	{
		/* Message */
		msg_format("Your %s %s %s", o_name,
		   ((o_ptr->stackc == 1) ? "has" :
		   ((!(o_ptr->stackc) && (o_ptr->number == 1)) ?
		   "has" : "have")), p);

		/* Destroy the item */
		if (o_ptr->stackc)
		{
			if (o_ptr->number == o_ptr->stackc) inven_drop_flags(o_ptr);
			inven_item_increase(item, -(o_ptr->stackc));
		}
		else
		{
			inven_drop_flags(o_ptr);
			inven_item_increase(item,-(o_ptr->number));
		}

		inven_item_optimize(item);
	}
	else
	{
		/* Message if object known */
		if (o_ptr->ident & (IDENT_MARKED)) msg_format("The %s %s %s", o_name,
		   ((o_ptr->stackc == 1) ? "has" :
		   ((!(o_ptr->stackc) && (o_ptr->number == 1)) ?
		   "has" : "have")), p);

		/* Destroy the item */
		if (o_ptr->stackc) floor_item_increase(0 - item, -(o_ptr->stackc));
		else floor_item_increase(0 - item, -(o_ptr->number));

		floor_item_optimize(0 - item);
	}

	return (TRUE);
}



/* Check monsters for fear */
static bool questor_test_fear(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	if (m_ptr->monfear) return (TRUE);
	else return (FALSE);
}


/*
 * Change monster fear.
 *
 * Monsters can be frightened or panicking.  In both cases, they try to
 * retreat, but when actually panicking, they cannot cast spells that don't
 * either heal or move them.
 */
void set_monster_fear(monster_type *m_ptr, int v, bool panic)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/*hack - monsters who cannot be scared are unaffected*/
	if (r_ptr->flags3 & (RF3_NO_FEAR)) v = 0;

	/* Set monfear */
	m_ptr->monfear = v > 255 ? 255 : (byte)v;

	/* Monster is panicking */
	if ((m_ptr->monfear) && (panic)) m_ptr->min_range = PANIC_RANGE;

	/* Otherwise, reset monster combat ranges (later) */
	else m_ptr->min_range = 0;

	/* Quest monster */
	if (r_ptr->flags1 & (RF1_QUESTOR))
	{
		check_monster_quest(cave_m_idx[m_ptr->fy][m_ptr->fx], questor_test_fear, EVENT_FEAR_RACE);
	}
}


/*
 * Let the given monster attempt to reproduce.
 *
 * Note that "reproduction" REQUIRES empty space.
 */
bool multiply_monster(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	int i, y, x;

	bool result = FALSE;

	/* Try up to 18 times */
	for (i = 0; i < 18; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(&y, &x, m_ptr->fy, m_ptr->fx, d, CAVE_XLOF);

		/* Require an "empty" floor grid */
		if (!cave_empty_bold(y, x)) continue;

		/* Require no risk of damage */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP)) continue;

		/* Require monster can pass and survive on terrain */
		if (place_monster_here(y, x, m_ptr->r_idx) <= MM_FAIL) continue;

		/* Create a new monster (awake, no groups) */
		result = place_monster_aux(y, x, m_ptr->r_idx, FALSE, FALSE, m_ptr->mflag & (MFLAG_ALLY | MFLAG_IGNORE));

		/* Result? */
		if (result)
		{
			/* Hack -- monster does not drop anything */
			m_list[cave_m_idx[y][x]].mflag |= (MFLAG_MADE);
	
			/* Hack -- new monster inherits poison status from their parent */
			m_list[cave_m_idx[y][x]].poisoned = m_ptr->poisoned;
		}
		
		/* Done */
		break;
	}

	/* Result */
	return (result);
}





/*
 * Dump a message describing a monster's reaction to damage
 */
void message_pain(int m_idx, int dam)
{
	long oldhp, newhp, tmp;
	int percentage;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	char m_name[80];


	/* Get the monster name */
	monster_desc(m_name, sizeof(m_name), m_idx, 0);

	/* Notice non-damage */
	if (dam == 0)
	{
#if 0
		msg_format("%^s is unharmed.", m_name);
#endif
		return;
	}

	/* Notice dead */
	if (m_ptr->hp < 0)
	{
#if 0
		msg_format("%^s is %s.", m_name, r_ptr->flags3 & (RF3_NONLIVING) ? "destoyed" : "dead");
#endif
		return;
	}
	
	
	/* Note -- subtle fix -CFT */
	newhp = (long)(m_ptr->hp);
	oldhp = newhp + (long)(dam);
	tmp = (newhp * 100L) / (oldhp);
	percentage = (int)(tmp);


	/* Hack -- avoid mentioning minor damage */
	if (!(m_ptr->ml) && (percentage > 95)) return;

	/* Hack -- avoid mentioning if out of sight */
	if (m_ptr->cdis > MAX_SIGHT) return;

	/* Hack -- if seen, only report changes in 'damage state'*/
	if ((m_ptr->ml) && (m_idx != p_ptr->health_who))
	{
		int percentage2;

		tmp = (oldhp * 100L) / (long)(m_ptr->maxhp + 1);
		percentage = (int)(tmp);

		tmp = (newhp * 100L) / (long)(m_ptr->maxhp + 1);
		percentage2 = (int)(tmp);

		/* Notify the player only if monster 'damage state' changes */
		if (((percentage >= 60) && (percentage2 < 60))
			|| ((percentage >= 25) && (percentage2 < 25))
			|| ((percentage >= 10) && (percentage2 < 10)))
		{
			msg_format("%^s is %s.", m_name, look_mon_desc(m_idx));
		}
		return;
	}

	/* Nonvocal monsters */
	if (r_ptr->flags3 & (RF3_NONVOCAL))
	{
		if (percentage > 95)
			msg_format("%^s barely notices.", m_name);
		else if (percentage > 75)
			msg_format("%^s flinches.", m_name);
		else if (percentage > 50)
			msg_format("%^s squelches.", m_name);
		else if (percentage > 35)
			msg_format("%^s quivers in pain.", m_name);
		else if (percentage > 20)
			msg_format("%^s writhes about.", m_name);
		else if (percentage > 10)
			msg_format("%^s writhes in agony.", m_name);
		else
			msg_format("%^s jerks limply.", m_name);
	}

	/* Dogs and Hounds */
	else if (strchr("CZ", r_ptr->d_char))
	{
		if (percentage > 95)
			msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 75)
			msg_format("%^s snarls with pain.", m_name);
		else if (percentage > 50)
			msg_format("%^s yelps in pain.", m_name);
		else if (percentage > 35)
			msg_format("%^s howls in pain.", m_name);
		else if (percentage > 20)
			msg_format("%^s howls in agony.", m_name);
		else if (percentage > 10)
			msg_format("%^s writhes in agony.", m_name);
		else
			msg_format("%^s yelps feebly.", m_name);
	}

	/* One type of monsters (ignore,squeal,shriek) */
	else if (strchr("FIKRSXabclrs", r_ptr->d_char))
	{
		if (percentage > 95)
			msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75)
			msg_format("%^s grunts with pain.", m_name);
		else if (percentage > 50)
			msg_format("%^s squeals in pain.", m_name);
		else if (percentage > 35)
			msg_format("%^s shrieks in pain.", m_name);
		else if (percentage > 20)
			msg_format("%^s shrieks in agony.", m_name);
		else if (percentage > 10)
			msg_format("%^s writhes in agony.", m_name);
		else
			msg_format("%^s cries out feebly.", m_name);
	}

	/* Another type of monsters (shrug,cry,scream) */
	else
	{
		if (percentage > 95)
			msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 75)
			msg_format("%^s grunts with pain.", m_name);
		else if (percentage > 50)
			msg_format("%^s cries out in pain.", m_name);
		else if (percentage > 35)
			msg_format("%^s screams in pain.", m_name);
		else if (percentage > 20)
			msg_format("%^s screams in agony.", m_name);
		else if (percentage > 10)
			msg_format("%^s writhes in agony.", m_name);
		else
			msg_format("%^s cries out feebly.", m_name);
	}
}

/*
 * Returns which of two monsters are deeper
 */
int deeper_monster(int r_idx, int r)
{
	/* Paranoia */
	if (!r) return (r_idx);
	if (!r_idx) return (r);

	/* Is the current monster deeper */
	if (r_info[r].level < r_info[r_idx].level)
	{
		/* Monster race */
		return(r_idx);
	}

	/* Is the current monster deeper */
	else if ((r_info[r].level == r_info[r_idx].level) && (r_info[r].mexp < r_info[r_idx].mexp))
	{
		/* Monster race */
		return(r_idx);
	}

	return(r);
}


/*
 * Update monster ecology deepest monster for the room
 */
static void deepest_in_ecology(int r_idx)
{
	int r = cave_ecology.num_ecologies - 1;

	/* Check overall depth */
	cave_ecology.deepest_race[0] = deeper_monster(r_idx, cave_ecology.deepest_race[0]);

	/* Check room depth */
	cave_ecology.deepest_race[r] = deeper_monster(r_idx, cave_ecology.deepest_race[r]);
}


/*
 * Auxiliary helper function for get_monster_ecology.
 *
 * This adds a number of races to the ecology, which match the hook,
 * without adding their related races. The first half of this function
 * is concerned with ensuring that the ecology doesn't overflow when it
 * starts to fill up - by checking whether existing monsters in the
 * ecology match the hook we're looking for. If so, we can use an
 * existing monster as an ally instead.
 */
static bool get_monster_ecology_aux(bool (*tmp_mon_num_hook)(int r_idx), int number)
{
	int i, k = 0;
	int count = 0;
	int races = cave_ecology.num_races + number;

	/* No more races */
	if (cave_ecology.num_races >= MAX_ECOLOGY_RACES - 1) return (FALSE);

	/* Reduce races required by allowing existing races in ecology */
	if (cave_ecology.num_races > 2 * MIN_ECOLOGY_RACES)
	{
		bool out_of_space = cave_ecology.num_races < 3 * MIN_ECOLOGY_RACES;
		bool pick_one = FALSE;

		/* Check to see if an existing race matches the criteria */
		for (i = 0; i < cave_ecology.num_races; i++)
		{
			/* The summoner shouldn't just summon themselves */
			if (cave_ecology.race[i] == summoner) continue;

			/* We shouldn't summon the 'seed' from another ecology unless we're
			 * really running out of space */
			if ((cave_ecology.num_races < MAX_ECOLOGY_RACES / 2) &&
					((cave_ecology.race_ecologies[i] & (1L)) == 0)) continue;

			/* We've found a match. Pick_one is used to pick a random monster
			 * to overlap between ecologies if we have plenty of space left in
			 * the ecology. */
			if (tmp_mon_num_hook(cave_ecology.race[i]))
			{
				/* We need to add less races */
				if (!pick_one) races--;

				/* Add monster to this room - always if we're running out of space */
				if (out_of_space)
				{
					cave_ecology.race_ecologies[i] |= (1L << (cave_ecology.num_ecologies - 1));
				}
				/* Pick a random monster to overlap */
				else
				{
					k = rand_int(++count);
					pick_one = TRUE;
				}
			}
		}

		/* We've picked one monster */
		if (pick_one)
		{
			cave_ecology.race_ecologies[k] |= (1L << (cave_ecology.num_ecologies - 1));
		}
	}

	/* Limit races */
	if (races >= MAX_ECOLOGY_RACES) races = MAX_ECOLOGY_RACES - 1;

	/* Ensure races */
	if (races <= cave_ecology.num_races) return (FALSE);

	/* Use supplied hook */
	get_mon_num_hook = tmp_mon_num_hook;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Limit loop */
	count = 0;

	/* Pick */
	while ((count < 1000) && (cave_ecology.num_races <= races))
	{
		/* Make a resident */
		int r_idx = get_mon_num(monster_level);

		/* Have a good race */
		if (r_idx)
		{
			/* Add monster to ecology */
			cave_ecology.get_mon[cave_ecology.num_races] = TRUE;
			cave_ecology.race[cave_ecology.num_races] = r_idx;

			/* Add monster to ecology */
			cave_ecology.race_ecologies[cave_ecology.num_races] = (1L << (cave_ecology.num_ecologies - 1));

			/* Shallow monsters wander outside ecology */
			if (r_info[r_idx].level < MAX(p_ptr->depth - 5, p_ptr->depth / 2 + 3)) cave_ecology.race_ecologies[cave_ecology.num_races] |= (1L);

			/* Is the current monster deeper */
			deepest_in_ecology(r_idx);

			/* Increase total race count */
			cave_ecology.num_races++;

			if (cheat_hear) msg_format("Ecology dependent monster (%s) based on %s (%d).",r_name + r_info[r_idx].name,
				r_name + r_info[summoner].name, tmp_mon_num_hook == summon_specific_okay ? summon_specific_type : -1);
		}

		/* Increase counter */
		count++;
	}

	return(TRUE);
}



/*
 * Get a monster, and add it and its allies to the ecology for the level.
 *
 * Allies are defined as any monster that can be summoned or escort the
 * monster passed to this routine. However, for small ecologies, we add
 * allies using by picking a hack_ecology number from 1 to 10:
 *
 * 10 -- Match on dragon breath type (dragons only)
 * 9 -- Match the prefix, unless a wyrm 'of' where match the suffix
 * 8 -- Match living/non-living
 * 7 -- Match the same colour for metallic monsters
 * 6 -- Match on evil-ness and intelligence
 * 5 -- Match on group (faerie, fire elemental etc) if group defined
 * 4 -- Match on race (orc, giant, dragon, insect etc) if has a race flag
 * 3 -- Match on class (warrior, mage etc) if has a class flag
 * 2 -- Match on animal (animal, either feathered, furry or foul)
 * 1 -- Match on kin
 *
 * Note that this routine is now not responsible for creating new
 * ecologies. To do so, increase cave_ecology.num_ecologies by one before
 * calling this routine.
 *
 * Hack_pit is used to force a minimum number of monsters of a certain
 * 'type' in order for monster pits (and possibly other room types) to
 * have enough variety.
 *
 * All monsters are added to the ecology cave_ecology.num_ecologies.
 */
void get_monster_ecology(int r_idx, int hack_pit)
{
	int i = cave_ecology.num_races;
	int j;

	/* Hack -- hack the ecology */
	int hack_ecology = 0;

	/* Hack -- used to reduce duplicate seed monster entries */
	bool already_exists = TRUE;

	/* Store old hook */
	bool (*get_mon_num_hook_temp)(int r_idx) = get_mon_num_hook;

	/* Regenerate hook? */
	bool used_new_hook = FALSE;

	int old_monster_level = monster_level;

	monster_race *r_ptr;

	/* Ensure we're pointing at a valid ecology number */
	if (cave_ecology.num_ecologies >= MAX_ECOLOGIES) return;

	/* Pick a monster if one not specified */
	if (!r_idx)
	{
		r_idx = get_mon_num(monster_level);

		/* Still not got one */
		if (!r_idx) return;
	}

	/* Get the race */
	r_ptr = &r_info[r_idx];

	/* Is the current monster deeper */
	deepest_in_ecology(r_idx);

	/* Find monster in ecology if it exists already */
	for (j = 0; j < cave_ecology.num_races; j++)
	{
		if (cave_ecology.race[j] == r_idx) break;
	}

	/* Force space for pits */
	if (hack_pit) j = cave_ecology.num_races;

	/* Make space for monster if it does not exist already. */
	if (j == cave_ecology.num_races)
	{
		/* Paranoia */
		if (cave_ecology.num_races >= MAX_ECOLOGY_RACES - 1) return;

		/* Add monster to ecology */
		cave_ecology.get_mon[cave_ecology.num_races] = TRUE;
		cave_ecology.race[cave_ecology.num_races] = r_idx;

		/* Increase total race count */
		cave_ecology.num_races++;

		/* Monster had to be added */
		already_exists = FALSE;

		if (cheat_hear) msg_format("Adding base race to ecology (%s)", r_name + r_ptr->name);
	}

	/* Add monster to room (base races don't wander around dungeon) */
	cave_ecology.race_ecologies[j] |= (1L << (cave_ecology.num_ecologies - 1));

	/* For the seed monster, we force some related monsters to appear */
	hack_ecology = randint(10);

	/* Try to ensure we have a hack. */
	switch(hack_ecology)
	{
		case 10:
			/* Hack -- dragons, except wyrms 'of' match on breath */
			if (!strchr("adD", r_ptr->d_char) ||
					(strstr(r_name +  + r_ptr->name, " of "))) hack_ecology--;
			/* Name matching is too limited for towns */
			else if (cave_ecology.town) hack_ecology--;
			else break;
		case 9:
			if (!strchr(r_name + r_ptr->name, ' ')) hack_ecology--;
			/* Paranoia to minimise empty pits */
			else if (r_ptr->flags1 & (RF1_UNIQUE)) hack_ecology--;
			/* Name matching is too limited for towns */
			else if (cave_ecology.town) hack_ecology--;
			else break;
		case 8:
		{
			u32b dummy_flag = 0L;

			/* We only populate a pit if the monster matches a level flag match */
			if ((hack_pit) && (!summon_specific_level_match(r_idx, &dummy_flag))) hack_ecology--;
			else break;
		}
		case 7:
			if ((r_ptr->flags9 & (RF9_DROP_MINERAL)) == 0) hack_ecology--;
			/* Coin matching is too limited for towns */
			else if (cave_ecology.town) hack_ecology--;
			else break;
		case 6:
			/* Summon align is too generic for pits */
			if (hack_pit) hack_ecology--;
			else break;
		case 5:
			if (!(r_ptr->grp_idx)) hack_ecology--;
			else break;
		case 4:
			if (((r_ptr->flags3 & (RF3_RACE_MASK)) == 0) &&
					((r_ptr->flags9 & (RF9_RACE_MASK)) == 0)) hack_ecology--;
			else break;
		case 3:
			if ((r_ptr->flags2 & (RF2_CLASS_MASK)) != 0) break;
		case 2:
		case 1:
			/* Battle-fields, towns and two thirds of failures match on d_char - the rest use animals */
			if ((level_flag & (LF1_BATTLE)) || (cave_ecology.town) || (rand_int(100) < 66)) hack_ecology = 1;
			else hack_ecology = 2;
			break;
	}

	/* Display hack index */
	if (hack_ecology && cheat_hear)
		msg_format("Hacking ecology (%d)", hack_ecology);

	/* Have a good race */
	for ( ; (i < cave_ecology.num_races) || (already_exists); i++)
	{
		int k;

		/* Get the monster */
		r_idx = cave_ecology.race[already_exists ? j : i];
		r_ptr = &r_info[r_idx];

		/* How many monsters to generate per summons */
		k = cave_ecology.num_races > 2 * MIN_ECOLOGY_RACES ? 1 :
					cave_ecology.num_races > MIN_ECOLOGY_RACES ? 2 : 3;

		/* Hack -- summoners get less of each race */
		if ((k > 1) && (r_ptr->flags7 & (RF7_SUMMON_MASK))) k--;

		/* We've used the already existing monster */
		already_exists = FALSE;

		/* Try slightly lower monster level */
		monster_level = MAX(1, MIN(monster_level - 3,
								   (monster_level + r_ptr->level) / 2 - 1));

		/* Set summoner */
		summoner = r_idx;

		/* Hack - always summon lower level monsters for ecology */
		summon_strict = TRUE;

		/* Add escort races */
		if ((!hack_pit) && (r_ptr->flags1 & (RF1_ESCORT | RF1_ESCORTS)))
		{
			place_monster_idx = r_idx;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(place_monster_okay, 1 + randint(k) + (r_ptr->flags1 & (RF1_ESCORT) ? rand_int(3) : 0));
		}

		/* Add summon races -- summon kin */
		if (((!hack_pit) && (r_ptr->flags7 & (RF7_S_KIN))) || (hack_ecology == 1))
		{
			summon_specific_type = SUMMON_KIN;

			/* Clear existing restrictions */
			summon_char_type = 0;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_KIN, (hack_ecology == 1));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay,
					(hack_ecology == 1) && hack_pit ? hack_pit : (1 + randint(k) + (r_ptr->flags1 & (RF1_UNIQUE) ? rand_int(3) : 0)));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Kin pit.");
		}

		/* Add summon races -- summon plant */
		if ((!hack_pit) && (r_ptr->flags7 & (RF7_S_PLANT)))
		{
			summon_specific_type = SUMMON_PLANT;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon insect */
		if ((!hack_pit) && (r_ptr->flags7 & (RF7_S_INSECT)))
		{
			summon_specific_type = SUMMON_INSECT;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon animal */
		if (((!hack_pit) && (r_ptr->flags7 & (RF7_S_ANIMAL))) || (hack_ecology == 2))
		{
			summon_specific_type = SUMMON_ANIMAL;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_ANIMAL, (hack_ecology == 2));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, (hack_ecology == 2) && hack_pit ? hack_pit : randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Zoo.");
		}

		/* Add summon races -- summon hound */
		if ((!hack_pit) && (r_ptr->flags7 & (RF7_S_HOUND)))
		{
			summon_specific_type = SUMMON_HOUND;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon spider */
		if ((!hack_pit) && (r_ptr->flags7 & (RF7_S_SPIDER)))
		{
			summon_specific_type = SUMMON_SPIDER;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon class */
		if (((!hack_pit) && (r_ptr->flags7 & (RF7_S_CLASS))) || (hack_ecology == 3))
		{
			summon_specific_type = SUMMON_CLASS;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_CLASS, (hack_ecology == 3));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, (hack_ecology == 3) && hack_pit ? hack_pit :  1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Class pit.");
		}

		/* Add summon races -- summon race */
		if (((!hack_pit) && (r_ptr->flags7 & (RF7_S_RACE))) || (hack_ecology == 4))
		{
			summon_specific_type = SUMMON_RACE;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_RACE, (hack_ecology == 4));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, (hack_ecology == 4) && hack_pit ? hack_pit :  1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Race pit.");
		}

		/* Add summon races -- summon group */
		if (((!hack_pit) && (r_ptr->flags7 & (RF7_S_GROUP))) || (hack_ecology == 5))
		{
			summon_specific_type = SUMMON_GROUP;

			/* Clear existing restrictions */
			summon_group_type = 0;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_GROUP, (hack_ecology == 5));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, (hack_ecology == 5) && hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Group pit.");
		}

		/* Add summon races -- summon alignment (match evilness, livingness). Note this is only done via ecology hacks. */
		if (hack_ecology == 6)
		{
			summon_specific_type = SUMMON_ALIGN;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_ALIGN, (hack_ecology == 6));

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Aligned pit.");
		}

		/* Add summon races -- summon colour. Note this is only done via ecology hacks. */
		if (hack_ecology == 7)
		{
			summon_specific_type = SUMMON_COLOUR;

			/* Clear existing restrictions */
			summon_attr_type = 0;
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_COLOUR, TRUE);

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Colour pit.");
		}

		/* Add summon races -- summon a good match for the level. Note this is only done via ecology hacks. */
		if (hack_ecology == 8)
		{
			summon_specific_type = SUMMON_LEVEL;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_LEVEL, TRUE);

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Level pit.");
		}

		/* Add summon races -- summon infix. Note this is only done via ecology hacks. */
		if (hack_ecology == 9)
		{
			summon_specific_type = SUMMON_INFIX_WYRM_OF;

			/* Clear existing restrictions */
			summon_word_type[0] = '\0';

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_INFIX_WYRM_OF, TRUE);

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_format("%s pit.", summon_word_type);
		}

		/* Add summon races -- summon dragon breath. Note this is only done via ecology hacks. */
		if (hack_ecology == 10)
		{
			summon_specific_type = SUMMON_DRAGON_BREATH;

			/* Clear existing restrictions */
			summon_flag_type = 0L;

			/* Set the restrictions */
			summon_specific_params(r_idx, SUMMON_DRAGON_BREATH, TRUE);

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, hack_pit ? hack_pit : 1 + randint(k));

			/* What type of pit */
			if (cheat_room && hack_pit) msg_print("Dragon pit.");
		}


		/* Break if generating a pit */
		if (hack_pit) break;


		/* Add summon races -- summon orc */
		if (r_ptr->flags7 & (RF7_S_ORC))
		{
			summon_specific_type = SUMMON_ORC;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon troll */
		if (r_ptr->flags7 & (RF7_S_TROLL))
		{
			summon_specific_type = SUMMON_TROLL;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon giant */
		if (r_ptr->flags7 & (RF7_S_GIANT))
		{
			summon_specific_type = SUMMON_GIANT;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon dragon */
		if (r_ptr->flags7 & (RF7_S_DRAGON))
		{
			summon_specific_type = SUMMON_DRAGON;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon high dragon */
		if (r_ptr->flags7 & (RF7_S_HI_DRAGON))
		{
			summon_specific_type = SUMMON_HI_DRAGON;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon demon */
		if (r_ptr->flags7 & (RF7_S_DEMON))
		{
			summon_specific_type = SUMMON_DEMON;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon high demon */
		if (r_ptr->flags7 & (RF7_S_HI_DEMON))
		{
			summon_specific_type = SUMMON_HI_DEMON;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon undead */
		if (r_ptr->flags7 & (RF7_S_UNDEAD))
		{
			summon_specific_type = SUMMON_UNDEAD;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Add summon races -- summon high undead */
		if (r_ptr->flags7 & (RF7_S_HI_UNDEAD))
		{
			summon_specific_type = SUMMON_HI_UNDEAD;

			/* Get additional monsters */
			used_new_hook |= get_monster_ecology_aux(summon_specific_okay, randint(k));
		}

		/* Unhack ecology */
		hack_ecology = 0;
	}

	/* Regenerate monster hook */
	if (used_new_hook)
	{
		get_mon_num_hook = get_mon_num_hook_temp;

		get_mon_num_prep();
	}

	/* Get old monster level */
	monster_level = old_monster_level;
}


/*
 * Places the guardian on the level.
 */
bool place_guardian(int y0, int x0, int y1, int x1)
{
	int y = 0;
	int x = 0;
	int guard;
	int count = 0;

	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Get the guardian */
	guard = actual_guardian(zone->guard, p_ptr->dungeon, zone - t_info[p_ptr->dungeon].zone);

	/* Paranoia */
	if ((guard) && (r_info[guard].cur_num < r_info[guard].max_num))
	{
		/* Generating */
		if (cheat_room) message_add(format("Placing guardian (%s).", r_name + r_info[guard].name), MSG_GENERIC);

		/* Pick a location */
		while (count++ < 1000)
		{
			y = y0 + rand_int(y1);
			x = x0 + rand_int(x1);

			/* Place the questor */
			if (place_monster_aux(y, x, guard, FALSE, TRUE, 0L)) break;

			if (count < MAX_RANGE)
			{
				/* Paranoia - ensure some horizontal space to choose from */
				if (x1 - x0 < count)
				{
					x0 = (x1 + x0) / 2 - count;
					x1 = (x1 + x0) / 2 + count;
				}

				/* Paranoia - ensure some vertical space to choose from */
				if (y1 - y0 < count)
				{
					y0 = (y1 + y0) / 2 - count;
					y1 = (y1 + y0)/ 2 + count;
				}
			}
		}

		if (count >= 1000)
		{
			if (cheat_room) message_add(format("Could not place guardian (%s).", r_name + r_info[guard].name), MSG_GENERIC);

			return (FALSE);
		}
	}

	return (TRUE);
}

