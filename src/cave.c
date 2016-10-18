/* File: cave.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.6
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Additional permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"

/*
 * Support for Adam Bolt's tileset, lighting and transparency effects
 * by Robert Ruehlmann (rr9@angband.org)
 */

/*
 * Approximate distance between two points.
 *
 * When either the X or Y component dwarfs the other component,
 * this function is almost perfect, and otherwise, it tends to
 * over-estimate about one grid per fifteen grids of distance.
 *
 * Algorithm: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2
 */
sint distance(int y1, int x1, int y2, int x2)
{
	int ay, ax;

	/* Find the absolute y/x distance components */
	ay = (y1 > y2) ? (y1 - y2) : (y2 - y1);
	ax = (x1 > x2) ? (x1 - x2) : (x2 - x1);

	/* Hack -- approximate the distance */
	return ((ay > ax) ? (ay + (ax>>1)) : (ax + (ay>>1)));
}


/*
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.
 *
 * This function returns TRUE if a "line of sight" can be traced from the
 * center of the grid (x1,y1) to the center of the grid (x2,y2), with all
 * of the grids along this path (except for the endpoints) being non-wall
 * grids.  Actually, the "chess knight move" situation is handled by some
 * special case code which allows the grid diagonally next to the player
 * to be obstructed, because this yields better gameplay semantics.  This
 * algorithm is totally reflexive, except for "knight move" situations.
 *
 * Because this function uses (short) ints for all calculations, overflow
 * may occur if dx and dy exceed 90.
 *
 * Once all the degenerate cases are eliminated, we determine the "slope"
 * ("m"), and we use special "fixed point" mathematics in which we use a
 * special "fractional component" for one of the two location components
 * ("qy" or "qx"), which, along with the slope itself, are "scaled" by a
 * scale factor equal to "abs(dy*dx*2)" to keep the math simple.  Then we
 * simply travel from start to finish along the longer axis, starting at
 * the border between the first and second tiles (where the y offset is
 * thus half the slope), using slope and the fractional component to see
 * when motion along the shorter axis is necessary.  Since we assume that
 * vision is not blocked by "brushing" the corner of any grid, we must do
 * some special checks to avoid testing grids which are "brushed" but not
 * actually "entered".
 *
 * Angband three different "line of sight" type concepts, including this
 * function (which is used almost nowhere), the "project()" method (which
 * is used for determining the paths of projectables and spells and such),
 * and the "update_view()" concept (which is used to determine which grids
 * are "viewable" by the player, which is used for many things, such as
 * determining which grids are illuminated by the player's torch, and which
 * grids and monsters can be "seen" by the player, etc).
 */
bool generic_los(int y1, int x1, int y2, int x2, byte flg)
{
	/* Delta */
	int dx, dy;

	/* Absolute */
	int ax, ay;

	/* Signs */
	int sx, sy;

	/* Fractions */
	int qx, qy;

	/* Scanners */
	int tx, ty;

	/* Scale factors */
	int f1, f2;

	/* Slope, or 1/Slope, of LOS */
	int m;

	/* Paranoia */
	if (!flg) flg = CAVE_XLOS;

	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);


	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (TRUE);


	/* Directly South/North */
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (!cave_flag_bold(ty, x1, flg)) return (FALSE);
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (!cave_flag_bold(ty, x1, flg)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (!cave_flag_bold(y1, tx, flg)) return (FALSE);
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (!cave_flag_bold(y1, tx, flg)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1)
	{
		if (ay == 2)
		{
			if (cave_flag_bold(y1 + sy, x1, flg)) return (TRUE);
		}
	}

	/* Horizontal "knights" */
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (cave_flag_bold(y1, x1 + sx, flg)) return (TRUE);
		}
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;

	/* Travel horizontally */
	if (ax >= ay)
	{
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (!cave_flag_bold(ty, tx, flg)) return (FALSE);

			qy += m;

			if (qy < f2)
			{
				tx += sx;
			}
			else if (qy > f2)
			{
				ty += sy;
				if (!cave_flag_bold(ty, tx, flg)) return (FALSE);
				qy -= f1;
				tx += sx;
			}
			else
			{
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else
	{
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2)
		{
			tx = x1 + sx;
			qx -= f1;
		}
		else
		{
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty)
		{
			if (!cave_flag_bold(ty, tx, flg)) return (FALSE);

			qx += m;

			if (qx < f2)
			{
				ty += sy;
			}
			else if (qx > f2)
			{
				tx += sx;
				if (!cave_flag_bold(ty, tx, flg)) return (FALSE);
				qx -= f1;
				ty += sy;
			}
			else
			{
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (TRUE);
}



/*
 * Returns true if the player's grid is dark
 */
bool no_lite(void)
{
	return (!player_can_see_bold(p_ptr->py, p_ptr->px));
}




/*
 * Determine if a given location may be "destroyed"
 *
 * Used by destruction spells, and for placing stairs, etc.
 */
bool cave_valid_bold(int y, int x)
{
	s16b this_o_idx, next_o_idx = 0;


	/* Forbid perma-grids */
	if (cave_perma_bold(y, x)) return (FALSE);

	/* Check objects */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Forbid artifact grids */
		if (artifact_p(o_ptr)) return (FALSE);
	}

	/* Accept */
	return (TRUE);
}



/*
 * Hack -- Legal monster codes
 */
static const char image_monster_hack[] = \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Hack -- Hallucinatory monster
 */
static u16b image_monster(void)
{
	byte a;
	char c;

	/* Random symbol from set above (not including final nul) */
	c = image_monster_hack[rand_int(sizeof(image_monster_hack) - 1)];

	/* Random color */
	a = (byte)randint(15);

	/* Encode */
	return (PICT(a,c));
}


/*
 * Hack -- Legal object codes
 */
static const char image_object_hack[] = \
"?/|\\\"!$()_-=[]{},~"; /* " */

/*
 * Hack -- Hallucinatory object
 */
static u16b image_object(void)
{
	byte a;
	char c;

	/* Random symbol from set above (not including final nul) */
	c = image_object_hack[rand_int(sizeof(image_object_hack) - 1)];

	/* Random color */
	a = (byte)randint(15);

	/* Encode */
	return (PICT(a,c));
}


/*
 * Hack -- Random hallucination
 */
static u16b image_random(void)
{
	/* Normally, assume monsters */
	if (rand_int(100) < 75)
	{
		return (image_monster());
	}

	/* Otherwise, assume objects */
	else
	{
		return (image_object());
	}
}


/*
 * These tables are built by having a 'or'ing the value below when
 * there is a wall in the same direction.
 *
 * This allows certain items to be defined in their relationship
 * to walls, for instance, having doors be either - or | depending
 * on how the adjacent walls are created.
 *
 * Similarly, for tile sets where walls are drawn as connected to
 * adjacent walls, it allows up to 15 types of walls to be defined
 * depending on the adjacent walls.
 */


/*
 * Offset to door char based on adjacent wall grids.
 */
byte door_char[16] =
{
	0,	0,	0,	0,
	1,	0,	0,	0,
	1,	1,	0,	0,
	1,	1,	1,	0
};


/*
 * Offset to wall char based on adjacent wall grids.
 */
byte wall_char[16] =
{
	14,	0,	14,	3,
	4,	13,	1,	2,
	12,	11,	5,	8,
	9,	10,	6,	7,
};

#define WALL_S	1
#define WALL_N	2
#define WALL_E	4
#define WALL_W	8
#define WALL_SE	16
#define WALL_SW	32
#define WALL_NE	64
#define WALL_NW	128


/*
 * Modify a grid appearance based on the adjacent walls
 *
 * Note in order to avoid weirdness with grids on the edge of known areas, we
 * ignore whether or not the player knows about adjacent walls.
 */
void modify_grid_adjacent_view(byte *a, char *c, int y, int x, byte adj_char[16])
{
	byte walls = 0, cancel = 0;
	int i, n = 4;

	(void)a;

	/* Hack -- check diagonals if proper wall */
	if (adj_char == wall_char) n = 8;

	for (i = 0; i < n; i++)
	{
		int yy = y + ddy_ddd[i];
		int xx = x + ddx_ddd[i];

		/* Treat annoying locations as if a wall */
		if (!in_bounds_fully(yy, xx))
		{
			walls |= 1 << i;
		}

		/* Note we use the mimic'ed value, but always assume this is known */
		else if ((f_info[f_info[cave_feat[yy][xx]].mimic].flags3 & (FF3_ATTR_WALL | FF3_ATTR_DOOR)) != 0)
		{
			walls |= 1 << i;
		}

		/* Note we check the underneath value as well, but always assume this is known */
		else if ((f_info[f_info[f_info[cave_feat[yy][xx]].mimic].under].flags3 & (FF3_ATTR_WALL | FF3_ATTR_DOOR)) != 0)
		{
			walls |= 1 << i;
		}

	}

	/* Hack -- if both diagonals set, clear wall in between.
	 * This saves having to have a 256 character array for walls and makes
	 * edges of continguous walls look a lot better.
	 *
	 * This relies on the fact that usually we will not see walls beyond the
	 * edge of a room. So we can approximate by 'cancelling' the effect of walls
	 * in certain directions for determining whether we have to display an
	 * adjacent wall.
	 */
	if ((walls & (WALL_SE | WALL_SW | WALL_E | WALL_W)) == (WALL_SE | WALL_SW | WALL_E | WALL_W)) cancel |= (WALL_S);
	if ((walls & (WALL_SE | WALL_NE | WALL_S | WALL_N)) == (WALL_SE | WALL_NE | WALL_S | WALL_N)) cancel |= (WALL_E);
	if ((walls & (WALL_SW | WALL_NW | WALL_S | WALL_N)) == (WALL_SW | WALL_NW | WALL_S | WALL_N)) cancel |= (WALL_W);
	if ((walls & (WALL_NE | WALL_NW | WALL_E | WALL_W)) == (WALL_NE | WALL_NW | WALL_E | WALL_W)) cancel |= (WALL_N);

	/* Cancel only after checking all combinations */
	if (cancel) walls &= ~(cancel);

	/* Clear if completely cancelled */
	if (cancel == (WALL_N | WALL_S | WALL_E | WALL_W))
	{
		*a = f_info[FEAT_NONE].x_attr;
		*c = f_info[FEAT_NONE].x_char;
	}

	/* Otherwise apply an offset */
	else
	{
		/* Modify char by adj_char offset */
		*c += adj_char[walls & (WALL_N | WALL_S | WALL_E | WALL_W)];
	}
}


/*
 * Translate text colours.
 *
 * This translates a color based on the attribute. We use this to set terrain to
 * be lighter or darker, make metallic monsters shimmer, highlight text under the
 * mouse, and reduce the colours on mono colour or 16 colour terms to the correct
 * colour space.
 *
 * TODO: Honour the attribute for the term (full color, mono, 16 color) but ensure
 * that e.g. the lighter version of yellow becomes white in a 16 color term, but
 * light yellow in a full colour term.
 */
byte get_color(byte a, int attr, int n)
{
	/* Accept any graphical attr (high bit set) */
	if (a & (0x80)) return (a);

	/* TODO: Honour the attribute for the term (full color, mono, 16 color) */
	if (!attr)
	{
		return(a);
	}

	/* Note: attempt at efficiency hack by unrolling loop. Whether this sort of thing works anymore
	 * is pretty questionable. However, we never call this function with n > 2 at the moment. */
	else
	{
loop_hack:
		switch(n)
		{
		case 2: a = color_table[a].color_translate[attr];
		case 1: a = color_table[a].color_translate[attr];
		case 0: break;
		default: a = color_table[a].color_translate[attr];
				 a = color_table[a].color_translate[attr];
				 a = color_table[a].color_translate[attr];
				 n-= 3;
				 goto loop_hack;
		}
	}

	/* Return the modified color */
	return (a);
}


/*
 * Modify a 'boring' grid appearance based on the ambient light
 */
void modify_grid_boring_view(byte *a, char *c, int y, int x, byte cinfo, byte pinfo)
{
	(void)y;
	(void)x;

	/* Handle "seen" grids */
	if (pinfo & (PLAY_SEEN))
	{
		/* Lit by "torch" lite */
		if (view_yellow_lite && !(cinfo & (CAVE_GLOW | CAVE_HALO | CAVE_DLIT)))
		{
			/* Mega-hack */
			if (*a & 0x80)
			{
				/* Use a brightly lit tile */
				if (arg_graphics == GRAPHICS_DAVID_GERVAIS)
					*c -= 1;
				else if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
					*c += 2;
			}
			else
			{
				/* Use "yellow" */
				*a = get_color(*a, ATTR_LITE, 1);
			}
		}
	}

	/* Handle "blind" or "asleep" */
	else if ((p_ptr->timed[TMD_BLIND]) || (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_BLIND, 1);
		}
	}

	/* Handle "dark" grids */
	else if (!(cinfo & (CAVE_LITE)))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "view_bright_lite" */
	else if (view_bright_lite)
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "gray" */
			*a = get_color(*a, ATTR_DARK, 1);
		}
	}
}

/*
 * Modify an 'unseen' grid appearance based on the ambient light
 */
void modify_grid_unseen_view(byte *a, char *c)
{
	/* Handle "blind", "asleep" and night time*/
	if ((p_ptr->timed[TMD_BLIND])
		|| p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP
		|| !(level_flag & (LF1_DAYLIGHT)))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "dark" grids */
	else if (view_bright_lite)
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO)
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "gray" */
			*a = get_color(*a, ATTR_DARK, 1);
		}
	}
}


/*
 * Modify an 'interesting' grid appearance based on the ambient light
 */
void modify_grid_interesting_view(byte *a, char *c, int y, int x, byte cinfo, byte pinfo)
{
	(void)y;
	(void)x;

	/* Handle "seen" grids */
	if (pinfo & (PLAY_SEEN))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			/* Use a lit tile */
		}
		else
		{
			/* Use "white" */
		}
	}

	/* Handle "blind" or "asleep" */
	else if ((p_ptr->timed[TMD_BLIND]) || (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "dark" grids */
	else if (!(cinfo & (CAVE_LITE)))
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "dark gray" */
			*a = get_color(*a, ATTR_DARK, 2);
		}
	}

	/* Handle "view_bright_lite" */
	else if (view_bright_lite)
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
			{
				/* Use a dark tile */
				*c += 1;
			}
		}
		else
		{
			/* Use "gray" */
			*a = get_color(*a, ATTR_DARK, 1);
		}
	}
	else
	{
		/* Mega-hack */
		if (*a & 0x80)
		{
			/* Use a brightly lit tile */
			if (arg_graphics == GRAPHICS_DAVID_GERVAIS)
				*c -= 1;
			else if ((arg_graphics != GRAPHICS_ORIGINAL) && (arg_graphics != GRAPHICS_DAVID_GERVAIS_ISO))
				*c += 2;
		}
		else
		{
			/* Use "white" */
		}
	}
}



/*
 * Checks if a square is at the (outer) edge of a trap detect area
 */
bool safe_edge(int y, int x)
{
 	/* Check for non-dtrap adjacent grids */
 	if (in_bounds_fully(y + 1, x    ) && (play_info[y + 1][x    ] & PLAY_SAFE)) return TRUE;
 	if (in_bounds_fully(y    , x + 1) && (play_info[y    ][x + 1] & PLAY_SAFE)) return TRUE;
 	if (in_bounds_fully(y - 1, x    ) && (play_info[y - 1][x    ] & PLAY_SAFE)) return TRUE;
 	if (in_bounds_fully(y    , x - 1) && (play_info[y    ][x - 1] & PLAY_SAFE)) return TRUE;

	return FALSE;
}

/*
 * Extract the attr/char to display at the given (legal) map location
 *
 * Note that this function, since it is called by "lite_spot()" which
 * is called by "update_view()", is a major efficiency concern.
 *
 * Basically, we examine each "layer" of the world (terrain, objects,
 * monsters/players), from the bottom up, extracting a new attr/char
 * if necessary at each layer, and defaulting to "darkness".  This is
 * not the fastest method, but it is very simple, and it is about as
 * fast as it could be for grids which contain no "marked" objects or
 * "visible" monsters.
 *
 * We apply the effects of hallucination during each layer.  Objects will
 * always appear as random "objects", monsters will always appear as random
 * "monsters", and normal grids occasionally appear as random "monsters" or
 * "objects", but note that these random "monsters" and "objects" are really
 * just "colored ascii symbols" (which may look silly on some machines).
 *
 * The hallucination functions avoid taking any pointers to local variables
 * because some compilers refuse to use registers for any local variables
 * whose address is taken anywhere in the function.
 *
 * As an optimization, we can handle the "player" grid as a special case.
 *
 * Note that the memorization of "objects" and "monsters" is not related
 * to the memorization of "terrain".  This allows the player to memorize
 * the terrain of a grid without memorizing any objects in that grid, and
 * to detect monsters without detecting anything about the terrain of the
 * grid containing the monster.
 *
 * The fact that all interesting "objects" and "terrain features" are
 * memorized as soon as they become visible for the first time means
 * that we only have to check the "CAVE_SEEN" flag for "boring" grids.
 *
 * Note that bizarre things must be done when the "attr" and/or "char"
 * codes have the "high-bit" set, since these values are used to encode
 * various "special" pictures in some versions, and certain situations,
 * such as "multi-hued" or "clear" monsters, cause the attr/char codes
 * to be "scrambled" in various ways.
 *
 * Note that the "zero" entry in the feature/object/monster arrays are
 * used to provide "special" attr/char codes, with "monster zero" being
 * used for the player attr/char, "object zero" being used for the "pile"
 * attr/char, and "feature zero" being used for the "darkness" attr/char.
 *
 * Note that eventually we may want to use the "&" symbol for embedded
 * treasure, and use the "*" symbol to indicate multiple objects, but
 * currently, we simply use the attr/char of the first "marked" object
 * in the stack, if any, and so "object zero" is unused.  XXX XXX XXX
 *
 * Note the assumption that doing "x_ptr = &x_info[x]" plus a few of
 * "x_ptr->xxx", is quicker than "x_info[x].xxx", even if "x" is a fixed
 * constant.  If this is incorrect then a lot of code should be changed.
 *
 *
 * Some comments on the "terrain" layer...
 *
 * Note that "boring" grids (floors, invisible traps, and any illegal grids)
 * are very different from "interesting" grids (all other terrain features),
 * and the two types of grids are handled completely separately.  The most
 * important distinction is that "boring" grids may or may not be memorized
 * when they are first encountered, and so we must use the "CAVE_SEEN" flag
 * to see if they are "see-able".
 *
 *
 * Some comments on the "terrain" layer (boring grids)...
 *
 * Note that "boring" grids are always drawn using the picture for "empty
 * floors", which is stored in "f_info[FEAT_FLOOR]".  Sometimes, special
 * lighting effects may cause this picture to be modified.
 *
 * Note that "invisible traps" are always displayes exactly like "empty
 * floors", which prevents various forms of "cheating", with no loss of
 * efficiency.  There are still a few ways to "guess" where traps may be
 * located, for example, objects will never fall into a grid containing
 * an invisible trap.  XXX XXX
 *
 * To determine if a "boring" grid should be displayed, we simply check to
 * see if it is either memorized ("CAVE_MARK"), or currently "see-able" by
 * the player ("CAVE_SEEN").  Note that "CAVE_SEEN" is now maintained by the
 * "update_view()" function.
 *
 * Note the "special lighting effects" which can be activated for "boring"
 * grids using the "view_special_lite" option, causing certain such grids
 * to be displayed using special colors (if they are normally "white").
 * If the grid is "see-able" by the player, we will use the normal "white"
 * (except that, if the "view_yellow_lite" option is set, and the grid
 * is *only* "see-able" because of the player's torch, then we will use
 * "yellow"), else if the player is "blind", we will use "dark gray",
 * else if the grid is not "illuminated", we will use "dark gray", else
 * if the "view_bright_lite" option is set, we will use "slate" (gray),
 * else we will use the normal "white".
 *
 *
 * Some comments on the "terrain" layer (non-boring grids)...
 *
 * Note the use of the "mimic" field in the "terrain feature" processing,
 * which allows any feature to "pretend" to be another feature.  This is
 * used to "hide" secret doors, and to make all "doors" appear the same,
 * and all "walls" appear the same, and "hidden" treasure stay hidden.
 * Note that it is possible to use this field to make a feature "look"
 * like a floor, but the "view_special_lite" flag only affects actual
 * "boring" grids.
 *
 * Since "interesting" grids are always memorized as soon as they become
 * "see-able" by the player ("CAVE_SEEN"), such a grid only needs to be
 * displayed if it is memorized ("CAVE_MARK").  Most "interesting" grids
 * are in fact non-memorized, non-see-able, wall grids, so the fact that
 * we do not have to check the "CAVE_SEEN" flag adds some efficiency, at
 * the cost of *forcing* the memorization of all "interesting" grids when
 * they are first seen.  Since the "CAVE_SEEN" flag is now maintained by
 * the "update_view()" function, this efficiency is not as significant as
 * it was in previous versions, and could perhaps be removed.
 *
 * Note the "special lighting effects" which can be activated for "wall"
 * grids using the "view_granite_lite" option, causing certain such grids
 * to be displayed using special colors (if they are normally "white").
 * If the grid is "see-able" by the player, we will use the normal "white"
 * else if the player is "blind", we will use "dark gray", else if the
 * "view_bright_lite" option is set, we will use "slate" (gray), else we
 * will use the normal "white".
 *
 * Note that "wall" grids are more complicated than "boring" grids, due to
 * the fact that "CAVE_GLOW" for a "wall" grid means that the grid *might*
 * be glowing, depending on where the player is standing in relation to the
 * wall.  In particular, the wall of an illuminated room should look just
 * like any other (dark) wall unless the player is actually inside the room.
 *
 * Thus, we do not support as many visual special effects for "wall" grids
 * as we do for "boring" grids, since many of them would give the player
 * information about the "CAVE_GLOW" flag of the wall grid, in particular,
 * it would allow the player to notice the walls of illuminated rooms from
 * a dark hallway that happened to run beside the room.
 *
 *
 * Some comments on the "object" layer...
 *
 * Currently, we do nothing with multi-hued objects, because there are
 * not any.  If there were, they would have to set "shimmer_objects"
 * when they were created, and then new "shimmer" code in "dungeon.c"
 * would have to be created handle the "shimmer" effect, and the code
 * in "cave.c" would have to be updated to create the shimmer effect.
 * This did not seem worth the effort.  XXX XXX
 *
 *
 * Some comments on the "monster"/"player" layer...
 *
 * Note that monsters can have some "special" flags, including "ATTR_MULTI",
 * which means their color changes, and "ATTR_CLEAR", which means they take
 * the color of whatever is under them, and "CHAR_CLEAR", which means that
 * they take the symbol of whatever is under them.  Technically, the flag
 * "CHAR_MULTI" is supposed to indicate that a monster looks strange when
 * examined, but this flag is currently ignored.
 *
 * Normally, players could be handled just like monsters, except that the
 * concept of the "torch lite" of others player would add complications.
 * For efficiency, however, we handle the (only) player first, since the
 * "player" symbol always "pre-empts" any other facts about the grid.
 *
 * ToDo: The transformations for tile colors, or brightness for the 16x16
 * tiles should be handled differently.  One possibility would be to
 * extend feature_type with attr/char definitions for the different states.
 */
