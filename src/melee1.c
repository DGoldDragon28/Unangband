/* File: melee1.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in  all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.1
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"



/*
 * Critical blows by monsters can inflict cuts and stuns.
 */
static int monster_critical(int dice, int sides, int dam, int effect)
{
	int max = 0;
	int bonus;
	int total = dice * sides;

	/* Special case -- wounding/battering attack */
	if ((effect == GF_WOUND) || (effect == GF_BATTER))
	{
		/* Must do at least 70% of perfect */
		if (dam < total * 7 / 10) return (0);

		max = 1;
	}

	/* Standard attack */
	else
	{
		/* Weak blows rarely work */
		if ((rand_int(20) >= dam) || (!rand_int(3))) return (0);

		/* Must do at least 90% of perfect */
		if (dam < total * 9 / 10) return (0);
	}


	/* Perfect damage */
	if (dam == total) max++;

	/* Get bonus to critical damage (never greater than 6) */
	bonus = MIN(6, dam / 8);

	/* Critical damage  (never greater than 6 + max) */
	return (randint(bonus) + max);
}


/*
 * Monster scale
 *
 * Scale a levelling monster based on monster depth vs actual depth.
 */
bool monster_scale(monster_race *n_ptr, int m_idx, int depth)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int d1, d2, scale;
	int i, boost;
	int n = 0;

	u32b flag[4];

	/* Paranoia */
	if ((r_ptr->flags9 & (RF9_LEVEL_MASK)) == 0) return (FALSE);

	/* Hack -- ensure distinct monster types */
	depth = depth - ((depth - r_ptr->level) % 5);

	/* Hack -- leader monsters */
	if (m_ptr->mflag & (MFLAG_LEADER)) depth += 5;

	/* Hack -- maximum power increase */
	if (depth > r_ptr->level + 15) depth = r_ptr->level + 15;

	/* This is the reverse of the algorithmic level
	   computation in eval_r_power in init1.c.
	   We apply this because monster power increases
	   much faster per level deeper in the dungeon.
	 */

	/* Compute effective power for monsters at this depth */
	d1 = (depth <= 30 ? depth :
			(depth <= 35 ? depth * 2 - 30 :
			(depth <= 45 ? depth * 6 - 170:
			(depth <= 50 ? depth * 12 - 440 :
			  depth * 20 - 840))));

	/* Compute effective power for monster's old depth */
	d2 = (r_ptr->level == 0 ? 1 : (r_ptr->level <= 30 ? r_ptr->level :
			(r_ptr->level <= 35 ? r_ptr->level * 2 - 30 :
			(r_ptr->level <= 45 ? r_ptr->level * 6 - 170 :
			(r_ptr->level <= 50 ? r_ptr->level * 12 - 440 :
			  r_ptr->level * 20 - 840)))));

	/* We only care about significant multipliers but scale up by multiple of 100 */
	scale = ((d1 * 100) / d2);

	/* Paranoia */
	if (scale <= 100) return (FALSE);

	/* Copy real monster to fake */
	COPY(n_ptr, r_ptr, monster_race);

	/* Set experience */
	n_ptr->mexp = r_ptr->mexp * scale / 100;

	/* Apply only one level flag */
	if (n_ptr->flags9 & (RF9_LEVEL_AGE)) flag[n++] = RF9_LEVEL_AGE;
	/* if (n_ptr->flags9 & (RF9_LEVEL_CLASS)) flag[n++] = RF9_LEVEL_CLASS;*/
	if (n_ptr->flags9 & (RF9_LEVEL_SPEED)) flag[n++] = RF9_LEVEL_SPEED;
	if (n_ptr->flags9 & (RF9_LEVEL_POWER)) flag[n++] = RF9_LEVEL_POWER;
	if (n_ptr->flags9 & (RF9_LEVEL_SIZE)) flag[n++] = RF9_LEVEL_SIZE;

	/* Paranoia */
	if (n == 0) return (FALSE);

	/* Clear all but one flag */
	if (n > 1)
	{
		/* Clear all flags */
		n_ptr->flags9 &= ~RF9_LEVEL_MASK;

		/* Add one back in based on m_idx */
		n_ptr->flags9 |= flag[m_idx % n];
	}

	/* Set level to depth */
	n_ptr->level = depth;

	/* Scale up for classes */
	if (n_ptr->flags9 & (RF9_LEVEL_CLASS))
	{
		int fake_m_idx = m_idx / n;
		bool add_ranged = FALSE;
		n = 0;

		if (!(fake_m_idx % 2) && (r_ptr->level <= depth - (++n * 5)))
		{
			n_ptr->flags2 |= (RF2_ARMOR);

			/* Increase effectiveness of some blows */
			for (i = 0; i < 4; i++)
			{
				if (!n_ptr->blow[i].d_side)
				{
					/* Give warrior some random ranged attack */
					n_ptr->blow[i].method = RBM_ARROW + (m_ptr->r_idx % 5);
					n_ptr->blow[i].effect = i > 0 ? n_ptr->blow[0].effect : GF_HURT;
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), 1);
					n_ptr->blow[i].d_side = d1 < 50 ? 5 : (d1 / 10);

					n_ptr->freq_innate = MAX(n_ptr->freq_innate, 25 + depth / 10);
					add_ranged = TRUE;

					break;
				}
				else if ((n_ptr->blow[i].method >= RBM_ARROW) && (n_ptr->blow[i].method < RBM_ARROW + 5))
				{
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), n_ptr->blow[i].d_dice);
					n_ptr->blow[i].d_side = MAX(d1 < 50 ? 5 : (d1 / 10), n_ptr->blow[i].d_side);
					break;
				}

				if (n_ptr->blow[i].effect == GF_HURT)
				{
					if ((m_idx % 11) < 4) n_ptr->blow[i].effect = GF_BATTER;
					else n_ptr->blow[i].effect = GF_WOUND;
				}
			}

			scale = scale * 4 / 5;
		}
		if (!(fake_m_idx % 3) && (r_ptr->level <= depth - (++n * 5)))
		{
			n_ptr->flags2 |= (RF2_PRIEST);

			/* Give priest some power */
			n_ptr->mana = MAX(n_ptr->mana, d1 / 10);
			n_ptr->spell_power = MAX(n_ptr->spell_power, depth / 4);
			n_ptr->freq_spell = MAX(n_ptr->freq_spell, 25 + depth / 10);

			/* Give priest some basic spells */
			if ((m_ptr->r_idx % 7) < (depth / 15)) n_ptr->flags6 |= (RF6_HEAL);
			if ((m_ptr->r_idx % 9) < (depth / 12)) n_ptr->flags6 |= (RF6_CURE);
			if ((m_ptr->r_idx % 11) < (depth / 10)) n_ptr->flags6 |= (RF6_BLESS);
			if ((m_ptr->r_idx % 13) < (depth / 8)) n_ptr->flags6 |= (RF6_BLINK);

			/* Pick a 'priestly' attack spell */
			switch(m_ptr->r_idx % 5)
			{
				case 0: n_ptr->flags6 |= (RF6_WOUND); break;
				case 1: n_ptr->flags6 |= (RF6_CURSE); break;
				case 2: n_ptr->flags5 |= (RF5_HOLY_ORB); break;
				case 3: n_ptr->flags5 |= (RF5_BOLT_ELEC); break;
				case 4: n_ptr->flags5 |= (RF5_BALL_LITE); break;
			}

			/* And maybe another */
			if (depth > 40) switch((m_ptr->r_idx * 7) % 5)
			{
				case 0: n_ptr->flags6 |= (RF6_WOUND); break;
				case 1: n_ptr->flags6 |= (RF6_CURSE); break;
				case 2: n_ptr->flags5 |= (RF5_HOLY_ORB); break;
				case 3: n_ptr->flags5 |= (RF5_BOLT_ELEC); break;
				case 4: n_ptr->flags5 |= (RF5_BALL_LITE); break;
			}

			scale = scale * 2 / 3;
		}
		if (!(fake_m_idx % 5) && (r_ptr->level <= depth - (++n * 5)))
		{
			n_ptr->flags2 |= (RF2_MAGE);

			/* Give priest some power */
			n_ptr->mana = MAX(n_ptr->mana, d1 / 12);
			n_ptr->spell_power = MAX(n_ptr->spell_power, depth / 3);
			n_ptr->freq_spell = MAX(n_ptr->freq_spell, 25 + depth / 10);

			/* Give mage some basic spells */
			if (((m_ptr->r_idx * 11) % 7) < (depth / 15)) n_ptr->flags6 |= (RF6_BLINK);
			if (((m_ptr->r_idx * 13) % 9) < (depth / 12)) n_ptr->flags6 |= (RF6_ADD_MANA);
			if (((m_ptr->r_idx * 17) % 11) < (depth / 10)) n_ptr->flags6 |= (RF6_TPORT);
			if (((m_ptr->r_idx * 19) % 13) < (depth / 8)) n_ptr->flags6 |= (RF6_SHIELD);

			/* Pick a 'magely' attack spell */
			switch((m_ptr->r_idx * 13) % 12)
			{
				case 0: if ((n_ptr->flags6 & (RF6_DARKNESS)) && (depth > 20)) { n_ptr->flags5 |= (RF5_BALL_DARK); break; }
				case 1: n_ptr->flags5 |= (RF5_BOLT_ACID); break;
				case 2: if ((n_ptr->flags6 & (RF6_CONF)) && (depth > 30)) { n_ptr->flags5 |= (RF5_BALL_CONFU); break; }
				case 3: n_ptr->flags5 |= (RF5_BOLT_COLD); break;
				case 4: if ((n_ptr->flags6 & (RF6_ILLUSION)) && (depth > 50)) { n_ptr->flags5 |= (RF5_BALL_CHAOS); break; }
				case 5: n_ptr->flags5 |= (RF5_BOLT_FIRE); break;
				case 6: if ((n_ptr->flags6 & (RF6_HOLD)) && (depth > 40)) { n_ptr->flags5 |= (RF5_BOLT_NETHR); break; }
				case 7: n_ptr->flags5 |= (RF5_BOLT_ELEC); break;
				case 8: if ((n_ptr->flags3 & (RF3_EVIL)) && (depth > 60)) { n_ptr->flags5 |= (RF5_BALL_NETHR); break; }
				case 9: n_ptr->flags5 |= (RF5_BOLT_POIS); break;
				case 10: if ((n_ptr->flags6 & (RF6_SLOW)) && (depth > 60)) { n_ptr->flags5 |= (RF5_BALL_SOUND); break; }
				case 11: n_ptr->flags5 |= (RF5_BOLT_MANA); break;
			}

			/* And another one */
			if (depth > 30) switch((m_ptr->r_idx * 17) % 12)
			{
				case 0: if ((n_ptr->flags5 & (RF5_BOLT_POIS))/* && (depth > 20) */) { n_ptr->flags5 |= (RF5_BALL_POIS); break; }
				case 1: n_ptr->flags5 |= (RF5_BOLT_ACID); break;
				case 2: if ((n_ptr->flags5 & (RF5_BOLT_ELEC))/* && (depth > 30) */) { n_ptr->flags5 |= (RF5_BALL_WIND); break; }
				case 3: n_ptr->flags5 |= (RF5_BOLT_COLD); break;
				case 4: if ((n_ptr->flags5 & (RF5_BOLT_ACID)) && (depth > 50)) { n_ptr->flags5 |= (RF5_BALL_ACID); break; }
				case 5: n_ptr->flags5 |= (RF5_BOLT_FIRE); break;
				case 6: if ((n_ptr->flags5 & (RF5_BOLT_FIRE)) && (depth > 40)) { n_ptr->flags5 |= (RF5_BALL_FIRE); break; }
				case 7: n_ptr->flags5 |= (RF5_BOLT_ELEC); break;
				case 8: if ((n_ptr->flags5 & (RF5_BOLT_COLD)) && (depth > 50)) { n_ptr->flags5 |= (RF5_BALL_COLD); break; }
				case 9: n_ptr->flags5 |= (RF5_BOLT_POIS); break;
				case 10: if ((n_ptr->flags5 & (RF5_BOLT_MANA)) && (depth > 60)) { n_ptr->flags5 |= (RF5_BALL_MANA); break; }
				case 11: n_ptr->flags5 |= (RF5_BOLT_MANA); break;
			}

			/* And maybe one more */
			if (depth > 50) switch((m_ptr->r_idx * 19) % 15)
			{
				case 0: n_ptr->flags5 |= (RF5_BALL_LITE); break;
				case 1: n_ptr->flags5 |= (RF5_BALL_DARK); break;
				case 2: n_ptr->flags5 |= (RF5_BALL_CONFU); break;
				case 3: n_ptr->flags5 |= (RF5_BALL_SOUND); break;
				case 4: n_ptr->flags5 |= (RF5_BALL_SHARD); break;
				case 5: n_ptr->flags5 |= (RF5_BALL_STORM); break;
				case 6: n_ptr->flags5 |= (RF5_BALL_NETHR); break;
				case 7: n_ptr->flags5 |= (RF5_BALL_WATER); break;
				case 8: n_ptr->flags5 |= (RF5_BALL_CHAOS); break;
				case 9: n_ptr->flags5 |= (RF5_BALL_ELEC); break;
				case 10: n_ptr->flags5 |= (RF5_BOLT_PLAS); break;
				case 11: n_ptr->flags5 |= (RF5_BOLT_ICE); break;
				case 12: n_ptr->flags5 |= (RF5_BEAM_ELEC); break;
				case 13: n_ptr->flags5 |= (RF5_BEAM_ICE); break;
				case 14: n_ptr->flags5 |= (RF5_ARC_FORCE); break;
			}

			scale = scale / 2;
		}
		if (!(fake_m_idx % 7) && (r_ptr->level <= depth - (++n * 5)))
		{
			n_ptr->flags2 |= (RF2_SNEAKY);

			/* Increase effectiveness of some blows */
			for (i = 0; i < 4; i++)
			{
				if ((!n_ptr->blow[i].d_side) && (!add_ranged))
				{
					/* Give thief some random ranged attack */
					n_ptr->blow[i].method = RBM_DAGGER + (m_ptr->r_idx % 3);
					n_ptr->blow[i].effect = i > 0 ? n_ptr->blow[0].effect : GF_HURT;
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), 1);
					n_ptr->blow[i].d_side = d1 < 50 ? 5 : (d1 / 10);

					n_ptr->freq_innate = MAX(n_ptr->freq_innate, 25 + depth / 10);
					add_ranged = TRUE;
				}
				else if ((!n_ptr->blow[i].d_side) && (i * 15 < depth))
				{
					n_ptr->blow[i].method = RBM_TOUCH;
					if (((m_idx + (i * 13)) % 11) < 4)
					{
						n_ptr->blow[i].effect = depth < 20 ? GF_EAT_GOLD : GF_EAT_ITEM;
					}
					else n_ptr->blow[i].effect = GF_POIS;

					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), 1);
					n_ptr->blow[i].d_side = d1 < 50 ? 5 : (d1 / 10);
				}
				else if ((n_ptr->blow[i].method >= RBM_DAGGER) && (n_ptr->blow[i].method < RBM_DAGGER + 3))
				{
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), n_ptr->blow[i].d_dice);
					n_ptr->blow[i].d_side = MAX(d1 < 50 ? 5 : (d1 / 10), n_ptr->blow[i].d_side);
				}
			}

			/* Give thief some power */
			n_ptr->mana = MAX(n_ptr->mana, d1 / 20);
			n_ptr->spell_power = MAX(n_ptr->spell_power, depth / 6);
			n_ptr->freq_spell = MAX(n_ptr->freq_spell, 25 + depth / 10);

			/* Give thief some basic spells */
			if (((m_ptr->r_idx * 13) % 7) < (depth / 15)) n_ptr->flags6 |= (RF6_BLINK);
			if (((m_ptr->r_idx * 17) % 9) < (depth / 12)) n_ptr->flags6 |= (RF6_TPORT);
			if (((m_ptr->r_idx * 19) % 11) < (depth / 10)) n_ptr->flags6 |= (RF6_INVIS);

			scale = scale * 4 / 5;
		}

		if ((!(fake_m_idx % 11) && (r_ptr->level <= depth - (++n * 5))) || (!n))
		{
			n_ptr->flags2 |= (RF2_ARCHER);

			/* Increase effectiveness of some blows */
			for (i = 0; i < 4; i++)
			{
				if ((!n_ptr->blow[i].d_side) && ((i * 15 < depth) || !(add_ranged)))
				{
					/* Give thief some random ranged attack */
					n_ptr->blow[i].method = RBM_ARROW + (m_ptr->r_idx % 2);
					n_ptr->blow[i].effect = i > 0 ? n_ptr->blow[0].effect : GF_HURT;
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), 1);
					n_ptr->blow[i].d_side = d1 < 50 ? 5 : (d1 / 10);

					n_ptr->freq_innate = MAX(n_ptr->freq_innate, 25 + depth / 10);
					add_ranged = TRUE;
				}
				else if ((n_ptr->blow[i].method >= RBM_ARROW) && (n_ptr->blow[i].method < RBM_ARROW + 2))
				{
					n_ptr->blow[i].d_dice = MAX(d1 < 50 ? d1 / 5 : d1 / (d1 / 10), n_ptr->blow[i].d_dice);
					n_ptr->blow[i].d_side = MAX(d1 < 50 ? 5 : (d1 / 10), n_ptr->blow[i].d_side);
				}
			}

			scale = scale * 4 / 5;
		}
	}

	/* Recheck scale */
	if (scale < 100) return (TRUE);

	/* Scale up for speed */
	if (((n_ptr->flags9 & (RF9_LEVEL_SPEED)) != 0)
		|| (((n_ptr->flags9 & (RF9_LEVEL_CLASS)) != 0) && (r_ptr->flags2 & (RF2_SNEAKY | RF2_ARCHER))))
	{
		/* We rely on speed giving us an overall scale improvement */
		/* Note breeders are more dangerous on speed overall */

		/* Boost speed first */
		boost = scale / 100;

		/* Limit to +25 faster */
		if (boost > 25) boost = 25;

		/* Increase fake speed */
		n_ptr->speed += boost;

		/* Reduce scale by actual improvement in energy */
		scale = scale * extract_energy[r_ptr->speed] / extract_energy[n_ptr->speed];

		/* Fast breeders more dangerous so reduce speed improvement */
		if ((n_ptr->flags2 & (RF2_MULTIPLY)) != 0) n_ptr->speed -= boost / 2;

		/* Hack -- faster casters run out of mana faster */
		else if (n_ptr->mana && (boost > 10)) n_ptr->flags6 |= RF6_ADD_MANA;

	}

	/* Recheck scale */
	if (scale < 100) return (TRUE);

	/* Scale up for size */
	if (((n_ptr->flags9 & (RF9_LEVEL_SIZE)) != 0)
		|| (((n_ptr->flags9 & (RF9_LEVEL_CLASS)) != 0) && (r_ptr->flags2 & (RF2_ARMOR))))
	{
		/* Boost attack damage partially */
		if (scale > 133)
			for (i = 0; i < 4; i++)
		{
			if (!n_ptr->blow[i].d_side) continue;

			boost = (scale * n_ptr->blow[i].d_dice / 25) / (n_ptr->blow[i].d_side * 3);

			if (boost > 19 * n_ptr->blow[i].d_dice) boost = 19 * n_ptr->blow[i].d_dice;

			if (boost > 1) n_ptr->blow[i].d_dice += boost - 1;
		}

		/* Boost power slightly */
		if (scale > 133)
		{
			n_ptr->power = (r_ptr->power * scale * 3) / 4;
		}
	}

	/* Recheck scale */
	if (scale < 100) return (TRUE);

	/* Scale up for power */
	if (((n_ptr->flags9 & (RF9_LEVEL_POWER)) != 0)
		|| (((n_ptr->flags9 & (RF9_LEVEL_CLASS)) != 0) && (r_ptr->flags2 & (RF2_MAGE))))
	{
		/* Boost speed first */
		boost = scale / 400;

		/* Limit to +15 faster */
		if (boost > 15) boost = 15;

		/* Increase fake speed */
		n_ptr->speed += boost;

		/* Reduce scale by actual improvement in energy */
		scale = scale * extract_energy[r_ptr->speed] / extract_energy[n_ptr->speed];

		/* Boost attack damage partially */
		if (scale > 200)
			for (i = 0; i < 4; i++)
		{
			if (!n_ptr->blow[i].d_side) continue;

			boost = (scale * n_ptr->blow[i].d_dice / 50) / n_ptr->blow[i].d_side;

			if (boost > 9 * n_ptr->blow[i].d_dice) boost = 9 * n_ptr->blow[i].d_dice;

			if (boost > 1) n_ptr->blow[i].d_dice += boost - 1;
		}

		/* Boost power */
		if (scale > 100)
		{
			scale = (scale - 100) / 2 + 100;
			n_ptr->power = (r_ptr->power * scale) / 100;
		}
	}

	/* Recheck scale */
	if (scale < 100) return (TRUE);

	/* Scale up for age or not done */
	if (scale > 100)
	{
		/* Boost armour class next */
		boost = r_ptr->ac * scale / 200;

		/* Limit armour class improvement */
		if (boost > 50 + n_ptr->ac / 3) boost = 50 + n_ptr->ac / 3;

		/* Improve armour class */
		n_ptr->ac += boost;

		/* Reduce scale by actual scaled improvement in armour class */
		/* XXX Correction to avoid divide by zero issues */
		scale = scale * (r_ptr->ac + 1) / (n_ptr->ac + 1);

		/* Boost speed next */
		boost = scale / 200;

		/* Limit to +15 faster */
		if (boost > 15) boost = 15;

		/* Increase fake speed */
		n_ptr->speed += boost;

		/* Reduce scale by actual improvement in energy */
		scale = scale * extract_energy[r_ptr->speed] / extract_energy[n_ptr->speed];

		/* Boost attack damage partially */
		if (scale > 200)
			for (i = 0; i < 4; i++)
		{
			if (!n_ptr->blow[i].d_side) continue;

			boost = (scale * n_ptr->blow[i].d_dice / 50) / n_ptr->blow[i].d_side;

			if (boost > 9 * n_ptr->blow[i].d_dice) boost = 9 * n_ptr->blow[i].d_dice;

			if (boost > 1) n_ptr->blow[i].d_dice += boost - 1;
		}

		/* Boost power */
		if (scale > 100)
		{
			scale = (scale - 100) / 2 + 100;
			n_ptr->power = (r_ptr->power * scale) / 100;

			/* Not done? */
			if (scale > 100)
			{
				/* Boost hit points -- unlimited */
				n_ptr->hp = n_ptr->hp * scale / 100;
			}
		}
	}

	return (TRUE);
}



