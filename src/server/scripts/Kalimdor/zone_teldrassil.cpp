/*
 * Copyright (C) 2013 Skymist Project.
 *
 * This program is not free software. You may not redistribute it or modify it.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedFollowerAI.h"

/*####
# npc_mist
####*/

enum Mist
{
    SAY_AT_HOME             = 0,
    EMOTE_AT_HOME           = 1,

    QUEST_MIST              = 938,
    NPC_ARYNIA              = 3519,

    FACTION_DARNASSUS       = 79
};

class npc_mist : public CreatureScript
{
public:
    npc_mist() : CreatureScript("npc_mist") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) OVERRIDE
    {
        if (quest->GetQuestId() == QUEST_MIST)
            if (npc_mistAI* pMistAI = CAST_AI(npc_mist::npc_mistAI, creature->AI()))
                pMistAI->StartFollow(player, FACTION_DARNASSUS, quest);

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_mistAI(creature);
    }

    struct npc_mistAI : public FollowerAI
    {
        npc_mistAI(Creature* creature) : FollowerAI(creature) { }

        void Reset() OVERRIDE { }

        void MoveInLineOfSight(Unit* who) OVERRIDE
        {
            FollowerAI::MoveInLineOfSight(who);

            if (!me->GetVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && who->GetEntry() == NPC_ARYNIA)
            {
                if (me->IsWithinDistInMap(who, 10.0f) && who->ToCreature())
                {
                    who->ToCreature()->AI()->Talk(SAY_AT_HOME);
                    DoComplete();
                }
            }
        }

        void DoComplete() OVERRIDE
        {
            Talk(EMOTE_AT_HOME);

            Player* player = GetLeaderForFollower();
            if (player && player->GetQuestStatus(QUEST_MIST) == QUEST_STATUS_INCOMPLETE)
                player->GroupEventHappens(QUEST_MIST, me);

            //The follow is over (and for later development, run off to the woods before really end)
            SetFollowComplete();
        }

        void UpdateFollowerAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

};

class npc_wounded_sentinel : public CreatureScript
{
public:
    npc_wounded_sentinel() : CreatureScript("npc_wounded_sentinel") { }

    struct npc_wounded_sentinelAI : public ScriptedAI
    {
        npc_wounded_sentinelAI(Creature* creature) : ScriptedAI(creature) { }

        bool IsHealed;

        void Reset() OVERRIDE
        {
            IsHealed = false;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if ((spell->Id == 774 || spell->Id == 2061) && !IsHealed)
            {
                Talk(urand(0,2));
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                me->DespawnOrUnsummon(4000);
                IsHealed = true;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_wounded_sentinelAI (creature);
    }
};

void AddSC_teldrassil()
{
    new npc_mist();
    new npc_wounded_sentinel();
}
