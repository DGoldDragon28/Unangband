/* File: monster1.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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


/*
 * Pronoun arrays, by gender.
 */
static cptr wd_he[3] =
{ "it", "he", "she" };
static cptr wd_his[3] =
{ "its", "his", "her" };


/*
 * Pluralizer.  Args(count, singular, plural)
 */
#define plural(c,s,p) \
	(((c) == 1) ? (s) : (p))






/*
 * Determine if the "armor" is known
 * The higher the level, the fewer 'missable' blows needed.
 */
static bool know_armour(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	s32b level = r_ptr->level;

	s32b blows = l_ptr->tblows;

	/* Normal monsters */
	if (blows > 304 / (4 + level)) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if (blows > 304 / (38 + (5 * level) / 4)) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*
 * Determine if the "life rating" is known
 * The higher the level, the fewer damaging blows needed.
 */
static bool know_life_rating(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	s32b level = r_ptr->level;

	s32b damage = l_ptr->tdamage;

	/* Normal monsters */
	if (damage > 304 / (4 + level)) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if (damage > 304 / (38 + (5 * level) / 4)) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*
 * Determine if the "damage" of the given attack is known
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 */
static bool know_damage(const monster_race *r_ptr, const monster_lore *l_ptr, int i)
{
	s32b level = r_ptr->level;

	s32b a = l_ptr->blows[i];

	s32b d1 = r_ptr->blow[i].d_dice;
	s32b d2 = r_ptr->blow[i].d_side;

	s32b d = d1 * d2;

	/* Normal monsters */
	if ((4 + level) * a > 80 * d) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if ((4 + level) * (2 * a) > 80 * d) return (TRUE);

	/* Assume false */
	return (FALSE);
}


static void describe_monster_desc(const monster_race *r_ptr)
{
	char buf[2048];

	char *s, *t;

	int match = (r_ptr->flags1 & (RF1_FEMALE)) ? 2 : 1;

	int state = 0;

#ifdef DELAY_LOAD_R_TEXT

	int fd;

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "monster.raw");

	/* Open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Use file */
	if (fd >= 0)
	{
		long pos;

		/* Starting position */
		pos = r_ptr->text;

		/* Additional offsets */
		pos += r_head->head_size;
		pos += r_head->info_size;
		pos += r_head->name_size;

#if 0

		/* Maximal length */
		len = r_head->text_size - r_ptr->text;

		/* Actual length */
		for (i = r_idx+1; i < z_info->r_max; i++)
		{
			/* Actual length */
			if (r_info[i].text > r_ptr->text)
			{
				/* Extract length */
				len = r_info[i].text - r_ptr->text;

				/* Done */
				break;
			}
		}

		/* Maximal length */
		if (len > 2048) len = 2048;

#endif

		/* Seek */
		fd_seek(fd, pos);

		/* Read a chunk of data */
		fd_read(fd, buf, sizeof(buf));

		/* Close it */
		fd_close(fd);
	}

#else

	/* Simple method */
	my_strcpy(buf, r_text + r_ptr->text, sizeof(buf));

#endif

	/* Remove gender sensitivity */
	for (t = s = buf; *s; s++)
	{
		if (*s == '|')
		{
			state++;
			if (state == 3) state = 0;
		}
		else if (!state || (state == match))
		{
			*t++ = *s;
		}
	}

	/* Terminate buffer */
	*t = '\0';

	/* Dump it */
	text_out(buf);

	if (strlen(buf)) text_out("  ");
}


static void describe_monster_spells(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int m, n;
	int msex = 0;
	bool ranged = l_ptr->flags4 & (RF4_BLOW_1 | RF4_BLOW_2 | RF4_BLOW_3 | RF4_BLOW_4);
	bool innate = FALSE;
	bool breath = FALSE;
	bool magic = FALSE;
	int vn;
	cptr vp[128];
	int vm[128];
	int ve[128];

	bool powerful = FALSE;

	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Collect innate attacks */
	vn = 0;
	if (l_ptr->flags4 & (RF4_ADD_AMMO))      vp[vn++] = "grow ammunition";
	if (l_ptr->flags4 & (RF4_QUAKE))      vp[vn++] = "create earthquakes";
	/*if (l_ptr->flags4 & (RF4_EXPLODE))     vp[vn++] = "explode";*/
	/*if (l_ptr->flags4 & (RF4_AURA)) vp[vn++] = "radiate a powerful aura";*/

	/* Describe innate attacks */
	if (vn)
	{
		innate = TRUE;

		/* Intro */
		if (ranged) text_out(" and");
		else text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" may ");
			else if (n < vn-1) text_out(", ");
			else text_out(" or ");

			/* Dump */
			text_out(vp[n]);
		}
	}

	/* Collect breaths */
	vn = 0;
	
	/* Iterate through breaths */
	for (m = 8; m < 32; m++)
	{
		if (l_ptr->flags4 & (1L << m))
		{
			ve[vn] = method_info[m + 96].d_res;
			vp[vn] = effect_text + effect_info[ve[vn]].desc[0];
			
			vn++;
		}
	}

	/* Describe breaths */
	if (vn)
	{
		/* Already innate? */
		if ((innate) || (ranged)) text_out(" and");

		/* Intro */
		else text_out(format("%^s", wd_he[msex]));

		/* Note breath */
		breath = TRUE;

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" may breathe ");
			else if (n < vn-1) text_out(", ");
			else text_out(" or ");

			/* Dump */
			text_out(vp[n]);
			
			/* Total casting - knowing life rating is enough, or seeing monster attack 10 times */
			if ((know_life_rating(r_ptr, l_ptr)) || (l_ptr->cast_innate >= 10))
			{
				int dam = get_breath_dam(r_ptr->hp, ve[n], (l_ptr->flags2 & RF2_POWERFUL));
				
				if (dam)
				{
					/* Hack -- we assume all breaths are damaging for simplicity */
					text_out(format(" for %d damage", dam));
					
					/* Monster hp varies */
					if (!(l_ptr->flags1 & RF1_FORCE_MAXHP))
					{
						text_out(" on average");
					}
				}
			}
		}
		
		/* Note that breathes vary by monster hp */
		text_out(" when at full health");
	}

	/* End the sentence about innate spells */
	if ((innate) || (ranged) || (breath))
	{
		if ((l_ptr->flags2 & RF2_POWERFUL) && (breath)) text_out(" powerfully");

		/* Total casting */
		m = l_ptr->cast_innate;

		/* Get frequency */
		n = r_ptr->freq_innate;

		/* Describe the spell frequency */
		if ((m > 100) && (n > 0))
		{
			text_out(format("; 1 time in %d", 100 / n));
		}

		/* Guess at the frequency */
		else if ((m) && (n > 0))
		{
			n = ((n + 9) / 10) * 10;
			text_out(format("; about 1 time in %d", 100 / n));
		}
	}


	/* Collect spells */
	vn = 0;
	
	/* Iterate through spells - flags 5 */
	for (m = 0; m < 32; m++)
	{
		if (l_ptr->flags5 & (1L << m))
		{
			ve[vn] = method_info[m + 128].d_res;
			vm[vn++] = m + 128;
		}
	}

	/* Iterate through spells - flags 6 */
	for (m = 0; m < 32; m++)
	{
		if (l_ptr->flags6 & (1L << m))
		{
			ve[vn] = method_info[m + 160].d_res;
			vm[vn++] = m + 160;
		}
	}

	/* Iterate through spells - flags 7 */
	for (m = 0; m < 32; m++)
	{
		if (l_ptr->flags7 & (1L << m))
		{
			ve[vn] = method_info[m + 192].d_res;
			vm[vn++] = m + 192;
		}
	}
	
	/* Hack -- powerful spells here */
	if (l_ptr->flags6 & (RF6_FORGET | RF6_ILLUSION | RF6_SCARE | RF6_BLIND | RF6_CONF | RF6_SLOW | RF6_HOLD)) powerful = TRUE;

	/* Describe spells */
	if (vn)
	{
		/* Note magic */
		magic = TRUE;

		/* Intro */
		if ((innate) || (ranged) || (breath))
		{
			text_out(" and is also");
		}
		else
		{
			text_out(format("%^s is", wd_he[msex]));
		}

		/* Verb Phrase */
		text_out(" magical, casting spells");

		/* Adverb */
		if (l_ptr->flags2 & RF2_SMART) text_out(" intelligently");

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			char buf[80];
			char buf2[80];
			
			int dam;
			
			bool self = (method_info[vm[n]].flags1 & (PROJECT_SELF)) != 0;
			
			/* Intro */
			if (n == 0) text_out(" which ");
			else if (n < vn-1) text_out(", ");
			else text_out(" or ");
			
			/* Clear buffers */
			buf[0] = '\0';
			buf2[0] = '\0';

			/* Get the damage */
			dam = get_dam(r_ptr->spell_power, vm[n], FALSE);
			
			/* Get the attack description */
			attack_desc(buf2, -1, vm[n], ve[n], dam, (ATK_DESC_NO_STOP) | (self ? (ATK_DESC_SELF) : 0), sizeof(buf2));
			
			/* Seeing monster attack 10 times is enough for specific damage */
			if ((dam) && (l_ptr->cast_innate >= 10))
			{
				/* Hack -- we assume all breaths are damaging for simplicity */
				my_strcpy(buf, format("%d", dam), sizeof(buf));
			}

			/* Use blow description if we have punctuation */
			if (strchr(buf2, '.') || strchr(buf2,'!') || !strlen(buf2))
			{
				/* Describe the blow */
				describe_blow(vm[n], ve[n], r_ptr->level, 0, NULL, buf, (self ? (msex == 2 ? 0x440: (msex == 1 ? 0x240 : 0x40)): 0), 1);
			}
			else
			{
				text_out(format("%s", buf2));
				
				if ((dam) && (l_ptr->cast_innate >= 10))
				{
					text_out(" ");
					text_out(format("for %s damage on average",buf));
				}
			}
		}
	}

	/* End the sentence about magic spells */
	if (magic)
	{
		/* Adverb */
		if ((l_ptr->flags2 & RF2_POWERFUL) && (powerful)) text_out(" powerfully enough to overcome your resistance");

		/* Total casting */
		m = l_ptr->cast_spell;

		/* Average frequency */
		n = r_ptr->freq_spell;

		/* Describe the spell frequency */
		if ((m > 100) && (n > 0))
		{
			text_out(format("; 1 time in %d", 100 / n));
		}

		/* Guess at the frequency */
		else if ((m) && (n > 0))
		{
			n = ((n + 9) / 10) * 10;
			text_out(format("; about 1 time in %d", 100 / n));
		}
	}

	if (innate || magic || ranged || breath)
	{
		/* End this sentence */
		text_out(".  ");
	}
}


