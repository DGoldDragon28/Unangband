/* File: spells2.c */

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
 * Apply a "project()" directly to all monsters/objects/grids.
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 *
 * PROJECT_PANEL limits the monsters/objects/grids to those within 2 * MAX_SIGHT
 * PROJECT_ALL_IN_LOS limits monsters/objects/grids to those within line of fire and/or
 *  line of sight if PROJECT_LOS is enabled.
 */
bool project_dist(int who, int what, int y0, int x0, int dam, int typ, u32b flg, u32b flg2)
{
	int i, x, y;

	bool notice = FALSE;

	bool player_hack = FALSE;
	
	/* Projection can also pass through line of sight grids */
	bool allow_los = (flg & (PROJECT_LOS)) != 0;

	/* Hack - for efficiency we process monsters separately. However, if affecting the
	 * player's field of view/fire, this is only efficient if the maximum number of monsters
	 * is smaller than the field of view/fire. */
	bool m_max_hack = ((allow_los) ? (m_max < view_n) : (m_max < fire_n));

	/* Origin is player */
	if ((flg2 & (PR2_ALL_IN_LOS)) && (y0 == p_ptr->py) && (x0 == p_ptr->px)) player_hack = TRUE;

	/* Affect player? */
	if ((player_hack) || (((flg2 & (PR2_ALL_IN_LOS)) == 0) && (((flg2 & (PR2_PANEL)) == 0) || distance(y0, x0, p_ptr->py, p_ptr->px) < 2 * MAX_SIGHT)) ||
			(generic_los(y0, x0, p_ptr->py, p_ptr->px, CAVE_XLOF)) ||
			((allow_los) && generic_los(y0, x0, p_ptr->py, p_ptr->px, CAVE_XLOS)))
	{
		/* Jump directly to the player */
		if (project(who, what, 0, 0, p_ptr->py, p_ptr->px, p_ptr->py, p_ptr->px, dam, typ, flg, 0, 10)) notice = TRUE;
	}

	/* Affect all (nearby) monsters. */
	if ((flg & (PROJECT_KILL)) && ((!player_hack) || (m_max_hack)))
		for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only affect nearby monsters */
		if ((flg2 & (PR2_PANEL)) && (distance(y0, x0, y, x) > 2 * MAX_SIGHT)) continue;

		/* Require line of fire - from player */
		if (player_hack)
		{
			if ((!player_can_fire_bold(y, x)) &&
				((!allow_los) || !player_has_los_bold(y, x))) continue;
		}
		/* Require line of fire - from origin */
		else if (flg2 & (PR2_ALL_IN_LOS))
		{
			if ((!generic_los(y0, x0, y, x, CAVE_XLOF)) &&
				((!allow_los) || !generic_los(y0, x0, y, x, CAVE_XLOS))) continue;
		}

		/* Jump directly to the target monster */
		if (project(who, what, 0, 0, y, x, y, x, dam, typ, flg, 0, 10)) notice = TRUE;
	}
	
	/* Affect all (nearby) objects & grids */
	if (flg & (PROJECT_ITEM | PROJECT_GRID))
	{
		/* Affect line of sight/line of fire - origin is player and restricting to LOS/LOF */
		if (player_hack)
		{
			if (allow_los) for (i = 0; i < view_n; i++)
			{
				/* Get grid */
				y = GRID_Y(view_g[i]);
				x = GRID_X(view_g[i]);
				
				/* Already affected grids with monsters? */
				if (((flg & (PROJECT_KILL)) != 0) && (cave_m_idx[y][x] > 0) && (m_max_hack)) continue;
				
				/* Already affected grids with player */
				if (cave_m_idx[y][x] < 0) continue;
				
				/* Affect grids with objects, or all grids */
				if (((flg & (PROJECT_GRID)) != 0) || (cave_o_idx[y][x]))
				{
					if (project(who, what, 0, 0, y, x, y, x, dam, typ, flg, 0, 10)) notice = TRUE;
				}
			}
			
			for (i = 0; i < fire_n; i++)
			{
				/* Get grid */
				y = GRID_Y(fire_g[i]);
				x = GRID_X(fire_g[i]);
				
				/* Already affected grids in view */
				if (allow_los && ((play_info[y][x] & (PLAY_VIEW)) != 0)) continue;
				
				/* Already affected grids with monsters? */
				if (((flg & (PROJECT_KILL)) != 0) && (cave_m_idx[y][x] > 0) && (m_max_hack)) continue;
				
				/* Already affected grids with player */
				if (cave_m_idx[y][x] < 0) continue;
				
				/* Affect grids with objects, or all grids */
				if (((flg & (PROJECT_GRID)) != 0) || (cave_o_idx[y][x]))
				{
					if (project(who, what, 0, 0, y, x, y, x, dam, typ, flg, 0, 10)) notice = TRUE;
				}
			}
		}
		/* Affect panel/level */
		else
		{
			int r = flg2 & (PR2_PANEL) ? MAX_SIGHT * 2 : MAX(DUNGEON_WID, DUNGEON_HGT);
			
			for (y = y0 - r; y <= y0 + r; y++)
			{
				for (x = x0 - r; x <= x0 + r; x++)
				{
					/* Inbounds */
					if (!in_bounds_fully(y, x)) continue;
					
					/* Already affected grids with monsters */
					if (((flg & (PROJECT_KILL)) != 0) && (cave_m_idx[y][x] > 0)) continue;
					
					/* Already affected grids with player */
					if (cave_m_idx[y][x] < 0) continue;
					
					/* Only affect nearby monsters */
					if ((flg2 & (PR2_PANEL)) && (distance(y0, x0, y, x) > 2 * MAX_SIGHT)) continue;

					/* Affect grids with objects, or all grids */
					if (((flg & (PROJECT_GRID)) != 0) || (cave_o_idx[y][x]))
					{
						if (project(who, what, 0, 0, y, x, y, x, dam, typ, flg, 0, 10)) notice = TRUE;
					}
				}
			}
		}
	}

	/* Result */
	return (notice);
}


/*
 * Generates the familiar attributes
 */
void generate_familiar(void)
{
	int i;
	int blow = 0;
	int size = 1;
	monster_race *r_ptr = &r_info[FAMILIAR_IDX];

	COPY(r_ptr, &r_info[FAMILIAR_BASE_IDX], monster_race);

	/* Hack - familiars don't level like regular monsters */
	r_ptr->flags9 &= ~(RF9_LEVEL_CLASS | RF9_LEVEL_SPEED | RF9_LEVEL_POWER | RF9_LEVEL_AGE | RF9_LEVEL_SIZE);

	/* Familiar not yet defined */
	if (!p_ptr->familiar) return;

	/* Generate the familiar attributes */
	for (i = 0; p_ptr->familiar_attr[i]; i++)
	{
		int attr = p_ptr->familiar_attr[i];

		/* Hack -- add a flag */
		if (attr < FAMILIAR_AC)
		{
			if (attr < 32) r_ptr->flags1 |= (1 << attr);
			else if (attr < 64) r_ptr->flags2 |= (1 << (attr - 32));
			else if (attr < 96) r_ptr->flags3 |= (1 << (attr - 64));
			else if (attr < 128) { r_ptr->flags4 |= (1 << (attr - 96)); r_ptr->freq_innate += 10; }
			else if (attr < 160) { r_ptr->flags5 |= (1 << (attr - 128)); r_ptr->freq_spell += 15;}
			else if (attr < 192) { r_ptr->flags6 |= (1 << (attr - 160)); r_ptr->freq_spell += 15;}
			else if (attr < 224) { r_ptr->flags7 |= (1 << (attr - 192)); r_ptr->freq_spell += 15;}
			else if (attr < 256) r_ptr->flags8 |= (1 << (attr - 224));
			else r_ptr->flags9 |= (1 << (attr - 256));

			/* Hack -- huge familiars lose evasion, gain lots of size */
			if (attr == 73) { r_ptr->flags9 &= ~(RF9_EVASIVE); size += 3; }
			
			/* Hack -- monsters which gain permanent invisibility, lose temporary invisibility */
			if (attr == 36) r_ptr->flags6 &= ~(RF6_INVIS);
			
			/* Hack -- ensure minimal mana */
			if ((attr >= 96) && (attr <= 224)) r_ptr->mana += method_info[attr].mana_cost;
		}
		/* Add other attributes */
		else
		{
			switch(p_ptr->familiar_attr[i])
			{
				case FAMILIAR_AC:
					r_ptr->ac += 25;
					break;
				case FAMILIAR_SPEED:
					r_ptr->speed += 10;
					break;
				case FAMILIAR_POWER:
					r_ptr->spell_power += p_ptr->max_lev;
					break;
				case FAMILIAR_HP:
					r_ptr->hp += 2;
					break;
				case FAMILIAR_VISION:
					r_ptr->aaf *= 2;
					break;
				case FAMILIAR_SIZE:
					r_ptr->hp += 5;
					size++;
					break;
				case FAMILIAR_MANA:
					r_ptr->mana += 10;
					break;
				case FAMILIAR_BLOW:
					if (blow < 4)
					{
						r_ptr->blow[blow].d_dice = 1;
						r_ptr->blow[blow].d_side = p_ptr->max_lev / 3;
						r_ptr->blow[blow].method = familiar_race[p_ptr->familiar].method;
						r_ptr->blow[blow++].effect = GF_HURT;
					}
					r_ptr->flags1 &= ~(RF1_NEVER_BLOW);
					break;
				case FAMILIAR_SPIKE:
				case FAMILIAR_SHOT:
					/* Paranoia */
					if (blow)
					{
						r_ptr->blow[blow-1].method = p_ptr->familiar_attr[i] == FAMILIAR_SPIKE ? RBM_SPIKE : RBM_SHOT;
						r_ptr->flags4 |= (1 << (blow-1));
						r_ptr->freq_innate += 25;
						
						/* Spikes are ranged only */
						if (blow == 1) r_ptr->flags1 |= (RF1_NEVER_BLOW);
					}
					break;
				default:
					/* Paranoia */
					if (blow)
					{
						r_ptr->blow[blow-1].effect = p_ptr->familiar_attr[i] - FAMILIAR_BLOW;
					}
					break;
			}
		}
	}

	/* Fix up blow damage based on size */
	for (i = 0; i < blow; i++)
	{
		r_ptr->blow[blow].d_dice *= size;
	}

	/* No blows - mark appropriately */
	if (!blow)
	{
		r_ptr->flags1 |= (RF1_NEVER_BLOW);
	}

	/* Fix up speed based on size */
	r_ptr->speed = 100 + (r_ptr->speed - 100) / size;

	/* Increase hit points at every level */
	r_ptr->hp *= p_ptr->max_lev;
	
	/* Huge monsters get more hp to compensate for loss of evasion */
	if (r_ptr->flags3 & (RF3_HUGE)) r_ptr->hp *= 3;

	/* Increase mana somewhat */
	if (p_ptr->max_lev > 15) r_ptr->mana += r_ptr->mana * p_ptr->max_lev / 15;

	/* Fix up attack frequencies */
	if (r_ptr->freq_innate > 90) r_ptr->freq_innate = 90;
	if (r_ptr->freq_spell > 90) r_ptr->freq_spell = 90;

	/* Set level for blood debt */
	r_ptr->level = p_ptr->max_lev;
}



/*
 * Increase players hit points, notice effects
 */
bool hp_player(int num)
{
	/* Healing needed */
	if (p_ptr->chp < p_ptr->mhp)
	{
		/* Gain hitpoints */
		p_ptr->chp += num;

		/* Enforce maximum */
		if (p_ptr->chp >= p_ptr->mhp)
		{
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1 | PW_PLAYER_2 | PW_PLAYER_3);

		/* Heal 0-4 */
		if (num < 5)
		{
			msg_print("You feel a little better.");
		}

		/* Heal 5-14 */
		else if (num < 15)
		{
			msg_print("You feel better.");
		}

		/* Heal 15-34 */
		else if (num < 35)
		{
			msg_print("You feel much better.");
		}

		/* Heal 35+ */
		else
		{
			msg_print("You feel very good.");
		}

		/* Notice */
		return (TRUE);
	}

	/* Ignore */
	return (FALSE);
}



/*
 * Leave a "glyph of warding" which prevents monster movement
 */
void warding_glyph(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
		msg_print("The object resists the spell.");
		return;
	}

	/* Create a glyph */
	cave_set_feat(py, px, FEAT_GLYPH);
}


/*
 * Leave a "trap" which affects those moving on it
 */
void warding_trap(int feat, int dir)
{
	int ty = p_ptr->py + ddy[dir];
	int tx = p_ptr->px + ddx[dir];

	/* XXX XXX XXX */
	if (!cave_clean_bold(ty, tx))
	{
		msg_print("The object resists the spell.");
		return;
	}

	/* Create a trap */
	cave_set_feat(ty, tx, feat);
}


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] =
{
	"strong",
	"smart",
	"wise",
	"dextrous",
	"healthy",
	"cute",
	"agile",
	"big"
};


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] =
{
	"weak",
	"stupid",
	"naive",
	"clumsy",
	"sickly",
	"ugly",
	"sluggish",
	"small"
};


/*
 * Lose a "point"
 */
int do_dec_stat(int stat)
{
	/* Sustain */
	if (p_ptr->cur_flags2 & (TR2_SUST_STR << (stat == A_AGI ? A_DEX : (stat == A_SIZ ? A_STR : stat))))
	{
		/* Message */
		msg_format("You feel very %s for a moment, but the feeling passes.",
			   desc_stat_neg[stat]);

		/* Always notice */
		equip_can_flags(0x0L, TR2_SUST_STR << (stat == A_AGI ? A_DEX : (stat == A_SIZ ? A_STR : stat)), 0x0L, 0x0L);

		/* Notice effect */
		return -1;
	}

	/* Attempt to reduce the stat */
	if (dec_stat(stat, 10))
	{
		/* Message */
		msg_format("You feel very %s.", desc_stat_neg[stat]);

		/* Always notice */
		equip_not_flags(0x0L, TR2_SUST_STR << (stat == A_AGI ? A_DEX : (stat == A_SIZ ? A_STR : stat)), 0x0L, 0x0L);

		/* Notice effect */
		return 1;
	}

	/* Nothing obvious */
	return 0;
}


/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(int stat)
{
	/* Attempt to increase */
	if (res_stat(stat))
	{
		/* Message */
		msg_format("You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(int stat)
{
	/* Restore stat */
	res_stat(stat);

	/* Increase stat */
	inc_stat(stat);

	/* Message */
	msg_format("You feel very %s!", desc_stat_pos[stat]);

	/* Notice */
	return (TRUE);
}

/*
 * Identify everything being carried.
 */
void identify_pack(void)
{
	int i;

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Aware and Known */
		object_aware(o_ptr, FALSE);
		object_known(o_ptr);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
}






/*
 * Used by the "enchant" function (chance of failure)
 */
static int enchant_table[16] =
{
	0, 10,  50, 100, 200,
	300, 400, 500, 700, 950,
	990, 992, 995, 997, 999,
	1000
};


/*
 * Hack -- Removes curse from an object.
 */
void uncurse_object(object_type *o_ptr)
{
	/* Uncurse it */
	o_ptr->ident &= ~(IDENT_CURSED);

	/* Break it */
	o_ptr->ident |= (IDENT_BROKEN);

	/* Take note */
	o_ptr->feeling = INSCRIP_UNCURSED;

	/* The object has been "sensed" */
	o_ptr->ident |= (IDENT_SENSE);
}


/*
 * Removes curses from items in inventory.
 *
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 */
static int remove_curse_aux(int all)
{
	int i, cnt = 0;

	/* Attempt to uncurse items being worn */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		u32b f1, f2, f3, f4;

		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Uncursed already */
		if (!cursed_p(o_ptr)) continue;

		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* Heavily Cursed Items need a special spell */
		if (!all && (f3 & (TR3_HEAVY_CURSE)))
		{
			/* Learn about the object */
			object_can_flags(o_ptr,0x0L,0x0L,TR3_HEAVY_CURSE,0x0L, FALSE);

			continue;
		}

		/* Perma-Cursed Items can NEVER be uncursed */
		if (f3 & (TR3_PERMA_CURSE))
		{
			/* Learn about the object */
			if (all) object_can_flags(o_ptr,0x0L,0x0L,TR3_PERMA_CURSE,0x0L, FALSE);

			continue;
		}

		/* Learn about the object */
		if (!all) object_not_flags(o_ptr,0x0L,0x0L,TR3_HEAVY_CURSE,0x0L, FALSE);

		/* Learn about the object */
		object_not_flags(o_ptr,0x0L,0x0L,TR3_PERMA_CURSE,0x0L, FALSE);

		/* Uncurse the object */
		uncurse_object(o_ptr);

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Count the uncursings */
		cnt++;
	}

	/* Return "something uncursed" */
	return (cnt);
}


/*
 * Remove most curses
 */
bool remove_curse(void)
{
	return (remove_curse_aux(FALSE));
}

/*
 * Remove all curses
 */
bool remove_all_curse(void)
{
	return (remove_curse_aux(TRUE));
}



/*
 * Restores any drained experience
 */
bool restore_level(void)
{
	/* Restore experience */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Message */
		msg_print("You feel your life energies returning.");

		/* Restore the experience */
		p_ptr->exp = p_ptr->max_exp;

		/* Check the experience */
		check_experience();

		/* Did something */
		return (TRUE);
	}

	/* No effect */
	return (FALSE);
}


/*
 *  Prints the diseases that the player is afflicted with or cured of.
 */
bool disease_desc(char *desc, size_t desc_s, u32b old_disease, u32b new_disease)
{
	cptr vp[64];

	int i, n, vn;

	u32b disease = 0x0L;

	/* Getting worse */
	if ((disease = (new_disease & ~(old_disease))))
	{
		my_strcpy(desc,"You are afflicted with ", desc_s);
	}
	/* Getting better */
	else if ((disease = (old_disease & ~(new_disease))))
	{
		my_strcpy(desc,"You are cured of ", desc_s);
	}
	/* No change */
	else
	{
		return(FALSE);
	}

	/* Collect symptoms */
	vn = 0;
	for (i = 1, n = 0; n < DISEASE_TYPES_HEAVY; i <<= 1, n++)
	{
		if ((disease & i) != 0) vp[vn++] = disease_name[n];
	}

	/* Scan */
	for (n = 0; n < vn; n++)
	{
		/* Intro */
		if (n == 0) { }
		else if (n < vn-1) my_strcat(desc,", ", desc_s);
		else my_strcat(desc," and ", desc_s);

		/* Dump */
		my_strcat(desc,vp[n], desc_s);
	}

	/* Collect causes */
	vn = 0;
	for (i = (1 << DISEASE_TYPES_HEAVY), n = DISEASE_TYPES_HEAVY; n < 32; i <<= 1, n++)
	{
		if ((disease & i) != 0) vp[vn++] = disease_name[n];
	}

	/* Scan */
	for (n = 0; n < vn; n++)
	{
		/* Intro */
		if (n == 0) { if ((disease & ((1 << DISEASE_TYPES_HEAVY) -1)) != 0) my_strcat(desc, " caused by ", desc_s); }
		else if (n < vn-1) my_strcat(desc,", ", desc_s);
		else my_strcat(desc," and ", desc_s);

		/* Dump */
		my_strcat(desc,vp[n], desc_s);
	}

	/* Dump */
	my_strcat(desc,".", desc_s);

	return(TRUE);
}


/*
 * Hack -- acquire self knowledge
 *
 * List various information about the player and/or his current equipment.
 *
 * Random set to true means we exclude information that appears elsewhere
 * in the character dump.
 */
void self_knowledge_aux(bool spoil, bool random)
{
	int i, n;

	u32b f1 = 0L, f2 = 0L, f3 = 0L, f4 = 0L;

	u32b t1, t2, t3, t4;

	object_type *o_ptr;

	cptr vp[64];

	int vn;

	bool healthy = TRUE;

	for (i = 0; i < TMD_MAX; i++)
	{
		if (p_ptr->timed[i])
		{
			text_out(format("%s  ", timed_effects[i].self_knowledge));

			/* Hack - treat everything as bad */
			healthy = FALSE;
		}
	}

	if (p_ptr->disease)
	{
		char output[1024];

		healthy = FALSE;

		/* Describe diseases */
		if (disease_desc(output, sizeof(output), 0, p_ptr->disease))
		{
			text_out(format("%s  ", output));
		}

		/* Collect changes */
		vn = 0;

		/* Describe changes */
		if (p_ptr->disease & (DISEASE_DISEASE)) vp[vn++] = "mutate";
		if (p_ptr->disease & (DISEASE_POWER)) vp[vn++] = "hatch into a monster";
		if (p_ptr->disease & (1 << DISEASE_SPECIAL)) vp[vn++] = "progressively worsen";
		if (p_ptr->disease & (DISEASE_LIGHT)) vp[vn++] = "subside naturally";

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) { text_out("It will "); }
			else if (n < vn-1) text_out(", ");
			else text_out(" and ");

			/* Dump */
			text_out(vp[n]);
		}

		/* Re-introduce? */
		if (!n) text_out("It ");
		else text_out(" and ");

		/* Permanent disease */
		if (p_ptr->disease & (DISEASE_PERMANENT)) text_out("cannot be cured");
		else
		{
			/* Light diseases are easily treated */
			if (p_ptr->disease & (DISEASE_LIGHT)) text_out("can be easily treated");
			else if (p_ptr->disease & (1 << DISEASE_SPECIAL)) text_out("can only be treated");
			else text_out("must be treated");

			/* Collect cures */
			vn = 0;

			/* Hints as to cure */
			if ((p_ptr->disease & (DISEASE_DISEASE | DISEASE_LIGHT | (1 << DISEASE_SPECIAL)))
				|| (p_ptr->disease < (DISEASE_DISEASE))) vp[vn++] = "the appropriate remedy";
			if (p_ptr->disease & (DISEASE_QUICK)) vp[vn++] = "destroying the parasite in your body";
			if (p_ptr->disease & (DISEASE_HEAVY)) vp[vn++] = "powerful healing magic";
			if (p_ptr->disease & (DISEASE_POWER)) vp[vn++] = "removing the powerful curse";

			/* Hack -- special disease */
			if (p_ptr->disease & (1 << DISEASE_SPECIAL)) vn = 1;

			/* Scan */
			for (n = 0; n < vn; n++)
			{
				/* Intro */
				if (!n) text_out(" by ");
				else if (n < vn-1) text_out(", by ");
				else if (p_ptr->disease & (DISEASE_LIGHT)) text_out(" or by ");
				else text_out(" and by ");

				/* Dump */
				text_out(vp[n]);
			}
		}

		/* Can cure by treating symptoms */
		if (!(p_ptr->disease & (DISEASE_PERMANENT | (1 << DISEASE_SPECIAL))))
		{
			text_out(", or you can treat the symptoms ");
			if (p_ptr->disease & (DISEASE_HEAVY)) text_out("for a temporary respite");
			else text_out("to cure the disease");
		}

		/* Dump */
		text_out(".  ");
	}

	if (healthy)
	{
		text_out("You suffer from no afflictions.  ");
	}

	if (p_ptr->climbing)
	{
		text_out("You are climbing over an obstacle.  ");
	}
	if (p_ptr->searching)
	{
		text_out("You are looking around very carefully.  ");
	}
	if (p_ptr->sneaking)
	{
		text_out(format("You are sneaking to avoid disturbing sleeping monsters%s.  ",
				p_ptr->not_sneaking ? " but your last action was noisy" : ""));
	}
	if (p_ptr->new_spells)
	{
		text_out("You can learn some spells/prayers.  ");
	}

	if (p_ptr->word_recall)
	{
		text_out("You will soon be recalled.  ");
	}

	if (p_ptr->word_return)
	{
		text_out("You will soon be returned to a nearby location.  ");
	}

	/* Get player flags */
	player_flags(&t1,&t2,&t3,&t4);

	/* Hack -- race / shape effects */
	if (!random)
	{
		bool intro = FALSE;

		object_flags(&inventory[INVEN_SELF], &f1, &f2, &f3, &f4);

		/* Hack -- shape flags */
		if ((!random) && (f1 || f2 || f3 || f4))
		{
			if (!intro) text_out(format("Your %s affects you.  ", p_ptr->prace != p_ptr->pshape ? "shape" : "race"));

			intro = TRUE;

			describe_shape(p_ptr->pshape, random);
		}

		/* Intro? */
		if (intro) text_out("\n");

		/* Eliminate shape flags */
		t1 &= ~(f1);
		t2 &= ~(f2);
		t3 &= ~(f3);
		t4 &= ~(f4);

		/* Clear the flags */
		f1 = f2 = f3 = f4 = 0L;
	}

	/* Hack -- class effects */
	if ((!random) && (t1 || t2 || t3 || t4))
	{
		text_out("Your training affects you.  ");

		list_object_flags(t1,t2,t3,t4,1, 1);

		text_out("\n");
	}

	if (random) goto remainder;

	/* Get item flags from equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Clear the flags */
		t1 = t2 = t3 = t4 = 0L;

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Extract the flags */
		if (spoil) object_flags(o_ptr, &t1, &t2, &t3, &t4);
		else object_flags_known(o_ptr, &t1, &t2, &t3, &t4);

		/* Extract flags */
		f1 |= t1 & ~(TR1_IGNORE_FLAGS);
		f2 |= t2 & ~(TR2_IGNORE_FLAGS);
		f3 |= t3 & ~(TR3_IGNORE_FLAGS);
		f4 |= t4 & ~(TR4_IGNORE_FLAGS);
	}

	/* Hack -- other equipment effects */
	if ((f1) || (f2) || (f3) || (f4))
	{
		text_out("Your equipment affects you.  ");

		list_object_flags(f1,f2,f3,f4,0, 1);

		if (spoil)
		{
			equip_can_flags(f1,f2,f3,f4);

			equip_not_flags(~(f1 | TR1_IGNORE_FLAGS),~(f2 | TR2_IGNORE_FLAGS),~(f3 | TR3_IGNORE_FLAGS), ~(f4 | TR4_IGNORE_FLAGS));
		}

		text_out("\n");
	}

	o_ptr = &inventory[INVEN_WIELD];

	if (o_ptr->k_idx)
	{
		/* Clear the flags */
		t1 = t2 = t3 = t4 = 0L;

		/* Extract the flags */
		if (spoil) object_flags(o_ptr, &t1, &t2, &t3, &t4);
		else object_flags_known(o_ptr, &t1, &t2, &t3, &t4);

		/* Extract weapon flags */
		t1 = t1 & (TR1_WEAPON_FLAGS);
		t2 = t2 & (TR2_WEAPON_FLAGS);
		t3 = t3 & (TR3_WEAPON_FLAGS);
		t4 = t4 & (TR4_WEAPON_FLAGS);

		/* Hack -- weapon effects */
		if (t1 || t2 || t3 || t4)
		{
			text_out("Your weapon has special powers.  ");

			list_object_flags(t1,t2,t3,t4,o_ptr->pval, 1);

			if (spoil)
			{
				object_can_flags(o_ptr,t1,t2,t3,t4, FALSE);

				object_not_flags(o_ptr,TR1_WEAPON_FLAGS & ~(t1),TR2_WEAPON_FLAGS & ~(t2),TR3_WEAPON_FLAGS & ~(t3), TR4_WEAPON_FLAGS & ~(t4), FALSE);
			}

			text_out("\n");
		}

	}

remainder:
	/* Clear the flags */
	t1 = t2 = t3 = t4 = 0L;

	/* Collect may flags */
	for (n = 0; n < INVEN_TOTAL; n++)
	{
		o_ptr = &inventory[n];

		t1 |= o_ptr->may_flags1;
		t2 |= o_ptr->may_flags2;
		t3 |= o_ptr->may_flags3;
		t4 |= o_ptr->may_flags4;
	}

	/* Hack -- weapon effects */
	if (t1 || t2 || t3 || t4)
	{
		text_out("You are carrying equipment that you have not fully identified.  ");

		list_object_flags(t1,t2,t3,t4,0, 1);

		text_out("\n");
	}

}


/*
 * Hack -- acquire self knowledge
 *
 * List various information about the player and/or his current equipment.
 *
 * Just prepares for the auxiliary routine above.
 */
void self_knowledge(bool spoil)
{
	/* Save screen */
	screen_save();

	/* Clear the screen */
	Term_clear();

	/* Set text_out hook */
	text_out_hook = text_out_to_screen;

	/* Head the screen */
	text_out_c(TERM_L_BLUE, "Self-knowledge\n");

	/* Really do self-knowledge */
	self_knowledge_aux(spoil, FALSE);

	(void)anykey();

	/* Load screen */
	screen_load();
}






/*
 * Forget everything. This really sucks now.
 */
