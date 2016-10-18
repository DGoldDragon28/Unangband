/* File: melee2.c */

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
 * Terrified monsters will turn to fight if they are slower than the
 * character, and closer to him than this distance.
 */
#define TURN_RANGE      3


/*
 *  Fake RF4_ masks to be used for ranged melee attacks;
 */

static u32b rf4_ball_mask;
static u32b rf4_self_target_mask;
static u32b rf4_no_player_mask;
static u32b rf4_beam_mask;
static u32b rf4_bolt_mask;
static u32b rf4_archery_mask;



/*
 * Calculate minimum and desired combat ranges.  -BR-
 */
void find_range(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u16b p_lev, m_lev;
	u16b p_chp, p_mhp;
	u16b m_chp, m_mhp;
	u32b p_val, m_val;

	/* Allied monsters without a target, or targetting the player */
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && (!(m_ptr->ty) ||
			((m_ptr->ty == p_ptr->py) && (m_ptr->tx == p_ptr->px))))
	{
		/* Breeders are deliberately annoying */
		if (r_ptr->flags2 & (RF2_MULTIPLY)) m_ptr->min_range = 1;

		/* Fleeing monsters are deliberately annoying */
		else if (m_ptr->monfear) m_ptr->min_range = 1;

		/* Some monsters run when low on mana */
		else if ((r_ptr->flags2 & (RF2_LOW_MANA_RUN)) &&
		    (m_ptr->mana < r_ptr->mana / 6)) m_ptr->min_range = 1;

		/* Don't crowd the player */
		else m_ptr->min_range = TURN_RANGE;

		/* Set the best range */
		m_ptr->best_range = m_ptr->min_range;

		return;
	}

	/* All "afraid" monsters will run away */
	if (m_ptr->monfear) m_ptr->min_range = FLEE_RANGE;

	/* Some monsters run when low on mana */
	else if ((r_ptr->flags2 & (RF2_LOW_MANA_RUN)) &&
	    (m_ptr->mana < r_ptr->mana / 6)) m_ptr->min_range = FLEE_RANGE;

	/* Hack -- townsmen go about their business in town, and are suspicious otherwise*/
	else if (m_ptr->mflag & (MFLAG_TOWN)) m_ptr->min_range = room_near_has_flag(m_ptr->fy, m_ptr->fx, ROOM_TOWN) ? 1 : 3;

	/* Breeders cannot be terrified */
	else if (r_ptr->flags2 & (RF2_MULTIPLY)) m_ptr->min_range = 1;

	/* Weak enemies may flee */
	else if ((m_ptr->mflag & (MFLAG_ALLY)) == 0)
	{
		/* Minimum distance - stay at least this far if possible */
		m_ptr->min_range=1;

		/* Examine player power (level) */
		p_lev = p_ptr->lev;

		/* Examine monster power (level plus morale) */
		m_lev = r_ptr->level + (cave_m_idx[m_ptr->fy][m_ptr->fx] & 0x08) + 25;

		/* Optimize extreme cases below */
		if (m_lev < p_lev + 4) m_ptr->min_range = FLEE_RANGE;
		else if (m_lev + 3 < p_lev)
		{

			/* Examine player health */
			p_chp = p_ptr->chp;
			p_mhp = p_ptr->mhp;

			/* Examine monster health */
			m_chp = m_ptr->hp;
			m_mhp = m_ptr->maxhp;

			/* Prepare to optimize the calculation */
			p_val = (p_lev * p_mhp) + (p_chp << 2);	/* div p_mhp */
			m_val = (m_lev * m_mhp) + (m_chp << 2);	/* div m_mhp */

			/* Strong players scare strong monsters */
			if (p_val * m_mhp > m_val * p_mhp) m_ptr->min_range = FLEE_RANGE;
		}
	}

	if (m_ptr->min_range < FLEE_RANGE)
	{
		/* Creatures that don't move never like to get too close */
		if (r_ptr->flags1 & (RF1_NEVER_MOVE)) m_ptr->min_range += 3;

		/* Spellcasters that don't stike never like to get too close */
		if (r_ptr->flags1 & (RF1_NEVER_BLOW)) m_ptr->min_range += 3;

		/* Petrified monsters would get away if they could */
		if (m_ptr->petrify) m_ptr->min_range += 3;
	}

	/* Maximum range to flee to (reduced elsewhere for themed levels */
	if (!(m_ptr->min_range < FLEE_RANGE)) m_ptr->min_range = FLEE_RANGE;

	/* Nearby monsters that cannot run away, won't unless completely afraid */
	else if ((m_ptr->cdis < TURN_RANGE) && (m_ptr->mspeed < p_ptr->pspeed))
		m_ptr->min_range = 1;

	/* Now find prefered range */
	m_ptr->best_range = m_ptr->min_range;

	/* Note below: Monsters who have had dangerous attacks happen to them
	   are more extreme as are monsters that are currenty hidden. */

	/* TODO: take range of spell and innate attacks into account;
	   update whenever mana levels change, not only when player attacks */

	/* Spellcasters with mana want to sit back */
	if (r_ptr->freq_spell
		> ((m_ptr->mflag & (MFLAG_AGGR | MFLAG_HIDE)) != 0 ? 0 : 24)
		&& m_ptr->mana >= r_ptr->mana / 6
		&& (((r_ptr->flags5 & (RF5_ATTACK_MASK)) != 0) ||
			((r_ptr->flags6 & (RF6_ATTACK_MASK)) != 0) ||
			((r_ptr->flags7 & (RF7_ATTACK_MASK)) != 0)))
	{
		/* Sit back - further if aggressive */
		m_ptr->best_range = 6 + ((m_ptr->mflag & (MFLAG_AGGR)) != 0 ? 2 : 0);
	}

	/* Breathers want to mix it up a little */
	else if ((r_ptr->freq_innate > ((m_ptr->mflag & (MFLAG_AGGR | MFLAG_HIDE)) != 0 ? 0 : 24)) &&
			((r_ptr->flags4 & (RF4_BREATH_MASK)) != 0))
	{
		m_ptr->best_range = 6;
	}

	/* Archers with ammo want to sit back */
	else if (((r_ptr->flags2 & (RF2_ARCHER)) != 0)
			 && find_monster_ammo(m_idx, -1, FALSE) >= 0)
	{
		/* Don't back off for aggression due to limited range */
		m_ptr->best_range = 6;
	}

	/* Innate magic users with mana (or with 0 max mana), and archers want to sit back */
	else if (r_ptr->freq_innate > ((m_ptr->mflag & (MFLAG_AGGR | MFLAG_HIDE)) != 0 ? 0 : 24)
			 && ((m_ptr->mana >= r_ptr->mana / 6)
			 || (((r_ptr->flags4 & (rf4_archery_mask)) != 0)
			 && (find_monster_ammo(m_idx, -1, FALSE) >= 0))))
	{
		m_ptr->best_range = 6 + ((m_ptr->mflag & (MFLAG_AGGR)) != 0 ? 2 : 0);
	}

	/* Creatures that don't move never like to get too close */
	else if (r_ptr->flags1 & (RF1_NEVER_MOVE)) m_ptr->best_range = 6;

	/* Petrified creatures never like to get too close */
	else if (m_ptr->petrify) m_ptr->best_range = 6;

	/* Spellcasters that don't strike never like to get too close */
	else if (r_ptr->flags1 & (RF1_NEVER_BLOW)) m_ptr->best_range = 8;

	/* Allow faster monsters to hack and back */
	/* Check speed differential. If we are fast enough to move more than once
	 * for every 1 targets move, and next to it, and have exactly 1
	 * move left, we set best_range to 4. */
	else if (m_ptr->mspeed > 115)
	{
		/* Hack and back against another monster */
		if ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_IGNORE)) &&
				(cave_m_idx[m_ptr->ty][m_ptr->tx]) &&
				(distance(m_ptr->ty, m_ptr->tx, m_ptr->fy, m_ptr->fx) == 1))
		{
			monster_type *n_ptr = &m_list[cave_m_idx[m_ptr->ty][m_ptr->tx]];

			/* Ensure that its an enemy */
			if ((((m_ptr->mflag & (MFLAG_ALLY)) != 0) != ((n_ptr->mflag & (MFLAG_ALLY)) != 0)) &&

			/* Hack -- check min_range, best_range instead of figuring out the monster's range */
				(n_ptr->min_range <= 1) && (n_ptr->best_range <= 1) &&

			/* Enough of a differential. This doesn't have to be exact */
				(n_ptr->mspeed < m_ptr->mspeed - 5) &&
				/* This is more important. The other monster will be
				 * able to act before us again. Therefore we are currently
				 * doing our last move. */
				(n_ptr->energy + extract_energy[n_ptr->mspeed] >= 100))
			{
				m_ptr->best_range = 4;

				/* Ensure we don't push anyone else into this space */
				m_ptr->mflag |= (MFLAG_PUSH);
			}
		}
		/* Hack and back against player */
		else if (((m_ptr->mflag & (MFLAG_ALLY)) == 0) && (m_ptr->cdis == 1))
		{
			/* Enough of a differential. This doesn't have to be exact */
			if ((p_ptr->pspeed < m_ptr->mspeed - 5) &&
				/* This is more important. The other monster will be
				 * able to act before us again. Therefore we are currently
				 * doing our last move. */
				(p_ptr->energy + extract_energy[p_ptr->pspeed] >= 100))
			{
				m_ptr->best_range = 4;

				/* Ensure we don't push anyone else into this space */
				m_ptr->mflag |= (MFLAG_PUSH);
			}
		}
	}
	/* Other aggressive monsters close immediately */
	else if (m_ptr->mflag & (MFLAG_AGGR))
	{
		m_ptr->min_range = 1;
	}
}




/*
 * Get and return the strength (age) of scent in a given grid.
 *
 * Return "-1" if no scent exists in the grid.
 */
int get_scent(int y, int x)
{
	int age;
	int scent;

	/* Check Bounds */
	if (!(in_bounds(y, x))) return (-1);

	/* Sent trace? */
	scent = cave_when[y][x];

	/* No scent at all */
	if (!scent) return (-1);

	/* Get age of scent */
	age = scent - scent_when;

	/* Return the age of the scent */
	return (age);
}


/*
 * Can the monster catch a whiff of the character?
 *
 * Many more monsters can smell, but they find it hard to smell and
 * track down something at great range.
 */
static bool monster_can_smell(monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int age;

	/* Get the age of the scent here */
	age = get_scent(m_ptr->fy, m_ptr->fx);

	/* No scent */
	if (age == -1) return (FALSE);

	/* Scent is too old */
	if (age > SMELL_STRENGTH) return (FALSE);

	/* Super scent are amazing trackers */
	if (r_ptr->flags9 & (RF9_SUPER_SCENT))
	{
		/* I smell a character! */
		return (TRUE);
	}

	/* Other monsters can sometimes make good use of scent */
	else if (r_ptr->flags9 & (RF9_SCENT))
	{
		if (age <= SMELL_STRENGTH - 10)
		{
			/* Something's in the air... */
			return (TRUE);
		}
	}

	/* You're imagining things. */
	return (FALSE);
}


/*
 * Determine if there is a space near the player in which
 * a summoned creature can appear
 */
static bool summon_possible(int y1, int x1)
{
	int y, x;

	/* Start at the player's location, and check 2 grids in each dir */
	for (y = y1 - 2; y <= y1 + 2; y++)
	{
		for (x = x1 - 2; x <= x1 + 2; x++)
		{
			/* Ignore illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Only check a circular area */
			if (distance(y1, x1, y, x) > 2) continue;

			/* Hack: no summon on glyph of warding */
			if (f_info[cave_feat[y][x]].flags1 & (FF1_GLYPH)) continue;

			/* Require empty floor grid in line of fire */
			if (cave_empty_bold(y, x) && generic_los(y1, x1, y, x, CAVE_XLOF))
			{
				return (TRUE);
			}
		}
	}

	return FALSE;
}



/*
 * Offsets for the spell indices
 */
#define RF4_OFFSET 32 * 3
#define RF5_OFFSET 32 * 4
#define RF6_OFFSET 32 * 5
#define RF7_OFFSET 32 * 6


/*states if monsters on two separate coordinates are similar or not*/
static bool similar_monsters(int m1y, int m1x, int m2y, int m2x)
{
	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_type *n_ptr;
	monster_race *nr_ptr;

	/*first check if there are monsters on both coordinates*/
	if (!(cave_m_idx[m1y][m1x] > 0)) return(FALSE);
	if (!(cave_m_idx[m2y][m2x] > 0)) return(FALSE);

	/* Access monster 1*/
	m_ptr = &m_list[cave_m_idx[m1y][m1x]];
	r_ptr = &r_info[m_ptr->r_idx];

	/* Access monster 2*/
	n_ptr = &m_list[cave_m_idx[m2y][m2x]];
	nr_ptr = &r_info[n_ptr->r_idx];

	/* Monsters have the same symbol */
	if (r_ptr->d_char == nr_ptr->d_char) return(TRUE);

	/* Professional courtesy */
	if ((r_ptr->flags3 & (RF3_EVIL)) && (nr_ptr->flags3 & (RF3_EVIL))) return(TRUE);

	/*
	 * Same race (we are not checking orcs, giants, or
	 * trolls because that would be true at
	 * the symbol check
	 * Evil probobly covers this as well, but you never know
	 */
	if ((r_ptr->flags3 & (RF3_DRAGON)) && (nr_ptr->flags3 & (RF3_DRAGON))) return(TRUE);

	/*
	 * Same race (we are not checking orcs, giants or
	 * trolls because that would be true at
	 * the symbol check
	 * Evil probobly covers this as well, but you never know
	 */
	if ((r_ptr->flags3 & (RF3_DEMON)) && (nr_ptr->flags3 & (RF3_DEMON))) return(TRUE);

	/*We are not checking for animal*/

	/*Not the same*/
	return(FALSE);
}


/*
 * Used to determine the player's known level of resistance to a
 * particular spell.
 *
 * This now uses the GF_ constants instead of the LRN_ constants.
 * Rather than passing the monster index, we instead pass the
 * smart flag, which allows us to estimate damage (potentially)
 * for the randart.c functions or power computations or similar.
 *
 * We should use this in several places to estimate the actual
 * damage.
 */
static int find_resist(u32b smart, int effect)
{
	int a = 0;

	/* Nothing Known */
	if (!smart) return (0);

	/* Which spell */
	switch (effect)
	{
		/* Acid Spells */
		case GF_ACID:
		case GF_VAPOUR:
		{
			if (smart & (SM_IMM_ACID)) return (100);
			else if ((smart & (SM_OPP_ACID)) && (smart & (SM_RES_ACID))) return (70);
			else if ((smart & (SM_OPP_ACID)) || (smart & (SM_RES_ACID))) return (40);
			else return (0);
		}

		/* Lightning Spells */
		case GF_ELEC:
		{
			if (smart & (SM_IMM_ELEC)) return (100);
			else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC))) return (70);
			else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC))) return (40);
			else return (0);
		}

		/* Fire Spells */
		case GF_FIRE:
		case GF_SMOKE:
		{
			if (smart & (SM_IMM_FIRE)) return (100);
			else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) return (70);
			else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) return (40);
			else return (0);
		}

		/* Cold Spells */
		case GF_COLD:
		{
			if (smart & (SM_IMM_COLD)) return (100);
			else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD))) return (70);
			else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD))) return (40);
			else return (0);
		}

		/* Poison Spells */
		case GF_POIS:
		case GF_POISON_WEAK:
		case GF_POISON_HALF:
		{
			if (smart & (SM_IMM_POIS)) return (100);
			else if ((smart & (SM_OPP_POIS)) && (smart & (SM_RES_POIS))) return (80);
			else if ((smart & (SM_OPP_POIS)) || (smart & (SM_RES_POIS))) return (55);
			else return (0);
		}

		/* Plasma Spells */
		case GF_PLASMA:
		{
			if (smart & (SM_RES_SOUND)) return (15);
			else return (0);
		}

		/* Nether Spells */
		case GF_NETHER:
		{
			if (smart & (SM_RES_NETHR)) return (30);
			else return (0);
		}

		/* Water Spells */
		case GF_WATER:
		{
			if (smart & (SM_RES_CONFU)) a += 10;
			if (smart & (SM_RES_SOUND)) a += 5;
			return (a);
		}

		/* Storm Spells */
		case GF_STORM:
		{
			if (smart & (SM_IMM_ELEC)) a += 33;
			else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC))) a += 22;
			else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC))) a += 11;
			if (smart & (SM_RES_CONFU)) a += 3;
			if (smart & (SM_RES_SOUND)) a += 2;
			return (a);
		}

		/* Wind spells */
		case GF_WIND:
		{
			/* Maybe should notice feather fall */
			return (0);
		}

		/* Chaos Spells */
		case GF_CHAOS:
		{
			if (smart & (SM_RES_CHAOS)) return(30);
			if (smart & (SM_RES_NETHR))  a += 10;
			if (smart & (SM_RES_CONFU))  a += 10;
			else return (a);
		}

		/* Shards Spells */
		case GF_SHARD:
		{
			if (smart & (SM_RES_SHARD)) return (30);
			else return (0);
		}

		/* Sound Spells */
		case GF_SOUND:
		case GF_FORCE:
		{
			if (smart & (SM_RES_SOUND)) a += 30;
			else return (a);
		}

		/* Confusion Spells, damage dealing */
		case GF_CONFUSION:
		{
			if (smart & (SM_RES_CONFU)) return (30);
			else return (0);
		}

		/* Hallucination or save */
		case GF_HALLU:
		{
			if (smart & (SM_RES_CHAOS)) a = 100;
			else if (smart & (SM_PERF_SAVE)) a = 100;
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 30;
				if (p_ptr->timed[TMD_AFRAID]) a += 50;
			}
			return (a);
		}

		/* Disenchantment Spells */
		case GF_DISENCHANT:
		{
			if (smart & (SM_RES_DISEN)) return (30);
			else return (0);
		}

		/* Nexus Spells */
		case GF_NEXUS:
		{
			if (smart & (SM_RES_NEXUS)) return (30);
			else return (0);
		}

		/* Light Spells */
		case GF_LITE_WEAK:
		{
			if (smart & (SM_RES_BLIND)) a = 60;
			if ((cave_info[p_ptr->py][p_ptr->px] & (CAVE_GLOW)) != 0) a += 10;

			/* Drop through */
		}
		case GF_LITE:
		{
			if (smart & (SM_RES_LITE)) a += 30;
			return (a);
		}

		/* Darkness Spells */
		case GF_DARK_WEAK:
		{
			if (smart & (SM_RES_BLIND)) a = 60;
			if ((cave_info[p_ptr->py][p_ptr->px] & (CAVE_GLOW)) == 0) a += 10;

			/* Drop through */
		}
		case GF_DARK:
		{
			if (smart & (SM_RES_DARK)) a += 30;
			return (a);
		}

		/* Ice Spells */
		case GF_ICE:
		{
			if (smart & (SM_IMM_COLD)) a=90;
			else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD))) a = 60;
			else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD))) a = 30;

			if (smart & (SM_RES_SOUND)) a += 5;
			if (smart & (SM_RES_SHARD)) a += 5;
			return (a);
		}

		/* Terrify spells */
		case GF_TERRIFY:
		{
			if (smart & (SM_RES_FEAR)) return (30);
			else if (smart & (SM_OPP_FEAR)) return(30);
			else if (smart & (SM_PERF_SAVE)) return(10);
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 5;
				if (p_ptr->timed[TMD_AFRAID]) a += 5;
			}
			return (a);
		}

		/* Save-able spells */
		case GF_BLIND:
		{
			if (smart & (SM_RES_BLIND)) return (10);
			else if (smart & (SM_PERF_SAVE)) return (10);
			else if (smart & (SM_GOOD_SAVE)) return (5);
			else return (0);
		}

		/* Save-able spells */
		case GF_SLOW:
		case GF_PARALYZE:
		{
			if (smart & (SM_FREE_ACT)) return (10);
			else if (smart & (SM_PERF_SAVE)) return (10);
			else if (smart & (SM_GOOD_SAVE)) return (5);
			else return (0);
		}

		/* Lava Spells */
		case GF_LAVA:
		{
			if (smart & (SM_IMM_FIRE)) a = 85;
			else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) a = 45;
			else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) a = 25;
			else a = 0;

			if (smart & (SM_RES_CONFU)) a += 10;
			if (smart & (SM_RES_SOUND)) a += 5;
			return (a);
		}

		/* Geothermal Spells */
		case GF_BWATER:
		case GF_BMUD:
		{
			if (smart & (SM_IMM_FIRE)) a = 15;
			else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) a = 10;
			else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) a = 5;
			else a = 0;

			if (smart & (SM_RES_CONFU)) a += 10;
			if (smart & (SM_RES_SOUND)) a += 5;
			return (a);
		}

		/* Geothermal Spells */
		case GF_STEAM:
		{
			if (smart & (SM_IMM_FIRE)) a = 30;
			else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) a = 20;
			else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) a = 10;
			else a = 0;

			return (a);
		}

		/* Spells that attack player mana */
		case GF_LOSE_MANA:
		case GF_GAIN_MANA:
		{
			if (smart & (SM_IMM_MANA)) return (100);
			else return (0);
		}

		/* Spells Requiring Save or Resist Fear */
		case GF_FEAR_WEAK:
		{
			if (smart & (SM_RES_FEAR)) a = 100;
			else if (smart & (SM_OPP_FEAR)) a = 100;
			else if (smart & (SM_PERF_SAVE)) a = 100;
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 30;
				if (p_ptr->timed[TMD_AFRAID]) a += 50;
			}
			return (a);
		}

		/* Spells Requiring Save or Resist Blindness */
		case GF_BLIND_WEAK:
		{
			if (smart & (SM_RES_BLIND)) a = 100;
			else if (smart & (SM_PERF_SAVE)) a = 100;
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 30;
				if (p_ptr->timed[TMD_BLIND]) a += 50;
			}
			return (a);
		}

		/* Spells Requiring Save or Resist Confusion */
		case GF_CONF_WEAK:
		{
			if (smart & (SM_RES_CONFU)) a = 100;
			else if (smart & (SM_PERF_SAVE)) a = 100;
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 30;
				if (p_ptr->timed[TMD_CONFUSED]) a += 50;
			}
			return (a);
		}

		/* Spells Requiring Save or Free Action */
		case GF_SLEEP:
		case GF_SLOW_WEAK:
		{
			if (smart & (SM_FREE_ACT)) a = 100;
			else if (smart & (SM_PERF_SAVE)) a = 100;
			else if (p_ptr->timed[TMD_PARALYZED]) a = 80;
			else
			{
				if (smart & (SM_GOOD_SAVE)) a += 30;
				if (p_ptr->timed[TMD_SLOW]) a += 50;
			}
			return (a);
		}

		/* Spells that require the player be evil */
		case GF_HOLY_ORB:
		{
			if (!(p_ptr->cur_flags4 & (TR4_EVIL))) return (50);
			return(0);
		}

		/* Spells that require the player not be evil or fire resistant */
		case GF_HELLFIRE:
		{
			if (smart & (SM_IMM_FIRE)) a = 66;
			else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) a = 44;
			else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) a = 22;

			if (p_ptr->cur_flags4 & (TR4_EVIL)) a += 33;
			else if (smart & (SM_RES_DARK)) a += 22;
			return(a);
		}

		/* Spells that probe the player */
		case GF_PROBE:
		{
			/* Hack -- do we still have abilities to learn */
			if (((player_smart_flags(p_ptr->cur_flags1, p_ptr->cur_flags2, p_ptr->cur_flags3, p_ptr->cur_flags4)) & ~(smart)) != 0) return (0);

			return(100);
		}

		/* Anything else */
		default:
		{
			return (0);
		}
	}
}


/*
 * Used to exclude spells which are too expensive for the
 * monster to cast.  Excludes all spells that cost more than the
 * current available mana.
 *
 * Smart monsters may also exclude spells that use a lot of mana,
 * even if they have enough.
 *
 * -BR-
 */
static void remove_expensive_spells(int m_idx, u32b *f4p, u32b *f5p, u32b *f6p, u32b *f7p)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int i, max_cost;

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);
	u32b f7 = (*f7p);

	/* Remove spells that require ammunition if the caster doesn't have any */
	for (i = 0; i < 4; i++)
	{
		if (find_monster_ammo(m_idx, i, FALSE) < 0) f4 &= ~(RF4_BLOW_1 << i);
	}

	/* Determine maximum amount of mana to be spent */
	/* Smart monsters will usually not blow all their mana on one spell.
	 */
	if (r_ptr->flags2 & (RF2_SMART))
		max_cost = (m_ptr->mana * (rand_range(4, 6))) / 6;

	/* Otherwise spend up to the full current mana */
	else max_cost = m_ptr->mana;

	/* check innate spells for mana available */
	for (i = 0; i < 32; i++)
	{
		if (spell_info_RF4[i][COL_SPELL_MANA_COST] > max_cost) f4 &= ~(0x00000001 << i);
	}

	/* check normal spells for mana available */
	for (i = 0; i < 32; i++)
	{
		if (spell_info_RF5[i][COL_SPELL_MANA_COST] > max_cost) f5 &= ~(0x00000001 << i);
	}

	/* check other spells for mana available */
	for (i = 0; i < 32; i++)
	{
		if (spell_info_RF6[i][COL_SPELL_MANA_COST] > max_cost) f6 &= ~(0x00000001 << i);
	}

	/* check other spells for mana available */
	for (i = 0; i < 32; i++)
	{
		if (spell_info_RF7[i][COL_SPELL_MANA_COST] > max_cost) f7 &= ~(0x00000001 << i);
	}

	/* Modify the spell list. */
	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
	(*f7p) = f7;

}

/*
 * Intelligent monsters use this function to filter away spells
 * which have no benefit.
 */
static void remove_useless_spells(int m_idx, u32b *f4p, u32b *f5p, u32b *f6p, u32b *f7p, bool require_los)
{
	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);
	u32b f7 = (*f7p);

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Don't regain mana if full */
	if (m_ptr->mana >= r_ptr->mana) f6 &= ~(RF6_ADD_MANA);

	/* Don't regain ammo if has some */
	if ((f4 & RF4_ADD_AMMO) && (find_monster_ammo(m_idx, -1, FALSE) >= 0)) f4 &= ~(RF4_ADD_AMMO);

	/* Don't heal if full */
	if (m_ptr->hp >= m_ptr->maxhp) f6 &= ~(RF6_HEAL);

	/* Don't Haste if Hasted */
	if (m_ptr->hasted) f6 &= ~(RF6_HASTE);

	/* Don't Invisible if Invisible or Can't be seen */
	if ((m_ptr->tim_invis) || !(m_ptr->ml)) f6 &= ~(RF6_INVIS);

	/* Don't Invisible if player known to see invisible */
	if (m_ptr->smart & (SM_SEE_INVIS)) f6 &= ~(RF6_INVIS);

	/* Don't Wraithform if Wraithform and Wraithform not running out in dangerous terrain */
	if ((m_ptr->tim_passw) && ((m_ptr->tim_passw > 10) || (place_monster_here(m_ptr->fy,m_ptr->fx,m_ptr->r_idx) > MM_FAIL))) f6 &= ~(RF6_WRAITHFORM);

	/* Don't Bless if Blessed */
	if (m_ptr->bless) f6 &= ~(RF6_BLESS);

	/* Don't Berserk if Berserk */
	if (m_ptr->berserk) f6 &= ~(RF6_BERSERK);

	/* Don't Shield if Shielded */
	if (m_ptr->shield) f6 &= ~(RF6_SHIELD);

	/* Don't Oppose elements if Oppose Elements */
	if (m_ptr->oppose_elem) f6 &= ~(RF6_OPPOSE_ELEM);

	/* Don't cure if not needed */
	if (!((m_ptr->stunned) ||(m_ptr->monfear) || (m_ptr->confused) || (m_ptr->blind) ||
	      (m_ptr->cut) || (m_ptr->poisoned) || (m_ptr->dazed) || (m_ptr->image) ||
	      (m_ptr->amnesia) || (m_ptr->terror)))	f6 &= ~(RF6_CURE);

	/* Don't jump in already close, or don't want to be close */
	if (!(m_ptr->cdis > m_ptr->best_range) && require_los)
		f6 &= ~(RF6_TELE_TO | RF6_TELE_SELF_TO);

	/* Don't teleport to or teleport self to if want to stay away */
	if (m_ptr->min_range > 5) f6 &= ~(RF6_TELE_TO | RF6_TELE_SELF_TO);

	/* Do not teleport to or teleport self to if too close - either adjacent or 1 step away */
	if (m_ptr->cdis < 3) f6 &= ~(RF6_TELE_TO | RF6_TELE_SELF_TO);

	/* Modify the spell list. */
	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
	(*f7p) = f7;

}



/*
 * Count the number of castable spells.
 *
 * If exactly 1 spell is available cast it.  If more than more is
 * available, and the random bit is set, pick one.
 *
 * Used as a short cut in 'choose_attack_spell' to circumvent AI
 * when there is only 1 choice. (random=FALSE)
 *
 * Also used in 'choose_attack_spell' to circumvent AI when
 * casting randomly (random=TRUE), as with dumb monsters.
 */
static int choose_attack_spell_fast(int m_idx, int target_m_idx, u32b *f4p, u32b *f5p, u32b *f6p, u32b *f7p, bool do_random)
{
	int i, num=0;
	byte spells[128];

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);
	u32b f7 = (*f7p);

	/* Extract the "innate" spells */
	for (i = 0; i < 32; i++)
	{
		if (f4 & (1L << i)) spells[num++] = i + 32 * 3;
	}

	/* Extract the "attack" spells */
	for (i = 0; i < 32; i++)
	{
		if (f5 & (1L << i)) spells[num++] = i + 32 * 4;
	}

	/* Extract the "miscellaneous" spells */
	for (i = 0; i < 32; i++)
	{
		if (f6 & (1L << i)) spells[num++] = i + 32 * 5;
	}

	/* Extract the "summon" spells */
	for (i = 0; i < 32; i++)
	{
		if (f7 & (1L << i)) spells[num++] = i + 32 * 6;
	}

	/* Paranoia */
	if (num == 0) return (0);

	/* Go quick if possible */
	if (num == 1)
	{
		/* Hack - Don't cast if known to be immune, unless
		 * casting randomly anyway.  */
		if (!(do_random))
		{
			monster_type *m_ptr = &m_list[m_idx];

			u32b smart = m_ptr->smart;

			if (target_m_idx > 0) smart = monster_smart_flags(target_m_idx);

			if (spells[0] < 128)
			{
				if (find_resist(smart, spell_desire_RF4[spells[0]-96][D_RES]) == 100) return (0);
			}
			else if (spells[0] < 160)
			{
				if (find_resist(smart, spell_desire_RF5[spells[0]-128][D_RES]) == 100) return (0);
			}
			else if (spells[0] < 192)
			{
				if (find_resist(smart, spell_desire_RF6[spells[0]-160][D_RES]) == 100) return (0);
			}
			else
			{
				if (find_resist(smart, spell_desire_RF7[spells[0]-192][D_RES]) == 100) return (0);
			}
		}

		/* Otherwise cast the one spell */
		else return (spells[0]);
	}

	/*
	 * If we aren't allowed to choose at random
	 * and we have multiple spells left, give up on quick
	 * selection
	 */
	if (!(do_random)) return (0);

	/* Pick at random */
	return (spells[rand_int(num)]);
}



/*
 * Choose the "real" ty, tx for the spell.
 *
 * At the moment, we either leave the ty, tx as is, or
 * point it back at the casting monster, for spells that
 * assist them.
 *
 * Note that this routine chooses the offset from the actual target, not
 * a decision whether to attack the player or an allied monster.
 */
static int pick_target(int m_idx, int *tar_y, int *tar_x, int i)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u32b f4 = (RF4_ASSIST_MASK);
	u32b f5 = (RF5_ASSIST_MASK);
	u32b f6 = (RF6_ASSIST_MASK);
	u32b f7 = (RF7_ASSIST_MASK);

	/* Priests can assist others */
	if (((r_ptr->flags2 & (RF2_PRIEST)) != 0) &&

		/* Allow assisting the player if an ally */
		(((cave_m_idx[*tar_y][*tar_x] < 0) && ((m_ptr->mflag & (MFLAG_ALLY)) != 0)) ||

		/* Allow assisting those whose allegiance matches your own */
		(((cave_m_idx[*tar_y][*tar_x] > 0)) &&
			(((m_ptr->mflag & (MFLAG_ALLY)) != 0) == ((m_list[cave_m_idx[*tar_y][*tar_x]].mflag & (MFLAG_ALLY)) != 0)))))
	{
		/* Don't redirect assistance spells back to caster */
		f4 = f5 = f6 = f7 = 0L;
	}

	/* Check the spell */
	if (i < 128)
	{
		u32b flag = 1L << (i - 96);

		if ((flag & (f4 | rf4_self_target_mask | RF4_SUMMON_MASK)) != 0)
		{
			*tar_y = m_ptr->fy;
			*tar_x = m_ptr->fx;
		}
	}
	else if (i < 160)
	{
		u32b flag = 1L << (i - 128);

		if ((flag & (f5 | RF5_SELF_TARGET_MASK | RF5_SUMMON_MASK)) != 0)
		{
			*tar_y = m_ptr->fy;
			*tar_x = m_ptr->fx;
		}
	}
	else if (i < 192)
	{
		u32b flag = 1L << (i - 160);

		if ((flag & (f6 | RF6_SELF_TARGET_MASK | RF6_SUMMON_MASK)) != 0)
		{
			*tar_y = m_ptr->fy;
			*tar_x = m_ptr->fx;
		}
	}
	else
	{
		u32b flag = 1L << (i - 192);

		if ((flag & (f7 | RF7_SELF_TARGET_MASK | RF7_SUMMON_MASK)) != 0)
		{
			*tar_y = m_ptr->fy;
			*tar_x = m_ptr->fx;
		}
	}

	/* Player dodges -- point the target at their old location */
	if ((p_ptr->dodging) && (*tar_y == p_ptr->py) && (*tar_x == p_ptr->px) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0)
		&& (rand_int(100) < (extract_energy[p_ptr->pspeed] + adj_agi_ta[p_ptr->stat_ind[A_AGI]] - 128)))
	{
		/* msg_print("You dodge the attack."); */

		*tar_y = *tar_y + ddy_ddd[p_ptr->dodging];
		*tar_x = *tar_x + ddx_ddd[p_ptr->dodging];
	}

	return(i);
}



/*
 * Initialise spell_info and spell_desire tables for monster blows 1-4, plus
 * aura and explode attacks. We do this before calling choose_ranged_attack
 * and make_attack_ranged, so that the monster will be able to correctly use
 * their blows at range.
 *
 * Note that huge monsters can use all their blows at range 1-2.
 */
