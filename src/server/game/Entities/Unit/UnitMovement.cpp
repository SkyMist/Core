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

#include "Unit.h"
#include "UnitMovement.h"
#include "ObjectMovement.h"
#include "Common.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "Battleground.h"
#include "CellImpl.h"
#include "ConditionMgr.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "CreatureGroups.h"
#include "Creature.h"
#include "CreatureMovement.h"
#include "Formulas.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "Log.h"
#include "MapManager.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvP.h"
#include "PassiveAI.h"
#include "PetAI.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerMovement.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Totem.h"
#include "Transport.h"
#include "UpdateFieldFlags.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include "WorldPacket.h"
#include "MovementStructures.h"
#include "WorldSession.h"

#include <math.h>

/*** Movement functions - Handled by UnitMovement. ***/

// Speeds functions.

float Unit::GetSpeed(UnitMoveType mtype) const
{
    return m_speed_rate[mtype] * (IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
}

void Unit::SetSpeed(UnitMoveType mtype, float rate, bool forced)
{
    if (fabs(rate) <= 0.00000023841858) // From client.
        rate = 1.0f;

    // Update speed only on change.
    bool clientSideOnly = m_speed_rate[mtype] == rate;

    m_speed_rate[mtype] = rate;

    if (!clientSideOnly)
        propagateSpeedChange();

    static Opcodes const moveTypeToOpcode[MAX_MOVE_TYPE][3] =
    {
        {SMSG_SPLINE_MOVE_SET_WALK_SPEED,        SMSG_MOVE_SET_WALK_SPEED,        SMSG_MOVE_UPDATE_WALK_SPEED       },
        {SMSG_SPLINE_MOVE_SET_RUN_SPEED,         SMSG_MOVE_SET_RUN_SPEED,         SMSG_MOVE_UPDATE_RUN_SPEED        },
        {SMSG_SPLINE_MOVE_SET_RUN_BACK_SPEED,    SMSG_MOVE_SET_RUN_BACK_SPEED,    SMSG_MOVE_UPDATE_RUN_BACK_SPEED   },
        {SMSG_SPLINE_MOVE_SET_SWIM_SPEED,        SMSG_MOVE_SET_SWIM_SPEED,        SMSG_MOVE_UPDATE_SWIM_SPEED       },
        {SMSG_SPLINE_MOVE_SET_SWIM_BACK_SPEED,   SMSG_MOVE_SET_SWIM_BACK_SPEED,   SMSG_MOVE_UPDATE_SWIM_BACK_SPEED  },
        {SMSG_SPLINE_MOVE_SET_TURN_RATE,         SMSG_MOVE_SET_TURN_RATE,         SMSG_MOVE_UPDATE_TURN_RATE        },
        {SMSG_SPLINE_MOVE_SET_FLIGHT_SPEED,      SMSG_MOVE_SET_FLIGHT_SPEED,      SMSG_MOVE_UPDATE_FLIGHT_SPEED     },
        {SMSG_SPLINE_MOVE_SET_FLIGHT_BACK_SPEED, SMSG_MOVE_SET_FLIGHT_BACK_SPEED, SMSG_MOVE_UPDATE_FLIGHT_BACK_SPEED},
        {SMSG_SPLINE_MOVE_SET_PITCH_RATE,        SMSG_MOVE_SET_PITCH_RATE,        SMSG_MOVE_UPDATE_PITCH_RATE       },
    };

    if (forced)
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
            // and do it only for real sent packets and use run for run/mounted as client expected
            ++ToPlayer()->m_forced_speed_changes[mtype];

            if (!isInCombat())
                if (Pet* pet = ToPlayer()->GetPet())
                    pet->SetSpeed(mtype, m_speed_rate[mtype], forced);
        }
    }

    // Don't build packets because we've got noone to send them to except self, and self is not created at client.
    if (!IsInWorld())
        return;

    static MovementStatusElements const speedVal = MSEExtraFloat;
    ExtraMovementStatusElement extra(&speedVal);
    extra.Data.floatData = GetSpeed(mtype);

    PacketSender(this, moveTypeToOpcode[mtype][0], moveTypeToOpcode[mtype][1], moveTypeToOpcode[mtype][2], &extra).Send();
}

