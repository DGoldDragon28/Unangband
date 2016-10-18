/* File: monlist.c */

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

enum {
	MONLIST_GROUP_BY_SEEN,
	MONLIST_GROUP_BY_NOT_SHOOT,
	MONLIST_GROUP_BY_AWARE,
	MONLIST_GROUP_BY_MAX
};

enum {
	MONLIST_SORT_BY_DISTANCE,
	MONLIST_SORT_BY_PROXIMITY,
	MONLIST_SORT_BY_DEPTH,
	MONLIST_SORT_BY_VENGEANCE,
	MONLIST_SORT_BY_NOVELTY,
	MONLIST_SORT_BY_MAX
};

const char *sort_by_name[MONLIST_SORT_BY_MAX]=
{
		"distance",
		"proximity",
		"depth",
		"vengeance",
		"novelty"
};


typedef struct monlist_type monlist_type;

enum {
MONLIST_NONE = 0, MONLIST_BLANK, MONLIST_MORE, MONLIST_RACE, MONLIST_KIND,
MONLIST_ARTIFACT, MONLIST_FEATURE,

/* The headings */
MONLIST_HEADER_SEEN_MONSTER, MONLIST_HEADER_NOT_SHOOT_MONSTER,
MONLIST_HEADER_AWARE_MONSTER, MONLIST_HEADER_SEEN_OBJECT,
MONLIST_HEADER_NOT_SHOOT_OBJECT,
MONLIST_HEADER_AWARE_OBJECT, MONLIST_HEADER_SEEN_FEATURE,
MONLIST_HEADER_NOT_SHOOT_FEATURE, MONLIST_HEADER_AWARE_FEATURE,

/* The others */
MONLIST_OTHER_SEEN_MONSTER, MONLIST_OTHER_NOT_SHOOT_MONSTER,
MONLIST_OTHER_AWARE_MONSTER, MONLIST_OTHER_SEEN_OBJECT,
MONLIST_OTHER_NOT_SHOOT_OBJECT,
MONLIST_OTHER_AWARE_OBJECT, MONLIST_OTHER_SEEN_FEATURE,
MONLIST_OTHER_NOT_SHOOT_FEATURE, MONLIST_OTHER_AWARE_FEATURE,

MONLIST_MAX
};

#define MONLIST_DISPLAY_MONSTER	0x01
#define MONLIST_DISPLAY_OBJECT	0x02
#define MONLIST_DISPLAY_FEATURE	0x04

struct monlist_type
{
	int row_type;
	int idx;
	int number;
	int len;

	char *text;
	byte attr;

	/* Graphic for row */
	char x_char;
	byte x_attr;

	int closest_y;
	int closest_x;

	monlist_type *next;
};


/*
 * Free a monster list
 */
void free_monlist(monlist_type *monlist)
{
	while (monlist)
	{
		monlist_type *next_monlist = monlist->next;

		if (monlist->text)
		{
			FREE(monlist->text);
		}

		FREE(monlist);

		monlist = next_monlist;
	}
}


/*
 * Copies the buffer to a screen line
 */
void monlist_copy_buffer_to_screen(char *buf, monlist_type *monlist)
{
	/* Allocate space for the buffer */
	monlist->text = C_ZNEW(strlen(buf) + 2, char);
	monlist->len = strlen(buf);

	/* Copy the buffer */
	my_strcpy(monlist->text, buf, strlen(buf));
}


/*
 * Gets the index into the array.
 */
int monlist_get_monster_index(int idx)
{
	/* Get the monster */
	monster_type *m_ptr = &m_list[idx];

	return (m_ptr->r_idx);
}

/*
 * Gets the index into the array.
 */
int monlist_get_object_index(int idx)
{
	/* Get the object */
	object_type *o_ptr = &o_list[idx];

	return (o_ptr->k_idx);
}


/*
 * Checks if a monster should be displayed for this grouping
 */
