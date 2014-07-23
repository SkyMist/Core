/*
 * Copyright (C) 2011-2014 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2014 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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
#include "Language.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Arena.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GossipDef.h"
#include "SocialMgr.h"

#define CHARTER_DISPLAY_ID 16161

// Charters ID in item_template
enum CharterItemIDs
{
    GUILD_CHARTER                                 = 5863
};

enum CharterCosts
{
    GUILD_CHARTER_COST                            = 1000
};

void SendPetitionTurnResult(WorldPacket* Packet, PetitionTurns Result)
{
    Packet->WriteBits(Result, 4);
    Packet->FlushBits();
}

void SendPetitionSignResult(WorldPacket* Packet, ObjectGuid PetitionGuid, ObjectGuid OwnerGuid, PetitionSigns Result)
{
    Packet->WriteBit(OwnerGuid[5]);
    Packet->WriteBit(OwnerGuid[4]);
    Packet->WriteBit(PetitionGuid[6]);
    Packet->WriteBit(OwnerGuid[2]);
    Packet->WriteBit(PetitionGuid[3]);
    Packet->WriteBit(OwnerGuid[1]);
    Packet->WriteBit(PetitionGuid[7]);
    Packet->WriteBit(PetitionGuid[2]);
    Packet->WriteBits(Result, 4);
    Packet->WriteBit(OwnerGuid[0]);
    Packet->WriteBit(OwnerGuid[7]);
    Packet->WriteBit(PetitionGuid[4]);
    Packet->WriteBit(PetitionGuid[0]);
    Packet->WriteBit(OwnerGuid[3]);
    Packet->WriteBit(PetitionGuid[1]);
    Packet->WriteBit(OwnerGuid[6]);
    Packet->WriteBit(PetitionGuid[5]);
    
    Packet->FlushBits();
    
    Packet->WriteByteSeq(OwnerGuid[0]);
    Packet->WriteByteSeq(PetitionGuid[5]);
    Packet->WriteByteSeq(OwnerGuid[7]);
    Packet->WriteByteSeq(PetitionGuid[2]);
    Packet->WriteByteSeq(OwnerGuid[1]);
    Packet->WriteByteSeq(OwnerGuid[3]);
    Packet->WriteByteSeq(PetitionGuid[4]);
    Packet->WriteByteSeq(OwnerGuid[6]);
    Packet->WriteByteSeq(PetitionGuid[6]);
    Packet->WriteByteSeq(PetitionGuid[3]);
    Packet->WriteByteSeq(PetitionGuid[0]);
    Packet->WriteByteSeq(OwnerGuid[2]);
    Packet->WriteByteSeq(OwnerGuid[4]);
    Packet->WriteByteSeq(OwnerGuid[5]);
    Packet->WriteByteSeq(PetitionGuid[7]);
    Packet->WriteByteSeq(PetitionGuid[1]);
}

void WorldSession::HandlePetitionBuyOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_BUY");

    ObjectGuid guidNPC;
    std::string name;
    std::size_t nameLen;

    guidNPC[6] = recvData.ReadBit();
    guidNPC[1] = recvData.ReadBit();
    guidNPC[4] = recvData.ReadBit();
    guidNPC[2] = recvData.ReadBit();
    guidNPC[5] = recvData.ReadBit();
    guidNPC[7] = recvData.ReadBit();
    guidNPC[2] = recvData.ReadBit();
    nameLen = recvData.ReadBits(7);
    guidNPC[0] = recvData.ReadBit();
    
    recvData.ReadByteSeq(guidNPC[0]);
    recvData.ReadByteSeq(guidNPC[2]);
    recvData.ReadByteSeq(guidNPC[4]);
    recvData.ReadByteSeq(guidNPC[6]);
    recvData.ReadByteSeq(guidNPC[7]);
    recvData.ReadByteSeq(guidNPC[5]);
    recvData.ReadByteSeq(guidNPC[3]);
    recvData.ReadByteSeq(guidNPC[1]);
    name = recvData.ReadString(nameLen);
    
    TC_LOG_DEBUG("network", "Petitioner with GUID %u tried sell petition: name %s", GUID_LOPART(guidNPC), name.c_str());

    // prevent cheating
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guidNPC, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        TC_LOG_DEBUG("network", "WORLD: HandlePetitionBuyOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(guidNPC));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    uint32 charterid = 0;
    uint32 cost = 0;
    uint32 type = 0;
    if (creature->IsTabardDesigner())
    {
        // if tabard designer, then trying to buy a guild charter.
        // do not let if already in guild.
        if (_player->GetGuildId())
            return;

        charterid = GUILD_CHARTER;
        cost = GUILD_CHARTER_COST;
        type = GUILD_CHARTER_TYPE;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (sGuildMgr->GetGuildByName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
            return;
        }

        if (sObjectMgr->IsReservedName(name) || !ObjectMgr::IsValidCharterName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_INVALID, name);
            return;
        }
    }

    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(charterid);
    if (!pProto)
    {
        _player->SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, charterid, 0);
        return;
    }

    if (!_player->HasEnoughMoney(uint64(cost)))
    {                                                       //player hasn't got enough money
        _player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, creature, charterid, 0);
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, charterid, pProto->BuyCount);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, NULL, NULL, charterid);
        return;
    }

    _player->ModifyMoney(-(int32)cost);
    Item* charter = _player->StoreNewItem(dest, charterid, true);
    if (!charter)
        return;

    charter->SetUInt32Value(ITEM_FIELD_ENCHANTMENT, charter->GetGUIDLow());

    // ITEM_FIELD_ENCHANTMENT is guild id, ITEM_FIELD_ENCHANTMENT+1 is current signatures count (showed on item)
    charter->SetState(ITEM_CHANGED, _player);
    _player->SendNewItem(charter, 1, true, false);

    // a petition is invalid, if both the owner and the type matches we checked above, so this must be datacorruption
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_BY_OWNER);
    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt8(1, type);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    std::ostringstream ssInvalidPetitionGUIDs;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            ssInvalidPetitionGUIDs << '\'' << fields[0].GetUInt32() << "', ";
        } while (result->NextRow());
    }

    // delete petitions with the same guid as this one
    ssInvalidPetitionGUIDs << '\'' << charter->GetGUIDLow() << '\'';

    TC_LOG_DEBUG("network", "Invalid petition GUIDs: %s", ssInvalidPetitionGUIDs.str().c_str());
    CharacterDatabase.EscapeString(name);
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM petition WHERE petitionguid IN (%s)",  ssInvalidPetitionGUIDs.str().c_str());
    trans->PAppend("DELETE FROM petition_sign WHERE petitionguid IN (%s)", ssInvalidPetitionGUIDs.str().c_str());

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION);
    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt32(1, charter->GetGUIDLow());
    stmt->setString(2, name);
    stmt->setUInt8(3, uint8(type));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandlePetitionShowSignOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_SHOW_SIGNATURES");

    uint8 signs = 0;
    ObjectGuid PetitionGuid;
    
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[3] = recvData.ReadBit();
    PetitionGuid[5] = recvData.ReadBit();
    PetitionGuid[6] = recvData.ReadBit();
    PetitionGuid[2] = recvData.ReadBit();
    PetitionGuid[1] = recvData.ReadBit();
    PetitionGuid[0] = recvData.ReadBit();
    PetitionGuid[7] = recvData.ReadBit();
    
    recvData.ReadByteSeq(PetitionGuid[2]);
    recvData.ReadByteSeq(PetitionGuid[0]);
    recvData.ReadByteSeq(PetitionGuid[3]);
    recvData.ReadByteSeq(PetitionGuid[7]);
    recvData.ReadByteSeq(PetitionGuid[4]);
    recvData.ReadByteSeq(PetitionGuid[1]);
    recvData.ReadByteSeq(PetitionGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[6]);

    // solve (possible) some strange compile problems with explicit use GUID_LOPART(petitionguid) at some GCC versions (wrong code optimization in compiler?)
    uint32 petitionGuidLow = GUID_LOPART(PetitionGuid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, petitionGuidLow);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_DEBUG("entities.player.items", "Petition %u is not found for player %u %s", GUID_LOPART(PetitionGuid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName().c_str());
        return;
    }
    Field* fields = result->Fetch();
    uint32 type = fields[0].GetUInt8();

    // if guild petition and has guild => error, return;
    if (type == GUILD_CHARTER_TYPE && _player->GetGuildId())
        return;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);

    stmt->setUInt32(0, petitionGuidLow);

    result = CharacterDatabase.Query(stmt);

    // result == NULL also correct in case no sign yet
    if (result)
        signs = uint8(result->GetRowCount());

    TC_LOG_DEBUG("network", "CMSG_PETITION_SHOW_SIGNATURES petition entry: '%u'", petitionGuidLow);

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, (9+9+4+1+signs*13));
    ObjectGuid OwnerGuid = GetPlayer()->GetGUID();
    ByteBuffer SignedBytes;
    
    data.WriteBit(OwnerGuid[4]);
    data.WriteBit(PetitionGuid[4]);
    data.WriteBit(OwnerGuid[5]);
    data.WriteBit(OwnerGuid[0]);
    data.WriteBit(OwnerGuid[6]);
    data.WriteBit(PetitionGuid[7]);
    data.WriteBit(OwnerGuid[7]);
    data.WriteBit(PetitionGuid[0]);
    data.WriteBit(OwnerGuid[2]);
    data.WriteBit(OwnerGuid[3]);
    
    data.WriteBits(signs, 21);
    
    for (uint8 i = 1; i <= signs; ++i)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid SignedGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);
        
        data.WriteBit(SignedGuid[6]);
        data.WriteBit(SignedGuid[2]);
        data.WriteBit(SignedGuid[4]);
        data.WriteBit(SignedGuid[5]);
        data.WriteBit(SignedGuid[3]);
        data.WriteBit(SignedGuid[0]);
        data.WriteBit(SignedGuid[7]);
        data.WriteBit(SignedGuid[1]);
        
        SignedBytes.WriteByteSeq(SignedGuid[4]);
        SignedBytes.WriteByteSeq(SignedGuid[7]);
        SignedBytes.WriteByteSeq(SignedGuid[5]);
        SignedBytes.WriteByteSeq(SignedGuid[3]);
        SignedBytes.WriteByteSeq(SignedGuid[2]);
        SignedBytes << uint32(0);
        SignedBytes.WriteByteSeq(SignedGuid[6]);
        SignedBytes.WriteByteSeq(SignedGuid[1]);
        SignedBytes.WriteByteSeq(SignedGuid[0]);
        
        result->NextRow();
    }
    
    data.WriteBit(PetitionGuid[5]);
    data.WriteBit(PetitionGuid[6]);
    data.WriteBit(PetitionGuid[1]);
    data.WriteBit(PetitionGuid[3]);
    data.WriteBit(OwnerGuid[1]);
    data.WriteBit(PetitionGuid[2]);
    
    data.FlushBits();
    
    data << uint32(petitionGuidLow);
    data.append(SignedBytes);
    data.WriteByteSeq(PetitionGuid[2]);
    data.WriteByteSeq(OwnerGuid[1]);
    data.WriteByteSeq(OwnerGuid[6]);
    data.WriteByteSeq(PetitionGuid[3]);
    data.WriteByteSeq(OwnerGuid[7]);
    data.WriteByteSeq(PetitionGuid[0]);
    data.WriteByteSeq(OwnerGuid[0]);
    data.WriteByteSeq(OwnerGuid[2]);
    data.WriteByteSeq(PetitionGuid[4]);
    data.WriteByteSeq(PetitionGuid[7]);
    data.WriteByteSeq(PetitionGuid[6]);
    data.WriteByteSeq(OwnerGuid[4]);
    data.WriteByteSeq(OwnerGuid[3]);
    data.WriteByteSeq(PetitionGuid[5]);
    data.WriteByteSeq(PetitionGuid[1]);
    SendPacket(&data);
}

void WorldSession::HandlePetitionQueryOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_QUERY");   // ok

    uint32 guildguid;
    ObjectGuid petitionguid;
    recvData >> guildguid;                  // in Trinity always same as GUID_LOPART(petitionguid)
    
    petitionguid[1] = recvData.ReadBit();
    petitionguid[2] = recvData.ReadBit();
    petitionguid[4] = recvData.ReadBit();
    petitionguid[7] = recvData.ReadBit();
    petitionguid[5] = recvData.ReadBit();
    petitionguid[0] = recvData.ReadBit();
    petitionguid[2] = recvData.ReadBit();
    petitionguid[6] = recvData.ReadBit();
    
    recvData.ReadByteSeq(petitionguid[6]);
    recvData.ReadByteSeq(petitionguid[5]);
    recvData.ReadByteSeq(petitionguid[1]);
    recvData.ReadByteSeq(petitionguid[7]);
    recvData.ReadByteSeq(petitionguid[4]);
    recvData.ReadByteSeq(petitionguid[3]);
    recvData.ReadByteSeq(petitionguid[2]);
    recvData.ReadByteSeq(petitionguid[0]);
    
    TC_LOG_DEBUG("network", "CMSG_PETITION_QUERY Petition GUID %u Guild GUID %u", GUID_LOPART(petitionguid), guildguid);

    SendPetitionQueryOpcode(petitionguid);
}

void WorldSession::SendPetitionQueryOpcode(ObjectGuid petitionguid)
{
    ObjectGuid OwnerGuid = 0;
    uint32 type;
    std::string name = "NO_NAME_FOR_GUID";
    std::string body = "NO_BODY_FOR_GUID";

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);

    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        OwnerGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        name      = fields[1].GetString();
        type      = fields[2].GetUInt8();
    }
    else
    {
        TC_LOG_DEBUG("network", "CMSG_PETITION_QUERY failed for petition (GUID: %u)", GUID_LOPART(petitionguid));
        return;
    }

    WorldPacket data(SMSG_PETITION_QUERY_RESPONSE, (4 + 8 + name.size() + 1 + 1 + 4*12 + 2 + 10));

    bool HasExtraData = (type == GUILD_CHARTER_TYPE) ? true : false; // check when is sent as false

    uint32 minLevelRequired = 12; // Minimum level required.
    uint32 maxLevelAllowed  = 90; // Maximum level.
    uint32 minSignaturesNeeded  = sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS); // normally 4 signatures except for yours.
    uint32 maxSignaturesAllowed = 9; // 9 signatures maximum on the chart as initial guild members.

    data.WriteBit(HasExtraData);

    if (HasExtraData)
    {
        data.WriteBits(name.size(), 7);
        data.WriteBit(OwnerGuid[6]);
        data.WriteBit(OwnerGuid[1]);

        for (std::size_t i = 0; i < 10; i++)
            data.WriteBits(0, 6);                           // unk strings size

        data.WriteBit(OwnerGuid[2]);
        data.WriteBit(OwnerGuid[5]);
        data.WriteBit(OwnerGuid[3]);
        data.WriteBit(OwnerGuid[0]);
        data.WriteBits(body.size(), 12);
        data.WriteBit(OwnerGuid[7]);
        data.WriteBit(OwnerGuid[4]);
    }

    data.FlushBits();

    if (HasExtraData)
    {
        data << uint32(0);                                  // dword10BC
        data << uint32(0);                                  // dword10B8
        data.WriteByteSeq(OwnerGuid[3]);
        data.WriteString(body);
        data << uint32(maxSignaturesAllowed);               // MaxSignatures
        data.WriteByteSeq(OwnerGuid[1]);
        data << uint32(0);                                  // dword10B0
        data << uint32(0);                                  // dword10D0
        data.WriteByteSeq(OwnerGuid[0]);

        // here comes the loop

        data.WriteByteSeq(OwnerGuid[1]);
        data << uint32(maxLevelAllowed);                    // MaxLevelRequired
        data.WriteByteSeq(OwnerGuid[7]);
        data << uint32(type);
        data << uint32(minLevelRequired);                   // MinLevelRequired
        data.WriteByteSeq(OwnerGuid[2]);
        data.WriteByteSeq(OwnerGuid[4]);
        data.WriteString(name);
        data.WriteByteSeq(OwnerGuid[6]);
        data << uint32(minSignaturesNeeded);                // MinSignatures
        data << uint16(0);                                  // word10C4
        data << uint32(0);                                  // dword18
        data << uint32(0);                                  // dword10D8
        data << uint32(0);                                  // dword10C0
        data << uint32(0);                                  // dword10B4
    }

    data << uint32(GUID_LOPART(petitionguid));              // guild/team guid (in Trinity always same as GUID_LOPART(petition guid)

    SendPacket(&data);
}

void WorldSession::HandlePetitionRenameOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_RENAME");

    uint64 petitionGuid;
    uint32 type;
    std::string newName;

    recvData >> petitionGuid;                              // guid
    recvData >> newName;                                   // new name

    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, GUID_LOPART(petitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        type = fields[0].GetUInt8();
    }
    else
    {
        TC_LOG_DEBUG("network", "CMSG_PETITION_RENAME failed for petition (GUID: %u)", GUID_LOPART(petitionGuid));
        return;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (sGuildMgr->GetGuildByName(newName))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, newName);
            return;
        }
        if (sObjectMgr->IsReservedName(newName) || !ObjectMgr::IsValidCharterName(newName))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_INVALID, newName);
            return;
        }
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PETITION_NAME);

    stmt->setString(0, newName);
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    CharacterDatabase.Execute(stmt);

    TC_LOG_DEBUG("network", "Petition (GUID: %u) renamed to '%s'", GUID_LOPART(petitionGuid), newName.c_str());
    /*WorldPacket data(MSG_PETITION_RENAME, (8+newName.size()+1));
    data << uint64(petitionGuid);
    data << newName;
    SendPacket(&data);*/
}

