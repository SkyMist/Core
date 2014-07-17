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

enum Yells
{
    SAY_INTRO                       = 0,
    SAY_AGGRO                       = 1,
    SAY_KILL                        = 2,
    SAY_OMEGA                       = 3,
    SAY_DEATH                       = 4
};

enum BrannYells
{
    BRANN_SAY_DOOR_INTRO            = 0,  // Right, let's go! Just need to input the final entry sequence into the door mechanism... and...
    BRANN_SAY_UNLOCK_DOOR           = 1,  // That did the trick! The control room should be right behind this... oh... wow...
    BRANN_SAY_TROGGS                = 2,  // What? This isn't the control room! There's another entire defense mechanism in place, and the blasted Rock Troggs broke into here somehow. Troggs. Why did it have to be Troggs!
    BRANN_SAY_THINK                 = 3,  // Ok, let me think a moment.
    BRANN_SAY_MIRRORS               = 4,  // Mirrors pointing all over the place.
    BRANN_SAY_ELEMENTALS            = 5,  // Four platforms with huge elementals.
    BRANN_SAY_GET_IT                = 6,  // I got it! I saw a tablet that mentioned this chamber. This is the Vault of Lights! Ok, simple enough. I need you adventurers to take out each of the four elementals to trigger the opening sequence for the far door!
    BRANN_1_ELEMENTAL_DEAD          = 7,  // One down!
    BRANN_2_ELEMENTAL_DEAD          = 8,  // Another one down! Just look at those light beams! They seem to be connecting to the far door!
    BRANN_3_ELEMENTAL_DEAD          = 9,  // One more elemental to go! The door is almost open!
    BRANN_4_ELEMENTAL_DEAD          = 10, // That''s it, you''ve done it! The vault door is opening! Now we can... oh, no!
    BRANN_SAY_ANRAPHET_DIED         = 11, // We''ve done it! The control room is breached!
    BRANN_SAY_MOMENT                = 12  // Here we go! Now this should only take a moment... 
};

enum Spells
{
    SPELL_DESTRUCTION_PROTOCOL      = 77437,

    // Beams NPC
    SPELL_VISUAL_ON_BEAM            = 76912, // Same as 91205, but each 500 ms dmg for 3 sec. - Normal version.
    SPELL_BEAM_VISUAL               = 91205, // Lasts forever, damage each 500 ms. - Heroic version of 76912.
    
    // Anraphet
    SPELL_BEAMS_CAST                = 76184, // Summons 3 beams, at 3s each.

    SPELL_NEMESIS_STRIKE            = 83650,
    SPELL_NEMESIS_STRIKE_H          = 91175,

    SPELL_CRUMBLING_RUIN            = 75609,
    SPELL_CRUMBLING_RUIN_H          = 91206,

    SPELL_OMEGA_STANCE_SUMMON       = 77106,
    SPELL_OMEGA_STANCE              = 75622,
    SPELL_OMEGA_STANCE_SPIDER_TRIGGER = 77121
};

enum Creatures
{
    NPC_BEAM                        = 41144
};

enum GOs
{
    GO_VAULT_OF_LIGHTS_DOOR         = 202313,
    GO_SUN_MIRROR                   = 207726,
    GOB_ANRAPHET_DOOR               = 202314,

    GO_DOODAD_ULDUM_LIGHTMACHINE_01 = 207375,
    GO_DOODAD_ULDUM_LIGHTMACHINE_02 = 207374,
    GO_DOODAD_ULDUM_LIGHTMACHINE_03 = 207377,
    GO_DOODAD_ULDUM_LIGHTMACHINE_04 = 207376,

    GO_DOODAD_ULDUM_LASERBEAMS01    = 207662, // Matches GO_DOODAD_ULDUM_LIGHTMACHINE_02
    GO_DOODAD_ULDUM_LASERBEAMS_01   = 207663, // Matches GO_DOODAD_ULDUM_LIGHTMACHINE_01
    GO_DOODAD_ULDUM_LASERBEAMS_02   = 207664, // Matches GO_DOODAD_ULDUM_LIGHTMACHINE_04
    GO_DOODAD_ULDUM_LASERBEAMS_03   = 207665  // Matches GO_DOODAD_ULDUM_LIGHTMACHINE_03 
};

