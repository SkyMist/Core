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
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "UpdateMask.h"
#include "NPCHandler.h"
#include "Pet.h"
#include "MapManager.h"
#include "Group.h"

void WorldSession::SendNameQueryOpcode(ObjectGuid guid)
{
    Player* player = ObjectAccessor::FindPlayer(guid);
    CharacterNameData const* nameData = sWorld->GetCharacterNameData(GUID_LOPART(guid));

    WorldPacket data(SMSG_NAME_QUERY_RESPONSE, 500);

    uint8 guidOrder[8] = {4, 0, 2, 6, 5, 3, 1, 7};
    data.WriteBitInOrder(guid, guidOrder);

    data.WriteByteSeq(guid[1]);

    data << uint8(!nameData);
    if (nameData)
    {
        data << uint32(realmID);
        data << uint32(50397209);
        data << uint8(nameData->m_level);
        data << uint8(nameData->m_race);
        data << uint8(nameData->m_gender);
        data << uint8(nameData->m_class);
    }

    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[6]);

    if (nameData)
    {
        ObjectGuid unkGuid = 0;
        ObjectGuid pGuid2 = guid;

        data.WriteBit(unkGuid[1]);
        data.WriteBit(pGuid2[2]);
        data.WriteBit(pGuid2[5]);
        data.WriteBit(pGuid2[0]);
        data.WriteBit(pGuid2[7]);
        data.WriteBit(unkGuid[5]);
        data.WriteBit(pGuid2[3]);
        data.WriteBit(unkGuid[4]);
        data.WriteBit(0);
        data.WriteBit(pGuid2[6]);
        data.WriteBits(nameData->m_name.size(), 6);
        data.WriteBit(unkGuid[2]);
        data.WriteBit(unkGuid[6]);
        data.WriteBit(unkGuid[0]);
        data.WriteBit(pGuid2[1]);
        data.WriteBit(pGuid2[4]);
    
        if (DeclinedName const* names = (player ? player->GetDeclinedNames() : NULL))
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                data.WriteBits(names->name[i].size(), 7);
        }
        else
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                data.WriteBits(0, 7);
        }

        data.WriteBit(unkGuid[7]);
        data.WriteBit(unkGuid[3]);

        data.FlushBits();
        
        if (DeclinedName const* names = (player ? player->GetDeclinedNames() : NULL))
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                if (names->name[i].size())
                    data.WriteString(names->name[i]);
        }
        
        data.WriteByteSeq(pGuid2[4]);
        data.WriteByteSeq(pGuid2[5]);
        data.WriteByteSeq(pGuid2[7]);
        data.WriteByteSeq(pGuid2[0]);
        data.WriteByteSeq(unkGuid[7]);
        data.WriteByteSeq(unkGuid[0]);
        data.WriteByteSeq(unkGuid[1]);
        data.WriteByteSeq(unkGuid[4]);
        data.WriteByteSeq(pGuid2[1]);
        data.WriteByteSeq(unkGuid[2]);
        data.WriteByteSeq(unkGuid[5]);
        data.WriteByteSeq(pGuid2[6]);
        data.WriteByteSeq(pGuid2[2]);
        data.WriteByteSeq(pGuid2[3]);

        data.WriteString(nameData->m_name);

        data.WriteByteSeq(unkGuid[3]);
        data.WriteByteSeq(unkGuid[6]);
    }

    SendPacket(&data);
}

void WorldSession::HandleNameQueryOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[4] = recvData.ReadBit();

    bool hasUnkBit1 = recvData.ReadBit();

    guid[2] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    bool hasUnkBit2 = recvData.ReadBit();

    recvData.FlushBits();

    uint8 order[8] = {1, 0, 2, 6, 4, 7, 5, 3};
    recvData.ReadBytesSeq(guid, order);

    if (hasUnkBit2)
    {
        uint32 unk = recvData.read<uint32>();
        sLog->outInfo(LOG_FILTER_NETWORKIO, "CMSG_NAME_QUERY uint32 unk : %u\r\n", unk);
    }

    if (hasUnkBit1)
    {
        uint32 unk1 = recvData.read<uint32>();
        sLog->outInfo(LOG_FILTER_NETWORKIO, "CMSG_NAME_QUERY uint32 unk1 (realm flags / id ?) : %u\r\n", unk1);
    }

    // This is disable by default to prevent lots of console spam
    // sLog->outInfo(LOG_FILTER_NETWORKIO, "HandleNameQueryOpcode %u", guid);

    SendNameQueryOpcode(guid);
}