bool monlist_check_monster_grouping(int idx, int group_by)
{
	/* Get the monster */
	monster_type *m_ptr = &m_list[idx];

	/* Only visible monsters */
	if (!m_ptr->ml) return (FALSE);

	/* Check which type we're collecting */
	if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE))
	{
		if (group_by != MONLIST_GROUP_BY_SEEN) return (FALSE);
	}
	else if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_SEEN))
	{
		if (group_by != MONLIST_GROUP_BY_SEEN) return (FALSE);
	}
	else if (group_by != MONLIST_GROUP_BY_SEEN) return (FALSE);

	return (TRUE);
}


/*
 * Checks if an object should be displayed for this grouping
 *
 * XXX Note we don't care whether we can shoot objects so
 * that grouping will always be empty.
 */
bool monlist_check_object_grouping(int idx, int group_by)
{
	/* Get the monster */
	object_type *o_ptr = &o_list[idx];

	/* Only visible objects */
	if ((o_ptr->ident & (IDENT_MARKED)) == 0) return (FALSE);

	/* Ignore objects carried by monsters */
	if (o_ptr->held_m_idx) return (FALSE);

	/* Check which type we're collecting */
	if (play_info[o_ptr->iy][o_ptr->ix] & (PLAY_SEEN))
	{
		if (group_by != MONLIST_GROUP_BY_SEEN) return (FALSE);
	}
	else if (group_by != MONLIST_GROUP_BY_AWARE) return (FALSE);

	return (TRUE);
}


/*
 * Returns the number that this index contributes
 */
int monlist_get_monster_count(int idx)
{
	(void)idx;

	return (1);
}


/*
 * Returns the number that this index contributes
 */
int monlist_get_object_count(int idx)
{
	/* Get the object */
	object_type *o_ptr = &o_list[idx];

	return (o_ptr->number);
}


/*
 * Checks if a monster should be counted for the secondary index
 */
bool monlist_check_monster_secondary(int idx)
{
	/* Get the monster */
	monster_type *m_ptr = &m_list[idx];

	/* Exclude awake monsters */
	if (!m_ptr->csleep) return (FALSE);

	return (TRUE);
}


/*
 * Checks if an object should be counted for the secondary index
 */
bool monlist_check_object_secondary(int idx)
{
	/* Get the object */
	object_type *o_ptr = &o_list[idx];

	/* Named objects are uninteresting */
	if (object_named_p(o_ptr)) return (FALSE);

	/* Uninteresting objects are uninteresting */
	if (uninteresting_p(o_ptr)) return (FALSE);

	return (TRUE);
}


/*
 * Returns an ordering value for the monster
 */
int monlist_get_monster_order(int idx, int sort_by)
{
	/* Get the monster */
	monster_type *m_ptr = &m_list[idx];

	/* Skip races at the sort distance */
	switch (sort_by)
	{
		case MONLIST_SORT_BY_DISTANCE:
			return (m_ptr->cdis);
		case MONLIST_SORT_BY_PROXIMITY:
			return (100 + r_info[m_ptr->r_idx].aaf - m_ptr->cdis);
		case MONLIST_SORT_BY_VENGEANCE:
			if (l_list[m_ptr->r_idx].deaths) return (l_list[m_ptr->r_idx].deaths + MAX_DEPTH);
			/* Fall through */
		case MONLIST_SORT_BY_DEPTH:
			return (r_info[m_ptr->r_idx].level);
		case MONLIST_SORT_BY_NOVELTY:
			if (l_list[m_ptr->r_idx].pkills >= 100) return (1);
			else if (l_list[m_ptr->r_idx].pkills) return (100 - l_list[m_ptr->r_idx].pkills);
			else if (l_list[m_ptr->r_idx].tkills >= 900) return (100);
			else return (1000 - l_list[m_ptr->r_idx].tkills);
	}

	return (0);
}


