/* File: randart.c */


/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"

#include "init.h"

#ifdef GJW_RANDART

#define LOG_PRINT(string) \
	do { if (randart_verbose) \
		fprintf(randart_log, (string)); \
	} while (0);

#define LOG_PRINT1(string, value) \
	do { if (randart_verbose) \
		fprintf(randart_log, (string), (int)(value)); \
	} while (0);

#define LOG_PRINT2(string, val1, val2) \
	do { if (randart_verbose) \
		fprintf(randart_log, (string), (int)(val1), (int)(val2)); \
	} while (0);


#define MAX_TRIES 200
#define BUFLEN 1024

#define MIN_ART_NAME_LEN 5
#define MAX_ART_NAME_LEN 9

#define sign(x)	((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))


/*
 * Average damage for good ego ammo of various types, used for balance
 * The current values assume normal (non-seeker) ammo enchanted to +9
 */

#define AVG_SLING_AMMO_DAMAGE 11
#define AVG_BOW_AMMO_DAMAGE 12
#define AVG_XBOW_AMMO_DAMAGE 12

/* Inhibiting factors for large bonus values */
#define INHIBIT_STRONG 6
#define INHIBIT_WEAK 2

/*
 * Numerical index values for the different learned probabilities
 * These are to make the code more readable.
 */

#define ART_IDX_BOW_SHOTS 0
#define ART_IDX_BOW_MIGHT 1
#define ART_IDX_WEAPON_HIT 2
#define ART_IDX_WEAPON_DAM 3
#define ART_IDX_NONWEAPON_HIT 4
#define ART_IDX_NONWEAPON_DAM 5

#define ART_IDX_MELEE_BLESS 6
#define ART_IDX_MELEE_BRAND_SLAY 7
#define ART_IDX_MELEE_SINV 8
#define ART_IDX_MELEE_BLOWS 9
#define ART_IDX_MELEE_AC 10
#define ART_IDX_MELEE_DICE 11
#define ART_IDX_MELEE_WEIGHT 12
#define ART_IDX_MELEE_TUNN 13

#define ART_IDX_ALLARMOR_WEIGHT 14

#define ART_IDX_BOOT_AC 15
#define ART_IDX_BOOT_FEATHER 16
#define ART_IDX_BOOT_STEALTH 17
#define ART_IDX_BOOT_SPEED 18

#define ART_IDX_GLOVE_AC 19
#define ART_IDX_GLOVE_FA 20
#define ART_IDX_GLOVE_DEX 21

#define ART_IDX_HELM_AC 22
#define ART_IDX_HELM_RBLIND 23
#define ART_IDX_HELM_ESP 24
#define ART_IDX_HELM_SINV 25
#define ART_IDX_HELM_WIS 26
#define ART_IDX_HELM_INT 27

#define ART_IDX_SHIELD_AC 28
#define ART_IDX_SHIELD_LRES 29

#define ART_IDX_CLOAK_AC 30
#define ART_IDX_CLOAK_STEALTH 31

#define ART_IDX_ARMOR_AC 32
#define ART_IDX_ARMOR_STEALTH 33
#define ART_IDX_ARMOR_HLIFE 34
#define ART_IDX_ARMOR_CON 35
#define ART_IDX_ARMOR_LRES 36
#define ART_IDX_ARMOR_ALLRES 37
#define ART_IDX_ARMOR_HRES 38

#define ART_IDX_GEN_STAT 39
#define ART_IDX_GEN_SUST 40
#define ART_IDX_GEN_STEALTH 41
#define ART_IDX_GEN_SEARCH 42
#define ART_IDX_GEN_INFRA 43
#define ART_IDX_GEN_SPEED 44
#define ART_IDX_GEN_IMMUNE 45
#define ART_IDX_GEN_FA 46
#define ART_IDX_GEN_HLIFE 47
#define ART_IDX_GEN_FEATHER 48
#define ART_IDX_GEN_LITE 49
#define ART_IDX_GEN_SINV 50
#define ART_IDX_GEN_ESP 51
#define ART_IDX_GEN_SDIG 52
#define ART_IDX_GEN_REGEN 53
#define ART_IDX_GEN_LRES 54
#define ART_IDX_GEN_RPOIS 55
#define ART_IDX_GEN_RFEAR 56
#define ART_IDX_GEN_RLITE 57
#define ART_IDX_GEN_RDARK 58
#define ART_IDX_GEN_RBLIND 59
#define ART_IDX_GEN_RCONF 60
#define ART_IDX_GEN_RSOUND 61
#define ART_IDX_GEN_RSHARD 62
#define ART_IDX_GEN_RNEXUS 63
#define ART_IDX_GEN_RNETHER 64
#define ART_IDX_GEN_RCHAOS 65
#define ART_IDX_GEN_RDISEN 66
#define ART_IDX_GEN_AC 67
#define ART_IDX_GEN_TUNN 68

/* Supercharged abilities - treated differently in algorithm */

#define ART_IDX_MELEE_DICE_SUPER 69
#define ART_IDX_BOW_SHOTS_SUPER 70
#define ART_IDX_BOW_MIGHT_SUPER 71
#define ART_IDX_GEN_SPEED_SUPER 72

/* Aggravation - weapon and nonweapon */
#define ART_IDX_WEAPON_AGGR 73
#define ART_IDX_NONWEAPON_AGGR 74

/* Start of ESP_IDX defines ARD_ESP */
#define ART_IDX_MELEE_SENSE 75
#define ART_IDX_BOW_SENSE 76
#define ART_IDX_NONWEAPON_SENSE 77

#define ART_IDX_MELEE_RESTRICT 78
#define ART_IDX_BOW_RESTRICT 79
#define ART_IDX_NONWEAPON_RESTRICT 80

#define ART_IDX_GEN_SAVE 81
#define ART_IDX_GEN_DEVICE 82
#define ART_IDX_GEN_RDISEA 83

/* Total of abilities */
#define ART_IDX_TOTAL 84

/* End of ESP_IDX defines ARD_ESP */

/* Tallies of different ability types */
#define ART_IDX_BOW_COUNT 3
#define ART_IDX_WEAPON_COUNT 3
#define ART_IDX_NONWEAPON_COUNT 4
#define ART_IDX_MELEE_COUNT 9
#define ART_IDX_ALLARMOR_COUNT 1
#define ART_IDX_BOOT_COUNT 4
#define ART_IDX_GLOVE_COUNT 3
#define ART_IDX_HELM_COUNT 6
#define ART_IDX_SHIELD_COUNT 2
#define ART_IDX_CLOAK_COUNT 2
#define ART_IDX_ARMOR_COUNT 7
#define ART_IDX_GEN_COUNT 30
#define ART_IDX_HIGH_RESIST_COUNT 13

/* Arrays of indices by item type, used in frequency generation */

/* ARD_ESP -- following changed to include ARD_IDX_BOW_SENSE */
static s16b art_idx_bow[] =
	{ART_IDX_BOW_SHOTS, ART_IDX_BOW_MIGHT, ART_IDX_BOW_SENSE, ART_IDX_BOW_RESTRICT};
static s16b art_idx_weapon[] =
	{ART_IDX_WEAPON_HIT, ART_IDX_WEAPON_DAM, ART_IDX_WEAPON_AGGR};
/* ARD_ESP -- following changed to include ARD_IDX_NONWEAPON_SENSE */
static s16b art_idx_nonweapon[] =
	{ART_IDX_NONWEAPON_HIT, ART_IDX_NONWEAPON_DAM, ART_IDX_NONWEAPON_AGGR,
	ART_IDX_NONWEAPON_SENSE, ART_IDX_NONWEAPON_RESTRICT};
/* ARD_ESP -- following changed to include ARD_IDX_MELEE_SENSE */
static s16b art_idx_melee[] =
	{ART_IDX_MELEE_BLESS, ART_IDX_MELEE_BRAND_SLAY, ART_IDX_MELEE_SINV,
	ART_IDX_MELEE_BLOWS, ART_IDX_MELEE_AC, ART_IDX_MELEE_DICE,
	ART_IDX_MELEE_WEIGHT, ART_IDX_MELEE_TUNN, ART_IDX_MELEE_SENSE, ART_IDX_MELEE_RESTRICT};
static s16b art_idx_allarmor[] =
	{ART_IDX_ALLARMOR_WEIGHT};
static s16b art_idx_boot[] =
	{ART_IDX_BOOT_AC, ART_IDX_BOOT_FEATHER, ART_IDX_BOOT_STEALTH, ART_IDX_BOOT_SPEED};
static s16b art_idx_glove[] =
	{ART_IDX_GLOVE_AC, ART_IDX_GLOVE_FA, ART_IDX_GLOVE_DEX};
static s16b art_idx_headgear[] =
	{ART_IDX_HELM_AC, ART_IDX_HELM_RBLIND, ART_IDX_HELM_ESP, ART_IDX_HELM_SINV,
	ART_IDX_HELM_WIS, ART_IDX_HELM_INT};
static s16b art_idx_shield[] =
	{ART_IDX_SHIELD_AC, ART_IDX_SHIELD_LRES};
static s16b art_idx_cloak[] =
	{ART_IDX_CLOAK_AC, ART_IDX_CLOAK_STEALTH};
static s16b art_idx_armor[] =
	{ART_IDX_ARMOR_AC, ART_IDX_ARMOR_STEALTH, ART_IDX_ARMOR_HLIFE, ART_IDX_ARMOR_CON,
	ART_IDX_ARMOR_LRES, ART_IDX_ARMOR_ALLRES, ART_IDX_ARMOR_HRES};
static s16b art_idx_gen[] =
	{ART_IDX_GEN_STAT, ART_IDX_GEN_SUST, ART_IDX_GEN_STEALTH,
	ART_IDX_GEN_SEARCH, ART_IDX_GEN_INFRA, ART_IDX_GEN_SPEED,
	ART_IDX_GEN_IMMUNE, ART_IDX_GEN_FA, ART_IDX_GEN_HLIFE,
	ART_IDX_GEN_FEATHER, ART_IDX_GEN_LITE, ART_IDX_GEN_SINV,
	ART_IDX_GEN_ESP, ART_IDX_GEN_SDIG, ART_IDX_GEN_REGEN,
	ART_IDX_GEN_LRES, ART_IDX_GEN_RPOIS, ART_IDX_GEN_RFEAR,
	ART_IDX_GEN_RLITE, ART_IDX_GEN_RDARK, ART_IDX_GEN_RBLIND,
	ART_IDX_GEN_RCONF, ART_IDX_GEN_RSOUND, ART_IDX_GEN_RSHARD,
	ART_IDX_GEN_RNEXUS, ART_IDX_GEN_RNETHER, ART_IDX_GEN_RCHAOS,
	ART_IDX_GEN_RDISEN, ART_IDX_GEN_AC, ART_IDX_GEN_TUNN,
	ART_IDX_GEN_SAVE, ART_IDX_GEN_DEVICE};
static s16b art_idx_high_resist[] =
	{ART_IDX_GEN_RPOIS, ART_IDX_GEN_RFEAR,
	ART_IDX_GEN_RLITE, ART_IDX_GEN_RDARK, ART_IDX_GEN_RBLIND,
	ART_IDX_GEN_RCONF, ART_IDX_GEN_RSOUND, ART_IDX_GEN_RSHARD,
	ART_IDX_GEN_RNEXUS, ART_IDX_GEN_RNETHER, ART_IDX_GEN_RCHAOS,
	ART_IDX_GEN_RDISEN, ART_IDX_GEN_RDISEA};


/* Initialize the data structures for learned probabilities */

static s16b artprobs[ART_IDX_TOTAL];
static s16b art_bow_total = 0;
static s16b art_melee_total = 0;
static s16b art_boot_total = 0;
static s16b art_glove_total = 0;
static s16b art_headgear_total = 0;
static s16b art_shield_total = 0;
static s16b art_cloak_total = 0;
static s16b art_armor_total = 0;
static s16b art_other_total = 0;
static s16b art_total = 0;

/*
 * Working array for holding frequency values - global to avoid repeated
 * allocation of memory
 */

static s16b art_freq[ART_IDX_TOTAL];
/*
 * Mean start and increment values for to_hit, to_dam and AC.  Update these
 * if the algorithm changes.  They are used in frequency generation.
 */

static s16b mean_hit_increment = 3;
static s16b mean_dam_increment = 3;
static s16b mean_hit_startval = 8;
static s16b mean_dam_startval = 8;
static s16b mean_ac_startval = 15;
static s16b mean_ac_increment = 5;

/*
 * Pointer for logging file
 */

static FILE *randart_log;

/*
 * Cache the results of lookup_kind(), which is expensive and would
 * otherwise be called much too often.
 */
static s16b *kinds;

/*
 * Store the original artifact power ratings
 */
static s32b *base_power;

/*
 * Store the original base item levels
 */
static byte *base_item_level;

/*
 * Store the original base item rarities
 */
static byte *base_item_rarity;

/*
 * Store the original artifact rarities
 */
static byte *base_art_rarity;

/* Global just for convenience. */
static int randart_verbose = 0;

/*
 * Use W. Sheldon Simms' random name generator.
 */
static errr init_names(void)
{
	char buf[BUFLEN];
	size_t name_size;
	char *a_base;
	char *a_next;
	int i;

	/* Temporary space for names, while reading and randomizing them. */
	char **names;

	/* Allocate the "names" array */
	/* ToDo: Make sure the memory is freed correctly in case of errors */
	names = (char**)C_ZNEW(z_info->a_max, cptr);

	for (i = 1 ; i < z_info->a_max; i++)
	{
		char *word = make_word(MIN_ART_NAME_LEN, MAX_ART_NAME_LEN);

		if (i == ART_POWER)
		{
			names[i-1] = string_make("of Power (The One Ring)");
			continue;
		}

		if (i == ART_GROND)
		{
			names[i-1] = string_make("'Grond'");
			continue;
		}

		if (i == ART_MORGOTH)
		{
			names[i-1] = string_make("of Morgoth");
			continue;
		}

		if ((!adult_randarts) && (i<z_info->a_max_standard))
		{
			names[i-1] = string_make(a_name+a_info[i].name);

			continue;
		}

		if (rand_int(3) == 0)
			sprintf(buf, "'%s'", word);
		else
			sprintf(buf, "of %s", word);

		names[i-1] = string_make(buf);
	}

	/* Convert our names array into an a_name structure for later use. */
	name_size = 2;

	for (i = 1; i < z_info->a_max; i++)
	{
		name_size += strlen(names[i-1]) + 1;
	}

	a_base = C_ZNEW(name_size, char);

	a_next = a_base + 1;	/* skip first char */

	for (i = 1; i < z_info->a_max; i++)
	{
		strcpy(a_next, names[i-1]);
		if (a_info[i].tval > 0)
		{
			a_info[i].name = a_next - a_base;
		}
		a_next += strlen(names[i-1]) + 1;
	}

	/* Free the old names */
	FREE(a_name);

	for (i = 1; i < z_info->a_max; i++)
	{
		string_free(names[i-1]);
	}

	/* Free the "names" array */
	FREE(names);

	/* Store the names */
	a_head.name_ptr = a_name = a_base;

	/* Success */
	return (0);
}


static void remove_contradictory(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_AGGRAVATE) a_ptr->flags1 &= ~(TR1_STEALTH);
	if (a_ptr->flags2 & TR2_IM_ACID) a_ptr->flags2 &= ~(TR2_RES_ACID);
	if (a_ptr->flags2 & TR2_IM_ELEC) a_ptr->flags2 &= ~(TR2_RES_ELEC);
	if (a_ptr->flags2 & TR2_IM_FIRE) a_ptr->flags2 &= ~(TR2_RES_FIRE);
	if (a_ptr->flags2 & TR2_IM_COLD) a_ptr->flags2 &= ~(TR2_RES_COLD);
	if (a_ptr->flags4 & TR4_IM_POIS) a_ptr->flags2 &= ~(TR2_RES_POIS);

	if (a_ptr->flags2 & TR2_IM_ACID) a_ptr->flags4 &= ~(TR4_HURT_ACID);
	if (a_ptr->flags2 & TR2_IM_ELEC) a_ptr->flags4 &= ~(TR4_HURT_ELEC);
	if (a_ptr->flags2 & TR2_IM_FIRE) a_ptr->flags4 &= ~(TR4_HURT_FIRE);
	if (a_ptr->flags2 & TR2_IM_COLD) a_ptr->flags4 &= ~(TR4_HURT_COLD);

	if (a_ptr->flags2 & TR2_RES_ACID) a_ptr->flags4 &= ~(TR4_HURT_ACID);
	if (a_ptr->flags2 & TR2_RES_ELEC) a_ptr->flags4 &= ~(TR4_HURT_ELEC);
	if (a_ptr->flags2 & TR2_RES_FIRE) a_ptr->flags4 &= ~(TR4_HURT_FIRE);
	if (a_ptr->flags2 & TR2_RES_COLD) a_ptr->flags4 &= ~(TR4_HURT_COLD);
	if (a_ptr->flags2 & TR2_RES_LITE) a_ptr->flags4 &= ~(TR4_HURT_LITE);

	if (a_ptr->flags4 & TR4_RES_WATER) a_ptr->flags4 &= ~(TR4_HURT_WATER);

	if (a_ptr->pval < 0)
	{
		if (a_ptr->flags1 & TR1_STR) a_ptr->flags2 &= ~(TR2_SUST_STR);
		if (a_ptr->flags1 & TR1_INT) a_ptr->flags2 &= ~(TR2_SUST_INT);
		if (a_ptr->flags1 & TR1_WIS) a_ptr->flags2 &= ~(TR2_SUST_WIS);
		if (a_ptr->flags1 & TR1_DEX) a_ptr->flags2 &= ~(TR2_SUST_DEX);
		if (a_ptr->flags1 & TR1_CON) a_ptr->flags2 &= ~(TR2_SUST_CON);
		if (a_ptr->flags1 & TR1_CHR) a_ptr->flags2 &= ~(TR2_SUST_CHR);
		a_ptr->flags1 &= ~(TR1_BLOWS);
	}

	if (a_ptr->flags3 & TR3_LIGHT_CURSE) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags1 & TR1_KILL_DRAGON) a_ptr->flags1 &= ~(TR1_SLAY_DRAGON);
	if (a_ptr->flags1 & TR1_KILL_DEMON) a_ptr->flags1 &= ~(TR1_SLAY_DEMON);
	if (a_ptr->flags1 & TR1_KILL_UNDEAD) a_ptr->flags1 &= ~(TR1_SLAY_UNDEAD);
	if (a_ptr->flags3 & TR3_DRAIN_EXP) a_ptr->flags3 &= ~(TR3_HOLD_LIFE);
	if (a_ptr->flags3 & TR3_DRAIN_HP) a_ptr->flags3 &= ~(TR3_REGEN_HP);
	if (a_ptr->flags3 & TR3_DRAIN_MANA) a_ptr->flags3 &= ~(TR3_REGEN_MANA);

	if (!(a_ptr->flags4 & TR4_EVIL))
	{
		if (a_ptr->flags1 & TR1_SLAY_NATURAL) a_ptr->flags4 &= ~(TR4_ANIMAL);
		if (a_ptr->flags1 & TR1_SLAY_UNDEAD) a_ptr->flags4 &= ~(TR4_UNDEAD);
		if (a_ptr->flags1 & TR1_SLAY_DEMON) a_ptr->flags4 &= ~(TR4_DEMON);
		if (a_ptr->flags1 & TR1_SLAY_ORC) a_ptr->flags4 &= ~(TR4_ORC);
		if (a_ptr->flags1 & TR1_SLAY_TROLL) a_ptr->flags4 &= ~(TR4_TROLL);
		if (a_ptr->flags1 & TR1_SLAY_GIANT) a_ptr->flags4 &= ~(TR4_GIANT);
		if (a_ptr->flags1 & TR1_SLAY_DRAGON) a_ptr->flags4 &= ~(TR4_DRAGON);
		if (a_ptr->flags1 & TR1_KILL_DRAGON) a_ptr->flags4 &= ~(TR4_DRAGON);
		if (a_ptr->flags1 & TR1_KILL_DEMON) a_ptr->flags4 &= ~(TR4_DEMON);
		if (a_ptr->flags1 & TR1_KILL_UNDEAD) a_ptr->flags4 &= ~(TR4_UNDEAD);

		if (a_ptr->flags1 & TR1_SLAY_ORC) a_ptr->flags4 &= ~(TR4_SLAY_ELF);
		if (a_ptr->flags1 & TR1_SLAY_GIANT) a_ptr->flags4 &= ~(TR4_SLAY_DWARF);

		if (a_ptr->flags4 & TR4_SLAY_MAN) a_ptr->flags4 &= ~(TR4_MAN);
		if (a_ptr->flags4 & TR4_SLAY_ELF) a_ptr->flags4 &= ~(TR4_ELF);
		if (a_ptr->flags4 & TR4_SLAY_DWARF) a_ptr->flags4 &= ~(TR4_DWARF);

		if (a_ptr->flags1 & TR1_BRAND_HOLY) a_ptr->flags4 &= ~(TR4_EVIL);
	}

	if (a_ptr->flags1 & TR1_BRAND_POIS) a_ptr->flags4 &= ~(TR4_HURT_POIS);
	if (a_ptr->flags1 & TR1_BRAND_ACID) a_ptr->flags4 &= ~(TR4_HURT_ACID);
	if (a_ptr->flags1 & TR1_BRAND_ELEC) a_ptr->flags4 &= ~(TR4_HURT_ELEC);
	if (a_ptr->flags1 & TR1_BRAND_FIRE) a_ptr->flags4 &= ~(TR4_HURT_FIRE);
	if (a_ptr->flags1 & TR1_BRAND_COLD) a_ptr->flags4 &= ~(TR4_HURT_COLD);

	if (a_ptr->flags4 & TR4_BRAND_LITE) a_ptr->flags4 &= ~(TR4_HURT_LITE);

	if (a_ptr->flags3 & TR3_UNCONTROLLED) a_ptr->flags4 &= ~(TR4_ANCHOR);

}


#define ADD_POWER(string, val, flag, flgnum, extra) \
	if (a_ptr->flags##flgnum & flag) { \
		p += (val); \
		extra; \
		LOG_PRINT1("Modifying power for " string ", total is %d\n", p); \
	}


