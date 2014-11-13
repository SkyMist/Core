/*
*
* FUCK CREDITS! (SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass).
*
* Raid: Siege of Orgrimmar.
* Boss: The Fallen Protectors.
*
* Wowpedia boss history:
*
* "The Golden Lotus and Shado-Pan guardians of the Vale of Eternal Blossoms were caught in the epicenter of the devastating blast that scarred the Vale, and torn apart by the dark energies.
*  Their spirits linger in the place they once protected, confused and tormented by their failure."
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

#include "siege_of_orgrimmar.h"

enum Yells
{
    /*** Bosses ***/

    // Lorewalker Cho Intro / Outro.

    LOREWALKER_SAY_INTRO_1         = 0, // Can it be...? Oh no, no no no no no.
    LOREWALKER_SAY_INTRO_2,             // The Golden Lotus... they pledged their lives to defend this place.                                                    -- After this : ROOK_SAY_INTRO_1.
    LOREWALKER_SAY_INTRO_3,             // Yes! Rook Stonetoe! You remember me! What has happened to you?                                                        -- After this : ROOK_SAY_INTRO_2 and SUN_SAY_INTRO_1.
    LOREWALKER_SAY_INTRO_4,             // I see. This sha energy has trapped their spirits here, to endlessly relive their failure. It feeds on their despair.  -- After this : ROOK_SAY_INTRO_3.
    LOREWALKER_SAY_INTRO_5,             // This is a fate far worse than death - please, heroes! Set their souls free!                                           -- After this : Intro Done!

    LOREWALKER_SAY_OUTRO_1,             // Be at peace, dear friends.
    LOREWALKER_SAY_OUTRO_2,             // May your souls become one with the land you gave your life to protect..

    // Rook Stonetoe

    ROOK_SAY_INTRO_1               = 0, // You... Rook knows you...
    ROOK_SAY_INTRO_2,                   // Rook... does not know... Head... is cloudy...
    ROOK_SAY_INTRO_3,                   // Please... help...

    ROOK_SAY_AGGRO,                     // Sun... He... aid me.
    ROOK_SAY_CORRUPTION_KICK,           // 0 - Brawl... with Rook. ; 1 - Rook fight!
    ROOK_SAY_DESPERATE_MEASURES,        // Rook... needs help!
    ROOK_SAY_KILL,                      // You... are not my friend.
    ROOK_SAY_BOND_LOTUS,                // Rook... not safe...
    ROOK_SAY_EVENT_COMPLETE,            // Feeling... better now... Thank you... little friend.

    ROOK_ANNOUNCE_MEASURES,             // Rook Stonetoe calls upon [Misery, Sorrow and Gloom]!
    ROOK_ANNOUNCE_BOND_LOTUS,           // Rook Stonetoe begins to cast [Bond of the Golden Lotus]!

    // He Softfoot

    HE_SAY_AGGRO                   = 0, // He Softfoot nods to his foes.
    HE_SAY_DESPERATE_MEASURES,          // He Softfoot signals for help.
    HE_SAY_BOND_LOTUS,                  // He Softfoot enters a meditative pose to heal his wounds.

    HE_ANNOUNCE_GOUGE,                  // He Softfoot attempts to [Gouge] you, turn away!
    HE_ANNOUNCE_MARK_OF_ANGUISH,        // He Softfoot creates a [Mark of Anguish]!
    HE_ANNOUNCE_NOXIOUS_POISON,         // He Softfoot coats his daggers with [Noxious Poison]!
    HE_ANNOUNCE_INSTANT_POISON,         // He Softfoot coats his daggers with [Instant Poison]!
    HE_ANNOUNCE_BOND_LOTUS,             // He Softfoot begins to cast [Bond of the Golden Lotus]!

    // Sun Tenderheart

    SUN_SAY_INTRO_1                = 0, // We know only despair now. A fitting punishment for our failure...

    SUN_SAY_AGGRO,                      // Will you succeed where we have failed?
    SUN_SAY_CALAMITY,                   // 0 - We must redeem the Vale! ; 1 - You will suffer from our failure.
    SUN_SAY_DESPERATE_MEASURES,         // Protectors, come to my aid!
    SUN_SAY_KILL,                       // You too have failed.
    SUN_SAY_WIPE,                       // We will never find solace.
    SUN_SAY_BOND_LOTUS,                 // We have not fulfilled our oath!
    SUN_SAY_EVENT_COMPLETE,             // We return to when we lost our way. You... are very kind.

    SUN_ANNOUNCE_CALAMITY,              // Sun Tenderheart begins to cast [Calamity]!
    SUN_ANNOUNCE_BOND_LOTUS             // Sun Tenderheart begins to cast [Bond of the Golden Lotus]!
};

enum Spells
{
    /*** Bosses ***/

    // General

    /*
        Bond of the Golden Lotus
        Rook Stonetoe, He Softfoot, and Sun Tenderheart share a spiritual bond, linking their fates to each other. 
        Upon reaching 1 health, each of them will begin to cast [Bond of the Golden Lotus] if any of the other Protectors are still in combat, restoring 20% of maximum health upon completion.
        Bringing all three Protectors to 1 health simultaneously will restore clarity to their clouded minds. 
    */
    SPELL_BOND_OF_THE_GOLDEN_LOTUS = 143497,

    SPELL_BERSERK                  = 144369,

    //============================================================================================================================================================================//

    // Rook Stonetoe

    /*
        Vengeful Strikes
        Rook Stonetoe enters an offensive stance, stunning his target and inflicting 180000 physical damage in a frontal cone twice per second for 3 sec.
    */
    SPELL_VENGEFUL_STRIKES         = 144396, // Stun, Damage, trigger 144397 each 0.5s.

    /*
        Corrupted brew
        Rook Stonetoe hurls a keg of Corrupted Brew at a distant target, inflicting 118750 to 131250 Shadow damage to enemies within 5 yards and slowing targets hit by 50% for 15 sec.
    */
    SPELL_CORRUPTED_BREW           = 143019, // Dummy on efect 0 to cast SPELL_CORRUPTED_BREW_MISSILE (1, 3, 2, 6 targets).
    SPELL_CORRUPTED_BREW_MISSILE   = 143021, // Triggers SPELL_CORRUPTED_BREW_DAMAGE.
    SPELL_CORRUPTED_BREW_DAMAGE    = 143023, // Damage and movement speed reduce in 5 yards.

    /*
        Clash
        Rook Stonetoe clashes against an enemy, charging each other and meeting halfway. Rook Stonetoe then immediately begins to perform Corruption Kick.
        In Heroic Difficulty, Rook Stonetoe's Clash will be cast during Desperate Measures phase too.
    */
    SPELL_CLASH                    = 143027, // Dummy on Effect 0 to cast SPELL_CLASH_ROOK + SPELL_CLASH_TARGET.
    SPELL_CLASH_ROOK               = 143030, // Boss charge - to - player.
    SPELL_CLASH_TARGET             = 143028, // Player charge - to - boss.

    /*
        Corruption Kick
        Spins rapidly in a circle, rooting himself and inflicting 117000 to 123000 Physical damage to targets within 10 yards every second for 4 sec. 
        Additionally torments targets struck, inflicting 75000 Shadow damage every two seconds for 6 sec. 
    */
    SPELL_CORRUPTION_KICK          = 143007, // Immediately after Clash. Triggers damage and aura apply.

    /*
        Desperate Measures
        Upon reaching 66% or 33% health remaining, Rook Stonetoe manifests evil forms of his closest friends, making Embodied Misery, Embodied Sorrow, and Embodied Gloom.
        Once all three spirits are defeated, Rook Stonetoe resumes fighting players.
    */

