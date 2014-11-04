/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

enum CompanionEntries
{
    // Clockwork Rocket Bot
    COMPANION_CLOCKWORK_ROCKET_BOT      = 24968,
    COMPANION_BLUE_CLOCKWORK_ROCKET_BOT = 40295,
};

enum CompanionSpells
{
    // Clockwork Rocket Bot
    SPELL_ROCKET_BOT_ATTACK             = 45269,
    
    // Pengu
    SPELL_SLEDDING                      = 51358,
};

uint32 const rocketBotsList[2] =
{
    COMPANION_CLOCKWORK_ROCKET_BOT,
    COMPANION_BLUE_CLOCKWORK_ROCKET_BOT
};

class npc_clockwork_rocket_bot : public CreatureScript
{
    public:
        npc_clockwork_rocket_bot() : CreatureScript("npc_clockwork_rocket_bot") { }

        struct npc_clockwork_rocket_botAI : public ScriptedAI
        {
            npc_clockwork_rocket_botAI(Creature* creature) : ScriptedAI(creature), _checkTimer(1000) 
            {
            }

            void UpdateAI(uint32 const diff)
            {
                if (_checkTimer <= diff)
                {
                    for (uint8 i = 0; i < 2; i++)
                    {
                        std::list<Creature*> rocketBots;
                        me->GetCreatureListWithEntryInGrid(rocketBots, rocketBotsList[i], 15.0f);
                        if (!rocketBots.empty())
                            for (std::list<Creature*>::const_iterator itr = rocketBots.begin(); itr != rocketBots.end(); ++itr)
                            {
                                Creature* rocketBot = *itr;
                                if (rocketBot && rocketBot != me)
                                    DoCast(rocketBot, SPELL_ROCKET_BOT_ATTACK);
                            }
                    }
                    _checkTimer = 5000;
                }
                else 
                    _checkTimer -= diff;
            }

        private:
              uint32 _checkTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_clockwork_rocket_botAI(creature);
        }
};

/*#####
# npc_train_wrecker
#####*/

enum TrainWrecker
{
    GO_TOY_TRAIN_SET    = 193963,
    SPELL_TRAIN_WRECKER = 62943,
    POINT_JUMP          = 1,
    EVENT_SEARCH        = 1,
    EVENT_JUMP_1        = 2,
    EVENT_WRECK         = 3,
    EVENT_DANCE         = 4
};

class npc_train_wrecker : public CreatureScript
{
    public:
        npc_train_wrecker() : CreatureScript("npc_train_wrecker") { }

        struct npc_train_wreckerAI : public ScriptedAI
        {
            npc_train_wreckerAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.ScheduleEvent(EVENT_SEARCH, 3000);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id == POINT_JUMP)
                    _events.ScheduleEvent(EVENT_JUMP_1, 500);
            }

            void UpdateAI(uint32 const diff)
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SEARCH:
                            if (GameObject* train = me->FindNearestGameObject(GO_TOY_TRAIN_SET, 20.0f))
                            {
                                if (me->GetDistance(train) > 1.5f)
                                {
                                    float x, y, z;
                                    me->GetNearPoint(me, x, y, z, 0.0f, me->GetDistance(train) - 1.5f, me->GetAngle(train));
                                    me->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                    me->GetMotionMaster()->MovePoint(POINT_JUMP, x, y, z);
                                }
                                else
                                    _events.ScheduleEvent(EVENT_JUMP_1, 500);
                            }
                            else
                                _events.ScheduleEvent(EVENT_SEARCH, 3000);
                            break;
                        case EVENT_JUMP_1:
                            if (GameObject* train = me->FindNearestGameObject(GO_TOY_TRAIN_SET, 5.0f))
                                me->GetMotionMaster()->MoveJump(train->GetPositionX(), train->GetPositionY(), train->GetPositionZ(), 4.0f, 6.0f);
                            _events.ScheduleEvent(EVENT_WRECK, 2500);
                            break;
                        case EVENT_WRECK:
                            if (GameObject* train = me->FindNearestGameObject(GO_TOY_TRAIN_SET, 5.0f))
                            {
                                DoCast(SPELL_TRAIN_WRECKER);
                                train->SetLootState(GO_JUST_DEACTIVATED);
                                _events.ScheduleEvent(EVENT_DANCE, 2500);
                            }
                            else
                                me->DespawnOrUnsummon(3000);
                            break;
                        case EVENT_DANCE:
                            me->HandleEmoteCommand(EMOTE_STATE_DANCE);
                            me->DespawnOrUnsummon(10000);
                            break;
                    }
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_train_wreckerAI(creature);
        }

};

class npc_mini_tyrael : public CreatureScript
{
    public:
        npc_mini_tyrael() : CreatureScript("npc_mini_tyrael") { }

        struct npc_mini_tyraelAI : public ScriptedAI
        {
            npc_mini_tyraelAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void ReceiveEmote(Player* /*player*/, uint32 emote)
            {
                switch (emote)
                {
                    case TEXT_EMOTE_DANCE:
                        me->HandleEmoteCommand(EMOTE_STATE_DANCE);
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mini_tyraelAI(creature);
        }
};

class npc_pengu : public CreatureScript
{
    public:
        npc_pengu() : CreatureScript("npc_pengu") { }

        struct npc_penguAI : public ScriptedAI
        {
            npc_penguAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void ReceiveEmote(Player* /*player*/, uint32 emote)
            {
                if (me->isMoving())
                    return;

                switch (emote)
                {
                    case TEXT_EMOTE_SEXY:
                        DoCastAOE(SPELL_SLEDDING);
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_penguAI(creature);
        }
};

class npc_puzzle_box_of_yogg_saron : public CreatureScript
{
    public:
        npc_puzzle_box_of_yogg_saron() : CreatureScript("npc_puzzle_box_of_yogg_saron") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_puzzle_box_of_yogg_saronAI(creature);
        }

        struct npc_puzzle_box_of_yogg_saronAI : public Scripted_NoMovementAI
        {
            npc_puzzle_box_of_yogg_saronAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
            {
            }

            void IsSummonedBy(Unit* owner)
            {
                if (owner->GetTypeId() == TYPEID_PLAYER)
                {
                    me->PlayDirectSound(23414, owner->ToPlayer());
                    Talk(0, owner->GetGUID());
                }
                me->DespawnOrUnsummon(1000);
            }
        };
};

void AddSC_npc_companions()
{
    new npc_clockwork_rocket_bot();
    new npc_train_wrecker();
    new npc_mini_tyrael();
    new npc_pengu();
    new npc_puzzle_box_of_yogg_saron();
}