bool lose_all_info(void)
{
	int i;

	/* Forget info about objects */
	for (i = 0; i < INVEN_TOTAL+1; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Allow "protection" by the MENTAL flag */
		if (o_ptr->ident & (IDENT_MENTAL)) continue;

		/* Remove special inscription, if any */
		o_ptr->feeling = 0;

		/* Hack -- Clear the "felt" flag */
		o_ptr->ident &= ~(IDENT_SENSE);

		/* Hack -- Clear the "bonus" flag */
		o_ptr->ident &= ~(IDENT_BONUS);

		/* Hack -- Clear the "known" flag */
		o_ptr->ident &= ~(IDENT_KNOWN);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Mega-Hack -- Forget the map */
	wiz_dark();

	/* It worked */
	return (TRUE);
}



/*
 *  Set word of recall as appropriate
 */
static bool set_recall(void)
{
	/* Ironman */
	if (adult_ironman && !p_ptr->total_winner)
	{
		msg_print("Nothing happens.");
		return (FALSE);
	}
	
	/* Nothing to recall to */
	if (min_depth(p_ptr->dungeon) == max_depth(p_ptr->dungeon))
	{
		msg_print("Nothing happens.");
		return (FALSE);
	}

	/* Activate recall */
	if (!p_ptr->word_recall)
	{
		if (!get_check("The air starts crackling. Are you sure you want to continue? "))
		{
		    msg_print("A sudden discharge fills the air with a strange smell...");
		    return (FALSE);
		}

		/* Save the old dungeon in case something goes wrong */
		if (autosave_backup)
			do_cmd_save_bkp();

		/* Reset recall depth */
		if (p_ptr->depth > min_depth(p_ptr->dungeon)
			 && p_ptr->depth != t_info[p_ptr->dungeon].attained_depth)
		{
			 if (get_check("Reset recall depth? "))
				 t_info[p_ptr->dungeon].attained_depth = p_ptr->depth;
		}

		p_ptr->word_recall = (s16b)rand_int(20) + 15;
		msg_print("The air about you becomes charged...");
	}

	/* Deactivate recall */
	else
	{
		p_ptr->word_recall = 0;
		msg_print("A tension leaves the air around you...");
	}

	return (TRUE);
}


/*
 * Detect all features on current panel
 */
bool detect_feat_flags(u32b f1, u32b f2, u32b f3, int r, bool *known)
{
	int y, x;

	bool detect = FALSE;

retry:
	/* Scan the grids out to radius */
	for (y = MAX(p_ptr->py - r, 0); y < MIN(p_ptr->py + r, DUNGEON_HGT); y++)
	{
		for (x = MAX(p_ptr->px - r, 0); x < MIN(p_ptr->px+r, DUNGEON_WID); x++)
		{
			/* Check distance */
			if (distance(p_ptr->py, p_ptr->px, y, x) > r) continue;

			/* Hack -- Safe from traps */
			if ((*known) && (view_unsafe_grids) && (f1 & (FF1_TRAP)))
			{
				play_info[y][x] |= (PLAY_SAFE);
				lite_spot(y,x);
			}

			/* Hack -- other detection magic */
			else if ((*known) && (view_detect_grids) && !(f1) && !(f2) && !(f3))
			{
				play_info[y][x] |= (PLAY_SAFE);
				lite_spot(y,x);
			}

			/* Detect flags */
			if ((f_info[cave_feat[y][x]].flags1 & (f1)) ||
				(f_info[cave_feat[y][x]].flags2 & (f2)) ||
				(f_info[cave_feat[y][x]].flags3 & (f3)))
			{
				/* Detect secrets */
				if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
				{
					/*Find secrets*/
					cave_alter_feat(y,x,FS_SECRET);
				}

				/* Retest following secrets being found */
				/* We do this to avoid detecting e.g. empty shelves once the item has been created that
				 * was on the shelf */
				if ((f_info[cave_feat[y][x]].flags1 & (f1)) ||
					(f_info[cave_feat[y][x]].flags2 & (f2)) ||
					(f_info[cave_feat[y][x]].flags3 & (f3)) ||
					((play_info[y][x] & (PLAY_SEEN | PLAY_MARK)) != 0))
				{
					/* Hack -- Memorize */
					play_info[y][x] |= (PLAY_MARK);
	
					/* Redraw */
					lite_spot(y, x);
	
					/* Obvious */
					detect = TRUE;
				}
			}
		}
	}

	/* Was unknown */
	if (detect && !(*known))
	{
		*known = TRUE;

		goto retry;
	}

	/* Seen something */
	if ((detect) || (*known))
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Result */
	return (detect);
}


/*
 * Detect all features on current panel
 */
bool detect_feat_blows(int effect, int r, bool *known)
{
	int y, x;

	bool detect = FALSE;

retry:
	/* Scan the grids out to radius */
	for (y = MAX(p_ptr->py - r, 0); y < MIN(p_ptr->py + r, DUNGEON_HGT); y++)
	{
		for (x = MAX(p_ptr->px - r, 0); x < MIN(p_ptr->px+r, DUNGEON_WID); x++)
		{
			/* Check distance */
			if (distance(p_ptr->py, p_ptr->px, y, x) > r) continue;

			/* Detect blow match */
			if (f_info[cave_feat[y][x]].blow.effect == effect)
			{
				/* Detect secrets */
				if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
				{
					/*Find secrets*/
					cave_alter_feat(y,x,FS_SECRET);
				}

				/* Hack -- Memorize */
				play_info[y][x] |= (PLAY_MARK);

				/* Redraw */
				lite_spot(y, x);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	/* Was unknown */
	if (detect && !(*known))
	{
		*known = TRUE;

		goto retry;
	}

	/* Seen something */
	if ((detect) || (*known))
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Result */
	return (detect);
}



/*
 * Detect all objects of a particular tval on the current panel
 */
bool detect_objects_tval(int tval)
{
	int i, y, x;

	bool detect = FALSE;

	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Skip stored objects */
		if (o_ptr->ident & (IDENT_STORE)) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->py, p_ptr->px, y, x) > 2 * MAX_SIGHT) continue;

		/* Detect "gold" objects */
		if (o_ptr->tval == tval)
		{
			/* Hack -- memorize it */
			if (!auto_pickup_ignore(o_ptr)) o_ptr->ident |= (IDENT_MARKED);

			/* Get awareness */
			object_aware_tips(o_ptr, TRUE);

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}
	}

	/* Seen something */
	if (detect)
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Result */
	return (detect);
}


/*
 * Detect all "normal" objects on the current panel
 */
bool detect_objects_normal(void)
{
	int i, y, x;

	bool detect = FALSE;

	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Skip stored objects */
		if (o_ptr->ident & (IDENT_STORE)) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->py, p_ptr->px, y, x) > 2 * MAX_SIGHT) continue;

		/* Detect "real" objects */
		if (o_ptr->tval < TV_GOLD)
		{
			/* Hack -- memorize it */
			if (!auto_pickup_ignore(o_ptr)) o_ptr->ident |= (IDENT_MARKED);

			/* Get awareness */
			object_aware_tips(o_ptr, TRUE);

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}
	}

	/* Seen something */
	if (detect)
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Result */
	return (detect);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 3 (Magic).
 */
int value_check_aux3(const object_type *o_ptr)
{
	/* If sensed cursed, have no more value to add */
	if ((o_ptr->feeling == INSCRIP_CURSED) || (o_ptr->feeling == INSCRIP_TERRIBLE)
		|| (o_ptr->feeling == INSCRIP_WORTHLESS)) return (0);

	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr))
		{
			if ((o_ptr->feeling == INSCRIP_UNBREAKABLE)
				|| (o_ptr->feeling == INSCRIP_ARTIFACT)) return (INSCRIP_TERRIBLE);

			return (INSCRIP_NONMAGICAL);
		}

		/* Normal */
		return (INSCRIP_SPECIAL);
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_NONMAGICAL);

		/* Superb */
		if (o_ptr->xtra1) return (INSCRIP_SUPERB);

		/* Normal */
		return (INSCRIP_EXCELLENT);
	}

	/* Cursed or broken items */
	if (cursed_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_UNUSUAL) return (INSCRIP_CURSED);

		return (INSCRIP_NONMAGICAL);
	}

	/* Cursed or broken items */
	if (broken_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_UNUSUAL) return (INSCRIP_BROKEN);

		return (INSCRIP_NONMAGICAL);
	}

	/* Coated item */
	if (coated_p(o_ptr)) return (INSCRIP_COATED);

	/* Magic item */
	if (o_ptr->xtra1)
	{
		if (object_power(o_ptr) > 0)
			return (INSCRIP_EXCELLENT);
		else
			return (INSCRIP_NONMAGICAL);
	}

	/* Great "armor" bonus */
	if (o_ptr->to_a > MAX(7, o_ptr->ac))
	  return (INSCRIP_GREAT);

	/* Great to_h bonus */
	if (o_ptr->to_h > 9) return (INSCRIP_GREAT);

	/* Great to_d bonus */
	if (o_ptr->to_d >
	    MAX(7, k_info[o_ptr->k_idx].dd * k_info[o_ptr->k_idx].ds))
	  return (INSCRIP_GREAT);

	/* Great "weapon" dice */
	if (o_ptr->dd > k_info[o_ptr->k_idx].dd) return (INSCRIP_GREAT);

	/* Great "weapon" sides */
	if (o_ptr->ds > k_info[o_ptr->k_idx].ds) return (INSCRIP_GREAT);

	/* Very good "armor" bonus */
	if (o_ptr->to_a > 5) return (INSCRIP_VERY_GOOD);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 7) return (INSCRIP_VERY_GOOD);

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return (INSCRIP_GOOD);

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return (INSCRIP_GOOD);

	/* Have already got nonmagical sensed */
	if (o_ptr->feeling == INSCRIP_AVERAGE) return(INSCRIP_AVERAGE);
	if (o_ptr->feeling == INSCRIP_UNCURSED) return(INSCRIP_AVERAGE);
	if (o_ptr->feeling == INSCRIP_UNUSUAL) return(INSCRIP_MAGICAL);

	/* Default to nothing */
	return (INSCRIP_NONMAGICAL);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 4 (Cursed).
 *
 * Note we return INSCRIP_UNCURSED for items that we do not mark
 * this way, but allow such items to be sensed again, elsewhere.
 * Hack -- we 'overload' this semantically with removed curses.
 */
int value_check_aux4(const object_type *o_ptr)
{
	/* If sensed precisely, have no more value to add */
	if ((o_ptr->feeling == INSCRIP_GOOD) || (o_ptr->feeling == INSCRIP_VERY_GOOD)
		|| (o_ptr->feeling == INSCRIP_GREAT) || (o_ptr->feeling == INSCRIP_EXCELLENT)
		|| (o_ptr->feeling == INSCRIP_SUPERB) || (o_ptr->feeling == INSCRIP_SPECIAL)
		|| (o_ptr->feeling == INSCRIP_AVERAGE) || (o_ptr->feeling == INSCRIP_BROKEN)
		|| (o_ptr->feeling == INSCRIP_USELESS) || (o_ptr->feeling == INSCRIP_USEFUL)
		|| (o_ptr->feeling == INSCRIP_MAGICAL) || (o_ptr->feeling == INSCRIP_UNCURSED)) return (0);

	/* Artifacts */
	if ((artifact_p(o_ptr)) || (o_ptr->feeling == INSCRIP_ARTIFACT))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_TERRIBLE);

		/* Known to be artifact strength */
		if ((o_ptr->feeling == INSCRIP_UNBREAKABLE)
			|| (o_ptr->feeling == INSCRIP_ARTIFACT)) return (INSCRIP_SPECIAL);

		/* Normal */
		return (INSCRIP_UNCURSED);
	}

	/* Ego-Items */
	if ((ego_item_p(o_ptr)) || (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM) || (o_ptr->feeling == INSCRIP_EGO_ITEM))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return (INSCRIP_WORTHLESS);

		/* Known to be high ego-item strength */
		if ((o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM)) return (INSCRIP_SUPERB);

		/* Known to be high ego-item strength */
		if ((o_ptr->feeling == INSCRIP_EGO_ITEM)) return (INSCRIP_EXCELLENT);

		/* Normal */
		return (INSCRIP_UNCURSED);
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return (INSCRIP_CURSED);

	/* Known to be magic items */
	if (o_ptr->feeling == INSCRIP_MAGIC_ITEM) return (INSCRIP_USEFUL);

	/* Known not to be nonmagical */
	if (o_ptr->feeling == INSCRIP_NONMAGICAL) return(INSCRIP_USELESS);

	/* Default to uncursed */
	return (INSCRIP_UNCURSED);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 5 (Power).
 */
int value_check_aux5(const object_type *o_ptr)
{
	/* If sensed magical, have no more value to add */
	if ((o_ptr->feeling == INSCRIP_GOOD) || (o_ptr->feeling == INSCRIP_VERY_GOOD)
		|| (o_ptr->feeling == INSCRIP_GREAT)
		|| (o_ptr->feeling == INSCRIP_SUPERB) || (o_ptr->feeling == INSCRIP_SPECIAL)
		|| (o_ptr->feeling == INSCRIP_ARTIFACT) || (o_ptr->feeling == INSCRIP_EGO_ITEM)
		|| (o_ptr->feeling == INSCRIP_TERRIBLE) || (o_ptr->feeling == INSCRIP_WORTHLESS)
		|| (o_ptr->feeling == INSCRIP_CURSED)|| (o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM)) return (0);

	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_UNCURSED) return (INSCRIP_SPECIAL);
		else if (o_ptr->feeling == INSCRIP_NONMAGICAL) return (INSCRIP_TERRIBLE);

		/* Normal */
		return (INSCRIP_ARTIFACT);
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_NONMAGICAL) return (INSCRIP_WORTHLESS);
		else if ((o_ptr->feeling == INSCRIP_UNCURSED) && (o_ptr->xtra1)) return (INSCRIP_EXCELLENT);
		else if (o_ptr->feeling == INSCRIP_UNCURSED) return (INSCRIP_EXCELLENT);

		/* Superb */
		if (o_ptr->xtra1) return (INSCRIP_HIGH_EGO_ITEM);

		/* Normal */
		return (INSCRIP_EGO_ITEM);
	}

	/* Broken items */
	if (broken_p(o_ptr)) return (INSCRIP_NONMAGICAL);

	/* Coated item */
	if (coated_p(o_ptr)) return (INSCRIP_COATED);

	/* Magical item */
	if ((o_ptr->xtra1) && (object_power(o_ptr) > 0)) return (INSCRIP_EGO_ITEM);
	if (o_ptr->feeling == INSCRIP_MAGICAL) return (0);

	/* Cursed items */
	if (cursed_p(o_ptr))
	{
		if (o_ptr->feeling == INSCRIP_NONMAGICAL) return (INSCRIP_CURSED);

		return (INSCRIP_MAGIC_ITEM);
	}

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0)
	{
		return (INSCRIP_MAGIC_ITEM);
	}

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0)
	{
		return (INSCRIP_MAGIC_ITEM);
	}

	/* Default to nothing */
	return (INSCRIP_AVERAGE);
}

/*
 * Returns true if an item detects as 'magic'
 */
static bool item_tester_magic(const object_type *o_ptr)
{
	/* Exclude cursed or broken */
	if (cursed_p(o_ptr) || broken_p(o_ptr)) return (FALSE);

	/* Include artifacts and ego items */
	if (artifact_p(o_ptr) || ego_item_p(o_ptr)) return (TRUE);

	/* Include magic items */
	if ((o_ptr->xtra1) && (o_ptr->xtra1 < OBJECT_XTRA_MIN_COATS)) return (TRUE);

	/* Artifacts, misc magic items, or enchanted wearables */
	switch (o_ptr->tval)
	{
		case TV_AMULET:
		case TV_RING:
		case TV_STAFF:
		case TV_WAND:
		case TV_ROD:
		case TV_SCROLL:
		case TV_POTION:
		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:
		case TV_SONG_BOOK:
		case TV_RUNESTONE:
		case TV_STUDY:
			return (TRUE);
	}

	/* Positive bonuses */
	if ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)) return (TRUE);

	return (FALSE);
}

/*
 * Returns true if an item detects as 'powerful'
 */
static bool item_tester_power(const object_type *o_ptr)
{
	/* Include cursed or broken */
	if (cursed_p(o_ptr) || broken_p(o_ptr)) return (TRUE);

	/* Otherwise magic */
	return (item_tester_magic(o_ptr));
}


/*
 * Returns true if an item detects as 'cursed'
 */
static bool item_tester_cursed(const object_type *o_ptr)
{
	/* Exclude cursed or broken */
	if (cursed_p(o_ptr) || broken_p(o_ptr)) return (TRUE);

	return (FALSE);
}


/*
 * Detect all objects on the current panel of a particular 'type'.
 *
 * This will light up all spaces with items matching a tester function
 * as well as senses all objects in the inventory using a particular
 * type of sensing function.
 *
 * If ignore_feeling is set, it'll treat a feeling of that value
 * as not inducing detection.
 *
 * It can probably be argued that this function is now too powerful.
 */
static bool detect_objects_type(bool (*detect_item_hook)(const object_type *o_ptr), int sense_type, int ignore_feeling)
{
	int i, y, x, tv;

	bool detect = FALSE;

	/* Scan all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Skip stored objects */
		if (o_ptr->ident & (IDENT_STORE)) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->py, p_ptr->px, y, x) > 2 * MAX_SIGHT) continue;

		/* Examine the tval */
		tv = o_ptr->tval;

		/* Artifacts, misc magic items, or enchanted wearables */
		if (!(detect_item_hook) || (detect_item_hook)(o_ptr))
		{
			/* Memorize the item */
			if (!auto_pickup_ignore(o_ptr)) o_ptr->ident |= (IDENT_MARKED);

			/* Hack -- have seen object */
			if (!(k_info[o_ptr->k_idx].flavor)) k_info[o_ptr->k_idx].aware = TRUE;

			/* XXX XXX - Mark monster objects as "seen" */
			if ((o_ptr->name3 > 0) && !(l_list[o_ptr->name3].sights)) l_list[o_ptr->name3].sights++;

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}

		/* Sense if necessary */
		if (sense_type)
		{
			/* Sense something */
			if (sense_magic(o_ptr, sense_type, TRUE, TRUE))
			{
				/* Auto-id average items */
				if (o_ptr->feeling == INSCRIP_AVERAGE) object_bonus(o_ptr, TRUE);
			}
		}
	}

	/* Sense inventory */
	if (sense_type) for (i = 0; i < INVEN_TOTAL; i++)
	{
		int feel = 0;

		object_type *o_ptr = &inventory[i];

		/* Get the inscription */
		if (sense_magic(o_ptr, sense_type, TRUE, TRUE))
		{
			/* Detect */
			if (o_ptr->feeling != ignore_feeling) detect = TRUE;

			/* Auto-id average items */
			if (feel == INSCRIP_AVERAGE) object_bonus(o_ptr, TRUE);

			/* Combine / Reorder the pack (later) */
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
		}
	}

	/* Seen something */
	if (detect)
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Return result */
	return (detect);
}



static int monster_race_hook = 0;


/*
 * Hook to specify a single race of monster;
 */
static bool monster_tester_hook_single(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	/* Don't detect invisible monsters */
	if (monster_race_hook == m_ptr->r_idx)
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "normal (non-invisible)" monsters
 */
static bool monster_tester_hook_normal(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Don't detect invisible monsters */
	if ((r_ptr->flags2 & (RF2_INVISIBLE)) || (m_ptr->tim_invis))
	{
		return (FALSE);
	}

	return (TRUE);
}


/*
 * Hook to specify "evil" monsters
 */
static bool monster_tester_hook_evil(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Detect evil monsters */
	if (r_ptr->flags3 & (RF3_EVIL))
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "living" monsters
 */
static bool monster_tester_hook_living(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Don't detect non-living monsters */
	if (r_ptr->flags3 & (RF3_NONLIVING))
	{
		return (FALSE);
	}

	return (TRUE);
}


/*
 * Hook to specify "mineral" monsters
 */
static bool monster_tester_hook_mineral(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Detect all mineral monsters */
	if (r_ptr->flags9 & (RF9_DROP_MINERAL))
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "mimic" monsters
 */
static bool monster_tester_hook_mimic(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Detect all object-like monsters */
	if (strchr("!-_\\/{}[]&,~\"=()",r_ptr->d_char))
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "water" monsters
 */
static bool monster_tester_hook_water(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Detect all watery monsters */
	if (r_ptr->flags3 & (RF3_HURT_WATER))
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "magic" monsters
 */
static bool monster_tester_hook_magic(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Detect all magic monsters */
	if (r_ptr->mana)
	{
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "mental" monsters
 */
static bool monster_tester_hook_mental(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Don't detect empty mind monsters */
	if (r_ptr->flags2 & (RF2_EMPTY_MIND))
	{
		return (FALSE);
	}
	/* Detect weird mind monsters 10% of the time (matches telepathy check in update_mon) */
	else if ((r_ptr->flags2 & (RF2_WEIRD_MIND)) && ((m_idx % 10) != 5))
	{
		return (FALSE);
	}

	return (TRUE);
}


/*
 * Hook to specify "fiery" monsters
 */
static bool monster_tester_hook_fire(const int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int i;

	for (i = 0; i < 4; i++)
	{
		int effect = r_ptr->blow[i].effect;

		if ((effect == GF_FIRE) || (effect == GF_SMOKE) || (effect == GF_LAVA))
			return (TRUE);
	}

	return (FALSE);
}




/*
 * Detect all "normal" monsters on the current panel
 */
bool detect_monsters(bool (*monster_test_hook)(const int m_idx), bool *known)
{
	int i, y, x;

	bool flag = FALSE;


	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->py, p_ptr->px, y, x) > 2 * MAX_SIGHT) continue;

		/* Detect all non-invisible monsters */
		if ((*monster_test_hook)(i))
		{
			/* Optimize -- Repair flags */
			repair_mflag_mark = repair_mflag_show = TRUE;

			/* Hack -- Detect the monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Update the monster */
			update_mon(i, FALSE);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Hack -- mark as safe */
	if (view_detect_grids) detect_feat_flags(0L, 0L, 0L, 2 * MAX_SIGHT, known);

	/* Seen something */
	if ((flag) || (view_detect_grids))
	{
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_MAP);
	}

	/* Result */
	return (flag);
}


/*
 * Convert existing terrain type to "up stairs"
 */
static void place_up_stairs(int y, int x)
{
	/* Create up stairs */
	cave_set_feat(y, x, FEAT_LESS);
}


/*
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(int y, int x)
{
	bool outside = (level_flag & (LF1_SURFACE))
		&& (f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE));

	/* Surface -- place entrance if outside */
	if (outside)
	{
		cave_set_feat(y, x, FEAT_ENTRANCE);
	}

	/* Create down stairs */
	else
	{
		cave_set_feat(y, x, FEAT_MORE);
	}
}


/*
 * Convert existing terrain type to "quest stairs"
 */
void place_quest_stairs(int y, int x)
{
	bool outside = (level_flag & (LF1_SURFACE))
		&& (f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE));

	/* Create up stairs in tower */
	if (level_flag & (LF1_TOWER))
	{
		cave_set_feat(y, x, FEAT_LESS);
	}

	/* Surface -- place entrance if outside */
	else if (outside)
	{
		cave_set_feat(y, x, FEAT_ENTRANCE);
	}

	/* Create down stairs */
	else
	{
		cave_set_feat(y, x, FEAT_MORE);
	}
}


/*
 * Place an up/down staircase at given location
 */
bool place_random_stairs(int y, int x, int feat)
{
	/* Paranoia */
	if (!cave_clean_bold(y, x)) return (FALSE);

	/* No dungeon, no stairs */
	if ((level_flag & (LF1_LESS | LF1_MORE)) == 0)
	{
		return (FALSE);
	}

	/* Fixed stairs */
	else if (feat)
	{
		/* Hack -- restrict stairs */
		if (((f_info[feat].flags1 & (FF1_LESS)) != 0) && ((level_flag & (LF1_LESS)) == 0)) feat = feat_state(feat, FS_MORE);
		else if (((f_info[feat].flags1 & (FF1_MORE)) != 0) && ((level_flag & (LF1_MORE)) == 0)) feat = feat_state(feat, FS_LESS);

		cave_set_feat(y, x, feat);
	}

	/* Cannot go down, must go up */
	else if ((level_flag & (LF1_MORE)) == 0)
	{
		place_up_stairs(y, x);
	}

	/* Cannot go up, must go down */
	else if ((level_flag & (LF1_LESS)) == 0)
	{
		place_down_stairs(y, x);
	}

	/* Random stairs -- bias towards direction player is heading */
	/* Important: If we are placing downstairs, it is because we headed up from the previous
	 * level. So downstairs should be less common as the player is climbing. */
	else if (rand_int(100) < (((f_info[p_ptr->create_stair].flags1 & (FF1_MORE)) != 0) ? 25 :
					(((f_info[p_ptr->create_stair].flags1 & (FF1_LESS)) != 0) ? 75 : 50)) )
	{
		place_down_stairs(y, x);
	}

	/* Random stairs */
	else
	{
		place_up_stairs(y, x);
	}

	return(TRUE);
}


/*
 * Concentrate hook. Destination must be water. Change to earth if
 * concentrated.
 */
bool concentrate_water_hook(const int y, const int x, const bool modify)
{
	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	/* Need 'running water' */
	if (((f_ptr->flags2 & (FF2_WATER)) == 0)

	/* Allow wet floors/fountains/wells */
		&& (f_ptr->k_idx != 224)) return (FALSE);

	/* Not modifying yet */
	if (!modify) return (TRUE);

	/* From lower water in spells1.c */
	if (prefix(f_name+f_ptr->name,"stone bridge"))
	{
		/* Hack -- change bridges to prevent exploits */
		cave_set_feat(y, x, FEAT_BRIDGE_CHASM);

		/* Notice any changes */
		if (player_can_see_bold(y,x)) return (TRUE);
	}

	/* Turn into earth */
	else
	{
		cave_set_feat(y,x,FEAT_EARTH);

		/* Notice any changes */
		if (player_can_see_bold(y,x)) return (TRUE);
	}

	/* Not changed, no need for update */
	return (FALSE);
}


/*
 * Concentrate hook. Destination must be living.
 */
bool concentrate_life_hook(const int y, const int x, const bool modify)
{
	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	/* Need 'running water' */
	if ((f_ptr->flags3 & (FF3_LIVING)) == 0) return (FALSE);

	/* Not modifying yet */
	if (!modify) return (TRUE);

	/* Kill it */
	cave_alter_feat(y,x,FS_LIVING);

	/* Notice any changes */
	if (player_can_see_bold(y,x)) return (TRUE);

	/* Not changed, no need for update */
	return (FALSE);
}


/*
 * Concentrate hook. Destination must be light. Change to dark if
 * concentrated.
 */
bool concentrate_light_hook(const int y, const int x, const bool modify)
{
	/* Need 'magical light' */
	if ((cave_info[y][x] & (CAVE_GLOW)) == 0) return (FALSE);

	/* Not modifying yet */
	if (!modify) return (TRUE);

	/* Darken the grid */
	cave_info[y][x] &= ~(CAVE_GLOW);

	/* Forget all grids that the player can see */
	if (player_can_see_bold(y, x))
	{
		/* Forget the grid */
		play_info[y][x] &= ~(PLAY_MARK);

		/* Process affected monsters */
		if (cave_m_idx[y][x] > 0)
		{
			/* Update the monster */
			(void)update_mon(cave_m_idx[y][x], FALSE);
		}

		/* Redraw */
		lite_spot(y, x);

		return (TRUE);
	}

	/* Not changed, no need for update */
	return (FALSE);
}


/*
 * Concentrate power from surrounding grids, note how much we gained.
 *
 * Allow use by both the character and monsters.
 *
 * Adapted from Sangband implementation of concentrate_light.
 *
 * TODO: Consider only altering the target terrain if the caster
 * is 'evil'.
 *
 * -DarkGod-, -LM-
 */
int concentrate_power(int y0, int x0, int radius, bool for_real, bool use_los,
		bool concentrate_hook(const int y, const int x, const bool modify))
{
	int power = 0;
	int y, x, r;

	bool changes = FALSE;

	/* Hack -- Flush any pending output */
	handle_stuff();

	/* Drain power inwards */
	for (r = radius; r >= 0; r--)
	{
		/* Scan the grids in range */
		for (y = y0 - r; y <= y0 + r; y++)
		{
			for (x = x0 - r; x <= x0 + r; x++)
			{
				/* Stay legal */
				if (!in_bounds_fully(y, x)) continue;

				/* Hack -- for power only. Must permit line of sight.
				 * This prevents the lit walls of rooms getting drained from
				 * the 'wrong side'. */
				if (use_los && !player_has_los_bold(y, x)) continue;

				/* Drain this distance only */
				if (distance(y, x, y0, x0) != r) continue;

				/* Must be in line of sight/fire */
				if (!generic_los(y, x, y0, x0, use_los ? CAVE_XLOS : CAVE_XLOF)) continue;

				/* Grid has power */
				if (concentrate_hook(y, x, FALSE))
				{
					/* Count this grid */
					power++;

					/* We're doing this for real, boys */
					if (for_real)
					{
						/* Apply the hook */
						changes |= concentrate_hook(y, x, TRUE);
					}
				}
			}
		}

#if 0
		/* Graphics */
		if ((for_real) && (changes) && (op_ptr->delay_factor))
		{
			/* Screen refresh */
			(void)Term_fresh();

			/* Update the view (now) */
			p_ptr->update |= (PU_UPDATE_VIEW);

			handle_stuff();

			/* Longish delay for character */
			if (who <= SOURCE_PLAYER_START)
				pause_for(50 + op_ptr->delay_factor * op_ptr->delay_factor);

			/* Allow a brief one for monsters */
			else if (op_ptr->delay_factor > 2)
				pause_for(op_ptr->delay_factor);
		}
#endif

	}

	/* Update the view (later) */
	if (for_real && changes) p_ptr->update |= (PU_UPDATE_VIEW);

	/* Note how much power we concentrated */
	return (power);
}




/*
 * Create stairs at the player location
 */
bool stair_creation(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	/* XXX XXX XXX */
	if (!cave_valid_bold(py, px))
	{
		msg_print("The object resists the spell.");
		return (FALSE);
	}

	/* XXX XXX XXX */
	delete_object(py, px);

	if (place_random_stairs(py, px, 0)) return (TRUE);

	return(FALSE);
}


/*
 * Hook to specify "weapon"
 */
static bool item_tester_hook_weapon(const object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		case TV_STAFF:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify a strict "weapon"
 */
static bool item_tester_hook_weapon_strict(const object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_STAFF:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify "ac"
 */
static bool item_tester_hook_ac(const object_type *o_ptr)
{
	if (k_info[o_ptr->k_idx].flags5 & (TR5_SHOW_AC)) return (TRUE);

	return (FALSE);
}



/*
 * Hook to specify "armour"
 */
static bool item_tester_hook_armour(const object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify "bolts"
 */
static bool item_tester_hook_ammo(const object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_ARROW:
		case TV_BOLT:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}



static bool item_tester_unknown(const object_type *o_ptr)
{
	if (object_known_p(o_ptr))
		return FALSE;
	else
		return TRUE;
}

static bool item_tester_unknown_name(const object_type *o_ptr)
{
	if (object_known_p(o_ptr))
		return FALSE;
	else if (o_ptr->ident & (IDENT_NAME))
		return FALSE;
	else
		return TRUE;
}


static int unknown_tval;

static bool item_tester_unknown_tval(const object_type *o_ptr)
{
	if(o_ptr->tval != unknown_tval) return FALSE;

	if (object_known_p(o_ptr))
		return FALSE;
	else
		return TRUE;
}


static bool item_tester_unknown_gauge(const object_type *o_ptr)
{
	if (object_known_p(o_ptr))
		return FALSE;
	else
	{
		switch (o_ptr->tval)
		{
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
			case TV_RING:
			case TV_AMULET:
			{
				if (!(o_ptr->ident & (IDENT_BONUS))) return TRUE;
				if (!(o_ptr->ident & (IDENT_PVAL))) return TRUE;
				break;
			}
			case TV_STAFF:
			{
				if (!(o_ptr->ident & (IDENT_BONUS))) return TRUE;
			}
			case TV_WAND:
			{
				if (!(o_ptr->ident & (IDENT_CHARGES))) return TRUE;
				break;
			}
		}
	}

	return FALSE;
}


static bool item_tester_unknown_star(const object_type *o_ptr)
{
	if (o_ptr->ident & IDENT_MENTAL)
		return FALSE;
	else
		return TRUE;
}


static bool item_tester_unknown_sense(const object_type *o_ptr)
{
	if (object_known_p(o_ptr))
		return FALSE;
	else if (o_ptr->ident & IDENT_SENSE)
	  return TRUE; /* TODO: should be done via IDENT_HEAVY_SENSE, etc. */
	else
		return TRUE;
}


static bool item_tester_unknown_value(const object_type *o_ptr)
{
	if (o_ptr->ident & IDENT_VALUE)
		return FALSE;
	else
		return TRUE;
}


static bool item_tester_unknown_runes(const object_type *o_ptr)
{
	if (o_ptr->xtra1 >= OBJECT_XTRA_MIN_RUNES)
		return FALSE;
	else if (o_ptr->ident & IDENT_RUNES)
		return FALSE;
	else
		return TRUE;
}


/*
 * Note for the following function, the player must know it is an artifact
 * or ego item, either by sensing or identifying it.
 */
static bool item_tester_known_rumor(const object_type *o_ptr)
{
	if (o_ptr->ident & IDENT_MENTAL)
		return FALSE;
	else if (!object_named_p(o_ptr)
		&& !(o_ptr->feeling == INSCRIP_SPECIAL)
		&& !(o_ptr->feeling == INSCRIP_EXCELLENT)
		&& !(o_ptr->feeling == INSCRIP_SUPERB)
		&& !(o_ptr->feeling == INSCRIP_WORTHLESS)
		&& !(o_ptr->feeling == INSCRIP_TERRIBLE)
		&& !(o_ptr->feeling == INSCRIP_ARTIFACT)
		&& !(o_ptr->feeling == INSCRIP_HIGH_EGO_ITEM)
		&& !(o_ptr->feeling == INSCRIP_EGO_ITEM)
		&& !(o_ptr->feeling == INSCRIP_UNBREAKABLE)
		&& !(o_ptr->feeling == INSCRIP_UNGETTABLE))
		return FALSE;
	else if (!(o_ptr->name1) && !(o_ptr->name2))
		return FALSE;
	else
		return TRUE;
}



/*
 * Enchant an item
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item.  -CFT
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(object_type *o_ptr, int n, int eflag)
{
	int i, chance, prob;

	bool res = FALSE;

	bool a = artifact_p(o_ptr);

	u32b f1, f2, f3, f4;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Large piles resist enchantment */
	prob = o_ptr->number * 100;

	/* Missiles are easy to enchant */
	if ((o_ptr->tval == TV_BOLT) ||
	    (o_ptr->tval == TV_ARROW) ||
	    (o_ptr->tval == TV_SHOT))
	{
		prob = prob / 20;
	}

	/* Try "n" times */
	for (i=0; i<n; i++)
	{
		/* Hack -- Roll for pile resistance */
		if ((prob > 100) && (rand_int(prob) >= 100)) continue;

		/* Enchant to hit */
		if (eflag & (ENCH_TOHIT))
		{
			if (o_ptr->to_h < 0) chance = 0;
			else if (o_ptr->to_h > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_h];

			/* Attempt to enchant */
			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				res = TRUE;

				/* Enchant */
				o_ptr->to_h++;

				/* Break curse */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_h >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");

					/* Uncurse the object */
					uncurse_object(o_ptr);
				}
			}
		}

		/* Enchant to damage */
		if (eflag & (ENCH_TODAM))
		{
			if (o_ptr->to_d < 0) chance = 0;
			else if (o_ptr->tval == TV_BOW)
			{
				if (o_ptr->to_d > 15) chance = 1000;
				else chance = enchant_table[o_ptr->to_d];
			}
			else if (o_ptr->to_d > o_ptr->dd * o_ptr->ds + 5) chance = 1000;
			else if (o_ptr->to_d < o_ptr->dd * o_ptr->ds) chance = enchant_table[o_ptr->to_d * 10 / o_ptr->dd / o_ptr->ds];
			else chance = enchant_table[o_ptr->to_d + 10 - o_ptr->dd * o_ptr->ds];

			/* Attempt to enchant */
			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				res = TRUE;

				/* Enchant */
				o_ptr->to_d++;

				/* Break curse */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_d >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");

					/* Uncurse the object */
					uncurse_object(o_ptr);
				}
			}
		}

		/* Enchant to armor class */
		if (eflag & (ENCH_TOAC))
		{
			if (o_ptr->to_a < 0) chance = 0;
			else if (o_ptr->to_a > o_ptr->ac + 5) chance = 1000;
			else if (o_ptr->to_a < o_ptr->ac) chance = enchant_table[o_ptr->to_a * 10 / o_ptr->ac];
			else chance = enchant_table[o_ptr->to_a + 10 - o_ptr->ac];

			/* Attempt to enchant */
			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				res = TRUE;

				/* Enchant */
				o_ptr->to_a++;

				/* Break curse */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_a >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");

					/* Uncurse the object */
					uncurse_object(o_ptr);
				}
			}
		}
	}

	/* Failure */
	if (!res) return (FALSE);

	/* Remove some inscriptions */
	if (!object_bonus_p(o_ptr) && ((o_ptr->feeling == INSCRIP_GOOD) || (o_ptr->feeling == INSCRIP_VERY_GOOD)))
	{
		/* Hack --- unsense the item */
		o_ptr->ident &= ~(IDENT_SENSE);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Success */
	return (TRUE);
}