void WorldSession::HandleQueryTimeOpcode(WorldPacket& /*recvData*/)
{
    SendQueryTimeResponse();
}

void WorldSession::SendQueryTimeResponse()
{
    WorldPacket data(SMSG_QUERY_TIME_RESPONSE, 4+4);
    data << uint32(time(NULL));
    data << uint32(sWorld->GetNextDailyQuestsResetTime() - time(NULL));
    SendPacket(&data);
}

void WorldSession::SendServerWorldInfo()
{
    bool IsInInstance = GetPlayer()->GetMap()->IsRaidOrHeroicDungeon();                   // Check being in raid / heroic dungeon map.
    bool HasGroup = GetPlayer()->GetGroup() != NULL;                                      // Check having a group.
    uint32 InstanceGroupSize = HasGroup ? GetPlayer()->GetGroup()->GetMembersCount() : 0; // Check if we need to send the instance group size - for Flex Raids.
    uint32 difficultyNumberToDisplay = 0;                                                 // Number to display in minimap text.

    switch(GetPlayer()->GetMap()->GetDifficulty())
    {
        case DUNGEON_DIFFICULTY_HEROIC:
            difficultyNumberToDisplay = 5;
            break;

        case RAID_DIFFICULTY_10MAN_NORMAL:
        case RAID_DIFFICULTY_10MAN_HEROIC:
            difficultyNumberToDisplay = 10;
            break;

        case RAID_DIFFICULTY_25MAN_NORMAL:
        case RAID_DIFFICULTY_25MAN_HEROIC:
        case RAID_DIFFICULTY_25MAN_LFR:
            difficultyNumberToDisplay = 25;
            break;

        case RAID_DIFFICULTY_40MAN:
            difficultyNumberToDisplay = 40;
            break;

        case SCENARIO_DIFFICULTY_HEROIC:
            difficultyNumberToDisplay = 3;
            break;

        case RAID_DIFFICULTY_1025MAN_FLEX:
            difficultyNumberToDisplay = HasGroup ? InstanceGroupSize : 10;
            break;

        case REGULAR_DIFFICULTY:
        case DUNGEON_DIFFICULTY_NORMAL:
        case DUNGEON_DIFFICULTY_CHALLENGE:
        case SCENARIO_DIFFICULTY_NORMAL:
        default: break;
    }

    WorldPacket data(SMSG_WORLD_SERVER_INFO);

    data.WriteBit(0);                                                // IsTrialAccount() - Has restriction on level.
    data.WriteBit(IsInInstance);
    data.WriteBit(0);                                                // IsTrialAccount() - Has money restriction.             
    data.WriteBit(0);                                                // IsTrialAccount() - Is ineligible for loot.

    data.FlushBits();

    // if (IsTrialAccount()) 
    //     data << uint32(0);                                                          // Is ineligible for loot - EncounterMask of the creatures the player cannot loot.

    data << uint32(sWorld->GetNextWeeklyQuestsResetTime() - WEEK);                  // LastWeeklyReset (quests, not instance reset).
    data << uint32(GetPlayer()->GetMap()->GetDifficulty());                         // Current Map Difficulty.
    data << uint8(0);                                                               // sWorld->getBoolConfig(CONFIG_IS_TOURNAMENT_REALM) IsOnTournamentRealm.

    // if (IsTrialAccount()) 
    //     data << uint32(sWorld->getIntConfig(CONFIG_TRIAL_MAX_MONEY));               // Has money restriction - Max amount of money allowed.

    // if (IsTrialAccount()) 
    //     data << uint32(sWorld->getIntConfig(CONFIG_TRIAL_MAX_LEVEL));               // Has restriction on level - Max level allowed.

    // This should be sent with the maximum player number as text, for raids & heroic dungeons, except for flex raids where they scale (that's why lua uses instanceGroupSize).
    if (IsInInstance)
        data << uint32(difficultyNumberToDisplay);

    SendPacket(&data);
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleCreatureQueryOpcode(WorldPacket& recvData)
{
    uint32 entry;
    recvData >> entry;

    CreatureTemplate const* info = sObjectMgr->GetCreatureTemplate(entry);
    uint32 entryToSend = info ? entry : 0x80000000 | entry;

    WorldPacket data(SMSG_CREATURE_QUERY_RESPONSE, 500);

    data << uint32(entryToSend);
    data.WriteBit(info != 0);                                    // Has data

    if (info)
    {
        std::string Name, SubName;
        Name = info->Name;
        SubName = info->SubName;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (CreatureLocale const* cl = sObjectMgr->GetCreatureLocale(entry))
            {
                ObjectMgr::GetLocaleString(cl->Name, loc_idx, Name);
                ObjectMgr::GetLocaleString(cl->SubName, loc_idx, SubName);
            }
        }

        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_CREATURE_QUERY '%s' - Entry: %u.", info->Name.c_str(), entry);

        data.WriteBit(info->RacialLeader);
        data.WriteBits(info->IconName.length() ? info->IconName.length() + 1 : 0, 6);

        for (int i = 0; i < 8; i++)
        {
            if (i == 1)
                data.WriteBits(Name.length() + 1, 11);
            else
                data.WriteBits(0, 11);                       // Name2, ..., name8
        }

        uint8 itemCount = 0;
        for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
            if (info->questItems[i])
                itemCount++;                                // itemId[6], quest drop

        data.WriteBits(itemCount, 22);
        data.WriteBits(SubName.length() ? SubName.length() + 1 : 0, 11);

        data.WriteBits(0, 11);
        data.FlushBits();

        data << float(info->ModMana);                         // dmg/mana modifier
        data << Name;                                         // Name        
        data << float(info->ModHealth);                       // dmg/hp modifier
        data << uint32(info->KillCredit[1]);                  // new in 3.1, kill credit
        data << uint32(info->Modelid2);                       // Modelid2

        for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS && itemCount > 0; ++i)
        {
            if (info->questItems[i])
            {
                data << uint32(info->questItems[i]);
                itemCount--;
            }
        }

        data << uint32(info->type);                           // CreatureType.dbc

        if (info->IconName.length())
            data << info->IconName;                           // Icon Name

        data << uint32(info->type_flags);                     // Flag
        data << uint32(info->type_flags2);                    // Flags2
        data << uint32(info->KillCredit[0]);                  // new in 3.1, kill credit
        data << uint32(info->family);                         // CreatureFamily.dbc
        data << uint32(info->movementId);                     // CreatureMovementInfo.dbc
        data << uint32(info->expansionUnknown);               // unknown meaning
        data << uint32(info->Modelid1);                       // Modelid1
        data << uint32(info->Modelid3);                       // Modelid3
        data << uint32(info->rank);                           // Creature Rank (elite, boss, etc)

        if (SubName.length())
            data << SubName;                                  // Sub Name

        data << uint32(info->Modelid4);                       // Modelid4

        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE");
    }
    else
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_CREATURE_QUERY - NO CREATURE INFO! (ENTRY: %u)", entry);

    SendPacket(&data);
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleGameObjectQueryOpcode(WorldPacket& recvData)
{
    uint32 entry;
    ObjectGuid guid;

    recvData >> entry;

    uint8 bitOrder[8] = { 1, 7, 0, 3, 5, 4, 6, 2 };
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = { 3, 6, 1, 2, 0, 7, 5, 4 };
    recvData.ReadBytesSeq(guid, byteOrder);
    
    const GameObjectTemplate* info = sObjectMgr->GetGameObjectTemplate(entry);

    uint32 entryToSend = info ? entry : 0x80000000 | entry;
    
    WorldPacket data (SMSG_GAMEOBJECT_QUERY_RESPONSE, 150);
    data << uint32(entryToSend);
    size_t pos = data.wpos();
    data << uint32(0);

    if (info)
    {
        std::string Name;
        std::string IconName;
        std::string CastBarCaption;

        Name = info->name;
        IconName = info->IconName;
        CastBarCaption = info->castBarCaption;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (GameObjectLocale const* gl = sObjectMgr->GetGameObjectLocale(entry))
            {
                ObjectMgr::GetLocaleString(gl->Name, loc_idx, Name);
                ObjectMgr::GetLocaleString(gl->CastBarCaption, loc_idx, CastBarCaption);
            }
        }

        data << uint32(info->type);
        data << uint32(info->displayId);
        data << Name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4
        data << IconName;                                     // 2.0.3, string. Icon name to use instead of default icon for go's (ex: "Attack" makes sword)
        data << CastBarCaption;                               // 2.0.3, string. Text will appear in Cast Bar when using GO (ex: "Collecting")
        data << info->unk1;                                 // 2.0.3, string

        data.append(info->raw.data, MAX_GAMEOBJECT_DATA);
        data << float(info->size);                            // go size

        uint8 questItemCount = 0;
        for (uint32 i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
            if (info->questItems[i])
                questItemCount++;

        data << uint8(questItemCount);

        for (int i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS && questItemCount > 0; i++)
        {
            if (info->questItems[i])
            {
                data << uint32(info->questItems[i]);          // itemId[6], quest drop
                questItemCount--;
            }
        }

        data << uint32(info->unkInt32);                       // 4.x, unknown
        data.put(pos, uint32(data.wpos() - (pos + 4)));
    }
    else
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_GAMEOBJECT_QUERY - Missing gameobject info for (GUID: %u, ENTRY: %u)",
            GUID_LOPART(guid), entry);
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE");
    }

    data.WriteBit(info != NULL);
    data.FlushBits();
    SendPacket(&data);
}

