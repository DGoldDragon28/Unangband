/* File: externs.h */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.6
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */


/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */


/*
 * Automatically generated "variable" declarations
 */

/* tables.c */
extern const s16b ddd[9];
extern const s16b ddx[10];
extern const s16b ddy[10];
extern const s16b ddx_ddd[9];
extern const s16b ddy_ddd[9];
extern const byte side_dirs[20][8];
extern const char hexsym[16];
extern const byte stat_gains[PY_MAX_LEVEL];
extern const byte adj_mag_study[];
extern const byte adj_mag_study_max[];
extern const s16b adj_mag_mana[];
extern const byte adj_mag_fail_min[];
extern const byte adj_mag_fail_rate[];
extern const byte adj_chr_gold[];
extern const byte adj_chr_stock[];
extern const byte adj_chr_taunt[];
extern const byte adj_chr_fear[];
extern const byte adj_int_break[];
extern const byte adj_int_dev[];
extern const byte adj_wis_sav[];
extern const byte adj_dex_dis[];
extern const byte adj_int_dis[];
extern const byte adj_agi_ta[];
extern const byte adj_wis_ta[];
extern const byte adj_str_td[];
extern const byte adj_dex_th[];
extern const byte adj_int_th[];
extern const byte adj_str_wgt[];
extern const byte adj_str_hold[];
extern const byte adj_str_dig[];
extern const byte adj_str_blow[];
extern const byte adj_dex_blow[];
extern const byte adj_agi_safe[];
extern const byte adj_agi_speed[];
extern const byte adj_con_fix[];
extern const byte adj_con_die[];
extern const byte adj_con_reserve[];
extern const byte adj_siz_die[];
extern const byte adj_charge_siz[];
extern const byte blows_table[12][12];
extern const byte extract_energy[200];
extern const s32b player_exp[PY_MAX_LEVEL];
extern const player_sex sex_info[MAX_SEXES];
extern const cptr stat_names[A_MAX];
extern const cptr stat_names_reduced[A_MAX];
extern const cptr stat_names_reduced_short[A_MAX];
extern const cptr window_flag_desc[32];
extern const cptr inscrip_text[MAX_INSCRIP];
extern const u32b object_xtra_base[OBJECT_XTRA_MAX_HIDDEN];
extern const int object_xtra_what[OBJECT_XTRA_MAX_HIDDEN];
extern const int object_xtra_size[OBJECT_XTRA_MAX_HIDDEN];
extern const object_grouper object_group[];
extern const cptr magic_name[4][32];
extern const cptr disease_name[33];
extern byte spell_info_RF4[32][5];
extern const byte spell_info_RF5[32][5];
extern const byte spell_info_RF6[32][5];
extern const byte spell_info_RF7[32][5];
extern byte spell_desire_RF4[32][8];
extern const byte spell_desire_RF5[32][8];
extern const byte spell_desire_RF6[32][8];
extern const byte spell_desire_RF7[32][8];
extern const u32b spell_smart_RF4[32];
extern const u32b spell_smart_RF5[32];
extern const u32b spell_smart_RF6[32];
extern const u32b spell_smart_RF7[32];
extern const element_type element[MAX_ELEMENTS];
extern const start_item common_items[MAX_COMMON_ITEMS];
extern const cptr vocalize[MAX_LANGUAGES];
extern const int month_day[9];
extern const cptr month_name[9];
extern const s16b bag_holds[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL][2];
extern const cptr w_name_style[32];
extern const s16b style2tval[32];
extern const s16b parasite_hack[DISEASE_BLOWS];
extern const cptr cause_of_death[-SOURCE_PLAYER_END][SOURCE_MESSAGES];
extern const do_cmd_item_type cmd_item_list[MAX_COMMANDS];
extern const familiar_type familiar_race[MAX_FAMILIARS];
extern const familiar_ability_type familiar_ability[MAX_FAMILIAR_ABILITIES];
extern const timed_effect timed_effects[TMD_MAX];
extern const skill_table_entry skill_table[];
extern const char *inscrip_info[];

