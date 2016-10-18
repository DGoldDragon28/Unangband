/* File: wizard1.c */

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


#ifdef ALLOW_SPOILERS


/*
 * The spoiler file being created
 */
static FILE *fff = NULL;


/*
 * A tval grouper
 */
typedef struct
{
	byte tval;
	cptr name;
} grouper;



/*
 * Item Spoilers by Ben Harrison (benh@phial.com)
 */



/*
 * Write out `n' of the character `c' to the spoiler file
 */
static void spoiler_out_n_chars(int n, char c)
{
	while (--n >= 0) fputc(c, fff);
}

/*
 * Write out `n' blank lines to the spoiler file
 */
static void spoiler_blanklines(int n)
{
	spoiler_out_n_chars(n, '\n');
}

/*
 * Write a line to the spoiler file and then "underline" it with hypens
 */
static void spoiler_underline(cptr str, char c)
{
	text_out(str);
	text_out("\n");
	spoiler_out_n_chars(strlen(str), c);
	text_out("\n");
}




/*
 * The basic items categorized by type
 */
static const grouper group_item[] =
{
	{ TV_SHOT,		"Ammo" },
	{ TV_ARROW,		  NULL },
	{ TV_BOLT,		  NULL },

	{ TV_BOW,		"Bows" },

	{ TV_SWORD,		"Weapons" },
	{ TV_POLEARM,	  NULL },
	{ TV_HAFTED,	  NULL },
	{ TV_DIGGING,	  NULL },

	{ TV_SOFT_ARMOR,	"Armour (Body)" },
	{ TV_HARD_ARMOR,	  NULL },
	{ TV_DRAG_ARMOR,	  NULL },

	{ TV_CLOAK,		"Armour (Misc)" },
	{ TV_SHIELD,	  NULL },
	{ TV_HELM,		  NULL },
	{ TV_CROWN,		  NULL },
	{ TV_GLOVES,	  NULL },
	{ TV_BOOTS,		  NULL },

	{ TV_AMULET,	"Amulets" },
	{ TV_RING,		"Rings" },

	{ TV_SCROLL,	"Scrolls" },
	{ TV_POTION,	"Potions" },
	{ TV_FOOD,		"Food" },
	{ TV_MUSHROOM,		"Mushroom" },

	{ TV_ROD,		"Rods" },
	{ TV_WAND,		"Wands" },
	{ TV_STAFF,		"Staffs" },

	{ TV_MAGIC_BOOK,	"Books (Mage)" },
	{ TV_PRAYER_BOOK,	"Books (Priest)" },
	{ TV_SONG_BOOK,	"Books (Bard)" },
	{ TV_RUNESTONE,	"Runestones" },

	  { TV_BAG,       "Bags" },
	  { TV_SERVICE,       "Services" },
	  { TV_MAP,       "Maps" },
		{ TV_STATUE,    "Art" },

	{ TV_SPIKE,		"Tools" },
	{ TV_ROPE,		  NULL },
	{ TV_LITE,		  NULL },
	{ TV_FLASK,		  NULL },

	{ TV_JUNK,		  "Junk" },
	{ TV_HOLD,      NULL },
	{ TV_BONE,    NULL },
	{ TV_SKIN,    NULL },
	{ TV_BODY,      NULL },
	{ TV_ASSEMBLY,      NULL },

	{ 0, "" }
};





/*
 * Describe the kind
 */
static void kind_info(char *buf, int buf_s, char *dam, int dam_s, char *wgt, int wgt_s, char *pow, int pow_s, int *lev, s32b *val, int k)
{
	object_kind *k_ptr;

	object_type *i_ptr;
	object_type object_type_body;

        s16b book[26];
	int num,i;

	(void)wgt_s;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Prepare a fake item */
	object_prep(i_ptr, k);

	/* Obtain the "kind" info */
	k_ptr = &k_info[k];

	/* It is known */
	i_ptr->ident |= (IDENT_KNOWN);

	/* Cancel bonuses */
	i_ptr->pval = 0;
	i_ptr->to_a = 0;
	i_ptr->to_h = 0;
	i_ptr->to_d = 0;


	/* Level */
	(*lev) = k_ptr->locale[0];

	/* Value */
	(*val) = object_value(i_ptr);


	/* Hack */
	if (!buf || !dam || !wgt) return;


	/* Description (too brief) */
	object_desc_spoil(buf, buf_s, i_ptr, FALSE, 0);

	/* Misc info */
	my_strcpy(dam, "", dam_s);

	/* Damage */
	switch (i_ptr->tval)
	{
		/* Bows */
		case TV_BOW:
		{
			break;
		}

		/* Ammo */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		{
			sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
			break;
		}

		/* Weapons */
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_STAFF:
		{
			sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
			break;
		}

		/* Armour */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			sprintf(dam, "%d", i_ptr->ac);
			break;
		}
	}


	/* Weight */
	sprintf(wgt, "%3d.%d", i_ptr->weight / 10, i_ptr->weight % 10);

        /* Power */
        my_strcpy(pow, "", pow_s);

	/* Fill the book with spells */
	fill_book(i_ptr,book,&num);

	if (num > 1)
	{
		int count = 0;

		for (i = 0; i < num; i++)
		{
			if (book[i]) count++;
		}

		if ((count) && ((i_ptr->tval == TV_MAGIC_BOOK) || (i_ptr->tval == TV_PRAYER_BOOK)
			|| (i_ptr->tval == TV_SONG_BOOK)))
		{
                        sprintf(pow," %d spells",count);
		}
		else if (count)
		{
                        sprintf(pow," %d powers",count);
		}
	}
	/* Power */
	else if (book[0])
	{
		spell_info(pow,pow_s, book[0],k_info[k].level);
	}

}

