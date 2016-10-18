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
 *
 */

/*
 * Code cleanup -- Pete Mack 02/2007
 * Use proper function tables and database methodology.
 * Tables are now tables, not multiline conditionals.
 * Joins are now relational, not ad hoc.
 * Function tables are used for iteration where reasonable. (C-style class model)
*/

#include "angband.h"
#include "option.h"


#define CURS_UNKNOWN (0)
#define CURS_KNOWN (1)
static const byte curs_attrs[2][2] = {
	{TERM_SLATE, TERM_BLUE},		/* Flavor is unknown */
	{TERM_WHITE, TERM_L_BLUE}		/* Object is IDed */
};


typedef struct {
	int maxnum; /* Maximum possible item count for this class */
	bool easy_know; /* Items don't need to be IDed to recognize membership */

	const char *(*name)(int gid); /* name of this group */

	/* Compare, in group and display order */
	/* Optional, if already sorted */
	int (*gcomp)(const void *, const void *); /* Compare GroupIDs of two oids */
	int (*group)(int oid); /* Group ID for of an oid */
	bool (*aware)(object_type *obj); /* Object is known sufficiently for group */

} group_funcs;

typedef struct {

	/* Display object label (possibly with cursor) at given screen location. */
	void (*display_label)(int col, int row, bool cursor, int oid);

	void (*lore)(int oid); /* Dump known lore to screen*/

	/* Required only for objects with modifiable display attributes */
	/* Unknown 'flavors' return flavor attributes */
	char *(*xchar)(int oid); /* get character attr for OID (by address) */
	byte *(*xattr)(int oid); /* get color attr for OID (by address) */

	/* Required only for manipulable (ordinary) objects */
	/* Address of inscription.  Unknown 'flavors' return null  */
	u16b *(*note)(int oid);

	/* Required only for features */
	/* Address of flags3 for manipulation (adding and removing lite/wall/door/item) */
	u32b *(*flags3)(int oid);

} member_funcs;


/* Helper class for generating joins */
typedef struct join {
	int oid;
	int gid;
} join_t;

/* A default group-by */
static join_t *default_join;
static int default_join_cmp(const void *a, const void *b)
{
	join_t *ja = &default_join[*(int*)a];
	join_t *jb = &default_join[*(int*)b];
	int c = ja->gid - jb->gid;
	if (c) return c;
	return ja->oid - jb->oid;
}
static int default_group(int oid) { return default_join[oid].gid; }


static int *obj_group_order = 0;
/*
 * Description of each monster group.
 */
static cptr monster_group_text[] =
{
	"Uniques",
	"Amphibians/Fish",
	"Ants",
	"Bats",
	"Birds",
	"Canines",
	"Centipedes",
	"Demons",
	"Dragons",
	"Elementals",
	"Elves",
	"Eyes/Beholders",
	"Felines",
	"Ghosts",
	"Giants",
	"Goblins",
	"Golems",
	"Harpies/Hybrids",
	"Hobbits/Dwarves",
	"Hydras",
	"Icky Things",
	"Insects",
	"Jellies",
	"Killer Beetles",
	"Lichs",
	"Maiar",
	"Mimics",
	"Molds",
	"Mushroom Patches",
	"Mosses",
	"Nagas",
	"Nightsbane",
	"Ogres",
	"Orcs",
	"People",
	"Quadrupeds",
	"Reptiles",
	"Rodents",
	"Scorpions/Spiders",
	"Skeletons",
	"Snakes",
	"Trees",
	"Trolls",
	"Vampires",
	"Vortices",
	"Wights/Wraiths",
	"Worms/Worm Masses",
	"Xorns/Xarens",
	"Yeti",
	"Zephyr Hounds",
	"Zombies",
	NULL
};

/*
 * Symbols of monsters in each group. Note the "Uniques" group
 * is handled differently.
 */
static cptr monster_group_char[] =
{
	(char *) -1L,
	"F",
	"a",
	"b",
	"B",
	"C",
	"c",
	"Uu",
	"ADd",
	"E",
	"l",
	"e",
	"f",
	"G",
	"P",
	"k",
	"g",
	"H",
	"h",
	"y",
	"i",
	"I",
	"j",
	"K",
	"L",
	"M",
	"$!?=.|~[]",
	"m",
	",",
	";",
	"n",
	"N",
	"O",
	"o",
	"pqt",
	"Q",
	"R",
	"r",
	"S",
	"s",
	"J",
	":",
	"T",
	"V",
	"v",
	"W",
	"w",
	"X",
	"Y",
	"Z",
	"z",
	NULL
};

/*
 * Description of each feature group.
 */
const cptr feature_group_text[] =
{
	"Floors",
	"Secrets",
	"Traps",
	"Doors",
	"Stairs",
	"Walls",
	"Streamers",
	"Stores",
	"Chests",
	"Furnishings",
	"Bridges",
	"Water",
	"Lava",
	"Ice",
	"Acid",
	"Oil",
	"Chasms",
	"Sand/Earth",
	"Ground",
	"Trees/Plants",
	"Smoke/Fire",
	"Other",
	NULL
};



/* Useful method declarations */
static void display_visual_list(int col, int row, int height, int width,
								byte attr_top, char char_left);

static void browser_mouse(key_event ke, int *column, int *grp_cur, int grp_cnt,
							int *list_cur, int list_cnt, int col0, int row0,
							int grp0, int list0, int *delay);

static void browser_cursor(char ch, int *column, int *grp_cur, int grp_cnt,
						   int *list_cur, int list_cnt);

static bool visual_mode_command(key_event ke, bool *visual_list_ptr,
				int height, int width,
				byte *attr_top_ptr, char *char_left_ptr,
				byte *cur_attr_ptr, char *cur_char_ptr,
				int col, int row, int *delay);

static void place_visual_list_cursor(int col, int row, byte a,
									byte c, byte attr_top, byte char_left);
/*
 *  Clipboard variables for copy&paste in visual mode
 */
static byte attr_idx = 0;
static byte char_idx = 0;

/*
 * Return a specific ordering for the features
 */
int feat_order(int feat)
{
	feature_type *f_ptr = &f_info[feat];

	if (f_ptr->flags1 & (FF1_STAIRS)) return (4);
	else if (f_ptr->flags1 & (FF1_ENTER)) return (7);
	else if (f_ptr->flags3 & (FF3_CHEST)) return (8);
	else if (f_ptr->flags1 & (FF1_DOOR)) return (3);
	else if (f_ptr->flags1 & (FF1_TRAP)) return (2);
	else if (f_ptr->flags3 & (FF3_ALLOC)) return (9);
	else if (f_ptr->flags1 & (FF1_FLOOR)) return (0);
	else if (f_ptr->flags1 & (FF1_STREAMER)) return (6);
	else if (f_ptr->flags2 & (FF2_COVERED)) return (10);
	else if (f_ptr->flags2 & (FF2_LAVA)) return (12);
	else if (f_ptr->flags2 & (FF2_ICE)) return (13);
	else if (f_ptr->flags2 & (FF2_CAN_DIG)) return (17);
	else if (f_ptr->flags2 & (FF2_ACID)) return (14);
	else if (f_ptr->flags2 & (FF2_OIL)) return (15);
	else if ((f_ptr->flags2 & (FF2_WATER | FF2_CAN_SWIM))) return (11);
	else if (f_ptr->flags2 & (FF2_CHASM)) return (16);
	else if (f_ptr->flags1 & (FF1_SECRET)) return (1);
	else if (f_ptr->flags3 & (FF3_LIVING)) return (19);
	else if (f_ptr->flags3 & (FF3_SPREAD | FF3_ADJACENT | FF3_INSTANT | FF3_TIMED)) return (20);
	else if (f_ptr->flags1 & (FF1_WALL)) return (5);
	else if (f_ptr->flags3 & (FF3_GROUND)) return (18);

	return (21);
}


/* Emit a 'graphical' symbol and a padding character if appropriate */
static void big_pad(int col, int row, byte a, byte c)
{
	Term_putch(col, row, a, c);
	if(!use_bigtile) return;
	if (a &0x80) Term_putch(col+1, row, 255, -1);
	else Term_putch(col+1, row, 1, ' ');
}

/*
 * Interact with inscriptions.
 * Create a copy of an existing quark, except if the quark has '=x' in it,
 * If an quark has '=x' in it remove it from the copied string, otherwise append it where 'x' is ch.
 * Return the new quark location.
 */
static int auto_note_modify(int note, char ch)
{
	char tmp[80];

	cptr s;

	/* Paranoia */
	if (!ch) return(note);

	/* Null length string to start */
	tmp[0] = '\0';

	/* Inscription */
	if (note)
	{

		/* Get the inscription */
		s = quark_str(note);

		/* Temporary copy */
		my_strcpy(tmp,s,sizeof(tmp));

		/* Process inscription */
		while (s)
		{

			/* Auto-pickup on "=g" */
			if (s[1] == ch)
			{

				/* Truncate string */
				tmp[strlen(tmp)-strlen(s)] = '\0';

				/* Overwrite shorter string */
				my_strcat(tmp,s+2,80);

				/* Create quark */
				return(quark_add(tmp));
			}

			/* Find another '=' */
			s = strchr(s + 1, '=');
		}
	}

	/* Append note */
	my_strcat(tmp,format("=%c",ch),80);

	/* Create quark */
	return(quark_add(tmp));
}

/*
 * Display a list with a cursor
 */
static void display_group_list(int col, int row, int wid, int per_page,
					int start, int max, int cursor, const cptr group_text[])
{
	int i, pos;

	/* Display lines until done */
	for (i = 0, pos = start; i < per_page && pos < max; i++, pos++)
	{
		char buffer[20];
		byte attr = curs_attrs[CURS_KNOWN][cursor == pos];

		/* Erase the line */
		Term_erase(col, row + i, wid);

		/* Display it (width should not exceed 19) */
		strncpy(buffer, group_text[pos], 19);
		buffer[19] = 0;
		c_put_str(attr, buffer, row + i, col);
	}
	/* Wipe the rest? */
}

/*
 * Display the members of a list.
 * Aware of inscriptions, wizard information, and string-formatted visual data.
 * label function must display actual visuals, to handle illumination, etc
 */
static void display_member_list(int col, int row, int wid, int per_page,
		int start, int o_count, int cursor, int object_idx [],
		member_funcs o_funcs)
{
	int i, pos;

	(void)wid;

	for(i = 0, pos = start; i < per_page && pos < o_count; i++, pos++) {
		int oid = object_idx[pos];
		byte attr = curs_attrs[CURS_KNOWN][cursor == oid];

		/* Print basic label */
		o_funcs.display_label(col, row + i, pos == cursor, oid);

		/* Show inscription, if applicable, aware and existing */
		if(o_funcs.note && o_funcs.note(oid) && *o_funcs.note(oid)) {
			c_put_str(TERM_YELLOW,quark_str(*o_funcs.note(oid)), row+i, 65);
		}

		if (p_ptr->wizard)
			c_put_str(attr, format("%d", oid), row, 60);

		/* Do visual mode */
		if(per_page == 1 && o_funcs.xattr) {
			char c = *o_funcs.xchar(oid);
			byte a = *o_funcs.xattr(oid);
			c_put_str(attr, format((c & 0x80) ? "%02x/%02x" : "%02x/%d", a, c), row + i, 60);
		}
	}

    /* Clear remaining lines */
    for (; i < per_page; i++)
    {
        Term_erase(col, row + i, 255);
    }
}


/*
 * Interactive group by.
 * Recognises inscriptions, graphical symbols, lore
 * Some more enlightment by Pete Mac:

The basic design of this command is a database 'GROUP BY'
using a merge join.

First, ensure both lists are sorted by group id (gid).  This
is the 'primary key' of the grouping relation, ie dungeon #,
monster class, etc.
This sort is generally implicit, because the grouping
relations are in constructed in order.

For the dependent relation (objects, oid), some relations
can be defined in order, if group_id can be constructed
directly from oid. Otherwise, use qsort, with gid for the
comparator.  For objects with the same group, order by
preferred order.  (Like 'd' before 'D', then lower depth to
higher depth.)

Then do a nested loop, with the outer iterating the grouping
relation, and the inner the dependent relation.  Display
them all together.  This is implicit in the way the menus
are set up.

For more information, check a database book like (Jim) Gray.

 */
static void display_knowledge_start_at(
	const char *title, int *obj_list, int o_count,
	group_funcs g_funcs, member_funcs o_funcs,
	const char *otherfields, int g_cur)
{
	/* maximum number of groups to display */
	int max_group = g_funcs.maxnum < o_count ? g_funcs.maxnum : o_count ;

	/* This could (should?) be (void **) */
	int *g_list, *g_offset;

	const char **g_names;

	int g_name_len = 8;  /* group name length, minumum is 8 */

	int grp_cnt = 0; /* total number groups */

	int grp_old = -1, grp_top = 0; /* group list positions */
	int o_cur = 0, object_top = 0; /* object list positions */
	int g_o_count = 0; /* object count for group */
	int o_first = 0, g_o_max = 0; /* group limits in object list */
	int oid = -1, old_oid = -1;  /* object identifiers */

	/* display state variables */
	bool visual_list = FALSE;
	byte attr_top = 0;
	char char_left = 0;
	int note_idx = 0;

	int delay = 0;
	int column = 0;

	bool flag = FALSE;
	bool redraw = TRUE;

	int browser_rows;
	int wid, hgt;
	int i;
	int prev_g = -1;

	/* Get size */
	Term_get_size(&wid, &hgt);
	browser_rows = hgt - 8;

	/* Do the group by. ang_sort only works on (void **) */
	/* Maybe should make this a precondition? */
	if(g_funcs.gcomp)
		qsort(obj_list, o_count, sizeof(*obj_list), g_funcs.gcomp);

	g_list = C_ZNEW(max_group+1, int);
	g_offset = C_ZNEW(max_group+1, int);

	for(i = 0; i < o_count; i++) {
		if(prev_g != g_funcs.group(obj_list[i])) {
			prev_g = g_funcs.group(obj_list[i]);
			g_offset[grp_cnt] = i;
			g_list[grp_cnt++] = prev_g;
		}
	}
	g_offset[grp_cnt] = o_count;
	g_list[grp_cnt] = -1;


	/* The compact set of group names, in display order */
	g_names = (const char**)C_ZNEW(grp_cnt, const char **);
	for (i = 0; i < grp_cnt; i++) {
		int len;
		g_names[i] = g_funcs.name(g_list[i]);
		len = strlen(g_names[i]);
		if(len > g_name_len) g_name_len = len;
	}
	if(g_name_len >= 19) g_name_len = 19;

	while ((!flag) && (grp_cnt))
	{
		key_event ke;

		if (redraw)
		{
			clear_from(0);
			/* Hack: This could be cleaner */
			prt( format("Knowledge - %s", title), 2, 0);
			prt( "Group", 4, 0);
			prt("Name", 4, g_name_len + 3);
			move_cursor(4, 65);
			if(o_funcs.note)
				Term_addstr(-1, TERM_WHITE, "Inscribed ");
			if(otherfields)
				Term_addstr(-1, TERM_WHITE, otherfields);

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			for (i = 0; i < browser_rows; i++)
			{
				Term_putch(g_name_len + 1, 6 + i, TERM_WHITE, '|');
			}

			redraw = FALSE;
		}

		/* Scroll group list */
		if (g_cur < grp_top) grp_top = g_cur;
		if (g_cur >= grp_top + browser_rows) grp_top = g_cur - browser_rows + 1;
		if (grp_top + browser_rows >= grp_cnt) grp_top = grp_cnt - browser_rows;
		if(grp_top < 0) grp_top = 0;

		if(g_cur != grp_old) {
			o_first = o_cur = g_offset[g_cur];
			object_top = o_first;
			g_o_count = g_offset[g_cur+1] - g_offset[g_cur];
			g_o_max = g_offset[g_cur+1];
			grp_old = g_cur;
			old_oid = -1;

			/* Tweak the starting object position
			if(init_obj >= 0) {
				o_cur = init_obj - g_offset[g_cur];
				assert(OK(o_cur));
				init_obj = -1;
			}
			else o_cur = 0;

			object_menu.cursor = o_cur; */
		}

		/* Display a scrollable list of groups */
		display_group_list(0, 6, g_name_len, browser_rows,
										grp_top, grp_cnt, g_cur, g_names);

		/* Scroll object list */
		if(o_cur >= g_o_max) o_cur = g_o_max-1;
		if(o_cur < o_first) o_cur = o_first;
		if (o_cur < object_top) object_top = o_cur;
		if (o_cur >= object_top + browser_rows)
			object_top = o_cur - browser_rows + 1;
		if (object_top + browser_rows >= g_o_max)
			object_top = g_o_max - browser_rows;
		if(object_top < o_first) object_top = o_first;

		oid = obj_list[o_cur];

		if(!visual_list) {
			/* Display a list of objects in the current group */
			display_member_list(g_name_len + 3, 6, g_name_len, browser_rows,
								object_top, g_o_max, o_cur, obj_list, o_funcs);
		}
		else
		{
			/* Edit 1 group member */
			object_top = o_cur;
			/* Display a single-row list */
			display_member_list(g_name_len + 3, 6, g_name_len, 1,
								o_cur, g_o_max, o_cur, obj_list, o_funcs);
			/* Display visual list below first object */
			display_visual_list(g_name_len + 3, 7, browser_rows-1,
									wid - (g_name_len + 3), attr_top, char_left);
		}
		/* Prompt */
		{
			const char *pedit = (!o_funcs.xattr) ? "" :
				(!(attr_idx|char_idx) ? ", 'c' to copy" : ", 'c', 'p' to paste");

			const char *pnote = (!o_funcs.note || !o_funcs.note(oid)) ? "" :
							((note_idx) ? ", '\\' to re-inscribe"  :  "");

			const char *pnote1 = (!o_funcs.note || !o_funcs.note(oid)) ? "" :
								", '{', '}', 'k', 'g', ...";

			const char *pvs = (!o_funcs.xattr) ? ", " : ", 'v' for visuals, ";

			if(visual_list)
				prt(format("<dir>, 'r' to recall, ENTER to accept%s, ESC to exit", pedit), hgt-1, 0);
			else
				prt(format("<dir>, 'r' to recall%sESC%s%s%s",
											pvs, pedit, pnote, pnote1), hgt-1, 0);
		}

		handle_stuff();

		if (visual_list)
		{
			place_visual_list_cursor(g_name_len + 3, 7, *o_funcs.xattr(oid),
									*o_funcs.xchar(oid), attr_top, char_left);
		}
		else if (!column)
		{
			Term_gotoxy(0, 6 + (g_cur - grp_top));
		}
		else
		{
			Term_gotoxy(g_name_len + 3, 6 + (o_cur - object_top));
		}

		if (delay)
		{
			/* Force screen update */
			Term_fresh();

			/* Delay */
			Term_xtra(TERM_XTRA_DELAY, delay);

			delay = 0;
		}

		ke = inkey_ex();
		/* Do visual mode command if needed */
		if (o_funcs.xattr && o_funcs.xchar &&
			visual_mode_command(ke, &visual_list,
						browser_rows-1, wid - (g_name_len + 3),
						&attr_top, &char_left,
						o_funcs.xattr(oid), o_funcs.xchar(oid),
						g_name_len + 3, 7, &delay))
		{
			continue;
		}

		switch (ke.key)
		{

			case ESCAPE:
			{
				flag = TRUE;
				break;
			}

			case '\xff':
			{
				/* Move the cursor */
				browser_mouse(ke, &column, &g_cur, grp_cnt,
								&o_cur, g_o_max, g_name_len + 3, 6,
								grp_top, object_top, &delay);
				if (!ke.mousebutton) break;
				if(oid != obj_list[o_cur])
					break;
			}

			case 'R':
			case 'r':
			{
				/* Recall on screen */
				if(oid >= 0)
					o_funcs.lore(oid);

				redraw = TRUE;
				break;
			}

			/* HACK: make this a function.  Replace g_funcs.aware() */
			case '{':
			case '}':
			case '\\':
				/* precondition -- valid object */
				if (o_funcs.note == NULL || o_funcs.note(oid) == 0)
					break;
			{
				char note_text[80] = "";
				u16b *note = o_funcs.note(oid);
				u16b old_note = *note;

				if (ke.key == '{')
				{
					/* Prompt */
					prt("Inscribe with: ", hgt, 0);

					/* Default note */
					if (old_note)
						sprintf(note_text, "%s", quark_str(old_note));

					/* Get a filename */
					if (!askfor_aux(note_text, sizeof note_text)) continue;

					/* Set the inscription */
					*note = quark_add(note_text);

					/* Set the current default note */
					note_idx = *note;
				}
				else if (ke.key == '}')
				{
					*note = 0;
				}
				else
				{
					/* '\\' */
					*note = note_idx;
				}

				/* Process existing objects */
				for (i = 1; i < o_max; i++)
				{
					/* Get the object */
					object_type *i_ptr = &o_list[i];

					/* Skip dead or differently sourced items */
					if (!i_ptr->k_idx || i_ptr->note != old_note) continue;

					/* Not matching item */
					if (!g_funcs.group(oid) != i_ptr->tval) continue;

					/* Auto-inscribe */
					if (g_funcs.aware(i_ptr) || adult_auto)
						i_ptr->note = note_idx;
				}

				break;
			}

			/* HACK: make this a function.  Replace g_funcs.aware() */
			case 'w': case 'W':
			case 'a': case 'A':
			case 'f': case 'F':
			case 'd': case 'D':
				/* precondition -- valid feature */
				if (o_funcs.flags3 != NULL && o_funcs.flags3(oid) != 0)
			{
				u32b *flags3 = o_funcs.flags3(oid);

			switch(ke.key)
			{
			case 'a': case 'A':
			{
				if (*flags3 & (FF3_ATTR_LITE))
				{
					*flags3 &= ~(FF3_ATTR_LITE);
				}
				else
				{
					*flags3 |= (FF3_ATTR_LITE);
				}
			}

			case 'f': case 'F':
			{
				if (*flags3 & (FF3_ATTR_ITEM))
				{
					*flags3 &= ~(FF3_ATTR_ITEM);
				}
				else
				{
					*flags3 |= (FF3_ATTR_ITEM);
				}
			}

			case 'd': case 'D':
			{
				if (*flags3 & (FF3_ATTR_DOOR))
				{
					*flags3 &= ~(FF3_ATTR_DOOR);
				}
				else
				{
					*flags3 |= (FF3_ATTR_DOOR);
				}
			}

			case 'w': case 'W':
			{
				if (*flags3 & (FF3_ATTR_WALL))
				{
					*flags3 &= ~(FF3_ATTR_WALL);
				}
				else
				{
					*flags3 |= (FF3_ATTR_WALL);
				}
			}
			}
			break;
			}
			default:
			{
				/* Move the cursor; disable roguelike keyset. */
				int omode = rogue_like_commands;
				rogue_like_commands = FALSE;
				if(target_dir(ke.key)) {
					browser_cursor(ke.key, &column, &g_cur, grp_cnt,
									&o_cur, g_o_max);
				}
				else if(o_funcs.note && o_funcs.note(oid)) {
					note_idx = auto_note_modify(*o_funcs.note(oid), ke.key);
					*o_funcs.note(oid) = note_idx;
				}
				rogue_like_commands = omode;
				break;
			}
		}
	}

	/* Prompt */
	if (!grp_cnt)
			prt(format("No %s known.", title), 15, 0);

	FREE(g_names);
	FREE(g_offset);
	FREE(g_list);
}