/* variable.c */
extern cptr copyright;
extern byte version_major;
extern byte version_minor;
extern byte version_patch;
extern byte version_extra;
extern byte sf_major;
extern byte sf_minor;
extern byte sf_patch;
extern byte sf_extra;
extern u32b sf_xtra;
extern u32b sf_when;
extern u16b sf_lives;
extern u16b sf_saves;
extern bool arg_fiddle;
extern bool arg_wizard;
extern bool arg_sound;
extern bool arg_mouse;
extern bool arg_trackmouse;
extern bool arg_graphics;
extern bool arg_graphics_nice;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool character_quickstart;
extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern s16b character_icky;
extern s16b character_xtra;
extern u32b seed_randart;
extern u32b seed_flavor;
extern u32b seed_town;
extern u32b seed_dungeon;
extern u32b seed_last_dungeon;
extern s16b num_repro;
extern s16b object_level;
extern s16b monster_level;
extern s16b summoner;
extern bool	summon_strict;
extern char summon_char_type;
extern byte summon_attr_type;
extern byte summon_group_type;
extern u32b summon_flag_type;
extern s16b summon_race_type;
extern char summon_word_type[80];
extern s32b turn;
extern s32b old_turn;
extern s32b player_turn;
extern s32b resting_turn;
extern bool use_mouse;
extern bool use_trackmouse;
extern bool use_graphics;
extern bool use_graphics_nice;
extern bool use_bigtile;
extern bool use_dbltile;
extern bool use_trptile;
extern s16b signal_count;
extern bool msg_flag;
extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;
extern s16b coin_type;
extern s16b food_type;
extern s16b race_drop_idx;
extern s16b tval_drop_idx;
extern bool opening_chest;
extern bool shimmer_monsters;
extern bool shimmer_objects;
extern bool repair_mflag_show;
extern bool repair_mflag_mark;
extern s16b o_max;
extern s16b o_cnt;
extern s16b m_max;
extern s16b m_cnt;
extern s16b q_max;
extern s16b q_cnt;
extern object_type term_object;
extern bool term_obj_real;
extern byte feeling;
extern s16b rating;
extern u32b level_flag;
extern bool good_item_flag;
extern bool closing_flag;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char savefile[1024];
extern s16b macro__num;
extern char **macro__pat;
extern char **macro__act;
extern term *angband_term[ANGBAND_TERM_MAX];
extern char angband_term_name[ANGBAND_TERM_MAX][16];
extern byte angband_color_table[256][4];
extern color_type color_table[256];
const sound_name_type angband_sound_name[MSG_MAX];
extern sint view_n;
extern u16b *view_g;
extern sint fire_n;
extern u16b *fire_g;
extern sint temp_n;
extern u16b *temp_g;
extern byte *temp_y;
extern byte *temp_x;
extern sint dyna_n;
extern u16b *dyna_g;
extern byte dyna_cent_y;
extern byte dyna_cent_x;
extern bool dyna_full;
extern byte (*cave_info)[256];
extern byte (*play_info)[256];
extern s16b (*cave_feat)[DUNGEON_WID];
extern s16b (*cave_o_idx)[DUNGEON_WID];
extern s16b (*cave_m_idx)[DUNGEON_WID];
extern s16b (*cave_region_piece)[DUNGEON_WID];
extern region_piece_type *region_piece_list;
extern int region_piece_max;
extern int region_piece_cnt;
extern region_type *region_list;
extern int region_max;
extern int region_cnt;
extern void (*modify_grid_adjacent_hook)(byte *a, char *c, int y, int x, byte adj_char[16]);
extern void (*modify_grid_boring_hook)(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);
extern void (*modify_grid_unseen_hook)(byte *a, char *c);
extern void (*modify_grid_interesting_hook)(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);
extern byte (*cave_cost)[DUNGEON_WID];
extern byte (*cave_when)[DUNGEON_WID];
extern maxima *z_info;
extern int scent_when;
extern int flow_center_y;
extern int flow_center_x;
extern int update_center_y;
extern int update_center_x;
extern int cost_at_center;
extern room_info_type room_info[DUN_ROOMS];
extern byte dun_room[MAX_ROOMS_ROW][MAX_ROOMS_COL];
extern object_type *o_list;
extern monster_type *m_list;
extern monster_lore *l_list;
extern object_info *a_list;
extern object_lore *e_list;
extern object_info *x_list;
extern quest_type *q_list;
extern s16b total_store_count;
extern s16b max_store_count;
extern store_type_ptr *store;
extern object_type *inventory;
extern s16b bag_contents[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL];
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_ego_size;
extern alloc_entry *alloc_ego_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern s16b alloc_feat_size;
extern alloc_entry *alloc_feat_table;
extern byte tval_to_attr[128];
extern char macro_buffer[1024];
extern char *keymap_act[KEYMAP_MODES][256];
extern const player_sex *sp_ptr;
extern const player_race *rp_ptr;
extern const player_class *cp_ptr;
extern player_other *op_ptr;
extern player_type *p_ptr;
extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern method_type *method_info;
extern char *method_name;
extern char *method_text;
extern effect_type *effect_info;
extern char *effect_name;
extern char *effect_text;
extern region_info_type *region_info;
extern char *region_name;
extern char *region_text;
extern feature_type *f_info;
extern char *f_name;
extern char *f_text;
extern desc_type *d_info;
extern char *d_name;
extern char *d_text;
extern object_kind *k_info;
extern char *k_name;
extern char *k_text;
extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern ego_item_type *e_info;
extern char *e_name;
extern char *e_text;
extern flavor_type *x_info;
extern char *x_name;
extern char *x_text;
extern monster_race *r_info;
extern char *r_name;
extern char *r_text;
extern player_race *p_info;
extern char *p_name;
extern char *p_text;
extern player_class *c_info;
extern char *c_name;
extern char *c_text;
extern spell_type *s_info;
extern char *s_name;
extern char *s_text;
extern weapon_style *w_info;
extern rune_type *y_info;
extern char *y_name;
extern char *y_text;
extern town_type *t_info;
extern char *t_name;
extern char *t_text;
extern store_type *u_info;
extern char *u_name;
extern char *u_text;
extern hist_type *h_info;
extern char *h_text;
extern owner_type *b_info;
extern char *b_name;
extern char *b_text;
extern byte *g_info;
extern char *g_name;
extern char *g_text;
extern quest_type *q_info;
extern char *q_name;
extern char *q_text;
extern names_type *n_info;
extern s16b tips[TIPS_MAX];
extern s16b tips_start;
extern s16b tips_end;
extern const char *ANGBAND_SYS;
extern const char *ANGBAND_GRAF;
extern char *ANGBAND_DIR;
extern char *ANGBAND_DIR_APEX;
extern char *ANGBAND_DIR_BONE;
extern char *ANGBAND_DIR_DATA;
extern char *ANGBAND_DIR_EDIT;
extern char *ANGBAND_DIR_FILE;
extern char *ANGBAND_DIR_HELP;
extern char *ANGBAND_DIR_INFO;
extern char *ANGBAND_DIR_SAVE;
extern char *ANGBAND_DIR_PREF;
extern char *ANGBAND_DIR_USER;
extern char *ANGBAND_DIR_XTRA;
extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(const object_type*);
extern bool (*teleport_hook)(const int oy, const int ox, const int ny, const int nx);
extern bool (*ang_sort_comp)(vptr u, vptr v, int a, int b);
extern void (*ang_sort_swap)(vptr u, vptr v, int a, int b);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern bool (*get_feat_num_hook)(int k_idx);
extern FILE *text_out_file;
extern void (*text_out_hook)(byte a, cptr str);
extern int text_out_wrap;
extern int text_out_indent;
extern int text_out_lines;
extern int highscore_fd;
extern bool use_transparency;
extern s32b magic_slay_power[32];
extern long tot_mon_power;
extern s32b *slays;
extern char pf_result[MAX_PF_LENGTH];
extern int pf_result_index;
extern quiver_group_type quiver_group[MAX_QUIVER_GROUPS];
extern s16b bag_kinds_cache[SV_BAG_MAX_BAGS][INVEN_BAG_TOTAL];
extern ecology_type cave_ecology;
extern u32b hack_monster_equip;
extern int target_path_n;
extern u16b target_path_g[512];
extern s16b target_path_d[512];


/*
 * Automatically generated "function declarations"
 */

/* birth.c */
extern void player_birth(void);

/* cave.c */
extern sint distance(int y1, int x1, int y2, int x2);
extern bool generic_los(int y1, int x1, int y2, int x2, byte flg);
extern bool no_lite(void);
extern bool cave_valid_bold(int y, int x);
extern bool feat_supports_lighting(byte feat);
extern byte get_color(byte a, int attr, int n);
extern void modify_grid_adjacent_view(byte *a, char *c, int y, int x, byte adj_char[16]);
extern void modify_grid_boring_view(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);
extern void modify_grid_unseen_view(byte *a, char *c);
extern void modify_grid_interesting_view(byte *a, char *c, int y, int x, byte cinfo, byte pinfo);
extern void map_info(int y, int x, byte *ap, char *cp, byte *tap, char *tcp);
extern void move_cursor_relative(int y, int x);
extern void big_putch(int x, int y, byte a, char c);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int y, int x);
extern void lite_spot(int y, int x);
extern void prt_map(void);
extern void prt_item_list(void);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);
extern errr vinfo_init(void);
extern void forget_view(void);
extern void update_view(void);
extern void update_dyna(void);
extern void update_noise(void);
extern void update_smell(void);
extern bool map_home(int m_idx);
extern void map_area(void);
extern void wiz_lite(void);
extern void wiz_dark(void);
extern void town_illuminate(bool daytime);
extern void check_attribute_lost(const int y, const int x, const int r, const byte los, tester_attribute_func require_attribute, tester_attribute_func has_attribute,
	tester_attribute_func redraw_attribute, modify_attribute_func_remove remove_attribute,	modify_attribute_func_reapply reapply_attribute);
