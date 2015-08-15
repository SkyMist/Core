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
 * Boss: Sha of Pride.
 *
 * Wowpedia boss history:
 *
 * "The seventh sha, the Sha of Pride, was the final burden to which Emperor Shaohao clung, shrouding the land in mist and biding its time for millennia.
 *  When Garrosh awakened the Heart of Y'shaarj, the force of his arrogance caused this dark energy to coalesce in the chamber where the Heart was unearthed."
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
#include "MoveSplineInit.h"

#include "siege_of_orgrimmar.h"

/*
Intro:

    Norushen yells: The corruption is amplifying. The fragments must be purged before it becomes too great. 145979 Door Channel.
    Taran Zhu says: *Cough cough* Nyyyhunnnggggg...
    Taran Zhu has survived his encounter with Garrosh, but his wound caused by Gorehowl is serious.
    Lorewalker Cho yells: Taran Zhu!
    Taran Zhu says: Cho... the outsiders... THEY did this... we should never have let them in...
    Lorewalker Cho says: Don't speak, friend, I will take you to the healers.
    Lorewalker Cho yells: I will find help! Try to hold off the sha for as long as you can!
    Cho leads Taran Zhu out the way he came in.

    Norushen yells: It did not matter. It comes. Steel your hearts and prepare your souls.
    Sha of Pride yells: Come, face me. Give in to your pride. Show me your "Greatness". 

Outro:

    Lady Jaina Proudmoore and Lor'themar Theron walk in ahead of Cho.
    Lorewalker Cho yells: Heroes! You're alive. I brought help.
    Cho stops and Jaina and Lor'themar continue crossing the room.
    Lady Jaina Proudmoore says: So Hellscream's arrogance unleashed the last of the sha. I am not surprised.
    Lor'themar Theron yells: Look here! He left his weapon behind.
    Lady Jaina Proudmoore says: Gorehowl.
    Lor'themar Theron says: This means he's completely unhinged.
    Lady Jaina Proudmoore says: News to no one, Regent Lord. King Wrynn's fleet is converging on Orgrimmar as we speak.
    Lor'themar Theron says: Likewise. Sylvanas and I have both sent ships to support Vol'jin's revolution.
    Lady Jaina Proudmoore says: I'm warning you, Lor'themar. The Alliance is besieging the city and we will destroy Hellscream. Your people had best stay out of our way.
    Lor'themar's response has a hint of sarcasm.
    Lor'themar Theron says: It is always a privilege to see you, Lady Proudmoore.
    Jaina and Lor'themar each summon a Portal to Orgrimmar as they walk forward to opposite corners of the room as Cho heads back out through the doorway.
    (Alliance) Lady Jaina Proudmoore says: Come heroes, through the portal! The Siege of Orgrimmar begins!
    (Horde) Lor'themar Theron says: Champions, come with me. It's time to settle grievances with our... "Warchief."
*/

enum Yells
{
    /*** Bosses ***/

    // Norushen.

    SAY_NORUSHEN_INTRO_1           = 0, // The corruption is amplifying. The fragments must be purged before it becomes too great.
    SAY_NORUSHEN_INTRO_2           = 1, // It did not matter. It comes. Steel your hearts and prepare your souls.

    SAY_NORUSHEN_GIFT_OF_THE_TIT   = 2, // 0 - Be humble. ; 1 - Free yourself of arrogance. .
    SAY_NORUSHEN_DEATH             = 3, // You... must... contain... it...

    // Lorewalker Cho Intro.

    SAY_LOREWALKER_INTRO_1         = 0, // Taran Zhu!
    SAY_LOREWALKER_INTRO_2         = 1, // Don't speak, friend, I will take you to the healers.
    SAY_LOREWALKER_INTRO_3         = 2, // I will find help! Try to hold off the sha for as long as you can!

    SAY_LOREWALKER_OUTRO_1         = 3, // Heroes! You're alive. I brought help.

    // Taran Zhu Intro.

    SAY_TARAN_ZHU_INTRO_1          = 0, // *Cough cough* Nyyyhunnnggggg...
    SAY_TARAN_ZHU_INTRO_2          = 1, // Cho... the outsiders... THEY did this... we should never have let them in...

    // Sha of Pride.

    SAY_SHA_OF_PRIDE_INTRO_1       = 0, // Come, face me. Give in to your pride. Show me your "Greatness".

    SAY_SHA_OF_PRIDE_AGGRO         = 1, // So foolish...
    SAY_SHA_OF_PRIDE_KILL          = 2, // 0 - That one is WEAK. ; 1 - That one is unworthy of your group. ; 2 - Blame your companions! .
    SAY_SHA_OF_PRIDE_SELF_REFLECT  = 3, // 0 - You are better than your companions! ; 1 - Your arrogance compels you... .
    SAY_SHA_OF_PRIDE_SWEL_PRIDE    = 4, // 0 - Succumb to your pride! ; 1 - You should bow to no king or warchief. .
    SAY_SHA_OF_PRIDE_CORR_PRISON   = 5, // Your arrogance feeds me!

    SAY_SHA_OF_PRIDE_UNLEASHED     = 6, // You've let your pride cloud your vision, Titan puppet! You can never contain ME!

    // Lady Jaina Proudmoore.

