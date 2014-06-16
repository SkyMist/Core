/*Copyright (C) 2014 Buli.
*
* This file is NOT free software. Third-party users may NOT redistribute it or modify it :).
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
#include "Unit.h"
#include "Player.h"
#include "Weather.h"

#include "heart_of_fear.h"

enum Yells
{
    // Blade Lord Ta'yak
    SAY_ENTER_ROOM          = 0, // Now go, impart my techniques to the initiates.

    SAY_KILL_TRASH          = 1, // 0 - Mere initiates. ; 1 - They should have studied harder. ; 2 - One technique alone is not enough. ; 3 - They failed their test. Will you?
    SAY_INTRO               = 2, // They approach? Good. Now, if only my students were here to see and learn from the coming display of martial prowess...

    SAY_AGGRO               = 3, // On your guard, invaders. I, Ta'yak, Lord of Blades, will be your opponent.
    SAY_SLAY                = 4, // 0 - A perfect cut. ; 1 - This is the technique of a Blade Lord.
    SAY_DEATH               = 5, // I'd always hoped to cut someone like that someday, to hear that sound. But to have it happen to my own prothorax is ridiculous.

    SAY_STORM_UNLEASHED     = 6, // Can you follow my blade?

    ANN_UNSEEN              = 7  // Blade Lord Ta'yak marks $N for [Unseen Strike]!
};

enum Spells
{
    /*** Blade Lord Ta'yak ***/

    // Tempest Slash - Launches a Tornado towards a player location; upon reaching it, tornado spins around at the spot.
    SPELL_TEMP_SLASH_SUMM_V = 125692, // Summons Heart of Fear - Trash Version Tempest Stalker (LTD).
    SPELL_TEMP_SLASH_AURA   = 122854, // Periodic trigger aura for SPELL_TEMP_SLASH_DAMAGE.
    SPELL_TEMP_SLASH_DAMAGE = 122853,

    // Unseen Strike - Boss disappears, appears at a player, massive damage split between targets in 15 yards cone.
    SPELL_UNSEEN_STRIKE_TR  = 122949, // Unattackable + Speed 200%. Triggers SPELL_UNSEEN_STRIKE_DMG after 5 secs, SPELL_UNSEEN_STRIKE_MKR on target, SPELL_UNSEEN_STRIKE_INV on self.
    SPELL_UNSEEN_STRIKE_DMG = 122994, // Damage.
    SPELL_UNSEEN_STRIKE_MKR = 123017, // Target marker visual aura.
    SPELL_UNSEEN_STRIKE_INV = 125242, // Invisibility aura.

    // Wind Step - Teleports to a player, casts the bleed, teleports back.
    SPELL_WIND_STEP_TP      = 123175, // Teleport. Triggers SPELL_WIND_STEP_DUMMY.
    SPELL_WIND_STEP_B_DMG   = 123180, // Bleed damage for 8y targets.
    SPELL_WIND_STEP_DUMMY   = 123459, // Dummy to apply SPELL_WIND_STEP_B_DMG to targets in 8y.
    SPELL_WIND_STEP_TP_BACK = 123460, // Teleport back to the main target.

    // Intensify - Every 60 seconds Phase 1 / 10 seconds Phase 2 (But no melee).
    SPELL_INTENSIFY_AURA    = 123470, // Triggers SPELL_INTENSIFY_BUFF every 60 secs.
    SPELL_INTENSIFY_BUFF    = 123471,

    // Overwhelming Assault.
    SPELL_OVERWHELMING_ASS  = 123474,

    // Blade tempest - Spins and pulls all players. Heroic ONLY. - Every 60 seconds.
    SPELL_BLADE_TEMPEST_AUR = 125310, // Triggers SPELL_BLADE_TEMPEST_DMG each 0.5s, SPELL_BLADE_TEMPEST_AT.
    SPELL_BLADE_TEMPEST_DMG = 125312, // Damage.
    SPELL_BLADE_TEMPEST_AT  = 125301, // Create Areatrigger 381.
    SPELL_BLADE_TEMPES_J_FC = 125325, // Force Cast SPELL_BLADE_TEMPES_JUMP in 200 yards.
    SPELL_BLADE_TEMPES_JUMP = 125327, // Player Jump-To-Boss spell.

    // Storm Unleashed - 20 % on one end, 10% on the opposite.
    SPELL_STORM_UNLEASHED_D = 123814, // Boss Dummy Visual.

    SPELL_SU_SUMMON_W1      = 123597, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed West 1 Tornado (LTD).

    SPELL_SU_AURA           = 123598, // Aura for the tornadoes, triggers SPELL_SU_RV_SE each 0.1 secs.
    SPELL_SU_RV_SE          = 124258, // Script effect for SPELL_SU_RV in 4 yards.
    SPELL_SU_RV             = 123599, // Control Vehicle aura.

    SPELL_SU_DUMMY_VIS      = 124024, // Some dummy visual (for tornadoes?).

    SPELL_SU_DMG_AURA       = 124785, // Triggers SPELL_SU_DMG every 1 sec.
    SPELL_SU_DMG            = 124783, // Damage in 300 yards.
    SPELL_SU_PACIFY         = 130908, // Pacify, Silence, 321 Aura in 200 yards.

    SPELL_BERSERK           = 120207  // Enrage, 490 seconds, or 8:10 minutes.

    // - ! NO USE FOUND FOR THESE. ! -

    // SPELL_TEMP_SLASH_SUMMON = 122838, // Summons Heart of Fear - Armsmaster Ta'yak Target Stalker (LTD).
    // SPELL_TEMP_SLASH_SUM_TR = 122839, // Force cast SPELL_TEMP_SLASH_SUMMON in 50 yards.
    // SPELL_TEMP_SLASH_T_FC   = 125689, // Force cast SPELL_TEMP_SLASH_T in 50 yards.
    // SPELL_TEMP_SLASH_T      = 125690, // Summons Heart of Fear - Trash Version Target Stalker (LTD).
    // SPELL_TEMP_SLASH_SUM_VT = 125692, // Summons Heart of Fear - Trash Version Tempest Stalker (LTD).

    // SPELL_SU_SUMMON_W2      = 123639, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed West 2 Tornado (LTD).
    // SPELL_SU_SUMMON_W3      = 123640, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed West 3 Tornado (LTD).
    // SPELL_SU_SUMMON_E1      = 123643, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed East 1 Tornado (LTD).
    // SPELL_SU_SUMMON_E2      = 123644, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed East 2 Tornado (LTD).
    // SPELL_SU_SUMMON_E3      = 123645, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed East 3 Tornado (LTD).

    // SPELL_STORM_UNLEASHED_C = 123815, // Spin dummy.
    // SPELL_SU_FORCECAST_J_FC = 123805, // Force cast SPELL_SU_FORCECAST_JUMP in 200 yards.
    // SPELL_SU_FORCECAST_JUMP = 124033, // Triggers SPELL_SU_SUMM_PS.
    // SPELL_SU_SUMMON_PS      = 124025, // Summons Heart of Fear - Armsmaster Ta'yak - Storm Unleashed Player Vehicle Stalker (LTD).
    // SPELL_SU_SUMM_DUMMY     = 123820, // Triggers SPELL_SU_DUMMY every 1.5 s.
    // SPELL_SU_DUMMY          = 123600, // Triggers 123616 on area entry in 15 y.
    // SPELL_SU_DUMMY_CRAP     = 123616, // Applies a dummy aura on a target.
};