static void init_ranged_attack(monster_race *r_ptr)
{
	int ap_cnt;

	/* Paranoia - clear the fake masks */
	rf4_ball_mask = RF4_BALL_MASK;
	rf4_self_target_mask = RF4_SELF_TARGET_MASK;
	rf4_no_player_mask = RF4_NO_PLAYER_MASK;
	rf4_beam_mask = RF4_BEAM_MASK;
	rf4_bolt_mask = RF4_BOLT_MASK;
	rf4_archery_mask = RF4_ARCHERY_MASK;

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		int mana = 0;
		int range = 0;

		/* Extract the attack information */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		method_type *method_ptr = &method_info[method];

		/* Skip 'tricky' attacks */
		if (((method_ptr->flags2 & (PR2_RANGED)) == 0) && !(r_ptr->flags3 & (RF3_HUGE))) continue;

		/* Should have already initialised blows */
		/* r_ptr->flags4 |= (RF4_BLOW_1 << ap_cnt); */

		/* Initialise mana cost and range based on blow type.
		   Also apply hack for explode and aura if required. */
		mana = method_ptr->mana_cost;
		range = scale_method(method_ptr->max_range, r_ptr->level);

		/* Hack -- huge attacks */
		if ((method_ptr->flags2 & (PR2_RANGED)) == 0)
		{
			range = 2;
			rf4_beam_mask |= (RF4_BLOW_1 << ap_cnt);
		}
		/* Hack -- initialise ball / beam / archery flags */
		else
		{
			if (method_ptr->flags1 & (PROJECT_BOOM)) rf4_ball_mask |= (RF4_BLOW_1 << ap_cnt);
			if (method_ptr->flags1 & (PROJECT_SELF)) rf4_self_target_mask |= (RF4_BLOW_1 << ap_cnt);
			if (method_ptr->flags1 & (PROJECT_SELF)) rf4_no_player_mask |= (RF4_BLOW_1 << ap_cnt);
			if (method_ptr->flags1 & (PROJECT_BEAM)) rf4_beam_mask |= (RF4_BLOW_1 << ap_cnt);
			if (method_ptr->flags1 & (PROJECT_STOP)) rf4_bolt_mask |= (RF4_BLOW_1 << ap_cnt);
			if ((method_ptr->ammo_tval) && (range > 5)) rf4_archery_mask |= (RF4_BLOW_1 << ap_cnt);
		}

		/* Initialise the table */
		spell_info_RF4[ap_cnt][COL_SPELL_MANA_COST] = mana;
		spell_info_RF4[ap_cnt][COL_SPELL_DAM_MULT] = d_dice * (d_side + 1) / 2;
		spell_info_RF4[ap_cnt][COL_SPELL_DAM_DIV] = r_ptr->spell_power;
		spell_info_RF4[ap_cnt][COL_SPELL_DAM_VAR] = d_side;
		spell_info_RF4[ap_cnt][COL_SPELL_BEST_RANGE] = range;
		spell_desire_RF4[ap_cnt][D_BASE] = (mana ? 40 : 50);	/* Hack -- 40 for all blows, 50 for spells */
		spell_desire_RF4[ap_cnt][D_SUMM] = 0;
		spell_desire_RF4[ap_cnt][D_HURT] = 0;
		spell_desire_RF4[ap_cnt][D_MANA] = (mana ? 0 : 5);
		spell_desire_RF4[ap_cnt][D_ESC] = 0;
		spell_desire_RF4[ap_cnt][D_TACT] = 0;
		spell_desire_RF4[ap_cnt][D_RES] = effect;
		spell_desire_RF4[ap_cnt][D_RANGE] = 0;
	}
}


/*
 * Does the monster attack the player if they walk
 * into them?
 */
bool choose_to_attack_player(const monster_type *m_ptr)
{
	/* Allies don't attack player */
	if (m_ptr->mflag & (MFLAG_ALLY)) return (FALSE);
	
	/* Aggressive do attack player */
	if (m_ptr->mflag & (MFLAG_AGGR)) return (TRUE);
	
	/* Townsfolk don't attack player */
	if (m_ptr->mflag & (MFLAG_TOWN)) return (FALSE);
	
	/* Otherwise attack */
	return (TRUE);
}


/*
 * Does the monster attack another monster if they walk
 * into them?
 * 
 * Important: This routine must be symetric.
 * However, we make it asymetric for 'harmless' monsters.
 */
bool choose_to_attack_monster(const monster_type *m_ptr, const monster_type *n_ptr)
{
	/* Hallucination always results in a fight */
	if ((m_ptr->image) || (n_ptr->image)) return (TRUE);

	/* Allies and enemies of the player don't attack each other */
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) == ((n_ptr->mflag & (MFLAG_ALLY)) != 0)) return (FALSE);
	
	/* Aggressive enemies attack allies and vice versa - note 1st check ensures both sides are not friends */
	if (((m_ptr->mflag & (MFLAG_ALLY)) == 0) && ((n_ptr->mflag & (MFLAG_AGGR)) != 0)) return (TRUE);
	if (((n_ptr->mflag & (MFLAG_ALLY)) == 0) && ((m_ptr->mflag & (MFLAG_AGGR)) != 0)) return (TRUE);	
	
	/* Townsfolk don't attack each other */
	if (((m_ptr->mflag & (MFLAG_TOWN)) != 0) && ((n_ptr->mflag & (MFLAG_TOWN)) != 0)) return (FALSE);
	
	/* Allies don't attack townsfolk and vice versa*/
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && ((n_ptr->mflag & (MFLAG_TOWN)) != 0)) return (FALSE);
	if (((n_ptr->mflag & (MFLAG_ALLY)) != 0) && ((m_ptr->mflag & (MFLAG_TOWN)) != 0)) return (FALSE);
	
	/* Allies and townsfolk don't attack harmless monsters in towns*/
	if (((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN)) != 0) && (level_flag & (LF1_TOWN))
			&& (r_info[n_ptr->r_idx].blow[0].effect == GF_NOTHING)) return (FALSE);

	/* Otherwise attack */
	return (TRUE);
}



/*
 * Have a monster choose a spell.
 *
 * Monster at m_idx uses this function to select a legal attack spell.
 * Spell casting AI is based here.
 *
 * First the code will try to save time by seeing if
 * choose_attack_spell_fast is helpful.  Otherwise, various AI
 * parameters are used to calculate a 'desirability' for each spell.
 * There is some randomness.  The most desirable spell is cast.
 *
 * archery_only can be used to restrict us to arrow/boulder type attacks.
 *
 * Returns the spell number, of '0' if no spell is selected.
 *
 *-BR-
 *
 * byte choice
 * 0x01   Choose innate spells
 * 0x02   Choose casteable spells
 */
static int choose_ranged_attack(int m_idx, int *tar_y, int *tar_x, byte choose)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	const byte *spell_desire;

	u32b f4, f5, f6, f7;

	byte spell_range;

	int dist;

	bool do_random = FALSE;

	bool require_los = TRUE;

	/* Target the player by default unless an ally */
	int target_m_idx = m_ptr->mflag & (MFLAG_ALLY) ? 0 : -1;

	bool is_breath = FALSE;

	int i;
	int breath_hp, breath_maxhp, path, spaces;

	int want_hps=0, want_escape=0, want_mana=0, want_summon=0;
	int want_tactic=0, cur_range=0;

	int best_spell=0, best_spell_rating=0;
	int cur_spell_rating;

	bool assist = FALSE;

	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->flags5;
	f6 = r_ptr->flags6;
	f7 = r_ptr->flags7;

	/* Always permit huge blows */
	if (r_ptr->flags3 & (RF3_HUGE))
	{
		for (i = 0; i < 4; i++) if ((method_info[r_ptr->blow[i].method].flags2 & (PR2_RANGED)) == 0) f4 |= (1L << i);
	}

	/* Eliminate innate spells if not set */
	if ((choose & 0x01) == 0)
	{
		/* Remove ranged blows */
		if (((m_ptr->mflag & (MFLAG_SHOT)) == 0) && !(r_ptr->flags3 & (RF3_HUGE)))
		{
			f4 &= ~(0x0FL);
		}

		/* Remove other shot options */
		if ((m_ptr->mflag & (MFLAG_SHOT)) == 0)
		{
			f4 &= ~(0xF0L);
		}

		/* Remove breaths */
		if ((m_ptr->mflag & (MFLAG_BREATH)) == 0)
		{
			f4 &= ~(RF4_BREATH_MASK);
		}

		/* Remove other spells */
		f5 &= ~(RF5_INNATE_MASK);
		f6 &= ~(RF6_INNATE_MASK);
		f7 &= ~(RF7_INNATE_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* Eliminate other spells if not set */
	if (((choose & 0x02) == 0) && ((m_ptr->mflag & (MFLAG_CAST)) == 0))
	{
		f4 &= (RF4_INNATE_MASK);
		f5 &= (RF5_INNATE_MASK);
		f6 &= (RF6_INNATE_MASK);
		f7 &= (RF7_INNATE_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}
	/* Eliminate 'direct' spells when blinded, afraid or terrified (except innate spells) * */
	else if ((m_ptr->blind) || (m_ptr->monfear) || (m_ptr->terror))
	{
		f4 &= (rf4_no_player_mask | (m_ptr->terror ? 0 : RF4_INNATE_MASK) | RF4_SUMMON_MASK);
		f5 &= (RF5_NO_PLAYER_MASK | (m_ptr->terror ? 0 : RF5_INNATE_MASK) | RF4_SUMMON_MASK);
		f6 &= (RF6_NO_PLAYER_MASK | (m_ptr->terror ? 0 : RF6_INNATE_MASK) | RF6_SUMMON_MASK);
		f7 &= (RF7_NO_PLAYER_MASK | (m_ptr->terror ? 0 : RF7_INNATE_MASK) | RF7_SUMMON_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* If dazed, eliminate casting on self */
	if (m_ptr->dazed)
	{
		f4 &= ~(rf4_self_target_mask);
		f5 &= ~(RF5_SELF_TARGET_MASK);
		f6 &= ~(RF6_SELF_TARGET_MASK);
		f7 &= ~(RF7_SELF_TARGET_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* Eliminate all direct spells when player entering a level */
	if (p_ptr->dodging == 9)
	{
		f4 &= (rf4_no_player_mask | RF4_SUMMON_MASK);
		f5 &= (RF5_NO_PLAYER_MASK | RF4_SUMMON_MASK);
		f6 &= (RF6_NO_PLAYER_MASK | RF6_SUMMON_MASK);
		f7 &= (RF7_NO_PLAYER_MASK | RF7_SUMMON_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* Eliminate all summoning spells if monster has recently summoned or been summoned or an ally or townsfolk */
	if ((m_ptr->summoned) || ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN)) != 0))
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
		f7 &= ~(RF7_SUMMON_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* Allies / townsfolk do not teleport unless afraid or blink unless we need to reposition */
	if (m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN))
	{
		/* Prevent blinking unless target is at wrong range - note check to see if we can blink for efficiency */
		if (((f6 & (RF6_BLINK)) != 0) && (m_ptr->cdis >= MAX_SIGHT / 3) &&
			(!(m_ptr->ty) || !(m_ptr->tx) || (ABS(m_ptr->best_range - distance(m_ptr->fy, m_ptr->fx, m_ptr->ty, m_ptr->tx)) < 4)))
		{
			f6 &= ~(RF6_BLINK);
		}

		/* Prevent teleporting unless afraid or too far away */
		if ((!m_ptr->monfear) || (m_ptr->cdis >= MAX_SIGHT))
		{
			f6 &= ~(RF6_TPORT);
		}

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/*default: target the player*/
	*tar_y = p_ptr->py;
	*tar_x = p_ptr->px;
	dist = distance(m_ptr->fy, m_ptr->fx, *tar_y, *tar_x);

	/* Priests will assist others if they are not near death */
	if (((r_ptr->flags2 & (RF2_PRIEST)) != 0) && (m_ptr->hp > m_ptr->maxhp / 5))
	{
		assist |= (((f4 & (RF4_ASSIST_MASK)) != 0) || ((f5 & (RF5_ASSIST_MASK)) != 0) ||
					((f6 & (RF6_ASSIST_MASK)) != 0) || ((f7 & (RF7_ASSIST_MASK)) != 0));

		/* Priests switch to using spells on their allies one third of the time */
		if (((m_ptr->mflag & (MFLAG_ALLY | MFLAG_IGNORE)) == 0) && (m_idx % 3 == (turn / 10) % 9))
		{
			m_ptr->mflag |= (MFLAG_IGNORE);
		}
	}

	/*
	 * Is the monster an ally who can assist the player in
	 * a time of crisis?
	 */
	if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && (p_ptr->chp < p_ptr->mhp) && (assist)

		/* Ensure proximity */
		&& (m_ptr->cdis < MAX_RANGE) && ((m_ptr->mflag & (MFLAG_VIEW)) != 0)

		/* Hack -- to avoid call to random */
		&& ((m_idx + (turn / 10)) % 6 < 5 - (10 * p_ptr->chp / p_ptr->mhp)))
	{
		f4 &= (RF4_ASSIST_MASK);
		f5 &= (RF5_ASSIST_MASK);
		f6 &= (RF6_ASSIST_MASK);
		f7 &= (RF7_ASSIST_MASK);

		/* Remove teleport unless player is near death */
		if (p_ptr->chp > p_ptr->mhp / 10) f6 &= ~(RF6_TELE_TO);
	}

	/*
	 * Is monster an ally, or fighting an ally of the player?
	 *
	 * XXX Blind monsters can only cast spells at enemies if aggravated.
	 */
	else if (((m_ptr->mflag & (MFLAG_IGNORE | MFLAG_ALLY)) != 0) &&
		(!(m_ptr->blind) || ((m_ptr->mflag & (MFLAG_AGGR)) != 0)))
	{
		/* Attempt at efficiency */
		bool ally = ((m_ptr->mflag & (MFLAG_ALLY)) != 0);
		bool aggressive = ((m_ptr->mflag & (MFLAG_AGGR)) != 0) ;
		bool need_lite = ((r_ptr->flags2 & (RF2_NEED_LITE)) == 0);
		bool sneaking = p_ptr->sneaking || dist > MAX_SIGHT;

		/* Note we scale this up, and use a pseudo-random hack to try to get multiple monsters
		 * to favour different equi-distant enemies */
		int k = (ally ? MAX_SIGHT : dist) * 16 + 15;

		/* Note the player can set target_near in the targetting routine to force allies to consider
		 * targets closest to another monster or a point, as opposed to themselves. */
		int ny = (ally && ((p_ptr->target_set & (TARGET_NEAR)) != 0)) ? (p_ptr->target_who ? m_list[p_ptr->target_who].fy : p_ptr->target_row) : m_ptr->fy;
		int nx = (ally && ((p_ptr->target_set & (TARGET_NEAR)) != 0)) ? (p_ptr->target_who ? m_list[p_ptr->target_who].fx : p_ptr->target_col) : m_ptr->fx;

		/* Note the player can set target_race in the targetting routine to force allies to consider
		 * targets only of a particular race if they can see at least one of them. */
		bool force_one_race = FALSE;

		/* Check all other monsters */
		for (i = m_max - 1; i >= 1; i--)
		{
			/* Access the monster */
			monster_type *n_ptr = &m_list[i];

			/* Skip itself */
			if (i == m_idx) continue;

			/* Skip hidden targets */
			if (n_ptr->mflag & (MFLAG_HIDE)) continue;

			/* Monster has an enemy, or can assist an ally */
			if ((choose_to_attack_monster(m_ptr, n_ptr)) || ((assist) &&
				(((m_ptr->mflag & (MFLAG_ALLY)) != 0) == ((n_ptr->mflag & (MFLAG_ALLY)) != 0))))
			{
				bool see_target = aggressive;
				bool assist_ally = (((m_ptr->mflag & (MFLAG_ALLY)) != 0) == ((n_ptr->mflag & (MFLAG_ALLY)) != 0));

				/* XXX Note we prefer closer targets, however, reverse this for range 3 or less
				 * This discourages the monster hitting itself with ball spells */
				int d = (distance(n_ptr->fy, n_ptr->fx, ny, nx) * 16) + ((m_idx + i) % 16);

				/* Ignore targets out of range */
				if (d > MAX_RANGE * 16) continue;
				
				/* Prefer targets at about range 3 */
				if (d < 4 * 16) d = (6 * 16) - d;

				/*
				 * Check if monster can see the target. We make various assumptions about what
				 * monsters have a yet to be implemented SEE_INVIS flag, if the target is
				 * invisible. We check if the target is in light, if the monster needs light to
				 * see by.
				 */
				if (!see_target)
				{
					/* Monster is invisible */
					if ((r_info[n_ptr->r_idx].flags2 & (RF2_INVISIBLE)) || (n_ptr->tim_invis))
					{
						/* Cannot see invisible, or use infravision to detect the monster */
						if ((r_ptr->d_char != 'e') && ((r_ptr->flags9 & (RF9_RES_BLIND)) == 0)
								&& ((r_ptr->aaf < d / 16) || ((r_info[n_ptr->r_idx].flags2 & (RF2_COLD_BLOOD)) != 0))) continue;
					}

					/* Monster needs light to see */
					if (need_lite)
					{
						/* Check for light */
						if (((cave_info[n_ptr->fy][n_ptr->fx] & (CAVE_LITE)) == 0)
							&& ((play_info[n_ptr->fy][n_ptr->fx] & (PLAY_LITE)) == 0))
						{
							continue;
						}
					}

					/* Needs line of sight, and (hack) sometimes line of fire. */
					see_target = generic_los(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx, CAVE_XLOS |
							((m_idx + turn) % 2 ? CAVE_XLOF : 0));
				}
				/*
				 * Sometimes check for line of fire
				 */
				else if ((m_idx + turn) % 2)
				{
					see_target = generic_los(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx, CAVE_XLOF);
				}

				/* Ignore certain targets if sneaking */
				if (sneaking && !assist_ally)
				{
					/* Target is asleep - ignore */
					if (m_ptr->csleep) continue;

					/* Target not aggressive */
					if ((n_ptr->mflag & (MFLAG_AGGR)) == 0)
					{
						monster_race *s_ptr = &r_info[n_ptr->r_idx];

						/* I'm invisible and target can't see me - ignore */
						if ((r_ptr->flags2 & (RF2_INVISIBLE)) || (m_ptr->tim_invis))
						{
							/* Cannot see invisible, or use infravision to detect the monster */
							if ((s_ptr->d_char != 'e') && ((s_ptr->flags9 & (RF9_RES_BLIND)) == 0)
								&& ((s_ptr->aaf < d /16) || ((r_ptr->flags2 & (RF2_COLD_BLOOD)) != 0))) continue;
						}

						/* I'm in darkness and target can't see in darkness - ignore */
						if (s_ptr->flags2 & (RF2_NEED_LITE))
						{
							/* Check for light */
							if (((cave_info[n_ptr->fy][n_ptr->fx] & (CAVE_LITE)) == 0)
									&& ((play_info[n_ptr->fy][n_ptr->fx] & (PLAY_LITE)) == 0))
							{
								continue;
							}
						}
					}
				}

				/* Determine if we can help ally */
				if ((assist_ally) && (!m_ptr->image))
				{
					int d_base = MAX_SIGHT * 16 + ((m_idx + i) % 16);

					u32b tmpf4 = f4;
					u32b tmpf5 = f5;
					u32b tmpf6 = f6;
					u32b tmpf7 = f7;

					/* Remove useless spells - don't require that the monster is smart to keep below code simple */
					remove_useless_spells(i, &tmpf4, &tmpf5, &tmpf6, &tmpf7, TRUE);

					/* I'd benefit more from ignoring you and healing myself. */
					if (((tmpf6 & (RF6_HEAL | RF6_TELE_TO)) != 0) && (!m_ptr->dazed)) d_base += MAX_SIGHT * 8 - MAX_SIGHT * 8 * m_ptr->hp / m_ptr->maxhp;

					/* Determine if it is worth helping this monster */
					d = d_base;

					/* Target would benefit more from healing / buffing / teleporting away from danger as its hit points decrease. */
					if ((tmpf6 & (RF6_HEAL | RF6_HASTE | RF6_SHIELD | RF6_BLESS | RF6_TELE_TO)) != 0) d -= MAX_SIGHT * 8 * n_ptr->hp / n_ptr->maxhp;

					/* Don't bother buffing or curing badly hurt targets that we can't heal or teleport away */
					if (((tmpf6 & (RF6_HEAL | RF6_TELE_TO)) == 0) && (n_ptr->hp < n_ptr->maxhp / 2)) d += MAX_SIGHT * 12 - MAX_SIGHT * 24 * n_ptr->hp / n_ptr->maxhp;

					/* Benefit from hasting */
					if ((tmpf6 & (RF6_HASTE)) != 0)
					{
						int d_speed = d;
						if (m_ptr->hasted) d_speed = MIN(d, d_base / 2);
						if (n_ptr->slowed) d_speed -= 48;
						d = ((tmpf6 & ((RF6_ASSIST_MASK) & ~(RF6_HASTE))) != 0) ? MIN(d, d_speed) : d_speed;
					}

					/* Benefit from shielding */
					if ((tmpf6 & (RF6_SHIELD)) != 0)
					{
						int d_shield = d;
						if (m_ptr->shield) d_shield = MIN(d, d_base / 2);
						d = ((tmpf6 & ((RF6_ASSIST_MASK) & ~(RF6_SHIELD))) != 0) ? MIN(d, d_shield) : d_shield;
					}

					/* Benefit from elemental protection */
					if ((tmpf6 & (RF6_OPPOSE_ELEM)) != 0)
					{
						int d_oppose_elem = d;
						if (m_ptr->oppose_elem) d_oppose_elem = MIN(d, d_base / 2);
						d = ((tmpf6 & ((RF6_ASSIST_MASK) & ~(RF6_OPPOSE_ELEM))) != 0) ? MIN(d, d_oppose_elem) : d_oppose_elem;
					}

					/* Benefit from blessing */
					if ((tmpf6 & (RF6_BLESS)) != 0)
					{
						int d_bless = d;
						if (m_ptr->bless) d_bless = MIN(d, d_base / 2);
						d = ((tmpf6 & ((RF6_ASSIST_MASK) & ~(RF6_BLESS))) != 0) ? MIN(d, d_bless) : d_bless;
					}

					/* Benefit from curing */
					if ((tmpf6 & (RF6_CURE)) != 0)
					{
						int d_cure = d_base;
						d_cure -= n_ptr->stunned / 2;
						d_cure -= n_ptr->confused / 2;
						d_cure -= n_ptr->monfear / 2;
						d_cure -= n_ptr->cut / 2;
						d_cure -= n_ptr->poisoned / 2;
						d_cure -= n_ptr->mana ? n_ptr->blind : n_ptr->blind / 2;
						d_cure -= n_ptr->image;
						d_cure -= n_ptr->mana ? n_ptr->dazed : n_ptr->dazed / 2;
						d_cure -= n_ptr->mana ? n_ptr->amnesia : n_ptr->amnesia / 2;
						d_cure -= n_ptr->terror;
						d = MIN(d, MAX(d_cure, d_base - MAX_SIGHT * 4));
					}

					/* Not worth helping */
					if (d >= MAX_SIGHT * 16) continue;
				}

				/* Check if forcing a particular kind */
				if (ally && p_ptr->target_race && !assist_ally)
				{
					/* No match */
					if (p_ptr->target_race != n_ptr->r_idx)
					{
						/* Have already found a match */
						if (force_one_race) continue;
					}
					else if (!force_one_race)
					{
						force_one_race = TRUE;
						k = MAX_RANGE * 16;
						i = m_max;
					}
				}

				/* Pick a random target. Have checked for LOS already. */
				if ((see_target) && (d < k))
				{
					*tar_y = n_ptr->fy;
					*tar_x = n_ptr->fx;
					dist = distance(m_ptr->fy, m_ptr->fx, *tar_y, *tar_x);

					target_m_idx = i;

					k = d;
				}
			}
		}

		/* Important: Don't use healing attacks against enemies */
		if ((target_m_idx > 0) && ((r_info[m_list[target_m_idx].r_idx].flags3 & (RF3_DEMON)) != 0)) f5 &= ~(RF5_ARC_HFIRE);
	}

	/* Not necessarily assisting any more */
	assist = FALSE;

	/* Hack -- helping allies */
	if ((target_m_idx > 0) && (!m_ptr->image) && ((m_ptr->mflag & (MFLAG_ALLY)) != 0) == ((m_list[target_m_idx].mflag & (MFLAG_ALLY)) != 0))
	{
		/* Hack - cure hallucinating monsters or kill them */
		if (m_list[target_m_idx].image)
		{
			if (f6 & (RF6_CURE))
			{
				assist = TRUE;

				f4 = f5 = f7 = 0L;
				f6 = (RF6_CURE);
			}
		}
		/* Sometimes be selfish */
		else if (!(m_ptr->dazed) && ((m_idx + target_m_idx + (turn / 10)) % 2))
		{
			target_m_idx = 0;
		}
		/* Only allow spells which assist the target */
		else
		{
			/* Assisting */
			assist = TRUE;

			f4 &= (RF4_ASSIST_MASK);
			f5 &= (RF5_ASSIST_MASK);
			f6 &= (RF6_ASSIST_MASK);
			f7 &= (RF7_ASSIST_MASK);

			/* Always remove useless spells - ignore whether the caster is smart */
			remove_useless_spells(target_m_idx, &f4, &f5, &f6, &f7, TRUE);
		}
	}

	/* If dazed, eliminate assistance spells if casting on self */
	if ((m_ptr->dazed) && !(assist))
	{
		f4 &= ~(RF4_ASSIST_MASK);
		f5 &= ~(RF5_ASSIST_MASK);
		f6 &= ~(RF6_ASSIST_MASK);
		f7 &= ~(RF7_ASSIST_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* No valid target or targetting player and friendly/neutral and not aggressive. */
	if ((!target_m_idx) || ((target_m_idx < 0) && ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN)) != 0) && ((m_ptr->mflag & (MFLAG_AGGR)) == 0)))
	{
		f4 &= (rf4_no_player_mask);
		f5 &= (RF5_NO_PLAYER_MASK);
		f6 &= (RF6_NO_PLAYER_MASK);
		f7 &= (RF7_NO_PLAYER_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}
	else
	{
		/* Check what kinds of spells can hit target */
		path = projectable(m_ptr->fy, m_ptr->fx, *tar_y, *tar_x, PROJECT_CHCK);

		/* We do not have the target in sight at all */
		if (path == PROJECT_NO)
		{
			bool clear_ball_spell = TRUE;

			/* Are we in range and additionally smart or annoyed but not stupid?
				Have we got access to ball spells or summon spells? */
			if (dist <= MAX_RANGE + 1
				 && (r_ptr->flags2 & (RF2_SMART) ||
					  (m_ptr->mflag & (MFLAG_AGGR)
						&& !(r_ptr->flags2 & (RF2_STUPID))))
				 &&
				 (f4 & (rf4_ball_mask | RF4_SUMMON_MASK) ||
				  f5 & (RF5_BALL_MASK | RF5_SUMMON_MASK) ||
				  f6 & (RF6_BALL_MASK | RF6_SUMMON_MASK) ||
				  f7 & (RF7_BALL_MASK | RF7_SUMMON_MASK)))
			{
				int alt_y, alt_x, alt_path, best_y, best_x, best_path;

				/*start with no alternate shot*/
				best_y = best_x = best_path = 0;

				/* Check for impassable terrain */
				for (i = 0; i < 8; i++)
				{
					alt_y = p_ptr->py + ddy_ddd[i];
					alt_x = p_ptr->px + ddx_ddd[i];

					alt_path = projectable(m_ptr->fy, m_ptr->fx, alt_y, alt_x, PROJECT_CHCK);

					if (alt_path == PROJECT_NO)
						continue;

					if (alt_path == PROJECT_NOT_CLEAR)
					{
						if (!similar_monsters(m_ptr->fy, m_ptr->fx, alt_y, alt_x))
							continue;

						/*we already have a NOT_CLEAR path*/
						if ((best_path == PROJECT_NOT_CLEAR) && (rand_int(2)))
							continue;
					}

					/* The spot close to the target is out of range */
					if (distance(m_ptr->fy, m_ptr->fx, alt_y, alt_x) > MAX_RANGE)
						continue;

					/*
				 	 * PROJECT_NOT_CLEAR, or the monster has an
				 	 * empty square to lob a ball spell at target
				  	 */
					best_y = alt_y;
					best_x = alt_x;
					best_path = alt_path;
					/* we want to keep ball spells */
					clear_ball_spell = FALSE;

					if (best_path == PROJECT_CLEAR) break;
				}

				if (best_y + best_x > 0)
				{
					/* Set to best target */
					*tar_y = best_y;
					*tar_x = best_x;
					dist = distance(m_ptr->fy, m_ptr->fx, *tar_y, *tar_x);
				}
			}

			if (clear_ball_spell)
			{
				/* Flat out 75% chance of not casting any spell at all
					if the player is unreachable. In addition, most spells
					don't work without a player around. */
				if (rand_int(4)) return 0;

				/* We don't have a reason to try a ball spell
					To make summoning less annoying we also assume
					monster don't waste summons if player
					not even reachable by balls */
				f4 &= ~(rf4_ball_mask | RF4_SUMMON_MASK);
				f5 &= ~(RF5_BALL_MASK | RF5_SUMMON_MASK);
				f6 &= ~(RF6_BALL_MASK | RF6_SUMMON_MASK);
				f7 &= ~(RF7_BALL_MASK | RF7_SUMMON_MASK);
			}

			require_los = FALSE;
		}

		/* We assume all innate attacks have a hard maximum range */
		for (i = 0; i < 8; i++)
		{
			/* Out of range - eliminate spell */
			if (dist > spell_info_RF4[i][COL_SPELL_BEST_RANGE])
				f4 &= ~(RF4_BLOW_1 << i);
		}

		/* Hack -- remove beam spells 'by hand' */
		if (dist > 10) f5 &= ~(RF5_BEAM_NETHR | RF5_BEAM_ELEC);
		if (dist >  12) f5 &= ~(RF5_BEAM_ICE);

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);

		/* Remove spells the 'no-brainers'*/

		/* Remove all spells that require LOS
			Not MAX_RANGE + 1, because we filter out non-ball spells here */
		if (path == PROJECT_NO || dist > MAX_RANGE)
		{
			/* Ball spells and summon spells would have been
				filtered out above if not usable */
			f4 &= (rf4_no_player_mask | rf4_ball_mask | RF4_SUMMON_MASK);
			f5 &= (RF5_NO_PLAYER_MASK | RF5_BALL_MASK | RF5_SUMMON_MASK);
			f6 &= (RF6_NO_PLAYER_MASK | RF6_BALL_MASK | RF6_SUMMON_MASK);
			f7 &= (RF7_NO_PLAYER_MASK | RF7_BALL_MASK | RF7_SUMMON_MASK);
		}

		/* Remove only bolts and archery shots */
		else if (path == PROJECT_NOT_CLEAR)
		{
			f4 &= ~(rf4_bolt_mask);
			f4 &= ~(rf4_archery_mask);
			f5 &= ~(RF5_BOLT_MASK);
			f5 &= ~(RF5_ARCHERY_MASK);
			f6 &= ~(RF6_BOLT_MASK);
			f6 &= ~(RF6_ARCHERY_MASK);
			f7 &= ~(RF7_BOLT_MASK);
			f7 &= ~(RF7_ARCHERY_MASK);
		}

		/* Additional checks for allies */
		if (!(choose_to_attack_player(m_ptr)) && ((r_ptr->flags2 & (RF2_STUPID)) == 0))
		{
			/* Prevent ball spells if they could hit player */
			if ((player_can_fire_bold(*tar_y, *tar_x)) &&
					(((f4 & (rf4_ball_mask)) != 0) ||
					 ((f5 & (RF5_BALL_MASK)) != 0) ||
					 ((f6 & (RF6_BALL_MASK)) != 0) ||
					 ((f7 & (RF7_BALL_MASK)) != 0)))
			{
				int rad = (r_ptr->spell_power < 10 ? 2 : (r_ptr->spell_power < 40 ? 3 : (r_ptr->spell_power < 80 ? 4 : 5)));
				int dist2 = distance(*tar_y, *tar_x, p_ptr->py, p_ptr->px);

				/* Hack -- melee attacks */
				if (dist2 < 2)
				{
					f4 &= ~(rf4_ball_mask);
				}

				/* Check for spell range */
				if (dist2 < rad)
				{
					f4 &= ~(rf4_ball_mask);
					f5 &= ~(RF5_BALL_MASK);
					f6 &= ~(RF6_BALL_MASK);
					f7 &= ~(RF7_BALL_MASK);
				}

				/* Hack -- some balls are bigger */
				else if (dist2 == rad)
				{
					f5 &= ~(RF5_BALL_POIS | RF5_BALL_WIND | RF5_BALL_WATER);
				}
			}

			/* Prevent breath / arc weapons if player in line of breath */
			if ((player_can_fire_bold(m_ptr->fy, m_ptr->fx)) &&
					(((f4 & (RF4_BREATH_MASK | RF4_ARC_MASK)) != 0) ||
					((f5 & (RF5_BREATH_MASK | RF5_ARC_MASK)) != 0) ||
					((f6 & (RF6_BREATH_MASK | RF6_ARC_MASK)) != 0) ||
					((f7 & (RF7_BREATH_MASK | RF7_ARC_MASK)) != 0)))
			{
				int angle1 = get_angle_to_target(m_ptr->fy, m_ptr->fx, *tar_y, *tar_x, 0) * 2;
				int angle2 = get_angle_to_target(m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px, 0) * 2;

				int angle = (360 + angle1 - angle2) % 360;

				/* Check arcs */
				if (angle < 60)
				{
					f4 &= ~(RF4_ARC_MASK);
					f5 &= ~(RF5_ARC_MASK);
					f6 &= ~(RF6_ARC_MASK);
					f7 &= ~(RF7_ARC_MASK);
				}

				/* Check breaths */
				if (angle < ((r_ptr->flags2 & (RF2_POWERFUL)) ? 40 : 20))
				{
					f4 &= ~(RF4_BREATH_MASK);
					f5 &= ~(RF5_BREATH_MASK);
					f6 &= ~(RF6_BREATH_MASK);
					f7 &= ~(RF7_BREATH_MASK);
				}

				/* Hack -- certain breaths are wider */
				else if (angle < ((r_ptr->flags2 & (RF2_POWERFUL)) ? 50 : 30))
				{
					f4 &= ~(RF4_BRTH_POIS | RF4_BRTH_SOUND);
				}
			}
		}

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f7) return (0);
	}

	/* Spells we can not afford */
	remove_expensive_spells(m_idx, &f4, &f5, &f6, &f7);

	/* No spells left */
	if (!f4 && !f5 && !f6 && !f7) return (0);

	/* Stupid monsters choose at random. */
	if (r_ptr->flags2 & (RF2_STUPID)) return(pick_target(m_idx, tar_y, tar_x, choose_attack_spell_fast(m_idx, target_m_idx, &f4, &f5, &f6, &f7, TRUE)));

	/* Remove spells that have no benefit
	 * Does not include the effects of player resists/immunities */
	if (!assist) remove_useless_spells(m_idx, &f4, &f5, &f6, &f7, require_los);

	/* No spells left */
	if (!f4 && !f5 && !f6 && !f7) return (0);

	/* Sometimes non-dumb monsters cast randomly (though from the
	 * restricted list)
	 */
	if ((r_ptr->flags2 & (RF2_SMART)) && (!rand_int(10))) do_random = TRUE;
	if ((!(r_ptr->flags2 & (RF2_SMART))) && (!rand_int(5))) do_random = TRUE;

	/* Try 'fast' selection first.
	 * If there is only one spell, choose that spell.
	 * If there are multiple spells, choose one randomly if the 'random' flag is set.
	 * Otherwise fail, and let the AI choose.
	 */
	best_spell = choose_attack_spell_fast(m_idx, target_m_idx, &f4, &f5, &f6, &f7, do_random);
	if (best_spell) return (pick_target(m_idx, tar_y, tar_x, best_spell));

	/* If we get this far, we are using the full-up AI.  Calculate
	   some parameters. */

	/* Figure out if we are hurt */
	if (m_ptr->hp < m_ptr->maxhp/8) want_hps += 5;
	else if (m_ptr->hp < m_ptr->maxhp/5) want_hps += 3;
	else if (m_ptr->hp < m_ptr->maxhp/4) want_hps += 2;
	else if (m_ptr->hp < m_ptr->maxhp/2) want_hps++;
	else if (m_ptr->hp == m_ptr->maxhp) f6 &= ~(RF6_HEAL);

	/* Figure out if we want mana */
	if (m_ptr->mana < r_ptr->mana/4) want_mana +=2;
	else if (m_ptr->mana < r_ptr->mana/2) want_mana++;
	else if (m_ptr->mana == m_ptr->mana) f6 &= ~(RF6_ADD_MANA);

	/* Figure out if we want to scram */
	if (want_hps) want_escape = want_hps - 1;
	if (m_ptr->min_range == FLEE_RANGE) want_escape++;

	/* Desire to keep minimum distance */
	if (dist < m_ptr->min_range)
		want_tactic += (m_ptr->min_range - dist + 1) / 2;
	if (want_tactic > 3) want_tactic=3;

	/* Check terrain for purposes of summoning spells */
	spaces = summon_possible(m_ptr->fy, m_ptr->fx);

	if (spaces > 10) want_summon=5;
	else if (spaces > 3) want_summon=3;
	else if (spaces > 0) want_summon=1;
	else /*no spaces to summon*/
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
		f7 &= ~(RF7_SUMMON_MASK);
	}

	/* Check if no spells left */
	if (!f4 && !f5 && !f6 && !f7) return (0);

	/* Find monster properties; Add an offset so that things are OK near zero */
	breath_hp = (m_ptr->hp > 2000 ? m_ptr->hp : 2000);
	breath_maxhp = (m_ptr->maxhp > 2000 ? m_ptr->maxhp : 2000);

	/* Cheat if a player ghost. */
	if (0/* r_ptr->flags9 & (RF9_PLAYER_GHOST) */)
	{
		update_smart_cheat(m_idx);
	}
	/* Know player racial abilities if smart or a playable race */
	else if ((r_ptr->flags2 & (RF2_SMART)) || (r_ptr->flags3 & (RF3_ORC | RF3_TROLL)) ||
		(r_ptr->flags9 & (RF9_MAN | RF9_ELF | RF9_DWARF)) || (r_ptr->d_char == 'h'))
	{
		update_smart_racial(m_idx);
	}

	/* The conditionals are written for speed rather than readability
	 * They should probably stay that way. */
	for (i = 0; i < 128; i++)
	{
		/* Do we even have this spell? */
		if (i < 32)
		{
			if (!(f4 &(1L <<  i    ))) continue;
			spell_desire=&spell_desire_RF4[i][0];
			spell_range = spell_info_RF4[i][COL_SPELL_BEST_RANGE];
			if (RF4_BREATH_MASK &(1L << (i   ))) is_breath=TRUE;
			else is_breath=FALSE;
		}
		else if (i < 64)
		{
			if (!(f5 &(1L << (i-32)))) continue;
			spell_desire=&spell_desire_RF5[i-32][0];
			spell_range=spell_info_RF5[i-32][COL_SPELL_BEST_RANGE];
			if (RF5_BREATH_MASK &(1L << (i-32))) is_breath=TRUE;
			else is_breath=FALSE;
		}
		else if (i < 96)
		{
			if (!(f6 &(1L << (i-64)))) continue;
			spell_desire=&spell_desire_RF6[i-64][0];
			spell_range=spell_info_RF6[i-64][COL_SPELL_BEST_RANGE];
			if (RF6_BREATH_MASK &(1L << (i-64))) is_breath=TRUE;
			else is_breath=FALSE;
		}
		else
		{
			if (!(f7 &(1L << (i-96)))) continue;
			spell_desire=&spell_desire_RF7[i-96][0];
			spell_range=spell_info_RF7[i-96][COL_SPELL_BEST_RANGE];
			if (RF7_BREATH_MASK &(1L << (i-96))) is_breath=TRUE;
			else is_breath=FALSE;
		}

		/* Base Desirability*/
		cur_spell_rating = spell_desire[D_BASE];

		/* modified for breath weapons */
		if (is_breath) cur_spell_rating = (cur_spell_rating * breath_hp) / breath_maxhp;

		/* Bonus if want summon and this spell is helpful */
		if (spell_desire[D_SUMM] && want_summon) cur_spell_rating +=
						      want_summon * spell_desire[D_SUMM];

		/* Bonus if wounded and this spell is helpful */
		if (spell_desire[D_HURT] && want_hps) cur_spell_rating +=
							want_hps * spell_desire[D_HURT];

		/* Bonus if low on mana and this spell is helpful */
		if (spell_desire[D_MANA] && want_mana) cur_spell_rating +=
							 want_mana * spell_desire[D_MANA];

		/* Bonus if want to flee and this spell is helpful */
		if (spell_desire[D_ESC] && want_escape) cur_spell_rating +=
							  want_escape * spell_desire[D_ESC];

		/* Bonus if want a tactical move and this spell is helpful */
		if (spell_desire[D_TACT] && want_tactic) cur_spell_rating +=
							   want_tactic * spell_desire[D_TACT];

		/* Penalty if this spell is resisted */
		if (spell_desire[D_RES])
		      cur_spell_rating = (cur_spell_rating * (100 - find_resist(target_m_idx > 0 ? monster_smart_flags(target_m_idx) : m_ptr->smart, spell_desire[D_RES])))/100;

		/* Penalty for range if attack drops off in power */
		if (spell_range)
		{
			cur_range = dist;
			while (cur_range-- > spell_range)
				cur_spell_rating = (cur_spell_rating * spell_desire[D_RANGE])/100;
		}

		/* Random factor; less random for smart monsters */
		if (r_ptr->flags2 & (RF2_SMART)) cur_spell_rating *= 16 + rand_int(100);
		else cur_spell_rating *= 12 + rand_int(50);

		/* Deflate for testing purposes */
		cur_spell_rating /= 20;

		/* Is this the best spell yet?, or alternate between equal spells*/
		if ((cur_spell_rating > best_spell_rating) ||
			((cur_spell_rating == best_spell_rating) && rand_int(2)))
		{
			best_spell_rating = cur_spell_rating;
			best_spell = i + 96;
		}
	}

	if (p_ptr->wizard)
	{
		msg_format("Spell rating: %i.", best_spell_rating);
	}

	/* Return Best Spell */
	return (pick_target(m_idx, tar_y, tar_x, best_spell));
}




/*
 * Can the monster exist in this grid?
 */
bool cave_exist_mon(int r_idx, int y, int x, bool occupied_ok)
{
	/* Check Bounds */
	if (!in_bounds(y, x)) return (FALSE);

	/* The grid is already occupied. */
	if (cave_m_idx[y][x] != 0)
	{
		if (!occupied_ok) return (FALSE);
	}

	/* Check passability and survivability of features */
	if (place_monster_here(y,x,r_idx) > MM_FAIL) return (TRUE);

	/* Catch weirdness */
	return (FALSE);
}


/*
 * Check if an illusion hides danger beneath
 */
bool region_illusion_hook(int y, int x, s16b d, s16b region)
{
	region_type *r_ptr = &region_list[region];

	(void)x; (void)y; (void)d;

	return (r_ptr->effect == GF_ILLUSION);
}


/*
 * Can the monster enter this grid?  How easy is it for them to do so?
 *
 * The code that uses this function sometimes assumes that it will never
 * return a value greater than 100.
 *
 * The usage of exp to determine whether one monster can kill another is
 * a kludge.  Maybe use HPs, plus a big bonus for acidic monsters
 * against monsters that don't like acid.
 *
 * The usage of exp to determine whether one monster can push past
 * another is also a tad iffy, but ensures that black orcs can always
 * push past other black orcs.
 */
static int cave_passable_mon(monster_type *m_ptr, int y, int x, bool *bash)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	/* Assume nothing in the grid other than the terrain hinders movement */
	int move_chance = 100;

	int mmove;



	/* Check Bounds */
	if (!in_bounds(y, x)) return (FALSE);

	/* Haven't had to bash it */
	*bash = FALSE;

	/* The grid is occupied by the player. */
	if (cave_m_idx[y][x] < 0)
	{
		/* Monster has no melee blows - character's grid is off-limits. */
		if (r_ptr->flags1 & (RF1_NEVER_BLOW)) return (0);

		/* Monster is an ally or townsfolk - character's grid is off-limits */
		else if (!choose_to_attack_player(m_ptr)) return (0);
		
		/* Any monster with melee blows can attack the character. */
		else move_chance = 100;
	}

	/* The grid is occupied by a monster. */
	else if (cave_m_idx[y][x] > 0)
	{
		monster_type *n_ptr = &m_list[cave_m_idx[y][x]];
		monster_race *nr_ptr = &r_info[n_ptr->r_idx];

		/* Enemies can always attack */
		if (choose_to_attack_monster(m_ptr, n_ptr))
		{
			/* Can always attack */
			return (100);
		}

		/* Guardians can always push through other monsters, except other guardians */
		else if ((r_ptr->flags1 & (RF1_GUARDIAN)) && ((nr_ptr->flags1 & (RF1_GUARDIAN)) == 0))
		{
			/* Can always push at full speed. */
			move_chance = 100;
		}

		/* Pushed already */
		else if ((m_ptr->mflag & (MFLAG_PUSH)) || (n_ptr->mflag & (MFLAG_PUSH)))
		{
			/* Cannot push away the other monster */
			return (0);
		}

		/* Push past weaker or similar monsters */
		else if (r_ptr->mexp >= nr_ptr->mexp)
		{
			/* It's easier to push past weaker monsters */
			if (r_ptr->mexp == nr_ptr->mexp) move_chance = 40;
			else move_chance = 80;
		}

		/* Push past flying monsters or if flying */
		else if ((m_ptr->mflag & (MFLAG_OVER)) ||
		    (n_ptr->mflag & (MFLAG_OVER)))
		{
			move_chance = 80;
		}

		/* Push past hidden monsters */
		else if ((m_ptr->mflag & (MFLAG_HIDE)) ||
		    (n_ptr->mflag & (MFLAG_HIDE)))
		{
			move_chance = 80;
		}

		/* Push past if fleeing, or target fleeing, but not both */
		else if (((m_ptr->monfear) ||
		    (n_ptr->monfear)) && !(m_ptr->monfear && (n_ptr->monfear)))
		{
			move_chance = 80;
		}

		/* Cannot do anything to clear away the other monster */
		else return (0);

		/* Attempt to move around monsters in combat with player, instead of pushing through them */
		if ((move_chance < 100) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0) && (n_ptr->cdis == 1))
		{
			return (0);
		}
	}

	/* Hack -- avoid less interesting squares if collecting items */
	else if ((cave_o_idx[y][x] == 0) && (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_EAT_BODY)))
	{
		move_chance = 99;
	}

	/* Paranoia -- move_chance must not be more than 100 */
	if (move_chance > 100) move_chance = 100;

	/* Check how we move */
	mmove = place_monster_here(y,x,m_ptr->r_idx);


	/*** Check passability of various features. ***/

	/* The monster is under covered terrain, moving to uncovered terrain. */
	if ((m_ptr->mflag & (MFLAG_HIDE)) && (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_COVERED)) &&
		!(f_ptr->flags2 & (FF2_COVERED)) && (mmove != MM_SWIM) && (mmove != MM_DIG) && (mmove != MM_UNDER))
	{

		if ((r_ptr->flags2 & (RF2_BASH_DOOR)) &&  (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags1 & (FF1_BASH)))
		{
			*bash = TRUE;
		}
		else
		{
			move_chance = 0;
		}

		return (move_chance);

	}

	/* Feature is passable or damaging */
	else if (mmove != MM_FAIL)
	{
		/* Hack -- allow incautious enemies to think they can move safely on illusory terrain */
		if (((r_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND)) == 0) && ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_SMART)) == 0)
				&& (region_grid(y, x, region_illusion_hook)))
		{
			if ((mmove == MM_TRAP) || (mmove == MM_DROWN)) mmove = MM_WALK;
		}

		/* Do not kill ourselves in terrain unless confused */
		if (mmove == MM_DROWN && !m_ptr->confused)
		{
			/* Try to get out of existing trouble */
            if (place_monster_here(m_ptr->fy, m_ptr->fx, m_ptr->r_idx) <= MM_DROWN) move_chance /= 2;

			/* Don't walk into trouble */
			else move_chance = 0;
		}

		/* We cannot natively climb, but are negotiating a tree or rubble */
		else if (mmove == MM_CLIMB && !(r_ptr->flags2 & (RF2_CAN_CLIMB)))
		{
			move_chance /= 2;
		}

		/* Check for traps */
		else if (mmove == MM_TRAP)
		{
			/* Stupid monsters ignore traps */
			if ((r_ptr->flags2 & (RF2_STUPID)) || (m_ptr->mflag & (MFLAG_STUPID)))
			{
				/* No modification */
			}

			/* Smart and sneaky monsters will disarm traps. */
			else if ((r_ptr->flags2 & (RF2_SMART | RF2_SNEAKY)) && (f_ptr->flags1 & (FF1_DISARM)))
			{
				move_chance /= 2;
			}

			/* Allies will avoid traps to avoid looking stupid/frustrating the player */
			else if ((m_ptr->mflag & (MFLAG_ALLY)) != 0)
			{
				move_chance = 0;
			}
			
			/* Monsters in line of fire to the player will consider moving
			 * through traps, as will those monsters aggravated by the player. */
			else if	((play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE)) ||
					(m_ptr->mflag & (MFLAG_AGGR)))
			{
				move_chance /= 4;
			}

			/* Don't move through traps unnecessarily */
			else
			{
				move_chance = 0;
			}
		}

		/* Anything else that's not a wall we assume to be passable. */
		return (move_chance);
	}

	/* Feature is a wall */
	else
	{
		int unlock_chance = 0;
		int bash_chance = 0;

		/* Glyphs */
		if (f_ptr->flags1 & (FF1_GLYPH))
		{
			/* Glyphs are hard to break */
			return (MIN(100 * r_ptr->level / BREAK_GLYPH, move_chance));
		}

		/* Monster can open doors */
		if (f_ptr->flags1 & (FF1_SECRET))
		{
				/* Discover the secret (temporarily) */
				f_ptr = &f_info[feat_state(cave_feat[y][x],FS_SECRET)];
		}

		/* Monster can open doors */
		if ((r_ptr->flags2 & (RF2_OPEN_DOOR)) && (f_ptr->flags1 & (FF1_OPEN)))
		{
			/* Secret doors and easily opened stuff */
			if (f_ptr->power == 0)
			{
				/*
				 * Note:  This section will have to be rewritten if
				 * secret doors can be jammed or locked as well.
				 */


				/*
				 * It usually takes two turns to open a door
				 * and move into the doorway.
				 */
				return (MIN(50, move_chance));
			}

			/*
			 * Locked doors (not jammed).  Monsters know how hard
			 * doors in their neighborhood are to unlock.
			 */
			else
			{
				int lock_power, ability;

				/* Door power (from 35 to 245) */
				lock_power = 35 * f_ptr->power;

				/* Calculate unlocking ability (usu. 11 to 200) */
				ability = r_ptr->level + 10;
				if (r_ptr->flags2 & (RF2_SMART)) ability *= 2;
				if (strchr("ph", r_ptr->d_char))
					ability = 3 * ability / 2;

				/*
				 * Chance varies from 5% to over 100%.  XXX XXX --
				 * we ignore the fact that it takes extra time to
				 * open the door and walk into the entranceway.
				 */
				unlock_chance = (MAX(5, (100 * ability / lock_power)));
			}
		}

		/* Monster can bash doors */
		if ((r_ptr->flags2 & (RF2_BASH_DOOR)) && (f_ptr->flags1 & (FF1_BASH)))
		{
			int door_power, bashing_power;

			/* Door power (from 60 to 420) */
			/*
			 * XXX - just because a door is difficult to unlock
			 * shouldn't mean that it's hard to bash.  Until the
			 * character door bashing code is changed, however,
			 * we'll stick with this.
			 */
			door_power = 60 + 60 * f_ptr->power;

			/*
			 * Calculate bashing ability (usu. 21 to 300).  Note:
			 * This formula assumes Oangband-style HPs.
			 */
			bashing_power = 20 + r_ptr->level + m_ptr->hp / 15;

			if ((r_ptr->flags3 & (RF3_GIANT)) || (r_ptr->flags3 & (RF3_TROLL)))
				bashing_power = 3 * bashing_power / 2;

			/*
			 * Chance varies from 2% to over 100%.  Note that
			 * monsters "fall" into the entranceway in the same
			 * turn that they bash the door down.
			 */
			bash_chance = (MAX(2, (100 * bashing_power / door_power)));
		}

		/*
		 * A monster cannot both bash and unlock a door in the same
		 * turn.  It needs to pick one of the two methods to use.
		 */
		if (unlock_chance > bash_chance) *bash = FALSE;
		else *bash = TRUE;

		return MIN(move_chance, (MAX(unlock_chance, bash_chance)));
	}

	/* Any wall grid that isn't explicitly made passible is impassible. */
	return (0);
}


