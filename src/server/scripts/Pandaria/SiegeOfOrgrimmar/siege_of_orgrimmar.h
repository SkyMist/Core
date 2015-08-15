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
 * Description: Header Script.
 */

#ifndef SIEGE_OF_ORGRIMMAR_H_
#define SIEGE_OF_ORGRIMMAR_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define MAX_ENCOUNTERS 14

// Boss encounters
enum Data
{
    // First part
    DATA_IMMERSEUS_EVENT              = 0,
    DATA_FALLEN_PROTECTORS_EVENT      = 1,
    DATA_NORUSHEN_EVENT               = 2,
    DATA_SHA_OF_PRIDE_EVENT           = 3,

    // Second part
    DATA_GALAKRAS_EVENT               = 4,
    DATA_IRON_JUGGERNAUT_EVENT        = 5,
    DATA_KORKRON_DARK_SHAMANS_EVENT   = 6,
    DATA_GENERAL_NAZGRIM_EVENT        = 7,

    // Third part
    DATA_MALKOROK_EVENT               = 8,
    DATA_SPOILS_OF_PANDARIA_EVENT     = 9,
    DATA_THOK_THE_BLOODTHIRSTY_EVENT  = 10,

    // Last part
    DATA_SIEGECRAFTER_BLACKFUSE_EVENT = 11,
    DATA_PARAGONS_OF_THE_KLAXXI_EVENT = 12,
    DATA_GARROSH_HELLSCREAM_EVENT     = 13
};

// GUID retrieval
enum Data64
{
    // First part
    DATA_IMMERSEUS                    = 0,

    DATA_FALLEN_PROTECTOR_1           = 1,
    DATA_FALLEN_PROTECTOR_2           = 2,
    DATA_FALLEN_PROTECTOR_3           = 3,

    DATA_NORUSHEN                     = 4,
    DATA_AMALGAM_OF_CORRUPTION        = 5,
    DATA_SHA_OF_PRIDE                 = 6,

    // Second part
    DATA_GALAKRAS                     = 7,
    DATA_IRON_JUGGERNAUT              = 8,

    DATA_KORKRON_DARK_SHAMAN_1        = 9,
    DATA_KORKRON_DARK_SHAMAN_2        = 10,

    DATA_GENERAL_NAZGRIM              = 11,

    // Third part
    DATA_MALKOROK                     = 12,
    DATA_SPOILS_OF_PANDARIA           = 13,
    DATA_THOK_THE_BLOODTHIRSTY        = 14,

    // Last part
    DATA_SIEGECRAFTER_BLACKFUSE       = 15,

    DATA_PARAGONS_OF_THE_KLAX_1       = 16,
    DATA_PARAGONS_OF_THE_KLAX_2       = 17,
    DATA_PARAGONS_OF_THE_KLAX_3       = 18,
    DATA_PARAGONS_OF_THE_KLAX_4       = 19,
    DATA_PARAGONS_OF_THE_KLAX_5       = 20,
    DATA_PARAGONS_OF_THE_KLAX_6       = 21,
    DATA_PARAGONS_OF_THE_KLAX_7       = 22,
    DATA_PARAGONS_OF_THE_KLAX_8       = 23,
    DATA_PARAGONS_OF_THE_KLAX_9       = 24,

    DATA_GARROSH_HELLSCREAM           = 25
};

enum CreatureIds
{
    // Immerseus
    BOSS_IMMERSEUS                      = 71543,
    NPC_SHA_PUDDLE                      = 71603,
    NPC_CONTAMINATED_PUDDLE             = 71604,

    // Fallen Protectors
    BOSS_ROOK_STONETOE                  = 71475,
    BOSS_HE_SOFTFOOT                    = 71479,
    BOSS_SUN_TENDERHEART                = 71480,

    // Norushen
    BOSS_NORUSHEN_AMALGAM               = 71965,
    BOSS_AMALGAM_OF_CORRUPTION          = 72276,
    NPC_ESSENCE_OF_CORRUPTION           = 71977,
    NPC_MANIFESTATION_OF_CORRUPTION     = 71976,

    // Sha of Pride
    BOSS_NORUSHEN_PRIDE                 = 71967,
    BOSS_SHA_OF_PRIDE                   = 71734,
    NPC_REFLECTION                      = 72172,

    // Galakras
    BOSS_GALAKRAS                       = 72249,
    NPC_MASTER_CANNONEER_DAGRYN         = 72356,
    NPC_HIGH_ENFORCER_THRANOK           = 72355,
    NPC_LIEUTENANT_KRUGRUK              = 72357,
    NPC_KORGRA_THE_SNAKE                = 72456,
    NPC_DRAGONMAW_BONECRUSHER           = 72354,
    
    // Iron Juggernaut
    BOSS_IRON_JUGGERNAUT                = 71466,
    NPC_CRAWLER_MINE                    = 72050,

