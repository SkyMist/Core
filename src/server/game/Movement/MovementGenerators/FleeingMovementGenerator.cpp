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
#include "CreatureAI.h"
#include "MapManager.h"
#include "FleeingMovementGenerator.h"
#include "ObjectAccessor.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "VMapFactory.h"

#define MIN_QUIET_DISTANCE 28.0f
#define MAX_QUIET_DISTANCE 43.0f

// ========== FleeingMovementGenerator ============ //

template<class T>
void FleeingMovementGenerator<T>::DoInitialize(T* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);
    owner->AddUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE);

    _Init(owner);

    if (Unit* fright = ObjectAccessor::GetUnit(*owner, i_frightGUID))
    {
        i_caster_x = fright->GetPositionX();
        i_caster_y = fright->GetPositionY();
        i_caster_z = fright->GetPositionZ();
    }
    else
    {
        i_caster_x = owner->GetPositionX();
        i_caster_y = owner->GetPositionY();
        i_caster_z = owner->GetPositionZ();
    }

    i_only_forward = true;
    i_cur_angle = 0.0f;
    i_last_distance_from_caster = 0.0f;
    i_to_distance_from_caster = 0.0f;

    _setTargetLocation(owner);
}

template<>
void FleeingMovementGenerator<Creature>::_Init(Creature* owner)
{
    is_water_ok = owner->canSwim();
    is_land_ok  = owner->canWalk();
}

template<>
void FleeingMovementGenerator<Player>::_Init(Player* owner)
{
    is_water_ok = true;
    is_land_ok  = true;
}

template<class T>
void FleeingMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void FleeingMovementGenerator<T>::_setTargetLocation(T* owner)
{
    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
        return;

    if (!_setMoveData(owner))
        return;

    float x, y, z;
    if (!_getPoint(owner, x, y, z))
        return;

    // Add LOS check for target point.
    Position mypos;
    owner->GetPosition(&mypos);

    bool isInLOS = VMAP::VMapFactory::createOrGetVMapManager()->isInLineOfSight(owner->GetMapId(), mypos.m_positionX, mypos.m_positionY, mypos.m_positionZ + 2.0f, x, y, z + 2.0f);
    if (!isInLOS)
    {
        i_nextCheckTime.Reset(200);
        return;
    }

    owner->AddUnitState(UNIT_STATE_FLEEING_MOVE);

    Movement::MoveSplineInit init(owner);
    init.MoveTo(x, y, z);
    init.SetWalk(false);
    init.Launch();
}

template<class T>
bool FleeingMovementGenerator<T>::_setMoveData(T* owner)
{
    float cur_dist_xyz = owner->GetDistance(i_caster_x, i_caster_y, i_caster_z);

    if (i_to_distance_from_caster > 0.0f)
    {
        if ((i_last_distance_from_caster > i_to_distance_from_caster && cur_dist_xyz < i_to_distance_from_caster)  || // If we reach a lower distance.
           (i_last_distance_from_caster > i_to_distance_from_caster && cur_dist_xyz > i_last_distance_from_caster) || // If we can't be close.
           (i_last_distance_from_caster < i_to_distance_from_caster && cur_dist_xyz > i_to_distance_from_caster)   || // If we reach a bigger distance.
           (cur_dist_xyz > MAX_QUIET_DISTANCE)                                                                     || // If we are too far.
           (i_last_distance_from_caster > MIN_QUIET_DISTANCE && cur_dist_xyz < MIN_QUIET_DISTANCE))                   // If we leave the 'quiet zone'.
        {
            // We are very far or too close, stopping.
            i_to_distance_from_caster = 0.0f;
            i_nextCheckTime.Reset(urand(750, 1500));
            return false;
        }
        else
        {
            // Now we are running, continue.
            i_last_distance_from_caster = cur_dist_xyz;
            return true;
        }
    }

    float cur_dist = 0.0f;
    float angle_to_caster = 0.0f;

    if (Unit* fright = ObjectAccessor::GetUnit(*owner, i_frightGUID))
    {
        cur_dist = fright->GetDistance(owner);
        if (cur_dist < cur_dist_xyz)
        {
            i_caster_x = fright->GetPositionX();
            i_caster_y = fright->GetPositionY();
            i_caster_z = fright->GetPositionZ();
            angle_to_caster = fright->GetAngle(owner);
        }
        else
        {
            cur_dist = cur_dist_xyz;
            angle_to_caster = owner->GetAngle(i_caster_x, i_caster_y) + static_cast<float>(M_PI);
        }
    }
    else
    {
        cur_dist = cur_dist_xyz;
        angle_to_caster = owner->GetAngle(i_caster_x, i_caster_y) + static_cast<float>(M_PI);
    }

    // If we are too close may use 'pathfinding' else just stop.
    i_only_forward = cur_dist >= MIN_QUIET_DISTANCE / 3;

    // Get angle and 'distance from caster' to run to.
    float angle = 0.0f;

    if (i_cur_angle == 0.0f && i_last_distance_from_caster == 0.0f) // Just started, first time.
    {
        angle = (float)rand_norm() * (1.0f - cur_dist / MIN_QUIET_DISTANCE) * static_cast<float>(M_PI / 3) + (float)rand_norm() * static_cast<float>(M_PI * 2 / 3);
        i_to_distance_from_caster = MIN_QUIET_DISTANCE;
        i_only_forward = true;
    }
    else if (cur_dist < MIN_QUIET_DISTANCE)
    {
        angle = static_cast<float>(M_PI / 6) + (float)rand_norm() * static_cast<float>(M_PI * 2 / 3);
        i_to_distance_from_caster = cur_dist * 2 / 3 + (float)rand_norm() * (MIN_QUIET_DISTANCE - cur_dist * 2 / 3);
    }
    else if (cur_dist > MAX_QUIET_DISTANCE)
    {
        angle = (float)rand_norm() * static_cast<float>(M_PI / 3) + static_cast<float>(M_PI * 2 / 3);
        i_to_distance_from_caster = MIN_QUIET_DISTANCE + 2.5f + (float)rand_norm() * (MAX_QUIET_DISTANCE - MIN_QUIET_DISTANCE - 2.5f);
    }
    else
    {
        angle = (float)rand_norm() * static_cast<float>(M_PI);
        i_to_distance_from_caster = MIN_QUIET_DISTANCE + 2.5f + (float)rand_norm() * (MAX_QUIET_DISTANCE - MIN_QUIET_DISTANCE - 2.5f);
    }

    int8 sign = (float)rand_norm() > 0.5f ? 1 : -1;
    i_cur_angle = sign * angle + angle_to_caster;

    // Current distance.
    i_last_distance_from_caster = cur_dist;

    return true;
}

