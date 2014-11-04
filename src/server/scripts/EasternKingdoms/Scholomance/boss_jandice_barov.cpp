#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

enum Spells
{
    SPELL_GRAVITY_FLUX_DMG  = 114038,
    SPELL_GRAVITY_FLUX      = 114059,
    SPELL_WHIRL_OF_ILLUSION = 114048,
    SPELL_WONDROUS_RAPID    = 114062
};

enum Talk
{
    SAY_AGGRO               = 0, // Ooh, it takes some real stones to challenge the Mistress of Illusion. Well? Show me what you've got! (30287)
    SAY_WHIRL_ILLUSION_1    = 1, // Come, try your luck! Ha ha haaa... (30292)
    SAY_WHIRL_ILLUSION_2    = 2, // Feeling a bit... dizzy? (30293)
    SAY_KILLER_1            = 3, // Ashes, ashes, we all fall down. (30289)
    SAY_KILLER_2            = 4, // Oh, careful not to bleed on the rug, please dear? (30290)
    SAY_WIPE_RESET          = 5, // Better luck next time. (30291)
    SAY_DEATH               = 6, // Jandice Barov yells: Ugh... death... hurts? Unreal... (30288)
};

enum Events
{
    EVENT_GRAVITY,
    EVENT_FINISH_INVIS,
    EVENT_WOUNDROUS_RAPID
};

Position const IllusionPos[9] =
{
    { 300.248f, 46.374f, 113.409f, 0.0f },
    { 299.957f, 31.166f, 113.409f, 0.0f },
    { 299.764f, 21.024f, 113.409f, 0.0f },
    { 280.708f, 31.672f, 113.408f, 0.0f },
    { 281.195f, 21.988f, 113.408f, 0.0f },
    { 262.164f, 32.343f, 113.409f, 0.0f },
    { 261.658f, 21.720f, 113.409f, 0.0f },
    { 262.375f, 46.644f, 113.409f, 0.0f },
    { 282.495f, 45.559f, 113.408f, 0.0f },
};

Position const IllusionHeroPos[15] =
{
    { 300.248f, 46.374f, 113.409f, 0.f },
    { 299.957f, 31.166f, 113.409f, 0.f },
    { 299.764f, 21.024f, 113.409f, 0.f },
    { 280.708f, 31.672f, 113.408f, 0.f },
    { 281.195f, 21.988f, 113.408f, 0.f },
    { 262.164f, 32.343f, 113.409f, 0.f },
    { 261.658f, 21.720f, 113.409f, 0.f },
    { 262.375f, 46.644f, 113.409f, 0.f },
    { 282.495f, 45.559f, 113.408f, 0.f },
    { 271.66f,  21.244f, 113.408f, 0.f },
    { 271.631f, 31.055f, 113.408f, 0.f },
    { 272.426f, 46.041f, 113.408f, 0.f },
    { 291.272f, 46.044f, 113.408f, 0.f },
    { 290.763f, 31.190f, 113.408f, 0.f },
    { 290.394f, 21.659f, 113.408f, 0.f },
};

class boss_jandice_barov : public CreatureScript
{
    public:
        boss_jandice_barov() : CreatureScript("boss_jandice_barov") { }

        struct boss_jandice_barovAI : public BossAI
        {
            boss_jandice_barovAI(Creature* creature) : BossAI(creature, DATA_JANDICE_BAROV) { }

            void Reset()
            {
                _Reset();
                Pone = false;
                Ptwo = false;
                summons.DespawnAll();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_GRAVITY, 12000);
                events.ScheduleEvent(EVENT_WOUNDROUS_RAPID, 20000);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void SummonIllusions()
            {
                if (IsHeroic())
                {
                    for (uint8 i = 0; i < 15; ++i)
                        me->SummonCreature(59220, IllusionHeroPos[i], TEMPSUMMON_DEAD_DESPAWN, 0);
                }
                else

                for (uint8 i = 0; i < 9; ++i)
                    me->SummonCreature(59220, IllusionPos[i], TEMPSUMMON_DEAD_DESPAWN, 0);

                PrepInvis();
            }

