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
 * Raid: Siege of Orgrimmar.
 * Boss: Norushen / Amalgam of Corruption.
 *
 * Wowpedia boss history:
 *
 * "Some say that the mogu race was created in the image of this titanic construct, left deep beneath Pandaria 
 *  to watch over and guard the continent's darkest and most dangerous secret."
 */

#include "ScriptPCH.h"
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
#include "MoveSplineInit.h"

#include "siege_of_orgrimmar.h"

/*
Intro:

Lorewalker Cho says: In the chamber ahead, under our peaceful land, slept the heart of an Old God.
Norushen yells: Halt!
Lorewalker Cho says: Oh my! What is this? Hello! I am Lorewalker Cho!
Norushen yells: No further corruption will enter the heart chamber!
Lorewalker Cho says: Further corruption? No, we are here to stop the corruption, and save Pandaria!
Norushen yells: You wish to purge the corruption?
Lorewalker Cho says: Yes! Please, let us pass!
Norushen yells: Should you pass this door, at this time, you would fail! You, all of you, are corrupted with the insidious plague known as Pride.
Norushen yells: You stand tall and proud atop your accomplishments, and this will be your downfall.
Norushen yells: Should you wish to defeat the corruption, you will first need to purify the corruption within yourselves.
Norushen yells: Speak to me again when you are prepared to face your inner demons.
*/

enum Yells
{
    /*** Bosses ***/

    // Lorewalker Cho Intro.

    SAY_LOREWALKER_INTRO_1       = 0,  // Astounding! Another chamber that looks to originate from these "Titans."
    SAY_LOREWALKER_INTRO_2       = 1,  // It must have been hidden away long before recorded history. And with good reason!

    SAY_LOREWALKER_EVENT_1       = 2,  // In the chamber ahead, under our peaceful land, slept the heart of an Old God.
    SAY_LOREWALKER_EVENT_2       = 3,  // Oh my! What is this? Hello! I am Lorewalker Cho! 38141
    SAY_LOREWALKER_EVENT_3       = 4,  // Further corruption? No, we are here to stop the corruption, and save Pandaria!
    SAY_LOREWALKER_EVENT_4       = 5,  // Yes! Please, let us pass!

    // Norushen Intro / Outro.

    SAY_NORUSHEN_EVENT_1         = 0,  // Halt!
    SAY_NORUSHEN_EVENT_2         = 1,  // No further corruption will enter the heart chamber!
    SAY_NORUSHEN_EVENT_3         = 2,  // You wish to purge the corruption?
    SAY_NORUSHEN_EVENT_4         = 3,  // Should you pass this door, at this time, you would fail! You, all of you, are corrupted with the insidious plague known as Pride.
                                       // You stand tall and proud atop your accomplishments, and this will be your downfall.
                                       // Should you wish to defeat the corruption, you will first need to purify the corruption within yourselves.
    SAY_NORUSHEN_EVENT_5         = 4,  // Speak to me again when you are prepared to face your inner demons.

    SAY_NORUSHEN_OUTRO           = 11, // You have been judged, and proven yourselves worthy. But be warned, what lies beyond will try your souls to their utmost. When you are fully prepared, you may enter the chamber.

    // Amalgam of Corruption.

    SAY_NORUSHEN_AGGRO_1         = 5,  // Very well, I will create a field to keep your corruption quarantined.
    SAY_NORUSHEN_AGGRO_2         = 6,  // Prove yourselves worthy and I will let you pass.
    SAY_NORUSHEN_AGGRO_3         = 7,  // The light cleanses; but it is not gentle. Gird yourselves and be purified!

    SAY_NORUSHEN_LOOK_WITHIN     = 8,  // 0 - Be purified in the light! ; 1 - Face your inner demons! ; 2 - Look inside yourself and cleanse the darkness within. ; 3 - Stand in the light and be judged! .
    SAY_NORUSHEN_KILL            = 9,  // 0 - Unworthy. ; 1 - Only the pure of heart may pass! .
    SAY_NORUSHEN_WIPE            = 10  // The light will not suffer your taint.
};

enum Spells
{
    /*** Bosses ***/

    // Norushen.

    SPELL_TELEPORT_PLAYERS       = 145188,
    SPELL_SHA_PRESUMMON          = 145143, // Triggers 145144 visuals and some other stuff.
    SPELL_SHA_SUMMON             = 145149, // Summon spell.

    //============================================================================================================================================================================//

    // Amalgam of Corruption.

    /*
        Amalgam
    */
    SPELL_AMALGAM                = 145118, // Players with Corruption inflict less damage to this creature. 7 mins duration -> Berserk.

    /*
        Corruption (bar)
        Measures the level of corruption on the target.
    */
    SPELL_CORRUPTION_BAR         = 144421, // Apply bar + Aura 303 (-50%) dmg decrease to Amalgamation + Periodic Dummy (Effect 2) - 1 second.
    SPELL_CORRUPTION_BAR_HIGH    = 147800, // 50% less damage inflicted to the Amalgam of Corruption. Periodic dummy (Effect 0) - 1 second + Aura 303 (- 50%).

    /*
        Despair
        Chokes the victim's heart with despair. After 10 sec the victim will be banished to their own realm to face their inner demons.
    */
    SPELL_DESPAIR                = 145725, // In LFR, there are no Purifying Light orbs and players are randomly selected to do the trial. This is their aura.

    /*
        Unleashed Anger
        The Amalgam slashes at its current tank target, inflicting 400000 Physical damage. 
    */
    SPELL_UNLEASHED_ANGER        = 145216, // Dummy 0.8 second cast, linked to above 145214 (no usage).
    SPELL_UNLEASHED_ANGER_DAMAGE = 145212, // Cast Time + Damage. This is the used spell.

    /*
        Self Doubt
        The caster reveals the deepest doubts of the victim, causing them to suffer 40% (50% H) more damage from the Amalgam's next Unleashed Anger.
    */
    SPELL_SELF_DOUBT             = 146124, // Stacks, Infected mechanic + 50% per app.

    /*
        Fusion
        Every Expelled Corruption that reaches the Amalgam of Corruption will increase the damage the Amalgam inflicts by 5% (8% H) for 15 sec. 
    */
    SPELL_FUSION                 = 145132,

    /*
        Icy Fear
        The Amalgam inflicts 75000 (100000+ H) Frost damage to all players every 3 sec. This damage increases as the Amalgam loses health.
    */
    SPELL_ICY_FEAR_AURA          = 145733, // Periodic damage aura.
    SPELL_ICY_FEAR_DAMAGE        = 145735, // Damage triggered by above, calculation script.

    /*
        Frayed
        When the Amalgam reaches 50% health remaining, and for every 10% health lost thereafter, the Amalgam spawns an Unleashed Manifestation of Corruption.
    */
    SPELL_FRAYED                 = 146179, // Dummy visual. Effect is handled through script.
    SPELL_UNLEASH_CORRUPTION_MIS = 145769, // Missile trigger.
    SPELL_UNLEASH_CORRUPTION_SUM = 145007, // Triggered by above, spawns the Manifestation of Corruption NPC.

    /*
        Unchecked Corruption
        Inflicts 650000 Shadow damage to all enemies.
        This attack is used by the Amalgam of Corruption whenever it is not actively engaged in melee combat.
    */
    SPELL_UNCHECKED_CORRUPTION   = 145679,

    /*
        Blind Hatred
    */
    SPELL_BLIND_HATRED           = 145226, // Actual channeled spell, triggers 145573 periodic dummy check.
    SPELL_BLIND_HATRED_NPC_VIS   = 145571, // Sha visual on far end npc.
    SPELL_BLIND_HATRED_BEAM_VIS  = 145613, // Beam visual boss on far end npc.
    SPELL_BLIND_HATRED_DMG       = 145227, // Damage for players between boss - far end npc.

    /*
        Foul Link
        The Amalgam links itself to corruption unleashed from players Looking Within in an attempt to grow stronger.
        Damage inflicted to this creature is copied to the Amalgam of Corruption.
    */
    SPELL_UNLEASHED_ESSENCE      = 146173, // Casted by boss on Essence in Normal realm spawn. SE (Effect 0) for SPELL_FOUL_LINK_ADDS. Spawn Effect.
    SPELL_UNLEASHED_MANIFEST     = 146174, // Casted by boss on Manifestation in Normal realm spawn. SE (Effect 0). Spawn Effect. Anim replacements. Eff 160.

    SPELL_FOUL_LINK_TRIGGER      = 149189, // Triggered by Unleashed spell. Applies dummy aura, visual.
    SPELL_FOUL_LINK_ADDS         = 148974, // Applies Share Damage aura 300 between Amalgamation and Adds. Casted Boss -> Adds.

    // Adds spawning.

    /*
        Unleash Corruption - Look Within + Normal realm.
        Whenever players successfully defeat an element of corruption during the Test of Serenity (DPS) it will be unleashed in the Normal realm.
        For Normal realm visual spawn effects + spawn triggers are handled by Victory Orb spawning spells.
    */
    SPELL_SPAWN_ESSENCE_N        = 144490, // Spawn effect + trigger 145006 summoning Normal realm NPC_ESSENCE_OF_CORRUPTION_N. Creates Areatrigger 1081 (NN). 145768
    SPELL_SPAWN_MANIF_N          = 144491, // Spawn effect + trigger 145007 summoning Normal realm NPC_MANIFEST_OF_CORRUPTION_N. Creates Areatrigger 1082 (NN). 145769

