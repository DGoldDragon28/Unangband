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

static bool quest_res_queued = TRUE;

/*
 * Handle a quest event.
 */
bool check_quest(quest_event *qe1_ptr, bool advance) {
    int i, j;
    bool questor = FALSE;

    msg_format("Checking quests (flags %#010x)", qe1_ptr->flags);
    message_flush();

    for (i = 0; i < MAX_Q_IDX; i++) {
        quest_type *q_ptr = &(q_list[i]);

        int next_stage = 0;
        bool partial = FALSE;

        if (!q_ptr->name) continue;

        /* Check the next possible stages */
        for (j = 0; j < MAX_QUEST_EVENTS; j++) {
            quest_event *qe2_ptr = &(q_ptr->event[j]);
            quest_event *qe3_ptr = &(q_ptr->event[QUEST_ACTION]);

            /* Not all quests advance */
            switch (q_ptr->stage) {
                /* We can succeed or fail quests at any time once the player has activated them. */
                case QUEST_ACTIVE:
                    if (j == QUEST_ACTIVE) break;
                    if (j == QUEST_FAILED) break;
                    continue;
                    /* Track what the player has done / changed in the world */
                case QUEST_ACTION: /* A quest should never get to this stage... */
                case QUEST_FINISH:
                case QUEST_PENALTY:
                    /* We don't do pay outs until the start of the next player turn */
                case QUEST_PAYOUT:
                case QUEST_FORFEIT:
                    continue;
                    /* We just advance otherwise */
                default:
                    if (q_ptr->stage != j) continue;
            }

            /* Hack -- disjunction of conditions */
            bool or = (qe2_ptr->flags & EVENT_OR) != 0;

            msg_format("Checking quest %i stage %i (flags %#010x)", i, j,
                    qe2_ptr->flags);
            message_flush();

            /* We support quests with blank transitions */
            if (qe2_ptr->flags) {
                u32b flags = qe1_ptr->flags & qe2_ptr->flags;

                /* Check for quest match */
                if (!flags) continue;

                /* Check for dungeon match */
                if (((!qe2_ptr->dungeon)
                        || (qe2_ptr->dungeon == qe1_ptr->dungeon))
                        && ((!qe2_ptr->level)
                                || (qe2_ptr->level == qe1_ptr->level))) qe3_ptr->flags |=
                        (flags & EVENT_MASK_TRAVEL);

                /* Do we have to stay on this level a set amount of time? */
                if (flags
                        & (EVENT_DEFEND_RACE | EVENT_DEFEND_FEAT
                                | EVENT_DEFEND_STORE)) {
                    if (old_turn + qe2_ptr->time > turn) continue;
                }

                /* Check for race match */
                if (flags & EVENT_MASK_RACE) {
                    /* Match any monster or specific race */
                    if ((qe2_ptr->race) && (qe2_ptr->race != qe1_ptr->race)) continue;

                    /* Have to check monster states? */
                    if ((flags & (EVENT_GIVE_RACE | EVENT_GET_RACE)) == 0) {

                        /* Hack -- we accumulate banishes and kills in the QUEST_ACTION. For others,
                         * we only check if the number provided >= the number required.
                         */
                        if (flags & (EVENT_BANISH_RACE | EVENT_KILL_RACE)) {
                            qe3_ptr->number +=  qe1_ptr->number;
                            if (qe3_ptr->number >= qe2_ptr->number) {
                                /* Add flags */
                                qe3_ptr->flags |= flags & EVENT_MASK_RACE;

                                if ((flags & EVENT_KILL_RACE) && ((flags & EVENT_MASK_ITEM) == 0)) {
                                   /* Hack -- quest drop items */
                                    q_drop_hack_art = qe2_ptr->artifact;
                                    q_drop_hack_ego = qe2_ptr->ego_item_type;
                                    q_drop_hack_kind = qe2_ptr->kind;
                                }
                            }
                        } else {
                            /* Add flags */
                            qe3_ptr->flags |= flags & EVENT_MASK_RACE;
                        }
                    } else {
                        if ((((qe2_ptr->artifact)
                                && (qe2_ptr->artifact != qe1_ptr->artifact))
                                || ((qe2_ptr->ego_item_type)
                                        && (qe2_ptr->ego_item_type
                                                != qe1_ptr->ego_item_type))
                                || ((qe2_ptr->kind)
                                        && (qe2_ptr->kind != qe1_ptr->kind)))) continue;

                        /* Add flags */
                        qe3_ptr->flags |= flags & EVENT_MASK_RACE;
                    }
                }

                /* Check for store match */
                if ((flags
                        & (EVENT_MASK_STORE | EVENT_BUY_STORE | EVENT_SELL_STORE))
                        && qe2_ptr->store && (qe2_ptr->store != qe1_ptr->store)) continue;

                /* Check for item match */
                if (flags & (EVENT_MASK_ITEM | EVENT_STOCK_STORE)) {
                    /* Match artifact, ego_item_type or kind of item or any item */
                    if ((((qe2_ptr->artifact)
                            && (qe2_ptr->artifact != qe1_ptr->artifact))
                            || ((qe2_ptr->ego_item_type)
                                    && (qe2_ptr->ego_item_type
                                            != qe1_ptr->ego_item_type))
                            || ((qe2_ptr->kind)
                                    && (qe2_ptr->kind != qe1_ptr->kind)))) continue;

                    /* XXX Paranoia around artifacts */
                    if ((qe2_ptr->artifact) && (qe2_ptr->number)) qe2_ptr->number =
                            1;

                    /* Hack -- we accumulate item destructions in the QUEST_ACTION. For others,
                     we only check if the number provided >= the number required.
                     */
                    if (flags & (EVENT_DESTROY_ITEM)) {
                        if ((qe3_ptr->number += qe1_ptr->number)
                                >= qe2_ptr->number) {
                            qe3_ptr->flags |= flags
                                    & (EVENT_MASK_ITEM | EVENT_STOCK_STORE);
                        }
                    } else {
                        qe3_ptr->flags |= flags
                                & (EVENT_MASK_ITEM | EVENT_STOCK_STORE);
                    }
                }

                /* Check for feature match */
                if (flags & EVENT_MASK_FEAT) {
                    /* Match feature or any feature */
                    if ((qe2_ptr->feat) && (qe2_ptr->feat != qe1_ptr->feat)) continue;

                    /* Accumulate features */
                    if ((qe3_ptr->number += qe1_ptr->number)
                            >= qe2_ptr->number) {
                        qe3_ptr->flags |= flags & EVENT_MASK_FEAT;
                    }
                }

                /* Check for room type match */
                if (flags & EVENT_MASK_ROOM) {
                    if (((qe2_ptr->room_type_a)
                            && (qe2_ptr->room_type_a != qe1_ptr->room_type_a))
                            || ((qe2_ptr->room_type_b)
                                    && (qe2_ptr->room_type_b
                                            != qe1_ptr->room_type_b))) continue;

                    /* Check for room flag match */
                    if (((qe1_ptr->flags & (EVENT_FLAG_ROOM | EVENT_UNFLAG_ROOM)))
                            && ((qe2_ptr->room_flags & (qe1_ptr->room_flags))
                                    == 0)) continue;

                    qe3_ptr->flags |= flags & EVENT_MASK_ROOM;
                }

                /* Do we have to succeed at a quest? */
                if ((flags & EVENT_PASS_QUEST)
                        && (q_info[qe2_ptr->quest].stage == QUEST_FINISH)) {
                    qe3_ptr->flags |= EVENT_PASS_QUEST;
                }

                /* Do we have to fail a quest? */
                if ((flags & EVENT_FAIL_QUEST)
                        && (q_info[qe2_ptr->quest].stage == QUEST_PENALTY)) {
                    qe3_ptr->flags |= EVENT_FAIL_QUEST;
                }

                /* Check for defensive failure */
                if (qe1_ptr->flags & (EVENT_DEFEND_RACE | EVENT_DEFEND_FEAT)) {
                    /* Hack - fail the quest */
                    if (!qe1_ptr->number) {
                        next_stage = QUEST_FAILED;
                        break;
                    }
                }
            }

            /* We have qualified for the next stage of the quest */
            if (or ?
                    (0 != qe3_ptr->flags) :
                    (qe2_ptr->flags == qe3_ptr->flags)) {
                next_stage = (j == QUEST_ACTIVE ? QUEST_REWARD : j + 1);
                msg_format("Quest %i advanced (%s) to stage %i", i,
                        or ? "OR" : "AND", next_stage);
                message_flush();
                anykey();
            }
        }

        /* Advance the quest */
        if (next_stage) {
            const char *prefix =
                    (next_stage == QUEST_REWARD ?
                            "To claim your reward, " : NULL);

            /* Not advancing */
            if (!advance) {
                /* Voluntarily fail quest? */
                if (next_stage == QUEST_FORFEIT) {
                    return (get_check(format("Fail %s?", q_name + q_ptr->name)));
                } else {
                    return (FALSE);
                }
            }

            /* Describe the quest if assigned it */
            if ((next_stage == QUEST_ACTIVE) && q_ptr->text) msg_format("%s",
                    q_text + q_ptr->text);

            /* Tell the player the next step of the quest. */
            if ((next_stage < QUEST_PAYOUT)
                    && (q_info[i].event[next_stage].flags)) {
                /* Display the event 'You must...' */
                /* print_event(&q_info[i].event[next_stage], 2, 3, prefix); */
            }
            /* Tell the player they have succeeded */
            else if (next_stage == QUEST_PAYOUT) {
                msg_format("You have completed %s!", q_name + q_ptr->name);
                quest_res_queued = TRUE;

                /* Hack -- Completing quest 0 finishes the game*/
                if (!i) {
                    /* Total winner */
                    p_ptr->total_winner = TRUE;

                    /* Redraw the "title" */
                    p_ptr->redraw |= (PR_TITLE);

                    /* Congratulations */
                    msg_print("*** CONGRATULATIONS ***");
                    msg_print("You have won the game!");
                    msg_print(
                            "You may retire (commit suicide) when you are ready.");
                }
            }
            /* Tell the player they have failed */
            else if (next_stage == QUEST_FORFEIT) {
                msg_format("You have failed %s.", q_name + q_ptr->name);
                quest_res_queued = TRUE;
            }

            /* Advance quest to the next stage */
            q_ptr->stage = next_stage;
            WIPE(&q_ptr->event[QUEST_ACTION], quest_event);

            /* Something done? */
            questor = TRUE;
        }
        /* Tell the player they have partially advanced the quest */
        else if (partial) {
            quest_event event;

            COPY(&event, &q_ptr->event[q_ptr->stage], quest_event);

            /* Tell the player the action they have completed */
            print_event(qe1_ptr, 2, 1, NULL);

            /* Tell the player the action(s) they must still do */
            print_event(&event, 2, 3, NULL);
        }
    }

    return (questor);
}

