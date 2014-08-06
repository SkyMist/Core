/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
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
    return m_speed_rate[mtype]*(IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
}

void Unit::SetSpeed(UnitMoveType mtype, float rate, bool forced)
{
    if (rate < 0)
        rate = 0.0f;

    // Update speed only on change
    if (m_speed_rate[mtype] == rate)
        return;

    m_speed_rate[mtype] = rate;

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

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
        // and do it only for real sent packets and use run for run/mounted as client expected
        ++ToPlayer()->m_forced_speed_changes[mtype];

        if (!IsInCombat())
            if (Pet* pet = ToPlayer()->GetPet())
                pet->SetSpeed(mtype, m_speed_rate[mtype], forced);
    }

    static MovementStatusElements const speedVal = MSEExtraFloat;
    Movement::ExtraMovementStatusElement extra(&speedVal);
    extra.Data.floatData = GetSpeed(mtype);

    Movement::PacketSender(this, moveTypeToOpcode[mtype][0], moveTypeToOpcode[mtype][1], moveTypeToOpcode[mtype][2], &extra).Send();
}

void Unit::UpdateSpeed(UnitMoveType mtype, bool forced)
{
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
            return;
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
            TC_LOG_ERROR("entities.unit", "Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
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
                if ((IsPet() || IsGuardian()) && !IsInCombat() && pOwner) // Must check for owner or crash on "Tame Beast"
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
            /// @todo possible affect only on MOVE_RUN
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
    if (slow)
    {
        AddPct(speed, slow);
        if (float minSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED))
        {
            float min_speed = minSpeedMod / 100.0f;
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

    Movement::MoveSplineInit init(this);
    init.Stop();
}

bool Unit::IsSplineEnabled() const
{
    return movespline->Initialized() && !movespline->Finalized();
}

void Unit::WriteMovementInfo(WorldPacket& data, Movement::ExtraMovementStatusElement* extras /*= NULL*/)
{
    MovementInfo const& mi = m_movementInfo;

    bool hasMovementFlags = GetUnitMovementFlags() != 0;
    bool hasMovementFlags2 = GetExtraUnitMovementFlags() != 0;
    bool hasTimestamp = true;
    bool hasOrientation = !G3D::fuzzyEq(GetOrientation(), 0.0f);
    bool hasTransportData = GetTransGUID() != 0;
    bool hasSpline = IsSplineEnabled();

    // Fix player movement visibility during being CC-ed.
    bool isInCC = (GetTypeId() == TYPEID_PLAYER && (HasAuraType(SPELL_AURA_MOD_CONFUSE) || HasAuraType(SPELL_AURA_MOD_FEAR))) ? true : false;
    if (isInCC && !hasSpline)
        hasSpline = true;

    bool hasTransportTime2 = hasTransportData && m_movementInfo.transport.time2 != 0;
    bool hasTransportTime3 = hasTransportData && m_movementInfo.transport.time3 != 0;
    bool hasPitch = HasUnitMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || HasExtraUnitMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING);
    bool hasFallDirection = HasUnitMovementFlag(MOVEMENTFLAG_FALLING);
    bool hasFallData = hasFallDirection || m_movementInfo.jump.fallTime != 0;
    bool hasSplineElevation = HasUnitMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);

    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (!sequence)
    {
        TC_LOG_ERROR("network", "Unit::WriteMovementInfo: No movement sequence found for opcode %s", GetOpcodeNameForLogging(data.GetOpcode(), true).c_str());
        return;
    }

    ObjectGuid guid = GetGUID();
    ObjectGuid tguid = hasTransportData ? GetTransGUID() : 0;

    for (; *sequence != MSEEnd; ++sequence)
    {
        MovementStatusElements const& element = *sequence;

        switch (element)
        {
        case MSEHasGuidByte0:
        case MSEHasGuidByte1:
        case MSEHasGuidByte2:
        case MSEHasGuidByte3:
        case MSEHasGuidByte4:
        case MSEHasGuidByte5:
        case MSEHasGuidByte6:
        case MSEHasGuidByte7:
            data.WriteBit(guid[element - MSEHasGuidByte0]);
            break;
        case MSEHasTransportGuidByte0:
        case MSEHasTransportGuidByte1:
        case MSEHasTransportGuidByte2:
        case MSEHasTransportGuidByte3:
        case MSEHasTransportGuidByte4:
        case MSEHasTransportGuidByte5:
        case MSEHasTransportGuidByte6:
        case MSEHasTransportGuidByte7:
            if (hasTransportData)
                data.WriteBit(tguid[element - MSEHasTransportGuidByte0]);
            break;
        case MSEGuidByte0:
        case MSEGuidByte1:
        case MSEGuidByte2:
        case MSEGuidByte3:
        case MSEGuidByte4:
        case MSEGuidByte5:
        case MSEGuidByte6:
        case MSEGuidByte7:
            data.WriteByteSeq(guid[element - MSEGuidByte0]);
            break;
        case MSETransportGuidByte0:
        case MSETransportGuidByte1:
        case MSETransportGuidByte2:
        case MSETransportGuidByte3:
        case MSETransportGuidByte4:
        case MSETransportGuidByte5:
        case MSETransportGuidByte6:
        case MSETransportGuidByte7:
            if (hasTransportData)
                data.WriteByteSeq(tguid[element - MSETransportGuidByte0]);
            break;
        case MSEHasMovementFlags:
            data.WriteBit(!hasMovementFlags);
            break;
        case MSEHasMovementFlags2:
            data.WriteBit(!hasMovementFlags2);
            break;
        case MSEHasTimestamp:
            data.WriteBit(!hasTimestamp);
            break;
        case MSEHasOrientation:
            data.WriteBit(!hasOrientation);
            break;
        case MSEHasTransportData:
            data.WriteBit(hasTransportData);
            break;
        case MSEHasTransportTime2:
            if (hasTransportData)
                data.WriteBit(hasTransportTime2);
            break;
        case MSEHasTransportTime3:
            if (hasTransportData)
                data.WriteBit(hasTransportTime3);
            break;
        case MSEHasPitch:
            data.WriteBit(!hasPitch);
            break;
        case MSEHasFallData:
            data.WriteBit(hasFallData);
            break;
        case MSEHasFallDirection:
            if (hasFallData)
                data.WriteBit(hasFallDirection);
            break;
        case MSEHasSplineElevation:
            data.WriteBit(!hasSplineElevation);
            break;
        case MSEHasSpline:
            data.WriteBit(hasSpline);
            break;
        case MSEMovementFlags:
            if (hasMovementFlags)
                data.WriteBits(GetUnitMovementFlags(), 30);
            break;
        case MSEMovementFlags2:
            if (hasMovementFlags2)
                data.WriteBits(GetExtraUnitMovementFlags(), 13);
            break;
        case MSETimestamp:
            if (hasTimestamp)
                data << getMSTime();
            break;
        case MSEPositionX:
            data << GetPositionX();
            break;
        case MSEPositionY:
            data << GetPositionY();
            break;
        case MSEPositionZ:
            data << GetPositionZ();
            break;
        case MSEOrientation:
            if (hasOrientation)
                data << GetOrientation();
            break;
        case MSETransportPositionX:
            if (hasTransportData)
                data << GetTransOffsetX();
            break;
        case MSETransportPositionY:
            if (hasTransportData)
                data << GetTransOffsetY();
            break;
        case MSETransportPositionZ:
            if (hasTransportData)
                data << GetTransOffsetZ();
            break;
        case MSETransportOrientation:
            if (hasTransportData)
                data << GetTransOffsetO();
            break;
        case MSETransportSeat:
            if (hasTransportData)
                data << GetTransSeat();
            break;
        case MSETransportTime:
            if (hasTransportData)
                data << GetTransTime();
            break;
        case MSETransportTime2:
            if (hasTransportData && hasTransportTime2)
                data << mi.transport.time2;
            break;
        case MSETransportTime3:
            if (hasTransportData && hasTransportTime3)
                data << mi.transport.time3;
            break;
        case MSEPitch:
            if (hasPitch)
                data << mi.pitch;
            break;
        case MSEFallTime:
            if (hasFallData)
                data << mi.jump.fallTime;
            break;
        case MSEFallVerticalSpeed:
            if (hasFallData)
                data << mi.jump.zspeed;
            break;
        case MSEFallCosAngle:
            if (hasFallData && hasFallDirection)
                data << mi.jump.cosAngle;
            break;
        case MSEFallSinAngle:
            if (hasFallData && hasFallDirection)
                data << mi.jump.sinAngle;
            break;
        case MSEFallHorizontalSpeed:
            if (hasFallData && hasFallDirection)
                data << mi.jump.xyspeed;
            break;
        case MSESplineElevation:
            if (hasSplineElevation)
                data << mi.splineElevation;
            break;
        case MSECounterCount:
            data.WriteBits(0, 22);
            break;
        case MSECounter:
            data << m_movementCounter++;
            break;
        case MSEZeroBit:
            data.WriteBit(0);
            break;
        case MSEOneBit:
        case MSEHasUnkTime:
            data.WriteBit(1);
            break;
        case MSEExtraElement:
            extras->WriteNextElement(data);
            break;
        case MSEUintCount:
            data << uint32(0);
            break;
        case MSEUnkTime:
            // data << uint32(0); Don't write anything here as we put the inversed bool false.
            break;
        case MSEMountScale:
            data << float(1.0f);
            break;

        default:
            ASSERT(Movement::PrintInvalidSequenceElement(element, __FUNCTION__));
            break;
        }
    }
}

