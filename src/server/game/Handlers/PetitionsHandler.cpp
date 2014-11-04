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
#include "Language.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "Arena.h"
#include "GossipDef.h"
#include "SocialMgr.h"

#define CHARTER_DISPLAY_ID 16161

/*enum PetitionType // dbc data
{
    PETITION_TYPE_GUILD      = 1,
    PETITION_TYPE_ARENA_TEAM = 3
};*/

// Charters ID in item_template
enum CharterItemIDs
{
    GUILD_CHARTER                                 = 5863,
    ARENA_TEAM_CHARTER_2v2                        = 23560,
    ARENA_TEAM_CHARTER_3v3                        = 23561,
    ARENA_TEAM_CHARTER_5v5                        = 23562
};

enum CharterCosts
{
    GUILD_CHARTER_COST                            = 1000,
    ARENA_TEAM_CHARTER_2v2_COST                   = 800000,
    ARENA_TEAM_CHARTER_3v3_COST                   = 1200000,
    ARENA_TEAM_CHARTER_5v5_COST                   = 2000000
};

void WorldSession::HandlePetitionBuyOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_BUY");
    ObjectGuid npcGuid;
    uint32 nameLen = 0;
    std::string name;

    npcGuid[6] = recvData.ReadBit();
    npcGuid[1] = recvData.ReadBit();
    npcGuid[4] = recvData.ReadBit();
    npcGuid[2] = recvData.ReadBit();
    npcGuid[5] = recvData.ReadBit();
    npcGuid[7] = recvData.ReadBit();
    npcGuid[3] = recvData.ReadBit();
    nameLen = recvData.ReadBits(7);
    npcGuid[0] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(npcGuid[0]);
    recvData.ReadByteSeq(npcGuid[2]);
    recvData.ReadByteSeq(npcGuid[4]);
    recvData.ReadByteSeq(npcGuid[6]);
    recvData.ReadByteSeq(npcGuid[7]);
    recvData.ReadByteSeq(npcGuid[5]);
    recvData.ReadByteSeq(npcGuid[3]);
    recvData.ReadByteSeq(npcGuid[1]);
    
    name = recvData.ReadString(nameLen);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petitioner with GUID %u tried sell petition: name %s", GUID_LOPART(npcGuid), name.c_str());

    // prevent cheating
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandlePetitionBuyOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(npcGuid));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    uint32 charterid = 0;
    uint32 cost = 0;
    uint32 type = 0;
    if (creature->isTabardDesigner())
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
            Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, name);
            return;
        }
        if (sObjectMgr->IsReservedName(name) || !ObjectMgr::IsValidCharterName(name))
        {
            Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_INVALID, name);
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

    charter->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1, charter->GetGUIDLow());
    // ITEM_FIELD_ENCHANTMENT_1_1 is guild/arenateam id
    // ITEM_FIELD_ENCHANTMENT_1_1+1 is current signatures count (showed on item)
    charter->SetState(ITEM_CHANGED, _player);
    _player->SendNewItem(charter, 1, true, false);

    // a petition is invalid, if both the owner and the type matches
    // we checked above, if this player is in an arenateam, so this must be
    // datacorruption
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
        }
        while (result->NextRow());
    }

    // delete petitions with the same guid as this one
    ssInvalidPetitionGUIDs << '\'' << charter->GetGUIDLow() << '\'';

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Invalid petition GUIDs: %s", ssInvalidPetitionGUIDs.str().c_str());
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
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_SHOW_SIGNATURES");

    uint8 signs = 0;
    ObjectGuid petitionguid;

    uint8 bitsOrder[8] = { 4, 3, 5, 6, 2, 1, 0, 7 };
    recvData.ReadBitInOrder(petitionguid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 2, 0, 3, 7, 4, 1, 5, 6 };
    recvData.ReadBytesSeq(petitionguid, bytesOrder);

    // solve (possible) some strange compile problems with explicit use GUID_LOPART(petitionguid) at some GCC versions (wrong code optimization in compiler?)
    uint32 petitionGuidLow = GUID_LOPART(petitionguid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, petitionGuidLow);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outDebug(LOG_FILTER_PLAYER_ITEMS, "Petition %u is not found for player %u %s", GUID_LOPART(petitionguid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName());
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

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_SHOW_SIGNATURES petition entry: '%u'", petitionGuidLow);

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES);
    ByteBuffer signsBuffer;
    ObjectGuid guid2 = _player->GetGUID();
    ObjectGuid guid1 = petitionguid;

    data.WriteBit(guid2[4]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid2[3]);
    data.WriteBits(signs, 21);

    for (uint8 i = 1; i <= signs; ++i)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();

        ObjectGuid signerGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        uint8 bitsSendOrder[8] = { 6, 2, 4, 5, 3, 0, 7, 1 };
        data.WriteBitInOrder(signerGuid, bitsSendOrder);

        signsBuffer.WriteByteSeq(signerGuid[4]);
        signsBuffer.WriteByteSeq(signerGuid[7]);
        signsBuffer.WriteByteSeq(signerGuid[5]);
        signsBuffer.WriteByteSeq(signerGuid[3]);
        signsBuffer.WriteByteSeq(signerGuid[2]);
        signsBuffer << uint32(0);
        signsBuffer.WriteByteSeq(signerGuid[6]);
        signsBuffer.WriteByteSeq(signerGuid[1]);
        signsBuffer.WriteByteSeq(signerGuid[0]);

        result->NextRow();
    }

    data.WriteBit(guid1[5]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[2]);
    data.FlushBits();

    data << uint32(petitionGuidLow);
    if (signsBuffer.size())
        data.append(signsBuffer);

    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid2[4]);
    data.WriteByteSeq(guid2[3]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid1[1]);

    SendPacket(&data);
}