/*
 * Returns an ordering value for the object
 */
int monlist_get_object_order(int idx, int sort_by)
{
	/* Get the object */
	object_type *o_ptr = &o_list[idx];

	/* Skip races at the sort distance */
	switch (sort_by)
	{
		case MONLIST_SORT_BY_DISTANCE:
		case MONLIST_SORT_BY_PROXIMITY:
			return (distance(o_ptr->iy, o_ptr->ix, p_ptr->py, p_ptr->px));
		case MONLIST_SORT_BY_DEPTH:
		case MONLIST_SORT_BY_VENGEANCE:
		case MONLIST_SORT_BY_NOVELTY:
			if (object_named_p(o_ptr)) return (k_info[o_ptr->k_idx].level);
			else return (MAX_DEPTH + o_ptr->tval);
	}

	return (0);
}


/*
 * This creates a new monlist and copies the monster information to it.
 */
monlist_type *monlist_copy_monster_to_screen(int idx, u16b *index_counts, u16b *index2_counts)
{
	/* Get the monster */
	monster_type *m_ptr = &m_list[idx];

	/* Get monster race */
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Get the monster name */
	const char *m_name = r_name + r_ptr->name;

	char buf[80];

	monlist_type *new_monlist;

	/* Get a new monster list */
	new_monlist = C_ZNEW(1, monlist_type);

	/* Display multiple monsters */
	if (index_counts[m_ptr->r_idx] > 1)
	{
		/* Add race count */
		my_strcpy(buf, format("%d %ss", index_counts[m_ptr->r_idx]), sizeof(buf));

		if ((index2_counts[m_ptr->r_idx]) && (index2_counts[m_ptr->r_idx] < index_counts[m_ptr->r_idx]))
		{
			/* Add race count */
			my_strcat(buf, format(" (%d awake)", index_counts[m_ptr->r_idx] - index2_counts[m_ptr->r_idx]), sizeof(buf));
		}
	}
	/* Display single monsters */
	else
	{
		/* Display the entry itself */
		my_strcpy(buf, m_name, sizeof(buf));
	}

	/* Set up the screen line */
	new_monlist->row_type = MONLIST_RACE;
	new_monlist->number = index_counts[m_ptr->r_idx];
	new_monlist->x_attr = r_ptr->x_attr;
	new_monlist->x_char = r_ptr->x_char;

	/* Exactly one - we can target it */
	if (index_counts[m_ptr->r_idx] == 1)
	{
		new_monlist->closest_y = m_ptr->fy;
		new_monlist->closest_x = m_ptr->fx;
	}

	/* Colour text based on wakefulness */
	new_monlist->attr = index2_counts[m_ptr->r_idx] == index_counts[m_ptr->r_idx] ? TERM_SLATE :
		(index2_counts[m_ptr->r_idx] ? TERM_L_WHITE : TERM_WHITE);

	/* Add the buffer */
	monlist_copy_buffer_to_screen(buf, new_monlist);

	/* Return the space taken up */
	return (new_monlist);
}


/*
 * This copies an object to the screen line
 */
