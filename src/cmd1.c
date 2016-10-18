/* File: cmd1.c */

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
 * Determine if the player "hits" a monster (normal combat).
 *
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Invisible monsters are harder to hit */
	if (!vis) chance = chance / 2;

	/* Power competes against armor */
	if ((chance > 0) && (rand_int(chance) >= (ac * 3 / 4))) return (TRUE);

	/* Assume miss */
	return (FALSE);
}



/*
 * Determine if the player "hits" a monster (normal combat).
 *
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_norm(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Penalize invisible targets */
	if (!vis) chance = chance / 2;

	/* Power competes against armor */
	if ((chance > 0) && (rand_int(chance) >= (ac * 3 / 4))) return (TRUE);

	/* Assume miss */
	return (FALSE);
}



/*
 * Critical hits (from objects thrown by player)
 * Factor in item weight, total plusses, and intelligence.
 */
sint critical_shot(int weight, int plus, int dam)
{
	int i, k, crit = 0;

	/* Extract "shot" power */
	i = weight + plus * 4 + adj_int_th[p_ptr->stat_ind[A_INT]] * 8;

	/* Critical hit */
	if (randint(5000) <= i)
	{
		k = weight + randint(500) + plus * 5;

		if (k < 500)
		{
			crit = dam + 5;
		}
		else if (k < 1000)
		{
			crit = ((dam * 3) / 2) + 10;
		}
		else
		{
			crit = dam * 2 + 15;
		}
	}

	return (crit);
}



/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, and intelligence.
 */
sint critical_norm(int weight, int plus, int dam)
{
	int i, k, crit = 0;

	/* Extract "blow" power */
	i = weight + plus * 5 + adj_int_th[p_ptr->stat_ind[A_INT]] * 10;

	/* Chance */
	if (randint(5000) <= i)
	{
		k = weight + randint(650) + plus * 5;

		if (k < 400)
		{
			crit = dam + 5;
		}
		else if (k < 700)
		{
			crit = ((dam * 3) / 2) + 10;
		}
		else if (k < 900)
		{
			crit = dam * 2 + 15;
		}
		else if (k < 1300)
		{
			crit = ((dam * 5) / 2) + 20;
		}
		else
		{
			crit = dam * 3 + 25;
		}
	}

	return (crit);
}



/*
 * Extract the "total damage" from a given object hitting a given monster.
 *
 * Note that most brands and slays are x3, except Slay Animal (x2),
 * Slay Evil (x2), and Kill dragon (x5).
 * Brand lite and brand dark are also (x2).
 *
 * Acid damage is only (x2) against armoured opponents.
 */
sint object_damage_multiplier(object_type *o_ptr, const monster_type *m_ptr, bool floor)
{
	int mult = 1;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	u32b f1, f2, f3, f4;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Hack -- apply player branding */
	if (!p_ptr->branded_blows) /* Nothing */ ;
	else if (p_ptr->branded_blows < 33) f1 |= 1L << (p_ptr->branded_blows - 1);
	else if (p_ptr->branded_blows < 65) f2 |= 1L << (p_ptr->branded_blows - 33);
	else if (p_ptr->branded_blows < 97) f3 |= 1L << (p_ptr->branded_blows - 65);
	else if (p_ptr->branded_blows < 129) f4 |= 1L << (p_ptr->branded_blows - 97);

	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_STAFF:
		case TV_GLOVES:
		{
			/* Slay Animal */
			if ((f1 & (TR1_SLAY_NATURAL)) &&
			    (r_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 17) object_can_flags(o_ptr,TR1_SLAY_NATURAL,0x0L,0x0L,0x0L, floor);
					if (r_ptr->flags3 & RF3_ANIMAL) l_ptr->flags3 |= (RF3_ANIMAL);
					if (r_ptr->flags3 & RF3_PLANT) l_ptr->flags3 |= (RF3_PLANT);
					if (r_ptr->flags3 & RF3_INSECT) l_ptr->flags3 |= (RF3_INSECT);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags3 & (RF3_ANIMAL | RF3_PLANT | RF3_INSECT)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_NATURAL,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Undead */
			if ((f1 & (TR1_SLAY_UNDEAD)) &&
			    (r_ptr->flags3 & (RF3_UNDEAD)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 19) object_can_flags(o_ptr,TR1_SLAY_UNDEAD,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_UNDEAD);
				}

				if (mult < 3) mult = 3;
			}
			else if ((r_ptr->flags3 & (RF3_UNDEAD)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_UNDEAD,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Demon */
			if ((f1 & (TR1_SLAY_DEMON)) &&
			    (r_ptr->flags3 & (RF3_DEMON)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 20) object_can_flags(o_ptr,TR1_SLAY_DEMON,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_DEMON);
				}
				if (mult < 3) mult = 3;
			}
			else if ((l_ptr->flags3 & (RF3_DEMON)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_DEMON,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Dragon */
			if ((f1 & (TR1_SLAY_DRAGON)) &&
			    (r_ptr->flags3 & (RF3_DRAGON)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 24) object_can_flags(o_ptr,TR1_SLAY_DRAGON,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_DRAGON);
				}

				if (mult < 3) mult = 3;
			}
			else if ((l_ptr->flags3 & (RF3_DRAGON)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_DRAGON,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Orc */
			if ((f1 & (TR1_SLAY_ORC)) &&
			    (r_ptr->flags3 & (RF3_ORC)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 21) object_can_flags(o_ptr,TR1_SLAY_ORC,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_ORC);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags3 & (RF3_ORC)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_ORC,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Troll */
			if ((f1 & (TR1_SLAY_TROLL)) &&
			    (r_ptr->flags3 & (RF3_TROLL)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 22) object_can_flags(o_ptr,TR1_SLAY_TROLL,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_TROLL);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags3 & (RF3_TROLL)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_TROLL,0x0L,0x0L,0x0L, floor);
			}

			/* Slay Giant */
			if ((f1 & (TR1_SLAY_GIANT)) &&
			    (r_ptr->flags3 & (RF3_GIANT)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 23) object_can_flags(o_ptr,TR1_SLAY_GIANT,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_GIANT);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags3 & (RF3_GIANT)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_SLAY_GIANT,0x0L,0x0L,0x0L, floor);
			}

			/* Execute Dragon */
			if ((f1 & (TR1_KILL_DRAGON)) &&
			    (r_ptr->flags3 & (RF3_DRAGON)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 25) object_can_flags(o_ptr,TR1_KILL_DRAGON,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_DRAGON);
				}

				if (mult < 5) mult = 5;
			}
			else if ((l_ptr->flags3 & (RF3_DRAGON)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_KILL_DRAGON,0x0L,0x0L,0x0L, floor);
			}

			/* Execute demon */
			if ((f1 & (TR1_KILL_DEMON)) &&
			    (r_ptr->flags3 & (RF3_DEMON)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 26) object_can_flags(o_ptr,TR1_KILL_DEMON,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_DEMON);
				}

				if (mult < 5) mult = 5;
			}
			else if ((l_ptr->flags3 & (RF3_DEMON)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_KILL_DEMON,0x0L,0x0L,0x0L, floor);
			}

			/* Execute undead */
			if ((f1 & (TR1_KILL_UNDEAD)) &&
			    (r_ptr->flags3 & (RF3_UNDEAD)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 27) object_can_flags(o_ptr,TR1_KILL_UNDEAD,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_UNDEAD);
				}

				if (mult < 5) mult = 5;
			}
			else if ((l_ptr->flags3 & (RF3_UNDEAD)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_KILL_UNDEAD,0x0L,0x0L,0x0L, floor);
			}

			/* Brand (Acid) */
			if (f1 & (TR1_BRAND_ACID))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_ACID))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 29) object_can_flags(o_ptr,TR1_BRAND_ACID,0x0L,0x0L,0x0L, floor);
						l_ptr->flags3 |= (RF3_IM_ACID);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if ((m_ptr->ml) && (p_ptr->branded_blows != 29)) object_can_flags(o_ptr,TR1_BRAND_ACID,0x0L,0x0L,0x0L, floor);

					/* Armour partially protects the monster */
					if (r_ptr->flags2 & (RF2_ARMOR))
					{
						if (m_ptr->ml)
						{
							if (p_ptr->branded_blows != 29) object_can_flags(o_ptr,TR1_BRAND_ACID,0x0L,0x0L,0x0L, floor);
							l_ptr->flags2 |= (RF2_ARMOR);
						}

						if (m_ptr->oppose_elem)/* Do nothing */;
						else if (mult < 2) mult = 2;
					}

					/* Water decreases damage */
					if (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_WATER))
					{
						if (m_ptr->oppose_elem)/* Do nothing */;
						else if (mult < 2) mult = 2;
					}
					else if ((m_ptr->oppose_elem) && (mult < 2)) mult = 2;
					else if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags3 & (RF3_IM_ACID)))
			{
				object_not_flags(o_ptr,TR1_BRAND_ACID,0x0L,0x0L,0x0L, floor);
			}

			/* Brand (Elec) */
			if (f1 & (TR1_BRAND_ELEC))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_ELEC))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 30) object_can_flags(o_ptr,TR1_BRAND_ELEC,0x0L,0x0L,0x0L, floor);
						l_ptr->flags3 |= (RF3_IM_ELEC);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if ((m_ptr->ml) && (p_ptr->branded_blows != 30)) object_can_flags(o_ptr,TR1_BRAND_ELEC,0x0L,0x0L,0x0L, floor);

					/* Water increases damage */
					if (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_WATER))
					{
						if ((m_ptr->oppose_elem) && (mult < 3)) mult = 3;
						else if (mult < 4) mult = 4;
					}
					else if ((m_ptr->oppose_elem) && (mult < 2)) mult = 2;
					else if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags3 & (RF3_IM_ELEC)))
			{
				object_not_flags(o_ptr,TR1_BRAND_ELEC,0x0L,0x0L,0x0L, floor);
			}

			/* Brand (Fire) */
			if (f1 & (TR1_BRAND_FIRE))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_FIRE))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 31) object_can_flags(o_ptr,TR1_BRAND_FIRE,0x0L,0x0L,0x0L, floor);
						l_ptr->flags3 |= (RF3_IM_FIRE);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if ((m_ptr->ml) && (p_ptr->branded_blows != 31)) object_can_flags(o_ptr,TR1_BRAND_FIRE,0x0L,0x0L,0x0L, floor);

					/* Water decreases damage */
					if (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_WATER))
					{
						if (m_ptr->oppose_elem) ;
						else if (mult < 2) mult = 2;
					}
					else if ((m_ptr->oppose_elem) && (mult < 2)) mult = 2;
					else if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags3 & (RF3_IM_FIRE)))
			{
				object_not_flags(o_ptr,TR1_BRAND_FIRE,0x0L,0x0L,0x0L, floor);
			}

			/* Brand (Cold) */
			if (f1 & (TR1_BRAND_COLD))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_COLD))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 32) object_can_flags(o_ptr,TR1_BRAND_COLD,0x0L,0x0L,0x0L, floor);
						l_ptr->flags3 |= (RF3_IM_COLD);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if ((m_ptr->ml) && (p_ptr->branded_blows != 32)) object_can_flags(o_ptr,TR1_BRAND_COLD,0x0L,0x0L,0x0L, floor);

					/* Water increases damage */
					if (f_info[cave_feat[m_ptr->fy][m_ptr->fx]].flags2 & (FF2_WATER))
					{
						if ((m_ptr->oppose_elem) && (mult < 3)) mult = 3;
						else if (mult < 4) mult = 4;
					}
					else if ((m_ptr->oppose_elem) && (mult < 2)) mult = 2;
					else if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags3 & (RF3_IM_COLD)))
			{
				object_not_flags(o_ptr,TR1_BRAND_COLD,0x0L,0x0L,0x0L, floor);
			}

			/* Brand (Poison) */
			if (f1 & (TR1_BRAND_POIS))
			{
				/* Notice immunity */
				if (r_ptr->flags3 & (RF3_IM_POIS))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 28) object_can_flags(o_ptr,TR1_BRAND_POIS,0x0L,0x0L,0x0L, floor);
						l_ptr->flags3 |= (RF3_IM_POIS);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if (m_ptr->ml) if (p_ptr->branded_blows != 28) object_can_flags(o_ptr,TR1_BRAND_POIS,0x0L,0x0L,0x0L, floor);

					if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags3 & (RF3_IM_POIS)))
			{
				object_not_flags(o_ptr,TR1_BRAND_POIS,0x0L,0x0L,0x0L, floor);
			}

			/* Brand Holy */
			if ((f1 & (TR1_BRAND_HOLY)) &&
			    (r_ptr->flags3 & (RF3_EVIL)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 18) object_can_flags(o_ptr,TR1_BRAND_HOLY,0x0L,0x0L,0x0L, floor);
					l_ptr->flags3 |= (RF3_EVIL);
				}

				if (mult < 3) mult = 3;
			}
			else if ((l_ptr->flags3 & (RF3_EVIL)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,TR1_BRAND_HOLY,0x0L,0x0L,0x0L, floor);
			}

			/* Brand lite */
			if ((f4 & (TR4_BRAND_LITE)) &&
			    (r_ptr->flags3 & (RF3_HURT_LITE)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 98) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_BRAND_LITE, floor);
					l_ptr->flags3 |= (RF3_HURT_LITE);
				}

				if (mult < 3) mult = 3;
			}
			else if ((l_ptr->flags3 & (RF3_HURT_LITE)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_BRAND_LITE, floor);
			}

			/* Brand (Dark) */
			if (f4 & (TR4_BRAND_DARK))
			{
				/* Notice immunity */
				if (r_ptr->flags9 & (RF9_RES_DARK))
				{
					if (m_ptr->ml)
					{
						if (p_ptr->branded_blows != 97) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_BRAND_DARK, floor);
						l_ptr->flags9 |= (RF9_RES_DARK);
					}
				}

				/* Otherwise, take the damage */
				else
				{
					if ((m_ptr->ml) && (p_ptr->branded_blows != 97)) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_BRAND_DARK, floor);

					if (mult < 3) mult = 3;
				}
			}
			else if ((m_ptr->ml) && (l_ptr->flags9 & (RF9_RES_DARK)))
			{
				object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_BRAND_DARK, floor);
			}

			/* Slay man */
			if ((f4 & (TR4_SLAY_MAN)) &&
			    (r_ptr->flags9 & (RF9_MAN)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 106) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_MAN, floor);
					l_ptr->flags9 |= (RF9_MAN);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags9 & (RF9_MAN)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_MAN, floor);
			}

			/* Slay elf - includes Maia */
			if ((f4 & (TR4_SLAY_ELF)) &&
			    (r_ptr->flags9 & (RF9_ELF)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 107) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_ELF, floor);
					l_ptr->flags9 |= (RF9_ELF);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags9 & (RF9_ELF)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_ELF, floor);
			}

			/* Slay dwarf */
			if ((f4 & (TR4_SLAY_DWARF)) &&
			    (r_ptr->flags9 & (RF9_DWARF)))
			{
				if (m_ptr->ml)
				{
					if (p_ptr->branded_blows != 108) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_DWARF, floor);
					l_ptr->flags9 |= (RF9_DWARF);
				}

				if (mult < 4) mult = 4;
			}
			else if ((l_ptr->flags9 & (RF9_ELF)) && (m_ptr->ml))
			{
				object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_SLAY_DWARF, floor);
			}

			break;

		}
	}

	return (mult);
}


/*
 * The above computation for legacy purposes.
 */
sint tot_dam_mult(int tdam, int mult)
{
	/* Hack -- if dice roll less than three, treat as three for adding multiplier only */
	if (tdam < 3)
	{
		return (tdam + 3 * (mult - 1));
	}

	/* Return the total damage */
	return (tdam * mult);
}




/*
 * Find a secret at the specified location and change it according to
 * the state.
 *
 * TODO: It's very repetitive having to walk up a corridor and find
 * every square is dusty. We should flow out from the initial secret
 * and find all other nearby squares that have the same feature, and
 * reveal the secrets on those as well. e.g. adjacent secret doors,
 * dusty corridors, deep water and so on.
 */
void find_secret(int y, int x)
{
	feature_type *f_ptr;

	cptr text;

	/* Get feature */
	f_ptr = &f_info[cave_feat[y][x]];

	/* Get the feature description */
	text = (f_text + f_ptr->text);

	if (strlen(text))
	{
		/* You have found something */
		msg_format("%s",text);
	}

	/* Change the location */
	cave_alter_feat(y,x,FS_SECRET);

	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Disturb */
	disturb(0, 0);
}


