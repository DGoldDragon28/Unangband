/* File: object1.c */

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
 * Max sizes of the following arrays.
 */
#define MAX_TITLES     85       /* Used with scrolls (min 76) */
#define MAX_SYLLABLES 158       /* Used with scrolls (see below) */


/*
 * Syllables for scrolls (must be 1-4 letters each).
 */

static cptr syllables[MAX_SYLLABLES] =
{
	"a", "ab", "ag", "aks", "ala", "an", "ankh", "app",
	"arg", "arze", "ash", "aus", "ban", "bar", "bat", "bek",
	"bie", "bin", "bit", "bjor", "blu", "bot", "bu",
	"byt", "comp", "con", "cos", "cre", "dalf", "dan",
	"den", "der", "doe", "dok", "eep", "el", "eng", "er", "ere", "erk",
	"esh", "evs", "fa", "fid", "flit", "for", "fri", "fu", "gan",
	"gar", "glen", "gop", "gre", "ha", "he", "hyd", "i",
	"ing", "ion", "ip", "ish", "it", "ite", "iv", "jo",
	"kho", "kli", "klis", "la", "lech", "man", "mar",
	"me", "mi", "mic", "mik", "mon", "mung", "mur", "nag", "nej",
	"nelg", "nep", "ner", "nes", "nis", "nih", "nin", "o",
	"od", "ood", "org", "orn", "ox", "oxy", "pay", "pet",
	"ple", "plu", "po", "pot", "prok", "re", "rea", "rhov",
	"ri", "ro", "rog", "rok", "rol", "sa", "san", "sat",
	"see", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
	"sno", "so", "sol", "sri", "sta", "sun", "ta", "tab",
	"tem", "ther", "ti", "tox", "trol", "tue", "turs", "u",
	"ulk", "um", "un", "uni", "ur", "val", "viv", "vly",
	"vom", "wah", "wed", "werg", "wex", "whon", "wun", "x",
	"yerg", "yp", "zun", "tri", "blaa"
};


/*
 * Hold the titles of scrolls, 6 to 14 characters each.
 *
 * Also keep an array of scroll colors (always WHITE for now).
 */

static char scroll_adj[MAX_TITLES][16];


static void flavor_assign_fixed(void)
{
	int i, j;

	for (i = 0; i < z_info->x_max; i++)
	{
		flavor_type *flavor_ptr = &x_info[i];

		/* Skip random flavors */
		if (flavor_ptr->sval == SV_UNKNOWN) continue;

		for (j = 0; j < z_info->k_max; j++)
		{
			/* Skip other objects */
			if ((k_info[j].tval == flavor_ptr->tval) &&
			    (k_info[j].sval == flavor_ptr->sval))
			{
				/* Store the flavor index */
				k_info[j].flavor = i;
			}
		}
	}
}


static void flavor_assign_random(byte tval)
{
	int i, j;
	int flavor_count = 0;
	int choice;

	/* Count the random flavors for the given tval */
	for (i = 0; i < z_info->x_max; i++)
	{
		if ((x_info[i].tval == tval) &&
		    (x_info[i].sval == SV_UNKNOWN))
		{
			flavor_count++;
		}
	}

	for (i = 0; i < z_info->k_max; i++)
	{
		/* Skip other object types */
		if (k_info[i].tval != tval) continue;

		/* Skip objects that already are flavored */
		if (k_info[i].flavor != 0) continue;

		/* Skip objects which don't get a flavor */
		if ((k_info[i].flags6 & (TR6_FLAVOR)) ==  0) continue;

		if (!flavor_count) quit_fmt("Not enough flavors for tval %d (at object %d).", tval, i);

		/* Select a flavor */
		choice = rand_int(flavor_count);

		/* Find and store the flavor */
		for (j = 0; j < z_info->x_max; j++)
		{
			/* Skip other tvals */
			if (x_info[j].tval != tval) continue;

			/* Skip assigned svals */
			if (x_info[j].sval != SV_UNKNOWN) continue;

			if (choice == 0)
			{
				/* Store the flavor index */
				k_info[i].flavor = j;

				/* Mark the flavor as used */
				x_info[j].sval = k_info[i].sval;

				/* One less flavor to choose from */
				flavor_count--;

				break;
			}

			choice--;
		}
	}
}


/*
 * Prepare the "variable" part of the "k_info" array.
 *
 * The "color"/"metal"/"type" of an item is its "flavor".
 * For the most part, flavors are assigned randomly each game.
 *
 * Initialize descriptions for the "colored" objects, including:
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.
 *
 * The first 4 entries for potions are fixed (Water, Apple Juice,
 * Slime Mold Juice, Unused Potion).
 *
 * Scroll titles are always between 6 and 14 letters long.  This is
 * ensured because every title is composed of whole words, where every
 * word is from 1 to 8 letters long (one or two syllables of 1 to 4
 * letters each), and that no scroll is finished until it attempts to
 * grow beyond 15 letters.  The first time this can happen is when the
 * current title has 6 letters and the new word has 8 letters, which
 * would result in a 6 letter scroll title.
 *
 * Duplicate titles are avoided by requiring that no two scrolls share
 * the same first four letters (not the most efficient method, and not
 * the least efficient method, but it will always work).
 *
 * Hack -- make sure everything stays the same for each saved game
 * This is accomplished by the use of a saved "random seed", as in
 * "town_gen()".  Since no other functions are called while the special
 * seed is in effect, so this function is pretty "safe".
 */
void flavor_init(void)
{
	int i, j;


	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant flavors */
	Rand_value = seed_flavor;


	flavor_assign_fixed();

	flavor_assign_random(TV_RING);
	flavor_assign_random(TV_AMULET);
	flavor_assign_random(TV_STAFF);
	flavor_assign_random(TV_WAND);
	flavor_assign_random(TV_ROD);
	flavor_assign_random(TV_MUSHROOM);
	flavor_assign_random(TV_POTION);
	flavor_assign_random(TV_SCROLL);

	/* Scrolls (random titles, always white) */
	for (i = 0; i < MAX_TITLES; i++)
	{
		/* Get a new title */
		while (TRUE)
		{
			char buf[80];

			bool okay;

			/* Start a new title */
			buf[0] = '\0';

			/* Collect words until done */
			while (1)
			{
				int q, s;

				char tmp[80];

				/* Start a new word */
				tmp[0] = '\0';

				/* Choose one or two syllables */
				s = ((rand_int(100) < 30) ? 1 : 2);

				/* Add a one or two syllable word */
				for (q = 0; q < s; q++)
				{
					/* Add the syllable */
					my_strcat(tmp, syllables[rand_int(MAX_SYLLABLES)], sizeof(tmp));
				}

				/* Stop before getting too long */
				if (strlen(buf) + 1 + strlen(tmp) > 15) break;

				/* Add a space */
				my_strcat(buf, " ", sizeof(buf));

				/* Add the word */
				my_strcat(buf, tmp, sizeof(buf));
			}

			/* Save the title */
			my_strcpy(scroll_adj[i], buf+1, sizeof(scroll_adj[0]));

			/* Assume okay */
			okay = TRUE;

			/* Check for "duplicate" scroll titles */
			for (j = 0; j < i; j++)
			{
				cptr hack1 = scroll_adj[j];
				cptr hack2 = scroll_adj[i];

				/* Compare first four characters */
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;
				if (*hack1++ != *hack2++) continue;

				/* Not okay */
				okay = FALSE;

				/* Stop looking */
				break;
			}

			/* Break when done */
			if (okay) break;
		}
	}


	/* Hack -- Use the "complex" RNG */
	Rand_quick = FALSE;

}

#ifdef ALLOW_BORG_GRAPHICS
#if 0
extern void init_translate_visuals(void);
#endif
#endif /* ALLOW_BORG_GRAPHICS */


/*
 * Reset the "visual" lists
 *
 * This involves resetting various things to their "default" state.
 *
 * If the "prefs" flag is TRUE, then we will also load the appropriate
 * "user pref file" based on the current setting of the "use_graphics"
 * flag.  This is useful for switching "graphics" on/off.
 *
 * The features, objects, and monsters, should all be encoded in the
 * relevant "font.pref" and/or "graf.prf" files.  XXX XXX XXX
 *
 * The "prefs" parameter is no longer meaningful.  XXX XXX XXX
 */
void reset_visuals(bool unused)
{
	int i;

	/* Prevent compilation warning */
	(void)unused;

	/* Extract default attr/char code for features */
	for (i = 0; i < z_info->f_max; i++)
	{
		feature_type *f_ptr = &f_info[i];

		/* Assume we will use the underlying values */
		f_ptr->x_attr = f_ptr->d_attr;
		f_ptr->x_char = f_ptr->d_char;

		/* Reset attr lite */
		f_ptr->flags3 |= FF3_ATTR_LITE;
	}

	/* Extract default attr/char code for objects */
	for (i = 0; i < z_info->k_max; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Default attr/char */
		k_ptr->x_attr = k_ptr->d_attr;
		k_ptr->x_char = k_ptr->d_char;
	}

	/* Extract default attr/char code for flavors */
	for (i = 0; i < z_info->x_max; i++)
	{
		flavor_type *x_ptr = &x_info[i];

		/* Default attr/char */
		x_ptr->x_attr = x_ptr->d_attr;
		x_ptr->x_char = x_ptr->d_char;
	}


	/* Extract default attr/char code for monsters */
	for (i = 0; i < z_info->r_max; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Default attr/char */
		r_ptr->x_attr = r_ptr->d_attr;
		r_ptr->x_char = r_ptr->d_char;
	}


	/* Extract default attr/char code for regions */
	for (i = 0; i < z_info->region_info_max; i++)
	{
		region_info_type *r_ptr = &region_info[i];

		/* Default attr/char */
		r_ptr->x_attr = r_ptr->d_attr;
		r_ptr->x_char = r_ptr->d_char;
	}

	/* Extract attr/chars for inventory objects (by tval) */
	for (i = 0; i < 128; i++)
	{
		/* Default to white */
		tval_to_attr[i] = TERM_WHITE;
	}


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

#ifdef ALLOW_BORG_GRAPHICS
#if 0
	/* Initialize the translation table for the borg */
	init_translate_visuals();
#endif
#endif /* ALLOW_BORG_GRAPHICS */
}

/*
 * Efficient version of '(T) += strfmt((T), "%c", (C))'
 */
#define object_desc_chr_macro(T,C) do { \
 \
	/* Copy the char */ \
	*(T)++ = (C); \
 \
} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%s", (S))'
 */
#define object_desc_str_macro(T,S) do { \
 \
	cptr s = (S); \
 \
	/* Copy the string */ \
	while (*s) *(T)++ = *s++; \
 \
} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%u", (N))'
 */
#define object_desc_num_macro(T,N) do { \
 \
	uint n = (N); \
 \
	uint p; \
 \
	/* Find "size" of "n" */ \
	for (p = 1; n >= p * 10 && p < 1000000000; p = p * 10) /* loop */; \
 \
	/* Dump each digit */ \
	while (p >= 1) \
	{ \
		/* Dump the digit */ \
		*(T)++ = I2D(n / p); \
 \
		/* Remove the digit */ \
		n = n % p; \
 \
		/* Process next digit */ \
		p = p / 10; \
	} \
 \
} while (0)



/*
 * Efficient version of '(T) += strfmt((T), "%+d", (I))'
 */
#define object_desc_int_macro(T,I) do { \
 \
	sint i = (I); \
 \
	/* Negative */ \
	if (i < 0) \
	{ \
		/* Take the absolute value */ \
		i = 0 - i; \
 \
		/* Use a "minus" sign */ \
		*(T)++ = '-'; \
	} \
 \
	/* Positive (or zero) */ \
	else \
	{ \
		/* Use a "plus" sign */ \
		*(T)++ = '+'; \
	} \
 \
	/* Dump the number itself */ \
	object_desc_num_macro(T, i); \
 \
} while (0)

/*
 * Creates a description of the item "o_ptr", and stores it in "out_val".
 *
 * One can choose the "verbosity" of the description, including whether
 * or not the "number" of items should be described, and how much detail
 * should be used when describing the item.
 *
 * The given "buf" must be 80 chars long to hold the longest possible
 * description, which can get pretty long, including incriptions, such as:
 * "no more Maces of Disruption (Defender) (+10,+10) [+5] (+3 to stealth)".
 * Note that the inscription will be clipped to keep the total description
 * under 79 chars (plus a terminator).
 *
 * This function uses a big temporary array to create the description,
 * and then copies up to 79 characters from this array into the buffer,
 * which will prevent crashes (but not ugliness) if any object name uses
 * more than 79 characters.
 *
 * Note the use of "object_desc_int_macro()" and "object_desc_num_macro()"
 * and "object_desc_str_macro()" and "object_desc_chr_macro()" as extremely
 * efficient, portable, versions of some common "strfmt()" commands (without
 * the bounds checking or termination writing), which allow a pointer to
 * efficiently move through a buffer while modifying it in various ways.
 *
 * Various improper uses and/or placements of "&" or "~" characters can
 * easily induce out-of-bounds memory accesses.  Some of these could be
 * easily checked for, if efficiency was not a concern.
 *
 * Note that all ego-items (when known) append an "Ego-Item Name", unless
 * the item is also an artifact, which should never happen.
 *
 * Note that all artifacts (when known) append an "Artifact Name", so we
 * have special processing for "Specials" (artifact Lites, Rings, Amulets).
 * The "Specials" never use "modifiers" if they are "known", since they
 * have special "descriptions", such as "The Necklace of the Dwarves".
 *
 * Special Lite's use the "k_info" base-name (Phial, Star, or Arkenstone),
 * plus the artifact name, just like any other artifact, if known.
 *
 * Special Ring's and Amulet's, if not "aware", use the same code as normal
 * rings and amulets, and if "aware", use the "k_info" base-name (Ring or
 * Amulet or Necklace).  They will NEVER "append" the "k_info" name.  But,
 * they will append the artifact name, just like any artifact, if known.
 *
 * Hack -- Display "The One Ring" as "a Plain Gold Ring" until aware.
 *
 * The "pluralization" rules are extremely hackish, in fact, for efficiency,
 * we only handle things like "torch"/"torches" and "cutlass"/"cutlasses",
 * and we would not handle "box"/"boxes", or "knife"/"knives", correctly.
 * Of course, it would be easy to add rules for these forms.
 *
 * If "pref" is true then a "numeric" prefix will be pre-pended, else is is
 * assumed that a string such as "The" or "Your" will be pre-pended later.
 *
 * Modes ("pref" is TRUE):
 *   0 -- The Chain Mail of Death
 *   1 -- A Cloak of Death [1,+3]
 *   2 -- An Amulet of Death [1,+3] (+2 to Stealth)
 *   3 -- 5 Rings of Death [1,+3] (+2 to Stealth) {nifty}
 *
 * Modes ("pref" is FALSE):
 *   0 -- Chain Mail of Death
 *   1 -- Cloak of Death [1,+3]
 *   2 -- Amulet of Death [1,+3] (+2 to Stealth)
 *   3 -- Rings of Death [1,+3] (+2 to Stealth) {nifty}
 */
