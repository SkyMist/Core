/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MovementPacketBuilder.h"
#include "MoveSpline.h"
#include "WorldPacket.h"
#include "Object.h"
#include "Unit.h"
#include "Transport.h"

namespace Movement
{
    /*** Operators. ***/

    inline void operator << (WorldPacket& b, const Vector3& v)
    {
        b << v.x << v.y << v.z;
    }

    inline void operator >> (WorldPacket& b, Vector3& v)
    {
        b >> v.x >> v.y >> v.z;
    }

    inline void operator << (ByteBuffer& b, const Vector3& v)
    {
        b << v.x << v.y << v.z;
    }

    inline void operator >> (ByteBuffer& b, Vector3& v)
    {
        b >> v.x >> v.y >> v.z;
    }

    /*** Path types. ***/

    void WriteLinearPath(const Spline<int32>& spline, WorldPacket& data)
    {
        uint32 last_idx = spline.getPointCount() - 3;
        const Vector3 * real_path = &spline.getPoint(1);

        if (last_idx > 0)
        {
            Vector3 middle = (real_path[0] + real_path[last_idx]) / 2.f;
            Vector3 offset;
            // first and last points already appended
            for (uint32 i = 0; i < last_idx; ++i)
            {
                offset = middle - real_path[i];
                data.appendPackXYZ(offset.x, offset.y, offset.z);
            }
        }
    }

    void WriteCatmullRomPath(const Spline<int32>& spline, WorldPacket& data)
    {
        uint32 count = spline.getPointCount() - 2;

        for (uint32 i = 0; i < count; i++)
            data << spline.getPoint(i + 2).x << spline.getPoint(i + 2).y << spline.getPoint(i + 2).z;

        //data.append<Vector3>(&spline.getPoint(2), count);
    }

    void WriteCatmullRomCyclicPath(const Spline<int32>& spline, WorldPacket& data)
    {
        uint32 count = spline.getPointCount() - 2;

        data << spline.getPoint(1).x << spline.getPoint(1).y << spline.getPoint(1).z; // fake point, client will erase it from the spline after first cycle done
        for (uint32 i = 0; i < count; i++)
            data << spline.getPoint(i + 1).x << spline.getPoint(i + 1).y << spline.getPoint(i + 1).z;

        //data.append<Vector3>(&spline.getPoint(1), count);
    }

    /*** Packets. ***/

    // Creature dynamic movement updates.

