/* File: object2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for protv_fit purposes provided that this copyright and statement
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
 * Excise a dungeon object from any stacks
 */
void excise_object_idx(int o_idx)
{
	object_type *j_ptr;

	s16b this_o_idx, next_o_idx = 0;

	s16b prev_o_idx = 0;


	/* Object */
	j_ptr = &o_list[o_idx];

	/* Monster */
	if (j_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Monster */
		m_ptr = &m_list[j_ptr->held_m_idx];

		/* Scan all objects in the grid */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					m_ptr->hold_o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *i_ptr;

					/* Previous object */
					i_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					i_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}

	/* Dungeon */
	else
	{
		int y = j_ptr->iy;
		int x = j_ptr->ix;

		/* Scan all objects in the grid */
		for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					cave_o_idx[y][x] = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *i_ptr;

					/* Previous object */
					i_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					i_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}
}


/*
 * Delete a dungeon object
 *
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(int o_idx)
{
	object_type *j_ptr;

	/* Excise */
	excise_object_idx(o_idx);

	/* Object */
	j_ptr = &o_list[o_idx];

	/* Dungeon floor */
	if (!(j_ptr->held_m_idx))
	{
		int y, x;

		/* Location */
		y = j_ptr->iy;
		x = j_ptr->ix;

		/* Check if was projecting lite */
		if (check_object_lite(j_ptr))
		{
			/* Recheck surrounding lite */
			check_attribute_lost(y, x, 2, CAVE_XLOS, require_halo, has_halo, redraw_halo_loss, remove_halo, reapply_halo);
		}

		/* Check the item was seen */
		if (j_ptr->ident & (IDENT_MARKED))
		{
			/* Window flags */
			p_ptr->window |= (PW_ITEMLIST);
		}

		/* Visual update */
		lite_spot(y, x);
	}

	/* Wipe the object */
	object_wipe(j_ptr);

	/* Count objects */
	o_cnt--;
}


/*
 * Deletes all objects at given location
 */
void delete_object(int y, int x)
{
	s16b this_o_idx, next_o_idx = 0;


	/* Paranoia */
	if (!in_bounds(y, x)) return;


	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Wipe the object */
		object_wipe(o_ptr);

		/* Count objects */
		o_cnt--;
	}

	/* Objects are gone */
	cave_o_idx[y][x] = 0;

	/* Visual update */
	lite_spot(y, x);
}



/*
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_objects_aux(int i1, int i2)
{
	int i;

	object_type *o_ptr;


	/* Do nothing */
	if (i1 == i2) return;


	/* Repair objects */
	for (i = 1; i < o_max; i++)
	{
		/* Get the object */
		o_ptr = &o_list[i];

		/* Skip "dead" objects */
		if (!o_ptr->k_idx) continue;

		/* Repair "next" pointers */
		if (o_ptr->next_o_idx == i1)
		{
			/* Repair */
			o_ptr->next_o_idx = i2;
		}
	}


	/* Get the object */
	o_ptr = &o_list[i1];


	/* Monster */
	if (o_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Get the monster */
		m_ptr = &m_list[o_ptr->held_m_idx];

		/* Repair monster */
		if (m_ptr->hold_o_idx == i1)
		{
			/* Repair */
			m_ptr->hold_o_idx = i2;
		}
	}

	/* Dungeon */
	else
	{
		int y, x;

		/* Get location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Repair grid */
		if (cave_o_idx[y][x] == i1)
		{
			/* Repair */
			cave_o_idx[y][x] = i2;
		}
	}


	/* Hack -- move object */
	COPY(&o_list[i2], &o_list[i1], object_type);

	/* Hack -- wipe hole */
	object_wipe(o_ptr);
}


/*
 * Compact and Reorder the object list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" objects, we base the saving throw on a
 * combination of object level, distance from player, and current
 * "desperation".
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_objects(int size)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, y, x, num, cnt;

	int cur_lev, cur_dis, chance;

	/* Compact */
	if (size)
	{
		/* Message */
		msg_print("Compacting objects...");

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);
	}


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Examine the objects */
		for (i = 1; i < o_max; i++)
		{
			object_type *o_ptr = &o_list[i];

			object_kind *k_ptr = &k_info[o_ptr->k_idx];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* Hack -- High level objects start out "immune" */
			if (k_ptr->level > cur_lev) continue;

			/* Monster */
			if (o_ptr->held_m_idx)
			{
				monster_type *m_ptr;

				/* Get the monster */
				m_ptr = &m_list[o_ptr->held_m_idx];

				/* Get the location */
				y = m_ptr->fy;
				x = m_ptr->fx;

				/* Monsters protect their objects */
				if (rand_int(100) < 90) continue;
			}

			/* Dungeon */
			else
			{
				/* Get the location */
				y = o_ptr->iy;
				x = o_ptr->ix;
			}

			/* Nearby objects start out "immune" */
			if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact artifacts in emergencies */
			if (artifact_p(o_ptr) && (cnt < 1000)) chance = 100;

			/* Apply the saving throw */
			if (rand_int(100) < chance) continue;

			/* Delete the object */
			delete_object_idx(i);

			/* Count it */
			num++;
		}
	}


	/* Excise dead objects (backwards!) */
	for (i = o_max - 1; i >= 1; i--)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip real objects */
		if (o_ptr->k_idx) continue;

		/* Move last object into open hole */
		compact_objects_aux(o_max - 1, i);

		/* Compress "o_max" */
		o_max--;
	}
}




/*
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 *
 * Hack -- we clear the "cave_o_idx[y][x]" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */
void wipe_o_list(void)
{
	int i;

	/* Delete the existing objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Hack -- Preserve unknown artifacts */
		if (artifact_p(o_ptr) && !object_known_p(o_ptr))
		{
			/* Mega-Hack -- Preserve the artifact */
			a_info[o_ptr->name1].cur_num = 0;
		}

		/* Monster */
		if (o_ptr->held_m_idx)
		{
			monster_type *m_ptr;

			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else
		{
			/* Get the location */
			int y = o_ptr->iy;
			int x = o_ptr->ix;

			/* Hack -- see above */
			cave_o_idx[y][x] = 0;
		}

		/* Wipe the object */
		object_wipe(o_ptr);
	}

	/* Reset "o_max" */
	o_max = 1;

	/* Reset "o_cnt" */
	o_cnt = 0;
}


/*
 * Get and return the index of a "free" object.
 *
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
s16b o_pop(void)
{
	int i;


	/* Initial allocation */
	if (o_max < z_info->o_max)
	{
		/* Get next space */
		i = o_max;

		/* Expand object array */
		o_max++;

		/* Count objects */
		o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Recycle dead objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[i];

		/* Skip live objects */
		if (o_ptr->k_idx) continue;

		/* Count objects */
		o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Warn the player (except during dungeon creation) */
	if (character_dungeon) msg_print("Too many objects!");

	/* Oops */
	return (0);
}


/*
 * Apply a "object restriction function" to the "object allocation table"
 */
errr get_obj_num_prep(void)
{
	int i;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Scan the allocation table */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Accept objects which pass the restriction, if any */
		if (!get_obj_num_hook || (*get_obj_num_hook)(table[i].index))
		{
			int chance = table[i].prob1;
			int level = k_info[table[i].index].level;

			/* As objects appear in stacks, their frequency decreases */
			switch(table[i].index)
			{
				case TV_POTION:
				case TV_SCROLL:
				case TV_FLASK:
				{
					if ((level < 75) && (object_level > level + 9)) chance /= (level >= 50 ? 2 : (level >= 25 ? 4 : 8));
					else if ((level < 50) && (object_level > level + 4)) chance /= (level >= 25 ? 2 : 4);
					break;
				}
				/* Activatable rings stack */
				case TV_RING:
				{
					if ((k_info[table[i].index].flags3 & (TR3_ACTIVATE | TR3_UNCONTROLLED)) == 0) break;

					/* Fall through */
				}
				case TV_ROD:
				case TV_STAFF:
				case TV_WAND:
				case TV_FOOD:
				case TV_MUSHROOM:
				{
					if (object_level > level + 9) chance /= 3;
					else if (object_level > level + 4) chance /= 2;
					break;
				}
			}

			/* Accept this object */
			table[i].prob2 = chance;
		}

		/* Do not use this object */
		else
		{
			/* Decline this object */
			table[i].prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/*
 * Choose an object kind that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "object allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" object, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire an object of the given level
 * than one of a lower level.  This is done by choosing several objects
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no objects are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_obj_num(int level)
{
	int i, j, p;

	int k_idx;

	long value, total;

	object_kind *k_ptr;

	alloc_entry *table = alloc_kind_table;


	/* Boost level */
	if (level > 0)
	{
		/* Occasional "boost" */
		if (rand_int(GREAT_OBJ) == 0)
		{
			/* What a bizarre calculation */
			level = 1 + (level * MAX_DEPTH / randint(MAX_DEPTH));
		}
	}


	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Objects are sorted by depth */
		if (table[i].level > level) break;

		/* Default */
		table[i].prob3 = 0;

		/* Get the index */
		k_idx = table[i].index;

		/* Get the actual kind */
		k_ptr = &k_info[k_idx];

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal objects */
	if (total <= 0) return (0);


	/* Pick an object */
	value = rand_int(total);

	/* Find the object */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = rand_int(100);

	/* Try for a "better" object once (50%) or twice (10%) */
	if (p < 60)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}


	/* Result */
	return (table[i].index);
}



/* Know an object fully */
/*
 * Call this after knowing an object. Currently just marks object with
 * mental flag.
 */
void object_mental(object_type *o_ptr, bool floor)
{
	u32b f1,f2,f3,f4;

	/* Now we know about the item */
	o_ptr->ident |= (IDENT_MENTAL);

	/* And we learn the rune recipe if any */
	k_info[o_ptr->k_idx].aware |= (AWARE_RUNES);

	/* And we learn the ego rune recipe if any */
	if (o_ptr->name2) e_info[o_ptr->name2].aware |= (AWARE_RUNES);

	/* Spoil the object */
	object_flags(o_ptr,&f1,&f2,&f3,&f4);

	object_can_flags(o_ptr,f1,f2,f3,f4, floor);

	object_not_flags(o_ptr,~(f1),~(f2),~(f3),~(f4), floor);
}


/*
 * Known is true when the "attributes" of an object are "known".
 *
 * These attributes include tohit, todam, toac, cost, and pval (charges).
 *
 * Note that "knowing" an object gives you everything that an "awareness"
 * gives you, and much more.  In fact, the player is always "aware" of any
 * item of which he has full "knowledge".
 *
 * But having full knowledge of, say, one "wand of wonder", does not, by
 * itself, give you knowledge, or even awareness, of other "wands of wonder".
 * It happens that most "identify" routines (including "buying from a shop")
 * will make the player "aware" of the object as well as fully "know" it.
 *
 * This routine also removes any inscriptions generated by "feelings".
 */
void object_known(object_type *o_ptr)
{
	/* Remove special inscription, except special inscriptions */
	if (o_ptr->feeling < INSCRIP_MIN_HIDDEN) o_ptr->feeling = 0;
	if (o_ptr->feeling >= MAX_INSCRIP) o_ptr->feeling = 0;

	/* The object is not "sensed" */
	o_ptr->ident &= ~(IDENT_SENSE);

	/* The object is not "partially sensed" */
	o_ptr->ident &= ~(IDENT_BONUS);

	/* The object is not "charge sensed" */
	o_ptr->ident &= ~(IDENT_CHARGES);

	/* The object is not "name sensed" */
	o_ptr->ident &= ~(IDENT_NAME);

	/* The object is not "pval sensed" */
	o_ptr->ident &= ~(IDENT_PVAL);

	/* Now we know about the item */
	o_ptr->ident |= (IDENT_KNOWN);
}


/*
 * Sense the bonus of an object
 *
 * Note under some circumstances, we fully know the object (When
 * its bonuses are all there is to know about it).
 */
void object_bonus(object_type *o_ptr, bool floor)
{
	/* Identify the bonuses */
	o_ptr->ident |= (IDENT_BONUS);

	/* Sense the item (if appropriate) */
	if (!object_known_p(o_ptr))
	{
		sense_magic(o_ptr, 1, TRUE, floor);
	}

	/* For armour/weapons - is this all we need to know? */
	if ((o_ptr->tval != TV_STAFF) && ((o_ptr->feeling == INSCRIP_AVERAGE) ||
			(o_ptr->feeling == INSCRIP_GOOD) ||
			(o_ptr->feeling == INSCRIP_VERY_GOOD) ||
			(o_ptr->feeling == INSCRIP_GREAT)))
	{
		object_known(o_ptr);
	}
}


/*
 * Sense the bonus/charges of an object
 *
 * Note under some circumstances, we fully know the object (When
 * its bonuses/charges are all there is to know about it).
 */
void object_gauge(object_type *o_ptr, bool floor)
{
	/* Identify the bonuses */
	o_ptr->ident |= (IDENT_BONUS | IDENT_CHARGES | IDENT_PVAL);

	/* Have we already learnt the name? */
	if (o_ptr->ident & (IDENT_NAME))
	{
		object_known(o_ptr);
	}

	/* Is this all we need to know */
	else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING))
	{
		if (object_aware_p(o_ptr)) object_known(o_ptr);
	}

	/* Sense the item (if appropriate) */
	if (!object_known_p(o_ptr))
	{
		sense_magic(o_ptr, 1, TRUE, floor);
	}

	/* For armour/weapons - is this all we need to know? */
	if ((o_ptr->tval != TV_STAFF) && ((o_ptr->feeling == INSCRIP_AVERAGE) ||
			(o_ptr->feeling == INSCRIP_GOOD) ||
			(o_ptr->feeling == INSCRIP_VERY_GOOD) ||
			(o_ptr->feeling == INSCRIP_GREAT)))
	{
		object_known(o_ptr);
	}
}



/*
 * Ensure tips are displayed as objects become either weakly or strongly aware
 */
void object_aware_tips(object_type *o_ptr, bool seen)
{
	int i, count = 0;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Check all objects */
	for (i = 0; i < z_info->k_max; i++)
	{
		/* Skip non-matching tvals */
		if (k_info[i].tval != k_ptr->tval) continue;

		/* Count number of objects known */
		if (k_info[i].aware & (AWARE_SEEN | AWARE_EXISTS)) count++;
	}

	/* We didn't know the object */
	if ((seen) && ((k_ptr->aware & (AWARE_SEEN)) == 0))
	{
		/* Seen the object */
		k_ptr->aware |= (AWARE_SEEN);

		/* Increase tvals seen */
		count++;

		/* Show tval tips if no objects of this type known */
		if (!count)
		{
			/* Show tval based tip */
			queue_tip(format("tval%d.txt", k_ptr->tval));
		}
		/* Show tval tips if 'count' svals of this type known */
		else
		{
			/* Show tval based tip */
			queue_tip(format("tval%d-%d.txt", k_ptr->tval, count));
		}
	}
	
	/* XXX XXX - Mark monster objects as "seen" */
	if ((seen) && (o_ptr->name3 > 0) && !(l_list[o_ptr->name3].sights))
	{
		l_list[o_ptr->name3].sights++;

		queue_tip(format("look%d.txt", o_ptr->name3));
	}

	/* We didn't note the object */
	if (((k_ptr->aware & (AWARE_EXISTS)) == 0) && (!(k_ptr->flavor) || (object_aware_p(o_ptr))))
	{
		/* Noted object */
		k_ptr->aware |= (AWARE_EXISTS);

		/* Show tip for kind of object */
		queue_tip(format("kind%d.txt", o_ptr->k_idx));
	}
}


/*
 * The player is now aware of the effects of the given object.
 */
void object_aware(object_type *o_ptr, bool floor)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	int i;

	u32b f1, f2, f3, f4;

	/* Get the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* No longer guessing */
	k_ptr->guess = 0;

	/* The object name is not guessed */
	o_ptr->guess1 = 0;
	o_ptr->guess2 = 0;

	/* No longer tried */
	k_ptr->aware &= ~(AWARE_TRIED);

	/* No longer sensed */
	if (k_ptr->aware & (AWARE_SENSEX))
	{
		k_ptr->aware |= (AWARE_SENSE);
		k_ptr->aware &= ~(AWARE_SENSEX);
	}

	/* Auto-inscribe */
	if (o_ptr->name2)
	{
		if (!o_ptr->note) o_ptr->note = e_info[o_ptr->name2].note;
	}
	else
	{
		if (!o_ptr->note) o_ptr->note = k_info[o_ptr->k_idx].note;
	}

	/* Remove name3 if artifact */
	if (o_ptr->name1) o_ptr->name3 = 0;

	/* Lose "magic bag" feeling */
	if (o_ptr->feeling >= MAX_INSCRIP) o_ptr->feeling = 0;

	/* Fully aware of the effects */
	k_info[o_ptr->k_idx].aware |= (AWARE_FLAVOR);

	/* Learn rune recipe if runes known */
	if (k_info[o_ptr->k_idx].aware & (AWARE_RUNEX))
	{
		k_info[o_ptr->k_idx].aware |= (AWARE_RUNES);
		k_info[o_ptr->k_idx].aware &= ~(AWARE_RUNEX);
	}

	/* Hack -- fully aware of the coating effects */
	if (o_ptr->xtra1 >= OBJECT_XTRA_MIN_COATS)
	{
		object_type object_type_body;
		object_type *i_ptr = &object_type_body;

		int coating = lookup_kind(o_ptr->xtra1, o_ptr->xtra2);

		/* Prepare object */
		object_prep(i_ptr,coating);

		/* Queue tips */
		object_aware_tips(i_ptr, FALSE);

		/* Make coating aware */
		k_info[coating].aware |= (AWARE_FLAVOR);

		/* Forget tried flag */
		k_info[coating].aware &= ~(AWARE_TRIED);
	}

	/* Identify the name */
	o_ptr->ident |= (IDENT_NAME);

	/* Is this all we need to know? - wands */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->ident & (IDENT_CHARGES))
			object_known(o_ptr);
	}

	/* Is this all we need to know? - staffs */
	else if (o_ptr->tval == TV_STAFF)
	{
		if ((o_ptr->ident & (IDENT_CHARGES)) && (o_ptr->ident & (IDENT_BONUS)))
			object_known(o_ptr);
	}

	/* Is this all we need to know? - other wearable items */
	else if ((o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET)
			  || (o_ptr->tval >= TV_SHOT && o_ptr->tval <= TV_DRAG_ARMOR))
	{
		if (((!(o_ptr->to_h) && !(o_ptr->to_d) && !(o_ptr->to_a))
				|| (o_ptr->ident & (IDENT_BONUS)))
				&& (!(o_ptr->pval) || (o_ptr->ident & (IDENT_PVAL))))
			object_known(o_ptr);
	}

	/* Is this all we need to know? - flavoured items */
	else if (k_info[o_ptr->k_idx].flavor)
	{
		object_known(o_ptr);
	}

	/* Process objects */
	for (i = 1; i < o_max; i++)
	{
		/* Get the object */
		object_type *i_ptr = &o_list[i];

		/* Skip dead objects */
		if (!i_ptr->k_idx) continue;

		/* Re-evaluate the object */
		object_guess_name(i_ptr);
	}

	/* Process objects */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		/* Get the object */
		object_type *i_ptr = &inventory[i];

		/* Skip dead objects */
		if (!i_ptr->k_idx) continue;

		/* Re-evaluate the object */
		object_guess_name(i_ptr);
	}

	/* Set the obvious flags */
	object_obvious_flags(o_ptr, floor);

	/* Now we know what it is, update what we know about it */
	object_can_flags(o_ptr,o_ptr->can_flags1,
			o_ptr->can_flags2,
			o_ptr->can_flags3,
			o_ptr->can_flags4, floor);

	object_not_flags(o_ptr,o_ptr->not_flags1,
			o_ptr->not_flags2,
			o_ptr->not_flags3,
			o_ptr->not_flags4, floor);

	object_may_flags(o_ptr,o_ptr->may_flags1,
			o_ptr->may_flags2,
			o_ptr->may_flags3,
			o_ptr->may_flags4, floor);

	/* Add a tip */
	object_aware_tips(o_ptr, FALSE);

	/* Know about ego-type */
	if ((o_ptr->name2) && !(o_ptr->ident & (IDENT_STORE)))
	{
		/* Show tips if required */
		if (!(e_info[o_ptr->name2].aware & (AWARE_EXISTS)))
		{
			queue_tip(format("ego%d.txt", o_ptr->name2));
		}

		/* Know ego exists */
		e_info[o_ptr->name2].aware |= (AWARE_EXISTS);

		/* Learn ego runes */
		if (o_ptr->ident & (IDENT_RUNES))
		{
			e_info[o_ptr->name2].aware |= (AWARE_RUNES);
		}
	}
}


/*
 * Something has been "sampled"
 */
void object_tried(object_type *o_ptr)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Don't mark it if aware */
	if (object_aware_p(o_ptr)) return;

	/* Mark it as tried */
	k_ptr->aware |= (AWARE_TRIED);
}


/*
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s32b object_value_base(const object_type *o_ptr)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Use template cost for aware objects */
	if (object_aware_p(o_ptr)) return (k_ptr->cost);

	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Un-aware Food */
		case TV_FOOD: return (1L);

		/* Un-aware Mushrooms */
		case TV_MUSHROOM: return (5L);

		/* Un-aware Potions */
		case TV_POTION: return (20L);

		/* Un-aware Scrolls */
		case TV_SCROLL: return (20L);

		/* Un-aware Staffs */
		case TV_STAFF: return (70L);

		/* Un-aware Wands */
		case TV_WAND: return (50L);

		/* Un-aware Rods */
		case TV_ROD: return (90L);

		/* Un-aware Rings */
		case TV_RING: return (45L);

		/* Un-aware Amulets */
		case TV_AMULET: return (45L);
	}

	/* Paranoia -- Oops */
	return (0L);
}


/*
 * Return the "real" price of a "known" item, not including discounts.
 *
 * Wand and staffs get cost for each charge.
 *
 * Armor is worth an extra 100 gold per bonus point to armor class.
 *
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).
 *
 * Missiles are only worth 5 gold per bonus point, since they
 * usually appear in groups of 20, and we want the player to get
 * the same amount of cash for any "equivalent" item.  Note that
 * missiles never have any of the "pval" flags, and in fact, they
 * only have a few of the available flags, primarily of the "slay"
 * and "brand" and "ignore" variety.
 *
 * Weapons with negative hit+damage bonuses are worthless.
 *
 * Every wearable item with a "pval" bonus is worth extra (see below).
 */
s32b object_value_real(const object_type *o_ptr)
{
	s32b value;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	int power;

	/* Hack -- "worthless" items */
	if (!k_ptr->cost) return (0L);

	/* Base cost */
	value = k_ptr->cost;

	/* Gold / gems */
	if ((o_ptr->tval == TV_GOLD) || (o_ptr->tval == TV_GEMS))
	{
		return (o_ptr->charges);
	}
	
	/* Artifact */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		/* Hack -- "worthless" artifacts */
		if (!a_ptr->cost) return (0L);

		/* Hack -- Use the artifact cost instead */
		value = a_ptr->cost;

		/* No negative value */
		if (value < 0) value = 0;

		return (value);
	}

	/* Ego-Item */
	else if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		/* Hack -- "worthless" ego-items */
		if (!e_ptr->cost) return (0L);

		/* Hack -- Reward the ego-item with a bonus */
		value += e_ptr->cost;
	}

	/* Now evaluate object power */
	power = object_power(o_ptr);

	/* A hack caused by "inhibit" in info.c */
	if (power > 1000)
		return 0;

#if 0
	/* Hack -- No negative power on uncursed objects */
	if ((power < 0) && !(cursed_p(o_ptr))) power = 0;
#endif
	/* Apply power modifier to cost */
	if ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))
	{
		value += power * (power > 0 ? (power + 2) / 3 : (-power + 2) / 3) * 5L;
	}
	else
	{
		value += power * (power > 0 ? (power + 2) / 3 : (-power + 2) / 3) * 100L;
	}

	/* Hack -- object power assumes (+11,+9) on weapons and ammo so we need to include some smaller bonuses,
		and ac bonus on armour equal to its base armour class so we need to include some bonuses. */
	switch (o_ptr->tval)
	{
		/* Armour */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Factor in the bonuses not considered by power equation */
			value += o_ptr->to_a <= o_ptr->ac ? o_ptr->to_a * 100L : o_ptr->ac * 100L;

			break;
		}

		/* Wands/Staffs */
		case TV_WAND:
		case TV_STAFF:
		{
			/* Pay extra for charges, depending on standard number of charges */
			value += ((value / 20) * o_ptr->charges);

			if (o_ptr->tval == TV_WAND) break;

			/* Fall through */
		}

		/* Bows/Weapons/Ammo */
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_SWORD:
		case TV_POLEARM:

		{
			/* Factor in the bonuses not considered by power equation */
		  /* TODO: change now that to_d is bound by weapon dice? */
			value += o_ptr->to_h < 12 ? o_ptr->to_h * 100L : 1200L;
			value += o_ptr->to_d < 10 ? o_ptr->to_d * 100L : 1000L;

			/* Done */
			break;
		}

		/* Ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			/* Factor in the bonuses not considered by power equation */
			value += o_ptr->to_h < 12 ? o_ptr->to_h * 5L : 60L;
			value += o_ptr->to_d < 10 ? o_ptr->to_d * 5L : 50L;

			/* Done */
			break;
		}
	}

	/* No negative value */
	if (value < 0) value = 0;

	/* Return the value */
	return (value);
}


/*
 * Return the price of an item including plusses (and charges).
 *
 * This function returns the "value" of the given item (qty one).
 *
 * Never notice "unknown" bonuses or properties, including "curses",
 * since that would give the player information he did not have.
 *
 * Now add a value for sensed items. This should be dependent on
 * the minimum good item values in e_info.txt, but just uses a rule
 * of thumb at this stage.
 *
 * Note that discounted items stay discounted forever.
 */
s32b object_value(const object_type *o_ptr)
{
	s32b value = 0;
	s32b pval = 0;

	/* Known items -- acquire the actual value */
	if (object_known_p(o_ptr) || ((o_ptr->ident & (IDENT_VALUE | IDENT_STORE)) != 0))
	{
		/* Broken items -- worthless */
		if (broken_p(o_ptr)) return (0L);

		/* Cursed items -- worthless */
		if (cursed_p(o_ptr)) return (0L);

		/* Real value (see above) */
		value = object_value_real(o_ptr);
	}

	/* Hack -- gold always worth something */
	else if (o_ptr->tval >= TV_GOLD)
	{
		/* Quick estimate */
		return ((o_ptr->k_idx - OBJ_GOLD_LIST + 1) * 4);
	}

	/* Hack -- Felt broken items */
	else if (o_ptr->feeling == INSCRIP_TERRIBLE
			  || o_ptr->feeling == INSCRIP_WORTHLESS
			  || o_ptr->feeling == INSCRIP_CURSED
			  || o_ptr->feeling == INSCRIP_BROKEN)
	{
		return (0L);
	}

	/* Named value (use 'real' value and attempt to hack) */
	else if (object_named_p(o_ptr))
	{
		object_type object_type_body;
		object_type *j_ptr = &object_type_body;

		/* Copy object */
		object_copy(j_ptr, o_ptr);

		/* Remove unknown information */
		if (!object_bonus_p(o_ptr))
		{
			if (o_ptr->name1)
			{
				j_ptr->to_h = a_info[o_ptr->name1].to_h;
				j_ptr->to_d = a_info[o_ptr->name2].to_d;
				j_ptr->to_a = a_info[o_ptr->name3].to_a;
			}
			else if (o_ptr->name2)
			{
				j_ptr->to_h = e_info[o_ptr->name2].max_to_h / 2;
				j_ptr->to_d = e_info[o_ptr->name2].max_to_d / 2;
				j_ptr->to_a = e_info[o_ptr->name2].max_to_a / 2;
			}
			else
			{
				j_ptr->to_h = k_info[o_ptr->to_h].to_h;
				j_ptr->to_d = k_info[o_ptr->to_d].to_d;
				j_ptr->to_a = k_info[o_ptr->to_a].to_a;
			}
		}
		if (!object_charges_p(o_ptr)) j_ptr->charges = 0;
		if (!object_pval_p(o_ptr))
		{
			if (o_ptr->name1)
			{
				j_ptr->pval = a_info[o_ptr->name1].pval;
			}
			else if (o_ptr->name2)
			{
				j_ptr->pval = e_info[o_ptr->name2].max_pval / 2;
			}
			else
			{
				j_ptr->pval = k_info[o_ptr->to_h].pval;
			}
		}

		/* Try curse analysis */
		if (o_ptr->name1)
		{
			if (a_info[o_ptr->name1].flags3 & (TR3_LIGHT_CURSE | TR3_HEAVY_CURSE | TR3_PERMA_CURSE | TR3_UNCONTROLLED))
				j_ptr->ident |= (IDENT_CURSED);
		}
		else if (o_ptr->name2)
		{
			if (e_info[o_ptr->name2].flags3 & (TR3_LIGHT_CURSE | TR3_HEAVY_CURSE | TR3_PERMA_CURSE | TR3_UNCONTROLLED))
				j_ptr->ident |= (IDENT_CURSED);
		}
		else
		{
			if (k_info[o_ptr->k_idx].flags3 & (TR3_LIGHT_CURSE | TR3_HEAVY_CURSE | TR3_PERMA_CURSE | TR3_UNCONTROLLED))
				j_ptr->ident |= (IDENT_CURSED);
			else
				j_ptr->ident &= ~(IDENT_CURSED | IDENT_BROKEN);
		}

		/* Hack -- get 'real' value */
		value = object_value_real(j_ptr);

		/* Apply discount (if any) */
		if (o_ptr->discount > 0)
		{
			value -= (value * o_ptr->discount / 100L);
		}
	}
	/* Unknown items -- acquire a base value */
	else
	{
		/* Base value (see above) */
		value = object_value_base(o_ptr);

		/* Hack -- assess known pval */
		if (object_pval_p(o_ptr))
		{
			/* Hack -- negative bonuses are bad */
			if (o_ptr->pval < 0) return (0L);

			/* Guess value of pval, ignore default pval of 1 */
			if (o_ptr->pval > 1)
				pval = o_ptr->pval * o_ptr->pval * 100L;
		}

		/* Hack -- Partially identified items */
		if (object_bonus_p(o_ptr))
		{
			switch(o_ptr->tval)
			{
				/* Rings/Amulets */
				case TV_RING:
				case TV_AMULET:
				{
					/* Hack -- negative bonuses are bad */
					if (o_ptr->to_a < 0) return (0L);
					if (o_ptr->to_h < 0) return (0L);
					if (o_ptr->to_d < 0) return (0L);

					/* Give credit for bonuses */
					value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L) + pval;

					/* Done */
					break;
				}

				/* Armor */
				case TV_BOOTS:
				case TV_GLOVES:
				case TV_CLOAK:
				case TV_CROWN:
				case TV_HELM:
				case TV_SHIELD:
				case TV_SOFT_ARMOR:
				case TV_HARD_ARMOR:
				case TV_DRAG_ARMOR:
				case TV_INSTRUMENT:
				{
					/* Give credit for hit bonus */
					value += ((o_ptr->to_h) * 100L);

					/* Give credit for damage bonus */
					value += ((o_ptr->to_d) * 100L);

					/* Give credit for armor bonus */
					value += (o_ptr->to_a * 100L);

					/* Give credit for known pval */
					value += pval;

					/* Done */
					break;
				}

				/* Bows/Weapons */
				case TV_BOW:
				case TV_DIGGING:
				case TV_HAFTED:
				case TV_SWORD:
				case TV_POLEARM:
				case TV_STAFF:
				{
					/* Hack -- negative hit/damage bonuses */
					if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

					/* Factor in the bonuses */
					value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);

					/* Give credit for known pval */
					value += pval;
				}

				/* Ammo */
				case TV_SHOT:
				case TV_ARROW:
				case TV_BOLT:
				{
					/* Hack -- negative hit/damage bonuses */
					if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

					/* Factor in the bonuses */
					value += ((o_ptr->to_h + o_ptr->to_d) * 5L);

					/* Done */
					break;
				}
			}
		}

		/* Add bonus for known charges */
		if (object_charges_p(o_ptr))
		{
			switch(o_ptr->tval)
			{
				/* Wands/staffs */
				case TV_WAND:
				case TV_STAFF:
				{
					if (object_charges_p(o_ptr))
					{
						/* Hack -- negative/zero hit/damage bonuses */
						if (o_ptr->charges <= 0) return (0L);

						/* Factor in the bonuses. Note hack to ensure
						 * that wands of spark are less valuable per charge */
						if (k_info[o_ptr->k_idx].level > 5) value += (o_ptr->charges * 5L);
					}
					/* Done */
					break;
				}
			}
		}

		/* Hack -- Felt good items. */
		if (o_ptr->ident & (IDENT_SENSE))
		{
			s32b bonus=0L;

			switch (o_ptr->feeling)
			{
				case INSCRIP_SPECIAL:
				{
					bonus =10000;
					break;
				}
				case INSCRIP_ARTIFACT:
				case INSCRIP_UNBREAKABLE:
				{
					bonus = 1000;
				}
				case INSCRIP_SUPERB:
				{
					bonus =2000;
					break;
				}
				case INSCRIP_HIGH_EGO_ITEM:
				{
					bonus = 1000;
					break;
				}
				case INSCRIP_EXCELLENT:
				{
					bonus =400;
					break;
				}
				case INSCRIP_EGO_ITEM:
				case INSCRIP_UNGETTABLE:
				{
					bonus = 200;
					break;
				}
				case INSCRIP_GREAT:
				{
					if (!object_bonus_p(o_ptr)) bonus =800;
					break;
				}
				case INSCRIP_VERY_GOOD:
				{
					if (!object_bonus_p(o_ptr)) bonus =400;
					break;
				}
				case INSCRIP_GOOD:
				{
					if (!object_bonus_p(o_ptr)) bonus =100;
					break;
				}
				case INSCRIP_UNCURSED:
				{
					bonus = 25;
					break;
				}
				case INSCRIP_MAGICAL:
				case INSCRIP_VALUABLE:
				{
					bonus = 100;
				}
			}

			if ((o_ptr->tval == TV_SHOT) ||
				(o_ptr->tval == TV_ARROW) ||
				(o_ptr->tval == TV_BOLT))
			{
				value += bonus/20;
			}
			else if (!object_bonus_p(o_ptr))
			{
				value += bonus;
			}
			else
			{
				value += bonus / 2;
			}
		}
	}

	/* Apply discount (if any) */
	if (o_ptr->discount > 0)
	{
		value -= (value * o_ptr->discount / 100L);
	}


	/* Return the final value */
	return (value);
}

