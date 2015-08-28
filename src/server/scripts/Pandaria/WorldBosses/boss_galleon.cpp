/*
 * Copyright (C) 2011-2015 SkyMist Gaming
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
 *
 * World Boss: Galleon.
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"

enum Yells
{
    // Chief Salyis.
    SAY_SPAWN                   = 0, // Loot and pillage, bwhahaha!
    SAY_AGGRO,                       // Bring me their corpses!
    SAY_DEATH,                       // Retreat to the hills!
    SAY_KILL,                        // They are soft... weak!
    SAY_CANNON_BARRAGE               // Arm the cannons! Ready... arm... fire!
};

#define ANN_CANNON_BARRAGE "Galleon prepares to unleash a |cFFFF0000|Hspell:121600|h[Cannon Barrage]|h|r!"
#define ANN_STOMP "Galleon is about to |cFFFF0000|Hspell:121787|h[Stomp]|h|r!"
#define ANN_WARMONGERS_LEAP "Warmongers leap from Galleon's back to join the battle!"

enum Spells
{
    // Galleon.
    SPELL_STOMP                 = 121787,
    SPELL_CANNON_BARRAGE        = 121600,
    SPELL_FIRE_SHOT             = 121673,
    SPELL_IMPALING_PULL         = 121747,
    SPELL_BERSERK               = 47008

    // NPC's.
};

enum Events
{
    EVENT_STOMP                 = 1,
    EVENT_CANNON                = 2,
    EVENT_FIRE_SHOT             = 3,
    EVENT_IMPALING              = 4,
    EVENT_SPAWN                 = 6,
    EVENT_BERSERK               = 7
};

enum Creatures
{
    NPC_SALYIN_WARMONGER        = 62351,
    NPC_CHIEF_SALYIS            = 62352
};

class boss_galleon : public CreatureScript
{
    public:
        boss_galleon() : CreatureScript("boss_galleon") { }

        struct boss_galleon_AI : public ScriptedAI
        {
            boss_galleon_AI(Creature* creature) : ScriptedAI(creature), vehicle(creature->GetVehicleKit()), summons(me)
            {
                ASSERT(vehicle);
            }

            EventMap events;
            SummonList summons;
            Vehicle* vehicle;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
            }

            void KilledUnit(Unit* victim) { }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_STOMP, 50000);
                events.ScheduleEvent(EVENT_CANNON, 25000);
                events.ScheduleEvent(EVENT_SPAWN, 60000);
                events.ScheduleEvent(EVENT_FIRE_SHOT, 10000);
                events.ScheduleEvent(EVENT_BERSERK, 900000);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STOMP:
                        {
                            DoCast(me, SPELL_STOMP);
                            events.ScheduleEvent(EVENT_STOMP, 60000);
                            break;
                        }

                        case EVENT_CANNON:
                        {
                            DoCast(me, SPELL_CANNON_BARRAGE);
                            events.ScheduleEvent(EVENT_CANNON, 60000);
                            break;
                        }

                        case EVENT_SPAWN:
                        {
                            for (uint8 i = 0; i < 6; i++)
                                me->SummonCreature(NPC_SALYIN_WARMONGER, me->GetPositionX()+rand()%5, me->GetPositionY()+3+rand()%5, me->GetPositionZ()+2, 10.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                            events.ScheduleEvent(EVENT_SPAWN, 60000);
                            break;
                        }

                        case EVENT_BERSERK:
                        {
                            DoCast(me, SPELL_BERSERK);
                            break;
                        }
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_galleon_AI(creature);
        }
};

class npc_salyin_warmonger : public CreatureScript
{
    public:
        npc_salyin_warmonger() : CreatureScript("npc_salyin_warmonger") { }

        struct npc_salyin_warmongerAI : public ScriptedAI
        {
            npc_salyin_warmongerAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* unit)
            {
                events.ScheduleEvent(EVENT_IMPALING, 50000);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_IMPALING:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                                DoCast(target, SPELL_IMPALING_PULL);
                            events.ScheduleEvent(EVENT_IMPALING, 60000);
                            break;
                        }
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_salyin_warmongerAI (creature);
        }
};

void AddSC_boss_galleon()
{
    new boss_galleon();
    new npc_salyin_warmonger();
}
