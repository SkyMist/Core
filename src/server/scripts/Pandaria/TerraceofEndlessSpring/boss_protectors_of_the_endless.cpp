/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "ScriptPCH.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Spell.h"
#include "Vehicle.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CreatureTextMgr.h"
#include "MoveSplineInit.h"
#include "Weather.h"

#include "terrace_of_endless_spring.h"

enum Yells
{
    // Boss
    SAY_AGGRO               = 0,
    SAY_SLAY                = 1,
    SAY_DEATH               = 2
};

enum Spells
{
    // Boss
    SPELL_SUMMON_VISUAL     = 103595
};

enum Events
{
    // Boss
    EVENT_MOVE              = 1
};

enum Actions
{
    // Boss
    ACTION_START            = 1
};

enum MovePoints
{
    // Boss
    POINT_SUMMON            = 1
};

class boss_protector_kaolan : public CreatureScript
{
public:
    boss_protector_kaolan() : CreatureScript("boss_protector_kaolan") { }

    struct boss_protector_kaolanAI : public BossAI
    {
        boss_protector_kaolanAI(Creature* creature) : BossAI(creature, DATA_PROT_OF_ENDLESS_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;

        void Reset() OVERRIDE
        {
            events.Reset();
            summons.DespawnAll();

            _Reset();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            // events.ScheduleEvent(EVENT_SUMMON_SERVITORS, 8000);

            if (instance)
            {
                instance->SetData(DATA_PROT_OF_ENDLESS_EVENT, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
            }

            _EnterCombat();
        }

        void KilledUnit(Unit* victim) OVERRIDE
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterEvadeMode() OVERRIDE
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            // me->DespawnOrUnsummon(1000);

            if (instance)
            {
                instance->SetData(DATA_PROT_OF_ENDLESS_EVENT, FAIL);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            _EnterEvadeMode();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();

            if (instance)
            {
                instance->SetData(DATA_PROT_OF_ENDLESS_EVENT, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            _JustDied();
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
            summons.Summon(summon);
            summon->setActive(true);

            if (me->isInCombat())
                summon->AI()->DoZoneInCombat();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            // while (uint32 eventId = events.ExecuteEvent())
            // {
            //     switch (eventId)
            //     {
            //     }
            // }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_protector_kaolanAI(creature);
    }
};

class boss_elder_asani : public CreatureScript
{
public:
    boss_elder_asani() : CreatureScript("boss_elder_asani") { }

    struct boss_elder_asaniAI : public BossAI
    {
        boss_elder_asaniAI(Creature* creature) : BossAI(creature, DATA_PROT_OF_ENDLESS_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;

        void Reset() OVERRIDE
        {
            events.Reset();
            summons.DespawnAll();

            _Reset();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            // events.ScheduleEvent(EVENT_SUMMON_SERVITORS, 8000);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add

            _EnterCombat();
        }

        void KilledUnit(Unit* victim) OVERRIDE
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterEvadeMode() OVERRIDE
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            // me->DespawnOrUnsummon(1000);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

            _EnterEvadeMode();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

            _JustDied();
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
            summons.Summon(summon);
            summon->setActive(true);

            if (me->isInCombat())
                summon->AI()->DoZoneInCombat();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            // while (uint32 eventId = events.ExecuteEvent())
            // {
            //     switch (eventId)
            //     {
            //     }
            // }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_elder_asaniAI(creature);
    }
};

class boss_elder_regali : public CreatureScript
{
public:
    boss_elder_regali() : CreatureScript("boss_elder_regali") { }

    struct boss_elder_regaliAI : public BossAI
    {
        boss_elder_regaliAI(Creature* creature) : BossAI(creature, DATA_PROT_OF_ENDLESS_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;

        void Reset() OVERRIDE
        {
            events.Reset();
            summons.DespawnAll();

            _Reset();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            // events.ScheduleEvent(EVENT_SUMMON_SERVITORS, 8000);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add

            _EnterCombat();
        }

        void KilledUnit(Unit* victim) OVERRIDE
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterEvadeMode() OVERRIDE
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            // me->DespawnOrUnsummon(1000);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

            _EnterEvadeMode();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

            _JustDied();
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
            summons.Summon(summon);
            summon->setActive(true);

            if (me->isInCombat())
                summon->AI()->DoZoneInCombat();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            // while (uint32 eventId = events.ExecuteEvent())
            // {
            //     switch (eventId)
            //     {
            //     }
            // }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_elder_regaliAI(creature);
    }
};

void AddSC_boss_protectors_of_the_endless()
{
    new boss_protector_kaolan();
    new boss_elder_asani();
    new boss_elder_regali();
}
