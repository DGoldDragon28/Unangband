/* File: cmd2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.	Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.1
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"

/*
 * Check if action permissable here.
 */

bool do_cmd_test(int y, int x, int action)
{
	u32b bitzero = 0x01;
	u32b flag;

	cptr act;

	cptr here = (((p_ptr->px == x ) && (p_ptr->py == y))?"here":"there");

	feature_type *f_ptr;
	int feat;

	if (disturb_detect && (play_info[p_ptr->py][p_ptr->px] & (PLAY_SAFE)) && !(play_info[y][x] & (PLAY_SAFE)))
	{
		disturb(1,0);
		msg_print("This doesn't feel safe.");

		if (!get_check("Are you sure you want to enter undetected territory?")) return (FALSE);

		/* Hack -- mark the target safe */
		play_info[y][x] |= (PLAY_SAFE);
	}

	/* Must have knowledge */
	if (!(play_info[y][x] & (PLAY_MARK)))
	{
		/* Message */
		msg_format("You see nothing %s.",here);

		/* Nope */
		return (FALSE);
	}

	/* Get memorised feature */
	feat = f_info[cave_feat[y][x]].mimic;

	f_ptr = &f_info[feat];

	act=NULL;

	switch (action)
	{
		case FS_SECRET: break;
		case FS_OPEN: act=" to open"; break;
		case FS_CLOSE: act=" to close"; break;
		case FS_BASH: act=" to bash"; break;
		case FS_DISARM: act=" to disarm"; break;
		case FS_SPIKE: act=" to spike"; break;
		case FS_ENTER: act=" to enter"; break;
		case FS_TUNNEL: act=" to tunnel"; break;
		case FS_LESS: act=" to climb up"; break;
		case FS_MORE: act=" to climb down"; break;
		case FS_RUN: act=" to run on"; break;
		case FS_KILL_MOVE: act=" to disturb"; break;
		case FS_FLOOR:
		case FS_GROUND: act=" to set a trap on"; break;
		default: break;
	}

	if (action < FS_FLAGS2)
	{
		flag = bitzero << (action - FS_FLAGS1);
		if ((!(f_ptr->flags1 & flag))
			/* Hack -- allow traps to be set on the ground */
			&& (((flag & (FF1_FLOOR)) == 0)
			|| ((f_ptr->flags3 & (FF3_GROUND)) == 0)))
		{
		 msg_format("You see nothing %s%s.",here,act);
		 return (FALSE);
		}
	}

	else if (action < FS_FLAGS3)
	{
		flag = bitzero << (action - FS_FLAGS2);
		if (!(f_ptr->flags2 & flag))
		{
		 msg_format("You see nothing %s%s.",here,act);
		 return (FALSE);
		}
	}

	else if (action < FS_FLAGS_END)
	{
		flag = bitzero << (action - FS_FLAGS3);
		if (!(f_ptr->flags3 & flag))
		{
		 msg_format("You see nothing %s%s.",here,act);
		 return (FALSE);
		}
	}

	/* Player is sneaking */
	if (p_ptr->sneaking)
	{
		/* If the player is trying to be stealthy, they lose the benefit of this for
		 * the following turn.
		 */
		p_ptr->not_sneaking = TRUE;
	}

	return (TRUE);

}


/*
 * Print a list of routes (for travelling).
 */
void print_routes(const s16b *route, int num, int y, int x)
{
	int i, town;

	char out_val[60];

	byte line_attr;

	/* Title the list */
	prt("", y, x);
	put_str("Location", y, x + 5);
	put_str(" Level", y, x + 50);

	/* Dump the routes */
	for (i = 0; i < num; i++)
	{
		char str[46];

		/* Get the town index */
		town = route[i];

		long_level_name(str, town, 0);

		line_attr = TERM_WHITE;

		/* Dump the route --(-- */
		sprintf(out_val, "  %c) %-45s %2d%3s ",
			I2A(i), str, min_depth(town), max_depth(town) > min_depth(town) ? format("-%-2d", max_depth(town)) : "");
		c_prt(line_attr, out_val, y + i + 1, x);
	}


	/* Clear the bottom line */
	prt("", y + i + 1, x);
}



/*
 * Other research field functions.
 */
bool route_commands(char choice, const s16b *sn, int i, bool *redraw)
{
	(void)sn;
	(void)i;

	switch (choice)
	{
		case 'L':
		{
			/* Save screen */
			if (!(*redraw)) screen_save();

			/* Do knowledge */
			do_knowledge_dungeons();

		    /* Load screen */
		    screen_load();

		    break;
		}

		case 'M':
		{
			/* Save screen */
			if (!(*redraw)) screen_save();

			(void)show_file("memap.txt", NULL, 0, 0);

			/* Load screen */
			screen_load();

			break;
		}

		default:
		{
			return (FALSE);
		}
	}
	return (TRUE);
}


/*
 * This gives either the dungeon, or a replacement one if it is defined.
 */
int actual_route(int dun)
{
	while(t_info[dun].replace_ifvisited && t_info[t_info[dun].replace_ifvisited].visited)
	{
		dun = t_info[dun].replace_with;
	}

	return (dun);
}


/*
 * Determine if the object is a reasonable auto-consume food
 * and has no "!<" in its inscription.
 * Also, if it has "=<' it will be always eaten, reasonable or not.
 */