    SPELL_MIS_SPAWN_ESSENCE_N    = 145768, // Missile. Actual spell to use.
    SPELL_MIS_SPAWN_MANIF_N      = 145769, // Missile. Actual spell to use.

    SPELL_SPAWN_ESSENCE_L        = 144733, // Summons Look Within realm NPC_ESSENCE_OF_CORRUPTION_L.
    SPELL_SPAWN_MANIF_L          = 144739, // Summons Look Within realm NPC_MANIFEST_OF_CORRUPTION_L.

    SPELL_SPAWN_TITANIC_CORR     = 144848, // Summons Look Within realm NPC_TITANIC_CORRUPTION.
    SPELL_SPAWN_GREATER_CORR     = 144980, // Summons Look Within realm NPC_GREATER_CORRUPTION.

    /*** Adds ***/

    // Quarantine Measures.

    /*
        Quarantine Safety Measures
        The quarantine zone collapses, slaying all within.
    */
    SPELL_QUARANTINE_MEASURES    = 145779, // Instakill in 500yd + some Dummy triggering.

    //============================================================================================================================================================================//

    // Purifying Light.

    /*
        Look Within
        The caster looks within their own heart to face the darkness lurking within.
    */
    SPELL_LOOK_WITHIN            = 144724, // 1 minute duration and timer to stand in realm. Phase, Dummy (Effect 1), Screen Effect.
    SPELL_LOOK_WITHIN_PERIODIC   = 144717, // Triggered by above. NPC visual.

    SPELL_TEST_OF_SERENITY_DPS   = 144849, // DPS spec test Look Within realm aura.
    SPELL_TEST_OF_RELIANCE_HEAL  = 144850, // Healer spec test Look Within realm aura.
    SPELL_TEST_OF_CONFIDENCE_TK  = 144851, // Tank spec test Look Within realm aura.

    //============================================================================================================================================================================//

    // Trial adds - General.

    SPELL_MANIFEST_SPAWN_VIS     = 144778, // Spell used for Emerge visual.

    /*
        Cleanse
        Removes all corruption from the caster. Used on players if the successfully complete the spec trial.
    */
    SPELL_CLEANSE_TRIAL          = 147657, // Used by Greater Corruption and Titanic Corruption.

    /*
        Purified
        The process of Purification grants the caster immunity to all damage for a short time.
        Cleansed of corruption, the caster's soul is illuminated in purity.
    */
    SPELL_PURIFIED_DMG_IMMUNE    = 146022, // Casted on the player when he leaves Look Within realm.
    SPELL_PURIFIED_DMG_INCREASE  = 144452, // Stays on the player from 0 - 25% Corruption, aura spell.

    //============================================================================================================================================================================//

    // Greater Corruption.

    /*
        Disheartening Laugh - Soak.
        Bottomless Pit - Move and Avoid.
        Lingering Corruption - Dispel.
    */

    /*
        Lingering Corruption
        Inflicts 400000 (500000 H) Shadow damage if not dispelled from the victim within 10 sec.
    */
    SPELL_LINGERING_CORRUPTION   = 144514,

    /*
        Disheartening Laugh
        Inflicts Shadow damage to all enemies over 6 sec.
    */
    SPELL_DISHEARTENING_LAUGH    = 146707, // Periodic damage aura apply, dmg each 2 s.

    /*
        Bottomless Pit
        Creates a Bottomless Pit at a random enemy's destination that inflicts 100000 (135000 H) Shadow damage every 1 sec.
    */
    SPELL_BOTTOMLESS_PIT_DUMMY   = 146705, // Dummy cast spell on a player to create the Areatrigger.
    SPELL_BOTTOMLESS_PIT_AT      = 146793, // Creates Areatrigger 1257.
    SPELL_BOTTOMLESS_PIT_DMG     = 146703, // Applies periodic damage each 1 second.

    //============================================================================================================================================================================//

    // Titanic Corruption.

    /*
        Corruption - Soak.
        Burst of Corruption - Absorb or Heal.
        Piercing Corruption - Block or Dodge.
        Hurl Corruption - Interrupt or Reflect.
        Titanic Smash - Move.
    */

    /*
        Corruption
        Every successful strike received from the Titanic Corruption inflicts the victim with a stack of Corruption, 
        inflicting 17000 (25000 H) Shadow damage per application every second for 12 seconds.
    */
    SPELL_CORRUPTION             = 144639,

    /*
        Hurl Corruption
        The caster hurls a ball of Sha energy, inflicting 693750 to 806250 (925000 to 1075000 H) Shadow damage to the target. This attack cannot be absorbed.
    */
    SPELL_HURL_CORRUPTION        = 144649, // Must be interrupted or tank dies.

    /*
        Burst of Corruption
        Inflicts 277500 to 322500 (462500 to 537500 H) Shadow damage to all enemies.
    */
    SPELL_BURST_OF_CORRUPTION    = 144654,

    /*
        Piercing Corruption
        The caster strikes at the target, inflicting a piercing strike of 600000 (800000 H) Physical damage.
        This attack bypasses absorption effects but can be avoided and blocked.
    */
    SPELL_PIERCING_CORRUPTION    = 144657,

    /*
        Titanic Smash
        The caster reaches back and smashes down in front of them, inflicting 1000000 (1.5 mil H) Physical damage to all enemies in front of the caster.
    */
    SPELL_TITANIC_SMASH          = 144628,

    //============================================================================================================================================================================//

    // Manifestation of Corruption.

    // 1. For NPC_MANIFEST_OF_CORRUPTION_L:

    /*
        Cleanse
        Cures the caster of 40 Corruption.
    */
    SPELL_CLEANSE_MANIF_L        = 144450, // Used by the manifestation when it dies, on the attacking player facing the trial.

    /*
        Tear Reality
        The caster reaches back then swipes forward, inflicting Shadow damage to all enemies in a 15y cone in front of him.
    */
    SPELL_TEAR_REALITY           = 144482, // Cone weapon dmg %. Nice visual :).

    // 2. For NPC_MANIFEST_OF_CORRUPTION_N:

    /*
        Burst of Anger
        Inflicts 50000 Shadow damage (75000 H) to all players within the Quarantine Zone. 
    */
    SPELL_BURST_OF_ANGER         = 147082,

    // Spawns NPC_RESIDUAL_CORRUPTION when it dies.

    //============================================================================================================================================================================//

    // Essence of Corruption.

    // 1. For NPC_ESSENCE_OF_CORRUPTION_L:

    /*
        Cleanse
        Cures the caster of 15 Corruption.
    */
    SPELL_CLEANSE_ESSENCE_L      = 144449, // Used by the essence when it dies, on the attacking player facing the trial.

    /*
        Essence of Corruption
        Dark energy forms a shield in front of the caster, deflecting spells and attacks made from the front.
    */
    SPELL_ESSENCE_OF_CORRUPTION  = 148452, // Spawn visual, Stealth Detection, triggers Dark Bulwark.
    SPELL_DARK_BULWARK           = 149601, // Block / Parry / Dodge increase by 100% from the front.

    /*
        Expel Corruption
        A ball of corrupted energy travels outward from the caster, inflicting 97500 to 102500 Shadow damage to the first enemy in its path.
        Every Expelled Corruption that reaches the Amalgam of Corruption will increase the damage the Amalgam inflicts by 5% for 15 sec. 
    */
    SPELL_EXPEL_CORRUPTION_L     = 144479, // Effect 0 Dummy. Effect 1 creates moving Areatrigger 1080.
    SPELL_EXPELLED_CORRUPTION_DL = 144480, // Periodic damage each sec. to the target in the Expelled Corruption's path. Same visual and effects as first.

    // 2. For NPC_ESSENCE_OF_CORRUPTION_N:

    /*
        Expel Corruption
        A ball of corrupted energy travels outward from the caster, inflicting 97500 to 102500 Shadow damage to the first enemy in its path.
        Every Expelled Corruption that reaches the Amalgam of Corruption will increase the damage the Amalgam inflicts by 5% for 15 sec. 
    */
    SPELL_EXPEL_CORRUPTION       = 145064, // Effect 0 Dummy. Effect 1 creates moving Areatrigger 1106.
    SPELL_EXPELLED_CORRUPTION_D  = 144547, // Damage to the target in the Expelled Corruption's path.
    SPELL_EXPEL_CORRUPTION_VIS   = 132094, // A ball that can be used as areatrigger movement.

    //============================================================================================================================================================================//

    // Residual Corruption.

    /*
        Residual Corruption
        When an Unleashed Manifestation of Corruption dies, it leaves behind a small amount of corruption that will 
        periodically inflict 90000 Shadow damage to all players in the Quarantine Zone until it is picked up.
        Players are unable to pick up the Residual Corruption unless they have removed sufficient corruption of their own first.
        Residual Corruption gives the player 25 Corruption. 
    */
    SPELL_RESIDUAL_CORUPTION     = 145052, // Gives Corruption and triggers below.
    SPELL_RESIDUAL_CORRUPTION_A  = 145074, // Cast time, spawn effect on NPC and adds periodic dmg aura. Creates Areatrigger 1107, despawns NPC @ touch + 25 C.
    SPELL_RESIDUAL_CORRUPTION_D  = 145073  // Damage to all players.
};

enum Events
{
    /*** Bosses ***/

    // Norushen / General.

    EVENT_INTRO_1                = 1,
    EVENT_INTRO_2,
    EVENT_INTRO_3,
    EVENT_INTRO_4,
    EVENT_INTRO_5,
    EVENT_INTRO_6,
    EVENT_INTRO_7,
    EVENT_INTRO_8,
    EVENT_INTRO_9,
    EVENT_INTRO_10,
    EVENT_INTRO_11,
    EVENT_INTRO_DONE,

