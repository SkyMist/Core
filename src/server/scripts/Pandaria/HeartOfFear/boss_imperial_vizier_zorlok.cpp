/*Copyright (C) 2014 Buli.
*
* !NOTES: 
*    1) - Attenuation ring spiral is made, on off., by an invisible Zor'lok spawned at the location of the boss, and using 4 diagonal summon spells + others (total about 16 spells).
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
#include "Unit.h"
#include "Player.h"
#include "Weather.h"

#include "heart_of_fear.h"

enum Yells
{
    // Vizier Zor'lok.
    SAY_INTRO               = 0,  // They are but the waves crashing upon the mountain of Her divine will. They may rise again and again; but will accomplish nothing. (- On Wave 1).

    SAY_WAVE_2_KILLED       = 1,  // The Divine challenges us to face these intruders.
    SAY_WAVE_3_KILLED       = 2,  // And so it falls to us, Her chosen voice.

    SAY_AGGRO               = 3,  // The Divine chose us to give mortal voice to Her divine will. We are but the vessel that enacts Her will.
    SAY_SLAY                = 4,  // 0 - Ours is but to serve; yours is but to die. ; 1 - The Divine will not suffer your taint here, outsider.
    SAY_DEATH               = 5,  // We will not give in to the despair of the dark void. If Her will for us is to perish, then it shall be so.

    SAY_INHALE              = 6,  // Through the air we breathe, She strengthens our voice.
    SAY_SONG_OF_EMPRESS     = 7,  // Behold the voice of Her divine wrath.
    SAY_CONVERT             = 8,  // Her will is greater than us all. You will join us or you will perish.
    SAY_PHASE2              = 9,  // We are as unwavering as the amber that shapes our halls. With Her might we will vanquish all who dare intrude upon Her divine home.

    SAY_SPLIT               = 10, // We are unfazed. We will stand firm.

    ANN_PLATFORM            = 11, // Imperial Vizier Zor'lok flies to one of his platforms!
    ANN_INHALE              = 12, // Imperial Vizier Zor'lok [Inhales] a large breath!
    ANN_CONVERT             = 13, // Imperial Vizier Zor'lok uses his voice to [Convert] members of the raid to fight on his side!
    ANN_FORCE_AND_VERVE     = 14, // Imperial Vizier Zor'lok shouts with [Force and Verve]!
    ANN_PHASE2              = 15  // Imperial Vizier Zor'lok inhales the Pheromones of Zeal!
};

enum Spells
{
    // Vizier Zor'lok.

    // Phase 1 - Changes platforms each 20% health, 20 x 3 = 60%. All three platforms are passed once.

    // On Engage.
    SPELL_PHEROMONES        = 123811, // Create Areatrigger.

    // General - Casted on all three platforms.
    SPELL_INHALE            = 122852,
    SPELL_EXHALE            = 122761, // Triggers 122760 damage spell. Players between him and the target "catch" the beam.

    // Left Platform - Attenuation.
    SPELL_ATTENUATION       = 127834,
    SPELL_ATTENUATING       = 122970, // Vehicle control spell + aura.
    SPELL_ATT_SUMMON_RING   = 122375, // Summons 62703 Sonic Ring NPC. All other Ring summon spells are not needed.

    // Right Platform - Force and Verve.
    SPELL_NOISE_CANCELLING  = 122707, // Missile triggering 122731 for zone creation.
    SPELL_FORCE_AND_VERVE   = 122713, // Triggers 122718 damage spell.

    // Middle Platform - Convert.
    SPELL_CONVERT           = 122740, // Spell casted on the Converted players.

    // Phase 2 - From 40 % health, flies to the middle of the room, the platforms get doors and are closed.
    SPELL_INHALE_PHEROMONES = 124018, // P2 transition spell, triggers 123833 (buff).

    SPELL_SONG_OF_EMPRESS   = 130133, // Triggers 123790 damage spell.

    SPELL_ECHOING           = 127545, // Echo summon spell, dummy. Heroic on platform change, and Phase 2.

    SPELL_BERSERK           = 120207, // Enrage.

    // Echo of Attenuation / of Force and Verve.
    SPELL_SONG_EMPRESS_ECHO = 127551, // Removal proc? "Echo of Zorlok" thing appears only here. Not of any particular usage.
    SPELL_CLEAR_THROAT      = 122933, // SPELL_AURA_PERIODIC_DUMMY. 4 secs before Force and Verve.

    // Sonic Rings / Pulse
    SPELL_SONIC_RING_AURA   = 122334, // Triggers 122336 damage spell + visual.
    SPELL_SONIC_PULSE_AURA  = 124668, // Triggers 124673 damage spell + visual.

    // Noise Cancelling Areatrigger
    SPELL_NOISE_CANC_DMG_R  = 122706, // Dummy effects for damage reduction.

    // Pheromones of Zeal Areatrigger
    SPELL_PHEROMONES_DMG    = 123812  // Damage and Silence aura.
};

enum Events
{
    // Vizier Zor'lok.
    EVENT_INHALE            = 1,
    EVENT_EXHALE,

    EVENT_ATTENUATION,     // 23s after reaching left platform.  Every 32.5-41s. 11s after Exhale.
    EVENT_NOISE_CANCELLING,
    EVENT_FORCE_AND_VERVE, // 16s after reaching right platform. Every 35-50s.
    EVENT_CONVERT,         // 22.5s after reaching mid platform. Every 33-50s.

    EVENT_SPLIT,

    EVENT_CHANGE_PLATFORM,
    EVENT_MOVE_PLATFORM,
    EVENT_SONG_OF_THE_EMPRESS,

    EVENT_REACHED_NEW_PLATFORM,

    EVENT_MOVE_PHASE_2,
    EVENT_PHASE_2,

    EVENT_BERSERK,

    // Sonic Rings / Discs.
    EVENT_CHANGE_ORIENTATION,
    EVENT_MOVE_SPIRAL,

    // Echo of Force And Verve.
    EVENT_CLEAR_THROAT
};

enum Actions
{
    ACTION_RESCH_ATTENUATE  = 1 // On Exhale.
};

enum Creatures
{
    // Heroic Echo spawns.
    NPC_ECHO_ATTENUATION    = 70767,
    NPC_ECHO_FORCE_VERVE    = 65174,
    NPC_SONIC_RING          = 62703,
    NPC_SONIC_PULSE         = 63835
};

enum MovePoints
{
    // Vizier Zor'lok.
    POINT_MID_UP            = 1, // In the middle, floating.

    POINT_PLATFORM1,             // Platform 1, mid point, ground.
    POINT_PLATFORM1_ARRIVE,      // Platform 1, floating,  arrive.

    POINT_PLATFORM2,             // Platform 2, mid point, ground.
    POINT_PLATFORM2_ARRIVE,      // Platform 2, floating,  arrive.

    POINT_PLATFORM3,             // Platform 3, mid point, ground.
    POINT_PLATFORM3_ARRIVE,      // Platform 3, floating,  arrive.

    POINT_PLATFORM_LEAVE         // Any Platform, floating,  leave.
};

enum Platforms
{
    // Vizier Zor'lok.
    PLATFORM_NONE              = 0,

    PLATFORM_RIGHT,
    PLATFORM_MID,
    PLATFORM_LEFT
};

// Each 20% HP changes platform, 3 platforms. 40% -> Boss goes to middle, and phase 2.
Position const PlatformPoint1     = {-2315.945f, 300.597f, 409.9f}; // Right, 5.403f Orientation.
Position const PlatformPoint2     = {-2313.428f, 220.207f, 409.9f}; // Mid,   0.785f Orientation.
Position const PlatformPoint3     = {-2238.010f, 222.054f, 409.9f}; // Left,  2.391f Orientation.

Position const MidPoint           = {-2276.036f, 258.319f, 414.027f}; // Orientation: 0.801f.

float platformOrientations[4] =
{
    0.801f, 5.403f, 0.785f, 2.391f
};

// Imperial Vizier Zor'lok: 62980.
class boss_imperial_vizier_zorlok : public CreatureScript
{
public:
    boss_imperial_vizier_zorlok() : CreatureScript("boss_imperial_vizier_zorlok") { }

    struct boss_imperial_vizier_zorlokAI : public BossAI
    {
        boss_imperial_vizier_zorlokAI(Creature* creature) : BossAI(creature, DATA_VIZIER_ZORLOK_EVENT), summons(me)
        {
            instance = creature->GetInstanceScript();
            introDone = false;
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;
        Unit* ExhaleTarget;
        bool introDone, phaseChanged, rightPlatformVisited, midPlatformVisited, leftPlatformVisited, reachedNewPlatform;
        uint32 currentPlatform, platformsVisited;

        /*** Regular AI calls. ***/

        void InitializeAI() OVERRIDE
        {
            if (!me->isDead())
                Reset();
        }

        void Reset() OVERRIDE
        {
            events.Reset();
            summons.DespawnAll();

            currentPlatform = PLATFORM_NONE;
            platformsVisited = 0;

            phaseChanged = false;

            rightPlatformVisited = false;
            midPlatformVisited = false;
            leftPlatformVisited = false;

            reachedNewPlatform = false;

            ExhaleTarget = NULL;

            if (instance)
                instance->SetData(DATA_VIZIER_ZORLOK_EVENT, NOT_STARTED);

            // Set flight and move to mid.
            SetFlight();
            me->GetMotionMaster()->MovePoint(POINT_MID_UP, MidPoint);

            _Reset();
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE
        {
            if (!introDone && me->IsWithinDistInMap(who, 50) && who->GetTypeId() == TYPEID_PLAYER)
            {
                Talk(SAY_INTRO);
                introDone = true;
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            DoCast(me, SPELL_PHEROMONES); // Create Areatrigger (TARGET_DEST_DB);

            events.ScheduleEvent(EVENT_BERSERK, 11 * MINUTE * IN_MILLISECONDS); // 11 minute berserk.

            ChangePlatform(); // Move to first, random platform.
            platformsVisited++; // Increase visited platforms count.

            if (instance)
            {
                instance->SetData(DATA_VIZIER_ZORLOK_EVENT, IN_PROGRESS);
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
            me->RemoveAreaTrigger(SPELL_PHEROMONES);
            if (instance)
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PHEROMONES_DMG);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CONVERT);
            }

            me->RemoveAllAuras();
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
            {
                instance->SetData(DATA_VIZIER_ZORLOK_EVENT, FAIL);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            _EnterEvadeMode();
        }

		void JustReachedHome() OVERRIDE
		{
            if (phaseChanged) // Going into Phase 2.
            {
                SetLand();
			    events.ScheduleEvent(EVENT_PHASE_2, 1500);
            }
		}

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();

            if (instance)
            {
                instance->SetData(DATA_VIZIER_ZORLOK_EVENT, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CONVERT);
            }

            _JustDied();
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
            summons.Summon(summon);
            summon->setActive(true);

			if (me->IsInCombat())
            {
                if (summon->GetEntry() == NPC_ECHO_ATTENUATION || summon->GetEntry() == NPC_ECHO_FORCE_VERVE)
                    summon->AI()->DoZoneInCombat();
                else // Discs, Pulses etc.
                    summon->SetInCombatWithZone();
            }
        }

        void DoAction(int32 action) OVERRIDE
        {
            switch (action)
            {
                case ACTION_RESCH_ATTENUATE:
                    events.CancelEvent(EVENT_ATTENUATION);
                    events.ScheduleEvent(EVENT_ATTENUATION, 11000);
                    break;

                default: break;
            }
        }

        void MovementInform(uint32 type, uint32 pointId) OVERRIDE
        {
            if (!me->IsAlive() || type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case POINT_MID_UP:
                    if (!phaseChanged || !me->IsInCombat()) // P1 / Reset.
                        me->SetFacingTo(platformOrientations[currentPlatform]);
                    else // P2.
                        events.ScheduleEvent(EVENT_MOVE_PHASE_2, 100);
                    break;

                case POINT_PLATFORM_LEAVE:
                    events.ScheduleEvent(EVENT_CHANGE_PLATFORM, 100);
                    break;

                case POINT_PLATFORM1_ARRIVE:
                case POINT_PLATFORM2_ARRIVE:
                case POINT_PLATFORM3_ARRIVE:
                    events.ScheduleEvent(EVENT_MOVE_PLATFORM, 100);
                    break;

                case POINT_PLATFORM1:
                case POINT_PLATFORM2:
                case POINT_PLATFORM3:
                    events.ScheduleEvent(EVENT_REACHED_NEW_PLATFORM, 100);
                    break;

                default: break;
            }
        }

        void DamageTaken(Unit* who, uint32& damage) OVERRIDE
        {
            // Platform change handling.
            if (me->HealthBelowPctDamaged(81, damage) && platformsVisited == 1 || me->HealthBelowPctDamaged(61, damage) && platformsVisited == 2)
            {
                ChangePlatform(); // Change the platform.
                platformsVisited++; // Increase visited platforms count.
            }

            // Phase change handling.
            if (me->HealthBelowPctDamaged(41, damage) && platformsVisited == 3 && !phaseChanged)
            {
                Talk(SAY_PHASE2);
                SetFlight();
                me->GetMotionMaster()->MovePoint(POINT_PLATFORM_LEAVE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 6.0f);
                phaseChanged = true;
            }

            // Song of The Empress + Platform combat handling.
            if (me->GetReactState() == REACT_DEFENSIVE && reachedNewPlatform)
            {
                if (who && who->IsWithinDistInMap(me, MELEE_RANGE)) // Engaged in melee combat!
                {
                    events.CancelEvent(EVENT_SONG_OF_THE_EMPRESS);
                    me->RemoveAurasDueToSpell(SPELL_SONG_OF_EMPRESS);
                    me->SetReactState(REACT_AGGRESSIVE);

                    // Time to schedule the events for each platform.
                    switch(currentPlatform)
                    {
                        case PLATFORM_RIGHT:
                            events.ScheduleEvent(EVENT_NOISE_CANCELLING, 15000);
                            break;
                        case PLATFORM_MID:
                            events.ScheduleEvent(EVENT_CONVERT, 22500);
                            break;
                        case PLATFORM_LEFT:
                            events.ScheduleEvent(EVENT_ATTENUATION, 23000);
                            break;

                        default: break;
                    }

                    events.ScheduleEvent(EVENT_INHALE, 15000);
                    reachedNewPlatform = false;
                }
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            // Remove Convert aura from players at / below 50% HP.
            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
            if (!PlayerList.isEmpty())
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    if (Player* playr = i->GetSource())
                        if (playr->HasAura(SPELL_CONVERT) && playr->HealthBelowPct(51))
                            playr->RemoveAurasDueToSpell(SPELL_CONVERT);

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_INHALE:
                        Talk(SAY_INHALE);
                        Talk(ANN_INHALE);
                        DoCast(me, SPELL_INHALE);
                        events.ScheduleEvent(EVENT_INHALE, urand(32000, 34000));
                        events.ScheduleEvent(EVENT_EXHALE, 3000);
                        break;

                    case EVENT_EXHALE:
                        ExhaleTarget = NULL;
                        // Set the target first.
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true, -SPELL_CONVERT))
                        {
                            ExhaleTarget = target;
                            DoCast(target, SPELL_EXHALE);
                        }
                        break;

                    case EVENT_ATTENUATION:
                        DoCast(me, SPELL_ATTENUATION);
                        events.ScheduleEvent(EVENT_ATTENUATION, urand(32500, 41000));
                        break;

                    case EVENT_NOISE_CANCELLING:
                    {
                        uint32 neededTargets = 0;
                        Difficulty difficulty = me->GetMap()->GetDifficulty();
                        switch(difficulty)
                        {
                            case RAID_DIFFICULTY_10MAN_NORMAL:
                                neededTargets = 2;
                                break;
                            case RAID_DIFFICULTY_25MAN_NORMAL:
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                                neededTargets = 3;
                                break;
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                neededTargets = 4;
                                break;
                        }

                        // Create the Noise Cancelling zones at the position of the two farthest players on the platform.
                        std::list<Unit*> playerList;
                        SelectTargetList(playerList, neededTargets, SELECT_TARGET_FARTHEST, 20.0f, true);
                        if (!playerList.empty())
                            for (std::list<Unit*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                                if (Unit* target = (*itr))
                                    DoCast(target, SPELL_NOISE_CANCELLING);

                        events.ScheduleEvent(EVENT_FORCE_AND_VERVE, 1000);
                        events.ScheduleEvent(EVENT_NOISE_CANCELLING, urand(35000, 45000));
                    }
                        break;

                    case EVENT_FORCE_AND_VERVE:
                        Talk(ANN_FORCE_AND_VERVE);
                        DoCast(me, SPELL_FORCE_AND_VERVE);
                        break;

                    case EVENT_CONVERT:
                    {
                        // Don't use this if there's a single player attacking Zor'lok, so as to not evade.
                        uint32 numb = 0;
                        Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                        if (!PlayerList.isEmpty())
                            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                if (Player* player = i->GetSource())
                                    if (!player->HasAura(SPELL_CONVERT))
                                        ++numb;

                        if (numb > 1)
                        {
                            Talk(SAY_CONVERT);
                            Talk(ANN_CONVERT);
                            DoCast(me, SPELL_CONVERT);
                            events.ScheduleEvent(EVENT_CONVERT, urand(33000, 43000));
                        }
                    }
                        break;

                    case EVENT_SPLIT:
                        Talk(SAY_SPLIT);
                        // DoCast(me, SPELL_ECHOING); - Not good, need a random spawn here.
                        if (me->GetMap()->IsHeroic())
                            me->SummonCreature(RAND(NPC_ECHO_ATTENUATION, NPC_ECHO_FORCE_VERVE), -2276.036f, 258.319f, 409.9f, 0.801f, TEMPSUMMON_MANUAL_DESPAWN);
                        events.ScheduleEvent(EVENT_SPLIT, urand(75000, 85000));
                        break;

                    case EVENT_CHANGE_PLATFORM:
                        if (!phaseChanged) // P1.
                        {
                            if (currentPlatform == PLATFORM_RIGHT)
                                me->GetMotionMaster()->MovePoint(POINT_PLATFORM1_ARRIVE, -2315.945f, 300.597f, 415.9f);
                            else if (currentPlatform == PLATFORM_MID)
                                me->GetMotionMaster()->MovePoint(POINT_PLATFORM2_ARRIVE, -2313.428f, 220.207f, 415.9f);
                            else // if (currentPlatform == PLATFORM_LEFT)
                                me->GetMotionMaster()->MovePoint(POINT_PLATFORM3_ARRIVE, -2238.010f, 222.054f, 415.9f);
                        }
                        else // P2
                            me->GetMotionMaster()->MovePoint(POINT_MID_UP, MidPoint);
                        break;

                    case EVENT_MOVE_PLATFORM:
                        if (currentPlatform == PLATFORM_RIGHT)
                            me->GetMotionMaster()->MovePoint(POINT_PLATFORM1, PlatformPoint1);
                        else if (currentPlatform == PLATFORM_MID)
                            me->GetMotionMaster()->MovePoint(POINT_PLATFORM2, PlatformPoint2);
                        else // if (currentPlatform == PLATFORM_LEFT)
                            me->GetMotionMaster()->MovePoint(POINT_PLATFORM3, PlatformPoint3);
                        break;

                    case EVENT_REACHED_NEW_PLATFORM:
                        me->SetFacingTo(platformOrientations[currentPlatform]);
                        SetLand();
                        events.ScheduleEvent(EVENT_SONG_OF_THE_EMPRESS, 10000);
                        reachedNewPlatform = true;
                        break;

                    case EVENT_SONG_OF_THE_EMPRESS:
                    {
                        Player* nearPlayer = me->FindNearestPlayer(100.0f);
                        if (!nearPlayer || !nearPlayer->IsWithinDistInMap(me, MELEE_RANGE))
                        {
                            Talk(SAY_SONG_OF_EMPRESS);
                            DoCast(me, SPELL_SONG_OF_EMPRESS);
                        }
                    }
                        break;

                    case EVENT_MOVE_PHASE_2:
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveTargetedHome();
                        break;

                    case EVENT_PHASE_2:
                        Talk(ANN_PHASE2);
                        DoCast(me, SPELL_INHALE_PHEROMONES);
                        me->RemoveAreaTrigger(SPELL_PHEROMONES);
                        if (instance)
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PHEROMONES_DMG);

                        events.ScheduleEvent(EVENT_INHALE, 5000);
                        events.ScheduleEvent(EVENT_ATTENUATION, 54000);
                        events.ScheduleEvent(EVENT_NOISE_CANCELLING, 27000);
                        events.ScheduleEvent(EVENT_CONVERT, 14000);
                        events.ScheduleEvent(EVENT_SPLIT, 22000);
                        break;

                    case EVENT_BERSERK: // Nasty if you're here!
                        DoCast(me, SPELL_BERSERK);
                        break;

                    default: break;
                }
            }

            DoMeleeAttackIfReady();
        }

        /*** Particular AI functions. ***/

        void SetFlight()
        {
            // me->GetMotionMaster()->MovementExpired();
            // me->GetMotionMaster()->Clear();
            me->SetReactState(REACT_PASSIVE);
            me->HandleEmote(EMOTE_ONESHOT_LIFTOFF);
            me->SetDisableGravity(true);
            me->SetByteFlag(UNIT_FIELD_ANIM_TIER, 3, UNIT_BYTE1_FLAG_HOVER);
            me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
        }

        void SetLand()
        {
            me->HandleEmote(EMOTE_ONESHOT_LAND);
            me->SetDisableGravity(false);
            me->RemoveByteFlag(UNIT_FIELD_ANIM_TIER, 3, UNIT_BYTE1_FLAG_HOVER);
            me->RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
            // me->GetMotionMaster()->MovementExpired();
            // me->GetMotionMaster()->Clear();
            me->SetReactState(REACT_DEFENSIVE);
        }

        void ChangePlatform()
        {
            Talk(ANN_PLATFORM);

            events.CancelEvent(EVENT_INHALE);
            events.CancelEvent(EVENT_EXHALE);
            events.CancelEvent(EVENT_ATTENUATION);
            events.CancelEvent(EVENT_NOISE_CANCELLING);
            events.CancelEvent(EVENT_FORCE_AND_VERVE);
            events.CancelEvent(EVENT_CONVERT);
            events.CancelEvent(EVENT_SPLIT);

            // Select the next platform to move to.
            switch(currentPlatform)
            {
                case PLATFORM_NONE:
                    currentPlatform = RAND(PLATFORM_RIGHT, PLATFORM_MID, PLATFORM_LEFT);
                    events.ScheduleEvent(EVENT_CHANGE_PLATFORM, 100);
                    return; // First platform in selection, move through event directly.

                case PLATFORM_RIGHT:
                    // Summon the Echo on Heroic.
                    if (me->GetMap()->IsHeroic())
                        me->SummonCreature(NPC_ECHO_FORCE_VERVE, -2315.945f, 300.597f, 409.9f, 5.403f, TEMPSUMMON_MANUAL_DESPAWN);
                    // Set the platform as visited.
                    rightPlatformVisited = true;
                    // Select another platform.
                    if (leftPlatformVisited)
                        currentPlatform = PLATFORM_MID;
                    else if (midPlatformVisited)
                        currentPlatform = PLATFORM_LEFT;
                    else
                        currentPlatform = RAND(PLATFORM_MID, PLATFORM_LEFT);
                    break;

                case PLATFORM_MID:
                    // Set the platform as visited.
                    midPlatformVisited = true;
                    // Select another platform.
                    if (rightPlatformVisited)
                        currentPlatform = PLATFORM_LEFT;
                    else if (leftPlatformVisited)
                        currentPlatform = PLATFORM_RIGHT;
                    else
                        currentPlatform = RAND(PLATFORM_RIGHT, PLATFORM_LEFT);
                    break;

                case PLATFORM_LEFT:
                    // Summon the Echo on Heroic.
                    if (me->GetMap()->IsHeroic())
                        me->SummonCreature(NPC_ECHO_ATTENUATION, -2238.010f, 222.054f, 409.9f, 2.391f, TEMPSUMMON_MANUAL_DESPAWN);
                    // Set the platform as visited.
                    leftPlatformVisited = true;
                    // Select another platform.
                    if (rightPlatformVisited)
                        currentPlatform = PLATFORM_MID;
                    else if (midPlatformVisited)
                        currentPlatform = PLATFORM_RIGHT;
                    else
                        currentPlatform = RAND(PLATFORM_RIGHT, PLATFORM_MID);
                    break;

                default: break;
            }

            // Leave current platform.
            SetFlight();
            me->GetMotionMaster()->MovePoint(POINT_PLATFORM_LEAVE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 6.0f);
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_imperial_vizier_zorlokAI(creature);
    }
};