void WorldSession::HandleCorpseQueryOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_CORPSE_QUERY");

    Corpse* corpse = GetPlayer()->GetCorpse();

    if (!corpse)
    {
        WorldPacket data(SMSG_CORPSE_QUERY, 5 * 4 + 2);
        data.WriteBits(0, 9); // Not found + guid stream
        for (int i = 0; i < 5; i++)
            data << uint32(0);
        SendPacket(&data);
        return;
    }

    uint32 mapid = corpse->GetMapId();
    float x = corpse->GetPositionX();
    float y = corpse->GetPositionY();
    float z = corpse->GetPositionZ();
    uint32 corpsemapid = mapid;

    // if corpse at different map
    if (mapid != _player->GetMapId())
    {
        // search entrance map for proper show entrance
        if (MapEntry const* corpseMapEntry = sMapStore.LookupEntry(mapid))
        {
            if (corpseMapEntry->IsDungeon() && corpseMapEntry->entrance_map >= 0)
            {
                // if corpse map have entrance
                if (Map const* entranceMap = sMapMgr->CreateBaseMap(corpseMapEntry->entrance_map))
                {
                    mapid = corpseMapEntry->entrance_map;
                    x = corpseMapEntry->entrance_x;
                    y = corpseMapEntry->entrance_y;
                    z = entranceMap->GetHeight(GetPlayer()->GetPhaseMask(), x, y, MAX_HEIGHT);
                }
            }
        }
    }

    _player->SendCorpseReclaimDelay();

    ObjectGuid guid = corpse->GetGUID();

    WorldPacket data(SMSG_CORPSE_QUERY, 4 + 4 + 4 + 4 + 4 + 1 + 1 + 8);
    
    data.WriteBit(guid[4]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[0]);
    data.WriteBit(1); // Corpse Found
    data.WriteBit(guid[7]);

    data.FlushBits();

    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[1]);
    data << uint32(mapid);
    data << float(x);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[5]);
    data << uint32(corpsemapid); // Corpse Map Id
    data.WriteByteSeq(guid[7]);
    data << float(z);
    data.WriteByteSeq(guid[0]);
    data << float(y);

    SendPacket(&data);
}

