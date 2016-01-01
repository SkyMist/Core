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
#include "RandomMovementGenerator.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "Util.h"
#include "CreatureGroups.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

#define RUNNING_CHANCE_RANDOMMV 20                                  //will be "1 / RUNNING_CHANCE_RANDOMMV"

#ifdef MAP_BASED_RAND_GEN
#define rand_norm() creature.rand_norm()
#endif

// ========== RandomMovementGenerator ============ //

template<>
void RandomMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    if (!wander_distance)
        wander_distance = owner->GetRespawnRadius();

    owner->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
    _setRandomLocation(owner);
}

template<>
void RandomMovementGenerator<Creature>::DoReset(Creature* owner)
{
    Initialize(owner);
}

template<>
void RandomMovementGenerator<Creature>::_setRandomLocation(Creature* owner)
{
    float respX, respY, respZ, respO, destX, destY, destZ, travelDistZ;

    owner->GetHomePosition(respX, respY, respZ, respO);

    Map const* map = owner->GetBaseMap();

    // For 2D / 3D system selection
    // bool is_land_ok  = owner->CanWalk();                // not used?
    // bool is_water_ok = owner->CanSwim();                // not used?
    bool is_air_ok = owner->CanFly();

    const float angle = float(rand_norm()) * static_cast<float>(M_PI * 2.0f);
    const float range = float(rand_norm()) * wander_distance;
    const float distanceX = range * std::cos(angle);
    const float distanceY = range * std::sin(angle);

    destX = respX + distanceX;
    destY = respY + distanceY;

    // prevent invalid coordinates generation
    SkyMistCore::NormalizeMapCoord(destX);
    SkyMistCore::NormalizeMapCoord(destY);

    travelDistZ = range; // sin^2 + cos^2 = 1, so travelDistZ = range^2; no need for sqrt below

    if (is_air_ok)                                          // 3D system above ground and above water (flying mode)
    {
        // Limit height change.
        const float distanceZ = float(rand_norm()) * travelDistZ / 2.0f;
        destZ = respZ + distanceZ;
        float levelZ = map->GetWaterOrGroundLevel(destX, destY, destZ - 2.0f);

        // A problem here, we must fly above the ground and water, not under. Let's try on next tick.
        if (levelZ >= destZ)
            return;
    }
    //else if (is_water_ok)                                 // 3D system under water and above ground (swimming mode)
    else                                                    // 2D only
    {
        // 10.0 is the max that Vmap height can check (MAX_CAN_FALL_DISTANCE).
        travelDistZ = travelDistZ >= 10.0f ? 10.0f : travelDistZ;

        // The fastest way to get an accurate result 90% of the time.
        // A better result can be obtained like 99% accuracy with a ray light, but the cost is too high and the code is too long.
        destZ = map->GetHeight(owner->GetPhaseMask(), destX, destY, respZ + travelDistZ - 2.0f, false);

        if (fabs(destZ - respZ) > travelDistZ)              // Map check
        {
            // Vmap Horizontal or above
            destZ = map->GetHeight(owner->GetPhaseMask(), destX, destY, respZ - 2.0f, true);

            if (fabs(destZ - respZ) > travelDistZ)
            {
                // Vmap is higher.
                destZ = map->GetHeight(owner->GetPhaseMask(), destX, destY, respZ + travelDistZ - 2.0f, true);

                // Let's forget these bad coords where a z cannot be found and retry at next tick.
                if (fabs(destZ - respZ) > travelDistZ)
                    return;
            }
        }
    }

    if (is_air_ok)
        i_nextMoveTime.Reset(2500);
    else
        i_nextMoveTime.Reset(urand(3000, 7000));

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    Movement::MoveSplineInit init(owner);
    init.MoveTo(destX, destY, destZ);
    init.SetWalk(true);
    init.Launch();

    // Call for creature group update.
    if (owner->GetFormation() && owner->GetFormation()->getLeader() == owner)
        owner->GetFormation()->LeaderMoveTo(destX, destY, destZ);
}

template<>
bool RandomMovementGenerator<Creature>::DoUpdate(Creature* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED))
    {
        i_nextMoveTime.Reset(0);  // Expire the timer.
        owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }

    if (owner->movespline->Finalized())
    {
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed())
            _setRandomLocation(owner);
    }

    return true;
}

template<>
void RandomMovementGenerator<Creature>::DoFinalize(Creature* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (!owner->isAlive())
        return;

    owner->SetWalk(false);
}

template<>
bool RandomMovementGenerator<Creature>::GetResetPosition(Creature* owner, float &x, float &y, float &z)
{
    if (!owner)
        return false;

    float radius;
    owner->GetRespawnPosition(x, y, z, NULL, &radius);

    // Use current if in range.
    if (owner->IsWithinDist2d(x, y, radius))
        owner->GetPosition(x, y, z);

    return true;
}