    EVENT_ENCOUNTER_1,
    EVENT_ENCOUNTER_2,
    EVENT_ENCOUNTER_3,
    EVENT_START_COMBAT,

    EVENT_START_OUTRO,
    EVENT_OUTRO_SAY_OPEN_DOOR,
    EVENT_SPAWN_NORUSHEN_PRIDE,

    // Amalgam of Corruption.

    EVENT_UNLEASHED_ANGER,
    EVENT_BLIND_HATRED,
    EVENT_CHECK_UNCHECKED_CORRUPTION,
    EVENT_SAFETY_MEASURES,

    EVENT_SPAWN_PURIF_LIGHT_ORB_1,
    EVENT_SPAWN_PURIF_LIGHT_ORB_2,
    EVENT_SPAWN_PURIF_LIGHT_ORB_3,
    EVENT_SPAWN_PURIF_LIGHT_ORB_4,
    EVENT_SPAWN_PURIF_LIGHT_ORB_5,

    // Lorewalker Cho.

    EVENT_CHO_MOVE_1,
    EVENT_CHO_MOVE_2,
    EVENT_CHO_MOVE_3,
    EVENT_CHO_MOVE_4,
    EVENT_CHO_MOVE_5,

    /*** Adds ***/

    // Manifestation of Corruption.

    EVENT_TEAR_REALITY,
    EVENT_BURST_OF_ANGER,

    // Essence of Corruption.

    EVENT_EXPEL_CORRUPTION,

    // Greater Corruption.

    EVENT_DISHEARTENING_LAUGH,
    EVENT_LINGERING_CORRUPTION,
    EVENT_BOTTOMLESS_PIT,

    // Titanic Corruption.

    EVENT_CORRUPTION,
    EVENT_HURL_CORRUPTION,
    EVENT_BURST_OF_CORRUPTION,
    EVENT_PIERCING_CORRUPTION,
    EVENT_TITANIC_SMASH,

    // Blind Hatred.

    EVENT_MOVE_CIRCLE,
    EVENT_HATRED_TARGET_CHECK,
};

enum Actions
{
    /*** Bosses ***/

    // Norushen / General.

    ACTION_START_NORUSHEN_INTRO  = 1, // Intro with Norushen.
    ACTION_START_LOREWALKER_INTRO,    // Intro with Lorewalker Cho.
    ACTION_START_EVENT,               // Gossip select.
    ACTION_RESET_EVENT,               // Encounter failed / other reasons.
    ACTION_FINISHED_EVENT,            // Encounter succedeed.

    // Amalgam of Corruption.

    ACTION_DECREASE_AVAILABLE_REALM_COUNT,

    /*** Adds ***/

    // Quarantine Measures.

    ACTION_QUARANTINE_MEASURES
};

enum Npcs
{
    /*** Bosses ***/

    // Norushen / General.

    NPC_LOREWALKER_CHO_NORUSHEN  = 72872,

    /*** Adds ***/

    // Norushen / General.

    NPC_QUARANTINE_MEASURES      = 72669,

    // Amalgam of Corruption.

    NPC_BLIND_HATRED             = 72565,

    NPC_PURIFYING_LIGHT          = 72065, // Orbs used to enter the Look Within realm. 2 in 10 players, 4 in 25.

    NPC_GREATER_CORRUPTION       = 72001, // Healer trial.
    NPC_TITANIC_CORRUPTION       = 72051, // Tank trial.

    // Each has 2 versions, a Look Within realm one (for DPS trial) and a Normal realm (Unleashed) one.
    /* Unlike tanks and healers (who lose all Corruption at the end of their test), DPS lose Corruption based on how many adds they defeat.
       Defeating the Manifestation removes 40 Corruption, and defeating Essences removes 15 Corruption per Essence.
       There will always be one Manifestation, while the number of Essences to defeat depends on player's initial Corruption when entering the Test Realm. */
    NPC_MANIFEST_OF_CORRUPTION_L = 71977,
    NPC_MANIFEST_OF_CORRUPTION_N = 72264,

    NPC_ESSENCE_OF_CORRUPTION_L  = 71976,
    NPC_ESSENCE_OF_CORRUPTION_N  = 72263,

    NPC_RESIDUAL_CORRUPTION      = 72550, // Spawned by NPC_MANIFEST_OF_CORRUPTION_N when it dies.

    NPC_EXPELLED_CORRUPTION      = 74001, // Spawned by NPC_ESSENCE_OF_CORRUPTION_N / L.

    // Greater Corruption fight NPC's.
    NPC_LEVEN_DAWNBLADE_GC       = 71995,
    NPC_ROOK_STONETOE_GC         = 71996,
    NPC_SUN_TENDERHEART_GC       = 72000
};

enum GOs
{
    /*** Bosses ***/

    // Norushen - Quarantine Zone.
    NORUSHEN_LIGHT_CONTAIN_WALL1 = 223142,
    NORUSHEN_LIGHT_CONTAIN_WALL2 = 223143,
    NORUSHEN_LIGHT_CONTAIN_WALL3 = 223144,
    NORUSHEN_LIGHT_CONTAIN_WALL4 = 223145,
    NORUSHEN_LIGHT_CONTAIN_CEIL  = 223146,
    NORUSHEN_LIGHT_CONTAIN_FLOOR = 223147
};

/*
    Players start the encounter with 75 points of Corruption. While at this level of Corruption they will inflict 50% less damage to the Amalgam of Corruption.
    When players reach 0 points of Corruption remaining, they become Purified and will inflict 25% more damage to the Amalgam. 
    With each 25 corruption their damage decreases by 25%.
*/
enum CorruptionLevels
{
    CORRUPTION_LEVEL_NONE =   0, // Purified, 125% damage done.
    CORRUPTION_LEVEL_LOW  =  25, // Purified removal, 100% damage done.
    CORRUPTION_LEVEL_MED  =  50, // 75% damage done.
    CORRUPTION_LEVEL_HIGH =  75, // Encounter start, 50% damage done.
    CORRUPTION_LEVEL_FULL = 100  // No side-effect, but 0 damage done.
};

enum MovementPoints
{
    // Lorewalker Cho (Intro).
    POINT_CHO_MOVE_1             = 1,
    POINT_CHO_MOVE_2             = 2,
    POINT_CHO_MOVE_3             = 3,
    POINT_CHO_MOVE_4             = 4,
    POINT_CHO_MOVE_5             = 5,

    // Norushen (Outro).
    POINT_NORUSHEN_OUTRO_MID     = 1
};

enum EncounterRealmPhaseMasks
{
    // Basic phase.
    PHASEMASK_NORMAL_REALM       = 1,

    // There are 5 total Look Within realms usable at once.
    PHASEMASK_LOOK_WITHIN_REALM1 = 2,
    PHASEMASK_LOOK_WITHIN_REALM2 = 4,
    PHASEMASK_LOOK_WITHIN_REALM3 = 8,
    PHASEMASK_LOOK_WITHIN_REALM4 = 16,
    PHASEMASK_LOOK_WITHIN_REALM5 = 32,

    PHASEMASK_ALL_REALMS         = 63
};

enum LookWithinRealmsCount
{
    // Max. 20% of the players at once, depending on difficulty.
    LW_REALM_COUNT_10MAN         = 2,
    LW_REALM_COUNT_25MAN         = 5
};

// Lorewalker Cho spawn position.
Position const loreChoSpawnPos   = { 842.565f, 878.615f, 371.047f };

// Quarantine Measures spawn position.
Position const quarMeasSpawnPos  = { 777.509f, 974.266f, 356.340f };

// Blind Hatred spawn position.
Position const hatredSpawnPos    = { 809.392f, 1023.77f, 356.084f };

// Lorewalker Cho movement positions.
Position const choIntroMove[5]   =
{
    { 824.137f, 873.984f, 371.046f },
    { 807.499f, 877.782f, 371.067f },
    { 792.955f, 893.652f, 371.130f },
    { 787.619f, 917.563f, 356.090f },
    { 787.397f, 927.692f, 356.073f }
};

// Purifying Light spawn positions.
Position const purifyingLight[5] =
{
    { 764.870f, 982.885f, 356.340f },
    { 768.526f, 961.314f, 356.340f },
    { 785.719f, 987.002f, 356.340f },
    { 790.052f, 966.103f, 356.340f },
    { 775.052f, 973.103f, 356.340f }
};

/*** Bosses ***/

// Norushen 71965.
class boss_norushen : public CreatureScript
{
    public:
        boss_norushen() : CreatureScript("boss_norushen") { }

        struct boss_norushenAI : public ScriptedAI
        {
            boss_norushenAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                instance = creature->GetInstanceScript();

                choSummoned  = false;
                introStarted = false;
                introDone    = false;
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            bool choSummoned, introStarted, introDone, eventStarted;
            Creature* lorewalkerChoIntro;

            /*** Special AI Functions ***/

            void ActivateQuarantineZone()
            {
                std::list<GameObject*> wallsList;
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL1, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL2, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL3, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL4, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_CEIL,  300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_FLOOR, 300.0f);

                if (!wallsList.empty())
                    for (std::list<GameObject*>::iterator walls = wallsList.begin(); walls != wallsList.end(); walls++)
                        (*walls)->SetGoState(GO_STATE_READY);
            }

