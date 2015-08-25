/*
 * Copyright (C) 2011-2015 SkyMist Gaming
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * World Boss: Sha of Anger.
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"

enum Yells
{
    SAY_SPAWN                    = 0,      // 0 - Give in to your anger. ; 1 - Your rage gives you strength! ; 2 - Your rage sustains me! ; 3 - You will not bury me again! ; 4 - My wrath flows freely!
    SAY_AGGRO,                             // Yes, YES! Bring your rage to bear! Try to strike me down!
    SAY_KILL,                              // 0 - Extinguished! ; 1 - Does that make you angry? ; 2 - Feel your rage! ; 3 - Let your rage consume you.
    SAY_GROWING_ANGER,                     // Feed me with your ANGER!
    SAY_UNLEASHED_WRATH                    // My fury is UNLEASHED!
};

enum Sounds
{
    SOUND_DEATH                  = 29000   // Laughter on boss death.
};

enum Spells
{
    // Boss
    SPELL_SEETHE                 = 119487, // Dmg  + increase damage taken.
    SPELL_ENDLESS_RAGE           = 119586, // Cast time. Triggers 119592 SE.
    SPELL_ENDLESS_RAGE_SE        = 119592, // Triggered by above. SE to cast 119587.
    SPELL_ENDLESS_RAGE_MISS      = 119587, // Missile. Triggers 119446.
    SPELL_ENDLESS_RAGE_DMG       = 119446, // Triggered by above. Damage + summon NPC's Ire and Bitter Thoughts.
    SPELL_GROWING_ANGER          = 119622, // Cast time, per. dummy aura for Aggressive behaviour.
    SPELL_AGGRESSIVE_BEHAVIOUR   = 119626, // MC, dmg + health increase, heal, periodic dummy for health pct. check.
    SPELL_UNLEASHED_WRATH        = 119488, // Triggers 119489.
    SPELL_UNLEASHED_WRATH_DMG    = 119489, // Triggered by above.
    SPELL_ENERGY_DRAIN           = 117707, // No Energy Regen.
    SPELL_BERSERK                = 47008,

    // NPC's
    SPELL_OVERCOME_BY_ANGER      = 129356, // When standing in zones in Kun Lai.
    SPELL_BITTER_THOUGHTS        = 119601, // Aura, triggers 119610.
    SPELL_BITTER_THOUGHTS_PACIFY = 119610  // Triggered by above. Pacify and Silence.
};

enum Events
{
    // Boss
    EVENT_SEETHE                 = 1,
    EVENT_ENDLESS_RAGE,
    EVENT_GROWING_ANGER,
    EVENT_UNLEASHED_WRATH,
    EVENT_BERSERK
};

enum Creatures
{
    NPC_OVERCOME_BY_ANGER_BUNNY  = 60732,
    NPC_IRE                      = 60579,
    NPC_BITTER_THOUGHTS          = 61523
};

class boss_sha_of_anger : public CreatureScript
{
    public:
        boss_sha_of_anger() : CreatureScript("boss_sha_of_anger") { }

        struct boss_sha_of_anger_AI : public ScriptedAI
        {
            boss_sha_of_anger_AI(Creature* creature) : ScriptedAI(creature), vehicle(creature->GetVehicleKit()), summons(me)
            {
                ASSERT(vehicle);                // Red energy bar. Vehicle Id 2189.
            }

            EventMap events;
            SummonList summons;
            Vehicle* vehicle;
            bool unleashWrathPhase;
            uint32 energyTimer;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                Talk(SAY_SPAWN);

                events.Reset();
                summons.DespawnAll();

                unleashWrathPhase = false;
                energyTimer = -1;

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 0);

                DoCast(me, SPELL_ENERGY_DRAIN);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                energyTimer = 1000;
                events.ScheduleEvent(EVENT_SEETHE, 5000);         // Melee range check.
                events.ScheduleEvent(EVENT_ENDLESS_RAGE, 20000);  // 20s into the fight, 45s after that.
                events.ScheduleEvent(EVENT_GROWING_ANGER, 30000); // 30s into the fight, 45s after that.
                events.ScheduleEvent(EVENT_BERSERK, 900000);      // Berserk, 15 mins.
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void EnterEvadeMode()
            {
                RemoveAggressiveBehaviour();
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void JustDied(Unit* /*killer*/)
            {
                // Send death laughter sound.
                sCreatureTextMgr->SendSound(me, SOUND_DEATH, CHAT_MSG_MONSTER_YELL, 0, TEXT_RANGE_NORMAL, TEAM_OTHER, false);

                summons.DespawnAll();
                RemoveAggressiveBehaviour();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

		        if (me->isInCombat())
                    summon->SetInCombatWithZone();

                // Bitter Thoughts clouds.
                if (summon->GetEntry() == NPC_BITTER_THOUGHTS)
                {
                    summon->SetReactState(REACT_PASSIVE);
                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    summon->CastSpell(summon, SPELL_BITTER_THOUGHTS, true);
                    summon->DespawnOrUnsummon(60000); // 1 min duration.
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                // Handle energy regen and events.
                if (!unleashWrathPhase && me->GetPower(POWER_ENERGY) == 100)
                {
                    events.ScheduleEvent(EVENT_UNLEASHED_WRATH, 1000);
                    unleashWrathPhase = true;
                    energyTimer = 5000; // 1s until event + 3s cast time + 1s for first drain.
                }

                if (unleashWrathPhase && me->GetPower(POWER_ENERGY) == 0)
                {
                    unleashWrathPhase = false;
                    energyTimer = 1000;
			    }

                if (energyTimer <= diff)
                {
                    me->SetPower(POWER_ENERGY, !unleashWrathPhase ? (me->GetPower(POWER_ENERGY) + 2) : (me->GetPower(POWER_ENERGY) - 4));
                    energyTimer = 1000;
                }
                else energyTimer -= diff;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Melee range check.
                        case EVENT_SEETHE:
                            if (!HasHostileInMeleeRange())
                                DoCast(me->getVictim(), SPELL_SEETHE);
                            events.ScheduleEvent(EVENT_SEETHE, 2500);
                            break;

                        case EVENT_ENDLESS_RAGE:
                            DoCast(me, SPELL_ENDLESS_RAGE);
                            events.ScheduleEvent(EVENT_ENDLESS_RAGE, 45000);
                            break;

                        case EVENT_GROWING_ANGER:
                            Talk(SAY_GROWING_ANGER);
                            DoCast(me, SPELL_GROWING_ANGER);
                            events.ScheduleEvent(EVENT_GROWING_ANGER, 45000);
                            break;

                        case EVENT_UNLEASHED_WRATH:
                            Talk(SAY_UNLEASHED_WRATH);
                            DoCast(me, SPELL_UNLEASHED_WRATH);
                            break;

                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        // Particular AI functions.
        private:

            // Check for pets or players in Melee range.
            bool HasHostileInMeleeRange()
            {
                // Check for tank.
                if (Unit* tank = me->getVictim())
                    if (tank->IsWithinDistInMap(me, MELEE_RANGE))
                        return true;

                // Check for pets.
                if (Unit* unit = me->SelectNearbyTarget(NULL, MELEE_RANGE))
                    if (unit->isPet())
                        return true;

                // Check for players.
                if (Player* nearPlayer = me->FindNearestPlayer(MELEE_RANGE))
                    if (nearPlayer->IsWithinDistInMap(me, MELEE_RANGE))
                        if (!nearPlayer->isGameMaster())
                            return true;
            
                return false;
            }

            // Remove boss auras from players.
            void RemoveAggressiveBehaviour()
            {
                std::list<Player*> playerList;
                GetPlayerListInGrid(playerList, me, 100.0f);

                for (auto itr : playerList)
                    itr->RemoveAurasDueToSpell(SPELL_AGGRESSIVE_BEHAVIOUR);
			}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sha_of_anger_AI(creature);
        }
};

