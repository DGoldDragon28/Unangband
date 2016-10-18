/* File: variable.c */

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
 * Angband variants FAQ.  rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"


/*
 * Hack -- Link a copyright message into the executable
 */
cptr copyright =
	"Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Keoneke\n"
	"\n"
	"This software may be copied and distributed for educational, research,\n"
	"and not for profit purposes provided that this copyright and statement\n"
	"are included in all such copies.  Other copyrights may also apply.\n"
	"\n"
	"UnAngband (c) 2001-6 Andrew Doull. Modifications to the Angband 2.9.6\n"
	"source code are released under the Gnu Public License. See www.fsf.org\n"
	"for current GPL license details. Addition permission granted to\n"
	"incorporate modifications in all Angband variants as defined in the\n"
	"Angband variants FAQ. See rec.games.roguelike.angband for FAQ.\n";


/*
 * Executable version
 */
byte version_major = VERSION_MAJOR;
byte version_minor = VERSION_MINOR;
byte version_patch = VERSION_PATCH;
byte version_extra = VERSION_EXTRA;

/*
 * Savefile version
 */
byte sf_major;		  /* Savefile's "version_major" */
byte sf_minor;		  /* Savefile's "version_minor" */
byte sf_patch;		  /* Savefile's "version_patch" */
byte sf_extra;		  /* Savefile's "version_extra" */

/*
 * Savefile information
 */
u32b sf_xtra;		   /* Operating system info */
u32b sf_when;		   /* Time when savefile created */
u16b sf_lives;		  /* Number of past "lives" with this file */
u16b sf_saves;		  /* Number of "saves" during this life */

/*
 * Run-time arguments
 */
bool arg_fiddle;			/* Command arg -- Request fiddle mode */
bool arg_wizard;			/* Command arg -- Request wizard mode */
bool arg_sound;			 /* Command arg -- Request special sounds */
bool arg_mouse;			 /* Command arg -- Request mouse */
bool arg_trackmouse;			 /* Command arg -- Request constant mouse tracking */
bool arg_graphics;		      /* Command arg -- Request graphics mode */
bool arg_graphics_nice;		      /* Command arg -- Request 'nice' graphics mode */
bool arg_force_original;	/* Command arg -- Request original keyset */
bool arg_force_roguelike;       /* Command arg -- Request roguelike keyset */

/*
 * Various things
 */

bool character_quickstart = FALSE;       /* The character has been loaded and a quickstart profile exists */

bool character_generated;       /* The character exists */
bool character_dungeon;	 /* The character has a dungeon */
bool character_loaded;	  /* The character was loaded from a savefile */
bool character_saved;	   /* The character was just saved to a savefile */

s16b character_icky;	    /* Depth of the game in special mode */
s16b character_xtra;	    /* Depth of the game in startup mode */

u32b seed_randart;	      /* Hack -- consistent random artifacts */

u32b seed_flavor;	       /* Hack -- consistent object colors */
u32b seed_town;		 /* Hack -- consistent town layout */

u32b seed_dungeon;		/* Hack -- consistent current dungeon */
u32b seed_last_dungeon;		/* Hack -- consistent current dungeon */

s16b num_repro;		 /* Current reproducer count */
s16b object_level;	      /* Current object creation level */
s16b monster_level;	     /* Current monster creation level */

s16b summoner = 0;				/* Hack -- See summon specific() */
bool summon_strict = FALSE;		/* Hack -- See summon specific() */
char summon_char_type = '\0';	   /* Hack -- See summon_specific() */
byte summon_attr_type = 0;	   /* Hack -- See summon_specific() */
byte summon_group_type = 0;  /* Hack -- See summon_specific() */
u32b summon_flag_type = 0L;	   /* Hack -- See summon_specific() */
s16b summon_race_type = 0;	   /* Hack -- See summon_specific() */
char summon_word_type[80];		/* Hack -- See summon_specific() */

s32b turn;			      /* Current game turn */
s32b old_turn;		      /* unused */

bool use_mouse;		 /* The "mouse" mode is enabled */
bool use_trackmouse;	 /* The "trackmouse" mode is enabled */
bool use_graphics;	      /* The "graphics" mode is enabled */
bool use_graphics_nice;	      /* The 'nice' "graphics" mode is enabled */
bool use_trptile = FALSE;
bool use_dbltile = FALSE;
bool use_bigtile = FALSE;

s16b signal_count;	      /* Hack -- Count interrupts */

bool msg_flag;		  /* Player has pending message */

bool inkey_base;		/* See the "inkey()" function */
bool inkey_xtra;		/* See the "inkey()" function */
bool inkey_scan;		/* See the "inkey()" function */
bool inkey_flag;		/* See the "inkey()" function */

s16b coin_type;		 /* Hack -- force coin type */

s16b food_type;		 /* Hack -- force food type */

s16b race_drop_idx = 0;	     /* Hack -- force race drop */
s16b tval_drop_idx = 0;	     /* Hack -- force race drop */

bool opening_chest = FALSE;	     /* Hack -- theme chest generation */

bool shimmer_monsters;  /* Hack -- optimize multi-hued monsters */
bool shimmer_objects;   /* Hack -- optimize multi-hued objects */