/*
 * Get a target for a monster using the special "townsman" AI.
 */
static void get_town_target(monster_type *m_ptr)
{
	int i, feat;
	int y, x;

	/* Clear target */
	m_ptr->ty = 0;
	m_ptr->tx = 0;

	/* Hack -- Usually choose a random store */
	if (((level_flag & (LF1_TOWN)) == 0) && (rand_int(100) < 80))
	{
		i = t_info[p_ptr->town].store[rand_int(8)];

		/* Try to find the store XXX XXX */
		if (i) for (y = 1; y < TOWN_HGT - 2; y++)
		{
			for (x = 1; x < TOWN_WID - 2; x++)
			{
				feat = cave_feat[y][x];

				/* Is our store */
				if (feat == i)
				{
					m_ptr->ty = y;
					m_ptr->tx = x;
					break;
				}
			}
		}
	}

	/* No store chosen */
	if (!m_ptr->ty)
	{
		for (i = 0;; i++)
		{
			/* Pick a grid on the edge of the map (simple test) */
			if (((level_flag & (LF1_TOWN)) != 0) && (i < 100))
			{
				if (rand_int(2))
				{
					/* Pick a random location along the N/S walls */
					x = rand_range(1, TOWN_WID - 2);

					if (rand_int(2)) y = 1;
					else            y = TOWN_HGT - 2;
				}
				else
				{
					/* Pick a random location along the E/W walls */
					y = rand_range(1, TOWN_HGT - 2);

					if (rand_int(2)) x = 1;
					else            x = TOWN_WID - 2;
				}
			}
			else
			{
				y = rand_range(1, ((level_flag & (LF1_TOWN)) ? TOWN_HGT : DUNGEON_HGT) - 2);
				x = rand_range(1, ((level_flag & (LF1_TOWN)) ? TOWN_WID : DUNGEON_WID) - 2);

				/* Ensure pick in town */
				if (!room_near_has_flag(y, x, ROOM_TOWN)) continue;
			}

			/* Require a grid we can get to */
			if (!mon_resist_feat(f_info[cave_feat[y][x]].mimic, m_ptr->r_idx)) continue;

			/* Require "empty" floor grids */
			if (cave_empty_bold(y, x))
			{
				m_ptr->ty = y;
				m_ptr->tx = x;
				break;
			}
		}
	}
}



/*
 * Helper function for monsters that want to advance toward the character.
 * Assumes that the monster isn't frightened, and is not in LOS of the
 * character.
 *
 * Ghosts and rock-eaters do not use flow information, because they
 * can - in general - move directly towards the character.  We could make
 * them look for a grid at their preferred range, but the character
 * would then be able to avoid them better (it might also be a little
 * hard on those poor warriors...).
 *
 * Other monsters will use target information, then their ears, then their
 * noses (if they can), and advance blindly if nothing else works.
 *
 * When flowing, monsters prefer non-diagonal directions.
 *
 * XXX - At present, this function does not handle difficult terrain
 * intelligently.  Monsters using flow may bang right into a door that
 * they can't handle.  Fixing this may require code to set monster
 * paths.
 */
static void get_move_advance(int m_idx, int *ty, int *tx)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, y, x, y1, x1;

	int lowest_cost = 250;

	bool use_sounds = FALSE;
	bool use_scent = FALSE;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Monster can go through rocks - head straight for character */
	if ((r_ptr->flags2 & (RF2_PASS_WALL)) || (m_ptr->tim_passw > 10) ||
	   (r_ptr->flags2 & (RF2_KILL_WALL)))
	{
		*ty = py;
		*tx = px;
		return;
	}

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Use target information if available */
	if ((m_ptr->ty) && (m_ptr->tx))
	{
		*ty = m_ptr->ty;
		*tx = m_ptr->tx;
		return;
	}

	/* If we can hear noises, advance towards them */
	if (cave_cost[y1][x1])
	{
		use_sounds = TRUE;
	}

	/* Otherwise, try to follow a scent trail */
	else if (monster_can_smell(m_ptr))
	{
		use_scent = TRUE;
	}

	/* Otherwise, advance blindly */
	if ((!use_sounds) && (!use_scent))
	{
		*ty = py;
		*tx = px;
		return;
	}

	/* Using flow information.  Check nearby grids, diagonals first. */
	for (i = 7; i >= 0; i--)
	{
		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Check Bounds */
		if (!in_bounds(y, x)) continue;

		/* We're following a scent trail */
		if (use_scent)
		{
			int age = get_scent(y, x);
			if (age == -1) continue;

			/* Accept younger scent */
			if (lowest_cost < age) continue;
			lowest_cost = age;
		}

		/* We're using sound */
		else
		{
			int cost = cave_cost[y][x];

			/* Accept louder sounds */
			if ((cost == 0) || (lowest_cost < cost)) continue;
			lowest_cost = cost;
		}

		/* Save the location */
		*ty = y;
		*tx = x;
	}
}


/*
 * "Do not be seen."
 *
 * Monsters in LOS that want to retreat are primarily interested in
 * finding a nearby place that the character can't see into.
 * Search for such a place with the lowest cost to get to up to 15
 * grids away.
 *
 * Look outward from the monster's current position in a square-
 * shaped search pattern.  Calculate the approximate cost in monster
 * turns to get to each passable grid, using a crude route finder.  Penal-
 * ize grids close to or approaching the character.  Ignore hiding places
 * with no safe exit.  Once a passable grid is found that the character
 * can't see, the code will continue to search a little while longer,
 * depending on how pricey the first option seemed to be.
 *
 * If the search is successful, the monster will target that grid,
 * and (barring various special cases) run for it until it gets there.
 *
 * We use a limited waypoint system (see function "get_route_to_target()"
 * to reduce the likelihood that monsters will get stuck at a wall between
 * them and their target (which is kinda embarrassing...).
 *
 * This function does not yield perfect results; it is known to fail
 * in cases where the previous code worked just fine.  The reason why
 * it is used is because its failures are less common and (usually)
 * less embarrassing than was the case before.  In particular, it makes
 * monsters great at not being seen.
 *
 * This function is fairly expensive.  Call it only when necessary.
 */
static bool find_safety(monster_type *m_ptr, int *ty, int *tx)
{
	int i, j, d;

	/* Scanning range for hiding place search. */
	byte scan_range = 15;

	int y, x, yy, xx;

	int countdown = scan_range;

	int least_cost = 100;
	int least_cost_y = 0;
	int least_cost_x = 0;
	int chance, cost, parent_cost;
	bool dummy;

	/* Factors for converting table to actual dungeon grids */
	int conv_y, conv_x;

	/*
	 * Allocate and initialize a table of movement costs.
	 * Both axis must be (2 * scan_range + 1).
	 */
	byte safe_cost[31][31];

	for (i = 0; i < 31; i++)
	{
		for (j = 0; j < 31; j++)
		{
			safe_cost[i][j] = 0;
		}
	}

	conv_y = scan_range - m_ptr->fy;
	conv_x = scan_range - m_ptr->fx;

	/* Mark the origin */
	safe_cost[scan_range][scan_range] = 1;

	/* If the character's grid is in range, mark it as being off-limits */
	if ((ABS(m_ptr->fy - p_ptr->py) <= scan_range) &&
	    (ABS(m_ptr->fx - p_ptr->px) <= scan_range))
	{
		safe_cost[p_ptr->py + conv_y][p_ptr->px + conv_x] = 100;
	}

	/* Work outward from the monster's current position */
	for (d = 0; d < scan_range; d++)
	{
		for (y = scan_range - d; y <= scan_range + d; y++)
		{
			for (x = scan_range - d; x <= scan_range + d;)
			{
				int x_tmp;

				/*
				 * Scan all grids of top and bottom rows, just
				 * outline other rows.
				 */
				if ((y != scan_range - d) && (y != scan_range + d))
				{
					if (x == scan_range + d) x_tmp = 999;
					else x_tmp = scan_range + d;
				}
				else x_tmp = x + 1;

				/* Grid and adjacent grids must be legal */
				if (!in_bounds_fully(y - conv_y, x - conv_x))
				{
					x = x_tmp;
					continue;
				}

				/* Grid is inaccessable (or at least very difficult to enter) */
				if ((safe_cost[y][x] == 0) || (safe_cost[y][x] >= 100))
				{
					x = x_tmp;
					continue;
				}

				/* Get the accumulated cost to enter this grid */
				parent_cost = safe_cost[y][x];

				/* Scan all adjacent grids */
				for (i = 0; i < 8; i++)
				{
					yy = y + ddy_ddd[i];
					xx = x + ddx_ddd[i];

					/* check bounds */
					if ((yy < 0) || (yy > 30) || (xx < 0) || (xx > 30)) continue;

					/*
					 * Handle grids with empty cost and passable grids
					 * with costs we have a chance of beating.
					 */
					if ((safe_cost[yy][xx] == 0) ||
					      ((safe_cost[yy][xx] > parent_cost + 1) &&
					       (safe_cost[yy][xx] < 100)))
					{
						/* Get the cost to enter this grid */
						chance = cave_passable_mon(m_ptr, yy - conv_y,
							 xx - conv_x, &dummy);

						/* Impassable */
						if (!chance)
						{
							/* Cannot enter this grid */
							safe_cost[yy][xx] = 100;
							continue;
						}

						/* Calculate approximate cost (in monster turns) */
						cost = 100 / chance;

						/* Next to character */
						if (distance(yy - conv_y, xx - conv_x,
						    p_ptr->py, p_ptr->px) <= 1)
						{
							/* Don't want to maneuver next to the character */
							cost += 3;
						}

						/* Mark this grid with a cost value */
						safe_cost[yy][xx] = parent_cost + cost;

						/* Character can't see this grid */
						if (!player_can_see_bold(yy - conv_y, xx - conv_x))
						{
							int this_cost = safe_cost[yy][xx];

							/* Penalize grids that approach character */
							if (ABS(p_ptr->py - (yy - conv_y)) <
							    ABS(m_ptr->fy - (yy - conv_y)))
							{
								 this_cost *= 2;
							}
							if (ABS(p_ptr->px - (xx - conv_x)) <
							    ABS(m_ptr->fx - (xx - conv_x)))
							{
								 this_cost *= 2;
							}

							/* Accept lower-cost, sometimes accept same-cost options */
							if ((least_cost > this_cost) ||
							    (least_cost == this_cost && rand_int(2) == 0))
							{
								bool has_escape = FALSE;

								/* Scan all adjacent grids for escape routes */
								for (j = 0; j < 8; j++)
								{
									/* Calculate real adjacent grids */
									int yyy = yy - conv_y + ddy_ddd[i];
									int xxx = xx - conv_x + ddx_ddd[i];

									/* Check bounds */
									if (!in_bounds(yyy, xxx)) continue;

									/* Look for any passable grid that isn't in LOS */
									if ((!player_can_see_bold(yyy, xxx)) &&
									    (cave_passable_mon(m_ptr, yyy, xxx, &dummy)))
									{
										/* Not a one-grid cul-de-sac */
										has_escape = TRUE;
										break;
									}
								}

								/* Ignore cul-de-sacs */
								if (has_escape == FALSE) continue;

								least_cost = this_cost;
								least_cost_y = yy;
								least_cost_x = xx;

								/*
								 * Look hard for alternative
								 * hiding places if this one
								 * seems pricey.
								 */
								countdown = 1 + least_cost - d;
							}
						}
					}
				}

				/* Adjust x as instructed */
				x = x_tmp;
			}
		}

		/*
		 * We found a good place a while ago, and haven't done better
		 * since, so we're probably done.
		 */
		if (countdown-- == 0) break;
	}

	/* We found a place that can be reached in reasonable time */
	if (least_cost < 50)
	{
		/* Convert to actual dungeon grid. */
		y = least_cost_y - conv_y;
		x = least_cost_x - conv_x;

		/* Move towards the hiding place */
		*ty = y;
		*tx = x;

		/* Target the hiding place */
		m_ptr->ty = y;
		m_ptr->tx = x;

		return (TRUE);
	}


	/* No good place found */
	return (FALSE);
}


/*
 * The monster either surrenders or turns to fight
 */
static void monster_surrender_or_fight(int m_idx)
{
	char m_name[80];
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Cancel fear */
	set_monster_fear(m_ptr, 0, FALSE);

	/* Forget target */
	m_ptr->ty = 0;    m_ptr->tx = 0;

	/* Charge!  XXX XXX */
	m_ptr->min_range = 1;  m_ptr->best_range = 1;

	/* Visible */
	if (m_ptr->ml)
	{
		/* Get the monster name */
		monster_desc(m_name, sizeof(m_name), m_idx, 0);
	
		/* Chance to surrender */
		if (!(birth_evil) && ((r_ptr->flags2 & (RF2_STUPID)) == 0)
			&& (rand_int(adj_chr_stock[p_ptr->stat_ind[A_CHR]]) < (r_ptr->flags2 & (RF2_SNEAKY) ? 20 : 10)))
		{
			/* Dump a message */
			msg_format("%^s surrenders!", m_name);
			
			/* Stop attacking player */
			m_ptr->mflag |= (MFLAG_TOWN);
			m_ptr->mflag &= ~(MFLAG_AGGR);
			
			/* For a while */
			m_ptr->summoned = 400 - 3 * adj_chr_gold[p_ptr->stat_ind[A_CHR]];
			
			/* Sneaky / evil */
			if (r_ptr->flags2 & (RF2_SNEAKY)) m_ptr->summoned /= 2;
			if (r_ptr->flags3 & (RF3_EVIL)) m_ptr->summoned /= 3;					
		}
		else
		{
			/* Dump a message */
			msg_format("%^s turns to fight!", m_name);
		}
	}
}


/*
 * Helper function for monsters that want to retreat from the character.
 * Used for any monster that is terrified, frightened, is looking for a
 * temporary hiding spot, or just wants to open up some space between it
 * and the character.
 *
 * If the monster is well away from danger, let it relax.
 * If the monster's current target is not in LOS, use it (+).
 * If the monster is not in LOS, and cannot pass through walls, try to
 * use flow (noise) information.
 * If the monster is in LOS, even if it can pass through walls,
 * search for a hiding place (helper function "find_safety()").
 * If no hiding place is found, and there seems no way out, go down
 * fighting.
 *
 * If none of the above solves the problem, run away blindly.
 *
 * (+) There is one exception to the automatic usage of a target.  If the
 * target is only out of LOS because of "knight's move" rules (distance
 * along one axis is 2, and along the other, 1), then the monster will try
 * to find another adjacent grid that is out of sight.  What all this boils
 * down to is that monsters can now run around corners properly!
 *
 * Return TRUE if the monster did actually want to do anything.
 */