void WorldSession::HandlePetitionSignOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_SIGN");    // ok

    Field* fields;
    ObjectGuid petitionGuid;
    uint8 unk = recvData.read<uint8>();
    
    petitionGuid[5] = recvData.ReadBit();
    petitionGuid[2] = recvData.ReadBit();
    petitionGuid[0] = recvData.ReadBit();
    petitionGuid[7] = recvData.ReadBit();
    petitionGuid[1] = recvData.ReadBit();
    petitionGuid[4] = recvData.ReadBit();
    petitionGuid[6] = recvData.ReadBit();
    petitionGuid[3] = recvData.ReadBit();
    
    recvData.ReadByteSeq(petitionGuid[5]);
    recvData.ReadByteSeq(petitionGuid[0]);
    recvData.ReadByteSeq(petitionGuid[6]);
    recvData.ReadByteSeq(petitionGuid[4]);
    recvData.ReadByteSeq(petitionGuid[2]);
    recvData.ReadByteSeq(petitionGuid[7]);
    recvData.ReadByteSeq(petitionGuid[1]);
    recvData.ReadByteSeq(petitionGuid[3]);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURES);

    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_ERROR("network", "Petition %u is not found for player %u %s", GUID_LOPART(petitionGuid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName().c_str());
        return;
    }

    fields = result->Fetch();
    uint64 ownerGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
    uint64 signs = fields[1].GetUInt64();
    uint8 type = fields[2].GetUInt8();

    uint32 playerGuid = _player->GetGUIDLow();
    if (GUID_LOPART(ownerGuid) == playerGuid)
    {
        WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
        SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_CANT_SIGN_OWN);
        // close at signer side
        SendPacket(&data);
        return;
    }

    // not let enemies sign guild charter
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(ownerGuid))
    {
        if (type == GUILD_CHARTER_TYPE)
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (_player->GetGuildId())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_IN_GUILD_S, _player->GetName());

            WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
            SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_ALREADY_IN_GUILD);
            // close at signer side
            SendPacket(&data);
            return;
        }
        if (_player->GetGuildIdInvited())
        {
            // WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
            // SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_ALREADY_SIGNED); // This actually seems to be already invited in guild?
            // // close at signer side
            // SendPacket(&data);
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
            return;
        }
    }

    if (++signs > type)                                        // client signs maximum
    {
        WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
        SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_FULL);
        // close at signer side
        SendPacket(&data);
        return;
    }

    // Client doesn't allow to sign petition two times by one character, but not check sign by another character from same account
    // not allow sign another player from already sign player account
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIG_BY_ACCOUNT);

    stmt->setUInt32(0, GetAccountId());
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    result = CharacterDatabase.Query(stmt);

    if (result)
    {
        WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
        SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_ALREADY_SIGNED); // maybe _OTHER here?
        // close at signer side
        SendPacket(&data);
        return;
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION_SIGNATURE);

    stmt->setUInt32(0, GUID_LOPART(ownerGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));
    stmt->setUInt32(2, playerGuid);
    stmt->setUInt32(3, GetAccountId());

    CharacterDatabase.Execute(stmt);

    TC_LOG_DEBUG("network", "PETITION SIGN: GUID %u by player: %s (GUID: %u Account: %u)", GUID_LOPART(petitionGuid), _player->GetName().c_str(), playerGuid, GetAccountId());

    WorldPacket data(SMSG_PETITION_SIGN_RESULTS, (9+9+1));
    SendPetitionSignResult(&data, petitionGuid, GetPlayer()->GetGUID(), PETITION_SIGN_OK);
    // close at signer side
    SendPacket(&data);

    // update signs count on charter, required testing...
    //Item* item = _player->GetItemByGuid(petitionguid));
    //if (item)
    //    item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT+1, signs);

    // update for owner if online
    if (Player* owner = ObjectAccessor::FindPlayer(ownerGuid))
        owner->GetSession()->SendPacket(&data);
}

