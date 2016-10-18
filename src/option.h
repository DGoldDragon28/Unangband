#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H

const char *option_name(int opt);
const char *option_desc(int opt);

void option_set(int opt, bool on);
void option_set_defaults(void);


/*** Option display definitions ***/

/*
 * Information for "do_cmd_options()".
 */
#define OPT_PAGE_MAX				5
#define OPT_PAGE_PER				19

/* The option data structures */
extern const byte option_page[OPT_PAGE_MAX][OPT_PAGE_PER];



/*** Option definitions ***/

/*
 * Option indexes (offsets)
 *
 * These values are hard-coded by savefiles (and various pieces of code).  Ick.
 */
#define OPT_BIRTH					128
#define OPT_CHEAT					160
#define OPT_ADULT					192
#define OPT_SCORE					224
#define OPT_NONE					255
#define OPT_MAX					256

/*
 * Option indexes (hard-coded by savefiles)
 */
#define OPT_rogue_like_commands	0
#define OPT_quick_messages			1
#define OPT_use_sound            2
#define OPT_pickup_detail			3
#define OPT_use_old_target			4
#define OPT_pickup_always			5
#define OPT_pickup_inven			6

#define OPT_show_labels				10
#define OPT_show_lists           11

#define OPT_ring_bell				14
#define OPT_show_flavors			15

#define OPT_disturb_move			20
#define OPT_disturb_near			21
#define OPT_disturb_detect       22
#define OPT_disturb_state			23

#define OPT_view_perma_grids		38
#define OPT_view_torch_grids		39

#define OPT_flush_failure			52
#define OPT_flush_disturb			53

#define OPT_hilite_player			59
#define OPT_view_yellow_lite		60
#define OPT_view_bright_lite		61
#define OPT_view_granite_lite		62
#define OPT_view_special_lite		63
#define OPT_easy_open 				64
#define OPT_easy_alter 				65

#define OPT_show_piles				67
#define OPT_center_player			68

#define OPT_auto_more		   	71

/* unused, from V */
#define OPT_mouse_movement	   	77
/* unused, from V */
#define OPT_mouse_buttons	   	78

/* options specific to Un */
#define OPT_room_descriptions	   90
#define OPT_room_names				91
#define OPT_verify_mana			   92
#define OPT_easy_autos           93
#define OPT_easy_search			   94
#define OPT_view_glowing_lite    95
#define OPT_show_sidebar	      96
#define OPT_show_itemlist	      97
#define OPT_depth_in_feet        98
#define OPT_view_flavors         99
#define OPT_easy_corpses         100
#define OPT_view_unsafe_grids    101
#define OPT_view_detect_grids    102
#define OPT_show_tips            103
#define OPT_easy_more  		     104
#define OPT_autosave_backup      105
#define OPT_auto_monlist  		 106
#define OPT_easy_monlist  		 107
#define OPT_view_fogged_grids	108
#define OPT_ally_messages	109

#define OPT_birth_randarts          (OPT_BIRTH+1)
#define OPT_birth_rand_stats        (OPT_BIRTH+2)
#define OPT_birth_ironman           (OPT_BIRTH+3)
#define OPT_birth_no_stores         (OPT_BIRTH+4)
#define OPT_birth_no_artifacts      (OPT_BIRTH+5)
#define OPT_birth_no_stacking       (OPT_BIRTH+6)
#define OPT_birth_no_identify		(OPT_BIRTH+7)
#define OPT_birth_no_stairs			(OPT_BIRTH+8)
#define OPT_birth_no_selling		(OPT_BIRTH+9)
#define OPT_birth_lore				(OPT_BIRTH+10)
#define OPT_birth_auto           (OPT_BIRTH+11)
/* Options specific to Un */
#define OPT_birth_campaign          (OPT_BIRTH+20)
#define OPT_birth_haggle            (OPT_BIRTH+21)
#define OPT_birth_beginner          (OPT_BIRTH+22)
#define OPT_birth_intermediate      (OPT_BIRTH+23)
#define OPT_birth_first_time        (OPT_BIRTH+24)
#define OPT_birth_reseed_artifacts  (OPT_BIRTH+25)
#define OPT_birth_evil              (OPT_BIRTH+26)
#define OPT_birth_gollum			(OPT_BIRTH+27)