static bool get_move_retreat(int m_idx, int *ty, int *tx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int i;
	int y, x;

	bool done = FALSE;
	bool dummy;


	/* If the monster is well away from danger, let it relax. */
	if (m_ptr->cdis >= FLEE_RANGE)
	{
		return (FALSE);
	}

	/* Monster has a target */
	if ((m_ptr->ty) && (m_ptr->tx))
	{
		/* It's out of LOS; keep using it, except in "knight's move" cases */
		if (!player_has_los_bold(m_ptr->ty, m_ptr->tx))
		{
			/* Get axis distance from character to current target */
			int dist_y = ABS(p_ptr->py - m_ptr->ty);
			int dist_x = ABS(p_ptr->px - m_ptr->tx);


			/* It's only out of LOS because of "knight's move" rules */
			if (((dist_y == 2) && (dist_x == 1)) ||
			    ((dist_y == 1) && (dist_x == 2)))
			{
				/*
				 * If there is another grid adjacent to the monster that
				 * the character cannot see into, and it isn't any harder
				 * to enter, use it instead.  Prefer diagonals.
				 */
				for (i = 7; i >= 0; i--)
				{
					y = m_ptr->fy + ddy_ddd[i];
					x = m_ptr->fx + ddx_ddd[i];

					/* Check Bounds */
					if (!in_bounds(y, x)) continue;

					if (player_has_los_bold(y, x)) continue;

					if ((y == m_ptr->ty) && (x == m_ptr->tx)) continue;

					if (cave_passable_mon(m_ptr, m_ptr->ty, m_ptr->tx, &dummy) >
					    cave_passable_mon(m_ptr, y, x, &dummy)) continue;

					m_ptr->ty = y;
					m_ptr->tx = x;
					break;
				}
			}

			/* Move towards the target */
			*ty = m_ptr->ty;
			*tx = m_ptr->tx;
			return (TRUE);
		}

		/* It's in LOS; cancel it. */
		else
		{
			m_ptr->ty = 0;
			m_ptr->tx = 0;
		}
	}

	/* The monster is not in LOS, but thinks it's still too close. */
	if (!player_has_los_bold(m_ptr->fy, m_ptr->fx))
	{
		/* Monster cannot pass through walls */
		if (!((r_ptr->flags2 & (RF2_PASS_WALL)) || (m_ptr->tim_passw > 10) ||
	      	 (r_ptr->flags2 & (RF2_KILL_WALL))))
		{
			/* Run away from noise */
			if (cave_cost[m_ptr->fy][m_ptr->fx])
			{
				int start_cost = cave_cost[m_ptr->fy][m_ptr->fx];

				/* Look at adjacent grids, diagonals first */
				for (i = 7; i >= 0; i--)
				{
					y = m_ptr->fy + ddy_ddd[i];
					x = m_ptr->fx + ddx_ddd[i];

					/* Check Bounds */
					if (!in_bounds(y, x)) continue;

					/* Accept the first non-visible grid with a higher cost */
					if (cave_cost[y][x] > start_cost)
					{
						if (!player_has_los_bold(y, x))
						{
							*ty = y;  *tx = x;
							done = TRUE;
							break;
						}
					}
				}

				/* Return if successful */
				if (done) return (TRUE);
			}
		}

		/* No flow info, or don't need it -- see bottom of function */
	}

	/* The monster is in line of sight. */
	else
	{
		int prev_cost = cave_cost[m_ptr->fy][m_ptr->fx];
		int start = rand_int(8);

		/* Look for adjacent hiding places */
		for (i = start; i < 8 + start; i++)
		{
			y = m_ptr->fy + ddy_ddd[i % 8];
			x = m_ptr->fx + ddx_ddd[i % 8];

			/* Check Bounds */
			if (!in_bounds(y, x)) continue;

			/* No grids in LOS */
			if (player_has_los_bold(y, x)) continue;

			/* Grid must be pretty easy to enter */
			if (cave_passable_mon(m_ptr, y, x, &dummy) < 50) continue;

			/* Accept any grid that doesn't have a lower flow (noise) cost. */
			if (cave_cost[y][x] >= prev_cost)
			{
				*ty = y;
				*tx = x;
				prev_cost = cave_cost[y][x];

				/* Success */
				return (TRUE);
			}
		}

		/* Find a nearby grid not in LOS of the character. */
		if (find_safety(m_ptr, ty, tx) == TRUE) return (TRUE);

		/*
		 * No safe place found.  If monster is in LOS and close,
		 * it will turn to fight.
		 */
		if ((player_has_los_bold(m_ptr->fy, m_ptr->fx)) &&
		    (m_ptr->cdis < TURN_RANGE))
		{
			/* Turn on the player or give up */
			monster_surrender_or_fight(m_idx);
			
			/* Go for the player */
			*ty = p_ptr->py;
			*tx = p_ptr->px;
			
			return (TRUE);
		}
	}

	/* Move directly away from character. */
	*ty = -(p_ptr->py - m_ptr->fy);
	*tx = -(p_ptr->px - m_ptr->fx);

	/* We want to run away */
	return (TRUE);
}



/*
 * Choose the probable best direction for a monster to move in.  This
 * is done by choosing a target grid and then finding the direction that
 * best approaches it.
 *
 * Monsters that cannot move always attack if possible.
 * Frightened monsters retreat.
 * Monsters adjacent to the character attack if possible.
 *
 * Monster packs lure the character into open ground and then leap
 * upon him.  Monster groups try to surround the character.  -KJ-
 *
 * Monsters not in LOS always advance (this avoids player frustration).
 * Monsters in LOS will advance to the character, up to their standard
 * combat range, to a grid that allows them to target the character, or
 * just move at random if they are happy where they are, depending on the
 * tactical situation and the monster's preferred and minimum combat
 * ranges.
 * NOTE:  Here is an area that would benefit from more development work.
 *
 * Non-trivial movement calculations are performed by the helper
 * functions "get_move_advance" and "get_move_retreat", which keeps
 * this function relatively simple.
 *
 * The variable "must_use_target" is used for monsters that can't
 * currently perceive the character, but have a known target to move
 * towards.  With a bit more work, this will lead to semi-realistic
 * "hunting" behavior.
 *
 * Return FALSE if monster doesn't want to move or can't.
 */
static bool get_move(int m_idx, int *ty, int *tx, bool *fear,
		     bool must_use_target)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int i, start;
	int y, x;

	int py = p_ptr->py;
	int px = p_ptr->px;

	/* Assume no movement */
	*ty = m_ptr->fy;
	*tx = m_ptr->fx;

	/*
	 * Monsters that cannot move will attack the character if he is
	 * adjacent.
	 *
	 * Now also attack allies.
	 */
	if ((r_ptr->flags1 & (RF1_NEVER_MOVE)) || (m_ptr->petrify))
	{
		int i;
		int d = 9;

		/* Hack -- memorize lack of moves after a while. */
		if ((r_ptr->flags1 & (RF1_NEVER_MOVE)) && !(l_ptr->flags1 & (RF1_NEVER_MOVE)))
		{
			if ((m_ptr->ml) && (randint(20) == 1))
				l_ptr->flags1 |= (RF1_NEVER_MOVE);
		}
		
		/* Use a target if it is adjacent */
		if ((*ty) && (*tx) && (distance(m_ptr->fy, m_ptr->fx, *ty, *tx) <= 1))
		{
			return (TRUE);
		}

		/* Look for adjacent enemies */
		for (i = 0; i < 8; i++)
		{
			int y1 = m_ptr->fy + ddy_ddd[i];
			int x1 = m_ptr->fx + ddx_ddd[i];

			if (!in_bounds_fully(y1, x1)) continue;

			if (cave_m_idx[y1][x1] <= 0) continue;

			/* Attack if enemies */
			if (choose_to_attack_monster(m_ptr, &m_list[cave_m_idx[y1][x1]]))
			{
				d = i;
			}
		}

		/* Is character in range? */
		if ((d < 9) || ((choose_to_attack_player(m_ptr)) && (m_ptr->cdis <= 1)))
		{
			/* Monster can't melee either (pathetic little creature) */
			if (r_ptr->flags1 & (RF1_NEVER_BLOW))
			{
				/* Hack -- memorize lack of attacks after a while */
				if (!(l_ptr->flags1 & (RF1_NEVER_BLOW)))
				{
					if ((m_ptr->ml) && (randint(10) == 1))
						l_ptr->flags1 |= (RF1_NEVER_BLOW);
				}
				return (FALSE);
			}

			/* Not afraid */
			*fear = FALSE;

			/* Prefer player */
			if ((choose_to_attack_player(m_ptr)) && (m_ptr->cdis <= 1))
			{
				/* Kill. */
				*ty = py;
				*tx = px;
			}
			else
			{
				/* Kill. */
				*ty = m_ptr->fy + ddy_ddd[d];
				*tx = m_ptr->fx + ddx_ddd[d];
			}
			return (TRUE);
		}

		/* If we can't hit anything, do not move */
		else
		{
			return (FALSE);
		}
	}


	/*
	 * Monster is only allowed to use targetting information.
	 */
	if (must_use_target)
	{
		*ty = m_ptr->ty;
		*tx = m_ptr->tx;
		return (TRUE);
	}


	/*** Handle monster fear -- only for monsters that can move ***/

	/* Is the monster scared? */
	if ((m_ptr->min_range == FLEE_RANGE) || (m_ptr->monfear)) *fear = TRUE;
	else *fear = FALSE;

	/* Monster is frightened or terrified. */
	if (*fear)
	{
		/* The character is too close to avoid, and faster than we are */
		if ((!m_ptr->monfear) && (m_ptr->cdis < TURN_RANGE) &&
		     (p_ptr->pspeed > m_ptr->mspeed))
		{
			/* Recalculate range */
			find_range(m_idx);

			/* Note changes in monster attitude */
			if (m_ptr->min_range < m_ptr->cdis)
			{
				/* Cancel fear */
				*fear = FALSE;

				/* No message -- too annoying */

				/* Charge! */
				*ty = py;
				*tx = px;

				return (TRUE);
			}
		}

		/* The monster is within 25 grids of the character */
		else if (m_ptr->cdis < FLEE_RANGE)
		{
			/* Find and move towards a hidey-hole */
			get_move_retreat(m_idx, ty, tx);
			return (TRUE);
		}

		/* Monster is well away from danger */
		else
		{
			/* No need to move */
			return (FALSE);
		}
	}


	/* If the character is adjacent, attack or back off.  */
	if ((!*fear) && (m_ptr->cdis <= 1))
	{
		/* Monsters that cannot attack back off. */
		if (r_ptr->flags1 & (RF1_NEVER_BLOW))
		{
			/* Hack -- memorize lack of attacks after a while */
			if (!(l_ptr->flags1 & (RF1_NEVER_BLOW)))
			{
				if ((m_ptr->ml) && (randint(10) == 1))
					l_ptr->flags1 |= (RF1_NEVER_BLOW);
			}

			/* Back away */
			*fear = TRUE;
		}

		else
		{
			/* All other monsters attack. */
			*ty = py;
			*tx = px;
			return (TRUE);
		}
	}


	/* Animal packs try to lure the character into the open. */
	if ((!*fear) && (r_ptr->flags1 & (RF1_FRIENDS)) &&
			(r_ptr->flags3 & (RF3_ANIMAL))  &&
		      (!((r_ptr->flags2 & (RF2_PASS_WALL)) || (m_ptr->tim_passw > 10) ||
		      (r_ptr->flags2 & (RF2_KILL_WALL)))))
	{
		/* Animal has to be willing to melee */
		if (m_ptr->min_range == 1)
		{
			/*
			 * If character vulnerability has not yet been
			 * calculated this turn, calculate it now.
			 */
			if (p_ptr->vulnerability == 0)
			{
				/* Count passable grids next to player */
				for (i = 0; i < 8; i++)
				{
					y = py + ddy_ddd[i];
					x = px + ddx_ddd[i];

					/* Check Bounds */
					if (!in_bounds_fully(y, x)) continue;

					/* Count floor grids (generic passable) */
					if (cave_floor_bold(y, x))
					{
						p_ptr->vulnerability++;
					}
				}

				/*
				 * Take character weakness into account (this
				 * always adds at least one)
				 */
				if (p_ptr->chp <= 10) p_ptr->vulnerability = 100;
				else p_ptr->vulnerability += (p_ptr->mhp / p_ptr->chp);
			}

			/* Character is insufficiently vulnerable */
			if (p_ptr->vulnerability <= 4)
			{
				/* If we're in sight, find a hiding place */
				if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_SEEN))
				{
					/* Find a safe spot to lurk in */
					if (get_move_retreat(m_idx, ty, tx))
					{
						*fear = TRUE;
					}
					else
					{
						/* No safe spot -- charge */
						*ty = py;
						*tx = px;
					}
				}

				/* If we're not viewable, we advance cautiously */
				else
				{
					/* Advance, ... */
					get_move_advance(m_idx, ty, tx);

					/* ... but make sure we stay hidden. */
					*fear = TRUE;
				}

				/* done */
				return (TRUE);
			}
		}
	}

	/* Monster groups try to surround the character. */
	if ((!*fear) && (r_ptr->flags1 & (RF1_FRIENDS)) && (m_ptr->cdis <= 3))
	{
		start = rand_int(8);

		/* Find a random empty square next to the player to head for */
		for (i = start; i < 8 + start; i++)
		{
			/* Pick squares near player */
			y = py + ddy_ddd[i % 8];
			x = px + ddx_ddd[i % 8];

			/* Check Bounds */
			if (!in_bounds(y, x)) continue;

			/* Ignore occupied grids */
			if (cave_m_idx[y][x] != 0) continue;

			/* Ignore grids that monster can't enter immediately */
			if (!cave_exist_mon(m_ptr->r_idx, y, x, FALSE)) continue;

			/* Accept */
			*ty = y;
			*tx = x;
			return (TRUE);
		}
	}

	/* Sneak monsters try to get 'near' the player without being seen,
		before attacking. */
	if ((!*fear) && (r_ptr->flags2 & (RF2_SNEAKY)) && (m_ptr->cdis >= 3))
	{
		/* If we're in sight, find a hiding place */
		if (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_SEEN))
		{
			/* Find a safe spot to lurk in */
			if (!(play_info[*ty][*tx] & (PLAY_SEEN))
				&& get_move_retreat(m_idx, ty, tx))
			{
				*fear = TRUE;
			}
			/* The game is up */
			else
			{
				/* No safe spot -- charge */
				*ty = py;
				*tx = px;
			}
		}

		/* If we're not viewable, we advance cautiously */
		else
		{
			/* Advance, ... */
			get_move_advance(m_idx, ty, tx);

			/* But avoid visibility */
			*fear = TRUE;
		}
	}


	/* Monster can go through rocks - head straight for character */
	if ((!*fear) && ((r_ptr->flags2 & (RF2_PASS_WALL)) || (m_ptr->tim_passw > 10) ||
			 (r_ptr->flags2 & (RF2_KILL_WALL))))
	{
		*ty = py;
		*tx = px;
		return (TRUE);
	}


	/* No special moves made -- use standard movement */

	/* Not frightened */
	if (!*fear)
	{
		/*
		 * XXX XXX -- The monster cannot see the character.  Make it
		 * advance, so the player can have fun ambushing it.
		 */
		if (!player_has_los_bold(m_ptr->fy, m_ptr->fx))
		{
			/* Advance */
			get_move_advance(m_idx, ty, tx);
		}

		/* Monster can see the character */
		else
		{
			/* Always reset the monster's target */
			m_ptr->ty = py;
			m_ptr->tx = px;

			/* Monsters too far away will advance. */
			if (m_ptr->cdis > m_ptr->best_range)
			{
				*ty = py;
				*tx = px;
			}

			/* Monsters not too close will often advance */
			else if ((m_ptr->cdis > m_ptr->min_range)  && (rand_int(2) == 0))
			{
				*ty = py;
				*tx = px;
			}

			/* Monsters that can't target the character will advance. */
			else if (!projectable(m_ptr->fy, m_ptr->fx, py, px, 0))
			{
				*ty = py;
				*tx = px;
			}

			/* Otherwise they will move randomly. */
			/* TODO: Monsters could look for better terrain... */
			else
			  {
			    /* pick a random grid next to the monster */
			    int i = rand_int(8);

			    *ty = m_ptr->fy + ddy_ddd[i];
			    *tx = m_ptr->fx + ddx_ddd[i];
			  }
		}
	}

	/* Monster is frightened */
	else
	{
		/* Back away -- try to be smart about it */
		get_move_retreat(m_idx, ty, tx);
	}


	/* We do not want to move */
	if ((*ty == m_ptr->fy) && (*tx == m_ptr->fx)) return (FALSE);

	/* We want to move */
	return (TRUE);
}




/*
 * A simple method to help fleeing monsters who are having trouble getting
 * to their target.  It's very limited, but works fairly well in the
 * situations it is called upon to resolve.  XXX
 *
 * If this function claims success, ty and tx must be set to a grid
 * adjacent to the monster.
 *
 * Return TRUE if this function actually did any good.
 */
static bool get_route_to_target(monster_type *m_ptr, int *ty, int *tx)
{
	int i, j;
	int y, x, yy, xx;
	int target_y, target_x, dist_y, dist_x;

	bool dummy;
	bool below = FALSE;
	bool right = FALSE;

	target_y = 0;
	target_x = 0;

	/* Is the target further away vertically or horizontally? */
	dist_y = ABS(m_ptr->ty - m_ptr->fy);
	dist_x = ABS(m_ptr->tx - m_ptr->fx);

	/* Target is further away vertically than horizontally */
	if (dist_y > dist_x)
	{
		/* Find out if the target is below the monster */
		if (m_ptr->ty - m_ptr->fy > 0) below = TRUE;

		/* Search adjacent grids */
		for (i = 0; i < 8; i++)
		{
			y = m_ptr->fy + ddy_ddd[i];
			x = m_ptr->fx + ddx_ddd[i];

			/* Check Bounds (fully) */
			if (!in_bounds_fully(y, x)) continue;

			/* Grid is not passable */
			if (!cave_passable_mon(m_ptr, y, x, &dummy)) continue;

			/* Grid will take me further away */
			if ((( below) && (y < m_ptr->fy)) ||
			    ((!below) && (y > m_ptr->fy)))
			{
				continue;
			}

			/* Grid will not take me closer or further */
			else if (y == m_ptr->fy)
			{
				/* See if it leads to better things */
				for (j = 0; j < 8; j++)
				{
					yy = y + ddy_ddd[j];
					xx = x + ddx_ddd[j];

					/* Grid does lead to better things */
					if ((( below) && (yy > m_ptr->fy)) ||
					    ((!below) && (yy < m_ptr->fy)))
					{
						/* But it is not passable */
						if (!cave_passable_mon(m_ptr, yy, xx, &dummy)) continue;

						/* Accept (original) grid, but don't immediately claim success */
						*ty = y;
						*tx = x;
					}
				}
			}

			/* Grid will take me closer */
			else
			{
				/* Don't look this gift horse in the mouth. */
				*ty = y;
				*tx = x;
				return (TRUE);
			}
		}
	}

	/* Target is further away horizontally than vertically */
	else if (dist_x > dist_y)
	{
		/* Find out if the target is right of the monster */
		if (m_ptr->tx - m_ptr->fx > 0) right = TRUE;

		/* Search adjacent grids */
		for (i = 0; i < 8; i++)
		{
			y = m_ptr->fy + ddy_ddd[i];
			x = m_ptr->fx + ddx_ddd[i];

       		/* Check Bounds (fully) */
			if (!in_bounds_fully(y, x)) continue;

			/* Grid is not passable */
			if (!cave_passable_mon(m_ptr, y, x, &dummy)) continue;

			/* Grid will take me further away */
			if ((( right) && (x < m_ptr->fx)) ||
			    ((!right) && (x > m_ptr->fx)))
			{
				continue;
			}

			/* Grid will not take me closer or further */
			else if (x == m_ptr->fx)
			{
				/* See if it leads to better things */
				for (j = 0; j < 8; j++)
				{
					yy = y + ddy_ddd[j];
					xx = x + ddx_ddd[j];

					/* Grid does lead to better things */
					if ((( right) && (xx > m_ptr->fx)) ||
					    ((!right) && (xx < m_ptr->fx)))
					{
						/* But it is not passable */
						if (!cave_passable_mon(m_ptr, yy, xx, &dummy)) continue;

						/* Accept (original) grid, but don't immediately claim success */
						target_y = y;
						target_x = x;
					}
				}
			}

			/* Grid will take me closer */
			else
			{
				/* Don't look this gift horse in the mouth. */
				*ty = y;
				*tx = x;
				return (TRUE);
			}
		}
	}

	/* Target is the same distance away along both axes. */
	else
	{
		/* XXX XXX - code something later to fill this hole. */
		return (FALSE);
	}

	/* If we found a solution, claim success */
	if ((target_y) && (target_x))
	{
		*ty = target_y;
		*tx = target_x;
		return (TRUE);
	}

	/* No luck */
	return (FALSE);
}


/*
 * If one monster moves into another monster's grid, they will
 * normally swap places.  If the second monster cannot exist in the
 * grid the first monster left, this can't happen.  In such cases,
 * the first monster tries to push the second out of the way.
 */
bool push_aside(int fy, int fx, monster_type *n_ptr)
{

	int y, x, i;
	int dir = 0;


	/*
	 * Translate the difference between the locations of the two
	 * monsters into a direction of travel.
	 */
	for (i = 0; i < 10; i++)
	{
		/* Require correct difference along the y-axis */
		if ((n_ptr->fy - fy) != ddy[i]) continue;

		/* Require correct difference along the x-axis */
		if ((n_ptr->fx - fx) != ddx[i]) continue;

		/* Found the direction */
		dir = i;
		break;
	}

	/* Favor either the left or right side on the "spur of the moment". */
	if (turn % 2 == 0) dir += 10;

	/* Check all directions radiating out from the initial direction. */
	for (i = 0; i < 8; i++)
	{
		int side_dir = side_dirs[dir][i];

		y = n_ptr->fy + ddy[side_dir];
		x = n_ptr->fx + ddx[side_dir];

		/* Illegal grid */
		if (!in_bounds_fully(y, x)) continue;

		/* Grid is not occupied, and the 2nd monster can exist in it. */
		if (cave_exist_mon(n_ptr->r_idx, y, x, FALSE))
		{
			/* Push the 2nd monster into the empty grid. */
			monster_swap(n_ptr->fy, n_ptr->fx, y, x);
			return (TRUE);
		}
	}

	/* We didn't find any empty, legal grids */
	return (FALSE);
}


/*
 *  Determine the language a monster speaks.
 */
int monster_language(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Hack -- there are many natural languages */
	if (r_ptr->flags3 & (RF3_ANIMAL | RF3_INSECT | RF3_PLANT))
	{
		/* Hack -- language depends on graphic */
		int language = LANG_NATURAL;

		if ((r_ptr->d_char == ':') || (r_ptr->d_char == ';')) language = LANG_FOREST;
		else if (r_ptr->d_char == ',') language = LANG_MUSHROOM;
		else if ((r_ptr->d_char >= 'A') && (r_ptr->d_char <= 'Z')) language += r_ptr->d_char - 'A' + 1;
		else if ((r_ptr->d_char >= 'a') && (r_ptr->d_char <= 'z')) language += r_ptr->d_char - 'a' + 26 + 1;

		return (language);
	}
	else if (r_ptr->flags3 & (RF3_UNDEAD)) return (LANG_UNDEAD);
	else if (r_ptr->flags3 & (RF3_DEMON)) return (LANG_DEMON);
	else if (r_ptr->flags3 & (RF3_DRAGON)) return (LANG_DRAGON);
	else if (r_ptr->flags3 & (RF3_ORC)) return (LANG_ORC);
	else if (r_ptr->flags3 & (RF3_TROLL)) return (LANG_TROLL);
	else if (r_ptr->flags3 & (RF3_GIANT)) return (LANG_GIANT);
	else if (r_ptr->flags9 & (RF9_ELF)) return (LANG_ELF);
	else if (r_ptr->flags9 & (RF9_DWARF)) return (LANG_DWARF);

	return (LANG_COMMON);
}

/*
 * Check if player understands language
 */
bool player_understands(int language)
{
	bool understand = FALSE;

	if (language == LANG_COMMON) understand = TRUE;
	else if ((language == LANG_ELF) && (p_ptr->cur_flags4 & (TR4_ELF))) understand = TRUE;
	else if ((language == LANG_DWARF) && (p_ptr->cur_flags4 & (TR4_DWARF))) understand = TRUE;
	else if ((language == LANG_ORC) && ((p_ptr->cur_flags4 & (TR4_ORC)) || (p_ptr->cur_flags3 & (TR3_ESP_ORC)))) understand = TRUE;
	else if ((language == LANG_TROLL) && ((p_ptr->cur_flags4 & (TR4_TROLL)) || (p_ptr->cur_flags3 & (TR3_ESP_TROLL)))) understand = TRUE;
	else if ((language == LANG_GIANT) && ((p_ptr->cur_flags4 & (TR4_GIANT)) || (p_ptr->cur_flags3 & (TR3_ESP_GIANT)))) understand = TRUE;
	else if ((language == LANG_DRAGON) && ((p_ptr->cur_flags4 & (TR4_DRAGON)) || (p_ptr->cur_flags3 & (TR3_ESP_DRAGON)))) understand = TRUE;
	else if ((language == LANG_DEMON) && ((p_ptr->cur_flags4 & (TR4_DEMON)) || (p_ptr->cur_flags3 & (TR3_ESP_DEMON)))) understand = TRUE;
	else if ((language == LANG_UNDEAD) && ((p_ptr->cur_flags4 & (TR4_UNDEAD)) || (p_ptr->cur_flags3 & (TR3_ESP_UNDEAD)))) understand = TRUE;
	else if (((language == LANG_FOREST) || (language == LANG_MUSHROOM)) && (p_ptr->pshape == RACE_ENT)) understand = TRUE;
	else if ((language >= LANG_NATURAL) && ((p_ptr->cur_flags4 & (TR4_ANIMAL)) || (p_ptr->cur_flags3 & (TR3_ESP_NATURE)))) understand = TRUE;

	return (understand);
}


/*
 *  Monsters speak to each other in various situations.
 */
void monster_speech(int m_idx, cptr saying, bool understand)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int language = monster_language(m_ptr->r_idx);
	int speech = language;

	char m_name[80];

	/* Monster never speaks aloud */
	if (r_ptr->flags3 & (RF3_NONVOCAL))
	{
		if (!(p_ptr->cur_flags3 & (TR3_TELEPATHY))) return;

		speech = LANG_ESP;
	}

	/* Check if player understands language */
	understand |= player_understands(language);

	/* Get accent */
	if (!speech)
	{
		speech = LANG_NATURAL;

		if ((r_ptr->d_char == ':') || (r_ptr->d_char == ';')) speech = LANG_FOREST;
		else if (r_ptr->d_char == ',') speech = LANG_MUSHROOM;
		else if ((r_ptr->d_char >= 'A') && (r_ptr->d_char <= 'Z')) speech += r_ptr->d_char - 'A' + 1;
		else if ((r_ptr->d_char >= 'a') && (r_ptr->d_char <= 'z')) speech += r_ptr->d_char - 'a' + 26 + 1;
	}

	/* Paranoia */
	if (speech >= MAX_LANGUAGES) return;

	/* Get the monster name */
	monster_desc(m_name, sizeof(m_name), m_idx, 0);

	/* Dump a message */
	if (!understand) msg_format("%^s %s something.", m_name, vocalize[speech]);
	else
	{
		char buf[256];

		char *t = buf;
		cptr s = saying;
		cptr u;

		/* Copy the string */
		for (; *s; s++)
		{
			/* Player name */
			if (*s == '&')
			{
				/* Hack -- townsfolk from normal towns and allies know the players name */
				if (((((m_ptr->mflag & (MFLAG_TOWN)) != 0) && ((level_flag & (LF1_TOWN)) != 0))
						|| ((m_ptr->mflag & (MFLAG_ALLY)) != 0)) && (strlen(op_ptr->full_name)))
				{
					u = op_ptr->full_name;

					/* Copy the string */
					while (*u) *t++ = *u++;
				}
				else
				{
					u = "The ";
					if (s != saying) u = "the ";

					/* Copy the string */
					while (*u) *t++ = *u++;

					u = p_name + rp_ptr->name;

					/* Copy the string */
					while (*u) *t++ = *u++;

					*t++ = ' ';

					u = c_name + cp_ptr->name;

					/* Copy the string */
					while (*u) *t++ = *u++;
				}
			}
			/* Player race only */
			else if (*s == '@')
			{
				/* Get race only */
				u = p_name + rp_ptr->name;

				/* Copy the string */
				while (*u) *t++ = *u++;
			}
			/* Player class only */
			else if (*s == '#')
			{
				/* Get race only */
				u = c_name + cp_ptr->name;

				/* Copy the string */
				while (*u) *t++ = *u++;
			}
			/* Normal */
			else
			{
				/* Copy */
				*t++ = *s;
			}
		}

		/* Terminate the buffer */
		*t = '\0';

		msg_format("%^s %s '%s'", m_name, vocalize[speech], buf);
	}
}


/*
 * Alert others around the monster to some information that usually modifies their AI state.
 *
 * u, v and w are parameters for the information conveyed.
 */
bool tell_allies_info(int y, int x, cptr saying, int u, int v, int w, bool wakeup,
		bool query_ally_hook(const monster_type *n_ptr, int u, int v, int w),
		void tell_ally_hook(monster_type *n_ptr, int u, int v, int w))
{
	int i, language, d;
	bool vocal = FALSE;

	/* Get the language */
	language = monster_language(m_list[cave_m_idx[y][x]].r_idx);

	/* Scan all other monsters */
	for (i = 1; i < z_info->m_max; i++)
	{
		/* Access the monster */
		monster_type *n_ptr = &m_list[i];

		/* Ignore dead monsters */
		if (!n_ptr->r_idx) continue;

		/* Ignore itself */
		if (i == cave_m_idx[y][x]) continue;

		/* Ignore if monster awake and knows already */
		if (!(n_ptr->csleep) && (query_ally_hook) && query_ally_hook(n_ptr, u, v, w)) continue;

		/* Ignore if monster is asleep and we're not waking them */
		if ((n_ptr->csleep) && !wakeup) continue;

		/* Ignore allies or vice versa */
		if (((n_ptr->mflag & (MFLAG_ALLY)) != 0) != ((m_list[cave_m_idx[y][x]].mflag & (MFLAG_ALLY)) != 0)) continue;

		/* Ignore monsters who speak different language */
		if (monster_language(n_ptr->r_idx) != language) continue;

		/* Get distance */
		d = distance(y, x, n_ptr->fy, n_ptr->fx);

		/* Ignore monsters that are too far away */
		if (d > MAX_SIGHT) continue;

		/* Ignore monsters that are not nearby or projectable */
		if ((d > 3) && !generic_los(y, x, n_ptr->fy, n_ptr->fx, CAVE_XLOF)) continue;

		/* Activate all other monsters and communicate to them */
		n_ptr->csleep = 0;
		n_ptr->mflag |= (MFLAG_ACTV);

		/* Tell the ally further information */
		if (tell_ally_hook) tell_ally_hook(n_ptr, u, v, w);

		/* Vocalize */
		vocal = TRUE;
	}

	/* Nothing to say? */
	if (!vocal) return (FALSE);

	/* Speak */
	if ((saying) && (strlen(saying))) monster_speech(cave_m_idx[y][x], saying, FALSE);

	/* Something said */
	return (TRUE);
}


/*
 * Check if ally knows about player capability already
 */
bool query_ally_can(const monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Check if the ally already knows */
	if ((n_ptr->smart) & flag) return (TRUE);

	return (FALSE);
}

/*
 * Tells the ally the player can do something
 */
void tell_ally_can(monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Tell the ally */
	n_ptr->smart |= (flag);
}



/*
 * Alert others around the monster that the player has a resistance, and wake them up.
 */
bool tell_allies_player_can(int y, int x, u32b flag)
{
	cptr saying = NULL;

	/* Define saying */
	if (flag & (SM_IMM_ACID)) saying = "& is immune to acid.";
	else if (flag & (SM_IMM_ELEC)) saying = "& is immune to electricity.";
	else if (flag & (SM_IMM_FIRE)) saying = "& is immune to fire.";
	else if (flag & (SM_IMM_COLD)) saying = "& is immune to cold.";
	else if (flag & (SM_IMM_POIS)) saying = "& is immune to poison.";
	else if (((flag & (SM_OPP_ACID)) != 0) || ((flag & (SM_RES_ACID)) != 0)) saying = "& resists acid.";
	else if (((flag & (SM_OPP_ELEC)) != 0) || ((flag & (SM_RES_ELEC)) != 0)) saying = "& resists electricity.";
	else if (((flag & (SM_OPP_FIRE)) != 0) || ((flag & (SM_RES_FIRE)) != 0)) saying = "& resists fire.";
	else if (((flag & (SM_OPP_COLD)) != 0) || ((flag & (SM_RES_COLD)) != 0)) saying = "& resists cold.";
	else if (((flag & (SM_OPP_POIS)) != 0) || ((flag & (SM_RES_POIS)) != 0)) saying = "& resists poison.";
	else if (((flag & (SM_OPP_FEAR)) != 0) || ((flag & (SM_RES_FEAR)) != 0)) saying = "& resists fear.";
	else if (flag & (SM_SEE_INVIS)) saying = "& can see invisible.";
	else if (flag & (SM_GOOD_SAVE)) saying = "& can easily resist magic.";
	else if (flag & (SM_PERF_SAVE)) saying = "& can always resist magic.";
	else if (flag & (SM_FREE_ACT)) saying = "& resists paralyzation.";
	else if (flag & (SM_IMM_MANA)) saying = "& has no mana.";
	else if (flag & (SM_RES_LITE)) saying = "& resists lite.";
	else if (flag & (SM_RES_DARK)) saying = "& resists darkness.";
	else if (flag & (SM_RES_BLIND)) saying = "& resists blindness.";
	else if (flag & (SM_RES_CONFU)) saying = "& resists confusion.";
	else if (flag & (SM_RES_SOUND)) saying = "& resists sound.";
	else if (flag & (SM_RES_SHARD)) saying = "& resists shard.";
	else if (flag & (SM_RES_NEXUS)) saying = "& resists nexus.";
	else if (flag & (SM_RES_NETHR)) saying = "& resists nether.";
	else if (flag & (SM_RES_CHAOS)) saying = "& resists chaos.";
	else if (flag & (SM_RES_DISEN)) saying = "& resists disenchantment.";

	/* Something said? */
	return (tell_allies_info(y, x, saying, (int)flag, 0, 0, FALSE, query_ally_can, tell_ally_can));
}


/*
 * Check if ally knows about loss of player capability already
 */
bool query_ally_not(const monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Check if the ally already knows */
	if (((n_ptr->smart) & flag) == 0) return (TRUE);

	return (FALSE);
}

/*
 * Tells the ally the player can no longer do something
 */
void tell_ally_not(monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Tell the ally */
	n_ptr->smart &= ~(flag);
}


/*
 * Alert others around the monster that the player does not have a resistance, and wake them up.
 */
bool tell_allies_player_not(int y, int x, u32b flag)
{
	cptr saying = NULL;

	/* Define saying */
	if (flag & (SM_IMM_ACID)) saying = "& no longer is immune to acid.";
	else if (flag & (SM_IMM_ELEC)) saying = "& no longer is immune to electricity.";
	else if (flag & (SM_IMM_FIRE)) saying = "& no longer is immune to fire.";
	else if (flag & (SM_IMM_COLD)) saying = "& no longer is immune to cold.";
	else if (flag & (SM_IMM_POIS)) saying = "& no longer is immune to poison.";
	else if (((flag & (SM_OPP_ACID)) != 0) || ((flag & (SM_RES_ACID)) != 0)) saying = "& no longer resists acid.";
	else if (((flag & (SM_OPP_ELEC)) != 0) || ((flag & (SM_RES_ELEC)) != 0)) saying = "& no longer resists electricity.";
	else if (((flag & (SM_OPP_FIRE)) != 0) || ((flag & (SM_RES_FIRE)) != 0)) saying = "& no longer resists fire.";
	else if (((flag & (SM_OPP_COLD)) != 0) || ((flag & (SM_RES_COLD)) != 0)) saying = "& no longer resists cold.";
	else if (((flag & (SM_OPP_POIS)) != 0) || ((flag & (SM_RES_POIS)) != 0)) saying = "& no longer resists poison.";
	else if (((flag & (SM_OPP_FEAR)) != 0) || ((flag & (SM_RES_FEAR)) != 0)) saying = "& no longer resists fear.";
	else if (flag & (SM_SEE_INVIS)) saying = "& cannot see invisible.";
	else if (flag & (SM_GOOD_SAVE)) saying = "& no longer easily resists magic.";
	else if (flag & (SM_PERF_SAVE)) saying = "& no longer always resists magic.";
	else if (flag & (SM_FREE_ACT)) saying = "& no longer resists paralyzation.";
	else if (flag & (SM_IMM_MANA)) saying = "& no longer has no mana.";
	else if (flag & (SM_RES_LITE)) saying = "& no longer resists lite.";
	else if (flag & (SM_RES_DARK)) saying = "& no longer resists darkness.";
	else if (flag & (SM_RES_BLIND)) saying = "& no longer resists blindness.";
	else if (flag & (SM_RES_CONFU)) saying = "& no longer resists confusion.";
	else if (flag & (SM_RES_SOUND)) saying = "& no longer resists sound.";
	else if (flag & (SM_RES_SHARD)) saying = "& no longer resists shard.";
	else if (flag & (SM_RES_NEXUS)) saying = "& no longer resists nexus.";
	else if (flag & (SM_RES_NETHR)) saying = "& no longer resists nether.";
	else if (flag & (SM_RES_CHAOS)) saying = "& no longer resists chaos.";
	else if (flag & (SM_RES_DISEN)) saying = "& no longer resists disenchantment.";

	/* Something said? */
	return (tell_allies_info(y, x, saying, (int)flag, 0, 0, FALSE, query_ally_not, tell_ally_not));
}


