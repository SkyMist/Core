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

#include "Object.h"
#include "ObjectMovement.h"
#include "Common.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "CreatureMovement.h"
#include "Player.h"
#include "PlayerMovement.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Transport.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "VMapFactory.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellAuraEffects.h"
#include "UpdateFieldFlags.h"
#include "TemporarySummon.h"
#include "Totem.h"
#include "OutdoorPvPMgr.h"
#include "MovementPacketBuilder.h"
#include "DynamicTree.h"
#include "Unit.h"
#include "UnitMovement.h"
#include "Group.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"

/*** Movement functions. ***/

void Object::BuildMovementUpdate(ByteBuffer* data, uint16 flags) const
{
    const Player* player = ToPlayer();
    const Unit* unit = ToUnit();
    const GameObject* go = ToGameObject();
    const DynamicObject* dob = ToDynObject();
    const AreaTrigger* atr = ToAreaTrigger();

    const WorldObject* wo =
        player ? (const WorldObject*)player : (
        unit ? (const WorldObject*)unit : (
        go ? (const WorldObject*)go : (
        dob ? (const WorldObject*)dob : atr ? (const WorldObject*)atr : (const WorldObject*)ToCorpse())));

    bool hasAreaTriggerData = isType(TYPEMASK_AREATRIGGER) && ((AreaTrigger*)this)->GetVisualRadius() != 0.0f;
    bool isSceneObject = false;

    data->WriteBit(false);                                                        // 676 UNK
    data->WriteBit((flags & UPDATEFLAG_VEHICLE) && unit);                         // hasVehicleData 488
    data->WriteBit(false);                                                        // 1044 UNK
    data->WriteBit((flags & UPDATEFLAG_ROTATION) && go);                          // hasRotation 512
    data->WriteBit(false);                                                        // fakeBit 0
    data->WriteBit((flags & UPDATEFLAG_LIVING) && unit);                          // isAlive 368
    data->WriteBit(false);                                                        // hasSceneObjectData 1032
    data->WriteBit(false);                                                        // fakeBit 2
    data->WriteBit(hasAreaTriggerData);                                           // hasAreaTrigger 668
    data->WriteBit(flags & UPDATEFLAG_SELF);                                      // hasSelf 680
    data->WriteBit(false);                                                        // 681 UNK
    data->WriteBit(false);                                                        // fakeBit 1
    data->WriteBit((flags & UPDATEFLAG_GO_TRANSPORT_POSITION) && wo);             // hasGameObjectData (hasTransportPosition) 424
    data->WriteBit(flags & (UPDATEFLAG_TRANSPORT | UPDATEFLAG_TRANSPORT_ARR));    // hasTransport 476
    data->WriteBit(flags & UPDATEFLAG_ANIMKITS);                                  // HasAnimKits 498
    data->WriteBit((flags & UPDATEFLAG_STATIONARY_POSITION) && wo);               // hasStationaryPosition 448
    data->WriteBit((flags & UPDATEFLAG_HAS_TARGET) && unit && unit->getVictim()); // hasTarget 464
    data->WriteBit(false);                                                        // fakeBit 3

    std::vector<uint32> transportFrames;
    if (flags & UPDATEFLAG_TRANSPORT_ARR)
    {
        const GameObjectTemplate* goInfo = ToGameObject()->GetGOInfo();
        if (goInfo->type == GAMEOBJECT_TYPE_TRANSPORT)
        {
            if (goInfo->transport.startFrame)
                transportFrames.push_back(goInfo->transport.startFrame);
            if (goInfo->transport.nextFrame1)
                transportFrames.push_back(goInfo->transport.nextFrame1);
            //if (goInfo->transport.nextFrame2)
            //    transportFrames.push_back(goInfo->transport.nextFrame2);
            //if (goInfo->transport.nextFrame3)
            //    transportFrames.push_back(goInfo->transport.nextFrame3);
        }
    }
    data->WriteBits(transportFrames.size(), 22);                                  // transportFramesCount 1068

    data->WriteBit(false);                                                        // 810 UNK
    data->WriteBit(false);                                                        // 1064 UNK

    // =========== Define needed stuff and make the checks. =================
    bool isLiving = (flags & UPDATEFLAG_LIVING) && unit;
    bool isTransportPos = (flags & UPDATEFLAG_GO_TRANSPORT_POSITION) && wo;
    bool isStationaryPos = (flags & UPDATEFLAG_STATIONARY_POSITION) && wo;

    ASSERT(!((isLiving && isTransportPos) || (isLiving && isStationaryPos)));
    if ((flags & UPDATEFLAG_HAS_TARGET) && unit && unit->getVictim())
    {
        ASSERT(unit->getVictim()->GetGUID());
    }
    
    if (flags & UPDATEFLAG_SELF)
    {
        ASSERT(isLiving);
    }

    if (unit)
    {
        const_cast<Unit*>(unit)->m_movementInfo.Normalize();
    }
    // =========== End of needed stuff, structure below. ================

    // if (1064 UNK)
    //     bits418 = packet.ReadBits(22);

    if ((flags & UPDATEFLAG_GO_TRANSPORT_POSITION) && wo)
    {
        ObjectGuid transGuid = wo->m_movementInfo.t_guid;

        data->WriteBit(transGuid[3]);
        data->WriteBit(transGuid[5]);
        data->WriteBit(transGuid[2]);
        data->WriteBit(transGuid[1]);
        data->WriteBit(transGuid[4]);
        data->WriteBit(wo->m_movementInfo.has_t_time3);              // HasTransportTime3
        data->WriteBit(wo->m_movementInfo.has_t_time2);              // HasTransportTime2
        data->WriteBit(transGuid[0]);
        data->WriteBit(transGuid[6]);
        data->WriteBit(transGuid[7]);
    }

    if ((flags & UPDATEFLAG_LIVING) && unit)
    {
        ObjectGuid guid = GetGUID();

        bool isSplineEnabled = unit->IsSplineEnabled();

        data->WriteBit(unit->m_movementInfo.t_guid != 0LL);         // Has transport data
        if (unit->m_movementInfo.t_guid)
        {
            ObjectGuid transGuid = unit->m_movementInfo.t_guid;

            data->WriteBit(transGuid[4]);
            data->WriteBit(transGuid[0]);
            data->WriteBit(transGuid[5]);
            data->WriteBit(transGuid[2]);
            data->WriteBit(transGuid[3]);
            data->WriteBit(unit->m_movementInfo.has_t_time2);                                                  // Has transport time 2
            data->WriteBit(transGuid[7]);
            data->WriteBit(transGuid[6]);
            data->WriteBit(transGuid[1]);
            data->WriteBit(unit->m_movementInfo.has_t_time3);                                                  // Has transport time 3
        }
        data->WriteBit(!unit->m_movementInfo.HavePitch);
        data->WriteBit(false);                                      // UNK.
        data->WriteBits(0, 19);                                     // bits168 UNK.
        data->WriteBit(guid[1]);
        data->WriteBit(!unit->m_movementInfo.flags2);
        data->WriteBit(false);                                      // UNK.
        data->WriteBit(!unit->m_movementInfo.HaveSplineElevation);

        if (unit->m_movementInfo.flags2)
            data->WriteBits(uint16(unit->m_movementInfo.flags2), 13);

        data->WriteBit(!unit->HasOrientation());                   // Has Orientation bit.
        data->WriteBit(!unit->m_movementInfo.time);
        data->WriteBit(!unit->m_movementInfo.flags);
        data->WriteBit(true);                                       // bitA8 Movement counter inversed.
        data->WriteBit(guid[2]);
        data->WriteBit(guid[6]);
        data->WriteBit(unit->m_movementInfo.hasFallData);
        data->WriteBit(guid[5]);
        data->WriteBit(guid[4]);
        data->WriteBit(guid[0]);

        if (unit->m_movementInfo.flags)
            data->WriteBits(uint32(unit->m_movementInfo.flags), 30);

        data->WriteBit(false);                                      // UNK.

        if (unit->m_movementInfo.hasFallData)
            data->WriteBit(unit->m_movementInfo.hasFallDirection);

        data->WriteBits(0, 22);                                     // bits98 UNK.

        data->WriteBit(guid[7]);
        data->WriteBit(isSplineEnabled);
        data->WriteBit(guid[3]);
        if (isSplineEnabled)
            Movement::PacketBuilder::WriteCreateBits(*unit->movespline, *data);
    }
    
    if (hasAreaTriggerData)
    {
        bool hasVisualRadius = ((AreaTrigger*)this)->GetVisualRadius() != 0.0f;

        data->WriteBit(false); // bit230 UNK.
        data->WriteBit(false); // bit258 UNK.
        data->WriteBit(false); // bit20E UNK.
        data->WriteBit(false); // bit20F UNK.
        data->WriteBit(false); // bit228 UNK.
        data->WriteBit(false); // bit20C UNK.
        data->WriteBit(false); // bit218 UNK.
        data->WriteBit(false); // bit20D UNK.
        data->WriteBit(false); // bit284 UNK.

        // if (bit284 UNK)
        // {
        //     data->WriteBits(bits25C, 21);
        //     data->WriteBits(bits26C, 21);
        // }

        data->WriteBit(false); // bit298 UNK.

        // if (bit298 UNK)
        //     data->WriteBits(bits288, 20);

        data->WriteBit(hasVisualRadius); // bit23C, visual radius.
        data->WriteBit(false); // bit210 UNK.
        data->WriteBit(false); // bit220 UNK.
    }

    if ((flags & UPDATEFLAG_HAS_TARGET) && unit && unit->getVictim())
    {
        ObjectGuid victimGuid = unit->getVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer

        uint8 bitOrder[8] = {4, 6, 3, 5, 0, 2, 7, 1};
        data->WriteBitInOrder(victimGuid, bitOrder);
    }

    if (flags & UPDATEFLAG_ANIMKITS)
    {
        data->WriteBit(1);                                                      // Missing AnimKit1
        data->WriteBit(1);                                                      // Missing AnimKit2
        data->WriteBit(1);                                                      // Missing AnimKit3
    }

    // if (810 UNK)
    //     data->WriteBits(bits2AA, 7);

    data->FlushBits();

    // Data
    if (flags & UPDATEFLAG_TRANSPORT_ARR)
        for (int i = 0; i < transportFrames.size(); ++i)
            *data << uint32(transportFrames[i]);

    if (hasAreaTriggerData)
    {
        bool hasVisualRadius = ((AreaTrigger*)this)->GetVisualRadius() != 0.0f;

        if (hasVisualRadius)
        {
            *data << float(((AreaTrigger*)this)->GetVisualRadius()); // float234 scale
            *data << float(((AreaTrigger*)this)->GetVisualRadius()); // float238 scale
        }

      /*if (bit284 UNK)
        {
            for (uint8 i = 0; i < bits26C; ++i)
            {
                *data << float(Float270+0);
                *data << float(Float270+1);
            }

            *data << float("Float27C");

            for (uint8 i = 0; i < bits25C; ++i)
            {
                *data << float(Float260+0);
                *data << float(Float260+1);
            }

            *data << float(Float280);
        }

        if (bit258 UNK)
        {
            *data << float(Float244);
            *data << float(Float250);
            *data << float(Float254);
            *data << float(Float248);
            *data << float(Float240);
            *data << float(Float24C);
        }*/

        *data << uint32(8); // Areatrigger id?

      /*if (bit298 UNK)
        {
            for (uint8 i = 0; i < bits288; ++i)
            {
                *data << float(Float28C+0");
                *data << float(Float28C+1");
                *data << float(Float28C+2");
            }
        }

        if (bit220 UNK)
            *data << uint32(int21C);

        if (bit218 UNK)
            *data << uint32(int214);

        if (bit230 UNK)
            *data << uint32(int22C);

        if (bit228 UNK)
            *data << uint32(int224); */
    }

    if ((flags & UPDATEFLAG_LIVING) && unit)
    {
        ObjectGuid guid = GetGUID();

        if (unit->m_movementInfo.hasFallData)
        {
            if (unit->m_movementInfo.hasFallDirection)
            {
                *data << float(unit->m_movementInfo.j_sinAngle);
                *data << float(unit->m_movementInfo.j_cosAngle);
                *data << float(unit->m_movementInfo.j_xyspeed);
            }

            *data << float(unit->m_movementInfo.j_zspeed);
            *data << uint32(unit->m_movementInfo.fallTime);
        }

        if (unit->IsSplineEnabled())
            Movement::PacketBuilder::WriteCreateData(*unit->movespline, *data);

        //for (uint8 i = 0; i < bits168 UNK; ++i)
        //    *data << float(unk);

        //for (uint8 i = 0; i < bits98 UNK; ++i)
        //    *data << uint32(Int9C);

        *data << float(unit->GetPositionZMinusOffset());
        *data << float(unit->GetPositionY());
        *data << float(unit->GetSpeed(MOVE_FLIGHT)); // 192
        data->WriteByteSeq(guid[6]);
        *data << float(unit->GetSpeed(MOVE_FLIGHT_BACK)); //196

        if (unit->m_movementInfo.t_guid != 0LL)
        {
            ObjectGuid transGuid = unit->m_movementInfo.t_guid;

            data->WriteByteSeq(transGuid[7]);
            data->WriteByteSeq(transGuid[4]);

            if (unit->m_movementInfo.has_t_time3)
                *data << uint32(unit->m_movementInfo.t_time3);

            *data << uint32(unit->GetTransTime());

            if (unit->m_movementInfo.has_t_time2)
                *data << uint32(unit->m_movementInfo.t_time2);

            *data << float(unit->GetTransOffsetO());
            *data << float(unit->GetTransOffsetX());
            data->WriteByteSeq(transGuid[6]);
            data->WriteByteSeq(transGuid[3]);
            data->WriteByteSeq(transGuid[2]);
            *data << float(unit->GetTransOffsetZ());
            *data << float(unit->GetTransOffsetY());
            *data << int8(unit->GetTransSeat());
            data->WriteByteSeq(transGuid[1]);
            data->WriteByteSeq(transGuid[0]);
            data->WriteByteSeq(transGuid[5]);
        }

        *data << float(unit->GetPositionX());
        data->WriteByteSeq(guid[2]);

        if (unit->m_movementInfo.HavePitch)
            *data << float(unit->m_movementInfo.pitch);

        *data << float(unit->GetSpeed(MOVE_SWIM)); //180
        data->WriteByteSeq(guid[1]);
        *data << float(unit->GetSpeed(MOVE_RUN_BACK)); // 176
        *data << float(unit->GetSpeed(MOVE_SWIM_BACK)); //184
        data->WriteByteSeq(guid[5]);
        *data << float(unit->GetSpeed(MOVE_TURN_RATE)); // 188
        data->WriteByteSeq(guid[3]);

        if (unit->m_movementInfo.HaveSplineElevation)
            *data << float(unit->m_movementInfo.splineElevation);

        // if (bitA8) // Movement counter.
        //     *data << uint32(IntA8); // Movement count.

        *data << float(unit->GetSpeed(MOVE_RUN)); // 172
        data->WriteByteSeq(guid[7]);
        *data << float(unit->GetSpeed(MOVE_WALK)); //168
        *data << float(unit->GetSpeed(MOVE_PITCH_RATE)); //200

        if (unit->m_movementInfo.time)
            *data << uint32(unit->m_movementInfo.time);

        data->WriteByteSeq(guid[4]);
        data->WriteByteSeq(guid[0]);

        if (unit->HasOrientation())
            *data << float(unit->GetOrientation());
    }

    if ((flags & UPDATEFLAG_HAS_TARGET) && unit && unit->getVictim())
    {
        ObjectGuid victimGuid = unit->getVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer

        data->WriteByteSeq(victimGuid[5]);
        data->WriteByteSeq(victimGuid[1]);
        data->WriteByteSeq(victimGuid[2]);
        data->WriteByteSeq(victimGuid[0]);
        data->WriteByteSeq(victimGuid[3]);
        data->WriteByteSeq(victimGuid[4]);
        data->WriteByteSeq(victimGuid[6]);
        data->WriteByteSeq(victimGuid[7]);
    }

    if ((flags & UPDATEFLAG_STATIONARY_POSITION) && wo)
    {
        *data << float(wo->GetOrientation());
        *data << float(wo->GetPositionX());
        *data << float(wo->GetPositionY());
        if (unit)
            *data << float(unit->GetPositionZMinusOffset());
        else
            *data << float(wo->GetPositionZ());
    }

    if ((flags & UPDATEFLAG_GO_TRANSPORT_POSITION) && wo)
    {
        ObjectGuid transGuid = wo->m_movementInfo.t_guid;

        *data << int8(wo->GetTransSeat());
        *data << float(wo->GetTransOffsetX());
        data->WriteByteSeq(transGuid[1]);
        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[5]);
        data->WriteByteSeq(transGuid[4]);
        if (wo->m_movementInfo.has_t_time3)
            *data << uint32(wo->m_movementInfo.t_time3);

        data->WriteByteSeq(transGuid[7]);
        *data << float(wo->GetTransOffsetO());
        *data << float(wo->GetTransOffsetZ());
        *data << float(wo->GetTransOffsetY());
        if (wo->m_movementInfo.has_t_time2)
            *data << uint32(wo->m_movementInfo.t_time2);

        data->WriteByteSeq(transGuid[3]);
        *data << uint32(wo->GetTransTime());
    }

    if (flags & UPDATEFLAG_TRANSPORT)
        *data << uint32(getMSTime());              // Transport path timer - getMSTime is wrong.
    else if (flags & UPDATEFLAG_TRANSPORT_ARR)
        *data << uint32(GetUInt32Value(GAMEOBJECT_LEVEL));

    // if (676 UNK)
    //     *data << uint32(int2A0);

    // if (810 UNK)
    //     packet.ReadBytes("Bytes", (int)bits2AA); -> string.

    if ((flags & UPDATEFLAG_ROTATION) && go)
        *data << uint64(go->GetRotation());

    if ((flags & UPDATEFLAG_VEHICLE) && unit)
    {
        *data << uint32(unit->GetVehicleKit()->GetVehicleInfo()->m_ID);
        *data << float(unit->GetOrientation());
    }

    // if (1044 UNK)
    //     *data << uint32(int410);

    if (flags & UPDATEFLAG_ANIMKITS)
    {
        *data << uint16(1);                                                      // Missing AnimKit1
        *data << uint16(1);                                                      // Missing AnimKit2
        *data << uint16(1);                                                      // Missing AnimKit3
    }

    // if (1064 UNK)
    //     for (uint8 i = 0; i < bits418; ++i)
    //         *data << uint32(Int3F8);

    if ((flags & UPDATEFLAG_LIVING) && unit && unit->IsSplineEnabled())
        Movement::PacketBuilder::WriteCreateGuid(*unit->movespline, *data);
}