void object_desc(char *buf, size_t max, const object_type *o_ptr, int pref, int mode)
{
	const char *basenm;
	const char *modstr;

	int power;

	bool aware;
	bool named;
	bool bonus;
	bool charges;
	bool pval;
	bool known;

	int flavor;

	bool append_name;
	bool append_modstr;

	bool show_weapon;
	bool show_armour;

	char *b;

	char *t;

	cptr s;

	cptr u;
 	cptr v;

	char p1 = '(', p2 = ')';
	char b1 = '[', b2 = ']';
	char c1 = '{', c2 = '}';

	char discount_buf[80];

	char tmp_buf[128];
	char mon_buf[80];

	u32b f1, f2, f3, f4;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	/* Prevent compiler warning */
	(void)max;

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);


	/* See if the object is "aware" */
	aware = (object_aware_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "aware" */
	named = (object_named_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "aware" */
	bonus = (object_bonus_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "aware" */
	charges = (object_charges_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "aware" */
	pval = (object_pval_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "known" */
	known = (object_known_p(o_ptr) ? TRUE : FALSE);

	/* See if the object is "flavored" */
	flavor = k_ptr->flavor;

	/* Allow flavors to be hidden when aware */
	if (aware && !show_flavors) flavor = FALSE;

	/* Object is in the inventory of a store */
	if (o_ptr->ident & IDENT_STORE)
	{
		/* Don't show flavors */
		flavor = FALSE;

		/* Pretend known and aware */
		aware = TRUE;
		named = TRUE;
		bonus = TRUE;
		charges = TRUE;
		known = TRUE;
	}

	/* Assume no name appending */
	append_name = FALSE;

	/* Assume no modstr appending */
	append_modstr = FALSE;

	/* Assume no need to show "weapon" bonuses */
	show_weapon = (k_ptr->flags5 & (TR5_SHOW_WEAPON)) != 0;

	/* Assume no need to show "armour" bonuses */
	show_armour = (k_ptr->flags5 & (TR5_SHOW_AC)) != 0;

	/* Extract default "base" string */
	basenm = (k_name + k_ptr->name);

	/* Assume no "modifier" string */
	modstr = "";

	/* Describe 'simple' objects and return */
	if (k_ptr->flags6 & (TR6_SIMPLE))
	{
		my_strcpy(buf, basenm, max);
		return;
	}

	/* Get the modstr */
	if (artifact_p(o_ptr) && aware)
	{
		/* Do nothing */
	}
	/* Modify the name in various ways */
	else
	{
		bool show_flavor = flavor || (k_ptr->flags6 & (TR6_FORCE_MOD));

		/* Hack for study materials */
		if (k_ptr->flags6 & (TR6_RESEARCH))
		{
			if (o_ptr->pval >= 0) modstr = s_name + s_info[o_ptr->pval].name;
			else modstr = s_name + s_info[0].name;
		}
		else if (k_ptr->flags6 & (TR6_TITLE)) modstr = scroll_adj[o_ptr->sval];
		else if (k_ptr->flags6 & (TR6_FLAVOR)) modstr = x_text + x_info[flavor].text;
		else if (k_ptr->flags6 & (TR6_MOD_NAME)) modstr = basenm;

		/* Add the name if required */
		if ((aware || !flavor) && (k_ptr->flags6 & (TR6_ADD_NAME))) append_name = TRUE;

		/* Prepend or append the base name */
		if (k_ptr->flags6 & (TR6_NO_TVAL))
		{
			basenm = "& #~";
		}
		else if (k_ptr->flags6 & (TR6_PREPEND))
		{
			basenm = (show_flavor ? "& # @~" : "& @~");
		}
		/* Append the base name */
		else if (k_ptr->flags6 & (TR6_MOD_NAME))
		{
			basenm = (show_flavor ? "& @~ #" : "& @~");
		}
	}

	/* Prep the monster name if required */
	if (k_ptr->flags6 & (TR6_NAMED))
	{
		/* Describe monster */
		if (o_ptr->name3 > 0)
		{
			/* Get the name */
			race_desc(mon_buf, sizeof(mon_buf), o_ptr->name3, 0x800, 1);
		}

		/* Describe player */
		else if (o_ptr->name3 < 0)
		{
			/* Save the monster name */
			my_strcpy(mon_buf, "your", sizeof(mon_buf));
		}

		/* This particular object isn't named */
		if (!o_ptr->name3)
		{
			switch (o_ptr->tval)
			{
				case TV_STATUE:
					modstr = "an ancient god";
					break;
				case TV_ASSEMBLY:
					modstr = "mechanism";
					break;
				case TV_SKIN:
					modstr = "dusty";
					break;
				case TV_BODY:
					modstr = "mummified";
					break;
				case TV_FLASK:
					modstr = "spoiled";
					break;
				case TV_HOLD:
					modstr = "empty";
					break;
				case TV_EGG:
					if (o_ptr->sval == SV_EGG_SPORE)
					{
						modstr = "dried";
						break;
					}
					/* Drop down */
				default:
					modstr = "broken";
					break;
			}
		}
		else if (o_ptr->tval == TV_HOLD)
		{
			/* Note the container is closed */
			modstr = "sealed";
		}
		else if (r_info[o_ptr->name3].flags1 & (RF1_UNIQUE))
		{
			if (o_ptr->tval != TV_STATUE) my_strcat(mon_buf, "'s", sizeof(mon_buf));

			/* Use the mod string */
			modstr = mon_buf;

			/* Skip a/an */
			if (basenm[2] == '#') basenm = &basenm[2];
		}
		else
		{
			if (o_ptr->tval == TV_STATUE)
			{
				my_strcpy(mon_buf, format("%s %s", is_a_vowel(mon_buf[0]) ? "an" : "a", mon_buf), sizeof(mon_buf));
			}
			else if (o_ptr->tval == TV_FLASK)
			{
				append_modstr = TRUE;
			}

			/* Set the mod string */
			modstr = mon_buf;
		}
	}

	/* Start dumping the result */
	t = b = tmp_buf;

	/* Begin */
	s = basenm;

	/* Handle objects which sometimes use "a" or "an" */
	if (*s == '&')
	{
		/* Paranoia XXX XXX XXX */
		/* ASSERT(s[1] == ' '); */

		/* Skip the ampersand and the following space */
		s += 2;

		/* No prefix */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- None left */
		else if (o_ptr->number <= 0)
		{
			object_desc_str_macro(t, "no more ");
		}

		/* Hack -- rope */
		else if (k_ptr->flags6 & (TR6_IN_FEET))
		{
			object_desc_num_macro(t, o_ptr->number);
			object_desc_str_macro(t, "0 feet of ");
		}

		/* Extract the number */
		else if (o_ptr->number > 1)
		{
			object_desc_num_macro(t, o_ptr->number);
			object_desc_chr_macro(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if (named && artifact_p(o_ptr))
		{
			object_desc_str_macro(t, "The ");
		}

		/* Hack -- belongs to the player */
		else if ((k_ptr->flags6 & (TR6_NAMED)) && (o_ptr->name3 < 0))
		{
			/* Nothing */
		}

		/* Hack -- A single one, and next character will be a vowel */
		else if ((*s == '#') ? is_a_vowel(modstr[0]) : is_a_vowel(*s))
		{
			object_desc_str_macro(t, "an ");
		}

		/* A single one, and next character will be a non-vowel */
		else
		{
			object_desc_str_macro(t, "a ");
		}
	}

	/* Handle objects which never use "a" or "an" */
	else
	{
		/* No pref */
		if (!pref)
		{
			/* Nothing */
		}

		/* Hack -- all gone */
		else if (o_ptr->number <= 0)
		{
			object_desc_str_macro(t, "no more ");
		}

		/* Prefix a number if required */
		else if (o_ptr->number > 1)
		{
			object_desc_num_macro(t, o_ptr->number);
			object_desc_chr_macro(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if (named && artifact_p(o_ptr))
		{
			object_desc_str_macro(t, "The ");
		}

		/* Hack -- A single item, so no prefix needed */
		else
		{
			/* Nothing */
		}
	}

	/* Hack -- display debug tval/sval info in cheat_peek mode */
	if (cheat_peek)
	{
		object_desc_str_macro(t, "(t");
		object_desc_num_macro(t,o_ptr->tval);
		object_desc_str_macro(t, ":s");
		object_desc_num_macro(t,o_ptr->sval);
		object_desc_str_macro(t, ") ");
	}

	/* Hack -- display debug stack info in cheat_peek mode */
	if ((cheat_peek) && (o_ptr->stackc))
	{
		object_desc_str_macro(t, "(stack ");
		object_desc_num_macro(t,o_ptr->stackc);
		object_desc_str_macro(t, ") ");
	}


	/* Hack -- display debug show_idx info in cheat_peek mode */
	if ((cheat_peek) && (o_ptr->show_idx))
	{
		object_desc_str_macro(t, "(show_idx ");
		object_desc_num_macro(t,o_ptr->show_idx);
		object_desc_str_macro(t, ") ");
	}

	/* Hack -- display debug value in cheat_peek mode */
	if (cheat_peek)
	{
		object_desc_str_macro(t, "(worth ");
		object_desc_num_macro(t,object_value(o_ptr));
		object_desc_str_macro(t, ") ");
	}

	/* Hack -- display id details */
	if (cheat_peek)
	{
		object_desc_str_macro(t, "(");
		if (o_ptr->ident & (IDENT_SENSE)) object_desc_chr_macro(t,'s');
		if (o_ptr->ident & (IDENT_FIXED)) object_desc_chr_macro(t,'x');
		if (o_ptr->ident & (IDENT_BONUS)) object_desc_chr_macro(t,'b');
		if (o_ptr->ident & (IDENT_KNOWN)) object_desc_chr_macro(t,'k');
		if (o_ptr->ident & (IDENT_STORE)) object_desc_chr_macro(t,'u');
		if (o_ptr->ident & (IDENT_MENTAL)) object_desc_chr_macro(t,'f');
		if (o_ptr->ident & (IDENT_CURSED)) object_desc_chr_macro(t,'c');
		if (o_ptr->ident & (IDENT_BROKEN)) object_desc_chr_macro(t,'w');
		if (o_ptr->ident & (IDENT_BREAKS)) object_desc_chr_macro(t,'d');
		if (o_ptr->ident & (IDENT_CHARGES)) object_desc_chr_macro(t,'s');
		if (o_ptr->ident & (IDENT_VALUE)) object_desc_chr_macro(t,'v');
		if (o_ptr->ident & (IDENT_RUNES)) object_desc_chr_macro(t,'r');
		if (o_ptr->ident & (IDENT_NAME)) object_desc_chr_macro(t,'n');
		if (o_ptr->ident & (IDENT_PVAL)) object_desc_chr_macro(t,'p');
		if (o_ptr->ident & (IDENT_MARKED)) object_desc_chr_macro(t,'m');
		if (o_ptr->ident & (IDENT_FORGED)) object_desc_chr_macro(t,'g');
		object_desc_str_macro(t, ") ");
	}

	/* Hack -- display debug xtra info in cheat_peek mode */
	if ((cheat_peek) && (o_ptr->xtra1))
	{
		object_desc_str_macro(t, "(xtra ");
		object_desc_num_macro(t,o_ptr->xtra1);
		object_desc_str_macro(t, ":");
		object_desc_num_macro(t,o_ptr->xtra2);
		object_desc_str_macro(t, ") ");
	}
	/* Paranoia XXX XXX XXX */
	/* ASSERT(*s != '~'); */

	/* Copy the string */
	for (; *s; s++)
	{
		/* Pluralizer */
		if (*s == '~')
		{
			/* Don't display if in feet */
			if (k_ptr->flags6 & (TR6_IN_FEET)) continue;

			/* Add a plural if needed */
			if (o_ptr->number != 1)
			{
				char k = t[-1];

				/* Hack -- "Cutlass-es" and "Torch-es" */
				if ((k == 's') || (k == 'h')) *t++ = 'e';

				/* Add an 's' */
				*t++ = 's';
			}
		}

		/* Modifier */
		else if (*s == '#')
		{
			/* Append title text */
			if (k_ptr->flags6 & (TR6_TITLE)) object_desc_str_macro(t, "titled \"");

			/* Append the modifier */
			object_desc_str_macro(t, modstr);

			/* Append title terminator */
			if (k_ptr->flags6 & (TR6_TITLE)) object_desc_str_macro(t, "\"");
		}

		/* Base */
		else if (*s == '@')
		{
			int i;

			/* Find base text */
			for (i = 0; object_group[i].text; i++)
			{
				if (object_group[i].tval == k_ptr->tval) break;
			}

			/* Append base text */
			if (object_group[i].text)
			{
				/* Append the modifier */
				object_desc_str_macro(t, object_group[i].text);
			}
		}

		/* Normal */
		else
		{
			/* Copy */
			*t++ = *s;
		}
	}


	/* Append the "kind name" and/or "mod string" to the "base name" */
	if ((append_name) || (append_modstr))
	{
		object_desc_str_macro(t, " of ");
		if (append_modstr) object_desc_str_macro(t, modstr);
		if ((append_modstr) && (append_name)) object_desc_str_macro(t, " ");
		if (append_name) object_desc_str_macro(t, (k_name + k_ptr->name));
	}


	/* Hack -- Append "Artifact" or "Special" names */
	if (named)
	{
		/* Grab any artifact name */
		if (o_ptr->name1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			object_desc_chr_macro(t, ' ');
			object_desc_str_macro(t, (a_name + a_ptr->name));
		}

		/* Grab any ego-item name */
		else if (o_ptr->name2)
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];

			object_desc_chr_macro(t, ' ');
			object_desc_str_macro(t, (e_name + e_ptr->name));
		}

		/* Grab any magic-item name */
		else if ((o_ptr->xtra1) && (o_ptr->xtra1 < OBJECT_XTRA_MIN_RUNES)/* && (o_ptr->feeling < INSCRIP_MIN_HIDDEN)*/)
		{
			int i;
			u32b j;

			for (i = 0, j = 0x00000001L; (i< 32) && (j != object_xtra_base[o_ptr->xtra1]);i++, j <<= 1);

			object_desc_chr_macro(t, ' ');
			object_desc_str_macro(t, magic_name[object_xtra_what[o_ptr->xtra1]-1][i + o_ptr->xtra2]);
		}
	}
	/* Hack -- Append guessed names */
	else
	{
		/* Grab any artifact name */
		if (o_ptr->guess1)
		{
			artifact_type *a_ptr = &a_info[o_ptr->guess1];

			object_desc_str_macro(t, " (");
			if (*(a_name + a_ptr->name) == '(')
			{
				object_desc_str_macro(t, (a_name + a_ptr->name) + 1);
				*(t)-- = '\0';
			}
			else
			{
				object_desc_str_macro(t, (a_name + a_ptr->name));
			}
			object_desc_str_macro(t, "?)");
		}

		/* Grab any ego-item name */
		else if (o_ptr->guess2)
		{
			ego_item_type *e_ptr = &e_info[o_ptr->guess2];

			object_desc_str_macro(t, " (");
			if (*(e_name + e_ptr->name) == '(')
			{
				object_desc_str_macro(t, (e_name + e_ptr->name) + 1);
				*(t)-- = '\0';
			}
			else
			{
				object_desc_str_macro(t, (e_name + e_ptr->name));
			}
			object_desc_str_macro(t, "?)");
		}

		/* Grab any kind name */
		else if (k_ptr->guess)
		{
			object_kind *j_ptr = &k_info[lookup_kind(o_ptr->tval,k_ptr->guess-1)];

			object_desc_str_macro(t, " (of ");
			object_desc_str_macro(t, (k_name + j_ptr->name));
			object_desc_str_macro(t, "?)");
		}

		/* Grab any magic name */
		else if (o_ptr->feeling < INSCRIP_MIN_HIDDEN)
		{
			int i;
			int x1, x2; /* Fake xtra flags */
			u32b j;

			u32b f1 = o_ptr->can_flags1;
			u32b f2 = o_ptr->can_flags2;
			u32b f3 = o_ptr->can_flags3;
			u32b f4 = o_ptr->can_flags4;

			/* Remove flags on aware objects */
			if (k_info[o_ptr->k_idx].aware)
			{
				f1 &= ~(k_info[o_ptr->k_idx].flags1);
				f2 &= ~(k_info[o_ptr->k_idx].flags2);
				f3 &= ~(k_info[o_ptr->k_idx].flags3);
				f4 &= ~(k_info[o_ptr->k_idx].flags4);
			}

			x1 = 0;
			x2 = 0;

			/* Loop through first flags */
			for (i = 0, j = 0x00000001L; i < 32; i++, j<<=1)
			{
				/* Found a flag */
				if ((j & f1) != 0)
				{
					/* First flag */
					if (!x1) { x1 = 1; x2 = i; }

					/* More than one flag - can't guess magic name */
					else {x1 = -1; }
				}
			}

			/* Loop through second flags */
			for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
			{
				/* Found a flag */
				if ((j & f2) != 0)
				{
					/* First flag */
					if (!x1) { x1 = 2; x2 = i; }

					/* More than one flag - can't guess magic name */
					else {x1 = -1; }
				}
			}

			/* Loop through third flags */
			for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
			{
				/* Skip 'useless' flags */
				if (j & (TR3_ACTIVATE | TR3_ACT_ON_BLOW | TR3_UNCONTROLLED)) continue;

				/* Found a flag */
				if ((j & f3) != 0)
				{
					/* First flag */
					if (!x1) { x1 = 3; x2 = i; }

					/* More than one flag - can't guess magic name */
					else {x1 = -1; }
				}
			}

			/* Loop through fourth flags */
			for (i = 0, j = 0x00000001L; i < 32; i++, j <<=1)
			{
				/* Found a flag */
				if ((j & f4) != 0)
				{
					/* First flag */
					if (!x1) { x1 = 4; x2 = i; }

					/* More than one flag - can't guess magic name */
					else {x1 = -1; }
				}
			}

			if (x1 > 0)
			{
				object_desc_str_macro(t, " (");
				if (*(magic_name[x1-1][x2]) == '(')
				{
					object_desc_str_macro(t, (magic_name[x1-1][x2]) + 1);
					*(t)-- = '\0';
				}
				else
				{
					object_desc_str_macro(t, magic_name[x1-1][x2]);
				}

				object_desc_str_macro(t, "?)");
			}
		}
	}

	/* Looks like/holds a monster */
	if ((o_ptr->tval == TV_HOLD) && (o_ptr->name3 > 0))
	{
		object_desc_str_macro(t, " containing ");

		if (named)
		{

			if (!(r_info[o_ptr->name3].flags1 & (RF1_UNIQUE)))
			{
				cptr name = mon_buf;

				if (is_a_vowel(name[0])) object_desc_str_macro(t, "an ");
				else object_desc_str_macro(t, "a ");
			}

			object_desc_str_macro(t, mon_buf);
		}
		else
		{
			object_desc_str_macro(t, "something");
		}
	}

	/*
	 * Display object value
	 */
	if (o_ptr->ident & (IDENT_VALUE))
	{
		object_desc_str_macro(t, " worth ");
		object_desc_num_macro(t,object_value_real(o_ptr));
		object_desc_str_macro(t, " gold pieces");
	}

	/* No more details wanted */
	if (mode < 1) goto object_desc_done;

	/* Display the item like a weapon */
	if (o_ptr->to_h && o_ptr->to_d) show_weapon = TRUE;

	/* Display the item like armour */
	if (o_ptr->ac) show_armour = TRUE;

	/* Dump base weapon info */
	if (k_ptr->flags5 & (TR5_SHOW_DD))
	{
		/* Append a "damage" string */
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, p1);
		object_desc_num_macro(t, o_ptr->dd);
		object_desc_chr_macro(t, 'd');
		object_desc_num_macro(t, o_ptr->ds);
		object_desc_chr_macro(t, p2);
	}

	/* Dump bow weapon info */
	if (k_ptr->flags6 & (TR6_SHOW_MULT))
	{
		/* Hack -- Extract the "base power" */
		power = bow_multiplier(o_ptr->sval);

		/* Append a "power" string */
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, p1);
		object_desc_chr_macro(t, 'x');
		object_desc_num_macro(t, power);
		object_desc_chr_macro(t, p2);
	}

	/* Add the weapon bonuses */
	if (bonus)
	{
		/* Show the tohit/todam on request */
		if (show_weapon)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
			object_desc_int_macro(t, o_ptr->to_h);
			object_desc_chr_macro(t, ',');
			object_desc_int_macro(t, o_ptr->to_d);
			object_desc_chr_macro(t, p2);
		}

		/* Show the tohit if needed */
		else if (o_ptr->to_h)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
			object_desc_int_macro(t, o_ptr->to_h);
			object_desc_chr_macro(t, p2);
		}

		/* Show the todam if needed */
		else if (o_ptr->to_d)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, p1);
			object_desc_int_macro(t, o_ptr->to_d);
			object_desc_chr_macro(t, p2);
		}
	}


	/* Add the armor bonuses */
	if (bonus)
	{
		/* Show the armor class info */
		if (show_armour)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, b1);
			object_desc_num_macro(t, o_ptr->ac);
			object_desc_chr_macro(t, ',');
			object_desc_int_macro(t, o_ptr->to_a);
			object_desc_chr_macro(t, b2);
		}

		/* No base armor, but does increase armor */
		else if (o_ptr->to_a)
		{
			object_desc_chr_macro(t, ' ');
			object_desc_chr_macro(t, b1);
			object_desc_int_macro(t, o_ptr->to_a);
			object_desc_chr_macro(t, b2);
		}
	}

	/* Hack -- always show base armor */
	else if (show_armour)
	{
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, b1);
		object_desc_num_macro(t, o_ptr->ac);
		object_desc_chr_macro(t, b2);
	}


	/* No more details wanted */
	if (mode < 2) goto object_desc_done;


	/* Hack -- Wands and Staffs have charges */
	if ((charges) &&
	    ((k_ptr->flags5 & (TR5_SHOW_CHARGE)) != 0))
	{
		/* Dump " (N charges)" */
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, p1);
		object_desc_num_macro(t, o_ptr->charges);
		object_desc_str_macro(t, " charge");
		if (o_ptr->charges != 1)
		{
			object_desc_chr_macro(t, 's');
		}
		object_desc_chr_macro(t, p2);
	}

	/* Hack -- Process Lanterns/Torches */
	else if ((k_ptr->flags5 & (TR5_SHOW_FUEL)) != 0)
	{
		/* Hack -- Turns of light for normal lites */
		if (o_ptr->charges)
		{
			object_desc_str_macro(t, " (with ");
			object_desc_num_macro(t, o_ptr->charges);
			object_desc_str_macro(t, " turns of light)");
		}
		else if (o_ptr->timeout)
		{
			object_desc_str_macro(t, " (with ");
			object_desc_num_macro(t, o_ptr->timeout);
			object_desc_str_macro(t, " turns of light left)");
		}
	}

	/* Hack -- Process food/fuel */
	else if ((cheat_peek) && ((o_ptr->tval == TV_FOOD) || (o_ptr->tval == TV_MUSHROOM) || (o_ptr->tval == TV_FLASK)))
	{
		/* Dump " (N charges)" */
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, p1);
		object_desc_num_macro(t, o_ptr->charges);
		object_desc_str_macro(t, " fuel");
		object_desc_chr_macro(t, p2);
	}

	/* Dump "pval" flags for wearable items */
	if ((pval) && ((f1 & (TR1_PVAL_MASK)) || (f3 & (TR3_PVAL_MASK))))
	{
		/* Start the display */
		object_desc_chr_macro(t, ' ');
		object_desc_chr_macro(t, p1);

		/* Dump the "pval" itself */
		object_desc_int_macro(t, o_ptr->pval);

		/* Finish the display */
		object_desc_chr_macro(t, p2);
	}

	/* Indicate "charging" artifacts/rods */
	if (o_ptr->timeout)
	{
		if ((k_ptr->flags6 & (TR6_NO_TIMEOUT)) == 0)
		{
			/* Hack -- variant timeout stack */
			if (o_ptr->stackc)
			{
				object_desc_str_macro(t, " (");
				object_desc_num_macro(t, o_ptr->stackc);
				object_desc_str_macro(t, " charging)");
			}
			else
			{
				object_desc_str_macro(t, " (charging)");
			}
		}
	}

	/* No more details wanted */
	if (mode < 3) goto object_desc_done;

	/* Weapon coated */
	if (o_ptr->xtra1 >= OBJECT_XTRA_MIN_COATS)
	{
		int coating = lookup_kind(o_ptr->xtra1, o_ptr->xtra2);

		if (k_info[coating].flavor ? k_info[coating].aware & (AWARE_FLAVOR) : k_info[coating].aware & (AWARE_EXISTS))
		{
			object_desc_str_macro(t, " <");
			object_desc_str_macro(t, k_name+k_info[coating].name);
			object_desc_str_macro(t, "-coated>");
		}
	}
	/* Weapon has runes applied by player */
	else if (o_ptr->xtra1 >= OBJECT_XTRA_MIN_RUNES)
	{
		object_desc_str_macro(t, " <");
		if (o_ptr->xtra2 > 1)
		{
			object_desc_num_macro(t, o_ptr->xtra2);
			object_desc_chr_macro(t,' ');
		}
		object_desc_str_macro(t, y_name+y_info[o_ptr->xtra1 - OBJECT_XTRA_MIN_RUNES].name);
		if (o_ptr->xtra2 > 1) object_desc_chr_macro(t,'s');
		object_desc_chr_macro(t, '>');
	}
	/* Weapon has 'runes' identified by player */
	else if (o_ptr->ident & (IDENT_RUNES))
	{
		if (o_ptr->name1)
		{
			object_desc_str_macro(t, " <Unique>");
		}
		else if ((o_ptr->name2) && (e_info[o_ptr->name2].runest))
		{
			object_desc_str_macro(t, " <");
			if (e_info[o_ptr->name2].runesc > 1)
			{
				object_desc_num_macro(t, e_info[o_ptr->name2].runesc);
				object_desc_chr_macro(t,' ');
			}
			object_desc_str_macro(t, y_name+y_info[e_info[o_ptr->name2].runest].name);
			if (e_info[o_ptr->name2].runesc > 1) object_desc_chr_macro(t,'s');
			object_desc_chr_macro(t, '>');
		}
		else if (k_info[o_ptr->k_idx].runest)
		{
			object_desc_str_macro(t, " <");
			if (k_info[o_ptr->k_idx].runesc > 1)
			{
				object_desc_num_macro(t, k_info[o_ptr->k_idx].runesc);
				object_desc_chr_macro(t,' ');
			}
			object_desc_str_macro(t, y_name+y_info[k_info[o_ptr->k_idx].runest].name);
			if (k_info[o_ptr->k_idx].runesc > 1) object_desc_chr_macro(t,'s');
			object_desc_chr_macro(t, '>');
		}
		else if ((o_ptr->name2) || (o_ptr->xtra1))
		{
			object_desc_str_macro(t, " <Magic>");
		}
	}


	/* Use standard inscription */
	if (o_ptr->note)
	{
		u = quark_str(o_ptr->note);
	}

	/* Use nothing */
	else
	{
		u = NULL;
	}


	/* Use magical bag as proxy inscription */
	if (o_ptr->feeling >= MAX_INSCRIP)
	{
		int bag = lookup_kind(TV_BAG, o_ptr->feeling - MAX_INSCRIP);

		if (bag)
		{
			v = k_name + k_info[bag].name;
		}
		else v = NULL;
	}

	/* Use special inscription, if any */
	else if (o_ptr->feeling)
	{
		v = inscrip_text[o_ptr->feeling];
		if (strlen(v) == 0) v = NULL;
	}

	/* Use "cursed" if the item is known to be cursed */
	else if (cursed_p(o_ptr) && known)
	{
		v = "cursed";
	}

	/* Use "tried" if the object has been tested unsuccessfully, but not guessed */
	else if (!aware && object_tried_p(o_ptr))
	{
		v = "tried";
	}

	/* Use the discount, if any */
	else if (o_ptr->discount > 0)
	{
		char *q = discount_buf;
		object_desc_num_macro(q, o_ptr->discount);
		object_desc_str_macro(q, "% off");
		*q = '\0';
		v = discount_buf;
	}

	/* Nothing */
	else
	{
		v = NULL;
	}


	/* Inscription */
	if (u || v)
	{
		/* Begin the inscription */
		*t++ = ' ';
		*t++ = c1;

		/* Standard inscription */
		if (u)
		{
			/* Append the inscription */
			while ((t < b + 75) && *u) *t++ = *u++;
		}

		/* Special inscription too */
		if (u && v && (t < b + 75))
		{
			/* Separator */
			*t++ = ',';
			*t++ = ' ';
		}

		/* Special inscription */
		if (v)
		{
			/* Append the inscription */
			while ((t < b + 75) && *v) *t++ = *v++;
		}

		/* Terminate the inscription */
		*t++ = c2;
	}