void WorldSession::HandlePetitionDeclineOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_PETITION_DECLINE");  // ok

    ObjectGuid PetitionGuid;
    
    PetitionGuid[2] = recvData.ReadBit();
    PetitionGuid[3] = recvData.ReadBit();
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[0] = recvData.ReadBit();
    PetitionGuid[7] = recvData.ReadBit();
    PetitionGuid[1] = recvData.ReadBit();
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[6] = recvData.ReadBit();
    
    recvData.ReadByteSeq(PetitionGuid[7]);
    recvData.ReadByteSeq(PetitionGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[1]);
    recvData.ReadByteSeq(PetitionGuid[3]);
    recvData.ReadByteSeq(PetitionGuid[6]);
    recvData.ReadByteSeq(PetitionGuid[4]);
    recvData.ReadByteSeq(PetitionGuid[2]);
    recvData.ReadByteSeq(PetitionGuid[0]);
    
    TC_LOG_DEBUG("network", "Petition %u declined by %u", GUID_LOPART(PetitionGuid), _player->GetGUIDLow());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_OWNER_BY_GUID);

    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    Field* fields = result->Fetch();
    uint64 ownerguid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    Player* owner = ObjectAccessor::FindPlayer(ownerguid);
    if (owner)                                               // petition owner online
    {
        WorldPacket data(SMSG_PETITION_DECLINED, 9);
        ObjectGuid PlayerGuid = GetPlayer()->GetGUID();
        
        data.WriteBit(PlayerGuid[4]);
        data.WriteBit(PlayerGuid[2]);
        data.WriteBit(PlayerGuid[1]);
        data.WriteBit(PlayerGuid[5]);
        data.WriteBit(PlayerGuid[7]);
        data.WriteBit(PlayerGuid[6]);
        data.WriteBit(PlayerGuid[0]);
        data.WriteBit(PlayerGuid[3]);
        
        data.FlushBits();
        
        data.WriteByteSeq(PlayerGuid[6]);
        data.WriteByteSeq(PlayerGuid[2]);
        data.WriteByteSeq(PlayerGuid[7]);
        data.WriteByteSeq(PlayerGuid[1]);
        data.WriteByteSeq(PlayerGuid[3]);
        data.WriteByteSeq(PlayerGuid[4]);
        data.WriteByteSeq(PlayerGuid[0]);
        data.WriteByteSeq(PlayerGuid[5]);
        
        owner->GetSession()->SendPacket(&data);
    }
}