/*
 * Evaluate the artifact's overall power level.
 */
static s32b artifact_power(int a_idx)
{
	const artifact_type *a_ptr = &a_info[a_idx];
	s32b p = 0;
	s16b k_idx;
	object_kind *k_ptr;
	int immunities = 0;
	int sustains = 0;
	int low_resists = 0;
	int high_resists = 0;

	LOG_PRINT("********** ENTERING EVAL POWER ********\n");
	LOG_PRINT1("Artifact index is %d\n", a_idx);

	/* Try to use the cache */
	k_idx = kinds[a_idx];

	/* Lookup the item if not yet cached */
	if (!k_idx)
	{
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Cache the object index */
		kinds[a_idx] = k_idx;

		/* Paranoia */
		if (!k_idx)
		{
			quit_fmt("Illegal tval(%d)/sval(%d) value for artifact %d!", a_ptr->tval, a_ptr->sval,a_idx);
		}
	}

	k_ptr = &k_info[k_idx];

	if (a_idx >= ART_MIN_NORMAL)
	{
		LOG_PRINT1("Initial power level is %d\n", p);
	}

	/* Evaluate certain abilities based on type of object. */
	switch (a_ptr->tval)
	{
		case TV_BOW:
		{
			int mult;

			/*
			 * Add the average damage of fully enchanted (good) ammo for this
			 * weapon.  Could make this dynamic based on k_info if desired.
			 */

			if (a_ptr->sval == SV_SLING)
			{
				p += AVG_SLING_AMMO_DAMAGE;
			}
			else if (a_ptr->sval == SV_SHORT_BOW ||
				a_ptr->sval == SV_LONG_BOW)
			{
				p += AVG_BOW_AMMO_DAMAGE;
			}
			else if (a_ptr->sval == SV_HAND_XBOW ||
					 a_ptr->sval == SV_LIGHT_XBOW ||
					 a_ptr->sval == SV_HEAVY_XBOW)
			{
				p += AVG_XBOW_AMMO_DAMAGE;
			}

			LOG_PRINT1("Adding power from ammo, total is %d\n", p);

			mult = bow_multiplier(a_ptr->sval);
			LOG_PRINT1("Base multiplier for this weapon is %d\n", mult);

			if (a_ptr->to_h > 9)
			{
				p+= (a_ptr->to_h - 9) * 2 / 3;
				LOG_PRINT1("Adding power from high to_hit, total is %d\n", p);
			}
			else
			{
				p += 6;
				LOG_PRINT1("Adding power from base to_hit, total is %d\n", p);
			}

			if (a_ptr->flags1 & TR1_MIGHT)
			{
				if (a_ptr->pval > 3 || a_ptr->pval < 0)
				{
					p += 20000;	/* inhibit */
					mult = 1;	/* don't overflow */
				}
				else
				{
					mult += a_ptr->pval;
				}

				LOG_PRINT1("Extra might multiple is %d\n", mult);
			}
			p *= mult;

			LOG_PRINT2("Multiplying power by %d, total is %d\n", mult, p);

			/*
			 * Damage multiplier for bows should be weighted less than that
			 * for melee weapons, since players typically get fewer shots
			 * than hits (note, however, that the multipliers are applied
			 * afterwards in the bow calculation, not before as for melee
			 * weapons, which tends to bring these numbers back into line).
			 */

			if (a_ptr->to_d > 9)
			{
				p += a_ptr->to_d;
				LOG_PRINT1("Adding power from to_dam greater than 9, total is %d\n", p);
			}
			else
			{
				p += 9;
				LOG_PRINT1("Adding power for base to_dam, total is %d\n", p);
			}

			LOG_PRINT1("Adding power from to_dam, total is %d\n", p);

			if (a_ptr->flags1 & TR1_SHOTS)
			{
				/*
				 * Extra shots are calculated differently for bows than for
				 * slings or crossbows, because of rangers ... not any more CC 13/8/01
				 */

				LOG_PRINT1("Extra shots: %d\n", a_ptr->pval);

				if (a_ptr->pval > 3 || a_ptr->pval < 0)
				{
					p += 20000;	/* inhibit */
					LOG_PRINT("INHIBITING - more than 3 extra shots\n");
				}
				else if (a_ptr->pval > 0)
				{
					if (a_ptr->sval == SV_SHORT_BOW ||
						a_ptr->sval == SV_LONG_BOW)
					{
						p = (p * (1 + a_ptr->pval));
					}
					else
					{
						p = (p * (1 + a_ptr->pval));
					}
					LOG_PRINT2("Multiplying power by 1 + %d, total is %d\n", a_ptr->pval, p);
				}

			}

			/* Normalise power back */
			/* We now only count power as 'above' having the basic weapon at the same level */
			if (a_ptr->sval == SV_SLING)
			{
				if (ABS(p) > AVG_SLING_AMMO_DAMAGE)
					p -= sign(p) * AVG_SLING_AMMO_DAMAGE * bow_multiplier(k_ptr->sval);
				else
					p = 0;
			}
			else if (a_ptr->sval == SV_SHORT_BOW ||
					 a_ptr->sval == SV_LONG_BOW)
			{
				if (ABS(p) > AVG_BOW_AMMO_DAMAGE)
					p -= sign(p) * AVG_BOW_AMMO_DAMAGE * bow_multiplier(k_ptr->sval);
				else
					p = 0;
			}
			else if (a_ptr->sval == SV_HAND_XBOW ||
					 a_ptr->sval == SV_LIGHT_XBOW ||
					 a_ptr->sval == SV_HEAVY_XBOW)
			{
				if (ABS(p) > AVG_XBOW_AMMO_DAMAGE)
					p -= sign(p) * AVG_XBOW_AMMO_DAMAGE * bow_multiplier(k_ptr->sval);
				else
					p = 0;
			}

			if (a_ptr->weight < k_ptr->weight)
			{
				p++;
				LOG_PRINT("Incrementing power by one for low weight\n");
			}

			/*
			 * Correction to match ratings to melee damage ratings.
			 * We multiply all missile weapons by 1.5 in order to compare damage.
			 * (CR 11/20/01 - changed this to 1.25).
			 * Melee weapons assume 5 attacks per turn, so we must also divide
			 * by 5 to get equal ratings.
			 */

			if (a_ptr->sval == SV_SHORT_BOW ||
				a_ptr->sval == SV_LONG_BOW)
			{
				p = sign(p) * (ABS(p) / 4);
				LOG_PRINT1("Rescaling bow power, total is %d\n", p);
			}
			else
			{
				p = sign(p) * (ABS(p) / 4);
				LOG_PRINT1("Rescaling xbow/sling power, total is %d\n", p);
			}

			/* Ignore theft / acid / fire -- Slight bonus as may choose to swap bows */
			p += 1;
			LOG_PRINT1("Adding power for ignore theft / acid / fire, total is %d\n", p);

			break;
		}
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			/* Not this is 'uncorrected' */
			p += a_ptr->dd * (a_ptr->ds + 1);
			LOG_PRINT1("Adding power for dam dice, total is %d\n", p);

			/* Apply the correct slay multiplier */

			p = (p * slay_power(slay_index(a_ptr->flags1, a_ptr->flags2, a_ptr->flags3, a_ptr->flags4))) / tot_mon_power;
			LOG_PRINT1("Adjusted for slay power, total is %d\n", p);

			p /= 2;
			LOG_PRINT1("Correcting damage, total is %d\n", p);

			/* To hit uses a percentage - up to 65%*/
			if (a_ptr->to_h > 9)
			{
				p+= a_ptr->to_h * 2 / 3;
				LOG_PRINT1("Adding power for to_hit greater than 9, total is %d\n", p);
			}
			else
			{
				p+= 6;
				LOG_PRINT1("Adding power for base to_hit, total is %d\n", p);
			}

			if (a_ptr->to_d > 9)
			{
				p += a_ptr->to_d;
				LOG_PRINT1("Adding power for to_dam greater than 9, total is %d\n", p);
			}
			else
			{
				p+= 9;
				LOG_PRINT1("Adding power for base to_dam, total is %d\n", p);
			}


			if (a_ptr->flags1 & TR1_BLOWS)
			{
				LOG_PRINT1("Extra blows: %d\n", a_ptr->pval);
				if (a_ptr->pval > 3 || a_ptr->pval < 0)
				{
					p += 20000;	/* inhibit */
					LOG_PRINT("INHIBITING, more than 3 extra blows or a negative number\n");
				}
				else if (a_ptr->pval > 0)
				{
					p = sign(p) * ((ABS(p) * (5 + a_ptr->pval)) / 5);
					/* Add an extra +5 per blow to account for damage rings */
					/* (The +5 figure is a compromise here - could be adjusted) */
					p += 5 * a_ptr->pval;
					LOG_PRINT1("Adding power for blows, total is %d\n", p);
				}
			}

			/* Normalise power back */
			/* We remove the weapon base damage to get 'true' power */
			/* This makes e.g. a sword that provides fire immunity the same value as
			   a ring that provides fire immunity */
			if (ABS(p) > k_ptr->dd * (k_ptr->ds + 1) / 2 + 15)
				p -= sign(p) * (k_ptr->dd * (k_ptr->ds + 1) / 2 + 15);
			else
				p = 0;

			LOG_PRINT1("Normalising power against base item, total is %d\n", p);

			if (a_ptr->ac != k_ptr->ac)
			{
				p += a_ptr->ac - k_ptr->ac;
				LOG_PRINT1("Adding power for non-standard AC value, total is %d\n", p);
			}

			/* Remember, weight is in 0.1 lb. units. */
			if (a_ptr->weight != k_ptr->weight)
			{
			/*	p += (k_ptr->weight - a_ptr->weight) / 20; */
				LOG_PRINT1("Adding power for low weight, total is %d\n", p);
			}

			/* Ignore theft / acid / fire -- Bonus as may choose to swap melee weapons */
			p += 3;
			LOG_PRINT1("Adding power for ignore theft / acid / fire, total is %d\n", p);


			ADD_POWER("blessed",		 1, TR3_BLESSED, 3,);

			ADD_POWER("blood vampire",	 25, TR4_VAMP_HP, 4,);
			ADD_POWER("mana vampire",	 14, TR4_VAMP_MANA, 4,);
			break;
		}
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			if (a_ptr->ac != k_ptr->ac)
			{
				p += a_ptr->ac - k_ptr->ac;
			}
			LOG_PRINT1("Adding power for non-standard AC value, total is %d\n", p);

			p += sign(a_ptr->to_h) * ((ABS(a_ptr->to_h) * 2) / 3);
			LOG_PRINT1("Adding power for to_hit, total is %d\n", p);

			p += a_ptr->to_d * 2;
			LOG_PRINT1("Adding power for to_dam, total is %d\n", p);

			if (a_ptr->weight < k_ptr->weight)
			{
				p += (k_ptr->weight - a_ptr->weight) / 10;
				LOG_PRINT1("Adding power for low weight, total is %d\n", p);
			}

			/* Ignore theft / acid / fire -- Big bonus for ignore acid */
			p += 5;
			LOG_PRINT1("Adding power for ignore theft / *acid* / fire, total is %d\n", p);

			break;
		}
		case TV_LITE:
		{
			p += 10;
			LOG_PRINT("Artifact light source, adding 10 as base\n");

			p += sign(a_ptr->to_h) * ((ABS(a_ptr->to_h) * 2) / 3);
			LOG_PRINT1("Adding power for to_hit, total is %d\n", p);

			p += a_ptr->to_d * 2;
			LOG_PRINT1("Adding power for to_dam, total is %d\n", p);

			break;
		}
		case TV_RING:
		case TV_AMULET:
		{
			LOG_PRINT("Artifact jewellery, adding 0 as base\n");

			p += sign(a_ptr->to_h) * ((ABS(a_ptr->to_h) * 2) / 3);
			LOG_PRINT1("Adding power for to_hit, total is %d\n", p);

			p += a_ptr->to_d * 2;
			LOG_PRINT1("Adding power for to_dam, total is %d\n", p);

			/* Ignore theft -- Very slight bonus for ignore ignore */
			p += 1;
			LOG_PRINT1("Adding power for ignore theft, total is %d\n", p);

			break;
		}
	}

	/* Compute weight discount percentage */
	/*
	 * If an item weighs more than 5 pounds, we discount its power.
	 *
	 * This figure is from 30 lb weight limit for mages divided by 6 slots;
	 * but we do it for all items as they may be used as swap items, but use a divisor of 10 lbs instead.
	 */

	/* Evaluate ac bonus and weight differentl for armour and non-armour. */
	switch (a_ptr->tval)
	{
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			if (a_ptr->weight >= 50)
			{
				p -= a_ptr->weight / 50;

				LOG_PRINT2("Reducing power for armour weight of %d, total is %d\n", a_ptr->weight, p);
			}

			if (a_ptr->to_a > 9)
			{
				p+= (a_ptr->to_a - 9);
				LOG_PRINT2("Adding power for to_ac of %d, total is %d\n", a_ptr->to_a, p);
			}

			if (a_ptr->to_a > 19)
			{
				p += (a_ptr->to_a - 19);
				LOG_PRINT1("Adding power for high to_ac value, total is %d\n", p);
			}

			if (a_ptr->to_a > 29)
			{
				p += (a_ptr->to_a - 29);
				LOG_PRINT1("Adding power for very high to_ac value, total is %d\n", p);
			}

			if (a_ptr->to_a > 39)
			{
				p += 20000;	/* inhibit */
				LOG_PRINT("INHIBITING: AC bonus too high\n");
			}
			break;

		default:
			if (a_ptr->weight >= 100)
			{
				p -= a_ptr->weight / 100;

				LOG_PRINT2("Reducing power for swap item weight of %d, total is %d\n", a_ptr->weight, p);

			}

			p += sign(a_ptr->to_a) * (ABS(a_ptr->to_a) / 2);

			if (a_ptr->to_a > 9)
			{
				p+= (a_ptr->to_a - 9);
				LOG_PRINT2("Adding power for high to_ac of %d, total is %d\n", a_ptr->to_a, p);

			}

			if (a_ptr->to_a > 19)
			{
				p += (a_ptr->to_a - 19);
				LOG_PRINT1("Adding power for very high to_ac value, total is %d\n", p);
			}

			if (a_ptr->to_a > 29)
			{
				p += 20000;	/* inhibit */
				LOG_PRINT("INHIBITING: AC bonus too high\n");
			}
			break;
		}
	}

	/* Other abilities are evaluated independent of the object type. */
	if (a_ptr->pval > 0)
	{
		if (a_ptr->flags1 & TR1_STR)
		{
			p += 2 * a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for STR bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_INT)
		{
			p += a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for INT bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_WIS)
		{
			p += a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for WIS bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_DEX)
		{
			p += 2 * a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for DEX+AGI bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_CON)
		{
			p += 2 * a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for CON bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_CHR)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 2;
			LOG_PRINT2("Adding power for CHR bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_SAVE)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 2;
			LOG_PRINT2("Adding power for save bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_DEVICE)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 3;
			LOG_PRINT2("Adding power for device bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_STEALTH)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 2;
			LOG_PRINT2("Adding power for stealth bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_TUNNEL)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 4; /* Can't get lower */
			LOG_PRINT2("Adding power for tunnel bonus %d, total is %d\n", a_ptr->pval, p);
		}
		if (a_ptr->flags1 & TR1_SEARCH)
		{
			p += 1 + a_ptr->pval * a_ptr->pval / 4; /* Can't get lower */
			LOG_PRINT2("Adding power for searching bonus %d, total is %d\n", a_ptr->pval , p);
		}
		if (a_ptr->flags3 & TR3_REGEN_HP)
		{
			p += 3 * a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for regenerate hit points bonus %d, total is %d\n", a_ptr->pval , p);
		}
		if (a_ptr->flags3 & TR3_REGEN_MANA)
		{
			p += 2 * a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for regenerate mana bonus bonus %d, total is %d\n", a_ptr->pval , p);
		}
		if (a_ptr->flags3 & TR3_LITE)
		{
			p += a_ptr->pval * a_ptr->pval;
			LOG_PRINT2("Adding power for lite bonus %d, total is %d\n", a_ptr->pval , p);
		}

	}
	else if (a_ptr->pval < 0)	/* hack: don't give large negatives */
	{
		if (a_ptr->flags1 & TR1_STR) p += 4 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += 2 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += 2 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += 4 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += 4 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_CHR) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_SAVE) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEVICE) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += 2 * a_ptr->pval;
		if (a_ptr->flags1 & TR1_TUNNEL) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_SEARCH) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_INFRA) p += a_ptr->pval;
		if (a_ptr->flags3 & TR3_REGEN_HP) p += 6 * a_ptr->pval;
		if (a_ptr->flags3 & TR3_REGEN_MANA) p += 4 * a_ptr->pval;
		if (a_ptr->flags3 & TR3_LITE) p += 3 * a_ptr->pval;
		LOG_PRINT1("Subtracting power for negative ability values, total is %d\n", p);
	}
	if (a_ptr->flags1 & TR1_SPEED)
	{
		p += 3 * a_ptr->pval * a_ptr->pval;
		LOG_PRINT2("Adding power for speed bonus/penalty %d, total is %d\n", a_ptr->pval, p);
	}

	ADD_POWER("sustain STR",	 5, TR2_SUST_STR, 2,sustains++);
	ADD_POWER("sustain INT",	 2, TR2_SUST_INT, 2,sustains++);
	ADD_POWER("sustain WIS",	 2, TR2_SUST_WIS, 2,sustains++);
	ADD_POWER("sustain DEX",	 4, TR2_SUST_DEX, 2,sustains++);
	ADD_POWER("sustain CON",	 3, TR2_SUST_CON, 2,sustains++);
	ADD_POWER("sustain CHR",	 1, TR2_SUST_CHR, 2,sustains++);

	/* Add bonus for sustains getting 'sustain-lock' */
	if (sustains > 1)
	{
		p += (sustains -1);
		LOG_PRINT1("Adding power for multiple sustains, total is %d\n", p); \
	}

	ADD_POWER("acid immunity",	17, TR2_IM_ACID, 2, immunities++);
	ADD_POWER("elec immunity",	22, TR2_IM_ELEC, 2, immunities++);
	ADD_POWER("fire immunity",	22, TR2_IM_FIRE, 2, immunities++);
	ADD_POWER("cold immunity",	17, TR2_IM_COLD, 2, immunities++);
	ADD_POWER("poison immunity",	17, TR4_IM_POIS, 4, immunities++);

	if (immunities > 1)
	{
		p += 15;
		LOG_PRINT1("Adding power for multiple immunities, total is %d\n", p);
	}
	if (immunities > 2)
	{
		p += 45;
		LOG_PRINT1("Adding power for three or more immunities, total is %d\n", p);
	}
	if (immunities > 3)
	{
		p += 20000;		/* inhibit */
		LOG_PRINT("INHIBITING: Too many immunities\n");
	}

	low_resists += immunities;

	ADD_POWER("free action",	 7, TR3_FREE_ACT, 3, high_resists++);
	ADD_POWER("hold life",		 6, TR3_HOLD_LIFE, 3, high_resists++);
	ADD_POWER("feather fall",	 1, TR3_FEATHER, 3,); /* was 2 */

	ADD_POWER("see invisible",	 4, TR3_SEE_INVIS, 3,);
	ADD_POWER("sense orcs",	  	3, TR3_ESP_ORC, 3,);
	ADD_POWER("sense trolls",	3, TR3_ESP_TROLL, 3,);
	ADD_POWER("sense giants",	3, TR3_ESP_GIANT, 3,);
	ADD_POWER("sense demons",	4, TR3_ESP_DEMON, 3,);
	ADD_POWER("sense undead",	5, TR3_ESP_UNDEAD, 3,);
	ADD_POWER("sense dragons",       5, TR3_ESP_DRAGON, 3,);
	ADD_POWER("sense nature",	4, TR3_ESP_NATURE, 3,);
	ADD_POWER("telepathy",	  18, TR3_TELEPATHY, 3,);
	ADD_POWER("slow digestion",	 1, TR3_SLOW_DIGEST, 3,);

	ADD_POWER("resist acid",	 2, TR2_RES_ACID, 2, low_resists++);
	ADD_POWER("resist elec",	 3, TR2_RES_ELEC, 2, low_resists++);
	ADD_POWER("resist fire",	 3, TR2_RES_FIRE, 2, low_resists++);
	ADD_POWER("resist cold",	 3, TR2_RES_COLD, 2, low_resists++);

	/* Add bonus for getting 'low resist-lock' */
	if (low_resists > 1)
	{
		p += (low_resists-1) * 2;
		LOG_PRINT1("Adding power for multiple low resists, total is %d\n", p); \
	}

	ADD_POWER("resist poison",	14, TR2_RES_POIS, 2, high_resists++);
	ADD_POWER("resist light",	 3, TR2_RES_LITE, 2, high_resists++);
	ADD_POWER("resist dark",	 8, TR2_RES_DARK, 2, high_resists++);
	ADD_POWER("resist blindness",	 8, TR2_RES_BLIND, 2, high_resists++);
	ADD_POWER("resist confusion",	12, TR2_RES_CONFU, 2, high_resists++);
	ADD_POWER("resist sound",	 7, TR2_RES_SOUND, 2, high_resists++);
	ADD_POWER("resist shards",	 4, TR2_RES_SHARD, 2, high_resists++);
	ADD_POWER("resist nexus",	 5, TR2_RES_NEXUS, 2, high_resists++);
	ADD_POWER("resist nether",	10, TR2_RES_NETHR, 2, high_resists++);
	ADD_POWER("resist chaos",	10, TR2_RES_CHAOS, 2, high_resists++);
	ADD_POWER("resist disenchantment", 10, TR2_RES_DISEN, 2, high_resists++);
	ADD_POWER("resist disease",	10, TR4_RES_DISEASE, 4, high_resists++);
	ADD_POWER("resist water",	5, TR4_RES_WATER, 4, high_resists++);

	/* Add bonus for getting 'high resist-lock' */
	if (high_resists > 1)
	{
		p += (high_resists-1) * 3;
		LOG_PRINT1("Adding power for multiple high resists, total is %d\n", p); \
	}

	ADD_POWER("uncontrolled activation",	 -40, TR3_UNCONTROLLED, 3,);
	ADD_POWER("drain experience",	 -20, TR3_DRAIN_EXP, 3,);

	ADD_POWER("drain health",	 -20, TR3_DRAIN_HP, 3,);
	ADD_POWER("drain mana",	 	 -10, TR3_DRAIN_MANA, 3,);
	ADD_POWER("aggravation",	 -15, TR3_AGGRAVATE, 3,);
	ADD_POWER("light curse",	 -1,  TR3_LIGHT_CURSE, 3,);
	ADD_POWER("heavy curse",	 -4, TR3_HEAVY_CURSE, 3,);