void Unit::UpdateSpeed(UnitMoveType mtype, bool forced)
{
    //if (this->ToPlayer())
    //    sAnticheatMgr->DisableAnticheatDetection(this->ToPlayer());

    int32 main_speed_mod  = 0;
    float stack_bonus     = 1.0f;
    float non_stack_bonus = 1.0f;

    switch (mtype)
    {
        // Only apply debuffs
        case MOVE_FLIGHT_BACK:
        case MOVE_RUN_BACK:
        case MOVE_SWIM_BACK:
            break;
        case MOVE_WALK:
        case MOVE_RUN:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
                non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK) / 100.0f;
            }
            else
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
                non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK) / 100.0f;
            }
            break;
        }
        case MOVE_SWIM:
        {
            main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
            break;
        }
        case MOVE_FLIGHT:
        {
            if (GetTypeId() == TYPEID_UNIT && IsControlledByPlayer()) // not sure if good for pet
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS);

                // for some spells this mod is applied on vehicle owner
                int32 owner_speed_mod = 0;

                if (Unit* owner = GetCharmer())
                    owner_speed_mod = owner->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

                main_speed_mod = std::max(main_speed_mod, owner_speed_mod);
            }
            else if (IsMounted())
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS);
            }
            else             // Use not mount (shapeshift for example) auras (should stack)
                main_speed_mod  = GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) + GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

            non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK) / 100.0f;

            // Update speed for vehicle if available
            if (GetTypeId() == TYPEID_PLAYER && GetVehicle())
                GetVehicleBase()->UpdateSpeed(MOVE_FLIGHT, true);
            break;
        }
        default:
            return;
    }

    // now we ready for speed calculation
    float speed = std::max(non_stack_bonus, stack_bonus);
    if (main_speed_mod)
        AddPct(speed, main_speed_mod);

    switch (mtype)
    {
        case MOVE_RUN:
        case MOVE_SWIM:
        case MOVE_FLIGHT:
        {
            // Set creature speed rate
            if (GetTypeId() == TYPEID_UNIT)
            {
                Unit* pOwner = GetCharmerOrOwner();
                if ((isPet() || isGuardian()) && !isInCombat() && pOwner) // Must check for owner or crash on "Tame Beast"
                {
                    // For every yard over 5, increase speed by 0.01
                    //  to help prevent pet from lagging behind and despawning
                    float dist = GetDistance(pOwner);
                    float base_rate = 1.00f; // base speed is 100% of owner speed

                    if (dist < 5)
                        dist = 5;

                    float mult = base_rate + ((dist - 5) * 0.01f);

                    speed *= pOwner->GetSpeedRate(mtype) * mult; // pets derive speed from owner when not in combat
                }
                else
                    speed *= ToCreature()->GetCreatureTemplate()->speed_run;    // at this point, MOVE_WALK is never reached
            }
            // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
            // TODO: possible affect only on MOVE_RUN
            if (int32 normalization = GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
            {
                // Use speed from aura
                float max_speed = normalization / (IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
                if (speed > max_speed)
                    speed = max_speed;
            }
            break;
        }
        default:
            break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (ToCreature()->HasSearchedAssistance())
            speed *= 0.66f;                                 // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
    }

    // Apply strongest slow aura mod to speed
    int32 slow = GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);

    // Blizzard slow 50%
	if (HasAura(10))
	   slow -= 50;

    if (slow)
        AddPct(speed, slow);

    if (float minSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED))
    {
        float min_speed = minSpeedMod / 100.0f;
        if (speed < min_speed && mtype != MOVE_SWIM)
            speed = min_speed;
    }

    if (mtype == MOVE_SWIM)
    {
        if (float minSwimSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_INCREASE_MIN_SWIM_SPEED))
        {
            float min_speed = minSwimSpeedMod / 100.0f;
            if (speed < min_speed)
                speed = min_speed;
        }
    }

    SetSpeed(mtype, speed, forced);
}

// Movement functions.

void Unit::StopMoving()
{
    ClearUnitState(UNIT_STATE_MOVING);

    // not need send any packets if not in world or not moving
    if (!IsInWorld() || movespline->Finalized())
        return;

    // Update position using old spline
    UpdateSplinePosition();
    Movement::MoveSplineInit(this).Stop();
}

bool Unit::IsSplineEnabled() const
{
    return movespline->Initialized() && !movespline->Finalized();
}

