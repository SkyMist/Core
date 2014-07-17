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

class OrientationCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit OrientationCheck(Unit* caster) : _caster(caster) { }

        bool operator() (WorldObject* unit) const
        {
            return !unit->HasInArc(M_PI / 2, _caster);
        }

    private:
        Unit* _caster;
};

enum Yells
{
    SAY_AGGRO      = 0,
    SAY_SUPERNOVA  = 1,
    SAY_KILL       = 2,
    SAY_DEATH      = 3,
    SAY_SPLIT      = -1900026,
};

enum Spells
{
    SPELL_SUPERNOVA             = 74136,// requires a spell link
    SPELL_ASTRAL_RAIN_P1        = 74135,
    SPELL_ASTRAL_RAIN_P2        = 74366,
    SPELL_ASTRAL_RAIN_P3        = 74370,
    SPELL_CELESTIAL_CALL_P1     = 74362,
    SPELL_CELESTIAL_CALL_P2     = 74355,
    SPELL_CELESTIAL_CALL_P3     = 74364,
    SPELL_VEIL_OF_SKY_P1        = 74133,
    SPELL_VEIL_OF_SKY_P2        = 74372,
    SPELL_VEIL_OF_SKY_P3        = 74373,
    SPELL_ARCANE_BARRAGE        = 74374,
    SPELL_ARCANE_BARRAGE_H      = 89886,
    SPELL_VEIL_SKY              = 74133,
    SPELL_VEIL_SKY_H            = 90760,
    SPELL_FAMILIAR_VISUAL       = 64367,

    SPELL_SUMMON_ENERGY_FLUX    = 74042, // Summon mob 39613, used by Spatial Flux portal 39612. 6 sec duration.
    SPELL_ENERGY_FLUX           = 74043, // On 39613 Energy Flux mob.
    SPELL_ENERGY_FLUX_PERIODIC  = 74044  // Periodic damage trigger 74045, mob aura.
};