// =================================================================================== //

/*** Positions functions. ***/

// Position-specific.

bool Position::operator==(Position const &a)
{
    return (G3D::fuzzyEq(a.m_positionX, m_positionX) &&
            G3D::fuzzyEq(a.m_positionY, m_positionY) &&
            G3D::fuzzyEq(a.m_positionZ, m_positionZ) &&
            G3D::fuzzyEq(a.m_orientation, m_orientation));
}

bool Position::HasInLine(WorldObject const* target, float width) const
{
    if (!HasInArc(M_PI, target))
        return false;
    width += target->GetObjectSize();
    float angle = GetRelativeAngle(target);
    return fabs(sin(angle)) * GetExactDist2d(target->GetPositionX(), target->GetPositionY()) < width;
}

std::string Position::ToString() const
{
    std::stringstream sstr;
    sstr << "X: " << m_positionX << " Y: " << m_positionY << " Z: " << m_positionZ << " O: " << m_orientation;
    return sstr.str();
}

void Position::MovePosition(Position &pos, float dist, float angle, WorldObject* object)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged.
    if (!SkyMistCore::IsValidMapCoord(destx, desty))
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "Position::MovePosition invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    ground = object->GetMap()->GetHeight(object->GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = object->GetMap()->GetHeight(object->GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist / 10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // Do not allow too big z changes.
        if (fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = object->GetMap()->GetHeight(object->GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = object->GetMap()->GetHeight(object->GetPhaseMask(), destx, desty, pos.m_positionZ, true);
            destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // We have correct destz now.
        else if (object->GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ() + 2.f, destx, desty, destz + 2.f, object->GetPhaseMask()))
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    SkyMistCore::NormalizeMapCoord(pos.m_positionX);
    SkyMistCore::NormalizeMapCoord(pos.m_positionY);
    object->UpdateGroundPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer)
{
    float x, y, z, o;
    buf >> x >> y >> z >> o;
    streamer.m_pos->Relocate(x, y, z, o);
    return buf;
}

ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer)
{
    float x, y, z;
    streamer.m_pos->GetPosition(x, y, z);
    buf << x << y << z;
    return buf;
}

ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer)
{
    float x, y, z;
    buf >> x >> y >> z;
    streamer.m_pos->Relocate(x, y, z);
    return buf;
}

ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer)
{
    float x, y, z, o;
    streamer.m_pos->GetPosition(x, y, z, o);
    buf << x << y << z << o;
    return buf;
}