bool repair_mflag_show; /* Hack -- repair monster flags (show) */
bool repair_mflag_mark; /* Hack -- repair monster flags (mark) */

s16b o_max = 1;		 /* Number of allocated objects */
s16b o_cnt = 0;		 /* Number of live objects */

s16b m_max = 1;		 /* Number of allocated monsters */
s16b m_cnt = 0;		 /* Number of live monsters */

s16b q_max = 1;		 /* Number of allocated quests */
s16b q_cnt = 0;		 /* Number of live quests */

/*
 * Hack - Trackees for term windows
 */
object_type term_object;
bool term_obj_real;

/*
 * TRUE if process_command() is a repeated call.
 */
bool command_repeating = FALSE;


/*
 * Dungeon variables
 */

byte feeling;		   /* Most recent feeling */
s16b rating;		    /* Level's current rating */

u32b level_flag;		/* Level type */


bool good_item_flag;    /* True if "Artifact" on this level */

bool closing_flag;	      /* Dungeon is closing */


/*
 * The character generates both directed (extra) noise (by doing noisy
 * things) and ambiant noise (the combination of directed and innate
 * noise).  Directed noise can immediately wake up monsters in LOS.
 * Ambient noise determines how quickly monsters wake up and how often
 * they get new information on the current character position.
 *
 * Each player turn, more noise accumulates.  Every time monster
 * temporary conditions are processed, all non-innate noise is cleared.
 */
int add_wakeup_chance = 0;
u32b total_wakeup_chance = 0;


/*
 * Player info
 */
int player_uid;
int player_euid;
int player_egid;


/*
 * Buffer to hold the current savefile name
 */
char savefile[1024];


/*
 * Number of active macros.
 */
s16b macro__num;

/*
 * Array of macro patterns [MACRO_MAX]
 */
char **macro__pat;

/*
 * Array of macro actions [MACRO_MAX]
 */
char **macro__act;


/*
 * The number of quarks (first quark is NULL)
 */
s16b quark__num = 1;

/*
 * The array[QUARK_MAX] of pointers to the quarks
 */
cptr *quark__str;

/*
 * The array [TIPS_MAX] of tips. These are held as quarks.
 */
s16b tips[TIPS_MAX];

/*
 * Start and end of tips.
 */
s16b tips_start = 0, tips_end = 0;

/*
 * The next "free" index to use
 */
u16b message__next;

/*
 * The index of the oldest message (none yet)
 */
u16b message__last;

/*
 * The next "free" offset
 */
u16b message__head;

/*
 * The offset to the oldest used char (none yet)
 */
u16b message__tail;

/*
 * The array[MESSAGE_MAX] of offsets, by index
 */
u16b *message__ptr;

/*
 * The array[MESSAGE_BUF] of chars, by offset
 */
char *message__buf;

/*
 * The array[MESSAGE_MAX] of u16b for the types of messages
 */
u16b *message__type;

/*
 * Hack --- context sensitive help
 * XXX - probably should make them pointers to elsewhere
 */

/*
 * Help file name
 */
char context_help_file[16];

/*
 * Help file name
 */
char context_help_match[132];



/*
 * Table of colors associated to message-types
 */
byte message__color[MSG_MAX];


/*
 * The array[8] of window pointers
 */
term *angband_term[8];


/*
 * The array[8] of window names (modifiable?)
 */
char angband_term_name[8][16] =
{
	"Unangband",
	"Term-1",
	"Term-2",
	"Term-3",
	"Term-4",
	"Term-5",
	"Term-6",
	"Term-7"
};


/*
 * Global table of color definitions (mostly zeros)
 */
byte angband_color_table[256][4] =
{
	{0x00, 0x00, 0x00, 0x00},	/* TERM_DARK */
	{0x00, 0xFF, 0xFF, 0xFF},	/* TERM_WHITE */
	{0x00, 0x80, 0x80, 0x80},	/* TERM_SLATE */
	{0x00, 0xFF, 0x80, 0x00},	/* TERM_ORANGE */
	{0x00, 0xC0, 0x00, 0x00},	/* TERM_RED */
	{0x00, 0x00, 0x80, 0x40},	/* TERM_GREEN */
	{0x00, 0x00, 0x40, 0xFF},	/* TERM_BLUE */
	{0x00, 0x80, 0x40, 0x00},	/* TERM_UMBER */
	{0x00, 0x60, 0x60, 0x60},	/* TERM_L_DARK */
	{0x00, 0xC0, 0xC0, 0xC0},	/* TERM_L_WHITE */
	{0x00, 0xFF, 0x00, 0xFF},	/* TERM_L_PURPLE */
	{0x00, 0xFF, 0xFF, 0x00},	/* TERM_YELLOW */
	{0x00, 0xFF, 0x40, 0x40},	/* TERM_L_RED */
	{0x00, 0x00, 0xFF, 0x00},	/* TERM_L_GREEN */
	{0x00, 0x00, 0xFF, 0xFF},	/* TERM_L_BLUE */
	{0x00, 0xC0, 0x80, 0x40},	/* TERM_L_UMBER */

	{0x00, 144, 0x00,  144},	/* TERM_PURPLE */
	{0x00, 144, 32,    0xFF},	/* TERM_VIOLET */
	{0x00, 0x00, 160,  160},	/* TERM_TEAL */
	{0x00, 108,  108,  48},		/* TERM_MUD */
	{0x00, 0xFF, 0xFF, 144},	/* TERM_L_YELLOW */
	{0x00, 0xFF, 0x00, 160},	/* TERM_MAGENTA */
	{0x00, 32,   0xFF, 220},	/* TERM_L_TEAL */
	{0x00, 184,  168,  0xFF},	/* TERM_L_VIOLET */
	{0x00, 0xFF, 128,  128},	/* TERM_L_PINK */
	{0x00, 180,  180,  0x00},	/* TERM_MUSTARD */
	{0x00, 160,  192,  208},	/* TERM_BLUE_SLATE */
	{0x00, 0x00, 176,  255},	/* TERM_DEEP_L_BLUE */

	/* Rest to be filled in when the game loads */
};