void WorldSession::HandleOfferPetitionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_OFFER_PETITION");   // ok

    uint8 signs = 0;
    ObjectGuid PetitionGuid, PlayerGuid;
    uint32 type, SignaturesStart;
    Player* player;
    recvData >> SignaturesStart;                                      // if petition type == 1, sents MinSignatures + 1, if not - 0
    
    PlayerGuid[3] = recvData.ReadBit();
    PlayerGuid[2] = recvData.ReadBit();
    PlayerGuid[5] = recvData.ReadBit();
    PetitionGuid[4] = recvData.ReadBit();
    PlayerGuid[7] = recvData.ReadBit();
    PlayerGuid[6] = recvData.ReadBit();
    PetitionGuid[3] = recvData.ReadBit();
    PetitionGuid[7] = recvData.ReadBit();
    PetitionGuid[0] = recvData.ReadBit();
    PlayerGuid[4] = recvData.ReadBit();
    PetitionGuid[1] = recvData.ReadBit();
    PetitionGuid[6] = recvData.ReadBit();
    PetitionGuid[2] = recvData.ReadBit();
    PlayerGuid[1] = recvData.ReadBit();
    PetitionGuid[5] = recvData.ReadBit();
    PlayerGuid[0] = recvData.ReadBit();
    
    recvData.ReadByteSeq(PetitionGuid[2]);
    recvData.ReadByteSeq(PetitionGuid[3]);
    recvData.ReadByteSeq(PetitionGuid[1]);
    recvData.ReadByteSeq(PetitionGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[4]);
    recvData.ReadByteSeq(PlayerGuid[7]);
    recvData.ReadByteSeq(PetitionGuid[0]);
    recvData.ReadByteSeq(PlayerGuid[2]);
    recvData.ReadByteSeq(PlayerGuid[0]);
    recvData.ReadByteSeq(PlayerGuid[6]);
    recvData.ReadByteSeq(PetitionGuid[7]);
    recvData.ReadByteSeq(PlayerGuid[1]);
    recvData.ReadByteSeq(PlayerGuid[4]);
    recvData.ReadByteSeq(PlayerGuid[3]);
    recvData.ReadByteSeq(PlayerGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[6]);

    player = ObjectAccessor::FindPlayer(PlayerGuid);
    if (!player)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    Field* fields = result->Fetch();
    type = fields[0].GetUInt8();

    TC_LOG_DEBUG("network", "OFFER PETITION: type %u, GUID1 %u, to player id: %u", type, GUID_LOPART(PetitionGuid), GUID_LOPART(PlayerGuid));

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != player->GetTeam())
    {
        if (type == GUILD_CHARTER_TYPE)
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (player->GetGuildId())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_IN_GUILD_S, _player->GetName());
            return;
        }

        if (player->GetGuildIdInvited())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
            return;
        }
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);

    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));

    result = CharacterDatabase.Query(stmt);

    // result == NULL also correct charter without signs
    if (result)
        signs = uint8(result->GetRowCount());

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, (9+9+4+1+signs*13));
    ObjectGuid OwnerGuid = GetPlayer()->GetGUID();
    ByteBuffer SignedBytes;
    
    data.WriteBit(OwnerGuid[4]);
    data.WriteBit(PetitionGuid[4]);
    data.WriteBit(OwnerGuid[5]);
    data.WriteBit(OwnerGuid[0]);
    data.WriteBit(OwnerGuid[6]);
    data.WriteBit(PetitionGuid[7]);
    data.WriteBit(OwnerGuid[7]);
    data.WriteBit(PetitionGuid[0]);
    data.WriteBit(OwnerGuid[2]);
    data.WriteBit(OwnerGuid[3]);
    
    data.WriteBits(signs, 21);
    
    for (uint8 i = 1; i <= signs; ++i)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid SignedGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);
        
        data.WriteBit(SignedGuid[6]);
        data.WriteBit(SignedGuid[2]);
        data.WriteBit(SignedGuid[4]);
        data.WriteBit(SignedGuid[5]);
        data.WriteBit(SignedGuid[3]);
        data.WriteBit(SignedGuid[0]);
        data.WriteBit(SignedGuid[7]);
        data.WriteBit(SignedGuid[1]);
        
        SignedBytes.WriteByteSeq(SignedGuid[4]);
        SignedBytes.WriteByteSeq(SignedGuid[7]);
        SignedBytes.WriteByteSeq(SignedGuid[5]);
        SignedBytes.WriteByteSeq(SignedGuid[3]);
        SignedBytes.WriteByteSeq(SignedGuid[2]);
        SignedBytes << uint32(0);
        SignedBytes.WriteByteSeq(SignedGuid[6]);
        SignedBytes.WriteByteSeq(SignedGuid[1]);
        SignedBytes.WriteByteSeq(SignedGuid[0]);
        
        result->NextRow();
    }
    
    data.WriteBit(PetitionGuid[5]);
    data.WriteBit(PetitionGuid[6]);
    data.WriteBit(PetitionGuid[1]);
    data.WriteBit(PetitionGuid[3]);
    data.WriteBit(OwnerGuid[1]);
    data.WriteBit(PetitionGuid[2]);
    
    data.FlushBits();
    
    data << uint32(GUID_LOPART(PetitionGuid));
    data.append(SignedBytes);
    data.WriteByteSeq(PetitionGuid[2]);
    data.WriteByteSeq(OwnerGuid[1]);
    data.WriteByteSeq(OwnerGuid[6]);
    data.WriteByteSeq(PetitionGuid[3]);
    data.WriteByteSeq(OwnerGuid[7]);
    data.WriteByteSeq(PetitionGuid[0]);
    data.WriteByteSeq(OwnerGuid[0]);
    data.WriteByteSeq(OwnerGuid[2]);
    data.WriteByteSeq(PetitionGuid[4]);
    data.WriteByteSeq(PetitionGuid[7]);
    data.WriteByteSeq(PetitionGuid[6]);
    data.WriteByteSeq(OwnerGuid[4]);
    data.WriteByteSeq(OwnerGuid[3]);
    data.WriteByteSeq(PetitionGuid[5]);
    data.WriteByteSeq(PetitionGuid[1]);

    player->GetSession()->SendPacket(&data);
}

