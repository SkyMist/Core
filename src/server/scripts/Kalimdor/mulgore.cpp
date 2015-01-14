/*
* Copyright (C) 2013 Skymist Project
*
* This file is NOT free software. You may NOT copy, redistribute it or modify it.
*/

/* ScriptData
SDName: Mulgore
SD%Complete: 100
SDComment: Support for quest: 11129, 861
SDCategory: Mulgore
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "World.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Player.h"
#include "SpellScript.h"
#include "Group.h"

/*######
# npc_skorn_whitecloud
######*/

#define GOSSIP_SW "Tell me a story, Skorn."

class npc_skorn_whitecloud : public CreatureScript
{
public:
    npc_skorn_whitecloud() : CreatureScript("npc_skorn_whitecloud") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF)
            player->SEND_GOSSIP_MENU(523, creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->GetQuestRewardStatus(770))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        player->SEND_GOSSIP_MENU(522, creature->GetGUID());

        return true;
    }

};

/*#####
# npc_kyle_frenzied
######*/

enum KyleFrenzied
{
    //emote signed for 7780 but propably thats wrong id.
    EMOTE_SEE_LUNCH         = 0,
    EMOTE_EAT_LUNCH         = 1,
    EMOTE_DANCE             = 2,

    SPELL_LUNCH             = 42222,
    NPC_KYLE_FRENZIED       = 23616,
    NPC_KYLE_FRIENDLY       = 23622,
    POINT_ID                = 1
};

class npc_kyle_frenzied : public CreatureScript
{
public:
    npc_kyle_frenzied() : CreatureScript("npc_kyle_frenzied") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_kyle_frenziedAI (creature);
    }

    struct npc_kyle_frenziedAI : public ScriptedAI
    {
        npc_kyle_frenziedAI(Creature* creature) : ScriptedAI(creature) {}

        bool EventActive;
        bool IsMovingToLunch;
        uint64 PlayerGUID;
        uint32 EventTimer;
        uint8 EventPhase;

        void Reset()
        {
            EventActive = false;
            IsMovingToLunch = false;
            PlayerGUID = 0;
            EventTimer = 5000;
            EventPhase = 0;

            if (me->GetEntry() == NPC_KYLE_FRIENDLY)
                me->UpdateEntry(NPC_KYLE_FRENZIED);
        }

        void SpellHit(Unit* Caster, SpellInfo const* Spell)
        {
            if (!me->getVictim() && !EventActive && Spell->Id == SPELL_LUNCH)
            {
                if (Caster->GetTypeId() == TYPEID_PLAYER)
                    PlayerGUID = Caster->GetGUID();

                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                {
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->MoveIdle();
                    me->StopMoving();
                }

                EventActive = true;
                Talk(EMOTE_SEE_LUNCH);
                me->HandleEmote(EMOTE_ONESHOT_CREATURE_SPECIAL);
            }
        }

        void MovementInform(uint32 Type, uint32 PointId)
        {
            if (Type != POINT_MOTION_TYPE || !EventActive)
                return;

            if (PointId == POINT_ID)
                IsMovingToLunch = false;
        }

        void UpdateAI(uint32 const diff)
        {
            if (EventActive)
            {
                if (IsMovingToLunch)
                    return;

                if (EventTimer <= diff)
                {
                    EventTimer = 5000;
                    ++EventPhase;

                    switch (EventPhase)
                    {
                        case 1:
                            if (Unit* unit = Unit::GetUnit(*me, PlayerGUID))
                            {
                                if (GameObject* go = unit->GetGameObject(SPELL_LUNCH))
                                {
                                    IsMovingToLunch = true;
                                    me->GetMotionMaster()->MovePoint(POINT_ID, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
                                }
                            }
                            break;
                        case 2:
                            Talk(EMOTE_EAT_LUNCH);
                            me->HandleEmote(EMOTE_STATE_USE_STANDING);
                            break;
                        case 3:
                            if (Player* unit = Unit::GetPlayer(*me, PlayerGUID))
                                unit->TalkedToCreature(me->GetEntry(), me->GetGUID());
                            me->UpdateEntry(NPC_KYLE_FRIENDLY);
                            break;
                        case 4:
                            EventTimer = 30000;
                            Talk(EMOTE_DANCE);
                            me->HandleEmote(EMOTE_STATE_DANCESPECIAL);
                            break;
                        case 5:
                            me->HandleEmote(EMOTE_STATE_NONE);
                            Reset();
                            me->GetMotionMaster()->Clear();
                            break;
                    }
                }
                else
                    EventTimer -= diff;
            }
        }
    };

};

enum TribeImprisoned
{
    QUEST_TRIBE_IMPRISONED = 24852,
    GO_QUILBOAR_CAGE       = 202112,
    NPC_BRAVE_CAPTIVE      = 38345,

    EVENT_DESPAWN          = 1,
    POINT_INIT             = 1
};

/*######
# npc_brave_captive
######*/

class npc_brave_captive : public CreatureScript
{
    public:
        npc_brave_captive() : CreatureScript("npc_brave_captive") { }

        struct npc_brave_captiveAI : public ScriptedAI
        {
            npc_brave_captiveAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                if (GameObject* cage = me->FindNearestGameObject(GO_QUILBOAR_CAGE, 5.0f))
                {
                    cage->SetLootState(GO_JUST_DEACTIVATED);
                    cage->SetGoState(GO_STATE_READY);
                }