            void RemoveQuarantineZone()
            {
                std::list<GameObject*> wallsList;
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL1, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL2, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL3, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_WALL4, 300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_CEIL,  300.0f);
                GetGameObjectListWithEntryInGrid(wallsList, me, NORUSHEN_LIGHT_CONTAIN_FLOOR, 300.0f);

                if (!wallsList.empty())
                    for (std::list<GameObject*>::iterator walls = wallsList.begin(); walls != wallsList.end(); walls++)
                        (*walls)->SetGoState(GO_STATE_ACTIVE);
            }

            /*** General AI Functions ***/

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                if (instance)
                    instance->SetData(DATA_NORUSHEN_EVENT, NOT_STARTED);

                eventStarted = false; // Actual event.
                lorewalkerChoIntro = NULL;
            }

            void EnterCombat(Unit* /*who*/) { }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

                // Boss goes to combat aprox. 2 seconds later.
                if (summon->GetEntry() != BOSS_AMALGAM_OF_CORRUPTION)
                {
                    summon->SetInCombatWithZone();
                    summon->SetReactState(REACT_PASSIVE);
			    }

                // We use Quarantine Measures npc as a spawn trigger for all Look Within realms too.
                if (summon->GetEntry() == NPC_QUARANTINE_MEASURES)
                    summon->SetPhaseMask(PHASEMASK_ALL_REALMS, true);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_START_NORUSHEN_INTRO:
                        events.ScheduleEvent(EVENT_INTRO_1, 100);
                        break;

                    case ACTION_START_EVENT:
                        events.ScheduleEvent(EVENT_ENCOUNTER_1, 100);
                        break;

                    case ACTION_RESET_EVENT:
                        EnterEvadeMode();
                        break;

                    case ACTION_FINISHED_EVENT:
                        events.ScheduleEvent(EVENT_START_OUTRO, 100);
                        break;

                    default: break;
                }
            }

            void EnterEvadeMode()
            {
                Talk(SAY_NORUSHEN_WIPE);

                me->RemoveAllAuras();
                RemoveQuarantineZone();

                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);

                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

	            if (instance)
                {
                    instance->SetData(DATA_NORUSHEN_EVENT, FAIL);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CORRUPTION_BAR);
                    if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 300.0f, true))
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, amalgam); // Remove.
                }
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case POINT_NORUSHEN_OUTRO_MID: // Middle reached.
                        events.ScheduleEvent(EVENT_OUTRO_SAY_OPEN_DOOR, 100);
                        break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);
            
                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Intro.

                        // Lorewalker Cho single.

                        case EVENT_INTRO_1:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_INTRO_1);
                            events.ScheduleEvent(EVENT_INTRO_2, 8000);
                            break;

                        case EVENT_INTRO_2:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_INTRO_2);
                            events.ScheduleEvent(EVENT_INTRO_3, 8000);
                            break;

                        // Lorewalker Cho and Norushen.

                        case EVENT_INTRO_3:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_EVENT_1);
                            events.ScheduleEvent(EVENT_INTRO_4, 9000);
                            break;

                        case EVENT_INTRO_4:
                            Talk(SAY_NORUSHEN_EVENT_1);
                            events.ScheduleEvent(EVENT_INTRO_5, 4000);
                            break;

                        case EVENT_INTRO_5:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_EVENT_2);
                            events.ScheduleEvent(EVENT_INTRO_6, 7000);
                            break;

                        case EVENT_INTRO_6:
                            Talk(SAY_NORUSHEN_EVENT_2);
                            events.ScheduleEvent(EVENT_INTRO_7, 7000);
                            break;

                        case EVENT_INTRO_7:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_EVENT_3);
                            events.ScheduleEvent(EVENT_INTRO_8, 8000);
                            break;

                        case EVENT_INTRO_8:
                            Talk(SAY_NORUSHEN_EVENT_3);
                            events.ScheduleEvent(EVENT_INTRO_9, 6000);
                            break;

                        case EVENT_INTRO_9:
                            if (lorewalkerChoIntro) lorewalkerChoIntro->AI()->Talk(SAY_LOREWALKER_EVENT_4);
                            events.ScheduleEvent(EVENT_INTRO_10, 3000);
                            break;

                        case EVENT_INTRO_10:
                            Talk(SAY_NORUSHEN_EVENT_4);
                            events.ScheduleEvent(EVENT_INTRO_11, 32000);
                            break;

                        case EVENT_INTRO_11:
                            Talk(SAY_NORUSHEN_EVENT_5);
                            events.ScheduleEvent(EVENT_INTRO_DONE, 8000);
                            break;

                        case EVENT_INTRO_DONE:
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            introDone = true;
                            break;

                        // Gossip selected, actual event starts.

                        case EVENT_ENCOUNTER_1:
                            Talk(SAY_NORUSHEN_AGGRO_1);
                            ActivateQuarantineZone();
                            eventStarted = true;
	                        if (instance)
                            {
                                instance->SetData(DATA_NORUSHEN_EVENT, IN_PROGRESS);
                                instance->DoCastSpellOnPlayers(SPELL_TELEPORT_PLAYERS); // Teleport the players to the middle.
					        }
                            events.ScheduleEvent(EVENT_ENCOUNTER_2, 8000);
                            break;

                        case EVENT_ENCOUNTER_2:
                            Talk(SAY_NORUSHEN_AGGRO_2);
                            if (Creature* safetyMeasures = me->SummonCreature(NPC_QUARANTINE_MEASURES, quarMeasSpawnPos, TEMPSUMMON_MANUAL_DESPAWN))
                                DoCast(safetyMeasures, SPELL_SHA_PRESUMMON, false);
                            events.ScheduleEvent(EVENT_ENCOUNTER_3, 10200);
                            break;

                        case EVENT_ENCOUNTER_3:
                            Talk(SAY_NORUSHEN_AGGRO_3);
                            if (Creature* safetyMeasures = me->FindNearestCreature(NPC_QUARANTINE_MEASURES, 300.0f, true))
                                DoCast(safetyMeasures, SPELL_SHA_SUMMON, false);
                            events.ScheduleEvent(EVENT_START_COMBAT, 2000);
                            break;

                        case EVENT_START_COMBAT:
                            if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 300.0f, true))
                            {
                                amalgam->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                amalgam->SetReactState(REACT_AGGRESSIVE);
                                amalgam->AI()->DoZoneInCombat(amalgam, 100.0f);
					        }
                            break;

                        // Outro.

                        case EVENT_START_OUTRO:
                            if (instance)
                                instance->SetData(DATA_NORUSHEN_EVENT, DONE);
                            me->GetMotionMaster()->MovePoint(POINT_NORUSHEN_OUTRO_MID, quarMeasSpawnPos); // Move mid and talk.
                            break;

                        case EVENT_OUTRO_SAY_OPEN_DOOR:
                            Talk(SAY_NORUSHEN_OUTRO);
                            events.ScheduleEvent(EVENT_SPAWN_NORUSHEN_PRIDE, 20000);
                            break;

                        case EVENT_SPAWN_NORUSHEN_PRIDE:
                            me->SummonCreature(BOSS_NORUSHEN_PRIDE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
                            me->DespawnOrUnsummon(100);
                            break;

                        default: break;
                    }
                }
            }
        };

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver()) player->PrepareQuestMenu(creature->GetGUID());

            if (CAST_AI(boss_norushen::boss_norushenAI, creature->AI())->introDone == true &&
                CAST_AI(boss_norushen::boss_norushenAI, creature->AI())->eventStarted == false && 
                creature->GetInstanceScript() && creature->GetInstanceScript()->GetData(DATA_NORUSHEN_EVENT) != DONE) // Opening gossip.
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "We are ready, Keeper.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->SEND_GOSSIP_MENU(71965, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 uiSender, uint32 uiAction)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();

            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    creature->AI()->DoAction(ACTION_START_EVENT);
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;

                default: break;
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_norushenAI(creature);
        }
};

// Amalgam of Corruption 72276.
class boss_amalgam_of_corruption : public CreatureScript
{
    public:
        boss_amalgam_of_corruption() : CreatureScript("boss_amalgam_of_corruption") { }

        struct boss_amalgam_of_corruptionAI : public BossAI
        {
            boss_amalgam_of_corruptionAI(Creature* creature) : BossAI(creature, DATA_NORUSHEN_EVENT), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;

            uint8 purifyingRealmsAvailable;
            bool summonedFrayed[5], orbsMissing[5];

            /*** Special AI Functions ***/

            int32 CalculateCorruptionDamageTakenPercent(Unit* target)
            {
                int32 basePoints = 0;
                if (!target)
                    return basePoints;

                int32 corruption = target->GetPower(POWER_ALTERNATE_POWER);

                if (corruption >= CORRUPTION_LEVEL_NONE          && corruption <= CORRUPTION_LEVEL_LOW)
                    basePoints =  0;
                else if (corruption > CORRUPTION_LEVEL_LOW       && corruption <= CORRUPTION_LEVEL_MED)
                    basePoints = -25;
                else if (corruption > CORRUPTION_LEVEL_MED       && corruption <= CORRUPTION_LEVEL_HIGH)
                    basePoints = -50;
                else if (corruption > CORRUPTION_LEVEL_HIGH      && corruption <= CORRUPTION_LEVEL_FULL)
                    basePoints = -75;
                else if (corruption == CORRUPTION_LEVEL_FULL)
                    basePoints = -100;

                return basePoints;
            }

            uint8 GetLookWithinRealmsCount()
            {
                return (me->GetMap()->Is25ManRaid() ? LW_REALM_COUNT_25MAN : LW_REALM_COUNT_10MAN);
            }