object_desc_done:

	/* Terminate */
	*t = '\0';

	/* Truncate the string to 80 chars */
	tmp_buf[79] = '\0';

	/* Copy the string over */
	my_strcpy(buf, tmp_buf, sizeof(tmp_buf));
}

/*
 * Describe an item and pretend the item is fully known and has no flavor.
 */
void object_desc_spoil(char *buf, size_t max, const object_type *o_ptr, int pref, int mode)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;

	/* Make a backup */
	object_copy(i_ptr, o_ptr);

	/* HACK - Pretend the object is in a store inventory */
	i_ptr->ident |= IDENT_STORE;

	/* Describe */
	object_desc(buf, max, i_ptr, pref, mode);
}


/*
 * Convert an inventory index into a one character label.
 *
 * Note that the label does NOT distinguish inven/equip.
 */
char index_to_label(int i)
{
	/* Indexes for "inven" are easy */
	if (i < INVEN_WIELD) return (I2A(i));

	/* Indexes for "equip" are offset */
	return (I2A(i - INVEN_WIELD));
}


/*
 * Convert a label into the index of an item in the "inven".
 *
 * Return "-1" if the label does not indicate a real item.
 */
s16b label_to_inven(int c)
{
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return (-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}


/*
 * Convert a label into the index of a item in the "equip".
 *
 * Return "-1" if the label does not indicate a real item.
 */
s16b label_to_equip(int c)
{
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return (-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}


/*
 * Can the two weapons be wielded together?
 * If one does not exist, assume they can.
 */
bool two_weapons_balanced(const object_type *o_ptr, const object_type *i_ptr)
{
  /* If one of the weapons is a digger, the pair is not balanced
     (but they still can be dual-wielded if the digger goes
     into the main wield slot, see below) */
  if (o_ptr->tval == TV_DIGGING || i_ptr->tval == TV_DIGGING)
    return FALSE;

  /* Weapons do not unbalance each other if one does not exist... */
  if (!o_ptr->k_idx || !i_ptr->k_idx)
    return TRUE;
  /* ...or if one is actually a shield... */
  else if (o_ptr->tval == TV_SHIELD || i_ptr->tval == TV_SHIELD)
    return TRUE;
  /* ...or if one is actually a ring... */
  else if (o_ptr->tval == TV_RING || i_ptr->tval == TV_RING)
    return TRUE;
  /* ...or if one is actually an amulet... */
  else if (o_ptr->tval == TV_AMULET || i_ptr->tval == TV_AMULET)
    return TRUE;
  /* ...or i_ptr is throwing... */
  else if (is_known_throwing_item(i_ptr)
      && o_ptr->weight < 200)
    return TRUE;
  /* ...or o_ptr is throwing... */
  else if (is_known_throwing_item(o_ptr)
	   && i_ptr->weight < 200)
    return TRUE;
  /* ...or both are identical. */
  else if (i_ptr->tval == o_ptr->tval
	   && ((i_ptr->sval == o_ptr->sval
		&& (i_ptr->weight < 150)
		&& (o_ptr->weight < 150))
	       /* (All staffs are "identical".) */
	       || i_ptr->tval == TV_STAFF))
    return TRUE;
  /* No other possibility */
  else
    return FALSE;
}


/*
 * Determine which equipment slot (if any) an item likes.
 * Prefer empty slots, then prefer INVEN_WIELD.
 * For compatibility with do_cmd_wield assure
 * that if we propose INVEN_ARM then also INVEN_WIELD is legal.
 * It's possible to off-hand wield Ringil and fight unarmed,
 * but let people discover how to do that instead of making it
 * a separate explicit UI choice, annoying for everybody else.
 */
s16b wield_slot(const object_type *o_ptr)
{
  /* Slot for equipment */
  switch (o_ptr->tval)
    {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_STAFF:
      {
	object_type *w_ptr = &inventory[INVEN_WIELD];
	object_type *a_ptr = &inventory[INVEN_ARM];

	if (!w_ptr->k_idx)
	/* If main wield slot free, try to take it */
	  if (two_weapons_balanced(o_ptr, a_ptr))
	    /* Arm slot does not cause problems */
	    return INVEN_WIELD;
	  else
	    /* Arm slot precludes dual-wield; for do_cmd_wield
	       compatiblity we do not offer to replace arm slot */
	    return -1;
	else if (!a_ptr->k_idx)
	  /* else if arm slot free, try to take it */
	  if (two_weapons_balanced(o_ptr, w_ptr))
	    /* Main wield slot does not cause problems;
	       this single choice can be overriden in do_cmd_wield;
	       freeing the off-hand slot is also only the only way
	       to get a weapon to the off-hand slot */
	    return INVEN_ARM;
	  else
	    /* Main wield slot precludes dual-wield; replace it */
	    return INVEN_WIELD;
	else
	  /* else both slots are taken; try to replace
	     only the main wield slot to reduce the UI annoyance factor
	     --- now the off-hand weapon behaves much as a shield */
	  if (two_weapons_balanced(o_ptr, a_ptr))
	    /* Arm slot does not cause problems */
	    return INVEN_WIELD;
	  else
	    /* Arm slot precludes dual-wield, fail */
	    return -1;
      }

    case TV_DIGGING:
      {
	/* Diggers only go into the main wield slot,
	   but they coexist peacefully with everything, so that two-weapon
	   specialists do not have to unwield off-hand weapons all the time;
	   if there is systematic abuse, tone down digger attack power */
	return INVEN_WIELD;
      }

    case TV_INSTRUMENT:
    case TV_BOW:
      {
	return INVEN_BOW;
      }

    case TV_RING:
      {
	if (!inventory[INVEN_RIGHT].k_idx)
	  /* Use the right hand first */
	  return INVEN_RIGHT;
	else if (!inventory[INVEN_LEFT].k_idx)
	  /* Use the right hand second */
	  return INVEN_LEFT;
	else if (/*!inventory[INVEN_ARM].k_idx
		 && */p_ptr->pstyle == WS_RING)
	  /* Use the off-hand last */
	  return INVEN_ARM;
	else
	  /* Use the left hand for swapping, by default */
	  return INVEN_LEFT;
      }

    case TV_AMULET:
      {
	if (!inventory[INVEN_NECK].k_idx)
	  return INVEN_NECK;
	else if (/*!inventory[INVEN_ARM].k_idx
		 &&*/ p_ptr->pstyle == WS_AMULET)
	  return INVEN_ARM;
	else
	  return INVEN_NECK;
      }

    case TV_LITE:
      {
	return INVEN_LITE;
      }

    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
      {
	return INVEN_BODY;
      }

    case TV_CLOAK:
      {
	return INVEN_OUTER;
      }

    case TV_SHIELD:
      {
	return INVEN_ARM;
      }

    case TV_CROWN:
    case TV_HELM:
      {
	return INVEN_HEAD;
      }

    case TV_GLOVES:
      {
	return INVEN_HANDS;
      }

    case TV_BOOTS:
      {
	return INVEN_FEET;
      }

      /* Ammo asks for first quiver slot */
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
      {
	return INVEN_QUIVER;
      }
    case TV_EGG:
      {
	if (o_ptr->sval == SV_EGG_SPORE)
	  return INVEN_QUIVER;
	else
	  return -1;
      }
    case TV_FLASK:
    {
    	return INVEN_QUIVER;
    	break;
    }

    default:
      {
	break;
      }

    }

  /* No slot available */
  return -1;
}


/*
 * Get the string that represents the pseudo-tag of the given quiver slot.
 * The color of the pseudo-tag is also obtained.
 * Returns the length of the pseudo-tag (0 on error).
 */
static int get_pseudo_tag(int slot, char tag[], int max_len, byte *color)
{
	byte tag_num = 0;
	object_type *o_ptr, *i_ptr;
	bool locked;
	int i;
	byte o_group;

	/* Paranoia */
	if (!IS_QUIVER_SLOT(slot)) return 0;

	/* Get the object */
	o_ptr = &inventory[slot];

	/* Paranoia */
	if (!o_ptr->k_idx) return 0;

	/* Get the group of the object */
	o_group = quiver_get_group(o_ptr);

	/* Check if the ammo is locked */
	locked = get_tag_num(slot, quiver_group[o_group].cmd, &tag_num);

	/* We calculate the pseudo-tag if there is not a real one */
	if (!locked)
	{
		/* Search the slots of the given ammo type */
		for (i = INVEN_QUIVER; i < slot; i++)
		{
			byte i_group;

			/* Get the object */
			i_ptr = &inventory[i];

			/* Paranoia */
			if (!i_ptr->k_idx) continue;

			/* Get the group of the object */
			i_group = quiver_get_group(i_ptr);

			/* The groups must be equal */
			if (i_group != o_group) continue;

			/*
			 * A real tag overrides the current pseudo-tag when
			 * we have many locked ammo with the same tag
			 */
			(void)get_tag_num(i, quiver_group[i_group].cmd, &tag_num);

			/* But we always increment the pseudo-tag */
			++tag_num;
		}
	}

	/* Format the pseudo-tag */
	strnfmt(tag, max_len, "%s%c%d", (locked ? "@": ""), quiver_group[o_group].cmd, tag_num);

	/* Get the color of the group */
	*color = quiver_group[o_group].color;

	return strlen(tag);
}


/*
 * Return a string mentioning how a given item is carried
 */
cptr mention_use(int i)
{
	cptr p;

	/* Examine the location */
	switch (i)
	{
		case INVEN_WIELD: p = "Wielding"; break;
		case INVEN_BOW:   p = "Shooting"; break;
		case INVEN_LEFT:  p = "On left hand"; break;
		case INVEN_RIGHT: p = "On right hand"; break;
		case INVEN_NECK:  p = "Around neck"; break;
		case INVEN_LITE:  p = "Light source"; break;
		case INVEN_BODY:  p = "On body"; break;
		case INVEN_OUTER: p = "About body"; break;
		case INVEN_ARM:   p = "On arm"; break;
		case INVEN_HEAD:  p = "On head"; break;
		case INVEN_HANDS: p = "On hands"; break;
		case INVEN_FEET:  p = "On feet"; break;
		default:	  p = "In pack"; break;
	}

	/* Hack -- Handle quiver */
	if (IS_QUIVER_SLOT(i))
	{
		p = "In quiver";
	}

	/* Hack -- Heavy weapon */
	if ((i == INVEN_WIELD) && (p_ptr->heavy_wield)) p = "Just lifting";

	/* Hack -- Heavy bow */
	if ((i == INVEN_BOW) && (p_ptr->heavy_shoot)) p = "Just holding";

	/* Hack -- Non-shield item */
	if (i == INVEN_ARM)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (o_ptr->tval != TV_SHIELD)
		{
			p = "In off-hand";
		}
	}

	/* Hack -- Instrument */
	else if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (o_ptr->tval == TV_INSTRUMENT)
		{
                        p = "Playing";
		}
	}



	/* Return the result */
	return (p);
}