/*
 * Global array of color names and translations.
 */
color_type color_table[256] =
{							/* full mono vga blind lighter darker highlight metallic*/
	{ 'd', "Dark",         {  0,  0,  0,  TERM_DARK,	TERM_L_DARK,	TERM_DARK,	TERM_L_DARK, 	TERM_L_DARK, TERM_DARK}},
	{ 'w', "White",        {  1,  1,  1,  TERM_WHITE,	TERM_YELLOW,	TERM_SLATE,	TERM_L_BLUE,	TERM_YELLOW, TERM_WHITE}},
	{ 's', "Slate",        {  2,  1,  2,  TERM_SLATE,	TERM_L_WHITE,	TERM_L_DARK,TERM_L_WHITE,	TERM_L_WHITE, TERM_SLATE}},
	{ 'o', "Orange",       {  3,  1,  3,  TERM_L_WHITE,	TERM_YELLOW,	TERM_SLATE,	TERM_YELLOW,	TERM_YELLOW, TERM_ORANGE}},
	{ 'r', "Red",          {  4,  1,  4,  TERM_SLATE,	TERM_L_RED,		TERM_SLATE,	TERM_L_RED,		TERM_L_RED, TERM_RED}},
	{ 'g', "Green",        {  5,  1,  5,  TERM_SLATE,	TERM_L_GREEN,	TERM_SLATE,	TERM_L_GREEN,	TERM_L_GREEN, TERM_GREEN}},
	{ 'b', "Blue",         {  6,  1,  6,  TERM_SLATE,	TERM_L_BLUE,	TERM_SLATE,	TERM_L_BLUE,	TERM_L_BLUE, TERM_BLUE}},
	{ 'u', "Umber",        {  7,  1,  7,  TERM_L_DARK,	TERM_L_UMBER,	TERM_L_DARK,TERM_L_UMBER,	TERM_L_UMBER, TERM_UMBER}},
	{ 'D', "Light Dark",   {  8,  1,  8,  TERM_L_DARK,	TERM_SLATE,		TERM_L_DARK,TERM_SLATE,		TERM_SLATE, TERM_L_DARK}},
	{ 'W', "Light Slate",  {  9,  1,  9,  TERM_L_WHITE,	TERM_WHITE,		TERM_SLATE,	TERM_WHITE,		TERM_WHITE, TERM_SLATE}},
	{ 'P', "Light Purple", {  10, 1, 10,  TERM_SLATE,	TERM_YELLOW,	TERM_SLATE,	TERM_YELLOW,	TERM_YELLOW, TERM_L_PURPLE}},
	{ 'y', "Yellow",       {  11, 1, 11,  TERM_L_WHITE,	TERM_L_YELLOW,	TERM_L_WHITE,	TERM_WHITE,	TERM_WHITE, TERM_YELLOW}},
	{ 'R', "Light Red",    {  12, 1, 12,  TERM_L_WHITE,	TERM_YELLOW,	TERM_RED,	TERM_YELLOW,	TERM_YELLOW, TERM_L_RED}},
	{ 'G', "Light Green",  {  13, 1, 13,  TERM_L_WHITE,	TERM_YELLOW,	TERM_GREEN,	TERM_YELLOW,	TERM_YELLOW, TERM_L_GREEN}},
	{ 'B', "Light Blue",   {  14, 1, 14,  TERM_L_WHITE,	TERM_YELLOW,	TERM_BLUE,	TERM_YELLOW,	TERM_YELLOW, TERM_L_BLUE}},
	{ 'U', "Light Umber",  {  15, 1, 15,  TERM_L_WHITE,	TERM_YELLOW,	TERM_UMBER,	TERM_YELLOW,	TERM_YELLOW, TERM_L_UMBER}},

	{  'p', "Purple",	   {  16,   1, 10,TERM_SLATE,	TERM_L_PURPLE,	TERM_SLATE,	TERM_L_PURPLE,	TERM_L_PURPLE, TERM_PURPLE}},
	{  'v', "Violet",	   {  17,   1, 10,TERM_SLATE,	TERM_L_PURPLE,	TERM_SLATE,	TERM_L_PURPLE,	TERM_L_PURPLE, TERM_VIOLET}},
	{  't', "Teal",		   {  18,   1, 6, TERM_SLATE,	TERM_L_TEAL,	TERM_SLATE,	TERM_L_TEAL,	TERM_L_TEAL, TERM_TEAL}},
	{  'm', "Mud",		   {  19,   1, 5, TERM_SLATE,	TERM_MUSTARD,	TERM_SLATE,	TERM_MUSTARD,	TERM_MUSTARD, TERM_MUD}},
	{  'Y', "Light Yellow",{  20,   1, 11,TERM_WHITE,	TERM_WHITE,		TERM_YELLOW,TERM_WHITE,		TERM_WHITE, TERM_L_YELLOW}},
	{  'i', "Magenta-Pink",{  21,   1, 12,TERM_SLATE,	TERM_L_PINK,	TERM_RED,	TERM_L_PINK,	TERM_L_PINK, TERM_MAGENTA}},
	{  'T', "Light Teal",  {  22,   1, 14,TERM_L_WHITE,	TERM_YELLOW,	TERM_TEAL,	TERM_YELLOW,	TERM_YELLOW, TERM_L_TEAL}},
	{  'V', "Light Violet",{  23,   1, 10,TERM_L_WHITE,	TERM_YELLOW,	TERM_VIOLET,TERM_YELLOW,	TERM_YELLOW, TERM_L_VIOLET}},
	{  'I', "Light Pink",  {  24,   1, 12,TERM_L_WHITE,	TERM_YELLOW,	TERM_MAGENTA,TERM_YELLOW,	TERM_YELLOW, TERM_L_PINK}},
	{  'M', "Mustard",	   {  25,   1, 11,TERM_SLATE,	TERM_YELLOW,	TERM_SLATE,	TERM_YELLOW,	TERM_YELLOW, TERM_MUSTARD}},
	{  'z', "Blue Slate",  {  26,   1, 9, TERM_SLATE,	TERM_DEEP_L_BLUE,TERM_SLATE,TERM_DEEP_L_BLUE,	TERM_DEEP_L_BLUE, TERM_BLUE_SLATE}},
	{  'Z', "Deep Light Blue",{ 27, 1, 14,TERM_L_WHITE,	TERM_L_BLUE,	TERM_BLUE_SLATE,TERM_L_BLUE,	TERM_L_BLUE, TERM_DEEP_L_BLUE}},

	/* Rest to be filled in when the game loads */
};



