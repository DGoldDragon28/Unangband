/* File: generate.c */

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
 * Note that Level generation is *not* an important bottleneck,
 * though it can be annoyingly slow on older machines...  Thus
 * we emphasize "simplicity" and "correctness" over "speed".
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * Consider the "v_info.txt" file for vault generation.
 *
 * In this file, we use the "special" granite and perma-wall sub-types,
 * where "basic" is normal, "inner" is inside a room, "outer" is the
 * outer wall of a room, and "solid" is the outer wall of the dungeon
 * or any walls that may not be pierced by corridors.  Thus the only
 * wall type that may be pierced by a corridor is the "outer granite"
 * type.  The "basic granite" type yields the "actual" corridors.
 *
 * Note that we use the special "solid" granite wall type to prevent
 * multiple corridors from piercing a wall in two adjacent locations,
 * which would be messy, and we use the special "outer" granite wall
 * to indicate which walls "surround" rooms, and may thus be "pierced"
 * by corridors entering or leaving the room.
 *
 * Note that a tunnel which attempts to leave a room near the "edge"
 * of the dungeon in a direction toward that edge will cause "silly"
 * wall piercings, but will have no permanently incorrect effects,
 * as long as the tunnel can *eventually* exit from another side.
 * And note that the wall may not come back into the room by the
 * hole it left through, so it must bend to the left or right and
 * then optionally re-enter the room (at least 2 grids away).  This
 * is not a problem since every room that is large enough to block
 * the passage of tunnels is also large enough to allow the tunnel
 * to pierce the room itself several times.
 *
 * Note that no two corridors may enter a room through adjacent grids,
 * they must either share an entryway or else use entryways at least
 * two grids apart.  This prevents "large" (or "silly") doorways.
 *
 * To create rooms in the dungeon, we first divide the dungeon up
 * into "blocks" of 11x11 grids each, and require that all rooms
 * occupy a rectangular group of blocks.  As long as each room type
 * reserves a sufficient number of blocks, the room building routines
 * will not need to check bounds.  Note that most of the normal rooms
 * actually only use 23x11 grids, and so reserve 33x11 grids.
 *
 * Note that the use of 11x11 blocks (instead of the 33x11 panels)
 * allows more variability in the horizontal placement of rooms, and
 * at the same time has the disadvantage that some rooms (two thirds
 * of the normal rooms) may be "split" by panel boundaries.  This can
 * induce a situation where a player is in a room and part of the room
 * is off the screen.  This can be so annoying that the player must set
 * a special option to enable "non-aligned" room generation.
 *
 * Note that the dungeon generation routines are much different (2.7.5)
 * and perhaps "MAX_DUN_ROOMS" should be less than 50.
 *
 * XXX XXX XXX Note that it is possible to create a room which is only
 * connected to itself, because the "tunnel generation" code allows a
 * tunnel to leave a room, wander around, and then re-enter the room.
 *
 * XXX XXX XXX Note that it is possible to create a set of rooms which
 * are only connected to other rooms in that set, since there is nothing
 * explicit in the code to prevent this from happening.  But this is less
 * likely than the "isolated room" problem, because each room attempts to
 * connect to another room, in a giant cycle, thus requiring at least two
 * bizarre occurances to create an isolated section of the dungeon.
 *
 * Note that (2.7.9) monster pits have been split into monster "nests"
 * and monster "pits".  The "nests" have a collection of monsters of a
 * given type strewn randomly around the room (jelly, animal, or undead),
 * while the "pits" have a collection of monsters of a given type placed
 * around the room in an organized manner (orc, troll, giant, dragon, or
 * demon).  Note that both "nests" and "pits" are now "level dependant",
 * and both make 16 "expensive" calls to the "get_mon_num()" function.
 *
 * Note that the cave grid flags changed in a rather drastic manner
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain
 * features, such as doors and stairs and traps and rubble and walls,
 * are all handled as a set of 64 possible "terrain features", and
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.
 *
 * XXX Note there have been significant changes to how dungeon generation
 * works and many assumptions about the code need to be checked and
 * documented.
 */


/*
 * Dungeon generation values
 */
/* DUN_ROOMS now defined in defines.h */
#define DUN_NATURE_SURFACE	80	/* Chance in 100 of having lakes on surface */
#define DUN_NATURE_DEEP		40	/* Chance in 100 of having lakes at depth */
#define DUN_MAX_LAKES		 8	/* Maximum number of lakes/rivers */
#define DUN_MAX_LAKES_NONE   8	/* Maximum number of lakes/rivers in roomless areas */
#define DUN_MAX_LAKES_ROOM   4	/* Maximum number of lakes/rivers in normal areas */
#define DUN_FEAT_RNG    2       /* Width of lake */

/*
 * Special dungeon values */
#define SPECIAL_MOUNTAIN	1		/* Mountains (from FAAngband) */
#define SPECIAL_GREAT_HALL	2		/* An open area with large square pillars & rooms */
#define SPECIAL_GREAT_CAVE	3		/* An open area with large natural pillars & rooms */
#define SPECIAL_CHAMBERS	4		/* Chambers over whole level */
#define SPECIAL_STARBURST	5		/* Starburst over whole level */
#define SPECIAL_LABYRINTH	6		/* Maze over whole level */
#define SPECIAL_GREAT_PILLARS	7	/* An open area with large square pillars */
#define SPECIAL_CITADEL		8		/* An area consisting of city over the whole level */


#define MAX_SPECIAL_DUNGEONS	9

/*
 * Dungeon tunnel generation values
 */
#define DUN_TUN_RND     30      /* 1 in # chance of random direction */
#define DUN_TUN_ADJ     10      /* 1 in # chance of adjusting direction */
#define DUN_TUN_CAV     3      	/* 1 in # chance of random direction in caves */
#define DUN_TUN_LEN     10      /* 1 in # chance of cave tunnel becoming less random as it grows */
#define DUN_TUN_STYLE   10      /* 1 in # chance of changing style */
#define DUN_TUN_CRYPT   6       /* 1 in # chance of crypt niche having stairs or a monster */
#define DUN_TUN_CON     15      /* Chance of extra tunneling */
#define DUN_TUN_PEN     25      /* Chance of doors at room entrances */
#define DUN_TUN_JCT     90      /* Chance of doors at tunnel junctions */

#define DUN_TRIES		3		/* Number of tries to connect two rooms, before increasing 'scope' for
									connection attempts */

/*
 * Number of rooms to try to generate in a dungeon. This reduces with increasing depth -(depth/12).
 */
#define MIN_DUN_ROOMS 	9
#define MAX_DUN_ROOMS	DUN_ROOMS

/*
 * Dungeon streamer generation vaÄlues
 */
#define DUN_STR_WID          2  /* Width of streamers (can be higher) */
#define DUN_STR_CHG          16 /* 1/(4 + chance) of altering direction */
#define DUN_MAX_STREAMER     5  /* Number of streamers */


/*
 * Dungeon treasure allocation values
 */
#define DUN_AMT_ROOM    9       /* Amount of objects for rooms, doubled for strongholds */
#define DUN_AMT_ITEM    3       /* Amount of objects for rooms/corridors, quadrupled for crypts */
#define DUN_AMT_GOLD    3       /* Amount of treasure for rooms/corridors, quadrupled for mines */

/*
 * Hack -- Dungeon allocation "places"
 */
#define ALLOC_SET_CORR	  1       /* Hallway */
#define ALLOC_SET_ROOM	  2       /* Room */
#define ALLOC_SET_BOTH	  3       /* Anywhere */

/*
 * Hack -- Dungeon allocation "types"
 */
#define ALLOC_TYP_RUBBLE	1       /* Rubble */
#define ALLOC_TYP_TRAP		3       /* Trap */
#define ALLOC_TYP_GOLD		4       /* Gold */
#define ALLOC_TYP_OBJECT	5       /* Object */
#define ALLOC_TYP_FEATURE	6		/* Feature eg fountain */
#define ALLOC_TYP_BODY		7		/* Body parts. Placed if most powerful monster is out of depth. */


/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	DUN_ROOMS
#define DOOR_MAX	100
#define NEXT_MAX	200
#define WALL_MAX	80
#define TUNN_MAX	600
#define SOLID_MAX	240
#define QUEST_MAX	30
#define STAIR_MAX	30
#define DECOR_MAX	30


/*
 * These flags control the construction of starburst rooms and lakes
 */
#define STAR_BURST_ROOM		0x00000001	/* Mark grids with CAVE_ROOM */
#define STAR_BURST_LIGHT	0x00000002	/* Mark grids with CAVE_GLOW */
#define STAR_BURST_CLOVER	0x00000004	/* Allow cloverleaf rooms */
#define STAR_BURST_RAW_FLOOR	0x00000008	/* Floor overwrites dungeon */
#define STAR_BURST_RAW_EDGE	0x00000010	/* Edge overwrites dungeon */
#define STAR_BURST_MOAT		0x00000020	/* Skip areas defined as a room */
#define STAR_BURST_RANDOM	0x00000040	/* Write terrain 60% of the time, else floor */
#define STAR_BURST_NEED_FLOOR	0x00000080	/* Write terrain only over floors */




/*
 * Minimum number of rooms
 */
#define ROOM_MIN	2


/*
 * Room type information
 */

typedef struct room_data_type room_data_type;

struct room_data_type
{
	/* Allocation information. */
	s16b chance[11];

	/* Minimum level on which room can appear. */
	byte min_level;
	byte max_number;
	byte count_as;
	byte unused;

	/* Level that this appears on */
	u32b theme;
};


/*
 * Structure to hold all "dungeon generation" data
 */

typedef struct dun_data dun_data;

struct dun_data
{
	/* Number of lakes */
	int lake_n;
	coord lake[DUN_MAX_LAKES];

	/* Streamers */
	int stream_n;
	s16b streamer[DUN_MAX_STREAMER];

	/* Array of centers of rooms */
	int cent_n;
	coord cent[CENT_MAX];

	/* Array of partitions of rooms */
	int part_n;
	int part[CENT_MAX];

	/* Array of possible door locations */
	int door_n;
	coord door[DOOR_MAX];

	/* Array of solid walls */
	int solid_n;
	coord solid[SOLID_MAX];
	s16b solid_feat[SOLID_MAX];

	/* Array of decorations next to room entrances and feature types */
	int next_n;
	coord next[NEXT_MAX];
	s16b next_feat[NEXT_MAX];

	/* Array of wall piercing locations */
	int wall_n;
	coord wall[WALL_MAX];

	/* Array of tunnel grids and feature types */
	int tunn_n;
	coord tunn[TUNN_MAX];
	s16b tunn_feat[TUNN_MAX];

	/* Array of tunnel decorations */
	int decor_n;
	coord decor[DECOR_MAX];
	int decor_t[DECOR_MAX];

	/* Array of good potential quest grids */
	s16b quest_n;
	coord quest[QUEST_MAX];

	/* Array of good potential stair grids */
	s16b stair_n;
	coord stair[STAIR_MAX];

	/* Number of blocks along each axis */
	int row_rooms;
	int col_rooms;

	/* Hack -- number of entrances to dungeon */
	byte entrance;

	/* Hack -- special dungeons */
	byte special;

	/* Hack -- theme rooms */
	s16b theme_feat;

	/* Hack -- flooded rooms */
	s16b flood_feat;

	/* Array of which blocks are used */
	bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

	/* Good location for guardian */
	byte guard_x0;
	byte guard_y0;
	byte guard_x1;
	byte guard_y1;
};


/*
 * Dungeon generation data -- see "cave_gen()"
 */
static dun_data *dun;


/*
 * Table of values that control how many times each type of room will,
 * on average, appear on 100 levels at various depths.  Each type of room
 * has its own row, and each column corresponds to dungeon levels 0, 10,
 * 20, and so on.  The final value is the minimum depth the room can appear
 * at.  -LM-
 *
 * Level 101 and below use the values for level 100.
 *
 * Rooms with lots of monsters or loot may not be generated if the object or
 * monster lists are already nearly full.  Rooms will not appear above their
 * minimum depth.  No type of room (other than type 1) can appear more than
 * DUN_ROOMS/2 times in any level.  Tiny levels will not have space for all
 * the rooms you ask for.
 *
 * The entries for room type 1 are blank because these rooms are built once
 * all other rooms are finished -- until the level fills up, or the room
 * count reaches the limit (DUN_ROOMS).
 */
static room_data_type room_data[ROOM_MAX] =
{
   /* Depth:         0   6   12   18   24   30   36   42   48   54  60  min max_num count, theme*/

   /* Nothing */  {{100,100, 100, 100, 100, 100, 100, 100, 100, 100, 100},  0,DUN_ROOMS * 3,	1, 0, LF1_WILD | LF1_CAVERN | LF1_MINE | LF1_LABYRINTH},
   /* 'Empty' */  {{100,100, 100, 100, 100, 100, 100, 100, 100, 100, 100},  0,DUN_ROOMS * 3,	1, 0, LF1_DUNGEON | LF1_DESTROYED | LF1_WILD | LF1_CRYPT},
   /* Walls   */  {{80,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95},  0,DUN_ROOMS,	1, 0, LF1_STRONGHOLD | LF1_CRYPT | LF1_WILD},
   /* Centre */   {{60,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95},  0,DUN_ROOMS,	1, 0, LF1_STRONGHOLD | LF1_SEWER | LF1_WILD},
   /* Lrg wall */ {{ 0,  30,  60,  80,  90,  90,  90,  90,  90,  90,  90},  3,DUN_ROOMS/2,	2, 0, LF1_STRONGHOLD | LF1_WILD},
   /* Lrg cent */ {{ 0,  30,  60,  80,  90,  90,  90,  90,  90,  90,  90},  3,DUN_ROOMS/2,	2, 0, LF1_STRONGHOLD | LF1_SEWER | LF1_WILD},
   /* Xlg cent */ {{ 0,   0,   3,   3,   9,   9,  15,  15,  15,  15,  15}, 13,  3,	3, 0, LF1_STRONGHOLD},
   /* Chambers */ {{ 0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20},  7,	4,		5, 0, LF1_DUNGEON},
   /* I. Room */  {{30,  60,  70,  80,  80,  75,  70,  67,  65,  62,  60},  0,  6,		1, 0, LF1_THEME},
   /* L. Vault */ {{ 0,   1,   4,   9,  16,  27,  40,  55,  70,  80,  90},  7,	3,		3, 0, LF1_THEME},
   /* G. Vault */ {{ 0,   0,   0,   2,   3,   4,   6,   7,   8,  10,  12}, 20,	1,		6, 0, LF1_THEME},
   /* Starbrst */ {{ 0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20},  7,DUN_ROOMS/2,	2, 0, LF1_CAVERN | LF1_MINE},
   /* Hg star */  {{ 0,   0,   0,   0,   4,   4,   4,   4,   4,   4,   4}, 25,	1,		4, 0, LF1_MINE},
   /* Fractal */  {{ 0,  30,  60,  80,  90,  95,  90,  90,  90,  90,  90},  3,DUN_ROOMS/2,	1, 0, LF1_CAVE},
   /* Lrg fra */  {{ 0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20},  7,DUN_ROOMS/3,	2, 0, LF1_CAVE},
   /* Huge fra */ {{ 0,   0,   0,   0,   0,   4,   4,   4,   4,   4,   4}, 11,	3,		3, 0, LF1_CAVE},
   /* Lair */     {{ 0,   0,   0,   0,   4,   4,   4,   4,   4,   4,   4}, 25,	1,		6, 0, LF1_MINE | LF1_WILD},
   /* Maze */     {{ 0,  15,  30,  40,  45,  45,  50,  50,  50,  50,  50}, 1,DUN_ROOMS/2,	1, 0, LF1_THEME & ~(LF1_CAVE | LF1_CAVERN | LF1_POLYGON | LF1_BURROWS | LF1_DESTROYED)},
   /* Lrg maze */ {{ 0,   2,  6,   12,  18,  18,  19,  20,  20,  20,  20}, 6,DUN_ROOMS/3,	2, 0, LF1_THEME & ~(LF1_CAVE | LF1_CAVERN | LF1_POLYGON | LF1_BURROWS | LF1_DUNGEON | LF1_MINE | LF1_DESTROYED)},
   /* Huge maze */{{ 0,   0,   0,   4,   6,   6,   8,   8,  10,  10,  10}, 18,	3,		3, 0, LF1_THEME & ~(LF1_CAVE | LF1_CAVERN | LF1_POLYGON | LF1_BURROWS | LF1_DUNGEON | LF1_MINE | LF1_DESTROYED)},
   /* Cell cave */{{ 0,  15,  30,  40,  45,  45,  50,  50,  50,  50,  50}, 1,DUN_ROOMS,		1, 0, LF1_CAVERN | LF1_MINE},
   /* Lrg cell */ {{ 0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20}, 9,DUN_ROOMS/2,		2, 0, LF1_CAVERN},
   /* Huge cell */{{ 0,   0,   0,   4,   6,   6,   8,   8,  10,  10,  10}, 21,	3,		3, 0, LF1_CAVERN},
   /* Burrows */  {{ 0,  15,  30,  40,  45,  45,  50,  50,  50,  50,  50}, 1,DUN_ROOMS,	1, 0, (LF1_CRYPT | LF1_CAVE  | LF1_SEWER | LF1_CAVERN | LF1_LABYRINTH | LF1_BURROWS)},
   /* Lrg burrow*/{{ 0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20}, 6,DUN_ROOMS/2,	2, 0, (LF1_CRYPT | LF1_CAVE  | LF1_SEWER | LF1_CAVERN | LF1_LABYRINTH | LF1_BURROWS)},
   /* Huge burr.*/{{ 0,   0,   0,   4,   6,   6,   8,   8,  10,  10,  10}, 18,	3,		3, 0, (LF1_CRYPT | LF1_CAVE  | LF1_SEWER | LF1_CAVERN | LF1_LABYRINTH | LF1_BURROWS)},
   /* Monst.pit */{{ 0,   0,   0,   4,   6,   6,   8,   8,  10,  10,  10}, 18,	2,		2, 0, LF1_DUNGEON | LF1_CRYPT | LF1_LABYRINTH},
   /* Monst.town */{{ 0,   0,   0,   0,   4,   4,   4,   4,   4,   4,   4}, 25,	1,		6, 0, LF1_DUNGEON | LF1_CAVERN | LF1_MINE},
   /* Concave */  {{ 0,  30,  60,  80,  90,  90,  90,  90,  90,  90,  90}, 1,DUN_ROOMS,	1, 0, LF1_DUNGEON | LF1_POLYGON},
   /* Lrg c.cave*/{{ 0,   8,  15,  20,  22,  22,  25,  25,  25,  25,  25},  3,DUN_ROOMS/2,	2, 0, LF1_POLYGON},
   /* Xlg ccave */{{ 0,   0,   3,   3,   9,   9,  15,  15,  15,  15,  15}, 11,  3,	3, 0, LF1_POLYGON},
   /* Lake */     {{ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, 11,	1,		2, 0, LF1_WILD | LF1_CAVERN | LF1_SEWER},
   /* Huge lake */{{ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, 21,	1,		3, 0, LF1_WILD | LF1_CAVERN | LF1_SEWER},
   /* Tower */    {{ 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, 1,	1,		3, 0, LF1_TOWER}
};


/* Build rooms in descending order of difficulty of placing e.g. size, frequency. */
static byte room_build_order[ROOM_MAX] = {ROOM_LAIR, ROOM_GREATER_VAULT, ROOM_CHAMBERS, ROOM_MONSTER_TOWN, ROOM_HUGE_MAZE, 
						ROOM_HUGE_BURROW, ROOM_HUGE_CAVE, ROOM_HUGE_FRACTAL, ROOM_HUGE_STAR_BURST, ROOM_HUGE_CONCAVE, ROOM_HUGE_CENTRE, ROOM_LESSER_VAULT,
						ROOM_MONSTER_PIT, ROOM_LARGE_BURROW, ROOM_LARGE_MAZE, ROOM_LARGE_CAVE, ROOM_LARGE_FRACTAL, ROOM_LARGE_CONCAVE, ROOM_LARGE_CENTRE,
						ROOM_LARGE_WALLS, ROOM_INTERESTING, ROOM_STAR_BURST, ROOM_BURROW, ROOM_MAZE, ROOM_CELL_CAVE, ROOM_FRACTAL, ROOM_NORMAL_CONCAVE,
						ROOM_NORMAL_CENTRE, ROOM_NORMAL_WALLS, ROOM_NORMAL, 0, 0, 0, 0, 0};


/*
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y, x)"
 *
 * We count only granite walls and permanent walls.
 */
static int next_to_walls(int y, int x)
{
	int k = 0;

	if (f_info[cave_feat[y+1][x]].flags1 & (FF1_WALL)) k++;
	if (f_info[cave_feat[y+1][x]].flags1 & (FF1_WALL)) k++;
	if (f_info[cave_feat[y+1][x]].flags1 & (FF1_WALL)) k++;
	if (f_info[cave_feat[y+1][x]].flags1 & (FF1_WALL)) k++;

	return (k);
}



/*
 * Replace terrain with a mix of terrain types. Returns true iff
 * this is a mixed terrain type.
 */
static bool variable_terrain(int *feat, int oldfeat)
{
	int k;
	bool varies = FALSE;

	/* Hack -- ensure variable terrain creates continuous space */
	if (((f_info[oldfeat].flags1 & (FF1_MOVE)) == 0) &&
		((f_info[oldfeat].flags3 & (FF3_EASY_CLIMB)) == 0))
	{
		if (f_info[oldfeat].flags1 & (FF1_TUNNEL)) oldfeat = feat_state(oldfeat, FS_TUNNEL);
		else if (f_info[oldfeat].flags2 & (FF2_PATH)) oldfeat = feat_state(oldfeat, FS_BRIDGE);
		else oldfeat = FEAT_FLOOR;
	}

	/* Hack -- place trees to support need tree terrain */
	if (f_info[*feat].flags3 & (FF3_NEED_TREE))
	{
		k = randint(100);

		if (k<=15) *feat = FEAT_TREE;
		else if (k <= 25) *feat = feat_state(*feat, FS_NEED_TREE);

		varies = TRUE;
	}
	/* Hack -- place trees infrequently */
	else if (f_info[*feat].flags3 & (FF3_TREE))
	{
		k = randint(100);

		if (k<=85) *feat = oldfeat;

		varies = TRUE;
	}
	/* Hack -- place living terrain infrequently */
	else if (f_info[*feat].flags3 & (FF3_LIVING))
	{
		k = randint(100);
		if (k<=30) *feat = oldfeat;
		if (k<=90) *feat = FEAT_GRASS;

		varies = TRUE;
	}

	/* Hack -- place 'easy climb' terrain infrequently */
	else if (f_info[*feat].flags3 & (FF3_EASY_CLIMB))
	{
		k = randint(100);

		if ((k <= 20) && (f_info[*feat].flags2 & (FF2_CAN_DIG))) *feat = FEAT_RUBBLE;
		else if (k<=90) *feat = oldfeat;
		varies = TRUE;
	}

	/* Hack -- place 'adjacent terrain' infrequently */
	if (f_info[*feat].flags3 & (FF3_ADJACENT))
	{
		k = randint(100);

		if ((k <= 90) && ((f_info[f_info[*feat].edge].flags1 & (FF1_MOVE)) != 0)) *feat = f_info[*feat].edge;
		else if (k <= 90) *feat = oldfeat;
		else if (k <= 95) *feat = feat_state(*feat, FS_ADJACENT);
		varies = TRUE;
	}

	/* Hack -- place 'spreading terrain' infrequently */
	if (f_info[*feat].flags3 & (FF3_SPREAD))
	{
		k = randint(100);

		if ((k <= 90) && ((f_info[f_info[*feat].edge].flags1 & (FF1_MOVE)) != 0)) *feat = f_info[*feat].edge;
		else if ((k <= 90) && ((f_info[*feat].flags2 & (FF2_LAVA)) == 0)) *feat = oldfeat;
		else if (k <= 95) *feat = feat_state(*feat, FS_SPREAD);
		varies = TRUE;
	}

	/* Hack -- place 'erupting terrain' infrequently */
	if (f_info[*feat].flags3 & (FF3_ERUPT))
	{
		k = randint(100);

		if (k<=90) *feat = feat_state(*feat, FS_ERUPT);
		varies = TRUE;
	}

	/* Hack -- transform most 'timed' terrain */
	if (f_info[*feat].flags3 & (FF3_TIMED))
	{
		k = randint(100);

		if (k<=25) *feat = feat_state(*feat, FS_TIMED);
		varies = TRUE;
	}

	/* Some 'special' terrain types */
	if (*feat == oldfeat) switch (*feat)
	{
		/* Hack -- prevent random stone bridges in chasm */
		case FEAT_CHASM:
		case FEAT_CHASM_E:
			return (FALSE);

		default:
		{

			if ((f_info[*feat].edge) && ((f_info[f_info[*feat].edge].flags1 & (FF1_MOVE)) != 0))
			{
				k = randint(100);

				if (k<=20) *feat = f_info[*feat].edge;
				varies = TRUE;
			}
		}
	}

	return (varies);
}




/*
 * Hack -- mimic'ed feature for "room_info_feat()"
 */
static s16b room_info_feat_mimic;

/*
 *
 */
static bool room_info_feat(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	if (f_ptr->mimic == room_info_feat_mimic) return(TRUE);

	return(FALSE);
}


/*
 * Gets a feature that may match one of the 'mimic'ed features instead
 */
static s16b pick_room_feat(s16b feat)
{
	int newfeat;

	/* Paranoia */
	if (feat != f_info[feat].mimic) return (feat);

	/* Set feature hook */
	room_info_feat_mimic = feat;

	get_feat_num_hook = room_info_feat;

	/* Prepare allocation table */
	get_feat_num_prep();

	newfeat = get_feat_num(object_level);

	if (newfeat) feat = newfeat;

	get_feat_num_hook = NULL;

	/* Prepare allocation table */
	get_feat_num_prep();

	return(feat);
}



/*
 * Note that the order we generate the dungeon is terrain features, then
 * rooms, then corridors, then streamers. This is important, because
 * (currently) we ensure that deep or hostile terrain is bridged by safe
 * terrain, and rooms (and vaults) always have their respective walls intact.
 *
 * XXX XXX XXX Currently both types of lakes can have areas that are completely
 * blocked because of the 20% chance of filling a lake centre location with
 * a lake edge location. We should always guarantee that all areas are connected.
 *
 * XXX XXX These huge case statements should be cut down by using WALL, FLOOR,
 * etc. flags to take out the common cases and ensuring we never overwrite a
 * dun square with an edge square. But the resulting code might be less
 * efficient.
 */

/*
 * Places a terrain on another terrain
 */
static void build_terrain(int y, int x, int feat)
{
	int oldfeat, newfeat;
	feature_type *f_ptr;
	feature_type *f2_ptr;

	/* Get dungeon zone */
	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Get the feature */
	oldfeat = cave_feat[y][x];
	f_ptr = &f_info[oldfeat];

	/* Set the new feature */
	newfeat = oldfeat;
	f2_ptr = &f_info[feat];

	/* Special cases first */
	switch (feat)
	{
		case FEAT_QSAND_H:
		case FEAT_SAND_H:
		{
			if (oldfeat == FEAT_WATER) newfeat = FEAT_QSAND_H;
			if (oldfeat == FEAT_WATER_H) newfeat = FEAT_QSAND_H;
			break;
		}
		case FEAT_ICE_WATER_K:
		case FEAT_ICE_WATER_H:
		{
			if (f_ptr->flags2 & (FF2_LAVA)) newfeat = FEAT_BWATER;
			else if (f_ptr->flags2 & (FF2_ICE | FF2_WATER)) newfeat = feat;
			break;
		}
		case FEAT_BMUD:
		case FEAT_BWATER:
		{
			if (f_ptr->flags2 & (FF2_WATER)) newfeat = feat;
			if (f_ptr->flags2 & (FF2_HIDE_DIG)) newfeat = feat;
			if (oldfeat == FEAT_ICE) newfeat = FEAT_ICE_GEOTH;
			if (oldfeat == FEAT_ICE_C) newfeat = FEAT_ICE_GEOTH;
			if (oldfeat == FEAT_FLOOR_ICE) newfeat = FEAT_GEOTH;
			if (oldfeat == FEAT_LAVA) newfeat = FEAT_GEOTH;
			if (oldfeat == FEAT_LAVA_H) newfeat = feat;
			if (oldfeat == FEAT_LAVA_K) newfeat = feat;
			if (oldfeat == FEAT_FLOOR_EARTH) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_FLOOR_EARTH_T) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_SAND) newfeat = feat;
			if (oldfeat == FEAT_QSAND_H) newfeat = feat;
			if (oldfeat == FEAT_MUD_H) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_MUD_HT) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_MUD_K) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_EARTH) newfeat = FEAT_BMUD;
			break;
		}
		case FEAT_GEOTH:
		{
			if (oldfeat == FEAT_ICE) newfeat = FEAT_ICE_GEOTH;
			if (oldfeat == FEAT_ICE) newfeat = FEAT_ICE_GEOTH;
			if (oldfeat == FEAT_MUD_H) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_MUD_HT) newfeat = FEAT_BMUD;
			if (oldfeat == FEAT_MUD_K) newfeat = FEAT_BMUD;
			break;
		}
	}

	/* Have we handled a special case? */
	if (newfeat != oldfeat)
	{
		/* Nothing */
	}
	else if (!oldfeat)
	{
		newfeat = feat;
	}
	else if ((f_ptr->flags1 & (FF1_WALL))
		&& !(f_ptr->flags2 & (FF2_WATER | FF2_LAVA | FF2_ACID | FF2_OIL | FF2_ICE | FF2_CHASM))
		&& !(f_ptr->flags3 & (FF3_TREE)))
	{
		newfeat = feat;
	}
	else if (f2_ptr->flags2 & (FF2_COVERED))
	{
		newfeat = feat_state(oldfeat, FS_BRIDGE);
	}
	else if ((f_ptr->flags2 & (FF2_CHASM)) || (f2_ptr->flags2 & (FF2_CHASM)))
	{
		newfeat = feat_state(oldfeat,FS_CHASM);
	}
	else if (f_ptr->flags1 & (FF1_FLOOR))
	{
		newfeat = feat_state(feat,FS_TUNNEL);
	}
	else if (f2_ptr->flags1 & (FF1_FLOOR))
	{
		newfeat = feat_state(oldfeat,FS_TUNNEL);
	}
	else if (f_ptr->flags3 & (FF3_GROUND))
	{
		newfeat = feat;
	}
	else if (f2_ptr->flags3 & (FF3_GROUND))
	{
		newfeat = feat;
	}
	else if (f_ptr->flags2 & (FF2_LAVA))
	{
		if ((f2_ptr->flags2 & (FF2_ICE)) && (f2_ptr->flags1 & (FF1_WALL)) &&
                        (f2_ptr->flags2 & (FF2_CAN_OOZE)))
		{
			newfeat = FEAT_ICE_GEOTH_HC;
		}
		else if ((f2_ptr->flags2 & (FF2_ICE)) && (f2_ptr->flags1 & (FF1_WALL)))
		{
			newfeat = FEAT_ICE_GEOTH;
		}
		else if ((f2_ptr->flags2 & (FF2_HIDE_DIG)) && (f2_ptr->flags2 & (FF2_DEEP | FF2_FILLED)))
		{
			newfeat = FEAT_BMUD;
		}
		else if ((f2_ptr->flags2 & (FF2_WATER)) && (f2_ptr->flags2 & (FF2_DEEP | FF2_FILLED)))
		{
			newfeat = FEAT_BWATER;
		}
		else if (f2_ptr->flags2 & (FF2_WATER | FF2_ACID | FF2_OIL | FF2_ICE | FF2_CHASM))
		{
			newfeat = FEAT_GEOTH_LAVA;
		}
	}
	else if (f2_ptr->flags2 & (FF2_LAVA))
	{
		if ((f_ptr->flags2 & (FF2_ICE)) && (f_ptr->flags1 & (FF1_WALL)) &&
                        (f_ptr->flags2 & (FF2_CAN_OOZE)))
		{
			newfeat = FEAT_ICE_GEOTH_HC;
		}
		else if ((f_ptr->flags2 & (FF2_ICE)) && (f_ptr->flags1 & (FF1_WALL)))
		{
			newfeat = FEAT_ICE_GEOTH;
		}
		else if ((f_ptr->flags2 & (FF2_HIDE_DIG)) && (f_ptr->flags2 & (FF2_DEEP | FF2_FILLED)))
		{
			newfeat = FEAT_BMUD;
		}
		else if ((f_ptr->flags2 & (FF2_WATER)) && (f_ptr->flags2 & (FF2_DEEP | FF2_FILLED)))
		{
			newfeat = FEAT_BWATER;
		}
		else if (f_ptr->flags2 & (FF2_WATER | FF2_ACID | FF2_OIL | FF2_ICE | FF2_CHASM))
		{
			newfeat = FEAT_GEOTH_LAVA;
		}
		else
		{
			newfeat = feat;
		}
	}
	else if (f_ptr->flags2 & (FF2_ICE))
	{
		/* Handle case of ice wall over underwater */
                if ((f_ptr->flags1 & (FF1_WALL)) && (f_ptr->flags2 & (FF2_CAN_OOZE)))
		{
			if ((f2_ptr->flags2 & (FF2_WATER)) && (f2_ptr->flags2 & (FF2_FILLED))
			 && (f2_ptr->flags1 & (FF1_SECRET)))
			{
				newfeat = FEAT_UNDER_ICE_HC;
			}
			else if ((f2_ptr->flags2 & (FF2_WATER)) && (f2_ptr->flags2 & (FF2_FILLED)))
			{
				newfeat = FEAT_UNDER_ICE_KC;
			}
		}
		else if (f_ptr->flags1 & (FF1_WALL))
		{
			if ((f2_ptr->flags2 & (FF2_WATER)) && (f2_ptr->flags2 & (FF2_FILLED)))
			{
				newfeat = FEAT_UNDER_ICE;
			}
		}
		else if (f2_ptr->flags2 & (FF2_HURT_COLD)) newfeat = feat_state(feat,FS_HURT_COLD);
	}
	else if (f2_ptr->flags2 & (FF2_ICE))
	{
		/* Handle case of ice wall over underwater */
                if ((f2_ptr->flags1 & (FF1_WALL)) && (f2_ptr->flags2 & (FF2_CAN_OOZE)))
		{
			if ((f_ptr->flags2 & (FF2_WATER)) && (f_ptr->flags2 & (FF2_FILLED))
			 && (f_ptr->flags1 & (FF1_SECRET)))
			{
				newfeat = FEAT_UNDER_ICE_HC;
			}
			else if ((f_ptr->flags2 & (FF2_WATER)) && (f_ptr->flags2 & (FF2_FILLED)))
			{
				newfeat = FEAT_UNDER_ICE_KC;
			}
		}
		else if (f2_ptr->flags1 & (FF1_WALL))
		{
			if ((f_ptr->flags2 & (FF2_WATER)) && (f_ptr->flags2 & (FF2_FILLED)))
			{
				newfeat = FEAT_UNDER_ICE;
			}
		}
		else if (f_ptr->flags2 & (FF2_HURT_COLD)) newfeat = feat_state(oldfeat,FS_HURT_COLD);
		else if (f_ptr->flags2 & (FF2_HIDE_DIG)) newfeat = feat;
	}
	else if ((f_ptr->flags2 & (FF2_WATER)) && (f2_ptr->flags2 & (FF2_WATER)))
	{
		/* Hack -- we try and match water properties */
		u32b mask1 = (FF1_SECRET | FF1_LESS);
		u32b mask2 = (FF2_WATER | FF2_SHALLOW | FF2_FILLED | FF2_DEEP | FF2_ICE | FF2_LAVA | FF2_CHASM | FF2_HIDE_SWIM);
		u32b match1 = 0x0L;
		u32b match2 = FF2_WATER;

		int k_idx = f_info[oldfeat].k_idx;

		int i;

		/* Hack -- get most poisonous object */
		if (f_info[feat].k_idx > k_idx) k_idx = f_info[feat].k_idx;

		/* Hack -- get flags */
		if ((f_ptr->flags2 & (FF2_SHALLOW)) || (f2_ptr->flags2 & (FF2_SHALLOW)))
		{
			match2 |= FF2_SHALLOW;
		}
		else if ((f_ptr->flags2 & (FF2_FILLED)) || (f2_ptr->flags2 & (FF2_FILLED)))
		{
			match2 |= FF2_FILLED;
			match1 |= ((f_ptr->flags1 & (FF1_SECRET)) || (f2_ptr->flags1 & (FF1_SECRET)));
		}
		else
		{
			match2 |= FF2_DEEP;
			match2 |= ((f_ptr->flags2 & (FF2_HIDE_SWIM)) || (f2_ptr->flags2 & (FF2_HIDE_SWIM)));
			match1 |= ((f_ptr->flags1 & (FF1_SECRET)) || (f2_ptr->flags1 & (FF1_SECRET)));
			match1 |= ((f_ptr->flags1 & (FF1_LESS)) || (f2_ptr->flags1 & (FF1_LESS)));
		}

		for (i = 0;i < z_info->f_max;i++)
		{
			/* Hack -- force match */
			if ((f_info[i].flags1 & (mask1)) != match1) continue;
			if ((f_info[i].flags2 & (mask2)) != match2) continue;

			if (f_info[i].k_idx != k_idx) continue;

			newfeat = i;
		}

	}
	else if ((f_ptr->flags2 & (FF2_CAN_DIG)) || (f2_ptr->flags2 & (FF2_CAN_DIG)))
	{
		newfeat = feat;
	}

	/* Hack -- unchanged? */
	if (newfeat == oldfeat)
	{
		if (feat == zone->big) newfeat = zone->big;
		else if (feat == zone->small) newfeat = zone->small;
	}

	/* Vary the terrain */
	if (variable_terrain(&feat, oldfeat))
	{
		newfeat = feat;
	}
	/* Pick features if needed */
	else if (f_info[newfeat].flags1 & (FF1_HAS_ITEM | FF1_HAS_GOLD))
	{
		newfeat = pick_room_feat(feat);
	}

	/* Hack -- no change */
	if (newfeat == oldfeat) return;

	/* Set the feature if we have a change */
	cave_set_feat(y,x,newfeat);

	/* Change reference */
        f2_ptr = &f_info[newfeat];

	/*
	 * Handle creation of big trees.
         *
         * Note hack to minimise number of calls to rand_int.
	 */
	if (f_info[newfeat].flags3 & (FF3_TREE))
	{
            int k = 0;
		int i;

            k = rand_int(2<<26);

		/* Place branches over trunk */
            if (k & (0xFF000000)) cave_alter_feat(y,x,FS_TREE);

		for (i = 0; i < 8; i++)
		{
			int yy,xx;

			yy = y + ddy_ddd[i];
			xx = x + ddx_ddd[i];

			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Ignore if not placing a tree */
			/* Hack -- we make it 150% as likely to place branches on non-diagonal locations */
                  if (!(k & (2 << i)) && !(k & (2 << (i+8) )) && !((i<4) && (k & (2 << (i+16)))) ) continue;

			/* Place branches */
			cave_alter_feat(yy,xx,FS_TREE);
		}
	}
}



/*
 * Generate helper -- create a new room with optional light
 */
static void generate_room(int y1, int x1, int y2, int x2, int light)
{
	int y, x;

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			cave_info[y][x] |= (CAVE_ROOM);
			if (light) cave_info[y][x] |= (CAVE_GLOW);
		}
	}
}


/*
 * Generate helper -- fill a rectangle with a feature
 */
static void generate_fill(int y1, int x1, int y2, int x2, int feat)
{
	int y, x;

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			cave_set_feat(y, x, feat);
		}
	}
}


/*
 * Generate helper -- fill a rectangle with a feature. Place pillars if spacing set.
 */
static void generate_fill_pillars(int y1, int x1, int y2, int x2, int feat, int spacing, int scale)
{
	int y, x;

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			/* Check spacing */
			if ((spacing > 1) && (y % spacing < scale) && (x % spacing < scale) &&
					(x != x1) && (x != x2) && (y != y1) && (y != y2))
			{
				cave_set_feat(y, x, FEAT_WALL_INNER);
			}
			else
			{
				build_terrain(y, x, feat);
			}
		}
	}
}


/*
 * Generate helper -- draw a rectangle with a feature
 */
static void generate_rect(int y1, int x1, int y2, int x2, int feat)
{
	int y, x;

	for (y = y1; y <= y2; y++)
	{
		cave_set_feat(y, x1, feat);
		cave_set_feat(y, x2, feat);
	}

	for (x = x1; x <= x2; x++)
	{
		cave_set_feat(y1, x, feat);
		cave_set_feat(y2, x, feat);
	}
}


/*
 * Generate helper -- open one side of a rectangle with a feature
 */
static void generate_hole(int y1, int x1, int y2, int x2, int feat)
{
	int y0, x0;

	/* Center */
	y0 = (y1 + y2) / 2;
	x0 = (x1 + x2) / 2;

	/* Open random side */
	switch (rand_int(4))
	{
		case 0:
		{
			cave_set_feat(y1, x0, feat);
			break;
		}
		case 1:
		{
			cave_set_feat(y0, x1, feat);
			break;
		}
		case 2:
		{
			cave_set_feat(y2, x0, feat);
			break;
		}
		case 3:
		{
			cave_set_feat(y0, x2, feat);
			break;
		}
	}
}


/*
 * Generate door -- like generate hole above, but pick anywhere to
 * put the door, not just the centre
 */
static void generate_door(int y1, int x1, int y2, int x2, int side, int feat)
{
	int y, x;
	
	/* Extract a "door location" */
	switch (side)
	{
		/* Bottom side */
		case 0:
		{
			y = y2;
			x = rand_range(x1, x2);
			break;
		}

		/* Top side */
		case 1:
		{
			y = y1;
			x = rand_range(x1, x2);
			break;
		}

		/* Right side */
		case 2:
		{
			y = rand_range(y1, y2);
			x = x2;
			break;
		}

		/* Left side */
		default:
		{
			y = rand_range(y1, y2);
			x = x1;
			break;
		}
	}

	/* Clear previous contents, add a store door */
	cave_set_feat(y, x, feat);

}

/*
 * Record good location for quest objects
 */
static bool add_quest(int y, int x)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Record location */
	if (dun->quest_n < QUEST_MAX)
	{
		dun->quest[dun->quest_n].y = y;
		dun->quest[dun->quest_n].x = x;
		dun->quest_n++;

		return (TRUE);
	}

	return (FALSE);
}



/* Convert a maze coordinate into a dungeon coordinate */
#define YPOS(y, y1)		((y1) + (y * (width_path + width_wall)) + width_outer)
#define XPOS(x, x1)		((x1) + (x * (width_path + width_wall)) + width_outer)


#define MAZE_WALL		0x00000001L	/* Surround maze with width 1 wall */
#define MAZE_SAVE		0x00000002L	/* Save contents overwritten by maze and try to place them again afterwards */
#define MAZE_ROOM		0x00000004L	/* Set the cave_room flag */
#define MAZE_LITE		0x00000008L	/* Set the cave_glow flag */
#define MAZE_OUTER_N	0x00000010L	/* Make north edge of maze have 'outer' walls (requires MAZE_WALL set) */
#define MAZE_OUTER_S	0x00000020L	/* Make south edge of maze have 'outer' walls (requires MAZE_WALL set) */
#define MAZE_OUTER_W	0x00000040L	/* Make west edge of maze have 'outer' walls (requires MAZE_WALL set) */
#define MAZE_OUTER_E	0x00000080L	/* Make east edge of maze have 'outer' walls (requires MAZE_WALL set) */
#define MAZE_EXIT_N		0x00000100L	/* Make exit on north edge of maze */
#define MAZE_EXIT_S		0x00000200L	/* Make exit on south edge of maze */
#define MAZE_EXIT_W		0x00000400L	/* Make exit on west edge of maze */
#define MAZE_EXIT_E		0x00000800L	/* Make exit on east edge of maze */
#define MAZE_DEAD		0x00001000L	/* Fill some dead ends with walls */
#define MAZE_POOL		0x00002000L	/* Fill some dead ends with pools */
#define MAZE_DOOR		0x00004000L	/* Hide some dead ends with secret doors */
#define MAZE_FILL		0x00008000L	/* Fill some dead ends with useful goodies */
#define MAZE_CRYPT		0x00010000L	/* Paths in maze have ragged edges like crypt corridors (width_path must be greater than 1)*/
#define MAZE_CAVE		0x00020000L	/* Paths in maze wander like cave corridors (width_path must be 2) */
#define MAZE_FLOOD		0x00040000L	/* Fill in some dead ends with flooded terrain - open these dead ends to surrounds */
#define MAZE_LOOP		0x00080000L	/* Open some dead ends */



/*
 * Build an acyclic maze inside a given rectangle.  - Eric Bock -
 * Construct the maze from a given pair of features.
 *
 * Note that the edge lengths should be odd for standard width_wall = 1 and width_path = 1 mazes.
 *
 * Width_wall indicates the width of the maze 'walls' and should be >= 1.
 * Width_path indicates the width of the maze 'paths' and should be >= 1.
 *
 * Note that the outer walls of the maze are always width 1, which makes the maze math a little
 * more complicated, but allows tunnels to correctly connect to the edge of the maze.
 *
 * Important note particular for developers: width and height is given by x2 - x1 + 1 and
 * y2 - y1 + 1. So this means an odd width (for instance)
 *
 * Filled indicates how much of the maze dead ends are 're-filled' with walls.
 * Secrets indicates how much of the maze dead ends are hidden by secret doors.
 * Quests indicates how much of the maze dead ends are changed to quest locations.
 *
 * If room is true, the maze will refill edges locations preferentially, and turn those edge
 * locations into non-room locations. The outer edges of the maze will also be converted into
 * solid and outer walls respectively if the feat_wall feature is a WALL. This allows the
 * construction of maze rooms required for labrynth levels.
 */
static bool draw_maze(int y1, int x1, int y2, int x2, s16b feat_wall,
    s16b feat_path, int width_wall, int width_path, s16b feat_pool, u32b flag)
{
	int i, j;
	int ydim, xdim;
	int grids;

	int y, x;
	int ty = 0, tx = 0;
	int dy = 0, dx = 0;
	int yi, xi;

	int width_outer = flag & (MAZE_WALL) ? 1 : 0;

	byte dir[4];
	byte dirs;

	int count = 0;

	int offset = ((flag & (MAZE_CRYPT | MAZE_CAVE)) != 0) && (rand_int(100) < 50) ? 1 : 0;

	s16b *saved = NULL;

	int solid = ((f_info[feat_wall].flags1 & (FF1_OUTER)) != 0) ? feat_state(feat_wall, FS_SOLID) : 0;
	int inner = ((f_info[feat_wall].flags1 & (FF1_OUTER)) != 0) ? feat_state(feat_wall, FS_INNER) : feat_wall;

	/* Paranoia */
	if ((!feat_wall) || (!feat_path) || (feat_wall == feat_path) || (width_wall < 1) || (width_path < 1)) return (FALSE);

	/* Paranoia */
	if (!in_bounds_fully(y1, x1) || !in_bounds_fully(y2, x2)) return (FALSE);

	/* Paranoia */
	if ((y2 - y1 <= 0) || (x2 - x1 <= 0)) return (FALSE);

#if 0

	/* Extra crispy paranoia */
	if ((y2 - y1) % (width_wall + width_path) != (width_wall + 2) % (width_wall + width_path)) return;
	if ((x2 - x1) % (width_wall + width_path) != (width_wall + 2) % (width_wall + width_path)) return;
#endif

	if (cheat_room) message_add(format("Drawing maze of %s (%d) filled with %s (%d).", f_name + f_info[feat_wall].name, width_wall,
			f_name + f_info[feat_path].name, width_path), MSG_GENERIC);

	/* Save the existing terrain to overwrite the maze later */
	if ((flag & (MAZE_SAVE)) != 0)
	{
		int const dx =  1 + x2 - x1;
		int const dy =  1 + y2 - y1;

		saved = C_ZNEW(dx*dy,s16b);

		/* Save grids */
		for (y = 0; y < dy; y++)
		{
			for (x = 0; x < dx; x++)
			{
				saved[y * dx + x] = cave_feat[y + y1][x + x1];
			}
		}
	}

	/* Start with a solid rectangle of the "inner" wall feat */
	generate_fill(y1, x1, y2, x2, inner);

	/* Draw room if requested */
	if (flag & (MAZE_ROOM)) generate_room(y1, x1, y2, x2, ((flag & (MAZE_LITE)) != 0));

	/* If maze has a wall and feat_wall is an outer wall, surround by 'outer' and 'solid' sections of wall to allow corridor connections */
	if (((flag & (MAZE_WALL)) != 0) && (solid))
	{
		if (flag & (MAZE_OUTER_W | MAZE_OUTER_E))
		{
			for (y = y1; y <= y2; y++)
			{
				if (flag & (MAZE_OUTER_W)) cave_set_feat(y, x1, (y - y1 - 1) % (width_wall + width_path) < width_path ? feat_wall : solid);
				if (flag & (MAZE_OUTER_E)) cave_set_feat(y, x2, (y - y1 - 1) % (width_wall + width_path) < width_path ? feat_wall : solid);
			}
		}

		if (flag & (MAZE_OUTER_N | MAZE_OUTER_S))
		{
			for (x = x1; x <= x2; x++)
			{
				if (flag & (MAZE_OUTER_N)) cave_set_feat(y1, x, (x - x1 - 1) % (width_wall + width_path) < width_path ? feat_wall : solid);
				if (flag & (MAZE_OUTER_S)) cave_set_feat(y2, x, (x - x1 - 1) % (width_wall + width_path) < width_path ? feat_wall : solid);
			}
		}
	}

	/* Calculate dimensions.*/

	/* Note we correct by a factor of wall width, to allow the division to work
	 * minus two (for the requirement of two outer width 1 walls)
	 * plus 1 because y1 = 10, y2 = 10 implies a width 1 room. */

	ydim = ((y2 - y1) + width_wall - (2 * width_outer) + 1) / (width_wall + width_path);
	xdim = ((x2 - x1) + width_wall - (2 * width_outer) + 1) / (width_wall + width_path);

	/* Number of unexamined grids */
	grids = ydim * xdim - 1;

	/* Set the initial position */
	y = rand_int(ydim);
	x = rand_int(xdim);

	/* Place floor here */
	for (yi = YPOS(y, y1); yi < YPOS(y, y1) + width_path; yi++)
	{
		for (xi = XPOS(x, x1); xi < XPOS(x, x1) + width_path; xi++)
		{
			/* XXX We have to use the auxiliary cave_set_feat function.
			 * This is because trees and chasms would otherwise create infinite loops, as cave_set_feat(yi, xi, feat)
			 * does not guarantee that cave_feat[yi][xi] = feat_path. */
			cave_set_feat_aux(yi, xi, feat_path);
		}
	}

	/* Now build the maze */
	while ((++count < 10000) && (grids))
	{
		/* Only use maze grids */
		if ((cave_feat[YPOS(y, y1)][XPOS(x, x1)] == feat_path) || ((width_path > 1) &&
				(cave_feat[YPOS(y, y1)][XPOS(x, x1) + 1] == feat_path)))
		{
			/* Pick a target */
			ty = rand_int(ydim);
			tx = rand_int(xdim);

			while (TRUE)
			{
				dirs = 0;
				dy = 0;
				dx = 0;

				/* Calculate the dungeon position */
				j = YPOS(y, y1);
				i = XPOS(x, x1);

				/** Enumerate possible directions **/

				/* Up */
				if (y && (cave_feat[j - width_wall - width_path][i] != feat_path) && ((width_path == 1) ||
					(cave_feat[j - width_wall - width_path][i + 1] != feat_path)))  dir[dirs++] = 1;

				/* Down */
				if ((y < ydim - 1) && (cave_feat[j + width_wall + width_path][i] != feat_path) && ((width_path == 1) ||
					(cave_feat[j + width_wall + width_path][i + 1] != feat_path))) dir[dirs++] = 2;

				/* Left */
				if (x && (cave_feat[j][i - width_wall - width_path] != feat_path) && ((width_path == 1) ||
					(cave_feat[j + 1][i - width_wall - width_path] != feat_path))) dir[dirs++] = 3;

				/* Right */
				if ((x < xdim - 1) && (cave_feat[j][i + width_wall + width_path] != feat_path) && ((width_path == 1) ||
					(cave_feat[j + 1][i + width_wall + width_path] != feat_path))) dir[dirs++] = 4;

				/* Dead end; go to the next valid grid */
				if (!dirs) break;

				/* Pick a random direction */
				switch (dir[rand_int(dirs)])
				{
					/* Move up */
					case 1:  dy = -1;  break;

					/* Move down */
					case 2:  dy =  1;  break;

					/* Move left */
					case 3:  dx = -1;  break;

					/* Move right */
					case 4:  dx =  1;  break;
				}

				/* Place floors */
				for (yi = j - (dy < 0 ? width_wall + width_path : 0); yi < j + (dy < 0 ? 0 : width_path) + (dy > 0 ? width_wall + width_path : 0); yi++)
				{
					for (xi = i - (dx < 0 ? width_wall + width_path : 0); xi < i + (dx < 0 ? 0 : width_path) + (dx > 0 ? width_wall + width_path : 0); xi++)
					{
						/* Leave pillars in centre of width 3 corridors, except crypts and caves */
						if ((width_path == 3) && ((flag & (MAZE_CRYPT | MAZE_CAVE)) == 0))
						{
							/* Pillar in current grid */
							if ((yi == j + 1) && (xi == i + 1)) continue;

							/* Pillar in destination grid (note width_path replaced by 3) */
							if ((yi == j + 1 + (dy * (width_wall + 3))) && (xi == i + 1 + (dx * (width_wall + 3)))) continue;

							/* Pillar in-between if width_wall is 3 */
							if ((width_wall == 3) && ((yi == j + 1 + (dy * 3)) && (xi == i + 1 + (dx * 3)))) continue;
						}

						/* Leave pillars on edge of corridor for crypts */
						else if ((width_path > 2) && ((flag & (MAZE_CRYPT)) != 0))
						{
							/* Leave pillars on edges of corridor */
							if ((xi + yi) % 2 == offset)
							{
								if ((dy) && ((xi < i + 1) || (xi >= i + width_path - 1))) continue;
								else if ((dx) && ((yi < j + 1) || (yi >= j + width_path - 1))) continue;
							}
						}

						/* Draw the path */
						/* XXX We have to use the auxiliary cave_set_feat function.
						 * This is because trees and chasms would otherwise create infinite loops, as cave_set_feat(yi, xi, feat)
						 * does not guarantee that cave_feat[yi][xi] = feat_path. */
						cave_set_feat_aux(yi, xi, feat_path);
					}
				}

				/* Advance */
				y += dy;
				x += dx;

				/* One less grid to examine */
				grids--;

				/* Check for completion */
				if ((y == ty) && (x == tx)) break;
			}
		}

		/* Find a new position */
		y = rand_int(ydim);
		x = rand_int(xdim);
	}

	/* Warn the player */
	if (cheat_xtra && count >= 10000)
		message_add("Bug: Bad maze on level. Please report.", MSG_GENERIC);

	/* Create exits */
	if (flag & (MAZE_EXIT_N | MAZE_EXIT_S | MAZE_EXIT_W | MAZE_EXIT_E))
	{
		int feat = feat_path;

		/* Get door instead */
		if (f_info[feat_wall].flags1 & (FF1_OUTER)) feat = feat_state(feat_wall, FS_DOOR);

		if (flag & (MAZE_EXIT_N))
		{
			dx = rand_int(xdim);

			for (x = 0; x < width_path; x++)
			{
				/* Convert to path grid */
				cave_set_feat(y1, XPOS(dx, x1) + x, feat);
			}
		}

		if (flag & (MAZE_EXIT_S))
		{
			dx = rand_int(xdim);

			for (x = 0; x < width_path; x++)
			{
				/* Convert to path grid */
				cave_set_feat(y2, XPOS(dx, x1) + x, feat);
			}
		}

		if (flag & (MAZE_EXIT_W))
		{
			dy = rand_int(ydim);

			for (y = 0; y < width_path; y++)
			{
				/* Convert to path grid */
				cave_set_feat(YPOS(dy, y1) + y, x1, feat);
			}
		}

		if (flag & (MAZE_EXIT_E))
		{
			dy = rand_int(ydim);

			for (y = 0; y < width_path; y++)
			{
				/* Convert to path grid */
				cave_set_feat(YPOS(dy, y1) + y, x2, feat);
			}
		}
	}

	/* Clear temp flags */
	if (flag & (MAZE_FILL))
	{
		for (y = y1 + width_outer; y <= y2 - width_outer; y++)
		{
			for (x = x1 + width_outer; x <= x2 - width_outer; x++)
			{
				play_info[y][x] &= ~(PLAY_TEMP);
			}
		}
	}

	/* Partially fill the maze back in */
	if (flag & (MAZE_DEAD | MAZE_POOL))
	{
		/* Less dead ends if not surrounded by wall */
		int n = ydim * xdim - ( ((flag & (MAZE_WALL)) == 0) ? 2 * ydim + 2 * xdim - 4 : 0);

		int loops = (flag & (MAZE_LOOP)) ?  1 /* n / 4 */ : 0;
		int flood = (flag & (MAZE_FLOOD)) ? n / 4 : 0;
		int pools = (flag & (MAZE_POOL)) ? n / 4 : 0;
		int doors = (flag & (MAZE_DOOR)) ? n / 4 : 0;
		int stuff = (flag & (MAZE_FILL)) ? n / 4 : 0;

		/* Number of grids to fill back in */
		if (flag & (MAZE_DEAD)) grids = n / 4;

		/* Paranoia */
		if(loops<0) loops=0;
		if(pools<0) pools=0;
		if(doors<0) doors=0;
		if(stuff<0) stuff=0;
		if(grids<0) grids=0;

		/* Dead end filler */
		while (loops || grids || pools || doors || stuff)
		{
			int k = 0;

			for (y = 0; y < ydim; y++)
			{
				for (x = 0; x < xdim; x++)
				{
					dirs = 0;

					j = YPOS(y, y1);
					i = YPOS(x, x1);

					/* Already filled */
					if (cave_feat[j][i] != feat_path) continue;

					/* Already filled - marked with a temp flag */
					if (((flag & (MAZE_FILL | MAZE_DOOR)) != 0) && ((play_info[j][i] & (PLAY_TEMP)) != 0)) continue;

					/* Up */
					if ((y || (width_outer)) && (cave_feat[j - 1][i + width_path / 2] == feat_path)) dir[dirs++] = 1;

					/* Down */
					if (((y < ydim - 1) || (width_outer)) && (cave_feat[j + width_path][i + width_path / 2] == feat_path)) dir[dirs++] = 2;

					/* Left */
					if ((x || (width_outer)) && (cave_feat[j + width_path / 2][i - 1] == feat_path)) dir[dirs++] = 3;

					/* Right */
					if (((x < xdim - 1) || (width_outer)) && (cave_feat[j + width_path / 2][i + width_path] == feat_path)) dir[dirs++] = 4;

					/* Dead end */
					if ((dirs == 1) && (one_in_(++k)))
					{
						ty = y;
						tx = x;

						/* Note the direction */
						switch (dir[0])
						{
							/* Move up */
							case 1:  dy = -1; dx = 0;  break;

							/* Move down */
							case 2:  dy =  1; dx = 0;  break;

							/* Move left */
							case 3:  dx = -1; dy = 0; break;

							/* Move right */
							case 4:  dx =  1; dy = 0; break;
						}
					}
				}
			}

			/* Paranoia */
			if (!k) break;

			/* Convert coordinates */
			y = ty;
			x = tx;
			j = YPOS(y, y1);
			i = YPOS(x, x1);

			/* Open up dead-end as a loop */
			if (loops)
			{
				/* Place floors */
				for (yi = j - (dy < 0 ? width_wall + width_path : 0); yi < j + (dy < 0 ? 0 : width_path) + (dy > 0 ? width_wall + width_path : 0); yi++)
				{
					for (xi = i - (dx < 0 ? width_wall + width_path : 0); xi < i + (dx < 0 ? 0 : width_path) + (dx > 0 ? width_wall + width_path : 0); xi++)
					{
						/* Leave pillars in centre of width 3 corridors, except crypts and caves */
						if ((width_path == 3) && ((flag & (MAZE_CRYPT | MAZE_CAVE)) == 0))
						{
							/* Pillar in current grid */
							if ((yi == j + 1) && (xi == i + 1)) continue;

							/* Pillar in destination grid (note width_path replaced by 3) */
							if ((yi == j + 1 + (dy * (width_wall + 3))) && (xi == i + 1 + (dx * (width_wall + 3)))) continue;

							/* Pillar in-between if width_wall is 3 */
							if ((width_wall == 3) && ((yi == j + 1 + (dy * 3)) && (xi == i + 1 + (dx * 3)))) continue;
						}

						/* Leave pillars on edge of corridor for crypts */
						else if ((width_path > 2) && ((flag & (MAZE_CRYPT)) != 0))
						{
							/* Leave pillars on edges of corridor */
							if ((xi + yi) % 2 == offset)
							{
								if ((dy) && ((xi < i + 1) || (xi >= i + width_path - 1))) continue;
								else if ((dx) && ((yi < j + 1) || (yi >= j + width_path - 1))) continue;
							}
						}

						/* Draw the path */
						/* XXX We have to use the auxiliary cave_set_feat function.
						 * This is because trees and chasms would otherwise create infinite loops, as cave_set_feat(yi, xi, feat)
						 * does not guarantee that cave_feat[yi][xi] = feat_path. */
						cave_set_feat_aux(yi, xi, feat_path);
					}
				}

				loops--;
			}

			/* Fill in dead-end or flood */
			else if (grids || flood)
			{
				/* Fill in floors */
				for (yi = j - (dy < 0 ? width_wall : 0); yi < j + width_path + (dy > 0 ? width_wall : 0); yi++)
				{
					for (xi = i - (dx < 0 ? width_wall : 0); xi < i + width_path + (dx > 0 ? width_wall : 0); xi++)
					{
						if (grids) build_terrain(yi, xi, inner);
						else cave_set_feat(yi, xi, feat_pool);
					}
				}

				/* Move outer walls in and remove parts of room */
				if ((width_outer) && (solid) && ((flag & (MAZE_ROOM)) != 0))
				{
					/* Clear room flags as required */
					for (yi = j - (y ? width_wall : 1); yi < j + width_path + ((y < ydim - 1) ? width_wall : 1); yi++)
					{
						for (xi = i - (x ? width_wall : 1); xi < i + width_path + ((x < xdim - 1) ? width_wall : 1); xi++)
						{
							bool clear = TRUE;

							if ((yi < j - width_wall + 1) && (y) && ((cave_info[j - width_wall - 1][i] & (CAVE_ROOM)) != 0)) clear = FALSE;
							if ((xi < i - width_wall + 1) && (x) && ((cave_info[j][i - (x ? width_wall : 1) - 1] & (CAVE_ROOM)) != 0)) clear = FALSE;
							if ((yi >= j + width_path + width_wall - 1) && (y < ydim - 1) && ((cave_info[j + width_path + width_wall][i] & (CAVE_ROOM)) != 0)) clear = FALSE;
							if ((xi >= i + width_path + width_wall - 1) && (x < xdim - 1) && ((cave_info[j][i + width_path + width_wall] & (CAVE_ROOM)) != 0)) clear = FALSE;

							/* No longer in room */
							if (clear) cave_info[yi][xi] &= ~(CAVE_ROOM | CAVE_LITE);

							/* On edge of room */
							else
							{
								if (!grids) cave_info[yi][xi] &= ~(CAVE_ROOM | CAVE_LITE);
								else if (((yi - y1 - 1) % (width_wall + width_path) < width_path) &&
										((xi - x1 - 1) % (width_wall + width_path) < width_path)) cave_set_feat(yi, xi, feat_wall);
								else cave_set_feat(yi, xi, solid);
							}
						}
					}
				}

				if (grids) grids--;
				else flood--;
			}

			/* Fill in pool */
			else if (pools)
			{
				/* Fill in floors */
				for (yi = j - (dy < 0 ? width_wall : 0); yi < j + width_path + (dy > 0 ? width_wall : 0); yi++)
				{
					for (xi = i - (dx < 0 ? width_wall : 0); xi < i + width_path + (dx > 0 ? width_wall : 0); xi++)
					{
						if (cave_feat[yi][xi] != feat_path) continue;
						cave_set_feat(yi, xi, feat_pool);
					}
				}

				/* Ensure that external tunnels don't connect to pools in dead ends.*/
				/* In flooded mazes, we open up the dead end to 'outside' the maze */
				if (solid)
				{
					/* Check west & east */
					for (yi = j - 1; yi <= j + width_path; yi++)
					{
						if (cave_feat[yi][i - 1] == feat_wall) cave_set_feat(yi, i - 1, solid);
						if (cave_feat[yi][i + width_path] == feat_wall) cave_set_feat(yi, i + width_path, solid);
					}

					/* Check north & south */
					for (xi = i - 1; xi <= i + width_path; xi++)
					{
						if (cave_feat[j - 1][xi] == feat_wall) cave_set_feat(j - 1, xi, solid);
						if (cave_feat[j + width_path][xi] == feat_wall) cave_set_feat(j + width_path, xi, solid);
						}
				}

				pools--;
			}

			/* Fill in stuff and doors */
			else if (stuff | doors)
			{
				/* Fill in stuff */
				for (yi = j; yi < j + width_path; yi++)
				{
					for (xi = i; xi < i + width_path; xi++)
					{
						if (play_info[yi][xi] & (PLAY_TEMP)) continue;

						/* Good candidate for quest */
						if (!rand_int(width_path)) add_quest(yi, xi);

						play_info[yi][xi] |= PLAY_TEMP;
					}
				}

				if (stuff) stuff--;
				else doors--;
			}
		}
	}

	/* Clear temp flags */
	if (flag & (MAZE_FILL))
	{
		for (y = y1 + width_outer; y <= y2 - width_outer; y++)
		{
			for (x = x1 + width_outer; x <= x2 - width_outer; x++)
			{
				play_info[y][x] &= ~(PLAY_TEMP);
			}
		}
	}

#if 0
	/* Distribute cave grids */
	if (flag & (MAZE_CAVE))
	{
		message_add("cave maze", MSG_GENERIC);

		for (y = y1 + width_outer; y <= y2 - width_outer; y++)
		{
			for (x = x1 + width_outer; x <= x2 - width_outer; x++)
			{
				int move;
				byte add_info = ((flag & (MAZE_ROOM)) != 0 ? CAVE_ROOM : 0) | ((flag & (MAZE_LITE)) != 0 ? CAVE_LITE : 0);

				if (cave_feat[y][x] == inner) continue;
				if (cave_feat[y][x] == feat_wall) continue;
				if (cave_feat[y][x] == solid) continue;

				if (!y || !x) continue;

				move = 0;
				if (((y - y1 - width_outer) % (width_wall + width_path) < width_path) && (rand_int(100) < 50)) move += 1;
				if (((x - x1 - width_outer) % (width_wall + width_path) < width_path) && (rand_int(100) < 50)) move += 2;

				switch(move)
				{
					case 1:
					{
						cave_set_feat(y + 1, x, cave_feat[y][x]);
						cave_info[y+1][x] |= add_info;

						/* Important - this ensures the top and left hand walls are affected correctly */
						if  ((feat_wall != inner) && ((cave_feat[y-1][x] == solid) || (cave_feat[y-1][x] == feat_wall)) &&
								((cave_feat[y-1][x+1] == solid) || (cave_feat[y-1][x+1] == feat_wall) || (cave_feat[y-1][x+1] == inner)))
						{
							if (rand_int(100) < 75)
							{
								cave_set_feat(y, x, cave_feat[y-1][x]);
								cave_set_feat(y - 1, x, FEAT_WALL_EXTRA);
								if (flag & (MAZE_ROOM)) cave_info[y-1][x] &= ~(CAVE_ROOM | CAVE_GLOW);
							}
						}
						else if ((cave_feat[y-1][x] == inner) && (cave_feat[y-1][x+1] == inner) && (rand_int(100) < 75)) cave_set_feat(y, x, inner);
						break;
					}
					case 2:
					{
						cave_set_feat(y, x + 1, cave_feat[y][x]);
						cave_info[y][x+1] |= add_info;

						/* Important - this ensures the top and left hand walls are affected correctly */
						if  ((feat_wall != inner) && ((cave_feat[y][x-1] == solid) || (cave_feat[y][x-1] == feat_wall)) &&
								((cave_feat[y+1][x-1] == solid) || (cave_feat[y+1][x-1] == feat_wall) || (cave_feat[y+1][x-1] == inner)))
						{
							if (rand_int(100) < 75)
							{
								cave_set_feat(y, x, cave_feat[y][x-1]);
								cave_set_feat(y, x - 1, FEAT_WALL_EXTRA);
								if (flag & (MAZE_ROOM)) cave_info[y][x-1] &= ~(CAVE_ROOM | CAVE_GLOW);
							}
						}
						else if ((cave_feat[y][x-1] == inner) && (cave_feat[y+1][x-1] == inner) && (rand_int(100) < 75)) cave_set_feat(y, x, inner);
						break;
					}
					case 3:
					{
						cave_set_feat(y + 1, x + 1, cave_feat[y][x]);
						cave_info[y+1][x+1] |= add_info;

						/* Important - this ensures the top and left hand walls are affected correctly */
						if  ((feat_wall != inner) && ((cave_feat[y-1][x-1] == solid) || (cave_feat[y-1][x-1] == feat_wall)))
						{
							/* Nothing - this is rare enough that I don't want to write an overly complicated test for it */
						}
						else if ((cave_feat[y-1][x] == inner) && (cave_feat[y-1][x+1] == inner) &&
								(cave_feat[y][x-1] == inner) && (cave_feat[y+1][x-1] == inner) && (rand_int(100) < 75)) cave_set_feat(y, x, inner);
						break;
					}
				}
			}
		}
	}

	/* Re-process tree / chasm / glowing /outside grids.  This is required because
	 * of the use of the cave_set_feat auxiliary function above. */
	if (((f_info[feat_path].flags2 & (FF2_GLOW | FF2_CHASM)) != 0) ||
			((f_info[feat_path].flags2 & (FF3_TREE | FF3_OUTSIDE)) != 0))
	{
		for (y = y1 + width_outer; y <= y2 - width_outer; y++)
		{
			for (x = x1 + width_outer; x <= x2 - width_outer; x++)
			{
				int feat = cave_feat[y][x];
				cave_set_feat(y, x, FEAT_FLOOR);
				cave_set_feat(y, x, feat);
			}
		}
	}
#endif

	/* Restore grids */
	if ((flag & (MAZE_SAVE)) != 0)
	{
		int const dx =  1 + x2 - x1;
		int const dy =  1 + y2 - y1;

		for (y = 0; y < dy; y++)
		{
			for (x = 0; x < dx; x++)
			{
				/* Hack -- always overwrite if room flag set and room flag is clear */
				if (((flag & (MAZE_ROOM)) != 0) && ((cave_info[y + y1][x + x1] & (CAVE_ROOM)) == 0))
				{
					cave_set_feat(y + y1, x + x1, saved[y * dx + x]);

					continue;
				}

				/* Hack -- skip floor quickly */
				if (saved[y * dx + x] == FEAT_FLOOR) continue;

				/* Hack -- skip 'extra' walls quickly */
				if (saved[y * dx + x] == FEAT_WALL_EXTRA) continue;

				/* Hack -- skip 'outer' and 'solid' terrain quickly */
				if (f_info[saved[y * dx + x] == FEAT_WALL_EXTRA].flags1 & (FF1_OUTER | FF1_SOLID)) continue;

				/* Passable terrain - overwrite floor */
				if (((f_info[saved[y * dx + x]].flags1 & (FF1_MOVE)) != 0) ||
					((f_info[saved[y * dx + x]].flags3 & (FF3_EASY_CLIMB)) != 0))
				{
					/* Must be placed on floor grid */
					if (cave_feat[y + y1][x + x1] == feat_path)
					{
						cave_set_feat(y + y1, x + x1, saved[y * dx + x]);
					}

					/* Try adjacent saved */
					else for (i = 0; i < 8; i++)
					{
						int yy = y + y1 + ddy_ddd[i];
						int xx = x + x1 + ddx_ddd[i];

						/* Paranoia */
						if (!in_bounds_fully(yy, xx)) continue;

						if (cave_feat[yy][xx] == feat_path)
						{
							cave_set_feat(yy, xx, saved[y * dx + x]);
							break;
						}
					}
				}
				/* Impassable terrain - overwrite walls */
				else
				{
					/* Must be placed on wall grid -- except outer or solid wall */
					if (((f_info[cave_feat[y + y1][x + x1]].flags1 & (FF1_OUTER | FF1_SOLID)) == 0) &&
							((cave_feat[y + y1][x + x1] == feat_wall) ||
							(((f_info[feat_wall].flags1 & (FF1_OUTER)) != 0) && cave_feat[y + y1][x + x1] == feat_state(feat_wall, FS_INNER))))
					{
						cave_set_feat(y + y1, x + x1, saved[y * dx + x]);
					}
					/* Try adjacent saved */
					else for (i = 0; i < 8; i++)
					{
						int yy = y + y1 + ddy_ddd[i];
						int xx = x + x1 + ddx_ddd[i];

						/* Paranoia */
						if (!in_bounds_fully(yy, xx)) continue;

						if (((f_info[cave_feat[yy][xx]].flags1 & (FF1_OUTER | FF1_SOLID)) == 0) &&
							((cave_feat[yy][xx] == feat_wall) ||
							(((f_info[feat_wall].flags1 & (FF1_OUTER)) != 0) && cave_feat[yy][xx] == feat_state(feat_wall, FS_INNER))))
						{
							cave_set_feat(yy, xx, saved[y * dx + x]);
							break;
						}
					}
				}
			}
		}

		/* Free the grids */
		FREE(saved);
	}

	/* Successful */
	return (TRUE);
}


#undef YPOS
#undef XPOS


/*
 * Mark a starburst shape in the dungeon with the CAVE_TEMP flag, given the
 * coordinates of a section of the dungeon in "box" format. -LM-, -DG-
 *
 * Starburst are made in three steps:
 * 1: Choose a box size-dependant number of arcs.  Large starburts need to
 *    look less granular and alter their shape more often, so they need
 *    more arcs.
 * 2: For each of the arcs, calculate the portion of the full circle it
 *    includes, and its maximum effect range (how far in that direction
 *    we can change features in).  This depends on starburst size, shape, and
 *    the maximum effect range of the previous arc.
 * 3: Use the table "get_angle_to_grid" to supply angles to each grid in
 *    the room.  If the distance to that grid is not greater than the
 *    maximum effect range that applies at that angle, change the feature
 *    if appropriate (this depends on feature type).
 *
 * Usage notes:
 * - This function uses a table that cannot handle distances larger than
 *   20, so it calculates a distance conversion factor for larger starbursts.
 * - This function is not good at handling starbursts much longer along one axis
 *   than the other.
 * This function doesn't mark any grid in the perimeter of the given box.
 *
 */
static bool mark_starburst_shape(int y1, int x1, int y2, int x2, u32b flag)
{
	int y0, x0, y, x, ny, nx;
	int i;
	int size;
	int dist, max_dist, dist_conv, dist_check;
	int height, width, arc_dist;
	int degree_first, center_of_arc, degree;

	/* Special variant starburst.  Discovered by accident. */
	bool make_cloverleaf = FALSE;

	/* Holds first degree of arc, maximum effect distance in arc. */
	int arc[45][2];

	/* Number (max 45) of arcs. */
	int arc_num;

	/* Make certain the starburst does not cross the dungeon edge. */
	if ((!in_bounds_fully(y1, x1)) || (!in_bounds_fully(y2, x2))) return (FALSE);

	/* Robustness -- test sanity of input coordinates. */
	if ((y1 + 2 >= y2) || (x1 + 2 >= x2)) return (FALSE);

	/* Get room height and width. */
	height = 1 + y2 - y1;
	width  = 1 + x2 - x1;

	/* Note the "size" */
	size = 2 + (width + height) / 22;

	/* Get a shrinkage ratio for large starbursts, as table is limited. */
	if ((width > 40) || (height > 40))
	{
		if (width > height) dist_conv = 1 + (10 * width  / 40);
		else                dist_conv = 1 + (10 * height / 40);
	}
	else dist_conv = 10;

	/* Make a cloverleaf starburst sometimes.  (discovered by accident) */
	if ((flag & (STAR_BURST_CLOVER)) && (height > 10) && (!rand_int(20)))
	{
		arc_num = 12;
		make_cloverleaf = TRUE;
	}

	/* Usually, we make a normal starburst. */
	else
	{
		/* Ask for a reasonable number of arcs. */
		arc_num = 8 + (height * width / 80);
		arc_num = rand_spread(arc_num, 3);
		if (arc_num < 8) arc_num = 8;
		if (arc_num > 45) arc_num = 45;
	}


	/* Get the center of the starburst. */
	y0 = y1 + height / 2;
	x0 = x1 + width  / 2;

	/* Start out at zero degrees. */
	degree_first = 0;


	/* Determine the start degrees and expansion distance for each arc. */
	for (i = 0; i < arc_num; i++)
	{
		/* Get the first degree for this arc (using 180-degree circles). */
		arc[i][0] = degree_first;

		/* Get a slightly randomized start degree for the next arc. */
		degree_first += 180 / arc_num;

		/* Do not entirely leave the usual range */
		if (degree_first < 180 * (i+1) / arc_num)
		{
			degree_first = 180 * (i+1) / arc_num;
		}
		if (degree_first > (180 + arc_num) * (i+1) / arc_num)
		{
			degree_first = (180 + arc_num) * (i+1) / arc_num;
		}

		/* Get the center of the arc (convert from 180 to 360 circle). */
		center_of_arc = degree_first + arc[i][0];

		/* Get arc distance from the horizontal (0 and 180 degrees) */
		if      (center_of_arc <=  90) arc_dist = center_of_arc;
		else if (center_of_arc >= 270) arc_dist = ABS(center_of_arc - 360);
		else                           arc_dist = ABS(center_of_arc - 180);

		/* Special case -- Handle cloverleafs */
		if ((arc_dist == 45) && (make_cloverleaf)) dist = 0;

		/*
		 * Usual case -- Calculate distance to expand outwards.  Pay more
		 * attention to width near the horizontal, more attention to height
		 * near the vertical.
		 */
		else dist = ((height * arc_dist) + (width * (90 - arc_dist))) / 90;

		/* Randomize distance (should never be greater than radius) */
		arc[i][1] = rand_range(dist / 4, dist / 2);

		/* Keep variability under control (except in special cases). */
		if ((dist != 0) && (i != 0))
		{
			int diff = arc[i][1] - arc[i-1][1];

			if (ABS(diff) > size)
			{
				if (diff > 0)	arc[i][1] = arc[i-1][1] + size;
				else arc[i][1] = arc[i-1][1] - size;
			}
		}
	}

	/* Neaten up final arc of circle by comparing it to the first. */
	{
		int diff = arc[arc_num - 1][1] - arc[0][1];

		if (ABS(diff) > size)
		{
			if (diff > 0)	arc[arc_num - 1][1] = arc[0][1] + size;
			else arc[arc_num - 1][1] = arc[0][1] - size;
		}
	}

	/* Precalculate check distance. */
	dist_check = 21 * dist_conv / 10;

	/* Change grids between (and not including) the edges. */
	for (y = y1 + 1; y < y2; y++)
	{
		for (x = x1 + 1; x < x2; x++)
		{

			/* Get distance to grid. */
			dist = distance(y0, x0, y, x);

			/* Look at the grid if within check distance. */
			if (dist < dist_check)
			{
				/* Convert and reorient grid for table access. */
				ny = 20 + 10 * (y - y0) / dist_conv;
				nx = 20 + 10 * (x - x0) / dist_conv;

				/* Illegal table access is bad. */
				if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))  continue;

				/* Get angle to current grid. */
				degree = get_angle_to_grid[ny][nx];

				/* Scan arcs to find the one that applies here. */
				for (i = arc_num - 1; i >= 0; i--)
				{
					if (arc[i][0] <= degree)
					{
						max_dist = arc[i][1];

						/* Must be within effect range. */
						if (max_dist >= dist)
						{
							/* Mark the grid */
							play_info[y][x] |= (PLAY_TEMP);
						}

						/* Arc found.  End search */
						break;
					}
				}
			}
		}
	}

	return (TRUE);
}


/*
 * Make a starburst room. -LM-, -DG-
 *
 * Usage notes:
 * - This function is not good at handling rooms much longer along one axis
 *   than the other.
 * - It is safe to call this function on areas that might contain vaults or
 *   pits, because "icky" and occupied grids are left untouched.
 */
static bool generate_starburst_room(int y1, int x1, int y2, int x2,
	s16b feat, s16b edge, u32b flag)
{
	int y, x, d;

	/* Paranoia */
	if (!feat) return (FALSE);

	/* Mark the affected grids */
	if (!mark_starburst_shape(y1, x1, y2, x2, flag)) return (FALSE);


	if (cheat_room) message_add("Generating starburst.", MSG_GENERIC);

	/* Paranoia */
	if (edge == feat) edge = FEAT_NONE;

	/* Process marked grids */
	for (y = y1 + 1; y < y2; y++)
	{
		for (x = x1 + 1; x < x2; x++)
		{
			/* Floor overwrites the dungeon */
			if (flag & (STAR_BURST_MOAT))
			{
				/* Non-room grids only for moats */
				if (cave_info[y][x] & (CAVE_ROOM)) continue;

				/* Mark grids next to rooms with terrain if required */
				else if (!(play_info[y][x] & (PLAY_TEMP)))
				{
					int i;

					/* Check adjacent grids for 'rooms' */
					for (i = 0; i < 8; i++)
					{
						int yy,xx;

						yy = y + ddy_ddd[i];
						xx = x + ddx_ddd[i];

						/* Ignore annoying locations */
						if (!in_bounds_fully(yy, xx)) continue;

						/* Mark with temp flag if next to room */
						if (cave_info[yy][xx] & (CAVE_ROOM))
						{
							/* Include in starburst */
							play_info[y][x] |= (PLAY_TEMP);
						}
					}
				}
			}

			/* Marked grids only */
			if (!(play_info[y][x] & (PLAY_TEMP))) continue;

			/* Do not touch "icky" grids. */
			if (room_has_flag(y, x, ROOM_ICKY)) continue;

			/* Do not touch occupied grids. */
			if (cave_m_idx[y][x] != 0) continue;
			if (cave_o_idx[y][x] != 0) continue;

			/* Illuminate if requested */
			if (flag & (STAR_BURST_LIGHT))
			{
				cave_info[y][x] |= (CAVE_GLOW);
			}
			/* Or turn off the lights */
			else
			{
				cave_info[y][x] &= ~(CAVE_GLOW);
			}

			/* Only place on floor grids */
			if ((flag & (STAR_BURST_NEED_FLOOR)) && (cave_feat[y][x] != FEAT_FLOOR))
			{
				/* Do nothing */
			}
			/* Random sometimes places floor */
			else if ((flag & (STAR_BURST_RANDOM)) && (rand_int(100) < 40))
			{
				cave_set_feat(y, x, FEAT_FLOOR);
			}
			/* Floor overwrites the dungeon */
			else if (flag & (STAR_BURST_RAW_FLOOR))
			{
				cave_set_feat(y, x, feat);
			}
			/* Floor is merged with the dungeon */
			else
			{
				int tmp_feat = feat;

				/* Hack -- make variable terrain surrounded by floors */
				if (((f_info[cave_feat[y][x]].flags1 & (FF1_WALL)) != 0) &&
					(variable_terrain(&tmp_feat,feat))) cave_set_feat(y,x,FEAT_FLOOR);

				build_terrain(y, x, tmp_feat);
			}

			/* Make part of a room if requested */
			if (flag & (STAR_BURST_ROOM))
			{
				cave_info[y][x] |= (CAVE_ROOM);
			}

			/* Special case. No edge feature */
			if (edge == FEAT_NONE)
			{
				/*
				 * We lite the outside grids anyway, to
				 * avoid lakes surrounded with blackness.
				 * We only do this if the lake is lit.
				 */
				if ((flag & (STAR_BURST_LIGHT |
					STAR_BURST_ROOM)) == 0) continue;

				/* Look in all directions. */
				for (d = 0; d < 8; d++)
				{
					/* Extract adjacent location */
					int yy = y + ddy_ddd[d];
					int xx = x + ddx_ddd[d];

					/* Ignore annoying locations */
					if (!in_bounds_fully(yy, xx)) continue;

					/* Already processed */
					if (play_info[yy][xx] & (PLAY_TEMP)) continue;

					/* Lite the feature */
					if (flag & (STAR_BURST_LIGHT))
					{
						cave_info[yy][xx] |= (CAVE_GLOW);
					}

					/* Make part of the room */
					if (flag & (STAR_BURST_ROOM))
					{
						cave_info[yy][xx] |= (CAVE_ROOM);
					}
				}

				/* Done */
				continue;
			}

			/* Common case. We have an edge feature */

			/* Look in all directions. */
			for (d = 0; d < 8; d++)
			{
				/* Extract adjacent location */
				int yy = y + ddy_ddd[d];
				int xx = x + ddx_ddd[d];

				/* Ignore annoying locations */
				if (!in_bounds_fully(yy, xx)) continue;

				/* Already processed */
				if (play_info[yy][xx] & (PLAY_TEMP)) continue;

				/* Do not touch "icky" grids. */
				if (room_has_flag(yy, xx, ROOM_ICKY)) continue;

				/* Do not touch occupied grids. */
				if (cave_m_idx[yy][xx] != 0) continue;
				if (cave_o_idx[yy][xx] != 0) continue;

				/* Floor overwrites the dungeon */
				if (flag & (STAR_BURST_MOAT))
				{
					/* Non-room grids only for moats */
					if (cave_info[y][x] & (CAVE_ROOM)) continue;
				}

				/* Illuminate if requested. */
				if (flag & (STAR_BURST_LIGHT))
				{
					cave_info[yy][xx] |= (CAVE_GLOW);
				}

				/* Edge overwrites the dungeon */
				if (flag & (STAR_BURST_RAW_EDGE))
				{
					cave_set_feat(yy, xx, edge);

				}
				/* Edge is merged with the dungeon */
				else
				{
					build_terrain(yy, xx, edge);
				}

				/* Allow light to spill out of rooms through transparent edges */
				if (flag & (STAR_BURST_ROOM))
				{
					cave_info[yy][xx] |= (CAVE_ROOM);

					/* Allow lighting up rooms to work correctly */
					if  (((flag & (STAR_BURST_LIGHT)) != 0) && (f_info[cave_feat[yy][xx]].flags1 & (FF1_LOS)))
					{
						/* Look in all directions. */
						for (d = 0; d < 8; d++)
						{
							/* Extract adjacent location */
							int yyy = yy + ddy_ddd[d];
							int xxx = xx + ddx_ddd[d];

							/* Hack -- light up outside room */
							cave_info[yyy][xxx] |= (CAVE_GLOW);
						}
					}
				}
			}
		}
	}

	/* Clear the mark */
	for (y = y1 + 1; y < y2; y++)
	{
		for (x = x1 + 1; x < x2; x++)
		{
			play_info[y][x] &= ~(PLAY_TEMP);
		}
	}

	/* Success */
	return (TRUE);
}


#define GRID_WALL	0
#define GRID_FLOOR	1
#define GRID_EDGE	2


/*
 * Given a map of floors and walls, replaces walls with edges when adjacent to an existing edge, or an edge of the map.
 * 
 * Note that GRID_EDGE fills all walls which are 'outside' the boundaries of the room, and GRID_WALL all walls inside
 * this boundary.
 */
void edge_room(byte **grid, int size_y, int size_x)
{
	bool edges_finished = FALSE;
	int yi, xi;
	
	/* Surround the outside of the map with edges */
	for(yi=0; yi<size_y; yi++)
	{
		if (grid[yi][0] == GRID_WALL) grid[yi][0] = GRID_EDGE;
		if (grid[yi][size_x-1] == GRID_WALL) grid[yi][size_x-1] = GRID_EDGE;
	}
	for(xi=0; xi<size_x; xi++)
	{
		if (grid[0][xi] == GRID_WALL) grid[0][xi] = GRID_EDGE;
		if (grid[size_y-1][xi] == GRID_WALL) grid[size_y-1][xi] = GRID_EDGE;
	}

	/* Flow all edges to connected walls until none remain next to walls */
	while (!edges_finished)
	{
		edges_finished = TRUE;
		
	 	for(yi=1; yi<size_y - 1; yi++)
	 	for(xi=1; xi<size_x - 1; xi++)
	 	{
	 		if (grid[yi][xi] == GRID_WALL)
	 		{
	 			int d;
	 			
	 			/* Is an edge if adjacent grid is an edge */
	 			for (d = 0; d < 9; d++)
	 			{
 					if (grid[yi+ddy_ddd[d]][xi+ddx_ddd[d]] == GRID_EDGE)
 					{
 						grid[yi][xi] = GRID_EDGE;
 						edges_finished = FALSE;
 						break;
 					}
	 			}
	 		}
	 	}
	}
}


#define REGION 25

/*
 * The following 3 functions (floodfill, floodall, joinall) are creditted to Ray Dillinger.
 * 
 * These can be used to join disconnected regions.
 */ 
void floodfill(byte **map, int size_y, int size_x, int y, int x, int mark,
		int miny[REGION], int minx[REGION])
{ 
	int i; 
	int j; 
	for (i=-1;i<=1;i++) 
		for (j=-1;j<=1;j++) 
			if (i+x < size_x && i+x >= 0 && 
					j+y < size_y && j+y >= 0 && 
					map[j+y][i+x] != 0 && map[j+y][i+x] != mark)
			{ 
				map[j+y][i+x] = mark; 
				/* side effect of floodfilling is recording minimum x and y 
					for each region*/ 
				if (mark < REGION)
				{ 
					if (i+x < minx[mark]) minx[mark] = i+x; 
					if (j+y < miny[mark]) miny[mark] = j+y; 
				} 
				floodfill(map, size_y, size_x, j+y, i+x, mark, miny, minx); 
			}

}

/* find all regions, mark each open cell (by floodfill) with an integer 
	2 or greater indicating what region it's in. */ 
int floodall(byte **map, int size_y, int size_x, int miny[REGION],int minx[REGION])
{ 
	int x; 
	int y; 
	int count = 2; 
	int retval = 0; 
	/* start by setting all floor tiles to 1. */ 
	/* wall spaces are marked 0. */ 
	for (y=0;y< size_y;y++)
	{ 
		for (x=0;x< size_x;x++)
		{ 
			if (map[y][x] != 0)
			{ 
				map[y][x] = 1; 
			} 
		} 
	}
	
	/* reset region extent marks to -1 invalid */ 
	for (x=0;x<REGION;x++)
	{ 
		minx[x] = -1; 
		miny[x] = -1; 
	}
	
	/* now mark regions starting with the number 2. */ 
	for (y=0;y< size_y;y++)
	{ 
		for (x=0;x< size_x;x++)
		{ 
			if (map[y][x] == 1)
			{ 
				if (count < REGION)
				{ 
					minx[count] = x; 
					miny[count] = y; 
				} 
				floodfill(map, size_y, size_x, y, x, count, miny, minx); 
				count++; 
				retval++; 
			} 
		} 
	} 
	/* return the number of floodfill regions found */ 
	return(retval); 

} 



/* joinall is an iterative step toward joining all map regions. 
	The output map is guaranteed to have the same number of 
	open spaces, or fewer, than the input. With enough 
	iterations, an output map having exactly one open space 
	will be produced. Further iterations will just copy that 
	map. 
*/ 
int joinall(byte **mapin, byte **mapout, int size_y, int size_x)
{ 
	int minx[REGION]; 
	int miny[REGION]; 
	int x; 
	int y; 
	int count; 
	int retval; 
	retval = floodall(mapin, size_y, size_x, miny, minx); 
	/* if we have multiple unconnected regions */ 		
	if (retval > 1)
	{ 
		/* check to see if there are still regions that can be moved toward 
			0,0. */ 
		count = 0; 
		for (x = 2; x < REGION; x++) 
			if (minx[x] > 0 || miny[x] > 0) count = 1; 
		/* if no regions can still be moved toward (0,0) */ 
		if (count == 0)
		{ 
			/* the new map is the old map flipped about the diagonal. */ 
			for (y = 0; y < size_y; y++) 
				for (x = 0; x < size_x; x++) 
					mapout[size_y-y-1][size_x-x-1] = mapin[y][x]; 
			return(retval); 
		} 
		else
		{ /* there exist regions that can be moved toward 0,0 */ 
			/* write rock to every square of new map that is either 
				ærock or a region we can move; copy the map to all other squares. */ 
			for (y = 0; y < size_y; y++)
				for (x = 0; x < size_x; x++) 
				{ 
					if (mapin[y][x] >= REGION) mapout[y][x] = mapin[y][x]; 
					else mapout[y][x] = 0; 
				} 
			/* now copy regions we can move to new map, each with a shift 
				toward (0,0).*/ 
			for (y = 0; y < size_y; y++) 
				for (x = 0; x < size_x; x++) 
					if (mapin[y][x] != 0 && mapin[y][x] < REGION)
					{ 
						mapout[y-( miny[mapin[y][x]] > 0)]
									 [x-(minx[mapin[y][x]] > 0)] = 1; 
					} 
			return(retval); 	
		} 
	} 
	else
	{ /* there was only one connected region - the output map is the 
					input map. */ 
		for (y = 0; y < size_y; y++) 
			for (x = 0; x < size_x; x++) 
				if (mapin[y][x] == 0) mapout[y][x] = 0; 
				else mapout[y][x] = 1; 
		return(1); 
	}

}


/*
 * Instead of joining the regions, we remove all but the largest region, and renumber
 * this largest region 
 */
int removeallbutlargest(byte **map, int size_y, int size_x)
{
	int count[REGION];
	int y, x, c = 2;
	int retval;

	retval = floodall(map, size_y, size_x, count, count); 

	/* if we have multiple unconnected regions */ 		
	if (retval > 1)
	{ 
		for (x = 0; x < REGION; x++)
		{
			count[x]=0;
		}
		
		for (y = 0; y < size_y; y++)
			for (x = 0; x < size_x; x++) 
			{ 
				count[map[y][x]]++;
			}
		
		c = 0;
		
		for (x = 0; x < REGION; x++)
		{
			if (count[x] > count[c]) c = x;
		}
	}

	/* Remove all but largest region */
	for (y = 0; y < size_y; y++) 
			for (x = 0; x < size_x; x++) 
				if (map[y][x] == c) map[y][x] = 1; 
				else map[y][x] = 0; 
	
	return(1);
}


/*
 * This builds a cave-like region using cellular automata identical to that outlined in the
 * algorithm at http://roguebasin.roguelikedevelopment.org/index.php?title=Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels
 * 
 * The floor and wall grid define which features are generated.
 * Wall_prob defines the chance of being initialised as a wall grid.
 * 
 * R1 defines the minimum number of walls that must be within 1 grid to make the new grid a wall
 * R2 defines the maximum number of walls that must be within 2 grids to make the new grid a wall
 * else the new grid is a floor
 * gen gives the number of generations. gen2 gives the number of generations from which the r2 parameter
 * is ignored
 * 
 * Examples given in the article are:
 * wall_prob = 45, r1 = 5, r2 = N/A, gen = 5, gen2 = 0
 * wall_prob = 45, r1 = 5, r2 = 0, gen = 5, gen2 = 5
 * wall_prob = 40, r1 = 5, r2 = 2, gen = 7, gen2 = 4
 * 
 * We can define a combination of wall, floor and edge to allow e.g. a series of islands rising out of lava
 * or some other mix of terrain.
 */
static bool generate_cellular_cave(int y1, int x1, int y2, int x2, s16b wall, s16b floor, s16b edge, s16b pool, s16b require, byte cave_flag,
		int wall_prob, int r1, int r2, int gen, int gen2)
{
	int xi, yi;
	
	int size_y = y2 - y1 + 1;
	int size_x = x2 - x1 + 1;
	
	int ii, jj;
	
	int count;
	
	byte **grid  = C_ZNEW(size_y, byte*);
	byte **grid2 = C_ZNEW(size_y, byte*);
	
	/* Allocate space */
	for(yi=0; yi<size_y; yi++)
	{
		grid [yi] = C_ZNEW(size_x, byte);
		grid2[yi] = C_ZNEW(size_x, byte);
	}
	
	/* Initialise the starting grids randomly */
	for(yi=1; yi<size_y-1; yi++)
	for(xi=1; xi<size_x-1; xi++)
		grid[yi][xi] = rand_int(100) < wall_prob ? GRID_WALL : GRID_FLOOR;
	
	/* Initialise the destination grids - for paranoia */
	for(yi=0; yi<size_y; yi++)
	for(xi=0; xi<size_x; xi++)
		grid2[yi][xi] = GRID_WALL;
	
	/* Surround the starting grids in walls */
	for(yi=0; yi<size_y; yi++)
		grid[yi][0] = grid[yi][size_x-1] = GRID_WALL;
	for(xi=0; xi<size_x; xi++)
		grid[0][xi] = grid[size_y-1][xi] = GRID_WALL;
	
	/* Run through generations */
	for(; gen > 0; gen--, gen2--)
	{
		for(yi=1; yi<size_y-1; yi++)
		for(xi=1; xi<size_x-1; xi++)
	 	{
	 		int adjcount_r1 = 0,
	 		    adjcount_r2 = 0;
	 		
	 		for(ii=-1; ii<=1; ii++)
			for(jj=-1; jj<=1; jj++)
	 		{
	 			if(grid[yi+ii][xi+jj] != GRID_FLOOR)
	 				adjcount_r1++;
	 		}
	 		for(ii=yi-2; ii<=yi+2; ii++)
	 		for(jj=xi-2; jj<=xi+2; jj++)
	 		{
	 			if(abs(ii-yi)==2 && abs(jj-xi)==2)
	 				continue;
	 			if(ii<0 || jj<0 || ii>=size_y || jj>=size_x)
	 				continue;
	 			if(grid[ii][jj] != GRID_FLOOR)
	 				adjcount_r2++;
	 		}
	 		if(adjcount_r1 >= r1 || ((gen2 > 0) && (adjcount_r2 <= r2)))
	 			grid2[yi][xi] = GRID_WALL;
	 		else
	 			grid2[yi][xi] = GRID_FLOOR;
	 	}
	 	for(yi=1; yi<size_y-1; yi++)
	 	for(xi=1; xi<size_x-1; xi++)
	 		grid[yi][xi] = grid2[yi][xi];
	}
	
	/* Join all regions */
	do
	{
		joinall(grid,grid2, size_y, size_x);
		count = joinall(grid2,grid, size_y, size_x); 
	} while (count > 1);
	
	/* Edge the room */
	edge_room(grid, size_y, size_x);	
	
	/* Write final grids out to map */
 	for(yi=0; yi<size_y; yi++)
 	for(xi=0; xi<size_x; xi++)
 	{
 		/* Paranoia - if require defined, don't overwrite anything else */
 		if (require && (cave_feat[y1+yi][x1+xi] != require)) continue;
 		
 		switch(grid[yi][xi])
 		{
	 		case GRID_FLOOR:
	 		{
	 			if (floor) cave_set_feat(y1 + yi, x1 + xi, floor);
	 			cave_info[y1+yi][x1+xi] |= (cave_flag);
	 			break;
	 		}
	 		case GRID_WALL:
	 		{
	 			if (wall || pool) cave_set_feat(y1 + yi, x1 + xi, pool ? pool : wall);
	 			cave_info[y1+yi][x1+xi] |= (cave_flag);
	 			break;
	 		}
	 		case GRID_EDGE:
	 		{
	 			int d;
	 			bool edged = FALSE;
	 			byte cave_flag_edge = (edge && f_info[edge].flags1 & (FF1_OUTER)) ? (cave_flag) : ((cave_flag) & ~(CAVE_ROOM));

	 			if (edge || wall)
	 			{
		 			for (d = 0; d < 9; d++)
		 			{
		 				if ((yi + ddy_ddd[d] < 0) || (yi + ddy_ddd[d] >= size_y) || (xi + ddx_ddd[d] < 0) || (xi + ddx_ddd[d] >= size_x)) continue;
		 				
		 				if (grid[yi + ddy_ddd[d]][xi + ddx_ddd[d]] == GRID_FLOOR)
		 				{
							cave_set_feat(y1 + yi, x1 + xi, edge ? edge : wall);
							cave_info[y1+yi][x1 + xi] |= (cave_flag_edge);
							edged = TRUE;
		 					break;
		 				}
		 			}
	 			}
	 			
				break;
	 		}
 		}
 	}

 	/* Free memory */
	for(yi=0; yi<size_y; yi++)
	{
		FREE(grid[yi]);
		FREE(grid2[yi]);
	}

	FREE(grid);
	FREE(grid2);
	
	/* XXX Check connectivity */
	/* The big problem here is deciding which is the largest
	 * part of the cellular cave. So we just allow disconnected
	 * areas for the moment. */
	
	return (TRUE);
}


/*
 * The following function is from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html#The%20C%20Code
 * 
 * Copyright (c) 1970-2003, Wm. Randolph Franklin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files 
 * (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
 * 2. Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided 
 * with the distribution.
 * 3. The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific 
 * prior written permission. 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */
int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
{
	int i, j, c = 0;
	
	for (i = 0, j = nvert-1; i < nvert; j = i++) {
		if ( ((verty[i]>testy) != (verty[j]>testy)) &&
				(testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
				c = !c;
	}
	return c;
}




/* It is possible to generate poly rooms with very small sizes. We ensure that these rooms have
 * a minimum size */
#define MIN_POLY_ROOM_SIZE	17 + (p_ptr->depth / 6)

/*
 * Generates a room from a convex or concave polygon. We don't completely handle the complex (self-intersecting) case
 * but it could be extended to do so. Most of the smarts are on pnpoly (above).
 * 
 * *y and *x represent pairs of vertex coordinates. These should probably be ordered but may not require this.
 */
static bool generate_poly_room(int n, int *y, int *x, s16b edge, s16b floor, s16b wall, s16b requires, byte cave_flag)
{
	float *verty, *vertx;
	int i, xi, yi, size_y, size_x;
	int y0 = 256;
	int x0 = 256;
	int y1 = 0;
	int x1 = 0;
	int floors = 0;
	
	byte **grid;

	byte cave_flag_edge = (edge && f_info[edge].flags1 & (FF1_OUTER)) ? (cave_flag) : ((cave_flag) & ~(CAVE_ROOM));
	
	/* Allocate spaces for floating vertices */
	verty = C_ZNEW(n+1,float);
	vertx = C_ZNEW(n+1,float);
	
	/* Copy contents to floating array */
	for (i = 0; i < n; i++)
	{
		verty[i] = (float)y[i];
		vertx[i] = (float)x[i];
		
		/* Determine extents */
		if (y[i] > y1) y1 = y[i];
		if (y[i] < y0) y0 = y[i];
		if (x[i] > x1) x1 = x[i];
		if (x[i] < x0) x0 = x[i];
	}
	
	/* Safety - repeat initial vertex at end of polygon */
	if ((y[n-1] != y[0]) || (x[n-1] != x[0]))
	{
		verty[n] = (float)y[0];
		vertx[n] = (float)x[0];
		n++;
	}
	
	/* Allocate space for the region borders */
	y0--; x0--; y1++; x1++;

	/* Get grid size */
	size_y = y1 - y0 + 1;
	size_x = x1 - x0 + 1;
	
	/* Allocate space for room */
	grid  = C_ZNEW(size_y, byte*);
		
	/* Allocate space */
	for(yi=0; yi<size_y; yi++)
	{
		grid[yi] = C_ZNEW(size_x, byte);
	}
	
	/* Draw the polygon */
	for (yi = 0; yi < size_y; yi++)
	{
		for (xi = 0; xi < size_x; xi++)
		{
			/* Point in the polygon */
			if (pnpoly(n, verty, vertx, (float)(yi + y0), (float)(xi + x0)))
			{
				grid[yi][xi] = GRID_FLOOR;
				floors++;
			}
			else
			{
				grid[yi][xi] = GRID_WALL;
			}
		}
	}
	
	/* Ensure minimum size */
	if (floors < MIN_POLY_ROOM_SIZE) return (FALSE);
	
	/* Remove all but largest region */
	removeallbutlargest(grid, size_y, size_x);
	
	/* Compute edge of room */
	edge_room(grid, size_y, size_x);
	
	/* Write final grids out to map */
 	for(yi=0; yi<size_y; yi++)
 	for(xi=0; xi<size_x; xi++)
 	{
 		/* Paranoia - if require defined, don't overwrite anything else */
 		if (requires && (cave_feat[y0+yi][x0+xi] != requires)) continue;
 		
 		switch(grid[yi][xi])
 		{
	 		case GRID_FLOOR:
	 		{
	 			if (floor) cave_set_feat(y0 + yi, x0 + xi, floor);
	 			cave_info[y0+yi][x0+xi] |= (cave_flag);

	 			break;
	 		}
	 		case GRID_WALL:
	 		{
	 			if (wall) cave_set_feat(y0 + yi, x0 + xi, wall);
	 			cave_info[y0+yi][x0+xi] |= (cave_flag);
	 			break;
	 		}
	 		case GRID_EDGE:
	 		{
	 			int d;
	 			bool edged = FALSE;

	 			if (edge)
	 			{
		 			for (d = 0; d < 9; d++)
		 			{
		 				if ((yi + ddy_ddd[d] < 0) || (yi + ddy_ddd[d] >= size_y) || (xi + ddx_ddd[d] < 0) || (xi + ddx_ddd[d] >= size_x)) continue;
		 				
		 				if (grid[yi + ddy_ddd[d]][xi + ddx_ddd[d]] == GRID_FLOOR)
		 				{
							cave_set_feat(y0 + yi, x0 + xi, edge);
							cave_info[y0+yi][x0 + xi] |= (cave_flag_edge);
							edged = TRUE;
		 					break;
		 				}
		 			}
	 			}
	 			
				break;
	 		}
 		}
 	}
	
 	/* Free memory */
	for(yi=0; yi<size_y; yi++)
	{
		FREE(grid[yi]);
	}

	FREE(grid);
	FREE(verty);
	FREE(vertx);
	
	return (TRUE);
}








#define	BURROW_TEMP	1	/* Limit burrows building to temporary flags */
#define BURROW_CARD	2	/* Cardinal directions NSEW only */

/*
 * Builds a 'burrow' - that is the organic looking patterns
 * discovered by Kusigrosz and documented in this thread
 * http://groups.google.com/group/rec.games.roguelike.development/browse_thread/thread/4c56271970c253bf
 */

/* Arguments:
 *	 ngb_min, ngb_max: the minimum and maximum number of neighbouring
 *		 floor cells that a wall cell must have to become a floor cell.
 *		 1 <= ngb_min <= 3; ngb_min <= ngb_max <= 8;
 *	 connchance: the chance (in percent) that a new connection is
 *		 allowed; for ngb_max == 1 this has no effect as any
 *		 connecting cell must have 2 neighbours anyway.
 *	 cellnum: the maximum number of floor cells that will be generated.
 * The default values of the arguments are defined below.
 *
 * Algorithm description:
 * The algorithm operates on a rectangular grid. Each cell can be 'wall'
 * or 'floor'. A (non-border) cell has 8 neigbours - diagonals count. 
 * There is also a cell store with two operations: store a given cell on
 * top, and pull a cell from the store. The cell to be pulled is selected
 * randomly from the store if N_cells_in_store < 125, and from the top
 * 25 * cube_root(N_cells_in_store) otherwise. There is no check for
 * repetitions, so a given cell can be stored multiple times.
 *
 * The algorithm starts with most of the map filled with 'wall', with a
 * "seed" of some floor cells; their neigbouring wall cells are in store.
 * The main loop in delveon() is repeated until the desired number of 
 * floor cells is achieved, or there is nothing in store:
 *	 1) Get a cell from the store;
 *	 Check the conditions: 
 *	 a) the cell has between ngb_min and ngb_max floor neighbours, 
 *	 b) making it a floor cell won't open new connections,
 *		 or the RNG allows it with connchance/100 chance.
 *	 if a) and b) are met, the cell becomes floor, and its wall
 *	 neighbours are put in store in random order.
 * There are many variants possible, for example:
 * 1) picking the cell in rndpull() always from the whole store makes
 *	 compact patterns;
 * 2) storing the neighbours in digcell() clockwise starting from
 *	 a random one, and picking the bottom cell in rndpull() creates
 *	 meandering or spiral patterns.
 */
#define TRN_XSIZE 400
#define TRN_YSIZE 400

struct cellstore
{
	int* xs;
	int* ys;
	int index;
	int size;
};

/* Globals: */
int Xoff[8] = {1,  1,  0, -1, -1, -1,  0,  1};
int Yoff[8] = {0,  1,  1,  1,  0, -1, -1, -1};

/* Functions */
void rnd_perm(int *tab, int nelem)
{
	int i;
	int rind;
	int tmp;

	assert(tab && (nelem >= 0));
	

	for (i = 0; i < nelem; i++)
	{
		rind = rand_int(i + 1);
		
		tmp = tab[rind];
		tab[rind] = tab[i];
		tab[i] = tmp;
	}
}

struct cellstore mkstore(int size)
{
	struct cellstore ret;
	
	assert(size > 0);

	ret.xs = C_ZNEW(size, int);
	assert(ret.xs);
	ret.ys = C_ZNEW(size, int);
	assert(ret.ys);
	
	ret.size = size;
	ret.index = 0;

	return (ret);
}

void delstore(struct cellstore* cstore)
{
	assert(cstore);

	FREE(cstore->xs);
	cstore->xs = 0;

	FREE(cstore->ys);
	cstore->ys = 0;

	cstore->size = 0;
	cstore->index = 0;
}

int storecell(struct cellstore* cstore, int y, int x)
{
	int rind;

	assert(cstore);
	
	if (cstore->index < cstore->size)
	{
		cstore->xs[cstore->index] = x;
		cstore->ys[cstore->index] = y;
		(cstore->index)++;
		return (1); /* new cell stored */
	}
	else /* Replace another cell. Should not happen if lossless storage */
	{
		rind = rand_int(cstore->index);
		cstore->xs[rind] = x;
		cstore->ys[rind] = y;
		return (0); /* old cell gone, new cell stored */
	}
}

/* Remove a cell from the store and put its coords into x, y.
 * Note that pulling any cell except the topmost puts the topmost one in
 * its place. 
 */
int rndpull(struct cellstore* cstore, int* y, int* x, bool compact)
{
	int rind;

	assert(cstore && x && y);
	if (cstore->index <= 0)
	{
		return(0); /* no cells */
	}

	/* compact patterns */
	if (compact)
	{
		rind = rand_int(cstore->index);
	}
	else
	{
		/* fluffy patterns */
		rind = (cstore->index < 125) ?
			rand_int(cstore->index) :
			cstore->index - rand_int(25 * uti_icbrt(cstore->index)) - 1;
	}

	*x = cstore->xs[rind];
	*y = cstore->ys[rind];
	
	if (cstore->index - 1 != rind) /* not the topmost cell - overwrite */
	{
		cstore->xs[rind] = cstore->xs[cstore->index - 1];
		cstore->ys[rind] = cstore->ys[cstore->index - 1];
	}

	cstore->index -= 1;

	return(1);
}

/* Count neighbours of the given cells that contain terrain feat.
 */
int ngbcount(int y, int x, s16b feat)
{
	int i;
	int count = 0;
	
	for (i = 0; i < 8; i++)
	{
		if (in_bounds_fully(y + Yoff[i],x + Xoff[i]) && 
			(cave_feat[y + Yoff[i]][x + Xoff[i]] == feat))
		{
			count++;
		}
	}
	return (count);
}

/* Number of groups of '1's in the 8 neighbours around a central cell.
 * The encoding is binary, lsb is to the right, then clockwise.
 */
const int ngb_grouptab[256] = 
{
/**********  0  1  2  3  4  5  6  7  8  9  */
/* 000 */	0, 1, 1, 1, 1, 1, 1, 1, 1, 2,
/* 010 */	2, 2, 1, 1, 1, 1, 1, 2, 2, 2,
/* 020 */	1, 1, 1, 1, 1, 2, 2, 2, 1, 1,
/* 030 */	1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
/* 040 */	2, 3, 3, 3, 2, 2, 2, 2, 1, 2,
/* 050 */	2, 2, 1, 1, 1, 1, 1, 2, 2, 2,
/* 060 */	1, 1, 1, 1, 1, 1, 2, 1, 2, 1,
/* 070 */	2, 1, 2, 2, 3, 2, 2, 1, 2, 1,
/* 080 */	1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
/* 090 */	2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
/* 000 */	2, 1, 2, 1, 2, 2, 3, 2, 2, 1,
/* 110 */	2, 1, 1, 1, 2, 1, 1, 1, 1, 1,
/* 120 */	1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
/* 130 */	2, 1, 2, 1, 2, 1, 2, 2, 3, 2,
/* 140 */	2, 1, 2, 1, 2, 2, 3, 2, 2, 1,
/* 150 */	2, 1, 2, 2, 3, 2, 2, 1, 2, 1,
/* 160 */	2, 2, 3, 2, 3, 2, 3, 2, 3, 3,
/* 170 */	4, 3, 3, 2, 3, 2, 2, 2, 3, 2,
/* 180 */	2, 1, 2, 1, 2, 2, 3, 2, 2, 1,
/* 190 */	2, 1, 1, 1, 2, 1, 2, 1, 2, 1,
/* 200 */	2, 2, 3, 2, 2, 1, 2, 1, 1, 1,
/* 210 */	2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
/* 220 */	1, 1, 1, 1, 1, 1, 2, 1, 2, 1,
/* 230 */	2, 1, 2, 2, 3, 2, 2, 1, 2, 1,
/* 240 */	1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
/* 250 */	2, 1, 1, 1, 1, 1
};

/* Examine the 8 neigbours of the given cell, and count the number
 * of separate groups of terrain cells. A groups contains cells that are
 * of the same type (feat) and are adjacent (diagonally, too!)
 */
int ngbgroups(int y, int x, s16b feat)
{
	int bitmap = 0; /* lowest bit is the cell to the right, then clockwise */
	int i;
	
	for (i = 0; i < 8; i++)
	{
		bitmap >>= 1;

		if (in_bounds_fully(y + Yoff[i], x + Xoff[i]) &&
			cave_feat[y + Yoff[i]][x + Xoff[i]] == feat)
		{
			bitmap |= 0x80;
		}
	}

	return (ngb_grouptab[bitmap]);
}

/* Dig out an available cell to floor and store its available neighbours in
 * random order.
 */
int digcell(struct cellstore* cstore,
	int y, int x, s16b floor, s16b available, byte cave_flag, byte burrows_flag)
{
	int order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	int i, j;

	assert(cstore);

	if ((!in_bounds_fully(y, x)) || ((available) && (cave_feat[y][x] != available)) ||
			(((burrows_flag & (BURROW_TEMP)) != 0) && ((play_info[y][x] & (PLAY_TEMP)) == 0)))
	{
		return (0); /* did nothing */
	}

	/* Dig the cell */
	cave_alter_feat(y, x, FS_TUNNEL);
	
	/* Fill it with terrain */
	build_terrain(y, x, floor);
	
	/* Update the room flags */
	cave_info[y][x] |= (cave_flag);
	
	/* Clear the temp flag */
	if ((burrows_flag & (BURROW_TEMP)) != 0) play_info[y][x] &= ~(PLAY_TEMP);

	rnd_perm(order, 8);

	for (i = 0; i < 8; i += ((burrows_flag & (BURROW_CARD)) != 0) ? 2 : 1) 
	{
		j = order[i];
		if (in_bounds_fully(y + Yoff[j], x + Xoff[j]) &&
			((!available) || (cave_feat[y + Yoff[j]][x + Xoff[j]] == available)) &&
			(((burrows_flag & (BURROW_TEMP)) == 0) ||
					((play_info[y + Yoff[j]][x + Xoff[j]] & (PLAY_TEMP)) != 0)))
		{
			storecell(cstore, y + Yoff[j], x + Xoff[j]);
		}
		cave_info[y + Yoff[j]][x + Xoff[j]] |= (cave_flag);
	}

	return (1); /* dug 1 cell */
}

/* Continue digging until cellnum or no more cells in store. Digging is
 * allowed if the terrain in the cell is 'ava'ilable, cell has from
 * ngb_min to ngb_max flo neighbours, and digging won't open new
 * connections; the last condition is ignored with percent chance
 * connchance.
 */
int delveon(struct cellstore* cstore,
	int ngb_min, int ngb_max, int connchance, int cellnum,
	s16b floor, s16b available, bool compact, byte cave_flag, byte burrows_flag)
{
	int count = 0;
	int ngb_count;
	int ngb_groups;
	int x, y;

	assert(cstore);
	assert((cellnum >= 0)/* && (cellnum < mpc.xsize * mpc.ysize)*/);
	assert((connchance >= 0) && (connchance <= 100));
	assert((ngb_min >= 0) && (ngb_min <= 3) && 
		(ngb_min <= ngb_max) && (ngb_max <= 8));
	assert(floor != available);

	while ((count < cellnum) && rndpull(cstore, &y, &x, compact))
	{
		ngb_count = ngbcount(y, x, floor);
		ngb_groups = ngbgroups(y, x, floor);

		if ( in_bounds_fully(y, x) && ((!available) || (cave_feat[y][x] == available)) &&
			(((burrows_flag & (BURROW_TEMP)) == 0) || ((play_info[y][x] & (PLAY_TEMP)) != 0)) &&
			(ngb_count >= ngb_min) && (ngb_count <= ngb_max) && 
			((ngb_groups <= 1) || (rand_int(100) < connchance)) )
		{
				count += digcell(cstore, y, x, floor, available, cave_flag, burrows_flag);
		}
	}
	
	return (count);
}

/* Estimate a sensible number of cells for given ngb_min, ngb_max.
 */
int cellnum_est(int totalcells, int ngb_min, int ngb_max)
{
	int denom[12] = {8, 8, 8, 7, 6, 5, 5, 4, 4, 4, 3, 3};
	/* two first entries are not used */

	assert(totalcells > 0);
	assert((ngb_min + ngb_max >= 2) && (ngb_min + ngb_max < 12));
	
	return (totalcells / denom[ngb_min + ngb_max]);
}


/*
 * Generate a random 'burrow-like' cavern of cellnum cells.
 */
bool generate_burrows(int y1, int x1, int y2, int x2,
	int ngb_min, int ngb_max, int connchance, int cellnum, 
	s16b floor, s16b available, bool compact, byte cave_flag, byte burrows_flag)
{
	struct cellstore cstore;
	int count = 0;
	int ngb_count;
	int ngb_groups;
	int x, y;
	
	int xorig = (x1 + x2)/2;
	int yorig = (y1 + y2)/2;
	
	int xsize = x2 - x1 + 1;
	int ysize = y2 - y1 + 1;

	assert((cellnum >= 0) && (cellnum < xsize * ysize));
	assert((connchance >= 0) && (connchance <= 100));
	assert((ngb_min >= 0) && (ngb_min <= 3) && 
		(ngb_min <= ngb_max) && (ngb_max <= 8));
	assert(floor != available);

	cstore = mkstore(8 * xsize * ysize); /* overkill */
	storecell(&cstore, yorig, xorig);

	while ((count < 2 * ngb_min) && (count < cellnum) && 
		rndpull(&cstore, &y, &x, compact))
	{
		ngb_count = ngbcount(y, x, floor);
		ngb_groups = ngbgroups(y, x, floor);

		/* stay close to origin, ignore ngb_min */
		if ( in_bounds_fully(y, x) && ((!available) || (cave_feat[y][x] == available)) &&
			(((burrows_flag & (BURROW_TEMP)) == 0) || ((play_info[y][x] & (PLAY_TEMP)) != 0)) &&
			(abs(x - xorig) < 2) && (abs(y - yorig) < 2) &&
			(ngb_count <= ngb_max) && 
			((ngb_groups <= 1) || (rand_int(100) < connchance)) )
		{
			count += digcell(&cstore, y, x, floor, available, cave_flag, burrows_flag);
		}
	}

	if (count < cellnum)
		{
		count += delveon(&cstore, ngb_min, ngb_max, connchance, 
			cellnum - count, floor, available, compact, cave_flag, burrows_flag);
		}

	delstore(&cstore);
	return (count > 0);
}



/*
 * Number to place for scattering
 */
#define NUM_SCATTER   7


/*
 * Generate helper -- draw a rectangle with a feature using a series of 'pattern' flags.
 */
static void generate_patt(int y1, int x1, int y2, int x2, s16b feat, u32b flag, u32b exclude, int dy, int dx, int scale, int scatter)
{
	int y, x, i, k;

	int y_alloc = 0, x_alloc = 0, choice;

	int offset = rand_int(100) < 50 ? 1 : 0;
	int max_offset = offset + 1;

	bool outer;
	bool use_edge;

	s16b edge = f_info[feat].edge;

	/* Ignore edge terrain - use central terrain */
	if ((flag & (RG1_IGNORE_EDGE)) != 0)
	{
		edge = feat;
	}
	/* Bridge or tunnel edges instead */
	else if ((flag & (RG1_BRIDGE_EDGE)) != 0)
	{
		if (f_info[feat].flags2 & (FF2_PATH))
		{
			/* Bridge previous contents */
			edge = feat_state(feat, FS_BRIDGE);
		}
		/* Apply tunnel */
		else if (f_info[feat].flags1 & (FF1_TUNNEL))
		{
			/* Tunnel previous contents */
			edge = feat_state(feat, FS_TUNNEL);
		}
	}
	/* Paranoia */
	if (!dy || !dx) return;

	/* Pick features if needed */
	if ((feat) && (f_info[feat].mimic == feat))
	{
		/* Set feature hook */
		room_info_feat_mimic = feat;

		get_feat_num_hook = room_info_feat;

		/* Prepare allocation table */
		get_feat_num_prep();
	}

	/* Draw maze if required -- ensure minimum size of 5 in both directions (==> y2 - y1 >= 4), or 3 if maze decor (which has no outer walls) */
	if (((flag & (RG1_MAZE_PATH | RG1_MAZE_WALL | RG1_MAZE_DECOR)) != 0) && (ABS(y2 - y1) >= ((flag & (RG1_MAZE_DECOR)) ? 2 : 4))
			&& (ABS(x2 - x1) >= ((flag & (RG1_MAZE_DECOR)) ? 2 : 4)) && ((flag & (RG1_ALLOC)) == 0) && (feat))
	{
		/* Maze dimensions */
		/* Note: Ensure the correct ordering and that size is odd in both directions (==> y2 - y1 is even) */
		int y0 = (dy > 0) ? y1 : (y2 + ((y1 - y2) % 2 ? 1 : 0));
		int x0 = (dx > 0) ? x1 : (x2 + ((x1 - x2) % 2 ? 1 : 0));
		int y3 = (dy > 0) ? (y2 - ((y2 - y1) % 2 ? 1 : 0)) : y1;
		int x3 = (dx > 0) ? (x2 - ((x2 - x1) % 2 ? 1 : 0)) : x1;

		int wall = ((flag & (RG1_MAZE_WALL)) != 0) ? feat : (((flag & (RG1_MAZE_DECOR)) != 0) ? (edge && (edge != feat) ? edge : FEAT_FLOOR) : FEAT_WALL_OUTER);
		int path = ((flag & (RG1_MAZE_PATH | RG1_MAZE_DECOR)) != 0) ? feat : (edge && (edge != feat) ? edge : FEAT_FLOOR);
		int pool = 0;

		u32b maze_flags = MAZE_SAVE | MAZE_ROOM | ((flag & (RG1_MAZE_DECOR)) ? 0 : MAZE_WALL);

		/* Ensure light */
		if ((exclude & (RG1_DARK)) != 0) maze_flags |= MAZE_LITE;

		/* Need an exit? */
		if ((flag & (RG1_MAZE_DECOR)) == 0)
		{
			u32b temp_flags;

			do
			{
				temp_flags = 0L;

				/* Hack -- extend maze north & south if either side is directly next to outer wall */
				for (x = x0 + 1; x < x3; x++)
				{
					if (((maze_flags & (MAZE_OUTER_N)) == 0) && (y0 > 0) && ((f_info[cave_feat[y0-1][x]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)) temp_flags |= MAZE_OUTER_N;
					if (((maze_flags & (MAZE_OUTER_S)) == 0) && (y3 < DUNGEON_HGT-1) && ((f_info[cave_feat[y3+1][x]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)) temp_flags |= MAZE_OUTER_S;
				}

				/* Extend north and south */
				if (temp_flags)
				{
					if(temp_flags & MAZE_OUTER_N) y0--;
					if(temp_flags & MAZE_OUTER_S) y3++;

					/* Add extra flags */
					maze_flags |= temp_flags;
				}
			} while (temp_flags);

			do
			{
				temp_flags = 0L;

				/* Hack -- extend maze east & west if either side is directly next to outer wall */
				for (y = y0 + 1; y < y3; y++)
				{
					if (((maze_flags & (MAZE_OUTER_W)) == 0) && (x0 > 0) && ((f_info[cave_feat[y][x0-1]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)) temp_flags |= MAZE_OUTER_W;
					if (((maze_flags & (MAZE_OUTER_E)) == 0) && (x3 < DUNGEON_WID-1) && ((f_info[cave_feat[y][x3+1]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)) temp_flags |= MAZE_OUTER_E;
				}

				/* Extend east and west */
				if (temp_flags)
				{
					if(temp_flags & MAZE_OUTER_W) x0--;
					if(temp_flags & MAZE_OUTER_E) x3++;

					/* Add extra flags */
					maze_flags |= temp_flags;
				}
			} while (temp_flags);

			/* Outer wall north/south */
			for (x = x0 + 1; x < x3; x++)
			{
				if ((f_info[cave_feat[y0][x]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0) maze_flags |= MAZE_OUTER_N;
				if ((f_info[cave_feat[y3][x]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0) maze_flags |= MAZE_OUTER_S;
			}

			/* Outer wall east/west */
			for (y = y0 + 1; y < y3; y++)
			{
				if ((f_info[cave_feat[y][x0]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0) maze_flags |= MAZE_OUTER_W;
				if ((f_info[cave_feat[y][x3]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0) maze_flags |= MAZE_OUTER_E;
			}

			/* Two exits required? */
			if (((maze_flags & (MAZE_OUTER_N)) != 0) && ((maze_flags & (MAZE_OUTER_S)) != 0) && ((maze_flags & (MAZE_OUTER_E | MAZE_OUTER_W)) == 0))
			{
				maze_flags |= (MAZE_EXIT_W | MAZE_EXIT_E);
			}
			else if (((maze_flags & (MAZE_OUTER_W)) != 0) && ((maze_flags & (MAZE_OUTER_E)) != 0) && ((maze_flags & (MAZE_OUTER_N | MAZE_OUTER_S)) == 0))
			{
				maze_flags |= (MAZE_EXIT_N | MAZE_EXIT_S);
			}
			/* Zero or one exits required */
			else
			{
				u32b maze_exits = 0L;
				int k = 0;

				if (((maze_flags & (MAZE_OUTER_N)) == 0) && !(rand_int(++k))) maze_exits = MAZE_EXIT_N;
				if (((maze_flags & (MAZE_OUTER_S)) == 0) && !(rand_int(++k))) maze_exits = MAZE_EXIT_S;
				if (((maze_flags & (MAZE_OUTER_W)) == 0) && !(rand_int(++k))) maze_exits = MAZE_EXIT_W;
				if (((maze_flags & (MAZE_OUTER_E)) == 0) && !(rand_int(++k))) maze_exits = MAZE_EXIT_E;

				maze_flags |= maze_exits;
			}
		}

		/* Paranoia */
		if (wall == path) path = FEAT_FLOOR;

		/* Hack -- swap path and wall if path does not allow movement */
		if (((f_info[path].flags1 & (FF1_MOVE)) == 0) && ((f_info[path].flags3 & (FF3_EASY_CLIMB)) == 0))
		{
			int temp = wall;
			wall = path;
			path = temp;

			/* Paranoia */
			if (((f_info[path].flags1 & (FF1_MOVE)) == 0) && ((f_info[path].flags3 & (FF3_EASY_CLIMB)) == 0))
			{
				pool = path;
				path = FEAT_FLOOR;

				if (pool) maze_flags |= (MAZE_POOL);
			}
		}

		/* Draw the maze. */
		draw_maze(y0, x0, y3, x3, wall, path, 1, scale, pool, maze_flags);

		/* Hack -- scatter items inside maze */
		flag |= (RG1_SCATTER);
		feat = 0;
		edge = 0;
	}

	/* Place starburst if required */
	else if (((flag & (RG1_STARBURST)) != 0) && ((flag & (RG1_ALLOC)) == 0) &&
			(ABS(y2 - y1) >= 4) && (ABS(y2 - y1) >= 4))
	{
		/* Ensure the correct ordering of directions */
		int y0 = (dy > 0) ? y1 : y2;
		int x0 = (dx > 0) ? x1 : x2;
		int y3 = (dy > 0) ? y2 : y1;
		int x3 = (dx > 0) ? x2 : x1;

		u32b star_flags = STAR_BURST_ROOM | STAR_BURST_NEED_FLOOR;

		/* Ensure light */
		if ((exclude & (RG1_DARK)) != 0) star_flags |= STAR_BURST_LIGHT;

		/* Ensure random floor space */
		if (flag & (RG1_RANDOM | RG1_SCATTER | RG1_CHECKER)) star_flags |= STAR_BURST_RANDOM;

		/* Create the starburst */
		if (feat) generate_starburst_room(y0, x0, y3, x3, feat, edge, star_flags);

		/* Hack -- scatter items around the starburst */
		flag |= (RG1_SCATTER);
		feat = 0;
		edge = 0;
	}

	/* Use checkered for invalid mazes / starbursts */
	else if ((flag & (RG1_MAZE_PATH | RG1_MAZE_WALL | RG1_MAZE_DECOR | RG1_STARBURST)) != 0)
	{
		flag |= (RG1_CHECKER);
	}

	/* Scatter several about if requested */
	for (k = 0; k < ( ((flag & (RG1_SCATTER | RG1_TRAIL)) != 0) && ((flag & (RG1_ALLOC)) == 0) ? scatter : 1); k++)
	{
		/* Pick location */
		choice = 0;

		/* Scan the whole room */
		for (y = y1; (dy > 0) ? y <= y2 : y >= y2; y += dy)
		{
			for (x = x1; (dx > 0) ? x <= x2 : x >= x2; x += dx)
			{
				outer = ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) != 0);
				use_edge = FALSE;

				/* Checkered room */
				if (((flag & (RG1_CHECKER)) != 0) && ((x + y + offset) % 2)) continue;

				/* Only place on outer/solid walls */
				if (((flag & (RG1_OUTER)) != 0) && (cave_feat[y][x] != FEAT_WALL_OUTER) && (cave_feat[y][x] != FEAT_WALL_SOLID)) continue;

				/* Only place on floor otherwise */
				if (((flag & (RG1_OUTER)) == 0) && (cave_feat[y][x] != FEAT_FLOOR))
				{
					if (((flag & (RG1_CHECKER)) != 0) && (offset < max_offset)) offset++;

					continue;
				}

				/* Clear max_offset */
				max_offset = 0;

				/* Only place on edge of room if edge flag and not centre flag set */
				if (((flag & (RG1_EDGE)) != 0) && ((flag & (RG1_CENTRE)) == 0) && (cave_feat[y][x] != FEAT_WALL_OUTER))
				{
					for (i = 0; i < 8; i++)
					{
						if ((f_info[cave_feat[y + ddy_ddd[i]][x + ddx_ddd[i]]].flags1 & (FF1_OUTER)) != 0) use_edge = TRUE;
					}

					if (!use_edge) continue;
				}
				/* Don't place on edge of room if centre and not edge flag are set */
				else if (((flag & (RG1_CENTRE)) != 0) && ((flag & (RG1_EDGE)) == 0) && (cave_feat[y][x] != FEAT_WALL_OUTER))
				{
					bool accept = TRUE;

					for (i = 0; i < 4; i++)
					{
						if ((f_info[cave_feat[y + ddy_ddd[i]][x + ddx_ddd[i]]].flags1 & (FF1_OUTER)) != 0) accept = FALSE;
					}

					if (!accept) continue;

					use_edge = TRUE;
				}

				/* Leave inner area open */
				if ((flag & (RG1_INNER)) != 0)
				{
					if (((dy > 0) ? (y > y1) && (y + dy <= y2) : (y + dy >= y2) && (y < y1)) &&
						((dx > 0) ? (x > x1) && (x + dx <= x2) : (x + dx >= x2) && (x < x1))) continue;
				}

				/* Random */
				if (((flag & (RG1_RANDOM)) != 0) && (rand_int(100) < 40)) continue;

				/* Only place next to last choice */
				if ((flag & (RG1_TRAIL)) != 0)
				{
					/* Place next to previous position */
					if ((k == 0) || (distance(y, y_alloc, x, x_alloc) < ABS(dy) + ABS(dx)))
					{
						if (rand_int(++choice) == 0)
						{
							y_alloc = y;
							x_alloc = x;
						}
					}
				}
				/* Maybe pick if placing one */
				else if ((flag & (RG1_ALLOC | RG1_SCATTER | RG1_TRAIL | RG1_8WAY)) != 0)
				{
					if (rand_int(++choice) == 0)
					{
						y_alloc = y;
						x_alloc = x;
					}
				}

				/* Set feature */
				else
				{
					int place_feat = feat;

					/* Use edge instead */
					if ((use_edge) && (edge)) place_feat = edge;

					/* Hack -- in case we don't place enough */
					if ((flag & (RG1_RANDOM)) != 0)
					{
						if (rand_int(++choice) == 0)
						{
							y_alloc = y;
							x_alloc = x;
						}
					}

					/* Pick a random feature? */
					if ((feat) && (f_info[feat].mimic == feat))
					{
						place_feat = get_feat_num(object_level);

						if (!place_feat) place_feat = feat;
					}

					/* Assign feature */
					if (place_feat)
					{
						/* Preserve the 'solid' status of a wall & ensure that at least 1 feature is placed */
						if (cave_feat[y][x] == FEAT_WALL_SOLID)
						{
							/* Place solid wall now */
							if ((f_info[place_feat].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)
							{
								cave_set_feat(y, x, (f_info[place_feat].flags1 & (FF1_OUTER)) != 0 ? feat_state(place_feat, FS_SOLID) : place_feat);
							}
							else if (dun->next_n < NEXT_MAX)
							{
								/* Overwrite solid wall later */
								dun->next[dun->next_n].y = y;
								dun->next[dun->next_n].x = x;
								dun->next_feat[dun->next_n] = place_feat;
								dun->next_n++;
							}
						}
						else
						{
							cave_set_feat(y, x, place_feat);

							/* Hack - fix outer walls if placing inside a room */
							if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) && !(outer)) cave_alter_feat(y, x, FS_INNER);
						}

						/* Pick one choice for object on feature */
						if (((flag & (RG1_HAS_GOLD | RG1_HAS_ITEM)) != 0) && (rand_int(++choice) == 0))
						{
							y_alloc = y;
							x_alloc = x;
						}

						/* Otherwise don't place objects and features */
						continue;
					}

					/* Require "clean" floor space */
					if ((flag & (RG1_HAS_GOLD | RG1_HAS_ITEM)) != 0)
					{
						/* Either place or overwrite outer wall if required */
						if ((cave_clean_bold(y, x)) || (((flag & (RG1_OUTER)) != 0) && (cave_feat[y][x] == FEAT_WALL_OUTER)))
						{
							/* Hack -- erase outer wall */
							if (cave_feat[y][x] == FEAT_WALL_OUTER) cave_set_feat(y, x, FEAT_FLOOR);

							/* Drop gold 50% of the time if both defined */
							if (((flag & (RG1_HAS_GOLD)) != 0) && (((flag & (RG1_HAS_ITEM)) == 0) || (rand_int(100) < 50))) place_gold(y, x);
							else place_object(y, x, FALSE, FALSE);
						}
					}
				}
			}
		}

		/* Scatter objects around feature if both placed */
		if ((feat) && ((flag & (RG1_HAS_GOLD | RG1_HAS_ITEM)) != 0))
		{
			feat = 0;
			flag |= (RG1_SCATTER);

			/* Paranoia */
			if (!choice) continue;
		}

		/* Hack -- if we don't have enough the first time, scatter instead */
		if (((flag & (RG1_RANDOM)) != 0) && (choice < scatter))
		{
			flag &= ~(RG1_RANDOM);
			flag |= (RG1_SCATTER);

			/* Paranoia */
			if (!choice) continue;
		}

		/* Finally place in 8 directions */
		if (((flag & (RG1_8WAY)) != 0) && choice)
		{
			int place_feat = feat;

			/* Pick a random feature? */
			if ((feat) && (f_info[feat].mimic == feat))
			{
				place_feat = get_feat_num(object_level);

				if (!place_feat) place_feat = feat;
			}

			/* Loop through features */
			for (k = 0; k < MAX_SIGHT; k++)
			{
				for (i = 0; i < 8; i++)
				{
					/* Get position */
					y = y_alloc + k * ddy_ddd[i];
					x = x_alloc + k * ddx_ddd[i];

					/* Limit spread */
					if ((y < y1) || (y > y2) || (x < x1) || (x > x2)) continue;

					outer = ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) != 0);

					/* Assign feature */
					if (place_feat)
					{
						/* Hack -- if we are placing one feature, we replace it with a solid wall to ensure that it
						 * is not overwritten later on. We take advantage of the dun->next array to do this.
						 */
						if ((k == 0) && (dun->next_n < NEXT_MAX)) cave_feat[y][x] = FEAT_WALL_SOLID;

						/* Preserve the 'solid' status of a wall */
						if (cave_feat[y][x] == FEAT_WALL_SOLID)
						{
							/* Place solid wall now */
							if ((f_info[place_feat].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)
							{
								cave_set_feat(y, x, (f_info[place_feat].flags1 & (FF1_OUTER)) != 0 ? feat_state(place_feat, FS_SOLID) : place_feat);
							}
							else if (dun->next_n < NEXT_MAX)
							{
								/* Overwrite solid wall later */
								dun->next[dun->next_n].y = y;
								dun->next[dun->next_n].x = x;
								dun->next_feat[dun->next_n] = place_feat;
								dun->next_n++;
							}
						}
						else
						{
							cave_set_feat(y, x, place_feat);

							/* Hack - fix outer walls if placing inside a room */
							if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) && !(outer)) cave_alter_feat(y, x, FS_INNER);
						}
					}

					/* Require "clean" floor space */
					if ((flag & (RG1_HAS_GOLD | RG1_HAS_ITEM)) != 0)
					{
						/* Either place or overwrite outer wall if required */
						if ((cave_clean_bold(y, x)) || (((flag & (RG1_OUTER)) != 0) && (cave_feat[y][x] == FEAT_WALL_OUTER)))
						{
							/* Hack -- erase outer wall */
							if (cave_feat[y][x] == FEAT_WALL_OUTER) cave_set_feat(y, x, FEAT_FLOOR);

							/* Drop gold 50% of the time if both defined */
							if (((flag & (RG1_HAS_GOLD)) != 0) && (((flag & (RG1_HAS_ITEM)) == 0) || (rand_int(100) < 50))) place_gold(y, x);
							else place_object(y, x, FALSE, FALSE);
						}
					}
				}
			}
		}

		/* Finally place if allocating a single feature */
		else if (((flag & (RG1_ALLOC | RG1_SCATTER | RG1_TRAIL)) != 0) && choice)
		{
			int place_feat = feat;
			int alloc_scale = ((flag & (RG1_SCATTER)) == 0) ? scale : 1;

			/* Occasionally scale up single allocation */
			if (((flag & (RG1_SCATTER | RG1_TRAIL)) == 0) && (rand_int(100) < p_ptr->depth))
			{
				alloc_scale++;
			}

			/* Get location */
			for (y = y_alloc; (dy > 0) ? y < (y_alloc + dy * alloc_scale) : y > (y_alloc + dy * alloc_scale); y += dy)
			{
				for (x = x_alloc; (dx > 0) ? x < (x_alloc + dx * alloc_scale) : x > (x_alloc + dx * alloc_scale); x += dx)
				{
					/* Don't overwrite anything interesting */
					if ((cave_feat[y][x] != FEAT_FLOOR) && (cave_feat[y][x] != FEAT_WALL_OUTER) && (cave_feat[y][x] != FEAT_WALL_SOLID)) continue;

					/* Is outer? */
					outer = ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) != 0);

					/* Pick a random feature? */
					if ((feat) && (f_info[feat].mimic == feat))
					{
						place_feat = get_feat_num(object_level);

						if (!place_feat) place_feat = feat;
					}

					/* Assign feature */
					if (place_feat)
					{
						/* Hack -- if we are placing one feature, we replace it with a solid wall to ensure that it
						 * is not overwritten later on. We take advantage of the dun->next array to do this.
						 */
						if (k == 0) cave_feat[y][x] = FEAT_WALL_SOLID;

						/* Preserve the 'solid' status of a wall */
						if (cave_feat[y][x] == FEAT_WALL_SOLID)
						{
							/* Place solid wall now */
							if ((f_info[place_feat].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)
							{
								cave_set_feat(y, x, (f_info[place_feat].flags1 & (FF1_OUTER)) != 0 ? feat_state(place_feat, FS_SOLID) : place_feat);
							}
							else
							{
								/* Overwrite solid wall later */
								dun->next[dun->next_n].y = y;
								dun->next[dun->next_n].x = x;
								dun->next_feat[dun->next_n] = place_feat;
								dun->next_n++;
							}
						}
						else
						{
							cave_set_feat(y, x, place_feat);

							/* Hack - fix outer walls if placing inside a room */
							if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) && !(outer)) cave_alter_feat(y, x, FS_INNER);
						}
					}

					/* Require "clean" floor space */
					if ((flag & (RG1_HAS_GOLD | RG1_HAS_ITEM)) != 0)
					{
						/* Either place or overwrite outer wall if required */
						if ((cave_clean_bold(y, x)) || (((flag & (RG1_OUTER)) != 0) && (cave_feat[y][x] == FEAT_WALL_OUTER)))
						{
							/* Hack -- erase outer wall */
							if (cave_feat[y][x] == FEAT_WALL_OUTER) cave_set_feat(y, x, FEAT_FLOOR);

							/* Drop gold 50% of the time if both defined */
							if (((flag & (RG1_HAS_GOLD)) != 0) && (((flag & (RG1_HAS_ITEM)) == 0) || (rand_int(100) < 50))) place_gold(y, x);
							else place_object(y, x, FALSE, FALSE);
						}
					}
				}
			}
		}
	}

	/* Clear feature hook */
	if ((feat) && (f_info[feat].mimic == feat))
	{
		/* Clear the hook */
		get_feat_num_hook = NULL;

		get_feat_num_prep();
	}
}



/*
 * Find a good spot for the next room.  -LM-
 *
 * Find and allocate a free space in the dungeon large enough to hold
 * the room calling this function.
 *
 * We allocate space in 11x11 blocks, but want to make sure that rooms
 * align neatly on the standard screen.  Therefore, we make them use
 * blocks in few 11x33 rectangles as possible.
 * 
 * AD - This routine fails far too frequently, so have attempted to
 * reduce the failures by sliding the room around a little if it fails,
 * rather than randomly generating new possible location.
 *
 * Be careful to include the edges of the room in height and width!
 *
 * Return TRUE and values for the center of the room if all went well.
 * Otherwise, return FALSE.
 */
static bool find_space(int *y, int *x, int height, int width)
{
	int i;
	int by, bx, by1, bx1, by2, bx2;
	int block_y = 0;
	int block_x = 0;

	bool filled;

	/* Find out how many blocks we need. */
	int blocks_high = 1 + ((height - 1) / BLOCK_HGT);
	int blocks_wide = 1 + ((width - 1) / BLOCK_WID);

	/* Out of space in the room array */
	if (dun->cent_n >= CENT_MAX) return (FALSE);

	/* Sometimes, little rooms like to have more space. */
	if (blocks_wide == 2)
	{
		if (!rand_int(3)) blocks_wide = 3;
	}
	else if (blocks_wide == 1)
	{
		if (!rand_int(2)) blocks_wide = rand_range(2, 3);
	}


	/* We'll allow twenty-five guesses. */
	for (i = 0; i < 25; i++)
	{
		filled = FALSE;
	
		/* Hack -- try to put stuff near lakes */
		if (i < dun->lake_n)
		{
			/* Pick a block 'near' the lake */
			block_y = (dun->lake[i].y / BLOCK_HGT) + 1 - rand_int(blocks_high + 2);
			block_x = (dun->lake[i].x / BLOCK_WID) + 1 - rand_int(blocks_wide + 2);

			/* Keep in dungeon */
			if (block_y < 0) block_y = 0;
			if (block_x < 0) block_x = 0;
			if (block_y + blocks_high >= dun->row_rooms) block_y = dun->row_rooms - blocks_high - 1;
			if (block_x + blocks_wide >= dun->col_rooms) block_x = dun->col_rooms - blocks_wide - 1;
		}
		/* We try to use the previous attempt as an indicator where not to try this iteration */
		else
		{
			/* Random initial pick, else vary y half the time */
			if (i % 2 == 0)
			{
				/* Pick a random y */
				block_y = rand_int(dun->row_rooms - blocks_high);
			}
			/* Flip y if previous pick was near an edge */
			else if ((block_y < blocks_high) || (block_y >= dun->row_rooms - 2 * blocks_high))
			{
				block_y = dun->row_rooms - blocks_high - block_y - 1;
			}
			/* Otherwise try a random edge */
			else
			{
				block_y = rand_int(100) < 50 ? 0 : dun->row_rooms - blocks_high - 1;
			}
			
			/* Random initial pick, else vary x half the time */
			if ((i % 4 == 0) || (i % 4 == 3))
			{
				block_x = rand_int(dun->col_rooms - blocks_wide);
			}
			/* Flip x if previous pick was an edge */
			else if ((block_x < blocks_wide) || (block_x >= dun->col_rooms - 2 * blocks_wide))
			{
				block_x = dun->col_rooms - blocks_wide - block_x - 1;
			}
			/* Otherwise try a random edge */
			else
			{
				/* Pick a random x */
				block_x = rand_int(100) < 50 ? 0 : dun->col_rooms - blocks_wide - 1;
			}
		}
		
		/* Itty-bitty rooms can shift about within their rectangle */
		if (blocks_wide < 3)
		{
			/* Rooms that straddle a border must shift. */
			if ((blocks_wide == 2) && ((block_x % 3) == 2))
			{
				if (!rand_int(2)) block_x--;
				else block_x++;
			}
		}

		/* Rooms with width divisible by 3 get fitted to a rectangle. */
		else if ((blocks_wide % 3) == 0)
		{
			/* Align to the left edge of a 11x33 rectangle. */
			if ((block_x % 3) == 2) block_x++;
			if ((block_x % 3) == 1) block_x--;
		}

		/*
		 * Big rooms that do not have a width divisible by 3 get
		 * aligned towards the edge of the dungeon closest to them.
		 */
		else
		{
			/* Shift towards left edge of dungeon. */
			if (block_x + (blocks_wide / 2) <= dun->col_rooms / 2)
			{
				if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
					block_x--;
				if ((block_x % 3) == 1) block_x--;
			}

			/* Shift toward right edge of dungeon. */
			else
			{
				if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
					block_x++;
				if ((block_x % 3) == 1) block_x++;
			}
		}

		/* Extract blocks */
		by1 = block_y + 0;
		bx1 = block_x + 0;
		by2 = block_y + blocks_high;
		bx2 = block_x + blocks_wide;

		/* Never run off the screen */
		if ((by1 < 0) || (by2 > dun->row_rooms)) continue;
		if ((bx1 < 0) || (bx2 > dun->col_rooms)) continue;

		/* Verify available space */
		for (by = by1; by < by2; by++)
		{
			for (bx = bx1; bx < bx2; bx++)
			{
				if (dun->room_map[by][bx])
				{
					filled = TRUE;
				}
			}
		}
		
		/* If space filled, try again. */
		if (filled) continue;


		/* It is *extremely* important that the following calculation */
		/* be *exactly* correct to prevent memory errors XXX XXX XXX */

		/* Acquire the location of the room */
		(*y) = ((by1 + by2) * BLOCK_HGT) / 2;
		(*x) = ((bx1 + bx2) * BLOCK_WID) / 2;

		/* Save the room location */
		if (dun->cent_n < CENT_MAX)
		{
			dun->cent[dun->cent_n].y = *y;
			dun->cent[dun->cent_n].x = *x;
		}

		/* Reserve some blocks.  Mark each with the room index. */
		for (by = by1; by < by2; by++)
		{
			for (bx = bx1; bx < bx2; bx++)
			{
				dun->room_map[by][bx] = TRUE;
				dun_room[by][bx] = dun->cent_n;
			}
		}

		/* Get matching ecology */
		if (dun->cent_n < cave_ecology.num_ecologies)
		{
			/* Pick which ecology */
			room_info[dun->cent_n].ecology = (1L << dun->cent_n) | (1L << (dun->cent_n + MAX_ECOLOGIES));

			/* Pick which deepest monster */
			room_info[dun->cent_n].deepest_race = cave_ecology.deepest_race[dun->cent_n];
		}

		/* Get the closest ecology */
		else
		{
			int min_d = 3000;
			int d, pick = 1;

			for (i = 0; i < cave_ecology.num_ecologies; i++)
			{
				/* Find distance */
				d = distance((*y), (*x), dun->cent[i].y, dun->cent[i].x);

				/* Pick nearest ecology */
				if (d < min_d)
				{
					pick = i + 1;
					min_d = d;
				}
			}

			/* Pick which ecology */
			room_info[dun->cent_n].ecology = room_info[pick].ecology & ((1L << (MAX_ECOLOGIES)) - 1);

			/* Pick which deepest monster */
			room_info[dun->cent_n].deepest_race = room_info[pick].deepest_race;
		}

		/* Put the guardian in a room, unless they're in the tower */
		if ((dun->cent_n == 1) && ((level_flag & (LF1_TOWER)) == 0))
		{
			/* Set the coordinates */
			dun->guard_y0 = *y - height / 2;
			dun->guard_x0 = *x - width / 2;
			dun->guard_y1 = *y + height / 2;
			dun->guard_x1 = *x + width / 2;
		}

		/* Increase the room count */
		dun->cent_n++;

		/* Success. */
		return (TRUE);
	}

	/* Failure. */
	return (FALSE);
}



/*
 * If we fail to place a room after finding space for it,
 * we need to free it.
 */
static void free_space(int y, int x, int height, int width)
{
	int by, bx, by1, bx1, by2, bx2;

	/* Find out how many blocks we need. */
	int blocks_high = 1 + ((height - 1) / BLOCK_HGT);
	int blocks_wide = 1 + ((width - 1) / BLOCK_WID);

	/* Find top left corner */
	int block_y = (y - ((height+1) / 2)) / BLOCK_HGT;
	int block_x = (x - ((width+1) / 2)) / BLOCK_WID;
	
	/* Extract blocks */
	by1 = block_y + 0;
	bx1 = block_x + 0;
	by2 = block_y + blocks_high;
	bx2 = block_x + blocks_wide;

	if (cheat_room) message_add(format("Failed placing room %d. Freeing space.", dun->cent_n), MSG_GENERIC);
	
	/* Erase the room location */
	dun->cent_n--;
	
	/* Free reserved blocks */
	for (by = by1; by < by2; by++)
	{
		for (bx = bx1; bx < bx2; bx++)
		{
			dun->room_map[by][bx] = FALSE;
			dun_room[by][bx] = 0;
		}
	}
}



/*
 *  Ensure that the terrain matches the required level flags.
 */
static bool check_level_flags(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Exclude terrain of various types */
	if (level_flag & (LF1_WATER | LF1_LAVA | LF1_ICE | LF1_ACID | LF1_OIL | LF1_LIVING))
	{
		if (!(level_flag & (LF1_LIVING)) && (f_ptr->flags3 & (FF3_LIVING)))
		{
			return (FALSE);
		}

		if (!(level_flag & (LF1_WATER)) && (f_ptr->flags2 & (FF2_WATER)))
		{
			return (FALSE);
		}

		if (!(level_flag & (LF1_LAVA)) && (f_ptr->flags2 & (FF2_LAVA)))
		{
			return (FALSE);
		}

		if (!(level_flag & (LF1_ICE)) && (f_ptr->flags2 & (FF2_ICE)))
		{
			return (FALSE);
		}

		if (!(level_flag & (LF1_ACID)) && (f_ptr->flags2 & (FF2_ACID)))
		{
			return (FALSE);
		}

		if (!(level_flag & (LF1_OIL)) && (f_ptr->flags2 & (FF2_OIL)))
		{
			return (FALSE);
		}
	}

	return (TRUE);

}



/*
 * Pick appropriate feature for lake.
 */
bool cave_feat_lake(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Require lake or river */
	if ((f_ptr->flags2 & (FF2_LAKE | FF2_RIVER)) == 0)
	{
		return (FALSE);
	}

	/* Okay */
	return (check_level_flags(f_idx));
}


/*
 * Returns TRUE if f_idx is a valid pool feature
 */
static bool cave_feat_pool(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Hack -- Ignore solid features */
	if ((f_ptr->flags1 & (FF1_MOVE)) == 0)
	{
		return (FALSE);
	}

	/* All remaining lake features will be fine */
	return (cave_feat_lake(f_idx));
}


/*
 * Returns TRUE if f_idx is a valid sewer feature
 */
static bool cave_feat_sewer(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Hack -- Ignore solid features */
	if ((f_ptr->flags1 & (FF1_MOVE)) == 0)
	{
		return (FALSE);
	}
	
	/* Hack -- require shallow, deep or covered */
	if ((f_ptr->flags2 & (FF2_SHALLOW | FF2_DEEP | FF2_COVERED)) == 0)
	{
		return (FALSE);
	}

	/* All remaining lake features will be fine */
	return (cave_feat_lake(f_idx));
}


/*
 * Returns TRUE if f_idx is a valid island feature
 */
static bool cave_feat_island(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Hack -- Ignore damaging features */
	if ((f_ptr->blow.effect) == 0)
	{
		return (FALSE);
	}

	/* All remaining lake features will be fine */
	return (cave_feat_lake(f_idx));
}


/*
 * Returns TRUE if f_idx is a valid pool feature
 */
static bool cave_feat_streamer(int f_idx)
{
	feature_type *f_ptr = &f_info[f_idx];

	/* Require lake or river */
	if (!(f_ptr->flags1 & (FF1_STREAMER)))
	{
		return (FALSE);
	}

	/* MegaHack -- now only quartz/magma is a valid stream */
	if (f_idx > 54) return (FALSE);

	/* All remaining features depend on level flags */
	return (check_level_flags(f_idx));
}


/*
 * Choose a terrain feature for the current level.
 * You can use a hook to ensure consistent terrain (lakes/pools).
 * This function handles themed levels as a special case. The "feeling"
 * global variable must be properly set to recognize the themed level. See
 * "build_themed_level".
 */
static u16b pick_proper_feature(bool (*feat_hook)(int f_idx))
{
	/* Default depth for the feature */
	int max_depth = p_ptr->depth;
	u16b feat;

	/* Set the given hook, if any */
	get_feat_num_hook = feat_hook;

	get_feat_num_prep();

	/* Pick a feature */
	feat = get_feat_num(max_depth);

	/* Clear the hook */
	get_feat_num_hook = NULL;

	get_feat_num_prep();

	/* Set the level flags based on the feature */
	set_level_flags(feat);

	/* Return the feature */
	return (feat);
}



/*
 * Hack -- tval and sval range for "room_info_kind()"
 */
static byte room_info_kind_tval;
static byte room_info_kind_min_sval;
static byte room_info_kind_max_sval;


/*
 *
 */
static bool room_info_kind(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->tval != room_info_kind_tval) return (FALSE);
	if (k_ptr->sval < room_info_kind_min_sval) return (FALSE);
	if (k_ptr->sval > room_info_kind_max_sval) return (FALSE);

	return(TRUE);
}

/*
 * How we match on flags
 *
 * Match any = no descriptions encountered matching monster or level flags
 * Match chance = descriptions matching monster or level flags only
 * Match theme = descriptions matching feature or tval theme
 *
 */

#define MATCH_ANY 0
#define MATCH_CHANCE 1
#define MATCH_THEME 2

/*
 * Variables to indicate whether we have the name decided already.
 */

#define PICKED_NAME1	0x01L
#define PICKED_NAME2	0x02L


/*
 * Get the room description. Return when we have a valid description, with
 * various stuff set to allow placement of features and objects relevent
 * to the room description parsed so far.
 */
static bool get_room_info(int room, int *chart, int *j, u32b *place_flag, s16b *place_feat, byte *place_tval, byte *place_min_sval, byte *place_max_sval, byte *branch, byte *branch_on, byte *name, u32b exclude)
{
	int i, count, pick, match, chance;

	byte last_tval;
	s16b last_feat;

	bool placement = FALSE;

	int counter = 0;

	if (cheat_room) message_add(format("Getting room info %d.",*chart), MSG_GENERIC);

	/* Record last placements */
	last_feat = *place_feat;
	last_tval = *place_tval;

	/* Clear placements */
	*place_tval = 0;
	*place_min_sval = 0;
	*place_max_sval = 0;
	*place_flag = 0;
	*place_feat = 0;

	/* Process the description */
	/* Note we need 1 space at end of room_desc_sections to terminate description */
	while ((*chart != 0) && (*j < ROOM_DESC_SECTIONS - 1))
	{
		/* Start over */
		i = 0;
		count = 0;
		pick = -1;
		match = MATCH_ANY;

		/* Get the start of entries in the table for this index */
		while ((*chart != d_info[i].chart) && (i < z_info->d_max)) { i++; }

		/* Reached end of chart */
		if (i >= z_info->d_max)
		{
			msg_format("Error: Reached end of chart looking for %d. Please report.", *chart);
			return (FALSE);
		}

		/* Cycle through valid entries */
		while (*chart == d_info[i].chart)
		{
			bool good_chance = FALSE;
			u32b exclude_hack = d_info[i].p_flag;

			if (counter++ > 5000)
			{
				msg_print("Error: aborting loop in room descriptions. Please report.");
				return (FALSE);
			}

			/* If not allowed at this depth, skip completely */
			if (p_ptr->depth < d_info[i].level_min) { i++; continue; }
			if (p_ptr->depth > d_info[i].level_max) { i++; continue; }

			/* Hack for excluding directions. This allows us to exclude 'directions' except 'all directions' */
			if ((exclude_hack & (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST)) == (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST))
			{
				exclude_hack &= ~(RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST);
			}

			/* If exluding these flags, skip completely. */
			if ((exclude & (exclude_hack)) != 0) { i++; continue; }

			/* Exclude items or gold */
			if ((exclude & (RG1_HAS_ITEM)) && (d_info[i].tval) && (d_info[i].tval < TV_GOLD)) { i++;continue; }
			if ((exclude & (RG1_HAS_GOLD)) /* && (d_info[i].tval) */ && (d_info[i].tval >= TV_GOLD)) { i++;continue; }

			/* Don't place same features or objects twice */
			if (last_tval && (d_info[i].tval == last_tval)) {i++; continue;}
			if (last_feat && (d_info[i].feat == last_feat)) {i++; continue;}

			/* Reset chance */
			chance = 0;

			/* If requires this level type, reduce chance of occurring */
			if ((d_info[i].l_flag) && ((level_flag & d_info[i].l_flag) != 0)) good_chance = TRUE;

			/* If not allowed because doesn't match level monster, reduce chance of occuring */
			else if ((cave_ecology.ready) && (cave_ecology.num_races))
			{
				/* Match main monster */
				monster_race *r_ptr = &r_info[room_info[room].deepest_race];

				/* Check for char match */
				if ((d_info[i].r_char) && (d_info[i].r_char == r_ptr->d_char)) good_chance = TRUE;

				/* Check for flag match */
				if (d_info[i].r_flag)
				{
					if ((d_info[i].r_flag < 33) &&
						((r_ptr->flags1 & (1L << (d_info[i].r_flag - 1))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 33) &&
						(d_info[i].r_flag < 65) &&
						((r_ptr->flags2 & (1L << (d_info[i].r_flag - 33))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 65) &&
						(d_info[i].r_flag < 97) &&
						((r_ptr->flags3 & (1L << (d_info[i].r_flag - 65))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 97) &&
						(d_info[i].r_flag < 129) &&
						((r_ptr->flags4 & (1L << (d_info[i].r_flag - 97))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 129) &&
						(d_info[i].r_flag < 161) &&
						((r_ptr->flags5 & (1L << (d_info[i].r_flag - 129))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 161) &&
						(d_info[i].r_flag < 193) &&
						((r_ptr->flags6 & (1L << (d_info[i].r_flag - 161))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 193) &&
						(d_info[i].r_flag < 225) &&
						((r_ptr->flags7 & (1L << (d_info[i].r_flag - 193))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 225) &&
						(d_info[i].r_flag < 257) &&
						((r_ptr->flags8 & (1L << (d_info[i].r_flag - 225))) != 0)) good_chance = TRUE;

					else if ((d_info[i].r_flag >= 257) &&
						(d_info[i].r_flag < 289) &&
						((r_ptr->flags9 & (1L << (d_info[i].r_flag - 257))) != 0)) good_chance = TRUE;
				}
			}

			/* Good chance */
			if (good_chance)
			{
				/* Set chance */
				chance = d_info[i].chance;

				/* Improve match if placing objects or features */
				if ((match < MATCH_CHANCE) && ((d_info[i].feat) || (d_info[i].tval)))
				{
					count = 0;
					match = MATCH_CHANCE;
				}
			}

			/* Themes */
			if ((dun->theme_feat) || ((dun->flood_feat) && ((room_info[room].flags & (ROOM_FLOODED)) != 0)))
			{
				int k;
				bool theme_match = FALSE;
				bool any_match = FALSE;

				for (k = 0; k < MAX_THEMES; k++)
				{
					if (d_info[i].theme[k])
					{
						any_match = TRUE;
					}

					/* If room is flooded match flooding */
					if ((dun->flood_feat) && ((room_info[room].flags & (ROOM_FLOODED)) != 0))
					{
						if (d_info[i].theme[k] == dun->flood_feat)
						{
							theme_match = TRUE;
						}
					}
					
					/* Match theme all of the time */
					if (d_info[i].theme[k] == dun->theme_feat)
					{
						theme_match = TRUE;
					}
				}

				/* Chance to match a theme */
				if (any_match)
				{
					/* Matched a theme */
					if (theme_match)
					{
						/* Must choose a matching theme for this section */
						if (match < MATCH_THEME)
						{
							chance = 0;
							match = MATCH_THEME;
						}
					}
					/* Already has a match to a theme in this section, and this choice doesn't match */
					else if (match == MATCH_THEME)
					{
						/* No chance of picking this choice */
						chance = 0;
					}
				}
			}

			/* Default chance */
			if ((!chance) && (match == MATCH_ANY)) chance = d_info[i].not_chance;

			/* Chance of room entry */
			if (chance)
			{
				/* Add to chances */
				count += chance;

				/* Check chance */
				if (rand_int(count) < chance) pick = i;
			}

			/* Increase index */
			i++;
		}

		/* Paranoia -- Have picked any entry? */
		if (pick >= 0)
		{
			/* Set index to choice */
			i = pick;

			/* Save index if we have anything to describe */
			/* Note hack for efficiency */
			if (((((*name) & (PICKED_NAME1)) == 0) && (strlen(d_name + d_info[i].name1) > 0))
				|| ( (((*name) & (PICKED_NAME2)) == 0) && (strlen(d_name + d_info[i].name2) > 0))
				|| (strlen(d_text + d_info[i].text) > 0))
			{
				/* Paranoia */
				if ((room < DUN_ROOMS) && (*j < ROOM_DESC_SECTIONS))
				{
					room_info[room].section[(*j)++] = i;

					if (cheat_room) message_add(format("Saving room info %d.",i), MSG_GENERIC);
				}

				if (strlen(d_name + d_info[i].name1) > 0) *name |= PICKED_NAME1;
				if (strlen(d_name + d_info[i].name2) > 0) *name |= PICKED_NAME2;
			}

			/* Enter the next chart */
			*chart = d_info[i].next;

			/* Branch if required */
			if ((*branch_on) && (*chart == *branch_on))
			{
				/* Set alternate chart */
				*chart = *branch;

				/* Clear branch conditions */
				*branch = 0;
				*branch_on = 0;
			}

			/* Paranoia */
			if (room < DUN_ROOMS)
			{
				int k;

				/* Place flags except SEEN or HEARD */
				room_info[room].flags |= (d_info[i].flags & ~(ROOM_SEEN | ROOM_HEARD));

				/* Set room themes */
				for (k = 0; k < MAX_THEMES; k++)
				{
					if (!(room_info[room].theme[k]) && (d_info[i].theme[k])) room_info[room].theme[k] = d_info[i].theme[k];
				}
			}

			/* Get tval */
			if (d_info[i].tval)
			{
				*place_tval = d_info[i].tval;
				*place_min_sval = d_info[i].min_sval;
				*place_max_sval = d_info[i].max_sval;
			}

			/* Get feature */
			if (d_info[i].feat) *place_feat = d_info[i].feat;

			/* Get branch */
			if (d_info[i].branch) *branch = d_info[i].branch;

			/* Get branch condition */
			if (d_info[i].branch_on) *branch_on = d_info[i].branch_on;

			/* Set dungeon theme. */
			if (!(dun->theme_feat))
			{
				int l;
				int k = 0;

				/* Try room themes */
				for (l = 0; l < MAX_THEMES; l++)
				{
					if ((d_info[i].theme[l]) && !(rand_int(++k))) dun->theme_feat = d_info[i].theme[l];
				}

				/* Also try the feature we are placing */
				if ((*place_feat) && !(rand_int(++k))) dun->theme_feat = *place_feat;
			}

			/* Add flags */
			*place_flag |= d_info[i].p_flag;

			/* Rows and columns exclude mazes and star bursts for next placement */
			if (d_info[i].p_flag & (RG1_ROWS | RG1_COLS)) exclude |= (RG1_STARBURST | RG1_MAZE_DECOR | RG1_MAZE_PATH | RG1_MAZE_WALL);

			/* Don't place yet */
			if ((*place_flag & (RG1_PLACE)) == 0) continue;

			/* Pick objects if needed */
			if (*place_tval)
			{
				/* Set object hooks if required */
				if (*place_tval < TV_GOLD)
				{
					room_info_kind_tval = *place_tval;
					room_info_kind_min_sval = *place_min_sval;
					room_info_kind_max_sval = *place_max_sval;

					get_obj_num_hook = room_info_kind;

					/* Prepare allocation table */
					get_obj_num_prep();

					/* Drop gold */
					*place_flag |= (RG1_HAS_ITEM);
				}
				else
				{
					/* Drop gold */
					*place_flag |= (RG1_HAS_GOLD);
				}
			}

			/* Record streamers for later */
			if ((*place_feat) && (f_info[*place_feat].flags1 & (FF1_STREAMER)) && (dun->stream_n < DUN_MAX_STREAMER))
			{
				int n;

				for (n = 0; n < dun->stream_n; n++)
				{
					/* Already added */
					if (dun->streamer[n] == *place_feat) break;
				}

				if (n == dun->stream_n)
				{
					/* Add dungeon streamer */
					dun->streamer[dun->stream_n++] = *place_feat;
				}
			}

			/* Placement */
			placement = TRUE;

			break;
		}

		/* Report errors */
		if (pick < 0)
		{
			break;
		}
	}

	/* Finished (chart = 0) */
	return (placement);
}


/*
 * Set room flags.
 *
 * Use the get_room_info function to set room flags only, not place anything.
 * We do this for interesting rooms, vaults, flooded rooms of all types and
 * anywhere else that we do not explicitly generate room contents.
 */
static void set_room_flags(int room, int type, bool light)
{
	int j = 0;

	u32b place_flag = 0L;
	u32b exclude = (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST);

	byte place_tval = 0;
	byte place_min_sval = 0;
	byte place_max_sval = 0;
	s16b place_feat = 0;

	byte name = 0L;

	byte branch = 0;
	byte branch_on = 0;

	/* Exclude light or dark */
	if (light) exclude |= RG1_DARK;
	else exclude |= RG1_LITE;

	/* Get room info */
	while (get_room_info(room, &type, &j, &place_flag, &place_feat, &place_tval, &place_min_sval, &place_max_sval, &branch, &branch_on, &name, exclude))
	{
		/* Don't place anything */
	}

	/* Clear object hook */
	if (get_obj_num_hook)
	{
		get_obj_num_hook = NULL;

		/* Prepare allocation table */
		get_obj_num_prep();
	}

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Type */
		room_info[room].type = type;

		/* Further paranoia */
		if (j < ROOM_DESC_SECTIONS)
		{
			/* Terminate index list */
			room_info[room].section[j] = -1;
		}
	}
}


typedef s16b pool_type[3];


/*
 *  Get room info for rooms where we do not have any north / south / east / west or clear place to drop items.
 */
static void set_irregular_room_info(int room, int type, bool light, u32b exclude, s16b *feat, s16b *edge, s16b *inner, s16b *alloc, pool_type *pool, int *n_pools)
{
	int j = 0;

	u32b place_flag = 0L;

	byte place_tval = 0;
	byte place_min_sval = 0;
	byte place_max_sval = 0;
	s16b place_feat = 0;

	byte name = 0L;

	byte branch = 0;
	byte branch_on = 0;

	/* Default exclusions */
	exclude |= (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST | RG1_HAS_ITEM | RG1_HAS_GOLD |
			RG1_MAZE_PATH | RG1_MAZE_WALL | RG1_MAZE_DECOR | RG1_CHECKER | RG1_ROWS | RG1_COLS |
			RG1_8WAY | RG1_3X3HIDDEN);

	/* Hack -- diagnostics */
	if (cheat_room) message_add("Setting irregular room info.", MSG_GENERIC);

	/* Exclude light or dark */
	if (light) exclude |= RG1_DARK;
	else exclude |= RG1_LITE;

	/* Flooded dungeon */
	if (room_info[room].flags & (ROOM_FLOODED))
	{
		/* Flood floor */
		*feat = dun->flood_feat;

		/* Surround by edge */
		*edge = f_info[dun->flood_feat].edge;

		/* Has a passable feature */
		if ((*edge) && ((f_info[*edge].flags1 & (FF1_MOVE)) != 0))
		{
			room_info[room].flags &= ~(ROOM_FLOODED);
		}
		/* Must bridge otherwise */
		else
		{
			room_info[room].flags &= (ROOM_BRIDGED);
		}

		/* Get 'flooded' room flags. This is always 'boring'. */
		set_room_flags(room, ROOM_NORMAL, light);

		/* Type */
		room_info[room].type = type;

		return;
	}

	/* Get room info */
	else while (get_room_info(room, &type, &j, &place_flag, &place_feat, &place_tval, &place_min_sval, &place_max_sval, &branch, &branch_on, &name,
		exclude))
	{
		/* Place features or items if needed */
		if (place_feat)
		{
			/* Place throughout room, unless scattering */
			if (((place_flag & (RG1_CENTRE)) != 0) &&
				((place_flag & (RG1_ALLOC | RG1_INNER | RG1_OUTER | RG1_STARBURST | RG1_SCATTER | RG1_RANDOM)) == 0))
			{
				/* Hack -- only fill irregular rooms with lake-like terrain, floors or ground */
				if (((f_info[place_feat].flags1 & (FF1_FLOOR)) == 0) &&
					((f_info[place_feat].flags2 & (FF2_LAKE | FF2_RIVER)) == 0) &&
					((f_info[place_feat].flags3 & (FF3_GROUND)) == 0))
				{
					if ((exclude & (RG1_INNER)) == 0)
					{
						place_flag |= (RG1_INNER);
					}
					else if ((exclude & (RG1_ALLOC)) == 0)
					{
						place_flag |= (RG1_ALLOC);
					}
					else if (((exclude & (RG1_RANDOM)) == 0) && ((place_flag & (RG1_SCATTER)) == 0))
					{
						place_flag |= (RG1_RANDOM);
					}

					/* Clear flags */
					place_flag &= ~(RG1_CENTRE);
				}
				/* Fill room with terrain */
				else
				{
					exclude |= RG1_CENTRE;
					*feat = place_feat;

					if (((place_flag & (RG1_EDGE)) != 0) && ((place_flag & (RG1_IGNORE_EDGE | RG1_BRIDGE_EDGE)) != 0))
					{
						exclude |= RG1_EDGE;
						*edge = place_feat;

						if ((place_flag & (RG1_BRIDGE_EDGE)) != 0)
						{
							if (f_info[*feat].flags2 & (FF2_PATH))
							{
								/* Bridge previous contents */
								*edge = feat_state(*feat, FS_BRIDGE);
							}
							/* Apply tunnel */
							else if (f_info[*feat].flags1 & (FF1_TUNNEL))
							{
								/* Tunnel previous contents */
								*edge = feat_state(*feat, FS_TUNNEL);
							}
						}
					}
				}
			}

			if ((place_flag & (RG1_ALLOC | RG1_SCATTER)) != 0)
			{
				exclude |= (RG1_ALLOC | RG1_SCATTER);
				*alloc = place_feat;

				place_flag &= ~(RG1_INNER | RG1_STARBURST | RG1_OUTER | RG1_EDGE | RG1_SCATTER | RG1_RANDOM);
			}

			if ((place_flag & (RG1_INNER | RG1_STARBURST)) != 0)
			{
				exclude |= RG1_INNER | RG1_STARBURST;
				*inner = place_feat;

				/* Hack -- inner and outer --> inner only */
				place_flag &= ~(RG1_OUTER);
			}

			if ((place_flag & (RG1_EDGE)) != 0)
			{
				exclude |= RG1_EDGE;
				*edge = place_feat;
			}

			if ((place_flag & (RG1_OUTER)) != 0)
			{
				exclude |= RG1_OUTER;
				room_info[room].theme[THEME_SOLID] = place_feat;
			}

			if ((place_flag & (RG1_RANDOM)) != 0)
			{
				if (*n_pools >= 2) exclude |= RG1_RANDOM;
				(*pool)[(*n_pools)++] = place_feat;
			}
		}
	}

	/* Clear object hook */
	if (get_obj_num_hook)
	{
		get_obj_num_hook = NULL;

		/* Prepare allocation table */
		get_obj_num_prep();
	}

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Type */
		room_info[room].type = type;

		/* Further paranoia */
		if (j < ROOM_DESC_SECTIONS)
		{
			/* Terminate index list */
			room_info[room].section[j] = -1;
		}
	}
}


/* Hack - spill light out of the edge of a room */
static void check_windows_y(int y1, int y2, int x)
{
	int y;

	for (y = y1; y <= y2; y++)
	{
		if (f_info[cave_feat[y][x]].flags1 & (FF1_LOS))
			cave_info[y][x] |= (CAVE_GLOW);
	}
}


/* Hack - spill light out of the edge of a room */
static void check_windows_x(int x1, int x2, int y)
{
	int x;

	for (x = x1; x <= x2; x++)
	{
		if (f_info[cave_feat[y][x]].flags1 & (FF1_LOS))
			cave_info[y][x] |= (CAVE_GLOW);
	}
}


/* This ensures we have enough variation at each edge */
#define MIN_CONCAVITY	9

/*
 * Helper function for build_type_concave. We use this to generate
 * inner concave spaces as well.
 */
static bool generate_concave(int points, int y1a, int x1a, int y2a, int x2a,
	int y1b, int x1b, int y2b, int x2b, bool light, int edge, int floor, int requires)
{
	int verty[12];
	int vertx[12];
	int n = 0;
	int d, i;
	
	/* Make certain the overlapping room does not cross the dungeon edge. */
	if ((!in_bounds_fully(y1a, x1a)) || (!in_bounds_fully(y1b, x1b))
		 || (!in_bounds_fully(y2a, x2a)) || (!in_bounds_fully(y2b, x2b))) return (FALSE);

	/* If edged, shrink boundaries to fit */
	if (edge)
	{
		y1a++; y1b++; x1a++; x1b++;
		y2a--; y2b--; x2a--; x2b--;
	}
	
	/* Generate 1 or more points for each edge */
	d = randint(points);
	
	/* Add vertices */
	for (i = 0; i < d; i++)
	{
		int y1w = (x1a < x1b ? y1a : (x1a == x1b ? MIN(y1a, y1b) : y1b));
		int y2w = (x1a < x1b ? y2a : (x1a == x1b ? MAX(y2a, y2b) : y2b));
		int x1w = MIN(x1a, x1b);
		int x2w = (x1a == x1b ? x1a + 1 : MAX(x1a, x1b) - 1);
		
		int y = y1w + rand_int(y2w - y1w + 1);
		int x = x1w + rand_int(x2w - x1w + 1);
		
		int j = n;
		
		/* Sort from highest to lowest y */
		while ((j > 0) && (verty[j-1] <= y))
		{
			verty[j] = verty[j-1];
			vertx[j] = vertx[j-1];
			j--;
		}

		/* Insert new value */
		verty[j] = y;
		vertx[j] = x;
		n++;
	}

	/* Generate 1 or more points for each edge */
	d = randint(points);
	
	/* Add vertices */
	for (i = 0; i < d; i++)
	{
		int y1n = MIN(y1a, y1b);
		int y2n = (y1a == y1b ? y1a + 1 : MAX(y1a, y1b) - 1);
		int x1n = (y1a < y1b ? x1a : (y1a == y1b ? MIN(x1a, x1b): x1b));
		int x2n = (y1a < y1b ? x2a : (y1a == y1b ? MAX(x2a, x2b): x2b));
		
		int y = y1n + rand_int(y2n - y1n + 1);
		int x = x1n + rand_int(x2n - x1n + 1);
		
		int j = n;
		
		/* Sort from lowest to highest x */
		while ((j > 0) && (vertx[j-1] >= x))
		{
			verty[j] = verty[j-1];
			vertx[j] = vertx[j-1];
			j--;
		}

		/* Insert new value */
		verty[j] = y;
		vertx[j] = x;
		n++;
	}

	/* Generate 1 or more points for each edge */
	d = randint(points);
	
	/* Add vertices */
	for (i = 0; i < d; i++)
	{
		int y1e = (x2a > x2b ? y1a : (x1a == x1b ? MIN(y1a, y1b): y1b));
		int y2e = (x2a > x2b ? y2a : (x1a == x1b ? MAX(y2a, y2b): y2b));
		int x1e = (x2a == x2b ? x2a - 1 : MIN(x2a, x2b) + 1);
		int x2e = MAX(x2a, x2b);
		
		int y = y1e + rand_int(y2e - y1e + 1);
		int x = x1e + rand_int(x2e - x1e + 1);
		
		int j = n;
		
		/* Sort from lowest to highest y */
		while ((j > 0) && (verty[j-1] >= y))
		{
			verty[j] = verty[j-1];
			vertx[j] = vertx[j-1];
			j--;
		}

		/* Insert new value */
		verty[j] = y;
		vertx[j] = x;
		n++;
	}

	/* Generate 1 or more points for each edge */
	d = randint(points);
	
	/* Add vertices */
	for (i = 0; i < d; i++)
	{
		int y1s = (y2a == y2b ? y2a - 1 : MIN(y2a, y2b) + 1);
		int y2s = MAX(y2a, y2b);
		int x1s = (y2a > y2b ? x1a : (y2a == y2b ? MIN(x1a, x1b): x1b));
		int x2s = (y2a > y2b ? x2a : (y2a == y2b ? MAX(x2a, x2b): x2b));
		
		int y = y1s + rand_int(y2s - y1s + 1);
		int x = x1s + rand_int(x2s - x1s + 1);
		
		int j = n;
		
		/* Sort from highest to lowest x */
		while ((j > 0) && (vertx[j-1] <= x))
		{
			verty[j] = verty[j-1];
			vertx[j] = vertx[j-1];
			j--;
		}

		/* Insert new value */
		verty[j] = y;
		vertx[j] = x;
		n++;
	}

	/* Remove duplicate vertices */
	for (i = 1; i < n; i++)
	{
		if ((verty[i] == verty[i-1]) && (vertx[i] == vertx[i-1]))
		{
			int j;
			
			for (j = i + 1; j < n; j++)
			{
				verty[j-1] = verty[j];
				vertx[j-1] = vertx[j];
			}
			
			n--;
		}
	}
	
	/* Generate the polygon */
	return(generate_poly_room(n, verty, vertx, edge, floor, edge, requires, (CAVE_ROOM) | (light ? (CAVE_GLOW) : 0)));
}



/*
 * Build a concave polygonal room using the algorithm described at
 * http://roguebasin.roguelikedevelopment.org/index.php?title=Irregular_Shaped_Rooms
 */
static bool build_type_concave(int room, int type, int y0, int x0, int y1a, int x1a, int y2a, int x2a,
	int y1b, int x1b, int y2b, int x2b, bool light, bool inner_allowed)
{
	s16b floor = FEAT_FLOOR;
	s16b edge = FEAT_WALL_OUTER;
	s16b inner = 0;
	s16b alloc = 0;
	pool_type pool;
	int n_pools = 3;
	
	u32b exclude = (RG1_RANDOM);

	/* Inner allowed */
	if (!inner_allowed) exclude |= (RG1_INNER | RG1_STARBURST);

	/* No pool to start */
	pool[0] = 0;
	
	/* Flood dungeon if required */
	if (room_info[room].flags & (ROOM_FLOODED))
	{
		if ((!f_info[dun->flood_feat].edge) || ((f_info[f_info[dun->flood_feat].edge].flags1 & (FF1_MOVE)) == 0))
		{
			floor = dun->flood_feat;
			edge = f_info[dun->flood_feat].edge;

			room_info[room].flags |= (ROOM_BRIDGED);
		}
		else
		{
			/* Generate pool */
			inner = dun->flood_feat;

			room_info[room].flags |= (ROOM_EDGED);
		}
	}

	/* Set up the room */
	set_irregular_room_info(room, type, light, exclude, &floor, &edge, &inner, &alloc, &pool, &n_pools);
	
	/* Build the room */
	if (!generate_concave(type - ROOM_NORMAL_CONCAVE + 1, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, edge, floor, 0))
	{
		return (FALSE);
	}
	
	/* Build the inner room if required */
	if (inner)
	{
		int d = rand_range(2, 4);
		
		/* Shrink the room boundaries */
		y1a = ((d*y1a) + y2a) / (d+1);
		y2a = ((d*y2a) + y1a) / (d+1) +1;
		x1a = ((d*x1a) + x2a) / (d+1);
		x2a = ((d*x2a) + x1a) / (d+1) +1;
		y1b = ((d*y1b) + y2b) / (d+1);
		y2b = ((d*y2b) + y1b) / (d+1) +1;
		x1b = ((d*x1b) + x2b) / (d+1);
		x2b = ((d*x2b) + x1b) / (d+1) +1;
		
		/* Build the inner room */
		generate_concave(type - ROOM_NORMAL_CONCAVE + 1, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, f_info[inner].edge, inner, floor);
	}
	
	/* Place allocation */
	if (alloc)
	{
		cave_set_feat(y0, x0, alloc);
	}
	
	return (TRUE);
}



/*
 * Build a room consisting of two overlapping rooms.
 * Get the room description, and place stuff accordingly.
 */
static bool build_overlapping(int room, int type, int y1a, int x1a, int y2a, int x2a,
	int y1b, int x1b, int y2b, int x2b, bool light, int spacing, int scale, int scatter, bool pillars)
{
	int j = 0;
	int l = 1;
	int floor = FEAT_FLOOR;
	int edge = 0;

	u32b place_flag = 0L;
	u32b exclude = 0L;

	byte place_tval = 0;
	byte place_min_sval = 0;
	byte place_max_sval = 0;
	s16b place_feat = 0;

	byte name = 0L;

	byte branch = 0;
	byte branch_on = 0;

	bool try_simple = FALSE;

	int limit = 0;

	/* Make certain the overlapping room does not cross the dungeon edge. */
	if ((!in_bounds_fully(y1a, x1a)) || (!in_bounds_fully(y1b, x1b))
		 || (!in_bounds_fully(y2a, x2a)) || (!in_bounds_fully(y2b, x2b))) return (FALSE);

	/* Try 'simple' room */
	if ((y1a == y2a) && (y1b == y2b) && (x1a == x2a) && (x1b == x2b)) try_simple = TRUE;

	/* Flood dungeon if required */
	if (room_info[room].flags & (ROOM_FLOODED))
	{
		floor = dun->flood_feat;
		edge = f_info[dun->flood_feat].edge;

		if ((!edge) || ((f_info[edge].flags1 & (FF1_MOVE)) == 0))
		{
			room_info[room].flags |= (ROOM_BRIDGED);
		}
		else
		{
			/* Keep edge around edge of room */
			l = 2;

			room_info[room].flags |= (ROOM_EDGED);
		}
	}

	/* Exclude light or dark */
	if (light) exclude |= RG1_DARK;
	else exclude |= RG1_LITE;

	/* Generate new room (a) */
	generate_room(y1a, x1a, y2a, x2a, light);

	/* Generate new room (b) */
	generate_room(y1b, x1b, y2b, x2b, light);

	/* Generate outer walls (a) */
	generate_rect(y1a, x1a, y2a, x2a, FEAT_WALL_OUTER);

	/* Generate outer walls (b) */
	generate_rect(y1b, x1b, y2b, x2b, FEAT_WALL_OUTER);

	/* Make corners solid */
	cave_set_feat(y1a, x1a, FEAT_WALL_SOLID);
	cave_set_feat(y2a, x2a, FEAT_WALL_SOLID);
	cave_set_feat(y1a, x2a, FEAT_WALL_SOLID);
	cave_set_feat(y2a, x1a, FEAT_WALL_SOLID);
	cave_set_feat(y1b, x1b, FEAT_WALL_SOLID);
	cave_set_feat(y2b, x2b, FEAT_WALL_SOLID);
	cave_set_feat(y1b, x2b, FEAT_WALL_SOLID);
	cave_set_feat(y2b, x1b, FEAT_WALL_SOLID);

	/* Generate edge if required */
	if (edge)
	{
		/* Generate outer walls (a) */
		generate_rect(y1a + 1, x1a + 1, y2a - 1, x2a - 1, edge);

		/* Generate outer walls (b) */
		generate_rect(y1b + 1, x1b + 1, y2b - 1, x2b - 1, edge);
	}

	/* Generate inner floors (a) */
	generate_fill(y1a+l, x1a+l, y2a-l, x2a-l, FEAT_FLOOR);

	/* Generate inner floors (b) */
	generate_fill(y1b+l, x1b+l, y2b-l, x2b-l, FEAT_FLOOR);

	/* Generate inner floors (a) */
	generate_fill_pillars(y1a+l, x1a+l, y2a-l, x2a-l, floor, pillars ? spacing + 1: 0, scale);

	/* Generate inner floors (b) */
	generate_fill_pillars(y1b+l, x1b+l, y2b-l, x2b-l, floor, pillars ? spacing + 1: 0, scale);

	/* Flooded dungeon */
	if (room_info[room].flags & (ROOM_FLOODED))
	{
		/* Has edge we can move around. Treat as not flooded. */
		if (room_info[room].flags & (ROOM_EDGED)) room_info[room].flags &= ~(ROOM_FLOODED);

		/* Get 'flooded' room flags. This is always 'boring'. */
		set_room_flags(room, ROOM_NORMAL, light);

		/* Type */
		room_info[room].type = type;

		return (TRUE);
	}

	/* Get room info */
	else while (get_room_info(room, &type, &j, &place_flag, &place_feat, &place_tval, &place_min_sval, &place_max_sval, &branch, &branch_on, &name, exclude))
	{
		if (limit++ > 1000)
		{
			msg_format("Error: Keep trying to place stuff with chart item %d. Please report.", j);
			return (FALSE);
		}

		/* Place features or items if needed */
		if ((place_feat) || (place_tval))
		{
			int dy = ((place_flag & (RG1_ROWS)) != 0) ? 1 + spacing : 1;
			int dx = ((place_flag & (RG1_COLS)) != 0) ? 1 + spacing : 1;
			int outer = ((place_flag & (RG1_OUTER)) == 0) ? 1 : 0;

			int placements = 1;

			/* Place in centre of room */
			if ((place_flag & (RG1_CENTRE)) != 0)
			{
				int y1c = MAX(y1a, y1b) + 1;
				int y2c = MIN(y2a, y2b) - 1;
				int x1c = MAX(x1a, x1b) + 1;
				int x2c = MIN(x2a, x2b) - 1;
				u32b place_flag_temp = place_flag;

				/* Simple room - we can fill it completely */
				if ((try_simple) && ((place_flag & (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST)) == 0))
				{
					/* Expand edges */
					y1c--; y2c++; x1c--; x2c++;

					exclude |= (RG1_NORTH | RG1_SOUTH | RG1_EAST | RG1_WEST);
				}

				/* Fill all of vertical room */
				else if (((place_flag & (RG1_NORTH | RG1_SOUTH)) != 0) && ((place_flag & (RG1_WEST | RG1_EAST)) == 0))
				{
					if (place_flag & (RG1_NORTH)) y1c = MIN(y1a, y1b) + outer;
					if (place_flag & (RG1_SOUTH)) y2c = MAX(y2a, y2b) - outer;

					place_flag &= ~(RG1_NORTH | RG1_SOUTH);
				}

				/* Fill all of horizontal room */
				else if (((place_flag & (RG1_WEST | RG1_EAST)) != 0) && ((place_flag & (RG1_NORTH | RG1_SOUTH)) == 0))
				{
					if (place_flag & (RG1_WEST)) x1c = MIN(x1a, x1b) + outer;
					if (place_flag & (RG1_EAST)) x2c = MAX(x2a, x2b) - outer;

					place_flag &= ~(RG1_WEST | RG1_EAST);
				}

				/* Ensure some space */
				if (y1c >= y2c) { y1c = y2c - 1; y2c = y1c + 3;}
				if (x1c >= x2c) { x1c = x2c - 1; x2c = x1c + 3;}

				/* Hack -- 'outer' walls do not exist in centre of room */
				if ((place_flag_temp & (RG1_OUTER)) != 0)
				{
					place_flag_temp &= ~(RG1_OUTER);
					place_flag_temp |= (RG1_INNER);
				}

				/* Count other placements. This reduces the number of items found in the room */
				if ((place_flag & (RG1_WEST)) != 0) placements++;
				if ((place_flag & (RG1_EAST)) != 0) placements++;
				if ((place_flag & (RG1_NORTH)) != 0) placements++;
				if ((place_flag & (RG1_SOUTH)) != 0) placements++;

				generate_patt(y1c, x1c, y2c, x2c, place_feat, place_flag_temp, exclude, dy, dx, scale, (scatter + placements - 1) / placements);
			}

			/* Hack -- we've already counted for centre */
			else
			{
				placements--;

				if ((place_flag & (RG1_WEST)) != 0) placements++;
				if ((place_flag & (RG1_EAST)) != 0) placements++;
				if ((place_flag & (RG1_NORTH)) != 0) placements++;
				if ((place_flag & (RG1_SOUTH)) != 0) placements++;
			}

			/* Place in west of room */
			if ((place_flag & (RG1_WEST)) != 0)
			{
				int y1w = (x1a < x1b ? y1a : (x1a == x1b ? MIN(y1a, y1b) : y1b));
				int y2w = (x1a < x1b ? y2a : (x1a == x1b ? MAX(y2a, y2b) : y2b));
				int x1w = MIN(x1a, x1b) + outer;
				int x2w = (x1a == x1b ? x1a + 1 : MAX(x1a, x1b) - 1);

				/* No longer simple */
				try_simple = FALSE;

				/* Ensure some space */
				if (x2w <= x2w) x2w = x1w + 1;
				generate_patt(y1w, x1w, y2w, x2w, place_feat, place_flag, exclude, dy, dx, scale, (scatter + placements - 1) / placements);
			}

			/* Place in east of room */
			if ((place_flag & (RG1_EAST)) != 0)
			{
				int y1e = (x2a > x2b ? y1a : (x1a == x1b ? MIN(y1a, y1b): y1b));
				int y2e = (x2a > x2b ? y2a : (x1a == x1b ? MAX(y2a, y2b): y2b));
				int x1e = (x2a == x2b ? x2a - 1 : MIN(x2a, x2b) + 1);
				int x2e = MAX(x2a, x2b) - outer;

				/* No longer simple */
				try_simple = FALSE;

				/* Ensure some space */
				if (x1e >= x2e) x1e = x2e - 1;

					/* Draw from east to west */
					generate_patt(y1e, x2e, y2e, x1e, place_feat, place_flag, exclude, dy, -dx, scale, (scatter + placements - 1) / placements);
			}

			/* Place in north of room */
			if ((place_flag & (RG1_NORTH)) != 0)
			{
				int y1n = MIN(y1a, y1b) + outer;
				int y2n = (y1a == y1b ? y1a + 1 : MAX(y1a, y1b) - 1);
				int x1n = (y1a < y1b ? x1a : (y1a == y1b ? MIN(x1a, x1b): x1b));
				int x2n = (y1a < y1b ? x2a : (y1a == y1b ? MAX(x2a, x2b): x2b));

				/* No longer simple */
				try_simple = FALSE;

				/* Ensure some space */
				if (y2n <= y1n) y2n = y1n + 1;

				generate_patt(y1n, x1n, y2n, x2n, place_feat, place_flag, exclude, dy, dx, scale, (scatter + placements - 1) / placements);
			}

			/* Place in south of room */
			if ((place_flag & (RG1_SOUTH)) != 0)
			{
				int y1s = (y2a == y2b ? y2a - 1 : MIN(y2a, y2b) + 1);
				int y2s = MAX(y2a, y2b) - outer;
				int x1s = (y2a > y2b ? x1a : (y2a == y2b ? MIN(x1a, x1b): x1b));
				int x2s = (y2a > y2b ? x2a : (y2a == y2b ? MAX(x2a, x2b): x2b));

				/* No longer simple */
				try_simple = FALSE;

				/* Ensure some space */
				if (y1s >= y2s) y1s = y2s - 1;

				/* Draw from south to north */
				generate_patt(y2s, x1s, y1s, x2s, place_feat, place_flag, exclude, -dy, dx, scale, (scatter + placements - 1) / placements);
			}
		}

		/* Clear object hook */
		if (place_tval)
		{
			get_obj_num_hook = NULL;

			/* Prepare allocation table */
			get_obj_num_prep();
		}
	}

	/* Clear object hook */
	if (get_obj_num_hook)
	{
		get_obj_num_hook = NULL;

		/* Prepare allocation table */
		get_obj_num_prep();
	}

	/* Hack - spill light out of room */
	if (light)
	{
		check_windows_y(y1a, y2a, x1a);
		check_windows_y(y1a, y2a, x2a);
		check_windows_x(x1a, x2a, y1a);
		check_windows_x(x1a, x2a, y2a);
		check_windows_y(y1b, y2b, x1b);
		check_windows_y(y1b, y2b, x2b);
		check_windows_x(x1b, x2b, y1b);
		check_windows_x(x1b, x2b, y2b);
	}

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Type */
		room_info[room].type = type;

		/* Further paranoia */
		if (j < ROOM_DESC_SECTIONS)
		{
			/* Terminate index list */
			room_info[room].section[j] = -1;
		}
	}

	return(TRUE);
}




/* Maximum size of a fractal map */
#define MAX_FRACTAL_SIZE 65

/*
 * A fractal map is a matrix (MAX_FRACTAL_SIZE * MAX_FRACTAL_SIZE) of
 * small numbers. Each number in the map should be replaced with a dungeon
 * feature. This way we can use fractal maps to perform different actions like
 * building rooms, placing pools, etc.
 *
 * We are going to store this matrix in dynamic memory so we have to define
 * a couple of new types:
 */

/*
 * A row of the fractal map
 */
typedef byte fractal_map_wid[MAX_FRACTAL_SIZE];

/*
 * FRACTAL MAP. An array of rows. It can be used as a two-dimensional array.
 */
typedef fractal_map_wid *fractal_map;

/*
 * VERY IMPORTANT: The map must be a square. Its size must be
 * "power of 2 plus 1". Valid values are 3, 5, 9, 17, 33, 65, 129, 257, etc.
 * The maximum supported size is 65.
 *
 * Aditional comments about the construction of the map:
 * Only nine grids of this square are processed at the beginning.
 * These grids are the four corners (who MUST be previously set to meaningful
 * values) and five inner grids.
 * The inner grids are the middle points of each side plus the grid
 * at the center.
 * These nine grids can be viewed as a "3x3" square.
 * Example:
 *
 * a*b
 * ***
 * c*d
 *
 * The algorithm supplies the values for the five inner grids by
 * randomly chosing one *adjacent* corner. Example:
 *
 * aab
 * cbd
 * ccd
 *
 * The next step is to consider this "3x3" square as a part of a larger
 * square:
 *
 * a*a*b
 * *****
 * c*b*d
 * *****
 * c*c*d
 *
 * Now we have four "3x3" squares. The process is repeated for each one of
 * them. The process is stopped when the initial square can't be "magnified"
 * any longer.
 */

/* Available fractal map types */
enum
{
	FRACTAL_TYPE_17x33 = 0,
	FRACTAL_TYPE_33x65,
	FRACTAL_TYPE_9x9,
	FRACTAL_TYPE_17x17,
	FRACTAL_TYPE_33x33,
	MAX_FRACTAL_TYPES
};

static struct
{
	int hgt, wid;
} fractal_dim[MAX_FRACTAL_TYPES] =
{
	{17, 33},		/* FRACTAL_TYPE_17x33 */
	{33, 65},		/* FRACTAL_TYPE_33x65 */
	{9, 9},			/* FRACTAL_TYPE_9x9 */
	{17, 17},		/* FRACTAL_TYPE_17x17 */
	{33, 33},		/* FRACTAL_TYPE_33x33 */
};


/* These ones are the valid values for the map grids */
#define FRACTAL_NONE	0	/* Used only at construction time */
#define FRACTAL_WALL	1	/* Wall grid */
#define FRACTAL_EDGE	2	/* Wall grid adjacent to floor (outer wall) */
#define FRACTAL_FLOOR	3	/* Floor grid */
#define FRACTAL_POOL_1	4	/* Pool grid */
#define FRACTAL_POOL_2	5	/* Pool grid */
#define FRACTAL_POOL_3	6	/* Pool grid */

typedef struct fractal_template fractal_template;

/* Initialization function for templates */
typedef void (*fractal_init_func)(fractal_map map, fractal_template *t_ptr);

/*
 * A fractal template is used to set the basic shape of the fractal.
 *
 * Templates must provide values for at least the four corners of the map.
 * See the examples.
 */
struct fractal_template
{
	/* The type of the fractal map (one of FRACTAL_TYPE_*) */
	byte type;

	/* The maximum size of the fractal map (3, 5, 9, 17, 33 or 65) */
	int size;

	/* The initialization function for this template */
	fractal_init_func init_func;
};

/* Verify that a point is inside a fractal */
#define IN_FRACTAL(template,y,x) \
	(((y) >= 0) && ((y) < (template)->size) && \
	((x) >= 0) && ((x) < (template)->size))

/*
 * Places a line in a fractal map given its start and end points and a certain
 * grid type. To be used in template initialization routines.
 * Note: This is a very basic drawing routine. It works fine with vertical,
 * horizontal and the diagonal lines of a square. It doesn't support other
 * oblique lines well.
 */
static void fractal_draw_line(fractal_map map, fractal_template *t_ptr,
		int y1, int x1, int y2, int x2, byte content)
{
	int dx, dy;

	/* Get the proper increments to reach the end point */
	dy = ((y1 < y2) ? 1: (y1 > y2) ? -1: 0);
	dx = ((x1 < x2) ? 1: (x1 > x2) ? -1: 0);

	/* Draw the line */
	while (TRUE)
	{
		/* Stop at the first illegal grid */
		if (!IN_FRACTAL(t_ptr, y1, x1)) break;

		/* Set the new content of the grid */
		map[y1][x1] = content;

		/* We reached the end point? */
		if ((y1 == y2) && (x1 == x2)) break;

		/* Advance one position */
		y1 += dy;
		x1 += dx;
	}
}

/*
 * Places walls in the perimeter of a fractal map
 */
static void fractal_draw_borders(fractal_map map, fractal_template *t_ptr)
{
	int last = t_ptr->size - 1;

	/* Left */
	fractal_draw_line(map, t_ptr, 0, 0, last, 0, FRACTAL_WALL);
	/* Important: Leave some space for tunnels */
	fractal_draw_line(map, t_ptr, 0, 1, last, 1, FRACTAL_WALL);

	/* Right */
	fractal_draw_line(map, t_ptr, 0, last, last, last, FRACTAL_WALL);
	/* Important: Leave some space for tunnels */
	fractal_draw_line(map, t_ptr, 0, last - 1, last, last - 1, FRACTAL_WALL);

	/* Top */
	fractal_draw_line(map, t_ptr, 0, 1, 0, last - 1, FRACTAL_WALL);

	/* Bottom */
	fractal_draw_line(map, t_ptr, last, 1, last, last - 1, FRACTAL_WALL);
}

/*
 * Some fractal templates
 */

/* 17x33 template */
static void fractal1_init_func(fractal_map map, fractal_template *t_ptr)
{
	/* Borders */
	fractal_draw_borders(map, t_ptr);

	/*
	 * Mega-hack -- place walls in the middle of the 33x33 map to generate
	 * a 17x33 map
	 */
	fractal_draw_line(map, t_ptr, 16, 1, 16, 32, FRACTAL_WALL);

	map[8][8] = (!rand_int(15) ? FRACTAL_POOL_1: FRACTAL_FLOOR);
	map[8][16] = (!rand_int(15) ? FRACTAL_POOL_2: FRACTAL_FLOOR);
	map[8][24] = (!rand_int(15) ? FRACTAL_POOL_3: FRACTAL_FLOOR);
}


/* 33x65 template */
static void fractal2_init_func(fractal_map map, fractal_template *t_ptr)
{
	int k;

	/* Borders */
	fractal_draw_borders(map, t_ptr);

	/*
	 * Mega-hack -- place walls in the middle of the 65x65 map to generate
	 * a 33x65 map
	 */
	fractal_draw_line(map, t_ptr, 32, 1, 32, 64, FRACTAL_WALL);

	k = rand_int(100);
	/* 1 in 5 chance to make a pool and 1 in 5 to leave the map untouched */
	if (k < 80) map[8][16] = ((k < 20) ? FRACTAL_POOL_2: FRACTAL_FLOOR);

	k = rand_int(100);
	/* 1 in 4 chance to make a pool and 1 in 4 to leave the map untouched */
	if (k < 75) map[8][32] = ((k < 25) ? FRACTAL_POOL_3: FRACTAL_FLOOR);

	k = rand_int(100);
	/* 1 in 5 chance to make a pool and 1 in 5 to leave the map untouched */
	if (k < 80) map[8][48] = ((k < 20) ? FRACTAL_POOL_1: FRACTAL_FLOOR);

	map[16][16] = (!rand_int(4) ? FRACTAL_POOL_1: FRACTAL_FLOOR);
	map[16][32] = (!rand_int(3) ? FRACTAL_POOL_2: FRACTAL_FLOOR);
	map[16][48] = (!rand_int(4) ? FRACTAL_POOL_3: FRACTAL_FLOOR);

	k = rand_int(100);
	/* 1 in 5 chance to make a pool and 1 in 5 to leave the map untouched */
	if (k < 80) map[24][16] = ((k < 20) ? FRACTAL_POOL_3: FRACTAL_FLOOR);

	k = rand_int(100);
	/* 1 in 4 chance to make a pool and 1 in 4 to leave the map untouched */
	if (k < 75) map[24][32] = ((k < 25) ? FRACTAL_POOL_1: FRACTAL_FLOOR);

	k = rand_int(100);
	/* 1 in 5 chance to make a pool and 1 in 5 to leave the map untouched */
	if (k < 80) map[24][48] = ((k < 20) ? FRACTAL_POOL_2: FRACTAL_FLOOR);
}

/* 9x9 template for pools */
static void fractal3_init_func(fractal_map map, fractal_template *t_ptr)
{
	/*Unused*/
	(void)t_ptr;

	/* Walls in the corners */
	map[0][0] = FRACTAL_WALL;
	map[0][8] = FRACTAL_WALL;
	map[8][0] = FRACTAL_WALL;
	map[8][8] = FRACTAL_WALL;

	map[2][4] = FRACTAL_FLOOR;
	map[4][2] = FRACTAL_FLOOR;
	map[4][4] = FRACTAL_FLOOR;
	map[4][6] = FRACTAL_FLOOR;
	map[6][4] = FRACTAL_FLOOR;
}

/* 17x17 template for pools */
static void fractal4_init_func(fractal_map map, fractal_template *t_ptr)
{
	/*Unused*/
	(void)t_ptr;

	/* Walls in the corners */
	map[0][0] = FRACTAL_WALL;
	map[0][16] = FRACTAL_WALL;
	map[16][0] = FRACTAL_WALL;
	map[16][16] = FRACTAL_WALL;

	map[4][8] = FRACTAL_FLOOR;
	map[8][4] = FRACTAL_FLOOR;
	map[8][8] = FRACTAL_FLOOR;
	map[8][12] = FRACTAL_FLOOR;
	map[12][8] = FRACTAL_FLOOR;
}

/* 33x33 template */
static void fractal5_init_func(fractal_map map, fractal_template *t_ptr)
{
	bool flip_h = !rand_int(2);

	/* Borders */
	fractal_draw_borders(map, t_ptr);

	if (!rand_int(15)) map[8][flip_h ? 24: 8] = FRACTAL_FLOOR;

	map[16][16] = FRACTAL_FLOOR;

	if (!rand_int(15)) map[24][flip_h ? 8: 24] = FRACTAL_FLOOR;
}


/*
 * A list of the available fractal templates.
 */
static fractal_template fractal_repository[] =
{
	{FRACTAL_TYPE_17x33, 33, fractal1_init_func},
	{FRACTAL_TYPE_33x65, 65, fractal2_init_func},
	{FRACTAL_TYPE_9x9, 9, fractal3_init_func},
	{FRACTAL_TYPE_17x17, 17, fractal4_init_func},
	{FRACTAL_TYPE_33x33, 33, fractal5_init_func},
};

/*
 * Wipes the contents of a fractal map and applies the given template.
 */
static void fractal_map_reset(fractal_map map, fractal_template *t_ptr)
{
	int x, y;

	/* Fill the map with FRACTAL_NONE */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			map[y][x] = FRACTAL_NONE;
		}
	}

	/* Call the initialization function to place some floors */
	if (t_ptr->init_func)
	{
		t_ptr->init_func(map, t_ptr);
	}
}


/*
 * Returns a *reset* fractal map allocated in dynamic memory.
 * You must deallocate the map with FREE when it isn't used anymore.
 */
static fractal_map fractal_map_create(fractal_template *t_ptr)
{
	/* The new map */
	fractal_map map;

	/* Allocate the map */
	map = C_ZNEW(t_ptr->size, fractal_map_wid);

	/* Reset the contents of the map */
	fractal_map_reset(map, t_ptr);

	/* Done */
	return (map);
}

/*#define DEBUG_FRACTAL_TEMPLATES 1*/

#ifdef DEBUG_FRACTAL_TEMPLATES

/* This table is used to convert the map grids to printable characters */
static char fractal_grid_to_char[] =
{
	' ',	/* FRACTAL_NONE */
	'#',	/* FRACTAL_WALL */
	'&',	/* FRACTAL_EDGE */
	'.',	/* FRACTAL_FLOOR*/
	'1',	/* FRACTAL_POOL_1*/
	'2',	/* FRACTAL_POOL_2*/
	'3',	/* FRACTAL_POOL_3*/
};

/*
 * Prints a fractal map to stdout. "title" is optional (can be NULL).
 */
static void fractal_map_debug(fractal_map map, fractal_template *t_ptr,
	char *title)
{
	int x, y;
	FILE *fff;
	static bool do_create = TRUE;

	fff = my_fopen("fractal.txt", do_create ? "w": "a");

	do_create = FALSE;

	if (!fff) return;

 	/* Show the optional title */
	if (title)
	{
		fputs(title, fff);

		putc('\n', fff);
	}

	/* Show the map */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			byte grid = map[y][x];
			char chr;

			/* Check for strange grids */
			if (grid >= N_ELEMENTS(fractal_grid_to_char))
			{
				chr = '?';
			}
			/* Translate to printable char */
			else
			{
				chr = fractal_grid_to_char[grid];
			}

			/* Show it */
			putc(chr, fff);
		}

		/* Jump to the next row */
		putc('\n', fff);
	}

	putc('\n', fff);

	/* Done */
	my_fclose(fff);
}

#endif /* DEBUG_FRACTAL_TEMPLATES */

/*
 * Completes a fractal map. The map must have been reset.
 */
static void fractal_map_complete(fractal_map map, fractal_template *t_ptr)
{
	int x, y, x1, y1, x2, y2, cx, cy;
	/*
	 * Set the initial size of the squares. At first, we have only
	 * one big square.
	 */
	int cur_size = t_ptr->size - 1;

	/*
	 * Construct the map using a variable number of iterations.
	 * Each iteration adds more details to the map.
	 * This algorithm is originally recursive but we made it iterative
	 * for efficiency.
	 */
	do
	{
		/* Get the vertical coordinates of the first square */
		y1 = 0;
		y2 = cur_size;

		/*
		 * Process the whole map. Notice the step used: (cur_size / 2)
		 * is the middle point of the current "3x3" square.
		 */
		for (y = 0; y < t_ptr->size; y += (cur_size / 2))
		{
			/* Change to the next "3x3" square, if needed */
			if (y > y2)
			{
				/*
				 * The end of the previous square becomes the
				 * beginning of the new square
				 */
				y1 = y2;

				/* Get the end of the new square */
				y2 += cur_size;
			}

			/* Get the horizontal coordinates of the first square */
			x1 = 0;
			x2 = cur_size;

			/* Notice the step */
			for (x = 0; x < t_ptr->size; x += (cur_size / 2))
			{
				/* Change to the next "3x3" square, if needed */
				if (x > x2)
				{
					/*
					 * The end of the previous square
					 * becomes the beginning of the new
					 * square
					 */
					x1 = x2;

					/* Get the end of the new square */
					x2 += cur_size;
				}

				/* IMPORTANT: ignore already processed grids */
				if (map[y][x] != FRACTAL_NONE) continue;

				/*
				 * Determine if the vertical coordinate of
				 * this grid should be fixed
				 */
				if ((y == y1) || (y == y2)) cy = y;
				/* Pick one *adjacent* corner randomly */
				else cy = ((rand_int(100) < 50) ? y1: y2);

				/*
				 * Determine if the horizontal coordinate of
				 * this grid should be fixed
				 */
				if ((x == x1) || (x == x2)) cx = x;
				/* Pick one *adjacent* corner randomly */
				else cx = ((rand_int(100) < 50) ? x1: x2);

				/* Copy the value of the chosed corner */
				map[y][x] = map[cy][cx];
			}
		}

	/* Decrease the size of the squares for the next iteration */
	cur_size /= 2;

	/* We stop when the squares can't be divided anymore */
	} while (cur_size > 1);
}

/*
 * Verify if all floor grids in a completed fractal map are connected.
 */
static int fractal_map_is_connected(fractal_map map, fractal_template *t_ptr)
{
	int x, y, i, connected = TRUE;
	fractal_map_wid *visited;
	/* Queue of visited grids */
	grid_queue_type queue, *q_ptr = &queue;

	/* Allocate a "visited" matrix */
	visited = C_ZNEW(t_ptr->size, fractal_map_wid);

	/* Create the queue */
	grid_queue_create(q_ptr, 500);

	/* Find a floor grid */
	for (y = 0; (y < t_ptr->size) && GRID_QUEUE_EMPTY(q_ptr); y++)
	{
		for (x = 0; (x < t_ptr->size) && GRID_QUEUE_EMPTY(q_ptr); x++)
		{
			/* Found one */
			if (map[y][x] >= FRACTAL_FLOOR)
			{
				/* Put it on the queue */
				grid_queue_push(q_ptr, y, x);

				/* Mark as visited */
				visited[y][x] = TRUE;
			}
		}
	}

	/* Paranoia. No floor grid was found */
	if (GRID_QUEUE_EMPTY(q_ptr))
	{
		/* Free resources */
		FREE(visited);
		grid_queue_destroy(q_ptr);

		/* Done */
		return (!connected);
	}

	/* Process all reachable floor grids */
	while (!GRID_QUEUE_EMPTY(q_ptr))
	{
		/* Get the visited grid at the front of the queue */
		y = GRID_QUEUE_Y(q_ptr);
		x = GRID_QUEUE_X(q_ptr);

		/* Remove that grid from the queue */
		grid_queue_pop(q_ptr);

		/* Scan all adjacent grids */
		for (i = 0; i < 8; i++)
		{
			/* Get coordinates */
			int yy = y + ddy_ddd[i];
			int xx = x + ddx_ddd[i];

			/* Check bounds */
			if (!IN_FRACTAL(t_ptr, yy, xx)) continue;

			/* Ignore already processed grids */
			if (visited[yy][xx]) continue;

			/* Ignore walls */
			if (map[yy][xx] < FRACTAL_FLOOR) continue;

			/* Append the grid to the queue, if possible */
			if (!grid_queue_push(q_ptr, yy, xx)) continue;

			/* Mark as visited */
			visited[yy][xx] = TRUE;
		}
	}

	/* Find non-visited floor grids */
	for (y = 0; (y < t_ptr->size) && connected; y++)
	{
		for (x = 0; (x < t_ptr->size) && connected; x++)
		{
			/* Check the grid */
			if ((map[y][x] >= FRACTAL_FLOOR) && !visited[y][x])
			{
				/* Found a non-visited floor grid. Done */
				connected = FALSE;
			}
		}
	}

	/* Free resources */
	FREE(visited);
	grid_queue_destroy(q_ptr);

	/* Return answer */
	return connected;
}

/*
 * Places FRACTAL_EDGE walls in a completed fractal map. These grids were
 * created to be replaced by outer walls or other similar features.
 */
static void fractal_map_mark_edge(fractal_map map, fractal_template *t_ptr)
{
	int x, y, i;

	/* Process the whole map */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			/* Ignore wall grids */
			if (map[y][x] < FRACTAL_FLOOR) continue;

			/* Scan adjacent grids */
			for (i = 0; i < 8; i++)
			{
				/* Get coordinates */
				int yy = y + ddx_ddd[i];
				int xx = x + ddy_ddd[i];

				/* Check bounds */
				if (!IN_FRACTAL(t_ptr, yy, xx)) continue;

				/* Turn plain walls to edge walls */
				if (map[yy][xx] == FRACTAL_WALL)
				{
					map[yy][xx] = FRACTAL_EDGE;
				}
			}
		}
	}
}


/*
 * Ensures that the fractal map has at least n_pools differing pool types (at most 3).
 *
 * Ensures that the pools it does have are sequentially numbered from 1 up
 */
static bool fractal_map_has_pools(fractal_map map, fractal_template *t_ptr, int n_pools)
{
	byte pools = 0;
	int x, y;

	/* Basic cases */
	if ((n_pools < 1) && (n_pools > 3)) return (TRUE);

	/* Process the whole map */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			/* Count different pool types */
			if ((map[y][x] >= FRACTAL_POOL_1) && ((pools & (1 << ((map[y][x]) - FRACTAL_POOL_1))) == 0))
			{
				pools |= 1 << ((map[y][x]) - FRACTAL_POOL_1);
				n_pools--;
			}
		}
	}

	/* Check that pools are sequentially numbered */
	if (!pools || (pools == 1) || (pools == 3) || (pools == 7)) return ((n_pools <= 0));

	/* Process the whole map */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			/* Missing pool 2 */
			if ((map[y][x] >= FRACTAL_POOL_3) && (!(pools & 1)))
			{
				map[y][x]--;
			}

			/* Missing pool 1 */
			if ((map[y][x] >= FRACTAL_POOL_2) && (!(pools & 1)))
			{
				map[y][x]--;
			}

		}
	}

	return (TRUE);
}



/*
 * Construct a fractal room given a fractal map and the room center's coordinates.
 */
static void fractal_map_to_room(fractal_map map, byte fractal_type, int y0, int x0, bool light, s16b floor, s16b wall, pool_type pool)
{
	int x, y, y1, x1, wid, hgt;
	int irregular = randint(3)*25;


	/* Get the dimensions of the fractal map */
	hgt = fractal_dim[fractal_type].hgt;
	wid = fractal_dim[fractal_type].wid;

	/* Get top-left coordinate */
	y1 = y0 - hgt / 2;
	x1 = x0 - wid / 2;

	/* Occasional light */
	if (p_ptr->depth <= randint(25)) light = TRUE;

	/* Apply the map to the dungeon */
	for (y = 0; y < hgt; y++)
	{
		for (x = 0; x < wid; x++)
		{
			byte grid_type = map[y][x];
			/* Translate to dungeon coordinates */
			int yy = y1 + y;
			int xx = x1 + x;

			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Translate each grid type to dungeon features */
			if (grid_type >= FRACTAL_FLOOR)
			{
				s16b feat = FEAT_NONE;

				/* Pool grid */
				if (grid_type == FRACTAL_POOL_1)
				{
					/* Use the pool feature */
					feat = pool[0];
				}
				/* Pool grid */
				else if (grid_type == FRACTAL_POOL_2)
				{
					/* Use the pool feature */
					feat = pool[1];
				}
				/* Pool grid */
				else if (grid_type == FRACTAL_POOL_3)
				{
					/* Use the pool feature */
					feat = pool[2];
				}

				/* Place the selected pool feature */
				if (feat != FEAT_NONE)
				{
					int tmp_feat = feat;

					/* Hack -- make variable terrain surrounded by floors */
					if (((f_info[cave_feat[yy][xx]].flags1 & (FF1_WALL)) != 0) &&
						(variable_terrain(&tmp_feat,feat))) cave_set_feat(yy,xx,FEAT_FLOOR);

					build_terrain(yy, xx, tmp_feat);
				}
				/* Or place a floor */
				else
				{
					int tmp_feat = floor;

					/* Hack -- make variable terrain surrounded by floors */
					if (((f_info[cave_feat[yy][xx]].flags1 & (FF1_WALL)) != 0) &&
						(variable_terrain(&tmp_feat,floor))) cave_set_feat(yy,xx,FEAT_FLOOR);

					build_terrain(yy, xx, tmp_feat);
				}

				/* Mark the grid as a part of the room */
				cave_info[yy][xx] |= (CAVE_ROOM);
			}
			else if ((grid_type == FRACTAL_EDGE) && (wall))
			{
				/* Hack -- irregular rooms */
				if (rand_int(100) < irregular) cave_set_feat(yy, xx, FEAT_WALL_OUTER);
				
				/* Set the wall */
				else cave_set_feat(yy, xx, wall);

				/* Mark the grid as a part of the room */
				if (f_info[cave_feat[yy][xx]].flags1 & (FF1_OUTER)) cave_info[yy][xx] |= (CAVE_ROOM);

				/* Allow light to spill out of rooms through transparent edges */
				if (light)
				{
					/* Allow lighting up rooms to work correctly */
					if (f_info[cave_feat[yy][xx]].flags1 & (FF1_LOS))
					{
						int d;

						/* Look in all directions. */
						for (d = 0; d < 8; d++)
						{
							/* Extract adjacent location */
							int yyy = yy + ddy_ddd[d];
							int xxx = xx + ddx_ddd[d];

							/* Hack -- light up outside room */
							cave_info[yyy][xxx] |= (CAVE_GLOW);
						}
					}
				}
			}
			else if ((grid_type == FRACTAL_EDGE) && !(wall) && (light))
			{
				/* Hack -- light up outside room */
				cave_info[yy][xx] |= (CAVE_GLOW);
			}
			else
			{
				continue;
			}

			/* Light the feature if needed */
			if (light)
			{
				cave_info[yy][xx] |= (CAVE_GLOW);
			}

 			/* Or turn off the lights */
			else if (grid_type != FRACTAL_EDGE)
			{
				cave_info[yy][xx] &= ~(CAVE_GLOW);
			}
		}
	}
}

/*
 * Creates a fractal map given a template and copy part of it in the given map
 */
static void fractal_map_merge_another(fractal_map map, fractal_template *t_ptr)
{
	int y, x;

	fractal_map map2;

	/* Create the map */
	map2 = fractal_map_create(t_ptr);

	/* Complete it */
	fractal_map_complete(map2, t_ptr);

	/* Merge the maps */
	for (y = 0; y < t_ptr->size; y++)
	{
		for (x = 0; x < t_ptr->size; x++)
		{
			/* Sometimes we overwrite a grid in the original map */
			if ((map[y][x] != map2[y][x]) && !rand_int(4)) map[y][x] = map2[y][x];
		}
	}

	/* Free resources */
	FREE(map2);
}


/*
 * Build a fractal room given its center. Returns TRUE on success.
 */
static bool build_type_fractal(int room, int chart, int y0, int x0, byte type, bool light)
{
	fractal_map map;
	fractal_template *t_ptr;
	int tries;
	bool do_merge = FALSE;

	/* Default floor and edge */
	s16b feat = FEAT_FLOOR;
	s16b edge = FEAT_WALL_OUTER;
	s16b inner = FEAT_NONE;
	s16b alloc = FEAT_NONE;
	u32b exclude = (RG1_INNER | RG1_STARBURST);

	pool_type pool;

	int n_pools = 0;

	int i;

	/* Paranoia */
	if (type >= MAX_FRACTAL_TYPES) return (FALSE);

	/* Set irregular room info */
	set_irregular_room_info(room, chart, light, exclude, &feat, &edge, &inner, &alloc, &pool, &n_pools);

	/* Clear remaining pools */
	for (i = n_pools; i < 3; i++)
	{
		if (room_info[room].flags & (ROOM_FLOODED))
		{
			pool[i] = pick_proper_feature(cave_feat_pool);
		}
		else
		{
			pool[i] = FEAT_NONE;
		}

		/* Mark room if feat is not passable */
		if ((f_info[pool[i]].flags1 & (FF1_MOVE)) == 0)
		{
			/* Bridge into room */
			room_info[room].flags |= (ROOM_BRIDGED);
		}
	}

	/* Mark room if feat is not passable */
	if ((f_info[feat].flags1 & (FF1_MOVE)) == 0)
	{
		/* Bridge into room */
		room_info[room].flags |= (ROOM_BRIDGED);
	}

	/* Mark room if edge is not 'outer' */
	if ((f_info[edge].flags1 & (FF1_OUTER)) == 0)
	{
		/* Bridge into room */
		room_info[room].flags |= (ROOM_EDGED);
	}

	/* Reset the loop counter */
	tries = 0;

	/* Get a fractal template */
	while (TRUE)
	{
		/* Get a template */
		int which = rand_int(N_ELEMENTS(fractal_repository));

		t_ptr = &fractal_repository[which];

		/* Check if the type matches the wanted one */
		if (t_ptr->type == type) break;

		/* Avoid infinite loops */
		if (++tries >= 100) return (FALSE);
	}

	/* Create and reset the fractal map */
	map = fractal_map_create(t_ptr);

#ifdef DEBUG_FRACTAL_TEMPLATES
	/* Show the template to the developer */
	fractal_map_debug(map, t_ptr, "Fractal");
#endif

	/* Make medium fractal rooms more exotic sometimes */
	if ((type == FRACTAL_TYPE_33x33) && rand_int(3)) do_merge = TRUE;

	/* Reset the loop counter */
	tries = 0;

	/* Construct the fractal map */
	while (TRUE)
	{
		/* Complete the map */
		fractal_map_complete(map, t_ptr);

		/* Put another room on top of this one if necessary */
		if (do_merge) fractal_map_merge_another(map, t_ptr);

		/* Accept only connected maps */
		if ((fractal_map_is_connected(map, t_ptr)) && (fractal_map_has_pools(map, t_ptr, n_pools))) break;

		/* Avoid infinite loops */
		if (++tries >= 100)
		{
			/* Free resources */
			FREE(map);

			/* Failure */
			return (FALSE);
		}

		/* Reset the map. Try again */
		fractal_map_reset(map, t_ptr);
	}

	/* Get edge information */
	fractal_map_mark_edge(map, t_ptr);

	/* Place the room */
	fractal_map_to_room(map, type, y0, x0, light, feat, edge, pool);

	/* Free resources */
	FREE(map);

	/* Hack -- place feature at centre */
	if (alloc)
	{
		alloc = pick_room_feat(alloc);

		cave_set_feat(y0, x0, alloc);

		for (i = 0; i < 8; i++)
		{
			int y = y0 + ddy_ddd[i];
			int x = x0 + ddx_ddd[i];

			/* Don't overwrite anything interesting */
			if ((cave_feat[y][x] != FEAT_FLOOR) && (cave_feat[y][x] != FEAT_WALL_OUTER) && (cave_feat[y][x] != FEAT_WALL_SOLID)) continue;

			if (rand_int(100) < (i >= 4 ? 40 : 20)) continue;
			cave_set_feat(y, x, alloc);
		}
	}

	/* Success */
	return (TRUE);
}


/*
 * Build a pool in a room given the center of the pool and a feature.
 * Outer and solid walls, and permanent features are unnafected.
 * Returns TRUE on success.
 */
static bool build_pool(int y0, int x0, int feat, bool do_big_pool)
{
	byte type;
	int wid, hgt;
	int x, y, x1, y1;
	fractal_map map;
	fractal_template *t_ptr;

	/* Paranoia */
	if (!feat) return (FALSE);

	/* Set some basic info */
	if (do_big_pool)
	{
		type = FRACTAL_TYPE_17x17;
	}
	else
	{
		type = FRACTAL_TYPE_9x9;
	}

	/* Get the dimensions of the fractal map */
	hgt = fractal_dim[type].hgt;
	wid = fractal_dim[type].wid;

	/* Get the top-left grid of the pool */
	y1 = y0 - hgt / 2;
	x1 = x0 - wid / 2;

	/* Choose a template for the pool */
	while (TRUE)
	{
		/* Pick a random template */
		int which = rand_int(N_ELEMENTS(fractal_repository));

		t_ptr = &fractal_repository[which];

		/* Found the desired template type? */
		if (t_ptr->type == type) break;
	}

	/* Create and reset the fractal map */
	map = fractal_map_create(t_ptr);

	/* Complete the map */
	fractal_map_complete(map, t_ptr);

	/* Copy the map into the dungeon */
	for (y = 0; y < hgt; y++)
	{
		for (x = 0; x < wid; x++)
		{
			/* Translate map coordinates to dungeon coordinates */
			int yy = y1 + y;
			int xx = x1 + x;

			/* Ignore non-floors grid types in the map */
			if (map[y][x] != FRACTAL_FLOOR) continue;

			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* A pool must be inside a room */
			if (!(cave_info[yy][xx] & (CAVE_ROOM))) continue;

			/* Ignore anti-teleport grids */
			if (room_has_flag(yy, xx, ROOM_ICKY)) continue;

			/* Ignore some walls, and permanent features */
			if ((f_info[cave_feat[yy][xx]].flags1 & (FF1_OUTER |
				FF1_SOLID | FF1_PERMANENT)) != 0) continue;

			/* Set the feature */
			build_terrain(yy, xx, feat);
		}
	}

	/* Free resources */
	FREE(map);

	/* Success */
	return (TRUE);
}


static void build_type_starburst(int room, int type, int y0, int x0, int dy, int dx, bool light)
{
	bool want_pools = (rand_int(150) < p_ptr->depth);

	bool giant_room = (dy >= 19) && (dx >= 33);

	/* Default terrain */
	s16b feat = FEAT_FLOOR;
	s16b edge = FEAT_WALL_OUTER;
	s16b inner = FEAT_NONE;
	s16b alloc = FEAT_NONE;
	s16b pool[3];

	int n_pools = 0;

	/* Default flags, classic rooms */
	u32b flag = (room_info[room].flags & (ROOM_FLOODED)) != 0 ? 0L : (STAR_BURST_RAW_EDGE);

	/* Set irregular room info */
	set_irregular_room_info(room, type, light, 0L, &feat, &edge, &inner, &alloc, &pool, &n_pools);

	/* Have pool contents */
	if (n_pools) want_pools = TRUE;

	/* Occasional light */
	if (light) flag |= (STAR_BURST_LIGHT);

	/* Mark as room */
	if (room) flag |= (STAR_BURST_ROOM);

	/* No floor - need inner room */
	if ((f_info[feat].flags1 & (FF1_MOVE)) == 0)
	{
		/* Floor */
		if (!inner) inner = FEAT_FLOOR;

		/* Hack -- no longer a room - but note we add flag later for 'inner' room */
		flag &= ~(STAR_BURST_ROOM);
	}

	/* Mark room if feat is not passable */
	if ((f_info[feat].flags1 & (FF1_MOVE)) == 0)
	{
		/* Bridge into room */
		room_info[room].flags |= (ROOM_BRIDGED);
	}

	/* Mark room if edge is not 'outer' */
	if ((edge) && ((f_info[edge].flags1 & (FF1_MOVE)) == 0))
	{
		/* Bridge into room */
		room_info[room].flags |= (ROOM_EDGED);
	}

	/* Case 1. Plain starburst room */
	if (!(inner) && (rand_int(100) < 75))
	{
		/* Allow cloverleaf rooms if pools are disabled */
		if (!want_pools) flag |= (STAR_BURST_CLOVER);

		generate_starburst_room (y0 - dy, x0 - dx, y0 + dy, x0 + dx,
			feat, edge, flag);
	}
	/* Case 2. Add an inner room */
	else
	{
		/* Note no cloverleaf room */
		generate_starburst_room (y0 - dy, x0 - dx, y0 + dy, x0 + dx,
			feat, edge, flag);

		/* Special case. Create a solid wall formation */
		if ((inner) || (!rand_int(2)))
		{
			if (!inner) inner = edge;

			/* Classic inner room (unless whole thing is not a room) */
			if (((f_info[inner].flags1 & (FF1_OUTER)) != 0) && ((room) || (flag & STAR_BURST_ROOM)))
			{
				feat = feat_state(inner, FS_INNER);
			}
			else
			{
				feat = inner;
			}

			/* No edge */
			edge = FEAT_NONE;
		}

		/* Adjust the size of the inner room */
		if ((f_info[edge].flags1 & (FF1_WALL)) != 0)
		{
			dy /= 4;
			dx /= 4;
		}
		else
		{
			dy /= 3;
			dx /= 3;
		}

		/* Minimum size */
		if (dy < 1) dy = 1;
		if (dx < 1) dx = 1;

		generate_starburst_room (y0 - dy, x0 - dx, y0 + dy, x0 + dx,
			feat, edge, flag | (room ? STAR_BURST_ROOM : 0L));
	}

	/* Build pools */
	if (want_pools)
	{
		int i, range;

		/* Haven't allocated pools */
		if (!n_pools)
		{
			/* Randomize the number of pools */
			n_pools = randint(2);

			/* Adjust for giant rooms */
			if (giant_room) n_pools += 1;

			/* Place the pools */
			for (i = 0; i < n_pools; i++)
			{
				/* Choose a feature for the pool */
				pool[i] = pick_proper_feature(cave_feat_pool);

				/* Got none */
				if (!pool[i])
				{
					i--;
					n_pools--;
					continue;
				}
			}
		}

		/* How far of room center? */
		range = giant_room ? 12: 5;

		/* Force the selection of a new feature */
		feat = FEAT_NONE;

		/* Place the pools */
		for (i = 0; i < n_pools; i++)
		{
			int tries;

			for (tries = 0; tries < 2500; tries++)
			{
				/* Get the center of the pool */
				int y = rand_spread(y0, range);
				int x = rand_spread(x0, range);

				/* Verify center */
				if (cave_feat[y][x] == feat)
				{
					build_pool(y, x, pool[i], giant_room);

					/* Mark room */
					if ((f_info[pool[i]].flags1 & (FF1_MOVE)) == 0) room_info[room].flags |= (ROOM_BRIDGED);

					/* Done */
					break;
				}
			}
		}
	}

	/* Hack -- place feature at centre */
	if (alloc)
	{
		int i;

		alloc = pick_room_feat(alloc);

		cave_set_feat(y0, x0, alloc);

		if ((dy > 1) && (dx > 1)) for (i = 0; i < 8; i++)
		{
			int y = y0 + ddy_ddd[i];
			int x = x0 + ddx_ddd[i];

			/* Don't overwrite anything interesting */
			if ((cave_feat[y][x] != FEAT_FLOOR) && (cave_feat[y][x] != FEAT_WALL_OUTER) && (cave_feat[y][x] != FEAT_WALL_SOLID)) continue;

			if (rand_int(100) < (i >= 4 ? 40 : 20)) continue;
			cave_set_feat(y, x, alloc);
		}
	}
}




/*
 * Helper function to build chambers.  Fill a room matching
 * the rectangle input with a 'wall type', and surround it with inner wall.
 * Create a door in a random inner wall grid along the border of the
 * rectangle.
 *
 * XXX We may replace FEAT_MAGMA in Leon's original algorithm with a specified
 * 'wall feature'.
 */
static void make_chamber(int c_y1, int c_x1, int c_y2, int c_x2)
{
	int i, d, y, x;
	int count;

	/* Fill with soft granite (will later be replaced with floor). */
	generate_fill(c_y1+1, c_x1+1, c_y2-1, c_x2-1, FEAT_MAGMA);

	/* Generate inner walls over dungeon granite and magma. */
	for (y = c_y1; y <= c_y2; y++)
	{
		/* left wall */
		x = c_x1;

		if ((cave_feat[y][x] == FEAT_WALL_EXTRA) ||
			(cave_feat[y][x] == FEAT_MAGMA))
			cave_set_feat(y, x, FEAT_WALL_INNER);
	}

	for (y = c_y1; y <= c_y2; y++)
	{
		/* right wall */
		x = c_x2;

		if ((cave_feat[y][x] == FEAT_WALL_EXTRA) ||
			(cave_feat[y][x] == FEAT_MAGMA))
			cave_set_feat(y, x, FEAT_WALL_INNER);
	}

	for (x = c_x1; x <= c_x2; x++)
	{
		/* top wall */
		y = c_y1;

		if ((cave_feat[y][x] == FEAT_WALL_EXTRA) ||
			(cave_feat[y][x] == FEAT_MAGMA))
			cave_set_feat(y, x, FEAT_WALL_INNER);
	}

	for (x = c_x1; x <= c_x2; x++)
	{
		/* bottom wall */
		y = c_y2;

		if ((cave_feat[y][x] == FEAT_WALL_EXTRA) ||
			(cave_feat[y][x] == FEAT_MAGMA))
			cave_set_feat(y, x, FEAT_WALL_INNER);
	}

	/* Try a few times to place a door. */
	for (i = 0; i < 20; i++)
	{
		/* Pick a square along the edge, not a corner. */
		if (!rand_int(2))
		{
			/* Somewhere along the (interior) side walls. */
			if (!rand_int(2)) x = c_x1;
			else x = c_x2;
			y = c_y1 + rand_int(1 + ABS(c_y2 - c_y1));
		}
		else
		{
			/* Somewhere along the (interior) top and bottom walls. */
			if (!rand_int(2)) y = c_y1;
			else y = c_y2;
			x = c_x1 + rand_int(1 + ABS(c_x2 - c_x1));
		}

		/* If not an inner wall square, try again. */
		if (cave_feat[y][x] != FEAT_WALL_INNER) continue;

		/* Paranoia */
		if (!in_bounds_fully(y, x)) continue;

		/* Reset wall count */
		count = 0;

		/* If square has not more than two adjacent walls, and no adjacent doors, place door. */
		for (d = 0; d < 9; d++)
		{
			/* Extract adjacent (legal) location */
			int yy = y + ddy_ddd[d];
			int xx = x + ddx_ddd[d];

			/* No doors beside doors. */
			if (cave_feat[yy][xx] == FEAT_OPEN) break;

			/* Count the inner walls. */
			if (cave_feat[yy][xx] == FEAT_WALL_INNER) count++;

			/* No more than two walls adjacent (plus the one we're on). */
			if (count > 3) break;

			/* Checked every direction? */
			if (d == 8)
			{
				/* Place an open door. */
				cave_set_feat(y, x, FEAT_OPEN);

				/* Success. */
				return;
			}
		}
	}
}



/*
 * Expand in every direction from a start point, turning magma into rooms.
 * Stop only when the magma and the open doors totally run out.
 */
static void hollow_out_room(int y, int x)
{
	int d, yy, xx;

	for (d = 0; d < 9; d++)
	{
		/* Extract adjacent location */
		yy = y + ddy_ddd[d];
		xx = x + ddx_ddd[d];

		/* Change magma to floor. */
		if (cave_feat[yy][xx] == FEAT_MAGMA)
		{
			cave_set_feat(yy, xx, FEAT_FLOOR);

			/* Hollow out the room. */
			hollow_out_room(yy, xx);
		}
		/* Change open door to broken door. */
		else if (cave_feat[yy][xx] == FEAT_OPEN)
		{
			cave_set_feat(yy, xx, FEAT_BROKEN);

			/* Hollow out the (new) room. */
			hollow_out_room(yy, xx);
		}
	}
}



/*
 * Build chambers
 *
 * Build a room, varying in size between 22x22 and 44x66, consisting of
 * many smaller, irregularly placed, chambers all connected by doors or
 * short tunnels. -LM-
 *
 * Plop down an area-dependent number of magma-filled chambers, and remove
 * blind doors and tiny rooms.
 *
 * Hollow out a chamber near the center, connect it to new chambers, and
 * hollow them out in turn.  Continue in this fashion until there are no
 * remaining chambers within two squares of any cleared chamber.
 *
 * Clean up doors.  Neaten up the wall types.  Turn floor grids into rooms,
 * illuminate if requested.
 *
 * Fill the room with up to 35 (sometimes up to 50) monsters of a creature
 * race or type that offers a challenge at the character's depth.  This is
 * similar to monster pits, except that we choose among a wider range of
 * monsters.
 *
 * Special quest levels modify some of these steps.
 */
static bool build_chambers(int y1, int x1, int y2, int x2, int monsters_left, bool light)
{
	int i;
	int d;
	int area, num_chambers;
	int y, x, yy, xx;
	int yy1, xx1, yy2, xx2, yy3, xx3;

	int count;

	/* Determine how much space we have. */
	area = ABS(y2 - y1) * ABS(x2 - x1);

	/* Calculate the number of smaller chambers to make. */
	num_chambers = 10 + area / 80;

	/* Build the chambers. */
	for (i = 0; i < num_chambers; i++)
	{
		int c_y1, c_x1, c_y2, c_x2;
		int size, w, h;

		/* Determine size of chamber. */
		size = 3 + rand_int(4);
		w = size + rand_int(10);
		h = size + rand_int(4);

		/* Pick an upper-left corner at random. */
		c_y1 = rand_range(y1, y2 - h);
		c_x1 = rand_range(x1, x2 - w);

		/* Determine lower-right corner of chamber. */
		c_y2 = c_y1 + h;
		c_x2 = c_x1 + w;

		/* Make me a (magma filled) chamber. */
		make_chamber(c_y1, c_x1, c_y2, c_x2);
	}

	/* Remove useless doors, fill in tiny, narrow rooms. */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			count = 0;

			/* Stay legal. */
			if (!in_bounds_fully(y, x)) continue;

			/* Check all adjacent grids. */
			for (d = 0; d < 8; d++)
			{
				/* Extract adjacent location */
				yy = y + ddy_ddd[d];
				xx = x + ddx_ddd[d];

				/* Count the walls and dungeon granite. */
				if ((cave_feat[yy][xx] == FEAT_WALL_INNER) ||
					(cave_feat[yy][xx] == FEAT_WALL_EXTRA)) count++;
			}

			/* Five adjacent walls:  Change non-chamber to wall. */
			if ((count == 5) && (cave_feat[y][x] != FEAT_MAGMA))
				cave_set_feat(y, x, FEAT_WALL_INNER);

			/* More than five adjacent walls:  Change anything to wall. */
			else if (count > 5) cave_set_feat(y, x, FEAT_WALL_INNER);
		}
	}

	/* Pick a random magma spot near the center of the room. */
	for (i = 0; i < 50; i++)
	{
		y = y1 + rand_spread((y2 - y1) / 2, (y2 - y1) / 4);
		x = x1 + rand_spread((x2 - x1) / 2, (x2 - x1) / 4);
		if (cave_feat[y][x] == FEAT_MAGMA) break;
	}

	/* Hollow out the first room. */
	cave_set_feat(y, x, FEAT_FLOOR);
	hollow_out_room(y, x);


	/* Attempt to change every in-room magma grid to open floor. */
	for (i = 0; i < 100; i++)
	{
		/* Assume this run will do no useful work. */
		bool joy = FALSE;

		/* Make new doors and tunnels between magma and open floor. */
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				/* Current grid must be magma. */
				if (cave_feat[y][x] != FEAT_MAGMA) continue;

				/* Stay legal. */
				if (!in_bounds_fully(y, x)) continue;

				/* Check only horizontal and vertical directions. */
				for (d = 0; d < 4; d++)
				{
					/* Extract adjacent location */
					yy1 = y + ddy_ddd[d];
					xx1 = x + ddx_ddd[d];

					/* Find inner wall. */
					if (cave_feat[yy1][xx1] == FEAT_WALL_INNER)
					{
						/* Keep going in the same direction. */
						yy2 = yy1 + ddy_ddd[d];
						xx2 = xx1 + ddx_ddd[d];

						/* If we find open floor, place a door. */
						if ((in_bounds(yy2, xx2)) && (cave_feat[yy2][xx2] == FEAT_FLOOR))
						{
							joy = TRUE;

							/* Make a broken door in the wall grid. */
							cave_set_feat(yy1, xx1, FEAT_BROKEN);

							/* Hollow out the new room. */
							cave_set_feat(y, x, FEAT_FLOOR);
							hollow_out_room(y, x);

							break;
						}

						/* If we find more inner wall... */
						if ((in_bounds(yy2, xx2)) && (cave_feat[yy2][xx2] == FEAT_WALL_INNER))
						{
							/* ...Keep going in the same direction. */
							yy3 = yy2 + ddy_ddd[d];
							xx3 = xx2 + ddx_ddd[d];

							/* If we /now/ find floor, make a tunnel. */
							if ((in_bounds(yy3, xx3)) && (cave_feat[yy3][xx3] == FEAT_FLOOR))
							{
								joy = TRUE;

								/* Turn both wall grids into floor. */
								cave_set_feat(yy1, xx1, FEAT_FLOOR);
								cave_set_feat(yy2, xx2, FEAT_FLOOR);

								/* Hollow out the new room. */
								cave_set_feat(y, x, FEAT_FLOOR);
								hollow_out_room(y, x);

								break;
							}
						}
					}
				}
			}
		}

		/* If we could find no work to do, stop. */
		if (!joy) break;
	}


	/* Turn broken doors into a random kind of door, remove open doors. */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (cave_feat[y][x] == FEAT_OPEN)
				cave_set_feat(y, x, FEAT_WALL_INNER);
			else if (cave_feat[y][x] == FEAT_BROKEN)
				place_random_door(y, x);
		}
	}


	/* Turn all walls and magma not adjacent to floor into dungeon granite. */
	/* Turn all floors and adjacent grids into rooms, sometimes lighting them. */
	for (y = (y1-1 > 0 ? y1-1 : 0) ;
		y < (y2+2 < DUNGEON_HGT ? y2+2 : DUNGEON_HGT) ; y++)
	{
		for (x = (x1-1 > 0 ? x1-1 : 0) ;
			x < (x2+2 < DUNGEON_WID ? x2+2 : DUNGEON_WID) ; x++)
		{
			if ((cave_feat[y][x] == FEAT_WALL_INNER) ||
				(cave_feat[y][x] == FEAT_MAGMA))
			{
				for (d = 0; d < 9; d++)
				{
					/* Extract adjacent location */
					yy = y + ddy_ddd[d];
					xx = x + ddx_ddd[d];

					/* Stay legal */
					if (!in_bounds(yy, xx)) continue;

					/* No floors allowed */
					if (cave_feat[yy][xx] == FEAT_FLOOR) break;

					/* Turn me into dungeon granite. */
					if (d == 8)
					{
						cave_set_feat(y, x, FEAT_WALL_EXTRA);
					}
				}
			}
			if (cave_feat[y][x] == FEAT_FLOOR)
			{
				for (d = 0; d < 9; d++)
				{
					/* Extract adjacent location */
					yy = y + ddy_ddd[d];
					xx = x + ddx_ddd[d];

					/* Stay legal */
					if (!in_bounds(yy, xx)) continue;

					/* Turn into room. */
					cave_info[yy][xx] |= (CAVE_ROOM);

					/* Illuminate if requested. */
					if (light) cave_info[yy][xx] |= (CAVE_GLOW);
				}
			}
		}
	}


	/* Turn all inner wall grids adjacent to dungeon granite into outer walls. */
	for (y = (y1-1 > 0 ? y1-1 : 0) ; y < (y2+2 < DUNGEON_HGT ? y2+2 : DUNGEON_HGT) ; y++)
	{
		for (x = (x1-1 > 0 ? x1-1 : 0) ; x < (x2+2 < DUNGEON_WID ? x2+2 : DUNGEON_WID) ; x++)
		{
			/* Stay legal. */
			if (!in_bounds_fully(y, x)) continue;

			if (cave_feat[y][x] == FEAT_WALL_INNER)
			{
				for (d = 0; d < 9; d++)
				{
					/* Extract adjacent location */
					yy = y + ddy_ddd[d];
					xx = x + ddx_ddd[d];

					/* Look for dungeon granite */
					if (cave_feat[yy][xx] == FEAT_WALL_EXTRA)
					{
						/* Turn me into outer wall. */
						cave_set_feat(y, x, FEAT_WALL_OUTER);

						/* Done */
						break;
					}
				}
			}
		}
	}

	/*** Now we get to place the monsters. ***/

	/* Place the monsters. */
	for (i = 0; i < 300; i++)
	{
		/* Check for early completion. */
		if (!monsters_left) break;

		/* Pick a random in-room square. */
		y = rand_range(y1, y2);
		x = rand_range(x1, x2);

		/* Require a floor square with no monster in it already. */
		if (!cave_naked_bold(y, x)) continue;

		/* Enforce monster selection */
		cave_ecology.use_ecology = room_info[room_idx_ignore_valid(y, x)].ecology;

		/* Place a single monster.  Sleeping 2/3rds of the time. */
		place_monster_aux(y, x, get_mon_num(monster_level),
			(bool)(rand_int(3)), FALSE, 0L);

		/* End enforcement of monster selection */
		cave_ecology.use_ecology = (0L);

		/* One less monster to place. */
		monsters_left--;
	}

	/* Success */
	return (TRUE);
}

#if 0
/*
 * This builds a simple building in a monster town.
 */
static bool build_simple_building(int by, int bx, int floor, int wall)
{
	/* Do we need a door? */
	bool need_door = ((f_info[wall].flags1 & (FF1_MOVE)) == 0) && ((f_info[wall].flags3 & (FF3_EASY_CLIMB)) == 0);
	
	/* Occasionaly light */
	bool light = (p_ptr->depth <= randint(25));
	
	/* Rare pillars except on crypts */
	bool pillars = ((level_flag & (LF1_CRYPT)) != 0) || (rand_int(20) == 0);
	
	/* Simple buildings always have locked doors */
	int door = FEAT_DOOR_CLOSED + randint(1 + p_ptr->depth / 10);
	
	int y, x;
	
	/* Build polygon room */
	if (level_flag & (LF1_POLYGON))
	{
		/* Edge coordinates */
		int verty[5];
		int vertx[5];
		
		/* Place vertices */
		verty[0] = by * BLOCK_HGT + 1;
		verty[1] = by * BLOCK_HGT + 1 + randint(BLOCK_HGT - 2);
		verty[2] = (by + 1) * BLOCK_HGT - 2;
		verty[3] = by * BLOCK_HGT + 1 + randint(BLOCK_HGT - 2);
		verty[4] = 0;
		
		vertx[0] = bx * BLOCK_WID + 1 + randint(BLOCK_WID - 2);
		vertx[1] = (bx + 1) * BLOCK_WID - 2;
		vertx[2] = bx * BLOCK_WID + 1 + randint(BLOCK_WID - 2);
		vertx[3] = bx * BLOCK_WID + 1;
		vertx[4] = 0;
		
		/* Generate room */
		if (generate_poly_room(5, verty, vertx, wall, floor, wall, 0, (CAVE_ROOM) | (light ? (CAVE_LITE) : 0)))
		{
			int i = 0;
			
			/* Hack - Place doorway. Try 50 times or no door! */
			if (need_door) while (i++ < 50)
			{
				y = by * BLOCK_HGT + 1 + rand_int(BLOCK_HGT - 2);
				x = bx * BLOCK_WID + 1 + randint(BLOCK_WID - 2);
				
				if (cave_feat[y][x] == wall)
				{
					cave_set_feat(y, x, door);
					
					break;
				}
			}
			
			return (TRUE);
		}
	}
	/* Build normal room */
	else
	{
		/* Room coordinates */
		int y1 = randint(4);
		int y2 = randint(4);
		int x1 = randint(4);
		int x2 = randint(4);
		
		/* And recentre */
		y1 = by * BLOCK_HGT + 5 - y1;
		y2 = by * BLOCK_HGT + 6 + y2;
		x1 = bx * BLOCK_WID + 5 - x1;
		x2 = bx * BLOCK_WID + 6 + x2;
		
		/* Generate new room (a) */
		generate_room(y1, x1, y2, x2, light);

		/* Generate outer walls (a) */
		generate_rect(y1, x1, y2, x2, wall);

		/* Make corners solid */
		cave_set_feat(y1, x1, FEAT_WALL_SOLID);
		cave_set_feat(y2, x2, FEAT_WALL_SOLID);
		cave_set_feat(y1, x2, FEAT_WALL_SOLID);
		cave_set_feat(y2, x1, FEAT_WALL_SOLID);

		/* Generate inner floors (a) */
		generate_fill(y1+1, x1+1, y2-1, x2-1, FEAT_FLOOR);

		/* Generate inner floors (a) */
		generate_fill_pillars(y1+1, x1+1, y2-1, x2-1, floor, pillars ? 2: 0, 1);
				
		/* Place locked doors */
		if (need_door) generate_door(y1, x1, y2, x2, rand_int(4), door);
		
		/* Place second door door - note we ensure both are locked with equal difficulty */
		if ((need_door) && (rand_int(100) < 33))
		{
			/* Place back door */
			generate_door(y1, x1, y2, x2, rand_int(4), door);
		}
		
		return (TRUE);
	}
	
	return (FALSE);
}


/*
 * Build a monster town.
 * 
 * In each 11x11 block (by, bx) which the town fully occupies:
 * 
 * 1. If the dun_room[by][bx] == base room, we either
 *   a. place a simple room. up to 9x9 in size with a width 1
 * border and a door
 *   b. clear the dun->room_map[by][bx] value (= FALSE) for
 * certain dungeon types only (those that place only rooms which
 * are permitted on LF1_WILD locations)
 *   c. if either the trailing or leading block is empty, and
 * we have free rooms to place, we place an overlapping room up
 * to 20x9 in size with a width 1 border
 * 2. If the dun_room[by][bx] != base_room, we skip this cell
 * 
 */
static bool generate_town(int y1, int x1, int y2, int x2, int base_room, s16b ground, s16b wall, s16b floor, bool light)
{
	int by, bx;
	int y, x;
	
	int rooms;
	
	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);
	
	/* Fix borders so they are block aligned */
	if (y1 % BLOCK_HGT) y1+= (y1 % BLOCK_HGT);
	if (y2 % BLOCK_HGT) y2-= (y1 % BLOCK_HGT);
	if (x1 % BLOCK_WID) x1+= (y1 % BLOCK_WID);
	if (x2 % BLOCK_WID) x2-= (y1 % BLOCK_WID);

	/* Paranoia */
	if ((y1 >= y2) || (x1 >= x2)) return (FALSE);

	/* Expected number of rooms */
	rooms = ((y2-y1) / BLOCK_HGT) * ((x2 - x1) / BLOCK_WID);
	
	/* Minimum number of rooms allowed, 1 for 1, 2 for 2, 2 for 3 etc. */
	rooms = (rooms + 2) / 2;
	
	/* Check each block */
	for (by = (y1 / BLOCK_HGT); by * BLOCK_HGT < y2; by++)
	{
		for (bx = (x1 / BLOCK_WID); bx * BLOCK_WID < x2; bx++)
		{
			/* Handle pinching and holes. Don't worry about decreasing room counter. */
			if (dun->room_map[by][bx] == FALSE) continue;
			
			/* Clear the grid and light it if requested*/
			for (y = by * BLOCK_HGT; y < (by + 1) * BLOCK_HGT; y++)
			{
				for (x = bx * BLOCK_WID; x < (bx + 1) * BLOCK_HGT; x++)
				{
					/* Be careful not to overwrite rooms */
					if ((dun_room[by][bx] != base_room) && (cave_info[y][x] & (CAVE_ROOM))) continue;
					
					/* Hack -- only overwrite 'base' terrain */
					if (cave_feat[y][x] == zone->base) cave_set_feat(y, x, ground);
					
					/* Light the grid */
					if (light) cave_feat[y][x] |= (CAVE_GLOW);
				}
			}
			
			/* Make a choice */
			switch (rand_int((level_flag & (LF1_STRONGHOLD)) ? 4 : 3))
			{
				/* Make a larger room */
				case 2: case 3:
				{
#if 0
					/* Check for empty trailing block */
					if (((bx > (x1 / BLOCK_WID)) && (dun->room_map[by][bx-1] == FALSE)) ||
							/* Hack -- check for 'chance' of leading block empty */
							((bx < (x2 / BLOCK_WID) - 1) && (!rand_int((level_flag & (LF1_STRONGHOLD)) ? 2 : 3))))
					{
						int y0 = by * BLOCK_HGT + 5;
						int x0 = bx * BLOCK_WID;
						int y1a, y2a, x1a, x2a, y1b, y2b, x1b, x2b;
												
						/* Shift to the right instead */
						if ((bx == (x1 / BLOCK_WID)) || (dun->room_map[by][bx-1]) || (rand_int(100) < 25))
						{
							x0 += BLOCK_WID;
						}
						
						/* Save the room location */
						if (dun->cent_n < CENT_MAX)
						{
							dun->cent[dun->cent_n].y = y0;
							dun->cent[dun->cent_n].x = x0;
						}

						/* Use base room's ecology */
						room_info[dun->cent_n].ecology = room_info[base_room].ecology;

						/* Use base room's deepest monster */
						room_info[dun->cent_n].deepest_race = room_info[base_room].deepest_race;

						/* Increase the room count */
						dun->cent_n++;
						
						/* Note hack to have 'town-like' rooms */
						if (build_overlapping(dun->cent_n-1, ROOM_MONSTER_TOWN_SUBROOM, y1a, x1a, y2a, x2a,
							y1b, x1b, y2b, x2b, light, spacing, scale, scatter, pillars))
						{
							/* Room built */
							break;
						}
						else
						{
							/* Room not built */
							dun->cent_n--;
						}
#endif
					}
					/* Fall through */
				}
				/* Make a simple room from 4x4 to 9x9 in size */
				case 1:
				{
					/* Strong holds don't have simple buildings */
					if (((level_flag & (LF1_STRONGHOLD)) == 0) && (build_simple_building(by, bx, floor, wall)))
					{
						break;
					}
				}
				/* No room */
				case 0:
				{
					/*
					 * Hack -- certain room types look problematic in open spaces.
					 * We normally handle this with the LF1_WILD flag. However, to
					 * keep this simple and allow variety of rooms in the rest of
					 * the dungeon, we decide to just risk the very occasional
					 * problematic room, and instead exclude level types which almost
					 * exclusively place these problematic rooms.
					 */
					if ((level_flag & (LF1_CAVE | LF1_CAVERN | LF1_BURROWS | LF1_MINE)) == 0)
					{
						/* Clear the room */
						dun->room_map[by][bx] = FALSE;
						
						/* We've created less rooms than expected. Here to prevent infinite looping below. */
						rooms--;
					}
					
					break;
				}
			}
		}
	}

#if 0
	/* Ensure we at least generate the mimimum number of rooms */
	/* This is risky, but we 'should' be safe if we've counted correctly above */
	while (((level_flag & (LF1_STRONGHOLD)) == 0) && (rooms <= 0))
	{
		/* Pick random block */
		by = (y1 / BLOCK_HGT) + rand_int((y2 - y1) / BLOCK_HGT);
		bx = (x1 / BLOCK_WID) + rand_int((x2 - x1) / BLOCK_WID;
		
		/* Find space */
		if (dun->room[by][bx]) continue;
		
		/* Build simple room */
		if (build_simple_building(by, bx, floor, wall))
		{
			/* Built it */
			rooms++;
		}		
	}
#endif
	
	/* We are done */
	return (TRUE);
}
#endif




/*
 * Convert existing terrain type to rubble
 */
static void place_rubble(int y, int x)
{
	/* Put item under rubble */
	if (rand_int(100) < 5) cave_set_feat(y, x, FEAT_RUBBLE_H);

	/* Create rubble */
	else cave_set_feat(y, x, FEAT_RUBBLE);
}


char vault_monster_symbol = '\0';

/*
 * Check whether a vault monster matches a particular symbol
 */
static bool vault_monster_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Require symbol if set */
	if ((vault_monster_symbol) && (vault_monster_symbol != r_ptr->d_char)) return (FALSE);

	return (TRUE);
}

int vault_deepest_monster;

/*
 * Place a monster in a vault and get the details. Add these to the ecology.
 */
static void vault_monster(int y, int x, char symbol, int level)
{
	int i;
	int r_idx;

	/* Do we have to add this to the ecology */
	bool needed = TRUE;

	int old_ecology_ready = cave_ecology.ready;

	/* Out of depth monsters not part of the ecology */
	if (level >= 0)
	{
		cave_ecology.ready = FALSE;
	}

	/* Restrict to a symbol */
	if (symbol)
	{
		vault_monster_symbol = symbol;

		/* Require "okay" monsters */
		get_mon_num_hook = vault_monster_okay;

		/* Prepare allocation table */
		get_mon_num_prep();
	}

	/* Pick a monster, using the level calculation */
	r_idx = get_mon_num(p_ptr->depth + level);

	/* Clear restriction */
	if (symbol)
	{
		/* Don't require monster */
		get_mon_num_hook = NULL;

		/* Prepare allocation table */
		get_mon_num_prep();
	}

	cave_ecology.ready = old_ecology_ready;

	/* Handle failure */
	if (!r_idx) return;

	/* Attempt to place the monster (asleep, groups allowed) */
	if (!place_monster_aux(y, x, r_idx, TRUE, TRUE, 0L)) return;

	/* Check if vault monster in ecology */
	for (i = 0; i < cave_ecology.num_races; i++)
	{
		if (cave_ecology.race[i] == r_idx) needed = FALSE;
	}

	/* Get monster ecology */
	if (needed)
	{
		get_monster_ecology(r_idx, 0);
	}

	/* Track deepest monster level */
	if (vault_deepest_monster < r_info[r_idx].level) vault_deepest_monster = r_info[r_idx].level;
}


/*
 * Hack -- fill in "vault" rooms
 */
static bool build_vault(int room, int y0, int x0, int ymax, int xmax, cptr data)
{
	int i, j;
	int y, x, y1, x1, y2, x2;
	int c, len;
	int temp;
	cptr t;

	/* Allow vertical and horizontal flipping */
	bool flip_vert = (bool)rand_int(2);
	bool flip_hori = (bool)rand_int(2);

	int ecology_start = cave_ecology.num_races;

	/* Calculate the borders of the vault. */
	y1 = y0 - (ymax / 2);
	x1 = x0 - (xmax / 2);
	y2 = y1 + ymax - 1;
	x2 = x1 + xmax - 1;

	/* Make certain that the vault does not cross the dungeon edge. */
	if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);

	/* Make space for the ecology of the vault */
	if (cave_ecology.num_ecologies < MAX_ECOLOGIES) cave_ecology.num_ecologies++;

	/* Vault gets its own ecology */
	room_info[room].ecology |= (1L << (cave_ecology.num_ecologies - 1)) | (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));
	
	/* Sweep up existing ecology for use in vault */
	for (i = 0; i < cave_ecology.num_races; i++)
	{
		/* Monster in this room's base ecology */
		if ((cave_ecology.race_ecologies[i] & (room_info[room].ecology)) != 0)
		{
			/* Add monster to vault ecology */
			cave_ecology.race_ecologies[i] |= (1L << (cave_ecology.num_ecologies - 1));
		}
	}

	/* Use this ecology for monster placement during vault generation */
	cave_ecology.use_ecology = (1L << (cave_ecology.num_ecologies - 1));

	/* Track the deepest monster to ramp up difficulty near the vault */
	vault_deepest_monster = 0;

	/* Place dungeon features and objects */
	for (t = data, i = 0; i < ymax; i++)
	{
		for (j = 0; j < xmax; t++)
		{
			len = (byte)*t;

			/* Hack -- high bit indicates a run */
			if (len & 0x80)
			{
				len ^= 0x80;
				t++;
			}
			else
			{
				len = 1;
			}

			/* Extract encoded run */
			for (c = 0; c < len; j++, c++)
			{
				/* Hack -- skip "non-grids" */
				if (*t == ' ')
				{
					continue;
				}

				/* Allow vertical flips */
				if (flip_vert) y = y2 - i;
				else		   y = y1 + i;

				/* Allow horizontal flips */
				if (flip_hori) x = x2 - j;
				else		   x = x1 + j;

				/* Lay down a floor */
				cave_set_feat(y, x, FEAT_FLOOR);

				/* Add cave_info flags */
				cave_info[y][x] |= (CAVE_ROOM);

				/* Analyze the grid */
				switch (*t)
				{
					/* Granite wall (outer) */
					case '%':
					{
						cave_set_feat(y, x, FEAT_WALL_OUTER);
						break;
					}
					/* Granite wall (inner) */
					case '#':
					{
						cave_set_feat(y, x, FEAT_WALL_INNER);
						break;
					}
					/* Permanent wall (inner) */
					case 'X':
					{
						cave_set_feat(y, x, FEAT_PERM_INNER);
						break;
					}
					/* Treasure seam, in either magma or quartz. */
					case '*':
					{
						if (one_in_(2))
							cave_set_feat(y, x, FEAT_MAGMA_K);
						else cave_set_feat(y, x, FEAT_QUARTZ_K);
						break;
					}
					/* Chest */
					case '&':
					{
						place_chest(y, x);
					}
					/* Lava. */
					case '@':
					{
						cave_set_feat(y, x, FEAT_LAVA);
						break;
					}
					/* Water. */
					case 'x':
					{
						cave_set_feat(y, x, FEAT_WATER);
						break;
					}
					/* Tree. */
					case ';':
					{
						cave_set_feat(y, x, FEAT_TREE);
						break;
					}
					/* Rubble (sometimes with hidden treasure). */
					case ':':
					{
						place_rubble(y, x);
						break;
					}
					/* Treasure/trap */
					case '\'':
					{
						if (one_in_(2))
						{
							place_object(y, x, FALSE, FALSE);
						}
						else
						{
							place_trap(y, x);
						}
						break;
					}
					/* Secret doors */
					case '+':
					{
						place_secret_door(y, x);
						break;
					}
					/* Trap */
					case '^':
					{
						place_trap(y, x);
						break;
					}
					/* Up stairs  */
					case '<':
					{
						place_random_stairs(y, x, FEAT_LESS);
						break;
					}
					/* Down stairs. */
					case '>':
					{
						place_random_stairs(y, x, FEAT_MORE);
						break;
					}
				}
			}
		}
	}

	/* Place dungeon monsters and objects */
	for (t = data, i = 0; i < ymax; i++)
	{
		for (j = 0; j < xmax; t++)
		{
			len = (byte)*t;

			/* Hack -- high bit indicates a run */
			if (len & 0x80)
			{
				len ^= 0x80;
				t++;
			}
			else
			{
				len = 1;
			}

			/* Extract encoded run */
			for (c = 0; c < len; j++, c++)
			{
				/* Hack -- skip "non-grids" */
				if (*t == ' ') continue;

				/* Allow vertical flips */
				if (flip_vert) y = y2 - i;
				else		   y = y1 + i;

				/* Allow horizontal flips */
				if (flip_hori) x = x2 - j;
				else		   x = x1 + j;

				/* Most alphabetic characters signify monster races. */
				if ((isalpha(*t)) && (*t != 'x') && (*t != 'X'))
				{
					/* Place the monster */
					vault_monster(y, x, *t, 0);
				}

				/* Otherwise, analyze the symbol */
				else switch (*t)
				{
					/* An ordinary monster, object (sometimes good), or trap. */
					case '1':
					{
						int choice = rand_int(4);

						if (choice < 2)
						{
							vault_monster(y, x, 0, 0);
						}

						/* I had not intended this function to create
						 * guaranteed "good" quality objects, but perhaps
						 * it's better that it does at least sometimes.
						 */
						else if (choice == 2)
						{
							if (one_in_(10)) place_object(y, x, TRUE,
								FALSE);
							else place_object(y, x, FALSE, FALSE);
						}
						else
						{
							place_trap(y, x);
						}
						break;
					}
					/* Slightly out of depth monster. */
					case '2':
					{
						vault_monster(y, x, 0, 3);
						break;
					}
					/* Slightly out of depth object. */
					case '3':
					{
						object_level = p_ptr->depth + 3;
						place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;
						break;
					}
					/* Monster and/or object */
					case '4':
					{
						if (one_in_(2))
						{
							vault_monster(y, x, 0, 4);
						}
						if (one_in_(2))
						{
							object_level = p_ptr->depth + 4;
							place_object(y, x, FALSE, FALSE);
							object_level = p_ptr->depth;
						}
						break;
					}
					/* Out of depth object. */
					case '5':
					{
						object_level = p_ptr->depth + 6;
						place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;
						break;
					}
					/* Out of depth monster. */
					case '6':
					{
						vault_monster(y, x, 0, 6);
						break;
					}

					/* Very out of depth object. */
					case '7':
					{
						object_level = p_ptr->depth + 15;
						place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;
						break;
					}
					/* Very out of depth monster. */
					case '8':
					{
						vault_monster(y, x, 0, 12);
						break;
					}
					/* Meaner monster, plus "good" (or better) object */
					case '9':
					{
						vault_monster(y, x, 0, 10);
						object_level = p_ptr->depth + 5; /* +10 for good */
						place_object(y, x, TRUE, FALSE);
						object_level = p_ptr->depth;
						break;
					}

					/* Nasty monster and "great" (or better) object */
					case '0':
					{
						vault_monster(y, x, 0, 15);
						object_level = p_ptr->depth + 10; /* +10 for good */
						place_object(y, x, TRUE, TRUE);
						object_level = p_ptr->depth;
						break;
					}

					/* A chest. */
					case '&':
					{
						tval_drop_idx = TV_CHEST;

						object_level = p_ptr->depth + 5;
						place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Treasure. */
					case '$':
					{
						place_gold(y, x);
						break;
					}
					/* Armor. */
					case ']':
					{
						object_level = p_ptr->depth + 3;

						if (one_in_(3)) temp = randint(9);
						else temp = randint(8);

						if	    (temp == 1) tval_drop_idx = TV_BOOTS;
						else if (temp == 2) tval_drop_idx = TV_GLOVES;
						else if (temp == 3) tval_drop_idx = TV_HELM;
						else if (temp == 4) tval_drop_idx = TV_CROWN;
						else if (temp == 5) tval_drop_idx = TV_SHIELD;
						else if (temp == 6) tval_drop_idx = TV_CLOAK;
						else if (temp == 7) tval_drop_idx = TV_SOFT_ARMOR;
						else if (temp == 8) tval_drop_idx = TV_HARD_ARMOR;
						else tval_drop_idx = TV_DRAG_ARMOR;

						place_object(y, x, TRUE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Weapon. */
					case '|':
					{
						object_level = p_ptr->depth + 3;

						temp = randint(3);

						if      (temp == 1) tval_drop_idx = TV_SWORD;
						else if (temp == 2) tval_drop_idx = TV_POLEARM;
						else if (temp == 3) tval_drop_idx = TV_HAFTED;

						place_object(y, x, TRUE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Ring. */
					case '=':
					{
						tval_drop_idx = TV_RING;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Amulet. */
					case '"':
					{
						tval_drop_idx = TV_AMULET;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Potion. */
					case '!':
					{
						tval_drop_idx = TV_POTION;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Scroll. */
					case '?':
					{
						tval_drop_idx = TV_SCROLL;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Staff. */
					case '_':
					{
						tval_drop_idx = TV_STAFF;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Wand or rod. */
					case '-':
					{
						if (one_in_(2)) tval_drop_idx = TV_WAND;
						else tval_drop_idx = TV_ROD;

						object_level = p_ptr->depth + 3;
						if (one_in_(4))
							place_object(y, x, TRUE, FALSE);
						else place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
					/* Food or mushroom. */
					case ',':
					{
						tval_drop_idx = TV_FOOD;

						object_level = p_ptr->depth + 3;
						place_object(y, x, FALSE, FALSE);
						object_level = p_ptr->depth;

						tval_drop_idx = 0;

						break;
					}
				}
			}
		}
	}

	/* Shallower vault monsters are found outside the core room */
	/* We use this to ramp up the difficulty near the vault to warn
	 * the player and make it harder to exploit.
	 */
	for (j = ecology_start; j < cave_ecology.num_races; j++)
	{
		/* Deepest monster */
		if (r_info[cave_ecology.race[j]].level < (p_ptr->depth + vault_deepest_monster) / 2)
		{
			/* Appears in rooms near the vault */
			cave_ecology.race_ecologies[j] |= (room_info[room].ecology & ((1L << MAX_ECOLOGIES) -1));
		}
		else
		{
			/* Appears in the vault only */
			cave_ecology.race_ecologies[j] |= (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));
		}
	}

	/* Deepest race */
	room_info[room].deepest_race = deeper_monster(room_info[room].deepest_race, cave_ecology.deepest_race[cave_ecology.num_ecologies - 1]);

	/* Don't use this ecology */
	cave_ecology.use_ecology = 0L;

	/* Success. */
	return (TRUE);
}


/*
 * Hack -- fill in "tower" rooms
 *
 * Similar to vaults, but we never place monsters/traps/treasure.
 */
static void build_tower(int y0, int x0, int ymax, int xmax, cptr data)
{
	int dx, dy, x, y, i;

	cptr t;

	int monsters_left = 30;

	/* Put the guardian in the tower */
	dun->guard_y0 = y0 - ymax / 2;
	dun->guard_x0 = x0 - xmax / 2;
	dun->guard_y1 = y0 + ymax / 2;
	dun->guard_x1 = x0 + xmax / 2;

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* Extract the location */
			x = x0 - (xmax / 2) + dx;
			y = y0 - (ymax / 2) + dy;

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Lay down a floor */
			cave_set_feat(y, x, FEAT_FLOOR);

			/* Part of a vault */
			cave_info[y][x] |= (CAVE_ROOM);

			/* Hack -- always lite towers */
			cave_info[y][x] |= (CAVE_GLOW);

			/* Analyze the grid */
			switch (*t)
			{
				/* Granite wall (outer) */
				case '%':
				{
					cave_set_feat(y, x, FEAT_WALL_OUTER);
					break;
				}

				/* Granite wall (inner) */
				case '#':
				{
					cave_set_feat(y, x, FEAT_WALL_INNER);
					break;
				}

				/* Permanent wall (inner) */
				case 'X':
				{
					cave_set_feat(y, x, FEAT_PERM_INNER);
					break;
				}

				/* Treasure/trap */
				case '*':
				{
					break;
				}

				/* Now locked doors */
				case '+':
				{
					place_locked_door(y, x);
					break;
				}

				/* Trap */
				case '^':
				{
					break;
				}
			}
		}
	}

	/*** Now we get to place the monsters. ***/

	/* Place the monsters. */
	for (i = 0; i < 300; i++)
	{
		/* Check for early completion. */
		if (!monsters_left) break;

		/* Pick a random in-room square. */
		y = rand_range(y0 - ymax, y0 + ymax);
		x = rand_range(x0 - xmax, x0 + xmax);

		/* Require a floor square with no monster in it already. */
		if (!cave_naked_bold(y, x)) continue;

		/* Place a single monster.  Sleeping 2/3rds of the time. */
		place_monster_aux(y, x, get_mon_num(monster_level),
			(bool)(rand_int(3)), FALSE, 0L);

		/* One less monster to place. */
		monsters_left--;
	}

}


/*
 * Hack -- fill in "roof"
 *
 * This allows us to stack multiple towers on top of each other, so that
 * a player ascending from one tower to another will not fall.
 */
static void build_roof(int y0, int x0, int ymax, int xmax, cptr data)
{
	int dx, dy, x, y;

	cptr t;

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* Extract the location */
			x = x0 - (xmax / 2) + dx;
			y = y0 - (ymax / 2) + dy;

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Lay down a floor */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	}
}


/*#define ENABLE_WAVES 1 */

#ifdef ENABLE_WAVES

/*
 * Pick a point from the border of a rectangle given its top left corner (p1)
 * and its bottom right corner (p2).
 * Assumes (p1.x < p2.x) and (p1.y < p2.y)
 * Returns the selected point in "result".
 */
static void pick_point_from_border(coord p1, coord p2, coord *result)
{
	/* Pick a side of the rectangle */
	switch (rand_int(4))
	{
		/* Top border */
		case 0:
		{
			/* Fixed coordinate */
			result->y = p1.y;
			result->x = rand_range(p1.x, p2.x);
			break;
		}
		/* Bottom border */
		case 1:
		{
			/* Fixed coordinate */
			result->y = p2.y;
			result->x = rand_range(p1.x, p2.x);
			break;
		}
		/* Left border */
		case 2:
		{
			/* Fixed coordinate */
			result->x = p1.x;
			result->y = rand_range(p1.y, p2.y);
			break;
		}
		/* Right border */
		case 3:
		{
			/* Fixed coordinate */
			result->x = p2.x;
			result->y = rand_range(p1.y, p2.y);
			break;
		}
	}
}



/*
 * Construct a chain of waves for a water lake, given the coordinates of the
 * rectangle who contains the lake.
 */
static void build_waves(int y1, int x1, int y2, int x2)
{
	coord p1, p2;
	coord start, end;
	int dx, dy;
	int wid, hgt;

	int tries = 0;

	/* Fill in the point structures */
	p1.x = x1;
	p1.y = y1;

	p2.x = x2;
	p2.y = y2;

	/* Paranoia */
	if (!in_bounds_fully(y1, x1) || !in_bounds_fully(y2, x2)) return;

	/* Get the size of the lake */
	hgt = y2 - y1 + 1;
	wid = x2 - x1 + 1;

	/* We don't like small lakes */
	if ((hgt < 15) || (wid < 15)) return;

	/* Pick a pair of points from the border of the rectangle */
	while (TRUE)
	{
		int chain_wid, chain_hgt;

		/* Paranoia */
		if (++tries > 2500) return;

		/* Pick the points */
		pick_point_from_border(p1, p2, &start);
		pick_point_from_border(p1, p2, &end);

		/* These points define a rectangle. Get its size */
		chain_hgt = (int)start.y - (int)end.y;
		chain_hgt = ABS(chain_hgt) + 1;

		chain_wid = (int)start.x - (int)end.x;
		chain_wid = ABS(chain_wid) + 1;

		/* This new rectangle must be noticeable */
		if ((chain_hgt >= (2 * hgt / 3)) &&
			(chain_wid >= (2 * wid / 3))) break;
	}

	/* Reset the loop count */
	tries = 0;

	/* Join the points using waves */
	while ((start.x != end.x) || (start.y != end.y))
	{
		int x, y, i, wave;

		/* Paranoia */
		if (++tries > 2500) return;

		/* Get the current coordinate of the chain */
		y = start.y;
		x = start.x;

		/* Get the next coordinate */
		correct_dir(&dy, &dx, start.y, start.x, end.y, end.x);

		/* Get the next coordinate of the chain (for later) */
		start.y += dy;
		start.x += dx;

		/* Paranoia */
		if (!in_bounds_fully(y, x)) break;

		/* Ignore non-water */
		if ((f_info[cave_feat[y][x]].flags2 & (FF2_WATER)) == 0) continue;

		/* Pick the proper "crest" feature */
		if ((f_info[cave_feat[y][x]].flags2 & (FF2_DEEP)) == 0)
		{
			wave = FEAT_CREST_H;
		}
		else
		{
			wave = FEAT_CREST;
		}

		/* A chain of waves is built using "crest of waves" */
		cave_set_feat(y, x, wave);

		/* Sometimes, surround the crest with "plain" waves */
		if (rand_int(100) < 50)	continue;

		/* Place the surrounding waves (similar to build_streamer) */
		for (i = 0; i < 2; i++)
		{
			/* Pick a nearby location (very close) */
			int yy = rand_spread(y, 1);
			int xx = rand_spread(x, 1);

			/* Ignore annoying locations */
			if (!in_bounds_fully(yy, xx)) continue;

			/* Ignore non-water */
			if(!cave_ff3_match(yy, xx, FF3_WATER)) continue;

			/* Pick the proper "wave" feature */
			if (cave_ff2_match(yy, xx, FF2_DEEP))
			{
				wave = FEAT_WAVE_H;
			}
			else
			{
				wave = FEAT_WAVE;
			}

			/* Place the wave */
			cave_set_feat(yy, xx, wave);
		}
	}
}

#endif /* ENABLE_WAVES */


/*
 * Room type information
 */

typedef struct lake_size lake_size;

struct lake_size
{
	/* Required size in blocks */
	s16b dy1, dy2, dx1, dx2;

	/* Hack -- minimum level */
	s16b level;
};


/* Available size for lakes (in blocks) */
enum
{
	LAKE_DATA_2x2,
	LAKE_DATA_2x3,
	LAKE_DATA_3x3,
	LAKE_DATA_3x4,
	LAKE_DATA_4x4,
	LAKE_DATA_4x5,
	MAX_LAKE_DATA
};

/* Block information for lakes (sorted by size, smaller first) */
static const lake_size lake_data[MAX_LAKE_DATA] =
{
	{0, 1, 0, 1, 1},		/* LAKE_DATA_2x2 */
	{0, 1, -1, 1, 1},		/* LAKE_DATA_2x3 */
	{-1, 1, -1, 1, 1},		/* LAKE_DATA_3x3 */
	{-1, 1, -1, 2, 1},		/* LAKE_DATA_3x4 */
	{-1, 2, -1, 2, 1},		/* LAKE_DATA_4x4 */
	{-1, 2, -2, 2, 1},		/* LAKE_DATA_4x5 */
};

/*
 * Build a lake using the given feature.
 * Returns TRUE on success.
 * The coordinates of its center are stored in y0 and x0.
 */
static bool build_lake(int feat, bool do_big_lake, bool merge_lakes,
	int *y0, int *x0)
{
	int bx0, by0;
	int bx1, by1, bx2, by2;
	int wid, hgt;
	int tries = 0;
	int lake_idx;
	const lake_size *ld;
	/* Starburst flags */
	u32b flag = 0;
	/*
	 * Notice special cases: these are replaced with passable features
	 * sometimes (build_terrain)
	 */
	bool solid_lake = ((f_info[feat].flags1 & (FF1_WALL)) != 0);

	/* Solid lakes are made very large sometimes */
	if (solid_lake || !rand_int(3)) do_big_lake = TRUE;

	/* Choose an initial size for the lake */
	if (do_big_lake)
	{
		if (!rand_int(10)) lake_idx = LAKE_DATA_4x5;
		else if (!rand_int(5)) lake_idx = LAKE_DATA_4x4;
		else lake_idx = LAKE_DATA_3x4;
	}
	else
	{
		/*
		 * Lakes at shallow levels are smaller, and some at deeper
		 * levels too
		 */
		if ((p_ptr->depth >= 25) && !rand_int(7))
		{
			lake_idx = LAKE_DATA_3x4;
		}
		else
		{
			lake_idx = LAKE_DATA_2x3;
		}
	}

	/* Adjust the size of the lake, if needed */
	while (TRUE)
	{
		/* Get block information for this kind of lake */
		ld = &lake_data[lake_idx];

		/* Get the size of the lake in blocks */
		hgt = ld->dy2 - ld->dy1 + 1;
		wid = ld->dx2 - ld->dx1 + 1;

		/* Can be placed in this dungeon? */
		if ((hgt <= dun->row_rooms) && (wid <= dun->col_rooms)) break;

		/* Try again with a smaller lake */
		--lake_idx;

		/* Level too small, give up */
		if (lake_idx < 0) return (FALSE);
	}

	/* Try to get a location for the lake */
	while (TRUE)
	{
		/* Too many tries. Reject lake */
		if (++tries >= 25) return (FALSE);

		/* Pick a random block */
		if (!merge_lakes || solid_lake)
		{
			by0 = ld->dy1 + rand_int(dun->row_rooms - ld->dy2);
			bx0 = ld->dx1 + rand_int(dun->col_rooms - ld->dx2);
		}
		/* Force lake overlapping */
		else
		{
			/* Put the lakes in the middle of the level */
			by0 = (dun->row_rooms / 2);

			/* Slightly move the lake horizontally */
			/* Big lakes (-1, +0 or +1 blocks) */
			if (lake_idx > LAKE_DATA_2x3)
			{
				bx0 = (dun->col_rooms / 2) + (rand_int(3) - 1);
			}
			/* Small lakes (+0 or +1 blocks) */
			else
			{
				bx0 = (dun->col_rooms / 2) + rand_int(2);
			}
		}

		/* Get the blocks */
		by1 = by0 + ld->dy1;
		by2 = by0 + ld->dy2;
		bx1 = bx0 + ld->dx1;
		bx2 = bx0 + ld->dx2;

		/* Ignore blocks outside the dungeon */
		if ((by1 < 0) || (by2 >= dun->row_rooms)) continue;
		if ((bx1 < 0) || (bx2 >= dun->col_rooms)) continue;

		/* Found a suitable location */
		break;
	}

	/* Get total height and width of the available space */
	hgt *= BLOCK_HGT;
	wid *= BLOCK_WID;

	/* Get the center of the lake */
	*y0 = by1 * BLOCK_HGT + hgt / 2;
	*x0 = bx1 * BLOCK_WID + wid / 2;

	/* Store extra information for passable lakes */
	if (!solid_lake)
	{
		/* Forests are always lit. Others not so much */
		if (((f_info[feat].flags3 & (FF3_LIVING)) != 0) ||
			(p_ptr->depth <= randint(25)))
		{
			flag |= (STAR_BURST_LIGHT);
		}
	}

	/*
	 * Convenience. Get the distance from the center to the borders.
	 * Note that we substract some space to place tunnels later and to
	 * avoid dungeon permanent boundry
	 */
	hgt = (hgt - 4) / 2;
	wid = (wid - 4) / 2;

	/* Place the lake */
	if (!generate_starburst_room(*y0 - hgt, *x0 - wid, *y0 + hgt, *x0 + wid,
		feat, f_info[feat].edge, flag))
	{
		return (FALSE);
	}
	
	/* Store extra information for passable lakes */
	if (!solid_lake)
	{
		/* Wilderness dungeon */
		level_flag |= (LF1_WILD);

		/* Block out exactly the centre grid only */
		dun->room_map[by0][bx0] = TRUE;
	}

#ifdef ENABLE_WAVES
	/* Special case -- Give some flavor to water lakes */
	if ((feat == FEAT_WATER) || (feat == FEAT_WATER_H))
	{
		int i;

		for (i = 0; i < (do_big_lake ? 2: 1); i++)
		{
			build_waves(*y0 - hgt, *x0 - wid, *y0 + hgt, *x0 + wid);
		}
	}
#endif /* ENABLE_WAVES */

	/* Success */
	return (TRUE);
}


/*
 * Build a river given a feature and its starting location
 */
static void build_river(int feat, int y, int x)
{
	/*
	 * This map contains the directions of the grids who must be converted
	 * to edge, given a compass direction [0-3]
	 */
	static byte edge_map[][3] =
	{
		{1, 2, 3},
		{7, 8, 9},
		{3, 6, 9},
		{1, 4, 7}
	};

	int i, dir, old_dir;
	int old_feat;
	int edge = f_info[feat].edge;
	/*
	 * Notice special cases: they are replaced by passable features
	 * sometimes (build_terrain)
	 */
	bool solid_river = ((f_info[feat].flags1 & (FF1_WALL)) != 0);

	/* Choose a random compass direction */
	dir = old_dir = rand_int(4);

	/* Place river into dungeon */
	while (TRUE)
	{
		/* Stop at dungeon edge */
		if (!in_bounds_fully(y, x)) break;

		/* Get the previous content of the grid */
		old_feat = cave_feat[y][x];

		/* Stop at permanent feature */
		if ((f_info[old_feat].flags1 & (FF1_PERMANENT)) != 0) break;

		/* Most rivers aren't pierced by rooms. */
		if (!solid_river)
		{
			/* Forbid rooms here */
			int by = y / BLOCK_HGT;
			int bx = x / BLOCK_WID;

			dun->room_map[by][bx] = TRUE;
		}

		/* Place a piece of the river, if needed */
		if (feat != old_feat) build_terrain(y, x, feat);

		/* Place river edge, if needed */
		if (edge != FEAT_NONE)
		{
			for (i = 0; i < 3; i++)
			{
				/* Use the map to modify only grids ahead */
				int yy = y + ddy[edge_map[dir][i]];
				int xx = x + ddx[edge_map[dir][i]];

				/* Ignore annoying locations */
				if (!in_bounds_fully(yy, xx)) continue;

				/* Get the previous content of the grid */
				old_feat = cave_feat[yy][xx];

				/* Avoid permanent features */
				if ((f_info[old_feat].flags1 & (FF1_PERMANENT)) != 0) continue;

				/* IMPORTANT: Don't overwrite the river */
				if (old_feat == feat) continue;

				/* Place river edge, if needed */
				if (edge != old_feat) build_terrain(yy, xx, edge);
			}
		}

		/* Stagger the river */
		if (!rand_int(2))
		{
			dir = rand_int(4);
		}
		/* Advance the streamer using the original direction */
		else
		{
			dir = old_dir;
		}

		/* Get the next coordinates */
		y += ddy_ddd[dir];
		x += ddx_ddd[dir];
	}
}


/*
 * Record good location for stairwell, player start or quest object
 */
static bool add_stair(int y, int x)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Record location */
	if (dun->stair_n < STAIR_MAX)
	{
		dun->stair[dun->stair_n].y = y;
		dun->stair[dun->stair_n].x = x;
		dun->stair_n++;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Always picks a correct direction
 */
static void correct_dir_cave(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;
}



/*
 * Always picks a correct direction
 */
static void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (rand_int(100) < 50)
		{
			*rdir = 0;
		}
		else
		{
			*cdir = 0;
		}
	}
}


/*
 * Go in a semi-random direction from current location to target location.
 * Do not actually head away from the target grid.  Always make a turn.
 */
static void adjust_dir(int *row_dir, int *col_dir, int y1, int x1, int y2, int x2)
{
	/* Always turn 90 degrees. */
	if ((*row_dir == 0) || ((*row_dir != 0) && (*col_dir != 0) && (rand_int(100) < 50)))
	{
		*col_dir = 0;

		/* On the y-axis of target - freely choose a side to turn to. */
		if (y1 == y2) *row_dir = ((rand_int(2)) ? - 1 : 1);

		/* Never turn away from target. */
		else *row_dir = ((y1 < y2) ? 1 : -1);
	}
	else
	{
		*row_dir = 0;

		/* On the x-axis of target - freely choose a side to turn to. */
		if (x1 == x2) *col_dir = ((rand_int(2)) ? - 1 : 1);

		/* Never turn away from target. */
		else *col_dir = ((x1 < x2) ? 1 : -1);
	}
}


/*
 * Go in a completely random orthogonal direction.  If we turn around
 * 180 degrees, save the grid; it may be a good place to place stairs
 * and/or the player.
 */
static void rand_dir(int *row_dir, int *col_dir, int y, int x)
{
	/* Pick a random direction */
	int i = rand_int(4);

	/* Extract the dy/dx components */
	int row_dir_tmp = ddy_ddd[i];
	int col_dir_tmp = ddx_ddd[i];

	/* Save useful grids. */
	if ((-(*row_dir) == row_dir_tmp) && (-(*col_dir) == col_dir_tmp))
	{
		/* Save the current tunnel location if surrounded by walls. */
		if (next_to_walls(y, x) == 4)
		{
			add_stair(y, x);
		}
	}

	/* Save the new direction. */
	*row_dir = row_dir_tmp;
	*col_dir = col_dir_tmp;
}


/*
 * Go in a completely random direction.  If we turn around
 * 180 degrees, save the grid; it may be a good place to place stairs
 * and/or the player.
 */
static void rand_dir_cave(int *row_dir, int *col_dir, int y, int x)
{
	/* Pick a random direction */
	int i = rand_int(8);

	/* Extract the dy/dx components */
	int row_dir_tmp = ddy_ddd[i];
	int col_dir_tmp = ddx_ddd[i];

	/* Save useful grids. */
	if ((-(*row_dir) == row_dir_tmp) && (-(*col_dir) == col_dir_tmp))
	{
		/* Save the current tunnel location if surrounded by walls. */
		if ((in_bounds_fully(y, x)) && (dun->stair_n < STAIR_MAX) &&
			(next_to_walls(y, x) == 4))
		{
			add_stair(y, x);
		}
	}

	/* Save the new direction. */
	*row_dir = row_dir_tmp;
	*col_dir = col_dir_tmp;
}


/*
 * Leave 1 & 2 empty as these are used to vary the crenallations in crypt
 * corridors
 */

#define TUNNEL_STYLE	0x00000004L	/* First 'real' style */
#define TUNNEL_CRYPT_L	0x00000004L
#define TUNNEL_CRYPT_R	0x00000008L
#define TUNNEL_LARGE_L	0x00000010L
#define TUNNEL_LARGE_R	0x00000020L
#define TUNNEL_CAVE		0x00000040L
#define TUNNEL_PATH		0x00000080L
#define TUNNEL_ANGLE	0x00000100L

static u32b get_tunnel_style(void)
{
	int style = 0;
	int i = rand_int(100);

	/* Polygonal levels have direct straight paths between locations */
	if (level_flag & (LF1_POLYGON))
	{
		style |= (TUNNEL_ANGLE);
	}	

	/* Wilderness have direct paths between locations */
	/* From FAAngband */
	else if (((level_flag & (LF1_WILD)) != 0) && ((level_flag & (LF1_SURFACE)) != 0))
	{
		style |= (TUNNEL_PATH);
	}
	
	/* Stronghold levels have width 2 corridors, or width 3 corridors, often with pillars */
	/* The style of the tunnel does not change after initial selection */
	else if (level_flag & (LF1_STRONGHOLD))
	{
		if (i < 66) style |= (TUNNEL_LARGE_L);
		if (i > 33) style |= (TUNNEL_LARGE_R);
	}
	/* Dungeon levels have width 1 corridors, or width 2 corridors deeper in the dungeon */
	/* The style of the tunnel does not change after initial selection */
	else if (level_flag & (LF1_DUNGEON))
	{
		if (i < p_ptr->depth) style |= (i % 2) ? (TUNNEL_LARGE_L) : (TUNNEL_LARGE_R);
	}
	/* Sewer levels have width 2 corridors, or width 3 corridors, often with pillars */
	/* The centre of the corridor is filled with pool type terrain, and the corridor style
	   is fixed for each corridor. */
	else if (level_flag & (LF1_SEWER))
	{
		if (i < 50) style |= (TUNNEL_LARGE_L);
		if ((i < 25) || (i >= 75)) style |= (TUNNEL_LARGE_R);
	}
	/* Crypt levels have width 2 corridors, or width 3 corridors, with pillars on the edge of the corridor */
	/* The corridor style changes regularly. */
	else if (level_flag & (LF1_CRYPT))
	{
		if (i < 50) style |= (TUNNEL_CRYPT_L);
		if ((i < 25) || (i >= 75)) style |= (TUNNEL_CRYPT_R);
	}
	/* Cave and burrows levels have narrow, frequently random corridors. Mines occasionally do. */
	/* Caverns have wider corridors than caves */
	else if (((level_flag & (LF1_CAVE | LF1_CAVERN | LF1_BURROWS)) != 0) || (((level_flag & (LF1_MINE)) != 0) && (i < 33)))
	{
		style |= (TUNNEL_CAVE);
		if ((level_flag & (LF1_CAVERN)) != 0)
		{
			if (i < 33) style |= (TUNNEL_LARGE_L);
			else if (i >= 66) style |= (TUNNEL_LARGE_R);
		}
	}

	style |= rand_int(TUNNEL_STYLE);
	
	return (style);
}


/*
 * Record location for tunnel decoration
 */
static bool add_decor(int y, int x, int t)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Needs to be a wall */
	if ((f_info[cave_feat[y][x]].flags1 & (FF1_WALL)) == 0) return (FALSE);

	/* Record location and tunnel length */
	if (dun->decor_n < DECOR_MAX)
	{
		dun->decor[dun->decor_n].y = y;
		dun->decor[dun->decor_n].x = x;
		dun->decor_t[dun->decor_n] = t;
		dun->decor_n++;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Record location for entrance to room through outer wall
 */
static bool add_wall(int y, int x)
{
	/* XXX Don't need to check inbounds because of use of inbounds_fully_tunnel from caller */

	/* Needs an 'outer' wall */
	if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) == 0) return (FALSE);

	/* Record location */
	if (dun->wall_n < WALL_MAX)
	{
		dun->wall[dun->wall_n].y = y;
		dun->wall[dun->wall_n].x = x;
		dun->wall_n++;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Record location for entrance to room through outer wall
 */
static bool add_solid(int y, int x, int feat)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Needs an 'outer' wall */
	if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) == 0) return (FALSE);

	/* Record location and feature */
	if (dun->solid_n < SOLID_MAX)
	{
		dun->solid[dun->solid_n].y = y;
		dun->solid[dun->solid_n].x = x;
		dun->solid_feat[dun->solid_n] = feat;
		dun->solid_n++;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Record location for tunnel
 */
static bool add_tunnel(int y, int x, int feat)
{
	/* XXX Don't need to check inbounds because of use of inbounds_fully_tunnel from caller */

	/* XXX We could bridge or tunnel a location and add it as a 'tunnel' */

	/* Record location and feature */
	if (dun->tunn_n < TUNN_MAX)
	{
		dun->tunn[dun->tunn_n].y = y;
		dun->tunn[dun->tunn_n].x = x;
		dun->tunn_feat[dun->tunn_n] = feat;
		dun->tunn_n++;

		play_info[y][x] =255;

		if ((cheat_room) && !(dun->tunn_n % 100)) message_add(format("Tunnel length %d.", dun->tunn_n), MSG_GENERIC);

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Record location for entrance to room through outer wall
 */
static bool add_next(int y, int x, int feat)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Needs an 'solid' wall */
	if ((f_info[cave_feat[y][x]].flags1 & (FF1_SOLID)) == 0) return (FALSE);

	/* Record location and feature */
	if (dun->next_n < NEXT_MAX)
	{
		dun->next[dun->next_n].y = y;
		dun->next[dun->next_n].x = x;
		dun->next_feat[dun->next_n] = f_info[feat].flags1 & (FF1_OUTER) ? feat_state(feat, FS_SOLID) : feat;
		dun->next_n++;

		return (TRUE);
	}

	return (FALSE);
}


/*
 * Record location for door at intersection
 */
static bool add_door(int y, int x)
{
	/* Needs to be in bounds */
	if (!in_bounds_fully(y, x)) return (FALSE);

	/* Needs not to be a wall */
	if ((f_info[cave_feat[y][x]].flags1 & (FF1_WALL)) != 0) return (FALSE);

	/* Record location */
	if (dun->door_n < DOOR_MAX)
	{
		dun->door[dun->door_n].y = y;
		dun->door[dun->door_n].x = x;
		dun->door_n++;
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Check if there are any "non-room" grids adjacent to the given grid.
 */
static bool near_edge(int y1, int x1)
{
	int y, x;

	/* Scan nearby grids */
	for (y = y1 - 2; y < y1 + 2; y++)
	{
		for (x = x1 - 2; x < x1 + 2; x++)
		{
			/* Skip grids inside rooms */
			if (cave_info[y][x] & (CAVE_ROOM)) continue;

			/* Count these grids */
			return (TRUE);
		}
	}

	/* Inside a room */
	return (FALSE);
}


/*
 * Constructs a tunnel between two points
 *
 * This function must be called BEFORE any streamers are created,
 * since we use the special "granite wall" sub-types to keep track
 * of legal places for corridors to pierce rooms.
 *
 * We use "door_flag" to prevent excessive construction of doors
 * along overlapping corridors.
 *
 * We queue the tunnel grids to prevent door creation along a corridor
 * which intersects itself.
 *
 * We queue the wall piercing grids to prevent a corridor from leaving
 * a room and then coming back in through the same entrance.
 *
 * We "pierce" grids which are "outer" walls of rooms, and when we
 * do so, we change all adjacent "outer" walls of rooms into "solid"
 * walls so that no two corridors may use adjacent grids for exits.
 *
 * The "solid" wall check prevents corridors from "chopping" the
 * corners of rooms off, as well as "silly" door placement, and
 * "excessively wide" room entrances.
 *
 * Useful "feat" values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_EXTRA -- shop walls (perma)
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 *
 *
 * We now style the tunnels. The following tunnel styles are supported:
 *
 * -- standard Angband tunnel
 * -- tunnel with pillared edges (on LF1_CRYPT levels)
 * -- width 2 or width 3 tunnel (on LF1_STRONGHOLD levels)
 * -- tunnels with lateral and diagonal interruptions (on LF1_CAVE levels)
 * -- tunnels that follow a direct path (on LF1_WILD levels)
 *
 * We also can fill tunnels now. Filled tunnels occur iff the last floor
 * space leaving the start room is not a FEAT_FLOOR or the first floor
 * space entering the finishing room is not a FEAT_FLOOR.
 *
 * We also put 'decorations' next to tunnel entrances. These are various
 * solid wall types relating to the room the tunnel goes from or to.
 * However, we use the decorations of the room at the other end of the tunnel
 * unless that room has no decorations, in which case we use our own.
 */
static bool build_tunnel(int row1, int col1, int row2, int col2, bool allow_overrun, bool allow_corridors)
{
	int i, j, y, x;
	int tmp_row = row1, tmp_col = col1;
	int row_dir, col_dir;
	int start_row, start_col;
	int main_loop_count = 0;
	int last_turn = 0, first_door, last_door, first_tunn, first_next, first_stair;
	int start_tunnel = 0, start_decor = 0, end_decor = 0;
	int midpoint = 0, midpoint_solid = 0;
	int last_decor = 0;
	int door_l = 0;
	int door_r = 0;
	int old_row_dir = 0;
	int old_col_dir = 0;
	int deadends = 0;
	int loopiness = 3;

	bool flood_tunnel = FALSE;
	bool door_flag = FALSE;
	bool overrun_flag = FALSE;
	bool decor_flag = FALSE;
	bool abort_and_cleanup = FALSE;

	/* Force style change */
	u32b style = get_tunnel_style();

	int start_room = room_idx_ignore_valid(row1, col1);
	int end_room = 0;

	/* Initialize some movement counters */
	int adjust_dir_timer = randint(DUN_TUN_ADJ * 2) * (((level_flag & (LF1_MINE)) != 0) ? 2 : 1);
	int rand_dir_timer   = randint(DUN_TUN_RND * 2);
	int correct_dir_timer = 0;
	int tunnel_style_timer = randint(DUN_TUN_STYLE * 2);
	int crypt_timer = randint(DUN_TUN_CRYPT * 2);
	int cave_length_timer = ((style & TUNNEL_CAVE) != 0) ? randint(DUN_TUN_LEN * 2) : 0;

	/* Not yet worried about our progress */
	int desperation = 0;

	/* Partition to mark array with */
	int part1 = CENT_MAX;

	/* Details for tunnel style TUNNEL_PATH & TUNNEL_ANGLE*/
	u16b path[512];
	int path_n = 0;
	int path_i = 0;
	
	/* Record initial partition number */
	if (start_room)
	{
		part1 = dun->part[start_room];

		if ((cheat_room) && (part1 != start_room)) message_add(format("Starting room is %d out of %d, partition is %d.", start_room, dun->cent_n, part1), MSG_GENERIC);
		
		/* Flood if part of flooded dungeon */
		if ((room_info[start_room].flags & (ROOM_FLOODED)) != 0)
		{
			flood_tunnel = TRUE;
		}
		
		/* Simple corridors from simple starting rooms */
		if ((room_info[start_room].flags & (ROOM_SIMPLE)) != 0)
		{
			style &= (TUNNEL_STYLE -1);
			tunnel_style_timer *= 2;
		}
	}

	/* Keep stronghold / dungeon / sewer corridors tidy */
	if ((level_flag & (LF1_STRONGHOLD | LF1_DUNGEON | LF1_SEWER | LF1_WILD | LF1_POLYGON)) != 0) tunnel_style_timer = -1;

	/* Readjust movement counter for caves */
	if ((style & TUNNEL_CAVE) != 0) rand_dir_timer = randint(DUN_TUN_CAV * 2);

	/* Don't adjust paths / angles */
	if ((style & (TUNNEL_PATH | TUNNEL_ANGLE)) != 0)
	{
		rand_dir_timer = -1;
		adjust_dir_timer = -1;
	}

	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;
	dun->solid_n = 0;
	dun->decor_n = 0;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Record number of doorways */
	first_door = dun->door_n;
	last_door = dun->door_n;

	/* Record start locations in the event of aborting */
	first_tunn = dun->tunn_n;
	first_next = dun->next_n;
	first_stair = dun->stair_n;

	/* Wider corridors tolerate less loopiness */
	if (style & (TUNNEL_LARGE_L | TUNNEL_CRYPT_L)) loopiness--;
	if (style & (TUNNEL_LARGE_R | TUNNEL_CRYPT_R)) loopiness--;
	
	/* Get path direction */
	if (style & (TUNNEL_PATH | TUNNEL_ANGLE))
	{
		int tmp_row2 = row2;
		int tmp_col2 = col2;

		/* Start out in correct direction */
		path_n = project_path(path, 512, row1, col1, &tmp_row2, &tmp_col2, (PROJECT_THRU | PROJECT_PASS));

		/* Distance to go */
		if (path_n)
		{
			/* Sanity check */
			row_dir = GRID_Y(path[path_i]) - row1;
			col_dir = GRID_X(path[path_i]) - col1;
		}
	}
	else
	{
		/* Start out in the correct direction */
		correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
	}

	/* Keep going until done (or bored) */
	while (TRUE)
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000)
		{
			if (cheat_room) message_add(format("Main loop count exceeded from room %d in partition %d. Aborting.", start_room, part1), MSG_GENERIC);
			abort_and_cleanup = TRUE;
			break;
		}

		/* Hack -- if we are not starting in a room, include starting grid. Also add as a possible stair location. */
		if ((dun->tunn_n == 0) && ((cave_info[row1][col1] & (CAVE_ROOM)) == 0))
		{
			row1 = row1 - row_dir;
			col1 = col1 - col_dir;
			
			/* Increase tunnel length by one */
			add_tunnel(row1, col1, ((f_info[cave_feat[row1][col1]].flags2 & (FF2_PATH)) != 0) ? 0 : 1);

			/* Good candidate for stairs */
			add_stair(row1, col1);

			/* Decorate 'left-hand' corner of dead end */
			add_decor(row1 - row_dir + col_dir * (((style & (TUNNEL_LARGE_L)) != 0) ? 2 : 1),
							col1 - col_dir - row_dir * (((style & (TUNNEL_LARGE_L)) != 0) ? 2 : 1),
							dun->tunn_n);

			/* Decorate 'right-hand' corner of dead end */
			add_decor(row1 - row_dir + col_dir * (((style & (TUNNEL_LARGE_R)) != 0) ? 2 : 1),
							col1 - col_dir + row_dir * (((style & (TUNNEL_LARGE_R)) != 0) ? 2 : 1),
							dun->tunn_n);

			/* Force decoration */
			decor_flag = FALSE;
		}

		/* Try moving randomly if we seem stuck. */
		else if ((row1 != tmp_row) || (col1 != tmp_col))
		{
			desperation++;

			/* Try a 90 degree turn. */
			if (desperation == 1)
			{
				adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
				adjust_dir_timer = 3;
			}

			/* Try turning randomly. */
			else if (desperation < 4)
			{
				rand_dir(&row_dir, &col_dir, row1, col1);
				correct_dir_timer = 2;
			}
			else
			{
				if (cheat_room) message_add(format("Tunnel stuck from room %d in partition %d. Aborting.", start_room, part1), MSG_GENERIC);

				abort_and_cleanup = TRUE;
				break;
			}
		}

		/* We're making progress. */
		else
		{
			/* No worries. */
			desperation = 0;

			/* Count down times until next movement changes. */
			if (adjust_dir_timer > 0) adjust_dir_timer--;
			if (rand_dir_timer > 0) rand_dir_timer--;
			if (correct_dir_timer > 0) correct_dir_timer--;
			if (tunnel_style_timer > 0) tunnel_style_timer--;
			if (cave_length_timer > 0) cave_length_timer--;

			/* Adjust the tunnel style if required */
			if (tunnel_style_timer == 0)
			{
				/* Caves re-pick style less frequently due to hacking style */
				if (!cave_length_timer || (style % 4 == 3)) style = get_tunnel_style();
				tunnel_style_timer = randint(DUN_TUN_STYLE * 2);
			}

			/* Decrease cave tunnel randomness to allow it to find rooms further away */
			if (cave_length_timer == 0)
			{
				if ((style % 4) < 3) style++;
				cave_length_timer = randint(DUN_TUN_LEN * 2);
			}

			/* Get path direction */
			if ((style & (TUNNEL_PATH | TUNNEL_ANGLE)) && (path_i))
			{
				if (path_i < path_n)
				{
					/* Sanity check */
					row_dir = GRID_Y(path[path_i]) - GRID_Y(path[path_i-1]);
					col_dir = GRID_X(path[path_i]) - GRID_X(path[path_i-1]);
				}
				else
				{
					correct_dir_timer = 1;
				}
			}
			
			/* Get path direction */
			if ((style & (TUNNEL_PATH | TUNNEL_ANGLE)) &&
				((correct_dir_timer == 0) || (rand_dir_timer == 0) || (adjust_dir_timer == 0)))
			{
				int tmp_row2 = row2;
				int tmp_col2 = col2;

				/* Start out in correct direction */
				path_n = project_path(path, 512, row1, col1, &tmp_row2, &tmp_col2, (PROJECT_THRU | PROJECT_PASS));

				/* Reset path_i */
				path_i = 0;

				/* Distance to go */
				if (path_n)
				{
					/* Sanity check */
					row_dir = GRID_Y(path[path_i]) - row1;
					col_dir = GRID_X(path[path_i]) - col1;
				}
			}

			/* Make a random turn, set timer. */
			if (rand_dir_timer == 0)
			{
				if ((style & TUNNEL_CAVE) != 0)
				{
					rand_dir_cave(&row_dir, &col_dir, row1, col1);
					rand_dir_timer = randint((DUN_TUN_CAV * (1 + (style % 4))) * 2);
				}
				else
				{
					rand_dir(&row_dir, &col_dir, row1, col1);
					rand_dir_timer = randint(DUN_TUN_RND * 2);
				}

				correct_dir_timer = randint(4);
			}

			/* Adjust direction, set timer. */
			if (adjust_dir_timer == 0)
			{
				adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);

				adjust_dir_timer = randint(DUN_TUN_ADJ * 2) * (((level_flag & (LF1_BURROWS)) != 0) ? 2 : 1);
			}

			/* Go in correct direction. */
			if (correct_dir_timer == 0)
			{
				if ((style & (TUNNEL_CAVE)) != 0)
				{
					/* Allow diagonals */
					correct_dir_cave(&row_dir, &col_dir, row1, col1, row2, col2);
				}
				else
				{
					correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
				}

				/* Don't use again unless needed. */
				correct_dir_timer = -1;
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;

		/* Do not leave the dungeon */
		while (!in_bounds_fully_tunnel(tmp_row, tmp_col))
		{
			/* Fall back to last turn coords */
			if (last_turn <= midpoint)
			{
				row1 = start_row;
				col1 = start_col;
			}
			else
			{
				row1 = dun->tunn[last_turn - 1].y;
				col1 = dun->tunn[last_turn - 1].x;

				if (style & (TUNNEL_PATH | TUNNEL_ANGLE)) correct_dir_timer = 1;
			}

			/* Clear temp flags */
			for (i = last_turn; i < dun->tunn_n; i++)
			{
				play_info[dun->tunn[i].y][dun->tunn[i].x] = 0;
			}

			/* Fall back to last turn */
			dun->tunn_n = last_turn;
			dun->door_n = last_door;
			
			/* Have to fall back all the way for stairs */
			dun->stair_n = first_stair;

			/* Clear decorations up to here */
			for ( ; dun->decor_n > 0; dun->decor_n--)
			{
				if (dun->decor_t[dun->decor_n - 1] < dun->tunn_n) break;
			}

			/* Set last decor */
			if (last_decor > dun->decor_n) last_decor = dun->decor_n;

			/* Back up some more */
			last_turn = (last_turn / 2) + midpoint;
			last_door = first_door;

			/* Adjust direction */
			adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;

			/* Mega-Hack -- Paranoia -- prevent infinite loops */
			if (main_loop_count++ > 2000)
			{
				if (cheat_room) message_add(format("Main loop count exceeded from room %d in partition %d. Aborting.", start_room, part1), MSG_GENERIC);
				abort_and_cleanup = TRUE;
				break;
			}
		}

		/* Avoid "solid" granite walls */
		if (f_info[cave_feat[tmp_row][tmp_col]].flags1 & (FF1_SOLID)) continue;

		/* Avoid itself  */
		if (play_info[tmp_row][tmp_col] == 255)
		{
			deadends++;

			/* Short dead ends allowed */
			if (deadends > 5)
			{
				continue;
			}

			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			continue;
		}
		/* Slowly clear dead end counter */
		else if (deadends > 0)
		{
			deadends--;
		}

		/* Pierce "outer" walls of rooms */
		if ((f_info[cave_feat[tmp_row][tmp_col]].flags1 & (FF1_OUTER)) &&
			(((room_has_flag(tmp_row, tmp_col, (ROOM_EDGED))) == 0) || !(near_edge(tmp_row, tmp_col))))
		{
			int wall1 = dun->wall_n;
			bool door = TRUE;
			bool pillar = FALSE;

			/* Get the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid walls */
			if (f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) continue;
			if (f_info[cave_feat[y][x]].flags1 & (FF1_SOLID)) continue;

			/* No need to decorate */
			dun->decor_n = last_decor;
			decor_flag = TRUE;

			/* Save the wall location */
			if (add_wall(tmp_row, tmp_col))
			{
				/* Add a 'random' wall piercing */
				if (style & (TUNNEL_PATH))
				{
					int r = row_dir ? rand_int(4) : rand_int(2) + 2;

					if ((col_dir) && (r / 2))
					{
						if (!add_wall(tmp_row + col_dir * ((r % 2) ? 1 : -1), tmp_col)) door = FALSE;
					}
					else if (row_dir)
					{
						if (!add_wall(tmp_row, tmp_col + row_dir * ((r % 2) ? 1 : -1))) door = FALSE;
					}
				}

				/* Add a 'left-hand' wall piercing */
				if (style & (TUNNEL_LARGE_L))
				{
					if (add_wall(tmp_row + col_dir, tmp_col - row_dir))
					{
						/* Hack -- add regular pillars to some width 3 corridors */
						if ((((tmp_row + tmp_col) % ((style % 4) + 2)) == 0)
							&& ((style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R))== 0)) pillar = TRUE;
					}
					else
					{
						door = FALSE;
					}
				}

				/* Add a 'right-hand' wall piercing */
				if (style & (TUNNEL_LARGE_R))
				{
					if (pillar) dun->wall_n -= 2;

				 	if (add_wall(tmp_row - col_dir, tmp_col + row_dir))
				 	{
						/* Done */
				 	}
				 	else
				 	{
				 		door = FALSE;
				 	}

				 	if (pillar) dun->wall_n++;
				}
			}

			/* Cancel if can't make all the doors */
			if (!door)
			{
				dun->wall_n = wall1;
				continue;
			}

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Forbid re-entry near these piercings */
			for (i = wall1; i < dun->wall_n; i++)
			{
				for (y = dun->wall[i].y - 1; y <= dun->wall[i].y + 1; y++)
				{
					for (x = dun->wall[i].x - 1; x <= dun->wall[i].x + 1; x++)
					{
						bool doorway;

						doorway = FALSE;

						/* Avoid solidifying areas where we'll end up placing doors */
						for (j = wall1; j < dun->wall_n; j++)
						{
							if ((y == dun->wall[j].y) && (x == dun->wall[j].x)) doorway = TRUE;
						}

						if (doorway) continue;

						/* Convert adjacent "outer" walls as "solid" walls */
						if (add_solid(y, x, cave_feat[y][x]))
						{
							/* Room */
							end_room = room_idx_ignore_valid(tmp_row, tmp_col);

							/* Change the wall to a "solid" wall */
							cave_alter_feat(y, x, FS_SOLID);

							/* Decorate next to the start and/or end of the tunnel with the starting room decorations */
							if ((start_room) && ((room_info[start_room].theme[THEME_SOLID]) || (start_room == end_room)))
							{
								/* Overwrite with alternate terrain from starting room later */
								add_next(y, x, room_info[start_room].theme[THEME_SOLID]);
							}
						}
					}
				}
			}
		}

		/* Travel quickly through rooms -- unless we have to overwrite an edge, or are bridging it
		 * from a non-flooded tunnel */
		else if ((cave_info[tmp_row][tmp_col] & (CAVE_ROOM)) &&
			((flood_tunnel) || ((room_has_flag(tmp_row, tmp_col, (ROOM_BRIDGED))) == 0)) &&
			(((room_has_flag(tmp_row, tmp_col, (ROOM_EDGED))) == 0) || !(near_edge(tmp_row, tmp_col))))
		{
			/* Room */
			end_room = room_idx_ignore_valid(tmp_row, tmp_col);

			/* Different room */
			if ((start_room) && (end_room) && (start_room < DUN_ROOMS)
				&& (end_room < DUN_ROOMS) && (start_room != end_room))
			{
				/* Different room in same partition */
				if (part1 == dun->part[end_room])
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Loop in partition %d. Aborting.", part1), MSG_GENERIC);
					break;
				}

				/* Trying to connect simple room to non-simple room with simple style */
				else if (((room_info[start_room].flags & (ROOM_SIMPLE)) != 0)
						&& ((room_info[end_room].flags & (ROOM_SIMPLE)) == 0) && (style < TUNNEL_STYLE))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to simply connect complex partition %d. Aborting.", part1), MSG_GENERIC);
					break;
				}
				
				/* Trying to connect flooded to unflooded area */
				else if ((flood_tunnel) && ((room_info[end_room].flags & (ROOM_FLOODED)) == 0))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to flood unflooded partition %d. Aborting.", part1), MSG_GENERIC);
					break;
				}

				else if (part1 < CENT_MAX)
				{
					int part2 = dun->part[end_room];

					/* Merging successfully */
					if (cheat_room) message_add(format("Merging partition %d (room %d) with %d (room %d) (length %d).", part1, start_room, part2, end_room, dun->tunn_n), MSG_GENERIC);

					/* Report tunnel style */
					if (cheat_room)
					{
						message_add(format("Tunnel style: %s%s%s%s%s%s", (style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R)) ? "Crypt" : "",
								(style & (TUNNEL_LARGE_L | TUNNEL_LARGE_R)) ? "Large" : "",
								(style & (TUNNEL_CAVE)) ? "Cave" : "",
								(style & (TUNNEL_PATH)) ? "Path" : "",
								(style & (TUNNEL_ANGLE)) ? "Angle" : "",
								(style < (TUNNEL_STYLE)) ? "None" : ""), MSG_GENERIC);
					}
					
					/* Merge partitions */
					for (i = 0; i < dun->cent_n; i++)
					{
						if (dun->part[i] == part2) dun->part[i] = part1;
					}

					/* Accept the location */
					row1 = tmp_row;
					col1 = tmp_col;

					/* We haven't tried a t-intersecton from the midpoint */
					if (!midpoint)
					{
						int d1 = TUNN_MAX - dun->tunn_n, d2;
						
						/* Reduce possible distance by size */
						j = 2;
						if (style & (TUNNEL_LARGE_L | TUNNEL_CRYPT_L | TUNNEL_PATH)) j++;
						if (style & (TUNNEL_LARGE_R | TUNNEL_CRYPT_R | TUNNEL_PATH)) j++;
						d1 /= j;
						
						/* No room found */
						j = -1;
						
						/* Find the nearest room to the midpoint */
						if (d1 >= 3) for (i = 0; i < dun->cent_n; i++)
						{
							/* Ignore start and end if tunnel long enough. Note that this is shorter for wide tunnels */
							if ((dun->tunn_n > 40) && ((j == start_room) || (j == end_room))) continue;
							
							/* Check distance from tunnel midpoint */
							d2 = distance(dun->cent[i].y, dun->cent[i].x, dun->tunn[dun->tunn_n/2].y, dun->tunn[dun->tunn_n/2].x);
							
							if (d2 < d1) { j = i; d1 = d2; }
						}
						
						/* Candidate room found */
						if ((j >= 0) && (part1 != dun->part[j]))
						{
							/* Tunnel has worked up to this point */
							last_door = first_door = dun->door_n;
							first_next = dun->next_n;
							first_stair = dun->stair_n;
							midpoint_solid = dun->solid_n;
							
							/* Try again from the midpoint */
							last_turn = midpoint = dun->tunn_n / 2;
							start_row = tmp_row = row1 = dun->tunn[midpoint].y;
							start_col = tmp_col = col1 = dun->tunn[midpoint].x;
							
							/* To the new destination */
							row2 = dun->cent[j].y;
							col2 = dun->cent[j].x;

							/* Merging successfully */
							if (cheat_room) message_add(format("Trying for partition %d (room %d) from midpoint of partition %d join (length %d).", dun->part[j], j, part1, midpoint), MSG_GENERIC);
						}
					}

					/* Accept tunnel */
					break;
				}
			}
			else
			{
				/* Accept the location */
				row1 = tmp_row;
				col1 = tmp_col;

				/* Set tunnel feature if feature is not a floor, or it is a sewers level */
				if (((cave_feat[tmp_row][tmp_col] != FEAT_FLOOR) || (level_flag & (LF1_SEWER)))
					&& (start_room < DUN_ROOMS))
				{
					start_tunnel = room_info[start_room].theme[THEME_TUNNEL];
				}
				/* Clear tunnel feature if feature is a floor */
				else
				{
					start_tunnel = 0;
				}

				if (start_room < DUN_ROOMS)
				{
					/* Set tunnel decor */
					start_decor = room_info[start_room].theme[THEME_SOLID];
				}

				door_flag = FALSE;
				decor_flag = TRUE;

				/* End tunnel if required */
				if (start_room != end_room) break;
			}
		}

		/* Handle corridor intersections or overlaps */
		else if ((play_info[tmp_row][tmp_col] > 0) && (play_info[tmp_row][tmp_col] < 255))
		{
			bool pillar = FALSE;

			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
			
			/* Prevent encountering tunnel leaving same room */
			if ((start_room == play_info[tmp_row][tmp_col]) && !(--loopiness))
			{
				if (cheat_room) message_add(format("Self-intersecting corridor from room %d in partition %d. Aborting.", start_room, part1), MSG_GENERIC);

				abort_and_cleanup = TRUE;
				break;
			}

			/* Don't put doors in paths/angled tunnels */
			if ((style & (TUNNEL_PATH | TUNNEL_ANGLE)) == 0)
			{
				/* Prevent us following corridor length */
				if ((door_flag) && !(allow_overrun))
				{
					if ((overrun_flag) || (dun->door_n > first_door + 6))
					{
						if (cheat_room) message_add(format("Overrun in doors from room %d in partition %d. Aborting.", start_room, part1), MSG_GENERIC);
	
						abort_and_cleanup = TRUE;
						break;
					}
	
					overrun_flag = TRUE;
				}
	
				/* Allowed left hand side door */
				if (!(door_flag) || (--door_l > 0))
				{
					/* Add 'left-hand' door */
					if (style & (TUNNEL_LARGE_L))
					{
						if (add_door(tmp_row + col_dir, tmp_col - row_dir))
						{
							/* Hack -- add regular pillars to some width 3 corridors */
							if ((((tmp_row + tmp_col) % ((style % 4) + 2)) == 0)
								&& ((style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R))== 0)) pillar = TRUE;
						}
	
						/* Allow door in next grid */
						if (!door_flag) door_l = 3;
					}
				}
	
				/* Allowed left hand side door */
				if (!(door_flag) || (--door_r > 0))
				{
					/* Add 'right-hand' door */
					if (style & (TUNNEL_LARGE_R))
					{
	
						if (add_door(tmp_row + col_dir, tmp_col - row_dir))
						{
							/* Allow door in next grid */
							if (!door_flag) door_r = 3;
						}
					}
					else
					{
						pillar = FALSE;
					}
				}
	
				/* Collect legal door locations */
				if ((!door_flag) && (!pillar))
				{
					/* Save the door location */
					add_door(row1, col1);
	
					/* No door in next grid */
					door_flag = TRUE;
	
					/* Haven't overrun doors yet */
					overrun_flag = FALSE;
				}			
			}

			/* Force decoration in next grid */
			decor_flag = FALSE;

			/* Hack -- allow rooms in the dungeon to join a corridor */
			if ((allow_corridors) && (start_room) && (start_room < DUN_ROOMS) && (start_room != play_info[tmp_row][tmp_col]) && !(--loopiness))
			{
				/* Room */
				end_room = play_info[tmp_row][tmp_col];

				/* Different room in same partition */
				if (part1 == dun->part[end_room])
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Loop in partition %d from corridor join. Aborting.", part1), MSG_GENERIC);
					break;
				}

				/* Trying to connect simple room to non-simple room with simple style */
				else if (((room_info[start_room].flags & (ROOM_SIMPLE)) != 0)
						&& ((room_info[end_room].flags & (ROOM_SIMPLE)) == 0) && (style < TUNNEL_STYLE))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to simply connect complex partition %d with corridor join. Aborting.", part1), MSG_GENERIC);
					break;
				}
				
				/* Trying to connect flooded to unflooded area */
				else if ((flood_tunnel) && ((room_info[end_room].flags & (ROOM_FLOODED)) == 0))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to flood unflooded partition %d with corridor join. Aborting.", part1), MSG_GENERIC);
					break;
				}

				else if (part1 < CENT_MAX)
				{
					int part2 = dun->part[end_room];

					/* Merging successfully */
					if (cheat_room) message_add(format("Merging partition %d (room %d) with %d (room %d) (length %d) through corridor join.", part1, start_room, part2, end_room, dun->tunn_n), MSG_GENERIC);

					/* Report tunnel style */
					if (cheat_room)
					{
						message_add(format("Tunnel style: %s%s%s%s%s%s", (style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R)) ? "Crypt" : "",
								(style & (TUNNEL_LARGE_L | TUNNEL_LARGE_R)) ? "Large" : "",
								(style & (TUNNEL_CAVE)) ? "Cave" : "",
								(style & (TUNNEL_PATH)) ? "Path" : "",
								(style & (TUNNEL_ANGLE)) ? "Angle" : "",
								(style < (TUNNEL_STYLE)) ? "None" : ""), MSG_GENERIC);
					}
					
					/* Merge partitions */
					for (i = 0; i < dun->cent_n; i++)
					{
						if (dun->part[i] == part2) dun->part[i] = part1;
					}
					
					/* Accept tunnel */
					break;
				}
			}
			
			/* Hack -- allow pre-emptive tunnel termination */
			if ((dun->door_n < 3) && (loopiness) && (rand_int(100) >= DUN_TUN_CON))
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0) tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0) tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10))
				{
					if (cheat_room) message_add(format("Prematurely ending tunnel from room %d in partition %d at corridor join.", start_room, part1), MSG_GENERIC);
					break;
				}
			}
		}

		/* Bridge features (always in rooms, unless 'flooding') */
		else if (((f_info[cave_feat[tmp_row][tmp_col]].flags2 & (FF2_PATH)) != 0) ||
			((cave_info[tmp_row][tmp_col] & (CAVE_ROOM)) && !(flood_tunnel)))
		{
			bool left_turn = FALSE;
			bool right_turn = FALSE;

			bool made_tunnel;

			/* Force bridges to consider correct direction frequently */
			if (correct_dir_timer < 0) correct_dir_timer = randint(4);

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Add a bridge location */
			made_tunnel = add_tunnel(row1, col1, 0);

			/* Add a 'random' adjacent grid */
			if ((made_tunnel) && (style & (TUNNEL_PATH)))
			{
				int r = row_dir ? rand_int(4) : rand_int(2) + 2;
				int d;

				/* Pick a random adjacent grid */
				if ((col_dir) && (r / 2))
				{
					d = col_dir * ((r % 2) ? 1 : -1);

					if (f_info[cave_feat[row1 + d][col1]].flags2 & (FF2_PATH))
					{
						made_tunnel = add_tunnel(row1 + d, col1, 0);
					}
				}
				/* Pick a random adjacent grid */
				else if (row_dir)
				{
					d = row_dir * ((r % 2) ? 1 : -1);

					if (f_info[cave_feat[row1][col1 + d]].flags2 & (FF2_PATH))
					{
						made_tunnel = add_tunnel(row1, col1 + d, 0);
					}
				}
			}

			/* Added location */
			else if (made_tunnel)
			{
				/* Are we turning left? */
				if ((row_dir == -old_col_dir) || (col_dir == old_row_dir)) left_turn = TRUE;

				/* Are we turning right? */
				if ((row_dir == old_col_dir) || (col_dir == -old_row_dir)) right_turn = TRUE;

				/* Add a 'left-hand' bridge, but ensure the full bridge is no more than 2 wide */
				if ((style & (TUNNEL_LARGE_L)) && (!(style & (TUNNEL_LARGE_R)) || (style % 2)))
				{
					if (f_info[cave_feat[row1+col_dir][col1-row_dir]].flags2 & (FF2_PATH))
					{
						/* Add a bridge location */
						made_tunnel = add_tunnel(row1 + col_dir, col1 - row_dir, 0);
					}

					/* Add 'diagonally backwards left-hand' bridge if just turned right */
					if (made_tunnel && right_turn)
					{
						/* Add if bridgeable */
						if ((f_info[cave_feat[row1 - row_dir + col_dir ][col1 - col_dir - row_dir]].flags2 & (FF2_PATH)) != 0)
						{
							made_tunnel = add_tunnel(row1 - row_dir + col_dir, col1 - col_dir - row_dir, 0);
						}
					}
				}

				/* Add a 'right-hand' bridge, but ensure the full bridge is no more than 2 wide */
				else if (style & (TUNNEL_LARGE_L | TUNNEL_LARGE_R))
				{
					if (f_info[cave_feat[row1-col_dir][col1+row_dir]].flags2 & (FF2_PATH))
					{
						/* Add a bridge location */
						made_tunnel = add_tunnel(row1 - col_dir, col1 + row_dir, 0);
					}

					/* Add 'diagonally backwards right-hand' bridge if just turned left */
					if (made_tunnel && left_turn)
					{
						/* Add if tunnelable, but not inner, outer or solid */
						if ((f_info[cave_feat[row1 - row_dir - col_dir ][col1 - col_dir + row_dir]].flags2 & (FF2_PATH)) != 0)
						{
							made_tunnel = add_tunnel(row1 - row_dir + col_dir, col1 - col_dir - row_dir, 0);
						}
					}
				}
			}

			/* Ran out of tunnel space */
			if (!made_tunnel)
			{
				if (cheat_room) message_add(format("Tunnel length exceeded from %d (%d, %d) to (%d, %d). Aborting.", part1, start_row, start_col, row2, col2), MSG_GENERIC);

				/* Hack -- themed tunnels can get too long. So try unthemed. */
				/* XXX Caves are usually the culprit here. But they're narrow, so we have more space to try again */
				if (distance(start_row, start_col, row2, col2) > TUNN_MAX / (level_flag & (LF1_CAVE) ? 3 : 6)) level_flag &= ~(LF1_THEME);

				abort_and_cleanup = TRUE;
				break;
			}

			/* Prevent door decoration in next grid */
			door_flag = TRUE;

			/* Force decoration in next grid */
			decor_flag = FALSE;
		}

		/* Tunnel through all other walls and bridge features */
		else if ((f_info[cave_feat[tmp_row][tmp_col]].flags1 & (FF1_TUNNEL))
				|| ((cave_info[tmp_row][tmp_col] & (CAVE_ROOM))))
		{
			bool pillar = FALSE;
			bool decor_l = FALSE;
			bool decor_r = FALSE;
			bool left_turn = FALSE;
			bool right_turn = FALSE;

			bool made_tunnel = TRUE;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/*
			 * Save the tunnel location unless an inner wall
			 * Note hack to differentiate a tunnel from a bridge, if start_tunnel not set.
			 */
			if ((f_info[cave_feat[row1][col1]].flags1 & (FF1_INNER)) == 0)
			{
				made_tunnel = add_tunnel(row1, col1, start_tunnel ? start_tunnel : 1);

				if (made_tunnel)
				{
					/* Did we turn around? Tunnel dead ends are good candidates for stairs.
					 * Note use of decor flag to ensure previous move was a tunnel build.
					 *
					 * XXX Could also have been a move through 'outer' wall. Should be rare
					 * enough not to worry about.
					 */
					if ((decor_flag) && (row_dir == -old_row_dir) && (col_dir == -old_col_dir))
					{
						add_stair(row1 + old_row_dir, col1 + old_col_dir);

						/* Hack -- move decoration to corners of dead end. This 'fakes'
						 * a secret door. */
						if (dun->decor_n > 1)
							for (i = dun->decor_n - 2; i < dun->decor_n; i++)
						{
							dun->decor[i].y += old_row_dir;
							dun->decor[i].x += old_col_dir;
						}

						/* Force decoration */
						decor_flag = FALSE;
					}

					/* Are we turning left? */
					if ((row_dir == -old_col_dir) || (col_dir == old_row_dir)) left_turn = TRUE;

					/* Are we turning right? */
					if ((row_dir == old_col_dir) || (col_dir == -old_row_dir)) right_turn = TRUE;

					/* Do we not need decoration? */
					if ((decor_flag) && (dun->tunn_n % (7 + (style % 4) * 5)))
					{
						dun->decor_n = last_decor;
					}

					/* Save the decoration */
					last_decor = dun->decor_n;

					/* Add a 'random' adjacent grid */
					if (style & (TUNNEL_PATH))
					{
						int r = row_dir ? rand_int(4) : rand_int(2) + 2;
						int d;

						/* Pick a random adjacent grid */
						if ((col_dir) && (r / 2))
						{
							d = col_dir * ((r % 2) ? 1 : -1);

							if (f_info[cave_feat[row1 + d][col1]].flags1 & (FF1_TUNNEL))
							{
								made_tunnel = add_tunnel(row1 + d, col1, 0);
							}
						}
						/* Pick a random adjacent grid */
						else if (row_dir)
						{
							d = row_dir * ((r % 2) ? 1 : -1);

							if (f_info[cave_feat[row1][col1 + d]].flags1 & (FF1_TUNNEL))
							{
								made_tunnel = add_tunnel(row1, col1 + d, 0);
							}
						}
					}

					/* Add width to tunnel if wide or crypt.
					 * Note checks to ensure width 2 tunnels near rooms.
					 */
					if ((style & (TUNNEL_CRYPT_L | TUNNEL_LARGE_L))
						|| ((style & (TUNNEL_CRYPT_R | TUNNEL_LARGE_R))
							&& ((f_info[cave_feat[row1 - col_dir][col1 + row_dir]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)))
					{
						/* Add 'left-hand' tunnel */
						if ((f_info[cave_feat[row1+col_dir][col1-row_dir]].flags1 & (FF1_TUNNEL))
							&& ((f_info[cave_feat[row1 + col_dir][col1 - row_dir]].flags1 & (FF1_OUTER | FF1_SOLID)) == 0)
							&& ((style & (TUNNEL_LARGE_L)) || !((row1 + col1 + style) % 2)))
						{
							/* Save the tunnel location unless an inner wall */
							if ((f_info[cave_feat[row1 + col_dir][col1 - row_dir]].flags1 & (FF1_INNER)) == 0)
							{
								made_tunnel = add_tunnel(row1 + col_dir, col1 - row_dir, 0);

								if (made_tunnel)
								{
									/* Hack -- add regular pillars to some width 3 corridors */
									if ((((row1 + col1) % ((style % 4) + 2)) == 0)
										&& ((style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R))== 0)) pillar = TRUE;

									/* Save the location for later stair allocation */
									if ((style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R))
										&& (crypt_timer-- == 0))
									{
										add_stair(row1 + col_dir, col1 - row_dir);

										crypt_timer = randint(DUN_TUN_CRYPT * 2);
									}

									/* Decorate */
									if (add_decor(row1 + 2 * col_dir, col1 - 2 * row_dir, dun->tunn_n)) decor_l = TRUE;
								}
							}
						}

						/* Add 'diagonally backwards left-hand' tunnel if just turned right */
						if (made_tunnel && right_turn)
						{
							/* Add if tunnelable, but not inner, outer or solid */
							if (((f_info[cave_feat[row1 - row_dir + col_dir ][col1 - col_dir - row_dir]].flags1 & (FF1_TUNNEL)) != 0)
								&& ((f_info[cave_feat[row1 - row_dir + col_dir ][col1 - col_dir - row_dir]].flags1 & (FF1_INNER | FF1_OUTER | FF1_SOLID)) == 0))
							{
								made_tunnel = add_tunnel(row1 - row_dir + col_dir, col1 - col_dir - row_dir, 0);
							}
						}
					}

					/* Add width to tunnel if wide or crypt.
					 * Note checks to ensure width 2 tunnels near rooms.
					 */
					if ((style & (TUNNEL_CRYPT_R | TUNNEL_LARGE_R))
						|| ((style & (TUNNEL_CRYPT_L | TUNNEL_LARGE_L))
							&& ((f_info[cave_feat[row1 + col_dir][col1 - row_dir]].flags1 & (FF1_OUTER | FF1_SOLID)) != 0)))
					{
						/* Add 'right-hand' tunnel */
						if ((f_info[cave_feat[row1-col_dir][col1+row_dir]].flags1 & (FF1_TUNNEL))
							&& ((f_info[cave_feat[row1 - col_dir][col1 + row_dir]].flags1 & (FF1_OUTER | FF1_SOLID)) == 0)
							&& ((style & (TUNNEL_LARGE_R)) || !((row1 + col1 + style / 2) % 2)))
						{
							/* Save the tunnel location unless an 'inner' wall */
							if ((f_info[cave_feat[row1 - col_dir][col1 + row_dir]].flags1 & (FF1_INNER)) == 0)
							{
								if (made_tunnel && pillar)
								{
									dun->tunn_n -= 2; 
									
									/* Note we don't delete PLAY_TEMP here as the pillar counts as a part of the tunnel */
									if (play_info[dun->tunn[dun->tunn_n].y][dun->tunn[dun->tunn_n].x] < 255)
									{
										play_info[dun->tunn[dun->tunn_n].y][dun->tunn[dun->tunn_n].x] = 0;
									}
								}

								made_tunnel = add_tunnel(row1 - col_dir, col1 + row_dir, 0);

								if (made_tunnel)
								{
									/* Save the location for later stair allocation */
									if ((style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R))
										&& (crypt_timer-- == 0))
									{
										add_stair(row1 - col_dir, col1 + row_dir);

										crypt_timer = randint(DUN_TUN_CRYPT * 2);
									}

									/* Decorate */
									if (add_decor(row1 - 2 * col_dir, col1 + 2 * row_dir, dun->tunn_n)) decor_r = TRUE;
								}

								if (made_tunnel && pillar) dun->tunn_n++;
							}
						}

						/* Add 'diagonally backwards right-hand' tunnel if just turned left */
						if (made_tunnel && left_turn)
						{
							/* Add if tunnelable, but not inner, outer or solid */
							if (((f_info[cave_feat[row1 - row_dir - col_dir ][col1 - col_dir + row_dir]].flags1 & (FF1_TUNNEL)) != 0)
								&& ((f_info[cave_feat[row1 - row_dir - col_dir ][col1 - col_dir + row_dir]].flags1 & (FF1_INNER | FF1_OUTER | FF1_SOLID)) == 0))
							{
								made_tunnel = add_tunnel(row1 - row_dir + col_dir, col1 - col_dir - row_dir, 0);
							}
						}
					}
				}
			}

			/* Ran out of tunnel space */
			if (!made_tunnel)
			{
				if (cheat_room) message_add(format("Tunnel length exceeded from %d (%d, %d) to (%d, %d). Aborting.", part1, start_row, start_col, row2, col2), MSG_GENERIC);

				/* Hack -- themed tunnels can get too long. So try unthemed. */
				if (distance(start_row, start_col, row2, col2) > TUNN_MAX / 6) level_flag &= ~(LF1_THEME);

				abort_and_cleanup = TRUE;
				break;
			}

			/* Decorate 'left-hand side' if required */
			if (!decor_l)
			{
				add_decor(row1 + col_dir, col1 - row_dir, dun->tunn_n);
			}

			/* Decorate 'left-hand side' if required */
			if (!decor_r)
			{
				add_decor(row1 - col_dir, col1 + row_dir, dun->tunn_n);
			}

			/* Allow door in next grid */
			door_flag = FALSE;

			/* Don't require decoration next to this grid */
			decor_flag = TRUE;
		}


		/* Record old directions to check for dead ends */
		old_row_dir = row_dir;
		old_col_dir = col_dir;

		/* Fix up diagonals from cave tunnels after 1 move */
		/* Never move diagonally */
		if (row_dir && col_dir)
		{
			if (rand_int(100) < 50)
			{
				row_dir = 0;
			}
			else
			{
				col_dir = 0;
			}
		}

		/* End found */
		if ((row1 == row2) && (col1 == col2))
		{
			/* Room */
			end_room = room_idx_ignore_valid(row1, col1);

			/* Both ends are rooms */
			if ((start_room) && (end_room) &&
				(start_room < DUN_ROOMS) && (end_room < DUN_ROOMS))
			{
				/* Different room in same partition */
				if (part1 == dun->part[end_room])
				{
					if (cheat_room) message_add(format("Loop in partition %d endpoints. Aborting.", part1), MSG_GENERIC);

					abort_and_cleanup = TRUE;
					break;
				}

				/* Trying to connect simple room to non-simple room with simple style */
				else if (((room_info[start_room].flags & (ROOM_SIMPLE)) != 0)
						&& ((room_info[end_room].flags & (ROOM_SIMPLE)) == 0) && (style < TUNNEL_STYLE))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to simply connect complex partition %d. Aborting.", part1), MSG_GENERIC);
					break;
				}
				
				/* Trying to connect flooded to unflooded area */
				else if ((flood_tunnel) && ((room_info[end_room].flags & (ROOM_FLOODED)) == 0))
				{
					abort_and_cleanup = TRUE;
					if (cheat_room) message_add(format("Trying to flood unflooded partition %d. Aborting.", part1), MSG_GENERIC);
					break;
				}

				else if (part1 < CENT_MAX)
				{
					int part2 = dun->part[end_room];

					/* Merging successfully */
					if (cheat_room) message_add(format("Merging partition %d (room %d) with %d (room %d) by reaching end point (length %d).", part1, start_room, part2, end_room, dun->tunn_n), MSG_GENERIC);

					/* Merge partitions */
					for (i = 0; i < dun->cent_n; i++)
					{
						if (dun->part[i] == part2) dun->part[i] = part1;
					}

					/* We haven't tried a t-intersecton from the midpoint */
					if (!midpoint)
					{
						int d1 = TUNN_MAX - dun->tunn_n, d2;
						
						/* Reduce possible distance by size */
						j = 2;
						if (style & (TUNNEL_LARGE_L | TUNNEL_CRYPT_L | TUNNEL_PATH)) j++;
						if (style & (TUNNEL_LARGE_R | TUNNEL_CRYPT_R | TUNNEL_PATH)) j++;
						d1 /= j;
						
						/* No room found */
						j = -1;
						
						/* Find the nearest room to the midpoint */
						if (d1 >= 3) for (i = 0; i < dun->cent_n; i++)
						{
							/* Ignore start and end if tunnel long enough. Note that this is shorter for wide tunnels */
							if ((dun->tunn_n > 40) && ((j == start_room) || (j == end_room))) continue;

							/* Check distance from tunnel midpoint */
							d2 = distance(dun->cent[i].y, dun->cent[i].x, dun->tunn[dun->tunn_n/2].y, dun->tunn[dun->tunn_n/2].x);
							
							if (d2 < d1) { j = i; d1 = d2; }
						}
						
						/* Candidate room found */
						if ((j >= 0) && (part1 != dun->part[j]))
						{
							/* Tunnel has worked up to this point */
							last_door = first_door = dun->door_n;
							first_next = dun->next_n;
							first_stair = dun->stair_n;
							midpoint_solid = dun->solid_n;
							
							/* Try again from the midpoint */
							last_turn = midpoint = dun->tunn_n / 2;
							start_row = tmp_row = row1 = dun->tunn[midpoint].y;
							start_col = tmp_col = col1 = dun->tunn[midpoint].x;
							
							/* To the new destination */
							row2 = dun->cent[j].y;
							col2 = dun->cent[j].x;
							
							/* Merging successfully */
							if (cheat_room) message_add(format("Trying for partition %d (room %d) from midpoint of partition %d join (length %d).", dun->part[j], j, part1, midpoint), MSG_GENERIC);
						}
					}
					
					/* Paranoia */
					abort_and_cleanup = FALSE;
				}
			}
			
			/* Report tunnel style */
			if (cheat_room)
			{
				message_add(format("Tunnel style: %s%s%s%s%s%s", (style & (TUNNEL_CRYPT_L | TUNNEL_CRYPT_R)) ? "Crypt" : "",
						(style & (TUNNEL_LARGE_L | TUNNEL_LARGE_R)) ? "Large" : "",
						(style & (TUNNEL_CAVE)) ? "Cave" : "",
						(style & (TUNNEL_PATH)) ? "Path" : "",
						(style & (TUNNEL_ANGLE)) ? "Angle" : "",
						(style < (TUNNEL_STYLE)) ? "None" : ""), MSG_GENERIC);
			}

			/* Accept tunnel */
			break;
		}

		if (style & (TUNNEL_PATH | TUNNEL_ANGLE))
		{
			/* Increase path count if required */
			path_i++;

			/* Almost at end */
			if (path_i == path_n)
			{
				correct_dir_timer = 1;
			}
		}
	}


	/* We have to cleanup some stuff before returning if we abort */
	if (abort_and_cleanup)
	{
		/* Clear intersections and decorations */
		dun->door_n = first_door;
		dun->next_n = first_next;
		dun->stair_n = first_stair;

		/* Remove the solid walls we applied */
		for (i = midpoint_solid; i < dun->solid_n; i++)
		{
			/* Get the grid */
			y = dun->solid[i].y;
			x = dun->solid[i].x;

			cave_set_feat(y, x, dun->solid_feat[i]);
		}

		/* Clear temp flags */
		for (i = midpoint; i < dun->tunn_n; i++)
		{
			play_info[dun->tunn[i].y][dun->tunn[i].x] = 0;
		}

		/* Finish if we never built a tunnel successfully */
		if (!midpoint) return FALSE;
		
		/* Shorten tunnel */
		dun->tunn_n = midpoint;
		dun->solid_n = midpoint_solid;
		midpoint = 0;
		
		/* Merging successfully */
		if (cheat_room) message_add("Midpoint merge failed.", MSG_GENERIC);
	}

	/* Merging successfully */
	if ((cheat_room) && (midpoint)) message_add("Midpoint merge succeeded.", MSG_GENERIC);

	/* Rewrite tunnel to room if we end up on a non-floor, or we are a sewer level, or the start or end tunnel is not a room, or 20% of the time otherwise */
	if ((cave_feat[row1][col1] != FEAT_FLOOR) || (level_flag & (LF1_SEWER)) || (!start_room) || (!end_room) || (rand_int(100) < 20))
	{
		/* Hack -- overwrite half of tunnel */
		if ((start_tunnel) || (!start_room))
		{
			/* Round up some times */
			if (((dun->tunn_n - first_tunn) % 2) && (rand_int(100) < 50)) first_tunn++;

			/* Adjust from half-way */
			first_tunn = first_tunn + (dun->tunn_n - first_tunn) / 2;
		}

		/* Remove tunnel terrain */
		if ((!start_room) || (!end_room))
		{
			for (i = ((!start_room) ? 0 : first_tunn); i < ((!end_room) ? dun->tunn_n : first_tunn); i++)
			{
				dun->tunn_feat[i] = 0;
				
				/* Mark as a saved corridor */
				play_info[dun->tunn[i].y][dun->tunn[i].x] = start_room;
			}
		}

		/* Overwrite starting tunnel terrain with end tunnel terrain */
		if ((end_room) && (end_room < DUN_ROOMS) && (room_info[end_room].theme[THEME_TUNNEL]))
		{
			for (i = first_tunn; i < dun->tunn_n; i++)
			{
				if (dun->tunn_feat[i]) dun->tunn_feat[i] = room_info[end_room].theme[THEME_TUNNEL];
			}
		}
	}

	/* If the ending room has decorations next to doors, overwrite the start */
	if ((end_room) && (end_room < DUN_ROOMS) && (room_info[end_room].theme[THEME_SOLID]) && (dun->next_n < NEXT_MAX))
	{
		int j;

		for (j = first_next; j < dun->next_n; j++)
		{
			/* Overwrite with alternate terrain from ending room later */
			dun->next_feat[j] = room_info[end_room].theme[THEME_SOLID];
		}
	}
	/* If ending room does not have decorations and neither does start, clear the above 'fake' decorations */
	else if ((start_room) && !(room_info[start_room].theme[THEME_SOLID]) && (start_room != end_room))
	{
		dun->next_n = first_next;
	}

	/* Get end decor */
	if ((end_room) && (end_room < DUN_ROOMS))
	{
		end_decor = room_info[end_room].theme[THEME_SOLID];
	}

	/* Turn the tunnel into corridor */
	for (i = 0; i < dun->tunn_n; i++)
	{
		/* Get the grid */
		y = dun->tunn[i].y;
		x = dun->tunn[i].x;

		/* Clear temp flags */
		if (play_info[y][x] == 255) play_info[y][x] = 0;

		/* Allow doors in tunnels */
		if (dun->tunn_feat[i] > 0)
		{
			/* Mark as a saved corridor */
			play_info[y][x] = start_room;
		}

		/* Apply feature - note hack */
		if (dun->tunn_feat[i] > 1)
		{
			/* Clear previous contents, write terrain */
			cave_set_feat(y, x, dun->tunn_feat[i]);

			continue;
		}

		/* Apply sewer feature or flood corridor */
		if ((flood_tunnel) || ((dun->tunn_feat[i] == 1) && (level_flag & (LF1_SEWER))))
		{
			/* Guarantee flooded feature */
			if (!dun->flood_feat)
			{
				dun->flood_feat = pick_proper_feature(cave_feat_sewer);
				
				/* Hack -- always have a sewer feature */
				if (!dun->flood_feat) dun->flood_feat = FEAT_WATER;
			}

			/* Clear previous contents, write terrain */
			build_terrain(y, x, dun->flood_feat);

			/*
			 * Only accept movement squares terrain in tunnels.
			 * Never fill tunnels with terrain that hits adjacent grids, unless flooded
			 */
			if (((f_info[cave_feat[y][x]].flags1 & (FF1_MOVE))
				|| (f_info[cave_feat[y][x]].flags3 & (FF3_EASY_CLIMB)))
				&& ((flood_tunnel) || ((f_info[dun->flood_feat].flags3 & (FF3_SPREAD | FF3_ADJACENT | FF3_ERUPT)) == 0)))
			{
				continue;
			}

			/* Try the edge of the terrain instead */
			else if ((f_info[f_info[dun->flood_feat].edge].flags1 & (FF1_MOVE)) ||
				(f_info[f_info[dun->flood_feat].edge].flags3 & (FF3_EASY_CLIMB)))
			{
				cave_set_feat(y, x, f_info[dun->flood_feat].edge);
				continue;
			}

			/* Force terrain for sewers */
			else if ((dun->tunn_feat[i] == 1) && (level_flag & (LF1_SEWER)))
			{
				continue;
			}
		}

		/* Apply bridge */
		if (f_info[cave_feat[y][x]].flags2 & (FF2_PATH))
		{
			/* Bridge previous contents */
			cave_alter_feat(y, x, FS_BRIDGE);

			continue;
		}

		/* Apply tunnel */
		if (f_info[cave_feat[y][x]].flags1 & (FF1_TUNNEL))
		{
			/* Tunnel previous contents */
			cave_alter_feat(y, x, FS_TUNNEL);

			continue;
		}
	}

	/* Apply the piercings that we found */
	for (i = 0; i < dun->wall_n; i++)
	{
		/* Get the grid */
		y = dun->wall[i].y;
		x = dun->wall[i].x;

		/* Flood corridor */
		if (flood_tunnel)
		{
			/* Clear previous contents, write terrain */
			build_terrain(y, x, dun->flood_feat);

			/* Only accept movement squares in tunnels */
			if (((f_info[cave_feat[y][x]].flags1 & (FF1_MOVE))
				|| (f_info[cave_feat[y][x]].flags3 & (FF3_EASY_CLIMB)))
				&& ((f_info[dun->flood_feat].flags3 & (FF3_SPREAD | FF3_ADJACENT | FF3_ERUPT)) == 0))
			{
				continue;
			}

			/* Try the edge of the terrain instead */
			else if ((f_info[f_info[dun->flood_feat].edge].flags1 & (FF1_MOVE)) ||
				(f_info[f_info[dun->flood_feat].edge].flags3 & (FF3_EASY_CLIMB)))
			{
				cave_set_feat(y, x, f_info[dun->flood_feat].edge);
				continue;
			}
		}

		/* Convert to doorway if an outer wall */
		if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) != 0)
		{
			cave_alter_feat(y, x, FS_DOOR);
		}

		/* Convert to floor grid */
		else cave_set_feat(y, x, FEAT_FLOOR);

		/* Occasional doorway */
		if (rand_int(100) < DUN_TUN_PEN)
		{
			/* Place a random door */
			place_random_door(y, x);
		}

		/* Place identical doors if next set of doors is adjacent */
		while ((i < dun->wall_n - 1) && (ABS(dun->wall[i+1].y - y) <= 1) && (ABS(dun->wall[i+1].x - x) <= 1))
		{
			cave_set_feat(dun->wall[i+1].y, dun->wall[i+1].x, cave_feat[y][x]);
			i++;
		}
	}

	/* Set up decorations */
	if (!start_decor) start_decor = end_decor;
	if (!end_decor) end_decor = start_decor;

	/*
	 * Hack -- 'no movement'/'no spread' tunnels get flood terrain
	 * added as wall decorations instead.
	 */
	if ((flood_tunnel) && ((((f_info[dun->flood_feat].flags1 & (FF1_MOVE)) == 0)
		&& ((f_info[dun->flood_feat].flags3 & (FF3_EASY_CLIMB)) == 0))
		|| ((f_info[dun->flood_feat].flags3 & (FF3_SPREAD | FF3_ADJACENT | FF3_ERUPT)) != 0)))
	{
		start_decor = dun->flood_feat;
		end_decor = dun->flood_feat;
	}

	/* Apply the decorations that we need */
	if (start_decor) for (i = 0; i < dun->decor_n; i++)
	{
		/* Get the grid */
		y = dun->decor[i].y;
		x = dun->decor[i].x;

		/* TODO: Should do something smarter here */
		/*
		 * We need to walk the decoration back to the previous location if required here
		 */
		if ((f_info[cave_feat[y][x]].flags1 & (FF1_WALL)) == 0) continue;

		/* Prevent vents and other movable paths getting overwritten */
		if ((f_info[cave_feat[y][x]].flags1 & (FF1_MOVE)) != 0) continue;
		if ((f_info[cave_feat[y][x]].flags3 & (FF3_EASY_CLIMB)) != 0) continue;

		/* Convert to decor grid */
		if (dun->decor_t[i] < dun->tunn_n / 2)
		{
			cave_set_feat(y, x, start_decor);
		}
		else
		{
			cave_set_feat(y, x, end_decor);
		}
	}

	/* Clear stairs if flooding tunnel */
	if (flood_tunnel) dun->stair_n = first_stair;

	return TRUE;
}




/*
 * Count the number of "corridor" grids adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y1, x1)"
 *
 * This routine currently only counts actual "empty floor" grids
 * which are not in rooms.  We might want to also count stairs,
 * open doors, closed doors, etc.  XXX XXX
 */
static int next_to_corr(int y1, int x1)
{
	int i, y, x, k = 0;


	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Skip non floors */
		if (!cave_floor_bold(y, x)) continue;

		/* Skip non "empty floor" grids */
		if (cave_feat[y][x] != FEAT_FLOOR) continue;

		/* Skip grids inside rooms */
		if (cave_info[y][x] & (CAVE_ROOM)) continue;

		/* Count these grids */
		k++;
	}

	/* Return the number of corridors */
	return (k);
}



/*
 * Determine if the given location is "between" two walls,
 * and "next to" two corridor spaces.  XXX XXX XXX
 *
 * Assumes "in_bounds_fully(y,x)"
 */
static bool possible_doorway(int y, int x)
{
	/* Count the adjacent corridors */
	if (next_to_corr(y, x) >= 2)
	{
		/* Check Vertical */
		if ((cave_feat[y-1][x] >= FEAT_MAGMA) &&
		    (cave_feat[y+1][x] >= FEAT_MAGMA))
		{
			return (TRUE);
		}

		/* Check Horizontal */
		if ((cave_feat[y][x-1] >= FEAT_MAGMA) &&
		    (cave_feat[y][x+1] >= FEAT_MAGMA))
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}


/*
 * Places door at y, x position if at least 2 walls found
 */
static bool try_door(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return (FALSE);

	/* Ignore walls */
	if (!(f_info[cave_feat[y][x]].flags1 & (FF1_WALL))) return (FALSE);

	/* Ignore room grids */
	if (cave_info[y][x] & (CAVE_ROOM)) return (FALSE);

	/* Occasional door (if allowed) */
	if ((rand_int(100) < DUN_TUN_JCT) && possible_doorway(y, x))
	{
		/* Place a door */
		place_random_door(y, x);

		return (TRUE);
	}

	return (FALSE);
}



/*
 * Type 0 room. Tunnel termination point.
 */
static bool build_type0(int room, int type)
{
	int y0, x0;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, 1, 1)) return (FALSE);

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Type */
		room_info[room].type = type;

		/* Terminate index list */
		room_info[room].section[0] = -1;
	}

	/* Good spot for a player/stair */
	add_stair(y0, x0);

	return (TRUE);
}


/*
 * Type 1, 2 & 3 room. Overlapping, with no 'visible' feature (1), feature on walls (2) or in centre (3).
 */
static bool build_type123(int room, int type)
{
	int y0, x0;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;
	int height, width;
	int symetry = rand_int(100);

	bool light = FALSE;
	int spacing = 1;
	bool pillars = ((level_flag & (LF1_CRYPT)) != 0) || (rand_int(20) == 0);

	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (!flooded) && ((level_flag & (LF1_ISLANDS)) != 0);

	/* Occasional light */
	if (p_ptr->depth <= randint(25)) light = TRUE;

	/* Determine extents of room (a) */
	y1a = randint(4);
	x1a = randint(13);
	y2a = randint(3);
	x2a = randint(9);

	/* Determine extents of room (b) */
	y1b = randint(3);
	x1b = randint(9);
	y2b = randint(4);
	x2b = randint(13);

	/* Sometimes express symetry */
	if (symetry < 30)
	{
		x1a = x2a; x2b = x1b;
	}

	/* Sometimes express symetry */
	if ((symetry < 15) || (symetry > 85))
	{
		y1a = y2a; y2b = y1b;
	}

	/* Calculate dimensions */
	height = MAX(y1a, y1b) + MAX(y2a, y2b) + 3;
	width = MAX(x1a, x1b) + MAX(x2a, x2b) + 3;

	/* Calculate additional space for moat */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (spaced)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Rebalance centre */
	y0 -= (1 + MAX(y2a, y2b) - MAX(y1a, y1b)) / 2;
	x0 -= (1 + MAX(x2a, x2b) - MAX(x1a, x1b)) / 2;

	/* locate room (a) */
	y1a = y0 - y1a - 1;
	x1a = x0 - x1a - 1;
	y2a = y0 + y2a + 1;
	x2a = x0 + x2a + 1;

	/* locate room (b) */
	y1b = y0 - y1b - 1;
	x1b = x0 - x1b - 1;
	y2b = y0 + y2b + 1;
	x2b = x0 + x2b + 1;

	/* Build a polygonal room */
	if (type == ROOM_NORMAL_CONCAVE)
	{
		/* Build the room */
		if (!build_type_concave(room, type, y0, x0, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, FALSE))
		{
			if (spaced)
			{
				height += 2 * BLOCK_HGT;
				width += 2 * BLOCK_WID;
			}
			
			free_space(y0, x0, height, width);
			
			return (FALSE);
		}
	}
	
	/* Build an overlapping room with the above shape */
	else if (!build_overlapping(room, type, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, spacing, 1, NUM_SCATTER, pillars))
	{
		if (spaced)
		{
			height += 2 * BLOCK_HGT;
			width += 2 * BLOCK_WID;
		}
		
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Handle flooding */
	if (spaced)
	{
		int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

		/* Grow the moat around the chambers */
		y1a = MAX(MIN(y1a, y1b) - BLOCK_HGT, 1);
		x1a = MAX(MIN(x1a, x1b) - BLOCK_WID, 1);
		y2a = MIN(MAX(y2a, y2b) + BLOCK_HGT, DUNGEON_HGT - 2);
		x2a = MIN(MAX(x2a, x2b) + BLOCK_WID, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1a, x1a, y2a, x2a, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);
	}

	return (TRUE);
}


/*
 * Type 4 & 5 room. Double-height overlapping, with feature on walls (4) or in the centre (5).
 */
static bool build_type45(int room, int type)
{
	int y0, x0;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;
	int height, width;
	int symetry = rand_int(100);

	bool light = FALSE;
	int spacing = 2 + rand_int(2);
	bool pillars = TRUE;

	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (!flooded) && ((level_flag & (LF1_ISLANDS)) != 0);

	/* Occasional light */
	if (p_ptr->depth <= randint(25)) light = TRUE;

	/* Determine extents of room (a) */
	y1a = randint(13);
	x1a = randint(13) + 6;
	y2a = randint(9);
	x2a = randint(9) + 5;

	/* Determine extents of room (b) */
	y1b = randint(9);
	x1b = randint(9) + 5;
	y2b = randint(13);
	x2b = randint(13) + 6;

	/* Sometimes express symetry */
	if (symetry < 30)
	{
		x1a = x2a; x2b = x1b;
	}

	/* Sometimes express symetry */
	if ((symetry < 20) || (symetry > 90))
	{
		y1a = y2a; y2b = y1b;
	}

	/* Calculate dimensions */
	height = MAX(y1a, y1b) + MAX(y2a, y2b) + 3;
	width = MAX(x1a, x1b) + MAX(x2a, x2b) + 3;

	/* Calculate additional space for moat */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (spaced)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Rebalance centre */
	y0 -= (1 + MAX(y2a, y2b) - MAX(y1a, y1b)) / 2;
	x0 -= (1 + MAX(x2a, x2b) - MAX(x1a, x1b)) / 2;

	/* locate room (a) */
	y1a = y0 - y1a - 1;
	x1a = x0 - x1a - 1;
	y2a = y0 + y2a + 1;
	x2a = x0 + x2a + 1;

	/* locate room (b) */
	y1b = y0 - y1b - 1;
	x1b = x0 - x1b - 1;
	y2b = y0 + y2b + 1;
	x2b = x0 + x2b + 1;

	/* Build a polygonal room */
	if (type == ROOM_LARGE_CONCAVE)
	{
		/* Build the room */
		if (!build_type_concave(room, type, y0, x0, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, TRUE))
		{
			if (spaced)
			{
				height += 2 * BLOCK_HGT;
				width += 2 * BLOCK_WID;
			}
			
			free_space(y0, x0, height, width);
			
			return (FALSE);
		}
	}
	
	/* Build an overlapping room with the above shape */
	else if (!build_overlapping(room, type, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, spacing, 1, NUM_SCATTER + 3, pillars))
	{
		if (spaced)
		{
			height += 2 * BLOCK_HGT;
			width += 2 * BLOCK_WID;
		}
		
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}


	/* Handle flooding */
	if (spaced)
	{
		int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

		/* Grow the moat around the chambers */
		y1a = MAX(MIN(y1a, y1b) - BLOCK_HGT, 1);
		x1a = MAX(MIN(x1a, x1b) - BLOCK_WID, 1);
		y2a = MIN(MAX(y2a, y2b) + BLOCK_HGT, DUNGEON_HGT - 2);
		x2a = MIN(MAX(x2a, x2b) + BLOCK_WID, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1a, x1a, y2a, x2a, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);
	}

	return (TRUE);
}



/*
 * Type 6 room. Triple-height, triple-width overlapping, with feature in the centre.
 */
static bool build_type6(int room, int type)
{
	int y0, x0;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;
	int height, width;
	int symetry = rand_int(100);

	bool light = FALSE;
	int spacing = 4 + rand_int(4);
	bool pillars = TRUE;
	int scale = 1;

	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (!flooded) && ((level_flag & (LF1_ISLANDS)) != 0);

	/* Occasional light */
	if (p_ptr->depth <= randint(25)) light = TRUE;

	/* Occasional fat pillars */
	if (rand_int(100) < p_ptr->depth)
	{
		scale++;
		spacing += 4;
	}

	/* Determine extents of room (a) */
	y1a = randint(8) + 4;
	x1a = randint(26) + 13;
	y2a = randint(6) + 3;
	x2a = randint(18) + 9;

	/* Determine extents of room (b) */
	y1b = randint(6) + 3;
	x1b = randint(18) + 9;
	y2b = randint(8) + 4;
	x2b = randint(26) + 13;

	/* Sometimes express symetry */
	if (symetry < 40)
	{
		x1a = x2a; x2b = x1b;
	}

	/* Sometimes express symetry */
	if ((symetry < 30) || (symetry > 90))
	{
		y1a = y2a; y2b = y1b;
	}

	/* Calculate dimensions */
	height = MAX(y1a, y1b) + MAX(y2a, y2b) + 3;
	width = MAX(x1a, x1b) + MAX(x2a, x2b) + 3;


	/* Calculate additional space for moat */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (spaced)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Rebalance centre */
	y0 -= (1 + MAX(y2a, y2b) - MAX(y1a, y1b)) / 2;
	x0 -= (1 + MAX(x2a, x2b) - MAX(x1a, x1b)) / 2;

	/* locate room (a) */
	y1a = y0 - y1a - 1;
	x1a = x0 - x1a - 1;
	y2a = y0 + y2a + 1;
	x2a = x0 + x2a + 1;

	/* locate room (b) */
	y1b = y0 - y1b - 1;
	x1b = x0 - x1b - 1;
	y2b = y0 + y2b + 1;
	x2b = x0 + x2b + 1;

	/* Build a polygonal room */
	if (type == ROOM_HUGE_CONCAVE)
	{
		/* Build the room */
		if (!build_type_concave(room, type, y0, x0, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, TRUE))
		{
			if (spaced)
			{
				height += 2 * BLOCK_HGT;
				width += 2 * BLOCK_WID;
			}
			
			free_space(y0, x0, height, width);
			
			return (FALSE);
		}
	}
	
	/* Build an overlapping room with the above shape */
	else if (!build_overlapping(room, type, y1a, x1a, y2a, x2a, y1b, x1b, y2b, x2b, light, spacing, scale, NUM_SCATTER + 8, pillars))
	{
		if (spaced)
		{
			height += 2 * BLOCK_HGT;
			width += 2 * BLOCK_WID;
		}
		
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Handle flooding */
	if (spaced)
	{
		int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

		/* Grow the moat around the chambers */
		y1a = MAX(MIN(y1a, y1b) - BLOCK_HGT, 1);
		x1a = MAX(MIN(x1a, x1b) - BLOCK_WID, 1);
		y2a = MIN(MAX(y2a, y2b) + BLOCK_HGT, DUNGEON_HGT - 2);
		x2a = MIN(MAX(x2a, x2b) + BLOCK_WID, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1a, x1a, y2a, x2a, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);
	}

	return (TRUE);
}


/*
 * Type 7 room. Chambers. Build chambers of random size with 30 monsters.
 */
static bool build_type7(int room, int type)
{
	int y0, x0, height, width;
	int y1, x1, y2, x2;
	int size_mod = 0;
	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (flooded) || ((level_flag & (LF1_ISLANDS)) != 0);

	/* Deeper in the dungeon, chambers are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	/* Rooms get (slightly) larger with depth */
	if (p_ptr->depth > rand_range(40, 140)) size_mod = 4;
	else size_mod = 3;

	/* Calculate the room size. */
	height = BLOCK_HGT * size_mod;
	width = BLOCK_WID * (size_mod + rand_int(3));

	/* Calculate additional space for moat */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (spaced)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Calculate the borders of the room. */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;

	/* Make certain the room does not cross the dungeon edge. */
	if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	{
		if (spaced)
		{
			height += 2 * BLOCK_HGT;
			width += 2 * BLOCK_WID;
		}
		
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Build the chambers */
	build_chambers(y1, x1, y2, x2, 30, light);

	/* Handle flooding */
	if (spaced)
	{
		int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

		/* Grow the moat around the chambers */
		y1 = MAX(y1 - BLOCK_HGT, 1);
		x1 = MAX(x1 - BLOCK_WID, 1);
		y2 = MIN(y2 + BLOCK_HGT, DUNGEON_HGT - 2);
		x2 = MIN(x2 + BLOCK_WID, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1, x1, y2, x2, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);

		/* Hack -- 'room' part is not flooded */
		room_info[room].flags &= ~(ROOM_FLOODED);
	}

	/* Set the chamber flags */
	set_room_flags(room, type, light);

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Initialize room description */
		room_info[room].type = ROOM_CHAMBERS;
	}

	return (TRUE);
}


/*
 * Type 8, 9, 10. Either an interesting room, lesser or greater vault.
 */
static bool build_type8910(int room, int type)
{
	vault_type *v_ptr = NULL;
	int y0, x0;
	int height, width;
	int v = -1;
	int i, j;
	int count = 0;

	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (flooded) || ((level_flag & (LF1_ISLANDS)) != 0);

	/* Pick a vault */
	for (i = 0; i < z_info->v_max; i++)
	{
		u32b vault_flag = v_info[i].level_flag;
		
		/* Match vault type */
		if (v_info[i].typ != type) continue;
		
		/* Clear vault themes if we haven't picked a theme yet */
		if ((level_flag & (LF1_THEME)) == 0) vault_flag &= ~(LF1_THEME);
		
		/* Vault flags? */
		if ((vault_flag) && ((level_flag & (vault_flag)) == 0)) continue;

		/* Chance of room entry */
		if (v_info[i].rarity)
		{
			/* Add to chances */
			count += v_info[i].rarity;

			/* Check chance */
			if (rand_int(count) < v_info[i].rarity) v = i;
		}
	}
	
	/* Paranoia - no vault found? */
	if (v <= 0) return (FALSE);
	
	/* Get the vault record */
	v_ptr = &v_info[v];

	if (cheat_room) message_add(format("Building vault %d.", v_ptr - v_info), MSG_GENERIC);
	
	/* Get height and width */
	height = v_ptr->hgt;
	width = v_ptr->wid;

	/* Calculate additional space for moat */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Initialize room description */
	room_info[dun->cent_n].type = ROOM_INTERESTING + type - 8;

	/* Boost the rating */
	rating += v_ptr->rat;

	/* Hack -- Build the vault */
	if (!build_vault(room, y0, x0, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text))
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Handle flooding */
	if (spaced)
	{
		int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

		/* Grow the moat around the chambers */
		int y1 = MAX(y0 - height / 2, 1);
		int x1 = MAX(x0 - width / 2, 1);
		int y2 = MIN(y0 + (width + 1) / 2, DUNGEON_HGT - 2);
		int x2 = MIN(x0 + (width + 1) / 2, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1, x1, y2, x2, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);

		/* Hack -- 'room' part is not flooded */
		room_info[room].flags &= ~(ROOM_FLOODED);
	}

	/* Set the vault / interesting room flags */
	set_room_flags(room, type, FALSE);

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		switch (type)
		{
			case 8:
				/* Initialize room description */
				room_info[room].type = ROOM_INTERESTING;

				break;

			case 9:
				/* Initialize room description */
				room_info[room].type = ROOM_LESSER_VAULT;

				break;

			case 10:
				/* Initialize room description */
				room_info[room].type = ROOM_GREATER_VAULT;

				break;
		}
	}
	
	/* Vault affects the rest of level generation */
	/* XXX This is mostly duplicated from place_rooms and should be refactored */
	if ((level_flag & (LF1_THEME)) == 0)
	{
		/* Hack -- no choice */
		int choice = -1;

		/* Reset count */
		count = 0;

		/* Keep dungeons 'simple' when deeper rooms first encountered */
		if ((v_info[v].min_lev >= 5) && (v_info[v].min_lev >= p_ptr->depth - 6))
		{
			/* Make corridors 'dungeon-like' */
			level_flag &= ~(LF1_THEME);
			level_flag |= (LF1_DUNGEON);
			
			/* Empty rooms for remainder of level */
			i = ROOM_NORMAL;
			
			/* Message */
			if (cheat_room) message_add("One unique room. Rest of level is normal.", MSG_GENERIC);
		}
		/* Pick a theme */
		else for (j = 0; j < 32; j++)
		{
			/* Place nature and destroyed elsewhere */
			if (((1L << j) == (LF1_WILD)) || ((1L << j) == (LF1_DESTROYED))) continue;
			
			/* Pick a theme */
			if ( ((v_info[v].level_flag & (1L << j)) != 0) && (rand_int(++count) == 0)) choice = j;	
		}

		/* Set a theme if picked */
		if (choice >= 0)
		{
			level_flag |= (1L << choice);
			
			/* Message */
			if (cheat_room) message_add(format("Theme chosen for level is %d.", choice), MSG_GENERIC);					
		}

	}
	
	return (TRUE);
}


/*
 * Type 11 or 12. Small or large starburst.
 */
static bool build_type1112(int room, int type)
{
	int y0, x0, dy, dx;

	/* Deeper in the dungeon, starbursts are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	switch (type)
	{
		case 14: case 13: dy = 19; dx = 33; break;
		default: dy = 9; dx = 14; break;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, dy * 2 + 1, dx * 2 + 1)) return (FALSE);

	/* Try building starburst */
	build_type_starburst(room, type, y0, x0, dy, dx, light);

	return (TRUE);
}


/*
 * Type 13, 14, 15. Small, medium or large fractal.
 */
static bool build_type131415(int room, int type)
{
	int y0, x0, height, width;

	byte fractal_type;

	/* Deeper in the dungeon, starbursts are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	switch (type)
	{
		case ROOM_HUGE_FRACTAL: fractal_type = FRACTAL_TYPE_33x65; height = 33; width = 65; break;
		case ROOM_LARGE_FRACTAL: fractal_type = FRACTAL_TYPE_33x33; height = 33; width = 33; break;
		default: fractal_type = FRACTAL_TYPE_17x33; height = 17; width = 33; break;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Build fractal */
	if (!build_type_fractal(room, type, y0, x0, fractal_type, light))
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	return (TRUE);
}


/*
 * Type 16. Lair.
 */
static bool build_type16(int room, int type)
{
	int y0, x0, height, width;
	int y1, x1, y2, x2;
	int size_mod = 0;

	int feat;

	u32b flag = (STAR_BURST_RAW_EDGE | STAR_BURST_MOAT | STAR_BURST_LIGHT);

	/* Deeper in the dungeon, lairs are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	/* Get a feature to surround the lair */
	feat = pick_proper_feature(cave_feat_lake);

	/* Lairs are always constant size */
	size_mod = 2;

	/* Lairs are never flooded */
	room_info[room].flags &= ~(ROOM_FLOODED);

	/* Calculate the room size. */
	height = BLOCK_HGT * size_mod;
	width = BLOCK_WID * (size_mod + rand_int(3));

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height + 2 * BLOCK_HGT, width + 2 * BLOCK_WID)) return (FALSE);

	/* Calculate the borders of the room. */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;

	/* Make certain the room does not cross the dungeon edge. */
	if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Build the chambers */
	build_chambers(y1, x1, y2, x2, 30, light);

	/* Grow the moat around the chambers */
	y1 = MAX(y1 - BLOCK_HGT * 1, 1);
	x1 = MAX(x1 - BLOCK_WID * 1, 1);
	y2 = MIN(y2 + BLOCK_HGT * 1, DUNGEON_HGT - 2);
	x2 = MIN(x2 + BLOCK_WID * 1, DUNGEON_WID  - 2);

	/* Place the moat */
	generate_starburst_room(y1, x1, y2, x2,
		feat, f_info[feat].edge, flag);

	/* Set the chamber flags */
	set_room_flags(room, type, light);

	/* Paranoia */
	if (dun->cent_n < DUN_ROOMS)
	{
		/* Initialize room description */
		room_info[dun->cent_n].type = ROOM_LAIR;
	}

	return (TRUE);
}



/*
 * Type 17, 18, 19. Small, medium or large maze.
 */
static bool build_type171819(int room, int type)
{
	int y0, x0, width_wall, width_path, ydim, xdim, height, width;

	int y1, x1, y2, x2;

	int pool = 0;
	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = flooded || ((level_flag & (LF1_ISLANDS)) != 0);

	/* Maze flags */
	u32b maze_flags = (MAZE_OUTER_N | MAZE_OUTER_S | MAZE_OUTER_E | MAZE_OUTER_W | MAZE_WALL | MAZE_ROOM | MAZE_FILL);

	/* Occasional light */
	if (p_ptr->depth <= randint(25)) maze_flags |= (MAZE_LITE);

	/* Occasional loop */
	if (p_ptr->depth <= randint(25)) maze_flags |= (MAZE_LOOP);

	/* Hack -- maze and no theme: pick one at random */
	if ((level_flag & (LF1_THEME)) == 0)
	{
		switch(randint(10))
		{
			case 1: level_flag |= (LF1_CRYPT); break;
			case 2: level_flag |= (LF1_CAVE); break;
			case 3: level_flag |= (LF1_STRONGHOLD); break;
			case 4: level_flag |= (LF1_SEWER); break;
			case 5: level_flag |= (LF1_CAVERN); break;
			default: level_flag |= (LF1_LABYRINTH); break;
		}
	}

	switch (type)
	{
		case ROOM_HUGE_MAZE: width_wall = rand_int(9) / 3 + 1; width_path = rand_int(15) / 6 + 1; height = 33; width = 65; level_flag |= (LF1_LABYRINTH); break;
		case ROOM_LARGE_MAZE: width_wall = rand_int(15) / 4 + 1; width_path = 1; height = 33; width = 33; break;
		default: width_wall = rand_int(9) / 6 + 1; width_path = rand_int(9) / 6 + 1; height = 17; width = 33; break;
	}

	/*Fill in dead-ends on larger mazes - always on labyrinth levels */
	if ((level_flag & (LF1_LABYRINTH)) || (randint(75) < height + width)) maze_flags |= (MAZE_DEAD | MAZE_SAVE);
	
	/* Ensure bigger paths on stronghold levels. This allows these corridors to connect correctly to the maze. */
	if (level_flag & (LF1_STRONGHOLD)) width_path++;

	/* Fill crypt mazes with pillars */
	else if (level_flag & (LF1_CRYPT))
	{
		width_path = 3;
		maze_flags |= (MAZE_CRYPT);
	}

	/* Fill cave mazes with lots of twisty passages */
	else if (level_flag & (LF1_CAVE | LF1_CAVERN))
	{
		if (level_flag & (LF1_CAVE)) width_path = 1;
		width_wall++;
		maze_flags |= (MAZE_CAVE);
	}

	/* Calculate maze size */
	ydim = (height + width_wall - 1) / (width_wall + width_path);
	xdim = (width + width_wall - 1) / (width_wall + width_path);

	/* Ensure minimums */
	if (ydim < 3) ydim = 3;
	if (xdim < 3) xdim = 3;

	/* Recalculate height and width based on maze size */
	height = (ydim * width_path) + ((ydim - 1) * width_wall) + 2;
	width = (xdim * width_path) + ((xdim - 1) * width_wall) + 2;

	/* Calculate additional space for moat / around islands etc */
	if (spaced)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (spaced)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Dimensions of room */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;

	/* Flooded maze */
	if (flooded)
	{
		/* Flood floor */
		pool = dun->flood_feat;

		if (pool) maze_flags |= (MAZE_FLOOD);
	}
	/* Sewer maze */
	else if (level_flag & (LF1_SEWER))
	{
		/* Guarantee flooded feature */
		while (!pool)
		{
			pool = pick_proper_feature(cave_feat_pool);
		}

		maze_flags |= (MAZE_POOL);
	}

	/* Draw maze */
	if (draw_maze(y1, x1, y2, x2, FEAT_WALL_OUTER, FEAT_FLOOR, width_wall, width_path, pool, maze_flags))
	{
		/* Handle flooding */
		if (spaced)
		{
			int moat = flooded ? dun->flood_feat : pick_proper_feature(cave_feat_island);

			/* Grow the moat around the chambers */
			y1 = MAX(y1 - BLOCK_HGT, 1);
			x1 = MAX(x1 - BLOCK_WID, 1);
			y2 = MIN(y2 + BLOCK_HGT, DUNGEON_HGT - 2);
			x2 = MIN(x2 + BLOCK_WID, DUNGEON_WID  - 2);

			/* Place the moat */
			generate_starburst_room(y1, x1, y2, x2, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);
		}

		/* Set the vault / interesting room flags */
		set_room_flags(room, type, FALSE);

		/* Hack -- 'room' part is not flooded */
		room_info[room].flags &= ~(ROOM_FLOODED);

		return (TRUE);
	}

	/* Failed */
	free_space(y0, x0, height, width);
		
	return (FALSE);
}


/*
 * Build caves based on cellular automata
 */
static bool build_type202122(int room, int type)
{
	int y0, x0, height, width;
	int y1, x1, y2, x2;
	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool spaced = (flooded) || ((level_flag & (LF1_ISLANDS)) != 0);

	int moat = flooded ? dun->flood_feat : 0;
	int floor = spaced ? pick_proper_feature(cave_feat_island) : FEAT_FLOOR;
	int edge = moat ? f_info[moat].edge : 0;
	int pool = 0;

	/* Deeper in the dungeon, caves are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	switch (type)
	{
		case ROOM_HUGE_CAVE: height = 33; width = 65; break;
		case ROOM_LARGE_CAVE: height = 33; width = 33; break;
		default: height = 17; width = 33; break;
	}

	/* Calculate additional space for moat */
	if (moat)
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate original room size */
	if (moat)
	{
		height -= 2 * BLOCK_HGT;
		width -= 2 * BLOCK_WID;
	}

	/* Calculate the borders of the room. */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;

	/* Make certain the room does not cross the dungeon edge. */
	if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);

	/* Make the cave or islands */
	if (!generate_cellular_cave(y1, x1, y2, x2, moat, floor, edge, pool, 0, (CAVE_ROOM) | (light ? (CAVE_LITE) : 0),
			40, 5, 2, 7, 4))
	{
		if (moat)
		{
			height += 2 * BLOCK_HGT;
			width += 2 * BLOCK_WID;
		}
		
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}
	
	/* Handle flooding */
	if (moat)
	{
		/* Grow the moat around the chambers */
		y1 = MAX(y1 - BLOCK_HGT, 1);
		x1 = MAX(x1 - BLOCK_WID, 1);
		y2 = MIN(y2 + BLOCK_HGT, DUNGEON_HGT - 2);
		x2 = MIN(x2 + BLOCK_WID, DUNGEON_WID  - 2);

		/* Place the moat */
		generate_starburst_room(y1, x1, y2, x2, moat, f_info[moat].edge, STAR_BURST_RAW_EDGE | STAR_BURST_MOAT);

		/* Hack -- 'room' part is not flooded */
		room_info[room].flags &= ~(ROOM_FLOODED);
	}

	/* Set the chamber flags */
	set_room_flags(room, type, light);

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Initialize room description */
		room_info[room].type = type;
	}

	return (TRUE);	
}


/*
 * Build a burrows
 */
static bool build_type232425(int room, int type)
{
	int y0, x0, height, width;
	int y1, x1, y2, x2;
	int y, x;
	bool flooded = ((room_info[room].flags & (ROOM_FLOODED)) != 0);
	bool compact;
	bool succeed;
	int ngb_min = 2;
	int ngb_max = 4;
	int connchance = 20;
	int cellnum;
	bool cardinal = FALSE;
	int irregular = randint(3)*25;

	/* Deeper in the dungeon, chambers are less likely to be lit. */
	bool light = (rand_range(25, 60) > p_ptr->depth) ? TRUE : FALSE;

	/* Default floor and edge */
	s16b feat = flooded ? dun->flood_feat : FEAT_FLOOR;
	s16b edge = FEAT_WALL_OUTER;
	s16b inner = FEAT_NONE;
	s16b alloc = FEAT_NONE;
	u32b exclude = (RG1_STARBURST);

	pool_type pool;

	int n_pools = 1; /* Only allow one pool */
	
	/* Hack -- burrows and no theme: pick one at random */
	if ((level_flag & (LF1_THEME)) == 0)
	{
		switch(randint(10))
		{
			case 1: level_flag |= (LF1_CRYPT); break;
			case 2: level_flag |= (LF1_CAVE); break;
			case 3: level_flag |= (LF1_SEWER); break;
			case 4: level_flag |= (LF1_CAVERN); break;
			case 5: level_flag |= (LF1_LABYRINTH); break;
			default: level_flag |= (LF1_BURROWS); break;
		}
	}
	
	/* Pick neighbours based on level theme */
	if (level_flag & (LF1_DESTROYED)) { ngb_min = 1; ngb_max = 1; }
	else if (level_flag & (LF1_LABYRINTH)) { ngb_min = 1; ngb_max = 3; cardinal = TRUE;}
	else if (level_flag & (LF1_CRYPT)) { ngb_min = 2; ngb_max = 3; }
	else if (level_flag & (LF1_CAVE)) { ngb_min = randint(3); ngb_max = ngb_min +rand_int(8-ngb_min); exclude |= (RG1_INNER);}
	else if (level_flag & (LF1_SEWER)) { ngb_min = 3; ngb_max = 5; }
	else if (level_flag & (LF1_CAVERN)) { ngb_min = 1; ngb_max = 8; exclude |= (RG1_INNER);}

	switch (type)
	{
		case ROOM_HUGE_BURROW: height = 33; width = 65; compact = (rand_range(25, 60) > p_ptr->depth); break;
		case ROOM_LARGE_BURROW: height = 33; width = 33; compact = TRUE; break;
		default: height = 17; width = 33; compact = TRUE; break;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate the borders of the room. */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;
	
	/* Make certain the room does not cross the dungeon edge. */
	if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}
	
	/* Set irregular room info */
	set_irregular_room_info(room, type, light, exclude, &feat, &edge, &inner, &alloc, &pool, &n_pools);
	
	/* Mark grid to fill with temp flags */
	for (y = y1 + 1; y < y2 - 1; y++)
	{
		for (x = x1 + 1; x < x2 - 1; x++)
		{
			/* Mark */
			play_info[y][x] |= (PLAY_TEMP);
		}
	}
	
	/* Find maximum number of cells */
	cellnum = (y2 - y1 - 2) * (x2 - x1 - 2);
	
	/* Re-esimate number of cells required */
	cellnum = cellnum_est(cellnum, ngb_min, ngb_max);
	
	/* Build the burrows */
	succeed = generate_burrows(y1 + 1, x1 + 1, y2 - 1, x2 - 1, ngb_min, ngb_max, connchance, cellnum, 
	    feat, 0, compact, (CAVE_ROOM) | (light ? (CAVE_LITE) : 0), (BURROW_TEMP) | (cardinal ? (BURROW_CARD) : 0));

	/* Clear temp flags */
	for (y = y1; y < y2; y++)
	{
		for (x = x1; x < x2; x++)
		{
			/* Clear temp flag */
			if (((play_info[y][x] & (PLAY_TEMP)) != 0) || (x == x1) || (x == x2 - 1) ||
					(y == y1) || (y == y2 - 1))
			{
				int d;
				
				/* Unmark */
				play_info[y][x] &= ~(PLAY_TEMP);

				/* Marked as room - this indicates inner/outer wall */
				if ((cave_info[y][x] & (CAVE_ROOM)) != 0)
				{
					/* Check adjacent squares */
					for (d = 0; d < 8; d++)
					{
						/* On edge of dungeon */
						if (!in_bounds_fully(y + ddy_ddd[d], x + ddx_ddd[d])) break;
						
						/* On outside of room */
						if ((cave_info[y+ddy_ddd[d]][x+ddx_ddd[d]] & (CAVE_ROOM)) == 0) break;
					}
					
					/* Not on edge of room */
					if (d == 8)
					{
						cave_set_feat(y, x, inner ? inner : edge);
						
						if ((!inner) && ((f_info[edge].flags1 & (FF1_OUTER)) != 0)) cave_alter_feat(y, x, FS_INNER);
					}
					else
					{
						/* Hack -- irregular rooms */
						if (rand_int(100) < irregular) cave_set_feat(y, x, FEAT_WALL_OUTER);
						
						/* Set the cave edge */
						else cave_set_feat(y, x, edge);
						
						/* Not a 'proper' outer wall */
						if ((f_info[cave_feat[y][x]].flags1 & (FF1_OUTER)) == 0)
						{
							/* Clear the room flag */
							cave_info[y][x] &= ~(CAVE_ROOM);
						}
						
						/* Allow light to spill out of rooms through transparent edges */
						if (light)
						{
							/* Allow lighting up rooms to work correctly */
							if (f_info[cave_feat[y][x]].flags1 & (FF1_LOS))
							{
								int d;

								/* Look in all directions. */
								for (d = 0; d < 8; d++)
								{
									/* Extract adjacent location */
									int yy = y + ddy_ddd[d];
									int xx = x + ddx_ddd[d];

									/* Hack -- light up outside room */
									cave_info[yy][xx] |= (CAVE_GLOW);
								}
							}
						}
					}
				}
			}			
		}
	}
	
	/* Build pool */
	if (n_pools == 2)
	{
		succeed = generate_burrows(y1, x1, y2, x2, ngb_min, ngb_max, connchance, (cellnum + 7)/4, 
		    pool[n_pools-1], cave_feat[(y1 + y2)/2][(x1 + x2)/2], compact, (CAVE_ROOM) | (light ? (CAVE_LITE) : 0),0);
	}
	
	/* Build alloc */
	if (alloc)
	{
		cave_set_feat((y1 + y2)/2, (x1 + x2)/2, alloc);
	}
	
	if (!succeed)
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Paranoia */
	if (room < DUN_ROOMS)
	{
		/* Initialize room description */
		room_info[room].type = type;
	}

	return (TRUE);
}



/* The number of different races used for the pit ecology */
/* XXX We assume this is at least 16 */
#define NUM_PIT_RACES	16

/*
 * Monster pits
 *
 * A monster pit is a large room, with an inner room filled with monsters.
 * 
 * Monster pits will never contain unique monsters.
 *
 * Note:
 * Monster pits are fun, but they are among the chief monster (and thus
 * object) inflators going.  We have therefore reduced their size in many
 * cases.
 */
static bool build_type26(int room, int type)
{
	int y, x, y0, x0, y1, x1, y2, x2;
	int i, j;

	int width;

	bool ordered = FALSE;
	int light = FALSE;

	/* Save level rating */
	int old_rating = rating;

	/* Save ecology start */
	int ecology_start = cave_ecology.num_races;
	bool old_ecology_ready = cave_ecology.ready;

	/* Ensure we have empty space in the ecology for the monster races */
	if (cave_ecology.num_races + NUM_PIT_RACES >= MAX_ECOLOGY_RACES) return (FALSE);

	/* Pick a room size (less border walls) */
	y = 9;
	x = (one_in_(3) ? 23 : 13);

	/* Save the width */
	width = (x-5) / 2;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, y+2, x+2)) return (FALSE);

	/* Locate the room */
	y1 = y0 - y / 2;
	x1 = x0 - x / 2;
	y2 =  y1 + y - 1;
	x2 =  x1 + x - 1;

	/* Check bounds */
	if (!(in_bounds_fully(y1, x1)) || !(in_bounds_fully(y2, x2))) return (FALSE);

	/* Allow tougher monsters */
	monster_level = p_ptr->depth + 3 + div_round(p_ptr->depth, 12);

	/* Make space for the ecology of the pit */
	if (cave_ecology.num_ecologies < MAX_ECOLOGIES) cave_ecology.num_ecologies++;
	
	/* Pit gets its own ecology */
	room_info[room].ecology |= (1L << (cave_ecology.num_ecologies - 1)) | (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));

	/* Use ecology of room */
	cave_ecology.ready = FALSE;

	/* Get the ecology */
	get_monster_ecology(0, NUM_PIT_RACES);

	/* Clear ecology use */
	cave_ecology.ready = old_ecology_ready;

	/* Stop allowing tougher monsters */
	monster_level = p_ptr->depth;
	
	/* Remove duplicate races from pit */
	for (i = ecology_start; i < cave_ecology.num_races; i++)
	{
		for (j = i + 1; j < cave_ecology.num_races; j++)
		{
			/* Duplicate detected */
			if (cave_ecology.race[i] == cave_ecology.race[j])
			{
				cave_ecology.race[j] = cave_ecology.race[cave_ecology.num_races - 1];
				cave_ecology.num_races--;

				/* Need to recheck swapped in race */
				j--;
			}
		}
	}

	/* Need at least 3 different races in pit */
	if (ecology_start + 2 >= cave_ecology.num_races)
	{
		cave_ecology.num_races = ecology_start;

		return(FALSE);
	}

	/* Deepest race */
	room_info[room].deepest_race = deeper_monster(room_info[room].deepest_race, cave_ecology.deepest_race[cave_ecology.num_ecologies - 1]);

	/* Shallower pit monsters are found outside the pit
	   We use this to ramp up the difficulty near the pit to warn
	 * the player and make it harder to exploit.
	 */
	for (j = ecology_start; j < cave_ecology.num_races; j++)
	{
		/* Deepest monster */
		if (r_info[cave_ecology.race[j]].level < (p_ptr->depth + r_info[room_info[room].deepest_race].level) / 2)
		{
			/* Appears in rooms near the pit */
			cave_ecology.race_ecologies[j] |= (room_info[room].ecology & ((1L << MAX_ECOLOGIES) - 1));
		}
		else
		{
			/* Appears in the pit only */
			cave_ecology.race_ecologies[j] |= (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));
		}
	}

	/* Generate new room.  Quit immediately if out of bounds. */
	generate_room(y1-1, x1-1, y2+1, x2+1, light);

	/* Generate outer walls */
	generate_rect(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_OUTER);

	/* Generate inner floors */
	generate_fill(y1, x1, y2, x2, FEAT_FLOOR);

	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* Generate inner walls */
	generate_rect(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_INNER);

	/* Open the inner room with a secret door */
	generate_hole(y1-1, x1-1, y2+1, x2+1, FEAT_SECRET);

	/* Order on the basis of intelligence of the deepest race */
	ordered = ((r_info[cave_ecology.deepest_race[room]].flags2 & (RF2_SMART)) != 0);

	/* Arrange the monsters in the room randomly. */
	if (!ordered)
	{
		int r_idx = 0;

		/* Place some monsters */
		for (y = y0 - 2; y <= y0 + 2; y++)
		{
			for (x = x0 - width; x <= x0 + width; x++)
			{
				int r = ecology_start + rand_int(cave_ecology.num_races - ecology_start);

				/* Get a monster index */
				r_idx = cave_ecology.race[r];

				/* Place a single monster */
				(void)place_monster_aux(y, x, r_idx, FALSE, FALSE, 0L);

				/* If a unique, fill in the gap */
				if (r_info[r_idx].flags1 & (RF1_UNIQUE))
				{
					cave_ecology.race[r] = cave_ecology.race[cave_ecology.num_races - 1];
					if (cave_ecology.num_races > ecology_start + 1) cave_ecology.num_races--;
				}
			}
		}
	}

	/* Arrange the monsters in the room in an orderly fashion. */
	else
	{
		s16b what[NUM_PIT_RACES];

		/* Pick some monster types */
		for (i = 0; i < NUM_PIT_RACES; i++)
		{
			/* Get a monster index. */
			/* Paranoia in case we were unable to get NUM_PIT_RACES */
			what[i] = cave_ecology.race[ecology_start + (i * (cave_ecology.num_races - ecology_start)) / NUM_PIT_RACES];
		}

		/* Sort the monsters */
		for (i = 0; i < NUM_PIT_RACES - 1; i++)
		{
			for (j = 0; j < NUM_PIT_RACES - 1; j++)
			{
				int i1 = j;
				int i2 = j + 1;

				int p1 = r_info[what[i1]].level;
				int p2 = r_info[what[i2]].level;

				/* Bubble sort */
				if ((p1 > p2) || (r_info[what[i1]].flags1 & (RF1_UNIQUE)))
				{
					int tmp = what[i1];
					what[i1] = what[i2];
					what[i2] = tmp;
				}
			}
		}

		/* Handle both widths */
		if (width == 9)
		{
			/* Top and bottom rows (outer) */
			for (x = x0 - 9; x <= x0 - 4; x++)
			{
				place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE, 0L);
			}
			for (x = x0 + 4; x <= x0 + 9; x++)
			{
				place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE, 0L);
			}

			/* Top and bottom rows (inner) */
			for (x = x0 - 3; x <= x0 + 3; x++)
			{
				place_monster_aux(y0 - 2, x, what[3], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[3], FALSE, FALSE, 0L);
			}

			/* Outer and middle columns */
			for (y = y0 - 1; y <= y0 + 1; y++)
			{
				place_monster_aux(y, x0 - 9, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 9, what[2], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 8, what[4], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 8, what[4], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 7, what[5], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 7, what[5], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 6, what[6], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 6, what[6], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 5, what[7], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 5, what[7], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 4, what[8], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 4, what[8], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 3, what[9], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 3, what[9], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 2, what[11], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 2, what[11], FALSE, FALSE, 0L);
			}
		}
		else if (width == 4)
		{
			/* Top and bottom rows (outer) */
			for (x = x0 - 4; x <= x0 - 3; x++)
			{
				place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE, 0L);
			}
			for (x = x0 + 3; x <= x0 + 4; x++)
			{
				place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE, 0L);
			}

			/* Top and bottom rows (inner) */
			for (x = x0 - 2; x <= x0 + 2; x++)
			{
				place_monster_aux(y0 - 2, x, what[3], FALSE, FALSE, 0L);
				place_monster_aux(y0 + 2, x, what[3], FALSE, FALSE, 0L);
			}

			/* Outer and middle columns */
			for (y = y0 - 1; y <= y0 + 1; y++)
			{
				place_monster_aux(y, x0 - 4, what[2], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 4, what[2], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 3, what[6], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 3, what[6], FALSE, FALSE, 0L);

				place_monster_aux(y, x0 - 2, what[9], FALSE, FALSE, 0L);
				place_monster_aux(y, x0 + 2, what[9], FALSE, FALSE, 0L);
			}
		}

		/* Above/Below the center monster */
		for (x = x0 - 1; x <= x0 + 1; x++)
		{
			place_monster_aux(y0 + 1, x, what[12], FALSE, FALSE, 0L);
			place_monster_aux(y0 - 1, x, what[12], FALSE, FALSE, 0L);
		}

		/* Next to the center monster */
		place_monster_aux(y0, x0 + 1, what[14], FALSE, FALSE, 0L);
		place_monster_aux(y0, x0 - 1, what[14], FALSE, FALSE, 0L);

		/* Center monster */
		place_monster_aux(y0, x0, what[15], FALSE, FALSE, 0L);
	}

	/* Describe */
	if (cheat_room) msg_print("Monster pit");

	/*
	 * Because OOD monsters in monster pits are generally less dangerous than
	 * they are when wandering around, apply only one-half the difference in
	 * level rating, and then add something for the sheer quantity of monsters.
	 */
	i = rating - old_rating;
	rating = old_rating + (i / 2) + (width <= 4 ? 5 : 10);

	/* Set the vault / interesting room flags */
	set_room_flags(room, type, FALSE);

	/* Success */
	return (TRUE);
}
#if 0

/*
 * Monster towns
 *
 * A monster town is a large area, with inner rooms which correspond to
 * buildings. It is the only room type that may have 'genuine' sub
 * rooms in the sense that the subrooms are treated as a fully fledged
 * room from the point of view of descriptions and room connectivity.
 * Note that it will be possible to surround other rooms with townships
 * so that this may not be strictly correct.
 * 
 * Towns buildings consist of a combination of two building types:
 * 'simple' buildings which contain locked doors and are usually
 * empty, and 'complex' buildings which are generated using the standard
 * town algorithm. The town inhabitants wander around from building to
 * building and are by default set to 'neutral' so that the player is
 * not immediately subject to attack. As a result, it is possible for
 * the player to wander around town. Inhabitants will notice and attack
 * the player occasionally - depending on the player's stealth and
 * proximity, but it is very dangerous for him to fight back because
 * 'neutral' monsters will alert other townsfolk to join the fight
 * for a variety of reasons (including their death being seen by another
 * monster).
 * 
 * Note we don't calculate additional space for the moat, both to make
 * the larger towns easier to place, and to create lots of back alleys
 * around the edges of the outermost buildings if we have them.
 */
static bool build_type27(int room, int type)
{
	int y, x, y0, x0, y1, x1, y2, x2;
	int i, j;

	int width;

	bool ordered = FALSE;
	int light = FALSE;

	/* Save level rating */
	int old_rating = rating;
	
	/* Save ecology start */
	int ecology_start = cave_ecology.num_races;
	bool old_ecology_ready = cave_ecology.ready;

	/* Race flags for founder */
	monster_race *r_ptr;
	
	int height, width, size_mod;
	
	/* Terrain for room */
	int floor = FEAT_FLOOR;
	int wall = FEAT_WALL_INNER;
	
	/* Allow tougher monsters */
	monster_level = p_ptr->depth + 3 + div_round(p_ptr->depth, 12);

	/* Make space for the ecology of the pit */
	if (cave_ecology.num_ecologies < MAX_ECOLOGIES) cave_ecology.num_ecologies++;

	/* Town gets its own ecology */
	room_info[room] |= (1L << (cave_ecology.num_ecologies - 1)) | (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));

	/* Use ecology of room */
	cave_ecology.ready = FALSE;

	/* Choose appropriate monsters and relationships for town */
	cave_ecology.town = TRUE;
	
	/* Get a founding monster for the town */
	
	/* Set monster hook */
	get_mon_num_hook = dun_level_mon;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Get the ecology */
	get_monster_ecology(0, NUM_TOWN_RACES);

	/* Clear monster hook */
	get_mon_num_hook = NULL;

	/* Prepare allocation table */
	get_mon_num_prep();
	
	/* Stop allowing tougher monsters */
	monster_level = p_ptr->depth;
	
	/* Remove duplicate races from town */
	for (i = ecology_start; i < cave_ecology.num_races; i++)
	{
		for (j = i + 1; j < cave_ecology.num_races; j++)
		{
			/* Duplicate detected */
			if (cave_ecology.race[i] == cave_ecology.race[j])
			{
				cave_ecology.race[j] = cave_ecology.race[cave_ecology.num_races - 1];
				cave_ecology.num_races--;

				/* Need to recheck swapped in race */
				j--;
			}
		}
	}

	/* XXX We hackily modify the RF9_TOWNSFOLK flag to keep track of which
	 * monsters are 'native' to town. */
	for (i = 0; i < z_ptr->m_max; i++)
	{
		monster_race *r_ptr = &r_info[i];
		monster_lore *l_ptr = &l_list[i];
		
		/* Don't touch existing town monsters */
		if (!r_ptr->level) continue;
		
		/* Clear the flag */
		r_ptr->flags9 &= ~(RF9_TOWNSFOLK);
		l_ptr->flags9 &= ~(RF9_TOWNSFOLK);
	}

	/* Mark the above added monsters as townsfolk */
	for (i = ecology_start; i < cave_ecology.num_races; i++)
	{
		monster_race *r_ptr = &r_info[cave_ecology.race[i]];

		/* Hack - Skip tunnelling monsters so they don't destroy town */
		if (r_ptr->flags2 & (RF2_KILL_WALL)) continue;
		
		/* Set the flag */
		r_ptr->flags9 |= (RF9_TOWNSFOLK);
	}
	
	/* XXX We also generate some client species to keep town interesting */
	
	/* Allow any relationships */
	cave_ecology.town = FALSE;
	
	/* Get the ecology */
	get_monster_ecology(0, NUM_TOWN_CLIENT_RACES);

	/* Clear ecology use */
	cave_ecology.ready = old_ecology_ready;

	/* Need at least 3 different races in town */
	if (ecology_start + 2 >= cave_ecology.num_races)
	{
		cave_ecology.num_races = ecology_start;

		return(FALSE);
	}

	/* Deepest race */
	room_info[room].deepest_race = deeper_monster(room_info[room].deepest_race, cave_ecology.deepest_race[cave_ecology.num_ecologies - 1]);
	
	/* Shallower town monsters are found outside the town
	   We use this to ramp up the difficulty near the town to warn
	 * the player and make it harder to exploit.
	 */
	for (j = ecology_start; j < cave_ecology.num_races; j++)
	{
		/* Deepest monster */
		if (r_info[cave_ecology.race[j]].level < (p_ptr->depth + r_info[room_info[room].deepest_race].level) / 2)
		{
			/* Appears in rooms near the town */
			cave_ecology.race_ecologies[j] |= (room_info[room].ecology & ((1L << MAX_ECOLOGIES) = 1));
		}
		else
		{
			/* Appears in the town only */
			cave_ecology.race_ecologies[j] |= (1L << (MAX_ECOLOGIES + cave_ecology.num_ecologies - 1));
		}
	}

	
	/*** Now we generate the town ***/

	/* The basic town design doesn't work very well for monsters which can't open doors.
	 * So under some circumstances, we generate alternate town layouts.
	 * 
	 * For flying monsters and dragons, we generate a 'roost' - just a large flooded cellular cave.
	 * For monsters with weird minds or webs, we generate a 'weird web' - a starburst room filled
	 * with polygon shaped buildings made of webs.
	 * For undead which can't pass through walls and animals, we generate abandoned towns - we just
	 * don't mark the room as a town, so no monster will use the town AI for movement. However, the
	 * monsters will ignore the player to start with as they will still be generated with the town
	 * AI flag.
	 * For all other monster types, we don't place them in rooms when placing the monsters if they
	 * can't open doors. This usually means we get weird behaviour with demons and golems, but I'm
	 * happy for this weird behaviour for those types of monsters.
	 * 
	 * Note we only check the 'founding' monster in order to determine this. If the founder is fine
	 * we have at least one monster capable of moving through town correctly, which should be
	 * enough.
	 */
	
	/* Get founder flags */
	r_ptr = &r_info[cave_ecology.race[ecology_start]];

	/* How big is the town? Answer: Really big! */
	size_mod = 2 + rand_int(1 + p_ptr->depth / 20);
	
	/* Calculate the room size. */
	height = BLOCK_HGT * size_mod;
	width = BLOCK_WID * (size_mod + randint(size_mod) + rand_int(3));

	/* Even bigger for a roost */
	if (((r_ptr->flags2 & (RF2_CAN_FLY)) != 0) || ((r_ptr->flags3 & (RF3_DRAGON)) != 0))
	{
		height += 2 * BLOCK_HGT;
		width += 2 * BLOCK_WID;
	}
	
	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) return (FALSE);

	/* Calculate the borders of the room. */
	y1 = y0 - (height / 2);
	x1 = x0 - (width / 2);
	y2 = y0 + (height - 1) / 2;
	x2 = x0 + (width - 1) / 2;

	/* Make certain the room does not cross the dungeon edge. */
	if ((!in_bounds_fully(y1, x1)) || (!in_bounds_fully(y2, x2)))
	{
		free_space(y0, x0, height, width);
		
		return (FALSE);
	}

	/* Generate a 'roost' - for flying monsters and dragons*/
	if (((r_ptr->flags2 & (RF2_CAN_FLY)) != 0) || ((r_ptr->flags3 & (RF3_DRAGON)) != 0))
	{
		/* Generate roost */
		if (!generate_cellular_cave(y1 + BLOCK_HGT, x1 + BLOCK_WID, y2 - BLOCK_HGT, x2 - BLOCK_WID, moat, floor, edge, pool, 0, (CAVE_ROOM) | (light ? (CAVE_LITE) : 0),
					40, 5, 2, 7, 4))
		{
			free_space(y0, x0, height, width);
			
			return (FALSE);
		}
		
		/* Hack - describe this as a huge cellular cave */
		type = ROOM_HUGE_CAVE;
	}
	
	/* Generate a 'weird web' - for web-crawlers and weird monsters */
	else if ((r_ptr->flags2 & (RF2_WEIRD_MIND | RF2_HAS_WEB)) != 0)
	{
		/* Save level flags */
		int old_level_flag = level_flag;

		/* Hack -- ensure polygonal rooms in town */
		level_flag |= (LF1_POLYGON);
		
		/* Build the town */
		generate_town(y1, x1, y2, x2, room, ground, FEAT_WEB, ground, light)
		
		/* Restore level flag */
		level_flag = old_level_flag;
		
		/* Hack - describe this as a huge cellular cave */
		type = ROOM_HUGE_CAVE;
	}
	
	/* Generate a normal town */
	else
	{
		/* Build the town */
		generate_town(y1, x1, y2, x2, room, ground, wall, floor, bool light)		
	}
		
	/* Grow the moat around the town */
	y1 = MAX(y1 - BLOCK_HGT * 1, 1);
	x1 = MAX(x1 - BLOCK_WID * 1, 1);
	y2 = MIN(y2 + BLOCK_HGT * 1, DUNGEON_HGT - 2);
	x2 = MIN(x2 + BLOCK_WID * 1, DUNGEON_WID  - 2);

	/* Place the moat */
	generate_starburst_room(y1, x1, y2, x2, floor, f_info[floor].edge, flag);

	/*** Now we get to place the monsters. ***/

	/* Place the monsters. */
	for (i = 0; i < 300; i++)
	{
		/* Check for early completion. */
		if (!monsters_left) break;

		/* Pick a random in-room square. */
		y = rand_range(y1, y2);
		x = rand_range(x1, x2);

		/* Require a floor square with no monster in it already. */
		if (!cave_naked_bold(y, x)) continue;

		/* Enforce monster selection */
		cave_ecology.use_ecology = room_info[room].ecology;

		/* Place a single monster.  Sleeping 2/3rds of the time. */
		place_monster_aux(y, x, get_mon_num(monster_level),
			(bool)(rand_int(3)), FALSE, 0L);

		/* End enforcement of monster selection */
		cave_ecology.use_ecology = (0L);

		/* One less monster to place. */
		monsters_left--;
	}

	/*
	 * Because OOD monsters in monster towns are generally less dangerous than
	 * they are hostile, apply only one-half the difference in
	 * level rating, and then add something for the sheer quantity of monsters.
	 */
	i = rating - old_rating;
	rating = old_rating + (i / 2) + (width <= 4 ? 5 : 10);

	/* Set the vault / interesting room flags */
	set_room_flags(room, type, FALSE);

	/* Mark the room as a town unless the founder is an animal or an undead which can't pass walls */
	if (((r_ptr->flags3 & (RF3_UNDEAD | RF3_ANIMAL)) == 0) && ((r_ptr->flags3 & (RF2_PASS_WALL)) == 0))
	{
		/* Townsfolk use the town AI to navigate through this room */
		room_info[room].flags |= (ROOM_TOWN);
	}
	
	/* Set the chamber flags */
	set_room_flags(room, type, light);
	
	/* Success */
	return (TRUE);

}
#endif


/*
 * Attempt to build a room of the given type.
 */
static bool room_build(int room, int type)
{
	/* Flood if required */
	if (dun->flood_feat)
	{
		/* Always flood the first room */
		if (room == 1) room_info[room].flags |= (ROOM_FLOODED);

		/* Flood additional rooms if deep in dungeon - up to 2/3rds of all rooms */
		else if ((room % 3) && (room < (p_ptr->depth - f_info[dun->flood_feat].level) / 5)) room_info[room].flags |= (ROOM_FLOODED);

		/* This floods 1 room, plus a second room if terrain is more than 10 levels shallow, plus a third room if terrain is more than
		 * 20 levels shallow, then 25, 35, 40, 50, 55 etc. */
	}
	
	/* Keep dungeons somewhat 'simple' when deeper rooms first encountered */
	if ((room > 1) && (room % 2) && (room_data[room_info[1].type].min_level >= 5) && (room_data[room_info[1].type].min_level >= p_ptr->depth - 12))
	{
		/* Make empty rooms half the time */
		type = ROOM_NORMAL;
	}
	
	/* Generating */
	if (cheat_room) message_add(format("Building room %d as type %d.", room, type), MSG_GENERIC);

	/* Build a room */
	switch (type)
	{
		/* Build an appropriate room */
		case ROOM_MONSTER_TOWN: /*if (build_type27(room, type)) return (TRUE); break;*/
		case ROOM_MONSTER_PIT: if (build_type26(room, type)) return (TRUE); break;
		case ROOM_HUGE_BURROW:
		case ROOM_LARGE_BURROW:
		case ROOM_BURROW: if (build_type232425(room,type)) return(TRUE); break;
		case ROOM_HUGE_CAVE:
		case ROOM_LARGE_CAVE:
		case ROOM_CELL_CAVE: if (build_type202122(room,type)) return(TRUE); break;
		case ROOM_HUGE_MAZE:
		case ROOM_LARGE_MAZE:
		case ROOM_MAZE: if (build_type171819(room,type)) return(TRUE); break;
		case ROOM_LAIR: if (build_type16(room, type)) return(TRUE); break;
		case ROOM_HUGE_FRACTAL:
		case ROOM_LARGE_FRACTAL:
		case ROOM_FRACTAL: if (build_type131415(room, type)) return(TRUE); break;
		case ROOM_HUGE_STAR_BURST:
		case ROOM_STAR_BURST: if (build_type1112(room, type)) return(TRUE); break;
		case ROOM_GREATER_VAULT:
		case ROOM_LESSER_VAULT:
		case ROOM_INTERESTING: if (build_type8910(room, type)) return(TRUE); break;
		case ROOM_CHAMBERS: if (build_type7(room, type)) return(TRUE); break;
		case ROOM_HUGE_CONCAVE:
		case ROOM_HUGE_CENTRE: if (build_type6(room, type)) return(TRUE); break;
		case ROOM_LARGE_CONCAVE:
		case ROOM_LARGE_CENTRE:
		case ROOM_LARGE_WALLS: if (build_type45(room, type)) return(TRUE); break;
		case ROOM_NORMAL_CONCAVE:
		case ROOM_NORMAL_CENTRE:
		case ROOM_NORMAL_WALLS:
		case ROOM_NORMAL: if (build_type123(room, type)) { if (type == ROOM_NORMAL) room_info[room].flags |= (ROOM_SIMPLE); return(TRUE); } break;

		default: if (build_type0(room, type)) return(TRUE); break;
	}

	/* Failure */
	return (FALSE);
}



/*
 * Place lakes and rivers given a feature
 */
static bool build_feature(int feat, bool do_big_lake, bool merge_lakes)
{
	/* No coordinates yet */
	int x0 = 0, y0 = 0;

	/* Build a lake? */
	if (((f_info[feat].flags2 & (FF2_LAKE)) != 0) || ((f_info[feat].flags2 & (FF2_RIVER)) == 0))
	{
		/* Try to place the lake. Get its center */
		if (!build_lake(feat, do_big_lake, merge_lakes, &y0, &x0)) return (FALSE);
	}

	/* Build a river */
	if ((f_info[feat].flags2 & (FF2_RIVER)) != 0)
	{
		/* Pick starting coordinates, if needed */
		if ((y0 + x0) == 0)
		{
			y0 = randint(DUNGEON_HGT - 2);
			x0 = randint(DUNGEON_WID - 2);
		}

		/* Generate the river */
		build_river(feat, y0, x0);

		/* Usually build an outflow as well */
		if ((!merge_lakes) && (rand_int(100) < 90))
		{
			/* Pick starting coordinates, if needed */
			if ((y0 + x0) == 0)
			{
				y0 = randint(DUNGEON_HGT - 2);
				x0 = randint(DUNGEON_WID - 2);
			}

			/* Generate the river */
			build_river(feat, y0, x0);
		}
	}

	/* Ensure interesting stuff in lake */
	dun->lake[dun->lake_n].y = y0;
	dun->lake[dun->lake_n++].x = x0;

	/* Success */
	return (TRUE);
}


/*
 * Build lakes and rivers for the dungeon
 */
static void build_nature(void)
{
	bool big = FALSE;
	bool done_big = FALSE;
	bool force_big = FALSE;

	int feat;
	int max_lakes = level_flag & (LF1_ROOMS) ? DUN_MAX_LAKES_ROOM : DUN_MAX_LAKES_NONE;

	/* Flavor */
	bool merge_lakes = ((p_ptr->depth >= 30) && !rand_int(7));

	/* Get dungeon zone */
	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Allocate some lakes and rivers */
	for (dun->lake_n = 0; dun->lake_n < max_lakes; )
	{
		/* Have placed features */
		if (!(zone->big) && !(zone->small) && (rand_int(100) >= ((level_flag & (LF1_SURFACE)) ? DUN_NATURE_SURFACE : DUN_NATURE_DEEP)))
		{
			break;
		}

		/* Place zone big terrain last */
		if ((force_big) || ((dun->lake_n >= max_lakes - (max_lakes + 3) / 4) && (zone->big)))
		{
			feat = zone->big;
			big = TRUE;
		}
		else if (zone->small)
		{
			feat = zone->small;
		}
		else
		{
			/* Pick a feature */
			feat = pick_proper_feature(cave_feat_lake);
		}

		/* Got a valid feature? */
		if (feat)
		{
			/* Try a big lake */
			if ((!done_big) && (!big))
			{
				big = (randint(150) < p_ptr->depth);

				/* Got one */
				done_big |= big;
			}

			/* Shallow and roomless dungeons have separate lakes */
			if ((force_big) || ((level_flag & (LF1_ROOMS)) == 0) || (rand_int(75) > p_ptr->depth))
			{
				/* Report creation of lakes/rivers */
				if (cheat_room)
				{
					if (f_info[feat].edge)
					{
						message_add(format("Building %s%s surrounded by %s.",
								big ? "big ": "", f_name + f_info[feat].name, f_name + f_info[f_info[feat].edge].name), MSG_GENERIC);
					}
					else
					{
						message_add(format("Building %s%s.", big ? "big ": "", f_name + f_info[feat].name), MSG_GENERIC);
					}
				}

				/* Build one lake/river */
				if ((!build_feature(feat, big, merge_lakes)) || (force_big)) break;
			}
			else
			{
				/* Report creation of flooded dungeons */
				if (cheat_room)
				{
					message_add(format("Flooding dungeon with %s.", f_name + f_info[feat].name), MSG_GENERIC);
				}

				/* Flood the dungeon */
				dun->flood_feat = feat;

				/* Must place one big feature */
				if (zone->big) force_big = TRUE;
				else break;
			}
		}

		/* Clear big-ness for next iteration */
		big = FALSE;
	}
}



/*
 * Value "1" means the grid will be changed, value "0" means it won't.
 *
 * We have 47 entries because 47 is not divisible by any reasonable
 * figure for streamer width.
 */
static bool streamer_change_grid[47] =
{
	0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0
};


/*
 * Places "streamers" of rock through dungeon.
 *
 * Note that there are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
static void build_streamer(int feat)
{
	int table_start;
	int i;
	int y, x, dy, dx;
	int start_dir, dir;
	int out1, out2;
	bool change;

	/* Get chance */
	int chance = f_info[feat_state(feat, FS_STREAMER)].rarity;

	/* Initialize time until next turn, and time until next treasure */
	int time_to_treas = randint(chance * 2);
	int time_to_turn = randint(DUN_STR_CHG * 2);

	/* Set standard width.  Vary width sometimes. */
	int width = 2 * DUN_STR_WID + 1;
	if (!rand_int(6)) width += randint(3);
	else if (!rand_int(6)) width -= randint(3);
	if (width < 1) width = 1;

	/* Set expansion outward from centerline. */
	out1 = width / 2;
	out2 = (width + 1) / 2;


	/* Hack -- Choose starting point */
	y = rand_spread(DUNGEON_HGT / 2, DUNGEON_HGT / 4);
	x = rand_spread(DUNGEON_WID / 2, DUNGEON_WID / 4);

	/* Choose a random compass direction */
	dir = start_dir = ddd[rand_int(8)];

	/* Get an initial start position on the grid alteration table. */
	table_start = rand_int(47);

	/* Place streamer into dungeon */
	while (TRUE)
	{
		/* Advance streamer width steps on the table. */
		table_start += width;

		/*
		 * Change grids outwards along sides.  If moving diagonally,
		 * change a cross-shaped area.
		 */
		if (ddy[dir])
		{
			for (dx = x - out1; dx <= x + out2; dx++)
			{
				/* Stay within dungeon. */
				if (!in_bounds(y, dx)) continue;

				/* Only convert "granite" walls */
				if (cave_feat[y][dx] < FEAT_WALL_EXTRA) continue;
				if (cave_feat[y][dx] > FEAT_WALL_SOLID) continue;

				/* Skip vaults and whatnot */
				if (room_has_flag(y, x, ROOM_ICKY)) continue;

				i = table_start + dx - x;

				if ((i < 47) && (i >= 0)) change = streamer_change_grid[i];
				else change = streamer_change_grid[i % 47];

				/* No change to be made. */
				if (!change) continue;

				/* Clear previous contents, add proper vein type */
				cave_set_feat(y, dx, feat);

				/* Count down time to next treasure. */
				time_to_treas--;

				/* Hack -- Add some (known) treasure */
				if (time_to_treas == 0)
				{
					time_to_treas = randint(chance * 2);
					cave_feat[y][dx] = feat_state(feat, FS_STREAMER);
				}
			}
		}

		if (ddx[dir])
		{
			for (dy = y - out1; dy <= y + out2; dy++)
			{
				/* Stay within dungeon. */
				if (!in_bounds(dy, x)) continue;

				/* Only convert "granite" walls */
				if (cave_feat[dy][x] < FEAT_WALL_EXTRA) continue;
				if (cave_feat[dy][x] > FEAT_WALL_SOLID) continue;

				i = table_start + dy - y;

				if ((i < 47) && (i >= 0)) change = streamer_change_grid[i];
				else change = streamer_change_grid[i % 47];

				/* No change to be made. */
				if (!change) continue;

				/* Clear previous contents, add proper vein type */
				cave_set_feat(dy, x, feat);

				/* Count down time to next treasure. */
				time_to_treas--;

				/* Hack -- Add some (known) treasure */
				if (time_to_treas == 0)
				{
					time_to_treas = randint(chance * 2);
					cave_feat[dy][x] = feat_state(feat, FS_STREAMER);
				}
			}
		}

		/* Count down to next direction change. */
		time_to_turn--;

		/* Sometimes, vary direction slightly. */
		if (time_to_turn == 0)
		{
			/* Get time until next turn. */
			time_to_turn = randint(DUN_STR_CHG * 2);

			/* Randomizer. */
			i = rand_int(3);

			/* New direction is always close to start direction. */
			if (start_dir == 2)
			{
				if (i == 0) dir = 2;
				if (i == 1) dir = 1;
				else        dir = 3;
			}
			else if (start_dir == 8)
			{
				if (i == 0) dir = 8;
				if (i == 1) dir = 9;
				else        dir = 7;
			}
			else if (start_dir == 6)
			{
				if (i == 0) dir = 6;
				if (i == 1) dir = 3;
				else        dir = 9;
			}
			else if (start_dir == 4)
			{
				if (i == 0) dir = 4;
				if (i == 1) dir = 7;
				else        dir = 1;
			}
			else if (start_dir == 3)
			{
				if (i == 0) dir = 3;
				if (i == 1) dir = 2;
				else        dir = 6;
			}
			else if (start_dir == 1)
			{
				if (i == 0) dir = 1;
				if (i == 1) dir = 4;
				else        dir = 2;
			}
			else if (start_dir == 9)
			{
				if (i == 0) dir = 9;
				if (i == 1) dir = 6;
				else        dir = 8;
			}
			else if (start_dir == 7)
			{
				if (i == 0) dir = 7;
				if (i == 1) dir = 8;
				else        dir = 4;
			}
		}

		/* Advance the streamer */
		y += ddy[dir];
		x += ddx[dir];

		/* Stop at dungeon edge */
		if (!in_bounds(y, x)) break;
	}
}


#define SPELL_DESTRUCTION	32

/*
 * Build a destroyed level
 */
static void destroy_level(void)
{
	int y, x, n;
	bool dummy = FALSE;

	/* Note destroyed levels */
	if (cheat_room) message_add("Destroyed Level", MSG_GENERIC);

	/* Drop a few epi-centers (usually about two) */
	for (n = 0; n < randint(5); n++)
	{
		/* Pick an epi-center */
		x = rand_range(5, DUNGEON_WID-1 - 5);
		y = rand_range(5, DUNGEON_HGT-1 - 5);

		/* Apply destruction effect */
		process_spell_target(0, 0, y, x, y, x, SPELL_DESTRUCTION, 0, 1, FALSE, TRUE, FALSE, &dummy, NULL);
	}
}




/*
 *  Initialise a dungeon ecology based on a seed race.
 */
static void init_ecology(int r_idx)
{
	int i, j, k;
	int l = 0;
	int count = 0;

	/* Initialise the dungeon ecology */
	(void)WIPE(&cave_ecology, ecology_type);
	assert (cave_ecology.ready == FALSE);
	assert (cave_ecology.valid_hook == FALSE);

	/* Count of different non-unique monsters in ecology */
	k = MIN_ECOLOGY_RACES + rand_int(MIN_ECOLOGY_RACES);

	/* Start with two ecologies - one for the dungeon, and one for rooms in the dungeon */
	cave_ecology.num_ecologies = 2;

	/* Dungeon uses first ecology */
	room_info[0].ecology = (1L);

	/* Initialise ecology based on seed race */
	if (r_idx)
	{
		/* Get seed monster for ecology */
		get_monster_ecology(r_idx, 0);
	}

	/* Get a seed monster for the ecology */
	else
	{
		/* Set monster hook */
		get_mon_num_hook = dun_level_mon;

		/* Prepare allocation table */
		get_mon_num_prep();

		/* Get seed monster for ecology */
		get_monster_ecology(0, 0);

		/* Clear monster hook */
		get_mon_num_hook = NULL;

		/* Prepare allocation table */
		get_mon_num_prep();
	}

	/* Get additional monsters for the ecology */
	while (TRUE)
	{
		/* Check ecology */
		for (i = l; i < cave_ecology.num_races; i++)
		{
			bool differs = TRUE;

			/* Don't count uniques */
			if ((r_info[cave_ecology.race[i]].flags1 & (RF1_UNIQUE)) != 0) continue;

			/* Check if appears already */
			for (j = 0; j < i; j++)
			{
				if (cave_ecology.race[i] == cave_ecology.race[j]) differs = FALSE;
			}

			/* Race not already counted */
			if (differs) k--;
		}

		/* Note last race checked */
		l = cave_ecology.num_races;

		/* Not enough different monsters */
		if ((count++ < 100) && (k >= 0) && (cave_ecology.num_races < MAX_ECOLOGY_RACES))
		{
			/* XXX Sometimes we have to cancel out due to not being able to
			 * find a dun_level_mon match for the level we are on.
			 */
			if (count < 10)
			{
				/* Set monster hook */
				get_mon_num_hook = dun_level_mon;

				/* Prepare allocation table */
				get_mon_num_prep();
			}

			/* Increase ecologies by one. */
			if ((cave_ecology.num_ecologies < MAX_ECOLOGIES)
					/* Sometimes merge them instead */
					&& (rand_int(100) < 80))
				cave_ecology.num_ecologies++;

			/* Get seed monster for ecology */
			get_monster_ecology(0, 0);

			if (get_mon_num_hook)
			{
				/* Clear monster hook */
				get_mon_num_hook = NULL;

				/* Prepare allocation table */
				get_mon_num_prep();
			}
		}
		else
		{
			break;
		}
	}

	/* Deepest monster in each ecology does not wander outside of core room */
	for (i = 0; i < cave_ecology.num_ecologies; i++)
	{
		for (j = 0; j < cave_ecology.num_races; j++)
		{
			/* Deepest monster */
			if (cave_ecology.deepest_race[i] == cave_ecology.race[j])
			{
				/* Does not wander through dungeon */
				cave_ecology.race_ecologies[j] &= ~(1L);

				/* Does not appear outside core */
				cave_ecology.race_ecologies[j] &= ~(1L << i);

				/* Does appear in core */
				cave_ecology.race_ecologies[j] |= (1L << (i + MAX_ECOLOGIES));
			}
		}
	}

	/* Start the ecology */
	cave_ecology.ready = TRUE;
	cave_ecology.valid_hook = TRUE;
}


/*
 *  Place tower in the dungeon.
 */
static void place_tower()
{
	vault_type *v_ptr;
	int y, x;

	int by, bx;

	dungeon_zone *zone=&t_info[0].zone[0];

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Get the location of the tower */
	y = (DUNGEON_HGT) / 2;
	x = (DUNGEON_WID) / 2;

	/* Hack -- are we directly above another tower? */
	if ((p_ptr->depth == zone->level) && (p_ptr->depth > min_depth(p_ptr->dungeon)))
	{
		dungeon_zone *roof;

		get_zone(&roof,p_ptr->dungeon,p_ptr->depth-1);

		v_ptr = &v_info[roof->tower];

		build_roof(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
	}

	/* Get the vault */
	v_ptr = &v_info[zone->tower];

	/* Hack -- Build the tower */
	build_tower(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);

	/* Paranoia */
	if (dun->cent_n < CENT_MAX)
	{
		/* Set corridor here */
		dun->cent[dun->cent_n].y = y;
		dun->cent[dun->cent_n].x = x;

		/* Reserve some blocks */
		for (by = (y - v_ptr->hgt / 2) / BLOCK_HGT; by <= (y + v_ptr->hgt / 2 + 1) / BLOCK_HGT; by++)
		{
			for (bx = (x - v_ptr->wid / 2) / BLOCK_WID; bx <= (x + v_ptr->wid / 2 + 1) / BLOCK_WID; bx++)
			{
				dun->room_map[by][bx] = TRUE;

				dun_room[by][bx] = dun->cent_n;
			}
		}

		/* Paranoia */
		if (dun->cent_n < DUN_ROOMS)
		{
			/* Initialise room description */
			room_info[dun->cent_n].type = ROOM_TOWER;

			/* Non-surface levels will be set to ROOM_ICKY at end of generation */
			room_info[dun->cent_n].flags = (level_flag & (LF1_SURFACE)) != 0 ? ROOM_ICKY : 0;

			room_info[dun->cent_n].theme[THEME_TUNNEL] = 0;
			room_info[dun->cent_n].theme[THEME_SOLID] = 0;
		}
		
		/* Increase the number of rooms */
		dun->cent_n++;
	}

	/* Hack -- descending player always in tower */
	if ((level_flag & LF1_SURFACE) && ((f_info[p_ptr->create_stair].flags1 & (FF1_LESS)) != 0))
	{
		/* Clear previous contents, add stairs up */
		place_random_stairs(y, x, FEAT_LESS);

		/* Have created stairs */
		p_ptr->create_stair = 0;

		player_place(y, x, TRUE);
	}

	/* Hack -- always have upstairs in surface of tower */
	if ((level_flag & LF1_SURFACE)
		&& p_ptr->depth < max_depth(p_ptr->dungeon))
	{
		feat_near(FEAT_LESS, y, x);
	}
}



/*
 *  Place rooms in the dungeon.
 */
static bool place_rooms()
{
	int i, j, k;

	int rooms_built = 0;
	int count = 0;

	/* Check if this is the last room type we can place for this theme. If so, continue to place it. */
	bool last = FALSE;

	int try_rooms = MIN_DUN_ROOMS + rand_int(MAX_DUN_ROOMS - MIN_DUN_ROOMS) - (p_ptr->depth / 12);
	
	/*
	 * Build each type of room in turn until we cannot build any more.
	 */
	for (i = ((level_flag & (LF1_ROOMS)) != 0) ? 0 : ROOM_MAX - 1;
			(count++ < 100) && (i < ROOM_MAX) && (dun->cent_n < try_rooms);
			last ? /* Do nothing */ i : i++)
	{
		/* What type of room are we building now? */
		int room_type = room_build_order[i];

		/* A series of checks for rooms only */
		if ((level_flag & (LF1_ROOMS)) != 0)
		{
			/* Skip if level themed and we don't match the theme */
			if (((level_flag & (LF1_THEME)) != 0) && ((room_data[room_type].theme & (level_flag)) == 0)) continue;

			/* Level is themed */
			if ((level_flag & (LF1_THEME)) != 0)
			{
				last = TRUE;

				/* Check remaining room types */
				for (k = i + 1; k < ROOM_MAX; k++)
				{
					/* Valid room type left */
					if ((room_data[room_build_order[k]].theme & (level_flag)) != 0) last = FALSE;
				}
				
				/* Skip at this depth while ensuring room exists */
				if ((!last) && (room_data[room_type].min_level > p_ptr->depth)) continue;
			}
		}
		/* No rooms, just tunnels */
		else
		{
			/* Message */
			if (cheat_room) message_add("No rooms on level.", MSG_GENERIC);
			
			/* Place tunnel endpoints only */
			room_type = 0;
		}
		
		/* Build the room. */
		while (((last) || (rand_int(100) < room_data[room_type].chance[p_ptr->depth < 60 ? p_ptr->depth / 6 : 10]))
			&& (rooms_built < room_data[room_type].max_number + (p_ptr->depth - room_data[room_type].min_level) / 12 - 1)
			&& (dun->cent_n < DUN_ROOMS) &&	(room_build(dun->cent_n, room_type)))
		{
			/* Built room */
			if (cheat_room) message_add(format("Built room type %d.", room_type), MSG_GENERIC);

			/* Increase the room built count. */
			rooms_built += room_data[room_type].count_as;

			/* Validate the room centre */
			if (room_idx_ignore_valid(dun->cent[dun->cent_n-1].y, dun->cent[dun->cent_n-1].x) != dun->cent_n-1)
			{
				/* Oops */
				if (cheat_room) message_add(format("Centre of room %d of type %d is not marked as a room.", dun->cent_n-1, i), MSG_GENERIC);
				
				/* Paranoia - mark it */
				dun->room_map[dun->cent[dun->cent_n-1].y / BLOCK_HGT][dun->cent[dun->cent_n-1].x / BLOCK_WID] = TRUE;
				dun_room[dun->cent[dun->cent_n-1].y / BLOCK_HGT][dun->cent[dun->cent_n-1].x / BLOCK_WID] = dun->cent_n-1;
			}
			
			/* No theme chosen */
			if ((level_flag & (LF1_THEME)) == 0)
			{
				/* Hack -- no choice */
				int choice = -1;

				/* Reset count */
				k = 0;

				/* Keep dungeons 'simple' when deeper rooms first encountered */
				if ((room_data[room_type].min_level >= 5) && (room_data[room_type].min_level >= p_ptr->depth - 6))
				{
					/* Make corridors 'dungeon-like' */
					level_flag &= ~(LF1_THEME);
					level_flag |= (LF1_DUNGEON);
					
					/* Empty rooms for remainder of level */
					i = ROOM_NORMAL;
					
					/* Message */
					if (cheat_room) message_add("One unique room. Rest of level is normal.", MSG_GENERIC);
				}
				/* Pick a theme */
				else for (j = 0; j < 32; j++)
				{
					/* Place nature and destroyed elsewhere */
					if (((1L << j) == (LF1_WILD)) || ((1L << j) == (LF1_DESTROYED))) continue;
					
					/* Pick a theme */
					if ( ((room_data[room_type].theme & (1L << j)) != 0) && (rand_int(++k) == 0)) choice = j;	
				}

				/* Set a theme if picked */
				if (choice >= 0)
				{
					level_flag |= (1L << choice);
					
					/* Message */
					if (cheat_room) message_add(format("Theme chosen for level is %d.", choice), MSG_GENERIC);					
				}
				
				/* Need to repeat this test here. This occurs because we can place one room, fail to place a
				 * second, and then exit out of the loop because this is the last type of room we can place. */
				last = TRUE;

				/* Check remaining room types */
				for (k = i + 1; k < ROOM_MAX; k++)
				{
					/* Valid room type left */
					if (((level_flag & (LF1_THEME)) != 0) && ((room_data[room_build_order[k]].theme & (level_flag)) != 0)) last = FALSE;
				}
			}
		}		
	}

	/* No tunnels if only zero or one room. Note room '0' is the dungeon */
	if (dun->cent_n <= 1) level_flag &= ~(LF1_TUNNELS);

	/* Attempt successful */
	return (TRUE);
}



/*
 * Create the irregular borders of a wilderness levels.
 * Based on code created by Nick McConnel (FAangband)
 */
static void build_wilderness_borders(s16b feat)
{
	int hgt = DUNGEON_HGT;
	int wid = DUNGEON_WID;
	int y, x;
	int i;

	int start_offset = 4;
	int max_offset = 7;

	/* Top border */
	i = start_offset;

	for (x = 0; x < wid; x++)
	{
		/* Modify the offset by -1, +1 or 0 */
		i += (1 - rand_int(3));

		/* Check bounds */
		if (i > max_offset) i = max_offset;
		else if (i < 0) i = 0;

		/* Place border */
		for (y = 0; y < i; y++)
		{
			cave_set_feat(y, x, feat);
		}
	}

	/* Bottom border */
	i = start_offset;

	for (x = 0; x < wid; x++)
	{
		/* Modify the offset by -1, +1 or 0 */
		i += (1 - rand_int(3));

		/* Check bounds */
		if (i > max_offset) i = max_offset;
		else if (i < 0) i = 0;

		/* Place border */
		for (y = 0; y < i; y++)
		{
			cave_set_feat(hgt - 1 - y, x, feat);
		}
	}

	/* Left border */
	i = start_offset;

	for (y = 0; y < hgt; y++)
	{
		/* Modify the offset by -1, +1 or 0 */
		i += (1 - rand_int(3));

		/* Check bounds */
		if (i > max_offset) i = max_offset;
		else if (i < 0) i = 0;

		/* Place border */
		for (x = 0; x < i; x++)
		{
			cave_set_feat(y, x, feat);
		}
	}

	/* Right border */
	i = start_offset;

	for (y = 0; y < hgt; y++)
	{
		/* Modify the offset by -1, +1 or 0 */
		i += (1 - rand_int(3));

		/* Check bounds */
		if (i > max_offset) i = max_offset;
		else if (i < 0) i = 0;

		/* Place border */
		for (x = 0; x < i; x++)
		{
			cave_set_feat(y, wid - 1 - x, feat);
		}
	}

	/* Prevent rooms near the borders */
	for (y = 0; y < dun->row_rooms; y++)
	{
		dun->room_map[y][0] = TRUE;
		dun->room_map[y][dun->col_rooms - 1] = TRUE;

	}

	/* Prevent rooms near the borders */
	for (x = 0; x < dun->col_rooms; x++)
	{
		dun->room_map[0][x] = TRUE;
		dun->room_map[dun->row_rooms - 1][x] = TRUE;
	}
}


/*
 * Place boundary walls around the edge of the dungeon
 */
static void place_boundary_walls()
{
	int y, x;

	/* Special boundary walls -- Top */
	for (x = 0; x < DUNGEON_WID; x++)
	{
		y = 0;

		cave_feat[y][x] = FEAT_PERM_SOLID;

		cave_info[y][x] |= (CAVE_XLOS);
		cave_info[y][x] |= (CAVE_XLOF);
	}

	/* Special boundary walls -- Bottom */
	for (x = 0; x < DUNGEON_WID; x++)
	{
		y = DUNGEON_HGT-1;

		cave_feat[y][x] = FEAT_PERM_SOLID;

		cave_info[y][x] |= (CAVE_XLOS);
		cave_info[y][x] |= (CAVE_XLOF);
	}

	/* Special boundary walls -- Left */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		x = 0;

		cave_feat[y][x] = FEAT_PERM_SOLID;

		cave_info[y][x] |= (CAVE_XLOS);
		cave_info[y][x] |= (CAVE_XLOF);
	}

	/* Special boundary walls -- Right */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		x = DUNGEON_WID-1;

		cave_feat[y][x] = FEAT_PERM_SOLID;

		cave_info[y][x] |= (CAVE_XLOS);
		cave_info[y][x] |= (CAVE_XLOF);
	}
}


/*
 *  Place tunnels in the dungeon.
 */
static bool place_tunnels()
{
	int i, j, k, y, x;

	int counter = 0;
	int retries = 0;
	
	bool allow_corridors = FALSE;
	
	/* Hack -- Clear play safe grids */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			play_info[y][x] = 0;
		}
	}

	/* Mega hack -- try to connect lakes */
	if ((level_flag & (LF1_SURFACE)) == 0)
	{
		for (i = 0; i < dun->lake_n; i++)
		{
			/* Save the room location, unless already in a room */
			if ((dun->cent_n < CENT_MAX)
					 && ((cave_info[dun->lake[i].y][dun->lake[i].x] & (CAVE_ROOM)) == 0)
					 && !(room_idx_ignore_valid(dun->lake[i].y, dun->lake[i].x)))
			{
				/* Add the lake centre */
				dun->cent[dun->cent_n].y = dun->lake[i].y;
				dun->cent[dun->cent_n].x = dun->lake[i].x;
				
				/* Associate this with the lake */
				dun_room[dun->cent[dun->cent_n].y / BLOCK_HGT][dun->cent[dun->cent_n].x / BLOCK_WID] = dun->cent_n;
				
				dun->cent_n++;
			}
		}
	}

	/* Start with no tunnel doors */
	dun->door_n = 0;

	/* Partition rooms */
	/* Note that rooms start from 1, but we initialize room 0 (the dungeon) in order to distinguish errors */
	for (i = 0; i < dun->cent_n; i++)
	{
		dun->part[i] = i;
	}

	/*
	 * New tunnel generation routine.
	 *
	 * We partition the rooms into distinct partition numbers. We then find the room in
	 * each partition with the closest neighbour in an adjacent partition and attempt
	 * to connect the two rooms.
	 *
	 * When two rooms are connected by build_tunnel, the partitions are merged.
	 *
	 * We repeat, until there is only one partition.
	 */

	while (TRUE)
	{
		bool finished = TRUE;

		/* Abort */
		if (counter++ > DUN_ROOMS * DUN_ROOMS)
		{
			if (cheat_room)
			{
				message_add(format("Unable to connect rooms in %d attempted tunnels.", DUN_ROOMS * DUN_ROOMS), MSG_GENERIC);
				
				for (i = 1; i < dun->cent_n; i++)
				{
					message_add(format("Room %d was in partition %d.", i, dun->part[i]), MSG_GENERIC);
				}
			}
			
			/* Hack -- Clear play safe grids */
			for (y = 0; y < DUNGEON_HGT; y++)
			{
				for (x = 0; x < DUNGEON_WID; x++)
				{
					play_info[y][x] = 0;
				}
			}

			/* Megahack -- we have connectivity on surface anyway */
			if (level_flag & (LF1_SURFACE)) return (TRUE);

			return(FALSE);
		}

		retries++;

		/* Check each partition */
		for (i = 1; i < dun->cent_n; i++)
		{
			int dist = 30000;
			int c1 = -1;
			int c2 = -1;
			int r1 = -1;
			int r2 = -1;
			int n1 = 0;
			int n2 = 0;

			/* After the first loop, choose the smallest partition one third of the time instead */
			if ((retries > DUN_TRIES) && (retries % 3 == 0))
			{
				/* Count of partition size */
				int m = dun->cent_n, n = 0, o = 0;
				
				/* We lose track of i, so abort another way */
				if (rand_int(dun->cent_n) == 0) break;

				/* Count of all partitions */
				for (j = 1; j < dun->cent_n; j++)
				{
					int l = 0;
					
					for (k = 1; k < dun->cent_n; k++)
					{
						/* Count this partition */
						if (dun->part[k] == j) l++;
					}
					
					/* Ensure we choose a partition with at least one room */
					if (l)
					{
						/* Count how many different partitions */
						n++;
						
						/* Accept smaller partition */
						if (l < m) { i = j; m = l; o = 1; }
						
						/* Accept random partition if the same size */
						else if ((l == m) && !(rand_int(++o))) i = j;
					}
				}
								
				/* We have a choice */
				if (m < dun->cent_n)
				{
					if (cheat_room) message_add(format("Choosing smallest partition %d with %d members (%d of this size).", i, m, o), MSG_GENERIC);
				}
								
				/* Hack - we have two partitions left, and one only contains one room. Allow corridors from this partition to merge
				 * with other corridors to unite the dungeon.
				 */
				if ((m == 1) && (n == 2)) allow_corridors = TRUE;
			}
			/* Stop allowing corridors for this iteration */
			else
			{
				allow_corridors = retries > 4 * DUN_TRIES;
			}
			
			/* Pick a random member of this partition */
			for (j = 1; j < dun->cent_n; j++)
			{
				if (dun->part[j] != i) continue;

				/* Pick a random choice */
				if (!rand_int(++n1)) r1 = j;
			}

			/* Check members of this partition */
			for (j = 1; j < dun->cent_n; j++)
			{
				if (dun->part[j] != i) continue;

				/* Check all rooms */
				for (k = 1; k < dun->cent_n; k++)
				{
					int dist1;

					/* Skip itself */
					if (k == j) continue;

					/* Skip members of the same partition */
					if (dun->part[k] == i) continue;

					/* Check the flooding state */
					if ((retries > 7 * DUN_TRIES) || ((retries > DUN_TRIES) && ((retries % 5) == 0)))
					{
						/* Don't skip connecting flooded to non-flooded some of the time */
					}
					/* Skip if state of flooding doesn't match */					
					else if ((retries > 2 * DUN_TRIES) &&
						((room_info[k].flags & (ROOM_FLOODED)) != (room_info[r1].flags & (ROOM_FLOODED))))
					{
						continue;
					}
					else if ((room_info[k].flags & (ROOM_FLOODED)) != (room_info[j].flags & (ROOM_FLOODED)))
					{
						continue;
					}

					/* Trying a random choice */
					if (retries > 2 * DUN_TRIES)
					{
						dist1 = distance(dun->cent[r1].y, dun->cent[r1].x, dun->cent[k].y, dun->cent[k].x);
					}
					else
					{
						dist1 = distance(dun->cent[j].y, dun->cent[j].x, dun->cent[k].y, dun->cent[k].x);
					}

					/* Pick this room sometimes */
					if (dist1 < dist)
					{
						dist = dist1;
						if (retries > 2 * DUN_TRIES) c1 = r1; else c1 = j;
						c2 = k;
					}

					/* Pick a random choice */
					if (!rand_int(++n2)) r2 = k;
				}
			}
			
			/* Try 'forcing' way into irregular rooms */
			if (((retries > 5 * DUN_TRIES) && (room_info[r1].type > ROOM_HUGE_CENTRE)) || (retries > 6 * DUN_TRIES))
			{
				/* Ignore outer walls */
				if ((room_info[r1].flags & (ROOM_EDGED)) == 0)
				{
					room_info[r1].flags |= (ROOM_EDGED);
				}
			}

			/* Use the choice after the first iteration until we get desperate*/
			if ((c1 >= 0) && (c2 >= 0) && (c1 != c2) && (((retries >= DUN_TRIES) && (retries < 3 * DUN_TRIES) && (rand_int(4 * DUN_TRIES) > retries)) ||
					
					/* The first DUN_TRIES through, we connect tunnels randomly the start room is near the edge of the dungeon, and directly if it is not. */
					((retries < DUN_TRIES) && (dun->cent[c1].y > DUNGEON_HGT / 4) && (dun->cent[c1].y < DUNGEON_HGT * 3 / 4)
							&& (dun->cent[c1].x > DUNGEON_WID / 3) && (dun->cent[c1].x < DUNGEON_WID * 2 / 3))
					))
			{
				if (build_tunnel(dun->cent[c1].y, dun->cent[c1].x, dun->cent[c2].y, dun->cent[c2].x, retries > DUN_TRIES, allow_corridors)) retries = 0;
			}
			/* Try a random choice */
			else if ((r1 >= 0) && (r2 >= 0) && (r1 != r2))
			{
				if (build_tunnel(dun->cent[r1].y, dun->cent[r1].x, dun->cent[r2].y, dun->cent[r2].x, TRUE, allow_corridors)) retries = 0;
			}
		}

		/* Retrying */
		if ((cheat_room) && (!(retries % DUN_TRIES))) message_add(format("Retrying %d.", retries), MSG_GENERIC);

		/* Check if we have finished. All partition numbers will be the same. Note we start checking from room 2 */
		for (i = 2; i < dun->cent_n; i++)
		{
			if (dun->part[i] != dun->part[i-1]) finished = FALSE;
		}

		/* Finished? */
		if (finished) break;
	}

	/* Place intersection doors */
	for (i = 0; i < dun->door_n; i++)
	{
		int dk = rand_int(100) < 50 ? 1 : -1;
		int count = 0;

		/* Extract junction location */
		y = dun->door[i].y;
		x = dun->door[i].x;

		/* Try a door in one direction */
		/* If the first created door is secret, stop */
		for (j = 0, k = rand_int(4); j < 4; j++)
		{
			if (try_door(y + ddy_ddd[k], x + ddx_ddd[k])) count++;

			if ((!count) && (f_info[cave_feat[y + ddy_ddd[k]][x + ddx_ddd[k]]].flags1 & (FF1_SECRET))) break;

			k = (4 + k + dk) % 4;
		}
	}

	/* Hack -- Clear play safe grids */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			play_info[y][x] = 0;
		}
	}

	/* Attempts successful */
	return (TRUE);
}


/*
 * Place decorations on walls (streamers and room entrance markings)
 */
static void place_decorations()
{
	int i, y, x;

	/* Place room decorations */
	for (i = 0; i < dun->next_n; i++)
	{
		/* Extract doorway location */
		y = dun->next[i].y;
		x = dun->next[i].x;

		/* Make sure we are replacing solid wall */
		if (!(f_info[cave_feat[y][x]].flags1 & (FF1_SOLID))) continue;

		/* Place feature if required */
		if (dun->next_feat[i]) cave_set_feat(y, x, dun->next_feat[i]);
	}

	/* Check rooms for streamer types */
	for (i = 0; i < dun->cent_n; i++)
	{
		int feat = room_info[i].theme[THEME_SOLID];
		int s = dun->stream_n;

		/* Record streamers for later */
		if ((feat) && (f_info[feat].flags1 & (FF1_STREAMER)) && (dun->stream_n < DUN_MAX_STREAMER))
		{
			int n;

			for (n = s; n < dun->stream_n; n++)
			{
				/* Already added */
				if (dun->streamer[n] == feat) break;
			}

			if (n == dun->stream_n)
			{
				/* Add dungeon streamer */
				dun->streamer[dun->stream_n++] = feat;
			}
		}
	}

	/* Allocate some streamers */
	for (i = 0; i < DUN_MAX_STREAMER; i++)
	{
		/* Pick a feature */
		int feat;

		/* Hack -- try to match dungeon themes */
		if (dun->stream_n)
		{
			feat = dun->streamer[i % dun->stream_n];
		}
		else
		{
			feat = pick_proper_feature(cave_feat_streamer);
		}

		/* Got a valid feature? */
		if (feat)
		{
			/* Generating */
			if (cheat_room) message_add(format("Building %s streamer.", f_name + f_info[feat].name), MSG_GENERIC);

			/* Build one streamer. */
			build_streamer(feat);
		}
	}
}



/*
 * Returns a new spot for the player
 */
static bool new_player_spot(void)
{
	int i = 0;
	int y = 0;
	int x = 0;
	bool tunnel = FALSE;

	while (TRUE)
	{
		i++;

		/* Scan stored locations first. */
		if ((i < dun->stair_n) && (p_ptr->create_stair > 0))
		{
			/* Get location */
			y = dun->stair[i].y;
			x = dun->stair[i].x;

			/* Require a "naked" floor grid */
			if (!cave_naked_bold(y, x)) continue;

			/* Success */
			break;
		}

		/* Then, search at random */
		else
		{
			/* Pick a random grid */
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);

			/* Mega hack -- try to place extra tunnel, except above surface in towers */
			if ((((level_flag & (LF1_TOWER)) == 0) || ((level_flag & (LF1_SURFACE)) != 0)) &&
				 (!(tunnel) && (i == 400)))
			{
				if (build_type0(0, 0))
				{
					place_tunnels();

					i = 0;

					tunnel = TRUE;
				}
			}

			/* We have failed */
			if (i > 1000) return (FALSE);

			/* Refuse to start on anti-teleport (vault) grids */
			if ((cave_info[y][x] & (CAVE_ROOM)) && (room_has_flag(y, x, ROOM_ICKY))) continue;

			/* Must be a safe start grid clear of monsters and objects  XXX */
			if ((i < 650) && (!cave_start_bold(y, x))) continue;

			/* Must be a floor grid clear of monsters and objects  XXX */
			if ((i < 700) && (!cave_naked_bold(y, x))) continue;

			/* Try nearly naked grids if desperate */
			if ((i >= 700) && (!cave_nearly_naked_bold(y, x))) continue;

			/* Try not to start in rooms */
			if ((i < 450) && (cave_info[y][x] & (CAVE_ROOM))) continue;

			/* Player prefers to be near walls. */
			if      ((i < 300) && (next_to_walls(y, x) < 2)) continue;
			else if ((i < 600) && (next_to_walls(y, x) < 1)) continue;

			/* Success */
			break;
		}
	}

	/* Place the stairs (if any) */
	/* Don't allow stairs if connected stair option is off */
	if (p_ptr->create_stair &&
		 ((f_info[p_ptr->create_stair].flags1 & (FF1_STAIRS)) == 0
		  || !adult_no_stairs))
	{
		/* Create the feature */
		cave_set_feat(y, x, p_ptr->create_stair);

		/* Have created stairs */
		p_ptr->create_stair = 0;
	}

	/* Place the player */
	player_place(y, x, TRUE);

	return (TRUE);
}






/*
 * Places some staircases near walls
 */
static int alloc_stairs(int feat, int num, int walls)
{
	int y = 0, x = 0;

	int i = 0, j;

	/* Place "num" stairs */
	for (j = 0; j < num; j++)
	{
		while (num)
		{
			i++;

			/* Scan stored locations first. */
			if (i < dun->stair_n)
			{
				/* Get location */
				y = dun->stair[i].y;
				x = dun->stair[i].x;

				/* Require a "naked" floor grid */
				if (!cave_naked_bold(y, x)) continue;

				/* Hack -- only half the time */
				if (rand_int(100) < 50) continue;

				/* Success */
				break;
			}

			/* Then, search at random */
			else
			{
				/* Pick a random grid */
				y = rand_int(DUNGEON_HGT);
				x = rand_int(DUNGEON_WID);

				/* Reduce stairs if unable to place after a while */
				if ((i % 400) == 0) num--;

				/* Must be a safe start grid clear of monsters and objects  XXX */
				if ((i < 650) && (!cave_start_bold(y, x))) continue;

				/* Must be a floor grid clear of monsters and objects  XXX */
				if ((i < 700) && (!cave_naked_bold(y, x))) continue;

				/* Try nearly naked grids if desperate */
				if ((i >= 700) && (!cave_nearly_naked_bold(y, x))) continue;

				/* Player prefers to be near walls. */
				if      ((i % 300) == 0) walls--;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(y, x) < walls) continue;

				/* Success */
				break;
			}
		}

		/* Place fixed stairs */
		place_random_stairs(y, x, feat);
	}

	/* Number of stairs placed */
	return (num);
}



/*
 * Allocates some objects (using "place" and "type")
 */
static int alloc_object(int set, int typ, int num)
{
	int y = 0, x = 0;
	int k, i = 0;

	/* Place some objects */
	for (k = 0; k < num; k++)
	{
		/* Pick a "legal" spot */
		while (num)
		{
			bool room;
			bool surface = (bool)(level_flag & (LF1_SURFACE));

			/* Paranoia */
			i++;

			/* Scan stored quest locations first if allowed to place in rooms. */
			if ((i < dun->quest_n) && ((set == ALLOC_SET_ROOM) || (set == ALLOC_SET_BOTH)))
			{
				/* Get location */
				y = dun->quest[i].y;
				x = dun->quest[i].x;

				/* Require a "clean" floor grid */
				if (!cave_clean_bold(y, x)) continue;

				/* Require a "naked" floor grid */
				if (!cave_naked_bold(y, x)) continue;

				/* Success */
				break;
			}

			/* Scan stored stair locations first if allowed to place in corridors. */
			else if ((i < dun->stair_n) && ((set == ALLOC_SET_CORR) || (set == ALLOC_SET_BOTH)))
			{
				/* Get location */
				y = dun->stair[i].y;
				x = dun->stair[i].x;

				/* Require a "clean" floor grid */
				if (!cave_clean_bold(y, x)) continue;

				/* Require a "naked" floor grid */
				if (!cave_naked_bold(y, x)) continue;

				/* Success */
				break;
			}

			/* Then search at random */
			else
			{
				/* Location */
				y = rand_int(DUNGEON_HGT);
				x = rand_int(DUNGEON_WID);

				/* Reduce objects if unable to place after a while */
				if ((i % 400) == 0) num--;

				/* Require actual floor or ground grid */
				if (((f_info[cave_feat[y][x]].flags1 & (FF1_FLOOR)) == 0)
					&& ((f_info[cave_feat[y][x]].flags3 & (FF3_GROUND)) == 0)) continue;

				/* Check for "room" */
				room = (cave_info[y][x] & (CAVE_ROOM)) ? TRUE : FALSE;

				/* Require corridor? */
				if ((set == ALLOC_SET_CORR) && room && !surface) continue;

				/* Require room? */
				if ((set == ALLOC_SET_ROOM) && !room) continue;

				/* Success */
				break;
			}
		}

		/* Place something */
		switch (typ)
		{
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(y, x);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(y, x);
				break;
			}

			case ALLOC_TYP_GOLD:
			{
				place_gold(y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(y, x, FALSE, FALSE);
				break;
			}

			case ALLOC_TYP_FEATURE:
			{
				place_feature(y, x);
				break;
			}

			case ALLOC_TYP_BODY:
			{
				/* Hack -- pick body type */
				switch (rand_int(3))
				{
					case 0:
						tval_drop_idx = TV_BODY;
						break;
					case 1:
						tval_drop_idx = TV_SKIN;
						break;
					case 2:
						tval_drop_idx = TV_BONE;
						break;
				}

				place_object(y, x, FALSE, FALSE);

				tval_drop_idx = 0;
				break;
			}
		}
	}

	/* Number of objects placed */
	return (num);
}



/*
 * Place contents into dungeon.
 */
static bool place_contents()
{
	int i, k;

	/* Hack -- have less monsters during day light */
	if ((level_flag & (LF1_DAYLIGHT)) != 0) k = (p_ptr->depth / 10);
	else k = (p_ptr->depth / 5);

	if (k > 10) k = 10;
	if (k < 2) k = 2;

	/* Hack -- make sure we have rooms/corridors to place stuff */
	if ((level_flag & (LF1_ROOMS | LF1_TOWER)) != 0)
	{
		/* Generating */
		if (cheat_room) message_add("Placing stairs, rubble, traps.", MSG_GENERIC);

		/* Place 1 or 2 down stairs near some walls */
		if (!alloc_stairs(FEAT_MORE, rand_range(1, 2), 3)) return (FALSE);

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(FEAT_LESS, rand_range(1, 2), 3)) return (FALSE);

		/* Place 2 random stairs near some walls */
		alloc_stairs(0, 2, 3);

		/* Put some rubble in corridors -- we want to exclude towers unless other rooms on level */
		if ((level_flag & (LF1_ROOMS)) != 0) alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k * (((level_flag & (LF1_CRYPT | LF1_BURROWS | LF1_CAVE)) != 0) ? 3 : 1)));

		/* Place some traps in the dungeon */
		alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k * (((level_flag & (LF1_STRONGHOLD | LF1_CRYPT)) != 0) ? 2 : 1)));

		/* Place some features in rooms */
		alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_FEATURE, 1);

		/* If deepest monster is powerful, place lots of bodies around */
		if ((p_ptr->depth > 10) && (r_info[cave_ecology.deepest_race[0]].level > p_ptr->depth * 5 / 4))
		{
			/* Place some bodies in the dungeon */
			alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_BODY, randint(k * (((level_flag & (LF1_STRONGHOLD | LF1_WILD)) != 0) ? 3 : 1)));
		}
	}
	/* FIXME - Check what assumptions we make here. */
	else if (!dun->entrance)
	{
		/* Generating */
		if (cheat_room) message_add("Placing dungeon entrance.", MSG_GENERIC);

		/* Place the dungeon entrance */
		if (!alloc_stairs(FEAT_ENTRANCE, 1, 0)) return (FALSE);
	}

	/* Determine the character location */
	if (((p_ptr->py == 0) || (p_ptr->px == 0)) && ! (new_player_spot())) return (FALSE);

	/* Pick a base number of monsters */
	/* Strongholds, sewers, battlefields and wilderness areas have more monsters */
	i = MIN_M_ALLOC_LEVEL + randint(5 * (((level_flag & (LF1_STRONGHOLD | LF1_SEWER | LF1_BATTLE | LF1_WILD)) != 0) ? 2 : 1));

	/* Generating */
	if (cheat_room) message_add("Placing monsters.", MSG_GENERIC);

	/* If deepest monster is powerful, reduce total number of monsters */
	if (r_info[cave_ecology.deepest_race[0]].level > p_ptr->depth * 5 / 4) i = (i + 2) / 3;

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
	{
		(void)alloc_monster(0, TRUE);
	}

	/* Hack -- make sure we have rooms to place stuff */
	if ((level_flag & (LF1_ROOMS | LF1_TOWER)) != 0)
	{
		/* Generating */
		if (cheat_room) message_add("Placing objects, treasure.", MSG_GENERIC);

		/* Put some objects in rooms */
		alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM * (((level_flag & (LF1_STRONGHOLD)) != 0) ? 2 : 1), 3));

		/* Put some objects/gold in the dungeon */
		alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM * (((level_flag & (LF1_CRYPT)) != 0) ? 2 : 1), 3));
		alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD* (((level_flag & (LF1_MINE)) != 0) ? 2 : 1), 3));
	}

	/* Successfully placed some stuff */
	return (TRUE);
}


/*
 * Generate a new dungeon level
 *
 * Note that "dun_body" adds about 4000 bytes of memory to the stack.
 */
static bool cave_gen(void)
{
	int y, x;

	int by, bx;

	dungeon_zone *zone=&t_info[0].zone[0];

	dun_data dun_body;

	/* Global data */
	dun = &dun_body;
	WIPE(dun, dun_data);

	/* Set allowed guardian locations */
	dun->guard_x0 = 1;
	dun->guard_y0 = 1;
	dun->guard_x1 = DUNGEON_WID - 1;
	dun->guard_y1 = DUNGEON_HGT - 1;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Actual maximum number of rooms on this level */
	dun->row_rooms = DUNGEON_HGT / BLOCK_HGT;
	dun->col_rooms = DUNGEON_WID / BLOCK_WID;

	/* Initialise 'zeroeth' room description */
	room_info[0].flags = 0;

	/* Initialize the room table */
	for (by = 0; by < dun->row_rooms; by++)
	{
		for (bx = 0; bx < dun->col_rooms; bx++)
		{
			dun->room_map[by][bx] = FALSE;
			dun_room[by][bx] = 0;
		}
	}

	/* No "entrance" yet */
	dun->entrance = FALSE;

	/* No rooms yet */
	/* Hack -- the dungeon is room '0' therefore we start counting at room 1 upwards */
	dun->cent_n = 1;

	/* No quests yet */
	dun->quest_n = 0;

	/* No stairs yet */
	dun->stair_n = 0;

	/* Start with no tunnel doorways -- note needs to happen before placement of rooms now */
	dun->next_n = 0;

	/* Start with no themes */
	dun->theme_feat = 0;

	/* Start with no lakes */
	dun->lake_n = 0;

	/* Start with no streamers */
	dun->stream_n = 0;

	/* Start with no flooding */
	dun->flood_feat = 0;

	if (cheat_room) message_add("*** Starting generating dungeon. ***", MSG_GENERIC);

	/* Hack -- Start with base */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			cave_set_feat(y,x,zone->base);
		}
	}

	/* Hack -- Build terrain */
	/* XXX Get rid of this later */
	if ((zone->fill) && (zone->fill != zone->base))
	{
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				build_terrain(y,x,zone->fill);
			}
		}
	}

	/* Build wilderness borders */
	if ((level_flag & (LF1_WILD)) != 0)
	{
		/* Generating */
		if (cheat_room) message_add("Building wilderness.", MSG_GENERIC);

		build_wilderness_borders(FEAT_WALL_EXTRA);
	}

	/* Build special dungeon features */
	if (zone->special)
	{
		/* Various special level types. Never random. */
		dun->special = zone->special;

		message_add(format("Building special dungeon (%d).", dun->special), MSG_GENERIC);

		/* For safety sake we identify and fix special levels */
		switch(dun->special)
		{
			case SPECIAL_GREAT_PILLARS:
				level_flag &= ~(LF1_ROOMS);
			case SPECIAL_GREAT_HALL:
			case SPECIAL_GREAT_CAVE:
				level_flag |= (LF1_WILD);
				level_flag &= ~(LF1_TUNNELS);
				break;
			case SPECIAL_STARBURST:
				level_flag &= ~(LF1_TUNNELS | LF1_ROOMS);
				break;
			case SPECIAL_CHAMBERS:
				level_flag &= ~(LF1_TUNNELS | LF1_ROOMS);
				break;
			case SPECIAL_LABYRINTH:
				level_flag &= ~(LF1_TUNNELS | LF1_ROOMS);
				break;
		}

		/* Then actually build them */
		switch(dun->special)
		{
			case SPECIAL_GREAT_PILLARS:
			case SPECIAL_GREAT_HALL:
			case SPECIAL_GREAT_CAVE:
			{
				int w = dun->special == SPECIAL_GREAT_PILLARS ? 1 : rand_int(2) + 1;
				int w1 = dun->special == SPECIAL_GREAT_PILLARS ? 1 : rand_int(2) + 2;

				/* Place pillars */
				for (by = 1; by < dun->row_rooms; by += dun->special == SPECIAL_GREAT_PILLARS ? 1 : 3)
				{
					for (bx = w;
						 bx < dun->col_rooms;
						 bx += w1)
					{
						if (dun->room_map[by][bx]) continue;

						dun->room_map[by][bx] = TRUE;

						switch(dun->special)
						{
							/* 7x7 pillars with corner cut */
							case SPECIAL_GREAT_PILLARS:
							case SPECIAL_GREAT_HALL:
							{
								for (y = 2; y < BLOCK_HGT - 2; y++)
								{
									for (x = 2; x < BLOCK_WID - 2; x++)
									{
										if (((y == 2) || (y == BLOCK_HGT - 3)) &&
												((x == 2) || (x == BLOCK_HGT - 3))) continue;

										/* Draw pillar */
										cave_set_feat(by * BLOCK_HGT + y, bx * BLOCK_WID + x, FEAT_WALL_EXTRA);
									}
								}
								break;
							}
							/* Fractal 9x9 pillars */
							case SPECIAL_GREAT_CAVE:
							{
								fractal_map map;
								fractal_template *t_ptr;

								/* Choose a template for the pool */
								while (TRUE)
								{
									/* Pick a random template */
									int which = rand_int(N_ELEMENTS(fractal_repository));

									t_ptr = &fractal_repository[which];

									/* Found the desired template type? */
									if (t_ptr->type == FRACTAL_TYPE_9x9) break;
								}

								/* Create and reset the fractal map */
								map = fractal_map_create(t_ptr);

								/* Complete the map */
								fractal_map_complete(map, t_ptr);

								/* Copy the map into the dungeon */
								for (y = 0; y < 9; y++)
								{
									for (x = 0; x < 9; x++)
									{
										/* Translate map coordinates to dungeon coordinates */
										int yy = by * BLOCK_HGT + 1 + y;
										int xx = bx * BLOCK_WID + 1 + x;

										/* Ignore non-floors grid types in the map */
										if (map[y][x] != FRACTAL_FLOOR) continue;

										/* Ignore annoying locations */
										if (!in_bounds_fully(yy, xx)) continue;

										/* Set the feature */
										cave_set_feat(yy, xx, FEAT_WALL_EXTRA);
									}
								}

								/* Free resources */
								FREE(map);

								break;
							}
						}
					}
				}
				break;
			}
			case SPECIAL_STARBURST:
				if (cheat_room) message_add("Building star burst level.", MSG_GENERIC);
				if (!generate_starburst_room(4, 4, DUNGEON_HGT - 4, DUNGEON_WID - 4,
					(f_info[zone->fill].flags1 & (FF1_WALL)) == 0 ? zone->fill : FEAT_FLOOR,
						f_info[zone->fill].edge, (STAR_BURST_RAW_FLOOR | STAR_BURST_RAW_EDGE))) return (FALSE);
				break;
			case SPECIAL_CHAMBERS:
				if (cheat_room) message_add("Building chambers level.", MSG_GENERIC);
				if (!build_chambers(1, 1, DUNGEON_HGT -1, DUNGEON_WID - 1, 45, FALSE)) return (FALSE);
				break;
			case SPECIAL_LABYRINTH:
			{
				int width_wall, width_path;
				int ydim, xdim;
				int height = DUNGEON_HGT - 2;
				int width = DUNGEON_WID - 2;
				u32b maze_flags = (MAZE_DEAD) | (zone->fill ? (MAZE_POOL) : (0L));

				if ((level_flag & (LF1_THEME)) == 0)
				{
					switch(randint(5))
					{
						case 1: level_flag |= (LF1_LABYRINTH); break;
						case 2: level_flag |= (LF1_CRYPT); break;
						case 3: level_flag |= (LF1_CAVE); break;
						case 4: level_flag |= (LF1_STRONGHOLD); break;
						case 5: level_flag |= (LF1_SEWER); break;
					}
				}

				/* Choose path and wall width */
				width_wall = rand_int(9) / 3 + 2;
				width_path = rand_int(15) / 6 + 1;

				/* Ensure bigger paths on stronghold levels. This allows these corridors to connect correctly to the maze. */
				if (level_flag & (LF1_STRONGHOLD)) width_path++;

				/* Fill crypt mazes with pillars */
				else if (level_flag & (LF1_CRYPT))
				{
					width_path = 3;
					maze_flags |= (MAZE_CRYPT);
				}

				/* Fill cave mazes with lots of twisty passages */
				else if (level_flag & (LF1_CAVE))
				{
					width_path = 1;
					width_wall++;
					maze_flags |= (MAZE_CAVE);
				}

				/* Calculate maze size */
				ydim = (height + width_wall - 1) / (width_wall + width_path);
				xdim = (width + width_wall - 1) / (width_wall + width_path);

				/* Ensure minimums */
				if (ydim < 3) ydim = 3;
				if (xdim < 3) xdim = 3;

				/* Recalculate height and width based on maze size */
				height = (ydim * width_path) + ((ydim - 1) * width_wall);
				width = (xdim * width_path) + ((xdim - 1) * width_wall);

				if (!draw_maze((DUNGEON_HGT - height) / 2, (DUNGEON_WID - width) / 2,
						(DUNGEON_HGT - height) / 2 + height, (DUNGEON_WID - width) / 2 + width, FEAT_WALL_EXTRA,
				    FEAT_FLOOR, width_wall, width_path, zone->fill, maze_flags)) return (FALSE);
				break;
			}
		}
	}

	/* No features in a tower above the surface */
	else if (((level_flag & (LF1_TOWER)) == 0) || ((level_flag & (LF1_SURFACE)) != 0))
	{
		/* Generating */
		if (cheat_room) message_add("Building nature.", MSG_GENERIC);
		
		build_nature();
	}

	/* Set up the monster ecology before placing rooms */
	/* XXX Very early levels boring with ecologies enabled */
	if (p_ptr->depth > 2)
	{
		/* Generating */
		if (cheat_room) message_add("Generating ecology.", MSG_GENERIC);

		/* Place guardian if permitted */
		if ((level_flag & (LF1_GUARDIAN)) != 0)
		{
			init_ecology(actual_guardian(zone->guard, p_ptr->dungeon, zone - t_info[p_ptr->dungeon].zone));
		}
		/* Place any monster */
		else
		{
			init_ecology(0);
		}
	}
	else
	{
		/* Wipe the previous dungeon ecology */
		(void)WIPE(&cave_ecology, ecology_type);
	}

	/* Hack -- build a tower in the centre of the level */
	if (level_flag & (LF1_TOWER))
	{
		/* Generating */
		if (cheat_room) message_add("Building tower.", MSG_GENERIC);

		/* Place the tower */
		place_tower();
	}

	/* Build some rooms or tunnel endpoints */
	if ((level_flag & (LF1_ROOMS | LF1_TUNNELS)) != 0)
	{
		/* Generating */
		if (cheat_room) message_add("Building rooms.", MSG_GENERIC);

		/* Place the rooms */
		if (!place_rooms()) return (FALSE);
	}

	/* Build boundary walls */
	place_boundary_walls();

	/* Build some tunnels */
	if ((level_flag & (LF1_TUNNELS)) != 0)
	{
		if (!place_tunnels()) return (FALSE);
	}

	/* Hack -- No destroyed "quest" levels */
	if (level_flag & (LF1_QUEST)) level_flag &= ~(LF1_DESTROYED);

	/* Hack -- No shallow destroyed levels */
	if (p_ptr->depth <= 20) level_flag &= ~(LF1_DESTROYED);

	/* Destroy the level if necessary */
	if ((level_flag & (LF1_DESTROYED)) != 0) destroy_level();

	/* Build streamers and entrance markings in caves */
	place_decorations();

	/* Ensure guardian monsters */
	if ((level_flag & (LF1_GUARDIAN)) != 0)
	{
		/* Place the guardian in the dungeon */
		if (!place_guardian(dun->guard_y0, dun->guard_x0, dun->guard_y1, dun->guard_x1)) return (FALSE);
	}

	/* Build traps, treasure, rubble etc and place the player */
	if (!place_contents()) return (FALSE);

	/* Apply illumination */
	if ((level_flag & (LF1_SURFACE)) != 0) town_illuminate((level_flag & (LF1_DAYLIGHT)) != 0);

	/* Generating */
	if (cheat_room) message_add("Finished generating dungeon.", MSG_GENERIC);

	/* Hack -- restrict teleporation in towers */
	/* XXX Important that this occurs after placing the player */
	if ((level_flag & (LF1_TOWER)) != 0)
	{
		room_info[1].flags = (ROOM_ICKY);
	}

	/* Hack -- set flags on dungeon */
	/* XXX Important that this occurs after placing the player */
	room_info[0].flags |= zone->flags2;

	/* Generation successful */
	return(TRUE);
}



/*
 * Builds a store at a given pseudo-location
 *
 * As of 2.7.4 (?) the stores are placed in a more "user friendly"
 * configuration, such that the four "center" buildings always
 * have at least four grids between them, to allow easy running,
 * and the store doors tend to face the middle of town.
 *
 * The stores now lie inside boxes from 3-9 and 12-18 vertically,
 * and from 7-17, 21-31, 35-45, 49-59.  Note that there are thus
 * always at least 2 open grids between any disconnected walls.
 *
 * Note the use of "town_illuminate()" to handle all "illumination"
 * and "memorization" issues.
 */
static void build_store(int feat, int yy, int xx)
{
	int y, x, y0, x0, y1, x1, y2, x2, tmp;

	/* Hack -- extract char value */
	byte d_char = f_info[feat].d_char;

	/* Hack -- don't build building for some 'special locations' */
	bool building = (((d_char > '0') && (d_char <= '8')) || (d_char == '+'));

	town_type *t_ptr = &t_info[p_ptr->dungeon];
	dungeon_zone *zone=&t_ptr->zone[0];;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Find the "center" of the store */
	y0 = yy * 9 + 6;
	x0 = xx * 14 + 12;

	/* Determine the store boundaries */
	y1 = y0 - randint((yy == 0) ? 3 : 2);
	y2 = y0 + randint((yy == 1) ? 3 : 2);
	x1 = x0 - randint(5);
	x2 = x0 + randint(5);

	/* Hack -- decrease building size to create space for small terrain */
	if (zone->small)
	{
		if (x2 == 31) x2--;
		if (x1 == 35) x1++;
		if (x1 == 36) x1++;
		if (y2 == 9) y2--;
		if (y1 == 12) y1++;
	}

	/* Create a building? */
	if (building)
	{
		/* Build an invulnerable rectangular building */
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				/* Create the building */
				cave_set_feat(y, x, FEAT_PERM_EXTRA);
			}
		}
	}

	/* Pick a door direction (S,N,E,W) */
	tmp = rand_int(4);

	/* Re-roll "annoying" doors */
	if (((tmp == 0) && (yy == 1)) ||
	    ((tmp == 1) && (yy == 0)) ||
	    ((tmp == 2) && (xx == 3)) ||
	    ((tmp == 3) && (xx == 0)))
	{
		/* Pick a new direction */
		tmp = rand_int(4);
	}

	/* Place the store door */
	generate_door(y1, x1, y2, x2, tmp, feat);

	/* Let monsters shop as well */
	if (u_info[f_info[feat].power].base >= STORE_MIN_BUY_SELL)
	{
		room_info[0].flags |= (ROOM_TOWN);
	}
}




/*
 * Generate the "consistent" town features, and place the player
 *
 * Hack -- play with the R.N.G. to always yield the same town
 * layout, including the size and shape of the buildings, the
 * locations of the doorways, and the location of the stairs.
 */
static void town_gen_hack(void)
{
	int y, x, k, n;
	bool placed_player = FALSE;

	int rooms[MAX_STORES];

	town_type *t_ptr = &t_info[p_ptr->dungeon];
	dungeon_zone *zone=&t_ptr->zone[0];;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistant town layout */
	Rand_value = seed_town;

	/* Then place some floors */
	for (y = 1; y < TOWN_HGT-1; y++)
	{
		for (x = 1; x < TOWN_WID-1; x++)
		{
			/* Create terrain on top */
			build_terrain(y, x, zone->big);
		}
	}

	/* MegaHack -- place small terrain north to south & bridge east to west */
	if (zone->small)
	{
		for (y = 1; y < TOWN_HGT-1; y++)
		{
			for (x = 32; x < 36; x++)
			{
				/* Create terrain on top */
				build_terrain(y, x, zone->small);
			}
		}

		for (y = 10; y < 12; y++)
		{
			for (x = 1; x < TOWN_WID-1; x++)
			{
				/* Create terrain on top */
				cave_alter_feat(y, x, FS_BRIDGE);
			}
		}

		/* Hack -- town square */
		if (feat_state(zone->big, FS_BRIDGE) == zone->small)
		{
			for (y = 2; y < 20; y++)
			{
				for (x = 6; x < 61; x++)
				{
					/* Exclude already built terrain */
					if ((y < 10) || (y >= 12) || (x< 32) || (x >= 36))
					{
						/* Create terrain on top */
						build_terrain(y, x, zone->small);
					}
				}
			}
		}
	}

	/* Prepare an Array of "remaining stores", and count them */
	for (n = 0; n < MAX_STORES; n++) rooms[n] = n;

	/* Place two rows of stores */
	for (y = 0; y < 2; y++)
	{
		/* Place four stores per row */
		for (x = 0; x < 4; x++)
		{
			/* Pick a random unplaced store */
			k = ((n <= 1) ? 0 : rand_int(n));

			/* Build that store at the proper location */
			if (t_ptr->store[rooms[k]]) build_store(t_ptr->store[rooms[k]], y, x);

			/* Shift the stores down, remove one store */
			rooms[k] = rooms[--n];
		}
	}

	/* Place a down stairs if necessary */
	if (level_flag & (LF1_MORE))
	{
		while (TRUE)
		{
			/* Pick a location at least "three" from the outer walls */
			y = rand_range(3, TOWN_HGT - 4);
			x = rand_range(3, TOWN_WID - 4);

			/* Require a "starting" floor grid */
			if (cave_start_bold(y, x)) break;
		}

		if (level_flag & (LF1_SURFACE))
		{
			/* Clear previous contents, add dungeon entrance */
			place_random_stairs(y, x, FEAT_ENTRANCE);
		}
		else
		{
			/* No dungeon entrances in underground towns/towers */
			cave_set_feat(y, x, FEAT_MORE);
		}
		
		/* Place the player on the map, unless coming up the stairs */
		if (p_ptr->create_stair != FEAT_LESS)
		{
			player_place(y, x, TRUE);
			placed_player = TRUE;
		}
	}

	/* Place an up stairs if necessary */
	if (level_flag & (LF1_LESS))
	{
		/* Place the stairs */
		while (TRUE)
		{
			/* Pick a location at least "three" from the outer walls */
			y = rand_range(3, TOWN_HGT - 4);
			x = rand_range(3, TOWN_WID - 4);

			/* Require a "naked" floor grid */
			if (cave_naked_bold(y, x)) break;
		}

		/* Clear previous contents, add the other stairs */
		cave_set_feat(y, x, FEAT_LESS);
		
		/* Place the player if we haven't done so yet */
		if (!placed_player)
		{
			player_place(y, x, TRUE);
			placed_player = TRUE;
		}
	}

	/* Place the player if we haven't done so yet (if there are no stairs) */
	if (!placed_player)
	{
		while (TRUE)
		{
			/* Pick a location at least "three" from the outer walls */
			y = rand_range(3, TOWN_HGT - 4);
			x = rand_range(3, TOWN_WID - 4);
			
			/* Require a "starting" floor grid */
			if (cave_start_bold(y, x)) break;
		}
		
		player_place(y, x, TRUE);
	}
	
	/* Hack -- use the "complex" RNG */
	Rand_quick = FALSE;
}




/*
 * Town logic flow for generation of new town
 *
 * We start with a fully wiped cave of normal floors.
 *
 * Note that town_gen_hack() plays games with the R.N.G.
 *
 * This function does NOT do anything about the owners of the stores,
 * nor the contents thereof.  It only handles the physical layout.
 *
 * We place the player on the stairs at the same time we make them.
 *
 * Hack -- since the player always leaves the dungeon by the stairs,
 * he is always placed on the stairs, even if he left the dungeon via
 * word of recall or teleport level.
 */
static bool town_gen(void)
{
	int i, y, x;

	int residents;

	int by,bx;

	town_type *t_ptr = &t_info[p_ptr->dungeon];
	dungeon_zone *zone=&t_ptr->zone[0];;

	/* Get the zone */
	get_zone(&zone,p_ptr->dungeon,p_ptr->depth);

	/* Initialize the room table */
	for (by = 0; by < MAX_ROOMS_ROW; by++)
	{
		for (bx = 0; bx < MAX_ROOMS_COL; bx++)
		{
			dun_room[by][bx] = 0;
		}
	}

	/* Initialise 'zeroeth' room description */
	room_info[0].flags = 0;

	/* Town does not have an ecology */
	cave_ecology.num_races = 0;
	cave_ecology.ready = FALSE;

	/* Day time */
	if ((level_flag & (LF1_DAYLIGHT)) != 0)
	{
		/* Number of residents */
		residents = MIN_M_ALLOC_TD;
	}

	/* Night time / underground */
	else
	{
		/* Number of residents */
		residents = MIN_M_ALLOC_TN;

	}

	/* Start with solid walls */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Create "solid" perma-wall */
			cave_set_feat(y, x, FEAT_PERM_SOLID);
		}
	}

	/* Then place some floors */
	for (y = 1; y < TOWN_HGT-1; y++)
	{
		for (x = 1; x < TOWN_WID-1; x++)
		{
			/* Create empty ground */
			cave_set_feat(y, x, f_info[zone->big].flags1 & (FF1_FLOOR) ? zone->big : FEAT_GROUND);
		}
	}

	/* Build stuff */
	town_gen_hack();

	/* Apply illumination */
	if ((level_flag & (LF1_SURFACE)) != 0) town_illuminate((level_flag & (LF1_DAYLIGHT)) != 0);

	/* Ensure guardian monsters */
	if (((level_flag & (LF1_GUARDIAN)) != 0) && ((level_flag & (LF1_DAYLIGHT)) == 0))
	{
		/* Place the guardian in the town */
		place_guardian(3, 3, TOWN_HGT - 4, TOWN_WID -4);
	}
	else
	{
		/* Ensure wandering monsters suit the dungeon level */
		get_mon_num_hook = dun_level_mon;

		/* Prepare allocation table */
		get_mon_num_prep();

		/* Make some residents */
		for (i = 0; i < residents; i++)
		{
			/* Make a resident */
			(void)alloc_monster(3, TRUE);
		}

		get_mon_num_hook = NULL;

		/* Prepare allocation table */
		get_mon_num_prep();
	}

	/* Always successful? */
	return(TRUE);
}


/*
 * Generate a random dungeon level
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Note that this function resets "cave_feat" and "cave_info" directly.
 */
void generate_cave(void)
{
	int i, j, y, x, num;

	quest_event event;

	/* Use this to allow quests to succeed or fail */
	WIPE(&event, quest_event);

	/* Set up departure event */
	event.flags = EVENT_TRAVEL;
	event.dungeon = p_ptr->dungeon;
	event.level = p_ptr->depth - min_depth(p_ptr->dungeon);

	/* Reset the monster generation level; make level feeling interesting */
	monster_level = p_ptr->depth >= 4 ? p_ptr->depth + 2 :
		(p_ptr->depth >= 2 ? p_ptr->depth + 1 : p_ptr->depth);

	/* The dungeon is not ready */
	character_dungeon = FALSE;

	/* Important - prevent getting stuck in rock */
	p_ptr->word_return = 0;
	p_ptr->return_y = 0;
	p_ptr->return_x = 0;

	/* Generating */
	if (cheat_room) message_add(format("Generating new level (level %d in %s)", p_ptr->depth, t_name + t_info[p_ptr->dungeon].name), MSG_GENERIC);

	/* Reset level flags */
	level_flag = 0;

	/* Hack -- ensure quest monsters not randomly generated */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		quest_type *q_ptr = &q_list[i];
		quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);

		/* Hack -- player's actions don't change level */
		if (q_ptr->stage == QUEST_ACTION) continue;

		/* Hack: Quest occurs anywhere - don't force quest items on the level. */
		if (!qe_ptr->dungeon) continue;

		/* Quest doesn't occur on this level */
		else if ((qe_ptr->dungeon != p_ptr->dungeon) ||
			(qe_ptr->level != (p_ptr->depth - min_depth(p_ptr->dungeon)))) continue;

		/* Mark the level as a quest level */
		level_flag |= (LF1_QUEST);

		/* Mark questors - features take precedence */
		if ((!qe_ptr->feat) && (qe_ptr->race)) r_info[qe_ptr->race].flags1 |= (RF1_QUESTOR);
	}

	/* Are we using seeded dungeon generation? */
	if (seed_dungeon)
	{
		/* Hack -- Use the "simple" RNG */
		Rand_quick = TRUE;

		/* Hack -- Induce consistant dungeon layout */
		Rand_value = seed_dungeon;
	}

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;

		cptr why = NULL;

		/* Reset */
		o_max = 1;
		m_max = 1;

		/* There is no dynamic terrain */
		dyna_full = FALSE;
		dyna_n = 0;

		/* Initialise level flags */
		init_level_flags();

		/* Reset 'quest' status of monsters and count of current number on level */
		for (i = 0; i < z_info->r_max; i++)
		{
			r_info[i].flags1 &= ~(RF1_QUESTOR);
			r_info[i].cur_num = 0;
		}

		/* Start with a blank cave */
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				/* No flags */
				cave_info[y][x] = 0;

				/* No flags */
				play_info[y][x] = 0;

				/* No features */
				cave_feat[y][x] = 0;

				/* No objects */
				cave_o_idx[y][x] = 0;

				/* No monsters */
				cave_m_idx[y][x] = 0;

				/* No flow */
				cave_cost[y][x] = 0;
				cave_when[y][x] = 0;
			}
		}

		/* Clear room info */
		for (i = 0; i < DUN_ROOMS; i++)
		{
			/* Initialise room */
			room_info[i].flags = 0;

			room_info[i].section[0] = -1;
			room_info[i].type = ROOM_NONE;
			room_info[i].vault = 0;
			room_info[i].deepest_race = 0;

			for (j = 0; j < MAX_THEMES; j++)
			{
				room_info[i].theme[j] = 0;
			}
		}

		/* Mega-Hack -- no player yet */
		p_ptr->px = p_ptr->py = 0;

		/* Hack -- illegal panel */
		p_ptr->wy = DUNGEON_HGT;
		p_ptr->wx = DUNGEON_WID;

		/* Reset the monster generation level; make level feeling interesting */
		monster_level = p_ptr->depth >= 4 ? p_ptr->depth + 2 :
			(p_ptr->depth >= 2 ? p_ptr->depth + 1 : p_ptr->depth);

		/* Reset the object generation level */
		object_level = p_ptr->depth;

		/* Nothing special here yet */
		good_item_flag = FALSE;

		/* Nothing good here yet */
		rating = 0;

		/* Build the town */
		if (level_flag & (LF1_TOWN))
		{
			/* Make a town */
			if (!town_gen()) okay = FALSE;

			/* Report why */
			if (cheat_room) why = "defective town";
		}

		/* Build a real level */
		else
		{
			/* Make a dungeon */
			if (!cave_gen()) okay = FALSE;

			/* Report why */
			if (cheat_room) why = "defective dungeon";
		}

		/* Ensure quest components */
		ensure_quest();

		/* Extract the feeling */
		if (rating > 100) feeling = 2;
		else if (rating > 70) feeling = 3;
		else if (rating > 40) feeling = 4;
		else if (rating > 30) feeling = 5;
		else if (rating > 20) feeling = 6;
		else if (rating > 10) feeling = 7;
		else if (rating > 5) feeling = 8;
		else if (rating > 0) feeling = 9;
		else feeling = 10;

		/* Prevent object over-flow */
		if (o_max >= z_info->o_max)
		{
			/* Message */
			why = "too many objects";

			/* Message */
			okay = FALSE;
		}

		/* Prevent monster over-flow */
		if (m_max >= z_info->m_max)
		{
			/* Message */
			why = "too many monsters";

			/* Message */
			okay = FALSE;

		}

		/* Are we using seeded dungeon generation? */
		if (seed_dungeon)
		{
			/* Hack -- use the "complex" RNG */
			Rand_quick = FALSE;

			/* Store the dungeon seed */
			seed_last_dungeon = seed_dungeon;

			/* Get new seed */
			seed_dungeon = rand_int(0x10000000);
		}

		/* Accept */
		if (okay) break;

		/* Message */
		if (why) message_add(format("Generation restarted (%s)", why), MSG_GENERIC);

		/* Interrupt? */
		if ((cheat_xtra) && (why) && (get_check("Interrupt? "))) break;

		/* Wipe the objects */
		wipe_o_list();

		/* Wipe the monsters */
		wipe_m_list();

		/* Wipe the regions */
		wipe_region_piece_list();
		wipe_region_list();

		/* Safety */
		if (num > 100)
		{
			msg_format("A bug hidden in %s leaps out at you and takes you elsewhere.", t_name + t_info[p_ptr->dungeon].name);

			/* Banish the player to nowhere town. */
			p_ptr->dungeon = 0;
			p_ptr->depth = 0;
		}
	}


	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* XXX Hurt light players sleep until it gets dark before entering surface levels */
	/* FIXME - Should really check that the player knows they are vulnerable to light */
	if (((p_ptr->cur_flags4 & (TR4_HURT_LITE)) != 0) && ((level_flag & (LF1_DAYLIGHT)) != 0))
	{
		/* Hack -- Set time to one turn before sun down */
		turn += ((10L * TOWN_DAWN) / 2) - (turn % (10L * TOWN_DAWN)) - 1;

		/* XXX Set food, etc */

		/* Inform the player */
		msg_print("You sleep during the day.");
	}

	/* Set dodging - 'just appeared' */
	p_ptr->dodging = 9;

	/* Redraw state */
	p_ptr->redraw |= (PR_STATE);

	/* Notify the player of changes to the map */
	if (!t_info[p_ptr->dungeon].visited)
	{
		/* This breaks for Angband */
		if (p_ptr->dungeon) for (i = 0; i < z_info->t_max; i++)
		{
		  char str[46];
		  char m_name[80];

			if (t_info[i].replace_ifvisited == p_ptr->dungeon)
			{
			  long_level_name(str, i, 0);
			  msg_format("%^s has fallen.", str);

			  long_level_name(str, t_info[i].replace_with, 0);
			  msg_format("%^s now stands in its place.", str);
			}

			if ((t_info[i].town_lockup_ifvisited == p_ptr->dungeon) && (r_info[t_info[i].town_lockup_monster].max_num > 0))
			{
				/* Get the name */
				race_desc(m_name, sizeof(m_name), t_info[i].town_lockup_monster, 0x400, 1);

			  long_level_name(str, i, 0);
			  msg_format("%^s now terrorizes %s.", m_name, str);
			}

			if (t_info[i].guardian_ifvisited == p_ptr->dungeon) {
			  if (r_info[t_info[i].replace_guardian].max_num > 0)
			    {
					/* Get the name */
					race_desc(m_name, sizeof(m_name), t_info[i].replace_guardian, 0x400, 1);

			      long_level_name(str, i, 0);
			      msg_format("%^s now guards %s.", m_name, str);
			    }
			  else {
			    /* remove the guardian to avoid fake victories */
			    t_info[i].guardian_ifvisited = 0;
			    t_info[i].replace_guardian = 0;
			  }
			}
		}

		/* Set this dungeon as visited */
		t_info[p_ptr->dungeon].visited = TRUE;

		/* Show tip */
		queue_tip(format("dungeon%d.txt", p_ptr->dungeon));
	}

	/* Set maximum depth for this dungeon */
	if (t_info[p_ptr->dungeon].attained_depth < p_ptr->depth)
	{
		for (i = t_info[p_ptr->dungeon].attained_depth; i < p_ptr->depth; i++)
		{
			/* Style tips */
			queue_tip(format("depth%d-%d.txt", p_ptr->dungeon, i));
		}

		/* Set new maximum depth */
		t_info[p_ptr->dungeon].attained_depth = p_ptr->depth;
	}

	/* Hit by the plague */
	if (p_ptr->disease) suffer_disease(TRUE);

	/* Set the turn the player entered this level (again) */
	old_turn = turn;

	/* Check for quest failure. */
	while (check_quest(&event, TRUE));

}