/*
 * Create a spoiler file for items
 */
static void spoil_obj_desc(cptr fname)
{
	int i, k, s, t, n = 0;

	u16b who[200];

	char buf[1024];

	char wgt[80];
	char dam[80];
	char pow[80];

        cptr formatter = "%-37s  %7s%6s%4s%9s %-12s\n";

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Header */
	fprintf(fff, "Spoiler File -- Basic Items (%s)\n\n\n", VERSION_STRING);

	/* More Header */
	fprintf(fff, formatter, "Description", "Dam/AC", "Wgt","Lev", "Cost", "Info");
        fprintf(fff, formatter, "-------------------------------------",
		"------", "---","---", "----", "---------");

	/* List the groups */
	for (i = 0; TRUE; i++)
	{
		/* Write out the group title */
		if (group_item[i].name)
		{
			/* Hack -- bubble-sort by cost and then level */
			for (s = 0; s < n - 1; s++)
			{
				for (t = 0; t < n - 1; t++)
				{
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					kind_info(NULL, 0, NULL, 0, NULL, 0, NULL, 0, &e1, &t1, who[i1]);
					kind_info(NULL, 0, NULL, 0, NULL, 0, NULL, 0, &e2, &t2, who[i2]);

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
					{
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++)
			{
				int e;
				s32b v;

				/* Describe the kind */
				kind_info(buf, sizeof(buf), dam, sizeof(dam), wgt, sizeof(wgt), pow, sizeof(pow), &e, &v, who[s]);

				/* Dump it */
                                fprintf(fff, "  %-41s%3s%6s%4d%9ld%-12s\n",
					buf, dam, wgt, e, (long)(v), pow);

			}

			/* Start a new set */
			n = 0;

			/* Notice the end */
			if (!group_item[i].tval) break;

			/* Start a new set */
			fprintf(fff, "\n\n%s\n\n", group_item[i].name);
		}

		/* Get legal item types */
		for (k = 1; k < z_info->k_max; k++)
		{
			object_kind *k_ptr = &k_info[k];

			/* Skip wrong tval's */
			if (k_ptr->tval != group_item[i].tval) continue;

			/* Hack -- Skip instant-artifacts */
			if (k_ptr->flags5 & (TR5_INSTA_ART)) continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}



/*
 * Create a spoiler file for items
 */
static void spoil_object(cptr fname)
{
	int i, k, s, t, n = 0;

	u16b who[200];

	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Dump to the spoiler file */
	text_out_hook = text_out_to_file;
	text_out_file = fff;

	/* Dump the header */
	spoiler_underline(format("Object Spoilers for %s %s",
                         VERSION_NAME, VERSION_STRING), '=');


	/* List the groups */
	for (i = 0; TRUE; i++)
	{
		/* Write out the group title */
		if (group_item[i].name)
		{

			/* Hack -- bubble-sort by cost and then level */
			for (s = 0; s < n - 1; s++)
			{
				for (t = 0; t < n - 1; t++)
				{
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					kind_info(NULL, 0, NULL, 0, NULL, 0, NULL, 0, &e1, &t1, who[i1]);
					kind_info(NULL, 0, NULL, 0, NULL, 0, NULL, 0, &e2, &t2, who[i2]);

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
					{
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++)
			{
				object_type *o_ptr;
				object_type object_type_body;

				/* Get local object */
				o_ptr = &object_type_body;

				/* Wipe the object */
				object_wipe(o_ptr);

				/* Create the artifact */
				object_prep(o_ptr, who[s]);

				/* Hack -- its in the store */
				o_ptr->ident |= (IDENT_STORE);

				/* It's fully know */
				if (!k_info[who[s]].flavor) object_known(o_ptr);

				/* Grab artifact name */
				object_desc_spoil(buf, sizeof(buf), o_ptr, TRUE, 0);

				/* Print name and underline */
				spoiler_underline(buf, '-');

				/* Write out the artifact description to the spoiler file */
				list_object(o_ptr,1);

				/* Terminate the entry */
				spoiler_blanklines(2);

			}

			/* Start a new set */
			n = 0;

			/* Notice the end */
			if (!group_item[i].tval) break;

			spoiler_blanklines(2);
			spoiler_underline(group_item[i].name, '-');
			spoiler_blanklines(1);

		}

		/* Get legal item types */
		for (k = 1; k < z_info->k_max; k++)
		{
			object_kind *k_ptr = &k_info[k];

			/* Skip wrong tval's */
			if (k_ptr->tval != group_item[i].tval) continue;

			/* Hack -- Skip instant-artifacts */
			if (k_ptr->flags5 & (TR5_INSTA_ART)) continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}





/*
 * Artifact Spoilers by: randy@PICARD.tamu.edu (Randy Hutson)
 */


/*
 * Returns a "+" string if a number is non-negative and an empty
 * string if negative
 */
#define POSITIZE(v) (((v) >= 0) ? "+" : "")

/*
 * These are used to format the artifact spoiler file. INDENT1 is used
 * to indent all but the first line of an artifact spoiler. INDENT2 is
 * used when a line "wraps". (Bladeturner's resistances cause this.)
 */
#define INDENT1 "    "
#define INDENT2 "      "

/*
 * MAX_LINE_LEN specifies when a line should wrap.
 */
#define MAX_LINE_LEN 75


/*
 * The artifacts categorized by type
 */
static const grouper group_artifact[] =
{
	{ TV_SWORD,		"Edged Weapons" },
	{ TV_POLEARM,	"Polearms" },
	{ TV_HAFTED,	"Hafted Weapons" },
	{ TV_BOW,		"Bows" },
	{ TV_DIGGING,	"Diggers" },

	{ TV_SOFT_ARMOR,	"Body Armor" },
	{ TV_HARD_ARMOR,	  NULL },
	{ TV_DRAG_ARMOR,	  NULL },

	{ TV_CLOAK,		"Cloaks" },
	{ TV_SHIELD,	"Shields" },
	{ TV_HELM,		"Helms/Crowns" },
	{ TV_CROWN,		  NULL },
	{ TV_GLOVES,	"Gloves" },
	{ TV_BOOTS,		"Boots" },

	{ TV_LITE,		"Light Sources" },
	{ TV_AMULET,	"Amulets" },
	{ TV_RING,		"Rings" },

	{ 0, NULL }
};





/*
 * Create a spoiler file for artifacts
 */
static void spoil_artifact(cptr fname)
{
	int i, j;

	object_type *i_ptr;
	object_type object_type_body;

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Dump to the spoiler file */
	text_out_hook = text_out_to_file;
	text_out_file = fff;

	/* Dump the header */
	spoiler_underline(format("Artifact Spoilers for %s %s",
                         VERSION_NAME, VERSION_STRING), '=');

	/* List the artifacts by tval */
	for (i = 0; group_artifact[i].tval; i++)
	{
		/* Write out the group title */
		if (group_artifact[i].name)
		{
			spoiler_blanklines(2);
			spoiler_underline(group_artifact[i].name, '-');
			spoiler_blanklines(1);
		}

		/* Now search through all of the artifacts */
		for (j = 1; j < z_info->a_max; ++j)
		{
			artifact_type *a_ptr = &a_info[j];

			/* We only want objects in the current group */
			if (a_ptr->tval != group_artifact[i].tval) continue;

			/* Get local object */
			i_ptr = &object_type_body;

			/* Wipe the object */
			object_wipe(i_ptr);

			/* Attempt to "forge" the artifact */
			if (!make_fake_artifact(i_ptr, (byte)j)) continue;

			/* Grab artifact name */
			object_desc_spoil(buf, sizeof(buf), i_ptr, TRUE, 2);

			/* Print name and underline */
			spoiler_underline(buf, '-');

			/* Write out the artifact description to the spoiler file */
			list_object(i_ptr,1);

			/*
			 * Determine the minimum depth an artifact can appear, its rarity,
			 * its weight, and its value in gold pieces.
			 */
			text_out(format("\nLevel %u, Rarity %u, %d.%d lbs, %ld AU\n",
		                a_ptr->level, a_ptr->rarity, (a_ptr->weight / 10),
                		(a_ptr->weight % 10), ((long)a_ptr->cost)));

			/* Terminate the entry */
			spoiler_blanklines(2);
		}
	}

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}


/*
 * The artifacts categorized by type
 */
static const cptr mon_spell_desc[] =
{
	"BLOW_1",
	"BLOW_2",
	"BLOW_3",
	"BLOW_4",
	"AMMO",
	"QUAKE",
	"EXPLOD",
	"AURA",
	"BRACID",
	"BRELEC",
	"BRFIRE",
	"BRCOLD",
	"BRPOIS",
	"BRPLAS",
	"BRLITE",
	"BRDARK",
	"BRCONF",
	"BRSOUN",
	"BRSHAR",
	"BRINER",
	"BRGRAV",
	"BRWIND",
	"BRFORC",
	"BRNEXU",
	"BRNETH",
	"BRCHAO",
	"BRDISE",
	"BRTIME",
	"BRMANA",
	"BRHOLY",
	"BRFEAR",
	"BRDISE",
	"BAACID",
	"BAELEC",
	"BAFIRE",
	"BACOLD",
	"BAPOIS",
	"BALITE",
	"BADARK",
	"BACONF",
	"BASOUN",
	"BASHAR",
	"BAWIND",
	"BASTOR",
	"BANETH",
	"BACHAO",
	"BAMANA",
	"BAWATE",
	"BOACID",
	"BOELEC",
	"BOFIRE",
	"BOCOLD",
	"BOPOIS",
	"BOPLAS",
	"BOICE",
	"BOWATE",
	"BONETH",
	"BOMANA",
	"HLYORB",
	"BEELEC",
	"BEICE",
	"BENETH",
	"ARCHFI",
	"ARCFOR",
	"HASTE",
	"ADMANA",
	"HEAL",
	"CURE",
	"BLINK",
	"TPORT",
	"INVIS",
	"TESELF",
	"TELETO",
	"TELEAW",
	"TELLVL",
	"WRAITH",
	"DARKNE",
	"TRAPS",
	"FORGET",
	"DRAINM",
	"CURSE",
	"DISPEL",
	"MINDBL",
	"ILLUSI",
	"WOUND",
	"BLESS",
	"BESERK",
	"SHIELD",
	"OPPOSE",
	"HUNGER",
	"PROBE",
	"SCARE",
	"BLIND",
	"CONF",
	"SLOW",
	"HOLD",
	"S_KIN",
	"R_KIN",
	"A_DEAD",
	"S_MON",
	"S_MONS",
	"R_MON",
	"R_MONS",
	"S_PLAN",
	"S_INSE",
	"S_ANIM",
	"S_HOUN",
	"S_SPID",
	"S_CLAS",
	"S_RACE",
	"S_GROU",
	"S_FRD",
	"S_FRDS",
	"S_ORC",
	"S_TROL",
	"S_GIAN",
	"S_DRAG",
	"S_HIDR",
	"A_ELEM",
	"A_OBJE",
	"S_DEMO",
	"S_HIDE",
	"R_UNIQ",
	"S_UNIQ",
	"S_HUNQ",
	"S_UNDE",
	"S_HIUN",
	"S_WRAI",
	"XXXX",
	"XXXX",
	"XXXX",
	"XXXX"
};


/*
 * Create a spoiler file for monsters
 */
static void spoil_mon_desc(cptr fname)
{
	int i, n = 0;

	char buf[1024];

	char nam[80];
	char lev[80];
	char rar[80];
	char pow[80];
	char exp[80];
	char bsp[80];
	char thr[80];
	char bth[80];
	char spd[80];
	char ac[80];
	char hp[80];
	char vis[80];

	u16b *who;
	u16b why = 2;
	
	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Dump the header */
	fprintf(fff, "Monster Spoilers for %s Version %s\n",
		VERSION_NAME, VERSION_STRING);
	fprintf(fff, "------------------------------------------\n\n");

	/* Dump the header */
	fprintf(fff, "%-40.40s%4s%4s%8s%8s%7s%8s%7s%6s%8s%4s  %11.11s\n",
		"Name", "Lev", "Rar", "Power", "Exper", "Spell", "Threat", "From", "Spd", "Hp", "Ac", "Visual Info");
	fprintf(fff, "%-40.40s%4s%4s%8s%8s%7s%8s%7s%6s%8s%4s  %11.11s\n",
		"----", "---", "---", "-----", "-----", "-----", "------", "----", "---", "--", "--", "-----------");

	/* Allocate the "who" array */
	who = C_ZNEW(z_info->r_max, u16b);

	/* Scan the monsters (except the ghost) */
	for (i = 1; i < z_info->r_max - 1; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name) who[n++] = (u16b)i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);

	/* Scan again */
	for (i = 0; i < n; i++)
	{
		monster_race *r_ptr = &r_info[who[i]];
		char m_name[80];
		
		/* Get the name */
		race_desc(m_name, sizeof(m_name), who[i], 0xC00, 1);

		/* Get the "name" */
		if (r_ptr->flags1 & (RF1_QUESTOR))
		{
			sprintf(nam, "[Q] %s", m_name);
		}
		else if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			sprintf(nam, "[U] %s", m_name);
		}
		else
		{
			sprintf(nam, "The %s", m_name);
		}


		/* Level */
		sprintf(lev, "%d", r_ptr->level);

		/* Rarity */
		sprintf(rar, "%d", r_ptr->rarity);

		/* Rarity */
		sprintf(pow, "%d", r_ptr->power);

		/* Experience */
		sprintf(exp, "%ld", (long)(r_ptr->mexp));

		/* Best spell */
		if (r_ptr->best_spell) sprintf(bsp, "%s", mon_spell_desc[r_ptr->best_spell-96]);
		else my_strcpy(bsp, "", sizeof(bsp));

		/* Highest threat */
		sprintf(thr, "%d", r_ptr->highest_threat);

		/* Best spell */
		if (r_ptr->best_threat) sprintf(bth, "%s", mon_spell_desc[r_ptr->best_threat-96]);
		else my_strcpy(bth, "", sizeof(bth));

		/* Speed */
		if (r_ptr->speed >= 110)
		{
			sprintf(spd, "+%d", (r_ptr->speed - 110));
		}
		else
		{
			sprintf(spd, "-%d", (110 - r_ptr->speed));
		}

		/* Armor Class */
		sprintf(ac, "%d", r_ptr->ac);

		/* Hitpoints */
		sprintf(hp, "%d", r_ptr->hp);

		/* Hack -- use visual instead */
		sprintf(vis, "%s '%c'", attr_to_text(r_ptr->d_attr), r_ptr->d_char);

		/* Dump the info */
		fprintf(fff, "%-40.40s%4s%4s%8s%8s%7s%8s%7s%6s%8s%4s  %11.11s\n",
			nam, lev, rar, pow, exp, bsp, thr, bth, spd, hp, ac, vis);
	}

	/* End it */
	fprintf(fff, "\n");

	/* Free the "who" array */
	FREE(who);


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Worked */
	msg_print("Successfully created a spoiler file.");
}




/*
 * Monster spoilers originally by: smchorse@ringer.cs.utsa.edu (Shawn McHorse)
 */

/*
 * Create a spoiler file for monsters (-SHAWN-)
 */
static void spoil_mon_info(cptr fname)
{
	char buf[1024];
	int i, n;
	u16b why = 2;
	u16b *who;
	int count = 0;


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Dump to the spoiler file */
	text_out_hook = text_out_to_file;
	text_out_file = fff;

	/* Dump the header */
	sprintf(buf, "Monster Spoilers for %s Version %s\n",
		VERSION_NAME, VERSION_STRING);
	text_out(buf);
	text_out("------------------------------------------\n\n");

	/* Allocate the "who" array */
	who = C_ZNEW(z_info->r_max, u16b);

	/* Scan the monsters */
	for (i = 1; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name) who[count++] = (u16b)i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, count);

	/*
	 * List all monsters in order (except the ghost).
	 */
	for (n = 0; n < count; n++)
	{
		int r_idx = who[n];
		monster_race *r_ptr = &r_info[r_idx];

		/* Prefix */
		if (r_ptr->flags1 & RF1_QUESTOR)
		{
			text_out("[Q] ");
		}
		else if (r_ptr->flags1 & RF1_UNIQUE)
		{
			text_out("[U] ");
		}
		else
		{
			text_out("The ");
		}

		/* Name */
		sprintf(buf, "%s  (", (r_name + r_ptr->name));	/* ---)--- */
		text_out(buf);

		/* Color */
		text_out(attr_to_text(r_ptr->d_attr));

		/* Symbol --(-- */
		sprintf(buf, " '%c')\n", r_ptr->d_char);
		text_out(buf);


		/* Indent */
		sprintf(buf, "=== ");
		text_out(buf);

		/* Number */
		sprintf(buf, "Num:%d  ", r_idx);
		text_out(buf);

		/* Level */
		sprintf(buf, "Lev:%d  ", r_ptr->level);
		text_out(buf);

		/* Rarity */
		sprintf(buf, "Rar:%d  ", r_ptr->rarity);
		text_out(buf);

		/* Speed */
		if (r_ptr->speed >= 110)
		{
			sprintf(buf, "Spd:+%d  ", (r_ptr->speed - 110));
		}
		else
		{
			sprintf(buf, "Spd:-%d  ", (110 - r_ptr->speed));
		}
		text_out(buf);

		/* Hitpoints */
		sprintf(buf, "Hp:%d  ", r_ptr->hp);

		text_out(buf);

		/* Armor Class */
		sprintf(buf, "Ac:%d  ", r_ptr->ac);
		text_out(buf);

		/* Experience */
		sprintf(buf, "Exp:%ld\n", (long)(r_ptr->mexp));
		text_out(buf);

		/* Describe */
		describe_monster_race(&r_info[r_idx], &l_list[r_idx], TRUE);

		/* Terminate the entry */
		text_out("\n");
	}

	/* Free the "who" array */
	FREE(who);

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	msg_print("Successfully created a spoiler file.");
}


/*
 * Create a spoiler file for terrain
 */
static void spoil_feat_desc(cptr fname)
{
	int i, k, s, t, n = 0;

	u16b who[600];

	char buf[1024];

        cptr formatter = "%-37s  %7s%4s%4s\n";

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Header */
	fprintf(fff, "Spoiler File -- Terrain (%s)\n\n\n", VERSION_STRING);

	/* More Header */
	fprintf(fff, formatter, "Description", "Dam", "Lev", "Rar");
        fprintf(fff, formatter, "-------------------------------------",
		"---", "---","---");

	/* List the groups */
	for (i = 0; i < 23; i++)
	{
		/* Write out the group title */
		if (TRUE)
		{
			/* Hack -- bubble-sort by level then by rarity*/
			for (s = 0; s < n - 1; s++)
			{
				for (t = 0; t < n - 1; t++)
				{
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					t1 = f_info[who[i1]].level;
					t2 = f_info[who[i2]].level;

					e1 = f_info[who[i1]].rarity;
					e2 = f_info[who[i2]].rarity;

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
					{
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++)
			{
				feature_type *f_ptr = &f_info[who[s]];

				/* Dump it */
                                fprintf(fff, "%-37s  %7s%4d%4d\n",
					f_name + f_ptr->name, f_ptr->blow.method ? format("%dd%d", f_ptr->blow.d_dice, f_ptr->blow.d_side) : "",
					f_ptr->level, f_ptr->rarity);

			}

			/* Start a new set */
			n = 0;

			/* Start a new set */
			if (feature_group_text[i]) fprintf(fff, "\n\n%s\n\n", feature_group_text[i]);
		}

		/* Get legal terrain */
		for (k = 0; k < z_info->f_max; k++)
		{
			/* Skip wrong tval's */
			if (feat_order(k) != i) continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}



/*
 * Create a spoiler file for terrain
 */
static void spoil_feat_info(cptr fname)
{
	int i, k, s, t, n = 0;

	u16b who[600];

	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Dump to the spoiler file */
	text_out_hook = text_out_to_file;
	text_out_file = fff;

	/* Dump the header */
	spoiler_underline(format("Terrain Spoilers for %s %s",
                         VERSION_NAME, VERSION_STRING), '=');

	/* List the groups */
	for (i = 0; i < 22; i++)
	{
		/* Write out the group title */
		if (TRUE)
		{
			/* Hack -- bubble-sort by cost and then level */
			for (s = 0; s < n - 1; s++)
			{
				for (t = 0; t < n - 1; t++)
				{
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					t1 = f_info[who[i1]].level;
					t2 = f_info[who[i2]].level;

					e1 = f_info[who[i1]].rarity;
					e2 = f_info[who[i2]].rarity;

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
					{
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++)
			{
				/* Grab feature name */
				sprintf(buf, "%s", f_name + f_info[who[s]].name);

				/* Print name and underline */
				spoiler_underline(buf, '-');

				/* Write out the feature description to the spoiler file */
				describe_feature(who[s]);

				/* Terminate the entry */
				spoiler_blanklines(2);

			}

			/* Start a new set */
			n = 0;

			spoiler_blanklines(2);
			spoiler_underline(feature_group_text[i], '-');
			spoiler_blanklines(1);

		}

		/* Get legal terrain */
		for (k = 0; k < z_info->f_max; k++)
		{
			/* Skip wrong tval's */
			if (feat_order(k) != i) continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}



/*
 * Create a spoiler file for items
 */
static void spoil_ego_desc(cptr fname)
{
	int i, j, k, s, t, n = 0;

	u16b who[200];

	char buf[1024];

        cptr formatter = "%-37s  %7s%4s%4s\n";

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Header */
	fprintf(fff, "Spoiler File -- Ego Items (%s)\n\n", VERSION_STRING);
	fprintf(fff, "Note: 1000 power equals no benefit\n\n");

	/* More Header */
	fprintf(fff, formatter, "Description", "Power", "Lev", "Rar");
        fprintf(fff, formatter, "-------------------------------------",
		"-----", "---", "---");

	/* List the groups */
	for (i = 0; i < 15; i++)
	{
		/* Write out the group title */
		if (group_artifact[i].name)
		{
			/* Hack -- bubble-sort by cost and then level */
			for (s = 0; s < n - 1; s++)
			{
				for (t = 0; t < n - 1; t++)
				{
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					t1 = e_info[who[i1]].level;
					t2 = e_info[who[i2]].level;

					e1 = e_info[who[i1]].rarity;
					e2 = e_info[who[i2]].rarity;

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
					{
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++)
			{
				ego_item_type *e_ptr = &e_info[who[s]];

				/* Dump it */
                                fprintf(fff, "%-37s  %7ld%4d%4d\n",
					e_name + e_ptr->name, e_ptr->slay_power * 1000 / tot_mon_power,
					e_ptr->level, e_ptr->rarity);

			}

			/* Start a new set */
			n = 0;

			/* Notice the end */
			if (!group_artifact[i].tval) break;

			/* Start a new set */
			fprintf(fff, "\n\n%s\n\n", group_artifact[i].name);
		}

		/* Get legal item types */
		for (k = 1; k < z_info->e_max; k++)
		{
			ego_item_type *e_ptr = &e_info[k];

			for (j = 0; j < 3; j++)
			{
				/* Skip wrong tval's */
				if (e_ptr->tval[j] != group_artifact[i].tval) continue;

				/* Save the index */
				who[n++] = k;
			}
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}


/*
 * Names for slay table
 *
 * Could probably be smart and extract this from magic item table.
 *
 */
static const cptr slay_name[]=
{
	"Slay Nature",
	"Brand Holy",
	"Slay Undead",
	"Slay Demon",
	"Slay Orc",
	"Slay Troll",
	"Slay Giant",
	"Slay Dragon",
	"Execute Dragon",
	"Execute Demon",
	"Execute Undead",
	"Brand Poison",
	"Brand Acid",
	"Brand Elec",
	"Brand Fire",
	"Brand Cold",
	"Brand Lite",
	"Brand Dark",
	"Slay Man",
	"Slay Elf",
	"Slay Dwarf",
	NULL
};


/*
 * Create a spoiler file for relative slay power
 */
static void spoil_slay_table(cptr fname)
{
	int i;

	char buf[1024];

        cptr formatter = "%-37s  %7s\n";

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Header */
	fprintf(fff, "Spoiler File -- Relative Slay Power (%s)\n\n", VERSION_STRING);
	fprintf(fff, "Note: 1000 power equals no benefit\n\n");

	/* More Header */
	fprintf(fff, formatter, "Slay", "Power");
        fprintf(fff, formatter, "-------------------------------------",
		"-----");

	/* List the groups */
	for (i = 0; TRUE; i++)
	{
		if (!(slay_name[i])) break;
	        fprintf(fff, "%-37s  %7ld\n", slay_name[i],magic_slay_power[i] * 1000 / tot_mon_power);
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}



/*
 * Hack - fake class
 */
static void spoil_fake_class(char *buf, size_t buf_s, int c, int t, int s)
{
	/* Hack - get specialist if requested */
	if (!c)
	{
		switch (t)
		{
			case TV_MAGIC_BOOK: p_ptr->pclass = 1; p_ptr->pstyle = WS_MAGIC_BOOK; break;
			case TV_PRAYER_BOOK: p_ptr->pclass = 2; p_ptr->pstyle = WS_MAGIC_BOOK; break;
			case TV_SONG_BOOK: p_ptr->pclass = 9; p_ptr->pstyle = WS_MAGIC_BOOK; break;
		}

		p_ptr->psval = s;
	}
	/* Hack - get a real class */
	else
	{
		p_ptr->pclass = c;
		p_ptr->pstyle = 0;
	}

	/* Get other details */
	cp_ptr = &c_info[p_ptr->pclass];

	/* Get the class name */
	lookup_prettyname(buf, buf_s, p_ptr->pclass, p_ptr->pstyle, p_ptr->psval, FALSE, TRUE);
}



/*
 * Create a spoiler file for items
 */
static void spoil_magic_info(cptr fname)
{
	int old_pclass = p_ptr->pclass;
	int old_pstyle = p_ptr->pstyle;
	int old_psval = p_ptr->psval;
	const player_class *old_cp_ptr = cp_ptr;

	int t, s, i, j, k, l;

	s16b book[26];

	char name[32];
	char desc[80];
	s16b casters[32];

	int casters_n;

	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Dump to the spoiler file */
	text_out_hook = text_out_to_file;
	text_out_file = fff;

	/* Header */
	fprintf(fff, "Spoiler File -- Spell Casting (%s)\n\n\n", VERSION_STRING);

	/* Check relevent tvals */
	for (t = TV_MAGIC_BOOK; t <= TV_SONG_BOOK; t++)
	{
		switch (t)
		{
			case TV_MAGIC_BOOK:	spoiler_underline("Magic Books", '='); break;
			case TV_PRAYER_BOOK:	spoiler_underline("Prayer Books", '='); break;
			case TV_SONG_BOOK:	spoiler_underline("Song Books", '='); break;
		}

		fprintf(fff, "\n");

		/* Cycle through all the books */
		for (s = 0; s < 99; s++)
		{
			int k_idx, num;
			object_type *o_ptr;
			object_type object_type_body;
			spell_type *s_ptr;

			int len;

			o_ptr = &object_type_body;

			/* Hack -- jump around to display basic prayer/magic books first */
			if (t < TV_SONG_BOOK)
			{
				if (s == 0) s = SV_BOOK_MAX_GOOD;
				else if (s == 98) s = 0;
				else if (s == SV_BOOK_MAX_GOOD) s = 99;
			}

			/* Get kind */
			k_idx = lookup_kind(t, s);

			/* Nothing found */
			if (!k_idx) continue;

			/* Prepare the book */
			object_prep(o_ptr, k_idx);

			/* Description (too brief) */
			object_desc_spoil(desc, sizeof(desc), o_ptr, FALSE, 0);

			/* Get length of book name */
			len = strlen(desc);

			/* Fill the book with spells */
			fill_book(o_ptr,book,&num);

			/* No casters cast from this yet -- except specialist */
			casters_n = 1;

			/* Hack -- null caster for specialist */
			if (casters_n) casters[0] = 0;

			/* Collect some spell information */
			for (i = 0; i < num; i++)
			{
				/* Get spell */
				s_ptr = &s_info[book[i]];

				/* Check length of spell name */
				l = strlen(s_name + s_ptr->name);

				if (l > len) len = l;

				/* Get the casters for the spell */
				for (j = 0; j < MAX_SPELL_CASTERS; j++)
				{
					bool done = FALSE;

					/* Not cast by anyone */
					if (!s_ptr->cast[j].class) continue;

					for (k = 1; k < casters_n; k++)
					{
						/* Already have it */
						if (casters[k] == s_ptr->cast[j].class) done = TRUE;
					}

					/* Add additional caster */
					if (!done) casters[casters_n++] = s_ptr->cast[j].class;
				}
			}

			/* Display the spell casters, 3 per row */
			for (i = 0; i < casters_n; i += 3)
			{
				/* Line 1                 Warrior Mage  Priest   Mage*/

				/* Display header */
				spoiler_out_n_chars(len + 2, ' ');

				/* Get caster details */
				for (j = 0; (i+j < casters_n) && (j < 3); j++)
				{
					/* Fake the class */
					spoil_fake_class(name, sizeof(name),casters[i+j], t, s);

					/* Get the length of the class name */
					l = strlen(name);

					fprintf(fff, "-%s-", name);

					/* Extend space */
					if (l < 12) spoiler_out_n_chars(12 - l, '-');

					/* End of line */
					if ((i+j == casters_n - 1) || (j == 2))
					{
						fprintf(fff, "\n");
					}
					else
					{
						fprintf(fff, "   ");
					}
				}

				/* Line 2  Magic Book of Spells   Lvl Mana Fail   Lvl Mana Fail   Lvl Mana Fail  */

				/* Display book name */
				fprintf(fff, "%s", desc);

				/* Go to next column */
				if ((int) strlen(desc) < len + 2) spoiler_out_n_chars(len + 2 - strlen(desc), ' ');

				/* Get caster details */
				for (j = 0; (i + j < casters_n) && (j < 3); j++)
				{
					int l;

					/* Fake the class */
					spoil_fake_class(name, sizeof(name),casters[i+j], t, s);

					/* Get the length of the class name */
					l = strlen(name);

					fprintf(fff, "Lvl Mana Fail ");

					/* Extend space */
					if (l >= 12) spoiler_out_n_chars(l - 11, ' ');

					/* End of line */
					if ((i+j == casters_n - 1) || (j == 2))
					{
						fprintf(fff, "\n");
					}
					else
					{
						fprintf(fff, "   ");
					}
				}

				/* Line 3 ------------------- --- ---- ---- --- ---- ---- --- ---- ---- */

				/* Display header */
				spoiler_out_n_chars(len, '-');

				/* Gap */
				fprintf(fff, "  ");

				/* Get caster details */
				for (j = 0; (i+j < casters_n) && (j < 3); j++)
				{
					int l;

					/* Fake the class */
					spoil_fake_class(name, sizeof(name),casters[i+j], t, s);

					/* Get the length of the class name */
					l = strlen(name);

					fprintf(fff, "--- ---- ---- ");

					/* Extend space */
					if (l >= 12) spoiler_out_n_chars(l - 11, ' ');

					/* End of line */
					if ((i+j == casters_n - 1) || (j == 2))
					{
						fprintf(fff, "\n");
					}
					else
					{
						fprintf(fff, "   ");
					}
				}

				/* Display spells - 1 per line */
				for (j = 0; j < num; j++)
				{
					s_ptr = &s_info[book[j]];

					/* Display spell name */
					fprintf(fff, "%s", s_name + s_ptr->name);

					/* Go to next column */
					if ((int) strlen(s_name + s_ptr->name) < len + 2) spoiler_out_n_chars(len + 2 - strlen(s_name + s_ptr->name), ' ');

					/* Get caster details */
					for (k = 0; (i+k < casters_n) && (k < 3); k++)
					{
						/* Fake the class */
						spoil_fake_class(name, sizeof(name),casters[i+k], t, s);

						/* Display casting details */
						for (l = 0; l < MAX_SPELL_CASTERS; l++)
						{
							if (s_ptr->cast[l].class == p_ptr->pclass) break;
						}

						if (l < MAX_SPELL_CASTERS)
						{
							/* Display spell details */
							fprintf(fff, "%3d %4d %3d%% ", spell_level(book[j]), s_ptr->cast[l].mana, s_ptr->cast[l].fail);
						}
						else
						{
							/* Display spell details */
							fprintf(fff, "xxx xxxx xxxx ");
						}

						/* End of line */
						if ((i+k == casters_n - 1) || (k == 2))
						{
							fprintf(fff, "\n");
						}
						else
						{
							/* Get the length of the class name */
							l = strlen(name);

							/* Extend space */
							if (l >= 11) spoiler_out_n_chars(l - 11, ' ');

							fprintf(fff, "   ");
						}
					}
				}

				fprintf(fff, "\n\n");
			}
		}

		fprintf(fff, "\n\n");
	}

	/* Restore class information */
	p_ptr->pclass = old_pclass;
	p_ptr->pstyle = old_pstyle;
	p_ptr->psval = old_psval;
	cp_ptr = old_cp_ptr;

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}




/*
 * Create Spoiler files
 */
void do_cmd_spoilers(void)
{
	char ch;


	/* Save screen */
	screen_save();


	/* Interact */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Info */
		prt("Create a spoiler file.", 2, 0);

		/* Prompt for a file */
		prt("(1) Brief Object Info (obj-desc.spo)", 5, 5);
                prt("(2) Full Object Info (obj-info.spo)", 6, 5);
                prt("(3) Full Artifact Info (art-info.spo)", 7, 5);
                prt("(4) Brief Monster Info (mon-desc.spo)", 8, 5);
                prt("(5) Full Monster Info (mon-info.spo)", 9, 5);
                prt("(6) Brief Terrain Info (ter-desc.spo)", 10, 5);
                prt("(7) Full Terrain Info (ter-info.spo)", 11, 5);
                prt("(8) Brief Ego Item Info (ego-desc.spo)", 12, 5);
                prt("(9) Slay Table (slay-tbl.spo)", 13, 5);
                prt("(0) Spell Casting (mag-info.spo)", 14, 5);


		/* Prompt */
		prt("Command: ", 16, 0);

		/* Get a choice */
		ch = inkey();

		/* Escape */
		if (ch == ESCAPE)
		{
			break;
		}

		/* Option (1) */
		else if (ch == '1')
		{
			spoil_obj_desc("obj-desc.spo");
		}

		/* Option (2) */
                else if (ch == '2')
		{
			spoil_object("obj-info.spo");
		}

		/* Option (2) */
                else if (ch == '3')
		{
			spoil_artifact("art-info.spo");
		}

		/* Option (4) */
                else if (ch == '4')
		{
			spoil_mon_desc("mon-desc.spo");
		}

		/* Option (5) */
                else if (ch == '5')
		{
			spoil_mon_info("mon-info.spo");
		}

		/* Option (6) */
                else if (ch == '6')
		{
			spoil_feat_desc("ter-desc.spo");
		}

		/* Option (7) */
                else if (ch == '7')
		{
			spoil_feat_info("ter-info.spo");
		}

		/* Option (8) */
                else if (ch == '8')
		{
			spoil_ego_desc("ego-desc.spo");
		}

		/* Option (9) */
                else if (ch == '9')
		{
			spoil_slay_table("slay-tbl.spo");
		}

		/* Option (9) */
                else if (ch == '0')
		{
			spoil_magic_info("mag-info.spo");
		}

		/* Oops */
		else
		{
			bell("Illegal command for spoilers!");
		}

		/* Flush messages */
		message_flush();
	}


	/* Load screen */
	screen_load();
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif
