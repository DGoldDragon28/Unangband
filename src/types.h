/* File: types.h */

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


/*
 * Note that "char" may or may not be signed, and that "signed char"
 * may or may not work on all machines.  So always use "s16b" or "s32b"
 * for signed values.  Also, note that unsigned values cause math problems
 * in many cases, so try to only use "u16b" and "u32b" for "bit flags",
 * unless you really need the extra bit of information, or you really
 * need to restrict yourself to a single byte for storage reasons.
 *
 * Also, if possible, attempt to restrict yourself to sub-fields of
 * known size (use "s16b" or "s32b" instead of "int", and "byte" instead
 * of "bool"), and attempt to align all fields along four-byte words, to
 * optimize storage issues on 32-bit machines.  Also, avoid "bit flags"
 * since these increase the code size and slow down execution.  When
 * you need to store bit flags, use one byte per flag, or, where space
 * is an issue, use a "byte" or "u16b" or "u32b", and add special code
 * to access the various bit flags.
 *
 * Many of these structures were developed to reduce the number of global
 * variables, facilitate structured program design, allow the use of ascii
 * template files, simplify access to indexed data, or facilitate efficient
 * clearing of many variables at once.
 *
 * Note that certain data is saved in multiple places for efficient access,
 * and when modifying the data in one place it must also be modified in the
 * other places, to prevent the creation of inconsistant data.
 */



/**** Available Types ****/


/*
 * An array of 256 byte's
 */
typedef byte byte_256[256];

/*
 * An array of 256 s16b's
 */
typedef s16b s16b_256[256];


/*
 * An array of DUNGEON_WID byte's
 */
typedef byte byte_wid[DUNGEON_WID];

/*
 * An array of DUNGEON_WID s16b's
 */
typedef s16b s16b_wid[DUNGEON_WID];


/**** Available Function Definitions ****/

typedef void (*print_list_func)(const s16b *sn, int num, int y, int x);
typedef bool (*list_command_func)(char choice, const s16b *sn, int i, bool *redraw);


typedef bool (*tester_attribute_func)(int y, int x);
typedef void (*modify_attribute_func)(int y, int x);
typedef int (*modify_attribute_func_remove)(int y, int x);
typedef void (*modify_attribute_func_reapply)(int y, int x, int r);


/**** Available Structs ****/


typedef struct maxima maxima;
typedef struct dungeon_zone dungeon_zone;
typedef struct town_type town_type;
typedef struct method_desc_type method_desc_type;
typedef struct method_level_scalar_type method_level_scalar_type;
typedef struct method_type method_type;
typedef struct effect_type effect_type;
typedef struct desc_type desc_type;
typedef struct room_info_type room_info_type;
typedef struct feature_state feature_state;
typedef struct feature_blow feature_blow;
typedef struct feature_type feature_type;
typedef struct object_kind object_kind;
typedef struct object_info object_info;
typedef struct object_lore object_lore;
typedef struct artifact_type artifact_type;
typedef struct names_type names_type;
typedef struct ego_item_type ego_item_type;
typedef struct flavor_type flavor_type;
typedef struct monster_blow monster_blow;
typedef struct monster_race monster_race;
typedef struct monster_lore monster_lore;
typedef struct vault_type vault_type;
typedef struct region_piece_type region_piece_type;
typedef struct region_info_type region_info_type;
typedef struct region_type region_type;
typedef struct object_grouper object_grouper;
typedef struct object_type object_type;
typedef struct monster_type monster_type;
typedef struct alloc_entry alloc_entry;
typedef struct quest_event quest_event;
typedef struct quest_type quest_type;
typedef struct owner_type owner_type;
typedef struct store_type store_type;
typedef store_type *store_type_ptr;
typedef struct skill_table_entry skill_table_entry;
typedef struct player_sex player_sex;
typedef struct player_race player_race;
typedef struct player_class player_class;
typedef struct weapon_style weapon_style;
typedef struct spell_appears spell_appears;
typedef struct spell_cast spell_cast;
typedef struct spell_blow spell_blow;
typedef struct spell_type spell_type;
typedef struct rune_type rune_type;
typedef struct hist_type hist_type;
typedef struct player_other player_other;
typedef struct quickstart_type quickstart_type;
typedef struct familiar_type familiar_type;
typedef struct familiar_ability_type familiar_ability_type;
typedef struct player_type player_type;
typedef struct start_item start_item;
typedef struct tval_desc tval_desc;
typedef struct element_type element_type;
typedef struct quiver_group_type quiver_group_type;
typedef struct ecology_type ecology_type;
typedef struct do_cmd_item_type do_cmd_item_type;
typedef struct color_type color_type;
typedef struct timed_effect timed_effect;
typedef struct sound_name_type sound_name_type;




/**** Available structs ****/


/*
 * Information about maximal indices of certain arrays
 * Actually, these are not the maxima, but the maxima plus one
 */
struct maxima
{
	u32b fake_text_size;
	u32b fake_name_size;

	u16b d_max;     /* Max size for "d_info[]" */
	u16b f_max;     /* Max size for "f_info[]" */

	u16b a_max_standard; /* Max size for standard artifacts */
	u16b a_max;     /* Max size for "a_info[]" */
	u16b e_max;     /* Max size for "e_info[]" */

	u16b k_max;     /* Max size for "k_info[]" */
	u16b x_max;     /* Max size for "x_info[]" */

	u16b q_max;	/* Max size for "q_info[]" */
	u16b v_max;     /* Max size for "v_info[]" */

	u16b r_max;     /* Max size for "r_info[]" */
	u16b p_max;     /* Max size for "p_info[]" */

	u16b g_max;     /* Max size for "g_info[]" */
	u16b h_max;     /* Max size for "h_info[]" */

	u16b t_max;	/* Max size for "t_info[]" */
	u16b u_max;     /* Max size for "u_info[]" */

	u16b b_max;     /* Max size per element of "b_info[]" */
	u16b c_max;    	/* Max size per element of "c_info[]" */

	u16b s_max;	/* Max size per element of "s_info[]" */
	u16b y_max;     /* Max size per element of "y_info[]" */

	u16b w_max;	/* Max size per element of "w_info[]" */
	u16b method_max;	/* Max size per element of "blow_info[]" */
	u16b effect_max;	/* Max size per element of "effect_info[]" */

	u16b o_max;     /* Max size for "o_list[]" */
	u16b m_max;     /* Max size for "m_list[]" */

	u16b region_piece_max;	/* Max size for "region_piece_list" */
	u16b region_max;		/* Max size for "region_list" */
	u16b region_info_max;		/* Max size for "region_list" */
};



/*
 * Dungeon zone structure
 *
 */
struct dungeon_zone
{
	u32b name;     /* Text (offset) */

	s16b level;
	s16b special;

	s16b base;		/* Base terrain */
	s16b fill;		/* Terrain drawn over by */
	s16b big;		/* 'Big' terrain */
	s16b small;		/* 'Little' terrain */

	u16b guard;
	s16b tower;

	u32b flags1;	/* Level flags */
	u32b flags2;	/* Room flags for whole level */
};


/*
 * Dungeon level structure
 *
 */

struct town_type
{
	u32b name;     /* Name (offset) */
	u32b text;     /* Text (offset) */

	u16b nearby[MAX_NEARBY];

	u16b quest_opens;
	u16b quest_monster;

	u16b replace_with;
	u16b replace_ifvisited;

