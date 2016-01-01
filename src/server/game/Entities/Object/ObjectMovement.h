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

#ifndef _OBJECT_MOVEMENT_H
#define _OBJECT_MOVEMENT_H

#include "Common.h"
#include "UpdateMask.h"
#include "GridReference.h"
#include "ObjectDefines.h"
#include "Map.h"

#include <set>
#include <string>
#include <sstream>

/*** Movement flags. ***/

enum MovementFlags
{
    MOVEMENTFLAG_NONE = 0x00000000,
    MOVEMENTFLAG_FORWARD = 0x00000001,
    MOVEMENTFLAG_BACKWARD = 0x00000002,
    MOVEMENTFLAG_STRAFE_LEFT = 0x00000004,
    MOVEMENTFLAG_STRAFE_RIGHT = 0x00000008,
    MOVEMENTFLAG_LEFT = 0x00000010,
    MOVEMENTFLAG_RIGHT = 0x00000020,
    MOVEMENTFLAG_PITCH_UP = 0x00000040,
    MOVEMENTFLAG_PITCH_DOWN = 0x00000080,
    MOVEMENTFLAG_WALKING = 0x00000100,               // Walking.
    MOVEMENTFLAG_DISABLE_GRAVITY = 0x00000200,               // Former MOVEMENTFLAG_LEVITATING. This is used when walking is not possible.
    MOVEMENTFLAG_ROOT = 0x00000400,               // Must not be set along with MOVEMENTFLAG_MASK_MOVING.
    MOVEMENTFLAG_FALLING = 0x00000800,               // damage dealt on that type of falling.
    MOVEMENTFLAG_FALLING_FAR = 0x00001000,
    MOVEMENTFLAG_PENDING_STOP = 0x00002000,
    MOVEMENTFLAG_PENDING_STRAFE_STOP = 0x00004000,
    MOVEMENTFLAG_PENDING_FORWARD = 0x00008000,
    MOVEMENTFLAG_PENDING_BACKWARD = 0x00010000,
    MOVEMENTFLAG_PENDING_STRAFE_LEFT = 0x00020000,
    MOVEMENTFLAG_PENDING_STRAFE_RIGHT = 0x00040000,
    MOVEMENTFLAG_PENDING_ROOT = 0x00080000,
    MOVEMENTFLAG_SWIMMING = 0x00100000,               // Appears with fly flag also.
    MOVEMENTFLAG_ASCENDING = 0x00200000,               // Press "space" when flying.
    MOVEMENTFLAG_DESCENDING = 0x00400000,
    MOVEMENTFLAG_CAN_FLY = 0x00800000,               // Appears when unit can fly AND also walk.
    MOVEMENTFLAG_FLYING = 0x01000000,               // unit is actually flying. pretty sure this is only used for players. creatures use disable_gravity.
    MOVEMENTFLAG_SPLINE_ELEVATION = 0x02000000,               // used for flight paths.
    MOVEMENTFLAG_WATERWALKING = 0x04000000,               // prevent unit from falling through water.
    MOVEMENTFLAG_FALLING_SLOW = 0x08000000,               // active rogue safe fall spell (passive).
    MOVEMENTFLAG_HOVER = 0x10000000,               // hover, cannot jump.
    MOVEMENTFLAG_DISABLE_COLLISION = 0x20000000,

    // TODO: Check if PITCH_UP and PITCH_DOWN really belong here..
    MOVEMENTFLAG_MASK_MOVING =
    MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT |
    MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING |
    MOVEMENTFLAG_SPLINE_ELEVATION,

    MOVEMENTFLAG_MASK_TURNING =
    MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT,

    MOVEMENTFLAG_MASK_MOVING_FLY =
    MOVEMENTFLAG_FLYING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING,

    // Movement flags allowed for creature in CreateObject - we need to keep all other enabled serverside to properly calculate all movement.
    MOVEMENTFLAG_MASK_CREATURE_ALLOWED =
    MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT | MOVEMENTFLAG_SWIMMING |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER,

    //! TODO if needed: add more flags to this masks that are exclusive to players
    MOVEMENTFLAG_MASK_PLAYER_ONLY =
    MOVEMENTFLAG_FLYING,

    /// Movement flags that have change status opcodes associated for players.
    MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE = MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER
};