#define OPT_cheat_peek				(OPT_CHEAT+0)
#define OPT_cheat_hear				(OPT_CHEAT+1)
#define OPT_cheat_room				(OPT_CHEAT+2)
#define OPT_cheat_xtra				(OPT_CHEAT+3)
#define OPT_cheat_know				(OPT_CHEAT+4)
#define OPT_cheat_live				(OPT_CHEAT+5)
/* options specific to Un */
#define OPT_cheat_wall           (OPT_CHEAT+22)

#define OPT_adult_randarts          (OPT_ADULT+1)
#define OPT_adult_rand_stats        (OPT_ADULT+2)
#define OPT_adult_ironman           (OPT_ADULT+3)
#define OPT_adult_no_stores         (OPT_ADULT+4)
#define OPT_adult_no_artifacts      (OPT_ADULT+5)
#define OPT_adult_no_stacking       (OPT_ADULT+6)
#define OPT_adult_no_identify		(OPT_ADULT+7)
#define OPT_adult_no_stairs			(OPT_ADULT+8)
#define OPT_adult_no_selling		(OPT_ADULT+9)
#define OPT_adult_lore				(OPT_SCORE+10)
#define OPT_adult_auto				(OPT_SCORE+11)
/* options specific to Un */
#define OPT_adult_campaign  (OPT_ADULT+20)
#define OPT_adult_haggle (OPT_ADULT+21)
#define OPT_adult_beginner (OPT_ADULT+22)
#define OPT_adult_intermediate (OPT_ADULT+23)
#define OPT_adult_first_time (OPT_ADULT+24)
#define OPT_adult_reseed_artifacts (OPT_ADULT+25)
#define OPT_adult_evil              (OPT_ADULT+26)
#define OPT_adult_gollum			(OPT_ADULT+27)

#define OPT_score_peek				(OPT_SCORE+0)
#define OPT_score_hear				(OPT_SCORE+1)
#define OPT_score_room				(OPT_SCORE+2)
#define OPT_score_xtra				(OPT_SCORE+3)
#define OPT_score_know				(OPT_SCORE+4)
#define OPT_score_live				(OPT_SCORE+5)
/* options specific to Un */
#define OPT_score_wall           (OPT_SCORE+22)

/*
 * Hack -- Option symbols
 *
 * These shouldn't even be here.
 */
#define OPT(opt_name)	op_ptr->opt[OPT_##opt_name]

#define rogue_like_commands		OPT(rogue_like_commands)
#define quick_messages			OPT(quick_messages)
#define use_sound				OPT(use_sound)
#define pickup_detail			OPT(pickup_detail)
#define use_old_target			OPT(use_old_target)
#define pickup_always			OPT(pickup_always)
#define pickup_inven			OPT(pickup_inven)
#define show_labels				OPT(show_labels)
#define show_lists            OPT(show_lists)
#define ring_bell				OPT(ring_bell)
#define show_flavors			OPT(show_flavors)
#define disturb_move			OPT(disturb_move)
#define disturb_near			OPT(disturb_near)
#define disturb_detect			OPT(disturb_detect)
#define disturb_state			OPT(disturb_state)
#define view_perma_grids		OPT(view_perma_grids)
#define view_torch_grids		OPT(view_torch_grids)
#define flush_failure			OPT(flush_failure)
#define flush_disturb			OPT(flush_disturb)
#define hilite_player			OPT(hilite_player)
#define view_yellow_lite		OPT(view_yellow_lite)
#define view_bright_lite		OPT(view_bright_lite)
#define view_granite_lite		OPT(view_granite_lite)
#define view_special_lite		OPT(view_special_lite)
#define easy_open				OPT(easy_open)
#define easy_alter				OPT(easy_alter)
#define show_piles				OPT(show_piles)
#define center_player			OPT(center_player)
#define auto_more				OPT(auto_more)
#define mouse_movement			OPT(mouse_movement)
#define mouse_buttons			OPT(mouse_buttons)
#define room_descriptions 	OPT(room_descriptions)
#define room_names		  	OPT(room_names)
#define verify_mana		  	OPT(verify_mana)
#define easy_autos        	OPT(easy_autos)
#define easy_search		  	OPT(easy_search)
#define view_glowing_lite 	OPT(view_glowing_lite)
#define show_sidebar      	OPT(show_sidebar)
#define show_itemlist      OPT(show_itemlist)
#define depth_in_feet      OPT(depth_in_feet)
#define view_flavors       OPT(view_flavors)
#define easy_corpses      	OPT(easy_corpses)
#define view_unsafe_grids 	OPT(view_unsafe_grids)
#define view_detect_grids 	OPT(view_detect_grids)
#define show_tips         	OPT(show_tips)
#define easy_more	        OPT(easy_more)
#define autosave_backup     OPT(autosave_backup)
#define auto_monlist        OPT(auto_monlist)
#define easy_monlist        OPT(easy_monlist)
#define view_fogged_grids 	OPT(view_fogged_grids)
#define ally_messages	 	OPT(ally_messages)


