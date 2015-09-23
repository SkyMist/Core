/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef _UNIT_MOVEMENT_H
#define _UNIT_MOVEMENT_H

#include "DBCStructure.h"
#include "EventProcessor.h"
#include "FollowerReference.h"
#include "FollowerRefManager.h"
#include "HostileRefManager.h"
#include "MotionMaster.h"
#include "Object.h"
#include "ObjectMovement.h"
#include "SpellAuraDefines.h"
#include "ThreatManager.h"

namespace Movement
{
    class ExtraMovementStatusElement;
    class MoveSpline;
}

enum UnitMoveType
{
    MOVE_WALK           = 0,
    MOVE_RUN            = 1,
    MOVE_RUN_BACK       = 2,
    MOVE_SWIM           = 3,
    MOVE_SWIM_BACK      = 4,
    MOVE_TURN_RATE      = 5,
    MOVE_FLIGHT         = 6,
    MOVE_FLIGHT_BACK    = 7,
    MOVE_PITCH_RATE     = 8
};

#define MAX_MOVE_TYPE     9

extern float baseMoveSpeed[MAX_MOVE_TYPE];
extern float playerBaseMoveSpeed[MAX_MOVE_TYPE];

#endif