monlist_type *monlist_copy_object_to_screen(int idx, u16b *index_counts, u16b *index2_counts)
{
	/* Fake object */
	object_type object_type_body;

	/* Get the fake object */
	object_type *i_ptr = &object_type_body;

	/* Get the object */
	object_type *o_ptr = &o_list[idx];

	char buf[80];

	monlist_type *new_monlist;

	/* Get a new monster list */
	new_monlist = C_ZNEW(1, monlist_type);

	/* Prepare a fake object */
	object_prep(i_ptr, o_ptr->k_idx);

	/* Fake the artifact */
	if (object_named_p(o_ptr) && o_ptr->name1)
	{
		i_ptr->name1 = o_ptr->name1;
	}

	/* Fake the number */
	else
	{
		if (index_counts[o_ptr->k_idx] > 99) i_ptr->number = 99;
		i_ptr->number = index_counts[o_ptr->k_idx];
	}

	/* Describe the object */
	object_desc(buf, sizeof(buf), i_ptr, TRUE, 0);

	/* Add information about how many known */
	if ((index2_counts[o_ptr->k_idx]) && (index2_counts[o_ptr->k_idx] < index_counts[o_ptr->k_idx]))
	{
		/* Add race count */
		my_strcat(buf, format(" (%d unknown)", index2_counts[o_ptr->k_idx]), sizeof(buf));
	}

	/* Set up the screen line */
	new_monlist->row_type = MONLIST_KIND;
	new_monlist->number = index_counts[o_ptr->k_idx];
	new_monlist->x_attr = object_attr(o_ptr);
	new_monlist->x_char = object_char(o_ptr);

	/* Exactly one - we can target it */
	if (index_counts[o_ptr->k_idx] == o_ptr->number)
	{
		new_monlist->closest_y = o_ptr->iy;
		new_monlist->closest_x = o_ptr->ix;
	}

	/* Colour text based on known-ness */
	new_monlist->attr = index2_counts[o_ptr->k_idx] == index_counts[o_ptr->k_idx] ? TERM_WHITE :
		(index2_counts[o_ptr->k_idx] ? TERM_L_WHITE : TERM_SLATE);

	/* Add the buffer */
	monlist_copy_buffer_to_screen(buf, new_monlist);

	/* Return the space taken up */
	return (new_monlist);
}


/*
 * Creates a list of sorted indexes
 */
void *monlist_sort_index(int sort_by, int max, int idx_max, monlist_type **monlist_groups, bool *intro,
		const char* index_name, int header_offset, int monlist_get_index(int idx), int monlist_get_count(int idx),
		bool monlist_check_grouping(int idx, int group_by), bool monlist_check_secondary(int idx),
		int monlist_get_order(int idx, int sort_by),
		monlist_type *monlist_copy_to_screen(int idx, u16b *index_counts, u16b *index2_counts)
)
{
	int idx;

	int i, j;
	int status_count;
	int max_order;

	char buf[80];

	u16b *index_counts;
	u16b *index2_counts;

	monlist_type *last_monlist;
	monlist_type *monlist;

	/* Allocate the counter arrays */
	index_counts = C_ZNEW(idx_max, u16b);
	index2_counts = C_ZNEW(idx_max, u16b);

	/* Go through the groupings for the monster list */
	for (i = 0; i < MONLIST_GROUP_BY_MAX; i++)
	{
		/* Start of list */
		last_monlist = monlist_groups[header_offset * MONLIST_GROUP_BY_MAX + i];

		/* Reset status count */
		status_count = 0;
		max_order = 0;

		/* Iterate over index list */
		for (idx = 1; idx < max; idx++)
		{
			/* Not in this grouping */
			if (!monlist_check_grouping(idx, i)) continue;

			/* Bump the count for this index */
			index_counts[monlist_get_index(idx)] += monlist_get_count(idx);

			/* Bump the secondary count if qualifies */
			if (monlist_check_secondary(idx)) index2_counts[monlist_get_index(idx)] += monlist_get_count(idx);

			/* We have an index */
			status_count++;

			/* Efficiency - Get maximum sort by */
			max_order = MAX(max_order, monlist_get_order(idx, sort_by));
		}

		/* No visible monsters */
		if (!status_count) continue;

		/* Get a new monlist */
		monlist = C_ZNEW(1, monlist_type);

		/* Set up the buffer */
		my_strcpy(buf, format("You %s%s %d %s%s:%s",
			(i < 2) ? "can see" : "are aware of", (i == 1) ? " but not shoot" : "",
			status_count, index_name, (status_count > 1 ? "s" : ""),
			*intro ? format(" (by %s)", sort_by_name[sort_by]) : ""), sizeof(buf));

		/* Copy the buffer */
		monlist_copy_buffer_to_screen(buf, monlist);

		/* Set up the screen line */
		monlist->row_type = MONLIST_HEADER_SEEN_MONSTER + header_offset * MONLIST_GROUP_BY_MAX + i;
		monlist->number = status_count;
		monlist->attr = TERM_WHITE;

		/* Head of list? */
		if (!last_monlist)
		{
			last_monlist = monlist;
		}
		/* Add as child */
		else
		{
			last_monlist->next = monlist;
		}

		/* Thread it */
		last_monlist = monlist;

		/* Iterate through sort */
		for (j = sort_by ? max_order : 0; (sort_by ? j >= 0 : j <= max_order); sort_by ? j-- : j++)
		{
			/* Iterate over mon_list ( again :-/ ) */
			for (idx = 1; idx < max; idx++)
			{
				/* Do each race only once */
				if (!index_counts[monlist_get_index(idx)]) continue;

				/* Check the monster is valid */
				if (!monlist_check_grouping(idx, j)) continue;

				/* Copy to the screen */
				monlist = monlist_copy_to_screen(idx, index_counts, index2_counts);

				/* Add as child */
				last_monlist->next = monlist;

				/* Thread it */
				last_monlist = monlist;

				/* Don't display again */
				index_counts[monlist_get_index(idx)] = 0;
				index2_counts[monlist_get_index(idx)] = 0;
			}
		}

		/* Get a new monlist */
		monlist = C_ZNEW(1, monlist_type);

		/* Set up the screen line */
		monlist->row_type = MONLIST_BLANK;

		/* Add as child */
		last_monlist->next = monlist;

		/* Thread it */
		last_monlist = monlist;

		/* Introduced */
		*intro = FALSE;
	}

	/* Free the counters */
	FREE(index_counts);
	FREE(index2_counts);
}