/*
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow wands/staffs (if they are known to have equal
 * charges) and rods (if fully charged) to combine.  They will unstack
 * (if necessary) when they are used.
 *
 * If permitted, we allow weapons/armor to stack, if fully "known".
 * or if both stacks have the same "known" status.
 * This is done mostly to make unidentified stacks of missiles useful.
 *
 * Food, potions, scrolls, and "easy know" items always stack.
 *
 */
bool object_similar(const object_type *o_ptr, const object_type *j_ptr)
{
	int total = o_ptr->number + j_ptr->number;

	/* Hack -- magical bags */
	if ((o_ptr->tval == TV_BAG) || (j_ptr->tval == TV_BAG))
	{
		const object_type *i_ptr = j_ptr;

		int i, sval = o_ptr->sval;

		if (j_ptr->tval == TV_BAG)
		{
			i_ptr = o_ptr;
			sval = j_ptr->sval;
		}

		/* Find slot */
		for (i = 0; i < INVEN_BAG_TOTAL; i++)
		{
			if ((bag_holds[sval][i][0] == i_ptr->tval)
				&& (bag_holds[sval][i][1] == i_ptr->sval)) return (TRUE);
		}
	}

	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return (0);

	/* Analyze the items */
	switch (o_ptr->tval)
	{
		/* Money */
		case TV_GEMS:
		case TV_GOLD:
		{
			/* Avoid overflow */
			if (o_ptr->charges >= (2 << 15) - j_ptr->charges) return (0);

			/* Always okay */
			return (TRUE);
		}

		/* Spells */
		case TV_SPELL:
		{
			/* Never okay */
			return (0);
		}


		/* Bodies, Skeletons, Skins, Eggs, Statues */
		case TV_ASSEMBLY:
		case TV_STATUE:
		case TV_EGG:
		case TV_BONE:
		case TV_BODY:
		case TV_SKIN:
		case TV_HOLD:
		{
			/* Probably okay */
			break;
		}

		/* Food and Potions and Scrolls */
		case TV_SPIKE:
		case TV_FOOD:
		case TV_MUSHROOM:
		case TV_POTION:
		case TV_SCROLL:
		case TV_RUNESTONE:
		case TV_MAP:
		case TV_FLASK:
		case TV_ROPE:
		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:
		case TV_SONG_BOOK:
		case TV_BAG:
		{
			/* Assume okay */
			break;
		}

		/* Wands */
		case TV_WAND:
		{
			/* Require identical knowledge of both items */
			if (object_charges_p(o_ptr) != object_charges_p(j_ptr)) return (0);

			/* Require 'similar' charges */
			if ((o_ptr->charges != j_ptr->charges))
			{
				/* Check for sign difference */
				if ((o_ptr->charges > 0)&&(j_ptr->charges <= 0)) return (0);
				if ((o_ptr->charges < 0)&&(j_ptr->charges >= 0)) return (0);
				if ((o_ptr->charges == 0)&&(j_ptr->charges != 0)) return (0);
			}

			/* Probably okay */
			break;
		}

		/* Rods */
		case TV_ROD:
		{
			/* Probably okay */
			break;
		}

		/* Lites */
		case TV_LITE:
		{
			if ((o_ptr->charges != 0) && (j_ptr->timeout != 0)) return (FALSE);
			if ((o_ptr->timeout != 0) && (j_ptr->charges != 0)) return (FALSE);

			/* Fall through */
		}

		/* Weapons and Armor */
		case TV_STAFF:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Fall through */
		}

		/* Rings, Amulets, Instruments */
		case TV_RING:
		case TV_AMULET:
		case TV_INSTRUMENT:

		/* Missiles */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			/* Require identical knowledge of both items */
			if (object_known_p(o_ptr) != object_known_p(j_ptr))
			{
				if (cheat_xtra) msg_print("Known state does not match");
				return (0);
			}
			if (object_named_p(o_ptr) != object_named_p(j_ptr))
			{
				if (cheat_xtra) msg_print("Named state does not match");
				return (0);
			}
			if (object_bonus_p(o_ptr) != object_bonus_p(j_ptr))
			{
				if (cheat_xtra) msg_print("Bonus state does not match");
				return (0);
			}
			if (object_pval_p(o_ptr) != object_pval_p(j_ptr))
			{
				if (cheat_xtra) msg_print("Pval state does not match");
				return (0);
			}

			/* Require identical "bonuses" */
			if (o_ptr->to_h != j_ptr->to_h) return (FALSE);
			if (o_ptr->to_d != j_ptr->to_d) return (FALSE);
			if (o_ptr->to_a != j_ptr->to_a) return (FALSE);
			if (o_ptr->pval != j_ptr->pval) return (FALSE);

			/* XXX Just merge coating "charges" */
			/* Require identical "artifact" names */
			if (o_ptr->name1 != j_ptr->name1) return (FALSE);

			/* Require identical "ego-item" names */
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);

			/* Require identical "values" */
			if (o_ptr->ac != j_ptr->ac) return (FALSE);
			if (o_ptr->dd != j_ptr->dd) return (FALSE);
			if (o_ptr->ds != j_ptr->ds) return (FALSE);

			/* Probably okay */
			break;
		}

		/* Study items */
		case TV_STUDY:
		{
			/* Require identical pvals */
			if (o_ptr->pval != j_ptr->pval) return (FALSE);
			break;
		}

		/* Various */
		default:
		{
			/* Require knowledge */
			if (!object_named_p(o_ptr) || !object_named_p(j_ptr)) return (0);

			/* Probably okay */
			break;
		}
	}

	/* Hack -- Require identical "cursed" and "broken" status and "breaking" and "stored" status */
	if (((o_ptr->ident & (IDENT_CURSED)) != (j_ptr->ident & (IDENT_CURSED))) ||
	    ((o_ptr->ident & (IDENT_BROKEN)) != (j_ptr->ident & (IDENT_BROKEN))) ||
	    ((o_ptr->ident & (IDENT_BREAKS)) != (j_ptr->ident & (IDENT_BREAKS))) ||
	    ((o_ptr->ident & (IDENT_FORGED)) != (j_ptr->ident & (IDENT_FORGED))) ||
	    ((o_ptr->ident & (IDENT_STORE)) != (j_ptr->ident & (IDENT_STORE))))
	{
		/* Debugging code for stacking problems */
		if (cheat_xtra)
		{
			if ((o_ptr->ident & (IDENT_CURSED)) != (j_ptr->ident & (IDENT_CURSED))) msg_print("Ident cursed not matching");
			if ((o_ptr->ident & (IDENT_BROKEN)) != (j_ptr->ident & (IDENT_BROKEN))) msg_print("Ident broken not matching");
			if ((o_ptr->ident & (IDENT_BREAKS)) != (j_ptr->ident & (IDENT_BREAKS))) msg_print("Ident breaks not matching");
			if ((o_ptr->ident & (IDENT_STORE)) != (j_ptr->ident & (IDENT_STORE))) msg_print("Ident store not matching");
			if ((o_ptr->ident & (IDENT_FORGED)) != (j_ptr->ident & (IDENT_FORGED))) msg_print("Ident forged not matching");
		}

		return (0);
	}

	/* Hack -- Require identical "xtra1" status */
	if (o_ptr->xtra1 != j_ptr->xtra1)
	{
		if (cheat_xtra) msg_format("xtra1 %d does not match xtra1 %d", o_ptr->xtra1, j_ptr->xtra1);
		return (0);
	}

	/* Hack -- Require identical "xtra2" status */
	if (o_ptr->xtra2 != j_ptr->xtra2)
	{
		if (cheat_xtra) msg_format("xtra2 %d does not match xtra2 %d", o_ptr->xtra2, j_ptr->xtra2);
		return (0);
	}

	/* Hack -- Require identical "name3" status */
	if (o_ptr->name3 != j_ptr->name3)
	{
	  if (cheat_xtra) msg_format("name3 %d does not match name3 %d for tval=%d sval=%d, tval=%d sval=%d", o_ptr->name3, j_ptr->name3, o_ptr->tval, o_ptr->sval, j_ptr->tval, j_ptr->sval);
		return (0);
	}

	/* Hack -- Require compatible inscriptions */
	if (o_ptr->note != j_ptr->note && o_ptr->note && j_ptr->note)
	{
		if (cheat_xtra) msg_format("note %d does not match note %d", o_ptr->note, j_ptr->note);

		return (0);
	}

	/* Hack -- Require compatible "feeling" fields */
	if (o_ptr->feeling != j_ptr->feeling && (o_ptr->feeling) && (j_ptr->feeling))
	{
		if (cheat_xtra) msg_format("feeling %d does not match feeling %d", o_ptr->feeling, j_ptr->feeling);

		return (0);
	}

	/* Maximal "stacking" limit */
	if (total >= MAX_STACK_SIZE) return (0);


	/* They match, so they must be similar */
	return (TRUE);
}


/*
 * Allow one item to "absorb" another, assuming they are similar.
 *
 * The blending of the "note" field assumes that either (1) one has an
 * inscription and the other does not, or (2) neither has an inscription.
 * In both these cases, we can simply use the existing note, unless the
 * blending object has a note, in which case we use that note.
 *
 * The blending of the "discount" field assumes that either (1) one is a
 * special inscription and one is nothing, or (2) one is a discount and
 * one is a smaller discount, or (3) one is a discount and one is nothing,
 * or (4) both are nothing.  In all of these cases, we can simply use the
 * "maximum" of the two "discount" fields.
 *
 * These assumptions are enforced by the "object_similar()" code.
 *
 * We now support 'blending' of timeouts and charges. - ANDY
 * Charges are averaged across a stack of items.
 * Timeouts are the maximum across a stack
 * of items.
 *
 * Note we treat rods like timeout items rather than charges items. This
 * unnecessarily complicates the code in a lot of places.
 *
 * Note for negative charge items, we don't use higher in the absolute
 * sense, and we have to mess around because division and modulo on
 * negative numbers is undefined in C.
 */

void object_absorb(object_type *o_ptr, const object_type *j_ptr, bool floor)
{
	int total = o_ptr->number + j_ptr->number;

	int number = ((total < MAX_STACK_SIZE) ? total : (MAX_STACK_SIZE - 1));

	/* Hack -- magical bags -- bag swallows object */
	if ((j_ptr->tval == TV_BAG) && (o_ptr->tval != TV_BAG))
	{
		object_type object_type_body;
		object_type *i_ptr = &object_type_body;

		/* Swap objects */
		object_copy(i_ptr, o_ptr);
		object_copy(o_ptr, j_ptr);
		object_copy(j_ptr, i_ptr);
	}

	/* Hack -- magical bags -- object swallowed by bag */
	if ((o_ptr->tval == TV_BAG) && (j_ptr->tval != TV_BAG))
	{
		int i;

		/* Find slot */
		for (i = 0; i < INVEN_BAG_TOTAL; i++)
		{
			if ((bag_holds[o_ptr->sval][i][0] != j_ptr->tval)
				|| (bag_holds[o_ptr->sval][i][1] != j_ptr->sval)) continue;

			/* Hack -- absorb wands */
			if (j_ptr->tval == TV_WAND)
			{
				/* Set total number */
				total = bag_contents[o_ptr->sval][i] + j_ptr->number;
				number = ((total < ((2 << 15) - 1)) ? total : (((2 << 15) - 1) - 1));
				bag_contents[o_ptr->sval][i] = number;

				/* Set total charges */
				total = bag_contents[o_ptr->sval+1][i] + j_ptr->charges * j_ptr->number - j_ptr->stackc;
				number = ((total < ((2 << 15) - 1)) ? total : (((2 << 15) - 1) - 1));
				bag_contents[o_ptr->sval+1][i] = number;
			}
			/* Hack -- absorb torches */
			else if ((j_ptr->tval == TV_LITE) && (j_ptr->sval == SV_LITE_TORCH))
			{
				/* Set total charges */
				total = bag_contents[o_ptr->sval][i] + j_ptr->charges * j_ptr->number - j_ptr->stackc;
				number = ((total < ((2 << 15) - 1)) ? total : (((2 << 15) - 1) - 1));
				bag_contents[o_ptr->sval][i] = number;
			}
			/* Hack -- absorb other */
			else
			{
				/* Set total number */
				total = bag_contents[o_ptr->sval][i] + j_ptr->number;
				number = ((total < ((2 << 15) - 1)) ? total : (((2 << 15) - 1) - 1));
				bag_contents[o_ptr->sval][i] = number;
			}

			/* Hack -- Decrease the weight -- will be increased later */
			p_ptr->total_weight -= (j_ptr->number * j_ptr->weight);
		}
		/* Aware of object? */
		if (!object_aware_p(j_ptr))
		{
			int k;
			k_info[j_ptr->k_idx].aware |= (AWARE_SENSEX);

		    /* Mark all such objects sensed */
		    /* Check world */
		    for (k = 0; k < o_max; k++)
		    {
				if ((o_list[k].k_idx == j_ptr->k_idx) && !(o_list[k].feeling))
				{
					o_list[k].feeling = o_ptr->sval + MAX_INSCRIP;
				}
		    }
			/* Check inventory */
			for (k = 0; k < INVEN_TOTAL; k++)
			{
				if ((inventory[k].k_idx == j_ptr->k_idx) && !(inventory[k].feeling))
				{
					inventory[k].feeling = o_ptr->sval + MAX_INSCRIP;
				}
			}
		}
		else
		{
			k_info[j_ptr->k_idx].aware |= (AWARE_SENSE);
		}

		/* Combine and re-order again */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		/* Successful */
		return;
	}

	/* Blend "charges" if necessary*/
	if (o_ptr->charges != j_ptr->charges)
	{
		/* Weird calculation. But it works. */
		s32b stackt = ((o_ptr->charges * o_ptr->number)
				+ (j_ptr->charges * j_ptr->number)
				- o_ptr->stackc
				- j_ptr->stackc);

		bool negative = (stackt < 0) ? TRUE : FALSE;

		/* Modulo on negative signs undefined in C */
		if (negative) stackt = -stackt;

		/* Assign charges */
		o_ptr->charges = stackt / total;
		o_ptr->stackc = number - (stackt % total);

		/* Fix charges */
		if ((o_ptr->stackc)) o_ptr->charges++;

		/* Hack -- Fix stack count */
		if (o_ptr->stackc >= number) o_ptr->stackc = 0;

		/* Correction for negative chargess */
		if (negative)
		{
			/* Reverse sign */
			o_ptr->charges = -o_ptr->charges;

			/* Reverse stack count */
			o_ptr->stackc = number - o_ptr->stackc;

			/* Fix charges */
			if ((o_ptr->stackc)) o_ptr->charges++;

			/* Paranoia -- always ensure cursed items */
			if (o_ptr->charges >=0)
			{
				o_ptr->charges = -1;
				o_ptr->stackc = 0;
			}

		}
	}

	/* Blend "timeout" if necessary */
	if (o_ptr->timeout != j_ptr->timeout)
	{
		/* Hack -- Fix stack count */
		if ((o_ptr->timeout) && !(o_ptr->stackc)) o_ptr->stackc = o_ptr->number;

		/* Hack -- Fix stack count */
		if ((j_ptr->timeout) && !(j_ptr->stackc)) o_ptr->stackc = j_ptr->number;

		/* Add stack count */
		o_ptr->stackc += j_ptr->stackc;

		/* Hack -- Fix stack count */
		if (o_ptr->stackc == number) o_ptr->stackc = 0;

		/* Hack -- Use Maximum timeout */
		if (o_ptr->timeout < j_ptr->timeout) o_ptr->timeout = j_ptr->timeout;
	}

	/* Add together the item counts */
	o_ptr->number = number;

	/* Hack -- Blend identify */
	o_ptr->ident |= (j_ptr->ident);

	/* Hack -- Blend "known" status */
	if (object_known_p(j_ptr)) object_known(o_ptr);

	/* Hack -- Blend "notes" */
	if (j_ptr->note != 0) o_ptr->note = j_ptr->note;

	/* Hack -- Blend flags */
	object_can_flags(o_ptr,j_ptr->can_flags1,j_ptr->can_flags2,j_ptr->can_flags3,j_ptr->can_flags4, floor);
	object_not_flags(o_ptr,j_ptr->not_flags1,j_ptr->not_flags2,j_ptr->not_flags3,j_ptr->not_flags4, floor);
	object_may_flags(o_ptr,j_ptr->may_flags1,j_ptr->may_flags2,j_ptr->may_flags3,j_ptr->may_flags4, floor);

	/* Mega-Hack -- Blend "discounts" */
	if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;

	/* Mega-Hack -- Blend "feelings" */
	if (!o_ptr->feeling) o_ptr->feeling = j_ptr->feeling;

	/* Mega Hack -- Blend "usages" */
	if (o_ptr->usage < j_ptr->usage) o_ptr->usage = j_ptr->usage;

	/* Blend origins */
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
 * Find the index of the object_kind with the given tval and sval
 */
s16b lookup_kind(int tval, int sval)
{
	int k;

	/* Look for it */
	for (k = 1; k < z_info->k_max; k++)
	{
		object_kind *k_ptr = &k_info[k];

		/* Found a match */
		if ((k_ptr->tval == tval) && (k_ptr->sval == sval)) return (k);
	}

	/* Since this function will be called at startup, msg_format is not yet usable. */
#if 0
	/* Oops */
	msg_format("No object (%d,%d)", tval, sval);
#endif

	/* Oops */
	return (0);
}


/*
 * Wipe an object clean.
 */
void object_wipe(object_type *o_ptr)
{
	/* Wipe the structure */
	(void)WIPE(o_ptr, object_type);
}


/*
 * Prepare an object based on an existing object
 */
void object_copy(object_type *o_ptr, const object_type *j_ptr)
{
	/* Copy the structure */
	COPY(o_ptr, j_ptr, object_type);
}


/*
 * Prepare an object based on an object kind.
 */
void object_prep(object_type *o_ptr, int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Clear the record */
	object_wipe(o_ptr);

	/* Save the kind index */
	o_ptr->k_idx = k_idx;

	/* Efficiency -- tval/sval */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* No more annoying objects with lots of flags, but 0 pval */
	o_ptr->pval = k_ptr->pval ? k_ptr->pval : 1;

	/* Default "charges" */
	o_ptr->charges = k_ptr->charges;

	/* Default number */
	o_ptr->number = 1;

	/* Default stack count */
	o_ptr->stackc = 0;

	/* Default weight */
	o_ptr->weight = k_ptr->weight;

	/* Default magic */
	o_ptr->to_h = k_ptr->to_h;
	o_ptr->to_d = k_ptr->to_d;
	o_ptr->to_a = k_ptr->to_a;

	/* Default power */
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Hack -- worthless items are always "broken" */
	/*if (k_ptr->cost <= 0) o_ptr->ident |= (IDENT_BROKEN);*/

	/* Hack -- cursed items are often "cursed" */
	if (k_ptr->flags3 & (TR3_HEAVY_CURSE | TR3_PERMA_CURSE))  o_ptr->ident |= (IDENT_CURSED);
	else if (k_ptr->flags3 & (TR3_LIGHT_CURSE))
	{
		/* Some 'easily noticeable' attributes always result in cursing an item.
		 * See do_cmd_wield(). */
		if (k_ptr->flags1 & (TR1_BLOWS | TR1_SHOTS | TR1_SPEED | TR1_STR |
				TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR))
		{
			o_ptr->ident |= (IDENT_CURSED);
		}
#ifndef ALLOW_OBJECT_INFO_MORE
		else if ((k_ptr->flags1 & (TR1_INFRA)) || (k_ptr->flags3 & (TR3_LITE | TR3_TELEPATHY | TR3_SEE_INVIS)))
		{
			o_ptr->ident |= (IDENT_CURSED);
		}
#endif
		/* Generate cursed items deeper in the dungeon */
		else if (rand_int(75) < p_ptr->depth) o_ptr->ident |= (IDENT_CURSED);
		/* Otherwise item is easily removable */
		else o_ptr->ident |= (IDENT_BROKEN);
	}
	else if (k_ptr->flags3 & (TR3_UNCONTROLLED))
	  /* always start cursed so that uncontrolled effects can occur */
	  o_ptr->ident |= (IDENT_CURSED);

	o_ptr->can_flags1 = 0x0L;
	o_ptr->can_flags2 = 0x0L;
	o_ptr->can_flags3 = 0x0L;

	o_ptr->may_flags1 = 0x0L;
	o_ptr->may_flags2 = 0x0L;
	o_ptr->may_flags3 = 0x0L;

	o_ptr->not_flags1 = 0x0L;
	o_ptr->not_flags2 = 0x0L;
	o_ptr->not_flags3 = 0x0L;
}


/*
 * Help determine an "enchantment bonus" for an object.
 *
 * To avoid floating point but still provide a smooth distribution of bonuses,
 * we simply round the results of division in such a way as to "average" the
 * correct floating point value.
 *
 * This function has been changed.  It uses "Rand_normal()" to choose values
 * from a normal distribution, whose mean moves from zero towards the max as
 * the level increases, and whose standard deviation is equal to 1/4 of the
 * max, and whose values are forced to lie between zero and the max, inclusive.
 *
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very
 * rare to get the "full" enchantment on an object, even a deep levels.
 *
 * It is always possible (albeit unlikely) to get the "full" enchantment.
 *
 * A sample distribution of values from "m_bonus(10, N)" is shown below:
 *
 *   N       0     1     2     3     4     5     6     7     8     9    10
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07
 */
static s16b m_bonus(int max, int level)
{
	int bonus, stand, extra, value;


	/* Paranoia -- enforce maximal "level" */
	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;


	/* The "bonus" moves towards the max */
	bonus = ((max * level) / MAX_DEPTH);

	/* Hack -- determine fraction of error */
	extra = ((max * level) % MAX_DEPTH);

	/* Hack -- simulate floating point computations */
	if (rand_int(MAX_DEPTH) < extra) bonus++;


	/* The "stand" is equal to one quarter of the max */
	stand = (max / 4);

	/* Hack -- determine fraction of error */
	extra = (max % 4);

	/* Hack -- simulate floating point computations */
	if (rand_int(4) < extra) stand++;


	/* Choose an "interesting" value */
	value = Rand_normal(bonus, stand);

	/* Enforce the minimum value */
	if (value < 0) return (0);

	/* Enforce the maximum value */
	if (value > max) return (max);

	/* Result */
	return (value);
}




/*
 * Cheat -- describe a created object for the user
 */
static void object_mention(object_type *o_ptr)
{
	char o_name[80];

	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

	/* Artifact */
	if (artifact_p(o_ptr))
	{
		/* Silly message */
		if (o_ptr->name1 < ART_MIN_NORMAL)
			msg_format("Specialart (%s)", o_name);
		else if (o_ptr->name1 < z_info->a_max_standard)
			msg_format("Artifact (%s)", o_name);
		else
			msg_format("Randart (%s)", o_name);
	}

	/* Ego-item */
	else if (ego_item_p(o_ptr))
	{
		/* Silly message */
		msg_format("Ego-item (%s)", o_name);
	}

	/* Normal item */
	else
	{
		/* Silly message */
		msg_format("Object (%s)", o_name);
	}
}

/*
 *  Try to boost an item up to the appropriate power for the level.
 *
 *  We increase to_hit, to_dam, to_ac, pval, damage dice and/or sides
 */
static void boost_item(object_type *o_ptr, int lev, int power)
{
	int boost_power, old_boost_power;
	int sign = (power >= 0 ? 1 : -1);
	bool tryagain = TRUE;
	bool supercharge = FALSE;
	int tries = 0;
	int choice = 0;

	/* Paranoia */
	if (!power) return;

	/* Nothing to boost */
	if (!(o_ptr->to_h) && !(o_ptr->to_d) && !(o_ptr->to_a) && !(o_ptr->pval)) return;

	/* Evaluate power */
	boost_power = object_power(o_ptr);
	/* Reverse sign so that the same code applied to cursed items */
	if (power < 0) boost_power = -boost_power;

	/* Too powerful already */
	if (boost_power >= lev) return;

	/* Prefer to supercharge up 1 ability */
	if (rand_int(5)) supercharge = TRUE;

	do
	{
		old_boost_power = boost_power;

		if ((tryagain) || !(supercharge)) choice = rand_int(12);

		tryagain = FALSE;
		tries++;

		switch(choice)
		{
			case 0: case 1: case 2:

				/* Increase to_h */
				if ((o_ptr->to_h) && (sign > 0 ? o_ptr->to_h > 0 : o_ptr->to_h < 0)) o_ptr->to_h += sign;
				else tryagain = TRUE;

				break;

			case 3: case 4: case 5:

				/* Increase to_d; not above weapon dice */
				if (((o_ptr->to_d) && (sign > 0 ? o_ptr->to_d > 0 : o_ptr->to_d < 0))
				    && o_ptr->to_d + sign < (o_ptr->tval == TV_BOW ? 15 : o_ptr->dd * o_ptr->ds + 5))
				  o_ptr->to_d += sign;
				else tryagain = TRUE;

				break;

			case 6: case 7: case 8:

				/* Increase to_a */
				if (((o_ptr->to_a) && (sign > 0 ? o_ptr->to_a > 0 : o_ptr->to_a < 0))
				    && ((o_ptr->to_a + sign < o_ptr->ac + 5) || o_ptr->tval == TV_CROWN))
				  o_ptr->to_a += sign;
				else tryagain = TRUE;

				break;

			case 9:

			  /* Is this an ego item? */
			  if (o_ptr->name2)
				{
				ego_item_type *e_ptr = &e_info[o_ptr->name2];

				/* Increase pval; only rarely if SPEED */
				if (o_ptr->pval
					&& (!(e_ptr->flags1 & (TR1_SPEED))
					    || rand_int(100) < 33))
				  o_ptr->pval += sign;
				else tryagain = TRUE;
				}
			  else
				{
				/* Increase pval */
				if ((o_ptr->pval) && (sign > 0 ? o_ptr->pval > 0 : o_ptr->pval < 0))
				  o_ptr->pval += sign;
				else tryagain = TRUE;
				}
				break;

			case 10:

				/* Increase damage dice */
				if ((power > 0) && ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_HAFTED)
				|| (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_ARROW)
				|| (o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_STAFF)))
					 o_ptr->dd++;
				else tryagain = TRUE;

				break;

			case 11:

				/* Increase damage dice */
				if ((power > 0) && ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_HAFTED)
				|| (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_ARROW)
				|| (o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_STAFF)))
					 o_ptr->ds++;
				else tryagain = TRUE;

				break;
		}

		/* Evaluate power */
		boost_power = object_power(o_ptr);
		if (power < 0) boost_power = -boost_power;

		/* Hack -- boost to-hit, to-dam, to-ac to 10 if required to increase power */
		if ((boost_power <= old_boost_power) && !(tryagain) && (choice < 10))
		{
			switch(choice)
			{
				case 0: case 1: case 2:
				{
					if ((sign > 0) && (o_ptr->to_h > 0) && (o_ptr->to_h < 12)) o_ptr->to_h = 12;
					else if ((sign < 0) && (o_ptr->to_h < 0) && (o_ptr->to_h > -12)) o_ptr->to_h = -12;
					break;
				}
				case 3: case 4: case 5:
				{
					if ((sign > 0) && (o_ptr->to_d > 0) && (o_ptr->to_d < 10)) o_ptr->to_d = MIN(o_ptr->dd * o_ptr->ds + 5, 10);
					else if ((sign < 0) && (o_ptr->to_d < 0) && (o_ptr->to_d > -10)) o_ptr->to_d = -10;
					break;
				}
				case 6: case 7: case 8:
				{
					if ((sign > 0) && (o_ptr->to_a > 0) && (o_ptr->to_a < 10)) o_ptr->to_a = MIN(o_ptr->ac + 5, 10);
					else if ((sign < 0) && (o_ptr->to_a < 0) && (o_ptr->to_a > -10)) o_ptr->to_a = -10;
					break;
				}
			}

			boost_power = object_power(o_ptr);
			if (power < 0) boost_power = -boost_power;
		}
	} while ((!(boost_power <= old_boost_power) && (boost_power < lev)) || (tryagain && (tries < 40)));

	/* Hack -- undo the change that took us over the maximum power */
	if (!tryagain)
	{
		switch(choice)
		{
			case 0: case 1: case 2:o_ptr->to_h -= sign; break;
			case 3: case 4: case 5: o_ptr->to_d -= sign; break;
			case 6: case 7: case 8: o_ptr->to_a -= sign; break;
			case 9: o_ptr->pval -= sign; break;
			case 10: o_ptr->dd--; break;
			case 11: o_ptr->ds--; break;
		}
	}
}


/*
 * Attempt to change an object into an magic-item
 * This function picks a magic item that has 90% to 100% of the power supplied to this
 * function, in a manner similar to random artifacts.
 */