void WorldSession::HandleCemeteryListOpcode(WorldPacket& /*recvData*/)
{
    GetPlayer()->SendCemeteryList(false);
}

void WorldSession::HandleForcedReactionsOpcode(WorldPacket& /*recvData*/)
{
    GetPlayer()->GetReputationMgr().SendForceReactions();
}

void WorldSession::HandleNpcTextQueryOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 textID;
    bool hasGossip;

    recvData >> textID;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_NPC_TEXT_QUERY ID '%u'", textID);

    uint8 bitOrder[8] = {0, 1, 2, 6, 4, 3, 7, 5};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {3, 1, 4, 6, 2, 0, 5, 7};
    recvData.ReadBytesSeq(guid, byteOrder);

    GetPlayer()->SetSelection(guid);

    GossipText const* gossip = sObjectMgr->GetGossipText(textID);

    if (Unit* interactionUnit = ObjectAccessor::FindUnit(uint64(guid)))
    {
        if (interactionUnit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
            hasGossip = true;
        else
            hasGossip = false;
    }
    else if (GameObject* go = GetPlayer()->GetMap()->GetGameObject(uint64(guid)))
    {
        if (gossip)
            hasGossip = true;
    }
    else
	    hasGossip = false;

    WorldPacket data(SMSG_NPC_TEXT_UPDATE, 4 + 32 + 32 + 4 + 1);

    data << uint32(64);                                 // size: (MAX_GOSSIP_TEXT_OPTIONS(8) * 4) * 2. Common value seems to be 64.

    for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; i++)
        data << float(gossip ? gossip->Options[i].Probability : 0);

    data << uint32(textID);                              // Send the Text Id as first broadcast id. This is the gossip textID the creature updates to.

    for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS - 1; i++)
        data << uint32(0);                               // Broadcast Text Id for all other slots.

    data << uint32(textID);                              // This is the gossip textID and first to show when speaking to something.

    data.WriteBit(hasGossip);                            // Has gossip data - controls gossip window opening.
    data.FlushBits();

    SendPacket(&data);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_NPC_TEXT_UPDATE");
}

