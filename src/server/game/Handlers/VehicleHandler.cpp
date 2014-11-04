/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Vehicle.h"
#include "Player.h"
#include "Log.h"
#include "ObjectAccessor.h"

void WorldSession::HandleDismissControlledVehicle(WorldPacket &recvData)
{
    uint64 vehicleGUID = _player->GetCharmGUID();
    if (!vehicleGUID) // something wrong here...
    {
        recvData.rfinish(); // prevent warnings spam
        return;
    }
    // Too lazy to parse all data, just read pos and forge pkt
    MovementInfo mi;
    _player->ReadMovementInfo(recvData, &mi);
    mi.guid = _player->GetGUID();

    uint32 mstime = getMSTime();
    if (m_clientTimeDelay == 0)
        m_clientTimeDelay = mstime - mi.time;

    mi.time = mi.time + m_clientTimeDelay + 0;

    _player->m_movementInfo = mi;

    WorldPacket data(SMSG_MOVE_UPDATE);
    _player->WriteMovementInfo(data);
    _player->SendMessageToSet(&data, _player);

    _player->ExitVehicle();

}

void WorldSession::HandleChangeSeatsOnControlledVehicle(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE");

    Unit* vehicle_base = GetPlayer()->GetVehicleBase();
    if (!vehicle_base)
    {
        recvData.rfinish();                                // prevent warnings spam
        return;
    }

    VehicleSeatEntry const* seat = GetPlayer()->GetVehicle()->GetSeatForPassenger(GetPlayer());
    if (!seat->CanSwitchFromSeat())
    {
        recvData.rfinish();                                // prevent warnings spam
        sLog->outError(LOG_FILTER_NETWORKIO, "HandleChangeSeatsOnControlledVehicle, Opcode: %u, Player %u tried to switch seats but current seatflags %u don't permit that.",
            recvData.GetOpcode(), GetPlayer()->GetGUIDLow(), seat->m_flags);
        return;
    }

    switch (recvData.GetOpcode())
    {/*
        case CMSG_REQUEST_VEHICLE_PREV_SEAT:
            GetPlayer()->ChangeSeat(-1, false);
            break;
        case CMSG_REQUEST_VEHICLE_NEXT_SEAT:
            GetPlayer()->ChangeSeat(-1, true);
            break;
            */
        case CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE:
        {
            float x, y, z;
            int8 seatId;
            ObjectGuid playerGUID;
            ObjectGuid accessoryGUID;
            ObjectGuid transportGUID;
            recvData >> x >> z >> seatId >> y;

            accessoryGUID[5] = recvData.ReadBit();
            uint32 unkcounter = recvData.ReadBits(22);
            auto bit176 = !recvData.ReadBit();
            playerGUID[5] = recvData.ReadBit();
            playerGUID[3] = recvData.ReadBit();
            auto bit40 = !recvData.ReadBit();
            accessoryGUID[2] = recvData.ReadBit();
            auto bit98 = !recvData.ReadBit();
            auto bit38 = !recvData.ReadBit();
            auto bit112 = recvData.ReadBit();
            accessoryGUID[4] = recvData.ReadBit();
            playerGUID[1] = recvData.ReadBit();
            auto bit148 = recvData.ReadBit();
            auto bit36 = !recvData.ReadBit();
            auto bit32 = !recvData.ReadBit();
            accessoryGUID[0] = recvData.ReadBit();
            accessoryGUID[1] = recvData.ReadBit();
            auto bit78 = !recvData.ReadBit();
            auto bit156 = recvData.ReadBit();
            playerGUID[2] = recvData.ReadBit();
            playerGUID[6] = recvData.ReadBit();
            accessoryGUID[3] = recvData.ReadBit();
            auto bit157 = recvData.ReadBit();
            playerGUID[4] = recvData.ReadBit();
            playerGUID[0] = recvData.ReadBit();
            accessoryGUID[6] = recvData.ReadBit();
            accessoryGUID[7] = recvData.ReadBit();
            playerGUID[7] = recvData.ReadBit();
            auto bit180 = recvData.ReadBit();

            auto bit100 = false;
            auto bit108 = false;
            if (bit112)
            {
                transportGUID[3] = recvData.ReadBit();
                transportGUID[6] = recvData.ReadBit();
                bit100 = recvData.ReadBit();
                transportGUID[1] = recvData.ReadBit();
                transportGUID[4] = recvData.ReadBit();
                bit108 = recvData.ReadBit();
                transportGUID[7] = recvData.ReadBit();
                transportGUID[2] = recvData.ReadBit();
                transportGUID[0] = recvData.ReadBit();
                transportGUID[5] = recvData.ReadBit();
            }

            if (bit32)
            {
                recvData.ReadBits(30);
            }

            auto bit144 = false;
            if (bit148)
                bit144 = recvData.ReadBit();

            if (bit36)
                recvData.ReadBits(13);

            recvData.FlushBits();
            recvData.ReadByteSeq(accessoryGUID[6]);
            recvData.ReadByteSeq(playerGUID[7]);
            recvData.ReadByteSeq(playerGUID[2]);
            recvData.ReadByteSeq(accessoryGUID[7]);

            recvData.read_skip(sizeof(uint32) * unkcounter);
            recvData.ReadByteSeq(playerGUID[1]);
            recvData.ReadByteSeq(accessoryGUID[1]);
            recvData.ReadByteSeq(playerGUID[5]);
            recvData.ReadByteSeq(accessoryGUID[2]);
            recvData.ReadByteSeq(playerGUID[4]);
            recvData.ReadByteSeq(accessoryGUID[4]);
            recvData.ReadByteSeq(accessoryGUID[0]);
            recvData.ReadByteSeq(playerGUID[3]);
            recvData.ReadByteSeq(accessoryGUID[5]);
            recvData.ReadByteSeq(playerGUID[6]);
            recvData.ReadByteSeq(accessoryGUID[3]);
            recvData.ReadByteSeq(playerGUID[0]);

            if (bit112)
            {
                recvData.read_skip<float>();
                if (bit100)
                    recvData.read_skip<uint32>();
                recvData.ReadByteSeq(transportGUID[0]);
                recvData.read_skip<float>();
                recvData.read_skip<float>();
                if (bit108)
                    recvData.read_skip<uint32>();
                recvData.ReadByteSeq(transportGUID[4]);
                recvData.ReadByteSeq(transportGUID[5]);
                recvData.read_skip<float>();
                recvData.read_skip<uint8>();
                recvData.ReadByteSeq(transportGUID[3]);
                recvData.ReadByteSeq(transportGUID[1]);
                recvData.read_skip<uint32>();
                recvData.ReadByteSeq(transportGUID[6]);
                recvData.ReadByteSeq(transportGUID[7]);
                recvData.ReadByteSeq(transportGUID[2]);
            }

            if (bit98)
                recvData.read_skip<float>();

            if (bit148)
            {
                if (bit144)
                {
                    recvData.read_skip<float>();
                    recvData.read_skip<float>();
                    recvData.read_skip<float>();
                }
                recvData.read_skip<float>();
                recvData.read_skip<uint32>();
            }

            if (bit40)
                recvData.read_skip<uint32>();

            if (bit38)
                recvData.read_skip<float>();

            if (bit176)
                recvData.read_skip<uint32>();

            if (bit78)
                recvData.read_skip<float>();

            if (!accessoryGUID)
                GetPlayer()->ChangeSeat(-1, seatId > 0); // prev/next
            else if (Unit* vehUnit = Unit::GetUnit(*GetPlayer(), accessoryGUID))
            {
                if (Vehicle* vehicle = vehUnit->GetVehicleKit())
                    if (vehicle->HasEmptySeat(seatId))
                        vehUnit->HandleSpellClick(GetPlayer(), seatId);
            }
            break;
        }
        case CMSG_REQUEST_VEHICLE_SWITCH_SEAT:
        {
            ObjectGuid guid;

            int8 seatId;
            recvData >> seatId;

            uint8 bitOrder[8] = {2, 7, 4, 3, 0, 5, 1, 6};
            recvData.ReadBitInOrder(guid, bitOrder);

            uint8 byteOrder[8] = {5, 6, 4, 0, 1, 2, 7, 3};
            recvData.ReadBytesSeq(guid, byteOrder);

            if (vehicle_base->GetGUID() == guid)
                GetPlayer()->ChangeSeat(seatId);
            else if (Unit* vehUnit = Unit::GetUnit(*GetPlayer(), guid))
                if (Vehicle* vehicle = vehUnit->GetVehicleKit())
                    if (vehicle->HasEmptySeat(seatId))
                        vehUnit->HandleSpellClick(GetPlayer(), seatId);
            break;
        }
        default:
            break;
    }
}

