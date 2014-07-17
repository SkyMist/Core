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
#include "WorldPacket.h"

#include "halls_of_origination.h"

enum Yells
{
    SAY_AGGRO                                = 0,
    SAY_DEATH                                = 1,
    SAY_KILL                                 = 2
};

enum Spells
{
    SPELL_FLAME_BOLT                         = 77370,
    SPELL_RAGING_SMASH                       = 83650,
    SPELL_EARTH_SPIKE                        = 94974, // Dummy and if not run die. 95729 is something else.
    SPELL_QUICKSAND_SLOW                     = 75548,
    SPELL_QUICKSAND_DMG                      = 75546,
    SPELL_SMASH                              = 75453,
    SPELL_SAND_VORTEX                        = 79109,
    SPELL_EARTHQUAKE_VISUAL                  = 73238,
    SPELL_DEAD                               = 81238, // Dead look.

    // Earthrager Ptah
    SPELL_SELF_ROOT                          = 42716,
    SPELL_FREEZE_NO_TURN                     = 93199
};

enum BossPhases
{
    PHASE_NORMAL                             = 1,
    PHASE_50_PER                             = 2
};

const Position SpawnPosition[8] =
{
    {-542.85f,  -389.745f, 158.871f},
    {-526.344f, -430.457f, 155.513f},
    {-521.232f, -376.484f, 156.975f},
    {-516.064f, -405.301f, 155.591f},
    {-503.511f, -373.971f, 155.352f},
    {-493.174f, -433.977f, 156.578f},
    {-479.587f, -398.807f, 156.877f},
    {-475.642f, -423.303f, 158.438f}
};

