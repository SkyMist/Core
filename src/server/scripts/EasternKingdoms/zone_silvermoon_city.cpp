/*
* Copyright (C) 2013 Skymist Project
*
* This file is NOT free software. You may NOT copy, redistribute it or modify it.
*/


/* ScriptData
SDName: Silvermoon_City
SD%Complete: 100
SDComment: Quest support: 9685
SDCategory: Silvermoon City
EndScriptData */

/* ContentData
npc_blood_knight_stillblade
EndContentData */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Creature.h"

/*#######
# npc_blood_knight_stillblade
#######*/

enum eStillbladeData
{
    SAY_HEAL                    = 0,

    QUEST_REDEEMING_THE_DEAD    = 9685,
    SPELL_SHIMMERING_VESSEL     = 31225,
    SPELL_REVIVE_SELF           = 32343,
};

class npc_blood_knight_stillblade : public CreatureScript
{
public:
    npc_blood_knight_stillblade() : CreatureScript("npc_blood_knight_stillblade") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_blood_knight_stillbladeAI (creature);
    }

    struct npc_blood_knight_stillbladeAI : public ScriptedAI
    {
        npc_blood_knight_stillbladeAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 lifeTimer;
        bool spellHit;

        void Reset() OVERRIDE
        {
            lifeTimer = 120000;
            me->SetStandState(UNIT_STAND_STATE_DEAD);
            spellHit = false;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE { }

        void MoveInLineOfSight(Unit* /*who*/) OVERRIDE { }

        void UpdateAI (uint32 diff) OVERRIDE
        {
            if (me->IsStandState())
            {
                if (lifeTimer <= diff)
                    me->AI()->EnterEvadeMode();
                else
                    lifeTimer -= diff;
            }
        }

        void SpellHit(Unit* Hitter, const SpellInfo* Spellkind) OVERRIDE
        {
            if ((Spellkind->Id == SPELL_SHIMMERING_VESSEL) && !spellHit &&
				(Hitter->GetTypeId() == TYPEID_PLAYER) && Hitter->ToPlayer()->IsActiveQuest(QUEST_REDEEMING_THE_DEAD))
            {
                Hitter->ToPlayer()->AreaExploredOrEventHappens(QUEST_REDEEMING_THE_DEAD);
                DoCast(me, SPELL_REVIVE_SELF);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, 0);
                //me->RemoveAllAuras();
                Talk(SAY_HEAL);
                spellHit = true;
            }
        }
    };
};

/*######
## npc_wounded_outrunner
######*/

class npc_wounded_outrunner : public CreatureScript
{
public:
    npc_wounded_outrunner() : CreatureScript("npc_wounded_outrunner") { }

    struct npc_wounded_outrunnerAI : public ScriptedAI
    {
        npc_wounded_outrunnerAI(Creature* creature) : ScriptedAI(creature) { }

        bool IsHealed;

        void Reset() OVERRIDE
        {
            IsHealed = false;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if (spell->Id == 2061 && !IsHealed)
            {
                switch(urand(0, 2))
                {
                    case 0: me->MonsterSay("Blessings to you!", LANG_UNIVERSAL, 0); break;
                    case 1: me->MonsterSay("I'm regaining my strenght! Thank you, stranger. For the Sin'dorei!", LANG_UNIVERSAL, 0); break;
                    case 2: me->MonsterSay("Ahh, I feel better already. Thank you.", LANG_UNIVERSAL, 0); break;
                    default: break;
                }

                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                me->DespawnOrUnsummon(3000);
                IsHealed = true;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_wounded_outrunnerAI (creature);
    }
};

void AddSC_silvermoon_city()
{
    new npc_blood_knight_stillblade();
    new npc_wounded_outrunner();
}