/*
 * Determine if a monster attack against the player succeeds.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 */
static bool check_hit(int power, int level, int who, bool ranged)
{
	int k, ac;
	int blocking = 10;

	/* Object  */
	object_type *o_ptr = &inventory[INVEN_ARM];

	/* Is player blocking? */
	if (p_ptr->blocking > 1)
	{
		/* No shield / secondary weapon */
		if (!o_ptr->k_idx)
		{
			if (inventory[INVEN_WIELD].k_idx)
			{
				o_ptr = &inventory[INVEN_WIELD];
			}
			else if (inventory[INVEN_HANDS].k_idx)
			{
				o_ptr = &inventory[INVEN_HANDS];
			}
		}

		/* Modify by object */
		if (o_ptr->k_idx)
		{
			/* Adjust by ac factor */
			blocking += o_ptr->ac + o_ptr->to_a;
		}

		/* Modify by style */
		if (!p_ptr->heavy_wield)
		{
			int i;

			/* Base blocking */
			blocking += p_ptr->to_h + 3;

			for (i = 0; i < z_info->w_max; i++)
			{
				if (w_info[i].class != p_ptr->pclass) continue;

				if (w_info[i].level > p_ptr->lev) continue;

				/* Check for styles */
				if (w_info[i].styles==0
					|| w_info[i].styles & (p_ptr->cur_style & (WS_WIELD_FLAGS)) & (1L << p_ptr->pstyle))
				{
					switch (w_info[i].benefit)
					{
						case WB_HIT:
						case WB_AC:
							blocking += (p_ptr->lev - w_info[i].level) /2;
							break;
					}
				}
			}
		}

		/* Player condition */
		if ((p_ptr->timed[TMD_BLIND]) || (p_ptr->timed[TMD_CONFUSED]) || (p_ptr->timed[TMD_IMAGE]) || (p_ptr->timed[TMD_BERSERK]))
			blocking /= 2;
	}

	/* Determine monster to hit chance */
	if (who > SOURCE_MONSTER_START)
	{
		monster_type *m_ptr = &m_list[who];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Monster never misses */
		if (r_ptr->flags9 & (RF9_NEVER_MISS)) return (TRUE);

		/* Calculate the "attack quality".  Blind monsters are greatly hindered. Stunned monsters are hindered. */
		power = (power + (m_ptr->blind ? level * 1 : (m_ptr->stunned ? level * 3 : level * 5)));

		/* Apply monster stats */
		if (m_ptr->mflag & (MFLAG_CLUMSY)) power -= 5;
		else if (m_ptr->mflag & (MFLAG_SKILLFUL)) power += 5;

		/* Apply temporary conditions */
		if (m_ptr->bless) power += 10;
		if (m_ptr->berserk) power += 24;

		/* Blind monsters almost always miss at ranged combat */
		if ((ranged) && (m_ptr->blind)) power /= 10;
	}
	else
	{
		/* Level counts for full amount */
		power += level * 3;
	}

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Always miss or hit */
	if (k < 10) return (k < 5);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Modify for blocking */
	ac += blocking;

	/* Some items and effects count for more at range */
	if (ranged)
	{
		object_type *o_ptr = &inventory[INVEN_ARM];

		/* Shields count for double */
		if ((o_ptr->k_idx) && (o_ptr->tval == TV_SHIELD))
		{
			/* Add shield ac again */
			ac += o_ptr->ac;
			ac += o_ptr->to_a;
		}

		/* Shield spell counts for double */
		if (p_ptr->timed[TMD_SHIELD]) ac += 50;

		/* Ill winds help a lot */
		if ((p_ptr->cur_flags4 & (TR4_WINDY)) ||
				room_has_flag(p_ptr->py, p_ptr->px, ROOM_WINDY)) ac += 100;
	}

	/* Some rooms make the player vulnerable */
	if (room_has_flag(p_ptr->py, p_ptr->px, ROOM_CURSED)) ac /= 2;

	/* Power and Level compete against Armor */
	if (power > 0)
	{
		/* Armour or blocking protects */
		if (randint(power) > ((ac * 3) / 4)) return (TRUE);

		/* Give blocking message to encourage blocking */
		if (randint(power) < (blocking)) msg_print("You block the attack.");
	}

	/* Assume miss */
	return (FALSE);
}


/*
 * Describe attack
 *
 * Returns -1 if no attack described.
 * Otherwise returns the description selected. If various alternative
 * descriptions are applied for the same description type, it returns
 * the description 'band' chosen from.
 */
int attack_desc(char *buf, int target, int method, int effect, int damage, u16b flg, int buf_size)
{
	char t_name[80];
	char t_poss[80];

	const char *s;
	char *t;

	method_type *method_ptr = &method_info[method];

	int i, c, k;

	int div = 1;
	int mod = 0;

	bool punctuate = (flg & (ATK_DESC_NO_STOP)) == 0;
	bool uppercase = FALSE;

	int state = 0;
	int match = (flg & (ATK_DESC_TENSE)) != 0 ? 1 : 2;

	/* Describe target */
	if (flg & (ATK_DESC_SELF))
	{
		/* Get the monster reflexive ("himself"/"herself"/"itself") */
		monster_desc(t_name, sizeof(t_name), target, 0x23);

		/* Get the monster possessive ("his"/"her"/"its") */
		monster_desc(t_poss, sizeof(t_poss), target, 0x22);
	}
	else if (target > 0)
	{
		/* Get the monster name (or "it") */
		monster_desc(t_name, sizeof(t_name), target, 0x00);

		/* Get the monster possessive ("the goblin's") */
		monster_desc(t_poss, sizeof(t_poss), target, 0x02);
	}
	else if (target < 0)
	{
		my_strcpy(t_name,"you", sizeof(t_name));
		my_strcpy(t_poss,"your", sizeof(t_poss));
	}
	else
	{
		my_strcpy(t_name,"it", sizeof(t_name));
		my_strcpy(t_poss,"its", sizeof(t_poss));
	}

	/* Check for blows which are of alternate type - the attack is stunning as opposed to cutting */
	if ((method_ptr->flags2 & (PR2_ALTERNATE)) && (flg & (ATK_DESC_PRIMARY | ATK_DESC_ALTERNATE)))
	{
		div++;

		if (flg & (ATK_DESC_ALTERNATE)) mod++;
	}

	/* Check for blows which are heard as opposed to seen because the player is blind */
	if (method_ptr->flags2 & (PR2_HEARD))
	{
		div++;

		if (flg & (ATK_DESC_HEARD)) mod++;
	}

	/* Check for blows which are indirect - the caster/attacker is out of line of sight */
	if (method_ptr->flags2 & (PR2_INDIRECT))
	{
		div++;

		if (flg & (ATK_DESC_INDIRECT)) mod++;
	}

	/* Pick the appropriate attack description */
	for (i = 0, k = 0, c = -1; i < MAX_METHOD_DESCRIPTIONS; i++)
	{
		/* Skip empty blows */
		if (method_ptr->desc[i].max == 0) continue;

		/* Skip seen and/or direct blows as required */
		if ((div > 1) && (i % div != mod)) continue;

		/* Check blow damage */
		if ((damage >= method_ptr->desc[i].min) &&
				(damage <= method_ptr->desc[i].max))
		{
			/* Pick a description that qualifies */
			if ((flg & (ATK_DESC_LAST)) || (!rand_int(++k))) c = i;
		}
	}

	/* No valid description */
	if (c < 0) return (c);

	/* Start dumping the result */
	t = buf;

	/* Begin */
	s = method_text + method_ptr->desc[c].text;

	/* Copy the string */
	for (; (*s) && ((t - buf) < buf_size); s++)
	{
		/* Handle tense changes */
		if (*s == '|')
		{
			state++;
			if (state == 3) state = 0;
		}

		/* Handle input suppression */
		if ((state) && (state != match))
		{
			continue;
		}

		/* Handle tense changes */
		if (*s == '|')
		{
			continue;
		}

		/* String ends in punctuation? */
		else if ((*s == '.') || (*s == '!') || (*s == '?'))
		{
			punctuate = FALSE;
			uppercase = TRUE;
		}

		/* String doesn't end in punctuation */
		else
		{
			punctuate = (flg & (ATK_DESC_NO_STOP)) == 0;
		}

		/* Handle the target*/
		if (*s == '&')
		{
			/* Append the name */
			cptr u = t_name;

			while ((*u) && ((t - buf) < buf_size))
			{
				*t++ = *u++;
				if (uppercase)
				{
					if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
					uppercase = FALSE;
				}
			}
		}

		/* Handle the target*/
		else if (*s == '%')
		{
			/* Append the name */
			cptr u = t_poss;

			while ((*u) && ((t - buf) < buf_size))
			{
				*t++ = *u++;
				if (uppercase)
				{
					if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
					uppercase = FALSE;
				}
			}
		}

		/* Handle the target */
		else if (*s == '$')
		{
			/* Append the name */
			cptr u = target < 0 ? "are" : "is";

			while ((*u) && ((t - buf) < buf_size))
			{
				*t++ = *u++;
				if (uppercase)
				{
					if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
					uppercase = FALSE;
				}
			}
		}

		/* Handle effect */
		else if (*s == '*')
		{
			/* Append the name */
			cptr u = effect_text + effect_info[effect].desc[0];

			while ((*u) && ((t - buf) < buf_size))
			{
				*t++ = *u++;
				if (uppercase)
				{
					if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
					uppercase = FALSE;
				}
			}
		}

		/* Handle participle */
		else if (*s == '@')
		{
			cptr u;
			cptr v = s+1;

			/* Skip white space */
			for ( ; (*v) && (*v == ' '); v++) ;

			/* Is a vowel -- note hacks to guess vowelness of other string substitutions */
			if (is_a_vowel(*v) || (*v == '$') ||
					((*v == '*') && is_a_vowel((effect_text + effect_info[effect].desc[0])[0])))
			{
				u = "an";
			}
			else
			{
				u = "a";
			}

			/* Copy */
			while ((*u) && ((t - buf) < buf_size))
			{
				*t++ = *u++;
				if (uppercase)
				{
					if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
					uppercase = FALSE;
				}
			}
		}

		/* Tensor */
		else if (*s == '~')
		{
			/* Add an 's' */
			if (match == 2) *t++ = 's';
		}

		/* Normal */
		else
		{
			/* Copy */
			*t++ = *s;

			if (uppercase)
			{
				if ((*t >= 'a') && (*t <= 'z')) *t -= 32;
				uppercase = FALSE;
			}
		}
	}

	/* Terminate the string with '.' or '!' */
	if ((punctuate) && ((t - buf) <  buf_size))
	{
		if (flg & (ATK_DESC_EXCLAIM))
		{
			*t++ = '!';
		}
		else
		{
			*t++ = '.';
		}
	}

	/* Terminate */
	if ((t - buf) <  buf_size) *t = '\0';

	/* Truncate the string to buf_size chars */
	buf[buf_size - 1] = '\0';

	/* Return choice */
	return (c / div);
}


/*
 * Determine if a monster attack against another monster succeeds.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against monster armor.
 */
bool mon_check_hit(int m_idx, int power, int level, int who, bool ranged)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_type *n_ptr = &m_list[who];
	monster_race *r_ptr = &r_info[n_ptr->r_idx];

	int k, ac;

	/* Monster never misses */
	if (r_ptr->flags9 & (RF9_NEVER_MISS)) return (TRUE);

	/* Calculate the "attack quality".  Blind monsters are greatly hindered. Stunned monsters are hindered. */
	power = (power + (n_ptr->blind ? level * 1 : (n_ptr->stunned ? level * 3 : level * 5)));

	/* Apply monster stats */
	if (n_ptr->mflag & (MFLAG_CLUMSY)) power -= 5;
	else if (n_ptr->mflag & (MFLAG_SKILLFUL)) power += 5;

	/* Apply temporary conditions */
	if (n_ptr->bless) power += 10;
	if (n_ptr->berserk) power += 24;

	/* Blind monsters almost always miss at ranged combat */
	if ((ranged) && (n_ptr->blind)) power /= 10;

	/* Monsters can evade */
	if (mon_evade(m_idx,
		(ranged ? 5 : 3) + (m_ptr->confused
				 || m_ptr->stunned ? 1 : 3),
		9, "")) return (FALSE);

	/* Huge monsters are hard to hit. */
	if (r_info[m_ptr->r_idx].flags3 & (RF3_HUGE))
	{
		/* Easier for climbers, flyers and other huge monsters */
		if (((n_ptr->mflag & (MFLAG_OVER)) == 0)
			 && (r_ptr->flags3 & (RF3_HUGE)) &&
			 rand_int(100) < ((r_ptr->flags2 & (RF3_GIANT)) ? 35 : 70)) return (FALSE);
	}

	/* Hack -- make armoured monsters much more effective against monster arrows. */
	if ((ranged) && (r_info[m_ptr->r_idx].flags2 & (RF2_ARMOR)))
	{
		/* XXX Tune this to make warriors useful against archers. */
		if (rand_int(100) < 40) return (FALSE);
	}

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Always miss or hit */
	if (k < 10) return (k < 5);

	/* Total armor */
	ac = calc_monster_ac(m_idx, ranged);

	/* Power and Level compete against Armor */
	if (power > 0)
	{
		/* Armour or blocking protects */
		if (randint(power) > ((ac * 3) / 4)) return (TRUE);
	}

	/* Assume miss */
	return (FALSE);
}


/*
 * Attack the player via physical attacks.
 * TODO: join with other (monster?) attack routines
 * 
 * Harmless indicates we only do harmless attacks
 */