	u16b replace_guardian;
	u16b guardian_ifvisited;

	u16b town_lockup_monster;
	u16b town_lockup_ifvisited;

	byte r_char;    /* Add races of this char */
	u16b r_flag;    /* Add races with this flag */
	byte unused;	/* Unused */

	s16b chasm;		/* Hidden 'chasm' exit */

	byte attained_depth; /* Absolute, not from dungeon surface */
	byte visited;

	u16b store[MAX_STORES];
	u16b store_index[MAX_STORES];

	dungeon_zone zone[MAX_DUNGEON_ZONES];

	byte a_char;	/* Use races of this char as allies for BATTLE levels */
	u32b a_flag;	/* Use races with this flag as allies for BATTLE levels */
};


/*
 * Text used for blow descriptions
 */
struct method_desc_type
{
	s16b min;
	s16b max;
	u32b text;
};


/*
 * Scalar for blow effect per level
 *
 * Returned value is value + gain * actual_level / levels;
 */
struct method_level_scalar_type
{
	s16b base;
	s16b gain;
	s16b levels;
};


/*
 * Information about blow and projection types.
 */
struct method_type
{
	u32b name;     /* Name (offset) */

	u32b flags1;	/* Projection flags */
	u32b flags2;	/* Melee and spell flags */

	method_desc_type desc[MAX_METHOD_DESCRIPTIONS];	/* Possible text strings describing blow */
	u32b info[2];	/* Partial text strings to provide info about blow */

	u16b sound;
	u16b ammo_kind;	/* Ammunition kind - lookup is cached for efficiency */

	byte ammo_tval;
	byte ammo_sval;

	byte arc;
	byte diameter_of_source;
	byte degree_factor;

	byte mana_cost;
	byte best_range;

	/* Monter summoning */
	byte summon_type;
	byte unused;

	/* Monster spell/breath damage  */
	byte dam_mult;
	byte dam_div;
	byte dam_var;
	byte dam_div_powerful;

	s16b dam_max;
	s16b dam_max_powerful;

	/* Monster spell desirability */
	byte d_base;
	byte d_hurt;
	byte d_esc;
	byte d_res;

	byte d_summ;
	byte d_mana;
	byte d_tact;
	byte d_range;

	method_level_scalar_type radius;
	method_level_scalar_type number;
	method_level_scalar_type max_range;
};


/*
 * Information about effect types.
 */
struct effect_type
{
	u32b name;     /* Name (offset) */

	u32b info[7];     /* Info (offset) */
	u32b desc[2];     /* Description (offset) */

	u32b flags1;	/* Projection flags */

	byte power;		/* Attack power */
	byte unused;

	byte attr_vert;      /* Spell graphics */
	char char_vert;

	byte attr_horiz;
	char char_horiz;

	byte attr_rdiag;
	char char_rdiag;

	byte attr_ldiag;
	char char_ldiag;

	byte attr_ball;
	char char_ball;
};


#define THEME_TUNNEL	0
#define THEME_SOLID		1
#define THEME_TRAP		2
#define THEME_BRIDGE	3
#define MAX_THEMES		4

/*
 * Information about room descriptions
 */
struct desc_type
{
	u32b name1;     /* Name (offset) */
	u32b name2;
	u32b text;      /* Text (offset) */

	u32b flags;     /* Room flags */
	u32b p_flag;	/* Description placement flags */

	byte chance;  	/* Frequency of this entry */
	byte not_chance;/* Frequency of this entry if conditions not met */
	byte chart; 	/* Chart index */
	byte next;  	/* Next chart index */

	byte branch;	/* Branch to chart index */
	byte branch_on;	/* Branch on chart index */

	byte level_min; /* Minimum level */
	byte level_max;	/* Maximum level */

	u32b l_flag;    /* Restrict to these level types */
	u16b r_flag;    /* Restrict to levels with these monster types */

	s16b feat;      /* Extra features of this type */
	s16b theme[MAX_THEMES];	/* Theme parts of this room using these features */

	byte tval;      /* Add objects of this tval */
	byte min_sval;  /* And from this sval */
	byte max_sval;  /*   ... to this sval */
	byte r_char;    /* Add races of this char */
};






/*
 * Description of a room
 */
struct room_info_type
{
	s16b type;				/* Type of room (normal/pit) */
	s16b vault;				/* Vault chosen */
	s16b section[ROOM_DESC_SECTIONS];	/* Array of room descriptions */

	s16b deepest_race;	/* Deepest race in this ecology */
	u32b ecology;	/* What ecologies appear in the room */

	u32b flags;		/* Room flags */

	/* Decorations: TODO some of these could be discarded after dungeon generation*/

	s16b	theme[MAX_THEMES];	/* Features to use for placement around the room */

#if 0
	s16b	solid;		/* Feature to use as solid wall */
	s16b	tunnel;		/* Feature to use as tunnel */
	s16b	trap;		/* Feature to use as trap */
	s16b	bridge;		/* Feature to use as bridge over chasm */

	byte d_attr[5];    	/* Desired feature attribute (basic / inner / outer / solid) */
	char d_char[5];    	/* Desired feature character (basic / inner / outer / solid) */

	byte x_attr[5];    	/* Desired feature attribute (basic / inner / outer / solid) */
	char x_char[5];    	/* Desired feature character (basic / inner / outer / solid) */
#endif
};


/*
 * Feature state structure
 *
 *    - Action (FS_*)
 *   - Result (FEAT_*)
 */
struct feature_state
{
	byte action;
	byte power;
	s16b result;
};

/*
 * Feature blow structure
 *
 *    - Method (RBM_*)
 *   - Effect (GF_*)
 *  - Damage Dice
 * - Damage Sides
 */
struct feature_blow
{
	byte method;
	byte effect;
	byte d_dice;
	byte d_side;
};




/*
 * Information about terrain "features"
 */
struct feature_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	s16b mimic;     /* Feature to mimic */
	s16b edge;

	u32b flags1;
	u32b flags2;
	u32b flags3;
#if 0
	u32b flows;    	/* Flow flags */
#endif
	u16b level;     /* Minimum level */
	u16b rarity;    /* 1/Rarity */

	u16b priority;  /* Map priority */
	s16b defaults;     /* Default state */

	feature_blow blow;

	feature_state state[MAX_FEAT_STATES];

	s16b k_idx;    /* Object at this location */
	s16b unseen;
	s16b under;	/* Default feature under this feature */

	byte spell;
	byte power;

	byte d_attr;    /* Default feature attribute */
	char d_char;    /* Default feature character */

	byte x_attr;    /* Desired feature attribute */
	char x_char;    /* Desired feature character */
};


/*
 * Information about object "kinds", including player knowledge.
 *
 * Only "aware" and "tried" are saved in the savefile
 */