#define DEFAULT_GREETINGS_GOSSIP      68

void WorldSession::SendBroadcastTextDb2Reply(uint32 entry)
{
    ByteBuffer buff;
    WorldPacket data(SMSG_DB_REPLY);

    GossipText const* pGossip = sObjectMgr->GetGossipText(entry);

    if (!pGossip)
        pGossip = sObjectMgr->GetGossipText(DEFAULT_GREETINGS_GOSSIP);

    std::string text = "Greetings, $N";

    uint16 size1 = pGossip ? pGossip->Options[0].Text_0.length() : text.length();
    uint16 size2 = pGossip ? pGossip->Options[0].Text_1.length() : text.length();

    buff << uint32(entry);
    buff << uint32(0); // unk
    buff << uint16(size1);
    if (size1)
        buff << std::string( pGossip ? pGossip->Options[0].Text_0 : text);
    buff << uint16(size2);
    if (size2)
        buff << std::string(pGossip ? pGossip->Options[0].Text_1 : text);

    buff << uint32(0);
    buff << uint32(0);
    buff << uint32(0);
    buff << uint32(0);
    buff << uint32(0);
    buff << uint32(0);

    buff << uint32(0); // sound Id
    buff << uint32(pGossip ? pGossip->Options[0].Emotes[0]._Delay : 0); // Delay
    buff << uint32(pGossip ? pGossip->Options[0].Emotes[0]._Emote : 0); // Emote

    data << uint32(entry);
    data << uint32(DB2_REPLY_BROADCAST_TEXT);
    data << uint32(sObjectMgr->GetHotfixDate(entry, DB2_REPLY_BROADCAST_TEXT));
    data << uint32(buff.size());
    data.append(buff);

    SendPacket(&data);
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandlePageTextQueryOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_PAGE_TEXT_QUERY");

    ObjectGuid objectGuid;
    uint32 pageID;

    recvData >> pageID;

    uint8 bitsOrder[8] = { 1, 5, 2, 3, 6, 4, 0, 7 };
    recvData.ReadBitInOrder(objectGuid, bitsOrder);

    recvData.FlushBits();

    uint8 bytesOrder[8] = { 6, 4, 0, 3, 7, 5, 2, 1 };
    recvData.ReadBytesSeq(objectGuid, bytesOrder);

    /*if (IS_UNIT_GUID(objectGuid))
    {
        if (Unit* unit = Unit::GetUnit(*(GetPlayer()), objectGuid))
            sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Received CMSG_PAGE_TEXT_QUERY. Unit Entry: %u", unit->GetEntry());
    }
    else if (IS_GAMEOBJECT_GUID(objectGuid))
    {
        if (GameObject* go = GetPlayer()->GetMap()->GetGameObject(objectGuid))
            sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Received CMSG_PAGE_TEXT_QUERY. Gameobject Entry: %u", go->GetEntry());
    }*/

    WorldPacket data(SMSG_PAGE_TEXT_QUERY_RESPONSE, 50);
    while (pageID)
    {
        PageText const* pageText = sObjectMgr->GetPageText(pageID);

        data << uint32(pageID);
        data.WriteBit(pageText != NULL);

        if (pageText)
        {
            std::string Text = pageText->Text;

            int loc_idx = GetSessionDbLocaleIndex();
            if (loc_idx >= 0)
                if (PageTextLocale const* player = sObjectMgr->GetPageTextLocale(pageID))
                    ObjectMgr::GetLocaleString(player->Text, loc_idx, Text);

            data.WriteBits(Text.size(), 12);

            data.FlushBits();

            data << uint32(pageID);
            data.WriteString(Text);
            data << uint32(pageText->NextPage);
        }
        else
            data.FlushBits();

        pageID = pageText ? pageText->NextPage : 0;

        SendPacket(&data);

        data.clear();

        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_PAGE_TEXT_QUERY_RESPONSE");
    }
}