    void PacketBuilder::WriteMonsterMove(const MoveSpline& move_spline, WorldPacket& data, Unit* unit)
    {
        ObjectGuid moverGUID = unit->GetGUID();
        ObjectGuid transportGUID = unit->GetTransGUID();

        const Spline<int32>& spline = move_spline.spline;
        MoveSplineFlag splineflags = move_spline.splineflags;
        splineflags.enter_cycle = move_spline.isCyclic();

        bool hasTransport = (unit->GetTransGUID() != 0) ? true : false;

        bool hasUnk1 = false;
        bool hasUnk2 = false;
        bool hasUnk3 = false;
        bool unk4    = false;

        uint32 unkCounter = 0;
        uint32 packedWPcount = splineflags & MoveSplineFlag::UncompressedPath ? 0 : move_spline.spline.getPointCount() - 3;
        uint32 WPcount = move_spline.spline.getPointCount() - 2;
        uint32 sendSplineFlags = splineflags & ~MoveSplineFlag::Mask_No_Monster_Move;
        int8 seat = unit->GetTransSeat();

        if (splineflags.cyclic)
            WPcount += 1;

        uint8 splineType = 0;
        switch (splineflags & MoveSplineFlag::Mask_Final_Facing)
        {
            case MoveSplineFlag::Final_Target:
                splineType = MonsterMoveFacingTarget;
                break;
            case MoveSplineFlag::Final_Angle:
                splineType = MonsterMoveFacingAngle;
                break;
            case MoveSplineFlag::Final_Point:
                splineType = MonsterMoveFacingPoint;
                break;
            default:
                splineType = MonsterMoveNormal;
                break;
        }

        // ================ Calculate unit transport offsets for the target point. ==================== //
        float splinePointTargetX = move_spline.spline.getPoint(move_spline.spline.first()).x;
        float splinePointTargetY = move_spline.spline.getPoint(move_spline.spline.first()).y;
        float splinePointTargetZ = move_spline.spline.getPoint(move_spline.spline.first()).z;
        float transportPointTargetX = 0.0f;
        float transportPointTargetY = 0.0f;
        float transportPointTargetZ = 0.0f;

        if (hasTransport && unit->GetTransport() && unit->GetTransport()->IsInWorld())
        {
            splinePointTargetZ -= unit->GetTransport()->GetPositionZ();
            splinePointTargetY -= unit->GetTransport()->GetPositionY();
            splinePointTargetX -= unit->GetTransport()->GetPositionX();

            float inx = splinePointTargetX, iny = splinePointTargetY, ino = unit->GetTransport()->GetOrientation();

            transportPointTargetZ = splinePointTargetZ;
            transportPointTargetY = (iny - inx * tan(ino)) / (cos(ino) + std::sin(ino) * tan(ino));
            transportPointTargetX = (inx + iny * tan(ino)) / (cos(ino) + std::sin(ino) * tan(ino));
        }
        // ================ Done, now we have all we need. ==================== //

        data << float(transportPointTargetY);
        data << uint32(move_spline.GetId());
        data << float(transportPointTargetZ);
        data << float(transportPointTargetX);

        data << float(move_spline.spline.getPoint(move_spline.spline.first()).x);
        data << float(move_spline.spline.getPoint(move_spline.spline.first()).y);
        data << float(move_spline.spline.getPoint(move_spline.spline.first()).z);

        data.WriteBit(moverGUID[3]);
        data.WriteBit(sendSplineFlags == 0);        // !hasFlags
        data.WriteBit(moverGUID[6]);
        data.WriteBit(!splineflags.animation);      // animation state
        data.WriteBit(!hasUnk2);                    // !hasUnk2, unk byte
        data.WriteBits(splineType, 3);              // splineType
        data.WriteBit(seat == -1);                  // !has seat
        data.WriteBit(moverGUID[2]);
        data.WriteBit(moverGUID[7]);
        data.WriteBit(moverGUID[5]);

        if (splineType == MonsterMoveFacingTarget)
        {
            ObjectGuid facingTargetGUID = move_spline.facing.target;
            uint8 facingTargetBitsOrder[8] = { 6, 7, 0, 5, 2, 3, 4, 1 };
            data.WriteBitInOrder(facingTargetGUID, facingTargetBitsOrder);
        }

        data.WriteBit(!splineflags.parabolic);      // !hasParabolicTime
        data.WriteBit(moverGUID[4]);
        data.WriteBits(packedWPcount, 22);          // packed waypoint count
        data.WriteBit(true);                        // !unk, send uint32
        data.WriteBit(false);                       // fake bit
        data.WriteBit(moverGUID[0]);

        uint8 transportBitsOrder[8] = { 3, 6, 5, 0, 1, 2, 4, 7 };
        data.WriteBitInOrder(transportGUID, transportBitsOrder);

        data.WriteBit(!hasUnk3);                    // !hasUnk3
        data.WriteBit(!splineflags.parabolic);      // !hasParabolicSpeed
        data.WriteBit(!splineflags.animation);      // !hasAnimationTime
        data.WriteBits(WPcount, 20);
        data.WriteBit(moverGUID[1]);
        data.WriteBit(hasUnk1);                     // unk, has counter + 2 bits & somes uint16/float

        if (hasUnk1)
        {
            data.WriteBits(unkCounter, 22);
            data.WriteBits(0, 2);
        }

        data.WriteBit(unk4);                        // unk bit 38
        data.WriteBit(!move_spline.Duration());     // !has duration

        data.FlushBits();

        if (splineType == MonsterMoveFacingTarget)
        {
            ObjectGuid facingTargetGUID = move_spline.facing.target;
            uint8 facingTargetBitsOrder[8] = { 5, 3, 6, 1, 4, 2, 0, 7 };
            data.WriteBytesSeq(facingTargetGUID, facingTargetBitsOrder);
        }

        data.WriteByteSeq(moverGUID[3]);
        data.WriteByteSeq(transportGUID[7]);
        data.WriteByteSeq(transportGUID[3]);
        data.WriteByteSeq(transportGUID[2]);
        data.WriteByteSeq(transportGUID[0]);
        data.WriteByteSeq(transportGUID[6]);
        data.WriteByteSeq(transportGUID[4]);
        data.WriteByteSeq(transportGUID[5]);
        data.WriteByteSeq(transportGUID[1]);

        // Write bytes
        if (hasUnk1)
        {
            data << float(0.0f);

            for (uint32 i = 0; i < unkCounter; i++)
            {
                data << uint16(0);
                data << uint16(0);
            }
            data << float(0.0f);
            data << uint16(0);
            data << uint16(0);
        }

        if (hasUnk2)
            data << uint8(0);                   // unk byte

        if (splineType == MonsterMoveFacingAngle)
            data << move_spline.facing.angle;

        if (sendSplineFlags)
            data << uint32(sendSplineFlags);

        data.WriteByteSeq(moverGUID[7]);
        if (seat != -1)
            data << int8(seat);

        if (false)
            data << uint32(0);                  // unk uint32

        if (splineflags.animation)
            data << uint8(splineflags.getAnimationId());

        if (packedWPcount)
        {
            WriteLinearPath(move_spline.spline, data);
        }

        data.WriteByteSeq(moverGUID[5]);
        data.WriteByteSeq(moverGUID[1]);
        data.WriteByteSeq(moverGUID[2]);

        if (splineflags.animation)
            data << int32(move_spline.effect_start_time);

        if (WPcount > 0)
        {
            if (splineflags.cyclic)
                WriteCatmullRomCyclicPath(move_spline.spline, data);
            else
                WriteCatmullRomPath(move_spline.spline, data);
        }

        data.WriteByteSeq(moverGUID[6]);

        if (move_spline.Duration())
            data << move_spline.Duration();

        if (splineType == MonsterMoveFacingPoint)
            data << move_spline.facing.f.x << move_spline.facing.f.y << move_spline.facing.f.z;

        if (splineflags.parabolic)
            data << move_spline.vertical_acceleration;

        if (hasUnk3)
            data << uint8(0);                   // unk byte

        data.WriteByteSeq(moverGUID[0]);

        if (splineflags.parabolic)
            data << move_spline.effect_start_time;

        data.WriteByteSeq(moverGUID[4]);
    }