template<class T>
bool FleeingMovementGenerator<T>::_getPoint(T* owner, float &x, float &y, float &z)
{
    x = owner->GetPositionX();
    y = owner->GetPositionY();
    z = owner->GetPositionZ();

    float temp_x, temp_y, angle;
    const Map* _map = owner->GetBaseMap();

    // Primitive pathfinding.
    for (uint8 i = 0; i < 18; ++i)
    {
        if (i_only_forward && i > 2)
            break;

        float distance = 5.0f;

        switch (i)
        {
            case 0:
                angle = i_cur_angle;
                break;
            case 1:
                angle = i_cur_angle;
                distance /= 2;
                break;
            case 2:
                angle = i_cur_angle;
                distance /= 4;
                break;
            case 3:
                angle = i_cur_angle + static_cast<float>(M_PI/4);
                break;
            case 4:
                angle = i_cur_angle - static_cast<float>(M_PI/4);
                break;
            case 5:
                angle = i_cur_angle + static_cast<float>(M_PI/4);
                distance /= 2;
                break;
            case 6:
                angle = i_cur_angle - static_cast<float>(M_PI/4);
                distance /= 2;
                break;
            case 7:
                angle = i_cur_angle + static_cast<float>(M_PI/2);
                break;
            case 8:
                angle = i_cur_angle - static_cast<float>(M_PI/2);
                break;
            case 9:
                angle = i_cur_angle + static_cast<float>(M_PI/2);
                distance /= 2;
                break;
            case 10:
                angle = i_cur_angle - static_cast<float>(M_PI/2);
                distance /= 2;
                break;
            case 11:
                angle = i_cur_angle + static_cast<float>(M_PI/4);
                distance /= 4;
                break;
            case 12:
                angle = i_cur_angle - static_cast<float>(M_PI/4);
                distance /= 4;
                break;
            case 13:
                angle = i_cur_angle + static_cast<float>(M_PI/2);
                distance /= 4;
                break;
            case 14:
                angle = i_cur_angle - static_cast<float>(M_PI/2);
                distance /= 4;
                break;
            case 15:
                angle = i_cur_angle +  static_cast<float>(3 * M_PI/4);
                distance /= 2;
                break;
            case 16:
                angle = i_cur_angle -  static_cast<float>(3 * M_PI/4);
                distance /= 2;
                break;
            case 17:
                angle = i_cur_angle + static_cast<float>(M_PI);
                distance /= 2;
                break;
            default:
                angle = 0.0f;
                distance = 0.0f;
                break;
        }

        temp_x = x;
        temp_y = y;
        float temp_z = z;
        bool goodCoordinates = true;

        for (uint8 i = 0; i < 5; ++i)
        {
            temp_x += distance/5 * cos(angle);
            temp_y += distance/5 * sin(angle);
            float _temp_x = temp_x;
            float _temp_y = temp_y;
            SkyMistCore::NormalizeMapCoord(_temp_x);
            SkyMistCore::NormalizeMapCoord(_temp_y);
            float _temp_z = _map->GetHeight(temp_x, temp_y, z, true);
            if (fabs(_temp_z - temp_z) > 2.0f)
            {
                goodCoordinates = false;
                break;
            }
            temp_z = _temp_z;
            if (i == 4)
            {
                temp_x = _temp_x;
                temp_y = _temp_y;
            }
        }
        if (!goodCoordinates)
            continue;

        SkyMistCore::NormalizeMapCoord(temp_x);
        SkyMistCore::NormalizeMapCoord(temp_y);
        if (!owner->IsWithinLOS(temp_x, temp_y, z))
            continue;
        else
        {
            bool is_water_now = _map->IsInWater(x,y,z);

            if (is_water_now && _map->IsInWater(temp_x,temp_y,z))
            {
                x = temp_x;
                y = temp_y;
                return true;
            }
            float new_z = _map->GetHeight(owner->GetPhaseMask(), temp_x, temp_y, z, true);

            if (new_z <= INVALID_HEIGHT)
                continue;

            bool is_water_next = _map->IsInWater(temp_x, temp_y, new_z);

            if ((is_water_now && !is_water_next && !is_land_ok) || (!is_water_now && is_water_next && !is_water_ok))
                continue;

            if (!(new_z - z) || distance / fabs(new_z - z) > 1.0f)
            {
                float new_z_left  = _map->GetHeight(owner->GetPhaseMask(), temp_x + 1.0f * std::cos(angle + static_cast<float>(M_PI / 2)), temp_y + 1.0f * std::sin(angle + static_cast<float>(M_PI / 2)), z, true);
                float new_z_right = _map->GetHeight(owner->GetPhaseMask(), temp_x + 1.0f * std::cos(angle - static_cast<float>(M_PI / 2)), temp_y + 1.0f * std::sin(angle - static_cast<float>(M_PI / 2)), z, true);
                if (fabs(new_z_left - new_z) < 1.2f && fabs(new_z_right - new_z) < 1.2f)
                {
                    x = temp_x;
                    y = temp_y;
                    z = new_z;
                    return true;
                }
            }
        }
    }

    i_to_distance_from_caster = 0.0f;
    i_nextCheckTime.Reset(urand(750, 1500));
    return false;
}