static bool make_magic_item(object_type *o_ptr, int lev, int power)
{
	int i;
	u32b j;

	int count = 0;

	int x1 = 0;
	int x2 = 0;

	int obj_pow1;
	int obj_pow2 = 0;

	int max_pval = 0;

	bool great = ABS(power) > 1;

	/* Fail if object already is artifact */
	if (o_ptr->name1) return (FALSE);

	/* Hack -- some egos can have any magic item flag */
	if ((o_ptr->name2) && (o_ptr->xtra1 < 16)) return (FALSE);

	/* Sanity check */
	if (!power) return (FALSE);

	/* Hack -- clear ego extra flag */
	if (o_ptr->name2) o_ptr->xtra1 = 0;

	/* Boost level for great drops */
	if (great) lev += 10;

	/* Recompute power */
	obj_pow1 = object_power(o_ptr);
	/* Reverse sign so that the same code applied to cursed items */
	if (power < 0) obj_pow1 = -obj_pow1;

	/* Already too magical */
	if (obj_pow1 >= lev) return (FALSE);

	/* Boost items most of the time -- except instruments and lites */
	if ((o_ptr->tval != TV_LITE) && (o_ptr->tval != TV_INSTRUMENT) && rand_int(3))
	{
		/* Boost the item */
		boost_item(o_ptr, lev, power);

		/* Recompute power */
		obj_pow1 = object_power(o_ptr);
		if (power < 0) obj_pow1 = -obj_pow1;

		/* Already too magical */
		if (obj_pow1 >= lev) return (FALSE);

		/* Magical enough */
		if (rand_int(2)) return (TRUE);
	}

	/* Hack -- Done boosting racial items */
	if (!(o_ptr->name2) && (o_ptr->xtra1)) return (TRUE);

	/* Iterate through flags 1 */
	for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
	{
		o_ptr->xtra1 = 16;
		o_ptr->xtra2 = i;

		/* Skip non-weapons -- we have to do this because brands grant ignore flags XXX */
		if ((j >= TR1_BRAND_ACID) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_HAFTED)
			&& (o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_ARROW)
			&& (o_ptr->tval != TV_SHOT) && (o_ptr->tval != TV_BOLT) && (o_ptr->tval != TV_STAFF)) continue;

		/* Hack -- don't allow tunnelling on missile weapons -- people seem to object to this somehow */
		if ((j == TR1_TUNNEL) && (o_ptr->tval == TV_BOW)) continue;

		/* Evaluate power */
		obj_pow2 = object_power(o_ptr);
		if (power < 0) obj_pow2 = -obj_pow2;

		/* Pick this flag with default pval? */
		if ((obj_pow2 > obj_pow1) &&			/* Flag has any effect? */
			((!great) || (obj_pow2 >= ((lev * 18) / 20))) && 	/* Great forces at least 90% */
			obj_pow2 <= lev &&			/* No more than 100% */
			!rand_int(++count))		/* Sometimes pick */
		{
			x1 = 16;
			x2 = i;
			max_pval = 0;
		}

		/* Try increasing pval in addition to the flag */
		else
		{
			int old_pval = o_ptr->pval;
			int old_pow2;

			do
			{
				old_pow2 = obj_pow2;

				if (power > 0)
				{
					/* Increase pval */
					o_ptr->pval++;

					/* Evaluate power */
					obj_pow2 = object_power(o_ptr);
				}
				else
				{
					/* Decrease pval */
					o_ptr->pval--;

					/* Evaluate power - sign reversed */
					obj_pow2 = -object_power(o_ptr);
				}

			} while (obj_pow2 > old_pow2 && obj_pow2 < lev * 19 / 20);

			/* Can find valid pval? */
			if (obj_pow2 > obj_pow1 && obj_pow2 < lev && !rand_int(++count))
			{
				x1 = 16;
				x2 = i;
				if (o_ptr->pval > 0)
					max_pval = o_ptr->pval - old_pval;
				else
					max_pval = o_ptr->pval + old_pval;
			}

			/* Reset pval */
			o_ptr->pval = old_pval;
		}
	}


	/* Iterate through flags 2 */
	for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
	{
		o_ptr->xtra1 = 17;
		o_ptr->xtra2 = i;

		/* Skip non-wearable items -- we have to do this because resistances and immunities grant ignore flags XXX */
		if ((j >= TR2_IM_ACID) && ((o_ptr->tval == TV_ARROW)
			|| (o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_BOLT))) continue;

		/* Hack -- play balance. Only allow immunities on armour, shields and cloaks XXX */
		if ((j >= TR2_IM_ACID) && (j <= TR2_IM_COLD) && (o_ptr->tval != TV_HARD_ARMOR)
			&& (o_ptr->tval != TV_SOFT_ARMOR) && (o_ptr->tval != TV_DRAG_ARMOR)
			&& (o_ptr->tval != TV_SHIELD) && (o_ptr->tval != TV_CLOAK)) continue;

		/* Evaluate power */
		obj_pow2 = object_power(o_ptr);
		if (power < 0) obj_pow2 = -obj_pow2;

		/* Pick this flag? */
		if ((obj_pow2 > obj_pow1) &&			/* Flag has any effect ? */
			((!great) || (obj_pow2 >= ((lev * 18) / 20))) && 	/* Great forces at least 90% */
			obj_pow2 <= lev &&			/* No more than 100% */
			!rand_int(++count))		/* Sometimes pick */
		{
			x1 = 17;
			x2 = i;
			max_pval = 0;
		}
	}

	/* Iterate through flags 3 */
	for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
	{
		o_ptr->xtra1 = 18;
		o_ptr->xtra2 = i;

		/* Evaluate power */
		obj_pow2 = object_power(o_ptr);
		if (power < 0) obj_pow2 = -obj_pow2;

		/* Pick this flag? */
		if ((obj_pow2 > obj_pow1) &&			/* Flag has any effect ? */
			((!great) || (obj_pow2 >= ((lev * 18) / 20))) && 	/* Great forces at least 90% */
			obj_pow2 <= lev &&			/* No more than 100% */
			!rand_int(++count))		/* Sometimes pick */
		{
			x1 = 18;
			x2 = i;
			max_pval = 0;
		}
	}

	/* Iterate through flags 4 */
	for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
	{
		o_ptr->xtra1 = 19;
		o_ptr->xtra2 = i;

		/* Skip non-weapons -- we have to do this because of the secondary lite flag on brand lite XXX */
		if ((j == TR4_BRAND_LITE) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_HAFTED)
			&& (o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_ARROW)
			&& (o_ptr->tval != TV_SHOT) && (o_ptr->tval != TV_BOLT) && (o_ptr->tval != TV_STAFF)) continue;

		/* Hack -- play balance. Only allow immunities on armour, shields and cloaks XXX */
		if ((j == TR4_IM_POIS) && (o_ptr->tval != TV_HARD_ARMOR) && (o_ptr->tval != TV_SOFT_ARMOR)
			&& (o_ptr->tval != TV_DRAG_ARMOR) && (o_ptr->tval != TV_SHIELD) && (o_ptr->tval != TV_CLOAK)) continue;

		/* Evaluate power */
		obj_pow2 = object_power(o_ptr);
		if (power < 0) obj_pow2 = -obj_pow2;

		/* Pick this flag? */
		if ((obj_pow2 > obj_pow1) &&			/* Flag has any effect ? */
			((!great) || (obj_pow2 >= ((lev * 18) / 20))) && 	/* Great forces at least 90% */
			obj_pow2 <= lev &&			/* No more than 100% */
			!rand_int(++count))		/* Sometimes pick */
		{
			x1 = 19;
			x2 = i;
			max_pval = 0;
		}
	}

	/* Valid choice */
	if (x1)
	{
		o_ptr->xtra1 = x1;
		o_ptr->xtra2 = x2;

		/* Set new pval */
		if (max_pval)
		{
			if (max_pval > 0)
				o_ptr->pval += great ? (s16b)max_pval : (s16b)rand_range(1, max_pval);
			else
				o_ptr->pval += great ? (s16b)max_pval : -(s16b)rand_range(1, -max_pval);
		}

		return(TRUE);
	}
	else
	{
		o_ptr->xtra1 = 0;
		o_ptr->xtra2 = 0;

		return(FALSE);
	}
}


/*
 * Attempt to change an object into an ego-item -MWK-
 * Better only called by apply_magic()
 * The return value is currently unused, but a wizard might be interested in it.
 */
static bool make_ego_item(object_type *o_ptr, bool cursed, bool great)
{
	int i, j, k, level;

	int e_idx;

	long value, total;

	ego_item_type *e_ptr;

	alloc_entry *table = alloc_ego_table;

	/* Fail if object already is ego or artifact */
	if (o_ptr->name1) return (FALSE);
	if (o_ptr->name2) return (FALSE);

	level = p_ptr->depth;

	/* Boost level if great */
	if (great) level += 10;

	/* Boost level (like with object base types) */
	if (level > 0)
	{
		/* Occasional "boost" */
		if (rand_int(GREAT_EGO) == 0)
		{
			/* The bizarre calculation again */
			level = 1 + (level * MAX_DEPTH / randint(MAX_DEPTH));
		}
	}

	/*
	 * Hack - *Cursed* items have a totally different level distribution
	 * This is needed to obtain the old Weapons of Morgul distribution.
	 */
	if (cursed)
	{
		/* Probability goes linear with level */
		level = p_ptr->depth + rand_int(127);
	}

	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_ego_size; i++)
	{
		/* Default */
		table[i].prob3 = 0;

		/* Objects are sorted by depth */
		if (table[i].level > level) continue;

		/* Get the index */
		e_idx = table[i].index;

		/* Get the actual kind */
		e_ptr = &e_info[e_idx];

		/* Test if this is a possible ego-item for value of (cursed) */
		if (!cursed && (e_ptr->flags3 & TR3_LIGHT_CURSE)) continue;
		if (cursed && !(e_ptr->flags3 & TR3_LIGHT_CURSE)) continue;

		/* Hack -- filter on xtra flag if set */
		if (o_ptr->xtra1)
		{
			if ((object_xtra_what[o_ptr->xtra1] == 1)
				&& ((e_ptr->flags1 & (object_xtra_base[o_ptr->xtra1] << o_ptr->xtra2)) == 0)) continue;
			else if ((object_xtra_what[o_ptr->xtra1] == 2)
				&& ((e_ptr->flags2 & (object_xtra_base[o_ptr->xtra1] << o_ptr->xtra2)) == 0)) continue;
			else if ((object_xtra_what[o_ptr->xtra1] == 3)
				&& ((e_ptr->flags3 & (object_xtra_base[o_ptr->xtra1] << o_ptr->xtra2)) == 0)) continue;
			else if ((object_xtra_what[o_ptr->xtra1] == 4)
				&& ((e_ptr->flags4 & (object_xtra_base[o_ptr->xtra1] << o_ptr->xtra2)) == 0)) continue;
		}

		/* Otherwise skip racial equipment */
		else if (!(e_ptr->slot))
		{
			continue;
		}

		/* Fake ego power */
		o_ptr->name2 = e_idx;
		j = object_power(o_ptr);
		if (cursed) j = -j;

		/* Force better for non-weapons as we can't modify power as easily */
		if ((great) && (j < level / 2) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_HAFTED)
			&& (o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_ARROW)
			&& (o_ptr->tval != TV_SHOT) && (o_ptr->tval != TV_BOLT) && (o_ptr->tval != TV_STAFF)) continue;

		/* Force better if we can't modify power */
		else if ((great) && !(e_ptr->xtra) && (j < level * 3 / 4)) continue;

		/* Force at least useful */
		else if (j < level / 3) continue;

		/* Test if permitted ego power */
		if (j > level) continue;

		/* Test if this is a legal ego-item type for this object */
		if (e_ptr->slot)
			for (k = 0; k < 3; k++)
		{
			/* Require identical base type */
			if (o_ptr->tval == e_ptr->tval[k])
			{
				/* Require sval in bounds, lower */
				if (o_ptr->sval >= e_ptr->min_sval[k])
				{
					/* Require sval in bounds, upper */
					if (o_ptr->sval <= e_ptr->max_sval[k])
					{

						/* Accept */
						table[i].prob3 = table[i].prob2;
					}
				}
			}
		}

		/* Total */
		total += table[i].prob3;
	}

	/* Remove fake ego power */
	o_ptr->name2 = 0;

	/* No legal ego-items */
	if (total <= 0) return (FALSE);

	/* Pick an ego-item */
	value = rand_int(total);

	/* Find the object */
	for (i = 0; i < alloc_ego_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}

	/* We have one */
	o_ptr->name2 = (byte)table[i].index;

	/* Auto-inscribe if necessary */
	if (adult_auto) o_ptr->note = e_info[o_ptr->name2].note;

	/* Apply obvious flags */
	object_obvious_flags(o_ptr, TRUE);

	return (TRUE);
}


/*
 * Mega-Hack -- Attempt to create one of the "Special Objects".
 *
 * We are only called from "make_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()".
 *
 * We *prefer* to create the special artifacts in order, but this is
 * normally outweighed by the "rarity" rolls for those artifacts.  The
 * only major effect of this logic is that the Phial (with rarity one)
 * is always the first special artifact created.
 */
static bool make_artifact_special(object_type *o_ptr)
{
	int i;

	int k_idx;


	/* No artifacts, do nothing */
	if (adult_no_artifacts) return (FALSE);

	/* No artifacts in the basic town */
	if (!p_ptr->depth) return (FALSE);

	/* Check the special artifacts */
	for (i = 0; i < ART_MIN_NORMAL; ++i)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		/* Enforce minimum "depth" (loosely) */
		if (a_ptr->level > p_ptr->depth)
		{
			/* Get the "out-of-depth factor" */
			int d = (a_ptr->level - p_ptr->depth) * 2;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* Artifact "rarity roll" */
		if (rand_int(a_ptr->rarity) != 0) continue;

		/* Find the base object */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Enforce minimum "object" level (loosely) */
		if (k_info[k_idx].level > object_level)
		{
			/* Get the "out-of-depth factor" */
			int d = (k_info[k_idx].level - object_level) * 5;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* Assign the template */
		object_prep(o_ptr, k_idx);

		/* Mark the item as an artifact */
		o_ptr->name1 = i;

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Attempt to change an object into an artifact
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(object_type *o_ptr)
{
	int i;


	/* No artifacts, do nothing */
	if (adult_no_artifacts) return (FALSE);

	/* No artifacts in the basic town */
	if (!p_ptr->depth) return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return (FALSE);

	/* Check the artifact list (skip the "specials") */
	for (i = ART_MIN_NORMAL; i < z_info->a_max; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" items */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		/* Must have the correct fields */
		if (a_ptr->tval != o_ptr->tval) continue;
		if (a_ptr->sval != o_ptr->sval) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > p_ptr->depth)
		{
			/* Get the "out-of-depth factor" */
			int d = (a_ptr->level - p_ptr->depth) * 2;

			/* Roll for out-of-depth creation */
			if (rand_int(d) != 0) continue;
		}

		/* XXX Enfore acceptable "power" */
		if ((a_ptr->power < p_ptr->depth) || (a_ptr->power > p_ptr->depth * 2))
		{
			continue;
		}

		/* We must make the "rarity roll" */
		if (rand_int(a_ptr->rarity) != 0) continue;

		/* Mark the item as an artifact */
		o_ptr->name1 = i;

		/* Clear xtra flags */
		o_ptr->xtra1 = 0;
		o_ptr->xtra2 = 0;

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Charge a new wand/staff.
 */
static void charge_item(object_type *o_ptr)
{
	if (k_info[o_ptr->k_idx].level < 2)
	{
		o_ptr->charges = (s16b)randint(15)+8;
	}
	else if (k_info[o_ptr->k_idx].level < 4)
	{
		o_ptr->charges = (s16b)randint(10)+6;
	}
	else if (k_info[o_ptr->k_idx].level < 40)
	{
		o_ptr->charges = (s16b)randint(8)+6;
	}
	else if (k_info[o_ptr->k_idx].level < 50)
	{
		o_ptr->charges = (s16b)randint(6)+2;
	}
	else if (k_info[o_ptr->k_idx].level < 60)
	{
		o_ptr->charges = (s16b)randint(4)+2;
	}
	else if (k_info[o_ptr->k_idx].level < 70)
	{
		o_ptr->charges = (s16b)randint(3)+1;
	}
	else
	{
		o_ptr->charges = (s16b)randint(2)+1;
	}

}



/*
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 */
static void a_m_aux_1(object_type *o_ptr, int level, int power)
{
	int tohit1 = randint(5) + m_bonus(5, level);
	int todam1 = randint(5) + m_bonus(5, level);

	int tohit2 = m_bonus(10, level);
	int todam2 = m_bonus(10, level);


	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_h += tohit1;
		o_ptr->to_d = MIN(o_ptr->to_d + todam1,
				  (o_ptr->tval == TV_BOW ? 15 : o_ptr->dd * o_ptr->ds + 5));

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_h += tohit2;
			o_ptr->to_d = MIN(o_ptr->to_d + todam2,
					  (o_ptr->tval == TV_BOW ? 15 : o_ptr->dd * o_ptr->ds + 5));
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_h -= tohit2;
			o_ptr->to_d -= todam2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_h + o_ptr->to_d < 0)
		{
			if (rand_int(100) < 30) o_ptr->ident |= (IDENT_CURSED);
			else o_ptr->ident |= (IDENT_BROKEN);
		}
	}


	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		{

			/* Very bad */
			if (power < -1)
			{
				/* Hack -- Horrible digging bonus */
				o_ptr->pval = 0 - (5 + (s16b)randint(5));
			}

			/* Bad */
			else if (power < 0)
			{
				/* Hack -- Reverse digging bonus */
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}


		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_STAFF:
		{
			/* Very Good */
			if (power > 1)
			{
				/* Hack -- Super-charge the damage dice */
				while ((o_ptr->dd * o_ptr->ds > 0) &&
				       (rand_int(15) == 0))
				{
					o_ptr->dd++;
				}

				/* Hack -- Limit the damage dice to max of 9*/
				if (o_ptr->dd > 9) o_ptr->dd = 9;

				/* Hack -- Super-charge the damage sides */
				while ((o_ptr->dd * o_ptr->ds > 0) &&
				       (rand_int(15) == 0))
				{
					o_ptr->ds++;
				}

				/* Hack -- Limit the damage dice to max of 9*/
				if (o_ptr->ds > 9) o_ptr->ds = 9;
			}

			break;
		}


		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			/* Very good */
			if (power > 1)
			{
				/* Hack -- super-charge the damage side */
				while ((o_ptr->dd * o_ptr->ds > 0) &&
				       (rand_int(25) == 0))
				{
					o_ptr->ds++;
				}

				/* Hack -- restrict the damage side */
				if (o_ptr->ds > 9) o_ptr->ds = 9;
			}

			break;
		}
	}
}


/*
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type *o_ptr, int level, int power)
{
	int toac1 = randint(5) + m_bonus(5, level);

	int toac2 = m_bonus(10, level);


	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_a = MIN(o_ptr->tval == TV_CROWN ? 30 : o_ptr->ac + toac1, o_ptr->to_a + 5 + toac1);

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_a = MIN(o_ptr->tval == TV_CROWN ? 30 : o_ptr->ac + toac2, o_ptr->to_a + 5 + toac2);
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_a -= toac1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_a -= toac2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_a < 0)
		{
			if (rand_int(100) < 30) o_ptr->ident |= (IDENT_CURSED);
			else o_ptr->ident |= (IDENT_BROKEN);
		}
	}


	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		{
			/* Mention the item */
			if (cheat_peek) object_mention(o_ptr);

			break;
		}
	}
}



/*
 * Apply magic to an item known to be a "ring" or "amulet"
 *
 * Hack -- note special "pval boost" code for ring of speed
 * Hack -- note that some items must be cursed (or blessed)
 */
static void a_m_aux_3(object_type *o_ptr, int level, int power)
{
	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_RING:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				/* Strength, Constitution, Dexterity, Intelligence */
				case SV_RING_STR:
				case SV_RING_CON:
				case SV_RING_DEX:
				case SV_RING_INT:
				{
				  /* Penalize double stat rings */
				  int penalty = 2 * (o_ptr->sval == SV_RING_DEX)
				    + 3 * (o_ptr->sval == SV_RING_STR);

					/* Stat bonus */
					o_ptr->pval = 1 + m_bonus(5 - penalty, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Ring of Speed! */
				case SV_RING_SPEED:
				{
					/* Base speed (1 to 8) */
					o_ptr->pval = (s16b)randint(3) + m_bonus(5, level);

					/* Super-charge the ring */
					while (p_ptr->depth > 25 + rand_int (20)
							 && rand_int(100) < 33)
						o_ptr->pval++;

					/* Cursed Ring */
					if (power < 0)
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);

						break;
					}

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}

				/* Searching */
				case SV_RING_SEARCHING:
				{
					/* Bonus to searching */
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}
#if 0
				/* Flames, Acid, Ice */
				case SV_RING_FLAMES:
				case SV_RING_ACID:
				case SV_RING_ICE:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint(5) + m_bonus(10, level);
					break;
				}
#endif
				/* Weakness, Stupidity */
				case SV_RING_WEAKNESS:
				case SV_RING_STUPIDITY:
				{
					/* Penalize */
					o_ptr->pval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* WOE, Stupidity */
				case SV_RING_WOE:
				{
					/* Penalize */
					o_ptr->to_a = 0 - (5 + m_bonus(10, level));
					o_ptr->pval = 0 - (1 + m_bonus(5, level));

					break;
				}

				/* Ring of damage */
				/* TODO: reduce bonus here and elsewhere now that to_d is bound by weapon dice */
				case SV_RING_DAMAGE:
				{
					/* Bonus to damage */
					o_ptr->to_d = 5 + (s16b)randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse bonus */
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}

				/* Ring of Accuracy */
				case SV_RING_ACCURACY:
				{
					/* Bonus to hit */
					o_ptr->to_h = 5 + (s16b)randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse tohit */
						o_ptr->to_h = 0 - (o_ptr->to_h);
					}

					break;
				}

				/* Ring of Protection */
				case SV_RING_PROTECTION:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + (s16b)randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse toac */
						o_ptr->to_a = 0 - (o_ptr->to_a);
					}

					break;
				}

				/* Ring of Slaying */
				case SV_RING_SLAYING:
				{
					/* Bonus to damage and to hit */
					o_ptr->to_d = (s16b)randint(5) + m_bonus(10, level);
					o_ptr->to_h = (s16b)randint(5) + m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse bonuses */
						o_ptr->to_h = 0 - (o_ptr->to_h);
						o_ptr->to_d = 0 - (o_ptr->to_d);
					}

					break;
				}
			}

			break;
		}

		case TV_AMULET:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				/* Amulet of wisdom/charisma */
				case SV_AMULET_WISDOM:
				case SV_AMULET_CHARISMA:
				case SV_AMULET_INFRAVISION:
				{
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Amulet of searching */
				case SV_AMULET_SEARCHING:
				{
					o_ptr->pval = (s16b)randint(5) + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Regeneration */
				case SV_AMULET_REGEN:
				{
					/* Bonus to regeneration */
					o_ptr->pval = 1 + m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Amulet of ESP -- never cursed */
				case SV_AMULET_ESP:
				{
					o_ptr->pval = (s16b)randint(5) + m_bonus(5, level);

					break;
				}

				/* Amulet of the Magi -- never cursed */
				case SV_AMULET_THE_MAGI:
				{
					o_ptr->pval = 1 + m_bonus(3, level);
					o_ptr->to_a = (s16b)randint(5) + m_bonus(5, level);

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}

				/* Amulet of Devotion -- never cursed */
				case SV_AMULET_DEVOTION:
				{
					o_ptr->pval = 1 + m_bonus(3, level);

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}

				/* Amulet of Weaponmastery -- never cursed */
				case SV_AMULET_WEAPONMASTERY:
				{
					o_ptr->to_h = 1 + m_bonus(4, level);
					o_ptr->to_d = 1 + m_bonus(4, level);
					o_ptr->pval = 1 + m_bonus(1, level);

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}

				/* Amulet of Trickery -- never cursed below DL50 */
				case SV_AMULET_TRICKERY:
				{
					o_ptr->pval = (s16b)randint(1) + m_bonus(3, level);

					/* Cursed */
					if (p_ptr->depth < 20 + rand_int (30))
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);

						break;
					}

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}

				/* Amulet of Doom -- always cursed */
				case SV_AMULET_DOOM:
				{
					/* Penalize */
					o_ptr->pval = 0 - ((s16b)randint(5) + m_bonus(5, level));
					o_ptr->to_a = 0 - ((s16b)randint(5) + m_bonus(5, level));

					break;
				}

				/* Amulet of the Serpents --- no speed, so bigger pval */
				case SV_AMULET_SERPENTS:
				{
					o_ptr->pval = 1 + m_bonus(4, level);

					/* Cursed */
					if (power < 0)
					{
						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);

						break;
					}

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

					break;
				}
			}

			break;
		}
	}

	/* Check if has negatives */
	if ((o_ptr->to_h < 0) || (o_ptr->to_d < 0) || (o_ptr->to_a < 0) || (o_ptr->pval < 0))
	{
		if ((power < -1) || (rand_int(100) < 30))
		{
			/* Cursed */
			o_ptr->ident |= (IDENT_CURSED);
		}
		else if (power < 0)
		{
			/* Some 'easily noticeable' attributes always result in cursing an item.
			 * See do_cmd_wield(). */
			if (k_info[o_ptr->k_idx].flags1 & (TR1_BLOWS | TR1_SHOTS | TR1_SPEED | TR1_STR |
					TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR))
			{
				o_ptr->ident |= (IDENT_CURSED);
			}
		#ifndef ALLOW_OBJECT_INFO_MORE
			else if ((k_info[o_ptr->k_idx].flags1 & (TR1_INFRA)) || (k_ptr->flags3 & (TR3_LITE | TR3_TELEPATHY | TR3_SEE_INVIS)))
			{
				o_ptr->ident |= (IDENT_CURSED);
			}
		#endif
			else
			{
				o_ptr->ident |= (IDENT_BROKEN);
			}
		}
	}
}


/*
 * Apply magic to an item known to be "boring"
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power)
{

	/* Prevent compiler warning */
	(void)level;
	(void)power;

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_LITE:
		{
			/* Hack -- Torches -- random fuel */
			if (o_ptr->sval == SV_LITE_TORCH)
			{
				if (o_ptr->charges > 0) o_ptr->charges = (s16b)randint(o_ptr->charges);
			}

			/* Hack -- Lanterns -- random fuel */
			if (o_ptr->sval == SV_LITE_LANTERN)
			{
				if (o_ptr->charges > 0) o_ptr->charges = (s16b)randint(o_ptr->charges);
			}

			break;
		}

		case TV_WAND:
		case TV_STAFF:
		{
			/* Hack -- charge wands */
			charge_item(o_ptr);

			break;
		}

		case TV_EGG:
		{
			/* Hack -- eggs/spores will hatch */
			if (o_ptr->name3 > 0) o_ptr->timeout = o_ptr->weight * damroll(2,6) * 10;
		}
	}
}


/*
 * Test player and item for a racial flag and update knowledge
 */
static void value_check_race_flag(object_type *o_ptr, u32b object_flag, u32b flag, bool floor)
{
	/* Sense race flag */
	if (object_flag & (flag))
	{
		if (p_ptr->cur_flags4 & (flag)) o_ptr->ident |= (IDENT_NAME);

		object_can_flags(o_ptr, 0x0L, 0x0L, 0x0L, flag, floor);
	}
	else
	{
		object_not_flags(o_ptr, 0x0L, 0x0L, 0x0L, flag, floor);
	}
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 0 (Racial).
 */
int value_check_aux0(object_type *o_ptr, bool floor)
{
	u32b f1, f2, f3, f4;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	value_check_race_flag(o_ptr, f4, TR4_UNDEAD, floor);
	value_check_race_flag(o_ptr, f4, TR4_ORC, floor);
	value_check_race_flag(o_ptr, f4, TR4_TROLL, floor);
	value_check_race_flag(o_ptr, f4, TR4_GIANT, floor);
	value_check_race_flag(o_ptr, f4, TR4_MAN, floor);
	value_check_race_flag(o_ptr, f4, TR4_DWARF, floor);
	value_check_race_flag(o_ptr, f4, TR4_ELF, floor);

	/* No feeling */
	return (0);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 6 (Rune magic).
 */
int value_check_aux6(object_type *o_ptr)
{
	/* Hack -- mark object if it has no runes */
	if (!(o_ptr->name1) && !(o_ptr->name2) && !(o_ptr->xtra1)
		&& !(k_info[o_ptr->k_idx].runest)) return (INSCRIP_UNRUNED);

	o_ptr->ident |= (IDENT_RUNES);

	/* Learn the rune recipe if the object is aware */
	if (object_aware_p(o_ptr))
	{
		k_info[o_ptr->k_idx].aware |= (AWARE_RUNES);
	}
	/* Otherwise associate flavor with the object */
	else if (k_info[o_ptr->k_idx].flavor)
	{
		int k;

		k_info[o_ptr->k_idx].aware |= (AWARE_RUNEX);

	    /* Mark all such objects sensed */
	    /* Check world */
	    for (k = 0; k < o_max; k++)
	    {
			if (o_list[k].k_idx == o_ptr->k_idx)
			{
				o_list[k].ident |= (IDENT_RUNES);
			}
	    }

		/* Check inventory */
		for (k = 0; k < INVEN_TOTAL; k++)
		{
			if (inventory[k].k_idx == o_ptr->k_idx)
			{
				inventory[k].ident |= (IDENT_RUNES);
			}
		}
	}

	/* Add ego item rune awareness */
	if ((object_named_p(o_ptr)) && (o_ptr->name2))
	{
		/* Learn ego runes */
		e_info[o_ptr->name2].aware |= (AWARE_RUNES);
	}

	/* No feeling */
	return (0);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 7 (Value magic).
 */
int value_check_aux7(object_type *o_ptr)
{
	/* Cursed/Broken */
	if (cursed_p(o_ptr) || broken_p(o_ptr))
	{
		if (artifact_p(o_ptr)) return (INSCRIP_TERRIBLE);
		if (o_ptr->name2) return (INSCRIP_WORTHLESS);
		if (cursed_p(o_ptr)) return (INSCRIP_CURSED);
		return (INSCRIP_BROKEN);
	}

	/* Hack -- mark object as average and don't value if it is average */
	if (!(o_ptr->name1) && !(o_ptr->name2) && !(o_ptr->xtra1)
		&& !(o_ptr->to_a > 0) && !(o_ptr->to_h + o_ptr->to_d > 0)) return (INSCRIP_AVERAGE);

	/* Value the item */
	o_ptr->ident |= (IDENT_VALUE);

	/* No feeling */
	return (0);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 8 (Name magic).
 */
int value_check_aux8(object_type *o_ptr, bool floor)
{
	/* Important!!! Note different treatment for artifacts. Otherwise the player can
	   potentially lose artifacts by leaving a level after they are 'automatically'
	   identified. Note that for some artifacts - at the moment rings and amulets,
	   even calling object_aware is dangerous at this stage. So we manually make
	   any artifact 'aware' manually. */
	if (artifact_p(o_ptr))
	{
		/* Identify the name */
		o_ptr->ident |= (IDENT_NAME);
	}
	else
	{
		/* Become aware of object */
		object_aware(o_ptr, floor);
	}

	/* No feeling */
	return (0);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 9 (Arrow magic).
 */
int value_check_aux9(object_type *o_ptr)
{
	int feel = 0;

	/* Heavy sensing of missile weapons and ammo */
	if ((o_ptr->tval == TV_BOW) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT) ||
		(o_ptr->tval == TV_BOLT))
	{
		/* Important!!! Note different treatment for artifacts. Otherwise the player can
		   potentially lose artifacts by leaving a level after they are 'automatically'
		   identified. Note that for some artifacts - at the moment rings and amulets,
		   even calling object_aware is dangerous at this stage. */
		if (artifact_p(o_ptr))
		{
			/* Identify the name */
			o_ptr->ident |= (IDENT_NAME);

			/* Mark as an artifact */
			feel = value_check_aux5(o_ptr);
		}
		else
		{
			object_known(o_ptr);
		}
	}
	else
	{
		feel = value_check_aux2(o_ptr);
	}

	/* No feeling */
	return (feel);
}

/*
 * Return a "feeling" (or NULL) about an item.  Methods 10 & 10a (Flag magic and flag weapon).
 *
 * If limit is set, only return either weapon or nonweapon flags.
 */
int value_check_aux10(object_type *o_ptr, bool limit, bool weapon, bool floor)
{
	int feel = 0;

	u32b f1, f2, f3, f4;

	int i, count = 0;
	u32b j, flag2 = 0;

	int flag1 = 0;

	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Remove known flags */
	f1 &= ~(o_ptr->can_flags1);
	f2 &= ~(o_ptr->can_flags2);
	f3 &= ~(o_ptr->can_flags3);
	f4 &= ~(o_ptr->can_flags4);

	/* Check flags 1 */
	for (i = 0, j = 0x00000001L; (i< 32);i++, j <<= 1)
	{
		if (limit && !weapon && (j > TR1_SPEED)) continue;
		else if (limit && weapon && (j <= TR1_SPEED)) continue;

		if ( ((f1) & (j)) && !rand_int(++count) ) { flag1 = 1; flag2 = j;}
	}

	/* Check flags 2 if not weapon */
	if (!limit || !weapon) for (i = 0, j = 0x00000001L; (i< 32);i++, j <<= 1)
	{
		if (((f2) & (j)) && !rand_int(++count)) { flag1 = 2; flag2 = j;}
	}

	/* Check flags 3 if not weapon */
	if (!limit || !weapon) for (i = 0, j = 0x00000001L; (i< 32);i++, j <<= 1)
	{
		/* Skip 'useless' flags */
		if (j & (TR3_ACTIVATE | TR3_ACT_ON_BLOW)) continue;

		if (((f3) & (j)) && !rand_int(++count)) { flag1 = 3; flag2 = j;}
	}

	/* Check flags 4 if not weapon */
	if (!limit || !weapon) for (i = 0, j = 0x00000001L; (i< 32);i++, j <<= 1)
	{
		if (((f4) & (j)) && !rand_int(++count)) { flag1 = 4; flag2 = j;}
	}

	switch(flag1)
	{
		case 1:
			object_can_flags(o_ptr, flag2, 0x0L, 0x0L, 0x0L, floor);
			break;
		case 2:
			object_can_flags(o_ptr, 0x0L, flag2, 0x0L, 0x0L, floor);
			break;
		case 3:
			object_can_flags(o_ptr, 0x0L, 0x0L, flag2, 0x0L, floor);
			break;
		case 4:
			object_can_flags(o_ptr, 0x0L, 0x0L, 0x0L, flag2, floor);
			break;
	}

	if (!flag1) feel = value_check_aux2(o_ptr);

	/* No feeling */
	return (feel);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 11 (Average vs non-average).
 */
int value_check_aux11(object_type *o_ptr)
{
	/* If sensed, have no more value to add */
	if (o_ptr->feeling) return(0);

	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Normal */
		return (INSCRIP_UNUSUAL);
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Normal */
		return (INSCRIP_UNUSUAL);
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return (INSCRIP_UNUSUAL);

	/* Broken items */
	if (broken_p(o_ptr)) return (INSCRIP_UNUSUAL);

	/* Magic item */
	if ((o_ptr->xtra1) && (object_power(o_ptr) > 0)) return (INSCRIP_UNUSUAL);

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return (INSCRIP_UNUSUAL);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return (INSCRIP_UNUSUAL);

	/* Kind */
	if (k_info[o_ptr->k_idx].flavor) return (INSCRIP_UNUSUAL);

	/* Default to average */
	return (INSCRIP_AVERAGE);
}




/*
 * Return a "feeling" (or NULL) about an item.  Method 12 (Magical vs non-magical).
 */
int value_check_aux12(object_type *o_ptr)
{
	/* If sensed magical, have no more value to add */
	if ((o_ptr->feeling == INSCRIP_GOOD) || (o_ptr->feeling == INSCRIP_VERY_GOOD)
		|| (o_ptr->feeling == INSCRIP_GREAT) || (o_ptr->feeling == INSCRIP_EXCELLENT)
		|| (o_ptr->feeling == INSCRIP_SUPERB) || (o_ptr->feeling == INSCRIP_SPECIAL)
		|| (o_ptr->feeling == INSCRIP_MAGICAL)) return (0);

	/* Artifacts */
	if ((artifact_p(o_ptr)) || (o_ptr->feeling == INSCRIP_ARTIFACT))
	{
		/* Known to be artifact strength */
		if ((o_ptr->feeling == INSCRIP_UNBREAKABLE)
			|| (o_ptr->feeling == INSCRIP_ARTIFACT))
		{
			/* Cursed/Broken */
			if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_TERRIBLE);

			return (INSCRIP_SPECIAL);
		}

		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_NONMAGICAL);

		/* Normal */
		return (INSCRIP_MAGICAL);
	}

	/* Ego-Items */
	if ((ego_item_p(o_ptr)) || (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM) || (o_ptr->feeling == INSCRIP_EGO_ITEM))
	{
		/* Known to be high ego-item strength */
		if ((o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM))
		{
			/* Cursed/Broken */
			if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_WORTHLESS);

			return (INSCRIP_SUPERB);
		}

		/* Known to be high ego-item strength */
		if ((o_ptr->feeling == INSCRIP_EGO_ITEM))
		{
			/* Cursed/Broken */
			if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_WORTHLESS);

			return (INSCRIP_EXCELLENT);
		}

		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_NONMAGICAL);

		/* Normal */
		return (INSCRIP_MAGICAL);
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return (INSCRIP_NONMAGICAL);

	/* Broken items */
	/* if (broken_p(o_ptr)) return (INSCRIP_BROKEN); */

	/* Magic item */
	if ((o_ptr->xtra1) && (object_power(o_ptr) > 0)) return (INSCRIP_MAGICAL);

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return (INSCRIP_MAGICAL);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return (INSCRIP_MAGICAL);

	/* Kind */
	if (k_info[o_ptr->k_idx].flavor) return (INSCRIP_MAGICAL);

	/* Default to average */
	return (INSCRIP_NONMAGICAL);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 13 (Runed vs unruned).
 */
int value_check_aux13(object_type *o_ptr)
{
	/* If sensed, have no more value to add */
	if (o_ptr->feeling) return(0);

	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Normal */
		return (INSCRIP_RUNED);
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Normal */
		return (INSCRIP_RUNED);
	}

	/* Magic item */
	if (o_ptr->xtra1) return (INSCRIP_RUNED);

	/* Kind has rune */
	if (k_info[o_ptr->k_idx].runest) return (INSCRIP_RUNED);

	/* Default to average */
	return (INSCRIP_UNRUNED);
}



/*
 * The new item sensing routine.
 *
 * This routine is called at the following times:
 *
 * When an object is created.
 * When an object is walked on
 * When an object is wielded.
 * When an object is carried or wielded for a certain length of time.
 *
 * The sense_level determines what abilities are determined.
 *
 * Level 0 senses only racial abilities.
 * Level 1 is equivalent to old heavy pseudo-id.
 * Level 2 is equivalent to old light pseudo-id.
 * Level 3 is equivalent to detect magic.
 * Level 4 is equivalent to detect curse.
 * Level 5 is equivalent to detect power.
 * Level 6 is equivalent to rune magic.
 * Level 7 is equivalent to value magic.
 * Level 8 is equivalent to identify name only.
 * Level 9 is equivalent to full identify on restricted objects only.
 */
bool sense_magic(object_type *o_ptr, int sense_type, bool heavy, bool floor)
{
	int feel = 0;

	int old_feel;

	bool okay = FALSE;

	/* Skip empty slots */
	if (!o_ptr->k_idx) return (0);

	/* Fully identify items */
	if (adult_no_identify)
	{
		/* Important!!! Note different treatment for artifacts. Otherwise the player can
		   potentially lose artifacts by leaving a level after they are 'automatically'
		   identified. Note that for some artifacts - at the moment rings and amulets,
		   even calling object_aware is dangerous at this stage. */
		if (artifact_p(o_ptr))
		{
			/* Identify the name */
			o_ptr->ident |= (IDENT_NAME);

			/* Mark as an artifact */
			feel = value_check_aux5(o_ptr);
		}
		else
		{
			object_known(o_ptr);
		}
		
		return (TRUE);
	}
	
	/* Sensed this kind? */
	if (k_info[o_ptr->tval].aware & (AWARE_SENSEX))
	{
		int i, j;

		/* Check bags */
		for (i = 0; i < SV_BAG_MAX_BAGS; i++)

		/* Find slot */
		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			if ((bag_holds[i][j][0] == o_ptr->tval)
				&& (bag_holds[i][j][1] == o_ptr->sval))
			  {
			    o_ptr->feeling = MAX_INSCRIP + i;
			  }
		}
	}

	/* Valid "tval" codes */
	switch (o_ptr->tval)
	{
		case TV_LITE:
		{
			/* Torches don't pseudo id */
			if (o_ptr->sval == SV_LITE_TORCH) break;
			
			/* Lanterns only pseudo id if magical/ego/artifact */
			if (!(o_ptr->name1) && !(o_ptr->name2) && !(o_ptr->xtra1)) break;
		}

		case TV_RING:
		case TV_AMULET:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_STAFF:
		case TV_INSTRUMENT:
		{
			okay = TRUE;
			break;
		}
	}

	/* Skip objects */
	if (!okay) return (FALSE);

	/* It is fully known, no information needed */
	if (object_known_p(o_ptr)) return (FALSE);

	/* Always update racial information */
	(void)value_check_aux0(o_ptr, floor);

	old_feel = o_ptr->feeling;

	switch (sense_type)
	{
		case 1:
			feel = heavy ? value_check_aux1(o_ptr) : value_check_aux11(o_ptr);
			break;
		case 2:
			feel = heavy ? value_check_aux2(o_ptr) : value_check_aux11(o_ptr);
			break;
		case 3:
			feel = heavy ? value_check_aux3(o_ptr) : value_check_aux12(o_ptr);
			break;
		case 4:
			feel = heavy ? value_check_aux4(o_ptr) : value_check_aux4(o_ptr);
			break;
		case 5:
			feel = heavy ? value_check_aux5(o_ptr) : value_check_aux11(o_ptr);
			break;
		case 6:
			feel = heavy ? value_check_aux6(o_ptr) : value_check_aux13(o_ptr);
			break;
		case 7:
			feel = heavy ? value_check_aux7(o_ptr) : value_check_aux11(o_ptr);
			break;
		case 8:
			feel = heavy ? value_check_aux8(o_ptr, floor) : value_check_aux11(o_ptr);
			break;
		case 9:
			feel = heavy ? value_check_aux9(o_ptr) : value_check_aux11(o_ptr);
			break;
		case 10:
			feel = heavy ? value_check_aux10(o_ptr, TRUE, FALSE, floor) : value_check_aux11(o_ptr);
			break;
		case 11:
			feel = heavy ? value_check_aux10(o_ptr, TRUE, TRUE, floor) : value_check_aux11(o_ptr);
			break;
	}

	/* Rings and amulets: Only sense curses/artifacts. */
	if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) &&
		/* Rings and amulets only distinguish cursed vs not cursed */
			((feel == INSCRIP_UNUSUAL) || (feel == INSCRIP_MAGIC_ITEM) || (feel == INSCRIP_AVERAGE))) return(FALSE);

	if (feel == old_feel) return(FALSE);

	/* Mark with feeling */
	o_ptr->feeling = feel;

	/* Mark as sensed */
	if ((sense_type) && (heavy)) o_ptr->ident |= (IDENT_SENSE);

	return (TRUE);
}