static void display_knowledge(const char *title, int *obj_list, int o_count,
										group_funcs g_funcs, member_funcs o_funcs,
										const char *otherfields)
{
	display_knowledge_start_at(title, obj_list, o_count,
										g_funcs, o_funcs,
										otherfields, 0);
}

/*
 * Display visuals.
 */
static void display_visual_list(int col, int row, int height, int width, byte attr_top, char char_left)
{
	int i, j;

	/* Clear the display lines */
	for (i = 0; i < height; i++)
	{
		Term_erase(col, row + i, width);
	}

	/* Triple tile uses double height and width width */
	if (use_trptile)
	{
		width /= (use_bigtile ? 6 : 3);
		height /= 3;
	}
	/* Double tile uses double height and width width */
	else if (use_dbltile)
	{
		width /= (use_bigtile ? 4 : 2);
		height /= 2;
	}

	/* Bigtile mode uses double width */
	else if (use_bigtile) width /= 2;

	/* Display lines until done */
	for (i = 0; i < height; i++)
	{
		/* Display columns until done */
		for (j = 0; j < width; j++)
		{
			byte a;
			char c;
			int x = col + j;
			int y = row + i;
			int ia, ic;

			/* Triple tile mode uses double width and double height */
			if (use_trptile)
			{
				y += 2 * i;
				x += (use_bigtile ? 5 : 2) * j;
			}
			/* Double tile mode uses double width and double height */
			else if (use_dbltile)
			{
				y += i;
				x += (use_bigtile ? 3 : 1) * j;
			}
			/* Bigtile mode uses double width */
			else if (use_bigtile) x += j;

			ia = attr_top + i;
			ic = char_left + j;

			a = (byte)ia;
			c = (char)ic;

			/* Display symbol */
			Term_putch(x, y, a, c);

			if (use_bigtile || use_dbltile || use_trptile)
			{
				big_putch(x, y, a, c);
			}
		}
	}
}


/*
 * Place the cursor at the collect position for visual mode
 */
static void place_visual_list_cursor(int col, int row, byte a, byte c, byte attr_top, byte char_left)
{
	int i = a - attr_top;
	int j = c - char_left;

	int x = col + j;
	int y = row + i;

	/* Triple tile mode uses double height and width */
	if (use_trptile)
	{
		y += 2 * i;
		x += (use_bigtile ? 5 : 2) * j;
	}
	/* Double tile mode uses double height and width */
	else if (use_dbltile)
	{
		y += i;
		x += (use_bigtile ? 3 : 1) * j;
	}
	/* Bigtile mode uses double width */
	else if (use_bigtile) x += j;

	/* Place the cursor */
	Term_gotoxy(x, y);
}


/*
 *  Do visual mode command -- Change symbols
 */
static bool visual_mode_command(key_event ke, bool *visual_list_ptr,
				int height, int width,
				byte *attr_top_ptr, char *char_left_ptr,
				byte *cur_attr_ptr, char *cur_char_ptr,
				int col, int row, int *delay)
{
	static byte attr_old = 0;
	static char char_old = 0;

	int frame_left = 10;
	int frame_right = 10;
	int frame_top = 4;
	int frame_bottom = 4;

	if (use_trptile) frame_left /= (use_bigtile ? 6 : 3);
	else if (use_dbltile) frame_left /= (use_bigtile ? 4 : 2);
	else if (use_bigtile) frame_left /= 2;

	else if (use_dbltile) frame_right /= (use_bigtile ? 4 : 2);
	else if (use_bigtile) frame_right /= 2;

	if (use_trptile) frame_top /= 3;
	else if (use_dbltile) frame_top /= 2;

	if (use_trptile) frame_bottom /= 3;
	else if (use_dbltile) frame_bottom /= 2;

	switch (ke.key)
	{
		case ESCAPE:
			if (*visual_list_ptr)
			{
				/* Cancel change */
				*cur_attr_ptr = attr_old;
				*cur_char_ptr = char_old;
				*visual_list_ptr = FALSE;

				return TRUE;
			}
			break;

		case '\n':
		case '\r':
			if (*visual_list_ptr)
			{
				/* Accept change */
				*visual_list_ptr = FALSE;

				return TRUE;
			}
			break;

		case 'V':
		case 'v':
			if (!*visual_list_ptr)
			{
				*visual_list_ptr = TRUE;

				*attr_top_ptr = (byte)MAX(0, (int)*cur_attr_ptr - frame_top);
				*char_left_ptr = (char)MAX(-128, (int)*cur_char_ptr - frame_left);

				attr_old = *cur_attr_ptr;
				char_old = *cur_char_ptr;

				return TRUE;
			}
			else
			{
				/* Cancel change */
				*cur_attr_ptr = attr_old;
				*cur_char_ptr = char_old;
				*visual_list_ptr = FALSE;

				return TRUE;
			}
			break;

		case 'C':
		case 'c':
			/* Set the visual */
			attr_idx = *cur_attr_ptr;
			char_idx = *cur_char_ptr;

			return TRUE;

		case 'P':
		case 'p':
			if (attr_idx)
			{
				/* Set the char */
				*cur_attr_ptr = attr_idx;
				*attr_top_ptr = (byte)MAX(0, (int)*cur_attr_ptr - frame_top);
			}

			if (char_idx)
			{
				/* Set the char */
				*cur_char_ptr = char_idx;
				*char_left_ptr = (char)MAX(-128, (int)*cur_char_ptr - frame_left);
			}

			return TRUE;

		default:
			if (*visual_list_ptr)
			{
				int eff_width, eff_height;
				int d = target_dir(ke.key);
				byte a = *cur_attr_ptr;
				char c = *cur_char_ptr;

				if (use_trptile) eff_width = width / (use_bigtile ? 6 : 3);
				else if (use_dbltile) eff_width = width / (use_bigtile ? 4 : 2);
				else if (use_bigtile) eff_width = width / 2;
				else eff_width = width;

				if (use_trptile) eff_height = height / 3;
				else if (use_dbltile) eff_height = height / 2;
				else eff_height = height;

				/* Get mouse movement */
				if (ke.key == '\xff')
				{
					int my = ke.mousey - row;
					int mx = ke.mousex - col;

					if (use_trptile) { my /= 3; mx /=3; }
					else if (use_dbltile) {my /=2; mx /=2; }

					if (use_bigtile) mx /= 2;

					if ((my >= 0) && (my < eff_height) && (mx >= 0) && (mx < eff_width)
						&& ((ke.mousebutton) || (a != *attr_top_ptr + my) || (c != *char_left_ptr + mx)) )
					{
						/* Set the visual */
						*cur_attr_ptr = a = *attr_top_ptr + my;
						*cur_char_ptr = c = *char_left_ptr + mx;

						/* Move the frame */
						if (*char_left_ptr > MAX(-128, (int)c - frame_left)) (*char_left_ptr)--;
						if (*char_left_ptr + eff_width < MIN(127, (int)c + frame_right)) (*char_left_ptr)++;
						if (*attr_top_ptr > MAX(0, (int)a - frame_top)) (*attr_top_ptr)--;
						if (*attr_top_ptr + eff_height < MIN(255, (int)a + frame_bottom)) (*attr_top_ptr)++;

						/* Delay */
						*delay = 100;

						/* Accept change */
						if (ke.mousebutton) *visual_list_ptr = FALSE;

						return TRUE;
					}
					/* Cancel change */
					else if (ke.mousebutton)
					{
						*cur_attr_ptr = attr_old;
						*cur_char_ptr = char_old;
						*visual_list_ptr = FALSE;

						return TRUE;
					}
				}
				else
				{
					/* Restrict direction */
					if ((a == 0) && (ddy[d] < 0)) d = 0;
					if ((c == -128) && (ddx[d] < 0)) d = 0;
					if ((a == 255) && (ddy[d] > 0)) d = 0;
					if ((c == 127) && (ddx[d] > 0)) d = 0;

					a += ddy[d];
					c += ddx[d];

					/* Set the visual */
					*cur_attr_ptr = a;
					*cur_char_ptr = c;

					/* Move the frame */
					if ((ddx[d] < 0) && *char_left_ptr > MAX(-128, (int)c - frame_left)) (*char_left_ptr)--;
					if ((ddx[d] > 0) && *char_left_ptr + eff_width < MIN(127, (int)c + frame_right)) (*char_left_ptr)++;
					if ((ddy[d] < 0) && *attr_top_ptr > MAX(0, (int)a - frame_top)) (*attr_top_ptr)--;
					if ((ddy[d] > 0) && *attr_top_ptr + eff_height < MIN(255, (int)a + frame_bottom)) (*attr_top_ptr)++;

					if (d != 0) return TRUE;
				}
			}

		break;
	}

	/* Visual mode command is not used */
	return FALSE;
}


/* The following sections implement "subclasses" of the
 * abstract classes represented by member_funcs and group_funcs
*/

/* =================== MONSTERS ==================================== */
/* Many-to-many grouping - use default auxiliary join */

/*
 * Display a monster
 */
static void display_monster(int col, int row, bool cursor, int oid)
{
	/* HACK Get the race index. (Should be a wrapper function) */
	int r_idx = default_join[oid].oid;

	/* Access the race */
	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[r_idx];
	char m_name[80];

	/* Choose colors */
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
	byte a = r_ptr->x_attr;
	byte c = r_ptr->x_char;

	/* Get the name */
	race_desc(m_name, sizeof(m_name), r_idx, 0x400, 1);
	
	/* Display the name */
	c_prt(attr, m_name, row, col);

	if (use_dbltile || use_trptile)
		return;

	/* Display symbol */
	big_pad(66, row, a, c);

	/* Display kills */
	if (r_ptr->flags1 & (RF1_UNIQUE))
		put_str(format("%s", (r_ptr->max_num == 0)?  " dead" : "alive"), row, 70);
	else put_str(format("%5d", l_ptr->pkills), row, 70);
}


static int m_cmp_race(const void *a, const void *b) {
	monster_race *r_a = &r_info[default_join[*(int*)a].oid];
	monster_race *r_b = &r_info[default_join[*(int*)b].oid];
	int gid = default_join[*(int*)a].gid;
	/* group by */
	int c = gid - default_join[*(int*)b].gid;
	if(c) return c;
	/* order results */
	c = r_a->d_char - r_b->d_char;
	if(c && gid != 0) /* UNIQUE group is ordered by level & name only */
		return strchr(monster_group_char[gid], r_b->d_char)
					- strchr(monster_group_char[gid], r_a->d_char);
	c = r_a->level - r_b->level;
	if(c) return c;
	return strcmp(r_name + r_a->name, r_name + r_b->name);
}

static char *m_xchar(int oid)
	{ return &r_info[default_join[oid].oid].x_char; }
static byte *m_xattr(int oid)
	{ return &r_info[default_join[oid].oid].x_attr; }
static const char *race_name(int gid) { return monster_group_text[gid]; }
static void mon_lore(int oid) { screen_roff(default_join[oid].oid, &l_list[default_join[oid].oid]); anykey(); }

/*
 * Display known monsters.
 */
static void do_cmd_knowledge_monsters(void)
{
	group_funcs r_funcs = {N_ELEMENTS(monster_group_text), FALSE, race_name,
							m_cmp_race, default_group, 0};

	member_funcs m_funcs = {display_monster, mon_lore, m_xchar, m_xattr, 0, 0};

	int *monsters;
	int m_count = 0;
	int i;
	size_t j;

	for(i = 0; i < z_info->r_max; i++) {
		monster_race *r_ptr = &r_info[i];
		if(!adult_lore && !l_list[i].sights) continue;
		if(!r_ptr->name) continue;

		if(r_ptr->flags1 & RF1_UNIQUE) m_count++;
		for(j = 1; j < N_ELEMENTS(monster_group_char)-1; j++) {
			const char *pat = monster_group_char[j];
			if(strchr(pat, r_ptr->d_char)) m_count++;
		}
	}

	default_join = C_ZNEW(m_count, join_t);
	monsters = C_ZNEW(m_count, int);

	m_count = 0;
	for(i = 0; i < z_info->r_max; i++) {
		monster_race *r_ptr = &r_info[i];
		if(!adult_lore && !l_list[i].sights) continue;
		if(!r_ptr->name) continue;

		for(j = 0; j < N_ELEMENTS(monster_group_char)-1; j++) {
			const char *pat = monster_group_char[j];
			if(j == 0 && !(r_ptr->flags1 & RF1_UNIQUE))
				continue;
			else if(j > 0 && !strchr(pat, r_ptr->d_char))
				continue;

			monsters[m_count] = m_count;
			default_join[m_count].oid = i;
			default_join[m_count++].gid = j;
		}
	}

	display_knowledge("monsters", monsters, m_count, r_funcs, m_funcs,
						"Sym  Kills");
	FREE(monsters);
	FREE(default_join);
	default_join = 0;
}

/* =================== ARTIFACTS ==================================== */
/* Many-to-one grouping */

/*
 * Display an artifact label
 */
static void display_artifact(int col, int row, bool cursor, int oid)
{
	char o_name[80];
	object_type object_type_body;
	object_type *o_ptr = &object_type_body;

	/* Choose a color */
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];

	/* Get local object */
	o_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(o_ptr);

	/* Make fake artifact */
	make_fake_artifact(o_ptr, oid);

	/* Get its name */
	object_desc_spoil(o_name, sizeof(o_name), o_ptr, TRUE, 0);

	/* Display the name */
	c_prt(attr, o_name, row, col);
}

/*
 * Show artifact lore
 */
static void desc_art_fake(int a_idx)
{
	object_type *o_ptr;
	object_type object_type_body;

	/* Get local object */
	o_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(o_ptr);

	/* Make fake artifact */
	make_fake_artifact(o_ptr, a_idx);
	o_ptr->ident |= IDENT_STORE | IDENT_PVAL | IDENT_KNOWN;
	if(adult_lore) o_ptr->ident |= IDENT_MENTAL;

	/* Hack -- Handle stuff */
	handle_stuff();

	/* Save the screen */
	screen_save();

	/* Describe */
	screen_object(o_ptr);

	Term_flush();
	(void)anykey();
	screen_load();
}

static int a_cmp_tval(const void *a, const void *b)
{
	artifact_type *a_a = &a_info[*(int*)a];
	artifact_type *a_b = &a_info[*(int*)b];
	/*group by */
	int ta = obj_group_order[a_a->tval];
	int tb = obj_group_order[a_b->tval];
	int c = ta - tb;
	if(c) return c;
	/* order by */
	c = a_a->sval - a_b->sval;
	if(c) return c;
	return strcmp(a_name+a_a->name, a_name+a_b->name);
}

static const char *kind_name(int gid)
{ return object_group[obj_group_order[gid]].text; }
static int art2tval(int oid) { return a_info[oid].tval; }

/*
 * Display known artifacts
 */
static void do_cmd_knowledge_artifacts(void)
{
	/* HACK -- should be TV_MAX */
	group_funcs obj_f = {TV_GEMS, FALSE, kind_name, a_cmp_tval, art2tval, 0};
	member_funcs art_f = {display_artifact, desc_art_fake, 0, 0, 0, 0};

	int *artifacts;
	int a_count = 0;
	int i, j;

	artifacts = C_ZNEW(z_info->a_max, int);

	/* Collect valid artifacts */
	for(i = 0; i < z_info->a_max; i++) {
		if((adult_lore || a_info[i].cur_num) && a_info[i].name)
			artifacts[a_count++] = i;
	}
	for(i = 0; !adult_lore && i < z_info->o_max; i++) {
		int a = o_list[i].name1;
		if(a && !object_known_p(&o_list[i])) {
			for(j = 0; j < a_count && a != artifacts[j]; j++);
			a_count -= 1;
			for(; j < a_count; j++)
				artifacts[j] = artifacts[j+1];
		}
	}

	display_knowledge("artifacts", artifacts, a_count, obj_f, art_f, 0);
	FREE(artifacts);
}

/* =================== EGO ITEMS  ==================================== */
/* Many-to-many grouping (uses default join) */

static void display_ego_item(int col, int row, bool cursor, int oid)
{
	/* HACK: Access the object */
	ego_item_type *e_ptr = &e_info[default_join[oid].oid];

	/* Choose a color */
	byte attr = curs_attrs[(e_ptr->aware) ? 1 : 0][(int)cursor];

	/* Display the name */
	c_prt(attr, e_name + e_ptr->name, row, col);
}

/*
 * Describe fake ego item "lore"
 */
static void desc_ego_fake(int oid)
{
	/* Hack: dereference the join */
	const char *cursed [] = {"permanently cursed", "heavily cursed", "cursed"};
	int f3, i;
	int e_idx = default_join[oid].oid;
	object_lore *n_ptr = &e_list[e_idx];
	ego_item_type *e_ptr = &e_info[e_idx];

	/* Save screen */
	screen_save();

	/* Dump the name */
	c_prt(TERM_L_BLUE, format("Ego Item %s",e_name+e_ptr->name), 0, 0);

	/* Begin recall */
	Term_gotoxy(0, 1);

	/* List can flags */
	if(adult_lore) list_object_flags(e_ptr->flags1, e_ptr->flags2, e_ptr->flags3,
									e_ptr->flags4, 0, 1);

	else {
		list_object_flags(n_ptr->can_flags1, n_ptr->can_flags2, n_ptr->can_flags3,
							n_ptr->can_flags4, 0, 1);
		/* List may flags */
		list_object_flags(n_ptr->may_flags1,n_ptr->may_flags2,n_ptr->may_flags3,n_ptr->may_flags4,0, 2);
	}

	for(i = 0, f3 = TR3_PERMA_CURSE; i < 3 ; f3 >>= 1, i++) {
		if(e_ptr->flags3 & f3) {
			text_out_c(TERM_RED, format("It is %s.", cursed[i]));
			break;
		}
	}

	/* List the runes required to make this item */
	list_ego_item_runes(e_idx, adult_lore);

	Term_flush();
	(void)anykey();
	screen_load();
}

/* TODO? Currently ego items will order by e_idx */
static int e_cmp_tval(const void *a, const void *b)
{
	ego_item_type *ea = &e_info[default_join[*(int*)a].oid];
	ego_item_type *eb = &e_info[default_join[*(int*)b].oid];
	/*group by */
	int c = default_join[*(int*)a].gid - default_join[*(int*)b].gid;
	if(c) return c;
	/* order by */
	return strcmp(e_name + ea->name, e_name + eb->name);
}
static u16b *e_note(int oid) { return &e_info[default_join[oid].oid].note; }
static const char *ego_grp_name(int gid) { return object_group[gid].text; }

/*
 * Display known ego_items
 */
static void do_cmd_knowledge_ego_items(void)
{
	group_funcs obj_f =
		{TV_GEMS, FALSE, ego_grp_name, e_cmp_tval, default_group, 0};

	member_funcs ego_f = {display_ego_item, desc_ego_fake, 0, 0, e_note, 0};

	int *egoitems;
	int e_count = 0;
	int i, j;

	/* HACK: currently no more than 3 tvals for one ego type */
	egoitems = C_ZNEW(z_info->e_max*3, int);
	default_join = C_ZNEW(z_info->e_max*3, join_t);
	for(i = 0; i < z_info->e_max; i++) {
		if((e_info[i].aware & (AWARE_EXISTS)) || adult_lore) {
			for(j = 0; j < 3 && e_info[i].tval[j]; j++) {
				int gid = obj_group_order[e_info[i].tval[j]];
				/* HACK: Ignore duplicate tvals */
				if(j > 0 && gid == default_join[e_count-1].gid)
					continue;
				egoitems[e_count] = e_count;
				default_join[e_count].oid = i;
				default_join[e_count++].gid = gid;
			}
		}
	}

	display_knowledge("ego items", egoitems, e_count, obj_f, ego_f, "Sym");
	FREE(default_join);
	default_join = 0;
	FREE(egoitems);
}

/* =================== ORDINARY OBJECTS  ==================================== */
/* Many-to-one grouping */

/*
 * Strip an "object kind name" into a buffer
 */