/*	ADD_POWER("permanent curse",	 -40, TR3_PERMA_CURSE, 3,);*/
	ADD_POWER("light vulnerability", -30, TR4_HURT_LITE, 4,);
	ADD_POWER("water vulnerability", -30, TR4_HURT_WATER, 4,);
	ADD_POWER("hunger",	 	 -15, TR3_HUNGER, 3,);
	ADD_POWER("anchor",	 	 -1, TR4_ANCHOR, 4,);
	ADD_POWER("silent",	 	 -20, TR4_SILENT, 4,);
	ADD_POWER("static",	 	 -15, TR4_STATIC, 4,);
	ADD_POWER("windy",	 	 -15, TR4_WINDY, 4,);
	ADD_POWER("animal",	 	 -5, TR4_ANIMAL, 4,);
	ADD_POWER("evil",	 	 -5, TR4_EVIL, 4,);
	ADD_POWER("undead",	 	 -30, TR4_UNDEAD, 4,);
	ADD_POWER("demon",	 	 -10, TR4_DEMON, 4,);
	ADD_POWER("orc",	 	 -3, TR4_ORC, 4,);
	ADD_POWER("troll",	 	 -3, TR4_TROLL, 4,);
	ADD_POWER("giant",	 	 -5, TR4_GIANT, 4,);
	ADD_POWER("dragon",	 	 -10, TR4_DRAGON, 4,);
	ADD_POWER("man",	 	 -3, TR4_MAN, 4,);
	ADD_POWER("dwarf",	 	 -3, TR4_DWARF, 4,);
	ADD_POWER("elf",	 	 -3, TR4_ELF, 4,);
	ADD_POWER("poison vulnerability",	 -50, TR4_HURT_POIS, 4,);
	ADD_POWER("acid vulnerability",	 	 -30, TR4_HURT_ACID, 4,);
	ADD_POWER("lightning vulnerability",	 -40, TR4_HURT_ELEC, 4,);
	ADD_POWER("fire vulnerability",	 -40, TR4_HURT_FIRE, 4,);
	ADD_POWER("cold vulnerability",	 -40, TR4_HURT_COLD, 4,);

	return (p);

}


/*
 * Store the original artifact power ratings as a baseline
 */

static void store_base_power (void)
{
	int i;
	artifact_type *a_ptr;
	object_kind *k_ptr;
	s16b k_idx;

	for(i = 0; i < z_info->a_max; i++)
	{
		a_ptr = &a_info[i];
		remove_contradictory(a_ptr);
		base_power[i] = artifact_power(i);
		a_ptr->power = base_power[i];
	}

	for(i = 0; i < z_info->a_max; i++)
	{
		/* Kinds array was populated in the above step */
		k_idx = kinds[i];
		k_ptr = &k_info[k_idx];
		a_ptr = &a_info[i];
		base_item_level[i] = k_ptr->level;
		base_item_rarity[i] = k_ptr->chance[0];
		base_art_rarity[i] = a_ptr->rarity;
	}

	/* Store the number of different types, for use later */

	for (i = 0; i < z_info->a_max; i++)
	{
		switch (a_info[i].tval)
		{
			case TV_SWORD:
			case TV_POLEARM:
			case TV_HAFTED:
				art_melee_total++; break;
			case TV_BOW:
				art_bow_total++; break;
			case TV_SOFT_ARMOR:
			case TV_HARD_ARMOR:
			case TV_DRAG_ARMOR:
				art_armor_total++; break;
			case TV_SHIELD:
				art_shield_total++; break;
			case TV_CLOAK:
				if (a_info[i].sval <= SV_SHADOW_CLOAK) art_cloak_total++;
				else art_armor_total++; break;
			case TV_HELM:
			case TV_CROWN:
				art_headgear_total++; break;
			case TV_GLOVES:
				art_glove_total++; break;
			case TV_BOOTS:
				art_boot_total++; break;
			default:
				art_other_total++;
		}
	}
	art_total = art_melee_total + art_bow_total + art_armor_total +
		art_shield_total + art_cloak_total + art_headgear_total +
		art_glove_total + art_boot_total + art_other_total;
}

static struct item_choice {
	int threshold;
	int tval;
	const char *report;
} item_choices[] = {
	{  6, TV_BOW,		"a missile weapon"},
	{  9, TV_DIGGING,	"a digger"},
	{ 19, TV_HAFTED,	"a blunt weapon"},
	{ 33, TV_SWORD,		"an edged weapon"},
	{ 42, TV_POLEARM,	"a polearm"},
	{ 64, TV_SOFT_ARMOR,	"body armour"},
	{ 71, TV_BOOTS,		"footwear"},
	{ 78, TV_GLOVES,	"gloves"},
	{ 87, TV_HELM,		"a hat"},
	{ 94, TV_SHIELD,	"a shield"},
	{100, TV_CLOAK,		"a cloak"}
};

/*
 * Randomly select a base item type (tval,sval).  Assign the various fields
 * corresponding to that choice.
 *
 * The return value gives the index of the new item type.  The method is
 * passed a pointer to a rarity value in order to return the rarity of the
 * new item.
 */
static s16b choose_item(int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	int tval, sval=0;
	object_kind *k_ptr;
	int r, i;
	s16b k_idx, r2;
	byte target_level;

	/*
	 * Look up the original artifact's base object kind to get level.
	 */
	k_idx = kinds[a_idx];
	k_ptr = &k_info[k_idx];
	target_level = base_item_level[a_idx];
	LOG_PRINT1("Base item level is: %d\n", target_level);


	/*
	 * If the artifact level is higher then we use that instead.  Note that
	 * we can get away with reusing the artifact rarities here since we
	 * don't change them.  If artifact rarities are changed then the
	 * original values will need to be stored, as for base items.
	 */

	if(a_ptr->level > target_level) target_level = a_ptr->level;
	LOG_PRINT1("Target level is: %d\n", target_level);


	/*
	 * Pick a category (tval) of weapon randomly.  Within each tval, roll
	 * an sval (specific item) based on the target level.  The number we
	 * roll should be a bell curve.  The mean and standard variation of the
	 * bell curve are based on the target level; the distribution of
	 * kinds versus the bell curve is hand-tweaked. :-(
	 * CR 8/11/01 - reworked some of these lists
	 */
	r = rand_int(100);
	r2 = Rand_normal(target_level * 2, target_level);
	LOG_PRINT2("r is: %d, r2 is: %d\n", r, r2);


	i = 0;
	while (r >= item_choices[i].threshold)
	{
		i++;
	}

	tval = item_choices[i].tval;
	LOG_PRINT(format("Creating item: %s\n", item_choices[i].report));


	switch (tval)
	{
	case TV_BOW:
		if (r2 < 15) sval = SV_SLING;
		else if (r2 < 30) sval = SV_SHORT_BOW;
		else if (r2 < 60) sval = SV_HAND_XBOW;
		else if (r2 < 90) sval = SV_LONG_BOW;
		else if (r2 < 120) sval = SV_LIGHT_XBOW;
		else sval = SV_HEAVY_XBOW;
		break;

	case TV_DIGGING:
		if (r2 < 15) sval = SV_SHOVEL;
		else if (r2 < 30) sval = SV_PICK;
		else if (r2 < 60) sval = SV_GNOMISH_SHOVEL;
		else if (r2 < 90) sval = SV_ORCISH_PICK;
		else if (r2 < 120) sval = SV_DWARVEN_SHOVEL;
		else sval = SV_DWARVEN_PICK;
		break;

	case TV_HAFTED:
		if (r2 < 3) sval = SV_WHIP;
		else if (r2 < 5) sval = SV_BATON;
		else if (r2 < 8) sval = SV_MACE;
		else if (r2 < 15) sval = SV_WAR_HAMMER;
		else if (r2 < 22) sval = SV_QUARTERSTAFF;
		else if (r2 < 28) sval = SV_LUCERN_HAMMER;
		else if (r2 < 35) sval = SV_MORNING_STAR;
		else if (r2 < 45) sval = SV_FLAIL;
		else if (r2 < 60) sval = SV_LEAD_FILLED_MACE;
		else if (r2 < 80) sval = SV_BALL_AND_CHAIN;
		else if (r2 < 100) sval = SV_TWO_HANDED_MACE;
		else if (r2 < 130) sval = SV_TWO_HANDED_FLAIL;
		else sval = SV_MACE_OF_DISRUPTION;
		break;

	case TV_SWORD:
		if (r2 < -5) sval = SV_BROKEN_DAGGER;
		else if (r2 < 0) sval = SV_BROKEN_SWORD;
		else if (r2 < 4) sval = SV_DAGGER;
		else if (r2 < 8) sval = SV_MAIN_GAUCHE;
		else if (r2 < 12) sval = SV_RAPIER;	/* or at least pointy ;-) */
		else if (r2 < 15) sval = SV_SMALL_SWORD;
		else if (r2 < 18) sval = SV_SHORT_SWORD;
		else if (r2 < 22) sval = SV_SABRE;
		else if (r2 < 26) sval = SV_CUTLASS;
		else if (r2 < 30) sval = SV_TULWAR;
		else if (r2 < 35) sval = SV_BROAD_SWORD;
		else if (r2 < 41) sval = SV_LONG_SWORD;
		else if (r2 < 45) sval = SV_SCIMITAR;
		else if (r2 < 51) sval = SV_BASTARD_SWORD;
		else if (r2 < 64) sval = SV_KATANA;
		else if (r2 < 90) sval = SV_TWO_HANDED_SWORD;
		else if (r2 < 120) sval = SV_EXECUTIONERS_SWORD;
		else sval = SV_BLADE_OF_CHAOS;
		break;

	case TV_POLEARM:
		if (r2 < 3) sval = SV_DART;
		else if (r2 < 8) sval = SV_JAVELIN;
		else if (r2 < 17) sval = SV_SPEAR;
		else if (r2 < 22) sval = SV_TWO_HANDED_SPEAR;
		else if (r2 < 30) sval = SV_AWL_PIKE;
		else if (r2 < 37) sval = SV_PIKE;
		else if (r2 < 45) sval = SV_BEAKED_AXE;
		else if (r2 < 54) sval = SV_BROAD_AXE;
		else if (r2 < 64) sval = SV_BATTLE_AXE;
		else if (r2 < 75) sval = SV_GLAIVE;
		else if (r2 < 83) sval = SV_TRIDENT;
		else if (r2 < 91) sval = SV_HALBERD;
		else if (r2 < 96) sval = SV_LANCE;
		else if (r2 < 106) sval = SV_GREAT_AXE;
		else if (r2 < 115) sval = SV_SCYTHE;
		else if (r2 < 130) sval = SV_LOCHABER_AXE;
		else sval = SV_SCYTHE_OF_SLICING;
		break;

	case TV_SOFT_ARMOR:
		/* Hack - multiply r2 by 3/2 (armor has deeper base levels than other types) */
		r2 = sign(r2) * ((ABS(r2) * 3) / 2);

		/* Adjust tval, as all armour is done together */
		if (r2 < 30) tval = TV_SOFT_ARMOR;
		else if (r2 < 37) tval = TV_CLOAK;
		else if (r2 < 106) tval = TV_HARD_ARMOR;
		else tval = TV_DRAG_ARMOR;

		/* Soft stuff. */
		if (r2 < 0) sval = SV_FILTHY_RAG;
		else if (r2 < 5) sval = SV_ROBE;
		else if (r2 < 10) sval = SV_SOFT_LEATHER_ARMOR;
		else if (r2 < 15) sval = SV_SOFT_STUDDED_LEATHER;
		else if (r2 < 20) sval = SV_HARD_LEATHER_ARMOR;
		else if (r2 < 25) sval = SV_HARD_STUDDED_LEATHER;
		else if (r2 < 30) sval = SV_LEATHER_SCALE_MAIL;

		/* Cloak-like armour */
		else if (r2 < 33) sval = SV_RIVETED_LEATHER_COAT;
		else if (r2 < 37) sval = SV_CHAIN_MAIL_COAT;

		/* Hard stuff. */
		else if (r2 < 39) sval = SV_RUSTY_CHAIN_MAIL;
		else if (r2 < 43) sval = SV_METAL_SCALE_MAIL;
		else if (r2 < 46) sval = SV_CHAIN_MAIL;
		else if (r2 < 49) sval = SV_AUGMENTED_CHAIN_MAIL;
		else if (r2 < 54) sval = SV_DOUBLE_CHAIN_MAIL;
		else if (r2 < 59) sval = SV_BAR_CHAIN_MAIL;
		else if (r2 < 64) sval = SV_METAL_BRIGANDINE_ARMOUR;
		else if (r2 < 69) sval = SV_PARTIAL_PLATE_ARMOUR;
		else if (r2 < 74) sval = SV_METAL_LAMELLAR_ARMOUR;
		else if (r2 < 80) sval = SV_FULL_PLATE_ARMOUR;
		else if (r2 < 85) sval = SV_RIBBED_PLATE_ARMOUR;
		else if (r2 < 92) sval = SV_MITHRIL_CHAIN_MAIL;
		else if (r2 < 99) sval = SV_MITHRIL_PLATE_MAIL;
		else if (r2 < 106) sval = SV_ADAMANTITE_PLATE_MAIL;

		/* DSM - CC 18/8/01 */
		else if (r2 < 111) sval = SV_DRAGON_BLACK;
		else if (r2 < 116) sval = SV_DRAGON_BLUE;
		else if (r2 < 121) sval = SV_DRAGON_WHITE;
		else if (r2 < 126) sval = SV_DRAGON_RED;
		else if (r2 < 132) sval = SV_DRAGON_GREEN;
		else if (r2 < 138) sval = SV_DRAGON_MULTIHUED;
		else if (r2 < 144) sval = SV_DRAGON_ETHER;
		else if (r2 < 152) sval = SV_DRAGON_LAW;
		else if (r2 < 159) sval = SV_DRAGON_BRONZE;
		else if (r2 < 166) sval = SV_DRAGON_GOLD;
		else if (r2 < 174) sval = SV_DRAGON_CHAOS;
		else if (r2 < 183) sval = SV_DRAGON_BALANCE;
		else sval = SV_DRAGON_POWER;
		break;

	case TV_BOOTS:
		if (r2 < 15) sval = SV_PAIR_OF_SOFT_LEATHER_BOOTS;
		else if (r2 < 60) sval = SV_PAIR_OF_HARD_LEATHER_BOOTS;
		else sval = SV_PAIR_OF_METAL_SHOD_BOOTS;
		break;

	case TV_GLOVES:
		if (r2 < 15) sval = SV_SET_OF_LEATHER_GLOVES;
		else if (r2 < 60) sval = SV_SET_OF_GAUNTLETS;
		else sval = SV_SET_OF_CESTI;
		break;

	case TV_HELM:
		/* Adjust tval to handle crowns and helms here */
		if (r2 < 50) tval = TV_HELM; else tval = TV_CROWN;

		if (r2 < 9) sval = SV_HARD_LEATHER_CAP;
		else if (r2 < 20) sval = SV_METAL_CAP;
		else if (r2 < 40) sval = SV_IRON_HELM;
		else if (r2 < 50) sval = SV_STEEL_HELM;

		else if (r2 < 70) sval = SV_IRON_CROWN;
		else if (r2 < 90) sval = SV_GOLDEN_CROWN;
		else sval = SV_JEWELED_CROWN;
		break;

	case TV_SHIELD:
		if (r2 < 15) sval = SV_SMALL_LEATHER_SHIELD;
		else if (r2 < 40) sval = SV_SMALL_METAL_SHIELD;
		else if (r2 < 70) sval = SV_LARGE_LEATHER_SHIELD;
		else if (r2 < 120) sval = SV_LARGE_METAL_SHIELD;
		else sval = SV_SHIELD_OF_DEFLECTION;
		break;

	case TV_CLOAK:
		if (r2 < 30) sval = SV_CLOAK;
		else if (r2 < 50) sval = SV_TABARD;
		else if (r2 < 100) sval = SV_LEATHER_COAT;
		else sval = SV_SHADOW_CLOAK;
		break;
	}

	k_idx = lookup_kind(tval, sval);
	k_ptr = &k_info[k_idx];
	kinds[a_idx] = k_idx;

	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;
	a_ptr->pval = k_ptr->pval;
	a_ptr->to_h = k_ptr->to_h;
	a_ptr->to_d = k_ptr->to_d;
	a_ptr->to_a = k_ptr->to_a;
	a_ptr->ac = k_ptr->ac;
	a_ptr->dd = k_ptr->dd;
	a_ptr->ds = k_ptr->ds;
	a_ptr->weight = k_ptr->weight;
	a_ptr->flags1 = k_ptr->flags1;
	a_ptr->flags2 = k_ptr->flags2;
	a_ptr->flags3 = k_ptr->flags3;
	a_ptr->flags4 = k_ptr->flags4;

	/*
	 * Dragon armor: remove activation flag.  This is because the current
	 * code doesn't support standard DSM activations for artifacts very
	 * well.  If it gets an activation from the base artifact it will be
	 * reset later.
	 */
	if (a_ptr->tval == TV_DRAG_ARMOR)
	{
		a_ptr->flags3 &= ~TR3_ACTIVATE;
		a_ptr->pval = 0;
	}

	/* Artifacts ignore everything */
	a_ptr->flags2 |= TR2_IGNORE_MASK;

	/* Assign basic stats to the artifact based on its artifact level. */
	/*
	 * CR, 2001-09-03: changed to a simpler version to match the hit-dam
	 * parsing algorithm. We use random ranges averaging mean_hit_startval
	 * and mean_dam_startval, but permitting variation of 50% to 150%.
	 * Level-dependent term has been removed for the moment.
	 */
	switch (a_ptr->tval)
	{
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_SWORD:
		case TV_POLEARM:
			a_ptr->to_h += (s16b)(mean_hit_startval / 2 +
				randint( mean_hit_startval ) );
			a_ptr->to_d += (s16b)(mean_dam_startval / 2 +
				randint( mean_dam_startval ) );
			LOG_PRINT2("Assigned basic stats, to_hit: %d, to_dam: %d\n", a_ptr->to_h, a_ptr->to_d);
			break;
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
			/* CR: adjusted this to work with parsing logic */
			a_ptr->to_a += (s16b)(mean_ac_startval / 2 +
				randint( mean_ac_startval ) );

			LOG_PRINT1("Assigned basic stats, AC bonus: %d\n", a_ptr->to_a);

			break;
	}

	/* Done - return the index of the new object kind. */
	return k_idx;
}


/*
 * We've just added an ability which uses the pval bonus.  Make sure it's
 * not zero.  If it's currently negative, leave it negative (heh heh).
 */
static void do_pval(artifact_type *a_ptr)
{
	int factor = 1;
	/* Track whether we have blows, might or shots on this item */
	if (a_ptr->flags1 & TR1_BLOWS) factor++;
	if (a_ptr->flags1 & TR1_MIGHT) factor++;
	if (a_ptr->flags1 & TR1_SHOTS) factor++;

	if (a_ptr->pval == 0)
	{
		/* Blows, might, shots handled separately */
		if (factor > 1)
		{
			a_ptr->pval = (s16b)(1 + rand_int(2));
			/* Give it a shot at +3 */
			if (rand_int(INHIBIT_STRONG) == 0) a_ptr->pval = 3;
		}
		else a_ptr->pval = (s16b)(1 + rand_int(5));
		LOG_PRINT1("Assigned initial pval, value is: %d\n", a_ptr->pval);
	}
	else if (a_ptr->pval < 0)
	{
		if (rand_int(2) == 0)
		{
			a_ptr->pval--;
			LOG_PRINT1("Decreasing pval by 1, new value is: %d\n", a_ptr->pval);
		}
	}
	else if (rand_int(a_ptr->pval * factor) == 0)
	{
		/*
		 * CR: made this a bit rarer and diminishing with higher pval -
		 * also rarer if item has blows/might/shots already
		 */
		a_ptr->pval++;
		LOG_PRINT1("Increasing pval by 1, new value is: %d\n", a_ptr->pval);
	}
}


/*
 * Adjust the parsed frequencies for any peculiarities of the
 * algorithm.  For example, if stat bonuses and sustains are
 * being added in a correlated fashion, it will tend to push
 * the frequencies up for both of them.  In this method we
 * compensate for cases like this by applying corrective
 * scaling.
 */

static void adjust_freqs()
{
	/*
	 * Enforce minimum values for any frequencies that might potentially
	 * be missing in the standard set, especially supercharged ones.
	 * Numbers here represent the average number of times this ability
	 * would appear if the entire randart set was eligible to receive
	 * it (so in the case of a bow ability: if the set was all bows).
	 *
	 * Note that low numbers here for very specialized abilities could
	 * mean that there's a good chance this ability will not appear in
	 * a given randart set.  If this is a problem, raise the number.
	 */
	if (artprobs[ART_IDX_GEN_RFEAR] < 5)
		artprobs[ART_IDX_GEN_RFEAR] = 5;
	if (artprobs[ART_IDX_MELEE_DICE_SUPER] < 5)
		artprobs[ART_IDX_MELEE_DICE_SUPER] = 5;
	if (artprobs[ART_IDX_BOW_SHOTS_SUPER] < 5)
		artprobs[ART_IDX_BOW_SHOTS_SUPER] = 5;
	if (artprobs[ART_IDX_BOW_MIGHT_SUPER] < 5)
		artprobs[ART_IDX_BOW_MIGHT_SUPER] = 5;
	if (artprobs[ART_IDX_GEN_SPEED_SUPER] < 2)
		artprobs[ART_IDX_GEN_SPEED_SUPER] = 2;
	if (artprobs[ART_IDX_GEN_AC] < 5)
		artprobs[ART_IDX_GEN_AC] = 5;
	if (artprobs[ART_IDX_GEN_TUNN] < 5)
		artprobs[ART_IDX_GEN_TUNN] = 5;

	/* Cut aggravation frequencies in half since they're used twice */
	artprobs[ART_IDX_NONWEAPON_AGGR] /= 2;
	artprobs[ART_IDX_WEAPON_AGGR] /= 2;
}