/*
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(int num_hit, int num_dam, int num_ac)
{
	int item;
	bool okay = FALSE;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;

	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;

	/* Enchant armor if requested */
	if (num_ac) item_tester_hook = item_tester_hook_ac;

	/* Get an item */
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

	/* Describe */
	msg_format("%s %s glow%s brightly!",
		   ((item >= 0) ? "Your" : "The"), o_name,
		   ((o_ptr->number > 1) ? "" : "s"));

	/* Enchant */
	if (enchant(o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
	if (enchant(o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
	if (enchant(o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;

	/* Failure */
	if (!okay)
	{
		/* Flush */
		if (flush_failure) flush();

		/* Message */
		msg_print("The enchantment failed.");
	}
	else
	{

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	}

	/* Something happened */
	return (TRUE);
}

/*
 * Brand any item
 */
bool brand_item(int brand, cptr act)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;

	bool brand_ammo = FALSE;

	/* Get an item */
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
#if 0
	/* In a bag? */
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}
#endif
	/* Hack -- check ammo */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			brand_ammo = TRUE;
		break;
	}

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);

	/* Overwrite runes and coatings */
	if (o_ptr->xtra1 >= OBJECT_XTRA_MIN_RUNES)
	{
		/* Warning */
		msg_format("It has %s applied to it.", o_ptr->xtra1 < OBJECT_XTRA_MIN_COATS ? "runes" : "a coating");

		/* Verify */
		if (!get_check(o_ptr->xtra1 < OBJECT_XTRA_MIN_COATS ? "Overwrite them? " : "Remove it? "))
		{
			return (FALSE);
		}
	}

	/* Describe */
	msg_format("%s %s %s!",
		   ((item >= 0) ? "Your" : "The"), o_name, act);

	/* you can never modify artifacts or items with an extra power. */
	if (!(artifact_p(o_ptr)) && (!(o_ptr->xtra1) || !(o_ptr->xtra1 < OBJECT_XTRA_MIN_RUNES)))
	{
		bool split = FALSE;

		object_type *i_ptr;
		object_type object_type_body;

		/* Hack -- split stack only if required. This is dangerous otherwise as we may
		   be calling from a routine where we delete items later. XXX XXX */
		/* Mega-hack -- we allow 5 arrows/shots/bolts to be enchanted per application */
		if ((o_ptr->number > 1) && ((!brand_ammo) || (o_ptr->number > 5)))
		{
			int qty = (brand_ammo) ? 5 : 1;
			split = TRUE;

			/* Get local object */
			i_ptr = &object_type_body;

			/* Obtain a local object */
			object_copy(i_ptr, o_ptr);

			/* Modify quantity */
			i_ptr->number = qty;

			/* Reset stack counter */
			i_ptr->stackc = 0;

			/* Decrease the item (in the pack) */
			if (item >= 0)
			{
				/* Forget about item */
				if (o_ptr->number == qty) inven_drop_flags(o_ptr);

				inven_item_increase(item, -qty);
				inven_item_describe(item);
				inven_item_optimize(item);
			}
			/* Decrease the item (from the floor) */
			else
			{
				floor_item_increase(0 - item, -qty);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}

			/* Hack -- use new temporary item */
			o_ptr = i_ptr;
		}

		/* Hack -- still know runes if object was runed */
		if ((o_ptr->xtra1 >= OBJECT_XTRA_MIN_RUNES)
			&& (o_ptr->xtra1 < OBJECT_XTRA_MIN_COATS)) o_ptr->ident |= (IDENT_RUNES);

		/* Apply the brand */
		o_ptr->xtra1 = brand;
		o_ptr->xtra2 = (byte)rand_int(object_xtra_size[brand]);

		if (object_xtra_what[brand] == 1)
		{
    			object_can_flags(o_ptr,object_xtra_base[brand] << o_ptr->xtra2,0x0L,0x0L,0x0L, item < 0);
		}
		else if (object_xtra_what[brand] == 2)
		{
	    		object_can_flags(o_ptr,0x0L,object_xtra_base[brand] << o_ptr->xtra2,0x0L,0x0L, item < 0);
		}
		else if (object_xtra_what[brand] == 3)
		{
    			object_can_flags(o_ptr,0x0L,0x0L,object_xtra_base[brand] << o_ptr->xtra2,0x0L, item < 0);
		}
		else if (object_xtra_what[brand] == 4)
		{
    			object_can_flags(o_ptr,0x0L,0x0L,0x0L,object_xtra_base[brand] << o_ptr->xtra2, item < 0);
		}

		/* Hack -- some items become marked with a brand feeling */
		if (o_ptr->name1 || o_ptr->name2)
		{
			/* Set brand feeling */
			o_ptr->feeling = INSCRIP_MIN_HIDDEN + brand - 1;

			/* The object has been "sensed" */
			if (!object_known_p(o_ptr)) o_ptr->ident |= (IDENT_SENSE);
		}

		/* Carry item again if split */
		if (split)
		{
			/* Carry the item */
			if (inven_carry_okay(o_ptr)) inven_carry(o_ptr);
			else drop_near(o_ptr,0,p_ptr->py,p_ptr->px, FALSE);
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	}
	else
	{
		if (flush_failure) flush();
		msg_print("The branding failed.");

		/* Sense it? */
	}

	/* Something happened */
	return (TRUE);
}



/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown;

	/* Get an item */
	q = "Identify which item? ";
	s = "You have nothing to identify.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it fully */
	object_aware(o_ptr, item < 0);
	object_known(o_ptr);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}
	/* Something happened */
	return (TRUE);
}


/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_name(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_name;

	/* Get an item */
	q = "Identify which item name? ";
	s = "You have nothing to identify.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it fully */
	object_aware(o_ptr, item < 0);
	o_ptr->ident |= (IDENT_NAME);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}
	/* Something happened */
	return (TRUE);
}


/*
 * Identify the bonus/charges of an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_gauge(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_gauge;

	/* Get an item */
	q = "Gauge which item? ";
	s = "You have nothing to gauge.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it's bonuses */
	object_gauge(o_ptr, item < 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Something happened */
	return (TRUE);
}


/*
 * Heavily senses an item in the inventory or on the floor.
 * Returns TRUE if something was sensed, else FALSE.
 */
bool ident_spell_sense(void)
{
  int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_sense;

	/* Get an item */
	q = "Sense which item? ";
	s = "You have nothing to sense.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Sense non-wearable items */
	if (sense_magic(o_ptr, cp_ptr->sense_type, TRUE, item < 0))
	{
		if (o_ptr->feeling == INSCRIP_AVERAGE) object_bonus(o_ptr, item < 0);
	}
	/* Hack -- describe the item as if sensed */
	else
	{
		int i, j, k;

		/* Check bags */
		for (i = 0; i < SV_BAG_MAX_BAGS; i++)

		/* Find slot */
		for (j = 0; j < INVEN_BAG_TOTAL; j++)
		{
			if ((bag_holds[i][j][0] == o_ptr->tval)
				&& (bag_holds[i][j][1] == o_ptr->sval))
			  {
			    o_ptr->feeling = MAX_INSCRIP + i;

			    if (object_aware_p(o_ptr)) k_info[o_ptr->k_idx].aware |= (AWARE_SENSE);
			    else if (k_info[o_ptr->k_idx].flavor) k_info[o_ptr->k_idx].aware |= (AWARE_SENSEX);

			    /* Mark all such objects sensed */
			    /* Check world */
			    for (k = 0; k < o_max; k++)
			    {
					if ((o_list[k].k_idx == o_ptr->k_idx) && !(o_list[k].feeling))
					{
						o_list[k].feeling = o_ptr->feeling;
					}
			    }

				/* Check inventory */
				for (k = 0; k < INVEN_TOTAL; k++)
				{
					if ((inventory[k].k_idx == o_ptr->k_idx) && !(inventory[k].feeling))
					{
						inventory[k].feeling = o_ptr->feeling;
					}
				}

				break;
			  }
		}

		/* Nothing found */
		if (o_ptr->feeling < MAX_INSCRIP) return (FALSE);
	}

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Something happened */
	return (TRUE);
}



/*
 * If an item is a magic item, provide its name. Else
 * identify one flag that it has.
 * Returns TRUE if something was sensed, else FALSE.
 */
bool ident_spell_magic(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;

	bool examine = FALSE;

	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown;

	/* Get an item */
	q = "Test which item? ";
	s = "You have nothing to test.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify name if a magic item */
	if (!o_ptr->name1 && !o_ptr->name2 && o_ptr->xtra1) object_aware(o_ptr, item < 0);

	/* Else identify one random flag -- if none left, get item name unless flavoured */
	else if ((value_check_aux10(o_ptr, FALSE, FALSE, item < 0)) && !(k_info[o_ptr->k_idx].flavor)) object_aware(o_ptr, item < 0);

	/* List object abilities */
	else examine = TRUE;

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Examine */
	if (examine)
	{
		msg_print("");

		/* Save the screen */
		screen_save();

		/* Describe */
		screen_object(o_ptr);

		(void)anykey();

		/* Load the screen */
		screen_load();
	}

	/* Something happened */
	return (TRUE);
}



/*
 * Identify the value of an object in the inventory (or on the floor)
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_value(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_value;

	/* Get an item */
	q = "Value which item? ";
	s = "You have nothing to value.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Value the item */
	o_ptr->ident |= (IDENT_VALUE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Something happened */
	return (TRUE);
}


/*
 * Determine the runes on an object in the inventory (or on the floor)
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_runes(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;

	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_runes;

	/* Get an item */
	q = "Read runes on which item? ";
	s = "You have nothing to read runes on.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it's bonuses */
	o_ptr->ident |= (IDENT_RUNES);

	/* Make the object aware if player knows the rune recipe */
	if (k_info[o_ptr->k_idx].aware & (AWARE_RUNES))
	{
		object_aware(o_ptr, item < 0);
	}

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

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

	/* Something happened */
	return (TRUE);
}


/*
 * Reveal some powers of a known artifact or ego item.
 * Returns TRUE if something was attempted, else FALSE.
 */
bool ident_spell_rumor(void)
{
	int item;

	object_type *o_ptr;

	cptr p, q, r, s;

	int i;

	bool done;

	u32b f1,f2,f3,f4;

	/* Only un-id'ed items */
	item_tester_hook = item_tester_known_rumor;

	/* Get an item */
	q = "Learn legends about which item? ";
	s = "You have nothing legendary to examine.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Pick an interesting phrase */
	switch (randint(6))
	{
		case 1:
			p="Hmm... these runes look interesting..";
			r=". but they tell you nothing more.";
			break;
		case 2:
			p="You recall tales fitting an item of this description";
			r=", and they are bawdy and dull.";
			break;
		case 3:
			p="Ancient visions fill your mind..";
			r=". and give you a headache.";
			break;
		case 4:
			p="The maker's mark is strangely familiar";
			r="... oh, it's just a smudge of dirt.";
			break;
		case 5:
			p="The item glows with faint enchantments";
			r=", snaps, crackles and pops.";
			break;
		default:
			p="Ah... the memories..";
			r=". things were always worse than you remember them.";
			break;
	}

	/* Examine the item */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Remove known flags */
	f1 &= ~(o_ptr->can_flags1);
	f2 &= ~(o_ptr->can_flags2);
	f3 &= ~(o_ptr->can_flags3);
	f4 &= ~(o_ptr->can_flags4);

	/* We know everything? */
	done = ((f1 | f2 | f3 | f4) ? FALSE : TRUE);

	/* Clear some flags1 */
	if (f1)	for (i = 0;i<32;i++)
	{
		if ((f1 & (1L<<i)) && (rand_int(100)<25)) f1 &= ~(1L<<i);
	}

	/* Clear some flags2 */
	if (f2)	for (i = 0;i<32;i++)
	{
		if ((f2 & (1L<<i)) && (rand_int(100)<25)) f2 &= ~(1L<<i);
	}

	/* Clear some flags3 */
	if (f3)	for (i = 0;i<32;i++)
	{
		if ((f3 & (1L<<i)) && (rand_int(100)<25)) f3 &= ~(1L<<i);
	}

	/* Clear some flags3 */
	if (f4)	for (i = 0;i<32;i++)
	{
		if ((f4 & (1L<<i)) && (rand_int(100)<25)) f4 &= ~(1L<<i);
	}

	if (done || (f1 | f2 | f3 | f4))
	{
		char o_name[80];

		/* Tell the player the good news */
		msg_format("%s.",p);

		if (done)
		{
			/* Do we know absolutely everything? */
			if (object_known_p(o_ptr)) object_mental(o_ptr, item < 0);
			else
			{
				object_aware(o_ptr, item < 0);
				object_known(o_ptr);
			}

		}

		/* Learn more about the item */
		object_can_flags(o_ptr,f1,f2,f3,f4, item < 0);

		/* Description */
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Describe */
		if (item >= INVEN_WIELD)
		{
			msg_format("%^s: %s (%c).",
				   describe_use(item), o_name, index_to_label(item));
		}
		else if (item >= 0)
		{
			msg_format("In your pack: %s (%c).",
				   o_name, index_to_label(item));
		}
		else
		{
			msg_format("On the ground: %s.",
				   o_name);
		}
	}
	else
	{
		/* Tell the player the bad news */
		msg_format("%s%s.",p,r);

		/* Unlucky */
		msg_print("The legend lore has failed.");
	}

	if (done)
	{
		/* Tell the player to stop trying */
		msg_format("You feel you know all %s secrets.",(o_ptr->number>0?"their":"its"));

	}
	else if (f1 | f2 | f3 | f4)
	{
		/* Set text_out hook */
		text_out_hook = text_out_to_screen;

		/* Load screen */
		screen_save();

		/* Begin recall */
		Term_gotoxy(0, 1);

		/* Actually display the item */
		list_object_flags(f1, f2, f3, f4, o_ptr->ident & (IDENT_PVAL | IDENT_MENTAL | IDENT_KNOWN) ? o_ptr->pval : 0, 1);

		(void)anykey();

		/* Load screen */
		screen_load();
	}

	/* Something happened */
	return (TRUE);
}



/*
 * Identify an object in the inventory (or on the floor)
 * of a specified tval only.
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_tval(int tval)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Restrict items */
	unknown_tval = tval;

	/* Only un-id'ed items */
	item_tester_hook = item_tester_unknown_tval;

	/* Get an item */
	q = "Identify which item? ";
	s = "You have nothing to identify.";

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it fully */
	object_aware(o_ptr, item < 0);
	object_known(o_ptr);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Something happened */
	return (TRUE);
}



/*
 * Fully "identify" an object in the inventory
 *
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(void)
{
	int item;

	object_type *o_ptr;

	char o_name[80];

	cptr q, s;


	/* Only un-*id*'ed items */
	item_tester_hook = item_tester_unknown_star;

	/* Get an item */
	q = "Identify which item? ";
	s = "You have nothing to identify.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Identify it fully */
	object_aware(o_ptr, item < 0);
	object_known(o_ptr);
	object_mental(o_ptr, item < 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

	/* Handle stuff */
	handle_stuff();

	/* Description */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}


	msg_print("");

	/* Save the screen */
	screen_save();

	/* Describe */
	screen_object(o_ptr);

	(void)anykey();

	/* Load the screen */
	screen_load();

	/* Success */
	return (TRUE);
}

/*
 * Hook for "get_item()".  Determine if something is rechargable.
 */
static bool item_tester_hook_recharge(const object_type *o_ptr)
{
	/* Recharge staffs */
	if (o_ptr->tval == TV_STAFF) return (TRUE);

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return (TRUE);

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return (TRUE);

	/* Nope */
	return (FALSE);
}


/*
 * Recharge a wand/staff/rod from the pack or on the floor.
 *
 * Mage -- Recharge I --> recharge(5)
 * Mage -- Recharge II --> recharge(40)
 * Mage -- Recharge III --> recharge(100)
 *
 * Priest -- Recharge --> recharge(15)
 *
 * Scroll of recharging --> recharge(60)
 *
 * recharge(20) = 1/6 failure for empty 10th level wand
 * recharge(60) = 1/10 failure for empty 10th level wand
 *
 * It is harder to recharge high level, and highly charged wands.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 *
 * Should probably not "destroy" over-charged items, unless we
 * "replace" them by, say, a broken stick or some such.  The only
 * reason this is okay is because "scrolls of recharging" appear
 * BEFORE all staffs/wands/rods in the inventory.  Note that the
 * new "auto_sort_pack" option would correctly handle replacing
 * the "broken" wand with any other item (i.e. a broken stick).
 *
 * XXX XXX XXX Perhaps we should auto-unstack recharging stacks.
 */
bool recharge(int num)
{
	int i, t, item, lev;

	object_type *o_ptr;

	cptr q, s;


	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	/* Get an item */
	q = "Recharge which item? ";
	s = "You have nothing to recharge.";
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;

	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD)
	{
		/* Extract a recharge power */
		i = (100 - lev + num) / 5;

		/* Back-fire */
		if ((i <= 1) || (rand_int(i) == 0))
		{
			/* Hack -- backfire */
			msg_print("The recharge backfires, draining the rod further!");

			/* Hack -- decharge the rod */
			if (o_ptr->charges < 10000) o_ptr->charges = (o_ptr->charges + 100) * 2;
		}

		/* Recharge */
		else
		{
			/* Rechange amount */
			t = (num * damroll(2, 4));

			/* Recharge by that amount */
			if (o_ptr->timeout > t)
			{
				o_ptr->timeout -= t;
			}

			/* Fully recharged */
			else
			{
				o_ptr->timeout = 0;
			}
		}

		/* Hack -- round up */
		o_ptr->stackc = 0;

	}

	/* Recharge wand/staff */
	else
	{
		/* Recharge power */
		i = (num + 100 - lev - (10 * o_ptr->charges)) / 15;

		/* Back-fire XXX XXX XXX */
		if ((i <= 1) || (rand_int(i) == 0))
		{
			/* Dangerous Hack -- Destroy the item */
			msg_print("There is a bright flash of light.");

			/* Reduce and describe inventory */
			if (item >= 0)
			{
				/* Forget about item */
				inven_drop_flags(o_ptr);

				inven_item_increase(item, -999);
				inven_item_describe(item);
				inven_item_optimize(item);
			}

			/* Reduce and describe floor item */
			else
			{
				floor_item_increase(0 - item, -999);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}
		}

		/* Recharge */
		else
		{
			/* Extract a "power" */
			t = (num / (lev + 2)) + 1;

			/* Recharge based on the power */
			if (t > 0) o_ptr->charges += 2 + (s16b)randint(t);

			/* Hack -- we no longer "know" the item */
			o_ptr->ident &= ~(IDENT_KNOWN);

			/* Hack -- the item is no longer empty */
			if (o_ptr->feeling == INSCRIP_EMPTY) o_ptr->feeling = 0;

			/* Hack -- round up */
			o_ptr->stackc = 0;

		}
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	/* Something was done */
	return (TRUE);
}





/*
 * Wake up all monsters, and speed up "los" monsters.
 */
void aggravate_monsters(int who)
{
	int i;

	bool sleep = FALSE;
	bool speed = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (m_ptr->cdis < MAX_SIGHT * 2)
		{
			/* Wake up */
			if (m_ptr->csleep)
			{
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}

		/* Speed up monsters in line of sight */
		if (player_has_los_bold(m_ptr->fy, m_ptr->fx))
		{
			/* Speed up (instantly) to racial base + 10 */
			if (m_ptr->mspeed < r_ptr->speed + 10)
			{
				/* Speed up */
				m_ptr->mspeed = r_ptr->speed + 10;
				speed = TRUE;
			}
		}
	}

	/* Messages */
	if (speed) msg_print("You feel a sudden stirring nearby!");
	else if (sleep) msg_print("You hear a sudden stirring in the distance!");
}



/*
 * Delete all non-unique monsters of a given "type" from the level
 */
bool banishment(int who, int what, char typ)
{
	int i;

	bool result = FALSE;

	/* Mega-Hack -- Get a monster symbol */
	if (typ == '\0') (void)(get_com("Choose a monster race (by symbol) to banish: ", &typ));

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		/* Delete the monster */
		delete_monster_idx(i);

		/* Take some damage */
		if (who <= SOURCE_MONSTER_START) take_hit(who, what, randint(4));

		/* Take note */
		result = TRUE;
	}

	/* Update monster list window */
	p_ptr->window |= PW_MONLIST;

	return (result);
}


/*
 * Delete all nearby (non-unique) monsters
 */
bool mass_banishment(int who, int what, int y, int x)
{
	int i;

	bool result = FALSE;

	/* Banishment affects player */
	if (who > SOURCE_PLAYER_START)
	{
		if (distance(y, x, p_ptr->py, p_ptr->px) <= MAX_SIGHT)
		{
			msg_print("You are banished.");
			p_ptr->create_stair = 0;
			p_ptr->leaving = TRUE;
		}
	}

	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip unique monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Skip distant monsters */
		if (distance(y, x, m_ptr->fy, m_ptr->fx) > MAX_SIGHT) continue;

		/* Delete the monster */
		delete_monster_idx(i);

		/* Take some damage */
		if (who <= SOURCE_PLAYER_START) take_hit(who, what, randint(3));

		/* Note effect */
		result = TRUE;
	}

	/* Update monster list window */
	p_ptr->window |= PW_MONLIST;

	return (result);
}



/*
 * Entomb a player or monster in a location known to be impassable.
 *
 * Players take a lot of damage.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when banished.
 *
 * Note that players and monsters (except eaters of walls and passers
 * through walls) will never occupy the same grid as a wall (or door).
 *
 * This is not 'strictly' true, as it is now possible to get stuck in
 * cages, blocks of ice and so on, and locations such as rubble that
 * are not strictly empty. So we should technically check for FF1_MOVE
 * and FF3_EASY_CLIMB both not existing before calling this function.
 *
 * Note we encode a boolean value of invalid directions around the
 * grid to prevent monsters getting hit twice by earthquakes.
 *
 * XXX Consider passing a damage value and/or flavor so that getting
 * stuck in ice is different from granite wall is different from
 * (impassable) rubble. Of course, we could just pull this from the
 * feature at this location.
 *
 * XXX This now does not kill nonliving monsters, for balance reasons.
 */
void entomb(int cy, int cx, byte invalid)
{
	int i;
	int y, x;
	int sy = 0, sx = 0, sn = 0;
	int damage = 0;

	/* Entomb the player */
	if (cave_m_idx[cy][cx] < 0)
	{
		/* Check around the player */
		for (i = 0; i < 8; i++)
		{
			/* Get the location */
			y = cy + ddy_ddd[i];
			x = cx + ddx_ddd[i];

			/* Skip non-empty grids */
			if (!cave_empty_bold(y, x)) continue;

			/* Important -- Skip "quake" grids */
			if ((invalid & (1 << i)) != 0) continue;

			/* Count "safe" grids, apply the randomizer */
			if ((++sn > 1) && (rand_int(sn) != 0)) continue;

			/* Save the safe location */
			sy = y; sx = x;
		}

		/* Hurt the player a lot */
		if (!sn)
		{
			/* Message and damage */
			msg_format("You are crushed by the %s!", f_name + f_info[cave_feat[cy][cx]].name);
			damage = 300;
		}

		/* Destroy the grid, and push the player to safety */
		else
		{
			/* Calculate results */
			switch (randint(3))
			{
				case 1:
				{
					msg_format("You nimbly dodge the %s!", f_name + f_info[cave_feat[cy][cx]].name);
					damage = 0;
					break;
				}
				case 2:
				{
					msg_format("You are bashed by %s!", f_name + f_info[cave_feat[cy][cx]].name);
					damage = damroll(10, 4);
					inc_timed(TMD_STUN, randint(50), TRUE);
					break;
				}
				case 3:
				{
					msg_format("You are crushed between the %s and ceiling!", f_name + f_info[cave_feat[cy][cx]].name);
					damage = damroll(10, 4);
					inc_timed(TMD_STUN, randint(50), TRUE);
					break;
				}
			}

			/* Move player */
			monster_swap(cy, cx, sy, sx);
		}

		/* Take some damage */
		if (damage) take_hit(SOURCE_ENTOMB, cave_feat[cy][cx], damage);
	}
	/* Entomb a monster */
	else if (cave_m_idx[cy][cx] > 0)
	{
		monster_type *m_ptr = &m_list[cave_m_idx[cy][cx]];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Most monsters cannot co-exist with rock */
		if (!(r_ptr->flags2 & (RF2_KILL_WALL)) && !(m_ptr->tim_passw) &&
		    !(r_ptr->flags2 & (RF2_PASS_WALL)) &&
			!(f_info[cave_feat[cy][cx]].flags3 & (FF3_OUTSIDE)))
		{
			char m_name[80];

			/* Assume not safe */
			sn = 0;

			/* Monster can move to escape the wall */
			if (!(r_ptr->flags1 & (RF1_NEVER_MOVE)) && !(m_ptr->petrify))
			{
				/* Look for safety */
				for (i = 0; i < 8; i++)
				{
					/* Get the grid */
					y = cy + ddy_ddd[i];
					x = cx + ddx_ddd[i];

					/* Skip non-empty grids */
					if (!cave_empty_bold(y, x)) continue;

					/* Hack -- no safety on glyph of warding */
					if (f_info[cave_feat[y][x]].flags1 & (FF1_GLYPH)) continue;

					/* Important -- Skip "quake" grids */
					if ((invalid & (1 << i)) != 0) continue;

					/* Count "safe" grids, apply the randomizer */
					if ((++sn > 1) && (rand_int(sn) != 0)) continue;

					/* Save the safe grid */
					sy = y;
					sx = x;
				}
			}

			/* Describe the monster */
			monster_desc(m_name, sizeof(m_name), cave_m_idx[cy][cx], 0);

			/* Scream in pain */
			msg_format("%^s wails out in pain!", m_name);

			/* Take damage from the quake */
			damage = (sn || (r_ptr->flags3 & (RF3_NONLIVING))) ? (int)damroll(4, 8) : (m_ptr->hp + 1);

			/* Monster is certainly awake */
			m_ptr->csleep = 0;

			/* Apply damage directly */
			m_ptr->hp -= damage;

			/* Delete (not kill) "dead" monsters */
			if (m_ptr->hp < 0)
			{
				/* Message */
				msg_format("%^s is trapped in the %s!", m_name, f_name + f_info[cave_feat[cy][cx]].name);

				/* Delete the monster */
				delete_monster(cy, cx);

				/* No longer safe */
				sn = 0;
			}

			/* Hack -- Escape from the rock */
			if (sn)
			{
				/* Move the monster */
				monster_swap(cy, cx, sy, sx);
			}
		}
	}
}


/*
 * This routine clears the entire "temp" set.
 */
void clear_temp_array(void)
{
	int i;

	/* Apply flag changes */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		/* No longer in the array */
		play_info[y][x] &= ~(PLAY_TEMP);
	}

	/* None left */
	temp_n = 0;
}


/*
 * Aux function -- see below
 */