            void UnleashAdds(uint32 entry)
            {
                switch (entry)
                {
                    case NPC_MANIFEST_OF_CORRUPTION_N:
                        DoCast(me, SPELL_MIS_SPAWN_MANIF_N);
                        break;

                    case NPC_ESSENCE_OF_CORRUPTION_N:
                        DoCast(me, SPELL_MIS_SPAWN_ESSENCE_N);
                        break;

                    default: break;
                }
            }

            /*** General AI Functions ***/

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);

                purifyingRealmsAvailable = 0;

                for (uint8 i = 0; i < 5; i++)
                    orbsMissing[i] = false;

                for (uint8 i = 0; i < 5; i++)
                    summonedFrayed[i] = false;

                _Reset();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                DoCast(me, SPELL_AMALGAM); // Emerge + aura.
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->AddAura(SPELL_ICY_FEAR_AURA, me);

                // Set the Look Within realms count and spawn the NPC's.
                purifyingRealmsAvailable = GetLookWithinRealmsCount();
                for (uint8 i = 0; i < purifyingRealmsAvailable; i++)
                    me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[i]);

                if (instance)
                {
                    instance->DoAddAuraOnPlayers(SPELL_CORRUPTION_BAR);
                    instance->DoSetAlternatePowerOnPlayers(CORRUPTION_LEVEL_HIGH);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add.
                }

                events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);
                events.ScheduleEvent(EVENT_BLIND_HATRED, 25000);
                events.ScheduleEvent(EVENT_CHECK_UNCHECKED_CORRUPTION, 6000);

                events.ScheduleEvent(EVENT_SAFETY_MEASURES, 7 * MINUTE * IN_MILLISECONDS); // 7 minute berserk.

                _EnterCombat();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

				if (me->isInCombat())
                    summon->AI()->DoZoneInCombat(summon, 100.0f);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* norushen = me->FindNearestCreature(BOSS_NORUSHEN_AMALGAM, 300.0f, true))
                        norushen->AI()->Talk(SAY_NORUSHEN_KILL);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_DECREASE_AVAILABLE_REALM_COUNT:
                        purifyingRealmsAvailable -= 1;
                        break;

                    default: break;
                }
            }

            void EnterEvadeMode()
            {
                me->RemoveAllAuras();
                me->RemoveAllAreaTriggers();

                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);

                _EnterEvadeMode();

                if (Creature* norushen = me->FindNearestCreature(BOSS_NORUSHEN_AMALGAM, 300.0f, true))
                    norushen->AI()->DoAction(ACTION_RESET_EVENT);
            }

            void JustDied(Unit* /*killer*/)
            {
                me->RemoveAllAreaTriggers();

                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CORRUPTION_BAR);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove.
			    }

                if (Creature* norushen = me->FindNearestCreature(BOSS_NORUSHEN_AMALGAM, 300.0f, true))
                    norushen->AI()->DoAction(ACTION_FINISHED_EVENT);

                _JustDied();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // Handle Frayed aura addition and mechanic.
                if (me->HealthBelowPct(51) && !me->HasAura(SPELL_FRAYED))
                    me->AddAura(SPELL_FRAYED, me);

                if (me->HasAura(SPELL_FRAYED))
                {
                    for (uint8 i = 0; i < 5; i++) // Ex. : Check 40 %: i = 1 => 51 - 10 => HealthBelowPct(41).
                    {
						if (me->HealthBelowPct(51 - (i * 10)) && !summonedFrayed[i])
                        {
                            summonedFrayed[i] = true;
                            DoCast(me, SPELL_UNLEASH_CORRUPTION_MIS);
                        }
                    }
                }

                // Handle Purifying Light orbs spawning.
                if (purifyingRealmsAvailable < GetLookWithinRealmsCount())
                {
                    for (uint8 i = 0; i < 5; i++)
                    {
                        if (orbsMissing[i])
                        {
                            events.ScheduleEvent(EVENT_SPAWN_PURIF_LIGHT_ORB_1 + i, 61000);
                            orbsMissing[i] = false;
                        }
                    }
                }

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UNLEASHED_ANGER:
                            DoCast(me->getVictim(), SPELL_UNLEASHED_ANGER);
                            events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);
                            break;

                        case EVENT_BLIND_HATRED:
                            if (Creature* hatred = me->SummonCreature(NPC_BLIND_HATRED, hatredSpawnPos))
                                DoCast(hatred, SPELL_BLIND_HATRED);
                            events.ScheduleEvent(EVENT_BLIND_HATRED, 60000);
                            break;

                        case EVENT_CHECK_UNCHECKED_CORRUPTION:
                            if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                                DoCast(me, SPELL_UNCHECKED_CORRUPTION);
                            events.ScheduleEvent(EVENT_CHECK_UNCHECKED_CORRUPTION, 4000);
                            break;

                        case EVENT_SAFETY_MEASURES:
                            if (Creature* quarantine = me->FindNearestCreature(NPC_QUARANTINE_MEASURES, 200.0f, true))
                                quarantine->AI()->DoAction(ACTION_QUARANTINE_MEASURES);
                            break;

                        // Purifying Light orbs spawning.

                        case EVENT_SPAWN_PURIF_LIGHT_ORB_1:
                            me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[0]);
                            break;

                        case EVENT_SPAWN_PURIF_LIGHT_ORB_2:
                            me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[1]);
                            break;

                        case EVENT_SPAWN_PURIF_LIGHT_ORB_3:
                            me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[2]);
                            break;

                        case EVENT_SPAWN_PURIF_LIGHT_ORB_4:
                            me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[3]);
                            break;

                        case EVENT_SPAWN_PURIF_LIGHT_ORB_5:
                            me->SummonCreature(NPC_PURIFYING_LIGHT, purifyingLight[4]);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_amalgam_of_corruptionAI(creature);
        }
};

/*** Adds ***/

// Manifestation of Corruption 72264 71977.
class npc_manifestation_of_corruption : public CreatureScript
{
    public:
        npc_manifestation_of_corruption() : CreatureScript("npc_manifestation_of_corruption") { }

        struct npc_manifestation_of_corruptionAI : public ScriptedAI
        {
            npc_manifestation_of_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();

                if (me->GetEntry() == NPC_MANIFEST_OF_CORRUPTION_L)
                    me->AddAura(SPELL_MANIFEST_SPAWN_VIS, me); // Emerge.
                else // Foul Link handling.
                {
                    me->AddAura(SPELL_FOUL_LINK_TRIGGER, me);
                    if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                        amalgam->CastSpell(me, SPELL_FOUL_LINK_ADDS, true);
                }

                DoZoneInCombat(me, 100.0f);
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (me->GetEntry() == NPC_MANIFEST_OF_CORRUPTION_L)
                {
                    me->RemoveAurasDueToSpell(SPELL_MANIFEST_SPAWN_VIS);
                    events.ScheduleEvent(EVENT_TEAR_REALITY, 4000);
                }
                else
                {
                    events.ScheduleEvent(EVENT_BURST_OF_ANGER, urand(3000, 6000));
                }
            }