enum MovementFlags2
{
    MOVEMENTFLAG2_NONE = 0x00000000,
    MOVEMENTFLAG2_NO_STRAFE = 0x00000001,
    MOVEMENTFLAG2_NO_JUMPING = 0x00000002,
    MOVEMENTFLAG2_FULL_SPEED_TURNING = 0x00000004,
    MOVEMENTFLAG2_FULL_SPEED_PITCHING = 0x00000008,
    MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING = 0x00000010,
    MOVEMENTFLAG2_UNK7 = 0x00000020,
    MOVEMENTFLAG2_DISMISS_CONTROLLED_VEHICLE = 0x00000040,
    MOVEMENTFLAG2_UNK9 = 0x00000080,
    MOVEMENTFLAG2_UNK10 = 0x00000100,
    MOVEMENTFLAG2_INTERPOLATED_MOVEMENT = 0x00000200,
    MOVEMENTFLAG2_INTERPOLATED_TURNING = 0x00000400,
    MOVEMENTFLAG2_INTERPOLATED_PITCHING = 0x00000800,
};

/*** Positions holder. ***/

struct Position
{
    // Streams.
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

    Position::PositionXYZStreamer PositionXYZStream()
    {
        return PositionXYZStreamer(*this);
    }

    Position::PositionXYZOStreamer PositionXYZOStream()
    {
        return PositionXYZOStreamer(*this);
    }

    // Basic declare stuff.
    float m_positionX;
    float m_positionY;
    float m_positionZ;
    float m_orientation;

    // Basic check stuff.
    float GetPositionX() const                                     { return m_positionX; }
    float GetPositionY() const                                     { return m_positionY; }
    float GetPositionZ() const                                     { return m_positionZ; }
    float GetOrientation() const                                   { return m_orientation; }

    bool HasOrientation() const                                    { return m_orientation != 0.0f; }
    void SetOrientation(float orientation)                         { m_orientation = NormalizeOrientation(orientation); }

    // Positions.
    void GetPosition(float &x, float &y) const                     { x = m_positionX; y = m_positionY; }
    void GetPosition(float &x, float &y, float &z) const           { x = m_positionX; y = m_positionY; z = m_positionZ; }
    void GetPosition(float &x, float &y, float &z, float &o) const { x = m_positionX; y = m_positionY; z = m_positionZ; o = m_orientation; }
    void GetPosition(Position* pos) const                          { if (pos) pos->Relocate(m_positionX, m_positionY, m_positionZ, m_orientation); }

    bool IsPositionValid() const;
    void GetPositionOffsetTo(const Position & endPos, Position & retOffset) const;
    void MovePosition(Position &pos, float dist, float angle, WorldObject* object);

    // Relocations.
    void Relocate(float x, float y)                                { m_positionX = x; m_positionY = y;}
    void Relocate(float x, float y, float z)                       { m_positionX = x; m_positionY = y; m_positionZ = z; }
    void Relocate(float x, float y, float z, float orientation)    { m_positionX = x; m_positionY = y; m_positionZ = z; SetOrientation(orientation); }
    void Relocate(const Position &pos)                             { m_positionX = pos.m_positionX; m_positionY = pos.m_positionY; m_positionZ = pos.m_positionZ; SetOrientation(pos.m_orientation); }
    void Relocate(const Position* pos)                             { m_positionX = pos->m_positionX; m_positionY = pos->m_positionY; m_positionZ = pos->m_positionZ; SetOrientation(pos->m_orientation); }
    void RelocateOffset(const Position &offset);

    // Distances.
    float GetExactDist2dSq(float x, float y) const                 { float dx = m_positionX - x; float dy = m_positionY - y; return dx*dx + dy*dy; }
    float GetExactDist2d(const float x, const float y) const       { return sqrt(GetExactDist2dSq(x, y)); }
    float GetExactDist2dSq(const Position* pos) const              { float dx = m_positionX - pos->m_positionX; float dy = m_positionY - pos->m_positionY; return dx*dx + dy*dy; }
    float GetExactDist2d(const Position* pos) const                { return sqrt(GetExactDist2dSq(pos)); }
    float GetExactDistSq(float x, float y, float z) const          { float dz = m_positionZ - z; return GetExactDist2dSq(x, y) + dz*dz; }
    float GetExactDist(float x, float y, float z) const            { return sqrt(GetExactDistSq(x, y, z)); }
    float GetExactDistSq(const Position* pos) const                { float dx = m_positionX - pos->m_positionX; float dy = m_positionY - pos->m_positionY; float dz = m_positionZ - pos->m_positionZ; return dx*dx + dy*dy + dz*dz; }
    float GetExactDist(const Position* pos) const                  { return sqrt(GetExactDistSq(pos)); }