    SPELL_MISSERY_SORROW_GLOOM     = 143955, // Dummy aura, "Meditating" tooltip.

    //============================================================================================================================================================================//

    // He Softfoot

    /*
        Garrote
        Traveling through the shadows, He Softfoot Garrotes targets, inflicting 80000 Physical damage every 2 sec.
        This effect is removed when He Softfoot begins his Desperate Measures. 
    */
    SPELL_SHADOWSTEP               = 143048, // Triggers SPELL_HE_GARROTE and Teleport 2y behind target.
    SPELL_HE_GARROTE               = 143198, // Damage every 2 seconds.

    /*
        Gouge
        He Softfoot attempts to gouge the eyes of his target, incapacitating them for 6 sec and fixating on a random target until Taunted.
        If the target is facing away, Gouge will instead knock them away a short distance. 
    */
    SPELL_HE_GOUGE                 = 143330, // Dummy of Effect 0. Checks for SPELL_HE_GOUGE_DMG_STUN / SPELL_HE_GOUGE_KB.
    SPELL_HE_GOUGE_DMG_STUN        = 143301, // Damage and 6 seconds stun.
    SPELL_HE_GOUGE_KB              = 143331, // Knockback.

    /*
        Master Poisoner
        He Softfoot is a Master Poisoner and occasionally coats his weapons with various types of poisons.
        In Heroic Difficulty, Master Poisoner abilities also take effect when using Garrote.
        In Normal Difficulty, He Softfoot's Master Poisoner abilities will not be cast during any Desperate Measures phase.
    */

    /*
        Instant Poison
        He Softfoot coats his weapon in an Instant Poison, causing successful melee attacks to inflict 73125 to 76875 additional Nature damage. 
    */
    SPELL_INSTANT_POISON           = 143210, // Applies Proc aura on melee atack for Instant Poison damage (Effect 1) and Script Effect for removal for SPELL_NOXIOUS_POISON on Effect 2.

    /*
        Noxious Poison
        He Softfoot coats his weapon in a Noxious Poison, causing successful melee attacks to create pools of poison on the ground, inflicting 85000 Nature damage every second. These pools may be jumped over.
        This effect is removed when He Softfoot begins his Desperate Measures. 
    */
    SPELL_NOXIOUS_POISON           = 143225, // Applies Proc aura on melee atack for SPELL_NOXIOUS_POISON_MISSILE (Effect 1) and Script Effect for removal for SPELL_INSTANT_POISON on Effect 2.
    SPELL_NOXIOUS_POISON_MISSILE   = 143245, // Triggers SPELL_NOXIOUS_POISON_AT_MISS.
    SPELL_NOXIOUS_POISON_AT_MISS   = 143276, // Triggers SPELL_NOXIOUS_POISON_AT.
    SPELL_NOXIOUS_POISON_AT        = 143235, // Creates Areatrigger 1013 (Pool of Noxious Poison).
    SPELL_NOXIOUS_POISON_AURA      = 143239, // Player periodic damage trigger aura (From Areatrigger).

    /*
        Desperate Measures
        Upon reaching 66% or 33% health remaining, He Softfoot manifests a twisted spirit of his brother, creating Embodied Anguish, which fixates upon the raid member who has the Mark of Anguish.
        Once this spirit is defeated, He Softfoot resumes fighting players.
    */

    SPELL_MARK_OF_ANGUISH          = 143822, // Dummy on Effect 0 for SPELL_MARK_OF_ANGUISH_MAIN aura apply on random target.
    SPELL_MARK_OF_ANGUISH_MAIN     = 143840, // Mechanic abilities, Periodic Dummy on Effect 1 for SPELL_MARK_OF_ANGUISH_DAMAGE, Root apply.
    SPELL_MARK_OF_ANGUISH_DAMAGE   = 144365, // Damage.
    SPELL_MARK_OF_ANGUISH_TRANSFER = 143842, // Mechanic Abilities button, Dummy on Effect 0 for SPELL_MARK_OF_ANGUISH_MAIN aura apply on selected friendly target.
    SPELL_MARK_OF_ANGUISH_VISUAL   = 143812, // Kneel boss visual, "Meditating" tooltip.
    SPELL_DEBILITATION             = 147383, // Possessing the Mark of Anguish debilitates targets, decreasing their armor by 50% for 2 min.

    //============================================================================================================================================================================//

    // Sun Tenderheart

    /*
        Sha Sear
        Causes an explosion of Shadow magic around the target, inflicting increasing Shadow damage every second to all enemies within 5 yards around the target.
    */
    SPELL_SHA_SHEAR                = 143423, // Triggers SPELL_SHA_SHEAR_DAMAGE each sec.
    SPELL_SHA_SHEAR_DAMAGE         = 143424, // Damage.

    /*
        Shadow Word: Bane
        Expels a word of misery upon multiple targets, inflicting 100000 Shadow damage every 3 sec for 18 sec. Each time this effect deals damage, it will jump to an additional target, up to a total of 3 times.
    */
    SPELL_SHADOW_WORD_BANE         = 143446, // Cast time and Dumy on Effect 0 for targets apply of SPELL_SHADOW_WORD_BANE_HIT (2, 5, 4, 10 targets).
    SPELL_SHADOW_WORD_BANE_HIT     = 143434, // Dummy for Jump on Effect 0, Periodic Damage aura apply (Effect 1).
    SPELL_SHADOW_WORD_BANE_JUMP    = 143443, // Jump spell between players on tick. Dummy on Effect 0 to apply SPELL_SHADOW_WORD_BANE_HIT.

    /*
        Calamity
        Calls forth a great Calamity, striking all players for 30% of their maximum health as Shadow damage. This also removes Shadow Word: Bane from all targets.
        In Heroic difficulty, Calamity increases in magnitude by an additional 10% of maximum health each additional time it is cast. This effect resets when Sun Tenderheart begins her Desperate Measures.
        In Normal Difficulty, Sun Tenderheart's Calamity will not be cast during any Desperate Measures phase.
    */
    SPELL_CALAMITY                 = 143491, // Cast time and Dummy on Effect 0 for SPELL_CALAMITY_DAMAGE.
    SPELL_CALAMITY_DAMAGE          = 143493, // Damage health % (Effect 0) and Dummy (Effect 1) for 10% Heroic increase.

    /*
        Desperate Measures
        Upon reaching 66% or 33% health remaining, Sun Tenderheart manifests an evil form of her protectors, making Embodied Despair and Embodied Desperation.
        Once both spirits are defeated, Sun Tenderheart resumes fighting players.
    */

    SPELL_DARK_MEDITATION          = 143546, // Creates Areatrigger 1032 (Dark Meditation) Effect 0, Periodic Dummy (Effect 1) for SPELL_DARK_MEDITATION_DAMAGE trigger every 0.5 seconds.
    SPELL_DARK_MEDITATION_DAMAGE   = 143559, // Damage.
    SPELL_DARK_MEDITATION_VISUAL   = 143843, // Visual area and Dummy Effect 0 (Unk).
    SPELL_DARK_MEDITATION_DMG_RED  = 143564, // Damage reduction aura on players inside the field.

    // ! On HEROIC difficulty, Sun Tenderheart periodically fires a bolt of dark energy !
    SPELL_MEDITATION_SPIKE_MISSILE = 143599, // Triggers SPELL_MEDITATION_SPIKE_DAMAGE.
    SPELL_MEDITATION_SPIKE_DAMAGE  = 143602, // Damage.

