/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "DBCEnums.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "Arena.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "Language.h"
#include "Player.h"
#include "SpellMgr.h"
#include "DisableMgr.h"
#include "ScriptMgr.h"
#include "MapManager.h"
#include "Battleground.h"
#include "BattlegroundAB.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Group.h"
#include "BattlePetMgr.h"

BattlePetMgr::BattlePetMgr(Player* owner) : m_player(owner)
{
}

void BattlePetMgr::GetBattlePetList(PetBattleDataList &battlePetList) const
{
    auto spellMap = m_player->GetSpellMap();
    for (auto itr : spellMap)
    {
        if (!itr.second)
            continue;

        if (itr.second->state == PLAYERSPELL_REMOVED)
            continue;

        if (!itr.second->active || itr.second->disabled)
            continue;

        SpellInfo const* spell = sSpellMgr->GetSpellInfo(itr.first);
        if (!spell)
            continue;

        // Is summon pet spell
        if ((spell->Effects[0].Effect == SPELL_EFFECT_SUMMON && spell->Effects[0].MiscValueB == 3221) == 0)
            continue;

        const CreatureTemplate* creature = sObjectMgr->GetCreatureTemplate(spell->Effects[0].MiscValue);
        if (!creature)
            continue;

        const BattlePetSpeciesEntry* species = sBattlePetSpeciesStore.LookupEntry(creature->Entry);
        if (!species)
            continue;

        PetBattleData pet(creature->Entry, creature->Modelid1, species->ID, spell->Id);
        battlePetList.push_back(pet);
    }
}

void BattlePetMgr::BuildBattlePetJournal(WorldPacket *data)
{
    PetBattleDataList petList;
    GetBattlePetList(petList);

    ByteBuffer dataBuffer;

    data->Initialize(SMSG_BATTLEPET_JOURNAL);

    data->WriteBits(0, 25);                     // Battle Team
    data->WriteBits(petList.size(), 19);

    // bits part
    for (auto pet : petList)
    {
        // Not sent for the moment, pig pig
        ObjectGuid petGuid = uint64(pet.m_summonSpellID);

        data->WriteBits(0, 7);                  // name length
        data->WriteBit(true);                   // hasQuality, inverse
        data->WriteBit(petGuid[3]);
        data->WriteBit(true);                   // hasBreed, inverse
        data->WriteBit(petGuid[5]);
        data->WriteBit(petGuid[0]);
        data->WriteBit(true);                   // hasUnk, inverse
        data->WriteBit(false);                  // hasFirstOwnerGuid
        data->WriteBit(petGuid[2]);
        // if hasfirstownerguid bits 7 1 6 2 4 3 0 5
        data->WriteBit(false);                  // unk bit
        data->WriteBit(petGuid[7]);
        data->WriteBit(petGuid[4]);
        data->WriteBit(petGuid[6]);
        data->WriteBit(petGuid[1]);

        if (false)
            dataBuffer << uint16(0); // hasUnk

        dataBuffer << uint32(1);                // Health or MaxHealth
        dataBuffer << uint32(1);                // Speed

        if (false)
            dataBuffer << uint16(0); // hasBreed
            
        if (false)
            dataBuffer << uint16(0); // hasQuality

        dataBuffer.WriteByteSeq(petGuid[5]);
        dataBuffer << uint16(0);                // Expirience
        dataBuffer.WriteByteSeq(petGuid[2]);
        dataBuffer.WriteByteSeq(petGuid[1]);
        dataBuffer.WriteByteSeq(petGuid[6]);
        dataBuffer << uint32(1);                // MaxHealth or Health
        dataBuffer << uint32(pet.m_displayID);  // Model
        dataBuffer << uint32(pet.m_speciesID);  // Species
        dataBuffer << uint32(pet.m_entry);      // PetEntry
        dataBuffer << uint32(0);                // Power
        dataBuffer.WriteByteSeq(petGuid[3]);
        dataBuffer << uint16(1);                // Level
        dataBuffer.WriteByteSeq(petGuid[0]);
        dataBuffer.WriteByteSeq(petGuid[4]);
        dataBuffer.WriteByteSeq(petGuid[7]);
    }

    data->WriteBit(1);                          // Unk
    data->FlushBits();

    if (dataBuffer.size())
        data->append(dataBuffer);

    *data << uint16(1);                         // Unk
}

void WorldSession::HandleSummonBattlePet(WorldPacket& recvData)
{
    ObjectGuid battlePetGuid;

    uint8 bitsOrder[8] = { 0, 3, 6, 4, 5, 7, 2, 1 };
    recvData.ReadBitInOrder(battlePetGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 3, 1, 5, 4, 2, 6, 0, 7 };
    recvData.ReadBytesSeq(battlePetGuid, bytesOrder);

    if (!_player->HasSpell(uint32(battlePetGuid)))
        return;

    _player->CastSpell(_player, uint32(battlePetGuid), true);
}
