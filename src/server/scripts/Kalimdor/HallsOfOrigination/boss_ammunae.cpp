/*Copyright (C) 2013 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

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
#include "Player.h"
#include "Weather.h"

#include "halls_of_origination.h"

#define SAY_ANNOUNCE "Ammunae plants some Seedling Pods nearby!"

enum Spells
{
    // NPCs
    SPELL_ENERGIZING_GROWTH                  = 89123,
    SPELL_ENERGIZING_GROWTH_TICK             = 89124,
    SPELL_THORN_SLASH                        = 76044,
    SPELL_THORN_SLASH_H                      = 90007,
    SPELL_ENERGIZE_ENRAGE                    = 75657,
    SPELL_VISUAL_ENERGIZE                    = 75624,
    
    // Ammunae
    SPELL_NO_REGEN                           = 78725,
    SPELL_RAMPANT_GROWTH                     = 75790,
    SPELL_RAMPANT_GROWTH_H                   = 89888,
    SPELL_WITHER                             = 76043,
    SPELL_CONSUME_ENERGY_MANA_N              = 75718, 
    SPELL_CONSUME_ENERGY_ENER_N              = 79766,
    SPELL_CONSUME_ENERGY_RAGE_N              = 79767, // rage stiil rises (1)
    SPELL_CONSUME_ENERGY_RUNE_N              = 79768, // runic is (6)
    SPELL_CONSUME_ENERGY_FOCU_N              = 80968, // focus is (2)
    SPELL_CONSUME_ENERGY_ENER_H              = 94961, // power burn (3)
    
    SPELL_SPORE                              = 75866  // Summon spore.
};

enum ScriptTexts
{
    SAY_AGGRO                = 0,
    SAY_RAMPANT              = 1,
    SAY_KILL                 = 2,
    SAY_DEATH                = 3
};

class boss_ammunae : public CreatureScript
{
public:
    boss_ammunae() : CreatureScript("boss_ammunae") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_ammunaeAI(creature);
    }

    struct boss_ammunaeAI : public ScriptedAI
    {
        boss_ammunaeAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            ASSERT(creature->GetVehicleKit()); // we dont actually use it, just check if exists
            creature->ApplySpellImmune(0, IMMUNITY_ID, 75702, true);
            creature->ApplySpellImmune(0, IMMUNITY_ID, 89889, true);
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 m_uiWitherTimer;
        uint32 m_uiSporeTimer;
        uint32 m_uiSeedlingTimer;
        uint32 m_uiBoomTimer;
        uint32 m_uiEnergizingGrowthTimer;
        uint32 m_uiLife_Drain_Timer;
        uint32 m_uiLife_Drain2_Timer;
        uint32 m_uiLife_Drain3_Timer;
        uint32 m_uiLife_Drain4_Timer;
        uint32 m_uiLife_Drain5_Timer;
        SummonList summons;
        Creature* seedling[10];
        Creature* blossom[10];
        uint16 i;

        void Reset() OVERRIDE
        {
            if (instance)
                instance->SetData(DATA_AMMUNAE_EVENT, NOT_STARTED);

            DoCast(me, SPELL_NO_REGEN);
            summons.DespawnAll();

            me->SetPower(POWER_ENERGY, 0);
            me->SetMaxPower(POWER_ENERGY, 100);

            m_uiWitherTimer       = 5000;
            m_uiSeedlingTimer     = 10000;
            m_uiLife_Drain_Timer  = 6000;
            m_uiLife_Drain2_Timer = 10000;
            m_uiLife_Drain3_Timer = 14000;
            m_uiLife_Drain4_Timer = 19000;
            m_uiLife_Drain5_Timer = 25000;
            m_uiSporeTimer        = 7500;
            i                     = 0;

            for (uint8 p = 0; p < 10; p++)
            {
                seedling[p] = NULL;
                blossom[p] = NULL;
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);
            DoCast(me, SPELL_NO_REGEN);

            if (instance)
            {
                instance->SetData(DATA_AMMUNAE_EVENT, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }
        }

        void KilledUnit(Unit* /*killed*/) OVERRIDE
        {
            Talk(SAY_KILL);
        }

        void JustDied(Unit* killer) OVERRIDE
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();
            i = 0;

            if (instance)
            {
                instance->SetData(DATA_AMMUNAE_EVENT, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
        }

        void EnterEvadeMode() OVERRIDE
        {
            me->RemoveAllAuras();
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
            {
                instance->SetData(DATA_AMMUNAE_EVENT, FAIL);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
		    summons.Summon(summon);
		    summon->setActive(true);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (m_uiSporeTimer <= diff)
            {
                if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                    DoCast(victim, SPELL_SPORE);

                m_uiSporeTimer = urand(20000, 25000);
            }
            else m_uiSporeTimer -= diff;

            if (m_uiWitherTimer <= diff)
            {
                if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    DoCast(victim, SPELL_WITHER);

                m_uiWitherTimer = urand(14000, 18000);
            }
            else m_uiWitherTimer -= diff;

            if (m_uiLife_Drain_Timer <= diff)
            {
                Unit* pTarget = NULL;

                uint8 i = 0;
                while (i < 5)                                   // max 5 tries to get a random target with power_mana
                {
                    ++i;
                    pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true); 
                    if (pTarget && pTarget->getPowerType() == POWER_MANA)
                    {
                        i = 5;
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 10); // cast spell on target with mana.
                        DoCast(pTarget, SPELL_CONSUME_ENERGY_MANA_N);
                    }
                }

                m_uiLife_Drain_Timer = 15000;
            } 
            else m_uiLife_Drain_Timer -= diff;

            if (m_uiLife_Drain2_Timer <= diff)
            {
                Unit* pTarget = NULL;

                uint8 i = 0;
                while (i < 5)                                   // max 5 tries to get a random target with power_energy
                {
                    ++i;
                    pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true); 
                    if (pTarget && pTarget->getPowerType() == POWER_ENERGY)
                    {
                        i = 5;
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 10); // cast spell on target with mana.
                        DoCast(pTarget, SPELL_CONSUME_ENERGY_ENER_N);
                    }
                }

                m_uiLife_Drain2_Timer = 15000;
            }
            else m_uiLife_Drain2_Timer -= diff;

            if (m_uiLife_Drain3_Timer <= diff)
            {
                Unit* pTarget = NULL;

                uint8 i = 0;
                while (i < 5)                                   // max 5 tries to get a random target with power_runic
                {
                    ++i;
                    pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true); 
                    if (pTarget && pTarget->getPowerType() == POWER_RUNIC_POWER)
                    {
                        i = 5;
                        DoCast(pTarget, SPELL_CONSUME_ENERGY_RUNE_N);
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 20);
                    }
                }

                m_uiLife_Drain3_Timer = 15000;
            }
            else m_uiLife_Drain3_Timer -= diff;

            if (m_uiLife_Drain4_Timer <= diff)
            {
                Unit* pTarget = NULL;

                uint8 i = 0;
                while (i < 5)                                   // max 5 tries to get a random target with power_rage
                {
                    ++i;
                    pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true);                    
                    if (pTarget && pTarget->getPowerType() == POWER_RAGE)
                    {
                        i = 5;
                        DoCast(pTarget, SPELL_CONSUME_ENERGY_RAGE_N);
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 10);
                    }
                }

                m_uiLife_Drain4_Timer = 15000;
            }
            else m_uiLife_Drain4_Timer -= diff;

            if (m_uiLife_Drain5_Timer <= diff)
            {
                Unit* pTarget = NULL;

                uint8 i = 0;
                while (i < 5)                                   // max 5 tries to get a random target with power_focus
                {
                    ++i;
                    pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true); 
                    if (pTarget && pTarget->getPowerType() == POWER_FOCUS)
                    {
                        i = 5;

                        DoCast(pTarget, SPELL_CONSUME_ENERGY_FOCU_N);
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 20);
                    }
                }

                m_uiLife_Drain5_Timer = 15000;
            }
            else m_uiLife_Drain5_Timer -= diff;

            if (m_uiSeedlingTimer <= diff)
            {
                me->MonsterTextEmote(SAY_ANNOUNCE, NULL, true);

                if (urand(1, 2) == 1)
                {
                    seedling[i] = me->SummonCreature(40716, me->GetPositionX() + 15 + i * 3, me->GetPositionY() + urand(10, 20), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 300000);
                    DoZoneInCombat(seedling[i]);
                }
                else
                {
                    seedling[i] = me->SummonCreature(40716, me->GetPositionX() - 15 - i * 3, me->GetPositionY() + urand(10, 20), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 300000);
                    DoZoneInCombat(seedling[i]);
                }
                if(!i) 
                    m_uiEnergizingGrowthTimer = 3000;
                i++;
                m_uiSeedlingTimer = IsHeroic() ? urand(12000, 17000) : urand(17000, 23000);
            }
            else m_uiSeedlingTimer -= diff;

            if (me->GetPower(POWER_ENERGY) == 100)
            {
                Talk(SAY_RAMPANT);
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    me->CastStop();
                DoCast(me, IsHeroic() ? SPELL_RAMPANT_GROWTH_H : SPELL_RAMPANT_GROWTH);

                m_uiBoomTimer = 2000;
            }

            if (m_uiBoomTimer > 0 && m_uiBoomTimer <= diff)
            {
                uint8 k = 0;
                for (uint8 j = 0; j < 10; j++)
                if (seedling[j] && k != i)
                {
                    k++;
                    blossom[j] = me->SummonCreature(40620, seedling[j]->GetPositionX(), seedling[j]->GetPositionY(), seedling[j]->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 300000);
                    DoZoneInCombat(blossom[j]);
                    seedling[j]->setDeathState(JUST_DIED);
                }

                for(uint8 p = 0; p < 10; p++)
                {
                    seedling[p] = NULL;
                    blossom[p] = NULL;
                }

                i = 0;
                m_uiBoomTimer = 0;
            }
            else m_uiBoomTimer -= diff;

            if (i > 0 && m_uiEnergizingGrowthTimer <= diff)
            {
                if (IsHeroic())
                {
                    DoCast(me, SPELL_ENERGIZING_GROWTH_TICK);
                    me->AddAura(SPELL_ENERGIZE_ENRAGE, me);
                }
                else if (!IsHeroic())
                    me->AddAura(SPELL_ENERGIZE_ENRAGE, me);

                m_uiEnergizingGrowthTimer = 4000 - i*1000;
                me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 10);
            }
            else m_uiEnergizingGrowthTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_seedling : public CreatureScript
{
public:
    npc_seedling() : CreatureScript("npc_seedling") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_seedlingAI(creature);
    }

    struct npc_seedlingAI : public ScriptedAI
    {
        npc_seedlingAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            creature->CastSpell(creature, SPELL_VISUAL_ENERGIZE, false);
            creature->CastSpell(creature, SPELL_ENERGIZING_GROWTH, false);
        }

        InstanceScript* instance;

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            me->RemoveAllAuras();

            if (Creature* Ammunae = me->FindNearestCreature(BOSS_AMMUNAE, 200.0f, true))
                CAST_AI(boss_ammunae::boss_ammunaeAI, Ammunae->AI())->i--;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    };
};

class npc_blossom : public CreatureScript
{
public:
    npc_blossom() : CreatureScript("npc_blossom") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_blossomAI(creature);
    }

    struct npc_blossomAI : public ScriptedAI
    {
        npc_blossomAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            m_uiThornSlashTimer = urand(5000, 10000);
        }

        InstanceScript* instance;
        uint32 m_uiThornSlashTimer;

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (m_uiThornSlashTimer <= diff)
            {
                DoCast(me->GetVictim(), IsHeroic() ? SPELL_THORN_SLASH_H : SPELL_THORN_SLASH);
                m_uiThornSlashTimer = urand(15000, 20000);
            }
            else
                m_uiThornSlashTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_spore : public CreatureScript
{
public:
    npc_spore() : CreatureScript("npc_spore") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_sporeAI(creature);
    }

    struct npc_sporeAI : public ScriptedAI
    {
        npc_sporeAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            me->SummonCreature(40585, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_CORPSE_DESPAWN, 30000);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            DoMeleeAttackIfReady();        
        }
    };
};

void AddSC_boss_ammunae()
{
    new boss_ammunae();
    new npc_seedling();
    new npc_blossom();
    new npc_spore();
}
