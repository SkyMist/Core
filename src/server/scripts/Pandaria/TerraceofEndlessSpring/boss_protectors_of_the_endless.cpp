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

#include "deadmines.h"

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

class boss_admiral_ripsnarl : public CreatureScript
{
public:
    boss_admiral_ripsnarl() : CreatureScript("boss_admiral_ripsnarl") { }

    struct boss_admiral_ripsnarlAI : public BossAI
    {
        boss_admiral_ripsnarlAI(Creature* creature) : BossAI(creature, DATA_ADMIRAL_RIPSNARL_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;

        void Reset()
        {
            events.Reset();
            summons.DespawnAll();

            _Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);

            // events.ScheduleEvent(EVENT_SUMMON_SERVITORS, 8000);

            if (instance)
            {
                instance->SetData(DATA_ADMIRAL_RIPSNARL_EVENT, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
            }

            _EnterCombat();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterEvadeMode()
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            // me->DespawnOrUnsummon(1000);

            if (instance)
            {
                instance->SetData(DATA_ADMIRAL_RIPSNARL_EVENT, FAIL);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            _EnterEvadeMode();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();

            if (instance)
            {
                instance->SetData(DATA_ADMIRAL_RIPSNARL_EVENT, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            _JustDied();
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
            summon->setActive(true);

			if (me->isInCombat())
                summon->AI()->DoZoneInCombat();
        }

        void UpdateAI(const uint32 diff)
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

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_admiral_ripsnarlAI(creature);
    }
};

void AddSC_boss_admiral_ripsnarl()
{
    new boss_admiral_ripsnarl();
}