struct object_kind
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	byte tval;      /* Object type */
	byte sval;      /* Object sub type */

	s16b pval;      /* Object extra info */
	s16b charges;   /* Object charges info */

	s16b to_h;      /* Bonus to hit */
	s16b to_d;      /* Bonus to damage */
	s16b to_a;      /* Bonus to armor */

	s16b ac;	/* Base armor */

	byte dd, ds;    /* Damage dice/sides */

	s16b weight;    /* Weight */

	s32b cost;      /* Object "base cost" */

	u32b flags1;    /* Flags, set 1 */
	u32b flags2;    /* Flags, set 2 */
	u32b flags3;    /* Flags, set 3 */
	u32b flags4;    /* Flags, set 4 */

	u32b flags5;	/* Flags, set 5 (kind only) */
	u32b flags6;	/* Flags, set 6 (kind only) */

	byte locale[4]; /* Allocation level(s) */
	byte chance[4]; /* Allocation chance(s) */

	byte level;     /* Level */
	byte guess;     /* Guessed "sval" */

	byte d_attr;    /* Default object attribute */
	char d_char;    /* Default object character */


	byte x_attr;    /* Desired object attribute */
	char x_char;    /* Desired object character */


	s16b flavor;    /* Special object flavor (or zero) */
	u16b note;     /* Auto-inscription */

	byte runest;   /* Rune type */
	byte runesc;   /* Rune count */

	u16b aware;     /* The player is "aware" of the item. */

	s16b used;		/* Number of times used in this life */
	s16b ever_used;	/* Number of times used 'ever' */
};


/*
 * Information known about objects.
 *
 * This is used for flavors/artifacts which only record whether
 * or not an object has an ability. Ego items, which 'may' have
 * abilities use the object_lore structure instead.
 */
struct object_info
{
	u32b can_flags1;
	u32b can_flags2;
	u32b can_flags3;
	u32b can_flags4;

	u32b not_flags1;
	u32b not_flags2;
	u32b not_flags3;
	u32b not_flags4;
};


/*
 * Lore about "objects".
 */
struct object_lore
{
	u32b can_flags1;
	u32b can_flags2;
	u32b can_flags3;
	u32b can_flags4;

	u32b may_flags1;
	u32b may_flags2;
	u32b may_flags3;
	u32b may_flags4;

	u32b not_flags1;
	u32b not_flags2;
	u32b not_flags3;
	u32b not_flags4;
};



/*
 * Information about "artifacts".
 *
 * Note that the save-file only writes "cur_num" to the savefile.
 *
 * Note that "max_num" is always "1" (if that artifact "exists")
 */
struct artifact_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	byte tval;      /* Artifact type */
	byte sval;      /* Artifact sub type */

	s16b pval;      /* Artifact extra info */

	s16b to_h;      /* Bonus to hit */
	s16b to_d;      /* Bonus to damage */
	s16b to_a;      /* Bonus to armor */

	s16b ac;/* Base armor */

	byte dd, ds;    /* Damage when hits */

	s16b weight;    /* Weight */

	s32b cost;      /* Artifact "cost" */

	u32b flags1;    /* Artifact Flags, set 1 */
	u32b flags2;    /* Artifact Flags, set 2 */
	u32b flags3;    /* Artifact Flags, set 3 */
	u32b flags4;    /* Artifact Flags, set 4 */

	byte level;     /* Artifact level */
	byte rarity;    /* Artifact rarity */

	byte cur_num;   /* Number created (0 or 1) */
	byte max_num;   /* Unused (should be "1") */

	s16b activation;/* Activation to use */
	u16b time;      /* Activation time */
	u16b randtime;  /* Activation time dice */

	s16b activated; /* Count of times activated */

	s32b power;	/* Pre-computed power */
};


/*structure of letter probabilitiesfor the random name generator*/
struct names_type
{
	u16b lprobs[S_WORD+1][S_WORD+1][S_WORD+1];
	u16b ltotal[S_WORD+1][S_WORD+1];
};



/*
 * Information about "ego-items".
 */
struct ego_item_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	byte slot;      /* Standard slot value */
	byte rating;    /* Rating boost */

	byte level;     /* Minimum level */
	byte rarity;    /* Object rarity */

	byte tval[3];   /* Legal tval */
	byte min_sval[3];       /* Minimum legal sval */
	byte max_sval[3];       /* Maximum legal tval */
	byte xtra;      /* Extra Sustain/Resist/Power */

	s16b max_to_h;  /* Maximum to-hit bonus */
	s16b max_to_d;  /* Maximum to-dam bonus */
	s16b max_to_a;  /* Maximum to-ac bonus */

	s16b max_pval;  /* Maximum pval */

	s32b cost;      /* Ego-item "cost" */

	u32b flags1;    /* Ego-Item Flags, set 1 */
	u32b flags2;    /* Ego-Item Flags, set 2 */
	u32b flags3;    /* Ego-Item Flags, set 3 */
	u32b flags4;    /* Ego-Item Flags, set 4 */

	u32b obv_flags1;    /* Obvious Ego-Item Flags, set 1 */
	u32b obv_flags2;    /* Obvious Ego-Item Flags, set 2 */
	u32b obv_flags3;    /* Obvious Ego-Item Flags, set 3 */
	u32b obv_flags4;    /* Obvious Ego-Item Flags, set 3 */

	u16b note;     /* Auto-inscription */

	byte runest;   /* Rune type */
	byte runesc;   /* Rune count */

    u16b aware;

    s16b activation;/* Activation to use */
    u16b time;      /* Activation time */
    u16b randtime;  /* Activation time dice */

    s16b activated; /* Count of times activated */

    s32b slay_power;	/* Pre-computed power from brands/slays */
};

struct flavor_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	byte tval;      /* Associated object type */
	byte sval;      /* Associated object sub-type */

	byte d_attr;    /* Default flavor attribute */
	char d_char;    /* Default flavor character */

	byte x_attr;    /* Desired flavor attribute */
	char x_char;    /* Desired flavor character */
};

/*
 * Monster blow structure
 *
 *      - Method (RBM_*)
 *      - Effect (RBE_*)
 *      - Damage Dice
 *      - Damage Sides
 */
struct monster_blow
{
	byte method;
	byte effect;
	byte d_dice;
	byte d_side;
};



/*
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Maybe "x_attr", "x_char", "cur_num", and "max_num" should
 * be moved out of this array since they are not read from
 * "r_info.txt".
 */
struct monster_race
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	s16b hp;     /* Creatures average hit points */

	byte hdice;
	byte hside;

	s16b ac;	/* Armour Class */

	s16b sleep;     /* Inactive counter (base) */
	byte aaf;       /* Area affect radius (1-100) */
	byte speed;     /* Speed (normally 110) */

	s32b mexp;      /* Exp value for kill */
	s16b power;     /* Power of monster for slays */

	byte freq_innate;/* Inate spell frequency */
	byte freq_spell;/* Other spell frequency */
	byte mana;	/* Mana */
	byte spell_power;/* Spell power */

	u32b flags1;    /* Flags 1 (general) */
	u32b flags2;    /* Flags 2 (abilities) */
	u32b flags3;    /* Flags 3 (race/resist) */
	u32b flags4;    /* Flags 4 (inate/breath) */
	u32b flags5;    /* Flags 5 (normal spells) */
	u32b flags6;    /* Flags 6 (special spells) */
	u32b flags7;    /* Flags 7 (summons) */
	u32b flags8;	/* Flags 8 (drops) */
	u32b flags9;	/* Flags 9 (extra) */
#if 0
	u32b flows;	/* Flow flags */
#endif
	monster_blow blow[4];   /* Up to four blows per round */

	byte level;     /* Level of creature */
	byte rarity;    /* Rarity of creature */
	byte d_attr;    /* Default monster attribute */
	char d_char;    /* Default monster character */

	byte x_attr;    /* Desired monster attribute */
	char x_char;    /* Desired monster character */
	byte max_num;   /* Maximum population allowed per level */
	byte cur_num;   /* Monster population on current level */

	byte grp_idx;	/* Monster group index */
	byte pad;
	s16b pad2;

	byte best_spell;	/* Best attack spell */
	s16b highest_threat;	/* Computed highest threat */
	byte best_threat;	/* Best threat (one off attack) */
