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
                anykey();

    for (i = 0; i < MAX_Q_IDX; i++) {
        quest_type *q_ptr = &(q_list[i]);

        int next_stage = 0;
        bool partial = FALSE;

        if(!q_ptr->name) continue;

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
            anykey();

            /* We support quests with blank transitions */
            if (qe2_ptr->flags) {
                u32b flags = qe1_ptr->flags & qe2_ptr->flags;

                /* Check for quest match */
                if (!flags) continue;

                /* Check for dungeon match */
                if (((!qe2_ptr->dungeon) || (qe2_ptr->dungeon == qe1_ptr->dungeon))
                        && ((!qe2_ptr->level) || (qe2_ptr->level == qe1_ptr->level)))
                    qe3_ptr->flags |= (flags & EVENT_MASK_TRAVEL);

                /* Do we have to stay on this level a set amount of time? */
                if (flags & (EVENT_DEFEND_RACE | EVENT_DEFEND_FEAT
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
                        if (flags & (EVENT_BANISH_RACE | EVENT_KILL_RACE))
                            qe3_ptr->number += qe1_ptr->number;

                        if ((qe1_ptr->number >= qe2_ptr->number) || (qe3_ptr->number >= qe2_ptr->number)) {
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
                                    && (qe2_ptr->kind != qe1_ptr->kind))))
                            continue;

                        /* Add flags */
                        qe3_ptr->flags |= flags & EVENT_MASK_RACE;
                    }
                }

                /* Check for store match */
                if ((flags & (EVENT_MASK_STORE | EVENT_BUY_STORE | EVENT_SELL_STORE))
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
                    if ((qe2_ptr->artifact) && (qe2_ptr->number)) qe2_ptr->number = 1;

                    /* Hack -- we accumulate item destructions in the QUEST_ACTION. For others,
                     we only check if the number provided >= the number required.
                     */
                    if (flags & (EVENT_DESTROY_ITEM)) {
                        if((qe3_ptr->number += qe1_ptr->number) >= qe2_ptr->number) {
                            qe3_ptr->flags |= flags & (EVENT_MASK_ITEM | EVENT_STOCK_STORE);
                        }
                    } else {
                        qe3_ptr->flags |= flags & (EVENT_MASK_ITEM | EVENT_STOCK_STORE);
                    }
                }

                /* Check for feature match */
                if (flags & EVENT_MASK_FEAT) {
                    /* Match feature or any feature */
                    if ((qe2_ptr->feat) && (qe2_ptr->feat != qe1_ptr->feat)) continue;

                    /* Accumulate features */
                    if((qe3_ptr->number += qe1_ptr->number) >= qe2_ptr->number) {
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
                            && ((qe2_ptr->room_flags & (qe1_ptr->room_flags)) == 0)) continue;

                    qe3_ptr->flags |= flags & EVENT_MASK_ROOM;
                }


                /* Do we have to succeed at a quest? */
                if ((flags & EVENT_PASS_QUEST) && (q_info[qe2_ptr->quest].stage == QUEST_FINISH)) {
                    qe3_ptr->flags |= EVENT_PASS_QUEST;
                }

                /* Do we have to fail a quest? */
                if ((flags & EVENT_FAIL_QUEST) && (q_info[qe2_ptr->quest].stage == QUEST_PENALTY)) {
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
            if (or ? (0 != qe3_ptr->flags) : (qe2_ptr->flags == qe3_ptr->flags)) {
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
            if ((next_stage == QUEST_ACTIVE) && q_ptr->text) msg_format(
                    "%s", q_text + q_ptr->text);

            /* Tell the player the next step of the quest. */
            if ((next_stage < QUEST_PAYOUT)
                    && (q_info[i].event[next_stage].flags)) {
                /* Display the event 'You must...' */
                //print_event(&q_info[i].event[next_stage], 2, 3, prefix);
            }
            /* Tell the player they have succeeded */
            else if (next_stage == QUEST_PAYOUT) {
                msg_format("You have completed %s!", q_name + q_ptr->name);
                quest_res_queued = TRUE;
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
    while(check_quest(&qe, TRUE));
}

/*
 * Apply instantaneous quest effect (QUEST_PAYOUT, QUEST_FORFEIT)
 */

bool do_quest_resolution(void) {
    int i;
    bool ret = FALSE;

    /* Hack -- efficiency */
    if(!quest_res_queued) return FALSE;
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
            int k_idx;

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
                k_idx = qe_ptr->kind ?
                        qe_ptr->kind :
                        lookup_kind(e_ptr->tval[0],
                                rand_range(e_ptr->min_sval[0], e_ptr->max_sval[0]));

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

            if(flags & EVENT_GET_ITEM) {
                /* Gain the item */
                (void)inven_carry(i_ptr);
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

            if(qe_ptr->artifact && (qe_ptr->flags & EVENT_DESTROY_ITEM)) {
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
            for (i = 0;i < STORE_CHOICES;i++) {
                if(!qstore->count[i] || (i == STORE_CHOICES - 1)) {
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
