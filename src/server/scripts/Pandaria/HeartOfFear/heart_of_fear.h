/*Copyright (C) 2014 Buli.
*
* This file is NOT free software. Third-party users may NOT redistribute it or modify it :).
*/

#ifndef DEF_HEART_OF_FEAR_H
#define DEF_HEART_OF_FEAR_H

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define MAX_ENCOUNTERS 6

enum Data
{
    // Bosses
	DATA_VIZIER_ZORLOK_EVENT      = 0,
	DATA_BLADE_LORD_TAYAK_EVENT   = 1,
	DATA_GARALON_EVENT            = 2,

	DATA_WIND_LORD_MELJARAK_EVENT = 3,
	DATA_AMBER_SHAPER_UNSOK_EVENT = 4,
    DATA_EMPRESS_SHEKZEER_EVENT   = 5
};

enum Data64
{
    // Bosses
	DATA_VIZIER_ZORLOK            = 0,
	DATA_BLADE_LORD_TAYAK         = 1,
	DATA_GARALON                  = 2,

	DATA_WIND_LORD_MELJARAK       = 3,
	DATA_AMBER_SHAPER_UNSOK       = 4,
    DATA_EMPRESS_SHEKZEER         = 5
};

enum CreatureIds 
{
	// Raid Bosses
	BOSS_GRAND_VIZIER_ZORLOK      = 62980,
	BOSS_BLADE_LORD_TAYAK         = 62543,
	BOSS_GARALON                  = 62164,
	BOSS_WIND_LORD_MELJARAK       = 62397,
	BOSS_AMBER_SHAPER_UNSOK       = 62511,
	BOSS_GRAND_EMPRESS_SHEKZEER   = 62837,

	// NPCs
};

// enum GameObjectIds
// {
// };

#endif