void WorldSession::HandleTurnInPetitionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_TURN_IN_PETITION");

    // Get petition guid from packet
    WorldPacket data;
    ObjectGuid PetitionGuid;

    PetitionGuid[2] = recvData.ReadBit();
    PetitionGuid[3] = recvData.ReadBit();
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[0] = recvData.ReadBit();
    PetitionGuid[7] = recvData.ReadBit();
    PetitionGuid[1] = recvData.ReadBit();
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[6] = recvData.ReadBit();
    
    recvData.ReadByteSeq(PetitionGuid[7]);
    recvData.ReadByteSeq(PetitionGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[1]);
    recvData.ReadByteSeq(PetitionGuid[3]);
    recvData.ReadByteSeq(PetitionGuid[6]);
    recvData.ReadByteSeq(PetitionGuid[4]);
    recvData.ReadByteSeq(PetitionGuid[2]);
    recvData.ReadByteSeq(PetitionGuid[0]);

    // Check if player really has the required petition charter
    Item* item = _player->GetItemByGuid(PetitionGuid);
    if (!item)
        return;

        TC_LOG_DEBUG("network", "Petition %u turned in by %u", GUID_LOPART(PetitionGuid), _player->GetGUIDLow());

    // Get petition data from db
    uint32 ownerguidlo;
    uint32 type;
    std::string name;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);
    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        ownerguidlo = fields[0].GetUInt32();
        name = fields[1].GetString();
        type = fields[2].GetUInt8();
    }
    else
    {
        TC_LOG_ERROR("network", "Player %s (guid: %u) tried to turn in petition (guid: %u) that is not present in the database", _player->GetName().c_str(), _player->GetGUIDLow(), GUID_LOPART(PetitionGuid));
        return;
    }

    // Only the petition owner can turn in the petition
    if (_player->GetGUIDLow() != ownerguidlo)
        return;

    // Petition type (guild/arena) specific checks
    if (type == GUILD_CHARTER_TYPE)
    {
        // Check if player is already in a guild
        if (_player->GetGuildId())
        {
            data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
            SendPetitionTurnResult(&data, PETITION_TURN_ALREADY_IN_GUILD);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // Check if guild name is already taken
        if (sGuildMgr->GetGuildByName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
            return;
        }
    }

    // Get petition signatures from db
    uint8 signatures;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));
    result = CharacterDatabase.Query(stmt);

    if (result)
        signatures = uint8(result->GetRowCount());
    else
        signatures = 0;

    uint32 requiredSignatures;
    if (type == GUILD_CHARTER_TYPE)
        requiredSignatures = sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS);
    else
        requiredSignatures = type-1;

    // Notify player if signatures are missing
    if (signatures < requiredSignatures)
    {
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
        SendPetitionTurnResult(&data, PETITION_TURN_NEED_MORE_SIGNATURES);
        SendPacket(&data);
        return;
    }

    // Proceed with guild/arena team creation

    // Delete charter item
    _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    if (type == GUILD_CHARTER_TYPE)
    {
        // Create guild
        Guild* guild = new Guild;

        if (!guild->Create(_player, name))
        {
            delete guild;
            return;
        }

        // Register guild and add guild master
        sGuildMgr->AddGuild(guild);

        Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_COMMAND_SUCCESS, name);

        // Add members from signatures
        for (uint8 i = 0; i < signatures; ++i)
        {
            Field* fields = result->Fetch();
            guild->AddMember(MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));
            result->NextRow();
        }
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(PetitionGuid));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    // created
    TC_LOG_DEBUG("network", "TURN IN PETITION GUID %u", GUID_LOPART(PetitionGuid));

    data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
    SendPetitionTurnResult(&data, PETITION_TURN_OK);
    SendPacket(&data);
}