/*
 * Search for hidden things
 */
void search(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, chance;

	int range = 1;

	/* Start with base search ability, modified by depth */
	chance = 10 + (2 * p_ptr->skills[SKILL_SEARCH]) - p_ptr->depth;

	/* Penalize various conditions */
	if (p_ptr->timed[TMD_BLIND] || no_lite()) chance = chance / 10;
	if (p_ptr->timed[TMD_BLIND] || p_ptr->timed[TMD_IMAGE]) chance = chance / 10;

	/* Increase searching range sometimes */
	if (chance >= rand_range(40,  70)) range++;
	if (chance >= rand_range(80, 140)) range++;

	/* Search all grids in range and in los */
	for (y = py - range; y <= py + range; y++)
	{
		for (x = px - range; x <= px + range; x++)
		{
			/* Get adjusted chance */
			int chance2 = chance - ((range-1) * 40);

			/* Require that grid be fully in bounds, in LOS and lit */
			if (!in_bounds_fully(y, x)) continue;
			if (!generic_los(py, px, y, x, CAVE_XLOS)) continue;
			if (((cave_info[y][x] & (CAVE_LITE)) == 0) &&
				((play_info[y][x] & (PLAY_LITE)) == 0)) continue;

			/* Sometimes, notice things */
			if (rand_int(100) < chance2)
			{
				if (f_info[cave_feat[y][x]].flags1 & (FF1_SECRET))
				{
					find_secret(y,x);
				}
			}
		}
	}
}



/*
 * Objects that combine with items already in the quiver get picked
 * up, placed in the quiver, and combined automatically.
 */
bool quiver_combine(object_type *o_ptr, int o_idx)
{
	int i;

	object_type *i_ptr, *j_ptr = NULL;

	char name[80];

	/* Must be ammo. */
	if (!ammo_p(o_ptr)
	    && !(is_known_throwing_item(o_ptr)
		 && wield_slot(o_ptr) >= INVEN_WIELD))
	  return (FALSE);

	/* Known or sensed cursed ammo is avoided */
	if (cursed_p(o_ptr) && ((o_ptr->ident & (IDENT_SENSE)) || object_known_p(o_ptr))) return (FALSE);

	/* Check quiver space */
	if (!quiver_carry_okay(o_ptr, o_ptr->number, -1)) return (FALSE);

	/* Check quiver for similar objects. */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* Get object in that slot. */
		i_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!i_ptr->k_idx)
		{
			/* But save first empty slot, see later */
			if (!j_ptr) j_ptr = i_ptr;

			continue;
		}

		/* Look for similar. */
		if (object_similar(i_ptr, o_ptr))
		{

			/* Absorb floor object. */
			object_absorb(i_ptr, o_ptr, FALSE);

			/* Remember this slot */
			j_ptr = i_ptr;

			/* Done */
			break;
		}
	}

	/* Can't combine the ammo. Search for the "=g" inscription */
	if (i >= END_QUIVER)
	{
		char *s;

		/* Full quiver or no inscription at all */
		if (!j_ptr || !(o_ptr->note)) return (FALSE);

		/* Search the '=' character in the inscription */
		s = strchr(quark_str(o_ptr->note), '=');

		while (TRUE)
		{
			/* We reached the end of the inscription */
			if (!s) return (FALSE);

			/* We found the "=g" inscription */
			if (s[1] == 'g')
			{
				/* Put the ammo in the empty slot */
				object_copy(j_ptr, o_ptr);

				/* Done */
				break;
			}

			/* Keep looking */
			s = strchr(s + 1, '=');
		}
	}

	/*
	 * Increase carried weight.
	 * Note that o_ptr has the right number of missiles to add.
	 */
	p_ptr->total_weight += o_ptr->weight * o_ptr->number;

	/* Reorder the quiver, track the index */
	i = reorder_quiver(j_ptr - inventory);

	/* Get the final slot */
	j_ptr = &inventory[i];

	/* Cursed! */
	if (cursed_p(j_ptr))
	{
		/* Warn the player */
		sound(MSG_CURSED);
		msg_print("Oops! It feels deathly cold!");

		mark_cursed_feeling(o_ptr);
	}

	/* Describe the object */
	object_desc(name, sizeof(name), j_ptr, TRUE, 3);

	/* Message */
	msg_format("You have %s (%c).", name, index_to_label(i));

	/* Delete the object */
	if (o_idx >= 0)
	  delete_object_idx(o_idx);

	/* Update "p_ptr->pack_size_reduce" */
	find_quiver_size();

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return (TRUE);
}


/*
 * Objects get picked
 * up, placed in the quiver, and combined automatically.
 *
 * FIXME: make a function with this name that will behave
 * similarly as inven_carry; then use it in do_cmd_apply_rune_or_coating
 * Now, this is just a very ugly copy of the code above,
 * with slight hackish changes.
 */
bool quiver_carry(object_type *o_ptr, int o_idx)
{
	int i;

	object_type *i_ptr, *j_ptr = NULL;

	char name[80];

	/* Must be ammo. */
	if (!ammo_p(o_ptr)
	    && !(is_known_throwing_item(o_ptr)
		 && wield_slot(o_ptr) >= INVEN_WIELD))
	  return (FALSE);

	/* Known or sensed cursed ammo is avoided */
	if (cursed_p(o_ptr) && ((o_ptr->ident & (IDENT_SENSE)) || object_known_p(o_ptr))) return (FALSE);

	/* Check quiver space */
	if (!quiver_carry_okay(o_ptr, o_ptr->number, -1)) return (FALSE);

	/* Check quiver for similar objects. */
	for (i = INVEN_QUIVER; i < END_QUIVER; i++)
	{
		/* Get object in that slot. */
		i_ptr = &inventory[i];

		/* Ignore empty objects */
		if (!i_ptr->k_idx)
		{
			/* But save first empty slot, see later */
			if (!j_ptr) j_ptr = i_ptr;

			continue;
		}

		/* Look for similar. */
		if (object_similar(i_ptr, o_ptr))
		{

			/* Absorb floor object. */
			object_absorb(i_ptr, o_ptr, FALSE);

			/* Remember this slot */
			j_ptr = i_ptr;

			/* Done */
			break;
		}
	}

	/* Can't combine the ammo. Force it */
	if (i >= END_QUIVER)
	{
		/* Full quiver or no inscription at all */
		if (!j_ptr) return (FALSE);

		/* Put the ammo in the empty slot */
		object_copy(j_ptr, o_ptr);
	}

	/*
	 * Increase carried weight.
	 * Note that o_ptr has the right number of missiles to add.
	 */
	p_ptr->total_weight += o_ptr->weight * o_ptr->number;

	/* Reorder the quiver, track the index */
	i = reorder_quiver(j_ptr - inventory);

	/* Get the final slot */
	j_ptr = &inventory[i];

	/* Cursed! */
	if (cursed_p(j_ptr))
	{
		/* Warn the player */
		sound(MSG_CURSED);
		msg_print("Oops! It feels deathly cold!");

		mark_cursed_feeling(o_ptr);
	}

	/* Describe the object */
	object_desc(name, sizeof(name), j_ptr, TRUE, 3);

	/* Message */
	msg_format("You have %s (%c).", name, index_to_label(i));

	/* Delete the object */
	if (o_idx >= 0)
	  delete_object_idx(o_idx);

	/* Update "p_ptr->pack_size_reduce" */
	find_quiver_size();

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP);

	return (TRUE);
}

/*
 * Pickup all gold at the player's current location.
 */
static void py_pickup_gold(int py, int px)
{
	char o_name[80];

	s16b this_o_idx, next_o_idx = 0;

	object_type *o_ptr;

	long value;

	/* Pick up all the ordinary gold objects */
	for (this_o_idx = cave_o_idx[py][px]; this_o_idx; this_o_idx = next_o_idx)
	{
		/* Get the object */
		o_ptr = &o_list[this_o_idx];

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Ignore if not legal treasure */
		if (o_ptr->tval < TV_GOLD)
			continue;

		value = o_ptr->number * o_ptr->charges - o_ptr->stackc;
		object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

		/* Message */
		if (o_ptr->tval == TV_GOLD)
			msg_format("You have found %ld gold pieces worth of %s.",
						  value, o_name);
		else msg_format("You have found %s worth %ld gold pieces.",
							 o_name, value);

		/* Collect the gold */
		p_ptr->au += value;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

		/* Delete the gold */
		delete_object_idx(this_o_idx);

		/* Gathering? */
		/*if (gathered && (scan_feat(py,px) < 0)) cave_alter_feat(py,px,FS_GET_FEAT);*/
	}
}

/*
 * Check to see if an item is stackable in the inventory
 */
bool inven_stack_okay(const object_type *o_ptr)
{
	int j;

	/* Similar slot? */
	for (j = 0; j < INVEN_PACK; j++)
	{
		object_type *j_ptr = &inventory[j];

		/* Skip non-objects */
		if (!j_ptr->k_idx) continue;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr)) return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


/*
 * Determine if the object has a "=d" in its inscription.
 * If "=d" is followed by a number, don't pick up if the player has this number or more in their inventory.
 *
 * Hack -- treat all monster body parts as having a =d, if the option is set.
 */