enum Events
{
    // Blade Lord Ta'yak
    EVENT_TEMPEST_SLASH     = 1, // 10 seconds from pull. Every 15.5 seconds.
    EVENT_UNSEEN_STRIKE,         // 30.5 seconds from pull. Every 53 - 61 seconds.
    EVENT_UNSEEN_STRIKE_RETURN,
    EVENT_WIND_STEP,             // 20.5 seconds from pull. Every 25 seconds.
    EVENT_WIND_STEP_RETURN,
    EVENT_OVERWHELMING_ASS,      // 15.5 seconds from pull. Every 20.5 seconds, delayable by up to 15 seconds.

    EVENT_BLADE_TEMPEST,         // Every 60 seconds. Heroic only.

    EVENT_STORM_UNLEASHED,       // 20%, 10%.
    EVENT_SUMMON_TORNADOES,

    EVENT_BERSERK,               // Enrage at 8 minutes, or, more precisely, 490 seconds.

    // Tempest Slash tornado
    EVENT_MOVE_RANDOM
};

enum TayakPhases
{
    PHASE_NORMAL            = 1,
    PHASE_STORM_UNLEASHED   = 2
};

enum TayakCreatures
{
    NPC_US_TORNADO          = 63278
};

enum MovementPoints
{
    // Tempest Slash tornado
    POINT_TEMPEST_TARGET    = 1
};