void map_info(int y, int x, byte *ap, char *cp, byte *tap, char *tcp)
{
	byte a;
	char c;

	s16b feat;
	byte cinfo, pinfo;

	feature_type *f_ptr;

	s16b this_o_idx, next_o_idx = 0;

	s16b m_idx;

	s16b image = p_ptr->timed[TMD_IMAGE];

	int floor_num = 0;

	bool under = FALSE;
	bool trap = FALSE;


	/* Use the simple RNG to preserve seed from before save */
	Rand_quick = TRUE;

	/* Monster/Player */
	m_idx = cave_m_idx[y][x];

	/* Feature */
	feat = cave_feat[y][x];

	/* Cave flags */
	cinfo = cave_info[y][x];

	/* Player flags */
	pinfo = play_info[y][x];

	/* Hack -- rare random hallucination on non-outer walls */
	if (image && (!rand_int(256)) && (feat != FEAT_PERM_SOLID))
	{
		int i = image_random();

		a = PICT_A(i);
		c = PICT_C(i);
	}

	/* Boring grids (floors, etc) */
	else if (!(f_info[feat].flags1 & (FF1_REMEMBER)))
	{
		/* Mega Hack -- handle trees branches */
		if (p_ptr->outside && (f_info[feat].flags3 & (FF3_NEED_TREE)))
		{
			feat = FEAT_TREE_SHADE;
		}

		/* Memorized (or seen) floor */
		if ((pinfo & (PLAY_MARK)) ||
		    (pinfo & (PLAY_SEEN)))
		{
			/* Apply "mimic" field */
			feat = f_info[feat].mimic;

			/* Get the feature */
			f_ptr = &f_info[feat];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;

			/* Special wall effects */
			if (f_ptr->flags3 & (FF3_ATTR_WALL))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, wall_char);
			}

			/* Special wall effects */
			else if (f_ptr->flags3 & (FF3_ATTR_DOOR))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, door_char);
			}

			/* Special lighting effects */
			else if ((view_special_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
			{
				/* Modify the lighting */
				(*modify_grid_boring_hook)(&a, &c, y, x, cinfo, pinfo);
			}
		}

		/* Handle surface grids */
		else if (p_ptr->outside)
		{
			/* Get the feature */
			feat = f_info[feat].unseen;

			/* Get the feature*/
			f_ptr = &f_info[feat];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;

			/* Special wall effects */
			if (f_ptr->flags3 & (FF3_ATTR_WALL))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, wall_char);
			}

			/* Special wall effects */
			else if (f_ptr->flags3 & (FF3_ATTR_DOOR))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, door_char);
			}

			/* Special lighting effects */
			else if ((view_special_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
			{
				/* Modify lighting */
				(*modify_grid_unseen_hook)(&a, &c);
			}
		}

		/* Hack -- Safe cave grid -- now use 'invisible trap' */
		else if ((view_unsafe_grids || view_detect_grids) && !(pinfo & (PLAY_SAFE))
				&& (view_fogged_grids || safe_edge(y, x)))
		{
			/* Get the darkness feature */
			f_ptr = &f_info[FEAT_INVIS];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;
		}

		/* Unknown */
		else
		{
			/* Get the darkness feature */
			f_ptr = &f_info[FEAT_NONE];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;
		}
	}

	/* Interesting grids (non-floors) */
	else
	{
		/* Mega Hack -- handle trees branches */
		if (p_ptr->outside && (f_info[feat].flags3 & (FF3_NEED_TREE)))
		{
			feat = FEAT_TREE_SHADE;
		}

		/* Memorized grids */
		if (pinfo & (PLAY_MARK))
		{
			/* Apply "mimic" field */
			if (!cheat_wall) feat = f_info[feat].mimic;

			/* Apply "under" field if appropriate */
			if (f_info[feat].under)
			{
				/* Get the feat beneath */
				feat = f_info[feat].under;

				/* Showing terrain under */
				under = TRUE;
			}

			/* Get the feature */
			f_ptr = &f_info[feat];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;

			/* Special wall effects */
			if (f_ptr->flags3 & (FF3_ATTR_WALL))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, wall_char);
			}

			/* Special wall effects */
			else if (f_ptr->flags3 & (FF3_ATTR_DOOR))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, door_char);
			}

			/* Special lighting effects */
			else if ((view_granite_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
			{
				/* Modify lighting */
				(*modify_grid_interesting_hook)(&a, &c, y, x, cinfo, pinfo);
			}

			/* Note if a trap */
			if (f_ptr->flags1 & (FF1_TRAP)) trap = TRUE;
		}

		/* Handle surface grids */
		else if (p_ptr->outside)
		{
			/* Get the feature */
			feat = f_info[feat].unseen;

			/* Get the feature*/
			f_ptr = &f_info[feat];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;

			/* Special wall effects */
			if (f_ptr->flags3 & (FF3_ATTR_WALL))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, wall_char);
			}

			/* Special wall effects */
			else if (f_ptr->flags3 & (FF3_ATTR_DOOR))
			{
				/* Modify based on adjacent grids */
				(*modify_grid_adjacent_hook)(&a, &c, y, x, door_char);
			}

			/* Special lighting effects */
			else if ((view_special_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
			{
				/* Modify lighting */
				(*modify_grid_unseen_hook)(&a, &c);
			}
		}

		/* Hack -- Safe cave grid -- now use 'invisible trap' */
		else if ((view_unsafe_grids || view_detect_grids) && !(pinfo & (PLAY_SAFE))
				&& (view_fogged_grids || safe_edge(y, x)))
		{
			/* Get the darkness feature */
			f_ptr = &f_info[FEAT_INVIS];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;
		}

		/* Unknown */
		else
		{
			/* Get the darkness feature */
			f_ptr = &f_info[FEAT_NONE];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;
		}
	}


	/* Save the terrain info for the transparency effects */
	(*tap) = a;
	(*tcp) = c;

	/* Terrain over terrain */
	if (under)
	{
		/* Apply "mimic" field */
		feat = f_info[cave_feat[y][x]].mimic;

		/* Get the feature */
		f_ptr = &f_info[feat];

		/* Normal attr */
		a = f_ptr->x_attr;

		/* Normal char */
		c = f_ptr->x_char;

		/* Special wall effects */
		if (f_ptr->flags3 & (FF3_ATTR_WALL))
		{
			/* Modify based on adjacent grids */
			(*modify_grid_adjacent_hook)(&a, &c, y, x, wall_char);
		}

		/* Special wall effects */
		else if (f_ptr->flags3 & (FF3_ATTR_DOOR))
		{
			/* Modify based on adjacent grids */
			(*modify_grid_adjacent_hook)(&a, &c, y, x, door_char);
		}

		/* Special lighting effects */
		else if ((view_special_lite) && (view_granite_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
		{
			/* Modify lighting */
			(*modify_grid_boring_hook)(&a, &c, y, x, cinfo, pinfo);
		}

		/* Special lighting effects */
		else if ((view_granite_lite) && (f_ptr->flags3 & (FF3_ATTR_LITE)))
		{
			/* Modify lighting */
			(*modify_grid_interesting_hook)(&a, &c, y, x, cinfo, pinfo);
		}
	}

	/*
	 * Display regions when:
	 *  1.  when the player can see the region (either because it is PLAY_SEEN or in PLAY_VIEW and the region is self-lit
	 *  2.  when explicitly told to
	 *
	 *  as long as the feature underneath is projectable.
	 */
	if ((pinfo & (PLAY_REGN | PLAY_VIEW)) && (f_info[cave_feat[y][x]].flags1 & (FF1_PROJECT)))
	{
		s16b this_region_piece, next_region_piece = 0;

		for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
		{
			region_piece_type *rp_ptr = &region_piece_list[this_region_piece];
			region_type *r_ptr = &region_list[rp_ptr->region];

			/* Get the next region */
			next_region_piece = rp_ptr->next_in_grid;

			/* Skip dead regions */
			if (!r_ptr->type) continue;

			/* Displaying region */
			if (((cheat_hear) || ((r_ptr->flags1 & (RE1_NOTICE)) != 0)) &&
					(((pinfo & (PLAY_REGN | PLAY_SEEN)) != 0) ||
						(((pinfo & (PLAY_VIEW)) != 0) && ((r_ptr->flags1 & (RE1_SHINING)) != 0))) &&
							((r_ptr->flags1 & (RE1_DISPLAY)) != 0))
			{
				/* Skip because there is an underlying trap */
				if ((r_ptr->flags1 & (RE1_HIT_TRAP)) && (trap))
				{
					/* Don't display */
				}

				/* Display the effect */
				else if (r_ptr->flags1 & (RE1_ATTR_EFFECT))
				{
					u16b pict = bolt_pict(y, x, y, x, r_ptr->effect);

					a = PICT_A(pict);
					c = PICT_C(pict);
				}
				/* Display the region graphic */
				else
				{
					a = region_info[r_ptr->type].x_attr;
					c = region_info[r_ptr->type].x_char;
				}
				break;
			}
		}
	}

#if 0
	/* Room decoration -- should really be '3rd/ego' layer */
	if ((view_special_lite) && (pinfo & (PLAY_SEEN)) &&
		(cinfo & (CAVE_ROOM)) &&
		(f_info[cave_feat[y][x]] & (FF1_WALL)) &&
		(f_info[cave_feat[y][x]] & (FF1_OUTER | FF1_INNER | FF1_SOLID)))
	{
		/* Hack -- move towards player */
		int yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
		int xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

		/* Check for "complex" illumination */
		if ((!(cave_info[yy][xx] & (CAVE_ROOM)) &&
		      (play_info[yy][xx] & (PLAY_SEEN))) ||
		    (!(cave_info[y][xx] & (CAVE_ROOM)) &&
		      (play_info[y][xx] & (PLAY_SEEN))) ||
		    (!(cave_info[yy][x] & (CAVE_ROOM)) &&
		      (play_info[yy][x] & (PLAY_SEEN))))
		{
			int i = (f_ptr->flags & (FF1_OUTER) ? 1 : (f_ptr->flags & (FF1_INNER) ? 2 : (f_ptr->flags & (FF1_SOLID) ? 3);

			/* Special rooms affect some of this */
			int by = y/BLOCK_HGT;
			int bx = x/BLOCK_HGT;

			/* Get the room */
			room_info_type *d_ptr = &room_info[dun_room[by][bx]];

			/* Room dependent attr */
			a = d_ptr->x_attr[i];

			/* Room dependent char */
			c = d_ptr->x_char[i];
		}
	}
#endif


	/* Objects */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized objects */
		if (o_ptr->ident & (IDENT_MARKED))
		{
			/* Hack -- object hallucination */
			if (image)
			{
				int i = image_object();

				a = PICT_A(i);
				c = PICT_C(i);

				break;
			}

			/* Normal attr */
			a = object_attr(o_ptr);

			/* Normal char */
			c = object_char(o_ptr);

			/* First marked object */
			if (!show_piles) break;

			/* Special stack symbol */
			if (++floor_num > 1)
			{
				object_kind *k_ptr;

				/* Get the "pile" feature */
				k_ptr = &k_info[0];

				/* Normal attr */
				a = k_ptr->x_attr;

				/* Normal char */
				c = k_ptr->x_char;

				break;
			}
		}
	}


	/* Monsters */
	if (m_idx > 0)
	{
		monster_type *m_ptr = &m_list[m_idx];

		/* Visible monster */
		if (m_ptr->ml)
		{
			monster_race *r_ptr = &r_info[m_ptr->r_idx];

			byte da;
			char dc;

			/* Desired attr */
			da = r_ptr->x_attr;

			/* Desired char */
			dc = r_ptr->x_char;

			/* Hack -- monster hallucination */
			if (image)
			{
				int i = image_monster();

				a = PICT_A(i);
				c = PICT_C(i);
			}

			/* Special attr/char codes */
			else if ((da & 0x80) && (dc & 0x80))
			{
				/* Use attr */
				a = da;

				/* Use char */
				c = dc;
			}

			/* Multi-hued monster */
			else if (r_ptr->flags1 & (RF1_ATTR_MULTI))
			{
				/* Multi-hued attr */
				a = (byte)randint(BASE_COLORS - 1);

				/* Normal char */
				c = dc;
			}

			/* Metallic monster */
			else if (r_ptr->flags9 & (RF9_ATTR_METAL))
			{
				/* Flickering metallic attr - predominate base color */
				if (!rand_int(3)) a = get_color(da, ATTR_METAL, 1);
				else a = da;

				/* Normal char */
				c = dc;
			}

			/* Flavored monsters */
			else if (r_ptr->flags9 & (RF9_ATTR_INDEX))
			{
				/* Hack -- Fake flavored attr based on monster index */
				a = (m_idx % (BASE_COLORS - 1)) + 1;

				/* Normal char */
				c = dc;
			}

			/* Normal monster (not "clear" in any way) */
			else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)))
			{
				/* Use attr */
				a = da;

				/* Use char */
				c = dc;
			}

			/* Hack -- Bizarre grid under monster */
			else if ((a & 0x80) || (c & 0x80))
			{
				/* Use attr */
				a = da;

				/* Use char */
				c = dc;
			}

			/* Normal char, Clear attr, monster */
			else if (!(r_ptr->flags1 & (RF1_CHAR_CLEAR)))
			{
				/* Normal char */
				c = dc;
			}

			/* Normal attr, Clear char, monster */
			else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR)))
			{
				/* Normal attr */
				a = da;
			}
		}
	}

	/* Handle "player" */
	else if (m_idx < 0)
	{
		monster_race *r_ptr = &r_info[0];

		/* Hack - can't see invisible player due to see_invis */
		if ((p_ptr->timed[TMD_INVIS]) && !(p_ptr->timed[TMD_SEE_INVIS]) && !(p_ptr->cur_flags3 & (TR3_SEE_INVIS)))
		{
			/* Use underlying attribute/char */
		}

		/* Get the "player" attr */
		/*  DSV:  I've chosen the following sequence of colors to indicate
				the player's current HP.  There are colors are left over, but I
				left them in this comment for easy reference, in the likely case
				that I decide to change the order of color changes.

			TERM_WHITE		90-100% of HP remaining
			TERM_YELLOW		70- 89% of HP remaining
			TERM_ORANGE		50- 69% of HP remaining
			TERM_L_RED		30- 49% of HP remaining
			TERM_RED		 0- 29% of HP remaining


			TERM_SLATE		_% of HP remaining
			TERM_UMBER		_% of HP remaining
			TERM_L_UMBER	_% of HP remaining
			TERM_BLUE		-% of HP remaining
			TERM_L_BLUE		-% of HP remaining
			TERM_GREEN		-% of HP remaining
			TERM_L_GREEN	-% of HP remaining
			TERM_DARK		-% of HP remaining
			TERM_L_DARK		-% of HP remaining
			TERM_L_WHITE	-% of HP remaining
			TERM_VIOLET		-% of HP remaining
		*/

		else if (arg_graphics == GRAPHICS_NONE)
		{
			/* Invisible players are 'clear' */
			if (p_ptr->timed[TMD_IMAGE])
			{
				/* Use underlying attr */
			}

			switch(p_ptr->chp * 10 / p_ptr->mhp)
				{
				case 10:
				case  9:	a = TERM_WHITE  ;	break;
				case  8:
				case  7:	a = TERM_YELLOW ;	break;
				case  6:
				case  5:	a = TERM_ORANGE ;	break;
				case  4:
				case  3:	a = TERM_L_RED  ;	break;
				case  2:
				case  1:
				case  0:	a = TERM_RED    ;	break;
				default:	a = TERM_WHITE  ;	break;
				}
		}

		else a = r_ptr->x_attr;

		/* Get the "player" char */
		c = r_ptr->x_char;
	}

#ifdef MAP_INFO_MULTIPLE_PLAYERS
	/* Players */
	else if (m_idx < 0)
#else /* MAP_INFO_MULTIPLE_PLAYERS */
	/* Handle "player" */
	else if (m_idx < 0)
#endif /* MAP_INFO_MULTIPLE_PLAYERS */
	{
		monster_race *r_ptr = &r_info[0];

		/* Get the "player" attr */
		a = r_ptr->x_attr;

		/* Get the "player" char */
		c = r_ptr->x_char;
	}

	/* Result */
	(*ap) = a;
	(*cp) = c;

	/* Use the complex RNG again */
	Rand_quick = FALSE;
}



/*
 * Move the cursor to a given map location.
 *
 * The main screen will always be at least 24x80 in size.
 */
void move_cursor_relative(int y, int x)
{
	unsigned ky, kx;
	unsigned vy, vx;

	/* Location relative to panel */
	ky = (unsigned)(y - p_ptr->wy);

	/* Verify location */
	if (ky >= (unsigned)(SCREEN_HGT)) return;

	/* Location relative to panel */
	kx = (unsigned)(x - p_ptr->wx);

	/* Verify location */
	if (kx >= (unsigned)(SCREEN_WID)) return;

	/* Location in window */
	vy = ky + ROW_MAP;

	/* Location in window */
	vx = kx + COL_MAP;

	if (use_trptile)
	{
		vx += (use_bigtile ? 5 : 2) * kx;
		vy += 2 * ky;
	}
	else if (use_dbltile)
	{
		vx += (use_bigtile ? 3 : 1) * kx;
		vy += ky;
	}
	else if (use_bigtile) vx += kx;

	/* Go there */
	Term_gotoxy(vx, vy);
}


void big_queue_char(int x, int y, byte a, char c, byte a1, char c1)
{
	/* Avoid warning */
	(void)c;

	/* Paranoia */
	if (use_bigtile || use_dbltile || use_trptile)
	{
		/* Mega-Hack : Queue dummy char */
		if (a & 0x80)
			Term_queue_char(x + 1, y, 255, -1, 0, 0);
		else
			Term_queue_char(x + 1, y, TERM_WHITE, ' ', a1, c1);

		/* Mega-Hack : Queue more dummy chars */
		if (use_dbltile || use_trptile)
		{
			if (a & 0x80)
			{
				if (use_bigtile || use_trptile) Term_queue_char(x + 2, y, 255, -1, 0, 0);
				if (use_bigtile) Term_queue_char(x + 3, y, 255, -1, 0, 0);
				if (use_bigtile && use_trptile)
				{
					Term_queue_char(x + 4, y, 255, -1, 0, 0);
					Term_queue_char(x + 5, y, 255, -1, 0, 0);
				}

				Term_queue_char(x , y + 1, 255, -1, 0, 0);
				Term_queue_char(x + 1, y + 1, 255, -1, 0, 0);

				if (use_bigtile || use_trptile) Term_queue_char(x + 2, y + 1, 255, -1, 0, 0);
				if (use_bigtile) Term_queue_char(x + 3, y + 1, 255, -1, 0, 0);
				if (use_bigtile && use_trptile)
				{
					Term_queue_char(x + 4, y + 1, 255, -1, 0, 0);
					Term_queue_char(x + 5, y + 1, 255, -1, 0, 0);
				}

				if (use_trptile)
				{
					Term_queue_char(x , y + 2, 255, -1, 0, 0);
					Term_queue_char(x + 1, y + 2, 255, -1, 0, 0);
					Term_queue_char(x + 2, y + 2, 255, -1, 0, 0);

					if (use_bigtile)
					{
						Term_queue_char(x + 3, y + 2, 255, -1, 0, 0);
						Term_queue_char(x + 4, y + 2, 255, -1, 0, 0);
						Term_queue_char(x + 5, y + 2, 255, -1, 0, 0);
					}
				}
			}
			else
			{
				if (use_bigtile || use_trptile) Term_queue_char(x + 2, y, TERM_WHITE, ' ', a1, c1);
				if (use_bigtile) Term_queue_char(x + 3, y, TERM_WHITE, ' ', a1, c1);
				if (use_bigtile && use_trptile)
				{
					Term_queue_char(x + 4, y, TERM_WHITE, ' ', a1, c1);
					Term_queue_char(x + 5, y, TERM_WHITE, ' ', a1, c1);
				}

				Term_queue_char(x , y + 1, TERM_WHITE, ' ', a1, c1);
				Term_queue_char(x + 1, y + 1, TERM_WHITE, ' ', a1, c1);

				if (use_bigtile || use_trptile) Term_queue_char(x + 2, y + 1, TERM_WHITE, ' ', a1, c1);
				if (use_bigtile) Term_queue_char(x + 3, y + 1, TERM_WHITE, ' ', a1, c1);
				if (use_bigtile && use_trptile)
				{
					Term_queue_char(x + 4, y + 1, TERM_WHITE, ' ', a1, c1);
					Term_queue_char(x + 5, y + 1, TERM_WHITE, ' ', a1, c1);
				}

				if (use_trptile)
				{
					Term_queue_char(x , y + 2, TERM_WHITE, ' ', a1, c1);
					Term_queue_char(x + 1, y + 2, TERM_WHITE, ' ', a1, c1);
					Term_queue_char(x + 2, y + 2, TERM_WHITE, ' ', a1, c1);

					if (use_bigtile)
					{
						Term_queue_char(x + 3, y + 2, TERM_WHITE, ' ', a1, c1);
						Term_queue_char(x + 4, y + 2, TERM_WHITE, ' ', a1, c1);
						Term_queue_char(x + 5, y + 2, TERM_WHITE, ' ', a1, c1);
					}
				}
			}
		}
	}
}

void big_putch(int x, int y, byte a, char c)
{
	/* Avoid warning */
	(void)c;

	/* Paranoia */
	if (use_bigtile || use_dbltile)
	{
		/* Mega-Hack : Queue dummy char */
		if (a & 0x80)
			Term_putch(x + 1, y, 255, -1);
		else
			Term_putch(x + 1, y, TERM_WHITE, ' ');

		/* Mega-Hack : Queue more dummy chars */
		if (use_dbltile || use_trptile)
		{
			if (a & 0x80)
			{
				if (use_bigtile || use_trptile) Term_putch(x + 2, y, 255, -1);
				if (use_bigtile) Term_putch(x + 3, y, 255, -1);
				if (use_bigtile && use_trptile)
				{
					Term_putch(x + 4, y, 255, -1);
					Term_putch(x + 5, y, 255, -1);
				}

				Term_putch(x , y + 1, 255, -1);
				Term_putch(x + 1, y + 1, 255, -1);

				if (use_bigtile || use_trptile) Term_putch(x + 2, y + 1, 255, -1);
				if (use_bigtile) Term_putch(x + 3, y + 1, 255, -1);
				if (use_bigtile && use_trptile)
				{
					Term_putch(x + 4, y + 1, 255, -1);
					Term_putch(x + 5, y + 1, 255, -1);
				}

				if (use_trptile)
				{
					Term_putch(x , y + 2, 255, -1);
					Term_putch(x + 1, y + 2, 255, -1);
					Term_putch(x + 2, y + 2, 255, -1);

					if (use_bigtile)
					{
						Term_putch(x + 3, y + 2, 255, -1);
						Term_putch(x + 4, y + 2, 255, -1);
						Term_putch(x + 5, y + 2, 255, -1);
					}
				}
			}
			else
			{
				if (use_bigtile || use_trptile) Term_putch(x + 2, y, TERM_WHITE, ' ');
				if (use_bigtile) Term_putch(x + 3, y, TERM_WHITE, ' ');
				if (use_bigtile && use_trptile)
				{
					Term_putch(x + 4, y, TERM_WHITE, ' ');
					Term_putch(x + 5, y, TERM_WHITE, ' ');
				}

				Term_putch(x , y + 1, TERM_WHITE, ' ');
				Term_putch(x + 1, y + 1, TERM_WHITE, ' ');

				if (use_bigtile || use_trptile) Term_putch(x + 2, y + 1, TERM_WHITE, ' ');
				if (use_bigtile) Term_putch(x + 3, y + 1, TERM_WHITE, ' ');
				if (use_bigtile && use_trptile)
				{
					Term_putch(x + 4, y + 1, TERM_WHITE, ' ');
					Term_putch(x + 5, y + 1, TERM_WHITE, ' ');
				}

				if (use_trptile)
				{
					Term_putch(x , y + 2, TERM_WHITE, ' ');
					Term_putch(x + 1, y + 2, TERM_WHITE, ' ');
					Term_putch(x + 2, y + 2, TERM_WHITE, ' ');

					if (use_bigtile)
					{
						Term_putch(x + 3, y + 2, TERM_WHITE, ' ');
						Term_putch(x + 4, y + 2, TERM_WHITE, ' ');
						Term_putch(x + 5, y + 2, TERM_WHITE, ' ');
					}
				}
			}
		}
	}
}


/*
 * Display an attr/char pair at the given map location
 *
 * Note the inline use of "panel_contains()" for efficiency.
 *
 * Note the use of "Term_queue_char()" for efficiency.
 *
 * The main screen will always be at least 24x80 in size.
 */
