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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

#include <cmath>

// ========== ChaseMovementGenerator ============ //

template<>
void ChaseMovementGenerator<Player>::DoInitialize(Player* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    _setTargetLocation(owner);
}

template<>
void ChaseMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->SetWalk(false);
    owner->AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    _setTargetLocation(owner);
}

template<class T>
void ChaseMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T, typename D>
void TargetedMovementGeneratorMedium<T,D>::_setTargetLocation(T* owner)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE))
        return;

    // This is hacky. ToDo: remove.
    if (owner->GetTypeId() == TYPEID_UNIT)
    {
        switch(owner->GetEntry())
        {
            case 52498: // Beth'tilac.
                if (owner->GetMapId() == 720)
                {
                    if (owner->GetAI()->GetData(2) == 0 && (i_target->GetPositionZ() < 100.0f || i_target->IsPetGuardianStuff()))
                        return;
                }
                break;
            case 52581: // Cinderweb Drone.
            case 52447: // Cinderweb Spiderling.
            case 53745: // Engorged Broodling.
                if (owner->GetMapId() == 720)
                    if (i_target->GetPositionZ() > 100.0f)
                        return;
                break;
            case 56923: // Twilight Sapper
                if (owner->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_CONTROLLED))
                    return;
                break;
            case 45870:
            case 45871:
            case 45872:
            case 45812:
                if (Creature* creature = owner->ToCreature())
                    if (creature->GetHomePosition().GetExactDist2d(i_target->GetPositionX(), i_target->GetPositionY()) > 60.0f)
                    return;
                break;
        }
    }

    float x, y, z;

    if (!i_offset)
    {
        if (i_target->IsWithinMeleeRange(owner))
            return;

        // To nearest random contact position.
        i_target->GetRandomContactPoint(owner, x, y, z, 0, MELEE_RANGE - 0.5f);
    }
    else
    {
        if (i_target->IsWithinDistInMap(owner, i_offset + 1.0f))
            return;

        // To at i_offset distance from target and i_angle from target facing.
        i_target->GetClosePoint(x, y, z, owner->GetObjectSize(), i_offset, i_angle);
    }

    /*
        We MUST not check the distance difference and avoid setting the new location for smaller distances.
        By that we risk having far too many GetContactPoint() calls freezing the whole system.
        In TargetedMovementGenerator<T>::DoUpdate() we check the distance to the target and at
        some range we calculate a new position. The calculation takes some processor cycles due to vmaps.
        If the distance to the target it too large to ignore,
        but the distance to the new contact point is short enough to be ignored,
        we will calculate a new contact point each update loop, but will never move to it.
        The system will freeze.
        ralf

        // We don't update Mob Movement, if the difference between New destination and last destination is < BothObjectSize
        float  bothObjectSize = i_target->GetObjectBoundingRadius() + owner->GetObjectBoundingRadius() + CONTACT_DISTANCE;
        if ( i_destinationHolder.HasDestination() && i_destinationHolder.GetDestinationDiff(x,y,z) < bothObjectSize )
            return;
    */

    D::_addUnitStateMove(owner);
    i_targetReached = false;
    i_recalculateTravel = false;

    owner->UpdateAllowedPositionZ(x, y, z);

    Movement::MoveSplineInit init(*owner);
    init.MoveTo(x, y, z);
    init.SetWalk(((D*)this)->EnableWalking());
    // Using the same condition for facing target as the one that is used for SetInFront on movement end - applies to ChaseMovementGenerator mostly.
    if (i_angle == 0.f)
        init.SetFacing(i_target.getTarget());
    init.Launch();
}

template<>
void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // Nothing to do for Player.
}

template<>
void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // Nothing to do for Player.
}