            void EnterEvadeMode()
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);

                me->DespawnOrUnsummon();
            }

            void JustDied(Unit* killer)
            {
                if (me->GetEntry() == NPC_MANIFEST_OF_CORRUPTION_L)
                {
                    if (killer)
                        killer->CastSpell(killer, SPELL_CLEANSE_MANIF_L, true);

                    // Have the Amalgam Unleash it when killed.
                    if (Creature* quarantine = me->FindNearestCreature(NPC_QUARANTINE_MEASURES, 200.0f, true))
                        if (Creature* amalgam = quarantine->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                            CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->UnleashAdds(NPC_MANIFEST_OF_CORRUPTION_N);
                }
                else // Summon Residual Corruption.
                    me->SummonCreature(NPC_RESIDUAL_CORRUPTION, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TEAR_REALITY:
                            DoCast(me, SPELL_TEAR_REALITY);
                            events.ScheduleEvent(EVENT_TEAR_REALITY, urand(9000, 12000));
                            break;

                        case EVENT_BURST_OF_ANGER:
                            DoCast(me, SPELL_BURST_OF_ANGER);
                            events.ScheduleEvent(EVENT_BURST_OF_ANGER, urand(9000, 12000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_manifestation_of_corruptionAI(creature);
        }
};

// Essence of Corruption 72263 71976.
class npc_essence_of_corruption : public CreatureScript
{
    public:
        npc_essence_of_corruption() : CreatureScript("npc_essence_of_corruption") { }

        struct npc_essence_of_corruptionAI : public ScriptedAI
        {
            npc_essence_of_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();

                if (me->GetEntry() == NPC_ESSENCE_OF_CORRUPTION_L)
                    DoCast(me, SPELL_ESSENCE_OF_CORRUPTION); // Emerge + Bulwark aura.
                else // Foul Link handling.
                {
                    me->AddAura(SPELL_FOUL_LINK_TRIGGER, me);
                    if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                        amalgam->CastSpell(me, SPELL_FOUL_LINK_ADDS, true);
                }

                DoZoneInCombat(me, 100.0f);
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_EXPEL_CORRUPTION, urand(3000, 7000));
            }

            void EnterEvadeMode()
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);

                me->DespawnOrUnsummon();
            }

            void JustDied(Unit* killer)
            {
                if (me->GetEntry() == NPC_ESSENCE_OF_CORRUPTION_L)
                {
                    if (killer)
                        killer->CastSpell(killer, SPELL_CLEANSE_ESSENCE_L, true);

                    // Have the Amalgam Unleash it when killed.
                    if (Creature* quarantine = me->FindNearestCreature(NPC_QUARANTINE_MEASURES, 200.0f, true))
                        if (Creature* amalgam = quarantine->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                            CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->UnleashAdds(NPC_ESSENCE_OF_CORRUPTION_N);
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_EXPEL_CORRUPTION:
                            if (me->GetEntry() == NPC_ESSENCE_OF_CORRUPTION_L)
                                DoCast(me, SPELL_EXPEL_CORRUPTION_L);
                            else
                                DoCast(me, SPELL_EXPEL_CORRUPTION);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_essence_of_corruptionAI(creature);
        }
};

// Greater Corruption 72001.
class npc_greater_corruption : public CreatureScript
{
    public:
        npc_greater_corruption() : CreatureScript("npc_greater_corruption") { }

        struct npc_greater_corruptionAI : public ScriptedAI
        {
            npc_greater_corruptionAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            uint32 summonsDead;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                summonsDead = 0;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->AddAura(SPELL_MANIFEST_SPAWN_VIS, me); // Emerge.

                if (Creature* sunGC = me->SummonCreature(NPC_SUN_TENDERHEART_GC, me->GetPositionX() + 3.0f, me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    sunGC->SetPhaseMask(me->GetPhaseMask(), true);
                    sunGC->AI()->AttackStart(me);
			    }
                if (Creature* rookGC = me->SummonCreature(NPC_ROOK_STONETOE_GC, me->GetPositionX() - 3.0f, me->GetPositionY() + 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    rookGC->SetPhaseMask(me->GetPhaseMask(), true);
                    rookGC->AI()->AttackStart(me);
			    }
                if (Creature* levenGC = me->SummonCreature(NPC_LEVEN_DAWNBLADE_GC, me->GetPositionX() - 3.0f, me->GetPositionY() - 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    levenGC->SetPhaseMask(me->GetPhaseMask(), true);
                    levenGC->AI()->AttackStart(me);
			    }

                DoZoneInCombat(me, 100.0f);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->RemoveAurasDueToSpell(SPELL_MANIFEST_SPAWN_VIS);

                events.ScheduleEvent(EVENT_DISHEARTENING_LAUGH, 12000);
                events.ScheduleEvent(EVENT_LINGERING_CORRUPTION, 16000);
                events.ScheduleEvent(EVENT_BOTTOMLESS_PIT, 20000);
            }

            void EnterEvadeMode()
            {
                Reset();
                me->RemoveAllAreaTriggers();
                me->DeleteThreatList();
                me->CombatStop(true);

                me->DespawnOrUnsummon(100);
            }

            void JustDied(Unit* killer)
            {
                me->RemoveAllAreaTriggers();

                if (killer)
                    killer->CastSpell(killer, SPELL_CLEANSE_TRIAL, true);
            }

            void SummonedCreatureDies(Creature* summon, Unit* killer)
            {
                if (summon->GetEntry() == NPC_SUN_TENDERHEART_GC || summon->GetEntry() == NPC_ROOK_STONETOE_GC || summon->GetEntry() == NPC_LEVEN_DAWNBLADE_GC)
                    summonsDead++;
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // If all 3 mobs are dead evade and despawn.
                if (summonsDead == 3)
                    EnterEvadeMode();

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DISHEARTENING_LAUGH:
                            DoCast(me, SPELL_DISHEARTENING_LAUGH);
                            events.ScheduleEvent(EVENT_DISHEARTENING_LAUGH, urand(12000, 14000));
                            break;

                        case EVENT_LINGERING_CORRUPTION:
                            DoCast(me, SPELL_LINGERING_CORRUPTION);
                            events.ScheduleEvent(EVENT_LINGERING_CORRUPTION, urand(16500, 18500));
                            break;

                        case EVENT_BOTTOMLESS_PIT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_BOTTOMLESS_PIT_DUMMY);
                            events.ScheduleEvent(EVENT_LINGERING_CORRUPTION, urand(25000, 30000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_greater_corruptionAI(creature);
        }
};

// Titanic Corruption 72051.
class npc_titanic_corruption : public CreatureScript
{
    public:
        npc_titanic_corruption() : CreatureScript("npc_titanic_corruption") { }

        struct npc_titanic_corruptionAI : public ScriptedAI
        {
            npc_titanic_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->AddAura(SPELL_MANIFEST_SPAWN_VIS, me); // Emerge.
                DoZoneInCombat(me, 100.0f);
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->RemoveAurasDueToSpell(SPELL_MANIFEST_SPAWN_VIS);

                events.ScheduleEvent(EVENT_CORRUPTION, 4000);
                events.ScheduleEvent(EVENT_HURL_CORRUPTION, 21000);
                events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, 12000);
                events.ScheduleEvent(EVENT_PIERCING_CORRUPTION, 17000);
                events.ScheduleEvent(EVENT_TITANIC_SMASH, 7000);
            }

            void EnterEvadeMode()
            {
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);

                me->DespawnOrUnsummon();
            }

            void JustDied(Unit* killer)
            {
                if (killer)
                    killer->CastSpell(killer, SPELL_CLEANSE_TRIAL, true);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CORRUPTION:
                            DoCast(me->getVictim(), SPELL_CORRUPTION);
                            events.ScheduleEvent(EVENT_CORRUPTION, 3000);
                            break;

                        case EVENT_HURL_CORRUPTION:
                            DoCast(me->getVictim(), SPELL_HURL_CORRUPTION);
                            events.ScheduleEvent(EVENT_HURL_CORRUPTION, urand(20000, 23000));
                            break;

                        case EVENT_BURST_OF_CORRUPTION:
                            DoCast(me, SPELL_BURST_OF_CORRUPTION);
                            events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, urand(20000, 23000));
                            break;

                        case EVENT_PIERCING_CORRUPTION:
                            DoCast(me->getVictim(), SPELL_PIERCING_CORRUPTION);
                            events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, urand(17000, 20000));
                            break;

                        case EVENT_TITANIC_SMASH:
                            DoCast(me->getVictim(), SPELL_TITANIC_SMASH);
                            events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, urand(14000, 17000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_titanic_corruptionAI(creature);
        }
};

// Blind Hatred 72565.
class npc_blind_hatred : public CreatureScript
{
    public:
        npc_blind_hatred() : CreatureScript("npc_blind_hatred") { }

        struct npc_blind_hatredAI : public ScriptedAI
        {
            npc_blind_hatredAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            // Function for creating a circle around the boss to move around.
            void FillCirclePath(Position const& centerPos, float radius, float z, Movement::PointsArray& path)
            {
                float step = -M_PI / 8.0f; // Clockwise.
                float angle = centerPos.GetAngle(me->GetPositionX(), me->GetPositionY());

                for (uint8 i = 0; i < 16; angle += step, ++i)
                {
                    G3D::Vector3 point;
                    point.x = centerPos.GetPositionX() + radius * cosf(angle);
                    point.y = centerPos.GetPositionY() + radius * sinf(angle);
                    point.z = z;
                    path.push_back(point);
                }
            }

            void Reset()
            {
                events.Reset();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_BLIND_HATRED_NPC_VIS, me);
                me->DespawnOrUnsummon(33200);

                events.ScheduleEvent(EVENT_MOVE_CIRCLE, 3100);
                events.ScheduleEvent(EVENT_HATRED_TARGET_CHECK, 3450);
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_CIRCLE:
                        {
                            if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 150.0f, true))
                            {
                                Movement::MoveSplineInit init(me);
                                FillCirclePath(quarMeasSpawnPos, me->GetDistance(amalgam), me->GetPositionZ(), init.Path());
                                init.SetWalk(true);
                                init.SetCyclic();
                                init.SetSmooth();
                                init.Launch();
                                me->SetWalk(false);
                            }
                            break;
                        }

                        case EVENT_HATRED_TARGET_CHECK:
                        {
                            if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                            {
                                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                                if (!PlayerList.isEmpty())
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                        if (Player* player = i->getSource())
                                            if (player->IsInBetween(me, amalgam, 2.0f)) // Check for players between the npc and the boss.
                                                player->CastSpell(player, SPELL_BLIND_HATRED_DMG, true);
                            }
                            events.ScheduleEvent(EVENT_HATRED_TARGET_CHECK, 250);
                            break;
                        }

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_blind_hatredAI(creature);
        }
};

// Residual Corruption 72550.
class npc_residual_corruption : public CreatureScript
{
    public:
        npc_residual_corruption() : CreatureScript("npc_residual_corruption") { }

        struct npc_residual_corruptionAI : public ScriptedAI
        {
            npc_residual_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset() { }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
                DoCast(me, SPELL_RESIDUAL_CORUPTION);
            }

            void UpdateAI(uint32 const diff)
            {
                // Check for a valid player, energize him and despawn.
                std::list<Player*> playerList;
                GetPlayerListInGrid(playerList, me, 3.0f);

                if (!playerList.empty())
			    {
                    for (auto player: playerList)
			        {
                        if (player->isAlive() && player->GetPower(POWER_ALTERNATE_POWER) < 75)
			            {
                            player->SetPower(POWER_ALTERNATE_POWER, player->GetPower(POWER_ALTERNATE_POWER) + 25);
                            me->DespawnOrUnsummon(200);
                            break;
			            }
			        }
			    }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_residual_corruptionAI(creature);
        }
};

// Expelled Corruption 74001.
class npc_expelled_corruption : public CreatureScript
{
    public:
        npc_expelled_corruption() : CreatureScript("npc_expelled_corruption") { }

        struct npc_expelled_corruptionAI : public ScriptedAI
        {
            npc_expelled_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            bool toBuff;

            void Reset()
            {
                toBuff = false;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_EXPEL_CORRUPTION_VIS, me);
                if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                    me->GetMotionMaster()->MoveChase(amalgam);
            }

            void UpdateAI(uint32 const diff)
            {
                // Handle Fusion if reaching the boss.
                if (Creature* amalgam = me->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                {
                    if (me->IsWithinDistInMap(amalgam, 2.0f, true) && !toBuff)
                    {
                        me->AddAura(SPELL_FUSION, amalgam);
                        toBuff = true;
                        me->DespawnOrUnsummon(200);
                    }
                }

                // Handle damaging players in the path. They 'absorb' the Expelled Corruption and it despawns.
                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                if (!PlayerList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    {
                        if (Player* player = i->getSource())
                        {
                            if (player->IsWithinDistInMap(me, 1.0f) && me->isInFront(player, M_PI / 3) && !toBuff)
                            {
                                DoCast(player, SPELL_EXPELLED_CORRUPTION_D, true);
                                toBuff = true;
                                me->DespawnOrUnsummon(200);
                            }
                        }
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_expelled_corruptionAI(creature);
        }
};

// Quarantine Measures 72669.
class npc_quarantine_measures : public CreatureScript
{
    public:
        npc_quarantine_measures() : CreatureScript("npc_quarantine_measures") { }

        struct npc_quarantine_measuresAI : public ScriptedAI
        {
            npc_quarantine_measuresAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            /*** Special AI Functions ***/

            // Spawns specific mobs for each Look Within realm phase.
            void SpawnLookWithinMobs(int32 altPower, uint32 specRole, uint32 phaseMask)
            {
                switch (specRole)
                {
                    case ROLES_DPS: // Manifestation + Essence.
                    {
                        // DoCast(me, SPELL_SPAWN_MANIF_L); DoCast(me, SPELL_SPAWN_ESSENCE_L); // Must set in correct phase, spells not helping.
                        if (Creature* manifestation = me->SummonCreature(NPC_MANIFEST_OF_CORRUPTION_L, quarMeasSpawnPos, TEMPSUMMON_MANUAL_DESPAWN))
                            manifestation->SetPhaseMask(phaseMask, true);

                        // Calculate and spawn Essences of Corruption.
                        if (altPower > 40)
                        {
                            uint8 EssencesToSpawn = 1;

                            if (altPower > 55)
                                EssencesToSpawn += 1;
                            if (altPower > 70)
                                EssencesToSpawn += 1;
                            if (altPower > 85)
                                EssencesToSpawn += 1;

                            for (uint8 i = 0; i < EssencesToSpawn; i++)
                                if (Creature* essence = me->SummonCreature(NPC_ESSENCE_OF_CORRUPTION_L, me->GetPositionX() + frand(10.0f, -10.0f), 
                                    me->GetPositionY() + frand(10.0f, -10.0f), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                                    essence->SetPhaseMask(phaseMask, true);
                        }
                        break;
                    }

                    case ROLES_HEALER: // Greater Corruption.
                        // DoCast(me, SPELL_SPAWN_GREATER_CORR); // Must set in correct phase, spell not helping.
                        if (Creature* greaterCorr = me->SummonCreature(NPC_GREATER_CORRUPTION, quarMeasSpawnPos, TEMPSUMMON_MANUAL_DESPAWN))
                            greaterCorr->SetPhaseMask(phaseMask, true);
                        break;

                    case ROLES_TANK: // Titanic Corruption.
                        // DoCast(me, SPELL_SPAWN_TITANIC_CORR); // Must set in correct phase, spell not helping.
                        if (Creature* titanicCorr = me->SummonCreature(NPC_GREATER_CORRUPTION, quarMeasSpawnPos, TEMPSUMMON_MANUAL_DESPAWN))
                            titanicCorr->SetPhaseMask(phaseMask, true);
                        break;

                    default: break;
                }
            }

            /*** General AI Functions ***/

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_QUARANTINE_MEASURES:
                        DoCast(me, SPELL_QUARANTINE_MEASURES);
                        break;

                    default: break;
                }
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_quarantine_measuresAI(creature);
        }
};

// Purifying Light 72065.
class npc_purifying_light_orb : public CreatureScript
{
    public:
        npc_purifying_light_orb() : CreatureScript("npc_purifying_light_orb") { }

        struct npc_purifying_light_orbAI : public ScriptedAI
        {
            npc_purifying_light_orbAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->setFaction(35);
                me->SetReactState(REACT_PASSIVE);

                me->AddAura(SPELL_LOOK_WITHIN_PERIODIC, me);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }

            void OnSpellClick(Unit* clicker)
            {
                if (clicker->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (!clicker->ToPlayer()->GetActiveSpec() || !clicker->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 300.0f, true))
                    return;

                // Retrieve, check and set realms available.
                Creature* amalgam = clicker->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 300.0f, true);
                uint8 realmsAvailable = CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->purifyingRealmsAvailable;

                if (!realmsAvailable) return; // Should never happen as they are equalized to the orb count. Doesn't hurt as a preventive check though.

                // Remove spellclick flag.
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);

                // Check spec and choose what spells to use.
                uint32 specSpell = 0;

                Player* clickerPlayer = clicker->ToPlayer();
                uint32 playerRole = clickerPlayer->GetRoleForGroup(clickerPlayer->GetSpecializationId(clickerPlayer->GetActiveSpec()));

                switch (playerRole)
                {
                    case ROLES_DPS:
                        specSpell = SPELL_TEST_OF_SERENITY_DPS;
                        break;
                    case ROLES_HEALER:
                        specSpell = SPELL_TEST_OF_RELIANCE_HEAL;
                        break;
                    case ROLES_TANK:
                        specSpell = SPELL_TEST_OF_CONFIDENCE_TK;
                        break;

                    default: break;
                }

                // Add Look Within and spec-specific Test aura to the player.
                clicker->CastSpell(clicker, SPELL_LOOK_WITHIN, true);
                clicker->AddAura(specSpell, clicker);

                // Calculate and set the player in the correct phase.
                uint32 playerPhase = 0;
                switch (realmsAvailable)
                {
                    case 5:
                        playerPhase = PHASEMASK_LOOK_WITHIN_REALM1;
                        break;
                    case 4:
                        playerPhase = PHASEMASK_LOOK_WITHIN_REALM2;
                        break;
                    case 3:
                        playerPhase = PHASEMASK_LOOK_WITHIN_REALM3;
                        break;
                    case 2:
                        playerPhase = PHASEMASK_LOOK_WITHIN_REALM4;
                        break;
                    case 1:
                        playerPhase = PHASEMASK_LOOK_WITHIN_REALM5;
                        break;

                    default: break;
                }

                clicker->ToPlayer()->GetPhaseMgr().SetCustomPhase(playerPhase);

                // Decrease available realms count + schedule boss spawning for missing orbs.

                uint8 actualOrb = 0;
                for (uint8 i = 0; i < 5; i++)
                    if (me->GetPositionX() == purifyingLight[i].GetPositionX() && me->GetPositionY() == purifyingLight[i].GetPositionY())
                        actualOrb = i;

                CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->orbsMissing[actualOrb] = true;
                amalgam->AI()->DoAction(ACTION_DECREASE_AVAILABLE_REALM_COUNT);

                // Have Quarantine Measures trigger NPC summon the needed mobs for the phased Test realm.
                if (Creature* quarantine = clicker->FindNearestCreature(NPC_QUARANTINE_MEASURES, 200.0f, true))
                    CAST_AI(npc_quarantine_measures::npc_quarantine_measuresAI, quarantine->AI())->SpawnLookWithinMobs(clicker->GetPower(POWER_ALTERNATE_POWER), playerRole, playerPhase);

                // Despawn.
                me->DespawnOrUnsummon(500);
            }

            void UpdateAI(uint32 const diff) { };
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_purifying_light_orbAI(creature);
        }
};

// Lorewalker Cho 72872.
class npc_lorewalker_cho_norushen : public CreatureScript
{
    public:
        npc_lorewalker_cho_norushen() : CreatureScript("npc_lorewalker_cho_norushen") { }

        struct npc_lorewalker_cho_norushenAI : public ScriptedAI
        {
            npc_lorewalker_cho_norushenAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
                DoAction(ACTION_START_LOREWALKER_INTRO);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case POINT_CHO_MOVE_1:
                        events.ScheduleEvent(EVENT_CHO_MOVE_2, 100);
                        break;

                    case POINT_CHO_MOVE_2:
                        events.ScheduleEvent(EVENT_CHO_MOVE_3, 100);
                        break;

                    case POINT_CHO_MOVE_3:
                        events.ScheduleEvent(EVENT_CHO_MOVE_4, 100);
                        break;

                    case POINT_CHO_MOVE_4:
                        events.ScheduleEvent(EVENT_CHO_MOVE_5, 100);
                        break;

                    case POINT_CHO_MOVE_5: // Nothing to do here.
			            if (Creature* norushen = me->FindNearestCreature(BOSS_NORUSHEN_AMALGAM, 500.0f, true))
                        {
                            if (CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->introStarted == false &&
                                CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->introDone == false &&
                                instance && instance->GetData(DATA_NORUSHEN_EVENT) != DONE)
                            {
                                norushen->AI()->DoAction(ACTION_START_NORUSHEN_INTRO);
                                CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->lorewalkerChoIntro = me;
                                CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->introStarted = true;
                            }
                        }
                        break;

                    default: break;
                }
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_START_LOREWALKER_INTRO:
                        events.ScheduleEvent(EVENT_CHO_MOVE_1, 100);
                        break;

                    default: break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Intro - Lorewalker Cho movement.

                        case EVENT_CHO_MOVE_1:
                            me->GetMotionMaster()->MovePoint(POINT_CHO_MOVE_1, choIntroMove[0]);
                            break;

                        case EVENT_CHO_MOVE_2:
                            me->GetMotionMaster()->MovePoint(POINT_CHO_MOVE_2, choIntroMove[1]);
                            break;

                        case EVENT_CHO_MOVE_3:
                            me->GetMotionMaster()->MovePoint(POINT_CHO_MOVE_3, choIntroMove[2]);
                            break;

                        case EVENT_CHO_MOVE_4:
                            me->GetMotionMaster()->MovePoint(POINT_CHO_MOVE_4, choIntroMove[3]);
                            break;

                        case EVENT_CHO_MOVE_5:
                            me->GetMotionMaster()->MovePoint(POINT_CHO_MOVE_5, choIntroMove[4]);
                            break;

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lorewalker_cho_norushenAI(creature);
        }
};

// Vault of Yshaarj entrance AT 9316.
class at_soo_vault_of_yshaarj_entrance : public AreaTriggerScript
{
    public:
        at_soo_vault_of_yshaarj_entrance() : AreaTriggerScript("at_soo_vault_of_yshaarj_entrance") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (!player)
                return true;

            InstanceScript* instance = player->GetInstanceScript();
            if (!instance)
                return true;

			if (Creature* norushen = player->FindNearestCreature(BOSS_NORUSHEN_AMALGAM, 500.0f, true))
            {
                if (CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->choSummoned == false)
                {
                    // Summon Lorewalker Cho and start the intro.
                    instance->instance->SummonCreature(NPC_LOREWALKER_CHO_NORUSHEN, loreChoSpawnPos);
                    CAST_AI(boss_norushen::boss_norushenAI, norushen->AI())->choSummoned = true;
                }
			}

            return true;
        }
};