    /*** Adds (Desperate Measures) ***/

    // Embodied Misery
    SPELL_DEFILED_GROUND           = 143961, // Damage, Knockback and trigger SPELL_DEFILED_GROUND_AT (Effect 1).
    SPELL_DEFILED_GROUND_AT        = 143960, // Creates Areatrigger 1053 (Defiled Ground).
    SPELL_DEFILED_GROUND_AURA      = 143959, // Triggers Periodic Damage (Player aura).

    // Embodied Sorrow
    SPELL_INFERNO_STRIKE           = 143962, // Dummy for damage increase on Effect 0, Damage on effect 1. Damage split between targets in 8 yards.

    // Embodied Gloom 
    SPELL_CORRUPTION_SHOCK         = 143958, // Dummy on Effect 0 for SPELL_CORRUPTION_SHOCK_MISSILE (2, 5, 4, 10 targets).
    SPELL_CORRUPTION_SHOCK_MISSILE = 144020, // Triggers SPELL_CORRUPTION_SHOCK_DAMAGE at location.
    SPELL_CORRUPTION_SHOCK_DAMAGE  = 144018, // Damage.

    // ! On HEROIC difficulty, Corruption Chain replaces Corruption Shock !
    SPELL_CORRUPTION_CHAIN         = 145653, // Cast time and dummy on Effect 0 for SPELL_CORRUPTION_CHAIN_DAMAGE.
    SPELL_CORRUPTION_CHAIN_DAMAGE  = 145631, // Damage and chain.

    // All three above.
    SPELL_SHARED_TORMENT           = 145655, // All three NPC's share health.

    // Embodied Anguish
    SPELL_SHADOW_WEAKNESS_AURA     = 144079, // Proc aura for SPELL_SHADOW_WEAKNESS.
    SPELL_SHADOW_WEAKNESS          = 144176,
    SPELL_SHADOW_WEAKNESS_TRANSFER = 144081, // Dummy on Effect 0 to apply a stack of SPELL_SHADOW_WEAKNESS on all players when using SPELL_MARK_OF_ANGUISH_TRANSFER.

    // Manifest Emotions – Embodied Despair and Embodied Desperation focus their negative emotions, creating Sha manifestations which attack players. 
    // Any damage taken by these manifested emotions will also be suffered by the creature that spawned them.

    // Embodied Despair
    SPELL_MANIFEST_DESPAIR         = 143746, // Summons NPC_DESPAIR_SPAWN.

    // Embodied Desperation
    SPELL_MANIFEST_DESPERATION     = 144504  // Summons NPC_DESPERATION_SPAWN.
};

enum Events
{
    /*** Bosses ***/

    // Rook Stonetoe
    EVENT_ROOK_VENGEFUL_STRIKES    = 1,      //  7 seconds after combat start. Every 21 seconds.
    EVENT_ROOK_CORRUPTED_BREW,               // 18 seconds after combat start. Every 17-25 seconds.
    EVENT_ROOK_CLASH,                        // 45 seconds after combat start. Every 46 seconds.
    EVENT_ROOK_CORRUPTION_KICK,              // After Clash.
    EVENT_ROOK_DESPERATE_MEASURES,

    // He Softfoot
    EVENT_HE_GARROTE,                        // 15 seconds after combat start. Every 30-46 (Heroic 20-26) seconds.
    EVENT_HE_GOUGE,                          // 23 seconds after combat start. Every 30-41 seconds.
    EVENT_HE_POISON_DAGGERS,
    EVENT_HE_DESPERATE_MEASURES,

    // Sun Tenderheart
    EVENT_SUN_SHA_SHEAR,
    EVENT_SUN_SHADOW_WORD_BANE,              // 15 seconds after combat start. Every 17-25 (Heroic 13-20) seconds.
    EVENT_SUN_CALAMITY,                      // 31 seconds after combat start. Every 40-50 seconds.
    EVENT_SUN_DESPERATE_MEASURES,
    EVENT_SUN_MEDITATION_SPIKE,              // HEROIC only!

    // General
    EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS,
    EVENT_BERSERK,                           // 900 seconds Normal. 600 seconds Heroic.

    /*** Adds (Desperate Measures) ***/

    // Embodied Misery
    EVENT_DEFILED_GROUND,                    // Every 10.5 seconds.

    // Embodied Sorrow
    EVENT_INFERNO_STRIKE,                    // Every 9.5 seconds.

    // Embodied Gloom 
    EVENT_CORRUPTION_SHOCK_CHAIN,            // Shock -> Chain HEROIC!

    // Embodied Despair
    EVENT_MANIFEST_DESPAIR,

    // Embodied Desperation
    EVENT_MANIFEST_DESPERATION
};

enum Actions
{
    ACTION_EVADE_COMBAT           = 1,
    ACTION_EVENT_COMPLETE
};

enum Npcs
{
    /*** Bosses ***/

    // Rook Stonetoe
    NPC_EMBODIED_MISERY            = 71476,
    NPC_EMBODIED_SORROW            = 71481,
    NPC_EMBODIED_GLOOM             = 71477,

    // He Softfoot
    NPC_EMBODIED_ANGUISH           = 71478,

    // Sun Tenderheart
    NPC_EMBODIED_DESPAIR           = 71474,
    NPC_EMBODIED_DESPERATION       = 71482,

    /*** Adds (Desperate Measures) ***/

    // Embodied Despair
    NPC_DESPAIR_SPAWN              = 71712,

    // Embodied Desperation
    NPC_DESPERATION_SPAWN          = 71993
};

/*** Bosses ***/

// Rook Stonetoe 71475 - Acts as controller!
class boss_rook_stonetoe : public CreatureScript
{
    public:
        boss_rook_stonetoe() : CreatureScript("boss_rook_stonetoe") { }