/* Display the list */
bool display_monlist_rows(monlist_type *monlist, int row, int line, int width, bool force)
{
	int i;

	/* If displaying on the terminal using the '[' command, recenter on the player,
	 * taking away the used up width on the left hand side. Otherwise, recenter when
	 * the player walks next to the display box. */
	if ((Term == angband_term[0]) && ((signed)width < SCREEN_WID) &&
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
		if ((!force) && (p_ptr->px - p_ptr->wx <= (signed)width + 1) && (p_ptr->py - p_ptr->wy <= (signed)line + 1)) return (TRUE);

		screen_save();
	}

	/* Print the display */
	for (i = row; i < line; i++)
	{
		/* Display the line */
		c_prt(monlist->attr, monlist->text, row + i, 0);

		/* Get the next in the list */
		monlist = monlist->next;
	}

	return (FALSE);
}


/* Interact with the list */
key_event display_monlist_interact(monlist_type *monlist, int row, int line, int width, bool *done, bool force)
{
	/* Get an acceptable keypress. */
	key_event ke = force ? anykey() : inkey_ex();

	while ((ke.key == '\xff') && !(ke.mousebutton))
	{
		int y = KEY_GRID_Y(ke);
		int x = KEY_GRID_X(ke);

		int room = dun_room[p_ptr->py/BLOCK_HGT][p_ptr->px/BLOCK_WID];

		ke = target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
	}

	/* Tried a command - avoid rest of list */
	if (ke.key != ' ')
	{
		*done = TRUE;
	}

	return (ke);
}


/*
 * Display visible monsters and/or objects in a window
 *
 * Row is the row to start displaying the list from.
 * Command indicates we return the selected command.
 * Force indicates we're not doing this from the easy_monlist option (should
 * probably rename this).
 *
 * op_ptr->monlist_display defines whether we see monsters, objects or both:
 *
 * 1 	- monsters
 * 2	- objects
 * 3	- both
 *
 * op_ptr->monlist_sort_by shows which way we have sorted this:
 *
 * 0	- distance
 * 1	- depth
 * 2	- number of times your ancestor has been killed
 *
 * Returns the width of the monster and/or object lists, or 0 if no monsters/objects are seen.
 *
 */
