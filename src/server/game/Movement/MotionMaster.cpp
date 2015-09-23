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

#include "MotionMaster.h"
#include "CreatureAISelector.h"
#include "Creature.h"

#include "ConfusedMovementGenerator.h"
#include "FleeingMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "PointMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "RandomMovementGenerator.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void MotionMaster::Initialize()
{
    // clear ALL movement generators (including default)
    while (!empty())
    {
        MovementGenerator* current = top();
        pop();
        if (current && !isStatic(current))
            DirectDelete(current); // Skip finalizing on delete, it might launch new movement.       
    }

    InitDefault();
}

// Set new default movement generator.
void MotionMaster::InitDefault()
{
    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator(_owner->ToCreature());
        Mutate(movement == NULL ? &si_idleMovement : movement, MOTION_SLOT_IDLE);
    }
    else
    {
        Mutate(&si_idleMovement, MOTION_SLOT_IDLE);
    }
}

MotionMaster::~MotionMaster()
{
    // clear ALL movement generators (including default)
    while (!empty())
    {
        MovementGenerator* current = top();
        pop();
        if (current && !isStatic(current))
            delete current; // Skip finalizing on delete, it might launch new movement.
    }
}

void MotionMaster::UpdateMotion(uint32 diff)
{
    if (!_owner)
        return;

    if (_owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED)) // what about UNIT_STATE_DISTRACTED? Why is this not included?
        return;

    ASSERT(!empty());

    _cleanFlag |= MMCF_UPDATE;
    if (!top()->Update(_owner, diff))
    {
        _cleanFlag &= ~MMCF_UPDATE;
        MovementExpired();
    }
    else
        _cleanFlag &= ~MMCF_UPDATE;

    if (_expList)
    {
        for (size_t i = 0; i < _expList->size(); ++i)
        {
            MovementGenerator* movementGen = (*_expList)[i];
            DirectDelete(movementGen);
        }

        delete _expList;
        _expList = NULL;

        if (empty())
            Initialize();
        else if (needInitTop())
            InitTop();
        else if (_cleanFlag & MMCF_RESET)
            top()->Reset(_owner);

        _cleanFlag &= ~MMCF_RESET;
    }
}

void MotionMaster::DirectClean(bool reset)
{
    while (size() > 1)
    {
        MovementGenerator* current = top();
        pop();
        if (current) DirectDelete(current);
    }

    if (empty())
        return;

    if (needInitTop())
        InitTop();
    else if (reset)
        top()->Reset(_owner);
}

void MotionMaster::DelayedClean()
{
    while (size() > 1)
    {
        MovementGenerator* current = top();
        pop();
        if (current)
            DelayedDelete(current);
    }
}

void MotionMaster::DirectExpire(bool reset)
{
    if (size() > 1)
    {
        MovementGenerator* current = top();
        pop();
        if (current)
            DirectDelete(current);
    }

    while (!empty() && !top())
        --_top;

    if (empty())
        Initialize();
    else if (needInitTop())
        InitTop();
    else if (reset)
        top()->Reset(_owner);
}

void MotionMaster::DelayedExpire()
{
    if (size() > 1)
    {
        MovementGenerator *curr = top();
        pop();
        DelayedDelete(curr);
    }

    while (!empty() && !top())
        --_top;
}

void MotionMaster::MoveIdle()
{
    // Should be preceded by MovementExpired or Clear if there's an overlying MovementGenerator active.
    if (empty() || !isStatic(top()))
        Mutate(&si_idleMovement, MOTION_SLOT_IDLE);
}

void MotionMaster::MoveRandom(float spawndist)
{
    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (GUID: %u) starts moving random.", _owner->GetGUIDLow());
        Mutate(new RandomMovementGenerator<Creature>(spawndist), MOTION_SLOT_IDLE);
    }
}

