/*Copyright (C) 2013 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
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
#include "MoveSplineInit.h"
#include "Player.h"
#include "Weather.h"

#include "halls_of_origination.h"

#define SAY_ANNOUNCE "Rajh's energy has dropped low!"
#define SAY_JUMP "Rajh is about to leap on a nearby target!"

enum Texts
{
    SAY_AGGRO                           = 0,
    SAY_BLESSING                        = 1,
    SAY_KILL                            = 2,
    SAY_DEATH                           = 3
};

enum Spells
{
    SPELL_NO_REGEN                      = 78725,

    SPELL_SUN_STRIKE                    = 73872,
    SPELL_SUN_STRIKE_PERIODIC           = 73874,

    SPELL_SUMMON_ORB                    = 80352,
    SPELL_SUN_ORB_VISUAL                = 73658,

    SPELL_BLAZING_INFERNO_N             = 76195,
    SPELL_BLAZING_INFERNO_H             = 89877,

    SPELL_WINDS_VISUAL                  = 74109,
    SPELL_SOLAR_WINDS                   = 74104, // trigger 74105 summoning 39634. This casts 74106 summoning vortex 39635.
    SPELL_SOLAR_WINDS_PERIODIC          = 74107, // summon flames on the ground also. Triggers 74108 summoning 47922 on the ground.
    SPELL_SUMMON_WINDS                  = 74106, // By trigger.

    SPELL_SOLAR_FIRE                    = 89131, // damage for the leftover flames

    SPELL_INFERNO_LEAP_MOB_SUMMON       = 87650,
    SPELL_INFERNO_LEAP_CAST_TIME        = 87653, // actual spell, Jumps
    SPELL_INFERNO_LEAP_BUFF             = 87645, // visual aura for inferno leap target
    SPELL_ADRENALINE                    = 87657, // Casted by Leap Target
    SPELL_INFERNO_LEAP                  = 87647, // Damage

    SPELL_BLESSING_RECHARGE             = 76352,
    SPELL_SELF_ROOT                     = 42716
};

enum Creatures
{
    NPC_WINDS                           = 39634,
    NPC_SOLAR_WINDS                     = 39635,
    NPC_SUN_ORB                         = 40835,
    NPC_INFERNO_LEAP                    = 47040
};

class boss_rajh : public CreatureScript
{
    public:
        boss_rajh() : CreatureScript("boss_rajh") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_rajhAI(creature);
        }
            
        struct boss_rajhAI : public ScriptedAI
        {
            boss_rajhAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                ASSERT(creature->GetVehicleKit());
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            uint32 m_uiSunStrikeTimer;
            uint32 m_uiWindsTimer;
            uint32 m_uiSunOrbTimer;
            uint32 m_uiLeapTimer;
            uint32 leap_timer;
            uint32 leap_despawn_timer;
            bool leap;

            void Reset() OVERRIDE
            {
                if (instance)
                    instance->SetData(DATA_RAJH_EVENT, NOT_STARTED);

                me->RemoveAurasDueToSpell(SPELL_NO_REGEN);
                me->SetPower(POWER_ENERGY, 100);
                me->SetMaxPower(POWER_ENERGY, 100);
                m_uiSunStrikeTimer = 5000;
                m_uiWindsTimer = urand(7000, 12000);
                m_uiSunOrbTimer = urand(20000, 24000);
                m_uiLeapTimer = urand(17000, 19000);
                leap = false;
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
		    	summons.Summon(summon);
		    	summon->setActive(true);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);
                DoCast(me, SPELL_NO_REGEN);

                if (instance)
                {
                    instance->SetData(DATA_RAJH_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();
                DespawnWinds();

                if (IsHeroic())
                    if (!me->GetMap()->GetPlayers().isEmpty())
                        for (Map::PlayerList::const_iterator i = me->GetMap()->GetPlayers().begin(); i != me->GetMap()->GetPlayers().end(); ++i)
                            if (i->GetSource())
                                i->GetSource()->KilledMonsterCredit(48815, 0);

                if (instance)
                {
                    instance->SetData(DATA_RAJH_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void EnterEvadeMode() OVERRIDE
            {
                summons.DespawnAll();
                DespawnWinds();
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(false);
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->SetData(DATA_RAJH_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }
            }

            void DespawnWinds()
            {
                std::list<Creature*> winds;
                GetCreatureListWithEntryInGrid(winds, me, NPC_SOLAR_WINDS, 100.0f);
                if (!winds.empty())
                    for (std::list<Creature*>::iterator iter = winds.begin(); iter != winds.end(); ++iter)
                        (*iter)->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() && !me->HasAura(SPELL_BLESSING_RECHARGE) || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (me->GetPower(POWER_ENERGY) < 0)
                    me->SetPower(POWER_ENERGY, 0);

                if (me->GetPower(POWER_ENERGY) == 0 && !me->HasAura(SPELL_BLESSING_RECHARGE))
                {
                    Talk(SAY_BLESSING);
                    me->MonsterTextEmote(SAY_ANNOUNCE, NULL, true);
                    me->NearTeleportTo(-308.792f, 196.908f, 343.942f, 3.144f);
                    DoCast(me, SPELL_BLESSING_RECHARGE);
                    me->SetReactState(REACT_PASSIVE);
                    me->AddAura(SPELL_SELF_ROOT, me);
                }

                if (me->GetPower(POWER_ENERGY) == 100 && me->HasAura(SPELL_BLESSING_RECHARGE))
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveAura(SPELL_BLESSING_RECHARGE);
                    me->RemoveAura(SPELL_SELF_ROOT);
                }

                if (m_uiSunStrikeTimer <= diff)
                {
                    if (!me->HasAura(SPELL_BLESSING_RECHARGE))
                        DoCastVictim(SPELL_SUN_STRIKE);
                    m_uiSunStrikeTimer = urand(22000, 27000);
                }
                else m_uiSunStrikeTimer -= diff;

                if (m_uiWindsTimer <= diff)
                {
                    if (!me->HasAura(SPELL_BLESSING_RECHARGE))
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                            DoCast(target, SPELL_SOLAR_WINDS);
                    m_uiWindsTimer = IsHeroic() ? urand(29000, 31000) : urand(20000, 21500);
                }
                else m_uiWindsTimer -= diff;

                if (m_uiLeapTimer <= diff)
                {
                    if (!me->HasAura(SPELL_BLESSING_RECHARGE))
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                    {
                        me->MonsterTextEmote(SAY_JUMP, NULL, true);
                        DoCast(target, SPELL_INFERNO_LEAP_MOB_SUMMON);
                        leap_timer = 1000;
                        leap_despawn_timer = 2600;
                        leap = true;

                        if (Creature* Summoned = me->FindNearestCreature(NPC_INFERNO_LEAP, 100, true))
                        {
                            Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            Summoned->SetFloatValue(OBJECT_FIELD_SCALE, Summoned->GetFloatValue(OBJECT_FIELD_SCALE) * 2.5f);
                            Summoned->setFaction(me->getFaction());
                            Summoned->SetLevel(me->getLevel());
                        }
                    }

                    m_uiLeapTimer = urand(18000, 22000);
                }
                else m_uiLeapTimer -= diff;

                if (leap_timer <= diff && leap)
                {
                    if (Creature* Summoned = me->FindNearestCreature(NPC_INFERNO_LEAP, 100, true))
                        DoCast(Summoned, SPELL_INFERNO_LEAP_CAST_TIME); // Control it.
                    leap = false;
                }
                else leap_timer -= diff;

                if (leap_despawn_timer <= diff)
                {
                    if (Creature* Summoned = me->FindNearestCreature(NPC_INFERNO_LEAP, 100, true))
                        Summoned->DisappearAndDie();
                    leap_despawn_timer = -1;
                }
                else leap_despawn_timer -= diff;

                if (m_uiSunOrbTimer <= diff)
                {
                    if (!me->HasAura(SPELL_BLESSING_RECHARGE))
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                            DoCast(target, SPELL_SUMMON_ORB);
                    m_uiSunOrbTimer = urand(18000, 25000);
                }
                else m_uiSunOrbTimer -= diff;

                if (!me->HasAura(SPELL_BLESSING_RECHARGE))
                    DoMeleeAttackIfReady();
            }
        };
};

class npc_sun_orb : public CreatureScript
{
    public:
        npc_sun_orb() : CreatureScript("npc_sun_orb") { }

        struct npc_sun_orbAI : public ScriptedAI
        {
            npc_sun_orbAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->CastSpell(creature, SPELL_SUN_ORB_VISUAL, false);
                creature->SetSpeed(MOVE_WALK, 0.7f);
                creature->SetSpeed(MOVE_RUN, 0.7f);
                creature->GetMotionMaster()->MovePoint(1, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ() - 10.0f);
            }

            InstanceScript* instance;

            void MovementInform(uint32 type, uint32 point) OVERRIDE
            {
                if (type != POINT_MOTION_TYPE)
                    return;
    
                if (point == 1)
                {
                    DoCast(me, DUNGEON_MODE(SPELL_BLAZING_INFERNO_N, SPELL_BLAZING_INFERNO_H));
                    me->DespawnOrUnsummon(100);
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_sun_orbAI(creature);
        }
};

class npc_inferno_leap : public CreatureScript
{
    public:
        npc_inferno_leap() : CreatureScript("npc_inferno_leap") { }

        struct npc_inferno_leapAI : public ScriptedAI
        {
            npc_inferno_leapAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->CastSpell(creature, SPELL_INFERNO_LEAP_BUFF, false);
                creature->CastSpell(creature, SPELL_ADRENALINE, false);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_inferno_leapAI(creature);
        }
};

class npc_solar_wind : public CreatureScript // 39634
{
    public:
        npc_solar_wind() : CreatureScript("npc_solar_wind") { }

        struct npc_solar_windAI : public ScriptedAI
        {
            npc_solar_windAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->CastSpell(creature, SPELL_SUMMON_WINDS, false);
                creature->DespawnOrUnsummon(2000);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_solar_windAI(creature);
        }
};

class npc_winds : public CreatureScript // 39635
{
    public:
        npc_winds() : CreatureScript("npc_winds") { }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_windsAI(creature);
        }

        struct npc_windsAI : public ScriptedAI
        {
            npc_windsAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();

                creature->SetReactState(REACT_PASSIVE);
                creature->ApplySpellImmune(0, IMMUNITY_ID, 74108, true);
                creature->ApplySpellImmune(0, IMMUNITY_ID, 89130, true);
                creature->SetSpeed(MOVE_WALK, 0.9f);
                creature->SetSpeed(MOVE_RUN, 0.9f);
                creature->CastSpell(creature, SPELL_SOLAR_WINDS_PERIODIC, false);
                creature->CastSpell(creature, SPELL_WINDS_VISUAL, false);
                creature->GetMotionMaster()->MovementExpired();
                creature->GetMotionMaster()->Clear();
                creature->GetMotionMaster()->MoveRandom(50.0f);
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) OVERRIDE { } // No melee.
        };
};

class spell_rajh_sun_orb_summon : public SpellScriptLoader // 76338
{
public:
    spell_rajh_sun_orb_summon() : SpellScriptLoader("spell_rajh_sun_orb_summon") { }

    class spell_rajh_sun_orb_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_rajh_sun_orb_summon_SpellScript);

        void ModDestHeight(SpellEffIndex /*effIndex*/)
        {
            Position offset = {0.0f, 0.0f, 10.0f, 0.0f};
            const_cast<WorldLocation*>(GetExplTargetDest())->RelocateOffset(offset);
            GetHitDest()->RelocateOffset(offset);
        }

        void Register() OVERRIDE
        {
            OnEffectLaunch += SpellEffectFn(spell_rajh_sun_orb_summon_SpellScript::ModDestHeight, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_rajh_sun_orb_summon_SpellScript();
    }
};

void AddSC_boss_rajh()
{
    new boss_rajh();
    new npc_sun_orb();
    new npc_inferno_leap();
    new npc_solar_wind();
    new npc_winds();
    new spell_rajh_sun_orb_summon();
}