void Unit::MonsterMoveWithSpeed(float x, float y, float z, float speed, bool generatePath, bool forceDestination)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(x, y, z, generatePath, forceDestination);
    init.SetVelocity(speed);
    init.Launch();
}

void Unit::UpdateSplineMovement(uint32 t_diff)
{
    if (movespline->Finalized())
        return;

    movespline->updateState(t_diff);

    m_movesplineTimer.Update(t_diff);
    if (m_movesplineTimer.Passed())
        UpdateSplinePosition();
}

void Unit::UpdateSplinePosition()
{
    uint32 const positionUpdateDelay = 400;

    m_movesplineTimer.Reset(positionUpdateDelay);
    Movement::Location loc = movespline->ComputePosition();

    if (GetTransGUID())
    {
        Position& pos = m_movementInfo.transport.pos;
        pos.m_positionX = loc.x;
        pos.m_positionY = loc.y;
        pos.m_positionZ = loc.z;
        pos.SetOrientation(loc.orientation);

        if (TransportBase* transport = GetDirectTransport())
            transport->CalculatePassengerPosition(loc.x, loc.y, loc.z, &loc.orientation);
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
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_KNOCK_BACK, (1 + 8 + 4 + 4 + 4 + 4 + 4));

    data.WriteBit(guid[4]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);

    data.FlushBits();

    data << uint32(0);

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[3]);

    data << float(vcos);

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);

    data << float(speedZ);

    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[5]);

    data << float(vsin);

    data.WriteByteSeq(guid[4]);

    data << float(speedXY);

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

