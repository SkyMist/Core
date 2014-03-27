/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef DEF_TERRACE_OF_ENDLESS_SPRING_H
#define DEF_TERRACE_OF_ENDLESS_SPRING_H

#include "Map.h"
#include "Creature.h"

#define MAX_ENCOUNTER 6

enum Data
{
    // Bosses
    DATA_PROTECTOR_KAOLAN           = 0,
    DATA_ELDER_ASANJ                = 1,
    DATA_ELDER_REGAIL               = 2,
    DATA_TSULONG                    = 3,
    DATA_LEI_SHI                    = 4,
    DATA_SHA_OF_FEAR                = 5
};

enum Data64
{
    // Bosses
    DATA_PROTECTOR_KAOLAN           = 0,
    DATA_ELDER_ASANJ                = 1,
    DATA_ELDER_REGAIL               = 2,
    DATA_TSULONG                    = 3,
    DATA_LEI_SHI                    = 4,
    DATA_SHA_OF_FEAR                = 5
};

enum CreatureIds
{
    // Bosses
    BOSS_PROTECTOR_KAOLAN           = 60583,
    BOSS_ELDER_ASANJ                = 60586,
    BOSS_ELDER_REGAIL               = 60585,
    BOSS_TSULONG                    = 62442,
    BOSS_LEI_SHI                    = 62983,
    BOSS_SHA_OF_FEAR                = 60999, 

    // NPC's
    NPC_PROTECTOR_KAOLAN_FIRE_WALL_VEHICLE   = 48974, // Big, main vehicle, 1365.
};

enum GameObjectIds
{
    // GameObjects
    GO_SOMETHING                 = ,
};

#endif