void cave_temp_mark(int y, int x, bool room)
{
	/* Avoid infinite recursion */
	if (play_info[y][x] & (PLAY_TEMP)) return;

	/* Option -- do not leave the current room */
	if ((room) && (!(cave_info[y][x] & (CAVE_ROOM)))) return;

	/* Verify space */
	if (temp_n == TEMP_MAX) return;

	/* Mark the grid */
	play_info[y][x] |= (PLAY_TEMP);

	/* Add it to the marked set */
	temp_y[temp_n] = y;
	temp_x[temp_n] = x;
	temp_n++;
}

/*
 * Mark the nearby area with CAVE_TEMP flags.  Allow limited range.
 */
void spread_cave_temp(int y1, int x1, int range, bool room)
{
	int i, y, x;

	/* Add the initial grid */
	cave_temp_mark(y1, x1, room);

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < temp_n; i++)
	{
		x = temp_x[i], y = temp_y[i];

		/* Walls get marked, but stop further spread */
		/* Note that light 'projects' through many obstacles */
		if (!cave_project_bold(y, x) && !cave_floor_bold(y, x)) continue;

		/* Note limited range (note:  we spread out one grid further) */
		if ((range) && (distance(y1, x1, y, x) >= range)) continue;

		/* Spread adjacent */
		cave_temp_mark(y + 1, x, room);
		cave_temp_mark(y - 1, x, room);
		cave_temp_mark(y, x + 1, room);
		cave_temp_mark(y, x - 1, room);

		/* Spread diagonal */
		cave_temp_mark(y + 1, x + 1, room);
		cave_temp_mark(y - 1, x - 1, room);
		cave_temp_mark(y - 1, x + 1, room);
		cave_temp_mark(y + 1, x - 1, room);
	}
}




/*
 * This routine clears the entire "temp" set.
 *
 * This routine will Perma-Lite all "temp" grids.
 *
 * This routine is used (only) by "lite_room()"
 *
 * Dark grids are illuminated.
 *
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 3/4 the time when illuminated
 * STUPID monsters wake up 3/10 the time when illuminated
 *
 * Percentages were adjusted up, to make a greater change
 * of waking up monsters.
 */
static void cave_temp_room_lite(void)
{
	int i;

	/* Apply flag changes */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		/* No longer in the array */
		play_info[y][x] &= ~(PLAY_TEMP);

		/* Perma-Lite */
		cave_info[y][x] |= (CAVE_GLOW);
	}

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Update stuff */
	update_stuff();

	/* Process the grids */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		/* Redraw the grid */
		lite_spot(y, x);

		/* Process affected monsters */
		if (cave_m_idx[y][x] > 0)
		{
			int chance = 75;

			monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & (RF2_STUPID)) chance = 30;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & (RF2_SMART)) chance = 100;

			/* Sometimes monsters wake up */
			if (m_ptr->csleep && (rand_int(100) < chance))
			{
				/* Wake up! */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					char m_name[80];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

					/* Dump a message */
					msg_format("%^s wakes up.", m_name);
				}
			}
		}
	}

	/* None left */
	temp_n = 0;
}



/*
 * This routine clears the entire "temp" set.
 *
 * This routine will "darken" all "temp" grids.
 *
 * In addition, some of these grids will be "unmarked".
 *
 * This routine is used (only) by "unlite_room()"
 */
static void cave_temp_room_unlite(void)
{
	int i,ii;

	/* Apply flag changes */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		/* No longer in the array */
		play_info[y][x] &= ~(PLAY_TEMP);

		/* Darken the grid */
		if (!(f_info[cave_feat[y][x]].flags2 & (FF2_GLOW)))
		{
			cave_info[y][x] &= ~(CAVE_GLOW);
		}

		/* Check to see if not illuminated by innately glowing grids */
		for (ii = 0; ii < 8; ii++)
		{
			int yy = y + ddy_ddd[ii];
			int xx = x + ddx_ddd[ii];

			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			if (f_info[cave_feat[yy][xx]].flags2 & (FF2_GLOW))
			{
				/* Illuminate the grid */
				cave_info[y][x] |= (CAVE_GLOW);

			}

		}

		/* Hack -- Forget "boring" grids */
		if (!(cave_info[y][x] & (CAVE_GLOW)) &&
			!(f_info[cave_feat[y][x]].flags1 & (FF1_REMEMBER)))
		{
			/* Forget the grid */
			play_info[y][x] &= ~(PLAY_MARK);
		}
	}

	/* Fully update the visuals */
	p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);

	/* Update stuff */
	update_stuff();

	/* Process the grids */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		/* Redraw the grid */
		lite_spot(y, x);
	}

	/* None left */
	temp_n = 0;
}


/*
 * Illuminate any room containing the given location.
 */
void lite_room(int y, int x)
{
	/* Check the room */
	if (cave_info[y][x] & (CAVE_ROOM))
	{
		/* Hack --- Have we seen this room before? */
		if (!(room_has_flag(y, x, ROOM_SEEN)))
		{
			p_ptr->update |= (PU_ROOM_INFO);
			p_ptr->window |= (PW_ROOM_INFO);
		}

		/* Some rooms cannot be completely lit */
		if (room_has_flag(y, x, ROOM_GLOOMY))
		{

			/* Warn the player */
			msg_print("The light fails to penetrate the gloom.");

			return;
		}
	}

	/* Add the initial grid */
	spread_cave_temp(y, x, MAX_SIGHT, TRUE);

	/* Lite the (part of) room */
	cave_temp_room_lite();
}


/*
 * Darken all rooms containing the given location
 */
void unlite_room(int y, int x)
{
	/* Add the initial grid */
	spread_cave_temp(y, x, MAX_SIGHT, TRUE);

	/* Lite the (part of) room */
	cave_temp_room_unlite();
}


/*
 * Create a (wielded) spell item
 */
static void wield_spell(int item, int k_idx, int time, int level, int r_idx)
{
	object_type *i_ptr;
	object_type object_type_body;

	cptr act;

	char o_name[80];

	/* Get the wield slot */
	object_type *o_ptr = &inventory[item];

	/* Get local object */
	i_ptr = &object_type_body;

	/* Create the spell */
	object_prep(i_ptr, k_idx);
	i_ptr->timeout = time;
	object_aware(i_ptr, item < 0);
	object_known(i_ptr);
	i_ptr->weight = 0;

	/* Scale the item based on the player's level */

	/* Hack - scale damage */
	if ((level > 8) && (i_ptr->ds)) i_ptr->dd += (level-5)/4;

	/* Hack - scale pvals */
	if (i_ptr->pval) i_ptr->pval += (level / 10);

	/* Hack - scale to hit */
	if (i_ptr->to_h) i_ptr->to_h += (level / 2);

	/* Hack - scale to dam */
	if (i_ptr->to_d) i_ptr->to_d += (level / 2);

	/* Hack - scale to ac */
	if (i_ptr->ac) i_ptr->to_a += (i_ptr->ac * level / 25);

	/* Mark with 'monster race' */
	i_ptr->name3 = r_idx;

	/* Take off existing item */
	if (o_ptr->k_idx)
	{
		/* Check if same spell */
		if ((o_ptr->tval == i_ptr->tval) && (o_ptr->sval == i_ptr->sval))
		{
			/* Reset duration */
			if (o_ptr->timeout < time) o_ptr->timeout = time;

			/* Ensure minimum level of enchantment */
			if (o_ptr->dd < i_ptr->dd) o_ptr->dd = i_ptr->dd;
			if (o_ptr->to_h < i_ptr->to_h) o_ptr->to_h = i_ptr->to_h;
			if (o_ptr->to_d < i_ptr->to_d) o_ptr->to_d = i_ptr->to_d;
			if (o_ptr->to_a < i_ptr->to_a) o_ptr->to_a = i_ptr->to_a;
			if (o_ptr->pval < i_ptr->pval) o_ptr->pval = i_ptr->pval;

			/* Update */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);

			/* And done */
			return;
		}

		/* Take off existing item */
		(void)inven_takeoff(item, 255);
	}

	/* 'Wear' the spell */
	object_copy(o_ptr,i_ptr);

	/* Increment the equip counter by hand */
	p_ptr->equip_cnt++;

	/* Where is the item now */
	if ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)
		|| (o_ptr->tval == TV_HAFTED) || (o_ptr->tval == TV_STAFF) ||
		(o_ptr->tval == TV_DIGGING))
	{
		act = "You are wielding";
		if (item == INVEN_ARM) act = "You are wielding off-handed";
	}
	else if (item == INVEN_WIELD)
	{
		act = "You are using";
	}
	else if (o_ptr->tval == TV_INSTRUMENT)
	{
		act = "You are playing music with";
	}
	else if (item == INVEN_BOW)
	{
		act = "You are shooting with";
	}
	else if (item == INVEN_LITE)
	{
		act = "Your light source is";
	}
	else
	{
		act = "You are wearing";
	}

	/* Describe the result */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Message */
	msg_format("%s %s (%c).", act, o_name, index_to_label(item));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
}

/*
 *  Change shape. Add 'built-in' equipment for that shape.
 */
void change_shape(int shape)
{
	/* Set new shape */
	p_ptr->pshape = shape;

	/* Message */
	if (p_ptr->pshape != p_ptr->prace)
	{
		msg_format("You change into a %s.", p_name + p_info[shape].name);
	}

	/* Update stuff */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
}

/*
 * Enchant item --- a big pile of (fun) hack
 *
 * Basically we select an adjacent item of the same tval,
 * compare the difference in levels between the two, and
 * if randint(plev) > difference, polymorph the item.
 * Otherwise scan until we encounter a 0 value item
 * of the same tval and polymorph the item into that, or
 * generate some junk (50% chance of each).
 * We need to generate junk or caster could infinitely
 * polymorph their items until getting exceedingly good
 * stuff.
 * Note it is dangerous to allow enchant scroll...
 */
static void enchant_item(byte tval, int plev)
{
	object_type *o_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	cptr q,s;
	int dirs,kind,i;

	int item;

	/* Get the local item */
	i_ptr = &object_type_body;

	q = "Enchant which item?";
	s = "You have no items to enchant.";

	/* Restrict choices */
	item_tester_tval = tval;

	/* Get an item */
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

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
	if (o_ptr->tval == TV_BAG)
	{
		/* Get item from bag */
		if (!get_item_from_bag(&item, q, s, o_ptr)) return;

		/* Refer to the item */
		o_ptr = &inventory[item];
	}

	/* Start with same kind */
	i = o_ptr->k_idx;

	/* Randomly increase or decrease */
	if (rand_int(100)<50)
	{
		if (k_info[i-1].tval == tval) dirs = -1;
		else dirs = 1;
	}
	else
	{
		if (k_info[i-1].tval == tval) dirs = 1;
		else dirs = -1;
	}

	/* Check for improvement */
	if (randint(plev) > (k_info[i+dirs].level - k_info[i].level))
	{
		/*Success*/
		kind = i+dirs;
	}
	else if (randint(100) < 50)
	{
		/* Pick bad failure */
		while ((k_info[i].cost) && (k_info[i].tval == tval))
		{
			/* Keep trying */
			i+=dirs;
		}

		/* No bad items downwards */
		if (k_info[i].tval != tval)
		{
			/* Try other direction */
			i = o_ptr->k_idx;
			dirs = -dirs;

			/* Pick bad failure */
			while ((k_info[i].cost) && (k_info[i].tval == tval))
			{
				/* Keep trying */
				i+=dirs;
			}
		}

		/* No bad items at all */
		if (k_info[i].tval !=tval)
		{
			i = o_ptr->k_idx;
		}

		/* No change */
		kind = i;
	}
	else
	{

		/* Pick some junk*/
		switch (tval)
		{
			case TV_POTION:
			{
				kind = lookup_kind(TV_HOLD,1);
				break;
			}
		/* XXX Really need some junk for each of these */
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
			{
				kind = lookup_kind(TV_JUNK,6);
				break;
			}
		/* XXX Odd junk, but can't be food */
		case TV_FOOD:
		case TV_MUSHROOM:
			{
				kind = lookup_kind(TV_JUNK,3);
				break;
			}
		/* XXX Pick meaningless junk */
		default:
			{
				kind = lookup_kind(TV_JUNK,3);
				break;
			}
		}
	}


	/* Ta da! */
	msg_print("There is a blinding flash!");

	/* Destroy an item in the pack */
	if (item >= 0)
	{
		/* Forget about item */
		if (o_ptr->number == 1) inven_drop_flags(o_ptr);

		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Destroy a potion on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Create the new object */
	object_prep(i_ptr, kind);

	/* Carry the object */
	item = inven_carry(i_ptr);

	/* Describe the new object */
	inven_item_describe(item);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

}

/*
 * Create gold
 */
static void create_gold(void)
{
	object_type *i_ptr;
	object_type object_type_body;

	int base;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Create the gold */
	object_prep(i_ptr, lookup_kind(TV_GOLD, 10));

	/* Hack -- Base coin cost */
	base = k_info[i_ptr->k_idx].cost;

	/* Determine how much the treasure is "worth" */
	i_ptr->charges = (s16b)(base + (8 * randint(base)) + randint(8));

	/* Floor carries the item */
	drop_near(i_ptr, 0, p_ptr->py, p_ptr->px, FALSE);

	/* XXX To do thesis on inflation in Angband economies. */
}


/*
 * Curse the players armor
 */
static bool curse_armor(void)
{
	object_type *o_ptr;

	char o_name[80];


	/* Curse the body armor */
	o_ptr = &inventory[INVEN_BODY];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 3);

	/* Attempt a saving throw for artifacts */
	if (artifact_p(o_ptr) && (rand_int(100) < 50))
	{
		/* Cool */
		msg_format("A %s tries to %s, but your %s resists the effects!",
			   "terrible black aura", "surround your armor", o_name);
	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
		msg_format("A terrible black aura blasts your %s!", o_name);

		/* Blast the armor */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_BLASTED;
		o_ptr->to_a = 0 - (s16b)randint(5) - (s16b)randint(5);
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		/* Forget about it */
		inven_drop_flags(o_ptr);
		drop_all_flags(o_ptr);

		/* Curse it */
		o_ptr->ident |= (IDENT_CURSED);

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
	}

	return (TRUE);
}


/*
 * Curse the players weapon
 */
static bool curse_weapon(void)
{
	object_type *o_ptr;

	char o_name[80];


	/* Curse the weapon */
	o_ptr = &inventory[INVEN_WIELD];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 3);

	/* Attempt a saving throw */
	if (artifact_p(o_ptr) && (rand_int(100) < 50))
	{
		/* Cool */
		msg_format("A %s tries to %s, but your %s resists the effects!",
			   "terrible black aura", "surround your weapon", o_name);
	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
		msg_format("A terrible black aura blasts your %s!", o_name);

		/* Shatter the weapon */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_SHATTERED;
		o_ptr->to_h = 0 - (s16b)randint(5) - (s16b)randint(5);
		o_ptr->to_d = 0 - (s16b)randint(5) - (s16b)randint(5);
		o_ptr->to_a = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		/* Drop all flags */
		inven_drop_flags(o_ptr);
		drop_all_flags(o_ptr);

		/* Curse it */
		o_ptr->ident |= (IDENT_CURSED);

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
	}

	/* Notice */
	return (TRUE);
}


/*
 * Turn curses on equipped items into starbursts of nether.  -LM-
 *
 * Idea by Mikko Lehtinen.
 *
 * Todo: Finish implementing this, once remaining Sangband projection types
 * are implemented.
 */
static bool thaumacurse(bool verbose, int power, bool forreal)
{
	int i, dam;
	int curse_count = 0;
	object_type *o_ptr;
	bool notice = FALSE;
	u32b f1, f2, f3, f4;


	/* Count curses */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* All cursed objects have TR3_LIGHT_CURSE.  Perma-cursed objects count double. */
		if (f3 & (TR3_LIGHT_CURSE | TR3_HEAVY_CURSE)) curse_count++;
		if (f3 & (TR3_PERMA_CURSE)) curse_count++;
	}

	/* There are no curses to use */
	if (curse_count == 0)
	{
		if (verbose) msg_print("Your magic fails to find a focus ... and dissipates harmlessly.");

		return (0);
	}

	/* Calculate damage for 1 cursed item (between about 61 and 139) (10x inflation) */
	dam = power * 6;

	/*
	 * Increase damage for each curse found (10x inflation).
	 * Diminishing effect for each, but damage can potentially be very high indeed.
	 */
	dam *= curse_count * 10; /* Was sqrt(curse_count * 100);*/

	/* Deflate */
	dam = dam /100;

	/* Boost player power */
	p_ptr->boost_spell_power = dam;

	/* Boost player number */
	p_ptr->boost_spell_number = 3 + curse_count / 2;

	/* Not for real */
	if (!forreal) return (FALSE);

	/* Break light curses */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Object isn't cursed -- ignore */
		if (!cursed_p(o_ptr)) continue;

		object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* Ignore heavy and permanent curses */
		if (f3 & (TR3_HEAVY_CURSE | TR3_PERMA_CURSE)) continue;

		/* Uncurse the object 1 time in 3 */
		if (rand_int(100) < 33) uncurse_object(o_ptr);

		/* Hack -- Assume felt */
		o_ptr->ident |= (IDENT_SENSE);

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Notice */
		notice = TRUE;
	}

	/* Return "something was noticed" */
	return (notice);
}


/*
 * Get damage from casting spell
 */
static int spell_damage(spell_blow *blow_ptr, int level, u32b flg, bool player, bool forreal)
{
	int damage = 0;

	/* Use player hit points */
	if ((player) && (flg & (PR2_BREATH)))
	{
		/* Damage uses current hit points */
		damage = p_ptr->chp / 3;
	}
	/* Damage uses dice roll */
	else
	{
		/* Roll out the damage */
		if (blow_ptr->d_side)
		{
			int dd = blow_ptr->d_dice;
			int ds = blow_ptr->d_side;

			/* Determine damage */
			damage += (forreal) ? damroll(dd, ds) : ((ds > 1) ? (dd * (ds + 1) / 2) : (dd * ds));
		}

		/* Roll out level dependent damage */
		if (blow_ptr->l_side)
		{
			int dd = blow_ptr->l_dice * level / blow_ptr->levels;
			int ds = blow_ptr->l_side;

			/* Determine damage */
			damage += (forreal) ? damroll(dd, ds) : ((ds > 1) ? (dd * (ds + 1) / 2) : (dd * ds));
		}

		/* Add constant damage */
		damage += blow_ptr->d_plus;

		/* Add level dependent damage */
		if (blow_ptr->l_plus)
		{
			damage += blow_ptr->l_plus * level / blow_ptr->levels;
		}

		/* Add boosted damage */
		if (player) damage += p_ptr->boost_spell_power;
	}

	return (damage);
}


/*
 * Helper function for a variety of spell blow based attacks
 *
 * who, what		Originator of blow
 * x0, y0, x1, y1	Source and destination grids
 * spell			Spell to cast
 * level			Level of spell effect
 * flg				Spell effect flags - overrides spell if != 0L
 * region_id		Type of region to create
 * delay			Delay of initial region effect - used to simulate delaying casting a spell
 * damage_div		Damage divisor - used for coatings
 * one_grid			Effect only a single target grid - for efficiency
 * forreal			Actually do attack - if false we return the damage the attack would do
 * player			Apply player damage bonus from spells
 * retarget			If the player needs an additional target, we set this target using this function
 */
int process_spell_target(int who, int what, int y0, int x0, int y1, int x1, int spell, int level,
		int damage_div, bool one_grid, bool forreal, bool player, bool *cancel,
		bool retarget(int *ty, int *tx, u32b *flg, int method, int level, bool full, bool *one_grid))
{
	spell_type *s_ptr = &s_info[spell];

	bool obvious = FALSE;
	int ap_cnt;
	int ty = y1;
	int tx = x1;
	int damage = 0;
	int region_id = 0;
	int delay = 0;
	int damage_mult = 1;

	bool initial_delay = FALSE;

	/* Hack -- fix damage divisor */
	if (!damage_div) damage_div = 1;
	else if (damage_div < 0)
	{
		damage_mult = -damage_div;
		damage_div = 1;
	}

	/* Get the region identity */
	if ((who == SOURCE_PLAYER_CAST) && ((p_ptr->spell_trap) || (p_ptr->delay_spell)))
	{
		/* Override region */
		region_id = p_ptr->spell_trap;

		/* Delay the effect */
		delay = p_ptr->delay_spell;

		/* Start with a delay */
		initial_delay = TRUE;
	}

	/* Hack -- create spell region */
	else if ((s_ptr->type == SPELL_REGION) ||
			(s_ptr->type == SPELL_SET_TRAP))
	{
		region_id = s_ptr->param;
	}

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		spell_blow *blow_ptr = &s_ptr->blow[ap_cnt];
		int method = blow_ptr->method;
		int effect = blow_ptr->effect;

		method_type *method_ptr = &method_info[method];
		int num = scale_method(method_ptr->number, level);

		s16b region = 0;

		int i;

		u32b flg = method_ptr->flags1;

		/* Hack -- no more attacks */
		if (!method) break;

		/* Hack -- fix number */
		if (!num) num = 1;

		/* Hack -- increase number */
		if (player) num += p_ptr->boost_spell_number;

		/* Hack -- spells don't miss */
		flg &= ~(PROJECT_MISS);

		/* Get the target */
		if ((retarget) && (!retarget(&ty, &tx, &flg, method, level, TRUE, &one_grid) != 0)) return (obvious);

		/* Set target coords */
		y1 = ty;
		x1 = tx;

		/* Get initial damage */
		damage += spell_damage(blow_ptr, level, method_ptr->flags2, player, forreal) * (damage_mult) / (damage_div);

		/* Special case for some spells which affect a single grid or for damage computations */
		if (one_grid || !forreal)
		{
			/* Roll out the damage */
			for (i = 1; i < num; i++) damage += spell_damage(blow_ptr, level, method_ptr->flags2, player, forreal) * (damage_mult) / (damage_div);

			/* Apply once */
			if (forreal)
			{
				/* Note hack to ensure we affect target grid */
				obvious |= project_one(who, what, y1, x1, damage, effect, flg | (PROJECT_PLAY | PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID));
				
				/* Can't cancel */
				*cancel = FALSE;
				
				/* Reset damage */
				damage = 0;
			}

			continue;
		}

		/* Initialise the region */
		if (region_id)
		{
			region_type *r_ptr;

			/* Get a newly initialized region */
			region = init_region(who, what, region_id, damage, method, effect, level, y0, x0, y1, x1);

			/* Get the region */
			r_ptr = &region_list[region];

			/* Hack -- we delay subsequent regions from the same spell casting until the first has expired */
			r_ptr->delay = delay;

			/* Hack -- we only apply one attack to determine region shape. */
			if (r_ptr->flags1 & (RE1_PROJECTION)) num = 1;

			/* Delaying casting of a spell */
			if (initial_delay)
			{
				/* Lasts one turn only */
				r_ptr->lifespan = 1;
			}

			/* Set the life span according to the duration */
			else if ((s_ptr->lasts_dice) && (s_ptr->lasts_side))
			{
				r_ptr->lifespan = damroll(s_ptr->lasts_dice, s_ptr->lasts_side) + s_ptr->lasts_plus;
			}
			else if (s_ptr->lasts_plus)
			{
				r_ptr->lifespan = s_ptr->lasts_plus;
			}

			/* Increase delay - we predict effects of acceleration/deceleration */
			if (r_ptr->flags1 & (RE1_ACCELERATE | RE1_DECELERATE))
			{
				int i;
				int delay_current = r_ptr->delay_reset;

				for (i = 0; i < r_ptr->lifespan; i++)
				{
					delay += delay_current;

					if ((r_ptr->flags1 & (RE1_ACCELERATE)) && (!(r_ptr->flags1 & (RE1_DECELERATE)) || (i < r_ptr->lifespan / 2)))
					{
						if (delay_current < 3) delay_current -= 1;
						delay_current -= delay_current / 3;
						if (delay_current < 1) delay_current = 1;
					}

					/* Decelerating */
					if ((r_ptr->flags1 & (RE1_DECELERATE)) && (!(r_ptr->flags1 & (RE1_ACCELERATE)) || (i > r_ptr->lifespan / 2)))
					{
						delay_current += delay_current / 3;
						if (delay_current < 3) delay_current += 1;
					}
				}
			}
			/* Normal delay - this is the total lifespan of the effect for subsequent effects */
			else
			{
				delay += r_ptr->lifespan * r_ptr->delay_reset;
			}

			/* Display first of newly created regions. Allow features to be seen underneath. */
			if ((!ap_cnt) && (r_ptr->effect != GF_FEATURE)) r_ptr->flags1 |= (RE1_DISPLAY);
		}

		/* Apply multiple times */
		for (i = 0; i < num; i++)
		{
			/* Retarget after first attack */
			if ((i) && (retarget))
			{
				if (!retarget(&ty, &tx, &flg, method, level, FALSE, &one_grid) != 0) return (obvious);
			}

			/* Projection method */
			obvious |= project_method(who, what, method, effect, damage, level, y0, x0, y1, x1, region, flg);

			/* Revise damage */
			if (i < num - 1) damage = spell_damage(blow_ptr, level, method_ptr->flags2, player, forreal) * (damage_mult) / (damage_div);

			/* Reset damage */
			else damage = 0;
		}

		/* Some region tidy up required */
		if (region)
		{
			region_type *r_ptr = &region_list[region];
			region_info_type *ri_ptr = &region_info[r_ptr->type];

			/*
			 * Hack -- Some regions' source is target.
			 *
			 * This is used to prevent projections which have no apparent source from weirdly jumping around
			 * the map, because the target stops being projectable from the invisible source. We set the source
			 * for the projection here, and then keep the source in sync with the target if it moves.
			 */
			if (((((r_ptr->flags1 & (RE1_MOVE_SOURCE)) &&
					(method_ptr->flags1 & (PROJECT_BOOM | PROJECT_4WAY | PROJECT_4WAX | PROJECT_JUMP)) &&
					((method_ptr->flags1 & (PROJECT_ARC | PROJECT_STAR)) == 0))
			/* Hack -- do the same if we are aiming a projection which creates self-targetting blows */
					|| ((method_info[ri_ptr->method].flags1 & (PROJECT_SELF)) != 0))) && (r_ptr->first_piece))
			{
				/* Set source to first projectable location */
				r_ptr->y0 = region_piece_list[r_ptr->first_piece].y;
				r_ptr->x0 = region_piece_list[r_ptr->first_piece].x;

				/* Set target to same */
				r_ptr->y1 = r_ptr->y0;
				r_ptr->x1 = r_ptr->x0;
			}

			/* Notice region? */
			if (obvious && !ap_cnt) r_ptr->flags1 |= (RE1_NOTICE);
			
			/* Refresh the region */
			region_refresh(region);
		}

		/* Can't cancel */
		*cancel = FALSE;

		/* Player successfully delayed casting */
		if ((delay) && (who == SOURCE_PLAYER_CAST))
		{
			/* Clear delay */
			p_ptr->delay_spell = 0;
			p_ptr->spell_trap = 0;
		}
	}

	if (forreal) return(obvious);
	else return (damage);
}



int retarget_blows_dir;
bool retarget_blows_known;
bool retarget_blows_subsequent;
bool retarget_blows_eaten;

/*
 * Allow retargetting of subsequent spell blows
 */
bool retarget_blows(int *ty, int *tx, u32b *flg, int method, int level, bool full, bool *one_grid)
{
	method_type *method_ptr = &method_info[method];
	int range = scale_method(method_ptr->max_range, level);
	int radius = scale_method(method_ptr->radius, level);

	int py = p_ptr->py;
	int px = p_ptr->px;

	/* Never retarget for spells which affect self */
	if (*flg & (PROJECT_SELF))
	{
		*ty = py;
		*tx = px;
	}

	/* If we're not fully retargetting, just continue through the target */
	else if (!full)
	{
		if (!target_okay()) *flg |= (PROJECT_THRU);
	}

	/* Eaten spells use a single target */
	else if (retarget_blows_eaten && (method != RBM_SPIT) && (method != RBM_BREATH))
	{
		*one_grid = TRUE;
		retarget_blows_dir = (method == RBM_VOMIT) ? ddd[rand_int(8)] : 5;

		/* Use the given direction */
		*ty = py + ddy[retarget_blows_dir];
		*tx = px + ddx[retarget_blows_dir];
	}
	
	/* Melee only attacks */
	else if ((method_ptr->flags2 & (PR2_RANGED)) == 0)
	{
		*one_grid = TRUE;
		
		/* Get direction or escape */
		if (!get_rep_dir(&retarget_blows_dir)) return (FALSE);
		
		/* Use the given direction */
		*ty = py + ddy[retarget_blows_dir];
		*tx = px + ddx[retarget_blows_dir];
	}
	
	/* Ranged attacks */
	else
	{
		/* Hack -- get new target if last target is dead / missing */
		if (retarget_blows_subsequent && !(target_okay())) p_ptr->command_dir = 0;

		/* Allow direction to be cancelled for free */
		if (!get_aim_dir(&retarget_blows_dir, TARGET_KILL, retarget_blows_known ? range : MAX_RANGE, retarget_blows_known ? radius : 0,
						retarget_blows_known ? *flg : (PROJECT_BEAM),
						retarget_blows_known ? method_ptr->arc : 0,
						retarget_blows_known ? method_ptr->diameter_of_source : 0)) return (FALSE);

		/* Use the given direction */
		s16b dy = ddy[retarget_blows_dir];
		s16b dx = ddx[retarget_blows_dir];
		*ty = (dy < 0) ? 0 : (py + 99 * dy);
		*tx = (dx < 0) ? 0 : (px + 99 * dx);

		/* Hack -- prevent off-map bugs */
		if(retarget_blows_dir == 7) {
		    *ty = (py > px) ? py - px : 0;
		    *tx = (py > px) ? py - px : 0;
		}

		/* Hack -- Use an actual "target" */
		if ((retarget_blows_dir == 5) && target_okay())
		{
			*ty = p_ptr->target_row;
			*tx = p_ptr->target_col;

			/* Targetting a monster */
			if (cave_m_idx[*ty][*tx])
			{
				/* Any further blows have slightly differing semantics - provided we targetted a monster */
				retarget_blows_subsequent = TRUE;
			}
		}
		/* Stop at first target if we're firing in a direction */
		else if (method_ptr->flags2 & (PR2_DIR_STOP))
		{
			*flg |= (PROJECT_STOP);
		}
	}

	return (TRUE);
}



/*
 *      Process a spell.
 *
 *      Returns -1 iff the spell did something obvious.
 *
 *      Level is set to 0 for most objects, and player level for spells.
 *
 *      *abort is set to TRUE if the user aborts before any spell effects
 *      occur. The return value reflects whether or not they were able to
 *      determine what effect the item has (Currently enchantment and
 *      identify items).
 *
 *      We should choose any item for enchant/curse/identify spells before
 *      calling this routine, when we are using an unknown object, and
 *      only apply the spell, and make the effect obvious, if the object
 *      is correctly chosen. XXX
 *
 *      We should allow useful spells to be cast on adjacent monsters. XXX
 *
 *      We should make summoned monsters friendly. XXX
 *
 */