void Position::RelocateOffset(const Position & offset)
{
    m_positionX = GetPositionX() + (offset.GetPositionX() * std::cos(GetOrientation()) + offset.GetPositionY() * std::sin(GetOrientation() + M_PI));
    m_positionY = GetPositionY() + (offset.GetPositionY() * std::cos(GetOrientation()) + offset.GetPositionX() * std::sin(GetOrientation()));
    m_positionZ = GetPositionZ() + offset.GetPositionZ();
    SetOrientation(GetOrientation() + offset.GetOrientation());
}

void Position::GetPositionOffsetTo(const Position & endPos, Position & retOffset) const
{
    float dx = endPos.GetPositionX() - GetPositionX();
    float dy = endPos.GetPositionY() - GetPositionY();

    retOffset.m_positionX = dx * std::cos(GetOrientation()) + dy * std::sin(GetOrientation());
    retOffset.m_positionY = dy * std::cos(GetOrientation()) - dx * std::sin(GetOrientation());
    retOffset.m_positionZ = endPos.GetPositionZ() - GetPositionZ();
    retOffset.SetOrientation(endPos.GetOrientation() - GetOrientation());
}

float Position::GetAngle(const Position* obj) const
{
    if (!obj)
        return 0;

    return GetAngle(obj->GetPositionX(), obj->GetPositionY());
}

