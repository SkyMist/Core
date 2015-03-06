/*
*
* SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sue your ass.
*
* Raid: Throne of Thunder.
* Description: Header Script.
*/

#ifndef DEF_THRONE_OF_THUNDER_H
#define DEF_THRONE_OF_THUNDER_H

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define MAX_ENCOUNTERS 13

// Boss encounters
enum Data
{
    // Bosses
	DATA_JIN_ROKH_BREAKER_EVENT      = 0,
	DATA_HORRIDON_EVENT              = 1,
	DATA_COUNCIL_OF_ELDERS_EVENT     = 2,

	DATA_TORTOS_EVENT                = 3,
	DATA_MEGAERA_EVENT               = 4,
    DATA_JI_KUN_EVENT                = 5,

	DATA_DURUMU_THE_FORGOTTEN_EVENT  = 6,
	DATA_PRIMORDIUS_EVENT            = 7,
    DATA_DARK_ANIMUS_EVENT           = 8,

	DATA_IRON_QON_EVENT              = 9,
	DATA_TWIN_CONSORTS_EVENT         = 10,
    DATA_LEI_SHEN_EVENT              = 11,

    DATA_RA_DEN_EVENT                = 12  // Heroic only.
};

// GUID retrieval
enum Data64
{
    // Bosses

	DATA_JIN_ROKH_BREAKER      = 0,
	DATA_HORRIDON              = 1,

    // Council of Elders
	DATA_KAZRAJIN              = 2,
	DATA_SUL_THE_SANDCRAWLER   = 3,
	DATA_FROST_KING_MALAKK     = 4,
	DATA_HIGH_PRIESTESS_MARLI  = 5,

	DATA_TORTOS                = 6,
	DATA_MEGAERA               = 7,
    DATA_JI_KUN                = 8,

	DATA_DURUMU_THE_FORGOTTEN  = 9,
	DATA_PRIMORDIUS            = 10,
    DATA_DARK_ANIMUS           = 11,

	DATA_IRON_QON              = 12,

    // Twin Consorts
	DATA_LU_LIN                = 13,
	DATA_SUEN                  = 14,

    DATA_LEI_SHEN              = 15,

    DATA_RA_DEN                = 16,  // Heroic only.

    // GameObjects

    DATA_FIRST_DOOR            = 17,

    // Jin'Rokh the Breaker
    DATA_MOGU_FOUNTAIN_NE      = 18,
    DATA_MOGU_FOUNTAIN_NW      = 19,
    DATA_MOGU_FOUNTAIN_SE      = 20,
    DATA_MOGU_FOUNTAIN_SW      = 21,

    DATA_JIN_ROKH_FRONT_DOOR   = 22,     // door he is looking at
    DATA_JIN_ROKH_BACK_DOOR    = 23      // door to the next boss
};

enum CreatureIds
{
    BOSS_JIN_ROKH_BREAKER      = 69495,

    BOSS_HORRIDON              = 68476,

    // Council of Elders
    BOSS_KAZRAJIN              = 69134,
    BOSS_SUL_THE_SANDCRAWLER   = 69078,
    BOSS_FROST_KING_MALAKK     = 69131,
    BOSS_HIGH_PRIESTESS_MARLI  = 69132,

    BOSS_TORTOS                = 67977,
    BOSS_MEGAERA               = 68065,
    BOSS_JI_KUN                = 69712,

    BOSS_DURUMU_THE_FORGOTTEN  = 68036,
    BOSS_PRIMORDIUS            = 69017,
    BOSS_DARK_ANIMUS           = 69427,

    BOSS_IRON_QON              = 68078,

    // Twin Consorts
    BOSS_LU_LIN                = 68905,
    BOSS_SUEN                  = 68904,

    BOSS_LEI_SHEN              = 68397,

    BOSS_RA_DEN                = 69473  // Heroic only.
};

enum GameObjectIds
{
    GO_FIRST_DOOR              = 218665,

    // Jin'Rokh the Breaker
    GO_MOGU_FOUNTAIN_NE        = 218678,
    GO_MOGU_FOUNTAIN_NW        = 218675,
    GO_MOGU_FOUNTAIN_SE        = 218677,
    GO_MOGU_FOUNTAIN_SW        = 218676,

    GO_JIN_ROKH_FRONT_DOOR     = 218664,     // door he is looking at
    GO_JIN_ROKH_BACK_DOOR      = 218663,     // door to the next boss
};

#endif // DEF_THRONE_OF_THUNDER_H
