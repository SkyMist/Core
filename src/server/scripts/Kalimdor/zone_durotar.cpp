/*
* Copyright (C) 2013 Skymist Project
*
* This file is NOT free software. You may NOT copy, redistribute it or modify it.
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"

/*######
## Quest 25134: Lazy Peons
## npc_lazy_peon
######*/

enum LazyPeonYells
{
    SAY_SPELL_HIT = 0 // Ow! OK, I''ll get back to work, $N!'
};

enum LazyPeon
{
    QUEST_LAZY_PEONS    = 25134,
    GO_LUMBERPILE       = 175784,
    SPELL_BUFF_SLEEP    = 17743,
    SPELL_AWAKEN_PEON   = 19938
};

class npc_lazy_peon : public CreatureScript
{
public:
    npc_lazy_peon() : CreatureScript("npc_lazy_peon") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_lazy_peonAI(creature);
    }

    struct npc_lazy_peonAI : public ScriptedAI
    {
        npc_lazy_peonAI(Creature* creature) : ScriptedAI(creature) { }

        bool work, hit;

        void Reset() OVERRIDE
        {
            work = false;
            hit = false;
            if (!me->HasAura(SPELL_BUFF_SLEEP)) DoCast(me, SPELL_BUFF_SLEEP);
        }

        void MovementInform(uint32 /*type*/, uint32 id) OVERRIDE
        {
            if (id == 1)
                work = true;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if (!hit && spell->Id == SPELL_AWAKEN_PEON && caster->GetTypeId() == TYPEID_PLAYER && caster->ToPlayer()->GetQuestStatus(QUEST_LAZY_PEONS) == QUEST_STATUS_INCOMPLETE)
            {
                Talk(SAY_SPELL_HIT, caster->GetGUID());
                me->RemoveAllAuras();

                caster->ToPlayer()->KilledMonsterCredit(me->GetEntry(), me->GetGUID());

                if (GameObject* Lumberpile = me->FindNearestGameObject(GO_LUMBERPILE, 20))
                    me->GetMotionMaster()->MovePoint(1, Lumberpile->GetPositionX()-1, Lumberpile->GetPositionY(), Lumberpile->GetPositionZ());

                hit = true;
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (work)
            {
                me->HandleEmote(EMOTE_ONESHOT_WORK_CHOPWOOD);
                me->DespawnOrUnsummon(15000);
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

enum VoodooSpells
{
    SPELL_BREW      = 16712, // Special Brew
    SPELL_GHOSTLY   = 16713, // Ghostly
    SPELL_HEX1      = 16707, // Hex
    SPELL_HEX2      = 16708, // Hex
    SPELL_HEX3      = 16709, // Hex
    SPELL_GROW      = 16711, // Grow
    SPELL_LAUNCH    = 16716, // Launch (Whee!)
};

// 17009
class spell_voodoo : public SpellScriptLoader
{
    public:
        spell_voodoo() : SpellScriptLoader("spell_voodoo") { }

        class spell_voodoo_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_voodoo_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BREW) || !sSpellMgr->GetSpellInfo(SPELL_GHOSTLY) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HEX1) || !sSpellMgr->GetSpellInfo(SPELL_HEX2) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HEX3) || !sSpellMgr->GetSpellInfo(SPELL_GROW) ||
                    !sSpellMgr->GetSpellInfo(SPELL_LAUNCH))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/) OVERRIDE
            {
                uint32 spellid = RAND(SPELL_BREW, SPELL_GHOSTLY, RAND(SPELL_HEX1, SPELL_HEX2, SPELL_HEX3), SPELL_GROW, SPELL_LAUNCH);
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, spellid, false);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_voodoo_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_voodoo_SpellScript();
        }
};

enum BbladeCultist
{
    SPELL_INCINER  = 79938,
    SPELL_SUMMIMP  = 11939,
    SPELL_IMMOL    = 11962,

    SPELL_FELBLOOD = 80174,
    SPELL_INFUSED  = 84325,

    SAY_FLEE       = 0 // 15%
};

class npc_bblade_cultist : public CreatureScript
{
public:
    npc_bblade_cultist() : CreatureScript("npc_bblade_cultist") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_bblade_cultistAI(creature);
    }

    struct npc_bblade_cultistAI : public ScriptedAI
    {
        npc_bblade_cultistAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 IncinerateTimer;
        uint32 ImmolateTimer;
        bool Flee;

        void Reset() OVERRIDE
        {
            IncinerateTimer = urand(1000, 4000);
            ImmolateTimer = urand (5500, 9500);
            DoCast(me, SPELL_SUMMIMP);
            Flee = false;
        }

        void EnterEvadeMode() OVERRIDE
        {
            me->RemoveAllAuras();
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void JustDied (Unit* killer) OVERRIDE
        {
            if (killer->HasAura(SPELL_FELBLOOD))
            {
                if (Aura* aura = killer->GetAura(SPELL_FELBLOOD))
                {
                    if (aura->GetStackAmount() >= 5)
                    {
                        killer->RemoveAurasDueToSpell(SPELL_FELBLOOD);
                        me->AddAura(SPELL_INFUSED, killer);
                    }
                    else me->AddAura(SPELL_FELBLOOD, killer);
                }
            }
            else me->AddAura(SPELL_FELBLOOD, killer);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (!Flee && HealthBelowPct(16))
            {
                Talk(SAY_FLEE);
                me->DoFleeToGetAssistance();
                IncinerateTimer = 10000;
                Flee = true;
            }

            if (IncinerateTimer <= diff)
            {
                DoCast(me->GetVictim(), SPELL_INCINER);
                IncinerateTimer = urand(3000, 6000);
            }
            else
                IncinerateTimer -= diff;

            if (ImmolateTimer <= diff)
            {
                DoCast(me->GetVictim(), SPELL_IMMOL);
                ImmolateTimer = urand(17000, 21000);
            }
            else
                ImmolateTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

enum Watershed // quest 25187 item 52514 spell 73817 http://www.youtube.com/watch?v=J501FKs1CgE
{
    // Vehicles, on boarding give credit and eject back to place.
    WATERSHED_RAGARRAN      = 39320,
    WATERSHED_TEKLA         = 39345,
    WATERSHED_MISHA         = 39346,
    WATERSHED_ZENTAJI       = 39347,
};

void AddSC_durotar()
{
    new npc_lazy_peon();
    new spell_voodoo();
    new npc_bblade_cultist();
}