static bool auto_consume_okay(const object_type *o_ptr)
{
	cptr s;

	/* Inedible */
	if (!item_tester_hook_food_edible(o_ptr)) return (FALSE);

	/* Hack -- normal food is fine except gorging meat, etc. */
	/* You can inscribe them with !< to prevent this however */
	if ((o_ptr->tval == TV_FOOD)
		 && (o_ptr->sval != SV_FOOD_WAYBREAD)
		 && (o_ptr->sval != SV_FOOD_SPRIG_OF_ATHELAS)
		 && (o_ptr->sval != SV_FOOD_COOKED_MEET)
		 && (o_ptr->sval != SV_FOOD_PINT_OF_SPIRITS))
	{
		/* No inscription */
		if (!o_ptr->note) return (TRUE);

		/* Find a '!' */
		s = strchr(quark_str(o_ptr->note), '!');

		/* Process inscription */
		while (s)
		{
			/* Not auto-consume on "!<" */
			if (s[1] == '<')
			{
				/* Pick up */
				return (FALSE);
			}

			/* Find another '=' */
			s = strchr(s + 1, '!');
		}

		return (TRUE);
	}

	/* No inscription */
	if (!o_ptr->note) return (FALSE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Auto-consume on "=<" */
		if (s[1] == '<')
		{
			/* Pick up */
			return (TRUE);
		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Don't auto consume */
	return (FALSE);
}


/*
 * Set routes
 *
 * Set up the possible routes from this location.
 *
 * Returns number of routes set up.
 */
int set_routes(s16b *routes, int max_num, int from)
{
	town_type *t_ptr = &t_info[from];
	dungeon_zone *zone1 = &t_ptr->zone[0];
	dungeon_zone *zone2 = &t_ptr->zone[0];

	int i, ii, max, num = 0;

	/* Get the top of the dungeon */
	get_zone(&zone1,from,min_depth(from));

	/* Get the bottom of the dungeon */
	get_zone(&zone2,from,max_depth(from));

	/* Campaign mode gets standard routes */
	if (adult_campaign)
	{
		/* Add nearby route */
		for (i = 0; i < MAX_NEARBY; i++)
		{
			if (t_ptr->nearby[i])
			{
				routes[num++] = actual_route(t_ptr->nearby[i]);
			}
		}

		/* Add quest route if possible */
		if ((t_ptr->quest_monster) && (!r_info[t_ptr->quest_monster].max_num) && (t_ptr->quest_opens))
		{
				routes[num++] = actual_route(t_ptr->quest_opens);
		}
	}

	/* Add maps if not in Moria or other level with no exits */
	if (num)
	/* or only Moria --- if (t_ptr->store_index[0] != 805) */
	{
		for (i = 0; (i < INVEN_WIELD) && (num < max_num); i++)
		{
			/* Check for maps */
			if ((inventory[i].k_idx) && (inventory[i].tval == TV_MAP))
			{
                                if(inventory[i].pval && (inventory[i].pval != p_ptr->dungeon)) continue;
                                /* Add map routes */
				routes[num++] = actual_route(inventory[i].sval);
			}

			/* Check for bags for maps */
			else if (inventory[i].tval == TV_BAG)
			{
				/* Scan the bag */
				for (ii = 0; ii < INVEN_BAG_TOTAL; ii++)
				{
					/* Slot holds a map */
					if ((bag_holds[inventory[i].sval][ii][0] == TV_MAP) 
                                                && (bag_contents[inventory[i].sval][ii])
                                                && (bag_contents[inventory[i].sval][ii])
                                                && (((k_info[lookup_kind(TV_MAP, 
                                                bag_contents[inventory[i].sval][ii])].pval)
                                                == p_ptr->dungeon) || ((k_info[lookup_kind(TV_MAP, 
                                                bag_contents[inventory[i].sval][ii])].pval) == 0)))
					{
						/* Add route */
						routes[num++] = actual_route(bag_holds[inventory[i].sval][ii][1]);
					}
				}
			}
		}
	}

	/* Always can travel back to Angband if not in campaign mode */
	if (!adult_campaign)
	{
		/* Add route to Angband */
		if (num < max_num) routes[num++] = z_info->t_max - 2;

		/* Paranoia - add a route back, even if out of space */
		else if (p_ptr->dungeon != z_info->t_max - 2)
			routes[0] = z_info->t_max - 2;
	}

/* This next section removed because it may 'confuse' people too much
 * e.g. routes add and disappear 'at random' */
#if 0
	/* Add routes further away if visited, while in campaign mode */
	if (adult_campaign) for (i = 0; i < num; i++)
	{
		/* Only add routes from visited locations */
		if (!t_info[routes[i]].visited) continue;

		/* Add nearby routes */
		for (ii = 0; (ii < MAX_NEARBY) && (num < max_num); ii++)
		{
			/* Skip non-existent routes or circular roots */
			if (!(t_info[routes[i]].nearby[ii]) || (t_info[routes[i]].nearby[ii] == p_ptr->dungeon)) continue;

			/* Add the route */
			routes[num++] = actual_route(t_info[routes[i]].nearby[ii]);
		}

		/* Add quest route if possible */
		if ((num < max_num) &&
				(t_info[routes[i]].quest_monster) && (!r_info[t_info[routes[i]].quest_monster].max_num) && (t_info[routes[i]].quest_opens))
		{
				routes[num++] = actual_route(t_info[routes[i]].quest_opens);
		}

		/* Scan and remove duplicate routes and routes looping back to start */
		for (ii = i + 1; ii < num; ii++)
		{
			if (routes[ii] == p_ptr->dungeon) routes[ii] = routes[--num];

			for (iii = ii; iii < num; iii++)
			{
				if (routes[iii] == routes[ii])
				{
					routes[iii] = routes[--num];
				}
			}
		}
	}
#endif

	/* Scan and remove duplicate routes and routes looping back to start */
	for (i = 0; i < num; i++)
	{
		if (routes[i] == from) routes[i] = routes[--num];

		for (ii = i + 1; ii < num; ii++)
		{
			if (routes[ii] == routes[i])
			{
				routes[ii] = routes[--num];
			}
		}
	}

	/* Sort the routes in order */
	for(i = 0; i < num; i++)
	{
		int ii;
		s16b temp;

		max = i;
		for(ii = i; ii < num; ii++)
		{
			if(routes[max] > routes[ii])
			{
				max = ii;
			}
		}

		temp = routes[max];
		routes[max] = routes[i];
		routes[i] = temp;
	}

	/* Return number of routes */
	return(num);
}


/*
 * Travel to a different dungeon.
 *
 * This whole thing is a hack -- I haven't decided how elegant it is yet.
 */
static void do_cmd_travel(void)
{
	town_type *t_ptr = &t_info[p_ptr->dungeon];
	dungeon_zone *zone = &t_ptr->zone[0];

	int i, num = 0;

	int journey = 0;

	int by = p_ptr->py / BLOCK_HGT;
	int bx = p_ptr->px / BLOCK_WID;

	bool edge_y = ((by < 2) || (by > ((DUNGEON_HGT/BLOCK_HGT)-3)));
	bool edge_x = ((bx < 2) || (bx > ((DUNGEON_WID/BLOCK_WID)-3)));

	char str[46];

	current_long_level_name(str);

	/* Get the top of the dungeon */
	get_zone(&zone,p_ptr->dungeon,min_depth(p_ptr->dungeon));

	if (level_flag & (LF1_SURFACE))
	{
		if (p_ptr->timed[TMD_BLIND])
		{
			msg_print("You can't read any maps.");
		}
		else if (p_ptr->timed[TMD_CONFUSED])
		{
			msg_print("You are too confused.");
		}
		else if (p_ptr->timed[TMD_PETRIFY])
		{
			msg_print("You are petrified.");
		}
		else if (p_ptr->timed[TMD_AFRAID])
		{
			msg_print("You are too afraid.");
		}
		else if (p_ptr->timed[TMD_IMAGE])
		{
			msg_print("The mice don't want you to leave.");
		}
		else if (((p_ptr->timed[TMD_POISONED]) && (p_ptr->timed[TMD_SLOW_POISON] == 0)) || (p_ptr->timed[TMD_CUT]) || (p_ptr->timed[TMD_STUN]))
		{
			msg_print("You need to recover from any poison, cuts or stun damage.");
		}
		else if (!edge_y && !edge_x && ((level_flag & (LF1_TOWN)) == 0))
		{
			msg_format("You need to be close to the edge of %s.", str);
		}
		else
		{
			int selection = p_ptr->dungeon;

			s16b routes[24];

			quest_event event;

			/* Use this to allow quests to succeed or fail */
			WIPE(&event, quest_event);

			/* Set up departure event */
			event.flags = EVENT_LEAVE;
			event.dungeon = p_ptr->dungeon;
			event.level = p_ptr->depth - min_depth(p_ptr->dungeon);

			/* Check for quest failure. Abort if requested. */
			if (check_quest(&event, FALSE)) return;

			/* Routes */
			num = set_routes(routes, 24, p_ptr->dungeon);

			/* Display the list and get a selection */
			if (get_list(print_routes, routes, num, "Routes", "Travel to where", ", L=locations, M=map", 1, 22, route_commands, &selection))
			{
				int found = 0;
				
				bool risk_stat_loss = FALSE;

				if (!t_info[selection].visited)
				{
					char str[1000];

					for (i = 1; i < z_info->t_max; i++)
					{
						if (t_info[i].replace_ifvisited == selection)
						{
							if (found == 0)
							{
								sprintf(str, "If you enter %s, you will never find %s", t_info[selection].name + t_name, t_info[i].name + t_name);
								found = -1;
							}
							else if (found < 0)
							{
								found = i;
							}
							else
							{
								my_strcat(str, format(", %s", t_info[found].name + t_name), 1000);
								found = i;
							}
						}
					}
					if (found > 0)
						my_strcat(str, format(" and %s again!", t_info[found].name + t_name), 1000);
					else if (found < 0)
						my_strcat(str, " again!", 1000);
					if (found != 0)
						msg_print(str);
				}

				if (found == 0
					&& count_routes(selection, p_ptr->dungeon) <= 0)
				{
					msg_print("As you set foot upon the path, you have the sudden feeling you might not be able to get back this way for a long while.");
					found = 1;
				}

				if (found != 0
					&& !get_check("Are you sure you want to travel? "))
					/* Bail out */
					return;

				/* Save the old dungeon in case something goes wrong */
				if (autosave_backup)
					do_cmd_save_bkp();

				/* Will try to auto-eat? */
				if (p_ptr->food < PY_FOOD_FULL)
				{
					msg_print("You set about filling your stomach for the long road ahead.");
				}

				/* Hack -- Consume most food not inscribed with !< */
				while (p_ptr->food < PY_FOOD_FULL)
				{
					for (i = INVEN_PACK - 1; i >= 0; i--)
					{
						/* Eat the food */
						if (auto_consume_okay(&inventory[i]))
						{
							/* Eat the food */
							player_eat_food(i);

							break;
						}
					}

					/* Escape out if no food */
					if (i < 0) break;
				}

				if (easy_more) messages_easy(FALSE);

				/* Need to be full to travel */
				if (p_ptr->food < PY_FOOD_FULL)
				{
					msg_print("You notice you don't have enough food to fully satiate you before the travel.");
					msg_print("You realize you will face the unforeseen dangers with a half-empty stomach!");

					if (!get_check("Are you sure you want to travel? "))
						/* Bail out */
						return;
					
					risk_stat_loss = TRUE;
				}

				/* Longer and more random journeys via map */
				journey = damroll(2 + (risk_stat_loss ? 1 : 0) + (level_flag & LF1_DAYLIGHT ? 1 : 0), 4);

				/* Shorter and more predictable distance if nearby */
				for (i = 0; i < MAX_NEARBY; i++)
				{
					if (t_info[p_ptr->dungeon].nearby[i] == selection) journey = damroll(1 + (risk_stat_loss ? 1 : 0) + (level_flag & LF1_DAYLIGHT ? 1 : 0), 3);
				}

				if (journey < 4)
				{
					msg_print("You have a mild and pleasant journey.");
				}
				else if (journey < 7)
				{
					msg_print("Your travels are without incident.");
				}
				else if (journey < 10)
				{
					msg_print("You have a long and arduous trip.");
				}
				else
				{
					msg_print("You get lost in the wilderness!");
					/* XXX Fake a wilderness location? */
				}

				/* Hack -- Get hungry/tired/sore */
				set_food(p_ptr->food - PY_FOOD_FULL/10*journey);

				/* Hack -- Time passes (at 4* food use rate) */
				turn += PY_FOOD_FULL/10*journey*4;
				
				/*
				 * Hack -- To prevent a player starving to death a few steps after arriving from travelling,
				 * we reduce a stat and leave them only very hungry if they would end up weak.
				 * This also prevents the player from infinitely travelling.
				 */
				while (p_ptr->food < PY_FOOD_WEAK + PY_FOOD_STARVE)
				{
					/* Decrease random stat */
					if (risk_stat_loss) do_dec_stat(rand_int(A_MAX));
					
					/* Feed the player */
					set_food(p_ptr->food + PY_FOOD_FAINT);
				}

				/* XXX Recharges, stop temporary speed etc. */
				/* We don't do this to e.g. allow the player to buff themselves before fighting Beorn. */

				/* Change the dungeon */
				p_ptr->dungeon = selection;

				/* Set the new depth */
				p_ptr->depth = min_depth(p_ptr->dungeon);

				/* Clear stairs */
				p_ptr->create_stair = 0;

				/* Leaving */
				p_ptr->leaving = TRUE;

				/* Check for quest failure. */
				while (check_quest(&event, TRUE));
			}
		}

		return;
	}
}




/*
 * Go up one level, or choose a different dungeon.
 */
void do_cmd_go_up(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	feature_type *f_ptr= &f_info[cave_feat[py][px]];

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	char str[46];

	quest_event event;

	/* Use this to allow quests to succeed or fail */
	WIPE(&event, quest_event);

	/* Set up departure event */
	event.flags = EVENT_LEAVE;
	event.dungeon = p_ptr->dungeon;
	event.level = p_ptr->depth - min_depth(p_ptr->dungeon);

	/* Check for quest failure. Abort if requested. */
	if (check_quest(&event, FALSE)) return;

	current_long_level_name(str);

	/* Verify stairs */
	if (!(f_ptr->flags1 & (FF1_STAIRS)) || !(f_ptr->flags1 & (FF1_LESS)))
	{
		/* Travel if possible */
		if (level_flag & (LF1_SURFACE))
		{
			do_cmd_travel();
			return;
		}

		msg_print("I see no up staircase here.");
		return;
	}

	/* Ironman */
	if ((adult_ironman) && !(adult_campaign))
	{
		msg_print("Nothing happens!");
		return;
	}

	/* Save the old dungeon in case something goes wrong */
	if (autosave_backup)
		do_cmd_save_bkp();

	/* Hack -- take a turn */
	p_ptr->energy_use = 100;

	/* Hack -- travel through wilderness */
	if (adult_campaign
		&& p_ptr->depth == max_depth(p_ptr->dungeon)
		&& level_flag & (LF1_TOWER))
	{
		int guard;
		dungeon_zone *zone;

		/* Get the zone */
		get_zone(&zone, p_ptr->dungeon, p_ptr->depth);

		guard = actual_guardian(zone->guard, p_ptr->dungeon, zone - t_info[p_ptr->dungeon].zone);

		/* Check that travel opening monster is dead and of this zone */
		if ((t_ptr->quest_opens) && (t_ptr->quest_monster == guard) && (r_info[guard].max_num == 0))
		{
			/* Success */
			message(MSG_STAIRS_DOWN,0,format("You have valiantly defeated the sinister guardian at %s. The way forth lies open before you.", str));

			/* Change the dungeon */
			p_ptr->dungeon = actual_route(t_ptr->quest_opens);
		}
		else
		{
			/* Success */
			message(MSG_STAIRS_DOWN,0,format("You have reached the top of %s and you climb down outside.", str));
		}

		/* Hack -- take a turn */
		p_ptr->energy_use = 100;

		/* Clear stairs */
		p_ptr->create_stair = 0;

		/* Set the new depth */
		p_ptr->depth = min_depth(p_ptr->dungeon);
	}
	else
	{
		/* Success */
		message(MSG_STAIRS_UP, 0, "You enter a maze of up staircases.");

		/* Hack -- take a turn */
		p_ptr->energy_use = 100;

		/* Create a way back */
		p_ptr->create_stair = feat_state(cave_feat[py][px], FS_LESS);

		/* Hack -- tower level increases depth */
		if (level_flag & (LF1_TOWER))
		{
			/* New depth */
			p_ptr->depth++;
		}
		else
		{
			/* New depth */
			p_ptr->depth--;
		}
	}

	/* Leaving */
	p_ptr->leaving = TRUE;

	/* Check for quest failure. */
	while (check_quest(&event, TRUE));
}


/*
 * Go down one level
 */
void do_cmd_go_down(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	feature_type *f_ptr= &f_info[cave_feat[py][px]];

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	char str[46];

	quest_event event;

	/* Use this to allow quests to succeed or fail */
	WIPE(&event, quest_event);

	/* Set up departure event */
	event.flags = EVENT_LEAVE;
	event.dungeon = p_ptr->dungeon;
	event.level = p_ptr->depth - min_depth(p_ptr->dungeon);

	/* Check for quest failure. Abort if requested. */
	if (check_quest(&event, FALSE)) return;

	current_long_level_name(str);

	/* Verify stairs */
	if (!(f_ptr->flags1 & (FF1_STAIRS)) || !(f_ptr->flags1 & (FF1_MORE)))
	{
		msg_print("I see no down staircase here.");
		return;
	}

	/* Save the old dungeon in case something goes wrong */
	if (autosave_backup)
		do_cmd_save_bkp();

	/* Hack -- take a turn */
	p_ptr->energy_use = 100;

	/* Hack -- travel through wilderness */
	if (adult_campaign
		&& p_ptr->depth == max_depth(p_ptr->dungeon)
		&& !(level_flag & (LF1_TOWER)))
	{
	  int guard;
	  dungeon_zone *zone;

	  /* Get the zone */
	  get_zone(&zone, p_ptr->dungeon, p_ptr->depth);

	  guard = actual_guardian(zone->guard, p_ptr->dungeon, zone - t_info[p_ptr->dungeon].zone);

		/* Check that quest opens monster is dead and of this zone */
		if ((t_ptr->quest_opens) && (t_ptr->quest_monster == guard) && (r_info[guard].max_num == 0))
		{
			/* Success */
			message(MSG_STAIRS_DOWN,0,format("You have valiantly defeated the sinister guardian at %s. The way forth lies open before you.", str));

			/* Change the dungeon */
			p_ptr->dungeon = actual_route(t_ptr->quest_opens);
		}
		else
		{
			/* Success */
			message(MSG_STAIRS_DOWN,0,format("You have reached the bottom of %s and uncovered a secret shaft back up to the surface.", str));
		}

		/* Hack -- take a turn */
		p_ptr->energy_use = 100;

		/* Clear stairs */
		p_ptr->create_stair = 0;

		/* Set the new depth */
		p_ptr->depth = min_depth(p_ptr->dungeon);
	}
	else
	{
		/* Success */
		message(MSG_STAIRS_DOWN, 0, "You enter a maze of down staircases.");

		/* Hack -- take a turn */
		p_ptr->energy_use = 100;

		/* Create a way back */
		p_ptr->create_stair = feat_state(cave_feat[py][px], FS_MORE);

		/* Hack -- tower level decreases depth */
		if (level_flag & (LF1_TOWER))
		{
			/* New depth */
			p_ptr->depth--;
		}
		else
		{
			/* New depth */
			p_ptr->depth++;
		}
	}

	/* Leaving */
	p_ptr->leaving = TRUE;

	/* Check for quest failure. */
	while (check_quest(&event, TRUE));
}



/*
 * Simple command to "search" for one turn
 * Now also allows you to steal from monsters
 */
void do_cmd_search_or_steal(void)
{
	/* Get the feature */
	feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

	int d, y, x;
	
	u16b valid_dir = 0;
	
	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Check around the character */
	for (d = 0; d < 8; d++)
	{
		/* Extract adjacent (legal) location */
		y = p_ptr->py + ddy_ddd[d];
		x = p_ptr->px + ddx_ddd[d];

		/* Must have monster */
		if (cave_m_idx[y][x] <= 0) continue;

		/* Must be able to see monster */
		if (!m_list[cave_m_idx[y][x]].ml) continue;
		
		/* Skip allied monsters */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY)) continue;

		/* Valid direction for a monster */
		valid_dir |= (1 << ddd[d]);
	}
	
	/* We have a monster to steal from */
	if (valid_dir)
	{	
		/* Get a direction (or abort) */
		if ((get_rep_dir(&d)) && ((valid_dir & (1 << d)) != 0))
		{
			/* Extract adjacent (legal) location */
			y = p_ptr->py + ddy[d];
			x = p_ptr->px + ddx[d];

			/* Set player target */
			p_ptr->target_who = cave_m_idx[y][x];
			
			/* Steal stuff */
			do_cmd_item(COMMAND_ITEM_STEAL);
		}
	}

	/* We want to search instead. Allow aborted steals to search instead. */
	if ((!valid_dir) || ((p_ptr->energy_use == 0) && (get_check("Search instead? "))))
	{
		/* Catch breath */
		if (!(f_ptr->flags2 & (FF2_FILLED)))
		{
			/* Rest the player */
			set_rest(p_ptr->rest + PY_REST_RATE - p_ptr->tiring);
		}
	
		/* Search */
		search();
		
		/* Take a turn */
		p_ptr->energy_use = 100;
	}
}


