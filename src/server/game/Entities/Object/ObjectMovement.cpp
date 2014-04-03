/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
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
    bool self = flags & UPDATEFLAG_SELF;
    bool hasLiving = flags & UPDATEFLAG_LIVING;
    bool hasStationaryPosition = flags & UPDATEFLAG_STATIONARY_POSITION;
    bool hasGobjectRotation = flags & UPDATEFLAG_ROTATION;
    bool hasVehicle = flags & UPDATEFLAG_VEHICLE;
    bool hasTarget = flags & UPDATEFLAG_HAS_TARGET;
    bool hasAnimKits = flags & UPDATEFLAG_ANIMKITS;
    bool hasGOTransportFrames = flags & UPDATEFLAG_TRANSPORT_ARR;
    bool hasTransport = (flags & UPDATEFLAG_TRANSPORT) || hasGOTransportFrames;
    bool hasGoTransportPos = flags & UPDATEFLAG_GO_TRANSPORT_POSITION;          //proper update block for transports, sends transport data used for generating a smooth path, conversation with sebastian 09.03.2014 about transports

    std::vector<uint32> GOTransportFrames;
    if (hasGOTransportFrames)
    {
        const GameObjectTemplate* goInfo = ToGameObject()->GetGOInfo();
        if (goInfo->type == GAMEOBJECT_TYPE_TRANSPORT)
        {
            if (goInfo->transport.startFrame)
                GOTransportFrames.push_back(goInfo->transport.startFrame);
            if (goInfo->transport.nextFrame1)
                GOTransportFrames.push_back(goInfo->transport.nextFrame1);
            //if (goInfo->transport.nextFrame2)
            //    GOTransportFrames.push_back(goInfo->transport.nextFrame2);
            //if (goInfo->transport.nextFrame3)
            //    GOTransportFrames.push_back(goInfo->transport.nextFrame3);
        }
    }

    bool hasFallData;
    bool hasFallDirection;
    bool hasTransportData;
    bool hasTransportTime2;
    bool hasTransportTime3;
    bool hasSplineFinalTarget;
    bool hasTimestamp;
    bool hasMovementCounter;
    uint32 movementFlags;
    uint32 movementFlagsExtra;
    ByteBuffer SplineFinalTargetGUID(9);

    data->WriteBit(0);                                              //has Unk dword676
    data->WriteBit(hasVehicle);
    data->WriteBit(0);                                              //has unk dword1044
    data->WriteBit(hasGobjectRotation);
    data->WriteBit(0);                                              //unk byte0
    data->WriteBit(hasLiving);
    data->WriteBit(0);                                              //has Unk Large Block
    data->WriteBit(0);                                              //unk byte2
    data->WriteBit(0);                                              //has Unk Large Block2
    data->WriteBit(self);                                           
    data->WriteBit(0);                                              //unk byte681
    data->WriteBit(0);                                              //unk byte1
    data->WriteBit(hasGoTransportPos);
    data->WriteBit(hasTransport);
    data->WriteBit(hasAnimKits);
    data->WriteBit(hasStationaryPosition);
    data->WriteBit(hasTarget);
    data->WriteBit(0);                                              //unk byte3
    data->WriteBits(GOTransportFrames.size(), 22);
    data->WriteBit(0);                                              //has Unk string
    data->WriteBit(0);                                              //hasTaxiPathNodeRecord?

    // if (hasTaxiPathNodeRecord)
    //     data->WriteBits(MOtransportFrames.size(), 22);

    if (hasGoTransportPos)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.transport.guid;               

        data->WriteBit(transGuid[3]);
        data->WriteBit(transGuid[5]);
        data->WriteBit(transGuid[2]);
        data->WriteBit(transGuid[1]);
        data->WriteBit(transGuid[4]);
        data->WriteBit(self->m_movementInfo.transport.time3 && self->m_movementInfo.transport.guid); // Has GO transport time 3
        data->WriteBit(self->m_movementInfo.transport.time2 && self->m_movementInfo.transport.guid); // Has GO transport time 2
        data->WriteBit(transGuid[0]);
        data->WriteBit(transGuid[6]);
        data->WriteBit(transGuid[7]);
    }

    if (hasLiving)
    {
        Unit const* self = ToUnit();
        ObjectGuid guid = GetGUID();

        hasFallDirection = self->HasUnitMovementFlag(MOVEMENTFLAG_FALLING);
        hasFallData = hasFallDirection || self->m_movementInfo.jump.fallTime != 0;
        hasTransportData = self->m_movementInfo.transport.guid != 0;
        hasTransportTime2 = self->m_movementInfo.transport.time2 != 0;
        hasTransportTime3 = self->m_movementInfo.transport.time3 != 0;
        hasTimestamp = self->m_movementInfo.time != 0;
        hasMovementCounter = self->GetMovementCounter() != 0;
        movementFlags = self->GetUnitMovementFlags();
        movementFlagsExtra = self->GetExtraUnitMovementFlags();
        
        if (GetTypeId() == TYPEID_UNIT)
            movementFlags &= MOVEMENTFLAG_MASK_CREATURE_ALLOWED;

        data->WriteBit(hasTransportData);       // HasTransport

        if (hasTransportData)
        {
            ObjectGuid transGuid = self->m_movementInfo.transport.guid;
            data->WriteBit(transGuid[4]);
            data->WriteBit(transGuid[0]);
            data->WriteBit(transGuid[5]);
            data->WriteBit(transGuid[2]);
            data->WriteBit(transGuid[3]);
            data->WriteBit(hasTransportTime2);                                  // Has transport time 2
            data->WriteBit(transGuid[7]);
            data->WriteBit(transGuid[6]);
            data->WriteBit(transGuid[1]);
            data->WriteBit(hasTransportTime3);                                  // Has transport time 3
        }

        data->WriteBit(!((movementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) ||
            (movementFlagsExtra & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)));       // HasPitch
        data->WriteBit(0);                                                      // HasSpline from movement packets
        data->WriteBits(0, 19);
        data->WriteBit(guid[1]);
        data->WriteBit(!movementFlagsExtra);
        data->WriteBit(0);                                                      // unk bit from movement packets
        data->WriteBit(!(movementFlags & MOVEMENTFLAG_SPLINE_ELEVATION));       // HasSplineElevation

        if (movementFlagsExtra)
           data->WriteBits(movementFlagsExtra, 13);

        data->WriteBit(G3D::fuzzyEq(self->GetOrientation(), 0.0f)); //!G3D::fuzzyEq(self->GetOrientation(), 0.0f)

        data->WriteBit(!hasTimestamp);                                          // hasTimestamp
        data->WriteBit(!movementFlags);
        data->WriteBit(!hasMovementCounter);                                    // hasMovementCounter
        data->WriteBit(guid[2]);
        data->WriteBit(guid[6]);
        data->WriteBit(hasFallData);
        data->WriteBit(guid[5]);
        data->WriteBit(guid[4]);
        data->WriteBit(guid[0]);

        if (movementFlags)
            data->WriteBits(movementFlags, 30);

        data->WriteBit(0);

        if (hasFallData)
            data->WriteBit(hasFallDirection);

        data->WriteBits(0, 22);
        data->WriteBit(guid[7]);
        data->WriteBit(self->IsSplineEnabled());                                //HasSplineData
        data->WriteBit(guid[3]);
        if (self->IsSplineEnabled())
            Movement::PacketBuilder::WriteCreateBits(*self->movespline, *data);
    }

    if (hasTarget)
    {
        ObjectGuid victimGuid = ToUnit()->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteBit(victimGuid[4]);
        data->WriteBit(victimGuid[6]);
        data->WriteBit(victimGuid[3]);
        data->WriteBit(victimGuid[5]);
        data->WriteBit(victimGuid[0]);
        data->WriteBit(victimGuid[2]);
        data->WriteBit(victimGuid[7]);
        data->WriteBit(victimGuid[1]);
    }

    if (hasAnimKits)
    {
        data->WriteBit(1);                                  //hasAnimKit2, negated
        data->WriteBit(1);                                  //hasAnimKit3, negated
        data->WriteBit(1);                                  //hasAnimKit1, negated
    }

    data->FlushBits();

    if (hasGOTransportFrames)
    {
        for (std::vector<uint32>::iterator itr = GOTransportFrames.begin(); itr != GOTransportFrames.end(); ++itr)
            *data << uint32(*itr);
    }

    if (hasLiving)
    {
        Unit const* self = ToUnit();
        ObjectGuid guid = GetGUID();

        if (hasFallData)
        {
            if (hasFallDirection)
            {
                *data << float(self->m_movementInfo.jump.sinAngle);
                *data << float(self->m_movementInfo.jump.cosAngle);
                *data << float(self->m_movementInfo.jump.xyspeed);
            }
            
            *data << float(self->m_movementInfo.jump.zspeed);
            *data << uint32(self->m_movementInfo.jump.fallTime);
        }

        if (self->IsSplineEnabled())
            Movement::PacketBuilder::WriteCreateData(*self->movespline, *data, &hasSplineFinalTarget, &SplineFinalTargetGUID);

        *data << float(self->GetPositionZMinusOffset());

        //here is a skipped loop, controlled by the writebits(19) in the bit section

        *data << float(self->GetPositionY());
        *data << float(self->GetSpeed(MOVE_FLIGHT));
        data->WriteByteSeq(guid[6]);
        *data << float(self->GetSpeed(MOVE_FLIGHT_BACK));

        if (hasTransportData)
        {
            ObjectGuid transGuid = self->m_movementInfo.transport.guid;

            data->WriteByteSeq(transGuid[7]);
            data->WriteByteSeq(transGuid[4]);

            if (hasTransportTime3)
                *data << uint32(self->m_movementInfo.transport.time3);
	
            *data << uint32(self->GetTransTime());

            if (hasTransportTime2)
                *data << uint32(self->m_movementInfo.transport.time2);
	
            *data << float(self->GetTransOffsetO());
            *data << float(self->GetTransOffsetX());
            data->WriteByteSeq(transGuid[6]);
            data->WriteByteSeq(transGuid[3]);
            data->WriteByteSeq(transGuid[2]);
            *data << float(self->GetTransOffsetZ());
            *data << float(self->GetTransOffsetY());
            *data << int8(self->GetTransSeat());
            data->WriteByteSeq(transGuid[1]);
            data->WriteByteSeq(transGuid[0]);
            data->WriteByteSeq(transGuid[5]);
        }

        *data << float(self->GetPositionX());
        data->WriteByteSeq(guid[2]);

        if ((movementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (movementFlagsExtra & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
            *data << float(self->m_movementInfo.pitch);

        *data << float(self->GetSpeed(MOVE_SWIM));
        data->WriteByteSeq(guid[1]);
        *data << float(self->GetSpeed(MOVE_RUN_BACK));
        *data << float(self->GetSpeed(MOVE_SWIM_BACK));
        data->WriteByteSeq(guid[5]);
        *data << float(self->GetSpeed(MOVE_TURN_RATE));
        data->WriteByteSeq(guid[3]);

        if (movementFlags & MOVEMENTFLAG_SPLINE_ELEVATION)
            *data << float(self->m_movementInfo.splineElevation);

        if (hasMovementCounter)
            *data << uint32(self->GetMovementCounter());

        *data << float(self->GetSpeed(MOVE_RUN));
        data->WriteByteSeq(guid[7]);
        *data << float(self->GetSpeed(MOVE_WALK));
        *data << float(self->GetSpeed(MOVE_PITCH_RATE));

        if (hasTimestamp)
            *data << uint32(self->m_movementInfo.time);

        data->WriteByteSeq(guid[4]);
        data->WriteByteSeq(guid[0]);

        if (!G3D::fuzzyEq(self->GetOrientation(), 0.0f))
            *data << float(Position::NormalizeOrientation(self->GetOrientation()));     
    }

    if (hasTarget)
    {
        ObjectGuid victimGuid = ToUnit()->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteByteSeq(victimGuid[5]);
        data->WriteByteSeq(victimGuid[1]);
        data->WriteByteSeq(victimGuid[2]);
        data->WriteByteSeq(victimGuid[0]);
        data->WriteByteSeq(victimGuid[3]);
        data->WriteByteSeq(victimGuid[4]);
        data->WriteByteSeq(victimGuid[6]);
        data->WriteByteSeq(victimGuid[7]);
    }

    if (hasStationaryPosition)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        *data << float(self->GetStationaryO());
        *data << float(self->GetStationaryX());
        *data << float(self->GetStationaryY());

        if (Unit const* unit = ToUnit())
            *data << float(unit->GetPositionZMinusOffset());
        else
            *data << float(self->GetStationaryZ());
    }

    if (hasGoTransportPos)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.transport.guid;

        *data << int8(self->GetTransSeat());
        *data << float(self->GetTransOffsetX());
        data->WriteByteSeq(transGuid[1]);
        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[5]);
        data->WriteByteSeq(transGuid[4]);

        if (self->m_movementInfo.transport.time3 && self->m_movementInfo.transport.guid)
            *data << uint32(self->m_movementInfo.transport.time3);

        data->WriteByteSeq(transGuid[7]);
        *data << float(self->GetTransOffsetO());
        *data << float(self->GetTransOffsetZ());
        *data << float(self->GetTransOffsetY());

        if (self->m_movementInfo.transport.time2 && self->m_movementInfo.transport.guid)
            *data << uint32(self->m_movementInfo.transport.time2);

        data->WriteByteSeq(transGuid[3]);
        *data << uint32(self->GetTransTime());   
    }

    if (hasTransport)
    {
        GameObject const* go = ToGameObject();
        if (go && go->IsTransport())
        {
            if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT)
                *data << uint32(go->GetGOValue()->Transport.PathProgress); // Transport path timer.
            else if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT && hasGOTransportFrames)
                *data << uint32(GetUInt32Value(GAMEOBJECT_FIELD_LEVEL)); // Animation timer (calculated).
            else // !hasGOTransportFrames.
                *data << uint32(getMSTime());
        }
        else
            *data << uint32(getMSTime());
    }

    if (hasGobjectRotation)
        *data << uint64(ToGameObject()->GetRotation());

    if (hasVehicle)
    {
        *data << uint32(ToUnit()->GetVehicleKit()->GetVehicleInfo()->m_ID);
        if (ToUnit()->GetTransGUID())
            *data << float(ToUnit()->GetTransOffsetO());
        else
            *data << float(ToUnit()->GetOrientation());
    }

    if (hasAnimKits)
    {
    /*
        if (hasAnimKit1)
            *data << uint16(hasAnimKit1);
        if (hasAnimKit2)
            *data << uint16(hasAnimKit2);
        if (hasAnimKit3)
            *data << uint16(hasAnimKit3);
    */
    }

    /*if (hasTaxiPathNodeRecord)
    {
        for (std::vector<uint32>::iterator itr = MOtransportFrames.begin(); itr != MOtransportFrames.end(); ++itr)
            *data << uint32(*itr);
    } */

    if (hasLiving)
    {
        if (ToUnit()->IsSplineEnabled() && hasSplineFinalTarget)
            Movement::PacketBuilder::WriteFacingTargetPart(*ToUnit()->movespline, *data);
    }
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