#define birth_randarts			OPT(birth_randarts)
#define birth_rand_stats		OPT(birth_rand_stats)
#define birth_ironman			OPT(birth_ironman)
#define birth_no_stores			OPT(birth_no_stores)
#define birth_no_artifacts		OPT(birth_no_artifacts)
#define birth_no_stacking       OPT(birth_no_stacking)
#define birth_no_identify       OPT(birth_no_identify)
#define birth_no_stairs			OPT(birth_no_stairs)
#define birth_no_selling       OPT(birth_no_selling)
#define birth_lore            OPT(birth_lore)
#define birth_auto            OPT(birth_auto)
#define birth_campaign  			OPT(birth_campaign)
#define birth_haggle 		   	OPT(birth_haggle)
#define birth_beginner 			   OPT(birth_beginner)
#define birth_intermediate			OPT(birth_intermediate)
#define birth_first_time 			OPT(birth_first_time)
#define birth_reseed_artifacts  	OPT(birth_reseed_artifacts)
#define birth_evil	 			OPT(birth_evil)

#define cheat_peek				OPT(cheat_peek)
#define cheat_hear				OPT(cheat_hear)
#define cheat_room				OPT(cheat_room)
#define cheat_xtra				OPT(cheat_xtra)
#define cheat_know				OPT(cheat_know)
#define cheat_live				OPT(cheat_live)
#define cheat_wall            OPT(cheat_wall)

#define adult_randarts			OPT(adult_randarts)
#define adult_rand_stats		OPT(adult_rand_stats)
#define adult_ironman			OPT(adult_ironman)
#define adult_no_stores			OPT(adult_no_stores)
#define adult_no_artifacts		OPT(adult_no_artifacts)
#define adult_no_stacking		OPT(adult_no_stacking)
#define adult_no_identify       OPT(adult_no_identify)
#define adult_no_stairs			OPT(adult_no_stairs)
#define adult_no_selling       OPT(adult_no_selling)
#define adult_lore            OPT(adult_lore)
#define adult_auto            OPT(adult_auto)
#define adult_campaign  			OPT(adult_campaign)
#define adult_haggle 		   	OPT(adult_haggle)
#define adult_beginner 			   OPT(adult_beginner)
#define adult_intermediate			OPT(adult_intermediate)
#define adult_first_time 			OPT(adult_first_time)
#define adult_reseed_artifacts  	OPT(adult_reseed_artifacts)
#define adult_evil	 			OPT(adult_evil)
#define adult_gollum	 		OPT(adult_gollum)

#define score_peek				OPT(score_peek)
#define score_hear				OPT(score_hear)
#define score_room				OPT(score_room)
#define score_xtra				OPT(score_xtra)
#define score_know				OPT(score_know)
#define score_live				OPT(score_live)
#define score_wall            OPT(score_wall)

#endif /* !INCLUDED_OPTIONS_H */