class boss_isiset : public CreatureScript
{
    public:
        boss_isiset() : CreatureScript("boss_isiset") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_isisetAI(creature);
        }

        struct boss_isisetAI : public ScriptedAI
        {
            boss_isisetAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                SetCombatMovement(true);
            }

            std::list<uint64> SummonList;
            InstanceScript* instance;

            uint32 SupernovaTimer;
            uint32 AstralRainPhase3Timer;
            uint32 AstralRainPhase2Timer;
            uint32 AstralRainPhase1Timer;
            uint32 CelestialCallPhase3Timer;
            uint32 CelestialCallPhase2Timer;
            uint32 CelestialCallPhase1Timer;
            uint32 VeilOfSkyPhase3Timer;
            uint32 VeilOfSkyPhase2Timer;
            uint32 VeilOfSkyPhase1Timer;
            uint32 Phase;

            bool Phased;
            bool AstralRain, VeilOfSky, CelestialCall, AstralRain2, VeilOfSky2, CelestialCall2;

            void Reset() OVERRIDE
            {
                if (instance)
                   instance->SetData(DATA_ISISET_EVENT, NOT_STARTED);

                SupernovaTimer = urand(15000, 20000);
                AstralRainPhase3Timer = 10000;
                AstralRainPhase2Timer = 10000;
                AstralRainPhase1Timer = 10000;
                CelestialCallPhase3Timer = 25000;
                CelestialCallPhase2Timer = 25000;
                CelestialCallPhase1Timer = 25000;
                VeilOfSkyPhase3Timer = 20000;
                VeilOfSkyPhase2Timer = 20000;
                VeilOfSkyPhase1Timer = 20000;
                Phased = false;
                AstralRain = false;
                VeilOfSky = false;
                CelestialCall = false;
                AstralRain2 = false;
                VeilOfSky2 = false;
                CelestialCall2 = false;
                Phase = 0;
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
                    instance->SetData(DATA_ISISET_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void EnterCombat(Unit* who) OVERRIDE
            {
                Talk(SAY_AGGRO);
                if (IsHeroic()) me->SummonCreature(39612, -484.982f, 419.362f, 343.944f, 3.182f, TEMPSUMMON_MANUAL_DESPAWN); // Portal.

                if (instance)
                {
                    instance->SetData(DATA_ISISET_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                }
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* killer) OVERRIDE
            {
                Talk(SAY_DEATH);
                RemoveSummons();
                if (instance)
                {
                    instance->SetData(DATA_ISISET_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); //Remove
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) OVERRIDE
            {
                switch(summon->GetEntry())
                {
                    case 39720: // Astral Rain
                        if (Phase == 1)
                            AstralRain = false;
                        else if (Phase == 2)
                            AstralRain2 = false;
                        break;
                    case 39721: // Celestial Call
                        if (Phase == 1)
                            CelestialCall = false;
                        else if (Phase == 2)
                            CelestialCall2 = false;
                        break;
                    case 39722: // Veil of Sky
                        if (Phase == 1)
                            VeilOfSky = false;
                        else if (Phase == 2)
                            VeilOfSky2 = false;
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
                summon->setActive(true);
                if (me->isInCombat())
                    summon->AI()->DoZoneInCombat();
                SummonList.push_back(summon->GetGUID());
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (me->GetPower(POWER_MANA) <= 100)
                {
                    me->RemoveAurasDueToSpell(SPELL_VEIL_OF_SKY_P1);
                    me->RemoveAurasDueToSpell(SPELL_VEIL_OF_SKY_P2);
                    me->RemoveAurasDueToSpell(SPELL_VEIL_OF_SKY_P3);
                }

                if (Phased)
                    me->SetVisible(false);

                if (!Phased)
                    me->SetVisible(true);

                if (me->HealthBelowPct(67) && Phase == 0)
                {
                    DoScriptText(SAY_SPLIT, me);
                    me->SetReactState(REACT_PASSIVE);
                    Phased = true;
                    AstralRain = true;
                    VeilOfSky = true;
                    CelestialCall = true;
                    Position pos;
                    me->GetPosition(&pos);
                    me->SummonCreature(39720, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    me->SummonCreature(39721, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    me->SummonCreature(39722, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    Phase = 1;
                }

                if (HealthBelowPct(34) && Phase == 1)
                {
                    DoScriptText(SAY_SPLIT, me);
                    me->SetReactState(REACT_PASSIVE);
                    Phased = true;
                    Position pos;
                    me->GetPosition(&pos);
                    if (AstralRain == false) // Make other two visible again.
                    {
                        VeilOfSky2 = true;
                        CelestialCall2 = true;
                        me->SummonCreature(39721, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                        me->SummonCreature(39722, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    }
                    if (CelestialCall == false)
                    {
                        AstralRain2 = true;
                        VeilOfSky2 = true;
                        me->SummonCreature(39722, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                        me->SummonCreature(39720, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    }
                    if (VeilOfSky == false)
                    {
                        AstralRain2 = true;
                        CelestialCall2 = true;
                        me->SummonCreature(39721, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                        me->SummonCreature(39720, pos, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                    }
                    Phase = 2;
                }

                if (Phase == 0)
                {
                    Phased = false;
                    
                    if (CelestialCallPhase1Timer <= diff)
                    {
                        DoCast(me, SPELL_CELESTIAL_CALL_P1);
                        CelestialCallPhase1Timer = 45000;
                    } else CelestialCallPhase1Timer -= diff;

                    if (VeilOfSkyPhase1Timer <= diff)
                    {
                        DoCast(me, SPELL_VEIL_OF_SKY_P1);
                        VeilOfSkyPhase1Timer = 45000;
                    } else VeilOfSkyPhase1Timer -= diff;

                    if (AstralRainPhase1Timer <= diff)
                    {
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P1);
                        AstralRainPhase1Timer = 10000;
                    } else AstralRainPhase1Timer -= diff;
                }

                if (Phase == 1)
                {
                    if (CelestialCall == false)
                    {
                        if(Creature* veil = me->FindNearestCreature(39722, 300, true))
                            veil->DespawnOrUnsummon();
                        if(Creature* astral = me->FindNearestCreature(39720, 300, true))
                            astral->DespawnOrUnsummon();

                        me->SetReactState(REACT_AGGRESSIVE);
                        Phased = false;

                        if (VeilOfSkyPhase2Timer <= diff)
                        {
                            DoCast(me, SPELL_VEIL_OF_SKY_P2);
                            VeilOfSkyPhase2Timer = 45000;
                        } else VeilOfSkyPhase2Timer -= diff;
                        
                        if (AstralRainPhase2Timer <= diff)
                        {
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P2);
                            AstralRainPhase2Timer = 10000;
                        } else AstralRainPhase2Timer -= diff;
                    }

                    if (VeilOfSky == false)
                    {
                        if(Creature* celestial = me->FindNearestCreature(39721, 300, true))
                            celestial->DespawnOrUnsummon();
                        if(Creature* astral = me->FindNearestCreature(39720, 300, true))
                            astral->DespawnOrUnsummon();

                        me->SetReactState(REACT_AGGRESSIVE);
                        Phased = false;

                        if (CelestialCallPhase2Timer <= diff)
                        {
                            DoCast(me, SPELL_CELESTIAL_CALL_P2);
                            CelestialCallPhase2Timer = 45000;
                        } else CelestialCallPhase2Timer -= diff;

                        if (AstralRainPhase2Timer <= diff)
                        {
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P2);
                            AstralRainPhase2Timer = 10000;
                        } else AstralRainPhase2Timer -= diff;
                    }
                    
                    if (AstralRain == false)
                    {
                        if(Creature* celestial = me->FindNearestCreature(39721, 300, true))
                            celestial->DespawnOrUnsummon();
                        if(Creature* veil = me->FindNearestCreature(39722, 300, true))
                            veil->DespawnOrUnsummon();

                        me->SetReactState(REACT_AGGRESSIVE);
                        Phased = false;
                       
                        if (CelestialCallPhase2Timer <= diff)
                        {
                            DoCast(me, SPELL_CELESTIAL_CALL_P2);
                            CelestialCallPhase2Timer = 45000;
                        } else CelestialCallPhase2Timer -= diff;
                    
                        if (VeilOfSkyPhase2Timer <= diff)
                        {
                            DoCast(me, SPELL_VEIL_OF_SKY_P2);
                            VeilOfSkyPhase2Timer = 45000;
                        } else VeilOfSkyPhase2Timer -= diff;
                    }
                }

                if (Phase == 2)
                {
                    if (CelestialCall2 == false)
                    if (CelestialCall == true) // First one not dead, dead in second phase.
                    {
                        if(Creature* veil = me->FindNearestCreature(39722, 300, true))
                            veil->DespawnOrUnsummon();
                        if(Creature* astral = me->FindNearestCreature(39720, 300, true))
                            astral->DespawnOrUnsummon();
                        
                        me->SetReactState(REACT_AGGRESSIVE);
                        Phased = false;
                        
                        if (VeilOfSkyPhase3Timer <= diff)
                        {
                            DoCast(me, SPELL_VEIL_OF_SKY_P3);
                            VeilOfSkyPhase3Timer = 45000;
                        } else VeilOfSkyPhase3Timer -= diff;
                        
                        if (AstralRainPhase3Timer <= diff)
                        {
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P3);
                            AstralRainPhase3Timer = 10000;
                        } else AstralRainPhase3Timer -= diff;
                    }

                    if (VeilOfSky2 == false)
                    if (VeilOfSky == true) // First one not dead, dead in second phase.
                    {
                        if(Creature* celestial = me->FindNearestCreature(39721, 300, true))
                            celestial->DespawnOrUnsummon();
                        if(Creature* astral = me->FindNearestCreature(39720, 300, true))
                            astral->DespawnOrUnsummon();
                       
                        me->SetReactState(REACT_AGGRESSIVE);
                        Phased = false;

                        if (CelestialCallPhase3Timer <= diff)
                        {
                            DoCast(me, SPELL_CELESTIAL_CALL_P3);
                            CelestialCallPhase3Timer = 45000;
                        } else CelestialCallPhase3Timer -= diff;
                
                        if (AstralRainPhase3Timer <= diff)
                        {
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P3);
                            AstralRainPhase3Timer = 10000;
                        } else AstralRainPhase3Timer -= diff;
                    }
                    
                    if (AstralRain2 == false)
                    if (AstralRain == true) // First one not dead, dead in second phase.
                    {
                        if(Creature* celestial = me->FindNearestCreature(39721, 300, true))
                            celestial->DespawnOrUnsummon();
                        if(Creature* veil = me->FindNearestCreature(39722, 300, true))
                            veil->DespawnOrUnsummon();

                        me->SetReactState(REACT_AGGRESSIVE);                       
                        Phased = false;
                       
                        if (CelestialCallPhase3Timer <= diff)
                        {
                            DoCast(me, SPELL_CELESTIAL_CALL_P3);
                            CelestialCallPhase3Timer = 45000;
                        } else CelestialCallPhase3Timer -= diff;
                    
                        if (VeilOfSkyPhase3Timer <= diff)
                        {
                            DoCast(me, SPELL_VEIL_OF_SKY_P3);
                            VeilOfSkyPhase3Timer = 45000;
                        } else VeilOfSkyPhase3Timer -= diff;
                    }
                }

                if (SupernovaTimer <= diff)
                {
                    if (!Phased)
                    {
                        Talk(SAY_SUPERNOVA);
                        DoCast(me->getVictim(), SPELL_SUPERNOVA);
                    }
                    SupernovaTimer = urand(25000, 35000);
                } else SupernovaTimer -= diff;

                if (!Phased)
                    DoMeleeAttackIfReady();
            }
        };
};

class spell_isiset_supernova : public SpellScriptLoader
{
    public:
        spell_isiset_supernova() : SpellScriptLoader("spell_isiset_supernova") { }

        class spell_isiset_supernova_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_isiset_supernova_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(OrientationCheck(GetCaster()));
            }

            void Register() OVERRIDE
            {
               OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_isiset_supernova_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_isiset_supernova_SpellScript();
        }
};

class npc_celestial_familiar : public CreatureScript
{
    public:
        npc_celestial_familiar() : CreatureScript("npc_celestial_familiar") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_celestial_familiarAI(creature);
        }
            
        struct npc_celestial_familiarAI : public ScriptedAI
        {
            npc_celestial_familiarAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                creature->SetDisplayId(25347);
                creature->AddAura(SPELL_FAMILIAR_VISUAL, creature);
                m_uiBarrageTimer = 1000;
            }

            InstanceScript* instance;
            uint32 m_uiBarrageTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiBarrageTimer <= diff)
                {
                    DoCast(me->getVictim(), IsHeroic() ? SPELL_ARCANE_BARRAGE_H : SPELL_ARCANE_BARRAGE);
                    m_uiBarrageTimer = urand(2000, 3000);
                }
                else
                    m_uiBarrageTimer -= diff;
            }
        };
};

class npc_veil_sky : public CreatureScript
{
    public:
        npc_veil_sky() : CreatureScript("npc_veil_sky") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_veil_skyAI(creature);
        }
            
        struct npc_veil_skyAI : public ScriptedAI
        {
            npc_veil_skyAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                ASSERT(creature->GetVehicleKit()); // we dont actually use it, just check if exists
                creature->SetMaxPower(POWER_MANA, IsHeroic() ? 147250 : 97360);
                creature->SetPower(POWER_MANA, IsHeroic() ? 147250 : 97360);
                m_uiVeilSkyTimer = 2000;
            }

            InstanceScript* instance;
            uint32 m_uiVeilSkyTimer;
            
            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiVeilSkyTimer <= diff)
                {
                    DoCast(me, IsHeroic() ? SPELL_VEIL_SKY_H : SPELL_VEIL_SKY);
                    m_uiVeilSkyTimer = 60000;
                }
                else m_uiVeilSkyTimer -= uiDiff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_celestial_call : public CreatureScript
{
    public:
        npc_celestial_call() : CreatureScript("npc_celestial_call") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_celestial_callAI(creature);
        }
            
        struct npc_celestial_callAI : public ScriptedAI
        {
            npc_celestial_callAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                ASSERT(creature->GetVehicleKit()); // we dont actually use it, just check if exists
                creature->SetMaxPower(POWER_ENERGY, 100);
                creature->SetPower(POWER_ENERGY, 100);
                m_uiBarrageTimer = 1000;
            }

            InstanceScript* instance;
            uint32 m_uiBarrageTimer;
            
            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiBarrageTimer <= diff)
                {
                    DoCast(me->getVictim(), IsHeroic() ? SPELL_ARCANE_BARRAGE_H : SPELL_ARCANE_BARRAGE);
                    m_uiBarrageTimer = 2000;
                }
                else m_uiBarrageTimer -= diff;
            }
        };
};