/*
 * Return a string describing how a given item is being worn.
 * Currently, only used for items in the equipment, not inventory.
 */
cptr describe_use(int i)
{
	cptr p;

	switch (i)
	{
		case INVEN_WIELD: p = "attacking monsters with"; break;
		case INVEN_BOW:   p = "shooting missiles with"; break;
		case INVEN_LEFT:  p = "wearing on your left hand"; break;
		case INVEN_RIGHT: p = "wearing on your right hand"; break;
		case INVEN_NECK:  p = "wearing around your neck"; break;
		case INVEN_LITE:  p = "using to light the way"; break;
		case INVEN_BODY:  p = "wearing on your body"; break;
		case INVEN_OUTER: p = "wearing on your back"; break;
		case INVEN_ARM:   p = "wearing on your arm"; break;
		case INVEN_HEAD:  p = "wearing on your head"; break;
		case INVEN_HANDS: p = "wearing on your hands"; break;
		case INVEN_FEET:  p = "wearing on your feet"; break;
		default:	  p = "carrying in your pack"; break;
	}

	if (i<0) p = "using from the ground";

	/* Hack -- Heavy weapon */
	if ((i == INVEN_WIELD) && (p_ptr->heavy_wield)) p = "Just lifting";

	/* Hack -- Heavy bow */
	if ((i == INVEN_BOW) && (p_ptr->heavy_shoot)) p = "Just holding";

	/* Hack -- Non-shield item */
	if (i == INVEN_WIELD)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];

		/* Multiple wielded items */
		if (o_ptr->number > 1) p = "carrying in your hands";

		/* Describe use better */
		else switch (o_ptr->tval)
		{
			case TV_DIGGING:
				p = "digging with";
				break;

			case TV_SWORD:
			case TV_STAFF:
			case TV_POLEARM:
			case TV_HAFTED:
				break;

			default:
				p = "using in your hands";
				break;
		}
	}

	/* Hack -- Non-shield item */
	else if (i == INVEN_ARM)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
		if (o_ptr->tval != TV_SHIELD)
		{
			p = "using in your off-hand";
		}
	}

	/* Hack -- Instrument */
	else if (i == INVEN_BOW)
	{
		object_type *o_ptr;
		o_ptr = &inventory[i];
                if (o_ptr->tval == TV_INSTRUMENT)
		{
                        p = "playing music with";
		}
	}

	/* Return the result */
	return p;
}

/*
 * Check an item against the item tester info
 */
bool item_tester_okay(const object_type *o_ptr)
{
	/* Hack -- allow listing empty slots */
	if (item_tester_full) return (TRUE);

	/* Require an item */
	if (!o_ptr->k_idx) return (FALSE);

	/* Hack -- ignore "gold" */
	/*if (o_ptr->tval >= TV_GOLD) return (FALSE);*/

	/* Hack -- check bag contents */
	if (o_ptr->tval == TV_BAG)
	{
		object_type object_type_body;
		object_type *i_ptr = &object_type_body;

		int i;

		/* Check bag contents */
		for (i = 0; i < INVEN_BAG_TOTAL; i++)
		{
			/* Empty slot */
			if (!(bag_holds[o_ptr->sval][i][0]) || !(bag_contents[o_ptr->sval][i])) continue;

			/* Check the tval */
			if (item_tester_tval)
			  {
			    if (!(item_tester_tval == bag_holds[o_ptr->sval][i][0]))
			      continue;
			    else
			      break;
			  }

			/* Fake the item */
			fake_bag_item(i_ptr, o_ptr->sval, i);

			/* Check the hook */
			if (item_tester_hook)
			  {
			    if (!(*item_tester_hook)(i_ptr))
			      continue;
			    else
			      break;
			  }
		}

		if (i < INVEN_BAG_TOTAL) return (TRUE);
	}

	/* Check the tval */
	if (item_tester_tval)
	{
		if (!(item_tester_tval == o_ptr->tval)) return (FALSE);
	}

	/* Check the hook */
	if (item_tester_hook)
	{
		if (!(*item_tester_hook)(o_ptr)) return (FALSE);
	}

	/* Assume okay */
	return (TRUE);
}



/*
 * Get the indexes of objects at a given floor location.
 *
 * Return the number of object indexes acquired.
 *
 * Never acquire more than "size" object indexes, and never return a
 * number bigger than "size", even if more floor objects exist.
 *
 * Valid flags are any combination of the bits:
 *
 *   0x01 -- Verify item tester
 *   0x02 -- Marked items only
 *   0x04 -- Stored items only
 *   0x08 -- Disallow 'stored' items
 */
sint scan_floor(int *items, int size, int y, int x, int mode)
{
	int this_o_idx, next_o_idx;

	int num = 0;

	/* Sanity */
	if (!in_bounds(y, x)) return (0);

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Verify item tester */
		if ((mode & 0x01) && !item_tester_okay(o_ptr)) continue;

		/* Marked items only */
		if ((mode & 0x02) && ((o_ptr->ident & (IDENT_MARKED)) == 0)) continue;

		/* Stored items only */
		if ((mode & 0x04) && ((o_ptr->ident & (IDENT_STORE)) == 0)) continue;

		/* Disallow 'stored' items */
		if ((mode & 0x08) && ((o_ptr->ident & (IDENT_STORE)) != 0)) continue;

		/* Accept this item */
		items[num++] = this_o_idx;

		/* Enforce size limit */
		if (num >= size) break;
	}

	/* Result */
	return (num);
}


/*
 * Get a random object which is associated with the feature.
 *
 * Return the object index.
 *
 * If no object found, return -1.
 *
 */
sint scan_feat(int y, int x)
{
	int this_o_idx, next_o_idx;

	int num = 0;

	int item = -1;

	/* Sanity */
	if (!in_bounds(y, x)) return (-1);

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Marked items only */
		if (!(o_ptr->ident & (IDENT_STORE))) continue;

		/* Enforce size limit */
		if (rand_int(++num)) continue;

		item = this_o_idx;
	}

	/* Result */
	return (item);
}