void strip_name(char *buf, int buf_s, int oid)
{
	char *t;

	int k_idx = oid >= 0 ? oid : -oid;

	object_kind *k_ptr = &k_info[k_idx];

	cptr str = (k_name + k_ptr->name);

	/* If not aware, use flavor */
	if (oid < 0) str = x_text + x_info[k_ptr->flavor].text;

	/* Skip past leading characters */
	while ((*str == ' ') || (*str == '&') || (*str == '#')) str++;

	/* Copy useful chars */
	for (t = buf; *str && buf_s; str++)
	{
		if (prefix(str,"# ")) str++; /* Skip following space */
		else if (*str != '~') { *t++ = *str; buf_s--; }
	}

	/* Terminate the new name */
	*t = '\0';
}

/*
 * Display the objects in a group.
 */
static void display_object(int col, int row, bool cursor, int oid)
{
	int k_idx = oid >= 0 ? oid : -oid;

	/* Access the object */
	object_kind *k_ptr = &k_info[k_idx];

	char o_name[80];

	/* Choose a color */
	bool aware = (oid >= 0) || (!view_flavors && (k_ptr->aware & (AWARE_FLAVOR)));
	byte a = aware ? k_ptr->x_attr : x_info[k_ptr->flavor].x_attr;
	byte c = aware ? k_ptr->x_char : x_info[k_ptr->flavor].x_char;
	byte attr = curs_attrs[(int)oid >= 0  && k_ptr->flavor != 0 ? ((k_ptr->aware & (AWARE_FLAVOR)) != 0 ? 1 : 0)
						: ((k_ptr->aware & (AWARE_SEEN)) != 0) ? 1 : 0][(int)cursor];

	/* Symbol is unknown.  This should never happen.*/
	if (!(k_ptr->aware & (AWARE_EXISTS)) && !k_ptr->flavor && !p_ptr->wizard && !adult_lore)
	{
		assert(FALSE);
		c = ' ';
		a = TERM_DARK;
	}

	/* Tidy name */
	strip_name(o_name,sizeof(o_name),oid);

	/* Display the name */
	c_prt(attr, o_name, row, col);


	/* Hack - don't use if double tile */
	if (use_dbltile || use_trptile)
		return;

	/* Display symbol */
	big_pad(76, row, a, c);
}

/*
 * Describe fake object
 */
static void desc_obj_fake(int oid)
{
	int k_idx = oid >= 0 ? oid : -oid;

	object_type object_type_body;
	object_type *o_ptr = &object_type_body;
	/* Wipe the object */
	object_wipe(o_ptr);

	/* Create the artifact */
	object_prep(o_ptr, k_idx);

	/* Hack -- its in the store */
	if (oid >= 0) o_ptr->ident |= (IDENT_STORE);

	/* Track the object */
	object_actual_track(o_ptr);

	/* Hack - mark as fake */
	term_obj_real = FALSE;

	/* Hack -- Handle stuff */
	handle_stuff();

	/* Save the screen */
	screen_save();

	/* Describe */
	screen_object(o_ptr);

	Term_flush();
	(void)anykey();
	screen_load();
}

static int o_cmp_tval(const void *a, const void *b)
{
	object_kind *k_a = &k_info[*(int*)a >= 0 ? *(int*)a : -*(int*)a];
	object_kind *k_b = &k_info[*(int*)b >= 0 ? *(int*)b : -*(int*)b];
	/*group by */
	int ta = obj_group_order[k_a->tval];
	int tb = obj_group_order[k_b->tval];
	int c = ta - tb;
	if(c) return c;
	/* order by */
	c = (*(int*)a >= 0 ? 1 : 0) - (*(int*)b >= 0 ? 1 : 0);
	if(c) return -c; /* aware has low sort weight */
	if(*(int*)a < 0) {
		return strcmp(x_text + x_info[k_a->flavor].text,
									x_text +x_info[k_b->flavor].text);
	}
	c = k_a->cost - k_b->cost;
	if(c) return c;
	return strcmp(k_name + k_a->name, k_name + k_b->name);
}
static int obj2tval(int oid) { return k_info[oid >= 0 ? oid : -oid].tval; }
static char *o_xchar(int oid) {
	object_kind *k_ptr = &k_info[oid >= 0 ? oid : -oid];
	if(oid >= 0) return &k_ptr->x_char;
	else return &x_info[k_ptr->flavor].x_char;
}
static byte *o_xattr(int oid) {
	object_kind *k_ptr = &k_info[oid >= 0 ? oid : -oid];
	if(oid >= 0) return &k_ptr->x_attr;
	else return &x_info[k_ptr->flavor].x_attr;
}
static u16b *o_note(int oid) {
	object_kind *k_ptr = &k_info[oid >= 0 ? oid : -oid];
	if(oid >= 0) return &k_ptr->note;
	else return 0;
}

/*
 * Display known objects
 */
static void do_cmd_knowledge_objects(void)
{
	group_funcs kind_f =
		{TV_GEMS, FALSE, kind_name, o_cmp_tval, obj2tval, 0};
	member_funcs obj_f =
		{display_object, desc_obj_fake, o_xchar, o_xattr, o_note, 0};

	int *objects;
	int o_count = 0;
	int i;

	objects = C_ZNEW(z_info->k_max * 2, int);

	for(i = 0; i < z_info->k_max; i++) {
		if((k_info[i].aware & (AWARE_EXISTS)) || (k_info[i].flavor) || adult_lore) {
			int c = obj_group_order[k_info[i].tval];
			if(c >= 0 && object_group[c].text) {
				if ((k_info[i].aware & (AWARE_EXISTS)) || (adult_lore))
				{
					objects[o_count++] = i;
				}
				if (k_info[i].flavor && !(k_info[i].aware & (AWARE_FLAVOR)))
				{
					objects[o_count++] = -i;
				}
			}
		}
	}
	display_knowledge("known objects", objects, o_count, kind_f, obj_f, "Sym");
	FREE(objects);
}

/* =================== TERRAIN FEATURES ==================================== */
/* Many-to-one grouping */

/*
 * Display the features in a group.
 */
static void display_feature(int col, int row, bool cursor, int oid )
{
	int col2 = 70 + use_bigtile;
	int col3 = col2 + 2 + use_bigtile;

	/* Get the feature index */
	int f_idx = oid;

	/* Access the feature */
	feature_type *f_ptr = &f_info[f_idx];

	/* Choose a color */
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];

	/* Display the name */
	c_prt(attr, f_name + f_ptr->name, row, col);


	if (use_dbltile || use_trptile) return;

	/* Display symbol */
	big_pad(68, row, f_ptr->x_attr, f_ptr->x_char);


	/* Tile supports special lighting? */
	if (!(f_ptr->flags3 & (FF3_ATTR_LITE | FF3_ATTR_DOOR | FF3_ATTR_WALL)) ||
		((f_ptr->x_attr) && (arg_graphics == GRAPHICS_DAVID_GERVAIS_ISO)))
	{
			prt("       ", row, col2);
			return;
	}

	c_prt(TERM_SLATE, (use_bigtile == 0) ? "( / )" : "(  /  ) ", row, col2);

	/* Mega-hack */
	if (f_ptr->x_attr & 0x80)
	{
		/* Use a brightly lit tile */
		if ((arg_graphics == GRAPHICS_DAVID_GERVAIS) || (arg_graphics == GRAPHICS_DAVID_GERVAIS_ISO))
			big_pad(col2+1, row, f_ptr->x_attr, f_ptr->x_char-1);
		else
			big_pad(col2+1, row, f_ptr->x_attr, f_ptr->x_char+2);

		/* Use a dark tile */
		big_pad(col3+1, row, f_ptr->x_attr, f_ptr->x_char+1);
	}
	else
	{
		/* Use "yellow" */
		big_pad(col2+1, row, get_color(f_ptr->x_attr, ATTR_LITE, 1), f_ptr->x_char);

		/* Use "grey" */
		big_pad(col3+1, row, get_color(f_ptr->x_attr, ATTR_DARK, 1), f_ptr->x_char);
	}
}


static int f_cmp_fkind(const void *a, const void *b) {
	feature_type *fa = &f_info[*(int*)a];
	feature_type *fb = &f_info[*(int*)b];
	/* group by */
	int c = feat_order(*(int*)a) - feat_order(*(int*)b);
	if(c) return c;
	/* order by feature name */
	return strcmp(f_name + fa->name, f_name + fb->name);
}

static const char *fkind_name(int gid) { return feature_group_text[gid]; }
static byte *f_xattr(int oid) { return &f_info[oid].x_attr; }
static char *f_xchar(int oid) { return &f_info[oid].x_char; }
static void feat_lore(int oid) { screen_feature_roff(oid); anykey(); }
static u32b *f_flags3(int oid) { return &f_info[oid].flags3; }

/*
 * Interact with feature visuals.
 */
static void do_cmd_knowledge_features(void)
{
	group_funcs fkind_f = {N_ELEMENTS(feature_group_text), FALSE,
							fkind_name, f_cmp_fkind, feat_order, 0};

	member_funcs feat_f = {display_feature, feat_lore, f_xchar, f_xattr, 0, f_flags3};

	int *features;
	int f_count = 0;
	int i;
	features = C_ZNEW(z_info->f_max + 1, int);

	for(i = 0; i < z_info->f_max; i++) {
		if ((f_info[i].mimic != i) && (i != FEAT_INVIS)) continue;
		features[f_count++] = i; /* Currently no filter for features */
	}

	display_knowledge("features", features, f_count, fkind_f, feat_f, " Sym (Lt/Dk)");
	FREE(features);
}

/* =================== HOMES AND STORES ==================================== */
/* Many-to-many grouping */

static void display_store_object(int col, int row, bool cursor, int oid)
{
	/* HACK: get the object */
	/* Do something about bags */
	store_type *s_ptr = store[default_join[oid].gid];
	object_type *o_ptr = &s_ptr->stock[default_join[oid].oid];
	char buffer[160];

	/* Should be UNKNOWN for unreachable locations like Bombadil's */
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];

	object_desc(buffer, sizeof buffer, o_ptr, TRUE, 3);

	c_prt(attr, buffer, row, col);
}

static void screen_store_object(int oid)
{
	store_type *s_ptr = store[default_join[oid].gid];
	object_type *o_ptr = &s_ptr->stock[default_join[oid].oid];

	/* Save the screen */
	screen_save();

	/* Describe */
	screen_object(o_ptr);

	Term_flush();
	(void)anykey();
	screen_load();
}


/* this isn't necessary; data is already sorted.*/
/* static int s_cmp_obj(void *a, void *b); */
static const char *store_name(int gid)
{
	/* HACK: Home works differently from storage */
	if(gid != STORE_HOME) return u_name + store[gid]->name;
	else return u_name + u_info[store[STORE_HOME]->index].name;
}

static void do_cmd_knowledge_home(void)
{
	member_funcs contents_f = {display_store_object, screen_store_object, 0, 0, 0, 0};
	group_funcs home_f = {total_store_count, FALSE, store_name, 0, default_group, 0};

	int *objects, store_count = 0;
	int i, j;
	int o_count = 0;

	for(i = 0; i < total_store_count; i++) {
		if(!cheat_xtra && i != STORE_HOME && store[i]->base != 1 && store[i]->base != 2)
			continue;
		store_count++;
	}

	objects = C_ZNEW((MAX_INVENTORY_HOME+1)*store_count, int);
	default_join = C_ZNEW((MAX_INVENTORY_HOME+1)*store_count, join_t);

	for(i = 0; i < total_store_count; i++) {
		/* Check for current home */
		if(!cheat_xtra && i != STORE_HOME && store[i]->base != 1 && store[i]->base != 2)
			continue;
		for(j = 0; j < store[i]->stock_size && store[i]->stock[j].k_idx; j++) {
			objects[o_count] = o_count;
			default_join[o_count].gid = i;
			default_join[o_count++].oid = j;
		}
	}

	display_knowledge(cheat_xtra ? "stores" : "homes", objects, o_count,
						home_f, contents_f, 0);
	FREE(default_join);
	default_join = 0;
	FREE(objects);
}

/* =================== TOWNS AND DUNGEONS ================================ */

int count_routes(int from, int to)
{
  s16b routes[24];
  int i, num;

  num = set_routes(routes, 24, from);

  for (i = 0; i < num; i++)
    if (to == routes[i]) return num;

    /* no route */
    return -num;
}