/*
 * Check if ally has mflag already
 */
bool query_ally_mflag(const monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Check if the ally already knows */
	if ((n_ptr->mflag) & flag) return (TRUE);

	return (FALSE);
}

/*
 * Tells the ally mflag
 */
void tell_ally_mflag(monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Tell the ally */
	n_ptr->mflag |= (flag);
}


/*
 * Alert others around the monster about something using the m_flag, and wake them up
 */
bool tell_allies_mflag(int y, int x, u32b flag, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, saying, (int)flag, 0, 0, TRUE, query_ally_mflag, tell_ally_mflag));
}


/*
 * Check if ally has cleared mflag already
 */
bool query_ally_not_mflag(const monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Check if the ally already knows */
	if (((n_ptr->mflag) & flag) == 0) return (TRUE);

	return (FALSE);
}


/*
 * Tells the ally to clear mflag
 */
void tell_ally_not_mflag(monster_type *n_ptr, int u, int v, int w)
{
	u32b flag = (u32b)u;
	(void)v;
	(void)w;

	/* Don't get rid of MFLAG_TOWN on allies */
	if (((n_ptr->mflag) & (MFLAG_ALLY)) != 0) flag &= ~(MFLAG_TOWN);

	/* Tell the ally */
	n_ptr->mflag &= ~(flag);
}


/*
 * Alert others around the monster about something using the m_flag, and wake them up
 */
bool tell_allies_not_mflag(int y, int x, u32b flag, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, saying, (int)flag, 0, 0, TRUE, query_ally_not_mflag, tell_ally_not_mflag));
}


/*
 * Tells the allies of the ally about death
 */
void tell_ally_death(monster_type *n_ptr, int u, int v, int w)
{
	cptr saying = (cptr)u;
	(void)v;
	(void)w;

	/* Clear the town flag */
	if (((n_ptr->mflag) & (MFLAG_ALLY)) != 0) n_ptr->mflag &= ~(MFLAG_TOWN);

	/* Tell the allies of the ally */
	tell_allies_not_mflag(n_ptr->fy, n_ptr->fx, (MFLAG_TOWN), saying);
}


/*
 * Alert others around the monster that they have died, and wake them up, provided one is awake to observe it.
 */
bool tell_allies_death(int y, int x, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, NULL, (int)saying, 0, 0, TRUE, NULL, tell_ally_death));
}


/*
 * Check if ally is willing to go to that range
 */
bool query_ally_range(const monster_type *n_ptr, int u, int v, int w)
{
	(void)u;
	(void)v;
	(void)w;

	/* Check if the ally is not fleeing */
	if (n_ptr->min_range < FLEE_RANGE) return (TRUE);

	return (FALSE);
}

/*
 * Tells the ally to move to best range
 */
void tell_ally_range(monster_type *n_ptr, int u, int v, int w)
{
	(void)w;

	/* Tell the ally */
	if (u) n_ptr->best_range = u;
	if (v) n_ptr->min_range = v;
}


/*
 * Alert others around the monster about something and set their best ranged value
 */
bool tell_allies_range(int y, int x, int best_range, int min_range, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, saying, best_range, min_range, 0, TRUE, query_ally_range, tell_ally_range));
}


/*
 * Check if ally is willing to wait for a time period
 */
bool query_ally_summoned(const monster_type *n_ptr, int u, int v, int w)
{
	(void)u;
	(void)v;
	(void)w;

	/* Check if not townsfolk */
	if ((n_ptr->mflag & (MFLAG_TOWN)) == 0) return (TRUE);

	return (FALSE);
}


/*
 * Tells the ally to wait a while
 */
void tell_ally_summoned(monster_type *n_ptr, int u, int v, int w)
{
	(void)v;
	(void)w;

	/* Tell the ally */
	n_ptr->summoned = u;
}


/*
 * Alert others around the monster about something and set their 'summoned' timer
 */
bool tell_allies_summoned(int y, int x, int summoned, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, saying, summoned, 0, 0, TRUE, query_ally_summoned, tell_ally_summoned));
}



/*
 * Check if ally is willing to accept a target
 */
bool query_ally_target(const monster_type *n_ptr, int u, int v, int w)
{
	(void)u;
	(void)v;

	/* Ignore monsters who have orders */
	if ((n_ptr->ty) || (n_ptr->tx)) return (TRUE);

	/* Ignore monsters picking up a good scent */
	if ((w) && (get_scent(n_ptr->fy, n_ptr->fx) < SMELL_STRENGTH - 10)) return (TRUE);

	return (FALSE);
}


/*
 * Tells the ally to change timer
 */
void tell_ally_target(monster_type *n_ptr, int u, int v, int w)
{
	(void)w;

	/* Tell the ally */
	n_ptr->ty = u;
	n_ptr->tx = v;
}


/*
 * Alert others around the monster about something and set their best ranged value
 */
bool tell_allies_target(int y, int x, int ty, int tx, bool scent, cptr saying)
{
	/* Something said? */
	return (tell_allies_info(y, x, saying, ty, tx, scent, TRUE, query_ally_target, tell_ally_target));
}


/*
 * Check if the monster has any allies around
 */
bool check_allies_exist(int y, int x)
{
	/* Something said? */
	return (tell_allies_info(y, x, "", 0, 0, 0, FALSE, NULL, NULL));
}


/*
 * Given a target grid, calculate the grid the monster will actually
 * attempt to move into.
 *
 * The simplest case is when the target grid is adjacent to us and
 * able to be entered easily.  Usually, however, one or both of these
 * conditions don't hold, and we must pick an initial direction, than
 * look at several directions to find that most likely to be the best
 * choice.  If so, the monster needs to know the order in which to try
 * other directions on either side.  If there is no good logical reason
 * to prioritize one side over the other, the monster will act on the
 * "spur of the moment", using current turn as a randomizer.
 *
 * The monster then attempts to move into the grid.  If it fails, this
 * function returns FALSE and the monster ends its turn.
 *
 * The variable "fear" is used to invoke any special rules for monsters
 * wanting to retreat rather than advance.  For example, such monsters
 * will not leave an non-viewable grid for a viewable one and will try
 * to avoid the character.
 *
 * The variable "bash" remembers whether a monster had to bash a door
 * or not.  This has to be remembered because the choice to bash is
 * made in a different function than the actual bash move.  XXX XXX  If
 * the number of such variables becomes greater, a structure to hold them
 * would look better than passing them around from function to function.
 */
static bool make_move(int m_idx, int *ty, int *tx, bool fear, bool *bash)
{
	monster_type *m_ptr = &m_list[m_idx];

	int i, j;

	/* Start direction, current direction */
	int dir0, dir;

	/* Deltas, absolute axis distances from monster to target grid */
	int dy, ay, dx, ax;

	/* Existing monster location, proposed new location */
	int oy, ox, ny, nx;

	bool avoid = FALSE;
	bool passable = FALSE;
	bool look_again = FALSE;

	int chance;

	/* Remember where monster is */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Get the change in position needed to get to the target */
	dy = oy - *ty;
	dx = ox - *tx;

	/* Is the target grid adjacent to the current monster's position? */
	if ((!fear) && (dy >= -1) && (dy <= 1) && (dx >= -1) && (dx <= 1))
	{
		/* If it is, try the shortcut of simply moving into the grid */

		/* Get the probability of entering this grid */
		chance = cave_passable_mon(m_ptr, *ty, *tx, bash);

		/* Grid must be pretty easy to enter, or monster must be confused */
		if ((m_ptr->confused) || (chance >= 50))
		{
			/*
			 * Amusing messages and effects for confused monsters trying
			 * to enter terrain forbidden to them.
			 */
			if (chance == 0)
			{
				/* Do not actually move */
				return (FALSE);
			}

			/* We can enter this grid */
			if ((chance == 100) || (chance > rand_int(100)))
			{
				return (TRUE);
			}

			/* Failure to enter grid.  Cancel move */
			else
			{
				return (FALSE);
			}
		}
	}



	/* Calculate vertical and horizontal distances */
	ay = ABS(dy);
	ax = ABS(dx);

	/* We mostly want to move vertically */
	if (ay > (ax * 2))
	{
		/* Choose between directions '8' and '2' */
		if (dy > 0)
		{
			/* We're heading up */
			dir0 = 8;
			if ((dx > 0) || (dx == 0 && turn % 2 == 0)) dir0 += 10;
		}
		else
		{
			/* We're heading down */
			dir0 = 2;
			if ((dx < 0) || (dx == 0 && turn % 2 == 0)) dir0 += 10;
		}
	}

	/* We mostly want to move horizontally */
	else if (ax > (ay * 2))
	{
		/* Choose between directions '4' and '6' */
		if (dx > 0)
		{
			/* We're heading left */
			dir0 = 4;
			if ((dy < 0) || (dy == 0 && turn % 2 == 0)) dir0 += 10;
		}
		else
		{
			/* We're heading right */
			dir0 = 6;
			if ((dy > 0) || (dy == 0 && turn % 2 == 0)) dir0 += 10;
		}
	}

	/* We want to move up and sideways */
	else if (dy > 0)
	{
		/* Choose between directions '7' and '9' */
		if (dx > 0)
		{
			/* We're heading up and left */
			dir0 = 7;
			if ((ay < ax) || (ay == ax && turn % 2 == 0)) dir0 += 10;
		}
		else
		{
			/* We're heading up and right */
			dir0 = 9;
			if ((ay > ax) || (ay == ax && turn % 2 == 0)) dir0 += 10;
		}
	}

	/* We want to move down and sideways */
	else
	{
		/* Choose between directions '1' and '3' */
		if (dx > 0)
		{
			/* We're heading down and left */
			dir0 = 1;
			if ((ay > ax) || (ay == ax && turn % 2 == 0)) dir0 += 10;
		}
		else
		{
			/* We're heading down and right */
			dir0 = 3;
			if ((ay < ax) || (ay == ax && turn % 2 == 0)) dir0 += 10;
		}
	}

	/*
	 * Now that we have an initial direction, we must determine which
	 * grid to actually move into.
	 */
	if (TRUE)
	{
		/* Build a structure to hold movement data */
		typedef struct move_data move_data;
		struct move_data
		{
			int move_chance;
			bool move_bash;
		};
		move_data moves_data[8];


		/*
		 * Scan each of the eight possible directions, in the order of
		 * priority given by the table "side_dirs", choosing the one that
		 * looks like it will get the monster to the character - or away
		 * from him - most effectively.
		 */
		for (i = 0; i < 8; i++)
		{
			/* Get the actual direction */
			dir = side_dirs[dir0][i];

			/* Get the grid in our chosen direction */
			ny = oy + ddy[dir];
			nx = ox + ddx[dir];

			/* Check Bounds */
			if (!in_bounds(ny, nx)) continue;

			/* Store this grid's movement data. */
			moves_data[i].move_chance =
				cave_passable_mon(m_ptr, ny, nx, bash);
			moves_data[i].move_bash = *bash;


			/* Confused monsters must choose the first grid */
			if (m_ptr->confused) break;

			/* If this grid is totally impassable, skip it */
			if (moves_data[i].move_chance == 0) continue;

			/* Frightened monsters work hard not to be seen. */
			if (fear)
			{
				/* Monster is having trouble navigating to its target. */
				if ((m_ptr->ty) && (m_ptr->tx) && (i >= 2))
				{
					/* Look for an adjacent grid leading to the target */
					if (get_route_to_target(m_ptr, ty, tx))
					{
						int chance;

						/* Calculate the chance to enter the grid */
						chance = cave_passable_mon(m_ptr, *ty, *tx, bash);

						/* Try to move into the grid */
						if ((chance < 100) && (randint(100) > chance))
						{
							/* Can't move */
							return (FALSE);
						}

						/* Can move */
						return (TRUE);
					}

					/* No good route found */
					else if (i >= 3)
					{
						/*
						 * We can't get to our hiding place.  We're in line of fire.
						 * The only thing left to do is go down fighting.  XXX XXX
						 */
						 if ((m_ptr->ml) && (cave_project_bold(oy, ox)))
						 {
							/* Turn on the player or give up */
							monster_surrender_or_fight(m_idx);

							/* Hack -- lose some time  XXX XXX */
							return (FALSE);
						}
					}
				}

				/* Attacking the character as a first choice? */
				if ((i == 0) && (ny == p_ptr->py) && (nx == p_ptr->px))
				{
					/* Need to rethink some plans XXX XXX XXX */
					m_ptr->ty = 0;
					m_ptr->tx = 0;
				}

				/* Monster is visible */
				if (m_ptr->ml)
				{
					/* And is in LOS */
					if (player_has_los_bold(oy, ox))
					{
						/* Accept any easily passable grid out of LOS */
						if ((!player_has_los_bold(ny, nx)) &&
							(moves_data[i].move_chance > 40))
						{
							break;
						}
					}

					else
					{
						/* Do not enter a grid in LOS */
						if (player_has_los_bold(ny, nx))
						{
							moves_data[i].move_chance = 0;
							continue;
						}
					}
				}

				/* Monster can't be seen, and is not in a "seen" grid. */
				if ((!m_ptr->ml) && (!(play_info[oy][ox] & (PLAY_SEEN))))
				{
					/* Do not enter a "seen" grid */
					if (play_info[ny][nx] & (PLAY_SEEN))
					{
						moves_data[i].move_chance = 0;
						continue;
					}
				}
			}

			/* XXX XXX -- Sometimes attempt to break glyphs. */
			if ((f_info[cave_feat[ny][nx]].flags1 & (FF1_GLYPH)) && (!fear) &&
			    (rand_int(5) == 0))
			{
				break;
			}

			/* Initial direction is almost certainly the best one */
			if ((i == 0) && (moves_data[i].move_chance >= 80))
			{
				/*
				 * If backing away and close, try not to walk next
				 * to the character, or get stuck fighting him.
				 */
				if ((fear) && (m_ptr->cdis <= 2) &&
					(distance(p_ptr->py, p_ptr->px, ny, nx) <= 1))
				{
					avoid = TRUE;
				}

				else break;
			}

			/* Either of the first two side directions looks good */
			else if (((i == 1) || (i == 2)) &&
				 (moves_data[i].move_chance >= 50))
			{
				/* Accept the central direction if at least as good */
				if ((moves_data[0].move_chance >=
				     moves_data[i].move_chance))
				{
					if (avoid)
					{
						/* Frightened monsters try to avoid the character */
						if (distance(p_ptr->py, p_ptr->px, ny, nx) == 0)
						{
							i = 0;
						}
					}
					else
					{
						i = 0;
					}
				}

				/* Accept this direction */
				break;
			}

			/* This is the first passable direction */
			if (!passable)
			{
				/* Note passable */
				passable = TRUE;

				/* All the best directions are blocked. */
				if (i >= 3)
				{
					/* Settle for "good enough" */
					break;
				}
			}

			/* We haven't made a decision yet; look again. */
			if (i == 7) look_again = TRUE;
		}


		/* We've exhausted all the easy answers. */
		if (look_again)
		{
			/* There are no passable directions. */
			if (!passable)
			{
				return (FALSE);
			}

			/* We can move. */
			for (j = 0; j < 8; j++)
			{
				/* Accept the first option, however poor.  XXX */
				if (moves_data[j].move_chance)
				{
					i = j;
					break;
				}
			}
		}

		/* If no direction was acceptable, end turn */
		if (i >= 8)
		{
			return (FALSE);
		}

		/* Get movement information (again) */
		dir = side_dirs[dir0][i];
		*bash = moves_data[i].move_bash;

		/* No good moves, so we just sit still and wait. */
		if ((dir == 5) || (dir == 0))
		{
			return (FALSE);
		}

		/* Get grid to move into */
		*ty = oy + ddy[dir];
		*tx = ox + ddx[dir];

		/*
		 * Amusing messages and effects for confused monsters trying
		 * to enter terrain forbidden to them.
		 */
		if ((m_ptr->confused) && (moves_data[i].move_chance == 0))
		{
			/* Do not actually move */
			return (FALSE);
		}

		/* Try to move in the chosen direction.  If we fail, end turn. */
		if ((moves_data[i].move_chance < 100) &&
		    (randint(100) > moves_data[i].move_chance))
		{
			return (FALSE);
		}
	}

	/* Monster is frightened, and is obliged to fight. */
	if ((fear) && (cave_m_idx[*ty][*tx] < 0))
	{
		/* Cancel fear */
		set_monster_fear(m_ptr, 0, FALSE);

		/* Turn and fight */
		fear = FALSE;

		/* Forget target */
		m_ptr->ty = 0;    m_ptr->tx = 0;

		/* Charge!  XXX XXX */
		m_ptr->min_range = 1;  m_ptr->best_range = 1;

		/* Message if seen */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Get the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0);

			/* Dump a message */
			msg_format("%^s turns on you!", m_name);
		}
	}

	/* We can move. */
	return (TRUE);
}


/*
 * Bash or tunnel up from under various terrain.
 */
static bool bash_from_under(int m_idx, int y, int x, bool *bash)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	bool emerge = FALSE;

	/* Check variety of ways of breaking up through terrain */
	if (((r_ptr->flags2 & (RF2_BASH_DOOR | RF2_KILL_WALL)) || (r_ptr->flags3 & (RF3_HUGE))) && (f_ptr->flags1 & (FF1_BASH)))
	{
		/* Bash through the terrain */
		cave_alter_feat(y, x, FS_BASH);

		/* Bashed */
		if (r_ptr->flags2 & (RF2_BASH_DOOR)) *bash = TRUE;

		/* Emerges */
		emerge = TRUE;
	}
	else if (((r_ptr->flags2 & (RF2_KILL_WALL)) || (r_ptr->flags3 & (RF3_HUGE))) && (f_ptr->flags1 & (FF1_TUNNEL)))
	{
		/* Tunnel through the terrain */
		cave_alter_feat(y, x, FS_TUNNEL);

		/* Emerges */
		emerge = TRUE;
	}
	else if (((r_ptr->flags2 & (RF2_KILL_WALL)) || (r_ptr->flags3 & (RF3_HUGE))) && (f_ptr->flags2 & (FF2_KILL_HUGE)))
	{
		/* Tunnel through the terrain */
		cave_alter_feat(y, x, FS_KILL_HUGE);

		/* Emerges */
		emerge = TRUE;
	}
	else if (f_ptr->flags2 & (FF2_KILL_MOVE))
	{
		/* Tunnel through the terrain */
		cave_alter_feat(y, x, FS_KILL_MOVE);

		/* Emerges */
		emerge = TRUE;
	}

	/* Hack -- ensure we now have uncovered terrain */
	if (f_ptr->flags2 & (FF2_COVERED)) return (FALSE);

	/* Monster emerges from hiding if required */
	if ((emerge) && (m_ptr->mflag & (MFLAG_HIDE)))
	{
		/* Unhide the monster */
		m_ptr->mflag &= ~(MFLAG_HIDE);

		/* And reveal */
		update_mon(m_idx,FALSE);

		/* Get new feature */
		f_ptr = &f_info[cave_feat[y][x]];

		/* Hack --- tell the player if something unhides */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Get the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0);

			msg_format("%^s emerges from %s%s.",m_name,
				((f_ptr->flags2 & (FF2_FILLED))?"":"the "),
				f_name+f_ptr->name);

			if ((disturb_move || ((m_ptr->mflag & (MFLAG_VIEW)) &&
		      		disturb_near))
		      		&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				/* Disturb */
				disturb(0, 0);
			}
		}
	}

	return (emerge);
}


/*
 * Crash through the ceiling.
 */
static bool crash_from_above(int m_idx, int y, int x)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Check variety of ways of breaking through ceiling */
	if (r_ptr->flags2 & (RF2_KILL_WALL) || (r_ptr->flags3 & (RF3_HUGE)))
	{
		/* Crash through the ceiling */
		feat_near(FEAT_RUBBLE, y, x);

		/* Unhide the monster */
		m_ptr->mflag &= ~(MFLAG_HIDE | MFLAG_OVER);

		/* And reveal */
		update_mon(m_idx,FALSE);

		/* Hack --- tell the player if something unhides */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Get the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0);

			msg_format("%^s crashes through the ceiling.",m_name);

			if ((disturb_move || ((m_ptr->mflag & (MFLAG_VIEW)) &&
		      		disturb_near))
	      		&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				/* Disturb */
				disturb(0, 0);
			}
		}

		return (TRUE);
	}

	return (FALSE);
}

/*
 * Process a monster's move.
 *
 * All the plotting and planning has been done, and all this function
 * has to do is move the monster into the chosen grid.
 *
 * This may involve attacking the character, breaking a glyph of
 * warding, bashing down a door, etc..  Once in the grid, monsters may
 * stumble into monster traps, hit a scent trail, pick up or destroy
 * objects, and so forth.
 *
 * A monster's move may disturb the character, depending on which
 * disturbance options are set.
 */