void WorldSession::HandlePetitionShowListOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received CMSG_PETITION_SHOWLIST");

    ObjectGuid PetitionGuid;
    
    PetitionGuid[4] = recvData.ReadBit();
    PetitionGuid[3] = recvData.ReadBit();
    PetitionGuid[2] = recvData.ReadBit();
    PetitionGuid[7] = recvData.ReadBit();
    PetitionGuid[6] = recvData.ReadBit();
    PetitionGuid[1] = recvData.ReadBit();
    PetitionGuid[0] = recvData.ReadBit();
    PetitionGuid[5] = recvData.ReadBit();
    
    recvData.ReadByteSeq(PetitionGuid[5]);
    recvData.ReadByteSeq(PetitionGuid[0]);
    recvData.ReadByteSeq(PetitionGuid[6]);
    recvData.ReadByteSeq(PetitionGuid[2]);
    recvData.ReadByteSeq(PetitionGuid[1]);
    recvData.ReadByteSeq(PetitionGuid[7]);
    recvData.ReadByteSeq(PetitionGuid[3]);
    recvData.ReadByteSeq(PetitionGuid[4]);
    
    SendPetitionShowList(PetitionGuid);
}

void WorldSession::SendPetitionShowList(ObjectGuid guid)
{
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        TC_LOG_DEBUG("network", "WORLD: HandlePetitionShowListOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    WorldPacket data(SMSG_PETITION_SHOWLIST, 8+1+4);
    
    data.WriteBit(guid[3]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[5]);
    
    data.FlushBits();
    
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[3]);
    data << uint32(GUILD_CHARTER_COST);
    data.WriteByteSeq(guid[6]);

    SendPacket(&data);
    TC_LOG_DEBUG("network", "Sent SMSG_PETITION_SHOWLIST");
}