static void describe_surface_dungeon(int dun)
{
	int myd = p_ptr->dungeon;
	int num;

	if (dun == rp_ptr->home)
	{
		text_out_c(TERM_WHITE, t_info[dun].text + t_text);

		if (dun == myd)
			text_out_c(TERM_SLATE, "  You were born right here.");
		else
			text_out_c(TERM_SLATE, "  This is where you were born, though it was quite long ago.");
	}
	else if (t_info[dun].visited)
		text_out_c(TERM_WHITE, t_info[dun].text + t_text);
	else
		text_out_c(TERM_SLATE, "You've heard about this place and you can already spot it far away ahead.");

	/* routes from descibed dungeon to current dungeon */
	num = count_routes(dun, myd);

	if (num <= 0 && dun != myd)
		/* no road back to current */
		if (count_routes(myd, dun) > 0)
			/* road forth, hence one way */
			text_out_c(TERM_RED, format("  You know of no way back from %s to %s!",
										t_info[dun].name + t_name,
										t_info[myd].name + t_name));

	if (!num)
		text_out_c(TERM_SLATE, format("  You feel your old maps will not avail you %s.", dun == myd ? "here" : "there"));

	if (!t_info[dun].visited)
	{
		char str[1000];
		int i, found = 0;
		for (i = 1; i < z_info->t_max; i++)
		{
			if (t_info[i].replace_ifvisited == dun)
			{
				if (found == 0)
				{
					sprintf(str, "  If you enter %s, you will never find %s", t_info[dun].name + t_name, t_info[i].name + t_name);
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
			text_out_c(TERM_WHITE, str);
	}
}

static void describe_zone_guardian(int dun, int zone)
{
  int guard = actual_guardian(t_info[dun].zone[zone].guard, dun, zone);

  char m_name[80];
  
  if (guard) {
    bool bars = (r_info[guard].max_num
		 && t_info[dun].quest_monster == guard);
    char str[46];

    if (bars)
      long_level_name(str, actual_route(t_info[dun].quest_opens), 0);

	/* Get the name */
	race_desc(m_name, sizeof(m_name), guard, 0x100, 1);

    text_out_c(TERM_SLATE, format("  It %s guarded by %s%s%s.",
				  r_info[guard].max_num ? "is" : "was",
				  m_name,
				  bars ? ", who bars the way to " : "",
				  bars ? str : ""));
  }
}

static void dungeon_lore(int oid) {
	int dun = oid / MAX_DUNGEON_ZONES;
	int zone = oid % MAX_DUNGEON_ZONES;
	char str[46];

	long_level_name(str, dun, t_info[dun].zone[zone].level);

	screen_save();

	if (zone == MAX_DUNGEON_ZONES - 1
			|| t_info[dun].zone[zone+1].level == 0
			|| t_info[dun].zone[zone+1].level - 1 == t_info[dun].zone[zone].level) {
		/* one-level zone */
		if (t_info[dun].zone[zone].level == 0)
			c_prt(TERM_L_BLUE, str, 0, 0);
		else if (depth_in_feet)
			c_prt(TERM_L_BLUE, format("%d feet at %s", 50 * t_info[dun].zone[zone].level, str), 0, 0);
		else
			c_prt(TERM_L_BLUE, format("Level %d of %s", t_info[dun].zone[zone].level, str), 0, 0);
	}
	else {
		if (depth_in_feet)
			c_prt(TERM_L_BLUE, format("%d-%d feet at %s",
					t_info[dun].zone[zone].level * 50,
					(t_info[dun].zone[zone+1].level - 1) * 50,
					str), 0, 0);
		else
			c_prt(TERM_L_BLUE, format("Levels %d-%d of %s",
					t_info[dun].zone[zone].level,
					t_info[dun].zone[zone+1].level - 1,
					str), 0, 0);
	}

	Term_gotoxy(0, 1);

	if (!zone)
	  describe_surface_dungeon(dun);

	describe_zone_guardian(dun, zone);

	if (dun == p_ptr->dungeon && (zone || dun != rp_ptr->home)) {
	  dungeon_zone *my_zone;

	  /* Get the zone */
	  get_zone(&my_zone, dun, p_ptr->depth);

	  if (my_zone == &t_info[dun].zone[zone])
	    text_out_c(TERM_SLATE, "  This is where you stand right now.");
	}

	Term_flush();
	(void)anykey();
	screen_load();
}

static void display_dungeon_zone(int col, int row, bool cursor, int oid)
{
	int dun = oid / MAX_DUNGEON_ZONES;
	int zone = oid % MAX_DUNGEON_ZONES;
	int guard = actual_guardian(t_info[dun].zone[zone].guard, dun, zone);
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
	char str[46];

	long_level_name(str, dun, t_info[dun].zone[zone].level);

	if(!t_info[dun].zone[zone].name) {
		if(zone == 0) {
			c_prt(attr, t_info[dun].name + t_name, row, col);
		}
		else {
			if (depth_in_feet)
				c_prt(attr, format("%d' in %s", t_info[dun].zone[zone].level * 50,
									 t_info[dun].name + t_name),  row, col);
			else
				c_prt(attr, format("Level %d of %s", t_info[dun].zone[zone].level,
									 t_info[dun].name + t_name),  row, col);
		}
	}
	else {
		c_prt(attr, format("%s", str), row, col);
	}

	if (t_info[dun].attained_depth >= t_info[dun].zone[zone].level)
	{
		if (/* last zone */
			(zone == MAX_DUNGEON_ZONES - 1
			 || t_info[dun].zone[zone+1].level == 0)
			/* depth in that dungeon is less than start of the next zone */
			|| (t_info[dun].attained_depth < t_info[dun].zone[zone+1].level))
		{
			if (t_info[dun].attained_depth)
				if (depth_in_feet)
					c_prt(attr, format(" %4d'", 50 * t_info[dun].attained_depth),
							row, 67);
				else
					c_prt(attr, format(" Lev %3d", t_info[dun].attained_depth),
							row, 67);
			else if (zone == 1
						|| t_info[dun].zone[zone+1].level == 0)
				; /* no dungeon nor tower --- no babbling */
			else
				c_prt(attr, " surface", row, 67);
		}
		else
		{
			if (guard && r_info[guard].max_num)
				/* we've reached the guardian and escaped (or he did) */
				c_prt(attr, " !!!", row, 67);
			else
				/* we are already past the zone */
				c_prt(attr, " ***", row, 67);
		}
	}

	if (guard && !r_info[guard].max_num)
		/* we've killed the guardian, regardless if we've been there */
		c_prt(attr, " victory", row, 67);
}

static int oiddiv4 (int oid) { return oid/MAX_DUNGEON_ZONES; }
static const char* town_name(int gid) { return t_info[gid].name+t_name; }

void long_level_name(char* str, int town, int depth)
{
  dungeon_zone *zone;

  /* Get the zone */
  get_zone(&zone, town, depth);

  if (zone->name)
    sprintf(str, "%s %s", zone->name + t_name, t_info[town].name + t_name);
  else
    sprintf(str, "%s", t_info[town].name + t_name);
}

void current_long_level_name(char* str)
{
  long_level_name(str, p_ptr->dungeon, p_ptr->depth);
}

static void do_cmd_knowledge_dungeons(void)
{
	int i, j;
	int z_count = 0;
	int *zones;
	member_funcs zone_f = {display_dungeon_zone, dungeon_lore, 0, 0, 0, 0};
	group_funcs dun_f = {z_info->t_max, FALSE, town_name, 0, oiddiv4, 0};
	int myd = p_ptr->dungeon;
	int start_at = 0;

	zones = C_ZNEW(z_info->t_max*MAX_DUNGEON_ZONES, int);

	for (i = 1; i < z_info->t_max; i++)
	{
		if (!t_info[i].visited
			&& !(i == rp_ptr->home)
			&& !(count_routes(myd, i) > 0))
			continue;

		if (t_info[i].replace_ifvisited
			&& t_info[t_info[i].replace_ifvisited].visited)
			continue;

		if (i < p_ptr->dungeon)
			start_at++;

		for(j = 0; (j < 1 || t_info[i].zone[j].level != 0 )
				&& j < MAX_DUNGEON_ZONES; j++)
	    {
			zones[z_count++] = MAX_DUNGEON_ZONES*i + j;
	    }
	}

	display_knowledge_start_at(
		"locations", zones, z_count, dun_f, zone_f, "   Reached", start_at);
	FREE(zones);
}

/* Keep macro counts happy. */
static void cleanup_cmds (void) {
	FREE(obj_group_order);
}

/* The stand-alone version, e.g. for travel menu help */
void do_knowledge_dungeons(void)
{
	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	do_cmd_knowledge_dungeons();

	/* Flush messages */
	message_flush();
}

/* =================== HELP TIPS ================================ */

/*
 * File name prefix of each group
 */
static cptr tip_prefix[] =
{
	"All current tips",
	"birth",
	"race",
	"class",
	"style",
	"school",
	"scho",
	"spec",
	"spell",
	"tval",
	"kind",
	"ego",
	"art",
	"oflag",
	"look",
	"kill",
	"rflag",
	"dungeon",
	"depth",
	"level",
	NULL
};

static void display_tip_title(int col, int row, bool cursor, int oid)
{
	byte attr = curs_attrs[CURS_KNOWN][(int)cursor];

	cptr tip = quark_str(tips[default_join[oid].oid]);

	/* Current help file */
	FILE *fff = NULL;

	/* General buffer */
	char buf[128];

	/* Path buffer */
	char path[1024];

	/* Build the filename */
	path_build(path, 1024, ANGBAND_DIR_INFO, tip);

	/* Open the file */
	fff = my_fopen(path, "r");

	/* File doesn't exist or is empty */
	if (!fff || my_fgets(fff, buf, sizeof(buf)))
	{
		/* Message */
		msg_format("Cannot open '%s'.", tip);
		message_flush();

		/* Oops */
		c_prt(attr, "an unreadable help tip", row, col);
		return;
	}

	/* Close the file */
	my_fclose(fff);

	c_prt(attr, buf, row, col);
}

static void display_help_tip(int oid)
{
	cptr tip = quark_str(tips[default_join[oid].oid]);

	/* Save the screen */
	screen_save();

	/* Peruse the help tip file */
	(void)show_file(tip, NULL, 0, 0);

	/* Load the screen */
	screen_load();
}

static const char *tip_group_name(int gid)
{
	return tip_prefix[gid];
}

static int tip_group_cmp(const void *a, const void *b) {
	int gid = default_join[*(int*)a].gid;
	int c = gid - default_join[*(int*)b].gid;
	return c;
}

static void do_cmd_knowledge_help_tips(void)
{
	member_funcs tip_f = {display_tip_title, display_help_tip, 0, 0, 0, 0};
	group_funcs prefix_f = {N_ELEMENTS(tip_prefix), FALSE, tip_group_name, tip_group_cmp, default_group, 0};

	int cloned_tip_count = 0;
	int *help_tips;
	int i, j;

	help_tips = C_ZNEW(tips_start * 2, int);
	default_join = C_ZNEW(tips_start * 2, join_t);

	for(i = 0; i < tips_start; i++)
	{
		for(j = 0; j < (int)N_ELEMENTS(tip_prefix) - 1; j++) {
			const char *pat = tip_prefix[j];
			const cptr tip = quark_str(tips[i]);

			if (j > 0 && strncmp(pat, tip, strlen(pat)) != 0)
				continue;

			help_tips[cloned_tip_count] = cloned_tip_count;
			default_join[cloned_tip_count].oid = i;
			default_join[cloned_tip_count++].gid = j;
		}
	}

	display_knowledge("help tips", help_tips, cloned_tip_count,
							prefix_f, tip_f, 0);

	FREE(default_join);
	default_join = 0;
	FREE(help_tips);
}


/* =================== END JOIN DEFINITIONS ================================ */


/*
 * Hack -- redraw the screen
 *
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This command is also used to "instantiate" the results of the user
 * selecting various things, such as graphics mode, so it must call
 * the "TERM_XTRA_REACT" hook before redrawing the windows.
 */
void do_cmd_redraw(void)
{
	int j;

	term *old = Term;


	/* Low level flush */
	Term_flush();

	/* Reset "inkey()" */
	flush();

	/* Hack -- React to changes */
	Term_xtra(TERM_XTRA_REACT, 0);


	/* Combine and Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);


	/* Update torch */
	p_ptr->update |= (PU_TORCH);

	/* Update stuff */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw everything */
	p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP | PR_ITEM_LIST);

	/* Window everything */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1 |
			  PW_PLAYER_2 | PW_PLAYER_3 | PW_MESSAGE | PW_MAP |
			  PW_OVERHEAD | PW_MONSTER | PW_OBJECT | PW_FEATURE |
			  PW_SNAPSHOT | PW_MONLIST | PW_ITEMLIST | PW_HELP | PW_SELF_KNOW);

	/* Clear screen */
	Term_clear();

	/* Hack -- update */
	handle_stuff();


	/* Redraw every window */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		/* Dead window */
		if (!angband_term[j]) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Redraw */
		Term_redraw();

		/* Refresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*
 * Hack -- change name
 */
void do_cmd_change_name(void)
{
	key_event ke;

	int mode = 0;

	cptr p;

	/* Prompt */
	p = "['c' to change name, 'f' to file, 'h' to change mode, or ESC]";

	/* Save screen */
	screen_save();

	/* Forever */
	while (1)
	{
		/* Display the player */
		display_player(mode);

		/* Prompt */
		Term_putstr(2, 23, -1, TERM_WHITE, p);

		/* Query */
		ke = anykey();

		/* Exit */
		if (ke.key == ESCAPE) break;

		/* Change name */
		if ((ke.key == 'c') || ((ke.key == '\xff') && (ke.mousey == 2) && (ke.mousex < 26)))
		{
			get_name();
		}

		/* File dump */
		else if (ke.key == 'f')
		{
			char ftmp[80];

			sprintf(ftmp, "%s.txt", op_ptr->base_name);

			if (get_string("File name: ", ftmp, 80))
			{
				if (ftmp[0] && (ftmp[0] != ' '))
				{
					if (file_character(ftmp, FALSE))
					{
						msg_print("Character dump failed!");
					}
					else
					{
						msg_print("Character dump successful.");
					}
				}
			}
		}

		/* Toggle mode */
		else if ((ke.key == 'h') || (ke.key == '\xff'))
		{
			mode = (mode + 1) % 6;
		}

		/* Oops */
		else
		  {
		    bell(NULL);
		  }

		/* Flush messages */
		message_flush();
	}

	/* Load screen */
	screen_load();
}


/*
 * Recall the most recent message
 */
void do_cmd_message_one(void)
{
	/* Recall one message XXX XXX XXX */
	c_prt(message_color(0), format( "> %s", message_str(0)), 0, 0);
}


/*
 * Show previous messages to the user
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilite the matching portions of the string.
 */
void do_cmd_messages(void)
{
	key_event ke;

	int i, j, n, q;
	int wid, hgt;

	char shower[80];
	char finder[80];


	/* Wipe finder */
	my_strcpy(finder, "",sizeof(finder));

	/* Wipe shower */
	my_strcpy(shower, "",sizeof(shower));


	/* Total messages */
	n = message_num();

	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;

	/* Get size */
	Term_get_size(&wid, &hgt);

	/* Save screen */
	screen_save();

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Dump messages */
		for (j = 0; (j < hgt - 4) && (i + j < n); j++)
		{
			cptr msg = message_str((s16b)(i+j));
			byte attr = message_color((s16b)(i+j));

			/* Apply horizontal scroll */
			msg = ((int)strlen(msg) >= q) ? (msg + q) : "";

			/* Dump the messages, bottom to top */
			Term_putstr(0, hgt - 3 - j, -1, attr, msg);

			/* Hilite "shower" */
			if (shower[0])
			{
				cptr str = msg;

				/* Display matches */
				while ((str = strstr(str, shower)) != NULL)
				{
					int len = strlen(shower);

					/* Display the match */
					Term_putstr(str-msg, hgt - 3 - j, len, TERM_YELLOW, shower);

					/* Advance */
					str += len;
				}
			}
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
			   i, i + j - 1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[Press 'p' for older, 'n' for newer, ..., or ESCAPE]", hgt - 1, 0);

		/* Get a command */
		ke = anykey();

		/* Exit on Escape */
		if (ke.key == ESCAPE) break;

		/* Hack -- Save the old index */
		j = i;

		/* Horizontal scroll */
		if (ke.key == '4')
		{
			/* Scroll left */
			q = (q >= wid / 2) ? (q - wid / 2) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (ke.key == '6')
		{
			/* Scroll right */
			q = q + wid / 2;

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (ke.key == '=')
		{
			/* Prompt */
			prt("Show: ", hgt - 1, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80)) continue;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (ke.key == '/')
		{
			s16b z;

			/* Prompt */
			prt("Find: ", hgt - 1, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80)) continue;

			/* Show it */
			my_strcpy(shower, finder, sizeof(shower));

			/* Scan messages */
			for (z = i + 1; z < n; z++)
			{
				cptr msg = message_str(z);

				/* Search for it */
				if (strstr(msg, finder))
				{
					/* New location */
					i = z;

					/* Done */
					break;
				}
			}
		}

		/* Recall 20 older messages */
		if ((ke.key == 'p') || (ke.key == KTRL('P')) || (ke.key == ' '))
		{
			/* Go older if legal */
			if (i + 20 < n) i += 20;
		}

		/* Recall 10 older messages */
		if (ke.key == '+')
		{
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 1 older message */
		if ((ke.key == '8') || (ke.key == '\n') || (ke.key == '\r'))
		{
			/* Go older if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 20 newer messages */
		if ((ke.key == 'n') || (ke.key == KTRL('N')))
		{
			/* Go newer (if able) */
			i = (i >= 20) ? (i - 20) : 0;
		}

		/* Recall 10 newer messages */
		if (ke.key == '-')
		{
			/* Go newer (if able) */
			i = (i >= 10) ? (i - 10) : 0;
		}

		/* Recall 1 newer messages */
		if (ke.key == '2')
		{
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Scroll forwards or backwards using mouse clicks */
		if (ke.key == '\xff')
		{
			if (ke.mousebutton)
			{
				if (ke.mousey <= hgt / 2)
				{
					/* Go older if legal */
					if (i + 20 < n) i += 20;
				}
				else
				{
					/* Go newer (if able) */
					i = (i >= 20) ? (i - 20) : 0;
				}
			}
		}

		/* Hack -- Error of some kind */
		if (i == j) bell(NULL);
	}

	/* Load screen */
	screen_load();
}



/*
 * Ask for a "user pref line" and process it
 */
void do_cmd_pref(void)
{
	char tmp[80];

	/* Default */
	my_strcpy(tmp, "", sizeof(tmp));

	/* Ask for a "user pref command" */
	if (!get_string("Pref: ", tmp, sizeof(tmp))) return;

	/* Process that pref command */
	(void)process_pref_file_command(tmp);
}


/*
 * Ask for a "user pref file" and process it.
 *
 * This function should only be used by standard interaction commands,
 * in which a standard "Command:" prompt is present on the given row.
 *
 * Allow absolute file names?  XXX XXX XXX
 */
static void do_cmd_pref_file_hack(int row)
{
	char ftmp[80];

	/* Prompt */
	prt("Command: Load a user pref file", row, 0);

	/* Prompt */
	prt("File: ", row + 1, 0);

	/* Default filename */
	sprintf(ftmp, "%s.prf", op_ptr->base_name);

	/* Ask for a file (or cancel) */
	if (!askfor_aux(ftmp, 80)) return;

	/* Process the given filename */
	if (process_pref_file(ftmp))
	{
		/* Mention failure */
		msg_format("Failed to load '%s'!", ftmp);
	}
	else
	{
		/* Mention success */
		msg_format("Loaded '%s'.", ftmp);
	}
}



/*
 * Interact with some options
 */
static void do_cmd_options_aux(int page, cptr info)
{
	key_event ke;

	int i, k = 0, n = 0;

	int opt[OPT_PAGE_PER];

	char buf[80];


	/* Scan the options */
	for (i = 0; i < OPT_PAGE_PER; i++)
	{
		/* Collect options on this "page" */
		if (option_page[page][i] != 255)
		{
			opt[n++] = option_page[page][i];
		}
	}


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		/* Prompt XXX XXX XXX */
		sprintf(buf, "%s (RET to advance, y/n to set, ESC to accept) ", info);
		prt(buf, 0, 0);

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s  (%s)",
					  option_desc(opt[i]),
					  op_ptr->opt[opt[i]] ? "yes" : "no ",
					  option_name(opt[i]));
			c_prt(a, buf, i + 2, 0);
		}

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ke = inkey_ex();

		/* Analyze */
		switch (ke.key)
		{
			case ESCAPE:
			{
				/* Hack -- Notice use of any "cheat" options */
				for (i = OPT_CHEAT; i < OPT_ADULT; i++)
				{
					if (op_ptr->opt[i])
					{
						/* Set score option */
						op_ptr->opt[OPT_SCORE + (i - OPT_CHEAT)] = TRUE;
					}
				}

				return;
			}

			case '-':
			case '8':
			{
				k = (n + k - 1) % n;
				break;
			}

			case ' ':
			case '\n':
			case '\r':
			case '2':
			{
				k = (k + 1) % n;
				break;
			}

			case 't':
			case '5':
			{
				op_ptr->opt[opt[k]] = !op_ptr->opt[opt[k]];
				break;
			}

			case 'y':
			case '6':
			{
				op_ptr->opt[opt[k]] = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case '4':
			{
				op_ptr->opt[opt[k]] = FALSE;
				k = (k + 1) % n;
				break;
			}

			case '?':
			{
				sprintf(buf, "option.txt#%s", option_name(opt[k]));
				show_file(buf, NULL, 0, 0);
				Term_clear();
				break;
			}

			case '\xff':
			{
				int choice = ke.mousey - 2;

				if ((choice >= 0) && (choice < n)) k = choice;

				if (ke.mousebutton)
				{
					op_ptr->opt[opt[k]] = !op_ptr->opt[opt[k]];
				}
				break;
			}

			default:
			{
				bell("Illegal command for normal options!");
				break;
			}
		}
	}
}


/*
 * Modify the "window" options
 */
static void do_cmd_options_win(void)
{
	int i, j, d;

	int y = 0;
	int x = 0;

	key_event ke;

	u32b old_flag[ANGBAND_TERM_MAX];


	/* Memorize old flags */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		old_flag[j] = op_ptr->window_flag[j];
	}


	/* Clear screen */
	Term_clear();

	/* Interact */
	while (1)
	{
		/* Prompt */
		prt("Window flags (<dir> to move, 't' to toggle, or ESC)", 0, 0);

		/* Display the windows */
		for (j = 0; j < ANGBAND_TERM_MAX; j++)
		{
			byte a = TERM_WHITE;

			cptr s = angband_term_name[j];

			/* Use color */
			if (j == x) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
		}

		/* Display the options */
		for (i = 0; i < PW_MAX_FLAGS; i++)
		{
			byte a = TERM_WHITE;

			cptr str = window_flag_desc[i];

			/* Use color */
			if (i == y) a = TERM_L_BLUE;

			/* Unused option */
			if (!str) str = "(Unused option)";

			/* Flag name */
			Term_putstr(0, i + 5, -1, a, str);

			/* Display the windows */
			for (j = 0; j < ANGBAND_TERM_MAX; j++)
			{
				byte a = TERM_WHITE;

				char c = '.';

				/* Use color */
				if ((i == y) && (j == x)) a = TERM_L_BLUE;

				/* Active flag */
				if (op_ptr->window_flag[j] & (1L << i)) c = 'X';

				/* Flag value */
				Term_putch(35 + j * 5, i + 5, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + 5);

		/* Get key */
		ke = inkey_ex();

		/* Allow escape */
		if ((ke.key == ESCAPE) || (ke.key == 'q')) break;

		/* Mouse interaction */
		if (ke.key == '\xff')
		{
			int choicey = ke.mousey - 5;
			int choicex = (ke.mousex - 35)/5;

			if ((choicey >= 0) && (choicey < PW_MAX_FLAGS)
				&& (choicex > 0) && (choicex < ANGBAND_TERM_MAX)
				&& !(ke.mousex % 5))
			{
				y = choicey;
				x = (ke.mousex - 35)/5;
			}

			/* Toggle using mousebutton later */
			if (!ke.mousebutton) continue;
		}

		/* Toggle */
		if ((ke.key == '5') || (ke.key == 't') || ((ke.key == '\xff') && (ke.mousebutton)))
		{
			/* Hack -- ignore the main window */
			if (x == 0)
			{
				bell("Cannot set main window flags!");
			}

			/* Toggle flag (off) */
			else if (op_ptr->window_flag[x] & (1L << y))
			{
				op_ptr->window_flag[x] &= ~(1L << y);
			}

			/* Toggle flag (on) */
			else
			{
				op_ptr->window_flag[x] |= (1L << y);
			}

			/* Continue */
			continue;
		}

		/* Extract direction */
		d = target_dir(ke.key);

		/* Move */
		if (d != 0)
		{
			x = (x + ddx[d] + 8) % ANGBAND_TERM_MAX;
			y = (y + ddy[d] + 16) % PW_MAX_FLAGS;
		}

		/* Oops */
		else
		{
			bell("Illegal command for window options!");
		}
	}

	/* Notice changes */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		term *old = Term;

		/* Dead window */
		if (!angband_term[j]) continue;

		/* Ignore non-changes */
		if (op_ptr->window_flag[j] == old_flag[j]) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Erase */
		Term_clear();

		/* Refresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*
 * Write all current options to the given preference file in the
 * lib/user directory. Modified from KAmband 1.8.
 */
static errr option_dump(cptr fname)
{
	int i, j;

	FILE *fff;

	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff) return (-1);


	/* Skip some lines */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic option dump\n\n");

	/* Dump options (skip cheat, adult, score) */
	for (i = 0; i < OPT_CHEAT; i++)
	{
		/* Require a real option */
		if (!option_name(i)) continue;

		/* Comment */
		fprintf(fff, "# Option '%s'\n", option_desc(i));

		/* Dump the option */
		if (op_ptr->opt[i])
		{
			fprintf(fff, "Y:%s\n", option_name(i));
		}
		else
		{
			fprintf(fff, "X:%s\n", option_name(i));
		}

		/* Skip a line */
		fprintf(fff, "\n");
	}

	/* Dump window flags */
	for (i = 1; i < ANGBAND_TERM_MAX; i++)
	{
		/* Require a real window */
		if (!angband_term[i]) continue;

		/* Check each flag */
		for (j = 0; j < 32; j++)
		{
			/* Require a real flag */
			if (!window_flag_desc[j]) continue;

			/* Comment */
			fprintf(fff, "# Window '%s', Flag '%s'\n",
				angband_term_name[i], window_flag_desc[j]);

			/* Dump the flag */
			if (op_ptr->window_flag[i] & (1L << j))
			{
				fprintf(fff, "W:%d:%d:1\n", i, j);
			}
			else
			{
				fprintf(fff, "W:%d:%d:0\n", i, j);
			}

			/* Skip a line */
			fprintf(fff, "\n");
		}
	}

	/* Close */
	my_fclose(fff);

	/* Success */
	return (0);
}


void do_cmd_options_append(int dummy, const char *dummy2)
{
	char ftmp[80];

	/* Unused parameters */
	(void)dummy;
	(void)dummy2;

	/* Prompt */
	prt("Command: Append options to a file", 22, 0);
	prt("File: ", 23, 0);

	/* Default filename */
	sprintf(ftmp, "%s.prf", op_ptr->base_name);

	/* Ask for a file */
	if (!askfor_aux(ftmp, 80)) return;

	/* Dump the options */
	if (option_dump(ftmp))
	{
		/* Failure */
		msg_print("Failed!");
	}
	else
	{
		/* Success */
		msg_print("Done.");
	}
}

void do_cmd_reset_options(int dummy, const char *dummy2)
{
	/* Unused parameters */
	(void)dummy;
	(void)dummy2;

	/* Prompt */
	prt("Command: Reset all options to defaults", 22, 0);
	/* Verify */
	if (!get_check("Do you really want to reset all options? ")) return;

	option_set_defaults();

	/* Success!!! */
	msg_print("Done.");
}

/* Hack -- Base Delay Factor */
void do_cmd_delay(void)
{
	/* Prompt */
	prt("Command: Base Delay Factor", 22, 0);

	/* Get a new value */
	while (1)
	{
		char cx;
		int msec = op_ptr->delay_factor * op_ptr->delay_factor;
		prt(format("Current base delay factor: %d (%d msec)",
					   op_ptr->delay_factor, msec), 24, 0);
		prt("New base delay factor (0-9 or ESC to accept): ", 23, 0);

		cx = inkey();
		if (cx == ESCAPE) break;
		if (isdigit(cx)) op_ptr->delay_factor = D2I(cx);
		else bell("Illegal delay factor!");
	}
}

/* Hack -- hitpoint warning factor */
void do_cmd_hp_warn(void)
{
	/* Prompt */
	prt("Command: Hitpoint Warning", 22, 0);

	/* Get a new value */
	while (1)
	{
		char cx;
		prt(format("Current hitpoint warning: %2d%%",
			   op_ptr->hitpoint_warn * 10), 24, 0);
		prt("New hitpoint warning (0-9 or ESC to accept): ", 23, 0);

		cx = inkey();
		if (cx == ESCAPE) break;
		if (isdigit(cx)) op_ptr->hitpoint_warn = D2I(cx);
		else bell("Illegal hitpoint warning!");
	}
}


#ifdef ALLOW_MACROS

/*
 * Hack -- append all current macros to the given file
 */
static errr macro_dump(cptr fname)
{
	int i;

	FILE *fff;

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff) return (-1);


	/* Skip some lines */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic macro dump\n\n");

	/* Dump them */
	for (i = 0; i < macro__num; i++)
	{
		/* Start the macro */
		fprintf(fff, "# Macro '%d'\n\n", i);

		/* Extract the macro action */
		ascii_to_text(buf, sizeof(buf), macro__act[i]);

		/* Dump the macro action */
		fprintf(fff, "A:%s\n", buf);

		/* Extract the macro pattern */
		ascii_to_text(buf, sizeof(buf), macro__pat[i]);

		/* Dump the macro pattern */
		fprintf(fff, "P:%s\n", buf);

		/* End the macro */
		fprintf(fff, "\n\n");
	}

	/* Start dumping */
	fprintf(fff, "\n\n\n\n");


	/* Close */
	my_fclose(fff);

	/* Success */
	return (0);
}


/*
 * Hack -- ask for a "trigger" (see below)
 *
 * Note the complex use of the "inkey()" function from "util.c".
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 */
static void do_cmd_macro_aux(char *buf)
{
	char ch;

	int n = 0;

	char tmp[1024];


	/* Flush */
	flush();

	/* Do not process macros */
	inkey_base = TRUE;

	/* First key */
	ch = inkey();

	/* Read the pattern */
	while (ch != '\0')
	{
		/* Save the key */
		buf[n++] = ch;

		/* Do not process macros */
		inkey_base = TRUE;

		/* Do not wait for keys */
		inkey_scan = TRUE;

		/* Attempt to read a key */
		ch = inkey();
	}

	/* Terminate */
	buf[n] = '\0';

	/* Flush */
	flush();


	/* Convert the trigger */
	ascii_to_text(tmp, sizeof(tmp), buf);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);
}


/*
 * Hack -- ask for a keymap "trigger" (see below)
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 */
static void do_cmd_macro_aux_keymap(char *buf)
{
	char tmp[1024];


	/* Flush */
	flush();


	/* Get a key */
	buf[0] = inkey();
	buf[1] = '\0';


	/* Convert to ascii */
	ascii_to_text(tmp, sizeof(tmp), buf);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);


	/* Flush */
	flush();
}


/*
 * Hack -- Append all keymaps to the given file.
 *
 * Hack -- We only append the keymaps for the "active" mode.
 */
static errr keymap_dump(cptr fname)
{
	int i;

	FILE *fff;

	char buf[1024];

	int mode;


	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff) return (-1);


	/* Skip some lines */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic keymap dump\n\n");

	/* Dump them */
	for (i = 0; i < (int)N_ELEMENTS(keymap_act[mode]); i++)
	{
		char key[2] = "?";

		cptr act;

		/* Loop up the keymap */
		act = keymap_act[mode][i];

		/* Skip empty keymaps */
		if (!act) continue;

		/* Encode the action */
		ascii_to_text(buf, sizeof(buf), act);

		/* Dump the keymap action */
		fprintf(fff, "A:%s\n", buf);

		/* Convert the key into a string */
		key[0] = i;

		/* Encode the key */
		ascii_to_text(buf, sizeof(buf), key);

		/* Dump the keymap pattern */
		fprintf(fff, "C:%d:%s\n", mode, buf);

		/* Skip a line */
		fprintf(fff, "\n");
	}

	/* Skip some lines */
	fprintf(fff, "\n\n\n");


	/* Close */
	my_fclose(fff);

	/* Success */
	return (0);
}


#endif


/*
 * Interact with "macros"
 *
 * Could use some helpful instructions on this page.  XXX XXX XXX
 * CLEANUP
 */
void do_cmd_macros(void)
{
	key_event ke;

	char tmp[1024];

	char pat[1024];

	int mode;

	int line = 0;

	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save screen */
	screen_save();

	/* Clear screen */
	Term_clear();

	/* Process requests until done */
	while (1)
	{
		/* Describe */
		prt("Interact with Macros", 2, 0);


		/* Describe that action */
		prt("Current action (if any) shown below:", 20, 0);

		/* Analyze the current action */
		ascii_to_text(tmp, sizeof(tmp), macro_buffer);

		/* Display the current action */
		prt(tmp, 22, 0);


		/* Selections */
		c_prt(line == 4 ? TERM_L_BLUE : TERM_WHITE, "(1) Load a user pref file", 4, 5);
#ifdef ALLOW_MACROS
		c_prt(line == 5 ? TERM_L_BLUE : TERM_WHITE, "(2) Append macros to a file", 5, 5);
		c_prt(line == 6 ? TERM_L_BLUE : TERM_WHITE, "(3) Query a macro", 6, 5);
		c_prt(line == 7 ? TERM_L_BLUE : TERM_WHITE, "(4) Create a macro", 7, 5);
		c_prt(line == 8 ? TERM_L_BLUE : TERM_WHITE, "(5) Remove a macro", 8, 5);
		c_prt(line == 9 ? TERM_L_BLUE : TERM_WHITE, "(6) Append keymaps to a file", 9, 5);
		c_prt(line == 10 ? TERM_L_BLUE : TERM_WHITE, "(7) Query a keymap", 10, 5);
		c_prt(line == 11 ? TERM_L_BLUE : TERM_WHITE, "(8) Create a keymap", 11, 5);
		c_prt(line == 12 ? TERM_L_BLUE : TERM_WHITE, "(9) Remove a keymap", 12, 5);
		c_prt(line == 13 ? TERM_L_BLUE : TERM_WHITE, "(0) Enter a new action", 13, 5);
#endif /* ALLOW_MACROS */

		/* Prompt */
		prt("Command: ", 16, 0);

		/* Get a command */
		ke = inkey_ex();

		/* React to mouse movement */
		if ((ke.key == '\xff') && !(ke.mousebutton))
		{
			line = ke.mousey;
			continue;
		}

		/* Leave */
		if (ke.key == ESCAPE) break;

		/* Load a user pref file */
		if ((ke.key == '1') || ((ke.key == '\xff') && (ke.mousey == 4)))
		{
			/* Ask for and load a user pref file */
			do_cmd_pref_file_hack(16);
		}

#ifdef ALLOW_MACROS

		/* Save macros */
		else if ((ke.key == '2') || ((ke.key == '\xff') && (ke.mousey == 5)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Append macros to a file", 16, 0);

			/* Prompt */
			prt("File: ", 18, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Ask for a file */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Dump the macros */
			(void)macro_dump(ftmp);

			/* Prompt */
			msg_print("Appended macros.");
		}

		/* Query a macro */
		else if ((ke.key == '3') || ((ke.key == '\xff') && (ke.mousey == 6)))
		{
			int k;

			/* Prompt */
			prt("Command: Query a macro", 16, 0);

			/* Prompt */
			prt("Trigger: ", 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(pat);

			/* Get the action */
			k = macro_find_exact(pat);

			/* Nothing found */
			if (k < 0)
			{
				/* Prompt */
				msg_print("Found no macro.");
			}

			/* Found one */
			else
			{
				/* Obtain the action */
				my_strcpy(macro_buffer, macro__act[k], sizeof(macro_buffer));

				/* Analyze the current action */
				ascii_to_text(tmp, sizeof(tmp), macro_buffer);

				/* Display the current action */
				prt(tmp, 22, 0);

				/* Prompt */
				msg_print("Found a macro.");
			}
		}

		/* Create a macro */
		else if ((ke.key == '4') || ((ke.key == '\xff') && (ke.mousey == 7)))
		{
			/* Prompt */
			prt("Command: Create a macro", 16, 0);

			/* Prompt */
			prt("Trigger: ", 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(pat);

			/* Clear */
			clear_from(20);

			/* Prompt */
			prt("Action: ", 20, 0);

			/* Convert to text */
			ascii_to_text(tmp, sizeof(tmp), macro_buffer);

			/* Get an encoded action */
			if (askfor_aux(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);

				/* Link the macro */
				macro_add(pat, macro_buffer);

				/* Prompt */
				msg_print("Added a macro.");
			}
		}

		/* Remove a macro */
		else if ((ke.key == '5') || ((ke.key == '\xff') && (ke.mousey == 8)))
		{
			/* Prompt */
			prt("Command: Remove a macro", 16, 0);

			/* Prompt */
			prt("Trigger: ", 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(pat);

			/* Link the macro */
			macro_add(pat, pat);

			/* Prompt */
			msg_print("Removed a macro.");
		}

		/* Save keymaps */
		else if ((ke.key == '6') || ((ke.key == '\xff') && (ke.mousey == 9)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Append keymaps to a file", 16, 0);

			/* Prompt */
			prt("File: ", 18, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Ask for a file */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Dump the macros */
			(void)keymap_dump(ftmp);

			/* Prompt */
			msg_print("Appended keymaps.");
		}

		/* Query a keymap */
		else if ((ke.key == '7') || ((ke.key == '\xff') && (ke.mousey == 10)))
		{
			cptr act;

			/* Prompt */
			prt("Command: Query a keymap", 16, 0);

			/* Prompt */
			prt("Keypress: ", 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(pat);

			/* Look up the keymap */
			act = keymap_act[mode][(byte)(pat[0])];

			/* Nothing found */
			if (!act)
			{
				/* Prompt */
				msg_print("Found no keymap.");
			}

			/* Found one */
			else
			{
				/* Obtain the action */
				my_strcpy(macro_buffer, act, sizeof(macro_buffer));

				/* Analyze the current action */
				ascii_to_text(tmp, sizeof(tmp), macro_buffer);

				/* Display the current action */
				prt(tmp, 22, 0);

				/* Prompt */
				msg_print("Found a keymap.");
			}
		}

		/* Create a keymap */
		else if ((ke.key == '8') || ((ke.key == '\xff') && (ke.mousey == 11)))
		{
			/* Prompt */
			prt("Command: Create a keymap", 16, 0);

			/* Prompt */
			prt("Keypress: ", 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(pat);

			/* Clear */
			clear_from(20);

			/* Prompt */
			prt("Action: ", 20, 0);

			/* Convert to text */
			ascii_to_text(tmp, sizeof(tmp), macro_buffer);

			/* Get an encoded action */
			if (askfor_aux(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);

				/* Free old keymap */
				string_free(keymap_act[mode][(byte)(pat[0])]);

				/* Make new keymap */
				keymap_act[mode][(byte)(pat[0])] = string_make(macro_buffer);

				/* Prompt */
				msg_print("Added a keymap.");
			}
		}

		/* Remove a keymap */
		else if ((ke.key == '9') || ((ke.key == '\xff') && (ke.mousey == 12)))
		{
			/* Prompt */
			prt("Command: Remove a keymap", 16, 0);

			/* Prompt */
			prt("Keypress: ", 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(pat);

			/* Free old keymap */
			string_free(keymap_act[mode][(byte)(pat[0])]);

			/* Make new keymap */
			keymap_act[mode][(byte)(pat[0])] = NULL;

			/* Prompt */
			msg_print("Removed a keymap.");
		}

		/* Enter a new action */
		else if ((ke.key == '0') || ((ke.key == '\xff') && (ke.mousey == 13)))
		{
			/* Prompt */
			prt("Command: Enter a new action", 16, 0);

			/* Go to the correct location */
			Term_gotoxy(0, 22);

			/* Analyze the current action */
			ascii_to_text(tmp, sizeof(tmp), macro_buffer);

			/* Get an encoded action */
			if (askfor_aux(tmp, 80))
			{
				/* Extract an action */
				text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);
			}
		}

#endif /* ALLOW_MACROS */

		/* Oops */
		else
		{
			/* Oops */
			bell("Illegal command for macros!");
		}

		/* Flush messages */
		message_flush();

		/* Clear screen */
		Term_clear();
	}


	/* Load screen */
	screen_load();
}



/*
 * Interact with "visuals"
 */
void do_cmd_visuals(void)
{
	key_event ke;
	int cx;

	int i;

	FILE *fff;

	char buf[1024];

	const char *empty_symbol = "<< ? >>";
	const char *empty_symbol2 = "\0";
	const char *empty_symbol3 = "\0";

	int line = 0;

	if (use_trptile && use_bigtile)
	{
		empty_symbol = "// ?????? \\\\";
		empty_symbol2 = "   ??????   ";
		empty_symbol3 = "\\\\ ?????? //";
	}
	else if (use_dbltile && use_bigtile)
	{
		empty_symbol = "// ???? \\\\";
		empty_symbol2 = "\\\\ ???? //";
	}
	else if (use_trptile)
	{
		empty_symbol = "// ??? \\\\";
		empty_symbol2 = "   ???   ";
		empty_symbol3 = "\\\\ ??? //";
	}
	else if (use_dbltile)
	{
		empty_symbol = "// ?? \\\\";
		empty_symbol2 = "\\\\ ?? //";
	}
	else if (use_bigtile) empty_symbol = "<< ?? >>";

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save screen */
	screen_save();


	/* Clear screen */
	Term_clear();

	/* Interact until done */
	while (1)
	{
		/* Ask for a choice */
		prt("Interact with Visuals", 2, 0);

		/* Give some choices */
		c_prt(line == 4 ? TERM_L_BLUE : TERM_WHITE, "(1) Load a user pref file", 4, 5);
#ifdef ALLOW_VISUALS
		c_prt(line == 5 ? TERM_L_BLUE : TERM_WHITE, "(2) Dump monster attr/chars", 5, 5);
		c_prt(line == 6 ? TERM_L_BLUE : TERM_WHITE, "(3) Dump object attr/chars", 6, 5);
		c_prt(line == 7 ? TERM_L_BLUE : TERM_WHITE, "(4) Dump feature attr/chars", 7, 5);
		c_prt(line == 8 ? TERM_L_BLUE : TERM_WHITE, "(5) Dump flavor attr/chars", 8, 5);
		c_prt(line == 9 ? TERM_L_BLUE : TERM_WHITE, "(6) Change monster attr/chars", 9, 5);
		c_prt(line == 10 ? TERM_L_BLUE : TERM_WHITE, "(7) Change object attr/chars", 10, 5);
		c_prt(line == 11 ? TERM_L_BLUE : TERM_WHITE, "(8) Change feature attr/chars", 11, 5);
		c_prt(line == 12 ? TERM_L_BLUE : TERM_WHITE, "(9) Change flavor attr/chars", 12, 5);
#endif
		c_prt(line == 13 ? TERM_L_BLUE : TERM_WHITE, "(0) Reset visuals", 13, 5);

		/* Prompt */
		prt("Command: ", 15, 0);

		/* Prompt */
		ke = inkey_ex();

		/* React to mouse movement */
		if ((ke.key == '\xff') && !(ke.mousebutton))
		{
			line = ke.mousey;
			continue;
		}

		/* Done */
		if (ke.key == ESCAPE) break;

		/* Load a user pref file */
		if ((ke.key == '1') || ((ke.key == '\xff') && (ke.mousey == 4)))
		{
			/* Ask for and load a user pref file */
			do_cmd_pref_file_hack(15);
		}

#ifdef ALLOW_VISUALS

		/* Dump monster attr/chars */
		else if ((ke.key == '2') || ((ke.key == '\xff') && (ke.mousey == 5)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Dump monster attr/chars", 15, 0);

			/* Prompt */
			prt("File: ", 17, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Get a filename */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

			/* Append to the file */
			fff = my_fopen(buf, "a");

			/* Failure */
			if (!fff) continue;


			/* Skip some lines */
			fprintf(fff, "\n\n");

			/* Start dumping */
			fprintf(fff, "# Monster attr/char definitions\n\n");

			/* Dump monsters */
			for (i = 0; i < z_info->r_max; i++)
			{
				monster_race *r_ptr = &r_info[i];
				char m_name[80];

				/* Skip non-entries */
				if (!r_ptr->name) continue;

				/* Get the name */
				race_desc(m_name, sizeof(m_name), i, 0x400, 1);
				
				/* Dump a comment */
				fprintf(fff, "# %s\n", m_name);

				/* Dump the monster attr/char info */
				fprintf(fff, "R:%d:0x%02X:0x%02X\n\n", i,
					(byte)(r_ptr->x_attr), (byte)(r_ptr->x_char));
			}

			/* All done */
			fprintf(fff, "\n\n\n\n");

			/* Close */
			my_fclose(fff);

			/* Message */
			msg_print("Dumped monster attr/chars.");
		}

		/* Dump object attr/chars */
		else if ((ke.key == '3') || ((ke.key == '\xff') && (ke.mousey == 6)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Dump object attr/chars", 15, 0);

			/* Prompt */
			prt("File: ", 17, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Get a filename */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

			/* Append to the file */
			fff = my_fopen(buf, "a");

			/* Failure */
			if (!fff) continue;


			/* Skip some lines */
			fprintf(fff, "\n\n");

			/* Start dumping */
			fprintf(fff, "# Object attr/char definitions\n\n");

			/* Dump objects */
			for (i = 0; i < z_info->k_max; i++)
			{
				object_kind *k_ptr = &k_info[i];

				/* Skip non-entries */
				if (!k_ptr->name) continue;

				/* Dump a comment */
				fprintf(fff, "# %s\n", (k_name + k_ptr->name));

				/* Dump the object attr/char info */
				fprintf(fff, "K:%d:0x%02X:0x%02X\n\n", i,
					(byte)(k_ptr->x_attr), (byte)(k_ptr->x_char));
			}

			/* All done */
			fprintf(fff, "\n\n\n\n");

			/* Close */
			my_fclose(fff);

			/* Message */
			msg_print("Dumped object attr/chars.");
		}

		/* Dump feature attr/chars */
		else if ((ke.key == '4') || ((ke.key == '\xff') && (ke.mousey == 7)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Dump feature attr/chars", 15, 0);

			/* Prompt */
			prt("File: ", 17, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Get a filename */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

			/* Append to the file */
			fff = my_fopen(buf, "a");

			/* Failure */
			if (!fff) continue;


			/* Skip some lines */
			fprintf(fff, "\n\n");

			/* Start dumping */
			fprintf(fff, "# Feature attr/char definitions\n\n");

			/* Dump features */
			for (i = 0; i < z_info->f_max; i++)
			{
				feature_type *f_ptr = &f_info[i];

				/* Skip non-entries */
				if (!f_ptr->name) continue;

				/* Skip mimic entries -- except invisible trap */
				if ((f_ptr->mimic != i) && (i != FEAT_INVIS)) continue;

				/* Dump a comment */
				fprintf(fff, "# %s\n", (f_name + f_ptr->name));

				/* Dump the feature attr/char info */
				fprintf(fff, "F:%d:0x%02X:0x%02X:%d:%s%s%s%s\n\n", i,
					(byte)(f_ptr->x_attr), (byte)(f_ptr->x_char),f_ptr->under,
						(f_ptr->flags3 & (FF3_ATTR_LITE)) ? "A" : "",
						(f_ptr->flags3 & (FF3_ATTR_ITEM)) ? "F" : "",
						(f_ptr->flags3 & (FF3_ATTR_DOOR)) ? "D" : "",
						(f_ptr->flags3 & (FF3_ATTR_WALL)) ? "W" : "");
			}

			/* All done */
			fprintf(fff, "\n\n\n\n");

			/* Close */
			my_fclose(fff);

			/* Message */
			msg_print("Dumped feature attr/chars.");
		}

		/* Dump flavor attr/chars */
		else if ((ke.key == '5') || ((ke.key == '\xff') && (ke.mousey == 8)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Dump flavor attr/chars", 15, 0);

			/* Prompt */
			prt("File: ", 17, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Get a filename */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

			/* Append to the file */
			fff = my_fopen(buf, "a");

			/* Failure */
			if (!fff) continue;


			/* Skip some lines */
			fprintf(fff, "\n\n");

			/* Start dumping */
			fprintf(fff, "# Flavor attr/char definitions\n\n");

			/* Dump flavors */
			for (i = 0; i < z_info->x_max; i++)
			{
				flavor_type *x_ptr = &x_info[i];

				/* Dump a comment */
				fprintf(fff, "# %s\n", (x_text + x_ptr->text));

				/* Dump the flavor attr/char info */
				fprintf(fff, "L:%d:0x%02X:0x%02X\n\n", i,
					(byte)(x_ptr->x_attr), (byte)(x_ptr->x_char));
			}

			/* All done */
			fprintf(fff, "\n\n\n\n");

			/* Close */
			my_fclose(fff);

			/* Message */
			msg_print("Dumped flavor attr/chars.");
		}

		/* Modify monster attr/chars */
		else if ((ke.key == '6') || ((ke.key == '\xff') && (ke.mousey == 9)))
		{
			static int r = 0;

			/* Prompt */
			prt("Command: Change monster attr/chars", 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				monster_race *r_ptr = &r_info[r];
				char m_name[80];

				byte da = (byte)(r_ptr->d_attr);
				byte dc = (byte)(r_ptr->d_char);
				byte ca = (byte)(r_ptr->x_attr);
				byte cc = (byte)(r_ptr->x_char);

				int linec = (use_trptile ? 22: (use_dbltile ? 21 : 20));

				/* Get the name */
				race_desc(m_name, sizeof(m_name), r, 0x400, 1);
				
				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Monster = %d, Name = %-40.40s",
						   r, m_name));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3u / %3u", da, dc));
				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol3);

				Term_putch(43, 19, da, dc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, 19, da, dc);
				}

				/* Label the Current values */
				Term_putstr(10, linec, -1, TERM_WHITE,
					    format("Current attr/char = %3u / %3u", ca, cc));
				Term_putstr(40, linec, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, linec+1, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, linec+2, -1, TERM_WHITE, empty_symbol3);
				Term_putch(43, linec, ca, cc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, linec++, ca, cc);
				}
				if (use_trptile) linec++;

				/* Prompt */
				Term_putstr(0, linec + 2, -1, TERM_WHITE,
					    "Command (n/N/a/A/c/C): ");

				/* Get a command */
				cx = inkey();

				/* All done */
				if (cx == ESCAPE) break;

				/* Analyze */
				if (cx == 'n') r = (r + z_info->r_max + 1) % z_info->r_max;
				if (cx == 'N') r = (r + z_info->r_max - 1) % z_info->r_max;
				if (cx == 'a') r_ptr->x_attr = (byte)(ca + 1);
				if (cx == 'A') r_ptr->x_attr = (byte)(ca - 1);
				if (cx == 'c') r_ptr->x_char = (byte)(cc + 1);
				if (cx == 'C') r_ptr->x_char = (byte)(cc - 1);
			}
		}

		/* Modify object attr/chars */
		else if ((ke.key == '7') || ((ke.key == '\xff') && (ke.mousey == 10)))
		{
			static int k = 0;

			/* Prompt */
			prt("Command: Change object attr/chars", 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				object_kind *k_ptr = &k_info[k];

				byte da = (byte)(k_ptr->d_attr);
				byte dc = (byte)(k_ptr->d_char);
				byte ca = (byte)(k_ptr->x_attr);
				byte cc = (byte)(k_ptr->x_char);

				int linec = (use_trptile ? 22: (use_dbltile ? 21 : 20));

				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Object = %d, Name = %-40.40s",
						   k, (k_name + k_ptr->name)));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3u / %3u", da, dc));
				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol3);

				Term_putch(43, 19, da, dc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, 19, da, dc);
				}

				/* Label the Current values */
				Term_putstr(10, linec, -1, TERM_WHITE,
					    format("Current attr/char = %3u / %3u", ca, cc));
				Term_putstr(40, linec, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, linec+1, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, linec+2, -1, TERM_WHITE, empty_symbol3);
				Term_putch(43, linec, ca, cc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, linec++, ca, cc);
				}
				if (use_trptile) linec++;

				/* Prompt */
				Term_putstr(0, linec+2, -1, TERM_WHITE,
					    "Command (n/N/a/A/c/C): ");

				/* Get a command */
				cx = inkey();

				/* All done */
				if (cx == ESCAPE) break;

				/* Analyze */
				if (cx == 'n') k = (k + z_info->k_max + 1) % z_info->k_max;
				if (cx == 'N') k = (k + z_info->k_max - 1) % z_info->k_max;
				if (cx == 'a') k_info[k].x_attr = (byte)(ca + 1);
				if (cx == 'A') k_info[k].x_attr = (byte)(ca - 1);
				if (cx == 'c') k_info[k].x_char = (byte)(cc + 1);
				if (cx == 'C') k_info[k].x_char = (byte)(cc - 1);
			}
		}

		/* Modify feature attr/chars */
		else if ((ke.key == '8') || ((ke.key == '\xff') && (ke.mousey == 11)))
		{
			static int f = 0;

			/* Prompt */
			prt("Command: Change feature attr/chars", 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				feature_type *f_ptr = &f_info[f];

				byte da = (byte)(f_ptr->d_attr);
				byte dc = (byte)(f_ptr->d_char);
				byte ca = (byte)(f_ptr->x_attr);
				byte cc = (byte)(f_ptr->x_char);

				int linec = (use_trptile ? 22: (use_dbltile ? 21 : 20));

				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Terrain = %d, Name = %-40.40s",
						   f, (f_name + f_ptr->name)));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3u / %3u", da, dc));
				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol3);

				Term_putch(43, 19, da, dc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, 19, da, dc);
				}

				/* Label the Current values */
				Term_putstr(10, linec, -1, TERM_WHITE,
					    format("Current attr/char = %3u / %3u", ca, cc));
				Term_putstr(40, linec, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, linec+1, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, linec+2, -1, TERM_WHITE, empty_symbol3);
				Term_putch(43, linec, ca, cc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, linec++, ca, cc);
				}
				if (use_trptile) linec++;

				/* Prompt */
				Term_putstr(0, linec+2, -1, TERM_WHITE,
					    "Command (n/N/a/A/c/C): ");

				/* Get a command */
				cx = inkey();

				/* All done */
				if (cx == ESCAPE) break;

				/* Analyze */
				if (cx == 'n') f = (f + z_info->f_max + 1) % z_info->f_max;
				if (cx == 'N') f = (f + z_info->f_max - 1) % z_info->f_max;
				if (cx == 'a') f_info[f].x_attr = (byte)(ca + 1);
				if (cx == 'A') f_info[f].x_attr = (byte)(ca - 1);
				if (cx == 'c') f_info[f].x_char = (byte)(cc + 1);
				if (cx == 'C') f_info[f].x_char = (byte)(cc - 1);
			}
		}

		/* Modify flavor attr/chars */
		else if ((ke.key == '9') || ((ke.key == '\xff') && (ke.mousey == 12)))
		{
			static int f = 0;

			/* Prompt */
			prt("Command: Change flavor attr/chars", 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				flavor_type *x_ptr = &x_info[f];

				byte da = (byte)(x_ptr->d_attr);
				byte dc = (byte)(x_ptr->d_char);
				byte ca = (byte)(x_ptr->x_attr);
				byte cc = (byte)(x_ptr->x_char);

				int linec = (use_trptile ? 22: (use_dbltile ? 21 : 20));

				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Flavor = %d, Text = %-40.40s",
						   f, (x_text + x_ptr->text)));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3u / %3u", da, dc));
				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol3);

				Term_putch(43, 19, da, dc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, 19, da, dc);
				}

				/* Label the Current values */
				Term_putstr(10, linec, -1, TERM_WHITE,
					    format("Current attr/char = %3u / %3u", ca, cc));
				Term_putstr(40, linec, -1, TERM_WHITE, empty_symbol);
				if (use_dbltile || use_trptile) Term_putstr (40, linec+1, -1, TERM_WHITE, empty_symbol2);
				if (use_trptile) Term_putstr (40, linec+2, -1, TERM_WHITE, empty_symbol3);
				Term_putch(43, linec, ca, cc);

				if (use_bigtile || use_dbltile || use_trptile)
				{
					big_putch(43, linec++, ca, cc);
				}
				if (use_trptile) linec++;

				/* Prompt */
				Term_putstr(0, linec+2, -1, TERM_WHITE,
					    "Command (n/N/a/A/c/C): ");

				/* Get a command */
				cx = inkey();

				/* All done */
				if (cx == ESCAPE) break;

				/* Analyze */
				if (cx == 'n') f = (f + z_info->x_max + 1) % z_info->x_max;
				if (cx == 'N') f = (f + z_info->x_max - 1) % z_info->x_max;
				if (cx == 'a') x_info[f].x_attr = (byte)(ca + 1);
				if (cx == 'A') x_info[f].x_attr = (byte)(ca - 1);
				if (cx == 'c') x_info[f].x_char = (byte)(cc + 1);
				if (cx == 'C') x_info[f].x_char = (byte)(cc - 1);
			}
		}

#endif /* ALLOW_VISUALS */

		/* Reset visuals */
		else if ((ke.key == '0') || ((ke.key == '\xff') && (ke.mousey == 13)))
		{
			/* Reset */
			reset_visuals(TRUE);

			/* Message */
			msg_print("Visual attr/char tables reset.");
		}

		/* Unknown option */
		else
		{
			bell("Illegal command for visuals!");
		}

		/* Flush messages */
		message_flush();

		/* Clear screen */
		Term_clear();
	}


	/* Load screen */
	screen_load();
}