/*
 * Hack -- toggle search mode
 */
void do_cmd_toggle_search(void)
{

	/* Hack - Check if we are holding a song */
	if (p_ptr->held_song)
	{
		/* Finish song */
		p_ptr->held_song = 0;

		/* Tell the player */
		msg_print("You finish your song.");
	}

	/* TODO: Stop searching, start sneaking */
	else if (p_ptr->searching)
	{
		/* Clear the searching flag */
		p_ptr->searching = FALSE;

		/* Set the sneaking flag */
		p_ptr->sneaking = TRUE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Stop searching */
	else if (p_ptr->sneaking)
	{
		/* Clear the searching flag */
		p_ptr->searching = FALSE;

		/* Clear the sneaking flag */
		p_ptr->sneaking = FALSE;

		/* Clear the last disturb */
		p_ptr->last_disturb = turn;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Start searching */
	else
	{
		/* Set the searching flag */
		p_ptr->searching = TRUE;

		/* Update stuff */
		p_ptr->update |= (PU_BONUS);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_STATE | PR_SPEED);
	}
}



#if defined(ALLOW_EASY_OPEN)

/*
 * Return the number of features around (or under) the character.
 * Usually look for doors and floor traps.
 * ANDY - Counts features that allow action.
 */
static int count_feats(int *y, int *x, int action)
{
	int d, count;

	int feat;

	feature_type *f_ptr;

	u32b flag, bitzero = 0x000000001L;


	/* Count how many matches */
	count = 0;

	/* Check around the character */
	for (d = 0; d < 8; d++)
	{
		/* Extract adjacent (legal) location */
		int yy = p_ptr->py + ddy_ddd[d];
		int xx = p_ptr->px + ddx_ddd[d];

		/* Must have knowledge */
		if (!(play_info[yy][xx] & (PLAY_MARK))) continue;

		/* Get the feature */
		feat = cave_feat[yy][xx];

		/* Get the mimiced feature */
		feat = f_info[feat].mimic;

		f_ptr = &f_info[feat];

		if (action < FS_FLAGS2)
		{
			flag = bitzero << (action - FS_FLAGS1);
			if (!(f_ptr->flags1 & flag)) continue;
		}

		else if (action < FS_FLAGS_END)
		{
			flag = bitzero << (action - FS_FLAGS2);
			if (!(f_ptr->flags2 & flag)) continue;
		}

		/* Count it */
		++count;

		/* Remember the location of the last door found */
		*y = yy;
		*x = xx;
	}

	/* All done */
	return count;
}

/*
 * Extract a "direction" which will move one step from the player location
 * towards the given "target" location (or "5" if no motion necessary).
 */
static int coords_to_dir(int y, int x)
{
	return (motion_dir(p_ptr->py, p_ptr->px, y, x));
}

#endif /* ALLOW_EASY_OPEN */


/*
 * Perform the basic "open" command on doors
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_open_aux(int y, int x)
{
	int i, j;

	bool more = FALSE;

	/* Verify legality */
	if (!do_cmd_test(y, x, FS_OPEN)) return (FALSE);

	/* Unknown trapped door */
	if (f_info[cave_feat[y][x]].flags3 & (FF3_PICK_DOOR))
	{
		pick_door(y,x);
	}

	/* Get hit by regions */
	if (cave_region_piece[y][x])
	{
		/* Trigger the region */
		trigger_region(y, x, TRUE);
	}

	/* Trapped door */
	if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))
	{
		hit_trap(y,x);
	}

	/* Permanent doors */
	else if (f_info[cave_feat[y][x]].flags1 & (FF1_PERMANENT))
	{
		/* Stuck */
		find_secret(y,x);

		return (FALSE);
	}

	/* Locked door */
	else if ((f_info[cave_feat[y][x]].flags1 & (FF1_OPEN)) && (f_info[cave_feat[y][x]].power >0))
	{
		/* Find secrets */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
		{
			find_secret(y,x);

			/* Sanity check */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_OPEN))) return (FALSE);
		}

		/* Disarm factor */
		i = p_ptr->skills[SKILL_DISARM];

		/* Penalize some conditions */
		if (p_ptr->timed[TMD_BLIND] || no_lite()) i = i / 10;
		if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE]) i = i / 10;

		/* Extract the lock power */
		j = f_info[cave_feat[y][x]].power;

		/* Extract the difficulty XXX XXX XXX */
		j = i - (j * 4);

		/* Always have a small chance of success */
		if (j < 2) j = 2;

		/* Success */
		if (rand_int(100) < j)
		{
			/* Message */
			message(MSG_OPENDOOR, 0, "You have picked the lock.");

			/* Open the door */
			cave_alter_feat(y, x, FS_OPEN);

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

			/* Experience */
			gain_exp(1);
		}

		/* Failure */
		else
		{
			/* Failure */
			if (flush_failure) flush();

			/* Message */
			message(MSG_LOCKPICK_FAIL, 0, "You failed to pick the lock.");

			/* We may keep trying */
			more = TRUE;
		}
	}

	/* Closed door */
	else
	{
		/* Find secrets */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
		{
			find_secret(y,x);

			/* Sanity check */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_OPEN))) return (FALSE);
		}

		/* Open the door */
		cave_alter_feat(y, x, FS_OPEN);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

		/* Sound */
		sound(MSG_OPENDOOR);
	}

	/* Result */
	return (more);
}



/*
 * Open a closed/locked/jammed door.
 *
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	bool more = FALSE;

#ifdef ALLOW_EASY_OPEN

	/* Easy Open */
	if (easy_open)
	{
		/* Handle a single closed door  */
		if (count_feats(&y, &x, FS_OPEN)  == 1)
		{
			p_ptr->command_dir = coords_to_dir(y, x);
		}
	}

#endif /* ALLOW_EASY_OPEN */

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];


	/* Verify legality */
	if (!do_cmd_test(y, x,FS_OPEN)) return;


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}


	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Door */
	else
	{
		/* Open the door */
		more = do_cmd_open_aux(y, x);
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(0, 0);
}


/*
 * Perform the basic "close" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_close_aux(int y, int x)
{
	bool more = FALSE;


	/* Verify legality */
	if (!do_cmd_test(y, x,FS_CLOSE)) return (FALSE);

	/* Get hit by regions */
	if (cave_region_piece[y][x])
	{
		/* Trigger the region */
		trigger_region(y, x, TRUE);
	}

	/* Trapped door */
	if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))
	{
		hit_trap(y,x);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	}

	/* Permanent doors */
	else if	(f_info[cave_feat[y][x]].flags1 & (FF1_PERMANENT))
	{
		/* Stuck */
		find_secret(y,x);

		return (FALSE);
	}

	/* Close door */
	else
	{
		/* Find secrets */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
		{
			find_secret(y,x);

			/* Sanity check */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_CLOSE))) return (FALSE);
		}

		/* Close the door */
		cave_alter_feat(y, x, FS_CLOSE);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

		/* Sound */
		sound(MSG_SHUTDOOR);
	}

	/* Result */
	return (more);
}


/*
 * Close an open door.
 */
void do_cmd_close(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	bool more = FALSE;

#ifdef ALLOW_EASY_OPEN

	/* Easy Close */
	if (easy_open)
	{
		/* Handle a single open door */
		if (count_feats(&y, &x, FS_CLOSE) == 1)
		{
			/* Don't close door player is on */
			if ((y != py) || (x != px))
			{
				p_ptr->command_dir = coords_to_dir(y, x);
			}
		}
	}

#endif /* ALLOW_EASY_OPEN */

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];

	/* Verify legality */
	if (!do_cmd_test(y, x, FS_CLOSE)) return;

	/* Take a turn */
	p_ptr->energy_use = 50;

	/* Apply stuck / confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Door */
	else
	{
		/* Close door */
		more = do_cmd_close_aux(y, x);
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(0, 0);
}






/*
 * Perform the basic "tunnel" command
 *
 * Assumes that no monster is blocking the destination
 *
 * Do not use twall anymore --- ANDY
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_tunnel_aux(int y, int x)
{
	bool more = FALSE;

	int i,j;

	cptr name;

	int feat;

	feat = cave_feat[y][x];

	/* Verify legality */
	if (!do_cmd_test(y, x, FS_TUNNEL)) return (FALSE);

	i = p_ptr->skills[SKILL_DIGGING];

	j = f_info[cave_feat[y][x]].power;

	/* Hack - bump up power for doors */
	if (f_info[cave_feat[y][x]].flags1 & (FF1_DOOR)) j = 30;

	/* Get hit by regions */
	if (cave_region_piece[y][x])
	{
		/* Trigger the region */
		trigger_region(y, x, TRUE);
	}

	/* Trapped door */
	if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))
	{
		hit_trap(y,x);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Permanent rock */
	else if (f_info[cave_feat[y][x]].flags1 & (FF1_PERMANENT))
	{
		/* Stuck */
		find_secret(y,x);

		return (FALSE);
	}

	/* Dig or tunnel */
	else if (f_info[cave_feat[y][x]].flags2 & (FF2_CAN_DIG))
	{
		/* Dig */
		if (p_ptr->skills[SKILL_DIGGING] > rand_int(20 * j))
		{
			sound(MSG_DIG);

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Get the name */
			name = (f_name + f_info[feat].name);

			/* Give the message */
			msg_format("You have removed the %s.",name);

			cave_alter_feat(y,x,FS_TUNNEL);

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
		}

		/* Keep trying */
		else
		{
			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Get the name */
			name = (f_name + f_info[feat].name);

			/* We may continue tunelling */
			msg_format("You dig into the %s.",name);
			more = TRUE;
		}
	}

	else
	{
		/* Tunnel -- much harder */
		if (p_ptr->skills[SKILL_DIGGING] > (j + rand_int(40 * j)))
		{
			sound(MSG_DIG);

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Get the name */
			name = (f_name + f_info[feat].name);

			/* Give the message */
			msg_print("You have finished the tunnel.");

			/* FIXME: how to mark the grid is altered by the player so that he cannot scum for XP by disarming pits? Perhaps add some rounded pebbles to that location? */
			cave_alter_feat(y,x,FS_TUNNEL);

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
		}

		/* Keep trying */
		else
		{
			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Get the name */
			name = (f_name + f_info[feat].name);

			/* We may continue tunelling */
			msg_format("You tunnel into the %s.",name);
			more = TRUE;
		}
	}

	/* Result */
	return (more);
}

/*
 * Tunnel through "walls" (including rubble and secret doors)
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 */
void do_cmd_tunnel(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	bool more = FALSE;

#ifdef ALLOW_EASY_OPEN

	/* Easy Tunnel */
	if (easy_open)
	{
		/* Handle a single open door */
		if (count_feats(&y, &x, FS_TUNNEL) == 1)
		{
			/* Don't close door player is on */
			if ((y != py) || (x != px))
			{
				p_ptr->command_dir = coords_to_dir(y, x);
			}
		}
	}

#endif /* ALLOW_EASY_OPEN */

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];


	/* Oops */
	if (!do_cmd_test(y, x, FS_TUNNEL)) return;

	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Walls */
	else
	{
		/* Tunnel through walls */
		more = do_cmd_tunnel_aux(y, x);
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(0, 0);
}