enum Points
{
    POINT_ANRAPHET_ACTIVATE         = 0,
    MAX_BRANN_WAYPOINTS_INTRO       = 17
};

enum Events
{
    EVENT_BRANN_MOVE_INTRO          = 1,
    EVENT_BRANN_UNLOCK_DOOR         = 2,
    EVENT_BRANN_THINK               = 3,
    EVENT_BRANN_SET_ORIENTATION_1   = 4,
    EVENT_BRANN_SET_ORIENTATION_2   = 5,
    EVENT_BRANN_SET_ORIENTATION_3   = 6,
    EVENT_BRANN_SAY_ELEMENTALS      = 7,
    EVENT_BRANN_SAY_GET_IT          = 8,
    EVENT_BRANN_SET_ORIENTATION_4   = 9,

    EVENT_ANRAPHET_APPEAR           = 10,
    EVENT_ANRAPHET_ACTIVATE         = 11,
    EVENT_ANRAPHET_DESTROY          = 12,
    EVENT_ANRAPHET_READY            = 13
};

enum Actions
{
    ACTION_ANRAPHET_INTRO           = 1,
    ACTION_ELEMENTAL_DIED,
    ACTION_ANRAPHET_DIED
};

Position const AnraphetActivatePos = {-193.656f, 366.689f, 75.91001f, 3.138207f}; 

Position const BrannIntroWaypoint[MAX_BRANN_WAYPOINTS_INTRO] =
{
    {-429.583f,  367.019f,  89.79282f, 0.0f},
    {-409.9531f, 367.0469f, 89.81111f, 0.0f},
    {-397.8246f, 366.967f,  86.37722f, 0.0f},
    {-383.7813f, 366.8229f, 82.07919f, 0.0f},
    {-368.2604f, 366.7448f, 77.0984f,  0.0f},
    {-353.6458f, 366.4896f, 75.92504f, 0.0f},
    {-309.0608f, 366.7205f, 75.91345f, 0.0f},
    {-276.3303f, 367.0f,    75.92413f, 0.0f},
    {-246.5104f, 366.6389f, 75.87791f, 0.0f},
    {-202.0417f, 366.7517f, 75.92508f, 0.0f},
    {-187.6024f, 366.7656f, 76.23077f, 0.0f},
    {-155.0938f, 366.783f,  86.45834f, 0.0f},
    {-143.5694f, 366.8177f, 89.73354f, 0.0f},
    {-128.5608f, 366.8629f, 89.74199f, 0.0f},
    {-103.559f,  366.5938f, 89.79725f, 0.0f},
    {-71.58507f, 367.0278f, 89.77069f, 0.0f},
    {-35.04861f, 366.6563f, 89.77447f, 0.0f},
}; 