/*
 * Standard sound (and message) names
 */
const sound_name_type angband_sound_name[MSG_MAX] =
{
		{ "", MSG_GENERIC },
		{ "hit", MSG_HIT },
		{ "miss", MSG_MISS },
		{ "flee", MSG_FLEE },
		{ "drop", MSG_DROP },
		{ "kill", MSG_KILL },
		{ "level", MSG_LEVEL },
		{ "death", MSG_DEATH },
		{ "study", MSG_STUDY },
		{ "teleport", MSG_TELEPORT },
		{ "shoot", MSG_SHOOT },
		{ "quaff", MSG_QUAFF },
		{ "zap_rod", MSG_ZAP_ROD },
		{ "walk", MSG_WALK },
		{ "tpother", MSG_TPOTHER },
		{ "hitwall", MSG_HITWALL },
		{ "eat", MSG_EAT },
		{ "store1", MSG_STORE1 },
		{ "store2", MSG_STORE2 },
		{ "store3", MSG_STORE3 },
		{ "store4", MSG_STORE4 },
		{ "dig", MSG_DIG },
		{ "opendoor", MSG_OPENDOOR},
		{ "shutdoor", MSG_SHUTDOOR},
		{ "tplevel", MSG_TPLEVEL},
		{ "bell", MSG_BELL},
		{ "nothing_to_open", MSG_NOTHING_TO_OPEN},
		{ "lockpick_fail", MSG_LOCKPICK_FAIL},
		{ "stairs_down",  MSG_STAIRS_DOWN},
		{ "hitpoint_warn", MSG_HITPOINT_WARN},
		{ "act_artifact", MSG_ACT_ARTIFACT},
		{ "use_staff",  MSG_USE_STAFF},
		{ "destroy", MSG_DESTROY},
		{ "mon_hit", MSG_MON_HIT},
		{ "mon_touch", MSG_MON_TOUCH},
		{ "mon_punch", MSG_MON_PUNCH},
		{ "mon_kick", MSG_MON_KICK},
		{ "mon_claw", MSG_MON_CLAW},
		{ "mon_bite", MSG_MON_BITE},
		{ "mon_sting", MSG_MON_STING},
		{ "mon_butt", MSG_MON_BUTT},
		{ "mon_crush", MSG_MON_CRUSH},
		{ "mon_engulf", MSG_MON_ENGULF},
		{ "mon_crawl", MSG_MON_CRAWL},
		{ "mon_drool", MSG_MON_DROOL},
		{ "mon_spit", MSG_MON_SPIT},
		{ "mon_gaze", MSG_MON_GAZE},
		{ "mon_wail", MSG_MON_WAIL},
		{ "mon_spore", MSG_MON_SPORE},
		{ "mon_beg", MSG_MON_BEG},
		{ "mon_insult", MSG_MON_INSULT},
		{ "mon_moan", MSG_MON_MOAN},
		{ "recover", MSG_RECOVER},
		{ "blind", MSG_BLIND},
		{ "confused", MSG_CONFUSED},
		{ "poisoned", MSG_POISONED},
		{ "afraid", MSG_AFRAID},
		{ "paralyzed", MSG_PARALYZED},
		{ "drugged", MSG_DRUGGED},
		{ "speed", MSG_SPEED},
		{ "slow", MSG_SLOW},
		{ "shield", MSG_SHIELD},
		{ "blessed", MSG_BLESSED},
		{ "hero", MSG_HERO},
		{ "berserk", MSG_BERSERK},
		{ "prot_evil", MSG_PROT_EVIL},
		{ "invuln", MSG_INVULN},
		{ "see_invis", MSG_SEE_INVIS},
		{ "infrared", MSG_INFRARED},
		{ "res_acid", MSG_RES_ACID},
		{ "res_elec", MSG_RES_ELEC},
		{ "res_fire", MSG_RES_FIRE},
		{ "res_cold", MSG_RES_COLD}, 
		{ "res_pois", MSG_RES_POIS},
		{ "stun", MSG_STUN},
		{ "cut", MSG_CUT},
		{ "stairs_up", MSG_STAIRS_UP},
		{ "store_enter", MSG_STORE_ENTER},
		{ "store_leave", MSG_STORE_LEAVE},
		{ "store_home", MSG_STORE_HOME},
		{ "money1", MSG_MONEY1},
		{ "money2", MSG_MONEY2},
		{ "money3", MSG_MONEY3},
		{ "shoot_hit", MSG_SHOOT_HIT},
		{ "store5", MSG_STORE5},
		{ "lockpick", MSG_LOCKPICK},
		{ "disarm", MSG_DISARM},
		{ "identify_bad", MSG_IDENT_BAD},
		{ "identify_ego", MSG_IDENT_EGO},
		{ "identify_art", MSG_IDENT_ART},
		{ "breathe_elements", MSG_BR_ELEMENTS},
		{ "breathe_frost", MSG_BR_FROST},
		{ "breathe_elec", MSG_BR_ELEC},
		{ "breathe_acid", MSG_BR_ACID},
		{ "breathe_gas", MSG_BR_GAS},
		{ "breathe_fire", MSG_BR_FIRE },
		{"breathe_confusion", MSG_BR_CONF},
		{"breathe_disenchant",  MSG_BR_DISENCHANT},
		{"breathe_chaos", MSG_BR_CHAOS},
		{"breathe_shards", MSG_BR_SHARDS},
		{"breathe_sound", MSG_BR_SOUND},
		{"breathe_light", MSG_BR_LIGHT},
		{"breathe_dark", MSG_BR_DARK},
		{"breathe_nether", MSG_BR_NETHER},
		{"breathe_nexus", MSG_BR_NEXUS},
		{"breathe_time", MSG_BR_TIME},
		{"breathe_inertia", MSG_BR_INERTIA},
		{"breathe_gravity", MSG_BR_GRAVITY},
		{"breathe_plasma", MSG_BR_PLASMA},
		{"breathe_force", MSG_BR_FORCE},
		{"summon_monster", MSG_SUM_MONSTER},
		{"summon_maia", MSG_SUM_MAIA},
		{"summon_undead", MSG_SUM_UNDEAD},
		{"summon_animal", MSG_SUM_ANIMAL},
		{"summon_spider", MSG_SUM_SPIDER},
		{"summon_hound", MSG_SUM_HOUND},
		{"summon_hydra", MSG_SUM_HYDRA},
		{"summon_demon", MSG_SUM_DEMON},
		{"summon_dragon", MSG_SUM_DRAGON},
		{"summon_gr_undead", MSG_SUM_HI_UNDEAD},
		{"summon_gr_dragon", MSG_SUM_HI_DRAGON},
		{"summon_gr_demon", MSG_SUM_HI_DEMON},
		{"summon_ringwraith", MSG_SUM_WRAITH},
		{"summon_unique", MSG_SUM_UNIQUE},
		{"wield", MSG_WIELD},
		{"cursed", MSG_CURSED},
		{"pseudo_id", MSG_PSEUDOID},
		{"hungry", MSG_HUNGRY},
		{"notice", MSG_NOTICE},
		{"ambient_day", MSG_AMBIENT_DAY},
		{"ambient_nite", MSG_AMBIENT_NITE},
		{"ambient_dng1", MSG_AMBIENT_DNG1},
		{"ambient_dng2", MSG_AMBIENT_DNG2},
		{"ambient_dng3", MSG_AMBIENT_DNG3},
		{"ambient_dng4", MSG_AMBIENT_DNG4},
		{"ambient_dng5", MSG_AMBIENT_DNG5},
		{"mon_create_trap", MSG_CREATE_TRAP},
		{"mon_shriek", MSG_SHRIEK},
		{"mon_cast_fear", MSG_CAST_FEAR},
		{"hit_good", MSG_HIT_GOOD},
		{"hit_great", MSG_HIT_GREAT},
		{"hit_superb", MSG_HIT_SUPERB},
		{"hit_hi_great", MSG_HIT_HI_GREAT},
		{"hit_hi_superb", MSG_HIT_HI_SUPERB},
		{"cast_spell", MSG_SPELL},
		{"pray_prayer", MSG_PRAYER},
		{"kill_unique", MSG_KILL_UNIQUE},
		{"kill_king", MSG_KILL_KING},
		{"drain_stat",MSG_DRAIN_STAT},
		{"multiply", MSG_MULTIPLY},
		{"screendump", MSG_SCREENDUMP}
};