    // Kor'Kron Dark Shamans
    BOSS_EARTHBREAKER_HAROMM            = 71859,
    BOSS_WAVEBINDER_KARDIS              = 71858,
    NPC_DARKFANG                        = 71923,
    NPC_BLOODCLAW                       = 71923,
    NPC_FOULSTREAM_TOTEM                = 71916,
    NPC_POISONMIST_TOTEM                = 71917,
    NPC_ASHFLARE_TOTEM                  = 71917,
    NPC_FOUL_SLIM                       = 71825,

    // General Nazgrim
    BOSS_GENERAL_NAZGRIM                = 71515,
    NPC_ORGRIMMAR_FAITHFUL              = 71715,
    NPC_KORKRON_IRONBLADE               = 71770,
    NPC_KORKRON_ARCWEAVER               = 71771,
    NPC_KORKRON_ASSASSIN                = 71772,
    NPC_KORKRON_WARSHAMAN               = 71773,

    // Malkorok
    BOSS_MALKOROK                       = 71454,

    // Spoils ofPandaria
    BOSS_SPOILS_OF_PANDARIA             = 71889,
    NPC_MODIFIED_ANIMA_GOLEM            = 71395,
    NPC_MOGU_SHADOW_RITUALIST           = 71393,
    NPC_ZARTHIK_AMBER_PRIEST            = 71397,
    NPC_SETTHIK_WIELDER                 = 71405,
    NPC_ANIMATED_STONE_MOGU             = 71380,
    NPC_BURIAL_URN                      = 71382,
    NPC_QUILEN_GARDIANS                 = 72223,
    NPC_SRITHIK_BOMBARDIER              = 71385,
    NPC_KORTHIK_WARCALLER               = 62754,
    NPC_ANCIENT_BREWMASTER_SPIRIT       = 71427, // Unknown coordinates
    NPC_WISE_MISTEWEAVER_SPIRIT         = 71428, // Unknown coordinates
    NPC_NAMELESSE_WINDWALLKER_SPIRIT    = 71430, // Unknown coordinates

    // Thok the Bloothirsty
    BOSS_THOK_THE_BLOODTHIRSTY          = 71529,
    NPC_KORKRON_JAILER                  = 71658,
    NPC_AKOLIK                          = 71742,
    NPC_WATERSPEAKER_GORAI              = 71749,
    NPC_MONTAK                          = 71763,

    // Siegecrafter Blackfuse
    BOSS_SIEGECRAFTER_BLACKFUSE         = 71504,
    NPC_AUTOMATED_SHREDDER              = 71591,

    // Paragons of the Klaxxi
    BOSS_KILRUK_THE_WIND_REAVER         = 71161,
    BOSS_XARIL_THE_POISONED_MIND        = 71157,
    BOSS_KAZTIK_THE_MANIPULATOR         = 71156,
    BOSS_KORVEN_THE_PRIME               = 71155,
    BOSS_IYYOKUK_THE_LUCID              = 71160,
    BOSS_KAROZ_THE_LOCUST               = 71154,
    BOSS_SKEER_THE_BLOODSEEKER          = 71152,
    BOSS_RIKKAL_THE_DISSECTOR           = 71158,
    BOSS_HISEK_THE_SWARMKEEPER          = 71153,

    // Garrosh Hellscream
    BOSS_GARROSH_HELLSCREAM             = 71865,
    NPC_SIEGE_ENGINEER                  = 71984,
	NPC_WARBRINGER_KORKRON              = 71979,
	NPC_WOLF_RIDER_FARSEER              = 71983
};

// 3 Fallen Protectors.
enum FallenProtectors
{
    ROOK_STONETOE                       = 0,
    HE_SOFTFOOT,
    SUN_TENDERHEART
};

#define MAX_FALLEN_PROTECTORS 3

// 2 Dark Shamans.
enum KorkronDarkShamans
{
    EARTHBREAKER_HAROMM                 = 0,
    WAVEBINDER_KARDIS
};

#define MAX_KORKRON_DARK_SHAMANS 2

// 9 Paragons of The Klaxxi.
enum ParagonsOfTheKlaaxi
{
    KILRUK_THE_WIND_REAVER              = 0,
    XARIL_THE_POISONED_MIND,
    KAZTIK_THE_MANIPULATOR,
    KORVEN_THE_PRIME,
    IYYOKUK_THE_LUCID,
    KAROZ_THE_LOCUST,
    SKEER_THE_BLOODSEEKER,
    RIKKAL_THE_DISSECTOR,
    HISEK_THE_SWARMKEEPER
};

#define MAX_PARAGONS_OF_THE_KLAXXI 9

enum GameObjects
{
};

enum AchievementsData
{
};

#endif // SIEGE_OF_ORGRIMMAR_H_