        struct boss_rook_stonetoeAI : public BossAI
        {
            boss_rook_stonetoeAI(Creature* creature) : BossAI(creature, DATA_FALLEN_PROTECTORS_EVENT), summons(me)
            {
                instance  = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            Unit* clashTarget;
            bool doneDesperateMeasuresPhase, doneDesperateMeasuresPhase2;
            bool lotusScheduled, eventComplete;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                clashTarget = NULL;
                doneDesperateMeasuresPhase  = false;
                doneDesperateMeasuresPhase2 = false;

                lotusScheduled = false;
                eventComplete  = false;

                if (instance)
                    instance->SetData(DATA_FALLEN_PROTECTORS_EVENT, NOT_STARTED);

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                Talk(ROOK_SAY_AGGRO);

				events.ScheduleEvent(EVENT_ROOK_VENGEFUL_STRIKES, 7000);
				events.ScheduleEvent(EVENT_ROOK_CORRUPTED_BREW, 18000);
				events.ScheduleEvent(EVENT_ROOK_CLASH, 45000);

				events.ScheduleEvent(EVENT_BERSERK, (!IsHeroic() ? 15 : 10) * MINUTE * IN_MILLISECONDS);

                if (instance)
                    instance->SetData(DATA_FALLEN_PROTECTORS_EVENT, IN_PROGRESS);

                _EnterCombat();
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (uiDamage >= me->GetHealth())
                {
                    if (!lotusScheduled || lotusScheduled && !eventComplete)
                        uiDamage = 0;

                    if (!lotusScheduled)
                    {
                        Talk(ROOK_SAY_BOND_LOTUS);
                        Talk(ROOK_ANNOUNCE_BOND_LOTUS);
                        me->SetHealth(1);
                        events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 1000);
                        lotusScheduled = true;
                    }
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(ROOK_SAY_KILL);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_EVADE_COMBAT:
                        // Avoid bad calls / loops.
                        if (me->HasUnitState(UNIT_STATE_EVADE))
                            return;
                        EnterEvadeMode();
                        break;

                    default: break;
                }
            };

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                if (Creature* heSoftfoot = me->FindNearestCreature(BOSS_HE_SOFTFOOT, 300.0f, true))
                    heSoftfoot->AI()->DoAction(ACTION_EVADE_COMBAT);
                if (Creature* sunTenderheart = me->FindNearestCreature(BOSS_SUN_TENDERHEART, 300.0f, true))
                    sunTenderheart->AI()->DoAction(ACTION_EVADE_COMBAT);

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SetData(DATA_FALLEN_PROTECTORS_EVENT, FAIL);

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);

                _JustReachedHome();
            }

            void JustDied(Unit* killer)
            {
                Talk(ROOK_SAY_EVENT_COMPLETE);
                summons.DespawnAll();

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->setFaction(35);
                me->SetFullHealth();

                eventComplete = true;

                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SetData(DATA_FALLEN_PROTECTORS_EVENT, DONE);

                _JustDied();
            }

            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || !CheckInRoom())
                    return;

                if (lotusScheduled && !eventComplete)
                {
                    if (Creature* heSoftfoot = me->FindNearestCreature(BOSS_HE_SOFTFOOT, 300.0f, true))
                    {
                        if (Creature* sunTenderheart = me->FindNearestCreature(BOSS_SUN_TENDERHEART, 300.0f, true))
                        {
                            if (heSoftfoot->GetHealth() == 1 && sunTenderheart->GetHealth() == 1)
                            {
                                heSoftfoot->AI()->DoAction(ACTION_EVENT_COMPLETE);
                                sunTenderheart->AI()->DoAction(ACTION_EVENT_COMPLETE);
                                JustDied(NULL);
                                eventComplete = true;
                            }
                        }
                    }
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                // Schedule Desperate Measures phase entrance.
                if (me->HealthBelowPct(67) && !doneDesperateMeasuresPhase ||  me->HealthBelowPct(34) && !doneDesperateMeasuresPhase2)
                {
                    events.ScheduleEvent(EVENT_ROOK_DESPERATE_MEASURES, 500);
                    if (!doneDesperateMeasuresPhase)
                        doneDesperateMeasuresPhase = true;
                    else
                        doneDesperateMeasuresPhase2 = true;
                }

                // Schedule Desperate Measures phase exit.
                if ((doneDesperateMeasuresPhase || doneDesperateMeasuresPhase2) && 
                !me->FindNearestCreature(NPC_EMBODIED_MISERY, 300.0f, true) && !me->FindNearestCreature(NPC_EMBODIED_SORROW, 300.0f, true) && !me->FindNearestCreature(NPC_EMBODIED_GLOOM, 300.0f, true))
                {
                    me->RemoveAurasDueToSpell(SPELL_MISSERY_SORROW_GLOOM);

                    // On Normal difficulty Clash needs reschedule.
                    if (!me->GetMap()->IsHeroic())
                        events.ScheduleEvent(EVENT_ROOK_CLASH, 15000);
                }

                // Schedule Corruption Kick.
                if (clashTarget)
                    if (me->GetDistance(clashTarget) <= 3.0f)
                        events.ScheduleEvent(EVENT_ROOK_CORRUPTION_KICK, 1500);

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ROOK_VENGEFUL_STRIKES:
                            DoCast(me->getVictim(), SPELL_VENGEFUL_STRIKES);
				            events.ScheduleEvent(EVENT_ROOK_VENGEFUL_STRIKES, 21000);
                            break;

                        case EVENT_ROOK_CORRUPTED_BREW:
                            DoCast(me, SPELL_CORRUPTED_BREW);
				            events.ScheduleEvent(EVENT_ROOK_CORRUPTED_BREW, urand(17000, 25000));
                            break;

                        case EVENT_ROOK_CLASH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 100.0f, true))
                            {
                                clashTarget = target;
                                DoCast(target, SPELL_CLASH);
                            }
				            events.ScheduleEvent(EVENT_ROOK_CLASH, 46000);
                            break;

                        case EVENT_ROOK_CORRUPTION_KICK:
                            Talk(ROOK_SAY_CORRUPTION_KICK);
                            clashTarget = NULL;
                            DoCast(me, SPELL_CORRUPTION_KICK);
                            break;

                        case EVENT_ROOK_DESPERATE_MEASURES:
                            Talk(ROOK_SAY_DESPERATE_MEASURES);
                            Talk(ROOK_ANNOUNCE_MEASURES);

                            // On Normal difficulty Clash is not cast during this phase.
                            if (!me->GetMap()->IsHeroic())
                                events.CancelEvent(EVENT_ROOK_CLASH);

                            DoCast(me, SPELL_MISSERY_SORROW_GLOOM);

                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                me->SummonCreature(NPC_EMBODIED_MISERY, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                me->SummonCreature(NPC_EMBODIED_SORROW, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            me->SummonCreature(NPC_EMBODIED_GLOOM, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            break;

                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK);
                            if (Creature* heSoftfoot = me->FindNearestCreature(BOSS_HE_SOFTFOOT, 300.0f, true))
                                heSoftfoot->CastSpell(heSoftfoot, SPELL_BERSERK, false);
                            if (Creature* sunTenderheart = me->FindNearestCreature(BOSS_SUN_TENDERHEART, 300.0f, true))
                                sunTenderheart->CastSpell(sunTenderheart, SPELL_BERSERK, false);
                            break;

                        case EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS:
                            if (me->HealthBelowPct(100) && !eventComplete)
                            {
                                Talk(ROOK_ANNOUNCE_BOND_LOTUS);
                                DoCast(me, SPELL_BOND_OF_THE_GOLDEN_LOTUS);
                                events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 16000);
                            }
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rook_stonetoeAI(creature);
        }
};

// He Softfoot 71479.
class boss_he_softfoot : public CreatureScript
{
    public:
        boss_he_softfoot() : CreatureScript("boss_he_softfoot") { }