    void PacketBuilder::WriteStopMovement(Vector3 const& pos, uint32 splineId, WorldPacket& data, Unit* unit)
    {
        ObjectGuid guid = unit->GetGUID();
        ObjectGuid transport = unit->GetTransGUID();

        bool hasTransport = (unit->GetTransGUID() != 0) ? true : false;

        data << float(hasTransport? unit->GetTransOffsetY() : 0.f); // Most likely transport Y
        data << uint32(splineId);
        data << float(hasTransport? unit->GetTransOffsetZ() : 0.f); // Most likely transport Z
        data << float(hasTransport? unit->GetTransOffsetX() : 0.f); // Most likely transport X
        data << float(pos.x);
        data << float(pos.y);
        data << float(pos.z);

        data.WriteBit(guid[3]);
        data.WriteBit(1);
        data.WriteBit(guid[6]);

        data.WriteBit(1);
        data.WriteBit(1);

        data.WriteBits(MonsterMoveStop, 3);
        data.WriteBit(1);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[7]);
        data.WriteBit(guid[5]);
        data.WriteBit(1);
        data.WriteBit(guid[4]);

        data.WriteBits(0, 22); // WP count
        data.WriteBit(1);
        data.WriteBit(0);

        data.WriteBit(guid[0]);
        
        uint8 transportBitsOrder[8] = {3, 6, 5, 0, 1, 2, 4, 7};
        data.WriteBitInOrder(transport, transportBitsOrder);

        data.WriteBit(1);
        data.WriteBit(1); // Parabolic speed // esi+4Ch
        data.WriteBit(1);