    bool IsInDist2d(float x, float y, float dist) const            { return GetExactDist2dSq(x, y) < dist * dist; }
    bool IsInDist2d(const Position* pos, float dist) const         { return GetExactDist2dSq(pos) < dist * dist; }
    bool IsInDist(float x, float y, float z, float dist) const     { return GetExactDistSq(x, y, z) < dist * dist; }
    bool IsInDist(const Position* pos, float dist) const           { return GetExactDistSq(pos) < dist * dist; }

    // Angles.
    float GetAngle(const Position* pos) const;
    float GetAngle(float x, float y) const;
    float GetRelativeAngle(const Position* pos) const              { return GetAngle(pos) - m_orientation; }
    float GetRelativeAngle(float x, float y) const                 { return GetAngle(x, y) - m_orientation; }
    void GetSinCos(float x, float y, float &vsin, float &vcos) const;

    bool HasInArc(float arcangle, const Position* pos) const;
    bool HasInLine(WorldObject const* target, float width) const;

    std::string ToString() const;

    // Orientation normalization (radian range of 0-2PI).
    static float NormalizeOrientation(float o)
    {
        if (o >= 0.0f && o < 6.2831864f)
            return o;

        // fmod only supports positive numbers. Thus we have
        // to emulate negative numbers
        if (o < 0)
        {
            float mod = -o;
            mod = fmod(mod, 2.0f * static_cast<float>(M_PI));
            mod = -mod + 2.0f * static_cast<float>(M_PI);
            return mod;
        }
        return fmod(o, 2.0f * static_cast<float>(M_PI));
    }

    // Pitch normalization (radian range of 0-2PI).
    static float NormalizePitch(float o)
    {
        if (o > -M_PI && o < M_PI)
            return o;

        o = NormalizeOrientation(o + M_PI) - M_PI;

        return o;
    }

    // Operators.
    bool operator==(Position const &a);
    inline bool operator!=(Position const &a) { return !(operator==(a)); }
};

ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer);
ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer);
ByteBuffer& operator>>(ByteBuffer& buf, Position::PositionXYZStreamer const& streamer);
ByteBuffer& operator<<(ByteBuffer& buf, Position::PositionXYZOStreamer const& streamer);

/*** Movement holder. ***/

struct MovementInfo
{
    // Common.
    uint64 guid;
    uint32 flags;
    uint16 flags2;
    Position pos;
    uint32 time;

    // Transports.
    uint64 t_guid;
    Position t_pos;
    int8 t_seat;
    uint32 t_time;
    uint32 t_time2;
    uint32 t_time3;

    // Swimming / Flying.
    union
    {
        float pitch;
        uint32 HavePitch;
    };

    // Falling.
    uint32 fallTime;

    // Jumping.
    float j_zspeed, j_cosAngle, j_sinAngle, j_xyspeed;

    // Spline.
    union
    {
        float splineElevation;
        uint32 HaveSplineElevation;
    };

    uint32 Alive32;

    bool hasFallData : 1;
    bool hasFallDirection : 1;
    bool has_t_time2 : 1;
    bool has_t_time3 : 1;

    MovementInfo()
    {
        pos.Relocate(0, 0, 0, 0);
        guid = 0;
        flags = 0;
        flags2 = 0;
        time = t_time = t_time2 = t_time3 = fallTime = 0;
        splineElevation = 0.0f;
        pitch = j_zspeed = j_sinAngle = j_cosAngle = j_xyspeed = 0.0f;
        t_guid = 0;
        t_pos.Relocate(0, 0, 0, 0);
        t_seat = -1;
        hasFallData = hasFallDirection = has_t_time2 = has_t_time3 = false;
        Alive32 = 0;
    }

    uint32 GetMovementFlags() const { return flags; }
    void SetMovementFlags(uint32 flag) { flags = flag; }
    void AddMovementFlag(uint32 flag) { flags |= flag; }
    void RemoveMovementFlag(uint32 flag) { flags &= ~flag; }
    bool HasMovementFlag(uint32 flag) const { return flags & flag; }

    uint16 GetExtraMovementFlags() const { return flags2; }
    void AddExtraMovementFlag(uint16 flag) { flags2 |= flag; }
    bool HasExtraMovementFlag(uint16 flag) const { return flags2 & flag; }

    void SetFallTime(uint32 time) { fallTime = time; }
    void StopMoving() { flags &= ~MOVEMENTFLAG_MASK_MOVING; splineElevation = 0.0f; }
    void Normalize();

    void OutDebug();
};

#endif