        struct boss_he_softfootAI : public BossAI
        {
            boss_he_softfootAI(Creature* creature) : BossAI(creature, DATA_FALLEN_PROTECTORS_EVENT), summons(me)
            {
                instance  = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            bool doneDesperateMeasuresPhase, doneDesperateMeasuresPhase2;
            bool lotusScheduled, eventComplete;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                doneDesperateMeasuresPhase  = false;
                doneDesperateMeasuresPhase2 = false;

                lotusScheduled = false;
                eventComplete  = false;

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                Talk(HE_SAY_AGGRO);

				events.ScheduleEvent(EVENT_HE_GARROTE, 15000);
				events.ScheduleEvent(EVENT_HE_GOUGE, 23000);
				events.ScheduleEvent(EVENT_HE_POISON_DAGGERS, 45000);

                _EnterCombat();
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (uiDamage >= me->GetHealth())
                {
                    if (!lotusScheduled || lotusScheduled && !eventComplete)
                        uiDamage = 0;

                    if (!lotusScheduled)
                    {
                        Talk(HE_SAY_BOND_LOTUS);
                        Talk(HE_ANNOUNCE_BOND_LOTUS);
                        me->SetHealth(1);
                        events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 1000);
                        lotusScheduled = true;
                    }
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_EVADE_COMBAT:
                        // Avoid bad calls / loops.
                        if (me->HasUnitState(UNIT_STATE_EVADE))
                            return;
                        EnterEvadeMode();
                        break;

                    case ACTION_EVENT_COMPLETE:
                        JustDied(NULL);
                        break;

                    default: break;
                }
            };

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                if (Creature* rookStonetoe = me->FindNearestCreature(BOSS_ROOK_STONETOE, 300.0f, true))
                    rookStonetoe->AI()->DoAction(ACTION_EVADE_COMBAT);
                if (Creature* sunTenderheart = me->FindNearestCreature(BOSS_SUN_TENDERHEART, 300.0f, true))
                    sunTenderheart->AI()->DoAction(ACTION_EVADE_COMBAT);

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);

                _JustReachedHome();
            }

            void JustDied(Unit* killer)
            {
                summons.DespawnAll();

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->setFaction(35);
                me->SetFullHealth();

                eventComplete = true;

                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                _JustDied();
            }

            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->DespawnOrUnsummon();
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

                // Schedule Desperate Measures phase entrance.
                if (me->HealthBelowPct(67) && !doneDesperateMeasuresPhase ||  me->HealthBelowPct(34) && !doneDesperateMeasuresPhase2)
                {
                    events.ScheduleEvent(EVENT_HE_DESPERATE_MEASURES, 500);
                    if (!doneDesperateMeasuresPhase)
                        doneDesperateMeasuresPhase = true;
                    else
                        doneDesperateMeasuresPhase2 = true;
                }

                // Schedule Desperate Measures phase exit.
                if ((doneDesperateMeasuresPhase || doneDesperateMeasuresPhase2) && !me->FindNearestCreature(NPC_EMBODIED_ANGUISH, 300.0f, true))
                {
                    me->RemoveAurasDueToSpell(SPELL_MARK_OF_ANGUISH_VISUAL);

                    // On Normal difficulty needs reschedule.
                    if (!me->GetMap()->IsHeroic())
                        events.ScheduleEvent(EVENT_HE_POISON_DAGGERS, 15000);
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HE_GARROTE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 50.0f, true))
                                DoCast(target, SPELL_SHADOWSTEP);
				            events.ScheduleEvent(EVENT_HE_GARROTE, me->GetMap()->IsHeroic() ? urand(20000, 26000) : urand(30000, 46000));
                            break;

                        case EVENT_HE_GOUGE:
                            Talk(HE_ANNOUNCE_GOUGE, me->getVictim()->GetGUID());
                            DoCast(me->getVictim(), SPELL_HE_GOUGE);
				            events.ScheduleEvent(EVENT_HE_GOUGE, urand(30000, 41000));
                            break;

                        case EVENT_HE_POISON_DAGGERS:
                            if (!me->HasAura(SPELL_INSTANT_POISON) && !me->HasAura(SPELL_NOXIOUS_POISON)) // First cast, no poison selected yet, select random one.
                            {
                                if (uint32 poisonChoice = urand(0, 1))
                                {
                                    if (poisonChoice == 0)   // Instant Poison selected.
                                    {
                                        Talk(HE_ANNOUNCE_INSTANT_POISON);
                                        DoCast(me, SPELL_INSTANT_POISON);
                                    }
                                    else                     // Noxious Poison selected.
                                    {
                                        Talk(HE_ANNOUNCE_NOXIOUS_POISON);
                                        DoCast(me, SPELL_NOXIOUS_POISON);
                                    }
                                }
                            }
                            else if (me->HasAura(SPELL_INSTANT_POISON))                                   // Has Instant Poison, select Noxious Poison.
                            {
                                Talk(HE_ANNOUNCE_NOXIOUS_POISON);
                                DoCast(me, SPELL_NOXIOUS_POISON);
                            }
                            else                                                                          // Has Noxious Poison, select Instant Poison.
                            {
                                Talk(HE_ANNOUNCE_INSTANT_POISON);
                                DoCast(me, SPELL_INSTANT_POISON);
                            }
				            events.ScheduleEvent(EVENT_HE_POISON_DAGGERS, 46000);
                            break;

                        case EVENT_HE_DESPERATE_MEASURES:
                            Talk(HE_SAY_DESPERATE_MEASURES);
                            Talk(HE_ANNOUNCE_MARK_OF_ANGUISH);

                            // Remove Garrote effect from players and Noxious Poison trigger aura from boss.
                            me->RemoveAurasDueToSpell(SPELL_NOXIOUS_POISON);
                            if (instance)
                                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_HE_GARROTE);

                            // On Normal difficulty Master Poisoner abilities are not cast during this phase.
                            if (!me->GetMap()->IsHeroic())
                                events.CancelEvent(EVENT_HE_POISON_DAGGERS);

                            me->AddAura(SPELL_MARK_OF_ANGUISH_VISUAL, me);

                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            {
                                DoCast(target, SPELL_MARK_OF_ANGUISH);

                                if (Creature* anguish = me->SummonCreature(NPC_EMBODIED_ANGUISH, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000))
                                {
                                    anguish->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                                    anguish->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                                    anguish->AI()->AttackStart(target);
                                }
                            }
                            break;

                        case EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS:
                            if (me->HealthBelowPct(100) && !eventComplete)
                            {
                                Talk(HE_ANNOUNCE_BOND_LOTUS);
                                DoCast(me, SPELL_BOND_OF_THE_GOLDEN_LOTUS);
                                events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 16000);
                            }
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_he_softfootAI(creature);
        }
};

// Sun Tenderheart 71480.
class boss_sun_tenderheart : public CreatureScript
{
    public:
        boss_sun_tenderheart() : CreatureScript("boss_sun_tenderheart") { }

