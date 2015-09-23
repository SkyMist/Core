/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef _PLAYER_MOVEMENT_H
#define _PLAYER_MOVEMENT_H

#include "DBCStores.h"
#include "GroupReference.h"
#include "MapReference.h"

#include "Item.h"
#include "PhaseMgr.h"
#include "QuestDef.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "UnitMovement.h"
#include "ObjectMovement.h"
#include "Opcodes.h"
#include "WorldSession.h"

#include <string>
#include <vector>

enum TeleportToOptions
{
    TELE_TO_GM_MODE             = 0x01,
    TELE_TO_NOT_LEAVE_TRANSPORT = 0x02,
    TELE_TO_NOT_LEAVE_COMBAT    = 0x04,
    TELE_TO_NOT_UNSUMMON_PET    = 0x08,
    TELE_TO_SPELL               = 0x10
};

#endif