float TayakStormPoint1[4] = { -2119.072f, 184.985f, 422.162f, 1.585f }; // 20 - 10% tele point.
float TayakStormPoint2[4] = { -2119.872f, 379.372f, 422.162f, 4.703f }; // 10 -  0% tele point.

float PlayerTelePos[4]    = { -2124.063f, 281.056f, 420.901f, 0.053f }; // Tele point for players on P2 start.

Position const Tornado1[3] = // StormPoint1 Tornado summon points.
{
    { -2123.702f, 198.023f, 420.910f, 1.561f }, // Left
    { -2119.503f, 198.023f, 420.910f, 1.561f }, // Center
    { -2114.113f, 198.023f, 420.910f, 1.561f }, // Right
};

Position const Tornado2[3] = // StormPoint2 Tornado summon points.
{
    { -2114.113f, 363.058f, 420.910f, 4.710f }, // Left
    { -2119.503f, 363.058f, 420.910f, 4.710f }, // Center
    { -2123.702f, 363.058f, 420.910f, 4.710f }, // Right
};

class boss_blade_lord_tayak : public CreatureScript
{
public:
    boss_blade_lord_tayak() : CreatureScript("boss_blade_lord_tayak") { }

    struct boss_blade_lord_tayakAI : public BossAI
    {
        boss_blade_lord_tayakAI(Creature* creature) : BossAI(creature, DATA_BLADE_LORD_TAYAK_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
            entranceDone = false;
            introDone = false;
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;
        Unit* unseenTank;
        Unit* currentTank;
        Unit* tempestTarget;
        bool entranceDone, introDone, storm1Done;
        uint8 Phase, tornadoSummonCounter;

        void InitializeAI() OVERRIDE
        {
            if (!me->isDead())
                Reset();
        }

        void Reset() OVERRIDE
        {
            events.Reset();
            summons.DespawnAll();

            unseenTank = NULL;
            currentTank = NULL;
            tempestTarget = NULL;
            storm1Done = false;
            Phase = PHASE_NORMAL;
            tornadoSummonCounter = 0;

            if (instance)
                instance->SetData(DATA_BLADE_LORD_TAYAK_EVENT, NOT_STARTED);

            _Reset();
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE
        {
            if (!entranceDone && me->IsWithinDistInMap(who, 50) && who->GetTypeId() == TYPEID_PLAYER)
            {
                Talk(SAY_ENTER_ROOM);
                entranceDone = true;
            }

            if (entranceDone && !introDone && me->IsWithinDistInMap(who, 30) && who->GetTypeId() == TYPEID_PLAYER)
            {
                Talk(SAY_INTRO);
                introDone = true;
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            DoCast(me, SPELL_INTENSIFY_AURA); // Periodic aura buff appliance.

            events.ScheduleEvent(EVENT_TEMPEST_SLASH, urand(9500, 11000));
            events.ScheduleEvent(EVENT_UNSEEN_STRIKE, urand(29500, 31500));
            events.ScheduleEvent(EVENT_WIND_STEP, urand(19500, 21500));
            events.ScheduleEvent(EVENT_OVERWHELMING_ASS, urand(14500, 16500));
            if (me->GetMap()->IsHeroic()) events.ScheduleEvent(EVENT_BLADE_TEMPEST, 60000); // Heroic only.

            events.ScheduleEvent(EVENT_BERSERK, 8 * MINUTE * IN_MILLISECONDS); // 8 minute Enrage timer.

            if (instance)
            {
                instance->SetData(DATA_BLADE_LORD_TAYAK_EVENT, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
            }

            _EnterCombat();
        }

        void KilledUnit(Unit* victim) OVERRIDE
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterEvadeMode() OVERRIDE
        {
            me->RemoveAllAuras();
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();;

            if (instance)
            {
                instance->SetData(DATA_BLADE_LORD_TAYAK_EVENT, FAIL);
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
                instance->SetData(DATA_BLADE_LORD_TAYAK_EVENT, DONE);
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

        void TeleportPlayers() // Phase 2 player teleportation.
        {
            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                if (Player* playr = i->GetSource()) if (playr->IsAlive())
                    playr->TeleportTo(me->GetMapId(), PlayerTelePos[0], PlayerTelePos[1], PlayerTelePos[2], PlayerTelePos[3], TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (me->HealthBelowPct(21) && Phase == PHASE_NORMAL) // Storm Unleashed 20 - 10%.
            {
                // Set phase.
                Phase = PHASE_STORM_UNLEASHED;

                Talk(SAY_STORM_UNLEASHED);

                // Cancel the P1 events.
                events.CancelEvent(EVENT_TEMPEST_SLASH);
                events.CancelEvent(EVENT_UNSEEN_STRIKE);
                events.CancelEvent(EVENT_WIND_STEP);
                events.CancelEvent(EVENT_OVERWHELMING_ASS);
                if (me->GetMap()->IsHeroic()) events.CancelEvent(EVENT_BLADE_TEMPEST); // Heroic only.

                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->Clear();
                me->SetReactState(REACT_PASSIVE);

                // Teleport players and move to P2 first point.
                TeleportPlayers();
                me->NearTeleportTo(TayakStormPoint1[0], TayakStormPoint1[1], TayakStormPoint1[2], TayakStormPoint1[3]);

                events.ScheduleEvent(EVENT_STORM_UNLEASHED, 1000);
            }

            if (me->HealthBelowPct(11) && Phase == PHASE_STORM_UNLEASHED && !storm1Done) // Storm Unleashed 10 - 0%.
            {
                storm1Done = true;

                Talk(SAY_STORM_UNLEASHED);

                // Remove auras from first Storm Unleashed and cancel tornado summoning.
                me->RemoveAurasDueToSpell(SPELL_STORM_UNLEASHED_D);
                me->RemoveAurasDueToSpell(SPELL_SU_DMG_AURA);
                events.CancelEvent(EVENT_SUMMON_TORNADOES);

                // Teleport players and move to P2 second point.
                TeleportPlayers();
                me->NearTeleportTo(TayakStormPoint2[0], TayakStormPoint2[1], TayakStormPoint2[2], TayakStormPoint2[3]);

                events.ScheduleEvent(EVENT_STORM_UNLEASHED, 1000);
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_TEMPEST_SLASH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 50.0f, true))
                            tempestTarget = target;
                        DoCast(me, SPELL_TEMP_SLASH_SUMM_V);
                        events.ScheduleEvent(EVENT_TEMPEST_SLASH, urand(14500, 16500));
                        break;

                    case EVENT_UNSEEN_STRIKE:
                        unseenTank = me->GetVictim();
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                        {
                            Talk(ANN_UNSEEN, target->GetGUID());
                            DoCast(target, SPELL_UNSEEN_STRIKE_TR);
                            me->AddAura(SPELL_UNSEEN_STRIKE_MKR, target);
                            me->GetMotionMaster()->MovementExpired();
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveChase(target);
                            me->SetReactState(REACT_PASSIVE);
                        }
                        events.ScheduleEvent(EVENT_UNSEEN_STRIKE_RETURN, 5500);
                        events.ScheduleEvent(EVENT_UNSEEN_STRIKE, urand(53000, 61000));
                        break;

                    case EVENT_UNSEEN_STRIKE_RETURN:
                        if (unseenTank)
                        {
                            me->GetMotionMaster()->MovementExpired();
                            me->GetMotionMaster()->Clear();
                            me->SetReactState(REACT_AGGRESSIVE);
                            if (unseenTank->IsAlive()) AttackStart(unseenTank);
                            else DoZoneInCombat(me, 100.0f);

                            unseenTank = NULL;
                        }
                        break;

                    case EVENT_WIND_STEP:
                        currentTank = me->GetVictim(); // Store current victim to return to it afterwards.
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 50.0f, true))
                            DoCast(target, SPELL_WIND_STEP_TP);
                        events.ScheduleEvent(EVENT_WIND_STEP_RETURN, 1000);
                        events.ScheduleEvent(EVENT_WIND_STEP, urand(24000, 26000));
                        break;

                    case EVENT_WIND_STEP_RETURN: // Return to old target.
                        if (currentTank)
                        {
                            if (currentTank->IsAlive()) DoCast(currentTank, SPELL_WIND_STEP_TP_BACK);
                            currentTank = NULL;
                        }
                        break;

                    case EVENT_OVERWHELMING_ASS:
                        DoCastVictim(SPELL_OVERWHELMING_ASS); // On tank.
                        events.ScheduleEvent(EVENT_OVERWHELMING_ASS, urand(20500, 35500));
                        break;

                    case EVENT_BLADE_TEMPEST: // Heroic.
                        DoCast(me, SPELL_BLADE_TEMPES_J_FC); // Pull all players.
                        DoCast(me, SPELL_BLADE_TEMPEST_AUR);
                        events.ScheduleEvent(EVENT_BLADE_TEMPEST, 60000);
                        break;

                    case EVENT_STORM_UNLEASHED:
                        DoCast(me, SPELL_STORM_UNLEASHED_D);
                        me->AddAura(SPELL_SU_DMG_AURA, me); // Damage aura.
                        events.ScheduleEvent(EVENT_SUMMON_TORNADOES, 1000);
                        break;

                    case EVENT_SUMMON_TORNADOES:
                        // Summon the tornado and increase the counter.
                        me->SummonCreature(NPC_US_TORNADO, !storm1Done ? Tornado1[tornadoSummonCounter] : Tornado2[tornadoSummonCounter], TEMPSUMMON_TIMED_DESPAWN, 20000);
                        tornadoSummonCounter++;
                        // Reset it at 3, to start over.
                        if (tornadoSummonCounter >= 3)
                            tornadoSummonCounter = 0;
                        events.ScheduleEvent(EVENT_SUMMON_TORNADOES, 1000);
                        break;

                    case EVENT_BERSERK: // Nasty if you're here!
                        me->AddAura(SPELL_BERSERK, me);
                        break;

                    default: break;
                }
            }

            if (Phase == PHASE_NORMAL) // No melee in P2.
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_blade_lord_tayakAI(creature);
    }
};

// Heart of Fear - Trash Version Tempest Stalker (LTD): 64373.
class npc_tempest_slash_tornado : public CreatureScript
{
public:
	npc_tempest_slash_tornado() : CreatureScript("npc_tempest_slash_tornado") { }

	struct npc_tempest_slash_tornadoAI : public ScriptedAI
	{
		npc_tempest_slash_tornadoAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        Unit* moveTarget;

        void IsSummonedBy(Unit* summoner) OVERRIDE
        {
            events.Reset();
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);

            me->AddAura(SPELL_TEMP_SLASH_AURA, me); // Damage + Visual aura.
            me->DespawnOrUnsummon(120000);

            if (CAST_AI(boss_blade_lord_tayak::boss_blade_lord_tayakAI, summoner->ToCreature()->AI())->tempestTarget != NULL)
            {
                if (CAST_AI(boss_blade_lord_tayak::boss_blade_lord_tayakAI, summoner->ToCreature()->AI())->tempestTarget->IsAlive())
                {
                    if (moveTarget = CAST_AI(boss_blade_lord_tayak::boss_blade_lord_tayakAI, summoner->ToCreature()->AI())->tempestTarget)
                    {
                        me->GetMotionMaster()->MovePoint(POINT_TEMPEST_TARGET, moveTarget->GetPositionX(), moveTarget->GetPositionY(), moveTarget->GetPositionZ());
                        CAST_AI(boss_blade_lord_tayak::boss_blade_lord_tayakAI, summoner->ToCreature()->AI())->tempestTarget = NULL;
                        moveTarget = NULL;
                    }
                }
                else
                {
                    moveTarget = NULL;
                    me->DespawnOrUnsummon(100); // Nowhere to go, simply despawn.
                }
            }
            else
            {
                moveTarget = NULL;
                me->DespawnOrUnsummon(100); // Nowhere to go, simply despawn.
            }
        }

        void MovementInform(uint32 type, uint32 pointId) OVERRIDE
        {
            if (!me->IsAlive() || type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case POINT_TEMPEST_TARGET:
                    events.ScheduleEvent(EVENT_MOVE_RANDOM, 500);
                    break;

                default: break;
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_MOVE_RANDOM:
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveRandom(4.0f);
                        break;

                    default: break;
                }
            }
        }
	};

