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

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

// ========== PointMovementGenerator ============ //

template<class T>
void PointMovementGenerator<T>::DoInitialize(T* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    if (!owner->IsStopped())
        owner->StopMoving();

    owner->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    i_recalculateSpeed = false;
    Movement::MoveSplineInit init(owner);
    init.MoveTo(i_x, i_y, i_z);
    if (speed > 0.0f)
        init.SetVelocity(speed);
    init.Launch();
}

template<class T>
void PointMovementGenerator<T>::DoReset(T* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    if (!owner->IsStopped())
        owner->StopMoving();

    owner->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
}

template<class T>
bool PointMovementGenerator<T>::DoUpdate(T* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
    {
        owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    if (i_recalculateSpeed && !owner->movespline->Finalized())
    {
        i_recalculateSpeed = false;

        Movement::MoveSplineInit init(owner);
        init.MoveTo(i_x, i_y, i_z);
        if (speed > 0.0f) // Default value for point motion type is 0.0f, if 0.0f -> spline will use GetSpeed() on owner.
            init.SetVelocity(speed);
        init.Launch();
    }

    return !owner->movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>::DoFinalize(T* owner)
{
    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (owner->movespline->Finalized())
        MovementInform(owner);
}

// Hack stuff!! ToDo: remove.
enum specialSpells
{
    BABY_ELEPHANT_TAKES_A_BATH  = 108938,
    BABY_ELEPHANT_TAKES_A_BATH_2= 108937,
    MONK_CLASH                  = 126452,
    MONK_CLASH_IMPACT           = 126451,
};

template<class T>
void PointMovementGenerator<T>::MovementInform(T* owner) { }

template <> void PointMovementGenerator<Creature>::MovementInform(Creature* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    if (owner->AI())
        owner->AI()->MovementInform(POINT_MOTION_TYPE, id);

    switch (id)
    {
        case BABY_ELEPHANT_TAKES_A_BATH:
            owner->CastSpell(owner, BABY_ELEPHANT_TAKES_A_BATH_2, true);
            break;

        default: break;
    }
}

template <> void PointMovementGenerator<Player>::MovementInform(Player* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    switch (id)
    {
        case MONK_CLASH:
            owner->CastSpell(owner, MONK_CLASH_IMPACT, true);
            break;

        default: break;
    }
}

template void PointMovementGenerator<Player>::DoInitialize(Player* owner);
template void PointMovementGenerator<Creature>::DoInitialize(Creature* owner);
template void PointMovementGenerator<Player>::DoReset(Player* owner);
template void PointMovementGenerator<Creature>::DoReset(Creature* owner);
template void PointMovementGenerator<Player>::DoFinalize(Player* owner);
template void PointMovementGenerator<Creature>::DoFinalize(Creature* owner);
template bool PointMovementGenerator<Player>::DoUpdate(Player* owner, uint32 diff);
template bool PointMovementGenerator<Creature>::DoUpdate(Creature* owner, uint32 diff);

// ========== AssistanceMovementGenerator ============ //

void AssistanceMovementGenerator::DoFinalize(Unit* owner)
{
    if (!owner)
        return;

    if (!owner->isAlive())
        return;

    owner->ToCreature()->SetNoCallAssistance(false);
    owner->ToCreature()->CallAssistance();
    if (owner->isAlive())
        owner->GetMotionMaster()->MoveSeekAssistanceDistract(sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

// ========== EffectMovementGenerator ============ //

bool EffectMovementGenerator::Update(Unit* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (!owner->isAlive())
        return false;

    return !owner->movespline->Finalized();
}

void EffectMovementGenerator::Finalize(Unit* owner)
{
    if (!owner)
        return;

    MovementInform(owner);
}

void EffectMovementGenerator::MovementInform(Unit* owner)
{
    if (owner->GetTypeId() == TYPEID_UNIT)
    {
        // We need to restore previous movement since we have no proper states system.
        if (owner->isAlive() && !owner->HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
        {
            if (Unit* victim = owner->getVictim())
                owner->GetMotionMaster()->MoveChase(victim);
            else
                owner->GetMotionMaster()->Initialize();
        }

        if (Creature* creature = owner->ToCreature())
            if (creature->AI())
                creature->AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);
    }
}