bool process_spell_blows(int who, int what, int spell, int level, bool *cancel, bool *known, bool eaten)
{
	spell_type *s_ptr = &s_info[spell];
	spell_blow *blow_ptr = &s_ptr->blow[0];

	bool one_grid = FALSE;

	int py = p_ptr->py;
	int px = p_ptr->px;

	/* No blows - return */
	if (!blow_ptr->method) return (FALSE);

	/* Hack - cancel casting of restricted teleports if we are not in the correct starting condition */
	if (method_info[blow_ptr->method].flags1 & (PROJECT_SELF))
	{
		switch (blow_ptr->effect)
		{
			/* Teleport from darkness to darkness */
			case GF_AWAY_DARK:
			{
				if (!teleport_darkness_hook(py, px, py, px))
				{
					if (*known) msg_print("You cannot do that unless you are in darkness.");

					return (FALSE);
				}
				break;
			}

			/* Teleport from water/living to water/living */
			case GF_AWAY_NATURE:
			{
				if (!teleport_nature_hook(py, px, py, px))
				{
					if (*known) msg_print("You cannot do that unless you are next to nature or running water.");

					return (FALSE);
				}
				break;
			}

			/* Teleport from fire/lava to fire/lava */
			case GF_AWAY_FIRE:
			{
				if (!teleport_fiery_hook(py, px, py, px))
				{
					if (*known) msg_print("You cannot do that unless you are next to smoke or flame.");

					return (FALSE);
				}
				break;
			}
		}
	}

	/* Set up retargetting */
	retarget_blows_dir = 0;
	retarget_blows_known = *known;
	retarget_blows_subsequent = FALSE;
	retarget_blows_eaten = eaten;

	/* Cast the spell */
	return (process_spell_target(who, what, py, px, py, px, spell, level, 1, one_grid, TRUE, who == (SOURCE_PLAYER_CAST), cancel, retarget_blows) != 0);
}


/*
 * Process various spell flags.
 */
bool process_spell_flags(int who, int what, int spell, int level, bool *cancel, bool *known)
{
	spell_type *s_ptr = &s_info[spell];

	bool obvious = FALSE;

	int lasts;

	int ench_toh = 0;
	int ench_tod = 0;
	int ench_toa = 0;

	int n, vn = 0;

	cptr vp[64];

	char buf[1024];

	char *s = buf;

	/* Hack to prevent chests being called buried treasure */
	bool hack_chest = FALSE;
	
	/* Roll out the duration */
	if ((s_ptr->lasts_dice) && (s_ptr->lasts_side))
	{
		lasts = damroll(s_ptr->lasts_dice, s_ptr->lasts_side) + s_ptr->lasts_plus;
	}
	else
	{
		lasts = s_ptr->lasts_plus;
	}

	/* Process the flags that apply to equipment */
	if (s_ptr->flags1 & (SF1_IDENT_GAUGE))
	{
		if (!ident_spell_gauge() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_SENSE))
	{
		if (!ident_spell_sense() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_RUNES))
	{
		if (!ident_spell_runes() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_VALUE))
	{
		if (!ident_spell_value() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT))
	{
		if (!ident_spell() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_MAGIC))
	{
		if (!ident_spell_magic() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_RUMOR))
	{
		if (!ident_spell_rumor() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_IDENT_FULLY))
	{
		if (!identify_fully() && (*cancel)) return (TRUE);
		*cancel = FALSE;
		obvious = TRUE;
	}

	/* Process enchantment */
	if (s_ptr->flags1 & (SF1_ENCHANT_TOH))
	{
		if (s_ptr->flags1 & (SF1_ENCHANT_HIGH)) ench_toh = 3+rand_int(4);
		else ench_toh = 1;
	}
	if (s_ptr->flags1 & (SF1_ENCHANT_TOD))
	{
		if (s_ptr->flags1 & (SF1_ENCHANT_HIGH)) ench_tod = 3+rand_int(4);
		else ench_tod = 1;
	}
	if (s_ptr->flags1 & (SF1_ENCHANT_TOA))
	{
		if (s_ptr->flags1 & (SF1_ENCHANT_HIGH)) ench_toa = 3+rand_int(4);
		else ench_toa = 1;
	}

	/* Apply enchantment */
	if (ench_toh || ench_tod || ench_toa)
	{
		if (!(enchant_spell(ench_toh, ench_tod, ench_toa)) && (*cancel))
		{
			return (TRUE);
		}

		*cancel = FALSE;
		obvious = TRUE;
	}

	/* Process word of recall */
	if (s_ptr->flags2 & (SF2_RECALL))
	{
		if (!set_recall() && !(*cancel))
		{
			return (TRUE);
		}
		*cancel = FALSE;
		obvious = TRUE;
	}

	/*** From this point forward, any abilities cannot be cancelled ***/
	if (s_ptr->flags1 || s_ptr->flags2 || (s_ptr->flags3 & ~(SF3_HOLD_SONG | SF3_THAUMATURGY))) *cancel = FALSE;
	
	/* Process the flags -- basic feature detection */
	if ((s_ptr->flags1 & (SF1_DETECT_DOORS)) && (detect_feat_flags(FF1_DOOR, 0x0L, 0x0L, 2 * MAX_SIGHT, known))) vp[vn++] = "doors";
	if ((s_ptr->flags1 & (SF1_DETECT_TRAPS)) && (detect_feat_flags(FF1_TRAP, 0x0L, 0x0L, 2 * MAX_SIGHT, known))) vp[vn++] = "traps";
	if ((s_ptr->flags1 & (SF1_DETECT_STAIRS)) && (detect_feat_flags(FF1_STAIRS, 0x0L, 0x0L, 2 * MAX_SIGHT, known))) vp[vn++] = "stairs";

	/* Process the flags -- basic item detection */
	if ((s_ptr->flags1 & (SF1_DETECT_CURSE)) && (detect_objects_type(item_tester_cursed, 4, INSCRIP_UNCURSED))) vp[vn++] = "cursed items";
	if ((s_ptr->flags1 & (SF1_DETECT_POWER)) && (detect_objects_type(item_tester_power, 5, INSCRIP_AVERAGE))) vp[vn++] = "powerful items";

	/* Process the flags -- basic monster detection */
	if ((s_ptr->flags1 & (SF1_DETECT_EVIL)) && (detect_monsters(monster_tester_hook_evil, known))) vp[vn++] = "evil";
	if ((s_ptr->flags1 & (SF1_DETECT_MONSTER)) && (detect_monsters(monster_tester_hook_normal, known))) vp[vn++] = "monsters";
	
	/* Process the flags -- specific monster detection */
	if (s_ptr->type == SUMMON_RACE)
	{
		/* If summoning a unique that already exists, find it instead */
		if (r_info[s_ptr->param].cur_num >= r_info[s_ptr->param].max_num)
		{
			monster_race_hook = s_ptr->param;
			if (detect_monsters(monster_tester_hook_single, known)) vp[vn++] = r_name + r_info[s_ptr->param].name;
		}
	}

	/* Process the flags -- detect water */
	if (s_ptr->flags1 & (SF1_DETECT_WATER))
	{		
		if (detect_feat_flags(0x0L, FF2_WATER, 0x0L, 2 * MAX_SIGHT, known)) vp[vn++] = "water";
		if (detect_monsters(monster_tester_hook_water, known)) vp[vn++] = "watery creatures";	
	}

	/* Process the flags -- detect fire */
	if (s_ptr->type == SPELL_DETECT_FIRE)
	{
        if (detect_feat_blows(GF_FIRE, 2 * MAX_SIGHT, known)) vp[vn++] = "fire";
        if (detect_feat_blows(GF_SMOKE, 2 * MAX_SIGHT, known)) vp[vn++] = "smoke";
        if (detect_feat_blows(GF_LAVA, 2 * MAX_SIGHT, known)) vp[vn++] = "lava";
        if (detect_monsters(monster_tester_hook_fire, known)) vp[vn++] = "fiery creatures";
	}

	/* Hack to prevent chests being called buried treasure */
	if (s_ptr->flags1 & (SF1_DETECT_GOLD | SF1_DETECT_OBJECT))
	{
        if (detect_feat_flags(0x0L, 0x0L, FF3_CHEST, 2 * MAX_SIGHT, known)) { vp[vn++] = "treasure chests"; hack_chest = TRUE; }
	}
	
	/* Process the flags -- detect gold */
	/* Important: Because when we detect hidden items, we potentially drop gold, we must detect hidden items
	 * before detecting objects */
	if (s_ptr->flags1 & (SF1_DETECT_GOLD))
    {
		/* We still detect buried treasure if we've found chests, we just don't report it */
        if ((detect_feat_flags(FF1_HAS_GOLD, 0x0L, 0x0L, 2 * MAX_SIGHT, known)) && !(hack_chest)) vp[vn++] = "buried treasure";
        if (detect_objects_tval(TV_GOLD)) vp[vn++] = "gold";
        if (detect_objects_tval(TV_GEMS)) vp[vn++] = "gems";
        if (detect_monsters(monster_tester_hook_mineral, known)) vp[vn++] = "mineral creatures";
    }

	/* Process the flags -- detect objects and object monsters */
	/* Important: Because when we detect hidden items, we potentially drop objects, we must detect hidden items
	 * before detecting objects */
	if (s_ptr->flags1 & (SF1_DETECT_OBJECT))
    {
		/* We still detect hidden items if we've found chests, we just don't report it */
        if ((detect_feat_flags(FF1_HAS_ITEM, 0x0L, 0x0L, 2 * MAX_SIGHT, known)) && !(hack_chest)) vp[vn++] = "hidden items";
	    if (detect_objects_normal()) vp[vn++] = "objects";
        if (detect_monsters(monster_tester_hook_mimic, known)) vp[vn++] = "mimics";
    }

	/* Process the flags -- detect magic */
	if (s_ptr->flags1 & (SF1_DETECT_MAGIC))
	{
		if (detect_objects_type(item_tester_magic, 3, INSCRIP_NONMAGICAL)) vp[vn++] = "magic items";
		if (detect_monsters(monster_tester_hook_magic, known)) vp[vn++] = "magical monsters";
	}

	/* Process the flags -- detect life */
	if (s_ptr->flags1 & (SF1_DETECT_LIFE))
	{
		if (detect_feat_flags(0x0L, 0x0L, FF3_LIVING, 2 * MAX_SIGHT, known)) vp[vn++] = "life";
		if (detect_monsters(monster_tester_hook_living, known)) vp[vn++] = "living creatures";
	}

	/* Process the 'fake flag' -- detect minds */
	if ((s_ptr->type == SPELL_DETECT_MIND) || (s_ptr->type == SPELL_MINDS_EYE))
	{
		/* Hack -- forbid cancellation */
		*cancel = FALSE;

		if (detect_monsters(monster_tester_hook_mental, known)) vp[vn++] = "minds";
	}

	/* Process the flags -- reveal secrets */
	if (s_ptr->type == SPELL_REVEAL_SECRETS)
	{
		if (detect_feat_flags(FF1_SECRET, 0x0L, 0x0L, 2 * MAX_SIGHT, known)) vp[vn++] = "secrets";
	}
	
	/* Describe detected magic */
	if (vn)
	{
		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
			if (n == 0) { sprintf(s, "You sense the presence of "); s += strlen("You sense the presence of "); }
			else if (n < vn-1) { sprintf(s, ", "); s += 2; }
			else { sprintf(s, " and "); s += 5; }

			/* Dump */
			sprintf(s, vp[n]);

			s += strlen(vp[n]);
		}

		/* End sentence */
		sprintf(s, ".");

		/* Dump */
		msg_print(buf);

		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_MAP_AREA))
	{
		map_area();
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_WIZ_LITE))
	{
		wiz_lite();
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_LITE_ROOM))
	{
		lite_room(p_ptr->py,p_ptr->px);
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_DARK_ROOM))
	{
		unlite_room(p_ptr->py,p_ptr->px);
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_FORGET))
	{
		lose_all_info();
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_SELF_KNOW))
	{
		self_knowledge(TRUE);
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_ACQUIREMENT))
	{
		acquirement(p_ptr->py,p_ptr->px, 1, TRUE);
		obvious = TRUE;
	}

	if (s_ptr->flags1 & (SF1_STAR_ACQUIREMENT))
	{
		acquirement(p_ptr->py,p_ptr->px, rand_int(2)+1, TRUE);
		obvious = TRUE;
	}

	/* SF2 - timed abilities and modifying */
	/* Note slow poison is silent in its effect */
	if ((s_ptr->flags2 & (SF2_SLOW_POIS)) && (set_slow_poison(p_ptr->timed[TMD_SLOW_POISON] + lasts))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_SLOW_DIGEST)) && (inc_timed(TMD_SLOW_DIGEST, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_INFRA)) && (inc_timed(TMD_INFRA, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_HERO)) && (inc_timed(TMD_HERO, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_SHERO)) && (inc_timed(TMD_BERSERK, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_BLESS)) && (inc_timed(TMD_BLESSED, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_SHIELD)) && (inc_timed(TMD_SHIELD, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_INVIS)) && (inc_timed(TMD_INVIS, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_SEE_INVIS)) && (inc_timed(TMD_SEE_INVIS, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_PROT_EVIL)) && (inc_timed(TMD_PROTEVIL, lasts + (level== 0 ? 3 * p_ptr->lev : 0), TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_FIRE)) && (inc_timed(TMD_OPP_FIRE, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_COLD)) && (inc_timed(TMD_OPP_COLD, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_ACID)) && (inc_timed(TMD_OPP_ACID, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_ELEC)) && (inc_timed(TMD_OPP_ELEC, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_POIS)) && (inc_timed(TMD_OPP_POIS, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags2 & (SF2_OPP_POIS)) && (inc_timed(TMD_OPP_POIS, lasts, TRUE))) obvious = TRUE;

	/* SF3 - timed abilities */
	if ((s_ptr->flags3 & (SF3_PFIX_CURSE)) && (set_timed(TMD_CURSED, p_ptr->timed[TMD_CURSED] / 2, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_CURSE)) && (clear_timed(TMD_CURSED, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_STR)) && (inc_timed(TMD_INC_STR, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_STR)) && (inc_timed(TMD_INC_SIZ, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_INT)) && (inc_timed(TMD_INC_INT, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_WIS)) && (inc_timed(TMD_INC_WIS, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_DEX)) && (inc_timed(TMD_INC_DEX, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_DEX)) && (inc_timed(TMD_INC_AGI, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_CON)) && (inc_timed(TMD_INC_CON, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_INC_CHR)) && (inc_timed(TMD_INC_CHR, lasts, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_FREE_ACT)) && (inc_timed(TMD_FREE_ACT, lasts, TRUE))) obvious = TRUE;

	if (s_ptr->flags2 & (SF2_AGGRAVATE))
	{
		aggravate_monsters(0);
		obvious = TRUE;
	}

	if (s_ptr->flags2 & (SF2_CREATE_STAIR))
	{
		obvious = stair_creation();
	}

	if (s_ptr->flags2 & (SF2_TELE_LEVEL))
	{
		(void)teleport_player_level();
		obvious = TRUE;
	}

	if (s_ptr->flags2 & (SF2_ALTER_LEVEL))
	{
		/* Save the old dungeon in case something goes wrong */
		if (autosave_backup)
			do_cmd_save_bkp();

		p_ptr->create_stair = 0;
		p_ptr->leaving = TRUE;
		obvious = TRUE;
	}

	if (s_ptr->flags2 & (SF2_BANISHMENT))
	{
		(void)banishment(who, what, '\0');
		obvious = TRUE;
	}

	if (s_ptr->flags2 & (SF2_MASS_BANISHMENT))
	{
		mass_banishment(who, what, p_ptr->py, p_ptr->px);
		obvious = TRUE;
	}

	if (s_ptr->flags2 & (SF2_CUT))
	{
		if ((p_ptr->cur_flags2 & (TR2_RES_SHARD)) == 0)
		{
			if (inc_timed(TMD_CUT, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_SHARD,0x0L,0x0L);
			}
		}
		else /* if (obvious) */
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_SHARD,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_STUN))
	{
		if ((p_ptr->cur_flags2 & (TR2_RES_SOUND)) == 0)
		{
			if (inc_timed(TMD_STUN, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_SOUND,0x0L,0x0L);
			}
		}
		else /* if (obvious)*/
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_SHARD,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_POISON))
	{
		if (((p_ptr->cur_flags2 & (TR2_RES_POIS)) == 0) && !(p_ptr->timed[TMD_OPP_POIS]) &&
			(p_ptr->cur_flags4 & (TR4_IM_POIS)) == 0)
		{
			if (inc_timed(TMD_POISONED, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_POIS,0x0L,TR4_IM_POIS);
			}
		}
		else if (!p_ptr->timed[TMD_OPP_POIS]) /* && (obvious) */
		{
			/* Always notice */
			if (p_ptr->cur_flags4 & (TR4_IM_POIS)) equip_can_flags(0x0L,TR2_RES_POIS,0x0L,0x0L);

			/* Always notice */
			else equip_can_flags(0x0L,TR2_RES_POIS,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_BLIND))
	{
		if ((p_ptr->cur_flags2 & (TR2_RES_BLIND)) == 0)
		{
			if (inc_timed(TMD_BLIND, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_BLIND,0x0L,0x0L);
			}
		}
		else /* if (obvious)*/
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_BLIND,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_FEAR))
	{
		if (((p_ptr->cur_flags2 & (TR2_RES_FEAR)) == 0) && (!p_ptr->timed[TMD_HERO]) && (!p_ptr->timed[TMD_BERSERK]))
		{
			if (set_afraid(p_ptr->timed[TMD_AFRAID] + lasts))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_FEAR,0x0L,0x0L);
			}
		}
		else if ((!p_ptr->timed[TMD_HERO])&&(!p_ptr->timed[TMD_BERSERK])) /* && (obvious) */
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_FEAR,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_CONFUSE))
	{
		if ((p_ptr->cur_flags2 & (TR2_RES_CONFU)) == 0)
		{
			if (inc_timed(TMD_CONFUSED, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_CONFU,0x0L,0x0L);
			}
		}
		else /* if (obvious) */
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_CONFU,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_HALLUC))
	{
		if ((p_ptr->cur_flags2 & (TR2_RES_CHAOS)) == 0)
		{
			if (inc_timed(TMD_IMAGE, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,TR2_RES_CHAOS,0x0L,0x0L);
			}
		}
		else /* if (obvious) */
		{
			/* Always notice */
			equip_can_flags(0x0L,TR2_RES_CHAOS,0x0L,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_PARALYZE))
	{
		if (((p_ptr->cur_flags3 & (TR3_FREE_ACT)) == 0) && !(p_ptr->timed[TMD_FREE_ACT]))
		{
			if (inc_timed(TMD_PARALYZED, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,0x0L,TR3_FREE_ACT,0x0L);
			}
		}
		else /* if (obvious) */
		{
			/* Always notice */
			if (!p_ptr->timed[TMD_FREE_ACT]) equip_can_flags(0x0L,0x0L,TR3_FREE_ACT,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_SLOW))
	{
		if (((p_ptr->cur_flags3 & (TR3_FREE_ACT)) == 0) && !(p_ptr->timed[TMD_FREE_ACT]))
		{
			if (inc_timed(TMD_SLOW, lasts, TRUE))
			{
				obvious = TRUE;

				/* Always notice */
				equip_not_flags(0x0L,0x0L,TR3_FREE_ACT,0x0L);
			}
		}
		else /* if (obvious) */
		{
			/* Always notice */
			if (!p_ptr->timed[TMD_FREE_ACT]) equip_can_flags(0x0L,0x0L,TR3_FREE_ACT,0x0L);
		}
	}

	if (s_ptr->flags2 & (SF2_HASTE))
	{
		if (inc_timed(TMD_FAST, p_ptr->timed[TMD_FAST] ? rand_int(s_ptr->lasts_side/3): lasts + level, TRUE)) obvious = TRUE;
	}

	/* SF3 - healing self, and untimed improvements */
	if (s_ptr->flags3 & (SF3_CURE_STR))
	  {
	    if (do_res_stat(A_STR)) obvious = TRUE;
	    if (do_res_stat(A_SIZ)) obvious = TRUE;
	  }
	if ((s_ptr->flags3 & (SF3_CURE_INT)) && (do_res_stat(A_INT))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_WIS)) && (do_res_stat(A_WIS))) obvious = TRUE;
	if (s_ptr->flags3 & (SF3_CURE_DEX))
	  {
	    if (do_res_stat(A_DEX)) obvious = TRUE;
	    if (do_res_stat(A_AGI)) obvious = TRUE;
	  }
	if ((s_ptr->flags3 & (SF3_CURE_CON))  && (do_res_stat(A_CON))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_CHR))  && (do_res_stat(A_CHR))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_EXP)) && (restore_level())) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_FREE_ACT)) && (clear_timed(TMD_SLOW, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_MEM)) && (clear_timed(TMD_AMNESIA, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_PFIX_CURSE)) && (remove_curse())) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_CURSE)) && (remove_all_curse())) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_PFIX_CUTS)) && (pfix_timed(TMD_CUT, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_CUTS)) && (clear_timed(TMD_CUT, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_PFIX_STUN)) && (pfix_timed(TMD_STUN, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_STUN)) && (clear_timed(TMD_STUN, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_POIS)) && (clear_timed(TMD_POISONED, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_CONF)) && (clear_timed(TMD_CONFUSED, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_FOOD)) && (set_food(PY_FOOD_MAX -1))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_FEAR)) && (clear_timed(TMD_AFRAID, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_BLIND)) && (clear_timed(TMD_BLIND, TRUE))) obvious = TRUE;
	if ((s_ptr->flags3 & (SF3_CURE_IMAGE)) && (clear_timed(TMD_IMAGE, TRUE))) obvious = TRUE;


	if (s_ptr->flags3 & (SF3_DEC_EXP))
	{
		/* Undead races are healed instead */
		if (p_ptr->cur_flags4 & TR4_UNDEAD
			&& !p_ptr->timed[TMD_BLESSED])
		{
			obvious = hp_player(300);

			equip_can_flags(0x0L,0x0L,0x0L,TR4_UNDEAD);
		}
		else if ((p_ptr->cur_flags3 & (TR3_HOLD_LIFE)) != 0
				 || p_ptr->timed[TMD_BLESSED]
				 || p_ptr->exp == 0)
		{
			if (!p_ptr->timed[TMD_BLESSED] && p_ptr->exp > 0)
				equip_can_flags(0x0L,0x0L,TR3_HOLD_LIFE,0x0L);
			equip_not_flags(0x0L,0x0L,0x0L,TR4_UNDEAD);
		}
		else
		{
			lose_exp(p_ptr->exp / 4);
			obvious = TRUE;

			equip_not_flags(0x0L,0x0L,TR3_HOLD_LIFE,0x0L);
			equip_not_flags(0x0L,0x0L,0x0L,TR4_UNDEAD);
		}
	}

	if (s_ptr->flags3 & (SF3_DEC_FOOD))
	{
		(void)set_food(PY_FOOD_STARVE - 1);
		obvious = TRUE;
	}

	if (s_ptr->flags3 & (SF3_INC_EXP))
	{
		s32b ee = (p_ptr->exp / 2) + 10;
		if (ee > 100000L) ee = 100000L;
		gain_exp(ee);
		obvious = TRUE;
	}

	return (obvious);

}


#define SPELL_BLOW_HACK	0
#define SPELL_SHOT_HACK	1
#define SPELL_HURL_HACK	2

/*
 * This helper function allows the player to ensorcle a single
 * round of attacks, round of shots or round of shooting with
 * various effects.
 *
 * Returns FALSE if cancelled
 */
bool process_spell_blow_shot_hurl(int type)
{
	switch (type)
	{
		case SPELL_BLOW_HACK:
		{
			int dir;

			/* Allow direction to be cancelled for free */
			if (!get_rep_dir(&dir)) return (FALSE);

			/* Attack */
			py_attack(dir);

			break;
		}
		case SPELL_SHOT_HACK:
		{
			/* Fire */
			do_cmd_item(p_ptr->fire_command);

			/* Cancelled */
			if (!p_ptr->energy_use) return (FALSE);

			break;
		}
		case SPELL_HURL_HACK:
		{
			/* Fire */
			do_cmd_item(COMMAND_ITEM_THROW);

			/* Cancelled */
			if (!p_ptr->energy_use) return (FALSE);

			break;
		}
	}

	return (TRUE);
}



/*
 * Process the spell types.
 *
 * Basically a collection of hacks
 */