static void describe_monster_drop(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	bool sin = FALSE;
	bool plu = TRUE;

	int n;

	cptr p;

	int msex = 0;

	int vn;
	cptr vp[64];

	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Drops gold and/or items */
	if (l_ptr->drop_gold || l_ptr->drop_item)
	{
		/* Count maximum drop */
		n = MAX(l_ptr->drop_gold, l_ptr->drop_item);

		/* Intro */
		text_out(format("%^s may carry", wd_he[msex]));

		/* One drop (may need an "n") */
		if (n == 1)
		{
			text_out(" a");
			sin = TRUE;
			plu = FALSE;
		}

		/* Two drops */
		else if (n == 2)
		{
			text_out(" one or two");
		}

		/* Many drops */
		else
		{
			text_out(format(" up to %d", n));
		}


		/* Great */
		if (l_ptr->flags1 & RF1_DROP_GREAT)
		{
			p = " exceptional ";
		}

		/* Good (no "n" needed) */
		else if (l_ptr->flags1 & RF1_DROP_GOOD)
		{
			p = " good ";
			sin = FALSE;
		}

		/* Okay */
		else
		{
			p = " ";
		}

		/* Collect special abilities. */
		vn = 0;

		/* Objects */
		if (l_ptr->drop_item)
		{
			if (l_ptr->flags8 & (RF8_DROP_CHEST)) vp[vn++] = "chest";
			if (l_ptr->flags8 & (RF8_DROP_WEAPON)) vp[vn++] = "weapon";
			if (l_ptr->flags8 & (RF8_DROP_MISSILE)) vp[vn++] = "missile weapon";
			if (l_ptr->flags8 & (RF8_DROP_ARMOR)) vp[vn++] = "armour";
			if (l_ptr->flags8 & (RF8_DROP_CLOTHES)) vp[vn++] = "garment";
			if (l_ptr->flags8 & (RF8_DROP_TOOL)) vp[vn++] = "tool";
			if (l_ptr->flags8 & (RF8_DROP_LITE)) vp[vn++] = "lite";
			if (l_ptr->flags8 & (RF8_DROP_JEWELRY)) vp[vn++] = "adornment";
			if (l_ptr->flags8 & (RF8_DROP_RSW)) vp[vn++] = "magical device";
			if (l_ptr->flags8 & (RF8_DROP_WRITING)) vp[vn++] = "written item";
			if (l_ptr->flags8 & (RF8_DROP_MUSIC)) vp[vn++] = "musical item";
			if (l_ptr->flags8 & (RF8_DROP_POTION)) vp[vn++] = "potion";
			if (l_ptr->flags8 & (RF8_DROP_FOOD)) vp[vn++] = "edible item";
			if (l_ptr->flags8 & (RF8_DROP_JUNK)) vp[vn++] = "junk item";
			if (l_ptr->flags9 & (RF9_DROP_ESSENCE)) vp[vn++] = "essence";

			/* Only drop mushrooms? */
			if (l_ptr->flags9 & (RF9_DROP_MUSHROOM))
			{
				vn = 0;
				vp[vn++] = "mushroom";
			}

			if (!vn) vp[vn++] = "special object";
		}

		/* Treasures */
		if (l_ptr->drop_gold)
		{
			/* Dump "treasure(s)" */
			if (l_ptr->flags9 & (RF9_DROP_MINERAL))
			{
				int coin_type = get_coin_type(r_ptr);

				switch (k_info[coin_type + OBJ_GOLD_LIST].tval)
				{
					case TV_GOLD:
						vp[vn++] = "precious metal";
						break;
					case TV_GEMS:
						vp[vn++] = "gem stone";
						break;
				}
			}
			else vp[vn++] = "treasure";
		}

		/* Fix up singular */
		if ((vn) && !(is_a_vowel(vp[0][0]))) sin = FALSE;

		/* Describe special abilities. */
		if (vn)
		{
			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (n == 0) { if (sin) text_out("n"); text_out(p);  }
				else if (n < vn-1) text_out(", ");
				else text_out(" or ");

				/* Dump */
				text_out(format("%s%s", vp[n], (plu ? "s" : "")));
			}

			/* End */
			text_out(".  ");
		}
	}
}


