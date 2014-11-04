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

namespace Movement
{
    inline void operator << (ByteBuffer& b, const Vector3& v)
    {
        b << v.x << v.y << v.z;
    }

    inline void operator >> (ByteBuffer& b, Vector3& v)
    {
        b >> v.x >> v.y >> v.z;
    }

    enum MonsterMoveType
    {
        MonsterMoveNormal       = 0,
        MonsterMoveStop         = 1,
        MonsterMoveFacingSpot   = 2,
        MonsterMoveFacingTarget = 3,
        MonsterMoveFacingAngle  = 4
    };

    void PacketBuilder::WriteCommonMonsterMovePart(const MoveSpline& move_spline, WorldPacket& data)
    {
        MoveSplineFlag splineflags = move_spline.splineflags;

        data << uint8(0);                                       // sets/unsets MOVEMENTFLAG2_UNK7 (0x40)
        data << move_spline.spline.getPoint(move_spline.spline.first());
        data << move_spline.GetId();

        switch (splineflags & MoveSplineFlag::Mask_Final_Facing)
        {
            case MoveSplineFlag::Final_Target:
                data << uint8(MonsterMoveFacingTarget);
                data << move_spline.facing.target;
                break;
            case MoveSplineFlag::Final_Angle:
                data << uint8(MonsterMoveFacingAngle);
                data << move_spline.facing.angle;
                break;
            case MoveSplineFlag::Final_Point:
                data << uint8(MonsterMoveFacingSpot);
                data << move_spline.facing.f.x << move_spline.facing.f.y << move_spline.facing.f.z;
                break;
            default:
                data << uint8(MonsterMoveNormal);
                break;
        }

        // add fake Enter_Cycle flag - needed for client-side cyclic movement (client will erase first spline vertex after first cycle done)
        splineflags.enter_cycle = move_spline.isCyclic();
        data << uint32(splineflags & ~MoveSplineFlag::Mask_No_Monster_Move);

        if (splineflags.animation)
        {
            data << splineflags.getAnimationId();
            data << move_spline.effect_start_time;
        }

        data << move_spline.Duration();

        if (splineflags.parabolic)
        {
            data << move_spline.vertical_acceleration;
            data << move_spline.effect_start_time;
        }
    }

    void PacketBuilder::WriteStopMovement(Vector3 const& pos, uint32 splineId, ByteBuffer& data, Unit* unit)
    {
        ObjectGuid guid = unit->GetGUID();
        ObjectGuid transport = unit->GetTransGUID();

        data << float(0.f); // Most likely transport Y
        data << uint32(splineId);
        data << float(0.f); // Most likely transport Z
        data << float(0.f); // Most likely transport X
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
        uint8 transportBytesOrder[8] = {7, 3, 2, 0, 6, 4, 5, 1};
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
        data.WriteBytesSeq(transport, transportBytesOrder);
        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[1]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[4]);
    }

    void PacketBuilder::WriteMonsterMove(const MoveSpline& move_spline, WorldPacket& data)
    {
        WriteCommonMonsterMovePart(move_spline, data);

        const Spline<int32>& spline = move_spline.spline;
        MoveSplineFlag splineflags = move_spline.splineflags;
        /*if (splineflags & MoveSplineFlag::UncompressedPath)
        {
            if (splineflags.cyclic)
                WriteCatmullRomCyclicPath(spline, data);
            else
                WriteCatmullRomPath(spline, data);
        }*/
        //else
            //WriteLinearPath(spline, data);
    }

    void PacketBuilder::WriteCreateBits(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        ASSERT(!moveSpline.Finalized());

        MoveSplineFlag flags = moveSpline.splineflags;

        data.WriteBit(true);
        data.WriteBit(flags.parabolic || flags.animation);
        data.WriteBits(uint8(moveSpline.spline.mode()), 2);
        data.WriteBits(moveSpline.getPath().size(), 20);
        data.WriteBits(flags.raw(), 25);
        data.WriteBit(flags.parabolic);
        data.WriteBit(false);
        if (false)
        {
            data.WriteBits(0, 2);
            data.WriteBits(0, 21);
        }
    }

    void PacketBuilder::WriteCreateData(MoveSpline const& moveSpline, ByteBuffer& data, Unit* unit)
    {
        if (/*!moveSpline.Finalized()*/true)
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
                    splineType = MonsterMoveFacingSpot;
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

        if (!moveSpline.isCyclic())
        {
            Vector3 dest = moveSpline.FinalDestination();
            data << moveSpline.GetId();
            data << float(dest.x);
            data << float(dest.y);
            data << float(dest.z);
        }
        else
        {
            data << moveSpline.GetId();
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