// Return angle in range 0-2 * pi.
float Position::GetAngle(const float x, const float y) const
{
    float dx = x - GetPositionX();
    float dy = y - GetPositionY();

    float ang = atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * M_PI + ang;
    return ang;
}

void Position::GetSinCos(const float x, const float y, float &vsin, float &vcos) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;

    if (fabs(dx) < 0.001f && fabs(dy) < 0.001f)
    {
        float angle = (float)rand_norm()*static_cast<float>(2*M_PI);
        vcos = std::cos(angle);
        vsin = std::sin(angle);
    }
    else
    {
        float dist = sqrt((dx*dx) + (dy*dy));
        vcos = dx / dist;
        vsin = dy / dist;
    }
}

bool Position::HasInArc(float arc, const Position* obj) const
{
    // Always has self in arc.
    if (obj == this)
        return true;

    // Move arc to range 0-2 * pi.
    arc = NormalizeOrientation(arc);

    float angle = GetAngle(obj);
    angle -= m_orientation;

    // Move angle to range -pi - +pi
    angle = NormalizeOrientation(angle);
    if (angle > M_PI)
        angle -= 2.0f*M_PI;

    float lborder = -1 * (arc/2.0f);                        // in range -pi..0
    float rborder = (arc/2.0f);                             // in range 0..pi
    return ((angle >= lborder) && (angle <= rborder));
}