void Unit::WriteMovementInfo(WorldPacket &data, ExtraMovementStatusElement* extras) const
{
    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (!sequence)
    {
        //sLog->outError(LOG_FILTER_NETWORKIO, "WorldSession::WriteMovementInfo: No movement sequence found for opcode 0x%04X", uint32(data.GetOpcode()));
        return;
    }

    MovementInfo const* mi = &m_movementInfo;

    bool hasMovementFlags = mi->GetMovementFlags() != 0;
    bool hasMovementFlags2 = mi->GetExtraMovementFlags() != 0;
    bool hasTransportData = mi->t_guid != 0LL;
    bool hasSpline = IsSplineEnabled();

    // Fix player movement visibility during being CC-ed.
    if (GetTypeId() == TYPEID_PLAYER && IsInCC() && !hasSpline)
        hasSpline = true;

    ObjectGuid guid = mi->guid;
    ObjectGuid tguid = mi->t_guid;

    for (; *sequence != MSEEnd; ++sequence)
    {
        MovementStatusElements const& element = *sequence;
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            data.WriteBit(guid[element - MSEHasGuidByte0]);
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteBit(tguid[element - MSEHasTransportGuidByte0]);
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.WriteByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        switch (element)
        {
            case MSEExtraElement:
                extras->WriteNextElement(data);
                break;
            case MSEUnkUIntCount:
                data.WriteBits(0, 22);
                break;
            case MSEUnkUIntLoop:
                for (uint8 m = 0; m < 0; ++m)
                {
                    data << uint32(0);
                }
                break;
            case MSECounter:
                data << uint32(0); // movement counter
                break;
            case MSEFlushBits:
                data.FlushBits();
                break;
            case MSEHasMovementFlags:
                data.WriteBit(!hasMovementFlags);
                break;
            case MSEHasMovementFlags2:
                data.WriteBit(!hasMovementFlags2);
                break;
            case MSEHasTimestamp:
                data.WriteBit(!mi->time);
                break;
            case MSEHasOrientation:
                data.WriteBit(!mi->pos.HasOrientation());
                break;
            case MSEHasTransportData:
                data.WriteBit(hasTransportData);
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    data.WriteBit(mi->has_t_time2);
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    data.WriteBit(mi->has_t_time3);
                break;
            case MSEHasPitch:
                data.WriteBit(!mi->HavePitch);
                break;
            case MSEHasFallData:
                data.WriteBit(mi->hasFallData);
                break;
            case MSEHasFallDirection:
                if (mi->hasFallData)
                    data.WriteBit(mi->hasFallDirection);
                break;
            case MSEHasSplineElevation:
                data.WriteBit(!mi->HaveSplineElevation);
                break;
            case MSEHasSpline:
                data.WriteBit(hasSpline);
                break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                    data.WriteBits(mi->flags, 30);
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                    data.WriteBits(mi->flags2, 13);
                break;
            case MSETimestamp:
                if (mi->time)
                    data << mi->time;
                break;
            case MSEPositionX:
                data << mi->pos.m_positionX;
                break;
            case MSEPositionY:
                data << mi->pos.m_positionY;
                break;
            case MSEPositionZ:
                data << mi->pos.m_positionZ;
                break;
            case MSEOrientation:
                if (mi->pos.HasOrientation())
                    data << mi->pos.GetOrientation();
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data << mi->t_pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data << mi->t_pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data << mi->t_pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    data << mi->t_pos.GetOrientation();
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data << mi->t_seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data << mi->t_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && mi->has_t_time2)
                    data << mi->t_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && mi->has_t_time3)
                    data << mi->t_time3;
                break;
            case MSEPitch:
                if (mi->HavePitch)
                    data << mi->pitch;
                break;
            case MSEFallTime:
                if (mi->hasFallData)
                    data << mi->fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (mi->hasFallData)
                    data << mi->j_zspeed;
                break;
            case MSEFallCosAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_cosAngle;
                break;
            case MSEFallSinAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_xyspeed;
                break;
            case MSESplineElevation:
                if (mi->HaveSplineElevation)
                    data << mi->splineElevation;
                break;
            case MSEZeroBit:
                data.WriteBit(0);
                break;
            case MSEOneBit:
                data.WriteBit(1);
                break;
            case MSEHasUnkTime:
                data.WriteBit(!mi->Alive32);
                break;
            case MSEUnkTime:
                if (mi->Alive32)
                    data << mi->Alive32;
                break;
            default:
                ASSERT(false && "Incorrect sequence element detected at WriteMovementInfo");
                break;
        }
    }
}