/*
 *  Check how many monsters of a particular race are affected by a state
 *  that a quest requires.
 */
void check_monster_quest(int m_idx, bool (*questor_test_hook)(int m_idx),
        u32b event) {
    int i;
    int k = 0;
    int r_idx = m_idx ? m_list[m_idx].r_idx : 0;

    quest_event qe;

    for (i = 0; i < z_info->m_max; i++) {
        monster_type *m_ptr = &m_list[i];

        /* Skip dead monsters */
        if (!m_ptr->r_idx) continue;

        /* Check any monster race, if necessary */
        if ((r_idx) && !(m_ptr->r_idx != r_idx)) continue;

        /* Check for state */
        if (!questor_test_hook(m_idx)) continue;

        /* Accumulate count */
        k++;
    }

    if (!k) return;

    WIPE(&qe, quest_event);

    qe.flags = event;
    qe.number = k;

    /* Check for quest completion */
    while (check_quest(&qe, TRUE))
        ;
}

/*
 * Apply instantaneous quest effect (QUEST_PAYOUT, QUEST_FORFEIT)
 */

bool do_quest_resolution(void) {
    int i;
    bool ret = FALSE;

    /* Hack -- efficiency */
    if (!quest_res_queued) return FALSE;
    quest_res_queued = FALSE;

    for (i = 0; i < MAX_Q_IDX; i++) {
        quest_type *q_ptr = &(q_list[i]);
        quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);
        u32b flags = qe_ptr->flags & ~(EVENT_OR);

        if ((q_ptr->stage != QUEST_PAYOUT) && (q_ptr->stage != QUEST_FORFEIT)) continue;

        p_ptr->au += qe_ptr->gold;
        gain_exp(qe_ptr->experience);

        if (flags & EVENT_TRAVEL) {
            quest_event event;

            /* Use this to allow quests to succeed or fail */
            WIPE(&event, quest_event);

            /* Set up departure event */
            event.flags = EVENT_LEAVE;
            event.dungeon = p_ptr->dungeon;
            event.level = p_ptr->depth - min_depth(p_ptr->dungeon);

            /* Check for quest failure. */
            while (check_quest(&event, TRUE))
                ;

            /* Save the old dungeon in case something goes wrong */
            if (autosave_backup)
            do_cmd_save_bkp();

            /* Hack -- take a turn */
            p_ptr->energy_use = 100;

            /* Clear stairs */
            p_ptr->create_stair = 0;

            if (qe_ptr->dungeon) {
                /* Teleporting to a new dungeon level */
                p_ptr->dungeon = qe_ptr->dungeon;
                p_ptr->depth = min_depth(qe_ptr->dungeon) + qe_ptr->level;
            } else {
                /* Just jumping between floors */
                p_ptr->depth = min_depth(p_ptr->dungeon) + qe_ptr->level;

                /* Sanity checks */
                if (p_ptr->depth < min_depth(p_ptr->dungeon)) p_ptr->depth =
                        min_depth(p_ptr->dungeon);
                else if (p_ptr->depth > max_depth(p_ptr->dungeon)) p_ptr->depth =
                        max_depth(p_ptr->dungeon);
            }
            /* Leaving */
            p_ptr->leaving = TRUE;
        }

        if (flags & EVENT_PASS_QUEST) {
            quest_type *q1_ptr = &(q_list[qe_ptr->quest]);

            /* Complete quest if not already resolved */
            if (q1_ptr->stage < QUEST_PAYOUT) {
                q1_ptr->stage = QUEST_PAYOUT;
            }
        } else if (flags & EVENT_FAIL_QUEST) {
            quest_type *q1_ptr = &(q_list[qe_ptr->quest]);

            /* Complete quest if not already resolved */
            if (q1_ptr->stage < QUEST_PAYOUT) {
                q1_ptr->stage = QUEST_FORFEIT;
            }
        }

        if (flags & (EVENT_GET_ITEM | EVENT_STOCK_STORE)) {
            object_type object_type_body;
            object_type *i_ptr;
            int k_idx, tvidx;

            /* Set pointer */
            i_ptr = &object_type_body;
            object_wipe(i_ptr);

            if (qe_ptr->artifact) {
                artifact_type *a_ptr = &(a_info[qe_ptr->artifact]);

                if (a_ptr->cur_num > 0) continue;

                /* Set pointer */
                i_ptr = &object_type_body;

                /* Get base item */
                k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

                /* Prep the object */
                object_prep(i_ptr, k_idx);

                /* Set as artifact */
                i_ptr->name1 = qe_ptr->artifact;

            } else if (qe_ptr->ego_item_type) {
                ego_item_type *e_ptr = &(e_info[qe_ptr->ego_item_type]);

                /* Set pointer */
                i_ptr = &object_type_body;

                /* Get base item */
                if(q_drop_hack_kind) {
                    k_idx = q_drop_hack_kind;
                } else {
                    do {
                        tvidx = rand_int(3);
                    } while(!e_ptr->tval[tvidx]);
                    do {
                        k_idx = lookup_kind(e_ptr->tval[tvidx],
                            rand_range(e_ptr->min_sval[tvidx],
                                    e_ptr->max_sval[tvidx]));
                    } while(!k_idx);
                }

                /* Prep the object */
                object_prep(i_ptr, k_idx);

                /* Set as ego item */
                i_ptr->name2 = qe_ptr->ego_item_type;

            } else if (qe_ptr->kind) {
                /* Prep the object */
                object_prep(i_ptr, qe_ptr->kind);
            }

            /* Apply magic attributes */
            apply_magic(i_ptr, qe_ptr->power, FALSE, TRUE, TRUE);

            /* Get origin */
            i_ptr->origin = ORIGIN_STORE_REWARD;

            /* Its now safe to identify quest rewards */
            object_aware(i_ptr, TRUE);
            object_known(i_ptr);

            if (flags & EVENT_GET_ITEM) {
                /* Gain the item */
                (void) inven_carry(i_ptr);
            } else {
                /* Stock the item */
                store_carry(i_ptr, qe_ptr->store);
            }
        } else if (flags & (EVENT_DESTROY_ITEM | EVENT_LOSE_ITEM)) {
            int i, j;
            object_type *o_ptr;

            j = qe_ptr->number ? qe_ptr->number : 1024;

            /* Iterate through the inventory*/
            for (i = 0; i < INVEN_TOTAL; i++) {
                o_ptr = &inventory[i];

                /* Require the artifact if specified */
                if (qe_ptr->artifact && (qe_ptr->artifact != o_ptr->name1)) continue;

                /* Require the ego item if specified */
                if (qe_ptr->ego_item_type
                        && (qe_ptr->ego_item_type != o_ptr->name2)) continue;

                /* Require the item kind if specified */
                if (qe_ptr->kind && (qe_ptr->kind != o_ptr->k_idx)) continue;

                if (j <= o_ptr->number) {
                    if (j == o_ptr->number) inven_drop_flags(o_ptr);

                    inven_item_increase(i, -j);
                    inven_item_describe(i);
                    inven_item_optimize(i);
                    break;
                }

                j -= o_ptr->number;

                inven_drop_flags(o_ptr);

                inven_item_increase(i, -o_ptr->number);
                inven_item_describe(i);
                inven_item_optimize(i);
            }

            if (qe_ptr->artifact && (qe_ptr->flags & EVENT_DESTROY_ITEM)) {
                a_info[qe_ptr->artifact].cur_num = 1;
            }
        }

        if (flags & EVENT_FIND_RACE) {
            int x, y;

            /* Try 10 times */
            for (i = 0; i < 10; i++) {
                int d = 5;

                /* Pick a location */
                scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);

                /* Require empty grids */
                if (!cave_empty_bold(y, x)) continue;

                /* Place it (allow groups) */
                if (place_monster_aux(y, x, qe_ptr->race, FALSE, TRUE, 0L)) break;
            }
        } else if (flags & EVENT_KILL_RACE) {
            monster_race *r_ptr = &r_info[qe_ptr->race];
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    bool fear = FALSE;
                    mon_take_hit(i, 0x7FFF, &fear, " falls dead. ");
                    if (!--j) break;
                }
            }

            if ((r_ptr->flags1 & RF1_UNIQUE) && r_ptr->max_num) {
                char m_name[80];
                /* Extract monster name */
                monster_desc(m_name, sizeof(m_name), qe_ptr->race, 0);
                message_format(MSG_KILL, qe_ptr->race, "%^s%s", m_name,
                        "'s reign is ended. ");
                r_ptr->max_num = 0;
                gain_exp(
                        ((long) r_ptr->mexp * (10 + r_ptr->level))
                                / (10 + p_ptr->max_lev));
            }
        } else if (flags & EVENT_ALLY_RACE) {
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    /* Befriend the monster */
                    m_list[i].mflag |= MFLAG_ALLY;
                    if (!--j) break;
                }
            }
        } else if (flags & EVENT_HATE_RACE) {
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    /* Enrage the monster */
                    m_list[i].mflag |= MFLAG_AGGR;
                    if (!--j) break;
                }
            }
        } else if (flags & EVENT_FEAR_RACE) {
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    /* Terrify the monster */
                    set_monster_fear(&m_list[i], qe_ptr->power, TRUE);
                    if (!--j) break;
                }
            }
        } else if (flags & EVENT_HEAL_RACE) {
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    /* Heal the monster */
                    m_list[i].hp += qe_ptr->power;
                    if (!--j) break;
                }
            }
        } else if (flags & EVENT_BANISH_RACE) {
            int i, j;

            j = qe_ptr->number;

            for (i = 0; i < m_cnt; i++) {
                if (m_list[i].r_idx == qe_ptr->race) {
                    /* Delete the monster */
                    delete_monster_idx(i);
                    if (!--j) break;
                }
            }
        }

        if (flags) check_quest(qe_ptr, TRUE);

        /* Advance to the lasting effect */
        q_ptr->stage++;
        ret = TRUE;
    }
    return ret;
}

