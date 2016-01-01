/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Creature.h"
#include "MapManager.h"
#include "ConfusedMovementGenerator.h"
#include "VMapFactory.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

#ifdef MAP_BASED_RAND_GEN
#define rand_norm() owner.rand_norm()
#define urand(a, b) owner.urand(a, b)
#endif

// ========== ConfusedMovementGenerator ============ //

template<class T>
void ConfusedMovementGenerator<T>::DoInitialize(T* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    float min_wander_distance = 2.0f;
    float max_wander_distance = 4.0f;
    float x = owner->GetPositionX();
    float y = owner->GetPositionY();
    float z = owner->GetPositionZ();

    Map const* map = owner->GetBaseMap();

    i_nextMove = 1;

    bool is_water_ok, is_land_ok;
    _InitSpecific(owner, is_water_ok, is_land_ok);

    for (uint8 idx = 0; idx < MAX_CONF_WAYPOINTS + 1; ++idx)
    {
        float wanderX = x + frand(min_wander_distance, max_wander_distance);
        float wanderY = y + frand(min_wander_distance, max_wander_distance);

        // prevent invalid coordinates generation
        SkyMistCore::NormalizeMapCoord(wanderX);
        SkyMistCore::NormalizeMapCoord(wanderY);

        if (owner->IsWithinLOS(wanderX, wanderY, z))
        {
            bool is_water = map->IsInWater(wanderX, wanderY, z);

            if ((is_water && !is_water_ok) || (!is_water && !is_land_ok))
            {
                //! Cannot use coordinates outside our InhabitType. Use the current or previous position.
                wanderX = idx > 0 ? i_waypoints[idx - 1][0] : x;
                wanderY = idx > 0 ? i_waypoints[idx - 1][1] : y;
            }
        }
        else
        {
            //! Trying to access path outside line of sight. Skip this by using the current or previous position.
            wanderX = idx > 0 ? i_waypoints[idx - 1][0] : x;
            wanderY = idx > 0 ? i_waypoints[idx - 1][1] : y;
        }

        owner->UpdateAllowedPositionZ(wanderX, wanderY, z);

        //! Positions are fine - apply them to this waypoint
        i_waypoints[idx][0] = wanderX;
        i_waypoints[idx][1] = wanderY;
        i_waypoints[idx][2] = z;
    }

    if (!owner->IsStopped())
        owner->StopMoving();
    owner->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    owner->AddUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
}

template<>
void ConfusedMovementGenerator<Creature>::_InitSpecific(Creature* creature, bool &is_water_ok, bool &is_land_ok)
{
    is_water_ok = creature->canSwim();
    is_land_ok  = creature->canWalk();
}

template<>
void ConfusedMovementGenerator<Player>::_InitSpecific(Player* player, bool &is_water_ok, bool &is_land_ok)
{
    is_water_ok = true;
    is_land_ok  = true;
}

template<class T>
void ConfusedMovementGenerator<T>::DoReset(T* owner)
{
    i_nextMove = 1;
    i_nextMoveTime.Reset(0);

    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    if (!owner->IsStopped())
        owner->StopMoving();

    owner->AddUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
}

template<class T>
bool ConfusedMovementGenerator<T>::DoUpdate(T* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED))
    {
        owner->ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
        return true;
    }

    if (i_nextMoveTime.Passed())
    {
        // Currently moving, update location.
        owner->AddUnitState(UNIT_STATE_CONFUSED_MOVE);

        if (owner->movespline->Finalized())
        {
            i_nextMove = urand(1, MAX_CONF_WAYPOINTS);
            i_nextMoveTime.Reset(urand(1000, 1500));
        }
    }
    else
    {
        // Waiting for next move.
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed())
        {
            // Start moving.
            owner->AddUnitState(UNIT_STATE_CONFUSED_MOVE);

            if (i_nextMove > MAX_CONF_WAYPOINTS) // Should never happen.
                i_nextMove = MAX_CONF_WAYPOINTS;

            float x = i_waypoints[i_nextMove][0];
            float y = i_waypoints[i_nextMove][1];
            float z = i_waypoints[i_nextMove][2];

            Movement::MoveSplineInit init(owner);
            init.MoveTo(x, y, z);
            init.SetWalk(true);
            init.Launch();
        }
    }

    return true;
}

template<>
void ConfusedMovementGenerator<Player>::DoFinalize(Player* owner)
{
    if (!owner)
        return;

    owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    owner->ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
    if (!owner->IsStopped())
        owner->StopMoving();
}

template<>
void ConfusedMovementGenerator<Creature>::DoFinalize(Creature* owner)
{
    if (!owner)
        return;

    owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    owner->ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
    if (owner->getVictim())
        owner->SetTarget(owner->getVictim()->GetGUID());
}

template void ConfusedMovementGenerator<Player>::DoInitialize(Player* owner);
template void ConfusedMovementGenerator<Creature>::DoInitialize(Creature* owner);
template void ConfusedMovementGenerator<Player>::DoReset(Player* owner);
template void ConfusedMovementGenerator<Creature>::DoReset(Creature* owner);
template bool ConfusedMovementGenerator<Player>::DoUpdate(Player* owner, uint32 diff);
template bool ConfusedMovementGenerator<Creature>::DoUpdate(Creature* owner, uint32 diff);