bool make_attack_normal(int m_idx, bool harmless)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int ap_cnt;

	int tmp, ac, rlev;
	bool do_cut, do_stun, touched = FALSE;

	char m_name[80];
	char atk_desc[80];

	char ddesc[80];

	bool blinked = FALSE;

	bool shrieked = FALSE;

	byte flg = 0L;

	int result;

	/* Not allowed to attack */
	if (r_ptr->flags1 & (RF1_NEVER_BLOW)) return (FALSE);

	/* Player in stastis */
	if (p_ptr->timed[TMD_STASTIS]) return (FALSE);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);


	/* Get the monster name (or "it") */
	monster_desc(m_name, sizeof(m_name), m_idx, 0);

	/* Get the "died from" information (i.e. "a goblin") */
	monster_desc(ddesc, sizeof(ddesc), m_idx, 0x88);


	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool visible = FALSE;
		bool obvious = FALSE;

		int damage = 0;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		method_type *method_ptr = &method_info[method];
		effect_type *effect_ptr = &effect_info[effect];

		/* Hack -- no more attacks */
		if (!method) break;

		/* Handle "leaving" */
		if (p_ptr->leaving) break;
		
		/* Exclude attacks which damage the player if we're being harmless */
		if ((harmless) && (effect)) continue;

		/* Handle "automatic" attacks */
		if (method_ptr->flags2 & (PR2_AUTOMATIC))
		{
			make_attack_ranged(m_idx, 96 + ap_cnt, p_ptr->py, p_ptr->px);

			continue;
		}

		/* Get whether attack cuts, stuns, touches */
		do_cut = (method_ptr->flags2 & (PR2_CUTS)) != 0;
		do_stun = (method_ptr->flags2 & (PR2_STUN)) != 0;
		shrieked |= (method_ptr->flags2 & (PR2_SHRIEK)) != 0;
		touched |= (method_ptr->flags2 & (PR2_TOUCH)) != 0;

		if (cheat_xtra) msg_format("base dice in this blow: dice %d, sides %d", d_dice, d_side);

		/* Extract visibility (before blink) */
		if (m_ptr->ml) visible = TRUE;

		/* Skip 'tricky' attacks */
		if ((method_ptr->flags2 & (PR2_MELEE)) == 0) continue;

		if (cheat_xtra) msg_format("dice (2) in this blow: dice %d, sides %d", d_dice, d_side);

		/* Apply monster stats */
		if (d_side > 1)
		{
			/* Apply monster stats */
			if (m_ptr->mflag & (MFLAG_WEAK)) d_side = MIN(d_side - 2, d_side * 9 / 10);
			else if (m_ptr->mflag & (MFLAG_STRONG)) d_side = MAX(d_side + 2, d_side * 11 / 10);

			if (d_side <= 0) d_side = 1;
		}

		if (cheat_xtra) msg_format("dice (3) in this blow: dice %d, sides %d", d_dice, d_side);

		/* Roll out the damage */
		damage = damroll(d_dice, d_side);

		/* Monster hits player */
		if (!effect || check_hit(effect_ptr->power, rlev, m_idx, FALSE))
		{
			/* Always disturbing */
			disturb(1, 1);

			/* Hack -- Apply "protection from evil" */
			if ((p_ptr->timed[TMD_PROTEVIL] > 0) &&
			    (r_ptr->flags3 & (RF3_EVIL)) &&
			    (p_ptr->lev >= rlev) &&
			    ((rand_int(100) + p_ptr->lev) > 50))
			{
				/* Remember the Evil-ness */
				if (m_ptr->ml)
				{
					l_ptr->flags3 |= (RF3_EVIL);
				}

				/* Message */
				msg_format("%^s is repelled.", m_name);

				/* Hack -- Next attack */
				continue;
			}

			/* Hack -- force cutting on wounds */
			if (effect == GF_WOUND)
			{
				flg |= (ATK_DESC_PRIMARY);
				if (!rand_int(5)) do_stun = 0;
				else do_cut = 0;
			}

			/* Hack -- force stunning on batter */
			if (effect == GF_BATTER)
			{
				flg |= (ATK_DESC_ALTERNATE);
				if (!rand_int(5)) do_cut = 0;
				else do_stun = 0;
			}

			/* Get the attack string */
			result = attack_desc(atk_desc, -1, method, effect, damage, flg, 80);

			/* Describe the attack */
			if (result >= 0) msg_format("%^s %s", m_name, atk_desc);

			/* Slime */
			if (method_ptr->flags2 & (PR2_SLIME))
			{
				/* Slime player */
				if (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags1 & (FF1_FLOOR))
					feat_near(p_ptr->py, p_ptr->px, FEAT_FLOOR_SLIME_T);
			}

			/* Blood */
			if (damage > p_ptr->chp / 3)
			{
				/* Player loses blood */
				if (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags1 & (FF1_FLOOR))
					feat_near(p_ptr->py, p_ptr->px, FEAT_FLOOR_BLOOD_T);
			}

			/* Check for usage */
			if (TRUE)
			{
				int slot;
				int y = m_ptr->fy;
				int x = m_ptr->fx;

				object_type *o_ptr;

				/* Pick a (possibly empty) inventory slot */
				switch (randint(6))
				{
					case 1: slot = INVEN_BODY; break;
					case 2: slot = INVEN_ARM; break;
					case 3: slot = INVEN_OUTER; break;
					case 4: slot = INVEN_HANDS; break;
					case 5: slot = INVEN_HEAD; break;
					default: slot = INVEN_FEET; break;
				}

				/* Get object */
				o_ptr = &inventory[slot];

				/* Object used? */
				if (o_ptr->k_idx) object_usage(slot);

				/* Apply additional effect from activation */
				if (o_ptr->k_idx && auto_activate(o_ptr))
				{
					/* Make item strike */
					process_item_blow(o_ptr->name1 ? SOURCE_PLAYER_ACT_ARTIFACT : (o_ptr->name2 ? SOURCE_PLAYER_ACT_EGO_ITEM : SOURCE_PLAYER_ACTIVATE),
							o_ptr->name1 ? o_ptr->name1 : (o_ptr->name2 ? o_ptr->name2 : o_ptr->k_idx), o_ptr, y, x, TRUE, FALSE);
				}

				/* Apply additional effect from coating*/
				else if (o_ptr->k_idx && coated_p(o_ptr))
				{
					/* Make item strike */
					process_item_blow(SOURCE_PLAYER_COATING, lookup_kind(o_ptr->xtra1, o_ptr->xtra2), o_ptr, y, x, TRUE, TRUE);
				}
			}

			/* Player armor reduces total damage */
			damage -= (damage * ((p_ptr->ac + p_ptr->to_a < 150) ? p_ptr->ac + p_ptr->to_a: 150) / 250);

			if (cheat_xtra) msg_format("base damage dealt by monster in this blow: %d, dice %d, sides %d", damage, d_dice, d_side);

			if (effect)
			{
				/* New result routine */
				if (project_one(m_idx, ap_cnt, p_ptr->py, p_ptr->px, damage, effect, (PROJECT_PLAY | PROJECT_HIDE)))
				{
					obvious = TRUE;

					if ((effect == GF_EAT_ITEM) || (effect == GF_EAT_GOLD)) blinked = TRUE;
				}

				/* Hack -- only one of cut or stun */
				if (do_cut && do_stun)
				{
					/* Cancel cut */
					if (result % 2)
					{
						do_cut = FALSE;
					}
					/* Cancel stun */
					else
					{
						do_stun = FALSE;
					}
				}

				/* Handle cut */
				if (do_cut)
				{
					int k;

					/* Critical hit (zero if non-critical) */
					tmp = monster_critical(d_dice, d_side, damage, effect);

					/* Roll for damage */
					switch (tmp)
					{
						case 0: k = 0; break;
						case 1: k = randint(5); break;
						case 2: k = randint(5) + 5; break;
						case 3: k = randint(20) + 20; break;
						case 4: k = randint(50) + 50; break;
						case 5: k = randint(100) + 100; break;
						case 6: k = 300; break;
						default: k = 500; break;
					}

					/* Apply the cut */
					if (k) (void)set_cut(p_ptr->timed[TMD_CUT] + k);
				}

				/* Handle stun */
				if (do_stun)
				{
					int k;

					/* Critical hit (zero if non-critical) */
					tmp = monster_critical(d_dice, d_side, damage, effect);

					/* Roll for damage */
					switch (tmp)
					{
						case 0: k = 0; break;
						case 1: k = randint(5); break;
						case 2: k = randint(8) + 8; break;
						case 3: k = randint(15) + 15; break;
						case 4: k = randint(25) + 25; break;
						case 5: k = randint(35) + 35; break;
						case 6: k = randint(45) + 45; break;
						default: k = 100; break;
					}

					/* Apply the stun */
					if (k) inc_timed(TMD_STUN, p_ptr->timed[TMD_STUN] ? randint(MIN(k,10)) : k, TRUE);

				}
			}
			else
			{
				obvious = TRUE;
			}
		}

		/* Monster missed player */
		else if (touched)
		{
			/* Visible monsters */
			if (m_ptr->ml)
			{
				/* Disturbing */
				disturb(1, 0);

				/* Message */
				if (!(p_ptr->timed[TMD_PSLEEP])) msg_format("%^s misses you.", m_name);
			}
		}


		/* Analyze "visible" monsters only */
		if (visible)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (l_ptr->blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (l_ptr->blows[ap_cnt] < MAX_UCHAR)
				{
					l_ptr->blows[ap_cnt]++;
				}
			}
		}
	}

	/* Blink away */
	if (blinked)
	{
		/* Hack -- haste and flee with the loot */
		if ((!m_ptr->monfear) && (rand_int(3)))
		{
			/* Set monster fast */
			set_monster_haste(m_idx, 3 + (s16b)rand_int(3), FALSE);

			/* Make monster 'panic' -- move away */
			set_monster_fear(m_ptr, m_ptr->hp / 3, TRUE);
		}
		else
		{
			msg_print("There is a puff of smoke!");
			teleport_hook = NULL;
			teleport_away(m_idx, MAX_SIGHT * 2 + 5);
		}
	}

	/* Shriek */
	if (shrieked)
	{
		aggravate_monsters(m_idx);
	}

	/* Always notice cause of death */
	if (p_ptr->is_dead && (l_ptr->deaths < MAX_SHORT))
	{
		l_ptr->deaths++;
	}


	/* Assume we attacked */
	return (TRUE);
}


/*
 * Using an input value for average damage, and another that controls
 * variability, return the actual base damage of a monster's attack
 * spell.  The larger the value for "control", the less likely the damage
 * will vary greatly.
 */
int get_dam(byte power, int attack, bool varies)
{
	int dam = 0;
	int spread;

	int control, av_dam;

	/* Determine mana cost */
	if (attack >= 224) return (FALSE);
	else if (attack >= 192)
	{
		av_dam = power * spell_info_RF7[attack-192][COL_SPELL_DAM_MULT];
		av_dam /= MAX(1, spell_info_RF7[attack-192][COL_SPELL_DAM_DIV]);
		control = spell_info_RF7[attack-192][COL_SPELL_DAM_VAR];
	}
	else if (attack >= 160)
	{
		av_dam = power * spell_info_RF6[attack-160][COL_SPELL_DAM_MULT];
		av_dam /= MAX(1, spell_info_RF6[attack-160][COL_SPELL_DAM_DIV]);
		control = spell_info_RF6[attack-160][COL_SPELL_DAM_VAR];
	}
	else if (attack >= 128)
	{
		av_dam = power * spell_info_RF5[attack-128][COL_SPELL_DAM_MULT];
		av_dam /= MAX(1, spell_info_RF5[attack-128][COL_SPELL_DAM_DIV]);
		control = spell_info_RF5[attack-128][COL_SPELL_DAM_VAR];
	}
	else if (attack >=  96)
	{
		av_dam = power * spell_info_RF4[attack- 96][COL_SPELL_DAM_MULT];
		av_dam /= MAX(1, spell_info_RF4[attack- 96][COL_SPELL_DAM_DIV]);
		control = spell_info_RF4[attack- 96][COL_SPELL_DAM_VAR];
	}
	else return (FALSE);

	/*Hack - handle Wound spell differently */
	if (attack == 160+20)
	{
		if (power > 75) av_dam = 225 + power - 75;
	}


	/*No point in going through this to return 0*/
	if ((av_dam < 1) || (!varies)) return (av_dam);

	/* Damage may never differ by more than 50% from the average */
	if (control < 4) control = 4;

	/*
	 * Get the allowable spread (two standard deviations, or 100,
	 * whichever is less).
	 */
	spread = MIN(100, av_dam * 2 / control);

	/* Paranoia - no spread */
	if (!spread) return (av_dam);
	
	/* Loop until damage is within the allowable spread */
	while (TRUE)
	{
		/* Randomize damage (average, standard deviation) */
		dam = Rand_normal(av_dam, av_dam / control);

		/* Forbid too great a variation */
		if (dam > av_dam + spread) continue;
		if (dam < av_dam - spread) continue;

		/* Accept */
		break;
	}

	/* Return randomized damage */
	return (dam);
}


/*
 * Determines the amount of damage caused by monster breaths.
 *
 * Monster breathers are amongst the highest damage sources in the game so
 * we need to be careful to limit the maximum damage inflicted in order to
 * allow the player a fighting chance.
 */
extern int get_breath_dam(s16b hp, int method, bool powerful)
{
	method_type *method_ptr = &method_info[method];

	int dam_div = powerful ? method_ptr->dam_div_powerful : method_ptr->dam_div;
	int dam_max = powerful ? method_ptr->dam_max_powerful : method_ptr->dam_max;

	/* Damage reduction */
	if (dam_div) hp /= dam_div;

	/* Limit maximum damage for breath weapons */
	if (dam_max && (hp > dam_max))
	{
		hp = dam_max;
	}

	return (hp);
}



#define FLG_MON_BEAM (PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | \
			PROJECT_PLAY | PROJECT_MAGIC)
#define FLG_MON_SHOT (PROJECT_STOP | PROJECT_KILL | PROJECT_PLAY | PROJECT_GRID)
#define FLG_MON_BALL_SHOT (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | \
			PROJECT_PLAY | PROJECT_WALL)
#define FLG_MON_BOLT (PROJECT_STOP | PROJECT_KILL | PROJECT_PLAY | PROJECT_GRID | PROJECT_MAGIC)
#define FLG_MON_BALL (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | \
			PROJECT_PLAY | PROJECT_WALL | PROJECT_MAGIC)
#define FLG_MON_AREA (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | \
			PROJECT_PLAY | PROJECT_WALL | PROJECT_AREA | PROJECT_MAGIC)
#define FLG_MON_8WAY (PROJECT_4WAY | PROJECT_4WAX | PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | \
			PROJECT_PLAY | PROJECT_WALL | PROJECT_AREA | PROJECT_MAGIC)
#define FLG_MON_CLOUD (PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | PROJECT_PLAY | \
			PROJECT_HIDE | PROJECT_WALL | PROJECT_MAGIC)
#define FLG_MON_ARC  (PROJECT_ARC | PROJECT_BOOM | PROJECT_GRID | PROJECT_ITEM | \
	           PROJECT_KILL | PROJECT_PLAY | PROJECT_WALL | PROJECT_MAGIC)
#define FLG_MON_DIRECT (PROJECT_JUMP | PROJECT_KILL | PROJECT_PLAY | PROJECT_MAGIC)





/*
 * Monster attempts to make a ranged melee attack.
 * TODO: join with other (monster?) attack routines
 *
 * Use by aura and trail effects and eating part
 * of the monster.
 */