void MotionMaster::MoveTargetedHome()
{
    Clear(false);

    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        if (!_owner->ToCreature()->GetCharmerOrOwnerGUID())
        {
            sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) now targeted and is moving home.", _owner->GetEntry(), _owner->GetGUIDLow());
            Mutate(new HomeMovementGenerator<Creature>(), MOTION_SLOT_ACTIVE);
        }
        else
        {
            sLog->outDebug(LOG_FILTER_GENERAL, "Pet or controlled creature (Entry: %u GUID: %u) now targeted and is moving home.", _owner->GetEntry(), _owner->GetGUIDLow());
            if (Unit* target = _owner->ToCreature()->GetCharmerOrOwner())
            {
                sLog->outDebug(LOG_FILTER_GENERAL, "Following %s (GUID: %u)", target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", target->GetTypeId() == TYPEID_PLAYER ? target->GetGUIDLow() : ((Creature*)target)->GetDBTableGUIDLow());
                Mutate(new FollowMovementGenerator<Creature>(target, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE), MOTION_SLOT_ACTIVE);
            }
        }
    }
    else
    {
        sLog->outError(LOG_FILTER_GENERAL, "Player (GUID: %u) just attempted to MoveTargetedHome!", _owner->GetGUIDLow());
    }
}

void MotionMaster::MoveConfused()
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) now moves confused.", _owner->GetGUIDLow());
        Mutate(new ConfusedMovementGenerator<Player>(), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) now moves confused.", _owner->GetEntry(), _owner->GetGUIDLow());
        Mutate(new ConfusedMovementGenerator<Creature>(), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // Ignore movement request if the target does not exist.
    if (!target || target == _owner || _owner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return;

    // Ignore if the owner is a hostile creature, chasing a player, and it's evading / moving home.
    if (_owner->GetTypeId() != TYPEID_PLAYER && target->GetTypeId() == TYPEID_PLAYER && _owner->IsHostileTo(target))
        if (_owner->HasUnitState(UNIT_STATE_EVADE) || Impl[MOTION_SLOT_ACTIVE] && Impl[MOTION_SLOT_ACTIVE]->GetMovementGeneratorType() == HOME_MOTION_TYPE)
            return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) is now chasing %s (GUID: %u)", _owner->GetGUIDLow(), target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", target->GetTypeId() == TYPEID_PLAYER ? target->GetGUIDLow() : target->ToCreature()->GetDBTableGUIDLow());
        Mutate(new ChaseMovementGenerator<Player>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) is now chasing %s (GUID: %u)", _owner->GetEntry(), _owner->GetGUIDLow(), target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", target->GetTypeId() == TYPEID_PLAYER ? target->GetGUIDLow() : target->ToCreature()->GetDBTableGUIDLow());
        Mutate(new ChaseMovementGenerator<Creature>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveFollow(Unit* target, float dist, float angle, MovementSlot slot)
{
    // Ignore movement request if the target does not exist.
    if (!target || target == _owner || _owner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return;

    // Ignore if the owner is a hostile creature, following a player, and it's evading / moving home.
    if (_owner->GetTypeId() != TYPEID_PLAYER && target->GetTypeId() == TYPEID_PLAYER && _owner->IsHostileTo(target))
        if (_owner->HasUnitState(UNIT_STATE_EVADE) || Impl[MOTION_SLOT_ACTIVE] && Impl[MOTION_SLOT_ACTIVE]->GetMovementGeneratorType() == HOME_MOTION_TYPE)
            return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) is now following %s (GUID: %u)", _owner->GetGUIDLow(), target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", target->GetTypeId() == TYPEID_PLAYER ? target->GetGUIDLow() : target->ToCreature()->GetDBTableGUIDLow());
        Mutate(new FollowMovementGenerator<Player>(target, dist, angle), slot);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) is now following %s (GUID: %u)", _owner->GetEntry(), _owner->GetGUIDLow(), target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", target->GetTypeId() == TYPEID_PLAYER ? target->GetGUIDLow() : target->ToCreature()->GetDBTableGUIDLow());
        Mutate(new FollowMovementGenerator<Creature>(target, dist, angle), slot);
    }
}

