/*
*
* SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass.
*
* Raid: Throne of Thunder.
* Boss: Tortos.
*
* Wowpedia boss history:
*
* "Over the millennia, small amounts of mogu flesh-shaping magic seeped into the caverns below the Thunder King's citadel. 
*  The dark energies warped one of the chamber's native dragon turtles, melding it to the surrounding crystalline walls. 
*  Known as Tortos, this amalgamation of flesh and stone has since feasted on the cave's rich mineral deposits and grown to a colossal size."
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
#include "VehicleDefines.h"
#include "SpellInfo.h"

#include "throne_of_thunder.h"

enum Yells
{
    // Boss
    ANN_TURTLES                 = 0,      // Tortos lets out a booming call, attracting nearby turtles.
	ANN_FURIOUS_BREATH                    // Tortos prepares to unleash a [Furious Stone Breath]!
};

enum Spells
{
    // Boss
    SPELL_ZERO_POWER            = 72242,  // No Regen

    SPELL_KICK_SHELL_A          = 134030, // Boss aura for adding mechanic abilities to players in 130y radius.

    SPELL_CALL_OF_TORTOS        = 136294, // Dummy on eff 0 for summoning 3 turtles.
    SPELL_FURIOUS_STONE_BREATH  = 133939, // Triggers damage each 500 ms + prevents Fury regen for duration.
    SPELL_GROWING_FURY          = 136010, // When no players are in melee range. Adds 10 Fury.
    SPELL_SNAPPING_BITE         = 135251, // On tank, main ability.

    SPELL_QUAKE_STOMP           = 134920, // Massive AOE damage. Interruptible. Triggers SPELL_ROCKFALL_STOMP.
    SPELL_ROCKFALL_STOMP        = 134915, // 8 second aura triggering SPELL_ROCKFALL_STOMP_S_TRIG each 500 ms.
    SPELL_ROCKFALL_STOMP_S_TRIG = 140431, // Dummy on eff 0 for SPELL_ROCKFALL_SUMMON on random player.

    SPELL_ROCKFALL_AURA         = 134849, // Triggers SPELL_ROCKFALL_AURA_S_TRIG each 10 seconds. Permanent boss aura in fight.
    SPELL_ROCKFALL_AURA_S_TRIG  = 134364, // Dummy on eff 0 for SPELL_ROCKFALL_SUMMON on random player.
    SPELL_ROCKFALL_SUMMON       = 134365, // Summons NPC_ROCKFALL_TORTOS.

    SPELL_SUMMON_BATS           = 136686, // Summons 8 Vampiric Cave Bats.

    SPELL_BERSERK               = 144369, // Berserk, Enrage, Bye - Bye or, simply put, a wipe. :)

    // Adds

    // Whirl Turtle
    SPELL_SPINNING_SHELL_VISUAL = 133974, // Spin + aura visual.
    SPELL_SPINNING_SHELL_DUMMY  = 140443, // Speed decrease + periodic dummy on effect 1 for SPELL_SPINNING_SHELL_DMG.
    SPELL_SPINNING_SHELL_DMG    = 134011, // Damage and knockback.

    SPELL_SHELL_BLOCK           = 133971, // Damage immune and kickable state aura.

    SPELL_KICK_SHELL_TRIGGER    = 134031, // Spell from mechanic abilities button. Sends turtles forward fast. Needs turtle aura SPELL_SHELL_BLOCK to become usable.
    SPELL_KICK_SHELL_STUN       = 134073, // Unused.

    SPELL_SHELL_CONCUSSION      = 134092, // When kicked, aura triggering SPELL_SHELL_CONCUSSION_INT and SPELL_SHELL_CONCUSSION_D_IN each 300 ms.
    SPELL_SHELL_CONCUSSION_INT  = 134091, // Spell casting interruption for 3 seconds in 8y.
    SPELL_SHELL_CONCUSSION_D_IN = 136431, // Damage taken increase by 50% in 8y.

    // Vampiric Cave Bat
    SPELL_DRAIN_THE_WEAK_A      = 135103, // Triggers SPELL_DRAIN_THE_WEAK_DMG if target is below 35% health and drains 50x damage dealt.
    SPELL_DRAIN_THE_WEAK_DMG    = 135101, // 25% weapon damage.

    // Rockfall
    SPELL_ROCKFALL              = 134475, // Visual on ground and triggers 134539 missile drop + damage after 5 seconds.

    // Humming Crystal - HEROIC only.
    SPELL_CRYSTAL_SHELL_AURA    = 137552, // Adds SPELL_CRYSTAL_SHELL_ABS, SPELL_CRYSTAL_SHELL_MOD_ABS to player attackers.
    SPELL_CRYSTAL_SHELL_ABS     = 137633, // Eff 0 absorb, eff 1 dummy for absorbing max 15% of player's hp.
    SPELL_CRYSTAL_SHELL_MOD_ABS = 137648, // Eff 0 mod absorb %, eff 1 dummy for adding player aura SPELL_CRYSTAL_SHELL_CAPPED on cap when absorbing max 75% player hp.
    SPELL_CRYSTAL_SHELL_CAPPED  = 140701  // "Maximum capacity" dummy aura from Crystal Shield (at 5 stacks).
};

enum Npcs
{
    // Boss
    NPC_WHIRL_TURTLE            = 67966,
    NPC_VAMPIRIC_CAVE_BAT       = 69352,
    NPC_ROCKFALL_TORTOS         = 68219,

    // Misc
    NPC_HUMMING_CRYSTAL         = 69639  // HEROIC only. When attacked adds Crystal Shell debuff to player atacker.
};

enum Events
{
    // Boss
    EVENT_CALL_OF_TORTOS        = 1,
    EVENT_FURIOUS_STONE_BREATH,
    EVENT_RESET_CAST,
    EVENT_SNAPPING_BITE,
    EVENT_QUAKE_STOMP,
    EVENT_SUMMON_BATS,

    EVENT_GROWING_FURY,
    EVENT_REGEN_FURY_POWER,

    EVENT_BERSERK,

    // Whirl Turtle
    EVENT_SHELL_BLOCK,
    EVENT_KICKED
};

enum Timers
{
    // Boss
    TIMER_CALL_OF_TORTOS_F      = 21000,
    TIMER_CALL_OF_TORTOS_S      = 60500,

    TIMER_QUAKE_STOMP_F         = 27000,
    TIMER_QUAKE_STOMP_S         = 47000,

    TIMER_FURIOUS_STONE_BREATH  = 500,
    TIMER_RESET_CAST            = 6500,

    TIMER_SNAPPING_BITE_N       = 12000,
    TIMER_SNAPPING_BITE_H       = 8000,

    TIMER_CALL_BATS_F           = 57000,
    TIMER_CALL_BATS_S           = 50000,

    TIMER_GROWING_FURY          = 6000,
    TIMER_REGEN_FURY_POWER      = 1000,

    TIMER_BERSERK_H             = 600000, // 10 minutes (Heroic).
    TIMER_BERSERK               = 780000  // 13 minutes.
};

class boss_tortos : public CreatureScript
{
    public:
        boss_tortos() : CreatureScript("boss_tortos") { }

        struct boss_tortosAI : public BossAI
        {
            boss_tortosAI(Creature* creature) : BossAI(creature, DATA_TORTOS_EVENT), summons(me), vehicle(creature->GetVehicleKit())
            {
                instance  = creature->GetInstanceScript();
                ASSERT(vehicle);
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            SummonList summons;
            EventMap events;
            bool breathScheduled;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                breathScheduled = false;

                if (instance)
                    instance->SetData(DATA_TORTOS_EVENT, NOT_STARTED);

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                me->AddAura(SPELL_KICK_SHELL_A, me);
                me->AddAura(SPELL_ROCKFALL_AURA, me);

				events.ScheduleEvent(EVENT_CALL_OF_TORTOS, TIMER_CALL_OF_TORTOS_F);
				events.ScheduleEvent(EVENT_FURIOUS_STONE_BREATH, TIMER_FURIOUS_STONE_BREATH);
				events.ScheduleEvent(EVENT_SNAPPING_BITE, IsHeroic() ? TIMER_SNAPPING_BITE_H : TIMER_SNAPPING_BITE_N);
				events.ScheduleEvent(EVENT_QUAKE_STOMP, TIMER_QUAKE_STOMP_F);
				events.ScheduleEvent(EVENT_SUMMON_BATS, TIMER_CALL_BATS_F);

                events.ScheduleEvent(EVENT_GROWING_FURY, TIMER_GROWING_FURY);
                events.ScheduleEvent(EVENT_REGEN_FURY_POWER, TIMER_REGEN_FURY_POWER);
				events.ScheduleEvent(EVENT_BERSERK, TIMER_BERSERK);

                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    instance->SetData(DATA_TORTOS_EVENT, IN_PROGRESS);
                }

                _EnterCombat();
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

                if (instance)
                {
                    instance->SetData(DATA_TORTOS_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);

                _JustReachedHome();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void JustDied(Unit* killer)
            {
                summons.DespawnAll();

                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    instance->SetData(DATA_TORTOS_EVENT, DONE);
                }

                _JustDied();
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || !CheckInRoom() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 100 && !breathScheduled)
                {
				    events.ScheduleEvent(EVENT_FURIOUS_STONE_BREATH, TIMER_FURIOUS_STONE_BREATH);
				    events.ScheduleEvent(EVENT_RESET_CAST, TIMER_RESET_CAST);
					breathScheduled = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CALL_OF_TORTOS:
                            Talk(ANN_TURTLES);
				            DoCast(me, SPELL_CALL_OF_TORTOS);
				            events.ScheduleEvent(EVENT_CALL_OF_TORTOS, TIMER_CALL_OF_TORTOS_S);
                            break;

                        case EVENT_FURIOUS_STONE_BREATH:
                            Talk(ANN_FURIOUS_BREATH);
				            DoCast(me, SPELL_FURIOUS_STONE_BREATH);
                            break;

                        case EVENT_RESET_CAST:
				            breathScheduled = false;
                            break;

                        case EVENT_SNAPPING_BITE:
				            DoCast(me->getVictim(), SPELL_SNAPPING_BITE);
				            events.ScheduleEvent(EVENT_SNAPPING_BITE, IsHeroic() ? TIMER_SNAPPING_BITE_H : TIMER_SNAPPING_BITE_N);
                            break;

                        case EVENT_QUAKE_STOMP:
				            DoCast(me, SPELL_QUAKE_STOMP);
				            events.ScheduleEvent(EVENT_QUAKE_STOMP, TIMER_QUAKE_STOMP_S);
                            break;

                        case EVENT_SUMMON_BATS:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 100.0f, true))
				                me->CastSpell(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ() + 12.0f, SPELL_SUMMON_BATS, true);
				            events.ScheduleEvent(EVENT_SUMMON_BATS, TIMER_CALL_BATS_S);
                            break;

                        case EVENT_GROWING_FURY:
                            if (!me->IsWithinDistInMap(me->getVictim(), me->GetAttackDistance(me->getVictim())))
				                DoCast(me, SPELL_GROWING_FURY);
                            events.ScheduleEvent(EVENT_GROWING_FURY, TIMER_GROWING_FURY);
                            break;

                        case EVENT_REGEN_FURY_POWER:
                            me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 2);
                            events.ScheduleEvent(EVENT_REGEN_FURY_POWER, TIMER_REGEN_FURY_POWER);
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_tortosAI(creature);
        }
};

// Whirl Turtle 67966
class npc_whirl_turtle : public CreatureScript
{
    public:
        npc_whirl_turtle() : CreatureScript("npc_whirl_turtle") { }

        struct npc_whirl_turtleAI : public ScriptedAI
        {
            npc_whirl_turtleAI(Creature* creature) : ScriptedAI(creature) { }

            bool shellBlocked;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->MoveRandom(50.0f);
            }

            void Reset()
            {
                shellBlocked = false;
                // me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_SPINNING_SHELL_DMG, true);
                me->AddAura(SPELL_SPINNING_SHELL_VISUAL, me);
                me->AddAura(SPELL_SPINNING_SHELL_DUMMY, me);
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_KICK_SHELL_TRIGGER)
                {
                    me->AddAura(SPELL_SHELL_CONCUSSION, me);

                    float x, y, z;
                    caster->GetClosePoint(x, y, z, caster->GetObjectSize() / 3, 50.0f);
                    me->GetMotionMaster()->MoveCharge(x, y, z, 20.0f);
                    me->DespawnOrUnsummon(6000);
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (me->HealthBelowPct(26) && !shellBlocked)
                {
                    me->RemoveAurasDueToSpell(SPELL_SPINNING_SHELL_VISUAL);
                    me->RemoveAurasDueToSpell(SPELL_SPINNING_SHELL_DUMMY);

                    me->AddAura(SPELL_SHELL_BLOCK, me);
                    shellBlocked = true;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_whirl_turtleAI(creature);
        }
};

// Vampiric Cave Bat 69352
class npc_vampiric_cave_bat : public CreatureScript
{
    public:
        npc_vampiric_cave_bat() : CreatureScript("npc_vampiric_cave_bat") { }

        struct npc_vampiric_cave_batAI : public ScriptedAI
        {
            npc_vampiric_cave_batAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                DoZoneInCombat(me, 100.0f);
            }

            void Reset()
            {
                me->AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY);
                me->AddAura(SPELL_DRAIN_THE_WEAK_A, me);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_vampiric_cave_batAI(creature);
        }
};

// Rockfall 68219
class npc_rockfall_tortos : public CreatureScript
{
    public:
        npc_rockfall_tortos() : CreatureScript("npc_rockfall_tortos") { }

        struct npc_rockfall_tortosAI : public ScriptedAI
        {
            npc_rockfall_tortosAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->AddAura(SPELL_ROCKFALL, me);
                me->DespawnOrUnsummon(7000);
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rockfall_tortosAI(creature);
        }
};

// Humming Crystal 69639 - HEROIC only
class npc_humming_crystal : public CreatureScript
{
    public:
        npc_humming_crystal() : CreatureScript("npc_humming_crystal") { }

        struct npc_humming_crystalAI : public ScriptedAI
        {
            npc_humming_crystalAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->AddAura(SPELL_CRYSTAL_SHELL_AURA, me);
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_humming_crystalAI(creature);
        }
};

// Call of Tortos 136294
class spell_call_of_tortos : public SpellScriptLoader
{
    public:
        spell_call_of_tortos() : SpellScriptLoader("spell_call_of_tortos") { }

        class spell_call_of_tortos_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_call_of_tortos_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                for (uint8 i = 0; i < 3; i++)
                    caster->SummonCreature(NPC_WHIRL_TURTLE, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_call_of_tortos_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_call_of_tortos_SpellScript();
        }
};

// Rockfall 140431, 134364
class spell_rockfall_trigger_tortos : public SpellScriptLoader
{
    public:
        spell_rockfall_trigger_tortos() : SpellScriptLoader("spell_rockfall_trigger_tortos") { }

        class spell_rockfall_trigger_tortos_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rockfall_trigger_tortos_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (Unit* target = caster->ToCreature()->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f, true))
                    caster->SummonCreature(NPC_ROCKFALL_TORTOS, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rockfall_trigger_tortos_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rockfall_trigger_tortos_SpellScript();
        }
};

// Spinning Shell 140443
class spell_spinning_shell : public SpellScriptLoader
{
    public:
        spell_spinning_shell() : SpellScriptLoader("spell_spinning_shell") { }

        class spell_spinning_shell_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_spinning_shell_AuraScript)

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                Map::PlayerList const &PlayerList = caster->GetMap()->GetPlayers();
                if (!PlayerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        if (Player* player = i->getSource())
                            if (player->IsWithinDistInMap(caster, 5.0f))
                                if (Unit* plr = player->ToUnit())
                                    plr->CastSpell(plr, SPELL_SPINNING_SHELL_DMG, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_spinning_shell_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_spinning_shell_AuraScript();
        }
};

// Drain the Weak 135103
class spell_drain_the_weak : public SpellScriptLoader
{
    public:
        spell_drain_the_weak() : SpellScriptLoader("spell_drain_the_weak") { }

        class spell_drain_the_weak_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_drain_the_weak_AuraScript);

            void HandleProc(constAuraEffectPtr aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                if (target->GetHealth() < 350000)
                    caster->CastSpell(target, SPELL_DRAIN_THE_WEAK_DMG, true);
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_drain_the_weak_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_drain_the_weak_AuraScript();
        }
};

// Crystal Shell 137633
class spell_crystal_shell_aura : public SpellScriptLoader
{
    public:
        spell_crystal_shell_aura() : SpellScriptLoader("spell_crystal_shell_aura") { }

        class spell_crystal_shell_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_crystal_shell_aura_AuraScript);

            uint32 totalAbsorbAmount;

            bool Load()
            {
                totalAbsorbAmount = 0;
                return true;
            }

            void OnAbsorb(AuraEffectPtr aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                if (Unit* target = dmgInfo.GetVictim())
                {
                    if (Unit* attacker = dmgInfo.GetAttacker())
                    {
                        if (AuraPtr crystal = target->GetAura(SPELL_CRYSTAL_SHELL_AURA))
                        {
                            if (totalAbsorbAmount <= target->CountPctFromMaxHealth(15 * crystal->GetStackAmount()))
                            {
                                absorbAmount = dmgInfo.GetDamage();
                                totalAbsorbAmount += dmgInfo.GetDamage();
                            }
                            else
                            {
                                target->RemoveAurasDueToSpell(SPELL_CRYSTAL_SHELL_AURA);
                                absorbAmount = 0;
                            }
                        }
                        else absorbAmount = 0;
                    }
                }
            }

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    if (AuraPtr crystal = target->GetAura(SPELL_CRYSTAL_SHELL_AURA))
                        if (crystal->GetStackAmount() == 5)
                            target->AddAura(SPELL_CRYSTAL_SHELL_CAPPED, target);
                }
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    if (target->HasAura(SPELL_CRYSTAL_SHELL_CAPPED))
                        target->RemoveAurasDueToSpell(SPELL_CRYSTAL_SHELL_CAPPED);
                }
            }

            void Register()
            {
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_crystal_shell_aura_AuraScript::OnAbsorb, EFFECT_0);
                OnEffectApply += AuraEffectApplyFn(spell_crystal_shell_aura_AuraScript::OnApply, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_crystal_shell_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_crystal_shell_aura_AuraScript();
        }
};

// Quake Stomp 134920
class spell_tortos_quake_stomp : public SpellScriptLoader
{
    public:
        spell_tortos_quake_stomp() : SpellScriptLoader("spell_tortos_quake_stomp") { }

        class spell_tortos_quake_stomp_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_tortos_quake_stomp_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                SetHitDamage(target->CountPctFromMaxHealth(sSpellMgr->GetSpellInfo(SPELL_QUAKE_STOMP, caster->GetMap()->GetDifficulty())->Effects[0].BasePoints));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_tortos_quake_stomp_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_tortos_quake_stomp_SpellScript();
        }
};

/*** INTRO ***/