    SAY_JAINA_OUTRO_1              = 0, // So Hellscream's arrogance unleashed the last of the sha. I am not surprised.
    SAY_JAINA_OUTRO_2              = 1, // Gorehowl.
    SAY_JAINA_OUTRO_3              = 2, // News to no one, Regent Lord. King Wrynn's fleet is converging on Orgrimmar as we speak.
    SAY_JAINA_OUTRO_4              = 3, // I'm warning you, Lor'themar. The Alliance is besieging the city and we will destroy Hellscream. Your people had best stay out of our way.
    SAY_JAINA_OUTRO_5              = 4, // (Alliance) Lady Jaina Proudmoore says: Come heroes, through the portal! The Siege of Orgrimmar begins!

    // Lor'themar Theron.

    SAY_THERON_OUTRO_1             = 0, // Look here! He left his weapon behind.
    SAY_THERON_OUTRO_2             = 1, // This means he's completely unhinged.
    SAY_THERON_OUTRO_3             = 2, // Likewise. Sylvanas and I have both sent ships to support Vol'jin's revolution.
    SAY_THERON_OUTRO_4             = 3, // It is always a privilege to see you, Lady Proudmoore.
    SAY_THERON_OUTRO_5             = 4  // (Horde) Lor'themar Theron says: Champions, come with me. It's time to settle grievances with our... "Warchief."
};

enum Spells
{
    /*** Bosses ***/

    // Norushen.

    /* Gift of the Titans 
        Norushen gifts players with immunity to pride for 20 seconds. He will always choose at least one of the healers, and seems to never choose any of the tanks.
        In addition, if all players with Gift of the Titans are within 8 yards of each other, they gain Power of the Titans.
        Power of the Titans increases haste and all damage and healing done by 15%.
    */
    SPELL_GIFT_OF_THE_TITANS_A     = 146595, // Trigger for all checks.
    SPELL_GIFT_OF_THE_TITANS_1     = 144359, // Triggered by the aura.
    SPELL_GIFT_OF_THE_TITANS_2     = 146594, // Triggered by the aura. Same as 1.
    SPELL_GIFT_OF_THE_TITANS_D     = 144363, // Dummy for 8 yard checktriggered by 1 and 2 each sec.

    SPELL_POWER_OF_THE_TITANS      = 144364, // Buff for dmg + healing increase and threat decrease.

    /* Final Gift
        As Norushen dies, he purifies all players, reducing their Pride to zero. 
    */
    SPELL_FINAL_GIFT               = 144854, // Sets Pride to 0 in 300y.

    // Sha of Pride.

    SPELL_SHA_VORTEX_INTRO         = 146024, // Intro Aura - Visual + Trigger.
    SPELL_SHA_VORTEX_INTRO_SUMMON  = 149220, // Intro Aura - Dummy to summon a mob every 4 seconds.
    SPELL_SHA_INTRO_SPAWN_KB       = 149213, // Intro Knockback when Sha emerges.
    SPELL_GOREHOWL_VISUAL          = 146058, // Gorehowl in ground visual.

    /*
    Pride
        Players start the encounter with 0 points of Pride. Whenever hit by an ability from the Sha of Pride or one of its minions, players gain 5 Pride.
        As a player's Pride increases to 25, 50, 75 and 100, Swelling Pride will inflict additional effects on that player.
    */
    SPELL_PRIDE_BAR                = 144343, // Enables Pride power.

    /* Reaching Attack
        The Sha of Pride strikes at a distant target, inflicting 50% of weapon damage as Shadow and increasing Shadow damage taken by 25% for 8 sec.
        The Sha uses this ability when no targets are in melee range. 
    */
    SPELL_REACHING_ATTACK          = 144774,

    /* Mark of Arrogance
        The Sha of Pride marks 2 players, inflicting 70000 Shadow damage every 1 sec, for the remainder of the encounter. This effect stacks.
        This effect is only removed by single target dispels and gives the dispeller 5 Pride when removed.
    */
    SPELL_MARK_OF_ARROGANCE        = 144351,

    /* Corrupted Prison
        The Sha of Pride activates 2 titan prisons, trapping a player inside each. 
        As the prison activates, it releases a burst of Sha energy that inflicts 250000 Shadow damage to all players within 0 yards, knocking them back and giving them 5 Pride.
        Imprisoned players are stunned, suffer 80000 Shadow damage, and gain 5 Pride every second while they remain imprisoned.
        Players remain stunned until all titan locks surrounding the prison have been activated. 
        Titan locks are activated by having a player stand within the lock's rune and remain there until the prison is deactivated. 

        !Note: Usable with GO's.
    */
    SPELL_IMPRISON                 = 144563, // Cast time + Dummy (Effect 0). Boss main spell.
    SPELL_CORRUPTED_PRISON_1       = 144574, // First  prison victim. Teleport (Effect 0), Stun, Periodic dmg, Pacify. Needs script for Pride addition.
    SPELL_CORRUPTED_PRISON_2       = 144636, // Second prison victim. Teleport (Effect 0), Stun, Periodic dmg, Pacify. Needs script for Pride addition.
    SPELL_CORRUPTED_PRISON_3       = 144683, // Third  prison victim (25 man). Teleport (Effect 0), Stun, Periodic dmg, Pacify. Needs script for Pride addition.
    SPELL_CORRUPTED_PRISON_4       = 144684, // Fourth prison victim (25 man). Teleport (Effect 0), Stun, Periodic dmg, Pacify. Needs script for Pride addition.
    SPELL_CORRUPTED_PRISON_ACTIV   = 144615, // Activation knockback and damage (Effect 1). Needs script for Pride addition.