void WorldSession::HandlePetitionQueryOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_QUERY");   // ok

    uint32 guildId;
    ObjectGuid petitionGuid;

    recvData >> guildId;

    uint8 bitsOrder[8] = { 1, 3, 4, 7, 5, 0, 2, 6 };
    recvData.ReadBitInOrder(petitionGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 6, 5, 1, 7, 4, 3, 2, 0 };
    recvData.ReadBytesSeq(petitionGuid, bytesOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY Petition GUID %u Guild GUID %u", GUID_LOPART(petitionGuid), guildId);

    SendPetitionQueryOpcode(uint64(guildId));
}

void WorldSession::SendPetitionQueryOpcode(uint64 petitionguid)
{
    uint64 ownerguid = 0;
    uint32 type;
    std::string name = "NO_NAME_FOR_GUID";

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);

    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        ownerguid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        name      = fields[1].GetString();
        type      = fields[2].GetUInt8();
    }
    else
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY failed for petition (GUID: %u)", GUID_LOPART(petitionguid));
        return;
    }

    WorldPacket data(SMSG_PETITION_QUERY_RESPONSE);
    ObjectGuid ownerGuid = ownerguid;

    data.WriteBit(type == GUILD_CHARTER_TYPE);

    if (type == GUILD_CHARTER_TYPE)
    {
        data.WriteBits(name.size(), 7);
        data.WriteBit(ownerGuid[6]);
        data.WriteBit(ownerGuid[1]);
        for (int i = 0; i < 10; ++i)
            data.WriteBits(0, 6);               // Unk Strings

        data.WriteBit(ownerGuid[2]);
        data.WriteBit(ownerGuid[5]);
        data.WriteBit(ownerGuid[3]);
        data.WriteBit(ownerGuid[0]);
        data.WriteBits(0, 12);                  // Unk String
        data.WriteBit(ownerGuid[7]);
        data.WriteBit(ownerGuid[4]);

        data << uint32(0);
        data << uint32(0);
        data.WriteByteSeq(ownerGuid[3]);
        data << uint32(4);
        data.WriteByteSeq(ownerGuid[1]);
        data << uint32(0);
        data << uint32(0);
        data.WriteByteSeq(ownerGuid[0]);
        data.WriteByteSeq(ownerGuid[5]);
        data << uint32(0);
        data.WriteByteSeq(ownerGuid[7]);
        data << uint32(0);
        data << uint32(0);
        data.WriteByteSeq(ownerGuid[2]);
        data.WriteByteSeq(ownerGuid[4]);
        if (name.size())
            data.append(name.c_str(), name.size());
        data.WriteByteSeq(ownerGuid[6]);
        
        data << uint32(4);
        data << uint16(0);
        data << uint32(GUID_LOPART(petitionguid));
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        
        //data << uint32(GUID_LOPART(petitionguid));
    }

    data << uint32(GUID_LOPART(petitionguid));

    SendPacket(&data);
}