enum LeiShenIntroYells
{
    SAY_LEI_SHEN_INTRO_1 = 0, // You have swept the filth from my doorstep. Perhaps, you are worthy of my attention. - sound 35587
    SAY_LEI_SHEN_INTRO_2,     // But...your trespass ends here. None may enter my forbidden stronghold. I shall rebuild this bridge with your bones for bricks." - sound 35588
};

enum LeiShenEvents
{
    EVENT_LEI_SHEN_SEND_CINEMATIC = 1,
    EVENT_LEI_SHEN_I_1,
    EVENT_LEI_SHEN_I_2,
    EVENT_LEI_SHEN_VISUAL_CAST,
    EVENT_LEI_SHEN_DESTROY_BRIDGE,
    EVENT_LEI_SHEN_TELEPORT_PLAYERS
};

enum LeiShenIntroSpells
{
    SPELL_PLATFORM_DUMMY = 82827,
    SPELL_BRIDGE_VIS_S   = 139853, // Boss visual
    SPELL_TELEPORT_DEPTH = 139852  // Tortos tele spell
};

enum LeiShenIntroCreatures
{
    NPC_BRIDGE_TRIGGER   = 66305,
    NPC_TORTOS_TRIGGER   = 55091
};

enum LeiShenIntroGo
{
    GO_TORTOS_BRIDGE     = 218869
};