/*
 * Array[VIEW_MAX] used by "update_view()" for los calculation
 */
sint view_n = 0;
u16b *view_g;

/*
 * Array[VIEW_MAX] used by "update_view()" for lof calculation
 */
sint fire_n = 0;
u16b *fire_g;


/*
 * Arrays[TEMP_MAX] used for various things
 */
sint temp_n = 0;
u16b *temp_g;
byte *temp_y;
byte *temp_x;

/*
 * Arrays[DYNA_MAX] used for various things
 */
sint dyna_n = 0;
u16b *dyna_g;
byte dyna_cent_y;
byte dyna_cent_x;
bool dyna_full;

/*
 * Array[DUNGEON_HGT][256] of cave grid info flags (padded)
 *
 * This array is padded to a width of 256 to allow fast access to elements
 * in the array via "grid" values (see the GRID() macros).
 */
byte (*cave_info)[256];

/*
 * Array[DUNGEON_HGT][256] of player grid info flags (padded)
 *
 * This array is padded to a width of 256 to allow fast access to elements
 * in the array via "grid" values (see the GRID() macros).
 */
byte (*play_info)[256];

/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid feature codes
 */
s16b (*cave_feat)[DUNGEON_WID];



/*
 * Array of room information
 *
 */
room_info_type room_info[DUN_ROOMS];