void mon_blow_ranged(int who, int what, int x, int y, int method, int range, int flg)
{
	monster_lore *l_ptr;
	monster_race *r_ptr;

	int fy, fx;

	bool obvious, known;

	int i;

	if (who > SOURCE_MONSTER_START)
	{
		monster_type *m_ptr = &m_list[who];
		r_ptr = &r_info[m_ptr->r_idx];
		l_ptr = &l_list[m_ptr->r_idx];

		known = m_ptr->ml;

		fy = m_ptr->fy;
		fx = m_ptr->fx;
	}
	else if ((who == SOURCE_SELF) || (who == SOURCE_PLAYER_ALLY))
	{
		monster_type *m_ptr = &m_list[what];
		r_ptr = &r_info[m_ptr->r_idx];
		l_ptr = &l_list[m_ptr->r_idx];

		known = m_ptr->ml;

		fy = m_ptr->fy;
		fx = m_ptr->fx;
	}
	else if (who == SOURCE_PLAYER_EAT_MONSTER)
	{
		r_ptr = &r_info[what];
		l_ptr = &l_list[what];

		fy = p_ptr->py;
		fx = p_ptr->px;

		known = TRUE;
	}
	else
	{
		return;
	}

	/* Scan through all four blows */
	for (i = 0; i < 4; i++)
	{
		/* End of attacks */
		if (!(r_ptr->blow[i].method)) break;

		/* Skip if not spores */
		if (r_ptr->blow[i].method != method) continue;

		/* Target the player with a ranged attack */
		obvious = project(who, what, range, range, fy, fx, y, x, damroll(r_ptr->blow[i].d_side,
					r_ptr->blow[i].d_dice),r_ptr->blow[i].effect, flg, 0, 0);

		/* Analyze "visible" monsters only */
		if (known)
		{
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
	}
}


/*
 * Monsters can concentrate light or conjure up darkness.
 *
 * Also, druidic type monsters use water and trees to boost
 * their recovery of hit points and mana.
 *
 * Return TRUE if the monster did anything.
 *
 * Note we force 'destroy' to FALSE to keep druidic monsters
 * more interesting. This means they don't destroy the
 * resources that they concentrate - forcing the player to
 * consider how to do so.
 */
static int mon_concentrate_power(int y, int x, int spower, bool use_los, bool destroy, bool concentrate_hook(const int y, const int x, const bool modify))
{
	int damage, radius, lit_grids;

	/* Radius of darkness-creation varies depending on spower */
	radius = MIN(6, 3 + spower / 20);

	/* Check to see how much we would gain (use a radius of 6) */
	lit_grids = concentrate_power(y, x, 6,
		FALSE, use_los, concentrate_hook);

	/* We have enough juice to make it worthwhile (make a hasty guess) */
	if (lit_grids >= rand_range(40, 60))
	{
		/* Actually concentrate the light */
		(void)concentrate_power(y, x, radius,
			destroy, use_los, concentrate_hook);

		/* Calculate damage (60 grids => break-even point) */
		damage = lit_grids * spower / 20;

		/* Limit damage, but allow fairly high values */
		if (damage > 9 * spower / 2) damage = 9 * spower / 2;

		/* We did something */
		return (damage);
	}

	/* We decided not to do anything */
	return (0);
}


/*
 * Get Sauron's new shape.
 */
int sauron_shape(int old_form)
{
	int i, k = 0;

	int r_idx = SAURON_TRUE;

	/* Player has killed the true Sauron */
	if (r_info[SAURON_TRUE].cur_num >= r_info[SAURON_TRUE].max_num) return (0);

	/* Oops - we have a true Sauron */
	if ((r_info[SAURON_TRUE].cur_num) && (old_form != SAURON_TRUE)) return (0);

	/* Check that another form is not on this level */
	for (i = SAURON_FORM; i < SAURON_FORM + MAX_SAURON_FORMS; i++)
	{
		if ((r_info[i].cur_num) && (old_form != i)) return (0);
	}

	/* Choose a form */
	for (i = SAURON_FORM; i < SAURON_FORM + MAX_SAURON_FORMS; i++)
	{
		/* Never pick old shape */
		if (i == old_form) continue;

		/* Previously killed this shape on this level allows the 'true' form to be fought. */
		if ((p_ptr->sauron_forms & (1 << (i - SAURON_FORM))) && one_in_(++k))
		{
			r_idx = SAURON_TRUE;
			continue;
		}

		/* Player has killed this shape */
		if (r_info[i].cur_num >= r_info[i].max_num) continue;

		/* Pick the shape */
		if (one_in_(++k)) r_idx = i;
	}

	return (r_idx);
}



/*
 * Monster attempts to make a ranged (non-melee) attack.
 * TODO: join with other (monster?) attack routines
 *
 * Determine if monster can attack at range, then see if it will.  Use
 * the helper function "choose_attack_spell()" to pick a physical ranged
 * attack, magic spell, or summon.  Execute the attack chosen.  Process
 * its effects, and update character knowledge of the monster.
 *
 * Perhaps monsters should breathe at locations *near* the player,
 * since this would allow them to inflict "partial" damage.
 *
 * XXX We currently only call this routine with the following values of
 * 'who'.
 *
 * who > SOURCE_MONSTER_START: 	source is monster
 * who == SOURCE_FEATURE:		source is feature
 * who == SOURCE_PLAYER_TRAP:	source is player trap
 */
bool make_attack_ranged(int who, int attack, int y, int x)
{
	int k, rlev, spower;

	method_type *method_ptr = &method_info[attack];

	int method = attack;
	int effect = method_ptr->d_res;

	effect_type *effect_ptr = &effect_info[effect];

	monster_type *m_ptr, *n_ptr;
	monster_race *r_ptr, *s_ptr;
	monster_lore *l_ptr, *k_ptr;

	char m_name[80];
	char m_poss[80];

	char t_name[80];
	char t_poss[80];
	char t_nref[80]; /* Not reflexive if required */

	int result;
	int dam;
	int what;

	int target = cave_m_idx[y][x];

	/* Summoner */
	int summoner = 0;

	/* Summon count */
	int count = 0;

	/* Summon level */
	int summon_lev = 0;

	/* Is the player blind */
	bool blind = (p_ptr->timed[TMD_BLIND] ? TRUE : FALSE);

	bool seen;	/* Source seen */
	bool known;	/* Either source or target seen */
	bool powerful;
	bool normal;
	bool direct;

	u32b allies = 0L;

	char atk_desc[80];
	int dam_desc;

	byte atk_flg = 0;

	int fy = y;
	int fx = x;

	int range = 0;

	u32b flg;

	/* Some summons override cave ecology */
	bool old_cave_ecology = cave_ecology.ready;

	/* Hack -- Confused monsters get a random target */
	if ((who > SOURCE_MONSTER_START) && (target != who) && (m_list[who].confused) && (m_list[who].confused > rand_int(33)))
	{
		int dir = randint(8);
		int path_n;
		u16b path_g[256];
		int i, ty, tx;

		/* Get the monster */
		m_ptr = &m_list[who];

		/* Predict the "target" location */
		ty = m_ptr->fy + 99 * ddy_ddd[dir];
		tx = m_ptr->fx + 99 * ddx_ddd[dir];

		/* Calculate the path */
		path_n = project_path(path_g, MAX_SIGHT, m_ptr->fy, m_ptr->fx, &ty, &tx, 0);

		/* Hack -- Handle stuff */
		handle_stuff();

		/* Hack -- default is to start at monster and go in random dir */
		y = m_ptr->fy + ddy[dir];
		x = m_ptr->fx + ddx[dir];

		/* Project along the path */
		for (i = 0; i < path_n; ++i)
		{
			int ny = GRID_Y(path_g[i]);
			int nx = GRID_X(path_g[i]);

			/* Hack -- Stop before hitting walls */
			if (!cave_project_bold(ny, nx)) break;

			/* Advance */
			x = nx;
			y = ny;

			/* Handle monster */
			if (cave_m_idx[y][x]) break;
		}
	}

	/* Hack -- Blind monsters shoot near the target */
	else if ((who > SOURCE_MONSTER_START) && (target != who) && (m_list[who].blind))
	{
		int ty = y;
		int tx = x;

		/* Get the monster */
		m_ptr = &m_list[who];

		/* Scatter the target */
		/* Aggressive monsters are much more accurate */
		scatter(&ty, &tx, y, x,(distance(m_ptr->fy, m_ptr->fx, y, x)+1) / (m_ptr->mflag & (MFLAG_AGGR) ? 4 : 2), CAVE_XLOF);

		/* Don't target self */
		if ((ty != m_ptr->fy) || (tx != m_ptr->fx))
		{
			y = ty;
			x = tx;
		}
	}

	/* Describe target - target is itself */
	if (((who > 0) && (who == target)) || (!who))
	{
		/* Paranoia */
		if (!who) who = target;

		n_ptr = &m_list[who];
		s_ptr = &r_info[n_ptr->r_idx];
		k_ptr = &l_list[who];

		/* Get the monster name (or "it") */
		monster_desc(t_nref, sizeof(t_name), target, 0x00);

		/* Get the monster reflexive ("himself"/"herself"/"itself") */
		monster_desc(t_name, sizeof(t_nref), target, 0x23);

		/* Get the monster possessive ("his"/"her"/"its") */
		monster_desc(t_poss, sizeof(t_poss), target, 0x22);

		/* Targetting self */
		atk_flg |= (ATK_DESC_SELF);
	}
	/* Describe target - target is another monster */
	else if (target > 0)
	{
		n_ptr = &m_list[target];
		s_ptr = &r_info[n_ptr->r_idx];
		k_ptr = &l_list[target];

		/* Get the monster name (or "it") */
		monster_desc(t_name, sizeof(t_name), target, 0x00);

		/* Get the monster name (or "it") */
		monster_desc(t_nref, sizeof(t_nref), target, 0x00);

		/* Get the monster possessive ("the goblin's") */
		monster_desc(t_poss, sizeof(t_poss), target, 0x02);
	}
	/* Describe target - target is the player */
	else if (target < 0)
	{
		n_ptr = &m_list[0];
		s_ptr = &r_info[0];
		k_ptr = &l_list[0];

		my_strcpy(t_name,"you", sizeof(t_name));
		my_strcpy(t_nref,"you", sizeof(t_nref));
		my_strcpy(t_poss,"your", sizeof(t_poss));
	}
	/* Describe target - target is an empty grid/feature */
	else
	{
		n_ptr = &m_list[0];
		s_ptr = &r_info[0];
		k_ptr = &l_list[0];

		my_strcpy(t_name,format("the %s",f_name + f_info[cave_feat[y][x]].name), sizeof(t_name));
		my_strcpy(t_nref,format("the %s",f_name + f_info[cave_feat[y][x]].name), sizeof(t_nref));
		my_strcpy(t_poss,format("the %s's",f_name + f_info[cave_feat[y][x]].name), sizeof(t_poss));
	}

	/* Describe caster - player traps and features only */
	if (who < SOURCE_MONSTER_START)
	{
		m_ptr = &m_list[0];
		l_ptr = &l_list[0];
		r_ptr = &r_info[0];

		what = cave_feat[y][x];
		
		/* Feature is seen */
		if (play_info[y][x] & (PLAY_SEEN))
		{
			my_strcpy(m_name,format("the %s", f_name + f_info[what].name), sizeof(m_name));
			my_strcpy(m_poss,format("the %s's", f_name + f_info[what].name), sizeof(m_poss));
		}
		else
		{
			my_strcpy(m_name,"it", sizeof(m_name));
			my_strcpy(m_poss,"its", sizeof(m_poss));
		}

		/* Hack -- Message text depends on feature description */
		seen = FALSE;

		/* Assume "normal" target */
		normal = (target < 0);

		/* Assume "projectable" */
		direct = TRUE;

		/* Check if known */
		known = player_can_see_bold(y,x);

		/* Fake the monster level */
		rlev = f_info[cave_feat[y][x]].power;

		/* Fake the spell power */
		spower = 2 + p_ptr->depth / 2;

		/* Fake the powerfulness */
		powerful = (p_ptr->depth > 50 ? TRUE : FALSE);

		/* No summoner */
		summoner = 0;

		/* Fake the summoning level */
		summon_lev = p_ptr->depth + 3;

		/* Get damage for breath weapons */
		if (method_ptr->flags2 & (PR2_BREATH))
		{
			/* Get the damage */
			dam = get_breath_dam(p_ptr->depth * 10, method, powerful);
			dam_desc = dam;
		}
		/* Get the damage */
		else
		{
			dam = get_dam(spower, attack, TRUE);
			dam_desc = spower;
		}
	}
	else
	{
		m_ptr = &m_list[who];
		l_ptr = &l_list[m_ptr->r_idx];
		r_ptr = &r_info[m_ptr->r_idx];

		/* Hack -- never make spell attacks if hidden */
		if (m_ptr->mflag & (MFLAG_HIDE)) return (FALSE);

		/* Get the monster name (or "it") */
		monster_desc(m_name, sizeof(m_name), who, 0x00);

		/* Get the monster possessive ("his"/"her"/"its") */
		monster_desc(m_poss, sizeof(m_poss), who, 0x22);

		/* Extract the "see-able-ness" */
		seen = !blind && m_ptr->ml;

		/* Extract the monster level.  Must be at least 1. */
		rlev = MAX(1, r_ptr->level);

		/* Extract spell power */
		spower = r_ptr->spell_power;

		/* Extract the powerfulness */
		powerful = (r_ptr->flags2 & (RF2_POWERFUL) ? TRUE : FALSE);

		/* Extract the summoner */
		summoner = m_list[who].r_idx;

		/* Extract the summoning level.  Must be at least 1. */
		summon_lev = MAX(1, (r_ptr->level + p_ptr->depth) / 2 - 1);

		/* Hack -- guardians can summon any monster */
		if (r_ptr->flags1 & (RF1_GUARDIAN)) cave_ecology.ready = FALSE;

		/* Extract the source location */
		fy = m_ptr->fy;
		fx = m_ptr->fx;

		/* Fake the method for blow_1 etc. */
		if ((attack >= 96) && (attack < 96 + 4))
		{
			int d_dice = r_ptr->blow[attack - 96].d_dice;
			int d_side = r_ptr->blow[attack - 96].d_side;

			method = r_ptr->blow[attack - 96].method;
			effect = r_ptr->blow[attack - 96].effect;

			/* Get the damage */
			if ((d_dice) && (d_side)) dam = damroll(d_dice, d_side);
			else dam = 0;

			/* Get the damage description */
			dam_desc = dam;

			/* Get the new blow info */
			method_ptr = &method_info[method];
		}

		/* Get damage for spell attacks */
		else
		{
			dam = get_dam(spower, attack, TRUE);

			dam_desc = spower;
		}

		/* Get damage for breath weapons */
		if (method_ptr->flags2 & (PR2_BREATH))
		{
			dam = get_breath_dam(m_ptr->hp, method, powerful);

			dam_desc = dam;
		}

		/* Get range to target */
		range = m_ptr->cdis;

		/* Attack needs mana to cast */
		if (method_ptr->mana_cost)
		{
			/* Determine mana cost */
			int manacost = method_ptr->mana_cost;

			/* Adjust monster mana */
			m_ptr->mana -= MIN(manacost, m_ptr->mana);
		}

		/* Attack needs ammunition to use */
		if (method_ptr->ammo_tval)
		{
			/* Use ammunition */
			int ammo = find_monster_ammo(who, attack - 96, FALSE);

			/* Reduce ammunition */
			if (ammo > 0)
			{
				floor_item_increase(ammo, -1);
				floor_item_optimize(ammo);
			}
			/* Out of ammunition */
			else if (ammo < 0)
			{
				/* Message -- only stupid monsters get this and not really applicable to them*/
				/*if (m_ptr->ml) msg_format("%^s is out of ammunition.", m_name);*/

				return (TRUE);
			}
		}

		/* Check for spell failure (breath/shot attacks never fail) */
		if (method_ptr->flags2 & (PR2_FAIL))
		{
			/* Calculate spell failure rate */
			int failrate = 25 - (rlev + 3) / 4;

			/* Stunned monsters always have a chance to fail */
			if ((m_ptr->stunned) && (failrate < 10)) failrate = 10;

			/* Hack -- Stupid monsters will never fail (for jellies and such) */
			if (r_ptr->flags2 & (RF2_STUPID)) failrate = 0;

			/* Apply monster intelligence stat */
			if (m_ptr->mflag & (MFLAG_STUPID)) failrate = failrate * 3 / 2;
			else if (m_ptr->mflag & (MFLAG_SMART)) failrate /= 2;

			/* Message */
			msg_format("%^s tries to cast a spell, but fails.", m_name);

			return (TRUE);
		}

		/* Extract the monster's spell power. */
		spower = r_ptr->spell_power;

		/* Monster has cast a spell */
		m_ptr->mflag &= ~(MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH);

		if (target > 0)
		{
			known = ((m_ptr->ml || n_ptr->ml));

			/* Not "normal" target */
			normal = FALSE;
		}
		else if (target < 0)
		{
			/* Always known if target */
			known = TRUE;

			/* Assume "normal" target */
			normal = TRUE;
		}
		else
		{
			known = (m_ptr->ml && player_can_see_bold(y,x));

			/* Assume "normal" target */
			normal = TRUE;
		}

		/* Check "projectable" */
		direct = ((projectable(m_ptr->fy, m_ptr->fx, y, x, 0)) != PROJECT_NO);

		/* Attacking itself */
		if (who == target)
		{
			what = who;
			who = SOURCE_SELF;
			atk_flg |= (ATK_DESC_SELF);
		}

		/* Allies */
		else if (m_ptr->mflag & (MFLAG_ALLY))
		{
			what = who;
			who = SOURCE_PLAYER_ALLY;
		}

		/* Notice attack */
		else
		{
			what = attack;
		}
	}

	/* Get the method flag */
	flg = method_info[method].flags1;

	/* Player has chance of being missed by various ranged attacks */
	if (flg & (PROJECT_MISS))
	{
		/* See if we hit the player player */
		if ((target < 0) && check_hit(effect_ptr->power - BTH_RANGE_ADJ * range, rlev, who, TRUE))
		{
			/* Hit the player */
			flg &= ~(PROJECT_MISS);
		}
		else if ((target > 0) && mon_check_hit(target, effect_ptr->power - BTH_RANGE_ADJ * range, rlev, who, TRUE))
		{
			/* Hit the monster */
			flg &= ~(PROJECT_MISS);
		}
		else
		{
			/* Go through the target and hit something behind */
			flg |= (PROJECT_THRU | PROJECT_STOP);
		}
	}


	/* Centre on caster */
	if (method_ptr->flags1 & (PROJECT_SELF))
	{
		y = fy;
		x = fx;

		if (who > SOURCE_MONSTER_START)
		{
			what = who;
			who = SOURCE_SELF;
		}
	}

	/* Player is blind */
	if (p_ptr->timed[TMD_BLIND]) atk_flg |= (ATK_DESC_HEARD);

	/* Player is indirect */
	if (!direct) atk_flg |= (ATK_DESC_INDIRECT);

	/* Hack -- force cutting on wounds */
	if (effect == GF_WOUND) atk_flg |= (ATK_DESC_PRIMARY);

	/* Hack -- force stunning on batter */
	if (effect == GF_BATTER) atk_flg |= (ATK_DESC_ALTERNATE);

	/* Add ammunition */
	if (method_ptr->flags2 & (PR2_ADD_AMMO))
	{
		int ammo = 0;

		/* Requires a monster */
		if ((who > SOURCE_MONSTER_START) || (who == SOURCE_PLAYER_ALLY))
		{
			/* Grow ammunition */
			ammo = find_monster_ammo(who > SOURCE_MONSTER_START ? who : what, -1, TRUE);

			/* Describe - this should always happen */
			if (ammo >= 0)
			{
				/* Get the ammunition type */
				dam_desc = o_list[ammo].tval;
			}
		}
	}

	/* Weak attack */
	if (method_ptr->flags2 & (PR2_MAGIC_MISSILE))
	{
		/* Weaker magic missile */
		if (spower <= rlev / 10) atk_flg |= (ATK_DESC_LAST);
	}

	/* Get the attack description */
	result = attack_desc(atk_desc, target, method, effect, dam_desc, atk_flg, 80);

	/* Weaken powerful lightning */
	if (method_ptr->flags2 & (PR2_LIGHTNING_STRIKE))
	{
		/* Weaken damage */
		if ((result) && (spower >= 40)) dam = dam * 3 / 4;
	}

	/* Hack -- Weaken poison ball attacks */
	if ((effect == GF_POIS) && (result >= 4))
	{
		effect = result >= 6 ? GF_POISON_WEAK  : GF_POISON_HALF;

		effect_ptr = &effect_info[GF_POISON_HALF];

		/* Hack -- bump damage */
		dam *= 2;
	}

	/* Any effect? */
	if (result >= 0)
	{
		/* Describe the attack */
		msg_format("%^s %s", m_name, atk_desc);

		/* Some summons/projections override ecology */
		if (method_ptr->flags2 & (PR2_NO_ECOLOGY)) cave_ecology.ready = FALSE;

		/* Blow summons something */
		if (method_ptr->flags2 & (PR2_SUMMON))
		{
			int num = scale_method(method_ptr->number, rlev);

			int summon_type = method_ptr->summon_type;

			/* Hack -- we have to handle summon friends slightly differently */
			bool friend = (summon_type == SUMMON_FRIEND);

			/* Uniques have special friends */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) && (friend))
			{
				summon_type = SUMMON_UNIQUE_FRIEND;
			}

			/* Boost power of raising uniques if high level */
			if ((summon_type == RAISE_UNIQUE) && (r_ptr->flags7 & (RF7_S_HI_UNIQUE)))
			{
				summon_type = RAISE_HI_UNIQUE;
			}

			/* XXX Always disturb the player */
			disturb(1, 0);

			if ((who > SOURCE_MONSTER_START) || (who == SOURCE_PLAYER_ALLY) || (who == SOURCE_SELF))
			{
				/* Hack -- prevent summoning for a short while */
				m_ptr->summoned = 20;

				/* Set the parameters based on the summoner */
				summon_specific_params(who > SOURCE_MONSTER_START ? who : what, summon_type, FALSE);
			}
			else
			{
				/* Set the parameters later */
				summon_specific_params(0, summon_type, FALSE);
			}

			/* Count them for later */
			for (k = 0; k < num; k++)
			{
				count += summon_specific(y, x, summoner, summon_lev - (friend ? 0 : 1), summon_type, TRUE, allies);
			}
		}

		/* Blow projects something? */
		if ((method_ptr->flags1 & (PR1_PROJECT)) ||
				(method_ptr->flags2 & (PR2_PROJECT)))
		{
			int rad = scale_method(method_ptr->radius, rlev);
			int rng = scale_method(method_ptr->max_range, rlev);
			int num = scale_method(method_ptr->number, rlev);

			int degrees_of_arc = method_ptr->arc;
			int diameter_of_source = method_ptr->diameter_of_source;

			/* Hack -- scale radius up more */
			if (method_ptr->flags2 & (PR2_SCALE_RADIUS))
			{
				rad = rad - result;
			}

			/* Hack -- apply special effects */
			if (!result)
			{
				/* Increase radius */
				if (method_ptr->flags2 & (PR2_LARGE_RADIUS))
				{
					rad++;
				}

				/* Apply a massive powerful stroke */
				if (method_ptr->flags2 & (PR2_LIGHTNING_STRIKE))
				{
					dam = dam * 3 / 2;
					rad = 0;
				}

				/* Turn into a powerful arc */
				if (method_ptr->flags2 & (PR2_POWER_ARC))
				{
					flg = method_info[71].flags1;
					rad = 6;
					degrees_of_arc = 60;
					diameter_of_source = 10;
					dam = dam * 7 / 5;
				}

				/* Concentrate light or darkness */
				if (method_ptr->flags2 & (PR2_CONCENTRATE_LITE))
				{
					/* Check if we can hit target */
					if ((who <= 0) || (generic_los(y, x, m_ptr->fy, m_ptr->fx, CAVE_XLOF)))
					{
						/* Check to see if doing so would be worthwhile */
						int damage = mon_concentrate_power(y, x, spower, TRUE, FALSE, concentrate_light_hook);

						/* We decided to concentrate light */
						if (damage)
						{
							/* Fire bolt */
							dam = damage;
							rad = 0;
							flg = method_info[43].flags1;
						}
					}
				}
			}

			/* Hack -- fix number */
			if (num < 1) num = 1;

			/* Hack -- fix diameter of source */
			if (!diameter_of_source) diameter_of_source = 10;

			/* Create one or more projections */
			while (num--)
			{
				int ty = y;
				int tx = x;

				/* Pick a 'nearby' location */
				if (method_ptr->flags2 & (PR2_SCATTER)) scatter(&ty, &tx, y, x, 5, method_ptr->flags1 & (PROJECT_LOS) ? CAVE_XLOS : CAVE_XLOF);

				/* Affect distant monsters */
				if (method_ptr->flags2 & (PR2_ALL_IN_LOS | PR2_PANEL | PR2_LEVEL))
				{
					project_dist(who, what, ty, tx, dam, effect, flg, method_ptr->flags2);
				}

				/* Analyze the "dir" and the "target". */
				else
				{
					project(who, what, rad, rng, fy, fx, ty, tx, dam, effect, flg, degrees_of_arc,
							(byte)diameter_of_source);
				}
			}
		}
	}

	/* Handle special case attacks */
	if (method_ptr->flags2 & (PR2_SPECIAL_CASE))
	{
	switch (attack)
	{
		/* RF4_AURA */
		case 96+7:
		{
			/* The target (self) is attacked by a ball attack */
			mon_blow_ranged(who, attack, m_ptr->fy, m_ptr->fx, RBM_AURA, 2, FLG_MON_CLOUD);

			break;
		}

		/* RF6_HASTE */
		case 160+0:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s body.", m_name, t_poss);
			}

			if (target > 0)
			{
				/* Add to the monster haste counter */
				set_monster_haste((s16b)target, (s16b)(n_ptr->hasted + rlev + rand_int(rlev)), n_ptr->ml);
			}
			else if (target < 0)
			{
				if (p_ptr->timed[TMD_STASTIS]) /* No effect */ ;
				else if (p_ptr->timed[TMD_FAST]) inc_timed(TMD_FAST,rand_int(rlev/3), TRUE);
				else inc_timed(TMD_FAST, rlev, TRUE);
			}

			break;
		}

		/* RF6_ADD_MANA */
		case 160+1:
		{
			if (target == 0) break;

			/* Druids/shamans sometimes add mana from water */
			if ((who > 0) && ((r_ptr->flags2 & (RF2_MAGE)) != 0) && ((r_ptr->flags2 & (RF2_PRIEST)) != 0))
			{
				if (rand_int(100) < 50)
				{
					/* Check to see if doing so would be worthwhile */
					int power = mon_concentrate_power(y, x, spower, FALSE, FALSE, concentrate_water_hook);

					/* We decided to concentrate light */
					if (power)
					{
						/* Message */
						if (blind) msg_format("%^s mumbles.", m_name);
						else msg_format("%^s absorbs mana from the surrounding water.", m_name);

						/* Big boost to mana */
						n_ptr->mana = MIN(255, n_ptr->mana + power / 5 + 1);
						if (n_ptr->mana > s_ptr->mana)
							n_ptr->mana = s_ptr->mana;

						/* Done */
						break;
					}
				}

				/* Benefit less from gaining mana any other way */
				spower /= 2;
			}

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s gathers power for %s.", m_name, t_name);
			}

			if (target > 0)
			{
				/* Increase current mana.  Do not exceed maximum. */
				n_ptr->mana = MIN(255, n_ptr->mana + spower / 15 + 1);
				if (n_ptr->mana > s_ptr->mana)
					n_ptr->mana = s_ptr->mana;
			}
			else if (target < 0)
			{
				/* Player mana is worth more */
				if (p_ptr->timed[TMD_STASTIS]) /* No effect */ ;
				else if (p_ptr->csp < p_ptr->msp)
				{
					p_ptr->csp = p_ptr->csp + damroll((spower / 15) + 1,5);
					if (p_ptr->csp >= p_ptr->msp)
					{
						p_ptr->csp = p_ptr->msp;
						p_ptr->csp_frac = 0;
					}

					p_ptr->redraw |= (PR_MANA);
					p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
				}
			}
			break;
		}

		/* RF6_HEAL */
		case 160+2:
		{
			int gain, cost;

			if (target == 0) break;

			/* Sauron changes shape */
			if ((target > 0) && ((m_ptr->r_idx == SAURON_TRUE) ||
				((m_ptr->r_idx >= SAURON_FORM) && (m_ptr->r_idx < SAURON_FORM + MAX_SAURON_FORMS))))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s shifts %s shape.", m_name, t_poss);

				/* Get the new Sauron Shape */
				m_ptr->r_idx = sauron_shape(m_ptr->r_idx);

				/* We have a problem */
				if (!m_ptr->r_idx) m_ptr->r_idx = SAURON_TRUE;
			}
			/* Druids/shamans sometimes add health from trees/plants */
			if ((who > 0) && (target == who) && ((r_ptr->flags2 & (RF2_MAGE)) != 0) && ((r_ptr->flags2 & (RF2_PRIEST)) != 0) && (rand_int(100) < 50))
			{
				/* Check to see if doing so would be worthwhile */
				int power = mon_concentrate_power(y, x, spower, FALSE, FALSE, concentrate_life_hook);

				/* We decided to concentrate life */
				if (power)
				{
					/* Message */
					if (blind) msg_format("%^s mumbles.", m_name);
					else msg_format("%^s absorbs life from the surrounding plants.", m_name);

					/* Big boost to mana */
					spower = MAX(spower, power);
				}
			}
			else if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s wounds.", m_name, t_poss);
			}

			/* Heal up monsters */
			if (target > 0)
			{
				/* We regain lost hitpoints (up to spower * 3) */
				gain = MIN(n_ptr->maxhp - n_ptr->hp, spower * 3);

				/* We do not gain more than mana * 15 HPs at a time */
				gain = MIN(gain, m_ptr->mana * 15);

				/* Regain some hitpoints */
				n_ptr->hp += gain;

				/* Lose some mana (high-level monsters are more efficient) */
				cost = 1 + gain / (5 + 2 * rlev / 5);

				/* Reduce mana (do not go negative) */
				m_ptr->mana -= MIN(cost, m_ptr->mana);

				/* Fully healed */
				if (n_ptr->hp >= n_ptr->maxhp)
				{
					/* Fully healed */
					n_ptr->hp = n_ptr->maxhp;

					/* Message */
					if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
					{
						if ((!blind) && (n_ptr->ml)) msg_format("%^s looks very healthy!",  t_nref);
						else msg_format("%^s sounds very healthy!", t_nref);
					}
				}

				/* Partially healed */
				else
				{
					/* Message */
					if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
					{
						if ((!blind) && (n_ptr->ml)) msg_format("%^s looks healthier.",  t_nref);
						else msg_format("%^s sounds healthier.", t_nref);
					}
				}

				/* Redraw (later) if needed */
				if ((p_ptr->health_who == target) && (n_ptr->ml))
					p_ptr->redraw |= (PR_HEALTH);

				/* Cancel fear */
				if (n_ptr->monfear)
				{
					/* Cancel fear */
					set_monster_fear(m_ptr, 0, FALSE);

					/* Message */
					if (n_ptr->ml)
						msg_format("%^s recovers %s courage.", t_nref, t_poss);
				}

				/* Recalculate combat range later */
				m_ptr->min_range = 0;
			}

			/* Heal up player */
			else if (target < 0)
			{
				/* We regain lost hitpoints */
				if (!p_ptr->timed[TMD_STASTIS]) hp_player(damroll(spower, 8));

				/* Lose some mana (high-level monsters are more efficient) */
				cost = 1 + spower * 3 / (5 + 2 * rlev / 5);

				/* Reduce mana (do not go negetive) */
				m_ptr->mana -= MIN(cost, m_ptr->mana);
			}

			break;
		}

		/* RF6_CURE */
		case 160+3:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s ailments.", m_name, t_poss);
			}

			/* Cure a monster */
			if (target > 0)
			{
				/* Cancel stunning */
				if (n_ptr->stunned)
				{
					/* Cancel stunning */
					n_ptr->stunned = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer stunned.", t_nref);
				}

				/* Cancel fear */
				if (n_ptr->monfear)
				{
					/* Cancel fear */
					n_ptr->monfear = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s recovers %s courage.", t_nref, t_poss);
				}

				/* Cancel confusion */
				if (n_ptr->confused)
				{
					/* Cancel fear */
					n_ptr->confused = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer confused.", t_nref);
				}

				/* Cancel cuts */
				if (n_ptr->cut)
				{
					/* Cancel fear */
					n_ptr->cut = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer bleeding.", t_nref);
				}

				/* Cancel confusion */
				if (n_ptr->poisoned)
				{
					/* Cancel fear */
					n_ptr->poisoned = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer poisoned.", t_nref);
				}

				/* Cancel blindness */
				if (n_ptr->blind)
				{
					/* Cancel fear */
					n_ptr->blind = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer blind.", t_nref);
				}

				/* Cancel image */
				if (n_ptr->image)
				{
					/* Cancel image */
					n_ptr->image = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer hallucinating.", t_nref);
				}

				/* Cancel dazed */
				if (n_ptr->dazed)
				{
					/* Cancel dazed */
					n_ptr->dazed = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer dazed.", t_nref);
				}

				/* Cancel amnesia */
				if (n_ptr->amnesia)
				{
					/* Cancel dazed */
					n_ptr->amnesia = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer forgetful.", t_nref);
				}

				/* Cancel terror */
				if (n_ptr->terror)
				{
					/* Cancel dazed */
					n_ptr->terror = 0;

					/* Message */
					if (n_ptr->ml) msg_format("%^s is no longer terrified.", t_nref);
				}

				/* Redraw (later) if needed */
				if (p_ptr->health_who == who) p_ptr->redraw |= (PR_HEALTH);
			}

			/* Cure the player */
			else if (target < 0)
			{
				k = 0;

				/* Hack -- always cure paralyzation first */
				if ((p_ptr->timed[TMD_PARALYZED]) && !(p_ptr->timed[TMD_STASTIS]))
				{
					clear_timed(TMD_PARALYZED, TRUE);
					break;
				}

				/* Count ailments */
				if (p_ptr->timed[TMD_CUT]) k++;
				if (p_ptr->timed[TMD_STUN]) k++;
				if (p_ptr->timed[TMD_POISONED]) k++;
				if (p_ptr->timed[TMD_BLIND]) k++;
				if (p_ptr->timed[TMD_AFRAID]) k++;
				if (p_ptr->timed[TMD_CONFUSED]) k++;
				if (p_ptr->timed[TMD_IMAGE]) k++;
				if (p_ptr->food < PY_FOOD_WEAK) k++;

				/* High level monsters restore stats and experience if nothing worse ails the player */
				if ((rlev >= 30) && (!k))
				{
					if (p_ptr->stat_cur[A_STR] != p_ptr->stat_max[A_STR]) k++;
					if (p_ptr->stat_cur[A_INT] != p_ptr->stat_max[A_INT]) k++;
					if (p_ptr->stat_cur[A_WIS] != p_ptr->stat_max[A_WIS]) k++;
					if (p_ptr->stat_cur[A_DEX] != p_ptr->stat_max[A_DEX]) k++;
					if (p_ptr->stat_cur[A_CON] != p_ptr->stat_max[A_CON]) k++;
					if (p_ptr->stat_cur[A_CHR] != p_ptr->stat_max[A_CHR]) k++;
					if (p_ptr->stat_cur[A_AGI] != p_ptr->stat_max[A_AGI]) k++;
					if (p_ptr->stat_cur[A_SIZ] != p_ptr->stat_max[A_SIZ]) k++;
					if (p_ptr->exp < p_ptr->max_exp) k++;
				}

				/* Stastis */
				if (p_ptr->timed[TMD_STASTIS]) /* No effect */ ;

				/* Pick a random ailment */
				else if (k)
				{
					/* Check what to restore - note paranoia checks */
					if ((p_ptr->timed[TMD_CUT]) && (k) && (rand_int(k--))) { set_cut(0); set_stun(0); }
					else if ((p_ptr->timed[TMD_STUN]) && (k) && (rand_int(k--))) { set_stun(0); set_cut(0); }
					else if ((p_ptr->timed[TMD_POISONED]) && (k) && (rand_int(k--))) set_poisoned(0);
					else if ((p_ptr->timed[TMD_BLIND]) && (k) && (rand_int(k--))) clear_timed(TMD_BLIND, TRUE);
					else if ((p_ptr->timed[TMD_AFRAID]) && (k) && (rand_int(k--))) set_afraid(0);
					else if ((p_ptr->timed[TMD_CONFUSED]) && (k) && (rand_int(k--))) clear_timed(TMD_CONFUSED, TRUE);
					else if ((p_ptr->timed[TMD_IMAGE]) && (k) && (rand_int(k--))) clear_timed(TMD_IMAGE, TRUE);
					else if ((p_ptr->food < PY_FOOD_WEAK) && (k) && (rand_int(k--))) set_food(PY_FOOD_MAX -1);
					else if ((rlev < 30) || (k <= 0)) /* Nothing */ ;
					else if ((p_ptr->stat_cur[A_STR] != p_ptr->stat_max[A_STR]) && (k) && (rand_int(k--))) do_res_stat(A_STR);
					else if ((p_ptr->stat_cur[A_INT] != p_ptr->stat_max[A_INT]) && (k) && (rand_int(k--))) do_res_stat(A_INT);
					else if ((p_ptr->stat_cur[A_WIS] != p_ptr->stat_max[A_WIS]) && (k) && (rand_int(k--))) do_res_stat(A_WIS);
					else if ((p_ptr->stat_cur[A_DEX] != p_ptr->stat_max[A_DEX]) && (k) && (rand_int(k--))) do_res_stat(A_DEX);
					else if ((p_ptr->stat_cur[A_CON] != p_ptr->stat_max[A_CON]) && (k) && (rand_int(k--))) do_res_stat(A_CON);
					else if ((p_ptr->stat_cur[A_CHR] != p_ptr->stat_max[A_CHR]) && (k) && (rand_int(k--))) do_res_stat(A_CHR);
					else if ((p_ptr->stat_cur[A_AGI] != p_ptr->stat_max[A_AGI]) && (k) && (rand_int(k--))) do_res_stat(A_AGI);
					else if ((p_ptr->stat_cur[A_SIZ] != p_ptr->stat_max[A_SIZ]) && (k) && (rand_int(k--))) do_res_stat(A_SIZ);
					else if ((p_ptr->exp < p_ptr->max_exp) && (k) && (rand_int(k--))) restore_level();
				}

				/* Hack -- everything else cured, feed a hungry player */
				else if (p_ptr->food < PY_FOOD_ALERT) set_food(PY_FOOD_MAX -1);
			}

			break;
		}

		/* RF6_BLINK */
		case 160+4:
		{
			if (target > 0)
			{
				direct = !blind && player_can_see_bold(m_ptr->fy,m_ptr->fx);
				if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0)) disturb(1, 0);

				/* Get the target name (using "A"/"An") again. */
				monster_desc(t_name, sizeof(t_name), target, 0x08);

				/* Move monster (also updates "m_ptr->ml"). */
				teleport_away(target, 10);

				/*
				 * If it comes into view from around a corner (unlikely)
				 * give a message and learn about the casting
				 */
				if (!direct
					&& !blind && player_can_see_bold(m_ptr->fy, m_ptr->fx)
					&& m_ptr->ml)
				{
					msg_format("%^s blinks into view.", t_nref);
				}

				/* Normal message */
				else if (known && !m_ptr->ml)
				{
					msg_format("%^s blinks away.", t_nref);
				}

				/* Telepathic message */
				else if (known)
				{
					msg_format("%^s blinks.", t_nref);
				}

				/* Add to the "see-able-ness" after teleport*/
				seen = seen || (!blind && m_ptr->ml);
			}
			break;
		}

		/* RF6_TPORT */
		case 160+5:
		{
			if (target > 0)
			{
				direct = !blind && player_can_see_bold(m_ptr->fy,m_ptr->fx);
				if ((known)	&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0)) disturb(1, 0);

				/* Get the target name (using "A"/"An") again. */
				monster_desc(t_name, sizeof(t_name), target, 0x08);

				/* Move monster (also updates "m_ptr->ml"). */
				teleport_away(target, MAX_SIGHT * 2 + 5);

				/*
				 * If it comes into view from around a corner (unlikely)
				 * give a message and learn about the casting
				 */
				if (!direct
					&& !blind && player_can_see_bold(m_ptr->fy, m_ptr->fx)
					&& m_ptr->ml)
				{
					msg_format("%^s teleports into view.", t_nref);
				}

				/* Normal message */
				else if (known && !m_ptr->ml)
				{
					msg_format("%^s teleports away.", t_nref);
				}

				/* Telepathic message */
				else if (known)
				{
					msg_format("%^s teleports.", t_nref);
				}

				/* Add to the "see-able-ness" after teleport*/
				seen = seen || (!blind && m_ptr->ml);
			}
			break;
		}

		/* RF6_INVIS */
		case 160+6:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s appearance.", m_name, t_poss);
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->tim_invis + rlev + rand_int(rlev);

				/* Add to the monster invisibility counter */
				n_ptr->tim_invis += tmp > 255 ? 255 : (byte)tmp;

				/* Notify player */
				if (n_ptr->ml)
				{
					/* Get the target name (using "A"/"An") again. */
					monster_desc(t_name, sizeof(t_name), target, 0x08);

					update_mon(target, FALSE);

					if (!n_ptr->ml)
					{
						msg_format("%^s disppears!", t_nref);
					}
				}
			}

			break;
		}

		/* RF6_TELE_SELF_TO */
		case 160+7:
		{
			if ((who > SOURCE_MONSTER_START) || (who == SOURCE_PLAYER_ALLY))
			{
				int old_cdis = m_ptr->cdis;

				direct = !blind && player_can_see_bold(m_ptr->fy,m_ptr->fx);

				/* Move monster near player (also updates "m_ptr->ml"). */
				teleport_towards(m_ptr->fy, m_ptr->fx, y, x);

				/* Monster is now directly visible, but wasn't before. */
				if (!direct
					&& !blind && player_can_see_bold(m_ptr->fy, m_ptr->fx)
					&& m_ptr->ml)
				{
					/* Get the name (using "A"/"An") again. */
					monster_desc(m_name, sizeof(m_name), who > SOURCE_MONSTER_START ? who : what, 0x08);

					/* Message */
					msg_format("%^s suddenly appears.", m_name);
				}

				/* Monster was visible before, but isn't now. */
				else if (seen && !m_ptr->ml)
				{
					/* Message */
					msg_format("%^s blinks away.", m_name);
				}

				/* Monster is visible both before and after. */
				else if (seen && m_ptr->ml)
				{
					if (player_can_see_bold(m_ptr->fy, m_ptr->fx)
						&& (distance(m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px)
							< (old_cdis - 1)))
					{
						msg_format("%^s blinks toward you.", m_name);
					}
					else
					{
						msg_format("%^s blinks.", m_name);
					}
				}
			}

			/* Add to the "see-able-ness" after teleport*/
			seen = seen || (!blind && m_ptr->ml);

			break;
		}

		/* RF6_TELE_TO */
		case 160+8:
		{
			if ((who > SOURCE_MONSTER_START) && (target < 0))
			{
				if (!direct) break;
				disturb(1, 0);

				msg_format("%^s commands you to return.", m_name);

				if ((p_ptr->cur_flags4 & (TR4_ANCHOR)) || (room_has_flag(p_ptr->py, p_ptr->px, ROOM_ANCHOR)))
				{
					msg_print("You remain anchored in place.");
					if (!(room_has_flag(p_ptr->py, p_ptr->px, ROOM_ANCHOR))) player_can_flags(who, 0x0L, 0x0L, 0x0L, TR4_ANCHOR);
				}
				else
				{
					player_not_flags(who, 0x0L, 0x0L, 0x0L, TR4_ANCHOR);
					teleport_player_to(m_ptr->fy, m_ptr->fx);
				}
			}
			
			break;
		}

		/* RF6_TELE_AWAY */
		case 160+9:
		{
			if (target < 0)
			{
				if (!direct) break;
				disturb(1, 0);

				if ((p_ptr->cur_flags4 & (TR4_ANCHOR)) || (room_has_flag(p_ptr->py, p_ptr->px, ROOM_ANCHOR)))
				{
					msg_format("%^s fails to teleport you away.", m_name);
					if (!(room_has_flag(p_ptr->py, p_ptr->px, ROOM_ANCHOR))) player_can_flags(who, 0x0L, 0x0L, 0x0L, TR4_ANCHOR);
				}
				else
				{
					player_not_flags(who, 0x0L, 0x0L, 0x0L, TR4_ANCHOR);
					teleport_hook = NULL;
					teleport_player(100);
				}
			}
			else if (target > 0)
			{
				if (!(s_ptr->flags9 & (RF9_RES_TPORT)))
				{
					disturb(1, 0);
					msg_format("%^s teleports %s away.", m_name, t_name);
					teleport_hook = NULL;
					teleport_away(target, MAX_SIGHT * 2 + 5);
				}
			}
			break;
		}

		/* RF6_TELE_LEVEL */
		case 160+10:
		{
			if (target < 0)
			{
				if (!direct) break;
				disturb(1, 0);
				if ((blind) && (known)) msg_format("%^s mumbles strangely.", m_name);
				else if (known) msg_format("%^s gestures at your feet.", m_name);

				if ((p_ptr->cur_flags2 & (TR2_RES_NEXUS)) || (p_ptr->cur_flags4 & (TR4_ANCHOR)))
				{
					msg_print("You are unaffected!");

					/* Always notice */
					if ((p_ptr->cur_flags2 & (TR2_RES_NEXUS))) player_can_flags(who, 0x0L,TR2_RES_NEXUS,0x0L,0x0L);
					if ((p_ptr->cur_flags4 & (TR4_ANCHOR))) player_can_flags(who, 0x0L,0x0L,0x0L,TR4_ANCHOR);
				}
				else if (rand_int(100) < p_ptr->skills[SKILL_SAVE])
				{
					msg_print("You resist the effects!");

					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					teleport_player_level();

					/* Always notice */
					if (!player_not_flags(who, 0x0L,TR2_RES_NEXUS,0x0L,TR4_ANCHOR) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			break;
		}

		/* RF6_WRAITHFORM */
		case 160+11:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s body.", m_name, m_poss);
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->tim_passw + rlev + rand_int(rlev);

				/* Notify player */
				if ((n_ptr->ml) && !(n_ptr->tim_passw))
				{
					msg_format("%^s becomes more insubstantial!", t_nref);
				}

				/* Add to the monster passwall counter */
				n_ptr->tim_passw += tmp > 255 ? 255 : (byte)tmp;
			}
			else if (target < 0)
			{
				/* Nothing, yet */
			}

			break;
		}

		/* RF6_DARKNESS */
		case 160+12:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (who > SOURCE_MONSTER_START)
			{
				if ((blind) && (known)) msg_format("%^s mumbles.", m_name);
				else if (known) msg_format("%^s gestures in shadow.", m_name);
			}

			/* Hack -- Message */
			if (!((blind) && (known)) && (target < 0))
			{
				msg_print("Darkness surrounds you.");
			}

			/* Hook into the "project()" function */
			(void)project(who, what, 3, 0, y, x, y, x, 0, GF_DARK_WEAK, FLG_MON_BALL, 0, 0);

			/* Lite up the room */
			unlite_room(y, x);

			break;
		}

		/* RF6_TRAPS */
		case 160+13:
		{
			int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;

			if (!direct) break;
			if (target < 0) disturb(1, 0);

			sound(MSG_CREATE_TRAP);
			if (who > SOURCE_MONSTER_START)
			{
				if (((blind) && (known)) && (target < 0)) msg_format("%^s mumbles, and then cackles evilly.", m_name);
				else if ((target < 0) || ((target ==0) && (known))) msg_format("%^s casts a spell and cackles evilly.", m_name);
				else if (known) msg_format("%^s casts a spell at %s and cackles evilly.",m_name,t_name);
			}

			(void)project(who, what, 1, 0, y, x, y, x, 0, GF_MAKE_TRAP, flg, 0, 0);

			break;
		}

		/* RF6_FORGET */
		case 160+14:
		{
			if (!direct) break;
			if (target >=0) break;
			disturb(1, 0);
			msg_format("%^s tries to blank your mind.", m_name);

			if (p_ptr->timed[TMD_STASTIS]) /* No effect */ ;
			else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
			{
				msg_print("You resist the effects!");

				if (who > 0) update_smart_save(who, TRUE);
			}
			else
			{
				inc_timed(TMD_AMNESIA, rlev / 8 + 4 + rand_int(4), TRUE);

				if (who > 0) update_smart_save(who, FALSE);
			}
			break;
		}

		/* RF6_DRAIN_MANA */
		case 160+15:
		{
			int r1 = 0;

			if (!direct) break;
			if (target == 0) break;

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (p_ptr->csp)
				{
					/* Disturb if legal */
					disturb(1, 0);

					/* Basic message */
					msg_format("%^s draws psychic energy from you!", m_name);

					/* Attack power */
					r1 = (randint(spower) / 20) + 1;

					/* Full drain */
					if (r1 >= p_ptr->csp)
					{
						r1 = p_ptr->csp;
						p_ptr->csp = 0;
						p_ptr->csp_frac = 0;
					}

					/* Partial drain */
					else
					{
						p_ptr->csp -= r1;
					}

					/* Redraw mana */
					p_ptr->redraw |= (PR_MANA);

					/* Window stuff */
					p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
				}
				else break;
			}
			else if (target > 0)
			{
				if (n_ptr->mana > 0)
				{
					/* Basic message */
					msg_format("%^s draws psychic energy from %s.", m_name, t_name);

					r1 = (randint(spower) / 20) + 1;

					/* Full drain */
					r1 = MIN(r1, n_ptr->mana);

					n_ptr->mana -= r1;
				}
				else break;
			}

			/* Replenish monster mana */
			if ((who > SOURCE_MONSTER_START) && (m_ptr->mana < r_ptr->mana) && (r1))
			{
				if ( r1 > r_ptr->mana - m_ptr->mana)
				{
					 r1 -= r_ptr->mana - m_ptr->mana;
					 m_ptr->mana = r_ptr->mana;
				}
				else
				{
					 m_ptr->mana += r1;
					 r1 = 0;
				}
			}

			/* Heal the monster with remaining energy */
			if ((who > SOURCE_MONSTER_START) && (m_ptr->hp < m_ptr->maxhp) && (r1))
			{
				/* Heal */
				m_ptr->hp += (30 * (r1 + 1));
				if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

				/* Redraw (later) if needed */
				if (p_ptr->health_who == who) p_ptr->redraw |= (PR_HEALTH);

				/* Special message */
				if (seen)
				{
					msg_format("%^s appears healthier.", m_name);
				}
			}

			/* Inform allies of new player mana */
			if ((who > 0) && (target > 0))
			{
				if (p_ptr->csp)
				{
					update_smart_forget(who, SM_IMM_MANA);
				}
				else
				{
					update_smart_learn(who, SM_IMM_MANA);
				}
			}
			break;
		}

		/* RF6_CURSE */
		case 160+16:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if ((blind) && (known)) msg_format("%^s curses %s.", m_name, t_name);
			else msg_format("%^s points at %s and curses.", m_name, t_name);

			(void)project(who, what, 0, 0, m_ptr->fy, m_ptr->fx, y, x, get_dam(spower, attack, TRUE), GF_CURSE, FLG_MON_DIRECT, 0, 0);

			break;
		}

		/* RF6_DISPEL */
		case 160+17:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);
			if ((who > SOURCE_MONSTER_START) && (!seen))
			{
				if (target < 0) msg_print("There is a static feeling in the air.");
			}
			else if (who > SOURCE_MONSTER_START)
			{
				msg_format("%^s dispels %s magic.", m_name, t_name);
			}

			(void)project(who, what, 0, 0, m_ptr->fy, m_ptr->fx, y, x, rlev, GF_DISPEL, FLG_MON_DIRECT, 0, 0);

			break;
		}

		/* RF6_MIND_BLAST */
		case 160+18:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);
			if ((target < 0) && (!seen)) msg_print("You feel something focusing on your mind.");
			else msg_format("%^s gazes deep into %s eyes.", m_name, t_poss);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (rand_int(100) < p_ptr->skills[SKILL_SAVE])
				{
					msg_print("You resist the effects!");

					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					msg_print("Your mind is blasted by psionic energy.");
					if ((p_ptr->cur_flags2 & (TR2_RES_CONFU)) == 0)
					{
						inc_timed(TMD_CONFUSED, rand_int(4) + 4, TRUE);
						if (!player_not_flags(who, 0x0L, TR2_RES_CONFU, 0x0L, 0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						if (!player_can_flags(who, 0x0L, TR2_RES_CONFU, 0x0L, 0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}

					/* Apply damage directly */
					project_one(who, what, y, x, get_dam(spower, attack, TRUE), GF_HURT, (PROJECT_PLAY | PROJECT_HIDE));
				}
			}
			else if (target > 0)
			{
				if (!(r_ptr->flags2 & (RF2_EMPTY_MIND)))
				{
					if (known) msg_format ("%^s mind is blasted by psionic energy.",t_poss);

					/* Hack --- Use GF_CONFUSION */
					project_one(who, what, y, x, get_dam(spower, attack, TRUE), GF_CONFUSION, (PROJECT_KILL));
				}
				else if (n_ptr->ml)
				{
					k_ptr->flags2 |= (RF2_EMPTY_MIND);
				}
			}
			break;
		}

		/* RF6_ILLUSION */
		case 160+19:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (known) sound(MSG_CAST_FEAR);
			if (who > SOURCE_MONSTER_START)
			{
				if (((blind) && (known)) && (target < 0)) msg_format("%^s mumbles, and you hear deceptive noises.", m_name);
				else if ((blind) && (known)) msg_format("%^s mumbles.",m_name);
				else if ((target < 0) || ((target ==0) && (known))) msg_format("%^s casts a deceptive illusion.", m_name);
				else if (known) msg_format("%^s casts a deceptive illusion at %s.",m_name,t_name);
			}

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (p_ptr->cur_flags2 & (TR2_RES_CHAOS))
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						inc_timed(TMD_IMAGE, rand_int(4) + 1, TRUE);
					}
					else msg_print("You refuse to be deceived.");

					/* Sometimes notice */
					player_can_flags(who, 0x0L,TR2_RES_CHAOS,0x0L,0x0L);
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You refuse to be deceived.");

					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					inc_timed(TMD_IMAGE, rand_int(4) + 4, TRUE);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,TR2_RES_CHAOS,0x0L,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_HALLU */
				project_one(who, what, y, x, rlev, GF_HALLU, (PROJECT_KILL));
			}
			break;
		}

		/* RF6_WOUND */
		case 160+20:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (spower < 4)
			{
				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s points at %s and curses.", m_name, t_name);
				k = 1;
			}
			else if (spower < 10)
			{
				if (blind) msg_format("%^s mumbles deeply.", m_name);
				else msg_format("%^s points at %s and curses horribly.", m_name, t_name);
				k = 2;
			}
			else if (spower < 20)
			{
				if (blind) msg_format("%^s murmurs loudly.", m_name);
				else msg_format("%^s points at %s, incanting terribly.", m_name, t_name);
				k = 3;
			}
			else if (spower < 35)
			{
				if (blind) msg_format("%^s cries out wrathfully.", m_name);
				else msg_format("%^s points at %s, screaming words of peril!", m_name, t_name);
				k = 4;
			}
			else
			{
				if (blind) msg_format("%^s screams the word 'DIE!'", m_name);
				else msg_format("%^s points at %s, screaming the word DIE!", m_name, t_name);
				k = 5;
			}

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (rand_int(rlev / 2 + 70) < p_ptr->skills[SKILL_SAVE])
				{
					msg_format("You resist the effects%c",
					      (spower < 30 ?  '.' : '!'));

					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					/*
					 * Inflict damage. Note this spell has a hack
					 * that handles damage differently in get_dam.
					 */
					project_one(who, what, y, x, get_dam(spower, attack, TRUE), GF_HURT, (PROJECT_PLAY | PROJECT_HIDE));

					/* Cut the player depending on strength of spell. */
					if (k == 1) (void)set_cut(p_ptr->timed[TMD_CUT] + 8 + damroll(2, 4));
					if (k == 2) (void)set_cut(p_ptr->timed[TMD_CUT] + 23 + damroll(3, 8));
					if (k == 3) (void)set_cut(p_ptr->timed[TMD_CUT] + 46 + damroll(4, 12));
					if (k == 4) (void)set_cut(p_ptr->timed[TMD_CUT] + 95 + damroll(8, 15));
					if (k == 5) (void)set_cut(1200);

					if (who > 0) update_smart_save(who, FALSE);
				}
			}
			else if (target > 0)
			{
				int cut = 0;

				/* Cut the monster depending on strength of spell. */
				if (k == 1) cut = 8 + damroll(2, 4);
				if (k == 2) cut = 23 + damroll(3, 8);
				if (k == 3) cut = 46 + damroll(4, 12);
				if (k == 4) cut = 95 + damroll(8, 15);
				if (k == 5) cut = 1200;

				/* Adjust for monster level */
				cut /= r_ptr->level / 10 + 1;

				if (who < 0)
				{
					/* Hack --- Use GF_DRAIN_LIFE */
					project_one(who, what, y, x, damroll(8,8), GF_DRAIN_LIFE, (PROJECT_KILL));

					/* Hack -- player can cut monsters */
					if ((s_ptr->flags3 & (RF3_NONLIVING)) == 0) n_ptr->cut = MIN(255, n_ptr->cut + cut);
				}
				else
				{
					/* Hack -- monsters only do damage, not cuts, to each other */
					project_one(who, what, y, x, damroll(8,8) + cut, GF_DRAIN_LIFE, (PROJECT_KILL));
				}
			}

			break;
		}

		/* RF6_BLESS */
		case 160+21:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind)
				{
					msg_format("%^s mumbles.", m_name);
				}
				else if (who != SOURCE_SELF)
				{
					msg_format("%^s blesses %s.", m_name, t_name);
				}
				else
				{
					msg_format("%^s invokes %s diety.", m_name, m_poss);
				}
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->bless + rlev + rand_int(rlev);

				/* Notify player */
				if ((n_ptr->ml) && !(n_ptr->bless))
				{
					msg_format("%^s appears righteous!", t_nref);
				}

				/* Add to the monster bless counter */
				n_ptr->bless += tmp > 255 ? 255 : (byte)tmp;
			}
			else if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				/* Set blessing */
				inc_timed(TMD_BLESSED, rlev, TRUE);
			}

			break;
		}

		/* RF6_BERSERK */
		case 160+22:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s whips %s up into a frenzy.", m_name, t_name);
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->berserk + rlev + rand_int(rlev);

				/* Notify player */
				if ((n_ptr->ml) && !(n_ptr->berserk))
				{
					msg_format("%^s goes berserk!", t_nref);
				}

				/* Add to the monster berserk counter */
				n_ptr->berserk += tmp > 255 ? 255 : (byte)tmp;
			}
			else if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				/* Set blessing */
				inc_timed(TMD_BERSERK, rlev, TRUE);
			}

			break;
		}

		/* RF6_SHIELD */
		case 160+23:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on the air around %s.", m_name, t_name);
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->shield + rlev + rand_int(rlev);

				/* Notify player */
				if ((n_ptr->ml) && !(n_ptr->shield))
				{
					msg_format("%^s becomes magically shielded.", t_nref);
				}

				/* Add to the monster shield counter */
				n_ptr->shield += tmp > 255 ? 255 : (byte)tmp;
			}
			else if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				/* Set blessing */
				inc_timed(TMD_SHIELD, rlev, TRUE);
			}

			break;
		}

		/* RF6_OPPOSE_ELEM */
		case 160+24:
		{
			if (target == 0) break;

			if ((known) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				disturb(1,0);

				if (blind) msg_format("%^s mumbles.", m_name);
				else msg_format("%^s concentrates on %s body.", m_name, t_poss);
			}

			if (target > 0)
			{
				/* Paranoia: Prevent overflow */
				int tmp = n_ptr->oppose_elem + rlev + rand_int(rlev);

				/* Notify player */
				if ((n_ptr->ml) && !(n_ptr->oppose_elem))
				{
					msg_format("%^s becomes temporarily resistant to the elements.", t_nref);
				}

				/* Add to the monster elements counter */
				n_ptr->oppose_elem += tmp > 255 ? 255 : (byte)tmp;
			}
			else if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				/* Set opposition */
				inc_timed(TMD_OPP_ACID, rlev, FALSE);
				inc_timed(TMD_OPP_COLD, rlev, FALSE);
				inc_timed(TMD_OPP_ELEC, rlev, FALSE);
				inc_timed(TMD_OPP_FIRE, rlev, FALSE);
				inc_timed(TMD_OPP_POIS, rlev, FALSE);
			}

			break;
		}

		/* RF6_HUNGER */
		case 160+25:
		{
			if (!direct) break;
			if (target == 0) break;

			if (blind) msg_format("%^s %s commanded to feel hungry.", t_nref, target < 0 ? "are" : "is");
			else msg_format("%^s gestures at %s, and commands that %s feel%s hungry.", m_name, t_name, t_name, target < 0 ? "" : "s");

			if (target < 0)
			{
				if (p_ptr->timed[TMD_STASTIS]) /* No effect */;
				else if (rand_int(rlev / 2 + 70) > p_ptr->skills[SKILL_SAVE])
				{
					/* Reduce food abruptly.  */
					(void)set_food(p_ptr->food - (p_ptr->food/4));

					if (who > 0) update_smart_save(who, FALSE);
				}
				else
				{
					msg_print ("You resist the effects!");

					if (who > 0) update_smart_save(who, TRUE);
				}
			}
			else if ((target > 0) && ((r_info[n_ptr->r_idx].flags3 & (RF3_NONLIVING)) == 0))
			{
				/* Hack -- use strength to reflect hunger */
				if (n_ptr->mflag & (MFLAG_STRONG)) m_ptr->mflag &= ~(MFLAG_STRONG);
				else if (m_ptr->mflag & (MFLAG_WEAK))
				{
					/* Hack --- Use GF_HURT */
					project_one(who, what, y, x, get_dam(spower, attack, TRUE), GF_HURT, (PROJECT_KILL));
				}
				else m_ptr->mflag |= (MFLAG_WEAK);
			}

			break;
		}

		/* RF6_PROBE */
		case 160+26:
		{
			if (!direct) break;
			if ((who <= 0) || (target >= 0)) break;

			msg_format("%^s probes your weaknesses.", m_name);

			if (who > SOURCE_MONSTER_START)
			{
				update_smart_cheat(who);
			}

			break;
		}

		/* RF6_SCARE */
		case 160+27:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (known) sound(MSG_CAST_FEAR);
			if (((blind) && (known)) && (target < 0)) msg_format("%^s mumbles, and you hear scary noises.", m_name);
			else if ((blind) && (known)) msg_format("%^s mumbles.",m_name);
			else if ((target < 0) || ((target ==0) && (known))) msg_format("%^s casts a fearful illusion.", m_name);
			else if (known) msg_format("%^s casts a fearful illusion at %s.",m_name,t_name);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if ((p_ptr->cur_flags2 & (TR2_RES_FEAR)) != 0)
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						(void)set_afraid(p_ptr->timed[TMD_AFRAID] + rand_int(4) + 1);

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_FEAR,0x0L,0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						msg_print("You refuse to be frightened.");

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_FEAR,0x0L,0x0L) && who)
						{
							update_smart_save(who, TRUE);
						}
					}
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You refuse to be frightened.");
					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					(void)set_afraid(p_ptr->timed[TMD_AFRAID] + rand_int(4) + 4);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,TR2_RES_FEAR,0x0L,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_TERRIFY */
				project_one(who, what, y, x, rlev, GF_TERRIFY, (PROJECT_KILL));
			}
			break;
		}

		/* RF6_BLIND */
		case 160+28:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if ((blind) && (known)) msg_format("%^s mumbles.", m_name);
			else if (known) msg_format("%^s casts a spell, burning %s eyes.", m_name, t_poss);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if ((p_ptr->cur_flags2 & (TR2_RES_BLIND)) != 0)
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						inc_timed(TMD_BLIND, rand_int(6) + 1, TRUE);

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_BLIND,0x0L,0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						msg_print("You are unaffected!");

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_BLIND,0x0L,0x0L) && who)
						{
							update_smart_save(who, TRUE);
						}
					}
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You resist the effects!");

					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					inc_timed(TMD_BLIND, 12 + rlev / 4 + rand_int(4), TRUE);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,TR2_RES_BLIND,0x0L,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_CONF_WEAK */
				project_one(who, what, y, x, rlev, GF_CONF_WEAK, (PROJECT_KILL));
			}
			break;
		}

		/* RF6_CONF */
		case 160+29:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (((blind) && (known)) && (target < 0)) msg_format("%^s mumbles, and you hear puzzling noises.", m_name);
			else if ((blind) && (known)) msg_format ("%^s mumbles.",m_name);
			else if ((target < 0) || ((target == 0) && (known))) msg_format("%^s casts a mesmerising illusion.", m_name);
			else if (known) msg_format("%^s creates a mesmerising illusion for %s.", m_name, t_name);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if ((p_ptr->cur_flags2 & (TR2_RES_CONFU)) != 0)
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						inc_timed(TMD_CONFUSED, rand_int(5) + 1, 1);

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_CONFU,0x0L,0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						msg_print("You disbelieve the feeble spell.");

						/* Always notice */
						if (!player_can_flags(who, 0x0L,TR2_RES_CONFU,0x0L,0x0L) && who)
						{
							update_smart_save(who, TRUE);
						}
					}
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You disbelieve the feeble spell.");
					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					inc_timed(TMD_CONFUSED, rlev / 8 + 4 + rand_int(4), TRUE);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,TR2_RES_CONFU,0x0L,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_CONF_WEAK */
				project_one(who, what, y, x, rlev, GF_CONF_WEAK, (PROJECT_KILL));
			}
			break;
		}

		/* RF6_SLOW */
		case 160+30:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if (((blind) && (known)) && (target < 0)) msg_format("%^s drains power from your muscles.", m_name);
			else if ((blind) && (known)) msg_format ("%^s mumbles.",m_name);
			else if (known) msg_format("%^s drains power from %s muscles.", m_name, t_poss);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (((p_ptr->cur_flags3 & (TR3_FREE_ACT)) != 0) || (p_ptr->timed[TMD_FREE_ACT]))
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						inc_timed(TMD_SLOW, rand_int(3) + 1, TRUE);

						/* Player is temporarily resistant */
						if (p_ptr->timed[TMD_FREE_ACT])
						{
							update_smart_save(who, FALSE);
						}
						/* Always notice */
						else if (!player_can_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						msg_print("You are unaffected!");

						/* Player is temporarily resistant */
						if (p_ptr->timed[TMD_FREE_ACT])
						{
							update_smart_save(who, TRUE);
						}
						/* Always notice */
						else if (!player_can_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
						{
							update_smart_save(who, TRUE);
						}
					}
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You resist the effects!");
					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					inc_timed(TMD_SLOW, rand_int(4) + 4 + rlev / 25, TRUE);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_SLOW_WEAK */
				project_one(who, what, y, x, rlev, GF_SLOW_WEAK, (PROJECT_KILL));
			}
			break;
		}

		/* RF6_HOLD */
		case 160+31:
		{
			if (!direct) break;
			if (target < 0) disturb(1, 0);

			if ((blind) && (known)) msg_format ("%^s mumbles.",m_name);
			else if (known) msg_format("%^s stares deeply into %s muscles.", m_name, t_poss);

			if ((target < 0) && !(p_ptr->timed[TMD_STASTIS]))
			{
				if (((p_ptr->cur_flags3 & (TR3_FREE_ACT)) != 0) || (p_ptr->timed[TMD_FREE_ACT]))
				{
					if (powerful && (rand_int(100) > p_ptr->skills[SKILL_SAVE]))
					{
						msg_format("%^s power overcomes your resistance.", m_poss);

						if (!p_ptr->timed[TMD_SLOW]) inc_timed(TMD_SLOW, rand_int(4) + 1, 1);
						else inc_timed(TMD_PARALYZED, 1, TRUE);

						/* Player is temporarily resistant */
						if (p_ptr->timed[TMD_FREE_ACT])
						{
							update_smart_save(who, FALSE);
						}
						/* Always notice */
						else if (!player_can_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
						{
							update_smart_save(who, FALSE);
						}
					}
					else
					{
						msg_print("You are unaffected!");

						/* Player is temporarily resistant */
						if (p_ptr->timed[TMD_FREE_ACT])
						{
							update_smart_save(who, TRUE);
						}
						/* Always notice */
						else if (!player_can_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
						{
							update_smart_save(who, TRUE);
						}
					}
				}
				else if (rand_int(100) < (powerful ? p_ptr->skills[SKILL_SAVE] * 2 / 3 : p_ptr->skills[SKILL_SAVE]))
				{
					msg_print("You resist the effects!");
					if (who > 0) update_smart_save(who, TRUE);
				}
				else
				{
					inc_timed(TMD_PARALYZED, rand_int(4) + 4, TRUE);

					/* Always notice */
					if (!player_not_flags(who, 0x0L,0x0L,TR3_FREE_ACT,0x0L) && who)
					{
						update_smart_save(who, FALSE);
					}
				}
			}
			else if (target > 0)
			{
				/* Hack --- Use GF_SLEEP */
				project_one(who, what, y, x, rlev, GF_SLEEP, (PROJECT_KILL));
			}
			break;
		}

		/* Paranoia */
		default:
		{
			if ((who > SOURCE_MONSTER_START) || (who == SOURCE_PLAYER_ALLY) || (who == SOURCE_SELF))
				msg_print("A monster tried to cast a spell that has not yet been defined.");
			else
				msg_print("Something tried to cast a spell that has not yet been defined.");
		}
	}
	}

	/* Restore cave ecology */
	cave_ecology.ready = old_cave_ecology;

	/* Hack - Inform a blind player about monsters appearing nearby */
	if (blind && count && (target < 0))
	{
		if (count == 1)
		{
			msg_print("You hear something appear nearby.");
		}
		else if (count < 4)
		{
			msg_print("You hear several things appear nearby.");
		}
		else
		{
			msg_print("You hear many things appear nearby.");
		}
	}

	/* Monster updates */
	if (who >= SOURCE_MONSTER_START)
	{
		/* Mark minimum desired range for recalculation */
		m_ptr->min_range = 0;

		/* Remember what the monster did to us */
		if (seen)
		{
			/* Innate spell */
			if (attack < 32*4)
			{
				l_ptr->flags4 |= (1L << (attack - 32*3));
				if (l_ptr->cast_innate < MAX_UCHAR) l_ptr->cast_innate++;

				/* Hack -- always notice if powerful */
				if (r_ptr->flags2 & (RF2_POWERFUL)) l_ptr->flags2 |= (RF2_POWERFUL);

				/* Hack -- blows */
				if (attack - 32*3 < 4)
				{
					/* Count attacks of this type */
					if (l_ptr->blows[attack - 32*3] < MAX_UCHAR)
					{
						l_ptr->blows[attack - 32*3]++;
					}
				}
			}

			/* Bolt or Ball */
			else if (attack < 32*5)
			{
				l_ptr->flags5 |= (1L << (attack - 32*4));
				if (l_ptr->cast_spell < MAX_UCHAR) l_ptr->cast_spell++;
			}

			/* Special spell */
			else if (attack < 32*6)
			{
				l_ptr->flags6 |= (1L << (attack - 32*5));
				if (l_ptr->cast_spell < MAX_UCHAR) l_ptr->cast_spell++;

				/* Hack -- always notice if powerful */
				if (r_ptr->flags2 & (RF2_POWERFUL)) l_ptr->flags2 |= (RF2_POWERFUL);
			}

			/* Summon spell */
			else if (attack < 32*7)
			{
				l_ptr->flags7 |= (1L << (attack - 32*6));
				if (l_ptr->cast_spell < MAX_UCHAR) l_ptr->cast_spell++;
			}
		}

		if (seen && p_ptr->wizard)
			msg_format("%^s has %i mana remaining.", m_name, m_ptr->mana);

		/* Always take note of monsters that kill you */
		if (p_ptr->is_dead && (l_ptr->deaths < MAX_SHORT))
		{
			l_ptr->deaths++;
		}
	}

	/* A spell was cast */
 	return (TRUE);
}