/*
 * Parse the list of artifacts and count up the frequencies of the various
 * abilities.  This is used to give dynamic generation probabilities.
 */

static void parse_frequencies ()
{
	int i;
	const artifact_type *a_ptr;
	object_kind *k_ptr;
	s32b temp;
	s16b k_idx;


	LOG_PRINT("\n****** BEGINNING GENERATION OF FREQUENCIES\n\n");

	/* Zero the frequencies */

	for(i = 0; i < ART_IDX_TOTAL; i++)
	{
		artprobs[i] = 0;
	}

	/* Go through the list of all artifacts */

	for(i = 0; i < z_info->a_max; i++)
	{

		LOG_PRINT1("Current artifact index is %d\n", i);

		a_ptr = &a_info[i];

		/* Special cases -- don't parse these! */
		if ((i == ART_POWER) ||
			(i == ART_GROND) ||
			(i == ART_MORGOTH))
			continue;

		/* Also don't parse cursed items */
		if (base_power[i] < 0) continue;

		/* Get a pointer to the base item for this artifact */
		k_idx = kinds[i];
		k_ptr = &k_info[k_idx];

		/* Count up the abilities for this artifact */

		if (a_ptr->tval == TV_BOW)
		{
			if (a_ptr->flags1 & TR1_SHOTS)
			{
				/* Do we have 3 or more extra shots? (Unlikely) */
				if(a_ptr->pval > 2)
				{
					LOG_PRINT("Adding 1 for supercharged shots (3 or more!)\n");

					(artprobs[ART_IDX_BOW_SHOTS_SUPER])++;
				}
				else {
					LOG_PRINT("Adding 1 for extra shots\n");

					(artprobs[ART_IDX_BOW_SHOTS])++;
				}
			}

			if (a_ptr->flags1 & TR1_MIGHT)
			{
				/* Do we have 3 or more extra might? (Unlikely) */
				if(a_ptr->pval > 2)
				{
					LOG_PRINT("Adding 1 for extra shots\n");

					(artprobs[ART_IDX_BOW_MIGHT_SUPER])++;
				}
				else {
					LOG_PRINT("Adding 1 for extra might\n");

					(artprobs[ART_IDX_BOW_MIGHT])++;
				}
			}

			if (a_ptr->flags3 & 0x00300F00)
			{
				/* We have some sensing - count them */
				temp = 0;
				if (a_ptr->flags3 & TR3_ESP_ORC) temp++;
				if (a_ptr->flags3 & TR3_ESP_TROLL) temp++;
				if (a_ptr->flags3 & TR3_ESP_GIANT) temp++;
				if (a_ptr->flags3 & TR3_ESP_DRAGON) temp++;
				if (a_ptr->flags3 & TR3_ESP_DEMON) temp++;
				if (a_ptr->flags3 & TR3_ESP_UNDEAD) temp++;
				if (a_ptr->flags3 & TR3_ESP_NATURE) temp++;

				/* Add these to the frequency count */
				artprobs[ART_IDX_BOW_SENSE] += temp;

				LOG_PRINT1("Adding %d instances of sensing for bows\n", temp);
			}

			if (a_ptr->flags4 & 0x07FF0000)
			{
				/* We have some restrictions - count them */
				temp = 0;
				if (a_ptr->flags4 & TR4_ANIMAL) temp++;
				if (a_ptr->flags4 & TR4_EVIL) temp++;
				if (a_ptr->flags4 & TR4_UNDEAD) temp++;
				if (a_ptr->flags4 & TR4_DEMON) temp++;
				if (a_ptr->flags4 & TR4_ORC) temp++;
				if (a_ptr->flags4 & TR4_TROLL) temp++;
				if (a_ptr->flags4 & TR4_GIANT) temp++;
				if (a_ptr->flags4 & TR4_DRAGON) temp++;
				if (a_ptr->flags4 & TR4_MAN) temp++;
				if (a_ptr->flags4 & TR4_DWARF) temp++;
				if (a_ptr->flags4 & TR4_ELF) temp++;

				/* Add these to the frequency count */
				artprobs[ART_IDX_BOW_RESTRICT] += temp;

				LOG_PRINT1("Adding %d restrictions for bows\n", temp);
			}
		}

		/* Handle hit / dam ratings - are they higher than normal? */
		/* Also handle other weapon/nonweapon abilities */
		if (a_ptr->tval == TV_BOW || a_ptr->tval == TV_DIGGING ||
			a_ptr->tval == TV_HAFTED || a_ptr->tval == TV_POLEARM ||
			a_ptr->tval == TV_SWORD)
		{
			if (a_ptr->to_h - k_ptr->to_h - mean_hit_startval > 0)
			{
				temp = (a_ptr->to_d - k_ptr->to_d - mean_dam_startval) /
					mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Adding %d instances of extra to-hit bonus for weapon\n", temp);

					(artprobs[ART_IDX_WEAPON_HIT]) += temp;
				}
			}
			else if (a_ptr->to_h - k_ptr->to_h - mean_hit_startval < 0)
			{
				temp = ( -(a_ptr->to_d - k_ptr->to_d - mean_dam_startval) ) /
					mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Subtracting %d instances of extra to-hit bonus for weapon\n", temp);

					(artprobs[ART_IDX_WEAPON_HIT]) -= temp;
				}
			}
			if (a_ptr->to_d - k_ptr->to_d - mean_dam_startval > 0)
			{
				temp = (a_ptr->to_d - k_ptr->to_d - mean_dam_startval) /
					mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Adding %d instances of extra to-dam bonus for weapon\n", temp);

					(artprobs[ART_IDX_WEAPON_DAM]) += temp;
				}
			}
			else if (a_ptr->to_d - k_ptr->to_d - mean_dam_startval < 0)
			{
				temp = ( -(a_ptr->to_d - k_ptr->to_d - mean_dam_startval)) /
					mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Subtracting %d instances of extra to-dam bonus for weapon\n", temp);

					(artprobs[ART_IDX_WEAPON_DAM]) -= temp;
				}
			}

			/* Aggravation */
			if (a_ptr->flags3 & TR3_AGGRAVATE)
			{
				LOG_PRINT("Adding 1 for aggravation - weapon\n");
				(artprobs[ART_IDX_WEAPON_AGGR])++;
			}

			/* End weapon stuff */
		}
		else
		{
			if (a_ptr->to_h - k_ptr->to_h > 0)
			{
				temp = (a_ptr->to_d - k_ptr->to_d) / mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Adding %d instances of extra to-hit bonus for non-weapon\n", temp);

					(artprobs[ART_IDX_NONWEAPON_HIT]) += temp;
				}
			}
			if (a_ptr->to_d - k_ptr->to_d > 0)
			{
				temp = (a_ptr->to_d - k_ptr->to_d) / mean_dam_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Adding %d instances of extra to-dam bonus for non-weapon\n", temp);

					(artprobs[ART_IDX_NONWEAPON_DAM]) += temp;
				}
			}
			/* Aggravation */
			if (a_ptr->flags3 & TR3_AGGRAVATE)
			{
				LOG_PRINT("Adding 1 for aggravation - nonweapon\n");
				(artprobs[ART_IDX_NONWEAPON_AGGR])++;
			}

			if (a_ptr->flags3 & 0x00300F00)
			{
				/* We have some sensing - count them */
				temp = 0;
				if (a_ptr->flags3 & TR3_ESP_ORC) temp++;
				if (a_ptr->flags3 & TR3_ESP_TROLL) temp++;
				if (a_ptr->flags3 & TR3_ESP_GIANT) temp++;
				if (a_ptr->flags3 & TR3_ESP_DRAGON) temp++;
				if (a_ptr->flags3 & TR3_ESP_DEMON) temp++;
				if (a_ptr->flags3 & TR3_ESP_UNDEAD) temp++;
				if (a_ptr->flags3 & TR3_ESP_NATURE) temp++;

				LOG_PRINT1("Adding %d instances of sensing for nonweapons\n", temp);

				/* Add these to the frequency count */
				artprobs[ART_IDX_NONWEAPON_SENSE] += temp;
			}

			if ((a_ptr->flags1 & 0x00FF0000) || (a_ptr->flags4 & 0x07FF0E00))
			{
				/* We have some restrictions - count them */
				/* Note that the only reason slay flags are on non-weapons is for restrictions */

				temp = 0;
				if (a_ptr->flags1 & TR1_SLAY_NATURAL) temp++;
				if (a_ptr->flags1 & TR1_SLAY_UNDEAD) temp++;
				if (a_ptr->flags1 & TR1_BRAND_HOLY) temp++;
				if (a_ptr->flags1 & TR1_SLAY_DEMON) temp++;
				if (a_ptr->flags1 & TR1_SLAY_ORC) temp++;
				if (a_ptr->flags1 & TR1_SLAY_TROLL) temp++;
				if (a_ptr->flags1 & TR1_SLAY_GIANT) temp++;
				if (a_ptr->flags1 & TR1_SLAY_DRAGON) temp++;
				if (a_ptr->flags4 & TR4_SLAY_MAN) temp++;
				if (a_ptr->flags4 & TR4_SLAY_ELF) temp++;
				if (a_ptr->flags4 & TR4_SLAY_DWARF) temp++;
				if (a_ptr->flags4 & TR4_ANIMAL) temp++;
				if (a_ptr->flags4 & TR4_EVIL) temp++;
				if (a_ptr->flags4 & TR4_UNDEAD) temp++;
				if (a_ptr->flags4 & TR4_DEMON) temp++;
				if (a_ptr->flags4 & TR4_ORC) temp++;
				if (a_ptr->flags4 & TR4_TROLL) temp++;
				if (a_ptr->flags4 & TR4_GIANT) temp++;
				if (a_ptr->flags4 & TR4_DRAGON) temp++;
				if (a_ptr->flags4 & TR4_MAN) temp++;
				if (a_ptr->flags4 & TR4_DWARF) temp++;
				if (a_ptr->flags4 & TR4_ELF) temp++;

				LOG_PRINT1("Adding %d restrictions for nonweapons\n", temp);

				/* Add these to the frequency count */
				artprobs[ART_IDX_NONWEAPON_RESTRICT] += temp;
			}
		}

		if (a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_HAFTED ||
			a_ptr->tval == TV_POLEARM || a_ptr->tval == TV_SWORD)
		{
			/* Blessed weapon Y/N */

			if(a_ptr->flags3 & TR3_BLESSED)
			{
				LOG_PRINT("Adding 1 for blessed weapon\n");

				(artprobs[ART_IDX_MELEE_BLESS])++;
			}

			/*
			 * Brands or slays - count all together
			 * We will need to add something here unless the weapon has
			 * nothing at all
			 */

			if ((a_ptr->flags1 & 0xFFFF0000) || (a_ptr->flags4 & 0x00000E03))
			{
				/* We have some brands or slays - count them */

				temp = 0;
				if (a_ptr->flags1 & TR1_KILL_DRAGON) temp++;
				if (a_ptr->flags1 & TR1_KILL_DEMON) temp++;
				if (a_ptr->flags1 & TR1_KILL_UNDEAD) temp++;
				if (a_ptr->flags1 & TR1_SLAY_NATURAL) temp++;
				if (a_ptr->flags1 & TR1_SLAY_UNDEAD) temp++;
				if (a_ptr->flags1 & TR1_SLAY_DRAGON) temp++;
				if (a_ptr->flags1 & TR1_SLAY_DEMON) temp++;
				if (a_ptr->flags1 & TR1_SLAY_TROLL) temp++;
				if (a_ptr->flags1 & TR1_SLAY_ORC) temp++;
				if (a_ptr->flags1 & TR1_SLAY_GIANT) temp++;
				if (a_ptr->flags1 & TR1_BRAND_HOLY) temp++;
				if (a_ptr->flags1 & TR1_BRAND_POIS) temp++;
				if (a_ptr->flags1 & TR1_BRAND_ACID) temp++;
				if (a_ptr->flags1 & TR1_BRAND_ELEC) temp++;
				if (a_ptr->flags1 & TR1_BRAND_FIRE) temp++;
				if (a_ptr->flags1 & TR1_BRAND_COLD) temp++;

				if (a_ptr->flags4 & TR4_BRAND_LITE) temp++;
				if (a_ptr->flags4 & TR4_BRAND_DARK) temp++;

				if (a_ptr->flags4 & TR4_SLAY_MAN) temp++;
				if (a_ptr->flags4 & TR4_SLAY_ELF) temp++;
				if (a_ptr->flags4 & TR4_SLAY_DWARF) temp++;

				LOG_PRINT1("Adding %d for slays and brands\n", temp);

				/* Add these to the frequency count */
				artprobs[ART_IDX_MELEE_BRAND_SLAY] += temp;
			}

			if (a_ptr->flags3 & 0x00300F00)
			{
				/* We have some sensing - count them */
				temp = 0;
				if (a_ptr->flags3 & TR3_ESP_ORC) temp++;
				if (a_ptr->flags3 & TR3_ESP_TROLL) temp++;
				if (a_ptr->flags3 & TR3_ESP_GIANT) temp++;
				if (a_ptr->flags3 & TR3_ESP_DRAGON) temp++;
				if (a_ptr->flags3 & TR3_ESP_DEMON) temp++;
				if (a_ptr->flags3 & TR3_ESP_UNDEAD) temp++;
				if (a_ptr->flags3 & TR3_ESP_NATURE) temp++;

				LOG_PRINT1("Adding %d instances of sensing for melee weapons\n", temp);

				/* Add these to the frequency count */
				artprobs[ART_IDX_MELEE_SENSE] += temp;
			}

			if (a_ptr->flags4 & 0x07FF0000)
			{
				/* We have some restrictions - count them */
				temp = 0;
				if (a_ptr->flags4 & TR4_ANIMAL) temp++;
				if (a_ptr->flags4 & TR4_EVIL) temp++;
				if (a_ptr->flags4 & TR4_UNDEAD) temp++;
				if (a_ptr->flags4 & TR4_DEMON) temp++;
				if (a_ptr->flags4 & TR4_ORC) temp++;
				if (a_ptr->flags4 & TR4_TROLL) temp++;
				if (a_ptr->flags4 & TR4_GIANT) temp++;
				if (a_ptr->flags4 & TR4_DRAGON) temp++;
				if (a_ptr->flags4 & TR4_MAN) temp++;
				if (a_ptr->flags4 & TR4_DWARF) temp++;
				if (a_ptr->flags4 & TR4_ELF) temp++;

				LOG_PRINT1("Adding %d restrictions for melee weapons\n", temp);

				/* Add these to the frequency count */
				artprobs[ART_IDX_MELEE_RESTRICT] += temp;
			}

			/* See invisible? */
			if(a_ptr->flags3 & TR3_SEE_INVIS)
			{
				LOG_PRINT("Adding 1 for see invisible (weapon case)\n");

				(artprobs[ART_IDX_MELEE_SINV])++;
			}

			/* Does this weapon have extra blows? */
			if (a_ptr->flags1 & TR1_BLOWS)
			{
				LOG_PRINT("Adding 1 for extra blows\n");

				(artprobs[ART_IDX_MELEE_BLOWS])++;
			}

			/* Does this weapon have an unusual bonus to AC? */
			if ( (a_ptr->to_a - k_ptr->to_a) > 0)
			{
				temp = (a_ptr->to_a - k_ptr->to_a) / mean_ac_increment;
				if (temp > 0)
				{
					LOG_PRINT1("Adding %d instances of extra AC bonus for weapon\n", temp);

					(artprobs[ART_IDX_MELEE_AC]) += temp;
				}
			}

			/* Check damage dice - are they more than normal? */
			if (a_ptr->dd > k_ptr->dd)
			{
				/* Difference of 3 or more? */
				if ( (a_ptr->dd - k_ptr->dd) > 2)
				{
					LOG_PRINT("Adding 1 for super-charged damage dice!\n");

					(artprobs[ART_IDX_MELEE_DICE_SUPER])++;
				}
				else
				{
					LOG_PRINT("Adding 1 for extra damage dice.\n");

					(artprobs[ART_IDX_MELEE_DICE])++;
				}
			}

			/* Check weight - is it different from normal? */
			if (a_ptr->weight != k_ptr->weight)
			{
				LOG_PRINT("Adding 1 for unusual weight.\n");

				(artprobs[ART_IDX_MELEE_WEIGHT])++;
			}

			/* Check for tunnelling ability */
			if (a_ptr->flags1 & TR1_TUNNEL)
			{
				LOG_PRINT("Adding 1 for tunnelling bonus.\n");

				(artprobs[ART_IDX_MELEE_TUNN])++;
			}

			/* End of weapon-specific stuff */
		}
		else
		{
			/* Check for tunnelling ability */
			if (a_ptr->flags1 & TR1_TUNNEL)
			{
				LOG_PRINT("Adding 1 for tunnelling bonus - general.\n");

				(artprobs[ART_IDX_GEN_TUNN])++;
			}

		}
		/*
		 * Count up extra AC bonus values.
		 * Could also add logic to subtract for lower values here, but it's
		 * probably not worth the trouble since it's so rare.
		 */

		if ( (a_ptr->to_a - k_ptr->to_a - mean_ac_startval) > 0)
		{
			temp = (a_ptr->to_a - k_ptr->to_a - mean_ac_startval) /
				mean_ac_increment;
			if (temp > 0)
			{
				if (a_ptr->tval == TV_BOOTS)
				{
					LOG_PRINT1("Adding %d for AC bonus - boots\n", temp);
					(artprobs[ART_IDX_BOOT_AC]) += temp;
				}
				else if (a_ptr->tval == TV_GLOVES)
				{
					LOG_PRINT1("Adding %d for AC bonus - gloves\n", temp);
					(artprobs[ART_IDX_GLOVE_AC]) += temp;
				}
				else if (a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)
				{
					LOG_PRINT1("Adding %d for AC bonus - headgear\n", temp);
					(artprobs[ART_IDX_HELM_AC]) += temp;
				}
				else if (a_ptr->tval == TV_SHIELD)
				{
					LOG_PRINT1("Adding %d for AC bonus - shield\n", temp);
					(artprobs[ART_IDX_SHIELD_AC]) += temp;
				}
				else if (a_ptr->tval == TV_CLOAK)
				{
					LOG_PRINT1("Adding %d for AC bonus - cloak\n", temp);
					(artprobs[ART_IDX_CLOAK_AC]) += temp;
				}
				else if (a_ptr->tval == TV_SOFT_ARMOR || a_ptr->tval == TV_HARD_ARMOR ||
					a_ptr->tval == TV_DRAG_ARMOR)
				{
					LOG_PRINT1("Adding %d for AC bonus - body armor\n", temp);
					(artprobs[ART_IDX_ARMOR_AC]) += temp;
				}
				else
				{
					LOG_PRINT1("Adding %d for AC bonus - general\n", temp);
					(artprobs[ART_IDX_GEN_AC]) += temp;
				}
			}
		}

		/* Generic armor abilities */
		if ( a_ptr->tval == TV_BOOTS || a_ptr->tval == TV_GLOVES ||
			a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN ||
			a_ptr->tval == TV_SHIELD || a_ptr->tval == TV_CLOAK ||
			a_ptr->tval == TV_SOFT_ARMOR || a_ptr->tval == TV_HARD_ARMOR ||
			a_ptr->tval == TV_DRAG_ARMOR)
		{
			/* Check weight - is it different from normal? */
			if (a_ptr->weight != k_ptr->weight)
			{
				LOG_PRINT("Adding 1 for unusual weight.\n");

				(artprobs[ART_IDX_ALLARMOR_WEIGHT])++;
			}

			/* Done with generic armor abilities */
		}

		/*
		 * General abilities.  This section requires a bit more work
		 * than the others, because we have to consider cases where
		 * a certain ability might be found in a particular item type.
		 * For example, ESP is commonly found on headgear, so when
		 * we count ESP we must add it to either the headgear or
		 * general tally, depending on the base item.  This permits
		 * us to have general abilities appear more commonly on a
		 * certain item type.
		 */

		if ( (a_ptr->flags1 & TR1_STR) || (a_ptr->flags1 & TR1_INT) ||
			(a_ptr->flags1 & TR1_WIS) || (a_ptr->flags1 & TR1_DEX) ||
			(a_ptr->flags1 & TR1_CON) || (a_ptr->flags1 & TR1_CHR) )
		{
			/* Stat bonus case.  Add up the number of individual bonuses */

			temp = 0;
			if (a_ptr->flags1 & TR1_STR) temp++;
			if (a_ptr->flags1 & TR1_INT) temp++;
			if (a_ptr->flags1 & TR1_WIS) temp++;
			if (a_ptr->flags1 & TR1_DEX) temp++;
			if (a_ptr->flags1 & TR1_CON) temp++;
			if (a_ptr->flags1 & TR1_CHR) temp++;

			/* Handle a few special cases separately. */

			if((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
				((a_ptr->flags1 & TR1_WIS) || (a_ptr->flags1 & TR1_INT)))
			{
				/* Handle WIS and INT on helms and crowns */
				if(a_ptr->flags1 & TR1_WIS)
				{
					LOG_PRINT("Adding 1 for WIS bonus on headgear.\n");

					(artprobs[ART_IDX_HELM_WIS])++;
					/* Counted this one separately so subtract it here */
					temp--;
				}
				if(a_ptr->flags1 & TR1_INT)
				{
					LOG_PRINT("Adding 1 for INT bonus on headgear.\n");

					(artprobs[ART_IDX_HELM_INT])++;
					/* Counted this one separately so subtract it here */
					temp--;
				}
			}
			else if ((a_ptr->tval == TV_SOFT_ARMOR ||
				a_ptr->tval == TV_HARD_ARMOR ||
				a_ptr->tval == TV_DRAG_ARMOR) && a_ptr->flags1 & TR1_CON)
			{
				/* Handle CON bonus on armor */
				LOG_PRINT("Adding 1 for CON bonus on body armor.\n");

				(artprobs[ART_IDX_ARMOR_CON])++;
				/* Counted this one separately so subtract it here */
				temp--;
			}
			else if ((a_ptr->tval == TV_GLOVES) && (a_ptr->flags1 & TR1_DEX))
			{
				/* Handle DEX bonus on gloves */
				LOG_PRINT("Adding 1 for DEX bonus on gloves.\n");

				(artprobs[ART_IDX_GLOVE_DEX])++;
				/* Counted this one separately so subtract it here */
				temp--;
			}

			/* Now the general case */

			if (temp > 0)
			{
				/* There are some bonuses that weren't handled above */
				LOG_PRINT1("Adding %d for stat bonuses - general.\n", temp);

				(artprobs[ART_IDX_GEN_STAT]) += temp;

			/* Done with stat bonuses */
			}
		}

		if ( (a_ptr->flags2 & TR2_SUST_STR) || (a_ptr->flags2 & TR2_SUST_INT) ||
			(a_ptr->flags2 & TR2_SUST_WIS) || (a_ptr->flags2 & TR2_SUST_DEX) ||
			(a_ptr->flags2 & TR2_SUST_CON) || (a_ptr->flags2 & TR2_SUST_CHR) )
		{
			/* Now do sustains, in a similar manner */
			temp = 0;
			if (a_ptr->flags2 & TR2_SUST_STR) temp++;
			if (a_ptr->flags2 & TR2_SUST_INT) temp++;
			if (a_ptr->flags2 & TR2_SUST_WIS) temp++;
			if (a_ptr->flags2 & TR2_SUST_DEX) temp++;
			if (a_ptr->flags2 & TR2_SUST_CON) temp++;
			if (a_ptr->flags2 & TR2_SUST_CHR) temp++;

			LOG_PRINT1("Adding %d for stat sustains.\n", temp);

			(artprobs[ART_IDX_GEN_SUST]) += temp;
		}

		if (a_ptr->flags1 & TR1_SAVE)
		{
			/* General case */
			LOG_PRINT("Adding 1 for spell resistance bonus - general.\n");

			(artprobs[ART_IDX_GEN_SAVE])++;

			/* Done with magical devices */
		}

		if (a_ptr->flags1 & TR1_DEVICE)
		{
			/* General case */
			LOG_PRINT("Adding 1 for magical device bonus - general.\n");

			(artprobs[ART_IDX_GEN_DEVICE])++;

			/* Done with magical devices */
		}


		if (a_ptr->flags1 & TR1_STEALTH)
		{
			/* Handle stealth, including a couple of special cases */
			if(a_ptr->tval == TV_BOOTS)
			{
				LOG_PRINT("Adding 1 for stealth bonus on boots.\n");

				(artprobs[ART_IDX_BOOT_STEALTH])++;
			}
			else if (a_ptr->tval == TV_CLOAK)
			{
				LOG_PRINT("Adding 1 for stealth bonus on cloak.\n");

				(artprobs[ART_IDX_CLOAK_STEALTH])++;
			}
			else if (a_ptr->tval == TV_SOFT_ARMOR ||
				a_ptr->tval == TV_HARD_ARMOR || a_ptr->tval == TV_DRAG_ARMOR)
			{
				LOG_PRINT("Adding 1 for stealth bonus on armor.\n");

				(artprobs[ART_IDX_ARMOR_STEALTH])++;
			}
			else
			{
				/* General case */
				LOG_PRINT("Adding 1 for stealth bonus - general.\n");

				(artprobs[ART_IDX_GEN_STEALTH])++;
			}
			/* Done with stealth */
		}

		if (a_ptr->flags1 & TR1_SEARCH)
		{
			/* Handle searching bonus - fully generic this time */
			LOG_PRINT("Adding 1 for search bonus - general.\n");

			(artprobs[ART_IDX_GEN_SEARCH])++;
		}

		if (a_ptr->flags1 & TR1_INFRA)
		{
			/* Handle infravision bonus - fully generic */
			LOG_PRINT("Adding 1 for infravision bonus - general.\n");

			(artprobs[ART_IDX_GEN_INFRA])++;
		}

		if (a_ptr->flags1 & TR1_SPEED)
		{
			/*
			 * Speed - boots handled separately.
			 * This is something of a special case in that we use the same
			 * frequency for the supercharged value and the normal value.
			 * We get away with this by using a somewhat lower average value
			 * for the supercharged ability than in the basic set (around
			 * +4 or +5 - c.f. Ringil and the others at +7 and upwards).
			 * This then allows us to add an equal number of
			 * small bonuses around +3 or so without unbalancing things.
			 */

			if (a_ptr->pval > 6)
			{
				/* Supercharge case */
				LOG_PRINT("Adding 1 for supercharged speed bonus!\n");

				(artprobs[ART_IDX_GEN_SPEED_SUPER])++;
			}
			else if(a_ptr->tval == TV_BOOTS)
			{
				/* Handle boots separately */
				LOG_PRINT("Adding 1 for normal speed bonus on boots.\n");

				(artprobs[ART_IDX_BOOT_SPEED])++;
			}
			else
			{
				LOG_PRINT("Adding 1 for normal speed bonus - general.\n");

				(artprobs[ART_IDX_GEN_SPEED])++;
			}
			/* Done with speed */
		}

		if ( (a_ptr->flags2 & TR2_IM_ACID) || (a_ptr->flags2 & TR2_IM_ELEC) ||
			(a_ptr->flags2 & TR2_IM_FIRE) || (a_ptr->flags2 & TR2_IM_COLD) ||
			(a_ptr->flags4 & TR4_IM_POIS))
		{
			/* Count up immunities for this item, if any */
			temp = 0;
			if (a_ptr->flags2 & TR2_IM_ACID) temp++;
			if (a_ptr->flags2 & TR2_IM_ELEC) temp++;
			if (a_ptr->flags2 & TR2_IM_FIRE) temp++;
			if (a_ptr->flags2 & TR2_IM_COLD) temp++;
			if (a_ptr->flags4 & TR4_IM_POIS) temp++;

			LOG_PRINT1("Adding %d for immunities.\n", temp);

			(artprobs[ART_IDX_GEN_IMMUNE]) += temp;
		}

		if (a_ptr->flags3 & TR3_FREE_ACT)
		{
			/* Free action - handle gloves separately */
			if(a_ptr->tval == TV_GLOVES)
			{
				LOG_PRINT("Adding 1 for free action on gloves.\n");

				(artprobs[ART_IDX_GLOVE_FA])++;
			}
			else
			{
				LOG_PRINT("Adding 1 for free action - general.\n");

				(artprobs[ART_IDX_GEN_FA])++;
			}
		}

		if (a_ptr->flags3 & TR3_HOLD_LIFE)
		{
			/* Hold life - do body armor separately */
			if( (a_ptr->tval == TV_SOFT_ARMOR) || (a_ptr->tval == TV_HARD_ARMOR) ||
				(a_ptr->tval == TV_DRAG_ARMOR))
			{
				LOG_PRINT("Adding 1 for hold life on armor.\n");

				(artprobs[ART_IDX_ARMOR_HLIFE])++;
			}
			else
			{
				LOG_PRINT("Adding 1 for hold life - general.\n");

				(artprobs[ART_IDX_GEN_HLIFE])++;
			}
		}

		if (a_ptr->flags3 & TR3_FEATHER)
		{
			/* Feather fall - handle boots separately */
			if(a_ptr->tval == TV_BOOTS)
			{
				LOG_PRINT("Adding 1 for feather fall on boots.\n");

				(artprobs[ART_IDX_BOOT_FEATHER])++;
			}
			else
			{
				LOG_PRINT("Adding 1 for feather fall - general.\n");

				(artprobs[ART_IDX_GEN_FEATHER])++;
			}
		}

		if (a_ptr->flags3 & TR3_LITE)
		{
			/* Handle permanent light */
			LOG_PRINT("Adding 1 for permanent light - general.\n");

			(artprobs[ART_IDX_GEN_LITE])++;
		}

		if (a_ptr->flags3 & TR3_SEE_INVIS)
		{
			/*
			 * Handle see invisible - do helms / crowns separately
			 * (Weapons were done already so exclude them)
			 */
			if( !(a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_HAFTED ||
			a_ptr->tval == TV_POLEARM || a_ptr->tval == TV_SWORD))
			{
				if (a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)
				{
					LOG_PRINT("Adding 1 for see invisible - headgear.\n");

					(artprobs[ART_IDX_HELM_SINV])++;
				}
				else
				{
					LOG_PRINT("Adding 1 for see invisible - general.\n");

					(artprobs[ART_IDX_GEN_SINV])++;
				}
			}
		}

		if (a_ptr->flags3 & TR3_TELEPATHY)
		{
			/* ESP case.  Handle helms/crowns separately. */
			if(a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)
			{
				LOG_PRINT("Adding 1 for ESP on headgear.\n");

				(artprobs[ART_IDX_HELM_ESP])++;
			}
			else
			{
				LOG_PRINT("Adding 1 for ESP - general.\n");

				(artprobs[ART_IDX_GEN_ESP])++;
			}
		}

		if (a_ptr->flags3 & TR3_SLOW_DIGEST)
		{
			/* Slow digestion case - generic. */
			LOG_PRINT("Adding 1 for slow digestion - general.\n");

			(artprobs[ART_IDX_GEN_SDIG])++;
		}

		if (a_ptr->flags3 & TR3_REGEN_HP)
		{
			/* Regeneration case - generic. */
			LOG_PRINT("Adding 1 for regeneration - general.\n");

			(artprobs[ART_IDX_GEN_REGEN])++;
		}

		if (a_ptr->flags3 & TR3_REGEN_MANA)
		{
			/* Regeneration case - generic. */
			LOG_PRINT("Adding 1 for regeneration - general.\n");

			(artprobs[ART_IDX_GEN_REGEN])++;
		}

		if ( (a_ptr->flags2 & TR2_RES_ACID) || (a_ptr->flags2 & TR2_RES_ELEC) ||
			(a_ptr->flags2 & TR2_RES_FIRE) || (a_ptr->flags2 & TR2_RES_COLD) )
		{
			/* Count up low resists (not the type, just the number) */
			temp = 0;
			if (a_ptr->flags2 & TR2_RES_ACID) temp++;
			if (a_ptr->flags2 & TR2_RES_ELEC) temp++;
			if (a_ptr->flags2 & TR2_RES_FIRE) temp++;
			if (a_ptr->flags2 & TR2_RES_COLD) temp++;

			/* Shields treated separately */
			if (a_ptr->tval == TV_SHIELD)
			{
				LOG_PRINT1("Adding %d for low resists on shield.\n", temp);

				(artprobs[ART_IDX_SHIELD_LRES]) += temp;
			}
			else if (a_ptr->tval == TV_SOFT_ARMOR ||
				a_ptr->tval == TV_HARD_ARMOR || a_ptr->tval == TV_DRAG_ARMOR)
			{
				/* Armor also treated separately */
				if (temp == 4)
				{
					/* Special case: armor has all four low resists */
					LOG_PRINT("Adding 1 for ALL LOW RESISTS on body armor.\n");

					(artprobs[ART_IDX_ARMOR_ALLRES])++;
				}
				else
				{
					/* Just tally up the resists as usual */
					LOG_PRINT1("Adding %d for low resists on body armor.\n", temp);

					(artprobs[ART_IDX_ARMOR_LRES]) += temp;
				}
			}
			else
			{
				/* General case */
				LOG_PRINT1("Adding %d for low resists - general.\n", temp);

				(artprobs[ART_IDX_GEN_LRES]) += temp;
			}

			/* Done with low resists */
		}

		/*
		 * If the item is body armor then count up all the high resists before
		 * going through them individually.  High resists are an important
		 * component of body armor so we track probability for them separately.
		 * The proportions of the high resists will be determined by the
		 * generic frequencies - this number just tracks the total.
		 */
		if (a_ptr->tval == TV_SOFT_ARMOR ||
			a_ptr->tval == TV_HARD_ARMOR || a_ptr->tval == TV_DRAG_ARMOR)
		{
			temp = 0;
			if (a_ptr->flags2 & TR2_RES_POIS) temp++;
			if (a_ptr->flags2 & TR2_RES_FEAR) temp++;
			if (a_ptr->flags2 & TR2_RES_LITE) temp++;
			if (a_ptr->flags2 & TR2_RES_DARK) temp++;
			if (a_ptr->flags2 & TR2_RES_BLIND) temp++;
			if (a_ptr->flags2 & TR2_RES_CONFU) temp++;
			if (a_ptr->flags2 & TR2_RES_SOUND) temp++;
			if (a_ptr->flags2 & TR2_RES_SHARD) temp++;
			if (a_ptr->flags2 & TR2_RES_NEXUS) temp++;
			if (a_ptr->flags2 & TR2_RES_NETHR) temp++;
			if (a_ptr->flags2 & TR2_RES_CHAOS) temp++;
			if (a_ptr->flags2 & TR2_RES_DISEN) temp++;
			if (a_ptr->flags4 & TR4_RES_DISEASE) temp++;
			if (a_ptr->flags4 & TR4_RES_WATER) temp++;

			LOG_PRINT1("Adding %d for high resists on body armor.\n", temp);

			(artprobs[ART_IDX_ARMOR_HRES]) += temp;
		}

		/* Now do the high resists individually */
		if (a_ptr->flags2 & TR2_RES_POIS)
		{
			/* Resist poison ability */
			LOG_PRINT("Adding 1 for resist poison - general.\n");

			(artprobs[ART_IDX_GEN_RPOIS])++;
		}

		if (a_ptr->flags2 & TR2_RES_FEAR)
		{
			/* Resist fear ability */
			LOG_PRINT("Adding 1 for resist fear - general.\n");

			(artprobs[ART_IDX_GEN_RFEAR])++;
		}

		if (a_ptr->flags2 & TR2_RES_LITE)
		{
			/* Resist light ability */
			LOG_PRINT("Adding 1 for resist light - general.\n");

			(artprobs[ART_IDX_GEN_RLITE])++;
		}

		if (a_ptr->flags2 & TR2_RES_DARK)
		{
			/* Resist dark ability */
			LOG_PRINT("Adding 1 for resist dark - general.\n");

			(artprobs[ART_IDX_GEN_RDARK])++;
		}

		if (a_ptr->flags2 & TR2_RES_BLIND)
		{
			/* Resist blind ability - helms/crowns are separate */
			if(a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)
			{
				LOG_PRINT("Adding 1 for resist blindness - headgear.\n");

				(artprobs[ART_IDX_HELM_RBLIND])++;
			}
			else
			{
				/* General case */
				LOG_PRINT("Adding 1 for resist blindness - general.\n");

				(artprobs[ART_IDX_GEN_RBLIND])++;
			}
		}

		if (a_ptr->flags2 & TR2_RES_CONFU)
		{
			/* Resist confusion ability */
			LOG_PRINT("Adding 1 for resist confusion - general.\n");

			(artprobs[ART_IDX_GEN_RCONF])++;
		}

		if (a_ptr->flags2 & TR2_RES_SOUND)
		{
			/* Resist sound ability */
			LOG_PRINT("Adding 1 for resist sound - general.\n");

			(artprobs[ART_IDX_GEN_RSOUND])++;
		}

		if (a_ptr->flags2 & TR2_RES_SHARD)
		{
			/* Resist shards ability */
			LOG_PRINT("Adding 1 for resist shards - general.\n");

			(artprobs[ART_IDX_GEN_RSHARD])++;
		}

		if (a_ptr->flags2 & TR2_RES_NEXUS)
		{
			/* Resist nexus ability */
			LOG_PRINT("Adding 1 for resist nexus - general.\n");

			(artprobs[ART_IDX_GEN_RNEXUS])++;
		}

		if (a_ptr->flags2 & TR2_RES_NETHR)
		{
			/* Resist nether ability */
			LOG_PRINT("Adding 1 for resist nether - general.\n");

			(artprobs[ART_IDX_GEN_RNETHER])++;
		}

		if (a_ptr->flags2 & TR2_RES_CHAOS)
		{
			/* Resist chaos ability */
			LOG_PRINT("Adding 1 for resist chaos - general.\n");

			(artprobs[ART_IDX_GEN_RCHAOS])++;
		}

		if (a_ptr->flags2 & TR2_RES_DISEN)
		{
			/* Resist disenchantment ability */
			LOG_PRINT("Adding 1 for resist disenchantment - general.\n");

			(artprobs[ART_IDX_GEN_RDISEN])++;
		}

		if (a_ptr->flags4 & TR4_RES_DISEASE)
		{
			/* Resist disenchantment ability */
			LOG_PRINT("Adding 1 for resist disease - general.\n");

			(artprobs[ART_IDX_GEN_RDISEA])++;
		}

		/* Done with parsing of frequencies for this item */
	}
	/* End for loop */

	if(randart_verbose)
	{
	/* Print out some of the abilities, to make sure that everything's fine */
		for(i=0; i<ART_IDX_TOTAL; i++)
		{
			fprintf(randart_log, "Frequency of ability %d: %d\n", i, artprobs[i]);
		}
	}

	/*
	 * Rescale the abilities so that dependent / independent abilities are
	 * comparable.  We do this by rescaling the frequencies for item-dependent
	 * abilities as though the entire set was made up of that item type.  For
	 * example, if one bow out of three has extra might, and there are 120
	 * artifacts in the full set, we rescale the frequency for extra might to
	 * 40 (if we had 120 randart bows, about 40 would have extra might).
	 *
	 * This will allow us to compare the frequencies of all ability types,
	 * no matter what the dependency.  We assume that generic abilities (like
	 * resist fear in the current version) don't need rescaling.  This
	 * introduces some inaccuracy in cases where specific instances of an
	 * ability (like INT bonus on helms) have been counted separately -
	 * ideally we should adjust for this in the general case.  However, as
	 * long as this doesn't occur too often, it shouldn't be a big issue.
	 *
	 * The following loops look complicated, but they are simply equivalent
	 * to going through each of the relevant ability types one by one.
	 */

	/* Bow-only abilities */
	for (i = 0; i < ART_IDX_BOW_COUNT; i++)
	{
		artprobs[art_idx_bow[i]] = (artprobs[art_idx_bow[i]] * art_total)
			/ art_bow_total;
	}

	/* All weapon abilities */
	for (i = 0; i < ART_IDX_WEAPON_COUNT; i++)
	{
		artprobs[art_idx_weapon[i]] = (artprobs[art_idx_weapon[i]] *
			art_total) / (art_bow_total + art_melee_total);
	}

	/* Corresponding non-weapon abilities */
	temp = art_total - art_melee_total - art_bow_total;
	for (i = 0; i < ART_IDX_NONWEAPON_COUNT; i++)
	{
		artprobs[art_idx_nonweapon[i]] = (artprobs[art_idx_nonweapon[i]] *
			art_total) / temp;
	}

	/* All melee weapon abilities */
	for (i = 0; i < ART_IDX_MELEE_COUNT; i++)
	{
		artprobs[art_idx_melee[i]] = (artprobs[art_idx_melee[i]] *
			art_total) / art_melee_total;
	}

	/* All general armor abilities */
	temp = art_armor_total + art_boot_total + art_shield_total +
		art_headgear_total + art_cloak_total + art_glove_total;
	for (i = 0; i < ART_IDX_ALLARMOR_COUNT; i++)
	{
		artprobs[art_idx_allarmor[i]] = (artprobs[art_idx_allarmor[i]] *
			art_total) / temp;
	}

	/* Boots */
	for (i = 0; i < ART_IDX_BOOT_COUNT; i++)
	{
		artprobs[art_idx_boot[i]] = (artprobs[art_idx_boot[i]] *
			art_total) / art_boot_total;
	}

	/* Gloves */
	for (i = 0; i < ART_IDX_GLOVE_COUNT; i++)
	{
		artprobs[art_idx_glove[i]] = (artprobs[art_idx_glove[i]] *
			art_total) / art_glove_total;
	}

	/* Headgear */
	for (i = 0; i < ART_IDX_HELM_COUNT; i++)
	{
		artprobs[art_idx_headgear[i]] = (artprobs[art_idx_headgear[i]] *
			art_total) / art_headgear_total;
	}

	/* Shields */
	for (i = 0; i < ART_IDX_SHIELD_COUNT; i++)
	{
		artprobs[art_idx_shield[i]] = (artprobs[art_idx_shield[i]] *
			art_total) / art_shield_total;
	}

	/* Cloaks */
	for (i = 0; i < ART_IDX_CLOAK_COUNT; i++)
	{
		artprobs[art_idx_cloak[i]] = (artprobs[art_idx_cloak[i]] *
			art_total) / art_cloak_total;
	}

	/* Body armor */
	for (i = 0; i < ART_IDX_ARMOR_COUNT; i++)
	{
		artprobs[art_idx_armor[i]] = (artprobs[art_idx_armor[i]] *
			art_total) / art_armor_total;
	}

	/*
	 * All others are general case and don't need to be rescaled,
	 * unless the algorithm is getting too clever about separating
	 * out individual cases (in which case some logic should be
	 * added for them in the following method call).
	 */

	/* Perform any additional rescaling and adjustment, if required. */
	adjust_freqs();

	/* Log the final frequencies to check that everything's correct */
	for(i=0; i<ART_IDX_TOTAL; i++)
	{
		LOG_PRINT2( "Rescaled frequency of ability %d: %d\n", i, artprobs[i]);
	}

}