/*
 * Array[MAX_ROOMS_ROW][MAX_ROOMS_COL] of room information
 *
 * This indexes into the above room information
 *
 */
byte dun_room[MAX_ROOMS_ROW][MAX_ROOMS_COL];





/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid object indexes
 *
 * Note that this array yields the index of the top object in the stack of
 * objects in a given grid, using the "next_o_idx" field in that object to
 * indicate the next object in the stack, and so on, using zero to indicate
 * "nothing".  This array replicates the information contained in the object
 * list, for efficiency, providing extremely fast determination of whether
 * any object is in a grid, and relatively fast determination of which objects
 * are in a grid.
 */
s16b (*cave_o_idx)[DUNGEON_WID];




/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid monster indexes
 *
 * Note that this array yields the index of the monster or player in a grid,
 * where negative numbers are used to represent the player, positive numbers
 * are used to represent a monster, and zero is used to indicate "nobody".
 * This array replicates the information contained in the monster list and
 * the player structure, but provides extremely fast determination of which,
 * if any, monster or player is in any given grid.
 */
s16b (*cave_m_idx)[DUNGEON_WID];



/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid region indexes
 *
 * Note that this array yields the index of the top region in the stack of
 * region in a given grid, using the "next_region_idx" field in that region to
 * indicate the next region in the stack, and so on, using zero to indicate
 * "nothing".  This array replicates the information contained in the region
 * list, for efficiency, providing extremely fast determination of whether
 * any region is in a grid, and relatively fast determination of which regions
 * are in a grid.
 */
s16b (*cave_region_piece)[DUNGEON_WID];


/*
 * Array[z_info->?_max] of region pieces
 */
region_piece_type *region_piece_list;


int region_piece_max = 1;
int region_piece_cnt = 0;


/*
 * Array[z_info->?_max] of regions
 */
region_type *region_list;


int region_max = 1;
int region_cnt = 0;




/*
 * The follow functions allow for 'alternate' modes of display the main map.
 */
void (*modify_grid_adjacent_hook)(byte *a, char *c, int y, int x, byte adj_char[16]);
void (*modify_grid_boring_hook)(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);
void (*modify_grid_unseen_hook)(byte *a, char *c);
void (*modify_grid_interesting_hook)(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);


#ifdef MONSTER_FLOW

/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid flow "cost" values
 * Used to simulate character noise.
 */
byte (*cave_cost)[DUNGEON_WID];

/*
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid flow "when" stamps.
 * Used to store character scent trails.
 */
byte (*cave_when)[DUNGEON_WID];

/*
 * Current scent age marker.  Counts down from 250 to 0 and then loops.
 */
int scent_when = 250;


/*
 * Centerpoints of the last flow (noise) rebuild and the last flow update.
 */
int flow_center_y;
int flow_center_x;
int update_center_y;
int update_center_x;

/*
 * Flow cost at the center grid of the current update.
 */
int cost_at_center = 0;

#endif	/* MONSTER_FLOW */



/*
 * Array[z_info->o_max] of dungeon objects
 */
object_type *o_list;