class boss_anraphet : public CreatureScript
{
    public:
        boss_anraphet() : CreatureScript("boss_anraphet") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_anraphetAI(creature);
        }
            
        struct boss_anraphetAI : public ScriptedAI
        {
            boss_anraphetAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                instance = creature->GetInstanceScript();
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                IntroDone = false;
            }

            bool IntroDone;
            InstanceScript* instance;

            uint32 m_uiAppearTimer;
            uint32 m_uiActivateTimer;
            uint32 m_uiDestroyTimer;
            uint32 m_uiReadyTimer;

            uint32 m_uiBeamsTimer;
            uint32 m_uiFirstTimer;
            uint32 m_uiSecondTimer;
            uint32 m_uiThirdTimer;
            uint32 m_uiNemesisStrikeTimer;
            uint32 m_uiCrumblingRuinTimer;
            uint32 m_uiOmegaStanceTimer;
            SummonList summons;

            Unit* victim1;
            Unit* victim2;
            Unit* victim3;

            void Reset() OVERRIDE
            {
                if (instance)
                    instance->SetData(DATA_ANRAPHET_EVENT, NOT_STARTED);
            }

            void DoAction(int32 action) OVERRIDE
            {
                if (action == ACTION_ANRAPHET_INTRO)
                    m_uiAppearTimer = 6000;
            } 

            void MovementInform(uint32 type, uint32 point) OVERRIDE
            {
                if (type != POINT_MOTION_TYPE)
                    return;
    
                if (point == POINT_ANRAPHET_ACTIVATE)
                {
                    m_uiActivateTimer = 1500;
                    me->SetHomePosition(AnraphetActivatePos);
                }
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);

                m_uiNemesisStrikeTimer = 7000;
                m_uiBeamsTimer = 10000;
                m_uiFirstTimer = 0;
                m_uiSecondTimer = 0;
                m_uiThirdTimer = 0;
                m_uiCrumblingRuinTimer = 25000;
                m_uiOmegaStanceTimer = 30000;

                if (instance)
                {
                    instance->SetData(DATA_ANRAPHET_EVENT, IN_PROGRESS);                    
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);            
                }
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
                    instance->SetData(DATA_ANRAPHET_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); //Remove
                    instance->DoRemoveAurasDueToSpellOnPlayers(IsHeroic() ? SPELL_CRUMBLING_RUIN_H : SPELL_CRUMBLING_RUIN);
                }
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void JustDied(Unit* killer) OVERRIDE
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();

                if (Creature* brann = me->FindNearestCreature(39908, 500.0f, true))
                    brann->AI()->DoAction(ACTION_ANRAPHET_DIED);

                if (instance)
                {
                    instance->SetData(DATA_ANRAPHET_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); //Remove   
                    instance->DoRemoveAurasDueToSpellOnPlayers(IsHeroic() ? SPELL_CRUMBLING_RUIN_H : SPELL_CRUMBLING_RUIN);
                }
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                summon->setActive(true);
                summons.push_back(summon->GetGUID());
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() && IntroDone || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (m_uiAppearTimer <= diff)
                {
                    me->SetWalk(true);
                    me->GetMotionMaster()->MovePoint(POINT_ANRAPHET_ACTIVATE, AnraphetActivatePos); 
                    m_uiAppearTimer = -1;
                }
                else m_uiAppearTimer -= diff;

                if (m_uiActivateTimer <= diff)
                {
                    me->SetWalk(false);
                    Talk(SAY_INTRO); 
                    m_uiActivateTimer = -1;
                    m_uiDestroyTimer = 17500;
                }
                else m_uiActivateTimer -= diff;

                if (m_uiDestroyTimer <= diff)
                {
                    DoCast(me, SPELL_DESTRUCTION_PROTOCOL);
                    m_uiDestroyTimer = -1;
                    m_uiReadyTimer = 6000;
                }
                else m_uiDestroyTimer -= diff;

                if (m_uiReadyTimer <= diff)
                {
                    IntroDone = true;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    m_uiReadyTimer = -1;
                }
                else m_uiReadyTimer -= diff;

                if (m_uiNemesisStrikeTimer <= diff && me->isInCombat())
                {
                    DoCast(me->getVictim(), IsHeroic() ? SPELL_NEMESIS_STRIKE_H : SPELL_NEMESIS_STRIKE);
                    m_uiNemesisStrikeTimer = urand(15000, 20000);
                }
                else m_uiNemesisStrikeTimer -= diff;

                if (m_uiBeamsTimer <= diff && me->isInCombat())
                {
                    victim1 = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    victim2 = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    victim3 = SelectTarget(SELECT_TARGET_RANDOM, 0);

                    DoCast(me, SPELL_BEAMS_CAST);
                    //me->MonsterYell(SAY_ALPHA, LANG_UNIVERSAL, NULL);
                    m_uiFirstTimer = 3300;
                    m_uiSecondTimer = 0;
                    m_uiThirdTimer = 0;
                    m_uiBeamsTimer = 45000;
                }
                else m_uiBeamsTimer -= diff;

                if (m_uiFirstTimer > 0 && m_uiFirstTimer <= diff && me->isInCombat())
                {
                    Creature* Beam1 = me->SummonCreature(NPC_BEAM, victim1->GetPositionX(), victim1->GetPositionY(), victim1->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(Beam1);
                    if (IsHeroic())
                    {
                        DoCast(Beam1, SPELL_VISUAL_ON_BEAM, false);
                        Beam1->AddAura(SPELL_BEAM_VISUAL, Beam1);
                    }
                    else
                        DoCast(Beam1, SPELL_VISUAL_ON_BEAM);
                    m_uiFirstTimer = 0;
                    m_uiSecondTimer = 3300;
                    m_uiThirdTimer = 0;
                }
                else m_uiFirstTimer -= diff;

                if (m_uiSecondTimer > 0 && m_uiSecondTimer <= diff && me->isInCombat())
                {
                    Creature* Beam2 = me->SummonCreature(NPC_BEAM, victim2->GetPositionX(), victim2->GetPositionY(), victim2->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(Beam2);
                    if (IsHeroic())
                    {
                        DoCast(Beam2, SPELL_VISUAL_ON_BEAM, false);
                        Beam2->AddAura(SPELL_BEAM_VISUAL, Beam2);
                    }
                    else
                        DoCast(Beam2, SPELL_VISUAL_ON_BEAM);
                    m_uiFirstTimer = 0;
                    m_uiSecondTimer = 0;
                    m_uiThirdTimer = 3300;
                }
                else m_uiSecondTimer -= diff;

                if (m_uiThirdTimer > 0 && m_uiThirdTimer <= diff && me->isInCombat())
                {
                    Creature* Beam3 = me->SummonCreature(NPC_BEAM, victim3->GetPositionX(), victim3->GetPositionY(), victim3->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                    DoZoneInCombat(Beam3);
                    if (IsHeroic())
                    {
                        DoCast(Beam3, SPELL_VISUAL_ON_BEAM, false);
                        Beam3->AddAura(SPELL_BEAM_VISUAL, Beam3);
                    }
                    else
                        DoCast(Beam3, SPELL_VISUAL_ON_BEAM);

                    m_uiFirstTimer = 0;
                    m_uiSecondTimer = 0;
                    m_uiThirdTimer = 0;
                    m_uiNemesisStrikeTimer = 5500;
                }
                else m_uiThirdTimer -= diff;

                if (m_uiCrumblingRuinTimer <= diff && me->isInCombat())
                {
                    std::vector<Unit*> targets;
                    std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
                    for (; i != me->getThreatManager().getThreatList().end(); ++i)
                        if ((*i)->getTarget()->GetTypeId() == TYPEID_PLAYER)
                            me->AddAura(IsHeroic() ? SPELL_CRUMBLING_RUIN_H : SPELL_CRUMBLING_RUIN, (*i)->getTarget());

                    m_uiCrumblingRuinTimer = IsHeroic() ? 45000 : 50000;
                }
                else m_uiCrumblingRuinTimer -= diff;

                if (m_uiOmegaStanceTimer <= diff && me->isInCombat())
                {
                    Talk(SAY_OMEGA);
                    DoCast(me, SPELL_OMEGA_STANCE_SUMMON);
                    DoCast(me, SPELL_OMEGA_STANCE);
                    m_uiOmegaStanceTimer = 40000;
                }
                else m_uiOmegaStanceTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
};

class npc_brann_bronzebeard_anraphet : public CreatureScript
{
    public:
        npc_brann_bronzebeard_anraphet() : CreatureScript("npc_brann_bronzebeard_anraphet") { }

        struct npc_brann_bronzebeard_anraphetAI : public ScriptedAI
        {
            npc_brann_bronzebeard_anraphetAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                _currentPoint = 0;
                deadElementals = 0;
                eventDone = false;
            }

            InstanceScript* instance;
            EventMap events;
            uint32 _currentPoint;
            uint32 deadElementals;
            bool eventDone;

            void sGossipSelect(Player* player, uint32 sender, uint32 action) OVERRIDE
            {
                if (eventDone)
                    return;

                Talk(BRANN_SAY_DOOR_INTRO);
                _currentPoint = 0;
                events.Reset();
                player->PlayerTalkClass->ClearMenus();
                player->CLOSE_GOSSIP_MENU();
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetWalk(true);
                events.ScheduleEvent(EVENT_BRANN_UNLOCK_DOOR, 7500);
            }

            void DoAction(int32 action) OVERRIDE
            {
                switch (action)
                {
                    case ACTION_ELEMENTAL_DIED:
                    {
                        deadElementals++;
                        Talk(BRANN_1_ELEMENTAL_DEAD + deadElementals - 1);
                        if (deadElementals == 4)
                        {
                            eventDone = true;
                            if (GameObject* SunMirror = me->FindNearestGameObject(GO_SUN_MIRROR, 1000.0f))
                                SunMirror->SetGoState(GO_STATE_ACTIVE);
                            if (GameObject* AnraphetDoor = me->FindNearestGameObject(GOB_ANRAPHET_DOOR, 1000.0f))
                                AnraphetDoor->SetGoState(GO_STATE_ACTIVE);
                            if (Creature* anraphet = me->FindNearestCreature(BOSS_ANRAPHET, 1000.0f, true))
                                anraphet->AI()->DoAction(ACTION_ANRAPHET_INTRO);
                        }
                        break;
                    }
                    case ACTION_ANRAPHET_DIED:
                        Talk(BRANN_SAY_ANRAPHET_DIED);
                        me->NearTeleportTo(-482.134f, 174.226f, 330.662f, 2.664f);
                        break;
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BRANN_UNLOCK_DOOR:
                            Talk(BRANN_SAY_UNLOCK_DOOR);
                            if (GameObject* ADoor = me->FindNearestGameObject(202313, 100.0f))
                                ADoor->SetGoState(GO_STATE_ACTIVE);
                            events.ScheduleEvent(EVENT_BRANN_MOVE_INTRO, 3500);
                            break;
                        case EVENT_BRANN_MOVE_INTRO:
                            if (_currentPoint < MAX_BRANN_WAYPOINTS_INTRO)
                                me->GetMotionMaster()->MovePoint(_currentPoint, BrannIntroWaypoint[_currentPoint]);
                            break;
                        case EVENT_BRANN_THINK:
                            Talk(BRANN_SAY_THINK);
                            events.ScheduleEvent(EVENT_BRANN_SET_ORIENTATION_1, 6000);
                            break;
                        case EVENT_BRANN_SET_ORIENTATION_1:
                            Talk(BRANN_SAY_MIRRORS);
                            me->SetFacingTo(5.445427f);
                            events.ScheduleEvent(EVENT_BRANN_SET_ORIENTATION_2, 1000);
                            break;
                        case EVENT_BRANN_SET_ORIENTATION_2:
                            me->SetFacingTo(0.6283185f);
                            events.ScheduleEvent(EVENT_BRANN_SET_ORIENTATION_3, 2500);
                            break;
                        case EVENT_BRANN_SET_ORIENTATION_3:
                            me->SetFacingTo(0.01745329f);
                            events.ScheduleEvent(EVENT_BRANN_SAY_ELEMENTALS, 200);
                            break;
                        case EVENT_BRANN_SAY_ELEMENTALS:
                            Talk(BRANN_SAY_ELEMENTALS);
                            events.ScheduleEvent(EVENT_BRANN_SAY_GET_IT, 3500);
                            break;
                        case EVENT_BRANN_SAY_GET_IT:
                            Talk(BRANN_SAY_GET_IT);
                            // me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            break;
                        case EVENT_BRANN_SET_ORIENTATION_4:
                            me->SetFacingTo(3.141593f);
                            break;
                    }
                }
            }

            void MovementInform(uint32 movementType, uint32 pointId) OVERRIDE
            {
                if (movementType != POINT_MOTION_TYPE)
                    return;

                _currentPoint = pointId + 1;
                uint32 delay = 1;

                switch (pointId)
                {
                    case 0:
                        Talk(BRANN_SAY_TROGGS);
                        events.ScheduleEvent(EVENT_BRANN_THINK, 15000);
                        return;
                    case 1:
                        Talk(BRANN_SAY_ANRAPHET_DIED);
                        delay = 1000;
                        break;
                    case 14:
                        Talk(BRANN_SAY_MOMENT);
                        delay = 2200;
                        break;
                    case 16:
                        events.ScheduleEvent(EVENT_BRANN_SET_ORIENTATION_4, 6000);
                        return;
                    default:
                        break;
                }

                events.ScheduleEvent(EVENT_BRANN_MOVE_INTRO, delay);
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_brann_bronzebeard_anraphetAI(creature);
        }
}; 

class npc_beam : public CreatureScript
{
    public:
        npc_beam() : CreatureScript("npc_beam") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_beamAI(creature);
        }
            
        struct npc_beamAI : public ScriptedAI
        {
            npc_beamAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                creature->AddAura(93199, creature);
                if (!IsHeroic()) creature->DespawnOrUnsummon(3100);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;
            }
        };
};