	CreatureAI* GetAI(Creature* creature) const OVERRIDE
	{
		return new npc_tempest_slash_tornadoAI (creature);
	}
};

// Heart of Fear - Armsmaster Ta'yak - Storm Unleashed West 1 Tornado (LTD): 63278.
class npc_storm_unleashed_tornado : public CreatureScript
{
public:
	npc_storm_unleashed_tornado() : CreatureScript("npc_storm_unleashed_tornado") { }

	struct npc_storm_unleashed_tornadoAI : public ScriptedAI
	{
		npc_storm_unleashed_tornadoAI(Creature* creature) : ScriptedAI(creature), vehicle(creature->GetVehicleKit())
        {
            ASSERT(vehicle);
        }

        Vehicle* vehicle;

        void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
        {
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);

            me->SetSpeed(MOVE_WALK, 1.1f, true);
            me->SetSpeed(MOVE_RUN, 1.1f, true);
            Movement::MoveSplineInit init(me);
            init.SetOrientationFixed(true);
            init.Launch();

            float x, y, z;
            me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 100.0f);
            me->GetMotionMaster()->MovePoint(1, x, y, z);

            me->AddAura(SPELL_SU_AURA, me); // Control vehicle aura.
            me->AddAura(SPELL_SU_DUMMY_VIS, me); // Visual aura.
        }
	};

	CreatureAI* GetAI(Creature* creature) const OVERRIDE
	{
		return new npc_storm_unleashed_tornadoAI (creature);
	}
};