bool Position::IsPositionValid() const
{
    return SkyMistCore::IsValidMapCoord(m_positionX, m_positionY, m_positionZ, m_orientation);
}

// WorldObjects.

void WorldObject::GetNearPoint2D(float &x, float &y, float distance2d, float absAngle) const
{
    x = GetPositionX() + (GetObjectSize() + distance2d) * std::cos(absAngle);
    y = GetPositionY() + (GetObjectSize() + distance2d) * std::sin(absAngle);

    SkyMistCore::NormalizeMapCoord(x);
    SkyMistCore::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* searcher, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle) const
{
    GetNearPoint2D(x, y, distance2d+searcher_size, absAngle);
    z = GetPositionZ();
    if (!searcher || !searcher->ToCreature() || !searcher->GetMap()->Instanceable())
        UpdateAllowedPositionZ(x, y, z);
}

void WorldObject::GetClosePoint(float &x, float &y, float &z, float size, float distance2d /*= 0*/, float angle /*= 0*/) const
{
    // angle calculated from current orientation
    GetNearPoint(NULL, x, y, z, size, distance2d, GetOrientation() + angle);
}

void WorldObject::GetNearPosition(Position &pos, float dist, float angle)
{
    GetPosition(&pos);
    MovePosition(pos, dist, angle);
}

