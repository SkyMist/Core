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

#define SAY_REIG "Setesh begins casting Reign of Chaos!"

enum Spells
{
    // NPCs
    SPELL_PORTAL_VISUAL                      = 76714,
    SPELL_CHANNEL_CHAOS_PORTAL               = 76784,
    SPELL_MAGIC_PRISON                       = 76903,
    SPELL_VOID_BARRIER                       = 76959,
    SPELL_CHARGED_FISTS                      = 77238,

    // Setesh
    SPELL_CHAOS_BOLT                         = 77069,
    SPELL_CHAOS_BOLT_H                       = 89873,
    SPELL_REIGN_OF_CHAOS_N                   = 77213,
    SPELL_REIGN_OF_CHAOS_H                   = 89870,
    SPELL_REIGN_OF_CHAOS_CAST                = 77023, //casted by the boss
    SPELL_REIGN_OF_CHAOS_AURA                = 77026, //addaura to boss (visual) (77023)
    SPELL_SEED_OF_CHAOS_N                    = 76870, //to be used with both DoCast & AddAura
    SPELL_SEED_OF_CHAOS_H                    = 89867, //same
    SPELL_SEED_OF_CHAOS_VISUAL               = 76865,

    SPELL_CHAOS_BLAST                        = 76676, // Needs pre-aura 76681 on mob.
    SPELL_CHAOS_BLAST_VOIDZONE               = 76681  // First effect is damage from missile hit.
};

enum Texts
{
    SAY_AGGRO                                = 0,
    SAY_REIGN                                = 1,
    SAY_KILL                                 = 2,
    SAY_DEATH                                = 3
};

enum Creatures
{
    NPC_PORTAL                               = 41055,
    NPC_VOID_SENTINEL                        = 41208,
    NPC_VOID_WURM                            = 41212,
    NPC_VOID_SEEKER                          = 41371,
    NPC_SEED_OF_CHAOS                        = 41126
};