// Echo of Attenuation: 70767.
class npc_echo_of_attenuation : public CreatureScript
{
    public:
        npc_echo_of_attenuation() : CreatureScript("npc_echo_of_attenuation") { }

        struct npc_echo_of_attenuationAI : public ScriptedAI
        {
            npc_echo_of_attenuationAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void IsSummonedBy(Unit* summoner) OVERRIDE
            {
                events.Reset();
                events.ScheduleEvent(EVENT_ATTENUATION, 32000);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ATTENUATION:
                            DoCast(me, SPELL_ATTENUATION);
                            events.ScheduleEvent(EVENT_ATTENUATION, urand(48000, 54000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_echo_of_attenuationAI(creature);
        }
};

// Echo of Attenuation: 65174.
class npc_echo_of_force_and_verve : public CreatureScript
{
    public:
        npc_echo_of_force_and_verve() : CreatureScript("npc_echo_of_force_and_verve") { }

        struct npc_echo_of_force_and_verveAI : public ScriptedAI
        {
            npc_echo_of_force_and_verveAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void IsSummonedBy(Unit* summoner) OVERRIDE
            {
                events.Reset();
                events.ScheduleEvent(EVENT_CLEAR_THROAT, 23000);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CLEAR_THROAT:
                            DoCast(me, SPELL_CLEAR_THROAT);
                            events.ScheduleEvent(EVENT_NOISE_CANCELLING, 4100);
                            break;

                        case EVENT_NOISE_CANCELLING:
                        {
                            // Create the two Noise Cancelling zones at the position of the two farthest players on the platform.
                            std::list<Unit*> playerList;
                            SelectTargetList(playerList, 2, SELECT_TARGET_FARTHEST, 20.0f,true);
                            if (!playerList.empty())
                                for (std::list<Unit*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                                    if (Unit* target = (*itr))
                                        DoCast(target, SPELL_NOISE_CANCELLING);
                        
                            events.ScheduleEvent(EVENT_FORCE_AND_VERVE, 1000);
                        }
                            break;

                        case EVENT_FORCE_AND_VERVE:
                            DoCast(me, SPELL_FORCE_AND_VERVE);
                            events.ScheduleEvent(EVENT_CLEAR_THROAT, urand(43000, 49000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_echo_of_force_and_verveAI(creature);
        }
};

// Sonic Ring: 62703.
class npc_zorlok_sonic_ring : public CreatureScript
{
public:
    npc_zorlok_sonic_ring() : CreatureScript("npc_zorlok_sonic_ring") { }

    struct npc_zorlok_sonic_ringAI : public ScriptedAI
    {
        npc_zorlok_sonic_ringAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        uint32 movePointReached;

        void IsSummonedBy(Unit* summoner) OVERRIDE
        {
            Reset();
        }

        void Reset() OVERRIDE 
        {
            events.Reset();
            movePointReached = 1;

            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(SPELL_SONIC_RING_AURA, me);
            me->SetSpeed(MOVE_WALK, 0.9f);
            me->SetSpeed(MOVE_RUN, 0.9f);

            events.ScheduleEvent(EVENT_MOVE_SPIRAL, 10);
        }

        void MovementInform(uint32 type, uint32 pointId) OVERRIDE
        {
            if (!me->IsAlive() || type != POINT_MOTION_TYPE)
                return;

            events.ScheduleEvent(EVENT_CHANGE_ORIENTATION, 10);
            events.ScheduleEvent(EVENT_MOVE_SPIRAL, 20);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHANGE_ORIENTATION:
                    {
                        // Check next orientation to move to in order to create a "spiral" motion feeling. If it's lower then 0, it should reset to 6.
                        float nextOrientation = me->GetOrientation() - 0.5f >= 0.0f ? me->GetOrientation() - 0.5f : 6.0f;
                        me->SetFacingTo(nextOrientation);
                        break;
                    }

                    case EVENT_MOVE_SPIRAL:
                    {
                        float x, y, z;
                        me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 3.0f);
                        me->GetMotionMaster()->MovePoint(movePointReached, x, y, z);
                        movePointReached++;
                    }

                    default: break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_zorlok_sonic_ringAI (creature);
    }
};

// Sonic Pulse: 63835.
class npc_zorlok_sonic_pulse : public CreatureScript
{
public:
    npc_zorlok_sonic_pulse() : CreatureScript("npc_zorlok_sonic_pulse") { }

    struct npc_zorlok_sonic_pulseAI : public ScriptedAI
    {
        npc_zorlok_sonic_pulseAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        uint32 movePointReached;

        void IsSummonedBy(Unit* summoner) OVERRIDE
        {
            Reset();
        }

        void Reset() OVERRIDE 
        {
            events.Reset();
            movePointReached = 1;

            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(SPELL_SONIC_PULSE_AURA, me);
            me->SetSpeed(MOVE_WALK, 0.9f);
            me->SetSpeed(MOVE_RUN, 0.9f);

            events.ScheduleEvent(EVENT_MOVE_SPIRAL, 10);
        }

        void MovementInform(uint32 type, uint32 pointId) OVERRIDE
        {
            if (!me->IsAlive() || type != POINT_MOTION_TYPE)
                return;

            events.ScheduleEvent(EVENT_CHANGE_ORIENTATION, 10);
            events.ScheduleEvent(EVENT_MOVE_SPIRAL, 20);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHANGE_ORIENTATION:
                    {
                        // Check next orientation to move to in order to create a "spiral" motion feeling. If it's lower then 0, it should reset to 6.
                        float nextOrientation = me->GetOrientation() - 0.5f >= 0.0f ? me->GetOrientation() - 0.5f : 6.0f;
                        me->SetFacingTo(nextOrientation);
                        break;
                    }

                    case EVENT_MOVE_SPIRAL:
                    {
                        float x, y, z;
                        me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 3.0f);
                        me->GetMotionMaster()->MovePoint(movePointReached, x, y, z);
                        movePointReached++;
                    }

                    default: break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_zorlok_sonic_pulseAI (creature);
    }
};

// Exhale: 122761
class spell_zorlok_exhale : public SpellScriptLoader
{
    public:
        spell_zorlok_exhale() : SpellScriptLoader("spell_zorlok_exhale") { }

        // First handle the cast.
        class spell_zorlok_exhale_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_zorlok_exhale_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (!GetCaster())
                    return;

                // Clear the targets.
                if (!targets.empty())
                    targets.clear();

                // When first cast, this spell selects as target just the Exhale target of the boss (for the aura apply and the stun).
                if (GetCaster()->ToCreature() && GetCaster()->ToCreature()->AI() && GetCaster()->ToCreature()->GetEntry() == BOSS_GRAND_VIZIER_ZORLOK)
                {
                    Unit* exhaleTarget = CAST_AI(boss_imperial_vizier_zorlok::boss_imperial_vizier_zorlokAI, GetCaster()->ToCreature()->AI())->ExhaleTarget;
                    if (exhaleTarget)
                        targets.push_back(target);
                }
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster())
                    return;

                // Check Inhale stacks - If boss has at least 3 stacks, remove the aura.
                if (GetCaster()->ToCreature()->GetEntry() == BOSS_GRAND_VIZIER_ZORLOK)
                    if (GetCaster()->HasAura(SPELL_INHALE))
                        if (Aura* Inhale = GetCaster()->GetAura(SPELL_INHALE))
                            if (Inhale->GetStackAmount() >= 3)
                                GetCaster()->RemoveAurasDueToSpell(SPELL_INHALE);
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_exhale_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_exhale_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_zorlok_exhale_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        // Then handle the aura and periodic ticks.
        class spell_zorlok_exhale_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_zorlok_exhale_AuraScript)

            bool Load() OVERRIDE
            {
                foundNewExhaleVictim = false;
                newExhaleVictim = NULL;
                return true;
            }

            void PeriodicTick(AuraEffect const* /*aurEff*/)
            {
                if (!GetCaster())
                    return;

                if (GetCaster()->ToCreature() && GetCaster()->ToCreature()->AI() && GetCaster()->ToCreature()->GetEntry() == BOSS_GRAND_VIZIER_ZORLOK)
                {
                    PreventDefaultAction();

                    Unit* exhaleTarget = CAST_AI(boss_imperial_vizier_zorlok::boss_imperial_vizier_zorlokAI, GetCaster()->ToCreature()->AI())->ExhaleTarget;

                    // Check for existing target.
                    if (exhaleTarget)
                    {
                        if (!foundNewExhaleVictim)
                        {
                            Map::PlayerList const& playerList = GetCaster()->GetMap()->GetPlayers();
                            if (!playerList.isEmpty())
                            {
                                for (Map::PlayerList::const_iterator c_iter = playerList.begin(); c_iter != playerList.end(); ++c_iter)
                                {
                                    if (Player* player = c_iter->GetSource())
                                    {
                                        // Check for players between boss and target.
                                        if (player->IsInBetween(GetCaster(), exhaleTarget, 1.0f))
                                        {
                                            // Found a new player to damage, so remove old player stun.
                                            if (exhaleTarget->ToPlayer()->HasAura(SPELL_EXHALE))
                                                exhaleTarget->ToPlayer()->RemoveAurasDueToSpell(SPELL_EXHALE);
                            
                                            // New channel victim found, so change to it and break the loop, we have what we need.
                                            newExhaleVictim = player->ToUnit();
                                            foundNewExhaleVictim = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (foundNewExhaleVictim)
                            GetCaster()->CastSpell(newExhaleVictim, GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
                        else
                            GetCaster()->CastSpell(exhaleTarget, GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
                    }
                }

                bool foundNewExhaleVictim;
                Unit* newExhaleVictim;
            }

            void Register() OVERRIDE
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_zorlok_exhale_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_zorlok_exhale_SpellScript();
        }

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_zorlok_exhale_AuraScript();
        }
};

// Exhale (damage): 122760
class spell_zorlok_exhale_damage : public SpellScriptLoader
{
    public:
        spell_zorlok_exhale_damage() : SpellScriptLoader("spell_zorlok_exhale_damage") { }

        class spell_zorlok_exhale_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_zorlok_exhale_damage_SpellScript);

            // void FilterTargets(std::list<WorldObject*>& targets)
            // {
            //     if (!GetCaster() || targets.empty())
            //         return;
            // }

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster() || !GetHitUnit())
                    return;

                // Add Inhale stacks amount to damage done.
                if (GetCaster()->ToCreature()->GetEntry() == BOSS_GRAND_VIZIER_ZORLOK)
                    if (GetCaster()->HasAura(SPELL_INHALE))
                        if (Aura* Inhale = GetCaster()->GetAura(SPELL_INHALE))
                            SetHitDamage(int32(GetHitDamage() + (GetHitDamage()  * (0.5 * Inhale->GetStackAmount()))));
            }

            void Register() OVERRIDE
            {
                // OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_exhale_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_129);
                OnEffectHitTarget += SpellEffectFn(spell_zorlok_exhale_damage_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_zorlok_exhale_damage_SpellScript();
        }
};

// Attenuation: 127834 (Needs some further testing, the spiral may require a bit of fixing).
class spell_zorlok_attenuation : public SpellScriptLoader
{
    public:
        spell_zorlok_attenuation() : SpellScriptLoader("spell_zorlok_attenuation") { }

        class spell_zorlok_attenuation_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_zorlok_attenuation_AuraScript);

            void HandlePeriodic(AuraEffect const* aurEff)
            {
                if (!GetCaster())
                    return;

                // Create the spiral for the sonic rings.
                if (Unit* caster = GetCaster())
                {
                    // We calculate Y variations depending on the tick number.
                    uint32 tickNumber = aurEff->GetTickNumber();

                    float X;
                    float Y;
                    float Z;
                    float FinalY;
                    float degree = 0.0f;
                    float width = 0.0f;

                    // Calculate the summon points in the spiral. Seems like 8 rings are summoned each tick.
                    for (uint8 i = 0; i < 8; i++)
                    {
                        degree += 0.18f * (tickNumber > 1 ? tickNumber : tickNumber / 2); // Modify the Y position a little, so the outcome is a spiral.
                        width += 0.785f;
                        if (degree >= 6.28f) degree -= 6.28f;

                        X = caster->GetPositionX();
                        Y = sin(degree) * width;
                        FinalY = caster->GetPositionY() + Y;
                        Z = caster->GetMap()->GetHeight(X, FinalY, caster->GetPositionZ());

                        caster->SummonCreature(NPC_SONIC_RING, X, FinalY, Z, degree, TEMPSUMMON_TIMED_DESPAWN, 20000);
                    }

                    // Summon also 4 Sonic Pulses on Heroic, which go straighter a bit.
                    if (caster->GetMap()->IsHeroic())
                        for (uint8 i = 0; i < 4; i++)
                            caster->SummonCreature(NPC_SONIC_PULSE, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), 0.0f + (1.5f * i), TEMPSUMMON_TIMED_DESPAWN, 20000);
                }
            }

            void Register() OVERRIDE
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_zorlok_attenuation_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_zorlok_attenuation_AuraScript();
        }
};

// Force and Verve: 122718.
class spell_zorlok_force_and_verve: public SpellScriptLoader
{
    public:
        spell_zorlok_force_and_verve() : SpellScriptLoader("spell_zorlok_force_and_verve") { }

    private:
        class spell_zorlok_force_and_verve_SpellScript: public SpellScript
        {
            PrepareSpellScript(spell_zorlok_force_and_verve_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                if (!GetHitUnit())
                    return;

                if (GetHitUnit()->HasAura(SPELL_NOISE_CANC_DMG_R))
                    SetHitDamage(int32(GetHitDamage() * 0.4)); // 60% reduction.
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_zorlok_force_and_verve_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_zorlok_force_and_verve_SpellScript();
        }
};

class ConvertCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit ConvertCheck(Unit* _caster) : caster(_caster) { }

        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() != TYPEID_PLAYER)
                return true;

            if (object->ToPlayer()->HasAura(SPELL_CONVERT))
                return true;

            return false;
        }

    private:
        Unit* caster;
};

// Convert: 122740
class spell_zorlok_convert : public SpellScriptLoader
{
    public:
        spell_zorlok_convert() : SpellScriptLoader("spell_zorlok_convert") { }

        class spell_zorlok_convert_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_zorlok_convert_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (!GetCaster() || targets.empty())
                    return;

                targets.remove_if(ConvertCheck(GetCaster()));

                if (targets.empty())
                    return;

                uint32 neededTargets = 0;

                Difficulty difficulty = GetCaster()->GetMap()->GetDifficulty();
                switch(difficulty)
                {
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                        neededTargets = 2;
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        neededTargets = 5;
                        break;
                    case RAID_DIFFICULTY_10MAN_HEROIC:
                        neededTargets = 3;
                        break;
                    case RAID_DIFFICULTY_25MAN_HEROIC:
                        neededTargets = 6;
                        break;
                }

                if (targets.size() > neededTargets)
                    targets.resize(neededTargets);
                else if (targets.size() <= neededTargets && targets.size() > 1)
                    targets.resize(neededTargets - 1);
                else return; // This should never get called.
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_convert_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_convert_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_convert_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_convert_SpellScript::FilterTargets, EFFECT_3, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zorlok_convert_SpellScript::FilterTargets, EFFECT_4, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_zorlok_convert_SpellScript();
        }
};

void AddSC_boss_imperial_vizier_zorlok()
{
    new boss_imperial_vizier_zorlok();
    new npc_echo_of_attenuation();
    new npc_echo_of_force_and_verve();
    new npc_zorlok_sonic_ring();
    new npc_zorlok_sonic_pulse();
    new spell_zorlok_exhale();
    new spell_zorlok_exhale_damage();
    new spell_zorlok_attenuation();
    new spell_zorlok_force_and_verve();
    new spell_zorlok_convert();
}