/*
 * Array[z_info->m_max] of dungeon monsters
 */
monster_type *m_list;

/*
 * Array[z_info->r_max] of monster lore
 */
monster_lore *l_list;


/*
 * Array[z_info->a_max] of artifact lore
 */
object_info *a_list;

/*
 * Array[z_info->e_max] of ego item lore
 */
object_lore *e_list;

/*
 * Array[z_info->k_max] of flavor lore
 */
object_info *x_list;


/*
 * Hack -- Array[MAX_Q_IDX] of random quests
 */
quest_type *q_list;



/*
 * The maximum number of "stores" (at most z_info->t_max * MAX_STORES)
 */
s16b max_store_count;

/*
 * The size of "store" (at most z_info->t_max * MAX_STORES)
 */
s16b total_store_count = 0;


/*
 * Array[total_store_count] of pointers to stores
 */
store_type_ptr *store;

/*
 * Array[INVEN_TOTAL+1] of objects in the player's inventory
 */
object_type *inventory;


/*
 * Array[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL] of numbers in magical bags
 */
s16b bag_contents[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL];


/*
 * Array[INVEN_TOTAL] of items listed at the bottom of the screen
 * This is used to track player items in a pointer-based user interface
 */
int itemlist[INVEN_TOTAL];


/*
 * The size of "alloc_kind_table" (at most z_info->k_max * 4)
 */
s16b alloc_kind_size;

/*
 * The array[alloc_kind_size] of entries in the "kind allocator table"
 */
alloc_entry *alloc_kind_table;


/*
 * The size of the "alloc_ego_table"
 */
s16b alloc_ego_size;

/*
 * The array[alloc_ego_size] of entries in the "ego allocator table"
 */
alloc_entry *alloc_ego_table;

/*
 * The size of "alloc_feat_table" (at most MAX_F_IDX * 4)
 */
s16b alloc_feat_size;

/*
 * The array[alloc_feat_size] of entries in the "feat allocator table"
 */
alloc_entry *alloc_feat_table;


/*
 * The size of "alloc_race_table" (at most z_info->r_max)
 */
s16b alloc_race_size;

/*
 * The array[alloc_race_size] of entries in the "race allocator table"
 */
alloc_entry *alloc_race_table;


/*
 * Specify color for inventory item text display (by tval)
 * Be sure to use "index & 0x7F" to avoid illegal access
 */
byte tval_to_attr[128];


/*
 * Current (or recent) macro action
 */
char macro_buffer[1024];


/*
 * Keymaps for each "mode" associated with each keypress.
 */
char *keymap_act[KEYMAP_MODES][256];



/*** Player information ***/

/*
 * Pointer to the player tables (sex, race, class, magic)
 */
const player_sex *sp_ptr;
const player_race *rp_ptr;
const player_class *cp_ptr;

/*
 * The player other record (static)
 */
static player_other player_other_body;

/*
 * Pointer to the player other record
 */
player_other *op_ptr = &player_other_body;

/*
 * The player info record (static)
 */
static player_type player_type_body;

/*
 * Pointer to the player info record
 */
player_type *p_ptr = &player_type_body;


/*
 * Structure (not array) of size limits
 */
maxima *z_info;

/*
 * The vault generation arrays
 */
vault_type *v_info;
char *v_name;
char *v_text;

/*
 * The blow arrays
 */
method_type *method_info;
char *method_name;
char *method_text;

/*
 * The effect arrays
 */
effect_type *effect_info;
char *effect_name;
char *effect_text;

/*
 * The region info arrays
 */
region_info_type *region_info;
char *region_name;
char *region_text;

/*
 * The terrain feature arrays
 */
feature_type *f_info;
char *f_name;
char *f_text;

/*
 * The room description information arrays
 */
desc_type *d_info;
char *d_name;
char *d_text;

/*
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/*
 * The artifact arrays
 */
artifact_type *a_info;
char *a_name;
char *a_text;

/*
 * The random name generator tables
 */
names_type *n_info;

/*
 * The ego-item arrays
 */
ego_item_type *e_info;
char *e_name;
char *e_text;

/*
 * The flavor arrays
 */
flavor_type *x_info;
char *x_name;
char *x_text;

/*
 * The monster race arrays
 */
monster_race *r_info;
char *r_name;
char *r_text;


/*
 * The player race arrays
 */
player_race *p_info;
char *p_name;
char *p_text;

/*
 * The player class arrays
 */
player_class *c_info;
char *c_name;
char *c_text;

/*
 * The weapon style arrays
 */
weapon_style *w_info;

/*
 * The spell arrays
 */
spell_type *s_info;
char *s_name;
char *s_text;

/*
 * The rune arrays
 */
rune_type *y_info;
char *y_name;
char *y_text;


/*
 * The town/dungeon arrays
 */
town_type *t_info;
char *t_name;
char *t_text;

/*
 * The store arrays
 */
store_type *u_info;
char *u_name;
char *u_text;


/*
 * The player history arrays
 */
hist_type *h_info;
char *h_text;

/*
 * The shop owner arrays
 */
owner_type *b_info;
char *b_name;
char *b_text;

/*
 * The racial price adjustment arrays
 */