// Intensify: 123470.
class spell_tayak_intensify_aura : public SpellScriptLoader
{
    public:
        spell_tayak_intensify_aura() : SpellScriptLoader("spell_tayak_intensify_aura") { }

        class spell_tayak_intensify_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_tayak_intensify_aura_AuraScript);

            // Modify periodic timers depending on boss phase (1 minute / 10 seconds).
            void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
            {
                isPeriodic = true;
                amplitude = CAST_AI(boss_blade_lord_tayak::boss_blade_lord_tayakAI, GetUnitOwner()->ToCreature()->AI())->Phase == PHASE_NORMAL ? 60 * IN_MILLISECONDS : 10 * IN_MILLISECONDS;
            }

            void Register() OVERRIDE
            {
                DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_tayak_intensify_aura_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_tayak_intensify_aura_AuraScript();
        }
};

// Wind Step: 123459.
class spell_tayak_wind_step: public SpellScriptLoader
{
    public:
        spell_tayak_wind_step() : SpellScriptLoader("spell_tayak_wind_step") { }

        class spell_tayak_wind_stepSpellScript: public SpellScript
        {
            PrepareSpellScript(spell_tayak_wind_stepSpellScript);

            bool Validate(SpellInfo const* spell) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(spell->Id))
                    return false;