void WorldSession::HandlePetitionRenameOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_RENAME");

    ObjectGuid petitionGuid;
    uint32 type = 0;
    std::string newName;
    uint32 nameLen = 0;

    petitionGuid[7] = recvData.ReadBit();
    petitionGuid[1] = recvData.ReadBit();
    petitionGuid[0] = recvData.ReadBit();
    petitionGuid[2] = recvData.ReadBit();
    nameLen = recvData.ReadBits(7);
    petitionGuid[3] = recvData.ReadBit();
    petitionGuid[5] = recvData.ReadBit();
    petitionGuid[4] = recvData.ReadBit();
    petitionGuid[6] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(petitionGuid[3]);
    recvData.ReadByteSeq(petitionGuid[0]);
    recvData.ReadByteSeq(petitionGuid[5]);
    newName = recvData.ReadString(nameLen);
    recvData.ReadByteSeq(petitionGuid[7]);
    recvData.ReadByteSeq(petitionGuid[4]);
    recvData.ReadByteSeq(petitionGuid[2]);
    recvData.ReadByteSeq(petitionGuid[6]);
    recvData.ReadByteSeq(petitionGuid[1]);

    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY failed for petition (GUID: %u)", GUID_LOPART(petitionGuid));
        return;
    }

    if (sGuildMgr->GetGuildByName(newName))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, newName);
        return;
    }
    if (sObjectMgr->IsReservedName(newName) || !ObjectMgr::IsValidCharterName(newName))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_INVALID, newName);
        return;
    }

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PETITION_NAME);
    stmt->setString(0, newName);
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));
    CharacterDatabase.Execute(stmt);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition (GUID: %u) renamed to '%s'", GUID_LOPART(petitionGuid), newName.c_str());

    WorldPacket data(SMSG_PETITION_RENAME);

    data.WriteBit(petitionGuid[1]);
    data.WriteBit(petitionGuid[7]);
    data.WriteBit(petitionGuid[3]);
    data.WriteBit(petitionGuid[0]);
    data.WriteBits(nameLen, 7);
    data.WriteBit(petitionGuid[2]);
    data.WriteBit(petitionGuid[5]);
    data.WriteBit(petitionGuid[6]);
    data.WriteBit(petitionGuid[4]);

    data.WriteByteSeq(petitionGuid[7]);
    data.WriteByteSeq(petitionGuid[2]);
    data.WriteByteSeq(petitionGuid[1]);
    data.WriteByteSeq(petitionGuid[3]);
    data.WriteByteSeq(petitionGuid[0]);
    data.WriteByteSeq(petitionGuid[4]);
    data.WriteByteSeq(petitionGuid[5]);
    if (nameLen)
        data.append(newName.c_str(), nameLen);
    data.WriteByteSeq(petitionGuid[6]);

    SendPacket(&data);
}