/*** Spells ***/

// Corruption (Player bar) 144421.
class spell_amalgam_corruption_bar : public SpellScriptLoader
{
    public:
        spell_amalgam_corruption_bar() : SpellScriptLoader("spell_amalgam_corruption_bar") { }

        class spell_amalgam_corruption_bar_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_amalgam_corruption_bar_AuraScript);

            int32 basePoints;

            bool Load()
            {
                basePoints = 0;
                return true;
            }

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* target = GetTarget();

                if (!target || !GetAura())
                    return;

                Creature* amalgam = target->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 500.0f, true);
                if (!amalgam)
                    return;

                // Handle Tooltip display.
                basePoints = CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->CalculateCorruptionDamageTakenPercent(target);

                if (AuraEffectPtr percent = target->GetAuraEffect(SPELL_CORRUPTION_BAR, EFFECT_1))
                    percent->SetAmount(basePoints);

                // Handle Purified aura increase (127% damage dealt 0 - 25% Corruption).
                if (target->GetPower(POWER_ALTERNATE_POWER) < CORRUPTION_LEVEL_LOW)
                {
                    if (!target->HasAura(SPELL_PURIFIED_DMG_INCREASE))
                        target->AddAura(SPELL_PURIFIED_DMG_INCREASE, target);
                }
                else
                {
                    if (target->HasAura(SPELL_PURIFIED_DMG_INCREASE))
                        target->RemoveAurasDueToSpell(SPELL_PURIFIED_DMG_INCREASE);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_amalgam_corruption_bar_AuraScript::OnTick, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_amalgam_corruption_bar_AuraScript();
        }
};