static bool auto_pickup_never(const object_type *o_ptr)
{
	cptr s;

	/* Ignore corpses */
	if (easy_corpses)
	{
		switch (o_ptr->tval)
		{
			case TV_BONE:
			case TV_BODY:
			case TV_SKIN:
			case TV_JUNK:
			{
				return (TRUE);
			}
		}
	}

	/* No inscription */
	if (!o_ptr->note) return (FALSE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Never pickup on "=d" */
		if (s[1] == 'd')
		{
			if ((s[2] >= '0') && (s[2] <= '9'))
			{
				int i, n;

				n = atoi(s+2);

				for (i = 0; i < INVEN_WIELD; i++)
				{
					if (object_similar(o_ptr, &inventory[i]))
					{
						/* If sufficient, don't pickup */
						if (inventory[i].number > n) return (TRUE);
					}
				}

				/* Allow pickup */
				return (FALSE);
			}

			/* No number specified */
			else return (TRUE);

		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Can pickup */
	return (FALSE);
}


/*
 * Determine if the object can be picked up and has "=g" in its inscription
 * or options determine it should be picked up.
 *
 * If "=g" is followed by a number, pick up if the player has
 * less than this number of similar objects in their inventory.
 */
static bool auto_pickup_okay(const object_type *o_ptr)
{
	cptr s;

	/* It can't be carried */
	if (!inven_carry_okay(o_ptr)) return FALSE;

	/* It can't be picked up */
	if (auto_pickup_never(o_ptr)) return FALSE;
	if (auto_pickup_ignore(o_ptr)) return FALSE;

	if (OPT(pickup_inven) && inven_stack_okay(o_ptr)) return TRUE;
	if (OPT(pickup_always)) return TRUE;

	/* No inscription */
	if (!o_ptr->note) return FALSE;

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Auto-pickup on "=g" */
		if (s[1] == 'g')
		{
			if ((s[2] >= '0') && (s[2] <= '9'))
			{
				int i, n;

				n = atoi(s+2);

				if (o_ptr->number > n) return FALSE;

				for (i = 0; i < INVEN_WIELD; i++)
				{
					if (object_similar(o_ptr, &inventory[i]))
					{
						/* If too many, don't pick up */
						if (inventory[i].number > n) return FALSE;
					}
				}

				/* Pick up */
				return TRUE;
			}

			/* No number specified */
			else return TRUE;

		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Don't auto pickup */
	return FALSE;
}


/*
 * Determine if the object has "=k" in its inscription.
 * If "=k" is followed by a number, destroy if the player has more than this number of similar objects in their inventory.
 */
static bool auto_destroy_okay(const object_type *o_ptr)
{
	cptr s;

	/* No inscription */
	if (!o_ptr->note) return (FALSE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Auto-destroy on "=k" */
		if (s[1] == 'k')
		{
			if ((s[2] >= '0') && (s[2] <= '9'))
			{
				int i, n;

				n = atoi(s+2);

				for (i = 0; i < INVEN_WIELD; i++)
				{
					if (object_similar(o_ptr, &inventory[i]))
					{
						/* If too many, allow destroy */
						if (inventory[i].number > n) return (TRUE);
					}
				}

				/* Don't auto destroy */
				return (FALSE);
			}

			/* No number specified */
			else return (TRUE);
		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Don't auto destroy */
	return (FALSE);
}


/*
 * Determine if the object has "=i" in its inscription.
 * If "=i" is followed by a number, ignore if the player has more than this number of similar objects in their inventory.
 */
bool auto_pickup_ignore(const object_type *o_ptr)
{
	cptr s;

	/* No inscription */
	if (!o_ptr->note) return (FALSE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Auto-ignore on "=i" */
		if (s[1] == 'i')
		{
			if ((s[2] >= '0') && (s[2] <= '9'))
			{
				int i, n;

				n = atoi(s+2);

				for (i = 0; i < INVEN_WIELD; i++)
				{
					if (object_similar(o_ptr, &inventory[i]))
					{
						/* If too many, allow destroy */
						if (inventory[i].number > n) return (TRUE);
					}
				}

				/* Don't auto-ignore */
				return (FALSE);
			}

			/* No number specified */
			else return (TRUE);
		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Don't auto ignore */
	return (FALSE);
}


/*
 * Helper routine for py_pickup() and py_pickup_floor().
 *
 * Destroy objects on the ground.
 */
static void py_destroy_aux(int o_idx)
{
	char o_name[80];
	char out_val[160];

	object_type *o_ptr;

	o_ptr = &o_list[o_idx];

	/* Describe the object */
	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

	/* Verify destruction */
	if (!auto_pickup_ignore(o_ptr))
	{
		sprintf(out_val, "Really destroy %s? ", o_name);
		if (!get_check(out_val)) return;
	}

	/* Take turn */
	p_ptr->energy_use = 100;

	/* Containers release contents */
	if ((o_ptr->tval == TV_HOLD) && (o_ptr->name3 > 0))
	{
		if (animate_object(-o_idx)) return;

		/* Message */
		msg_format("You cannot destroy %s.", o_name);

		return;
	}

	/* Artifacts cannot be destroyed */
	if (artifact_p(o_ptr))
	{

		/* Message */
		msg_format("You cannot destroy %s.", o_name);

		/* Sense the object if allowed, don't sense ID'ed stuff */
		if (!(o_ptr->feeling)
			&& !(o_ptr->ident & (IDENT_SENSE))
			 && !(object_named_p(o_ptr)))
		{
			o_ptr->feeling = INSCRIP_UNBREAKABLE;

			/* The object has been "sensed" */
			o_ptr->ident |= (IDENT_SENSE);

			/* Combine the pack */
			p_ptr->notice |= (PN_COMBINE);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
		}

		/* Done */
		return;

	}

	/* Message */
	if (!auto_pickup_ignore(o_ptr))
	{
		/* Message */
		msg_format("You destroy %s.", o_name);
	}

	/* Delete the object directly */
	delete_object_idx(o_idx);
}



/*
 * Helper routine for py_pickup() and py_pickup_floor().
 *
 * Add the given dungeon object to the character's inventory.
 *
 * Delete the object afterwards.
 */
static void py_pickup_aux(int o_idx, bool msg)
{
	int slot;

	char o_name[80];
	char j_name[80];
	object_type *o_ptr;
	object_type *j_ptr;

	o_ptr = &o_list[o_idx];

	/* Carry the object */
	slot = inven_carry(o_ptr);

	/* Get the object again */
	j_ptr = &inventory[slot];
	
	/* No longer 'stored' */
	j_ptr->ident &= ~(IDENT_STORE);

	/* Optionally, display a message */
	if (msg)
	{
		if (o_ptr->tval != TV_BAG && j_ptr->tval == TV_BAG)
		{
			/* Describe the bag */
			object_desc(j_name, sizeof(j_name), j_ptr, FALSE, 3);

			/* Describe the object */
			object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);
			
			/* Message */
			msg_format("You put %s in your %s (%c).", o_name, j_name, index_to_label(slot));		
		}
		else
		{
			/* Describe the object */
			object_desc(j_name, sizeof(j_name), j_ptr, TRUE, 3);
			
			/* Message */
			msg_format("You have %s (%c).", j_name, index_to_label(slot));
		}
	}
	
	/* Delete the object */
	if (o_idx >= 0) delete_object_idx(o_idx);
}


/*
 * Pick up objects and treasure on the floor.  -LM-
 *
 * Called with pickup:
 * 0 to act according to the player's settings
 * 1 to quickly pickup single objects or present a menu
 * 2 to force a menu for any number of objects
 *
 * Scan the list of objects in that floor grid. Pick up gold automatically.
 * Pick up objects automatically until backpack space is full if
 * auto-pickup option is on. Otherwise, store objects on
 * floor in an array, and tally both how many there are and can be picked up.
 *
 * If not picking up anything, indicate objects on the floor.  Show more
 * details if the "pickup_detail" option is set.  Do the same thing if we
 * don't have room for anything.
 *
 * Pick up multiple objects using Tim Baker's menu system.   Recursively
 * call this function (forcing menus for any number of objects) until
 * objects are gone, backpack is full, or player is satisfied.
 *
 * We keep track of number of objects picked up to calculate time spent.
 * This tally is incremented even for automatic pickup, so we are careful
 * (in "dungeon.c" and elsewhere) to handle pickup as either a separate
 * automated move or a no-cost part of the stay still or 'g'et command.
 */
byte py_pickup(int py, int px, int pickup)
{
	char o_name[80];

	bool gather = FALSE;
	bool gathered2 = FALSE;

	s16b this_o_idx, next_o_idx = 0;

	object_type *o_ptr;

	/* Objects picked up.  Used to determine time cost of command. */
	byte objs_picked_up = 0;

	int floor_num = 0, feature_num = 0,
		floor_list[MAX_FLOOR_STACK + 1], floor_o_idx = 0;

	int can_pickup = 0;
	bool call_function_again = FALSE;

	bool blind = (p_ptr->timed[TMD_BLIND] || no_lite());
	bool msg = TRUE;

	feature_type *f_ptr = &f_info[cave_feat[py][px]];

	/* Hack -- gather features from walls */
	if (!(f_ptr->flags1 & (FF1_MOVE))
		 && !(f_ptr->flags3 & (FF3_EASY_CLIMB))
		 && (f_ptr->flags3 & (FF3_GET_FEAT)))
	{
		gather = TRUE;
	}

	/* Are we allowed to pick up anything here? */
	if (!(f_ptr->flags1 & (FF1_DROP))
		 && (f_ptr->flags1 & (FF1_MOVE))
		 && !gather)
		return 0;

	/* Always pickup gold, effortlessly */
	py_pickup_gold(py, px);

	/* Scan the remaining objects */
	for (this_o_idx = cave_o_idx[py][px]; this_o_idx || gather; this_o_idx = next_o_idx)
	{
		bool gathered = FALSE;

		/* Gathering? */
		if (gather)
		{
			object_type object_type_body;

			o_ptr = &object_type_body;

			/* Hack -- gather once only XXX */
			/* Note that this occurs before make_feat or scan_feat below, as otherwise we end up looking at (nothing)s */
			next_o_idx = cave_o_idx[py][px];
			gather = FALSE;
			gathered = TRUE;

			/* Get the feature */
			if (!make_feat(o_ptr, py, px))
				return 1;

			/* Find the feature */
			this_o_idx = scan_feat(py, px);

			/* And we have to be doubly paranoid */
			if (this_o_idx == next_o_idx)
				next_o_idx = o_list[this_o_idx].next_o_idx;
		}
		else
		{
			/* Get the object and the next object */
			o_ptr = &o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			/* Ignore, but count, 'store' items */
			if (o_ptr->ident & (IDENT_STORE))
			{
				feature_num++;
				continue;
			}

			/* Ignore all hidden objects and non-objects */
			if (!o_ptr->k_idx) continue;

			/* XXX Hack -- Enforce limit */
			if (floor_num >= (int)N_ELEMENTS(floor_list)) break;
		}

		/* Mark the object */
		if (!auto_pickup_ignore(o_ptr))
		{
			o_ptr->ident |= (IDENT_MARKED);

			/* Class learning */
			if (k_info[o_ptr->k_idx].aware & (AWARE_CLASS))
			{
				/* Learn flavor */
				object_aware(o_ptr, TRUE);
			}

			/* Mark objects seen */
			object_aware_tips(o_ptr, TRUE);
		}

		/* Hack -- disturb */
		if (!auto_pickup_never(o_ptr)
			 && !auto_pickup_ignore(o_ptr))
			disturb(0, 0);

		/* Test for auto-destroy */
		if (auto_destroy_okay(o_ptr))
		{
			/* Destroy the object */
			py_destroy_aux(this_o_idx);

			/* Gathering? */
			if (gathered && (scan_feat(py,px) < 0))
				cave_alter_feat(py,px,FS_GET_FEAT);

			/* Check the next object */
			continue;
		}

		/* Test for quiver auto-pickup */
		if (quiver_combine(o_ptr, this_o_idx))
		{
			/* Gathering? */
			if (gathered && (scan_feat(py,px) < 0))
				cave_alter_feat(py,px,FS_GET_FEAT);

			continue;
		}

		/* Automatically pick up items into the backpack */
		if (auto_pickup_okay(o_ptr))
		{
			/* Pick up the object with message */
			py_pickup_aux(this_o_idx, TRUE);
			objs_picked_up++;

			/* Gathering? */
			if (gathered && (scan_feat(py,px) < 0))
				cave_alter_feat(py,px,FS_GET_FEAT);

			continue;
		}

		/* Tally objects and store them in an array. */

		/* Remember this object index */
		floor_list[floor_num] = this_o_idx;

		/* Count non-gold objects that remain on the floor. */
		floor_num++;

		/* Tally objects that can be picked up.*/
		if (inven_carry_okay(o_ptr))
			can_pickup++;
	}

	/* There are no objects left */
	if ((pickup != 1 && floor_num == 0)
		 || floor_num + feature_num == 0)
		return objs_picked_up;

	if (floor_num > 0)
	{

	/* Get hold of the last floor index */
	floor_o_idx = floor_list[floor_num - 1];

	/* Mention the objects if player is not picking them up. */
	if (pickup == 0 || !can_pickup)
	{
		const char *p = "see";

		/* One object */
		if (floor_num == 1)
		{
			if (blind)            p = "feel";
			else if (!can_pickup) p = "have no room for";

			/* Get the object */
			o_ptr = &o_list[floor_o_idx];

			/* Describe the object.  Less detail if blind. */
			if (blind) object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 0);
			else       object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 3);

			/* Message */
			msg_format("You %s %s.", p, o_name);
		}
		else
		{
			/* Optionally, display more information about floor items */
			if (pickup_detail)
			{
				if (blind)            p = "feel something on the floor";
				else if (!can_pickup) p = "have no room for the following objects";

				/* Scan all marked objects in the grid */
				floor_num = scan_floor(floor_list, N_ELEMENTS(floor_list), py, px, 0x0B);

				/* Save screen */
				screen_save();

				/* Display objects on the floor */
				show_floor(floor_list, floor_num, FALSE);

				/* Display prompt */
				prt(format("You %s: ", p), 0, 0);

				/* Move cursor back to character, if needed */
				if (hilite_player) move_cursor_relative(p_ptr->py, p_ptr->px);

				/* Wait for it.  Use key as next command. */
				p_ptr->command_new = inkey_ex();

				/* Restore screen */
				screen_load();
			}

			/* Show less detail */
			else
			{
				message_flush();

				if (!can_pickup)
					msg_print("You have no room for any of the items on the floor.");
				else
					msg_format("You %s a pile of %d items.", (blind ? "feel" : "see"), floor_num);
			}
		}

		/* Done */
		return (objs_picked_up);
	}

	}

	/* We can pick up objects.  Menus are not requested (yet). */
	if (pickup == 1)
	{
		/* Scan floor (again) */
		floor_num = scan_floor(floor_list, N_ELEMENTS(floor_list), py, px, 0x0B);

		/* Use a menu interface for multiple objects or feature objects,
			or pickup single objects */
		if (floor_num == 1)
			this_o_idx = floor_o_idx;
		else
			pickup = 2;
	}

	/* Display a list if requested. */
	if (pickup == 2)
	{
		cptr q, s;
		int item;

		/* Hack for player */
		int oy = p_ptr->py;
		int ox = p_ptr->px;

		bool done = FALSE;

		p_ptr->py = py;
		p_ptr->px = px;

		/* Restrict the choices */
		item_tester_hook = inven_carry_okay;

		/* Request a list */
		p_ptr->command_see = TRUE;

		/* Get an object or exit. */
		q = "Get which item?";
		s = "You see nothing there.";
		if (!get_item(&item, q, s, (USE_FLOOR | USE_FEATG)))
			done = TRUE;

		/* Fix player position */
		p_ptr->py = oy;
		p_ptr->px = ox;

		if (done)
			return (objs_picked_up);

		this_o_idx = 0 - item;

		/* Destroy the feature */
		if (o_list[this_o_idx].ident & (IDENT_STORE))
		{
			gathered2 = TRUE;
			/* Features are somewhat special, disturb */
			msg = TRUE;
			call_function_again = FALSE;
		}
		else
		{
			/* With a list, we do not need explicit pickup messages */
			msg = FALSE;
			call_function_again = TRUE;
		}
	}

	/* Pick up object, if legal */
	if (this_o_idx)
	{
		/* Pick up the object */
		py_pickup_aux(this_o_idx, msg);

		/* Get the feature */
		if (gathered2 && (scan_feat(py,px) < 0))
			cave_alter_feat(py,px,FS_GET_FEAT);

		/* Indicate an object picked up. */
		objs_picked_up = 1;
	}

	/*
	 * If requested, call this function recursively.  Count objects picked
	 * up.  Force the display of a menu in all cases.
	 */
	if (call_function_again)
		objs_picked_up += py_pickup(py, px, 2);

	/* Indicate how many objects have been picked up. */
	return (objs_picked_up);
}


/*
 *  Check if a player avoids the trap
 */
bool avoid_trap(int y, int x, int feat)
{
	feature_type *f_ptr;

	cptr name;

	/* Get feature */
	f_ptr = &f_info[feat];

	/* Get the feature name */
	name = (f_name + f_ptr->name);

	/* Hack - test whether we hit trap based on the attr of the trap */
	switch (f_ptr->d_attr)
	{
		/* Trap door */
		case TERM_WHITE:
		/* Pit */
		case TERM_SLATE:
		{
			/* Avoid by flying */
			break;
		}
		/* Strange rune */
		case TERM_ORANGE:
		{
			/* Avoid by being strange */
			if ((p_ptr->timed[TMD_CONFUSED]) || (p_ptr->timed[TMD_IMAGE]))
			{
				msg_format("You are too strange for the %s.", name);
				return (TRUE);
			}
			break;
		}
		/* Spring loaded trap */
		case TERM_RED:
		{
			/* Avoid by being light footed */
			if (p_ptr->cur_flags3 & (TR3_FEATHER))
			{
				msg_format("You are too light-footed to trigger the %s.", name);
				equip_can_flags(0L, 0L, TR3_FEATHER, 0L);
				return (TRUE);
			}
			break;
		}
		/* Gas trap */
		case TERM_GREEN:
		{
			/* Avoid by dropping stuff to trigger it */
			break;
		}
		/* Explosive trap */
		case TERM_BLUE:
		{
			/* Avoid by dropping stuff to trigger it */
			break;
		}
		/* Discoloured spot */
		case TERM_UMBER:
		{
			/* Avoid by dropping stuff to trigger it */
			break;
		}
		/* Silent watcher */
		case TERM_L_DARK:
		{
			/* Avoid by being invisible */
			if (p_ptr->timed[TMD_INVIS])
			{
				msg_format("The %s cannot see you.", name);
				return (TRUE);
			}
			break;
		}
		/* Strange visage */
		case TERM_L_WHITE:
		{
			/* Avoid by being dragonish */
			if (p_ptr->cur_flags4 & (TR4_DRAGON))
			{
				msg_format("The %s ignores your draconic nature.", name);
				equip_can_flags(0L, 0L, 0L, TR4_DRAGON);
				return (TRUE);
			}
			break;
		}
		/* Loose rock or multiple traps */
		case TERM_L_PURPLE:
		{
			/* Avoid by sneaking */
			if (p_ptr->sneaking)
			{
				/* Can't avoid multiple traps */
				if (strstr("multiple traps", name)) return (FALSE);
				msg_format("Hmm... not this time. You avoid standing on the %s.", name);
				return (TRUE);
			}
			break;
		}
		/* Trip wire */
		case TERM_YELLOW:
		{
			/* Avoid by searching */
			if (p_ptr->searching)
			{
				msg_format("You find the %s and carefully step over it.", name);
				return (TRUE);
			}
			break;
		}
		/* Murder hole */
		case TERM_L_RED:
		{
			/* Arrows always blocked. Do this later to use up ammunition. */
			break;
		}
		/* Ancient hex */
		case TERM_L_GREEN:
		{
			/* Avoid by being undead */
			if (p_ptr->cur_flags4 & (TR4_UNDEAD))
			{
				msg_format("The %s ignores your undead nature.", name);
				equip_can_flags(0L, 0L, 0L, TR4_UNDEAD);
				return (TRUE);
			}
			break;
		}
		/* Magic symbol */
		case TERM_L_BLUE:
		{
			/* Avoid by not having much mana */
			if ((!p_ptr->csp) || (p_ptr->csp < p_ptr->msp / 5))
			{
				msg_format("You lack the magic to trigger the %s.", name);
				return (TRUE);
			}
			break;
		}
		/* Shallow pit */
		case TERM_L_UMBER:
		{
			break;
		}
		/* Surreal painting */
		case TERM_PURPLE:
		{
			/* Most surreal paintings... */
			if ((y + x) % 3)
			{
				/* Avoid by being blind */
				if (p_ptr->timed[TMD_BLIND]) return (TRUE);
			}
			/* But some surreal paintings... */
			else
			{
				/* Avoid by being able to see the painting */
				if (play_info[y][x] & (PLAY_SEEN)) return (TRUE);
			}
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
			if (p_ptr->total_weight > 1000)
			{
				msg_format("The weight of your equipment protects you from the %s.",name);
				return (TRUE);
			}
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
			/* Avoid by not being perma lit */
			if ((cave_info[y][x] & (CAVE_GLOW | CAVE_DLIT)) == 0) return (TRUE);
			break;
		}
		/* Glowing glyph */
		case TERM_MUSTARD:
		{
			/* Avoid by dropping stuff to trigger it */
			break;
		}
		/* Demonic sign */
		case TERM_L_TEAL:
		{
			/* Avoid by being demonic */
			if (p_ptr->cur_flags4 & (TR4_DEMON))
			{
				msg_format("The %s ignores your demonic nature.", name);
				equip_can_flags(0L, 0L, 0L, TR4_DEMON);
				return (TRUE);
			}
			break;
		}
		/* Radagast's snare */
		case TERM_L_VIOLET:
		{
			/* Avoid by being animal */
			if (p_ptr->cur_flags4 & (TR4_ANIMAL))
			{
				msg_format("The %s ignores your animal nature.", name);
				equip_can_flags(0L, 0L, 0L, TR4_ANIMAL);
				return (TRUE);
			}
			break;
		}
		/* Siege engine */
		case TERM_L_PINK:
		/* Clockwork mechanism */
		case TERM_MAGENTA:
		{
			/* Always avoid */
			return (TRUE);
			break;
		}
		/* Mark of a white hand */
		case TERM_BLUE_SLATE:
		{
			/* Avoid by being orc */
			if (p_ptr->cur_flags4 & (TR4_ORC))
			{
				msg_format("The %s ignores your orcish nature.", name);
				equip_can_flags(0L, 0L, 0L, TR4_UNDEAD);
				return (TRUE);
			}
			break;
		}
		/* Shimmering portal */
		case TERM_DEEP_L_BLUE:
		{
			/* Avoid by being anti-teleport */
			if (p_ptr->cur_flags4 & (TR4_ANCHOR))
			{
				msg_print("You are magically anchored in place.");
				equip_can_flags(0L, 0L, 0L, TR4_ANCHOR);
				return (TRUE);
			}
			break;
		}
	}

	/* Can't avoid trap */
	return (FALSE);
}


/*
 * This alters features as per cave_alter_feat, but if
 * the feature is the source of a region, we perform the
 * transition on all features of this type in the region.
 */
void cave_alter_source_feat(int y, int x, int action)
{
	s16b this_region_piece, next_region_piece = 0;

	int feat = cave_feat[y][x];

	/* Iterate through regions on this grid */
	for (this_region_piece = cave_region_piece[y][x]; this_region_piece; this_region_piece = next_region_piece)
	{
		/* Get the region piece */
		region_piece_type *rp_ptr = &region_piece_list[this_region_piece];

		/* Get the region */
		region_type *r_ptr = &region_list[rp_ptr->region];

		/* Get the next object */
		next_region_piece = rp_ptr->next_in_grid;

		/* Skip regions that don't apply */
		if ((r_ptr->flags1 & (RE1_SOURCE_FEATURE)) == 0) continue;
		if ((r_ptr->who != SOURCE_FEATURE) && (r_ptr->who != SOURCE_PLAYER_TRAP)) continue;

		/* Source feature match */
		if (r_ptr->what == feat)
		{
			s16b this_region_piece2, next_region_piece2 = 0;

			/* Change the source */
			r_ptr->what = feat_state(feat, action);

			for (this_region_piece2 = r_ptr->first_piece; this_region_piece2; this_region_piece2 = next_region_piece2)
			{
				/* Get the region piece */
				region_piece_type *rp_ptr2 = &region_piece_list[this_region_piece2];

				/* Get the next object */
				next_region_piece2 = rp_ptr2->next_in_region;

				/* Change it */
				if (cave_feat[rp_ptr2->y][rp_ptr2->x] == feat) cave_alter_feat(rp_ptr2->y, rp_ptr2->x, action);
			}
		}
	}

	/* Change it */
	if (cave_feat[y][x] == feat) cave_alter_feat(y, x, action);

}



/*
 * Handle a trap being discharged.
 *
 * Returns if the player noticed this.
 *
 * Child region is used to create a region instead of directly fire the trap.
 */
bool discharge_trap(int y, int x, int ty, int tx, s16b child_region)
{
	int path_n;

	u16b path_g[256];
	bool obvious = FALSE;
	bool apply_terrain = TRUE;

	int feat = cave_feat[y][x];
	int dam = 0;

	/* Get feature */
	feature_type *f_ptr = &f_info[feat];

	/* Hack --- discover the trap */
	/* XXX XXX Dangerous */
	while (f_ptr->flags3 & (FF3_PICK_TRAP))
	{
		pick_trap(y,x, FALSE);

		/* Error */
		if (cave_feat[y][x] == feat) break;

		feat = cave_feat[y][x];

		/* Get feature */
		f_ptr = &f_info[feat];
	}

	/* Use covered if necessary */
	if (f_ptr->flags2 & (FF2_COVERED))
	{
		feat = f_ptr->mimic;
		f_ptr = &f_info[feat];
	}
	
	/* Object here is used in trap */
	if ((cave_o_idx[y][x]) && (f_ptr->flags1 & (FF1_HIT_TRAP)))
	{
		object_type *o_ptr = &o_list[cave_o_idx[y][x]];

		char o_name[80];

		int power = 0;

		/* By default, don't apply terrain or show messages */
		apply_terrain = FALSE;
		
		/* Object is a mechanism - get next object */
		if ((o_ptr->tval == TV_ASSEMBLY) && (o_ptr->next_o_idx))
		{
			o_ptr = &o_list[o_ptr->next_o_idx];
		}

		switch (o_ptr->tval)
		{
			case TV_BOW:
			case TV_HAFTED:
			case TV_SWORD:
			case TV_POLEARM:
			{
				object_type *j_ptr;
				u32b f1, f2, f3, f4;

				int i, j, shots = 1;
				int tdis = 6;

				/* Get bow */
				j_ptr = o_ptr;

				/* Get bow flags */
				object_flags(o_ptr,&f1,&f2,&f3,&f4);

				/* Apply extra shots and hurls. Note extra shots for weapons other than bows helps for putting weapons in traps only. */
				if (f1 & (TR1_SHOTS)) shots += j_ptr->pval;
				if (f3 & (TR3_HURL_NUM)) shots += j_ptr->pval;
				
				/* Hack - make more skilled trap setters get a small bonus */
				shots += f_ptr->level / 10;

				/* Increase range */
				if (j_ptr->tval == TV_BOW) tdis += bow_multiplier(j_ptr->sval) * 3;

				/* Apply extra might -- note extra might increases range of melee weapons */
				if (f1 & (TR1_MIGHT)) tdis += j_ptr->pval * 3;

				/* Test for hit */
				for (i = 0; i < shots; i++)
				{
					int ny = y;
					int nx = x;

					/* Calculate the path */
					path_n = fire_or_throw_path(path_g, tdis, y, x, &ty, &tx, f_ptr->level < 50 ? 5 - (f_ptr->level / 10): 0);

					/* Do we need ammo */
					if ((j_ptr->next_o_idx) || (o_ptr->tval != TV_BOW))
					{
						int ammo = o_ptr->tval != TV_BOW ? cave_o_idx[y][x] : j_ptr->next_o_idx;
						object_type *i_ptr;
						object_type object_type_body;

						byte missile_attr;
						char missile_char;

						/* Use ammo instead of bow */
						o_ptr = &o_list[ammo];

						/* Find the color and symbol for the object */
						missile_attr = object_attr(o_ptr);
						missile_char = object_char(o_ptr);

						/* Describe ammo */
						object_desc(o_name, sizeof(o_name), o_ptr, TRUE, 0);

						/* Continue along path. Note hack to affect zero length path e.g. straight down. */
						for (j = 0; (j < path_n) || (!path_n && (j < 1)); ++j)
						{
							bool player;

							int msec = op_ptr->delay_factor * op_ptr->delay_factor;

							/* Are we travelling along the path? */
							if (path_n)
							{
								ny = GRID_Y(path_g[j]);
								nx = GRID_X(path_g[j]);
							}

							/* Only do visuals if the player can "see" the missile */
							if (panel_contains(ny, nx) && player_can_see_bold(ny, nx))
							{
								/* Visual effects */
								print_rel(missile_char, missile_attr, ny, nx);
								move_cursor_relative(ny, nx);
								Term_fresh();
								Term_xtra(TERM_XTRA_DELAY, msec);
								lite_spot(ny, nx);
								Term_fresh();
							}

							player = (ny == p_ptr->py) && (nx == p_ptr->px);
							
							/* We see the monster being hit by a trap */
							if (cave_m_idx[ny][nx] > 0)
							{
								monster_type *m_ptr = &m_list[cave_m_idx[ny][nx]];
								monster_lore *l_ptr = &l_list[cave_m_idx[ny][nx]];

								/* We are attacking - count attacks */
								if ((l_ptr->tblows < MAX_SHORT) && (m_ptr->ml)) l_ptr->tblows++;
							}
							
							/* Hack - Block murder holes here to use up ammunition */
							if ((player) && (p_ptr->blocking))
							{
								msg_format("You knock aside the %s.", o_name);
								obvious = TRUE;

								break;
							}
							else if ((ammo) &&
									player ? (test_hit_fire((j_ptr->to_h + o_ptr->to_h)* BTH_PLUS_ADJ + f_ptr->power, p_ptr->ac, TRUE)) :
								(cave_m_idx[ny][nx] > 0 ? test_hit_fire((j_ptr->to_h + o_ptr->to_h)* BTH_PLUS_ADJ + f_ptr->power,  calc_monster_ac(cave_m_idx[ny][nx], TRUE), TRUE)
										: FALSE))
							{
								int k;

								int mult = (j_ptr->tval == TV_BOW) ? bow_multiplier(j_ptr->sval) : 1;

								/* Apply extra might. Note might helps for other weapons to be put in traps only. */
								if (f1 & (TR1_MIGHT)) mult += j_ptr->pval;

								/* Add bow multiplier */
								k = damroll(o_ptr->dd, o_ptr->ds);
								k *= mult;

								/* Hack - make more skilled trap setters get a small bonus */
								shots += f_ptr->level / 10;

								/* Add slay multipliers. TODO: Apply equivalent multipliers for trap affecting player */
								if (!player)
								{
									mult = object_damage_multiplier(o_ptr, &m_list[cave_m_idx[y][x]], TRUE);
									mult = MAX(mult, object_damage_multiplier(j_ptr, &m_list[cave_m_idx[y][x]], TRUE) - 1);

									k = tot_dam_mult(k, mult);
								}

								/* Add other damage multipliers */
								k += critical_shot(o_ptr->weight, o_ptr->to_h + j_ptr->to_h, k);
								k += o_ptr->to_d + j_ptr->to_d;

								/* No negative damage */
								if (k < 0) k = 0;

								/* Trap description */
								if (player) msg_format("%^s hits you.",o_name);

								/* Seen */
								obvious = TRUE;

								/* Apply damage directly */
								project_one(SOURCE_PLAYER_TRAP, o_ptr->k_idx, ny, nx, k, GF_HURT, (PROJECT_PLAY | PROJECT_KILL));

								/* Apply additional effect from coating or sometimes activate */
								if ((coated_p(o_ptr)) || (auto_activate(o_ptr)))
								{
									/* Make item strike */
									process_item_blow(SOURCE_PLAYER_TRAP, o_ptr->k_idx, o_ptr, y, x, TRUE, TRUE);

									/* Hack -- Remove coating on original */
									if ((!coated_p(o_ptr)) && (o_ptr->feeling == INSCRIP_COATED)) o_ptr->feeling = 0;
								}

								break;
							}
							else
							{
								/* Trap description */
								if (player) msg_format("%^s narrowly misses you.",o_name);
							}

							/* Hack -- to make placing player traps worthwhile, and somewhat exploitable, we only use up arrows / bolts / shots,
							 * and less frequently than if firing these. */
							if (((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT)) && (rand_int(100) < breakage_chance(o_ptr)))
							{
								/* Get local object */
								i_ptr = &object_type_body;
	
								/* Obtain a local object */
								object_copy(i_ptr, o_ptr);
	
								/* Modify quantity */
								i_ptr->number = 1;
	
								/* Decrease the item */
								floor_item_increase(ammo, -1);
	
								/* Nothing left - stop shooting */
								if (!o_ptr->number)
								{
									/* Finished shooting */
									shots = 0;
									path_n = 0;
								}
	
								/* Then optimize the stack */
								floor_item_optimize(ammo);
	
								/* Alter feat if out of ammunition */
								if (!cave_o_idx[y][x]) cave_alter_source_feat(y,x,FS_DISARM);
	
								/* Drop nearby - some chance of breakage */
								/* XXX Put last for safety in case region triggered or broken */
								drop_near(i_ptr,ny,nx,breakage_chance(i_ptr), FALSE);
							}
						}
					}
					else
					{
						/* Disarm */
						cave_alter_source_feat(y,x,FS_DISARM);
					}
				}

				break;
			}

			/* Similar to hitting a regular trap below, but uses up a charge of the object. */
			case TV_SPELL:
			{
				/* Apply terrain */
				apply_terrain = TRUE;

				/* Drop through to use a charge */
			}

			case TV_WAND:
			case TV_STAFF:
			{
				if (o_ptr->charges > 0)
				{
					/* Get item effect */
					get_spell(&power, "use", o_ptr, FALSE);

					/* XXX Hack -- new unstacking code */
					o_ptr->stackc++;

					/* No spare charges */
					if (o_ptr->stackc >= o_ptr->number)
					{
						/* Use a charge off the stack */
						o_ptr->charges--;

						/* Reset the stack count */
						o_ptr->stackc = 0;
					}

					/* XXX Hack -- unstack if necessary */
					if ((o_ptr->number > 1) &&
					((!object_charges_p(o_ptr) && (o_ptr->charges == 2) && (o_ptr->stackc > 1)) ||
					  (!object_charges_p(o_ptr) && (rand_int(o_ptr->number) <= o_ptr->stackc) &&
					  (o_ptr->stackc != 1) && (o_ptr->charges > 2))))
					{
						object_type *i_ptr;
						object_type object_type_body;

						/* Get local object */
						i_ptr = &object_type_body;

						/* Obtain a local object */
						object_copy(i_ptr, o_ptr);

						/* Modify quantity */
						i_ptr->number = 1;

						/* Reset stack counter */
						i_ptr->stackc = 0;

				 		/* Unstack the used item */
				 		o_ptr->number--;

						/* Reduce the charges on the new item */
						if (o_ptr->stackc > 1)
						{
							i_ptr->charges-=2;
							o_ptr->stackc--;
						}
						else if (!o_ptr->stackc)
						{
							i_ptr->charges--;
							o_ptr->charges++;
							o_ptr->stackc = o_ptr->number-1;
						}

						(void)floor_carry(y,x,i_ptr);
					}
				}
				else
				{
					/* Disarm if runs out */
					cave_alter_source_feat(y,x,FS_DISARM);
				}

				break;
			}

			case TV_GLOVES:
			case TV_HELM:
			case TV_RING:
			case TV_AMULET:
			case TV_ROD:
			case TV_DRAG_ARMOR:
			{
				if (!((o_ptr->timeout) && ((!o_ptr->stackc) || (o_ptr->stackc >= o_ptr->number))))
				{
					int tmpval;

					/* Store pval */
					tmpval = o_ptr->timeout;

					/* Time rod out */
					o_ptr->timeout = o_ptr->charges;

					/* Get item effect */
					get_spell(&power, "use", o_ptr, FALSE);

					/* Has a power */
					/* Hack -- check if we are stacking rods */
					if (o_ptr->timeout > 0)
					{
						/* Hack -- one more rod charging */
						if (o_ptr->timeout) o_ptr->stackc++;

						/* Reset stack count */
						if (o_ptr->stackc == o_ptr->number) o_ptr->stackc = 0;

						/* Hack -- always use maximum timeout */
						if (tmpval > o_ptr->timeout) o_ptr->timeout = tmpval;

					}

					/* XXX Hack -- unstack if necessary */
					if ((o_ptr->number > 1) && (o_ptr->timeout > 0))
					{
						object_type *i_ptr;
						object_type object_type_body;

						/* Get local object */
						i_ptr = &object_type_body;

						/* Obtain a local object */
						object_copy(i_ptr, o_ptr);

						/* Modify quantity */
						i_ptr->number = 1;

						/* Clear stack counter */
						i_ptr->stackc = 0;

						/* Restore "charge" */
						o_ptr->timeout = tmpval;

						/* Unstack the used item */
						o_ptr->number--;

						/* Reset the stack if required */
						if (o_ptr->stackc == o_ptr->number) o_ptr->stackc = 0;

						(void)floor_carry(y,x,i_ptr);
					}
				}
				break;
			}

			case TV_SPIKE:
			{
				/* Apply damage directly */
				project_one(SOURCE_PLAYER_TRAP, o_ptr->k_idx, y, x, damroll(6, 6), GF_FALL_SPIKE, (PROJECT_PLAY));

				/* Decrease the item */
				floor_item_increase(cave_o_idx[y][x], -1);
				floor_item_optimize(cave_o_idx[y][x]);

				/* Disarm if runs out */
				if (!cave_o_idx[y][x]) cave_alter_source_feat(y,x,FS_DISARM);

				break;
			}

			case TV_ROPE:
			case TV_STUDY:
			{
				/* Apply terrain */
				apply_terrain = TRUE;

				/* Drop through to use a number */
			}

			case TV_POTION:
			case TV_SCROLL:
			case TV_FLASK:
			case TV_LITE:
			case TV_MUSHROOM:
			{
				/* Get item effect */
				get_spell(&power, "use", o_ptr, FALSE);

				/* Decrease the item */
				floor_item_increase(cave_o_idx[y][x], -1);
				floor_item_optimize(cave_o_idx[y][x]);

				/* Disarm if runs out */
				if (!cave_o_idx[y][x]) cave_alter_source_feat(y,x,FS_DISARM);

				break;
			}
			
			case TV_ASSEMBLY:
			{
				/* Trap description */
				if ((y == p_ptr->py) && (x == p_ptr->px)) msg_print("You hear a mechanism whirring uselessly.");

				break;
			}			

			default:
			{
				/* Trap description */
				if ((y == p_ptr->py) && (x == p_ptr->px)) msg_print("Hmm... there was something under this rock.");

				/* Disarm */
				cave_alter_source_feat(y,x,FS_DISARM);

				break;
			}
		}

		/* Has a power */
		if (power > 0)
		{
			bool dummy;
			int shots = 1 + f_ptr->level / 10;
			int i;
			
			/* Hack - we give out free shots to improve trap effectiveness */
			for (i = 0; i < shots; i++)
			{
				/* Object is used */
				if (k_info[o_ptr->k_idx].used < MAX_SHORT) k_info[o_ptr->k_idx].used++;
				if (k_info[o_ptr->k_idx].ever_used < MAX_SHORT) k_info[o_ptr->k_idx].ever_used++;
	
				/* Cast the spell - note hacky pass of negative damage divisor (-f_ptr->level/10) to multiple damage as well... */
				process_spell_target(SOURCE_PLAYER_TRAP, o_ptr->k_idx, y, x, ty, tx, power, MAX(k_info[o_ptr->k_idx].level, f_ptr->level),
						0 - f_ptr->level / 10, FALSE, TRUE, FALSE, &dummy, NULL);
			}
		}
	}

	/* Regular traps / terrain */
	if (apply_terrain)
	{
		/* Hack - Block murder holes here to use up ammunition */
		bool blocked = (f_ptr->flags1 & (FF1_TRAP)) && (f_ptr->d_attr == TERM_L_RED) && (p_ptr->blocking);

		const char *text = f_text + f_ptr->text;
		
		int i;

		/* Player on source of terrain */
		if ((y == p_ptr->py) && (x == p_ptr->px))
		{
			/* Player floats on terrain */
			if (player_ignore_terrain(feat)) return (FALSE);
		}

		/* Player on destination */
		if ((ty == p_ptr->py) && (tx == p_ptr->px))
		{
			/* Blocked message */
			if (blocked) msg_print("You knock aside the arrow.");

			/* Notice otherwise */
			else if (strlen(text)) msg_format("%s",text);
		}

		/* XXX If we have not already set up the child region for a region based trap,
		 * do so now. We do this to allow the player to walk onto the trap source to create such
		 * a region. */
		if (!child_region) for (i = 0; i < region_max; i++)
		{
			/* Get the region */
			region_type *r_ptr = &region_list[i];

			/* Delete traps */
			if (((r_ptr->flags1 & (RE1_HIT_TRAP)) != 0) && (r_ptr->y0 == y) && (r_ptr->x0 == x) && (r_ptr->child_region))
			{
				/* Get a newly initialized region */
				child_region = init_region(r_ptr->who, r_ptr->what, r_ptr->child_region, r_ptr->damage, r_ptr->method, r_ptr->effect, r_ptr->level, r_ptr->y0, r_ptr->x0, ty, tx);
				
				/* Hack -- lifespan */
				region_list[child_region].lifespan = scale_method(region_info[r_ptr->type].child_lasts, r_ptr->level);
				
				/* Hack -- notice the region */
				if (r_ptr->effect != GF_FEATURE) region_list[child_region].flags1 |= (RE1_DISPLAY);
				region_list[child_region].flags1 |= (RE1_NOTICE);
				region_refresh(child_region);
				break;
			}
		}

		/* Blocked the attack - no effect */
		if (blocked)
		{
			/* Do nothing */
		}
		/* Apply spell effect */
		else if ((f_ptr->spell) && (!child_region))
		{
			/* Regular spell effect */
			obvious |= make_attack_ranged(SOURCE_FEATURE,f_ptr->spell,ty,tx);
		}
		/* Apply blow effect and/or create child region */
		else if ((f_ptr->blow.method) || (child_region))
		{
			region_type *r_ptr = &region_list[child_region];
			feature_blow *blow_ptr = &f_ptr->blow;
			int method = f_ptr->spell ? f_ptr->spell : blow_ptr->method;
			method_type *method_ptr = &method_info[method];
			int effect = f_ptr->spell ? method_ptr->d_res : blow_ptr->effect;

			int num = scale_method(method_ptr->number, p_ptr->depth);
			int i, j;

			/* Hack -- minimum number */
			if (!num) num = 1;

			/* Create the region */
			for (i = 0; i < num; i++)
			{
				/* Get damage for breath weapons */
				if (method_ptr->flags2 & (PR2_BREATH))
				{
					dam = get_breath_dam(p_ptr->depth * 10, method, p_ptr->depth > 50);
				}
				/* Get damage for regular spells */
				else if (f_ptr->spell)
				{
					dam = get_dam(2 + p_ptr->depth / 2, f_ptr->spell, TRUE);
				}
				else
				{
					/* Get the damage */
					dam = damroll(blow_ptr->d_side,blow_ptr->d_dice);
				}

				/* Set child region values */
				if (child_region)
				{
					r_ptr->method = method;
					r_ptr->effect = effect;
					r_ptr->damage = dam;
				}
				
				/* Apply the blow */
				obvious |= project_method(SOURCE_FEATURE, feat, method, effect, dam, p_ptr->depth, y, x, ty, tx, child_region, method_ptr->flags1);
				
				/* Check if region random */
				for (j = 0; j < region_max; j++)
				{
					/* Get the region */
					region_type *r_ptr = &region_list[j];
	
					/* Region random? */
					if (((r_ptr->flags1 & (RE1_HIT_TRAP)) != 0) && (r_ptr->y0 == y) && (r_ptr->x0 == x) && (r_ptr->flags1 & (RE1_RANDOM)))
					{
						int r = region_random_piece(j);

						ty = region_piece_list[r].y;
						tx = region_piece_list[r].x;
						
						break;
					}
				}
			}
		}

		/* Re-get original feature */
		f_ptr = &f_info[cave_feat[y][x]];

		/* Hit the trap */
		if (f_ptr->flags1 & (FF1_HIT_TRAP))
		{
			/* Modify the location hit by the trap */
			cave_alter_source_feat(y,x,FS_HIT_TRAP);
		}
		else if (f_ptr->flags1 & (FF1_SECRET))
		{
			/* Discover */
			cave_alter_source_feat(y,x,FS_SECRET);
		}
	}

	return (obvious);
}


/*
 * Handle player hitting a real trap
 */
void hit_trap(int y, int x)
{
	feature_type *f_ptr;

	int feat = cave_feat[y][x];

	/* Get feature */
	f_ptr = &f_info[feat];

	/* Avoid trap */
	if ((f_ptr->flags1 & (FF1_TRAP)) && (avoid_trap(y, x, feat))) return;

	/* Hack -- fall onto trap if we can move */
	if ((f_ptr->flags1 & (FF1_MOVE)) && ((p_ptr->py != y) || (p_ptr->px !=x)))
	{
		/* Move player */
		monster_swap(p_ptr->py, p_ptr->px, y, x);
	}

	/* Disturb the player */
	disturb(0, 0);

	/* Discharge the trap */
	discharge_trap(y, x, y, x, 0);
}


/*
 *   Get style benefits against a monster, using permitted styles.
 */
void mon_style_benefits(const monster_type *m_ptr, u32b style, int *to_hit, int *to_dam, int *to_crit)
{
	monster_race *r_ptr = NULL;
	int i;

	/* Reset style benefits */
	*to_hit = 0;
	*to_dam = 0;
	*to_crit = 0;

  if (m_ptr) /* A hack for display of bonuses */
  {
	r_ptr = &r_info[m_ptr->r_idx];

	/* Now record in style what styles we can potentially exercise here.
	   Leter we will check if we actually have picked one of those at birth. */

	/* Check backstab if monster sleeping or fleeing */
	if ( (m_ptr->csleep || m_ptr->monfear)
		 && style & (1L<<WS_SWORD) /* works only for melee */
		 && inventory[INVEN_WIELD].weight < 100 )
		style |= (1L <<WS_BACKSTAB);

	/* Check slay orc if monster is an orc; works also for ranged attacks */
	if (r_ptr->flags3 & (RF3_ORC)) style |= (1L <<WS_SLAY_ORC);

	/* Check slay troll if monster is a troll */
	if (r_ptr->flags3 & (RF3_TROLL)) style |= (1L <<WS_SLAY_TROLL);

	/* Check slay giant if monster is a giant */
	if (r_ptr->flags3 & (RF3_GIANT)) style |= (1L <<WS_SLAY_GIANT);

	/* Check slay dragon if monster is a dragon */
	if (r_ptr->flags3 & (RF3_DRAGON)) style |= (1L <<WS_SLAY_DRAGON);

	/* Check slay evil if monster is evil */
	if (r_ptr->flags3 & (RF3_EVIL)) style |= (1L <<WS_SLAY_EVIL);

	/* Check slay giant if monster is undead */
	if (r_ptr->flags3 & (RF3_UNDEAD)) style |= (1L <<WS_SLAY_UNDEAD);

	/* Check slay giant if monster is an animal */
	if (r_ptr->flags3 & (RF3_ANIMAL)) style |= (1L <<WS_SLAY_ANIMAL);

	/* Check slay giant if monster is a demon */
	if (r_ptr->flags3 & (RF3_DEMON)) style |= (1L <<WS_SLAY_DEMON);
  }

	/*** Handle styles ***/
	for (i = 0;i< z_info->w_max;i++)
	{
		if (w_info[i].class != p_ptr->pclass) continue;

		if (w_info[i].level > p_ptr->lev) continue;

		/* Check for styles */
		if ((w_info[i].styles==0) || (w_info[i].styles & (style & (1L << p_ptr->pstyle))))
		{
			switch (w_info[i].benefit)
			{
				case WB_HIT:
					*to_hit += (p_ptr->lev - w_info[i].level) /2;
					break;

				case WB_DAM:
					*to_dam += (p_ptr->lev - w_info[i].level) /2;
					break;

				case WB_CRITICAL:
					*to_crit += 1;
					break;
			}
		}
	}

	/* Hack - backstab always of some benefit */
	if (style & (WS_BACKSTAB)) *to_crit += (adult_gollum ? 3 : 1);
	
	/* Only allow criticals against living opponents */
	if (r_ptr && (r_ptr->flags3 & (RF3_NONLIVING)
				  || r_ptr->flags2 & (RF2_STUPID)))
	{
		*to_crit = 0;
	}

	/* Only allow criticals against visible opponents */
	if (m_ptr && !(m_ptr->ml))
	{
		*to_crit = 0;
	}
}


/*
 * Get the weapon slot based on the melee style and number of blows so far.
 */
static int weapon_slot(u32b melee_style, int blows, bool charging)
{
	int slot = INVEN_WIELD;

	/* Get secondary weapon instead */
	if (!(blows % 2) && (melee_style & (1L << WS_TWO_WEAPON))) slot = INVEN_ARM;

	/* Get the unarmed weapon */
	if (melee_style & (1L << WS_UNARMED))
	{
		/* Use feet while charging */
		if (charging && blows == 1) slot = INVEN_FEET;

		/* Alternate hands and feet */
		else if (!(blows % 3)) slot = INVEN_FEET;

		/* Use hands */
		else if (inventory[INVEN_HANDS].k_idx) slot = INVEN_HANDS;

		/* Use rings */
		else if (blows % 3 == 1) slot = INVEN_RIGHT;

		/* Use rings */
		else slot = INVEN_LEFT;
	}

	return(slot);
}





/*
 * Determine if the object has a "=A" in its inscription.
 *
 * This is used to automatically activate the object when attacking, throwing or firing it.
 */
bool auto_activate(const object_type *o_ptr)
{
	u32b f1, f2, f3, f4;

	cptr s;

	/* Check timeout */
	if (o_ptr->timeout) return (FALSE);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Auto-activate on blow */
	if ((f3 & (TR3_ACT_ON_BLOW)) != 0) return (TRUE);

	/* Check if can activate */
	if ((f3 & (TR3_ACTIVATE | TR3_UNCONTROLLED)) == 0) return (FALSE);

	/* Check if uncontrolled */
	if ((f3 & (TR3_UNCONTROLLED)) && (uncontrolled_p(o_ptr))) return (TRUE);

	/* No inscription */
	if (!o_ptr->note) return (FALSE);

	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process inscription */
	while (s)
	{
		/* Automatically activate on "=A" */
		if (s[1] == 'A')
		{
			/* Can activate */
			return (TRUE);
		}

		/* Find another '=' */
		s = strchr(s + 1, '=');
	}

	/* Can activate */
	return (FALSE);
}



/*
 * Attack the monster at the given location
 *
 * If no "weapon" is available, then "punch" the monster one time.
 */
void py_attack(int dir)
{
	int y = p_ptr->py + ddy[dir];
	int x = p_ptr->px + ddx[dir];

	int k, bonus, chance;

	/* Style bonuses */
	int style_hit = 0;
	int style_dam = 0;
	int style_crit = 0;
	u32b melee_style;

	int blows = 0; /* Number of blows actually delivered */
	int slot;

	int i, c, crit;
	int num_blows = 0;
	int do_cuts = 0;
	int do_stun = 0;
	int do_trip = 0;

	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_lore *l_ptr;

	object_type *o_ptr;

	char m_name[80];

	bool fear = FALSE;

	bool was_asleep;

	cptr p;

	u32b k1[3], k2[3], k3[3], k4[3];

	u32b n1[3], n2[3], n3[3], n4[3];

	bool charging = FALSE;

	if (cave_m_idx[y][x] <= 0)
	{
		/* Oops */
		msg_print("You spin around.");
		return;
	}

	/* If moving, you can charge in the direction */
	if (p_ptr->num_charge > 1
		&& (p_ptr->charging == dir
			|| side_dirs[dir][1] == p_ptr->charging
			|| side_dirs[dir][2] == p_ptr->charging))
		charging = TRUE;

	/* Get the monster */
	m_ptr = &m_list[cave_m_idx[y][x]];
	r_ptr = &r_info[m_ptr->r_idx];
	l_ptr = &l_list[m_ptr->r_idx];

	/* Disturb the player */
	disturb(0, 0);

	/* Extract monster name (or "it") */
	monster_desc(m_name, sizeof(m_name), cave_m_idx[y][x], 0);

	/* Auto-Recall if possible and visible */
	if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

	/* Track a new monster */
	if (m_ptr->ml) health_track(cave_m_idx[y][x]);

	/* Some monsters radiate damage when attacked */
	if (r_ptr->flags2 & (RF2_HAS_AURA))
	{
		(void)make_attack_ranged(cave_m_idx[y][x],96+7,p_ptr->py,p_ptr->px);
	}

	/* Handle player fear */
	if (p_ptr->timed[TMD_AFRAID])
	{
		/* Message */
		msg_format("You are too afraid to attack %s!", m_name);

		/* Done */
		return;
	}

	/* Check melee styles only */
	melee_style = p_ptr->cur_style & (WS_WIELD_FLAGS);

	/* Get style benefits */
	mon_style_benefits(m_ptr, melee_style, &style_hit, &style_dam, &style_crit);

	/* Check if monster asleep */
	was_asleep = (m_ptr->csleep == 0);

	/* Disturb the monster */
	m_ptr->csleep = 0;

	/* Mark the monster as attacked by melee */
	m_ptr->mflag |= (MFLAG_HIT_BLOW);

	/* Get number of blows */
	num_blows = p_ptr->num_blow;

	/*
	 * Hack - Dodge in direction of attack.
	 * This helps protect the player whilst in melee, from other ranged attacks.
	 */
	p_ptr->dodging = dir;

	/* Player is sneaking */
	if (p_ptr->sneaking)
	{
		/* If the player is trying to be stealthy, they lose the benefit of this for
		 * the following turn.
		 */
		p_ptr->not_sneaking = TRUE;
	}

	/* Restrict blows if charging */
	if (charging)
	{
		/* Charging decreases number of blows */
		num_blows = (num_blows + 1) / 2;

		/* Ensure at least one blow with each weapon */
		if ((melee_style & (1L << WS_TWO_WEAPON)) && (num_blows > 0))
		  num_blows = MAX(2, num_blows);
	}

	/* Hack -- no blows */
	if (num_blows <= 0)
	{
/*		msg_print("You lack the skill to attack.");

		return; */

		/* Always get one blow */
		num_blows = 1;
	}

	/* Attack once for each legal blow */
	while (blows++ < num_blows)
	{
		/* Paranoia - ensure monster still exists */
		if (cave_m_idx[y][x] <= 0) break;

		/* Get weapon slot */
		slot = weapon_slot(melee_style,blows,charging);

		/* Weapon slot */
		o_ptr = &inventory[slot];

		/* Record the item flags */
		if (o_ptr->k_idx)
		{
			/* Get item first time it is used */
			if ((blows <= 2) && ((blows == 1)
				|| (melee_style & ((1L << WS_UNARMED) | (1L << WS_TWO_WEAPON))) ))
			{
				k1[blows] = o_ptr->can_flags1;
				k2[blows] = o_ptr->can_flags2;
				k3[blows] = o_ptr->can_flags3;
				k4[blows] = o_ptr->can_flags4;
			}
		}

		/* Some monsters are great at dodging  -EZ- */
		if (mon_evade(cave_m_idx[y][x], (m_ptr->stunned || m_ptr->confused) ? 50 : 80, 100, " your blow")) continue;

		/* Calculate the "attack quality" */
		if (o_ptr->k_idx) bonus = p_ptr->to_h + o_ptr->to_h + style_hit;
		else bonus = p_ptr->to_h + style_hit;

		/*
		 * Blocking - the player gets a big to-hit bonus on the riposte
		 */
		if (p_ptr->blocking) bonus += 20;

		chance = (p_ptr->skills[SKILL_TO_HIT_MELEE] + (bonus * BTH_PLUS_ADJ));
		
		/* We are attacking - count attacks */
		if ((l_ptr->tblows < MAX_SHORT) && (m_ptr->ml)) l_ptr->tblows++;
		
		/* Test for hit */
		if (!test_hit_norm(chance, calc_monster_ac(cave_m_idx[y][x], FALSE), m_ptr->ml))
		{
			/* Message */
			message_format(MSG_MISS, m_ptr->r_idx, "You miss %s.", m_name);
		}
		/* Test for resistance */
		else if (o_ptr->k_idx && mon_resist_object(cave_m_idx[y][x], o_ptr))
		{
			/* No need for message */
		}
		/* Test for huge monsters -- they resist non-charging attacks */
		else if ((r_ptr->flags3 & (RF3_HUGE)) && !(charging && blows == 1)
				 /* Modify chance to hit using the characters size */
				 && rand_int(adj_mag_mana[p_ptr->stat_ind[A_SIZ]]) < (20 + p_ptr->depth)
				 /* Easy climb locations provide enough of a boost to attack from on high */
				 && (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags3 & (FF3_EASY_CLIMB)) == 0)
		{
			/* Message */
			message_format(MSG_MISS, m_ptr->r_idx, "You cannot reach %s.", m_name);

			/* Miss */
		}
		/* Player hits */
		else
		{
			bool fumble = FALSE;

			/* Test for fumble */
			if (((p_ptr->heavy_wield)
				  || (o_ptr->k_idx && o_ptr->ident & (IDENT_CURSED)))
				 && (!rand_int(chance > 1 ? chance : 2)))
			{
				/* Hack -- use the player for the attack */
				my_strcpy(m_name, "yourself", sizeof(m_name));

				/* Hack -- use the 'fake' player */
				r_ptr = &r_info[0];

				/* Hack -- target self */
				y = p_ptr->py;
				x = p_ptr->px;

				/* Notice fumbling later */
				fumble = TRUE;
			}

			/* Handle normal weapon/gauntlets/boots */
			if (o_ptr->k_idx)
			{
				int mult = object_damage_multiplier(o_ptr, m_ptr, FALSE);

				k = damroll(o_ptr->dd, o_ptr->ds);

				/* Allow other items on hands to assist with melee blow multipliers */
				if (slot != INVEN_FEET)
				{
					/* Use gauntlet brand. Gauntlets use 1 less multiplier */
					if (inventory[INVEN_HANDS].k_idx)
					{
						mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_HANDS], m_ptr, FALSE) - 1);
					}

					/* Use ring brands on right hand for main weapon. Rings use 1 less multiplier. */
					if ((inventory[INVEN_RIGHT].k_idx) && ((slot == INVEN_WIELD) || (melee_style & (1L << WS_TWO_HANDED))))
					{
						mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_HANDS], m_ptr, FALSE) - 1);
					}

					/* Use ring brands on left hand for off-hand weapon. Rings use 1 less multiplier. */
					if ((inventory[INVEN_LEFT].k_idx) && ((slot == INVEN_ARM) || (melee_style & (1L << WS_TWO_HANDED))))
					{
						mult = MAX(mult, object_damage_multiplier(&inventory[INVEN_HANDS], m_ptr, FALSE) - 1);
					}
				}

				/* Get the total damage from the multiplier */
				k = tot_dam_mult(k, mult);

				/* Haven't got an critical type yet */
				c = 0;
				crit = 0L;

				/* Choose critical type */
				for (i = TR5_DO_CUTS; i <= TR5_DO_TRIP; i <<= 1)
				{
					/* Weapon doesn't have this critical type */
					if (!(k_info[o_ptr->k_idx].flags5 & i)) continue;

					if (!rand_int(++c)) crit = i;
				}

				/* Get critical effect */
				i = critical_norm(o_ptr->weight, bonus + (style_crit * 30), k);

				/* Apply critical */
				switch (crit)
				{
					case TR5_DO_CUTS:
					{
						do_cuts = i;
						break;
					}
					case TR5_DO_STUN:
					{
						do_stun = i;
						break;
					}
					case TR5_DO_CRIT:
					{
						k += i;
						break;
					}
					case TR5_DO_TRIP:
					{
						if (i) do_trip = 60 / num_blows;

						/* XXX 1 energy is equivalent to one damage. */
						if (do_trip > i) do_trip = i;
						else k += i - do_trip;
						break;
					}
					default:
					{
						/* Weapon never does criticals */
						break;
					}
				}

				/* Add damage bonus */
				k += o_ptr->to_d;

				/* Check usage */
				object_usage(slot);

				/* Adjust for equipment/stats to_d bounded by dice */
				k += MIN(p_ptr->to_d,
							o_ptr->dd * o_ptr->ds + 5);
			}
			/* Handle bare-hand/bare-foot attack */
			else
			{
				k = 1;
				do_stun = critical_norm(c_info[p_ptr->pclass].min_weight, (style_crit * 30), k);
			}

			/* Adjust for style */
			k += style_dam;

			/* Adjust for charging - for first attack only */
			if (charging && blows == 1)
			  k *= p_ptr->num_charge;

			/* Monster armor reduces total damage */
			k -= (k * ((r_ptr->ac < 150) ? r_ptr->ac : 150) / 250);

			/* No negative damage */
			if (k < 0) k = 0;

			/* Some monsters resist stun */
			if (r_ptr->flags3 & (RF3_NO_STUN)) do_stun = 0;

			/* Some monsters resist cuts */
			if (r_ptr->flags9 & (RF9_NO_CUTS)) do_cuts = 0;

			/* TODO: Some monsters resist tripping */

			/* Hack --- backstab. Weapon of 10 lbs or less */
			if (melee_style & (1L << WS_BACKSTAB))
			{
				/* Message */
				p = "You backstab %s!";
			}
			else if (do_stun)
			{
				/* Message */
				p = "You batter %s!";
			}
			else if (do_cuts)
			{
				/* Message */
				p = "You wound %s!";
			}
			else if (do_trip)
			{
				/* Message */
				p = "You slam %s!";
			}
			else if (charging && blows == 1)
			{
				/* Unarmed */
				if (melee_style & (1L << WS_UNARMED)) p = "You flying kick %s!";

				/* Message */
				else p = "You charge %s!";
			}
			else
			{
				/* Unarmed punch */
				if (melee_style & (1L << WS_UNARMED))
				{
					if (blows % 2) p = "You punch %s.";
					else p = "You kick %s.";
				}

				/* Message */
				else p = "You hit %s.";
			}

			/* Message */
			message_format(MSG_HIT, m_ptr->r_idx, p, m_name);

			/* Complex message */
			if (p_ptr->wizard)
			{
				msg_format("You do %d (out of %d) damage.", k, m_ptr->hp);
			}

			/* Apply stun effect */
			if (do_stun)
			{
				/* Hitting self */
				if (fumble)
				{
					/* Fumble stun */
					(void)set_stun(p_ptr->timed[TMD_STUN] + do_stun);
				}
				/* Avoid overflow */
				else if ((do_stun + m_ptr->stunned) / (r_ptr->level / 10 + 1) > 875)
				{
					k+= 4 * ((do_stun + m_ptr->stunned) / (r_ptr->level / 10 + 1) - 100) / 5;
					m_ptr->stunned = 255;
				}

				/* Convert some stun damage to damage */
				else if ((do_stun + m_ptr->stunned) / (r_ptr->level / 10 + 1) > 100)
				{
					k+= 4 * ((do_stun + m_ptr->stunned) / (r_ptr->level / 10 + 1) - 100) / 5;
					m_ptr->stunned = (100 + (do_stun + m_ptr->stunned - 100) / 5) ;
				}
				else m_ptr->stunned += do_stun / (r_ptr->level / 10 + 1);
			}

			/* Apply cuts effect */
			if (do_cuts)
			{
				/* Hitting self */
				if (fumble)
				{
					/* Fumble cuts */
					(void)set_cut(p_ptr->timed[TMD_CUT] + do_cuts);
				}
				else if ((m_ptr->cut + do_cuts) / (r_ptr->level / 10 + 1) > 255)
				{
					k+= ((m_ptr->cut + do_cuts) / (r_ptr->level / 10 + 1)) - 255;
					m_ptr->cut = 255;
				}
				else m_ptr->cut += do_cuts / (r_ptr->level / 10 + 1);
			}

			/* Trip - rob monster of energy */
			if (do_trip)
			{
				if (fumble)
				{
					/* Fumble trip - finish attacking immediately */
					blows = num_blows;
				}
				else
				{
					/* Adjust energy */
					int energy = m_ptr->energy - do_trip;

					/* Set bounds */
					/*if (energy > 250) energy = 250;*/
					if (energy <   0) energy =   0;

					/* Apply to monster */
					m_ptr->energy = (byte)energy;
				}
			}

			/* Fumble - damage self */
			if (fumble)
			{
				project_one(SOURCE_PLAYER_ATTACK, o_ptr->k_idx, y, x, k, GF_HURT, (PROJECT_PLAY));
			}

			/* Damage, check for fear and death */
			else if (o_ptr->k_idx
						&& mon_take_hit(cave_m_idx[y][x], k, &fear, NULL))
			{
				u32b f1, f2, f3, f4;

				/* Extract the flags */
				object_flags(o_ptr, &f1, &f2, &f3, &f4);

				/* We've killed it - Suck blood from the corpse */
				if (((f4 & (TR4_VAMP_HP)) || (p_ptr->branded_blows == 101)) & (r_ptr->flags8 & (RF8_HAS_BLOOD)))
				{
					hp_player(r_ptr->level);

					if (p_ptr->branded_blows != 101) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_VAMP_HP, FALSE);
					l_ptr->flags8 |= (RF8_HAS_BLOOD);
				}
				else if (l_ptr->flags8 & (RF8_HAS_BLOOD))
				{
					object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_VAMP_HP, FALSE);
				}

				/* We've killed it - Drain mana from the corpse*/
				if (((f4 & (TR4_VAMP_MANA)) || (p_ptr->branded_blows == 102)) & (r_ptr->mana > 0))
				{
					if (p_ptr->csp < p_ptr->msp)
					{
						p_ptr->csp = p_ptr->csp + r_ptr->mana;
						if (p_ptr->csp >= p_ptr->msp)
						{
							p_ptr->csp = p_ptr->msp;
							p_ptr->csp_frac = 0;
						}
						p_ptr->redraw |= (PR_MANA);
						p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
					}

					if (p_ptr->branded_blows != 102) object_can_flags(o_ptr,0x0L,0x0L,0x0L,TR4_VAMP_MANA, FALSE);
				}
				else if (r_ptr->mana > 0)
				{
					object_not_flags(o_ptr,0x0L,0x0L,0x0L,TR4_VAMP_MANA, FALSE);
				}

				/* Hack -- cancel wakeup call */
				was_asleep = FALSE;

				break;
			}

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
				/* Sometimes coating affects source player */
				if (!rand_int(chance > 1 ? chance : 2))
				{
					y = p_ptr->py;
					x = p_ptr->px;

					fumble = TRUE;
				}

				/* Make item strike */
				process_item_blow(SOURCE_PLAYER_COATING, lookup_kind(o_ptr->xtra1, o_ptr->xtra2), o_ptr, y, x, TRUE, TRUE);
			}

			/* Fumbling or coating backfiring */
			if (fumble) break;
		}
	}

	/* Hack -- wake up nearby allies */
	if (was_asleep)
	{
		/* Cutting, stunning, poisoning or dazing  a monster shuts them up */
		if ((m_ptr->cut < 20) && (m_ptr->stunned < 20)&& (m_ptr->poisoned < 20) && (m_ptr->dazed < 20)) tell_allies_not_mflag(m_ptr->fy, m_ptr->fx, (MFLAG_TOWN), "& has attacked me!");
	}

	/* Hack -- delay fear messages */
	if (fear && m_ptr->ml)
	{
		/* Message */
		message_format(MSG_FLEE, cave_m_idx[m_ptr->fy][m_ptr->fx], "%^s flees in terror!", m_name);
	}

	/* Charging uses full turn */
	if (charging) p_ptr->energy_use = 100;

	/* Only use required energy to kill monster */
	else p_ptr->energy_use = 100 * blows / p_ptr->num_blow;

	/* Check and display any changed weapon flags */
	for (i = 1; (i <= blows) && (i <= ((melee_style & ((1L << WS_UNARMED) | (1L << WS_TWO_WEAPON))) ? 2 : 1)); i++)
	{
		slot = weapon_slot(melee_style, i, charging);

		o_ptr = &inventory[slot];

		/* Check for new flags */
		n1[i] = (o_ptr->can_flags1 & ~(k1[i])) & (TR1_WEAPON_FLAGS);
		n2[i] = (o_ptr->can_flags2 & ~(k2[i])) & (TR2_WEAPON_FLAGS);
		n3[i] = (o_ptr->can_flags3 & ~(k3[i])) & (TR3_WEAPON_FLAGS);
		n4[i] = (o_ptr->can_flags4 & ~(k4[i])) & (TR4_WEAPON_FLAGS);

		if (n1[i] || n2[i] || n3[i] || n4[i]) update_slot_flags(slot, n1[i], n2[i], n3[i], n4[i]);
	}
}


