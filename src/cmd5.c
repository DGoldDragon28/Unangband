/* File: cmd5.c */

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
 * Checks if a given spellbook can be used by a specialist
 * Returns TRUE if the player can't use this kind of book.
 */
bool disdain_book(const object_type *o_ptr)
{	
	/* Specialists naturally disdain other books */
	if ((p_ptr->pstyle == WS_MAGIC_BOOK) || (p_ptr->pstyle == WS_PRAYER_BOOK) || (p_ptr->pstyle == WS_SONG_BOOK))
	{
		/* Reject some/all song books */
		if (((p_ptr->pstyle == WS_MAGIC_BOOK) && (o_ptr->tval != TV_MAGIC_BOOK)) ||
			((p_ptr->pstyle == WS_PRAYER_BOOK) && (o_ptr->tval != TV_PRAYER_BOOK)) ||
			((p_ptr->pstyle == WS_SONG_BOOK) && (o_ptr->tval != TV_SONG_BOOK)))
		{
			/* School specialists reject all alternate spellbooks, other specialists just reject the basic books */
			if ((p_ptr->psval >= SV_BOOK_MAX_GOOD) || (o_ptr->sval >= SV_BOOK_MAX_GOOD))
			{
				return(TRUE);
			}
		}
		/* Reject books outside of their school */
		else if ((o_ptr->sval >= SV_BOOK_MAX_GOOD)
				 && ((o_ptr->sval - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL != (p_ptr->pschool - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL))
		{
			return(TRUE);
		}
	}
	
	return(FALSE);
}

/*
 * Allow user to choose a spell/prayer from the given book.
 *
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray/sing, FALSE for study if
 * a book (eg magic book, prayer book, song book).
 * For all others, "known" should be TRUE if the player is to be
 * prompted to select an object power, or FALSE if a random power is
 * to be chosen.
 *
 * Now also allows scrolls, potions etc. We do not require mana or
 * ability to read to use any of these.
 */
int get_spell(int *sn, cptr prompt, object_type *o_ptr, bool known)
{
	int i;

	int num = 0;

	int spell = 0;

	int tval;

	s16b book[26];

	bool verify;


	int okay = 0;

	bool flag, redraw;
	key_event ke;

	spell_type *s_ptr;
	spell_cast *sc_ptr;

	char out_val[160];

	cptr p;

	bool cast = FALSE;

	/* Get fake tval */
	if (o_ptr->tval == TV_STUDY) tval = o_ptr->sval;
	else tval = o_ptr->tval;

	/* Spell */
	switch (tval)
	{
		case TV_PRAYER_BOOK:
			p="prayer";
			cast = TRUE;
			break;

		case TV_SONG_BOOK:
			p="song";
			cast = TRUE;
			break;

		case TV_MAGIC_BOOK:
			p="spell";
			cast = TRUE;
			break;

		default:
			p="power";
			break;
	}

	/* Cannot cast spells if illiterate */
	if ((cast) &&(c_info[p_ptr->pclass].spell_first > PY_MAX_LEVEL)
		&& (p_ptr->pstyle != WS_MAGIC_BOOK) && (p_ptr->pstyle != WS_PRAYER_BOOK) && (p_ptr->pstyle != WS_SONG_BOOK))
	{
		msg_print("You cannot read books.");

		return(-2);

	}

	/* Hack -- coated objects use coating instead */
	if (coated_p(o_ptr))
	{
		object_type object_type_body;
		object_type *j_ptr = &object_type_body;
		object_wipe(j_ptr);

		j_ptr->tval = o_ptr->xtra1;
		j_ptr->sval = o_ptr->xtra2;
		j_ptr->k_idx = lookup_kind(o_ptr->xtra1, o_ptr->xtra2);
		o_ptr = j_ptr;
	}

#ifdef ALLOW_REPEAT

	/* Get the spell, if available */
	if (repeat_pull(sn))
	{
		/* Verify the spell */
		if (!(cast) || (spell_okay(*sn, known)))
		{
			/* Success only if known */
			/* This is required to allow wands of wonder to repeat differently every time */
			if (known) return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT */

	/* Fill the book with spells */
	fill_book(o_ptr,book,&num);

	/* Assume no usable spells */
	okay = 0;

	/* Assume no spells available */
	(*sn) = -2;

	/* Check for "okay" spells if casting */
	if (cast) for (i = 0; i < num; i++)
	{
		/* Look for "okay" spells */
		if (spell_okay(book[i], known)) okay = TRUE;
	}

	/* Get a random spell/only one choice */
	else if ((!known) || (num == 1))
	{
		/* Get a random spell */
		*sn = book[rand_int(num)];

		/* Something happened */
		return (TRUE);
	}
	else okay = TRUE;

	/* No "okay" spells */
	if (!okay) return (FALSE);

	/* Assume cancelled */
	*sn = (-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Option -- automatically show lists */
	if (show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save screen */
		screen_save();

		/* Display a list of spells */
		if (cast) print_spells(book, num, 1, 20);
		else print_powers(book, num, 1, 20, o_ptr->name1 ? a_info[o_ptr->name1].level :
		(o_ptr->name2 ? e_info[o_ptr->name2].level : k_info[o_ptr->k_idx].level));
	}

	/* Build a prompt (accept all spells) */
	strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ",
	p, I2A(0), I2A(num - 1), prompt, p);

	/* Get a spell from the user */
	while (!flag && get_com_ex(out_val, &ke))
	{
		char choice;

		if (ke.key == '\xff')
		{
			if (ke.mousebutton)
			{
				if (redraw) ke.key = 'a' + ke.mousey - 2;
				else ke.key = ' ';
			}
			else continue;
		}

		/* Request redraw */
		if ((ke.key == ' ') || (ke.key == '*') || (ke.key == '?'))
		{
			/* Hide the list */
			if (redraw)
			{
				/* Load screen */
				screen_load();

				/* Hide list */
				redraw = FALSE;
			}

			/* Show the list */
			else
			{
				/* Show list */
				redraw = TRUE;

				/* Save screen */
				screen_save();

				/* Display a list of spells */
				if (cast) print_spells(book, num, 1, 20);
				else print_powers(book, num, 1, 20, o_ptr->name1 ? a_info[o_ptr->name1].level :
				(o_ptr->name2 ? e_info[o_ptr->name2].level : k_info[o_ptr->k_idx].level));
			}

			/* Ask again */
			continue;
		}

		choice = ke.key;

		/* Note verify */
		verify = (isupper(choice) ? TRUE : FALSE);

		/* Lowercase 1+*/
		choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell("Illegal spell choice!");
			continue;
		}

		/* Save the spell index */
		spell = book[i];

		/* Require "okay" spells */
		if ((cast) && (!spell_okay(spell, known)))
		{
			bell("Illegal spell choice!");
			msg_format("You may not %s that %s.", prompt, p);
			continue;
		}


		/* Verify it */
		if (verify)
		{
			char tmp_val[160];

			/* Get the spell */
			s_ptr = &s_info[spell];

			if (cast)
			{
				/* Get the casting details */
				sc_ptr = spell_cast_details(spell);

				/* Paranoia */
				if (!sc_ptr) return (FALSE);

				/* Prompt */
				strnfmt(tmp_val, 78, "%^s %s (%d mana, %d%% fail)? ",
				prompt, s_name + s_ptr->name,
				sc_ptr->mana, spell_chance(spell));
			}
			else
			{
				/* Prompt */
				strnfmt(tmp_val, 78, "%^s %s)? ",
				prompt, s_name + s_ptr->name);
			}

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
		/* Load screen */
		screen_load();

		/* Hack -- forget redraw */
		/* redraw = FALSE; */
	}

	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = spell;

#ifdef ALLOW_REPEAT

	repeat_push(*sn);

#endif /* ALLOW_REPEAT */

	/* Success */
	return (TRUE);
}


/*
 * Is the book okay to select?
 *
 * This requires that the player has spells in it that
 * they can 'read'.
 */
bool inven_book_okay(const object_type *o_ptr)
{
	if ((o_ptr->tval != TV_MAGIC_BOOK) &&
  	  (o_ptr->tval != TV_PRAYER_BOOK) &&
  	  (o_ptr->tval != TV_SONG_BOOK) &&
	  (o_ptr->tval != TV_STUDY)) return (0);

	/* Study notes */
	if (o_ptr->tval == TV_STUDY)
	{
		return (spell_legible(o_ptr->pval));
	}

	/* Book */
	else
	{
		s16b book[26];
		int num;
		int i;

		fill_book(o_ptr, book, &num);

		for (i=0; i < num; i++)
		{
			if (spell_legible(book[i])) return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Is the book okay to study from?
 *
 * This requires that the player has spells in it that
 * they can 'study'.
 */
bool inven_study_okay(const object_type *o_ptr)
{
	if ((o_ptr->tval != TV_MAGIC_BOOK) &&
  	  (o_ptr->tval != TV_PRAYER_BOOK) &&
  	  (o_ptr->tval != TV_SONG_BOOK) &&
	  (o_ptr->tval != TV_STUDY)) return (0);

	/* Study notes */
	if (o_ptr->tval == TV_STUDY)
	{
		return (spell_okay(o_ptr->pval, FALSE));
	}

	/* Book */
	else
	{
		if (disdain_book(o_ptr)) return FALSE;

		s16b book[26];
		int num;
		int i;

		fill_book(o_ptr, book, &num);

		for (i=0; i < num; i++)
		{
			if (spell_okay(book[i], FALSE)) return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Peruse the spells/prayers in a Book.
 *
 * Takes an object as a parameter
 */
bool player_browse_object(object_type *o_ptr)
{
	int num = 0;

	s16b book[26];

	cptr p;

	int spell=-1;

	int i, ii;

	int tval;
	int level;

	char choice = 0;

	char out_val[160];
	
	/* Get fake tval */
	if (o_ptr->tval == TV_STUDY) tval = o_ptr->sval;
	else tval = o_ptr->tval;

	/* Spell */
	switch (tval)
	{
		case TV_PRAYER_BOOK:
			p="prayer";
			break;

		case TV_SONG_BOOK:
			p="song";
			break;

		case TV_MAGIC_BOOK:
			p="spell";
			break;

		default:
			p="power";
			break;
	}

	/* Fill book with spells */
	fill_book(o_ptr,book,&num);

	/* Paranoia */
	if (num == 0)
	{
		msg_format("There are no %ss to browse.",p);
		return (FALSE);
	}

	/* Reject spellbooks if a specialist */
	if (disdain_book(o_ptr))
	{
		msg_format("You cannot read any %ss in it.",p);
		
		switch ((o_ptr->sval - SV_BOOK_MAX_GOOD) % SV_BOOK_SCHOOL)
		{
			case 0: msg_print("It shows a lack of grasp of simple theory."); break;
			case 1: msg_print("It could never work due to harmonic instability."); break;
			case 2: msg_print("It's the deranged scribblings from a lunatic asylum."); break;
			case 3: msg_print("The book snaps itself shut and jumps from your fingers."); break;
			default: msg_print("Your eyes twist away from the pages... there were some things man was not meant to know."); break;
		}

		return (FALSE);
	}
	
	/* Display the spells */
	print_spells(book, num, 1, 20);

	/* Build a prompt (accept all spells) */
	strnfmt(out_val, 78, "(%^ss %c-%c, ESC=exit) Browse which %s? ",
		p, I2A(0), I2A(num - 1), p);

	/* Get a spell from the user */
	while ((choice != ESCAPE) && get_com(out_val, &choice))
	{
		/* Lowercase 1+*/
		choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		if ((i >= 0) && (i < num))
		{
			spell_type *s_ptr;

			/* Save the spell index */
			spell = book[i];

			/* Load screen */
			screen_load();

			/* Save screen */
			screen_save();

			/* Display the spells */
			print_spells(book, num, 1, 20);

			/* Begin recall */
			Term_gotoxy(0, 1);

			/* Get the spell */
			s_ptr = &s_info[spell];

			/* Spell is illegible */
			if (!spell_legible(spell))
			{
				msg_format("You cannot read that %s.",p);

				/* Build a prompt (accept all spells) */
				strnfmt(out_val, 78, "(%^ss %c-%c, ESC=exit) Browse which %s? ",
					p, I2A(0), I2A(num - 1), p);
			}
			else
			{
				bool intro = FALSE;
				bool dummy = TRUE;

				/* Prepare the spell */
				process_spell_prepare(spell, spell_power(spell), &dummy, FALSE, TRUE);

				/* Set text_out hook */
				text_out_hook = text_out_to_screen;

				/* Hack -- 'wonder' spells */
				if (s_info[book[i]].type == SPELL_USE_OBJECT)
				{
					/* No listing of powers, since it overflows a standard screen */
					text_out("When cast, it affects the target in a random way.  ");
				}

				/* Recall spell */
				if (spell_desc(&s_info[spell],"When cast, it ",spell_power(spell), TRUE, 1))
				{
					int y, x;

					/* End the sentence */
					text_out(".  ");

					/* Clear until the end of the line */
					/* XXX XXX Not sure why we have to do this.*/

					/* Obtain the cursor */
					(void)Term_locate(&x, &y);

					/* Clear line, move cursor */
					Term_erase(x, y, 255);
				}

				/* Display pre-requisites, unless specialist */
				if (!spell_match_style(spell))
				  for (i = 0; i < MAX_SPELL_PREREQUISITES; i++)
				  {
				    /* Check if pre-requisite spells */
				    if (s_info[spell].preq[i])
				    {
				      if (!intro) text_out_c(TERM_VIOLET,"You must learn ");
				      else text_out_c(TERM_VIOLET, " or ");

				      intro = TRUE;

				      text_out_c(TERM_VIOLET, s_name + s_info[s_info[spell].preq[i]].name);
				    }

				  }

				/* Terminate if required */
				if (intro) text_out_c(TERM_VIOLET, format(" before studying this %s.\n",p));

				/* Spell not legible */
				if (!spell_legible(spell))
				{
					text_out_c(TERM_SLATE, "You lack the ability to learn this spell.\n");
				}
				else
				{
					/* Get level */
					level = spell_level(spell);

					/* Get the spell knowledge*/
					for (ii=0;ii<PY_MAX_SPELLS;ii++)
					{
						if (p_ptr->spell_order[ii] == spell) break;
					}

					/* Analyze the spell */
					if (ii==PY_MAX_SPELLS)
					{
						if (level <= p_ptr->lev)
						{
							if (spell_okay(spell, FALSE))
							{
								/* Describe how the player learns spells */
								if (o_ptr->tval == TV_PRAYER_BOOK)
								{
									text_out_c(TERM_BLUE_SLATE, "The luck of the gods decides whether you learn this spell.\n");
								}
								else if (o_ptr->tval == TV_SONG_BOOK)
								{
									int iii;

									/* Do the hard work */
									for(iii=0;iii<num;iii++)
									{
										if (spell_okay(book[iii],FALSE) && (i == iii))
										{
											text_out_c(TERM_L_BLUE, "You will learn this spell next if you study this book.  ");
										}
									}
									text_out_c(TERM_BLUE_SLATE, "You must learn these spells in the listed order.\n");
								}
								else
								{
									text_out_c(TERM_BLUE_SLATE, "You may choose to learn this spell.\n");
								}
							}
							else
							{
								/* Already highlighted pre-requisites */
							}
						}
						else
						{
							text_out_c(TERM_L_RED, "You are not at high enough a level to learn this spell.\n");
						}
					}
					else if ((ii < 32) ? (p_ptr->spell_forgotten1 & (1L << ii)) :
						  ((ii < 64) ? (p_ptr->spell_forgotten2 & (1L << (ii - 32))) :
						  ((ii < 96) ? (p_ptr->spell_forgotten3 & (1L << (ii - 64))) :
						  (p_ptr->spell_forgotten4 & (1L << (ii - 96))))))
					{
						/* Describe how the player learns spells */
						text_out_c(TERM_L_YELLOW, "You have forgotten how to cast this spell.\n");
					}
					else if (!((ii < 32) ? (p_ptr->spell_learned1 & (1L << ii)) :
						  ((ii < 64) ? (p_ptr->spell_learned2 & (1L << (ii - 32))) :
						  ((ii < 96) ? (p_ptr->spell_learned3 & (1L << (ii - 64))) :
						  (p_ptr->spell_learned4 & (1L << (ii - 96)))))))
					{
						if (level <= p_ptr->lev)
						{
							/* Describe how the player learns spells */
							if (o_ptr->tval == TV_PRAYER_BOOK)
							{
								text_out_c(TERM_BLUE_SLATE, "The luck of the gods decides whether you learn this spell.\n");
							}
							else if (o_ptr->tval == TV_SONG_BOOK)
							{
								int iii;

								/* Do the hard work */
								for(iii=0;iii<num;iii++)
								{
									if (spell_okay(book[iii],FALSE) && (i == iii))
									{
										text_out_c(TERM_L_BLUE, "You will learn this spell next if you study this book.  ");
									}
								}
								text_out_c(TERM_BLUE_SLATE, "You must learn these spells in the listed order.\n");
							}
							else
							{
								text_out_c(TERM_BLUE_SLATE, "You may choose to learn this spell.\n");
							}
						}
						else
						{
							text_out_c(TERM_L_RED, "You are not at high enough a level to learn this spell.\n");
						}
					}
					else if (!((ii < 32) ? (p_ptr->spell_worked1 & (1L << ii)) :
						  ((ii < 64) ? (p_ptr->spell_worked2 & (1L << (ii - 32))) :
						  ((ii < 96) ? (p_ptr->spell_worked3 & (1L << (ii - 64))) :
						  (p_ptr->spell_worked4 & (1L << (ii - 96)))))))
					{
						text_out_c(TERM_L_GREEN, "You have yet to cast this spell.\n");
					}
				}

				/* Build a prompt (accept all spells) */
				strnfmt(out_val, 78, "The %s of %s. (%c-%c, ESC) Browse which %s:",
					p, s_name + s_info[spell].name,I2A(0), I2A(num - 1), p);

				/* Paranoia - clear boost */
				p_ptr->boost_spell_power = 0;
			}

			continue;
		}
	}

	return (TRUE);
}



/*
 * Peruse the spells/prayers in a Book
 *
 * Note that *all* spells in the book are listed
 *
 * Note that browsing is allowed while berserk or blind,
 * and in the dark, primarily to allow browsing in stores.
 */
bool player_browse(int item)
{
	int sval;

	object_type *o_ptr;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Get the item's sval */
	sval = o_ptr->sval;

	/* Save the screen */
	screen_save();

	/* Browse the object */
	if (player_browse_object(o_ptr))
	{
		/* Prompt for a command */
		put_str("(Browsing) Command: ", 0, 0);

		/* Hack -- Get a new command */
		p_ptr->command_new = inkey_ex();
	}
	/* Hack -- we shouldn't need this here. TODO */
	else if (easy_more)
	{
		msg_print(NULL);

		messages_easy(TRUE);
	}

	/* Load screen */
	screen_load();

	/* Hack -- Process "Escape" */
	if (p_ptr->command_new.key == ESCAPE)
	{
		/* Reset stuff */
		p_ptr->command_new.key = 0;
	}

	return (TRUE);
}



/*
 * Study a book to gain a new spell/prayer
 */
bool player_study(int item)
{
	int i;

	int spell = -1;

	cptr p, r;

	cptr u = " book";

	object_type *o_ptr;

	spell_type *s_ptr;

	int tval;

	int max_spells = PY_MAX_SPELLS;
	int study_slot = MAX_STUDY_BOOK;

	object_type object_type_body;

	bool study_item = FALSE;
	bool disdain = FALSE;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Generate study item from features */
	if (o_ptr->ident & (IDENT_STORE)) study_item = TRUE;

	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);

	/* Hack -- Handle stuff */
	handle_stuff();

	/* Get fake tval */
	if (o_ptr->tval == TV_STUDY)
	{
		tval = o_ptr->sval;
		u = "";
	}
	else tval = o_ptr->tval;

	/* Spell */
	switch (tval)
	{
		case TV_PRAYER_BOOK:
			p="prayer";
			r="Pray for which blessing";
			break;

		case TV_SONG_BOOK:
			p="song";
			r="Improvise which melody";
			break;

		case TV_MAGIC_BOOK:
			p="spell";
			r="Research which field";
			break;

		default:
			p="power";
			r = "";
			break;
	}

	/* Study from books */
	if (o_ptr->tval != TV_STUDY)
	{
		/* Check if the item already using a study slot? */
		for (i = 0; i < p_ptr->pack_size_reduce_study; i++)
		{
			/* Already studied this book */
			if (o_ptr->k_idx == p_ptr->study_slot[i]) break;
		}

		/* No room for more book types */
		if (i == MAX_STUDY_BOOK)
		{
			msg_format("You cannot study any more types of %ss.", p);

			return (FALSE);
		}

		/* Slot to use */
		study_slot = i;
	}

	/* Specialists naturally disdain other books */
	if ((p_ptr->pstyle == WS_MAGIC_BOOK) || (p_ptr->pstyle == WS_PRAYER_BOOK) || (p_ptr->pstyle == WS_SONG_BOOK))
	{
		/* Reject some/all song books */
		if (((p_ptr->pstyle == WS_MAGIC_BOOK) && (o_ptr->tval != TV_MAGIC_BOOK)) ||
				((p_ptr->pstyle == WS_PRAYER_BOOK) && (o_ptr->tval != TV_PRAYER_BOOK)) ||
				((p_ptr->pstyle == WS_SONG_BOOK) && (o_ptr->tval != TV_SONG_BOOK)))
		{
			/* School specialists reject all alternate spellbooks, other specialists just reject the basic books */
			if ((p_ptr->psval >= SV_BOOK_MAX_GOOD) || (o_ptr->sval >= SV_BOOK_MAX_GOOD))
			{
				disdain = TRUE;
			}
		}
		/* Reject books outside of their school */
		else if ((o_ptr->sval >= SV_BOOK_MAX_GOOD)
			&& ((o_ptr->sval - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL != (p_ptr->pschool - SV_BOOK_MAX_GOOD) / SV_BOOK_SCHOOL))
		{
			disdain = TRUE;
		}
	}

	/* Spell is illegible */
	if (disdain)
	{
		msg_format("You cannot study that %s.",p);

		switch ((o_ptr->sval - SV_BOOK_MAX_GOOD) % SV_BOOK_SCHOOL)
		{
			case 0: msg_print("You lack the grasp of simple theory."); break;
			case 1: msg_print("You can't master it due to harmonic instability."); break;
			case 2: msg_print(format("Your %ss resemble deranged scribblings from a lunatic asylum.", p)); break;
			case 3: msg_print("The book burns red hot in your hands!"); break;
			default: msg_print("There were some things man was not meant to know, and those things are now crawling around your brain like worms..."); break;
		}

		msg_print("You pass out from the strain!");

		/* Hack -- Bypass free action */
		inc_timed(TMD_PARALYZED, randint((o_ptr->sval - SV_BOOK_MAX_GOOD) % SV_BOOK_SCHOOL + 1), FALSE);

		return (FALSE);
	}

	/* Prayer book -- Learn a random prayer */
	if (o_ptr->tval == TV_PRAYER_BOOK)
	{
		int ii;

		int k = 0;

		int gift = -1;

		for (i=0;i<z_info->s_max;i++)
		{
			s_ptr=&s_info[i];

			for (ii=0;ii<MAX_SPELL_APPEARS;ii++)
			{
				if ((s_ptr->appears[ii].tval == o_ptr->tval) &&
			    	(s_ptr->appears[ii].sval == o_ptr->sval) &&
				(spell_okay(i,FALSE)))
				{
					if ((++k == 1) || ((k > 1) &&
						(rand_int(k) ==0)))
					{
						gift = i;
					}
				}
			}
		}

		/* Accept gift */
		spell = gift;
	}

	/* Song book -- Learn a spell in order */
	else if (o_ptr->tval == TV_SONG_BOOK)
	{
		s16b book[26];

		int num = 0;

		int graft = -1;

		/* Fill the book with spells */
		fill_book(o_ptr,book,&num);

		/* Do the hard work */
		for(i=0;i<num;i++)
		{
			if (spell_okay(book[i],FALSE))
			{
				graft = book[i];
				break;
			}

		}

		/* Accept graft */
		spell = graft;
	}

	/* Magic book -- Learn a selected spell */
	else if (o_ptr->tval == TV_MAGIC_BOOK)
	{
		/* Ask for a spell, allow cancel */
		if (!get_spell(&spell, "study", o_ptr, FALSE) && (spell == -1)) return (FALSE);
	}

	/* Study -- only one spell available */
	else if (o_ptr->tval == TV_STUDY)
	{
		if (spell_okay(o_ptr->pval,FALSE)) spell = o_ptr->pval;
	}

	/* Nothing to study */
	if (spell < 0)
	{
		/* Message */
		msg_format("You cannot learn any %ss in that%s.", p, u);

		/* Abort */
		return (FALSE);
	}


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Find the next open entry in "spell_order[]" */
	for (i = 0; i < PY_MAX_SPELLS; i++)
	{
		/* Stop at the first empty space */
		if (p_ptr->spell_order[i] == 0) break;
	}

	/* Paranoia */
	if (i >= max_spells)
	{
		/* Message */
		msg_format("You cannot learn any more %ss.", p);

		return (FALSE);
	}

	/* Add the spell to the known list */
	p_ptr->spell_order[i] = spell;

	/* Learn the spell */
	if (i < 32)
	{
		p_ptr->spell_learned1 |= (1L << i);
	}
	else if (i < 64)
	{
		p_ptr->spell_learned2 |= (1L << (i - 32));
	}
	else if (i < 96)
	{
		p_ptr->spell_learned3 |= (1L << (i - 64));
	}
	else
	{
		p_ptr->spell_learned4 |= (1L << (i - 96));
	}

	/*Set to spell*/
	s_ptr = &(s_info[spell]);

	/* Mention the result */
	message_format(MSG_STUDY, 0, "You have learned the %s of %s.",
	   p, s_name + s_ptr->name);

	/* One less book available if we're studying from a new book */
	if ((study_slot < MAX_STUDY_BOOK) && (study_slot == p_ptr->pack_size_reduce_study))
	{
		p_ptr->study_slot[p_ptr->pack_size_reduce_study++] = o_ptr->k_idx;
	}

	/* One less spell available */
	p_ptr->new_spells--;

	/* Save the new_spells value */
	p_ptr->old_spells = p_ptr->new_spells;

	/* Create study item if required */
	if (study_item)
	{
		/* Create a new study object */
		o_ptr = &object_type_body;

		/* Prepare object */
		object_prep(o_ptr,lookup_kind(TV_STUDY, tval));

		/* Set the spell */
		o_ptr->pval = spell;

		/* And carry it */
		item = inven_carry(o_ptr);
	}

	/* Queue a tip if appropriate */
	queue_tip(format("spell%d.txt", spell));

	/* Redraw Study Status */
	p_ptr->redraw |= (PR_STUDY);

	/* Redraw object recall */
	p_ptr->window |= (PW_OBJECT);

	return (TRUE);

}


/*
 * Check whether there is a spell that can be cast from the object by the player.
 */
bool inven_cast_okay(const object_type *o_ptr)
{
	int i,ii;

	spell_type *s_ptr;

	if ((o_ptr->tval != TV_MAGIC_BOOK) &&
	    (o_ptr->tval != TV_PRAYER_BOOK) &&
		 (o_ptr->tval != TV_SONG_BOOK) &&
		 (o_ptr->tval != TV_STUDY))
		return (0);

	/* Research materials */
	if (o_ptr->tval == TV_STUDY)
	{
		for (i=0;i<PY_MAX_SPELLS;i++)
		{
			if (p_ptr->spell_order[i] == o_ptr->pval) return(1);
		}
	}

	/* Book */
	else {
		if (disdain_book(o_ptr)) return FALSE;

		for (i=0;i<PY_MAX_SPELLS;i++)
		{
			if (p_ptr->spell_order[i] == 0) continue;

			s_ptr=&s_info[p_ptr->spell_order[i]];

			/* Book */
			for (ii=0;ii<MAX_SPELL_APPEARS;ii++)
			{
				if ((s_ptr->appears[ii].tval == o_ptr->tval) &&
					(s_ptr->appears[ii].sval == o_ptr->sval))

				{
					return(1);
				}
			}
		}
	}

	return (0);

}


/*
 * Cast a spell (once chosen); return FALSE if aborted
 */
bool player_cast_spell(int spell, int plev, cptr p, cptr t)
{
	int i;
	int chance;

	spell_type *s_ptr;
	spell_cast *sc_ptr;


	/* Get the spell */
	s_ptr = &s_info[spell];

	/* Get the cost */
	sc_ptr = spell_cast_details(spell);

	/* Paranoia */
	if (!sc_ptr) return (FALSE);

	/* Verify "dangerous" spells */
	if (sc_ptr->mana > p_ptr->csp)
	{
		/* Warning */
		msg_format("You do not have enough mana to %s this %s.",p,t);

		/* No constitution to drain */
		if (!p_ptr->stat_ind[A_CON]) return FALSE;

		/* Verify */
		if (!get_check("Attempt it anyway? "))
		{
			if (p_ptr->held_song)
			{
				/* Redraw the state */
				p_ptr->redraw |= (PR_STATE);

				p_ptr->held_song = 0;
			}

			return FALSE;
		}
	}

	/* Verify "warning" spells */
	else if ((verify_mana) &&
	 ((p_ptr->csp - sc_ptr->mana) < (p_ptr->msp * op_ptr->hitpoint_warn) / 10))
	{
		/* Warning */
		msg_format("You have limited mana to %s this %s.",p,t);

		/* Verify */
		if (!get_check("Attempt it anyway? "))
		{
			if (p_ptr->held_song)
			{
				/* Redraw the state */
				p_ptr->redraw |= (PR_STATE);

				p_ptr->held_song = 0;
			}
			return FALSE;
		}
	}

	/* Spell failure chance */
	chance = spell_chance(spell);

	/* Some items and some rooms silence the player */
	if ((p_ptr->cur_flags4 & (TR4_SILENT)) || (room_has_flag(p_ptr->py, p_ptr->px, ROOM_SILENT)))
	{
		/* Some items silence the player */
		chance = 99;

		/* Warn the player */
		msg_print("You are engulfed in magical silence.");

		/* Get the room */
		if (!(room_has_flag(p_ptr->py, p_ptr->px, ROOM_SILENT)))
		{
			/* Always notice */
			equip_can_flags(0x0L,0x0L,0x0L,TR4_SILENT);
		}
	}
	else
	{
		/* Always notice */
		equip_not_flags(0x0L,0x0L,0x0L,TR4_SILENT);
	}

	/* Failed spell */
	if (rand_int(100) < chance)
	{
		if (flush_failure) flush();
		msg_format("You failed to %s the %s!",p,t);

		if (p_ptr->held_song)
		{
			/* Redraw the state */
			p_ptr->redraw |= (PR_STATE);

			p_ptr->held_song = 0;
		}
	}

	/* Process spell */
	else
	{
		/* Must be true to let us abort */
		bool abort = TRUE;

		/* Always true */
		bool known = TRUE;

		/* Apply the spell effect */
		process_spell(SOURCE_PLAYER_CAST, spell, spell,plev,&abort,&known, FALSE);

		/* Did we cancel? */
		if (abort) return FALSE;

		for (i=0;i<PY_MAX_SPELLS;i++)
		{
			if (p_ptr->spell_order[i] == spell) break;
		}

		/* Paranoia */
		if (i==PY_MAX_SPELLS) ;

		/* A spell was cast */
		else if (!((i < 32) ?
		      (p_ptr->spell_worked1 & (1L << i)) :
		      ((i < 64) ? (p_ptr->spell_worked2 & (1L << (i - 32))) :
		      ((i < 96) ? (p_ptr->spell_worked3 & (1L << (i - 64))) :
		      (p_ptr->spell_worked4 & (1L << (i - 96)))))))
		{
			/* The spell worked */
			if (i < 32)
			{
				p_ptr->spell_worked1 |= (1L << i);
			}
			else if (i < 64)
			{
				p_ptr->spell_worked2 |= (1L << (i - 32));
			}
			else if (i < 96)
			{
				p_ptr->spell_worked3 |= (1L << (i - 64));
			}
			else if (i < 128)
			{
				p_ptr->spell_worked2 |= (1L << (i - 96));
			}

			/* Redraw object recall */
			p_ptr->window |= (PW_OBJECT);
		}
	}

	/* Sufficient mana */
	if (sc_ptr->mana <= p_ptr->csp)
	{
		/* Over-exert the player if casting in the 'red-zone' of their reserve.
		 * Note that if this occurs, the spell does not cost any mana.
		 */
		if ((p_ptr->reserves) && (p_ptr->csp < adj_con_reserve[p_ptr->stat_ind[A_CON]] / 2) &&
			(rand_int(100) < adj_con_reserve[p_ptr->stat_ind[A_CON]]) &&
			(sc_ptr->mana) && (p_ptr->stat_ind[A_CON]))
		{
			/* Temporarily weaken the player */
			if (!p_ptr->timed[TMD_DEC_CON])
			{
				inc_timed(TMD_DEC_CON,rand_int(20) + 20, TRUE);
			}

			/* Weaken the player */
			else
			{
				/* Message */
				msg_print("You have damaged your health!");

				/* Reduce constitution */
				(void)dec_stat(A_CON, 15 + randint(10));
			}
		}
		else
		{
			/* Use some mana */
			p_ptr->csp -= sc_ptr->mana;

			/* Need reserves */
			if (!(p_ptr->reserves) && (p_ptr->csp < adj_con_reserve[p_ptr->stat_ind[A_CON]] / 2))
			{
				/* Give some mana */
				msg_print("You draw on your reserves.");

				p_ptr->reserves = TRUE;

				p_ptr->csp = (p_ptr->csp + adj_con_reserve[p_ptr->stat_ind[A_CON]]) / 2;

				/* Update mana */
				p_ptr->update |= (PU_MANA);
			}
		}
	}

	/* Over-exert the player */
	else
	{
		int oops = sc_ptr->mana - p_ptr->csp;

		/* No mana left */
		p_ptr->csp = 0;
		p_ptr->csp_frac = 0;

		/* Message */
		msg_print("You faint from the effort!");

		/* Hack -- Bypass free action */
		inc_timed(TMD_PARALYZED, randint(5 * oops + 1), TRUE);

		/* Damage CON */
		if (rand_int(100) < 50)
		{
			/* Message */
			msg_print("You have damaged your health!");

			/* Reduce constitution */
			(void)dec_stat(A_CON, 15 + randint(10));

			/* Add to the temporary drain */
			inc_timed(TMD_DEC_CON, rand_int(20) + 20, TRUE);
		}
	}

	/* Redraw mana */
	p_ptr->redraw |= (PR_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

	return TRUE;
}


/*
 * Choose a spell to cast from an item
 */
bool player_cast(int item)
{
	int spell, tval;

	object_type *o_ptr;

	cptr p, t;

	cptr u = " book";

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}
		/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Get fake tval */
	if (o_ptr->tval == TV_STUDY)
	{
		tval = o_ptr->sval;
		u = "";
	}
	else tval = o_ptr->tval;

	/* Cast, recite, sing or play */
	switch (tval)
	{
		case TV_MAGIC_BOOK:
		{
			p="cast";
			t="spell";

			break;
		}
		case TV_PRAYER_BOOK:
		{
			p="recite";
			t="prayer";

			break;
		}
		case TV_SONG_BOOK:
		{
			if (p_ptr->pstyle == WS_INSTRUMENT)
			{
				p="play";
			}
			else
			{
				p="sing";
			}

			t = "song";
			break;
		}

		default:
		{
			p="use";
			t="power";
			break;
		}
	}

	/* Ask for a spell */
	if (!get_spell(&spell, p, o_ptr, TRUE))
	{
		if (spell == -2) msg_format("You don't know any %ss in that%s.",t,u);
		return (FALSE);
	}

	/* Hold a song if possible */
	if (s_info[spell].flags3 & (SF3_HOLD_SONG))
	{
		int i;

		for (i = 0;i< z_info->w_max;i++)
		{
			if (w_info[i].class != p_ptr->pclass) continue;

			if (w_info[i].level > p_ptr->lev) continue;

			if (w_info[i].benefit != WB_HOLD_SONG) continue;

			/* Check for styles */
			if ((w_info[i].styles==0) || (w_info[i].styles & (p_ptr->cur_style & (1L << p_ptr->pstyle))))
			{
				/* Verify */
				if (get_check(format("Continue singing %s?", s_name + s_info[spell].name))) p_ptr->held_song = spell;
			}

			/* Hack - Cancel searching */
			/* Stop searching */
			if (p_ptr->searching)
			{
				/* Clear the searching flag */
				p_ptr->searching = FALSE;

				/* Clear the last disturb */
				p_ptr->last_disturb = turn;
			}

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Redraw the state */
			p_ptr->redraw |= (PR_STATE);

		}
	}

	/* Cast the spell - held songs get cast later*/
	if (p_ptr->held_song != spell)
		if (player_cast_spell(spell,spell_power(spell),p,t))
			/* If not aborted, take a turn */
			p_ptr->energy_use = 100;

	return (TRUE);
}