        data.WriteBits(0, 20);

        data.WriteBit(guid[1]);
        data.WriteBit(0);
        data.WriteBit(0);
        data.WriteBit(1);

        data.FlushBits();

        data.WriteByteSeq(guid[3]);

        uint8 transportBytesOrder[8] = {7, 3, 2, 0, 6, 4, 5, 1};
        data.WriteBytesSeq(transport, transportBytesOrder);

        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[1]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[4]);
    }

    // Object create / movement visibility updates.

    void PacketBuilder::WriteCreateBits(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        bool isSplineEnabled = moveSpline.Initialized() && !moveSpline.Finalized();
        MoveSplineFlag flags = moveSpline.splineflags;

        data.WriteBit(isSplineEnabled);

        if (isSplineEnabled)
        {
            data.WriteBit(flags.parabolic || flags.animation);
            data.WriteBits(uint8(moveSpline.spline.mode()), 2);
            data.WriteBits(moveSpline.getPath().size(), 20);
            data.WriteBits(flags.raw(), 25);
            data.WriteBit(flags.parabolic);
            data.WriteBit(false); // bit134 UNK

            // if (bit134 UNK)
            // {
            //     data.WriteBits(word300 UNK, 2);
            //     data.WriteBits(bits138 UNK, 21);
            // }
        }
    }

    void PacketBuilder::WriteCreateData(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        bool isSplineEnabled = moveSpline.Initialized() && !moveSpline.Finalized();

        if (isSplineEnabled)
        {
            MoveSplineFlag splineFlags = moveSpline.splineflags;

            uint8 splineType = 0;
            switch (splineFlags & MoveSplineFlag::Mask_Final_Facing)
            {
                case MoveSplineFlag::Final_Target:
                    splineType = MonsterMoveFacingTarget;
                    break;
                case MoveSplineFlag::Final_Angle:
                    splineType = MonsterMoveFacingAngle;
                    break;
                case MoveSplineFlag::Final_Point:
                    splineType = MonsterMoveFacingPoint;
                    break;
                default:
                    splineType = MonsterMoveNormal;
                    break;
            }

            data << float(1.0f);                             // splineInfo.duration_mod_next; added in 3.1
            uint32 nodes = moveSpline.getPath().size();
            for (uint32 i = 0; i < nodes; ++i)
            {
                data << float(moveSpline.getPath()[i].z);
                data << float(moveSpline.getPath()[i].y);
                data << float(moveSpline.getPath()[i].x);
            }

            data << uint8(splineType);
            data << float(1.0f);                             // splineInfo.duration_mod; added in 3.1

            if (splineFlags.final_point)
                data << moveSpline.facing.f.x << moveSpline.facing.f.y << moveSpline.facing.f.z;

            if (splineFlags.parabolic)
                data << moveSpline.vertical_acceleration;   // added in 3.1

            if (splineFlags.final_angle)
                data << moveSpline.facing.angle;

            data << moveSpline.Duration();

            if (splineFlags.parabolic || splineFlags.animation)
                data << moveSpline.effect_start_time;       // added in 3.1

            data << moveSpline.timePassed();
        }

        data << moveSpline.GetId();

        if (!moveSpline.isCyclic())
        {
            Vector3 dest = moveSpline.FinalDestination();
            data << float(dest.x);
            data << float(dest.y);
            data << float(dest.z);
        }
        else
        {
            data << float(0.0f);
            data << float(0.0f);
            data << float(0.0f);
        }
    }

    void PacketBuilder::WriteCreateGuid(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if ((moveSpline.splineflags & MoveSplineFlag::Mask_Final_Facing) == MoveSplineFlag::Final_Target)
        {
            ObjectGuid facingGuid = moveSpline.facing.target;

            uint8 bitOrder[8] = { 6, 7, 3, 0, 5, 1, 4, 2 };
            data.WriteBitInOrder(facingGuid, bitOrder);

            uint8 byteOrder[8] = { 4, 2, 5, 6, 0, 7, 1, 3 };
            data.WriteBytesSeq(facingGuid, byteOrder);
        }
    }
}
