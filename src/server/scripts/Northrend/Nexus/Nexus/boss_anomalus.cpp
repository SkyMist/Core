/*
 * Copyright (C) 2009-2012 WowCircle <http://www.wowcircle.com/>
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
 
enum Texts
{
    SAY_AGGRO               = 0,
    SAY_DEATH               = 1,
    SAY_RIFT                = 2,
    SAY_SHIELD              = 3,
    EMOTE_CREATE_RIFT       = 4,
    EMOTE_RIFT_SHIELD       = 5
};

enum Spells
{
    // Anomalus
    SPELL_SPARK                     = 47751,
    SPELL_CREATE_RIFT               = 47743,
    SPELL_CHARGE_RIFT               = 47747,
    SPELL_RIFT_SHIELD               = 47748,
    SPELL_ARCANE_ATTRACTION         = 57063,

    // Chaotic Rift
    SPELL_RIFT_AURA                 = 47687,
    SPELL_RIFT_SUMMON_AURA          = 47732,
    SPELL_CHARGED_RIFT_AURA         = 47733,
    SPELL_CHARGED_RIFT_SUMMON_AURA  = 47742,
    SPELL_ARCANEFORM                = 48019
};

enum Events
{
    EVENT_SPARK                 = 1,
    EVENT_ARCANE_ATTRACTION     = 2,
    EVENT_CREATE_RIFT           = 3,
    EVENT_RIFT_SHIELD           = 4,
    EVENT_RIFT_DIE              = 5,
    EVENT_SPECIAL               = 6
};

enum EventGroups
{
    EVENT_GROUP_PHASE_ONE   = 1
};

enum Phases
{
    PHASE_ONE   = 1,
    PHASE_TWO   = 2
};    
 
class boss_anomalus : public CreatureScript
{
    public:
        boss_anomalus() : CreatureScript("boss_anomalus") { }

        struct boss_anomalusAI : public BossAI
        {
            boss_anomalusAI(Creature* creature) : BossAI(creature, DATA_ANOMALUS)
            {
            }
            
            void Reset()
            {
                _Reset();
                events.ScheduleEvent(EVENT_SPARK, 5000, EVENT_GROUP_PHASE_ONE);
                events.ScheduleEvent(EVENT_ARCANE_ATTRACTION, 11000, EVENT_GROUP_PHASE_ONE);
                _phase          = PHASE_ONE;
                _chaosTheory    = true;
                
                instance->SetBossState(DATA_ANOMALUS, NOT_STARTED);
            }
            
            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);
                
                instance->SetBossState(DATA_ANOMALUS, IN_PROGRESS);
            }
            
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {
                if (_phase == PHASE_ONE && !HealthAbovePct(50))
                {
                    _phase = PHASE_TWO;
                    me->ClearUnitState(UNIT_STATE_CASTING);
                    events.ScheduleEvent(EVENT_CREATE_RIFT, 1000);
                    events.DelayEvents(45000, EVENT_GROUP_PHASE_ONE);
                }
            }
            
            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();

                instance->SetBossState(DATA_ANOMALUS, DONE);
            }
            
            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                if (summon->GetEntry() == MOB_CHAOTIC_RIFT)
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        summon->AI()->AttackStart(target);
            }
            
            void SummonedCreatureDies(Creature* summoned, Unit* /*who*/)
            {
                if (summoned->GetEntry() == MOB_CHAOTIC_RIFT)
                {
                    _chaosTheory = false;
                    me->ClearUnitState(UNIT_STATE_CASTING);
                    events.ScheduleEvent(EVENT_RIFT_DIE, 1000);
                }
            }
            
            uint32 GetData(uint32 type)
            {
                if (type == DATA_CHAOS_THEORY)
                    return _chaosTheory ? 1 : 0;

                return 0;
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
                        case EVENT_SPARK:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_SPARK);
                            events.ScheduleEvent(EVENT_SPARK, 5000, EVENT_GROUP_PHASE_ONE);
                            break;
                        case EVENT_ARCANE_ATTRACTION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_ARCANE_ATTRACTION);
                            events.ScheduleEvent(EVENT_ARCANE_ATTRACTION, 10000, EVENT_GROUP_PHASE_ONE);
                            break;
                        case EVENT_CREATE_RIFT:
                            Talk(SAY_RIFT);
                            Talk(EMOTE_CREATE_RIFT);
                            DoCast(me, SPELL_CREATE_RIFT);
                            events.ScheduleEvent(EVENT_RIFT_SHIELD, 1000);
                            break;
                        case EVENT_RIFT_SHIELD:
                            Talk(SAY_SHIELD);
                            Talk(EMOTE_RIFT_SHIELD);
                            DoCast(me, SPELL_RIFT_SHIELD);
                            DoCast(me, SPELL_CHARGE_RIFT, true);
                            break;
                        case EVENT_RIFT_DIE:
                            me->RemoveAurasDueToSpell(SPELL_RIFT_SHIELD);
                            events.RescheduleEvent(EVENT_SPARK, 5000, EVENT_GROUP_PHASE_ONE);
                            events.RescheduleEvent(EVENT_ARCANE_ATTRACTION, 11000, EVENT_GROUP_PHASE_ONE);
                            break;
                        default:
                            break;
                    }
                }
                
                DoMeleeAttackIfReady();
            }
        private:
            uint8 _phase;
            bool _chaosTheory;
        };
 
        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_anomalusAI(creature);
        }
};

class mob_chaotic_rift : public CreatureScript
{
    public:
        mob_chaotic_rift() : CreatureScript("mob_chaotic_rift") { }

        struct mob_chaotic_riftAI : public Scripted_NoMovementAI
        {
            mob_chaotic_riftAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
            }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SPECIAL, 45000);
                DoCast(me, SPELL_CHARGED_RIFT_AURA, true);
                DoCast(me, SPELL_CHARGED_RIFT_SUMMON_AURA, true);
                DoCast(me, SPELL_ARCANEFORM, false);
            }
            
            void JustSummoned(Creature* summon)
            {
                if (summon->GetEntry() == MOB_CRAZED_MANA_WRAITH)
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        summon->AI()->AttackStart(target);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;
                    
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    if (eventId == EVENT_SPECIAL)
                    {
                        DoCast(me, SPELL_RIFT_AURA, true);
                        DoCast(me, SPELL_RIFT_SUMMON_AURA, true);
                    }
                }
            }
        public:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_chaotic_riftAI(creature);
        }
};

class achievement_chaos_theory : public AchievementCriteriaScript
{
    public:
        achievement_chaos_theory() : AchievementCriteriaScript("achievement_chaos_theory")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Anomalus = target->ToCreature())
                if (Anomalus->AI()->GetData(DATA_CHAOS_THEORY))
                    return true;

            return false;
        }
};

void AddSC_boss_anomalus()
{
    new boss_anomalus();
    new mob_chaotic_rift();
    new achievement_chaos_theory();
}