/*
 * Get the indexes of objects carried by monsters.
 * 
 * We list objects held by the 'm_idx' first, then objects held
 * by other monsters, if requested.
 * 
 * Valid flags are any combination of the bits:
 *
 *   0x01 -- Verify item tester
 *   0x02 -- Marked items only
 *   0x04 -- Specified monster only
 *   0x08 -- Visible monsters only
 *   0x10 -- Allies only
 *   0x20 -- Projectable monsters only
 *   0x40 -- Adjacent to player only
 */
sint scan_monsters(int *items, int size, int m_idx, int mode)
{
	int this_o_idx, next_o_idx;

	int num = 0;
	int i;
	
	monster_type *m_ptr = &m_list[m_idx];

	/* Get first monster's objects */
	if ((m_ptr->r_idx) && ((mode & 0x08) ? (m_ptr->ml) : TRUE)
			&& ((mode & 0x10) ? ((m_ptr->mflag & (MFLAG_ALLY)) != 0) : TRUE)
			&& ((mode & 0x20) ? player_can_fire_bold(m_ptr->fy, m_ptr->fx) : TRUE)
			&& ((mode & 0x40) ? (m_ptr->cdis <= 1) : TRUE))
	{
		/* Scan all objects held by m_idx */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
	
			/* Get the object */
			o_ptr = &o_list[this_o_idx];
	
			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;
	
			/* Verify item tester */
			if ((mode & 0x01) && !item_tester_okay(o_ptr)) continue;
	
			/* Marked items only */
			if ((mode & 0x02) && ((o_ptr->ident & (IDENT_MARKED)) == 0)) continue;
	
			/* Accept this item */
			items[num++] = this_o_idx;
	
			/* Enforce size limit */
			if (num >= size) break;
		}
	}
	
	/* Scan rest of monsters if we have space and are requested to */
	if (((mode & 0x04) == 0)  && (num < size))
	{
		/* Scan all monsters */
		for (i = 1; i < m_max; i++)
		{
			monster_type *m_ptr = &m_list[i];
	
			if (!m_ptr->r_idx) continue;
			
			if (i == m_idx) continue;

			/* Get monster's objects */
			if ( ((mode & 0x08) ? (m_ptr->ml) : TRUE)
					&& ((mode & 0x10) ? ((m_ptr->mflag & (MFLAG_ALLY)) != 0) : TRUE)
					&& ((mode & 0x20) ? player_can_fire_bold(m_ptr->fy, m_ptr->fx) : TRUE)
					&& ((mode & 0x40) ? (m_ptr->cdis <= 1) : TRUE))
			{
				/* Scan all objects held by m_idx */
				for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
				{
					object_type *o_ptr;
			
					/* Get the object */
					o_ptr = &o_list[this_o_idx];
			
					/* Get the next object */
					next_o_idx = o_ptr->next_o_idx;
			
					/* Verify item tester */
					if ((mode & 0x01) && !item_tester_okay(o_ptr)) continue;
			
					/* Marked items only */
					if ((mode & 0x02) && ((o_ptr->ident & (IDENT_MARKED)) == 0)) continue;
			
					/* Accept this item */
					items[num++] = this_o_idx;
			
					/* Enforce size limit */
					if (num >= size) break;
				}
			}
		}
	}

	/* Result */
	return (num);
}


/*
 * Choice window "shadow" of the "show_inven()" function
 */
void display_inven(void)
{
  register int i, k, n = 0;
	object_type *o_ptr;
	byte attr;

	char tmp_val[80];

	char o_name[80];

	/* Display the pack */
	for (k = 0, i = 0; i < INVEN_PACK; i++)
	{
		/* Examine the item */
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item "acceptable"? */
		if (item_tester_okay(o_ptr))
		{
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(k);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, k, 3, TERM_WHITE, tmp_val);

		/* Obtain an item description */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Get inventory color */
		attr = tval_to_attr[o_ptr->tval & 0x7F];

		/* Display the entry itself */
		Term_putstr(3, k, n, attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, k, 255);

		/* Display the weight if needed */
		if (o_ptr->weight)
		{
			int wgt = o_ptr->weight * o_ptr->number;
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			Term_putstr(71, k, -1, TERM_WHITE, tmp_val);
		}

		k++;
	}

	/* Erase the rest of the window */
	for (i = k; i < Term->hgt; i++)
	{
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}


/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(void)
{
  register int i, n;
	object_type *o_ptr;
	byte attr;

	char tmp_val[80];

	char o_name[80];

	char ptag_desc[MAX_QUIVER][10];
	byte ptag_len[MAX_QUIVER];
	byte ptag_color[MAX_QUIVER];
	byte max_ptag_len = 0;
	byte ptag_space;

	/* Get the pseudo-tags of the quiver slots */
	/* Calculate the maximum length of the pseudo-tags (for alignment) */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* The index in the temporary arrays */
		int q = i - INVEN_QUIVER;

		/* Paranoia */
		ptag_len[q] = 0;

		/* Get the object */
		o_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!o_ptr->k_idx) continue;

		/* Store pseudo-tag data in the arrays */
		ptag_len[q] = get_pseudo_tag(i, ptag_desc[q],
			sizeof(ptag_desc[q]), &ptag_color[q]);

		/* Update the maximum length if necessary */
		if (ptag_len[q] > max_ptag_len)
		{
			max_ptag_len = ptag_len[q];
		}
	}

	/* Display the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		/* Examine the item */
		o_ptr = &inventory[i];

		/* Hack -- Never show empty quiver slots */
		if (!o_ptr->k_idx && IS_QUIVER_SLOT(i))
		{
			/* Clear that line */
			Term_erase(0, i - INVEN_WIELD, 255);
			continue;
		}

		/* Hack -- Never show the gap between equipment and quiver */
		if (i == INVEN_BLANK)
		{
			/* Clear that line */
			Term_erase(0, i - INVEN_WIELD, 255);
			continue;
		}

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item "acceptable"? */
		if (o_ptr->k_idx && item_tester_okay(o_ptr))
		{
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i - INVEN_WIELD, 3, TERM_WHITE, tmp_val);

		/* Obtain an item description */
		if (o_ptr->k_idx)
		  object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
		else
		  o_name[0] = 0;

		/* Obtain the length of the description */
		n = strlen(o_name);

		/* Get inventory color */
		if (o_ptr->k_idx)
		  attr = tval_to_attr[o_ptr->tval & 0x7F];
		else
		  attr = 0;

		/* Regular slots don't have a pseudo-tag */
		ptag_space = 0;

		/* Show quiver slot pseudo-tag if needed */
		if (IS_QUIVER_SLOT(i) &&
			(ptag_len[i - INVEN_QUIVER] > 0))
		{
			/* The index in the temporary arrays */
			int q = i - INVEN_QUIVER;

			/* Reserve space for the pseudo-tag in this case */
			ptag_space = max_ptag_len + 1;

			/* Hack -- Clear that space first */
			Term_erase(3, i - INVEN_WIELD, ptag_space);

			/* Show the pseudo-tag */
			Term_putstr(3 + max_ptag_len - ptag_len[q],
				i - INVEN_WIELD, ptag_len[q],
				ptag_color[q], ptag_desc[q]);
		}

		/* Display the entry itself */
		Term_putstr(3 + ptag_space , i - INVEN_WIELD, n, attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3 + ptag_space + n, i - INVEN_WIELD, 255);

		/* Display the slot description (if needed) */
		if (show_labels)
		{
			Term_putstr(61, i - INVEN_WIELD, -1, TERM_WHITE, "<--");
			Term_putstr(65, i - INVEN_WIELD, -1, TERM_WHITE, mention_use(i));
		}

		/* Display the weight (if needed) */
		if (o_ptr->k_idx && o_ptr->weight)
		{
			int wgt = o_ptr->weight * o_ptr->number;
			int col = (show_labels ? 52 : 71);
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			Term_putstr(col, i - INVEN_WIELD, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++)
	{
		/* Clear that line */
		Term_erase(0, i, 255);
	}
}



/*
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void)
{
	int i, j, k, l, z = 0;
	int col, len, lim;

	object_type *o_ptr;

	char o_name[80];

	char tmp_val[80];

	int out_index[24];
	byte out_color[24];
	char out_desc[24][80];

	/* Default length */
	len = 79 - 50;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;

	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Track */
		z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++)
	{
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the index */
		out_index[k] = i;

		/* Get inventory color */
		out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];

		/* Save the object description */
		my_strcpy(out_desc[k], o_name, sizeof(out_desc[k]));

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		int wgt;

		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		wgt = o_ptr->weight * o_ptr->number;
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
		put_str(tmp_val, j + 1, 71);
	}

	/*
	 * Add notes about slots used by the quiver or bags, if we have space, want
	 * to show all slots, and have items in the quiver or bags.
	 */
	if ((p_ptr->pack_size_reduce_quiver || p_ptr->pack_size_reduce_bags || (p_ptr->pack_size_reduce_study/3)) && (item_tester_full) &&
		(j <= (INVEN_PACK - p_ptr->pack_size_reduce_quiver - p_ptr->pack_size_reduce_bags - (p_ptr->pack_size_reduce_study/3))))
	{
		int ammo_num = 0, ammo_slot;

		/* Count quiver ammo */
		for (i = INVEN_QUIVER; i < END_QUIVER; i++)
		{
			/* Get the object */
			o_ptr = &inventory[i];

			/* Ignore empty objects */
			if (!o_ptr->k_idx) continue;

			/* Increment counter */
			ammo_num += o_ptr->number * quiver_space_per_unit(o_ptr);
		}

		/* Insert a blank dividing line, if we have the space. */
		if (j <= ((INVEN_PACK - 1) - p_ptr->pack_size_reduce_quiver - p_ptr->pack_size_reduce_bags - (p_ptr->pack_size_reduce_study/3)))
		{
			j++;

			prt("", j, col ? col - 2 : col);
		}

		for (i = 0; i < p_ptr->pack_size_reduce_quiver + p_ptr->pack_size_reduce_bags + (p_ptr->pack_size_reduce_study/3); i++)
		{
			int attr;

			/* Go to next line. */
			j++;

			prt("", j, col ? col - 2 : col);

			/* Determine index, print it out. */
			sprintf(tmp_val, "%c)", index_to_label(INVEN_PACK -
				p_ptr->pack_size_reduce_quiver - p_ptr->pack_size_reduce_bags - (p_ptr->pack_size_reduce_study/3) + i));

			put_str(tmp_val, j, col);

			/* Get the number of missiles gathered in this slot */
			if (i == 0)
			{
				ammo_slot = ammo_num -
					99 * (p_ptr->pack_size_reduce_quiver - 1);
			}
			else
			{
				ammo_slot = 99;
			}

			/* Hack -- use "(QUIVER)" as a description. */
			if (i < p_ptr->pack_size_reduce_quiver)
			{
				strnfmt(o_name, sizeof(o_name), "(QUIVER - %d missile%s)", ammo_slot, (ammo_slot == 1) ? "": "s");
				attr = TERM_BLUE;
			}
			/* Hack -- use "(BAG CONTENTS)" as a description. */
			else if (i < p_ptr->pack_size_reduce_quiver + p_ptr->pack_size_reduce_bags)
			{
				strnfmt(o_name, sizeof(o_name), "(BAG CONTENTS)");
				attr = TERM_BLUE_SLATE;
			}
			/* Hack -- use "(STUDIED - Book name)" as a description. */
			else
			{
				attr = TERM_DEEP_L_BLUE;
				strnfmt(o_name, sizeof(o_name), "(STUDIED - %s)", k_name + k_info[p_ptr->study_slot[i - p_ptr->pack_size_reduce_quiver - p_ptr->pack_size_reduce_bags]].name + 3);
			}

			c_put_str(attr, o_name, j, col + 3);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < Term->hgt)) prt("", j + 1, col ? col - 2 : col);
}


/*
 * Display the equipment.
 */
void show_equip(void)
{
	int i, j, k, l;
	int col, len, lim;

	object_type *o_ptr;

	char tmp_val[80];

	char o_name[80];

	int out_index[24];
	byte out_color[24];
	char out_desc[24][80];

	char ptag_desc[MAX_QUIVER][10];
	byte ptag_len[MAX_QUIVER];
	byte ptag_color[MAX_QUIVER];
	byte max_ptag_len = 0;
	byte ptag_space;

	/* Default length */
	len = 79 - 50;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for labels (if needed) */
	if (show_labels) lim -= (14 + 2);

	/* Require space for weight (if needed) */
	lim -= 9;

	/*
	 * Get the pseudo-tags of the quiver slots
	 * Calculate the maximum length of the pseudo-tags (for UI
	 * alignment if show_labels is off)
	 */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* The index in the temporary arrays */
		int q = i - INVEN_QUIVER;

		/* Paranoia */
		ptag_len[q] = 0;

		/* Get the object */
		o_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!o_ptr->k_idx) continue;

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Store pseudo-tag data in the arrays */
		ptag_len[q] = get_pseudo_tag(i, ptag_desc[q],
			sizeof(ptag_desc[q]), &ptag_color[q]);

		/* Update the maximum length if necessary */
		if (ptag_len[q] > max_ptag_len)
		{
			max_ptag_len = ptag_len[q];
		}
	}

	/* Scan the equipment list */
	for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Don't show empty quiver slots */
		if (!o_ptr->k_idx && IS_QUIVER_SLOT(i)) continue;

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Description */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Regular slots don't have a pseudo-tag */
		ptag_space = 0;

		/* But quiver slots reserve some space if show_labels is off */
		if (IS_QUIVER_SLOT(i) && (ptag_len[i - INVEN_QUIVER] > 0) &&
			!show_labels)
		{
			ptag_space = max_ptag_len + 1;
		}

		/* Truncate the description */
		o_name[lim - ptag_space] = 0;

		/* Save the index */
		out_index[k] = i;

		/* Get inventory color */
		out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];

		/* Save the description */
		my_strcpy(out_desc[k], o_name, sizeof(out_desc[k]));

		/* Extract the maximal length (see below) */
		l = strlen(out_desc[k]) + (2 + 3) + ptag_space;

		/* Increase length for labels (if needed) */
		if (show_labels) l += (14 + 2);

		/* Increase length for weight (if needed) */
		l += 9;

		/* Maintain the max-length */
		if (l > len) len = l;

		/* Advance the entry */
		k++;
	}

	/* Hack -- Find a column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		int wgt;

		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Show a blank line between "real" equipment and quiver */
		if (i == INVEN_BLANK) continue;

		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j+1, col);

		/* Use labels */
		if (show_labels)
		{
			/* Mention the use */
			sprintf(tmp_val, "%-14s: ", mention_use(i));
			put_str(tmp_val, j+1, col + 3);

			/* Show the pseudo-tag if needed */
			if (IS_QUIVER_SLOT(i) &&
				(ptag_len[i - INVEN_QUIVER] > 0))
			{
				int q = i - INVEN_QUIVER;

				c_put_str(ptag_color[q], ptag_desc[q],
					j+1, col + 3 + 13 - ptag_len[q]);
			}

			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1, col + 3 + 14 + 2);
		}

		/* No labels */
		else
		{
			/* Regular slots don't have a pseudo-tag */
			ptag_space = 0;

			/* Show pseudo-tags if necessary */
			if (IS_QUIVER_SLOT(i) &&
				(ptag_len[i - INVEN_QUIVER] > 0))
			{
				int q = i - INVEN_QUIVER;

				/* Show the pseudo tag */
				c_put_str(ptag_color[q], ptag_desc[q], j+1,
					col + 3 + max_ptag_len - ptag_len[q]);

				/* Reserve space for the pseudo-tag */
				ptag_space = max_ptag_len + 1;
			}

			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1,
				col + 3 + ptag_space);
		}

		/* Display the weight if needed */
		wgt = o_ptr->weight * o_ptr->number;
		sprintf(tmp_val, "%3d.%d lb", wgt / 10, wgt % 10);
		put_str(tmp_val, j+1, 71);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < Term->hgt)) prt("", j + 1, col ? col - 2 : col);
}


