#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

enum Spells
{
    // Lilian
    SPELL_SHADOW_SHIV     = 111775,
    SPELL_DEATHS_GRASP    = 111570,
    SPELL_GRASP_GRIP      = 111573,
    SPELL_DARK_BLAZE      = 111585,

    // Misc
    SPELL_SOUL_COSMETIC   = 112057,
    SPELL_SUMMON_SOUL     = 112063,
    SPELL_COMBINE_VISUAL  = 111687,
    SPELL_GANDLING_VISUAL = 114200,

    // Lilians Soul
    SPELL_UNLEASH_ANGUISH = 111649,
    SPELL_FIXATE_ANGER    = 115350
};

enum Talk
{
    SAY_INTRO     = 0, // I...wont let you... (29464) yell
    SAY_AGGRO     = 1, // I cant...fight him... (29465) normal talk
    SAY_SOUL_BURN = 2, // It burns...my soul...it burns!!! (29468) yell
    SAY_EXTRACT   = 3, // END IT! NOW! Before he can... (29463) yell
    SAY_DONE      = 4, // DIE, NECROMANCER!(29466) yell
};

enum Events
{
    EVENT_DEATHS_GRASP,
    EVENT_SHADOW_SHIV,
    EVENT_DARK_BLAZE,
    EVENT_TAKE_SOUL,
    EVENT_EXTRACT_SOUL,
    EVENT_FAKE_DEATH,
    EVENT_START_PHASE2,
    EVENT_FOCUS_PLAYER,
    EVENT_RESURECT,
    EVENT_START_PHASE3
};

enum Actions
{
    ACTION_START_PHASE2 = 1,
    ACTION_RESURECT     = 2
};

Position MoveCenterPos = { 200.845f, 83.903f, 107.762f, 0.0f };

class boss_lillian_voss : public CreatureScript
{
    public:
        boss_lillian_voss() : CreatureScript("boss_lillian_voss") { }

        struct boss_lillian_vossAI : public BossAI
        {
            boss_lillian_vossAI(Creature* creature) : BossAI(creature, DATA_LILLIAN_VOSS)
            {
                Instance = creature->GetInstanceScript();
            }

            InstanceScript* Instance;

            void Reset()
            {
                _Reset();
                soulphase = false;
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->GetMotionMaster()->Clear();
                me->SetReactState(REACT_DEFENSIVE);
                me->SetDisableGravity(false);
                me->SetHover(false);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_DEATHS_GRASP, 5000);
                events.ScheduleEvent(EVENT_SHADOW_SHIV, 18000);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void KilledUnit(Unit* victim)
            {
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if(!soulphase && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(60))
                {
                    soulphase = true;
                    me->AttackStop();
                    DoCast(me, SPELL_SOUL_COSMETIC);
                    me->SetReactState(REACT_PASSIVE);
                    me->SetDisableGravity(true);
                    me->GetMotionMaster()->MovePoint(0, MoveCenterPos);
                    events.ScheduleEvent(EVENT_TAKE_SOUL, 9000);
                }

                if (damage >= me->GetHealth())
                {
                    damage = 0;

                    Talk(SAY_DONE);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MovePoint(0, MoveCenterPos);
                    me->RemoveAllAttackers();
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->InterruptNonMeleeSpells(true);
                    me->setFaction(35);
                }
             }

