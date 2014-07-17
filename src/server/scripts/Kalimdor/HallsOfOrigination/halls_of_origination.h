/*Copyright (C) 2013 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef DEF_HALLS_OF_ORIGINATION_H
#define DEF_HALLS_OF_ORIGINATION_H

#include "Map.h"
#include "Creature.h"

#define MAX_ENCOUNTER 7

enum Data
{
    // Bosses
    DATA_TEMPLE_GUARDIAN_ANHUUR_EVENT = 0,
    DATA_EARTHRAGER_PTAH_EVENT        = 1,
    DATA_ANRAPHET_EVENT               = 2,
    DATA_ISISET_EVENT                 = 3,
    DATA_AMMUNAE_EVENT                = 4,
    DATA_SETESH_EVENT                 = 5,
    DATA_RAJH_EVENT                   = 6
};

enum Data64
{
    // Boss GUIDs
    DATA_TEMPLE_GUARDIAN_ANHUUR       = 0,
    DATA_EARTHRAGER_PTAH              = 1,
    DATA_ANRAPHET                     = 2,
    DATA_ISISET                       = 3,
    DATA_AMMUNAE                      = 4,
    DATA_SETESH                       = 5,
    DATA_RAJH                         = 6,

    // GameObject GUIDs
    DATA_ANHUUR_BRIDGE                = 7,
    DATA_ORIGINATION_ELEVATOR         = 8
};

enum CreatureIds
{
    // Dungeon Bosses
    BOSS_TEMPLE_GUARDIAN_ANHUUR       = 39425,
    BOSS_EARTHRAGER_PTAH              = 39428,
    BOSS_ANRAPHET                     = 39788,
    BOSS_ISISET                       = 39587,
    BOSS_AMMUNAE                      = 39731,
    BOSS_SETESH                       = 39732,
    BOSS_RAJH                         = 39378,

    // Trash Mobs
    NPC_BLISTERING_SCARAB             = 40310,
    NPC_BLOODPETAL_BLOSSOM            = 40620,
    NPC_DUSTBONE_TORMENTOR            = 40311,
    NPC_DUSTBONE_HORROR               = 40787,
    NPC_JEWELED_SCARAB                = 40458,
    NPC_EARTH_WARDEN                  = 39801,
    NPC_FLAME_WARDEN                  = 39800,
    NPC_FLUX_ANIMATOR                 = 40033,
    NPC_LIFEWARDEN_NYMPH              = 40715,
    NPC_LIVING_VINE                   = 40668,
    NPC_SPATIAL_ANOMALY               = 40170,
    NPC_TEMPLE_SWIFTSTALKER           = 48139,
    NPC_TEMPLE_SHADOWLANCER           = 48141,
    NPC_TEMPLE_RUNECASTER             = 48140,
    NPC_TEMPLE_FIRESHAPER             = 48143,
    NPC_VENOMOUS_SKITTERER            = 39440,
    NPC_WATER_WARDEN                  = 39802,
    NPC_AIR_WARDEN                    = 39803,

    // Various NPCs
    NPC_BRANN_BRONZEBEARD             = 49941,
    NPC_PIT_SNAKE                     = 39444,
    NPC_SEARING_FLAME                 = 40283,
    NPC_LIGHT                         = 40183
};

enum GameObjectIds
{
    // GameObjects
    GO_ORIGINATION_ELEVATOR           = 207547,
    GO_ANHUUR_BRIDGE                  = 206506,
    GO_ANHUUR_DOOR                    = 202307,
    GO_ANRAPHET_DOOR                  = 202313
};

enum AreaIds
{
    AREA_TOMB_OF_THE_EARTHRAGER     = 5610,
};

#endif