/*
 * Display a list of the items on the floor at the given location.
 */
void show_floor(const int *floor_list, int floor_num, bool gold)
{
	int i, j, k, l;
	int col, len, lim;

	object_type *o_ptr;

	char o_name[80];

	char tmp_val[80];

	int out_index[MAX_FLOOR_STACK];
	byte out_color[MAX_FLOOR_STACK];
	char out_desc[MAX_FLOOR_STACK][80];


	/* Default length */
	len = 79 - 50;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight */
	lim -= 9;

	/* Limit displayed floor items to 23 (screen limits) */
	if (floor_num > MAX_FLOOR_STACK) floor_num = MAX_FLOOR_STACK;

	/* Display the floor */
	for (k = 0, i = 0; i < floor_num; i++)
	{
		o_ptr = &o_list[floor_list[i]];

		/* Optionally, show gold */
		if ((o_ptr->tval != TV_GOLD) || (!gold))
		{
			/* Is this item acceptable?  (always rejects gold) */
			if (!item_tester_okay(o_ptr)) continue;
		}

		/* Describe the object */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the index */
		out_index[k] = i;

		/* Get inventory color */
		out_color[k] = tval_to_attr[o_ptr->tval % N_ELEMENTS(tval_to_attr)];

		/* Save the object description */
		my_strcpy(out_desc[k], o_name, sizeof(out_desc[k]));

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		int wgt;

		/* Get the index */
		i = floor_list[out_index[j]];

		/* Get the item */
		o_ptr = &o_list[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare an index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(out_index[j]));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight */
		wgt = o_ptr->weight * o_ptr->number;
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
		put_str(tmp_val, j + 1, 71);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < Term->hgt)) prt("", j + 1, col ? col - 2 : col);
}


/*
 * Flip "inven" and "equip" in any sub-windows
 */
void toggle_inven_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Flip inven to equip */
		if (op_ptr->window_flag[j] & (PW_INVEN))
		{
			/* Flip flags */
			op_ptr->window_flag[j] &= ~(PW_INVEN);
			op_ptr->window_flag[j] |= (PW_EQUIP);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);
		}

		/* Flip inven to equip */
		else if (op_ptr->window_flag[j] & (PW_EQUIP))
		{
			/* Flip flags */
			op_ptr->window_flag[j] &= ~(PW_EQUIP);
			op_ptr->window_flag[j] |= (PW_INVEN);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN);
		}
	}
}




/*
 * Verify the choice of an item.
 *
 * The item can be negative to mean "item on floor".
 */
static bool verify_item(cptr prompt, int item)
{
	char o_name[80];

	char out_val[160];

	object_type *o_ptr;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Prompt */
	sprintf(out_val, "%s %s? ", prompt, o_name);

	/* Query */
	return (get_check(out_val));
}


/*
 * Hack -- allow user to "prevent" certain choices.
 *
 * Now can use '<n' to confirm if stack is less than size 'n' or '>n' if stack is more than size 'n'.
 *
 * The item can be negative to mean "item on floor".
 */
static bool get_item_allow(int item)
{
	cptr s;

	object_type *o_ptr;

	int n;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* No inscription */
	if (!o_ptr->note) return (TRUE);

	/* Find a '!' */
	s = strchr(quark_str(o_ptr->note), '!');

	/* Process preventions */
	while (s)
	{
		/* Check the "restriction" */
		if ((s[1] == p_ptr->command_cmd) || (s[1] == '*'))
		{
			/* Verify the choice */
			if (!verify_item("Really try", item)) return (FALSE);
		}

		/* Find another '!' */
		s = strchr(s + 1, '!');
	}

	/* Find a '<' */
	s = strchr(quark_str(o_ptr->note), '<');

	/* Process preventions */
	while (s)
	{

		/* Check the "restriction" */
		if ((s[1] >= '0') || (s[1] <= '9'))
		{
			n = atoi(s+1);

			/* Verify the choice */
			if ((o_ptr->number < n) && (!verify_item("Really try", item))) return (FALSE);
		}

		/* Check the "restriction" */
		else if ((s[1] == p_ptr->command_cmd) || (s[1] == '*'))
		{
			/* Check the "restriction" */
			if ((s[2] >= '0') || (s[2] <= '9'))
			{
				n = atoi(s+2);

				/* Verify the choice */
				if ((o_ptr->number < n) && (!verify_item("Really try", item))) return (FALSE);
			}

		}

		/* Find another '!' */
		s = strchr(s + 1, '<');
	}


	/* Find a '>' */
	s = strchr(quark_str(o_ptr->note), '>');

	/* Process preventions */
	while (s)
	{

		/* Check the "restriction" */
		if ((s[1] >= '0') || (s[1] <= '9'))
		{
			n = atoi(s+1);

			/* Verify the choice */
			if ((o_ptr->number > n) && (!verify_item("Really try", item))) return (FALSE);
		}

		/* Check the "restriction" */
		else if ((s[1] == p_ptr->command_cmd) || (s[1] == '*'))
		{
			/* Check the "restriction" */
			if ((s[2] >= '0') || (s[2] <= '9'))
			{
				n = atoi(s+2);

				/* Verify the choice */
				if ((o_ptr->number > n) && (!verify_item("Really try", item))) return (FALSE);
			}

		}

		/* Find another '!' */
		s = strchr(s + 1, '>');
	}

	/* Allow it */
	return (TRUE);
}


/*
 * Verify the "okayness" of a given item.
 *
 * The item can be negative to mean "item on floor".
 */
static bool get_item_okay(int item)
{
	object_type *o_ptr;

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Floor */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Verify the item */
	return (item_tester_okay(o_ptr));
}



/*
 * Find the "first" inventory object with the given "tag".
 *
 * A "tag" is a char "n" appearing as "@n" anywhere in the
 * inscription of an object.
 *
 * Also, the tag "@xn" will work as well, where "n" is a tag-char,
 * and "x" is the "current" p_ptr->command_cmd code.
 *
 */