/*
 * Determine if monster resists a blow.  -LM-
 *
 * Return TRUE if evasion was successful.
 */
bool mon_evade(int m_idx, int chance, int out_of, cptr r)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	char m_name[80];

	cptr p;

	int roll;

	if ((r_ptr->flags9 & (RF9_EVASIVE)) == 0) return (FALSE);

	roll = rand_int(out_of);

	/* Get "the monster" or "it" */
	monster_desc(m_name, sizeof(m_name), m_idx, 0x40);

	switch(roll % 4)
	{
		case 0: p = "dodges"; break;
		case 1: p = "evades"; break;
		case 2: p = "side steps"; break;
		default: p = "ducks"; break;
	}

	/* Hack -- evasive monsters may ignore trap */
	if ((!m_ptr->blind) && (!m_ptr->csleep)
		&& (roll < chance))
	{
		if (m_ptr->ml)
		{
			/* Message */
			message_format(MSG_MISS, 0, "%^s %s%s!", m_name, p, r);

			/* Note that monster is evasive */
			l_ptr->flags9 |= (RF9_EVASIVE);
		}
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Determine if monster resists a blow.  -LM-
 *
 * Return TRUE if blow was avoided.
 */
bool mon_resist_object(int m_idx, const object_type *o_ptr)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int resist = 0;
	bool learn = FALSE;

	cptr note = "";

	char m_name[80];
	char o_name[80];

	/* Get "the monster" or "it" */
	monster_desc(m_name, sizeof(m_name), m_idx, 0x40);

	/* Describe object */
	if (o_ptr->k_idx) object_desc(o_name, sizeof(o_name), o_ptr, FALSE, 0);
	else (my_strcpy(o_name,"attack", sizeof(o_name)));

	/*
	 * Handle monsters that resist blunt and/or edged weapons. We include
	 * martial arts as a blunt attack, as well as any unusual thrown objects.
	 * We include resist magic for instances where we try to attack monsters
	 * with spell based weapons.
	 */
	switch (o_ptr->tval)
	{
		case TV_SPELL:
		{
			/* Resist */
			if ((r_ptr->flags9 & (RF9_RES_MAGIC)) != 0)
			{
				resist = 60;

				if (((l_ptr->flags9 & (RF9_RES_MAGIC)) == 0) &&
					(m_ptr->ml))
				{
					l_ptr->flags9 |= (RF9_RES_MAGIC);
					learn = TRUE;
				}
			}

			/* Take note */
			note = "glances off of";
			break;
		}

		case TV_ARROW:
		case TV_BOLT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_SPIKE:
		{
			/* Immunity */
			if ((r_ptr->flags9 & (RF9_IM_EDGED)) != 0)
			{
				/* Resist */
				resist = 85;

				/* Learn */
				if (((l_ptr->flags9 & (RF9_IM_EDGED)) == 0) &&
					(m_ptr->ml))
				{
					l_ptr->flags9 |= (RF9_IM_EDGED);
					learn = TRUE;
				}
			}
			/* Resist */
			else if ((r_ptr->flags9 & (RF9_RES_EDGED)) != 0)
			{
				resist = 60;

				if (((l_ptr->flags9 & (RF9_RES_EDGED)) == 0) &&
					(m_ptr->ml))
				{
					l_ptr->flags9 |= (RF9_RES_EDGED);
					learn = TRUE;
				}
			}

			/* Take note */
			note = "glances off of";
			break;
		}

		default:
		{
			/* Immunity */
			if ((r_ptr->flags9 & (RF9_IM_BLUNT)) != 0)
			{
				resist = 85;

				if (((l_ptr->flags9 & (RF9_IM_BLUNT)) == 0) &&
					(m_ptr->ml))
				{
					l_ptr->flags9 |= (RF9_IM_BLUNT);
					learn = TRUE;
				}
			}
			/* Resist */
			else if ((r_ptr->flags9 & (RF9_RES_BLUNT)) != 0)
			{
				resist = 60;

				if (((l_ptr->flags9 & (RF9_RES_BLUNT)) == 0) &&
					(m_ptr->ml))
				{
					l_ptr->flags9 |= (RF9_RES_BLUNT);
					learn = TRUE;
				}
			}

			/* Take note */
			if (strchr("GvE", r_ptr->d_char))
				note = "passes harmlessly through";
			else
				note = "bounces off of";

			break;
		}
	}

	/* Hack -- more accurate weapons reduce resistance */
	resist -= o_ptr->to_h;

	/* Try for a miss */
	if (resist > rand_int(100))
	{
		/* Monster is fully visible */
		if (m_ptr->ml)
		{
			/* Take note of new resist */
			if (learn)
			{
				if (resist >= 80)
					msg_format("Your %s does almost no damage to %s!",
						o_name, m_name);
				else if (resist >= 70)
					msg_format("Your %s does very little damage to %s!",
						o_name, m_name);
				else if (resist >= 50)
					msg_format("Your %s does little damage to %s!",
						o_name, m_name);
				else
					msg_format("Your %s is resisted by %s.",
						o_name, m_name);
			}

			/* Note already known resistance */
			else
			{
				msg_format("Your %s %s %s.", o_name, note, m_name);
			}
		}

		/* Can't hurt me! */
		return (TRUE);
	}

	/* Can hurt me */
	return (FALSE);
}