void WorldSession::HandlePetitionSignOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_SIGN");

    Field* fields;
    ObjectGuid petitionGuid;
    uint8 unk;

    recvData >> unk;

    uint8 bitsOrder[8] = { 5, 2, 0, 7, 1, 4, 6, 3 };
    recvData.ReadBitInOrder(petitionGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 5, 0, 6, 4, 2, 7, 1, 3 };
    recvData.ReadBytesSeq(petitionGuid, bytesOrder);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    fields = result->Fetch();
    ObjectGuid ownerGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURES);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));
    result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Petition %u is not found for player %u %s", GUID_LOPART(petitionGuid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName());
        return;
    }

    fields = result->Fetch();
    uint64 signs = fields[1].GetUInt64();
    uint8 type = fields[2].GetUInt8();

    uint32 playerGuid = _player->GetGUIDLow();
    if (GUID_LOPART(ownerGuid) == playerGuid)
        return;

    // not let enemies sign guild charter
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(ownerGuid))
    {
        if (type == GUILD_CHARTER_TYPE)
            Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (_player->GetGuildId())
        {
            Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_IN_GUILD_S, _player->GetName());
            return;
        }
        if (_player->GetGuildIdInvited())
        {
            Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
            return;
        }
    }

    if (++signs > type)                                        // client signs maximum
        return;

    // Client doesn't allow to sign petition two times by one character, but not check sign by another character from same account
    // not allow sign another player from already sign player account
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIG_BY_ACCOUNT);

    stmt->setUInt32(0, GetAccountId());
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    result = CharacterDatabase.Query(stmt);

    if (result)
    {
        SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_ALREADY_SIGNED);
        return;
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION_SIGNATURE);

    stmt->setUInt32(0, GUID_LOPART(ownerGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));
    stmt->setUInt32(2, playerGuid);
    stmt->setUInt32(3, GetAccountId());

    CharacterDatabase.Execute(stmt);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "PETITION SIGN: GUID %u by player: %s (GUID: %u Account: %u)", GUID_LOPART(petitionGuid), _player->GetName(), playerGuid, GetAccountId());

    SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_OK);

    Player* owner = ObjectAccessor::FindPlayer(ownerGuid);
    if (owner)
        owner->GetSession()->SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_OK);
}

void WorldSession::HandlePetitionDeclineOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_DECLINE");

    ObjectGuid petitionGuid;

    uint8 bitsOrder[8] = { 1, 4, 6, 7, 3, 2, 0, 5 };
    recvData.ReadBitInOrder(petitionGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 5, 3, 4, 6, 0, 7, 2, 1 };
    recvData.ReadBytesSeq(petitionGuid, bytesOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition %u declined by %u", GUID_LOPART(petitionGuid), _player->GetGUIDLow());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    Field* fields = result->Fetch();
    ObjectGuid ownerGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    Player* owner = ObjectAccessor::FindPlayer(ownerGuid);
    if (owner)
        owner->GetSession()->SendPetitionSignResult(ownerGuid, petitionGuid, PETITION_SIGN_DECLINED);
}