/*
 * Interact with "colors"
 */
void do_cmd_colors(void)
{
	key_event ke;
	int cx;

	int i;

	FILE *fff;

	char buf[1024];

	int line = 0;

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save screen */
	screen_save();

	/* Clear screen */
	Term_clear();

	/* Interact until done */
	while (1)
	{
		/* Ask for a choice */
		prt("Interact with Colors", 2, 0);

		/* Give some choices */
		c_prt(line == 4 ? TERM_L_BLUE : TERM_WHITE, "(1) Load a user pref file", 4, 5);
#ifdef ALLOW_COLORS
		c_prt(line == 5 ? TERM_L_BLUE : TERM_WHITE, "(2) Dump colors", 5, 5);
		c_prt(line == 6 ? TERM_L_BLUE : TERM_WHITE, "(3) Modify colors", 6, 5);
#endif /* ALLOW_COLORS */

		/* Prompt */
		prt("Command: ", 8, 0);

		/* Prompt */
		ke = inkey_ex();

		/* React to mouse movement */
		if ((ke.key == '\xff') && !(ke.mousebutton))
		{
			line = ke.mousey;
			continue;
		}

		/* Done */
		if (ke.key == ESCAPE) break;

		/* Load a user pref file */
		if ((ke.key == '1') || ((ke.key == '\xff') && (ke.mousey == 4)))
		{
			/* Ask for and load a user pref file */
			do_cmd_pref_file_hack(8);

			/* Could skip the following if loading cancelled XXX XXX XXX */

			/* Mega-Hack -- React to color changes */
			Term_xtra(TERM_XTRA_REACT, 0);

			/* Mega-Hack -- Redraw physical windows */
			Term_redraw();
		}

#ifdef ALLOW_COLORS

		/* Dump colors */
		else if ((ke.key == '2') || ((ke.key == '\xff') && (ke.mousey == 5)))
		{
			char ftmp[80];

			/* Prompt */
			prt("Command: Dump colors", 8, 0);

			/* Prompt */
			prt("File: ", 10, 0);

			/* Default filename */
			sprintf(ftmp, "%s.prf", op_ptr->base_name);

			/* Get a filename */
			if (!askfor_aux(ftmp, 80)) continue;

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

			/* Append to the file */
			fff = my_fopen(buf, "a");

			/* Failure */
			if (!fff) continue;


			/* Skip some lines */
			fprintf(fff, "\n\n");

			/* Start dumping */
			fprintf(fff, "# Color redefinitions\n\n");

			/* Dump colors */
			for (i = 0; i < 256; i++)
			{
				int kv = angband_color_table[i][0];
				int rv = angband_color_table[i][1];
				int gv = angband_color_table[i][2];
				int bv = angband_color_table[i][3];

				cptr name = "unknown";

				/* Skip non-entries */
				if (!kv && !rv && !gv && !bv) continue;

				/* Extract the color name */
				if (i < BASE_COLORS) name = color_table[i].name;

				/* Dump a comment */
				fprintf(fff, "# Color '%s'\n", name);

				/* Dump the monster attr/char info */
				fprintf(fff, "V:%d:0x%02X:0x%02X:0x%02X:0x%02X\n\n",
					i, kv, rv, gv, bv);
			}

			/* All done */
			fprintf(fff, "\n\n\n\n");

			/* Close */
			my_fclose(fff);

			/* Message */
			msg_print("Dumped color redefinitions.");
		}

		/* Edit colors */
		else if ((ke.key == '3') || ((ke.key == '\xff') && (ke.mousey == 6)))
		{
			static byte a = 0;

			/* Prompt */
			prt("Command: Modify colors", 8, 0);

			/* Hack -- query until done */
			while (1)
			{
				cptr name;

				/* Clear */
				clear_from(10);

				/* Exhibit the normal colors */
				for (i = 0; i < 16; i++)
				{
					/* Exhibit this color */
					Term_putstr(i*4, 20, -1, a, "###");

					/* Exhibit all colors */
					Term_putstr(i*4, 22, -1, (byte)i, format("%3d", i));
				}

				/* Describe the color */
				name = ((a < BASE_COLORS) ? color_table[a].name : "undefined");

				/* Describe the color */
				Term_putstr(5, 10, -1, TERM_WHITE,
					    format("Color = %d, Name = %s", a, name));

				/* Label the Current values */
				Term_putstr(5, 12, -1, TERM_WHITE,
					    format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x",
						   angband_color_table[a][0],
						   angband_color_table[a][1],
						   angband_color_table[a][2],
						   angband_color_table[a][3]));

				/* Prompt */
				Term_putstr(0, 14, -1, TERM_WHITE,
					    "Command (n/N/k/K/r/R/g/G/b/B): ");

				/* Get a command */
				cx = inkey();

				/* All done */
				if (cx == ESCAPE) break;

				/* Analyze */
				if (cx == 'n') a = (byte)(a + 1);
				if (cx == 'N') a = (byte)(a - 1);
				if (cx == 'k') angband_color_table[a][0] = (byte)(angband_color_table[a][0] + 1);
				if (cx == 'K') angband_color_table[a][0] = (byte)(angband_color_table[a][0] - 1);
				if (cx == 'r') angband_color_table[a][1] = (byte)(angband_color_table[a][1] + 1);
				if (cx == 'R') angband_color_table[a][1] = (byte)(angband_color_table[a][1] - 1);
				if (cx == 'g') angband_color_table[a][2] = (byte)(angband_color_table[a][2] + 1);
				if (cx == 'G') angband_color_table[a][2] = (byte)(angband_color_table[a][2] - 1);
				if (cx == 'b') angband_color_table[a][3] = (byte)(angband_color_table[a][3] + 1);
				if (cx == 'B') angband_color_table[a][3] = (byte)(angband_color_table[a][3] - 1);

				/* Hack -- react to changes */
				Term_xtra(TERM_XTRA_REACT, 0);

				/* Hack -- redraw */
				Term_redraw();
			}
		}

#endif /* ALLOW_COLORS */

		/* Unknown option */
		else
		{
			bell("Illegal command for colors!");
		}

		/* Flush messages */
		message_flush();

		/* Clear screen */
		Term_clear();
	}


	/* Load screen */
	screen_load();
}