void MotionMaster::MovePoint(uint32 id, float x, float y, float z)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) now targeted and is moving to point (Id: %u X: %f Y: %f Z: %f)", _owner->GetGUIDLow(), id, x, y, z);
        Mutate(new PointMovementGenerator<Player>(id, x, y, z), MOTION_SLOT_ACTIVE);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) now targeted and is moving to point (ID: %u X: %f Y: %f Z: %f)", _owner->GetEntry(), _owner->GetGUIDLow(), id, x, y, z);
        Mutate(new PointMovementGenerator<Creature>(id, x, y, z), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveLand(uint32 id, Position const& pos)
{
    float x, y, z;
    pos.GetPosition(x, y, z);

    sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u) is now landing at point (ID: %u X: %f Y: %f Z: %f)", _owner->GetEntry(), id, x, y, z);

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetAnimation(Movement::ToGround);
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveTakeoff(uint32 id, Position const& pos)
{
    float x, y, z;
    pos.GetPosition(x, y, z);
    MoveTakeoff(id, x, y, z);
}

void MotionMaster::MoveTakeoff(uint32 id, float x, float y, float z)
{
    sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u) is now taking off at point (ID: %u X: %f Y: %f Z: %f)", _owner->GetEntry(), id, x, y, z);

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetAnimation(Movement::ToFly);
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveKnockbackFrom(float srcX, float srcY, float speedXY, float speedZ)
{
    // This function may make players fall under the map.
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        return;

    float x, y, z;
    float moveTimeHalf = speedZ / Movement::gravity;
    float dist = 2 * moveTimeHalf * speedXY;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);

    _owner->GetNearPoint(_owner, x, y, z, _owner->GetObjectSize(), dist, _owner->GetAngle(srcX, srcY) + M_PI);

    // This should help for players falling under the map needs testing.
    // while (!_owner->IsWithinLOS(x, y, z))
    //     z += 1.0f;

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetParabolic(max_height,0);
    init.SetOrientationFixed(true);
    init.SetVelocity(speedXY);
    init.Launch();
    Mutate(new EffectMovementGenerator(0), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveJumpTo(float angle, float speedXY, float speedZ)
{
    // This function may make players fall under the map.
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        return;

    float x, y, z;

    float moveTimeHalf = speedZ / Movement::gravity;
    float dist = 2 * moveTimeHalf * speedXY;

    _owner->GetClosePoint(x, y, z, _owner->GetObjectSize(), dist, angle);

    // This should help for players falling under the map needs testing.
    // while (!_owner->IsWithinLOS(x, y, z))
    //     z += 1.0f;

    MoveJump(x, y, z, speedXY, speedZ);
}

void MotionMaster::MoveJump(float x, float y, float z, float speedXY, float speedZ, float o, uint32 id)
{
    sLog->outDebug(LOG_FILTER_GENERAL, "Unit (GUID: %u) is now jumping to point (X: %f Y: %f Z: %f).", _owner->GetGUIDLow(), x, y, z);

    float moveTimeHalf = speedZ / Movement::gravity;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetParabolic(max_height, 0);
    init.SetVelocity(speedXY);
    if (o < 6.82f)
        init.SetFacing(o);
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::CustomJump(float x, float y, float z, float speedXY, float speedZ, uint32 id)
{
    sLog->outDebug(LOG_FILTER_GENERAL, "Unit (GUID: %u) is now custom jumping to point (X: %f Y: %f Z: %f).", _owner->GetGUIDLow(), x, y, z);

    speedZ *= 2.3f;
    speedXY *= 2.3f;
    float moveTimeHalf = speedZ / Movement::gravity;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);
    max_height /= 15.0f;

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetParabolic(max_height, 0);
    init.SetVelocity(speedXY);
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveFall(uint32 id/*=0*/)
{
    // Use larger distances for vmap height search than in most other cases.
    float mapHeight = _owner->GetMap()->GetHeight(_owner->GetPhaseMask(), _owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ(), true, MAX_FALL_DISTANCE);
    if (mapHeight <= INVALID_HEIGHT)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "MotionMaster::MoveFall: Unable to retrieve a proper height on map %u (x: %f, y: %f, z: %f).", _owner->GetMap()->GetId(), _owner->GetPositionX(), _owner->GetPositionX(), _owner->GetPositionZ());
        return;
    }

    // Abort also if the ground is very near, can't fall there.
    if (fabs(_owner->GetPositionZ() - mapHeight) < 0.1f)
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        _owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
        _owner->m_movementInfo.SetFallTime(0);
    }

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(_owner->GetPositionX(), _owner->GetPositionY(), mapHeight);
    init.SetFall();
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveBackward(uint32 id, float x, float y, float z, float speed)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        _owner->AddUnitMovementFlag(MOVEMENTFLAG_BACKWARD);

    Movement::MoveSplineInit init(*_owner);
    init.MoveTo(x, y, z);
    init.SetOrientationInversed();
    init.Launch();
    if (speed > 0.0f)
        init.SetVelocity(speed);
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveCharge(float x, float y, float z, float speed, uint32 id)
{
    if (Impl[MOTION_SLOT_CONTROLLED] && Impl[MOTION_SLOT_CONTROLLED]->GetMovementGeneratorType() != DISTRACT_MOTION_TYPE)
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) now charges to point (X: %f Y: %f Z: %f).", _owner->GetGUIDLow(), x, y, z);
        Mutate(new PointMovementGenerator<Player>(id, x, y, z, speed), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) now charges to point (X: %f Y: %f Z: %f).", _owner->GetEntry(), _owner->GetGUIDLow(), x, y, z);
        Mutate(new PointMovementGenerator<Creature>(id, x, y, z, speed), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        sLog->outError(LOG_FILTER_GENERAL, "Player (GUID: %u) is attempting to MoveSeekAssistance!", _owner->GetGUIDLow());
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) now attempts to seek assistance at position (X: %f Y: %f Z: %f).", _owner->GetEntry(), _owner->GetGUIDLow(), x, y, z);
        _owner->AttackStop();
        _owner->ToCreature()->SetReactState(REACT_PASSIVE);
        Mutate(new AssistanceMovementGenerator(x, y, z), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        sLog->outError(LOG_FILTER_GENERAL, "Player (GUID: %u) is attempting to MoveSeekAssistanceDistract!", _owner->GetGUIDLow());
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) is distracted after seeking assistance (Time: %u).", _owner->GetEntry(), _owner->GetGUIDLow(), time);
        Mutate(new AssistanceDistractMovementGenerator(time), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if (!enemy)
        return;

    if (_owner->HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Player (GUID: %u) is now fleeing from %s (GUID: %u).", _owner->GetGUIDLow(), enemy->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", enemy->GetTypeId() == TYPEID_PLAYER ? enemy->GetGUIDLow() : enemy->ToCreature()->GetDBTableGUIDLow());
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) is now fleeing from %s (GUID: %u) %s", _owner->GetEntry(), _owner->GetGUIDLow(), enemy->GetTypeId() == TYPEID_PLAYER ? "player" : "creature", enemy->GetTypeId() == TYPEID_PLAYER ? enemy->GetGUIDLow() : enemy->ToCreature()->GetDBTableGUIDLow(), time ? " for a limited time." : ".");
        if (time)
            Mutate(new TimedFleeingMovementGenerator(enemy->GetGUID(), time), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new FleeingMovementGenerator<Creature>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        if (path < sTaxiPathNodesByPath.size())
        {
            sLog->outDebug(LOG_FILTER_GENERAL, "Player %s now takes a taxi to (Path %u node %u).", _owner->GetName(), path, pathnode);
            FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(sTaxiPathNodesByPath[path], pathnode);
            Mutate(mgen, MOTION_SLOT_CONTROLLED);
        }
        else
            sLog->outError(LOG_FILTER_GENERAL, "Player %s attempts to take a taxi to non-existing (Path %u node %u)!", _owner->GetName(), path, pathnode);
    }
    else
        sLog->outError(LOG_FILTER_GENERAL, "Creature (Entry: %u GUID: %u) attempts to take a taxi to (Path %u node %u)!", _owner->GetEntry(), _owner->GetGUIDLow(), path, pathnode);
}