/*
 * Apply lasting quest effect (QUEST_FINISH, QUEST_PENALTY)
 */

bool apply_quest_finish(void) {
    int i;
    bool ret = FALSE;

    for (i = 0; i < MAX_Q_IDX; i++) {
        quest_type *q_ptr = &(q_list[i]);
        quest_event *qe_ptr = &(q_ptr->event[q_ptr->stage]);
        u32b flags = qe_ptr->flags & ~(EVENT_OR);

        if ((q_ptr->stage != QUEST_FINISH) && (q_ptr->stage != QUEST_PENALTY)) continue;

        if (q_ptr->resolved) continue;

        if (flags & EVENT_STOCK_STORE) {
            /* Permanently stock an item */
            store_type_ptr qstore = store[qe_ptr->store];
            for (i = 0; i < STORE_CHOICES; i++) {
                if (!qstore->count[i] || (i == STORE_CHOICES - 1)) {
                    qstore->tval[i] = k_info[qe_ptr->kind].tval;
                    qstore->sval[i] = k_info[qe_ptr->kind].sval;
                    qstore->count[i] = qe_ptr->power;
                    break;
                }
            }
        }

        if (flags) check_quest(qe_ptr, TRUE);
        q_ptr->resolved = TRUE;
        ret = TRUE;
    }
    return ret;
}