/*
 * Player is stuck inside terrain.
 *
 * This routine should only be called when energy has been expended.
 *
 */
bool stuck_player(int *dir)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	feature_type *f_ptr = &f_info[cave_feat[py][px]];

	/* Hack -- allowed to move nowhere */
	if ((!*dir) || (*dir == 5)) return (FALSE);

	/* Player can not walk through "walls" */
	if (!(f_ptr->flags1 & (FF1_MOVE))
	&& !(f_ptr->flags3 & (FF3_EASY_CLIMB)))
	{
		/* Disturb the player */
		disturb(0, 0);

		/* Notice unknown obstacles */
		if (!(play_info[py][px] & (PLAY_MARK)))
		{

			/* Get hit by terrain/traps */
			if (((f_ptr->flags1 & (FF1_HIT_TRAP)) && !(f_ptr->flags3 & (FF3_CHEST))) ||
				(f_ptr->spell) || (f_ptr->blow.method))
			{

				/* Hit the trap */
				hit_trap(py, px);
			}

			play_info[py][px] |= (PLAY_MARK);

			lite_spot(py, px);

		}

		/* Always make direction 0 */
		*dir = 0;

		return (TRUE);

	}

	return (FALSE);

}


/*
 * Move player in the given direction.
 *
 * This routine should only be called when energy has been expended.
 *
 * Note that this routine handles monsters in the destination grid,
 * and also handles attempting to move into walls/doors/rubble/etc.
 */