        struct boss_sun_tenderheartAI : public BossAI
        {
            boss_sun_tenderheartAI(Creature* creature) : BossAI(creature, DATA_FALLEN_PROTECTORS_EVENT), summons(me)
            {
                instance  = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            bool doneDesperateMeasuresPhase, doneDesperateMeasuresPhase2;
            bool lotusScheduled, eventComplete;

            void Reset()
            {
                events.Reset();

                doneDesperateMeasuresPhase  = false;
                doneDesperateMeasuresPhase2 = false;

                lotusScheduled = false;
                eventComplete  = false;

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                Talk(SUN_SAY_AGGRO);

				events.ScheduleEvent(EVENT_SUN_SHA_SHEAR, 2000);
				events.ScheduleEvent(EVENT_SUN_SHADOW_WORD_BANE, 15000);
				events.ScheduleEvent(EVENT_SUN_CALAMITY, 31000);

                _EnterCombat();
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (uiDamage >= me->GetHealth())
                {
                    if (!lotusScheduled || lotusScheduled && !eventComplete)
                        uiDamage = 0;

                    if (!lotusScheduled)
                    {
                        Talk(SUN_SAY_BOND_LOTUS);
                        Talk(SUN_ANNOUNCE_BOND_LOTUS);
                        me->SetHealth(1);
                        events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 1000);
                        lotusScheduled = true;
                    }
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SUN_SAY_KILL);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_EVADE_COMBAT:
                        // Avoid bad calls / loops.
                        if (me->HasUnitState(UNIT_STATE_EVADE))
                            return;
                        EnterEvadeMode();
                        break;

                    case ACTION_EVENT_COMPLETE:
                        JustDied(NULL);
                        break;

                    default: break;
                }
            };

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                if (Creature* rookStonetoe = me->FindNearestCreature(BOSS_ROOK_STONETOE, 300.0f, true))
                    rookStonetoe->AI()->DoAction(ACTION_EVADE_COMBAT);
                if (Creature* heSoftfoot = me->FindNearestCreature(BOSS_HE_SOFTFOOT, 300.0f, true))
                    heSoftfoot->AI()->DoAction(ACTION_EVADE_COMBAT);

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);

                _JustReachedHome();
            }

            void JustDied(Unit* killer)
            {
                Talk(SUN_SAY_EVENT_COMPLETE);
                summons.DespawnAll();

                me->RemoveAllAuras();
                me->RemoveAllAreasTrigger();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->setFaction(35);
                me->SetFullHealth();

                eventComplete = true;

                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                _JustDied();
            }

            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || !CheckInRoom() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    Talk(SUN_SAY_WIPE);
                    EnterEvadeMode();
                    return;
                }

                // Schedule Desperate Measures phase entrance.
                if (me->HealthBelowPct(67) && !doneDesperateMeasuresPhase ||  me->HealthBelowPct(34) && !doneDesperateMeasuresPhase2)
                {
                    events.ScheduleEvent(EVENT_SUN_DESPERATE_MEASURES, 500);
                    if (!doneDesperateMeasuresPhase)
                        doneDesperateMeasuresPhase = true;
                    else
                        doneDesperateMeasuresPhase2 = true;
                }

                // Schedule Desperate Measures phase exit.
                if ((doneDesperateMeasuresPhase || doneDesperateMeasuresPhase2) && !me->FindNearestCreature(NPC_EMBODIED_DESPAIR, 300.0f, true) && !me->FindNearestCreature(NPC_EMBODIED_DESPERATION, 300.0f, true))
                {
                    // On Normal difficulty Calamity needs reschedule.
                    if (!me->GetMap()->IsHeroic())
                        events.ScheduleEvent(EVENT_SUN_CALAMITY, 15000);

                    // On Heroic Meditation Spike is canceled.
                    if (me->GetMap()->IsHeroic())
				        events.CancelEvent(EVENT_SUN_MEDITATION_SPIKE);
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUN_SHA_SHEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SHA_SHEAR);
				            events.ScheduleEvent(EVENT_SUN_SHA_SHEAR, 3000);
                            break;

                        case EVENT_SUN_SHADOW_WORD_BANE:
                            DoCast(me, SPELL_SHADOW_WORD_BANE);
				            events.ScheduleEvent(EVENT_HE_GARROTE, me->GetMap()->IsHeroic() ? urand(13000, 20000) : urand(18000, 25000));
                            break;

                        case EVENT_SUN_CALAMITY:
                            Talk(SUN_SAY_CALAMITY);
                            Talk(SUN_ANNOUNCE_CALAMITY);

                            // Remove Shadow Word : Bane effect from players.
                            if (instance)
                                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHADOW_WORD_BANE_HIT);

                            DoCast(me, SPELL_CALAMITY);
				            events.ScheduleEvent(EVENT_SUN_CALAMITY, urand(40000, 50000));
                            break;

                        case EVENT_SUN_DESPERATE_MEASURES:
                            Talk(SUN_SAY_DESPERATE_MEASURES);

                            // On Normal difficulty Calamity is not cast during this phase.
                            if (!me->GetMap()->IsHeroic())
                                events.CancelEvent(EVENT_SUN_CALAMITY);

                            // On Heroic Meditation Spike is scheduled.
                            if (me->GetMap()->IsHeroic())
				                events.ScheduleEvent(EVENT_SUN_MEDITATION_SPIKE, urand(4000, 6000));

                            me->AddAura(SPELL_DARK_MEDITATION_VISUAL, me);

                            DoCast(me, SPELL_DARK_MEDITATION);

                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                                me->SummonCreature(NPC_EMBODIED_DESPAIR, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                                me->SummonCreature(NPC_EMBODIED_DESPERATION, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            break;

                        case EVENT_SUN_MEDITATION_SPIKE: // HEROIC only!
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_MEDITATION_SPIKE_MISSILE);
				            events.ScheduleEvent(EVENT_SUN_MEDITATION_SPIKE, urand(4000, 6000));
                            break;

                        case EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS:
                            if (me->HealthBelowPct(100) && !eventComplete)
                            {
                                Talk(SUN_ANNOUNCE_BOND_LOTUS);
                                DoCast(me, SPELL_BOND_OF_THE_GOLDEN_LOTUS);
                                events.ScheduleEvent(EVENT_CHECK_BOND_OF_THE_GOLDEN_LOTUS, 16000);
                            }
                            break;

                        default: break;
                    }
                }

                // No melee, no tank needed.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sun_tenderheartAI(creature);
        }
};

/*** Adds (Desperate Measures) ***/

// Embodied Misery 71476.
class npc_embodied_misery : public CreatureScript
{
    public:
        npc_embodied_misery() : CreatureScript("npc_embodied_misery") { }

        struct npc_embodied_miseryAI : public ScriptedAI
        {
            npc_embodied_miseryAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                events.Reset();
                me->AddAura(SPELL_SHARED_TORMENT, me);
                events.ScheduleEvent(EVENT_DEFILED_GROUND, 10500);
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (Creature* sorrow = me->FindNearestCreature(NPC_EMBODIED_SORROW, 200.0f, true))
                    sorrow->SetHealth(me->GetHealth());
                if (Creature* gloom = me->FindNearestCreature(NPC_EMBODIED_GLOOM, 200.0f, true))
                    gloom->SetHealth(me->GetHealth());
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DEFILED_GROUND:
                            DoCast(me, SPELL_DEFILED_GROUND);
                            events.ScheduleEvent(EVENT_DEFILED_GROUND, 10500);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_embodied_miseryAI(creature);
        }
};

// Embodied Sorrow 71481.
class npc_embodied_sorrow : public CreatureScript
{
    public:
        npc_embodied_sorrow() : CreatureScript("npc_embodied_sorrow") { }

        struct npc_embodied_sorrowAI : public ScriptedAI
        {
            npc_embodied_sorrowAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                events.Reset();
                me->AddAura(SPELL_SHARED_TORMENT, me);
                events.ScheduleEvent(EVENT_INFERNO_STRIKE, 9500);
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (Creature* misery = me->FindNearestCreature(NPC_EMBODIED_MISERY, 200.0f, true))
                    misery->SetHealth(me->GetHealth());
                if (Creature* gloom = me->FindNearestCreature(NPC_EMBODIED_GLOOM, 200.0f, true))
                    gloom->SetHealth(me->GetHealth());
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INFERNO_STRIKE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                DoCast(target, SPELL_INFERNO_STRIKE);
                            events.ScheduleEvent(EVENT_INFERNO_STRIKE, 9500);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_embodied_sorrowAI(creature);
        }
};

// Embodied Gloom 71477.
class npc_embodied_gloom : public CreatureScript
{
    public:
        npc_embodied_gloom() : CreatureScript("npc_embodied_gloom") { }

        struct npc_embodied_gloomAI : public ScriptedAI
        {
            npc_embodied_gloomAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                events.Reset();
                me->AddAura(SPELL_SHARED_TORMENT, me);
                events.ScheduleEvent(EVENT_CORRUPTION_SHOCK_CHAIN, 15000);
            }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (Creature* misery = me->FindNearestCreature(NPC_EMBODIED_MISERY, 200.0f, true))
                    misery->SetHealth(me->GetHealth());
                if (Creature* sorrow = me->FindNearestCreature(NPC_EMBODIED_SORROW, 200.0f, true))
                    sorrow->SetHealth(me->GetHealth());
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CORRUPTION_SHOCK_CHAIN:
                            if (!me->GetMap()->IsHeroic()) // Corruption Shock on Normal.
                                DoCast(me, SPELL_CORRUPTION_SHOCK);
                            else                           // Corruption Chain on Heroic.
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                    DoCast(target, SPELL_CORRUPTION_CHAIN);
                            }
                            events.ScheduleEvent(EVENT_CORRUPTION_SHOCK_CHAIN, 15000);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_embodied_gloomAI(creature);
        }
};