            void PrepInvis()
            {
                me->InterruptNonMeleeSpells(false);
                me->AddUnitState(UNIT_STATE_ROOT);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetVisible(false);
                DoModifyThreatPercent(me->getVictim(), -99);
                events.ScheduleEvent(EVENT_FINISH_INVIS, urand(15000, 25000));
                events.CancelEvent(EVENT_WOUNDROUS_RAPID);
                events.CancelEvent(EVENT_GRAVITY);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {
                if(!Pone && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(66))
                {
                    Pone = true;
                    Talk(RAND(SAY_WHIRL_ILLUSION_1, SAY_WHIRL_ILLUSION_2));
                    SummonIllusions();
                    DoCastAOE(SPELL_WHIRL_OF_ILLUSION);
                }
                if(!Ptwo && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(33))
                {
                    Ptwo = true;
                    Talk(RAND(SAY_WHIRL_ILLUSION_1, SAY_WHIRL_ILLUSION_2));
                    SummonIllusions();
                    DoCastAOE(SPELL_WHIRL_OF_ILLUSION);
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                summons.DespawnAll();
            }

            void KilledUnit(Unit* victim)
            {
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
                        case EVENT_GRAVITY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            {
                                if (Creature* fluxtarget = me->SummonCreature(CREATURE_GRAVITY_FLUX, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 30000))
                                {
                                    fluxtarget->setFaction(target->getFaction());
                                    fluxtarget->GetMotionMaster()->MoveIdle();
                                    fluxtarget->CastSpell(target, SPELL_GRAVITY_FLUX, true);
                                    fluxtarget->SetReactState(REACT_AGGRESSIVE);
                                }
                            }
                            events.ScheduleEvent(EVENT_GRAVITY, 25000);
                            break;
                        case EVENT_FINISH_INVIS:
                            me->ClearUnitState(UNIT_STATE_ROOT);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->SetVisible(true);
                            me->RemoveAura(SPELL_WHIRL_OF_ILLUSION);
                            events.ScheduleEvent(EVENT_WOUNDROUS_RAPID, urand(25000, 30000));
                            events.ScheduleEvent(EVENT_GRAVITY, 25000);
                            summons.DespawnAll();
                            break;
                        case EVENT_WOUNDROUS_RAPID:
                            DoCastVictim(SPELL_WONDROUS_RAPID);
                            events.ScheduleEvent(EVENT_WOUNDROUS_RAPID, urand(25000, 30000));
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
            private:
                bool Pone;
                bool Ptwo;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_jandice_barovAI(creature);
        }
};

class mob_gravity_flux : public CreatureScript
{
public:
    mob_gravity_flux() : CreatureScript("mob_gravity_flux") { }

    struct mob_gravity_fluxAI : public ScriptedAI
    {
        mob_gravity_fluxAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() { }

        void EnterCombat(Unit* /*who*/) { }

        void MoveInLineOfSight(Unit* who)
        {
        if (who && who->GetTypeId() == TYPEID_PLAYER && me->IsValidAttackTarget(who))

            if (!searchplayer && me->IsWithinDistInMap(who, 4.9f))
            {
                searchplayer = true;
                DoCast(who, SPELL_GRAVITY_FLUX_DMG);
                ScriptedAI::MoveInLineOfSight(who);
            }
        }
        private:
            bool searchplayer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_gravity_fluxAI (creature);
    }
};

class mob_jandice_barov_illusion : public CreatureScript
{
public:
    mob_jandice_barov_illusion() : CreatureScript("mob_jandice_barov_illusion") { }

    struct mob_jandice_barov_illusionAI : public ScriptedAI
    {
        mob_jandice_barov_illusionAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterCombat(Unit* /*who*/) { }

        void IsSummonedBy(Unit* summoner)
        {
            me->GetMotionMaster()->MoveIdle();
//            me->SetTarget(0);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_jandice_barov_illusionAI (creature);
    }
};

void AddSC_boss_jandice_barov()
{
    new boss_jandice_barov();
    new mob_gravity_flux();
    new mob_jandice_barov_illusion();
}
