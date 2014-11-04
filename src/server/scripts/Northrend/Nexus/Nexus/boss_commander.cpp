/*
 * Copyright (C) 2009-2012 WowCircle <http://www.wowcircle.com/>
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
 
/*
 * Script rewrited and improved by Ramusik
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "nexus.h"

enum Spells
{
    SPELL_BATTLE_SHOUT      = 31403,
    SPELL_CHARGE            = 60067,
    SPELL_FRIGHTENING_SHOUT = 19134,
    SPELL_WHIRLWIND         = 38618,
    SPELL_FROZEN_PRISON     = 47543,
};

enum Texts
{
    SAY_AGGRO               = 0,
    SAY_KILL                = 1,
    SAY_DEATH               = 2,
};

enum Events
{
    EVENT_BATTLE_SHOUT      = 0,
    EVENT_COMMANDER_CHARGE  = 1,
    EVENT_FRIGHTENING_SHOUT = 2,
    EVENT_WHIRLWIND         = 3,
};

class boss_commander_nexus : public CreatureScript
{
    public:
        boss_commander_nexus() : CreatureScript("boss_commander_nexus") { }

        struct boss_commander_nexusAI : public BossAI
        {
            boss_commander_nexusAI(Creature* creature) : BossAI(creature, DATA_COMMANDER)
            {
            }

            void Reset()
            {
                _Reset();
                events.ScheduleEvent(EVENT_BATTLE_SHOUT, 1000);
                events.ScheduleEvent(EVENT_COMMANDER_CHARGE, 2000);
                events.ScheduleEvent(EVENT_FRIGHTENING_SHOUT, 20000);
                events.ScheduleEvent(EVENT_WHIRLWIND, 15000);
                DoCast(me, SPELL_FROZEN_PRISON);
            }
            
            void MoveInLineOfSight(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER && me->IsInRange(who, 0, 20, false))
                {
                    me->RemoveAurasDueToSpell(SPELL_FROZEN_PRISON);
                    me->AI()->AttackStart(who);
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BATTLE_SHOUT:
                            DoCast(me, SPELL_BATTLE_SHOUT);
                            events.ScheduleEvent(EVENT_BATTLE_SHOUT, 120000);
                            break;
                        case EVENT_COMMANDER_CHARGE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 25, true))
                                if (me->GetExactDist2d(target) > 8.0f)
                                    DoCast(target, SPELL_CHARGE);
                            events.ScheduleEvent(EVENT_COMMANDER_CHARGE, urand(29000,32000));
                            break;
                        case EVENT_FRIGHTENING_SHOUT:
                            DoCast(me, SPELL_FRIGHTENING_SHOUT);
                            events.ScheduleEvent(EVENT_FRIGHTENING_SHOUT, 20000);
                            break;
                        case EVENT_WHIRLWIND:
                            DoCast(me, SPELL_WHIRLWIND);
                            events.ScheduleEvent(EVENT_WHIRLWIND, 15000);
                            events.RescheduleEvent(EVENT_COMMANDER_CHARGE, 3500);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_commander_nexusAI (creature);
        }
};

void AddSC_boss_commander_nexus()
{
    new boss_commander_nexus();
}