class npc_omega_stance : public CreatureScript
{
    public:
        npc_omega_stance() : CreatureScript("npc_omega_stance") { }

        struct npc_omega_stanceAI : public ScriptedAI
        {
            npc_omega_stanceAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* /*who*/) OVERRIDE
            {
                me->SetReactState(REACT_PASSIVE);
                DoCast(me, SPELL_OMEGA_STANCE_SPIDER_TRIGGER, true);
            }

            void EnterEvadeMode() OVERRIDE { }
            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_omega_stanceAI(creature);
        }
};

class spell_anraphet_omega_stance_summon : public SpellScriptLoader
{
public:
    spell_anraphet_omega_stance_summon() : SpellScriptLoader("spell_anraphet_omega_stance_summon") { }

    class spell_anraphet_omega_stance_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_anraphet_omega_stance_summon_SpellScript);

        void ModDestHeight(SpellEffIndex /*effIndex*/)
        {
            Position offset = {0.0f, 0.0f, 30.0f, 0.0f};
            const_cast<WorldLocation*>(GetExplTargetDest())->RelocateOffset(offset);
            GetHitDest()->RelocateOffset(offset);
        }

        void Register() OVERRIDE
        {
            OnEffectLaunch += SpellEffectFn(spell_anraphet_omega_stance_summon_SpellScript::ModDestHeight, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_anraphet_omega_stance_summon_SpellScript();
    }
};