void MotionMaster::MoveDistract(uint32 timer)
{
    if (Impl[MOTION_SLOT_CONTROLLED])
        return;

    sLog->outDebug(LOG_FILTER_GENERAL, "%s (GUID: %u) is now distracted (timer: %u).", _owner->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature", _owner->GetGUIDLow(), timer);

    DistractMovementGenerator* mgen = new DistractMovementGenerator(timer);
    Mutate(mgen, MOTION_SLOT_CONTROLLED);
}

void MotionMaster::Mutate(MovementGenerator* movementGen, MovementSlot slot)
{
    if (MovementGenerator* current = Impl[slot])
    {
        Impl[slot] = NULL; // In case a new one is generated in this slot during DirectDelete.
        if (_top == slot && (_cleanFlag & MMCF_UPDATE))
            DelayedDelete(current);
        else
            DirectDelete(current);
    }
    else if (_top < slot)
        _top = slot;

    Impl[slot] = movementGen;

    if (_top > slot)
        _needInit[slot] = true;
    else
    {
        _needInit[slot] = false;
        movementGen->Initialize(_owner);
    }
}

void MotionMaster::MovePath(uint32 path_id, bool repeatable)
{
    if (!path_id)
        return;

    sLog->outDebug(LOG_FILTER_GENERAL, "%s (GUID: %u) is now starting to move on path (id: %u, repeatable: %s).", _owner->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature", _owner->GetGUIDLow(), path_id, repeatable ? "YES" : "NO");

    Mutate(new WaypointMovementGenerator<Creature>(path_id, repeatable), MOTION_SLOT_IDLE);
}

void MotionMaster::MoveRotate(uint32 time, RotateDirection direction)
{
    if (!time)
        return;

    sLog->outDebug(LOG_FILTER_GENERAL, "%s (GUID: %u) is now rotating (time: %u).", _owner->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature",  _owner->GetGUIDLow(), time);

    Mutate(new RotateMovementGenerator(time, direction), MOTION_SLOT_ACTIVE);
}

void MotionMaster::propagateSpeedChange()
{
    for (int i = 0; i <= _top; ++i)
    {
        if (Impl[i])
            Impl[i]->unitSpeedChanged();
    }
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
   if (empty())
       return IDLE_MOTION_TYPE;

   return top()->GetMovementGeneratorType();
}

MovementGeneratorType MotionMaster::GetMotionSlotType(int slot) const
{
    if (!Impl[slot])
        return NULL_MOTION_TYPE;
    else
        return Impl[slot]->GetMovementGeneratorType();
}

void MotionMaster::InitTop()
{
    top()->Initialize(_owner);
    _needInit[_top] = false;
}

void MotionMaster::DirectDelete(_Ty current)
{
    if (isStatic(current))
        return;

    current->Finalize(_owner);
    delete current;
}

void MotionMaster::DelayedDelete(_Ty current)
{
    sLog->outFatal(LOG_FILTER_GENERAL, "Unit (Entry %u) is trying to delete it's updating MG (Type %u)!", _owner->GetEntry(), current->GetMovementGeneratorType());

    if (isStatic(current))
        return;

    if (!_expList)
        _expList = new ExpireList();
    _expList->push_back(current);
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
    if (_owner->movespline->Finalized())
       return false;

    G3D::Vector3 const& dest = _owner->movespline->FinalDestination();
    x = dest.x;
    y = dest.y;
    z = dest.z;

    return true;
}