template<class T>
bool FleeingMovementGenerator<T>::DoUpdate(T* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
    {
        owner->ClearUnitState(UNIT_STATE_FLEEING_MOVE);
        return true;
    }

    i_nextCheckTime.Update(diff);
    if (i_nextCheckTime.Passed() && owner->movespline->Finalized())
        _setTargetLocation(owner);

    return true;
}

template<>
void FleeingMovementGenerator<Player>::DoFinalize(Player* owner)
{
    if (!owner)
        return;

    owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);
    owner->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE);
    if (!owner->IsStopped())
        owner->StopMoving();
}

template<>
void FleeingMovementGenerator<Creature>::DoFinalize(Creature* owner)
{
    if (!owner)
        return;

    owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);
    owner->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE);
    if (owner->getVictim())
        owner->SetTarget(owner->getVictim()->GetGUID());
}

template void FleeingMovementGenerator<Player>::DoInitialize(Player* owner);
template void FleeingMovementGenerator<Creature>::DoInitialize(Creature* owner);
template bool FleeingMovementGenerator<Player>::_setMoveData(Player* owner);
template bool FleeingMovementGenerator<Creature>::_setMoveData(Creature* owner);
template bool FleeingMovementGenerator<Player>::_getPoint(Player* owner, float &x, float &y, float &z);
template bool FleeingMovementGenerator<Creature>::_getPoint(Creature* owner, float &x, float &y, float &z);
template void FleeingMovementGenerator<Player>::_setTargetLocation(Player* owner);
template void FleeingMovementGenerator<Creature>::_setTargetLocation(Creature* owner);
template void FleeingMovementGenerator<Player>::DoReset(Player* owner);
template void FleeingMovementGenerator<Creature>::DoReset(Creature* owner);
template bool FleeingMovementGenerator<Player>::DoUpdate(Player* owner, uint32 diff);
template bool FleeingMovementGenerator<Creature>::DoUpdate(Creature* owner, uint32 diff);

// ========== TimedFleeingMovementGenerator ============ //

bool TimedFleeingMovementGenerator::DoUpdate(Unit* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
    {
        owner->ClearUnitState(UNIT_STATE_FLEEING_MOVE);
        return true;
    }

    i_totalFleeTime.Update(diff);
    if (i_totalFleeTime.Passed())
        return false;

    // This calls grandparent Update method hiden by FleeingMovementGenerator::Update(Creature*, uint32) version.
    // This is done instead of casting Unit* to Creature* and calling parent method, we can use Unit* directly.
    return MovementGeneratorMedium< Creature, FleeingMovementGenerator<Creature> >::Update(owner, diff);
}

void TimedFleeingMovementGenerator::DoFinalize(Unit* owner)
{
    if (!owner)
        return;

    owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);
    owner->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE);

    if (Unit* victim = owner->getVictim())
    {
        if (owner->isAlive())
        {
            owner->AttackStop();
            owner->ToCreature()->AI()->AttackStart(victim);
        }
    }
}
