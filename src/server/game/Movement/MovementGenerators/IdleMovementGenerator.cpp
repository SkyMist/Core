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

#include "IdleMovementGenerator.h"
#include "CreatureAI.h"
#include "Creature.h"

// ========== IdleMovementGenerator ============ //

IdleMovementGenerator si_idleMovement;

// StopMoving is needed to make a unit stop if its last movement generator expires but it should not be sent otherwise there are many redundant packets.
void IdleMovementGenerator::Initialize(Unit* owner)
{
    if (!owner)
        return;

    Reset(owner);
}

void IdleMovementGenerator::Reset(Unit* owner)
{
    if (!owner)
        return;

    if (!owner->IsStopped())
        owner->StopMoving();
}

bool IdleMovementGenerator::Update(Unit* owner, uint32 diff)
{
    return true;
}

void IdleMovementGenerator::Finalize(Unit* owner) { }

// ========== RotateMovementGenerator ============ //

void RotateMovementGenerator::Initialize(Unit* owner)
{
    if (!owner)
        return;

    if (!owner->IsStopped())
        owner->StopMoving();

    if (!owner->isAlive())
        return;

    if (owner->getVictim())
        owner->SetInFront(owner->getVictim());

    owner->AddUnitState(UNIT_STATE_ROTATING);

    owner->AttackStop();
}

void RotateMovementGenerator::Reset(Unit* owner)
{
    if (!owner)
        return;

    Initialize(owner);
}

bool RotateMovementGenerator::Update(Unit* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    float angle = owner->GetOrientation();

    if (m_direction == ROTATE_DIRECTION_LEFT)
    {
        angle += (float)diff * static_cast<float>(M_PI * 2) / m_maxDuration;
        while (angle >= static_cast<float>(M_PI * 2)) angle -= static_cast<float>(M_PI * 2);
    }
    else
    {
        angle -= (float)diff * static_cast<float>(M_PI * 2) / m_maxDuration;
        while (angle < 0) angle += static_cast<float>(M_PI * 2);
    }

    owner->SetOrientation(angle);   // UpdateSplinePosition does not set orientation with UNIT_STATE_ROTATING.
    owner->SetFacingTo(angle);      // Send spline movement to clients.

    if (m_duration > diff)
        m_duration -= diff;
    else
        return false;

    return true;
}

void RotateMovementGenerator::Finalize(Unit* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_ROTATING);
    if (owner->GetTypeId() == TYPEID_UNIT)
        owner->ToCreature()->AI()->MovementInform(ROTATE_MOTION_TYPE, 0);
}

// ========== DistractMovementGenerator ============ //

void DistractMovementGenerator::Initialize(Unit* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->AddUnitState(UNIT_STATE_DISTRACTED);
}

void DistractMovementGenerator::Reset(Unit* owner)
{
    if (!owner)
        return;

    Initialize(owner);
}

bool DistractMovementGenerator::Update(Unit* owner, uint32 diff)
{
    if (diff > m_timer)
        return false;

    m_timer -= diff;
    return true;
}

void DistractMovementGenerator::Finalize(Unit* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_DISTRACTED);
}

// ========== AssistanceDistractMovementGenerator ============ //

void AssistanceDistractMovementGenerator::Finalize(Unit* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_DISTRACTED);
    owner->ToCreature()->SetReactState(REACT_AGGRESSIVE);
}