void print_rel(char c, byte a, int y, int x)
{
	unsigned ky, kx;
	unsigned vy, vx;

	/* Location relative to panel */
	ky = (unsigned)(y - p_ptr->wy);

	/* Verify location */
	if (ky >= (unsigned)(SCREEN_HGT)) return;

	/* Location relative to panel */
	kx = (unsigned)(x - p_ptr->wx);

	/* Verify location */
	if (kx >= (unsigned)(SCREEN_WID)) return;

	/* Location in window */
	vy = ky + ROW_MAP;

	/* Location in window */
	vx = kx + COL_MAP;

	if (use_trptile)
	{
		vx += (use_bigtile ? 5 : 2) * kx;
		vy += 2 * ky;
	}
	else if (use_dbltile)
	{
		vx += (use_bigtile ? 3 : 1) * kx;
		vy += ky;
	}
	else if (use_bigtile) vx += kx;

	/* Hack -- Queue it */
	Term_queue_char(vx, vy, a, c, 0, 0);

	if (use_bigtile || use_dbltile || use_trptile)
	{
		/* Mega-Hack : Queue dummy char */
		big_queue_char(vx, vy, a, c, 0, 0);
	}

	return;
}




/*
 * Memorize interesting viewable object/features in the given grid
 *
 * This function should only be called on "legal" grids.
 *
 * This function will memorize the object and/or feature in the given grid,
 * if they are (1) see-able and (2) interesting.  Note that all objects are
 * interesting, all terrain features except floors (and invisible traps) are
 * interesting, and floors (and invisible traps) are interesting sometimes
 * (depending on various options involving the illumination of floor grids).
 *
 * The automatic memorization of all objects and non-floor terrain features
 * as soon as they are displayed allows incredible amounts of optimization
 * in various places, especially "map_info()" and this function itself.
 *
 * Note that the memorization of objects is completely separate from the
 * memorization of terrain features, preventing annoying floor memorization
 * when a detected object is picked up from a dark floor, and object
 * memorization when an object is dropped into a floor grid which is
 * memorized but out-of-sight.
 *
 * This function should be called every time the "memorization" of a grid
 * (or the object in a grid) is called into question, such as when an object
 * is created in a grid, when a terrain feature "changes" from "floor" to
 * "non-floor", and when any grid becomes "see-able" for any reason.
 *
 * This function is called primarily from the "update_view()" function, for
 * each grid which becomes newly "see-able".
 */
void note_spot(int y, int x)
{
	s16b feat;
	byte cinfo, pinfo;

	s16b this_o_idx, next_o_idx = 0;


	/* Get cave info */
	cinfo = cave_info[y][x];

	/* Get player info */
	pinfo = play_info[y][x];

	/* Get cave feat */
	feat = cave_feat[y][x];

	/* Require "seen" flag */
	if (!(pinfo & (PLAY_SEEN))) return;

	/* Mark it */
	if (!disturb_detect) play_info[y][x] |= (PLAY_SAFE);

	/* Hack -- memorize objects */
	/* ANDY -- Only memorise objects if they are not hidden by the feature */
	if (!(f_info[feat].flags2 & (FF2_HIDE_ITEM)))
	{
		for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr = &o_list[this_o_idx];
			object_kind *k_ptr = &k_info[o_ptr->k_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Ignore 'store' objects */
			if (o_ptr->ident & (IDENT_STORE)) continue;

			/* Memorize objects */
			if (!auto_pickup_ignore(o_ptr)) o_ptr->ident |= (IDENT_MARKED);

			/* Class learning */
			if (k_ptr->aware & (AWARE_CLASS))
			{
				/* Learn flavor */
				object_aware(o_ptr, TRUE);
			}

			/* Mark objects seen */
			object_aware_tips(o_ptr, TRUE);
		}
	}

	/* Hack -- memorize grids */
	if (!(pinfo & (PLAY_MARK)))
	{
		/* Memorize some "boring" grids */
		if (!(f_info[feat].flags1 & (FF1_REMEMBER)))
		{
			/* Option -- memorize certain floors */
			if (((cinfo & (CAVE_GLOW)) && view_perma_grids) ||
			    view_torch_grids)
			{
				/* Memorize */
				play_info[y][x] |= (PLAY_MARK);
			}
		}

		/* Memorize all "interesting" grids */
		else
		{
			/* Memorize */
			play_info[y][x] |= (PLAY_MARK);
		}

	}
}


/*
 * Redraw (on the screen) a given map location
 *
 * This function should only be called on "legal" grids.
 *
 * Note the inline use of "print_rel()" for efficiency.
 *
 * The main screen will always be at least 24x80 in size.
 */
void lite_spot(int y, int x)
{
	byte a;
	char c;

	byte ta;
	char tc;

	unsigned ky, kx;
	unsigned vy, vx;

	/* Hack -- complete redraw */
	if (arg_graphics == GRAPHICS_DAVID_GERVAIS_ISO)
	{
		p_ptr->redraw |= (PR_MAP);
		return;
	}

	/* Location relative to panel */
	ky = (unsigned)(y - p_ptr->wy);

	/* Verify location */
	if (ky >= (unsigned)(SCREEN_HGT)) return;

	/* Location relative to panel */
	kx = (unsigned)(x - p_ptr->wx);

	/* Verify location */
	if (kx >= (unsigned)(SCREEN_WID)) return;

	/* Location in window */
	vy = ky + ROW_MAP;

	/* Location in window */
	vx = kx + COL_MAP;

	if (use_trptile)
	{
		vx += (use_bigtile ? 5 : 2) * kx;
		vy += 2 * ky;
	}
	else if (use_dbltile)
	{
		vx += (use_bigtile ? 3 : 1) * kx;
		vy += ky;
	}
	else if (use_bigtile) vx += kx;

	/* Hack -- redraw the grid */
	map_info(y, x, &a, &c, &ta, &tc);

	/* Hack -- Queue it */
	Term_queue_char(vx, vy, a, c, ta, tc);

	if (use_bigtile || use_dbltile || use_trptile)
	{
		big_queue_char(vx, vy, a, c, TERM_WHITE, ' ');
	}

	return;
}


/*
 * Redraw (on the screen) the current map panel
 *
 * Note the inline use of "lite_spot()" for efficiency.
 *
 * The main screen will always be at least 24x80 in size.
 */
void prt_map(void)
{
	byte a;
	char c;
	byte ta;
	char tc;

	int y, x;
	int vy, vx;
	int ty, tx;

	/* Assume screen */
	ty = p_ptr->wy + SCREEN_HGT;
	tx = p_ptr->wx + SCREEN_WID;

	/* Dump the map */
	for (y = p_ptr->wy, vy = ROW_MAP; y < ty; vy++, y++)
	{
		for (x = p_ptr->wx, vx = COL_MAP; x < tx; vx++, x++)
		{
			/* Check bounds */
			if (!in_bounds(y, x)) continue;

			/* Determine what is there */
			map_info(y, x, &a, &c, &ta, &tc);

			/* Hack -- Queue it */
			Term_queue_char(vx, vy, a, c, ta, tc);

			if (use_bigtile || use_dbltile || use_trptile)
			{
				big_queue_char(vx, vy, a, c, TERM_WHITE, ' ');

				if (use_trptile)
				{
					vx += (use_bigtile ? 5 : 2);
				}
				else
					vx+= ((use_dbltile && use_bigtile) ? 3 : 1);
			}
		}

		if (use_trptile)
			vy++;

		if (use_dbltile || use_trptile)
			vy++;
	}
}


/*
 * Display item list (below the map)
 */
void prt_item_list(void)
{
	int i, j;

	int x = 1;
	int y = Term->hgt - (use_trptile ? 3 : (use_dbltile ? 2 : 1));

	byte a, ta;
	char c, tc;

	byte na = f_info[FEAT_NONE].x_attr;

	char nc = f_info[FEAT_NONE].x_char;

	if (show_itemlist)
	{
		for (i = 1; i < SHOWN_TOTAL; i++)
		{
			for (j = 0; j < INVEN_TOTAL; j++)
			{
				if (!inventory[j].k_idx) continue;

				if (inventory[j].show_idx == i) break;
			}

			/* Invalid item */
			if (j >= INVEN_TOTAL)
			{
				/* Nothing */
				ta = a = na;
				tc = c = nc;
			}
			else
			{
				a = k_info[inventory[j].k_idx].x_attr;
				c = k_info[inventory[j].k_idx].x_char;

				if (j < INVEN_WIELD)
				{
					ta = na;
					tc = nc;
				}
				else
				{
					ta = f_info[FEAT_FLOOR].x_attr;
					tc = f_info[FEAT_FLOOR].x_char;
				}
			}

			/* Hack -- Queue it */
			Term_queue_char(x, y, a, c, ta, tc);

			if (use_bigtile || use_dbltile || use_trptile)
			{
				big_queue_char(x, y, a, c, TERM_WHITE, ' ');
			}

			/* Advance the column */
			if (use_trptile)
			{
				x += (use_bigtile ? 6 : 3);

			}
			else if (use_dbltile)
			{
				x += (use_bigtile ? 4 : 2);
			}
			else if (use_bigtile) x += 2;
			else x++;
		}
	}
}

/*
 * Hack -- a priority function (rewritten by ANDY)
 */
static byte priority(byte a, char c)
{
	int i, p1;

	feature_type *f_ptr;

	/* Scan the table */
	for (i = 0; i < z_info->f_max; i++)
	{
		/* Priority level */
		p1 = f_info[i].priority;

		/* Get the feature */
		f_ptr = &f_info[i];

		/* Check character and attribute, accept matches */
		if ((f_ptr->x_char == c) && (f_ptr->x_attr == a)) return (p1);
	}

	/* Default */
	return (20);
}


/*
 * Maximum size of map.
 */
#define MAP_HGT (DUNGEON_HGT / 3)
#define MAP_WID (DUNGEON_WID / 3)


/*
 * Display a "small-scale" map of the dungeon in the active Term.
 *
 * Note that this function must "disable" the special lighting effects so
 * that the "priority" function will work.
 *
 * Note the use of a specialized "priority" function to allow this function
 * to work with any graphic attr/char mappings, and the attempts to optimize
 * this function where possible.
 *
 * If "cy" and "cx" are not NULL, then returns the screen location at which
 * the player was displayed, so the cursor can be moved to that location,
 * and restricts the horizontal map size to SCREEN_WID.  Otherwise, nothing
 * is returned (obviously), and no restrictions are enforced.
 */
void display_map(int *cy, int *cx)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int map_hgt, map_wid;
	int dungeon_hgt, dungeon_wid;
	int row, col;

	int x, y;

	byte ta;
	char tc;

	byte tp;

	/* Large array on the stack */
	byte mp[DUNGEON_HGT][DUNGEON_WID];

	bool old_view_special_lite;
	bool old_view_granite_lite;

	monster_race *r_ptr = &r_info[0];

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	dungeon_zone *zone=&t_ptr->zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Desired map height */
	map_hgt = Term->hgt - 2;
	map_wid = Term->wid - 2;

	dungeon_hgt = (level_flag & (LF1_TOWN)) != 0 ? TOWN_HGT : DUNGEON_HGT;
	dungeon_wid = (level_flag & (LF1_TOWN)) != 0 ? TOWN_WID : DUNGEON_WID;

	/* Prevent accidents */
	if (map_hgt > dungeon_hgt) map_hgt = dungeon_hgt;
	if (map_wid > dungeon_wid) map_wid = dungeon_wid;

	/* Prevent accidents */
	if ((map_wid < 1) || (map_hgt < 1)) return;


	/* Save lighting effects */
	old_view_special_lite = view_special_lite;
	old_view_granite_lite = view_granite_lite;

	/* Disable lighting effects */
	view_special_lite = FALSE;
	view_granite_lite = FALSE;

	/* Change the way the grids are displayed */
	if (Term->notice_grid) Term_xtra(TERM_XTRA_GRIDS, 2);

	/* Nothing here */
	ta = TERM_WHITE;
	tc = ' ';

	/* Clear the priorities */
	for (y = 0; y < map_hgt; ++y)
	{
		for (x = 0; x < map_wid; ++x)
		{
			/* No priority */
			mp[y][x] = 0;
		}
	}

	/* Clear the screen (but don't force a redraw) */
	clear_from(0);

	/* Corners */
	x = map_wid + 1;
	y = map_hgt + 1;

	/* Draw the corners */
	Term_putch(0, 0, ta, '+');
	Term_putch(x, 0, ta, '+');
	Term_putch(0, y, ta, '+');
	Term_putch(x, y, ta, '+');

	/* Draw the horizontal edges */
	for (x = 1; x <= map_wid; x++)
	{
		Term_putch(x, 0, ta, '-');
		Term_putch(x, y, ta, '-');
	}

	/* Draw the vertical edges */
	for (y = 1; y <= map_hgt; y++)
	{
		Term_putch(0, y, ta, '|');
		Term_putch(x, y, ta, '|');
	}


	/* Analyze the actual map */
	for (y = 0; y < dungeon_hgt; y++)
	{
		for (x = 0; x < dungeon_wid; x++)
		{
			row = (y * map_hgt / dungeon_hgt);
			col = (x * map_wid / dungeon_wid);

			if (use_trptile)
			{
				col = col - (col % (use_bigtile ? 6 : 3));
				row = row - (row % 3);
			}
			else if (use_dbltile)
			{
				col = col & ~(use_bigtile ? 3 : 1);
				row = row & ~1;
			}
			else if (use_bigtile)
				col = col & ~1;

			/* Get the attr/char at that map location */
			map_info(y, x, &ta, &tc, &ta, &tc);

			/* Get the priority of that attr/char */
			tp = priority(ta, tc);

			/* Save "best" */
			if (mp[row][col] < tp)
			{
				/* Add the character */
				Term_putch(col + 1, row + 1, ta, tc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(col + 1, row + 1, ta, tc);
				}

				/* Save priority */
				mp[row][col] = tp;
			}
		}
	}


	/* Player location */
	row = (py * map_hgt / dungeon_hgt);
	col = (px * map_wid / dungeon_wid);

	if (use_trptile)
	{
		col = col - (col % (use_bigtile ? 6 : 3));
		row = row - (row % 3);
	}
	else if (use_dbltile)
	{
		col = col & ~(use_bigtile ? 3 : 1);
		row = row & ~1;
	}
	else if (use_bigtile)
		col = col & ~1;

	/*** Make sure the player is visible ***/

	/* Get the "player" attr */
	ta = r_ptr->x_attr;

	/* Get the "player" char */
	tc = r_ptr->x_char;

	/* Draw the player */
	Term_putch(col + 1, row + 1, ta, tc);

	if (use_bigtile || use_dbltile)
	{
		big_putch(col + 1, row + 1, ta, tc);
	}

	/* Return player location */
	if (cy != NULL) (*cy) = row + 1;
	if (cx != NULL) (*cx) = col + 1;


	/* Restore lighting effects */
	view_special_lite = old_view_special_lite;
	view_granite_lite = old_view_granite_lite;
}


/*
 * Display a "small-scale" map of the dungeon.
 *
 * Note that the "player" is always displayed on the map.
 */
void do_cmd_view_map(void)
{
	int cy, cx;
	cptr prompt = "Hit any key to continue";

	/* Display the 'overworld' map if we are in a town */
	if (level_flag & (LF1_TOWN))
	{
		screen_save();
		(void)show_file("memap.txt", NULL, 0, 0);
		screen_load();
		
		return;
	}
	
	/* Save screen */
	screen_save();

	/* Note */
	prt("Please wait...", 0, 0);

	/* Flush */
	Term_fresh();

	/* Clear the screen */
	Term_clear();

	/* Display the map */
	display_map(&cy, &cx);

	/* Show the prompt */
	put_str(prompt, Term->hgt - 1, Term->wid / 2 - strlen(prompt) / 2);

	/* Hilite the player */
	Term_gotoxy(cx, cy);

	/* Get any key */
	(void)anykey();

	/* Load screen */
	screen_load();
}



/*
 * Some comments on the dungeon related data structures and functions...
 *
 * Angband is primarily a dungeon exploration game, and it should come as
 * no surprise that the internal representation of the dungeon has evolved
 * over time in much the same way as the game itself, to provide semantic
 * changes to the game itself, to make the code simpler to understand, and
 * to make the executable itself faster or more efficient in various ways.
 *
 * There are a variety of dungeon related data structures, and associated
 * functions, which store information about the dungeon, and provide methods
 * by which this information can be accessed or modified.
 *
 * Some of this information applies to the dungeon as a whole, such as the
 * list of unique monsters which are still alive.  Some of this information
 * only applies to the current dungeon level, such as the current depth, or
 * the list of monsters currently inhabiting the level.  And some of the
 * information only applies to a single grid of the current dungeon level,
 * such as whether the grid is illuminated, or whether the grid contains a
 * monster, or whether the grid can be seen by the player.  If Angband was
 * to be turned into a multi-player game, some of the information currently
 * associated with the dungeon should really be associated with the player,
 * such as whether a given grid is viewable by a given player.
 *
 * One of the major bottlenecks in ancient versions of Angband was in the
 * calculation of "line of sight" from the player to various grids, such
 * as those containing monsters, using the relatively expensive "los()"
 * function.  This was such a nasty bottleneck that a lot of silly things
 * were done to reduce the dependancy on "line of sight", for example, you
 * could not "see" any grids in a lit room until you actually entered the
 * room, at which point every grid in the room became "illuminated" and
 * all of the grids in the room were "memorized" forever.  Other major
 * bottlenecks involved the determination of whether a grid was lit by the
 * player's torch, and whether a grid blocked the player's line of sight.
 * These bottlenecks led to the development of special new functions to
 * optimize issues involved with "line of sight" and "torch lit grids".
 * These optimizations led to entirely new additions to the game, such as
 * the ability to display the player's entire field of view using different
 * colors than were used for the "memorized" portions of the dungeon, and
 * the ability to memorize dark floor grids, but to indicate by the way in
 * which they are displayed that they are not actually illuminated.  And
 * of course many of them simply made the game itself faster or more fun.
 * Also, over time, the definition of "line of sight" has been relaxed to
 * allow the player to see a wider "field of view", which is slightly more
 * realistic, and only slightly more expensive to maintain.
 *
 * Currently, a lot of the information about the dungeon is stored in ways
 * that make it very efficient to access or modify the information, while
 * still attempting to be relatively conservative about memory usage, even
 * if this means that some information is stored in multiple places, or in
 * ways which require the use of special code idioms.  For example, each
 * monster record in the monster array contains the location of the monster,
 * and each cave grid has an index into the monster array, or a zero if no
 * monster is in the grid.  This allows the monster code to efficiently see
 * where the monster is located, while allowing the dungeon code to quickly
 * determine not only if a monster is present in a given grid, but also to
 * find out which monster.  The extra space used to store the information
 * twice is inconsequential compared to the speed increase.
 *
 * Some of the information about the dungeon is used by functions which can
 * constitute the "critical efficiency path" of the game itself, and so the
 * way in which they are stored and accessed has been optimized in order to
 * optimize the game itself.  For example, the "update_view()" function was
 * originally created to speed up the game itself (when the player was not
 * running), but then it took on extra responsibility as the provider of the
 * new "special effects lighting code", and became one of the most important
 * bottlenecks when the player was running.  So many rounds of optimization
 * were performed on both the function itself, and the data structures which
 * it uses, resulting eventually in a function which not only made the game
 * faster than before, but which was responsible for even more calculations
 * (including the determination of which grids are "viewable" by the player,
 * which grids are illuminated by the player's torch, and which grids can be
 * "seen" in some way by the player), as well as for providing the guts of
 * the special effects lighting code, and for the efficient redisplay of any
 * grids whose visual representation may have changed.
 *
 * Several pieces of information about each cave grid are stored in various
 * two dimensional arrays, with one unit of information for each grid in the
 * dungeon.  Some of these arrays have been intentionally expanded by a small
 * factor to make the two dimensional array accesses faster by allowing the
 * use of shifting instead of multiplication.
 *
 * Several pieces of information about each cave grid are stored in the
 * "cave_info" array, which is a special two dimensional array of bytes,
 * one for each cave grid, each containing eight separate "flags" which
 * describe some property of the cave grid.  These flags can be checked and
 * modified extremely quickly, especially when special idioms are used to
 * force the compiler to keep a local register pointing to the base of the
 * array.  Special location offset macros can be used to minimize the number
 * of computations which must be performed at runtime.  Note that using a
 * byte for each flag set may be slightly more efficient than using a larger
 * unit, so if another flag (or two) is needed later, and it must be fast,
 * then the two existing flags which do not have to be fast should be moved
 * out into some other data structure and the new flags should take their
 * place.  This may require a few minor changes in the savefile code.
 *
 * The "CAVE_ROOM" flag is saved in the savefile and is used to determine
 * which grids are part of "rooms", and thus which grids are affected by
 * "illumination" spells.  This flag does not have to be very fast.
 *
 * The "CAVE_ICKY" flag is saved in the savefile and is used to determine
 * which grids are part of "vaults", and thus which grids cannot serve as
 * the destinations of player teleportation.  This flag does not have to
 * be very fast.
 *
 * The "CAVE_MARK" flag is saved in the savefile and is used to determine
 * which grids have been "memorized" by the player.  This flag is used by
 * the "map_info()" function to determine if a grid should be displayed.
 * This flag is used in a few other places to determine if the player can
 * "know" about a given grid.  This flag must be very fast.
 *
 * The "CAVE_GLOW" flag is saved in the savefile and is used to determine
 * which grids are "permanently illuminated".  This flag is used by the
 * "update_view()" function to help determine which viewable flags may
 * be "seen" by the player.  This flag is used by the "map_info" function
 * to determine if a grid is only lit by the player's torch.  This flag
 * has special semantics for wall grids (see "update_view()").  This flag
 * must be very fast.
 *
 * The "CAVE_WALL" flag is used to determine which grids block the player's
 * line of sight.  This flag is used by the "update_view()" function to
 * determine which grids block line of sight, and to help determine which
 * grids can be "seen" by the player.  This flag must be very fast.
 *
 * The "CAVE_VIEW" flag is used to determine which grids are currently in
 * line of sight of the player.  This flag is set by (and used by) the
 * "update_view()" function.  This flag is used by any code which needs to
 * know if the player can "view" a given grid.  This flag is used by the
 * "map_info()" function for some optional special lighting effects.  The
 * "player_has_los_bold()" macro wraps an abstraction around this flag, but
 * certain code idioms are much more efficient.  This flag is used to check
 * if a modification to a terrain feature might affect the player's field of
 * view.  This flag is used to see if certain monsters are "visible" to the
 * player.  This flag is used to allow any monster in the player's field of
 * view to "sense" the presence of the player.  This flag must be very fast.
 *
 * The "CAVE_SEEN" flag is used to determine which grids are currently in
 * line of sight of the player and also illuminated in some way.  This flag
 * is set by the "update_view()" function, using computations based on the
 * "CAVE_VIEW" and "CAVE_WALL" and "CAVE_GLOW" flags of various grids.  This
 * flag is used by any code which needs to know if the player can "see" a
 * given grid.  This flag is used by the "map_info()" function both to see
 * if a given "boring" grid can be seen by the player, and for some optional
 * special lighting effects.  The "player_can_see_bold()" macro wraps an
 * abstraction around this flag, but certain code idioms are much more
 * efficient.  This flag is used to see if certain monsters are "visible" to
 * the player.  This flag is never set for a grid unless "CAVE_VIEW" is also
 * set for the grid.  Whenever the "CAVE_WALL" or "CAVE_GLOW" flag changes
 * for a grid which has the "CAVE_VIEW" flag set, the "CAVE_SEEN" flag must
 * be recalculated.  The simplest way to do this is to call "forget_view()"
 * and "update_view()" whenever the "CAVE_WALL" or "CAVE_GLOW" flags change
 * for a grid which has "CAVE_VIEW" set.  This flag must be very fast.
 *
 * The "CAVE_TEMP" flag is used for a variety of temporary purposes.  This
 * flag is used to determine if the "CAVE_SEEN" flag for a grid has changed
 * during the "update_view()" function.  This flag is used to "spread" light
 * or darkness through a room.  This flag is used by the "monster flow code".
 * This flag must always be cleared by any code which sets it, often, this
 * can be optimized by the use of the special "temp_g", "temp_y", "temp_x"
 * arrays (and the special "temp_n" global).  This flag must be very fast.
 *
 * Note that the "CAVE_MARK" flag is used for many reasons, some of which
 * are strictly for optimization purposes.  The "CAVE_MARK" flag means that
 * even if the player cannot "see" the grid, he "knows" about the terrain in
 * that grid.  This is used to "memorize" grids when they are first "seen" by
 * the player, and to allow certain grids to be "detected" by certain magic.
 * Note that most grids are always memorized when they are first "seen", but
 * "boring" grids (floor grids) are only memorized if the "view_torch_grids"
 * option is set, or if the "view_perma_grids" option is set, and the grid
 * in question has the "CAVE_GLOW" flag set.
 *
 * Objects are "memorized" in a different way, using a special "marked" flag
 * on the object itself, which is set when an object is observed or detected.
 * This allows objects to be "memorized" independant of the terrain features.
 *
 * The "update_view()" function is an extremely important function.  It is
 * called only when the player moves, significant terrain changes, or the
 * player's blindness or torch radius changes.  Note that when the player
 * is resting, or performing any repeated actions (like digging, disarming,
 * farming, etc), there is no need to call the "update_view()" function, so
 * even if it was not very efficient, this would really only matter when the
 * player was "running" through the dungeon.  It sets the "CAVE_VIEW" flag
 * on every cave grid in the player's field of view, and maintains an array
 * of all such grids in the global "view_g" array.  It also checks the torch
 * radius of the player, and sets the "CAVE_SEEN" flag for every grid which
 * is in the "field of view" of the player and which is also "illuminated",
 * either by the players torch (if any) or by any permanent light source.
 * It could use and help maintain information about multiple light sources,
 * which would be helpful in a multi-player version of Angband.
 *
 * The "update_view()" function maintains the special "view_g" array, which
 * contains exactly those grids which have the "CAVE_VIEW" flag set.  This
 * array is used by "update_view()" to (only) memorize grids which become
 * newly "seen", and to (only) redraw grids whose "seen" value changes, which
 * allows the use of some interesting (and very efficient) "special lighting
 * effects".  In addition, this array could be used elsewhere to quickly scan
 * through all the grids which are in the player's field of view.
 *
 * Note that the "update_view()" function allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone
 * of floor appearing as the player gets closer to the door.  Also, by not
 * turning on the "memorize perma-lit grids" option, the player will only
 * "see" those floor grids which are actually in line of sight.  And best
 * of all, you can now activate the special lighting effects to indicate
 * which grids are actually in the player's field of view by using dimmer
 * colors for grids which are not in the player's field of view, and/or to
 * indicate which grids are illuminated only by the player's torch by using
 * the color yellow for those grids.
 *
 * The old "update_view()" algorithm uses the special "CAVE_EASY" flag as a
 * temporary internal flag to mark those grids which are not only in view,
 * but which are also "easily" in line of sight of the player.  This flag
 * is actually just the "CAVE_SEEN" flag, and the "update_view()" function
 * makes sure to clear it for all old "CAVE_SEEN" grids, and then use it in
 * the algorithm as "CAVE_EASY", and then clear it for all "CAVE_EASY" grids,
 * and then reset it as appropriate for all new "CAVE_SEEN" grids.  This is
 * kind of messy, but it works.  The old algorithm may disappear eventually.
 *
 * The new "update_view()" algorithm uses a faster and more mathematically
 * correct algorithm, assisted by a large machine generated static array, to
 * determine the "CAVE_VIEW" and "CAVE_SEEN" flags simultaneously.  See below.
 *
 * It seems as though slight modifications to the "update_view()" functions
 * would allow us to determine "reverse" line-of-sight as well as "normal"
 * line-of-sight", which would allow monsters to have a more "correct" way
 * to determine if they can "see" the player, since right now, they "cheat"
 * somewhat and assume that if the player has "line of sight" to them, then
 * they can "pretend" that they have "line of sight" to the player.  But if
 * such a change was attempted, the monsters would actually start to exhibit
 * some undesirable behavior, such as "freezing" near the entrances to long
 * hallways containing the player, and code would have to be added to make
 * the monsters move around even if the player was not detectable, and to
 * "remember" where the player was last seen, to avoid looking stupid.
 *
 * Note that the "CAVE_GLOW" flag means that a grid is permanently lit in
 * some way.  However, for the player to "see" the grid, as determined by
 * the "CAVE_SEEN" flag, the player must not be blind, the grid must have
 * the "CAVE_VIEW" flag set, and if the grid is a "wall" grid, and it is
 * not lit by the player's torch, then it must touch a grid which does not
 * have the "CAVE_WALL" flag set, but which does have both the "CAVE_GLOW"
 * and "CAVE_VIEW" flags set.  This last part about wall grids is induced
 * by the semantics of "CAVE_GLOW" as applied to wall grids, and checking
 * the technical requirements can be very expensive, especially since the
 * grid may be touching some "illegal" grids.  Luckily, it is more or less
 * correct to restrict the "touching" grids from the eight "possible" grids
 * to the (at most) three grids which are touching the grid, and which are
 * closer to the player than the grid itself, which eliminates more than
 * half of the work, including all of the potentially "illegal" grids, if
 * at most one of the three grids is a "diagonal" grid.  In addition, in
 * almost every situation, it is possible to ignore the "CAVE_VIEW" flag
 * on these three "touching" grids, for a variety of technical reasons.
 * Finally, note that in most situations, it is only necessary to check
 * a single "touching" grid, in fact, the grid which is strictly closest
 * to the player of all the touching grids, and in fact, it is normally
 * only necessary to check the "CAVE_GLOW" flag of that grid, again, for
 * various technical reasons.  However, one of the situations which does
 * not work with this last reduction is the very common one in which the
 * player approaches an illuminated room from a dark hallway, in which the
 * two wall grids which form the "entrance" to the room would not be marked
 * as "CAVE_SEEN", since of the three "touching" grids nearer to the player
 * than each wall grid, only the farthest of these grids is itself marked
 * "CAVE_GLOW".
 *
 *
 * Here are some pictures of the legal "light source" radius values, in
 * which the numbers indicate the "order" in which the grids could have
 * been calculated, if desired.  Note that the code will work with larger
 * radiuses, though currently yields such a radius, and the game would
 * become slower in some situations if it did.
 *
 *       Rad=0     Rad=1      Rad=2Rad=3
 *      No-Lite  Torch,etc   Lantern     Artifacts
 *
 *  333
 *     333 43334
 *  212       32123       3321233
 * @1@1       31@13       331@133
 *  212       32123       3321233
 *     333 43334
 *  333
 *
 *
 * Here is an illustration of the two different "update_view()" algorithms,
 * in which the grids marked "%" are pillars, and the grids marked "?" are
 * not in line of sight of the player.
 *
 *
 *    Sample situation
 *
 *  #####################
 *  ############.%.%.%.%#
 *  #...@..#####........#
 *  #............%.%.%.%#
 *  #......#####........#
 *  ############........#
 *  #####################
 *
 *
 *  New Algorithm     Old Algorithm
 *
 *      ########?????????????    ########?????????????
 *      #...@..#?????????????    #...@..#?????????????
 *      #...........?????????    #.........???????????
 *      #......#####.....????    #......####??????????
 *      ########?????????...#    ########?????????????
 *
 *      ########?????????????    ########?????????????
 *      #.@....#?????????????    #.@....#?????????????
 *      #............%???????    #...........?????????
 *      #......#####........?    #......#####?????????
 *      ########??????????..#    ########?????????????
 *
 *      ########?????????????    ########?????%???????
 *      #......#####........#    #......#####..???????
 *      #.@..........%???????    #.@..........%???????
 *      #......#####........#    #......#####..???????
 *      ########?????????????    ########?????????????
 *
 *      ########??????????..#    ########?????????????
 *      #......#####........?    #......#####?????????
 *      #............%???????    #...........?????????
 *      #.@....#?????????????    #.@....#?????????????
 *      ########?????????????    ########?????????????
 *
 *      ########?????????%???    ########?????????????
 *      #......#####.....????    #......####??????????
 *      #...........?????????    #.........???????????
 *      #...@..#?????????????    #...@..#?????????????
 *      ########?????????????    ########?????????????
 */