extern void gain_attribute(int y, int x, int r, byte los, modify_attribute_func apply_attribute, tester_attribute_func redraw_attribute);
extern bool require_halo(int y, int x);
extern bool has_halo(int y, int x);
extern bool redraw_halo_loss(int y, int x);
extern bool redraw_halo_gain(int y, int x);
extern void apply_halo(int y, int x);
extern int remove_halo(int y, int x);
extern void reapply_halo(int y, int x, int r);
extern void apply_climb(int y, int x);
extern void set_level_flags(int feat);
extern void cave_set_feat_aux(const int y, const int x, int feat);
extern void cave_set_feat(const int y, const int x, int feat);
extern int feat_state(int feat, int action);
extern void cave_alter_feat(const int y, const int x, int action);
extern int  project_path(u16b *gp, int range, int y1, int x1, int *y2, int *x2, u32b flg);
extern byte projectable(int y1, int x1, int y2, int x2, u32b flg);
extern void scatter(int *yp, int *xp, int y, int x, int d, int m);
extern void health_track(int m_idx);
extern void monster_race_track(int r_idx);
extern void artifact_track(int a_idx);
extern void object_kind_track(int k_idx);
extern void object_actual_track(const object_type *j_ptr);
extern void disturb(int stop_search, int wake_up);
extern bool is_quest(int level);
extern void init_level_flags(void);

/* cmd1.c */
extern bool test_hit_fire(int chance, int ac, int vis);
extern bool test_hit_norm(int chance, int ac, int vis);
extern sint critical_shot(int weight, int plus, int dam);
extern sint critical_norm(int weight, int plus, int dam);
extern sint object_damage_multiplier(object_type *o_ptr, const monster_type *m_ptr, bool floor);
extern sint tot_dam_mult(int tdam, int mult);
extern void find_secret(int y, int x);
extern void search(void);
extern bool auto_pickup_ignore(const object_type *o_ptr);
extern byte py_pickup(int py, int px, int pickup);
extern bool avoid_trap(int y, int x, int feat);
extern bool discharge_trap(int y, int x, int ty, int tx, s16b child_region);
extern void hit_trap(int y, int x);
extern void mon_style_benefits(const monster_type *m_ptr, u32b style, int *to_hit, int *to_dam, int *to_crit);
extern bool auto_activate(const object_type *o_ptr);
extern void py_attack(int dir);
extern bool stuck_player(int *dir);
extern void move_player(int dir);
extern void run_step(int dir);

/* cmd2.c */
extern void do_cmd_go_up(void);
extern void do_cmd_go_down(void);
extern void do_cmd_search_or_steal(void);
extern void do_cmd_toggle_search(void);
extern void do_cmd_open(void);
extern void do_cmd_close(void);
extern void do_cmd_tunnel(void);
extern void do_cmd_disarm(void);
extern void do_cmd_bash(void);
extern void do_cmd_alter(void);
extern bool player_set_trap_or_spike(int item);
extern void do_cmd_walk(void);
extern void do_cmd_pathfind(int y, int x);
extern void do_cmd_run(void);
extern void do_cmd_hold(void);
extern void do_cmd_pickup(void);
extern void do_cmd_rest(void);
extern int breakage_chance(object_type *o_ptr);
extern int fire_or_throw_path(u16b *gp, int range, int y0, int x0, int *ty, int *tx, int inaccuracy);
extern bool item_tester_hook_throwable(const object_type *o_ptr);
extern bool item_tester_hook_slingable(const object_type *o_ptr);
extern bool player_fire(int item);
extern bool player_throw(int item);
extern int set_routes(s16b *routes, int max_num, int from);
extern int actual_route(int dun);