/*
 *  Check if race avoids the trap
 */
bool race_avoid_trap(int r_idx, int y, int x, int feat)
{
	feature_type *f_ptr;
	monster_race *r_ptr = &r_info[r_idx];

	/* Get feature */
	f_ptr = &f_info[feat];

	/* Hack - test whether we hit trap based on the attr of the trap */
	switch (f_ptr->d_attr)
	{
		/* Trap door */
		case TERM_WHITE:
		/* Pit */
		case TERM_SLATE:
		{
			/* Avoid by flying/climbing */
			if (r_ptr->flags2 & (RF2_MUST_FLY | RF2_CAN_FLY | RF2_CAN_CLIMB)) return (TRUE);
			break;
		}
		/* Strange rune */
		case TERM_ORANGE:
		{
			/* Avoid by being strange */
			if (r_ptr->flags2 & (RF2_WEIRD_MIND)) return (TRUE);
			break;
		}
		/* Spring loaded trap */
		case TERM_RED:
		{
			/* Avoid by being light footed */
			if (r_ptr->flags9 & (RF9_EVASIVE)) return (TRUE);
			break;
		}
		/* Gas trap */
		case TERM_GREEN:
		{
			/* Avoid by holding breath */
			break;
		}
		/* Explosive trap */
		case TERM_BLUE:
		{
			/* Avoid by XXXX */
			break;
		}
		/* Discoloured spot */
		case TERM_UMBER:
		{
			/* Avoid by XXXX */
			break;
		}
		/* Silent watcher */
		case TERM_L_DARK:
		{
			/* Avoid by being invisible */
			if (r_ptr->flags2 & (RF2_INVISIBLE)) return (TRUE);
			break;
		}
		/* Strange visage */
		case TERM_L_WHITE:
		{
			/* Avoid by being dragonish */
			if (r_ptr->flags3 & (RF3_DRAGON)) return (TRUE);
			break;
		}
		/* Loose rock or multiple traps */
		case TERM_L_PURPLE:
		{
			/* Avoid by sneaking */
			if (r_ptr->flags1 & (RF2_SNEAKY)) return (TRUE);
			break;
		}
		/* Trip wire */
		case TERM_YELLOW:
		{
			/* Avoid by searching */
			/*if (m_ptr->m_flag & (MFLAG_SMART)) return (TRUE);*/
			break;
		}
		/* Murder hole */
		case TERM_L_RED:
		{
			/* Avoid if armoured */
			if (r_ptr->flags2 & (RF2_ARMOR)) return (TRUE);
			break;
		}
		/* Ancient hex */
		case TERM_L_GREEN:
		{
			/* Avoid by being undead */
			if (r_ptr->flags3 & (RF3_UNDEAD)) return (TRUE);
			break;
		}
		/* Magic symbol */
		case TERM_L_BLUE:
		{
			/* Avoid by not having much mana */
			if (!r_ptr->mana) return (TRUE);
			break;
		}
		/* Fine net */
		case TERM_L_UMBER:
		{
			/* Avoid unless flying */
			if ((r_ptr->flags2 & (RF2_MUST_FLY | RF2_CAN_FLY)) == 0) return (TRUE);
			break;
		}
		/* Surreal painting */
		case TERM_PURPLE:
		{
			/* Avoid by being able to see the painting */
			/*if (!m_ptr->blind) return (TRUE); */
			break;
		}
		/* Ever burning eye */
		case TERM_VIOLET:
		{
			/* Avoid by being cold blooded */
			break;
		}
		/* Upwards draft */
		case TERM_TEAL:
		{
			/* Avoid by being heavy */
			if (r_ptr->flags3 & (RF3_HUGE)) return (TRUE);
			break;
		}
		/* Dead fall */
		case TERM_MUD:
		{
			/* Avoid by XXXX */
			break;
		}
		/* Shaft of light */
		case TERM_L_YELLOW:
		{
			/* Avoid by being unlit */
			if ((cave_info[y][x] & (CAVE_GLOW)) == 0) return (TRUE);
			break;
		}
		/* Glowing glyph */
		case TERM_MAGENTA:
		{
			/* Avoid by being blind or unable to read */
			if (r_ptr->flags2 & (RF2_STUPID)) return (TRUE);
			break;
		}
		/* Demonic sign */
		case TERM_L_TEAL:
		{
			/* Avoid by being demonic */
			if (r_ptr->flags3 & (RF3_DEMON)) return (TRUE);
			break;
		}
		/* Radagast's snare */
		case TERM_L_VIOLET:
		{
			/* Avoid by being animal */
			if (r_ptr->flags3 & (RF3_ANIMAL | RF3_INSECT | RF3_PLANT)) return (TRUE);
			break;
		}
		/* Siege engine */
		case TERM_L_PINK:
		/* Clockwork mechanism */
		case TERM_MUSTARD:
		{
			/* Always avoid */
			return (TRUE);
			break;
		}
		/* Mark of a white hand */
		case TERM_BLUE_SLATE:
		{
			/* Avoid by being orc */
			if (r_ptr->flags3 & (RF3_ORC)) return (TRUE);
			break;
		}
		/* Shimmering portal */
		case TERM_DEEP_L_BLUE:
		{
			/* Avoid by being anti-teleport */
			if (r_ptr->flags9 & (RF9_RES_TPORT)) return (TRUE);
			break;
		}
	}

	/* Can't avoid trap */
	return (FALSE);
}