#if 0
	s16b note;      /* Inscribe body parts with */
#endif
	byte calculated_level;     /* Calculated level of creature */
	s32b calculated_mexp;      /* Calculated exp value for kill */
	s32b forced_mexp;          /* Exp at the level forced in monster.txt */
};


/*
 * Monster "lore" information
 *
 * Note that these fields are related to the "monster recall" and can
 * be scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc). XXX XXX XXX
 *
 * ToDo: The "r_" prefix is no longer needed and should be removed.
 */
struct monster_lore
{
	s16b sights;  /* Count sightings of this monster */
	s16b deaths;  /* Count deaths from this monster */

	s16b pkills;  /* Count monsters killed in this life */
	s16b tkills;  /* Count monsters killed in all lives */

	s16b tblows;  /* Count number of times you have attacked monster with 'missable' attack */
	s16b tdamage;  /* Count number of times you have damaged monster */
	
	byte wake;    /* Number of times woken up (?) */
	byte ignore;  /* Number of times ignored (?) */

	byte xtra1;   /* Something (unused) */
	byte xtra2;   /* Something (unused) */

	byte drop_gold;       /* Max number of gold dropped at once */
	byte drop_item;       /* Max number of item dropped at once */

	byte cast_innate;      /* Max number of innate spells seen */
	byte cast_spell;      /* Max number of other spells seen */

	byte blows[4];/* Number of times each blow type was seen */

	u32b flags1;  /* Observed racial flags */
	u32b flags2;  /* Observed racial flags */
	u32b flags3;  /* Observed racial flags */
	u32b flags4;  /* Observed racial flags */
	u32b flags5;  /* Observed racial flags */
	u32b flags6;  /* Observed racial flags */
	u32b flags7;  /* Observed racial flags */
	u32b flags8;  /* Observed racial flags */
	u32b flags9;	/* Flags 9 (extra) */
};



/*
 * Information about "vault generation"
 */
struct vault_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	byte typ;       /* Vault type */

	byte rat;       /* Vault rating */
	s16b rarity;	/* Vault rarity */
	
	u32b level_flag;	/* Vault requires this flag on level */

	byte hgt;       /* Vault height */
	byte wid;       /* Vault width */

	byte min_lev;	/* Minimum level */
	byte max_lev;	/* Maximum level */
};


/*
 * Information about which grids lie in a particular "region"
 */
struct region_piece_type
{
	byte y;
	byte x;
	s16b d;					/* Scalar, usually distance */

	s16b region;			/* Index of region this region piece refers to */

	s16b next_in_grid;		/* Index of next region piece at this square. */
	s16b next_in_region;	/* Index of next region piece with the same effect. */
};


/*
 * Information about initialising various types of regions
 */
struct region_info_type
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	s16b method;	/* Instead of affecting all grids in region, use this method, if defined. */

	s16b child_region;	/* Instead of affecting all grids in region, create a region with this type instead. */
	method_level_scalar_type child_lasts;	/* How long does the child last for? */

	s16b delay_reset;		/* Number of turns to reset counter to when countdown has finished */

	u32b flags1;	/* Region bitflags */

	byte d_attr;    /* Default region attribute */
	char d_char;    /* Default region character */

	byte x_attr;    /* Desired region attribute */
	char x_char;    /* Desired region character */
};


/*
 * Information about regions affected by ongoing effects
 */
struct region_type
{
	s16b type;          /* Index into region_info_type */

	s16b who;          	/* Source of effect - 'who'. */
	s16b what;			/* Source of effect - 'what'. */

	s16b effect;		/* Effect of ongoing damage */
	s16b method;		/* Use this method */

	byte level;			/* Level used for method, if defined */
	byte facing;		/* Region facing */

	s16b damage;		/* Damage */
	s16b child_region;	/* Child region */

	s16b delay;     	/* Number of turns effect has left */
	s16b delay_reset;	/* Number of turns to reset counter to when countdown has finished */
	s16b age;			/* Number of turns effect has been alive */
	s16b lifespan;		/* Number of turns effect will be alive */

	byte y0;			/* Source y location */
	byte x0;			/* Source x location */
	byte y1;			/* Destination y location */
	byte x1;			/* Destination x location */

	u32b flags1;		/* Ongoing effect bitflags */

	s16b first_piece;	/* First region piece in sequence */
};



/*
 * Object information, for a generic object
 *
 * These correspond to 'tvals' although the tval
 * is indirect - it is held within the object group information.
 * This allows a sparse tval array.
 */
struct object_grouper
{
	char *text;
	int	tval;
};


/*
 * Object information, for a specific object.
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis,
 * and care must be taken when handling such objects.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * Each cave grid points to one (or zero) objects via the "o_idx"
 * field (above).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a "stack" of objects in the same grid.
 *
 * Each monster points to one (or zero) objects via the "hold_o_idx"
 * field (below).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a pile of objects held by the monster.
 *
 * The "held_m_idx" field is used to indicate which monster, if any,
 * is holding the object.  Objects being held have "ix=0" and "iy=0".
 */
struct object_type
{
	s16b k_idx;     /* Kind index (zero if "dead") */

	byte iy;/* Y-position on map, or zero */
	byte ix;/* X-position on map, or zero */

	byte tval;      /* Item type (from kind) */
	byte sval;      /* Item sub-type (from kind) */

	s16b pval;      /* Item extra-parameter */

	byte discount;  /* Discount (if any) */
	byte feeling;   /* Feeling (if any) */

	byte number;    /* Number of items */
	byte spare;	/* Spare */

	s16b weight;    /* Item weight */

	byte name1;     /* Artifact type, if any */
	byte name2;     /* Ego-Item type, if any */

	byte xtra1;     /* Extra info type */
	byte xtra2;     /* Extra info index */

	s16b to_h;      /* Plusses to hit */
	s16b to_d;      /* Plusses to damage */
	s16b to_a;      /* Plusses to AC */

	s16b ac;/* Normal AC */

	byte dd, ds;    /* Damage dice/sides */

	s16b charges;	/* Item charges */
	s16b timeout;   /* Timeout Counter */

	u16b ident;     /* Identify flags  */
	u16b note;      /* Inscription index */

	byte stackc;    /* Stack count */
	byte show_idx;	/* Index into show item list */

	s16b next_o_idx;/* Next object in stack (if any) */

	s16b held_m_idx;/* Monster holding us (if any) */

	s16b name3;   /* Race that dropped this item */

	byte origin;        /* How this item was found */
	byte origin_depth;  /* What depth the item was found at */
	u16b origin_xtra;   /* Extra information about origin */

	u32b can_flags1;
	u32b can_flags2;
	u32b can_flags3;
	u32b can_flags4;

	u32b may_flags1;
	u32b may_flags2;
	u32b may_flags3;
	u32b may_flags4;

	u32b not_flags1;
	u32b not_flags2;
	u32b not_flags3;
	u32b not_flags4;

	s16b usage;
	byte guess1;
	byte guess2;
};