static void process_move(int m_idx, int ty, int tx, bool bash)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int mmove;

	/* Existing monster location, proposed new location */
	int oy, ox, ny, nx;

	s16b this_o_idx, next_o_idx = 0;

	int feat;

	/* Default move, default lack of view */
	bool do_move = TRUE;
	bool do_view = FALSE;

	/* Assume nothing */
	bool did_open_door = FALSE;
	bool did_bash_door = FALSE;
	bool did_take_item = FALSE;
	bool did_pass_wall = FALSE;
	bool did_kill_wall = FALSE;
	bool did_smart = FALSE;
	bool did_sneak = FALSE;
	bool did_huge = FALSE;

	/* Remember where monster is */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Get the destination */
	ny = ty;
	nx = tx;

	/* Check Bounds */
	if (!in_bounds(ny, nx)) return;

	/* Get the feature in the grid that the monster is trying to enter. */
	feat = cave_feat[ny][nx];

	/* The monster is hidden in terrain, trying to attack.*/
	if (do_move && (m_ptr->mflag & (MFLAG_HIDE))
			&& (((cave_m_idx[ny][nx] < 0) && (choose_to_attack_player(m_ptr)))
			 || ((cave_m_idx[ny][nx] > 0) && (choose_to_attack_monster(m_ptr, &m_list[cave_m_idx[ny][nx]])))))
	{
		/* Monster is under covered terrain and can't slip out */
		if (!(r_ptr->flags2 & (RF2_PASS_WALL)) && !(m_ptr->tim_passw) &&
			(f_info[feat].flags2 & (FF2_COVERED)))
		{
			/* Get at player */
			if (bash_from_under(m_idx, ny, nx, &bash))
			{
				/* Can now see monster */
				if (m_ptr->ml)
				{
					/* Notice */
					if (bash) did_bash_door = TRUE;
					else if (r_ptr->flags2 & (RF2_KILL_WALL)) did_kill_wall = TRUE;
					else if (r_ptr->flags3 & (RF3_HUGE)) did_huge = TRUE;
				}
			}
			/* Can't get out, can't attack */
			else
			{
				return;
			}
		}
		/* Monster is in or over the ceiling and can't slip through */
		else if (!(r_ptr->flags2 & (RF2_PASS_WALL)) && !(m_ptr->tim_passw) &&
				(m_ptr->mflag & (MFLAG_HIDE)) && (m_ptr->mflag & (MFLAG_OVER)))
		{
			/* Get at player - through roof */
			if (crash_from_above(m_idx, ny, nx))
			{
				/* Can now see monster? */
				if (m_ptr->ml)
				{
					if (r_ptr->flags2 & (RF2_KILL_WALL)) did_kill_wall = TRUE;
					else if (r_ptr->flags3 & (RF3_HUGE)) did_huge = TRUE;
				}
			}
			/* Can't get out, can't attack */
			else
			{
				return;
			}
		}
		else
		{
			/* Unhide the monster */
			m_ptr->mflag &= ~(MFLAG_HIDE);

			/* And reveal */
			update_mon(m_idx,FALSE);

			/* Hack --- tell the player if something unhides */
			if ((m_ptr->mflag & (MFLAG_HIDE)) && (m_ptr->ml))
			{
				char m_name[80];

				/* Get the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				msg_format("%^s emerges from %s%s.",m_name,
					((f_info[cave_feat[oy][ox]].flags2 & (FF2_FILLED))?"":"the "),
					f_name+f_info[cave_feat[oy][ox]].name);
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

			/* Hack -- always notice pass wall */
			if (r_ptr->flags2 & (RF2_PASS_WALL)) did_pass_wall = TRUE;
		}
	}

	/* The grid is occupied by the player. */
	if (cave_m_idx[ny][nx] < 0)
	{
		/* Attack if possible */
		if (!(r_ptr->flags1 & (RF1_NEVER_BLOW)) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
		{
			(void)make_attack_normal(m_idx, !choose_to_attack_player(m_ptr));
		}

		/* End move */
		do_move = FALSE;
	}

	/* Get the move */
	mmove = place_monster_here(ny,nx,m_ptr->r_idx);

	/* Temporary pass wall */
	if ((m_ptr->tim_passw) && (mon_resist_feat(feat, m_ptr->r_idx))) mmove = MM_PASS;

	/* The monster is stuck in terrain e.g. cages */
	if (!(m_ptr->mflag & (MFLAG_OVER)) && !(f_info[cave_feat[oy][ox]].flags1 & (FF1_MOVE)) &&
		(place_monster_here(oy, ox, m_ptr->r_idx) <= MM_FAIL) && (mmove != MM_PASS))
	{
		/* Hack -- check old location */
		feat = cave_feat[oy][ox];
		ny = oy;
		nx = ox;

		/* Hack -- try to bash if allowed */
		if (r_ptr->flags1 & (FF1_BASH)) bash = TRUE;

		mmove = MM_FAIL;
	}

	/*
         * Monster is under covered terrain, moving to uncovered terrain.
         *
	 * This is for situations where a monster might be swimming around under ice, next to a floor square.
         * We don't allow it to easily move to the second location, but require that it bash up through the ice
	 * first. However, we do want to allow it to easily move from ice to normal deep water.
	 *
	 */
	if ((m_ptr->mflag & (MFLAG_HIDE)) && (f_info[cave_feat[oy][ox]].flags2 & (FF2_COVERED)) &&
		!(f_info[feat].flags2 & (FF2_COVERED)) && (mmove != MM_FAIL))
	{
		/* We can move normally */
		if ((mmove == MM_SWIM) || (mmove == MM_DIG) || (mmove == MM_PASS) || (mmove == MM_UNDER))
		{
			/* Move normally */
		}
		/* We can bash up */
		else if (bash_from_under(m_idx, oy, ox, &bash))
		{
			/* Can now see monster? */
			if (m_ptr->ml)
			{
				/* Notice */
				if (bash) did_bash_door = TRUE;
				else if (r_ptr->flags2 & (RF2_KILL_WALL)) did_kill_wall = TRUE;
				else if (r_ptr->flags3 & (RF3_HUGE)) did_huge = TRUE;
			}

			/* Do instead of moving */
			do_move = FALSE;
		}
		/* We can't otherwise move */
		else
		{
			return;
		}
	}

	/*
         * Monster is on covered terrain, moving to covered terrain.
         *
	 * This is for situations where a monster might be stuck on the surface of ice over water, but wants to get
         * back under so it can swim around in hiding.
	 *
	 */
	else if (!(m_ptr->mflag & (MFLAG_HIDE)) && (f_info[cave_feat[oy][ox]].flags2 & (FF2_COVERED)) &&
		(f_info[feat].flags2 & (FF2_COVERED)) &&
			((mmove == MM_SWIM) || (mmove == MM_DIG)))
	{
		/* Bash into new terrain */
		if (bash_from_under(m_idx, ny, nx, &bash))
		{
			/* Can now see monster */
			if (m_ptr->ml)
			{
				/* Notice */
				if (bash) did_bash_door = TRUE;
				else if (r_ptr->flags2 & (RF2_KILL_WALL)) did_kill_wall = TRUE;
				else if (r_ptr->flags3 & (RF3_HUGE)) did_huge = TRUE;
			}
		}

		else if ((r_ptr->flags2 & (RF2_CAN_FLY)) &&
			 (f_info[feat].flags2 & (FF2_CAN_FLY)))
		{
			mmove = MM_FLY;
		}

		else if (!(r_ptr->flags2 & (RF2_MUST_SWIM)) &&
			(mon_resist_feat(f_info[feat].mimic,m_ptr->r_idx)))
		{
			mmove = MM_WALK;
		}
		else
		{
			do_move = FALSE;
		}
	}

	/* Monster has to interact with terrain to try and pass it */
	else if (mmove <= MM_FAIL)
	{
		/* Glyphs */
		if (f_info[feat].flags1 & (FF1_GLYPH))
		{
			/* Describe observable breakage */
			if (play_info[ny][nx] & (PLAY_MARK))
			{
				msg_print("The rune of protection is broken!");
			}

			/* Break the rune */
			cave_alter_feat(ny, nx, FS_GLYPH);

		}

		/* Doors */
		if ((f_info[feat].flags1 & (FF1_BASH)) || (f_info[feat].flags1 & (FF1_OPEN)) ||
			  (f_info[feat].flags1 & (FF1_SECRET)))
		{
			/* Hack --- monsters find secrets */
			if (f_info[feat].flags1 & (FF1_SECRET)) cave_alter_feat(ny,nx,FS_SECRET);

			/* Monster walks through the door */
			if ((r_ptr->flags3 & (RF3_HUGE)) && (f_info[feat].flags1 & (FF1_TUNNEL)))
			{
				/* Character is not too far away */
				if (m_ptr->cdis < 30)
				{
					/* Message */
					if (f_info[feat].flags1 & (FF1_DOOR)) msg_print("You hear a door burst open!");
					else if (f_info[feat].flags3 & (FF3_TREE)) msg_print("You hear a tree crash down!");
					else msg_print("You hear wood split asunder!");

					/* Disturb */
					disturb(0, 0);
				}

				/* Break down the door */
				cave_alter_feat(ny, nx, FS_TUNNEL);

				/* Paranoia -- make sure we have bashed it */
				if (!(f_info[feat].flags1 & (FF1_MOVE)) && !(f_info[feat].flags3 & (FF3_EASY_CLIMB))) do_move = FALSE;

				/* Handle viewable doors */
				if (play_info[ny][nx] & (PLAY_SEEN))
				{
					/* Always disturb */
					disturb(0, 0);

					do_view = TRUE;
				}

				/* Disturb for non-viewable doors */
				else disturb(0, 0);

			}

			/* Monster bashes the door down */
			else if ((bash) && (f_info[feat].flags1 & (FF1_BASH)))
			{
				/* Character is not too far away */
				if (m_ptr->cdis < 30)
				{
					/* Message */
					if (f_info[feat].flags1 & (FF1_DOOR)) msg_print("You hear a door burst open!");
					else if (f_info[feat].flags3 & (FF3_TREE)) msg_print("You hear a tree crash down!");
					else msg_print("You hear wood split asunder!");

					/* Disturb */
					disturb(0, 0);
				}

				/* The door was bashed open */
				did_bash_door = TRUE;

				/* Break down the door */
				if (rand_int(100) < 50) cave_alter_feat(ny, nx, FS_OPEN);
				else cave_alter_feat(ny, nx, FS_BASH);

				/* Paranoia -- make sure we have bashed it */
				if (!(f_info[feat].flags1 & (FF1_MOVE)) && !(f_info[feat].flags3 & (FF3_EASY_CLIMB))) do_move = FALSE;

				/* Handle viewable doors */
				if (play_info[ny][nx] & (PLAY_SEEN))
				{
					/* Always disturb */
					disturb(0, 0);

					do_view = TRUE;
				}

				/* Disturb for non-viewable doors */
				else disturb(0, 0);
			}

			/* Monster opens the door */
			else if (f_info[feat].flags1 & (FF1_OPEN))
			{
				/* Unlock the door */
				cave_alter_feat(ny, nx, FS_OPEN);

				/* Do not move */
				do_move = FALSE;
			}
		}

		/* Hack --- smart or sneaky monsters try to disarm traps, unless berserk */
		else if ((f_info[feat].flags1 & (FF1_TRAP)) &&
			(r_ptr->flags2 & (RF2_SMART | RF2_SNEAKY)) && !(m_ptr->berserk))
		{
			int power, chance;

			/* Get trap "power" */
			power = f_info[feat].power;

			/* Base chance */
			chance = r_ptr->level;

			/* Apply intelligence */
			if (m_ptr->mflag & (MFLAG_STUPID)) chance /= 2;
			else if (m_ptr->mflag & (MFLAG_SMART)) chance = chance * 3 / 2;

			/* Apply dexterity */
			if (m_ptr->mflag & (MFLAG_CLUMSY)) chance /= 2;
			else if (m_ptr->mflag & (MFLAG_SKILLFUL)) chance = chance * 3 / 2;

			/* Break the ward */
			if (randint(chance) > power)
			{
				/* Describe hidden trap breakage */
				if ((feat == FEAT_INVIS) || (feat == FEAT_DOOR_INVIS))
				{
					/* Pick a trap */
					pick_trap(ny,nx, FALSE);

					/* Describe observable breakage */
					if ((play_info[ny][nx] & (PLAY_MARK)) && (m_ptr->ml))
					{
						char m_name[80];

						/* Get the monster name */
						monster_desc(m_name, sizeof(m_name), m_idx, 0);

						msg_format("%^s disarms the hidden %s.",m_name,f_name+f_info[feat].name);
					}

				}

				/* Describe observable breakage */
				else if (play_info[ny][nx] & (PLAY_MARK))
				{
					char m_name[80];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					msg_format("%^s disarms the %s.",m_name,f_name+f_info[feat].name);
				}

				/* Break the rune */
				cave_alter_feat(ny, nx, FS_DISARM);

				/* Use up time */
				do_move = FALSE;

				/* Did smart stuff */
				did_smart = (r_ptr->flags2 & (RF2_SMART)) != 0;
				did_sneak = (r_ptr->flags2 & (RF2_SNEAKY)) != 0;
			}

			/* Don't set off the ward */
			else if (randint(chance) > power)
			{
				do_move = FALSE;
			}
		}

		/* Monsters tunnel through impassable terrain */
		else if ((r_ptr->flags2 & (RF2_KILL_WALL)) && (f_info[feat].flags1 & (FF1_TUNNEL)))
		{
			/* Unlock the door */
			cave_alter_feat(ny, nx, FS_TUNNEL);

			/* Did kill wall */
			did_kill_wall = TRUE;
		}

		/* Uncrossable terrain */
		else if (!(f_info[feat].flags1 & (FF1_MOVE))) return;
	}

	/* Re-get the feature in the grid that the monster is trying to enter. */
	feat = cave_feat[ny][nx];

	/* Monster is allowed to move */
	if (do_move)
	{
		/* Attack adjacent monsters, if confused or enemies */
		if ((cave_m_idx[ny][nx] > 0) && ((m_ptr->confused) ||
			(choose_to_attack_monster(m_ptr, &m_list[cave_m_idx[ny][nx]]))))
		{
			monster_type *n_ptr = &m_list[cave_m_idx[ny][nx]];

			int ap_cnt;
			char m_name[80];
			bool hit = FALSE;

			do_move = FALSE;

			/* Target ignores player and retaliates */
			if (m_ptr->mflag & (MFLAG_ALLY)) n_ptr->mflag |= (MFLAG_IGNORE);

			/* Get the monster name (or "it") */
			monster_desc(m_name, sizeof(m_name), m_idx, 0x200);

			/* Notice if afraid */
			if (m_ptr->monfear)
			{
				/* Describe the attack */
				if (ally_messages) msg_format("%^s is afraid.", m_name);
			}
			
			/* Attack if not afraid */
			else
			{
				/* Scan through all four blows */
				for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
				{
					int damage = 0;

					char atk_desc[80];

					byte flg = 0;

					/* Extract the attack infomation */
					int effect = r_ptr->blow[ap_cnt].effect;
					int method = r_ptr->blow[ap_cnt].method;
					int d_dice = r_ptr->blow[ap_cnt].d_dice;
					int d_side = r_ptr->blow[ap_cnt].d_side;

					method_type *method_ptr = &method_info[method];

					bool do_cut = (method_ptr->flags2 & (PR2_CUTS)) != 0;
					bool do_stun = (method_ptr->flags2 & (PR2_STUN)) != 0;

					int who = m_ptr->mflag & (MFLAG_ALLY) ? SOURCE_PLAYER_ALLY : m_idx;
					int what = m_ptr->mflag & (MFLAG_ALLY) ? m_idx : ap_cnt;

					int ac = calc_monster_ac(cave_m_idx[ny][nx], FALSE);
					
					monster_lore *nl_ptr = &l_list[cave_m_idx[ny][nx]];
					
					int result;
					
					/* Hack -- no more attacks */
					if (!method)
					{
						/* Describe the attack */
						if ((!hit) && (ally_messages)) msg_format("%^s misses.", m_name);
						
						break;
					}

					/* We see the monster fighting */
					if ((l_ptr->blows[ap_cnt] < MAX_UCHAR) && (m_ptr->ml)) l_ptr->blows[ap_cnt]++;
					
					/* Hack -- ignore ineffective attacks */
					if (!effect) continue;
					
					/* We are attacking - count attacks */
					if ((nl_ptr->tblows < MAX_SHORT) && (m_ptr->ml)) nl_ptr->tblows++;

					/* Important: Don't use healing attacks against enemies */
					if ((effect == GF_HELLFIRE) && ((r_info[n_ptr->r_idx].flags3 & (RF3_DEMON)) != 0)) continue;
					if (((effect == GF_DRAIN_LIFE) || (effect == GF_VAMP_DRAIN)) && ((r_info[n_ptr->r_idx].flags3 & (RF3_UNDEAD)) != 0)) continue;

					/* Never display message XXX XXX XXX */
					if (!n_ptr->csleep && !mon_check_hit(cave_m_idx[ny][nx], effect, r_ptr->level, m_idx , FALSE)) continue;

					/* Roll out the damage. Note hack to make fights faster */
					damage = damroll(d_dice, d_side) * (n_ptr->csleep ? 4 : 2);

					/* Monster armor reduces total damage */
					damage -= (damage * ((ac < 150) ? ac: 150) / 250);

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

					/* Debugging - display attacks */
					result = attack_desc(atk_desc, cave_m_idx[ny][nx], method, effect, damage, flg, 80);

					/* Hack -- use cut or stun for resistance only */
					if (do_cut && do_stun)
					{
						/* Cancel cut */
						if ((m_idx % 11) < 4)
						{
							do_cut = FALSE;
						}

						/* Cancel stun */
						else
						{
							do_stun = FALSE;
						}
					}

					/* Check resistances */
					if (do_cut)
					{
						/* Immunity */
						if (((r_ptr->flags9 & (RF9_IM_EDGED)) != 0) && (rand_int(100) < 85)) continue;

						/* Resist */
						else if (((r_ptr->flags9 & (RF9_RES_EDGED)) != 0) && (rand_int(100) < 60)) continue;
					}
					else if (do_stun)
					{
						/* Immunity */
						if (((r_ptr->flags9 & (RF9_IM_BLUNT)) != 0) && (rand_int(100) < 85)) continue;

						/* Resist */
						else if (((r_ptr->flags9 & (RF9_RES_BLUNT)) != 0) && (rand_int(100) < 60)) continue;
					}
					else
					{
						/* Resist */
						if (((r_ptr->flags9 & (RF9_RES_MAGIC)) != 0) && (rand_int(100) < 60)) continue;
					}

					/* Describe the attack */
					if ((result >= 0) && (ally_messages)) msg_format("%^s %s", m_name, atk_desc);
					
					if (result >= 0) hit = TRUE;

					/* Notice we made an attack. This prevents others pushing into our position
					 * and allows fronts to form in combat in large groups */
					if (m_ptr->mflag & (MFLAG_ALLY)) m_ptr->mflag |= (MFLAG_PUSH);

					/* New result routine */
					project_one(who, what, ny, nx, damage, effect, (PROJECT_KILL | PROJECT_HIDE));
				}
			}
		}

		/* Never move if petrified or never move monster */
		else if ((r_ptr->flags1 & (RF1_NEVER_MOVE)) || (m_ptr->petrify))
		{
			do_move = FALSE;
		}

		/* Monster has to climb the grid slowly */
		else if ((mmove == MM_CLIMB) && !(r_ptr->flags2 & (RF2_CAN_CLIMB)) && !(m_ptr->mflag & (MFLAG_OVER))
			&& !(m_ptr->mflag & (MFLAG_HIDE)))
		{
			/* Climb */
			m_ptr->mflag |= (MFLAG_OVER);

			/* Climbing takes a turn */
			do_move = FALSE;
		}

		/* Monster flies or climbs over other monsters */
		else if ((mmove == MM_FLY) || (mmove == MM_CLIMB))
		{
			/* Move over, but not through ceiling */
			if (!(m_ptr->mflag & (MFLAG_HIDE))) m_ptr->mflag |= (MFLAG_OVER);

			/* Don't move monsters underneath ceiling otherwise */
			else do_move = FALSE;
		}

		/* The grid is occupied by a monster. */
		else if (cave_m_idx[ny][nx] > 0)
		{
			monster_type *n_ptr = &m_list[cave_m_idx[ny][nx]];

			/* The other monster cannot switch places */
			if (!cave_exist_mon(n_ptr->r_idx, m_ptr->fy, m_ptr->fx, TRUE))
			{
				/* Try to push it aside */
				if (!push_aside(m_ptr->fy, m_ptr->fx, n_ptr))
				{
					/* Cancel move on failure */
					do_move = FALSE;
				}
			}

			/* Mark monsters as pushed */
			if (do_move)
			{
				/* Monster has pushed */
				m_ptr->mflag |= (MFLAG_PUSH);

				/* Monster has been pushed aside */
				n_ptr->mflag |= (MFLAG_PUSH);
			}

			/* Monsters trade objects */
			if (m_ptr->hold_o_idx)
			{
				/* For efficiency in large groups of archers */
				bool swap = TRUE;

				/* Average the size of top of stacks if stackable */
				if (n_ptr->hold_o_idx)
				{
					object_type *o_ptr = &o_list[m_ptr->hold_o_idx];
					object_type *j_ptr = &o_list[n_ptr->hold_o_idx];

					if (object_similar(o_ptr, j_ptr))
					{
						int n = (o_ptr->number + j_ptr->number) / 2;
						int c = (o_ptr->number + j_ptr->number) % 2;

						o_ptr->number = n;
						j_ptr->number = n + c;

						swap = FALSE;
					}
				}

				/* Give some ammunition to archers */
				/* Not we do this as a give, because monsters in front tend to run out of
				 * ammunition, and monsters from behind tend to push.
				 * We also act slightly greedy and hold onto 1 object, otherwise the
				 * archers end up carrying with 1 carrying 1 huge stack */
				if ((swap) && (r_info[n_ptr->r_idx].flags2 & (RF2_ARCHER)) &&
						(o_list[m_ptr->hold_o_idx].next_o_idx))
				{
					/* Swap stacks. Identify ammunition. Either give the whole stack
					 * back, or bottom below the ammo. */
					int temp_o_idx = n_ptr->hold_o_idx;
					int ammo;

					n_ptr->hold_o_idx = m_ptr->hold_o_idx;
					m_ptr->hold_o_idx = temp_o_idx;

					/* Find ammo in stack */
					ammo = find_monster_ammo(cave_m_idx[ny][nx], -1, FALSE);

					/* Ammunition */
					if (ammo >= 0)
					{
						object_type *o_ptr = &o_list[o_list[ammo].next_o_idx];
						object_type *j_ptr = &o_list[ammo];

						int this_o_idx, next_o_idx;

						if (cheat_xtra) msg_format("Debug before swap: ammo %d, ammo next %d, temp %d, m_ptr->held %d, n_ptr->held %d", ammo, o_list[ammo].next_o_idx, temp_o_idx, m_ptr->hold_o_idx, n_ptr->hold_o_idx);

						/* Very carefully */
						temp_o_idx = o_list[ammo].next_o_idx;
						o_list[ammo].next_o_idx = m_ptr->hold_o_idx;
						m_ptr->hold_o_idx = temp_o_idx;
						n_ptr->hold_o_idx = ammo;

						/* Fix monster held. This is for efficiency. */
						for (this_o_idx = ammo; this_o_idx; this_o_idx = next_o_idx)
						{
							/* Get the object */
							o_ptr = &o_list[this_o_idx];

							/* Get the next object */
							next_o_idx = o_ptr->next_o_idx;

							/* Manually correct this */
							o_list[this_o_idx].held_m_idx = cave_m_idx[ny][nx];
						}

						/* Paranoia */
						/* Hack -- encourage archers to carry 2 stacks */
						if ((o_list[ammo].next_o_idx) && (o_list[o_list[ammo].next_o_idx].next_o_idx))
						{
							/* Get next in stack */
							o_ptr = &o_list[o_list[ammo].next_o_idx];

							/* Combine if possible */
							if (object_similar(o_ptr, j_ptr))
							{
								/* Paranoia */
								temp_o_idx = o_list[ammo].next_o_idx;

								object_absorb(o_ptr, j_ptr, TRUE);
								n_ptr->hold_o_idx = temp_o_idx;

								if (cheat_xtra) msg_format("Debug after swap and absorb: m_ptr->held %d, n_ptr->held %d", m_ptr->hold_o_idx, n_ptr->hold_o_idx);
							}
							else if (cheat_xtra) msg_format("Debug after swap: m_ptr->held %d, n_ptr->held %d, n_ptr->held next %d", m_ptr->hold_o_idx, n_ptr->hold_o_idx, o_list[n_ptr->hold_o_idx].next_o_idx);
						}
						else if (cheat_xtra) msg_format("Debug after swap: m_ptr->held %d, n_ptr->held %d, n_ptr->held next %d", m_ptr->hold_o_idx, n_ptr->hold_o_idx, o_list[n_ptr->hold_o_idx].next_o_idx);
					}
					/* No ammunition - swap back */
					else
					{
						temp_o_idx = m_ptr->hold_o_idx;
						m_ptr->hold_o_idx = n_ptr->hold_o_idx;
						n_ptr->hold_o_idx = temp_o_idx;
					}
				}
			}
		}
	}

	/* Monster can (still) move */
	if (do_move)
	{
		/* Hidden ? */
		bool hidden = ((m_ptr->mflag & (MFLAG_HIDE))!=0);

		/* Hide monster if allowed */
		monster_hide(ny,nx,mmove,m_ptr);

		/* Hack --- tell the player if something hides */
		if (!(hidden) && (m_ptr->mflag & (MFLAG_HIDE)) && (m_ptr->ml))
		{
			char m_name[80];

			/* Get the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0);

			msg_format("%^s hides in %s%s.",m_name,
			((f_info[feat].flags2 & (FF2_FILLED))?"":"the "),
			f_name+f_info[feat].name);
		}

		/* Move the monster */
		monster_swap(oy, ox, ny, nx);

		/* Cancel target when reached */
		if ((m_ptr->ty == ny) && (m_ptr->tx == nx))
		{
			m_ptr->ty = 0;
			m_ptr->tx = 0;
		}

		/* Hack --- tell the player if something unhides */
		if ((hidden) && !(m_ptr->mflag & (MFLAG_HIDE)) && (m_ptr->ml))
		{
			char m_name[80];

			/* Get the monster name */
			monster_desc(m_name, sizeof(m_name), m_idx, 0);

			msg_format("%^s emerges from %s%s.",m_name,
			((f_info[cave_feat[oy][ox]].flags2 & (FF2_FILLED))?"":"the "),
			f_name+f_info[cave_feat[oy][ox]].name);

			/* If flying or climbing, start over */
			if ((mmove == MM_CLIMB) || (mmove == MM_FLY)) m_ptr->mflag |= (MFLAG_OVER);
		}

		/* Possible disturb */
		if ((m_ptr->ml &&
		    (disturb_move ||
		     ((m_ptr->mflag & (MFLAG_VIEW)) &&
		      disturb_near)))
		      && ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
		{
			/* Disturb */
			disturb(0, 0);
		}

		/* Get hit by regions */
		if (cave_region_piece[ny][nx])
		{
			/* Trigger the region */
			trigger_region(ny, nx, TRUE);
		}

		/* Hit traps */
		if (f_info[feat].flags1 & (FF1_HIT_TRAP))
		{
			mon_hit_trap(m_idx,ny,nx);
		}
		/* Hit other terrain */
		else if ((!mon_resist_feat(feat,m_ptr->r_idx)) &&
			!(m_ptr->mflag & (MFLAG_OVER)))
		{
			mon_hit_trap(m_idx,ny,nx);
		}

		/* XXX XXX Note we don't hit the old monster with traps/terrain */

		/* Leave tracks */
		if ((f_info[feat].flags2 & (FF2_KILL_HUGE)) && (r_ptr->flags3 & (RF3_HUGE)))
		{
			if (!(m_ptr->mflag & (MFLAG_OVER))) cave_alter_feat(ny, nx, FS_KILL_HUGE);
		}
		else if (f_info[feat].flags2 & (FF2_KILL_MOVE))
		{
			if (!(m_ptr->mflag & (MFLAG_OVER))) cave_alter_feat(ny, nx, FS_KILL_MOVE);
		}
		else if ((f_info[cave_feat[oy][ox]].flags1 & (FF1_FLOOR)) && (!(m_ptr->mflag & (MFLAG_HIDE))))
		{
			if ((r_ptr->flags8 & (RF8_HAS_BLOOD)) && (m_ptr->hp < m_ptr->maxhp / 3) && (rand_int(100) < 30))
				cave_set_feat(oy, ox, FEAT_FLOOR_BLOOD_T);
			else if (!(r_ptr->flags9 & (RF9_NO_CUTS)) && (m_ptr->hp < m_ptr->maxhp / 3) && (rand_int(100) < 30))
				cave_set_feat(oy, ox, FEAT_FLOOR_SLIME_T);
			else if (r_ptr->flags8 & (RF8_HAS_SLIME))
				cave_set_feat(oy, ox, FEAT_FLOOR_SLIME_T);
		}

		/* Reget the feature */
		feat = cave_feat[ny][nx];

		/*
		 * If a monster capable of smelling hits a
		 * scent trail while out of LOS of the character, it will
		 * communicate this to similar monsters.
		 */
		if ((!player_has_los_bold(ny, nx)) &&
		    (monster_can_smell(m_ptr)) && (get_scent(oy, ox) == -1) &&
		    (!m_ptr->ty) && (!m_ptr->tx))
		{
			tell_allies_target(m_ptr->fy, m_ptr->fx, ny, nx, TRUE, "I have found a scent.");
		}

		/* Player will always be disturbed if a monster is adjacent */
		if ((m_ptr->cdis == 1)
      		&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
		{
			disturb(1, 0);
		}

		/* Possible disturb */
		else if ((m_ptr->ml && (disturb_move ||
			(m_ptr->mflag & (MFLAG_VIEW) && disturb_near)))
			&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
		{
			/* Disturb */
			disturb(0, 0);
		}


		/* Scan all objects in the grid */

		/* Can we get the objects */
		if ((f_info[feat].flags1 & (FF1_DROP)) != 0)
		{
			/* Check all objects in grid */
			for (this_o_idx = cave_o_idx[ny][nx]; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;

				/* Acquire object */
				o_ptr = &o_list[this_o_idx];

				/* Acquire next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Skip gold unless an ally or townsfolk */
				if ((o_ptr->tval >= TV_GOLD) && ((m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN)) != 0)) continue;

				/* Sneaky monsters hide behind big objects */
				if ((o_ptr->weight > 1500)
					&& (r_ptr->flags2 & (RF2_SNEAKY))
					&& !(m_ptr->mflag & (MFLAG_HIDE)))
				{
					char m_name[80];
					char o_name[80];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					/* Get the object name */
					object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

					msg_format("%^s hides behind %s.",m_name, o_name);

					m_ptr->mflag |= (MFLAG_HIDE);

					did_sneak = TRUE;
				}

				/* Take objects on the floor */
				if (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_EAT_BODY))
				{
					u32b f1, f2, f3, f4;

					u32b flg3 = 0L;

					char m_name[80];
					char o_name[120];

					/* Extract some flags */
					object_flags(o_ptr, &f1, &f2, &f3, &f4);

					/* Acquire the object name */
					object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

					/* Acquire the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0x04);

					/* React to objects that hurt the monster */
					if (f1 & (TR1_SLAY_DRAGON)) flg3 |= (RF3_DRAGON);
					if (f1 & (TR1_SLAY_TROLL)) flg3 |= (RF3_TROLL);
					if (f1 & (TR1_SLAY_GIANT)) flg3 |= (RF3_GIANT);
					if (f1 & (TR1_SLAY_ORC)) flg3 |= (RF3_ORC);
					if (f1 & (TR1_SLAY_DEMON)) flg3 |= (RF3_DEMON);
					if (f1 & (TR1_SLAY_UNDEAD)) flg3 |= (RF3_UNDEAD);
					if (f1 & (TR1_SLAY_NATURAL)) flg3 |= (RF3_ANIMAL | RF3_PLANT | RF3_INSECT);
					if (f1 & (TR1_BRAND_HOLY)) flg3 |= (RF3_EVIL);

					/* The object cannot be picked up by the monster */
					if (artifact_p(o_ptr) || ((r_ptr->flags3 & flg3) != 0))
					{
						/* Only give a message for "take_item" */
						if (r_ptr->flags2 & (RF2_TAKE_ITEM))
						{
							/* Take note */
							did_take_item = TRUE;

							/* Describe observable situations */
							if (m_ptr->ml && player_has_los_bold(ny, nx))
							{
								if (!auto_pickup_ignore(o_ptr))
								{
									/* Dump a message */
									msg_format("%^s tries to pick up %s, but fails.",
										   m_name, o_name);
								}

								/* Mark object as ungettable? */
								if (!(o_ptr->feeling) &&
									!(o_ptr->ident & (IDENT_SENSE))
									&& !(object_named_p(o_ptr)))
								{

									/* Sense the object */
									o_ptr->feeling = INSCRIP_UNGETTABLE;

									/* The object has been "sensed" */
									o_ptr->ident |= (IDENT_SENSE);
								}

								/* Invert the flags */
								if ((l_ptr->flags3 & (RF3_DRAGON)) == 0) f1 &= ~(TR1_SLAY_DRAGON);
								if ((l_ptr->flags3 & (RF3_TROLL)) == 0) f1 &= ~(TR1_SLAY_TROLL);
								if ((l_ptr->flags3 & (RF3_GIANT)) == 0) f1 &= ~(TR1_SLAY_GIANT);
								if ((l_ptr->flags3 & (RF3_ORC)) == 0) f1 &= ~(TR1_SLAY_ORC);
								if ((l_ptr->flags3 & (RF3_DEMON)) == 0) f1 &= ~(TR1_SLAY_DEMON);
								if ((l_ptr->flags3 & (RF3_UNDEAD)) == 0) f1 &= ~(TR1_SLAY_UNDEAD);
								if ((l_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT)) == 0) f1 &= (TR1_SLAY_NATURAL);
								if ((l_ptr->flags3 & (RF3_EVIL)) == 0) f1 &= ~(TR1_BRAND_HOLY);

								/* Learn about flags on the object */
								object_can_flags(o_ptr, f1, 0L, 0L, 0L, TRUE);
							}
						}
					}

					/* Hack -- do not pick up bodies unless going to eat them */
					else if ((o_ptr->tval == TV_BODY) && !(r_ptr->flags2 & (RF2_EAT_BODY)))
					{
						/* Do nothing */
					}

					/* Pick up the item */
					/* Hack -- eat body monsters can carry one body */
					else if (((r_ptr->flags2 & (RF2_TAKE_ITEM)) != 0) || (((r_ptr->flags2 & (RF2_EAT_BODY)) != 0)
						&& !(m_ptr->hold_o_idx) && (o_ptr->tval == TV_BODY)))
					{
						object_type *i_ptr;
						object_type object_type_body;

						/* Take note */
						did_take_item = TRUE;

						/* Describe observable situations */
						if (player_has_los_bold(ny, nx) && !auto_pickup_ignore(o_ptr))
 						{
							/* Dump a message */
							msg_format("%^s picks up %s.", m_name, o_name);
						}

						/* Get local object */
						i_ptr = &object_type_body;

						/* Obtain local object */
						object_copy(i_ptr, o_ptr);

						/* Delete the object */
						delete_object_idx(this_o_idx);

						/* Carry the object */
						(void)monster_carry(cave_m_idx[m_ptr->fy][m_ptr->fx], i_ptr);
					}
				}
			}
		}
	}	     /* End of monster's move */

	/* Notice changes in view */
	if (do_view)
	{
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Learn things from observable monster */
	if (m_ptr->ml)
	{
		/* Monster opened a door */
		if (did_open_door) l_ptr->flags2 |= (RF2_OPEN_DOOR);

		/* Monster bashed a door */
		if (did_bash_door) l_ptr->flags2 |= (RF2_BASH_DOOR);

		/* Monster tried to pick something up */
		if (did_take_item) l_ptr->flags2 |= (RF2_TAKE_ITEM);

		/* Monster passed through a wall */
		/* XXX Temporary spell to pass wall so need to check */
		if ((did_pass_wall)  && (r_ptr->flags2 & RF2_PASS_WALL)) l_ptr->flags2 |= (RF2_PASS_WALL);

		/* Monster destroyed a wall */
		if (did_kill_wall) l_ptr->flags2 |= (RF2_KILL_WALL);

		/* Monster disarmed a trap */
		if (did_smart) l_ptr->flags2 |= (RF2_SMART);

		/* Monster hide behind an object */
		if (did_sneak) l_ptr->flags2 |= (RF2_SNEAKY);

		/* Monster utilised its 'hugeness' */
		if (did_huge) l_ptr->flags3 |= (RF3_HUGE);

		/* Monster is climbing */
		/* XXX Rubble, trees, webs will always result in climbing, so need to check */
		if ((mmove == MM_CLIMB) && (r_ptr->flags2 & RF2_CAN_CLIMB)) l_ptr->flags2 |= (RF2_CAN_CLIMB);

		/* Monster is flying */
		if (mmove == MM_FLY) l_ptr->flags2 |= (RF2_CAN_FLY);

		/* Monster must swim */
		if ((mmove == MM_FLY) && (r_ptr->flags2 & (RF2_MUST_FLY))) l_ptr->flags2 |= (RF2_MUST_FLY);

		/* Monster is swimming */
		if (mmove == MM_SWIM) l_ptr->flags2 |= (RF2_CAN_SWIM);

		/* Monster must swim */
		if ((mmove == MM_SWIM) && (r_ptr->flags2 & (RF2_MUST_SWIM))) l_ptr->flags2 |= (RF2_MUST_SWIM);

		/* Monster is digging */
		if (mmove == MM_DIG) l_ptr->flags2 |= (RF2_CAN_DIG);

		/* Monster is oozing */
		if (mmove == MM_OOZE) l_ptr->flags3 |= (RF3_OOZE);

		/* Monster is passing */
		if (mmove == MM_PASS)
		{
			if (m_ptr->tim_passw) l_ptr->flags6 |= (RF6_WRAITHFORM);
			else l_ptr->flags2 |= (RF2_PASS_WALL);
		}
	}
}


/*
 * Monster gets fed
 */
void feed_monster(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int hp;

	/* Recover stat */
	/* Note that we use a hack to make hungry attacks make monsters weak. The ordering here
	   makes monsters that are weak the least likely to recover this. Therefore hungry monsters
	   will eat more than other monsters. */
	switch(rand_int(5))
	{
		case 0: if ((m_ptr->mflag & (MFLAG_WEAK)) != 0) {  m_ptr->mflag &= ~(MFLAG_WEAK); break; }
		case 1: if ((m_ptr->mflag & (MFLAG_SICK)) != 0)
			{
				m_ptr->mflag &= ~(MFLAG_SICK);
				hp = calc_monster_hp(m_idx);
				if (m_ptr->maxhp < hp) { m_ptr->maxhp = hp; break; }
			}
		case 2: if ((m_ptr->mflag & (MFLAG_CLUMSY)) != 0) {  m_ptr->mflag &= ~(MFLAG_CLUMSY); break; }
		case 3: if ((m_ptr->mflag & (MFLAG_STUPID)) != 0) {  m_ptr->mflag &= ~(MFLAG_STUPID); break; }
		case 4: if ((m_ptr->mflag & (MFLAG_NAIVE)) != 0) {  m_ptr->mflag &= ~(MFLAG_NAIVE); break; }
	}

	/* All monsters recover hit points */
	if (m_ptr->hp < m_ptr->maxhp)
	{
		/* Base regeneration */
		int frac = m_ptr->maxhp / 100;

		/* Minimal regeneration rate */
		if (!frac) frac = 1;

		/* Some monsters regenerate quickly */
		if (r_ptr->flags2 & (RF2_REGENERATE)) frac *= 2;

		/* Regenerate */
		m_ptr->hp += frac;

		/* Do not over-regenerate */
		if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

		/* Fully healed -> flag minimum range for recalculation */
		if (m_ptr->hp == m_ptr->maxhp) m_ptr->min_range = 0;
	}

	/* Hack -- trolls recover more hit points */
	if (r_ptr->flags3 & (RF3_TROLL))
	{
		m_ptr->hp += m_ptr->maxhp / 30;
		if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

		/* Fully healed -> flag minimum range for recalculation */
		if (m_ptr->hp == m_ptr->maxhp) m_ptr->min_range = 0;
	}
}


/*
 * Monster takes its turn.
 */
static void process_monster(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int i, k, y, x;
	int ty, tx;

	int chance_innate = 0;
	int chance_spell = 0;
	int choice = 0;

	int dir;
	bool fear = FALSE;

	bool bash;

	/* Assume the monster is able to perceive the player. */
	bool aware = TRUE;
	bool must_use_target = FALSE;

	/* Will the monster move randomly? */
	bool random = FALSE;

	/* Monster can act - Reset push flag */
	m_ptr->mflag &= ~(MFLAG_PUSH);

	/* If monster is sleeping, or in stasis, it loses its turn. */
	if (m_ptr->csleep) return;

	/* Calculate the monster's preferred combat range when needed */
	if (m_ptr->min_range == 0) find_range(m_idx);

	/* Monster is in active mode. */
	if (m_ptr->mflag & (MFLAG_ACTV))
	{
		/*
		 * Character is outside of scanning range and well outside
		 * of sighting range.  Monster does not have a target.
		 */
		if ((m_ptr->cdis >= FLEE_RANGE) && (m_ptr->cdis > r_ptr->aaf) &&
		    (!m_ptr->ty) && (!m_ptr->tx))
		{
			/* Monster cannot smell the character */
			if (!cave_when[m_ptr->fy][m_ptr->fx]) m_ptr->mflag &= ~(MFLAG_ACTV);
			else if (!monster_can_smell(m_ptr))   m_ptr->mflag &= ~(MFLAG_ACTV);
		}
	}

	/* Monster is in passive mode. */
	else
	{
		/* Character is inside scanning range */
		if (m_ptr->cdis <= r_ptr->aaf) m_ptr->mflag |= (MFLAG_ACTV);

		/* Monster has a target */
		else if ((m_ptr->ty) && (m_ptr->tx)) m_ptr->mflag |= (MFLAG_ACTV);

		/* Monster will get a target */
		else if (m_ptr->mflag & (MFLAG_TOWN | MFLAG_ALLY | MFLAG_IGNORE)) m_ptr->mflag |= (MFLAG_ACTV);

		/* The monster is catching too much of a whiff to ignore */
		else if (cave_when[m_ptr->fy][m_ptr->fx])
		{
			if (monster_can_smell(m_ptr))
			{
				m_ptr->mflag |= (MFLAG_ACTV);
			}
		}
	}

	/*
	 * Special behaviour for when character is not carrying a lite, and monster
	 * either needs lite or the player is invisible and the monster can't see
	 * invisibile. Note that we assume that player temporary invisibility
	 * also makes them cold-blooded.
	 */
	if (!(p_ptr->cur_lite) && ((r_ptr->flags2 & (RF2_NEED_LITE)) ||
			((r_ptr->d_char != 'e') && ((r_ptr->flags9 & (RF9_RES_BLIND)) == 0) && (p_ptr->timed[TMD_INVIS]))))
	{
		/* Character is not directly visible */
		if (!player_can_fire_bold(m_ptr->fy, m_ptr->fx))
		{
			/* Monster loses awareness */
			aware = FALSE;
		}

		/* Character is in darkness and monster is active */
		else if ((m_ptr->mflag & (MFLAG_ACTV)) && ((!(cave_info[p_ptr->py][p_ptr->px] & (CAVE_LITE)))
				|| ((r_ptr->d_char != 'e') && ((r_ptr->flags9 & (RF9_RES_BLIND)) == 0) && (p_ptr->timed[TMD_INVIS]))))
		{
			/* Lite up */
			if ((r_ptr->flags2 & (RF2_NEED_LITE)) && !(m_ptr->mflag & (MFLAG_LITE)))
			{
				m_ptr->mflag |= (MFLAG_LITE);

				gain_attribute(m_ptr->fy, m_ptr->fx, 2, CAVE_XLOS, apply_torch_lit, redraw_torch_lit_gain);
			}

			/* Player hasn't attacked the monster */
			if (!(m_ptr->mflag & (MFLAG_HIT_RANGE | MFLAG_HIT_BLOW))) aware = FALSE;
		}

		/* Player has attacked the monster */
		if (((m_ptr->mflag & (MFLAG_HIT_RANGE | MFLAG_HIT_BLOW)) != 0) &&

				/* Hack -- ensure monster cannot move before getting target */
				((r_ptr->flags1 & (RF1_NEVER_MOVE | RF1_NEVER_BLOW)) == 0) &&
			!(m_ptr->petrify))
		{
			m_ptr->ty = p_ptr->py;
			m_ptr->tx = p_ptr->px;
		}
	}

	/*
	 * Special handling if the first turn a monster has after
	 * being attacked by the player, but the player is out of sight
	 */
	if (m_ptr->mflag & (MFLAG_HIT_RANGE))
	{
		/* Monster will be very upset if it can't shoot the player
		   or wasn't expecting player attack. */
		if ((!player_can_fire_bold(m_ptr->fy, m_ptr->fx)) || ((m_ptr->mflag & (MFLAG_TOWN)) != 0))
		{
			m_ptr->mflag |= (MFLAG_AGGR);

			/* Tell allies to get aggressive or close */
			if ((tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), (aware) ? "& has attacked me!" : "Something has attacked me!"))
					||(tell_allies_range(m_ptr->fy, m_ptr->fx, 1, 0, (aware) ? "& has attacked me!" : "Something has attacked me!")))
			{
				/* Close oneself */
				if (m_ptr->min_range > 1) m_ptr->min_range--;
				m_ptr->best_range = 1;

				/* Sometimes feel less aggrieved */
				if (!rand_int(3)) m_ptr->mflag &= ~(MFLAG_TOWN | MFLAG_AGGR);
			}
			/* No allies to tell -- try to close */
			else
			{
				/* Close oneself */
				if (m_ptr->min_range > 1) m_ptr->min_range--;
				m_ptr->best_range = 1;
			}

			/*Tweak the monster speed*/
			if (!player_has_los_bold(m_ptr->fy, m_ptr->fx))
			{
				/*Monsters with ranged attacks will try to cast a spell, shoot or breath*/
				if (r_ptr->freq_spell || r_ptr->freq_innate) m_ptr->mflag |= (MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH);

				m_ptr->mflag &= ~(MFLAG_SLOW | MFLAG_FAST);
				if (!rand_int(3))	m_ptr->mflag |= (MFLAG_SLOW);
				else if (rand_int(2)) m_ptr->mflag |= (MFLAG_FAST);
				m_ptr->mspeed = calc_monster_speed(m_idx);
			}
		}
		/* Invisible monsters notice if the player can see invisible */
		else if ((((r_ptr->flags2 & (RF2_INVISIBLE)) != 0) || (m_ptr->tim_invis)) &&
					(((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0) || (p_ptr->timed[TMD_SEE_INVIS]) ||
					 (((r_ptr->flags2 & (RF2_COLD_BLOOD)) == 0) && (p_ptr->see_infra >= m_ptr->cdis))))
		{
			/* Tell allies as well */
			if (((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0) || (p_ptr->timed[TMD_SEE_INVIS]))
			{
				update_smart_learn(m_idx, (SM_SEE_INVIS));
			}
			/* Just quietly note we're too close and warm blooded */
			else
			{
				m_ptr->smart |= (SM_SEE_INVIS);
			}
		}

		/* Clear the ignore flag */
		m_ptr->mflag &= ~(MFLAG_IGNORE);

		/* Clear the flag unless player is murderer */
		if (!(m_ptr->mflag & (MFLAG_TOWN))) m_ptr->mflag &= ~(MFLAG_HIT_RANGE);

	}

	/*This if the first turn a monster has after being attacked by the player*/
	if (m_ptr->mflag & (MFLAG_HIT_BLOW))
	{
		/*
		 * Monster will be very upset if it isn't next to the
		 * player (pillar dance, hack-n-back, etc)
		 */
		if ((m_ptr->cdis > 1) || ((m_ptr->mflag & (MFLAG_TOWN)) != 0))
		{
			m_ptr->mflag |= (MFLAG_AGGR);

			/* Tell allies to get aggressive or close */
			if ((tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), (aware) ? "& has attacked me!" : "Something has attacked me!"))
					|| (tell_allies_range(m_ptr->fy, m_ptr->fx, 1, 0, (aware) ? "& has attacked me!" : "Something has attacked me!")))
			{
				/* Close oneself if only option is melee */
				if (!r_ptr->freq_spell && !r_ptr->freq_innate) m_ptr->min_range = m_ptr->best_range = 1;

				/* Sometimes feel less aggrieved */
				if (!rand_int(3)) m_ptr->mflag &= ~(MFLAG_TOWN | MFLAG_AGGR);
			}
			/* No allies to tell -- try to back off / flee */
			else
			{
				/* Don't keep closing if we can use range attacks */
				if ((m_ptr->hp >= m_ptr->maxhp / 3) && (r_ptr->freq_spell || r_ptr->freq_innate))
				{
					if (m_ptr->best_range < m_ptr->cdis) m_ptr->best_range = m_ptr->cdis;
				}
				/* Flee if getting hurt */
				else
				{
					m_ptr->min_range = m_ptr->best_range = MAX_SIGHT;
				}
			}

			/*Tweak the monster speed*/
			if (m_ptr->cdis > 1)
			{
				/* Monsters with ranged attacks will try to cast a spell*/
				if (r_ptr->freq_spell || r_ptr->freq_innate) m_ptr->mflag |= (MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH);

				m_ptr->mflag &= ~(MFLAG_SLOW | MFLAG_FAST);
				if (!rand_int(3))	m_ptr->mflag |= (MFLAG_SLOW);
				else if (rand_int(2)) m_ptr->mflag |= (MFLAG_FAST);
				m_ptr->mspeed = calc_monster_speed(m_idx);
			}
		}
		/* Invisible monsters notice if the player can see invisible */
		else if ((((r_ptr->flags2 & (RF2_INVISIBLE)) != 0) || (m_ptr->tim_invis)) &&
					(((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0) || (p_ptr->timed[TMD_SEE_INVIS]) ||
					 (((r_ptr->flags2 & (RF2_COLD_BLOOD)) == 0) && (p_ptr->see_infra >= m_ptr->cdis))))
		{
			/* Tell allies as well */
			if (((p_ptr->cur_flags3 & (TR3_SEE_INVIS)) != 0) || (p_ptr->timed[TMD_SEE_INVIS]))
			{
				update_smart_learn(m_idx, (SM_SEE_INVIS));
			}
			/* Just quietly note we're too close and warm blooded */
			else
			{
				m_ptr->smart |= (SM_SEE_INVIS);
			}
		}

		/* Clear the ignore flag */
		m_ptr->mflag &= ~(MFLAG_IGNORE);

		/* Clear the flag unless player is murderer */
		if (!(m_ptr->mflag & (MFLAG_TOWN))) m_ptr->mflag &= ~(MFLAG_HIT_BLOW);
	}

	/* A monster in passive mode will end its turn at this point. */
	if (!(m_ptr->mflag & (MFLAG_ACTV))) return;

	/* Hack -- Always redraw the current target monster health bar */
	if (p_ptr->health_who == cave_m_idx[m_ptr->fy][m_ptr->fx])
		p_ptr->redraw |= (PR_HEALTH);


	/* Attempt to multiply if able to and allowed */
	if ((r_ptr->flags2 & (RF2_MULTIPLY)) && (num_repro < MAX_REPRO))
	{
		/* Count the adjacent monsters */
		for (k = 0, y = m_ptr->fy - 1; y <= m_ptr->fy + 1; y++)
		{
			for (x = m_ptr->fx - 1; x <= m_ptr->fx + 1; x++)
			{
				/* Check Bounds */
				if (!in_bounds(y, x)) continue;

				/* Count monsters */
				if (cave_m_idx[y][x] > 0) k++;
			}
		}

		/* Hack -- multiply slower in crowded areas */
		if ((k < 4) && (!k || !rand_int(k * MON_MULT_ADJ)))
		{
			/* Try to multiply */
			if (multiply_monster(m_idx))
			{
				/* Take note if visible */
				if (m_ptr->ml)
				{
					l_ptr->flags2 |= (RF2_MULTIPLY);
				}

				/* Multiplying takes energy */
				return;
			}
		}
	}

	/*** Ranged attacks ***/

	/* Extract the ranged attack probability. */
	chance_innate = r_ptr->freq_innate;
	chance_spell = r_ptr->freq_spell;

	/* Cannot use ranged attacks beyond maximum range. */
	if ((chance_innate) && (m_ptr->cdis > MAX_RANGE + 1)) chance_innate = 0;
	if ((chance_spell) && (m_ptr->cdis > MAX_RANGE + 1)) chance_spell = 0;

	/* Cannot use spell attacks when enraged or not aware. */
	if ((chance_spell) && ((m_ptr->berserk) || (!aware) )) chance_spell = 0;

	/* Cannot use innate attacks when not aware. */
	if ((chance_innate) && (!aware)) chance_innate = 0;

	/* Blind, confused, amnesiac and stunned monsters use spell attacks half as often. */
	if ((m_ptr->blind) || (m_ptr->confused) || (m_ptr->stunned) || (m_ptr->amnesia))
	{
		chance_spell /= 2;
		chance_innate /= 2;
	}

	/* Blind and amnesiac monsters cannot cast spells */
	if ((m_ptr->blind) && (m_ptr->amnesia))
	{
		chance_spell = 0;
	}

	/* Monster can use ranged attacks */
	/* Now use a 'save' against the players charisma to avoid this */
	if ((chance_innate) || (chance_spell)
		|| ((r_ptr->flags3 & (RF3_HUGE)) && (m_ptr->cdis == 2))
		|| (m_ptr->mflag & (MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH)))
	{
		int roll;

		/* Monster must cast */
		if (m_ptr->mflag & (MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH))
		{
			roll = 0;
		}

		/* Aggressive/allied monsters use ranged attacks more frequently */
		else if (m_ptr->mflag & (MFLAG_AGGR | MFLAG_ALLY | MFLAG_IGNORE))
		{
			roll = rand_int(200);
			if (chance_innate) chance_innate += 100;
			if (chance_spell) chance_spell += 100;
		}
		/* Normal chance based on charisma */
		else
		{
			roll = rand_int(adj_chr_taunt[p_ptr->stat_ind[A_CHR]]);
		}

		/* Pick a ranged attack */
		if ((roll < chance_innate) || (roll < chance_spell)
			|| ((r_ptr->flags3 & (RF3_HUGE)) && (roll < 50) && (m_ptr->cdis == 2)))
		{
			/* Set up ranged melee attacks */
			init_ranged_attack(r_ptr);

			choice = choose_ranged_attack(m_idx, &ty, &tx, (roll < chance_innate ? 0x01 : 0x00) | (roll < chance_spell ? 0x02: 0x00));
		}

		/* Selected a ranged attack? */
		if (choice != 0)
		{
			/* The monster is hidden */
			if (m_ptr->mflag & (MFLAG_HIDE))
			{
				/* Reveal the monster */
				m_ptr->mflag &= ~(MFLAG_HIDE);

				/* And update */
				update_mon(cave_m_idx[m_ptr->fy][m_ptr->fx], FALSE);

				/* Hack --- tell the player if something unhides */
				if (m_ptr->ml)
				{
					char m_name[80];

					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					msg_format("%^s emerges from the %s.", m_name, f_name + f_info[cave_feat[m_ptr->fy][m_ptr->fx]].name);
				}

				/* Disturb on "move" */
				if ((m_ptr->ml && (disturb_move ||
					((m_ptr->mflag & (MFLAG_VIEW)) && disturb_near)))
						&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
				{
					/* Disturb */
					disturb(0, 0);
				}

				/* Set attack */
				if (choice < 96 + 8) m_ptr->mflag |= (MFLAG_SHOT);
				else if (choice < 128) m_ptr->mflag |= (MFLAG_BREATH);
				else m_ptr->mflag |= (MFLAG_CAST);

				/* Hack -- unhiding monsters end turn */
				return;
			}

			/* Execute said attack */
			make_attack_ranged(m_idx, choice, ty, tx);

			/* End turn */
			return;
		}
	}

	/*** Monster hungry? ***/
	/*** This is a bit unnecessarily complicated */

	/* We're just hungry */
	if ((((m_ptr->mflag & (MFLAG_WEAK | MFLAG_STUPID | MFLAG_NAIVE | MFLAG_CLUMSY | MFLAG_SICK)) != 0)

	/* or want to recover from something non-critical */
		|| (m_ptr->hp < m_ptr->maxhp) || (m_ptr->blind)) &&

	/* and we don't have a target */
		!(m_ptr->ty) && !(m_ptr->tx)

	/* aren't aggressive, and the player can't see us */
		&& (((m_ptr->mflag & (MFLAG_AGGR)) == 0) || !(player_has_los_bold(m_ptr->fy, m_ptr->fx)) ||

	/* or if we have a critical requirement */
			(m_ptr->hp < m_ptr->maxhp / 10) || ((m_ptr->blind) && (r_ptr->freq_spell >= 25))  || (m_ptr->mana < r_ptr->mana / 5))

	/* and are alive or eat bodies */
		&& (!(r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags2 & (RF2_EAT_BODY))))
	{
		int this_o_idx, next_o_idx, item = 0;

		object_type *o_ptr;

		object_kind *k_ptr;

		bool healing = FALSE;
		bool mana_recovery = FALSE;

		/* Scan the pile of objects */
		for (this_o_idx = cave_o_idx[m_ptr->fy][m_ptr->fx]; this_o_idx; this_o_idx = next_o_idx)
		{
			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Only undead/insect cannibals */
			if ((o_ptr->name3) && (r_info[o_ptr->name3].d_char == r_ptr->d_char) && ((r_ptr->flags3 & (RF3_UNDEAD | RF3_INSECT)) == 0)) continue;

			/* Check kind */
			k_ptr = &k_info[o_ptr->k_idx];

			/* Edible - mana recovery */
			if ((k_ptr->flags6 & (TR6_EAT_MANA)) && (m_ptr->mana < r_ptr->mana / 5))
			{
				item = this_o_idx;
				mana_recovery = TRUE;
				break;
			}

			/* We've found a healing item already - still looking for mana recovery */
			else if (healing) continue;

			/* Edible - healing */
			else if ((k_ptr->flags6 & (TR6_EAT_HEAL)) && ((m_ptr->hp < m_ptr->maxhp / 2) || (m_ptr->blind)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0))
			{
				item = this_o_idx;
				healing = TRUE;
			}

			/* General food requirements */
			else if ((k_ptr->flags6 & (TR6_EAT_BODY)) && (r_ptr->flags2 & (RF2_EAT_BODY))) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_INSECT)) && (r_ptr->flags3 & (RF3_INSECT))) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_ANIMAL)) && (r_ptr->flags3 & (RF3_ANIMAL)) && ((r_ptr->flags2 & (RF2_EAT_BODY)) == 0)) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_SMART)) && (r_ptr->flags2 & (RF2_SMART)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0)) item = this_o_idx;
		}

		/* Scan the carried objects - unless we've found mana recovery */
		if (!mana_recovery) for (this_o_idx = cave_o_idx[m_ptr->fy][m_ptr->fx]; this_o_idx; this_o_idx = next_o_idx)
		{
			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Only undead/insect cannibals */
			if ((o_ptr->name3) && (r_info[o_ptr->name3].d_char == r_ptr->d_char) && ((r_ptr->flags3 & (RF3_UNDEAD | RF3_INSECT)) == 0)) continue;

			/* Check kind */
			k_ptr = &k_info[o_ptr->k_idx];

			/* Edible - mana recovery */
			if ((k_ptr->flags6 & (TR6_EAT_MANA)) && (m_ptr->mana < r_ptr->mana / 5))
			{
				item = this_o_idx;
				mana_recovery = TRUE;
				break;
			}

			/* We've found a healing item already - still looking for mana recovery */
			else if (healing) continue;

			/* Edible - healing */
			else if ((k_ptr->flags6 & (TR6_EAT_HEAL)) && ((m_ptr->hp < m_ptr->maxhp / 2) || (m_ptr->blind)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0))
			{
				item = this_o_idx;
				healing = TRUE;
			}

			/* General food requirements */
			else if ((k_ptr->flags6 & (TR6_EAT_BODY)) && (r_ptr->flags2 & (RF2_EAT_BODY))) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_INSECT)) && (r_ptr->flags3 & (RF3_INSECT))) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_ANIMAL)) && (r_ptr->flags3 & (RF3_ANIMAL)) && ((r_ptr->flags2 & (RF2_EAT_BODY)) == 0)) item = this_o_idx;
			else if ((k_ptr->flags6 & (TR6_EAT_SMART)) && (r_ptr->flags2 & (RF2_SMART)) && ((r_ptr->flags3 & (RF3_UNDEAD)) == 0)) item = this_o_idx;
		}

		/* Something edible found? */
		if (item)
		{
			int part;

			int amount = r_ptr->level + 5;

			object_type object_type_body;

			/* Get temporary object */
			o_ptr = &object_type_body;

			/* Get a copy of the object */
			object_copy(o_ptr, &o_list[item]);

			/* Reduce item */
			floor_item_increase(item, -1);
			floor_item_optimize(item);

			/* Set object number */
			o_ptr->number = 1;

			/* Describe observable situations */
			if (player_has_los_bold(m_ptr->fy, m_ptr->fx))
			{
				char m_name[80];
				char o_name[80];

				/* Describe monster */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Describe */
				object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 0);

				/* Dump a message */
				if ((o_ptr->tval == TV_BONE) || ((o_ptr->tval == TV_BODY) && (o_ptr->weight > r_ptr->level))) msg_format("%^s chews on %s.", m_name, o_name);

				/* Dump a message */
				else msg_format("%^s swallows %s.", m_name, o_name);

				/* Apply good potion / mushroom effect */
				/* We assume anything that will eat bad mushrooms is immune to their effects */
				if (((o_ptr->tval == TV_FOOD) || (o_ptr->tval == TV_MUSHROOM) ||
						(o_ptr->tval == TV_POTION)) && (k_info[o_ptr->k_idx].cost))
				{
					/* Hack -- use item blow routine */
					process_item_blow(SOURCE_OBJECT, o_ptr->k_idx, o_ptr, m_ptr->fy, m_ptr->fx, TRUE, TRUE);

					/* Mega Hack -- heal blindness directly */
					if ((k_info[o_ptr->k_idx].flags6 & (TR6_EAT_HEAL)) && (m_ptr->blind)) m_ptr->blind = 1;
				}
			}

			/* Break up skeletons */
			if ((o_ptr->tval == TV_BONE) && (o_ptr->sval == SV_BONE_SKELETON))
			{
				part = rand_int(5) + 3;
				o_ptr->sval = SV_BONE_BONE;
				o_ptr->weight /= part + 1;

				for (i = 0; i < part; i++)
				{
					floor_carry(m_ptr->fy + rand_int(3) - 2, m_ptr->fx + rand_int(3) - 2, o_ptr);
				}
			}

			/* Partially eat bodies */
			else if ((o_ptr->tval == TV_BODY) && (o_ptr->weight > amount))
			{
				part = (o_ptr->weight - amount) * 100 / o_ptr->weight;
				o_ptr->weight -= amount;

				switch (o_ptr->sval)
				{
					case SV_BODY_CORPSE:
					{
						if (part < 90) o_ptr->sval = SV_BODY_HEADLESS;
					}
					case SV_BODY_HEADLESS:
					{
						if (part < 80) o_ptr->sval = SV_BODY_BUTCHERED;
					}
					case SV_BODY_BUTCHERED:
					{
						if ((part < 70) && ((rand_int(100) < 50) || !(make_part(o_ptr, o_ptr->name3)))) o_ptr->sval = SV_BODY_MEAT;
					}
				}

				if ((part < 30) && (r_info[o_ptr->name3].flags8 & (RF8_HAS_SKELETON)))
				{
					o_ptr->tval = TV_BONE;
					if (part > 10) o_ptr->sval = SV_BONE_SKELETON;
					else if (part > 5) o_ptr->sval = SV_BONE_BONE;
					else if (r_info[o_ptr->name3].flags8 & (RF8_HAS_TEETH)) o_ptr->sval = SV_BONE_TEETH;
					else (void)make_skin(o_ptr, o_ptr->name3);
				}

				floor_carry(m_ptr->fy, m_ptr->fx, o_ptr);
			}

			/* Monster benefits */
			feed_monster(m_idx);

			return;
		}
	}


	/*** Movement ***/

	/* Assume no movement */
	ty = 0;
	tx = 0;

	/*
	 * Innate semi-random movement.  Monsters adjacent to the character
	 * can always strike accurately at him (monster isn't confused) or
	 * move out of his way.
	 */
	if ((r_ptr->flags1 & (RF1_RAND_50 | RF1_RAND_25)) && (m_ptr->cdis > 1) && ((level_flag & (LF1_BATTLE)) == 0))
	{
		int chance = 0;

		int i;

		/* RAND_25 and RAND_50 are cumulative */
		if (r_ptr->flags1 & (RF1_RAND_25))
		{
			chance += 25;
			if (m_ptr->ml) l_ptr->flags1 |= (RF1_RAND_25);
		}
		if (r_ptr->flags1 & (RF1_RAND_50))
		{
			chance += 50;
			if (m_ptr->ml) l_ptr->flags1 |= (RF1_RAND_50);
		}

		/* Look for adjacent enemies */
		for (i = 0; i < 8; i++)
		{
			int y1 = m_ptr->fy + ddy_ddd[i];
			int x1 = m_ptr->fx + ddx_ddd[i];

			if (!in_bounds_fully(y1, x1)) continue;

			if (cave_m_idx[y1][x1] <= 0) continue;

			/* Adjacent monster we want to fight */
			if (choose_to_attack_monster(m_ptr, &m_list[cave_m_idx[y1][x1]])) chance = 0;
		}

		/* Chance of moving randomly */
		if (rand_int(100) < chance) random = TRUE;
	}


	/* Monster isnt' moving randomly, isnt' running away
	 * doesn't hear or smell the character
	 */
	if ((!aware) && (!random))
	{
		/* Monster has a known target */
		if ((m_ptr->ty) && (m_ptr->tx)) must_use_target = TRUE;

		/* Monster has to move randomly */
		else random = TRUE;
	}


	/* Invisible monsters 'dance' around the player in order to make their
	 * location less predictable. If the player can see invisible, don't do
	 * this. */
	if (((r_ptr->flags2 & (RF2_INVISIBLE)) || (m_ptr->tim_invis)) &&
			((m_ptr->smart & (SM_SEE_INVIS)) == 0))
	{
		/* At close range, check for available space */
		if ((m_ptr->cdis == 1)  &&
				(((r_ptr->flags2 & (RF2_COLD_BLOOD)) == 0) ||
				 !(p_ptr->see_infra)))
		{
			/*
			 * If character vulnerability has not yet been
			 * calculated this turn, calculate it now.
			 */
			if (p_ptr->vulnerability == 0)
			{
				/* Count passable grids next to player */
				for (i = 0; i < 8; i++)
				{
					y = p_ptr->py + ddy_ddd[i];
					x = p_ptr->px + ddx_ddd[i];

					/* Check Bounds */
					if (!in_bounds_fully(y, x)) continue;

					/* Count floor grids (generic passable) */
					if (cave_floor_bold(y, x))
					{
						p_ptr->vulnerability++;
					}
				}

				/*
				 * Take character weakness into account (this
				 * always adds at least one)
				 *
				 * We don't have to do this here, however, for
				 * animals, this calculation is important, so ensure
				 * computation.
				 */
				if (p_ptr->chp <= 10) p_ptr->vulnerability = 100;
				else p_ptr->vulnerability += (p_ptr->mhp / p_ptr->chp);
			}

			/* Character is sufficiently vulnerable */
			if (p_ptr->vulnerability > 4)
			{
				/* Move randomly some of the time */
				random = (rand_int(100) < 30);
			}
		}
	}

	/* Monster is using the special "townsman" AI in town */
	if (((m_ptr->mflag & (MFLAG_TOWN)) != 0) && ((m_ptr->mflag & (MFLAG_ALLY)) == 0)
			&& (room_near_has_flag(m_ptr->fy, m_ptr->fx, ROOM_TOWN)))
	{
		/* Town monster been attacked */
		if (m_ptr->mflag & (MFLAG_AGGR))
		{
			int i, dam = 0;

			/* Check blows for any ability to hurt the player */
			for (i = 0; i < 4; i++)
			{
				if (!r_ptr->blow[i].method) continue;
				if (!r_ptr->blow[i].d_dice) continue;

				dam = 1;
			}

			/* Can't hurt the player - flee */
			if (!dam) m_ptr->min_range = FLEE_RANGE;

			/* Advance on the character */
		}

		/* Always have somewhere to go */
		else if ((!m_ptr->ty) || (!m_ptr->tx) ||
		    (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags1 & (FF1_ENTER)))
		{
			/* Get a new target */
			get_town_target(m_ptr);

			/* Not interested in the character */
			must_use_target = TRUE;
		}
		else if ((m_ptr->ty == m_ptr->fy) &&
				 (m_ptr->tx == m_ptr->fx))
		{
			/* go to a new target */
			get_town_target(m_ptr);

			/* Not interested in the character */
			must_use_target = TRUE;
		}
		/* Has target, not interested in character */
		else if ((m_ptr->ty) && (m_ptr->tx))
		{
			/* Not interested in the character */
			must_use_target = TRUE;
		}
	}

	/* Is monster an ally, or fighting an ally of the player? */
	if ((m_ptr->mflag & (MFLAG_IGNORE | MFLAG_ALLY)) != 0)
	{
		/* Attempt at efficiency */
		bool ally = ((m_ptr->mflag & (MFLAG_ALLY)) != 0);
		bool aggressive = ((m_ptr->mflag & (MFLAG_AGGR)) != 0) ;
		bool need_lite = ((r_ptr->flags2 & (RF2_NEED_LITE)) == 0);
		bool sneaking = (p_ptr->sneaking) || (m_ptr->cdis > MAX_SIGHT);

		/* Note: We have to prevent never move monsters from acquiring non-adjacent targets or they will move */
		bool can_target = (aggressive || !(m_ptr->monfear));

		/* This allows smart monsters and the player's familiar to report enemy positions to the player */
		bool spying = ally && (((r_ptr->flags2 & (RF2_SMART)) != 0) || (m_ptr->r_idx == FAMILIAR_IDX));

		/* This prevents player monsters from being too effective whilst the player is not around */
		bool restrict_targets = ally && !aggressive;

		/* This encourages archers and magic users to stay at a distance in combat */
		bool closing = !ally || ((m_ptr->ty) != 0);

		/* k is used to record the distance of the closest enemy */
		/* Note we scale this up, and use a pseudo-random hack to try to get multiple monsters
		 * to favour different equi-distant enemies */
		int k = (ally ? MAX_SIGHT : m_ptr->cdis) * 16 + 15;

		/* And sometimes we artificially manipulate k to prefer enemies at a distance.
		 * We need the real distance later on, so have to fix it.*/
		int fix_k = 0;

		/* Note the player can set target_near in the targetting routine to force allies to consider
		 * targets closest to another monster or a point, as opposed to themselves. */
		int ny = (ally && ((p_ptr->target_set & (TARGET_NEAR)) != 0)) ? (p_ptr->target_who ? m_list[p_ptr->target_who].fy : p_ptr->target_row) : m_ptr->fy;
		int nx = (ally && ((p_ptr->target_set & (TARGET_NEAR)) != 0)) ? (p_ptr->target_who ? m_list[p_ptr->target_who].fx : p_ptr->target_col) : m_ptr->fx;

		/* Note the player can set target_race in the targetting routine to force allies to consider
		 * targets only of a particular race if they can see at least one of them. */
		bool force_one_race = FALSE;

		/* Need to understand spies, and be able to hear them.
		 * Smart, nonvocal monsters are telepathic. */
		if (spying && ((r_ptr->flags3 & (RF3_NONVOCAL)) == 0))
		{
			/* Too far away to hear */
			if ((m_ptr->cdis > MAX_RANGE / 3) && !player_can_fire_bold(m_ptr->fy, m_ptr->fx)) spying = FALSE;

			/* Cannot understand language */
			if (spying && (m_ptr->r_idx != FAMILIAR_IDX) && !player_understands(monster_language(m_ptr->r_idx))) spying = FALSE;
		}

		/* Spies report their own position */
		if (spying && (!(m_ptr->ml) || (p_ptr->timed[TMD_BLIND])))
		{
			/* Optimize -- Repair flags */
			repair_mflag_mark = repair_mflag_show = TRUE;

			/* Hack -- Detect the monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Update the monster */
			update_mon(m_idx, FALSE);
		}

		/* Check all the other monsters */
		for (i = m_max - 1; i >= 1; i--)
		{
			bool see_target = FALSE;
			bool los_target = FALSE;

			/* We may manipulate the distance to get monsters to prefer differing
			 * types of enemies. Currently this is used to get fast monsters to favour
			 * isolated enemies so they can hack-and-back them more effectively, using
			 * code similar to the player vulnerability code.
			 * TODO: This could be used to implement racial hatred. */
			int d, hack_d = 0;

			/* Access the monster */
			monster_type *n_ptr = &m_list[i];

			/* Skip itself */
			if (m_idx == i) continue;

			/* Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* We calculate the distance with a scale factor to get monsters to prefer
			 * difference equidistant enemies */
			d = (distance(n_ptr->fy, n_ptr->fx, ny, nx) * 16) + ((m_idx + i) % 16);

			/* Ignore targets out of range */
			if (d > MAX_RANGE * 16) continue;

			/* Check if the monster smells the target. */
			if (r_ptr->flags9 & (RF9_SCENT | RF9_WATER_SCENT))
			{
				feature_type *f_ptr = &f_info[cave_feat[n_ptr->fy][n_ptr->fx]];
				
				/* We should compute the actual distance in steps to the target but we
				 * don't. We just instead use distance as a proxy of scent strength */
				if (((d < MAX_RANGE * 16 / 2) || ((r_ptr->flags9 & (RF9_SUPER_SCENT)) != 0)) &&

					/* Pass wall and flyers don't leave scents */
					((r_info[n_ptr->r_idx].flags2 & (RF2_CAN_FLY | RF2_MUST_FLY | RF2_PASS_WALL)) == 0) &&

					/* Climbers only leave scents for other climbers */
					(((r_ptr->flags2 & (RF2_CAN_CLIMB)) != 0) ||
						((r_info[n_ptr->r_idx].flags2 & (RF2_CAN_CLIMB)) == 0)))
				{
					/* Test for water and not mud */
					if ((f_ptr->flags2 & (FF2_WATER | FF2_CAN_SWIM)) == (FF2_WATER | FF2_CAN_SWIM))
					{
						see_target = (r_ptr->flags9 & (RF9_WATER_SCENT)) != 0;
					}
					else if ((f_ptr->flags2 & (FF2_WATER | FF2_LAVA | FF2_ICE | FF2_ACID | FF2_OIL | FF2_CHASM)) == 0)
					{
						see_target = (r_ptr->flags9 & (RF9_SCENT)) != 0;
					}
				}
				
				/* Check whether we have los as well. This allows us to report this to the player. */
				if (see_target && spying && (!(n_ptr->ml) || (p_ptr->timed[TMD_BLIND])))
				{
					/* Needs line of sight. */
					los_target = generic_los(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx, CAVE_XLOS);
				}
			}
			
			/*
			 * Check if monster can see the target. We make various assumptions about what
			 * monsters have a yet to be implemented TODO: SEE_INVIS flag, if the target is
			 * invisible - we currently use the test either an eye monster or monster can
			 * resist blindness to determine whether it can see invisible. We check if the
			 * target is in light, if the monster needs light to see by.
			 */
			if (!see_target)
			{
				/* Monster is hidden */
				if (n_ptr->mflag & (MFLAG_HIDE))
				{
					if ((r_ptr->flags3 & (RF3_NONVOCAL)) == 0) continue;
				}

				/* Monster is invisible */
				if ((r_info[n_ptr->r_idx].flags2 & (RF2_INVISIBLE)) || (n_ptr->tim_invis))
				{
					/* Cannot see invisible, or use infravision to detect the monster */
					if ((r_ptr->d_char != 'e') && ((r_ptr->flags9 & (RF9_RES_BLIND)) == 0)
							&& ((r_ptr->aaf < d) || ((r_info[n_ptr->r_idx].flags2 & (RF2_COLD_BLOOD)) != 0))) continue;
				}

				/* Monster needs light to see */
				if (need_lite)
				{
					/* Check for light */
					if (((cave_info[n_ptr->fy][n_ptr->fx] & (CAVE_LITE)) == 0)
						&& ((play_info[n_ptr->fy][n_ptr->fx] & (PLAY_LITE)) == 0))
					{
						continue;
					}
				}

				/* Needs line of sight. */
				los_target = generic_los(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx, CAVE_XLOS);
				
				/* Sees the target? */
				see_target = los_target;
			}

			/* Can't see or smell the target */
			if (!see_target) continue;

			/* Allies report to player if spying on the enemy */
			/* We need to ensure that the monster can tell the player what they are
			 * seeing, either by using speech or telepathically from anywhere on
			 * the map. */
			if (spying && (!(n_ptr->ml) || (p_ptr->timed[TMD_BLIND])) && (d /16 <= r_ptr->aaf) && ((los_target) ||

				/* Hack -- we give occasional 'flashes' of scents */
				(turn % 40 == m_idx % 40)))
			{
				/* Optimize -- Repair flags */
				repair_mflag_mark = repair_mflag_show = TRUE;

				/* Hack -- Detect the monster */
				n_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);
				
				/* Hack -- Smell just gives limited information */
				/* if (!los_target) n_ptr->mflag |= (MFLAG_DLIM); */

				/* Update the monster */
				update_mon(i, FALSE);
			}

			/* Allies only choose targets visible to player, unless aggressive. If sneaking, we ignore detected monsters */
			if (restrict_targets && !(n_ptr->ml) && (!(sneaking) ||  ((n_ptr->mflag & (MFLAG_MARK | MFLAG_SHOW)) == 0))) continue;

			/* Monster is not an enemy */
			if (!choose_to_attack_monster(m_ptr, n_ptr)) continue;

			/* Ignore certain targets if sneaking */
			if (sneaking)
			{
				/* Target is asleep - ignore */
				if (n_ptr->csleep) continue;

				/* Target not aggressive */
				if ((n_ptr->mflag & (MFLAG_AGGR)) == 0)
				{
					monster_race *s_ptr = &r_info[n_ptr->r_idx];

					/* I'm invisible and target can't see me - ignore */
					if ((r_ptr->flags2 & (RF2_INVISIBLE)) || (m_ptr->tim_invis))
					{
						/* Cannot see invisible, or use infravision to detect the monster */
						if ((s_ptr->d_char != 'e') && ((s_ptr->flags2 & (RF2_INVISIBLE)) == 0)
							&& ((s_ptr->aaf < d) || ((r_ptr->flags2 & (RF2_COLD_BLOOD)) != 0))) continue;
					}

					/* I'm in darkness and target can't see in darkness - ignore */
					if (s_ptr->flags2 & (RF2_NEED_LITE))
					{
						/* Check for light */
						if (((cave_info[n_ptr->fy][n_ptr->fx] & (CAVE_LITE)) == 0)
								&& ((play_info[n_ptr->fy][n_ptr->fx] & (PLAY_LITE)) == 0))
						{
							continue;
						}
					}
				}
			}

			/* Never move can only target adjacent monsters. Note we must use real distance here */
			if ((((r_ptr->flags1 & (RF1_NEVER_MOVE)) != 0) || (m_ptr->petrify)) && (distance(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx) > 1)) continue;
			
			/* Can't attack the target */
			if (!(can_target) || ((r_ptr->flags1 & (RF1_NEVER_BLOW)) != 0))
			{
				/* Target set already */
				if (m_ptr->ty) continue;

				/* Run back to the player with tails between its legs,
				 * unless ordered somewhere. */
				if (((r_ptr->flags1 & (RF1_NEVER_MOVE)) == 0) && !(m_ptr->petrify) && ((!p_ptr->target_set) || (p_ptr->target_who)))
				{
					m_ptr->ty = p_ptr->py;
					m_ptr->tx = p_ptr->px;

					must_use_target = TRUE;
				}

				continue;
			}

			/* Check if forcing a particular kind */
			if (ally && p_ptr->target_race)
			{
				/* No match */
				if (p_ptr->target_race != n_ptr->r_idx)
				{
					/* Have already found a match */
					if (force_one_race) continue;
				}
				else if (!force_one_race)
				{
					force_one_race = TRUE;
					k = MAX_RANGE * 16;
					i = m_max;
				}
			}

			/* High speed differential. Compute vulnerability. */
			if (m_ptr->mspeed > n_ptr->mspeed + 5)
			{
				/* We do the monster equivalent of computing vulnerability.
				 * This allows fast monsters to try to surround isolated
				 * individuals. Otherwise, they tend to get chewed up in
				 * combat.
				 */
				int ii;
				int v = 0;

				for (ii = 0; ii < 8; ii++)
				{
					int yy = n_ptr->fy + ddy[ii];
					int xx = n_ptr->fx + ddx[ii];

					if (!in_bounds_fully(yy, xx)) continue;

					if (place_monster_here(yy,xx,m_ptr->r_idx) <= MM_FAIL) continue;

					/* Decrease range if the target has unaligned space around it */
					if ((cave_m_idx[yy][xx]) ||
						(ally != ((m_list[cave_m_idx[yy][xx]].mflag & (MFLAG_ALLY)) != 0)))
					{
						v++;
						if (v > 4) hack_d -= 32;

						/* Note we have to fix the distance later on by this adjustment factor */
					}
				}
			}

			/* Prefer not to attack fleeing targets */
			if (n_ptr->monfear)
			{
				hack_d += 16 * n_ptr->monfear;
			}

			/* Pick a random target. Have checked for LOS already. */
			if (d < k)
			{
				m_ptr->ty = n_ptr->fy;
				m_ptr->tx = n_ptr->fx;
				k = d + hack_d;

				/* Fix adjustment factor later */
				fix_k = hack_d;
				hack_d = 0;

				must_use_target = TRUE;
			}
		}

		/* Fix the distance */
		k = k - fix_k;

		/* Allies use 'player' targets if the monster unable to find a target
		 * TODO: We hackily use best range to interact with
		 * monster targetting code for allies. The best range for an
		 * ally is given as its distance from the player when a
		 * target is set, and increases by one per monster turn.
		 * If the player is moving away from the monster, the distance
		 * will exceed the best range and the target will be aborted.
		 */
		if (((m_ptr->mflag & (MFLAG_ALLY)) != 0) && ((m_ptr->mflag & (MFLAG_IGNORE)) == 0) && (!must_use_target) &&
				/* Hack - never move monsters never get a target */
				((r_ptr->flags1 & (RF1_NEVER_MOVE)) == 0) && !(m_ptr->petrify))
		{
			/* Player has target set. */
			if (p_ptr->target_set)
			{
				/* Player is targetting a monster and ally is allowed to attack. */
				if ((p_ptr->target_who > 0) && ((r_ptr->flags1 & (RF1_NEVER_BLOW)) == 0))
				{
					monster_type *n_ptr = &m_list[p_ptr->target_who];

					/* Target it if visible. Note can use more information than 'easily' visible. */
					if ((n_ptr->ml) || ((n_ptr->mflag & (MFLAG_SHOW | MFLAG_MARK)) != 0))
					{
						m_ptr->ty = n_ptr->fy;
						m_ptr->tx = n_ptr->fx;
					}
				}

				/* Player has targetted a grid. Hold at that position. */
				else
				{
					m_ptr->ty = p_ptr->target_row;
					m_ptr->tx = p_ptr->target_col;

					/* But not closing anymore */
					closing = FALSE;
				}
			}

			/* Get items for the player */
			else if (r_ptr->flags2 & (RF2_TAKE_ITEM))
			{
				int value, best_value = 0;

				for (i = o_max - 1; i >= 1; i--)
				{
					u32b f1, f2, f3, f4;
					u32b flg3 = 0L;
					int d;

					/* Access the object */
					object_type *o_ptr = &o_list[i];

					/* Skip dead objects */
					if (!o_ptr->k_idx) continue;

					/* Skip objects held by monsters */
					if (o_ptr->held_m_idx) continue;

					/* Allies only find objects visible to player */
					if ((o_ptr->ident & (IDENT_MARKED)) == 0) continue;

					/* Skip artifacts */
					if (artifact_p(o_ptr)) continue;

					/* Skip objects in ungettable locations */
					if ((f_info[cave_feat[o_ptr->iy][o_ptr->ix]].flags1 & (FF1_DROP)) == 0) continue;

					/* Paranoia - skip objects we are on top of */
					if ((o_ptr->iy == m_ptr->fy) && (o_ptr->ix == m_ptr->fx)) continue;

					/* Get distance */
					d = distance(o_ptr->iy, o_ptr->ix, p_ptr->py, p_ptr->px);

					/* Skip distant objects */
					if (d > MAX_RANGE / 2) continue;

					/* Extract some flags */
					object_flags(o_ptr, &f1, &f2, &f3, &f4);

					/* React to objects that hurt the monster */
					if (f1 & (TR1_SLAY_DRAGON)) flg3 |= (RF3_DRAGON);
					if (f1 & (TR1_SLAY_TROLL)) flg3 |= (RF3_TROLL);
					if (f1 & (TR1_SLAY_GIANT)) flg3 |= (RF3_GIANT);
					if (f1 & (TR1_SLAY_ORC)) flg3 |= (RF3_ORC);
					if (f1 & (TR1_SLAY_DEMON)) flg3 |= (RF3_DEMON);
					if (f1 & (TR1_SLAY_UNDEAD)) flg3 |= (RF3_UNDEAD);
					if (f1 & (TR1_SLAY_NATURAL)) flg3 |= (RF3_ANIMAL | RF3_PLANT | RF3_INSECT);
					if (f1 & (TR1_BRAND_HOLY)) flg3 |= (RF3_EVIL);

					/* The object cannot be picked up by the monster */
					if (r_ptr->flags3 & flg3) continue;

					/* Use value as a proxy for the order to get these */
					value = object_value(o_ptr) * o_ptr->number;

					/* Not worth as much as required. Scale this by depth */
					if (value <= best_value + d * p_ptr->depth * p_ptr->depth) continue;

					/* Remember best value */
					best_value = value;

					/* Get it */
					m_ptr->ty = o_ptr->iy;
					m_ptr->tx = o_ptr->ix;
				}
			}

			/* Follow the player.*/
			else if (m_ptr->min_range > m_ptr->cdis)
			{
				m_ptr->ty = p_ptr->py;
				m_ptr->tx = p_ptr->px;
			}

			/* If holding position, return to position */
			if (m_ptr->ty || m_ptr->tx)
			{
				must_use_target = TRUE;
			}
		}

		/* If the monster is closing, find the range */
		if (closing && must_use_target)
		{
			/* Important -- clear existing ranges */
			m_ptr->min_range = 1;
			m_ptr->best_range = 1;

			/* Refind range */
			find_range(m_idx);

			/* Forget the target if it is too close and we can shoot it */
			if ((m_ptr->best_range > 1) && (k < ((int)m_ptr->best_range * 16)) && generic_los(m_ptr->fy, m_ptr->fx, m_ptr->ty, m_ptr->tx, CAVE_XLOF))
			{
				/* Player has specified a position to hold */
				if ((p_ptr->target_set) && !(p_ptr->target_who) && ((p_ptr->target_set & (TARGET_NEAR)) == 0))
				{
					m_ptr->ty = p_ptr->target_row;
					m_ptr->tx = p_ptr->target_col;
				}
				/* Go to player position */
				else
				{
					must_use_target = FALSE;
					m_ptr->ty = 0;
					m_ptr->tx = 0;
				}

				/* Find range again */
				find_range(m_idx);

				/* Hack -- speed up combat */
				m_ptr->mflag |= (MFLAG_CAST | MFLAG_SHOT | MFLAG_BREATH);
			}
		}
	}


	/*** Find a target to move to ***/

	/* Monster is genuinely confused */
	if ((m_ptr->confused) && (m_ptr->confused > rand_int(100)))
	{
		/* Choose any direction except five and zero */
		dir = rand_int(8);

		/* Monster can try to wander into /anything/... */
		ty = m_ptr->fy + ddy_ddd[dir];
		tx = m_ptr->fx + ddx_ddd[dir];
	}

	/* Monster isn't confused, just moving semi-randomly, or monster is partially confused */
	else if ((random) || ((m_ptr->confused) && (m_ptr->confused > rand_int(50))))
	{
		int start = rand_int(8);
		bool dummy;

		/* Is the monster scared? */
		if ((!(r_ptr->flags1 & (RF1_NEVER_MOVE))) &&
				!(m_ptr->petrify) &&
		    ((m_ptr->min_range == FLEE_RANGE) ||
		     (m_ptr->monfear)))
		{
			fear = TRUE;
		}

		/* Look at adjacent grids, starting at random. */
		for (i = start; i < 8 + start; i++)
		{
			y = m_ptr->fy + ddy_ddd[i % 8];
			x = m_ptr->fx + ddx_ddd[i % 8];

			/* Accept first passable grid. */
			if (cave_passable_mon(m_ptr, y, x, &dummy) != 0)
			{
				ty = y;
				tx = x;
				break;
			}
		}

		/* No passable grids found */
		if ((ty == 0) && (tx == 0)) return;
	}

	/* Normal movement */
	else
	{
		/* Choose a pair of target grids, or cancel the move. */
		if (!get_move(m_idx, &ty, &tx, &fear, must_use_target))
			return;
	}

	/* Calculate the actual move.  Cancel move on failure to enter grid. */
	if (!make_move(m_idx, &ty, &tx, fear, &bash)) return;

	/* Change terrain, move the monster, handle secondary effects. */
	process_move(m_idx, ty, tx, bash);

	/* End turn */
	return;
}


