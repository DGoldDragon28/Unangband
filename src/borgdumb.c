/* File: borgdumb.c */

/* Code for the "mindless borg", an aid to profiling the game.
 *
 * Copyright (c) 2001
 * Leon Marrick, Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */



#include "angband.h"

#ifdef ALLOW_BORG

/*
 * Global variables
 */
u32b count_stop = 0L;
int change_level = 0;
int count_change_level = 0;
int count_teleport = 0;
byte allowed_depth[2] = { 0, 0 };
byte borg_dir;


/*
 * Run the "Mindless Borg".
 */
void do_cmd_borg(void)
{
	int i, j;
	int y, x;
	int feat;
	int dir_good[8];

	bool fear = FALSE;

	/* Start the borg. */
	if (count_stop == 0)
	{
		char ch[80] = "";

		/* Query */
		if (!get_string("How many turns shall the borg play for?", ch, 9)) return;

		/* Set timer */
		count_stop = 1L + (long)atol(ch);

		/* Query */
		if (!get_string("How many turns shall the borg stay on a level for (0 for random)?", ch, 9)) return;

		/* Set timer */
		change_level = 1L + (long)atol(ch);

		/* Query */
		if (!get_string("What is the minimun level that the borg should travel on?", ch, 3)) return;

		/* Set minimum depth */
		allowed_depth[0] = atoi(ch);
		/* if (allowed_depth[0] < 0) allowed_depth[0] = 0; */
		if (allowed_depth[0] >= MAX_DEPTH) allowed_depth[0] = MAX_DEPTH - 1;

		/* Query */
		if (!get_string("What is the maximum level that the borg should travel on?", ch, 3)) return;

		/* Set maximum depth */
		allowed_depth[1] = atoi(ch);
		/* if (allowed_depth[1] < 0) allowed_depth[1] = 0; */
		if (allowed_depth[1] >= MAX_DEPTH) allowed_depth[1] = MAX_DEPTH - 1;

		/* Check sanity. */
		if (allowed_depth[1] < allowed_depth[0])
		{
			msg_print("clearing all level restrictions.");
			allowed_depth[0] = 0;
			allowed_depth[1] = MAX_DEPTH - 1;

		}

		/* Change level immediately, wait a bit before teleporting. */
		count_change_level = 0;
		count_teleport = rand_range(100, 150);

		/* Pick a direction of travel at random */
		borg_dir = (byte)rand_int(8);
	}

	/* Stop when needed. */
	count_stop--;
	if (count_stop == 0)
	{
		/* Redraw everything */
		do_cmd_redraw();

		return;
	}
	else
	{
		/* The borg has a light radius of three. */
		p_ptr->cur_lite = 3;

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
			set_timed(i, 0, FALSE);
		}

		/* Fully healed */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;

		/* No longer hungry */
		p_ptr->food = PY_FOOD_MAX - 1;

		/* No longer tired */
		p_ptr->rest = PY_REST_MAX - 1;
	}

	/* Do not wait */
	inkey_scan = TRUE;

	/* Check for a key, and stop the borg if one is pressed */
	if (inkey())
	{
		/* Flush input */
		flush();

		/* Disturb */
		disturb(0, 0);

		/* Stop the borg */
		msg_print("stopping...");
		count_stop = 0;

		/* Redraw everything */
		do_cmd_redraw();

		return;
	}


	/* Take a turn */
	p_ptr->energy_use = 100 - (p_ptr->depth / 2);

	/* Refresh the screen */
	Term_fresh();

	/* Delay the borg */
	Term_xtra(TERM_XTRA_DELAY, op_ptr->delay_factor * op_ptr->delay_factor);


	/* Change level when needed. */
	if (count_change_level > 0) count_change_level--;
	else
	{
		/* Jump around dungeons */
		if (adult_campaign)
		{
			/* Skip towns */
			while (TRUE)
			{
				/* Get dungeon zone */
				dungeon_zone *zone=&t_info[0].zone[0];

				/* New dungeon */
				p_ptr->dungeon = (s16b)rand_int(z_info->t_max);

				/* New depth */
				p_ptr->depth = (s16b)rand_range(min_depth(p_ptr->dungeon), max_depth(p_ptr->dungeon));
				
				/* Get the zone */
				get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

				if ((zone->flags1 & (LF1_TOWN)) == 0) break;
			}

		}
		else
		{
			/* New depth */
			p_ptr->depth = (s16b)rand_range(allowed_depth[0], allowed_depth[1]);
		}

		/* Leaving */
		p_ptr->leaving = TRUE;

		if (change_level) count_change_level = change_level;
		else count_change_level = rand_range(1000, 2500);
		return;
	}

	/* Teleport when needed. */
	if (count_teleport > 0) count_teleport--;
	else
	{
		teleport_hook = NULL;
		teleport_player(rand_range(20, 200));
		count_teleport = rand_range(100, 150);
		return;
	}

	/* Look up to a distance of four for monsters to attack. */
	for (i = 1; i <= 4; i++)
	{
		for (y = p_ptr->py - i; y <= p_ptr->py + i; y++)
		{
			for (x = p_ptr->px - i; x <= p_ptr->px + i; x++)
			{
				if ((in_bounds(y, x)) && (cave_m_idx[y][x] > 0))
				{
					monster_type *m_ptr = &m_list[cave_m_idx[y][x]];

					if (m_ptr->ml)
					{
						/* Wizard zap the monster. */
						mon_take_hit(cave_m_idx[y][x], p_ptr->depth * 2, &fear, NULL);
						return;
					}
				}
			}
		}
	}

	/*
	 * Look at next grid.  Assume the dungeon to be surrounded by a
	 * wall that the character cannot pass.
	 */
	y = p_ptr->py + ddy_ddd[borg_dir];
	x = p_ptr->px + ddx_ddd[borg_dir];

	/*
	 * If grid in our current direction of travel is not passable, or
	 * sometimes even if it is, choose a passable adjacent grid at random.
	 */
	if ((cave_feat[y][x] > FEAT_RUBBLE) ||
	   ((cave_info[y][x] & (CAVE_ROOM)) && (rand_int(8) == 0)))
	{
		/* Look all around */
		for (j = 0, i = 0; i < 8; i++)
		{
			y = p_ptr->py + ddy_ddd[i];
			x = p_ptr->px + ddx_ddd[i];

			/* Accept any passable grid. */
			if (cave_feat[y][x] <= FEAT_RUBBLE)
			{
				/* Store good directions. */
				dir_good[j] = i;
				j++;
			}
		}

		/* If we're totally trapped, teleport away (later). */
		if (j == 0)
		{
			count_teleport = 0;
			return;
		}

		/* Choose a direction at random from our list. */
		borg_dir = dir_good[rand_int(j)];

		/* look at grid in chosen direction. */
		y = p_ptr->py + ddy_ddd[borg_dir];
		x = p_ptr->px + ddx_ddd[borg_dir];
	}

	/* Get the feature in the grid. */
	feat = cave_feat[y][x];

	/* Move into terrain that need not be altered. */
	if (f_info[feat].flags1 & (FF1_MOVE))
	{
		monster_swap(p_ptr->py, p_ptr->px, y, x);
	}

	/* Open doors. */
	else if (f_info[feat].flags1 & (FF1_OPEN))
	{
		cave_set_feat(y, x, FEAT_OPEN);
	}

	/* Disarm traps. */
	else if (f_info[feat].flags1 & (FF1_DISARM))
	{
		cave_set_feat(y, x, FEAT_FLOOR);
	}

	/* Remove anything at all we can move onto. */
	else if (f_info[feat].flags3 & (FF3_EASY_CLIMB))
	{
		cave_set_feat(y, x, FEAT_FLOOR);
	}

	/* If the terrain is anything else, do nothing. */
}
#else	/* ALLOW_BORG */

#ifdef MACINTOSH
static int i = 0;
#endif

#endif	/* ALLOW_BORG */