void Unit::MonsterMoveWithSpeed(float x, float y, float z, float speed)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(x, y, z);
    init.SetVelocity(speed);
    init.Launch();
}

void Unit::UpdateSplineMovement(uint32 t_diff)
{
    if (!IsSplineEnabled())
        return;

    // Update the Spline state.
    movespline->updateState(t_diff);

    // If the spline is Finalized disable it and update the mover's position.
    bool arrived = movespline->Finalized();

    if (arrived)
        DisableSpline();
 
    m_movesplineTimer.Update(t_diff);
    if (m_movesplineTimer.Passed() || arrived)
        UpdateSplinePosition();
}

void Unit::UpdateSplinePosition()
{
    uint32 const positionUpdateDelay = 400;

    m_movesplineTimer.Reset(positionUpdateDelay);
    Movement::Location loc = movespline->ComputePosition();

    if (GetTransGUID())
    {
        Position& pos = m_movementInfo.t_pos;
        pos.m_positionX = loc.x;
        pos.m_positionY = loc.y;
        pos.m_positionZ = loc.z;
        pos.SetOrientation(loc.orientation);

        if (TransportBase* transport = GetDirectTransport())
            transport->CalculatePassengerPosition(loc.x, loc.y, loc.z, loc.orientation);
    }

    if (HasUnitState(UNIT_STATE_CANNOT_TURN))
        loc.orientation = GetOrientation();

    UpdatePosition(loc.x, loc.y, loc.z, loc.orientation);
}

void Unit::DisableSpline()
{
    m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_FORWARD);
    movespline->_Interrupt();
}

void Unit::SendMoveKnockBack(Player* player, float speedXY, float speedZ, float vcos, float vsin)
{
    //if (this->ToPlayer())
    //    sAnticheatMgr->DisableAnticheatDetection(this->ToPlayer());

    m_movementInfo.hasFallData = true;
    m_movementInfo.hasFallDirection = true;

    m_movementInfo.fallTime = 0;
    m_movementInfo.j_cosAngle = vcos;
    m_movementInfo.j_sinAngle = vsin;
    m_movementInfo.j_xyspeed = speedXY;
    m_movementInfo.j_zspeed = speedZ;

    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_KNOCK_BACK, (1 + 8 + 4 + 4 + 4 + 4 + 4));
    
    uint8 bitOrder[8] = {4, 6, 0, 7, 5, 3, 1, 2};
    data.WriteBitInOrder(guid, bitOrder);

    data << uint32(0);

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[3]);

    data << float(speedZ); // vcos?

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);

    data << float(speedXY); // speedZ?

    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[5]);

    data << float(vcos); // vsin?

    data.WriteByteSeq(guid[4]);

    data << float(vsin); // speedXY?

    player->GetSession()->SendPacket(&data);
}

void Unit::KnockbackFrom(float x, float y, float speedXY, float speedZ)
{
    Player* player = ToPlayer();
    if (!player)
    {
        if (Unit* charmer = GetCharmer())
        {
            player = charmer->ToPlayer();
            if (player && player->m_mover != this)
                player = NULL;
        }
    }

    if (!player)
        GetMotionMaster()->MoveKnockbackFrom(x, y, speedXY, speedZ);
    else
    {
        float vcos, vsin;
        GetSinCos(x, y, vsin, vcos);
        SendMoveKnockBack(player, speedXY, -speedZ, vcos, vsin);
    }
}

void Unit::JumpTo(float speedXY, float speedZ, bool forward)
{
    float angle = forward ? 0 : M_PI;
    if (GetTypeId() == TYPEID_UNIT)
        GetMotionMaster()->MoveJumpTo(angle, speedXY, speedZ);
    else
    {
        float vcos = std::cos(angle+GetOrientation());
        float vsin = std::sin(angle+GetOrientation());
        SendMoveKnockBack(ToPlayer(), speedXY, -speedZ, vcos, vsin);
    }
}