void WorldSession::HandleCorpseMapPositionQuery(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recv CMSG_CORPSE_MAP_POSITION_QUERY");

    ObjectGuid TransportGuid;
    
    TransportGuid[6] = recvData.ReadBit();
    TransportGuid[1] = recvData.ReadBit();
    TransportGuid[7] = recvData.ReadBit();
    TransportGuid[2] = recvData.ReadBit();
    TransportGuid[4] = recvData.ReadBit();
    TransportGuid[0] = recvData.ReadBit();
    TransportGuid[5] = recvData.ReadBit();
    TransportGuid[3] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(TransportGuid[5]);
    recvData.ReadByteSeq(TransportGuid[2]);
    recvData.ReadByteSeq(TransportGuid[3]);
    recvData.ReadByteSeq(TransportGuid[6]);
    recvData.ReadByteSeq(TransportGuid[1]);
    recvData.ReadByteSeq(TransportGuid[0]);
    recvData.ReadByteSeq(TransportGuid[7]);
    recvData.ReadByteSeq(TransportGuid[4]);

    // Positions X, Y, Z & Orientation. Order?
    WorldPacket data(SMSG_CORPSE_MAP_POSITION_QUERY_RESPONSE, 4 + 4 + 4 + 4);
    data << float(0);
    data << float(0);
    data << float(0);
    data << float(0);

    SendPacket(&data);
}