static void describe_monster_attack(const monster_race *r_ptr, const monster_lore *l_ptr, bool ranged)
{
	int m, n, r;

	int msex = 0;

	char buf[80];

	int method;
	method_type *method_ptr;

	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Count the number of "known" attacks */
	for (n = 0, m = 0; m < 4; m++)
	{
		/* Extract the attack info */
		method = r_ptr->blow[m].method;

		/* Skip non-attacks */
		if (!method) continue;

		/* Get method */
		method_ptr = &method_info[method];

		/* Ranged? */
		if ((ranged) && !(method_ptr->flags2 & (PR2_RANGED))) continue;

		/* Melee? */
		if ((!ranged) && !(method_ptr->flags2 & (PR2_MELEE))) continue;

		/* Count known attacks */
		if (l_ptr->blows[m]) n++;
	}

	/* Examine (and count) the actual attacks */
	for (r = 0, m = 0; m < 4; m++)
	{
		int method, effect, d1, d2;

		bool detail = FALSE;

		/* Skip unknown attacks */
		if (!l_ptr->blows[m]) continue;

		/* Extract the attack info */
		method = r_ptr->blow[m].method;
		effect = r_ptr->blow[m].effect;
		d1 = r_ptr->blow[m].d_dice;
		d2 = r_ptr->blow[m].d_side;

		/* Skip non-attacks */
		if (!method) continue;

		/* Get method */
		method_ptr = &method_info[method];

		/* Ranged? */
		if ((ranged) && !(method_ptr->flags2 & (PR2_RANGED))) continue;

		/* Melee? */
		if ((!ranged) && !(method_ptr->flags2 & (PR2_MELEE))) continue;

		/* Confirm ranged blow */
		if ((ranged) && !(l_ptr->flags4 & (RF4_BLOW_1 << m))) continue;

		/* Introduce the attack description */
		if (!r)
		{
			if (ranged) text_out(format("%^s may ", wd_he[msex]));
			else text_out(format("%^s can ", wd_he[msex]));
		}
		else if (r < n-1)
		{
			text_out(", ");
		}
		else if (ranged)
		{
			text_out(" or ");
		}
		else
		{
			text_out(" and ");
		}

		/* Damage is known */
		if (d1 && d2 && know_damage(r_ptr, l_ptr, m))
		{
			my_strcpy(buf, format("%dd%d", d1, d2), sizeof(buf));
			detail = TRUE;
		}

		/* Describe the blow */
		describe_blow(method, effect, 0, d1 * d2, NULL, buf, 0x20 | (detail ? 0 : 0x10), 1);

		/* Count the attacks as printed */
		r++;
	}

	/* Continue with spells after this */
	if (ranged)
	{
		/* Nothing */
	}
	/* Finish sentence above */
	else if (r)
	{
		if (l_ptr->flags3 & (RF3_HUGE)) text_out(" up to 2 grids away");
		text_out(".  ");
	}

	/* Notice lack of attacks */
	else if (l_ptr->flags1 & RF1_NEVER_BLOW)
	{
		text_out(format("%^s has no physical attacks.  ", wd_he[msex]));
	}

	/* Or describe the lack of knowledge */
	else
	{
		text_out(format("Nothing is known about %s attack.  ", wd_his[msex]));
	}
}