/*
 * Monster information, for a specific monster.
 *
 * Note: fy, fx constrain dungeon size to 256x256
 *
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
struct monster_type
{
	s16b r_idx;     /* Monster race index */

	byte fy;	/* Y location on map */
	byte fx;	/* X location on map */

	s16b hp;/* Current Hit points */
	s16b maxhp;     /* Max Hit points */

	s16b csleep;    /* Inactive counter */

	byte mspeed;    /* Monster "speed" */
	byte energy;    /* Monster "energy" */

	byte stunned;   /* Monster is stunned */
	byte confused;  /* Monster is confused */
	byte monfear;   /* Monster is afraid */
	byte slowed;	/* Monster is slowed */

	byte hasted;	/* Monster is hasted */
	byte cut;	/* Monster is bleeding */
	byte poisoned;	/* Monster is poisoned */
	byte blind;	/* Monster is blind */

	byte tim_invis;	/* Monster is temporarily invisible */
	byte tim_passw;	/* Monster is temporarily passwall */
	byte bless;	/* Monster is temporarily blessed */
	byte berserk;	/* Monster is temporarily berserk */

	byte image;	/* Monster is hallucinating */
	byte dazed;	/* Monster is dazed */
	byte amnesia;	/* Monster is forgetful */
	byte terror;	/* Monster is terrified */

	byte shield;	/* Monster is temporarily shielded */
	byte oppose_elem; /* Monster is temporarily resistant to elements */
	byte petrify;	/* TODO: Monster is petrified / off-balance */
	byte facing; /* TODO: Monster facing for huge monsters and 'stealth' mode */

	byte ty;	/* Current target */
	byte tx;

	u32b mflag;     /* Extra monster flags */
	u32b smart;     /* Field for "smart_learn" */

	byte summoned;  /* Monster is/has summoned */
	byte min_range;
	byte best_range;
	byte mana; 	/* Current Mana */

	s16b hold_o_idx;/* Object being held (if any) */

	/* Computed values from here */

	byte cdis;      /* Current dis from player */

	bool ml;	/* Monster is "visible" */

};




/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */
struct alloc_entry
{
	s16b index;     /* The actual index */

	byte level;     /* Base dungeon level */
	byte prob1;     /* Probability, pass 1 */
	byte prob2;     /* Probability, pass 2 */
	byte prob3;     /* Probability, pass 3 */

	u16b total;     /* Unused for now */
};





/*
 * Structure for quest events
 */
struct quest_event
{
	u32b flags;		/* Flags */

	s16b quest;		/* Quest */

	byte dungeon;		/* Dungeon */
	byte level;		/* Level */
	byte room;		/* Room number */
	byte action;		/* Feature action */

	s16b feat;		/* Feature */
	s16b store;		/* Shop */

	s16b race;		/* Monster race */
	s16b kind;		/* Object kind */

	s16b number;		/* Number still needed */
	byte artifact;		/* Artifact */
	byte ego_item_type;	/* Ego item */

	s16b room_type_a;	/* Room adjective */
	s16b room_type_b;	/* Room noun */

	u32b room_flags;	/* Room flags */

	s32b time;		/* Time */

	s16b owner;		/* Shop owner */
	s16b power;		/* Power */

	s16b experience;	/* Experience */
	s16b gold;		/* Gold */
};


/*
 * Structure for the "quests"
 */
struct quest_type
{
	u32b name;		/* Quest name */
	u32b text;		/* Quest text */

	quest_event event[MAX_QUEST_EVENTS];	/* Quest events */

	byte stage;
};


/*
 * A store owner
 */
struct owner_type
{
	u32b owner_name;/* Name (offset) */
	bool busy; /* Runs a shop */

	s16b max_cost; /* Purse limit */

	byte max_inflate; /* Inflation (max) */
	byte min_inflate; /* Inflation (min) */

	byte haggle_per; /* Haggle unit */

	byte insult_max; /* Insult limit */

	byte owner_race; /* Owner race */
};




/*
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
struct store_type
{
	u32b name;    	/* Name (offset) */

	byte base;     	/* Store basic type */
	byte index;	/* Index into u_info */
	byte owner;    	/* Owner index */
	byte level;	/* Store generation level */

	s16b insult_cur; /* Insult counter */

	s16b good_buy;  /* Number of "good" buys */
	s16b bad_buy;   /* Number of "bad" buys */

	s32b store_open;/* Closed until this turn */
	s16b store_wary;/* Unused */

	byte tval[STORE_CHOICES];
	byte sval[STORE_CHOICES];
	s16b count[STORE_CHOICES];

	byte tvals_will_buy[STORE_WILL_BUY]; /* Tvals that the store will buy */

	byte stock_num; /* Stock -- Number of entries */
	s16b stock_size;/* Stock -- Total Size of Array */
	object_type *stock;     /* Stock -- Actual stock items */
        
        /* An artifact that always appears*/
        s16b auto_artifact;
};


/*
 * Quick table to display skills
 */
struct skill_table_entry
{
	int skill;
	const char *name;
	int div;
};


/*
 * Player sex info
 */
struct player_sex
{
	cptr title;     /* Type of sex */

	cptr winner;    /* Name of winner */
};


/*
 * Player racial info
 */
struct player_race
{
	u32b name;      /* Name (offset) */
	u32b text;      /* Text (offset) */

	s16b r_adj[A_MAX];      /* Racial stat bonuses */

	s16b r_skill[SKILL_MAX];	/* Racial skill bonus */

	s16b r_idx;	/* Monster index */

	byte r_exp;     /* Race experience factor */
	byte unused;	/* Unused */

	u16b b_age;     /* base age */
	u16b m_age;     /* mod age */

	u16b m_b_ht;    /* base height (males) */
	u16b m_m_ht;    /* mod height (males) */
	u16b m_b_wt;    /* base weight (males) */

	u16b f_b_ht;    /* base height (females) */
	u16b f_m_ht;    /* mod height (females)   */
	u16b f_b_wt;    /* base weight (females) */

	byte infra;     /* Infra-vision range */
	byte home;	/* Home town */

	s16b innate;	/* Hack - used to prepare body for innate abilities */

	u32b choice;    /* Legal class choices */

	s16b hist;      /* Starting history index */

	u32b cancel_flags1;    /* Race can never have these flags, set 1 */
	u32b cancel_flags2;    /* Race can never have these flags, set 2 */
	u32b cancel_flags3;    /* Race can never have these flags, set 3 */
	u32b cancel_flags4;    /* Race can never have these flags, set 4 */
};


/*
 * Starting equipment entry
 */
struct start_item
{
	byte tval;	/* Item's tval */
	byte sval;	/* Item's sval */
	byte number_min;/* Minimum starting amount */
	byte number_max;/* Maximum starting amount */
	s16b charge_min;/* Minimum charges */
	s16b charge_max;/* Maximum charges */
	byte social_min;/* Minimum social class to be given this */
	byte social_max;/* Maximum social class to be given this */
};


/*
 * Player class info
 */
struct player_class
{
	u32b name;
	u32b title[PY_MAX_LEVEL/5];

	s16b c_adj[A_MAX];      /* Class stat modifier */

	s16b c_skill_base[SKILL_MAX];	/* Starting skill level */
	s16b c_skill_improv[SKILL_MAX]; /* Increase in skill every 10 levels */
#if 0
	s16b c_dis;     /* class disarming */
	s16b c_dev;     /* class magic devices */
	s16b c_sav;     /* class saving throws */
	s16b c_stl;     /* class stealth */
	s16b c_srh;     /* class searching ability */
	s16b c_dig;     /* class digging ability */
	s16b c_tht;     /* class to hit (throwing) */
	s16b c_thn;     /* class to hit (normal) */
	s16b c_thb;     /* class to hit (bows) */