static bool add_str(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_STR) return FALSE;
	a_ptr->flags1 |= TR1_STR;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: STR (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static bool add_int(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_INT) return FALSE;
	a_ptr->flags1 |= TR1_INT;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: INT (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static bool add_wis(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_WIS) return FALSE;
	a_ptr->flags1 |= TR1_WIS;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: WIS (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static bool add_dex(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_DEX) return FALSE;
	a_ptr->flags1 |= TR1_DEX;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: DEX (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static bool add_con(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_CON) return FALSE;
	a_ptr->flags1 |= TR1_CON;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: CON (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static bool add_chr(artifact_type *a_ptr)
{
	if(a_ptr->flags1 & TR1_CHR) return FALSE;
	a_ptr->flags1 |= TR1_CHR;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: CHR (now %+d)\n", a_ptr->pval);
	return TRUE;
}

static void add_stat(artifact_type *a_ptr)
{
	int r;
	bool success = FALSE;

	/* Hack: break out if all stats are raised to avoid an infinite loop */
	if ((a_ptr->flags1 & TR1_STR) && (a_ptr->flags1 & TR1_INT) &&
		(a_ptr->flags1 & TR1_WIS) && (a_ptr->flags1 & TR1_DEX) &&
		(a_ptr->flags1 & TR1_CON) && (a_ptr->flags1 & TR1_CHR))
			return;

	/* Make sure we add one that hasn't been added yet */
	while (!success)
	{
		r = rand_int(6);
		if (r == 0) success = add_str(a_ptr);
		else if (r == 1) success = add_int(a_ptr);
		else if (r == 2) success = add_wis(a_ptr);
		else if (r == 3) success = add_dex(a_ptr);
		else if (r == 4) success = add_con(a_ptr);
		else if (r == 5) success = add_chr(a_ptr);
	}
}

static bool add_sus_str(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_STR) return FALSE;
	a_ptr->flags2 |= TR2_SUST_STR;
	LOG_PRINT("Adding ability: sustain STR\n");
	return TRUE;
}

static bool add_sus_int(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_INT) return FALSE;
	a_ptr->flags2 |= TR2_SUST_INT;
	LOG_PRINT("Adding ability: sustain INT\n");
	return TRUE;
}

static bool add_sus_wis(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_WIS) return FALSE;
	a_ptr->flags2 |= TR2_SUST_WIS;
	LOG_PRINT("Adding ability: sustain WIS\n");
	return TRUE;
}

static bool add_sus_dex(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_DEX) return FALSE;
	a_ptr->flags2 |= TR2_SUST_DEX;
	LOG_PRINT("Adding ability: sustain DEX\n");
	return TRUE;
}

static bool add_sus_con(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_CON) return FALSE;
	a_ptr->flags2 |= TR2_SUST_CON;
	LOG_PRINT("Adding ability: sustain CON\n");
	return TRUE;
}

