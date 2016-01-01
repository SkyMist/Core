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

#ifndef TRINITY_TARGETEDMOVEMENTGENERATOR_H
#define TRINITY_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"
#include "Timer.h"
#include "Unit.h"

class TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGeneratorBase(Unit* owner) { i_target.link(owner, this); }
        void stopFollowing() { }

    protected:
        FollowerReference i_target;
};

template<class T, typename D>
class TargetedMovementGeneratorMedium : public MovementGeneratorMedium< T, D >, public TargetedMovementGeneratorBase
{
    protected:
        TargetedMovementGeneratorMedium(Unit* owner, float offset, float angle) : TargetedMovementGeneratorBase(owner), i_recheckDistance(0), i_offset(offset), i_angle(angle), i_recalculateTravel(false), i_targetReached(false) { }
        ~TargetedMovementGeneratorMedium() { }

    public:
        bool DoUpdate(T* owner, uint32 diff);
        Unit* GetTarget() const { return i_target.getTarget(); }

        void unitSpeedChanged() { i_recalculateTravel = true; }
        void UpdateFinalDistance(float fDistance);

    protected:
        void _setTargetLocation(T* owner);

        TimeTrackerSmall i_recheckDistance;
        float i_offset;
        float i_angle;
        bool i_recalculateTravel : 1;
        bool i_targetReached : 1;
};

template<class T>
class ChaseMovementGenerator : public TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >
{
    public:
        ChaseMovementGenerator(Unit* owner) : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(owner) { }
        ChaseMovementGenerator(Unit* owner, float offset, float angle) : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(owner, offset, angle) { }
        ~ChaseMovementGenerator() { }

        void DoInitialize(T* owner);
        void DoFinalize(T* owner);
        void DoReset(T* owner);
        void MovementInform(T* owner);

        static void _addUnitStateMove(T* owner)  { owner->AddUnitState(UNIT_STATE_CHASE_MOVE); }
        static void _clearUnitStateMove(T* owner) { owner->ClearUnitState(UNIT_STATE_CHASE_MOVE); }

        bool EnableWalking() const { return false;}
        bool _lostTarget(T* owner) const { return owner->getVictim() != this->GetTarget(); }
        void _reachTarget(T* owner);

        MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }
};

template<class T>
class FollowMovementGenerator : public TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >
{
    public:
        FollowMovementGenerator(Unit* owner) : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(owner){ }
        FollowMovementGenerator(Unit* owner, float offset, float angle) : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(owner, offset, angle) { }
        ~FollowMovementGenerator() { }

        void DoInitialize(T* owner);
        void DoFinalize(T* owner);
        void DoReset(T* owner);
        void MovementInform(T* owner);

        static void _addUnitStateMove(T* owner)  { owner->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }
        static void _clearUnitStateMove(T* owner) { owner->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }

        bool EnableWalking() const;
        bool _lostTarget(T* owner) const { return false; }
        void _reachTarget(T* owner) { }

        MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }

    private:
        void _updateSpeed(T* owner);
};

#endif