void Unit::JumpTo(WorldObject* obj, float speedZ)
{
    float x, y, z;
    obj->GetContactPoint(this, x, y, z);
    float speedXY = GetExactDist2d(x, y) * 10.0f / speedZ;
    GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

void Unit::SendSetPlayHoverAnim(bool enable)
{
    ObjectGuid guid = GetGUID();

    WorldPacket data(SMSG_SET_PLAY_HOVER_ANIM, 10);

    data.WriteBit(guid[4]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[1]);
    data.WriteBit(enable);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[6]);

    data.FlushBits();

    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[6]);

    SendMessageToSet(&data, true);
}

bool Unit::SetWalk(bool enable)
{
    if (enable == IsWalking())
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

    if (enable)
        PacketSender(this, SMSG_SPLINE_MOVE_SET_WALK_MODE, SMSG_SPLINE_MOVE_SET_WALK_MODE).Send();
    else
        PacketSender(this, SMSG_SPLINE_MOVE_SET_RUN_MODE, SMSG_SPLINE_MOVE_SET_RUN_MODE).Send();

    return true;
}

bool Unit::SetDisableGravity(bool disable, bool /*packetOnly = false*/)
{
    if (disable == IsLevitating())
        return false;

    if (disable)
        AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);

    if (disable)
        PacketSender(this, SMSG_SPLINE_MOVE_GRAVITY_DISABLE, SMSG_MOVE_GRAVITY_DISABLE).Send();
    else
        PacketSender(this, SMSG_SPLINE_MOVE_GRAVITY_ENABLE, SMSG_MOVE_GRAVITY_ENABLE).Send();

    return true;
}

bool Unit::SetFall(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        return false;

    if (enable)
    {
        AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
        m_movementInfo.SetFallTime(0);
    }
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);

    return true;
}

bool Unit::SetSwim(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);

    if (enable)
        PacketSender(this, SMSG_SPLINE_MOVE_START_SWIM, NULL_OPCODE).Send();
    else
        PacketSender(this, SMSG_SPLINE_MOVE_STOP_SWIM, NULL_OPCODE).Send();

    return true;
}

/*
bool Unit::SetCanFly(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY))
        return false;

    if (enable)
    {
        AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
        RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_SPLINE_ELEVATION);
        SetFall(false);
    }
    else
    {
        RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_MASK_MOVING_FLY);
        if (!IsLevitating())
            SetFall(true);
    }

    if (enable)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_FLYING, SMSG_MOVE_SET_CAN_FLY).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_UNSET_FLYING, SMSG_MOVE_UNSET_CAN_FLY).Send();

    return true;
}
*/

void Unit::SetCanFly(bool enable)
{
    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
}

bool Unit::SetWaterWalking(bool enable, bool packetOnly /*= false */)
{
    if (!packetOnly)
    {
        if (enable == HasUnitMovementFlag(MOVEMENTFLAG_WATERWALKING))
            return false;

        if (enable)
            AddUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);
        else
            RemoveUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);
    }

    if (enable)
        PacketSender(this, SMSG_SPLINE_MOVE_SET_WATER_WALK, SMSG_MOVE_WATER_WALK).Send();
    else
        PacketSender(this, SMSG_SPLINE_MOVE_SET_LAND_WALK, SMSG_MOVE_LAND_WALK).Send();

    return true;
}

bool Unit::SetFeatherFall(bool enable, bool packetOnly /*= false */)
{
    if (!packetOnly)
    {
        if (enable == HasUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW))
            return false;

        if (enable)
            AddUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);
        else
            RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);
    }

    if (enable)
        PacketSender(this, SMSG_SPLINE_MOVE_SET_FEATHER_FALL, SMSG_MOVE_FEATHER_FALL).Send();
    else
        PacketSender(this, SMSG_SPLINE_MOVE_SET_NORMAL_FALL, SMSG_MOVE_NORMAL_FALL).Send();

    return true;
}

/* ToDo change with this.
bool Unit::SetHover(bool enable, bool packetOnly = false)
{
    if (!packetOnly)
    {
        if (enable == HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
            return false;

        if (enable)
        {
            //! No need to check height on ascent
            AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
            if (float hh = GetFloatValue(UNIT_FIELD_HOVER_HEIGHT))
                UpdateHeight(GetPositionZ() + hh);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
            if (float hh = GetFloatValue(UNIT_FIELD_HOVER_HEIGHT))
            {
                float newZ = GetPositionZ() - hh;
                UpdateAllowedPositionZ(GetPositionX(), GetPositionY(), newZ);
                UpdateHeight(newZ);
            }
        }
    }

    if (enable)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_HOVER, SMSG_MOVE_SET_HOVER).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_UNSET_HOVER, SMSG_MOVE_UNSET_HOVER).Send();

    return true;
}
*/