/*
 * Maximum number of grids in a single octant
 */
#define VINFO_MAX_GRIDS 161


/*
 * Maximum number of slopes in a single octant
 */
#define VINFO_MAX_SLOPES 126


/*
 * Mask of bits used in a single octant
 */
#define VINFO_BITS_3 0x3FFFFFFF
#define VINFO_BITS_2 0xFFFFFFFF
#define VINFO_BITS_1 0xFFFFFFFF
#define VINFO_BITS_0 0xFFFFFFFF


/*
 * Forward declare
 */
typedef struct vinfo_type vinfo_type;


/*
 * The 'vinfo_type' structure
 */
struct vinfo_type
{
	s16b grid[8];

	/* LOS slopes (up to 128) */
	u32b bits_3;
	u32b bits_2;
	u32b bits_1;
	u32b bits_0;

	/* Index of the first LOF slope */
	byte slope_fire_index1;

	/* Index of the (possible) second LOF slope */
	byte slope_fire_index2;

	vinfo_type *next_0;
	vinfo_type *next_1;

	byte y;
	byte x;
	byte d;
	byte r;
};



/*
 * The array of "vinfo" objects, initialized by "vinfo_init()"
 */
static vinfo_type vinfo[VINFO_MAX_GRIDS];




/*
 * Slope scale factor
 */
#define SCALE 100000L


/*
 * The actual slopes (for reference)
 */

/* Bit :     Slope   Grids */
/* --- :     -----   ----- */
/*   0 :      2439      21 */
/*   1 :      2564      21 */
/*   2 :      2702      21 */
/*   3 :      2857      21 */
/*   4 :      3030      21 */
/*   5 :      3225      21 */
/*   6 :      3448      21 */
/*   7 :      3703      21 */
/*   8 :      4000      21 */
/*   9 :      4347      21 */
/*  10 :      4761      21 */
/*  11 :      5263      21 */
/*  12 :      5882      21 */
/*  13 :      6666      21 */
/*  14 :      7317      22 */
/*  15 :      7692      20 */
/*  16 :      8108      21 */
/*  17 :      8571      21 */
/*  18 :      9090      20 */
/*  19 :      9677      21 */
/*  20 :     10344      21 */
/*  21 :     11111      20 */
/*  22 :     12000      21 */
/*  23 :     12820      22 */
/*  24 :     13043      22 */
/*  25 :     13513      22 */
/*  26 :     14285      20 */
/*  27 :     15151      22 */
/*  28 :     15789      22 */
/*  29 :     16129      22 */
/*  30 :     17241      22 */
/*  31 :     17647      22 */
/*  32 :     17948      23 */
/*  33 :     18518      22 */
/*  34 :     18918      22 */
/*  35 :     20000      19 */
/*  36 :     21212      22 */
/*  37 :     21739      22 */
/*  38 :     22580      22 */
/*  39 :     23076      22 */
/*  40 :     23809      22 */
/*  41 :     24137      22 */
/*  42 :     24324      23 */
/*  43 :     25714      23 */
/*  44 :     25925      23 */
/*  45 :     26315      23 */
/*  46 :     27272      22 */
/*  47 :     28000      23 */
/*  48 :     29032      23 */
/*  49 :     29411      23 */
/*  50 :     29729      24 */
/*  51 :     30434      23 */
/*  52 :     31034      23 */
/*  53 :     31428      23 */
/*  54 :     33333      18 */
/*  55 :     35483      23 */
/*  56 :     36000      23 */
/*  57 :     36842      23 */
/*  58 :     37142      24 */
/*  59 :     37931      24 */
/*  60 :     38461      24 */
/*  61 :     39130      24 */
/*  62 :     39393      24 */
/*  63 :     40740      24 */
/*  64 :     41176      24 */
/*  65 :     41935      24 */
/*  66 :     42857      23 */
/*  67 :     44000      24 */
/*  68 :     44827      24 */
/*  69 :     45454      23 */
/*  70 :     46666      24 */
/*  71 :     47368      24 */
/*  72 :     47826      24 */
/*  73 :     48148      24 */
/*  74 :     48387      24 */
/*  75 :     51515      25 */
/*  76 :     51724      25 */
/*  77 :     52000      25 */
/*  78 :     52380      25 */
/*  79 :     52941      25 */
/*  80 :     53846      25 */
/*  81 :     54838      25 */
/*  82 :     55555      24 */
/*  83 :     56521      25 */
/*  84 :     57575      26 */
/*  85 :     57894      25 */
/*  86 :     58620      25 */
/*  87 :     60000      23 */
/*  88 :     61290      25 */
/*  89 :     61904      25 */
/*  90 :     62962      25 */
/*  91 :     63636      25 */
/*  92 :     64705      25 */
/*  93 :     65217      25 */
/*  94 :     65517      25 */
/*  95 :     67741      26 */
/*  96 :     68000      26 */
/*  97 :     68421      26 */
/*  98 :     69230      26 */
/*  99 :     70370      26 */
/* 100 :     71428      25 */
/* 101 :     72413      26 */
/* 102 :     73333      26 */
/* 103 :     73913      26 */
/* 104 :     74193      27 */
/* 105 :     76000      26 */
/* 106 :     76470      26 */
/* 107 :     77777      25 */
/* 108 :     78947      26 */
/* 109 :     79310      26 */
/* 110 :     80952      26 */
/* 111 :     81818      26 */
/* 112 :     82608      26 */
/* 113 :     84000      26 */
/* 114 :     84615      26 */
/* 115 :     85185      26 */
/* 116 :     86206      27 */
/* 117 :     86666      27 */
/* 118 :     88235      27 */
/* 119 :     89473      27 */
/* 120 :     90476      27 */
/* 121 :     91304      27 */
/* 122 :     92000      27 */
/* 123 :     92592      27 */
/* 124 :     93103      28 */
/* 125 :    100000      13 */



/*
 * Forward declare
 */
typedef struct vinfo_hack vinfo_hack;


/*
 * Temporary data used by "vinfo_init()"
 *
 *	- Number of grids
 *
 *	- Number of slopes
 *
 *	- Slope values
 *
 *	- Slope range per grid
 */
struct vinfo_hack {

	int num_slopes;

	long slopes[VINFO_MAX_SLOPES];

	long slopes_min[MAX_SIGHT+1][MAX_SIGHT+1];
	long slopes_max[MAX_SIGHT+1][MAX_SIGHT+1];
};



/*
 * Sorting hook -- comp function -- array of long's (see below)
 *
 * We use "u" to point to an array of long integers.
 */
static bool ang_sort_comp_hook_longs(vptr u, vptr v, int a, int b)
{
	long *x = (long*)(u);

	/* Unused parameter */
	(void)v;

	return (x[a] <= x[b]);
}


/*
 * Sorting hook -- comp function -- array of long's (see below)
 *
 * We use "u" to point to an array of long integers.
 */
static void ang_sort_swap_hook_longs(vptr u, vptr v, int a, int b)
{
	long *x = (long*)(u);

	long temp;

	/* Unused parameter */
	(void)v;

	/* Swap */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;
}



/*
 * Save a slope
 */
static void vinfo_init_aux(vinfo_hack *hack, int y, int x, long m)
{
	int i;

	/* Handle "legal" slopes */
	if ((m > 0) && (m <= SCALE))
	{
		/* Look for that slope */
		for (i = 0; i < hack->num_slopes; i++)
		{
			if (hack->slopes[i] == m) break;
		}

		/* New slope */
		if (i == hack->num_slopes)
		{
			/* Paranoia */
			if (hack->num_slopes >= VINFO_MAX_SLOPES)
			{
				quit_fmt("Too many slopes (%d)!",
			 	VINFO_MAX_SLOPES);
			}

			/* Save the slope, and advance */
			hack->slopes[hack->num_slopes++] = m;
		}
	}

	/* Track slope range */
	if (hack->slopes_min[y][x] > m) hack->slopes_min[y][x] = m;
	if (hack->slopes_max[y][x] < m) hack->slopes_max[y][x] = m;
}



/*
 * Initialize the "vinfo" array
 *
 * Full Octagon (radius 20), Grids=1149
 *
 * Quadrant (south east), Grids=308, Slopes=251
 *
 * Octant (east then south), Grids=161, Slopes=126
 *
 * This function assumes that VINFO_MAX_GRIDS and VINFO_MAX_SLOPES
 * have the correct values, which can be derived by setting them to
 * a number which is too high, running this function, and using the
 * error messages to obtain the correct values.
 */