/* cmd3.c */
extern void mark_cursed_feeling(object_type *o_ptr);
extern bool item_tester_hook_wear(const object_type *o_ptr);
extern bool item_tester_hook_droppable(const object_type *o_ptr);
extern bool item_tester_hook_removable(const object_type *o_ptr);
extern bool item_tester_hook_tradeable(const object_type *o_ptr);
extern bool item_tester_empty_flask_or_lite(const object_type *o_ptr);
extern bool item_tester_refill_torch(const object_type *o_ptr);
extern bool item_tester_refill_lantern(const object_type *o_ptr);
extern bool item_tester_refill_flask(const object_type *o_ptr);
extern bool item_tester_refill_firearm(const object_type *o_ptr);
extern bool item_tester_light_source(const object_type *o_ptr);
extern int cmd_tester_fill_or_fuel(int item);
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern bool player_wield(int item);
extern bool player_takeoff(int item);
extern bool player_drop(int item);
extern bool player_steal(int item);
extern bool player_offer(int item);
extern bool player_trade(int item2);
extern bool player_destroy(int item);
extern bool player_observe(int item);
extern bool player_uninscribe(int item);
extern bool player_inscribe(int item);
extern bool player_refill(int item);
extern bool player_refill2(int item);
extern bool player_light_and_douse(int item);
extern void do_cmd_target(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_query_symbol(void);
extern bool ang_sort_comp_hook(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_hook(vptr u, vptr v, int a, int b);
extern void do_cmd_monlist(void);
extern void do_cmd_center_map(void);

/* cmd4.c */
extern void do_cmd_redraw(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void do_cmd_menu(int menuID, const char *title);
extern void do_cmd_pref(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern bool print_event(quest_event *event, int pronoun, int tense, cptr prefix);
extern bool print_quests(int min_stage, int max_stage);
extern void do_cmd_quest(void);
extern void do_cmd_feeling(void);
extern void do_cmd_timeofday(void);
extern void do_cmd_room_desc(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen(void);
extern void do_cmd_save_screen_html(void);
extern void game_statistics(void);
extern void do_cmd_game_statistics(void);
extern void strip_name(char *buf, int buf_s, int oid);
extern const cptr feature_group_text[];
extern int feat_order(int feat);
extern int count_routes(int from, int to);
extern void do_knowledge_dungeons(void);

/* cmd5.c */
extern int get_spell(int *sn, cptr prompt, object_type *o_ptr, bool known);
extern bool inven_book_okay(const object_type *o_ptr);
extern bool inven_study_okay(const object_type *o_ptr);
extern bool player_browse_object(object_type *o_ptr);
extern bool player_browse(int item);
extern bool player_study(int item);
extern bool player_cast_spell(int spell, int plev, cptr p, cptr t);
extern bool inven_cast_okay(const object_type *o_ptr);
extern bool player_cast(int item);

/* cmd6.c */
extern int find_item_to_reduce(int item);
extern bool do_cmd_item(int item);
extern bool item_tester_hook_food_edible(const object_type *o_ptr);
extern bool item_tester_hook_rod_charged(const object_type *o_ptr);
extern bool item_tester_hook_assembly(const object_type *o_ptr);
extern bool item_tester_hook_assemble(const object_type *o_ptr);
extern bool item_tester_hook_activate(const object_type *o_ptr);
extern bool item_tester_hook_apply(const object_type *o_ptr);
extern bool item_tester_hook_coating(const object_type *o_ptr);
extern int cmd_tester_rune_or_coating(int item);
extern bool player_eat_food(int item);
extern bool player_quaff_potion(int item);
extern bool player_read_scroll(int item);
extern bool player_use_staff(int item);
extern bool player_aim_wand(int item);
extern bool player_zap_rod(int item);
extern bool player_assembly(int item);
extern bool player_assemble(int item);
extern bool player_activate(int item);
extern bool player_apply_rune_or_coating(int item);
extern bool player_apply_rune_or_coating2(int item);
extern bool player_handle(int item);

/* dungeon.c */
extern void ensure_quest(void);
extern bool dun_level_mon(int r_idx);
extern void suffer_disease(bool allow_cure);
extern void play_game(bool new_game);
extern int value_check_aux1(const object_type *o_ptr);
extern int value_check_aux2(const object_type *o_ptr);

/* files.c */
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern s16b tokenize(char *buf, s16b num, char **tokens);
extern errr process_pref_file_command(char *buf);
extern errr process_pref_file(cptr name);
extern void dump_html(void);
extern void dump_startup_prefs(void);
extern errr check_time(void);
extern errr check_time_init(void);
extern errr check_load(void);
extern errr check_load_init(void);
extern void display_player_xtra_info(bool hide_extra);
extern void player_flags(u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern cptr likert(int x, int y, byte *attr);
extern void display_player_stat_info(int row, int col, int min, int max, int attr);
extern void display_player(int mode);
extern errr file_character(cptr name, bool full);
extern bool show_file(cptr name, cptr what, int line, int mode);
extern bool queue_tip(cptr tip);
extern void show_tip(void);
extern void do_cmd_help(void);
extern void do_cmd_quick_help(void);
extern void process_player_name(bool sf);
extern char *make_word(int min_len, int max_len);
extern void get_name(void);
extern void do_cmd_suicide(void);
extern void do_cmd_save_game(void);
extern void do_cmd_save_bkp(void);
extern long total_points(void);
extern void display_scores(int from, int to);
extern errr predict_score(void);
extern void close_game(void);
extern void exit_game_panic(void);
#ifdef HANDLE_SIGNALS
extern void (*(*signal_aux)(int, void (*)(int)))(int);
#endif
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
extern void display_scores_aux(int from, int to, int note, high_score *score);

/* generate.c */
extern void generate_cave(void);

/* info.c */
extern bool spell_desc(spell_type *s_ptr, const cptr intro, int level, bool detail, int target);
extern void spell_info(char *p, int p_s, int spell, int level);
extern void list_ego_item_runes(int ego, bool spoil);
extern bool list_object_flags(u32b f1, u32b f2, u32b f3, u32b f4, int pval, int mode);
extern void list_object(const object_type *o_ptr, int mode);
extern void screen_object(object_type *o_ptr);
extern void screen_self_object(object_type *o_ptr, int slot);
extern void describe_shape(int shape, bool random);
extern void print_powers(const s16b *book, int num, int y, int x, int level);
extern void print_spells(const s16b *book, int num, int y, int x);
extern bool make_fake_artifact(object_type *o_ptr, byte name1);
extern void display_koff(const object_type *o_ptr);
extern void object_obvious_flags(object_type *o_ptr, bool floor);
extern void object_can_flags(object_type *o_ptr, u32b f1, u32b f2, u32b f3, u32b f4, bool floor);
extern void object_not_flags(object_type *o_ptr, u32b f1, u32b f2, u32b f3, u32b f4, bool floor);
extern void object_may_flags(object_type *o_ptr, u32b f1, u32b f2, u32b f3, u32b f4, bool floor);
extern void drop_may_flags(object_type *o_ptr);
extern void drop_all_flags(object_type *o_ptr);
extern void object_usage(int slot);
extern void object_guess_name(object_type *o_ptr);
extern void update_slot_flags(int slot, u32b f1, u32b f2, u32b f3, u32b f4);
extern void equip_can_flags(u32b f1,u32b f2,u32b f3, u32b f4);
extern void equip_not_flags(u32b f1,u32b f2,u32b f3, u32b f4);
extern void inven_drop_flags(object_type *o_ptr);
extern int bow_multiplier(int sval);
extern s32b slay_power(u32b s_index);
extern u32b slay_index(const u32b f1, const u32b f2, const u32b f3, const u32b f4);
extern s32b object_power(const object_type *o_ptr);
extern void describe_blow(int method, int effect, int level, int feat, const char *intro, const char *damage, u16b details, int num);
extern void describe_feature(int f_idx);
extern void feature_roff_top(int f_idx);
extern void screen_feature_roff(int f_idx);
extern void display_feature_roff(int f_idx);
extern void describe_region_basic(region_type *r_ptr, const char *intro);
extern void describe_region_attacks(region_type *r_ptr, const char *intro, const char *damage, const char *lasts);
extern void describe_region(region_type *r_ptr);
extern void region_top(const region_type *r_ptr);
extern void screen_region(region_type *r_ptr);
extern void display_region(region_type *r_ptr);


/* init2.c */
extern void init_file_paths(char *path);
extern void create_user_dirs(void);
extern void init_angband(void);
extern void cleanup_angband(void);
void ang_atexit(void (*arg)(void));


/* load.c */
extern bool load_player(void);

/* melee1.c */
extern bool monster_scale(monster_race *n_ptr, int m_idx, int depth);
extern int get_breath_dam(s16b hp, int method, bool powerful);
extern bool mon_check_hit(int m_idx, int power, int level, int who, bool ranged);
extern int attack_desc(char *buf, int target, int method, int effect, int damage, u16b flg, int buf_size);
extern bool make_attack_normal(int m_idx, bool harmless);
extern int get_dam(byte power, int attack, bool varies);
extern void mon_blow_ranged(int who, int what, int x, int y, int method, int range, int flg);
extern int sauron_shape(int old_form);
extern bool make_attack_ranged(int who, int attack, int py, int px);
extern bool mon_evade(int m_idx, int chance, int out_of, cptr r);
extern bool mon_resist_object(int m_idx, const object_type *o_ptr);
extern bool race_avoid_trap(int r_idx, int y, int x, int feat);
extern bool mon_avoid_trap(monster_type *m_ptr, int y, int x, int feat);
extern void mon_hit_trap(int m_idx, int y, int x);

/* melee2.c */
extern bool push_aside(int fy, int fx, monster_type *n_ptr);
extern int monster_language(int r_idx);
extern bool player_understands(int language);
extern void monster_speech(int m_idx, cptr saying, bool understand);
extern bool tell_allies_player_can(int y, int x, u32b flag);
extern bool tell_allies_player_not(int y, int x, u32b flag);
extern bool tell_allies_mflag(int y, int x, u32b flag, cptr saying);
extern bool tell_allies_not_mflag(int y, int x, u32b flag, cptr saying);
extern bool tell_allies_death(int y, int x, cptr saying);
extern bool tell_allies_range(int y, int x, int best_range, int min_range, cptr saying);
extern bool tell_allies_summoned(int y, int x, int summoned, cptr saying);
extern bool tell_allies_target(int y, int x, int ty, int tx, bool scent, cptr saying);
extern void feed_monster(int m_idx);
extern void find_range(int m_idx);
extern bool choose_to_attack_player(const monster_type *m_ptr);
extern void process_monsters(byte minimum_energy);
extern int get_scent(int y, int x);
extern bool cave_exist_mon(int m_idx, int y, int x, bool occupied_ok);
extern void reset_monsters(void);

/* monster1.c */
extern void describe_monster_race(const monster_race *r_ptr, const monster_lore *l_ptr, bool spoilers);
extern void describe_monster_look(int m_idx, bool spoilers);
extern void screen_roff(const int r_idx, const monster_lore *l_ptr);
extern void display_roff(const int r_idx, const monster_lore *l_ptr);
extern void screen_monster_look(const int m_idx);
extern void display_monster_look(const int m_idx);
extern void get_closest_monster(int n, int y0, int x0, int *ty, int *tx, byte parameter, int who);

/* monster2.c */
extern s16b poly_r_idx(int y, int x, int r_idx, bool require_char, bool harmless, bool chaotic);
extern void delete_monster_idx(int i);
extern void delete_monster(int y, int x);
extern void delete_monster_lite(int i);
extern void compact_monsters(int size);
extern void wipe_m_list(void);
extern s16b m_pop(void);
extern errr get_mon_num_prep(void);
extern s16b get_mon_num(int level);
extern void display_monlist(int row, unsigned int width, int mode, bool command, bool force);
extern void race_desc(char *desc, size_t max, int r_idx, int mode, int number);
extern void monster_desc(char *desc, size_t max, int m_idx, int mode);
extern void lore_do_probe(int r_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern bool redraw_torch_lit_gain(int y, int x);
extern void apply_torch_lit(int y, int x);
extern void update_mon(int m_idx, bool full);
extern void update_monsters(bool full);
extern s16b monster_carry(int m_idx, object_type *j_ptr);
extern void monster_swap(int y1, int x1, int y2, int x2);
extern s16b player_place(int y, int x, bool escort_allowed);
extern void monster_hide(int y, int x, int mmove, monster_type *n_ptr);
extern s16b monster_place(int y, int x, monster_type *n_ptr);
extern int mon_resist_effect(int effect, int r_idx);
extern bool mon_resist_feat(int feat, int r_idx);
extern int calc_monster_ac(int m_idx, bool ranged);
extern int calc_monster_hp(int m_idx);
extern byte calc_monster_speed(int m_idx);
extern void set_monster_haste(s16b m_idx, s16b counter, bool message);
extern void set_monster_slow(s16b m_idx, s16b counter, bool message);
extern int find_monster_ammo(int m_idx, int blow, bool created);
extern int place_monster_here(int y, int x, int r_idx);
extern bool place_monster_aux(int y, int x, int r_idx, bool slp, bool grp, u32b flg);
extern bool place_monster(int y, int x, bool slp, bool grp);
extern bool alloc_monster(int dis, bool slp);
extern void summon_specific_params(int r_idx, int summon_specific_type, bool hack_ecology);
extern bool summon_specific(int y1, int x1, int restrict_race, int lev, int type, bool grp, u32b flg);
extern bool summon_specific_one(int y1, int x1, int r_idx, bool slp, u32b flg);
extern bool animate_object(int item);
extern void set_monster_fear(monster_type *m_ptr, int v, bool panic);
extern bool multiply_monster(int m_idx);
extern void message_pain(int m_idx, int dam);
extern int deeper_monster(int r_idx, int r);
extern void get_monster_ecology(int r_idx, int hack_pit);
extern bool place_guardian(int y0, int x0, int y1, int x1);


/* object1.c */
extern void flavor_init(void);
extern void reset_visuals(bool unused);
extern void object_flags(const object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern void object_flags_known(const object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern void object_desc(char *buf, size_t max, const object_type *o_ptr, int pref, int mode);
extern void object_desc_spoil(char *buf, size_t max, const object_type *o_ptr, int pref, int mode);
extern void identify_random_gen(const object_type *o_ptr);
extern char index_to_label(int i);
extern s16b label_to_inven(int c);
extern s16b label_to_equip(int c);
extern s16b wield_slot(const object_type *o_ptr);
extern cptr mention_use(int i);
extern cptr describe_use(int i);
extern bool item_tester_okay(const object_type *o_ptr);
extern sint scan_floor(int *items, int size, int y, int x, int mode);
extern sint scan_feat(int y, int x);
extern void display_inven(void);
extern void display_equip(void);
extern void show_inven(void);
extern void show_equip(void);
extern void toggle_inven_equip(void);
extern bool get_item(int *cp, cptr pmt, cptr str, int mode);
extern void fake_bag_item(object_type *o_ptr, int sval, int slot);
extern bool get_item_from_bag(int *cp, cptr pmt, cptr str, object_type *o_ptr);

/* object2.c */
extern void excise_object_idx(int o_idx);
extern void delete_object_idx(int o_idx);
extern void delete_object(int y, int x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern s16b o_pop(void);
extern errr get_obj_num_prep(void);
extern s16b get_obj_num(int level);
extern void object_known_store(object_type *o_ptr);
extern void object_known(object_type *o_ptr);
extern void object_bonus(object_type *o_ptr, bool floor);
extern void object_gauge(object_type *o_ptr, bool floor);
extern void object_mental(object_type *o_ptr, bool floor);
extern void object_aware_tips(object_type *o_ptr, bool seen);
extern void object_aware(object_type *o_ptr, bool floor);
extern void object_tried(object_type *o_ptr);
extern s32b object_value_real(const object_type *o_ptr);
extern s32b object_value(const object_type *o_ptr);
extern bool object_similar(const object_type *o_ptr, const object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, const object_type *j_ptr, bool floor);
extern s16b lookup_kind(int tval, int sval);
extern void object_wipe(object_type *o_ptr);
extern void object_copy(object_type *o_ptr, const object_type *j_ptr);
extern void object_prep(object_type *o_ptr, int k_idx);
extern bool sense_magic(object_type *o_ptr, int sense_level, bool heavy, bool floor);
extern void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, bool great);
extern bool make_object(object_type *j_ptr, bool good, bool great);
extern bool make_chest(int *feat);
extern bool make_gold(object_type *j_ptr, bool good, bool great);
extern bool make_body(object_type *j_ptr, int r_idx);
extern bool make_head(object_type *j_ptr, int r_idx);
extern bool make_part(object_type *j_ptr, int r_idx);
extern bool make_skin(object_type *j_ptr, int r_idx);
extern bool make_feat(object_type *j_ptr, int y, int x);
extern void place_trapped_door(int y, int x);
extern s16b floor_carry(int y, int x, object_type *j_ptr);
extern void race_near(int r_idx, int y1, int x1);
extern bool break_near(object_type *j_ptr, int y1, int x1);
extern bool check_object_lite(object_type *j_ptr);
extern void drop_near(object_type *j_ptr, int chance, int y, int x, bool dont_trigger);
extern void feat_near(int feat, int y, int x);
extern void acquirement(int y1, int x1, int num, bool great);
extern void place_object(int y, int x, bool good, bool great);
extern void place_chest(int y, int x);
extern void place_gold(int y, int x);
extern errr get_feat_num_prep(void);
extern s16b get_feat_num(int level);
extern void place_feature(int y, int x);
extern void pick_trap(int y, int x, bool player);
extern void place_trap(int y, int x);
extern void pick_door(int y, int x);
extern void place_secret_door(int y, int x);
extern void place_closed_door(int y, int x);
extern void place_locked_door(int y, int x);
extern void place_jammed_door(int y, int x);
extern void place_random_door(int y, int x);
extern void inven_item_charges(int item);
extern void inven_item_describe(int item);
extern void inven_item_increase(int item, int num);
extern void inven_item_optimize(int item);
extern void floor_item_charges(int item);
extern void floor_item_describe(int item);
extern void floor_item_increase(int item, int num);
extern void floor_item_optimize(int item);
extern bool inven_carry_okay(const object_type *o_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern s16b inven_takeoff(int item, int amt);
extern void inven_drop(int item, int amt, int m_idx);
extern void overflow_pack(void);
extern void combine_pack(void);
extern void reorder_pack(void);
extern void display_spell_list(void);
extern bool spell_match_style(int spell);
extern void fill_book(const object_type *o_ptr,s16b *book, int *num);
extern spell_cast *spell_cast_details(int spell);
extern bool spell_legible(int spell);
extern s16b spell_level(int spell);
extern s16b spell_power(int spell);
extern s16b spell_chance(int spell);
extern bool spell_okay(int spell, bool known);
extern byte allow_altered_inventory;
extern int get_tag_num(int o_idx, int cmd, byte *tag_num);
extern void find_quiver_size(void);
extern void combine_quiver(void);
extern int reorder_quiver(int slot);
extern bool is_throwing_item(const object_type *o_ptr);
extern bool is_known_throwing_item(const object_type *o_ptr);
extern int quiver_space_per_unit(const object_type *o_ptr);
extern bool quiver_carry_okay(const object_type *o_ptr, int num, int item);
extern byte quiver_get_group(const object_type *o_ptr);
extern bool quiver_carry(object_type *o_ptr, int o_idx);

/* save.c */
extern bool save_player(void);
extern bool save_player_bkp(bool bkp);

/* spells1.c */
extern u32b player_smart_flags(u32b f1,u32b f2,u32b f3, u32b f4);
extern u32b monster_smart_flags(int m_idx);
extern bool player_can_flags(int who, u32b f1,u32b f2,u32b f3, u32b f4);
extern bool player_not_flags(int who, u32b f1,u32b f2,u32b f3, u32b f4);
extern void update_smart_cheat(int who);
extern void update_smart_racial(int who);
extern bool update_smart_learn(int who, u32b flag);
extern bool update_smart_forget(int who, u32b flag);
extern bool update_smart_save(int who, bool saved);
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int dis);
extern void teleport_player_to(int ny, int nx);
extern void teleport_towards(int oy, int ox, int ny, int nx);
extern void teleport_player_level(void);
extern bool teleport_darkness_hook(const int oy, const int ox, const int ny, const int nx);
extern bool teleport_nature_hook(const int oy, const int ox, const int ny, const int nx);
extern bool teleport_fiery_hook(const int oy, const int ox, const int ny, const int nx);
extern void scatter_objects_under_feat(int y, int x);
extern void take_hit(int who, int what, int dam);
extern bool hates_terrain(object_type *o_ptr, int f_idx);
extern bool player_ignore_terrain(int f_idx);
extern int  calc_inc_stat(int value);
extern void inc_stat(int stat);
extern bool dec_stat(int stat, int amount);
extern bool res_stat(int stat);
extern bool apply_disenchant(int mode);
extern bool hates_fire(object_type *o_ptr);
extern void check_monster_quest(int m_idx, bool (*questor_test_hook)(int m_idx), u32b event);
extern bool temp_lite(int y, int x);
extern u16b bolt_pict(int y, int x, int ny, int nx, int typ);
#if 0
extern bool project_f(int who, int what, int y, int x, int dam, int typ);
extern bool project_o(int who, int what, int y, int x, int dam, int typ);
extern bool project_m(int who, int what, int y, int x, int dam, int typ);
extern bool project_p(int who, int what, int y, int x, int dam, int typ);
#endif
extern bool project_t(int who, int what, int y, int x, int dam, int typ);
extern bool project_shape(u16b *grid, s16b *gd, int *grids, int grid_s, int rad, int rng, int y0, int x0, int y1, int x1, int dam, int typ,
			 u32b flg, int degrees, byte source_diameter);
extern bool project_effect(int who, int what, u16b *grid, s16b *gd, int grids, int y0, int x0, int typ, u32b flg);
extern bool project(int who, int what, int rad, int rng, int y0, int x0, int y1, int x1, int dam, int typ,
			 u32b flg, int degrees, byte source_diameter);
extern bool project_one(int who, int what, int y, int x, int dam, int typ, u32b flg);
extern bool project_method(int who, int what, int method, int effect, int damage, int level, int y0, int x0,
			int y1, int x1, int region, u32b flg);


/* spells2.c */
extern bool project_dist(int who, int what, int y0, int x0, int dam, int typ, u32b flg, u32b flg2);
extern void generate_familiar(void);
extern bool hp_player(int num);
extern void warding_glyph(void);
extern void warding_trap(int feat, int dir);
extern int do_dec_stat(int stat);
extern bool do_res_stat(int stat);
extern bool do_inc_stat(int stat);
extern void identify_pack(void);
extern void uncurse_object(object_type *o_ptr);
extern bool remove_curse(void);
extern bool remove_all_curse(void);
extern bool restore_level(void);
extern bool disease_desc(char *desc, size_t desc_s, u32b old_disease, u32b new_disease);
extern void self_knowledge_aux(bool spoil, bool random);
extern void self_knowledge(bool spoil);
extern bool lose_all_info(void);
extern int value_check_aux3(const object_type *o_ptr);
extern int value_check_aux4(const object_type *o_ptr);
extern int value_check_aux5(const object_type *o_ptr);
extern int value_check_aux10(object_type *o_ptr, bool limit, bool weapon, bool floor);
extern bool place_random_stairs(int y, int x, int feat);
extern bool stair_creation(void);
extern bool enchant(object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(int num_hit, int num_dam, int num_ac);
extern bool brand_item(int brand, cptr act);
extern bool ident_spell(void);
extern bool ident_spell_gauge(void);
extern bool ident_spell_rumor(void);
extern bool ident_spell_tval(int tval);
extern bool identify_fully(void);
extern bool recharge(int num);
extern void aggravate_monsters(int who);
extern bool banishment(int who, int what, char typ);
extern bool mass_banishment(int who, int what, int y, int x);
extern bool probing(void);
extern void entomb(int cy, int cx, byte invalid);
extern void clear_temp_array(void);
extern void cave_temp_mark(int y, int x, bool room);
extern void spread_cave_temp(int y1, int x1, int range, bool room);
extern void lite_room(int y1, int x1);
extern void unlite_room(int y1, int x1);
extern void change_shape(int shape);
extern bool concentrate_water_hook(const int y, const int x, const bool modify);
extern bool concentrate_life_hook(const int y, const int x, const bool modify);
extern bool concentrate_light_hook(const int y, const int x, const bool modify);
extern int concentrate_power(int y0, int x0, int radius, bool for_real, bool use_los,
		bool concentrate_hook(const int y, const int x, const bool modify));
extern bool process_spell_flags(int who, int what, int spell, int level, bool *cancel, bool *known);
extern int process_spell_target(int who, int what, int y0, int x0, int y1, int x1, int spell, int level,
		int damage_div, bool one_grid, bool forreal, bool player, bool *cancel,
		bool retarget(int *ty, int *tx, u32b *flg, int method, int level, bool full, bool *one_grid));
extern bool process_spell_blows(int who, int what, int spell, int level, bool *cancel, bool *known, bool eaten);
extern bool item_tester_hook_magic_trap(const object_type *o_ptr);
extern bool player_set_magic_trap(int item);
extern bool process_spell_prepare(int spell, int level, bool *cancel, bool forreal, bool interact);
extern bool process_spell_types(int who, int spell, int level, bool *cancel, bool known);
extern bool process_spell(int who, int what, int spell, int level, bool *cancel, bool *known, bool eaten);
extern int process_item_blow(int who, int what, object_type *o_ptr, int y, int x, bool forreal, bool one_grid);
extern void wipe_region_piece_list(void);
extern void wipe_region_list(void);
extern void region_piece_wipe(region_piece_type *rp_ptr);
extern s16b region_piece_pop(void);
extern void region_piece_copy(region_piece_type *rp_ptr, const region_piece_type *rp_ptr2);
extern void compact_region_pieces(int size);
extern void region_wipe(region_type *r_ptr);
extern s16b region_pop(void);
extern void region_copy(region_type *r_ptr, const region_type *r_ptr2);
extern void compact_regions(int size);
extern void region_insert(u16b *gp, int grid_n, s16b *gd, s16b region);
extern void region_delete(s16b region);
extern bool region_grid(int y, int x, bool region_iterator(int y, int x, s16b d, s16b region));
extern void region_refresh(s16b region);
extern void region_highlight(s16b region);
extern void region_update(s16b region);
extern void region_terminate(s16b region);
extern s16b region_random_piece(s16b region);
extern void trigger_region(int y, int x, bool move);
extern s16b init_region(int who, int what, int type, int dam, int method, int effect, int level, int y0, int x0, int y1, int x1);
extern void process_regions(void);

/* store.c */
extern void do_cmd_store(void);
extern void store_shuffle(int store_index);
extern void store_maint(int store_index);
extern int store_init(int feat);

/* util.c */
extern errr path_parse(char *buf, int max, cptr file);
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern FILE *my_fopen(cptr file, cptr mode);
extern FILE *my_fopen_temp(char *buf, int max);
extern errr my_fclose(FILE *fff);
extern errr my_fgets(FILE *fff, char *buf, size_t n);
extern errr my_fputs(FILE *fff, cptr buf, size_t n);
extern errr fd_kill(cptr file);
extern errr fd_move(cptr file, cptr what);
extern errr fd_copy(cptr file, cptr what);
extern int fd_make(cptr file, int mode);
extern int fd_open(cptr file, int flags);
extern errr fd_lock(int fd, int what);
extern errr fd_seek(int fd, long n);
extern errr fd_read(int fd, char *buf, size_t n);
extern errr fd_write(int fd, cptr buf, size_t n);
extern errr fd_close(int fd);
extern errr check_modification_date(int fd, cptr template_file);
extern void text_to_ascii(char *buf, int len, cptr str);
extern void ascii_to_text(char *buf, int len, cptr str);
extern sint macro_find_exact(cptr pat);
extern errr macro_add(cptr pat, cptr act);
extern errr macro_init(void);
extern void flush(void);
extern key_event inkey_ex(void);
extern key_event anykey(void);
extern char inkey(void);
extern void bell(cptr reason);
extern void sound(int val);
extern s16b quark_add(cptr str);
extern cptr quark_str(s16b i);
extern errr quarks_init(void);
extern errr quarks_free(void);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern u16b message_type(s16b age);
extern byte message_color(s16b age);
extern errr message_color_define(u16b type, byte color);
extern void message_add(cptr str, u16b type);
extern void messages_easy(bool command);
extern errr messages_init(void);
extern void messages_free(void);
extern void move_cursor(int row, int col);
extern void msg_print(cptr msg);
extern void msg_format(cptr fmt, ...);
extern void message(u16b message_type, s16b extra, cptr message);
extern void message_format(u16b message_type, s16b extra, cptr fmt, ...);
extern void message_flush(void);
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern void text_out_to_file(byte attr, cptr str);
extern void text_out_to_screen(byte a, cptr str);
extern void text_out(cptr str);
extern void text_out_c(byte a, cptr str);
extern void clear_from(int row);
extern bool askfor_aux(char *buf, int len);
extern bool get_string(cptr prompt, char *buf, int len);
extern int get_quantity_aux(cptr prompt, int max, int amt);
extern s16b get_quantity(cptr prompt, int max);
extern bool get_check(cptr prompt);
extern bool get_com(cptr prompt, char *command);
extern bool get_com_ex(cptr prompt, key_event *command);
extern void pause_line(int row);
extern void request_command(bool shopping);
extern uint damroll(uint num, uint sides);
extern uint maxroll(uint num, uint sides);
extern bool is_a_vowel(int ch);
extern int color_char_to_attr(char c);
extern int color_text_to_attr(cptr name);
extern cptr attr_to_text(byte a);
extern bool is_valid_pf(int y, int x);
extern bool findpath(int y, int x);
extern s32b bst(s32b what, s32b t);
extern cptr get_month_name(int day, bool full, bool compact);
extern cptr get_day(int day);
extern bool get_list(print_list_func print_list, const s16b *sn, int num, cptr p, cptr q, cptr r, int y, int x, list_command_func list_command, int *selection);

#ifdef SUPPORT_GAMMA
extern void build_gamma_table(int gamma);
extern byte gamma_table[256];
#endif /* SUPPORT_GAMMA */

extern byte get_angle_to_grid[41][41];
extern int get_angle_to_target(int y0, int x0, int y1, int x1, int dir);
extern void get_grid_using_angle(int angle, int y0, int x0,
	int *ty, int *tx);
extern void grid_queue_create(grid_queue_type *q, size_t max_size);
extern void grid_queue_destroy(grid_queue_type *q);
extern bool grid_queue_push(grid_queue_type *q, byte y, byte x);
extern void grid_queue_pop(grid_queue_type *q);
extern int uti_icbrt(int arg);


/* xtra1.c */
extern void cnv_stat(int val, char *out_val);
extern void calc_spells(void);
extern s16b modify_stat_value(int value, int amount);
#ifdef USE_CLASS_PRETTY_NAMES
extern void lookup_prettyname(char *name, size_t name_s, int class, int style, int sval, bool long_name, bool short_name);
#endif
extern void notice_stuff(void);
extern void update_stuff(void);
extern void redraw_stuff(void);
extern void window_stuff(void);
extern void handle_stuff(void);

/* xtra2.c */
extern bool set_timed(int idx, int v, bool notify);
extern bool inc_timed(int idx, int v, bool notify);
extern bool dec_timed(int idx, int v, bool notify);
extern bool clear_timed(int idx, bool notify);
extern bool pfix_timed(int idx, bool notify);
extern bool set_poisoned(int v);
extern bool set_slow_poison(int v);
extern bool set_afraid(int v);
extern bool set_stun(int v);
extern bool set_cut(int v);
extern bool set_food(int v);
extern bool set_rest(int v);
extern bool set_msleep(int v);
extern bool set_psleep(int v);
extern bool set_stastis(int v);
extern void improve_familiar(void);
extern void check_experience(void);
extern void gain_exp(s32b amount);
extern void lose_exp(s32b amount);
extern int get_food_type(const monster_race *r_ptr);
extern int get_coin_type(const monster_race *r_ptr);
extern bool check_quest(quest_event *qe1_ptr, bool advance);
extern bool monster_drop(int m_idx);
extern bool monster_death(int m_idx);
extern bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note);
extern bool modify_panel(int wy, int wx);
extern bool adjust_panel(int y, int x);
extern bool change_panel(int dir);
extern void verify_panel(void);
extern void center_panel(void);
extern void display_room_info(int room);
extern void describe_room(void);
extern cptr look_mon_desc(int m_idx);
extern void ang_sort_aux(vptr u, vptr v, int p, int q);
extern void ang_sort(vptr u, vptr v, int n);
extern sint motion_dir(int y1, int x1, int y2, int x2);
extern sint target_dir(char ch);
extern bool target_able(int m_idx);
extern bool target_okay(void);
extern void target_set_monster(int m_idx, s16b flags);
extern void target_set_location(int y, int x, s16b flags);
extern key_event target_set_interactive_aux(int y, int x, int *room, int mode, cptr info);
extern bool target_set_interactive(int mode, int range, int radius, u32b flg, byte arc, byte diameter_of_source);
extern bool get_aim_dir(int *dp, int mode, int range, int radius, u32b flg, byte arc, byte diameter_of_source);
extern int get_monster_by_aim(int mode);
extern bool get_grid_by_aim(int mode, int *ty, int *tx);
extern int get_angle_to_dir(int angle);
extern bool get_rep_dir(int *dp);
extern bool confuse_dir(int *dp);
extern int min_depth(int dungeon);
extern int max_depth(int dungeon);
extern bool is_typical_town(int dungeon, int depth);
extern int town_depth(int dungeon);
extern void get_zone(dungeon_zone **zone_handle, int dungeon, int depth);
extern void long_level_name(char* str, int town, int depth);
extern void current_long_level_name(char* str);
extern int scale_method(method_level_scalar_type scalar, int level);


/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
# ifndef HAVE_USLEEP
/* util.c */
extern int usleep(huge usecs);
# endif /* HAVE_USLEEP */
extern void user_name(char *buf, size_t len, int id);
#endif /* SET_UID */


#ifdef ALLOW_REPEAT
/* util.c */
extern void repeat_push(int what);
extern bool repeat_pull(int *what);
extern void repeat_clear(void);
extern void repeat_check(void);
#endif /* ALLOW_REPEAT */

/* object1.c */
extern void show_floor(const int *floor_list, int floor_num, bool gold);

#ifdef GJW_RANDART
/* randart.c */
extern errr do_randart(u32b randart_seed, bool full);
#endif /* GJW_RANDART */


/*
 * Some global variables for the "mindless" borg.
 */
#ifdef ALLOW_BORG
/* borgdump.c */
extern u32b count_stop;	         /* Turns to automatic stop */
extern int count_change_level;   /* Turns to next level change */
extern int count_teleport;       /* Turns to next teleport */
extern byte allowed_depth[2];    /* Minimum and maximum depths */
extern byte borg_dir;            /* Current direction */
#endif