	s16b x_dis;     /* extra disarming */
	s16b x_dev;     /* extra magic devices */
	s16b x_sav;     /* extra saving throws */
	s16b x_stl;     /* extra stealth */
	s16b x_srh;     /* extra searching ability */
	s16b x_dig;     /* extra digging ability */
	s16b x_tht;     /* extra to hit (throwing) */
	s16b x_thn;     /* extra to hit (normal) */
	s16b x_thb;     /* extra to hit (bows) */
#endif
	s16b c_exp;     /* Class experience factor */

	u16b max_attacks;	/* Maximum possible attacks */
	u16b min_weight;	/* Minimum weapon weight for calculations */
	u16b att_multiply;	/* Multiplier for attack calculations */
	u16b chg_weight;	/* Divisor for charging damage calculations */

	byte spell_book;	/* Tval of spell books (if any) */
	byte spell_stat_study;	/* Stat for number of spells */
	byte spell_stat_mana;	/* Stat for determining mana */
	byte spell_stat_fail;	/* Stat for determine minimum and decrease in failure rate */

	u16b spell_first;	/* Level of first spell */
	u16b spell_weight;	/* Weight that hurts spells */

	bool spell_power;       /* Can cast 'powerful' spells */

	bool sense_squared;	/* Pseudo-id squared */
	byte sense_type;	/* Pseudo-id type */
	u32b sense_base;	/* Base pseudo-id value */
	u16b sense_div;		/* Pseudo-id divisor */

	start_item start_items[MAX_CLASS_ITEMS];/* The starting inventory */
};

/*
 * Weapon style info
 */
struct weapon_style
{
	s16b class;
	s16b level;

	u32b styles;
	s16b benefit;
};


/*
 * Spell appears structure
 *
 *      - Book
 *      - Slot
 */
struct spell_appears
{
	byte tval;
	byte sval;
	byte slot;
};

/*
 * Spell cast structure
 *
 *      - Book (RBM_*)
 *      - Slot (RBE_*)
 */
struct spell_cast
{
	byte class;
	byte level;
	s16b mana;
	byte fail;
	byte min;
};


/*
 * Spell blow structure
 *
 *      - Book (RBM_*)
 *      - Slot (RBE_*)
 */
struct spell_blow
{
	byte method;
	byte effect;
	byte d_dice;
	byte d_side;
	s16b d_plus;
	byte l_dice;
	byte l_side;
	s16b l_plus;
	byte levels;
};


/*
 * Spell type info
 */
struct spell_type
{
	u32b name;
	u32b text;

	u32b flags1;
	u32b flags2;
	u32b flags3;

	s16b type;
	s16b param;

	byte lasts_dice;
	byte lasts_side;
	s16b lasts_plus;

	spell_appears appears[MAX_SPELL_APPEARS];
	spell_cast cast[MAX_SPELL_CASTERS];
	spell_blow blow[4];

	s16b preq[MAX_SPELL_PREREQUISITES];
};

/*
 * Rune type info
 */
struct rune_type
{
	u32b name;
	u32b text;

	byte count[MAX_RUNE_FLAGS];
	byte flag[MAX_RUNE_FLAGS];
};


/*
 * Player background information
 */
struct hist_type
{
	u32b unused;    /* Unused */
	u32b text;  /* Text (offset) */

	byte roll;  /* Frequency of this entry */
	byte chart; /* Chart index */
	byte next;  /* Next chart index */
	byte bonus; /* Social Class Bonus + 50 */
};



/*
 * Some more player information
 *
 * This information is retained across player lives
 */
struct player_other
{
	char full_name[32];     /* Full name */
	char base_name[32];     /* Base name */

	bool opt[OPT_MAX];      /* Options */

	u32b window_flag[ANGBAND_TERM_MAX];    /* Window flags */

	byte hitpoint_warn;     /* Hitpoint warning (0 to 9) */
	byte delay_factor;      /* Delay factor (0 to 9) */
	byte monlist_sort_by;	/* Monster list sorted by (0 to 3) */
	byte monlist_display;	/* Monster list displays what (1 to 3) */
};


/*
 * Information about the player used for quick starts
 */
struct quickstart_type
{
	byte psex;      /* Sex index */
	byte prace;     /* Race index */
	byte pclass;    /* Class index */
	byte pstyle;    /* Style specialization */

	byte psval;		/* Style sub-specialization*/
	byte pschool;	/* Current magic 'school' */

	s16b stat_birth[A_MAX];	/* Birth "maximal" stat values */

	s32b birth_au;	/* Birth gold */
};


/*
 * Different familiar types
 */
struct familiar_type
{
	const char *name;
	u16b	method;
	u16b	attr1, attr2, attr3;
};


/*
 * Abilities familiars can gain
 */
struct familiar_ability_type
{
	const char *text;
	u16b attr;
	u16b preq;
};


/*
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This entire structure is wiped when a new character is born.
 *
 * This structure is more or less laid out so that the information
 * which must be saved in the savefile precedes all the information
 * which can be recomputed as needed.
 */
struct player_type
{
	s16b py;/* Player location */
	s16b px;/* Player location */

	byte psex;      /* Sex index */
	byte prace;     /* Race index */
	byte pclass;    /* Class index */
	byte pstyle;      /* Style specialization */

	byte hitdie;    /* Hit dice (sides) */
	byte expfact;   /* Experience factor */

	byte psval;	/* Style sub-specialization*/
	byte pshape;	/* Current shape */
	byte pschool;	/* Current magic 'school' */
	byte sauron_forms;	/* Forms of sauron that player has killed on this level */

	u16b dungeon;		/* Current dungeon number */
	u16b town;      /* Current town number */

	s16b age;       /* Characters age */
	s16b ht;/* Height */
	s16b wt;/* Weight */
	s16b sc;/* Social Class */

	s32b au;		/* Current Gold */
	s32b birth_au;	/* Gold at birth */

	s16b max_depth; /* Max depth */
	s16b depth;     /* Cur depth */

	s16b max_lev;   /* Max level */
	s16b lev;       /* Cur level */

	s32b max_exp;   /* Max experience */
	s32b exp;       /* Cur experience */
	u16b exp_frac;  /* Cur exp frac (times 2^16) */

	s16b mhp;       /* Max hit pts */
	s16b chp;       /* Cur hit pts */
	u16b chp_frac;  /* Cur hit frac (times 2^16); FIXME: probably unused */

	s16b msp;       /* Max mana pts */
	s16b csp;       /* Cur mana pts */
	u16b csp_frac;  /* Cur mana frac (times 2^16) */

	s16b stat_max[A_MAX];   /* Current "maximal" stat values */
	s16b stat_cur[A_MAX];   /* Current "natural" stat values */
	s16b stat_birth[A_MAX];	/* Birth "maximal" stat values */

	s16b timed[TMD_MAX];
#if 0
	s16b stat_inc_tim[A_MAX];      /* Timed -- Stat increase */
	s16b stat_dec_tim[A_MAX];      /* Timed -- Stat decrease */

