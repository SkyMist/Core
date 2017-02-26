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

#ifndef TRINITYSERVER_PACKET_BUILDER_H
#define TRINITYSERVER_PACKET_BUILDER_H

#include "Define.h" // for uint32
#include "G3D/Vector3.h"
using G3D::Vector3;

class ByteBuffer;
class WorldPacket;
class Unit;

namespace Movement
{
    enum MonsterMoveType
    {
        MonsterMoveNormal       = 0,
        MonsterMoveStop         = 1,
        MonsterMoveFacingPoint  = 2,
        MonsterMoveFacingTarget = 3,
        MonsterMoveFacingAngle  = 4
    };

    class MoveSpline;
    class PacketBuilder
    {
    public:
        static void WriteMonsterMove(const MoveSpline& mov, WorldPacket& data, Unit* unit);
        static void WriteStopMovement(Vector3 const& loc, uint32 splineId, WorldPacket& data, Unit* unit);

        static void WriteCreateBits(MoveSpline const& moveSpline, ByteBuffer& data);
        static void WriteCreateData(MoveSpline const& moveSpline, ByteBuffer& data);
        static void WriteCreateGuid(MoveSpline const& moveSpline, ByteBuffer& data);
    };
}
#endif // TRINITYSERVER_PACKET_BUILDER_H