bool Unit::SetHover(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
        return false;

    if (enable)
    {
        //! No need to check height on ascent
        AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
        if (float hh = GetFloatValue(UNIT_FIELD_HOVERHEIGHT))
            UpdateHeight(GetPositionZ() + hh);
    }
    else
    {
        RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
        if (float hh = GetFloatValue(UNIT_FIELD_HOVERHEIGHT))
        {
            float newZ = GetPositionZ() - hh;
            UpdateAllowedPositionZ(GetPositionX(), GetPositionY(), newZ);
            UpdateHeight(newZ);
        }
    }

    return true;
}

void Unit::SendMovementHover(bool apply)
{
    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendMovementSetHover(HasUnitMovementFlag(MOVEMENTFLAG_HOVER));

    if (apply)
    {
        /*WorldPacket data(MSG_MOVE_HOVER, 64);
        data.append(GetPackGUID());
        BuildMovementPacket(&data);
        SendMessageToSet(&data, false);*/
    }
}

void Unit::SetFeared(bool apply)
{
    if (apply && !HasAuraType(SPELL_AURA_MOD_ROOT))
    {
        SetTarget(0);

        uint32 mechanic_mask = (1 << MECHANIC_FEAR) | (1 << MECHANIC_HORROR) | (1 << MECHANIC_TURN);

        Unit* caster = NULL;
        Unit::AuraEffectList const& fearAuras = GetAuraEffectsByMechanic(mechanic_mask);
        if (!fearAuras.empty())
            caster = ObjectAccessor::GetUnit(*this, fearAuras.front()->GetCasterGUID());
        if (!caster)
            caster = getAttackerForHelper();
        GetMotionMaster()->MoveFleeing(caster, fearAuras.empty() ? sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_FLEE_DELAY) : 0);             // caster == NULL processed in MoveFleeing
    }
    else
    {
        if (isAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (getVictim())
                SetTarget(getVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SetClientControl(this, !apply);
}

void Unit::SetConfused(bool apply)
{
    if (apply)
    {
        SetTarget(0);
        GetMotionMaster()->MoveConfused();
    }
    else
    {
        if (isAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == CONFUSED_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (getVictim())
                SetTarget(getVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SetClientControl(this, !apply);
}

void Unit::SetStunned(bool apply)
{
    if (apply)
    {
        SetTarget(0);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        // MOVEMENTFLAG_ROOT cannot be used in conjunction with MOVEMENTFLAG_MASK_MOVING (tested 3.3.5a)
        // this will freeze clients. That's why we remove MOVEMENTFLAG_MASK_MOVING before
        // setting MOVEMENTFLAG_ROOT
        StopMoving();
        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);

        // Creature specific
        if (GetTypeId() != TYPEID_PLAYER)
            ToCreature()->StopMoving();
        else
            SetStandState(UNIT_STAND_STATE_STAND);

        SendMoveRoot(0);
        CastStop();
    }
    else
    {
        if (isAlive() && getVictim())
            SetTarget(getVictim()->GetGUID());

        // don't remove UNIT_FLAG_STUNNED for pet when owner is mounted (disabled pet's interface)
        Unit* owner = GetOwner();
        if (!owner || (owner->GetTypeId() == TYPEID_PLAYER && !owner->ToPlayer()->IsMounted()))
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if (!HasUnitState(UNIT_STATE_ROOT))         // prevent moving if it also has root effect
        {
            SendMoveUnroot(0);
            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
    }
}

/* ToDo change with this.
void Unit::SetRooted(bool apply, bool packetOnly = false)
{
    if (!packetOnly)
    {
        if (apply)
        {
            // MOVEMENTFLAG_ROOT cannot be used in conjunction with MOVEMENTFLAG_MASK_MOVING (tested 3.3.5a)
            // this will freeze clients. That's why we remove MOVEMENTFLAG_MASK_MOVING before setting MOVEMENTFLAG_ROOT.
            RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING);
            AddUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
        else
            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
    }

    if (apply)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_ROOT, SMSG_MOVE_ROOT, SMSG_MOVE_ROOT).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_UNROOT, SMSG_MOVE_UNROOT, SMSG_MOVE_UNROOT).Send();
}*/

void Unit::SetRooted(bool apply)
{
    if (apply)
    {
        if (m_rootTimes > 0) // blizzard internal check?
            m_rootTimes++;

        // MOVEMENTFLAG_ROOT cannot be used in conjunction with MOVEMENTFLAG_MASK_MOVING (tested 3.3.5a) as this will freeze clients.
        // That's why we remove MOVEMENTFLAG_MASK_MOVING before setting MOVEMENTFLAG_ROOT.
        StopMoving();

        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);
        PacketSender(this, SMSG_SPLINE_MOVE_ROOT, SMSG_MOVE_ROOT, SMSG_MOVE_ROOT).Send();
    }
    else
    {
        if (!HasUnitState(UNIT_STATE_STUNNED))      // prevent moving if it also has stun effect
        {
            PacketSender(this, SMSG_SPLINE_MOVE_UNROOT, SMSG_MOVE_UNROOT, SMSG_MOVE_UNROOT).Send();
            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
    }
}

void Unit::SendMoveRoot(uint32 value)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_ROOT, 1 + 8 + 4);
    
    uint8 bitOrder[8] = {5, 3, 6, 0, 1, 4, 2, 7};
    data.WriteBitInOrder(guid, bitOrder);

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[5]);
    data << uint32(value);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[0]);

    SendMessageToSet(&data, true);
}

void Unit::SendMoveUnroot(uint32 value)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_UNROOT, 1 + 8 + 4);

    uint8 bitOrder[8] = { 0, 6, 4, 1, 2, 3, 7, 5 };
    data.WriteBitInOrder(guid, bitOrder);

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[4]);
    data << uint32(value);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[2]);

    SendMessageToSet(&data, true);
}