errr vinfo_init(void)
{
	int i, g;
	int y, x;

	long m;

	vinfo_hack *hack;

	int num_grids = 0;

	int queue_head = 0;
	int queue_tail = 0;
	vinfo_type *queue[VINFO_MAX_GRIDS*2];


	/* Make hack */
	hack = ZNEW(vinfo_hack);


	/* Analyze grids */
	for (y = 0; y <= MAX_SIGHT; ++y)
	{
		for (x = y; x <= MAX_SIGHT; ++x)
		{
			/* Skip grids which are out of sight range */
			if (distance(0, 0, y, x) > MAX_SIGHT) continue;

			/* Default slope range */
			hack->slopes_min[y][x] = 999999999;
			hack->slopes_max[y][x] = 0;

			/* Paranoia */
			if (num_grids >= VINFO_MAX_GRIDS)
			{
				quit_fmt("Too many grids (%d >= %d)!",
					num_grids, VINFO_MAX_GRIDS);
			}

			/* Count grids */
			num_grids++;

			/* Slope to the top right corner */
			m = SCALE * (1000L * y - 500) / (1000L * x + 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to top left corner */
			m = SCALE * (1000L * y - 500) / (1000L * x - 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to bottom right corner */
			m = SCALE * (1000L * y + 500) / (1000L * x + 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);

			/* Slope to bottom left corner */
			m = SCALE * (1000L * y + 500) / (1000L * x - 500);

			/* Handle "legal" slopes */
			vinfo_init_aux(hack, y, x, m);
		}
	}

	/* Enforce maximal efficiency (grids) */
	if (num_grids < VINFO_MAX_GRIDS)
	{
		quit_fmt("Too few grids (%d < %d)!",
			num_grids, VINFO_MAX_GRIDS);
	}

	/* Enforce maximal efficiency (line of sight slopes) */
	if (hack->num_slopes < VINFO_MAX_SLOPES)
	{
		quit_fmt("Too few LOS slopes (%d < %d)!",
			hack->num_slopes, VINFO_MAX_SLOPES);
	}


	/* Sort slopes numerically */
	ang_sort_comp = ang_sort_comp_hook_longs;

	/* Sort slopes numerically */
	ang_sort_swap = ang_sort_swap_hook_longs;

	/* Sort the (unique) LOS slopes */
	ang_sort(hack->slopes, NULL, hack->num_slopes);


	/* Enqueue player grid */
	queue[queue_tail++] = &vinfo[0];

	/* Process queue */
	while (queue_head < queue_tail)
	{
		int e;

		/* Index */
		e = queue_head++;

		/* Main Grid */
		g = vinfo[e].grid[0];

		/* Location */
		y = GRID_Y(g);
		x = GRID_X(g);


		/* Compute grid offsets */
		vinfo[e].grid[0] = GRID(+y,+x);
		vinfo[e].grid[1] = GRID(+x,+y);
		vinfo[e].grid[2] = GRID(+x,-y);
		vinfo[e].grid[3] = GRID(+y,-x);
		vinfo[e].grid[4] = GRID(-y,-x);
		vinfo[e].grid[5] = GRID(-x,-y);
		vinfo[e].grid[6] = GRID(-x,+y);
		vinfo[e].grid[7] = GRID(-y,+x);


		/* Skip player grid */
		if (e > 0)
		{
			long slope_fire;

			long tmp0 = 0;
			long tmp1 = 0;
			long tmp2 = 999999L;

			/* Determine LOF slope for this grid */
			if (x == 0) slope_fire = SCALE;
			else slope_fire = SCALE * (1000L * y) / (1000L * x);

			/* Analyze LOS slopes */
			for (i = 0; i < hack->num_slopes; ++i)
			{
				m = hack->slopes[i];

				/* Memorize intersecting slopes */
				if ((hack->slopes_min[y][x] < m) &&
				    (hack->slopes_max[y][x] > m))
				{
					/* Add it to the LOS slope set */
					switch (i / 32)
					{
						case 3: vinfo[e].bits_3 |= (1L << (i % 32)); break;
						case 2: vinfo[e].bits_2 |= (1L << (i % 32)); break;
						case 1: vinfo[e].bits_1 |= (1L << (i % 32)); break;
						case 0: vinfo[e].bits_0 |= (1L << (i % 32)); break;
					}

					/* Check for exact match with the LOF slope */
					if (m == slope_fire) tmp0 = i;

					/* Remember index of nearest LOS slope < than LOF slope */
					else if ((m < slope_fire) && (m > tmp1)) tmp1 = i;

					/* Remember index of nearest LOS slope > than LOF slope */
					else if ((m > slope_fire) && (m < tmp2)) tmp2 = i;
				}
			}

			/* There is a perfect match with one of the LOS slopes */
			if (tmp0)
			{
				/* Save the (unique) slope */
				vinfo[e].slope_fire_index1 = tmp0;

				/* Mark the other empty */
				vinfo[e].slope_fire_index2 = 0;
			}

			/* The LOF slope lies between two LOS slopes */
			else
			{
				/* Save the first slope */
				vinfo[e].slope_fire_index1 = tmp1;

				/* Save the second slope */
				vinfo[e].slope_fire_index2 = tmp2;
			}
		}

		/* Default */
		vinfo[e].next_0 = &vinfo[0];

		/* Grid next child */
		if (distance(0, 0, y, x+1) <= MAX_SIGHT)
		{
			g = GRID(y,x+1);

			if (queue[queue_tail-1]->grid[0] != g)
			{
				vinfo[queue_tail].grid[0] = g;
				queue[queue_tail] = &vinfo[queue_tail];
				queue_tail++;
			}

			vinfo[e].next_0 = &vinfo[queue_tail - 1];
		}


		/* Default */
		vinfo[e].next_1 = &vinfo[0];

		/* Grid diag child */
		if (distance(0, 0, y+1, x+1) <= MAX_SIGHT)
		{
			g = GRID(y+1,x+1);

			if (queue[queue_tail-1]->grid[0] != g)
			{
				vinfo[queue_tail].grid[0] = g;
				queue[queue_tail] = &vinfo[queue_tail];
				queue_tail++;
			}

			vinfo[e].next_1 = &vinfo[queue_tail - 1];
		}


		/* Hack -- main diagonal has special children */
		if (y == x) vinfo[e].next_0 = vinfo[e].next_1;


		/* Grid coordinates, approximate distance  */
		vinfo[e].y = y;
		vinfo[e].x = x;
		vinfo[e].d = ((y > x) ? (y + x/2) : (x + y/2));
		vinfo[e].r = ((!y) ? x : (!x) ? y : (y == x) ? y : 0);
	}

	/* Verify maximal bits XXX XXX XXX */
	if (((vinfo[1].bits_3 | vinfo[2].bits_3) != VINFO_BITS_3) ||
	    ((vinfo[1].bits_2 | vinfo[2].bits_2) != VINFO_BITS_2) ||
	    ((vinfo[1].bits_1 | vinfo[2].bits_1) != VINFO_BITS_1) ||
	    ((vinfo[1].bits_0 | vinfo[2].bits_0) != VINFO_BITS_0))
	{
		quit("Incorrect bit masks!");
	}


	/* Free hack */
	FREE(hack);


	/* Success */
	return (0);
}


/*
 * Forget the "PLAY_VIEW" grids, redrawing as needed
 */
void forget_view(void)
{
	int i, g;

	int fast_view_n = view_n;
	u16b *fast_view_g = view_g;

	byte *fast_play_info = &play_info[0][0];

	/* None to forget */
	if (!fast_view_n) return;

	/* Clear them all */
	for (i = 0; i < fast_view_n; i++)
	{
		int y, x;

		/* Grid */
		g = fast_view_g[i];

		/* Location */
		y = GRID_Y(g);
		x = GRID_X(g);

		/* Clear "PLAY_VIEW" and "PLAY_SEEN" flags */
		fast_play_info[g] &= ~(PLAY_VIEW | PLAY_SEEN);

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	fast_view_n = 0;

	/* Save 'view_n' */
	view_n = fast_view_n;

	/* Clear the PLAY_FIRE flag */
	for (i = 0; i < fire_n; i++)
	{
		/* Grid */
		g = fire_g[i];

		/* Clear */
		fast_play_info[g] &= ~(PLAY_FIRE);
	}

	/* None left */
	fire_n = 0;

}



/*
 * Calculate the complete field of view using a new algorithm
 *
 * If "view_g" and "temp_g" were global pointers to arrays of grids, as
 * opposed to actual arrays of grids, then we could be more efficient by
 * using "pointer swapping".
 *
 * Note the following idiom, which is used in the function below.
 * This idiom processes each "octant" of the field of view, in a
 * clockwise manner, starting with the east strip, south side,
 * and for each octant, allows a simple calculation to set "g"
 * equal to the proper grids, relative to "pg", in the octant.
 *
 *   for (o2 = 0; o2 < 8; o2++)
 *   ...
 * g = pg + p->grid[o2];
 *   ...
 *
 *
 * Normally, vision along the major axes is more likely than vision
 * along the diagonal axes, so we check the bits corresponding to
 * the lines of sight near the major axes first.
 *
 * We use the "temp_g" array (and the "CAVE_TEMP" flag) to keep track of
 * which grids were previously marked "CAVE_SEEN", since only those grids
 * whose "CAVE_SEEN" value changes during this routine must be redrawn.
 *
 * This function is now responsible for maintaining the "CAVE_SEEN"
 * flags as well as the "CAVE_VIEW" flags, which is good, because
 * the only grids which normally need to be memorized and/or redrawn
 * are the ones whose "CAVE_SEEN" flag changes during this routine.
 *
 * Basically, this function divides the "octagon of view" into octants of
 * grids (where grids on the main axes and diagonal axes are "shared" by
 * two octants), and processes each octant one at a time, processing each
 * octant one grid at a time, processing only those grids which "might" be
 * viewable, and setting the "CAVE_VIEW" flag for each grid for which there
 * is an (unobstructed) line of sight from the center of the player grid to
 * any internal point in the grid (and collecting these "CAVE_VIEW" grids
 * into the "view_g" array), and setting the "CAVE_SEEN" flag for the grid
 * if, in addition, the grid is "illuminated" in some way.
 *
 * This function relies on a theorem (suggested and proven by Mat Hostetter)
 * which states that in each octant of a field of view, a given grid will
 * be "intersected" by one or more unobstructed "lines of sight" from the
 * center of the player grid if and only if it is "intersected" by at least
 * one such unobstructed "line of sight" which passes directly through some
 * corner of some grid in the octant which is not shared by any other octant.
 * The proof is based on the fact that there are at least three significant
 * lines of sight involving any non-shared grid in any octant, one which
 * intersects the grid and passes though the corner of the grid closest to
 * the player, and two which "brush" the grid, passing through the "outer"
 * corners of the grid, and that any line of sight which intersects a grid
 * without passing through the corner of a grid in the octant can be "slid"
 * slowly towards the corner of the grid closest to the player, until it
 * either reaches it or until it brushes the corner of another grid which
 * is closer to the player, and in either case, the existanc of a suitable
 * line of sight is thus demonstrated.
 *
 * It turns out that in each octant of the radius 20 "octagon of view",
 * there are 161 grids (with 128 not shared by any other octant), and there
 * are exactly 126 distinct "lines of sight" passing from the center of the
 * player grid through any corner of any non-shared grid in the octant.  To
 * determine if a grid is "viewable" by the player, therefore, you need to
 * simply show that one of these 126 lines of sight intersects the grid but
 * does not intersect any wall grid closer to the player.  So we simply use
 * a bit vector with 126 bits to represent the set of interesting lines of
 * sight which have not yet been obstructed by wall grids, and then we scan
 * all the grids in the octant, moving outwards from the player grid.  For
 * each grid, if any of the lines of sight which intersect that grid have not
 * yet been obstructed, then the grid is viewable.  Furthermore, if the grid
 * is a wall grid, then all of the lines of sight which intersect the grid
 * should be marked as obstructed for future reference.  Also, we only need
 * to check those grids for whom at least one of the "parents" was a viewable
 * non-wall grid, where the parents include the two grids touching the grid
 * but closer to the player grid (one adjacent, and one diagonal).  For the
 * bit vector, we simply use 4 32-bit integers.  All of the static values
 * which are needed by this function are stored in the large "vinfo" array
 * (above), which is machine generated by another program.  XXX XXX XXX
 *
 * Hack -- The queue must be able to hold more than VINFO_MAX_GRIDS grids
 * because the grids at the edge of the field of view use "grid zero" as
 * their children, and the queue must be able to hold several of these
 * special grids.  Because the actual number of required grids is bizarre,
 * we simply allocate twice as many as we would normally need.  XXX XXX XXX
 */
void update_view(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int pg = GRID(py,px);

	int i, g, o2;

	int radius;

	int fast_view_n = view_n;
	u16b *fast_view_g = view_g;

	int fast_temp_n = 0;
	u16b *fast_temp_g = temp_g;

	byte *fast_cave_info = &cave_info[0][0];
	byte *fast_play_info = &play_info[0][0];

	byte pinfo;
	byte cinfo;

	/*** Step 0 -- Begin ***/

	/* Save the old "view" grids for later */
	for (i = 0; i < fast_view_n; i++)
	{
		/* Grid */
		g = fast_view_g[i];

		/* Get grid info */
		pinfo = fast_play_info[g];
		cinfo = fast_cave_info[g];

		/* Save "PLAY_SEEN" grids */
		if (pinfo & (PLAY_SEEN))
		{
			/* Set "PLAY_TEMP" flag */
			pinfo |= (PLAY_TEMP);

			/* Save grid for later */
			fast_temp_g[fast_temp_n++] = g;
		}

		/* Clear various player FOV flags */
		pinfo &= ~(PLAY_VIEW | PLAY_SEEN | PLAY_LITE);

		/* Save cave info */
		fast_play_info[g] = pinfo;
	}

	/* Reset the "view" array */
	fast_view_n = 0;

	/* Clear the CAVE_FIRE flag */
	for (i = 0; i < fire_n; i++)
	{
		/* Grid */
		g = fire_g[i];

		/* Clear */
		fast_play_info[g] &= ~(PLAY_FIRE);
	}

	/* Reset the "fire" array */
	fire_n = 0;

	/* Extract "radius" value */
	radius = p_ptr->cur_lite;

	/* Handle real light */
	if (radius > 0) ++radius;

	/*** Step 1 -- player grid ***/

	/* Player grid */
	g = pg;

	/* Get grid info */
	cinfo = fast_cave_info[g];

	/* Get grid info */
	pinfo = fast_play_info[g];

	/* Assume viewable & shootable */
	pinfo |= (PLAY_VIEW | PLAY_FIRE);

	/* Torch-lit grid */
	if (0 < radius)
	{
		/* Mark as "PLAY_SEEN" */
		pinfo |= (PLAY_SEEN);

		/* Mark as "PLAY_LITE" */
		pinfo |= (PLAY_LITE);
	}

	/* Lit grid */
	else if (cinfo & (CAVE_LITE))
	{
		/* Mark as "PLAY_SEEN" */
		pinfo |= (PLAY_SEEN);
	}

	/* Save player info */
	fast_play_info[g] = pinfo;

	/* Save in array */
	fast_view_g[fast_view_n++] = g;

	/* Save in the "fire" array */
	fire_g[fire_n++] = g;

	/*** Step 2a -- octants (PLAY_VIEW | PLAY_SEEN) ***/
	/* Scan each octant */
	for (o2 = 0; o2 < 8; o2++)
	{
		vinfo_type *p;

		/* Last added */
		vinfo_type *last = &vinfo[0];

		/* Grid queue */
		int queue_head = 0;
		int queue_tail = 0;
		vinfo_type *queue[VINFO_MAX_GRIDS*2];

		/* Slope bit vector */
		u32b bits0 = VINFO_BITS_0;
		u32b bits1 = VINFO_BITS_1;
		u32b bits2 = VINFO_BITS_2;
		u32b bits3 = VINFO_BITS_3;

		/* Reset queue */
		queue_head = queue_tail = 0;

		/* Initial grids */
		queue[queue_tail++] = &vinfo[1];
		queue[queue_tail++] = &vinfo[2];

		/* Process queue */
		while (queue_head < queue_tail)
		{
			/* Dequeue next grid */
			p = queue[queue_head++];

			/* Check bits */
			if ((bits0 & (p->bits_0)) ||
			    (bits1 & (p->bits_1)) ||
			    (bits2 & (p->bits_2)) ||
			    (bits3 & (p->bits_3)))
			{
				/* Extract grid value XXX XXX XXX */
				g = pg + p->grid[o2];

				/* Get grid info */
				cinfo = fast_cave_info[g];

				/* Get grid info */
				pinfo = fast_play_info[g];

				/* Handle wall */
				if (cinfo & (CAVE_XLOS))
				{
					/* Clear bits */
					bits0 &= ~(p->bits_0);
					bits1 &= ~(p->bits_1);
					bits2 &= ~(p->bits_2);
					bits3 &= ~(p->bits_3);

					/* Newly viewable wall */
					if (!(pinfo & (PLAY_VIEW)))
					{
						/* Mark as viewable */
						pinfo |= (PLAY_VIEW);

						/* Torch-lit grids */
						if (p->d < radius)
						{
							/* Mark as "PLAY_SEEN" */
							pinfo |= (PLAY_SEEN);

							/* Mark as "PLAY_LITE" */
							pinfo |= (PLAY_LITE);

						}

						/* Lit grids */
						else if (cinfo & (CAVE_LITE))
						{
							int y = GRID_Y(g);
							int x = GRID_X(g);

							/* Hack -- move towards player */
							int yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
							int xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

#ifdef UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION

							/* Check for "complex" illumination */
							if ((!(cave_info[yy][xx] & (CAVE_XLOS)) &&
							      (cave_info[yy][xx] & (CAVE_LITE))) ||
							    (!(cave_info[y][xx] & (CAVE_XLOS)) &&
							      (cave_info[y][xx] & (CAVE_LITE))) ||
							    (!(cave_info[yy][x] & (CAVE_XLOS)) &&
							      (cave_info[yy][x] & (CAVE_LITE))))
							{
								/* Mark as seen */
								pinfo |= (PLAY_SEEN);
							}

#else /* UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION */

							/* Check for "simple" illumination */
							if (cave_info[yy][xx] & (CAVE_LITE))
							{
								/* Mark as seen */
								pinfo |= (PLAY_SEEN);
							}

#endif /* UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION */

						}

						/* Save cave info */
						fast_cave_info[g] = cinfo;

						/* Save player info */
						fast_play_info[g] = pinfo;

						/* Save in array */
						fast_view_g[fast_view_n++] = g;
					}
				}

				/* Handle non-wall */
				else
				{
					/* Enqueue child */
					if (last != p->next_0)
					{
						queue[queue_tail++] = last = p->next_0;
					}

					/* Enqueue child */
					if (last != p->next_1)
					{
						queue[queue_tail++] = last = p->next_1;
					}

					/* Newly viewable non-wall */
					if (!(pinfo & (PLAY_VIEW)))
					{
						/* Mark as "viewable" */
						pinfo |= (PLAY_VIEW);

						/* Torch-lit grids */
						if (p->d < radius)
						{
							/* Mark as "PLAY_SEEN" */
							pinfo |= (PLAY_SEEN);

							/* Mark as "PLAY_LITE" */
							pinfo |= (PLAY_LITE);
						}

						/* Lit grids */
						else if (cinfo & (CAVE_LITE))
						{
							/* Mark as "CAVE_SEEN" */
							pinfo |= (PLAY_SEEN);
						}

						/* Save cave info */
						fast_cave_info[g] = cinfo;

						/* Save player info */
						fast_play_info[g] = pinfo;

						/* Save in array */
						fast_view_g[fast_view_n++] = g;
					}
				}
			}
		}
	}

	/*** Step 2b -- octants (PLAY_FIRE) ***/
	/* Scan each octant */
	for (o2 = 0; o2 < 8; o2++)
	{
		vinfo_type *p;

		/* Last added */
		vinfo_type *last = &vinfo[0];

		/* Grid queue */
		int queue_head = 0;
		int queue_tail = 0;
		vinfo_type *queue[VINFO_MAX_GRIDS*2];

		/* Slope bit vector */
		u32b bits0 = VINFO_BITS_0;
		u32b bits1 = VINFO_BITS_1;
		u32b bits2 = VINFO_BITS_2;
		u32b bits3 = VINFO_BITS_3;

		/* Reset queue */
		queue_head = queue_tail = 0;

		/* Initial grids */
		queue[queue_tail++] = &vinfo[1];
		queue[queue_tail++] = &vinfo[2];

		/* Process queue */
		while (queue_head < queue_tail)
		{
			/* Assume no line of fire */
			bool line_fire = FALSE;

			/* Dequeue next grid */
			p = queue[queue_head++];

			/* Check bits */
			if ((bits0 & (p->bits_0)) ||
			    (bits1 & (p->bits_1)) ||
			    (bits2 & (p->bits_2)) ||
			    (bits3 & (p->bits_3)))
			{
				/* Extract grid value XXX XXX XXX */
				g = pg + p->grid[o2];

				/* Get grid info */
				cinfo = fast_cave_info[g];

				/* Get grid info */
				pinfo = fast_play_info[g];

				/* Check for first possible line of fire */
				i = p->slope_fire_index1;

				/* Check line(s) of fire */
				while (TRUE)
				{
					switch (i / 32)
					{
						case 3:
						{
							if (bits3 & (1L << (i % 32))) line_fire = TRUE;
							break;
						}
						case 2:
						{
							if (bits2 & (1L << (i % 32))) line_fire = TRUE;
							break;
						}
						case 1:
						{
							if (bits1 & (1L << (i % 32))) line_fire = TRUE;
							break;
						}
						case 0:
						{
							if (bits0 & (1L << (i % 32))) line_fire = TRUE;
							break;
						}
					}

					/* Check second LOF slope if necessary */
					if ((!p->slope_fire_index2) || (line_fire) ||
					    (i == p->slope_fire_index2))
					{
						break;
					}

					/* Check second possible line of fire */
					i = p->slope_fire_index2;
				}

				/* Note line of fire */
				if (line_fire)
				{
					pinfo |= (PLAY_FIRE);

					/* Save in array */
					fire_g[fire_n++] = g;

					/* Save player info */
					fast_play_info[g] = pinfo;
				}

				/* Handle non-projectable grids */
				if (cinfo & (CAVE_XLOF))
				{
					/* Clear bits */
					bits0 &= ~(p->bits_0);
					bits1 &= ~(p->bits_1);
					bits2 &= ~(p->bits_2);
					bits3 &= ~(p->bits_3);
				}

				/* Handle projectable grids */
				else
				{
					/* Enqueue child */
					if (last != p->next_0)
					{
						queue[queue_tail++] = last = p->next_0;
					}

					/* Enqueue child */
					if (last != p->next_1)
					{
						queue[queue_tail++] = last = p->next_1;
					}
				}
			}
		}
	}

	/*** Step 3 -- Complete the algorithm ***/

	/* Handle blindness */
	if ((p_ptr->timed[TMD_BLIND]) || (p_ptr->timed[TMD_PSLEEP] >= PY_SLEEP_ASLEEP))
	{
		/* Process "new" grids */
		for (i = 0; i < fast_view_n; i++)
		{
			/* Grid */
			g = fast_view_g[i];

			/* Grid cannot be "PLAY_SEEN" */
			fast_play_info[g] &= ~(PLAY_SEEN | PLAY_LITE);
		}
	}

	/* Process "new" grids */
	for (i = 0; i < fast_view_n; i++)
	{
		/* Grid */
		g = fast_view_g[i];

		/* Get grid info */
		pinfo = fast_play_info[g];

		/* Was not "PLAY_SEEN", is now "PLAY_SEEN" */
		if ((pinfo & (PLAY_SEEN)) && !(pinfo & (PLAY_TEMP)))
		{
			int y, x;

			/* Location */
			y = GRID_Y(g);
			x = GRID_X(g);

			/* Note */
			note_spot(y, x);

			/* Redraw */
			lite_spot(y, x);
		}
	}

	/* Process "old" grids */
	for (i = 0; i < fast_temp_n; i++)
	{
		/* Grid */
		g = fast_temp_g[i];

		/* Get grid info */
		cinfo = fast_cave_info[g];

		/* Get grid info */
		pinfo = fast_play_info[g];

		/* Clear "PLAY_TEMP" flag */
		pinfo &= ~(PLAY_TEMP);

		/* Save cave info */
		fast_play_info[g] = pinfo;

		/* Was "PLAY_SEEN", is now not "PLAY_SEEN" */
		if (!(pinfo & (PLAY_SEEN)))
		{
			int y, x;

			/* Location */
			y = GRID_Y(g);
			x = GRID_X(g);

			/* Redraw */
			lite_spot(y, x);
		}
	}


	/* Save 'view_n' */
	view_n = fast_view_n;
}



/*
 * Update the features that have dynamic flags. These grids need to be
 * checked every turn to see if they affected adjacent grids with
 * an attack of some kind.
 *
 * INSTANT grids always alter themselves.
 *
 * ADJACENT grids always affect cardinally adjacent grids with an attack equal to their blow
 * attack. They will affect diagonally adjacent grids 50% of the time (checked per grid).
 *
 * TIMED grids will 2% of the time alter themselves.
 *
 * SPREAD grids will 66% of the time affect an adjacent grid with an attack
 * equal to their blow attack, along with their own grid. 8% of the time they
 * will affect their own grid only.
 *
 * ERUPT grids will 5% of the time project a radius 2 ball attack equal to three times
 * the damage of their blow attack, centred on their grid.
 *
 * STRIKE grids will 5% of the time project a radius 0 ball attack equal to ten times
 * the damage of their blow attack, centred on their grid. At some point, it'd be
 * nice to change this to a beam attack in a random direction, but this will require
 * changes to the project function.
 *
 * Examples of dynamic attacks include fire, smoke, steam, acidic vapours and
 * poison gas.  Examples of erupt attacks include vents. Examples of strike
 * attacks include charged clouds.
 *
 * Because we can potentially have the whole level consisting of features of
 * this type, we maintain an array of 'nearby' grids, that is a list of all
 * grids out to an approximate distance of 20 away. We update the list of grids
 * whenever we move 5 or more distance away from the current location.
 *
 * As an efficiency gain, we only do the above, when we note such a level is
 * 'dyna_full', that is, we have tried adding a dyna_grid that would overflow
 * the array. In instances where we are not dyna_full, the dyna_grid array contains
 * every instance of a dynamic feature of some kind on the level. Once a level
 * has been noted as being 'dyna_full', we do not reset this information until
 * the start of level generation on a new level.
 *
 * We also update this list by adding and removing features that are either
 * dynamic as they are created/destroyed. However, this requires that we create
 * a temporary copy of this list when we are applying the attacks in
 * this function.
 * This may prove not to be such an efficiency gain.
 */
void update_dyna(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int dist, y, x, g, i;

	int dam, flg;

	int fast_dyna_n = dyna_n;
	u16b *fast_dyna_g = dyna_g;

	int temp_dyna_n;
	u16b temp_dyna_g[DYNA_MAX];

	s16b feat;

	feature_type *f_ptr;

	bool full = FALSE;

	int alter;

	/* Efficiency */
	if (dyna_full)
	{
		dist = ABS(p_ptr->py - dyna_cent_y);
		if (ABS(p_ptr->px - dyna_cent_x) > dist)
			dist = ABS(p_ptr->px - dyna_cent_x);

		/*
		 * Character is far enough away from the previous dyna center -
		 * do a full rebuild.
		 */
		if (dist >= 5) full = TRUE;
	}

	/* Fully update */
	if (full)
	{
		int min_y = MAX(0, py - MAX_SIGHT);
		int max_y = MIN(DUNGEON_HGT, py + MAX_SIGHT + 1);
		int min_x = MAX(0, px - MAX_SIGHT);
		int max_x = MIN(DUNGEON_WID, px + MAX_SIGHT + 1);

		fast_dyna_n = 0;

		/* Scan the potentially visible area of the dungeon */
		for (y = min_y; y < max_y; y++)
		{
			for (x = min_x; x < max_x; x++)
			{
				/* Check distance */
				if (distance(y,x,py,px) > MAX_SIGHT) continue;

				/* Get grid feat */
				feat = cave_feat[y][x];

				/* Get the feature */
				f_ptr = &f_info[feat];

				/* Check for dynamic */
				if (f_ptr->flags3 & (FF3_DYNAMIC_MASK))
				{
					fast_dyna_g[fast_dyna_n++] = GRID(y,x);
				}
			}
		}

		dyna_cent_y = py;
		dyna_cent_x = px;
		dyna_n = fast_dyna_n;
	}

	/* Actually apply the attacks */
	for (i = 0; i < fast_dyna_n; i++)
	{
		temp_dyna_g[i] = fast_dyna_g[i];
	}

	temp_dyna_n = fast_dyna_n;

	/* Actually apply the attacks */
	for (i = 0; i < temp_dyna_n; i++)
	{
		feature_type *f_ptr;

		/* Grid */
		g = temp_dyna_g[i];

		/* Coordinates */
		y = GRID_Y(g);
		x = GRID_X(g);

		/* Get grid feat */
		feat = cave_feat[y][x];

		/* Get the feature */
		f_ptr = &f_info[feat];

		/* Default action */
		alter = 0;

		/* Erupt */
		if (f_ptr->flags3 & (FF3_ERUPT))
		{
			if (!(rand_int(20)))
			{
				flg = PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAY;

				dam = damroll(f_ptr->blow.d_side,f_ptr->blow.d_dice) * 2;

				/* Apply the blow */
				project(SOURCE_FEATURE, feat, 2, 0, y, x, y, x, dam, f_ptr->blow.effect, flg, 0, 0);

				alter = FS_ERUPT;
			}
		}

		/* Strike */
		if (f_ptr->flags3 & (FF3_STRIKE))
		{
			if (!(rand_int(20)))
			{
				flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAY;

				dam = damroll(f_ptr->blow.d_side,f_ptr->blow.d_dice) * 10;

				/* Apply the blow */
				project(SOURCE_FEATURE, feat, 0, 0, y, x, y, x, dam, f_ptr->blow.effect, flg, 0, 0);

				alter = FS_STRIKE;
			}
		}

		/* Instant */
		if (f_ptr->flags3 & (FF3_INSTANT))
		{
			alter = FS_INSTANT;
		}

		/* Adjacent */
		if (f_ptr->flags3 & (FF3_ADJACENT))
		{
			int yy, xx, adjfeat, dir;
			byte affected = 0;

			for (dir = 0; dir < 8; dir ++)
			{
				/* Hack -- skip diagonals 50% of the time, as long as adjacent cardinal direction affected */
				if ((dir > 3) && (rand_int(100) < 50) &&
					(((affected & (1 << side_dirs[dir][1])) != 0) || ((affected & (1 << side_dirs[dir][2])) != 0))) continue;

				yy = y + ddy_ddd[dir];
				xx = x + ddx_ddd[dir];

				adjfeat = cave_feat[yy][xx];

				flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;

				dam = damroll(f_ptr->blow.d_side,f_ptr->blow.d_dice);

				/* Apply the blow */
				project(SOURCE_FEATURE, feat, 0, 0, yy, xx, yy, xx, dam, f_ptr->blow.effect, flg, 0, 0);

				/* Hack -- Remove adjacent grids from further processing */
				if (adjfeat != cave_feat[yy][xx])
				{
					int j;

					affected |= dir;

					for (j = i + 1; j < temp_dyna_n; j++)
					if (temp_dyna_g[j] == GRID(yy,xx))
					{
						temp_dyna_n--;

						temp_dyna_g[j] = temp_dyna_g[temp_dyna_n];
					}
				}
			}

			if (!alter) alter = FS_ADJACENT;
		}

		/* Spread */
		if (f_ptr->flags3 & (FF3_SPREAD))
		{
			int dir = rand_int(12);

			if (dir < 8)
			{
				int yy = y + ddy_ddd[dir];
				int xx = x + ddx_ddd[dir];

				int adjfeat = cave_feat[yy][xx];

				flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;

				dam = damroll(f_ptr->blow.d_side,f_ptr->blow.d_dice);

				/* Apply the blow */
				project(SOURCE_FEATURE, feat, 0, 0, yy, xx, yy, xx, dam, f_ptr->blow.effect, flg, 0, 0);

				/* Hack -- require smoke/acid clouds to move */
				if (adjfeat == cave_feat[yy][xx]) dir = 12;

				/* Hack -- remove altered grids from further processing this iteration */
				else
				{
					int j;

					for (j = i + 1; j < temp_dyna_n; j++)
					if (temp_dyna_g[j] == GRID(yy,xx))
					{
						temp_dyna_n--;

						temp_dyna_g[j] = temp_dyna_g[temp_dyna_n];
					}
				}
			}

			if ((dir < 9) && (!alter))
			{
				alter = FS_SPREAD;
			}
		}

		/* Timed */
		if (f_ptr->flags3 & (FF3_TIMED))
		{
			if (rand_int(50) == 0)
			{
				alter = FS_TIMED;
			}
		}

		/* Alter terrain */
		if (alter > 0)
		{
			cave_alter_feat(y,x,alter);
		}
	}
}



/*
 * Every so often, the character makes enough noise that nearby
 * monsters can use it to home in on him.
 *
 * Fill in the "cave_cost" field of every grid that the player can
 * reach with the number of steps needed to reach that grid.  This
 * also yields the route distance of the player from every grid.
 *
 * Monsters use this information by moving to adjacent grids with
 * lower flow costs, thereby homing in on the player even though
 * twisty tunnels and mazes.  Monsters can also run away from loud
 * noises.
 *
 * The biggest limitation of this code is that it does not easily
 * allow for alternate ways around doors (not all monsters can handle
 * doors) and lava/water (many monsters are not allowed to enter
 * water, lava, or both).
 *
 * The flow table is three-dimensional.  The first dimension allows the
 * table to both store and overwrite grids safely.  The second indicates
 * whether this value is that for x or for y.  The third is the number
 * of grids able to be stored at any flow distance.
 */
void update_noise(void)
{
#ifdef MONSTER_FLOW
	int cost;
	int route_distance = 0;

	int i, d;
	int y, x, y2, x2;
	int last_index;
	int grid_count = 0;

	int dist;
	bool full = FALSE;

	/* Note where we get information from, and where we overwrite */
	int this_cycle = 0;
	int next_cycle = 1;

	byte flow_table[2][2][8 * NOISE_STRENGTH];

	/* The character's grid has no flow info.  Do a full rebuild. */
	if (cave_cost[p_ptr->py][p_ptr->px] == 0) full = TRUE;

	/* Determine when to rebuild, update, or do nothing */
	if (!full)
	{
		dist = ABS(p_ptr->py - flow_center_y);
		if (ABS(p_ptr->px - flow_center_x) > dist)
			dist = ABS(p_ptr->px - flow_center_x);

		/*
		 * Character is far enough away from the previous flow center -
		 * do a full rebuild.
		 */
		if (dist >= 15) full = TRUE;

		else
		{
			/* Get axis distance to center of last update */
			dist = ABS(p_ptr->py - update_center_y);
			if (ABS(p_ptr->px - update_center_x) > dist)
				dist = ABS(p_ptr->px - update_center_x);

			/*
			 * We probably cannot decrease the center cost any more.
			 * We should assume that we have to do a full rebuild.
			 */
			if (cost_at_center - (dist + 5) <= 0) full = TRUE;


			/* Less than five grids away from last update */
			else if (dist < 5)
			{
				/* We're in LOS of the last update - don't update again */
				if (generic_los(p_ptr->py, p_ptr->px, update_center_y,
		 		    update_center_x, CAVE_XLOS)) return;

				/* We're not in LOS - update */
				else full = FALSE;
			}

			/* Always update if at least five grids away */
			else full = FALSE;
		}
	}

	/* Update */
	if (!full)
	{
		bool found = FALSE;

		/* Start at the character's location */
		flow_table[this_cycle][0][0] = p_ptr->py;
		flow_table[this_cycle][1][0] = p_ptr->px;
		grid_count = 1;

		/* Erase outwards until we hit the previous update center */
		for (cost = 0; cost <= NOISE_STRENGTH; cost++)
		{
			/*
			 * Keep track of the route distance to the previous
			 * update center.
			 */
			route_distance++;


			/* Get the number of grids we'll be looking at */
			last_index = grid_count;

			/* Clear the grid count */
			grid_count = 0;

			/* Get each valid entry in the flow table in turn */
			for (i = 0; i < last_index; i++)
			{
				/* Get this grid */
				y = flow_table[this_cycle][0][i];
				x = flow_table[this_cycle][1][i];

				/* Look at all adjacent grids */
				for (d = 0; d < 8; d++)
				{
					/* Child location */
					y2 = y + ddy_ddd[d];
					x2 = x + ddx_ddd[d];

					/* Check Bounds */
					if (!in_bounds(y2, x2)) continue;

					/* Ignore illegal grids */
					if (cave_cost[y2][x2] == 0) continue;

					/* Ignore previously erased grids */
					if (cave_cost[y2][x2] == 255) continue;

					/* Erase previous info, mark grid */
					cave_cost[y2][x2] = 255;

					/* Store this grid in the flow table */
					flow_table[next_cycle][0][grid_count] = y2;
					flow_table[next_cycle][1][grid_count] = x2;

					/* Increment number of grids stored */
					grid_count++;

					/* If this is the previous update center, we can stop */
					if ((y2 == update_center_y) &&
						(x2 == update_center_x)) found = TRUE;
				}
			}

			/* Stop when we find the previous update center. */
			if (found) break;


			/* Swap write and read portions of the table */
			if (this_cycle == 0)
			{
				this_cycle = 1;
				next_cycle = 0;
			}
			else
			{
				this_cycle = 0;
				next_cycle = 1;
			}
		}

		/*
		 * Reduce the flow cost assigned to the new center grid by
		 * enough to maintain the correct cost slope out to the range
		 * we have to update the flow.
		 */
		cost_at_center -= route_distance;

		/* We can't reduce the center cost any more.  Do a full rebuild. */
		if (cost_at_center < 0) full = TRUE;

		else
		{
			/* Store the new update center */
			update_center_y = p_ptr->py;
			update_center_x = p_ptr->px;
		}
	}


	/* Full rebuild */
	if (full)
	{
		/*
		 * Set the initial cost to 100; updates will progressively
		 * lower this value.  When it reaches zero, another full
		 * rebuild has to be done.
		 */
		cost_at_center = 100;

		/* Save the new noise epicenter */
		flow_center_y = p_ptr->py;
		flow_center_x = p_ptr->px;
		update_center_y = p_ptr->py;
		update_center_x = p_ptr->px;


		/* Erase all of the current flow (noise) information */
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				cave_cost[y][x] = 0;
			}
		}
	}


	/*** Update or rebuild the flow ***/


	/* Store base cost at the character location */
	cave_cost[p_ptr->py][p_ptr->px] = cost_at_center;

	/* Store this grid in the flow table, note that we've done so */
	flow_table[this_cycle][0][0] = p_ptr->py;
	flow_table[this_cycle][1][0] = p_ptr->px;
	grid_count = 1;

	/* Extend the noise burst out to its limits */
	for (cost = cost_at_center + 1; cost <= cost_at_center + NOISE_STRENGTH; cost++)
	{
		/* Get the number of grids we'll be looking at */
		last_index = grid_count;

		/* Stop if we've run out of work to do */
		if (last_index == 0) break;

		/* Clear the grid count */
		grid_count = 0;

		/* Get each valid entry in the flow table in turn. */
		for (i = 0; i < last_index; i++)
		{
			/* Get this grid */
			y = flow_table[this_cycle][0][i];
			x = flow_table[this_cycle][1][i];

			/* Look at all adjacent grids */
			for (d = 0; d < 8; d++)
			{
				/* Child location */
				y2 = y + ddy_ddd[d];
				x2 = x + ddx_ddd[d];

				/* Check Bounds */
				if (!in_bounds(y2, x2)) continue;

				/* When doing a rebuild... */
				if (full)
				{
					/* Ignore previously marked grids */
					if (cave_cost[y2][x2]) continue;

					/* Ignore walls.  Do not ignore rubble. */
					if (f_info[cave_feat[y2][x2]].flags1 & (FF1_WALL))
					{
						continue;
					}
				}

				/* When doing an update... */
				else
				{
					/* Ignore all but specially marked grids */
					if (cave_cost[y2][x2] != 255) continue;
				}

				/* Store cost at this location */
				cave_cost[y2][x2] = cost;

				/* Store this grid in the flow table */
				flow_table[next_cycle][0][grid_count] = y2;
				flow_table[next_cycle][1][grid_count] = x2;

				/* Increment number of grids stored */
				grid_count++;
			}
		}

		/* Swap write and read portions of the table */
		if (this_cycle == 0)
		{
			this_cycle = 1;
			next_cycle = 0;
		}
		else
		{
			this_cycle = 0;
			next_cycle = 1;
		}
	}

#endif
}


/*
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Smell is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away from him.
 *
 * Smell is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent of the current age is laid down.
 * Speedy characters leave more scent, true, but it also ages faster,
 * which makes it harder to hunt them down.
 *
 * Whenever the age count loops, most of the scent trail is erased and
 * the age of the remainder is recalculated.
 */
void update_smell(void)
{
#ifdef MONSTER_FLOW

	int i, j;
	int y, x;

	int py = p_ptr->py;
	int px = p_ptr->px;


	/* Create a table that controls the spread of scent */
	int scent_adjust[5][5] =
	{
		{ 250,  2,  2,  2, 250 },
		{   2,  1,  1,  1,   2 },
		{   2,  1,  0,  1,   2 },
		{   2,  1,  1,  1,   2 },
		{ 250,  2,  2,  2, 250 },
	};

	/* Scent becomes "younger" */
	scent_when--;

	/* Loop the age and adjust scent values when necessary */
	if (scent_when <= 0)
	{
		/* Scan the entire dungeon */
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				/* Ignore non-existent scent */
				if (cave_when[y][x] == 0) continue;

				/* Erase the earlier part of the previous cycle */
				if (cave_when[y][x] > SMELL_STRENGTH) cave_when[y][x] = 0;

				/* Reset the ages of the most recent scent */
				else cave_when[y][x] = 250 - SMELL_STRENGTH + cave_when[y][x];
			}
		}

		/* Reset the age value */
		scent_when = 250 - SMELL_STRENGTH;
	}


	/* Lay down new scent */
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			/* Translate table to map grids */
			y = i + py - 2;
			x = j + px - 2;

			/* Check Bounds */
			if (!in_bounds(y, x)) continue;

			/* Walls, water, and lava cannot hold scent. */
			if ((f_info[cave_feat[y][x]].flags1 & (FF1_WALL)) ||
			    (f_info[cave_feat[y][x]].flags2 & (FF2_SHALLOW)) ||
			    (f_info[cave_feat[y][x]].flags2 & (FF2_DEEP)) ||
			    (f_info[cave_feat[y][x]].flags2 & (FF2_FILLED)))
			{
				continue;
			}

			/* Grid must not be blocked by walls from the character */
			if (!generic_los(p_ptr->py, p_ptr->px, y, x, CAVE_XLOF)) continue;

			/* Note grids that are too far away */
			if (scent_adjust[i][j] == 250) continue;

			/* Mark the grid with new scent */
			cave_when[y][x] = scent_when + scent_adjust[i][j];
		}
	}