// Lei Shen - tortos intro 70437
class npc_lei_shen_tortos : public CreatureScript
{
    public:
        npc_lei_shen_tortos() : CreatureScript("npc_lei_shen_tortos") { }

        struct npc_lei_shen_tortosAI : public ScriptedAI
        {
            npc_lei_shen_tortosAI(Creature* creature) : ScriptedAI(creature) 
            {
                introDone = false;
			}

            EventMap events;
            bool introDone;

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!introDone && me->IsWithinDistInMap(who, 140.0f, true) && who->GetTypeId() == TYPEID_PLAYER)
                {
                    me->SetReactState(REACT_PASSIVE);
                    events.ScheduleEvent(EVENT_LEI_SHEN_SEND_CINEMATIC, 100);
                    introDone = true;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LEI_SHEN_SEND_CINEMATIC:
                        {
                            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    if (Player* player = i->getSource())
                                        player->SendCinematicStart(271);
                            events.ScheduleEvent(EVENT_LEI_SHEN_I_1, 5900); // at 6 seconds.
                            break;
                        }

                        case EVENT_LEI_SHEN_I_1:
                            Talk(SAY_LEI_SHEN_INTRO_1);
                            me->HandleEmote(EMOTE_ONESHOT_TALK);
                            events.ScheduleEvent(EVENT_LEI_SHEN_I_2, 8000); // at 14 seconds.
                            break;

                        case EVENT_LEI_SHEN_I_2:
                            Talk(SAY_LEI_SHEN_INTRO_2);
                            me->HandleEmote(EMOTE_ONESHOT_EXCLAMATION);
                            events.ScheduleEvent(EVENT_LEI_SHEN_VISUAL_CAST, 6000); // at 20 seconds.
                            break;

                        case EVENT_LEI_SHEN_VISUAL_CAST:
                            DoCast(me, SPELL_BRIDGE_VIS_S);
                            events.ScheduleEvent(EVENT_LEI_SHEN_DESTROY_BRIDGE, 5000); // at 25 seconds.
                            break;

                        case EVENT_LEI_SHEN_DESTROY_BRIDGE:
                        {
                            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    if (Player* player = i->getSource())
                                        player->AddAura(132228, player); // 1 min parachute.

                            if (GameObject* bridge = me->FindNearestGameObject(GO_TORTOS_BRIDGE, 300.0f))
                                bridge->SetDestructibleState(GO_DESTRUCTIBLE_DESTROYED);
                            events.ScheduleEvent(EVENT_LEI_SHEN_TELEPORT_PLAYERS, 16000); // at 41 seconds.
                            break;
                        }

                        case EVENT_LEI_SHEN_TELEPORT_PLAYERS:
                            if (Creature* trigger = me->SummonCreature(NPC_TORTOS_TRIGGER, 6041.180f, 5100.50f, -42.059f, 4.752f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                trigger->CastSpell(trigger, SPELL_TELEPORT_DEPTH, false);
                                trigger->DespawnOrUnsummon(1000);
                            }
                            me->DespawnOrUnsummon(1500);
                            break;

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lei_shen_tortosAI(creature);
        }
};

// Call Lightning 139853
class spell_lei_shen_tortos_bridge_call_lightning : public SpellScriptLoader
{
    public:
        spell_lei_shen_tortos_bridge_call_lightning() : SpellScriptLoader("spell_lei_shen_tortos_bridge_call_lightning") { }

        class spell_lei_shen_tortos_bridge_call_lightning_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_lei_shen_tortos_bridge_call_lightning_AuraScript)

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                std::list<Creature*> trigger;
                GetCreatureListWithEntryInGrid(trigger, caster, NPC_BRIDGE_TRIGGER, 300.0f);
                if (!trigger.empty())
                    for (std::list<Creature*>::iterator trig = trigger.begin(); trig != trigger.end(); trig++)
                        (*trig)->CastSpell(*trig, SPELL_PLATFORM_DUMMY, false);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_lei_shen_tortos_bridge_call_lightning_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_lei_shen_tortos_bridge_call_lightning_AuraScript();
        }
};

void AddSC_boss_tortos()
{
    new boss_tortos();
    new npc_whirl_turtle();
    new npc_vampiric_cave_bat();
    new npc_rockfall_tortos();
    new npc_humming_crystal();
    new spell_call_of_tortos();
    new spell_rockfall_trigger_tortos();
    new spell_spinning_shell();
    new spell_drain_the_weak();
    new spell_crystal_shell_aura();
    new spell_tortos_quake_stomp();
    new npc_lei_shen_tortos();
    new spell_lei_shen_tortos_bridge_call_lightning();
}