/*** Positions functions - Handled by UnitMovement. ***/

void Unit::SendTeleportPacket(Position& oldPos)
{
    if (GetTypeId() == TYPEID_UNIT)
        Relocate(&oldPos);

    ObjectGuid guid = GetGUID();
    ObjectGuid transGuid = GetTransGUID();

    bool hasTransport = (uint64(transGuid) != 0LL);
    bool teleportWithVehicle = IsOnVehicle(); 

    WorldPacket data(SMSG_MOVE_TELEPORT, 38);

    data << float(GetPositionX());
    data << float(GetPositionZMinusOffset());
    data << float(GetPositionY());
    data << uint32(0); // movement counter
    data << float(GetOrientation());

    data.WriteBit(guid[5]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[0]);
    data.WriteBit(hasTransport);

    if (hasTransport)
    {
        data.WriteBit(transGuid[6]);
        data.WriteBit(transGuid[4]);
        data.WriteBit(transGuid[2]);
        data.WriteBit(transGuid[5]);
        data.WriteBit(transGuid[3]);
        data.WriteBit(transGuid[0]);
        data.WriteBit(transGuid[7]);
        data.WriteBit(transGuid[1]);
    }

    data.WriteBit(teleportWithVehicle);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);

    data.FlushBits();

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[5]);

    if (hasTransport)
    {
        data.WriteByteSeq(transGuid[2]);
        data.WriteByteSeq(transGuid[1]);
        data.WriteByteSeq(transGuid[4]);
        data.WriteByteSeq(transGuid[0]);
        data.WriteByteSeq(transGuid[6]);
        data.WriteByteSeq(transGuid[5]);
        data.WriteByteSeq(transGuid[7]);
        data.WriteByteSeq(transGuid[3]);
    }

    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[4]);

    if (teleportWithVehicle)
        data << uint8(GetTransSeat() ? GetTransSeat() : 0);

    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);

    SendMessageToSet(&data, false);
    // if (GetTypeId() == TYPEID_PLAYER)
    //     ToPlayer()->SendDirectMessage(&data); // Send the SMSG_MOVE_TELEPORT packet to self.
    // Relocate(&pos);
    // if (GetTypeId() != TYPEID_PLAYER)
    //     SendMessageToSet(&data, true);
}