class boss_setesh : public CreatureScript
{
    public:
        boss_setesh() : CreatureScript("boss_setesh") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_seteshAI(creature);
        }
            
        struct boss_seteshAI : public ScriptedAI
        {
            boss_seteshAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            uint32 m_uiChaosBoltTimer;
            uint32 m_uiPortalTimer;
            uint32 m_uiShadowCrashTimer;
            uint32 m_uiSeedofChaosTimer;
            uint32 m_uiReignofChaosTimer;
            SummonList summons;

            void Reset() OVERRIDE
            {
                if (instance)
                    instance->SetData(DATA_SETESH_EVENT, NOT_STARTED);

                m_uiChaosBoltTimer = 1000;
                m_uiPortalTimer = 10000;
                m_uiShadowCrashTimer = 7000;
                m_uiReignofChaosTimer = IsHeroic() ? 20000 : 30000;
                m_uiSeedofChaosTimer = IsHeroic() ? 6000 : 9000;
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);
                if (instance)
                {
                    instance->SetData(DATA_SETESH_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* killer) OVERRIDE
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();

                if (instance)
                {
                    instance->SetData(DATA_SETESH_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                summon->setActive(true);
                if (me->IsInCombat())
                    summon->AI()->DoZoneInCombat();
                summons.Summon(summon);
            }

            void EnterEvadeMode() OVERRIDE
            {
                summons.DespawnAll();
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->GetMotionMaster()->MoveTargetedHome();
                
                if (instance)
                {
                    instance->SetData(DATA_SETESH_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiChaosBoltTimer <= diff)
                {
                    if(Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(victim, IsHeroic() ? SPELL_CHAOS_BOLT_H : SPELL_CHAOS_BOLT);
                    m_uiChaosBoltTimer = 2500;
                }
                else m_uiChaosBoltTimer -= diff;

                if (m_uiShadowCrashTimer <= diff)
                {
                    if(Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        if (Creature* ChaosBlast = me->SummonCreature(41041, victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 35000))
                            DoCast(ChaosBlast, SPELL_CHAOS_BLAST);
                    m_uiShadowCrashTimer = urand(17000, 21000);
                }
                else m_uiShadowCrashTimer -= diff;
                    
                if (m_uiSeedofChaosTimer <= diff)
                {
                    me->CastStop();
                    me->SummonCreature(NPC_SEED_OF_CHAOS, me->GetPositionX() + urand(-40, 40), me->GetPositionY() + urand(-40, 40), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 25000);
                    m_uiSeedofChaosTimer = urand(15000, 20000);
                }
                else m_uiSeedofChaosTimer -= diff;

                if (m_uiReignofChaosTimer <= diff)
                {
                    me->CastStop();
                    DoCast(me, SPELL_REIGN_OF_CHAOS_CAST, true);
                    Talk(SAY_REIGN);
                    me->MonsterTextEmote(SAY_REIG, NULL, true);
                    m_uiReignofChaosTimer = urand(23000,27000);
                    m_uiChaosBoltTimer = 8000;
                    m_uiSeedofChaosTimer = urand(7000, 12000);
                }
                else m_uiReignofChaosTimer -= diff;

                if (m_uiPortalTimer <= diff)
                {
                    me->CastStop();
                    float x, y, z;
                    me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 15.0f);
                    if (Creature* portal = me->SummonCreature(NPC_PORTAL, x, y, z, 0, TEMPSUMMON_MANUAL_DESPAWN))
                        DoCast(portal, SPELL_CHANNEL_CHAOS_PORTAL);
                    m_uiPortalTimer = IsHeroic() ? urand(22000, 27000) : urand(27000, 32000);
                }
                else m_uiPortalTimer -= diff;            
            }
        };
};

class npc_portal : public CreatureScript
{
    public:
        npc_portal() : CreatureScript("npc_portal") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_portalAI(creature);
        }
            
        struct npc_portalAI : public ScriptedAI
        {
            npc_portalAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                m_uiAuraTimer = 6000;
                m_uiWurmsTimer = IsHeroic() ? 13000 : 7000;
                m_uiSeekerTimer = IsHeroic()? 23000 : 19000;
                m_uiSentinelTimer = IsHeroic()? 7000 : 34000;

                if (!IsHeroic())
                    creature->DespawnOrUnsummon(36000);
            }

            InstanceScript* instance;
            uint32 m_uiSentinelTimer;
            uint32 m_uiWurmsTimer;
            uint32 m_uiSeekerTimer;
            uint32 m_uiAuraTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (m_uiAuraTimer <= diff)
                {
                    if (!me->HasAura(SPELL_PORTAL_VISUAL))
                        DoCast(me, SPELL_PORTAL_VISUAL);
                    m_uiAuraTimer = -1;
                }
                else m_uiAuraTimer -= diff;

                if (m_uiSentinelTimer <= diff)
                {
                    Creature* sentinel = me->SummonCreature(NPC_VOID_SENTINEL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(sentinel);
                    m_uiSentinelTimer = -1; // Only one sentinel ever appears.
                }
                else m_uiSentinelTimer -= diff;

                if (m_uiWurmsTimer <= diff)
                {
                    Creature* wurm1 = me->SummonCreature(NPC_VOID_WURM, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    Creature* wurm2 = me->SummonCreature(NPC_VOID_WURM, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(wurm1);
                    DoZoneInCombat(wurm2);
                    m_uiWurmsTimer = IsHeroic() ? 15000 : -1; // On heroic they spawn each 15 secs till you kill it.
                }
                else m_uiWurmsTimer -= diff;
                
                if (m_uiSeekerTimer <= diff)
                {
                    Creature* seeker = me->SummonCreature(NPC_VOID_SEEKER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(seeker);
                    m_uiSeekerTimer = IsHeroic() ? 30000 : -1; // On heroic they spawn each 30 secs till you kill it.
                }
                else m_uiSeekerTimer -= diff;
            }
        };
};

class npc_sentinel : public CreatureScript
{
    public:
        npc_sentinel() : CreatureScript("npc_sentinel") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_sentinelAI(creature);
        }
            
        struct npc_sentinelAI : public ScriptedAI
        {
            npc_sentinelAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                if (IsHeroic())
                    creature->AddAura(SPELL_VOID_BARRIER, creature);
                m_uiFistsTimer = urand(5000, 8000);
            }

            InstanceScript* instance;
            uint32 m_uiFistsTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (m_uiFistsTimer <= diff)
                {
                    DoCast(me, SPELL_CHARGED_FISTS);
                    m_uiFistsTimer = urand(25000, 30000);
                }
                else m_uiFistsTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_seeker : public CreatureScript
{
    public:
        npc_seeker() : CreatureScript("npc_seeker") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_seekerAI(creature);
        }
            
        struct npc_seekerAI : public ScriptedAI
        {
            npc_seekerAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                m_uiPrisonTimer = 5000;
            }

            InstanceScript* instance;
            uint32 m_uiPrisonTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiPrisonTimer <= diff)
                {
                    if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(victim, SPELL_MAGIC_PRISON);
                    m_uiPrisonTimer = urand(35000, 40000);
                }
                else m_uiPrisonTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_seed_of_chaos : public CreatureScript
{
    public:
        npc_seed_of_chaos() : CreatureScript("npc_seed_of_chaos") { }

        struct npc_seed_of_chaosAI : public ScriptedAI
        {
            npc_seed_of_chaosAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->CastSpell(creature, SPELL_SEED_OF_CHAOS_VISUAL);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                creature->setFaction(14);
                creature->DespawnOrUnsummon(26000);
                DoStartNoMovement(creature);
                m_uiCheckTimer = 1000;
            }

            InstanceScript* instance;
            uint32 m_uiCheckTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiCheckTimer <= diff)
                {
                    if (Unit* target = me->FindNearestPlayer(4.0f, true))
                    {
                        if (target->IsWithinDistInMap(me, 4.0f))
                        {
                            me->AddAura(DUNGEON_MODE(SPELL_SEED_OF_CHAOS_N, SPELL_SEED_OF_CHAOS_H), target);
                            DoCast(me, DUNGEON_MODE(SPELL_SEED_OF_CHAOS_N, SPELL_SEED_OF_CHAOS_H));
                            me->DespawnOrUnsummon();
                        }
                    }

                    m_uiCheckTimer = 1000;
                }
                else m_uiCheckTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_seed_of_chaosAI(creature);
        }
};

class npc_reign_of_chaos : public CreatureScript
{
    public:
        npc_reign_of_chaos() : CreatureScript("npc_reign_of_chaos") { }

        struct npc_reign_of_chaosAI : public ScriptedAI
        {
            npc_reign_of_chaosAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->AddAura(SPELL_REIGN_OF_CHAOS_AURA, creature);
                creature->DespawnOrUnsummon(5100);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_reign_of_chaosAI(creature);
        }
};

class npc_chaos_blast : public CreatureScript
{
    public:
        npc_chaos_blast() : CreatureScript("npc_chaos_blast") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_chaos_blastAI(creature);
        }

        struct npc_chaos_blastAI : public ScriptedAI
        {
            npc_chaos_blastAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                Movement::MoveSplineInit init(creature);
                init.SetOrientationFixed(true);
                init.Launch();
                m_uiAuraTimer = 5000;
            }

            InstanceScript* instance;
            uint32 m_uiAuraTimer;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (m_uiAuraTimer <= diff)
                {
                    DoCast(me, SPELL_CHAOS_BLAST_VOIDZONE);
                    me->AddAura(93199, me);
                    m_uiAuraTimer = -1;
                }
                else m_uiAuraTimer -= diff;
            }
        };
};

void AddSC_boss_setesh()
{
    new boss_setesh();
    new npc_portal();
    new npc_sentinel();
    new npc_seeker();
    new npc_seed_of_chaos();
    new npc_reign_of_chaos();
    new npc_chaos_blast();
}