    /* Banishment
        The Sha of Pride banishes random players to the corrupted Sha Realm. 
        Banished players leave behind a physical form of their pride and remain banished until it is destroyed.
        Players within the Sha realm move 50% faster and are compelled to continually run forward, unable to stop.
        Additionally, coming into contact with corruption within the Sha realm inflicts 42000 Shadow damage every second.
        Any player damaged by the Sha realm gains 5 Pride. 
    */
    SPELL_BANISHMENT               = 146823, // Main spell, Cast time, Dummy (Effect 0) for SPELL_BANISHMENT_AURA, Send Event (Effect 1).
    SPELL_BANISHMENT_AURA          = 145215, // Screen Effect, Phase, Move Forward force, Periodic Damage, Trigger spell (Effect 5).
    SPELL_BANISHMENT_AT            = 145217, // Creates 9 AreaTriggers.
    SPELL_BANISHMENT_SIZE          = 145684, // 75% size increase.
    SPELL_BANISHMENT_STUN          = 146623, // Stun.
    SPELL_BANISHMENT_TELEPORT      = 148705, // Teleport visual and effect.

    /* Unstable Corruption
        The Sha's energy tears open Rifts of Corruption every 8 sec. 
        Each rift launches a bolt of corruption at a random player's location every 5 sec.
        Players struck by the bolts suffer 42000 Shadow damage and gain 5 Pride.
        Players can close the rifts, causing them to explode, inflicting 30000 Shadow damage to all players within 8 yards.
        Closing a rift afflicts players with Weakened Resolve, preventing them from closing another rift for 1 min. 
    */
    SPELL_RIFT_OF_CORRUPTION_DUMMY = 147183, // Effect 0 - Periodic Dummy for Spawn Effect. The Sha's energy tears open Rifts of Corruption every 8 sec. 
    SPELL_RIFT_OF_CORRUPTION_VIS   = 147186, // NPC visual. Also 147210 & 147211.
    SPELL_RIFT_OF_CORRUPTION_SPAWN = 147199, // Spawn effect.
    SPELL_BOLT_OF_CORRUPTION_DUMMY = 147389, // Effect 0 - Periodic Trigger. Each rift launches a bolt of corruption at a random player's location every 5 sec.
    SPELL_BOLT_OF_CORRUPTION_MIS   = 147391, // Missile, triggers SPELL_BOLT_OF_CORRUPTION. Triggered by above.
    SPELL_BOLT_OF_CORRUPTION       = 147198, // Damage.
    SPELL_COLLAPSING_RIFT          = 147388, // Players can close the rifts, causing them to explode, 250000 Shadow damage to all players within 8 yards.
    SPELL_WEAKENED_RESOLVE         = 147207, // Dummy (Effect 0), cannot "close" (kill) a rift for 1 minute.

    /* Wounded Pride
        The Sha of Pride wounds his current target for 15 sec. Wounded players gain 5 Pride whenever they suffer melee damage from the Sha of Pride.
    */
    SPELL_WOUNDED_PRIDE            = 144358, // Aura, Dummy Effect 0.

    /* Self Reflection
        The Sha of Pride causes up to 5 players to reflect on their actions, creating a Reflection of Pride at their location.
        With each tick of Self-Reflection, the Sha focuses on players with higher levels of Pride, only targeting those with 25, 50, and then 75 Pride. 
    */
    SPELL_SELF_REFLECTION          = 144800, // Periodic Dummy.
    SPELL_SELF_REFLECTION_VISUAL   = 144784, // Spawn Effect on NPC.
    SPELL_SELF_REFLECTION_EXPLODE  = 144788, // Damage. Needs script for Pride addition.

    /* Unleashed
        When the Sha of Pride reaches 30% health remaining, it becomes Unleashed. With its unleashed power, the Sha focuses on Norushen, instantly killing him.
        The remaining power of the unleashed Sha inflicts 245000 Shadow damage every 10 seconds for the remainder of the encounter.
        Players gain 5 Pride each time they are damaged by Unleashed.
    */
    SPELL_UNLEASHED                = 144832, // Triggers SPELL_UNLEASHED_DMG every 10 seconds.
    SPELL_UNLEASHED_DMG            = 144836, // Damage. Needs script for Pride addition.

    /* Swelling Pride
        When the Sha of Pride reaches 100 energy, it releases a wave of dark energy, inflicting 350000 Shadow damage to all players, giving them 5 Pride.
        In addition, Swelling Pride will trigger a secondary effect on any player with 25 or more Pride.
    */
    SPELL_SWELLING_PRIDE           = 144400, // Damage. Needs script for Pride addition.

    //============================= Swelling Pride effects - Start =============================//

    /* Bursting Pride
        When Swelling Pride hits a player that has 25 to 49 Pride, it creates a mass of Sha corruption at their location.
        After 3 seconds, the corruption explodes, inflicting 300000 Shadow damage to all players within 4 yards.
        Any player damaged by this explosion gains 5 Pride.
    */
    SPELL_BURSTING_PRIDE_MIS       = 144910, // Missile. Triggers SPELL_BURSTING_PRIDE.
    SPELL_BURSTING_PRIDE           = 144911, // Damage. Needs script for Pride addition.

    /* Projection
        When Swelling Pride hits a player that has 50 to 74 Pride, a projection forms 15 yards from their current location.
        Projections explode after 6 seconds, inflicting 225000 Shadow damage to all players, unless the projection's creator is standing within it.
        Any player damaged by a projection's explosion gains 5 Pride.
    */
    SPELL_PROJECTION_DUMMY         = 146822, // 6 second player aura.
    SPELL_PROJECTION_EXPLODE       = 145320, // Damage. Needs script for Pride addition.