/*
 * Array of event pronouns strings - initial caps
 */
static cptr event_pronoun_text_caps[6] = { "We ", "I ", "You ", "He ", "She ",
        "It " };

/*
 * Array of event pronouns strings - comma
 */
static cptr event_pronoun_text[6] =
        { "we ", "I ", "you ", "he ", "she ", "it " };

/*
 * Array of event pronouns strings - comma
 */
static cptr event_pronoun_text_which[6] = { "which we ", "which I ",
        "which you ", "which he ", "which she ", "which it " };

/*
 * Array of tense strings (0, 1 and 2 occurred in past, however 0, 1 only are past tense )
 *
 */
static cptr event_tense_text[8] = { " ", "have ", "had to ", "must ", "will ",
        " ", "can ", "may " };

/*
 * Display a quest event.
 */
bool print_event(quest_event *event, int pronoun, int tense, cptr prefix) {
    int vn, n;
    cptr vp[64];

    bool reflex_feature = FALSE;
    bool reflex_monster = FALSE;
    bool reflex_object = FALSE;
    bool used_num = FALSE;
    bool used_race = FALSE;
    bool output = FALSE;
    bool intro = FALSE;

    if (prefix) {
        text_out(prefix);
        intro = TRUE;
    }

    /* Describe location */
    if ((event->dungeon) || (event->level)) {
        /* Collect travel actions */
        vn = 0;
        if (tense > 1) {
            if (event->flags & (EVENT_TRAVEL)) vp[vn++] = "travel to";
            if (event->flags & (EVENT_LEAVE)) vp[vn++] = "leave";
        } else {
            if (event->flags & (EVENT_TRAVEL)) vp[vn++] = "travelled to";
            if (event->flags & (EVENT_LEAVE)) vp[vn++] = "left";
        }

        /* Describe travel actions */
        if (vn) /* vn = 0 | 1 */
        {
            if (intro) {
                text_out(event_pronoun_text[pronoun]);
            } else {
                text_out(event_pronoun_text_caps[pronoun]);
                intro = TRUE;
            }
            text_out(event_tense_text[tense]);

            /* Dump */
            text_out(vp[0]);

            text_out(" ");
        }

        if (event->level) {
            if (!intro) text_out("On ");
            text_out(format("level %d of ", event->level));
        } else if (!vn) {
            if (intro) text_out(" in ");
            else text_out("In ");
        }

        /* TODO: full level 0 name? level n name? */
        text_out(t_name + t_info[event->dungeon].name);

        if ((event->owner) || (event->store)
                || ((event->feat) && (event->action)) || (event->race)
                || (event->kind) || (event->ego_item_type)
                || (event->artifact)) {
            if (!vn) {
                text_out(event_pronoun_text[pronoun]);
                text_out(event_tense_text[tense]);
            } else text_out(" and ");
        }

        output = TRUE;
    } else {
        if (intro) {
            text_out(event_pronoun_text[pronoun]);
        } else {
            text_out(event_pronoun_text_caps[pronoun]);
        }
        text_out(event_tense_text[tense]);
    }

    /* Visit a store */
    if ((event->owner) || (event->store)
            || (event->flags & (EVENT_TALK_STORE | EVENT_DEFEND_STORE))) {
        output = TRUE;

        if (event->flags & (EVENT_DEFEND_STORE)) {
            if (tense < 0) text_out("defended ");
            else text_out("defend ");

            used_race = TRUE;
        } else if (event->flags & (EVENT_TALK_STORE)) {
            if (tense < 0) text_out("talked to");
            else text_out("talk to");
        } else if (tense < 0) text_out("visited ");
        else text_out("visit ");

        if (!(event->store)) {
            text_out("the town");
        }

        if (event->store) {
            text_out(f_name + f_info[event->feat].name);
        }

        if ((event->flags & (EVENT_DEFEND_STORE)) && !(event->room_type_a)
                && !(event->room_type_b) && !(event->room_flags)
                && !((event->feat) && (event->action))) {
            text_out(" from ");
            reflex_monster = TRUE;
        } else if (((event->feat) && (event->action)) || (event->room_type_a)
                || (event->room_type_b) || (event->room_flags) || (event->race)
                || ((event->kind) || (event->ego_item_type) || (event->artifact))) {
            text_out(" and ");
        } else text_out(" ");

        output = TRUE;
    }

    /* Affect room */
    if ((event->room_type_a) || (event->room_type_b) || (event->room_flags)) {
        /* Collect room flags */
        vn = 0;
        if (tense > 1) {
            if (event->flags & (EVENT_FIND_ROOM)) vp[vn++] = "find";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_HIDDEN))) vp[vn++] = "reveal";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_SEALED))) vp[vn++] = "unseal";
            if ((event->flags & (EVENT_FLAG_ROOM))
                    && (event->room_flags & (ROOM_ENTERED))) vp[vn++] = "enter";