#endif
}



/*
 * Map all rooms and neighbourhood belonging to a particular
 * monster's ecology membership. We also show some monsters
 * which occur in that region.
 */
bool map_home(int m_idx)
{
	u32b ecology = 0L;
	int i, y, x;

	/* No ecology */
	if (!cave_ecology.ready)
	{
		/* Show the whole level instead */
		wiz_lite();
		return (TRUE);
	}

	/* Get the flags */
	for (i = 0; i < cave_ecology.num_races; i++)
	{
		/* This monster exists here */
		if (cave_ecology.race[i] == m_idx)
		{
			/* Note ecologies */
			ecology |= cave_ecology.race_ecologies[i];
		}
	}

	/* Nothing? */
	if (!ecology)
	{
		/* Not a native of these parts */
		return (FALSE);
	}

	/* Scan that area */
	for (y = 1; y < DUNGEON_HGT - 1; y++)
	{
		for (x = 1; x < DUNGEON_WID - 1; x++)
		{
			/* All non-walls are "checked" */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_WALL)))
			{
				/* Skip if not in the ecology */
				if ((room_info[room_idx_ignore_valid(y, x)].ecology & (ecology)) == 0) continue;

				/* Memorize normal features */
				if (f_info[cave_feat[y][x]].flags1 & (FF1_REMEMBER))
				{
					/* Memorize the object */
					play_info[y][x] |= (PLAY_MARK);
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Memorize walls (etc) */
					if (f_info[cave_feat[yy][xx]].flags1 & (FF1_REMEMBER))
					{
						/* Memorize the walls */
						play_info[yy][xx] |= (PLAY_MARK);
					}
				}

				/* Get monster */
				i = cave_m_idx[y][x];

				/* Show monsters */
				if (i <= 0) continue;

				/* Reveal own kind */
				if ((r_info[m_list[i].r_idx].d_char == r_info[m_list[m_idx].r_idx].d_char)

					/* Reveal animals, insects, plants */
					|| ((r_info[i].flags3 & (RF3_ANIMAL | RF3_INSECT | RF3_PLANT)) != 0))
				{
					/* Reveal half of monsters */
					if (i % 2)
					{
						/* Optimize -- Repair flags */
						repair_mflag_mark = repair_mflag_show = TRUE;

						/* Hack -- Detect the monster */
						m_list[i].mflag |= (MFLAG_MARK | MFLAG_SHOW);

						/* Update the monster */
						update_mon(i, FALSE);
					}
				}
			}
		}
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP);

	return (TRUE);
}


/*
 * Map a circle ala "magic mapping"
 *
 * We must never attempt to map the outer dungeon walls, or we
 * might induce illegal cave grid references.
 */
void map_area(void)
{
	int i, x, y, y1, y2, x1, x2, r;

	/* Radius of detection */
	r = 2 * MAX_SIGHT;

	/* Pick an area to map */
	y1 = MAX(p_ptr->py - r, 1);
	y2 = MIN(p_ptr->py + r, DUNGEON_HGT-1);
	x1 = MAX(p_ptr->px - r, 1);
	x2 = MIN(p_ptr->px + r, DUNGEON_WID-1);

	/* Scan that area */
	for (y = y1; y < y2; y++)
	{
		for (x = x1; x < x2; x++)
		{
			/* Check distance */
			if (distance(p_ptr->py, p_ptr->px, y, x) > r) continue;

			/* All non-walls are "checked" */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_WALL)))
			{

				/* Memorize normal features */
				if (f_info[cave_feat[y][x]].flags1 & (FF1_REMEMBER))
				{
					/* Memorize the object */
					play_info[y][x] |= (PLAY_MARK);
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Memorize walls (etc) */
					if (f_info[cave_feat[yy][xx]].flags1 & (FF1_REMEMBER))
					{
						/* Memorize the walls */
						play_info[yy][xx] |= (PLAY_MARK);
					}
				}
			}
		}
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP);
}



/*
 * Light up the dungeon using "claravoyance"
 *
 * This function "illuminates" every grid in the dungeon, memorizes all
 * "objects", memorizes all grids as with magic mapping, and, under the
 * standard option settings (view_perma_grids but not view_torch_grids)
 * memorizes all floor grids too.
 *
 * Note that if "view_perma_grids" is not set, we do not memorize floor
 * grids, since this would defeat the purpose of "view_perma_grids", not
 * that anyone seems to play without this option.
 *
 * Note that if "view_torch_grids" is set, we do not memorize floor grids,
 * since this would prevent the use of "view_torch_grids" as a method to
 * keep track of what grids have been observed directly.
 */
void wiz_lite(void)
{
	int i, y, x;


	/* Memorize objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Memorize */
		o_ptr->ident |= (IDENT_MARKED);
	}

	/* Scan all normal grids */
	for (y = 1; y < DUNGEON_HGT-1; y++)
	{
		/* Scan all normal grids */
		for (x = 1; x < DUNGEON_WID-1; x++)
		{
			/* Process all non-walls */
			if (!(f_info[cave_feat[y][x]].flags1 & (FF1_WALL)))
			{
				/* Scan all neighbors */
				for (i = 0; i < 9; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Perma-lite the grid */
					cave_info[yy][xx] |= (CAVE_GLOW);

					/* Memorize normal features */
					if (f_info[cave_feat[yy][xx]].flags1 & (FF1_REMEMBER))
					{
						/* Memorize the grid */
						play_info[yy][xx] |= (PLAY_MARK);
					}

					/* Normally, memorize floors (see above) */
					if (view_perma_grids && !view_torch_grids)
					{
						/* Memorize the grid */
						play_info[yy][xx] |= (PLAY_MARK);
					}
				}
			}
		}
	}

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP | PW_MONLIST);
}


/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(void)
{
	int i, y, x;


	/* Forget every grid */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Process the grid */
			play_info[y][x] &= ~(PLAY_MARK);
		}
	}

	/* Forget all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Forget the object */
		o_ptr->ident &= ~(IDENT_MARKED);
	}

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MAP | PW_MONLIST);
}



/*
 * Light or Darken the town
 */
void town_illuminate(bool daytime)
{
	int y, x, i;

	town_type *t_ptr = &t_info[p_ptr->dungeon];

	dungeon_zone *zone=&t_ptr->zone[0];

	int dungeon_hgt, dungeon_wid;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	dungeon_hgt = (level_flag & (LF1_TOWN)) != 0 ? TOWN_HGT : DUNGEON_HGT;
	dungeon_wid = (level_flag & (LF1_TOWN)) != 0 ? TOWN_WID : DUNGEON_WID;

	/* Darken at night */
	for (y = 0; y < dungeon_hgt; y++)
	{
		for (x = 0; x < dungeon_wid; x++)
		{
			/* Light outside and adjacent indoors */
			if ((!daytime) || ((f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE)) == 0))
			{
				/* Darken the grid */
				cave_info[y][x] &= ~(CAVE_DLIT);
			}
		}
	}

	/* Apply light during day */
	if (daytime) for (y = 0; y < dungeon_hgt; y++)
	{
		for (x = 0; x < dungeon_wid; x++)
		{
			/* Location is outside */
			if (((f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE)) != 0)

				/* or light rooms lit by daylight */
				|| ((room_has_flag(y, x, ROOM_DAYLITE)) != 0))
			{
				/* Illuminate the grid */
				cave_info[y][x] |= (CAVE_DLIT);

				for (i = 0; i < 8; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Ignore annoying locations */
					if (!in_bounds(yy, xx)) continue;

					/* Illuminate the grid */
					cave_info[yy][xx] |= (CAVE_DLIT);
				}
			}
		}
	}

	/* XXX WARNING:
	 *
	 * Town_illuminate is called once whilst loading a character.
	 *
	 * The following will cause a monster to be generated before loading
	 * the monsters, and cause the save file to fail to load.
	 *
	 * Therefore we need to ensure that the character is loaded first.
	 *
	 */

	/* Megahack --- darkness brings out the bad guys */
	if ((character_loaded) && (!daytime) && actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone) && (r_info[actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone)].cur_num <= 0))
	{
		/* Place the guardian */
		place_guardian(1, 1, dungeon_hgt- 1, dungeon_wid -1);
	}

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_MONLIST | PW_MAP);
}



/*
 * Hack -- Really change the feature
 */