    /* Aura of Pride
        When Swelling Pride hits a player that has 75 to 99 Pride, it afflicts them with Aura of Pride for 25 sec.
        This effect causes the player to inflict 11750 Shadow damage to allies within 4.5 yards every 1 sec.
    */
    SPELL_AURA_OF_PRIDE            = 146817, // Triggers SPELL_AURA_OF_PRIDE_DMG each sec.
    SPELL_AURA_OF_PRIDE_DMG        = 146818, // Damage. Needs script for Pride addition.

    /* Overcome
        When players reach 100 Pride, they are Overcome, increasing their damage and healing done by 50%.
        Players hit by Swelling Pride while Overcome are permanently mind controlled and have their health increased by 100%.
    */
    SPELL_OVERCOME                 = 144843, // Dmg / Healing increase, Morph.
    SPELL_OVERCOME_CHARMED         = 144863, // MC, max health increase.

    //============================= Swelling Pride effects - End   =============================//

    /*** Adds ***/

    // Manifestation of Pride.

    /* Mocking Blast
        Manifestations of Pride blast a random player, inflicting 225000 Shadow damage, giving them 5 Pride. 
    */
    SPELL_MOCKING_BLAST            = 144379, // Damage. Needs script for Pride addition.

    /* Last Word
        When a Manifestation of Pride dies, it gives 5 Pride to the 2 closest players.
    */
    SPELL_LAST_WORD                = 144370, // 500y Dummy.
};

enum Events
{
    /*** Bosses ***/

    // Intro - Controller: Norushen.

    EVENT_INTRO_1                  = 1,
    EVENT_INTRO_2,
    EVENT_INTRO_3,
    EVENT_INTRO_4,
    EVENT_INTRO_5,
    EVENT_INTRO_6,
    EVENT_INTRO_7,
    EVENT_INTRO_8,
    EVENT_INTRO_9,

    // Outro - Controller: Lady Jaina Proudmoore.

    EVENT_OUTRO_1,
    EVENT_OUTRO_2,
    EVENT_OUTRO_3,
    EVENT_OUTRO_4,
    EVENT_OUTRO_5,
    EVENT_OUTRO_6,
    EVENT_OUTRO_7,
    EVENT_OUTRO_8,
    EVENT_OUTRO_9,
    EVENT_OUTRO_10,
    EVENT_OUTRO_11,

    // Norushen.

    // Sha of Pride.

    /*** Adds ***/
};

enum Actions
{
    /*** Bosses ***/

    // Intro - Controller: Norushen.

    ACTION_START_SHA_PRIDE_INTRO  = 1,

    // Outro - Controller: Lady Jaina Proudmoore.

    ACTION_START_SHA_PRIDE_OUTRO,

    /*** Adds ***/
};

enum Npcs
{
    /*** Bosses ***/

    // Sha of Pride.

    NPC_LOREWALKER_CHO_NORUSHEN    = 72872,
    NPC_THARAN_ZHU_PRIDE           = 72779,

    NPC_MANIFESTATION_OF_PRIDE     = 72280, // From Banishment.
    NPC_SELF_REFLECTION            = 72172, // From Self Reflection.
    NPC_RIFT_OF_CORRUPTION         = 72846, // From Unstable Corruption.

    /*** Adds ***/

    // Intro / Outro.

    NPC_JAINA_PROUDMOORE_PRIDE     = 73598,
    NPC_LORTHEMAR_THERON_PRIDE     = 73605
};

enum GOs
{
    /*** Bosses ***/

    // Sha of Pride.

    // Shadow Prison - Each composed of 3 'parts', Game Objects which are blue by default and when stepped on by a player get Orange and activate.

    GO_N_PRISON_FLOOR              = 222682, // X: 772.8920 Y: 1096.243 Z: 355.6587, Rotation: X: 0 Y: 0 Z: -0.6293201 W: 0.777146. O 4.92183.
    GO_S_PRISON_FLOOR              = 222680, // X: 722.3790 Y: 1128.177 Z: 355.6587, Rotation: X: 0 Y: 0 Z: -0.6293201 W: 0.777146. O 4.92183.
    GO_E_PRISON_FLOOR              = 222683, // X: 731.6327 Y: 1087.473 Z: 355.6587, Rotation: X: 0 Y: 0 Z: -0.6293201 W: 0.777146. O 4.92183.
    GO_W_PRISON_FLOOR              = 222681, // X: 764.2142 Y: 1137.069 Z: 355.6587, Rotation: X: 0 Y: 0 Z: -0.6293201 W: 0.777146. O 4.92183.
    GO_PRISON_FLOOR                = 222679, // X: 747.9799 Y: 1112.775 Z: 356.0117, Rotation: X: 0 Y: 0 Z: -0.6293201 W: 0.777146. O 4.92183.

    // North

    GO_NORTH_PRISON_A              = 221755, // X: 772.9168 Y: 1096.363 Z: 354.6127, Rotation: X: 0 Y: 0 Z: -0.4656143 W: 0.884987. O 5.31453.
    GO_NORTH_PRISON_B              = 221750, // X: 772.8506 Y: 1096.849 Z: 354.6127, Rotation: X: 0 Y: 0 Z:  0.5336142 W: 0.845728. O 1.12574.
    GO_NORTH_PRISON_C              = 221754, // X: 772.4857 Y: 1096.761 Z: 354.6127, Rotation: X: 0 Y: 0 Z: -0.9992285 W: 0.039274. O 3.22016.

    // South