/*
 * Complete the "creation" of an object by applying "magic" to the item
 *
 * This includes not only rolling for random bonuses, but also putting the
 * finishing touches on ego-items and artifacts, giving charges to wands and
 * staffs and giving fuel to lites.
 *
 * In particular, note that "Instant Artifacts", if "created" by an external
 * routine, must pass through this function to complete the actual creation.
 *
 * The base "chance" of the item being "good" increases with the "level"
 * parameter, which is usually derived from the dungeon level, being equal
 * to the level plus 10, up to a maximum of 75.  If "good" is true, then
 * the object is guaranteed to be "good".  If an object is "good", then
 * the chance that the object will be "great" (ego-item or artifact), also
 * increases with the "level", being equal to half the level, plus 5, up to
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".
 *
 * If the object is not "good", there is a chance it will be "cursed", and
 * if it is "cursed", there is a chance it will be "broken".  These chances
 * are related to the "good" / "great" chances above.
 *
 * Otherwise "normal" rings and amulets will be "good" half the time and
 * "cursed" half the time, unless the ring/amulet is always good or cursed.
 *
 * If "okay" is true, and the object is going to be "great", then there is
 * a chance that an artifact will be created.  This is true even if both the
 * "good" and "great" arguments are false.  Objects which are forced "great"
 * get three extra "attempts" to become an artifact.
 */
void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, bool great)
{
	int i, rolls, f1, f2, power;


	/* Maximum "level" for various things */
	if (lev > MAX_DEPTH - 1) lev = MAX_DEPTH - 1;


	/* Base chance of being "good" */
	f1 = lev + 16;

	/* Maximal chance of being "good" */
	if (f1 > 80) f1 = 80;

	/* Base chance of being "great" */
	f2 = f1 / 2;

	/* Maximal chance of being "great" */
	if (f2 > 25) f2 = 25;


	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if (good || (rand_int(100) < f1))
	{
		/* Assume "good" */
		power = 1;

		/* Roll for "great" */
		if (great || (rand_int(100) < f2)) power = 2;
	}

	/* Roll for "cursed" */
	else if (rand_int(100) < f1)
	{
		/* Assume "cursed" */
		power = -1;

		/* Roll for "broken" */
		if (rand_int(100) < f2) power = -2;
	}


	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2) rolls = 1;

	/* Get four rolls if forced great */
	if (great) rolls = 4;

	/* Get no rolls if not allowed */
	if (!okay || o_ptr->name1) rolls = 0;

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++)
	{
		/* Roll for an artifact */
		if (make_artifact(o_ptr)) break;
	}


	/* Hack -- analyze artifacts */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		/* Hack -- Mark the artifact as "created" */
		a_ptr->cur_num = 1;

		/* Extract the other fields */
		o_ptr->pval = a_ptr->pval;
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;
		o_ptr->to_a = MIN(a_ptr->to_a,
				  a_ptr->ac + 5);
		o_ptr->to_h = a_ptr->to_h;
		o_ptr->to_d = MIN(a_ptr->to_d,
				  (a_ptr->tval == TV_BOW ? 15 : a_ptr->dd * a_ptr->ds + 5));
		o_ptr->weight = a_ptr->weight;

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- extract the "cursed" flag */
		if (a_ptr->flags3 & (TR3_LIGHT_CURSE)) o_ptr->ident |= (IDENT_CURSED);

		/* Set the good item flag */
		good_item_flag = TRUE;

		/* Cheat -- peek at the item */
		if (cheat_peek) object_mention(o_ptr);

		/* Done */
		return;
	}


	/* Apply magic */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOW:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (power)
			{
				a_m_aux_1(o_ptr, lev, power);

				/* Allow defensive weapons to be enchanted */
				if (k_info[o_ptr->k_idx].flags5 & (TR5_SHOW_AC)) a_m_aux_2(o_ptr, lev, power);

				if (((power > 1) || (o_ptr->xtra1) ? TRUE : FALSE) || ((power < -1) || (o_ptr->xtra1) ? TRUE : FALSE))
					(void)make_ego_item(o_ptr, (bool)((power < 0) ? TRUE : FALSE),great);

				if (lev > 3) (void)make_magic_item(o_ptr, lev / 4, power);
			}

			break;
		}

		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_HELM:
		case TV_CROWN:
		case TV_CLOAK:
		case TV_GLOVES:
		case TV_BOOTS:
		{
			if (power)
			{
				a_m_aux_2(o_ptr, lev, power);

				if (((power > 1) || (o_ptr->xtra1) ? TRUE : FALSE) || (power < -1) || (o_ptr->xtra1))
					(void)make_ego_item(o_ptr, (bool)((power < 0) ? TRUE : FALSE),great);

				if (lev > 3) (void)make_magic_item(o_ptr, lev / 4, power);
			}

			break;
		}

		case TV_INSTRUMENT:
		{
			if (power)
			{

				if (((power > 1) || (o_ptr->xtra1) ? TRUE : FALSE) || (power < -1) || (o_ptr->xtra1))
					(void)make_ego_item(o_ptr, (bool)((power < 0) ? TRUE : FALSE),power);

				if (lev > 3) (void)make_magic_item(o_ptr, lev / 4, power);
			}

			break;
		}

		case TV_RING:
		case TV_AMULET:
		{
			if (!power && (rand_int(100) < 50)) power = -1;
			a_m_aux_3(o_ptr, lev, power);
			if ((power) && (lev > k_info[o_ptr->k_idx].level + 3)) boost_item(o_ptr, lev / 4, power);
			break;
		}

		/* Worthless staffs are cursed */
		case TV_STAFF:
		{
			if (!(k_info[o_ptr->k_idx].cost)) power = -1;
			else if (power < 0) power = 0;

			if (power)
			{
				a_m_aux_1(o_ptr, lev, power);
				if (lev > k_info[o_ptr->k_idx].level + 3) boost_item(o_ptr, lev / 4, power);
			}
			a_m_aux_4(o_ptr, lev, power);
			break;
		}

		/* Ego Lanterns */
		case TV_LITE:
		{
			if ((o_ptr->sval == SV_LITE_LANTERN) && (power))
			{
				if (((power > 1) || (o_ptr->xtra1) ? TRUE : FALSE) || (power < -1) || (o_ptr->xtra1))
					(void)make_ego_item(o_ptr, (bool)((power < 0) ? TRUE : FALSE),power);

				if (lev > 3) (void)make_magic_item(o_ptr, lev / 4, power);
			}

			/* Fall through */
		}

		default:
		{
			a_m_aux_4(o_ptr, lev, power);
			break;
		}
	}


	/* Hack -- analyze ego-items */
	if (o_ptr->name2)
	{
		int ego_power;

		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		/* Hack -- acquire "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- cursed items are often "cursed" */
		if (e_ptr->flags3 & (TR3_HEAVY_CURSE | TR3_PERMA_CURSE))  o_ptr->ident |= (IDENT_CURSED);
		else if (e_ptr->flags3 & (TR3_LIGHT_CURSE))
		{
			if (rand_int(100) < 30) o_ptr->ident |= (IDENT_CURSED);
			else o_ptr->ident |= (IDENT_BROKEN);
		}

		/* Apply bonuses or penalties */
		if (e_ptr->max_to_h > 0)
			o_ptr->to_h = MAX(o_ptr->to_h, (s16b)randint(e_ptr->max_to_h));
		else if (e_ptr->max_to_h < 0) o_ptr->to_h -= (s16b)randint(-e_ptr->max_to_h);

		if (e_ptr->max_to_d > 0)
			o_ptr->to_d = MIN(MAX(o_ptr->to_d, (s16b)randint(e_ptr->max_to_d)),
									(o_ptr->tval == TV_BOW
									 ? 15 : o_ptr->dd * o_ptr->ds + 5));
		else if (e_ptr->max_to_d < 0) o_ptr->to_d -= (s16b)randint(-e_ptr->max_to_d);

		if (e_ptr->max_to_a > 0)
			o_ptr->to_a = MIN(MAX(o_ptr->to_a, (s16b)randint(e_ptr->max_to_a)),
									o_ptr->ac + 5);
		else if (e_ptr->max_to_a < 0) o_ptr->to_a -= (s16b)randint(-e_ptr->max_to_a);

		if (e_ptr->max_pval > 0)
			o_ptr->pval = MAX(1, MIN(o_ptr->pval, (s16b)randint(e_ptr->max_pval)));
		else if (e_ptr->max_pval < 0) o_ptr->pval -= (s16b)randint(-e_ptr->max_pval);

		/* Hack -- ensure negatives for broken or cursed items */
		if (cursed_p(o_ptr) || broken_p(o_ptr))
		{
			/* Hack -- obtain bonuses */
			if (o_ptr->to_h > 0) o_ptr->to_h = -o_ptr->to_h;
			if (o_ptr->to_d > 0) o_ptr->to_d = -o_ptr->to_d;
			if (o_ptr->to_a > 0) o_ptr->to_a = -o_ptr->to_a;

			/* Hack -- obtain pval */
			if (o_ptr->pval > 0) o_ptr->pval = -o_ptr->pval;
		}

		/* Get ego power */
		ego_power = object_power(o_ptr);
		if (power < 0) ego_power = -ego_power;

		/* Choose extra power appropriate to ego item */
		if (e_ptr->xtra)
		{
			int choice = 0;
			int x2 = -1;
			int w1 = 10000;
			int w2 = 0;
			int boost = lev * 3 / 2 - ego_power;
			int limit;

			/* 4 + ego_power should give some choice.
			   The rand_int gives a diminishing possibility
			   of the better choices being possible.
			   Tweak these two constants to balance results */
			if (boost < 4)
				boost = 4;
			if (boost < 15)
			{
				limit = ego_power + boost + rand_int(15 - boost);
			}
			else
			{
				limit = ego_power + boost;
			}

			o_ptr->xtra1 = e_ptr->xtra;

			for (o_ptr->xtra2 = 0;
				 o_ptr->xtra2 < object_xtra_size[e_ptr->xtra];
				 o_ptr->xtra2++)
			{
				ego_power = object_power(o_ptr);
				if (power < 0) ego_power = -ego_power;

				/* Is this weakest ability? */
				if (ego_power < w1)
				{
					w1 = ego_power; w2 = o_ptr->xtra2;
				}

				if (ego_power > limit) continue;

				if (!rand_int(++choice)) x2 = o_ptr->xtra2;
			}

			/* Found a power */
			if (x2 >= 0) o_ptr->xtra2 = x2;

			/* Very rarely no x2 ca be chosen -- choose weakest xtra ability */
			else o_ptr->xtra2 = w2;

			/* Reset ego power */
			ego_power = object_power(o_ptr);
			if (power < 0) ego_power = -ego_power;
		}

		/* Boost under-powered ego items if possible */
		if (ego_power < lev)
		{
			boost_item(o_ptr, lev, power);
		}

		/* Cheat -- describe the item */
		if (cheat_peek) object_mention(o_ptr);

		/* Done */
		return;
	}


	/* Examine real objects */
	if (o_ptr->k_idx)
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		/* Hack -- acquire "broken" flag */
		/*if (!k_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);*/

		/* Hack -- cursed items are often "cursed" */
		if (k_ptr->flags3 & (TR3_HEAVY_CURSE | TR3_PERMA_CURSE))  o_ptr->ident |= (IDENT_CURSED);
		else if (k_ptr->flags3 & (TR3_LIGHT_CURSE))
		{
			if (rand_int(100) < 30) o_ptr->ident |= (IDENT_CURSED);
			else o_ptr->ident |= (IDENT_BROKEN);
		}
	}
}



static int hack_body_weight[52]=
{
	500,
	5,
	10,
	300,
	100,
	10,
	0,
	40,
	1,
	5,
	20,
	15,
	30,
	30,
	30,
	50,
	20,
	10,
	6,
	40,
	25,
	15,
	15,
	30,
	20,
	20,
	6,
	3,
	5,
	60,
	20,
	12,
	60,
	12,
	4,
	4,
	12,
	14,
	4,
	15,
	18,
	16,
	16,
	8,
	12,
	16,
	12,
	15,
	6,
	0,
	25,
	16
};

static void modify_weight(object_type *j_ptr, int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Are we done? */
	if ((j_ptr->tval != TV_BONE) && (j_ptr->tval != TV_EGG)
		&& (j_ptr->tval != TV_SKIN) && (j_ptr->tval != TV_BODY) &&
		(j_ptr->tval != TV_HOLD)) return;

	/* Hack -- Increase weight */
	if ((r_ptr->d_char >='A') && (r_ptr->d_char <='Z'))
	{
		j_ptr->weight = j_ptr->weight * hack_body_weight[r_ptr->d_char-'A'] / 10;
	}

	/* Hack -- Increase weight */
	else if ((r_ptr->d_char >='a') && (r_ptr->d_char <='z'))
	{
		j_ptr->weight = j_ptr->weight * hack_body_weight[r_ptr->d_char-'a'+ 26] / 10;
	}

	/* Minimum weight */
	if (j_ptr->weight < 1) j_ptr->weight = 1;
}


static bool (*get_mon_old_hook)(int r_idx);

static int name_drop_k_idx;

static bool name_drop_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	object_kind *j_ptr = &k_info[name_drop_k_idx];

	/* Apply old hook restriction first */
	if (get_mon_old_hook && !(get_mon_old_hook(r_idx))) return (FALSE);

	/* Skip uniques */
	if ((j_ptr->tval != TV_STATUE) && (r_ptr->flags1 & (RF1_UNIQUE))) return (FALSE);

	/* Skip monsters with assemblies, unless an assembly */
	if ((j_ptr->tval != TV_ASSEMBLY) && (r_ptr->flags8 & (RF8_ASSEMBLY))) return (FALSE);

	if (j_ptr->tval == TV_BONE)
	{
		/* Skip if monster does not have body part */
		if ((j_ptr->sval == SV_BONE_SKULL) && !(r_ptr->flags8 & (RF8_HAS_SKULL))) return (FALSE);
		else if ((j_ptr->sval == SV_BONE_BONE) && !(r_ptr->flags8 & (RF8_HAS_SKELETON))) return (FALSE);
		else if ((j_ptr->sval == SV_BONE_SKELETON) && !(r_ptr->flags8 & (RF8_HAS_SKELETON))) return (FALSE);
		else if ((j_ptr->sval == SV_BONE_TEETH) && !(r_ptr->flags8 & (RF8_HAS_TEETH))) return (FALSE);
	}
	else if (j_ptr->tval == TV_EGG)
	{
		/* Spore shooters have spores */
		if ((j_ptr->sval == SV_EGG_SPORE) && !(r_ptr->flags8 & (RF8_HAS_SPORE))) return (FALSE);

		/* Egg-layers have feathers or scales or are insects*/
		else if ((j_ptr->sval == SV_EGG_EGG) && !((r_ptr->flags8 & (RF8_HAS_FEATHER | RF8_HAS_SCALE)) || (r_ptr->flags3 & (RF3_INSECT)))) return (FALSE);

		/* Seed layers are plants but not slimes or spores */
		else if ((j_ptr->sval == SV_EGG_SEED) && ((r_ptr->flags8 & (RF8_HAS_SPORE | RF8_HAS_SLIME)) || !(r_ptr->flags3 & (RF3_PLANT)))) return (FALSE);

		/* Lemure chrysalises hatch demons */
		if ((j_ptr->sval == SV_EGG_CHRYSALIS) && !(r_ptr->flags3 & (RF3_DEMON))) return (FALSE);

		/* Undead/demons never have eggs */
		else if (r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) return (FALSE);

		/* Hack -- dragons only hatch babies */
		if ((r_ptr->flags3 & (RF3_DRAGON)) && (!strstr(r_name+r_ptr->name, "aby"))) return (FALSE);
	}
	else if (j_ptr->tval == TV_SKIN)
	{
		/* Skip if monster does not have body part */
		if ((j_ptr->sval == SV_SKIN_FEATHERS) && !(r_ptr->flags8 & (RF8_HAS_FEATHER))) return (FALSE);
		else if ((j_ptr->sval == SV_SKIN_FEATHER_COAT) && !(r_ptr->flags8 & (RF8_HAS_FEATHER))) return (FALSE);
		else if ((j_ptr->sval == SV_SKIN_SCALES) && !(r_ptr->flags8 & (RF8_HAS_SCALE))) return (FALSE);
		else if ((j_ptr->sval == SV_SKIN_SCALE_COAT) && !(r_ptr->flags8 & (RF8_HAS_SCALE))) return (FALSE);
		else if ((j_ptr->sval == SV_SKIN_FURS) && !(r_ptr->flags8 & (RF8_HAS_FUR))) return (FALSE);
		else if ((j_ptr->sval == SV_SKIN_FUR_COAT) && !(r_ptr->flags8 & (RF8_HAS_FUR))) return (FALSE);

		/* Hack -- we allow lots of skins, Mr Lecter */
		else if ((j_ptr->sval == SV_SKIN_SKIN) && !(r_ptr->flags8 & (RF8_HAS_SLIME | RF8_HAS_FEATHER | RF8_HAS_SCALE | RF8_HAS_FUR))
			&& (r_ptr->flags8 & (RF8_HAS_CORPSE))) return (FALSE);

	}
	else if (j_ptr->tval == TV_BODY)
	{
		/* Skip if monster does not have body part */
		if ((j_ptr->sval == SV_BODY_HEAD) && !(r_ptr->flags8 & (RF8_HAS_HEAD))) return (FALSE);
		else if ((j_ptr->sval == SV_BODY_HAND) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_BODY_ARM) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_BODY_LEG) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_BODY_WING) && !(r_ptr->flags8 & (RF8_HAS_WING))) return (FALSE);
		else if ((j_ptr->sval == SV_BODY_CLAW) && !(r_ptr->flags8 & (RF8_HAS_CLAW))) return (FALSE);
	}
	else if (j_ptr->tval == TV_ASSEMBLY)
	{
		/* Skip if monster is not assembly */
		if (!(r_ptr->flags8 & (RF8_ASSEMBLY))) return (FALSE);

		/* Skip if monster does not have body part */
		if ((j_ptr->sval == SV_ASSEMBLY_NONE) && !(r_ptr->flags8 & (RF8_HAS_CORPSE))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_HEAD) && !(r_ptr->flags8 & (RF8_HAS_HEAD))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_HANDS) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_HAND_L) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_HAND_R) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_HANDS) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_HAND_L) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_HAND_R) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_MISS_HAND_L) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_MISS_HAND_R) && !(r_ptr->flags8 & (RF8_HAS_HAND))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_ARMS) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_ARM_L) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_ARM_R) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_ARMS) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_ARM_L) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_ARM_R) && !(r_ptr->flags8 & (RF8_HAS_ARM))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_LEG_L) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_LEG_R) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_LEGS) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_LEG_L) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_LEG_R) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
		else if ((j_ptr->sval == SV_ASSEMBLY_PART_LEGS) && !(r_ptr->flags8 & (RF8_HAS_LEG))) return (FALSE);
	}
	else if (j_ptr->tval == TV_HOLD)
	{
		/* Only fit elementals in bottles */
		if ((j_ptr->sval == SV_HOLD_BOTTLE) && !(r_ptr->d_char == 'E') && !(r_ptr->d_char == 'v')) return (FALSE);

		/* Only fit undead in boxes */
		else if ((j_ptr->sval == SV_HOLD_BOX) && !(r_ptr->flags3 & (RF3_UNDEAD))) return (FALSE);

		/* Only fit animals in cages */
		else if ((j_ptr->sval == SV_HOLD_CAGE) && !(r_ptr->flags3 & (RF3_ANIMAL))) return (FALSE);

		else return (FALSE);
	}
	else if (j_ptr->tval == TV_STATUE)
	{
		/* Skip never move/never blow */
		if (r_ptr->flags1 & (RF1_NEVER_MOVE | RF1_NEVER_BLOW)) return (FALSE);

		/* Skip stupid */
		if (r_ptr->flags2 & (RF2_STUPID)) return (FALSE);

		/* Hack -- force unique or hybrid statues */
		if (!(r_ptr->flags1 & (RF1_UNIQUE)) && (r_ptr->d_char != 'H')) return (FALSE);
	}
	else if (j_ptr->tval == TV_FLASK)
	{
		if ((j_ptr->sval == SV_FLASK_BLOOD) && !(r_ptr->flags8 & (RF8_HAS_BLOOD))) return (FALSE);
		/* Hack -- breathers have bile */
		if ((j_ptr->sval == SV_FLASK_BILE) && !(r_ptr->flags4 >= (RF4_BRTH_ACID))) return (FALSE);
		if ((j_ptr->sval == SV_FLASK_SLIME) && !(r_ptr->flags8 & (RF8_HAS_SLIME))) return (FALSE);
		if ((j_ptr->sval == SV_FLASK_WEB) && !(r_ptr->flags2 & (RF2_HAS_WEB))) return (FALSE);
	}

	/* Accept */
	return (TRUE);
}



/*
 * Mark item as dropped by monster. Also deal with randomly generated
 * skeletons, bodies and skins.
 */
