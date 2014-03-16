/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef _OBJECT_MOVEMENT_MGR_H
#define _OBJECT_MOVEMENT_MGR_H

#include "Common.h"
#include "UpdateMask.h"
#include "GridReference.h"
#include "ObjectDefines.h"
#include "Map.h"

#include <set>
#include <string>
#include <sstream>

/*** Positions holder. ***/
struct Position
{
    struct PositionXYZStreamer
    {
        explicit PositionXYZStreamer(Position& pos) : m_pos(&pos) { }
        Position* m_pos;
    };

    struct PositionXYZOStreamer
    {
        explicit PositionXYZOStreamer(Position& pos) : m_pos(&pos) { }
        Position* m_pos;
    };

    float m_positionX;
    float m_positionY;
    float m_positionZ;
    float m_orientation;

    bool operator==(Position const &a);

    inline bool operator!=(Position const &a)
    {
        return !(operator==(a));
    }

    void Relocate(float x, float y)
        { m_positionX = x; m_positionY = y;}
    void Relocate(float x, float y, float z)
        { m_positionX = x; m_positionY = y; m_positionZ = z; }
    void Relocate(float x, float y, float z, float orientation)
        { m_positionX = x; m_positionY = y; m_positionZ = z; SetOrientation(orientation); }
    void Relocate(Position const &pos)
        { m_positionX = pos.m_positionX; m_positionY = pos.m_positionY; m_positionZ = pos.m_positionZ; SetOrientation(pos.m_orientation); }
    void Relocate(Position const* pos)
        { m_positionX = pos->m_positionX; m_positionY = pos->m_positionY; m_positionZ = pos->m_positionZ; SetOrientation(pos->m_orientation); }
    void RelocateOffset(Position const &offset);
    void SetOrientation(float orientation)
    { m_orientation = NormalizeOrientation(orientation); }

    float GetPositionX() const { return m_positionX; }
    float GetPositionY() const { return m_positionY; }
    float GetPositionZ() const { return m_positionZ; }
    float GetOrientation() const { return m_orientation; }

    void GetPosition(float &x, float &y) const
        { x = m_positionX; y = m_positionY; }
    void GetPosition(float &x, float &y, float &z) const
        { x = m_positionX; y = m_positionY; z = m_positionZ; }
    void GetPosition(float &x, float &y, float &z, float &o) const
        { x = m_positionX; y = m_positionY; z = m_positionZ; o = m_orientation; }
    void GetPosition(Position* pos) const
    {
        if (pos)
            pos->Relocate(m_positionX, m_positionY, m_positionZ, m_orientation);
    }

    Position::PositionXYZStreamer PositionXYZStream()
    {
        return PositionXYZStreamer(*this);
    }
    Position::PositionXYZOStreamer PositionXYZOStream()
    {
        return PositionXYZOStreamer(*this);
    }

    bool IsPositionValid() const;

    float GetExactDist2dSq(float x, float y) const
        { float dx = m_positionX - x; float dy = m_positionY - y; return dx*dx + dy*dy; }
    float GetExactDist2d(const float x, const float y) const
        { return sqrt(GetExactDist2dSq(x, y)); }
    float GetExactDist2dSq(Position const* pos) const
        { float dx = m_positionX - pos->m_positionX; float dy = m_positionY - pos->m_positionY; return dx*dx + dy*dy; }
    float GetExactDist2d(Position const* pos) const
        { return sqrt(GetExactDist2dSq(pos)); }
    float GetExactDistSq(float x, float y, float z) const
        { float dz = m_positionZ - z; return GetExactDist2dSq(x, y) + dz*dz; }
    float GetExactDist(float x, float y, float z) const
        { return sqrt(GetExactDistSq(x, y, z)); }
    float GetExactDistSq(Position const* pos) const
        { float dx = m_positionX - pos->m_positionX; float dy = m_positionY - pos->m_positionY; float dz = m_positionZ - pos->m_positionZ; return dx*dx + dy*dy + dz*dz; }
    float GetExactDist(Position const* pos) const
        { return sqrt(GetExactDistSq(pos)); }