/*
 * Hack -- append all current auto-inscriptions to the given file
 */
static errr cmd_autos_dump(void)
{
	int i;

	FILE *fff;

	char buf[1024];
	char ftmp[80];

	/* Prompt */
	prt("Command: Dump auto-inscriptions", 22, 0);
	/* Prompt */
	prt("File: ", 23, 0);

	/* Default filename */
	sprintf(ftmp, "%s.prf", op_ptr->base_name);

	/* Get a filename */
	if (!askfor_aux(ftmp, 80)) return -1;

	/* Drop priv's */
	safe_setuid_drop();


	/* Dump the macros */

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, ftmp);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff) return (-1);

	/* Skip some lines */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic auto-inscription dump\n\n");

	/* Dump them */
	for (i = 0; i < z_info->k_max; i++)
	{
		if (k_info[i].note)
		{

			/* Start the macro */
			fprintf(fff, "# Kind '%s'\n\n", k_name + k_info[i].name);

			/* Dump the kind */
			fprintf(fff, "I:K:%d:%s\n", i, quark_str(k_info[i].note));

			/* End the inscription */
			fprintf(fff, "\n\n");

		}
	}

	/* Dump them */
	for (i = 0; i < z_info->e_max; i++)
	{
		if (e_info[i].note)
		{
			/* Start the macro */
			fprintf(fff, "# Ego item '%s'\n\n", e_name + e_info[i].name);

			/* Dump the kind */
			fprintf(fff, "I:E:%d:%s\n", i, quark_str(e_info[i].note));

			/* End the inscription */
			fprintf(fff, "\n\n");
		}
	}

#if 0
	/* Dump them */
	for (i = 0; i < z_info->r_max; i++)
	{
		if (r_info[i].note)
		{
			char m_name[80];
			
			/* Get the name */
			race_desc(m_name, sizeof(m_name), r_idx, 0x400, 1);

			/* Start the macro */
			fprintf(fff, "# Monster race '%s'\n\n", m_name);

			/* Dump the kind */
			fprintf(fff, "I:R:%d:%s\n", i, quark_str(r_info[i].note));

			/* End the inscription */
			fprintf(fff, "\n\n");
		}
	}
#endif

	/* Start dumping */
	fprintf(fff, "\n\n\n\n");

	/* Close */
	my_fclose(fff);

	/* Grab priv's */
	safe_setuid_grab();

	/* Message */
	msg_print("Appended auto-inscriptions.");

	/* Success */
	return (0);
}


/*
 * Note something in the message recall
 */
void do_cmd_note(void)
{
	char tmp[80];

	/* Default */
	my_strcpy(tmp, "",sizeof(tmp));

	/* Input */
	if (!get_string("Note: ", tmp, sizeof(tmp))) return;

	/* Ignore empty notes */
	if (!tmp[0] || (tmp[0] == ' ')) return;

	/* Add the note to the message recall */
	msg_format("Note: %s", tmp);
}


/*
 * Mention the current version
 */
void do_cmd_version(void)
{
	/* Silly message */
	msg_format("You are playing %s %s.  Type '?' for more info.",
		   VERSION_NAME, VERSION_STRING);
}


/*
 * Array of feeling strings
 */
static cptr do_cmd_feeling_text[11] =
{
	"You are still uncertain about this place...",
	"You feel there is something special here...",
	"Premonitions of death appall you!  This place is murderous!",
	"This place feels terribly dangerous!",
	"You have a nasty feeling about this place.",
	"You have a bad feeling about this place.",
	"You feel nervous.",
	"You have an uneasy feeling.",
	"You have a faint uneasy feeling.",
	"This place seems reasonably safe.",
	"This seems a quiet, peaceful place."
};

/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(void)
{
	/* Verify the feeling */
	if (feeling > 10) feeling = 10;

	/* No useful feeling in town */
	if (is_typical_town(p_ptr->dungeon, p_ptr->depth))
	{
		msg_print("Looks like a typical town.");
		return;
	}

	/* Wilderness is easy to escape, so make it harder by no level feeling */
	if (level_flag & (LF1_SURFACE))
	{
		switch (p_ptr->dungeon % 3)
		{
		case 0:
		{
			msg_print("You feel free as a bird.");
			return;
		}
		case 1:
		{
			msg_print("You feel the future unfolds before you.");
			return;
		}
		case 2:
		{
			msg_print("You feel something new is about to begin.");
			return;
		}
		}
	}

	/* Display the feeling */
	msg_print(do_cmd_feeling_text[feeling]);
}


/*
 * Display the time and date
 *
 * Note that "time of day" is important to determine when quest
 * monsters appear in surface locations.
 */
void do_cmd_timeofday()
{
	int day = bst(DAY, turn);

	int hour = bst(HOUR, turn);

	int min = bst(MINUTE, turn);

	char buf2[20];

	/* Format time of the day */
	strnfmt(buf2, 20, get_day(bst(YEAR, turn) + START_YEAR));

	/* Display current date in the Elvish calendar */
	msg_format("This is %s of the %s year of the third age.",
	           get_month_name(day, cheat_xtra, FALSE), buf2);

	/* Message */
	if (cheat_xtra)
	{
		msg_format("The time is %d:%02d %s.",
	           (hour % 12 == 0) ? 12 : (hour % 12),
	           min, (hour < 12) ? "AM" : "PM");
	}
	else
	{
		if (hour == SUNRISE - 1)
			msg_print("The sun is rising.");
		else if (hour < SUNRISE)
			msg_format("The sun rises in %d hours.", SUNRISE - hour);
		else if (hour == MIDDAY - 1)
			msg_print("It is almost midday.");
		else if (hour < MIDDAY)
			msg_format("There are %d hours until midday.", MIDDAY - hour);
		else if (hour == SUNSET - 1)
			msg_print("The sun is setting.");
		else if (hour < SUNSET)
			msg_format("The sun sets in %d hours.", SUNSET - hour);
		else if (hour == MIDNIGHT - 1)
			msg_print("It is almost midnight.");
		else if (hour < MIDNIGHT)
			msg_format("There are %d hours until midnight.", MIDNIGHT - hour);
	}
}


/*
 * Array of event pronouns strings - initial caps
 */
static cptr event_pronoun_text_caps[6] =
{
	"We ",
	"I ",
	"You ",
	"He ",
	"She ",
	"It "
};

/*
 * Array of event pronouns strings - comma
 */
static cptr event_pronoun_text[6] =
{
	"we ",
	"I ",
	"you ",
	"he ",
	"she ",
	"it "
};

/*
 * Array of event pronouns strings - comma
 */
static cptr event_pronoun_text_which[6] =
{
	"which we ",
	"which I ",
	"which you ",
	"which he ",
	"which she ",
	"which it "
};

/*
 * Array of tense strings (0, 1 and 2 occurred in past, however 0, 1 only are past tense )
 *
 */
static cptr event_tense_text[8] =
{
	" ",
	"have ",
	"had to ",
	"must ",
	"will ",
	" ",
	"can ",
	"may "
};

/*
 * Display a quest event.
 */
bool print_event(quest_event *event, int pronoun, int tense, cptr prefix)
{
	int vn, n;
	cptr vp[64];

	bool reflex_feature = FALSE;
	bool reflex_monster = FALSE;
	bool reflex_object = FALSE;
	bool used_num = FALSE;
	bool used_race = FALSE;
	bool output = FALSE;
	bool intro = FALSE;

	/* Describe location */
	if ((event->dungeon) || (event->level))
	{
		/* Collect travel actions */
		vn = 0;
		if (tense > 1)
		{
			if (event->flags & (EVENT_TRAVEL)) vp[vn++] = "travel to";
			if (event->flags & (EVENT_STAY))
			{
				if (event->level) vp[vn++] = "stay";
				else vp[vn++] = "stay in";
			}
			if (event->flags & (EVENT_LEAVE)) vp[vn++] = "leave";
		}
		else
		{
			if (event->flags & (EVENT_TRAVEL)) vp[vn++] = "travelled to";
			if (event->flags & (EVENT_STAY))
			{
				if (event->level) vp[vn++] = "stayed";
				else vp[vn++] = "stayed in";
			}
			if (event->flags & (EVENT_LEAVE)) vp[vn++] = "left";
		}

		/* Describe travel actions */
		if (vn)
		{
			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0)
				{
					if (prefix)
					{
						text_out(prefix);
						text_out(event_pronoun_text[pronoun]);
					}
					else
					{
						text_out(event_pronoun_text_caps[pronoun]);
					}
					text_out(event_tense_text[tense]);
				}
				else if (n < vn-1) text_out(", ");
				else text_out(" and ");

				/* Dump */
				text_out(vp[n]);
			}

			text_out(" ");
		}

		if ((!vn) && (prefix))
		{
			text_out(prefix);
			intro = TRUE;
		}

		if (event->level)
		{
			if ((!intro) && (!vn)) text_out("On ");
			text_out(format("level %d of ", event->level));
		}
		else if ((intro) && (!vn)) text_out(" in ");
		else if (!vn) text_out("In ");

		/* TODO: full level 0 name? level n name? */
		text_out(t_name + t_info[event->dungeon].name);

		if ((event->owner) || (event->store) || ((event->feat) && (event->action)) ||
			(event->race) || (event->kind) || (event->ego_item_type) || (event->artifact))
		{
			if (!vn)
			{
				text_out(event_pronoun_text[pronoun]);
				text_out(event_tense_text[tense]);
			}
			else text_out(" and ");
		}

		intro = TRUE;
		output = TRUE;
	}

	/* Visit a store */
	if ((event->owner) || (event->store) || (event->flags & (EVENT_TALK_STORE | EVENT_DEFEND_STORE)))
	{
		output = TRUE;

		if (!intro)
		{
			if (prefix)
			{
				text_out(prefix);
				text_out(event_pronoun_text[pronoun]);
			}
			else
			{
				text_out(event_pronoun_text_caps[pronoun]);
			}
			text_out(event_tense_text[tense]);

			intro = TRUE;
		}

		if (event->flags & (EVENT_DEFEND_STORE))
		{
			if (tense < 0) text_out("defended ");
			else text_out("defend ");

			used_race = TRUE;
		}
		else if (event->flags & (EVENT_TALK_STORE))
		{
			if (tense < 0) text_out("talked to");
			else text_out("talk to");
		}
		else if (tense < 0) text_out("visited ");
		else text_out("visit ");

		if (!(event->owner) && !(event->store))
		{
			text_out("the town");
		}
		else if (event->owner)
		{
			/* XXX owner name */;
			if (event->store) text_out(" at ");
		}

		if (event->store)
		{
			text_out(f_name + f_info[event->feat].name);
		}

		if ((event->flags & (EVENT_DEFEND_STORE)) && !(event->room_type_a) && !(event->room_type_b)
			&& !(event->room_flags) && !((event->feat) && (event->action)))
		{
			text_out(" from ");
			reflex_monster = TRUE;
		}
		else if (((event->feat) && (event->action)) || (event->room_type_a) || (event->room_type_b)
			|| (event->room_flags) || (event->race) || ((event->kind) || (event->ego_item_type) || (event->artifact)))
		{
			text_out(" and ");
		}
		else text_out(" ");

		output = TRUE;
	}

	/* Affect room */
	if ((event->room_type_a) || (event->room_type_b) || (event->room_flags))
	{
		/* Collect room flags */
		vn = 0;
		if (tense > 1)
		{
			if (event->flags & (EVENT_FIND_ROOM)) vp[vn++] = "find";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_HIDDEN))) vp[vn++] = "reveal";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_SEALED))) vp[vn++] = "unseal";
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_ENTERED))) vp[vn++] = "enter";
#if 0
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_LITE))) vp[vn++] = "light up";
#endif
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_SEEN))) vp[vn++] = "explore";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_LAIR))) vp[vn++] = "clear of monsters";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_OBJECT))) vp[vn++] = "empty of objects";
#if 0
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_TRAP))) vp[vn++] = "disarm";
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_DARK))) vp[vn++] = "darken";
#endif
		}
		else
		{
			if (event->flags & (EVENT_FIND_ROOM)) vp[vn++] = "found";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_HIDDEN))) vp[vn++] = "revealed";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_SEALED))) vp[vn++] = "unsealed";
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_ENTERED))) vp[vn++] = "entered";
#if 0
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_LITE))) vp[vn++] = "lit up";
#endif
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_SEEN))) vp[vn++] = "explored";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_LAIR))) vp[vn++] = "cleared of monsters";
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_OBJECT))) vp[vn++] = "emptied of objects";
#if 0
			if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_TRAP))) vp[vn++] = "disarmed";
			if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_DARK))) vp[vn++] = "darkened";