/*
 * Perform the basic "disarm" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_disarm_aux(int y, int x, bool disarm)
{
	int i, j, power;

	cptr name, act;

	bool more = FALSE;

	/* Get feature */
	feature_type *f_ptr = &f_info[cave_feat[y][x]];
	
	/* Arm or disarm */
	if (disarm) act = "disarm";
	else act = "arm";

	/* Verify legality */
	if (!do_cmd_test(y, x, (disarm ? FS_DISARM : FS_TRAP))) return (FALSE);

	/* Get the trap name */
	name = (f_name + f_ptr->name);

	/* Get the "disarm" factor */
	i = p_ptr->skills[SKILL_DISARM];

	/* Penalize some conditions */
	if (p_ptr->timed[TMD_BLIND] || no_lite()) i = i / 10;
	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE]) i = i / 10;

	/* XXX XXX XXX Variable power? */

	/* Extract trap "power" */
	power = f_ptr->power;

	/* Player trap */
	if (cave_o_idx[y][x])
	{
		/* Use object level instead */
		power = k_info[o_list[cave_o_idx[y][x]].k_idx].level;
	}

	/* Extract the difficulty */
	j = i - power;

	/* Always have a small chance of success */
	if (j < 2) j = 2;

	/* Success */
	if (rand_int(100) < j)
	{
		/* Message */
		msg_format("You have %sed the %s.", act, name);

		/* Reward, unless player trap */
		/* Note that player traps have an object in the trap */
		if (disarm && !cave_o_idx[y][x] &&
			/* Hack to avoid awarding xp for disarming siege engines/clockwork mechanisms */
			(f_ptr->d_attr != TERM_MAGENTA) && (f_ptr->d_attr != TERM_L_PINK))
		  gain_exp(power);

		/* Remove the trap */
		if (disarm)
		  {
		    /* A hack to e.g. get Flasks of Oil, etc. from traps */
		    object_type *i_ptr;
		    object_type object_type_body;

		    i_ptr = &object_type_body;

			/* Don't drop an object if a player trap */
		    if (!cave_o_idx[y][x]) make_feat(i_ptr, y, x);

		    /* Remove the trap */
		    cave_alter_feat(y, x, FS_DISARM);

		    /* Update the visuals */
		    p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
		  }
	}

	/* Failure -- Keep trying */
	else if ((i > 5) && (randint(i) > 5))
	{
		/* Failure */
		if (flush_failure) flush();

		/* Message */
		msg_format("You failed to %s the %s.", act, name);

		/* Disarming? */
		if (!disarm)
		{
			/* Disarm the grid */
			cave_alter_feat(y, x, FS_DISARM);
		}

		/* We may keep trying */
		more = TRUE;
	}

	/* Failure -- Set off the trap */
	else
	{
		/* Message */
		msg_format("You set off the %s!", name);

		/* Get hit by regions */
		if (cave_region_piece[y][x])
		{
			/* Trigger the region */
			trigger_region(y, x, TRUE);
		}

		/* Hit the trap */
		hit_trap(y, x);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	}

	/* Result */
	return (more);
}


/*
 * Disarms a trap
 */
void do_cmd_disarm(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	bool more = FALSE;

#ifdef ALLOW_EASY_OPEN

	/* Easy Disarm */
	if (easy_open)
	{
		/* Handle a single visible trap or trapped chest */
		if (count_feats(&y, &x, FS_DISARM) == 1)
		{
			p_ptr->command_dir = coords_to_dir(y, x);
		}
	}

#endif /* ALLOW_EASY_OPEN */

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];


	/* Verify legality */
	if (!do_cmd_test(y, x, FS_DISARM)) return;


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply stuck / confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}
	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Disarm trap */
	else
	{
		/* Disarm the trap */
		more = do_cmd_disarm_aux(y, x, TRUE);
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(0, 0);
}



/*
 * Perform the basic "bash" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_bash_aux(int y, int x, bool charging)
{
	int bash, temp;

	int feat;

	cptr name;

	bool more = FALSE;

	/* Verify legality */
	if (!do_cmd_test(y, x,FS_BASH)) return (FALSE);

	/* Get mimiced feature */
	feat = f_info[cave_feat[y][x]].mimic;

	/* Get the name */
	name = (f_name + f_info[feat].name);

	/* Message */
	msg_format("You smash into the %s!",name);

	/* Get hit by regions */
	if (cave_region_piece[y][x])
	{
		/* Trigger the region */
		trigger_region(y, x, TRUE);
	}

	/* Trapped door */
	if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))
	{
		hit_trap(y,x);

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Permanent doors */
	else if (f_info[cave_feat[y][x]].flags1 & (FF1_PERMANENT))
	{
		/* Stuck */
		find_secret(y,x);

		return (FALSE);
	}

	/* Hack -- Bash power based on strength */
	/* (Ranges from 3 to 20 to 100 to 200) */
	bash = adj_str_blow[p_ptr->stat_ind[A_STR]];

	/* Bonus for charging */
	if (charging) bash *= 2;

	/* Extract door power */
	temp = f_info[cave_feat[y][x]].power;

	/* Compare bash power to door power XXX XXX XXX */
	temp = (bash - (temp * 10));

	/* Hack -- always have a chance */
	if (temp < 1) temp = 1;

	/* Hack -- attempt to bash down the door */
	if (rand_int(100) < temp)
	{
		/* Message */
		msg_format("The %s crashes open!",name);

		/* Find secrets */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
		{
			find_secret(y,x);

			/* Sanity check */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_BASH))) return (FALSE);
		}

		/* Break down the door */
		if (rand_int(100) < 50)
		{
			cave_alter_feat(y, x, FS_BASH);
		}

		/* Open the door */
		else
		{
			cave_alter_feat(y, x, FS_OPEN);
		}

		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

		/* Sound */
		sound(MSG_OPENDOOR);

	}

	/* Saving throw against stun */
	else if (rand_int(100) < adj_agi_safe[p_ptr->stat_ind[A_AGI]] +
		 p_ptr->lev)
	{
		/* Message */
		msg_format("The %s holds firm.",name);

		/* Allow repeated bashing */
		more = TRUE;
	}

	/* High agility yields coolness */
	else
	{
		/* Message */
		msg_print("You are off-balance.");

		/* Hack -- Lose balance ala paralysis */
		inc_timed(TMD_PARALYZED, 2 + rand_int(2), TRUE);
	}

	/* Result */
	return (more);
}


/*
 * Bash open a door, success based on character strength
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 */
void do_cmd_bash(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	bool charging = FALSE;

#ifdef ALLOW_EASY_OPEN

	/* Easy Bash */
	if (easy_open)
	{
		/* Handle a single visible trap */
		if (count_feats(&y, &x, FS_BASH)==1)
		{
			p_ptr->command_dir = coords_to_dir(y, x);
		}
	}

#endif /* ALLOW_EASY_OPEN */

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];


	/* Verify legality */
	if (!do_cmd_test(y, x, FS_BASH)) return;


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}

	/* If moving, you can charge in the direction you move */
	if ((p_ptr->charging == dir) || (side_dirs[dir][1] == p_ptr->charging)
		|| (side_dirs[dir][2] == p_ptr->charging)) charging = TRUE;

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Door */
	else
	{
		/* Bash the door */
		if (!do_cmd_bash_aux(y, x, charging))
		{
			/* Cancel repeat */
			disturb(0, 0);
		}
	}
}



/*
 * Manipulate an adjacent grid in some way
 *
 * Attack monsters, tunnel through walls, disarm traps, open doors.
 *
 * This command must always take energy, to prevent free detection
 * of invisible monsters.
 *
 * The "semantics" of this command must be chosen before the player
 * is confused, and it must be verified against the new grid.
 */
void do_cmd_alter(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	int feat;

	bool more = FALSE;


	/* Get a direction */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- Apply stuck */
	stuck_player(&dir);

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];


	/* Original feature */
	feat = cave_feat[y][x];

	/* Get mimiced feature */
	feat = f_info[feat].mimic;

	/* Must have knowledge to know feature XXX XXX */
	if (!(play_info[y][x] & (PLAY_MARK))) feat = FEAT_NONE;


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Attack monsters */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Disarm traps */
	else if (f_info[feat].flags1 & (FF1_DISARM))
	{
		/* Tunnel */
		more = do_cmd_disarm_aux(y, x, TRUE);
	}

	/* Open closed doors */
	else if (f_info[feat].flags1 & (FF1_OPEN))
	{
		/* Tunnel */
		more = do_cmd_open_aux(y, x);
	}
#if 0
	/* Bash jammed doors */
	else if (f_info[feat].flags1 & (FF1_BASH))
	{
		/* Tunnel */
		more = do_cmd_bash_aux(y, x);
	}
#endif
	/* Tunnel through walls */
	else if (f_info[feat].flags1 & (FF1_TUNNEL))
	{
		/* Tunnel */
		more = do_cmd_tunnel_aux(y, x);
	}
#if 0

	/* Close open doors */
	else if (f_info[feat].flags1 & (FF1_CLOSE))
	{
		/* Close */
		more = do_cmd_close_aux(y, x);
	}

#endif

	/* Oops */
	else
	{
		/* Oops */
		msg_print("You spin around.");
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(0, 0);
}


/*
 * Hack -- Set a trap or jam a door closed with a spike.
 *
 * This command may not be repeated.
 *
 * See pick_trap for how traps are chosen, and hit_trap and mon_hit_trap for what
 * player set traps will do.
 */
bool player_set_trap_or_spike(int item)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, action;
	int dir = 0;

	object_type *o_ptr;

	cptr q,s;

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

	/* Spiking or setting trap? */
	if (o_ptr->tval == TV_SPIKE)
	{
		/* We are spiking */
		action = FS_SPIKE;

#ifdef ALLOW_EASY_OPEN

		/* Easy Bash */
		if (easy_open)
		{
			/* Handle a single visible trap */
			if (count_feats(&y, &x, action)==1)
			{
				p_ptr->command_dir = coords_to_dir(y, x);
			}
		}

#endif /* ALLOW_EASY_OPEN */

		/* Get a direction (or abort) */
		if (!get_rep_dir(&dir)) return (FALSE);

		/* Hack -- Apply stuck */
		stuck_player(&dir);
	}
	else
	{
		/* Hack -- only set traps on floors (at this stage) XXX */
		/* We hackily allow ground as well */
		action = FS_FLOOR;
	}

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];

	/* Verify legality */
	if (!do_cmd_test(y, x, action)) return (FALSE);

	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Apply stuck / confusion */
	if (dir && confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];
	}

	/* Monster */
	if (cave_m_idx[y][x] > 0)
	{
		/* Message */
		msg_print("There is a monster in the way!");

		/* Push allies out of the way */
		if (m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY))
		{
			/* Push */
			push_aside(py, px, &m_list[cave_m_idx[y][x]]);
		}
		else
		{
			/* Attack */
			py_attack(dir);
		}
	}

	/* Go for it */
	else
	{
		/* Verify legality */
		if (!do_cmd_test(y, x, action)) return (TRUE);

#if 0
		/* Trapped door */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))
		{
			hit_trap(y,x);
		}