// Anger Sha Effect Bunny 60732.
class npc_overcome_by_anger_bunny : public CreatureScript
{
    public:
        npc_overcome_by_anger_bunny() : CreatureScript("npc_overcome_by_anger_bunny") { }

        struct npc_overcome_by_anger_bunnyAI : public ScriptedAI
        {
            npc_overcome_by_anger_bunnyAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                SetCanSeeEvenInPassiveMode(true);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (me->GetMapId() == 870 && who->GetTypeId() == TYPEID_PLAYER && who->IsWithinDist(me, 30.0f))
                {
                    if (who->IsWithinDist(me, 20.0f))
                        who->AddAura(SPELL_OVERCOME_BY_ANGER, who);
                    else if (!who->IsWithinDist(me, 20.0f) && who->HasAura(SPELL_OVERCOME_BY_ANGER))
                        who->RemoveAurasDueToSpell(SPELL_OVERCOME_BY_ANGER);
                }
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_overcome_by_anger_bunnyAI(creature);
        }
};

// Endless Rage triggered spell 119592.
class spell_sha_of_anger_endless_rage : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_endless_rage() : SpellScriptLoader("spell_sha_of_anger_endless_rage") { }

        class spell_sha_of_anger_endless_rage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_of_anger_endless_rage_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                if (!GetCaster())
                    return;

                GetCaster()->CastSpell(GetHitUnit(), GetSpellInfo()->Effects[EFFECT_0].BasePoints, true); // SPELL_ENDLESS_RAGE_MISS.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_of_anger_endless_rage_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_of_anger_endless_rage_SpellScript();
        }
};