#endif
		}

		if (vn)
		{
			output = TRUE;

			if (!intro)
			{
				if (prefix)
				{
					text_out(prefix);
					text_out(event_pronoun_text[pronoun]);
				}
				else
				{
					text_out(event_pronoun_text_caps[pronoun]);
				}
				text_out(event_tense_text[tense]);

				intro = TRUE;
			}

			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0) ;
				else if (n < vn-1) text_out(", ");
				else text_out(" or ");

				/* Dump */
				text_out(vp[n]);
			}

			text_out(" a ");

			/* Room adjective */
			if (event->room_type_a) { text_out(d_name + d_info[event->room_type_a].name1); text_out(" "); }

			/* Room noun */
			if (!(event->room_type_a) && !(event->room_type_b)) text_out("all the rooms");
			else if (event->room_type_b) text_out(d_name + d_info[event->room_type_b].name1);
			else text_out("room");

			if (((event->feat) && (event->action)) || (event->race) || ((event->kind) || (event->ego_item_type) || (event->artifact)))
			{
				if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_SEALED)) && (event->room_flags & (ROOM_HIDDEN))) text_out(" hidden and sealed by ");
				else if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_SEALED))) text_out(" sealed by ");
				else if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_HIDDEN))) text_out(" hidden by ");
				else if ((event->flags & (EVENT_UNFLAG_ROOM)) || (event->flags & (EVENT_FLAG_ROOM))) text_out(" enchanted by ");
				else if ((event->feat) && (event->action))
				{
					text_out(" containing ");

					reflex_feature = TRUE;
				}
				else if (event->race)
				{
					text_out(" guarded by ");

					reflex_monster = TRUE;
				}
				else if ((event->kind) || (event->ego_item_type) || (event->artifact))
				{
					text_out(" containing ");

					reflex_object = TRUE;
				}

				if (event->race) used_race = TRUE;
			}
		}
	}

	/* Alter a feature */
	if ((event->feat) && (event->action))
	{
		/* Collect monster actions */
		vn = 0;
		if (tense > 1)
		{
			switch(event->action)
			{
				case FS_SECRET: vp[vn++] = "find"; break;
				case FS_OPEN: vp[vn++] = "open"; break;
				case FS_CLOSE: vp[vn++] = "close"; break;
				case FS_BASH: vp[vn++] = "bash"; break;
				case FS_SPIKE: vp[vn++] = "spike"; break;
				case FS_DISARM: vp[vn++] = "disarm"; break;
				case FS_TUNNEL: vp[vn++] = "dig through"; break;
				case FS_HIT_TRAP: vp[vn++] = "set off"; break;
				case FS_HURT_ROCK: vp[vn++] = "turn to mud"; break;
				case FS_HURT_FIRE: vp[vn++] = "burn"; break;
				case FS_HURT_COLD: vp[vn++] = "flood"; break;
				case FS_HURT_ACID: vp[vn++] = "dissolve"; break;
				case FS_KILL_MOVE: vp[vn++] = "step on"; break;
				case FS_HURT_POIS: vp[vn++] = "poison"; break;
				case FS_HURT_ELEC: vp[vn++] = "electrify"; break;
				case FS_HURT_WATER: vp[vn++] = "flood"; break;
				case FS_HURT_BWATER: vp[vn++] = "boil"; break;
			}
		}
		else
		{
			switch(event->action)
			{
				case FS_SECRET: vp[vn++] = "found"; break;
				case FS_OPEN: vp[vn++] = "opened"; break;
				case FS_CLOSE: vp[vn++] = "closed"; break;
				case FS_BASH: vp[vn++] = "bashed"; break;
				case FS_SPIKE: vp[vn++] = "spiked"; break;
				case FS_DISARM: vp[vn++] = "disarmed"; break;
				case FS_TUNNEL: vp[vn++] = "dug through"; break;
				case FS_HIT_TRAP: vp[vn++] = "set off"; break;
				case FS_HURT_ROCK: vp[vn++] = "turned to mud"; break;
				case FS_HURT_FIRE: vp[vn++] = "burnt"; break;
				case FS_HURT_COLD: vp[vn++] = "flooded"; break;
				case FS_HURT_ACID: vp[vn++] = "dissolved"; break;
				case FS_KILL_MOVE: vp[vn++] = "stepped on"; break;
				case FS_HURT_POIS: vp[vn++] = "poisoned"; break;
				case FS_HURT_ELEC: vp[vn++] = "electrified"; break;
				case FS_HURT_WATER: vp[vn++] = "flooded"; break;
				case FS_HURT_BWATER: vp[vn++] = "boiled"; break;
			}
		}

		/* Introduce feature */
		if (vn)
		{
			output = TRUE;

			if (reflex_feature)
			{
				if ((!used_num) && (event->number > 0)) text_out(format("%d ",event->number));

				text_out(f_name + f_info[event->feat].name);
				if ((!used_num) && (event->number > 0)) text_out("s");

				text_out(event_pronoun_text_which[pronoun]);
				text_out(event_tense_text[tense]);

				used_num = TRUE;
			}
			else if (!intro)
			{
				if (prefix)
				{
					text_out(prefix);
					text_out(event_pronoun_text[pronoun]);
				}
				else
				{
					text_out(event_pronoun_text_caps[pronoun]);
				}
				text_out(event_tense_text[tense]);

				intro = TRUE;
			}

			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0) ;
				else if (n < vn-1) text_out(", ");
				else text_out(" or ");

				/* Dump */
				text_out(vp[n]);
			}

			if (!(reflex_feature))
			{
				if ((!used_num) && (event->number > 0)) text_out(format(" %d",event->number));

				text_out(" ");
				text_out(f_name + f_info[event->feat].name);
				if ((!used_num) && (event->number > 0)) text_out("s");

				used_num = TRUE;
			}

			if ((event->race) || (event->kind) || (event->ego_item_type) || (event->artifact))
			{
				if (reflex_feature) { text_out(".  "); intro = FALSE; reflex_monster = FALSE; reflex_object = FALSE; }
				else if (event->race) { text_out(" guarded by "); used_race = TRUE; }
				else text_out(" and ");
			}
		}
	}

	/* Kill a monster */
	if ((event->race) && ((used_race) || !((event->kind) || (event->ego_item_type) || (event->artifact))))
	{
		used_race = TRUE;

		/* Collect monster actions */
		vn = 0;
		if (tense > 1)
		{
			if (event->flags & (EVENT_FIND_RACE)) vp[vn++] = "find";
			if (event->flags & (EVENT_TALK_RACE)) vp[vn++] = "talk to";
			if (event->flags & (EVENT_ALLY_RACE)) vp[vn++] = "befriend";
			if (event->flags & (EVENT_DEFEND_RACE)) vp[vn++] = "defend";
			if (event->flags & (EVENT_HATE_RACE)) vp[vn++] = "offend";
			if (event->flags & (EVENT_FEAR_RACE)) vp[vn++] = "terrify";
			if (event->flags & (EVENT_HEAL_RACE)) vp[vn++] = "heal";
			if (event->flags & (EVENT_BANISH_RACE)) vp[vn++] = "banish";
			if (event->flags & (EVENT_KILL_RACE | EVENT_DEFEND_STORE)) vp[vn++] = "kill";
		}
		else
		{
			if (event->flags & (EVENT_FIND_RACE)) vp[vn++] = "found";
			if (event->flags & (EVENT_TALK_RACE)) vp[vn++] = "talked to";
			if (event->flags & (EVENT_ALLY_RACE)) vp[vn++] = "befriended";
			if (event->flags & (EVENT_DEFEND_RACE)) vp[vn++] = "defended";
			if (event->flags & (EVENT_HATE_RACE)) vp[vn++] = "offended";
			if (event->flags & (EVENT_FEAR_RACE)) vp[vn++] = "terrified";
			if (event->flags & (EVENT_HEAL_RACE)) vp[vn++] = "healed";
			if (event->flags & (EVENT_BANISH_RACE)) vp[vn++] = "banished";
			if (event->flags & (EVENT_KILL_RACE | EVENT_DEFEND_STORE)) vp[vn++] = "killed";
		}

		/* Hack -- monster race */
		if (vn)
		{
			char m_name[80];
			output = TRUE;

			if (reflex_monster)
			{
				/* Get the name */
				race_desc(m_name, sizeof(m_name), event->race, (used_num ? 0x400 : 0x08), event->number);
				
				text_out(m_name);

				text_out(event_pronoun_text_which[pronoun]);
				text_out(event_tense_text[tense]);

				used_num = TRUE;
			}
			else if (!intro)
			{
				if (prefix)
				{
					text_out(prefix);
					text_out(event_pronoun_text[pronoun]);
				}
				else
				{
					text_out(event_pronoun_text_caps[pronoun]);
				}

				if ((event->flags & (EVENT_GET_STORE)) && (tense > 1)) text_out("will ");
				else text_out(event_tense_text[tense]);

				intro = TRUE;
			}

			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0) ;
				else if (n < vn-1) text_out(", ");
				else text_out(" or ");

				/* Dump */
				text_out(vp[n]);
			}

			if (!(reflex_monster))
			{
				/* Get the name */
				race_desc(m_name, sizeof(m_name), event->race, (used_num ? 0x400 : 0x08), event->number);				

				text_out(" ");
				text_out(m_name);

				used_num = TRUE;
			}

			if ((event->kind) || (event->ego_item_type) || (event->artifact))
			{
				if (reflex_monster) { text_out(".  "); intro = FALSE; prefix = NULL; reflex_object = FALSE; }
				else if (event->number > 1) text_out(" and each carrying ");
				else text_out(" and carrying ");
			}
		}
	}

	/* Collect an object */
	if ((event->kind) || (event->ego_item_type) || (event->artifact))
	{
		object_type o_temp;
		char o_name[140];

		/* Create temporary object */
		if (event->artifact)
		{
			object_prep(&o_temp, lookup_kind(a_info[event->artifact].tval,a_info[event->artifact].sval));

			o_temp.name1 = event->artifact;
		}
		else if (event->ego_item_type)
		{
			if (event->kind)
			{
				object_prep(&o_temp, event->kind);
			}
			else
			{
				object_prep(&o_temp, lookup_kind(e_info[event->ego_item_type].tval[0], e_info[event->ego_item_type].min_sval[0]));
			}

			o_temp.name2 = event->ego_item_type;
		}
		else
		{
			object_prep(&o_temp, event->kind);
		}

		if (!(used_num) && !(event->artifact)) o_temp.number = event->number;
		else o_temp.number = 1;

		if (!(used_race)) o_temp.name3 = event->race;

		o_temp.ident |= (IDENT_KNOWN);

		/* Describe object */
		object_desc(o_name, sizeof(o_name), &o_temp, TRUE, 0);

		/* Collect store actions */
		vn = 0;
		if (tense > 1)
		{
			if (event->flags & (EVENT_FIND_ITEM)) vp[vn++] = "find";
			if (event->flags & (EVENT_BUY_STORE)) vp[vn++] = "buy";
			if (event->flags & (EVENT_SELL_STORE)) vp[vn++] = "sell";
			if (event->flags & (EVENT_GET_ITEM)) vp[vn++] = "get";
			if (event->race) vp[vn++] = "collect";
			if (event->flags & (EVENT_GIVE_STORE | EVENT_GIVE_RACE)) vp[vn++] = "give";
			if (event->flags & (EVENT_GET_STORE | EVENT_GET_RACE)) vp[vn++] = "be given";
			if (event->flags & (EVENT_DEFEND_STORE)) vp[vn++] = "use";
			if (event->flags & (EVENT_LOSE_ITEM)) vp[vn++] = "lose";
			if (event->flags & (EVENT_DESTROY_ITEM)) vp[vn++] = "destroy";
		}
		else
		{
			if (event->flags & (EVENT_FIND_ITEM)) vp[vn++] = "found";
			if (event->flags & (EVENT_BUY_STORE)) vp[vn++] = "bought";
			if (event->flags & (EVENT_SELL_STORE)) vp[vn++] = "sold";
			if (event->flags & (EVENT_GET_ITEM)) vp[vn++] = "kept";
			if (event->race) vp[vn++] = "collected";
			if (event->flags & (EVENT_GIVE_STORE | EVENT_GIVE_STORE)) vp[vn++] = "gave";
			if (event->flags & (EVENT_GET_STORE | EVENT_GET_RACE)) vp[vn++] = "were given";
			if (event->flags & (EVENT_DEFEND_STORE)) vp[vn++] = "used";
			if (event->flags & (EVENT_LOSE_ITEM)) vp[vn++] = "lost";
			if (event->flags & (EVENT_DESTROY_ITEM)) vp[vn++] = "destroyed";
		}

		if (vn)
		{
			output = TRUE;

			if (event->flags & (EVENT_STOCK_STORE))
			{
				if (intro) text_out("he ");
				else text_out("He ");

				if (tense > 1) text_out("will stock ");
				else if (tense < 0) text_out("stocked ");
				else text_out("stocks ");

				reflex_object = TRUE;
			}

			if (reflex_object)
			{
				text_out(o_name);

				used_num = TRUE;

				text_out(event_pronoun_text_which[pronoun]);
				if ((event->flags & (EVENT_GET_STORE | EVENT_GET_ITEM)) && (tense > 1)) text_out("will ");
				else text_out(event_tense_text[tense]);
			}
			else if (!intro)
			{
				if (prefix)
				{
					text_out(prefix);
					text_out(event_pronoun_text[pronoun]);
				}
				else
				{
					text_out(event_pronoun_text_caps[pronoun]);
				}
				if ((event->flags & (EVENT_DEFEND_STORE)) && (tense > 1)) text_out("can ");
				else if ((event->flags & (EVENT_GET_STORE)) && (tense > 1)) text_out("will ");
				else text_out(event_tense_text[tense]);
			}

			intro = TRUE;

			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0) ;
				else if (n < vn-1) text_out(", ");
				else text_out(" or ");

				/* Dump */
				text_out(vp[n]);
			}

			if (!(reflex_object))
			{
				text_out(" ");
				text_out(o_name);
			}
		}
	}

	/* Collect rewards */
	if ((event->gold) || (event->experience))
	{
		output = TRUE;

		if (!intro)
		{
			if (prefix)
			{
				text_out(prefix);
				text_out(event_pronoun_text[pronoun]);
			}
			else
			{
				text_out(event_pronoun_text_caps[pronoun]);
			}
		}
		else
		{
			text_out(" and ");
		}


		if ((event->gold > 0) || (event->experience > 0))
		{
			text_out("gain ");

			if (event->gold > 0)
			{
				text_out(format("%d gold",event->gold));
				if (event->experience) text_out(" and ");
			}

			if (event->experience > 0)
			{
				text_out(format("%d experience",event->experience));
				if (event->gold < 0) text_out(" and ");
			}
		}

		if ((event->gold < 0) || (event->experience < 0))
		{
			text_out("cost ");

			if (event->gold < 0)
			{
				text_out(format("%d gold",-event->gold));
				if (event->experience < 0) text_out(" and ");
			}

			if (event->experience < 0)
			{
				text_out(format("%d experience",-event->experience));
			}
		}
	}

	return (output);

}

/*
 * Output all quests between certain stages
 *
 * Example - print all quests min = QUEST_ASSIGN, max = QUEST_PENALTY
 *         - print known/live quests min = QUEST_ACTIVE, max = QUEST_REWARD
 *
 * Returns true if anything output.
 */
bool print_quests(int min_stage, int max_stage)
{
	int i, j;

	int tense = 0;
	bool newline = FALSE;
	bool output = FALSE;
	bool event_out;

	cptr prefix = NULL;

	for (i = 0; i < 5; i++)
	{
		newline = FALSE;

		/* Quest not in range */
		if ((q_list[i].stage >= min_stage) && (q_list[i].stage <= max_stage)) continue;

		event_out = FALSE;

		for (j = 0; j < MAX_QUEST_EVENTS; j++)
		{
			switch(j)
			{
				case QUEST_ASSIGN:
					if (q_info[i].stage == QUEST_ASSIGN) { tense = 6; }
					else if (q_info[i].stage == QUEST_ACTIVE) { tense = 1; }
					else { tense = 0; }
					break;
				case QUEST_ACTIVE:
					if (q_info[i].stage == QUEST_ACTIVE) { tense = 1; }
					else if (q_info[i].stage == QUEST_ACTION) { tense = 1; }
					else if (q_info[i].stage == QUEST_FAILED) { tense = 2; }
					else if (q_info[i].stage == QUEST_PENALTY) { tense = 2; }
					else continue;
					break;
				case QUEST_ACTION:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage == QUEST_ASSIGN) { tense = 3; }
					else if (q_info[i].stage == QUEST_ACTIVE) { tense = 1; }
					else if (q_info[i].stage == QUEST_FINISH) { tense = 2; }
					else { tense = 0; }
					break;
				case QUEST_REWARD:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage == QUEST_REWARD) { tense = 3; }
					else if (q_info[i].stage == QUEST_FINISH) { tense = 0; }
					else if (q_info[i].stage > QUEST_FINISH) continue;
					else { tense = 3; prefix = "To claim your reward, "; }
					break;
				case QUEST_PAYOUT:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage == QUEST_PAYOUT) { tense = 0; prefix = "and "; }
					else if (q_info[i].stage == QUEST_FINISH) { tense = 0; prefix = "and "; }
					else if (q_info[i].stage > QUEST_FINISH) continue;
					else { tense = 4; prefix = "and ";}
					break;
				case QUEST_FINISH:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage == QUEST_FINISH) { tense = 0; prefix = "and "; }
					else if (q_info[i].stage > QUEST_FINISH) continue;
					else { tense = 4; prefix = "and ";}
					break;
				case QUEST_FAILED:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage < QUEST_REWARD)
					{
						tense = 5;
						prefix = ".  You will fail the quest if ";
					}
					else if (q_info[i].stage == QUEST_PENALTY) { tense = 0; }
					else continue;
					break;
				case QUEST_FORFEIT:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage < QUEST_REWARD) { tense = 4; prefix = ". If this happens "; }
					else if (q_info[i].stage == QUEST_FORFEIT) { tense = 0; prefix = "causing you to fail the quest and "; }
					else continue;
					break;
				case QUEST_PENALTY:
					if (q_info[i].stage < QUEST_ACTION) continue;
					else if (q_info[i].stage < QUEST_REWARD) { tense = 4; prefix = ". If this happens "; }
					else if (q_info[i].stage == QUEST_PENALTY) { tense = 0; prefix = "causing you to fail the quest and "; }
					else continue;
					break;
			}

			if (event_out)
			{
				if ((prefix) && (prefix[0] != '.'))
				{
					text_out(" ");
				}
				else if (!prefix)
				{
					text_out(".  ");
				}
			}

			event_out = print_event(&q_info[i].event[j], 2, tense, prefix);
			output |= event_out;

			prefix = NULL;

			if (output) newline = TRUE;
		}

		if (newline) text_out(".\n\n");

		output = TRUE;
	}

	return (output);
}


/*
 * Displays all active quests
 */
void do_cmd_quest(void)
{
	bool no_quests = TRUE;

	/* Save screen */
	screen_save();

	/* Clear the screen */
	Term_clear();

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	/* Print title */
	text_out_c(TERM_L_BLUE, "Current Quests\n");

	/* Warning */
	text_out_c(TERM_YELLOW, "Quests will not be enabled until version 0.7.0. This command is currently for testing purposes only.\n");

	/* Either print live quests on level, or if nothing live, display current quests */
	if (print_quests(QUEST_ACTIVE , QUEST_REWARD)) no_quests = FALSE;
	else no_quests = !print_quests(QUEST_ASSIGN , QUEST_FINISH);

	/* No quests? */
	if (no_quests) text_out("You currently have no quests.\n");

	anykey();

	/* Load screen */
	screen_load();

}




/*
 * Encode the screen colors
 */
static const char hack[17] = "dwsorgbuDWvyRGBU";