void WorldSession::HandleOfferPetitionOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_OFFER_PETITION");

    ObjectGuid petitionGuid, playerGuid;
    uint32 type, junk;
    Player* player;

    ObjectGuid guid1;
    ObjectGuid guid2;
    recvData >> junk;                                      // this is not petition type!

    guid1[3] = recvData.ReadBit();
    guid1[2] = recvData.ReadBit();
    guid1[5] = recvData.ReadBit();
    guid2[4] = recvData.ReadBit();
    guid1[7] = recvData.ReadBit();
    guid1[6] = recvData.ReadBit();
    guid2[3] = recvData.ReadBit();
    guid2[7] = recvData.ReadBit();
    guid2[0] = recvData.ReadBit();
    guid1[4] = recvData.ReadBit();
    guid2[1] = recvData.ReadBit();
    guid2[6] = recvData.ReadBit();
    guid2[2] = recvData.ReadBit();
    guid1[1] = recvData.ReadBit();
    guid2[5] = recvData.ReadBit();
    guid1[0] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(guid2[2]);
    recvData.ReadByteSeq(guid2[3]);
    recvData.ReadByteSeq(guid2[1]);
    recvData.ReadByteSeq(guid2[5]);
    recvData.ReadByteSeq(guid2[4]);
    recvData.ReadByteSeq(guid1[7]);
    recvData.ReadByteSeq(guid2[0]);
    recvData.ReadByteSeq(guid1[2]);
    recvData.ReadByteSeq(guid1[0]);
    recvData.ReadByteSeq(guid1[6]);
    recvData.ReadByteSeq(guid2[7]);
    recvData.ReadByteSeq(guid1[1]);
    recvData.ReadByteSeq(guid1[4]);
    recvData.ReadByteSeq(guid1[3]);
    recvData.ReadByteSeq(guid1[5]);
    recvData.ReadByteSeq(guid2[6]);

    petitionGuid = guid1;
    playerGuid = guid2;

    player = ObjectAccessor::FindPlayer(playerGuid);
    if (!player)
        return;

    type = GUILD_CHARTER_TYPE;

    uint32 petitionGuidLow = GUID_LOPART(petitionGuid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "OFFER PETITION: type %u, GUID1 %u, to player id: %u", type, petitionGuidLow, GUID_LOPART(playerGuid));

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != player->GetTeam())
    {
        if (type == GUILD_CHARTER_TYPE)
            Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (type == GUILD_CHARTER_TYPE)
    {
        if (player->GetGuildIdInvited())
        {
            SendPetitionSignResult(_player->GetGUID(), MAKE_NEW_GUID(petitionGuidLow, 0, HIGHGUID_ITEM), PETITION_SIGN_ALREADY_SIGNED_OTHER);
            return;
        }

        if (player->GetGuildId())
        {
            SendPetitionSignResult(_player->GetGUID(), MAKE_NEW_GUID(petitionGuidLow, 0, HIGHGUID_ITEM), PETITION_SIGN_ALREADY_IN_GUILD);
            return;
        }
    }

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, petitionGuidLow);
    auto result = CharacterDatabase.Query(stmt);

    typedef std::vector<uint32> storage;
    storage loParts;

    // result == NULL also correct charter without signs
    if (result)
    {
        loParts.reserve(uint32(result->GetRowCount()));

        do
        {
            auto fields = result->Fetch();
            auto loPart = fields[0].GetUInt32();
            if (GUID_LOPART(playerGuid) == loPart)
            {
                player->GetSession()->SendAlreadySigned(playerGuid);
                return;
            }

            loParts.push_back(loPart);
        }
        while (result->NextRow());
    }

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES);
    ByteBuffer signsBuffer;

    guid2 = _player->GetGUID();
    //ObjectGuid guid1 = petitionGuid;

    data.WriteBit(guid2[4]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid2[3]);
    data.WriteBits(loParts.size(), 21);

    for (auto lowGuid : loParts)
    {
        ObjectGuid signerGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        uint8 bitsSendOrder[8] = { 6, 2, 4, 5, 3, 0, 7, 1 };
        data.WriteBitInOrder(signerGuid, bitsSendOrder);

        signsBuffer.WriteByteSeq(signerGuid[4]);
        signsBuffer.WriteByteSeq(signerGuid[7]);
        signsBuffer.WriteByteSeq(signerGuid[5]);
        signsBuffer.WriteByteSeq(signerGuid[3]);
        signsBuffer.WriteByteSeq(signerGuid[2]);
        signsBuffer << uint32(0);
        signsBuffer.WriteByteSeq(signerGuid[6]);
        signsBuffer.WriteByteSeq(signerGuid[1]);
        signsBuffer.WriteByteSeq(signerGuid[0]);
    }

    data.WriteBit(guid1[5]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[2]);
    data.FlushBits();

    data << uint32(petitionGuidLow);
    if (signsBuffer.size())
        data.append(signsBuffer);

    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid2[4]);
    data.WriteByteSeq(guid2[3]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid1[1]);

    player->GetSession()->SendPacket(&data);
}

