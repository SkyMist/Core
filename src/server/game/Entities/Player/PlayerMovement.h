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