#endif

		/* Permanent doors */
		/* else */ if (f_info[cave_feat[y][x]].flags1 & (FF1_PERMANENT))
		{
			/* Stuck */
			find_secret(y,x);

			return (TRUE);
		}

		/* Spike the door */
		else if (action == FS_SPIKE)
		{
			object_type *k_ptr = NULL;

			int feat = cave_feat[y][x];
			int item2 = 0;

			/* Find secrets */
			if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
			{
				find_secret(y,x);

				/* Sanity check */
				if (!(f_info[cave_feat[y][x]].flags1 & (FF1_SPIKE))) return (TRUE);
			}

			if (item >= 0)
			{
				/* Mark item for reduction */
				o_ptr->ident |= (IDENT_BREAKS);
			}

			feat = feat_state(feat, FS_SPIKE);

			if (strstr(f_name + f_info[feat].name, "rope"))
			{
				/* Restrict an item */
				item_tester_tval = TV_ROPE;

				/* Get an item */
				q = "Attach which rope? ";
				s = "You have no rope to attach.";
				if (get_item(&item2, q, s, (USE_INVEN | USE_FLOOR)))
				{
					/* Get the object */
					if (item2 >= 0)
					{
						k_ptr = &inventory[item2];
					}
					else
					{
						k_ptr = &o_list[0 - item2];
					}

					/* In a bag? */
					if (k_ptr->tval == TV_BAG)
					{
						/* Get item from bag */
						if (get_item_from_bag(&item2, q, s, k_ptr))
						{
							/* Refer to the item */
							k_ptr = &inventory[item2];
						}
					}
				}
				else
				{
					return (TRUE);
				}
			}

			cave_alter_feat(y,x,FS_SPIKE);

			/* MegaHack -- handle chain */
			if ((k_ptr) && (k_ptr->sval == SV_ROPE_CHAIN)) cave_set_feat(y, x, cave_feat[y][x] + 1);

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

			/* Parania */
			if (item >= 0)
			{
				item = find_item_to_reduce(item);

				/* Item has already been destroyed */
				if (item < 0) return(TRUE);

				/* Get the object */
				o_ptr = &inventory[item];

				/* Clear marker */
				o_ptr->ident &= ~(IDENT_BREAKS);
			}
			/* More paranoia */
			else
			{
				if (!o_ptr->k_idx) return(TRUE);
			}
			
			/* Destroy a spike in the pack */
			if (item >= 0)
			{
				if ((o_ptr->number == 1) && (item2 > item)) item2--;

				inven_item_increase(item, -1);
				inven_item_describe(item);
				inven_item_optimize(item);
			}

			/* Destroy a spike on the floor */
			else
			{
				floor_item_increase(0 - item, -1);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}

			if (k_ptr)
			{
				/* Reduce inventory -- suppress messages */
				if (item2 >= 0)
				{
					if (k_ptr->number == 1)
					{
						inven_drop_flags(k_ptr);
						k_ptr = NULL;
					}

					inven_item_increase(item2, -1);
					inven_item_optimize(item2);
				}

				/* Reduce and describe floor item */
				else
				{
					if (k_ptr->number == 1) k_ptr = NULL;

					floor_item_increase(0 - item2, -1);
					floor_item_optimize(0 - item2);
				}
			}
		}

		/* Set the trap */
		/* We only let the player set traps when there are no existing
               objects in the grid OR the existing objects in the grid have
               the same tval and sval as the trap being set OR the trap
               being set is TV_BOW and all the objects in the grid can be fired
               by the bow in question OR the object selected is a digger,
               we substitute the above rules with spikes OR the object is
               a fully assembled mechanism */
		else
		{
			int this_o_idx, next_o_idx;

			bool trap_allowed = TRUE;
			bool hack_assembly = FALSE;

			/* Hack */
			int tmp = p_ptr->skills[SKILL_DISARM];

			object_type object_type_body;

			object_type *j_ptr;

			/* Find secrets */
			if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
			{
				find_secret(y,x);

				/* Sanity check */
				if (!(f_info[cave_feat[y][x]].flags1 & (FF1_FLOOR))) return (TRUE);
			}

			/* Get object body */
			j_ptr = &object_type_body;

			/* Structure Copy */
			object_copy(j_ptr, o_ptr);

			/* Set one object only */
			j_ptr->number = 1;

			/* Scan all objects in the grid */
			for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *i_ptr;

				/* Get the object */
				i_ptr = &o_list[this_o_idx];

				/* Get the next object */
				next_o_idx = i_ptr->next_o_idx;

				/* Check if fired */
				if (j_ptr->tval == TV_BOW)
				{
					switch(j_ptr->sval / 10)
					{
						case 1:
						{
							if (i_ptr->tval != TV_ARROW) trap_allowed = FALSE;
							break;
						}
					    case 2:
						{
							if (i_ptr->tval != TV_BOLT) trap_allowed = FALSE;
							break;
						}
					    case 3:
					    {
							if (i_ptr->tval != TV_SHOT) trap_allowed = FALSE;
					    }
						default:
						{
							if (!is_known_throwing_item(i_ptr)) trap_allowed = FALSE;
							break;
						}
					}
				}
				/* Check if spike -- use digger & spikes to build pits */
				else if (j_ptr->tval == TV_DIGGING)
				{
					if (i_ptr->tval != TV_SPIKE) trap_allowed = FALSE;
				}
				
				/* Mega Hack -- if the object is a fully assembled mechanism, we get the
				 * tval and sval of the first item instead */
				else if ((j_ptr->tval == TV_ASSEMBLY) && (j_ptr->sval == SV_ASSEMBLY_FULL) && (!j_ptr->name3))
				{
					/* Get these values temporarily and adjust back using a hack later */
					j_ptr->tval = i_ptr->tval;
					j_ptr->sval = i_ptr->sval;
					hack_assembly = TRUE;
				}

				else if ((j_ptr->tval != i_ptr->tval) || (j_ptr->sval != i_ptr->sval))
				{
					/* Not allowed */
					trap_allowed = FALSE;
				}
			}
			
			/* Hack - fix up assembly hack */
			if (hack_assembly)
			{
				j_ptr->tval = TV_ASSEMBLY;
				j_ptr->sval = SV_ASSEMBLY_FULL;
			}

			/* Hack -- Dig trap? */
			if (j_ptr->tval == TV_DIGGING)
			{
				/* Hack -- use tunnelling skill to build trap */
				p_ptr->skills[SKILL_DISARM] = p_ptr->skills[SKILL_DIGGING];

				/* Restrict an item */
				item_tester_tval = TV_SPIKE;

				/* Get an item */
				q = "Add which spikes? ";
				s = "You have no spikes to add.";
				if (get_item(&item, q, s, (USE_INVEN | USE_FLOOR)))
				{
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
					if (j_ptr->tval == TV_BAG)
					{
						/* Get item from bag */
						if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

						/* Refer to the item */
						o_ptr = &inventory[item];
					}

					/* Structure Copy */
					object_copy(j_ptr, o_ptr);

					/* Set one object only */
					j_ptr->number = 1;

				}
				else if (!cave_o_idx[y][x])
				{
					/* Hack -- no spikes. Dig a shallow pit instead. */
					cave_set_feat(y, x, FEAT_SHALLOW_PIT);

					return (TRUE);
				}
				else
				{
					/* Hack -- use existing floor spikes */
					item = -cave_o_idx[y][x];

					/* Structure Copy */
					object_copy(j_ptr, &o_list[cave_o_idx[y][x]]);

					/* Set one object only */
					j_ptr->number = 1;
				}
			}

			/* Trap allowed? */
			if ((trap_allowed) && (floor_carry(y,x,j_ptr)))
			{
				/* Hack -- ensure trap is created */
				object_level = 128;

				if (item >= 0)
				{
					/* Mark item for reduction */
					o_ptr->ident |= (IDENT_BREAKS);
				}

				/* Set the floor trap */
				cave_set_feat(y,x,FEAT_INVIS);

				/* Set the trap */
				pick_trap(y,x, TRUE);
				
				/* Reset object level */
				object_level = p_ptr->depth;

				/* Parania */
				if (item >= 0)
				{
					item = find_item_to_reduce(item);

					/* Item has already been destroyed */
					if (item < 0) return(TRUE);

					/* Get the object */
					o_ptr = &inventory[item];

					/* Clear marker */
					o_ptr->ident &= ~(IDENT_BREAKS);
				}

				/* More paranoia */
				else
				{
					if (!o_ptr->k_idx) return(TRUE);
				}
				
				/* Destroy an item in the pack */
				if (item >= 0)
				{
					inven_item_increase(item, -1);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Destroy an item on the floor */
				else
				{
					floor_item_increase(0 - item, -1);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
				
				/* Check if we can arm it? */
				do_cmd_disarm_aux(y,x, FALSE);
			}
			/* Message */
			else
			{
				msg_print("You can't set this trap here.");
			}

			/* Reset skill - hacked for pit traps */
			p_ptr->skills[SKILL_DISARM] = tmp;
		}
	}
	
	return (TRUE);
}


static bool do_cmd_walk_test(int y, int x)
{
	int feat;

	cptr name;

	/* Get feature */
	feat = cave_feat[y][x];

	/* Get mimiced feature */
	feat = f_info[feat].mimic;

	/* Get the name */
	name = (f_name + f_info[feat].name);

	/* Hack -- walking obtains knowledge XXX XXX */
	if (!(play_info[y][x] & (PLAY_MARK))) return (TRUE);

	/* Hack -- walking allows attacking XXX XXX */
	if (cave_m_idx[y][x] >0) return (TRUE);

	/* Hack -- walking allows pickup XXX XXX */
	if (cave_o_idx[y][x] >0) return (TRUE);

	/* Hack -- walking allows gathering XXX XXX */
	if (f_info[feat].flags3 & (FF3_GET_FEAT)) return (TRUE);

	/* Player can not walk through "walls" */
	/* Also cannot climb over unknown "trees/rubble" */
	if (!(f_info[feat].flags1 & (FF1_MOVE))
	&& (!(f_info[feat].flags3 & (FF3_EASY_CLIMB))
	|| !(play_info[y][x] & (PLAY_MARK))))
	{
		if (easy_alter)
		{
			if (f_info[feat].flags1 & (FF1_OPEN)) return(TRUE);
		}

		/* Message */
		msg_format("There is %s %s in the way.",
				(is_a_vowel(name[0]) ? "an" : "a"),name);

		/* Nope */
		return (FALSE);

	}

	/* Okay */
	return (TRUE);
}

/*
 * Walk into a grid. For mouse movement when confused.
 */
void do_cmd_walk()
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];

	/* Verify legality */
	if (!do_cmd_walk_test(y, x)) return;

	/* Take time */
	p_ptr->energy_use = 100;

	/* Hack -- handle stuck players */
	if (stuck_player(&dir))
	{
		/* Get the mimiced feature */
		int mimic = f_info[cave_feat[py][px]].mimic;

		/* Get the feature name */
		cptr name = (f_name + f_info[mimic].name);

		/* Tell the player */
		msg_format("You are stuck %s%s.",
			((f_info[mimic].flags2 & (FF2_FILLED)) ? "" :
				(is_a_vowel(name[0]) ? "inside an " : "inside a ")),name);
	}

	/* Apply confusion */
	else if (confuse_dir(&dir))
	{
		/* Get location */
		y = py + ddy[dir];
		x = px + ddx[dir];

	}

	/* Verify legality */
	if (!do_cmd_walk_test(y, x)) return;

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Move the player */
	move_player(dir);
}


/*
 * Start running.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_run(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, dir;

	/* Hack XXX XXX XXX */
	if (p_ptr->timed[TMD_CONFUSED])
	{
		msg_print("You are too confused!");
		return;
	}

	/* Get a direction (or abort) */
	if (!get_rep_dir(&dir)) return;

	/* Hack -- handle stuck players */
	if (stuck_player(&dir))
	{
		int mimic = f_info[cave_feat[py][px]].mimic;

		/* Get the feature name */
		cptr name = (f_name + f_info[mimic].name);

		/* Use up energy */
		p_ptr->energy_use = 100;

		/* Tell the player */
		msg_format("You are stuck %s%s.",
			((f_info[mimic].flags2 & (FF2_FILLED)) ? "" :
				(is_a_vowel(name[0]) ? "inside an " : "inside a ")),name);

		return;
	}

	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];

	/* Verify legality */
	if (!do_cmd_walk_test(y, x)) return;

	/* Start run */
	run_step(dir);
}



/*
 * Start running with pathfinder.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_pathfind(int y, int x)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	/* Must not be 0 or 5 */
	int dummy = 1;

	/* Hack XXX XXX XXX */
	if (p_ptr->timed[TMD_CONFUSED])
	{
		msg_print("You are too confused!");
		return;
	}

	/* Hack -- handle stuck players */
	if (stuck_player(&dummy))
	{
		int mimic = f_info[cave_feat[py][px]].mimic;

		/* Get the feature name */
		cptr name = (f_name + f_info[mimic].name);

		/* Use up energy */
		p_ptr->energy_use = 100;

		/* Tell the player */
		msg_format("You are stuck %s%s.",
			((f_info[mimic].flags2 & (FF2_FILLED)) ? "" :
				(is_a_vowel(name[0]) ? "inside an " : "inside a ")),name);

		return;
	}

	if (findpath(y, x))
	{
		p_ptr->running = 1000;
		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
		p_ptr->running_withpathfind = TRUE;
		run_step(0);
	}
}


/*
 * Stay still.  Search.  Enter stores.
 */