void WorldSession::HandleEnterPlayerVehicle(WorldPacket& recvData)
{
    // Read guid
    ObjectGuid guid;
    uint8 bitOrder[8] = { 4, 0, 3, 2, 1, 7, 5, 6 };
    recvData.ReadBitInOrder(guid, bitOrder);
    uint8 byteOrder[8] = { 0, 4, 5, 2, 7, 6, 1, 3 };
    recvData.ReadBytesSeq(guid, byteOrder);
    if (Player* player = ObjectAccessor::FindPlayer(guid))
    {
        if (!player->GetVehicleKit() || _player->GetVehicleKit())
            return;
        if (!player->IsInRaidWith(_player))
            return;
        if (!player->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
            return;
        _player->EnterVehicle(player);
    }
}

void WorldSession::HandleEjectPassenger(WorldPacket& data)
{
    Vehicle* vehicle = _player->GetVehicleKit();
    if (!vehicle)
    {
        data.rfinish();                                     // prevent warnings spam
        sLog->outError(LOG_FILTER_NETWORKIO, "HandleEjectPassenger: Player %u is not in a vehicle!", GetPlayer()->GetGUIDLow());
        return;
    }

    ObjectGuid guid;
    uint8 bitOrder[8] = { 2, 1, 0, 5, 4, 6, 7, 3 };
    data.ReadBitInOrder(guid, bitOrder);
    uint8 byteOrder[8] = { 4, 3, 2, 1, 6, 0, 5, 7 };
    data.ReadBytesSeq(guid, byteOrder);

    if (IS_PLAYER_GUID(guid))
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player)
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u tried to eject player %u from vehicle, but the latter was not found in world!", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
            return;
        }

        if (!player->IsOnVehicle(vehicle->GetBase()))
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u tried to eject player %u, but they are not in the same vehicle", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
            return;
        }

        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(player);
        ASSERT(seat);
        if (seat->IsEjectable())
            player->ExitVehicle();
        else
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u attempted to eject player %u from non-ejectable seat.", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
    }

    else if (IS_CREATURE_GUID(guid))
    {
        Unit* unit = ObjectAccessor::GetUnit(*_player, guid);
        if (!unit) // creatures can be ejected too from player mounts
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u tried to eject creature guid %u from vehicle, but the latter was not found in world!", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
            return;
        }

        if (!unit->IsOnVehicle(vehicle->GetBase()))
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u tried to eject unit %u, but they are not in the same vehicle", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
            return;
        }

        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(unit);
        ASSERT(seat);
        if (seat->IsEjectable())
        {
            ASSERT(GetPlayer() == vehicle->GetBase());
            unit->ExitVehicle();
        }
        else
            sLog->outError(LOG_FILTER_NETWORKIO, "Player %u attempted to eject creature GUID %u from non-ejectable seat.", GetPlayer()->GetGUIDLow(), GUID_LOPART(guid));
    }
}

void WorldSession::HandleRequestVehicleExit(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_REQUEST_VEHICLE_EXIT");

    if (Vehicle* vehicle = GetPlayer()->GetVehicle())
    {
        if (VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(GetPlayer()))
        {
            if (seat->CanEnterOrExit())
                GetPlayer()->ExitVehicle();
            else
                sLog->outError(LOG_FILTER_NETWORKIO, "Player %u tried to exit vehicle, but seatflags %u (ID: %u) don't permit that.",
                GetPlayer()->GetGUIDLow(), seat->m_ID, seat->m_flags);
        }
    }
}