static bool add_sus_chr(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_SUST_CHR) return FALSE;
	a_ptr->flags2 |= TR2_SUST_CHR;
	LOG_PRINT("Adding ability: sustain CHR\n");
	return TRUE;
}

static void add_sustain(artifact_type *a_ptr)
{
	int r;
	bool success = FALSE;

	/* Hack: break out if all stats are sustained to avoid an infinite loop */
	if ((a_ptr->flags2 & TR2_SUST_STR) && (a_ptr->flags2 & TR2_SUST_INT) &&
		(a_ptr->flags2 & TR2_SUST_WIS) && (a_ptr->flags2 & TR2_SUST_DEX) &&
		(a_ptr->flags2 & TR2_SUST_CON) && (a_ptr->flags2 & TR2_SUST_CHR))
			return;

	while (!success)
	{
		r = rand_int(6);
		if (r == 0) success = add_sus_str(a_ptr);
		else if (r == 1) success = add_sus_int(a_ptr);
		else if (r == 2) success = add_sus_wis(a_ptr);
		else if (r == 3) success = add_sus_dex(a_ptr);
		else if (r == 4) success = add_sus_con(a_ptr);
		else if (r == 5) success = add_sus_chr(a_ptr);
	}
}

static void add_spell_resistance(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_SAVE;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: spell resistance (now %+d)\n", a_ptr->pval);
}

static void add_magical_device(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_DEVICE;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: magical devices (now %+d)\n", a_ptr->pval);
}

static void add_stealth(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_STEALTH;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: stealth (now %+d)\n", a_ptr->pval);
}

static void add_search(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_SEARCH;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: searching (now %+d)\n", a_ptr->pval);
}

static void add_infravision(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_INFRA;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: infravision (now %+d)\n", a_ptr->pval);
}

static void add_tunnelling(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_TUNNEL;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: tunnelling (new bonus is %+d)\n", a_ptr->pval);
}

static void add_speed(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_SPEED;
	if (a_ptr->pval == 0)
	{
		a_ptr->pval = (s16b)(1 + rand_int(4));
		LOG_PRINT1("Adding ability: speed (first time) (now %+d)\n", a_ptr->pval);
	}
	else
	{
		do_pval(a_ptr);
		LOG_PRINT1("Adding ability: speed (now %+d)\n", a_ptr->pval);
	}
}

static void add_shots(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_SHOTS;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: extra shots (now %+d)\n", a_ptr->pval);
}

static void add_blows(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_BLOWS;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: extra blows (%d additional blows)\n", a_ptr->pval);
}

static void add_might(artifact_type *a_ptr)
{
	a_ptr->flags1 |= TR1_MIGHT;
	do_pval(a_ptr);
	LOG_PRINT1("Adding ability: extra might (now %+d)\n", a_ptr->pval);
}

static bool add_resist_acid(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_ACID) return FALSE;
	a_ptr->flags2 |= TR2_RES_ACID;
	LOG_PRINT("Adding ability: resist acid\n");
	return TRUE;
}

static bool add_resist_lightning(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_ELEC) return FALSE;
	a_ptr->flags2 |= TR2_RES_ELEC;
	LOG_PRINT("Adding ability: resist lightning\n");
	return TRUE;
}

static bool add_resist_fire(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_FIRE) return FALSE;
	a_ptr->flags2 |= TR2_RES_FIRE;
	LOG_PRINT("Adding ability: resist fire\n");
	return TRUE;
}

static bool add_resist_cold(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_COLD) return FALSE;
	a_ptr->flags2 |= TR2_RES_COLD;
	LOG_PRINT("Adding ability: resist cold\n");
	return TRUE;
}

static void add_low_resist(artifact_type *a_ptr)
{
	int r;
	bool success = FALSE;

	/* Hack - if all low resists added already, exit to avoid infinite loop */
	if( (a_ptr->flags2 & TR2_RES_ACID) && (a_ptr->flags2 & TR2_RES_ELEC) &&
		(a_ptr->flags2 & TR2_RES_FIRE) && (a_ptr->flags2 & TR2_RES_COLD) )
			return;

	/* Hack -- branded weapons prefer same kinds of resist as brand */
	r = rand_int(4);
	if ((r == 0) && (a_ptr->flags1 & (TR1_BRAND_ACID))) success = add_resist_acid(a_ptr);
	else if ((r == 1) && (a_ptr->flags1 & (TR1_BRAND_ELEC))) success = add_resist_lightning(a_ptr);
	else if ((r == 2) && (a_ptr->flags1 & (TR1_BRAND_FIRE))) success = add_resist_fire(a_ptr);
	else if ((r == 3) && (a_ptr->flags1 & (TR1_BRAND_COLD))) success = add_resist_cold(a_ptr);

	while (!success)
	{
		r = rand_int(4);
		if (r == 0) success = add_resist_acid(a_ptr);
		else if (r == 1) success = add_resist_lightning(a_ptr);
		else if (r == 2) success = add_resist_fire(a_ptr);
		else if (r == 3) success = add_resist_cold(a_ptr);
	}
}

static bool add_resist_poison(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_POIS) return FALSE;
	a_ptr->flags2 |= TR2_RES_POIS;
	LOG_PRINT("Adding ability: resist poison\n");
	return TRUE;
}

static bool add_resist_fear(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_FEAR) return FALSE;
	a_ptr->flags2 |= TR2_RES_FEAR;
	LOG_PRINT("Adding ability: resist fear\n");
	return TRUE;
}

static bool add_resist_light(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_LITE) return FALSE;
	a_ptr->flags2 |= TR2_RES_LITE;
	LOG_PRINT("Adding ability: resist light\n");
	return TRUE;
}

static bool add_resist_dark(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_DARK) return FALSE;
	a_ptr->flags2 |= TR2_RES_DARK;
	LOG_PRINT("Adding ability: resist dark\n");
	return TRUE;
}

static bool add_resist_blindness(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_BLIND) return FALSE;
	a_ptr->flags2 |= TR2_RES_BLIND;
	LOG_PRINT("Adding ability: resist blindness\n");
	return TRUE;
}

static bool add_resist_confusion(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_CONFU) return FALSE;
	a_ptr->flags2 |= TR2_RES_CONFU;
	LOG_PRINT("Adding ability: resist confusion\n");
	return TRUE;
}

static bool add_resist_sound(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_SOUND) return FALSE;
	a_ptr->flags2 |= TR2_RES_SOUND;
	LOG_PRINT("Adding ability: resist sound\n");
	return TRUE;
}

static bool add_resist_shards(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_SHARD) return FALSE;
	a_ptr->flags2 |= TR2_RES_SHARD;
	LOG_PRINT("Adding ability: resist sound\n");
	return TRUE;
}

static bool add_resist_nexus(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_NEXUS) return FALSE;
	a_ptr->flags2 |= TR2_RES_NEXUS;
	LOG_PRINT("Adding ability: resist nexus\n");
	return TRUE;
}

static bool add_resist_nether(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_NETHR) return FALSE;
	a_ptr->flags2 |= TR2_RES_NETHR;
	LOG_PRINT("Adding ability: resist nether\n");
	return TRUE;
}

static bool add_resist_chaos(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_CHAOS) return FALSE;
	a_ptr->flags2 |= TR2_RES_CHAOS;
	LOG_PRINT("Adding ability: resist chaos\n");
	return TRUE;
}

static bool add_resist_disenchantment(artifact_type *a_ptr)
{
	if (a_ptr->flags2 & TR2_RES_DISEN) return FALSE;
	a_ptr->flags2 |= TR2_RES_DISEN;
	LOG_PRINT("Adding ability: resist disenchantment\n");
	return TRUE;
}

static bool add_resist_disease(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_RES_DISEASE) return FALSE;
	a_ptr->flags4 |= TR4_RES_DISEASE;
	LOG_PRINT("Adding ability: resist disease\n");
	return TRUE;
}

static void add_high_resist(artifact_type *a_ptr)
{
	/* Add a high resist, according to the generated frequency distribution. */
	int r, i, temp;
	int count = 0;
	bool success = FALSE;

	temp = 0;
	for (i = 0; i < ART_IDX_HIGH_RESIST_COUNT; i++)
	{
		temp += artprobs[art_idx_high_resist[i]];
	}

	/* Hack -- brands prefer same kind of resist as their brand */
	r = rand_int(3);
	if ((r == 0) && (a_ptr->flags1 & (TR1_BRAND_POIS))) success = add_resist_poison(a_ptr);
	else if ((r == 1) && (a_ptr->flags4 & (TR4_BRAND_LITE))) success = add_resist_light(a_ptr);
	else if ((r == 2) && (a_ptr->flags4 & (TR4_BRAND_DARK))) success = add_resist_dark(a_ptr);

	/* The following will fail (cleanly) if all high resists already added */
	while ( (!success) && (count < MAX_TRIES) )
	{
		/* Randomize from 1 to this total amount */
		r = randint(temp);

		/* Determine which (weighted) resist this number corresponds to */

		temp = artprobs[art_idx_high_resist[0]];
		i = 0;
		while (r > temp && i < ART_IDX_HIGH_RESIST_COUNT)
		{
			temp += artprobs[art_idx_high_resist[i]];
			i++;
		}

		/* Now i should give us the index of the correct high resist */
		if (i == 0) success = add_resist_poison(a_ptr);
		else if (i == 1) success = add_resist_fear(a_ptr);
		else if (i == 2) success = add_resist_light(a_ptr);
		else if (i == 3) success = add_resist_dark(a_ptr);
		else if (i == 4) success = add_resist_blindness(a_ptr);
		else if (i == 5) success = add_resist_confusion(a_ptr);
		else if (i == 6) success = add_resist_sound(a_ptr);
		else if (i == 7) success = add_resist_shards(a_ptr);
		else if (i == 8) success = add_resist_nexus(a_ptr);
		else if (i == 9) success = add_resist_nether(a_ptr);
		else if (i == 10) success = add_resist_chaos(a_ptr);
		else if (i == 11) success = add_resist_disenchantment(a_ptr);
		else if (i == 12) success = add_resist_disease(a_ptr);

		count++;
	}
}

static void add_slow_digestion(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_SLOW_DIGEST;
	LOG_PRINT("Adding ability: slow digestion\n");
}

static void add_feather_falling(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_FEATHER;
	LOG_PRINT("Adding ability: feather fall\n");
}

static void add_permanent_light(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_LITE;
	LOG_PRINT("Adding ability: permanent light\n");
}

static void add_regeneration(artifact_type *a_ptr)
{
	if (rand_int(3))
	{
		a_ptr->flags3 |= TR3_REGEN_HP;
		LOG_PRINT("Adding ability: regenerate hit points\n");
	}
	else
	{
		a_ptr->flags3 |= TR3_REGEN_MANA;
		LOG_PRINT("Adding ability: regenerate mana\n");
	}
}

static void add_telepathy(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_TELEPATHY;
	LOG_PRINT("Adding ability: telepathy\n");
}

/* Start of ESP Add_functions ARD_ESP */
static bool add_sense_orc(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_ORC) return FALSE;
	a_ptr->flags3 |= TR3_ESP_ORC;
	LOG_PRINT("Adding ability: sense orc\n");
	return (TRUE);
}

static bool add_sense_giant(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_GIANT) return FALSE;
	a_ptr->flags3 |= TR3_ESP_GIANT;
	LOG_PRINT("Adding ability: sense giant\n");
	return (TRUE);
}

static bool add_sense_troll(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_TROLL) return FALSE;
	a_ptr->flags3 |= TR3_ESP_TROLL;
	LOG_PRINT("Adding ability: sense troll\n");
	return (TRUE);
}

static bool add_sense_dragon(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_DRAGON) return FALSE;
	a_ptr->flags3 |= TR3_ESP_DRAGON;
	LOG_PRINT("Adding ability: sense dragon\n");
	return (TRUE);
}

static bool add_sense_demon(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_DEMON) return FALSE;
	a_ptr->flags3 |= TR3_ESP_DEMON;
	LOG_PRINT("Adding ability: sense demon\n");
	return (TRUE);
}

static bool add_sense_undead(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_UNDEAD) return FALSE;
	a_ptr->flags3 |= TR3_ESP_UNDEAD;
	LOG_PRINT("Adding ability: sense undead\n");
	return (TRUE);
}

static bool add_sense_nature(artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_ESP_NATURE) return FALSE;
	a_ptr->flags3 |= TR3_ESP_NATURE;
	LOG_PRINT("Adding ability: sense nature\n");
	return (TRUE);
}


static void add_sense_slay(artifact_type *a_ptr)
{
	/* Pick a sense at random, as long as the weapon has the slay */
	/* Note we bias towards the high slays here */

	int r;
	int count = 0;
	bool success = FALSE;

	/* Hack -- not on telepathic items */
	if (a_ptr->flags3 & (TR3_TELEPATHY)) return;

	while ( (!success) & (count < MAX_TRIES) )
	{
		r = rand_int(10);
		if ((r == 0) && (a_ptr->flags1 & TR1_SLAY_ORC)) success = add_sense_orc(a_ptr);
		else if ((r == 1) && (a_ptr->flags1 & TR1_SLAY_GIANT)) success = add_sense_giant(a_ptr);
		else if ((r == 2) && (a_ptr->flags1 & TR1_SLAY_TROLL)) success = add_sense_troll(a_ptr);
		else if ((r == 3) && (a_ptr->flags1 & TR1_SLAY_DRAGON)) success = add_sense_dragon(a_ptr);
		else if ((r == 4) && (a_ptr->flags1 & TR1_SLAY_DEMON)) success = add_sense_demon(a_ptr);
		else if ((r == 5) && (a_ptr->flags1 & TR1_SLAY_UNDEAD)) success = add_sense_undead(a_ptr);
		else if ((r == 6) && (a_ptr->flags1 & TR1_KILL_DRAGON)) success = add_sense_dragon(a_ptr);
		else if ((r == 7) && (a_ptr->flags1 & TR1_KILL_DEMON)) success = add_sense_demon(a_ptr);
		else if ((r == 8) && (a_ptr->flags1 & TR1_KILL_UNDEAD)) success = add_sense_undead(a_ptr);
		    else if ((r == 9) && (a_ptr->flags1 & TR1_SLAY_NATURAL)) success = add_sense_nature(a_ptr);

		count++;
	}
}

static void add_sense_rand(artifact_type *a_ptr)
{
	/* Pick a sense at random */
	/* Note we bias towards the high slays here, even though per
	   * se, none of this equipment will have high-slays
	 */


	int r;
	int count = 0;
	bool success = FALSE;

	/* Hack -- not on telepathic items */
	if (a_ptr->flags3 & (TR3_TELEPATHY)) return;

	while ( (!success) & (count < MAX_TRIES) )
	{
		r = rand_int(10);
		if (r < 2) success = add_sense_dragon(a_ptr);
		else if (r < 4) success = add_sense_demon(a_ptr);
		else if (r < 6) success = add_sense_undead(a_ptr);
		else if (r == 6) success = add_sense_orc(a_ptr);
		else if (r == 7) success = add_sense_giant(a_ptr);
		else if (r == 8) success = add_sense_troll(a_ptr);
		else if (r == 9) success = add_sense_nature(a_ptr);

		count++;
	}
}
/* End of ESP add functions ARD_ESP */


static void add_see_invisible(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_SEE_INVIS;
	LOG_PRINT("Adding ability: see invisible\n");
}

static void add_free_action(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_FREE_ACT;
	LOG_PRINT("Adding ability: free action\n");
}

static void add_hold_life(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_HOLD_LIFE;
	LOG_PRINT("Adding ability: hold life\n");
}

static bool add_slay_natural(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_NATURAL) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_NATURAL;
	LOG_PRINT("Adding ability: slay animal\n");
	return TRUE;
}

static bool add_brand_holy(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_HOLY) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_HOLY;
	LOG_PRINT("Adding ability: slay evil\n");
	return TRUE;
}

static bool add_slay_orc(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_ORC) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_ORC;
	LOG_PRINT("Adding ability: slay orc\n");
	return TRUE;
}

static bool add_slay_troll(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_TROLL) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_TROLL;
	LOG_PRINT("Adding ability: slay troll \n");
	return TRUE;
}

static bool add_slay_giant(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_GIANT) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_GIANT;
	LOG_PRINT("Adding ability: slay giant\n");
	return TRUE;
}

static bool add_slay_demon(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_DEMON) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_DEMON;
	LOG_PRINT("Adding ability: slay demon\n");
	return TRUE;
}

static bool add_slay_undead(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_UNDEAD) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_UNDEAD;
	LOG_PRINT("Adding ability: slay undead\n");
	return TRUE;
}

static bool add_slay_dragon(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_SLAY_DRAGON) return FALSE;
	a_ptr->flags1 |= TR1_SLAY_DRAGON;
	LOG_PRINT("Adding ability: slay dragon\n");
	return TRUE;
}

static bool add_kill_demon(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_KILL_DEMON) return FALSE;
	a_ptr->flags1 |= TR1_KILL_DEMON;
	LOG_PRINT("Adding ability: kill demon\n");
	return TRUE;
}

static bool add_kill_undead(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_KILL_UNDEAD) return FALSE;
	a_ptr->flags1 |= TR1_KILL_UNDEAD;
	LOG_PRINT("Adding ability: kill undead\n");
	return TRUE;
}

static bool add_kill_dragon(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_KILL_DRAGON) return FALSE;
	a_ptr->flags1 |= TR1_KILL_DRAGON;
	LOG_PRINT("Adding ability: kill dragon\n");
	return TRUE;
}

static bool add_acid_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_ACID) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_ACID;
	LOG_PRINT("Adding ability: acid brand\n");
	return TRUE;
}

static bool add_lightning_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_ELEC) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_ELEC;
	LOG_PRINT("Adding ability: lightning brand\n");
	return TRUE;
}

static bool add_fire_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_FIRE) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_FIRE;
	LOG_PRINT("Adding ability: fire brand\n");
	return TRUE;
}

static bool add_frost_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_COLD) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_COLD;
	LOG_PRINT("Adding ability: frost brand\n");
	return TRUE;
}

static bool add_poison_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_POIS) return FALSE;
	a_ptr->flags1 |= TR1_BRAND_POIS;
	LOG_PRINT("Adding ability: poison brand\n");
	return TRUE;
}

static bool add_lite_brand(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_BRAND_LITE) return FALSE;
	a_ptr->flags4 |= TR4_BRAND_LITE;
	LOG_PRINT("Adding ability: light brand\n");
	return TRUE;
}

static bool add_dark_brand(artifact_type *a_ptr)
{
 	if (a_ptr->flags4 & TR4_BRAND_DARK) return FALSE;
	a_ptr->flags4 |= TR4_BRAND_DARK;
	LOG_PRINT("Adding ability: darkness brand\n");
	return TRUE;
}

static bool add_slay_man(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_HOLY) return FALSE;
	if (a_ptr->flags4 & TR4_SLAY_MAN) return FALSE;
	a_ptr->flags4 |= TR4_SLAY_MAN;
	if (rand_int(3) == 0) a_ptr->flags4 |= TR4_EVIL;
	LOG_PRINT("Adding ability: slay man\n");
	if (a_ptr->flags4 & TR4_EVIL) LOG_PRINT("Weapon is evil.");
	return TRUE;
}

static bool add_slay_elf(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_HOLY) return FALSE;
	if (a_ptr->flags4 & TR4_SLAY_ELF) return FALSE;
	a_ptr->flags4 |= TR4_SLAY_ELF;
	if (rand_int(3) == 0) a_ptr->flags4 |= TR4_EVIL;
	LOG_PRINT("Adding ability: slay elf\n");
	if (a_ptr->flags4 & TR4_EVIL) LOG_PRINT("Weapon is evil.");
	return TRUE;
}

static bool add_slay_dwarf(artifact_type *a_ptr)
{
	if (a_ptr->flags1 & TR1_BRAND_HOLY) return FALSE;
	if (a_ptr->flags4 & TR4_SLAY_DWARF) return FALSE;
	a_ptr->flags4 |= TR4_SLAY_DWARF;
	LOG_PRINT("Adding ability: slay dwarf\n");
	if (rand_int(3) == 0) a_ptr->flags4 |= TR4_EVIL;
	if (a_ptr->flags4 & TR4_EVIL) LOG_PRINT("Weapon is evil.");
	return TRUE;
}