static void name_drop(object_type *j_ptr)
{
	int r_idx = 0;

	/* Paranoia */
	if (j_ptr->name1) return;
	if (j_ptr->name2) return;
	if (j_ptr->xtra1) return;

#if 0
	/* Mark weapons and armour with racial flags */
	switch (j_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOW:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_HELM:
		case TV_CROWN:
		case TV_CLOAK:
		case TV_GLOVES:
		case TV_BOOTS:
		case TV_INSTRUMENT:
		{
			int count = 0;

			/* Exclude racial kinds */
			if (k_info[j_ptr->k_idx].flags4 & (TR4_ANIMAL | TR4_UNDEAD | TR4_DEMON | TR4_ORC | TR4_TROLL | TR4_GIANT |
						TR4_DRAGON | TR4_MAN | TR4_ELF | TR4_DWARF)) return;

			if (race_drop_idx) r_idx = race_drop_idx;
			else
			{
				/* Generate monster appropriate for level */
				r_idx = get_mon_num(monster_level);
			}

			/* Apply flags */
			if ((r_info[r_idx].flags3 & (RF3_UNDEAD)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 18;}
			if ((r_info[r_idx].flags3 & (RF3_DEMON)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 19;}
			if ((r_info[r_idx].flags3 & (RF3_ORC)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 20;}
			if ((r_info[r_idx].flags3 & (RF3_TROLL)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 21;}
			if ((r_info[r_idx].flags3 & (RF3_GIANT)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 22;}
			if ((r_info[r_idx].flags9 & (RF9_MAN)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 24;}
			if ((r_info[r_idx].flags9 & (RF9_DWARF)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 25;}
			if ((r_info[r_idx].flags9 & (RF9_ELF)) && !(rand_int(count++))) { j_ptr->xtra1 = 19; j_ptr->xtra2 = 26;}
		}
	}
#endif
	/* Are we done? */
	if ((j_ptr->tval != TV_BONE) && (j_ptr->tval != TV_EGG) && (j_ptr->tval != TV_STATUE)
		&& (j_ptr->tval != TV_SKIN) && (j_ptr->tval != TV_BODY) &&
		(j_ptr->tval != TV_HOLD) && (j_ptr->tval != TV_FLASK)) return;

	/* Hack -- only some flasks are flavoured */
	if ((j_ptr->tval == TV_FLASK) && (j_ptr->sval != SV_FLASK_BLOOD) && (j_ptr->sval != SV_FLASK_SLIME)
		&& (j_ptr->sval != SV_FLASK_BILE) && (j_ptr->sval != SV_FLASK_WEB)) return;

	/* Flavor the drop with a monster type */
	if ((rand_int(100) < (30+ (p_ptr->depth * 2))) || (race_drop_idx))
	{
		bool old_ecology = cave_ecology.ready;

		/* Store the old hook */
		get_mon_old_hook = get_mon_num_hook;

		/* Set the hook */
		get_mon_num_hook = name_drop_okay;

		/* Store the item kind */
		name_drop_k_idx = j_ptr->k_idx;

		/* Sometimes ignore the ecology */
		if (rand_int(100) < 50) cave_ecology.ready = FALSE;

		/* Prep the list */
		get_mon_num_prep();

		/* Generate monster appropriate for level */
		r_idx = get_mon_num(monster_level);

		/* Restore the old hook */
		get_mon_num_hook = get_mon_old_hook;

		/* Prep the list */
		get_mon_num_prep();

		/* Reset the ecology */
		cave_ecology.ready = old_ecology;

		/* Failure? */
		if (!r_idx) return;

		/* Flavor the skeleton */
		j_ptr->name3 = r_idx;
	}
}

/*
 * Hack -- determine if a template is "mushroom".
 */
static bool kind_is_shroom(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->tval != TV_MUSHROOM) return (FALSE);

	return (TRUE);
}


/*
 * Hack -- determine if a template is "great"
 *
 * Similar to kind is good, but doesn't include disposables / books / activatables.
 */
static bool kind_is_great(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Armor -- Good unless damaged */
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		{
			if (k_ptr->to_a < 0) return (FALSE);
			return (TRUE);
		}

		/* Weapons -- Good unless damaged */
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		{
			if (k_ptr->to_h < 0) return (FALSE);
			if (k_ptr->to_d < 0) return (FALSE);
			return (TRUE);
		}

		/* Rune stones and magical bags are good if not seen previously */
		case TV_BAG:
		{
			if (!(k_ptr->aware & (AWARE_SEEN))) return (TRUE);
			return (FALSE);
		}

		/* Rods/Scrolls/Potions/Amulets/Wands/Rings -- Deep is good */
		case TV_RING:
		case TV_AMULET:
		{
			if ((k_ptr->level >= 35) && (k_ptr->level > object_level + 9) && !(k_ptr->flags3 & (TR3_LIGHT_CURSE))) return (TRUE);
			return (FALSE);
		}
	}

	/* Assume not good */
	return (FALSE);

}


/*
 * Hack -- determine if a template is "good".
 *
 * Note that this test only applies to the object *kind*, so it is
 * possible to choose a kind which is "good", and then later cause
 * the actual object to be cursed.  We do explicitly forbid objects
 * which are known to be boring or which start out somewhat damaged.
 */
static bool kind_is_good(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Armor -- Good unless damaged */
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		{
			if (k_ptr->to_a < 0) return (FALSE);
			return (TRUE);
		}

		/* Weapons -- Good unless damaged */
		case TV_BOW:
		case TV_STAFF:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		{
			if (k_ptr->to_h < 0) return (FALSE);
			if (k_ptr->to_d < 0) return (FALSE);

			/* Special case for staves */
			if (k_ptr->tval == TV_STAFF
				 && k_ptr->level < 25
				 && k_ptr->level < object_level + 5)
				return (FALSE);

			return (TRUE);
		}

		/* Ammo -- Arrows/Bolts are good */
		case TV_BOLT:
		case TV_ARROW:
		{
			return (TRUE);
		}

		/* Books -- high level books are good if not seen previously */
		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:
		case TV_SONG_BOOK:
		{
			if ((k_ptr->sval < SV_BOOK_MAX_GOOD) && ((k_ptr->aware & (AWARE_SEEN)) == 0)) return (TRUE);
			return (FALSE);
		}

		/* Rune stones and magical bags are good if not seen previously */
		case TV_BAG:
		case TV_RUNESTONE:
		{
			if ((k_ptr->aware & (AWARE_SEEN)) == 0) return (TRUE);
			return (FALSE);
		}

		/* Rods/Scrolls/Potions/Amulets/Wands/Rings -- Deep is good */
		case TV_ROD:
		case TV_RING:
		case TV_AMULET:
		case TV_SCROLL:
		case TV_POTION:
		case TV_WAND:
		case TV_FLASK:
		{
			if ((k_ptr->level >= 35) && (k_ptr->level > object_level + 9) && !(k_ptr->flags3 & (TR3_LIGHT_CURSE))) return (TRUE);
			return (FALSE);
		}

		/* Lites -- Must be lantern */
		case TV_LITE:
		{
			if (k_ptr->sval == SV_LITE_LANTERN) return (TRUE);
			return (FALSE);
		}

	}

	/* Assume not good */
	return (FALSE);
}

/*
 * Hack -- determine if a template is dropped by a race.
 */
static bool kind_is_tval(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->tval == tval_drop_idx) return (TRUE);

	return (FALSE);
}

/*
 * Hack -- determine if a template is dropped by a race.
 */
static bool kind_is_race(int k_idx)
{
	monster_race *r_ptr = &r_info[race_drop_idx];
	object_kind *k_ptr = &k_info[k_idx];

	/* Handle good items */
	if ((r_ptr->flags1 & (RF1_DROP_GREAT)) && (!kind_is_great(k_idx))) return (FALSE);

	/* Handle good items */
	if ((r_ptr->flags1 & (RF1_DROP_GOOD)) && (!kind_is_good(k_idx))) return (FALSE);

	/* Handle mimics differently */
	if (r_ptr->flags1 & (RF1_CHAR_MULTI))
	{
		/* Hack -- act like chest */
		if (r_ptr->d_char == '&') return (TRUE);

		/* Force char */
		if (r_ptr->d_char != k_ptr->d_char) return (FALSE);

		/* Allow any attribute */
		if (r_ptr->flags9 & (RF9_ATTR_INDEX)) return (TRUE);

		/* Force attribute */
		if (r_ptr->d_attr != k_ptr->d_attr) return (FALSE);

		/* Done */
		return (TRUE);
	}

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Hard Armor/Dragon Armor/Shield/Helm */
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Hack -- monster equipment only has one armour */
			if (hack_monster_equip & (RF8_DROP_ARMOR)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_ARMOR)) return (TRUE);
			return (FALSE);
		}
		case TV_SHIELD:
		{
			/* Hack -- monster equipment only has one shield */
			if (hack_monster_equip & (RF8_HAS_ARM)) return (FALSE);

			/* Shield is heavy armour */
			if (r_ptr->flags8 & (RF8_DROP_ARMOR)) return (TRUE);
			return (FALSE);
		}
		case TV_HELM:
		{
			/* Hack -- monster equipment only has one gloves */
			if ((k_ptr->tval == TV_HELM) && (hack_monster_equip & (RF8_HAS_HEAD))) return (FALSE);

			/* Helms are heavy armour */
			if (r_ptr->flags8 & (RF8_DROP_ARMOR)) return (TRUE);
			return (FALSE);
		}
		/* Soft armor/boots/cloaks/gloves */
		case TV_SOFT_ARMOR:
		{
			/* Hack -- monster equipment only has one armour */
			if (hack_monster_equip & (RF8_DROP_ARMOR)) return (FALSE);

			/* Hack -- armored monsters don't carry soft armor */
			if (r_ptr->flags2 & (RF2_ARMOR)) return (FALSE);

			/* Clothes are soft armour */
			if (r_ptr->flags8 & (RF8_DROP_CLOTHES)) return (TRUE);
			return (FALSE);
		}

		case TV_GLOVES:
		{
			/* Hack -- monster equipment only has one gloves */
			if (hack_monster_equip & (RF8_HAS_HAND)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_CLOTHES)) return (TRUE);
			return (FALSE);
		}
		case TV_CLOAK:
		{
			/* Hack -- monster equipment only has one cloak */
			if (hack_monster_equip & (RF8_HAS_CORPSE)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_CLOTHES)) return (TRUE);
			return (FALSE);
		}
		case TV_BOOTS:
		{
			/* Hack -- monster equipment only has one armour */
			if (hack_monster_equip & (RF8_HAS_LEG)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_CLOTHES)) return (TRUE);
			return (FALSE);
		}
		/* Weapons */
		case TV_SWORD:
		case TV_POLEARM:
		{
			/* Hack -- priests other than shamans only carry hafted weapons */
			if ((r_ptr->flags2 & (RF2_PRIEST)) && !(r_ptr->flags2 & (RF2_MAGE))) return (FALSE);

		/* Fall through */
		}
		case TV_HAFTED:
		{
			/* Hack -- mages/priests/thieves/archers only carry weapons < 20 lbs */
			if ((r_ptr->flags2 & (RF2_PRIEST | RF2_MAGE | RF2_SNEAKY | RF2_ARCHER)) && (k_ptr->weight >= 200)) return (FALSE);

			/* Hack -- warriors only carry weapons >= 7 lbs */
			if ((r_ptr->flags2 & (RF2_ARMOR)) && (k_ptr->weight < 70)) return (FALSE);

			/* Hack -- monster equipment only has one weapon */
			if (hack_monster_equip & (RF8_DROP_WEAPON)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_WEAPON)) return (TRUE);
			return (FALSE);
		}

		/* Bows/Ammo */
		case TV_BOW:
		{
			/* Hack -- monster equipment only has one bow */
			if (hack_monster_equip & (RF8_DROP_MISSILE)) return (FALSE);

			/* Fall through */
		}
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		{
			if (r_ptr->flags8 & (RF8_DROP_MISSILE)) return (TRUE);
			return (FALSE);
		}

		/* Books/Scrolls */
		case TV_MAGIC_BOOK:
		{
			/* Hack -- priests other than shamans do not carry magic books*/
			if ((r_ptr->flags2 & (RF2_PRIEST)) && !(r_ptr->flags2 & (RF2_MAGE))) return (FALSE);

			/* Mega hack -- priests and paladins other than shamans do not carry magic books */
			if ((r_ptr->d_char == 'p') && !(r_ptr->flags2 & (RF2_MAGE))) return (FALSE);

			/* Hack -- monster equipment only has limited writings */
			if (hack_monster_equip & (RF8_DROP_WRITING)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_WRITING)) return (TRUE);
			return (FALSE);
		}
		case TV_PRAYER_BOOK:
		{
			/* Hack -- mages other than shamans do not carry priest books*/
			if ((r_ptr->flags2 & (RF2_MAGE)) && !(r_ptr->flags2 & (RF2_PRIEST))) return (FALSE);

			/* Mega hack -- mages and rangers other than shamans do not carry priest books */
			if ((r_ptr->d_char == 'q') && !(r_ptr->flags2 & (RF2_PRIEST))) return (FALSE);

			/* Hack -- monster equipment only has limited writings */
			if (hack_monster_equip & (RF8_DROP_WRITING)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_WRITING)) return (TRUE);
			return (FALSE);
		}
		case TV_SCROLL:
		case TV_RUNESTONE:
		case TV_MAP:
		{
			/* Hack -- monster equipment only has limited writings */
			if (hack_monster_equip & (RF8_DROP_WRITING)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_WRITING)) return (TRUE);
			return (FALSE);
		}

		/* Rings/Amulets/Crowns */
		case TV_RING:
		{
			/* Hack -- monster equipment only has limited rings */
			if (hack_monster_equip & (RF8_DROP_JEWELRY)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_JEWELRY)) return (TRUE);
			return (FALSE);
		}
		case TV_AMULET:
		{
			/* Hack -- monster equipment only has one amulet */
			if (hack_monster_equip & (RF8_HAS_SKULL)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_JEWELRY)) return (TRUE);
			return (FALSE);
		}
		case TV_CROWN:
		{
			/* Hack -- monster equipment only has one crown/amulet */
			if (hack_monster_equip & (RF8_HAS_HEAD)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_JEWELRY)) return (TRUE);
			return (FALSE);
		}

		/* Potions */
		case TV_POTION:
		{
			/* Hack -- monster equipment only has limited potions */
			if (hack_monster_equip & (RF8_DROP_POTION)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_POTION)) return (TRUE);
			return (FALSE);
		}

		/* Food */
		case TV_FOOD:
		case TV_MUSHROOM:
		{
			/* Hack -- monster equipment only has limited food */
			if (hack_monster_equip & (RF8_DROP_FOOD)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_FOOD)) return (TRUE);
			return (FALSE);
		}

		/* Lite/Fuel */
		case TV_LITE:
		{
			/* Hack -- monster equipment only has one lite */
			if (hack_monster_equip & (RF8_DROP_LITE)) return (FALSE);

			if (r_ptr->flags2 & (RF2_HAS_LITE | RF2_NEED_LITE)) return (TRUE);
			if (r_ptr->flags8 & (RF8_DROP_LITE)) return (TRUE);
			return (FALSE);
		}

		/* Containers */
		case TV_HOLD:
		case TV_BAG:
		{
			/* Hack -- monster equipment only has one bag */
			if (hack_monster_equip & (RF8_DROP_CHEST)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_CHEST)) return (TRUE);
			return (FALSE);
		}

		/* Bottles/Skeletons/Broken Pottery */
		case TV_BONE:
		case TV_JUNK:
		case TV_SKIN:
		{
			/* Hack -- monster equipment only has limited potions */
			if (hack_monster_equip & (RF8_DROP_JUNK)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_JUNK)) return (TRUE);
			return (FALSE);

		}

		/* Diggers/Spikes */
		case TV_DIGGING:
		case TV_SPIKE:
		case TV_FLASK:
		case TV_ROPE:
		{
			/* Hack -- monster equipment only has one tool */
			if (hack_monster_equip & (RF8_DROP_TOOL)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_TOOL)) return (TRUE);
			return (FALSE);

		}

		/* Instruments/Song Books */
		case TV_SONG_BOOK:
		case TV_INSTRUMENT:
		{
			/* Hack -- monster equipment only has limited song books / one instrument */
			if (hack_monster_equip & (RF8_DROP_MUSIC)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_MUSIC)) return (TRUE);
			return (FALSE);
		}

		/* Rod/staff/wand */
		case TV_STAFF:
		{
			if (hack_monster_equip & (RF8_DROP_WEAPON)) return (FALSE);
		}
		case TV_ROD:
		case TV_WAND:
		{
			/* Hack -- monster equipment only has limited rods/staffs/wands */
			if (hack_monster_equip & (RF8_DROP_RSW)) return (FALSE);

			if (r_ptr->flags8 & (RF8_DROP_RSW)) return (TRUE);
			return (FALSE);
		}
	}

	/* Assume not allowed */
	return (FALSE);
}





/*
 * Attempt to make an object (normal or good/great)
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * We assume that the given object has been "wiped".
 */
bool make_object(object_type *j_ptr, bool good, bool great)
{
	int prob, base;

	bool clear_temp = FALSE;

	/* Chance of "special object" */
	prob = (good ? 10 : 1000);

	/* Base level for the object */
	base = (good ? (object_level + 10) : object_level);

	/* Hack -- mushrooms only drop themselves */
	if (food_type)
	{
		int k_idx;

		if (food_type > 0)
		{
			k_idx = lookup_kind(TV_MUSHROOM,food_type-1);

			/* Handle failure */
			if (!k_idx) return (FALSE);
		}
		else
		{
			/* Activate restriction */
			get_obj_num_hook = kind_is_shroom;

			/* Prepare allocation table */
			get_obj_num_prep();

			/* Pick a random object */
			k_idx = get_obj_num(base);

			/* Clear restriction */
			get_obj_num_hook = NULL;

			/* Prepare allocation table */
			get_obj_num_prep();
		}

		/* Handle failure */
		if (!k_idx) return (FALSE);

		/* Prepare the object */
		object_prep(j_ptr, k_idx);

		/* Auto-inscribe if necessary */
		if ((adult_auto) || (object_aware_p(j_ptr))) j_ptr->note = k_info[k_idx].note;

	}

	/* Generate a special artifact, or a normal object */
	else if ((rand_int(prob) != 0) || !make_artifact_special(j_ptr))
	{
		int k_idx;

		/* Good objects */
		if ((good) || (great) || (race_drop_idx) || (tval_drop_idx))
		{
			/* Activate tval restriction */
			if (tval_drop_idx) get_obj_num_hook = kind_is_tval;

			/* Activate racial restriction */
			else if (race_drop_idx) get_obj_num_hook = kind_is_race;

			/* Activate 'great' restriction */
			else if (great) get_obj_num_hook = kind_is_great;

			/* Activate 'good' restriction */
			else get_obj_num_hook = kind_is_good;

			/* Prepare allocation table */
			get_obj_num_prep();
		}
		/* MegaHack - for the moment. Ensure that each tval is dropped approximately with the same
		 * frequency
		 */
		else if (!get_obj_num_hook)
		{
			/* Pick a random tval. */
			switch (rand_int(15 + 5 * (object_level / 3)) + 3)
			{
				case 7:	tval_drop_idx = TV_FOOD; break;
				case 8: tval_drop_idx = TV_MUSHROOM; break;
				case 9: tval_drop_idx = TV_POTION; break;
				case 10: tval_drop_idx = TV_SCROLL; break;
				case 11: tval_drop_idx = TV_FLASK; break;
				case 12:	tval_drop_idx = TV_SHOT; break;
				case 13:	tval_drop_idx = TV_ARROW; break;
				case 14: tval_drop_idx = TV_BOLT; break;
				case 15:	tval_drop_idx = TV_GLOVES; break;
				case 16:	tval_drop_idx = TV_BOOTS; break;
				case 17:	tval_drop_idx = TV_CLOAK; break;
				case 18:	tval_drop_idx = TV_RING; break;
				case 19: tval_drop_idx = TV_SHIELD; break;
				case 20: tval_drop_idx = TV_HELM; break;
				case 21: tval_drop_idx = TV_WAND; break;
				case 22:	tval_drop_idx = TV_SOFT_ARMOR; break;
				case 23:	tval_drop_idx = TV_LITE; break;
				case 24: tval_drop_idx = TV_SWORD; break;
				case 25: tval_drop_idx = TV_POLEARM; break;
				case 26: tval_drop_idx = TV_HAFTED; break;
				case 27: tval_drop_idx = TV_BOW; break;
				case 28: tval_drop_idx = TV_STAFF; break;
				case 29:	tval_drop_idx = TV_HARD_ARMOR; break;
				case 30:	tval_drop_idx = TV_AMULET; break;
				case 31: tval_drop_idx = TV_POTION; break;
				case 32: tval_drop_idx = TV_SCROLL; break;
				case 33: tval_drop_idx = TV_ROD; break;
				case 34: tval_drop_idx = TV_MAGIC_BOOK; break;
				case 35:	tval_drop_idx = TV_PRAYER_BOOK; break;
				case 36:	tval_drop_idx = TV_SONG_BOOK; break;
				case 37:	tval_drop_idx = TV_INSTRUMENT; break;
				case 38:	tval_drop_idx = TV_RUNESTONE; break;
			}

			if (tval_drop_idx)
			{
				/* Activate tval restriction */
				get_obj_num_hook = kind_is_tval;

				/* Prepare allocation table */
				get_obj_num_prep();

				clear_temp = TRUE;
			}
		}

		/* Pick a random object */
		k_idx = get_obj_num(base);

		/* Good objects */
		if ((good) || (great) || (race_drop_idx) || (tval_drop_idx))
		{
			/* Clear restriction */
			get_obj_num_hook = NULL;

			/* Prepare allocation table */
			get_obj_num_prep();

			if (clear_temp) tval_drop_idx = FALSE;
		}

		/* Handle failure */
		if (!k_idx) return (FALSE);

		/* Prepare the object */
		object_prep(j_ptr, k_idx);

		/* Hack -- good / great objects are never cursed */
		if (good || great) j_ptr->ident &= ~(IDENT_CURSED);

		/* Auto-inscribe if necessary */
		if ((adult_auto) || (object_aware_p(j_ptr))) j_ptr->note = k_info[k_idx].note;
	}

	/* Mark as dropped */
	name_drop(j_ptr);

	/* Modify weight */
	modify_weight(j_ptr,j_ptr->name3);

	/* Apply magic (allow artifacts) */
	apply_magic(j_ptr, object_level, TRUE, good, great);

	/* Generate multiple items */
	if (!artifact_p(j_ptr)) switch (j_ptr->tval)
	{
		case TV_ROPE:
		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			j_ptr->number = damroll(6, 7);
			break;
		}
		case TV_POTION:
		case TV_SCROLL:
		case TV_FLASK:
		{
			if (object_level > k_info[j_ptr->k_idx].level + 9) j_ptr->number = damroll(3, 4);
			else if (object_level > k_info[j_ptr->k_idx].level + 4) j_ptr->number = damroll(2, 3);
			else j_ptr->number = (byte)randint(3);

			/* Hack -- reduce stack sizes of deep items */
			j_ptr->number /= (k_info[j_ptr->k_idx].level / 15) + 1;

			if (j_ptr->number < 1) j_ptr->number = 1;
			break;
		}
		/* Activatable rings stack */
		case TV_RING:
		{
			if ((k_info[j_ptr->k_idx].flags3 & (TR3_ACTIVATE | TR3_UNCONTROLLED)) == 0) break;

			/* Fall through */
		}
		case TV_ROD:
		case TV_STAFF:
		case TV_WAND:
		case TV_FOOD:
		case TV_MUSHROOM:
		{
			if (object_level > k_info[j_ptr->k_idx].level + 9) j_ptr->number = (byte)randint(5);
			else if (object_level > k_info[j_ptr->k_idx].level + 4) j_ptr->number = (byte)randint(3);
			break;
		}
		default:
		{
			/* Throwing weapons come in stacks unless ego items */
			if ((k_info[j_ptr->k_idx].flags5 & (TR5_THROWING))
				&& (!ego_item_p(j_ptr))
				&& (object_level > k_info[j_ptr->k_idx].level + 4))
				{
					/* Magic items are permitted stacks only for a few different types */
					if (((j_ptr->xtra1 == 16) && (j_ptr->xtra2 < 14)) ||
							((j_ptr->xtra1 == 17) && ((j_ptr->xtra2 < 6) || (j_ptr->xtra2 > 11))) ||
							((j_ptr->xtra1 == 18) && (j_ptr->xtra2 < 27)) ||
							((j_ptr->xtra1 == 19) && (((j_ptr->xtra2 > 1) && (j_ptr->xtra2 < 8)) || (j_ptr->xtra2 > 11))))
					{
						break;
					}

					/* Stack of throwing weapons */
					j_ptr->number = damroll(3, 4);
				}
			break;
		}
	}

	/* Notice "okay" out-of-depth objects */
	if (!cursed_p(j_ptr) && !broken_p(j_ptr) &&
	    (k_info[j_ptr->k_idx].level > p_ptr->depth))
	{
		/* Cheat -- peek at items */
		if (cheat_peek) object_mention(j_ptr);
	}

	/* Sense some magic on object at creation time */
	sense_magic(j_ptr, cp_ptr->sense_type, (p_ptr->lev >= 40) || rand_int(100) < 20 + p_ptr->lev * 2, TRUE);

	/* Auto-id average items */
	if (j_ptr->feeling == INSCRIP_AVERAGE) object_bonus(j_ptr, TRUE);

	/* Hack -- theme chests */
	if (opening_chest) tval_drop_idx = j_ptr->tval;

	/* Apply obvious flags */
	object_obvious_flags(j_ptr, TRUE);

	/* Rune magic on this kind */
	if ((k_info[j_ptr->k_idx].flavor) && !(object_aware_p(j_ptr)) && (k_info[j_ptr->k_idx].aware & (AWARE_RUNEX)))
	{
		j_ptr->ident |= (IDENT_RUNES);
	}

	/* Sense magic on this kind */
	if ((k_info[j_ptr->k_idx].flavor) && !(object_aware_p(j_ptr)) && (k_info[j_ptr->k_idx].aware & (AWARE_SENSEX)))
	{
		int i, j;

		/* Check bags */
		for (i = 0; i < SV_BAG_MAX_BAGS; i++)

		/* Find slot */
		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			if ((bag_holds[i][j][0] == j_ptr->tval)
				&& (bag_holds[i][j][1] == j_ptr->sval))
			{
			    j_ptr->feeling = MAX_INSCRIP + i;
			    
			    break;
			}
		}
	}

	/* Success */
	return (TRUE);
}

/*
 * XXX XXX XXX Do not use these hard-coded values.
 */
#define OBJ_GOLD_LIST   480     /* First "gold" entry */
#define MAX_GOLD        18      /* Number of "gold" entries */

/*
 * Make a treasure object
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_gold(object_type *j_ptr, bool good, bool great)
{
	int i;

	s32b base;

	/* Hack -- Pick a Treasure variety */
	i = ((randint(object_level + 2) + 2) / 2) - 1;

	/* Apply "extra" magic */
	if (rand_int(GREAT_OBJ) == 0)
	{
		/* This used to cause too much adamantium */
		i += MAX_GOLD / 3;
	}

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type;

	/* Do not create "illegal" Treasure Types */
	if (i >= MAX_GOLD) i = MAX_GOLD - 1;

	/* Prepare a gold object */
	object_prep(j_ptr, OBJ_GOLD_LIST + i);

	/* Hack -- Base coin cost */
	base = k_info[OBJ_GOLD_LIST+i].cost;

	/* Determine how much the treasure is "worth" */
	base = (base + (8 * randint(base)) + randint(8));

	/* Apply good or great flags */
	if (great) base *= (k_info[OBJ_GOLD_LIST + i].tval == TV_GEMS ? 100 : damroll(7, 4));
	else if (good) base *= (k_info[OBJ_GOLD_LIST + i].tval == TV_GEMS ? 10 : damroll(2, 3));

	/* Prevent overrun */
	if (base > 31000) base = 30500 + rand_int(1000);
	
	/* Set item value */
	j_ptr->charges = base;
	
	/* Success */
	return (TRUE);
}

/*
 * Make a body
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_body(object_type *j_ptr, int r_idx)
{
	int k_idx = 0;

	monster_race *r_ptr = &r_info[r_idx];

	/* Hack -- handle trees */
	if (r_ptr->d_char == '<')
	{
		k_idx = lookup_kind(TV_JUNK,SV_JUNK_STUMP);

		if (!k_idx) return (FALSE);

		object_prep(j_ptr,k_idx);
#if 0
		/* Inscribe it */
		j_ptr->note = r_info[r_idx].note;
#endif
		return (TRUE);
	}

	/* No body */
	if (!(r_ptr->flags8 & (RF8_HAS_CORPSE | RF8_HAS_SKELETON | RF8_HAS_SKULL | RF8_ASSEMBLY))) return (FALSE);

	/* Living hurt rock monsters turn to stone */
	if ((r_ptr->flags3 & (RF3_HURT_ROCK)) && !(r_ptr->flags3 & (RF3_NONLIVING))) k_idx = lookup_kind(TV_STATUE,SV_STATUE_STONE);

	/* Hack -- golems leave behind assemblies */
	else if (r_ptr->flags8 & (RF8_ASSEMBLY))
	{
		int sval = rand_int(SV_MAX_ASSEMBLY);

		/* Hack -- ensure larger parts */
		if ((sval < SV_ASSEMBLY_FULL) && !(sval % 2)) sval++;
		else if (sval == SV_ASSEMBLY_HANDS) sval--;

		k_idx = lookup_kind(TV_ASSEMBLY, sval);
	}

	/* Monsters with corpses produce corpses */
	else if (r_ptr->flags8 & (RF8_HAS_CORPSE)) k_idx = lookup_kind(TV_BODY,SV_BODY_CORPSE);

	/* Monsters without corpses may produce skeletons */
	else if (r_ptr->flags8 & (RF8_HAS_SKELETON)) k_idx = lookup_kind(TV_BONE,SV_BONE_SKELETON);

	/* Monsters without corpses or skeletons may produce skulls */
	else if (r_ptr->flags8 & (RF8_HAS_SKULL)) k_idx = lookup_kind(TV_BONE,SV_BONE_SKULL);

	if (!k_idx) return (FALSE);

	/* Prepare the object */
	object_prep(j_ptr, k_idx);
#if 0
	/* Inscribe it */
	j_ptr->note = r_info[r_idx].note;
#endif
	/* Re-inscribe it */
	if ((!j_ptr->note) && !(r_ptr->flags1 & (RF1_UNIQUE))) j_ptr->note = k_info[k_idx].note;

	/* Modify weight */
	modify_weight(j_ptr,r_idx);

	/* Success */
	return (TRUE);
}

/*
 * Make a head
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_head(object_type *j_ptr, int r_idx)
{
	int k_idx = 0;

	monster_race *r_ptr = &r_info[r_idx];

	/* No body */
	if (!(r_ptr->flags8 & (RF8_HAS_HEAD))) return (FALSE);

	/* Usually produce a head */
	k_idx = lookup_kind(TV_BODY,SV_BODY_HEAD);

	if (!k_idx) return (FALSE);

	/* Living hurt rock monsters get assembly (statue) heads sometimes */
	if ((r_ptr->flags3 & (RF3_HURT_ROCK)) && !(r_ptr->flags3 & (RF3_NONLIVING))) k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_HEAD);

	/* Hack -- golems leave behind assemblies */
	else if (r_ptr->flags8 & (RF8_ASSEMBLY)) k_idx = lookup_kind(TV_ASSEMBLY, SV_ASSEMBLY_HEAD);

	if (!k_idx) return (FALSE);

	/* Prepare the object */
	object_prep(j_ptr, k_idx);
#if 0
	/* Inscribe it */
	j_ptr->note = r_info[r_idx].note;
#endif
	/* Re-inscribe it */
	if ((!j_ptr->note) && !(r_ptr->flags1 & (RF1_UNIQUE))) j_ptr->note = k_info[k_idx].note;

	/* Modify weight */
	modify_weight(j_ptr,r_idx);

	/* Success */
	return (TRUE);
}


/*
 * Make a body part
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_part(object_type *j_ptr, int r_idx)
{
	int k_idx = 0;

	monster_race *r_ptr = &r_info[r_idx];

	/* Hack -- handle trees */
	if (r_ptr->d_char == '<')
	{
		k_idx = lookup_kind(TV_JUNK,SV_JUNK_BRANCH);

		if (!k_idx) return (FALSE);

		object_prep(j_ptr,k_idx);

		/* Re-inscribe it */
		if (!j_ptr->note) j_ptr->note = k_info[k_idx].note;

		return (TRUE);
	}

	/* Usually hack off a hand or nearest approximation */
	if (r_ptr->flags8 & (RF8_HAS_CLAW)) k_idx = lookup_kind(TV_BODY,SV_BODY_CLAW);
	else if (r_ptr->flags8 & (RF8_HAS_HAND)) k_idx = lookup_kind(TV_BODY,SV_BODY_HAND);
	else if (r_ptr->flags8 & (RF8_HAS_WING)) k_idx = lookup_kind(TV_BODY,SV_BODY_WING);
	else if (r_ptr->flags8 & (RF8_HAS_ARM)) k_idx = lookup_kind(TV_BODY,SV_BODY_ARM);
	else if (r_ptr->flags8 & (RF8_HAS_LEG)) k_idx = lookup_kind(TV_BODY,SV_BODY_LEG);

	/* Sometimes hack off something else */
	if ((rand_int(100)<30) &&(r_ptr->flags8 & (RF8_HAS_ARM))) k_idx = lookup_kind(TV_BODY,SV_BODY_ARM);
	if ((rand_int(100)<30) &&(r_ptr->flags8 & (RF8_HAS_LEG))) k_idx = lookup_kind(TV_BODY,SV_BODY_LEG);
	if ((rand_int(100)<30) &&(r_ptr->flags8 & (RF8_HAS_WING))) k_idx = lookup_kind(TV_BODY,SV_BODY_WING);
	if ((rand_int(100)<30) &&(r_ptr->flags8 & (RF8_HAS_TEETH))) k_idx = lookup_kind(TV_BONE,SV_BONE_TEETH);

	/* Have a part */
	if (!k_idx) return(FALSE);

	/* Handle assembly monsters */
	if (r_ptr->flags8 & (RF8_ASSEMBLY))
	{
		if ((k_info[k_idx].tval == TV_BODY) && (k_info[k_idx].sval == SV_BODY_HAND))
		{
			if (rand_int(100)<50)
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_HAND_L);
			else
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_HAND_R);
		}
		else if ((k_info[k_idx].tval == TV_BODY) && (k_info[k_idx].sval == SV_BODY_ARM))
		{
			if (rand_int(100)<50)
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_ARM_L);
			else
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_ARM_R);
		}
		else if ((k_info[k_idx].tval == TV_BODY) && (k_info[k_idx].sval == SV_BODY_LEG))
		{
			if (rand_int(100)<50)
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_LEG_L);
			else
				k_idx = lookup_kind(TV_ASSEMBLY,SV_ASSEMBLY_LEG_R);
		}

		if (!k_idx) return(FALSE);
	}

	/* Prepare the object */
	object_prep(j_ptr, k_idx);

#if 0
	/* Inscribe it */
	j_ptr->note = r_info[r_idx].note;
#endif
	/* Re-inscribe it */
	if ((!j_ptr->note) && !(r_ptr->flags1 & (RF1_UNIQUE))) j_ptr->note = k_info[k_idx].note;

	/* Modify weight */
	modify_weight(j_ptr,r_idx);

	/* Success */
	return (TRUE);
}

/*
 * Make a body skin
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_skin(object_type *j_ptr, int m_idx)
{
	int k_idx = 0;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Hack -- handle trees */
	if (r_ptr->d_char == '<')
	{
		k_idx = lookup_kind(TV_JUNK,SV_JUNK_STICK);

		if (!k_idx) return (FALSE);

		object_prep(j_ptr,k_idx);

		/* Re-inscribe it */
		if (!j_ptr->note) j_ptr->note = k_info[k_idx].note;

		return (TRUE);
	}

	/* Usually produce a skin */
	if (r_ptr->flags8 & (RF8_HAS_SKIN)) k_idx = lookup_kind(TV_SKIN,SV_SKIN_SKIN);
	else if (r_ptr->flags8 & (RF8_HAS_FUR)) k_idx = lookup_kind(TV_SKIN,SV_SKIN_FURS);
	else if (r_ptr->flags8 & (RF8_HAS_FEATHER)) k_idx = lookup_kind(TV_SKIN,SV_SKIN_FEATHERS);
	else if (r_ptr->flags8 & (RF8_HAS_SCALE)) k_idx = lookup_kind(TV_SKIN,SV_SKIN_SCALES);

	/* No object? */
	if (!k_idx) return (FALSE);

	/* Prepare the object */
	object_prep(j_ptr, k_idx);

#if 0
	/* Inscribe it */
	j_ptr->note = r_info[r_idx].note;
#endif
	/* Re-inscribe it */
	if ((!j_ptr->note) && !(r_ptr->flags1 & (RF1_UNIQUE))) j_ptr->note = k_info[k_idx].note;

	/* Modify weight */
	modify_weight(j_ptr,m_ptr->r_idx);

	/* Success */
	return (TRUE);
}



static int feat_tval;

/*
 * Hack -- determine if a template matches feats_tval.
 */
static bool kind_is_feat_tval(int k_idx)
{

	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->tval != feat_tval) return (FALSE);

	return (TRUE);
}


/*
 * Either get a copy of an existing feat item, or create a new one.
 *
 * If a new feat item is created, place on the floor in the specified location.
 */
bool make_feat(object_type *j_ptr, int y, int x)
{
	feature_type *f_ptr;

	int k_idx;
	int item;

	/* Sanity */
	if (!in_bounds(y, x)) return (0);

	/* Get the feat */
	f_ptr = &f_info[cave_feat[y][x]];

	/* Get the item */
	k_idx = f_ptr->k_idx;

	/* Paranoia */
	if (!k_idx) return (FALSE);

	/* Get existing item */
	item = scan_feat(y,x);

	/* Are we done */
	if (item >= 0)
	{
		object_copy(j_ptr,&o_list[item]);

		return (TRUE);
	}

	/* Hack -- Pick random flavor, if flavored */
	if (f_ptr->flags2 & (FF2_FLAVOR))
	{
		/* Set restriction */
		feat_tval = k_info[k_idx].tval;

		/* Activate restriction */
		get_obj_num_hook = kind_is_feat_tval;

		/* Prepare allocation table */
		get_obj_num_prep();

		/* Pick a random object */
		k_idx = get_obj_num(object_level);

		/* Clear restriction */
		get_obj_num_hook = NULL;

		/* Prepare allocation table */
		get_obj_num_prep();

		/* Failed? */
		if (!k_idx) k_idx = f_ptr->k_idx;
	}

	/* Prepare the object */
	object_prep(j_ptr, k_idx);

	/* This is a 'store' item */
	j_ptr->ident |= (IDENT_STORE);

	/* Mark the origin */
	j_ptr->origin = ORIGIN_FEAT;
	j_ptr->origin_depth = p_ptr->depth;

	/* Hack -- only apply magic to boring objects */
	a_m_aux_4(j_ptr, object_level, 0);

	/* Auto-inscribe if necessary */
	if ((adult_auto) || (object_aware_p(j_ptr))) j_ptr->note = k_info[k_idx].note;

	/* Apply obvious flags */
	object_obvious_flags(j_ptr, TRUE);

	/* Add to the floor */
	if (floor_carry(y,x,j_ptr)) return (TRUE);

	/* Failed */
	return(FALSE);
}


