/*
    Dungeon : Stormstout Brewery 85-87
    Boss: Ook-Ook
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Vehicle.h"

#include "stormstout_brewery.h"

enum Yells
{
    SAY_INTRO                    = 0, // Who crashing Ook party!? Ook ook ook...
    SAY_AGGRO                    = 1, // Me gonna ook you in the dooker!
    SAY_KILL                     = 2, // In the dooker!
    SAY_BANANAS                  = 3, // 0 - Get Ooking party started! ; 1 - Come on and get your Ook on! ; 2 - We're gonna Ook all night!
    SAY_DEATH                    = 4  // Ook! Oooook!!
};

#define ANN_BANANAS "Ook-Ook is Going Bananas! More barrels are coming!"

enum Spells
{
    // Boss
    SPELL_GROUND_POUND           = 106807, // Aura.
    SPELL_GOING_BANANAS          = 106651, // Aura.
    SPELL_GOING_BANANAS_DUMMY    = 115978,

    // NPCs
    SPELL_BARREL_COSMETIC        = 106647, // Visual spell, triggers 106672 dummy each 300 ms.

    SPELL_BARREL_EXPLOSION       = 107016, // Explosion on players. Triggered by 115868 periodic aura.
    SPELL_BARREL_EXPLOSION_O     = 106784, // Explosion on Ook-Ook. Triggered by 115875, triggered by 115907 periodic aura.

    SPELL_BARREL_EXPLOSION_M_O   = 106769, // Explosion on monkeys (eff 0) && Ook-Ook (eff 1). Triggered by 106768 periodic aura.
    SPELL_BARREL_RIDE            = 106614, // Vehicle ride spell, triggers 106768, triggering 106769.
    SPELL_FORCECAST_BARREL_DROP  = 122385, // Triggers SPELL_BARREL_DROP.
    SPELL_BARREL_DROP            = 122376  // Jump spell for Barrel NPC.
};

enum Events
{
    // Boss
    EVENT_GROUND_POUND           = 1,
    EVENT_GOING_BANANAS,

    // NPCs
    EVENT_EXPLODE
};

enum GoingBananasStates
{
    DONE_NONE              = 0, // No casts done.
    DONE_90                = 1, // First cast done.
    DONE_60                = 2, // Second cast done.
    DONE_30                = 3  // All casts done.
};

class boss_ook_ook : public CreatureScript
{
    public:
        boss_ook_ook() : CreatureScript("boss_ook_ook") { }

        struct boss_ook_ook_AI : public BossAI
        {
            boss_ook_ook_AI(Creature* creature) : BossAI(creature, DATA_OOKOOK_EVENT), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            SummonList summons;
            uint8 goingBananasDone;
            bool summonedBarrels; // For Going Bananas phases.

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                Talk(SAY_INTRO);
                Reset();
            }

            void Reset() OVERRIDE
            {
                events.Reset();
                summons.DespawnAll();
                summonedBarrels = false;

                if (instance)
                    instance->SetData(DATA_OOKOOK_EVENT, NOT_STARTED);

                goingBananasDone = DONE_NONE;

                _Reset();
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }

                events.ScheduleEvent(EVENT_GROUND_POUND, 10000);

                _EnterCombat();
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void EnterEvadeMode() OVERRIDE
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _EnterEvadeMode();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _JustDied();
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                summons.Summon(summon);
                summon->setActive(true);

		        if (me->IsInCombat())
                    summon->SetInCombatWithZone();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // Set Going Bananas phases execution.
                if (me->HealthBelowPct(91) && goingBananasDone == DONE_NONE || me->HealthBelowPct(61) && goingBananasDone == DONE_90 || me->HealthBelowPct(31) && goingBananasDone == DONE_60)
                {
                    events.ScheduleEvent(EVENT_GOING_BANANAS, 1000);
                    goingBananasDone++;
                }

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_GROUND_POUND:
                            DoCast(me, SPELL_GROUND_POUND);
                            events.ScheduleEvent(EVENT_GROUND_POUND, urand(7000, 10000));
                            break;

                        case EVENT_GOING_BANANAS:
                            me->MonsterTextEmote(ANN_BANANAS, NULL, true);
                            Talk(SAY_BANANAS);
                            summonedBarrels = false;
                            DoCast(me, SPELL_GOING_BANANAS);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_ook_ook_AI(creature);
        }
};

// Brew Barrel 56682.
class npc_barrel : public CreatureScript
{
    public:
        npc_barrel() : CreatureScript("npc_barrel") { }

        struct npc_barrel_AI : public ScriptedAI
        {
            npc_barrel_AI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset() OVERRIDE
            {
                events.Reset();
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_BARREL_COSMETIC, me);
                me->SetSpeed(MOVE_WALK, 0.7f);
                me->SetSpeed(MOVE_RUN, 0.7f);

                float x, y, z;
                me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 40.0f);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
            }

            void MovementInform(uint32 type, uint32 id) OVERRIDE
            {
                if (!me->IsAlive() || type != POINT_MOTION_TYPE || id != 1)
                    return;

                events.ScheduleEvent(EVENT_EXPLODE, 500);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (CheckIfAgainstWall() || CheckIfAgainstPlayer())
                    DoExplode(true);
                else if (CheckIfAgainstOokOok())
                    DoExplode(false);

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_EXPLODE:
                            DoExplode(true);
                            break;

                        default: break;
                    }
                }
            }

        // Particular functions here.
        private:
            bool CheckIfAgainstWall()
            {
                float x, y, z;
                me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 5.0f);
                if (!me->IsWithinLOS(x, y, me->GetPositionZ()))
                    return true;

                return false;
            }

            bool CheckIfAgainstOokOok()
            {
                if (me->FindNearestCreature(BOSS_OOKOOK, 1.0f, true))
                    return true;

                return false;
            }

            bool CheckIfAgainstPlayer()
            {
                if (Player* nearPlayer = me->FindNearestPlayer(1.0f))
                    if (nearPlayer->IsWithinDistInMap(me, 1.0f))
                    return true;

                return false;
            }

            void DoExplode(bool onPlayer = true)
            {
                if (onPlayer) // On players.
                    DoCast(me, SPELL_BARREL_EXPLOSION);
                else // On Ook-Ook.
                {
                    if (Vehicle* barrel = me->GetVehicleKit())
                        barrel->RemoveAllPassengers();

                    DoCast(me, SPELL_BARREL_EXPLOSION_O);
                }

                me->DespawnOrUnsummon(200);
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_barrel_AI(creature);
        }
};

// Going Bananas 115978.
class spell_ook_ook_going_bananas_summon : public SpellScriptLoader
{
public :
    spell_ook_ook_going_bananas_summon() : SpellScriptLoader("spell_ook_ook_going_bananas_summon") { }

    class spell_ook_ook_going_bananas_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ook_ook_going_bananas_summon_SpellScript)

        bool Validate(const SpellInfo* spellInfo)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_GOING_BANANAS_DUMMY))
                return false;

            return true;
        }

        void HandleDummy(SpellEffIndex effIndex) // This comes periodic, so only do it once per phase.
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (CAST_AI(boss_ook_ook::boss_ook_ook_AI, GetCaster()->ToCreature()->AI())->summonedBarrels == false)
                {
                    for (uint8 i = 0; i < 6; i++)
                        if (Creature* OokOok = GetCaster()->ToCreature())
                            OokOok->SummonCreature(NPC_OOK_BARREL, OokOok->GetPositionX() + frand(-10.0f, 10.0f), OokOok->GetPositionY() + frand(-10.0f, 10.0f), OokOok->GetPositionZ() + 1.0f, frand(0.0f, 6.0f), TEMPSUMMON_MANUAL_DESPAWN);
                    CAST_AI(boss_ook_ook::boss_ook_ook_AI, GetCaster()->ToCreature()->AI())->summonedBarrels = true;
                }
            }
        }

        void Register() OVERRIDE
        {
            OnEffectLaunch += SpellEffectFn(spell_ook_ook_going_bananas_summon_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_ook_ook_going_bananas_summon_SpellScript();
    }
};

class PositionCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit PositionCheck(Unit* _caster) : caster(_caster) { }
        bool operator()(WorldObject* object)
        {
            return !caster->HasInArc(M_PI / 6, object);
        }

    private:
        Unit* caster;
};

// Ground Pound triggered spell 106808.
class spell_ook_ook_ground_pound_dmg : public SpellScriptLoader
{
public:
    spell_ook_ook_ground_pound_dmg() : SpellScriptLoader("spell_ook_ook_ground_pound_dmg") { }

    class spell_ook_ook_ground_pound_dmgSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ook_ook_ground_pound_dmgSpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(PositionCheck(GetCaster()));
        }

        void Register() OVERRIDE
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ook_ook_ground_pound_dmgSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ook_ook_ground_pound_dmgSpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_CONE_ENEMY_104);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_ook_ook_ground_pound_dmgSpellScript();
    }
};

// Barrel ride 106614.
class spell_ook_ook_barrel_ride : public SpellScriptLoader
{
    public:
        spell_ook_ook_barrel_ride() :  SpellScriptLoader("spell_ook_ook_barrel_ride") { }

        class spell_ook_ook_barrel_ride_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ook_ook_barrel_ride_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget())
                {
                    if (Unit* barrelBase = GetTarget())
                    {
                        barrelBase->GetMotionMaster()->MovementExpired();
                        barrelBase->GetMotionMaster()->Clear();
                        // barrelBase->GetMotionMaster()->MoveIdle();
                    }
                }
            }

            void Register() OVERRIDE
            {
                OnEffectApply += AuraEffectApplyFn(spell_ook_ook_barrel_ride_AuraScript::OnApply, EFFECT_0, SPELL_AURA_CONTROL_VEHICLE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_ook_ook_barrel_ride_AuraScript();
        }
};

void AddSC_boss_ook_ook()
{
	new boss_ook_ook();
    new npc_barrel();
    new spell_ook_ook_going_bananas_summon();
    new spell_ook_ook_barrel_ride();
    new spell_ook_ook_ground_pound_dmg();
}