// Growing Anger 119622.
class spell_sha_of_anger_growing_anger : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_growing_anger() : SpellScriptLoader("spell_sha_of_anger_growing_anger") { }

        class spell_sha_of_anger_growing_anger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_of_anger_growing_anger_SpellScript)

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (!GetCaster() || targets.empty())
                    return;

                // Maximum 3 targets.
                if (targets.size() > 3)
                    SkyMistCore::RandomResizeList(targets, 3);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_of_anger_growing_anger_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_of_anger_growing_anger_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_of_anger_growing_anger_SpellScript();
        }

        class spell_sha_of_anger_growing_anger_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_anger_growing_anger_AuraScript)

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (!caster || !target)
                    return;

                GetCaster()->CastSpell(target, SPELL_AGGRESSIVE_BEHAVIOUR, true);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_anger_growing_anger_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_anger_growing_anger_AuraScript();
        }
};

// Aggressive Behaviour 119626.
class spell_sha_of_anger_aggressive_behaviour : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_aggressive_behaviour() : SpellScriptLoader("spell_sha_of_anger_aggressive_behaviour") { }

        class spell_sha_of_anger_aggressive_behaviour_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_anger_aggressive_behaviour_AuraScript);

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();
                if (Unit* target = GetTarget())
                    if (target->GetHealthPct() < 50.0f)
                        this->Remove(AURA_REMOVE_BY_DEFAULT);
            }

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    if (!target->ToPlayer())
                        return;

                    target->SetPvP(true);
                    target->setFaction(16);
                    target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                }
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    target->SetPvP(false); // This should only be removed for FFA PvP realms.
                    target->RestoreFaction();
                    target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::HandlePeriodicTick, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectApply += AuraEffectApplyFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::OnApply, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::OnRemove, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_anger_aggressive_behaviour_AuraScript();
        }
};

// Unleashed Wrath 119489.
class spell_sha_of_anger_unleashed_wrath : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_unleashed_wrath() : SpellScriptLoader("spell_sha_of_anger_unleashed_wrath") { }

        class spell_sha_of_anger_unleashed_wrath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_of_anger_unleashed_wrath_SpellScript)

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (!GetCaster() || targets.empty())
                    return;

                // Maximum 10 targets.
                if (targets.size() > 10)
                    SkyMistCore::RandomResizeList(targets, 10);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_of_anger_unleashed_wrath_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_of_anger_unleashed_wrath_SpellScript();
        }
};

// Overcome by Anger - 129356.
class spell_sha_of_anger_overcome_by_anger : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_overcome_by_anger() : SpellScriptLoader("spell_sha_of_anger_overcome_by_anger") { }

        class spell_sha_of_anger_overcome_by_anger_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_anger_overcome_by_anger_AuraScript);

            void OnUpdate(const uint32 diff)
            {
                if (Unit* target = GetUnitOwner())
                {
                    // Teleport handling. Aura is removed if not in Kun-Lai Summit, Pandaria.
                    if (target->GetMapId() != 870 || target->GetZoneId() != 5841)
                        target->RemoveAurasDueToSpell(SPELL_OVERCOME_BY_ANGER);
                }
            }

            void Register()
            {
                OnAuraUpdate += AuraUpdateFn(spell_sha_of_anger_overcome_by_anger_AuraScript::OnUpdate);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_anger_overcome_by_anger_AuraScript();
        }
};

void AddSC_boss_sha_of_anger()
{
    new boss_sha_of_anger();
    new npc_overcome_by_anger_bunny();
    new spell_sha_of_anger_endless_rage();
    new spell_sha_of_anger_growing_anger();
    new spell_sha_of_anger_aggressive_behaviour();
    new spell_sha_of_anger_unleashed_wrath();
    new spell_sha_of_anger_overcome_by_anger();
}
