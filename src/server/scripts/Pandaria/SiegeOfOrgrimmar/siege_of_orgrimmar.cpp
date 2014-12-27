/*
*
* FUCK CREDITS! (SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass).
*
* Raid: Siege of Orgrimmar.
* Description: Mob scripts.
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
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MoveSplineInit.h"
#include "VehicleDefines.h"
#include "SpellInfo.h"

#include "siege_of_orgrimmar.h"

enum MobYells
{
    FALLEN_POOL_TENDER_SAY_AGGRO = 0,
};

enum MobSpells
{
    SPELL_INITIATE_BUBBLE_SHIELD = 147333,

    SPELL_FPT_BUBBLE_SHIELD_AURA = 147450,
    SPELL_FPT_WATERBOLT          = 147398,
    SPELL_FPT_CORRUPTED_WATER    = 147351,

    SPELL_AD_RUSHING_WATERS      = 147185,
};

enum MobEvents
{
    EVENT_FPT_WATERBOLT          = 1,
    EVENT_FPT_CORRUPTED_WATER,
    EVENT_AD_RUSHING_WATERS,
};

enum Mobs
{
    NPC_TORMENTED_INITIATE       = 73349,
    NPC_FALLEN_POOL_TENDER       = 73342,
    NPC_AQUEOUS_DEFENDER         = 73191,
};

// Tormented Initiate 73349.
class npc_soo_tormented_initiate : public CreatureScript
{
    public:
        npc_soo_tormented_initiate() : CreatureScript("npc_soo_tormented_initiate") { }

        struct npc_soo_tormented_initiate_AI : public ScriptedAI
        {
            npc_soo_tormented_initiate_AI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                std::list<Creature*> nearList;
                GetCreatureListWithEntryInGrid(nearList, me, NPC_TORMENTED_INITIATE, 20.0f);
                GetCreatureListWithEntryInGrid(nearList, me, NPC_FALLEN_POOL_TENDER, 20.0f);
                if (!nearList.empty())
                    for (auto nearMob : nearList)
                        if (!nearMob->isInCombat())
                            nearMob->AI()->DoZoneInCombat();
            }

            void DamageTaken(Unit* who, uint32& damage)
            {
                if (Spell* spell = me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                    if (spell->m_spellInfo->Id == SPELL_INITIATE_BUBBLE_SHIELD)
                        me->InterruptNonMeleeSpells(true);
            }

            void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
            }

            void JustDied(Unit* killer) { }

            void UpdateAI(uint32 const diff)
            {
                // Maintain Bubble Shield cast on Fallen Pool Tender.
                if (!me->HasUnitState(UNIT_STATE_CASTING) && !me->HasUnitState(UNIT_STATE_EVADE) && !me->isInCombat())
                    if (Creature* poolTender = me->FindNearestCreature(NPC_FALLEN_POOL_TENDER, 20.0f, true))
                        DoCast(poolTender, SPELL_INITIATE_BUBBLE_SHIELD);

                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_soo_tormented_initiate_AI(creature);
        }
};

// Fallen Pool Tender 73342.
class npc_soo_fallen_pool_tender : public CreatureScript
{
    public:
        npc_soo_fallen_pool_tender() : CreatureScript("npc_soo_fallen_pool_tender") { }

        struct npc_soo_fallen_pool_tender_AI : public ScriptedAI
        {
            npc_soo_fallen_pool_tender_AI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* who)
            {
                Talk(FALLEN_POOL_TENDER_SAY_AGGRO);

                // Remove Bubble Shield aura.
                if (me->HasAura(SPELL_FPT_BUBBLE_SHIELD_AURA))
                    me->RemoveAurasDueToSpell(SPELL_FPT_BUBBLE_SHIELD_AURA);

                events.ScheduleEvent(EVENT_FPT_WATERBOLT, 2000);
                events.ScheduleEvent(EVENT_FPT_CORRUPTED_WATER, 7000);
            }

            void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
            }

            void JustDied(Unit* killer) { }

            void UpdateAI(uint32 const diff)
            {
                // Maintain Bubble Shield aura OOC.
                if (!me->HasAura(SPELL_FPT_BUBBLE_SHIELD_AURA) && !me->HasUnitState(UNIT_STATE_EVADE) && !me->isInCombat())
                    DoCast(me, SPELL_FPT_BUBBLE_SHIELD_AURA);

                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FPT_WATERBOLT:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_FPT_WATERBOLT);
                            events.ScheduleEvent(EVENT_FPT_WATERBOLT, 3000);
                            break;
                        }

                        case EVENT_FPT_CORRUPTED_WATER:
                        {
                            DoCast(me, SPELL_FPT_CORRUPTED_WATER);
                            events.ScheduleEvent(EVENT_FPT_CORRUPTED_WATER, urand(17000, 20000));
                            break;
                        }

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_soo_fallen_pool_tender_AI(creature);
        }
};

// Aqueous Defender 73191.
class npc_soo_aqueous_defender : public CreatureScript
{
    public:
        npc_soo_aqueous_defender() : CreatureScript("npc_soo_aqueous_defender") { }

        struct npc_soo_aqueous_defender_AI : public ScriptedAI
        {
            npc_soo_aqueous_defender_AI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* who)
            {
                events.ScheduleEvent(EVENT_AD_RUSHING_WATERS, urand(3000, 5000));
            }

            void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
            }

            void JustDied(Unit* killer) { }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_AD_RUSHING_WATERS:
                        {
                            DoCast(me, SPELL_AD_RUSHING_WATERS);
                            events.ScheduleEvent(EVENT_AD_RUSHING_WATERS, urand(20000, 24000));
                            break;
                        }

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_soo_aqueous_defender_AI(creature);
        }
};

void AddSC_siege_of_orgrimmar()
{
    new npc_soo_tormented_initiate();
    new npc_soo_fallen_pool_tender();
    new npc_soo_aqueous_defender();
}