void move_player(int dir)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	feature_type *f_ptr;

	int y, x;

	int mimic;

	cptr name;

	/* Move is a climb? -- force boolean */
	bool climb = FALSE;

	/* Find the result of moving */
	y = py + ddy[dir];
	x = px + ddx[dir];

	f_ptr = &f_info[cave_feat[y][x]];

	climb = ((!(f_ptr->flags1 & (FF1_MOVE))
		&& (f_ptr->flags3 & (FF3_EASY_CLIMB)))
		|| (!(f_ptr->flags3 & (FF3_MUST_CLIMB))
		&& (f_info[cave_feat[py][px]].flags3 & (FF3_MUST_CLIMB)))) != 0;

	/* Hack -- pickup objects from locations you can't move to but can see */
	if (((cave_o_idx[y][x]) || (f_ptr->flags3 & (FF3_GET_FEAT))) && !(f_ptr->flags1 & (FF1_MOVE)) && !(f_ptr->flags3 & (FF3_EASY_CLIMB)) && (play_info[y][x] & (PLAY_MARK)))
	{
		/* Get item from the destination */
		p_ptr->energy_use = py_pickup(y, x, 1) * 10;

		/* Disturb the player */
		disturb(0, 0);

		return;
	}

	/* Hack -- attack monsters --- except hidden ones, allies or townsfolk */
	if ((cave_m_idx[y][x] > 0) && (choose_to_attack_player(&m_list[cave_m_idx[y][x]])) &&

	 /* Note hack to also ignore monsters on town level who don't do damage. */
		(((level_flag & (LF1_TOWN)) == 0) || (r_info[m_list[cave_m_idx[y][x]].r_idx].blow[0].effect != GF_NOTHING)) &&

		 /* Allow the player to run over most monsters -- except those that can't move */
		 (!(p_ptr->running) || (r_info[m_list[cave_m_idx[y][x]].r_idx].flags1 & (RF1_NEVER_MOVE))))
	{
		/* Attack */
		py_attack(dir);
	}

	else if (stuck_player(&dir))
	{
		/* Do nothing */
	}

	/* Optionally alter known traps/doors on movement */
	else if (easy_alter &&
	 (play_info[y][x] & (PLAY_MARK)) &&
		 ( (f_ptr->flags1 & (FF1_DISARM)) ||
		 ( !(f_ptr->flags1 & (FF1_MOVE)) &&
		 !(f_ptr->flags3 & (FF3_EASY_CLIMB)) &&
		 (f_ptr->flags1 & (FF1_OPEN)))))
	{
		/* Not already repeating */
		if (!p_ptr->command_rep && p_ptr->command_arg <= 0)
		{
			/* Repeat 99 times */
			p_ptr->command_rep = 99;

			/* Reset the command count */
			p_ptr->command_arg = 0;
		}

		/* Alter */
		do_cmd_alter();
	}

	/* Petrified */
	else if (p_ptr->timed[TMD_PETRIFY])
	{
		/* Disturb the player */
		disturb(0, 0);

		msg_print("You are too petrified to move.");
		return;
	}

	/* Player can not walk through "walls" */
	/* Also cannot climb over unknown "trees/rubble" */
	else if (!(f_ptr->flags1 & (FF1_MOVE))
				&& (!(f_ptr->flags3 & (FF3_EASY_CLIMB))
					 || !(play_info[y][x] & (PLAY_MARK))))
	{
		if (disturb_detect && (play_info[p_ptr->py][p_ptr->px] & (PLAY_SAFE)) && !(play_info[y][x] & (PLAY_SAFE)))
		{
			disturb(1,0);
			msg_print("This doesn't feel safe.");

			if (!get_check("Are you sure you want to enter undetected territory?")) return;
		}

		/* Disturb the player */
		disturb(0, 0);

		/* Notice unknown obstacles */
		if (!(play_info[y][x] & (PLAY_MARK)))
		{
			/* Get hit by regions */
			if (cave_region_piece[y][x])
			{
				/* Trigger the region */
				trigger_region(y, x, TRUE);
			}

			/* Get hit by terrain/traps */
			if (((f_ptr->flags1 & (FF1_HIT_TRAP)) && !(f_ptr->flags3 & (FF3_CHEST))) ||
				(f_ptr->spell) || (f_ptr->blow.method))
			{
				/* Hit the trap */
				hit_trap(y, x);
			}

			/* Get the mimiced feature */
			mimic = f_ptr->mimic;

			/* Get the feature name */
			name = (f_name + f_info[mimic].name);

			/* Tell the player */
			msg_format("You feel %s%s blocking your way.",
				((f_ptr->flags2 & (FF2_FILLED)) ? "" :
					(is_a_vowel(name[0]) ? "an " : "a ")), name);

			play_info[y][x] |= (PLAY_MARK);

			lite_spot(y, x);
		}

		/* Mention known obstacles */
		else
		{
			/* Get the mimicked feature */
			mimic = f_ptr->mimic;

			/* Get the feature name */
			name = (f_name + f_info[mimic].name);

			/* Tell the player */
			msg_format("There is %s%s blocking your way.",
				((f_ptr->flags2 & (FF2_FILLED)) ? "" :
				(is_a_vowel(name[0]) ? "an " : "a ")),name);

			/* If the mimiced feature appears movable, reveal secret */
			if (((((f_info[mimic].flags1 & (FF1_MOVE)) != 0) ||
				((f_info[mimic].flags3 & (FF3_EASY_CLIMB)) != 0))
				&& ((f_ptr->flags1 & (FF1_SECRET)) != 0)))
			{
				find_secret(y, x);
			}
		}
	}

	/* Partial movement */
	else if ((climb) && (dir != p_ptr->climbing))
	{
		if (disturb_detect && (play_info[p_ptr->py][p_ptr->px] & (PLAY_SAFE)) && !(play_info[y][x] & (PLAY_SAFE)))
		{
			disturb(1,0);
			msg_print("This doesn't feel safe.");

			if (!get_check("Are you sure you want to enter undetected territory?")) return;
		}

		/* Get the mimiced feature */
		mimic = f_ptr->mimic;

		/* Use existing feature if not easy climb */
		if (!(f_ptr->flags3 & (FF3_EASY_CLIMB))) mimic = f_info[cave_feat[py][px]].mimic;

		/* Get the feature name */
		name = (f_name + f_info[mimic].name);

		/* Tell the player */
		msg_format("You climb %s%s%s.",
				((f_ptr->flags3 & (FF3_EASY_CLIMB)) ? "" : "out of "),
				((f_ptr->flags2 & (FF2_FILLED)) ? "" : "the "), name);

		p_ptr->climbing = dir;

		/* Automate 2nd movement command */
		p_ptr->command_cmd = 59;
		p_ptr->command_rep = 1;
		p_ptr->command_dir = dir;
  	}

	/* Normal movement */
	else
	{
		if ((disturb_detect) && (play_info[p_ptr->py][p_ptr->px] & (PLAY_SAFE)) && !(play_info[y][x] & (PLAY_SAFE)))
		{
			disturb(1,0);
			msg_print("This doesn't feel safe.");

			if (!get_check("Are you sure you want to enter undetected territory?")) return;
		}

		/* Sound XXX XXX XXX */
		/* sound(MSG_WALK); */

		/* Players can push aside allies or run over moving monsters */
		/* This would normally be the only situation where the target grid still
		 * contains a monster */
		if (cave_m_idx[y][x] > 0)
		{
			monster_type *n_ptr = &m_list[cave_m_idx[y][x]];

			/* Push monster if it doesn't have a target and hasn't been pushed.
			 * This allows the player to move into a corridor with a monster in
			 * front of him, and have the monster move ahead, if it is faster. If its
			 * not faster, the player will push over it on the second move, as the push
			 * flag below will have been set. */
			if (((n_ptr->mflag & (MFLAG_PUSH)) == 0)  && !(n_ptr->ty) && !(n_ptr->tx)
					&& push_aside(p_ptr->py, p_ptr->px, n_ptr))
			{
				int dy = n_ptr->fy - y;
				int dx = n_ptr->fx - x;
				int count = 0;

				n_ptr->ty = n_ptr->fy;
				n_ptr->tx = n_ptr->fx;

				/* Hack -- get new target as far as the monster can move in the direction
				 * pushed. We do this with a walking stick approach to prevent us getting
				 * invalid target locations like (0,0) */
				while (in_bounds_fully(n_ptr->ty + dy, n_ptr->tx + dx)
						&& cave_exist_mon(n_ptr->r_idx, n_ptr->ty + dy, n_ptr->tx + dx, TRUE)
						&& (count++ < (MAX_SIGHT / 2)))
				{
					n_ptr->ty = n_ptr->ty + dy;
					n_ptr->tx = n_ptr->tx + dx;
				}

				/* Clear target if none available */
				if ((n_ptr->ty == n_ptr->fy) && (n_ptr->tx == n_ptr->fx))
				{
					n_ptr->ty = 0;
					n_ptr->tx = 0;
				}
			}

			/* The other monster cannot switch places */
			else if (!cave_exist_mon(n_ptr->r_idx, p_ptr->py, p_ptr->px, TRUE))
			{
				/* Try to push it aside. Allow aborting of move if an ally */
				if ((!push_aside(p_ptr->py, p_ptr->px, n_ptr)) && (n_ptr->mflag & (MFLAG_ALLY)))
				{
					/* Don't provide more warning */
					if (!get_check("Are you sure?")) return;
				}
			}

			/* Hack -- we clear the target if we move over a monster */
			else
			{
				n_ptr->ty = 0;
				n_ptr->tx = 0;
			}

			/* Mark monsters as pushed */
			n_ptr->mflag |= (MFLAG_PUSH);
		}

		/* Move player */
		monster_swap(py, px, y, x);

		/* New location */
		y = py = p_ptr->py;
		x = px = p_ptr->px;

		/* No longer climbing */
		p_ptr->climbing = 0;

		/* Allow running to abort sneaking */
		if (p_ptr->running) p_ptr->not_sneaking = FALSE;

		/* Spontaneous Searching */
		if ((p_ptr->skills[SKILL_SEARCH] >= 50) ||
		    (0 == rand_int(50 - p_ptr->skills[SKILL_SEARCH])))
		{
			search();
		}

		/* Catch breath */
		if (!(f_ptr->flags2 & (FF2_FILLED)))
		{
			/* Rest the player */
			set_rest(p_ptr->rest + PY_REST_RATE - p_ptr->tiring);
		}

		/* Continuous Searching */
		if (p_ptr->searching)
		{
			search();
		}

		/* Dodging */
		else
		{
			/* Dodging -- reverse direction 180 degrees */
			p_ptr->dodging = 10 - dir;
		}

		/* Handle "store doors" */
		if (f_ptr->flags1 & (FF1_ENTER))
		{
			/* Disturb */
			disturb(0, 0);

			/* Hack -- Enter store */
			p_ptr->command_new.key = '_';

			/* Free turn XXX XXX XXX */
			p_ptr->energy_use = 0;
		}

		/* All other grids (including traps) */
		else
		{
			/* Handle objects (later) */
			p_ptr->notice |= (PN_PICKUP);
		}

		/* Get hit by regions */
		if (cave_region_piece[y][x])
		{
			/* Trigger the region */
			trigger_region(y, x, TRUE);
		}

		/* Get hit by terrain/traps */
		/* Except for chests which must be opened to hit */
		if (((f_ptr->flags1 & (FF1_HIT_TRAP)) && !(f_ptr->flags3 & (FF3_CHEST))) ||
			(f_ptr->spell) || (f_ptr->blow.method))
		{
			/* Disturb */
			disturb(0, 0);

			/* Hit the trap */
			hit_trap(y, x);
		}

		/* Discover secrets */
		/* Except for hidden objects/gold which must be found by searching */
		else if ((f_ptr->flags1 & (FF1_SECRET))
			&& !(f_ptr->flags1 & (FF1_HAS_ITEM | FF1_HAS_GOLD)))
		{
			/* Find the secret */
			find_secret(y,x);

		}

		else if (f_ptr->flags2 & (FF2_KILL_MOVE))
		{
			cptr text;

			/* Get the feature description */
			text = (f_text + f_ptr->text);

			if (strlen(text))
			{
				/* You have found something */
				msg_format("%s",text);
			}

			cave_alter_feat(y,x, FS_KILL_MOVE);
		}

		/* Reveal when you are on shallow, deep or filled terrain */
		if (!(play_info[y][x] & (PLAY_MARK)) &&
		((f_ptr->flags2 & (FF2_SHALLOW)) ||
		(f_ptr->flags2 & (FF2_DEEP)) ||
		(f_ptr->flags2 & (FF2_FILLED)) ))
		{
			/* Get the mimiced feature */
			mimic = f_ptr->mimic;

			/* Get the feature name */
			name = (f_name + f_info[mimic].name);

			/* Tell the player */
			msg_format("You feel you are %s%s.",
				((f_ptr->flags2 & (FF2_FILLED)) ? "" : "in "), name);

			play_info[y][x] |= (PLAY_MARK);

			lite_spot(y, x);
		}
	}
}