void WorldObject::GetFirstCollisionPosition(Position &pos, float dist, float angle)
{
    GetPosition(&pos);
    MovePositionToFirstCollision(pos, dist, angle);
}

void WorldObject::GetCollisionPositionBetween(Position &pos, float distMin, float distMax, float angle)
{
    GetPosition(&pos);
    MovePositionToCollisionBetween(pos, distMin, distMax, angle);
}

void WorldObject::GetRandomNearPosition(Position &pos, float radius)
{
    GetPosition(&pos);
    MovePosition(pos, radius * (float)rand_norm(), (float)rand_norm() * static_cast<float>(2 * M_PI));
}

void WorldObject::GetContactPoint(const WorldObject* obj, float &x, float &y, float &z, float distance2d /*= CONTACT_DISTANCE*/) const
{
    // Angle to face `obj` to `this` using distance includes size of `obj`.
    GetNearPoint(obj, x, y, z, obj->GetObjectSize(), distance2d, GetAngle(obj));
}

void WorldObject::GetRandomPoint(const Position &pos, float distance, float &rand_x, float &rand_y, float &rand_z) const
{
    if (!distance)
    {
        pos.GetPosition(rand_x, rand_y, rand_z);
        return;
    }

    // angle to face `obj` to `this`
    float angle = (float)rand_norm()*static_cast<float>(2*M_PI);
    float new_dist = (float)rand_norm()*static_cast<float>(distance);

    rand_x = pos.m_positionX + new_dist * std::cos(angle);
    rand_y = pos.m_positionY + new_dist * std::sin(angle);
    rand_z = pos.m_positionZ;

    SkyMistCore::NormalizeMapCoord(rand_x);
    SkyMistCore::NormalizeMapCoord(rand_y);
    UpdateGroundPositionZ(rand_x, rand_y, rand_z);            // update to LOS height if available
}

void WorldObject::GetRandomPoint(const Position &srcPos, float distance, Position &pos) const
{
    float x, y, z;
    GetRandomPoint(srcPos, distance, x, y, z);
    pos.Relocate(x, y, z, GetOrientation());
}

void WorldObject::MovePosition(Position &pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!SkyMistCore::IsValidMapCoord(destx, desty))
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "WorldObject::MovePosition invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
            destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else if (IsWithinLOS(destx, desty, destz))
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    SkyMistCore::NormalizeMapCoord(pos.m_positionX);
    SkyMistCore::NormalizeMapCoord(pos.m_positionY);
    UpdateGroundPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