/*
 * Let the floor carry an object
 */
s16b floor_carry(int y, int x, object_type *j_ptr)
{
	int n = 0;

	s16b o_idx;

	s16b this_o_idx, next_o_idx = 0;

	/* Scan objects in that grid for combination */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
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

		/* Count objects */
		n++;
	}

	/* The stack is already too large */
	if (n > MAX_FLOOR_STACK) return (0);

	/* Option -- disallow stacking */
	if (adult_no_stacking && n) return (0);

	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, j_ptr);

		/* Location */
		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Forget monster */
		o_ptr->held_m_idx = 0;

		/* Link the object to the pile */
		o_ptr->next_o_idx = cave_o_idx[y][x];

		/* Link the floor to the object */
		cave_o_idx[y][x] = o_idx;

		/* Window flags */
		p_ptr->window |= (PW_ITEMLIST);

		/* Notice */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* Result */
	return (o_idx);
}


/*
 * Let a monster fall to the ground near a location.
 */
void race_near(int r_idx, int y1, int x1)
{
	int i, x, y;

	/* Try up to 18 times */
	for (i = 0; i < 18; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(&y, &x, y1, x1, d, CAVE_XLOF);

		/* Require an "empty" floor grid */
		if (!cave_empty_bold(y, x)) continue;

		/* Require monster can move unharmed on terrain */
		if (place_monster_here(y, x, r_idx) <= MM_FAIL) continue;

		/* Create a new monster (awake, no groups) */
		(void)place_monster_aux(y, x, r_idx, FALSE, FALSE, 0L);

		/* Hack -- monster does not drop anything */
		m_list[cave_m_idx[y][x]].mflag |= (MFLAG_MADE);

		/* Done */
		break;
	}
}



/*
 * Allow potion specialists to modify potion/flask/lite explosions
 * by inscribing an object with a special formula.
 * This takes the form of pairs of a number followed by a letter
 * following the equals sign. e.g =1r2d3i
 *
 * The letters indicate the effect:
 * a change to arc of 0 - 90 degrees (number times 10).
 * b changes to starburst of 0 - 9 radius.
 * d change damage multiplier from 0 - 9
 * e changes to beam of 0 - 9 radius.
 * f switch PROJECT_GRID flag on (if 1) or off (if 0)
 * h switch PROJECT_HIDE flag on (if 1) or off (if 0)
 * i switch PROJECT_ITEM flag on (if 1) or off (if 0)
 * j switch PROJECT_JUMP flag on (if 1) or off (if 0).
 * k switch PROJECT_KILL & PROJECT_PLAY flag on (if 1) or off (if 0)
 * l switch PROJECT_LITE flag on (if 1) or off (if 0). Overridden by some effects e.g. fire.
 * n explode from 0 - 9 times
 * r change radius from 0 - 9
 * s scatter blast from target by 0 - 9 squares
 * t potion explodes in approximately 0-9 turns (XXX not implemented yet)
 * u switch PROJECT_AREA flag on (if 1) or off (if 0).
 * w change to 8-way blast if number is 8
 *
 * Return FALSE if formula fails. We set power = 0 in calling routine to create a 'dud' effect.
 * Return TRUE if formula not applied, or applied successfully.
 */
bool apply_alchemical_formula(object_type *o_ptr, int *dam, int *rad, int *rng, u32b *flg, int *num, int *deg, int *dia)
{
	cptr s;
	int pow = 1;

	/* No inscription */
	if (!o_ptr->note) return (TRUE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		while ((s[1] >= '0') && (s[1] <= '9'))
		{
			int n;

			n = atoi(s+1);

			switch (s[2])
			{
				case 'a':
				{
					*flg |= (PROJECT_ARC | PROJECT_BOOM);
					*flg &= ~(PROJECT_BEAM | PROJECT_STAR);
					*deg = 10 * n;
					*dia = 15;
					*rad = 10;
					*rng = 5;
					break;
				}
				case 'b':
				{
					*flg |= (PROJECT_STAR | PROJECT_BOOM);
					*flg &= ~(PROJECT_BEAM | PROJECT_ARC);
					*rad = n;
					*rng = 5;
					break;
				}
				case 'c':
				{
					if (n == 4)
					{
						*flg |= (PROJECT_4WAY | PROJECT_BOOM);
						*flg &= ~(PROJECT_ARC | PROJECT_STAR | PROJECT_BEAM);
					}
					break;
				}
				case 'd':
				{
					*dam = n;
					break;
				}
				case 'e':
				{
					*flg |= (PROJECT_BEAM);
					*flg &= ~(PROJECT_ARC | PROJECT_STAR | PROJECT_BOOM);
					*rng = 5;
					*rad = n;
					break;
				}
				case 'f':
				{
					if (n) *flg |= (PROJECT_GRID); else *flg &= ~(PROJECT_GRID);
					break;
				}
				case 'h':
				{
					if (n) *flg |= (PROJECT_HIDE); else *flg &= ~(PROJECT_HIDE);
					break;
				}
				case 'i':
				{
					if (n) *flg |= (PROJECT_ITEM); else *flg &= ~(PROJECT_ITEM);
					break;
				}
				case 'j':
				{
					if (n) { *flg |= (PROJECT_JUMP); *rng = n; } else *flg &= ~(PROJECT_JUMP);
					break;
				}
				case 'k':
				{
					if (n) *flg |= (PROJECT_KILL | PROJECT_PLAY); else *flg &= ~(PROJECT_KILL | PROJECT_PLAY);
					break;
				}
				case 'l':
				{
					if (n) *flg |= (PROJECT_LITE); else *flg &= ~(PROJECT_LITE);
					break;
				}
				case 'm':
				{
					if (n) *flg |= (PROJECT_MYST); else *flg &= ~(PROJECT_MYST);
					break;
				}
				case 'n':
				{
					*num = n;
					break;
				}
				case 'o':
				{
					if (n == 2)
					{
						*flg |= (PROJECT_EDGE | PROJECT_BOOM);
						*flg &= ~(PROJECT_BEAM);
					}
				}
				case 'q':
				{
					if (n) *flg |= (PROJECT_FLOW); else *flg &= ~(PROJECT_FLOW);
					break;
				}
				case 'r':
				{
					*rad = n;
					break;
				}
				case 's':
				{
					*rng = n;
					break;
				}
				case 'u':
				{
					if (n) *flg |= (PROJECT_AREA); else *flg &= ~(PROJECT_AREA);
					break;
				}
				case 'w':
				{
					if (n == 8)
					{
						*flg |= (PROJECT_4WAY | PROJECT_4WAX | PROJECT_BOOM);
						*flg &= ~(PROJECT_ARC | PROJECT_STAR | PROJECT_BEAM);

						pow++;
					}
					break;
				}
				case 'x':
				{
					if (n == 4)
					{
						*flg |= (PROJECT_4WAX | PROJECT_BOOM);
						*flg &= ~(PROJECT_ARC | PROJECT_STAR | PROJECT_BEAM);
					}
					break;
				}
				case 'y':
				{
					if (n) *flg |= (PROJECT_FORK); else *flg &= ~(PROJECT_FORK);
					break;
				}
			}

			/* Increase difficulty */
			pow++;

			/* Check next formula */
			s += 2;
		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Increase power based on effect */
	pow	*= (*dam) * (*rad + 1) * (*num) * ((*deg / 10) + 1) * ((*dia / 5) + 1);

	/* Hack -- project myst is weaker */
	if (*flg & (PROJECT_MYST)) pow = (pow *2)/3;

	/* Hack -- project fork is weaker */
	if (*flg & (PROJECT_FORK)) pow = pow/3;

	/* Hack -- project edge is weaker */
	if ((*flg & (PROJECT_EDGE)) && (*rad > 2)) pow = (pow * 2)/3;

	/* Hack -- project area is powerful */
	if (*flg & (PROJECT_AREA)) pow *= (*rng + 1);

	/* Test against player level. 'Dud' potion if this is true. */
	if ((pow) && (rand_int(pow) > 5 + (p_ptr->lev / 5))) return (FALSE);

	/* Don't auto pickup */
	return (TRUE);
}



/*
 * Break an object near a location. Returns true if actually broken.
 *
 * Used to apply object breakage special effects.
 *
 * Currently only applies for containers, potions, flasks and eggs.
 *
 * XXX We assume all such breakage is player initiated, to prevent
 * smoke forming from thrown oil flasks, and to give the player
 * experience for using spores, oil etc to kill monsters.
 */
bool break_near(object_type *j_ptr, int y, int x)
{
	char o_name[80];

	bool plural = FALSE;

	u32b flg;

	int damage = 0;

	/* Extract the attack infomation */
	int effect;
	int method;
	int d_dice;
	int d_side;
	int d_plus;

	int i;

	bool obvious = FALSE;

	/* Describe object */
	object_desc(o_name, sizeof(o_name), j_ptr, FALSE, 0);

	/* These lose bonuses before breaking */
	switch (j_ptr->tval)
	  {
	  case TV_HAFTED:
	  case TV_POLEARM:
	  case TV_SWORD:
	  case TV_STAFF:
	  case TV_DIGGING:
	    {
	      /* Artifacts have 60% chance to resist disenchantment */
	      if (artifact_p(j_ptr) && (rand_int(100) < 60))
		{
		  return (FALSE);
		}
	      else if (j_ptr->to_h > 0 && j_ptr->to_h >= j_ptr->to_d)
		{
		  j_ptr->to_h--;

		  /* Message */
		  if (!(j_ptr->ident & (IDENT_BREAKS))) msg_format("Your %s bends!", o_name);

		  return (FALSE);
		}
	      else if (j_ptr->to_d > 0)
		{
		  j_ptr->to_d--;

		  /* Message */
		  if (!(j_ptr->ident & (IDENT_BREAKS))) msg_format("Your %s chips!", o_name);

		  return (FALSE);
		}
	    }
	  }

	/* Artifacts do not break */
	if (artifact_p(j_ptr))
	  return (FALSE);

	/* Extract plural */
	if (j_ptr->number != 1) plural = TRUE;

	/* Special case breakages */
	switch (j_ptr->tval)
	{
		/* Containers release contents */
		case TV_HOLD:
		{
			/* Message */
			msg_format("The %s break%s open.",o_name, (plural ? "" : "s"));

			if (j_ptr->name3 > 0)
			{
				while (j_ptr->number)
				{
					race_near(j_ptr->name3, y, x);

					j_ptr->number--;
				}
			}

			return TRUE;
		}

		/* Potions and flasks explode with radius 1. */
		case TV_POTION:
		case TV_FLASK:
		/* Non-artifact lites explode with radius 0 */
		case TV_LITE:
		{
			int power = 0;

			/* The following may all be modifiedy by alchemy */
			int rad = j_ptr->tval == TV_LITE ? 0 : 1;
			int dam = 1;
			int rng = 0;
			int num = 1;
			int j;
			int deg = 0;
			int dia = 10;

			/* Hack -- use power 0 for fake potion effects */
			spell_type *s_ptr;

			/* Initialise flags (may be modified by alchemy) */
			flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAY | PROJECT_BOOM;

			/* Get item effect */
			get_spell(&power, "use", j_ptr, FALSE);

			/* Apply alchemy - if failed, use 'nothing' power */
			if ((p_ptr->pstyle == WS_POTION) && (j_ptr->tval == TV_POTION) && !(apply_alchemical_formula(j_ptr, &dam, &rad, &rng, &flg, &num, &deg, &dia))) power = 0;

			/* Allow power to be 0 if required -- this is used for fake potion effects */
			s_ptr = &s_info[power > 0 ? power : 0];

			/* Applly num times */
			for (j = 0; j < num; j++)
			{
				/* Scan through all four blows */
				for (i = 0; i < 4; i++)
				{
					effect = s_ptr->blow[i].effect;
					method = s_ptr->blow[i].method;
					d_dice = s_ptr->blow[i].d_dice;
					d_side = s_ptr->blow[i].d_side;
					d_plus = s_ptr->blow[i].d_plus;

					/* Hack -- no more attacks. Mega hack - 'fake' the first attack. */
					if (i && !method) break;

					/* Message */
					if (!i && !j)
					{
					  if (power == 0)
					    msg_format("The %s break%s with a fizzling sound.",o_name, (plural ? "" : "s"));
					  else if (!method)
					    msg_format("The %s break%s with a splash.",o_name, (plural ? "" : "s"));
					  else if (j_ptr->tval == TV_LITE)
					    msg_format("The %s burst%s into flames.",o_name, (plural ? "" : "s"));
					  else
					    msg_format("The %s explode%s.",o_name, (plural ? "" : "s"));
					}

					/* Mega hack --- spells in objects */
					if (!d_side)
					{
						d_plus += 25 * d_dice;
					}

					/* Roll out the damage */
					if ((d_dice) && (d_side))
					{
						damage = damroll(d_dice, d_side) + d_plus;
					}
					else
					{
						damage = d_plus;
					}

					/* Hit with projection */
					obvious |= project(SOURCE_PLAYER_BREAK, j_ptr->k_idx, rad, 0, y, x, rand_spread(y, rng), rand_spread(x, rng),
						damage * j_ptr->number * dam, effect, flg, deg, dia);

					/* Object is used */
					if ((obvious) && (k_info[j_ptr->k_idx].used < MAX_SHORT)) k_info[j_ptr->k_idx].used++;
				}
			}

			return TRUE;

			break;

		}

		/* Spores explode with radius 1 effect. */
		/* Eggs turn into bodies. */
		case TV_EGG:
		{
			/* Spores explode */
			if ((j_ptr->sval == SV_EGG_SPORE) && (j_ptr->name3 > 0))
			{
				monster_race *r_ptr = &r_info[j_ptr->name3];
				monster_lore *l_ptr = &l_list[j_ptr->name3];

				/* Scan through all four blows */
				for (i = 0; i < 4; i++)
				{
					effect = r_ptr->blow[i].effect;
					method = r_ptr->blow[i].method;
					d_dice = r_ptr->blow[i].d_dice;
					d_side = r_ptr->blow[i].d_side;

					/* End of attacks */
					if (!method) break;

					/* Skip if not spores */
					if (method != RBM_SPORE) continue;

					/* Message */
					if (!i) msg_format("The %s explode%s.",o_name, (plural ? "" : "s"));

					flg = PROJECT_KILL | PROJECT_PLAY | PROJECT_BOOM;

					/* Hit with radiate attack */
					obvious = project(SOURCE_PLAYER_SPORE, j_ptr->name3, 1, 0, y, x, y, x, damroll(d_side, d_dice) * j_ptr->number,
						 effect, flg, 0, 0);

					/* Count "obvious" attacks */
					if (obvious || (l_ptr->blows[i] > 10))
					{
						/* Count attacks of this type */
						if (l_ptr->blows[i] < MAX_UCHAR)
						{
							l_ptr->blows[i]++;
						}
					}
				}

				if (obvious) return TRUE;

				break;

			}
			else if ((j_ptr->name3 > 0) && (r_info[j_ptr->name3].flags8 & (RF8_HAS_CORPSE)))
			{
				j_ptr->tval = TV_BODY;
				j_ptr->sval = SV_BODY_CORPSE;

				/* Hack - do not adjust weight. Also, if in process of hatching, will 're-animate'. */
				return (FALSE);
			}
		}
	}

	/* Message if not breaking as a result of item destruction */
	if (!(j_ptr->ident & (IDENT_BREAKS))) msg_format("The %s disappear%s.",o_name, (plural ? "" : "s"));

	return TRUE;
}

/*
 * Is item a lite source on the floor?
 */
bool check_object_lite(object_type *j_ptr)
{
#if 0
	/* Is item visible? */
	if (!(j_ptr->ident & (IDENT_MARKED))) return FALSE;
#endif
	/* Is timing out, and a lite? */
	if ((j_ptr->timeout) && (j_ptr->tval == TV_LITE)) return TRUE;

	/* Is timing out, and a lite source */
	else if (j_ptr->timeout)
	{
		u32b f1, f2, f3, f4;

		object_flags(j_ptr, &f1, &f2, &f3, &f4);

		if (f3 & (TR3_LITE)) return (TRUE);
	}

	return FALSE;
}


/*
 * Let an object fall to the ground at or near a location.
 *
 * The initial location is assumed to be "in_bounds_fully()".
 *
 * This function takes a parameter "chance".  This is the percentage
 * chance that the item will "disappear" instead of drop.  If the object
 * has been thrown, then this is the chance of disappearance on contact.
 * 
 * Some special chance values:
 *  0 = no chance of breakage, don't report it if it is dropped under the player
 * -1 = no chance of breakage, report it if it is dropped under the player
 * -2 = no chance of breakage, place it exactly on the square specified
 *
 * Hack -- this function uses "chance" to determine if it should produce
 * some form of "description" of the drop event (under the player).
 *
 * We check several locations to see if we can find a location at which
 * the object can combine, stack, or be placed.  Artifacts will try very
 * hard to be placed, including "teleporting" to a useful grid if needed.
 *
 * XXX We set dont_trigger when a monster dies to avoid their drop
 * triggering a region or projection from the object breaking and
 * potentially killing the monster twice (!?!)
 */
void drop_near(object_type *j_ptr, int chance, int y, int x, bool dont_trigger)
{
	int i, k, d, s;

	int bs, bn;
	int by, bx;
	int dy, dx;
	int ty, tx;

	s16b this_o_idx, next_o_idx = 0;

	char o_name[80];

	bool flag = FALSE;

	bool plural = FALSE;

	/* Extract plural */
	if (j_ptr->number != 1) plural = TRUE;

	/* Describe object */
	object_desc(o_name, sizeof(o_name), j_ptr, FALSE, 0);

	/* Handle normal "breakage" */
	if (rand_int(100) < chance)
	  {
	    if (!dont_trigger && break_near(j_ptr, y, x))
	      return;
	  }

	/* Score */
	bs = -1;

	/* Picker */
	bn = 0;

	/* Default */
	by = y;
	bx = x;

	/* Scan local grids */
	for (dy = -3; dy <= 3; dy++)
	{
		/* Scan local grids */
		for (dx = -3; dx <= 3; dx++)
		{
			bool comb = FALSE;

			/* Calculate actual distance */
			d = (dy * dy) + (dx * dx);

			/* Ignore distant grids */
			if (d > 10) continue;

			/* Location */
			ty = y + dy;
			tx = x + dx;

			/* Skip illegal grids */
			if (!in_bounds_fully(ty, tx)) continue;

			/* Require drop space */
			if ((f_info[cave_feat[ty][tx]].flags1 & (FF1_DROP)) == 0) continue;

			/* Requires terrain that won't destroy it */
			if ((chance <= 0) && hates_terrain(j_ptr, cave_feat[ty][tx])) continue;
	
			/* No objects */
			k = 0;

			/* Scan objects in that grid */
			for (this_o_idx = cave_o_idx[ty][tx]; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;

				/* Get the object */
				o_ptr = &o_list[this_o_idx];

				/* Get the next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Count objects */
				k++;

				/* Check for possible combination */
				if (object_similar(o_ptr, j_ptr)) comb = TRUE;
			}

			/* Add new object */
			if (!comb) k++;

			/* Option -- disallow stacking */
			if (adult_no_stacking && (k > 1)) continue;

			/* Paranoia */
			if (k > MAX_FLOOR_STACK) continue;

			/* Calculate score */
			s = 10000 - (d + k * 5);

			/* Don't like hiding items space */
			if (f_info[cave_feat[ty][tx]].flags2 & (FF2_HIDE_ITEM))
				s = s / 2;

			/* Don't like lack of  line of fire */
			if (!generic_los(y, x, ty, tx, CAVE_XLOF))
				s = s / 4;

			/* We are placing an object exactly */
			if ((chance == -2) && (ty == y) && (tx == x)) s = 20000;
			
			/* Skip bad values */
			if (s < bs) continue;

			/* New best value */
			if (s > bs) bn = 0;

			/* Apply the randomizer to equivalent values */
			if ((++bn >= 2) && (rand_int(bn) != 0)) continue;

			/* Keep score */
			bs = s;

			/* Track it */
			by = ty;
			bx = tx;

			/* Okay */
			flag = TRUE;
		}
	}


	/* Handle lack of space */
	if (!flag && !artifact_p(j_ptr))
	{
		/* Message */
		if ((character_dungeon) || (cheat_peek)) msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));

		/* Debug */
		if (p_ptr->wizard) msg_print("Breakage (no floor space).");

		/* Failure */
		return;
	}


	/* Find a grid */
	for (i = 0; !flag; i++)
	{
		/* Bounce around */
		if (i < 1000)
		{
			ty = rand_spread(by, 1);
			tx = rand_spread(bx, 1);
		}

		/* Random locations */
		else
		{
			ty = rand_int(DUNGEON_HGT);
			tx = rand_int(DUNGEON_WID);
		}

		/* Bounce to that location */
		by = ty;
		bx = tx;

		/* Require floor space */
		if (!cave_clean_bold(by, bx)) continue;

		/* Okay */
		flag = TRUE;
	}


	/* Give it to the floor */
	if (!floor_carry(by, bx, j_ptr))
	{
		/* Message */
		if ((character_dungeon) || (cheat_peek)) msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));

		/* Debug */
		if (p_ptr->wizard) msg_print("Breakage (too many objects).");

		/* Hack -- Preserve artifacts */
		a_info[j_ptr->name1].cur_num = 0;

		/* Failure */
		return;
	}

	/* Warn if we lose the item from view */
	if (f_info[cave_feat[by][bx]].flags2 & (FF2_HIDE_ITEM))
	{
		/* Clear visibility */
		j_ptr->ident &= ~(IDENT_MARKED);

		/* Skip message on auto-ignored items */
		if ((character_dungeon) && (!auto_pickup_ignore(j_ptr)))
		{
			/* Message */
			if ((character_dungeon) || (cheat_peek)) msg_format("The %s disappear%s from view.",
				   o_name, (plural ? "" : "s"));
		}
	}

	/* Gain lite? */
	if (check_object_lite(j_ptr))
	{
		/* Message */
		if ((character_dungeon) || (cheat_peek)) msg_format("The %s light%s up the surroundings.", o_name, (plural ? "" : "s"));

		gain_attribute(by, bx, 2, CAVE_XLOS, apply_halo, redraw_halo_gain);
	}
	
	/* Sound */
	sound(MSG_DROP);

	/* Mega-Hack -- no message if "dropped" by player */
	/* Message when an object falls under the player */
	if (chance && (cave_m_idx[by][bx] < 0))
	{
		/* Skip message on auto-ignored items */
		if ((!auto_pickup_ignore(j_ptr)) && ((character_dungeon) || (cheat_peek)))
		{
			msg_print("You feel something roll beneath your feet.");
		}
	}

	/* Trigger regions. */
	if (!dont_trigger) trigger_region(by, bx, FALSE);
	
	/* Hates terrain */
	if ((!dont_trigger) && (hates_terrain(j_ptr, cave_feat[by][bx])))
	{
		feature_type *f_ptr = &f_info[cave_feat[by][bx]];
		
		/* Hack -- this affects all objects in the grid */
		/* We can't use project_o directly because objects get marked for later breakage */
		project_one(SOURCE_FEATURE, f_ptr->mimic, p_ptr->py, p_ptr->px,damroll(f_ptr->blow.d_side,f_ptr->blow.d_dice), f_ptr->blow.effect, (PROJECT_ITEM | PROJECT_HIDE));
	}
}


/*
 * Let an feature appear near a location.
 *
 * The initial location is assumed to be "in_bounds_fully()".
 *
 */
void feat_near(int feat, int y, int x)
{
	int d, s;

	int bs, bn;
	int by, bx;
	int dy, dx;
	int ty, tx;

	bool flag = FALSE;

	/* Score */
	bs = -1;

	/* Picker */
	bn = 0;

	/* Default */
	by = y;
	bx = x;

	/* Scan local grids */
	for (dy = -3; dy <= 3; dy++)
	{
		/* Scan local grids */
		for (dx = -3; dx <= 3; dx++)
		{
			/* Calculate actual distance */
			d = (dy * dy) + (dx * dx);

			/* Ignore distant grids */
			if (d > 10) continue;

			/* Location */
			ty = y + dy;
			tx = x + dx;

			/* Skip illegal grids */
			if (!in_bounds_fully(ty, tx)) continue;

			/* Require line of fire */
			if (!generic_los(y, x, ty, tx, CAVE_XLOF)) continue;

			/* Prevent overwriting permanets */
			if (f_info[cave_feat[ty][tx]].flags1 & (FF1_PERMANENT)) continue;

			/* Don't like non-floor space */
			if (!(f_info[cave_feat[ty][tx]].flags1 & (FF1_FLOOR))) continue;

			/* Don't like objects */
			if (cave_o_idx[ty][tx]) continue;

			/* Calculate score */
			s = 1000 - (d - cave_feat[ty][tx]);

			/* Skip bad values */
			if (s < bs) continue;

			/* New best value */
			if (s > bs) bn = 0;

			/* Apply the randomizer to equivalent values */
			if ((++bn >= 2) && (rand_int(bn) != 0)) continue;

			/* Keep score */
			bs = s;

			/* Track it */
			by = ty;
			bx = tx;

			/* Okay */
			flag = TRUE;
		}
	}

	/* Give it to the floor */
	if (flag) cave_set_feat(by, bx, feat);
}

/*
 * Scatter some "great" objects near the player
 */
void acquirement(int y1, int x1, int num, bool great)
{
	object_type *i_ptr;
	object_type object_type_body;

	/* Acquirement */
	while (num--)
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Make a good (or great) object (if possible) */
		if (!make_object(i_ptr, TRUE, great)) continue;

		i_ptr->origin = ORIGIN_ACQUIRE;
		i_ptr->origin_depth = p_ptr->depth;

		/* Drop the object */
		drop_near(i_ptr, -1, y1, x1, TRUE);
	}
}


/*
 * Attempt to place an object (normal or good/great) at the given location.
 */
void place_object(int y, int x, bool good, bool great)
{
	object_type *i_ptr;
	object_type object_type_body;

	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Hack -- clean floor space */
	if (!cave_clean_bold(y, x)) return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make an object (if possible) */
	if (make_object(i_ptr, good, great))
	{
		i_ptr->origin = ORIGIN_FLOOR;
		i_ptr->origin_depth = p_ptr->depth;

		/* Give it to the floor */
		if (!floor_carry(y, x, i_ptr))
		{
			/* Hack -- Preserve artifacts */
			a_info[i_ptr->name1].cur_num = 0;
		}
	}
}


/*
 * Places a treasure (Gold or Gems) at given location
 */
void place_gold(int y, int x)
{
	object_type *i_ptr;
	object_type object_type_body;

	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Require clean floor space */
	if (!cave_clean_bold(y, x)) return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make some gold */
	if (make_gold(i_ptr, FALSE, FALSE))
	{
		/* Give it to the floor */
		(void)floor_carry(y, x, i_ptr);
	}
}


/*
 * Apply a "feature restriction function" to the "feature allocation table"
 */