/*
 * Hack -- allow quick "cycling" through the legal directions
 */
static const byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static const byte chome[] =
{ 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };



/*
 * We use this to ignore isolated pillars
 *
 * XXX Note we don't check whether the player can
 * see the dead end. Otherwise we'd stop at every
 * pillar the first time, which would make navigating
 * unknown pillared rooms and corridors unnecessarily
 * annoying.
 */
static bool see_dead_end(int y, int x)
{
	int yy, xx, i;
	int move = 0L;

	/* Count transitions from movable to non-movable and vice versa */
	int k =  0;

	/* Check for adjacent movable grids */
	for (i = 0; i < 8; i++)
	{
		yy = y + ddy[cycle[i]];
		xx = x + ddx[cycle[i]];

		if (!in_bounds_fully(yy, xx)) continue;

		/*if (!(play_info[yy][xx] & (PLAY_MARK))) continue;*/

		if (f_info[f_info[cave_feat[yy][xx]].mimic].flags1 & (FF1_MOVE))
		{
			move |= 1L << i;
		}

		/* Previous location does not match */
		if ((i > 0) && (((move & (1L << (i - 1))) != 0) != ((move & (1L << i)) != 0))) k++;

		/* Too many discontinuities */
		if (k > 2) return (FALSE);
	}

	/* Check first and last */
	if ((((move & (1L << 0)) != 0) != ((move & (1L << 7)) != 0)) && (k > 1)) return (FALSE);

	/* Reset counter */
	k = 0;

	/* Check for more than 4 contiguous empty locations */
	for (i = 0; i < 8 + 4; i++)
	{
		/* Count open areas */
		if (move & 1L << (i % 8)) k++;
		else k = 0;

		if (k > 4) return (FALSE);
	}

	/* Checked all locations -- dead end */
	return (TRUE);
}