#if 0
            if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_LITE))) vp[vn++] = "light up";
#endif
            if ((event->flags & (EVENT_FLAG_ROOM))
                    && (event->room_flags & (ROOM_SEEN))) vp[vn++] = "explore";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_LAIR))) vp[vn++] =
                    "clear of monsters";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_OBJECT))) vp[vn++] =
                    "empty of objects";
#if 0
            if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_TRAP))) vp[vn++] = "disarm";
            if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_DARK))) vp[vn++] = "darken";
#endif
        } else {
            if (event->flags & (EVENT_FIND_ROOM)) vp[vn++] = "found";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_HIDDEN))) vp[vn++] =
                    "revealed";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_SEALED))) vp[vn++] =
                    "unsealed";
            if ((event->flags & (EVENT_FLAG_ROOM))
                    && (event->room_flags & (ROOM_ENTERED))) vp[vn++] =
                    "entered";
#if 0
            if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_LITE))) vp[vn++] = "lit up";
#endif
            if ((event->flags & (EVENT_FLAG_ROOM))
                    && (event->room_flags & (ROOM_SEEN))) vp[vn++] = "explored";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_LAIR))) vp[vn++] =
                    "cleared of monsters";
            if ((event->flags & (EVENT_UNFLAG_ROOM))
                    && (event->room_flags & (ROOM_OBJECT))) vp[vn++] =
                    "emptied of objects";