/*
 *  Check if a monster avoids the trap
 *
 *  Note we only apply specific monster checks. All racial checks
 *  are done above.
 */
bool mon_avoid_trap(monster_type *m_ptr, int y, int x, int feat)
{
	feature_type *f_ptr;

	/* Get feature */
	f_ptr = &f_info[feat];

	/* Hack - test whether we hit trap based on the attr of the trap */
	switch (f_ptr->d_attr)
	{
		/* Strange rune */
		case TERM_ORANGE:
		{
			/* Avoid by being strange */
			if (m_ptr->confused) return (TRUE);
			break;
		}
		/* Silent watcher */
		case TERM_L_DARK:
		{
			/* Avoid by being invisible */
			if (m_ptr->tim_invis) return (TRUE);
			break;
		}
		/* Trip wire */
		case TERM_YELLOW:
		{
			/* Avoid by searching */
			if (m_ptr->mflag & (MFLAG_SMART)) return (TRUE);
			break;
		}
		/* Magic symbol */
		case TERM_L_BLUE:
		{
			/* Avoid by not having much mana */
			if (m_ptr->mana < r_info[m_ptr->r_idx].mana / 5) return (TRUE);
			break;
		}
		/* Surreal painting */
		case TERM_PURPLE:
		{
			/* Avoid by being able to see the painting */
			if (!m_ptr->blind) return (TRUE);
			break;
		}
		/* Glowing glyph */
		case TERM_MAGENTA:
		{
			/* Avoid by being blind or unable to read */
			if (m_ptr->blind) return (TRUE);
			break;
		}
	}

	/* Avoid trap racially */
	return (race_avoid_trap(m_ptr->r_idx, y, x, feat));
}