class boss_earthrager_ptah : public CreatureScript
{
    public:
        boss_earthrager_ptah() : CreatureScript("boss_earthrager_ptah") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_earthrager_ptahAI(creature);
        }

        struct boss_earthrager_ptahAI : public ScriptedAI
        {
            boss_earthrager_ptahAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            std::list<uint64> SummonList;
            InstanceScript* instance;

            uint8 Phase;
            bool Phased;
            uint8 SpawnCount;
            uint8 PhaseCount;

            uint32 FlameBoltTimer;
            uint32 RagingSmashTimer;
            uint32 EarthSpikeTimer;
            uint32 QuickSandTimer;

            void Reset() OVERRIDE
            {
                if (instance)
                   instance->SetData(DATA_EARTHRAGER_PTAH_EVENT, NOT_STARTED);

                Phase = PHASE_NORMAL;
                Phased = false;
                PhaseCount = 0;
                FlameBoltTimer = 7000;
                RagingSmashTimer = 4000;
                EarthSpikeTimer = 12000;

                if (me->HasAura(SPELL_SELF_ROOT))
                {
                    me->RemoveAura(SPELL_SELF_ROOT);
                    me->RemoveAura(SPELL_FREEZE_NO_TURN);
                }

                Movement::MoveSplineInit init(*me);
                init.SetOrientationFixed(false);
                init.Launch();

                me->RemoveAurasDueToSpell(SPELL_DEAD); // Dead look
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            }

            void EnterEvadeMode() OVERRIDE
            {
                RemoveSummons();
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->GetMotionMaster()->MoveTargetedHome();
                
                if (instance)
                {
                    instance->SetData(DATA_EARTHRAGER_PTAH_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void SummonedCreatureDespawn(Creature* summon) OVERRIDE
            {
                switch(summon->GetEntry())
                {
                    case NPC_DUSTBONE_HORROR:
                    case NPC_JEWELED_SCARAB:
                        SpawnCount--;
                        break;
                }
            }

            void RemoveSummons()
            {
                if (SummonList.empty())
                    return;

                for (std::list<uint64>::const_iterator itr = SummonList.begin(); itr != SummonList.end(); ++itr)
                {
                    if (Creature* pTemp = Unit::GetCreature(*me, *itr))
                        if (pTemp)
                            pTemp->DisappearAndDie();
                }

                SummonList.clear();
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                switch (summon->GetEntry())
                {
                    case NPC_DUSTBONE_HORROR:
                    case NPC_JEWELED_SCARAB:
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            summon->AI()->AttackStart(pTarget);
                        SummonList.push_back(summon->GetGUID());
                        break;
                }

                SummonList.push_back(summon);
            }

            void ChangePhase()
            {
                me->SetFacingTo(0.0f);
                Movement::MoveSplineInit init(*me);
                init.SetOrientationFixed(true);
                init.Launch();

                QuickSandTimer = 1000;
                me->AddAura(SPELL_DEAD, me); // Dead
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                Phased = true;
 
                DoCastAOE(SPELL_EARTHQUAKE_VISUAL);
                me->AddAura(SPELL_SELF_ROOT, me);
                me->AddAura(SPELL_FREEZE_NO_TURN, me);

                SetWeather(WEATHER_STATE_LIGHT_SANDSTORM);
                SpawnCount = 8;
                
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true, -SPELL_EARTH_SPIKE))
                    target->CastSpell(target, SPELL_EARTHQUAKE_VISUAL,1);

                for(uint32 x = 0; x < 6; ++x)
                    me->SummonCreature(NPC_JEWELED_SCARAB, SpawnPosition[x], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

                for(uint32 x = 0; x < 2; ++x)
                    me->SummonCreature(NPC_DUSTBONE_HORROR, SpawnPosition[6+x], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

                if (IsHeroic())
                    me->SummonCreature(42087, me->GetPositionX()+5, me->GetPositionY()+5, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 60000);
            }

            void SetWeather(uint32 weather)
            {
                Map* map = me->GetMap();
                if (!map->IsDungeon())
                    return;
            
                WorldPacket data(SMSG_WEATHER, 9);
                data << uint32(weather) << float(0.5f) << uint8(0);
                map->SendToPlayers(&data);
            }

            void KilledUnit(Unit* /*killed*/) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                Talk(SAY_DEATH);
                if (instance)
                {
                    instance->SetData(DATA_EARTHRAGER_PTAH_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); //Remove
                }
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);
                if (instance)
                {
                    instance->SetData(DATA_EARTHRAGER_PTAH_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                   return;
                
                if (me->HealthBelowPct(51) && Phase == PHASE_NORMAL && PhaseCount == 0)
                {
                    ChangePhase();
                    PhaseCount++;
                    Phase = PHASE_50_PER;
                }

                if (SpawnCount == 0 && Phase == PHASE_50_PER && PhaseCount == 1)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->RemoveAurasDueToSpell(SPELL_DEAD); // Dead Look
                    me->RemoveAura(SPELL_SELF_ROOT);
                    me->RemoveAura(SPELL_FREEZE_NO_TURN);

                    Movement::MoveSplineInit init(*me);
                    init.SetOrientationFixed(false);
                    init.Launch();

                    Phase = PHASE_NORMAL;
                    Phased = false;
                    SetWeather(WEATHER_STATE_SUNNY);
                    FlameBoltTimer = urand(7000, 11000);
                    RagingSmashTimer = 4000;
                    EarthSpikeTimer = 12000;
                }

                if (Phase == PHASE_NORMAL)
                {
                    if (FlameBoltTimer <= diff)
                    {
                        DoCast(me, SPELL_FLAME_BOLT);
                        FlameBoltTimer = urand(13000, 15000);
                    } else FlameBoltTimer -= diff;

                    if (RagingSmashTimer <= diff)
                    {
                        DoCastVictim(SPELL_RAGING_SMASH);
                        RagingSmashTimer = urand(8000, 11000);
                    } else RagingSmashTimer -= diff;

                    if (EarthSpikeTimer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_EARTH_SPIKE);
                        EarthSpikeTimer = 15000;
                    } else EarthSpikeTimer -= diff;
                }

                if (Phase == PHASE_50_PER)
                {
                    if (QuickSandTimer <= diff)
                    {
                        if(Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        {
                            Creature* Quicksand = me->SummonCreature(40503, victim->GetPositionX() + urand(3, 9), victim->GetPositionY() + urand(3, 9), victim->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, IsHeroic() ? 15000 : 24000);
                            DoZoneInCombat(Quicksand);
                        }
                        QuickSandTimer = 10000;
                    }
                    else
                        QuickSandTimer -= diff;
                }

                if (Phase == PHASE_NORMAL)
                    DoMeleeAttackIfReady();
            }
        };
};

class npc_horror : public CreatureScript
{
    public:
        npc_horror() : CreatureScript("npc_horror") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_horrorAI(creature);
        }

        struct npc_horrorAI : public ScriptedAI
        {
            npc_horrorAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                m_uiSmashTimer = urand(5000, 10000);
            }

            InstanceScript* instance;
            uint32 m_uiSmashTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (m_uiSmashTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_SMASH);
                    m_uiSmashTimer = urand(7000, 12000);
                }
                else
                    m_uiSmashTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_quicksand : public CreatureScript
{
    public:
        npc_quicksand() : CreatureScript("npc_quicksand") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_quicksandAI(creature);
        }
            
        struct npc_quicksandAI : public ScriptedAI
        {
            npc_quicksandAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->CastSpell(creature, SPELL_QUICKSAND_SLOW);
                creature->CastSpell(creature, SPELL_QUICKSAND_DMG);
                creature->AddAura(SPELL_FREEZE_NO_TURN, creature);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };
};

class npc_sandstorm : public CreatureScript
{
    public:
        npc_sandstorm() : CreatureScript("npc_sandstorm") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_sandstormAI(creature);
        }

        struct npc_sandstormAI : public ScriptedAI
        {
            npc_sandstormAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, false);
                creature->SetSpeed(MOVE_WALK, 0.8f);
                creature->SetSpeed(MOVE_RUN, 0.7f);
                creature->CastSpell(creature, SPELL_SAND_VORTEX, false);
                creature->GetMotionMaster()->MovementExpired();
                creature->GetMotionMaster()->Clear();
                creature->GetMotionMaster()->MoveRandom(30.0f);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };
};

void AddSC_boss_earthrager_ptah()
{
    new boss_earthrager_ptah();
    new npc_quicksand();
    new npc_horror();
    new npc_sandstorm();
}