	s16b fast;      /* Timed -- Fast */
	s16b slow;      /* Timed -- Slow */
	s16b blind;     /* Timed -- Blindness */
	s16b paralyzed; /* Timed -- Paralysis */
	s16b confused;  /* Timed -- Confusion */
	s16b afraid;    /* Timed -- Fear */
	s16b image;     /* Timed -- Hallucination */
	s16b poisoned;  /* Timed -- Poisoned */
	s16b cut;       /* Timed -- Cut */
	s16b stun;      /* Timed -- Stun */
	s16b cursed;    /* Timed -- Curse */
	s16b amnesia;	/* Timed -- Amnesia */
	s16b petrify;	/* Timed -- Petrification */
	s16b stastis;	/* Timed -- Stastis */

	s16b msleep;	/* Timed -- monster induced sleep */
	s16b psleep;	/* Timed -- player induced sleep */

	s16b protevil;  /* Timed -- Protection */
	s16b invis;	    /* Timed -- Invisbility */
	s16b free_act;  /* Timed -- Free action */
	s16b hero;      /* Timed -- Heroism */
	s16b shero;     /* Timed -- Super Heroism */
	s16b shield;    /* Timed -- Shield Spell */
	s16b blessed;   /* Timed -- Blessed */
	s16b tim_invis; /* Timed -- See Invisible */
	s16b tim_infra; /* Timed -- Infra Vision */

	s16b oppose_acid;       /* Timed -- oppose acid */
	s16b oppose_elec;       /* Timed -- oppose lightning */
	s16b oppose_fire;       /* Timed -- oppose heat */
	s16b oppose_cold;       /* Timed -- oppose cold */
	s16b oppose_pois;       /* Timed -- oppose poison */
	s16b slow_poison;       /* Timed -- slow poison */
	s16b slow_digest;       /* Timed -- slow digestion */

	s16b oppose_water;       /* Timed -- oppose water */
	s16b oppose_lava;       /* Timed -- oppose lava */
#endif
	s16b word_recall;       /* Word of recall counter */
	s16b word_return;		/* Word of return counter */
	s16b return_y;			/* Player return location */
	s16b return_x;			/* Player return location */

	s16b energy;    /* Current energy */

	s16b food;      /* Current nutrition */

	s16b rest;      /* Current rest */
	s16b water;     /* TODO: Current water */

	s16b held_song;     /* Currently held song */
	s16b spell_trap;	/* Spells the player cast get turned into traps */
	s16b delay_spell;	/* Delay the next spell cast for this many turns */

	byte dodging;   	/* Currently dodging */
	byte blocking;   	/* Currently blocking */
	byte sneaking; 		/* Currently sneaking */
	byte not_sneaking;	/* Lose the benefits of sneaking for next turn */

	byte branded_blows;	/* Current blow is branded with */
	byte familiar;		/* Familiar type */

	u16b familiar_attr[MAX_FAMILIAR_GAINS];	/* Abilities for the player's familiar */

	byte climbing; /* Currently climbing */
	byte searching; /* Currently searching */
	byte charging;	/* Currently charging */
	byte reserves;	/* Currently on reserve mana */

	u32b disease;	/* Disease types */

	s16b study_slot[MAX_STUDY_BOOK];	/* The inventory slots used to study books */

	u32b spell_learned1;    /* Spell flags */
	u32b spell_learned2;    /* Spell flags */
	u32b spell_learned3;    /* Spell flags */
	u32b spell_learned4;    /* Spell flags */
	u32b spell_worked1;     /* Spell flags */
	u32b spell_worked2;     /* Spell flags */
	u32b spell_worked3;     /* Spell flags */
	u32b spell_worked4;     /* Spell flags */
	u32b spell_forgotten1;  /* Spell flags */
	u32b spell_forgotten2;  /* Spell flags */
	u32b spell_forgotten3;  /* Spell flags */
	u32b spell_forgotten4;  /* Spell flags */

	s16b spell_order[PY_MAX_SPELLS];   /* Spell order */

	s16b player_hp[PY_MAX_LEVEL];   /* HP Array */

	char died_from[80];     /* Cause of death */
	char history[250];    /* Initial history */

	u16b total_winner;      /* Total winner */
	u16b panic_save;/* Panic save */

	u16b noscore;   /* Cheating flags */

	bool is_dead;   /* Player is dead */

	bool wizard;    /* Player is in wizard mode */



	/*** Temporary fields ***/

	bool playing;   /* True if player is playing */

	bool leaving;   /* True if player is leaving */

	s16b create_stair;   /* Create what type of stair on next level */

	s16b wy;/* Dungeon panel */
	s16b wx;/* Dungeon panel */

	s16b total_weight;      /* Total weight being carried */

	s16b inven_cnt; /* Number of items in inventory */
	s16b equip_cnt; /* Number of items in equipment (except quiver) */
	s16b pack_size_reduce_quiver;		/* Number of inventory slots used by
					   the quiver */
	s16b pack_size_reduce_bags;		/* Number of inventory slots used by
					   the bags */
	s16b pack_size_reduce_study;		/* Number of inventory slots used by
					   study slots */


	s16b target_set;/* Target flag */
	s16b target_who;/* Target identity */
	s16b target_row;/* Target location */
	s16b target_col;/* Target location */

	s16b target_race;/* Target monsters of this race only */

	s16b health_who;/* Health bar trackee */

	s16b monster_race_idx;  /* Monster race trackee */

	s16b object_kind_idx;   /* Object kind trackee */

	s16b energy_use;/* Energy use this turn */

	s16b resting;   /* Resting counter */
	s16b running;   /* Running counter */

	s16b run_cur_dir;       /* Direction we are running */
	s16b run_old_dir;       /* Direction we came from */
	s16b run_cur_feat;		/* Feature we started running on */
	s16b run_ignore_feat[9];/* Features we would normally notice which we choose to ignore */
	bool running_withpathfind;      /* Are we using the pathfinder ? */
	bool run_open_area;     /* Looking for an open area */
	bool run_break_right;   /* Looking for a break (right) */
	bool run_break_left;    /* Looking for a break (left) */

	s16b command_cmd;       /* Gives identity of current command */
	s16b command_arg;       /* Gives argument of current command */
	s16b command_rep;       /* Gives repetition of current command */
	s16b command_dir;       /* Gives direction of current command */
	key_event command_cmd_ex; /* Gives additional information of current command */

	s16b command_see;       /* See "cmd1.c" */
	s16b command_wrk;       /* See "cmd1.c" */

	s16b command_trans;		/* See "cmd6.c" */
	s16b command_trans_item;	/* See "cmd6.c" */

	key_event command_new;       /* Hack -- command chaining XXX XXX */

	s16b new_spells;/* Number of spells available */

	s16b old_spells;

	s16b boost_spell_power;		/* Temporary power boost for some spells */
	s16b boost_spell_number;	/* Temporary number boost for some spells */

	bool old_cumber_armor;
	bool old_cumber_glove;
	bool old_heavy_wield;
	bool old_heavy_shoot;
	bool old_icky_wield;

	s16b old_lite;  /* Old radius of lite (if any) */
	s16b old_view;  /* Old radius of view (if any) */

	s16b old_food_aux;      /* Old value of food */

	bool cumber_armor;      /* Mana draining armor */
	bool cumber_glove;      /* Mana draining gloves */
	bool heavy_wield;       /* Heavy weapon */
	bool heavy_shoot;       /* Heavy shooter */
	bool icky_wield;/* Icky weapon */
	bool uncontrolled;		/* Uncontrolled activation */