/*
 * Hack -- load a screen dump from a file
 *
 * ToDo: Add support for loading/saving screen-dumps with graphics
 * and pseudo-graphics.  Allow the player to specify the filename
 * of the dump.
 */
void do_cmd_load_screen(void)
{
	int i, y, x;

	byte a = 0;
	char c = ' ';

	bool okay = TRUE;

	FILE *fp;

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, "dump.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Oops */
	if (!fp) return;


	/* Save screen */
	screen_save();


	/* Clear the screen */
	Term_clear();


	/* Load the screen */
	for (y = 0; okay && (y < 24); y++)
	{
		/* Get a line of data */
		if (my_fgets(fp, buf, sizeof(buf))) okay = FALSE;


		/* Show each row */
		for (x = 0; x < 79; x++)
		{
			/* Put the attr/char */
			Term_draw(x, y, TERM_WHITE, buf[x]);
		}
	}

	/* Get the blank line */
	if (my_fgets(fp, buf, sizeof(buf))) okay = FALSE;


	/* Dump the screen */
	for (y = 0; okay && (y < 24); y++)
	{
		/* Get a line of data */
		if (my_fgets(fp, buf, sizeof(buf))) okay = FALSE;

		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Look up the attr */
			for (i = 0; i < 16; i++)
			{
				/* Use attr matches */
				if (hack[i] == buf[x]) a = i;
			}

			/* Put the attr/char */
			Term_draw(x, y, a, c);
		}
	}


	/* Close it */
	my_fclose(fp);


	/* Message */
	msg_print("Screen dump loaded.");
	message_flush();


	/* Load screen */
	screen_load();
}


/*
 * Hack -- save a screen dump to a file
 */
void do_cmd_save_screen(void)
{
	int y, x;

	byte a = 0;
	char c = ' ';

	FILE *fff;

	char buf[1024];

	/* Click! */
	sound(MSG_SCREENDUMP);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, "dump.txt");

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff) return;


	/* Save screen */
	screen_save();


	/* Dump the screen */
	for (y = 0; y < 24; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = c;
		}

		/* Terminate */
		buf[x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Skip a line */
	fprintf(fff, "\n");


	/* Dump the screen */
	for (y = 0; y < 24; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = hack[a&0x0F];
		}

		/* Terminate */
		buf[x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Skip a line */
	fprintf(fff, "\n");


	/* Close it */
	my_fclose(fff);


	/* Message */
	msg_print("Screen dump saved.");
	message_flush();


	/* Load screen */
	screen_load();
}

/*
 * Hack -- save a screen dump to a file in html format
 */
void do_cmd_save_screen_html(void)
{
	int i;

	FILE *fff;

	char file_name[1024];


	/* Build the filename */
	path_build(file_name, 1024, ANGBAND_DIR_USER, "dump.prf");

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(file_name, "w");

	/* Oops */
	if (!fff) return;


	/* Extract default attr/char code for features */
	for (i = 0; i < z_info->f_max; i++)
	{
		feature_type *f_ptr = &f_info[i];

		if (!f_ptr->name) continue;

		/* Dump a comment */
		fprintf(fff, "# %s\n", (f_name + f_ptr->name));

		/* Dump the attr/char info */
		fprintf(fff, "F:%d:0x%02X:0x%02X:%s\n\n", i,
			(byte)(f_ptr->x_attr), (byte)(f_ptr->x_char), (f_ptr->flags3 & (FF3_ATTR_LITE) ? "YES" : "NO"));

		/* Assume we will use the underlying values */
		f_ptr->x_attr = f_ptr->d_attr;
		f_ptr->x_char = f_ptr->d_char;
	}

	/* Extract default attr/char code for objects */
	for (i = 0; i < z_info->k_max; i++)
	{
		object_kind *k_ptr = &k_info[i];

		if (!k_ptr->name) continue;

		/* Dump a comment */
		fprintf(fff, "# %s\n", (k_name + k_ptr->name));

		/* Dump the attr/char info */
		fprintf(fff, "K:%d:0x%02X:0x%02X\n\n", i,
			(byte)(k_ptr->x_attr), (byte)(k_ptr->x_char));

		/* Default attr/char */
		k_ptr->x_attr = k_ptr->d_attr;
		k_ptr->x_char = k_ptr->d_char;
	}

	/* Extract default attr/char code for flavors */
	for (i = 0; i < z_info->x_max; i++)
	{
		flavor_type *x_ptr = &x_info[i];

		if (!x_ptr->name) continue;

		/* Dump a comment */
		fprintf(fff, "# %s\n", (x_name + x_ptr->name));

		/* Dump the attr/char info */
		fprintf(fff, "L:%d:0x%02X:0x%02X\n\n", i,
			(byte)(x_ptr->x_attr), (byte)(x_ptr->x_char));

		/* Default attr/char */
		x_ptr->x_attr = x_ptr->d_attr;
		x_ptr->x_char = x_ptr->d_char;
	}


	/* Extract default attr/char code for monsters */
	for (i = 0; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];
		char m_name[80];

		if (!r_ptr->name) continue;

		/* Get the name */
		race_desc(m_name, sizeof(m_name), i, 0x400, 1);

		/* Dump a comment */
		fprintf(fff, "# %s\n", m_name);

		/* Dump the attr/char info */
		fprintf(fff, "R:%d:0x%02X:0x%02X\n\n", i,
			(byte)(r_ptr->x_attr), (byte)(r_ptr->x_char));

		/* Default attr/char */
		r_ptr->x_attr = r_ptr->d_attr;
		r_ptr->x_char = r_ptr->d_char;
	}

	/* Skip a line */
	fprintf(fff, "\n");


	/* Close it */
	my_fclose(fff);

	/* Refresh */
	do_cmd_redraw();

	/* Hack -- dump the graphics before loading font/graphics */
	dump_html();

	/* Graphic symbols */
	if (use_graphics)
	{
		/* Process "graf.prf" */
		process_pref_file("graf.prf");
	}

	/* Normal symbols */
	else
	{
		/* Process "font.prf" */
		process_pref_file("font.prf");
	}

	/* Process "dump.prf" */
	process_pref_file("dump.prf");


#ifdef ALLOW_BORG_GRAPHICS
#if 0
	/* Initialize the translation table for the borg */
	init_translate_visuals();
#endif
#endif /* ALLOW_BORG_GRAPHICS */

	/* Message */
	msg_print("Html screen dump saved.");
	message_flush();

	/* Refresh */
	do_cmd_redraw();

	/* Delete the file */
	fd_kill(file_name);
}


#define TOP_N	5

/*
 * Display some statistical summaries about the game
 */
void game_statistics(void)
{
	int i, j;
	int most_kill[TOP_N];
	int most_kill_count[TOP_N];
	int deepest_kill[TOP_N];
	int deepest_kill_level[TOP_N];
	int deepest_unique_kill = 0;
	int deepest_unique_kill_level = -1;
	int sauron_form_kills = 0;
	int total_kills = 0;
	int total_unique_kills = 0;
	int uniques_alive = 0;
	
	char race_name[80];
	
	/* Clear the top_n count */
	for (i = 0; i < TOP_N; i++)
	{
		most_kill[i] = 0;
		most_kill_count[i] = 0;
		deepest_kill[i] = 0;
		deepest_kill_level[i] = 0;
	}
	
	/* Effectiveness against monsters */
	for (i = 0; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];
		monster_lore *l_ptr = &l_list[i];
		
		/* Uniques are worth knowing about */
		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			/* Count kills */
			if (l_ptr->pkills)
			{
				/* Count forms of sauron killed */
				if ((i >= SAURON_FORM) && (i < SAURON_FORM + MAX_SAURON_FORMS))
				{
					/* Count uniques killed */
					sauron_form_kills++;
				}
				else
				{
					/* Count uniques killed */
					total_unique_kills++;
				}
				
				/* Count as total kills regardless */
				total_kills++;
				
				/* Is this the deepest unique killed? */
				if (r_ptr->level > deepest_unique_kill_level)
				{
					deepest_unique_kill_level = r_ptr->level;
					deepest_unique_kill = i;
				}
			}
			/* Exclude uniques and various forms of sauron */
			else if ((i != FAMILIAR_IDX ) && (i != FAMILIAR_BASE_IDX)
					&& ((i < SAURON_FORM) || (i >= SAURON_FORM + MAX_SAURON_FORMS)))
			{
				uniques_alive++;
			}
		}
		/* Track monsters separately */
		else
		{
			if (l_ptr->pkills)
			{
				/* Count uniques killed */
				total_kills += l_ptr->pkills;
				
				/* Is this in the top n monsters killed by number */
				for (j = 0; j <= TOP_N; j++)
				{
					int k;
					
					/* Climb up the ranking table until we find a better ranking */
					if ((j == TOP_N) || (l_ptr->pkills < most_kill_count[j]))
					{
						/* Shift everything below this down one */
						for (k = 1; k < j; k++)
						{
							most_kill_count[k-1] = most_kill_count[k];
							most_kill[k-1] = most_kill[k];
						}
						
						/* Put current monster in table one place below */
						if (j > 0)
						{
							most_kill_count[j-1] = l_ptr->pkills;
							most_kill[j-1] = i;
						}
						
						break;
					}
				}
				
				/* Is this in the top n monsters killed by depth */
				for (j = 0; j <= TOP_N; j++)
				{
					int k;
					
					/* Climb up the ranking table until we find a better ranking */
					if ((j == TOP_N) || (r_ptr->level < deepest_kill_level[j]))
					{
						/* Shift everything below this down one */
						for (k = 1; k < j; k++)
						{
							deepest_kill_level[k-1] = deepest_kill_level[k];
							deepest_kill[k-1] = deepest_kill[k];
						}
						
						/* Put current monster in table one place below */
						if (j > 0)
						{
							deepest_kill_level[j-1] = r_ptr->level;
							deepest_kill[j-1] = i;
						}
						
						break;
					}
				}

			}
		}
	}
	
	/* Display monster death statistics */
	text_out(format("Total monsters killed: %d\n", total_kills));
	
	/* Display unique death statistics */
	if (total_unique_kills)
	{
		text_out(format("Total uniques killed: %d\n", total_unique_kills));
		text_out(format("Deepest unique killed: %s (normally at %d)\n",
				r_name + r_info[deepest_unique_kill].name, deepest_unique_kill_level));
	}
	
	/* Don't spoil Sauron */
	if (sauron_form_kills) text_out(format("Total forms of Sauron killed: %d\n", total_unique_kills));
	
	/* Give player something to aim for */
	text_out(format("Total uniques left alive: %d\n", uniques_alive));

	/* Get the top 5 monsters killed. We note townsfolk here. */
	if (total_kills > total_unique_kills)
	{
		text_out(format("Top %d monsters killed (by depth, excluding uniques and townsfolk):\n", TOP_N));
		
		for (i = TOP_N - 1; i >= 0; i--)
		{
			if (!deepest_kill_level[i]) continue;
			
			/* XXX Should sort this list */
			
			race_desc(race_name, sizeof(race_name), deepest_kill[i], 0x408, l_list[deepest_kill[i]].pkills);
			text_out(format(" %s\n", race_name));
		}
		
		text_out(format("Top %d monsters killed (by number, excluding uniques):\n", TOP_N));
		
		for (i = TOP_N - 1; i >= 0; i--)
		{
			if (!most_kill_count[i]) continue;
			
			/* XXX Should sort this list */
			
			race_desc(race_name, sizeof(race_name), most_kill[i], 0x408, most_kill_count[i]);
			text_out(format(" %s\n", race_name));
		}

	}
}


/*
 * Display game statistics on screen
 */
void do_cmd_knowledge_game_statistics(void)
{
	/* Save screen */
	screen_save();

	/* Clear the screen */
	Term_clear();

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	/* Head the screen */
	text_out_c(TERM_L_BLUE, "Game statistics\n");

	/* Really do game statistics */
	game_statistics();

	(void)anykey();

	/* Load screen */
	screen_load();
}




/*
 * Move the cursor using the mouse in a browser window
 */
static void browser_mouse(key_event ke, int *column, int *grp_cur,
							int grp_cnt, int *list_cur, int list_cnt,
							int col0, int row0, int grp0, int list0,
							int *delay)
{
	int my = ke.mousey - row0;
	int mx = ke.mousex;
	int wid;
	int hgt;

	int grp = *grp_cur;
	int list = *list_cur;

	/* Get size */
	Term_get_size(&wid, &hgt);

	if (mx < col0)
	{
		int old_grp = grp;

		*column = 0;
		if ((my >= 0) && (my < grp_cnt - grp0) && (my < hgt - row0 - 2)) grp = my + grp0;
		else if (my < 0) { grp--; *delay = 100; }
		else if (my >= hgt - row0 - 2) { grp++; *delay = 50; }

		/* Verify */
		if (grp >= grp_cnt)	grp = grp_cnt - 1;
		if (grp < 0) grp = 0;
		if (grp != old_grp)	list = 0;

	}
	else
	{
		*column = 1;
		if ((my >= 0) && (my < list_cnt - list0) && (my < hgt - row0 - 2)) list = my + list0;
		else if (my < 0) { list--; *delay = 100; }
		else if (my >= hgt - row0 - 2) { list++; *delay = 50; }

		/* Verify */
		if (list >= list_cnt) list = list_cnt - 1;
		if (list < 0) list = 0;
	}

	(*grp_cur) = grp;
	(*list_cur) = list;
}

/*
 * Move the cursor in a browser window
 */
static void browser_cursor(char ch, int *column, int *grp_cur, int grp_cnt,
						   int *list_cur, int list_cnt)
{
	int d;
	int col = *column;
	int grp = *grp_cur;
	int list = *list_cur;

	/* Extract direction */
	d = target_dir(ch);

	if (!d) return;

	/* Diagonals - hack */
	if ((ddx[d] > 0) && ddy[d])
	{
		int browser_rows;
		int wid, hgt;

		/* Get size */
		Term_get_size(&wid, &hgt);

		browser_rows = hgt - 8;

		/* Browse group list */
		if (!col)
		{
			int old_grp = grp;

			/* Move up or down */
			grp += ddy[d] * browser_rows;

			/* Verify */
			if (grp >= grp_cnt)	grp = grp_cnt - 1;
			if (grp < 0) grp = 0;
			if (grp != old_grp)	list = 0;
		}

		/* Browse sub-list list */
		else
		{
			/* Move up or down */
			list += ddy[d] * browser_rows;

			/* Verify */
			if (list >= list_cnt) list = list_cnt - 1;
			if (list < 0) list = 0;
		}

		(*grp_cur) = grp;
		(*list_cur) = list;

		return;
	}

	if (ddx[d])
	{
		col += ddx[d];
		if (col < 0) col = 0;
		if (col > 1) col = 1;

		(*column) = col;

		return;
	}

	/* Browse group list */
	if (!col)
	{
		int old_grp = grp;

		/* Move up or down */
		grp += ddy[d];

		/* Verify */
		if (grp < 0) grp = 0;
		if (grp >= grp_cnt)	grp = grp_cnt - 1;
		if (grp != old_grp)	list = 0;
	}

	/* Browse sub-list list */
	else
	{
		/* Move up or down */
		list += ddy[d];

		/* Verify */
		if (list >= list_cnt) list = list_cnt - 1;
		if (list < 0) list = 0;
	}

	(*grp_cur) = grp;
	(*list_cur) = list;
}


/* ========================= MENU DEFINITIONS ========================== */

typedef void (*action_f)(int page, const char *name);

typedef struct {
	char sel;
	const char *name;
	action_f act;
	int page;
} command_menu;

static command_menu option_actions [] =
{
	{'1', "Interface options", do_cmd_options_aux, 0},
	{'2', "Display options", do_cmd_options_aux, 1},
	{'3', "Warning and disturbance options", do_cmd_options_aux, 2},
	{'4', "Birth (difficulty) options", do_cmd_options_aux, 3},
	{'5', "Cheat (debug) options", do_cmd_options_aux, 4},
	{0, 0, 0, 0},
	{'W', "Subwindow display settings", (action_f) do_cmd_options_win, 0},
	{'D', "Set base delay factor", (action_f) do_cmd_delay, 0},
	{'H', "Set hitpoint warning", (action_f) do_cmd_hp_warn, 0},
	{0, 0, 0, 0},
	{'L', "Load a user pref file", (action_f) do_cmd_pref_file_hack, 22},
	{'A', "Append options to a file", do_cmd_options_append, 0},
	{'R', "Reset all options to defaults", do_cmd_reset_options, 0},
	{0, 0, 0, 0},
	{'M', "Interact with macros (advanced)", (action_f) do_cmd_macros, 0},
	{'V', "Interact with visuals (advanced)", (action_f) do_cmd_visuals, 0},
	{'C', "Interact with colours (advanced)", (action_f) do_cmd_colors, 0},
};

/*
static command_menu option_actions [] = {
	{'1', "User Interface Options", do_cmd_options_aux, 0},
	{'2', "Disturbance Options", do_cmd_options_aux, 1},
	{'3', "Game-Play Options", do_cmd_options_aux, 2},
	{'4', "Efficiency Options", do_cmd_options_aux, 3},
	{'5', "Display Options", do_cmd_options_aux, 4},
	{'6', "Birth Options", do_cmd_options_aux, 5},
	{'7', "Cheat Options", do_cmd_options_aux, 6},
	{ 0, 0, 0, 0},
	{'W', "Window Flags", (action_f) do_cmd_options_win, 0},
	{'L', "Load a user pref file", (action_f) do_cmd_pref_file_hack, 20},
	{'A', "Append options to a file", do_cmd_options_append, 0},
	{ 0, 0, 0, 0},
	{'D', "Base Delay Factor", (action_f) do_cmd_delay, 0},
	{'H', "Hitpoint Warning", (action_f) do_cmd_hp_warn, 0}
};
*/

static command_menu knowledge_actions[] = {
	{'1', "Display artifact knowledge", (action_f)do_cmd_knowledge_artifacts, 0},
	{'2', "Display monster knowledge", (action_f)do_cmd_knowledge_monsters, 0},
	{'3', "Display ego item knowledge", (action_f)do_cmd_knowledge_ego_items, 0},
	{'4', "Display object knowledge", (action_f)do_cmd_knowledge_objects, 0},
	{'5', "Display feature knowledge", (action_f)do_cmd_knowledge_features, 0},
	{0, 0, 0, 0},
	{'6', "Display contents of your homes", (action_f)do_cmd_knowledge_home, 0},
	{'7', "Display dungeon knowledge", (action_f)do_cmd_knowledge_dungeons, 0},
	{'8', "Display help tips from the current game", (action_f)do_cmd_knowledge_help_tips, 0},
	{'9', "Display self-knowledge", (action_f)self_knowledge, 0},
	{'0', "Display game-play statistics", (action_f)do_cmd_knowledge_game_statistics, 0},
	{0, 0, 0, 0},
	{'L', "Load a user pref file", (action_f) do_cmd_pref_file_hack, 22},
	{'D', "Dump auto-inscriptions", (action_f) cmd_autos_dump, 0},
	{0, 0, 0, 0},
	{'M', "Interact with macros (advanced)", (action_f) do_cmd_macros, 0},
	{'V', "Interact with visuals (advanced)", (action_f) do_cmd_visuals, 0},
	{'C', "Interact with colours (advanced)", (action_f) do_cmd_colors, 0},
};

/*
 * Set or unset various options.
 *
 * After using this command, a complete redraw should be performed,
 * in case any visual options have been changed.
 * CLEANUP
 */
void do_cmd_menu(int menuID, const char *title)
{
	key_event ke;
	int line = 0;
	command_menu *actions;
	int n_actions;

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;


	switch (menuID) {
		case MENU_OPTIONS:
			actions = option_actions;
			n_actions = N_ELEMENTS(option_actions);
			break;
		case MENU_KNOWLEDGE:
			actions = knowledge_actions;
			n_actions = N_ELEMENTS(knowledge_actions);
			break;
		default:
			msg_print(format("INTERNAL ERROR: Unrecognized menu ID %d\n", menuID));
		return;
	}

	/* initialize static variables */
	if(!obj_group_order) {
		int i, n = 0;
		for(n = 0; object_group[n].tval; n++)
		obj_group_order = C_ZNEW(TV_GEMS+1, int);
		ang_atexit(cleanup_cmds);
		for(i = 0; i <= TV_GEMS; i++) /* allow for missing values */
			obj_group_order[i] = -1;
		for(i = 0; i < n; i++) {
			obj_group_order[object_group[i].tval] = i;
		}
	}

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save screen */
	screen_save();

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (1)
	{
		int i, act = -1;
		/* Why are we here */
		prt(format("Display current %s", title), 2, 0);

		/* Give some choices */
		for(i = 0; i < n_actions; i++) {
			if(actions[i].act) {
				c_prt(curs_attrs[CURS_KNOWN][line == i + 4],
					format("(%c) %s", actions[i].sel, actions[i].name), i+4, 5);
			}
		}

		prt("Command: ", 22, 0);

		/* Get command */
		ke = inkey_ex();

		/* React to mouse movement */
		if ((ke.key == '\xff') && !(ke.mousebutton))
		{
			line = ke.mousey;
			continue;
		}

		/* Exit */
		if (ke.key == ESCAPE) break;
		if (ke.key == '\xff') {
			act = ke.mousey - 4;
		}
		else {
			for(i = 0; i < n_actions; i++) {
				if(actions[i].sel == toupper(ke.key)) {
					act = i;
					break;
				}
			}
		}

		if(act >= 0 && act < n_actions)
		{
			/* Do the sub-command. */
			if(actions[act].act)
				actions[act].act(actions[act].page, actions[act].name);
		}
		else if(ke.key != '\xff')
		{
			/* Oops */
			bell(format("Illegal command for %s!", title));
		}

		/* Flush messages */
		message_flush();

		/* Clear screen */
		Term_clear();
	}


	/* Load screen */
	screen_load();
}