void Unit::SendMovementSetSplineAnim(Movement::AnimType anim)
{
    WorldPacket data(SMSG_SPLINE_MOVE_SET_ANIM, 8 + 4);
    data.append(GetPackGUID());
    data << uint32(anim);
    SendMessageToSet(&data, false);
}

void Unit::SendSetPlayHoverAnim(bool enable)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_SET_PLAY_HOVER_ANIM, 8 + 1);

    data.WriteBit(guid[4]);

    data.WriteBit(enable);

    data.WriteBit(guid[0]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[5]);

    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[0]);

    SendMessageToSet(&data, true);
}

bool Unit::IsFalling() const
{
    return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR) || movespline->isFalling();
}

bool Unit::SetWalk(bool enable)
{
    if (enable == IsWalking())
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

    ///@ TODO: Find proper opcode for walk mode setting in player mind controlling a player case
    if (enable)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_WALK_MODE, SMSG_SPLINE_MOVE_SET_WALK_MODE).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_RUN_MODE, SMSG_SPLINE_MOVE_SET_RUN_MODE).Send();

    return true;
}

bool Unit::SetDisableGravity(bool disable, bool packetOnly /*= false*/)
{
    if (!packetOnly)
    {
        if (disable == IsLevitating())
            return false;

        if (disable)
        {
            AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
            RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_SPLINE_ELEVATION);
            SetFall(false);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
            if (!HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY))
                SetFall(true);
        }
    }

    if (disable)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_GRAVITY_DISABLE, SMSG_MOVE_GRAVITY_DISABLE).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_GRAVITY_ENABLE, SMSG_MOVE_GRAVITY_ENABLE).Send();

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
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_START_SWIM, NULL_OPCODE).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_STOP_SWIM, NULL_OPCODE).Send();

    return true;
}

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
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_WATER_WALK, SMSG_MOVE_WATER_WALK).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_LAND_WALK, SMSG_MOVE_LAND_WALK).Send();

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
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_FEATHER_FALL, SMSG_MOVE_FEATHER_FALL).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_NORMAL_FALL, SMSG_MOVE_NORMAL_FALL).Send();

    return true;
}

bool Unit::SetHover(bool enable, bool packetOnly /*= false*/)
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

        SendSetPlayHoverAnim(enable);
    }

    if (enable)
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_SET_HOVER, SMSG_MOVE_SET_HOVER).Send();
    else
        Movement::PacketSender(this, SMSG_SPLINE_MOVE_UNSET_HOVER, SMSG_MOVE_UNSET_HOVER).Send();

    return true;
}