errr get_feat_num_prep(void)
{
	int i;

	/* Get the entry */
	alloc_entry *table = alloc_feat_table;

	/* Scan the allocation table */
	for (i = 0; i < alloc_feat_size; i++)
	{
		/* Accept objects which pass the restriction, if any */
		if (!get_feat_num_hook || (*get_feat_num_hook)(table[i].index))
		{
			/* Accept this object */
			table[i].prob2 = table[i].prob1;
		}

		/* Do not use this object */
		else
		{
			/* Decline this object */
			table[i].prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/*
 * Choose an feature type that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "feature allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" feature, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire a feature of the given level
 * than one of a lower level.  This is done by choosing several features
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no features are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 * Happening all the time for trapped doors.
 */
s16b get_feat_num(int level)
{
	int i, j, p;

	int f_idx;

	long value, total;

	feature_type *f_ptr;

	alloc_entry *table = alloc_feat_table;


	/* Boost level */
	if (level > 0)
	{
		/* Occasional "boost" */
		if (rand_int(GREAT_OBJ) == 0)
		{
#if 0
			/* What a bizarre calculation */
			level = 1 + (level * MAX_DEPTH / randint(MAX_DEPTH));
#endif

			/* 4-7 levels boost */
			level += (4 + rand_int(4));
		}
	}


	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_feat_size; i++)
	{
		/* Default */
		table[i].prob3 = 0;

		/* Features are not sorted by depth */
		if (table[i].level > level) break;

		/* Get the index */
		f_idx = table[i].index;

		/* Get the actual feature */
		f_ptr = &f_info[f_idx];

		/* Hack -- restrict stairs */
		if ((f_ptr->flags1 & (FF1_LESS)) && !(level_flag & (LF1_LESS))) continue;
		if ((f_ptr->flags1 & (FF1_MORE)) && !(level_flag & (LF1_MORE))) continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal features */
	if (total <= 0) return (0);


	/* Pick a feature */
	value = rand_int(total);

	/* Find the feature */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = rand_int(100);

	/* Try for a "better" object once (50%) or twice (10%) */
	if (p < 60)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the monster */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = rand_int(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}


	/* Result */
	return (table[i].index);
}


/*
 * Helper function for "features"
 */
static bool vault_feat_alloc(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Require random allocation */
	if (!(f_ptr->flags3 & (FF3_ALLOC))) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Places a random 'interesting feature' at the given location.
 *
 * The location must be a legal, naked, floor grid.
 */
void place_feature(int y, int x)
{
	int feat;

	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Require empty, clean, floor grid */
	if (!cave_naked_bold(y, x)) return;

	/*Set the hook */
	get_feat_num_hook = vault_feat_alloc;

	get_feat_num_prep();

	/* Click! */
	feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* More paranoia */
	if (!feat) return;

	/* Activate the trap */
	cave_set_feat(y, x, feat);
}


/*
 * Helper function for "floor traps"
 */
static bool vault_trap_floor(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Require trap */
	if (!(f_ptr->flags1 & (FF1_TRAP))) return (FALSE);

	/* Decline door traps */
	if (f_ptr->flags1 & (FF1_DOOR)) return (FALSE);

	/* Decline chest traps */
	if (f_ptr->flags3 & (FF3_CHEST)) return (FALSE);

	/* Decline traps we have to pick */
	if (f_ptr->flags3 & (FF3_PICK_TRAP)) return (FALSE);

	/* Decline allocated */
	if (f_ptr->flags3 & (FF3_ALLOC)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/* Hack -- make sure we drop the same items */
static int chest_drops;

static bool chest_drop_good;
static bool chest_drop_great;
static bool chest_has_item;
static bool chest_has_gold;

/*
 * Helper function for "chest traps"
 */
static bool vault_trap_chest(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	int test_drops =  ((f_ptr->flags3 & (FF3_DROP_1D3)) ? 3 : 0) + ((f_ptr->flags3 & (FF3_DROP_1D2)) ? 2 : 0);
	bool test_drop_good = (f_ptr->flags3 & (FF3_DROP_GOOD)) ? TRUE : FALSE;
	bool test_drop_great = (f_ptr->flags3 & (FF3_DROP_GREAT)) ? TRUE : FALSE;
	bool test_has_item = (f_ptr->flags1 & (FF1_HAS_ITEM)) ? TRUE : FALSE;
	bool test_has_gold = (f_ptr->flags1 & (FF1_HAS_GOLD)) ? TRUE : FALSE;

	/* Decline non-chests */
	if (!(f_ptr->flags3 & (FF3_CHEST))) return (FALSE);

	/* Decline non-traps */
	if (!(f_ptr->flags1 & (FF1_TRAP))) return (FALSE);

	/* Decline allocated */
	if (f_ptr->flags3 & (FF3_ALLOC)) return (FALSE);

	/* Must match chest drops */
	if (test_drop_good != chest_drop_good) return (FALSE);
	if (test_drop_great != chest_drop_great) return (FALSE);
	if (test_has_item != chest_has_item) return (FALSE);
	if (test_has_gold != chest_has_gold) return (FALSE);
	if (test_drops != chest_drops) return (FALSE);

	/* Okay */
	return (TRUE);
}

/*
 * Helper function for "chests"
 */
static bool vault_chest(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Require alloc flag */
	if (!(f_ptr->flags3 & (FF3_ALLOC))) return (FALSE);

	/* Require chest flag */
	if (!(f_ptr->flags3 & (FF3_CHEST))) return (FALSE);

	/* Not okay */
	return (TRUE);
}




/* Hack -- global */
static char pick_attr;

/*
 * Helper function for "matched trap"
 */
static bool vault_trap_attr(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Decline non-traps */
	if (!(f_ptr->flags1 & (FF1_TRAP))) return (FALSE);

	/* Decline door traps */
	if (f_ptr->flags1 & (FF1_DOOR)) return (FALSE);

	/* Decline chest traps */
	if (f_ptr->flags3 & (FF3_CHEST)) return (FALSE);

	/* Decline traps we have to pick */
	if (f_ptr->flags3 & (FF3_PICK_TRAP)) return (FALSE);

	/* Restrict traps to same color */
	if (f_ptr->d_attr != pick_attr) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Create trap region
 */
int create_trap_region(int y, int x, int feat, int power, bool player)
{
	/* Create region associated with trap */
	feature_type *f_ptr = &f_info[feat];

	int ty = 0;
	int tx = 0;
	int dir = 0;
	int region;

	region_info_type *ri_ptr = &region_info[f_ptr->d_attr];

	int method = ri_ptr->method;
	int effect = 0; /* Not needed - we use discharge_trap code */

	int damage = 0; /* Not needed - we use discharge_trap code */

	method_type *method_ptr = &method_info[method];
	u32b flg = method_ptr->flags1;

	int radius = scale_method(method_ptr->radius, player ? p_ptr->lev : p_ptr->depth);

	/* Paranoia */
	if (!method) return (0);

	/* Paranoia */
	if (effect == GF_FEATURE) effect = 0;

	/* Not needed at the moment */
	(void)power;

	/* Player is setting a trap */
	if (player)
	{
		if (((flg & (PROJECT_SELF)) == 0) &&
				(!get_aim_dir(&dir, TARGET_KILL, MAX_RANGE, radius, flg, method_ptr->arc, method_ptr->diameter_of_source)))
						return (0);

		/* Use the given direction */
		ty = y + ddy[dir];
		tx = x + ddx[dir];

		/* Hack -- Use an actual "target" */
		if ((dir == 5) && target_okay())
		{
			ty = p_ptr->target_row;
			tx = p_ptr->target_col;
		}
		/* Stop at first target if we're firing in a direction */
		else if (method_ptr->flags2 & (PR2_DIR_STOP))
		{
			flg |= (PROJECT_STOP);
		}
	}

	/* Get the region */
	region = init_region(player ? SOURCE_PLAYER_TRAP : SOURCE_FEATURE, feat, f_ptr->d_attr, damage, method, effect,
					player ? p_ptr->lev : p_ptr->depth, y, x, ty, tx);

	/* Add to it */
	if (region)
	{
		region_type *r_ptr = &region_list[region];

		/* Display if player */
		if (player || player_can_see_bold(y,x))
		{
			r_ptr->flags1 |= (RE1_DISPLAY  | RE1_NOTICE);
		}

		/* Shape the region */
		project_method(player ? SOURCE_PLAYER_TRAP : SOURCE_FEATURE, feat, method, effect, damage,
			player ? p_ptr->lev : p_ptr->depth, y, x, r_ptr->y1, r_ptr->x1, region, flg);
	}

	return (region);
}


/*
 * Hack -- instantiate a trap
 *
 * Have modified this routine to use the modified feature selection
 * code above.
 */
void pick_trap(int y, int x, bool player)
{
	int feat= cave_feat[y][x];
	int room = room_idx(y, x);
	feature_type *f_ptr = &f_info[feat];
	int region = 0;

	int power = 0;
	int i;

	/* Paranoia */
	if (!(f_ptr->flags1 & (FF1_TRAP))) return;

	/* 'Pre-trap' region already exists */
	/* XXX We can't just check the grid, because it's highly likely that the source trap
	 * for a region won't affect the grid it's in.
	 */
	for (i = 0; i < region_max; i++)
	{
		/* Get this effect */
		region_type *r_ptr = &region_list[i];

		/* Skip empty effects */
		if (!r_ptr->type) continue;

		/* Skip regions with differing sources */
		if ((r_ptr->y0 != y) || (r_ptr->x0 != x)) continue;

		/* Skip non-trap regions */
		if ((r_ptr->flags1 & (RE1_HIT_TRAP)) == 0) continue;

		/* We have a region */
		region = i;

		/* Use this to pick the trap later */
		pick_attr = f_ptr->d_attr;

		break;
	}

	/* Region found */
	if (region)
	{
		/* Set hook*/
		get_feat_num_hook = vault_trap_attr;
	}

	/* Floor trap */
	else if (f_ptr->flags3 & (FF3_ALLOC))
	{
		/* Set hook */
		if (f_ptr->flags3 & (FF3_CHEST))
		{
			feature_type *f_ptr = &f_info[feat];

			chest_drops =  ((f_ptr->flags3 & (FF3_DROP_1D3)) ? 3 : 0) + ((f_ptr->flags3 & (FF3_DROP_1D2)) ? 2 : 0);
			chest_drop_good = (f_ptr->flags3 & (FF3_DROP_GOOD)) ? TRUE : FALSE;
			chest_drop_great = (f_ptr->flags3 & (FF3_DROP_GREAT)) ? TRUE : FALSE;
			chest_has_item = (f_ptr->flags1 & (FF1_HAS_ITEM)) ? TRUE : FALSE;
			chest_has_gold = (f_ptr->flags1 & (FF1_HAS_GOLD)) ? TRUE : FALSE;

			get_feat_num_hook = vault_trap_chest;
		}
		else if (cave_o_idx[y][x])
		{
			object_type *o_ptr = &o_list[cave_o_idx[y][x]];

			bool need_power = TRUE;

			switch (o_ptr->tval)
			{
				case TV_SHOT:
				case TV_ARROW:
				case TV_BOLT:
				case TV_BOW:
					need_power = FALSE;
					pick_attr = TERM_L_RED;		/* Murder hole */
					break;

				case TV_HAFTED:
				case TV_SWORD:
				case TV_POLEARM:
					need_power = FALSE;
					pick_attr = TERM_RED;		/* Spring-loaded trap */
					break;

				case TV_WAND:
					pick_attr = TERM_YELLOW;	/* Tripwire */
					break;

				case TV_STAFF:
					pick_attr = TERM_L_BLUE;	/* Magic symbol */
					break;

				case TV_ASSEMBLY:
				case TV_ROD:
					pick_attr = TERM_MUSTARD;	/* Clockwork mechanism */
					break;

				case TV_POTION:
					pick_attr = TERM_BLUE;		/* Explosive device */
					break;

				case TV_SCROLL:
					pick_attr = TERM_L_GREEN;	/* Ancient hex */
					break;

				case TV_FLASK:
					pick_attr = TERM_UMBER;		/* Discoloured spot */
					break;

				case TV_DRAG_ARMOR:
					pick_attr = TERM_L_WHITE;	/* Stone visage */
					break;

				case TV_MUSHROOM:
					pick_attr = TERM_GREEN;		/* Gas trap */
					break;

				case TV_RUNESTONE:
					pick_attr = TERM_ORANGE;	/* Strange rune */
					break;

				case TV_SPIKE:
					pick_attr = TERM_SLATE;	/* Pit */
					break;

				case TV_ROPE:
					pick_attr = TERM_L_UMBER;	/* Fine net */
					break;

				case TV_JUNK:
					pick_attr = TERM_MUD;	/* Dead fall */
					break;

				case TV_BAG:
					pick_attr = TERM_DEEP_L_BLUE; /* Shimmering portal */
					break;

				case TV_LITE:
					pick_attr = TERM_L_YELLOW;	/* Shaft of light */
					break;

				case TV_STUDY:
					pick_attr = TERM_MAGENTA;	/* Glowing glyph */
					break;

				case TV_STATUE:
					pick_attr = TERM_PURPLE;	/* Surreal painting */
					break;

				case TV_RING:
					pick_attr = TERM_VIOLET;	/* Ever burning eye  */
					break;

				case TV_AMULET:
					pick_attr = TERM_L_TEAL;	/* Demonic sign */
					break;

				case TV_INSTRUMENT:
					pick_attr = TERM_TEAL;		/* Upwards draft */
					break;

				case TV_HOLD:
					pick_attr = TERM_L_VIOLET;	/* Radagast's snare */
					break;

				case TV_HELM:
					pick_attr = TERM_L_DARK;	/* Silent watcher */
					break;

				case TV_GLOVES:
					pick_attr = TERM_BLUE_SLATE;	/* Mark of the white hand */
					break;

				default:
					pick_attr = TERM_L_PURPLE;	/* Loose rock */
					break;

				/* nothing
				 *
				 * pick_attr = TERM_L_PINK;		Siege engine
				 * break;
				 */
			}

			/* Set hook*/
			get_feat_num_hook = vault_trap_attr;

			/* Get item effect */
			if (need_power) get_spell(&power, "use", o_ptr, FALSE);
		}
		/* Picking a floor trap */
		else
		{
			get_feat_num_hook = vault_trap_floor;
		}
	}
	else
	{
		/* Set attribute */
		pick_attr = f_ptr->d_attr;

		/* Set hook*/
		get_feat_num_hook = vault_trap_attr;
	}

	/* Room has specific trap type associated with it. Use it if possible. */
	if ((room_info[room].theme[THEME_TRAP]) && ((get_feat_num_hook == vault_trap_floor) ||
			((get_feat_num_hook == vault_trap_attr) && (pick_attr == f_info[room_info[room].theme[THEME_TRAP]].d_attr) )))
	{
		/* Set the trap if in a room which has a trap set */
		feat = room_info[room].theme[THEME_TRAP];
	}
	/* Pick a trap */
	else
	{
		get_feat_num_prep();

		/* Hack --- force dungeon traps in town */
		if (!p_ptr->depth) object_level = 3;

		/* Click! */
		feat = get_feat_num(object_level);

		/* Hack --- force dungeon traps in town */
		if (!p_ptr->depth) object_level = 0;

		/* Clear the hook */
		get_feat_num_hook = NULL;

		get_feat_num_prep();

		/* More paranoia */
		if (!feat) return;
	}

	/* Activate the trap */
	cave_set_feat(y, x, feat);

	/* Get feature */
	f_ptr = &f_info[feat];

	/* Create trap region */
	if ((!region) && (power || (f_ptr->blow.method) || (f_ptr->spell)))
	{
		region = create_trap_region(y, x, feat, power, player);
	}

	/* Ensure trap region is visible */
	if ((region) && ((f_ptr->flags1 & (FF1_SECRET)) == 0))
	{
		region_list[region].flags1 |= (RE1_NOTICE | RE1_DISPLAY);
	}
}


/*
 * Places a random trap at the given location.
 *
 * The location must be a legal, naked, floor grid.
 *
 * Note that all traps start out as "invisible" and "untyped", and then
 * when they are "discovered" (by detecting them or setting them off),
 * the trap is "instantiated" as a visible, "typed", trap.
 */
void place_trap(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Require empty, clean, floor grid */
	if (!cave_naked_bold(y, x)) return;

	/* Place an invisible trap */
	cave_set_feat(y, x, FEAT_INVIS);
}


/*
 * 'Makes' a chest
 */
bool make_chest(int *feat)
{
	/*Set the hook */
	get_feat_num_hook = vault_chest;

	get_feat_num_prep();

	/* Click! */
	*feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* More paranoia */
	if (!feat) return (FALSE);

	return(TRUE);
}

/*
 * Places a random chest at the given location.
 *
 * The location must be a legal, naked, floor grid.
 */
void place_chest(int y, int x)
{
	int feat;

	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Require empty, clean, floor grid */
	if (!cave_naked_bold(y, x)) return;

	/*Set the hook */
	get_feat_num_hook = vault_chest;

	get_feat_num_prep();

	/* Click! */
	feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();


	/* More paranoia */
	if (!feat) return;

	/* Activate the trap */
	cave_set_feat(y, x, feat);
}


/*
 * Pick a door
 */
void pick_door(int y, int x)
{
	int feat = cave_feat[y][x];

	if (feat == FEAT_DOOR_INVIS) place_trapped_door(y,x);
	else if (feat == FEAT_SECRET) place_secret_door(y,x);
	else place_jammed_door(y,x);
}


/*
 * Place a secret door at the given location
 */
void place_secret_door(int y, int x)
{
	/* Create secret door */
	cave_set_feat(y, x, FEAT_SECRET);
}

/*
 * Helper function for "closed doors"
 */
static bool vault_closed_door(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Decline non-door */
	if (!(f_ptr->flags1 & (FF1_DOOR))) return (FALSE);

	/* Decline known trapped doors */
	if ((f_ptr->flags1 & (FF1_TRAP)) && !(f_ptr->flags1 & (FF1_SECRET))) return (FALSE);

	/* Decline open/secret/known jammed doors/broken */
	if (!(f_ptr->flags1 & (FF1_OPEN))) return (FALSE);

	/* Okay */
	return (TRUE);
}

/*
 * Place a random type of closed door at the given location.
 */
void place_closed_door(int y, int x)
{
	int feat;

	if (randint(400) < 300)
	{
		/* Create closed door */
		cave_set_feat(y, x, FEAT_DOOR_CLOSED);

		return;
	}
	else
	{
		/*Set the hook */
		get_feat_num_hook = vault_closed_door;

		get_feat_num_prep();

		/* Click! */
		feat = get_feat_num(object_level);

		/* Clear the hook */
		get_feat_num_hook = NULL;

		get_feat_num_prep();

		/* More paranoia */
		if (!feat) return;

		/* Activate the trap */
		cave_set_feat(y, x, feat);

	}

}

/*
 * Helper function for "closed doors"
 */
static bool vault_locked_door(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Decline non-door */
	if (!(f_ptr->flags1 & (FF1_DOOR))) return (FALSE);

	/* Decline known trapped doors */
	if ((f_ptr->flags1 & (FF1_TRAP)) && !(f_ptr->flags1 & (FF1_SECRET))) return (FALSE);

	/* Decline open/secret/known jammed doors/broken */
	if (!(f_ptr->flags1 & (FF1_OPEN))) return (FALSE);

	/* Decline unlocked doors */
	if (f_idx == FEAT_DOOR_CLOSED) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Place a random type of trapped/locked door at the given location
 */
void place_locked_door(int y, int x)
{
	int feat;

	/*Set the hook */
	get_feat_num_hook = vault_locked_door;

	get_feat_num_prep();

	/* Click! */
	feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* More paranoia */
	if (!feat) return;

	/* Activate the trap */
	cave_set_feat(y, x, feat);
}

/*
 * Helper function for "closed doors"
 */
static bool vault_jammed_door(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Decline non-door */
	if (!(f_ptr->flags1 & (FF1_DOOR))) return (FALSE);

	/* Decline trapped doors */
	if (f_ptr->flags1 & (FF1_TRAP)) return (FALSE);

	/* Decline unjammed doors */
	if (f_ptr->flags1 & (FF1_OPEN)) return (FALSE);

	/* Decline secret/unknown jammed doors */
	if (f_ptr->flags1 & (FF1_SECRET)) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Place a random type of jammed door at the given location.
 */
void place_jammed_door(int y, int x)
{
	int feat;

	/*Set the hook */
	get_feat_num_hook = vault_jammed_door;

	get_feat_num_prep();

	/* Click! */
	feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* More paranoia */
	if (!feat) return;

	/* Activate the trap */
	cave_set_feat(y, x, feat);
}


/*
 * Helper function for "door traps"
 */
static bool vault_trapped_door(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Decline non-doors */
	if (!(f_ptr->flags1 & (FF1_DOOR))) return (FALSE);

	/* Decline non-traps */
	if (!(f_ptr->flags1 & (FF1_TRAP))) return (FALSE);

	/* Decline pick doors */
	if (f_ptr->flags3 & (FF3_PICK_DOOR)) return (FALSE);

	/* Okay */
	return (TRUE);
}

/*
 * Place a random type of trapped door at the given location
 */
void place_trapped_door(int y, int x)
{
	int feat;

	/* Set the hook */
	get_feat_num_hook = vault_trapped_door;

	get_feat_num_prep();

	/* Click! */
	feat = get_feat_num(object_level);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* More paranoia */
	if (!feat) return;

	/* Activate the trap */
	cave_set_feat(y, x, feat);
}

/*
 * Place a random type of door at the given location.
 */
void place_random_door(int y, int x)
{
	int tmp;

	/* Choose an object */
	tmp = rand_int(1000);

	/* Open doors (300/1000) */
	if (tmp < 300)
	{
		/* Create open door */
		cave_set_feat(y, x, FEAT_OPEN);
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400)
	{
		/* Create broken door */
		cave_set_feat(y, x, FEAT_BROKEN);
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600)
	{
		/* Create secret door */
		cave_set_feat(y, x, FEAT_SECRET);
	}

	/* Closed, locked, or stuck doors (400/1000) */
	else
	{
		/* Create closed door */
		place_closed_door(y, x);
	}
}

/*
 * Describe the charges on an item in the inventory.
 */
void inven_item_charges(int item)
{
	object_type *o_ptr = &inventory[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	if (!object_charges_p(o_ptr)) return;

	/* Multiple charges */
	if (o_ptr->charges != 1)
	{
		/* Print a message */
		msg_format("You have %d charges remaining.", o_ptr->charges);
	}

	/* Single charge */
	else
	{
		/* Print a message */
		msg_format("You have %d charge remaining.", o_ptr->charges);
	}
}


/*
 * Describe an item in the inventory.
 */
void inven_item_describe(int item)
{
	object_type *o_ptr = &inventory[item];

	char o_name[80];

	/* Get a description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Print a message */
	msg_format("You have %s (%c).", o_name, index_to_label(item));
}


/*
 * Increase the "number" of an item in the inventory
 */
void inven_item_increase(int item, int num)
{
	object_type *o_ptr = &inventory[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number and weight */
	if (num)
	{
		/* Add the number */
		o_ptr->number += num;

		/* Check stack count overflow */
		if (o_ptr->stackc >= o_ptr->number)
		{
			/* Reset stack counter */
			o_ptr->stackc = 0;

			/* Decrease charges */
			if ((o_ptr->charges) && (o_ptr->number > 0)) o_ptr->charges--;

			/* Reset timeout */
			if (o_ptr->timeout) o_ptr->timeout = 0;
		}

		/* Add the weight */
		p_ptr->total_weight += (num * o_ptr->weight);

		/* Update "p_ptr->pack_size_reduce" */
		if (IS_QUIVER_SLOT(item)) find_quiver_size();

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}
}


/*
 * Erase an inventory slot if it has no more items
 */
void inven_item_optimize(int item)
{
	object_type *o_ptr = &inventory[item];

	int i;

	/* Only optimize real items */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* The item is in the pack */
	if (item < INVEN_WIELD)
	{
		/* Hack - bags take up 2 slots. We calculate this here. */
		if (o_ptr->tval == TV_BAG) p_ptr->pack_size_reduce_bags--;

		/* One less item */
		p_ptr->inven_cnt--;

		/* Slide everything down */
		for (i = item; i < INVEN_PACK; i++)
		{
			/* Hack -- slide object */
			COPY(&inventory[i], &inventory[i+1], object_type);
		}

		/* Hack -- wipe hole */
		object_wipe(&inventory[i]);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_ITEM_LIST);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN);
	}

	/* The item is being wielded */
	else
	{
		/* Reorder the quiver if necessary */
		if (IS_QUIVER_SLOT(item))
		  p_ptr->notice |= (PN_REORDER);
		else
		  /* One less item */
		  p_ptr->equip_cnt--;

		/* Erase the empty slot */
		object_wipe(&inventory[item]);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate torch */
		p_ptr->update |= (PU_TORCH);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_ITEM_LIST);
	}
}


/*
 * Describe the charges on an item on the floor.
 */
void floor_item_charges(int item)
{
	object_type *o_ptr = &o_list[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	if (!object_charges_p(o_ptr)) return;

	/* Multiple charges */
	if (o_ptr->charges != 1)
	{
		/* Print a message */
		msg_format("There are %d charges remaining.", o_ptr->charges);
	}

	/* Single charge */
	else
	{
		/* Print a message */
		msg_format("There is %d charge remaining.", o_ptr->charges);
	}
}



/*
 * Describe an item in the inventory.
 */
void floor_item_describe(int item)
{
	object_type *o_ptr = &o_list[item];

	char o_name[80];

	/* Hack -- haven't seen item on floor */
	if ((o_ptr->ident & (IDENT_MARKED)) == 0) return;

	/* Get a description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Print a message */
	msg_format("You see %s.", o_name);
}


/*
 * Increase the "number" of an item on the floor
 */
void floor_item_increase(int item, int num)
{
	object_type *o_ptr = &o_list[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number */
	o_ptr->number += num;

	/* Check stack count overflow */
	if (o_ptr->stackc >= o_ptr->number)
	{
		/* Reset stack counter */
		o_ptr->stackc = 0;

		/* Decrease charges */
		if (o_ptr->charges) o_ptr->charges--;

		/* Reset timeout */
		if (o_ptr->timeout) o_ptr->timeout = 0;
	}

}


/*
 * Optimize an item on the floor (destroy "empty" items)
 */
void floor_item_optimize(int item)
{
	object_type *o_ptr = &o_list[item];

	/* Paranoia -- be sure it exists */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* Delete the object */
	delete_object_idx(item);
}


/*
 * Check if we have space for an item in the pack without overflow
 */
bool inven_carry_okay(const object_type *o_ptr)
{
	int j;

	/* Empty slot? - note hack for bags taking up two slots */
	if (p_ptr->inven_cnt < INVEN_PACK - p_ptr->pack_size_reduce_quiver - (p_ptr->pack_size_reduce_study/3) - p_ptr->pack_size_reduce_bags - (o_ptr->tval == TV_BAG ? 1 : 0)) return (TRUE);

	/* Similar slot? */
	for (j = 0; j < INVEN_PACK; j++)
	{
		object_type *j_ptr = &inventory[j];

		/* Skip non-objects */
		if (!j_ptr->k_idx) continue;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr)) return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


/*
 * Add an item to the players inventory, and return the slot used.
 *
 * If the new item can combine with an existing item in the inventory,
 * it will do so, using "object_similar()" and "object_absorb()", else,
 * the item will be placed into the "proper" location in the inventory.
 *
 * This function can be used to "over-fill" the player's pack, but only
 * once, and such an action must trigger the "overflow" code immediately.
 * Note that when the pack is being "over-filled", the new item must be
 * placed into the "overflow" slot, and the "overflow" must take place
 * before the pack is reordered, but (optionally) after the pack is
 * combined.  This may be tricky.  See "dungeon.c" for info.
 *
 * Note that this code must remove any location/stack information
 * from the object once it is placed into the inventory.
 */
s16b inven_carry(object_type *o_ptr)
{
	int i, j, k;
	int n = -1;

	object_type *j_ptr;

	int book_tval= c_info[p_ptr->pclass].spell_book;

	/* XXX Douse lights in inventory */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->timeout))
	{
		o_ptr->charges = o_ptr->timeout;
		o_ptr->timeout = 0;
	}

	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++)
	{
		j_ptr = &inventory[j];

		/* Skip non-objects */
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr))
		{

			/* Combine the items */
			object_absorb(j_ptr, o_ptr, FALSE);

			/* Increase the weight */
			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN);

			/* Success */
			return (j);
		}

	}

	/* Paranoia */
	if (p_ptr->inven_cnt > INVEN_PACK) return (-1);

	/* Find an empty slot */
	for (j = 0; j <= INVEN_PACK; j++)
	{
		/*
		 * Hack -- Force pack overflow if we reached the slots of the
		 * inventory reserved for the quiver. -DG-
		 */
		if (j >= INVEN_PACK - p_ptr->pack_size_reduce_quiver - (p_ptr->pack_size_reduce_study/3) - p_ptr->pack_size_reduce_bags - (o_ptr->tval == TV_BAG ? 1 : 0))
		{
			/* Jump to INVEN_PACK to not mess up pack reordering */
			j = INVEN_PACK;

			break;
		}

		j_ptr = &inventory[j];

		/* Use it if found */
		if (!j_ptr->k_idx) break;
	}

	/* Use that slot */
	i = j;

	/* Reorder the pack */
	if (i < INVEN_PACK)
	{
		s32b o_value, j_value;

		/* Get the "value" of the item */
		o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			j_ptr = &inventory[j];

			/* Use empty slots */
			if (!j_ptr->k_idx) break;

			/* Hack -- readable books always come first */
			if ((o_ptr->tval == book_tval) &&
			    (j_ptr->tval != book_tval)) break;
			if ((j_ptr->tval == book_tval) &&
			    (o_ptr->tval != book_tval)) continue;

			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(o_ptr)) continue;
			if (!object_aware_p(j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Unidentified objects always come last */
			if (!object_known_p(o_ptr)) continue;
			if (!object_known_p(j_ptr)) break;

			/* Rods sort by increasing recharge time */
			if (o_ptr->tval == TV_ROD)
			{
				if (o_ptr->timeout < j_ptr->timeout) break;
				if (o_ptr->timeout > j_ptr->timeout) continue;
			}

			/* Wands/Staffs sort by decreasing charges */
			if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
			{
				if (o_ptr->charges > j_ptr->charges) break;
				if (o_ptr->charges < j_ptr->charges) continue;
			}

			/* Lites sort by decreasing fuel */
			if (o_ptr->tval == TV_LITE)
			{
				if (o_ptr->charges > j_ptr->charges) break;
				if (o_ptr->charges < j_ptr->charges) continue;
			}

			/* Determine the "value" of the pack item */
			j_value = object_value(j_ptr);

			/* Objects sort by decreasing value */
			if (o_value > j_value) break;
			if (o_value < j_value) continue;
		}

		/* Use that slot */
		i = j;

		/* Slide objects */
		for (k = n; k >= i; k--)
		{
			/* Hack -- Slide the item */
			object_copy(&inventory[k+1], &inventory[k]);
		}

		/* Wipe the empty slot */
		object_wipe(&inventory[i]);
	}

	/* Find if the show index is already in use */
	if (o_ptr->show_idx)
	{
		/* Check all items */
		for (k = 0; k < INVEN_TOTAL; k++) if ((inventory[k].k_idx) && (inventory[k].show_idx == o_ptr->show_idx)) o_ptr->show_idx = 0;
	}

	/* Need a show index? */
	if (!o_ptr->show_idx)
	{
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
		if (j < SHOWN_TOTAL) o_ptr->show_idx = j;
		else o_ptr->show_idx = 0;

		/* Redraw stuff */
		p_ptr->redraw |= (PR_ITEM_LIST);
	}

	/* Copy the item */
	object_copy(&inventory[i], o_ptr);

	/* Get the new object */
	j_ptr = &inventory[i];

	/* Forget stack */
	j_ptr->next_o_idx = 0;

	/* Forget monster */
	j_ptr->held_m_idx = 0;

	/* Forget location */
	j_ptr->iy = j_ptr->ix = 0;

	/* No longer marked */
	j_ptr->ident &= ~(IDENT_MARKED);

	/* Hack - bags take up 2 slots. We calculate this here. */
	if (j_ptr->tval == TV_BAG) p_ptr->pack_size_reduce_bags++;

	/* Increase the weight */
	p_ptr->total_weight += (j_ptr->number * j_ptr->weight);

	/* Count the items */
	p_ptr->inven_cnt++;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine and Reorder pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	/* Return the slot */
	return (i);
}



/*
 * Take off (some of) a non-cursed equipment item
 *
 * Note that only one item at a time can be wielded per slot.
 *
 * Note that taking off an item when "full" may cause that item
 * to fall to the ground.
 *
 * Return the inventory slot into which the item is placed.
 *
 * Destroys spells if taken off, rather than placing in inventory.
 *
 * It is now possible to drop an item from an ally, so we have
 * to handle this here.
 */
s16b inven_takeoff(int item, int amt)
{
	int slot;

	object_type *o_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	cptr act;
	cptr act2 = "";

	char o_name[80];
	char m_name[80];

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
		
		/* Get the monster name (or "it") */
		monster_desc(m_name, sizeof(m_name), o_ptr->held_m_idx, 0);
		
		/* Hack - capitalize monster name */
		if (islower((unsigned char)m_name[0]))
			m_name[0] = toupper((unsigned char)m_name[0]);
	}

	/* Paranoia */
	if (amt <= 0) return (-1);

	/* Verify */
	if (amt > o_ptr->number) amt = o_ptr->number;

	/* Hack - it is possible to take gold/gems off monsters */
	if ((o_ptr->tval == TV_GOLD) || (o_ptr->tval == TV_GEMS))
	{
		/* Money well earned? */
		p_ptr->au += o_ptr->charges;
		
		/* Update display */
		p_ptr->redraw |= (PR_GOLD);
	}
	
	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* Reset stack count */
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

	/* Describe the object */
	object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

	/* Take from monster */
	if (item < 0)
	{
		my_strcat(m_name, " was carrying", sizeof(m_name));
		act = m_name;
	}
	
	/* Destroy spell */
	else if (i_ptr->tval == TV_SPELL)
	{
		act = "You were enchanted with";
	}

	/* Took off weapon */
	else if (item == INVEN_WIELD)
	{
		act = "You were wielding";
	}

	/* Took off shield / secondary weapon */
	else if (item == INVEN_ARM)
	{
		if (i_ptr->tval == TV_SHIELD) act = "You were wearing";
		else act = "You were off-hand wielding";
	}

	/* Took off bow */
	else if (item == INVEN_BOW)
	{
		act = "You were holding";
	}

	/* Took off light */
	else if (item == INVEN_LITE)
	{
		act = "You were holding";

		/* Snuff light */
		if (!(artifact_p(i_ptr)) && (i_ptr->timeout))
		{
			i_ptr->charges = i_ptr->timeout;
			i_ptr->timeout = 0;
		}
	}

	/* Removed ammunition */
	else if (IS_QUIVER_SLOT(item))
	{
		act = "You removed";
		act2 = " from your quiver";
	}

	/* Took off something */
	else
	{
		act = "You were wearing";
	}

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_optimize(item);
	}
	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_optimize(0 - item);
	}

	/* Carry the object - spells/gold/gems are destroyed */
	if ((i_ptr->tval == TV_SPELL) || (i_ptr->tval == TV_GOLD) || (i_ptr->tval == TV_GEMS)) 
	{
		/* Forget the spell flags */
		inven_drop_flags(i_ptr);

		slot = -1;
	}
	/* All other items */
	else
	{
		/* Carry the item */
		slot = inven_carry(i_ptr);
	}

	/* Message, sound if not the quiver */
	if (!(IS_QUIVER_SLOT(item))) sound(MSG_WIELD);
	msg_format("%s %s (%c)%s.", act, o_name, index_to_label(slot), act2);

	/* Return slot */
	return (slot);
}




/*
 * Drop (some of) a non-cursed inventory/equipment item
 *
 * If m_idx = 0, the object will be dropped "near" the current location
 * otherwise it will be given to the monster m_idx.
 * 
 * It is now possible to drop an item from an ally, so we have
 * to handle this here.
 */
void inven_drop(int item, int amt, int m_idx)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	object_type *o_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	char o_name[80];
	char m_name[80];

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
		
		/* Get the monster name (or "it") */
		monster_desc(m_name, sizeof(m_name), o_ptr->held_m_idx, 0);
		
		/* Drop near monster instead */
		py = m_list[o_ptr->held_m_idx].fy;
		px = m_list[o_ptr->held_m_idx].fx;
	}

	/* Error check */
	if (amt <= 0) return;

	/* Not too many */
	if (amt > o_ptr->number) amt = o_ptr->number;

	/* Take off equipment */
	if (item >= INVEN_WIELD)
	{
		/* Take off first */
		item = inven_takeoff(item, amt);

		/* Check if destroyed by removal */
		if (item == -1) return;

		/* Get the original object */
		o_ptr = &inventory[item];
	}

	/* Forget about object */
	if ((item >= 0) && (amt == o_ptr->number)) inven_drop_flags(o_ptr);

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain local object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* Reset stack count */
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

	/* Forget information on dropped object */
	drop_may_flags(i_ptr);

	/* Describe local object */
	object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

	/* Message */
	msg_format("%s drop%s %s (%c).", item >= 0 ? "You" : m_name, item >= 0 ? "" : "s", o_name, index_to_label(item));

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
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
	}

	/* Give it to the monster */
	if (m_idx)
	{
		/* Carry the object */
		(void)monster_carry(m_idx, i_ptr);
	}
	else
	{
		/* Drop it near the player / monster */
		/* XXX Happens last for safety reasons */
		drop_near(i_ptr, 0, py, px, FALSE);
	}
}


/*
 * Check for pack overflow
 */
void overflow_pack(void)
{
	/* Hack -- Pack Overflow */
	if (inventory[INVEN_PACK].k_idx)
	{
		int item = INVEN_PACK;

		char o_name[80];

		object_type *o_ptr;

		object_type *i_ptr;
		object_type object_type_body;

		/* Get the slot to be dropped */
		o_ptr = &inventory[item];

		/* Get local object */
		i_ptr = &object_type_body;

		/* Obtain local object */
		object_copy(i_ptr, o_ptr);

		/* Disturbing */
		disturb(0, 0);

		/* Warning */
		msg_print("Your pack overflows!");

		/* Describe */
		object_desc(o_name, sizeof(o_name), i_ptr, TRUE, 3);

		/* Message */
		msg_format("You drop %s (%c).", o_name, index_to_label(item));

		/* Forget about it */
		inven_drop_flags(o_ptr);

		/* Forget information on dropped object */
		drop_may_flags(i_ptr);

		/* Modify, Describe, Optimize */
		inven_item_increase(item, -255);
		inven_item_describe(item);
		inven_item_optimize(item);

		/* Drop it (carefully) near the player */
		/* XXX Happens last for safety reasons */
		drop_near(i_ptr, 0, p_ptr->py, p_ptr->px, FALSE);

		/* Notice stuff (if needed) */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff (if needed) */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff (if needed) */
		if (p_ptr->redraw) redraw_stuff();

		/* Window stuff (if needed) */
		if (p_ptr->window) window_stuff();
	}
}


