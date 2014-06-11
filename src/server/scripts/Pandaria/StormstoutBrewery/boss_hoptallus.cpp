/*
    Dungeon : Stormstout Brewery 85-87
    Boss: Hoptallus
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"

#include "stormstout_brewery.h"

enum Yells
{
    SAY_AGGRO          = 0, // Oh yeah!
    SAY_KILL           = 1, // You die!
    SAY_DEATH          = 2, // You have...turnip...for a head...
    SAY_FURLWIND       = 3, // Gonna spins around!
    SAY_CARROT_BREATH  = 4  // Urp...eats too many carrots...
};

#define ANN_BUNNIES "Hoptallus lets out a loud screech! The virmen are coming!"

enum Spells
{
    // Boss
    SPELL_CARROT_BREATH          = 112944, // Aura. Triggers 112945 dmg.
    SPELL_FURLWIND               = 112992, // Aura. Triggers 112993 dmg.

    // NPCs
    SPELL_KEG_CARRY              = 131820, // Hopper carry keg.
    SPELL_EXPLOSIVE_BREW         = 114291, // When Hopper reaches target.

    SPELL_HAMMER_VISUAL          = 114530,
    SPELL_HAMMER_ARROW           = 114533,
    SPELL_SMASH_AURA             = 111662, // Hammer aura.
    SPELL_SMASH                  = 111666  // Player cast spell on button click.
};

enum Events
{
    // Boss
    EVENT_CARROT_BREATH          = 1,
    EVENT_FURLWIND,
    EVENT_SUMMON_NPCS,
    EVENT_SET_COMBAT,

    // NPCs
    EVENT_EXPLOSIVE_BREW,
    EVENT_CHECK_PLAYERS
};

Position const bunnySummonPosition     = { -713.955f, 1254.574f, 164.790f }; // Bunny adds summon position.
Position const hoptallusMovePosition   = { -692.524f, 1247.556f, 162.793f }; // Hoptallus move position after summoned.

class boss_hoptallus : public CreatureScript
{
    public :
        boss_hoptallus() : CreatureScript("boss_hoptallus") { }

        struct boss_hoptallus_AI : public BossAI
        {
            boss_hoptallus_AI(Creature* creature) : BossAI(creature, DATA_HOPTALLUS_EVENT), summons(me)
            {
                instance = creature->GetInstanceScript();
                creature->ApplySpellImmune(0, IMMUNITY_ID, 111666, true); // Immune to Smash.
                introStarted = false;
                introDone    = false;
            }

            EventMap events;
            InstanceScript* instance;
            SummonList summons;
            bool introStarted, introDone;

            void InitializeAI() OVERRIDE
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset() OVERRIDE
            {
                events.Reset();
                summons.DespawnAll();

                if (instance)
                    instance->SetData(DATA_HOPTALLUS_EVENT, NOT_STARTED);

                _Reset();

                if (!introStarted)
                {
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->GetMotionMaster()->MovePoint(1, hoptallusMovePosition);
                    introStarted = true;
                }
            }

            void MovementInform(uint32 type, uint32 id) OVERRIDE
            {
                if (!me->IsAlive() || type != POINT_MOTION_TYPE || id != 1)
                    return;

                events.ScheduleEvent(EVENT_SET_COMBAT, 200);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);

                if (instance)
                {
                    instance->SetData(DATA_HOPTALLUS_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }

                // Carrot Breath handled through EVENT_FURLWIND and the other way around.
                events.ScheduleEvent(EVENT_FURLWIND, urand(13000, 16000));
                events.ScheduleEvent(EVENT_SUMMON_NPCS, urand(48000, 52000));

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
                    instance->SetData(DATA_HOPTALLUS_EVENT, FAIL);
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
                    instance->SetData(DATA_HOPTALLUS_EVENT, DONE);
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
                if (!UpdateVictim() && introDone)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_FURLWIND:
                            Talk(SAY_FURLWIND);
                            DoCast(me, SPELL_FURLWIND);
                            events.ScheduleEvent(EVENT_CARROT_BREATH, urand(17000, 19000)); // Always 18 seconds after Furlwind.
                            break;

                        case EVENT_CARROT_BREATH:
                            Talk(SAY_CARROT_BREATH);
                            DoCast(me, SPELL_CARROT_BREATH);
                            events.ScheduleEvent(EVENT_FURLWIND, urand(24000, 26000)); // Always 25 seconds after Carrot Breath.
                            break;

                        case EVENT_SUMMON_NPCS:
                            me->MonsterTextEmote(ANN_BUNNIES, NULL, true);
                            CallNPCS();
                            events.ScheduleEvent(EVENT_SUMMON_NPCS, urand(48000, 52000)); // Constant.
                            break;

                        case EVENT_SET_COMBAT:
                            me->SetFacingTo(1.879f);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            me->GetMotionMaster()->MovementExpired();
                            me->GetMotionMaster()->Clear();
                            introDone = true;
                            break;

                        default: break;
                    }
                }

                if (introDone && me->IsInCombat())
                    DoMeleeAttackIfReady();
            }

        // Special functions here.
        private:
            void CallNPCS()
            {
                // 15 Hoppling, 7 Hopper, 5 Bopper.
                for (uint8 i = 0; i < 15; i++)
                    if (Creature* hoppling = me->SummonCreature(NPC_HOPPLING, bunnySummonPosition, TEMPSUMMON_MANUAL_DESPAWN))
                        hoppling->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 5.0f, 10.0f);

                for (uint8 i = 0; i < 7; i++)
                    if (Creature* hopper = me->SummonCreature(NPC_HOPPER, bunnySummonPosition, TEMPSUMMON_MANUAL_DESPAWN))
                        hopper->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 5.0f, 10.0f);

                for (uint8 i = 0; i < 5; i++)
                    if (Creature* bopper = me->SummonCreature(NPC_BOPPER, bunnySummonPosition, TEMPSUMMON_MANUAL_DESPAWN))
                        bopper->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 5.0f, 10.0f);
            }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_hoptallus_AI(creature);
    }
};

// Hopper 59464.
class npc_hopper : public CreatureScript
{
    public :
        npc_hopper() : CreatureScript("npc_hopper") { }

        struct npc_hopper_AI : public ScriptedAI
        {
            npc_hopper_AI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                me->AddAura(SPELL_KEG_CARRY, me);
                Reset();
            }

            void Reset() OVERRIDE
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                events.ScheduleEvent(EVENT_CHECK_PLAYERS, 1000);
            }

            void EnterEvadeMode() OVERRIDE
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_CHECK_PLAYERS:
                            if (Player* nearPlayer = me->FindNearestPlayer(3.0f))
                            {
                                if (nearPlayer->IsWithinDistInMap(me, 3.0f))
                                    DoCast(me, SPELL_EXPLOSIVE_BREW);
                                else
                                    events.ScheduleEvent(EVENT_CHECK_PLAYERS, 1000);
                            }
                            else
                                events.ScheduleEvent(EVENT_CHECK_PLAYERS, 1000);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_hopper_AI(creature);
        }
};

// Bopper 59551.
class npc_bopper : public CreatureScript
{
    public :
        npc_bopper() : CreatureScript("npc_bopper") { }

        struct npc_bopper_AI : public ScriptedAI
        {
            npc_bopper_AI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                Reset();
            }

            void Reset() OVERRIDE { }

            void EnterEvadeMode() OVERRIDE
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->DespawnOrUnsummon();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (Unit* hammer = me->SummonCreature(NPC_BOPPER_HAMMER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                {
                    hammer->ToCreature()->SetReactState(REACT_PASSIVE);
                    hammer->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    hammer->AddAura(SPELL_HAMMER_VISUAL, hammer);
                    hammer->AddAura(SPELL_HAMMER_ARROW, hammer);
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_bopper_AI(creature);
        }
};

// Giant hammer 59539.
class npc_hoptallus_bopper_hammer : public CreatureScript
{
    public:
        npc_hoptallus_bopper_hammer() : CreatureScript("npc_hoptallus_bopper_hammer") { }

        struct npc_hoptallus_bopper_hammerAI : public PassiveAI
        {
            npc_hoptallus_bopper_hammerAI(Creature* creature) : PassiveAI(creature) { }

            void OnSpellClick(Unit* clicker, bool& result) OVERRIDE
            {
                if (result)
				{
                    clicker->AddAura(SPELL_SMASH_AURA, clicker);
                    me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    me->DespawnOrUnsummon();
				}
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_hoptallus_bopper_hammerAI(creature);
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

class PlayerCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit PlayerCheck(Unit* _caster) : caster(_caster) { }
        bool operator()(WorldObject* object)
        {
            return object->GetTypeId() != TYPEID_PLAYER;
        }

    private:
        Unit* caster;
};

// Carrot Breath 112944.
class spell_hoptallus_carrot_breath : public SpellScriptLoader
{
public:
    spell_hoptallus_carrot_breath() : SpellScriptLoader("spell_hoptallus_carrot_breath") { }

    class spell_hoptallus_carrot_breath_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hoptallus_carrot_breath_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            PreventDefaultAction(); // Don't apply stupid dummy aura.
        }

        void Register() OVERRIDE
        {
            OnEffectApply += AuraEffectApplyFn(spell_hoptallus_carrot_breath_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const OVERRIDE
    {
        return new spell_hoptallus_carrot_breath_AuraScript();
    }
};

// Hoptallus Carrot breath damage 112945.
class spell_hoptallus_carrot_breath_damage : public SpellScriptLoader
{
public:
    spell_hoptallus_carrot_breath_damage() : SpellScriptLoader("spell_hoptallus_carrot_breath_damage") { }

    class spell_hoptallus_carrot_breath_damageSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hoptallus_carrot_breath_damageSpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(PositionCheck(GetCaster()));
        }

        void Register() OVERRIDE
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hoptallus_carrot_breath_damageSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hoptallus_carrot_breath_damageSpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_CONE_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hoptallus_carrot_breath_damageSpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_CONE_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_hoptallus_carrot_breath_damageSpellScript();
    }
};

// Hoptallus Furlwind damage 112993.
class spell_hoptallus_furlwind_damage : public SpellScriptLoader
{
public:
    spell_hoptallus_furlwind_damage() : SpellScriptLoader("spell_hoptallus_furlwind_damage") { }

    class spell_hoptallus_furlwind_damageSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hoptallus_furlwind_damageSpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(PlayerCheck(GetCaster()));
        }

        void Register() OVERRIDE
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hoptallus_furlwind_damageSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_hoptallus_furlwind_damageSpellScript();
    }
};

void AddSC_boss_hoptallus()
{
	new boss_hoptallus();
	new npc_hopper();
	new npc_bopper();
    new npc_hoptallus_bopper_hammer();
    new spell_hoptallus_carrot_breath();
    new spell_hoptallus_carrot_breath_damage();
    new spell_hoptallus_furlwind_damage();
}