bool Unit::UpdatePosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!SkyMistCore::IsValidMapCoord(x, y, z, orientation))
        return false;

    bool turn = (GetOrientation() != orientation);
    bool relocated = (teleport || GetPositionX() != x || GetPositionY() != y || GetPositionZ() != z);

    if (turn)
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

    if (relocated)
    {
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);

        // move and update visible state if need
        if (GetTypeId() == TYPEID_PLAYER)
            GetMap()->PlayerRelocation(ToPlayer(), x, y, z, orientation);
        else
        {
            GetMap()->CreatureRelocation(ToCreature(), x, y, z, orientation);
            // code block for underwater state update
            UpdateUnderwaterState(GetMap(), x, y, z);
        }
    }
    else if (turn)
        UpdateOrientation(orientation);

    return (relocated || turn);
}

bool Unit::UpdatePosition(const Position &pos, bool teleport /*= false*/)
{
    return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport);
}

//! Only server-side orientation update, does not broadcast to client
void Unit::UpdateOrientation(float orientation)
{
    SetOrientation(orientation);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

//! Only server-side height update, does not broadcast to client
void Unit::UpdateHeight(float newZ)
{
    Relocate(GetPositionX(), GetPositionY(), newZ);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

void Unit::SetInFront(Unit const* target)
{
    if (!HasUnitState(UNIT_STATE_CANNOT_TURN))
        SetOrientation(GetAngle(target));
}

void Unit::SetFacingTo(float ori)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(ori);
    init.Launch();
}

void Unit::SetFacingToObject(WorldObject* object)
{
    // Never set facing when already moving.
    if (!IsStopped())
        return;

    Movement::MoveSplineInit init(this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(object->ToUnit());
    init.Launch();
}

bool Unit::isInFrontInMap(Unit const* target, float distance,  float arc) const
{
    return IsWithinDistInMap(target, distance) && HasInArc(arc, target);
}

bool Unit::isInBackInMap(Unit const* target, float distance, float arc) const
{
    return IsWithinDistInMap(target, distance) && !HasInArc(2 * M_PI - arc, target);
}

float Unit::GetPositionZMinusOffset() const
{
    float offset = HasUnitMovementFlag(MOVEMENTFLAG_HOVER) ? GetFloatValue(UNIT_FIELD_HOVERHEIGHT) : 0.0f;

    return GetPositionZ() - offset;
}

bool Unit::IsWithinCombatRange(const Unit* obj, float dist2compare) const
{
    if (!obj || !IsInMap(obj) || !InSamePhase(obj))
        return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx * dx + dy * dy + dz * dz;

    float sizefactor = GetCombatReach() + obj->GetCombatReach();
    float maxdist = dist2compare + sizefactor;

    return distsq < maxdist * maxdist;
}

bool Unit::IsWithinMeleeRange(const Unit* obj, float dist) const
{
    if (!obj || !IsInMap(obj) || !InSamePhase(obj))
        return false;

    if (GetTypeId() == TYPEID_PLAYER && (obj->GetTypeId() == TYPEID_PLAYER || obj->IsPetGuardianStuff()))
    {
        float maxDist = NOMINAL_MELEE_RANGE;   // auto-attack maximum distance
        if (dist > MELEE_RANGE)
        {
            maxDist = dist;
            if (IsMoving() && obj->IsMoving())
                maxDist += 2.5f;
        }

        return GetExactDist(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ()) < maxDist;
    }

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetMeleeReach() + obj->GetMeleeReach();
    float maxdist = dist + sizefactor;

    return distsq < maxdist * maxdist;
}

void Unit::GetRandomContactPoint(const Unit* obj, float &x, float &y, float &z, float distance2dMin, float distance2dMax) const
{
    float combat_reach = GetCombatReach();
    if (combat_reach < 0.1f) // sometimes bugged for players
        combat_reach = DEFAULT_COMBAT_REACH;

    uint32 attacker_number = getAttackers().size();
    if (attacker_number > 0)
        --attacker_number;

    GetNearPoint(obj, x, y, z, obj->GetCombatReach(), distance2dMin+(distance2dMax-distance2dMin) * (float)rand_norm()
        , GetAngle(obj) + (attacker_number ? (static_cast<float>(M_PI/2) - static_cast<float>(M_PI) * (float)rand_norm()) * float(attacker_number) / combat_reach * 0.3f : 0));
}