/*
 * We use this to ignore isolated pillars
 *
 * XXX Note we don't check whether the player can
 * see the pillar. Otherwise we'd stop at every
 * pillar the first time, which would make navigating
 * unknown pillared rooms and corridors unnecessarily
 * annoying.
 */
static bool see_pillar(int y, int x)
{
	int yy, xx, i;

	/* Not a wall */
	if ((f_info[f_info[cave_feat[y][x]].mimic].flags1 & (FF1_WALL)) == 0) return (FALSE);

	/* Isolated pillars are not known walls */
	for (i = 0; i < 8; i++)
	{
		yy = y + ddy_ddd[i];
		xx = x + ddx_ddd[i];

		if (!in_bounds_fully(yy, xx)) break;

		/*if (!(play_info[yy][xx] & (PLAY_MARK))) break;*/

		if (f_info[f_info[cave_feat[yy][xx]].mimic].flags1 & (FF1_WALL)) break;
	}

	/* Checked all locations -- pillar is isolated */
	if (i == 8) return (TRUE);

	return (FALSE);
}


/*
 * Hack -- Check for a "known wall" (see below)
 */
static int see_wall(int dir, int y, int x)
{
	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are not known walls XXX XXX XXX */
	if (!in_bounds(y, x)) return (FALSE);

	/* Non-wall grids are not known walls */
	if (!(f_info[f_info[cave_feat[y][x]].mimic].flags1 & (FF1_WALL)))
	{
		/* Except where the non-wall grid is a short known dead end */
		if (see_dead_end(y, x)) return (TRUE);

		return (FALSE);
	}

	/* Unknown walls are not known walls */
	if (!(play_info[y][x] & (PLAY_MARK))) return (FALSE);

	/* Isolated pillars are not know walls */
	if (see_pillar(y, x)) return (FALSE);

	/* Default */
	return (TRUE);
}

/*
 * Hack -- Check for a "known obstacle" (see below)
 */
static int see_stop(int dir, int y, int x)
{
	s16b this_region_piece, next_region_piece = 0;

	int feat = f_info[cave_feat[y][x]].mimic;

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are not known walls XXX XXX XXX */
	if (!in_bounds(y, x)) return (FALSE);

	/* Unknown walls are not known obstacles */
	if (!(play_info[y][x] & (PLAY_MARK))) return (FALSE);

	/* Don't move over known regions */
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
				(((play_info[y][x] & (PLAY_REGN | PLAY_SEEN)) != 0) ||
					(((play_info[y][x] & (PLAY_VIEW)) != 0) && ((r_ptr->flags1 & (RE1_SHINING)) != 0))) &&
						((r_ptr->flags1 & (RE1_DISPLAY)) != 0))
		{
			return (TRUE);
		}
	}

	/* Run-able grids are not known obstacles */
	if (f_info[feat].flags1 & (FF1_RUN)) return (FALSE);

	/* The terrain the player started on is not a known obstacle */
	if (feat == p_ptr->run_cur_feat) return (FALSE);

	/* Default */
	return (TRUE);
}