void WorldSession::HandleQuestPOIQuery(WorldPacket& recvData)
{
    // The CMSG sends the number of quests which are watched (you may have 10 quests and only watch 2), and the quest order in the tracklog, for which we store the player slot.
    // The SMSG responds with the same count, and the data for each quest watched, using the questId retrieved from the player slot stored.

    uint32 WatchedQuests = 0;
    std::list<uint16> WatchedQuestsSlotList;

    // Get the query data and store what is needed (CMSG sends 50 entries, but they are not valid, weird shit. Store the player slots for the valid tracked quests to use in SMSG).
    for (uint8 count = 0; count < MAX_QUEST_LOG_SIZE; count++)
        if (uint32 questId = recvData.read<uint32>())
            if (uint16 questSlot = _player->FindQuestSlot(questId))
                if (questId && questSlot && questSlot < MAX_QUEST_LOG_SIZE)
                    WatchedQuestsSlotList.push_back(questSlot);

    recvData >> WatchedQuests;

    // Resize to max. MAX_QUEST_LOG_SIZE (50 quests) if over - should never happen as player can't have so many quests.
    if (WatchedQuests > MAX_QUEST_LOG_SIZE)
        WatchedQuests = MAX_QUEST_LOG_SIZE;

    WatchedQuestsSlotList.resize(WatchedQuests);

    // Send the response.
    WorldPacket data(SMSG_QUEST_POI_QUERY_RESPONSE);

    data.WriteBits(WatchedQuests, 20);

    if (!WatchedQuestsSlotList.empty())
    {
        for (std::list<uint16>::iterator itr = WatchedQuestsSlotList.begin(); itr != WatchedQuestsSlotList.end(); itr++)
        {
            uint16 watchedQuestSlot = (*itr);
            uint32 questID = _player->GetQuestSlotQuestId(watchedQuestSlot);

            QuestPOIVector const* POI = sObjectMgr->GetQuestPOIVector(questID);

            if (POI)
            {
                data.WriteBits(POI->size(), 18);             // POI size
        
                for (QuestPOIVector::const_iterator itr = POI->begin(); itr != POI->end(); itr++)
                    data.WriteBits(itr->points.size(), 21);  // POI points size
            }
            else
                data.WriteBits(0, 18);
        }
    }

    data.FlushBits();

    if (!WatchedQuestsSlotList.empty())
    {
        for (std::list<uint16>::iterator itr = WatchedQuestsSlotList.begin(); itr != WatchedQuestsSlotList.end(); itr++)
        {
            uint16 watchedQuestSlot = (*itr);
            uint32 questID = _player->GetQuestSlotQuestId(watchedQuestSlot);

            QuestPOIVector const* POI = sObjectMgr->GetQuestPOIVector(questID);

            if (POI)
            {
                for (QuestPOIVector::const_iterator itr = POI->begin(); itr != POI->end(); itr++)
                {
                    data << uint32(itr->Unk4);              // Unk 4 (Ok).
                    data << uint32(0);                      // another unk
                    data << uint32(0);                      // another unk
                    data << uint32(0);                      // another unk - high numbers, 269151 ex., somehow ordered. Could be blobId, some repeat.
        
                    for (std::vector<QuestPOIPoint>::const_iterator itr2 = itr->points.begin(); itr2 != itr->points.end(); itr2++)
                    {
                        data << int32(itr2->x);             // POI point x
                        data << int32(itr2->y);             // POI point y
                    }

                    data << uint32(itr->MapId);             // MapId
                    data << uint32(itr->FloorId);           // FloorId
                    data << uint32(itr->Unk3);              // Unk 3 (Ok).

                    data << uint32(itr->points.size());     // POI points count
                    data << uint32(itr->AreaId);            // AreaId
                    data << uint32(itr->Id);                // POI index
                    data << int32(itr->ObjectiveIndex);     // objective index
                }
        
                data << uint32(POI->size());                // POI count
                data << uint32(questID);                    // quest ID
            }
            else
            {
                data << uint32(0); // POI count
                data << uint32(questID); // quest ID
            }
        }
    }

    data << uint32(WatchedQuests);

    SendPacket(&data);
}