                return true;
            }

            bool Load()
            {
                return true;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                if (!GetCaster() || !GetHitUnit()) return;

                GetCaster()->AddAura(SPELL_WIND_STEP_B_DMG, GetHitUnit());
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_tayak_wind_stepSpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_tayak_wind_stepSpellScript();
        }
};

// Storm Unleashed Ride Me: 124258.
class spell_tayak_storms_vehicle: public SpellScriptLoader
{
    public:
        spell_tayak_storms_vehicle() : SpellScriptLoader("spell_tayak_storms_vehicle") { }

        class spell_tayak_storms_vehicleSpellScript: public SpellScript
        {
            PrepareSpellScript(spell_tayak_storms_vehicleSpellScript);

            bool Validate(SpellInfo const* spell) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(spell->Id))
                    return false;

                return true;
            }

            bool Load()
            {
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                // Find the nearest player in 4 yards, and that will be the target.
                WorldObject* target = GetCaster()->ToCreature()->SelectNearestPlayer(4.0f);

                targets.clear();
                targets.push_back(target);
            }

            void EffectScriptEffect(SpellEffIndex effIndex)
            {
                if (!GetCaster() || !GetHitUnit()) return;

                GetHitUnit()->CastSpell(GetCaster(), SPELL_SU_RV, true); // Enter the vehicle.
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_tayak_storms_vehicleSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_tayak_storms_vehicleSpellScript::EffectScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_tayak_storms_vehicleSpellScript();
        }
};