    GO_SOUTH_PRISON_A              = 221761, // X: 723.0390 Y: 1129.091 Z: 354.6127, Rotation: X: 0 Y: 0 Z: 0.8849869 W: 0.4656160. O 2.17293.
    GO_SOUTH_PRISON_B              = 221760, // X: 723.4701 Y: 1128.692 Z: 354.6127, Rotation: X: 0 Y: 0 Z: 0.0392599 W: 0.9992291. O 0.07854.
    GO_SOUTH_PRISON_C              = 221756, // X: 723.1052 Y: 1128.604 Z: 354.6127, Rotation: X: 0 Y: 0 Z: -0.845727 W: 0.5336158. O 4.26733.

    // East

    GO_EAST_PRISON_A               = 221753, // X: 731.9785 Y: 1088.212 Z: 354.6127, Rotation: X: 0 Y: 0 Z: 0.7343225 W: 0.6788007. O 1.649336.
    GO_EAST_PRISON_B               = 221751, // X: 731.5802 Y: 1087.781 Z: 354.6127, Rotation: X: 0 Y: 0 Z: -0.955020 W: 0.2965415. O 3.743731.
    GO_EAST_PRISON_C               = 221752, // X: 732.0667 Y: 1087.847 Z: 354.6127, Rotation: X: 0 Y: 0 Z: -0.220697 W: 0.9753423. O 5.838127.

    // West

    GO_WEST_PRISON_A               = 221758, // X: 764.3764 Y: 1137.689 Z: 354.6127, Rotation: X: 0 Y: 0 Z: 0.2965412 W: 0.9550201. O 0.60214.
    GO_WEST_PRISON_B               = 221759, // X: 763.8900 Y: 1137.623 Z: 354.6127, Rotation: X: 0 Y: 0 Z: 0.9753418 W: 0.2206997. O 2.69653.
    GO_WEST_PRISON_C               = 221757, // X: 763.9813 Y: 1137.247 Z: 354.6129, Rotation: X: 0 Y: 0 Z: -0.678800 W: 0.7343227. O 4.79093.

    /*** Adds ***/
};

// Corrupted Prisons count, depending on difficulty.
enum PrisonsCount
{
    SP_PRISONS_COUNT_10MAN         = 2,
    SP_PRISONS_COUNT_25MAN         = 4
};

// Corrupted Prisons id.
enum Prisons
{
    PRISON_NORTH                   = 1,
    PRISON_SOUTH,
    PRISON_WEST,
    PRISON_EAST
};

enum MovementPoints
{
    POINT_NORUSHEN_MOVE_1          = 1,
    POINT_NORUSHEN_MOVE_2          = 2,
    POINT_NORUSHEN_MOVE_3          = 3,
    POINT_NORUSHEN_MOVE_4          = 4,  // Chamber entrance.
    POINT_NORUSHEN_MOVE_5          = 5,  // Encounter.

    POINT_ZHU_MOVE_6               = 6,  // Spawn.
    POINT_ZHU_MOVE_7               = 7,  // Despawn.

    POINT_CHO_MOVE_8               = 8,  // Chamber entrance.
    POINT_CHO_MOVE_9               = 9,  // Zhu.
    POINT_CHO_MOVE_10              = 10, // Despawn.
    POINT_CHO_MOVE_11              = 11, // Outro.

    POINT_JL_MOVE_10               = 12, // Jaina / Lor'themar outro 1.
    POINT_JL_MOVE_11               = 13, // Jaina / Lor'themar outro 2.
    POINT_JL_MOVE_12               = 14, // Jaina / Lor'themar outro 3.
    POINT_JL_MOVE_13               = 15, // Jaina / Lor'themar outro 4.
    POINT_JL_MOVE_14               = 16, // Jaina / Lor'themar outro 5.
};

// Tharan Zhu spawn position.
Position const tharanZhuSpawnPos   = { 780.412f, 1017.587f, 356.062f };

// Sha of Pride spawn position.
Position const shaPrideSpawnPos    = { 749.194f, 1112.641f, 357.314f };

// Norushen movement positions.
Position const NorushenMove[5]     =
{
    { 768.722f, 1015.379f, 356.073f }, // Stair before door.
    { 767.805f, 1019.000f, 357.101f }, // Up the stair.
    { 761.164f, 1049.889f, 357.151f }, // Front of entrance.
    { 760.357f, 1051.784f, 356.072f }, // Entrance.
    { 759.110f, 1060.828f, 356.072f }, // Encounter.
};

/*** Bosses ***/

// Norushen 71967.
class boss_norushen_pride : public CreatureScript
{
    public:
        boss_norushen_pride() : CreatureScript("boss_norushen_pride") { }

        struct boss_norushen_prideAI : public ScriptedAI
        {
            boss_norushen_prideAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            Creature* lorewalkerChoIntro;
            Creature* tharanZhuIntro;

            void Reset()
            {
                events.Reset();
                lorewalkerChoIntro = NULL;
                DoAction(ACTION_START_SHA_PRIDE_INTRO);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_START_SHA_PRIDE_INTRO:
                        events.ScheduleEvent(EVENT_INTRO_1, 100);
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
                        // Intro.

                        case EVENT_INTRO_1:

                            // Define the actors.
                            if (Creature* loreWalker = me->FindNearestCreature(NPC_LOREWALKER_CHO_NORUSHEN, 100.0f, true))
                                lorewalkerChoIntro = loreWalker;
                            if (Creature* tharanZhu = me->SummonCreature(NPC_THARAN_ZHU_PRIDE, tharanZhuSpawnPos, TEMPSUMMON_MANUAL_DESPAWN))
                                tharanZhuIntro = tharanZhu;

                            // Go to the entrance.
                            me->GetMotionMaster()->MovePoint(POINT_NORUSHEN_MOVE_1, NorushenMove[0]);

                            events.ScheduleEvent(EVENT_INTRO_2, 8000);
                            break;

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_norushen_prideAI(creature);
        }
};

// Sha of Pride 71734.
class boss_sha_of_pride : public CreatureScript
{
    public:
        boss_sha_of_pride() : CreatureScript("boss_sha_of_pride") { }