/*
 * The running algorithm  -CJS-
 *
 * Basically, once you start running, you keep moving until something
 * interesting happens.  In an enclosed space, you run straight, but
 * you follow corners as needed (i.e. hallways).  In an open space,
 * you run straight, but you stop before entering an enclosed space
 * (i.e. a room with a doorway).  In a semi-open space (with walls on
 * one side only), you run straight, but you stop before entering an
 * enclosed space or an open space (i.e. running along side a wall).
 *
 * All discussions below refer to what the player can see, that is,
 * an unknown wall is just like a normal floor.  This means that we
 * must be careful when dealing with "illegal" grids.
 *
 * No assumptions are made about the layout of the dungeon, so this
 * algorithm works in hallways, rooms, town, destroyed areas, etc.
 *
 * In the diagrams below, the player has just arrived in the grid
 * marked as '@', and he has just come from a grid marked as 'o',
 * and he is about to enter the grid marked as 'x'.
 *
 * Running while confused is not allowed, and so running into a wall
 * is only possible when the wall is not seen by the player.  This
 * will take a turn and stop the running.
 *
 * Several conditions are tracked by the running variables.
 *
 *   p_ptr->run_open_area (in the open on at least one side)
 *   p_ptr->run_break_left (wall on the left, stop if it opens)
 *   p_ptr->run_break_right (wall on the right, stop if it opens)
 *
 * When running begins, these conditions are initialized by examining
 * the grids adjacent to the requested destination grid (marked 'x'),
 * two on each side (marked 'L' and 'R').  If either one of the two
 * grids on a given side is a wall, then that side is considered to
 * be "closed".  Both sides enclosed yields a hallway.
 *
 *    LL     @L
 *    @x      (normal)       RxL   (diagonal)
 *    RR      (east)  R    (south-east)
 *
 * In the diagram below, in which the player is running east along a
 * hallway, he will stop as indicated before attempting to enter the
 * intersection (marked 'x').  Starting a new run in any direction
 * will begin a new hallway run.
 *
 * #.#
 * ##.##
 * o@x..
 * ##.##
 * #.#
 *
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * In the diagram below, the player is running east down a hallway,
 * and will stop in the grid (marked '1') before the intersection.
 * Continuing the run to the south-east would result in a long run
 * stopping at the end of the hallway (marked '2').
 *
 * ##################
 * o@x       1
 * ########### ######
 * #2  #
 * #############
 *
 * After each step, the surroundings are examined to determine if
 * the running should stop, and to determine if the running should
 * change direction.  We examine the new current player location
 * (at which the runner has just arrived) and the direction from
 * which the runner is considered to have come.
 *
 * Moving one grid in some direction places you adjacent to three
 * or five new grids (for straight and diagonal moves respectively)
 * to which you were not previously adjacent (marked as '!').
 *
 *   ...!      ...
 *   .o@!  (normal)    .o.!  (diagonal)
 *   ...!  (east)      ..@!  (south east)
 *      !!!
 *
 * If any of the newly adjacent grids are "interesting" (monsters,
 * objects, some terrain features) then running stops.
 *
 * If any of the newly adjacent grids seem to be open, and you are
 * looking for a break on that side, then running stops.
 *
 * If any of the newly adjacent grids do not seem to be open, and
 * you are in an open area, and the non-open side was previously
 * entirely open, then running stops.
 *
 * If you are in a hallway, then the algorithm must determine if
 * the running should continue, turn, or stop.  If only one of the
 * newly adjacent grids appears to be open, then running continues
 * in that direction, turning if necessary.  If there are more than
 * two possible choices, then running stops.  If there are exactly
 * two possible choices, separated by a grid which does not seem
 * to be open, then running stops.  Otherwise, as shown below, the
 * player has probably reached a "corner".
 *
 *    ###     o##
 *    o@x  (normal)   #@!   (diagonal)
 *    ##!  (east)     ##x   (south east)
 *
 * In this situation, there will be two newly adjacent open grids,
 * one touching the player on a diagonal, and one directly adjacent.
 * We must consider the two "option" grids further out (marked '?').
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid.  For some unknown reason, we assign "check_dir" to
 * the grid marked 's', which may be incorrectly labelled.
 *
 *    ###s
 *    o@x?   (may be incorrect diagram!)
 *    ##!?
 *
 * If both "option" grids are closed, then there is no reason to enter
 * the corner, and so we can cut the corner, by moving into the other
 * grid (diagonally).  If we choose not to cut the corner, then we may
 * go straight, but we pretend that we got there by moving diagonally.
 * Below, we avoid the obvious grid (marked 'x') and cut the corner
 * instead (marked 'n').
 *
 *    ###:       o##
 *    o@x#   (normal)    #@n    (maybe?)
 *    ##n#   (east)      ##x#
 *       ####
 *
 * If one of the "option" grids is open, then we may have a choice, so
 * we check to see whether it is a potential corner or an intersection
 * (or room entrance).  If the grid two spaces straight ahead, and the
 * space marked with 's' are both open, then it is a potential corner
 * and we enter it if requested.  Otherwise, we stop, because it is
 * not a corner, and is instead an intersection or a room entrance.
 *
 *    ###
 *    o@x
 *    ##!#
 *
 * I do not think this documentation is correct.
 */



/*
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. (?)
 *
 * Diagonal Corridor    Blunt Corridor (?)
 *       # #  #
 *       #x# @x#
 *       @p.  p
 */
static void run_init(int dir)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, row, col;

	bool deepleft, deepright;
	bool shortleft, shortright;


	/* Save the direction */
	p_ptr->run_cur_dir = dir;

	/* Save the feature */
	p_ptr->run_cur_feat = f_info[cave_feat[py][px]].mimic;

	/* Save adjacent features */
	for (i = 0; i < 9; i++)
	{
		int y = py + ddy_ddd[i];
		int x = px + ddx_ddd[i];
		
		/* Add feats we would otherwise notice */
		if ((f_info[f_info[cave_feat[y][x]].mimic].flags1 & (FF1_NOTICE)) != 0)
		{
			p_ptr->run_ignore_feat[i] = f_info[cave_feat[y][x]].mimic;
		}
		else
		{
			p_ptr->run_ignore_feat[i] = 0;
		}
	}
	
	/* Assume running straight */
	p_ptr->run_old_dir = dir;

	/* Assume looking for open area */
	p_ptr->run_open_area = TRUE;

	/* Assume not looking for breaks */
	p_ptr->run_break_right = FALSE;
	p_ptr->run_break_left = FALSE;

	/* Assume no nearby walls */
	deepleft = deepright = FALSE;
	shortright = shortleft = FALSE;

	/* Find the destination grid */
	row = py + ddy[dir];
	col = px + ddx[dir];

	/* Extract cycle index */
	i = chome[dir];

	/* Check for nearby wall */
	if (see_wall(cycle[i+1], py, px))
	{
		p_ptr->run_break_left = TRUE;
		shortleft = TRUE;
	}

	/* Check for distant wall */
	else if (see_wall(cycle[i+1], row, col))
	{
		p_ptr->run_break_left = TRUE;
		deepleft = TRUE;
	}

	/* Check for nearby wall */
	if (see_wall(cycle[i-1], py, px))
	{
		p_ptr->run_break_right = TRUE;
		shortright = TRUE;
	}

	/* Check for distant wall */
	else if (see_wall(cycle[i-1], row, col))
	{
		p_ptr->run_break_right = TRUE;
		deepright = TRUE;
	}

	/* Looking for a break */
	if (p_ptr->run_break_left && p_ptr->run_break_right)
	{
		/* Not looking for open area */
		p_ptr->run_open_area = FALSE;

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01)
		{
			if (deepleft && !deepright)
			{
				p_ptr->run_old_dir = cycle[i - 1];
			}
			else if (deepright && !deepleft)
			{
				p_ptr->run_old_dir = cycle[i + 1];
			}
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(cycle[i], row, col))
		{
			if (shortleft && !shortright)
			{
				p_ptr->run_old_dir = cycle[i - 2];
			}
			else if (shortright && !shortleft)
			{
				p_ptr->run_old_dir = cycle[i + 2];
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
	int py = p_ptr->py;
	int px = p_ptr->px;

	int prev_dir;
	int new_dir;
	int check_dir = 0;

	int row, col;
	int i, max, inv;
	int option, option2;

	int feat;

	bool pillar = FALSE;

	/* No options yet */
	option = 0;
	option2 = 0;

	/* Where we came from */
	prev_dir = p_ptr->run_old_dir;


	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;


	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++)
	{
		s16b this_o_idx, next_o_idx = 0;


		/* New direction */
		new_dir = cycle[chome[prev_dir] + i];

		/* New location */
		row = py + ddy[new_dir];
		col = px + ddx[new_dir];

		/* Visible monsters abort running */
		if (cave_m_idx[row][col] > 0)
		{
			monster_type *m_ptr = &m_list[cave_m_idx[row][col]];

			/* Visible monster */
			if (m_ptr->ml) return (TRUE);
		}

		/* Visible pickable objects abort running */
		for (this_o_idx = cave_o_idx[row][col]; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Get the object */
			o_ptr = &o_list[this_o_idx];

			/* Get the next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Visible object */
			if (o_ptr->ident & (IDENT_MARKED)
				 && !auto_pickup_never(o_ptr)
				 && !auto_pickup_ignore(o_ptr))
				return (TRUE);
		}

		/* Assume unknown */
		inv = TRUE;

		/* Get feature */
		feat = cave_feat[row][col];

		/* Set mimiced feature */
		feat = f_info[feat].mimic;

		/* Check memorized grids */
		if (play_info[row][col] & (PLAY_MARK))
		{
			bool notice;
			
			feature_type *f_ptr = &f_info[feat];

			/* Notice feature? */
			notice = ((f_ptr->flags1 & (FF1_NOTICE)) !=0);

			/* Ignore stuff we can move over */
			if ((notice) && ((f_ptr->flags1 & (FF1_MOVE))
					|| (f_ptr->flags3 & (FF3_EASY_CLIMB))))
			{
				/* Ignore unusual floors; don't run if you want to inspect them */
				if (f_ptr->flags1 & (FF1_FLOOR))
					notice = FALSE;

				/* Ignore whatever we're standing on */
				if (feat == p_ptr->run_cur_feat) notice = FALSE;
			}
			
			/* Ignore stuff around us, except when stepping on it */
			if ((notice) && (i))
			{
				int j;

				/* Ignore whatever was around us to start */
				for (j = 0; j < 9; j++)
				{
					if (p_ptr->run_ignore_feat[j] == feat) notice = FALSE;
				}
			}

			/* Interesting feature */
			if (notice) return (TRUE);

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Analyze unknown grids and floors */
		if ((inv || cave_floor_bold(row, col) || (feat == p_ptr->run_cur_feat)) &&
				!see_dead_end(row, col))
		{
			/* Looking for open area */
			if (p_ptr->run_open_area)
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

			/* Two non-adjacent new directions.  Stop running. Ignore pillar between them. */
			else if ((option != cycle[chome[prev_dir] + i - 1]) && !(pillar))
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

			pillar = FALSE;
		}

		/* Obstacle, while looking for open area */
		else
		{
			/* Check for pillars if we're looking for open areas and not breaks */
			if (((p_ptr->run_open_area) ||
					((i < 0) && (!p_ptr->run_break_left)) ||
					((i > 0) && (!p_ptr->run_break_right))) &&
					(see_pillar(row, col)))
			{
				pillar = TRUE;
			}
			else
			{
				pillar = FALSE;

				if (p_ptr->run_open_area)
				{
					if (i < 0)
					{
						/* Break to the right */
						p_ptr->run_break_right = TRUE;
					}

					else if (i > 0)
					{
						/* Break to the left */
						p_ptr->run_break_left = TRUE;
					}
				}
			}
		}
	}


	/* Looking for open area */
	if (p_ptr->run_open_area)
	{
		/* Hack -- look again */
		for (i = -max; i < 0; i++)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = py + ddy[new_dir];
			col = px + ddx[new_dir];

			/* Get feature */
			feat = cave_feat[row][col];

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Unknown grid or non-wall */
			/* Was: cave_floor_bold(row, col) */
			if (!(play_info[row][col] & (PLAY_MARK)) ||
			    (!(f_info[feat].flags1 & (FF1_WALL))) ||
			    see_pillar(row, col))
			{
				/* Looking to break right */
				if (p_ptr->run_break_right)
				{
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break left */
				if (p_ptr->run_break_left)
				{
					return (TRUE);
				}
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = py + ddy[new_dir];
			col = px + ddx[new_dir];

			/* Get feature */
			feat = cave_feat[row][col];

			/* Get mimiced feature */
			feat = f_info[feat].mimic;

			/* Unknown grid or non-wall */
			/* Was: cave_floor_bold(row, col) */
			if (!(play_info[row][col] & (PLAY_MARK)) ||
			    (!(f_info[feat].flags1 & (FF1_WALL))) ||
			    (see_pillar(row, col)))
			{
				/* Looking to break left */
				if (p_ptr->run_break_left)
				{
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if (p_ptr->run_break_right)
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
			p_ptr->run_cur_dir = option;

			/* No other options */
			p_ptr->run_old_dir = option;
		}

		/* Two options, examining corners */
		else
		{
			/* Primary option */
			p_ptr->run_cur_dir = option;

			/* Hack -- allow curving */
			p_ptr->run_old_dir = option2;
		}
	}


	/* About to hit a known wall, stop */
	if (see_stop(p_ptr->run_cur_dir, py, px))
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
void run_step(int dir)
{
	/* Save the feature. We do this to allow a player to start
	 * running on 'unusual terrain, step off it and continue to
	 * run, but not step back on it again. */
	p_ptr->run_cur_feat = f_info[cave_feat[p_ptr->py][p_ptr->px]].mimic;
	
	/* Start run */
	if (dir)
	{
		/* Initialize */
		run_init(dir);

		/* Hack -- Set the run counter */
		p_ptr->running = (p_ptr->command_arg ? p_ptr->command_arg : 1000);

		/* Hack -- not running with pathfind */
		p_ptr->running_withpathfind = FALSE;

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
	}

	/* Continue run */
	else
	{
		if (!p_ptr->running_withpathfind)
		{
			/* Update run */
			if (run_test())
			{
				/* Disturb */
				disturb(0, 0);

				/* Done */
				return;
			}
		}
		else
		{
			/* Abort if we have finished */
			if (pf_result_index < 0)
			{
				disturb(0, 0);
				p_ptr->running_withpathfind = FALSE;
				return;
			}
			/* Abort if we would hit a wall */
			else if (pf_result_index == 0)
			{
				int y, x;

				/* Get next step */
				y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
				x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];

				/* Known wall */
				if ((play_info[y][x] & (PLAY_MARK)) && !is_valid_pf(y,x))
				{
					disturb(0,0);
					p_ptr->running_withpathfind = FALSE;
					return;
				}
			}
			/* Hack -- walking stick lookahead.
			 *
			 * If the player has computed a path that is going to end up in a wall,
			 * we notice this and convert to a normal run. This allows us to click
			 * on unknown areas to explore the map.
			 *
			 * We have to look ahead two, otherwise we don't know which is the last
			 * direction moved and don't initialise the run properly.
			 */
			else if (pf_result_index > 0)
			{
				int y, x;

				/* Get next step */
				y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
				x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];

				/* Known wall */
				if ((play_info[y][x] & (PLAY_MARK)) && !is_valid_pf(y,x))
				{
					disturb(0,0);
					p_ptr->running_withpathfind = FALSE;
					return;
				}

				/* Get step after */
				y = y + ddy[pf_result[pf_result_index-1] - '0'];
				x = x + ddx[pf_result[pf_result_index-1] - '0'];

				/* Known wall */
				if ((play_info[y][x] & (PLAY_MARK)) && !is_valid_pf(y,x))
				{
					p_ptr->running_withpathfind = FALSE;

					run_init(pf_result[pf_result_index] - '0');
				}
			}

			p_ptr->run_cur_dir = pf_result[pf_result_index--] - '0';

			/* Hack -- allow easy_alter */
			p_ptr->command_dir = p_ptr->run_cur_dir;
		}
	}

	/* Decrease counter */
	p_ptr->running--;

	/* Take time */
	p_ptr->energy_use = 100;

	/* Move the player */
	move_player(p_ptr->run_cur_dir);

	return;
}