// Return angle in range 0..2*pi
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

bool Position::HasInArc(float arc, const Position* obj, float border) const
{
    // always have self in arc
    if (obj == this)
        return true;

    // move arc to range 0.. 2*pi
    arc = NormalizeOrientation(arc);

    float angle = GetAngle(obj);
    angle -= m_orientation;

    // move angle to range -pi ... +pi
    angle = NormalizeOrientation(angle);
    if (angle > M_PI)
        angle -= 2.0f*M_PI;

    float lborder = -1 * (arc/border);                        // in range -pi..0
    float rborder = (arc/border);                             // in range 0..pi
    return ((angle >= lborder) && (angle <= rborder));
}

bool Position::IsPositionValid() const
{
    return Trinity::IsValidMapCoord(m_positionX, m_positionY, m_positionZ, m_orientation);
}

// WorldObjects.

void WorldObject::GetNearPoint2D(float &x, float &y, float distance2d, float absAngle) const
{
    x = GetPositionX() + (GetObjectSize() + distance2d) * std::cos(absAngle);
    y = GetPositionY() + (GetObjectSize() + distance2d) * std::sin(absAngle);

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* /*searcher*/, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle) const
{
    GetNearPoint2D(x, y, distance2d+searcher_size, absAngle);
    z = GetPositionZ();
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

void WorldObject::GetRandomNearPosition(Position &pos, float radius)
{
    GetPosition(&pos);
    MovePosition(pos, radius * (float)rand_norm(), (float)rand_norm() * static_cast<float>(2 * M_PI));
}

void WorldObject::GetContactPoint(const WorldObject* obj, float &x, float &y, float &z, float distance2d /*= CONTACT_DISTANCE*/) const
{
    // angle to face `obj` to `this` using distance includes size of `obj`
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

    Trinity::NormalizeMapCoord(rand_x);
    Trinity::NormalizeMapCoord(rand_y);
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
    if (!Trinity::IsValidMapCoord(destx, desty, pos.m_positionZ))
    {
        TC_LOG_FATAL("misc", "WorldObject::MovePosition: Object (TypeId: %u Entry: %u GUID: %u) has invalid coordinates X: %f and Y: %f were passed!",
            GetTypeId(), GetEntry(), GetGUIDLow(), destx, desty);
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
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
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
    if (!Trinity::IsValidMapCoord(destx, desty))
    {
        TC_LOG_FATAL("misc", "WorldObject::MovePositionToFirstCollision invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    bool col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(), pos.m_positionX, pos.m_positionY, pos.m_positionZ+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // collision occured
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

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z + 2.0f, true);
    if (new_z > INVALID_HEIGHT)
        z = new_z + 0.05f;                                   // just to be sure that we are not a few pixel under the surface
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z) const
{
    // Disable if MMaps not enabled.
    if (!sWorld->getBoolConfig(CONFIG_ENABLE_MMAPS))
        return;

    switch (GetTypeId())
    {
        case TYPEID_UNIT:
        {
            // non fly unit don't must be in air
            // non swim unit must be at ground (mostly speedup, because it don't must be in water and water level check less fast
            if (!ToCreature()->CanFly())
            {
                bool canSwim = ToCreature()->CanSwim();
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
        return GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ()+2.f, ox, oy, oz+2.f, GetPhaseMask());

    return true;
}

bool WorldObject::IsWithinLOSInMap(const WorldObject* obj) const
{
    if (!IsInMap(obj))
        return false;

    float ox, oy, oz;
    obj->GetPosition(ox, oy, oz);
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

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    float sizefactor = GetObjectSize() + obj->GetObjectSize();

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
    return (size * size) >= GetExactDist2dSq(obj1->GetPositionX() + std::cos(angle) * dist, obj1->GetPositionY() + std::sin(angle) * dist);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D) const
{
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float maxdist = dist2compare + sizefactor;

    if (m_transport && obj->GetTransport() &&  obj->GetTransport()->GetGUIDLow() == m_transport->GetGUIDLow())
    {
        float dtx = m_movementInfo.transport.pos.m_positionX - obj->m_movementInfo.transport.pos.m_positionX;
        float dty = m_movementInfo.transport.pos.m_positionY - obj->m_movementInfo.transport.pos.m_positionY;
        float disttsq = dtx * dtx + dty * dty;
        if (is3D)
        {
            float dtz = m_movementInfo.transport.pos.m_positionZ - obj->m_movementInfo.transport.pos.m_positionZ;
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

// =================================================================================== //

/*** Logging functions. ***/

void MovementInfo::OutDebug()
{
    TC_LOG_INFO("misc", "MOVEMENT INFO");
    TC_LOG_INFO("misc", "guid " UI64FMTD, guid);
    TC_LOG_INFO("misc", "flags %s (%u)", Movement::MovementFlags_ToString(flags).c_str(), flags);
    TC_LOG_INFO("misc", "flags2 %s (%u)", Movement::MovementFlagsExtra_ToString(flags2).c_str(), flags2);
    TC_LOG_INFO("misc", "time %u current time %u", time, getMSTime());
    TC_LOG_INFO("misc", "position: `%s`", pos.ToString().c_str());

    if (transport.guid)
    {
        TC_LOG_INFO("misc", "TRANSPORT:");
        TC_LOG_INFO("misc", "guid: " UI64FMTD, transport.guid);
        TC_LOG_INFO("misc", "position: `%s`", transport.pos.ToString().c_str());
        TC_LOG_INFO("misc", "seat: %i", transport.seat);
        TC_LOG_INFO("misc", "time: %u", transport.time);
        if (flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT)
            TC_LOG_INFO("misc", "time2: %u", transport.time2);
        if (transport.time3)
            TC_LOG_INFO("misc", "time3: %u", transport.time3);
    }

    if ((flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        TC_LOG_INFO("misc", "pitch: %f", pitch);

    if (flags & MOVEMENTFLAG_FALLING || jump.fallTime)
    {
        TC_LOG_INFO("misc", "fallTime: %u j_zspeed: %f", jump.fallTime, jump.zspeed);
        if (flags & MOVEMENTFLAG_FALLING)
            TC_LOG_INFO("misc", "j_sinAngle: %f j_cosAngle: %f j_xyspeed: %f", jump.sinAngle, jump.cosAngle, jump.xyspeed);
    }

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
        TC_LOG_INFO("misc", "splineElevation: %f", splineElevation);
}
