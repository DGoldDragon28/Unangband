/*
 * File: options.c
 * Purpose: Options table and definitions.
 *
 * Copyright (c) 1997 Ben Harrison
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#include "angband.h"
#include "option.h"

/*
 * Option screen interface
 */
const byte option_page[OPT_PAGE_MAX][OPT_PAGE_PER] =
{
	/* Interface */
	{
		OPT_use_sound,
		OPT_rogue_like_commands,
		OPT_use_old_target,
		OPT_pickup_always,
		OPT_pickup_inven,
		OPT_pickup_detail,
		OPT_easy_alter,
		OPT_easy_open,
		OPT_show_lists,
		OPT_easy_autos,
		OPT_easy_search,
		OPT_depth_in_feet,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE
	},

	/* Display */
	{
		OPT_hilite_player,
 		OPT_center_player,
		OPT_show_piles,
		OPT_show_flavors,
		OPT_view_flavors,
		OPT_show_labels,
		OPT_view_yellow_lite,
		OPT_view_bright_lite,
		OPT_view_granite_lite,
		OPT_view_special_lite,
		OPT_view_perma_grids,
		OPT_view_torch_grids,
		OPT_view_glowing_lite,
		OPT_show_sidebar,
		/*OPT_show_itemlist,*/
		OPT_room_names,
		OPT_room_descriptions,
		OPT_show_tips,
		OPT_NONE,
		OPT_NONE
	},

	/* Warning */
	{
		OPT_disturb_move,
		OPT_disturb_near,
		OPT_disturb_detect,
		OPT_disturb_state,
		OPT_quick_messages,
		OPT_auto_more,
		OPT_ring_bell,
		OPT_flush_failure,
		OPT_flush_disturb,
		OPT_easy_corpses,
		OPT_easy_more,
		OPT_auto_monlist,
		OPT_easy_monlist,
		OPT_verify_mana,
		OPT_view_unsafe_grids,
		OPT_view_detect_grids,
		OPT_view_fogged_grids,
		OPT_autosave_backup,
		OPT_ally_messages
	},

	/* Birth/Difficulty */
	{
		OPT_birth_randarts,
		OPT_birth_ironman,
		OPT_birth_no_stores,
		OPT_birth_no_selling,
		OPT_birth_no_artifacts,
		OPT_birth_no_stacking,
		OPT_birth_no_stairs,
		OPT_birth_no_identify,
		OPT_birth_lore,
		OPT_birth_auto,
		OPT_birth_campaign,
		OPT_birth_haggle,
		OPT_birth_beginner,
		OPT_birth_intermediate,
		OPT_birth_first_time,
		OPT_birth_reseed_artifacts,
		OPT_birth_rand_stats,
		OPT_birth_evil,
		OPT_birth_gollum
	},

	/* Cheat */
	{
		OPT_cheat_peek,
		OPT_cheat_hear,
		OPT_cheat_room,
		OPT_cheat_xtra,
		OPT_cheat_know,
		OPT_cheat_live,
		OPT_cheat_wall,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE,
		OPT_NONE
	}
};


typedef struct
{
	const char *name;
	const char *description;
	bool normal;
} option_entry;