// Unleashed Anger (Boss cast) 145216.
class spell_amalgam_unleashed_anger : public SpellScriptLoader
{
    public:
        spell_amalgam_unleashed_anger() : SpellScriptLoader("spell_amalgam_unleashed_anger") { }

        class spell_amalgam_unleashed_anger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_amalgam_unleashed_anger_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_UNLEASHED_ANGER_DAMAGE, true);
                caster->AddAura(SPELL_SELF_DOUBT, target); // Handle Self Doubt.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_amalgam_unleashed_anger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_amalgam_unleashed_anger_SpellScript();
        }
};

// Icy Fear (Damage spell) 145735.
class spell_amalgam_icy_fear : public SpellScriptLoader
{
    public:
        spell_amalgam_icy_fear() : SpellScriptLoader("spell_amalgam_icy_fear") { }

        class spell_amalgam_icy_fear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_amalgam_icy_fear_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                SetHitDamage(int32(GetHitDamage() + (GetHitDamage() * ((100 - caster->GetHealthPct()) / 100)))); // Increases by 1% for each 1% boss HP lost.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_amalgam_icy_fear_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_amalgam_icy_fear_SpellScript();
        }
};

// Test of Serenity 144849, Test of Reliance 144850, Test of Confidence 144851.
class spell_test_realm_amalgam : public SpellScriptLoader
{
    public:
        spell_test_realm_amalgam() : SpellScriptLoader("spell_test_realm_amalgam") { }

        class spell_test_realm_amalgam_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_test_realm_amalgam_AuraScript);

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (!(mode & AURA_EFFECT_HANDLE_REAL))
                    return;

                Unit* target = GetTarget();

                if (!target || !GetTargetApplication() || !GetAura())
                    return;

                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;

                // Only on removal by expire. Means the player failed the test.
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                    return;

                // Unleash the remaining mobs.
                uint32 spellId = GetAura()->GetId();

                switch (spellId)
                {
                    // Failure to complete the Reliance challenge will unleash a Manifestation of Corruption to the normal realm.
                    case SPELL_TEST_OF_RELIANCE_HEAL:
                    {
                        std::list<Creature*> unleashList;
                        bool greaterToUnleash = false;

                        GetCreatureListWithEntryInGrid(unleashList, target, NPC_GREATER_CORRUPTION, 200.0f);
                        if (!unleashList.empty())
                            for (auto unleash: unleashList)
                                if (unleash->GetPhaseMask() == target->GetPhaseMask() && !greaterToUnleash)
                                    greaterToUnleash = true;

                        if (greaterToUnleash)
                            if (Creature* quarantine = target->FindNearestCreature(NPC_QUARANTINE_MEASURES, 200.0f, true))
                                if (Creature* amalgam = quarantine->FindNearestCreature(BOSS_AMALGAM_OF_CORRUPTION, 200.0f, true))
                                        CAST_AI(boss_amalgam_of_corruption::boss_amalgam_of_corruptionAI, amalgam->AI())->UnleashAdds(NPC_MANIFEST_OF_CORRUPTION_N);
                        break;
                    }

                    default: break;
                }

                // Remove the realm auras and put the player in the basic phase, Corruption level is kept.
                target->RemoveAurasDueToSpell(SPELL_LOOK_WITHIN);
                target->RemoveAurasDueToSpell(spellId);
				target->SetPhaseMask(PHASEMASK_NORMAL_REALM, true);
            }

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;

                // The player succedeed in cleansing his corruption, and the test is complete.
                if (target->GetPower(POWER_ALTERNATE_POWER) == 0)
                {
                    target->RemoveAurasDueToSpell(SPELL_LOOK_WITHIN);
                    target->RemoveAurasDueToSpell(GetAura()->GetId());
				    target->SetPhaseMask(PHASEMASK_NORMAL_REALM, true);

                    // Purified aura addition.
                    target->CastSpell(target, SPELL_PURIFIED_DMG_IMMUNE, true);
			    }
            }

            void Register() 
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_test_realm_amalgam_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_test_realm_amalgam_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_test_realm_amalgam_AuraScript();
        }
};

// Bottomless Pit (Dummy cast) 146705.
class spell_bottomless_pit : public SpellScriptLoader
{
    public:
        spell_bottomless_pit() : SpellScriptLoader("spell_bottomless_pit") { }

        class spell_bottomless_pit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_bottomless_pit_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_BOTTOMLESS_PIT_AT, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_bottomless_pit_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_bottomless_pit_SpellScript();
        }
};

// Expel Corruption 144479, 145064.
class spell_expel_corruption : public SpellScriptLoader
{
    public:
        spell_expel_corruption() : SpellScriptLoader("spell_expel_corruption") { }

        class spell_expel_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_expel_corruption_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                caster->SummonCreature(NPC_EXPELLED_CORRUPTION, caster->GetPositionX(),caster->GetPositionY(), caster->GetPositionZ() + 1.5f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_expel_corruption_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_expel_corruption_SpellScript();
        }
};

void AddSC_norushen()
{
    new boss_norushen();
    new boss_amalgam_of_corruption();

    new npc_manifestation_of_corruption();
    new npc_essence_of_corruption();
    new npc_greater_corruption();
    new npc_titanic_corruption();

    new npc_blind_hatred();
    new npc_residual_corruption();
    new npc_expelled_corruption();

    new npc_quarantine_measures();
    new npc_purifying_light_orb();

    new npc_lorewalker_cho_norushen();
    new at_soo_vault_of_yshaarj_entrance();

    new spell_amalgam_corruption_bar();
    new spell_amalgam_unleashed_anger();
    new spell_amalgam_icy_fear();
    new spell_test_realm_amalgam();
    new spell_bottomless_pit();
    new spell_expel_corruption();
}