void display_monlist(int row, bool command, bool force)
{
	int line = row, max_row;
	int top = 0;
	int group = 0;

	int sort_by = op_ptr->monlist_sort_by;

	bool intro = TRUE;
	bool done = FALSE;
	int disp_count, total_count;
	int width;

	key_event ke;

	monlist_type *monlist_groups[MONLIST_GROUP_BY_MAX * 3];
	monlist_type *monlist_screen;
	monlist_type *monlist;

	int i, j;

	char buf[80];

	/* Hack -- initialise for the first time */
	if (!op_ptr->monlist_display)
	{
		op_ptr->monlist_display = (MONLIST_DISPLAY_MONSTER) | (MONLIST_DISPLAY_OBJECT);
		sort_by = MONLIST_SORT_BY_VENGEANCE;
	}

	/* Clear the term if in a subwindow, set x otherwise */
	if (Term != angband_term[0])
	{
		clear_from(0);
		max_row = Term->hgt - 1;
	}
	else
	{
		max_row = Term->hgt - 2;

		screen_save();
	}

	/* If hallucinating, we can't see any monsters */
	if (p_ptr->timed[TMD_IMAGE])
	{
		Term_putstr(0, line, 36, TERM_ORANGE, "You're too confused to see straight! ");
		return;
	}

	/*
	 * Iterate multiple times. We put monsters we can project to in the first list, then monsters we can see,
	 * then monsters we are aware of through other means.
	 */
	if (op_ptr->monlist_display % MONLIST_DISPLAY_MONSTER)
	{
		/* Add to the list */
		monlist_sort_index(sort_by, z_info->m_max, z_info->r_max, monlist_groups, &intro, "monster", 0,
				monlist_get_monster_index, monlist_get_monster_count, monlist_check_monster_grouping,
				monlist_check_monster_secondary, monlist_get_monster_order, monlist_copy_monster_to_screen);

#if 0
		/* Hack -- no ancestor deaths - demote difficulty */
		if ((sort_by == MONLIST_SORT_BY_VENGEANCE) && (max_order <= MAX_DEPTH))
		{
			sort_by = MONLIST_SORT_BY_DEPTH;

			/* We've used the '[' command -- apply this to the character options */
			if (force) op_ptr->monlist_sort_by = MONLIST_SORT_BY_DEPTH;
		}
#endif
	}

	/* Display items */
	if ((!done) && (op_ptr->monlist_display & (MONLIST_DISPLAY_OBJECT)))
	{
		/* Add to the list */
		monlist_sort_index(sort_by, z_info->o_max, z_info->k_max, monlist_groups, &intro, "object", 0,
				monlist_get_object_index, monlist_get_object_count, monlist_check_object_grouping,
				monlist_check_object_secondary, monlist_get_object_order, monlist_copy_object_to_screen);
	}
#if 0
	/* Display features */
	if ((!done) && (op_ptr->monlist_display & (MONLIST_DISPLAY_FEATURE)))
	{
		/* Add to the list */
		monlist_sort_index(sort_by, MAX_RANGE * MAX_RANGE * 4, z_info->f_max, monlist_groups, &intro, "notable feature", 0,
				monlist_get_feature_index, monlist_get_feature_count, monlist_check_feature_grouping,
				monlist_check_feature_secondary, monlist_get_feature_order, monlist_copy_feature_to_screen);
	}
#endif


	/* Allocate space for the screen display */
	monlist_screen = C_ZNEW(max_row - row, monlist_type);

	/* Loop */
	while (!done)
	{
		/* Restart from top */
		total_count = 0;
		disp_count = 0;
		group = 0;
		j = 0;
		width = 0;

		/* Copy monster list to screen */
		for (i = 0, group = 0; group < MONLIST_GROUP_BY_MAX * 3; )
		{
			/* Get first in the group */
			monlist = monlist_groups[group++];

			/* Get monster list */
			while ((monlist) && (i < max_row - row))
			{
				/* Add to the screen */
				if (j >= top)
				{
					COPY(monlist_screen, monlist, monlist_type);
					disp_count += monlist->number;
					width = MAX(width, monlist->len);
				}

				/* Increase total count */
				total_count += monlist->number;

				/* Get the next monster */
				monlist = monlist->next;
				i++; j++;
			}

			/* Get remaining undisplayed */
			while (monlist)
			{
				total_count += monlist->number;
				monlist = monlist->next;
			}
		}

		/* Print "--- more ---" prompt if required */
		if ((Term == angband_term[0]) && (disp_count != total_count))
		{
			/* Format the others */
			my_strcpy(buf, "-- more --", sizeof(buf));

			/* Copy the buffer */
			monlist_copy_buffer_to_screen(buf, &monlist_screen[i]);

			/* Set up the screen line */
			monlist_screen[i].row_type = MONLIST_MORE;
			monlist_screen[i].attr = TERM_WHITE;

			/* Fake backing up. This allows us to print the 'others' prompt below. */
			i--;
		}
		/* Put a shadow */
		else
		{
			monlist_screen[i].row_type = MONLIST_BLANK;
			i++;
		}

		/* Print "and others" message if we're out of space */
		if (disp_count != total_count)
		{
			/* XXX Back up one */
			i--;
			disp_count -= monlist_screen[i].number;

			/* Format the others */
			my_strcpy(buf, format("  ...and %d others.", total_count - disp_count), sizeof(buf));

			/* Copy the buffer */
			monlist_copy_buffer_to_screen(buf, &monlist_screen[i]);

			/* Set up the screen line */
			monlist_screen[i].row_type = monlist_screen[i-1].row_type + MONLIST_GROUP_BY_MAX * 3;
			monlist_screen[i].number = total_count - disp_count;
			monlist_screen[i].attr = TERM_WHITE;

			/* We've displayed the others */
			disp_count = total_count;

			/* Increase line */
			i++;
		}

		/* Notice nothing */
		if (!total_count)
		{
			prt(format("You see no %s%s%s%s%s. (by %s)",
					op_ptr->monlist_display & (MONLIST_DISPLAY_MONSTER) ? "monsters" : "",
					(op_ptr->monlist_display & (MONLIST_DISPLAY_OBJECT | MONLIST_DISPLAY_MONSTER)) ==
						(MONLIST_DISPLAY_OBJECT | MONLIST_DISPLAY_MONSTER) ?" or " : "",
					op_ptr->monlist_display & (MONLIST_DISPLAY_OBJECT) ? "objects" : "",
					(op_ptr->monlist_display & (MONLIST_DISPLAY_OBJECT | MONLIST_DISPLAY_FEATURE)) ==
						(MONLIST_DISPLAY_OBJECT | MONLIST_DISPLAY_FEATURE) ?" or " : "",
					op_ptr->monlist_display & (MONLIST_DISPLAY_OBJECT) ? "notable features" : "",
					sort_by_name[sort_by]), row, 0);

			done = TRUE;
		}
		else
		{
			/* Display the list */
			done = display_monlist_rows(monlist_screen, row, row + i, width, force);
		}

		/* Interact further with the list */
		if (Term == angband_term[0])
		{
			ke = display_monlist_interact(monlist_screen, row, line, width, &done,force);
		}
		else
			done = TRUE;
	}

	/* Note we don't free the text here */
	FREE(monlist_screen);

	/* Release monster groups */
	for (i = 0; i < MONLIST_GROUP_BY_MAX * 3; i++)
	{
		free_monlist(monlist_groups[i]);
	}
}