static void add_brand_or_slay(artifact_type *a_ptr)
{
	/* Pick a brand or slay at random */

	int r;
	int count = 0;
	bool success = FALSE;

	while ( (!success) & (count < MAX_TRIES) )
	{
		r = rand_int(19) + 1;
		if (r == 1) success = add_brand_holy(a_ptr);
		else if (r == 2) success = add_slay_natural(a_ptr);
		else if (r == 3) success = add_slay_undead(a_ptr);
		else if (r == 4) success = add_slay_dragon(a_ptr);
		else if (r == 5) success = add_slay_demon(a_ptr);
		else if (r == 6) success = add_slay_troll(a_ptr);
		else if (r == 7) success = add_slay_orc(a_ptr);
		else if (r == 8) success = add_slay_giant(a_ptr);
		else if (r == 9) success = add_acid_brand(a_ptr);
		else if (r == 10) success = add_lightning_brand(a_ptr);
		else if (r == 11) success = add_fire_brand(a_ptr);
		else if (r == 12) success = add_frost_brand(a_ptr);
		else if (r == 13) success = add_poison_brand(a_ptr);
		else if (r == 14) success = add_kill_demon(a_ptr);
		else if (r == 15) success = add_kill_undead(a_ptr);
		else if (r == 16) success = add_kill_dragon(a_ptr);
		else if (r == 17) success = add_lite_brand(a_ptr);
		else if (r == 18) success = add_dark_brand(a_ptr);

		/* Note: must keep this equal to brand_holy */
		else if (r == 19)
		{
			if (rand_int(3) == 0) success = add_slay_man(a_ptr);
			else if (rand_int(2) == 0) success = add_slay_elf(a_ptr);
			else success = add_slay_dwarf(a_ptr);
		}

		count++;
	}
}

static bool add_restrict_orc(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_ORC) return FALSE;
	a_ptr->flags4 |= TR4_ORC;
	LOG_PRINT("Adding ability: mark as orc\n");
	return TRUE;
}

static bool add_restrict_giant(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_GIANT) return FALSE;
	a_ptr->flags4 |= TR4_GIANT;
	LOG_PRINT("Adding ability: mark as giant\n");
	return TRUE;
}

static bool add_restrict_troll(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_TROLL) return FALSE;
	a_ptr->flags4 |= TR4_TROLL;
	LOG_PRINT("Adding ability: mark as troll\n");
	return TRUE;
}

static bool add_restrict_dragon(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_DRAGON) return FALSE;
	a_ptr->flags4 |= TR4_DRAGON;
	LOG_PRINT("Adding ability: mark as dragon\n");
	return TRUE;
}


static bool add_restrict_demon(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_DEMON) return FALSE;
	a_ptr->flags4 |= TR4_DEMON;
	LOG_PRINT("Adding ability: mark as demon\n");
	return TRUE;
}

static bool add_restrict_undead(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_UNDEAD) return FALSE;
	a_ptr->flags4 |= TR4_UNDEAD;
	LOG_PRINT("Adding ability: mark as undead\n");
	return TRUE;
}

static bool add_restrict_man(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_MAN) return FALSE;
	a_ptr->flags4 |= TR4_MAN;
	LOG_PRINT("Adding ability: mark as man\n");
	return TRUE;
}

static bool add_restrict_elf(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_ELF) return FALSE;
	a_ptr->flags4 |= TR4_ELF;
	LOG_PRINT("Adding ability: mark as elf\n");
	return TRUE;
}

static bool add_restrict_dwarf(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_DWARF) return FALSE;
	a_ptr->flags4 |= TR4_DWARF;
	LOG_PRINT("Adding ability: mark as dwarf\n");
	return TRUE;
}

static bool add_restrict_animal(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_ANIMAL) return FALSE;
	a_ptr->flags4 |= TR4_ANIMAL;
	LOG_PRINT("Adding ability: mark as animal\n");
	return TRUE;
}

static bool add_restrict_evil(artifact_type *a_ptr)
{
	if (a_ptr->flags4 & TR4_EVIL) return FALSE;
	a_ptr->flags4 |= TR4_EVIL;
	LOG_PRINT("Adding ability: mark as evil\n");
	return TRUE;
}

static void add_restrict_rand(artifact_type *a_ptr)
{
	/* Pick a restriction at random, as long as item has some related abilities or slays */

	int r;
	int count = 0;
	bool success = FALSE;

	while ( (!success) & (count < MAX_TRIES) )
	{
		r = rand_int(9);
		if ((r == 0) && ((a_ptr->flags2 & TR2_RES_DARK) || (a_ptr->flags4 & TR4_SLAY_ELF))) success = add_restrict_orc(a_ptr);
		else if ((r == 1) && ((a_ptr->flags1 & TR1_STR) || (a_ptr->flags2 & TR2_SUST_STR) || (a_ptr->flags4 & TR4_SLAY_DWARF))) success = add_restrict_giant(a_ptr);
		else if ((r == 2) && (a_ptr->flags3 & TR3_REGEN_HP)) success = add_restrict_troll(a_ptr);
		else if ((r == 3) && (a_ptr->flags2 & TR2_RES_ACID) && (a_ptr->flags2 & TR2_RES_FIRE) && (a_ptr->flags2 & TR2_RES_ELEC) && (a_ptr->flags2 & TR2_RES_COLD)) success = add_restrict_dragon(a_ptr);
		else if ((r == 4) && (a_ptr->flags2 & (TR2_RES_FIRE | TR2_IM_FIRE))) success = add_restrict_demon(a_ptr);
		else if ((r == 5) && ((a_ptr->flags2 & TR2_RES_NETHR) || (a_ptr->flags3 & (TR3_SEE_INVIS | TR3_HOLD_LIFE)))) success = add_restrict_undead(a_ptr);
		else if ((r == 6) && ((a_ptr->flags1 & (TR1_SLAY_ORC | TR1_STEALTH)) || (a_ptr->flags2 & TR2_RES_LITE) || (a_ptr->flags3 & TR3_SEE_INVIS))) success = add_restrict_elf(a_ptr);
		else if ((r == 7) && ((a_ptr->flags1 & (TR1_SLAY_GIANT)) || (a_ptr->flags2 & TR2_RES_BLIND) || (a_ptr->flags3 & (TR3_FREE_ACT | TR3_HOLD_LIFE)))) success = add_restrict_dwarf(a_ptr);
		else if ((r == 8) && ((a_ptr->flags1 & TR1_STEALTH) || (a_ptr->flags3 & (TR3_SLOW_DIGEST | TR3_FEATHER)))) success = add_restrict_animal(a_ptr);

		count++;
	}

	if (!success)
	{
		if (rand_int(3)) (void)add_restrict_man(a_ptr);
		else (void)add_restrict_evil(a_ptr);
	}
}



static void add_bless_weapon(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_BLESSED;
	LOG_PRINT("Adding ability: blessed blade\n");
}

static void add_damage_dice(artifact_type *a_ptr)
{
	/* CR 2001-09-02: changed this to increments 1 or 2 only */
	a_ptr->dd += (byte)(1 + rand_int(2));
	if (a_ptr->dd > 9)
		a_ptr->dd = 9;
	LOG_PRINT1("Adding ability: extra damage dice (now %d dice)\n", a_ptr->dd);
}

static void add_to_hit(artifact_type *a_ptr, int fixed, int random)
{
	/* Inhibit above certain threshholds */
	if (a_ptr->to_h > 25)
	{
		/* Strongly inhibit */
		if (rand_int(INHIBIT_STRONG) > 0)
		{
			LOG_PRINT1("Failed to add to-hit, value of %d is too high\n", a_ptr->to_h);
			return;
		}
	}
	else if (a_ptr->to_h > 15)
	{
		/* Weakly inhibit */
		if (rand_int(INHIBIT_WEAK) > 0)
		{
			LOG_PRINT1("Failed to add to-hit, value of %d is too high\n", a_ptr->to_h);
			return;
		}
	}
	a_ptr->to_h += (s16b)(fixed + rand_int(random));
	LOG_PRINT1("Adding ability: extra to_h (now %+d)\n", a_ptr->to_h);
}

static void add_to_dam(artifact_type *a_ptr, int fixed, int random)
{
	/* Inhibit above certain threshholds */
	if (a_ptr->to_d > 25)
	{
		/* Strongly inhibit */
		if (rand_int(INHIBIT_STRONG) > 0)
		{
			LOG_PRINT1("Failed to add to-dam, value of %d is too high\n", a_ptr->to_d);
			return;
		}
	}
	else if (a_ptr->to_d > 15)
	{
		/* Weakly inhibit */
		if (rand_int(INHIBIT_WEAK) > 0)
		{
			LOG_PRINT1("Failed to add to-dam, value of %d is too high\n", a_ptr->to_d);
			return;
		}
	}
	a_ptr->to_d += (s16b)(fixed + rand_int(random));
	LOG_PRINT1("Adding ability: extra to_dam (now %+d)\n", a_ptr->to_d);
}

static void add_aggravation(artifact_type *a_ptr)
{
	a_ptr->flags3 |= TR3_AGGRAVATE;
	LOG_PRINT("Adding aggravation\n");
}

static void add_to_AC(artifact_type *a_ptr, int fixed, int random)
{
	/* Inhibit above certain threshholds */
	if (a_ptr->to_a > 35)
	{
		/* Strongly inhibit */
		if (rand_int(INHIBIT_STRONG) > 0)
		{
			LOG_PRINT1("Failed to add to-AC, value of %d is too high\n", a_ptr->to_a);
			return;
		}
	}
	else if (a_ptr->to_a > 25)
	{
		/* Weakly inhibit */
		if (rand_int(INHIBIT_WEAK) > 0)
		{
			LOG_PRINT1("Failed to add to-AC, value of %d is too high\n", a_ptr->to_a);
			return;
		}
	}
	a_ptr->to_a += (s16b)(fixed + rand_int(random));
	LOG_PRINT1("Adding ability: AC bonus (new bonus is %+d)\n", a_ptr->to_a);
}

static void add_weight_mod(artifact_type *a_ptr)
{
	a_ptr->weight = (a_ptr->weight * 9) / 10;
	LOG_PRINT1("Adding ability: lower weight (new weight is %d)\n", a_ptr->weight);
}

/*
 * Add a random immunity to this artifact
 * ASSUMPTION: All immunities are equally likely.
 */
static void add_immunity(artifact_type *a_ptr)
{
	int imm_type = rand_int(5);

	switch(imm_type)
	{
		case 0:
		{
			a_ptr->flags2 |= TR2_IM_ACID;
			LOG_PRINT("Adding ability: immunity to acid\n");
			break;
		}
		case 1:
		{
			a_ptr->flags2 |= TR2_IM_ELEC;
			LOG_PRINT("Adding ability: immunity to lightning\n");
			break;
		}
		case 2:
		{
			a_ptr->flags2 |= TR2_IM_FIRE;
			LOG_PRINT("Adding ability: immunity to fire\n");
			break;
		}
		case 3:
		{
			a_ptr->flags2 |= TR2_IM_COLD;
			LOG_PRINT("Adding ability: immunity to cold\n");
			break;
		}
		case 4:
		{
			a_ptr->flags4 |= TR4_IM_POIS;
			LOG_PRINT("Adding ability: immunity to poison\n");
			break;
		}
	}
}

/*
 * Build a suitable frequency table for this item, based on the generated
 * frequencies.  The frequencies for any abilities that don't apply for
 * this item type will be set to zero.  First parameter is the artifact
 * for which to generate the frequency table.
 *
 * The second input parameter is a pointer to an array that the function
 * will use to store the frequency table.  The array must have size
 * ART_IDX_TOTAL.
 *
 * The resulting frequency table is cumulative for ease of use in the
 * weighted randomization algorithm.
 */

static void build_freq_table(artifact_type *a_ptr, s16b *freq)
{
	int i,j;
	s16b f_temp[ART_IDX_TOTAL];

	/* First, set everything to zero */
	for (i = 0; i < ART_IDX_TOTAL; i++)
	{
		f_temp[i] = 0;
		freq[i] = 0;
	}

	/* Now copy over appropriate frequencies for applicable abilities */
	/* Bow abilities */
	if (a_ptr->tval == TV_BOW)
	{
		for (j = 0; j < ART_IDX_BOW_COUNT; j++)
		{
			f_temp[art_idx_bow[j]] = artprobs[art_idx_bow[j]];
		}
	}
	/* General weapon abilities */
	if (a_ptr->tval == TV_BOW || a_ptr->tval == TV_DIGGING ||
		a_ptr->tval == TV_HAFTED || a_ptr->tval == TV_POLEARM ||
		a_ptr->tval == TV_SWORD)
	{
		for (j = 0; j < ART_IDX_WEAPON_COUNT; j++)
		{
			f_temp[art_idx_weapon[j]] = artprobs[art_idx_weapon[j]];
		}
	}
	/* General non-weapon abilities */
	else
	{
		for (j = 0; j < ART_IDX_NONWEAPON_COUNT; j++)
		{
			f_temp[art_idx_nonweapon[j]] = artprobs[art_idx_nonweapon[j]];
		}
	}
	/* General melee abilities */
	if (a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_HAFTED ||
		a_ptr->tval == TV_POLEARM || a_ptr->tval == TV_SWORD)
	{
		for (j = 0; j < ART_IDX_MELEE_COUNT; j++)
		{
			f_temp[art_idx_melee[j]] = artprobs[art_idx_melee[j]];
		}
	}
	/* General armor abilities */
	if ( a_ptr->tval == TV_BOOTS || a_ptr->tval == TV_GLOVES ||
		a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN ||
		a_ptr->tval == TV_SHIELD || a_ptr->tval == TV_CLOAK ||
		a_ptr->tval == TV_SOFT_ARMOR || a_ptr->tval == TV_HARD_ARMOR ||
		a_ptr->tval == TV_DRAG_ARMOR)
		{
		for (j = 0; j < ART_IDX_ALLARMOR_COUNT; j++)
		{
			f_temp[art_idx_allarmor[j]] = artprobs[art_idx_allarmor[j]];
		}
	}
	/* Boot abilities */
	if (a_ptr->tval == TV_BOOTS)
	{
		for (j = 0; j < ART_IDX_BOOT_COUNT; j++)
		{
			f_temp[art_idx_boot[j]] = artprobs[art_idx_boot[j]];
		}
	}
	/* Glove abilities */
	if (a_ptr->tval == TV_GLOVES)
	{
		for (j = 0; j < ART_IDX_GLOVE_COUNT; j++)
		{
			f_temp[art_idx_glove[j]] = artprobs[art_idx_glove[j]];
		}
	}
	/* Headgear abilities */
	if (a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)
	{
		for (j = 0; j < ART_IDX_HELM_COUNT; j++)
		{
			f_temp[art_idx_headgear[j]] = artprobs[art_idx_headgear[j]];
		}
	}
	/* Shield abilities */
	if (a_ptr->tval == TV_SHIELD)
	{
		for (j = 0; j < ART_IDX_SHIELD_COUNT; j++)
		{
			f_temp[art_idx_shield[j]] = artprobs[art_idx_shield[j]];
		}
	}
	/* Cloak abilities */
	if (a_ptr->tval == TV_CLOAK)
	{
		for (j = 0; j < ART_IDX_CLOAK_COUNT; j++)
		{
			f_temp[art_idx_cloak[j]] = artprobs[art_idx_cloak[j]];
		}
	}
	/* Armor abilities */
	if (a_ptr->tval == TV_SOFT_ARMOR || a_ptr->tval == TV_HARD_ARMOR ||
		a_ptr->tval == TV_DRAG_ARMOR)
	{
		for (j = 0; j < ART_IDX_ARMOR_COUNT; j++)
		{
			f_temp[art_idx_armor[j]] = artprobs[art_idx_armor[j]];
		}
	}

	/* General abilities - no constraint */
	for (j = 0; j < ART_IDX_GEN_COUNT; j++)
	{
		f_temp[art_idx_gen[j]] = artprobs[art_idx_gen[j]];
	}

	/*
	 * Now we have the correct individual frequencies, we build a cumulative
	 * frequency table for them.
	 */
	for (i = 0; i < ART_IDX_TOTAL; i++)
	{
		for (j = i; j < ART_IDX_TOTAL; j++)
		{
			freq[j] += f_temp[i];
		}
	}
	/* Done - the freq array holds the desired frequencies. */

	/* Print out the frequency table, for verification */
	for (i = 0; i < ART_IDX_TOTAL; i++)
		LOG_PRINT2("Cumulative frequency of ability %d is: %d\n", i, freq[i]);
}

/*
 * Choose a random ability using weights based on the given cumulative frequency
 * table.  A pointer to the frequency array (which must be of size ART_IDX_TOTAL)
 * is passed as a parameter.  The function returns a number representing the
 * index of the ability chosen.
 */

static int choose_ability (s16b *freq_table)
{
	int r, ability;

	/* Generate a random number between 1 and the last value in the table */
	r = randint (freq_table[ART_IDX_TOTAL-1]);

	/* Find the entry in the table that this number represents. */
	ability = 0;
	while (r > freq_table[ability])
		ability++;

	LOG_PRINT1("Ability chosen was number: %d\n", ability);
	/*
	 * The ability variable is now the index of the first value in the table
	 * greater than or equal to r, which is what we want.
	 */
	return ability;
}

/*
 * Add an ability given by the index r.  This is mostly just a long case
 * statement.
 *
 * Note that this method is totally general and imposes no restrictions on
 * appropriate item type for a given ability.  This is assumed to have
 * been done already.
 */

static void add_ability_aux(artifact_type *a_ptr, int r)
{
	switch(r)
	{
		case ART_IDX_BOW_SHOTS:
			add_shots(a_ptr);
			break;

		case ART_IDX_BOW_MIGHT:
			add_might(a_ptr);
			break;

		case ART_IDX_WEAPON_HIT:
		case ART_IDX_NONWEAPON_HIT:
			add_to_hit(a_ptr, 1, 2 * mean_hit_increment);
			break;

		case ART_IDX_WEAPON_DAM:
		case ART_IDX_NONWEAPON_DAM:
			add_to_dam(a_ptr, 1, 2 * mean_dam_increment);
			break;

		case ART_IDX_WEAPON_AGGR:
		case ART_IDX_NONWEAPON_AGGR:
			add_aggravation(a_ptr);
			break;

		case ART_IDX_MELEE_BLESS:
			add_bless_weapon(a_ptr);
			break;

		case ART_IDX_MELEE_BRAND_SLAY:
			add_brand_or_slay(a_ptr);
			break;

		case ART_IDX_MELEE_SINV:
		case ART_IDX_HELM_SINV:
		case ART_IDX_GEN_SINV:
			add_see_invisible(a_ptr);
			break;

		case ART_IDX_MELEE_BLOWS:
			add_blows(a_ptr);
			break;

		case ART_IDX_MELEE_AC:
		case ART_IDX_BOOT_AC:
		case ART_IDX_GLOVE_AC:
		case ART_IDX_HELM_AC:
		case ART_IDX_SHIELD_AC:
		case ART_IDX_CLOAK_AC:
		case ART_IDX_ARMOR_AC:
		case ART_IDX_GEN_AC:
			add_to_AC(a_ptr, 1, 2 * mean_ac_increment);
			break;

		case ART_IDX_MELEE_DICE:
			add_damage_dice(a_ptr);
			break;

		case ART_IDX_MELEE_WEIGHT:
		case ART_IDX_ALLARMOR_WEIGHT:
			add_weight_mod(a_ptr);
			break;

		case ART_IDX_MELEE_TUNN:
		case ART_IDX_GEN_TUNN:
			add_tunnelling(a_ptr);
			break;

		case ART_IDX_BOOT_FEATHER:
		case ART_IDX_GEN_FEATHER:
			add_feather_falling(a_ptr);
			break;

		case ART_IDX_GEN_SAVE:
			add_spell_resistance(a_ptr);
			break;

		case ART_IDX_GEN_DEVICE:
			add_magical_device(a_ptr);
			break;

		case ART_IDX_BOOT_STEALTH:
		case ART_IDX_CLOAK_STEALTH:
		case ART_IDX_ARMOR_STEALTH:
		case ART_IDX_GEN_STEALTH:
			add_stealth(a_ptr);
			break;

		case ART_IDX_BOOT_SPEED:
		case ART_IDX_GEN_SPEED:
			add_speed(a_ptr);
			break;

		case ART_IDX_GLOVE_FA:
		case ART_IDX_GEN_FA:
			add_free_action(a_ptr);
			break;

		case ART_IDX_GLOVE_DEX:
			add_dex(a_ptr);
			break;

		case ART_IDX_HELM_RBLIND:
		case ART_IDX_GEN_RBLIND:
			add_resist_blindness(a_ptr);
			break;

		case ART_IDX_HELM_ESP:
		case ART_IDX_GEN_ESP:
			add_telepathy(a_ptr);
			break;

		case ART_IDX_MELEE_SENSE:
			add_sense_slay(a_ptr);
			break;

		case ART_IDX_BOW_SENSE:
		case ART_IDX_NONWEAPON_SENSE:
			add_sense_rand(a_ptr);
			break;

		case ART_IDX_MELEE_RESTRICT:
		case ART_IDX_BOW_RESTRICT:
		case ART_IDX_NONWEAPON_RESTRICT:
			add_restrict_rand(a_ptr);
			break;

		case ART_IDX_HELM_WIS:
			add_wis(a_ptr);
			break;

		case ART_IDX_HELM_INT:
			add_int(a_ptr);
			break;

		case ART_IDX_SHIELD_LRES:
		case ART_IDX_ARMOR_LRES:
		case ART_IDX_GEN_LRES:
			add_low_resist(a_ptr);
			break;

		case ART_IDX_ARMOR_HLIFE:
		case ART_IDX_GEN_HLIFE:
			add_hold_life(a_ptr);
			break;

		case ART_IDX_ARMOR_CON:
			add_con(a_ptr);
			break;

		case ART_IDX_ARMOR_ALLRES:
			add_resist_acid(a_ptr);
			add_resist_lightning(a_ptr);
			add_resist_fire(a_ptr);
			add_resist_cold(a_ptr);
			break;

		case ART_IDX_ARMOR_HRES:
			add_high_resist(a_ptr);
			break;

		case ART_IDX_GEN_STAT:
			add_stat(a_ptr);
			break;

		case ART_IDX_GEN_SUST:
			add_sustain(a_ptr);
			break;

		case ART_IDX_GEN_SEARCH:
			add_search(a_ptr);
			break;

		case ART_IDX_GEN_INFRA:
			add_infravision(a_ptr);
			break;

		case ART_IDX_GEN_IMMUNE:
			add_immunity(a_ptr);
			break;

		case ART_IDX_GEN_LITE:
			add_permanent_light(a_ptr);
			break;

		case ART_IDX_GEN_SDIG:
			add_slow_digestion(a_ptr);
			break;

		case ART_IDX_GEN_REGEN:
			add_regeneration(a_ptr);
			break;

		case ART_IDX_GEN_RPOIS:
			add_resist_poison(a_ptr);
			break;

		case ART_IDX_GEN_RFEAR:
			add_resist_fear(a_ptr);
			break;

		case ART_IDX_GEN_RLITE:
			add_resist_light(a_ptr);
			break;

		case ART_IDX_GEN_RDARK:
			add_resist_dark(a_ptr);
			break;

		case ART_IDX_GEN_RCONF:
			add_resist_confusion(a_ptr);
			break;

		case ART_IDX_GEN_RSOUND:
			add_resist_sound(a_ptr);
			break;

		case ART_IDX_GEN_RSHARD:
			add_resist_shards(a_ptr);
			break;

		case ART_IDX_GEN_RNEXUS:
			add_resist_nexus(a_ptr);
			break;

		case ART_IDX_GEN_RNETHER:
			add_resist_nether(a_ptr);
			break;

		case ART_IDX_GEN_RCHAOS:
			add_resist_chaos(a_ptr);
			break;

		case ART_IDX_GEN_RDISEN:
			add_resist_disenchantment(a_ptr);
			break;
	}
}