class npc_astral_rain : public CreatureScript
{
    public:
        npc_astral_rain() : CreatureScript("npc_astral_rain") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_astral_rainAI(creature);
        }
            
        struct npc_astral_rainAI : public ScriptedAI
        {
            npc_astral_rainAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                ASSERT(creature->GetVehicleKit()); // we dont actually use it, just check if exists
                creature->SetMaxPower(POWER_ENERGY, 100);
                creature->SetPower(POWER_ENERGY, 100);
                m_uiAstralRainTimer = 2000;
            }

            InstanceScript* instance;
            uint32 m_uiAstralRainTimer;
            
            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiAstralRainTimer <= diff)
                {
                    Creature* CelestialCall = me->FindNearestCreature(39721, 100);
                    Creature* VeilSky = me->FindNearestCreature(39722, 100);
                    if (CelestialCall && VeilSky)
                    {
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P1);
                    }
                    else 
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true), SPELL_ASTRAL_RAIN_P2);
                    m_uiAstralRainTimer = 8000;
                }
                else m_uiAstralRainTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_spatial_flux : public CreatureScript // 39612
{
    public:
        npc_spatial_flux() : CreatureScript("npc_spatial_flux") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_spatial_fluxAI(creature);
        }

        struct npc_spatial_fluxAI : public ScriptedAI
        {
            npc_spatial_fluxAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                creature->SetReactState(REACT_PASSIVE);
                m_uiEnergyFluxTimer = 3000;
            }

            InstanceScript* instance;
            uint32 m_uiEnergyFluxTimer;

            void JustSummoned(Creature* summon) OVERRIDE
            {
                summon->setActive(true);
                summon->AI()->DoZoneInCombat();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (m_uiEnergyFluxTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                    if (Creature* flux = me->SummonCreature(39613, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                    {
						flux->SetSpeed(MOVE_WALK, 0.8f, true);
						flux->SetSpeed(MOVE_RUN, 0.8f, true);
						flux->AddThreat(target, 5000000.0f);
                        flux->GetMotionMaster()->MovementExpired();
                        flux->GetMotionMaster()->Clear();
						flux->GetMotionMaster()->MoveChase(target);
                        flux->SetReactState(REACT_PASSIVE);
                        DoCast(flux, SPELL_ENERGY_FLUX);
                        flux->AddAura(SPELL_ENERGY_FLUX_PERIODIC, flux);
                        flux->DespawnOrUnsummon(6000);
                    }

                    m_uiEnergyFluxTimer = 10000;
                }
                else
                    m_uiEnergyFluxTimer -= diff;
			}
        };
};

void AddSC_boss_isiset()
{
    new boss_isiset();
    new spell_isiset_supernova();
    new npc_celestial_familiar();
    new npc_celestial_call();
    new npc_veil_sky();
    new npc_astral_rain();
    new npc_spatial_flux();
}