/*
 * Handle monster hitting a real trap.
 * TODO: join with other (monster?) attack routines
 */
void mon_hit_trap(int m_idx, int y, int x)
{
	feature_type *f_ptr;
	monster_type *m_ptr = &m_list[m_idx];

	/* Hack --- don't activate unknown invisible traps */
	if (cave_feat[y][x] == FEAT_INVIS) return;

	/* Get feature */
	f_ptr = &f_info[cave_feat[y][x]];

	/* Avoid trap */
	if ((f_ptr->flags1 & (FF1_TRAP)) && (mon_avoid_trap(m_ptr, y, x, cave_feat[y][x]))) return;

	/* Hack -- monster falls onto trap */
	if ((m_ptr->fy!=y)|| (m_ptr->fx !=x))
	{
		/* Move monster */
		monster_swap(m_ptr->fy, m_ptr->fx, y, x);
	}

	/* Hack -- evasive monsters may ignore trap */
	if (mon_evade(m_idx, (m_ptr->stunned || m_ptr->confused) ? 50 : 80, 100, " a trap"))
	{
		if (f_ptr->flags1 & (FF1_SECRET))
		{
			/* Discover */
			cave_alter_feat(y,x,FS_SECRET);
		}

		return;
	}

	/* Apply the object */
	else
	{
		discharge_trap(y, x, y, x, 0);

		/* XXX Monster is no longer stupid */
		m_ptr->mflag &= ~(MFLAG_STUPID);
	}
}




#if 0

/*
 * The running algorithm  -CJS-
 *
 * Modified for monsters. This lets them navigate corridors 'quickly'
 * and correctly turn corners. We use the monster run algorithm to
 * move now.
 *
 * We should be careful not to to let a monster run past the player
 * unless they are fleeing.
 *
 * We should be careful to 'disturb' the monster if they are in a room
 * and the player becomes visible or moves while visible.
 *
 * Because monsters are effectively running, we should be careful to
 * update the run parameters if they are 'staggering', and not let
 * them run while confused.
 *
 */




/*
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] =
{ 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };



/*
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. ???
 *
 * Diagonal Corridor    Blunt Corridor (?)
 *       # #		  #
 *       #x#		 @x#
 *       @p.		  p
 */
static void mon_run_init(int dir)
{
	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int i, row, col;

	bool deepleft, deepright;
	bool shortleft, shortright;

	/* Save the direction */
	m_ptr->run_cur_dir = dir;

	/* Assume running straight */
	m_ptr->run_old_dir = dir;

	/* Assume looking for open area */
	m_ptr->mflag |= MFLAG_RUN_OPEN_AREA;

	/* Assume not looking for breaks */
	m_ptr->mflag &= ~(MFLAG_RUN_BREAK_RIGHT);
	m_ptr->mflag &= ~(MFLAG_RUN_BREAK_LEFT);

	/* Assume no nearby walls */
	deepleft = deepright = FALSE;
	shortright = shortleft = FALSE;

	/* Find the destination grid */
	row = fy + ddy[dir];
	col = fx + ddx[dir];

	/* Extract cycle index */
	i = chome[dir];

	/* Check for nearby wall */
	if (see_wall(cycle[i+1], fy, fx))
	{
		m_ptr->mflag |= MFLAG_RUN_BREAK_LEFT;
		shortleft = TRUE;
	}

	/* Check for distant wall */
	else if (see_wall(cycle[i+1], row, col))
	{
		m_ptr->mflag |= MFLAG_RUN_BREAK_LEFT;
		deepleft = TRUE;
	}

	/* Check for nearby wall */
	if (see_wall(cycle[i-1], fy, fx))
	{
		m_ptr->mflag |= MFLAG_RUN_BREAK_RIGHT;
		shortright = TRUE;
	}

	/* Check for distant wall */
	else if (see_wall(cycle[i-1], row, col))
	{
		m_ptr->mflag |= MFLAG_RUN_BREAK_RIGHT;
		deepright = TRUE;
	}

	/* Looking for a break */
	if ((m_ptr->mflag & (MFLAG_RUN_BREAK_LEFT)) && (m_ptr->mflag & (MFLAG_RUN_BREAK_RIGHT)))
	{
		/* Not looking for open area */
		m_ptr->mflag &= ~(MFLAG_RUN_OPEN_AREA);

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01)
		{
			if (deepleft && !deepright)
			{
				m_ptr->run_old_dir = cycle[i - 1];
			}
			else if (deepright && !deepleft)
			{
				m_ptr->run_old_dir = cycle[i + 1];
			}
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(cycle[i], row, col))
		{
			if (shortleft && !shortright)
			{
				m_ptr->run_old_dir = cycle[i - 2];
			}
			else if (shortright && !shortleft)
			{
				m_ptr->run_old_dir = cycle[i + 2];
			}
		}
	}
}


/*
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
static bool run_test(void)
{
	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int prev_dir;
	int new_dir;
	int check_dir = 0;

	int row, col;
	int i, max;
	int option, option2;

	int feat;

	bool notice;

	/* No options yet */
	option = 0;
	option2 = 0;

	/* Where we came from */
	prev_dir = m_ptr->run_old_dir;


	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;


	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++)
	{
		s16b this_o_idx, next_o_idx = 0;


		/* New direction */
		new_dir = cycle[chome[prev_dir] + i];

		/* New location */
		row = fy + ddy[new_dir];
		col = fx + ddx[new_dir];


		/* Visible monsters abort running */
		if (cave_m_idx[row][col] > 0)
		{
			return (TRUE);
		}

		/* Analyze unknown grids and floors */
		if (cave_floor_bold(row, col))
		{
			/* Looking for open area */
			if (m_ptr->mflag & (MFLAG_RUN_OPEN_AREA))
			{
				/* Nothing */
			}

			/* The first new direction. */
			else if (!option)
			{
				option = new_dir;
			}

			/* Three new directions. Stop running. */
			else if (option2)
			{
				return (TRUE);
			}

			/* Two non-adjacent new directions.  Stop running. */
			else if (option != cycle[chome[prev_dir] + i - 1])
			{
				return (TRUE);
			}

			/* Two new (adjacent) directions (case 1) */
			else if (new_dir & 0x01)
			{
				check_dir = cycle[chome[prev_dir] + i - 2];
				option2 = new_dir;
			}

			/* Two new (adjacent) directions (case 2) */
			else
			{
				check_dir = cycle[chome[prev_dir] + i + 1];
				option2 = option;
				option = new_dir;
			}
		}

		/* Obstacle, while looking for open area */
		else
		{
			if (m_ptr->mflag & (MFLAG_RUN_OPEN_AREA))
			{
				if (i < 0)
				{
					/* Break to the right */
					m_ptr->mflag |= MFLAG_RUN_BREAK_RIGHT;
				}

				else if (i > 0)
				{
					/* Break to the left */
					m_ptr->mflag |= MFLAG_RUN_BREAK_LEFT;
				}
			}
		}
	}


	/* Looking for open area */
	if (m_ptr->mflag & (MFLAG_RUN_OPEN_AREA)run_open_area)
	{
		/* Hack -- look again */
		for (i = -max; i < 0; i++)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = fy + ddy[new_dir];
			col = fx + ddx[new_dir];

			/* Get feature */
			feat = cave_feat[row][col];

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Unknown grid or non-wall */
			/* Was: cave_floor_bold(row, col) */
			if (!(play_info[row][col] & (PLAY_MARK)) ||
			    (!(f_info[feat].flags1 & (FF1_WALL))) )
			{
				/* Looking to break right */
				if ((m_ptr->mflag & (MFLAG_RUN_BREAK_RIGHT)))
				{
					return (TRUE);
				}
			}


			/* Obstacle */
			else
			{
				/* Looking to break left */
				if ((m_ptr->mflag & (MFLAG_RUN_BREAK_LEFT)))
				{
					return (TRUE);
				}
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = fy + ddy[new_dir];
			col = fx + ddx[new_dir];

			/* Get feature */
			feat = cave_feat[row][col];

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Unknown grid or non-wall */
			/* Was: cave_floor_bold(row, col) */
			if (!(play_info[row][col] & (PLAY_MARK)) ||
			    (!(f_info[feat].flags1 & (FF1_WALL))))
			{
				/* Looking to break left */
				if ((m_ptr->mflag & (MFLAG_RUN_BREAK_LEFT)))
				{
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if ((m_ptr->mflag & (MFLAG_RUN_BREAK_RIGHT)))
				{
					return (TRUE);
				}
			}
		}
	}


	/* Not looking for open area */
	else
	{
		/* No options */
		if (!option)
		{
			return (TRUE);
		}

		/* One option */
		else if (!option2)
		{
			/* Primary option */
			m_ptr->run_cur_dir = option;

			/* No other options */
			m_ptr->run_old_dir = option;
		}

		/* Two options, examining corners */
		else if (run_use_corners && !run_cut_corners)
		{
			/* Primary option */
			m_ptr->run_cur_dir = option;

			/* Hack -- allow curving */
			m_ptr->run_old_dir = option2;
		}

		/* Two options, pick one */
		else
		{
			/* Get next location */
			row = fy + ddy[option];
			col = fx + ddx[option];

			/* Don't see that it is closed off. */
			/* This could be a potential corner or an intersection. */
			if (!see_wall(option, row, col) ||
			    !see_wall(check_dir, row, col))
			{
				/* Can not see anything ahead and in the direction we */
				/* are turning, assume that it is a potential corner. */
				if (run_use_corners &&
				    see_nothing(option, row, col) &&
				    see_nothing(option2, row, col))
				{
					m_ptr->run_cur_dir = option;
					m_ptr->run_old_dir = option2;
				}

				/* STOP: we are next to an intersection or a room */
				else
				{
					return (TRUE);
				}
			}

			/* This corner is seen to be enclosed; we cut the corner. */
			else if (run_cut_corners)
			{
				m_ptr->run_cur_dir = option2;
				m_ptr->run_old_dir = option2;
			}

			/* This corner is seen to be enclosed, and we */
			/* deliberately go the long way. */
			else
			{
				m_ptr->run_cur_dir = option;
				m_ptr->run_old_dir = option2;
			}
		}
	}


	/* About to hit a known wall, stop */
	if (see_wall(m_ptr->run_cur_dir, fy, fx))
	{
		return (TRUE);
	}


	/* Failure */
	return (FALSE);
}



/*
 * Take one step along the current "run" path
 *
 * Called with a real direction to begin a new run, and with zero
 * to continue a run in progress.
 */
void mon_run_step(int dir)
{
	/* Start run */
	if (dir)
	{
		/* Initialize */
		run_init(dir);
	}

	/* Continue run */
	else
	{
		/* Update run */
		if (run_test())
		{
			/* Done */
			return;
		}
	}

}


#endif