void WorldSession::HandleTurnInPetitionOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_TURN_IN_PETITION");

    // Get petition guid from packet
    WorldPacket data;
    ObjectGuid petitionGuid;

    uint8 bitsOrder[8] = { 2, 3, 5, 0, 7, 1, 4, 6 };
    recvData.ReadBitInOrder(petitionGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 7, 5, 1, 3, 6, 4, 2, 0 };
    recvData.ReadBytesSeq(petitionGuid, bytesOrder);

    // Check if player really has the required petition charter
    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition %u turned in by %u", GUID_LOPART(petitionGuid), _player->GetGUIDLow());

    // Get petition data from db
    uint32 ownerguidlo;
    uint32 type;
    std::string name;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
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
        sLog->outError(LOG_FILTER_NETWORKIO, "Player %s (guid: %u) tried to turn in petition (guid: %u) that is not present in the database", _player->GetName(), _player->GetGUIDLow(), GUID_LOPART(petitionGuid));
        return;
    }

    // Only the petition owner can turn in the petition
    if (_player->GetGUIDLow() != ownerguidlo)
        return;

    // Check if player is already in a guild
    if (_player->GetGuildId())
    {
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 4);
        data.WriteBits(PETITION_TURN_ALREADY_IN_GUILD, 4);
        data.FlushBits();
        SendPacket(&data);
        return;
    }

    // Check if guild name is already taken
    if (sGuildMgr->GetGuildByName(name))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, name);
        return;
    }

    // Get petition signatures from db
    uint8 signatures;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    result = CharacterDatabase.Query(stmt);

    if (result)
        signatures = uint8(result->GetRowCount());
    else
        signatures = 0;

    uint32 requiredSignatures;
    requiredSignatures = sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS);

    // Notify player if signatures are missing
    if (signatures < requiredSignatures)
    {
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 4);
        data.WriteBits(PETITION_TURN_NEED_MORE_SIGNATURES, 4);
        data.FlushBits();
        SendPacket(&data);
        return;
    }

    // Proceed with guild/arena team creation

    // Delete charter item
    _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    // Create guild
    Guild* guild = new Guild;

    if (!guild->Create(_player, name))
    {
        delete guild;
        return;
    }

    // Register guild and add guild master
    sGuildMgr->AddGuild(guild);

    // Add members from signatures
    for (uint8 i = 0; i < signatures; ++i)
    {
        Field* fields = result->Fetch();
        guild->AddMember(MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));
        result->NextRow();
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    // created
    sLog->outDebug(LOG_FILTER_NETWORKIO, "TURN IN PETITION GUID %u", GUID_LOPART(petitionGuid));

    data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 4);
    data.WriteBits(PETITION_TURN_OK, 4);
    data.FlushBits();
    SendPacket(&data);
}

void WorldSession::HandlePetitionShowListOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received CMSG_PETITION_SHOWLIST");

    uint64 guid;
    recvData >> guid;

    SendPetitionShowList(guid);
}

void WorldSession::SendPetitionShowList(uint64 guid)
{
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandlePetitionShowListOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    WorldPacket data(SMSG_PETITION_SHOW_LIST, 8 + 1 + 4 * 6);
    ObjectGuid npcGuid = guid;

    uint8 bitsOrder[8] = { 3, 6, 2, 0, 1, 4, 7, 5 };
    data.WriteBitInOrder(npcGuid, bitsOrder);

    data.WriteByteSeq(npcGuid[2]);
    data.WriteByteSeq(npcGuid[7]);
    data.WriteByteSeq(npcGuid[5]);
    data.WriteByteSeq(npcGuid[4]);
    data.WriteByteSeq(npcGuid[1]);
    data.WriteByteSeq(npcGuid[0]);
    data.WriteByteSeq(npcGuid[3]);
    data << uint32(GUILD_CHARTER_COST);                 // charter cost
    data.WriteByteSeq(npcGuid[6]);

    SendPacket(&data);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Sent SMSG_PETITION_SHOW_LIST");
}

void WorldSession::SendPetitionSignResult(ObjectGuid ownerGuid, ObjectGuid petitionGuid, uint8 result)
{
    ObjectGuid guid2 = ownerGuid;
    ObjectGuid guid1 = petitionGuid;

    WorldPacket data(SMSG_PETITION_DECLINED);

    data.WriteBit(guid2[5]);
    data.WriteBit(guid2[4]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid1[2]);
    data.WriteBits(PetitionSigns(result), 4);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[3]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[5]);

    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid2[3]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid2[4]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid1[1]);

    SendPacket(&data);
}

void WorldSession::SendAlreadySigned(ObjectGuid playerGuid)
{
    WorldPacket data(SMSG_PETITION_ALREADY_SIGNED);

    uint8 bitsOrder[8] = { 4, 2, 1, 5, 7, 6, 0, 3 };
    data.WriteBitInOrder(playerGuid, bitsOrder);

    uint8 bytesOder[8] = { 6, 2, 7, 1, 3, 4, 0, 5 };
    data.WriteBytesSeq(playerGuid, bytesOder);

    SendPacket(&data);
}