// Embodied Anguish 71478.
class npc_embodied_anguish : public CreatureScript
{
    public:
        npc_embodied_anguish() : CreatureScript("npc_embodied_anguish") { }

        struct npc_embodied_anguishAI : public ScriptedAI
        {
            npc_embodied_anguishAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                me->AddAura(SPELL_SHADOW_WEAKNESS_AURA, me);
            }

            void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType damageType)
            {
                // Add a stack of Shadow Weakness to the victim on melee attack.
                if (damageType == DIRECT_DAMAGE)
                    me->AddAura(SPELL_SHADOW_WEAKNESS, victim);
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
            return new npc_embodied_anguishAI(creature);
        }
};

// Embodied Despair 71474.
class npc_embodied_despair : public CreatureScript
{
    public:
        npc_embodied_despair() : CreatureScript("npc_embodied_despair") { }

        struct npc_embodied_despairAI : public ScriptedAI
        {
            npc_embodied_despairAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                events.Reset();
                events.ScheduleEvent(EVENT_MANIFEST_DESPAIR, 3000);
            }

            void UpdateAI(uint32 const diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MANIFEST_DESPAIR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                DoCast(target, SPELL_MANIFEST_DESPAIR);
                            events.ScheduleEvent(EVENT_MANIFEST_DESPAIR, urand(5000, 7000));
                            break;

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_embodied_despairAI(creature);
        }
};

// Embodied Desperation 71482.
class npc_embodied_desperation : public CreatureScript
{
    public:
        npc_embodied_desperation() : CreatureScript("npc_embodied_desperation") { }

        struct npc_embodied_desperationAI : public ScriptedAI
        {
            npc_embodied_desperationAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void Reset() { }

            void EnterCombat(Unit* who)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                events.Reset();
                events.ScheduleEvent(EVENT_MANIFEST_DESPERATION, 3000);
            }

            void UpdateAI(uint32 const diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MANIFEST_DESPERATION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                                DoCast(target, SPELL_MANIFEST_DESPERATION);
                            events.ScheduleEvent(EVENT_MANIFEST_DESPERATION, urand(5000, 7000));
                            break;

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_embodied_desperationAI(creature);
        }
};

// Despair Spawn 71712.
class npc_despair_spawn : public CreatureScript
{
    public:
        npc_despair_spawn() : CreatureScript("npc_despair_spawn") { }

        struct npc_despair_spawnAI : public ScriptedAI
        {
            npc_despair_spawnAI(Creature* creature) : ScriptedAI(creature) { }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (Creature* despair = me->FindNearestCreature(NPC_EMBODIED_DESPAIR, 200.0f, true))
                    despair->SetHealth(despair->GetHealth() - uiDamage);
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
            return new npc_despair_spawnAI(creature);
        }
};

// Desperation Spawn 71993.
class npc_desperation_spawn : public CreatureScript
{
    public:
        npc_desperation_spawn() : CreatureScript("npc_desperation_spawn") { }

        struct npc_desperation_spawnAI : public ScriptedAI
        {
            npc_desperation_spawnAI(Creature* creature) : ScriptedAI(creature) { }

            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (Creature* desperation = me->FindNearestCreature(NPC_EMBODIED_DESPERATION, 200.0f, true))
                    desperation->SetHealth(desperation->GetHealth() - uiDamage);
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
            return new npc_desperation_spawnAI(creature);
        }
};

/*** Spells ***/

// Corrupted Brew (Boss cast) 143019.
class spell_rook_corrupted_brew : public SpellScriptLoader
{
    public:
        spell_rook_corrupted_brew() : SpellScriptLoader("spell_rook_corrupted_brew") { }

        class spell_rook_corrupted_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rook_corrupted_brew_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                uint32 count = 1; // 10 Normal.

                if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
                    count = 2;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                    count = 3;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                    count = 6;

                std::list<Unit*> targets;
                caster->ToCreature()->AI()->SelectTargetList(targets, count, SELECT_TARGET_RANDOM, 100.0f, true);
                if (!targets.empty())
                    for (std::list<Unit*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                        caster->CastSpell(*itr, SPELL_CORRUPTED_BREW_MISSILE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rook_corrupted_brew_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rook_corrupted_brew_SpellScript();
        }
};

// Clash (Boss cast) 143027.
class spell_rook_clash : public SpellScriptLoader
{
    public:
        spell_rook_clash() : SpellScriptLoader("spell_rook_clash") { }

        class spell_rook_clash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rook_clash_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_CLASH_ROOK, true);
                target->CastSpell(caster, SPELL_CLASH_TARGET, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rook_clash_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rook_clash_SpellScript();
        }
};

// Instant Poison 143210.
class spell_he_instant_poison : public SpellScriptLoader
{
    public:
        spell_he_instant_poison() : SpellScriptLoader("spell_he_instant_poison") { }

        class spell_he_instant_poison_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_he_instant_poison_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                caster->RemoveAurasDueToSpell(SPELL_NOXIOUS_POISON);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_he_instant_poison_SpellScript::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_he_instant_poison_SpellScript();
        }
};

// Noxious Poison 143225.
class spell_he_noxious_poison : public SpellScriptLoader
{
    public:
        spell_he_noxious_poison() : SpellScriptLoader("spell_he_noxious_poison") { }

        class spell_he_noxious_poison_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_he_noxious_poison_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                caster->RemoveAurasDueToSpell(SPELL_INSTANT_POISON);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_he_noxious_poison_SpellScript::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_he_noxious_poison_SpellScript();
        }
};

// Mark of Anguish (Boss cast) 143822.
class spell_he_mark_of_anguish : public SpellScriptLoader
{
    public:
        spell_he_mark_of_anguish() : SpellScriptLoader("spell_he_mark_of_anguish") { }

        class spell_he_mark_of_anguish_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_he_mark_of_anguish_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->AddAura(SPELL_MARK_OF_ANGUISH_MAIN, target);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_he_mark_of_anguish_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_he_mark_of_anguish_SpellScript();
        }
};

// Mark of Anguish (Main target spell) 143840.
class spell_he_mark_of_anguish_main : public SpellScriptLoader
{
    public:
        spell_he_mark_of_anguish_main() : SpellScriptLoader("spell_he_mark_of_anguish_main") { }

        class spell_he_mark_of_anguish_main_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_he_mark_of_anguish_main_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                target->AddAura(SPELL_DEBILITATION, target);

                // Make the Embodied Anguish attack this target.
                if (Creature* anguish = caster->FindNearestCreature(NPC_EMBODIED_ANGUISH, 300.0f, true))
                    anguish->AI()->AttackStart(target);
            }

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                target->CastSpell(target, SPELL_MARK_OF_ANGUISH_DAMAGE, true);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_he_mark_of_anguish_main_AuraScript::OnApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_he_mark_of_anguish_main_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_he_mark_of_anguish_main_AuraScript();
        }
};

// Mark of Anguish (Transfer spell) 143842.
class spell_he_mark_of_anguish_transfer : public SpellScriptLoader
{
    public:
        spell_he_mark_of_anguish_transfer() : SpellScriptLoader("spell_he_mark_of_anguish_transfer") { }

