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

#include "Opcodes.h"
#include "WorldSession.h"
#include "WorldPacket.h"

void WorldSession::SendAuthResponse(uint8 code, bool queued, uint32 queuePos)
{
    const static uint8 ClassExpensions[MAX_CLASSES] = 
    {
        0, // CLASS_NONE
        0, // CLASS_WARRIOR
        0, // CLASS_PALADIN
        0, // CLASS_HUNTER
        0, // CLASS_ROGUE
        0, // CLASS_PRIEST
        0, // CLASS_DEATH_KNIGHT
        0, // CLASS_SHAMAN
        0, // CLASS_MAGE
        0, // CLASS_WARLOCK
        4, // CLASS_MONK
        0  // CLASS_DRUID
    };

    WorldPacket packet(SMSG_AUTH_RESPONSE, 80);

    bool hasAccountData = code == AUTH_OK;
    const uint32 realmRaceCount = 15;

    packet.WriteBit(hasAccountData);

    if (hasAccountData)
    {
        packet.WriteBits(0, 21);
        packet.WriteBit(0);
        packet.WriteBits(realmRaceCount, 23);
        packet.WriteBits(0, 21);
        packet.WriteBit(0);
        packet.WriteBits(MAX_CLASSES - 1, 23);
        packet.WriteBit(0);
        packet.WriteBit(0);
        packet.WriteBit(0);
    }

    packet.WriteBit(queued);
    if (queued)
        packet.WriteBit(false);

    packet.FlushBits();
    
    if (queued)
        packet << uint32(queuePos);                            // Unknown

    if (hasAccountData)
    {
        for (uint32 i = 1; i < MAX_CLASSES; ++i)
        {
            packet << uint8(ClassExpensions[i]); // expension
            packet << uint8(i);                  // class
        }

        packet << uint8(Expansion());

        packet << uint8(0);
        packet << uint8(RACE_HUMAN);
        packet << uint8(0);
        packet << uint8(RACE_ORC);
        packet << uint8(0);
        packet << uint8(RACE_DWARF);
        packet << uint8(0);
        packet << uint8(RACE_NIGHTELF);
        packet << uint8(0);
        packet << uint8(RACE_UNDEAD_PLAYER);
        packet << uint8(0);
        packet << uint8(RACE_TAUREN);
        packet << uint8(0);
        packet << uint8(RACE_GNOME);
        packet << uint8(0);
        packet << uint8(RACE_TROLL);
        packet << uint8(1);
        packet << uint8(RACE_GOBLIN);
        packet << uint8(1);
        packet << uint8(RACE_BLOODELF);
        packet << uint8(1);
        packet << uint8(RACE_DRAENEI);
        packet << uint8(1);
        packet << uint8(RACE_WORGEN);
        packet << uint8(1);
        packet << uint8(RACE_PANDAREN_NEUTRAL);
        packet << uint8(1);
        packet << uint8(RACE_PANDAREN_ALLI);
        packet << uint8(1);
        packet << uint8(RACE_PANDAREN_HORDE);

        packet << uint32(Expansion());
        
        packet << uint32(0);
        packet << uint32(0);
        packet << uint32(0);
        packet << uint32(0);

        packet << uint8(Expansion());
        packet << uint32(0);
    }

    packet << uint8(code);

    SendPacket(&packet);
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CLIENT_CACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
}