bool process_spell_types(int who, int spell, int level, bool *cancel, bool known)
{
	spell_type *s_ptr = &s_info[spell];

	bool obvious = FALSE;

	int lasts;

	int dir;
	int y = p_ptr->py;
	int x = p_ptr->px;
	
	u32b flg = who == SOURCE_PLAYER_CAST ? (MFLAG_ALLY | MFLAG_SHOW | MFLAG_MARK) : 0L;

	/* Roll out the duration */
	if ((s_ptr->lasts_dice) && (s_ptr->lasts_side))
	{
		lasts = damroll(s_ptr->lasts_dice, s_ptr->lasts_side) + s_ptr->lasts_plus;
	}
	else
	{
		lasts = s_ptr->lasts_plus;
	}
	
	/* Hack -- for summoning */
	if (!lasts) lasts = 1;

	/* Has another effect? */
	if (s_ptr->type)
	{
		/* Process the effects */
		switch(s_ptr->type)
		{
			case SPELL_RECHARGE:
			{
				if ((!recharge(s_ptr->param)) && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_IDENT_TVAL:
			{
				if ((!ident_spell_tval(s_ptr->param)) && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_ENCHANT_TVAL:
			{
				enchant_item(s_ptr->param,level);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_BRAND_WEAPON:
			{
				/* Only brand weapons */
				item_tester_hook = item_tester_hook_weapon_strict;

				if (!brand_item(s_ptr->param, "glows brightly") && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_BRAND_ARMOR:
			{
				/* Only brand weapons */
				item_tester_hook = item_tester_hook_armour;

				if (!brand_item(s_ptr->param, "glows brightly") && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_BRAND_ITEM:
			{
				if (!brand_item(s_ptr->param, "glows brightly") && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_BRAND_AMMO:
			{
				/* Only brand ammo */
				item_tester_hook = item_tester_hook_ammo;

				if (!brand_item(s_ptr->param, "glows brightly") && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_WARD_GLYPH:
			{
				warding_glyph();
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_WARD_TRAP:
			{
				if ((!get_rep_dir(&dir)) && (*cancel)) return (FALSE);
				else warding_trap(s_ptr->param,dir);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_AIM_SUMMON:
			case SPELL_AIM_CREATE:
			case SPELL_AIM_SUMMONS:
			{
				/* Get a destination to summon around */
				if (!get_grid_by_aim(TARGET_KILL, &y, &x)) return (FALSE);
				
				/* Fall through */
			}
			case SPELL_CREATE:
			{
				if ((s_ptr->type != SPELL_AIM_SUMMON) && (s_ptr->type != SPELL_AIM_SUMMONS)) flg |= (MFLAG_MADE);
				
				/* Fall through */
			}
			case SPELL_SUMMON:
			case SPELL_SUMMONS:
			{
				bool grp = FALSE;
				int old_monster_level = monster_level;
				int k = 0;
				
				if ((s_ptr->type == SPELL_SUMMONS) || (s_ptr->type == SPELL_AIM_SUMMONS))
				{
					grp = TRUE;
					if (monster_level < 20) monster_level /= 2;
					else monster_level -= 10;
				}
				else if ((s_ptr->type == SPELL_CREATE) || (s_ptr->type == SPELL_AIM_CREATE))
				{
					if (monster_level < 10) monster_level /= 2;
					else monster_level -= 5;
				}
				
				while (lasts-- && summon_specific(y, x, 0, p_ptr->depth+5, s_ptr->param, grp, flg)) k++;
				
				if (k) obvious = TRUE;
				else if (known) msg_print("No such monsters exist at this depth.");
				*cancel = FALSE;
				
				monster_level = old_monster_level;
				break;
			}
			case SPELL_AIM_SUMMON_RACE:
			case SPELL_AIM_CREATE_RACE:
			case SPELL_AIM_SUMMONS_RACE:
			{
				/* Get a destination to summon around */
				if (!get_grid_by_aim(TARGET_KILL, &y, &x)) return (FALSE);
				
				/* Fall through */
			}
			case SPELL_CREATE_RACE:
			{
				if (s_ptr->type != SPELL_AIM_SUMMON_RACE) flg |= (MFLAG_MADE);
				
				/* Fall through */
			}
			case SPELL_SUMMON_RACE:
			case SPELL_SUMMONS_RACE:
			{
				/* Cancel if unique is dead */
				if (!r_info[s_ptr->param].max_num)
				{
					char m_name[80];
					
					/* Get the name */
					race_desc(m_name, sizeof(m_name), s_ptr->param, 0x400, 1);
					
					msg_format("%^s cannot be summoned from beyond the grave.", m_name);
					if (*cancel) return (TRUE);
				}
				/* Summoning a monster */
				else
				{
					int k = 0;
					
					while (lasts-- && summon_specific_one(y, x, s_ptr->param, (s_ptr->type == SPELL_SUMMONS_RACE) || (s_ptr->type == SPELL_AIM_SUMMONS_RACE), flg)) k++;

					if (k) obvious = TRUE;
					else if (known) msg_print("No such monsters exist at this depth.");

					*cancel = FALSE;
				}
				break;
			}
			case SPELL_AIM_SUMMON_GROUP_IDX:
			case SPELL_AIM_CREATE_GROUP_IDX:
			case SPELL_AIM_SUMMONS_GROUP_IDX:
			{
				/* Get a destination to summon around */
				if (!get_grid_by_aim(TARGET_KILL, &y, &x)) return (FALSE);
				
				/* Fall through */
			}
			case SPELL_CREATE_GROUP_IDX:
			{
				if (s_ptr->type != SPELL_AIM_SUMMON_GROUP_IDX) flg |= (MFLAG_MADE);
				
				/* Fall through */
			}
			case SPELL_SUMMON_GROUP_IDX:
			case SPELL_SUMMONS_GROUP_IDX:
			{
				bool grp = FALSE;
				int old_monster_level = monster_level;
				int k = 0;
				
				if ((s_ptr->type == SPELL_SUMMONS_GROUP_IDX) || (s_ptr->type == SPELL_AIM_SUMMONS_GROUP_IDX))
				{
					grp = TRUE;
					if (monster_level < 20) monster_level /= 2;
					else monster_level -= 10;
				}
				else if ((s_ptr->type == SPELL_CREATE_GROUP_IDX) || (s_ptr->type == SPELL_AIM_CREATE_GROUP_IDX))
				{
					if (monster_level < 10) monster_level /= 2;
					else monster_level -= 5;
				}
				
				summon_group_type = s_ptr->param;

				while (lasts-- && summon_specific(y, x, 0, p_ptr->depth+5, SUMMON_GROUP, grp, flg)) k++;
				*cancel = FALSE;
				
				if (k) obvious = TRUE;
				else if (known) msg_print("No such monsters exist at this depth.");

				monster_level = old_monster_level;
				break;
			}
			case SPELL_CREATE_KIND:
			{
				create_gold();
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}
			case SPELL_RAISE_RACE:
			{
				char m_name[80];
				
				/* Get the name */
				race_desc(m_name, sizeof(m_name), s_ptr->param, 0x400, 1);
				
				/* Cancel if unique is alive */
				if (r_info[s_ptr->param].max_num)
				{
					msg_format("%^s is already alive.", m_name);
					if (*cancel) return (TRUE);
				}
				
				/* Summons unique */
				msg_format("%^s has been summoned from beyond the grave.", m_name);
				r_info[s_ptr->param].max_num = 1;
				summon_specific_one(y, x, s_ptr->param, FALSE, flg);
				obvious = TRUE;
				*cancel = FALSE;
				break;
			}
			case SPELL_CURE_DISEASE:
			{
				u32b v;
				u32b old_disease = p_ptr->disease;

				char output[1024];

				/* Hack -- cure disease */
				if (s_ptr->param >= 32)
				{
					/* Cure minor disease */
					v = DISEASE_LIGHT;

					/* Hack -- Cure 'normal disease' */
					if (p_ptr->disease < (1L << DISEASE_TYPES_HEAVY))
					{
						/* Cures all normal diseases */
						v |= (1L << DISEASE_TYPES_HEAVY) - 1;
					}
				}
				/* Cure symptom / specific disease */
				else
				{
					v = (1L << s_ptr->param);
				}

				/* Character not affected by this disease */
				if ((p_ptr->disease & v) == 0)
				{
					/* Message */
					if (p_ptr->disease)
					{
						msg_print("This cure is ineffective for the disease you suffer from.");
						obvious = TRUE;
					}
					break;
				}

				/* Don't allow cancellation */
				*cancel = FALSE;

				/* Mega Hack -- one disease is hard to cure. */
				if ((p_ptr->disease & (1 << DISEASE_SPECIAL)) && (s_ptr->param != DISEASE_SPECIAL))
				{
					msg_print("This disease requires a special cure.");
					return (TRUE);
				}

				/* Cure diseases */
				obvious = TRUE;

				/* Hack -- always cure light diseases by treating any symptom */
				if (p_ptr->disease & (DISEASE_LIGHT))
					p_ptr->disease &= (DISEASE_HEAVY | DISEASE_PERMANENT);
				/* Remove a symptom/cause */
				else
					p_ptr->disease &= ~(v);

				/* Cured all symptoms or cured all origins of disease? */
				/* Note for heavy diseases - curing the symptoms is temporary only */
				if ( ((s_ptr->param >= DISEASE_TYPES_HEAVY) && (p_ptr->disease < (1L << DISEASE_TYPES_HEAVY)))
					|| ( ((p_ptr->disease & ((1L << DISEASE_TYPES_HEAVY) -1)) == 0) && !(p_ptr->disease & (DISEASE_HEAVY)) ) )
				{
					p_ptr->disease &= (DISEASE_PERMANENT);
				}

				/* Redraw disease status */
				p_ptr->redraw |= (PR_DISEASE);

				/* Describe diseases lost */
				disease_desc(output, sizeof(output), old_disease, p_ptr->disease);
				msg_print(output);

				/* Describe new disease */
				if (p_ptr->disease)
				{
					disease_desc(output, sizeof(output), 0x0L, p_ptr->disease);
					msg_print(output);
				}

				break;
			}
			case SPELL_PFIX_CONF:
			{
				*cancel = FALSE;
				if (pfix_timed(TMD_CONFUSED, TRUE)) obvious = TRUE;
				break;
			}
			case SPELL_PFIX_POIS:
			{
				*cancel = FALSE;
				if (pfix_timed(TMD_POISONED, TRUE)) obvious = TRUE;
				break;
			}
			case SPELL_CURSE_WEAPON:
			{
				*cancel = FALSE;
				if (curse_weapon()) obvious = TRUE;
				break;
			}
			case SPELL_CURSE_ARMOR:
			{
				*cancel = FALSE;
				if (curse_armor()) obvious = TRUE;
				break;
			}
			case SPELL_IDENT_PACK:
			{
				*cancel = FALSE;
				identify_pack();
				obvious = TRUE;
				break;
			}
			case SPELL_CHANGE_SHAPE:
			{
				*cancel = FALSE;
				change_shape(s_ptr->param);
				obvious = TRUE;
				break;
			}
			case SPELL_REVERT_SHAPE:
			{
				*cancel = FALSE;
				change_shape(p_ptr->prace);
				obvious = TRUE;
				break;
			}
			case SPELL_REFUEL:
			{
				int item;

				object_type *o_ptr;

				/* Get an item */
				cptr q = "Refuel which torch? ";
				cptr s = "You have no torches.";

				/* Restrict the choices */
				item_tester_hook = item_tester_refill_torch;

				obvious = TRUE;

				if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (TRUE);

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
				if (o_ptr->tval == TV_BAG)
				{
					/* Get item from bag */
					if (!get_item_from_bag(&item, q, s, o_ptr)) return (TRUE);

					/* Refer to the item */
					o_ptr = &inventory[item];
				}

				/* Switch on light source */
				if (o_ptr->charges)
				{
					o_ptr->timeout = o_ptr->charges;
					o_ptr->charges = 0;
				}

				/* Increase fuel */
				o_ptr->timeout += s_ptr->param / o_ptr->number;

				/* Over-fuel message */
				if (o_ptr->timeout >= FUEL_TORCH)
				{
					o_ptr->timeout = FUEL_TORCH;
					msg_format("Your torch%s fully fueled.", o_ptr->number > 1 ? "es are" : " is");
				}

				/* Switch off light source if in inventory*/
				if ((item >= 0) && (item < INVEN_WIELD))
				{
					o_ptr->charges = o_ptr->timeout;
					o_ptr->timeout = 0;
				}

				/* Lite if necessary */
				if (item == INVEN_LITE) p_ptr->update |= (PU_TORCH);
				*cancel = FALSE;
				break;
			}

			case SPELL_IDENT_NAME:
			{
				if ((!ident_spell_name()) && (*cancel)) return (TRUE);
				*cancel = FALSE;
				obvious = TRUE;
				break;
			}

			case SPELL_USE_OBJECT:
			case SPELL_DETECT_FIRE:
			case SPELL_DETECT_MIND:
			{
				/* Spell was processed previously */
				break;
			}

			case SPELL_MINDS_EYE:
			{
				/* This whole spell is a mega-hack of the highest order */
				int ty = p_ptr->py;
				int tx = p_ptr->px;
				int old_py = p_ptr->py;
				int old_px = p_ptr->px;
				int old_lite = p_ptr->cur_lite;
				int old_infra = p_ptr->see_infra;
				u32b old_cur_flags3 = p_ptr->cur_flags3;

				/* Allow direction to be cancelled for free */
				if (!get_aim_dir(&dir, TARGET_KILL, 99, 0, 0, 0, 0)) return (!(*cancel));

				/* Hack -- Use an actual "target" */
				if ((dir == 5) && target_okay())
				{
					ty = p_ptr->target_row;
					tx = p_ptr->target_col;
				}
				else
				{
					bool wall = FALSE;
					
					while (in_bounds_fully(ty, tx))
					{
						/* Predict the "target" location */
						ty += ddy[dir];
						tx += ddx[dir];
						
						/* Get first monster in direction */
						if ((cave_m_idx[ty][tx] > 0) && (m_list[cave_m_idx[ty][tx]].ml)) break;
						
						/* Look on the other side of the first wall found */
						if ((wall) && (cave_project_bold(ty,tx))) break;
						
						/* Look for wall */
						if (!(wall) && !(cave_project_bold(ty, tx))) wall = TRUE;
					}
				}

				/* Paranoia - ensure we have no outstanding updates before messing with
				 * player variables. */
				update_stuff();
				redraw_stuff();

				/* Paranoia - ensure bounds */
				if (in_bounds_fully(ty, tx))
				{
					/* No lite by default */
					p_ptr->cur_lite = 0;

					/* If target is a monster */
					if (cave_m_idx[ty][tx])
					{
						monster_type *m_ptr = &m_list[cave_m_idx[ty][tx]];
						monster_race *r_ptr = &r_info[m_ptr->r_idx];

						/* Hack -- get monster light radius */
						if (((r_ptr->flags2 & (RF2_HAS_LITE)) != 0) ||
							((m_ptr->mflag & (MFLAG_LITE)) != 0))
						{
							/* Get maximum light */
							p_ptr->cur_lite = 2;
						}

						/* Hack - special darknes sight for monsters that don't need lite */
						if ((r_ptr->flags2 & (RF2_NEED_LITE)) == 0)
						{
							/* Get maximum sight */
							p_ptr->see_infra = MIN(MAX_SIGHT, r_ptr->aaf);
						}

						/* Hack - monsters that see invisible */
						if ((r_ptr->flags2 & (RF2_INVISIBLE)) != 0)
						{
							p_ptr->cur_flags3 |= (TR3_SEE_INVIS);
						}

						/* XXX Show player scent?? */
					}
					
					/* Hack -- second sight */
					if (p_ptr->cur_lite < s_ptr->param) p_ptr->cur_lite = s_ptr->param;

					/* Use target location */
					p_ptr->py = ty;
					p_ptr->px = tx;

					/* Update and be paranoid about side-effects */
					p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW |
						PU_MONSTERS | PU_PANEL);
					p_ptr->redraw |= (PR_MAP);

					/* Update display */
					update_stuff();
					redraw_stuff();

					/* Message */
					msg_print("You cast your mind adrift.");
					msg_print("");

					/* Reset hacks */
					p_ptr->py = old_py;
					p_ptr->px = old_px;
					p_ptr->cur_lite = old_lite;
					p_ptr->see_infra = old_infra;
					p_ptr->cur_flags3 = old_cur_flags3;

					/* Update and be paranoid about side-effects */
					p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_DISTANCE |
						PU_MONSTERS | PU_PANEL);
					p_ptr->redraw |= (PR_MAP);

					/* Final update */
					update_stuff();
					redraw_stuff();

					/* Obvious */
					*cancel = FALSE;
					obvious = TRUE;
					break;
				}
			}

			case SPELL_LIGHT_CHAMBERS:
			{
				int x, y, i;

				for (y = 0; y < DUNGEON_HGT; y++)
				{
					for (x = 0; x < DUNGEON_WID; x++)
					{
						/* Light all room grids, but not vaults or hidden rooms.
						 * Note that light chambers _will_ light gloomy rooms. */
						if ((cave_info[y][x] & (CAVE_ROOM)) && !(room_has_flag(y, x, (ROOM_ICKY | ROOM_HIDDEN))))
						{
							cave_info[y][x] |= (CAVE_GLOW);

							/* Light spills out of windows etc. */
							if (f_info[cave_feat[y][x]].flags1 & (FF1_LOS))
							{
								for (i = 0; i < 8; i++)
								{
									cave_info[y + ddy_ddd[i]][x + ddx_ddd[i]] |= (CAVE_GLOW);
								}
							}

							*cancel = FALSE;
							obvious = TRUE;
						}
					}
				}
				break;
			}

			case SPELL_BLOOD_BOND:
			{
				msg_print("Oops. Spell not yet implemented.");
				break;
			}

			case SPELL_RELEASE_CURSE:
			case SPELL_SET_RETURN:
			case SPELL_SET_OR_MAKE_RETURN:
			case SPELL_CONCENTRATE_LITE:
			case SPELL_CONCENTRATE_WATER:
			case SPELL_CONCENTRATE_LIFE:
			{
				/* Implemented elsewhere */
				break;
			}

			case SPELL_REST_UNTIL_DUSK:
			case SPELL_REST_UNTIL_DAWN:
			{
				feature_type *f_ptr = &f_info[cave_feat[y][x]];
				bool notice = FALSE;
				int i;

				/* Hack -- only on the surface for the moment */
				if ((level_flag & (LF1_SURFACE | LF1_TOWN)) == 0)
				{
					msg_print("You cannot tell what time of day or night it is.");
					break;
				}

				/* Hack -- Vampires must be able to hide in the soil */
				if (((f_ptr->flags1 & (FF1_ENTER)) == 0) && ((f_ptr->flags2 & (FF2_CAN_DIG)) == 0))
				{
					msg_print("You cannot rest here.");
					break;
				}

				/* Check that it's day or night */
				if (((level_flag & (LF1_DAYLIGHT)) != 0) == (s_ptr->type == SPELL_REST_UNTIL_DAWN))
				{
					msg_format("It's still %s.", s_ptr->type == SPELL_REST_UNTIL_DAWN ? "daylight" : "night");
					return (TRUE);
				}

				/* Save the old dungeon in case something goes wrong */
				if (autosave_backup)
					do_cmd_save_bkp();

				/* Hack -- Set time to one turn before sun down / sunrise */
				turn += ((10L * TOWN_DAWN) / 2) - (turn % ((10L * TOWN_DAWN) / 2)) - 1;

				/* XXX Set food, etc */

				/* Inform the player */
				if (s_ptr->type == SPELL_REST_UNTIL_DUSK) msg_print("You sleep during the day.");

				/* Heroes don't sleep easy */
				else
				{	switch(p_ptr->lev / 10)
					{
						case 0: msg_print("You awake refreshed and invigorated."); break;
						case 1: msg_print("You toss and turn during the night."); break;
						case 2: msg_print("Your sleep is disturbed by strange dreams."); break;
						case 3: msg_print("You awake with a scream of half-remembered nightmares on your lips."); break;
						case 4: msg_print("You dream of a lidless eye searching unendingly for you, and awake burning with a cold sweat."); break;
						case 5: msg_print("You dream of black mountain crowned with lightning, raging with everlasting anger."); break;
					}
				}

				/* Hack -- regenerate level */
				p_ptr->leaving = TRUE;

				/* Recover one stat point */
				for (i = 0; i < A_MAX; i++)
				{
					if (p_ptr->stat_cur[i]<p_ptr->stat_max[i])
					{
						if (p_ptr->stat_cur[i] < 18) p_ptr->stat_cur[i]++;
						else p_ptr->stat_cur[i] += 10;

						if (p_ptr->stat_cur[i] > p_ptr->stat_max[i]) p_ptr->stat_cur[i] = p_ptr->stat_max[i];

						p_ptr->redraw |= (PR_STATS);

						notice = TRUE;
					}
				}

				if (notice) msg_print("You recover somewhat.");

				*cancel = FALSE;
				obvious = TRUE;

				break;
			}

			case SPELL_MAGIC_BLOW:
			case SPELL_MAGIC_SHOT:
			case SPELL_MAGIC_HURL:
			{
				bool result;

				/* Magical blow */
				p_ptr->branded_blows = s_ptr->param;

				/* Hack - try to assist a couple of things.
				 * Note that a lot of flags don't actually work
				 * such as e.g. stat bonuses. We hackily adjust
				 * blows, shots, might and a 'fake' hurls here,
				 * and use 'melee might' to increase charge bonus.
				 */
				switch (s_ptr->param)
				{
					case 14: case 15:
					{
						p_ptr->num_blow += (p_ptr->lev + 19) / 20;
						p_ptr->num_fire += (p_ptr->lev + 19) / 20;
						p_ptr->num_throw += (p_ptr->lev + 19) / 20;
						break;
					}
					case 16:
					{
						p_ptr->num_charge += (p_ptr->lev + 19) / 20;
						p_ptr->ammo_mult += (p_ptr->lev + 19) / 20;
						break;
					}
					/* Hack -- upgrade slays to executions, where possible */
					case 19:
					{
						if (p_ptr->lev >= 30) p_ptr->branded_blows = 27;
						break;
					}
					case 20:
					{
						if (p_ptr->lev >= 30) p_ptr->branded_blows = 26;
						break;
					}
					case 24:
					{
						if (p_ptr->lev >= 30) p_ptr->branded_blows = 25;
						break;
					}
				}

				/* Allow direction to be cancelled for free */
				result = process_spell_blow_shot_hurl(s_ptr->type - SPELL_MAGIC_BLOW);

				/* End branding */
				p_ptr->branded_blows = 0;

				/* Undo above hacks */
				switch (s_ptr->param)
				{
					case 14: case 15:
					{
						p_ptr->num_blow -= (p_ptr->lev + 19) / 20;
						p_ptr->num_fire -= (p_ptr->lev + 19) / 20;
						p_ptr->num_throw -= (p_ptr->lev + 19) / 20;
						break;
					}
					case 16:
					{
						p_ptr->num_charge -= (p_ptr->lev + 19) / 20;
						p_ptr->ammo_mult -= (p_ptr->lev + 19) / 20;
						break;
					}
				}

				if (!result) return (!(*cancel));

				*cancel = FALSE;
				obvious = TRUE;
				break;
			}

			case SPELL_ACCURATE_BLOW:
			case SPELL_ACCURATE_SHOT:
			case SPELL_ACCURATE_HURL:
			{
				bool result;

				/* Accurate blow */
				p_ptr->to_h += s_ptr->param;

				/* Allow direction to be cancelled for free */
				result = process_spell_blow_shot_hurl(s_ptr->type - SPELL_ACCURATE_BLOW);

				/* End branding */
				p_ptr->to_h -= s_ptr->param;

				if (!result) return (!(*cancel));

				*cancel = FALSE;
				obvious = TRUE;
				break;
			}

			case SPELL_DAMAGING_BLOW:
			case SPELL_DAMAGING_SHOT:
			case SPELL_DAMAGING_HURL:
			{
				bool result;

				/* Damaging blow */
				p_ptr->to_d += s_ptr->param;

				/* Allow direction to be cancelled for free */
				result = process_spell_blow_shot_hurl(s_ptr->type - SPELL_DAMAGING_BLOW);

				/* End branding */
				p_ptr->to_d -= s_ptr->param;

				if (!result) return (!(*cancel));

				*cancel = FALSE;
				obvious = TRUE;
				break;
			}

			case SPELL_REGION:
			case SPELL_SET_TRAP:
			case SPELL_DELAY_SPELL:
			{
				/* Already initialized elsewhere */
				break;
			}

			default:
			{
				if ((s_ptr->type >= INVEN_WIELD) && (s_ptr->type < END_EQUIPMENT))
				{
					wield_spell(s_ptr->type,s_ptr->param,lasts, level, 0);
					*cancel = FALSE;
					obvious = TRUE;
				}
				else
				{
					msg_format("Undefined spell %d.", s_ptr->type);
				}
				break;
			}
		}
	}

	/* Return result */
	return (obvious);
}


/*
 * Hook to determine if a trap is useful.
 */
bool item_tester_hook_magic_trap(const object_type *o_ptr)
{
	/* Check based on tval */
	switch(o_ptr->tval)
	{
		/* All these are good */
		case TV_MUSHROOM:
		case TV_HAFTED:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_DRAG_ARMOR:
		case TV_POTION:
		case TV_SCROLL:
		case TV_FLASK:
		case TV_LITE:
		case TV_SPIKE:
			return (TRUE);
	}

	/* Assume not edible */
	return (FALSE);
}


/* Hack to allow us to record the item used for later */
int set_magic_trap_item;

/*
 * The player sets a magic trap
 */
bool player_set_magic_trap(int item)
{
	/* Hack - record the item selected */
	set_magic_trap_item = item;

	/* Destroy item later */
	return (TRUE);
}




/*
 * Some spell actions have to occur prior to processing the spell
 * proper.
 *
 * We check the spell to see if we have to set a return point as
 * we have to set return points before processing spell blows.
 *
 * We also concentrate spells here, in order to hackily set
 * the spell power for later processing in spell blows.
 *
 * When browsing spells, we should also call this function to
 * correctly initialize the spell for the current circumstances.
 * When doing this forreal is set to FALSE, and interact is set to
 * TRUE if the player is examining the individual spell, and FALSE
 * if just browsing.
 */
bool process_spell_prepare(int spell, int level, bool *cancel, bool forreal, bool interact)
{
	spell_type *s_ptr = &s_info[spell];
	bool obvious = FALSE;

	/* Clear boost */
	p_ptr->boost_spell_power = 0;
	p_ptr->boost_spell_number = 0;

	switch (s_info[spell].type)
	{
		case SPELL_SET_RETURN:
		case SPELL_SET_OR_MAKE_RETURN:
		{
			bool return_time = FALSE;

			/* This affects the game world */
			if (forreal)
			{
				/* Set the return location if required */
				if (!(p_ptr->return_y) && !(p_ptr->return_x))
				{
					/* Set return point */
					p_ptr->return_y = p_ptr->py;
					p_ptr->return_x = p_ptr->px;

					/* Set the return time */
					if (s_ptr->type == SPELL_SET_RETURN) return_time = TRUE;
				}
				/* Set the return time */
				else if (s_ptr->type == SPELL_SET_OR_MAKE_RETURN)
				{
					/* Set the return time */
					return_time = TRUE;
				}

				/* Set return time */
				if (return_time)
				{
					/* Roll out the duration */
					if ((s_ptr->lasts_dice) && (s_ptr->lasts_side))
					{
						p_ptr->word_return = damroll(s_ptr->lasts_dice, s_ptr->lasts_side) + s_ptr->lasts_plus;
					}
					else
					{
						p_ptr->word_return = s_ptr->lasts_plus;
					}
				}

				obvious = TRUE;
				*cancel = FALSE;
			}

			break;
		}

		case SPELL_CONCENTRATE_LITE:
		{
			p_ptr->boost_spell_power = concentrate_power(p_ptr->py, p_ptr->px,
					5 + level / 10, forreal, TRUE, concentrate_light_hook);
			break;
		}

		case SPELL_CONCENTRATE_LIFE:
		{
			p_ptr->boost_spell_power = concentrate_power(p_ptr->py, p_ptr->px,
					5 + level / 10, forreal, FALSE, concentrate_life_hook);
			break;
		}

		case SPELL_CONCENTRATE_WATER:
		{
			p_ptr->boost_spell_power = concentrate_power(p_ptr->py, p_ptr->px,
					5 + level / 10, forreal, FALSE, concentrate_water_hook);
			break;
		}

		case SPELL_RELEASE_CURSE:
		{
			obvious |= thaumacurse(TRUE, 2 * level + level * level / 20, forreal);
			if (obvious) *cancel = FALSE;
			break;
		}

		case SPELL_SUMMON_RACE:
		{
			/* Check if summoning */
			if (forreal)
			{
				/* Choose familiar if summoning it for the first time */
				if ((s_ptr->param == FAMILIAR_IDX) && (!p_ptr->familiar))
				{
					p_ptr->familiar = (byte)randint(19);
					msg_format("You have found %s %s as a familiar.", is_a_vowel(familiar_race[p_ptr->familiar].name[0]) ? "an" : "a",
							familiar_race[p_ptr->familiar].name);

					/* Set the default attributes */
					p_ptr->familiar_attr[0] = familiar_race[p_ptr->familiar].attr1;
					p_ptr->familiar_attr[1] = familiar_race[p_ptr->familiar].attr2;

					improve_familiar();
				}

				/* Obvious */
				*cancel = FALSE;
				obvious = TRUE;
			}
			break;
		}

		case SPELL_SET_TRAP:
		{
			/* You can see how much damage a particular object does when setting it in a trap */
			if (interact)
			{
				int item;
				int power;
				object_type *o_ptr;

				/* Pick the item */
				if (do_cmd_item(COMMAND_ITEM_MAGIC_TRAP))
				{
					/* Get the item */
					item = set_magic_trap_item;

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

					/* Get item effect */
					get_spell(&power, "use", o_ptr, FALSE);

					/* Modify power */
					p_ptr->boost_spell_power = process_item_blow(0, 0, o_ptr, 0, 0, FALSE, FALSE);

					/*
					 * This hack is so ugly, I'm going to have to shower afterwards.
					 */
					s_ptr->blow[0].effect = power ? s_info[power].blow[0].effect : GF_HURT;

					/* Use up the item if required */
					if (forreal)
					{
						/* Destroy an item in the pack */
						if (item >= 0)
						{
							if (o_ptr->number == 1) inven_drop_flags(o_ptr);

							inven_item_increase(item, -1);
							inven_item_describe(item);
							inven_item_optimize(item);
						}

						/* Destroy a food on the floor */
						else
						{
							floor_item_increase(0 - item, -1);
							floor_item_describe(0 - item);
							floor_item_optimize(0 - item);
						}

						/* Window stuff */
						p_ptr->window |= (PW_INVEN | PW_EQUIP);
					}


					/* Done something? */
					*cancel = FALSE;
				}

				/* Obvious */
				obvious = TRUE;
			}
			/* Paranoia */
			else
			{
				/* Not interacting --> no effect */
				s_ptr->blow[0].effect = GF_NOTHING;
			}

			break;
		}

		/* The spell */
		case SPELL_DELAY_SPELL:
		{
			/* Use up the item if required */
			if (forreal)
			{
				/* Choose the effect */
				p_ptr->spell_trap = s_ptr->param;

				/* Set the delay */
				if ((s_ptr->lasts_dice) && (s_ptr->lasts_side))
				{
					p_ptr->delay_spell = damroll(s_ptr->lasts_dice, s_ptr->lasts_side) + s_ptr->lasts_plus;
				}
				else if (s_ptr->lasts_plus)
				{
					p_ptr->delay_spell = s_ptr->lasts_plus;
				}
				/* Fix bug with Delay Spell and Spell trap never being 'used' */
				*cancel = FALSE;
			}
		}
	}

	return (obvious);
}


/*
 * Process spells
 */
bool process_spell(int who, int what, int spell, int level, bool *cancel, bool *known, bool eaten)
{
	bool obvious = FALSE;

	/* Hack -- for 'wonder' spells */
	if (s_info[spell].type == SPELL_USE_OBJECT)
	{
		object_type object_type_body;
		object_type *o_ptr = &object_type_body;

		/* Create a fake object */
		object_prep(o_ptr, s_info[spell].param);

		/* Get a power */
		get_spell(&spell, "use", o_ptr, FALSE);
	}

	/* Inform the player */
	if (strlen(s_text + s_info[spell].text))
	{
		msg_format("%s",s_text + s_info[spell].text);
		obvious = TRUE;
	}

	/* Note the order is important -- because of the impact of blinding a character on their subsequent
		ability to see spell blows that affect themselves */
	if (process_spell_prepare(spell, level, cancel, TRUE, !eaten)) obvious = TRUE;
	if (process_spell_blows(who, what, spell, level, cancel, known, eaten)) obvious = TRUE;
	if (process_spell_flags(who, what, spell, level, cancel, known)) obvious = TRUE;
	if (process_spell_types(who, spell, level, cancel, *known)) obvious = TRUE;

	/* Paranoia - clear boost */
	p_ptr->boost_spell_power = 0;
	p_ptr->boost_spell_number = 0;

	/* Return result */
	return (obvious);
}


/*
 * Apply spell from an item as a blow to one grid and monsters/players, but not objects.
 *
 * If one grid is false, we apply the full from the player to the target grid.
 * Otherwise we only affect the targetted grid.
 *
 * XXX We assume that there is only 1 item in the stack at present.
 *
 * XXX We should never call this routine on an item in inventory (as opposed
 * to equipment or on the floor).
 */
int process_item_blow(int who, int what, object_type *o_ptr, int y, int x, bool forreal, bool one_grid)
{
	int power = 0;
	bool cancel = TRUE;

	int damage = 0;

	/* Get item effect */
	get_spell(&power, "use", o_ptr, FALSE);

	/* Check for return if player */
	if (cave_m_idx[y][x] < 0) process_spell_prepare(power, 25, &cancel, forreal, FALSE);

	/* Clear boosts */
	p_ptr->boost_spell_power = 0;
	p_ptr->boost_spell_number = 0;

	/* Calculate average base damage if guessing */
	if (!forreal)
	{
		int d_dice = o_ptr->dd;
		int d_side = o_ptr->ds;
		int d_plus = o_ptr->to_d;

		/* Base damage */
		damage += d_side > 1 ? (d_dice * (d_side + 1)) / 2 + d_plus : d_dice * d_side + d_plus;
	}

	/* Has a power */
	if (power > 0)
	{
		int damage_div = 1;

		int py = p_ptr->py;
		int px = p_ptr->px;

		bool dummy;

		/* Scale down effects of coatings */
		if (coated_p(o_ptr)) damage_div = 5;

		/* Affect the target */
		damage += process_spell_target(who, what, py, px, y, x, power, 0, damage_div, one_grid, forreal, FALSE, &dummy, NULL);

		/* We've calculated damage? */
		if (!forreal) return (damage);

		/* We no longer have a valid object? */
		if (!o_ptr->k_idx) return (damage);

		/* Object is used */
		if ((k_info[o_ptr->k_idx].used < MAX_SHORT)) k_info[o_ptr->k_idx].used++;

		/* Evaluate coating */
		if (damage && (coated_p(o_ptr)))
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

			k_info[coating].aware &= ~(AWARE_TRIED);

			if (o_ptr->feeling == INSCRIP_COATED) o_ptr->feeling = 0;
		}

		/* Reduce charges */
		if (o_ptr->charges)
		{
			o_ptr->charges--;

			/* Remove coating */
			if (coated_p(o_ptr) && (!o_ptr->charges))
			{
				o_ptr->xtra1 = 0;
				o_ptr->xtra2 = 0;

				if (o_ptr->feeling == INSCRIP_COATED) o_ptr->feeling = 0;
			}
		}

		/* Start recharging item */
		else if (auto_activate(o_ptr))
		{
			if (artifact_p(o_ptr))
			{
				artifact_type *a_ptr = &a_info[o_ptr->name1];

				/* Set the recharge time */
				if (a_ptr->randtime)
				{
					o_ptr->timeout = a_ptr->time + (byte)randint(a_ptr->randtime);
				}
				else
				{
					o_ptr->timeout = a_ptr->time;
				}
			}
			else
			{
				/* Time object out */
				o_ptr->timeout = (s16b)rand_int(o_ptr->charges)+o_ptr->charges;
			}
		}
	}

	/* Anything seen? */
	return(damage);
}


/*
 * Regions are a collection of grids that have a common ongoing effect.
 *
 * We usually define a region to indicate what grids an ongoing effect will possibly hit.
 * We use the projection function to return a list of grids, by passing it the PROJECT_CHCK
 * flag. We then initialise a region based on this list of grids.
 *
 * A region can be traversed in one of two ways:
 *
 * 1. cave_region_piece[y][x] refers to the first region index on top of a stack of regions.
 * Subsequent regions are chained together as a linked list of regions referred to by the
 * next_region_idx of each previous region in the list.
 *
 * 2. ongoing_effect[n] refers to the first region index of all grids that have a common
 * effect applied to them. Subsequent regions are chained together as a linked list of
 * all regions referred by by the next_region_sequence of each previous region in the list.
 *
 * We'll only every add and remove to regions by adding or deleting a list of grids
 * corresponding to an effect. However, if we modify a grid so that it blocks or unblocks
 * line of sight or line of fire (as appropriate) for an effect on that grid, we will
 * need to recompute the list of grids in the region for the ongoing effect.
 *
 * All this complication is required to ensure that we have a list of all ongoing effects
 * that could possibly apply to a particular grid, so that the monster AI can determine
 * whether a grid is safe or not.
 */

/*
 * Excise a region piece from any stacks
 */
void excise_region_piece(s16b region_piece)
{
	int y = region_piece_list[region_piece].y;
	int x = region_piece_list[region_piece].x;

	s16b this_region_piece, next_region_piece = 0;

	s16b prev_region_piece = 0;

	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Done */
		if (this_region_piece == region_piece)
		{
			if (prev_region_piece == 0)
			{
				cave_region_piece[y][x] = next_region_piece;
			}
			else
			{
				region_piece_type *i_ptr;

				/* Previous object */
				i_ptr = &region_piece_list[prev_region_piece];

				/* Remove from list */
				i_ptr->next_in_grid = next_region_piece;
			}

			/* Forget next pointer */
			rp_ptr->next_in_grid = 0;

			/* Done */
			break;
		}

		/* Save prev_o_idx */
		prev_region_piece = this_region_piece;
	}
}


/*
 * Wipe a region piece clean.
 */
void region_piece_wipe(region_piece_type *rp_ptr)
{
	/* Wipe the structure */
	(void)WIPE(rp_ptr, region_piece_type);
}


/*
 * Delete all region pieces when we leave a level.
 */
void wipe_region_piece_list(void)
{
	int i;

	/* Delete the existing objects */
	for (i = 1; i < region_piece_max; i++)
	{
		region_piece_type *rp_ptr = &region_piece_list[i];

		/* Get the location */
		int y = rp_ptr->y;
		int x = rp_ptr->x;

		/* Skip dead region piece */
		if (!rp_ptr->region) continue;

		/* Hack -- see above */
		cave_region_piece[y][x] = 0;

		/* Wipe the region_piece */
		region_piece_wipe(rp_ptr);
	}

	/* Reset "o_max" */
	region_piece_max = 1;

	/* Reset "o_cnt" */
	region_piece_cnt = 0;
}


/*
 * Copy a region piece
 */
void region_piece_copy(region_piece_type *rp_ptr, const region_piece_type *rp_ptr2)
{
	/* Copy the structure */
	(void)COPY(rp_ptr, rp_ptr2, region_piece_type);

	/* Zero some parameters */
	rp_ptr->next_in_region = 0;
	rp_ptr->next_in_grid = 0;
}


/*
 * Move a region piece from index i1 to index i2 in the region piece list
 */
void compact_region_pieces_aux(int i1, int i2)
{
	int i, y, x;

	region_piece_type *rp_ptr;
	region_type *r_ptr;

	/* Do nothing */
	if (i1 == i2) return;

	/* Repair region pieces */
	for (i = 1; i < region_piece_max; i++)
	{
		/* Get the object */
		rp_ptr = &region_piece_list[i];

		/* Skip "dead" objects */
		if (!rp_ptr->region) continue;

		/* Repair "next" pointers */
		if (rp_ptr->next_in_region == i1)
		{
			/* Repair */
			rp_ptr->next_in_region = i2;
		}

		/* Repair "next" pointers */
		if (rp_ptr->next_in_grid == i1)
		{
			/* Repair */
			rp_ptr->next_in_grid = i2;
		}
	}

	/* Repair regions */
	for (i = 1; i < region_max; i++)
	{
		/* Get the object */
		r_ptr = &region_list[i];

		/* Skip "dead" objects */
		if (!r_ptr->type) continue;

		/* Repair "next" pointers */
		if (r_ptr->first_piece == i1)
		{
			/* Repair */
			r_ptr->first_piece = i2;
		}
	}

	/* Get the object */
	rp_ptr = &region_piece_list[i1];

	/* Get location */
	y = rp_ptr->y;
	x = rp_ptr->x;

	/* Repair grid */
	if (cave_region_piece[y][x] == i1)
	{
		/* Repair */
		cave_region_piece[y][x] = i2;
	}

	/* Hack -- move object */
	COPY(&region_piece_list[i2], &region_piece_list[i1], region_piece_type);

	/* Hack -- wipe hole */
	region_piece_wipe(rp_ptr);
}


/*
 * Compact and Reorder the region piece list
 *
 * This function can be very dangerous, use with caution!
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_region_pieces(int size)
{
	int i;

	(void)size;

	/* Excise dead objects (backwards!) */
	for (i = region_piece_max - 1; i >= 1; i--)
	{
		region_piece_type *rp_ptr = &region_piece_list[i];

		/* Skip real region pieces */
		if (rp_ptr->region) continue;

		/* Move last object into open hole */
		compact_region_pieces_aux(region_piece_max - 1, i);

		/* Compress "region_piece_max" */
		region_piece_max--;
	}
}


/*
 * This gets set when we max out region pieces.
 * As the player reduces regions by disarming them,
 * we then attempt to regenerate some regions that
 * may previously not had any contents.
 */
bool region_piece_limit;


/*
 * Get an available region piece
 */
s16b region_piece_pop(void)
{
	int i;

	/* Initial allocation */
	if (region_piece_max < z_info->region_piece_max)
	{
		/* Get next space */
		i = region_piece_max;

		/* Expand region piece array */
		region_piece_max++;

		/* Count region pieces */
		region_piece_cnt++;

		/* Use this region piece */
		return (i);
	}


	/* Recycle dead region pieces */
	for (i = 1; i < region_piece_max; i++)
	{
		region_piece_type *rp_ptr;

		/* Get the region piece */
		rp_ptr = &region_piece_list[i];

		/* Skip live region pieces */
		if (rp_ptr->region) continue;

		/* Count region pieces */
		region_piece_cnt++;

		/* Use this region piece */
		return (i);
	}


	/* Warn the player */
	if (cheat_xtra)
	{
		msg_print("Too many region pieces!");

		/* We use a global to track when it might be possible
		 * to regenerate some regions.
		 */
		region_piece_limit = TRUE;
	}

	/* Oops */
	return (0);
}


/*
 * This returns a region piece from (y,x) for the corresponding
 * region, or 0 if no region piece exists on this grid for this
 * region
 */
int get_region_piece(int y, int x, s16b region)
{
	s16b this_region_piece, next_region_piece = 0;

	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Done */
		if (rp_ptr->region == region)
		{
			return (this_region_piece);
		}
	}

	return (0);
}


/*
 * Wipe a region clean.
 */
void region_wipe(region_type *r_ptr)
{
	/* Wipe the structure */
	(void)WIPE(r_ptr, region_type);
}


/*
 * Delete all regions when we leave a level.
 */
void wipe_region_list(void)
{
	int i;

	/* Delete the existing objects */
	for (i = 1; i < region_max; i++)
	{
		region_type *r_ptr = &region_list[i];

		/* Skip dead region piece */
		if (!r_ptr->type) continue;

		/* Wipe the region */
		region_wipe(r_ptr);
	}

	/* Reset "o_max" */
	region_max = 1;

	/* Reset "o_cnt" */
	region_cnt = 0;
}


/*
 * Move a region piece from index i1 to index i2 in the region piece list
 */
void compact_regions_aux(int i1, int i2)
{
	int i;

	region_piece_type *rp_ptr;
	region_type *r_ptr;

	/* Do nothing */
	if (i1 == i2) return;

	/* Repair region pieces */
	for (i = 1; i < region_piece_max; i++)
	{
		/* Get the object */
		rp_ptr = &region_piece_list[i];

		/* Skip "dead" objects */
		if (!rp_ptr->region) continue;

		/* Repair "next" pointers */
		if (rp_ptr->region == i1)
		{
			/* Repair */
			rp_ptr->region = i2;
		}
	}

	/* Get the object */
	r_ptr = &region_list[i1];

	/* Hack -- move object */
	COPY(&region_list[i2], &region_list[i1], region_type);

	/* Hack -- wipe hole */
	region_wipe(r_ptr);
}


/*
 * Compact and Reorder the region piece list
 *
 * This function can be very dangerous, use with caution!
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_regions(int size)
{
	int i;

	(void)size;

	/* Excise dead objects (backwards!) */
	for (i = region_max - 1; i >= 1; i--)
	{
		region_type *r_ptr = &region_list[i];

		/* Skip real region pieces */
		if (r_ptr->type) continue;

		/* Move last object into open hole */
		compact_regions_aux(region_max - 1, i);

		/* Compress "region_max" */
		region_max--;
	}
}


/*
 * Get an available region
 */
s16b region_pop(void)
{
	int i;

	/* Initial allocation */
	if (region_max < z_info->region_max)
	{
		/* Get next space */
		i = region_max;

		/* Expand region array */
		region_max++;

		/* Count regions */
		region_cnt++;

		/* Use this region */
		return (i);
	}


	/* Recycle dead regions */
	for (i = 1; i < region_max; i++)
	{
		region_type *r_ptr;

		/* Get the region */
		r_ptr = &region_list[i];

		/* Skip live regions */
		if (r_ptr->type) continue;

		/* Count regions */
		region_cnt++;

		/* Use this region */
		return (i);
	}


	/* Warn the player */
	if (cheat_xtra) msg_print("Too many regions!");

	/* Oops */
	return (0);
}


/*
 * Deletes all region pieces which refer to a region
 */
void region_delete(s16b region)
{
	s16b this_region_piece, next_region_piece = 0;

	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Excise region piece */
		excise_region_piece(this_region_piece);

		/* Forget next pointer */
		rp_ptr->next_in_region = 0;

		/* Forget owner */
		rp_ptr->region = 0;
	}

	/* Clear first in sequence */
	region_list[region].first_piece = 0;
}


/*
 * Inserts one region piece into a region
 */
void region_piece_insert(int y, int x, s16b d, s16b region)
{
	int r = region_piece_pop();
	region_piece_type *rp_ptr = &region_piece_list[r];

	/* Paranoia */
	if (!r) return;

	/* Update the region piece with the grid details */
	rp_ptr->d = d;
	rp_ptr->x = x;
	rp_ptr->y = y;
	rp_ptr->region = region;
	rp_ptr->next_in_grid = cave_region_piece[y][x];
	rp_ptr->next_in_region = region_list[region].first_piece;
	cave_region_piece[y][x] = r;
	region_list[region].first_piece = r;

}


/*
 * Makes a copy of a region, excluding the region pieces
 */
void region_copy(region_type *r_ptr, const region_type *r_ptr2)
{
	/* Copy the structure */
	(void)COPY(r_ptr, r_ptr2, region_type);

	/* Zero some parameters */
	r_ptr->first_piece = 0;
}


/*
 * Takes a copy of the region pieces and adds it to another region.
 *
 * If region2 is 0, a new region is created.
 */
s16b region_copy_pieces(s16b region, s16b region2)
{
	s16b this_region_piece, next_region_piece = 0;

	int new_region = region2 ? region2 : region_pop();

	if (!new_region) return (0);

	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Copy the region pieces */
		region_piece_insert(rp_ptr->y, rp_ptr->x, rp_ptr->d, new_region);
	}

	return (new_region);
}



/*
 * Inserts the grids of a region into the region pieces
 */
void region_insert(u16b *gp, int grid_n, s16b *gd, s16b region)
{
	int i;

	for (i = 0; i < grid_n; i++)
	{
		int y = GRID_Y(gp[i]);
		int x = GRID_X(gp[i]);

		region_type *r_ptr = &region_list[region];

		/* Hack -- don't overwrite the 'parent' trap */
		if (r_ptr->flags1 & (RE1_HIT_TRAP))
		{
			if ((y == r_ptr->y0) && (x == r_ptr->x0) && (f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP))) continue;
		}

		/* Update the region piece with the grid details */
		region_piece_insert(y, x, gd[i], region);
	}
}


/*
 * Iterate through the regions in a particular grid, applying a region_hook function to each region that
 * occupies the grid.
 */
bool region_grid(int y, int x, bool region_iterator(int y, int x, s16b d, s16b region))
{
	s16b this_region_piece, next_region_piece = 0;
	bool seen = FALSE;

	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Iterate on region piece */
		seen |= region_iterator(y, x, rp_ptr->d, rp_ptr->region);
	}

	return (seen);
}


/*
 * Iterate through a region, applying a region_hook function to each grid in the region
 */
bool region_iterate(s16b region, bool region_iterator(int y, int x, s16b d, s16b region))
{
	s16b this_region_piece, next_region_piece = 0;
	bool seen = FALSE;

	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Iterate on region piece */
		seen |= region_iterator(rp_ptr->y, rp_ptr->x, rp_ptr->d, region);
	}

	return (seen);
}


/*
 * Iterate through a region, applying a region_hook function to each grid in the region
 *
 * Similar to the above region_iterate, but we only affect grids within d distance of (y,x)
 * if gt is true, or further than d distance if gt is false. We also check for LOS.
 */
bool region_iterate_distance(s16b region, int y, int x, s16b d, bool gt, bool region_iterator(int y, int x, s16b d, s16b region))
{
	method_type *method_ptr = &method_info[region_list[region].method];
	s16b this_region_piece, next_region_piece = 0;
	bool seen = FALSE;

	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Check distance */
		if ((distance(y, x, rp_ptr->y, rp_ptr->x) > d) == gt) continue;

		/* Check projectable */
		if (!generic_los(y, x, rp_ptr->y, rp_ptr->x, method_ptr->flags1 & (PROJECT_LOS) ? CAVE_XLOS : CAVE_XLOF)) continue;

		/* Iterate on region piece */
		seen |= region_iterator(rp_ptr->y, rp_ptr->x, rp_ptr->d, region);
	}

	return (seen);
}


/*
 * Hook for redrawing all grids in a region.
 */
bool region_refresh_hook(int y, int x, s16b d, s16b region)
{
	(void)d;
	(void)region;

	lite_spot(y, x);

	/* Notice elsewhere */
	return (FALSE);
}


/*
 * Redraw the region
 */
void region_refresh(s16b region)
{
	region_iterate(region, region_refresh_hook);
}


#if 0

/*
 * Hook for redrawing all grids in a region.
 */
bool region_debug_hook(int y, int x, s16b d, s16b region)
{
	(void)d;
	(void)region;

	/* Visual effects */
	print_rel('*', TERM_GREEN, y, x);
	move_cursor_relative(y, x);
	Term_fresh();
	Term_xtra(TERM_XTRA_DELAY, op_ptr->delay_factor * op_ptr->delay_factor);
	lite_spot(y, x);
	Term_fresh();


	/* Notice elsewhere */
	return (FALSE);
}


/*
 * Redraw the region
 */
void region_debug(s16b region)
{
	region_iterate(region, region_debug_hook);

	anykey();
}
#endif


/*
 * Highlight the region. We do this by moving the first region pieces to the top of the stack.
 */
bool region_highlight_hook(int y, int x, s16b d, s16b region)
{
	s16b this_region_piece, next_region_piece = 0;

	s16b prev_region_piece = 0;

	(void)d;

	/* Iterate through stack of grids */
	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Done */
		if (region_piece_list[this_region_piece].region == region)
		{
			if (prev_region_piece == 0)
			{
				cave_region_piece[y][x] = next_region_piece;
			}
			else
			{
				region_piece_type *i_ptr;

				/* Previous object */
				i_ptr = &region_piece_list[prev_region_piece];

				/* Remove from list */
				i_ptr->next_in_grid = next_region_piece;
			}

			/* Remember first on stack */
			rp_ptr->next_in_grid = next_region_piece;

			/* Bring to top of stack */
			cave_region_piece[y][x] = this_region_piece;

			/* Done */
			break;
		}

		/* Save prev_o_idx */
		prev_region_piece = this_region_piece;
	}

	return(FALSE);
}


/*
 * Highlight the region
 */
void region_highlight(s16b region)
{
	region_iterate(region, region_highlight_hook);
}



/*
 * Hook for setting temp grids for region
 */
bool region_mark_temp_hook(int y, int x, s16b d, s16b region)
{
	(void)d;
	(void)region;

	/* Verify space */
	if (temp_n == TEMP_MAX) return (FALSE);

	/* Mark the grid */
	play_info[y][x] |= (PLAY_TEMP);

	/* Add it to the marked set */
	temp_y[temp_n] = y;
	temp_x[temp_n] = x;
	temp_n++;

	/* Notice elsewhere */
	return (FALSE);
}


/*
 * Set PLAY_TEMP on all grids in region
 */
void region_mark_temp(s16b region)
{
	region_iterate(region, region_mark_temp_hook);
}



/*
 * Updates a region based on the shape of the defined projection.
 */
void region_update(s16b region)
{
	region_type *r_ptr = &region_list[region];
	method_type *method_ptr = &method_info[r_ptr->method];

	int type = r_ptr->type;

	u32b flg = method_ptr->flags1;

	/* Fake clearing the old grids */
	r_ptr->type = 0;

	/* Clear the grids */
	region_refresh(region);

	/* Unfake */
	r_ptr->type = type;

	/* Hack - remove previous list of grids */
	region_delete(region);

	/* Hack -- we try to force regions to have a useful effect */
	if ((flg & (PROJECT_4WAY | PROJECT_4WAX | PROJECT_BOOM)) == 0) flg |= (PROJECT_BEAM | PROJECT_THRU);

	/* Hack - add regions */
	project_method(r_ptr->who, r_ptr->what, r_ptr->method, r_ptr->effect, r_ptr->damage, r_ptr->level, r_ptr->y0, r_ptr->x0, r_ptr->y1, r_ptr->x1, region, flg);

	/* Redraw grids */
	region_refresh(region);

	/* Check region for triggers */
	if ((r_ptr->flags1 & (RE1_TRIGGER_MOVE)) && !(r_ptr->flags1 & (RE1_TRIGGERED)))
	{
		s16b this_region_piece, next_region_piece = 0;
		int y = 0;
		int x = 0;
		int k = 0;

		for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
		{
			/* Get the region piece */
			region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

			/* Get the next object */
			next_region_piece = rp_ptr->next_in_region;

			/* Empty grids don't trigger anything */
			if (cave_m_idx[rp_ptr->y][rp_ptr->x] == 0)
			{
				continue;
			}

			/* Some traps are avoidable */
			else if (r_ptr->flags1 & (RE1_HIT_TRAP))
			{
				/* Some traps are avoidable by monsters*/
				if ((cave_m_idx[rp_ptr->y][rp_ptr->x] > 0) && (mon_avoid_trap(&m_list[cave_m_idx[rp_ptr->y][rp_ptr->x]],y, x, cave_feat[r_ptr->y0][r_ptr->x0])))
				{
					continue;
				}
				/* Some traps are avoidable by the player */
				else if ((cave_m_idx[rp_ptr->y][rp_ptr->x] < 0) && (avoid_trap(y, x, cave_feat[r_ptr->y0][r_ptr->x0])))
				{
					continue;
				}
			}

			/* Trigger the region */
			if (!rand_int(++k))
			{
				y = rp_ptr->y;
				x = rp_ptr->x;
			}
		}

		/* Valid target */
		if (k)
		{
			trigger_region(y, x, TRUE);
		}
	}

	/* Update the facing for various projections */
	/* XXX Try to avoid updating clockwise/anticlockwise to avoid compounding rounding errors */
	if (r_ptr->flags1 & (RE1_MOVE_SOURCE | RE1_SEEKER | RE1_RANDOM | RE1_SPREAD | RE1_INVERSE | RE1_CHAIN))
	{
		r_ptr->facing = get_angle_to_target(r_ptr->y0, r_ptr->x0, r_ptr->y1, r_ptr->x1, 0);
	}
}


/*
 * This routine writes the current feat under the grid to the scalar
 * for that region. This is used to overlay temporary terrain onto a region,
 * then restore it later using region_restore_hook.
 */
bool region_uplift_hook(int y, int x, s16b d, s16b region)
{
	region_type *r_ptr = &region_list[region];
	s16b region_piece = get_region_piece(y, x, region);

	region_piece_type *rp_ptr = &region_piece_list[region_piece];

	int this_region_piece, next_region_piece = 0;

	(void)d;

	/* Paranoia */
	if (!region_piece) return (FALSE);

	/* Start getting features */
	if ((r_ptr->flags1 & (RE1_SCALAR_DAMAGE | RE1_SCALAR_DISTANCE | RE1_SCALAR_VECTOR)) == 0)
	{
		r_ptr->flags1 |= (RE1_SCALAR_FEATURE);
	}

	/* Paranoia */
	if ((r_ptr->flags1 & (RE1_SCALAR_FEATURE))== 0) return (FALSE);

	/* Have we already collected a feature */
	if (rp_ptr->d) return (FALSE);

	/* Check overlapping regions */
	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr2 = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr2->next_in_grid;

		/* Skip itself */
		if (rp_ptr2->region == region) continue;

		/* Skip if not a region piece */
		if ((r_ptr->flags1 & (RE1_SCALAR_FEATURE))== 0) continue;

		/* We have something else collecting the underlying feature. Use it's value instead */
		rp_ptr->d = rp_ptr2->d;
		return (FALSE);
	}

	/* Collect this feature */
	rp_ptr->d = cave_feat[y][x];

	/* Notice elsewhere */
	return (FALSE);
}