        struct boss_sha_of_prideAI : public BossAI
        {
            boss_sha_of_prideAI(Creature* creature) : BossAI(creature, DATA_SHA_OF_PRIDE_EVENT)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            bool prisonActivated[4];

            /*** Special AI Functions ***/

            // Used to retrieve a single part (1 / 3) of a Prison.
            GameObject* GetPrisonTile(uint32 prisonGOId)
            {
                std::list<GameObject*> wallsList;

                GetGameObjectListWithEntryInGrid(wallsList, me, prisonGOId, 300.0f);

                if (!wallsList.empty())
                    return wallsList.front();
            }

            // Retrieve the player put in a certain Prison.
            Player* GetImprisonedPlayer(uint32 prisonId)
            {
                Player* neededPlayer = NULL;
                uint32 spellId = 0;

                switch (prisonId)
                {
                    case PRISON_NORTH:
                        spellId = SPELL_CORRUPTED_PRISON_1;
                        break;

                    case PRISON_SOUTH:
                        spellId = SPELL_CORRUPTED_PRISON_2;
                        break;

                    case PRISON_WEST:
                        spellId = SPELL_CORRUPTED_PRISON_3;
                        break;

                    case PRISON_EAST:
                        spellId = SPELL_CORRUPTED_PRISON_4;
                        break;

                    default: break;
                }

                // Check for errors / wrong calls and return NULL.
                if (spellId == 0)
                    return neededPlayer;

                // Now check and retrieve the player having the specific prison aura.
                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                if (!PlayerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        if (Player* player = i->getSource())
                            if (player->HasAura(spellId))
                                neededPlayer = player;

                // Return what we found.
                return neededPlayer;
            }

            // Used to set the state of a single part (1 / 3) of a Prison.
            void ActivatePrisonTile(bool active, uint32 prisonGOId)
            {
                std::list<GameObject*> wallsList;

                GetGameObjectListWithEntryInGrid(wallsList, me, prisonGOId, 300.0f);

                if (!wallsList.empty())
                    for (std::list<GameObject*>::iterator walls = wallsList.begin(); walls != wallsList.end(); walls++)
                        (*walls)->SetGoState(active ? GO_STATE_READY : GO_STATE_ACTIVE);
            }

            // Used to set the state of a whole Prison.
            void ActivatePrison(uint32 prisonId)
            {
                switch (prisonId)
                {
                    case PRISON_NORTH:
                        ActivatePrisonTile(true, GO_N_PRISON_FLOOR);
                        ActivatePrisonTile(true, GO_NORTH_PRISON_A);
                        ActivatePrisonTile(true, GO_NORTH_PRISON_B);
                        ActivatePrisonTile(true, GO_NORTH_PRISON_C);
                        break;

                    case PRISON_SOUTH:
                        ActivatePrisonTile(true, GO_S_PRISON_FLOOR);
                        ActivatePrisonTile(true, GO_SOUTH_PRISON_A);
                        ActivatePrisonTile(true, GO_SOUTH_PRISON_B);
                        ActivatePrisonTile(true, GO_SOUTH_PRISON_C);
                        break;

                    case PRISON_WEST:
                        ActivatePrisonTile(true, GO_W_PRISON_FLOOR);
                        ActivatePrisonTile(true, GO_WEST_PRISON_A);
                        ActivatePrisonTile(true, GO_WEST_PRISON_B);
                        ActivatePrisonTile(true, GO_WEST_PRISON_C);
                        break;

                    case PRISON_EAST:
                        ActivatePrisonTile(true, GO_E_PRISON_FLOOR);
                        ActivatePrisonTile(true, GO_EAST_PRISON_A);
                        ActivatePrisonTile(true, GO_EAST_PRISON_B);
                        ActivatePrisonTile(true, GO_EAST_PRISON_C);
                        break;

                    default: break;
                }

                prisonActivated[prisonId - 1] = true;
            }

            void DeactivatePrison(uint32 prisonId)
            {
                switch (prisonId)
                {
                    case PRISON_NORTH:
                        ActivatePrisonTile(false, GO_N_PRISON_FLOOR);
                        ActivatePrisonTile(false, GO_NORTH_PRISON_A);
                        ActivatePrisonTile(false, GO_NORTH_PRISON_B);
                        ActivatePrisonTile(false, GO_NORTH_PRISON_C);
                        break;

                    case PRISON_SOUTH:
                        ActivatePrisonTile(false, GO_S_PRISON_FLOOR);
                        ActivatePrisonTile(false, GO_SOUTH_PRISON_A);
                        ActivatePrisonTile(false, GO_SOUTH_PRISON_B);
                        ActivatePrisonTile(false, GO_SOUTH_PRISON_C);
                        break;

                    case PRISON_WEST:
                        ActivatePrisonTile(false, GO_W_PRISON_FLOOR);
                        ActivatePrisonTile(false, GO_WEST_PRISON_A);
                        ActivatePrisonTile(false, GO_WEST_PRISON_B);
                        ActivatePrisonTile(false, GO_WEST_PRISON_C);
                        break;

                    case PRISON_EAST:
                        ActivatePrisonTile(false, GO_E_PRISON_FLOOR);
                        ActivatePrisonTile(false, GO_EAST_PRISON_A);
                        ActivatePrisonTile(false, GO_EAST_PRISON_B);
                        ActivatePrisonTile(false, GO_EAST_PRISON_C);
                        break;

                    default: break;
                }

                prisonActivated[prisonId - 1] = false;
            }

            // UpdateAI method for automatic updating of Prisons & their parts.
            void CheckAndSetPrisonTiles()
            {
                // Check active prisons.

                bool canCheckNorth = false;
                bool canCheckSouth = false;
                bool canCheckWest  = false;
                bool canCheckEast  = false;

                if (prisonActivated[PRISON_NORTH - 1] == true)
                    canCheckNorth = true;

                if (prisonActivated[PRISON_SOUTH - 1] == true)
                    canCheckSouth = true;

                if (prisonActivated[PRISON_WEST - 1]  == true)
                    canCheckWest = true;

                if (prisonActivated[PRISON_EAST - 1]  == true)
                    canCheckEast = true;

                // North Prison.
                if (canCheckNorth)
                {
                    bool mustDisableNorth = false;

                    bool NorthATileActive = (GetPrisonTile(GO_NORTH_PRISON_A)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool NorthBTileActive = (GetPrisonTile(GO_NORTH_PRISON_B)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool NorthCTileActive = (GetPrisonTile(GO_NORTH_PRISON_C)->GetGoState() == GO_STATE_READY) ? true : false;

                    // A Tile.
                    if (NorthATileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_NORTH_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_NORTH_PRISON_A);
                            NorthATileActive = false;
                        }
                    }
                    else if (!NorthATileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_NORTH_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_NORTH_PRISON_A);
                            NorthATileActive = true;
                        }
                    }

                    // B Tile.
                    if (NorthBTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_NORTH_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_NORTH_PRISON_B);
                            NorthBTileActive = false;
                        }
                    }
                    else if (!NorthBTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_NORTH_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_NORTH_PRISON_B);
                            NorthBTileActive = true;
                        }
                    }

                    // C Tile.
                    if (NorthCTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_NORTH_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_NORTH_PRISON_C);
                            NorthCTileActive = false;
                        }
                    }
                    else if (!NorthCTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_NORTH_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_NORTH_PRISON_C);
                            NorthCTileActive = true;
                        }
                    }

                    // Check if the Prison should be disabled (all 3 Tiles active).
                    if (NorthATileActive && NorthBTileActive && NorthCTileActive)
                        mustDisableNorth = true;

                    // If the conditions are met, disable the Prison and remove the victim aura.
                    if (mustDisableNorth)
                    {
                        DeactivatePrison(PRISON_NORTH);
                        if (Player* imprisonedPlayer = GetImprisonedPlayer(PRISON_NORTH))
                            imprisonedPlayer->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_1);
                    }
                }

                // South Prison.
                if (canCheckSouth)
                {
                    bool mustDisableSouth = false;

                    bool SouthATileActive = (GetPrisonTile(GO_SOUTH_PRISON_A)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool SouthBTileActive = (GetPrisonTile(GO_SOUTH_PRISON_B)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool SouthCTileActive = (GetPrisonTile(GO_SOUTH_PRISON_C)->GetGoState() == GO_STATE_READY) ? true : false;

                    // A Tile.
                    if (SouthATileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_SOUTH_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_SOUTH_PRISON_A);
                            SouthATileActive = false;
                        }
                    }
                    else if (!SouthATileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_SOUTH_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_SOUTH_PRISON_A);
                            SouthATileActive = true;
                        }
                    }

                    // B Tile.
                    if (SouthBTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_SOUTH_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_SOUTH_PRISON_B);
                            SouthBTileActive = false;
                        }
                    }
                    else if (!SouthBTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_SOUTH_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_SOUTH_PRISON_B);
                            SouthBTileActive = true;
                        }
                    }

                    // C Tile.
                    if (SouthCTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_SOUTH_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_SOUTH_PRISON_C);
                            SouthCTileActive = false;
                        }
                    }
                    else if (!SouthCTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_SOUTH_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_SOUTH_PRISON_C);
                            SouthCTileActive = true;
                        }
                    }

                    // Check if the Prison should be disabled (all 3 Tiles active).
                    if (SouthATileActive && SouthBTileActive && SouthCTileActive)
                        mustDisableSouth = true;

                    // If the conditions are met, disable the Prison and remove the victim aura.
                    if (mustDisableSouth)
                    {
                        DeactivatePrison(PRISON_SOUTH);
                        if (Player* imprisonedPlayer = GetImprisonedPlayer(PRISON_SOUTH))
                            imprisonedPlayer->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_2);
                    }
                }

                // West Prison.
                if (canCheckWest)
                {
                    bool mustDisableWest = false;

                    bool WestATileActive = (GetPrisonTile(GO_WEST_PRISON_A)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool WestBTileActive = (GetPrisonTile(GO_WEST_PRISON_B)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool WestCTileActive = (GetPrisonTile(GO_WEST_PRISON_C)->GetGoState() == GO_STATE_READY) ? true : false;

                    // A Tile.
                    if (WestATileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_WEST_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_WEST_PRISON_A);
                            WestATileActive = false;
                        }
                    }
                    else if (!WestATileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_WEST_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_WEST_PRISON_A);
                            WestATileActive = true;
                        }
                    }

                    // B Tile.
                    if (WestBTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_WEST_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_WEST_PRISON_B);
                            WestBTileActive = false;
                        }
                    }
                    else if (!WestBTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_WEST_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_WEST_PRISON_B);
                            WestBTileActive = true;
                        }
                    }

                    // C Tile.
                    if (WestCTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_WEST_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_WEST_PRISON_C);
                            WestCTileActive = false;
                        }
                    }
                    else if (!WestCTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_WEST_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_WEST_PRISON_C);
                            WestCTileActive = true;
                        }
                    }

                    // Check if the Prison should be disabled (all 3 Tiles active).
                    if (WestATileActive && WestBTileActive && WestCTileActive)
                        mustDisableWest = true;

                    // If the conditions are met, disable the Prison and remove the victim aura.
                    if (mustDisableWest)
                    {
                        DeactivatePrison(PRISON_WEST);
                        if (Player* imprisonedPlayer = GetImprisonedPlayer(PRISON_WEST))
                            imprisonedPlayer->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_3);
                    }
                }

                // East Prison.
                if (canCheckEast)
                {
                    bool mustDisableEast = false;

                    bool EastATileActive = (GetPrisonTile(GO_EAST_PRISON_A)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool EastBTileActive = (GetPrisonTile(GO_EAST_PRISON_B)->GetGoState() == GO_STATE_READY) ? true : false;
                    bool EastCTileActive = (GetPrisonTile(GO_EAST_PRISON_C)->GetGoState() == GO_STATE_READY) ? true : false;

                    // A Tile.
                    if (EastATileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_EAST_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_EAST_PRISON_A);
                            EastATileActive = false;
                        }
                    }
                    else if (!EastATileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_EAST_PRISON_A)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_EAST_PRISON_A);
                            EastATileActive = true;
                        }
                    }

                    // B Tile.
                    if (EastBTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_EAST_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_EAST_PRISON_B);
                            EastBTileActive = false;
                        }
                    }
                    else if (!EastBTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_EAST_PRISON_B)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_EAST_PRISON_B);
                            EastBTileActive = true;
                        }
                    }

                    // C Tile.
                    if (EastCTileActive) // Enabled / Active.
                    {
                        if (!GetPrisonTile(GO_EAST_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(false, GO_EAST_PRISON_C);
                            EastCTileActive = false;
                        }
                    }
                    else if (!EastCTileActive) // Disabled / Inactive.
                    {
                        if (GetPrisonTile(GO_EAST_PRISON_C)->FindNearestPlayer(1.0f, true))
                        {
                            ActivatePrisonTile(true, GO_EAST_PRISON_C);
                            EastCTileActive = true;
                        }
                    }

                    // Check if the Prison should be disabled (all 3 Tiles active).
                    if (EastATileActive && EastBTileActive && EastCTileActive)
                        mustDisableEast = true;

                    // If the conditions are met, disable the Prison and remove the victim aura.
                    if (mustDisableEast)
                    {
                        DeactivatePrison(PRISON_EAST);
                        if (Player* imprisonedPlayer = GetImprisonedPlayer(PRISON_EAST))
                            imprisonedPlayer->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_4);
                    }
                }
            }

            /*** General AI Functions ***/

            void Reset()
            {
                events.Reset();

                for (uint8 i = 0; i < 4; i++)
                    prisonActivated[i] = false;
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_START_SHA_PRIDE_INTRO:
                        events.ScheduleEvent(EVENT_INTRO_1, 100);
                        break;

                    default: break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sha_of_prideAI(creature);
        }
};