static void describe_monster_abilities(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int n;

	int vn;
	cptr vp[64];

	int msex = 0;


	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Collect special abilities. */
	vn = 0;
	if ((l_ptr->flags2 & RF2_NEED_LITE) && !(l_ptr->flags2 & RF2_HAS_LITE)) vp[vn++] = "carry a lite to see you";
	if (l_ptr->flags2 & RF2_OPEN_DOOR) vp[vn++] = "open doors";
	if (l_ptr->flags2 & RF2_BASH_DOOR) vp[vn++] = "bash down doors";
	if (l_ptr->flags2 & RF2_PASS_WALL) vp[vn++] = "pass through walls";
	if (l_ptr->flags2 & RF2_KILL_WALL) vp[vn++] = "bore through walls";
	if (l_ptr->flags2 & RF2_EAT_BODY) vp[vn++] = "can eat bodies to regain strength";
	if (l_ptr->flags2 & RF2_TAKE_ITEM) vp[vn++] = "pick up objects";
	if (l_ptr->flags3 & RF3_OOZE) vp[vn++] = "ooze through tiny cracks";
	if (l_ptr->flags2 & RF2_CAN_CLIMB) vp[vn++] = "climb on walls and ceilings";
	if (l_ptr->flags2 & RF2_CAN_DIG) vp[vn++] = "dig through earth and rubble";
	if (l_ptr->flags2 & RF2_TRAIL) vp[vn++] = "leave a trail behind it";
	if (l_ptr->flags2 & RF2_SNEAKY) vp[vn++] = "hide in unusual places";
	if ((l_ptr->flags2 & RF2_CAN_SWIM) && !(l_ptr->flags2 & RF2_MUST_SWIM)) vp[vn++] = "swim under water";
	if ((l_ptr->flags2 & RF2_CAN_FLY) && !(l_ptr->flags2 & RF2_MUST_FLY)) vp[vn++] = "fly over obstacles";
	if (l_ptr->flags3 & RF3_NONVOCAL) vp[vn++] = "communicate telepathically with its own kind";
	if (l_ptr->flags9 & RF9_EVASIVE) vp[vn++] = "easily evade blows and missiles";

	if (l_ptr->flags9 & (RF9_SUPER_SCENT))
	{
		if (l_ptr->flags9 & (RF9_WATER_SCENT))
		{
			if (l_ptr->flags9 & (RF9_SCENT)) vp[vn++] = "track you unerringly over land and through water";
			else vp[vn++] = "track you unerringly through water";
		}
		else
		{
			if (l_ptr->flags9 & (RF9_SCENT)) vp[vn++] = "track you unerringly over land, but not through water";
			else vp[vn++] = "track you unerringly over land, but not through water";
		}
	}
	else
	{
		if (l_ptr->flags9 & (RF9_WATER_SCENT))
		{
			if (l_ptr->flags9 & (RF9_SCENT)) vp[vn++] = "track you over land and through water";
			else vp[vn++] = "track you through water";
		}
		else
		{
			if (l_ptr->flags9 & (RF9_SCENT)) vp[vn++] = "track you over land, but not through water";
		}
	}

	/* Describe special abilities. */
	if (vn)
	{
		/* Intro */
		text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" can ");
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");

			/* Dump */
			text_out(vp[n]);
		}

		/* End */
		text_out(".  ");
	}

	/* Describe special abilities. */
	if (l_ptr->flags2 & (RF2_MUST_SWIM))
	{
		text_out(format("%^s must swim and cannot move out of water.  ", wd_he[msex]));
	}

	/* Describe special abilities. */
	if (l_ptr->flags2 & (RF2_MUST_FLY))
	{
		text_out(format("%^s must fly and cannot move underwater or through webs.  ", wd_he[msex]));
	}

	/* Describe special abilities. */
	if (l_ptr->flags2 & (RF2_HAS_LITE))
	{
		if (l_ptr->flags2 & RF2_NEED_LITE) text_out(format("%^s always carries a lite to see you.  ", wd_he[msex]));
		else text_out(format("%^s lights up the surroundings.  ", wd_he[msex]));
	}
	else if (l_ptr->flags2 & (RF2_NEED_LITE))
	{
		text_out(format("%^s needs lite to see you.  ", wd_he[msex]));
	}


	/* Describe special abilities. */
	if (l_ptr->flags2 & RF2_INVISIBLE)
	{
		text_out(format("%^s is invisible.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_COLD_BLOOD)
	{
		text_out(format("%^s is cold blooded.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_EMPTY_MIND)
	{
		text_out(format("%^s is not detected by telepathy.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_WEIRD_MIND)
	{
		text_out(format("%^s is rarely detected by telepathy.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_MULTIPLY)
	{
		text_out(format("%^s breeds explosively.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_REGENERATE)
	{
		text_out(format("%^s regenerates quickly.  ", wd_he[msex]));
	}
	if (l_ptr->flags2 & RF2_HAS_WEB)
	{
		text_out(format("%^s appears in a giant web.  ", wd_he[msex]));
	}
	/* Resist blindness also indicates see invisible */
	if (l_ptr->flags9 & RF9_RES_BLIND)
	{
		text_out(format("%^s can see invisible.  ", wd_he[msex]));
	}


	/* Collect susceptibilities */
	vn = 0;
	if (l_ptr->flags3 & RF3_HURT_ROCK) vp[vn++] = "rock remover";
	if (l_ptr->flags3 & RF3_HURT_LITE) vp[vn++] = "bright light";
	if (l_ptr->flags3 & RF3_HURT_WATER) vp[vn++] = "water remover";

	/* Describe susceptibilities */
	if (vn)
	{
		/* Intro */
		text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" is hurt by ");
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");

			/* Dump */
			text_out(vp[n]);
		}

		/* End */
		text_out(".  ");
	}


	/* Collect immunities */
	vn = 0;
	if (l_ptr->flags3 & RF3_IM_ACID) vp[vn++] = "acid";
	if (l_ptr->flags3 & RF3_IM_ELEC) vp[vn++] = "lightning";
	if (l_ptr->flags3 & RF3_IM_FIRE) vp[vn++] = "fire";
	if (l_ptr->flags3 & RF3_IM_COLD) vp[vn++] = "cold";
	if (l_ptr->flags3 & RF3_IM_POIS) vp[vn++] = "poison";
	if ((l_ptr->flags9 & RF9_IM_EDGED) && (l_ptr->flags9 & RF9_IM_BLUNT)) vp[vn++] = "edged";
	else if (l_ptr->flags9 & RF9_IM_EDGED) vp[vn++] = "edged weapons";
	if (l_ptr->flags9 & RF9_IM_BLUNT) vp[vn++] = "blunt weapons";

	/* Describe immunities */
	if (vn)
	{
		/* Intro */
		text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" is immune to ");
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");

			/* Dump */
			text_out(vp[n]);
		}

		/* End */
		text_out(".  ");
	}


	/* Collect resistances */
	vn = 0;
	if (l_ptr->flags3 & RF3_RES_WATER) vp[vn++] = "water";
	if (l_ptr->flags3 & RF3_RES_NETHR) vp[vn++] = "nether";
	if (l_ptr->flags3 & RF3_RES_LAVA) vp[vn++] = "lava";
	if (l_ptr->flags3 & RF3_RES_PLAS) vp[vn++] = "plasma";
	if (l_ptr->flags3 & RF3_RES_NEXUS) vp[vn++] = "nexus";
	if (l_ptr->flags3 & RF3_RES_DISEN) vp[vn++] = "disenchantment";
	if (l_ptr->flags9 & RF9_RES_BLIND) vp[vn++] = "blindness";
	if (l_ptr->flags9 & RF9_RES_LITE) vp[vn++] = "lite";
	if (l_ptr->flags9 & RF9_RES_DARK) vp[vn++] = "darkness";
	if (l_ptr->flags9 & RF9_RES_CHAOS) vp[vn++] = "chaos";
	if (l_ptr->flags9 & RF9_RES_TPORT) vp[vn++] = "teleportation";
	if (l_ptr->flags9 & RF9_RES_MAGIC) vp[vn++] = "magical spells";
	if (l_ptr->flags9 & RF9_RES_MAGIC) vp[vn++] = "the effects of rods, staffs and wands";
	if ((l_ptr->flags9 & RF9_RES_EDGED) && (l_ptr->flags9 & RF9_RES_BLUNT)) vp[vn++] = "edged";
	else if (l_ptr->flags9 & RF9_RES_EDGED) vp[vn++] = "edged weapons";
	if (l_ptr->flags9 & RF9_RES_BLUNT) vp[vn++] = "blunt weapons";

	/* Describe resistances */
	if (vn)
	{
		/* Intro */
		text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" resists ");
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");

			/* Dump */
			text_out(vp[n]);
		}

		/* End */
		text_out(".  ");
	}


	/* Collect non-effects */
	vn = 0;
	if (l_ptr->flags3 & RF3_NO_STUN) vp[vn++] = "stunned";
	if (l_ptr->flags3 & RF3_NO_FEAR) vp[vn++] = "frightened";
	if (l_ptr->flags3 & RF3_NO_CONF) vp[vn++] = "confused";
	if (l_ptr->flags3 & RF3_NO_SLEEP) vp[vn++] = "charmed";
	if (l_ptr->flags3 & RF3_NO_SLEEP) vp[vn++] = "slept";
	if (l_ptr->flags9 & RF9_NO_CUTS) vp[vn++] = "cut";
	if (l_ptr->flags9 & RF9_NO_SLOW) vp[vn++] = "slowed";
	if (l_ptr->flags9 & RF9_NO_SLOW) vp[vn++] = "paralyzed";

	/* Describe non-effects */
	if (vn)
	{
		/* Intro */
		text_out(format("%^s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" cannot be ");
			else if (n < vn-1) text_out(", ");
			else text_out(" or ");

			/* Dump */
			text_out(vp[n]);
		}

		/* End */
		text_out(".  ");
	}


	/* Do we know how aware it is? */
	if ((((int)l_ptr->wake * (int)l_ptr->wake) > r_ptr->sleep) ||
	    (l_ptr->ignore == MAX_UCHAR) ||
	    ((r_ptr->sleep == 0) && (l_ptr->tkills >= 10)))
	{
		cptr act;

		if (r_ptr->sleep > 200)
		{
			act = "prefers to ignore";
		}
		else if (r_ptr->sleep > 95)
		{
			act = "pays very little attention to";
		}
		else if (r_ptr->sleep > 75)
		{
			act = "pays little attention to";
		}
		else if (r_ptr->sleep > 45)
		{
			act = "tends to overlook";
		}
		else if (r_ptr->sleep > 25)
		{
			act = "takes quite a while to see";
		}
		else if (r_ptr->sleep > 10)
		{
			act = "takes a while to see";
		}
		else if (r_ptr->sleep > 5)
		{
			act = "is fairly observant of";
		}
		else if (r_ptr->sleep > 3)
		{
			act = "is observant of";
		}
		else if (r_ptr->sleep > 1)
		{
			act = "is very observant of";
		}
		else if (r_ptr->sleep > 0)
		{
			act = "is vigilant for";
		}
		else
		{
			act = "is ever vigilant for";
		}

		text_out(format("%^s %s intruders, which %s may notice from %d feet.  ",
		    wd_he[msex], act, wd_he[msex], 10 * r_ptr->aaf));
	}

	/* Describe escorts */
	if ((l_ptr->flags1 & RF1_ESCORT) || (l_ptr->flags1 & RF1_ESCORTS))
	{
		text_out(format("%^s usually appears with escorts.  ",
		    wd_he[msex]));
	}

	/* Describe friends */
	else if (l_ptr->flags1 & RF1_FRIEND)
	{
		text_out(format("%^s usually appears with a friend.  ",
		    wd_he[msex]));
	}

	/* Describe friends */
	else if (l_ptr->flags1 & RF1_FRIENDS)
	{
		text_out(format("%^s usually appears in groups.  ",
		    wd_he[msex]));
	}
}


static void describe_monster_kills(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int msex = 0;


	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;


	/* Treat uniques differently */
	if (l_ptr->flags1 & RF1_UNIQUE)
	{
		/* Hack -- Determine if the unique is "dead" */
		bool dead = (r_ptr->max_num == 0) ? TRUE : FALSE;

		/* We've been killed... */
		if (l_ptr->deaths)
		{
			/* Killed ancestors */
			text_out(format("%^s has slain %d of your ancestors",
			    wd_he[msex], l_ptr->deaths));

			/* But we've also killed it */
			if (dead)
			{
				text_out(", but you have taken revenge!  ");
			}

			/* Unavenged (ever) */
			else
			{
				text_out(format(", who %s unavenged.  ",
				    plural(l_ptr->deaths, "remains", "remain")));
			}
		}

		/* Dead unique who never hurt us */
		else if (dead)
		{
			text_out("You have slain this foe.  ");
		}
		else
		{
			/* Alive and never killed us */
		}
	}

	/* Not unique, but killed us */
	else if (l_ptr->deaths)
	{
		/* Dead ancestors */
		text_out(format("%d of your ancestors %s been killed by this creature, ",
		    l_ptr->deaths, plural(l_ptr->deaths, "has", "have")));

		/* Some kills this life */
		if (l_ptr->pkills)
		{
			text_out(format("and you have exterminated at least %d of the creatures.  ",
			    l_ptr->pkills));
		}

		/* Some kills past lives */
		else if (l_ptr->tkills)
		{
			text_out(format("and %s have exterminated at least %d of the creatures.  ",
			    "your ancestors", l_ptr->tkills));
		}

		/* No kills */
		else
		{
			text_out(format("and %s is not ever known to have been defeated.  ",
			    wd_he[msex]));
		}
	}

	/* Normal monsters */
	else
	{
		/* Killed some this life */
		if (l_ptr->pkills)
		{
			text_out(format("You have killed at least %d of these creatures.  ",
			    l_ptr->pkills));
		}

		/* Killed some last life */
		else if (l_ptr->tkills)
		{
			text_out(format("Your ancestors have killed at least %d of these creatures.  ",
			    l_ptr->tkills));
		}

		/* Killed none */
		else
		{
			text_out("No battles to the death are recalled.  ");
		}
	}
}


static void describe_monster_toughness(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int msex = 0;
	bool intro = FALSE;

	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Describe monster "toughness" */
	if (know_armour(r_ptr, l_ptr))
	{
		/* Armor */
		text_out(format("%^s has an armor rating of %d",
		    wd_he[msex], r_ptr->ac));
		intro = TRUE;
	}
	
	/* Describe monster "toughness" */
	if (know_life_rating(r_ptr, l_ptr))
	{
		if (intro)
		{
			text_out(" and");
		}
		else
		{
			text_out(format("%^s has",
				    wd_he[msex]));
		}
		
		/* Maximized hitpoints */
		if (l_ptr->flags1 & RF1_FORCE_MAXHP)
		{
			text_out(format(" a life rating of %d.  ",
			    r_ptr->hp));
		}

		/* Variable hitpoints */
		else
		{
			text_out(format(" an average life rating of %d.  ",
			    r_ptr->hp));
		}
	}
}


static void describe_monster_exp(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	cptr p, q;

	long i, j;

	s32b mult, div;

	/* Describe experience if known */
	if (l_ptr->tkills)
	{
		/* Introduction */
		if ((l_ptr->flags3 & RF3_NONLIVING) && (l_ptr->flags1 & RF1_UNIQUE))
			text_out("Destroying");
		else if (l_ptr->flags1 & RF1_UNIQUE)
			text_out("Killing");
		else if (l_ptr->flags3 & RF3_NONLIVING)
			text_out("Destruction of");
		else
			text_out("A kill of");

		text_out(" this creature");

		/* 10 + killed monster level */
		mult = 10 + r_ptr->level;

		/* 10 + maximum player level */
		div = 10 + p_ptr->max_lev;

		/* calculate the integer exp part */
		i = (long)r_ptr->mexp * mult / div;

		/* calculate the fractional exp part scaled by 100, */
		/* must use long arithmetic to avoid overflow */
		j = ((((long)r_ptr->mexp * mult % div) *
			  (long)1000 / div + 5) / 10);

		/* Mention the experience */
		text_out(format(" is worth %ld.%02ld point%s",
			(long)i, (long)j,
			(((i == 1) && (j == 0)) ? "" : "s")));

		/* Take account of annoying English */
		p = "th";
		i = p_ptr->max_lev % 10;
		if ((p_ptr->max_lev / 10) == 1) /* nothing */;
		else if (i == 1) p = "st";
		else if (i == 2) p = "nd";
		else if (i == 3) p = "rd";

		/* Take account of "leading vowels" in numbers */
		q = "";
		i = p_ptr->max_lev;
		if ((i == 8) || (i == 11) || (i == 18)) q = "n";

		/* Mention the dependance on the player's level */
		text_out(format(" for a%s %lu%s level character.  ",
			q, (long)i, p));
	}
}


static void describe_monster_power(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int msex = 0;

	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Describe experience if known */
	if (l_ptr->tkills)
	{
		text_out(format("%^s is ", wd_he[msex]));

		if (r_ptr->power > 100) text_out("over powered");
		else if (r_ptr->power > 50) text_out("deadly");
		else if (r_ptr->power > 10) text_out("dangerous");
		else if (r_ptr->power > 5) text_out("challenging");
		else if (r_ptr->power > 1) text_out("threatening");
		else text_out("not threatening");

		text_out(format(" at %s native depth.  ", wd_his[msex]));
	}
}


static void describe_monster_movement(const monster_race *r_ptr, const monster_lore *l_ptr)
{
	int vn;
	cptr vp[64];
	int n;

	bool old = FALSE;

	text_out("This");

	/* Describe the "quality" */
	if (l_ptr->flags3 & (RF3_EVIL)) text_out(" evil");
	if (l_ptr->flags3 & (RF3_UNDEAD)) text_out(" undead");

	/* Collect races */
	vn = 0;

	/* Describe the "race" */
	if ((l_ptr->flags1 & (RF1_FEMALE)) && (l_ptr->flags9 & (RF9_MAN))) vp[vn++] = "woman";
	else if (l_ptr->flags9 & (RF9_MAN)) vp[vn++] ="man";
	if (l_ptr->flags9 & (RF9_ELF)) vp[vn++] ="elf";
	if (l_ptr->flags9 & (RF9_DWARF)) vp[vn++] ="dwarf";
	if (l_ptr->flags3 & (RF3_ORC)) vp[vn++] ="orc";
	if (l_ptr->flags3 & (RF3_TROLL)) vp[vn++] ="troll";
	if (l_ptr->flags3 & (RF3_GIANT)) vp[vn++] ="giant";
	if (l_ptr->flags3 & (RF3_ANIMAL)) vp[vn++] ="animal";
	if (l_ptr->flags3 & (RF3_DRAGON)) vp[vn++] ="dragon";
	if (l_ptr->flags3 & (RF3_DEMON)) vp[vn++] ="demon";
	if (l_ptr->flags3 & (RF3_PLANT)) vp[vn++] ="plant";
	if (l_ptr->flags3 & (RF3_INSECT)) vp[vn++] ="insect";

	/* Describe "races" */
	if (vn)
	{

		/* Intro */
		if (vn > 1) text_out(" mix of");

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" ");
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");
			/* Dump */
			text_out(vp[n]);
		}

	}

	/* Hack -- If not a mix, describe class */
	if (vn < 2)
	{

		if (l_ptr->flags2 & (RF2_ARMOR)) text_out(" warrior"); /* Hack */

		if ((l_ptr->flags2 & (RF2_PRIEST)) && (l_ptr->flags2 & (RF2_MAGE))) text_out(" shaman");
		else if (l_ptr->flags2 & (RF2_PRIEST)) text_out(" priest");
		else if (l_ptr->flags2 & (RF2_MAGE)) text_out(" mage");
		else if (l_ptr->flags2 & (RF2_SNEAKY)) text_out(" thief");
		else if (l_ptr->flags2 & (RF2_ARMOR)) {} /* Hack */
		else if (l_ptr->flags2 & (RF2_ARCHER)) text_out(" archer"); /* Hack */
		else if ((!vn) && (strchr("pqt", r_ptr->d_char))) text_out(" person");
		else if (!vn) text_out(" creature");
	}

	/* Describe location */
	if (r_ptr->level == 0)
	{
		text_out(" lives in the town");
		old = TRUE;
	}
	else if (l_ptr->tkills)
	{
		if (l_ptr->flags1 & RF1_FORCE_DEPTH)
			text_out(" is found ");
		else
			text_out(" is normally found ");

		if (depth_in_feet)
		{
			text_out(format("at depths of %d feet",
			    r_ptr->level * 50));
		}
		else
		{
			text_out(format("on dungeon level %d",
			    r_ptr->level));
		}
		old = TRUE;
	}

	if (old) text_out(" and");

	text_out(" moves");

	/* Random-ness */
	if ((l_ptr->flags1 & RF1_RAND_50) || (l_ptr->flags1 & RF1_RAND_25))
	{
		/* Adverb */
		if ((l_ptr->flags1 & RF1_RAND_50) && (l_ptr->flags1 & RF1_RAND_25))
		{
			text_out(" extremely");
		}
		else if (l_ptr->flags1 & RF1_RAND_50)
		{
			text_out(" somewhat");
		}
		else if (l_ptr->flags1 & RF1_RAND_25)
		{
			text_out(" a bit");
		}

		/* Adjective */
		text_out(" erratically");

		/* Hack -- Occasional conjunction */
		if (r_ptr->speed != 110) text_out(" and");
	}

	/* Speed */
	if (r_ptr->speed > 110)
	{
		if (r_ptr->speed > 130) text_out( " incredibly");
		else if (r_ptr->speed > 120) text_out(" very");
		text_out( " quickly");
	}
	else if (r_ptr->speed < 110)
	{
		if (r_ptr->speed < 90) text_out(" incredibly");
		else if (r_ptr->speed < 100) text_out(" very");
		text_out(" slowly");
	}
	else
	{
		text_out(" at normal speed");
	}

	/* Does the monster ignore the player? */
	if (l_ptr->flags9 & RF9_TOWNSFOLK)
	{
		text_out(" about ");

		/* Describe location */
		if (r_ptr->level == 0)
		{
			text_out("town");
		}
		else
		{
			text_out("the dungeon");
		}
		text_out(" on business");
	}

	/* Collect improvements */
	vn = 0;

	/* Describe the improvements */
	if (l_ptr->flags9 & (RF9_LEVEL_SIZE)) vp[vn++] ="larger";
	if (l_ptr->flags9 & (RF9_LEVEL_SPEED)) vp[vn++] ="faster";
	if (l_ptr->flags9 & (RF9_LEVEL_AGE)) vp[vn++] ="older";
	if (l_ptr->flags9 & (RF9_LEVEL_POWER)) vp[vn++] ="more powerful";
	if (l_ptr->flags9 & (RF9_LEVEL_CLASS)) vp[vn++] ="specialised";

	/* Describe "improvements" */
	if (vn)
	{
		if (old) text_out(", but");

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) text_out(" is ");
			else if (n < vn-1) text_out(", ");
			else text_out(" or ");
			/* Dump */
			text_out(vp[n]);
		}

		text_out(" deeper in the dungeon");
	}

	/* The code above includes "attack speed" */
	if (l_ptr->flags1 & RF1_NEVER_MOVE)
	{
		if (vn) text_out(" and");
		else text_out(", but");

		text_out(" does not deign to chase intruders");
	}

	/* End this sentence */
	text_out(".  ");

#if 0

	if (flags1 & RF1_FORCE_SLEEP)
	{
		sprintf(buf, "%s is always created sluggish.  ", wd_che[msex]);
	}

#endif
}



/*
 * Learn everything about a monster (by cheating)
 */
static void cheat_monster_lore(const monster_race *r_ptr, monster_lore *l_ptr)
{
	int i;

	/* Hack -- Maximal kills */
	l_ptr->tkills = MAX_SHORT;
	l_ptr->tblows = MAX_SHORT;
	l_ptr->tdamage = MAX_SHORT;

	/* Hack -- Maximal info */
	l_ptr->wake = l_ptr->ignore = MAX_UCHAR;

	/* Observe "maximal" attacks */
	for (i = 0; i < 4; i++)
	{
		/* Examine "actual" blows */
		if (r_ptr->blow[i].effect || r_ptr->blow[i].method)
		{
			/* Hack -- maximal observations */
			l_ptr->blows[i] = MAX_UCHAR;
		}
	}

	/* Hack -- maximal drops */
	l_ptr->drop_gold = l_ptr->drop_item =
	(((r_ptr->flags1 & RF1_DROP_1D4) ? 4 : 0) +
	 ((r_ptr->flags1 & RF1_DROP_1D3) ? 3 : 0) +
	 ((r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0) +
	 ((r_ptr->flags1 & RF1_DROP_90)  ? 1 : 0) +
	 ((r_ptr->flags1 & RF1_DROP_60)  ? 1 : 0) +
	 ((r_ptr->flags1 & RF1_DROP_30)  ? 1 : 0));

	/* Hack -- but only "valid" drops */
	if (r_ptr->flags1 & RF1_ONLY_GOLD) l_ptr->drop_item = 0;
	if (r_ptr->flags1 & RF1_ONLY_ITEM) l_ptr->drop_gold = 0;

	/* Hack -- observe many spells */
	l_ptr->cast_innate = MAX_UCHAR;
	l_ptr->cast_spell = MAX_UCHAR;

	/* Hack -- know all the flags */
	l_ptr->flags1 = r_ptr->flags1;
	l_ptr->flags2 = r_ptr->flags2;
	l_ptr->flags3 = r_ptr->flags3;
	l_ptr->flags4 = r_ptr->flags4;
	l_ptr->flags5 = r_ptr->flags5;
	l_ptr->flags6 = r_ptr->flags6;
	l_ptr->flags7 = r_ptr->flags7;
	l_ptr->flags8 = r_ptr->flags8;
	l_ptr->flags9 = r_ptr->flags9;
}


/*
 * Hack -- display monster information using "roff()"
 *
 * Note that there is now a compiler option to only read the monster
 * descriptions from the raw file when they are actually needed, which
 * saves about 60K of memory at the cost of disk access during monster
 * recall, which is optional to the user.
 *
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
void describe_monster_race(const monster_race *r_ptr, const monster_lore *l_ptr, bool spoilers)
{
	monster_lore lore;

	/* Hack -- create a copy of the monster-memory */
	COPY(&lore, l_ptr, monster_lore);

	/* Assume some "obvious" flags */
	lore.flags1 |= (r_ptr->flags1 & RF1_OBVIOUS_MASK);

	/* Killing a monster reveals some properties */
	if (lore.tkills)
	{
		/* Know "race" flags */
		lore.flags3 |= (r_ptr->flags3 & RF3_RACE_MASK);

		/* Know "forced" flags */
		lore.flags1 |= (r_ptr->flags1 & (RF1_FORCE_DEPTH | RF1_FORCE_MAXHP));
	}

	/* Cheat -- know everything */
	if (cheat_know || spoilers) cheat_monster_lore(r_ptr, &lore);

	/* Show kills of monster vs. player(s) */
	if (!spoilers)
		describe_monster_kills(r_ptr, &lore);

	/* Monster description */
	describe_monster_desc(r_ptr);

	/* Describe the movement and level of the monster */
	describe_monster_movement(r_ptr, &lore);

	/* Describe experience */
	describe_monster_exp(r_ptr, &lore);

	/* Describe power */
	describe_monster_power(r_ptr, &lore);

	/* Describe the known ranged attacks */
	describe_monster_attack(r_ptr, &lore, TRUE);

	/* Describe spells and innate attacks */
	describe_monster_spells(r_ptr, &lore);

	/* Describe monster "toughness" */
	if (!spoilers) describe_monster_toughness(r_ptr, &lore);

	/* Describe the abilities of the monster */
	describe_monster_abilities(r_ptr, &lore);

	/* Describe the monster drop */
	describe_monster_drop(r_ptr, &lore);

	/* Describe the known attacks */
	describe_monster_attack(r_ptr, &lore, FALSE);

	/* Notice "Quest" monsters */
	if (lore.flags1 & (RF1_QUESTOR))
	{
		text_out("You feel an intense desire to kill this monster...  ");
	}

	/* Notice "Guardian" monsters */
	if (lore.flags1 & (RF1_GUARDIAN))
	{
		text_out("It is a dungeon guardian, impeding your progress further.  ");
	}

	/* All done */
	text_out("\n");
}


/*
 * Hack -- Display the "name" and "attr/chars" of a monster race
 */
static void roff_top(const int r_idx, int m_idx)
{
	byte a1, a2;
	char c1, c2;

	char desc[80];
	monster_race *r_ptr = &r_info[r_idx];

	/* Describe the monster */
	if (m_idx)
	{
		/* Describe the monster */
		monster_desc(desc, sizeof(desc), m_idx, 0x80);

		/* Capitalise the first letter */
		if (islower(desc[0])) desc[0] = toupper(desc[0]);
	}
	/* Not describing a specific monster, so hack a description */
	else
	{
		/* Get the name */
		race_desc(desc, sizeof(desc), r_idx, 0x400, 1);
		
		/* Capitalise the first letter */
		if (islower(desc[0])) desc[0] = toupper(desc[0]);
	}

	/* Get the chars */
	c1 = r_ptr->d_char;
	c2 = r_ptr->x_char;

	/* Get the attrs */
	a1 = r_ptr->d_attr;
	a2 = r_ptr->x_attr;


	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, desc);

	if (!use_dbltile && !use_trptile)
	{
		/* Append the "standard" attr/char info */
		Term_addstr(-1, TERM_WHITE, " ('");
		Term_addch(a1, c1);
		Term_addstr(-1, TERM_WHITE, "')");

		/* Append the "optional" attr/char info */
		Term_addstr(-1, TERM_WHITE, "/('");
		Term_addch(a2, c2);
		if (use_bigtile && (a2 & 0x80)) Term_addch(255, -1);
		Term_addstr(-1, TERM_WHITE, "'):");
	}
}



/*
 * Hack -- describe the given monster race at the top of the screen
 */
void screen_roff(const int r_idx, const monster_lore *l_ptr)
{
	monster_race *r_ptr = &r_info[r_idx];
	
	/* Flush messages */
	message_flush();

	/* Begin recall */
	Term_erase(0, 1, 255);

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Recall monster */
	describe_monster_race(r_ptr, l_ptr, FALSE);

	/* Describe monster */
	roff_top(r_idx, 0);
}




/*
 * Hack -- describe the given monster race in the current "term" window
 */
void display_roff(const int r_idx, const monster_lore *l_ptr)
{
	monster_race *r_ptr = &r_info[r_idx];
	int y;

	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* Begin recall */
	Term_gotoxy(0, 1);

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Recall monster */
	describe_monster_race(r_ptr, l_ptr, FALSE);

	/* Describe monster */
	roff_top(r_idx, 0);
}


/*
 * Hack -- describe the given monster race at the top of the screen
 */
void screen_monster_look(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race race;

	/* Scale monster */
	if (((r_ptr->flags9 & RF9_LEVEL_MASK) == 0)
			&& monster_scale(&race, m_idx, p_ptr->depth))
	{
		/* Scaled the monster successfully */
	}
	/* Make copy in order to genderise monster, if required */
	else
	{
		COPY(&race, r_ptr, monster_race);
	}

	/* Remove genders */
	if ((race.flags1 & (RF1_FEMALE)) && (m_idx % 2)) race.flags1 &= ~(RF1_MALE);
	else if (race.flags1 & (RF1_MALE)) race.flags1 &= ~(RF1_FEMALE);

	/* Flush messages */
	message_flush();

	/* Begin recall */
	Term_erase(0, 1, 255);

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Recall monster */
	describe_monster_race(&race, &l_list[m_ptr->r_idx], FALSE);

	/* Describe monster */
	roff_top(m_ptr->r_idx, m_idx);
}


/*
 * Given a starting position, find the 'n'th closest monster.
 *
 * The follow parameters are possible.
 *
 * 0x01 Require LOS
 * 0x02	Require LOF
 * 0x04 Require visibility
 * 0x08 If no monsters found, fall back to choosing an empty square
 * near the source grid.
 *
 *
 * Set ty and tx to zero on failure.
 */
void get_closest_monster(int n, int y0, int x0, int *ty, int *tx, byte parameters, int who)
{
	monster_type *m_ptr;

	int i, j;
	int m_idx;
	int dist = 100;

	int *monster_dist;
	int *monster_index;
	int monster_count = 0;

	bool player = FALSE;

	/* Allocate some arrays */
	monster_dist = C_ZNEW(z_info->m_max + 1, int);
	monster_index = C_ZNEW(z_info->m_max + 1, int);

	/* Note that we're looking from the character's grid */
	if ((y0 == p_ptr->py) && (x0 == p_ptr->px)) player = TRUE;

	/* Reset target grids */
	*ty = 0;  *tx = 0;

	/* N, as input, goes from 1+.  Map it to 0+ for table access */
	if (n > 0) n--;


	/* Check all the monsters */
	for (i = 1; i < m_max; i++)
	{
		/* Get the monster */
		m_ptr = &m_list[i];

		/* Paranoia -- skip "dead" monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip the caster */
		if (who == i) continue;

		/* Skip allies */
		if ((who > SOURCE_MONSTER_START) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0)) continue;

		/* Skip player allies */
		if ((who <= SOURCE_PLAYER_START) && ((m_ptr->mflag & (MFLAG_ALLY)) != 0)) continue;

		/* Check for visibility */
		if (parameters & (0x04))
		{
			if ((player) && (!m_ptr->ml)) continue;
		}

		/* Use CAVE_VIEW information (fast way) */
		if (player)
		{
			/* Requires line of sight */
			if (((parameters & (0x01)) != 0) &&
					(!(play_info[m_ptr->fy][m_ptr->fx] & (PLAY_VIEW)))) continue;

			/* Requires line of fire */
			if (((parameters & (0x02)) != 0) &&
					(!(play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE)))) continue;

			/* Get stored distance */
			dist = m_ptr->cdis;
		}

		/* Monster must be in los from the starting position (slower way) */
		else
		{
			/* Get distance from starting position */
			dist = distance(y0, x0, m_ptr->fy, m_ptr->fx);

			/* Monster location must be within range */
			if (dist > MAX_SIGHT) continue;

			/* Require line of sight */
			if (((parameters & (0x01)) != 0) && (!generic_los(y0, x0, m_ptr->fy, m_ptr->fx, CAVE_XLOS))) continue;

			/* Require line of fire */
			if (((parameters & (0x02)) != 0) && (!generic_los(y0, x0, m_ptr->fy, m_ptr->fx, CAVE_XLOF))) continue;
		}

		/* Remember this monster */
		monster_dist[monster_count] = dist;
		monster_index[monster_count++] = i;
	}

	/* Check the player? */
	if (who > SOURCE_PLAYER_START)
	{
		monster_dist[monster_count] = distance(y0, x0, p_ptr->py, p_ptr->px);
		monster_index[monster_count++] = -1;
	}

	/* Not enough monsters found */
	if (monster_count <= n)
	{
		/* Free some arrays */
		FREE(monster_dist);
		FREE(monster_index);

		/* Pick a nearby grid instead */
		if (parameters & (0x08)) scatter(ty, tx, y0, x0, n,
				(parameters & (0x01) ? CAVE_XLOS : 0) | (parameters & (0x02) ? CAVE_XLOF : 0));

		return;
	}

	/* Sort the monsters in ascending order of distance */
	for (i = 0; i < monster_count - 1; i++)
	{
		for (j = 0; j < monster_count - 1; j++)
		{
			int this_dist = monster_dist[j];
			int next_dist = monster_dist[j + 1];

			/* Bubble sort */
			if (this_dist > next_dist)
			{
				int tmp_dist  = monster_dist[j];
				int tmp_index = monster_index[j];

				monster_dist[j] = monster_dist[j + 1];
				monster_dist[j + 1] = tmp_dist;

				monster_index[j] = monster_index[j + 1];
				monster_index[j + 1] = tmp_index;
			}
		}
	}


	/* Get the nth closest monster's index */
	m_idx = monster_index[n];

	/* Going for the player */
	if (m_idx < 0)
	{
		/* Set the target to player's location */
		*ty = p_ptr->py;
		*tx = p_ptr->px;
	}
	else
	{
		/* Get the monster */
		m_ptr = &m_list[m_idx];

		/* Set the target to its location */
		*ty = m_ptr->fy;
		*tx = m_ptr->fx;
	}

	/* Free some arrays */
	FREE(monster_dist);
	FREE(monster_index);
}