            void DoAction(int32 const action)
            {
                if (action == ACTION_RESURECT)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if(!UpdateVictim())
                    return;

                events.Update(diff);

                if(me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_DEATHS_GRASP:
                            DoCastVictim(SPELL_DEATHS_GRASP, false);
                            events.ScheduleEvent(EVENT_DEATHS_GRASP, 10000);
                            events.ScheduleEvent(EVENT_DARK_BLAZE, 3300);
                            break;
                        case EVENT_SHADOW_SHIV:
                            DoCastVictim(SPELL_SHADOW_SHIV);
                            events.ScheduleEvent(EVENT_SHADOW_SHIV, 20000);
                            break;
                        case EVENT_DARK_BLAZE:
                            Talk(SAY_SOUL_BURN);
                            DoCastAOE(SPELL_DARK_BLAZE);
                            break;
                        case EVENT_TAKE_SOUL:
                            Talk(SAY_EXTRACT);
                            me->SetHover(true);
                            me->SetSpeed(MOVE_FLIGHT, 0.1f);
                            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+5.0f);
                            events.ScheduleEvent(EVENT_EXTRACT_SOUL, 10000);
                        case EVENT_EXTRACT_SOUL:
                            DoCast(me, SPELL_SUMMON_SOUL);
                            events.ScheduleEvent(EVENT_FAKE_DEATH, 8000);
                            break;
                        case EVENT_FAKE_DEATH:
                            DoCastAOE(SPELL_COMBINE_VISUAL);
                            me->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                            me->GetMotionMaster()->MoveFall();
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->getThreatManager().resetAllAggro();
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        private:
            bool soulphase;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lillian_vossAI(creature);
        }
};

class boss_lillian_voss_soul : public CreatureScript
{
public:
    boss_lillian_voss_soul() : CreatureScript("boss_lillian_voss_soul") { }

    struct boss_lillian_voss_soulAI : public BossAI
    {
        boss_lillian_voss_soulAI(Creature* creature) : BossAI(creature, DATA_LILLIAN_VOSS_SOUL)
        {
            Instance = creature->GetInstanceScript();
        }

        InstanceScript* Instance;

        void Reset()
        {
//            me->SetReactState(REACT_PASSIVE);
//            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }

/*
        void DoAction(int32 const action) // will be included soon (needs correct retail infos)
        {
            if (action == ACTION_START_PHASE2)
            {
            }
        }
*/
        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            DoCast(me, SPELL_UNLEASH_ANGUISH);
            me->SetSpeed(MOVE_RUN, 0.5f, true);
            events.ScheduleEvent(EVENT_FOCUS_PLAYER, 3000);
        }

        void DamageTaken(Unit* /*who*/, uint32& damage)
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;

                me->RemoveAllAuras();
                me->GetMotionMaster()->MovePoint(0, MoveCenterPos);
                me->RemoveAllAttackers();
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->InterruptNonMeleeSpells(true);
                events.ScheduleEvent(EVENT_START_PHASE3, 5000);
            }
        }

        void ResurectVoss()
        {
            if (Instance)
            {
                if (Creature* Lilian = Instance->instance->GetCreature(Instance->GetData64(DATA_LILLIAN_VOSS)))
                    if (Lilian->AI())
                        Lilian->AI()->DoAction(ACTION_RESURECT);
            }
        }

        void UpdateAI(uint32 const diff)
        {
            if(!UpdateVictim())
                return;

            events.Update(diff);

            if(me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if(uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_FOCUS_PLAYER: // spellscript required (soon)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        {
                            me->AddAura(SPELL_FIXATE_ANGER, target);
                            me->Attack(target, true);
                            DoModifyThreatPercent(target, -99);
                        }
                        events.ScheduleEvent(EVENT_FOCUS_PLAYER, 10000);
                        break;
                    case EVENT_START_PHASE3:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoCast(me, SPELL_UNLEASH_ANGUISH);
                        ResurectVoss();
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_lillian_voss_soulAI (creature);
    }
};

class spell_lillian_grip : public SpellScriptLoader
{
    public:
        spell_lillian_grip() : SpellScriptLoader("spell_lillian_grip") { }

        class spell_lillian_grip_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_lillian_grip_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_GRASP_GRIP))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->CastSpell(GetCaster(), SPELL_GRASP_GRIP, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_lillian_grip_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_lillian_grip_SpellScript();
        }
};

void AddSC_boss_lillian_voss()
{
    new boss_lillian_voss();
    new boss_lillian_voss_soul();
    new spell_lillian_grip();
}