/*** Adds ***/

// Manifestation of Pride 72280.
class npc_manifestation_of_pride : public CreatureScript
{
    public:
        npc_manifestation_of_pride() : CreatureScript("npc_manifestation_of_pride") { }

        struct npc_manifestation_of_prideAI : public ScriptedAI
        {
            npc_manifestation_of_prideAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_manifestation_of_prideAI(creature);
        }
};

// Self Reflection 72172.
class npc_self_reflection : public CreatureScript
{
    public:
        npc_self_reflection() : CreatureScript("npc_self_reflection") { }

        struct npc_self_reflectionAI : public ScriptedAI
        {
            npc_self_reflectionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_self_reflectionAI(creature);
        }
};

// Rift of Corruption 72846.
class npc_rift_of_corruption : public CreatureScript
{
    public:
        npc_rift_of_corruption() : CreatureScript("npc_rift_of_corruption") { }

        struct npc_rift_of_corruptionAI : public ScriptedAI
        {
            npc_rift_of_corruptionAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rift_of_corruptionAI(creature);
        }
};

/*** Others ***/

// Jaina Proudmoore 73598.
class npc_jaina_proudmoore_pride : public CreatureScript
{
    public:
        npc_jaina_proudmoore_pride() : CreatureScript("npc_jaina_proudmoore_pride") { }

        struct npc_jaina_proudmoore_prideAI : public ScriptedAI
        {
            npc_jaina_proudmoore_prideAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
                DoAction(ACTION_START_SHA_PRIDE_OUTRO);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_START_SHA_PRIDE_OUTRO:
                        events.ScheduleEvent(EVENT_OUTRO_1, 100);
                        break;

                    default: break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_jaina_proudmoore_prideAI(creature);
        }
};

void AddSC_sha_of_pride()
{
    new boss_norushen_pride();
    new boss_sha_of_pride();

    new npc_manifestation_of_pride();
    new npc_self_reflection();
    new npc_rift_of_corruption();

    new npc_jaina_proudmoore_pride();
}