static int get_tag(int *cp, char tag)
{
	int i;
	cptr s;

	/*
	 * The 'f'ire and 't'hrow commands behave differently when we are using the
	 * equipment (quiver)
	 */
	if (((p_ptr->command_cmd == 'f') || (p_ptr->command_cmd == 'v')) && (p_ptr->command_wrk == USE_EQUIP))
	{
		/* The pseudo-tag */
		byte tag_num = 0;
		object_type *o_ptr;
		byte group;

		/* Get the proper quiver group to determine which objects can be selected */
		if (p_ptr->command_cmd == 'f')
		{
			/* Ammo groups are taken from the missile weapon */
			switch (p_ptr->fire_command)
			{
				case COMMAND_ITEM_FIRE_XBOW:	group = QUIVER_GROUP_BOLTS;	break;
				case COMMAND_ITEM_FIRE_BOW:	group = QUIVER_GROUP_ARROWS;	break;
				default:	group = QUIVER_GROUP_SHOTS;	break;
			}
		}
		/* Hack - shots are not a throwing weapon here */
		else
		{
		 	group = QUIVER_GROUP_THROWING_WEAPONS;
		}

		/* Iterate over the quiver */
		for (i = INVEN_QUIVER; i < END_QUIVER; i++)
		{
			o_ptr = &inventory[i];

			/* (Paranoia) Ignore empty slots */
			if (!o_ptr->k_idx) continue;

			 /* Groups must be equal */
			if (quiver_get_group(o_ptr) != group) continue;

			/* Allow pseudo-tag override */
			(void)get_tag_num(i, quiver_group[group].cmd, &tag_num);

			/* We have a match? */
			if (I2D(tag_num) == tag)
			{
				*cp = i;
				return TRUE;
			}

			/* Try with the next pseudo-tag */
			++tag_num;
		}
	}

	/* Check every object */
	for (i = 0; i < INVEN_TOTAL; ++i)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->note) continue;

		/* Find a '@' */
		s = strchr(quark_str(o_ptr->note), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Check the special tags */
			if ((s[1] == p_ptr->command_cmd) && (s[2] == tag))
			{
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}



/*
 * Let the user select an item, save its "index"
 *
 * Return TRUE only if an acceptable item was chosen by the user.
 *
 * The selected item must satisfy the "item_tester_hook()" function,
 * if that hook is set, and the "item_tester_tval", if that value is set.
 *
 * All "item_tester" restrictions are cleared before this function returns.
 *
 * The user is allowed to choose acceptable items from the equipment,
 * inventory, or floor, respectively, if the proper flag was given,
 * and there are any acceptable items in that location.
 *
 * The equipment or inventory are displayed (even if no acceptable
 * items are in that location) if the proper flag was given.
 *
 * If there are no acceptable items available anywhere, and "str" is
 * not NULL, then it will be used as the text of a warning message
 * before the function returns.
 *
 * Note that the user must press "-" to specify the item on the floor,
 * and there is no way to "examine" the item on the floor, while the
 * use of "capital" letters will "examine" an inventory/equipment item,
 * and prompt for its use.
 *
 * If a legal item is selected from the inventory, we save it in "cp"
 * directly (0 to 35), and return TRUE.
 *
 * If a legal item is selected from the floor, we save it in "cp" as
 * a negative (-1 to -511), and return TRUE.
 *
 * If no item is available, we do nothing to "cp", and we display a
 * warning message, using "str" if available, and return FALSE.
 *
 * If no item is selected, we do nothing to "cp", and return FALSE.
 *
 * Global "p_ptr->command_new" is used when viewing the inventory or equipment
 * to allow the user to enter a command while viewing those screens, and
 * also to induce "auto-enter" of stores, and other such stuff.
 *
 * Global "p_ptr->command_see" may be set before calling this function to start
 * out in "browse" mode.  It is cleared before this function returns.
 *
 * Global "p_ptr->command_wrk" is used to choose between equip/inven/floor
 * listings.  It is equal to USE_INVEN or USE_EQUIP or USE_FLOOR, except
 * when this function is first called, when it is equal to zero, which will
 * cause it to be set to USE_INVEN.
 *
 * We always erase the prompt when we are done, leaving a blank line,
 * or a warning message, if appropriate, if no items are available.
 *
 * Note that only "acceptable" floor objects get indexes, so between two
 * commands, the indexes of floor objects may change.  XXX XXX XXX
 */
bool get_item(int *cp, cptr pmt, cptr str, int mode)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	key_event ke;

	int i, j, k;

	int i1, i2;
	int e1, e2;
	int f1, f2;
	int a1, a2;
	int t1, t2;

	int monster_inven = 0;
	
	bool done, item;

	bool oops = FALSE;

	bool use_inven = ((mode & (USE_INVEN)) ? TRUE : FALSE);
	bool use_equip = ((mode & (USE_EQUIP)) ? TRUE : FALSE);
	bool use_floor = ((mode & (USE_FLOOR)) ? TRUE : FALSE);
	bool use_featg = ((mode & (USE_FEATG)) ? TRUE : FALSE);
	bool use_featu = ((mode & (USE_FEATU)) ? TRUE : FALSE);
	bool use_feath = ((mode & (USE_FEATH)) ? TRUE : FALSE);
	bool use_quiver = ((mode & (USE_QUIVER)) ? TRUE: FALSE);
	bool use_self = ((mode & (USE_SELF)) ? TRUE: FALSE);
	bool use_skin = ((mode & (USE_SKIN)) ? TRUE: FALSE);
	bool use_allies = ((mode & (USE_ALLY)) ? TRUE: FALSE);
	bool use_target = ((mode & (USE_TARGET)) ? TRUE: FALSE);
	bool use_range = ((mode & (USE_RANGE)) ? TRUE: FALSE);
	bool use_known = ((mode & (USE_KNOWN)) ? TRUE: FALSE);
	bool use_gold = ((mode & (USE_GOLD)) ? TRUE: FALSE);

	bool allow_inven = FALSE;
	bool allow_equip = FALSE;
	bool allow_floor = FALSE;
	bool allow_feats = FALSE;
	bool allow_self = FALSE;
	bool allow_allies = FALSE;
	bool allow_target = FALSE;

	bool toggle = FALSE;

	char tmp_val[160];
	char out_val[160];

	int floor_list[MAX_FLOOR_STACK];
	int floor_num;

	int allies_list[MAX_FLOOR_STACK];
	int allies_num;

	int target_list[MAX_FLOOR_STACK];
	int target_num;

#ifdef ALLOW_REPEAT

	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* Verify the item */
		if (get_item_okay(*cp))
		{
			/* Forget the item_tester_tval restriction */
			item_tester_tval = 0;

			/* Forget the item_tester_hook restriction */
			item_tester_hook = NULL;

			/* Success */
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT */


	/* Paranoia XXX XXX XXX */
	msg_print(NULL);


	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;


	/* Full inventory */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid inventory */
	if (!use_inven) i2 = -1;

	/* Restrict inventory indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;

	/* Accept inventory */
	if (i1 <= i2) allow_inven = TRUE;

	/* Hack -- The quiver is displayed in the equipment window */
	if (use_quiver) use_equip = TRUE;

	/* Hack -- The skin is displayed in the equipment window */
	if (use_skin) use_equip = TRUE;

	/* Full equipment */
	e1 = INVEN_WIELD;
	e2 = INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!use_equip) e2 = -1;

	/* Restrict the beginning of the equipment */
	if (use_quiver) e1 = INVEN_QUIVER;

	/* Set up skin */
	if (use_skin)
	{
		/* Get monster race */
		monster_race *r_ptr = &r_info[rp_ptr->r_idx];

		/* Get skin type */
		j = r_ptr->flags8 & (RF8_HAS_SCALE) ? SV_SKIN_SCALE_COAT :
			(r_ptr->flags8 & (RF8_HAS_FEATHER) ? SV_SKIN_FEATHER_COAT :
			(r_ptr->flags8 & (RF8_HAS_FUR) ? SV_SKIN_FUR_COAT : SV_SKIN_SKIN));

		/* Get bare skin */
		k = lookup_kind(TV_SKIN, j);

		/* Set up bare skin */
		for (i = INVEN_NECK; i <= INVEN_FEET; i++)
		{
			object_type *o_ptr = &inventory[i];

			/* Hack -- Skip light source */
			if (i == INVEN_LITE) continue;

			/* Bare skin */
			if (!o_ptr->k_idx)
			{
				/* Set up skin */
				object_prep(o_ptr, k);

				/* And remove weight */
				o_ptr->weight = 0;

				/* And belong to the player */
				o_ptr->name3 = -1;

				/* And store it - hack used later to identify which slots to clear */
				o_ptr->ident |= (IDENT_STORE);
			}
		}
	}

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	/* Accept equipment */
	if (e1 <= e2) allow_equip = TRUE;

	/* Scan all objects in the grid */
	floor_num = scan_floor(floor_list, MAX_FLOOR_STACK, py, px, 0x08);

	/* Full floor */
	f1 = 0;
	f2 = floor_num - 1;

	/* Forbid floor */
	if (!use_floor) f2 = -1;

	/* Restrict floor indexes */
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f1]))) f1++;
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f2]))) f2--;

	/* Accept floor */
	if (f1 <= f2) allow_floor = TRUE;

	/* Set monster inventory scan flags */
	if (!use_range) monster_inven |= 0x40;
	if (!use_known) monster_inven |= 0x20;
	
	/* Scan all objects carried by allies */
	allies_num = scan_monsters(allies_list, MAX_FLOOR_STACK, p_ptr->target_who, monster_inven | (0x19));

	/* Full allies */
	a1 = 0;
	a2 = allies_num - 1;

	/* Forbid allies */
	if (!use_allies) a2 = -1;

	/* Restrict allies indexes */
	while ((a1 <= a2) && (!get_item_okay(0 - allies_list[a1]))) a1++;
	while ((a1 <= a2) && (!get_item_okay(0 - allies_list[a2]))) a2--;

	/* Accept allies */
	if (a1 <= a2) allow_allies = TRUE;

	/* Scan all objects carried by target */
	target_num = scan_monsters(target_list, MAX_FLOOR_STACK, p_ptr->target_who, monster_inven | (0x0D));

	/* Full target */
	t1 = 0;
	t2 = target_num - 1;

	/* Forbid target */
	if (!use_target) t2 = -1;

	/* Restrict target indexes */
	while ((t1 <= t2) && (!get_item_okay(0 - target_list[t1]))) t1++;
	while ((t1 <= t2) && (!get_item_okay(0 - target_list[t2]))) t2--;
	
	/* Accept target */
	if (t1 <= t2) allow_target = TRUE;

	/* Scan the feature */
	if ((use_featg) && (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags3 & (FF3_GET_FEAT)))
	{
		object_type *i_ptr;
		object_type object_type_body;

		i_ptr = &object_type_body;

		if ((make_feat(i_ptr,p_ptr->py,p_ptr->px)) && item_tester_okay(i_ptr)) allow_feats = TRUE;
	}

	/* Rescan the feature */
	if ((use_featu) && (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags3 & (FF3_USE_FEAT)))
	{
		object_type *i_ptr;
		object_type object_type_body;

		i_ptr = &object_type_body;

		if ((make_feat(i_ptr,p_ptr->py,p_ptr->px)) && item_tester_okay(i_ptr)) allow_feats = TRUE;
	}

	/* Scan the feature -- this is a big hack.  We allow the feature to be set on fire. */
	if ((use_feath) && (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags2 & (FF2_HURT_FIRE))) allow_feats = TRUE;

	/* Scan the feature -- this is a big hack.  If we have burnable objects on the floor, we allow the feature to be set on fire. */
	if ((use_feath) && !(allow_feats))
	{
		int b1, b2;

		b1 = 0;
		b2 = floor_num - 1;

		/* Restrict floor indexes */
		while ((b1 <= b2) && (!hates_fire(&o_list[floor_list[b1]]))) b1++;
		while ((b1 <= b2) && (!hates_fire(&o_list[floor_list[b2]]))) b2--;

		if (b1 <= b2) allow_feats = TRUE;
	}

	/* Scan oneself */
	if ((use_self) && (item_tester_okay(&inventory[INVEN_SELF])))
		allow_self = TRUE;

	/* Require at least one legal choice */
	if (!allow_inven && !allow_equip && !allow_floor && !allow_feats && !allow_self && !use_gold)
	{
		/* Cancel p_ptr->command_see */
		if (!OPT(show_lists)) p_ptr->command_see = FALSE;

		/* Oops */
		oops = TRUE;

		/* Done */
		done = TRUE;
	}

	/* Analyze choices */
	else
	{
		/* Hack -- Start on equipment if requested */
		if (p_ptr->command_see &&
		    (p_ptr->command_wrk == (USE_EQUIP)) &&
		    use_equip)
		{
			p_ptr->command_wrk = (USE_EQUIP);
		}

		/* Hack -- Start on equipment if shooting, throwing or fueling */
		if ((p_ptr->command_cmd == 'f'
		     || p_ptr->command_cmd == 'v'
		     || (p_ptr->command_wrk != (USE_INVEN) && p_ptr->command_cmd == 'F'))
			&& allow_equip)
		{
			p_ptr->command_wrk = (USE_EQUIP);
		}

		/* Use inventory if allowed */
		else if (use_inven)
		{
			p_ptr->command_wrk = (USE_INVEN);
		}

		/* Use equipment if allowed */
		else if (use_equip)
		{
			p_ptr->command_wrk = (USE_EQUIP);
		}

		/* Use floor if allowed */
		else if (use_floor)
		{
			p_ptr->command_wrk = (USE_FLOOR);
		}

		/* Use allies if allowed */
		else if (use_allies)
		{
			p_ptr->command_wrk = (USE_ALLY);
		}

		/* Use target if allowed */
		else if (use_target)
		{
			p_ptr->command_wrk = (USE_TARGET);
		}

		/* Hack -- Use (empty) inventory */
		else
		{
			p_ptr->command_wrk = (USE_INVEN);
		}
	}

	/* Look up the tag */
	if (get_tag(&k, '*'))
	{
		/*Hack -- Validate the item */
		if ((k < INVEN_WIELD) ? !allow_inven : !allow_equip)
		{
			bell(format("Illegal object choice%s!", cheat_xtra ? " (1, tag)" : ""));
		}

		/* Validate the item */
		else if (!get_item_okay(k))
		{
			bell(format("Illegal object choice%s!", cheat_xtra ? " (2, tag)" : ""));
		}

		/* Allow player to "refuse" certain actions */
		else if (!get_item_allow(k))
		{
			/* Nothing */
		}

		else
		{
			/* Accept that choice */
			(*cp) = k;
			item = TRUE;
			done = TRUE;
			p_ptr->command_see = FALSE;
		}
	}

	/* Option to always show a list */
	if (show_lists)
	{
		p_ptr->command_see = TRUE;
	}

	/* Start out in "display" mode */
	if (p_ptr->command_see)
	{
		/* Save screen */
		screen_save();
	}

	/* Repeat until done */
	while (!done)
	{
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < 8; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (op_ptr->window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (op_ptr->window_flag[j] & (PW_EQUIP)) ne++;
		}

		/* Toggle if needed */
		if (((p_ptr->command_wrk == (USE_EQUIP)) && ni && !ne) ||
			 ((p_ptr->command_wrk == (USE_INVEN)) && !ni && ne))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		/* Update */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Redraw windows */
		window_stuff();

		/* Viewing inventory */
		if (p_ptr->command_wrk == (USE_INVEN))
		{
			/* Redraw if needed */
			if (p_ptr->command_see) show_inven();

			/* Begin the prompt */
			sprintf(out_val, "Inven:");

			/* List choices */
			if (i1 <= i2)
			{
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,",
					index_to_label(i1), index_to_label(i2));

				/* Append */
				my_strcat(out_val, tmp_val, sizeof(out_val));
			}

			/* Indicate legality of "toggle" */
			if (use_equip) my_strcat(out_val, " / for Equip,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_floor) my_strcat(out_val, " - for floor,", sizeof(out_val));

			/* Indicate legality of the "target" */
			if (allow_target) my_strcat(out_val, " * for target,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_allies) my_strcat(out_val, " & for allies,", sizeof(out_val));

		}

		/* Viewing equipment */
		else if (p_ptr->command_wrk == (USE_EQUIP))
		{
			/* Redraw if needed */
			if (p_ptr->command_see) show_equip();

			/* Begin the prompt */
			sprintf(out_val, "Equip:");

			/* List choices */
			if (e1 <= e2)
			{
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,",
					index_to_label(e1), index_to_label(e2));

				/* Append */
				my_strcat(out_val, tmp_val, sizeof(out_val));
			}

			/* Indicate legality of "toggle" */
			if (use_inven) my_strcat(out_val, " / for Inven,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_floor) my_strcat(out_val, " - for floor,", sizeof(out_val));

			/* Indicate legality of the "target" */
			if (allow_target) my_strcat(out_val, " * for target,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_allies) my_strcat(out_val, " & for allies,", sizeof(out_val));

		}

		/* Viewing allies */
		else if (p_ptr->command_wrk == (USE_ALLY))
		{
			/* Redraw if needed */
			if (p_ptr->command_see) show_floor(allies_list, allies_num, FALSE);

			/* Begin the prompt */
			sprintf(out_val, "Allies:");

			/* List choices */
			if (a1 <= a2)
			{
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,",
					index_to_label(a1), index_to_label(a2));

				/* Append */
				my_strcat(out_val, tmp_val, sizeof(out_val));
			}


			/* Append */
			if (use_inven) my_strcat(out_val, " / for Inven,", sizeof(out_val));

			/* Append */
			else if (use_equip) my_strcat(out_val, " / for Equip,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_floor) my_strcat(out_val, " - for floor,", sizeof(out_val));
			
			/* Indicate legality of the "target" */
			if (allow_target) my_strcat(out_val, " * for target,", sizeof(out_val));
			
		}

		/* Viewing target */
		else if (p_ptr->command_wrk == (USE_TARGET))
		{
			/* Redraw if needed */
			if (p_ptr->command_see) show_floor(target_list, target_num, FALSE);

			/* Begin the prompt */
			sprintf(out_val, "Target:");

			/* List choices */
			if (t1 <= t2)
			{
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,",
					index_to_label(t1), index_to_label(t2));

				/* Append */
				my_strcat(out_val, tmp_val, sizeof(out_val));
			}

			/* Indicate legality of "toggle" */
			if (use_inven) my_strcat(out_val, " / for Inven,", sizeof(out_val));

			/* Append */
			else if (use_equip) my_strcat(out_val, " / for Equip,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_floor) my_strcat(out_val, " - for floor,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_allies) my_strcat(out_val, " & for allies,", sizeof(out_val));
		}

		/* Viewing floor */
		else
		{
			/* Redraw if needed */
			if (p_ptr->command_see) show_floor(floor_list, floor_num, FALSE);

			/* Begin the prompt */
			sprintf(out_val, "Floor:");

			/* List choices */
			if (f1 <= f2)
			{
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,", I2A(f1), I2A(f2));

				/* Append */
				my_strcat(out_val, tmp_val, sizeof(out_val));
			}

			/* Append */
			if (use_inven) my_strcat(out_val, " / for Inven,", sizeof(out_val));

			/* Append */
			else if (use_equip) my_strcat(out_val, " / for Equip,", sizeof(out_val));
			
			/* Indicate legality of the "target" */
			if (allow_target) my_strcat(out_val, " * for target,", sizeof(out_val));

			/* Indicate legality of the "floor" */
			if (allow_allies) my_strcat(out_val, " & for allies,", sizeof(out_val));
			
		}

		/* Indicate ability to "view" */
		if (!p_ptr->command_see) my_strcat(out_val, " ? to see,", sizeof(out_val));

		/* Indicate legality of the "self" */
		if (allow_self) my_strcat(out_val, " @ for self,", sizeof(out_val));

		/* Indicate legality of the "self" */
		if (use_gold) my_strcat(out_val, " $ for gold,", sizeof(out_val));

		/* Indicate legality of the "feature" */
		if (allow_feats)
		{
			my_strcat(out_val, " . for ", sizeof(out_val));
			my_strcat(out_val,f_name + f_info[cave_feat[p_ptr->py][p_ptr->px]].name, sizeof(out_val));
			my_strcat(out_val,",", sizeof(out_val));
		}

		/* Finish the prompt */
		my_strcat(out_val, " ESC", sizeof(out_val));

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		ke = anykey();

		/* Hack -- apply mouse input as a modifier */
		if (ke.key == '\xff')
		{
			if (p_ptr->command_see)
			{
				int my = ke.mousey;

				if (p_ptr->command_wrk == (USE_INVEN))
				{
					for (i = i1; i <= i2; i++)
					{
						object_type *o_ptr = &inventory[i];

						/* Is this item acceptable? */
						if (!item_tester_okay(o_ptr)) continue;

						/* Is this the line clicked */
						if (--my == 0) ke.key = 'a' + i;
					}
				}
				else if (p_ptr->command_wrk == (USE_EQUIP))
				{
					for (i = e1; i <= e2; i++)
					{
						object_type *o_ptr = &inventory[i];

						/* Is this item acceptable? */
						if (!item_tester_okay(o_ptr)) continue;

						/* Is this the line clicked */
						if (--my == 0) ke.key = 'a' + i - INVEN_WIELD;
					}
				}
				else if (p_ptr->command_wrk == (USE_ALLY))
				{
					for (i = f1; i <= f2; i++)
					{
						object_type *o_ptr = &o_list[allies_list[i]];

						/* Is this item acceptable? */
						if (!item_tester_okay(o_ptr)) continue;

						/* Is this the line clicked */
						if (--my == 0) ke.key = 'a' + i;
					}
				}
				else if (p_ptr->command_wrk == (USE_TARGET))
				{
					for (i = f1; i <= f2; i++)
					{
						object_type *o_ptr = &o_list[target_list[i]];

						/* Is this item acceptable? */
						if (!item_tester_okay(o_ptr)) continue;

						/* Is this the line clicked */
						if (--my == 0) ke.key = 'a' + i;
					}
				}
				else if (p_ptr->command_wrk == (USE_FLOOR))
				{
					for (i = f1; i <= f2; i++)
					{
						object_type *o_ptr = &o_list[floor_list[i]];

						/* Is this item acceptable? */
						if (!item_tester_okay(o_ptr)) continue;

						/* Is this the line clicked */
						if (--my == 0) ke.key = 'a' + i;
					}
				}

				/* Hack -- swap between equip and inven */
				if (ke.key == '\xff')
				{
					if (ke.mousebutton == 1) ke.key = '/';
					else ke.key = '-';
				}
			}
			/* Display the list */
			else
			{
				ke.key = ' ';
			}
		}

		/* Parse it */
		switch (ke.key)
		{
			case ESCAPE:
			{
				done = TRUE;
				break;
			}

			case '?':
			case ' ':
			{
				/* Hide the list */
				if (p_ptr->command_see)
				{
					/* Flip flag */
					p_ptr->command_see = FALSE;

					/* Load screen */
					screen_load();
				}

				/* Show the list */
				else
				{
					/* Save screen */
					screen_save();

					/* Flip flag */
					p_ptr->command_see = TRUE;
				}

				break;
			}

			case '/':
			{
				/* Toggle to inventory */
				if (use_inven && (p_ptr->command_wrk != (USE_INVEN)))
				{
					p_ptr->command_wrk = (USE_INVEN);
				}

				/* Toggle to equipment */
				else if (use_equip && (p_ptr->command_wrk != (USE_EQUIP)))
				{
					p_ptr->command_wrk = (USE_EQUIP);
				}

				/* No toggle allowed */
				else
				{
					bell("Cannot switch item selector!");
					break;
				}

				/* Hack -- Fix screen */
				if (p_ptr->command_see)
				{
					/* Load screen */
					screen_load();

					/* Save screen */
					screen_save();
				}

				/* Need to redraw */
				break;
			}

			case '*':
			{
				/* Paranoia */
				if (!allow_target)
				{
					bell("Cannot select target!");
					break;
				}
#if 0
				/* There is only one item */
				if (target_num == 1)
				{
					/* Auto-Select */
					if (p_ptr->command_wrk == (USE_TARGET))
					{
						/* Special index */
						k = 0 - target_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;

						break;
					}
				}
#endif
				/* Hack -- Fix screen */
				if (p_ptr->command_see)
				{
					/* Load screen */
					screen_load();

					/* Save screen */
					screen_save();
				}

				p_ptr->command_wrk = (USE_TARGET);

				break;
			}
			
			case '&':
			{
				/* Paranoia */
				if (!allow_allies)
				{
					bell("Cannot select allies!");
					break;
				}
#if 0
				/* There is only one item */
				if (floor_num == 1)
				{
					/* Auto-Select */
					if (p_ptr->command_wrk == (USE_FLOOR))
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;

						break;
					}
				}
#endif
				/* Hack -- Fix screen */
				if (p_ptr->command_see)
				{
					/* Load screen */
					screen_load();

					/* Save screen */
					screen_save();
				}

				p_ptr->command_wrk = (USE_ALLY);

				break;
			}
			
			case '-':
			{
				/* Paranoia */
				if (!allow_floor)
				{
					bell("Cannot select floor!");
					break;
				}

				/* There is only one item */
				if (floor_num == 1)
				{
					/* Auto-Select */
					if (p_ptr->command_wrk == (USE_FLOOR))
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;

						break;
					}
				}

				/* Hack -- Fix screen */
				if (p_ptr->command_see)
				{
					/* Load screen */
					screen_load();

					/* Save screen */
					screen_save();
				}

				p_ptr->command_wrk = (USE_FLOOR);

				break;
			}

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				/* Look up the tag */
				if (!get_tag(&k, ke.key))
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (3, tag)" : ""));
					break;
				}

				/* Hack -- Validate the item */
				if ((k < INVEN_WIELD) ? !allow_inven : !allow_equip)
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (4, tag)" : ""));
					break;
				}

				/* Forbid classic equipment if using the quiver */
				if (use_quiver && (k >= INVEN_WIELD) && !IS_QUIVER_SLOT(k))
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (5, tag)" : ""));
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (6, tag)" : ""));
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}

			case '\n':
			case '\r':
			{
				/* Choose "default" inventory item */
				if (p_ptr->command_wrk == (USE_INVEN))
				{
					if (i1 != i2)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (7, default)" : ""));
						break;
					}

					k = i1;
				}

				/* Choose "default" equipment item */
				else if (p_ptr->command_wrk == (USE_EQUIP))
				{
					if (e1 != e2)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (8, default)" : ""));
						break;
					}

					k = e1;
				}

				/* Choose "default" target item */
				else if (p_ptr->command_wrk == (USE_TARGET))
				{
					if (t1 != t2)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (9, default)" : ""));
						break;
					}

					k = 0 - target_list[t1];
				}

				/* Choose "default" ally item */
				else if (p_ptr->command_wrk == (USE_ALLY))
				{
					if (a1 != a2)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (10, default)" : ""));
						break;
					}

					k = 0 - allies_list[a1];
				}

				/* Choose "default" floor item */
				else
				{
					if (f1 != f2)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (11, default)" : ""));
						break;
					}

					k = 0 - floor_list[f1];
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (12, default)" : ""));
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}

			case '.':

				/* Paranoia */
				if (!allow_feats)
				{
					bell("Cannot select feature!");
					break;
				}

				/* Mega-Hack -- we are setting the floor alight with a torch */
				if (use_feath)
				{
					project_one(0, 0, p_ptr->py, p_ptr->px, 1, GF_FIRE, (PROJECT_ITEM | PROJECT_GRID));

					done = TRUE;
					break;
				}

				/* Hack -- pick previous feature - don't validate or allow */
				k = scan_feat(p_ptr->py,p_ptr->px);

				/* Paranoia */
				if (k<0)
				{
					bell("Too many objects for feature!");
					break;
				}

				/* Accept that choice */
				(*cp) = 0 - k;
				item = TRUE;
				done = TRUE;

				break;

			case '@':

				/* Paranoia */
				if (!allow_self)
				{
					bell("Cannot select self!");
					break;
				}

				/* Accept that choice */
				(*cp) = INVEN_SELF;
				item = TRUE;
				done = TRUE;

				break;
				
			case '$':

				/* Paranoia */
				if (!use_gold)
				{
					bell("Cannot select gold!");
					break;
				}

				/* Accept that choice */
				(*cp) = INVEN_GOLD;
				item = TRUE;
				done = TRUE;

				break;
				
			default:
			{
				bool verify;
				int which = ke.key;

				/* Note verify */
				verify = (isupper(which) ? TRUE : FALSE);

				/* Lowercase */
				which = tolower(which);

				/* Convert letter to inventory index */
				if (p_ptr->command_wrk == (USE_INVEN))
				{
					k = label_to_inven(which);

					if (k < 0)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (11, inven)" : ""));
						break;
					}
				}

				/* Convert letter to equipment index */
				else if (p_ptr->command_wrk == (USE_EQUIP))
				{
					k = label_to_equip(which);

					if (k < 0)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (12, equip)" : ""));
						break;
					}

					/* Forbid classic equipment if using the quiver */
					if (use_quiver && !IS_QUIVER_SLOT(k))
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (13, equip)" : ""));
						break;
					}
				}

				/* Convert letter to target index */
				else if (p_ptr->command_wrk == (USE_TARGET))
				{
					k = (islower(which) ? A2I(which) : -1);

					if (k < 0 || k >= target_num)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (14, target)" : ""));
						break;
					}

					/* Special index */
					k = 0 - target_list[k];
				}

				/* Convert letter to floor index */
				else if (p_ptr->command_wrk == (USE_ALLY))
				{
					k = (islower(which) ? A2I(which) : -1);

					if (k < 0 || k >= allies_num)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (15, ally)" : ""));
						break;
					}

					/* Special index */
					k = 0 - allies_list[k];
				}

				/* Convert letter to floor index */
				else
				{
					k = (islower(which) ? A2I(which) : -1);

					if (k < 0 || k >= floor_num)
					{
						bell(format("Illegal object choice%s!", cheat_xtra ? " (16, floor)" : ""));
						break;
					}

					/* Special index */
					k = 0 - floor_list[k];
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell(format("Illegal object choice%s!", cheat_xtra ? " (15, normal)" : ""));
					break;
				}

				/* Verify the item */
				if (verify && !verify_item("Try", k))
				{
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
		}

	}

	/* Remove skin */
	if (use_skin)
	{
		/* Find bare skin */
		for (i = INVEN_LEFT; i <= INVEN_FEET; i++)
		{
			object_type *o_ptr = &inventory[i];

			/* Hack -- Skip light source */
			if (i == INVEN_LITE) continue;

			/* Hack -- Skip item */
			if (i == (*cp)) continue;

			/* Bare skin */
			if (o_ptr->ident & (IDENT_STORE))
			{
				/* Set up skin */
				object_wipe(o_ptr);
			}
		}
	}

	/* Fix the screen if necessary */
	if (p_ptr->command_see)
	{
		/* Load screen */
		screen_load();

		/* Hack -- Cancel "display" */
		p_ptr->command_see = FALSE;
	}

	/* Forget the item restrictions if not picking a bag */
	if (!(item) || ((*cp >= 0) && (inventory[*cp].tval != TV_BAG))
		|| ((*cp < 0) && (o_list[0 - *cp].tval != TV_BAG)))
	{
		/* Forget the item_tester_tval restriction */
		item_tester_tval = 0;

		/* Forget the item_tester_hook restriction */
		item_tester_hook = NULL;
	}

	/* Toggle again if needed */
	if (toggle) toggle_inven_equip();

	/* Update */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Window stuff */
	window_stuff();

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str) msg_print(str);