void do_cmd_hold(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	/* Get the feature */
	feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

	/* Allow repeated command */
	if (p_ptr->command_arg)
	{
		/* Set repeat count */
		p_ptr->command_rep = p_ptr->command_arg - 1;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		p_ptr->command_arg = 0;
	}

	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Catch breath */
	if (!(f_ptr->flags2 & (FF2_FILLED)))
	{
		/* Rest the player */
		set_rest(p_ptr->rest + PY_REST_RATE - p_ptr->tiring);
	}

	/* Spontaneous Searching */
	if ((p_ptr->skills[SKILL_SEARCH] >= 25) || (0 == rand_int(25 - p_ptr->skills[SKILL_SEARCH])))
	{
		search();
	}

	/* Continuous Searching */
	if (p_ptr->searching)
	{
		search();
	}

	/* Handle objects now.  XXX XXX XXX */
	p_ptr->energy_use += py_pickup(py, px, 0) * 10;

	/* Hack -- enter a store if we are on one */
	if (f_info[cave_feat[py][px]].flags1 & (FF1_ENTER))
	{
		/* Disturb */
		disturb(0, 0);

		/* Hack -- enter store */
		p_ptr->command_new.key = '_';

		/* Free turn XXX XXX XXX */
		p_ptr->energy_use = 0;
	}

	/* Blocking -- temporary bonus to ac */
	if (!p_ptr->searching)
	{
		/* Hack -- not dodging */
		p_ptr->dodging = 0;

		/* Hack - block for two turns. We make this 3 to allow 'blocking' to display up to the final action. */
		p_ptr->blocking = 3;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}
}


/*
 * Pick up objects on the floor beneath you.  -LM-
 */
void do_cmd_pickup(void)
{
	int energy_cost;

	/* Pick up floor objects, forcing a menu for multiple objects. */
	energy_cost = py_pickup(p_ptr->py, p_ptr->px, 1) * 10;

	/* Maximum time expenditure is a full turn. */
	if (energy_cost > 100) energy_cost = 100;

	/* Charge this amount of energy. */
	p_ptr->energy_use = energy_cost;
}


/*
 * Rest (restores hit points and mana and such)
 */
void do_cmd_rest(void)
{
	/* Get the feature */
	feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

	/* Prompt for time if needed */
	if (p_ptr->command_arg <= 0)
	{
		cptr p = "Rest (0-9999, '*' for HP/SP, '&' as needed, '$' to sleep to recover stats): ";

		char out_val[5];

		/* Default */
		my_strcpy(out_val, "&", sizeof(out_val));

		/* Ask for duration */
		if (!get_string(p, out_val, sizeof(out_val))) return;

		/* Fall asleep */
		if (out_val[0] == '$')
		{
			p_ptr->command_arg = (-3);
		}

		/* Rest until done */
		else if (out_val[0] == '&')
		{
			p_ptr->command_arg = (-2);
		}

		/* Rest a lot */
		else if (out_val[0] == '*')
		{
			p_ptr->command_arg = (-1);
		}

		/* Rest some */
		else
		{
			p_ptr->command_arg = atoi(out_val);
			if (p_ptr->command_arg <= 0) return;
		}
	}


	/* Paranoia */
	if (p_ptr->command_arg > 9999) p_ptr->command_arg = 9999;

	/* Catch breath */
	if (!(f_ptr->flags2 & (FF2_FILLED)))
	{
		/* Rest the player */
		set_rest(p_ptr->rest + PY_REST_RATE * 2 - p_ptr->tiring);
	}

	/* Take a turn XXX XXX XXX (?) */
	p_ptr->energy_use = 100;

	/* Fall asleep */
	if (p_ptr->command_arg == -3)
	{
		p_ptr->command_arg = PY_SLEEP_ASLEEP;
		set_timed(TMD_PSLEEP, 1, FALSE);
	}

	/* Save the rest code */
	p_ptr->resting = p_ptr->command_arg;

	/* Cancel the arg */
	p_ptr->command_arg = 0;

	/* Cancel searching */
	p_ptr->searching = FALSE;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff();

	/* Refresh XXX XXX XXX */
	Term_fresh();
}






/*
 * Determines the odds of an object breaking when thrown at a monster
 *
 * Note that artifacts never break, see the "drop_near()" function.
 */
int breakage_chance(object_type *o_ptr)
{
	/* Examine the item type */
	switch (o_ptr->tval)
	{
		/* Always break */
		case TV_FLASK:
		case TV_SPELL:
		case TV_POTION:
		case TV_HOLD:
		case TV_MUSHROOM:
		case TV_JUNK:
		case TV_SKIN:
		case TV_EGG:
		case TV_LITE:
		{
			return (100);
		}

		/* Often break */
		case TV_SCROLL:
		case TV_BONE:
		case TV_FOOD:
		{
			return (50);
		}

		/* Sometimes break */
		case TV_WAND:
		case TV_BODY:
		{
			return (25);
		}

		/* Sometimes break: 10% of the time with max INT */
		case TV_ARROW:
		{
		  return (41 - adj_int_break[p_ptr->stat_ind[A_INT]]);
		}

		/* Rarely break: shots, bolts
		   break 0% of the time with max INT */
		case TV_SHOT:
		case TV_BOLT:
		{
			return (31 - adj_int_break[p_ptr->stat_ind[A_INT]]);
		}

		/* Don't break thrown weapons (throwing and regular) to
		 * allow these to stack better. These weapons have enough
		 * chance of breaking on the ground after throwing anyway. */
		default:
		{
			return (0);
		}
	}
}


/*
 * Hook to determine if an object is rope
 */
static bool item_tester_hook_rope(const object_type *o_ptr)
{
	if ((o_ptr->tval == TV_ROPE) && (o_ptr->sval != SV_ROPE_CHAIN)) return (TRUE);

	/* Assume not */
	return (FALSE);
}


/*
 * Calculate the projection path for fired or thrown objects.  -LM-
 *
 * From Sangband.
 *
 * Apply variable inaccuracy, using angular comparison.
 *
 * If inaccuracy is non-zero, we reduce corridor problems by reducing
 * range instead of immediately running the projectile into the wall.
 * This favour is not extended to projection sources out in the open,
 * which makes it much better to be aiming from a firing slit than into
 * one.
 */
int fire_or_throw_path(u16b *gp, int range, int y0, int x0, int *ty, int *tx, int inaccuracy)
{
	int reduce, expected_distance;
	int dy, dx, delta, angle;
	int mult = 1;

	/* No inaccuracy or no distance -- calculate path and return */
	if ((inaccuracy <= 0) || ((*ty == y0) && (*tx == x0)))
	{
		/* Calculate the path */
		return(project_path(gp, range, y0, x0, ty, tx, PROJECT_THRU));
	}

	/* Inaccuracy penalizes range */
	reduce = rand_int(1 + inaccuracy / 2);
	if (reduce > range / 3) reduce = range / 3;
	range -= reduce;


	/* Get distance to target */
	dy = *ty - y0;
	dx = *tx - x0;
	delta = MAX(ABS(dy), ABS(dx));


	/* Extend target away from source, if necessary */
	if ((delta > 0) && (delta <= 7))
	{
		if      (delta == 1) mult = 10;
		else if (delta == 2) mult = 5;
		else if (delta == 3) mult = 4;
		else if (delta == 4) mult = 3;
		else                 mult = 2;

		*ty = y0 + (dy * mult);
		*tx = x0 + (dx * mult);
	}

	/* Note whether we expect the projectile to travel anywhere */
	expected_distance = project_path(gp, range, y0, x0, ty, tx, PROJECT_THRU);

	/* Path enters no grids except the origin -- return */
	if (expected_distance < 2) return (expected_distance);


	/* Continue until satisfied */
	while (TRUE)
	{
		int ty2 = *ty;
		int tx2 = *tx;

		/* Get angle to this target */
		angle = get_angle_to_target(y0, x0, ty2, tx2, 0);

		/* Inaccuracy changes the angle of projection */
		angle = Rand_normal(angle, inaccuracy);

		/* Handle the 0-240 angle discontinuity */
		if (angle < 0) angle = 240 + angle;

		/* Get a grid using the adjusted angle, target it */
		get_grid_using_angle(angle, y0, x0, &ty2, &tx2);

		/* Calculate the path, accept if it enters at least two grids */
		if (project_path(gp, range, y0, x0, &ty2, &tx2, PROJECT_THRU) > 1)
		{
			break;
		}

		/* Otherwise, reduce range and expected distance */
		else
		{
			range -= rand_int(div_round(range, 4));
			if (expected_distance > range) expected_distance = range;
		}
	}

	return(range);
}





/*
 * Fire or throw an already chosen object.
 *
 * You may only fire items that "match" your missile launcher
 * or a throwing weapon (see do_cmd_throw_selected).
 *
 * You must use slings + pebbles/shots/thrown objects,
 * bows + arrows, xbows + bolts.
 *
 * See "calc_bonuses()" for more calculations and such.
 *
 * Note: "unseen" monsters are very hard to hit.
 *
 * Objects are more likely to break if they "attempt" to hit a monster.
 *
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 *
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 *
 * Note that Bows of "Extra Shots" give an extra shot.
 */