/* The ally needs to be paid again */
#define MAX_COMMENT_1a   6

static cptr comment_1a[MAX_COMMENT_1a] =
{
	"I need more gold to stay in your service, my lord.",
	"The coffers of my home have grown empty, master.",
	"I need to be paid again, and soon.",
	"How will I pay for my upkeep without your assistance?",
	"I need a sign of favour from you to continue.",
	"I need a token of your esteem."
};


/* The townsfolk grow restless */
#define MAX_COMMENT_1b   6

static cptr comment_1b[MAX_COMMENT_1b] =
{
	"Is that the extent of your largess? You'd better have more for me, and soon...",
	"The dark lords are far more generous than you. Should we turn to them?",
	"Is the meagre gifts you've given all you have? You disgust me...",
	"What more have you got for me? Hurry up or else...",
	"I'm running out of patience with you. You'll regret not giving me more...",
	"Miser! I have children to feed and you give me nothing..."
};


/* Ally mercenary leaving */
#define MAX_COMMENT_2a   6

static cptr comment_2a[MAX_COMMENT_2a] =
{
	"No more gold? I quit your service...",
	"I've had enough. I'm off...",
	"I've gone too long without pay. I resign this comission.",
	"I've run out of money and can't afford to stay with you.",
	"Your lack of faith in me forces me to leave you.",
	"I would have stayed, if only you acknowledged my devotion."
};