    void GetPositionOffsetTo(Position const & endPos, Position & retOffset) const;

    float GetAngle(Position const* pos) const;
    float GetAngle(float x, float y) const;
    float GetRelativeAngle(Position const* pos) const
        { return GetAngle(pos) - m_orientation; }
    float GetRelativeAngle(float x, float y) const { return GetAngle(x, y) - m_orientation; }
    void GetSinCos(float x, float y, float &vsin, float &vcos) const;

    bool IsInDist2d(float x, float y, float dist) const
        { return GetExactDist2dSq(x, y) < dist * dist; }
    bool IsInDist2d(Position const* pos, float dist) const
        { return GetExactDist2dSq(pos) < dist * dist; }
    bool IsInDist(float x, float y, float z, float dist) const
        { return GetExactDistSq(x, y, z) < dist * dist; }
    bool IsInDist(Position const* pos, float dist) const
        { return GetExactDistSq(pos) < dist * dist; }
    bool HasInArc(float arcangle, Position const* pos, float border = 2.0f) const;
    bool HasInLine(WorldObject const* target, float width) const;
    std::string ToString() const;

    // Modulos a radian orientation to the range of 0..2PI
    static float NormalizeOrientation(float o)
    {
        // fmod only supports positive numbers. Thus we have to emulate negative numbers.
        if (o < 0)
        {
            float mod = o *-1;
            mod = fmod(mod, 2.0f * static_cast<float>(M_PI));
            mod = -mod + 2.0f * static_cast<float>(M_PI);
            return mod;
        }
        return fmod(o, 2.0f * static_cast<float>(M_PI));
    }
};

ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer);
ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer);
ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer);
ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer);

/*** Movement holder. ***/
struct MovementInfo
{
    // common
    uint64 guid;
    uint32 flags;
    uint16 flags2;
    Position pos;
    uint32 time;

    // transport
    struct TransportInfo
    {
        void Reset()
        {
            guid = 0;
            pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
            seat = -1;
            time = 0;
            time2 = 0;
            time3 = 0;
        }

        uint64 guid;
        Position pos;
        int8 seat;
        uint32 time;
        uint32 time2;
        uint32 time3;
    } transport;

    // swimming/flying
    float pitch;

    // jumping
    struct JumpInfo
    {
        void Reset()
        {
            fallTime = 0;
            zspeed = sinAngle = cosAngle = xyspeed = 0.0f;
        }

        uint32 fallTime;

        float zspeed, sinAngle, cosAngle, xyspeed;

    } jump;

    // spline
    float splineElevation;

    MovementInfo() :
        guid(0), flags(0), flags2(0), time(0), pitch(0.0f), splineElevation(0.0f)
    {
        pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
        transport.Reset();
        jump.Reset();
    }

    uint32 GetMovementFlags() const { return flags; }
    void SetMovementFlags(uint32 flag) { flags = flag; }
    void AddMovementFlag(uint32 flag) { flags |= flag; }
    void RemoveMovementFlag(uint32 flag) { flags &= ~flag; }
    bool HasMovementFlag(uint32 flag) const { return flags & flag; }

    uint16 GetExtraMovementFlags() const { return flags2; }
    void SetExtraMovementFlags(uint16 flag) { flags2 = flag; }
    void AddExtraMovementFlag(uint16 flag) { flags2 |= flag; }
    void RemoveExtraMovementFlag(uint16 flag) { flags2 &= ~flag; }
    bool HasExtraMovementFlag(uint16 flag) const { return flags2 & flag; }

    void SetFallTime(uint32 time) { jump.fallTime = time; }

    void ResetTransport()
    {
        transport.Reset();
    }

    void ResetJump()
    {
        jump.Reset();
    }

    void OutDebug();
};

#endif