void player_fire_or_throw_selected(int item, bool fire)
{
	int item2 = 0;
	int i = 0;
	int j, y, x, tricks, tdis;
	int bow_to_h = 0;
	int bow_to_d = 0;
	int ranged_skill;
	int catch_chance = 0;
	int inaccuracy = 5;

	int style_hit, style_dam, style_crit;

	object_type *o_ptr = item >= 0 ? &inventory[item] : &o_list[0 - item];
	object_type *k_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	bool ammo_can_break = TRUE;
	bool trick_failure = FALSE;
	bool short_range = FALSE;
	bool chasm = FALSE;
	bool fumble = FALSE;
	bool activate = FALSE;
	int feat;

	byte missile_attr;
	char missile_char;

	char o_name[80];

	int path_n;
	u16b path_g[256];

	int use_old_target_backup = use_old_target;

	/* Get kind flags */
	u32b f5 = k_info[o_ptr->k_idx].flags5;

	bool throwing = is_throwing_item(o_ptr);
	bool trick_throw = !fire && ((item == INVEN_WIELD) || (f5 & (TR5_TRICK_THROW))) && throwing;
	int num_tricks = trick_throw ? p_ptr->num_blow + 1 : 1;

	/* Get kind flags */
	u32b f6 = k_info[o_ptr->k_idx].flags6;

	/* Need a rope? (No rope for trick throws) */
	if ((f6 & (TR6_HAS_ROPE | TR6_HAS_CHAIN)) && !trick_throw)
	{
		cptr q, s;

		/* Allow chain for some weapons */
		if (f6 & (TR6_HAS_CHAIN))
			item_tester_tval = TV_ROPE;

		/* Require rope */
		else item_tester_hook = item_tester_hook_rope;

		/* Get an item */
		q = "Attach which rope? ";
		s = "You have no rope to attach.";
		if (get_item(&item2, q, s, (USE_INVEN | USE_FLOOR)))
		{
			/* Get the object */
			if (item2 >= 0)
			{
				k_ptr = &inventory[item2];
			}
			else
			{
				k_ptr = &o_list[0 - item2];
			}

			/* In a bag? */
			if (k_ptr->tval == TV_BAG)
			{
				/* Get item from bag */
				if (get_item_from_bag(&item2, q, s, k_ptr))
				{
					/* Refer to the item */
					k_ptr = &inventory[item2];
				}
			}
		}
		else
		{
			k_ptr = &o_list[0 - item2];
		}
	}
	else
	{
		k_ptr = NULL;
	}

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, o_ptr);

	/* Single object */
	i_ptr->number = 1;

	/* Reset stack counter */
	i_ptr->stackc = 0;

	/* No longer 'stored' */
	i_ptr->ident &= ~(IDENT_STORE);

	/* Describe the object */
	object_desc(o_name, sizeof(o_name), i_ptr, FALSE, 3);

	/* Find the color and symbol for the object */
	missile_attr = object_attr(i_ptr);
	missile_char = object_char(i_ptr);

	/* The first piece of code dependent on fire/throw */
	if (fire)
	{
		/* Get the bow */
		bow_to_h = inventory[INVEN_BOW].to_h;
		bow_to_d = inventory[INVEN_BOW].to_d;

		ranged_skill = p_ptr->skills[SKILL_TO_HIT_BOW];

		/* Base range XXX XXX */
		tdis = 6 + 3 * p_ptr->ammo_mult;
	}
	else
	{
		int mul, div;

		/* Compensating the lack of bow, add item bonuses twice; once here: */
		bow_to_h = i_ptr->to_h;
		bow_to_d = i_ptr->to_d;

		ranged_skill = p_ptr->skills[SKILL_TO_HIT_THROW];

		/* Badly balanced big weapons waste the throwing skill.
	 	Various junk (non-weapons) does not have to_hit,
	 	so don't penalize a second time. */
		if (!throwing
			&& (f6 & (TR6_BAD_THROW)))
				ranged_skill /= 2;

		/* Extract a "distance multiplier" */
		mul = throwing ? 10 : 3;

		/* Enforce a minimum "weight" of one pound */
		div = i_ptr->weight > 10 ? i_ptr->weight : 10;

		/* Hack -- Distance -- Reward strength, penalize weight */
		tdis = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;

		/* Max distance of 10 */
		if (tdis > 10)
		tdis = 10;
	}

	/* Coordinates of the fired/thrown object; start at the player */
	y = p_ptr->py;
	x = p_ptr->px;

	/* Increase inaccuracy */
	if (p_ptr->heavy_wield) inaccuracy += 5;
	if (coated_p(o_ptr)) inaccuracy += 2;
	if (cursed_p(o_ptr)) inaccuracy += 10;
	if (fire && (cursed_p(&inventory[INVEN_BOW]))) inaccuracy += 5;

	/* Decrease inaccuracy */
	inaccuracy -= ranged_skill / 40;

	/* Minimum inaccuracy */
	if (inaccuracy < 0) inaccuracy = 0;

	/* Hack to avoid divide by zero, and give nonskilled users
	 * a chance to shoot without fumbling */
	if ((ranged_skill <= 1) && (rand_int(100) < 50)) ranged_skill = 2;
	
	/* Test for fumble */
	/* XXX Avoid divide by zero here */
	if ((ranged_skill <= 1)
		|| ((inaccuracy > 5) && (rand_int(ranged_skill) < inaccuracy)))
	{
		int dir;

		/* Fake get a direction */
		if (!get_aim_dir(&dir, TARGET_KILL, tdis, 0, (PROJECT_BEAM), 0, 0)) return;

		fumble = TRUE;
	}

	/* Iterate through trick throw targets;
	 the last pass is for returning to player;
	 if no tricks, just one iteration */
	else for (tricks = 0; tricks < num_tricks; tricks++)
	{
		int dir = 0;
		int old_y, old_x; /* Previous weapon location */
		int ty, tx; /* Current target coordinates */

		bool report_miss = TRUE;	/* Report first monster we miss */

		/* If trick throw failure, stop trick throws */
		if (trick_failure) break;

		/* If too far or all tricks used, return to player; no target query */
		if (trick_throw
		&& (!ammo_can_break
			|| tricks == num_tricks - 1))
		{
			tx = p_ptr->px;
			ty = p_ptr->py;
		}
		else
		{
			/* Get a direction (or cancel) */
			tx = x;
			ty = y;
			while (tx == x && ty == y)
			{
				/* Reset the chosen direction */
				p_ptr->command_dir = 0;

				if (!get_aim_dir(&dir, TARGET_KILL, tdis, 0, (PROJECT_BEAM), 0, 0))
				{
					/* Canceled */
					if (tricks > 0)
					/* If canceled mid-trick-throw, try to return weapon */
					{
						tx = p_ptr->px;
						ty = p_ptr->py;
						break;
					}
					else
					/* If canceled before first throw, cancel whole move */
					{
						return;
					}
				}
				else
				{
					/* No cancel */

					/* Check for "target request" */
					if (dir == 5 && target_okay())
					{
						tx = p_ptr->target_col;
						ty = p_ptr->target_row;
					}
					else
					{
						/* Predict the "target" location */
						ty = y + 99 * ddy[dir];
						tx = x + 99 * ddx[dir];
					}

					/* Disable auto-target for the rest of trick throws */
					use_old_target = FALSE;
				}
			}

			/* Sound */
			sound(MSG_SHOOT);
		}

		/* If the weapon returns, don't limit the distance */
		if (tx == p_ptr->px && ty == p_ptr->py) tdis = 256;

		/* Calculate the path */
		path_n = fire_or_throw_path(path_g, tdis, y, x, &ty, &tx, inaccuracy);

		/* Hack -- Handle stuff */
		handle_stuff();
		
		/* Save the source of the shot/throw */
		old_y = y;
		old_x = x;

		/* Reset ammo_can_break */
		if (f6 & (TR6_BREAK_THROW))
		{
			/* Hack -- flasks, potions, spores break as if striking a monster */
			ammo_can_break = TRUE;
		}
		else
		{
			/* Otherwise not breaking by default */
			ammo_can_break = FALSE;
		}
		
		/* Throwing/firing an item at the character's feet activates it always */
		if (path_n < 1) activate = TRUE;

		/* Project along the path */
		for (i = 0; i < path_n; ++i)
		{
			int msec = op_ptr->delay_factor * op_ptr->delay_factor;

			int ny = GRID_Y(path_g[i]);
			int nx = GRID_X(path_g[i]);

			/* Hack -- Stop before hitting walls */
			if (!cave_project_bold(ny, nx))
			{
				/* 1st cause of failure: returning weapon hits a wall */
			        trick_failure = trick_failure || (tdis == 256);

				break;
			}
			
			/* Advance */
			x = nx;
			y = ny;

			/* Hack -- Stop if we are throwing at short range */
			if (!fire && target_okay() && (x == p_ptr->target_col) && (y == p_ptr->target_row) && (i < tdis / 2))
			{
				short_range = TRUE;
				
				/* Activate if we are at point blank range */
				if (i < 1) activate = TRUE;
				
				break;
			}

			/* Handle rope over chasm */
			if (k_ptr)
			{
				/* No worry that rope removed from inventory many times */
				assert (!trick_throw);

				feat = cave_feat[y][x];

				if (f_info[feat].flags2 & (FF2_CHASM))
					chasm = TRUE;
				else
					chasm = FALSE;

				feat = feat_state(feat, FS_SPIKE);

				if (strstr(f_name + f_info[feat].name, "rope"))
				{
					/* Hack -- remove spike */
					feat = feat_state(feat, FS_GET_FEAT);

					/* MegaHack -- handle chain */
					if (k_ptr->sval == SV_ROPE_CHAIN)
					feat++;

					/* Change the feature */
					cave_set_feat(y, x, feat);

					/* Reduce inventory -- suppress messages */
					if (item2 >= 0)
					{
						if (k_ptr->number == 1)
						{
							inven_drop_flags(k_ptr);
							k_ptr = NULL;
						}

						inven_item_increase(item2, -1);
						inven_item_optimize(item2);
					}

					/* Reduce and describe floor item */
					else
					{
						if (k_ptr->number == 1) k_ptr = NULL;

						floor_item_increase(0 - item2, -1);
						floor_item_optimize(0 - item2);
					}
				}
			}

			/* Only do visuals if the player can "see" the missile */
			if (panel_contains(y, x) && player_can_see_bold(y, x))
			{
				/* Visual effects */
				print_rel(missile_char, missile_attr, y, x);
				move_cursor_relative(y, x);
				Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(y, x);
				Term_fresh();
			}

			/* Delay anyway for consistency */
			else
			{
				/* Pause anyway, for consistancy */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}

			/* Always catch returning weapon */
			if (tdis == 256 && x == p_ptr->px && y == p_ptr->py)
			  break;

			/* Handle monster */
			if (cave_m_idx[y][x] > 0)
			{
				monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
				monster_lore *l_ptr = &l_list[cave_m_idx[y][x]];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				int visible = m_ptr->ml;
				int bonus;
				int chance;
				int chance2;
				int tdam;

				bool hit_or_near_miss;
				bool genuine_hit;

				/* Ignore hidden monsters */
				if (m_ptr->mflag & (MFLAG_HIDE)) continue;

				/* If the weapon returns, ignore monsters */
				if (tdis == 256) continue;
				
				/* Not short range if hitting a monster */
				short_range = FALSE;

				/* Badly balanced weapons do minimum damage
			 	Various junk (non-weapons) do not use damage for anything else
			 	so don't penalize a second time. */
				if (!fire && !throwing
					&& (f6 & (TR6_BAD_THROW)))
					/* Minimum damage from the object */
					tdam = i_ptr->dd;
				else
					/* Base damage from the object */
					tdam = damroll(i_ptr->dd, i_ptr->ds);

				/* The second fire/throw dependent code piece */
				if (fire)
				{
					u32b shoot_style;

					/* Boost the damage */
					tdam *= p_ptr->ammo_mult;

					/* Some monsters are great at dodging	-EZ- */
					if (mon_evade(cave_m_idx[y][x],
						m_ptr->cdis + (m_ptr->confused
								 || m_ptr->stunned ? 1 : 3),
						5 + m_ptr->cdis,
						" your shot"))
					continue;

					/* Check shooting styles only */
					shoot_style = p_ptr->cur_style & WS_LAUNCHER_FLAGS;

					/* Get style benefits */
					mon_style_benefits(m_ptr, shoot_style,
							 &style_hit, &style_dam, &style_crit);
				}
				else
				{
					/* Long throws are easier to dodge than long shots */
					if (mon_evade(cave_m_idx[y][x],
						2 * m_ptr->cdis + (m_ptr->confused
										 || m_ptr->stunned ? 1 : 4),
						5 + 2 * m_ptr->cdis,
						" your throw"))
					continue;

					if (throwing)
						mon_style_benefits(m_ptr, WS_THROWN_FLAGS,
								 &style_hit, &style_dam, &style_crit);
					else
						style_hit = style_dam = style_crit = 0;
				}

				/* Actually "fire" the object */
				bonus = (p_ptr->to_h + i_ptr->to_h + bow_to_h + style_hit + (p_ptr->blocking ? 15 : 0));
				chance = ranged_skill + bonus * BTH_PLUS_ADJ;
				chance2 = chance - BTH_RANGE_ADJ * distance(old_y, old_x, y, x);

				/* Record for later */
				catch_chance = chance;

				/* Test hit fire */
				hit_or_near_miss = test_hit_fire(chance2,
								 calc_monster_ac(cave_m_idx[y][x], FALSE),
							 m_ptr->ml);

				/* Genuine hit */
				genuine_hit = test_hit_fire(chance2,
						calc_monster_ac(cave_m_idx[y][x], TRUE),
						m_ptr->ml);
				
				/* We are attacking - count attacks */
				if ((l_ptr->tblows < MAX_SHORT) && (m_ptr->ml)) l_ptr->tblows++;

				/* Missiles bounce off resistant monsters */
				if (genuine_hit && mon_resist_object(cave_m_idx[y][x], i_ptr))
				{
					/* XXX Rewrite remaining path of missile */

					continue;
				}

				/* Did we hit it or get close? */
				if (hit_or_near_miss || genuine_hit)
				{
					bool fear = FALSE;
					bool was_asleep = (m_ptr->csleep == 0);

					/* Assume a default death */
					cptr note_dies = " dies.";

					int mult = object_damage_multiplier(o_ptr, m_ptr, item < 0);

					/* Apply bow brands */
					if (fire) mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_BOW], m_ptr, item < 0) - 1);

					/* Use left-hand ring brand when shooting. Rings contribute 1 less multiplier. */
					if ((fire) && (inventory[INVEN_LEFT].k_idx))
					{
						mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_LEFT], m_ptr, item < 0) - 1);
					}

					/* Use right-hand ring brand when shooting. Rings contribute 1 less multiplier. */
					if ((!fire) && (inventory[INVEN_LEFT].k_idx))
					{
						mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_RIGHT], m_ptr, item < 0) - 1);
					}

					/* Get total damage */
					tdam = tot_dam_mult(tdam, mult);

					/* Note the collision */
					ammo_can_break = TRUE;

					/* 2nd cause of failure: bounce off monster */
					trick_failure = trick_failure || !genuine_hit;

					/* Disturb the monster */
					m_ptr->csleep = 0;

					/* Mark the monster as attacked by the player */
					if (m_ptr->cdis > 1) m_ptr->mflag |= MFLAG_HIT_RANGE;
					else m_ptr->mflag |= MFLAG_HIT_BLOW;

					/* Some monsters get "destroyed" */
					if (r_ptr->flags3 & RF3_NONLIVING || r_ptr->flags2 & RF2_STUPID)
					{
						/* Special note at death */
						note_dies = " is destroyed.";
					}

					/* The third piece of fire/throw dependent code */
					if (fire)
					{
						/* Apply missile critical damage */
						tdam += critical_shot(i_ptr->weight,
										bonus + style_crit * 30,
										tdam);
					}
					else if (throwing)
					{
						/* Throws (with specialized throwing weapons) hit harder */
						tdam += critical_norm(i_ptr->weight,
										bonus + style_crit * 30,
								tdam);
					}
					else
					{
						/* Throwing non-throwing items gives no criticals */
						tdam += 0;
					}

					/* Badly balanced weapons have less effective damage bonus
				 	Various junk (non-weapons) do not use damage for anything else
				 	so don't penalize a second time. */
					if (!fire && !throwing
						&& (f6 & (TR6_BAD_THROW)) && (i_ptr->to_d > 0))
						/* Halve damage bonus */
						tdam += i_ptr->to_d + style_dam;
					else
						/* Apply launcher, missile and style bonus */
						tdam += i_ptr->to_d + bow_to_d + style_dam;

					/* No negative damage */
					if (tdam < 0) tdam = 0;

					/* Handle unseen monster */
					if (!visible)
					{
						/* Invisible monster */
						msg_format("The %s finds a mark.", o_name);

						/* Near miss? */
						if (!genuine_hit) break;
					}

					/* Handle visible monster */
					else
					{
						char m_name[80];

						/* Get "the monster" or "it" */
						monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

						/* Near miss */
						if (!genuine_hit)
						{
							/* Missile was stopped */
							if (r_ptr->flags2 & (RF2_ARMOR)
								|| m_ptr->shield)
							{
								msg_format("%^s blocks the %s with a %sshield.",
										 m_name,
										 o_name,
										 m_ptr->shield ? "mystic " : "");
							}
							/* Miss the monster */
							else if (report_miss && visible)
							{
								msg_format("You miss %s.", m_name);
							}

							/* No further effect */
							break;
						}
						/* Successful hit */
						else
						{
							msg_format("The %s hits %s.", o_name, m_name);
						}

						/* Hack -- Track this monster race */
						if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

						/* Hack -- Track this monster */
						if (m_ptr->ml) health_track(cave_m_idx[y][x]);
					}

					/* Complex message */
					if (p_ptr->wizard)
					{
							msg_format("You do %d (out of %d) damage.",
							 tdam, m_ptr->hp);
					}

					/* Hit the monster, check for death */
					if (mon_take_hit(cave_m_idx[y][x], tdam, &fear, note_dies))
					{
						/* Dead monster */
					}

					/* No death */
					else
					{
						/* Message */
						message_pain(cave_m_idx[y][x], tdam);

						/* Alert fellows */
						if (was_asleep)
						{
							m_ptr->mflag |= (MFLAG_AGGR);

							/* Let allies know */
							tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN),
									"& has attacked me!");
						}
						else if (fear)
						{
							tell_allies_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN),
									"& has hurt me badly!");
						}

						/* Take note */
						if (fear && m_ptr->ml)
						{
							char m_name[80];

							/* Get the monster name (or "it") */
							monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

							/* Message */
							message_format(MSG_FLEE, m_ptr->r_idx,
								 "%^s flees in terror!", m_name);
						}

						/* Apply additional effect from activation/coating */
						activate = TRUE;
					}

					/* Check usage */
					object_usage(item);

					/* Copy learned values to the thrown/fired weapon.
					 * We do this to help allow weapons to stack. */
					i_ptr->ident = o_ptr->ident;
					i_ptr->usage = o_ptr->usage;
					i_ptr->feeling = o_ptr->feeling;
					i_ptr->guess1 = o_ptr->guess1;
					i_ptr->guess2 = o_ptr->guess2;
					i_ptr->can_flags1 = o_ptr->can_flags1;
					i_ptr->can_flags2 = o_ptr->can_flags2;
					i_ptr->can_flags3 = o_ptr->can_flags3;
					i_ptr->can_flags4 = o_ptr->can_flags4;
					i_ptr->may_flags1 = o_ptr->may_flags1;
					i_ptr->may_flags2 = o_ptr->may_flags2;
					i_ptr->may_flags3 = o_ptr->may_flags3;
					i_ptr->may_flags4 = o_ptr->may_flags4;
					i_ptr->not_flags1 = o_ptr->not_flags1;
					i_ptr->not_flags2 = o_ptr->not_flags2;
					i_ptr->not_flags3 = o_ptr->not_flags3;
					i_ptr->not_flags4 = o_ptr->not_flags4;

					/* Stop looking */
					break;
				}
				/* Tell the player they missed */
				else if (report_miss && visible)
				{
					char m_name[80];

					/* Get the monster name (or "it") */
					monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

					msg_format("You miss %s.", m_name);

					report_miss = FALSE;
				}
			}
			/* Handle reaching targetted grid */
			else if ((i == path_n - 1) && !(trick_throw))
			{
				/* Activate item */
				activate = TRUE;
			}
		}

		/* 3rd and last cause of failure: out of range, unless returning */
		trick_failure = trick_failure || (i == path_n && tdis != 256);

		/* Apply effects of activation/coating */
		if (activate)
		{
			/* Apply additional effect from activation */
			if (auto_activate(o_ptr))
			{
				/* Make item strike */
				process_item_blow(o_ptr->name1 ? SOURCE_PLAYER_ACT_ARTIFACT : (o_ptr->name2 ? SOURCE_PLAYER_ACT_EGO_ITEM : SOURCE_PLAYER_ACTIVATE),
						o_ptr->name1 ? o_ptr->name1 : (o_ptr->name2 ? o_ptr->name2 : o_ptr->k_idx), i_ptr, y, x,  TRUE, (play_info[y][x] & (PLAY_FIRE)) == 0);
			}

			/* Apply additional effect from coating*/
			else if (coated_p(o_ptr))
			{
				/* Make item strike */
				process_item_blow(SOURCE_PLAYER_COATING, lookup_kind(o_ptr->xtra1, o_ptr->xtra2), i_ptr, y, x, TRUE, TRUE);
			}

			/* Chance of breakage */
			ammo_can_break = TRUE;

			/* Paranoia */
			activate = FALSE;
		}
	}

	/* Reenable auto-target */
	use_old_target = use_old_target_backup;

	/* Chance of breakage; trick throws always risky */
	j = (ammo_can_break || trick_throw) ? breakage_chance(i_ptr) : 0;

	/* The fourth and last piece of code dependent on fire/throw */
	if (fire)
	{
		/* Check usage of the bow */
		object_usage(INVEN_BOW);

		/* Take a turn */
		p_ptr->energy_use = 100 / p_ptr->num_fire;

		/* Check for charged weapon */
		if (inventory[INVEN_BOW].sval/10 == 3)
		{
			inventory[INVEN_BOW].charges--;
		}
	}
	else
	{
		int throws_per_round;

		if (trick_throw)
			throws_per_round = 1;
		else
			/* This does not depend on whether the weapon is a throwing weapon */
			throws_per_round = p_ptr->num_throw;

		/* Take a turn */
		p_ptr->energy_use = 100 / throws_per_round;
	}

	/* Is this a trick throw and has the weapon returned? */
	if (fumble || (trick_throw && !trick_failure))
	{
		if (!fumble)
		{
			/* Keep it sane */
			catch_chance = MAX(0, MAX(catch_chance, ranged_skill/5));

			/* Randomize */
			catch_chance = randint(catch_chance);
		}

		if ((fumble) || (catch_chance <= 2 + catch_chance / 20))
		/* You don't catch the returning weapon; it hits you */
		{
			/* TODO: this code is taken from hit_trap in cmd1.c --- factor it out */
			int k;

			/* Describe */
			object_desc(o_name, sizeof(o_name), i_ptr, FALSE, 0);

			k = damroll(i_ptr->dd, i_ptr->ds);

			/* The third piece of fire/throw dependent code */
			if (fire)
			{
				/* Apply missile critical damage */
				k += critical_shot(i_ptr->weight,
								bow_to_h + i_ptr->to_h,
								k);
			}
			else if (throwing)
			{
				/* Throws (with specialized throwing weapons) hit harder */
				k += critical_norm(i_ptr->weight,
								2 * i_ptr->to_h,
						k);
			}

			k += i_ptr->to_d;

			/* Armour reduces total damage */
			k -= (k * ((p_ptr->ac < 150) ? p_ptr->ac : 150) / 250);

			/* No negative damage */
			if (k < 0) k = 0;

			/* Damage */
			project_one(fire ? SOURCE_PLAYER_SHOT : SOURCE_PLAYER_THROW, i_ptr->k_idx, y, x, k, GF_HURT, (PROJECT_PLAY | PROJECT_HIDE));

			/* Apply additional effect from activation */
			if (auto_activate(o_ptr))
			{
				/* Make item strike */
				process_item_blow(SOURCE_PLAYER_ACT_ARTIFACT, o_ptr->name1, o_ptr, y, x, TRUE, FALSE);
			}

			/* Apply additional effect from coating*/
			else if (coated_p(o_ptr))
			{
				/* Make item strike */
				process_item_blow(SOURCE_PLAYER_COATING, lookup_kind(o_ptr->xtra1, o_ptr->xtra2), o_ptr, y, x, TRUE, TRUE);
			}

			/* Weapon caught */
			msg_print("Ouch! You prick yourself!");
		}
		else if (catch_chance <= 10 + catch_chance / 10)
		/* You don't catch the returning weapon; it almost hits you */
		{
			/* Describe */
			object_desc(o_name, sizeof(o_name), i_ptr, FALSE, 0);

			msg_format("The returning %^s bumps off your torso.", o_name);

			/* Weapon not caught */
			trick_failure = TRUE;
		}
		else
		{
			/* Weapon caught */
		}
	}

	/* Is this a trick throw and has the weapon returned? */
	if ((trick_throw && !trick_failure) ||
		/* Or the player trying to throw or fire away a cursed item? */
		((item >= INVEN_WIELD) && (cursed_p(o_ptr))))
	/* Try to return the weapon to the player */
	{
		/* Perhaps harm the weapon */
		if (trick_throw && !trick_failure && (rand_int(100) < j) && break_near(i_ptr, y, x))
		/* The returned weapon turned out to be totally broken */
		{
			/* Reduce and describe inventory */
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
		else
		/* Either intact or only mildly harmed */
		{
			/* Bad penny */
			if (cursed_p(o_ptr))
			{
				/* Inform the player */
				msg_print("You can't get rid of it that easily.");
			}

			/* Wear again the (possibly slighly harmed) weapon */
			object_copy(o_ptr, i_ptr);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff --- TODO: are all these needed? */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

			/* Update item list --- TODO: are all these needed? */
			p_ptr->redraw |= (PR_ITEM_LIST);
		}
	}
	else
	/* Not a successful trick throw; just drop near the last monster/wall */
	{
		/* Sometimes use lower stack object --- FIXME: clarify comment, search and replace similar comments */
		if (!object_charges_p(o_ptr) && rand_int(o_ptr->number) < o_ptr->stackc)
		{
			if (i_ptr->charges)
			i_ptr->charges--;

			if (i_ptr->timeout)
			i_ptr->timeout = 0;

			o_ptr->stackc--;
		}

		/* Reduce and describe inventory */
		if (item >= 0)
		{
			if (o_ptr->number == 1)
			{
				inven_drop_flags(o_ptr);
				if (item2 > item)
				item2--;
			}

			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}

		/* Reduce and describe floor item */
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_optimize(0 - item);

			/* Get feat */
			if (o_ptr->ident & (IDENT_STORE)
				&& scan_feat(p_ptr->py, p_ptr->px) < 0)
			cave_alter_feat(p_ptr->py, p_ptr->px, FS_GET_FEAT);
		}

		/* Forget information on dropped object --- FIXME: say why it is needed */
		drop_may_flags(i_ptr);

		/* At point blank or short range, don't break weapons which miss a monster, and put them exactly on the
		 * target square. */
		if (short_range) j = -2;

		/* Drop (or break) near that location */
		drop_near(i_ptr, j, y, x, FALSE);

		/* Rope doesn't reach other end of chasm */
		if (chasm)
		{
			/* Project along the path */
			for ( ; i >= 0; --i)
			{
				y = GRID_Y(path_g[i]);
				x = GRID_X(path_g[i]);

				feat = cave_feat[y][x];

				/* Drop rope into chasm */
				if (strstr(f_name + f_info[feat].name, "rope")
				|| strstr(f_name + f_info[feat].name, "chain"))
				{
					/* Hack -- drop into chasm */
					cave_alter_feat(y, x, FS_TIMED);
				}
				else
				{
					break;
				}
			}
		}
	}
}


/*
 * Returns TRUE if an object is permitted to be thrown
 */
bool item_tester_hook_throwable(const object_type *o_ptr)
{
	/* Spell cannot be thrown */
	if (o_ptr->tval == TV_SPELL) return (FALSE);

	/* Cannot throw any 'built-in' item type */
	if (o_ptr->ident & (IDENT_STORE)) return (FALSE);

	/* Allowed */
	return (TRUE);
}


/*
 * Returns TRUE if an object is permitted to be slung
 */
bool item_tester_hook_slingable(const object_type *o_ptr)
{
	/* Known throwing items can be slung */
	if (is_known_throwing_item(o_ptr)) return (TRUE);

	/* Sling shots */
	if (o_ptr->tval == TV_SHOT) return (TRUE);

	/* Not allowed */
	return (FALSE);
}


/*
 * Player throws an item.
 */
bool player_throw(int item)
{
	player_fire_or_throw_selected(item, FALSE);

	return TRUE;
}


/*
 * Player fires an item.
 */
bool player_fire(int item)
{
	player_fire_or_throw_selected(item, TRUE);

	return TRUE;
}