/*
 * Sets the feature for a grid, based on the scalar set for the
 * region of that grid. Use region_uplift_hook to set the region.
 */
bool region_restore_hook(int y, int x, s16b d, s16b region)
{
	region_type *r_ptr = &region_list[region];
	int this_region_piece, next_region_piece = 0;

	(void)region;

	/* Paranoia */
	if ((r_ptr->flags1 & (RE1_SCALAR_FEATURE))== 0) return (FALSE);

	/* Did we collect anything? */
	if (d)
	{
		/* Check overlapping regions */
		for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
		{
			region_piece_type *rp_ptr2 = &region_piece_list[this_region_piece];

			/* Get the next object */
			next_region_piece = rp_ptr2->next_in_grid;

			/* Skip itself */
			if (rp_ptr2->region == region) continue;

			/* Skip if not a region piece */
			if ((r_ptr->flags1 & (RE1_SCALAR_FEATURE))== 0) continue;

			/* We have something else collecting the underlying feature. Don't write this value down. */
			return (FALSE);
		}

		/* Restore the terrain */
		cave_set_feat(y, x, d);
	}

	/* Cave_set_feat handles the player noticing this */
	return (FALSE);
}


/*
 * Apply effect to every grid. Note that we have to apply the
 * project_t() effect separately. This is done by setting the
 * PROJECT_TEMP flag.
 */
bool region_project_hook(int y, int x, s16b d, s16b region)
{
	region_type *r_ptr = &region_list[region];
	method_type *method_ptr = &method_info[r_ptr->method];

	int dam = 0;
	bool notice;

	/* If temporarily marked, skip to prevent double-hitting any occupants */
	if (play_info[y][x] & (PLAY_TEMP)) return (FALSE);
	
	/* Get the feature from the region */
	if (r_ptr->effect == GF_FEATURE)
	{
		/* Always use base damage */
		dam = r_ptr->damage;

		/* Collecting the feature */
		region_uplift_hook(y, x, d, region);
	}

	/* Get the damage from the region piece */
	else if (r_ptr->flags1 & (RE1_SCALAR_DAMAGE))
	{
		dam = d;
	}
	/* Compute the damage based on distance */
	else if ((d) && (r_ptr->flags1 & (RE1_SCALAR_DISTANCE)))
	{
		dam = r_ptr->damage / d;
	}
	/* Get the damage from the region */
	else
	{
		dam = r_ptr->damage;
	}

	/* Apply projection effects to the grids */
	notice = project_one(r_ptr->who, r_ptr->what, y, x, dam, r_ptr->effect, method_ptr->flags1 | (PROJECT_TEMP | PROJECT_HIDE));

	/* Mark the grid if occupied to prevent double-hits */
	if (cave_m_idx[y][x]) cave_temp_mark(y, x, FALSE);
	
	return (notice);
}


/*
 * Apply project_t to every grid. Note that we have to apply the
 * project_t() effect separately.
 */
bool region_project_t_hook(int y, int x, s16b d, s16b region)
{
	region_type *r_ptr = &region_list[region];

	int dam = 0;
	bool notice;

	/* Using a feature */
	if (r_ptr->effect == GF_FEATURE)
	{
		dam = d;
	}
	/* Get the damage from the region piece */
	else if (r_ptr->flags1 & (RE1_SCALAR_DAMAGE))
	{
		dam = d;
	}
	/* Compute the damage based on distance */
	else if ((d) && (r_ptr->flags1 & (RE1_SCALAR_DISTANCE)))
	{
		dam = r_ptr->damage / d;
	}
	/* Get the damage from the region */
	else
	{
		dam = r_ptr->damage;
	}

	/* Apply projection effects to the grids */
	notice = project_t(r_ptr->who, r_ptr->what, y, x, dam, r_ptr->effect);

	return (notice);
}


/*
 * Hack - region we are copying this one to.
 */
s16b region_copy_to;


/*
 * Copy the pieces to a new region.
 */
bool region_copy_pieces_hook(int y, int x, s16b d, s16b region)
{
	(void)region;

	/* Copy the region pieces */
	region_piece_insert(y, x, d, region_copy_to);

	return (FALSE);
}



/*
 * Kill a region.
 */
void region_terminate(s16b region)
{
	region_type *r_ptr = &region_list[region];

	/* Effect is "dead" - mark for later */
	r_ptr->type = 0;

	/* Restore underlying terrain */
	region_iterate(region, region_restore_hook);

	/* Redraw region */
	region_refresh(region);

	/* Clear the grids associated with the region */
	region_delete(region);

	/* Wipe the region clean */
	region_wipe(r_ptr);
}


/*
 * Pick a random piece from a region.
 */
s16b region_random_piece(s16b region)
{
	s16b this_region_piece, next_region_piece = 0;
	int k = 0;

	/*
	 * Note for efficiency, we iterate through twice. This means we only generate one random number, as opposed to
	 * one random number for each grid checked.
	 */

	/* Count grids we could effect */
	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Count the grids */
		k++;
	}

	/* Pick random grid. */
	if (k) k = randint(k);

	/* Get the coordinates of the grid choosen */
	for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece, k--)
	{
		region_piece_type *rp_ptr;

		/* Get the object */
		rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Get coordinates */
		if (k == 1)
		{
			return (this_region_piece);
		}
	}

	return(0);
}


/*
 * Apply effect to a region
 *
 * If (y, x) defined, use this as the target.
 * If RE1_RANDOM, pick a random piece from the grid.
 * If RE1_INVERSE, reverse the source and destination. If combine with RANDOM
 * we randomize both the source and destination.
 * If RE1_CLOSEST_MON, choose the closest monster as a target.
 * If RE1_CHAIN, update the region after affecting it.
 * If RE1_SOURCE_FEATURE is used, attack multiple times,
 * once from each feature in the region.
 *
 * Note the concept of target only applies if we are using
 * a method, or spawning a region, not if we affect every
 * grid in the region.
 *
 * XXX It is important to realise that discharge_trap
 * called from here can result in an invalidated region.
 * Therefore the caller needs to check r_ptr->type > 0,
 * before attempting to refer to the region after calling
 * this routine.
 */
bool region_effect(int region, int y, int x)
{
	region_type *r_ptr = &region_list[region];
	region_info_type *ri_ptr = &region_info[r_ptr->type];

	bool notice = FALSE;

	int child_region = 0;

	/* Paranoia */
	if (!r_ptr->type) return (FALSE);

	/* Create a child region */
	if (r_ptr->child_region)
	{
		/* Get a newly initialized region */
		child_region = init_region(r_ptr->who, r_ptr->what, r_ptr->child_region, r_ptr->damage, r_ptr->method, r_ptr->effect, r_ptr->level, r_ptr->y0, r_ptr->x0, y, x);
	}

	/* A real trap */
	if ((r_ptr->flags1 & (RE1_HIT_TRAP)) && (f_info[cave_feat[r_ptr->y0][r_ptr->x0]].flags1 & (FF1_HIT_TRAP)))
	{
		/* Discharge the trap */
		notice |= discharge_trap(r_ptr->y0, r_ptr->x0, y, x, child_region);
	}
	else
	{
		int y0 = r_ptr->y0;
		int x0 = r_ptr->x0;
		int y1 = y ? y : r_ptr->y1;
		int x1 = x ? x : r_ptr->x1;

		/* Attacks come from multiple source features in the region */
		if (r_ptr->flags1 & (RE1_SOURCE_FEATURE))
		{
			s16b this_region_piece, next_region_piece = 0;

			for (this_region_piece = region_list[region].first_piece; this_region_piece; this_region_piece = next_region_piece)
			{
				/* Get the region piece */
				region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

				/* Get the next object */
				next_region_piece = rp_ptr->next_in_region;

				/* Skip if it doesn't match source feature */
				if ((r_ptr->who != SOURCE_FEATURE) && (r_ptr->who != SOURCE_PLAYER_TRAP)) continue;
				if (r_ptr->what != cave_feat[rp_ptr->y][rp_ptr->x]) continue;

				/* Copy to child region */
				if (child_region)
				{
					region_copy_to = child_region;

					notice |= region_iterate_distance(region, rp_ptr->y, rp_ptr->x, r_ptr->age, TRUE, region_copy_pieces_hook);
				}
				/* Hack -- restore original terrain if going backwards */
				else if ((r_ptr->flags1 & (RE1_BACKWARDS)) && (r_ptr->flags1 & (RE1_SCALAR_FEATURE)))
				{
					notice |= region_iterate_distance(region, rp_ptr->y, rp_ptr->x, r_ptr->age, FALSE, region_restore_hook);
				}
				/* Apply projection to region */
				else
				{
					/* Apply projection effects to the grids */
					notice |= region_iterate_distance(region, rp_ptr->y, rp_ptr->x, r_ptr->age, TRUE, region_project_hook);

					/* Apply temporary effects to grids */
					notice |= region_iterate_distance(region, rp_ptr->y, rp_ptr->x, r_ptr->age, TRUE, region_project_t_hook);

					/* Clear temporary array */
					clear_temp_array();
				}
			}
		}

		/* Method is defined */
		else if (ri_ptr->method)
		{
			method_type *method_ptr = &method_info[ri_ptr->method];

			/* Pick a random source grid if required */
			if ((r_ptr->flags1 & (RE1_RANDOM | RE1_INVERSE)) == (RE1_RANDOM | RE1_INVERSE))
			{
				int r = region_random_piece(region);

				y0 = region_piece_list[r].y;
				x0 = region_piece_list[r].x;
			}

			/* Reverse destination and source */
			else if (r_ptr->flags1 & (RE1_INVERSE))
			{
				y = y0;
				x = x0;
				y0 = y1;
				x0 = x1;
				y1 = y;
				x1 = x;
			}
			
			/* Pick a random grid if required */
			if (r_ptr->flags1 & (RE1_RANDOM))
			{
				int r = region_random_piece(region);

				y1 = region_piece_list[r].y;
				x1 = region_piece_list[r].x;
			}

			/* Attack the target */
			notice |= project_method(r_ptr->who, r_ptr->what, ri_ptr->method, r_ptr->effect, r_ptr->damage, r_ptr->level, y0, x0, y1, x1, child_region, method_ptr->flags1);
		}

		/* Method is not defined. Apply effect to all grids */
		else
		{
			/* Copy to child region */
			if (child_region)
			{
				region_copy_to = child_region;

				notice |= region_iterate(region, region_copy_pieces_hook);
			}
			else
			{
				/* Apply projection effects to the grids */
				notice |= region_iterate(region, region_project_hook);

				/* Apply temporary effects to grids */
				notice |= region_iterate(region, region_project_t_hook);

				/* Clear temporary array */
				clear_temp_array();
			}
		}

		/* Region chains */
		if (r_ptr->flags1 & (RE1_CHAIN))
		{
			r_ptr->y0 = y0;
			r_ptr->x0 = x0;
			r_ptr->y1 = y1;
			r_ptr->x1 = x1;

			region_update(region);
		}

		/* Noticed region? */
		if (notice)
		{
			bool refresh = (r_ptr->flags1 & (RE1_NOTICE)) == 0;

			/* Mark region noticed */
			r_ptr->flags1 |= (RE1_NOTICE | RE1_DISPLAY);

			/* Refresh required */
			if (refresh) region_refresh(region);
		}
	}

	/* Define child region */
	if (child_region)
	{
		region_type *r_ptr = &region_list[child_region];

		/* Hack -- lifespan */
		r_ptr->lifespan = scale_method(ri_ptr->child_lasts, r_ptr->level);

		/* Display newly created regions. Allow features to be seen underneath. */
		if (r_ptr->effect != GF_FEATURE) r_ptr->flags1 |= (RE1_DISPLAY);
		if (notice) r_ptr->flags1 |= (RE1_NOTICE);

		region_refresh(child_region);
	}

	return(notice);
}