/*
 * Combine items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(void)
{
	int i, j, k;

	object_type *o_ptr;
	object_type *j_ptr;

	bool flag = FALSE;


	/* Combine the pack (backwards) */
	for (i = INVEN_PACK; i > 0; i--)
	{
		/* Get the item */
		o_ptr = &inventory[i];

		/* Skip empty items */
		if (!o_ptr->k_idx) continue;

		/* Scan the items above that item */
		for (j = 0; j < i; j++)
		{
			/* Get the item */
			j_ptr = &inventory[j];

			/* Skip empty items */
			if (!j_ptr->k_idx) continue;

			/* Can we drop "o_ptr" onto "j_ptr"? */
			if (object_similar(j_ptr, o_ptr))
			{
				/* Take note */
				flag = TRUE;

				/* Add together the item counts */
				object_absorb(j_ptr, o_ptr, FALSE);

				/* One object is gone */
				p_ptr->inven_cnt--;

				/* Slide everything down */
				for (k = i; k < INVEN_PACK; k++)
				{
					/* Hack -- slide object */
					COPY(&inventory[k], &inventory[k+1], object_type);
				}

				/* Hack -- wipe hole */
				object_wipe(&inventory[k]);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN);

				/* Done */
				break;
			}
		}
	}

	/* Hack - bags take up 2 slots. We calculate this here. */
	p_ptr->pack_size_reduce_bags = 0;

	/* Hack - count the number of bags */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		if (!o_ptr->k_idx) continue;

		if (o_ptr->tval == TV_BAG) p_ptr->pack_size_reduce_bags++;
	}

	/* Message */
	if (flag) msg_print("You combine some items in your pack.");
}


/*
 * Reorder items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void reorder_pack(void)
{
	int i, j, k;

	s32b o_value;
	s32b j_value;

	object_type *o_ptr;
	object_type *j_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	bool flag = FALSE;

	int book_tval= c_info[p_ptr->pclass].spell_book;

	/* Re-order the pack (forwards) */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Mega-Hack -- allow "proper" over-flow */
		if ((i == INVEN_PACK) && (p_ptr->inven_cnt == INVEN_PACK)) break;

		/* Get the item */
		o_ptr = &inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			/* Get the item already there */
			j_ptr = &inventory[j];

			/* Use empty slots */
			if (!j_ptr->k_idx) break;

			/* Hack -- readable books always come first */
			if ((o_ptr->tval == book_tval) &&
			    (j_ptr->tval != book_tval)) break;
			if ((j_ptr->tval == book_tval) &&
			    (o_ptr->tval != book_tval)) continue;

			/* Objects sort by decreasing type */
			if (o_ptr->tval > j_ptr->tval) break;
			if (o_ptr->tval < j_ptr->tval) continue;

			/* Non-aware (flavored) items always come last */
			if (!object_aware_p(o_ptr)) continue;
			if (!object_aware_p(j_ptr)) break;

			/* Objects sort by increasing sval */
			if (o_ptr->sval < j_ptr->sval) break;
			if (o_ptr->sval > j_ptr->sval) continue;

			/* Unidentified objects always come last */
			if (!object_known_p(o_ptr)) continue;
			if (!object_known_p(j_ptr)) break;

			/* Rods sort by increasing recharge time */
			if (o_ptr->tval == TV_ROD)
			{
				if (o_ptr->timeout < j_ptr->timeout) break;
				if (o_ptr->timeout > j_ptr->timeout) continue;
			}

			/* Wands/Staffs sort by decreasing charges */
			if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
			{
				if (o_ptr->charges > j_ptr->charges) break;
				if (o_ptr->charges < j_ptr->charges) continue;
			}

			/* Lites sort by decreasing fuel */
			if (o_ptr->tval == TV_LITE)
			{
				if (o_ptr->charges > j_ptr->charges) break;
				if (o_ptr->charges < j_ptr->charges) continue;
			}

			/* Determine the "value" of the pack item */
			j_value = object_value(j_ptr);

			/* Objects sort by decreasing value */
			if (o_value > j_value) break;
			if (o_value < j_value) continue;
		}

		/* Never move down */
		if (j >= i) continue;

		/* Take note */
		flag = TRUE;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Save a copy of the moving item */
		object_copy(i_ptr, &inventory[i]);

		/* Slide the objects */
		for (k = i; k > j; k--)
		{
			/* Slide the item */
			object_copy(&inventory[k], &inventory[k-1]);
		}

		/* Insert the moving item */
		object_copy(&inventory[j], i_ptr);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN);
	}

	/* Message */
	if (flag) msg_print("You reorder some items in your pack.");
}



/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool book_sort_comp_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);
	u16b *why = (u16b*)(v);

	int w1 = who[a];
	int w2 = who[b];

	int z1, z2;
	int i, j;

	spell_type *s1_ptr = &s_info[w1];
	spell_type *s2_ptr = &s_info[w2];

	for (i = 0; i < MAX_SPELL_CASTERS; i++)
	{
		if (s1_ptr->cast[i].class == *why) break;
	}

	for (j = 0; j < MAX_SPELL_CASTERS; j++)
	{
		if (s2_ptr->cast[j].class == *why) break;
	}

	/* One spell is illegible */
	if ((i == MAX_SPELL_CASTERS) && (j == MAX_SPELL_CASTERS)) return (my_stricmp(s_name + s1_ptr->name, s_name + s2_ptr->name) <= 0);
	else if (i == MAX_SPELL_CASTERS) return (FALSE);
	else if (j == MAX_SPELL_CASTERS) return (TRUE);

	/* Extract spell level */
	z1 = s1_ptr->cast[i].level;
	z2 = s2_ptr->cast[j].level;

	/* Compare spell levels */
	if (z1 < z2) return (TRUE);
	if (z1 > z2) return (FALSE);

	/* Extract spell mana */
	z1 = s1_ptr->cast[i].mana;
	z2 = s2_ptr->cast[j].mana;

	/* Compare spell mana */
	if (z1 < z2) return (TRUE);
	if (z1 > z2) return (FALSE);

	/* Alphabetical sort */
	return (my_stricmp(s_name + s1_ptr->name, s_name + s2_ptr->name) <= 0);
}



/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void book_sort_swap_hook(vptr u, vptr v, int a, int b)
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
 * Does the spell match the player's style
 */
bool spell_match_style(int spell)
{
	int i;

	/* Check player styles */
	if ((p_ptr->pstyle != WS_SONG_BOOK) && (p_ptr->pstyle != WS_MAGIC_BOOK) && (p_ptr->pstyle != WS_PRAYER_BOOK)) return (FALSE);

	/* Check spells */
	for (i = 0; i < MAX_SPELL_APPEARS; i++)
	{
		int tval = s_info[spell].appears[i].tval;
		int sval = s_info[spell].appears[i].sval;

		/* Not a book */
		if ((tval != TV_SONG_BOOK) && (tval != TV_MAGIC_BOOK) && (tval != TV_PRAYER_BOOK)) continue;

		/* Book does not match player style */
		if ((tval == TV_SONG_BOOK) && (p_ptr->pstyle != WS_SONG_BOOK)) continue;
		if ((tval == TV_MAGIC_BOOK) && (p_ptr->pstyle != WS_MAGIC_BOOK)) continue;
		if ((tval == TV_PRAYER_BOOK) && (p_ptr->pstyle != WS_PRAYER_BOOK)) continue;

		/* Outright match */
		if (sval == p_ptr->psval) return (TRUE);
		
		/* Match high version of book */
		if ((sval < SV_BOOK_MAX_GOOD) && (p_ptr->psval == sval % (SV_BOOK_MAX_GOOD / 2))) return (TRUE);

		/* Match because we have specialised in a 'basic spellbook style, and book falls into this category */
		if ((p_ptr->psval >= SV_BOOK_MAX_GOOD) && (sval >= SV_BOOK_MAX_GOOD))
		{
			/* Confirm in same school */
			if ((sval - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL == (p_ptr->psval - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL) return (TRUE);
		}
	}

	/* No match */
	return (FALSE);
}



/*
 * Fills a book with spells (in order).
 */
void fill_book(const object_type *o_ptr, s16b *book, int *num)
{
	int i,ii;

	spell_type *s_ptr;

	u16b why = p_ptr->pclass;

	/* No spells */
	*num = 0;

	/* Fill book with nothing */
	for (i=0;i<26;i++) book[i]=0;

	/* Hack -- Study materials are easy */
	if (o_ptr->tval == TV_STUDY)
	{
		book[0] = o_ptr->pval;
		*num = 1;
		return;
	}

	/* Fill book with spells */
	for (i=0;(i<z_info->s_max) && (*num < 26);i++)
	{
		s_ptr=&s_info[i];

		for (ii=0;ii<MAX_SPELL_APPEARS;ii++)
		{
			int tval = s_ptr->appears[ii].tval;
			int sval = s_ptr->appears[ii].sval;

			if ((tval == o_ptr->tval) &&
				(sval == o_ptr->sval))
			{
				book[s_ptr->appears[ii].slot-1] = i;
				(*num)++;
			}
		}
	}

	/* Sort book contents */
	ang_sort_comp = book_sort_comp_hook;
	ang_sort_swap = book_sort_swap_hook;

	/* Sort the array */
	ang_sort(book, &why, *num);

	/* Paranoia */
	if (*num >= 26) return;

	/* Get artifact spells */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		if (a_ptr->activation)
		{
			book[(*num)++] = a_ptr->activation;
		}
	}
	/* Get ego item spells */
	else if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		if (e_ptr->activation)
		{
			book[(*num)++] = e_ptr->activation;
		}
	}
}


/*
 * Returns the spell casting details that the caster
 * uses. This may later be modified.
 */
spell_cast *spell_cast_details(int spell)
{
	/* Get spell details */
	spell_type *s_ptr = &s_info[spell];

	/* Spell cast details to return */
	spell_cast *sc_ptr = NULL;

	/* Get our casting information */
	int i;
	int worst = 0;
	int best = 0;

	/* Note check for warriors. */
	if (p_ptr->pclass) for (i = 0;i < MAX_SPELL_CASTERS; i++)
	{
		if (s_ptr->cast[i].class == p_ptr->pclass)
		{
			sc_ptr=&(s_ptr->cast[i]);
			
			/* Is this ability worse? */
			if (s_ptr->cast[i].level > s_ptr->cast[worst].level) worst = i;
			
			/* Is this ability better? */
			if (s_ptr->cast[i].level < s_ptr->cast[best].level) best = i;
		}
	}

	/* Hack -- if the character doesn't have the ability to cast a spell,
	 * choose the worst one if they are a specialist and not powerful,
	 * or the best one if they are a specialist and powerful */
	if (!sc_ptr && spell_match_style(spell)) sc_ptr = &(s_ptr->cast[cp_ptr->spell_power ? best : worst]);

	return (sc_ptr);
}


/*
 * Returns level for a spell.
 * 
 * We don't check legibility here because spell_legible calls this.
 */
s16b spell_level_aux(int spell)
{
	int i;

	s16b level;

	spell_type *s_ptr = &s_info[spell];

	spell_cast *sc_ptr = spell_cast_details(spell);

	bool fix_level = TRUE;

	/* Check we have casting details */
	if (!sc_ptr) return (100);

	/* Hack -- check if we can 'naturally' cast it,
	 * as opposed to relying on speciality.
	 * Note check for warriors. */
	if (p_ptr->pclass) for (i = 0;i < MAX_SPELL_CASTERS; i++)
	{
		if (s_ptr->cast[i].class == p_ptr->pclass)
		{
			/* Use the native casting level */
			fix_level = FALSE;
		}
	}

	/* Get the level */
	level = sc_ptr->level;

	/* Modify the level */
	for (i = 0;i< z_info->w_max;i++)
	{
		if (w_info[i].class != p_ptr->pclass) continue;

		/* Hack -- Increase minimum level */
		if (fix_level)
		{
			if ((w_info[i].styles & (1L << p_ptr->pstyle)) != 0)
			{
				level += w_info[i].level - 1;

				fix_level = FALSE;
			}
		}

		if (w_info[i].level > p_ptr->lev) continue;

		if (w_info[i].benefit != WB_POWER) continue;

		/* Check for styles */
		/* Hack -- we don't check 'current' styles */
		if ((w_info[i].styles == 0)
			|| (w_info[i].styles & (1L << p_ptr->pstyle)))
		{
			/* Check for style match */
			if (spell_match_style(spell))
			{
				/* Reduce casting level */
				level -= (level - w_info[i].level)/5;
			}
		}
	}

	return(level);
}


/*
 * Spell could be learnt by the player.
 *
 * This differs from spell_read_okay, in that the spell could appear
 * in another book that the player is allowed to use.
 */
bool spell_legible(int spell)
{
	int i;
	spell_type *s_ptr = &s_info[spell];

	/* Note check for warriors. */
	if (p_ptr->pclass) for (i = 0;i < MAX_SPELL_CASTERS; i++)
	{
		/* Class is allowed to cast the spell */
		if (s_ptr->cast[i].class == p_ptr->pclass) return (TRUE);
	}

	/* Gifted and chosen spell casters can read all spells from the book they have specialised in,
	 * unless the computed level is too high. */
	if (spell_match_style(spell) && (spell_level_aux(spell) <= 50)) return (TRUE);

	return (FALSE);
}


/*
 * Is the spell legible?
 */
s16b spell_level(int spell)
{
	/* Illegible */
	if (!spell_legible(spell)) return (100);

	return (spell_level_aux(spell));
}






/*
 * Get the spell power
 */
s16b spell_power(int spell)
{
	int i;

	int plev = p_ptr->lev;

	/* Hack -- instruments modify spell power per book for everyone */
	if (p_ptr->cur_style & (1L << WS_INSTRUMENT)) for(i = 0; i < MAX_SPELL_APPEARS; i++)
	{
		/* Line 1 - has item wielded */
		/* Line 2 - item is instrument */
		/* Line 3 - instrument sval matches spellbook sval */
		/* Line 4 - appears in spellbook */
		if ((inventory[INVEN_BOW].k_idx) &&
			(inventory[INVEN_BOW].tval == TV_INSTRUMENT) &&
			(s_info[spell].appears[i].sval == inventory[INVEN_BOW].sval) &&
			 (s_info[spell].appears[i].tval == TV_SONG_BOOK))
		{
			plev += 5;
		}
	}

	/* Modify the spell power */
	for (i = 0;i< z_info->w_max;i++)
	{
		if (w_info[i].class != p_ptr->pclass) continue;

		if (w_info[i].level > p_ptr->lev) continue;

		if (w_info[i].benefit != WB_POWER) continue;

		/* Check styles */
		/* Hack -- we don't check 'current' styles
		   except for rings, amulets, instruments, etc */
		if ((w_info[i].styles==0)
			|| (w_info[i].styles & (1L << p_ptr->pstyle)))
		switch (p_ptr->pstyle)
		{
			case WS_MAGIC_BOOK:
			case WS_PRAYER_BOOK:
			case WS_SONG_BOOK:
			{
				/* Increase by 10 levels */
				if (spell_match_style(spell)) plev += 10;
				break;
			}
			case WS_INSTRUMENT:
			{
				int j;

				for(j = 0; j < MAX_SPELL_APPEARS; j++)
				{
					/* Line 1 - has item wielded */
					/* Line 2 - item is instrument */
					/* Line 3 - instrument sval matches spellbook sval */
					/* Line 4 - appears in spellbook */
					if ((inventory[INVEN_BOW].k_idx) &&
						(inventory[INVEN_BOW].tval == TV_INSTRUMENT) &&
						(s_info[spell].appears[j].sval == inventory[INVEN_BOW].sval) &&
						 (s_info[spell].appears[j].tval == TV_SONG_BOOK))
					{
						plev += 10;
					}
				}
				break;
			}
		}
	}

	return(plev);
}


/*
 * Returns chance of failure for a spell
 */
s16b spell_chance(int spell)
{
	int chance, minfail;

	spell_cast *sc_ptr = spell_cast_details(spell);

	/* Illegible */
	if (!spell_legible(spell)) return (100);

	/* Check we have casting details */
	if (!sc_ptr) return (100);

	/* Extract the base spell failure rate */
	chance = sc_ptr->fail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (p_ptr->lev - spell_level(spell));

	/* Reduce failure rate by the fail-stat adjustment */
	chance -= 3 * (adj_mag_fail_rate[p_ptr->stat_ind[c_info[p_ptr->pclass].spell_stat_fail]] - 1);

	/* Not enough mana to cast */
	if (sc_ptr->mana > p_ptr->csp)
	{
		chance += 5 * (sc_ptr->mana - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail_min[p_ptr->stat_ind[c_info[p_ptr->pclass].spell_stat_fail]];

	/* Non mage/priest characters never get better than 5 percent */
	if (!(c_info[p_ptr->pclass].spell_power))
	{
		if (minfail < 5) minfail = 5;
	}

	/* Priest prayer penalty for "edged" weapons (before minfail) */
	if (p_ptr->icky_wield)
	{
		chance += 25;
	}

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder (after minfail) */
	if (p_ptr->timed[TMD_STUN] > 50) chance += 25;
	else if (p_ptr->timed[TMD_STUN]) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	/* Return the chance */
	return (chance);
}





/*
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 */
bool spell_okay(int spell, bool known)
{
	spell_type *s_ptr;

	int i;

	/* Get the spell */
	s_ptr = &s_info[spell];

	/* Spell is illegible */
	if (!spell_legible(spell)) return (FALSE);

	/* Spell is illegal */
	if (spell_level(spell) > p_ptr->lev) return (FALSE);

	/* Get the spell from the spells the player has learnt */
	for (i=0;i<PY_MAX_SPELLS;i++)
	{
		if (p_ptr->spell_order[i] == spell) break;
	}

	/* Spell okay to study, not to cast */
	if (i == PY_MAX_SPELLS)
	{
	  /* Spell may require pre-requisites */
		if (!known)
		{
			int n;

			bool preq = FALSE;

			/* Check prerequisites, unless specialist */
			if (!spell_match_style(spell))
			  for (n = 0; n < MAX_SPELL_PREREQUISITES; n++)
			  {
				if (s_info[spell].preq[n])
				{
					preq = TRUE;

					for (i=0;i<PY_MAX_SPELLS;i++)
					{
						if (p_ptr->spell_order[i] == s_info[spell].preq[n]) return (!known);
					}
				}
			  }

			return (!preq);
		}

		return (!known);
	}

	/* Spell is forgotten */
	if ((i < 32) ? (p_ptr->spell_forgotten1 & (1L << i)) :
	      ((i < 64) ? (p_ptr->spell_forgotten2 & (1L << (i - 32))) :
	      ((i < 96) ? (p_ptr->spell_forgotten3 & (1L << (i - 64))) :
	      (p_ptr->spell_forgotten4 & (1L << (i - 96))))))
	{
		/* Never okay */
		return (FALSE);
	}

	/* Spell is learned */
	if ((i < 32) ? (p_ptr->spell_learned1 & (1L << i)) :
	      ((i < 64) ? (p_ptr->spell_learned2 & (1L << (i - 32))) :
	      ((i < 96) ? (p_ptr->spell_learned3 & (1L << (i - 64))) :
	      (p_ptr->spell_learned4 & (1L << (i - 96))))))
	{
		/* Okay to cast, not to study */
		return (known);
	}

	/* Okay to study, not to cast */
	return (!known);
}


/*
 * Returns in tag_num the numeric value of the inscription tag associated with
 * an object in the inventory.
 * Valid tags have the form "@n" or "@xn" where "n" is a number (0-9) and "x"
 * is cmd. If cmd is 0 then "x" can be anything.
 * Returns FALSE if the object doesn't have a valid tag.
 */
int get_tag_num(int o_idx, int cmd, byte *tag_num)
{
	object_type *o_ptr = &inventory[o_idx];
	char *s;

	/* Ignore empty objects */
	if (!o_ptr->k_idx) return FALSE;

	/* Ignore objects without notes */
	if (!o_ptr->note) return FALSE;

	/* Find the first '@' */
	s = strchr(quark_str(o_ptr->note), '@');

	while (s)
	{
		/* Found "@n"? */
		if (isdigit((unsigned char)s[1]))
		{
			/* Convert to number */
			*tag_num = D2I(s[1]);
			return TRUE;
		}

		/* Found "@xn"? */
		if ((!cmd || ((unsigned char)s[1] == cmd)) &&
			isdigit((unsigned char)s[2]))
		{
			/* Convert to number */
			*tag_num = D2I(s[2]);
			return TRUE;
		}

		/* Find another '@' in any other case */
		s = strchr(s + 1, '@');
	}

	return FALSE;
}


/*
 * Calculate and apply the reduction in pack size due to use of the
 * Quiver.
 */
void find_quiver_size(void)
{
	int ammo_num, i;
	object_type *i_ptr;

	/*
	 * Items in the quiver take up space which needs to be subtracted
	 * from space available elsewhere.
	 */
	ammo_num = 0;

	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* Get the item */
		i_ptr = &inventory[i];

		/* Ignore empty. */
		if (!i_ptr->k_idx) continue;

		/* Tally up missiles. */
		ammo_num += i_ptr->number * quiver_space_per_unit(i_ptr);
	}

	/* Every 99 missiles in the quiver takes up one backpack slot. */
	p_ptr->pack_size_reduce_quiver = (ammo_num + 98) / 99;
}


/*
 * Combine ammo in the quiver.
 */
void combine_quiver(void)
{
	int i, j, k;

	object_type *i_ptr;
	object_type *j_ptr;

	bool flag = FALSE;

	/* Combine the quiver (backwards) */
	for (i = END_QUIVER - 1; i > INVEN_QUIVER; i--)
	{
		/* Get the item */
		i_ptr = &inventory[i];

		/* Skip empty items */
		if (!i_ptr->k_idx) continue;

		/* Scan the items above that item */
		for (j = INVEN_QUIVER; j < i; j++)
		{
			/* Get the item */
			j_ptr = &inventory[j];

			/* Skip empty items */
			if (!j_ptr->k_idx) continue;

			/* Can we drop "i_ptr" onto "j_ptr"? */
			if (object_similar(j_ptr, i_ptr))
			{
				/* Take note */
				flag = TRUE;

				/* Add together the item counts */
				object_absorb(j_ptr, i_ptr, FALSE);

				/* Slide everything down */
				for (k = i; k < (END_QUIVER - 1); k++)
				{
					/* Hack -- slide object */
					COPY(&inventory[k], &inventory[k+1], object_type);
				}

				/* Hack -- wipe hole */
				object_wipe(&inventory[k]);

				/* Done */
				break;
			}
		}
	}

	if (flag)
	{
		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Message */
		msg_print("You combine your quiver.");
	}
}

/*
 * Sort ammo in the quiver.  If requested, track the given slot and return
 * its final position.
 *
 * Items marked with inscriptions of the form "@ [f] [any digit]"
 * ("@f4", "@4", etc.) will be locked. It means that the ammo will be always
 * fired using that digit. Different locked ammo can share the same tag.
 * Unlocked ammo can be fired using its current pseudo-tag (shown in the
 * equipment window). In that case the pseudo-tag can change depending on
 * ammo consumption. -DG- (based on code from -LM- and -BR-)
 */
int reorder_quiver(int slot)
{
	int i, j, k;

	s32b i_value;
	byte i_group;

	object_type *i_ptr;
	object_type *j_ptr;

	bool flag = FALSE;

	byte tag;

	/* This is used to sort locked and unlocked ammo */
	struct ammo_info_type {
		byte group;
		s16b idx;
		byte tag;
		s32b value;
	} ammo_info[MAX_QUIVER];
	/* Position of the first locked slot */
	byte first_locked;
	/* Total size of the quiver (unlocked + locked ammo) */
	byte quiver_size;

	/*
	 * This will hold the final ordering of ammo slots.
	 * Note that only the indexes are stored
	 */
	s16b sorted_ammo_idxs[MAX_QUIVER];

	/*
	 * Reorder the quiver.
	 *
	 * First, we sort the ammo *indexes* in two sets, locked and unlocked
	 * ammo. We use the same table for both sets (ammo_info).
	 * Unlocked ammo goes in the beginning of the table and locked ammo
	 * later.
	 * The sets are merged later to produce the final ordering.
	 * We use "first_locked" to determine the bound between sets. -DG-
	 */
	first_locked = quiver_size = 0;

	/* Traverse the quiver */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* Get the object */
		i_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!i_ptr->k_idx) continue;

		/* Get the value of the object */
		i_value = object_value(i_ptr);

		/* Note that we store all throwing weapons in the same group */
		i_group = quiver_get_group(i_ptr);

		/* Get the real tag of the object, if any */
		if (get_tag_num(i, quiver_group[i_group].cmd, &tag))
		{
			/* Determine the portion of the table to be used */
			j = first_locked;
			k = quiver_size;
		}
		/*
		 * If there isn't a tag we use a special value
		 * to separate the sets.
		 */
		else
		{
			tag = 10;
			/* Determine the portion of the table to be used */
			j = 0;
			k = first_locked;

			/* We know that all locked ammo will be displaced */
			++first_locked;
		}

		/* Search for the right place in the table */
		for (; j < k; j++)
		{
			/* Get the other ammo */
			j_ptr = &inventory[ammo_info[j].idx];

			/* Objects sort by increasing group */
			if (i_group < ammo_info[j].group) break;
			if (i_group > ammo_info[j].group) continue;

			/*
			 * Objects sort by increasing tag.
			 * Note that all unlocked ammo have the same tag (10)
			 * so this step is meaningless for them -DG-
			 */
			if (tag < ammo_info[j].tag) break;
			if (tag > ammo_info[j].tag) continue;

			/* Objects sort by decreasing tval */
			if (i_ptr->tval > j_ptr->tval) break;
			if (i_ptr->tval < j_ptr->tval) continue;

			/* Non-aware items always come last */
			if (!object_aware_p(i_ptr)) continue;
			if (!object_aware_p(j_ptr)) break;

			/* Objects sort by increasing sval */
			if (i_ptr->sval < j_ptr->sval) break;
			if (i_ptr->sval > j_ptr->sval) continue;

			/* Unidentified objects always come last */
			if (!object_known_p(i_ptr)) continue;
			if (!object_known_p(j_ptr)) break;

			/* Objects sort by decreasing value */
			if (i_value > ammo_info[j].value) break;
			if (i_value < ammo_info[j].value) continue;
		}

		/*
		 * We found the place. Displace the other slot
		 * indexes if neccesary
		 */
		for (k = quiver_size; k > j; k--)
		{
			COPY(&ammo_info[k], &ammo_info[k - 1],
				struct ammo_info_type);
		}

		/* Cache some data from the slot in the table */
		ammo_info[j].group = i_group;
		ammo_info[j].idx = i;
		ammo_info[j].tag = tag;
		/* We cache the value of the object too */
		ammo_info[j].value = i_value;

		/* Update the size of the quiver */
		++quiver_size;
	}

	/*
	 * Second, we merge the two sets to find the final ordering of the
	 * ammo slots.
	 * What this step really does is to place unlocked ammo in
	 * the slots which aren't used by locked ammo. Again, we only work
	 * with indexes in this step.
	 */
	i = 0;
	j = first_locked;
	tag = k = 0;

	/* Compare ammo between the sets */
	while ((i < first_locked) && (j < quiver_size))
	{
		/* Groups are different. Add the smallest first */
		if (ammo_info[i].group > ammo_info[j].group)
		{
			sorted_ammo_idxs[k++] = ammo_info[j++].idx;
			/* Reset the tag */
			tag = 0;
		}

		/* Groups are different. Add the smallest first */
		else if (ammo_info[i].group < ammo_info[j].group)
		{
			sorted_ammo_idxs[k++] = ammo_info[i++].idx;
			/* Reset the tag */
			tag = 0;
		}

		/*
		 * Same group, and the tag is unlocked. Add some unlocked
		 * ammo first
		 */
		else if (tag < ammo_info[j].tag)
		{
			sorted_ammo_idxs[k++] = ammo_info[i++].idx;
			/* Increment the pseudo-tag */
			++tag;
		}
		/*
		 * The tag is locked. Perhaps there are several locked
		 * slots with the same tag too. Add the locked ammo first
		 */
		else
		{
			/*
			 * The tag is incremented only at the first ocurrence
			 * of that value
			 */
			if (tag == ammo_info[j].tag)
			{
				++tag;
			}
			/* But the ammo is always added */
			sorted_ammo_idxs[k++] = ammo_info[j++].idx;
		}
	}

	/* Add remaining unlocked ammo, if neccesary */
	while (i < first_locked)
	{
		sorted_ammo_idxs[k++] = ammo_info[i++].idx;
	}

	/* Add remaining locked ammo, if neccesary */
	while (j < quiver_size)
	{
		sorted_ammo_idxs[k++] = ammo_info[j++].idx;
	}

	/* Determine if we have to reorder the real ammo */
	for (k = 0; k < quiver_size; k++)
	{
		if (sorted_ammo_idxs[k] != (INVEN_QUIVER + k))
		{
			flag = TRUE;

			break;
		}
	}

	/* Reorder the real quiver, if necessary */
	if (flag)
	{
		/* Temporary copy of the quiver */
		object_type quiver[MAX_QUIVER];

		/*
		 * Copy the real ammo into the temporary quiver using
		 * the new order
		 */
		for (i = 0; i < quiver_size; i++)
		{
			/* Get the original slot */
			k = sorted_ammo_idxs[i];

			/* Note the indexes */
			object_copy(&quiver[i], &inventory[k]);

			/*
			 * Hack - Adjust the temporary slot if necessary.
			 * Note that the slot becomes temporarily negative
			 * to avoid multiple matches (indexes are positive).
			 */
			if (slot == k) slot = 0 - (INVEN_QUIVER + i);
		}

		/*
		 * Hack - The loop has ended and slot was changed only once
		 * like we wanted. Now we make slot positive again. -DG-
		 */
		if (slot < 0) slot = 0 - slot;

		/*
		 * Now dump the temporary quiver (sorted) into the real quiver
		 */
		for (i = 0; i < quiver_size; i++)
		{
			object_copy(&inventory[INVEN_QUIVER + i],
					&quiver[i]);
		}

		/* Clear unused slots */
		for (i = INVEN_QUIVER + quiver_size; i < END_QUIVER; i++)
		{
			object_wipe(&inventory[i]);
		}

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Message */
		if (!slot) msg_print("You reorganize your quiver.");
	}

	return (slot);
}


/*
 * Returns TRUE if an object is a throwing item
 */
bool is_throwing_item(const object_type *o_ptr)
{
  u32b f1, f2, f3, f4;

  object_flags(o_ptr, &f1, &f2, &f3, &f4);

  if (f3 & (TR3_HURL_NUM | TR3_HURL_DAM)) return (TRUE);

  return (k_info[o_ptr->k_idx].flags5 & (TR5_THROWING) ? TRUE : FALSE);
}


/*
 * Returns TRUE if an object is a known throwing item
 */
bool is_known_throwing_item(const object_type *o_ptr)
{
  u32b f1, f2, f3, f4;

  object_flags_known(o_ptr, &f1, &f2, &f3, &f4);

  if (f3 & (TR3_HURL_NUM | TR3_HURL_DAM)) return (TRUE);

  return (k_info[o_ptr->k_idx].flags5 & (TR5_THROWING) ? TRUE : FALSE);
}


/*
 * Returns the number of quiver units an object will consume when it's stored in the quiver.
 * Every 99 quiver units we consume an inventory slot
 */
int quiver_space_per_unit(const object_type *o_ptr)
{
	return (ammo_p(o_ptr)
		|| (o_ptr->tval == TV_EGG && o_ptr->sval == SV_EGG_SPORE)
		? 1 : 5);
}

/*
 * Returns TRUE if the specified number of objects of type o_ptr->k_idx can be hold in the quiver.
 * Hack: if you are moving objects from the inventory to the quiver pass the inventory slot occupied by the object in
 * "item". This will help us to determine if we have one free inventory slot more. You can pass -1 to ignore this feature.
 */
bool quiver_carry_okay(const object_type *o_ptr, int num, int item)
{
	int i;
	int ammo_num = 0;
	int have;
	int need;

	/* Paranoia */
	if ((num <= 0) || (num > o_ptr->number)) num = o_ptr->number;

	/* Count ammo in the quiver */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* Get the object */
		object_type *i_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!i_ptr->k_idx) continue;

		/* Increment the ammo count */
		ammo_num += (i_ptr->number * quiver_space_per_unit(i_ptr));
	}

	/* Add the requested amount of objects to be put in the quiver */
	ammo_num += (num * quiver_space_per_unit(o_ptr));

	/* We need as many free inventory as: */
	need = (ammo_num + 98) / 99;

	/* Calculate the number of available inventory slots */
	have = INVEN_PACK - p_ptr->inven_cnt;

	/* If we are emptying an inventory slot we have one free slot more */
	if ((item >= 0) && (item < INVEN_PACK) && (num == o_ptr->number)) ++have;

	/* Compute the result */
	return (need <= have);
}

/*
 * Returns the quiver group associated to an object. Defaults to throwing weapons
 */
byte quiver_get_group(const object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_BOLT: return (QUIVER_GROUP_BOLTS);
		case TV_ARROW: return (QUIVER_GROUP_ARROWS);
		case TV_SHOT: return (QUIVER_GROUP_SHOTS);
	}

	return (QUIVER_GROUP_THROWING_WEAPONS);
}