void WorldObject::MovePositionToFirstCollision(Position &pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    pos.m_positionZ += 2.0f;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!SkyMistCore::IsValidMapCoord(destx, desty))
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "WorldObject::MovePositionToFirstCollision invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    bool col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(), pos.m_positionX, pos.m_positionY, pos.m_positionZ+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // collision occurred
    if (col)
    {
        // move back a bit
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
    }

    // check dynamic collision
    col = GetMap()->getObjectHitPos(GetPhaseMask(), pos.m_positionX, pos.m_positionY, pos.m_positionZ+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // Collided with a gameobject
    if (col)
    {
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
    }

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
            destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    SkyMistCore::NormalizeMapCoord(pos.m_positionX);
    SkyMistCore::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

void WorldObject::MovePositionToCollisionBetween(Position &pos, float distMin, float distMax, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz, tempDestx, tempDesty, ground, floor;
    pos.m_positionZ += 2.0f;

    tempDestx = pos.m_positionX + distMin * std::cos(angle);
    tempDesty = pos.m_positionY + distMin * std::sin(angle);

    destx = pos.m_positionX + distMax * std::cos(angle);
    desty = pos.m_positionY + distMax * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!SkyMistCore::IsValidMapCoord(destx, desty))
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "WorldObject::MovePositionToFirstCollision invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    bool col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(), tempDestx, tempDesty, pos.m_positionZ+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // collision occurred
    if (col)
    {
        // move back a bit
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        distMax = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
    }

    // check dynamic collision
    col = GetMap()->getObjectHitPos(GetPhaseMask(), tempDestx, tempDesty, pos.m_positionZ+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // Collided with a gameobject
    if (col)
    {
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        distMax = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
    }

    float step = distMax/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
            destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    SkyMistCore::NormalizeMapCoord(pos.m_positionX);
    SkyMistCore::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z + 2.0f, true);
    if (new_z > INVALID_HEIGHT)
        z = new_z + 0.05f;                                   // just to be sure that we are not a few pixels under the surface
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z) const
{
    // Return this for now since MMAPS are not implemented and this prevents proper movement for creatures on Transports / Elevators.
    return;

    switch (GetTypeId())
    {
        case TYPEID_UNIT:
        {
            // non fly unit don't must be in air
            // non swim unit must be at ground (mostly speedup, because it don't must be in water and water level check less fast
            if (!ToCreature()->CanFly())
            {
                bool canSwim = ToCreature()->isPet() ? true : ToCreature()->canSwim();
                float ground_z = z;
                float max_z = canSwim
                    ? GetMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK))
                    : ((ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true)));
                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
                if (z < ground_z)
                    z = ground_z;
            }
            break;
        }
        case TYPEID_PLAYER:
        {
            // for server controlled moves playr work same as creature (but it can always swim)
            if (!ToPlayer()->CanFly())
            {
                float ground_z = z;
                float max_z = GetMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK));
                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
                if (z < ground_z)
                    z = ground_z;
            }
            break;
        }
        default:
        {
            float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
            if (ground_z > INVALID_HEIGHT)
                z = ground_z;
            break;
        }
    }
}