/* Townsfolk turning on you */
#define MAX_COMMENT_2b   6

static cptr comment_2b[MAX_COMMENT_2a] =
{
	"Traitor! Have it at you!",
	"Blackheart! Your villiany has been exposed...",
	"Fiend! You betrayed us in our hour of need...",
	"Liar! You've broken your word one time too many...",
	"Thief! Are all who call themselves a @ as conniving as you?",
	"Scoundrel! Your # code is just threadbare rags..."
};


/*
 * Monster regeneration of HPs and mana, and recovery from all temporary
 * conditions.
 *
 * This function is called a lot, and is therefore fairly expensive.
 */
static void recover_monster(int m_idx, bool regen)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	/* Get the origin */
	int y = m_ptr->fy;
	int x = m_ptr->fx;

	/* Monster name */
	char m_name[80];
	char m_poss[80];


	/* Hack -- Bug tracking */
	if (m_ptr->hp > m_ptr->maxhp)
	{
		msg_print("BUG: Monster hit points too high! Please report.");

		m_ptr->hp = m_ptr->maxhp;
	}

	/* Handle "summoned" - every 100 turns */
	if ((m_ptr->summoned) && (regen))
	{
		m_ptr->summoned--;

		/* Warn the player we're running out of patience */
		if ((m_ptr->summoned == MIN_TOWN_WARNING) && ((m_ptr->mflag & (MFLAG_TOWN)) != 0))
		{
			/* Note loss of patience. */
			monster_speech(m_idx, (m_ptr->mflag & (MFLAG_ALLY))? comment_1a[rand_int(MAX_COMMENT_1a)] : comment_1b[rand_int(MAX_COMMENT_1b)], FALSE);
		}

		/* No longer summoned */
		if (!m_ptr->summoned)
		{
			/* Town monsters change their tune */
			if ((m_ptr->mflag & (MFLAG_TOWN)) != 0)
			{
				/* Evil just silently betrays the player */
				if (r_ptr->flags3 & (RF3_EVIL))
				{
					/* Able to stab the player in the back, shoot him or ensorcle him? */
					if ((m_ptr->cdis <= 1) || (r_ptr->freq_innate > 35) ||
							((r_ptr->freq_spell > 25) && (m_ptr->mana > r_ptr->mana / 3)))
					{
						u32b flag_hack = m_ptr->mflag;

						/* Consider switching sides? */
						m_ptr->mflag &= ~(MFLAG_ALLY | MFLAG_TOWN);

						/* And wait for assistance most of the time */
						if ((!rand_int(10)) || (!check_allies_exist(m_ptr->fy, m_ptr->fx))) m_ptr->mflag = flag_hack;

						/* Try to summon assistance quietly */
						else (tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), NULL));
					}
					/* We're alone */
					else if (!check_allies_exist(m_ptr->fy, m_ptr->fx))
					{
						m_ptr->best_range = 1;
					}

					/* Try again shortly */
					if (m_ptr->mflag & (MFLAG_ALLY | MFLAG_TOWN)) m_ptr->summoned = MIN_TOWN_WARNING - 1;
				}
				/* Allies leave */
				else if (m_ptr->mflag & (MFLAG_ALLY))
				{
					/* Note loss of allegiance. */
					monster_speech(m_idx, comment_2a[rand_int(MAX_COMMENT_2a)], FALSE);
				}
				/* Only betray when the monster has support except rarely */
				else if ((tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), comment_2b[rand_int(MAX_COMMENT_2b)]))
						|| !(rand_int(10)))
				{
					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					/* Warn the player */
					msg_format("%^s turns to fight.",m_name);

					/* Become an enemy */
					m_ptr->mflag &= ~(MFLAG_TOWN);

					/* Don't allow the player to buy his way out of trouble */
					m_ptr->mflag &= (MFLAG_AGGR);
				}

				/* Give the player some more time to make amends */
				m_ptr->summoned = 150 - adj_chr_gold[p_ptr->stat_ind[A_CHR]];
			}

			/* Delete monster summoned by player - transform lemures */
			if ((m_ptr->mflag & (MFLAG_ALLY)) || (r_ptr->grp_idx == 28))
			{
				s16b this_o_idx, next_o_idx = 0;
				
				object_type *o_ptr, *i_ptr;
				object_type object_type_body;

				/* Drop a chrysalis */
				if (r_ptr->grp_idx == 28)
				{
					/* Get local object */
					i_ptr = &object_type_body;

					/* Copy the object */
					object_prep(i_ptr, lookup_kind(TV_EGG, SV_EGG_CHRYSALIS));
					
					/* Set charges */
					i_ptr->timeout = 66 /* 6 */;
					
					/* Set weight */
					i_ptr->weight *= 15;
					
					/* Set race */
					i_ptr->name3 = poly_r_idx(y, x, m_ptr->r_idx, TRUE, FALSE, FALSE);
					
					/* Make friendly if required */
					if (m_ptr->mflag & (MFLAG_ALLY)) i_ptr->ident |= (IDENT_FORGED);
					
					/* Drop it */
					drop_near(i_ptr, -1, y, x, TRUE);
				}

				/* Drop objects being carried - unless a mercenary */
				if ((m_ptr->mflag & (MFLAG_TOWN)) == 0)
				{
					for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
					{
						/* Get the object */
						o_ptr = &o_list[this_o_idx];

						/* Get the next object */
						next_o_idx = o_ptr->next_o_idx;

						/* Paranoia */
						o_ptr->held_m_idx = 0;

						/* Get local object */
						i_ptr = &object_type_body;

						/* Copy the object */
						object_copy(i_ptr, o_ptr);

						/* Delete the object */
						delete_object_idx(this_o_idx);

						/* Drop it */
						drop_near(i_ptr, -1, y, x, TRUE);
					}

					/* Forget objects */
					m_ptr->hold_o_idx = 0;
				}

				/* Get the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* "Kill" the monster */
				delete_monster_idx(m_idx);

				/* Give detailed messages if destroyed */
				msg_format("%^s leaves your service.", m_name);

				/* Redraw the monster grid */
				lite_spot(y, x);

				/* Paranoia */
				return;
			}
		}
	}

	/* Get hit by terrain continuously */
	if (place_monster_here(y, x, m_ptr->r_idx) < MM_FAIL)
	{
		bool daytime = (bool)(level_flag & (LF1_DAYLIGHT));

		bool hurt_lite = ((r_ptr->flags3 & (RF3_HURT_LITE)) ? TRUE : FALSE);

		bool outside = (level_flag & (LF1_SURFACE))
			&& (f_info[cave_feat[y][x]].flags3 & (FF3_OUTSIDE));

		/* Hack -- silently wake monster */
		m_ptr->csleep = 0;

		if (daytime && hurt_lite && outside)
		{
			/* Burn the monster */
			project_one(SOURCE_DAYLIGHT, 0, y, x, damroll(4,6), GF_LITE, (PROJECT_KILL));
		}
		else if ((f_info[cave_feat[y][x]].blow.method) && !(f_info[cave_feat[y][x]].flags1 & (FF1_HIT_TRAP)))
		{
			mon_hit_trap(m_idx,y,x);
		}

		/* Suffocate */
		else
		{
			/* Suffocate the monster */
			project_one(SOURCE_FEATURE, cave_feat[y][x], y, x, damroll(4,6), GF_SUFFOCATE, (PROJECT_KILL | PROJECT_HIDE));
		}

		/* Is the monster hidden?*/
		if (m_ptr->mflag & (MFLAG_HIDE))
		{
			/* Unhide the monster */
			m_ptr->mflag &= ~(MFLAG_HIDE);

			/* And reveal */
			update_mon(m_idx,FALSE);

			/* Hack --- tell the player if something unhides */
			if (m_ptr->ml)
			{
				/* Get the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				msg_format("%^s emerges from %s%s.",m_name,
					((f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_FILLED))?"":"the "),
					f_name+f_info[cave_feat[m_ptr->fy][m_ptr->fx]].name);
			}
			
			/* Disturb on "move" */
			if ((m_ptr->ml && (disturb_move ||
				((m_ptr->mflag & (MFLAG_VIEW)) && disturb_near)))
					&& ((m_ptr->mflag & (MFLAG_ALLY)) == 0))
			{
				/* Disturb */
				disturb(0, 0);
			}
		}
	}

	/* Every 100 game turns, regenerate monsters */
	else if ((regen) && !(m_ptr->cut) && !(m_ptr->poisoned))
	{
		/* Regenerate mana, if needed */
		if (m_ptr->mana < r_ptr->mana)
		{
			/* Monster regeneration depends on maximum mana */
			int frac = r_ptr->mana / 30;

			/* Minimal regeneration rate */
			if (!frac) frac = 1;

			/* Some monsters regenerate quickly */
			if (r_ptr->flags2 & (RF2_REGENERATE)) frac *= 2;

			/* Inactive monsters rest */
			if (!(m_ptr->mflag & (MFLAG_ACTV))) frac *= 2;

			/* Regenerate */
			m_ptr->mana += frac;

			/* Do not over-regenerate */
			if (m_ptr->mana > r_ptr->mana) m_ptr->mana = r_ptr->mana;

			/* Fully recovered mana -> flag minimum range for recalculation */
			if (m_ptr->mana >= r_ptr->mana / 2) m_ptr->min_range = 0;
		}

		/* Regenerate hit points, if needed */
		if (m_ptr->hp < m_ptr->maxhp)
		{
			/* Base regeneration */
			int frac = m_ptr->maxhp / 100;

			/* Minimal regeneration rate */
			if (!frac) frac = 1;

			/* Some monsters regenerate quickly */
			if (r_ptr->flags2 & (RF2_REGENERATE)) frac *= 2;

			/* Inactive monsters rest */
			if (!(m_ptr->mflag & (MFLAG_ACTV))) frac *= 2;

			/* Regenerate */
			m_ptr->hp += frac;

			/* Do not over-regenerate */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Fully healed -> flag minimum range for recalculation */
			if (m_ptr->hp == m_ptr->maxhp) m_ptr->min_range = 0;
		}


		/* Monster is a town monster - do some interesting stuff */
		if (((m_ptr->mflag & (MFLAG_TOWN)) != 0) && !(m_ptr->csleep) && !(m_ptr->summoned) && !(rand_int(9)))
		{
			/* We don't care about the player */
			if ((m_ptr->mflag & (MFLAG_AGGR | MFLAG_ALLY)) == 0)
			{
				/* Get the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Notice and attack the player */
				if ((m_ptr->cdis <= r_ptr->aaf) && (p_ptr->skills[SKILL_STEALTH] < rand_int(100)))
				{
					/* Give detailed messages */
					if (m_ptr->ml) msg_format("%^s decides you are an easy target.", m_name);

					/* Notice the player */
					m_ptr->mflag &= ~(MFLAG_TOWN);
				}
				/* Bored - go to sleep */
				else
				{
					int val = r_ptr->sleep;
					m_ptr->csleep = ((val * 2) + (s16b)randint(val * 10));

					/* Give detailed messages */
					if (m_ptr->ml) msg_format("%^s falls asleep.", m_name);
				}
			}
		}
	}


	/* Monster is sleeping, but character is within detection range */
	if ((m_ptr->csleep) && (m_ptr->cdis <= r_ptr->aaf))
	{
		u32b notice;

		/* Anti-stealth */
		notice = rand_int(1024);

		/* Aggravated by the player */
		/* Now only when in LOS or LOF */
		if (((p_ptr->cur_flags3 & (TR3_AGGRAVATE)) != 0)
			&& (play_info[m_ptr->fy][m_ptr->fx] & (PLAY_FIRE | PLAY_VIEW)))
		{
			/* Reset sleep counter */
			m_ptr->csleep = 0;

			/* Notice the "waking up" */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s wakes up.", m_name);
			}
		}

		/* Hack -- See if monster "notices" player */
		else if ((notice * notice * notice) <= p_ptr->noise)
		{
			int d = 1;

			/* Wake up faster near the player */
			if (m_ptr->cdis < 50) d = (100 / m_ptr->cdis);

			/* Cap effectiveness of waking up if the player is sneaking */
			/* XXX We do this so that it is never worthwhile just sneaking
			 * around the dungeon, but it is worthwhile switching to sneaking
			 * when you are near a monster you are aware of, which is about
			 * to wake up.
			 */
			if ((p_ptr->sneaking) && !(p_ptr->not_sneaking) && (m_ptr->ml) && (d > 20)) d = 20;

			/* Still asleep */
			if (m_ptr->csleep > d)
			{
				/* Monster wakes up "a little bit" */
				m_ptr->csleep -= d;

				/* Notice the "not waking up" */
				if (m_ptr->ml)
				{
					/* Hack -- Count the ignores */
					if (l_ptr->ignore < MAX_UCHAR)
					{
						l_ptr->ignore++;
					}

					/* Notice when nearly awake */
					if (m_ptr->csleep < 2 * d)
					{
						/* Get the monster name */
						monster_desc(m_name, sizeof(m_name), m_idx, 0);

						/* Dump a message */
						msg_format("%^s is nearly awake.", m_name);
					}
					/* Notice when waking up a lot */
					else if ((m_ptr->cdis < 6) && (m_ptr->csleep < 3 * d))
					{
						/* Get the monster poss */
						monster_desc(m_name, sizeof(m_name), m_idx, 0x02);

						/* Dump a message */
						msg_format("%^s slumber is disturbed.", m_name);
					}
				}
			}

			/* Just woke up */
			else
			{
				/* Reset sleep counter */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					/* Dump a message */
					msg_format("%^s wakes up.", m_name);

					/* Hack -- Update the health bar */
					if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);

					/* Hack -- Count the wakings */
					if (l_ptr->wake < MAX_UCHAR)
					{
						l_ptr->wake++;
					}
				}
			}
		}
	}


	/* Some monsters radiate damage when awake */
	if (!(m_ptr->csleep) && (r_ptr->flags2 & (RF2_HAS_AURA)))
	{
		(void)make_attack_ranged(m_idx,96+7,m_ptr->fy,m_ptr->fx);
	}


	/* Recover from stuns */
	if (m_ptr->stunned)
	{
		int d = 1;

		/* Make a "saving throw" against stun. */
		if (rand_int(330) < r_ptr->level + 10)
		{
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d)
		{
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->stunned = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer stunned.", m_name);
			}
		}
	}


	/* Recover from confusion */
	if (m_ptr->confused)
	{
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->confused > d)
		{
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->confused = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer confused.", m_name);
			}
		}
	}


	/* Recover from daze */
	if (m_ptr->dazed)
	{
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->dazed > d)
		{
			/* Reduce the confusion */
			m_ptr->dazed -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->dazed = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer dazed.", m_name);
			}
		}
	}


	/* Recover from hallucination */
	if (m_ptr->image)
	{
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->image > d)
		{
			/* Reduce the confusion */
			m_ptr->image -= d;

			/* Hack -- ensure this monster targets those around it */
			m_ptr->mflag |= (MFLAG_IGNORE);
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->image = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer drugged.", m_name);
			}
		}
	}


	/* Recover from terror */
	if (m_ptr->terror)
	{
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->terror > d)
		{
			/* Reduce the confusion */
			m_ptr->terror -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->terror = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer terrified.", m_name);
			}
		}
	}


	/* Recover from amnesia */
	if (m_ptr->amnesia)
	{
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->amnesia > d)
		{
			/* Reduce the confusion */
			m_ptr->amnesia -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->amnesia = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer forgetful.", m_name);
			}
		}
	}


	/* Recover courage */
	if (m_ptr->monfear)
	{
		/* Random recovery from fear - speed based on current hitpoints */
		int d = randint(m_ptr->hp / 10);

		/* Still afraid */
		if (m_ptr->monfear > d)
		{
			/* Reduce the fear */
			m_ptr->monfear -= d;
		}

		/* Recover from fear, take note if seen */
		else
		{
			/* No longer afraid */
			set_monster_fear(m_ptr, 0, FALSE);

			/* Recalculate minimum range immediately */
			find_range(m_idx);

			/* Visual note - only if monster isn't terrified */
			if ((m_ptr->ml) && (m_ptr->min_range != FLEE_RANGE))
			{
				/* Acquire the monster name/poss */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);
				monster_desc(m_poss, sizeof(m_poss), m_idx, 0x22);

				/* Dump a message */
				msg_format("%^s recovers %s courage.", m_name, m_poss);
			}
		}
	}


	/* Recover from cuts */
	if (m_ptr->cut)
	{
		bool fear;

		int d = 1;

		/* Some monsters regenerate cuts quickly */
		if (r_ptr->flags2 & (RF2_REGENERATE)) d *= 2;

		/* Some wounds are mortal */
		if (m_ptr->cut > 250) d = 0;

		/* Hack -- Recover from cuts */
		if (m_ptr->cut > d)
		{
			/* Recover somewhat */
			m_ptr->cut -= d;

			/* Bleed in place */
			if ((f_info[cave_feat[m_ptr->fy][m_ptr->fy]].flags1 & (FF1_FLOOR)) &&
				(rand_int(100) < m_ptr->cut))
			{
				if (r_ptr->flags8 & (RF8_HAS_BLOOD))
					cave_set_feat(m_ptr->fy, m_ptr->fx, FEAT_FLOOR_BLOOD_T);
				else
					cave_set_feat(m_ptr->fy, m_ptr->fx, FEAT_FLOOR_SLIME_T);
			}

			/* Notice if bleeding would kill monster */
			if ((m_ptr->ml) && (m_ptr->hp < (m_ptr->cut > 200 ? 3 : (m_ptr->cut > 100 ? 2 : 1))))
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s bleeds to death.", m_name);
			}

			/* Take damage - only players can cut monsters */
			mon_take_hit(m_idx, (m_ptr->cut > 200 ? 3 : (m_ptr->cut > 100 ? 2 : 1)), &fear, NULL);
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->cut = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer bleeding.", m_name);
			}
		}
	}


	/* Recover from poison */
	if (m_ptr->poisoned)
	{
		int d = 1;

		/* Hack -- Recover from cuts */
		if (m_ptr->poisoned > d)
		{
			bool fear;

			/* Recover somewhat */
			m_ptr->poisoned -= d;

			/* Notice if poison would kill monster */
			if ((m_ptr->ml) && (m_ptr->hp < 1))
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s expires from poisoning.", m_name);
			}

			/* Take damage - only players can poison monsters */
			mon_take_hit(m_idx, 1, &fear, NULL);
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->poisoned = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer poisoned.", m_name);
			}
		}
	}



	/* Recover from blind -- slower than confusion */
	if (m_ptr->blind)
	{
		int d = 1;

		/* Still confused */
		if (m_ptr->blind > d)
		{
			/* Reduce the confusion */
			m_ptr->blind -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->blind = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer blinded.", m_name);
			}
		}
	}


	/*
	 * Handle haste counter
	 */
	if (m_ptr->hasted)
	{

		/*efficiency*/
		if (m_ptr->hasted > 1) m_ptr->hasted -= 1;

		/*set to 0 and give message*/
		else set_monster_haste(m_idx, 0, m_ptr->ml);

	}

	/*
	 * Handle slow counter
	 */
	if (m_ptr->slowed)
	{

		/*efficiency*/
		if (m_ptr->slowed > 1) m_ptr->slowed -= 1;

		/*set to 0 and give message*/
		else set_monster_slow(m_idx, 0, m_ptr->ml);

	}

	/*
	 * Handle petrify counter
	 */
	if (m_ptr->petrify)
	{
		int d = 1;

		/* Still invisible */
		if (m_ptr->petrify > d)
		{
			/* Reduce the confusion */
			m_ptr->petrify -= d;
		}

		/* Expired */
		else
		{
			/* No longer invisible */
			m_ptr->petrify = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				if ((r_ptr->flags1 & (RF1_NEVER_MOVE)) == 0) msg_format("%^s is now able to move again.", m_name);
			}

			/* As we can now move, need to find new range */
			find_range(m_idx);
		}
	}


	/*
	 * Handle timed invisibility counter
	 */
	if (m_ptr->tim_invis)
	{
		int d = 1;

		/* Still invisible */
		if (m_ptr->tim_invis > d)
		{
			/* Reduce the confusion */
			m_ptr->tim_invis -= d;
		}

		/* Expired */
		else
		{
			/* No longer invisible */
			m_ptr->tim_invis = 0;

			/* And reveal */
			if (!m_ptr->ml)
			{
				update_mon(m_idx,FALSE);

				/* Hack --- tell the player if something unhides */
				if (m_ptr->ml)
				{
					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					msg_format("%^s appears from nowhere.",m_name);

					/* Learn about ability -- can be cast on others */
					if ((r_ptr->flags6 & (RF6_INVIS)) != 0) l_ptr->flags6 |= (RF6_INVIS);
				}
			}
		}
	}


	/*
	 * Handle passwall counter
	 */
	if (m_ptr->tim_passw)
	{
		int d = 1;

 		/* Still invisible */
		if (m_ptr->tim_passw > d)
		{
			/* Reduce the confusion */
			m_ptr->tim_passw -= d;
		}

 		/* Hack -- second chance for unseen monsters */
		else if (!m_ptr->ml)
		{
			/* Keep passing wall */
			m_ptr->tim_passw = 1;
		}

		/* Expired */
		else
		{
			feature_type *f_ptr = &f_info[cave_feat[m_ptr->fy][m_ptr->fx]];

			/* No longer passing wall */
			m_ptr->tim_passw = 0;

			/* Monster is hidden? */
			if (m_ptr->mflag & (MFLAG_HIDE))
			{
				/* Unhide the monster */
				m_ptr->mflag &= ~(MFLAG_HIDE);

				/* And reveal */
				update_mon(m_idx,FALSE);

				/* Hack --- tell the player if something unhides */
				if (m_ptr->ml)
				{
					/* Get the monster name */
					monster_desc(m_name, sizeof(m_name), m_idx, 0);

					msg_format("%^s emerges from %s%s.",m_name,
						((f_ptr->flags2 & (FF2_FILLED))?"":"the "), f_name+f_ptr->name);
				}
			}

			/* Hack -- crush stuck monsters */
			if (((f_ptr->flags1 & (FF1_MOVE)) == 0) && ((f_ptr->flags3 & (FF3_EASY_CLIMB)) == 0))
			{
				entomb(m_ptr->fy, m_ptr->fx, 0x00);
			}
		}
	}


	/*
	 * Handle blessing counter
	 */
	if (m_ptr->bless)
	{
		int d = 1;

		/* Still blessed */
		if (m_ptr->bless > d)
		{
			/* Reduce the confusion */
			m_ptr->bless -= d;
		}

		/* Expired */
		else
		{
			/* No longer blessed */
			m_ptr->bless = 0;

			/* Silently expires */
		}
	}


	/*
	 * Handle berserk counter
	 */
	if (m_ptr->berserk)
	{
		int d = 1;

		/* Still invisible */
		if (m_ptr->berserk > d)
		{
			/* Reduce the confusion */
			m_ptr->berserk -= d;
		}

		/* Expired */
		else
		{
			/* No longer invisible */
			m_ptr->berserk = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				/* Acquire the monster name */
				monster_desc(m_name, sizeof(m_name), m_idx, 0);

				/* Dump a message */
				msg_format("%^s is no longer berserk.", m_name);
			}
		}
	}

	/*
	 * Handle shielded counter
	 */
	if (m_ptr->shield)
	{
		int d = 1;

		/* Still invisible */
		if (m_ptr->shield > d)
		{
			/* Reduce the confusion */
			m_ptr->shield -= d;
		}

		/* Expired */
		else
		{
			/* No longer invisible */
			m_ptr->shield = 0;

			/* Silently expires */
		}
	}


	/*
	 * Handle opposition to elements counter
	 */
	if (m_ptr->oppose_elem)
	{
		int d = 1;

		/* Still opposed to the elements */
		if (m_ptr->oppose_elem > d)
		{
			/* Reduce the confusion */
			m_ptr->oppose_elem -= d;
		}

		/* Expired */
		else
		{
			/* No longer opposed to the elements */
			m_ptr->oppose_elem = 0;

			/* Silently expires */
		}
	}


	/* Hack -- Update the health bar (always) */
	if (p_ptr->health_who == m_idx)
		p_ptr->redraw |= (PR_HEALTH);
}


/*
 * Process all living monsters, once per game turn.
 *
 * Scan through the list of all living monsters, (backwards, so we can
 * excise any "freshly dead" monsters).
 *
 * Every ten game turns, allow monsters to recover from temporary con-
 * ditions.  Every 100 game turns, regenerate monsters.  Give energy to
 * each monster, and allow fully energized monsters to take their turns.
 *
 * This function and its children are responsible for at least a third of
 * the processor time in normal situations.  If the character is resting,
 * this may rise substantially.
 */
void process_monsters(byte minimum_energy)
{
	int i;
	monster_type *m_ptr;

	/* Only process some things every so often */
	bool recover = FALSE;
	bool regen = FALSE;

	/* Time out temporary conditions every ten game turns */
	if (turn % 10 == 0)
	{
		recover = TRUE;

		/* Regenerate hitpoints and mana every 100 game turns */
		if (turn % 100 == 0) regen = TRUE;
	}

	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Player is dead or leaving the current level */
		if (p_ptr->leaving) break;

		/* Access the monster */
		m_ptr = &m_list[i];

		/* Ignore dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Ignore monsters that have already been handled */
		if (m_ptr->mflag & (MFLAG_MOVE)) continue;

		/* Leave monsters without enough energy for later */
		if (m_ptr->energy < minimum_energy) continue;

		/* Prevent reprocessing */
		m_ptr->mflag |= (MFLAG_MOVE);

		/* Handle temporary monster attributes every ten game turns */
		if (recover) recover_monster(i, regen);

		/* Give the monsters some energy */
		m_ptr->energy += extract_energy[m_ptr->mspeed];

		/* End the turn of monsters without enough energy to move */
		if (m_ptr->energy < 100) continue;

		/* Use up some energy */
		m_ptr->energy -= 100;

		/* Let the monster take its turn */
		process_monster(i);
	}
}


/*
 * Clear 'moved' status from all monsters.
 *
 * Clear noise if appropriate.
 */
void reset_monsters(void)
{
	int i;
	monster_type *m_ptr;

	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];

		/* Monster is ready to go again */
		m_ptr->mflag &= ~(MFLAG_MOVE);
	}
}