void Unit::SetFeared(bool apply)
{
    if (apply)
    {
        SetTarget(0);

        Unit* caster = NULL;
        Unit::AuraEffectList const& fearAuras = GetAuraEffectsByType(SPELL_AURA_MOD_FEAR);
        if (!fearAuras.empty())
            caster = ObjectAccessor::GetUnit(*this, fearAuras.front()->GetCasterGUID());
        if (!caster)
            caster = getAttackerForHelper();
        GetMotionMaster()->MoveFleeing(caster, fearAuras.empty() ? sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_FLEE_DELAY) : 0);             // caster == NULL processed in MoveFleeing
    }
    else
    {
        if (IsAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (GetVictim())
                SetTarget(GetVictim()->GetGUID());
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
        if (IsAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == CONFUSED_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (GetVictim())
                SetTarget(GetVictim()->GetGUID());
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

        // Creature specific
        if (GetTypeId() != TYPEID_PLAYER)
            StopMoving();
        else
            SetStandState(UNIT_STAND_STATE_STAND);

        SetRooted(true);

        CastStop();
    }
    else
    {
        if (IsAlive() && GetVictim())
            SetTarget(GetVictim()->GetGUID());

        // don't remove UNIT_FLAG_STUNNED for pet when owner is mounted (disabled pet's interface)
        Unit* owner = GetOwner();
        if (!owner || (owner->GetTypeId() == TYPEID_PLAYER && !owner->ToPlayer()->IsMounted()))
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if (!HasUnitState(UNIT_STATE_ROOT))         // prevent moving if it also has root effect
            SetRooted(false);
    }
}

void Unit::SetRooted(bool apply, bool packetOnly /*= false*/)
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
}

/*** Positions functions - Handled by UnitMovement. ***/

void Unit::SendTeleportPacket(Position& pos)
{
    ObjectGuid guid = GetGUID();
    ObjectGuid transGuid = GetTransGUID();

    WorldPacket data(SMSG_MOVE_TELEPORT, 38);
    data << float(GetPositionX());
    data << float(GetPositionZMinusOffset());
    data << float(GetPositionY());
    data << uint32(m_movementCounter++); // movement counter
    data << float(GetOrientation());

    data.WriteBit(guid[5]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[0]);
    data.WriteBit(uint64(transGuid));

    if (transGuid)
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

    data.WriteBit(0); // unknown
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);

    data.FlushBits();

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[5]);

    if (transGuid)
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
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendDirectMessage(&data); // Send the MSG_MOVE_TELEPORT packet to self.

    Relocate(&pos);

    if (GetTypeId() != TYPEID_PLAYER)
        SendMessageToSet(&data, true);
}

bool Unit::UpdatePosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!Trinity::IsValidMapCoord(x, y, z, orientation))
    {
        TC_LOG_DEBUG("entities.unit", "Unit::UpdatePosition(%f, %f, %f) .. bad coordinates!", x, y, z);
        return false;
    }

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
            GetMap()->CreatureRelocation(ToCreature(), x, y, z, orientation);
    }
    else if (turn)
        UpdateOrientation(orientation);

    // code block for underwater state update
    UpdateUnderwaterState(GetMap(), x, y, z);

    return (relocated || turn);
}

bool Unit::UpdatePosition(const Position &pos, bool teleport)
{
    return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport);
}

// ! Only server-side orientation update, does not broadcast to client.
void Unit::UpdateOrientation(float orientation)
{
    SetOrientation(orientation);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

// ! Only server-side height update, does not broadcast to client.
void Unit::UpdateHeight(float newZ)
{
    Relocate(GetPositionX(), GetPositionY(), newZ);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

void Unit::SetInFront(WorldObject const* target)
{
    if (!HasUnitState(UNIT_STATE_CANNOT_TURN))
        SetOrientation(GetAngle(target));
}

void Unit::SetFacingTo(float ori)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset(), false);
    if (GetTransport())
        init.DisableTransportPathTransformations(); // It makes no sense to target global orientation
    init.SetFacing(ori);
    init.Launch();
}

void Unit::SetFacingToObject(WorldObject* object)
{
    // Never set facing when already moving.
    if (!IsStopped())
        return;

    // ToDo: figure out under what conditions creature will move towards object instead of facing it where it currently is.
    Movement::MoveSplineInit init(this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(GetAngle(object));   // When on transport, GetAngle will still return global coordinates (and angle) that needs transforming
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
    float offset = HasUnitMovementFlag(MOVEMENTFLAG_HOVER) ? GetFloatValue(UNIT_FIELD_HOVER_HEIGHT) : 0.0f;

    return GetPositionZ() - offset;
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
