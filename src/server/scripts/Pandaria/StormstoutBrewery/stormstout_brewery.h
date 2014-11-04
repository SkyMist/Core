/*
    Dungeon : Stormstout Brewery 85-87
    Instance General Script
*/

#ifndef DEF_STORMSTOUT_BREWERY_H_
#define DEF_STORMSTOUT_BREWERY_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define MAX_ENCOUNTERS 3

#define HOZEN_KILLS_NEEDED 40 // Hozen kills needed to spawn Ook-Ook.

enum DataTypes // Events / Encounters.
{
	DATA_OOKOOK_EVENT               = 0,
    DATA_HOPTALLUS_EVENT            = 1,
    DATA_YANZHU_THE_UNCASKED_EVENT  = 2,

    // For Ook-Ook Hozen.
    DATA_HOZEN_KILLED               = 3
};

enum Data     // GUID handling.
{
	DATA_OOKOOK                     = 0,
    DATA_HOPTALLUS                  = 1,
    DATA_YANZHU_THE_UNCASKED        = 2
};

enum InstanceSpells
{
    SPELL_BANANA_BAR        = 107297, // Alt Power bar for Hozen disrupted (summon Ook-Ook bar, 40).
    SPELL_ANCESTRAL_BREWM_V = 113124  // Ancestral Bewmaster cosmetic.
};

enum CreaturesIds
{
    // Bosses
    BOSS_OOKOOK                     = 56637,
	BOSS_HOPTALLUS                  = 56717,
    BOSS_YANZHU_THE_UNCASKED        = 59479,

    // NPCs

    NPC_AUNTIE_STORMSTOUT           = 59822,
    NPC_CHEN_STORMSTOUT_ENTRANCE    = 59704,
    NPC_CHEN_STORMSTOUT_YANZHU      = 64361,
    NPC_ANCESTRAL_BREWMASTER        = 59075, // Friendly.

    // - Ook-Ook.
    NPC_OOK_BARREL                  = 56682,

    // - Hoptallus.
    NPC_HOPPER                      = 59464,
    NPC_HOPPLING                    = 60208,
    NPC_BOPPER                      = 59551,
    NPC_BOPPER_HAMMER               = 59539, // Hammer dropped by Boppers.

    // Yan-zhu the Uncasked
    NPC_YEASTY_BREW_ALEMENTAL_Y     = 66413,	
    NPC_FIZZY_BUBBLE                = 59799,
    NPC_WALL_OF_SUDS                = 59512,
    NPC_BUBBLE_SHIELD               = 65522,

    // - Instance.
    NPC_BEER_BARREL_BUNNY           = 66215, // Npc on the ground (Beam cast on).
    NPC_LEAKING_BEER_BARREL         = 73186, // Npc on beer barrel.

    NPC_AQUA_DANCER                 = 56865,
    NPC_FIERY_TRICKSTER             = 56867,
    NPC_SODDEN_HOZEN_BRAWLER        = 59605,
    NPC_INFLAMED_HOZEN_BRAWLER      = 56924,
    NPC_SLEEPY_HOZEN_BRAWLER        = 56863,
    NPC_DRUNKEN_HOZEN_BRAWLER       = 56862,
    NPC_HOZEN_BOUNCER               = 56849,
    NPC_HOZEN_PARTY_ANIMAL          = 59684,
    NPC_STOUT_BREW_ALEMENTAL        = 59519,
    NPC_SUDSY_BREW_ALEMENTAL        = 59522,
    NPC_POOL_OF_SUDS                = 56748,
    NPC_UNRULY_ALEMENTAL            = 56684,
    NPC_BUBBLING_BREW_ALEMENTAL     = 59521,
    NPC_BUBBLE_SHIELD_TRASH         = 59487,
    NPC_BLOATED_BREW_ALEMENTAL      = 59518,
    NPC_FIZZY_BREW_ALEMENTAL        = 59520,
    NPC_CARBONATION_POOL            = 56746,
    NPC_YEASTY_BREW_ALEMENTAL       = 59494,
    NPC_HABANERO_BREW               = 56731
};

Position const ookOokSummonPosition     = { -755.653f, 1351.396f, 146.923f }; // Ook-Ook summon position.
Position const HoptallusSummonPosition  = { -713.955f, 1254.574f, 164.790f }; // Hoptallus summon position.

enum GameObjects
{
    GAMEOBJECT_BREWERY_DOOR         = 209938, // Instance doors. 
    GAMEOBJECT_PART_CHEWED_CARROT   = 210938, // Hoptallus door. 
};

#endif // DEF_STORMSTOUT_BREWERY_H_