/*
 * Randomly select an extra ability to be added to the artifact in question.
 * XXX - This function is way too large.
 */
static void add_ability(artifact_type *a_ptr)
{
	int r;

	/* Choose a random ability using the frequency table previously defined*/
	r = choose_ability(art_freq);

	/* Add the appropriate ability */
	add_ability_aux(a_ptr, r);

	/* Now remove contradictory or redundant powers. */
	remove_contradictory(a_ptr);

	/* Adding WIS to sharp weapons always blesses them */
	if ((a_ptr->flags1 & TR1_WIS) && (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM))
	{
		add_bless_weapon(a_ptr);
	}
}


/*
 * Try to supercharge this item by running through the list of the supercharge
 * abilities and attempting to add each in turn.  An artifact only gets one
 * chance at each of these up front (if applicable).
 */
static void try_supercharge(artifact_type *a_ptr)
{
	/* Huge damage dice - melee weapon only */
	if (a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_HAFTED ||
		a_ptr->tval == TV_POLEARM || a_ptr->tval == TV_SWORD)
	{
		if (rand_int (z_info->a_max) < artprobs[ART_IDX_MELEE_DICE_SUPER])
		{
			a_ptr->dd += 3 + (s16b)rand_int(4);
			if (a_ptr->dd > 9) a_ptr->dd = 9;
			LOG_PRINT1("Supercharging damage dice!  (Now %d dice)\n", a_ptr->dd);
		}
	}

	/* Bows - +3 might or +3 shots */
	if (a_ptr->tval == TV_BOW)
	{
		if (rand_int (z_info->a_max) < artprobs[ART_IDX_BOW_SHOTS_SUPER])
		{
			a_ptr->flags1 |= TR1_SHOTS;
			a_ptr->pval = 3;
			LOG_PRINT("Supercharging shots for bow!  (3 extra shots)\n");
		}
		else if (rand_int (z_info->a_max) < artprobs[ART_IDX_BOW_MIGHT_SUPER])
		{
			a_ptr->flags1 |= TR1_MIGHT;
			a_ptr->pval = 3;
			LOG_PRINT("Supercharging might for bow!  (3 extra might)\n");
		}
	}

	/* Big speed bonus - any item (potentially) */
	if (rand_int (z_info->a_max) < artprobs[ART_IDX_GEN_SPEED_SUPER])
	{
		a_ptr->flags1 |= TR1_SPEED;
		a_ptr->pval = 3 + (s16b)rand_int(3);
		LOG_PRINT1("Supercharging speed for this item!  (New speed bonus is %d)\n", a_ptr->pval);
	}
	/* Aggravation */
	if (a_ptr->tval == TV_BOW || a_ptr->tval == TV_DIGGING ||
		a_ptr->tval == TV_HAFTED || a_ptr->tval == TV_POLEARM ||
		a_ptr->tval == TV_SWORD)
	{
		if (rand_int (z_info->a_max) < artprobs[ART_IDX_WEAPON_AGGR])
		{
			a_ptr->flags3 |= TR3_AGGRAVATE;
			LOG_PRINT("Adding aggravation\n");
		}
	}
	else
	{
		if (rand_int (z_info->a_max) < artprobs[ART_IDX_NONWEAPON_AGGR])
		{
			a_ptr->flags3 |= TR3_AGGRAVATE;
			LOG_PRINT("Adding aggravation\n");
		}
	}
}

/*
 * Make it bad, or if it's already bad, make it worse!
 */
static void do_curse(artifact_type *a_ptr)
{

	int r = rand_int(21);

	r = rand_int(21);
	if (r == 0) a_ptr->flags3 |= TR3_AGGRAVATE;
	else if (r == 1) a_ptr->flags3 |= TR3_DRAIN_EXP;
	else if (r == 2) a_ptr->flags3 |= TR3_DRAIN_HP;
	else if (r == 3) a_ptr->flags3 |= TR3_DRAIN_MANA;
	else if (r == 4) a_ptr->flags3 |= TR3_UNCONTROLLED;
	else if (r == 5) a_ptr->flags4 |= TR4_HURT_LITE;
	else if (r == 6) a_ptr->flags4 |= TR4_HURT_WATER;
	else if (r == 7) a_ptr->flags3 |= TR3_HUNGER;
	else if (r == 8) a_ptr->flags4 |= TR4_ANCHOR;
	else if (r == 9) a_ptr->flags4 |= TR4_SILENT;
	else if (r == 10) a_ptr->flags4 |= TR4_STATIC;
	else if (r == 11) a_ptr->flags4 |= TR4_WINDY;
	else if (r == 12) a_ptr->flags4 |= TR4_EVIL;
	else if (r == 13) a_ptr->flags4 |= TR4_UNDEAD;
	else if (r == 14) a_ptr->flags4 |= TR4_DRAGON;
	else if (r == 15) a_ptr->flags4 |= TR4_DEMON;
	else if (r == 16) a_ptr->flags4 |= TR4_HURT_POIS;
	else if (r == 17) a_ptr->flags4 |= TR4_HURT_ACID;
	else if (r == 18) a_ptr->flags4 |= TR4_HURT_ELEC;
	else if (r == 19) a_ptr->flags4 |= TR4_HURT_FIRE;
	else if (r == 20) a_ptr->flags4 |= TR4_HURT_COLD;

	if ((a_ptr->pval > 0) && (rand_int(2) == 0))
		a_ptr->pval = -a_ptr->pval;
	if ((a_ptr->to_a > 0) && (rand_int(2) == 0))
		a_ptr->to_a = -a_ptr->to_a;
	if ((a_ptr->to_h > 0) && (rand_int(2) == 0))
		a_ptr->to_h = -a_ptr->to_h;
	if ((a_ptr->to_d > 0) && (rand_int(4) == 0))
		a_ptr->to_d = -a_ptr->to_d;

	if (a_ptr->flags3 & TR3_LIGHT_CURSE)
	{
		if (rand_int(2) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
		return;
	}

	a_ptr->flags3 |= TR3_LIGHT_CURSE;

	if (rand_int(4) == 0)
		a_ptr->flags3 |= TR3_HEAVY_CURSE;
}


/*
 * Note the three special cases (One Ring, Grond, Morgoth).
 */
static void scramble_artifact(int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	artifact_type a_old;
	object_kind *k_ptr;
	u32b activates = a_ptr->flags3 & TR3_ACTIVATE;
	s32b power;
	int tries=0;
	s16b k_idx;
	byte rarity_old, base_rarity_old;
	s16b rarity_new;
	s32b ap;
	bool curse_me = FALSE;
	bool success = FALSE;

	/* Special cases -- don't randomize these! */
	if ((a_idx == ART_POWER) ||
	    (a_idx == ART_GROND) ||
	    (a_idx == ART_MORGOTH))
		return;

	/* Skip unused artifacts, too! */
	if (a_ptr->tval == 0) return;

	/* Evaluate the original artifact to determine the power level. */
	power = base_power[a_idx];

	/* If it has a restricted ability then don't randomize it. */
	if (power > 10000)
	{
		LOG_PRINT1("Skipping artifact number %d - too powerful to randomize!", a_idx);
		return;
	}

	if (power < 0) curse_me = TRUE;

	LOG_PRINT("+++++++++++++++++ CREATING NEW ARTIFACT ++++++++++++++++++\n");
	LOG_PRINT2("Artifact %d: power = %d\n", a_idx, power);

	/*
	 * Flip the sign on power if it's negative, since it's only used for base
	 * item choice
	 */
	if (power < 0) power = -power;

	if (a_idx >= ART_MIN_NORMAL)
	{
		/*
		 * Normal artifact - choose a random base item type.  Not too
		 * powerful, so we'll have to add something to it.  Not too
		 * weak, for the opposite reason.  We also require a new
		 * rarity rating of at least 2.
		 *
		 * CR 7/15/2001 - lowered the upper limit so that we get at
		 * least a few powers (from 8/10 to 6/10) but permit anything
		 * more than 20 below the target power
		 */
		int count = 0;
		s32b ap2;

		/* Capture the rarity of the original base item and artifact */
		base_rarity_old = base_item_rarity[a_idx];
		rarity_old = base_art_rarity[a_idx];
		do
		{
			k_idx = choose_item(a_idx);

			/*
			 * Hack: if power is positive but very low, and if we're not having
			 * any luck finding a base item, curse it once.  This helps ensure
			 * that we get a base item for borderline cases like Wormtongue.
			 */

			if (power > 0 && power < 10 && count > MAX_TRIES / 2)
			{
				LOG_PRINT("Cursing base item to help get a match.\n");
				do_curse(a_ptr);
				remove_contradictory(a_ptr);
			}

			else if (power >= 10 && power < 20 && count > MAX_TRIES / 2)
			{
				LOG_PRINT("Restricting base item to help get a match.\n");
				add_restrict_rand(a_ptr);
				remove_contradictory(a_ptr);
			}

			ap2 = artifact_power(a_idx);
			count++;
			/*
			 * Calculate the proper rarity based on the new type.  We attempt
			 * to preserve the 'effective rarity' which is equal to the
			 * artifact rarity multiplied by the base item rarity.
			 */
			k_ptr = &k_info[k_idx];
			rarity_new = ( (s16b) rarity_old * (s16b) base_rarity_old ) /
				(s16b) (k_ptr->chance[0] ? k_ptr->chance[0] : base_rarity_old);

			if (rarity_new > 255) rarity_new = 255;
			if (rarity_new < 1) rarity_new = 1;

		} while ( (count < MAX_TRIES) &&
			(((ap2 > (power * 6) / 10 + 1) && (power-ap2 < 20)) ||
			(ap2 < (power / 10)) || rarity_new == 1) );

		/* Got an item - set the new rarity */
		a_ptr->rarity = (byte) rarity_new;
	}
	else
	{
		/* Special artifact (light source, ring, or
		   amulet).  Clear the following fields; leave
		   the rest alone. */
		a_ptr->pval = 0;
		a_ptr->to_h = a_ptr->to_d = a_ptr->to_a = 0;
		a_ptr->flags1 = a_ptr->flags2 = 0;

		/* Artifacts ignore everything */
		a_ptr->flags2 = (TR2_IGNORE_MASK);
	}

	/* Got a base item. */

	/* Generate the cumulative frequency table for this item type */
	build_freq_table(a_ptr, art_freq);

	/* Copy artifact info temporarily. */
	COPY(&a_old, a_ptr, artifact_type);

	/* Give this artifact a shot at being supercharged */
	try_supercharge(a_ptr);
	ap = artifact_power(a_idx);
	if (ap > (power * 23) / 20 + 1)
	{
		/* too powerful -- put it back */
		*a_ptr = a_old;
		LOG_PRINT("--- Supercharge is too powerful!  Rolling back.\n");
	}

	/* First draft: add two abilities, then curse it three times. */
	if (curse_me)
	{
		/* Copy artifact info temporarily. */
		COPY(&a_old, a_ptr, artifact_type);

		do
		{
			add_ability(a_ptr);
			add_ability(a_ptr);
			do_curse(a_ptr);
			do_curse(a_ptr);
			do_curse(a_ptr);
			remove_contradictory(a_ptr);
			ap = artifact_power(a_idx);
			/* Accept if it doesn't have any inhibited abilities */
			if (ap < 10000) success = TRUE;
			/* Otherwise go back and try again */
			else
			{
				LOG_PRINT("Inhibited ability added - rolling back.\n");
				COPY(a_ptr, &a_old, artifact_type);

			}
		} while (!success);
	}
	else
	{

		/*
		 * Select a random set of abilities which roughly matches the
		 * original's in terms of overall power/usefulness.
		 */
		for (tries = 0; tries < MAX_TRIES; tries++)
		{

			/* Copy artifact info temporarily. */
			COPY(&a_old, a_ptr, artifact_type);
			add_ability(a_ptr);
			ap = artifact_power(a_idx);


			/* CR 11/14/01 - pushed both limits up by about 5% */
			if (ap > (power * 23) / 20 + 1)
			{
				/* too powerful -- put it back */
				COPY(a_ptr, &a_old, artifact_type);
				continue;
			}

			else if (ap >= (power * 18) / 20)	/* just right */
			{

				/* Hack -- add a restriction on the most powerful artifacts */
				if (ap > 150)
				{
					add_restrict_rand(a_ptr);
					remove_contradictory(a_ptr);
				}

				break;
			}

			/* Stop if we're going negative, so we don't overload
			   the artifact with great powers to compensate. */
			/* Removed CR 11/10/01 */
			/*
			else if ((ap < 0) && (ap < (-(power * 1)) / 10))
			{
				break;
			}
			*/
		}		/* end of power selection */

		if (randart_verbose && tries >= MAX_TRIES)
		{
			/*
			 * We couldn't generate an artifact within the number of permitted
			 * iterations.  Show a warning message.
			 */
			msg_format("Warning!  Couldn't get appropriate power level.");
			LOG_PRINT("Warning!  Couldn't get appropriate power level.\n");
			msg_print(NULL);
		}

	}

	a_ptr->cost = ap * 1000L;

	if (a_ptr->cost < 0) a_ptr->cost = 0;

	/* Restore some flags */
	if (activates) a_ptr->flags3 |= TR3_ACTIVATE;

	/* Store artifact power */
	a_ptr->power = ap;

	/* Success */
	LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>> ARTIFACT COMPLETED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	LOG_PRINT2("Number of tries for artifact %d was: %d\n", a_idx, tries);
	LOG_PRINT1("Final artifact power is %d\n",ap);
}

/*
 * Return nonzero if the whole set of random artifacts meets certain
 * criteria.  Return 0 if we fail to meet those criteria (which will
 * restart the whole process).
 */
static int artifacts_acceptable(void)
{
	int swords = 5, polearms = 5, blunts = 5, bows = 4;
	int bodies = 5, shields = 4, cloaks = 4, hats = 4;
	int gloves = 4, boots = 4;
	int i;

	for (i = ART_MIN_NORMAL; i < z_info->a_max; i++)
	{
		switch (a_info[i].tval)
		{
			case TV_SWORD:
				swords--; break;
			case TV_POLEARM:
				polearms--; break;
			case TV_HAFTED:
				blunts--; break;
			case TV_BOW:
				bows--; break;
			case TV_SOFT_ARMOR:
			case TV_HARD_ARMOR:
			case TV_DRAG_ARMOR:
				bodies--; break;
			case TV_SHIELD:
				shields--; break;
			case TV_CLOAK:
				cloaks--; break;
			case TV_HELM:
			case TV_CROWN:
				hats--; break;
			case TV_GLOVES:
				gloves--; break;
			case TV_BOOTS:
				boots--; break;
		}
	}

	LOG_PRINT1("Deficit amount for swords is %d\n", swords);
	LOG_PRINT1("Deficit amount for polearms is %d\n", polearms);
	LOG_PRINT1("Deficit amount for blunts is %d\n", blunts);
	LOG_PRINT1("Deficit amount for bows is %d\n", bows);
	LOG_PRINT1("Deficit amount for bodies is %d\n", bodies);
	LOG_PRINT1("Deficit amount for shields is %d\n", shields);
	LOG_PRINT1("Deficit amount for cloaks is %d\n", cloaks);
	LOG_PRINT1("Deficit amount for hats is %d\n", hats);
	LOG_PRINT1("Deficit amount for gloves is %d\n", gloves);
	LOG_PRINT1("Deficit amount for boots is %d\n", boots);

	if (swords > 0 || polearms > 0 || blunts > 0 || bows > 0 ||
	    bodies > 0 || shields > 0 || cloaks > 0 || hats > 0 ||
	    gloves > 0 || boots > 0)
	{
		if (randart_verbose)
		{
			char types[256];
			sprintf(types, "%s%s%s%s%s%s%s%s%s%s",
				swords > 0 ? " swords" : "",
				polearms > 0 ? " polearms" : "",
				blunts > 0 ? " blunts" : "",
				bows > 0 ? " bows" : "",
				bodies > 0 ? " body-armors" : "",
				shields > 0 ? " shields" : "",
				cloaks > 0 ? " cloaks" : "",
				hats > 0 ? " hats" : "",
				gloves > 0 ? " gloves" : "",
				boots > 0 ? " boots" : "");
			msg_format("Restarting generation process: not enough %s", types);
			LOG_PRINT(format("Restarting generation process: not enough %s", types));
		}
		return (0);
	}
	else
	{
		return (1);
	}
}


static errr scramble(void)
{
	/* If our artifact set fails to meet certain criteria, we start over. */
	do
	{
		int a_idx;

		/* Note boundary condition. We only scramble the artifacts about the old
			z_info->a_max if adult_randarts is not set. */
		/* Generate all the artifacts. */
		for (a_idx = (adult_randarts ? 1 : z_info->a_max_standard) ; a_idx < z_info->a_max; a_idx++)
		{
			scramble_artifact(a_idx);
		}
	} while (!artifacts_acceptable());	/* end of all artifacts */

	/* Success */
	return (0);
}


static errr do_randart_aux(bool full)
{
	errr result;

	/* Generate random names */
	if ((result = init_names()) != 0) return (result);

	if (full)
	{
		/* Randomize the artifacts */
		if ((result = scramble()) != 0) return (result);
	}

	/* Success */
	return (0);
}

/*
 * Randomize the artifacts
 *
 * The full flag toggles between just randomizing the names and
 * complete randomization of the artifacts.
 */
errr do_randart(u32b randart_seed, bool full)
{
	errr err = 0;

	int i;
	u32b j;

	/* Prepare to use the Angband "simple" RNG. */
	Rand_value = randart_seed;
	Rand_quick = TRUE;

	/* Open the log file for writing */
	if (randart_verbose)
	{
		if ((randart_log = my_fopen("randart.log", "w")) == NULL)
		{
			msg_format("Error - can't open randart.log for writing.");
			exit(1);
		}

		LOG_PRINT("===============================\n");
		LOG_PRINT1("Seed is %d.\n", randart_seed);
		LOG_PRINT1("Total monster power is %d.\n", tot_mon_power);


		for (j = 1L; j < SLAY_MAX; j <<= 2)
		{
			LOG_PRINT1("Slay index is %d.\n", slay_power(j) * 1000 / tot_mon_power);
		}
	}


	/* Only do all the following if full randomization requested */
	if (full)
	{
		int i;

		int art_high_slot = z_info->a_max - 1;

		/* Copy base artifacts to seed random powers */
		for (i = ART_MIN_NORMAL; i < z_info->a_max_standard;i++)
		{
			artifact_type *a_ptr = &a_info[i];
			artifact_type *a2_ptr = &a_info[art_high_slot];

			if (a_info[i].tval == 0) continue;
			if (i == ART_POWER) continue;
			if (i == ART_GROND) continue;
			if (i == ART_MORGOTH) continue;

			COPY(a2_ptr,a_ptr,artifact_type);
			if (--art_high_slot < z_info->a_max_standard)
				break;
		}

		/* Allocate the "kinds" array */
		kinds = C_ZNEW(z_info->a_max, s16b);

		/* Allocate the various "original powers" arrays */
		base_power = C_ZNEW(z_info->a_max, s32b);
		base_item_level = C_ZNEW(z_info->a_max, byte);
		base_item_rarity = C_ZNEW(z_info->a_max, byte);
		base_art_rarity = C_ZNEW(z_info->a_max, byte);

		/* Store the original power ratings */
		store_base_power();

		/* Determine the generation probabilities */
		parse_frequencies();
	}

	/* Generate the random artifact (names) */
	err = do_randart_aux(full);

	/* Only do all the following if full randomization requested */
	if (full)
	{
		if (randart_verbose)
		{
			/* Just for fun, look at the frequencies on the finished items */
			store_base_power();
			parse_frequencies();

			/* Report artifact powers */
			for (i = 0; i < z_info->a_max; i++)
			{
				LOG_PRINT2("Artifact %d power is now %d\n",i,a_info[i].power);
			}
		}

		/* Free the "kinds" array */
		FREE(kinds);

		/* Free the "original powers" arrays */
		FREE(base_power);
		FREE(base_item_level);
		FREE(base_item_rarity);
		FREE(base_art_rarity);

	}

	/* Close the log file */
	if (randart_verbose)
	{
		if (my_fclose(randart_log) != 0)
		{
			msg_format("Error - can't close randart.log file.");
			exit(1);
		}
	}

	/* When done, resume use of the Angband "complex" RNG. */
	Rand_quick = FALSE;

	return (err);
}

#endif /* GJW_RANDART */