                _events.Reset();
                _player = NULL;
                _movementComplete = false;
            }

            void StartMoving(Player* owner)
            {
                if (owner)
                {
                    switch(urand(0, 2))
                    {
                        case 0: me->MonsterSay("The quilboar will pay!", LANG_UNIVERSAL, 0); break;
                        case 1: me->MonsterSay("Thank the Earth Mother!", LANG_UNIVERSAL, 0); break;
                        case 2: me->MonsterSay("I can move again!", LANG_UNIVERSAL, 0); break;
                        default: break;
                    }

                    _player = owner;
                }

                Position pos;
                me->GetNearPosition(pos, 3.0f, 0.0f);
                me->GetMotionMaster()->MovePoint(POINT_INIT, pos);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE || id != POINT_INIT)
                    return;

                if (_player)
                    _player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());

                _movementComplete = true;
                _events.ScheduleEvent(EVENT_DESPAWN, 3500);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!_movementComplete)
                    return;

                _events.Update(diff);

                if (_events.ExecuteEvent() == EVENT_DESPAWN)
                    me->DespawnOrUnsummon();
            }

        private:
            Player* _player;
            EventMap _events;
            bool _movementComplete;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_brave_captiveAI(creature);
        }
};

/*######
# go_quilboar_cage
######*/

class go_quilboar_cage : public GameObjectScript
{
    public:
        go_quilboar_cage() : GameObjectScript("go_quilboar_cage") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            if (player->GetQuestStatus(QUEST_TRIBE_IMPRISONED) == QUEST_STATUS_INCOMPLETE)
            {
                if (Creature* captive = go->FindNearestCreature(NPC_BRAVE_CAPTIVE, 5.0f, true))
                {
                    go->ResetDoorOrButton();
                    CAST_AI(npc_brave_captive::npc_brave_captiveAI, captive->AI())->StartMoving(player);
                    return false;
                }
            }
            return true;
        }
};

/*######
## npc_wounded_brave
######*/

class npc_wounded_brave : public CreatureScript
{
public:
    npc_wounded_brave() : CreatureScript("npc_wounded_brave") { }

    struct npc_wounded_braveAI : public ScriptedAI
    {
        npc_wounded_braveAI(Creature* creature) : ScriptedAI(creature) { }

        bool IsHealed;

        void Reset()
        {
            IsHealed = false;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if ((spell->Id == 2061 || spell->Id == 774) && !IsHealed)
            {
                switch(urand(0, 2))
                {
                    case 0: me->MonsterSay("For the Earthmother!", LANG_UNIVERSAL, 0); break;
                    case 1: me->MonsterSay("Blessings to you!", LANG_UNIVERSAL, 0); break;
                    case 2: me->MonsterSay("Ahh, I feel better already. Thank you!", LANG_UNIVERSAL, 0); break;
                    default: break;
                }

                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->HandleEmote(EMOTE_ONESHOT_BOW);
                me->DespawnOrUnsummon(3000);
                IsHealed = true;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wounded_braveAI (creature);
    }
};

/*######
## npc_agitated_earth_spirit
######*/

enum AgitatedEarth
{
    EVENT_ROCK_BARRAGE = 1,
    SPELL_ROCK_BARRAGE = 81305
};

class npc_agitated_earth_spirit : public CreatureScript
{
public:
    npc_agitated_earth_spirit() : CreatureScript("npc_agitated_earth_spirit") { }

    struct npc_agitated_earth_spiritAI : public ScriptedAI
    {
        npc_agitated_earth_spiritAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        bool IsCalmed;

        void Reset()
        {
            events.Reset();
            IsCalmed = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_ROCK_BARRAGE, urand(2000, 5000));
        }

        void EnterEvadeMode()
        {
            me->RemoveAllAuras();
            events.Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if (spell->Id == 69453 && !IsCalmed) // Drum of the Soothed Earth
            {
                switch(urand(0, 1))
                {
                    case 0:
                        me->MonsterTextEmote("The spirit is pleased and calms itself!", caster->GetGUID(), true);
                        if (caster->ToPlayer())
                            caster->ToPlayer()->KilledMonsterCredit(36872, 0); // Quest credit.
                        me->setFaction(35);
                        if (me->isInCombat()) EnterEvadeMode();
                        me->DespawnOrUnsummon(5000);
                        break;
                    case 1:
                        me->MonsterTextEmote("The spirit is displeased and attacks!", caster->GetGUID(), true);
                        me->setFaction(14);
                        me->AI()->AttackStart(caster);
                        break;
                    default: break;
                }

                IsCalmed = true;
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                IsCalmed = false; // Reset hit if kills player.
        }

        void UpdateAI(uint32 const diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (events.ExecuteEvent() == EVENT_ROCK_BARRAGE)
            {
                DoCast(me->getVictim(), SPELL_ROCK_BARRAGE);
                events.ScheduleEvent(EVENT_ROCK_BARRAGE, urand(6000, 9000));
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_agitated_earth_spiritAI (creature);
    }
};

void AddSC_mulgore()
{
    new npc_skorn_whitecloud();
    new npc_kyle_frenzied();
    new npc_brave_captive();
    new go_quilboar_cage();
    new npc_wounded_brave();
    new npc_agitated_earth_spirit();
}