class spell_omega_stance_spider_effect : public SpellScriptLoader
{
public:
    spell_omega_stance_spider_effect() : SpellScriptLoader("spell_omega_stance_spider_effect") { }

    class spell_omega_stance_spider_effect_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_omega_stance_spider_effect_SpellScript);

        void SetDestPosition(SpellEffIndex effIndex)
        {
            // Do our own calculations for the destination position.
            /// TODO: Remove this once we find a general rule for WorldObject::MovePosition (this spell shouldn't take the Z change into consideration)
            Unit* caster = GetCaster();
            float angle = float(rand_norm()) * static_cast<float>(2 * M_PI);
            uint32 dist = caster->GetObjectSize() + GetSpellInfo()->Effects[effIndex].CalcRadius(GetCaster()) * (float)rand_norm();

            float x = caster->GetPositionX() + dist * std::cos(angle);
            float y = caster->GetPositionY() + dist * std::sin(angle);
            float z = caster->GetPositionZ();

            const_cast<WorldLocation*>(GetExplTargetDest())->Relocate(x, y, z);
            GetHitDest()->Relocate(x, y, z);
        }

        void Register() OVERRIDE
        {
            OnEffectLaunch += SpellEffectFn(spell_omega_stance_spider_effect_SpellScript::SetDestPosition, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_omega_stance_spider_effect_SpellScript();
    }
};

void AddSC_boss_anraphet()
{
    new boss_anraphet();
    new npc_brann_bronzebeard_anraphet();
    new npc_beam();
    new npc_omega_stance();
    new spell_anraphet_omega_stance_summon();
    new spell_omega_stance_spider_effect();
}