void cave_set_feat_aux(const int y, const int x, int feat)
{
	/* Set if blocks los */
	bool los = cave_floor_bold(y,x);

	/* Get feature */
	feature_type *f_ptr = &f_info[feat];

	/* Set if dynamic */
	bool dyna = (f_info[cave_feat[y][x]].flags3 & (FF3_DYNAMIC_MASK)) != 0;

	bool hide_item = (f_info[cave_feat[y][x]].flags2 & (FF2_HIDE_ITEM)) != 0;

	bool use_feat = (f_info[cave_feat[y][x]].flags3 & (FF3_USE_FEAT)) != 0 ? f_info[cave_feat[y][x]].k_idx : 0;

	s16b this_o_idx, next_o_idx = 0;

	/* Paranoia */
	if ((feat > z_info->f_max) || (y < 0) || (y > DUNGEON_HGT) || (x < 0) || (x > DUNGEON_WID))
	{
		msg_format("Error: Setting invalid feat %d at (%d, %d).", feat, y, x);
		return;
	}

	/* Paranoia */
	if (((f_info[feat].flags1 & (FF1_ENTER)) != 0) && ((level_flag & (LF1_TOWN)) == 0))
	{
		msg_format("BUG: Setting store %d at (%d, %d) outside of a town level.", feat, y, x);
		return;
	}

	/* Really set the feature */
	cave_feat[y][x] = feat;

	/* Check for line of sight */
	if (f_ptr->flags1 & (FF1_LOS))
	{
		cave_info[y][x] &= ~(CAVE_XLOS);
	}

	/* Handle wall grids */
	else
	{
		cave_info[y][x] |= (CAVE_XLOS);
	}

	/* Check for line of fire */
	if (f_ptr->flags1 & (FF1_PROJECT))
	{
		cave_info[y][x] &= ~(CAVE_XLOF);
	}

	/* Handle wall grids */
	else
	{
		cave_info[y][x] |= (CAVE_XLOF);
	}

	/* Check for change to boring grid */
	if (!(f_ptr->flags1 & (FF1_REMEMBER))) play_info[y][x] &= ~(PLAY_MARK);

	/* Check for change to out of sight grid */
	else if (!(player_can_see_bold(y,x))) play_info[y][x] &= ~(PLAY_MARK);

	/* Check if adding to dynamic list */
	if (!dyna && (f_ptr->flags3 & (FF3_DYNAMIC_MASK)))
	{
		if (dyna_n >= (DYNA_MAX -1))
		{
			dyna_full = TRUE;

			/* Hack to force rebuild */
			dyna_cent_y = 255;
			dyna_cent_x = 255;
		}
		else
		{
			dyna_g[dyna_n++] = GRID(y,x);
		}
	}
	/* Check if removing from dynamic list */
	else if (dyna && !(f_ptr->flags3 & (FF3_DYNAMIC_MASK)))
	{
		int i;

		for (i = 0; i < dyna_n; i++)
		if (dyna_g[i] == GRID(y,x))
		{
			dyna_n--;
			dyna_g[i] = dyna_g[dyna_n];
		}
	}

	/* Check to see if monster exposed by change */
	if (cave_m_idx[y][x] > 0)
	{
		monster_type *m_ptr = &m_list[cave_m_idx[y][x]];

		bool hidden = ((m_ptr->mflag & (MFLAG_HIDE)) != 0);

		if (hidden)
		{
			monster_hide(y, x, place_monster_here(y, x, m_ptr->r_idx), m_ptr);

			if (!(m_ptr->mflag & (MFLAG_HIDE)))
			{
				/* And update */
				update_mon(cave_m_idx[y][x],FALSE);

				/* Hack --- tell the player if something unhides */
				if (m_ptr->ml)
				{
					char m_name[80];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

					msg_format("%^s emerges from %s%s.",m_name,
						((f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_FILLED))?"":"the "),
						f_name+f_info[cave_feat[m_ptr->fy][m_ptr->fx]].name);
				}

				/* Disturb on "move" */
				if ((m_ptr->ml &&
				    (disturb_move ||
				     ((m_ptr->mflag & (MFLAG_VIEW)) &&
				      disturb_near)))
				      	&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))

				{
					/* Disturb */
					disturb(0, 0);
				}
			}
		}
	}

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Hide stuff */
		if ((!hide_item) && (f_ptr->flags2 & (FF2_HIDE_ITEM)))
		{
			/* Hide it */
			o_ptr->ident &= ~(IDENT_MARKED);
		}

		/* Destroy stored items */
		if (o_ptr->ident & (IDENT_STORE))
		{
			/* Destroy anything that has a 'used' object that never really exists */
			if (use_feat && (!(f_ptr->flags3 & (FF3_USE_FEAT)) || (use_feat != f_ptr->k_idx)))
			{
				  delete_object_idx(this_o_idx);
			}
			/* Reveal other objects from e.g. traps */
			else if (!use_feat)
			{
				o_ptr->ident &= ~IDENT_STORE;

				/* Window flags */
				p_ptr->window |= (PW_ITEMLIST);
			}
			continue;
		}
	}

	/* Check if los has changed */
	if ((los) && (player_has_los_bold(y,x)) && !(cave_floor_bold(y,x)))
	{
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}
	else if ((!los) && (player_has_los_bold(y,x)) && (cave_floor_bold(y,x)))
	{
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Check if location changed under player */
	if ((p_ptr->py == y) && (p_ptr->px == x))
	{
		bool outside = (level_flag & (LF1_SURFACE)) &&
			(f_info[feat].flags3 & (FF3_OUTSIDE));

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Changed inside/outside */
		if (outside != p_ptr->outside)
		{
 			p_ptr->redraw |= (PR_MAP);

 			p_ptr->window |= (PW_OVERHEAD | PW_MAP);
		}

		/* Change state */
		p_ptr->outside = outside;
	}

	if (player_has_los_bold(y,x))
	{
		/* Notice */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* XXX XXX Disabled the following because it still causes infinite loops */
#if 0

	/* Apply terrain damage to all objects and player in the grid */
	/* XXX Don't call project as we may be getting called from there. */
	/* XXX Don't apply damage to monsters as we may lose out on getting experience */
	/* XXX XXX Be aware, that project_o can cause subsequent changes in terrain type,
	   resulting in recursion back here. However, we shouldn't have any dangerous
	   dangling pointers because project_o just marks objects for subsequent destruction,
	   and careful terrain design should hopefully pick up any infinite loops. */
	if (f_ptr->blow.effect)
	{
		project_o(0, y, x, damroll(f_ptr->blow.d_dice,f_ptr->blow.d_side), f_ptr->blow.effect);
		project_p(0, y, x, damroll(f_ptr->blow.d_dice,f_ptr->blow.d_side), f_ptr->blow.effect);
		project_t(0, y, x, damroll(f_ptr->blow.d_dice,f_ptr->blow.d_side), f_ptr->blow.effect);
	}

#endif
}

/*
 * Check adjacent locations to see if an attribute that a feature applies
 * to these locations still applies.
 *
 * This occurs for cave edges, trees, locations lit by daylight, locations lit by
 * glowing features etc.
 */
void check_attribute_lost(const int y, const int x, const int r, const byte los, tester_attribute_func require_attribute, tester_attribute_func has_attribute,
	tester_attribute_func redraw_attribute, modify_attribute_func_remove remove_attribute,	modify_attribute_func_reapply reapply_attribute)
{
	int yy, xx;
	int yyy, xxx;

	for (yy = y - r; yy <= y + r; yy++)
	{
		for (xx = x - r; xx <= x + r; xx++)
		{
			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Ignore distance locations */
			if ((r > 1) && (distance(y, x, yy, xx) > r)) continue;

			/* Was destroyed grid only one 'holding this one up' */
			if (require_attribute(yy, xx))
			{
				/* Temporarily remove the attribute from the grid */
				int rr = remove_attribute(yy, xx);

				/* Check nearby grids to reapply it */
				for (yyy = yy - r; yyy <= yy + r; yyy++)
				{
					for (xxx = x - r; xxx <= x + r; xxx++)
					{
						/* Ignore annoying locations */
						if (!in_bounds_fully(yyy, xxx)) continue;

						/* Ignore distance locations */
						if ((r > 1) && (distance(yy, xx, yyy, xxx) > r)) continue;

						/* Ensure grid has line of sight if required */
						if ((r > 1) && (los) && !(generic_los(yy, xx, yyy, xxx, los))) continue;

						/* Is supports the 'affected' grid */
						if (has_attribute(yyy,xxx))
						{
							reapply_attribute(yy, xx, rr);

							break;
						}
					}

					/* Require redraw */
					if (redraw_attribute(yy,xx))
					{
						note_spot(yy, xx);
						lite_spot(yy, xx);
					}
				}
			}
		}
	}
}


/*
 * Apply an attribute to nearby locations
 */
void gain_attribute(int y, int x, int r, byte los, modify_attribute_func apply_attribute, tester_attribute_func redraw_attribute)
{
	int yy, xx;

	for (yy = y - r; yy <= y + r; yy++)
	{
		for (xx = x - r; xx <= x + r; xx++)
		{
			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Ignore distance locations */
			if ((r > 1) && (distance(y, x, yy, xx) > r)) continue;

			/* Ensure grid has line of sight if required */
			if ((r > 1) && (los) && !(generic_los(y, x, yy, xx, los))) continue;

			/* Apply attribute */
			apply_attribute(yy, xx);

			/* Require redraw */
			if (redraw_attribute(yy, xx))
			{
				note_spot(yy, xx);
				lite_spot(yy, xx);
			}
		}
	}
}



/*
 * Halo functions
 */
bool require_halo(int y, int x)
{
	return ((cave_info[y][x] & (CAVE_HALO)) != 0);
}

bool has_halo(int y, int x)
{
	int this_o_idx, next_o_idx = 0;

	/* Glowing feature */
	if ((f_info[cave_feat[y][x]].flags2 & (FF2_GLOW)) != 0) return TRUE;

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Check object lite */
		if (check_object_lite(o_ptr)) return TRUE;
	}

	return FALSE;
}

bool redraw_halo_loss(int y, int x)
{
	return (((play_info[y][x] & (PLAY_VIEW)) && (play_info[y][x] & (PLAY_SEEN))
	&& !(cave_info[y][x] & (CAVE_LITE)))
	 || ((view_glowing_lite) && ((cave_info[y][x] & (CAVE_HALO)) == 0)));
}

bool redraw_halo_gain(int y, int x)
{
	return (((play_info[y][x] & (PLAY_VIEW)) && !(play_info[y][x] & (PLAY_SEEN)))
	 || ((view_glowing_lite) && ((cave_info[y][x] & (CAVE_HALO)) != 0)));
}

void apply_halo(int y, int x)
{
	cave_info[y][x] |= (CAVE_HALO);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}

int remove_halo(int y, int x)
{
	cave_info[y][x] &= ~(CAVE_HALO);
	if (!(play_info[y][x] & (PLAY_LITE)) && !(cave_info[y][x] & (CAVE_LITE))) play_info[y][x] &= ~(PLAY_SEEN);	
	
	return (0);
}

void reapply_halo(int y, int x, int r)
{
	(void)r;
	
	cave_info[y][x] |= (CAVE_HALO);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}


/*
 * Daylight functions
 */
bool require_daylight(int y, int x)
{
	return ((cave_info[y][x] & (CAVE_DLIT)) != 0);
}

bool has_daylight(int y, int x)
{
	bool daytime = (bool)(level_flag & (LF1_DAYLIGHT));
	bool outside = (level_flag & (LF1_SURFACE))
		&& (f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE));

	return (daytime && outside);
}

bool redraw_daylight_loss(int y, int x)
{
	return (((play_info[y][x] & (PLAY_VIEW)) && (play_info[y][x] & (PLAY_SEEN))
	&& !(cave_info[y][x] & (CAVE_LITE)))
	 || ((view_glowing_lite) && ((cave_info[y][x] & (CAVE_DLIT)) == 0)));
}

bool redraw_daylight_gain(int y, int x)
{
	return (((play_info[y][x] & (PLAY_VIEW)) && !(play_info[y][x] & (PLAY_SEEN)))
	 || ((view_glowing_lite) && ((cave_info[y][x] & (CAVE_DLIT)) != 0)));
}

void apply_daylight(int y, int x)
{
	cave_info[y][x] |= (CAVE_HALO);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}

int remove_daylight(int y, int x)
{
	cave_info[y][x] &= ~(CAVE_DLIT);
	if (!(play_info[y][x] & (PLAY_LITE)) && !(cave_info[y][x] & (CAVE_LITE))) play_info[y][x] &= ~(PLAY_SEEN);
	
	return (0);
}

void reapply_daylight(int y, int x, int r)
{
	(void)r;
	
	cave_info[y][x] |= (CAVE_DLIT);
	if ((play_info[y][x] & (PLAY_VIEW)) && !(p_ptr->timed[TMD_BLIND])) play_info[y][x] |= (PLAY_SEEN);
}


/*
 * Climb-able functions
 *
 * Optimisation for placement of monsters which can climb terrain.
 */
bool require_climb(int y, int x)
{
	return ((cave_info[y][x] & (CAVE_CLIM)) != 0);
}

bool has_climb(int y, int x)
{
	return ((f_info[cave_feat[y][x]].flags2 & (FF2_CAN_CLIMB)) != 0);
}

bool redraw_climb_loss(int y, int x)
{
	(void)y;
	(void)x;
	return FALSE;
}

bool redraw_climb_gain(int y, int x)
{
	(void)y;
	(void)x;
	return FALSE;
}

void apply_climb(int y, int x)
{
	cave_info[y][x] |= (CAVE_CLIM);
}

int remove_climb(int y, int x)
{
	cave_info[y][x] &= ~(CAVE_CLIM);
	
	return (0);
}

void reapply_climb(int y, int x, int r)
{
	(void)r;
	
	cave_info[y][x] |= (CAVE_CLIM);
}


/*
 * Chasm edge -> chasm functions
 */
bool require_chasm_edge(int y, int x)
{
	return (((f_info[cave_feat[y][x]].flags2 & (FF2_CHASM)) != 0) && (cave_feat[y][x] != FEAT_CHASM));
}

bool has_chasm_edge(int y, int x)
{
	return ((cave_feat[y][x] != FEAT_NONE) &&
			((f_info[cave_feat[y][x]].flags2 & (FF2_CHASM)) == 0) &&
			 ((f_info[cave_feat[y][x]].flags2 & (FF2_COVERED)) == 0));
}

bool redraw_chasm_edge_loss(int y, int x)
{
	(void)y;
	(void)x;

	return (FALSE);
}

bool redraw_chasm_edge_gain(int y, int x)
{
	(void)y;
	(void)x;

	return (FALSE);
}

void apply_chasm_edge(int y, int x)
{
	if (cave_feat[y][x] == FEAT_CHASM)
	{
		cave_set_feat_aux(y, x, FEAT_CHASM_E);
	}
}

int remove_chasm_edge(int y, int x)
{
	int r = cave_feat[y][x];

	cave_set_feat_aux(y, x, FEAT_CHASM);
	
	return (r);
}

void reapply_chasm_edge(int y, int x, int r)
{
	cave_set_feat_aux(y, x, r);
}


/*
 * Need tree functions
 */
bool require_tree(int y, int x)
{
	return ((f_info[cave_feat[y][x]].flags3 & (FF3_NEED_TREE)) != 0);
}

bool has_tree(int y, int x)
{
	return (((f_info[cave_feat[y][x]].flags3 & (FF3_TREE)) !=0) && (rand_int(100) < 50));
}

bool redraw_tree_loss(int y, int x)
{
	(void)y;
	(void)x;

	return (FALSE);
}

bool redraw_tree_gain(int y, int x)
{
	(void)y;
	(void)x;

	return (FALSE);
}

int remove_tree(int y, int x)
{
	int r = cave_feat[y][x];

	cave_set_feat_aux(y, x, feat_state(r, FS_NEED_TREE));
	
	return (r);
}

void reapply_tree(int y, int x, int r)
{
	cave_set_feat_aux(y, x, r);
}


void set_level_flags(int feat)
{
	feature_type *f_ptr = &f_info[feat];

	/* Set the level type */
	if (f_ptr->flags2 & (FF2_WATER)) level_flag |= (LF1_WATER);
	if (f_ptr->flags2 & (FF2_LAVA)) level_flag |= (LF1_LAVA);
	if (f_ptr->flags2 & (FF2_ICE)) level_flag |= (LF1_ICE);
	if (f_ptr->flags2 & (FF2_ACID)) level_flag |= (LF1_ACID);
	if (f_ptr->flags2 & (FF2_OIL)) level_flag |= (LF1_OIL);
	if (f_ptr->flags2 & (FF2_CHASM)) level_flag |= (LF1_CHASM);
	if (f_ptr->flags3 & (FF3_LIVING)) level_flag |= (LF1_LIVING);
}



/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(const int y, const int x, int feat)
{
	int i;

	int oldfeat = cave_feat[y][x];

	feature_type *f_ptr = &f_info[oldfeat];
	feature_type *f_ptr2 = &f_info[feat];

	bool surface = (bool)((level_flag & (LF1_SURFACE)));
	bool daytime = (bool)(level_flag & (LF1_DAYLIGHT));

	bool halo = (f_ptr->flags2 & FF2_GLOW) != 0;
	bool halo2 = (f_ptr2->flags2 & FF2_GLOW) != 0;

	bool dlit = daytime && surface && (f_ptr->flags3 & (FF3_OUTSIDE));
	bool dlit2 = daytime && surface && (f_ptr2->flags3 & (FF3_OUTSIDE));

	bool tree = (f_ptr->flags3 & (FF3_TREE)) !=0;
	bool tree2 = (f_ptr2->flags3 & (FF3_TREE)) != 0;

	bool climb = (f_ptr->flags2 & (FF2_CAN_CLIMB)) !=0;
	bool climb2 = (f_ptr2->flags2 & (FF2_CAN_CLIMB)) !=0;

	bool los =  (f_ptr->flags1 & (FF1_LOS)) !=0;
	bool los2 =  (f_ptr2->flags1 & (FF1_LOS)) !=0;

	bool project =  (f_ptr->flags1 & (FF1_PROJECT)) !=0;
	bool project2 =  (f_ptr2->flags1 & (FF1_PROJECT)) !=0;

	bool trap = (f_ptr->flags1 & (FF1_TRAP)) !=0;
	bool trap2 = (f_ptr2->flags1 & (FF1_TRAP)) !=0;

	bool chasm_edge = ((f_ptr->flags2 & (FF2_CHASM | FF2_COVERED)) ==0) && (cave_feat[y][x] != FEAT_NONE);
	bool chasm_edge2 = (f_ptr2->flags2 & (FF2_CHASM | FF2_COVERED)) ==0;

	/* Change the feature */
	cave_set_feat_aux(y,x,feat);

	/* Handle destroying "glowing" terrain */
	if (halo && !halo2)
	{
		check_attribute_lost(y, x, 2, CAVE_XLOS, require_halo, has_halo, redraw_halo_loss, remove_halo, reapply_halo);
	}

	/* Handle destroying "outside" terrain */
	if (dlit && !dlit2)
	{
		check_attribute_lost(y, x, 2, CAVE_XLOS, require_daylight, has_daylight, redraw_daylight_loss, remove_daylight, reapply_daylight);
	}

	/* Handle destroying "climb-able terrain */
	if (climb && !climb2)
	{
		check_attribute_lost(y, x, 1, 0, require_climb, has_climb, redraw_climb_loss, remove_climb, reapply_climb);
	}

	/* Handle removing chasms */
	if (chasm_edge && !chasm_edge2)
	{
		check_attribute_lost(y, x, 1, 0, require_chasm_edge, has_chasm_edge, redraw_chasm_edge_loss, remove_chasm_edge, reapply_chasm_edge);
	}

	/* Handle destroying "need_tree" terrain */
	if (tree && !tree2)
	{
		/* For correctness, we need to check if daylight is added to any adjacent location by branches falling */
		/* Should check more than this if we ever really mess with terrain.txt XXX */
		byte dlit_adj = 0;

		/* If in daylight on surface */
		if (daytime && surface) for (i = 0; i < 8; i++)
		{
			int yy = y + ddy_ddd[i];
			int xx = x + ddx_ddd[i];

     		/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Record daylight */
			if ((f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE)) != 0)
				dlit_adj |= 1 << i;
		}

		/* Cause branches to fall */
		check_attribute_lost(y, x, 1, 0, require_tree, has_tree, redraw_tree_loss, remove_tree, reapply_tree);

		/* Check if daylight added */
		if (daytime && surface) for (i = 0; i < 8; i++)
		{
			int yy = y + ddy_ddd[i];
			int xx = x + ddx_ddd[i];

     		/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Check if additional daylight */
			if (((dlit_adj & (1 << i)) != 0) && ((f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE)) != 0))
			{
				/* Add more light */
				gain_attribute(yy, xx, 2, CAVE_XLOS, apply_daylight, redraw_daylight_gain);
			}
		}
	}

	/* Handle disarming traps if required */
	if (trap && !trap2)
	{
		for (i = 0; i < region_max; i++)
		{
			/* Get the region */
			region_type *r_ptr = &region_list[i];

			/* Delete traps */
			if (((r_ptr->flags1 & (RE1_HIT_TRAP)) != 0) && (r_ptr->y0 == y) && (r_ptr->x0 == x))
			{
				/* Kill the region */
				region_terminate(i);

				continue;
			}
		}
	}

	/* Handle updating regions if required */
	if ((los && !los2) || (!los && los2) || (project && !project2) || (!project && project2))
	{
		s16b this_region_piece, next_region_piece = 0;

		for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
		{
			/* Get the region piece */
			region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

			/* Get the region */
			region_type *r_ptr = &region_list[rp_ptr->region];

			/* Get the method */
			method_type *method_ptr = &method_info[r_ptr->method];

			/* Get the next object */
			next_region_piece = rp_ptr->next_in_grid;

			/* Skip regions that don't need updating */
			if (((r_ptr->flags1 & (RE1_PROJECTION)) == 0) || ((r_ptr->flags1 & (RE1_LINGER)) != 0)) continue;

			/* Refresh the region if projection changes */
			if ((project != project2) && ((method_ptr->flags1 & (PROJECT_LOS | PROJECT_PASS)) == 0))
			{
				/* Update the region */
				region_update(rp_ptr->region);
			}
			/* Use line of sight instead */
			else if ((los != los2) && ((method_ptr->flags1 & (PROJECT_LOS)) != 0))
			{
				/* Update the region */
				region_update(rp_ptr->region);
			}
		}
	}

	/* Set the level flags based on the feature */
	set_level_flags(feat);

	/* Handle creating "glowing" terrain */
	if (!halo && halo2)
	{
		gain_attribute(y, x, 2, CAVE_XLOS, apply_halo, redraw_halo_gain);
	}

	/* Handle creating terrain lit by "daylight" */
	if (!dlit && dlit2)
	{
		gain_attribute(y, x, 1, CAVE_XLOS, apply_daylight, redraw_daylight_gain);
	}

	/* Handle creating "climb-able terrain */
	if (!climb && climb2)
	{
		gain_attribute(y, x, 2, 0, apply_climb, redraw_climb_gain);
	}

	/* Handle creating chasms */
	if (!chasm_edge && chasm_edge2)
	{
		gain_attribute(y, x, 1, 0, apply_chasm_edge, redraw_chasm_edge_gain);
	}

	/* Handle gold/items */
	if (((f_ptr->flags1 & FF1_HAS_ITEM) && !(f_ptr2->flags1 & (FF1_HAS_ITEM)))
		|| ((f_ptr->flags1 & FF1_HAS_GOLD) && !(f_ptr2->flags1 & (FF1_HAS_GOLD))))
	{
		int number = 0;
		int j;

		bool good = (f_ptr->flags3 & (FF3_DROP_GOOD)) ? TRUE : FALSE;
		bool great = (f_ptr->flags3 & (FF3_DROP_GREAT)) ? TRUE : FALSE;

		bool do_item = ((f_ptr->flags1 & FF1_HAS_ITEM) && !(f_ptr->flags2 & (FF1_HAS_ITEM)));
		bool do_gold = ((f_ptr->flags1 & FF1_HAS_GOLD) && !(f_ptr->flags2 & (FF1_HAS_GOLD)));

		object_type *i_ptr;
		object_type object_type_body;

		if (f_ptr->flags3 & (FF3_DROP_1D2)) number += damroll(1, 2);
		if (f_ptr->flags3 & (FF3_DROP_1D3)) number += damroll(1, 3);

		/* Always drop something */
		if (!number) number = 1;

		/* Opening a chest -- this forces tval_drop_idx to be set after first item selected */
		opening_chest = TRUE;

		/* Drop some objects */
		for (j = 0; j < number; j++)
		{
			/* Get local object */
			i_ptr = &object_type_body;

			/* Wipe the object */
			object_wipe(i_ptr);

			/* Make Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50)))
			{
				/* Make some gold */
				if (!make_gold(i_ptr, good, great)) continue;
			}

			/* Make Object */
			else
			{
				/* Make an object */
				if (!make_object(i_ptr, good, great)) continue;
			}

			/* No longer opening a chest */
			opening_chest = FALSE;

			/* Mark the origin */
			i_ptr->origin = ORIGIN_FEAT;
			i_ptr->origin_depth = p_ptr->depth;

			/* Drop it in the dungeon */
			drop_near(i_ptr, -1, y, x, TRUE);
		}

		/* Clear drop restriction */
		tval_drop_idx = 0;
	}
}


/*
 * Take a feature, determine what that feature becomes
 * through applying the given action.
 */
int feat_state(int feat, int action)
{
	int newfeat=feat;
	int i;

	/* Permanent stuff never gets changed */
	/* Hack -- ignore for less and more. This is required to reverse stair directions. */
	if ((f_info[feat].flags1 & FF1_PERMANENT) && (action != FS_LESS) && (action != FS_MORE)) return (feat);

	/* Set default feat */
	newfeat = f_info[feat].defaults;

	/* Get the new feature */
	for (i=0;i<MAX_FEAT_STATES;i++)
	{
		if (f_info[feat].state[i].action == action)
		{
			newfeat = f_info[feat].state[i].result;
			break;
		}
	}

	/* Paranoia */
	if (newfeat > z_info->f_max)
	{
		msg_format("Error: %d transitioning to %d by action %d out of bounds.", feat, newfeat, action);
		return (feat);
	}

	/* Change in state */
	return (newfeat);
}

/*
 * Takes a location and action and changes the feature at that
 * location through applying the given action.
 */
void cave_alter_feat(const int y, const int x, int action)
{
	int newfeat;

	/* Set old feature */
	int oldfeat = cave_feat[y][x];

	/* Get the new feat */
	newfeat = feat_state(cave_feat[y][x],action);

	/* Invisible trap */
	if (newfeat == oldfeat)
	{
		if (f_info[oldfeat].flags3 & (FF3_PICK_TRAP))
		{
			/* Pick a trap */
			pick_trap(y, x, FALSE);

			/* Update new feature */
			newfeat = cave_feat[y][x];

			/* Disturb */
			disturb(0, 0);
		}
		else if (f_info[oldfeat].flags3 & (FF3_PICK_DOOR))
		{
			/* Pick a trap */
			pick_door(y, x);

			/* Update new feature */
			newfeat = cave_feat[y][x];

			/* Disturb */
			disturb(0, 0);
		}
		else
		{
			return;
		}
	}

	/* Other stuff */
	else
	{
		/* Set the new feature */
		cave_set_feat(y,x,newfeat);
	}

	/* Notice */
	note_spot(y, x);

	/* Redraw */
	lite_spot(y, x);

}

/* Check to see whether a monster or player blocks a path */
/* Player always blocks, monster blocks if not hidden */
#define monster_block(Y, X) \
	((cave_m_idx[(Y)][(X)] < 0) || \
	((cave_m_idx[(Y)][(X)] > 0) && \
	 ((m_list[cave_m_idx[(Y)][(X)]].mflag & (MFLAG_HIDE)) == 0)))