        class spell_he_mark_of_anguish_transfer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_he_mark_of_anguish_transfer_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(caster, SPELL_SHADOW_WEAKNESS_TRANSFER, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_he_mark_of_anguish_transfer_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_he_mark_of_anguish_transfer_SpellScript();
        }
};

// Sha Shear (Damage spell) 143424.
class spell_sun_sha_shear : public SpellScriptLoader
{
    public:
        spell_sun_sha_shear() : SpellScriptLoader("spell_sun_sha_shear") { }

        class spell_sun_sha_shear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sun_sha_shear_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetHitUnit();

                if (!target)
                    return;

                if (constAuraEffectPtr shearAurEff = target->GetAuraEffect(SPELL_SHA_SHEAR, EFFECT_0))
                    SetHitDamage(int32(GetHitDamage() + ((GetHitDamage() * shearAurEff->GetTickNumber()) / 4))); // Increases by 25% each tick.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sun_sha_shear_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sun_sha_shear_SpellScript();
        }
};

// Shadow Word : Bane (Boss cast) 143446.
class spell_sun_shadow_word_bane : public SpellScriptLoader
{
    public:
        spell_sun_shadow_word_bane() : SpellScriptLoader("spell_sun_shadow_word_bane") { }

        class spell_sun_shadow_word_bane_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sun_shadow_word_bane_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                uint32 count = 2; // 10 Normal.

                if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
                    count = 5;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                    count = 4;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                    count = 10;

                std::list<Unit*> targets;
                caster->ToCreature()->AI()->SelectTargetList(targets, count, SELECT_TARGET_RANDOM, 100.0f, true);
                if (!targets.empty())
                    for (std::list<Unit*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                        caster->CastSpell(*itr, SPELL_SHADOW_WORD_BANE_HIT, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sun_shadow_word_bane_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sun_shadow_word_bane_SpellScript();
        }
};

// Shadow Word : Bane (Main target spell) 143434.
class spell_sun_shadow_word_bane_main : public SpellScriptLoader
{
    public:
        spell_sun_shadow_word_bane_main() : SpellScriptLoader("spell_sun_shadow_word_bane_main") { }

        class spell_sun_shadow_word_bane_main_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sun_shadow_word_bane_main_AuraScript)

            void OnPeriodic(constAuraEffectPtr aurEff)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // First 3 ticks, it jumps to another player not having the main spell.
                if (aurEff->GetTickNumber() <= 3)
                    if (Creature* Sun = target->FindNearestCreature(BOSS_SUN_TENDERHEART, 300.0f, true))
                        if (Unit* playerTarget = Sun->ToCreature()->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true, -SPELL_SHADOW_WORD_BANE_HIT))
                            target->CastSpell(playerTarget, SPELL_SHADOW_WORD_BANE_JUMP, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sun_shadow_word_bane_main_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sun_shadow_word_bane_main_AuraScript();
        }
};

// Shadow Word : Bane (Jump spell) 143443.
class spell_sun_shadow_word_bane_jump : public SpellScriptLoader
{
    public:
        spell_sun_shadow_word_bane_jump() : SpellScriptLoader("spell_sun_shadow_word_bane_jump") { }

        class spell_sun_shadow_word_bane_jump_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sun_shadow_word_bane_jump_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_SHADOW_WORD_BANE_HIT, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sun_shadow_word_bane_jump_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sun_shadow_word_bane_jump_SpellScript();
        }
};

// Calamity 143491.
class spell_sun_calamity : public SpellScriptLoader
{
    public:
        spell_sun_calamity() : SpellScriptLoader("spell_sun_calamity") { }

        class spell_sun_calamity_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sun_calamity_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_CALAMITY_DAMAGE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sun_calamity_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sun_calamity_SpellScript();
        }
};

// Dark Meditation 143546.
class spell_sun_dark_meditation : public SpellScriptLoader
{
    public:
        spell_sun_dark_meditation() : SpellScriptLoader("spell_sun_dark_meditation") { }

        class spell_sun_dark_meditation_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sun_dark_meditation_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                caster->CastSpell(caster, SPELL_DARK_MEDITATION_DAMAGE, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sun_dark_meditation_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sun_dark_meditation_AuraScript();
        }
};

// Corruption Shock (Boss cast) 143958.
class spell_embodied_gloom_corruption_shock : public SpellScriptLoader
{
    public:
        spell_embodied_gloom_corruption_shock() : SpellScriptLoader("spell_embodied_gloom_corruption_shock") { }

        class spell_embodied_gloom_corruption_shock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_embodied_gloom_corruption_shock_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                uint32 count = 2; // 10 Normal.

                if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
                    count = 5;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                    count = 4;
                else if (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                    count = 10;

                std::list<Unit*> targets;
                caster->ToCreature()->AI()->SelectTargetList(targets, count, SELECT_TARGET_RANDOM, 100.0f, true);
                if (!targets.empty())
                    for (std::list<Unit*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                        caster->CastSpell(*itr, SPELL_CORRUPTION_SHOCK_MISSILE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_embodied_gloom_corruption_shock_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_embodied_gloom_corruption_shock_SpellScript();
        }
};

// Corruption Chain (Boss cast) 145653.
class spell_embodied_gloom_corruption_chain : public SpellScriptLoader
{
    public:
        spell_embodied_gloom_corruption_chain() : SpellScriptLoader("spell_embodied_gloom_corruption_chain") { }

        class spell_embodied_gloom_corruption_chain_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_embodied_gloom_corruption_chain_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_CORRUPTION_CHAIN_DAMAGE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_embodied_gloom_corruption_chain_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_embodied_gloom_corruption_chain_SpellScript();
        }
};

// Shadow Weakness (Transfer spell) 144081.
class spell_embodied_anguish_shadow_weakness_transfer : public SpellScriptLoader
{
    public:
        spell_embodied_anguish_shadow_weakness_transfer() : SpellScriptLoader("spell_embodied_anguish_shadow_weakness_transfer") { }

        class spell_embodied_anguish_shadow_weakness_transfer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_embodied_anguish_shadow_weakness_transfer_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                if (target->GetTypeId() == TYPEID_PLAYER)
                    caster->AddAura(SPELL_SHADOW_WEAKNESS, target);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_embodied_anguish_shadow_weakness_transfer_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_embodied_anguish_shadow_weakness_transfer_SpellScript();
        }
};

void AddSC_fallen_protectors()
{
    new boss_rook_stonetoe();
    new boss_he_softfoot();
    new boss_sun_tenderheart();
    new npc_embodied_misery();
    new npc_embodied_sorrow();
    new npc_embodied_gloom();
    new npc_embodied_anguish();
    new npc_embodied_despair();
    new npc_embodied_desperation();
    new npc_despair_spawn();
    new npc_desperation_spawn();
    new spell_rook_corrupted_brew();
    new spell_rook_clash();
    new spell_he_instant_poison();
    new spell_he_noxious_poison();
    new spell_he_mark_of_anguish();
    new spell_he_mark_of_anguish_main();
    new spell_he_mark_of_anguish_transfer();
    new spell_sun_sha_shear();
    new spell_sun_shadow_word_bane();
    new spell_sun_shadow_word_bane_main();
    new spell_sun_shadow_word_bane_jump();
    new spell_sun_calamity();
    new spell_sun_dark_meditation();
    new spell_embodied_gloom_corruption_shock();
    new spell_embodied_gloom_corruption_chain();
    new spell_embodied_anguish_shadow_weakness_transfer();
}