#if 0
            if ((event->flags & (EVENT_UNFLAG_ROOM)) && (event->room_flags & (ROOM_TRAP))) vp[vn++] = "disarmed";
            if ((event->flags & (EVENT_FLAG_ROOM)) && (event->room_flags & (ROOM_DARK))) vp[vn++] = "darkened";
#endif
        }

        if (vn) {
            output = TRUE;

            /* Scan */
            for (n = 0; n < vn; n++) {
                /* Intro */
                if (n == 0) ;
                else if (n < vn - 1) text_out(", ");
                else text_out(" or ");

                /* Dump */
                text_out(vp[n]);
            }

            text_out(" a ");

            /* Room adjective */
            if (event->room_type_a) {
                text_out(d_name + d_info[event->room_type_a].name1);
                text_out(" ");
            }

            /* Room noun */
            if (!(event->room_type_a) && !(event->room_type_b)) text_out(
                    "all the rooms");
            else if (event->room_type_b) text_out(
                    d_name + d_info[event->room_type_b].name1);
            else text_out("room");

            if (((event->feat) && (event->action)) || (event->race)
                    || ((event->kind) || (event->ego_item_type)
                            || (event->artifact))) {
                if ((event->flags & (EVENT_UNFLAG_ROOM))
                        && (event->room_flags & (ROOM_SEALED))
                        && (event->room_flags & (ROOM_HIDDEN))) text_out(
                        " hidden and sealed by ");
                else if ((event->flags & (EVENT_UNFLAG_ROOM))
                        && (event->room_flags & (ROOM_SEALED))) text_out(
                        " sealed by ");
                else if ((event->flags & (EVENT_UNFLAG_ROOM))
                        && (event->room_flags & (ROOM_HIDDEN))) text_out(
                        " hidden by ");
                else if ((event->flags & (EVENT_UNFLAG_ROOM))
                        || (event->flags & (EVENT_FLAG_ROOM))) text_out(
                        " enchanted by ");
                else if ((event->feat) && (event->action)) {
                    text_out(" containing ");

                    reflex_feature = TRUE;
                } else if (event->race) {
                    text_out(" guarded by ");

                    reflex_monster = TRUE;
                } else if ((event->kind) || (event->ego_item_type)
                        || (event->artifact)) {
                    text_out(" containing ");

                    reflex_object = TRUE;
                }

                if (event->race) used_race = TRUE;
            }
        }
    }

    /* Alter a feature */
    if ((event->feat) && (event->action)) {
        /* Collect monster actions */
        vn = 0;
        if (tense > 1) {
            switch (event->action) {
                case FS_SECRET:
                    vp[vn++] = "find";
                    break;
                case FS_OPEN:
                    vp[vn++] = "open";
                    break;
                case FS_CLOSE:
                    vp[vn++] = "close";
                    break;
                case FS_BASH:
                    vp[vn++] = "bash";
                    break;
                case FS_SPIKE:
                    vp[vn++] = "spike";
                    break;
                case FS_DISARM:
                    vp[vn++] = "disarm";
                    break;
                case FS_TUNNEL:
                    vp[vn++] = "dig through";
                    break;
                case FS_HIT_TRAP:
                    vp[vn++] = "set off";
                    break;
                case FS_HURT_ROCK:
                    vp[vn++] = "turn to mud";
                    break;
                case FS_HURT_FIRE:
                    vp[vn++] = "burn";
                    break;
                case FS_HURT_COLD:
                    vp[vn++] = "flood";
                    break;
                case FS_HURT_ACID:
                    vp[vn++] = "dissolve";
                    break;
                case FS_KILL_MOVE:
                    vp[vn++] = "step on";
                    break;
                case FS_HURT_POIS:
                    vp[vn++] = "poison";
                    break;
                case FS_HURT_ELEC:
                    vp[vn++] = "electrify";
                    break;
                case FS_HURT_WATER:
                    vp[vn++] = "flood";
                    break;
                case FS_HURT_BWATER:
                    vp[vn++] = "boil";
                    break;
            }
        } else {
            switch (event->action) {
                case FS_SECRET:
                    vp[vn++] = "found";
                    break;
                case FS_OPEN:
                    vp[vn++] = "opened";
                    break;
                case FS_CLOSE:
                    vp[vn++] = "closed";
                    break;
                case FS_BASH:
                    vp[vn++] = "bashed";
                    break;
                case FS_SPIKE:
                    vp[vn++] = "spiked";
                    break;
                case FS_DISARM:
                    vp[vn++] = "disarmed";
                    break;
                case FS_TUNNEL:
                    vp[vn++] = "dug through";
                    break;
                case FS_HIT_TRAP:
                    vp[vn++] = "set off";
                    break;
                case FS_HURT_ROCK:
                    vp[vn++] = "turned to mud";
                    break;
                case FS_HURT_FIRE:
                    vp[vn++] = "burnt";
                    break;
                case FS_HURT_COLD:
                    vp[vn++] = "flooded";
                    break;
                case FS_HURT_ACID:
                    vp[vn++] = "dissolved";
                    break;
                case FS_KILL_MOVE:
                    vp[vn++] = "stepped on";
                    break;
                case FS_HURT_POIS:
                    vp[vn++] = "poisoned";
                    break;
                case FS_HURT_ELEC:
                    vp[vn++] = "electrified";
                    break;
                case FS_HURT_WATER:
                    vp[vn++] = "flooded";
                    break;
                case FS_HURT_BWATER:
                    vp[vn++] = "boiled";
                    break;
            }
        }

        /* Introduce feature */
        if (vn) {
            output = TRUE;

            if (reflex_feature) {
                if ((!used_num) && (event->number > 0)) text_out(
                        format("%d ", event->number));

                text_out(f_name + f_info[event->feat].name);
                if ((!used_num) && (event->number > 0)) text_out("s");

                text_out(event_pronoun_text_which[pronoun]);
                text_out(event_tense_text[tense]);

                used_num = TRUE;
            }

            /* Scan */
            for (n = 0; n < vn; n++) {
                /* Intro */
                if (n == 0) ;
                else if (n < vn - 1) text_out(", ");
                else text_out(" or ");

                /* Dump */
                text_out(vp[n]);
            }

            if (!(reflex_feature)) {
                if ((!used_num) && (event->number > 0)) text_out(
                        format(" %d", event->number));

                text_out(" ");
                text_out(f_name + f_info[event->feat].name);
                if ((!used_num) && (event->number > 0)) text_out("s");

                used_num = TRUE;
            }

            if ((event->race) || (event->kind) || (event->ego_item_type)
                    || (event->artifact)) {
                if (reflex_feature) {
                    text_out(".  ");
                    intro = FALSE;
                    reflex_monster = FALSE;
                    reflex_object = FALSE;
                } else if (event->race) {
                    text_out(" guarded by ");
                    used_race = TRUE;
                } else text_out(" and ");
            }
        }
    }

    /* Kill a monster */
    if ((event->race)
            && ((used_race)
                    || !((event->kind) || (event->ego_item_type)
                            || (event->artifact)))) {
        used_race = TRUE;

        /* Collect monster actions */
        vn = 0;
        if (tense > 1) {
            if (event->flags & (EVENT_FIND_RACE)) vp[vn++] = "find";
            if (event->flags & (EVENT_TALK_RACE)) vp[vn++] = "talk to";
            if (event->flags & (EVENT_ALLY_RACE)) vp[vn++] = "befriend";
            if (event->flags & (EVENT_DEFEND_RACE)) vp[vn++] = "defend";
            if (event->flags & (EVENT_HATE_RACE)) vp[vn++] = "offend";
            if (event->flags & (EVENT_FEAR_RACE)) vp[vn++] = "terrify";
            if (event->flags & (EVENT_HEAL_RACE)) vp[vn++] = "heal";
            if (event->flags & (EVENT_BANISH_RACE)) vp[vn++] = "banish";
            if (event->flags & (EVENT_KILL_RACE | EVENT_DEFEND_STORE)) vp[vn++] =
                    "kill";
        } else {
            if (event->flags & (EVENT_FIND_RACE)) vp[vn++] = "found";
            if (event->flags & (EVENT_TALK_RACE)) vp[vn++] = "talked to";
            if (event->flags & (EVENT_ALLY_RACE)) vp[vn++] = "befriended";
            if (event->flags & (EVENT_DEFEND_RACE)) vp[vn++] = "defended";
            if (event->flags & (EVENT_HATE_RACE)) vp[vn++] = "offended";
            if (event->flags & (EVENT_FEAR_RACE)) vp[vn++] = "terrified";
            if (event->flags & (EVENT_HEAL_RACE)) vp[vn++] = "healed";
            if (event->flags & (EVENT_BANISH_RACE)) vp[vn++] = "banished";
            if (event->flags & (EVENT_KILL_RACE | EVENT_DEFEND_STORE)) vp[vn++] =
                    "killed";
        }

        /* Hack -- monster race */
        if (vn) {
            char m_name[80];
            output = TRUE;

            if (reflex_monster) {
                /* Get the name */
                race_desc(m_name, sizeof(m_name), event->race,
                        (used_num ? 0x400 : 0x08), event->number);

                text_out(m_name);

                text_out(event_pronoun_text_which[pronoun]);
                text_out(event_tense_text[tense]);

                used_num = TRUE;
            }

            /* Scan */
            for (n = 0; n < vn; n++) {
                /* Intro */
                if (n == 0) ;
                else if (n < vn - 1) text_out(", ");
                else text_out(" or ");

                /* Dump */
                text_out(vp[n]);
            }

            if (!(reflex_monster)) {
                /* Get the name */
                race_desc(m_name, sizeof(m_name), event->race,
                        (used_num ? 0x400 : 0x08), event->number);

                text_out(" ");
                text_out(m_name);

                used_num = TRUE;
            }

            if ((event->kind) || (event->ego_item_type) || (event->artifact)) {
                if (reflex_monster) {
                    text_out(".  ");
                    intro = FALSE;
                    prefix = NULL;
                    reflex_object = FALSE;
                } else if (event->number > 1) text_out(" and each carrying ");
                else text_out(" and carrying ");
            }
        }
    }

    /* Collect an object */
    if ((event->kind) || (event->ego_item_type) || (event->artifact)) {
        object_type o_temp;
        char o_name[140];

        /* Create temporary object */
        if (event->artifact) {
            object_prep(&o_temp,
                    lookup_kind(a_info[event->artifact].tval,
                            a_info[event->artifact].sval));

            o_temp.name1 = event->artifact;
        } else if (event->ego_item_type) {
            if (event->kind) {
                object_prep(&o_temp, event->kind);
            } else {
                object_prep(&o_temp,
                        lookup_kind(e_info[event->ego_item_type].tval[0],
                                e_info[event->ego_item_type].min_sval[0]));
            }

            o_temp.name2 = event->ego_item_type;
        } else {
            object_prep(&o_temp, event->kind);
        }

        if (!(used_num) && !(event->artifact)) o_temp.number = event->number;
        else o_temp.number = 1;

        if (!(used_race)) o_temp.name3 = event->race;

        o_temp.ident |= (IDENT_KNOWN);

        /* Describe object */
        object_desc(o_name, sizeof(o_name), &o_temp, TRUE, 0);

        /* Collect store actions */
        vn = 0;
        if (tense > 1) {
            if (event->flags & (EVENT_FIND_ITEM)) vp[vn++] = "find";
            if (event->flags & (EVENT_BUY_STORE)) vp[vn++] = "buy";
            if (event->flags & (EVENT_SELL_STORE)) vp[vn++] = "sell";
            if (event->flags & (EVENT_GET_ITEM)) vp[vn++] = "get";
            if (event->race) vp[vn++] = "collect";
            if (event->flags & (EVENT_GIVE_STORE | EVENT_GIVE_RACE)) vp[vn++] =
                    "give";
            if (event->flags & (EVENT_GET_STORE | EVENT_GET_RACE)) vp[vn++] =
                    "be given";
            if (event->flags & (EVENT_DEFEND_STORE)) vp[vn++] = "use";
            if (event->flags & (EVENT_LOSE_ITEM)) vp[vn++] = "lose";
            if (event->flags & (EVENT_DESTROY_ITEM)) vp[vn++] = "destroy";
        } else {
            if (event->flags & (EVENT_FIND_ITEM)) vp[vn++] = "found";
            if (event->flags & (EVENT_BUY_STORE)) vp[vn++] = "bought";
            if (event->flags & (EVENT_SELL_STORE)) vp[vn++] = "sold";
            if (event->flags & (EVENT_GET_ITEM)) vp[vn++] = "kept";
            if (event->race) vp[vn++] = "collected";
            if (event->flags & (EVENT_GIVE_STORE | EVENT_GIVE_STORE)) vp[vn++] =
                    "gave";
            if (event->flags & (EVENT_GET_STORE | EVENT_GET_RACE)) vp[vn++] =
                    "were given";
            if (event->flags & (EVENT_DEFEND_STORE)) vp[vn++] = "used";
            if (event->flags & (EVENT_LOSE_ITEM)) vp[vn++] = "lost";
            if (event->flags & (EVENT_DESTROY_ITEM)) vp[vn++] = "destroyed";
        }

        if (vn) {
            output = TRUE;

            if (event->flags & (EVENT_STOCK_STORE)) {
                if (intro) text_out("he ");
                else text_out("He ");

                if (tense > 1) text_out("will stock ");
                else if (tense < 0) text_out("stocked ");
                else text_out("stocks ");

                reflex_object = TRUE;
            }

            if (reflex_object) {
                text_out(o_name);

                used_num = TRUE;

                text_out(event_pronoun_text_which[pronoun]);
                if ((event->flags & (EVENT_GET_STORE | EVENT_GET_ITEM))
                        && (tense > 1)) text_out("will ");
                else text_out(event_tense_text[tense]);
            }

            /* Scan */
            for (n = 0; n < vn; n++) {
                /* Intro */
                if (n == 0) ;
                else if (n < vn - 1) text_out(", ");
                else text_out(" or ");

                /* Dump */
                text_out(vp[n]);
            }

            if (!(reflex_object)) {
                text_out(" ");
                text_out(o_name);
            }
        }
    }

    /* Collect rewards */
    if ((event->gold) || (event->experience)) {
        output = TRUE;

        {
            text_out(" and ");
        }

        if ((event->gold > 0) || (event->experience > 0)) {
            text_out("gain ");

            if (event->gold > 0) {
                text_out(format("%d gold", event->gold));
                if (event->experience) text_out(" and ");
            }

            if (event->experience > 0) {
                text_out(format("%d experience", event->experience));
                if (event->gold < 0) text_out(" and ");
            }
        }

        if ((event->gold < 0) || (event->experience < 0)) {
            text_out("cost ");

            if (event->gold < 0) {
                text_out(format("%d gold", -event->gold));
                if (event->experience < 0) text_out(" and ");
            }

            if (event->experience < 0) {
                text_out(format("%d experience", -event->experience));
            }
        }
    }

    return (output);

}