	s16b cur_lite;  /* Radius of lite (if any) */

	u32b notice;    /* Special Updates (bit flags) */
	u32b update;    /* Pending Updates (bit flags) */
	u32b redraw;    /* Normal Redraws (bit flags) */
	u32b window;    /* Window Redraws (bit flags) */

	s16b stat_use[A_MAX];   /* Current modified stats */
	s16b stat_top[A_MAX];   /* Maximal modified stats */


	/*** Extracted fields ***/

	s16b stat_add[A_MAX];   /* Equipment stat bonuses */
	s16b stat_ind[A_MAX];   /* Indexes into stat tables */

	u32b cur_flags1;
	u32b cur_flags2;
	u32b cur_flags3;
	u32b cur_flags4;

	byte incr_resist[MAX_INCR_RESISTS];

	byte siz_penalty;

	s16b dis_to_h;  /* Known bonus to hit */
	s16b dis_to_d;  /* Known bonus to dam */
	s16b dis_to_a;  /* Known bonus to ac */

	s16b dis_ac;    /* Known base ac */

	s16b to_h;      /* Bonus to hit */
	s16b to_d;      /* Bonus to dam */
	s16b to_a;      /* Bonus to ac */

	s16b ac;/* Base ac */

	s16b see_infra; /* Infravision range */

	s16b skills[SKILL_MAX];
#if 0
	s16b skill_dis; /* Skill: Disarming */
	s16b skill_dev; /* Skill: Magic Devices */
	s16b skill_sav; /* Skill: Saving throw */
	s16b skill_stl; /* Skill: Stealth factor */
	s16b skill_dig; /* Skill: Digging */
	s16b skill_srh; /* Skill: Searching ability */
	s16b skill_thn; /* Skill: To hit (normal) */
	s16b skill_thb; /* Skill: To hit (shooting) */
	s16b skill_tht; /* Skill: To hit (throwing) */
#endif
	s16b regen_hp;	/* Hitpoint regeneration rate */
	s16b regen_mana;/* Mana regeneration rate */
	s16b glowing;	/* Light radius bonus from items */

	u32b noise;     /* Derived from stealth */

	s16b num_blow;   /* Number of blows */
	s16b num_charge; /* Number of shots */
	s16b num_fire;   /* Number of shots */
	s16b num_throw;  /* Number of throws */

	byte ammo_mult; /* Ammo multiplier */

	byte fire_command; /* Command to choose when firing */

	s16b pspeed;    /* Current speed */
	s16b tiring;    /* Current rate of tiring */

	u32b cur_style; /* Current weapon style(s)*/

	s32b last_disturb;	/* Last time disturbed */

	s16b vulnerability;     /* How vulnerable? */

	s32b player_turn;         /* Number of player turns (including resting) */
	s32b resting_turn;        /* Number of player turns spent resting */

	byte outside;	/* Player is outside? */
	bool cursed_quiver;	/* The quiver is cursed */

};


/*
 * Semi-Portable High Score List Entry (128 bytes)
 *
 * All fields listed below are null terminated ascii strings.
 *
 * In addition, the "number" fields are right justified, and
 * space padded, to the full available length (minus the "null").
 *
 * Note that "string comparisons" are thus valid on "pts".
 */

typedef struct high_score high_score;

struct high_score
{
	char what[8];   /* Version info (string) */

	char pts[10];   /* Total Score (number) */

	char gold[10];  /* Total Gold (number) */

	char turns[10]; /* Turns Taken (number) */

	char day[10];   /* Time stamp (string) */

	char who[16];   /* Player Name (string) */

	char uid[8];    /* Player UID (number) */

	char sex[2];    /* Player Sex (string) */
	char p_r[3];    /* Player Race (number) */
	char p_c[3];    /* Player Class (number) */
	char p_s[3];    /* Player Style (number) */
	char p_p[3];    /* Player Pvalstyle (number) */

	char cur_lev[4];/* Current Player Level (number) */
	char cur_dep[4];/* Current Player Depth (number) */
	char cur_dun[4];/* Current Dungeon (number) */
	char max_lev[4];/* Max Player Level (number) */
	char max_dep[4];/* Max Dungeon Level (number) */

	char how[32];   /* Method of death (string) */
};



/*
 * Simple structure to hold a map location
 */

typedef struct coord coord;

struct coord
{
	byte y;
	byte x;
};


/*
 * A circular queue of map locations.
 * Check out defines.h and util.c for usage
 */
typedef struct
{
	/* Maximum number of grids in the queue */
	size_t max_size;
	/* Grid data */
	coord *data;
	/* Head and tail of the queue */
	size_t head, tail;
} grid_queue_type;




/*
 * A structure to hold a tval and its description
 */
struct tval_desc
{
	int tval;
	cptr desc;
};




/*
 * A structure to hold flags to match element types
 */
struct element_type
{
	int effect;
	u32b flags2;
	int grp_idx;
	s16b k_idx;
};


/*
 * Info used to manage quiver groups
 */
struct quiver_group_type
{
	char cmd;		/* The command used to perform an action with the objects in the group */
	byte color;		/* The color of the pseudo-tag used for the group */
};


/*
 * Info about what monsters are placed in the dungeon.
 */
struct ecology_type
{
	s16b race[MAX_ECOLOGY_RACES];
	u32b race_ecologies[MAX_ECOLOGY_RACES];	/* Which ecologies the race appears in */
	s16b deepest_race[MAX_ECOLOGIES];
	u32b use_ecology;		/* Match this ecology when forced */
	byte num_ecologies;	/* Number of ecologies */
	byte num_races;
	bool ready;		/* Are we forced to use this ecology? */
	bool valid_hook;	/* Is at least one monster valid using current get_mon_hook */
	bool town;		/* Are we generating a town ecology? */
	bool get_mon[MAX_ECOLOGY_RACES];	/* Are we permitted to pick this race */
};


/*
 * Commands that the player can do to items.
 */
struct do_cmd_item_type
{
	bool (*player_command)(int item);	/* The function to use as the command */
	char cmd_char;						/* The 'internal' keypress the command corresponds to */

	const char *item_command;			/* The name of the command */
	const char *item_query;				/* The string to display when asking for which item */
	const char *item_not_found;			/* The string to display if no valid items are found */

	bool (*item_tester_hook)(const object_type*);	/* The item tester function */
	int item_tester_tval;				/* The item tester tval */

	u32b use_from;						/* Where item can be used from */
	u32b conditions;					/* Restrictions on what state the player must be in */
	u32b timed_conditions;				/* Restrictions on what timed effects the player may not have */

	byte next_command;					/* The next command to use */
	int (*next_command_eval)(int item);	/* The next command to use - evaluation function */
};


/*
 * A game color.
 */
struct color_type
{
	char index_char;            /* Character index:  'r' = red, etc. */

	char name[32];              /* Color name */

	byte color_translate[MAX_ATTR];       /* Index for various in-game translations */
};


/*
 * Player timed effects.
 */
struct timed_effect
{
  const char *on_begin;
  const char *on_end;
  const char *on_increase;
  const char *on_decrease;
  const char *on_condition;
  const char *self_knowledge;
  u32b flag_redraw, flag_update;
  int msg;
};


/*
 * Names and ids of sounds
 */
struct sound_name_type
{
	const char *name;
	int id;
};