float WorldObject::GetDistance(const WorldObject* obj) const
{
    float d = GetExactDist(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(const Position &pos) const
{
    float d = GetExactDist(&pos) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(float x, float y, float z) const
{
    float d = GetExactDist(x, y, z) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(const WorldObject* obj) const
{
    float d = GetExactDist2d(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(float x, float y) const
{
    float d = GetExactDist2d(x, y) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistanceZ(const WorldObject* obj) const
{
    float dz = fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = dz - sizefactor;
    return (dist > 0 ? dist : 0);
}

bool WorldObject::IsWithinDist3d(float x, float y, float z, float dist) const
{
    return IsInDist(x, y, z, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist3d(const Position* pos, float dist) const
{
    return IsInDist(pos, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist2d(float x, float y, float dist) const
{
    return IsInDist2d(x, y, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist2d(const Position* pos, float dist) const
{
    return IsInDist2d(pos, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D /*= true*/) const
{
    return obj && _IsWithinDist(obj, dist2compare, is3D);
}

bool WorldObject::IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D /*= true*/) const
{
    return obj && IsInMap(obj) && InSamePhase(obj) && _IsWithinDist(obj, dist2compare, is3D);
}

bool WorldObject::IsWithinLOS(float ox, float oy, float oz) const
{
    /*float x, y, z;
    GetPosition(x, y, z);
    VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
    return vMapManager->isInLineOfSight(GetMapId(), x, y, z+2.0f, ox, oy, oz+2.0f);*/
    if (IsInWorld())
        return GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ() + 2.f, ox, oy, oz + 2.f, GetPhaseMask());

    return true;
}

bool WorldObject::IsWithinLOSInMap(const WorldObject* obj) const
{
    if (!IsInMap(obj))
        return false;

    float ox, oy, oz;
    obj->GetPosition(ox, oy, oz);

    if (obj->GetTypeId() == TYPEID_UNIT)
        switch (obj->GetEntry())
        {
            // Hack fix for Ice Tombs (Sindragosa encounter)
            case 36980:
            case 38320:
            case 38321:
            case 38322:
            // Hack fix for Burning Tendons (Spine of Deathwing)
            case 56341:
            case 56575:
                return true;
            default:
                break;
        }

    // AoE spells
    if (GetTypeId() == TYPEID_UNIT)
        switch (GetEntry())
        {
            // Hack fix for Ice Tombs (Sindragosa encounter)
            case 36980:
            case 38320:
            case 38321:
            case 38322:
            // Hack fix for Burning Tendons (Spine of Deathwing)
            case 56341:
            case 56575:
                return true;
            default:
                break;
        }

    // Hack fix for Alysrazor
    if (GetMapId() == 720 && GetAreaId() == 5766)
        if ((GetTypeId() == TYPEID_PLAYER) || (obj->GetTypeId() == TYPEID_PLAYER))
            return true;

    return IsWithinLOS(ox, oy, oz);
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    float dx1 = GetPositionX() - obj1->GetPositionX();
    float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = dx1*dx1 + dy1*dy1;
    if (is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += dz1*dz1;
    }

    float dx2 = GetPositionX() - obj2->GetPositionX();
    float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = dx2*dx2 + dy2*dy2;
    if (is3D)
    {
        float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += dz2*dz2;
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */, bool useSizeFactor /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    float sizefactor = useSizeFactor ? (GetObjectSize() + obj->GetObjectSize()) : 0.0f;

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange2d(float x, float y, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float distsq = dx*dx + dy*dy;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange3d(float x, float y, float z, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInAxe(const WorldObject* obj1, const WorldObject* obj2, float size) const
{
    if (!obj1 || !obj2)
        return false;

    float dist = GetExactDist2d(obj1->GetPositionX(), obj1->GetPositionY());

    if (!size)
        size = GetObjectSize() / 2;

    float angle = obj1->GetAngle(obj2);

    // not using sqrt() for performance
    return (size * size) >= GetExactDist2dSq(obj1->GetPositionX() + cos(angle) * dist, obj1->GetPositionY() + sin(angle) * dist);
}

bool WorldObject::isInFront(WorldObject const* target,  float arc) const
{
    return HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float arc) const
{
    return !HasInArc(2 * M_PI - arc, target);
}

bool WorldObject::IsInBetween(const WorldObject* obj1, const WorldObject* obj2, float size) const
{
    if (!obj1 || !obj2)
        return false;

    float dist = GetExactDist2d(obj1->GetPositionX(), obj1->GetPositionY());

    // not using sqrt() for performance
    if ((dist * dist) >= obj1->GetExactDist2dSq(obj2->GetPositionX(), obj2->GetPositionY()))
        return false;

    if (!size)
        size = GetObjectSize() / 2;

    float angle = obj1->GetAngle(obj2);

    // not using sqrt() for performance
    return (size * size) >= GetExactDist2dSq(obj1->GetPositionX() + cos(angle) * dist, obj1->GetPositionY() + sin(angle) * dist);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D) const
{
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float maxdist = dist2compare + sizefactor;

    if (m_transport && obj->GetTransport() &&  obj->GetTransport()->GetGUIDLow() == m_transport->GetGUIDLow())
    {
        float dtx = m_movementInfo.t_pos.m_positionX - obj->m_movementInfo.t_pos.m_positionX;
        float dty = m_movementInfo.t_pos.m_positionY - obj->m_movementInfo.t_pos.m_positionY;
        float disttsq = dtx * dtx + dty * dty;
        if (is3D)
        {
            float dtz = m_movementInfo.t_pos.m_positionZ - obj->m_movementInfo.t_pos.m_positionZ;
            disttsq += dtz * dtz;
        }
        return disttsq < (maxdist * maxdist);
    }

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    return distsq < maxdist * maxdist;
}

// MovementInfo.
void MovementInfo::Normalize()
{
    pos.m_orientation = Position::NormalizeOrientation(pos.m_orientation);
    t_pos.m_orientation = Position::NormalizeOrientation(t_pos.m_orientation);

    pitch = Position::NormalizePitch(pitch);

    if (hasFallDirection && !hasFallData)
        hasFallDirection = false;
}

// =================================================================================== //

/*** Logging functions. ***/

void MovementInfo::OutDebug()
{
    sLog->outInfo(LOG_FILTER_GENERAL, "MOVEMENT INFO");
    sLog->outInfo(LOG_FILTER_GENERAL, "guid " UI64FMTD, guid);
    sLog->outInfo(LOG_FILTER_GENERAL, "flags %u", flags);
    sLog->outInfo(LOG_FILTER_GENERAL, "flags2 %u", flags2);
    sLog->outInfo(LOG_FILTER_GENERAL, "time %u current time " UI64FMTD "", flags2, uint64(::time(NULL)));
    sLog->outInfo(LOG_FILTER_GENERAL, "position: `%s`", pos.ToString().c_str());
    if (t_guid)
    {
        sLog->outInfo(LOG_FILTER_GENERAL, "TRANSPORT:");
        sLog->outInfo(LOG_FILTER_GENERAL, "guid: " UI64FMTD, t_guid);
        sLog->outInfo(LOG_FILTER_GENERAL, "position: `%s`", t_pos.ToString().c_str());
        sLog->outInfo(LOG_FILTER_GENERAL, "seat: %i", t_seat);
        sLog->outInfo(LOG_FILTER_GENERAL, "time: %u", t_time);
        if (flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT)
            sLog->outInfo(LOG_FILTER_GENERAL, "time2: %u", t_time2);
    }

    if ((flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        sLog->outInfo(LOG_FILTER_GENERAL, "pitch: %f", pitch);

    sLog->outInfo(LOG_FILTER_GENERAL, "fallTime: %u", fallTime);
    if (flags & MOVEMENTFLAG_FALLING)
        sLog->outInfo(LOG_FILTER_GENERAL, "j_zspeed: %f j_sinAngle: %f j_cosAngle: %f j_xyspeed: %f", j_zspeed, j_sinAngle, j_cosAngle, j_xyspeed);

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
        sLog->outInfo(LOG_FILTER_GENERAL, "splineElevation: %f", splineElevation);
}