/*
 * Determine the path taken by a projection.  -BEN-, -LM-
 *
 * Updated slightly for Unangband.
 *
 * The projection will always start one grid from the grid (y1,x1), and will
 * travel towards the grid (y2,x2), touching one grid per unit of distance
 * along the major axis, and stopping when it satisfies certain conditions
 * or has travelled the maximum legal distance of "range".  Projections
 * cannot extend further than MAX_SIGHT (at least at present).
 *
 * A projection only considers those grids which contain the line(s) of fire
 * from the start to the end point.  Along any step of the projection path,
 * either one or two grids may be valid options for the next step.  When a
 * projection has a choice of grids, it chooses that which offers the least
 * resistance.  Given a choice of clear grids, projections prefer to move
 * orthogonally.
 *
 * Also, projections to or from the character must stay within the pre-
 * calculated field of fire ("play_info & (PLAY_FIRE)").  This is a hack.
 * XXX XXX
 *
 * The path grids are saved into the grid array pointed to by "gp", and
 * there should be room for at least "range" grids in "gp".  Note that
 * due to the way in which distance is calculated, this function normally
 * uses fewer than "range" grids for the projection path, so the result
 * of this function should never be compared directly to "range".  Note
 * that the initial grid (y1,x1) is never saved into the grid array, not
 * even if the initial grid is also the final grid.  XXX XXX XXX
 *
 * We modify y2 and x2 if they are too far away, or (for PROJECT_PASS only)
 * if the projection threatens to leave the dungeon.
 *
 * The "flg" flags can be used to modify the behavior of this function:
 *    PROJECT_STOP:  projection stops when it cannot bypass a monster.
 *    PROJECT_CHCK:  projection notes when it cannot bypass a monster.
 *    PROJECT_THRU:  projection extends past destination grid
 *    PROJECT_PASS:  projection passes through walls
 *    PROJECT_MISS:  projection misses the first monster or player.
 *    PROJECT_LOS:   allow line of sight grids as well as projectable ones.
 *
 * This function returns the number of grids (if any) in the path.  This
 * may be zero if no grids are legal except for the starting one.
 */
int project_path_aux(u16b *gp, int range, int y1, int x1, int *y2, int *x2, u32b flg)
{
	int i, j, k;
	int dy, dx;
	int num, dist, octant;
	int grids = 0;
	bool line_fire;
	bool full_stop = FALSE;

	int y_a, x_a, y_b, x_b;
	int y = 0, old_y = 0;
	int x = 0, old_x = 0;

	/* Start with all lines of sight unobstructed */
	u32b bits0 = VINFO_BITS_0;
	u32b bits1 = VINFO_BITS_1;
	u32b bits2 = VINFO_BITS_2;
	u32b bits3 = VINFO_BITS_3;

	int slope_fire1 = -1, slope_fire2 = 0;

	/* Projections are either vertical or horizontal */
	bool vertical;

	/* Require projections to be strictly LOF when possible  XXX XXX */
	bool require_strict_lof = FALSE;
	bool really_require_strict_lof = FALSE;
	bool allow_los = (flg & (PROJECT_LOS)) != 0;

	/* Count of grids in LOF, storage of LOF grids */
	u16b tmp_grids[80];

	/* Count of grids in projection path */
	int step;

	/* Remember whether and how a grid is blocked */
	int blockage[2];

	/* Assume no monsters in way */
	bool monster_in_way = FALSE;

	/* Initial grid */
	s16b g0 = GRID(y1, x1);

	s16b g;

	/* Pointer to vinfo data */
	vinfo_type *p;

	/* Handle projections of zero length */
	if ((range <= 0) || ((*y2 == y1) && (*x2 == x1))) return (0);

	/* Note that the character is the source or target of the projection */
	if ((( y1 == p_ptr->py) && ( x1 == p_ptr->px)) ||
	    ((*y2 == p_ptr->py) && (*x2 == p_ptr->px)))
	{
		/* Require strict LOF */
		require_strict_lof = TRUE;

		/* Hack in a hack for trick throws */
		if ((range == 256)/* || (flg & (PROJECT_TEMP))  */)
		require_strict_lof = FALSE;
	}

	/* Get position change (signed) */
	dy = *y2 - y1;
	dx = *x2 - x1;

	/* Get distance from start to finish */
	dist = distance(y1, x1, *y2, *x2);

	/* Must stay within the field of sight XXX XXX */
	if (dist > MAX_SIGHT)
	{
		/* Always watch your (+/-) when doing rounded integer math. */
		int round_y = (dy < 0 ? -(dist / 2) : (dist / 2));
		int round_x = (dx < 0 ? -(dist / 2) : (dist / 2));

		/* Rescale the endpoints */
		dy = ((dy * (MAX_SIGHT - 1)) + round_y) / dist;
		dx = ((dx * (MAX_SIGHT - 1)) + round_x) / dist;
		*y2 = y1 + dy;
		*x2 = x1 + dx;
	}

	/* Get the correct octant */
	if (dy < 0)
	{
		/* Up and to the left */
		if (dx < 0)
		{
			/* More upwards than to the left - octant 4 */
			if (ABS(dy) > ABS(dx)) octant = 5;

			/* At least as much left as upwards - octant 3 */
			else                   octant = 4;
		}
		else
		{
			if (ABS(dy) > ABS(dx)) octant = 6;
			else                   octant = 7;
		}
	}
	else
	{
		if (dx < 0)
		{
			if (ABS(dy) > ABS(dx)) octant = 2;
			else                   octant = 3;
		}
		else
		{
			if (ABS(dy) > ABS(dx)) octant = 1;
			else                   octant = 0;
		}
	}

	/* Determine whether the major axis is vertical or horizontal */
	if ((octant == 5) || (octant == 6) || (octant == 2) || (octant == 1))
	{
		vertical = TRUE;
	}
	else
	{
		vertical = FALSE;
	}


	/* Scan the octant, find the grid corresponding to the end point */
	for (j = 1; j < VINFO_MAX_GRIDS; j++)
	{
		s16b vy, vx;

		/* Point to this vinfo record */
		p = &vinfo[j];

		/* Extract grid value */
		g = g0 + p->grid[octant];

		/* Get axis coordinates */
		vy = GRID_Y(g);
		vx = GRID_X(g);

		/* Allow for negative values XXX XXX XXX */
		if (vy > 256 * 127) vy = vy - (256 * 256);
		if (vx > x1 + 127)
		{
			vy++;
			vx = vx - 256;
		}

		/* Require that grid be correct */
		if ((vy != *y2) || (vx != *x2)) continue;

		/* Store lines of fire */
		slope_fire1 = p->slope_fire_index1;
		slope_fire2 = p->slope_fire_index2;

		break;
	}

	/* Note failure XXX XXX */
	if (slope_fire1 == -1) return (0);

	/* Scan the octant, collect all grids having the correct line of fire */
	for (j = 1; j < VINFO_MAX_GRIDS; j++)
	{
		line_fire = FALSE;

		/* Point to this vinfo record */
		p = &vinfo[j];

		/* See if any lines of sight pass through this grid */
		if (!((bits0 & (p->bits_0)) ||
			   (bits1 & (p->bits_1)) ||
			   (bits2 & (p->bits_2)) ||
			   (bits3 & (p->bits_3))))
		{
			continue;
		}

		/*
		 * Extract grid value.  Use pointer shifting to get the
		 * correct grid offset for this octant.
		 */
		g = g0 + *((s16b*)(((byte*)(p)) + (octant * 2)));

		y = GRID_Y(g);
		x = GRID_X(g);

		/* Must be legal (this is important) */
		if (!in_bounds_fully(y, x)) continue;

		/* Check for first possible line of fire */
		i = slope_fire1;

		/* Check line(s) of fire */
		while (TRUE)
		{
			switch (i / 32)
			{
				case 3:
				{
					if (bits3 & (1L << (i % 32)))
					{
						if (p->bits_3 & (1L << (i % 32))) line_fire = TRUE;
					}
					break;
				}
				case 2:
				{
					if (bits2 & (1L << (i % 32)))
					{
						if (p->bits_2 & (1L << (i % 32))) line_fire = TRUE;
					}
					break;
				}
				case 1:
				{
					if (bits1 & (1L << (i % 32)))
					{
						if (p->bits_1 & (1L << (i % 32))) line_fire = TRUE;
					}
					break;
				}
				case 0:
				{
					if (bits0 & (1L << (i % 32)))
					{
						if (p->bits_0 & (1L << (i % 32))) line_fire = TRUE;
					}
					break;
				}
			}

			/* We're done if no second LOF exists, or when we've checked it */
			if ((!slope_fire2) || (i == slope_fire2)) break;

			/* Check second possible line of fire */
			i = slope_fire2;
		}

		/* This grid contains at least one of the lines of fire */
		if (line_fire)
		{
			/* Do not accept breaks in the series of grids  XXX XXX */
			if ((grids) && ((ABS(y - old_y) > 1) || (ABS(x - old_x) > 1)))
			{
				break;
			}

			/* Optionally, require strict line of fire */
			if ((!require_strict_lof) || (play_info[y][x] & (PLAY_FIRE))
				|| ((allow_los) && (play_info[y][x] & (PLAY_VIEW))))
			{
				/* Hack - only end on a LOF exception */
				if (really_require_strict_lof)
				{
					grids--;
					really_require_strict_lof = FALSE;
				}
				
				/* Store grid value */
				tmp_grids[grids++] = g;
			}
			/* Really require strict lof. We allow exceptions for a final
			 * grid which blocks line of fire. */
			else if ((require_strict_lof) && !(really_require_strict_lof)
					&& !(cave_project_bold(y, x))
							&& (!(allow_los) || !(cave_los_bold(y, x))))
			{
				/* Store grid value */
				tmp_grids[grids++] = g;
				
				/* Great variable name */
				really_require_strict_lof = TRUE;
			}

			/* Remember previous coordinates */
			old_y = y;
			old_x = x;
		}

		/*
		 * Handle wall (unless ignored).  Walls can be in a projection path,
		 * but the path cannot pass through them.
		 */
		if (!(flg & (PROJECT_PASS)) && !cave_project_bold(y, x)
				&& ((!allow_los) || !(cave_los_bold(y, x))))
		{
			/* Clear any lines of sight passing through this grid */
			bits0 &= ~(p->bits_0);
			bits1 &= ~(p->bits_1);
			bits2 &= ~(p->bits_2);
			bits3 &= ~(p->bits_3);
		}
	}

	/* Scan the grids along the line(s) of fire */
	for (step = 0, j = 0; j < grids;)
	{
		/* Get the coordinates of this grid */
		y_a = GRID_Y(tmp_grids[j]);
		x_a = GRID_X(tmp_grids[j]);

		/* Get the coordinates of the next grid, if legal */
		if (j < grids - 1)
		{
			y_b = GRID_Y(tmp_grids[j+1]);
			x_b = GRID_X(tmp_grids[j+1]);
		}
		else
		{
			y_b = -1;
			x_b = -1;
		}

		/*
		 * We always have at least one legal grid, and may have two.  Allow
		 * the second grid if its position differs only along the minor axis.
		 */
		if (vertical ? y_a == y_b : x_a == x_b) num = 2;
		else                                    num = 1;
		
		/* Scan one or both grids */
		for (i = 0; i < num; i++)
		{
			blockage[i] = 0;

			/* Get the coordinates of this grid */
			y = (i == 0 ? y_a : y_b);
			x = (i == 0 ? x_a : x_b);

			/* Determine perpendicular distance */
			k = (vertical ? ABS(x - x1) : ABS(y - y1));

			/* Hack -- Check maximum range */
			if ((i == num-1) && (step + (k >> 1)) >= range-1)
			{
				/* End of projection */
				full_stop = TRUE;
			}

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((y == *y2) && (x == *x2))
				{
					/* End of projection */
					
					full_stop = TRUE;
				}
			}

			/* Usually stop at wall grids */
			if (!(flg & (PROJECT_PASS)))
			{
				if ((!cave_project_bold(y, x))
						&& ((!allow_los) || !(cave_los_bold(y, x))))
				{
					blockage[i] = 2;
				}
			}

			/* If we don't stop at wall grids, we must explicitly check legality */
			else if (!in_bounds_fully(y, x))
			{
				/* End of projection */
				full_stop = TRUE;
				blockage[i] = 3;
			}

			/* Try to avoid monsters/players between the endpoints */
			if ((monster_block(y,x)) && (blockage[i] < 2))
			{
				if		(flg & (PROJECT_MISS)) flg &= ~(PROJECT_MISS);
				else if (flg & (PROJECT_STOP)) blockage[i] = 2;
				else if (flg & (PROJECT_CHCK)) blockage[i] = 1;
			}
		}

		/* Pick the first grid if possible, the second if necessary.
		 * Hack -- try to pick the blocked path if we are require_strict_lof
		 * and are at the end of the path. This helps ensure we end on a
		 * blocked grid. */
		if ((num == 1) || (blockage[0] <= blockage[1])
			|| ((require_strict_lof) && (blockage[1] < 1) && (j >= grids - num)))
		{
			/* Store the first grid, advance */
			if (blockage[0] < 3) gp[step++] = tmp_grids[j];

			/* Blockage of 2 or greater means the projection ends */
			if (blockage[0] >= 2) break;

			/* Blockage of 1 means a monster bars the path */
			if (blockage[0] == 1)
			{
				/* Endpoints are always acceptable */
				if ((y != *y2) || (x != *x2)) monster_in_way = TRUE;
			}

			/* Handle end of projection */
			if (full_stop) break;
		}
		else
		{
			/* Store the second grid, advance */
			if (blockage[1] < 3) gp[step++] = tmp_grids[j + 1];

			/* Blockage of 2 or greater means the projection ends */
			if (blockage[1] >= 2) break;

			/* Blockage of 1 means a monster bars the path */
			if (blockage[1] == 1)
			{
				/* Endpoints are always acceptable */
				if ((y != *y2) || (x != *x2)) monster_in_way = TRUE;
			}

			/* Handle end of projection */
			if (full_stop) break;
		}

		/*
		 * Hack -- If we require orthogonal movement, but are moving
		 * diagonally, we have to plot an extra grid.  XXX XXX
		 */
		if (((flg & (PROJECT_ORTH)) != 0) && (step > 1))
		{
			/* Get grids for this projection step and the last */
			y_a = GRID_Y(gp[step-1]);
			x_a = GRID_X(gp[step-1]);

			y_b = GRID_Y(gp[step-2]);
			x_b = GRID_X(gp[step-2]);

			/* The grids differ along both axis -- we moved diagonally */
			if ((y_a != y_b) && (x_a != x_b))
			{
				/* Get locations for the connecting grids */
				int y_c = y_a;
				int x_c = x_b;
				int y_d = y_b;
				int x_d = x_a;

				/* Back up one step */
				step--;

				/* Assume both grids are available */
				blockage[0] = 0;
				blockage[1] = 0;

				/* Hack -- Check legality */
				if (!in_bounds_fully(y_c, x_c)) blockage[0] = 2;
				if (!in_bounds_fully(y_d, x_d)) blockage[1] = 2;

				/* Usually stop at wall grids */
				if (!(flg & (PROJECT_PASS)))
				{
					if ((!cave_project_bold(y_c, x_c))
						&& ((!allow_los) || !(cave_los_bold(y_c, x_c))))
					{
						blockage[0] = 2;
					}
					if ((!cave_project_bold(y_d, x_d))
							&& ((!allow_los) || !(cave_los_bold(y_d, x_d))))
					{
						blockage[1] = 2;
					}
				}

				/* Try to avoid non-initial monsters/players */
				if (monster_block(y_c,x_c))
				{
					if      (flg & (PROJECT_STOP)) blockage[0] = 2;
					else if (flg & (PROJECT_CHCK)) blockage[0] = 1;
				}
				if (monster_block(y_d, x_d))
				{
					if      (flg & (PROJECT_STOP)) blockage[1] = 2;
					else if (flg & (PROJECT_CHCK)) blockage[1] = 1;
				}

				/* Both grids are blocked -- we have to stop now */
				if ((blockage[0] >= 2) && (blockage[1] >= 2)) break;

				/* Accept the first grid if possible, the second if necessary */
				if (blockage[0] <= blockage[1]) gp[step++] = GRID(y_c, x_c);
				else                            gp[step++] = GRID(y_d, x_d);

				/* Re-insert the original grid, take an extra step */
				gp[step++] = GRID(y_a, x_a);

				/* Increase range to accommodate this extra step */
				range++;
			}
		}

		/* Advance to the next unexamined LOF grid */
		j += num;
	}

	/* Hack - if we are checking for require_strict_lof, it is possible
	 * that the projection could stop without hitting a CAVE_XLOF square.
	 * This might occur because a nearby grid created a projection shadow
	 * without directly impinging on the projection path. */
	if ((require_strict_lof) && !(monster_in_way) && !(full_stop)
			&& ((p_ptr->py != GRID_Y(gp[step -1])) || (p_ptr->px != GRID_X(gp[step -1]))))
	{
		/* Final grid is projectable */
		if (cave_project_bold(GRID_Y(gp[step -1]), GRID_X(gp[step -1])))
		{
			/* Just warn for the moment */
			if (cheat_xtra) msg_print("Path does not end correctly.");
		}
	}
	/* For some bizarre reason, if we modify this the original path gets recomputed
	 * with the modified destination. Not sure why. */
#if 0	
	/* Accept last grid as the new endpoint */
	*y2 = GRID_Y(gp[step -1]);
	*x2 = GRID_X(gp[step -1]);
#endif
	/* Return count of grids in projection path */
	if (monster_in_way) return (-step);
	else return (step);
}


/*
 * Wrapper for the above function due to the Sangband return value weirdness
 * that I introduced.
 */
int project_path(u16b *gp, int range, int y1, int x1, int *y2, int *x2, u32b flg)
{
	return(ABS(project_path_aux(gp, range, y1, x1, y2, x2, flg)));
}


/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, using the "project_path()" function to check
 * the projection path.
 *
 * Accept projection flags, and pass them onto "project_path()".
 *
 * Note that no grid is ever "projectable()" from itself.
 * This function is used to determine if the player can (easily) target
 * a given grid, if a monster can target the player, and if a clear shot
 * exists from monster to player.
 */

byte projectable(int y1, int x1, int y2, int x2, u32b flg)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x;

	int grid_n = 0;
	u16b grid_g[512];

	int old_y2 = y2;
	int old_x2 = x2;

	bool allow_los = (flg & (PROJECT_LOS)) != 0;


	/* We do not have permission to pass through walls */
	if (!(flg & (PROJECT_WALL | PROJECT_PASS)))
	{
		/* The character is the source of the projection */
		if ((y1 == py) && (x1 == px))
		{
			/* Require that destination be in line of fire */
			if (!(play_info[y2][x2] & (PLAY_FIRE)) || (allow_los &&
					(play_info[y2][x2] & (PLAY_VIEW)))) return (PROJECT_NO);
		}

		/* The character is the target of the projection */
		else if ((y2 == py) && (x2 == px))
		{
			/* Require that source be in line of fire */
			if (!(play_info[y1][x1] & (PLAY_FIRE)) || (allow_los &&
					(play_info[y1][x1] & (PLAY_VIEW)))) return (PROJECT_NO);
		}
	}

	/* Check the projection path */
	grid_n = project_path_aux(grid_g, MAX_RANGE, y1, x1, &y2, &x2, flg);

	/* No grid is ever projectable from itself */
	if (!grid_n) return (PROJECT_NO);

	/* Final grid.  As grid_n may be negative, use absolute value.  */
	y = GRID_Y(grid_g[ABS(grid_n) - 1]);
	x = GRID_X(grid_g[ABS(grid_n) - 1]);

	/* May not end in an unrequested grid */
	if ((y != old_y2) || (x != old_x2)) return (PROJECT_NO);

	/* May not end in a wall */
	if (!cave_project_bold(y, x)
			&& ((!allow_los) || !(cave_los_bold(y, x)))) return (PROJECT_NO);

	/* Promise a clear bolt shot if we have verified that there is one */
	if ((flg & (PROJECT_STOP)) || (flg & (PROJECT_CHCK)))
	{
		/* Positive value for grid_n mean no obstacle was found. */
		if (grid_n > 0) return (PROJECT_CLEAR);
	}

	/* Assume projectable, but make no promises about clear shots */
	return (PROJECT_NOT_CLEAR);

}


/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "lof()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * The m parameter implies what flags we check line of sight with.
 */
void scatter(int *yp, int *xp, int y, int x, int d, int m)
{
	int nx, ny;

	int tries;

	/* Pick a location */
	for (tries = 0; tries < 100; tries++)
	{
		/* Pick a new location */
		ny = rand_spread(y, d);
		nx = rand_spread(x, d);

		/* Ignore annoying locations */
		if (!in_bounds_fully(y, x)) continue;

		/* Ignore "excessively distant" locations */
		if ((d > 1) && (distance(y, x, ny, nx) > d)) continue;

		/* Require "line of fire" */
		if ((m) && (generic_los(y, x, ny, nx, m))) break;
	}

	/* Failed? */
	if (tries >= 100)
	{
		ny = y;
		nx = x;
	}

	/* Save the location */
	(*yp) = ny;
	(*xp) = nx;
}






/*
 * Track a new monster
 */
void health_track(int m_idx)
{
	/* Track a new guy */
	p_ptr->health_who = m_idx;

	/* Redraw (later) */
	p_ptr->redraw |= (PR_HEALTH);
}



/*
 * Hack -- track the given monster race
 */
void monster_race_track(int r_idx)
{
	/* Save this monster ID */
	p_ptr->monster_race_idx = r_idx;

	/* Window stuff */
	p_ptr->window |= (PW_MONSTER);
}


/*
 * Something has happened to disturb the player.
 *
 * The first arg indicates a major disturbance, which affects search.
 *
 * The second arg indicates a major disturbance, which wakes the player.
 *
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(int stop_search, int wake_up)
{
	/* Cancel auto-commands */
	/* p_ptr->command_new = 0; */

	/* Cancel repeated commands */
	if (p_ptr->command_rep)
	{
		/* Cancel */
		p_ptr->command_rep = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel Resting */
	if (p_ptr->resting)
	{
		/* Cancel */
		p_ptr->resting = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel running */
	if (p_ptr->running)
	{
		/* Cancel */
		p_ptr->running = 0;

 		/* Check for new panel if appropriate */
 		if (center_player) verify_panel();

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
	}

	/* Cancel searching if requested */
	if (stop_search && p_ptr->searching)
	{
		/* Cancel */
		p_ptr->searching = FALSE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Wake the player if requested */
	if (wake_up && p_ptr->timed[TMD_PSLEEP])
	{
		/* Cancel */
		set_psleep(0);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Flush the input if requested */
	if (flush_disturb) flush();

	/* Last disturbed */
	p_ptr->last_disturb = turn;

}


/*
 *  Sets various level flags at initialisation
 */
void init_level_flags(void)
{
	town_type *t_ptr = &t_info[p_ptr->dungeon];
	dungeon_zone *zone=&t_info[0].zone[0];
	int guard;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Get the guardian */
	guard = actual_guardian(zone->guard, p_ptr->dungeon, zone - t_ptr->zone);

	/* Set base level flags */
	level_flag = zone->flags1;

	/* Daylight if surface */
	if ((level_flag & (LF1_DAYLIGHT)) && (turn % (10L * TOWN_DAWN) >= (10L * TOWN_DAWN) / 2))
	{
		level_flag &= ~(LF1_DAYLIGHT);
	}

	/* Add 'guardian' level flags */
	if (guard && (r_info[guard].max_num > 0)) level_flag |= (LF1_GUARDIAN);

	/* Allow downstairs to be created on bottom level if the quest guardian has been killed */
	if (((level_flag & (LF1_SURFACE)) == 0) && (p_ptr->depth == max_depth(p_ptr->dungeon)) &&
		(t_ptr->quest_opens) && (r_info[t_ptr->quest_monster].max_num == 0))
	{
		level_flag |= (LF1_MORE);
	}
}