static option_entry options[OPT_MAX] =
{
{ "rogue_like_commands", "Rogue-like commands",                         FALSE }, /* 0 */
{ "quick_messages",      "Activate quick messages",                     TRUE },  /* 1 */
{ "use_sound",           "Use sound",                                   FALSE }, /* 2 */
{ "pickup_detail",       "Be verbose when picking things up",           TRUE },  /* 3 */
{ "use_old_target",      "Use old target by default",                   FALSE }, /* 4 */
{ "pickup_always",       "Always pickup items",                         FALSE }, /* 5 */
{ "pickup_inven",        "Always pickup items matching inventory",      TRUE },  /* 6 */
{ NULL,                  NULL,                                          FALSE }, /* 7 */
{ NULL,                  NULL,                                          FALSE }, /* 8 */
{ NULL,                  NULL,                                          FALSE }, /* 9 */
{ "show_labels",         "Show labels in equipment listings",           TRUE },  /* 10 */
{ "show_lists",          "Always show lists",                           TRUE },  /* 11 */
{ NULL,                  NULL,                                          FALSE }, /* 12 */
{ NULL,                  NULL,                                          FALSE }, /* 13 */
{ "ring_bell",           "Audible bell (on errors, etc)",               TRUE },  /* 14 */
{ "show_flavors",        "Show flavors in object descriptions",         TRUE },  /* 15 */
{ NULL,                  NULL,                                          FALSE }, /* 16 */
{ NULL,                  NULL,                                          FALSE }, /* 17 */
{ NULL,                  NULL,                                          FALSE }, /* 18 */
{ NULL,                  NULL,                                          FALSE }, /* 19 */
{ "disturb_move",        "Disturb whenever any monster moves",          FALSE }, /* 20 */
{ "disturb_near",        "Disturb whenever viewable monster moves",     TRUE },  /* 21 */
{ "disturb_detect",      "Disturb whenever leaving detected area",      TRUE },  /* 22 */
{ "disturb_state",       "Disturb whenever player state changes",       TRUE },  /* 23 */
{ NULL,                  NULL,                                          FALSE }, /* 24 */
{ NULL,                  NULL,                                          FALSE }, /* 25 */
{ NULL,                  NULL,                                          FALSE }, /* 26 */
{ NULL,                  NULL,                                          FALSE }, /* 27 */
{ NULL,                  NULL,                                          FALSE }, /* 28 */
{ NULL,                  NULL,                                          FALSE }, /* 29 */
{ NULL,                  NULL,                                          FALSE }, /* 30 */
{ NULL,                  NULL,                                          FALSE }, /* 31 */
{ NULL,                  NULL,                                          FALSE }, /* 32 */
{ NULL,                  NULL,                                          FALSE }, /* 33 */
{ NULL,                  NULL,                                          FALSE }, /* 34 */
{ NULL,                  NULL,                                          FALSE }, /* 35 */
{ NULL,                  NULL,                                          FALSE }, /* 36 */
{ NULL,                  NULL,                                          FALSE }, /* 37 */
{ "view_perma_grids",    "Map remembers all perma-lit grids",           TRUE },  /* 38 */
{ "view_torch_grids",    "Map remembers all torch-lit grids",           TRUE },  /* 39 */
{ NULL,                  NULL,                                          FALSE }, /* 40 */
{ NULL,                  NULL,                                          FALSE }, /* 41 */
{ NULL,                  NULL,                                          FALSE }, /* 42 */
{ NULL,                  NULL,                                          FALSE }, /* 43 */
{ NULL,                  NULL,                                          FALSE }, /* 44 */
{ NULL,                  NULL,                                          FALSE }, /* 45 */
{ NULL,                  NULL,                                          FALSE }, /* 46 */
{ NULL,                  NULL,                                          FALSE }, /* 47 */
{ NULL,                  NULL,                                          FALSE }, /* 48 */
{ NULL,                  NULL,                                          FALSE }, /* 49 */
{ NULL,                  NULL,                                          FALSE }, /* 50 */
{ NULL,                  NULL,                                          FALSE }, /* 51 */
{ "flush_failure",       "Flush input on various failures",             TRUE },  /* 52 */
{ "flush_disturb",       "Flush input whenever disturbed",              FALSE }, /* 53 */
{ NULL,                  NULL,                                          FALSE }, /* 54 */
{ NULL,                  NULL,                                          FALSE }, /* 55 */
{ NULL,                  NULL,                                          FALSE }, /* 56 */
{ NULL,                  NULL,                                          FALSE }, /* 57 */
{ NULL,                  NULL,                                          FALSE }, /* 58 */
{ "hilite_player",       "Hilite the player with the cursor",           FALSE }, /* 59 */
{ "view_yellow_lite",    "Use special colors for torch lite",           FALSE }, /* 60 */
{ "view_bright_lite",    "Use special colors for field of view",        TRUE },  /* 61 */
{ "view_granite_lite",   "Use special colors for wall grids",           FALSE }, /* 62 */
{ "view_special_lite",   "Use special colors for floor grids",          TRUE },  /* 63 */
{ "easy_open",           "Open/Disarm/Close without direction",         FALSE }, /* 64 */
{ "easy_alter",          "Open/Disarm doors/traps on movement",         FALSE }, /* 65 */
{ NULL,                  NULL,                                          FALSE }, /* 66 */
{ "show_piles",          "Show stacks using special attr/char",         FALSE }, /* 67 */
{ "center_player",       "Center map continuously",                     FALSE }, /* 68 */
{ NULL,                  NULL,                                          FALSE }, /* 69 */
{ NULL,                  NULL,                                          FALSE }, /* 70 */
{ "auto_more",           "Automatically clear '-more-' prompts",        FALSE }, /* 71 */
{ NULL,                  NULL,                                          FALSE }, /* 72 */
{ NULL,                  NULL,                                          FALSE }, /* 73 */
{ NULL,                  NULL,                                          FALSE }, /* 74 */
{ NULL,                  NULL,                                          FALSE }, /* 75 */
{ NULL,                  NULL,                                          FALSE }, /* 76 */
{ "mouse_movement",      "Allow mouse clicks to move the player",       FALSE }, /* 77 */
{ "mouse_buttons",       "Show mouse status line buttons",              FALSE }, /* 78 */
{ NULL,                  NULL,                                          FALSE }, /* 79 */
{ NULL,                  NULL,                                          FALSE }, /* 80 */
{ NULL,                  NULL,                                          FALSE }, /* 81 */
{ NULL,                  NULL,                                          FALSE }, /* 82 */
{ NULL,                  NULL,                                          FALSE }, /* 83 */
{ NULL,                  NULL,                                          FALSE }, /* 84 */
{ NULL,                  NULL,                                          FALSE }, /* 85 */
{ NULL,                  NULL,                                          FALSE }, /* 86 */
{ NULL,                  NULL,                                          FALSE }, /* 87 */
{ NULL,                  NULL,                                          FALSE }, /* 88 */
{ NULL,                  NULL,                                          FALSE }, /* 89 */
{ "room_descriptions",   "Display room descriptions",                   TRUE }, /* 90 */
{ "room_names",          "Display room names",                          TRUE }, /* 91 */
{ "verify_mana",         "Verify critical mana",                        FALSE }, /* 92 */
{ "easy_autos",          "Automatically inscribe all objects",          FALSE }, /* 93 */
{ "easy_search",         "Start searching if not disturbed",            FALSE }, /* 94 */
{ "view_glowing_lite",   "Use special colours for glowing lite",        TRUE }, /* 95 */
{ "show_sidebar",        "Display stats in main window",                FALSE }, /* 96 */
{ "show_itemlist",       "Display all items on the bottom line",        FALSE }, /* 97 */
{ "depth_in_feet",       "Show dungeon level in feet",                  FALSE }, /* 98 */
{ "view_flavors",        "Show flavors in object graphics",             TRUE }, /* 99 */
{ "easy_corpses",        "Ignore corpses by default",                   TRUE }, /* 100 */
{ "view_unsafe_grids",  "Mark where you have detected traps",           TRUE }, /* 101 */
{ "view_detect_grids",  "Mark where you have detected monsters",        TRUE }, /* 102 */
{ "show_tips",           "Show tips as you explore the dungeon",        TRUE }, /* 103 */
{ "easy_more",           "Minimise '-more-' prompts",                   TRUE }, /* 104 */
{ "autosave_backup",     "Create backup savefile before descending",   TRUE }, /* 105 */
{ "auto_monlist",        "Always show visible monsters/objects",		FALSE }, /* 106 */
{ "easy_monlist",        "Spacebar toggles visible monsters/objects",   FALSE }, /* 107 */
{ "view_fogged_grids",   "Show fog of war for unexplored areas",        TRUE }, /* 108 */
{ "ally_messages",       "Show detailed combat messages for allies",    FALSE }, /* 109 */
{ NULL,                  NULL,                                          FALSE }, /* 110 */
{ NULL,                  NULL,                                          FALSE }, /* 111 */
{ NULL,                  NULL,                                          FALSE }, /* 112 */
{ NULL,                  NULL,                                          FALSE }, /* 113 */
{ NULL,                  NULL,                                          FALSE }, /* 114 */
{ NULL,                  NULL,                                          FALSE }, /* 115 */
{ NULL,                  NULL,                                          FALSE }, /* 116 */
{ NULL,                  NULL,                                          FALSE }, /* 117 */
{ NULL,                  NULL,                                          FALSE }, /* 118 */
{ NULL,                  NULL,                                          FALSE }, /* 119 */
{ NULL,                  NULL,                                          FALSE }, /* 120 */
{ NULL,                  NULL,                                          FALSE }, /* 121 */
{ NULL,                  NULL,                                          FALSE }, /* 122 */
{ NULL,                  NULL,                                          FALSE }, /* 123 */
{ NULL,                  NULL,                                          FALSE }, /* 124 */
{ NULL,                  NULL,                                          FALSE }, /* 125 */
{ NULL,                  NULL,                                          FALSE }, /* 126 */
{ NULL,                  NULL,                                          FALSE }, /* 127 */
{ NULL,                  NULL,                                          FALSE }, /* 128 */
{ "birth_randarts",      "Randomize all of the artifacts, even fixed",  FALSE }, /* 129 */
{ "birth_rand_stats",    "Randomize attributes chosen on level gain",   FALSE }, /* 130 */
{ "birth_ironman",       "Restrict the use of stairs/recall",           FALSE }, /* 131 */
{ "birth_no_stores",     "Restrict the use of stores/home",             FALSE }, /* 132 */
{ "birth_no_artifacts",  "Restrict creation of artifacts",              FALSE }, /* 133 */
{ "birth_no_stacking",   "Don't stack objects on the floor",            FALSE }, /* 134 */
{ "birth_no_identify",   "Don't need to identify items",                FALSE }, /* 135 */
{ "birth_no_stairs",     "Don't generate connected stairs",             FALSE }, /* 136 */
{ "birth_no_selling",    "Don't sell to the stores",                    FALSE }, /* 137 */
{ "birth_lore",          "Know complete artifact/ego/monster info",     FALSE }, /* 138 */
{ "birth_auto",          "Auto-inscribe items as if known",             FALSE }, /* 139 */
{ NULL,                  NULL,                                          FALSE }, /* 140 */
{ NULL,                  NULL,                                          FALSE }, /* 141 */
{ NULL,                  NULL,                                          FALSE }, /* 142 */
{ NULL,                  NULL,                                          FALSE }, /* 143 */
{ NULL,                  NULL,                                          FALSE }, /* 144 */
{ NULL,                  NULL,                                          FALSE }, /* 145 */
{ NULL,                  NULL,                                          FALSE }, /* 146 */
{ NULL,                  NULL,                                          FALSE }, /* 147 */
{ "birth_campaign",      "Play in Lord of the Rings campaign",          TRUE }, /* 148 */
{ "birth_haggle",        "Haggle in stores",                            FALSE }, /* 149 */
{ "birth_beginner",      "Start the game with almost no birth choices", FALSE }, /* 150 */
{ "birth_intermediate",  "Reduce the number of birth choices",          FALSE }, /* 151 */
{ "birth_first_time",    "Ask all birth setup question at start",       TRUE }, /* 152 */
{ "birth_reseed_artifacts", "Reseed random artifacts on death",         TRUE }, /* 153 */
{ "birth_evil",          "Good monsters attack you",                    FALSE }, /* 154 */
{ "birth_gollum",        "Play in gollum mode",                         FALSE }, /* 155 */
{ NULL,                  NULL,                                          FALSE }, /* 156 */
{ NULL,                  NULL,                                          FALSE }, /* 157 */
{ NULL,                  NULL,                                          FALSE }, /* 158 */
{ NULL,                  NULL,                                          FALSE }, /* 159 */
{ "cheat_peek",          "Cheat: Peek into object creation",            FALSE }, /* 160 */
{ "cheat_hear",          "Cheat: Peek into monster creation",           FALSE }, /* 161 */
{ "cheat_room",          "Cheat: Peek into dungeon creation",           FALSE }, /* 162 */
{ "cheat_xtra",          "Cheat: Peek into something else",             FALSE }, /* 163 */
{ "cheat_know",          "Cheat: Know complete monster info",           FALSE }, /* 164 */
{ "cheat_live",          "Cheat: Allow player to avoid death",          FALSE }, /* 165 */
{ NULL,                  NULL,                                          FALSE }, /* 166 */
{ NULL,                  NULL,                                          FALSE }, /* 167 */
{ NULL,                  NULL,                                          FALSE }, /* 168 */
{ NULL,                  NULL,                                          FALSE }, /* 169 */
{ NULL,                  NULL,                                          FALSE }, /* 170 */
{ NULL,                  NULL,                                          FALSE }, /* 171 */
{ NULL,                  NULL,                                          FALSE }, /* 172 */
{ NULL,                  NULL,                                          FALSE }, /* 173 */
{ NULL,                  NULL,                                          FALSE }, /* 174 */
{ NULL,                  NULL,                                          FALSE }, /* 175 */
{ NULL,                  NULL,                                          FALSE }, /* 176 */
{ NULL,                  NULL,                                          FALSE }, /* 177 */
{ NULL,                  NULL,                                          FALSE }, /* 178 */
{ NULL,                  NULL,                                          FALSE }, /* 179 */
{ NULL,                  NULL,                                          FALSE }, /* 180 */
{ NULL,                  NULL,                                          FALSE }, /* 181 */
{ "cheat_wall",          "Cheat: Show false colours for walls",         FALSE }, /* 181 */
{ NULL,                  NULL,                                          FALSE }, /* 183 */
{ NULL,                  NULL,                                          FALSE }, /* 184 */
{ NULL,                  NULL,                                          FALSE }, /* 185 */
{ NULL,                  NULL,                                          FALSE }, /* 186 */
{ NULL,                  NULL,                                          FALSE }, /* 187 */
{ NULL,                  NULL,                                          FALSE }, /* 188 */
{ NULL,                  NULL,                                          FALSE }, /* 189 */
{ NULL,                  NULL,                                          FALSE }, /* 190 */
{ NULL,                  NULL,                                          FALSE }, /* 191 */
{ NULL,                  NULL,                                          FALSE }, /* 192 */
{ "adult_randarts",      "Randomize all of the artifacts, even fixed",  FALSE }, /* 193 */
{ NULL,                  NULL,                                          FALSE }, /* 194 */
{ "adult_ironman",       "Restrict the use of stairs/recall",           FALSE }, /* 195 */
{ "adult_no_stores",     "Restrict the use of stores/home",             FALSE }, /* 196 */
{ "adult_no_artifacts",  "Restrict creation of artifacts",              FALSE }, /* 197 */
{ "adult_no_stacking",   "Don't stack objects on the floor",            FALSE }, /* 198 */
{ NULL,                  NULL,                                          FALSE }, /* 199 */
{ "adult_no_stairs",     "Don't generate connected stairs",             FALSE }, /* 200 */
{ NULL,                  NULL,                                          FALSE }, /* 201 */
{ NULL,                  NULL,                                          FALSE }, /* 202 */
{ NULL,                  NULL,                                          FALSE }, /* 203 */
{ NULL,                  NULL,                                          FALSE }, /* 204 */
{ NULL,                  NULL,                                          FALSE }, /* 205 */
{ NULL,                  NULL,                                          FALSE }, /* 206 */
{ NULL,                  NULL,                                          FALSE }, /* 207 */
{ NULL,                  NULL,                                          FALSE }, /* 208 */
{ NULL,                  NULL,                                          FALSE }, /* 209 */
{ NULL,                  NULL,                                          FALSE }, /* 210 */
{ NULL,                  NULL,                                          FALSE }, /* 211 */
{ "adult_campaign",      "Play in Lord of the Rings campaign",          TRUE }, /* 212 */
{ "adult_haggle",        "Haggle in stores",                            FALSE }, /* 213 */
{ "adult_beginner",      "Start the game with almost no birth choices", FALSE }, /* 214 */
{ "adult_intermediate",  "Reduce the number of birth choices",          FALSE }, /* 215 */
{ "adult_first_time",    "Ask all birth setup question at start",       TRUE }, /* 216 */
{ "adult_reseed_artifacts", "Reseed random artifacts on death",         TRUE }, /* 217 */
{ NULL,                  NULL,                                          FALSE }, /* 218 */
{ NULL,                  NULL,                                          FALSE }, /* 219 */
{ NULL,                  NULL,                                          FALSE }, /* 220 */
{ NULL,                  NULL,                                          FALSE }, /* 221 */
{ NULL,                  NULL,                                          FALSE }, /* 222 */
{ NULL,                  NULL,                                          FALSE }, /* 223 */
{ "score_peek",          "Score: Peek into object creation",            FALSE }, /* 224 */
{ "score_hear",          "Score: Peek into monster creation",           FALSE }, /* 225 */
{ "score_room",          "Score: Peek into dungeon creation",           FALSE }, /* 226 */
{ "score_xtra",          "Score: Peek into something else",             FALSE }, /* 227 */
{ "score_know",          "Score: Know complete monster info",           FALSE }, /* 228 */
{ "score_live",          "Score: Allow player to avoid death",          FALSE }, /* 229 */
{ NULL,                  NULL,                                          FALSE }, /* 230 */
{ NULL,                  NULL,                                          FALSE }, /* 231 */
{ NULL,                  NULL,                                          FALSE }, /* 232 */
{ NULL,                  NULL,                                          FALSE }, /* 233 */
{ NULL,                  NULL,                                          FALSE }, /* 234 */
{ NULL,                  NULL,                                          FALSE }, /* 235 */
{ NULL,                  NULL,                                          FALSE }, /* 236 */
{ NULL,                  NULL,                                          FALSE }, /* 237 */
{ NULL,                  NULL,                                          FALSE }, /* 238 */
{ NULL,                  NULL,                                          FALSE }, /* 239 */
{ NULL,                  NULL,                                          FALSE }, /* 240 */
{ NULL,                  NULL,                                          FALSE }, /* 241 */
{ NULL,                  NULL,                                          FALSE }, /* 242 */
{ NULL,                  NULL,                                          FALSE }, /* 243 */
{ "score_lore",          "Score: Know complete artifact/ego info",      FALSE }, /* 244 */
{ "score_auto",          "Score: Auto-inscribe items as if known",      FALSE }, /* 245 */
{ "score_wall",          "Score: Show false colours for walls",         FALSE }, /* 245 */
{ NULL,                  NULL,                                          FALSE }, /* 247 */
{ NULL,                  NULL,                                          FALSE }, /* 248 */
{ NULL,                  NULL,                                          FALSE }, /* 249 */
{ NULL,                  NULL,                                          FALSE }, /* 250 */
{ NULL,                  NULL,                                          FALSE }, /* 251 */
{ NULL,                  NULL,                                          FALSE }, /* 252 */
{ NULL,                  NULL,                                          FALSE }, /* 253 */
{ NULL,                  NULL,                                          FALSE }, /* 254 */
{ NULL,                  NULL,                                          FALSE }, /* 255 */
};


/* Accessor functions */
const char *option_name(int opt)
{
	if (opt >= OPT_MAX) return NULL;
	return options[opt].name;
}

const char *option_desc(int opt)
{
	if (opt >= OPT_MAX) return NULL;
	return options[opt].description;
}

/* Setup functions */
void option_set(int opt, bool on)
{
	op_ptr->opt[opt] = on;
}

void option_set_defaults(void)
{
	size_t opt;

	for (opt = 0; opt < OPT_MAX; opt++)
		op_ptr->opt[opt] = options[opt].normal;
}