#ifdef ALLOW_REPEAT

	/* Save item if available */
	if (item) repeat_push(*cp);

#endif /* ALLOW_REPEAT */

	/* Result */
	return (item);
}


/*
 * Create a fake item from the bag arrays
 *
 * FIXME: It's sooo ugly and so fragile; why not just copy the objects?
 * Or if they aren't stored, why not store them somewhere?
 */
void fake_bag_item(object_type *i_ptr, int sval, int slot)
{
	s16b number = 0;
	s16b charges = 0;

	/* Paranoia, probably done in object_prep */
	object_wipe(i_ptr);

	/* Initially no item */
	i_ptr->k_idx = 0;

	assert (sval < SV_BAG_MAX_BAGS && slot < INVEN_BAG_TOTAL);

	/* Get bag kind from lookup kind cache */
	i_ptr->k_idx = bag_kinds_cache[sval][slot];

	/* Paranoia */
	if (!i_ptr->k_idx) return;

	/* Prepare object */
	object_prep(i_ptr, i_ptr->k_idx);

	/* Get bag number */
	number = bag_contents[sval][slot];

	/* Hack -- wand charges */
	if (i_ptr->tval == TV_WAND)
	{
		charges = bag_contents[sval + 1][slot];

		/* Real object */
		if (number)
		{
			/* Get bag charges */
			i_ptr->charges = charges / number;

			/* Round up */
			if (charges % number) i_ptr->charges++;
		}
	}
	/* Hack -- torch charges */
	else if ((i_ptr->tval == TV_LITE) && (i_ptr->sval == SV_LITE_TORCH))
	{
		charges = bag_contents[sval][slot];

		/* Fake bag number */
		number = charges / FUEL_TORCH;

		/* Round up */
		if (charges % FUEL_TORCH) number++;

		/* Real object */
		if (number)
		{
			/* Get bag charges */
			i_ptr->charges = charges / number;

			/* Round up */
			if (charges % number) i_ptr->charges++;
		}
	}

	/* Hack -- limit total number */
	if (number > 99)
	{
		/* Maximum number */
		i_ptr->number = 99;

		/* Set stack counter */
		i_ptr->stackc = 98;
	}
	else
	{
		/* Normal number */
		i_ptr->number = number;

		/* Real object */
		if (number)
		{
			/* Set stack counter */
			i_ptr->stackc = i_ptr->charges % number;
		}
	}

	/* Awareness always gives full knowledge */
	if (object_aware_p(i_ptr))
	{
		/* Hack -- Don't use object_aware() and object_known() due to performance problems */
		i_ptr->ident |= (IDENT_KNOWN);

		/* Add usage information */
		i_ptr->usage = k_info[i_ptr->k_idx].used;

		/* Auto-inscribe */
		if (!i_ptr->note) i_ptr->note = k_info[i_ptr->k_idx].note;

		/* Apply obvious flags, e.g. for throwing items. XXX
		 * This is, of course, *really dangerous* due to the calls in object_obvious_flags */
		/* object_obvious_flags(i_ptr); */

		/* Hack -- instead apply kind flags */
		i_ptr->can_flags1 = k_info[i_ptr->k_idx].flags1;
		i_ptr->can_flags2 = k_info[i_ptr->k_idx].flags2;
		i_ptr->can_flags3 = k_info[i_ptr->k_idx].flags3;
		i_ptr->can_flags4 = k_info[i_ptr->k_idx].flags4;
	}
}


/*
 * Get item from a bag.
 *
 * See tables.c for a description of how bags function.
 *
 * XXX This whole routine plays nasty tricks with inventory in order
 * to re-use the code from the get_item routine above.
 *
 * XXX We use an in-function version of inven_carry to ensure that
 * we don't re-absorb the new item into the bag we took it from.
 */
bool get_item_from_bag(int *cp, cptr pmt, cptr str, object_type *o_ptr)
{
	bool cancel = FALSE;

	int item;

	int i, j, k;

	object_type *i_ptr;

	object_type *inventory_old;
	object_type inventory_fake[INVEN_TOTAL];

	char str_buf[160];

	/* Paranoia */
	if (o_ptr->sval >= SV_BAG_MAX_BAGS) return (FALSE);

	/* Initialise fake inventory with objects */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		i_ptr = &inventory_fake[i];

		/* Empty slot */
		if ((i >= INVEN_BAG_TOTAL) || !(bag_holds[o_ptr->sval][i][0]) || !(bag_contents[o_ptr->sval][i]))
		{
			i_ptr->k_idx = 0;
			continue;
		}

		/* Fake the item */
		fake_bag_item(i_ptr, o_ptr->sval, i);
	}

	/* Save inventory */
	inventory_old = inventory;

	/* Switch to fake inventory */
	inventory = inventory_fake;

	/* Hack -- modify failure string */
	my_strcpy(str_buf, str, sizeof(str_buf));

	/* Hack -- remove trailing full stop */
	if (str_buf[strlen(str_buf)-1] == '.') str_buf[strlen(str_buf)-1] = '\0';

	/* Hack -- append */
	my_strcat(str_buf, " inside the bag.", sizeof(str_buf));

	/* Get an item */
	if (!get_item(&item, pmt, str_buf, (USE_INVEN))) cancel = TRUE;

	/* Restore inventory */
	inventory = inventory_old;

	/* Cancelled? */
	if (cancel) {
	  /* Window stuff */
	  p_ptr->window |= (PW_INVEN | PW_EQUIP);

	  /* Redraw stuff */
	  p_ptr->redraw |= (PR_ITEM_LIST);

	  return (FALSE);
	}

	/* Get the item */
	i_ptr = &inventory_fake[item];

	/* Paranoia */
	if (p_ptr->inven_cnt > INVEN_PACK) {
	  /* Window stuff */
	  p_ptr->window |= (PW_INVEN | PW_EQUIP);

	  /* Redraw stuff */
	  p_ptr->redraw |= (PR_ITEM_LIST);

	  return (FALSE);
	}

	/* Reduce bag contents - wands hack */
	if (i_ptr->tval == TV_WAND)
	{
		bag_contents[o_ptr->sval][item] -= i_ptr->number;
		bag_contents[o_ptr->sval+1][item] -= i_ptr->charges * i_ptr->number - i_ptr->stackc;
	}
	/* Reduce bag contents - torches hack */
	else if ((i_ptr->tval == TV_LITE) && (i_ptr->sval == SV_LITE_TORCH))
	{
		bag_contents[o_ptr->sval][item] -= i_ptr->charges * i_ptr->number - i_ptr->stackc;
	}
	/* Reduce bag contents */
	else
	{
		bag_contents[o_ptr->sval][item] -= i_ptr->number;
	}

	/* Find an empty slot */
	for (j = 0; j <= INVEN_PACK; j++) if (!inventory[j].k_idx) break;

	/* Use that slot */
	i = j;

	/* Copy the item */
	object_copy(&inventory[i], i_ptr);

	/* Get the new object */
	i_ptr = &inventory[i];

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
	if (j < SHOWN_TOTAL) i_ptr->show_idx = j;
	else i_ptr->show_idx = 0;

	/* Increase the weight */
	p_ptr->total_weight += (i_ptr->number * i_ptr->weight);

	/* Count the items */
	p_ptr->inven_cnt++;

	/* Set the slot */
	*cp = i;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine and Reorder pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_ITEM_LIST);

	/* Successful */
	return (TRUE);
}