template<>
void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T,D>::DoUpdate(T* owner, uint32 diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    // Prevent movement while casting spells with cast time or channel time. Some creatures need adding here.
    if (owner->HasUnitState(UNIT_STATE_CASTING))
    {
        bool glyphwaterelemn = false;

        if (owner->GetOwner() && owner->GetOwner()->HasAura(63090) && owner->GetCharmInfo() && owner->GetCharmInfo()->HasCommandState(COMMAND_FOLLOW) && owner->ToPet() && owner->ToPet()->HasReactState(REACT_HELPER))
            glyphwaterelemn = true;

        if (!glyphwaterelemn)
        {
            if (!owner->IsStopped())
                owner->StopMoving();

            return true;
        }
    }

    // Prevent crash after creature killed pet.
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    i_recheckDistance.Update(diff);
    if (i_recheckDistance.Passed())
    {
        i_recheckDistance.Reset(100);
        // More distance let have better performance, less distance let have more sensitive reaction at target move.
        float allowed_dist = i_target->GetObjectSize() + owner->GetObjectSize() + MELEE_RANGE - 0.5f;
        float dist = (owner->movespline->FinalDestination() - G3D::Vector3(i_target->GetPositionX(), i_target->GetPositionY(), i_target->GetPositionZ())).squaredLength();
        if (dist >= allowed_dist * allowed_dist)
            _setTargetLocation(owner);
    }

    if (owner->movespline->Finalized())
    {
        static_cast<D*>(this)->MovementInform(owner);
        if (i_angle == 0.f && !owner->HasInArc(0.01f, i_target.getTarget()))
            owner->SetInFront(i_target.getTarget());

        if (!i_targetReached)
        {
            i_targetReached = true;
            static_cast<D*>(this)->_reachTarget(owner);
        }
    }
    else
    {
        if (i_recalculateTravel)
            _setTargetLocation(owner);
    }

    return true;
}

template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T* owner)
{
    if (!owner)
        return;

    if (owner->IsWithinMeleeRange(this->i_target.getTarget()))
        owner->Attack(this->i_target.getTarget(), true);
}

template<class T>
void ChaseMovementGenerator<T>::DoFinalize(T* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T* /*owner*/) { }

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature* owner)
{
    if (!owner)
        return;

    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle.
    if (owner->AI())
        owner->AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

// ========== FollowMovementGenerator ============ //

template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player* /*owner*/)
{
    // Nothing to do for Player.
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    // Pet only syncs speed with owner.
    if (!owner->isPet() || !i_target.isValid())
        return;

    if (i_target->GetGUID() != owner->GetOwnerGUID())
        return;

    owner->UpdateSpeed(MOVE_RUN,true);
    owner->UpdateSpeed(MOVE_WALK,true);
    owner->UpdateSpeed(MOVE_SWIM,true);
}

template<>
void FollowMovementGenerator<Player>::DoInitialize(Player* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->AddUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);

    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<>
void FollowMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->AddUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);

    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<class T>
void FollowMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::DoFinalize(T* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T* /*owner*/) { }

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature* owner)
{
    if (!owner)
        return;

    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (owner->AI())
        owner->AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

template void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::_setTargetLocation(Player* owner);
template void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::_setTargetLocation(Player* owner);
template void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature* owner);
template void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::_setTargetLocation(Creature* owner);

template bool TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::DoUpdate(Player* owner, uint32 diff);
template bool TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::DoUpdate(Player* owner, uint32 diff);
template bool TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::DoUpdate(Creature* owner, uint32 diff);
template bool TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::DoUpdate(Creature* owner, uint32 diff);

template void ChaseMovementGenerator<Player>::_reachTarget(Player* owner);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature* owner);
template void ChaseMovementGenerator<Player>::DoFinalize(Player* owner);
template void ChaseMovementGenerator<Creature>::DoFinalize(Creature* owner);
template void ChaseMovementGenerator<Player>::DoReset(Player* owner);
template void ChaseMovementGenerator<Creature>::DoReset(Creature* owner);
template void ChaseMovementGenerator<Player>::MovementInform(Player* owner);

template void FollowMovementGenerator<Player>::DoFinalize(Player* owner);
template void FollowMovementGenerator<Creature>::DoFinalize(Creature* owner);
template void FollowMovementGenerator<Player>::DoReset(Player* owner);
template void FollowMovementGenerator<Creature>::DoReset(Creature* owner);
template void FollowMovementGenerator<Player>::MovementInform(Player* owner);