// Unleashed Storm target check for damage spell.
class TargetCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit TargetCheck(Unit* _caster) : caster(_caster) { }

        bool operator()(WorldObject* object)
        {
            return object->GetTypeId() == TYPEID_PLAYER && !object->ToPlayer()->GetVehicleCreatureBase();
        }

    private:
        Unit* caster;
};

// Storm Unleashed: 124783.
class spell_tayak_storm_unleashed_dmg: public SpellScriptLoader
{
    public:
        spell_tayak_storm_unleashed_dmg() : SpellScriptLoader("spell_tayak_storm_unleashed_dmg") { }

        class spell_tayak_storm_unleashed_dmgSpellScript: public SpellScript
        {
            PrepareSpellScript(spell_tayak_storm_unleashed_dmgSpellScript);

            bool Validate(SpellInfo const* spell) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(spell->Id))
                    return false;

                return true;
            }

            bool Load()
            {
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                // Filter players not being inside a tornado.
                targets.remove_if(TargetCheck(GetCaster()));
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_tayak_storm_unleashed_dmgSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_tayak_storm_unleashed_dmgSpellScript();
        }
};

void AddSC_boss_blade_lord_tayak()
{
    new boss_blade_lord_tayak();
    new npc_tempest_slash_tornado();
    new npc_storm_unleashed_tornado();
    new spell_tayak_intensify_aura();
    new spell_tayak_wind_step();
    new spell_tayak_storms_vehicle();
    new spell_tayak_storm_unleashed_dmg();
}