/*
 * Trigger regions at grid y, x when a monster moves through it.
 *
 * This is used to fire traps if a player or monster moves into them,
 * as well as handle these moving through lingering effects.
 *
 * If move is true, we check for TRIGGER_MOVE, otherwise we check for
 * TRIGGER_DROP.
 */
void trigger_region(int y, int x, bool move)
{
	int this_region_piece, next_region_piece = 0;
	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];
		region_type *r_ptr = &region_list[rp_ptr->region];
		method_type *method_ptr = &method_info[r_ptr->method];

		bool notice = FALSE;
		
		int ty = y;
		int tx = x;

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Paranoia */
		if (!r_ptr->type) continue;
		
		/* Shoot grid with effect */
		if ((move ? ((r_ptr->flags1 & (RE1_TRIGGER_MOVE)) != 0) : ((r_ptr->flags1 & (RE1_TRIGGER_DROP)) != 0)) &&
				((r_ptr->flags1 & (RE1_TRIGGERED)) == 0))
		{
			/* Handle avoidable traps */
			if ((move) && ((r_ptr->flags1 & (RE1_HIT_TRAP)) != 0))
			{
				/* Some traps are avoidable by monsters */
				if ((cave_m_idx[y][x] > 0) && (mon_avoid_trap(&m_list[cave_m_idx[y][x]],y, x, cave_feat[r_ptr->y0][r_ptr->x0])))
				{
					return;
				}
				/* Some traps are avoidable by the player */
				else if ((cave_m_idx[y][x] < 0) && (avoid_trap(y, x, cave_feat[r_ptr->y0][r_ptr->x0])))
				{
					return;
				}
			}

			/* Reset the delay */
			r_ptr->delay = r_ptr->delay_reset;

			/* Randomize grid hit */
			if (r_ptr->flags1 & (RE1_RANDOM))
			{
				int r = region_random_piece(rp_ptr->region);

				ty = region_piece_list[r].y;
				tx = region_piece_list[r].x;
			}

			/* Fixed grid hit */
			if (r_ptr->flags1 & (RE1_FIXED))
			{
				ty = r_ptr->y1;
				tx = r_ptr->x1;
			}
			
			/* Mark region as triggered */
			/* XXX It's really important we mark region triggered before firing the effect.
			 * This prevents a situation where a region gets retriggered by a monster drop
			 * due to a monster dying, and recursively overrunning the stack. */
			r_ptr->flags1 |= (RE1_TRIGGERED);

			/* Actually discharge the region */
			notice |= region_effect(rp_ptr->region, ty, tx);

			/* Paranoia - region has been removed */
			if (!r_ptr->type) return;
		}

		/* Lingering effect hits player/monster moving into grid */
		if ((r_ptr->flags1 & (RE1_LINGER)) &&
				((r_ptr->age) || ((r_ptr->flags1 & (RE1_TRIGGERED)) != 0)))
		{
			int dam = 0;

			/* Get the damage from the region piece */
			if (r_ptr->flags1 & (RE1_SCALAR_DAMAGE))
			{
				dam = rp_ptr->d;
			}
			/* Compute the damage based on distance */
			else if ((rp_ptr->d) && (r_ptr->flags1 & (RE1_SCALAR_DISTANCE)))
			{
				dam = r_ptr->damage / rp_ptr->d;
			}
			/* Get the damage from the region */
			else
			{
				dam = r_ptr->damage;
			}

			/* Avoid hitting player or monsters multiple times with lingering effects unless moving. */
			if (move)
			{
				/* Affect the monster, player or object in this grid only */
				notice |= project_one(r_ptr->who, r_ptr->what, y, x, dam, r_ptr->effect, method_ptr->flags1);
			}
			else
			{
				/* Damage objects directly if dropping an object. */
				notice |= project_one(r_ptr->who, r_ptr->what, y, x, dam, r_ptr->effect, (PROJECT_GRID));
			}
		}

		/* Noticed region? */
		if (notice)
		{
			bool refresh = (r_ptr->flags1 & (RE1_NOTICE)) == 0;

			/* Mark region noticed */
			r_ptr->flags1 |= (RE1_NOTICE | RE1_DISPLAY);

			/* Refresh required */
			if (refresh) region_refresh(rp_ptr->region);
		}
	}
}


/* Some special 'age' values */
#define AGE_OLD			9998
#define AGE_ANCIENT		9999
#define AGE_INFINITY	10000
#define AGE_LIFELESS	10001
#define AGE_EXPIRED		10002




/*
 * Iterate through a region, applying a region_hook function to each grid in the region
 *
 * Similar to the above region_iterate, but we are going to move each piece as a part of the
 * function and eliminate pieces if we cannot move them because of underlying terrain.
 *
 * Note the code that prevents ball projections with MOVE_SOURCE which move will jump all
 * over the place.
 * This is only because we can't see the source of the ball projection, which itself is moving.
 */
bool region_iterate_movement(s16b region, void region_iterator(int y, int x, s16b d, s16b region, int *y1, int *x1))
{
	region_type *r_ptr = &region_list[region];
	method_type *method_ptr = &method_info[region_info[r_ptr->type].method];

	s16b this_region_piece, next_region_piece = 0;
	int prev_region_piece = 0;
	bool seen = FALSE;
	bool update_facing = FALSE;
	int ty, tx;

	/* Move the destination */
	region_iterator(r_ptr->y1, r_ptr->x1, 0, region, &ty, &tx);

	/* Attempt at efficiency */
	if ((r_ptr->y1 != ty) || (r_ptr->x1 != tx))
	{
		r_ptr->y1 = ty;
		r_ptr->x1 = tx;
		update_facing = TRUE;
	}

	/* Move the source if requested */
	if (r_ptr->flags1 & (RE1_MOVE_SOURCE))
	{
		/*
		 * Hack -- Some regions' source is target.
		 *
		 * This is used to prevent projections which have no apparent source from weirdly jumping around
		 * the map, because the target stops being projectable from the invisible source. We set the source
		 * for the projection as the target when initialized, and then keep the source in sync with the target
		 * here.
		 */
		if ((method_ptr->flags1 & (PROJECT_BOOM | PROJECT_4WAY | PROJECT_4WAX | PROJECT_JUMP)) &&
				((method_ptr->flags1 & (PROJECT_ARC | PROJECT_STAR)) == 0))
		{
			/* Set source to target */
			ty = r_ptr->y1;
			tx = r_ptr->x1;
		}

		else
		{
			/* Move the source */
			region_iterator(r_ptr->y0, r_ptr->x0, 0, region, &ty, &tx);
		}

		/* Attempt at efficiency */
		if ((r_ptr->y0 != ty) || (r_ptr->x0 != tx))
		{
			r_ptr->y0 = ty;
			r_ptr->x0 = tx;
			update_facing = TRUE;
		}
	}

	/* Update the facing */
	if (update_facing)
	{
		r_ptr->facing = get_angle_to_target(r_ptr->y0, r_ptr->x0, r_ptr->y1, r_ptr->x1, 0);
	}

	/* We are done? */
	if ((r_ptr->flags1 & (RE1_MOVE_SOURCE)) && (r_ptr->flags1 & (RE1_PROJECTION)))
	{
		return (FALSE);
	}

	for (this_region_piece = r_ptr->first_piece; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_region;

		/* Unable to exist here */
		if (!cave_passable_bold(rp_ptr->y, rp_ptr->x, method_ptr->flags1))
		{
			/* Excise region piece */
			excise_region_piece(this_region_piece);

			/* No previous */
			if (prev_region_piece == 0)
			{
				region_list[region].first_piece = next_region_piece;
			}
			/* Real previous */
			else
			{
				region_piece_type *i_ptr;

				/* Previous object */
				i_ptr = &region_piece_list[prev_region_piece];

				/* Remove from list */
				i_ptr->next_in_region = next_region_piece;
			}

			/* Remove region piece */
			rp_ptr->region = 0;

			/* Forget next pointer */
			rp_ptr->next_in_region = 0;

			/* Relight */
			lite_spot(rp_ptr->y, rp_ptr->x);

			continue;
		}

		/* Update previous */
		prev_region_piece = this_region_piece;

		/* Iterate on region piece */
		region_iterator(rp_ptr->y, rp_ptr->x, rp_ptr->d, region, &ty, &tx);

		/* Move the piece */
		if ((ty != rp_ptr->y) || (tx != rp_ptr->x))
		{
			int y = rp_ptr->y;
			int x = rp_ptr->x;
			
			/* Can't move out of bounds */
			if (!in_bounds(ty, tx)) continue;

			/* Move the region piece */
			excise_region_piece(this_region_piece);
			rp_ptr->y = ty;
			rp_ptr->x = tx;
			rp_ptr->next_in_grid = cave_region_piece[ty][tx];
			cave_region_piece[ty][tx] = this_region_piece;

			/* Redraw old grid */
			lite_spot(y, x);

			/* Redraw new grid */
			lite_spot(ty, tx);
		}
	}

	/* Kill region if no grids are left */
	if (!r_ptr->first_piece) r_ptr->age = AGE_LIFELESS;

	return (seen);
}


/*
 * Movement for seekers
 */
void region_move_seeker_hook(int y, int x, s16b d, s16b region, int *ty, int *tx)
{
	region_type *r_ptr = &region_list[region];
	method_type *method_ptr = &method_info[r_ptr->method];
	int r = rand_int(100);
	int i, dir;
	int grids = 0;

	(void)d;

	/* No movement by default */
	*ty = y;
	*tx = x;

	/* If random, we only move vortexes 50% of the time */
	if ((r_ptr->flags1 & (RE1_RANDOM)) && (r % 2)) return;

	/* Check around (and under) the vortex */
	for (dir = 0; dir < 9; dir++)
	{
		/* Extract adjacent (legal) location */
		int yy = y + ddy_ddd[dir];
		int xx = x + ddx_ddd[dir];

		/* Count passable grids */
		if (cave_passable_bold(yy, xx, method_ptr->flags1)) grids++;

	}

	/*
	 * Vortexes only seek in open spaces.  This makes them useful in
	 * rooms and not too powerful in corridors.
	 */
	if      (grids >= 5) i = 85;
	else if (grids == 4) i = 50;
	else                 i = 0;

	/* Seek out monsters */
	if (r < i)
	{
		/* Try to get a target (nearest or next-nearest monster) */
		get_closest_monster(randint(2), y, x,
			ty, tx, (method_ptr->flags1 & (PROJECT_LOS)) != 0 ? 0x01 : 0x02, r_ptr->who);
	}

	/* No valid target, targetting self, or monster is in an impassable grid */
	if (((*ty == 0) && (*tx == 0)) || ((*ty == y) && (*tx == x)) || (!cave_passable_bold(*ty, *tx, method_ptr->flags1)))
	{
		/* Move randomly */
		dir = randint(9);
	}

	/* Valid target in passable terrain */
	else
	{
		/* Get change in position from current location to target */
		int dy = y - *ty;
		int dx = x - *tx;

		/* Calculate vertical and horizontal distances */
		int ay = ABS(dy);
		int ax = ABS(dx);

		/* We mostly want to move vertically */
		if (ay > (ax * 2))
		{
			/* Choose between directions '8' and '2' */
			if (dy > 0) dir = 8;
			else        dir = 2;
		}

		/* We mostly want to move horizontally */
		else if (ax > (ay * 2))
		{
			/* Choose between directions '4' and '6' */
			if (dx > 0) dir = 4;
			else        dir = 6;
		}

		/* We want to move up and sideways */
		else if (dy > 0)
		{
			/* Choose between directions '7' and '9' */
			if (dx > 0) dir = 7;
			else        dir = 9;
		}

		/* We want to move down and sideways */
		else
		{
			/* Choose between directions '1' and '3' */
			if (dx > 0) dir = 1;
			else        dir = 3;
		}
	}

	/* Extract adjacent (legal) location */
	*ty = y + ddy[dir];
	*tx = x + ddx[dir];

	/* Require passable grids */
	if ((cave_passable_bold(*ty, *tx, method_ptr->flags1)) == 0)
	{
		int k = 0;

		/* Is at least one adjacent grid passable? */
		for (i = 1; i < 9; i++)
		{
			int yy = y + ddy[i];
			int xx = x + ddx[i];

			if (((cave_passable_bold(yy, xx, method_ptr->flags1)) != 0) && (!rand_int(++k))) dir = i;
		}

		/* No grids passable, we're stuck */
		if ((k == 0) && ((cave_passable_bold(y, x, method_ptr->flags1)) != 0)) dir = randint(9);

		/* Extract adjacent (legal) location */
		*ty = y + ddy[dir];
		*tx = x + ddx[dir];
	}
}


/*
 * Function to move region pieces as if encoding a vector
 */
void region_move_vector_hook(int y, int x, s16b d, s16b region, int *ty, int *tx)
{
	region_type *r_ptr = &region_list[region];
	byte angle, speed;
	int speed32, angle32;

	u16b path_g[99];
	int path_n;

	/* No movement by default */
	*ty = y;
	*tx = x;

	/* Speed of travel */
	speed = GRID_X(d);
	speed32 = speed;
	
	/* Does this fragment move at this age? */
	if ((r_ptr->age * speed32 / 100) == ((r_ptr->age - 1) * speed32 / 100)) return;

	/* Angle of travel */
	angle = GRID_Y(d);
	angle32 = angle;
	
	/* Get the target grid for this angle */
	get_grid_using_angle(angle32, r_ptr->y0, r_ptr->x0, ty, tx);

	/* If we have a legal target, */
	if ((*ty != r_ptr->y0) || (*tx != r_ptr->x0))
	{
		/* Calculate the projection path */
		path_n = project_path(path_g, 99, r_ptr->y0, r_ptr->x0,
				ty, tx, PROJECT_PASS | PROJECT_THRU);

		/* Get the location */
		if (path_n >= (r_ptr->age * speed / 100))
		{
			*ty = GRID_Y(path_g[(r_ptr->age * speed / 100)]);
			*tx = GRID_X(path_g[(r_ptr->age * speed / 100)]);
		}
	}
}


/*
 * Function to spread regions
 */
void region_move_spread_hook(int y, int x, s16b d, s16b region, int *ty, int *tx)
{
	method_type *method_ptr = &method_info[region_list[region].method];
	int dir = rand_int(12);

	(void)d;

	if (dir < 8)
	{
		*ty = y + ddy_ddd[dir];
		*tx = x + ddx_ddd[dir];

		/* Can drift into new location? */
		if (!cave_passable_bold(*ty, *tx, method_ptr->flags1))
		{
			int i;

			/* Is at least one adjacent grid passable? */
			for (i = 0; i < 8; i++)
			{
				int yy = y + ddy_ddd[dir];
				int xx = x + ddx_ddd[dir];

				if (cave_passable_bold(yy, xx, method_ptr->flags1)) break;
			}

			/* If at least one grid passable, abort */
			if ((i < 8) || (cave_passable_bold(y, x, method_ptr->flags1)))
			{
				*ty = y;
				*tx = x;
			}
		}
	}
	else
	{
		*ty = y;
		*tx = x;
	}
}



/*
 * Process region.
 *
 * We potentially transform the region, then apply the effect if
 * required.
 */
void process_region(s16b region)
{
	region_type *r_ptr = &region_list[region];
	method_type *method_ptr = &method_info[r_ptr->method];

	int range = scale_method(method_ptr->max_range, r_ptr->level);
	int radius = scale_method(method_ptr->radius, r_ptr->level);

	int i, j, k;
	int path_n = 0;
	u16b path_g[99];

	bool update = FALSE;

	/* Paranoia */
	if (!r_ptr->type) return;

	/* Count down to next turn */
	if (r_ptr->delay)
	{
		/* Reduce timer */
		if (--r_ptr->delay > 0) return;
	}

	/* Ensure we have a timer interval */
	r_ptr->delay = r_ptr->delay_reset;

	/* Accelerating */
	if ((r_ptr->flags1 & (RE1_ACCELERATE)) && (!(r_ptr->flags1 & (RE1_DECELERATE)) || (r_ptr->age < r_ptr->lifespan / 2)))
	{
		if (r_ptr->delay_reset < 3) r_ptr->delay_reset -= 1;
		r_ptr->delay_reset -= r_ptr->delay_reset / 3;
		if (r_ptr->delay_reset < 1) r_ptr->delay_reset = 1;
	}

	/* Decelerating */
	if ((r_ptr->flags1 & (RE1_DECELERATE)) && (!(r_ptr->flags1 & (RE1_ACCELERATE)) || (r_ptr->age > r_ptr->lifespan / 2)))
	{
		r_ptr->delay_reset += r_ptr->delay_reset / 3;
		if (r_ptr->delay_reset < 3) r_ptr->delay_reset += 1;
	}

	/* Effects eventually "die" */
	if (r_ptr->age >= r_ptr->lifespan)
	{
		/* Region loops backwards once finished */
		if (region_info[r_ptr->type].flags1 & (RE1_BACKWARDS))
		{
			/* Start going backwards instead */
			r_ptr->flags1 |= (RE1_BACKWARDS);

			/* Wait for trigger?  */
			if (!(region_info[r_ptr->type].flags1 & (RE1_AUTOMATIC))) r_ptr->flags1 &= ~(RE1_AUTOMATIC);
		}
		else
		{
			/* Kill the region */
			region_terminate(region);

			return;
		}
	}

	/* Rotate the region */
	if (r_ptr->flags1 & (RE1_CLOCKWISE))
	{
		int old_dir = get_angle_to_dir(r_ptr->facing);
		int facing = r_ptr->facing - 2;
		int dir;
		int y, x;

		while (facing < 0) facing += 180;

		dir = get_angle_to_dir(facing);

		/* Change direction if we can't project at least 1 grid after rotating */
		if ((cave_passable_bold(r_ptr->y0 + ddy[old_dir], r_ptr->x0 + ddx[old_dir], method_ptr->flags1)) &&
				!(cave_passable_bold(r_ptr->y0 + ddy[dir], r_ptr->x0 + ddx[dir], method_ptr->flags1)))
		{
			facing += 4;
			while (facing >= 180) facing -= 180;

			r_ptr->flags1 &= ~(RE1_CLOCKWISE);
			r_ptr->flags1 |= (RE1_COUNTER_CLOCKWISE);
		}

		/* Use revised facing */
		r_ptr->facing = facing;

		/* Get new target */
		get_grid_using_angle(r_ptr->facing, r_ptr->y0, r_ptr->x0, &y, &x);

		r_ptr->y1 = y;
		r_ptr->x1 = x;

		update = TRUE;
	}

	/* Rotate the region */
	else if (r_ptr->flags1 & (RE1_COUNTER_CLOCKWISE))
	{
		int old_dir = get_angle_to_dir(r_ptr->facing);
		int facing = r_ptr->facing + 2;
		int dir;
		int y, x;

		while (facing >= 180) facing -= 180;

		dir = get_angle_to_dir(facing);

		/* Change direction if we can't project at least 1 grid after rotating */
		if ((cave_passable_bold(r_ptr->y0 + ddy[old_dir], r_ptr->x0 + ddx[old_dir], method_ptr->flags1)) &&
				!(cave_passable_bold(r_ptr->y0 + ddy[dir], r_ptr->x0 + ddx[dir], method_ptr->flags1)))
		{
			facing -= 4;
			while (facing < 0) facing += 180;

			r_ptr->flags1 &= ~(RE1_COUNTER_CLOCKWISE);
			r_ptr->flags1 |= (RE1_CLOCKWISE);
		}

		/* Use revised facing */
		r_ptr->facing = facing;

		/* Get new target */
		get_grid_using_angle(r_ptr->facing, r_ptr->y0, r_ptr->x0, &y, &x);

		r_ptr->y1 = y;
		r_ptr->x1 = x;

		update = TRUE;
	}

	/* Moving a wall */
	if (r_ptr->flags1 & (RE1_WALL))
	{
		/* Get the direction of travel (angular dir is 2x this) */
		int major_axis =
			get_angle_to_target(r_ptr->y0, r_ptr->x0, r_ptr->y1, r_ptr->x1, 0);

		/* Store target grid */
		int ty = r_ptr->y1;
		int tx = r_ptr->x1;

		/* Hack - fix range */
		range = range ? range : MAX_RANGE;

		/* Hack - mark grids with temp flags. Used to ensure wall grids are next to previous wall grids */
		region_mark_temp(region);

		/* Remove previous list of grids */
		region_delete(region);

		/* Calculate the projection path -- require orthogonal movement */
		path_n = project_path(path_g, range, r_ptr->y0, r_ptr->x0,
			&ty, &tx, PROJECT_PASS | PROJECT_THRU | PROJECT_ORTH);

		/* Require a working projection path */
		if ((path_n > r_ptr->age) || (path_n > range))
		{
			/* Remember delay factor */
			int old_delay = op_ptr->delay_factor;

			int zaps = 0;
			int axis;

			/* Get center grid (walls travel one grid per turn) */
			int y0 = GRID_Y(path_g[(r_ptr->age > range) ? range : r_ptr->age]);
			int x0 = GRID_X(path_g[(r_ptr->age > range) ? range : r_ptr->age]);

			/* Set delay to half normal */
			op_ptr->delay_factor /= 2;

			/*
			 * If the center grid is both projectable and next to a previous wall grid, zap it.
			 */
			if (cave_passable_bold(y0, x0, method_ptr->flags1))
			{
				bool near_old_wall = FALSE;

				for (i = 0; (i < 8) && (!near_old_wall); i++)
				{
					int y = y0 + ddy_ddd[i];
					int x = x0 + ddx_ddd[i];

					if (play_info[y][x] & (PLAY_TEMP)) near_old_wall = TRUE;
				}

				if (near_old_wall)
				{
					zaps++;
					region_piece_insert(y0, x0, r_ptr->damage, region);
				}
			}

			/* If this wall spreads out from the origin, */
			if (radius > 0)
			{
				/* Get the directions of spread (angles are twice this) */
				int minor_axis1 = (major_axis +  45) % 180;
				int minor_axis2 = (major_axis + 135) % 180;

				/* Process the left, then right-hand sides of the wall */
				for (i = 0; i < 2; i++)
				{
					if (i == 0) axis = minor_axis1;
					else        axis = minor_axis2;

					/* Get the target grid for this side */
					get_grid_using_angle(axis, y0, x0, &ty, &tx);

					/* If we have a legal target, */
					if ((ty != y0) || (tx != x0))
					{
						/* Calculate the projection path */
						path_n = project_path(path_g, radius, y0, x0, &ty, &tx,
							PROJECT_PASS | PROJECT_THRU | PROJECT_ORTH);

						/* Check all the grids */
						for (j = 0; j < path_n; j++)
						{
							/* Get grid */
							ty = GRID_Y(path_g[j]);
							tx = GRID_X(path_g[j]);

							/*
							 * If this grid is both projectable, and next to a previous wall grid, zap it.
							 */
							if (cave_passable_bold(ty, tx, method_ptr->flags1))
							{
								bool near_old_wall = FALSE;

								for (k = 0; (k < 8) && (!near_old_wall); k++)
								{
									int y = ty + ddy_ddd[k];
									int x = tx + ddx_ddd[k];

									if (play_info[y][x] & (PLAY_TEMP)) near_old_wall = TRUE;
								}

								if (near_old_wall)
								{
									zaps++;
									region_piece_insert(ty, tx, r_ptr->damage, region);
								}
							}
						}
					}
				}
			}

			/* Kill wall if nothing got zapped this turn */
			if (zaps == 0)
			{
				r_ptr->age = AGE_LIFELESS;
			}

			/* Restore standard delay */
			op_ptr->delay_factor = old_delay;
		}

		/* No working projection path -- kill the wall */
		else
		{
			r_ptr->age = AGE_LIFELESS;
		}

		/* Refresh temp grids */
		for (i =  0; i < temp_n; i++)
		{
			lite_spot(temp_y[i], temp_x[i]);
		}

		/* Clear temporary grids */
		clear_temp_array();
	}

	/* Apply seeker effect */
	if (r_ptr->flags1 & (RE1_SEEKER))
	{
		region_iterate_movement(region, region_move_seeker_hook);

		update = TRUE;
	}

	/* Apply scalar vector effect */
	if (r_ptr->flags1 & (RE1_SCALAR_VECTOR))
	{
		region_iterate_movement(region, region_move_vector_hook);

		update = TRUE;
	}

	/* Apply spreading effect */
	if (r_ptr->flags1 & (RE1_SPREAD))
	{
		region_iterate_movement(region, region_move_spread_hook);

		update = TRUE;
	}

	/* We've freed up some regions. Allow regions to regenerate
	 * if required.
	 */
	if ((region_piece_limit) && (region_piece_max * 5 / 4 < z_info->region_piece_max)
			&& !(r_ptr->first_piece))
	{
		update = TRUE;
	}

	/* Update the region if required */
	if ((update) && (r_ptr->flags1& (RE1_PROJECTION)))
	{
		region_update(region);
	}

	/* Apply effect every turn, unless already applied earlier in the turn */
	if ((r_ptr->flags1 & (RE1_AUTOMATIC)) && !(r_ptr->flags1 & (RE1_TRIGGERED)))
	{
		/* Discharge the region */
		if (region_effect(region, r_ptr->y1, r_ptr->x1))
		{
			bool refresh;

			/* Paranoia: Could end up with an invalid region */
			if (!r_ptr->type) return;

			refresh = (r_ptr->flags1 & (RE1_NOTICE)) == 0;

			/* Mark region noticed */
			r_ptr->flags1 |= (RE1_NOTICE | RE1_DISPLAY);

			/* Refresh required */
			if (refresh) region_refresh(region);
		}
	}

	/* Age the region */
	if (r_ptr->flags1 & (RE1_AUTOMATIC | RE1_TRIGGERED))
	{
		/* Region ages backwards */
		if (r_ptr->flags1 & (RE1_BACKWARDS))
		{
			if (r_ptr->age) r_ptr->age--;
			else
			{
				r_ptr->flags1 &= ~(RE1_BACKWARDS | RE1_AUTOMATIC);
				if (region_info[r_ptr->type].flags1 & (RE1_AUTOMATIC)) r_ptr->flags1 |= (RE1_AUTOMATIC);
			}
		}
		/* Region ages normally */
		else if (r_ptr->age < AGE_INFINITY) r_ptr->age++;

		/* Clear trigger */
		r_ptr->flags1 &= ~(RE1_TRIGGERED);
	}
}


/*
 * Process effects.
 */
void process_regions(void)
{
	int i;

	/* Process all regions */
	for (i = 0; i < region_max; i++)
	{
		/* Get this effect */
		region_type *r_ptr = &region_list[i];

		/* Skip empty effects */
		if (!r_ptr->type) continue;

		/* Process effect */
		process_region(i);
	}

	/*
	 * We've finished freeing up enough regions.
	 */
	if ((region_piece_limit) && (region_piece_max * 5 / 4 < z_info->region_piece_max))
	{
		region_piece_limit = FALSE;
	}

/* We've fixed the code that this check was used for */
#if 0
	/* Warn if we get orphaned pieces */
	for (i = 0; i < z_info->region_piece_max; i++)
	{
		s16b region = region_piece_list[i].region;
		/* Get this effect */
		region_type *r_ptr = &region_list[region];

		if (!region) continue;

		/* Skip empty effects */
		if (!r_ptr->type) msg_format("Piece %d is orphaned from %d.", i, region);
	}
#endif
}


/*
 * Initialise a region at (y,x).
 *
 * Use the level to initialise region effects.
 *
 * Optionally use a target (ty, tx). If target not supplied
 * try to pick a target to allow the region to affect a number of grids.
 *
 */
s16b init_region(int who, int what, int type, int dam, int method, int effect, int level, int y0, int x0, int y1, int x1)
{
	region_info_type *ri_ptr = &region_info[type];
	method_type *method_ptr = &method_info[method];

	s16b region = region_pop();
	int radius = scale_method(method_ptr->radius, level);
	int range = scale_method(method_ptr->max_range, level);

	region_type *r_ptr = &region_list[region];

	/* Paranoia */
	if (!region) return(0);

	/* Pick a target */
	if ((!y1) && (!x1))
	{
		/* Target self */
		if (method_ptr->flags1 & (PROJECT_SELF))
		{
			/* Target self */
			y1 = y0;
			x1 = x0;
		}
		else
		{
			/* Pick a target */
			scatter(&y1, &x1, y0, x0, radius ? radius : range, method_ptr->flags1 & (PROJECT_LOS) ? CAVE_XLOS : CAVE_XLOF);

			/* Parania - always have a target */
			if ((y1 == y0) && (x1 == x0))
			{
				int d = rand_int(8);

				y1 = y0 + ddy_ddd[d];
				x1 = x0 + ddx_ddd[d];
			}
		}
	}

	/* Initialise region values from parameters passed to this routine */
	r_ptr->type = type;
	r_ptr->level = level;
	r_ptr->y0 = y0;
	r_ptr->x0 = x0;
	r_ptr->y1 = y1;
	r_ptr->x1 = x1;
	r_ptr->who = who;
	r_ptr->what = what;
	r_ptr->damage = dam;
	r_ptr->effect = effect;
	r_ptr->lifespan = AGE_INFINITY;
	r_ptr->delay = 0;
	r_ptr->method = method;

	/* Initialise region values from region info type */
	r_ptr->flags1 = ri_ptr->flags1;
	r_ptr->delay_reset = ri_ptr->delay_reset;
	r_ptr->child_region = ri_ptr->child_region;

	/* Exclude one of clockwise and counter clockwise */
	if ((r_ptr->flags1 & (RE1_CLOCKWISE)) &&
			(r_ptr->flags1 & (RE1_COUNTER_CLOCKWISE)) &&
			(rand_int(100) < 50))
	{
		/* Remove one - we always default to clockwise if both set */
		r_ptr->flags1 &= ~(RE1_CLOCKWISE);
	}

	return(region);
}