byte *g_info;
char *g_name;
char *g_text;


/*
 * The fixed quest arrays
 */
quest_type *q_info;
char *q_name;
char *q_text;

/*
 * Hack -- The special Angband "System Suffix"
 * This variable is used to choose an appropriate "pref-xxx" file
 */
const char *ANGBAND_SYS = "xxx";

/*
 * Hack -- The special Angband "Graphics Suffix"
 * This variable is used to choose an appropriate "graf-xxx" file
 */
const char *ANGBAND_GRAF = "old";

/*
 * Path name: The main "lib" directory
 * This variable is not actually used anywhere in the code
 */
char *ANGBAND_DIR;

/*
 * High score files (binary)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_APEX;

/*
 * Bone files for player ghosts (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_BONE;

/*
 * Binary image files for the "*_info" arrays (binary)
 * These files are not portable between platforms
 */
char *ANGBAND_DIR_DATA;

/*
 * Textual template files for the "*_info" arrays (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_EDIT;

/*
 * Various extra files (ascii)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_FILE;

/*
 * Help files (normal) for the online help (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_HELP;

/*
 * Help files (spoilers) for the online help (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_INFO;

/*
 * Savefiles for current characters (binary)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_SAVE;

/*
 * User "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_PREF;


/*
 * User "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_USER;

/*
 * Various extra files (binary)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_XTRA;


/*
 * Total Hack -- allow all items to be listed (even empty ones)
 * This is only used by "do_cmd_inven_e()" and is cleared there.
 */
bool item_tester_full;


/*
 * Here is a "pseudo-hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
byte item_tester_tval;


/*
 * Here is a "hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
bool (*item_tester_hook)(const object_type*);


/*
 * Here is a "hook" used during calls to "teleport_player()" and
 * "teleport_away()
 */
bool (*teleport_hook)(const int oy, const int ox, const int ny, const int nx);


/*
 * Current "comp" function for ang_sort()
 */
bool (*ang_sort_comp)(vptr u, vptr v, int a, int b);


/*
 * Current "swap" function for ang_sort()
 */
void (*ang_sort_swap)(vptr u, vptr v, int a, int b);



/*
 * Hack -- function hook to restrict "get_mon_num_prep()" function
 */
bool (*get_mon_num_hook)(int r_idx);



/*
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool (*get_obj_num_hook)(int k_idx);


/*
 * Hack -- function hook to restrict "get_feat_num_prep()" function
 */
bool (*get_feat_num_hook)(int f_idx);


/*
 * Hack -- File hundle for output used within the text_out_to_file()
 */
FILE *text_out_file = NULL;


/*
 * Hack -- Where to wrap the text when using text_out().  Use the default
 * value (for example the screen width) when 'text_out_wrap' is 0.
 */
int text_out_wrap = 0;


/*
 * Hack -- Indentation for the text when using text_out().
 */
int text_out_indent = 0;

/*
 * Hack -- Where to stop text output in number of lines. The function will
 * write at most this many lines before aborting. Use the default
 * value (for example the screen height) when 'text_out_lines' is 0. Note
 * that the starting y position in screen output will assume that already
 * (y - 1) lines have been written (Similiar to text_out_indent).
 */
int text_out_lines = 0;

/*
 * Hack -- function hook for text_out()
 *
 * Returns 0 if all characters in str have been output, or the next position
 * in the string if not completely output.
 */
void (*text_out_hook)(byte a, cptr str);


/*
 * The "highscore" file descriptor, if available.
 */
int highscore_fd = -1;


/*
 * Use transparent tiles
 */
bool use_transparency = FALSE;

/*
 * Game can be saved
 */
bool can_save = TRUE;


/*
 * Slay power
 *
 * This is a precomputed table of slay powers for weapons with single slay or brand
 * flags.
 */
s32b magic_slay_power[32];

/*
 * Cache the results of slay_value(), which is expensive and would
 * otherwise be called much too often. We create this as a part of initialisation,
 * then free it after we have either loaded or randomly generated the artifacts.
 */
s32b *slays = NULL;

/*
 * Total monster power
 */
long tot_mon_power;


/* Path finding variables
 *
 */
char pf_result[MAX_PF_LENGTH];
int pf_result_index;


/*
 * Some static info used to manage quiver groups
 */
quiver_group_type quiver_group[MAX_QUIVER_GROUPS] =
{
	{'f', TERM_L_BLUE},
	{'f', TERM_L_GREEN},
	{'f', TERM_YELLOW},
	{'v', TERM_ORANGE},
};


/*
 * We cache the kinds of objects held in bags here
 *
 * Otherwise we get a significant performance penalty when checking bags for valid items due
 * to the performance of lookup_kind when faking the objects in bags.
 */
s16b bag_kinds_cache[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL];


/*
 * We now use a 'dungeon ecology' system to generate monsters on a level.
 */
ecology_type cave_ecology;


/*
 * We now try to force monsters to have 'one of each item slot'.
 */
u32b hack_monster_equip;


/*
 * Variables used to highlight project path to target.
 */
int target_path_n;
u16b target_path_g[512];
s16b target_path_d[512];


