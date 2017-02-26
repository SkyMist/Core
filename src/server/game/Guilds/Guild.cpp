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

#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildFinderMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"
#include "SocialMgr.h"
#include "Log.h"
#include "AccountMgr.h"

#define MAX_GUILD_BANK_TAB_TEXT_LEN 500
#define EMBLEM_PRICE 10 * GOLD

inline uint32 _GetGuildBankTabPrice(uint8 tabId)
{
    switch (tabId)
    {
        case 0: return 100;
        case 1: return 250;
        case 2: return 500;
        case 3: return 1000;
        case 4: return 2500;
        case 5: return 5000;
        default: return 0;
    }
}

void Guild::SendCommandResult(WorldSession* session, GuildCommandType type, GuildCommandError errCode, const std::string& param)
{
    WorldPacket data(SMSG_GUILD_COMMAND_RESULT, 8 + param.size() + 1);

    data << uint32(errCode);
    data << uint32(type);

    data.WriteBits(param.size(), 8);

    data.FlushBits();

    if (param.size() > 0)
        data.append(param.c_str(), param.size());

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_COMMAND_RESULT)");
}

void Guild::SendSaveEmblemResult(WorldSession* session, GuildEmblemError errCode)
{
    WorldPacket data(SMSG_PLAYER_SAVE_GUILD_EMBLEM, 4);
    data << uint32(errCode);
    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_PLAYER_SAVE_GUILD_EMBLEM)");
}

// LogHolder
Guild::LogHolder::~LogHolder()
{
    // Cleanup
    for (GuildLog::iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        delete (*itr);
}

// Adds event loaded from database to collection
inline void Guild::LogHolder::LoadEvent(LogEntry* entry)
{
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = entry->GetGUID();
    m_log.push_front(entry);
}

// Adds new event happened in game.
// If maximum number of events is reached, oldest event is removed from collection.
inline void Guild::LogHolder::AddEvent(SQLTransaction& trans, LogEntry* entry)
{
    // Check max records limit
    if (m_log.size() >= m_maxRecords)
    {
        LogEntry* oldEntry = m_log.front();
        delete oldEntry;
        m_log.pop_front();
    }
    // Add event to list
    m_log.push_back(entry);
    // Save to DB
    entry->SaveToDB(trans);
}

// Writes information about all events into packet.
inline void Guild::LogHolder::WritePacket(WorldPacket& data, bool hasCashFlow /* = false */, bool isBankLog /* = true */) const
{
    ByteBuffer buffer;
    data.WriteBits(m_log.size(), 21);
    for (GuildLog::const_iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        (*itr)->WritePacket(data, buffer);

    if (isBankLog)
        data.WriteBit(hasCashFlow);

    data.FlushBits();

    data.append(buffer);
}

inline uint32 Guild::LogHolder::GetNextGUID()
{
    // Next guid was not initialized. It means there are no records for this holder in DB yet.
    // Start from the beginning.
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = 0;
    else
        m_nextGUID = (m_nextGUID + 1) % m_maxRecords;
    return m_nextGUID;
}

///////////////////////////////////////////////////////////////////////////////
// EventLogEntry
void Guild::EventLogEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_EVENTLOG);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt32(1, m_guid);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    uint8 index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, uint8(m_eventType));
    stmt->setUInt32(++index, m_playerGuid1);
    stmt->setUInt32(++index, m_playerGuid2);
    stmt->setUInt8 (++index, m_newRank);
    stmt->setUInt64(++index, m_timestamp);

    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::EventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content, bool hasCashFlow /* = false */) const
{
    ObjectGuid guid1 = MAKE_NEW_GUID(m_playerGuid1, 0, HIGHGUID_PLAYER);
    ObjectGuid guid2 = MAKE_NEW_GUID(m_playerGuid2, 0, HIGHGUID_PLAYER);

    data.WriteBit(guid2[2]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid1[2]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[5]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid2[3]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid2[4]);

    content.WriteByteSeq(guid1[7]);
    content.WriteByteSeq(guid2[0]);
    content.WriteByteSeq(guid1[1]);
    content.WriteByteSeq(guid1[4]);
    content.WriteByteSeq(guid2[3]);
    content.WriteByteSeq(guid1[3]);
    content.WriteByteSeq(guid2[6]);
    content.WriteByteSeq(guid2[4]);
    content.WriteByteSeq(guid1[0]);
    content.WriteByteSeq(guid2[7]);

    // Event timestamp
    content << uint32(::time(NULL) - m_timestamp);

    content.WriteByteSeq(guid1[2]);

    // New Rank
    content << uint8(m_newRank);

    content.WriteByteSeq(guid1[6]);
    content.WriteByteSeq(guid1[5]);
    content.WriteByteSeq(guid2[1]);

    // Event type
    content << uint8(m_eventType);

    content.WriteByteSeq(guid2[5]);
    content.WriteByteSeq(guid2[2]);
}

///////////////////////////////////////////////////////////////////////////////
// BankEventLogEntry
void Guild::BankEventLogEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = NULL;
    uint8 index = 0;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, m_bankTabId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_BANK_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, m_bankTabId);
    stmt->setUInt8 (++index, uint8(m_eventType));
    stmt->setUInt32(++index, m_playerGuid);
    stmt->setUInt32(++index, m_itemOrMoney);
    stmt->setUInt16(++index, m_itemStackCount);
    stmt->setUInt8 (++index, m_destTabId);
    stmt->setUInt64(++index, m_timestamp);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::BankEventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content, bool hasCashFlow /* = false */) const
{
    ObjectGuid logGuid = MAKE_NEW_GUID(m_playerGuid, 0, HIGHGUID_PLAYER);
    
    bool hasItem = m_eventType == GUILD_BANK_LOG_DEPOSIT_ITEM || m_eventType == GUILD_BANK_LOG_WITHDRAW_ITEM ||
                   m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2;

    bool itemMoved = (m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2);

    bool hasStack = (hasItem && m_itemStackCount > 1);

    data.WriteBit(hasStack);

    data.WriteBit(logGuid[0]);
    data.WriteBit(logGuid[7]);
    data.WriteBit(logGuid[5]);

    data.WriteBit(hasItem);

    data.WriteBit(logGuid[1]);
    data.WriteBit(logGuid[6]);
    data.WriteBit(logGuid[4]);
    data.WriteBit(logGuid[2]);

    data.WriteBit(IsMoneyEvent());

    data.WriteBit(logGuid[3]);

    data.WriteBit(itemMoved);

    content.WriteByteSeq(logGuid[5]);
    content.WriteByteSeq(logGuid[7]);

    if (IsMoneyEvent())
        content << uint64(m_itemOrMoney);

    content << uint32(time(NULL) - m_timestamp);

    content.WriteByteSeq(logGuid[2]);
    content.WriteByteSeq(logGuid[3]);

    if (hasStack)
        content << uint32(m_itemStackCount);
    if (hasItem)
        content << uint32(m_itemOrMoney);

    content.WriteByteSeq(logGuid[1]);
    content.WriteByteSeq(logGuid[4]);
    content.WriteByteSeq(logGuid[6]);
    content.WriteByteSeq(logGuid[0]);

    content << uint8(m_eventType);

    if (itemMoved)
        content << uint8(m_destTabId);
}

///////////////////////////////////////////////////////////////////////////////
// RankInfo
void Guild::RankInfo::LoadFromDB(Field* fields)
{
    m_rankId            = fields[1].GetUInt8();
    m_name              = fields[2].GetString();
    m_rights            = fields[3].GetUInt32();
    m_bankMoneyPerDay   = fields[4].GetUInt32();

    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        m_rights |= GR_RIGHT_ALL;
}

void Guild::RankInfo::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_RANK);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_rankId);
    stmt->setString(2, m_name);
    stmt->setUInt32(3, m_rights);
    stmt->setUInt32(4, m_bankMoneyPerDay);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::RankInfo::UpdateId(uint32 newId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_MEMBERS_RANK);
    stmt->setUInt8 (0, newId);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt8 (2, m_rankId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_ID);
    stmt->setUInt8 (0, newId);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt8 (2, m_rankId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_BANK_RIGHTS_ID);
    stmt->setUInt8 (0, newId);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt8 (2, m_rankId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    CharacterDatabase.CommitTransaction(trans);

    SetId(newId);
}

void Guild::RankInfo::SetName(const std::string& name)
{
    if (m_name == name)
        return;

    m_name = name;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_NAME);
    stmt->setString(0, m_name);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetRights(uint32 rights)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rights = GR_RIGHT_ALL;

    if (m_rights == rights)
        return;

    m_rights = rights;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_RIGHTS);
    stmt->setUInt32(0, m_rights);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetBankMoneyPerDay(uint32 money)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        money = uint32(GUILD_WITHDRAW_MONEY_UNLIMITED);

    if (m_bankMoneyPerDay == money)
        return;

    m_bankMoneyPerDay = money;

    PreparedStatement* stmt = NULL;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_BANK_MONEY);
    stmt->setUInt32(0, money);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_BANK_RESET_TIME);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_rankId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetBankTabSlotsAndRights(uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rightsAndSlots.SetGuildMasterValues();

    if (m_bankTabRightsAndSlots[tabId].IsEqual(rightsAndSlots))
        return;

    m_bankTabRightsAndSlots[tabId] = rightsAndSlots;

    if (saveToDB)
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        PreparedStatement* stmt = NULL;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHT);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, tabId);
        stmt->setUInt8 (2, m_rankId);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_BANK_RIGHT);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, tabId);
        stmt->setUInt8 (2, m_rankId);
        stmt->setUInt8 (3, m_bankTabRightsAndSlots[tabId].rights);
        stmt->setUInt32(4, m_bankTabRightsAndSlots[tabId].slots);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_RANK_BANK_TIME0 + tabId);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_rankId);
        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);
    }
}

///////////////////////////////////////////////////////////////////////////////
// BankTab
bool Guild::BankTab::LoadFromDB(Field* fields)
{
    m_name = fields[2].GetString();
    m_icon = fields[3].GetString();
    m_text = fields[4].GetString();
    return true;
}

bool Guild::BankTab::LoadItemFromDB(Field* fields)
{
    uint8 slotId = fields[16].GetUInt8();
    uint32 itemGuid = fields[17].GetUInt32();
    uint32 itemEntry = fields[18].GetUInt32();

    if (slotId >= GUILD_BANK_MAX_SLOTS)
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid slot for item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto)
    {
        sLog->outError(LOG_FILTER_GUILD, "Unknown item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    Item* pItem = NewItemOrBag(proto);
    if (!pItem->LoadFromDB(itemGuid, 0, fields, itemEntry))
    {
        sLog->outError(LOG_FILTER_GUILD, "Item (GUID %u, id: %u) not found in item_instance, deleting from guild bank!", itemGuid, itemEntry);

        PreparedStatement *stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NONEXISTENT_GUILD_BANK_ITEM);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_tabId);
        stmt->setUInt8 (2, slotId);
        CharacterDatabase.Execute(stmt);

        delete pItem;
        return false;
    }

    pItem->AddToWorld();
    m_items[slotId] = pItem;
    return true;
}

// Deletes contents of the tab from the world (and from DB if necessary)
void Guild::BankTab::Delete(SQLTransaction& trans, bool removeItemsFromDB)
{
    for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
        if (Item* pItem = m_items[slotId])
        {
            pItem->RemoveFromWorld();
            if (removeItemsFromDB)
                pItem->DeleteFromDB(trans);
            delete pItem;
            pItem = NULL;
        }
}

void Guild::BankTab::SetInfo(const std::string& name, const std::string& icon)
{
    if (m_name == name && m_icon == icon)
        return;

    m_name = name;
    m_icon = icon;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_BANK_TAB_INFO);
    stmt->setString(0, m_name);
    stmt->setString(1, m_icon);
    stmt->setUInt32(2, m_guildId);
    stmt->setUInt8 (3, m_tabId);
    CharacterDatabase.Execute(stmt);
}

void Guild::BankTab::SetText(const std::string& text)
{
    if (m_text == text)
        return;

    m_text = text;
    utf8truncate(m_text, MAX_GUILD_BANK_TAB_TEXT_LEN);          // DB and client size limitation

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_BANK_TAB_TEXT);
    stmt->setString(0, m_text);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt8 (2, m_tabId);
    CharacterDatabase.Execute(stmt);
}

// Sets/removes contents of specified slot.
// If pItem == NULL contents are removed.
bool Guild::BankTab::SetItem(SQLTransaction& trans, uint8 slotId, Item* item)
{
    if (slotId >= GUILD_BANK_MAX_SLOTS)
        return false;

    m_items[slotId] = item;

    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_ITEM);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_tabId);
    stmt->setUInt8 (2, slotId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    if (item)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_BANK_ITEM);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_tabId);
        stmt->setUInt8 (2, slotId);
        stmt->setUInt32(3, item->GetGUIDLow());
        CharacterDatabase.ExecuteOrAppend(trans, stmt);

        item->SetUInt64Value(ITEM_FIELD_CONTAINED, 0);
        item->SetUInt64Value(ITEM_FIELD_OWNER, 0);
        item->FSetState(ITEM_NEW);
        item->SaveToDB(trans);                                 // Not in inventory and can be saved standalone
    }
    return true;
}

void Guild::BankTab::SendText(Guild const* guild, WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_BANK_QUERY_TEXT_RESULT);

    data.WriteBits(m_text.size(), 14);
    data.FlushBits();

    if (m_text.size() > 0)
        data.append(m_text.c_str(), m_text.size());

    data << uint32(m_tabId);

    if (session)
        session->SendPacket(&data);
    else
        guild->BroadcastPacket(&data);
}

///////////////////////////////////////////////////////////////////////////////
// Member
void Guild::Member::SetStats(Player* player)
{
    m_name      = player->GetName();
    m_level     = player->getLevel();
    m_class     = player->getClass();
    m_zoneId    = player->GetZoneId();
    m_accountId = player->GetSession()->GetAccountId();
}

void Guild::Member::SetStats(const std::string& name, uint8 level, uint8 _class, uint32 zoneId, uint32 accountId)
{
    m_name      = name;
    m_level     = level;
    m_class     = _class;
    m_zoneId    = zoneId;
    m_accountId = accountId;
}

void Guild::Member::SetPublicNote(const std::string& publicNote)
{
    if (m_publicNote == publicNote)
        return;

    m_publicNote = publicNote;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_MEMBER_PNOTE);
    stmt->setString(0, publicNote);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::Member::SetOfficerNote(const std::string& officerNote)
{
    if (m_officerNote == officerNote)
        return;

    m_officerNote = officerNote;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_MEMBER_OFFNOTE);
    stmt->setString(0, officerNote);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::Member::ChangeRank(uint8 newRank)
{
    m_rankId = newRank;

    // Update rank information in player's field, if he is online.
    if (Player* player = FindPlayer())
        player->SetRank(newRank);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_MEMBER_RANK);
    stmt->setUInt8 (0, newRank);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::Member::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_MEMBER);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    stmt->setUInt8 (2, m_rankId);
    stmt->setString(3, m_publicNote);
    stmt->setString(4, m_officerNote);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

// Loads member's data from database.
// If member has broken fields (level, class) returns false.
// In this case member has to be removed from guild.
bool Guild::Member::LoadFromDB(Field* fields)
{
    m_publicNote    = fields[3].GetString();
    m_officerNote   = fields[4].GetString();
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime  = fields[5].GetUInt32();
    m_bankRemaining[GUILD_BANK_MAX_TABS].value      = fields[6].GetUInt32();
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; ++i)
    {
        m_bankRemaining[i].resetTime                = fields[7 + i * 2].GetUInt32();
        m_bankRemaining[i].value                    = fields[8 + i * 2].GetUInt32();
    }

    SetStats(fields[23].GetString(),
             fields[24].GetUInt8(),                         // characters.level
             fields[25].GetUInt8(),                         // characters.class
             fields[26].GetUInt16(),                        // characters.zone
             fields[27].GetUInt32());                       // characters.account
    m_logoutTime    = fields[28].GetUInt32();               // characters.logout_time

    if (!CheckStats())
        return false;

    if (!m_zoneId)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has broken zone-data", GUID_LOPART(m_guid));
        m_zoneId = Player::GetZoneIdFromDB(m_guid);
    }
    return true;
}

// Validate player fields. Returns false if corrupted fields are found.
bool Guild::Member::CheckStats() const
{
    if (m_level < 1)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has a broken data in field `characters`.`level`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    if (m_class < CLASS_WARRIOR || m_class >= MAX_CLASSES)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has a broken data in field `characters`.`class`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    return true;
}

// Decreases amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) decrease money amount.
// Otherwise decrease remaining items amount for specified tab.
void Guild::Member::DecreaseBankRemainingValue(SQLTransaction& trans, uint8 tabId, uint32 amount)
{
    m_bankRemaining[tabId].value -= amount;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(
        tabId == GUILD_BANK_MAX_TABS ?
        CHAR_UPD_GUILD_MEMBER_BANK_REM_MONEY :
        CHAR_UPD_GUILD_MEMBER_BANK_REM_SLOTS0 + tabId);
    stmt->setUInt32(0, m_bankRemaining[tabId].value);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt32(2, GUID_LOPART(m_guid));
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

// Get amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) return money amount.
// Otherwise return remaining items amount for specified tab.
// If reset time was more than 24 hours ago, renew reset time and reset amount to maximum value.
uint32 Guild::Member::GetBankRemainingValue(uint8 tabId, const Guild* guild) const
{
    // Guild master has unlimited amount.
    if (IsRank(GR_GUILDMASTER))
        return tabId == GUILD_BANK_MAX_TABS ? GUILD_WITHDRAW_MONEY_UNLIMITED : GUILD_WITHDRAW_SLOT_UNLIMITED;

    // Check rights for non-money tab.
    if (tabId != GUILD_BANK_MAX_TABS)
        if ((guild->_GetRankBankTabRights(m_rankId, tabId) & GUILD_BANK_RIGHT_VIEW_TAB) != GUILD_BANK_RIGHT_VIEW_TAB)
            return 0;

    uint32 curTime = uint32(::time(NULL) / MINUTE); // minutes
    if (curTime > m_bankRemaining[tabId].resetTime + 24 * HOUR / MINUTE)
    {
        RemainingValue& rv = const_cast <RemainingValue&> (m_bankRemaining[tabId]);
        rv.resetTime = curTime;
        rv.value = tabId == GUILD_BANK_MAX_TABS ?
            guild->_GetRankBankMoneyPerDay(m_rankId) :
            guild->_GetRankBankTabSlotsPerDay(m_rankId, tabId);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(
            tabId == GUILD_BANK_MAX_TABS ?
            CHAR_UPD_GUILD_MEMBER_BANK_TIME_MONEY :
            CHAR_UPD_GUILD_MEMBER_BANK_TIME_REM_SLOTS0 + tabId);
        stmt->setUInt32(0, m_bankRemaining[tabId].resetTime);
        stmt->setUInt32(1, m_bankRemaining[tabId].value);
        stmt->setUInt32(2, m_guildId);
        stmt->setUInt32(3, GUID_LOPART(m_guid));
        CharacterDatabase.Execute(stmt);
    }
    return m_bankRemaining[tabId].value;
}

inline void Guild::Member::ResetTabTimes()
{
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
        m_bankRemaining[tabId].resetTime = 0;
}

inline void Guild::Member::ResetMoneyTime()
{
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime = 0;
}

void Guild::Member::GiveReputation(uint32 value)
{
    m_totalReputation += value;
    m_weekReputation += value;
}

///////////////////////////////////////////////////////////////////////////////
// EmblemInfo
void EmblemInfo::LoadFromDB(Field* fields)
{
    m_style             = fields[3].GetUInt8();
    m_color             = fields[4].GetUInt8();
    m_borderStyle       = fields[5].GetUInt8();
    m_borderColor       = fields[6].GetUInt8();
    m_backgroundColor   = fields[7].GetUInt8();
}

void EmblemInfo::WritePacket(WorldPacket& data) const
{
    data << uint32(m_style);
    data << uint32(m_color);
    data << uint32(m_borderStyle);
    data << uint32(m_borderColor);
    data << uint32(m_backgroundColor);
}

void EmblemInfo::SaveToDB(uint32 guildId) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_EMBLEM_INFO);
    stmt->setUInt32(0, m_style);
    stmt->setUInt32(1, m_color);
    stmt->setUInt32(2, m_borderStyle);
    stmt->setUInt32(3, m_borderColor);
    stmt->setUInt32(4, m_backgroundColor);
    stmt->setUInt32(5, guildId);
    CharacterDatabase.Execute(stmt);
}

///////////////////////////////////////////////////////////////////////////////
// MoveItemData
bool Guild::MoveItemData::CheckItem(uint32& splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount > m_pItem->GetCount())
        return false;
    if (splitedAmount == m_pItem->GetCount())
        splitedAmount = 0;
    return true;
}

bool Guild::MoveItemData::CanStore(Item* pItem, bool swap, bool sendError)
{
    m_vec.clear();
    InventoryResult msg = CanStore(pItem, swap);
    if (sendError && msg != EQUIP_ERR_OK)
        m_pPlayer->SendEquipError(msg, pItem);
    return (msg == EQUIP_ERR_OK);
}

bool Guild::MoveItemData::CloneItem(uint32 count)
{
    ASSERT(m_pItem);
    m_pClonedItem = m_pItem->CloneItem(count);
    if (!m_pClonedItem)
    {
        m_pPlayer->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, m_pItem);
        return false;
    }
    return true;
}

void Guild::MoveItemData::LogAction(MoveItemData* pFrom) const
{
    ASSERT(pFrom->GetItem());

    sScriptMgr->OnGuildItemMove(m_pGuild, m_pPlayer, pFrom->GetItem(),
        pFrom->IsBank(), pFrom->GetContainer(), pFrom->GetSlotId(),
        IsBank(), GetContainer(), GetSlotId());
}

inline void Guild::MoveItemData::CopySlots(SlotIds& ids) const
{
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); ++itr)
        ids.insert(uint8(itr->pos));
}

///////////////////////////////////////////////////////////////////////////////
// PlayerMoveItemData
bool Guild::PlayerMoveItemData::InitItem()
{
    m_pItem = m_pPlayer->GetItemByPos(m_container, m_slotId);
    if (m_pItem)
    {
        // Anti-WPE protection. Do not move non-empty bags to bank.
        if (m_pItem->IsNotEmptyBag())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_DESTROY_NONEMPTY_BAG, m_pItem);
            m_pItem = NULL;
        }
        // Bound items cannot be put into bank.
        else if (!m_pItem->CanBeTraded())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_CANT_SWAP, m_pItem);
            m_pItem = NULL;
        }
    }
    return (m_pItem != NULL);
}

void Guild::PlayerMoveItemData::RemoveItem(SQLTransaction& trans, MoveItemData* /*pOther*/, uint32 splitedAmount)
{
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->SetState(ITEM_CHANGED, m_pPlayer);
        m_pPlayer->SaveInventoryAndGoldToDB(trans);
    }
    else
    {
        m_pPlayer->MoveItemFromInventory(m_container, m_slotId, true);
        m_pItem->DeleteFromInventoryDB(trans);
        m_pItem = NULL;
    }
}

Item* Guild::PlayerMoveItemData::StoreItem(SQLTransaction& trans, Item* pItem)
{
    ASSERT(pItem);
    m_pPlayer->MoveItemToInventory(m_vec, pItem, true);
    m_pPlayer->SaveInventoryAndGoldToDB(trans);
    return pItem;
}

void Guild::PlayerMoveItemData::LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom);
    // Bank -> Char
    m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_WITHDRAW_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(),
        pFrom->GetItem()->GetEntry(), count);
}

inline InventoryResult Guild::PlayerMoveItemData::CanStore(Item* pItem, bool swap)
{
    return m_pPlayer->CanStoreItem(m_container, m_slotId, m_vec, pItem, swap);
}

///////////////////////////////////////////////////////////////////////////////
// BankMoveItemData
bool Guild::BankMoveItemData::InitItem()
{
    m_pItem = m_pGuild->_GetItem(m_container, m_slotId);
    return (m_pItem != NULL);
}

bool Guild::BankMoveItemData::HasStoreRights(MoveItemData* pOther) const
{
    ASSERT(pOther);

    // Do not check rights if item is being swapped within the same bank tab.
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;

    // If he has the rights all is ok, if not throw an error and return.
    if (m_pGuild->_MemberHasTabRights(m_pPlayer->GetGUID(), m_container, GUILD_BANK_RIGHT_DEPOSIT_ITEM))
        return true;
    else
        SendCommandResult(m_pPlayer->GetSession(), GUILD_COMMAND_MOVE_ITEM, ERR_GUILD_PERMISSIONS);

    return false;
}

bool Guild::BankMoveItemData::HasWithdrawRights(MoveItemData* pOther) const
{
    ASSERT(pOther);

    // Do not check rights if item is being swapped within the same bank tab.
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;

    // If he has the rights all is ok, if not throw an error and return.
    if (m_pGuild->_GetMemberRemainingSlots(m_pPlayer->GetGUID(), m_container) != 0)
        return true;
    else
        SendCommandResult(m_pPlayer->GetSession(), GUILD_COMMAND_MOVE_ITEM, ERR_GUILD_WITHDRAW_LIMIT);

    return false;
}

void Guild::BankMoveItemData::RemoveItem(SQLTransaction& trans, MoveItemData* pOther, uint32 splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->FSetState(ITEM_CHANGED);
        m_pItem->SaveToDB(trans);
    }
    else
    {
        m_pGuild->_RemoveItem(trans, m_container, m_slotId);
        m_pItem = NULL;
    }
    // Decrease amount of player's remaining items (if item is moved to different tab or to player)
    if (!pOther->IsBank() || pOther->GetContainer() != m_container)
        m_pGuild->_DecreaseMemberRemainingSlots(trans, m_pPlayer->GetGUID(), m_container);
}

Item* Guild::BankMoveItemData::StoreItem(SQLTransaction& trans, Item* pItem)
{
    if (!pItem)
        return NULL;

    BankTab* pTab = m_pGuild->GetBankTab(m_container);
    if (!pTab)
        return NULL;

    Item* pLastItem = pItem;
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); )
    {
        ItemPosCount pos(*itr);
        ++itr;

        sLog->outDebug(LOG_FILTER_GUILD, "GUILD STORAGE: StoreItem tab = %u, slot = %u, item = %u, count = %u",
            m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());
        pLastItem = _StoreItem(trans, pTab, pItem, pos, itr != m_vec.end());
    }
    return pLastItem;
}

void Guild::BankMoveItemData::LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom->GetItem());
    if (pFrom->IsBank())
        // Bank -> Bank
        m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_MOVE_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(),
            pFrom->GetItem()->GetEntry(), count, m_container);
    else
        // Char -> Bank
        m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_DEPOSIT_ITEM, m_container, m_pPlayer->GetGUIDLow(),
            pFrom->GetItem()->GetEntry(), count);
}

void Guild::BankMoveItemData::LogAction(MoveItemData* pFrom) const
{
    MoveItemData::LogAction(pFrom);
    if (!pFrom->IsBank() && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE) && !AccountMgr::IsPlayerAccount(m_pPlayer->GetSession()->GetSecurity()))       // TODO: move to scripts
        sLog->outCommand(m_pPlayer->GetSession()->GetAccountId(), "", m_pPlayer->GetGUIDLow(), m_pPlayer->GetName(), 0, "", 0, "",
                        "GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u)",
                        m_pPlayer->GetName(), m_pPlayer->GetSession()->GetAccountId(), pFrom->GetItem()->GetTemplate()->Name1.c_str(), pFrom->GetItem()->GetEntry(), pFrom->GetItem()->GetCount(), m_pGuild->GetId());
}

Item* Guild::BankMoveItemData::_StoreItem(SQLTransaction& trans, BankTab* pTab, Item* pItem, ItemPosCount& pos, bool clone) const
{
    uint8 slotId = uint8(pos.pos);
    uint32 count = pos.count;
    if (Item* pItemDest = pTab->GetItem(slotId))
    {
        pItemDest->SetCount(pItemDest->GetCount() + count);
        pItemDest->FSetState(ITEM_CHANGED);
        pItemDest->SaveToDB(trans);
        if (!clone)
        {
            pItem->RemoveFromWorld();
            pItem->DeleteFromDB(trans);
            delete pItem;
        }
        return pItemDest;
    }

    if (clone)
        pItem = pItem->CloneItem(count);
    else
        pItem->SetCount(count);

    if (pItem && pTab->SetItem(trans, slotId, pItem))
        return pItem;

    return NULL;
}

// Tries to reserve space for source item.
// If item in destination slot exists it must be the item of the same entry
// and stack must have enough space to take at least one item.
// Returns false if destination item specified and it cannot be used to reserve space.
bool Guild::BankMoveItemData::_ReserveSpace(uint8 slotId, Item* pItem, Item* pItemDest, uint32& count)
{
    uint32 requiredSpace = pItem->GetMaxStackCount();
    if (pItemDest)
    {
        // Make sure source and destination items match and destination item has space for more stacks.
        if (pItemDest->GetEntry() != pItem->GetEntry() || pItemDest->GetCount() >= pItem->GetMaxStackCount())
            return false;
        requiredSpace -= pItemDest->GetCount();
    }
    // Let's not be greedy, reserve only required space
    requiredSpace = std::min(requiredSpace, count);

    // Reserve space
    ItemPosCount pos(slotId, requiredSpace);
    if (!pos.isContainedIn(m_vec))
    {
        m_vec.push_back(pos);
        count -= requiredSpace;
    }
    return true;
}

void Guild::BankMoveItemData::CanStoreItemInTab(Item* pItem, uint8 skipSlotId, bool merge, uint32& count)
{
    for (uint8 slotId = 0; (slotId < GUILD_BANK_MAX_SLOTS) && (count > 0); ++slotId)
    {
        // Skip slot already processed in CanStore (when destination slot was specified)
        if (slotId == skipSlotId)
            continue;

        Item* pItemDest = m_pGuild->_GetItem(m_container, slotId);
        if (pItemDest == pItem)
            pItemDest = NULL;

        // If merge skip empty, if not merge skip non-empty
        if ((pItemDest != NULL) != merge)
            continue;

        _ReserveSpace(slotId, pItem, pItemDest, count);
    }
}

InventoryResult Guild::BankMoveItemData::CanStore(Item* pItem, bool swap)
{
    sLog->outDebug(LOG_FILTER_GUILD, "GUILD STORAGE: CanStore() tab = %u, slot = %u, item = %u, count = %u",
        m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());

    uint32 count = pItem->GetCount();
    // Soulbound items cannot be moved
    if (pItem->IsSoulBound())
        return EQUIP_ERR_DROP_BOUND_ITEM;

    // Make sure destination bank tab exists
    if (m_container >= m_pGuild->GetPurchasedTabsSize())
        return EQUIP_ERR_WRONG_BAG_TYPE;

    // Slot explicitely specified. Check it.
    if (m_slotId != NULL_SLOT)
    {
        Item* pItemDest = m_pGuild->_GetItem(m_container, m_slotId);
        // Ignore swapped item (this slot will be empty after move)
        if ((pItemDest == pItem) || swap)
            pItemDest = NULL;

        if (!_ReserveSpace(m_slotId, pItem, pItemDest, count))
            return EQUIP_ERR_CANT_STACK;

        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Slot was not specified or it has not enough space for all the items in stack
    // Search for stacks to merge with
    if (pItem->GetMaxStackCount() > 1)
    {
        CanStoreItemInTab(pItem, m_slotId, true, count);
        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Search free slot for item
    CanStoreItemInTab(pItem, m_slotId, false, count);
    if (count == 0)
        return EQUIP_ERR_OK;

    return EQUIP_ERR_BANK_FULL;
}

///////////////////////////////////////////////////////////////////////////////
// Guild
Guild::Guild() : m_id(0), m_leaderGuid(0), m_createdDate(0), m_accountsNumber(0), m_bankMoney(0), m_eventLog(NULL),
    m_achievementMgr(this), _level(1), _experience(0), _todayExperience(0), _newsLog(this)
{
    memset(&m_bankEventLog, 0, (GUILD_BANK_MAX_TABS + 1) * sizeof(LogHolder*));
}

Guild::~Guild()
{
    SQLTransaction temp(NULL);
    _DeleteBankItems(temp);

    // Cleanup
    if (m_eventLog)
        delete m_eventLog;
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        if (m_bankEventLog[tabId])
            delete m_bankEventLog[tabId];
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        delete itr->second;
}

// Creates new guild with default data and saves it to database.
bool Guild::Create(Player* pLeader, const std::string& name)
{
    // Check if the player exists.
    WorldSession* pLeaderSession = pLeader->GetSession();
    if (!pLeaderSession)
        return false;

    // Check if a guild with that name already exists
    if (sGuildMgr->GetGuildByName(name))
    {
        SendCommandResult(pLeaderSession, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
        return false;
    }

    // ToDo: Check if the guild name is valid.
    // if (name -> IsValid)
    // {
    //     SendCommandResult(pLeaderSession, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
    //     return false;
    // }

    m_id = sGuildMgr->GenerateGuildId();
    m_leaderGuid = pLeader->GetGUID();
    m_name = name;
    m_info = "";
    m_motd = "No message set.";
    m_bankMoney = 0;
    m_createdDate = ::time(NULL);
    _level = 1;
    _CreateLogHolders();

    sLog->outDebug(LOG_FILTER_GUILD, "GUILD: creating guild [%s] for leader %s (%u)",
        name.c_str(), pLeader->GetName(), GUID_LOPART(m_leaderGuid));

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_MEMBERS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    uint8 index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD);
    stmt->setUInt32(  index, m_id);
    stmt->setString(++index, name);
    stmt->setUInt32(++index, GUID_LOPART(m_leaderGuid));
    stmt->setString(++index, m_info);
    stmt->setString(++index, m_motd);
    stmt->setUInt64(++index, uint32(m_createdDate));
    stmt->setUInt32(++index, m_emblemInfo.GetStyle());
    stmt->setUInt32(++index, m_emblemInfo.GetColor());
    stmt->setUInt32(++index, m_emblemInfo.GetBorderStyle());
    stmt->setUInt32(++index, m_emblemInfo.GetBorderColor());
    stmt->setUInt32(++index, m_emblemInfo.GetBackgroundColor());
    stmt->setUInt64(++index, m_bankMoney);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
    // Create default ranks
    _CreateDefaultGuildRanks(pLeaderSession->GetSessionDbLocaleIndex());
    // Add guildmaster
    bool ret = AddMember(m_leaderGuid, GR_GUILDMASTER);
    // Call scripts on successful create
    if (ret)
        sScriptMgr->OnGuildCreate(this, pLeader, name);

    WorldPacket data(SMSG_GUILD_SEND_MOTD, m_motd.size() + 2);

    data.WriteBits(m_motd.size(), 10);
    data.FlushBits();

    if (m_motd.size() > 0)
        data.append(m_motd.c_str(), m_motd.size());

    BroadcastPacket(&data);

    return ret;
}

// Disbands guild and deletes all related data from database
void Guild::Disband()
{
    // Call scripts before guild data removed from database
    sScriptMgr->OnGuildDisband(this);

    _BroadcastEvent(GE_DISBANDED, 0);
    // Remove all members
    while (!m_members.empty())
    {
        Members::const_iterator itr = m_members.begin();
        DeleteMember(itr->second->GetGUID(), true);
    }

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_RANKS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_TABS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    // Free bank tab used memory and delete items stored in them
    _DeleteBankItems(trans, true);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_ITEMS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_EVENTLOGS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_EVENTLOGS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    sGuildFinderMgr->DeleteGuild(m_id);

    sGuildMgr->RemoveGuild(m_id);
}

void Guild::SaveToDB()
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_EXPERIENCE);
    stmt->setUInt32(0, GetLevel());
    stmt->setUInt64(1, GetExperience());
    stmt->setUInt64(2, GetTodayExperience());
    stmt->setUInt32(3, GetId());
    trans->Append(stmt);

    m_achievementMgr.SaveToDB(trans);

    CharacterDatabase.CommitTransaction(trans);
}

///////////////////////////////////////////////////////////////////////////////
// HANDLE CLIENT COMMANDS

void Guild::UpdateMemberData(Player* player, uint8 dataid, uint32 value)
{
    if (Member* member = GetMember(player->GetGUID()))
    {
        switch (dataid)
        {
            case GUILD_MEMBER_DATA_ZONEID:
                member->SetZoneId(value);
                break;
            case GUILD_MEMBER_DATA_ACHIEVEMENT_POINTS:
                member->SetAchievementPoints(value);
                break;
            case GUILD_MEMBER_DATA_LEVEL:
                member->SetLevel(value);
                break;
            default:
                sLog->outError(LOG_FILTER_GUILD, "Guild::UpdateMemberData: Called with incorrect DATAID %u (value %u)", dataid, value);
                return;
        }
        //HandleRoster();
    }
}

bool Guild::SetName(std::string const& name)
{
    if (m_name == name || name.empty() || name.length() > 24 || sObjectMgr->IsReservedName(name) || !ObjectMgr::IsValidCharterName(name))
        return false;

    m_name = name;
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_NAME);
    stmt->setString(0, m_name);
    stmt->setUInt32(1, GetId());
    CharacterDatabase.Execute(stmt);
    return true;
}

void Guild::HandleRoster(WorldSession* session /*= NULL*/)
{
    WorldPacket data(SMSG_GUILD_ROSTER, 100);
    ByteBuffer memberData;

    data << uint32(0);
    data << uint32(m_accountsNumber);
    data << uint32(secsToTimeBitFields(m_createdDate));
    data << uint32(1/*sWorld->getIntConfig(CONFIG_GUILD_WEEKLY_REP_CAP)*/);

    data.WriteBits(m_motd.length(), 10);
    data.WriteBits(m_info.length(), 11);
    data.WriteBits(m_members.size(), 17);

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Member* member = itr->second;
        Player* player = member->FindPlayer();
        size_t pubNoteLength = member->GetPublicNote().length();
        size_t offNoteLength = member->GetOfficerNote().length();

        uint8 flags = GUILDMEMBER_STATUS_NONE;
        if (player)
        {
            flags |= GUILDMEMBER_STATUS_ONLINE;
            if (player->isAFK())
                flags |= GUILDMEMBER_STATUS_AFK;
            if (player->isDND())
                flags |= GUILDMEMBER_STATUS_DND;
        }

        ObjectGuid guid = member->GetGUID();

        data.WriteBit(0); // Can Scroll of Ressurect
        data.WriteBits(pubNoteLength, 8);
        data.WriteBit(0); // Has Authenticator
        data.WriteBit(guid[5]);
        data.WriteBit(guid[4]);
        data.WriteBits(member->GetName().length(), 6);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[7]);
        data.WriteBits(offNoteLength, 8);
        data.WriteBit(guid[1]);
        data.WriteBit(guid[3]);
        data.WriteBit(guid[0]);


        memberData << uint32(player ? player->GetReputation(REP_GUILD) : 0);
        memberData << uint8(member->GetClass());
        memberData << uint8(member->GetLevel());
        memberData << uint32(0); // sWorld->getIntConfig(CONFIG_GUILD_WEEKLY_REP_CAP)
        memberData << uint64(0); // Total activity
        memberData.WriteString(member->GetPublicNote());
        memberData.WriteByteSeq(guid[1]);
        memberData << float(player ? 0.0f : float(::time(NULL) - member->GetLogoutTime()) / DAY);
        memberData.WriteByteSeq(guid[2]);
        memberData.WriteByteSeq(guid[4]);
        memberData << uint32(player ? player->GetZoneId() : member->GetZoneId());
        memberData << uint8(1);
        memberData << uint32(50528283);
        memberData.WriteByteSeq(guid[7]);
        memberData.WriteByteSeq(guid[5]);
        memberData << uint32(player ? player->GetAchievementMgr().GetAchievementPoints() : 0);
        memberData.WriteByteSeq(guid[3]);
        memberData << uint8(flags);
        memberData.WriteByteSeq(guid[6]);
        memberData << uint64(0); // Weekly activity
        memberData.WriteString(member->GetName());
        memberData.WriteString(member->GetOfficerNote());
            
        // for (2 professions)
        for (int i = 0; i < 2; ++i)
        {
            uint32 id = player ? player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + i) : 0;

            if (id)
                memberData << uint32(id) << uint32(player->GetSkillValue(id)) << uint32(player->GetSkillStep(id));
            else
                memberData << uint32(0) << uint32(0) << uint32(0);
        }
        
        memberData.WriteByteSeq(guid[0]);
        memberData << uint32(member->GetRankId());
    }

    data.FlushBits();
    data.append(memberData);

    data.WriteString(m_info);
    data.WriteString(m_motd);

    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_ROSTER)");
}

void Guild::HandleQuery(WorldSession* session)
{
    WorldPacket data(SMSG_GUILD_QUERY_RESPONSE, 64);
    ObjectGuid guildGuid = GetGUID(); // Guild GUID is sent twice

    data.WriteBit(1); // hasData
    if (true)
    {
        data.WriteBit(guildGuid[0]);
        data.WriteBits(_GetRanksSize(), 21);
        data.WriteBit(guildGuid[2]);
        data.WriteBit(guildGuid[6]);

        for (uint8 i = 0; i < _GetRanksSize(); i++)
            data.WriteBits(m_ranks[i].GetName().size(), 7);

        data.WriteBit(guildGuid[5]);
        data.WriteBit(guildGuid[1]);
        data.WriteBit(guildGuid[7]);

        data.WriteBits(m_name.length(), 7);

        data.WriteBit(guildGuid[4]);
        data.WriteBit(guildGuid[3]);
    }
    
    data.WriteBit(guildGuid[5]);
    data.WriteBit(guildGuid[4]);
    data.WriteBit(guildGuid[3]);
    data.WriteBit(guildGuid[0]);
    data.WriteBit(guildGuid[7]);
    data.WriteBit(guildGuid[1]);
    data.WriteBit(guildGuid[6]);
    data.WriteBit(guildGuid[2]);
    
    data.WriteByteSeq(guildGuid[6]);
    

    if (true)
    {
        if (m_name.size() > 0)
            data.append(m_name.c_str(), m_name.size());
            
        data << uint32(m_emblemInfo.GetStyle());
        data << uint32(m_emblemInfo.GetColor());
        
        data.WriteByteSeq(guildGuid[1]);
        data.WriteByteSeq(guildGuid[0]);
        
        for (uint8 i = 0; i < _GetRanksSize(); ++i)
        {
            data << uint32(m_ranks[i].GetId());
            data << uint32(i);

            if (m_ranks[i].GetName().size() > 0)
                data.append(m_ranks[i].GetName().c_str(), m_ranks[i].GetName().size());
        }

        data.WriteByteSeq(guildGuid[3]);
        data.WriteByteSeq(guildGuid[4]);
        data.WriteByteSeq(guildGuid[6]);
        data.WriteByteSeq(guildGuid[2]);
        data << uint32(m_emblemInfo.GetBorderStyle());
        data << uint32(m_emblemInfo.GetBackgroundColor());
        data.WriteByteSeq(guildGuid[5]);
        data << uint32(0); // unk flag ?
        data << uint32(m_emblemInfo.GetBorderColor()); 
        data.WriteByteSeq(guildGuid[7]);
    }

    data.WriteByteSeq(guildGuid[1]);
    data.WriteByteSeq(guildGuid[7]);
    data.WriteByteSeq(guildGuid[4]);
    data.WriteByteSeq(guildGuid[5]);
    data.WriteByteSeq(guildGuid[2]);
    data.WriteByteSeq(guildGuid[0]);
    data.WriteByteSeq(guildGuid[3]);

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_QUERY_RESPONSE)");
}

void Guild::HandleGuildRanks(WorldSession* session) const
{
    ByteBuffer rankData;
    WorldPacket data(SMSG_GUILD_RANK);

    data.WriteBits(_GetRanksSize(), 17);

    for (uint8 i = 0; i < _GetRanksSize(); i++)
    {
        RankInfo const* rankInfo = GetRankInfo(i);
        if (!rankInfo)
            continue;

        data.WriteBits(rankInfo->GetName().length(), 7);

        rankData << uint32(rankInfo->GetId());
        if (rankInfo->GetName().size() > 0)
            rankData.append(rankInfo->GetName().c_str(), rankInfo->GetName().size());
        rankData << uint32(rankInfo->GetRights());

        for (uint8 j = 0; j < GUILD_BANK_MAX_TABS; ++j)
        {
            rankData << uint32(rankInfo->GetBankTabSlotsPerDay(j));
            rankData << uint32(rankInfo->GetBankTabRights(j));
        }

        rankData << uint32(rankInfo->GetBankMoneyPerDay() / GOLD);
        rankData << uint32(i);
    }

    data.FlushBits();
    data.append(rankData);

    session->SendPacket(&data);
}

void Guild::HandleSetMOTD(WorldSession* session, const std::string& motd)
{
    if (m_motd == motd)
        return;

    // Player must have rights to set MOTD
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_SETMOTD))
        SendCommandResult(session, GUILD_COMMAND_EDIT_MOTD, ERR_GUILD_PERMISSIONS);
    else
    {
        m_motd = motd;

        sScriptMgr->OnGuildMOTDChanged(this, motd);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_MOTD);
        stmt->setString(0, motd);
        stmt->setUInt32(1, m_id);
        CharacterDatabase.Execute(stmt);

        WorldPacket data(SMSG_GUILD_SEND_MOTD, m_motd.size() + 2);

        data.WriteBits(m_motd.size(), 10);
        data.FlushBits();

        if (m_motd.size() > 0)
            data.append(m_motd.c_str(), m_motd.size());

        BroadcastPacket(&data);
    }
}

void Guild::HandleSetInfo(WorldSession* session, const std::string& info)
{
    if (m_info == info)
        return;

    // Player must have rights to set guild's info
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_MODIFY_GUILD_INFO))
        SendCommandResult(session, GUILD_COMMAND_ROSTER, ERR_GUILD_PERMISSIONS);
    else
    {
        m_info = info;

        sScriptMgr->OnGuildInfoChanged(this, info);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_INFO);
        stmt->setString(0, info);
        stmt->setUInt32(1, m_id);
        CharacterDatabase.Execute(stmt);
    }
}

void Guild::HandleSetEmblem(WorldSession* session, const EmblemInfo& emblemInfo)
{
    Player* player = session->GetPlayer();
    if (!_IsLeader(player))
        // "Only guild leaders can create emblems."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTGUILDMASTER);
    else if (!player->HasEnoughMoney(uint64(EMBLEM_PRICE)))
        // "You can't afford to do that."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTENOUGHMONEY);
    else
    {
        player->ModifyMoney(-int64(EMBLEM_PRICE));

        m_emblemInfo = emblemInfo;
        m_emblemInfo.SaveToDB(m_id);

        // "Guild Emblem saved."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_SUCCESS);

        HandleQuery(session);

        m_achievementMgr.UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_TABARD, 1, 0, 0, NULL, player);
    }
}

void Guild::HandleSetLeader(WorldSession* session, const std::string& name)
{
    Player* player = session->GetPlayer();

    // Only leader can assign new leader
    if (!_IsLeader(player))
        SendCommandResult(session, GUILD_COMMAND_CHANGE_LEADER, ERR_GUILD_PERMISSIONS);

    // Old leader must be a member of guild
    else if (Member* pOldLeader = GetMember(player->GetGUID()))
    {
        // New leader must be a member of guild
        if (Member* pNewLeader = GetMember(session, name))
        {
            _SetLeaderGUID(pNewLeader);
            pOldLeader->ChangeRank(GR_OFFICER);
            _BroadcastEvent(GE_LEADER_CHANGED, 0, player->GetName(), name.c_str());
        }
    }
    else
        SendCommandResult(session, GUILD_COMMAND_CHANGE_LEADER, ERR_GUILD_PLAYER_NOT_IN_GUILD_S, name);
}

bool Guild::SwitchGuildLeader(uint64 newLeaderGuid)
{
    if (Member* pOldLeader = GetMember(GetLeaderGUID()))
    {
        // New leader must be a member of guild
        if (Member* pNewLeader = GetMember(newLeaderGuid))
        {
            _SetLeaderGUID(pNewLeader);
            pOldLeader->ChangeRank(GR_OFFICER);
            _BroadcastEvent(GE_LEADER_CHANGED, 0, pOldLeader->GetName().c_str(), pNewLeader->GetName().c_str());
            return true;
        }
    }

    return false;
}

void Guild::HandleSetBankTabInfo(WorldSession* session, uint8 tabId, const std::string& name, const std::string& icon)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        pTab->SetInfo(name, icon);
        SendBankList(session, tabId, true, true);
    }
}

void Guild::HandleSetMemberNote(WorldSession* session, std::string const& note, uint64 guid, bool isPublic)
{
    // Player must have rights to set public/officer note
    if (!_HasRankRight(session->GetPlayer(), isPublic ? GR_RIGHT_EPNOTE : GR_RIGHT_EOFFNOTE))
        SendCommandResult(session, GUILD_COMMAND_PUBLIC_NOTE, ERR_GUILD_PERMISSIONS);
    else if (Member* member = GetMember(guid))
    {
        if (isPublic)
            member->SetPublicNote(note);
        else
            member->SetOfficerNote(note);
        HandleRoster(session);
    }
}

void Guild::HandleSetRankInfo(WorldSession* session, uint32 rankId, const std::string& name, uint32 rights, uint32 moneyPerDay, GuildBankRightsAndSlotsVec rightsAndSlots)
{
    // Only leader can modify ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CHANGE_RANK, ERR_GUILD_PERMISSIONS);
    else if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Changed RankName to '%s', rights to 0x%08X", name.c_str(), rights);

        rankInfo->SetName(name);
        rankInfo->SetRights(rights);
        _SetRankBankMoneyPerDay(rankId, moneyPerDay);

        uint8 tabId = 0;
        for (GuildBankRightsAndSlotsVec::const_iterator itr = rightsAndSlots.begin(); itr != rightsAndSlots.end(); ++itr)
            _SetRankBankTabRightsAndSlots(rankId, tabId++, *itr);

        HandleQuery(session);
        HandleRoster();                                     // Broadcast for tab rights update
        HandleGuildRanks(session);
    }
}

void Guild::HandleBuyBankTab(WorldSession* session, uint8 tabId)
{
    if (tabId != GetPurchasedTabsSize())
        return;

    uint32 tabCost = _GetGuildBankTabPrice(tabId) * GOLD;
    if (!tabCost && tabId < 6) // For SPELL_EFFECT_UNLOCK_GUILD_VAULT_TAB
        return;

    Player* player = session->GetPlayer();
    if (!player->HasEnoughMoney(uint64(tabCost)))                   // Should not happen, this is checked by client
        return;

    if (!_CreateNewBankTab())
        return;

    player->ModifyMoney(-int64(tabCost));
    _SetRankBankMoneyPerDay(player->GetRank(), uint32(GUILD_WITHDRAW_MONEY_UNLIMITED));
    _SetRankBankTabRightsAndSlots(player->GetRank(), tabId, GuildBankRightsAndSlots(GUILD_BANK_RIGHT_FULL, uint32(GUILD_WITHDRAW_SLOT_UNLIMITED)));
    HandleRoster();                                         // Broadcast for tab rights update
    SendBankList(session, tabId, false, true);
    HandleGuildRanks(session);
}

void Guild::HandleSpellEffectBuyBankTab(WorldSession* session, uint8 tabId)
{
    if (tabId != GetPurchasedTabsSize())
        return;

    Player* player = session->GetPlayer();
    if (!_CreateNewBankTab())
        return;

    _SetRankBankMoneyPerDay(player->GetRank(), uint32(GUILD_WITHDRAW_MONEY_UNLIMITED));
    _SetRankBankTabRightsAndSlots(player->GetRank(), tabId, GuildBankRightsAndSlots(GUILD_BANK_RIGHT_FULL, uint32(GUILD_WITHDRAW_SLOT_UNLIMITED)));
    HandleRoster();                                         // Broadcast for tab rights update
    SendBankList(session, tabId, false, true);
    HandleGuildRanks(session);

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS, tabId + 1, 0, 0, NULL, player);
}

void Guild::HandleInviteMember(WorldSession* session, const std::string& name)
{
    Player* pInvitee = sObjectAccessor->FindPlayerByName(name.c_str());
    if (!pInvitee)
    {
        SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_GUILD_PLAYER_NOT_FOUND_S, name);
        return;
    }

    Player* player = session->GetPlayer();
    // Do not show invitations from ignored players
    if (pInvitee->GetSocial()->HasIgnore(player->GetGUIDLow()))
        return;
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && pInvitee->GetTeam() != player->GetTeam())
    {
        SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_GUILD_NOT_ALLIED);
        return;
    }
    // Invited player cannot be in another guild
    if (pInvitee->GetGuildId())
    {
        SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_ALREADY_IN_GUILD_S, name);
        return;
    }
    // Invited player cannot be invited
    if (pInvitee->GetGuildIdInvited())
    {
        SendCommandResult(pInvitee->GetSession(), GUILD_COMMAND_INVITE, ERR_INVITED_TO_GUILD);
        return;
    }
    // Inviting player must have rights to invite
    if (!_HasRankRight(player, GR_RIGHT_INVITE))
    {
        SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_GUILD_PERMISSIONS);
        return;
    }

    sLog->outDebug(LOG_FILTER_GUILD, "Player %s invited %s to join his Guild", player->GetName(), name.c_str());

    pInvitee->SetGuildIdInvited(m_id);
    _LogEvent(GUILD_EVENT_LOG_INVITE_PLAYER, player->GetGUIDLow(), pInvitee->GetGUIDLow());

    WorldPacket data(SMSG_GUILD_INVITE, 50);
    ObjectGuid oldGuildGuid = MAKE_NEW_GUID(pInvitee->GetGuildId(), 0, pInvitee->GetGuildId() ? uint32(HIGHGUID_GUILD) : 0);
    ObjectGuid newGuildGuid = GetGUID();

    data.WriteBits(m_name.length(), 7);
    data.WriteBit(oldGuildGuid[5]);
    data.WriteBit(oldGuildGuid[7]);
    data.WriteBit(newGuildGuid[6]);
    data.WriteBit(oldGuildGuid[2]);
    data.WriteBit(newGuildGuid[2]);
    data.WriteBit(oldGuildGuid[1]);
    data.WriteBits(pInvitee->GetGuildName().length(), 7);
    data.WriteBit(newGuildGuid[3]);
    data.WriteBit(oldGuildGuid[0]);
    data.WriteBit(newGuildGuid[7]);
    data.WriteBit(newGuildGuid[0]);
    data.WriteBit(oldGuildGuid[3]);
    data.WriteBit(newGuildGuid[1]);
    data.WriteBits(strlen(player->GetName()), 6);
    data.WriteBit(oldGuildGuid[4]);
    data.WriteBit(newGuildGuid[4]);
    data.WriteBit(oldGuildGuid[6]);
    data.WriteBit(newGuildGuid[5]);

    data.FlushBits();
    data.WriteByteSeq(newGuildGuid[2]);
    data.WriteByteSeq(newGuildGuid[6]);
    data.WriteByteSeq(newGuildGuid[1]);
    data.WriteByteSeq(newGuildGuid[4]);
    data.WriteByteSeq(newGuildGuid[7]);
    data.WriteByteSeq(oldGuildGuid[5]);

    data << uint32(0);
    data.WriteString(pInvitee->GetGuildName());
    data << uint32(realmID);
    data << uint32(m_emblemInfo.GetBorderStyle());
    data.WriteByteSeq(oldGuildGuid[1]);
    data.WriteByteSeq(oldGuildGuid[0]);
    data << uint32(m_emblemInfo.GetColor());    data.WriteByteSeq(oldGuildGuid[6]);
    data << uint32(realmID);
    data.WriteByteSeq(newGuildGuid[0]);
    data << uint32(m_emblemInfo.GetStyle());
    data.WriteByteSeq(oldGuildGuid[3]);
    data.WriteString(player->GetName());
    data.WriteByteSeq(oldGuildGuid[7]);
    data.WriteByteSeq(newGuildGuid[5]);
    data.WriteString(m_name);
    data.WriteByteSeq(newGuildGuid[3]);
    data << uint32(m_emblemInfo.GetBackgroundColor());
    data.WriteByteSeq(oldGuildGuid[4]);
    data << uint32(GetLevel());
    data << uint32(m_emblemInfo.GetBorderColor());
    pInvitee->GetSession()->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_INVITE)");
}

void Guild::HandleAcceptMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) &&
        player->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(GetLeaderGUID()))
        return;

    if (AddMember(player->GetGUID()))
    {
        _LogEvent(GUILD_EVENT_LOG_JOIN_GUILD, player->GetGUIDLow());

        WorldPacket data;

        data.Initialize(SMSG_GUILD_SEND_PLAYER_JOINED);
        ObjectGuid playerGuid = player->GetGUID();

        data.WriteBit(playerGuid[7]);
        data.WriteBit(playerGuid[2]);
        data.WriteBits(strlen(player->GetName()), 6);
        data.WriteBit(playerGuid[3]);
        data.WriteBit(playerGuid[1]);
        data.WriteBit(playerGuid[5]);
        data.WriteBit(playerGuid[4]);
        data.WriteBit(playerGuid[0]);
        data.WriteBit(playerGuid[6]);

        data.FlushBits();

        data << uint32(realmID);
        data.WriteByteSeq(playerGuid[2]);
        if (strlen(player->GetName()) > 0)
            data.append(player->GetName(), strlen(player->GetName()));
        data.WriteByteSeq(playerGuid[5]);
        data.WriteByteSeq(playerGuid[7]);
        data.WriteByteSeq(playerGuid[1]);
        data.WriteByteSeq(playerGuid[6]);
        data.WriteByteSeq(playerGuid[0]);
        data.WriteByteSeq(playerGuid[3]);
        data.WriteByteSeq(playerGuid[4]);

        BroadcastPacket(&data);

        data.Initialize(SMSG_GUILD_SEND_MOTD, m_motd.size() + 2);

        data.WriteBits(m_motd.size(), 10);
        data.FlushBits();

        if (m_motd.size() > 0)
            data.append(m_motd.c_str(), m_motd.size());

        session->SendPacket(&data);

        sGuildFinderMgr->RemoveMembershipRequest(player->GetGUIDLow(), GUID_LOPART(this->GetGUID()));
    }
}

void Guild::HandleLeaveMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    bool disband = false;

    // If leader is leaving
    if (_IsLeader(player))
    {
        if (m_members.size() > 1) // Leader cannot leave if he is not the last member
            SendCommandResult(session, GUILD_COMMAND_FOUNDER, ERR_GUILD_LEADER_LEAVE);
        else if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_UNDELETABLE_LEVEL))
            SendCommandResult(session, GUILD_COMMAND_FOUNDER, ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
        else
        {
            // Guild is disbanded if leader leaves.
            Disband();
            disband = true;
        }
    }
    else
    {
        SendMemberLeave(session, ObjectGuid(player->GetGUID()), false);
        DeleteMember(player->GetGUID(), false, false);
        _LogEvent(GUILD_EVENT_LOG_LEAVE_GUILD, player->GetGUIDLow());
    }

    if (disband)
        delete this;
    else
        HandleRoster(session);
}

void Guild::HandleRemoveMember(WorldSession* session, uint64 guid)
{
    Player* player = session->GetPlayer();
    Player* removedPlayer = ObjectAccessor::FindPlayer(guid);
    Member* member = GetMember(guid);

    if (!_HasRankRight(player, GR_RIGHT_REMOVE)) // The member must have rights to remove other members.
        SendCommandResult(session, GUILD_COMMAND_REMOVE, ERR_GUILD_PERMISSIONS);
    else if (member)                             // The removed player must be a member of the guild.
    {
        // Do not allow other members to remove Guild masters / player with the same rank or higher.
        if (member->IsRank(GR_GUILDMASTER) || member->IsRankNotLower(player->GetRank()))
            SendCommandResult(session, GUILD_COMMAND_REMOVE, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
        else
        {
            // After call to DeleteMember pointer to member becomes invalid
            _LogEvent(GUILD_EVENT_LOG_UNINVITE_PLAYER, player->GetGUIDLow(), GUID_LOPART(guid));
            SendMemberLeave(session, ObjectGuid(member->GetGUID()), true);
            _BroadcastEvent(GE_REMOVED, 0, member->GetName().c_str(), player->GetName());
            DeleteMember(guid, false, true);
        }
    }
    else if (removedPlayer)                      // If the removed player is not a guild member throw an error.
        SendCommandResult(session, GUILD_COMMAND_REMOVE, ERR_GUILD_PLAYER_NOT_IN_GUILD_S, removedPlayer->GetName());

    HandleRoster(session);
}

void Guild::HandleSetMemberRank(WorldSession* session, uint64 targetGuid, uint64 setterGuid, uint32 rank)
{
    Player* player = session->GetPlayer();

    // Promoted player must be a member of guild.
    if (Member* member = GetMember(targetGuid))
    {
        GuildCommandType type = rank > member->GetRankId() ? GUILD_COMMAND_DEMOTE : GUILD_COMMAND_PROMOTE;

        if (!_HasRankRight(player, rank > member->GetRankId() ? GR_RIGHT_DEMOTE : GR_RIGHT_PROMOTE))
        {
            SendCommandResult(session, type, ERR_GUILD_PERMISSIONS);
            return;
        }

        // Player cannot promote himself.
        if (member->IsSamePlayer(player->GetGUID()))
        {
            SendCommandResult(session, type, ERR_GUILD_PERMISSIONS);
            return;
        }

        SendGuildRanksUpdate(setterGuid, targetGuid, rank);
    }
    else // If he's not a guild member send an error.
        SendCommandResult(session, GUILD_COMMAND_CHANGE_RANK, ERR_GUILD_PLAYER_NOT_IN_GUILD_S, ObjectAccessor::FindPlayer(targetGuid)->GetName());
}

void Guild::HandleSwapRanks(WorldSession* session, uint32 id, bool up)
{
    RankInfo* rankinfo = NULL;
    RankInfo* rankinfo2 = NULL;
    uint32 id2 = id - (-1 + 2*uint8(up));
    for (uint32 i = 0; i < m_ranks.size(); ++i)
    {
        if (m_ranks[i].GetId() == id)
            rankinfo = &m_ranks[i];
        if (m_ranks[i].GetId() == id2)
            rankinfo2 = &m_ranks[i];
    }
    if (!rankinfo || !rankinfo2)
        return;

    RankInfo tmp = *rankinfo2;
    rankinfo2->SetName(rankinfo->GetName());
    rankinfo2->SetRights(rankinfo->GetRights());
    rankinfo->SetName(tmp.GetName());
    rankinfo->SetRights(tmp.GetRights());

    HandleQuery(session);
    HandleRoster();                                             // Broadcast for tab rights update
    HandleGuildRanks(session);
}

void Guild::HandleAddNewRank(WorldSession* session, std::string const& name) //, uint32 rankId)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return;

    // Only leader can add new rank
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CHANGE_RANK, ERR_GUILD_PERMISSIONS);
    else
    {
        _CreateRank(name, GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
        _BroadcastEvent(GE_RANK_CREATED, 0);

        HandleQuery(session);
        HandleRoster();                                             // Broadcast for tab rights update
        HandleGuildRanks(session);
    }
}

void Guild::HandleRemoveRank(WorldSession* session, uint32 rankId)
{
    // Cannot remove rank if total count is minimum allowed by the client
    if (_GetRanksSize() <= GUILD_RANKS_MIN_COUNT)
        return;

    // Only leader can delete ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CHANGE_RANK, ERR_GUILD_PERMISSIONS);
    else
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        // Delete bank rights for rank
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS_FOR_RANK);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8(1, rankId);
        CharacterDatabase.ExecuteOrAppend(trans, stmt);
        // Delete rank
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_RANK);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8(1, rankId);
        CharacterDatabase.ExecuteOrAppend(trans, stmt);

        CharacterDatabase.CommitTransaction(trans);

        m_ranks.erase(m_ranks.begin() + rankId);

        // Restruct m_ranks
        for (uint8 i = 0; i < m_ranks.size(); ++i)
            if (m_ranks[i].GetId() != i)
                m_ranks[i].UpdateId(i);

        HandleQuery(session);
        HandleRoster();                                             // Broadcast for tab rights update
        HandleGuildRanks(session);
    }
}

void Guild::HandleMemberDepositMoney(WorldSession* session, uint64 amount, bool cashFlow /*=false*/)
{
    Player* player = session->GetPlayer();

    // If guild money is over the cap display an error. Not with Cash Flow to prevent actions spam.
    if (m_bankMoney >= uint64(MAX_MONEY_AMOUNT))
    {
        if (!cashFlow)
            SendCommandResult(session, GUILD_COMMAND_MOVE_ITEM, ERR_GUILD_TOO_MUCH_MONEY);
        return;
    }

    // If guild money + amount is over the cap decrease the amount.
    if (m_bankMoney + amount > uint64(MAX_MONEY_AMOUNT))
        amount = uint64(MAX_MONEY_AMOUNT) - m_bankMoney;

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberDepositMoney(this, player, amount);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // Add money to bank
    _ModifyBankMoney(trans, amount, true);
    // Remove money from player
    player->ModifyMoney(-int64(amount));
    player->SaveGoldToDB(trans);
    // Log GM action (TODO: move to scripts)
    if (!AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(player->GetSession()->GetAccountId(), "", player->GetGUIDLow(), player->GetName(), 0, "", 0, "",
                        "GM %s (Account: %u) deposit money (Amount: %u) to guild bank (Guild ID %u)",
                        player->GetName(), player->GetSession()->GetAccountId(), amount, m_id);
    }
    // Log guild bank event
    _LogBankEvent(trans, cashFlow ? GUILD_BANK_LOG_CASH_FLOW_DEPOSIT : GUILD_BANK_LOG_DEPOSIT_MONEY, uint8(0), player->GetGUIDLow(), amount);

    CharacterDatabase.CommitTransaction(trans);

    if (!cashFlow)
        SendBankList(session, 0, false, false);
}

void Guild::DepositMoney(uint64 amount)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    _ModifyBankMoney(trans, amount, true);
    CharacterDatabase.CommitTransaction(trans);
}

bool Guild::HandleMemberWithdrawMoney(WorldSession* session, uint64 amount, bool repair)
{
    // Clamp amount to MAX_MONEY_AMOUNT, Players can't hold more than that anyway.
    amount = std::min(amount, uint64(MAX_MONEY_AMOUNT));

    if (!m_bankMoney)         // No money in the bank.
    {
        SendCommandResult(session, GUILD_COMMAND_MOVE_ITEM, ERR_GUILD_NOT_ENOUGH_MONEY);
        return false;
    }

    if (m_bankMoney < amount) // Not enough money in bank. Reduce amount.
        amount = m_bankMoney;

    Player* player = session->GetPlayer();
    if (repair)
    {
        if (!_HasRankRight(player, GR_RIGHT_WITHDRAW_REPAIR))
        {
            SendCommandResult(session, GUILD_COMMAND_REPAIR, ERR_GUILD_PERMISSIONS);
            return false;
        }
    }
    else
    {
        if (!_HasRankRight(player, GR_RIGHT_WITHDRAW_GOLD))
        {
            SendCommandResult(session, GUILD_COMMAND_MOVE_ITEM, ERR_GUILD_PERMISSIONS);
            return false;
        }
    }

    uint32 remainingMoney = _GetMemberRemainingMoney(player->GetGUID());
    if (!remainingMoney)
        return false;

    if (remainingMoney < amount)
        return false;

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberWitdrawMoney(this, player, amount, repair);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // Update remaining money amount
    if (remainingMoney < uint32(GUILD_WITHDRAW_MONEY_UNLIMITED))
        if (Member* member = GetMember(player->GetGUID()))
            member->DecreaseBankRemainingValue(trans, GUILD_BANK_MAX_TABS, amount);
    // Remove money from bank
    _ModifyBankMoney(trans, amount, false);
    // Add money to player (if required)
    if (!repair)
    {
        player->ModifyMoney(amount);
        player->SaveGoldToDB(trans);
    }
    // Log guild bank event
    _LogBankEvent(trans, repair ? GUILD_BANK_LOG_REPAIR_MONEY : GUILD_BANK_LOG_WITHDRAW_MONEY, uint8(0), player->GetGUIDLow(), amount);
    CharacterDatabase.CommitTransaction(trans);

    SendMoneyInfo(session);
    if (!repair)
        SendBankList(session, 0, false, false);
    return true;
}

void Guild::HandleMemberLogout(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (Member* member = GetMember(player->GetGUID()))
    {
        member->SetStats(player);
        member->UpdateLogoutTime();
    }

    ObjectGuid playerGuid = player->GetGUID();

    // Login event
    WorldPacket data(SMSG_GUILD_SEND_PLAYER_LOGIN_STATUS);
    data.WriteBit(playerGuid[0]);
    data.WriteBit(0);
    data.WriteBit(playerGuid[1]);
    data.WriteBit(playerGuid[4]);
    data.WriteBit(playerGuid[5]);
    data.WriteBits(strlen(session->GetPlayer()->GetName()), 6);
    data.WriteBit(playerGuid[3]);
    data.WriteBit(playerGuid[6]);
    data.WriteBit(playerGuid[7]);
    data.WriteBit(0);
    data.WriteBit(playerGuid[2]);

    data.WriteByteSeq(playerGuid[1]);

    if (strlen(session->GetPlayer()->GetName()) > 0)
        data.append(session->GetPlayer()->GetName(), strlen(session->GetPlayer()->GetName()));

    data.WriteByteSeq(playerGuid[3]);
    data.WriteByteSeq(playerGuid[7]);
    data << uint32(realmID);
    data.WriteByteSeq(playerGuid[4]);
    data.WriteByteSeq(playerGuid[0]);
    data.WriteByteSeq(playerGuid[2]);
    data.WriteByteSeq(playerGuid[6]);
    data.WriteByteSeq(playerGuid[5]);
    BroadcastPacket(&data);

    SaveToDB();
}

void Guild::HandleDisband(WorldSession* session)
{
    // Only leader can disband guild
    if (!_IsLeader(session->GetPlayer()))
        Guild::SendCommandResult(session, GUILD_COMMAND_QUIT, ERR_GUILD_PERMISSIONS);
    else if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_UNDELETABLE_LEVEL))
        Guild::SendCommandResult(session, GUILD_COMMAND_FOUNDER, ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
    else
    {
        Disband();
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Guild Successfully Disbanded");
        delete this;
    }
}

void Guild::HandleGuildPartyRequest(WorldSession* session)
{
    Player* player = session->GetPlayer();
    Group* group = player->GetGroup();

    // Make sure player is a member of the guild and that he is in a group.
    if (!IsMember(player->GetGUID()) || !group)
        return;

    WorldPacket data(SMSG_GUILD_PARTY_STATE_RESPONSE, 13);

    data.WriteBit(player->GetMap()->GetOwnerGuildId(player->GetTeam()) == GetId());     // Is guild group
    data << float(0.f);                                                                 // Guild XP multiplier
    data << uint32(0);                                                                  // Needed guild members
    data << uint32(0);                                                                  // Current guild members
    data.FlushBits();

    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent (SMSG_GUILD_PARTY_STATE_RESPONSE)");
}

///////////////////////////////////////////////////////////////////////////////
// Send data to client
void Guild::SendEventLog(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_EVENT_LOG_QUERY_RESULT);
    m_eventLog->WritePacket(data, false, false);
    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_EVENT_LOG_QUERY_RESULT)");
}

void Guild::SendBankLog(WorldSession* session, uint8 tabId) const
{
    // GUILD_BANK_MAX_TABS send by client for money log
    if (tabId < GetPurchasedTabsSize() || tabId == GUILD_BANK_MAX_TABS)
    {
        LogHolder const* log = m_bankEventLog[tabId];
        WorldPacket data(SMSG_GUILD_BANK_LOG_QUERY_RESULT);
        bool hasCashFlow = GetLevel() >= 5 && tabId == GUILD_BANK_MAX_TABS;

        data << uint32(tabId);
        log->WritePacket(data, hasCashFlow);
        if (hasCashFlow)
            data << uint64(0); // CashFlowContribution
        session->SendPacket(&data);
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LOG_QUERY_RESULT) for tab %u", tabId);
    }
}

void Guild::SendBankList(WorldSession* session, uint8 tabId, bool withContent, bool withTabInfo) const
{
    uint32 itemCount = 0;
    if (withContent)
    {
        if (_MemberHasTabRights(session->GetPlayer()->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
        {
            if (BankTab const* tab = GetBankTab(tabId))
                for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
                    if (Item* tabItem = tab->GetItem(slotId))
                        ++itemCount;
        }
        else
            SendCommandResult(session, GUILD_COMMAND_VIEW_TAB, ERR_GUILD_PERMISSIONS);
    }

    ByteBuffer tabData;
    WorldPacket data(SMSG_GUILD_BANK_LIST);

    data << uint32(tabId);
    data << uint64(m_bankMoney);
    data << uint32(_GetMemberRemainingSlots(session->GetPlayer()->GetGUID(), tabId));
    data.WriteBits(itemCount, 18);

    if (withContent && _MemberHasTabRights(session->GetPlayer()->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
    {
        if (BankTab const* tab = GetBankTab(tabId))
        {
            for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
            {
                if (Item* tabItem = tab->GetItem(slotId))
                {
                    uint32 enchants = 0;

                    /*for (uint32 ench = 0; ench < MAX_ENCHANTMENT_SLOT; ++ench)
                        if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(ench)))
                            ++enchants;*/

                    data.WriteBit(0);
                    data.WriteBits(enchants, 21);

                    /*for (uint32 ench = 0; ench < MAX_ENCHANTMENT_SLOT; ++ench)
                    {
                        if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(ench)))
                        {
                            tabData << uint32(ench);
                            tabData << uint32(enchantId);
                        }
                    }*/
                    
                    tabData << uint32(tabItem->GetCount());
                    tabData << uint32(0);
                    tabData << uint32(0);
                    tabData << uint32(tabItem->GetEntry());
                    tabData << uint32(tabItem->GetItemRandomPropertyId());
                    tabData << uint32(tabItem->GetItemSuffixFactor());
                    tabData << uint32(0);
                    tabData << uint32(slotId);
                    tabData << uint32(abs(tabItem->GetSpellCharges()));
                    tabData << uint32(0);
                }
            }
        }
    }

    data.WriteBits(withTabInfo ? GetPurchasedTabsSize() : 0, 21);

    if (withTabInfo)
    {
        for (uint8 i = 0; i < GetPurchasedTabsSize(); ++i)
        {
            data.WriteBits(m_bankTabs[i]->GetName().length(), 7);

            uint8 wrongLen = m_bankTabs[i]->GetIcon().size() % 2;
            data.WriteBits((m_bankTabs[i]->GetIcon().size() - wrongLen) / 2, 8);
            data.WriteBit(wrongLen != 0);
        }
    }

    data.WriteBit(0);

    data.FlushBits();
    if (!tabData.empty())
        data.append(tabData);

    if (withTabInfo)
    {
        for (uint8 i = 0; i < GetPurchasedTabsSize(); ++i)
        {
            if (m_bankTabs[i]->GetIcon().size() > 0)
                data.append(m_bankTabs[i]->GetIcon().c_str(), m_bankTabs[i]->GetIcon().size());

            if (m_bankTabs[i]->GetName().size() > 0)
                data.append(m_bankTabs[i]->GetName().c_str(), m_bankTabs[i]->GetName().size());

            data << uint32(i);
        }
    }

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::SendBankTabText(WorldSession* session, uint8 tabId) const
{
     if (BankTab const* tab = GetBankTab(tabId))
        tab->SendText(this, session);
}

void Guild::SendPermissions(WorldSession* session) const
{
    uint64 guid = session->GetPlayer()->GetGUID();
    uint32 rankId = session->GetPlayer()->GetRank();

    WorldPacket data(SMSG_GUILD_PERMISSIONS_QUERY_RESULTS);

    data << uint32(_GetMemberRemainingMoney(guid));
    data << uint32(GetPurchasedTabsSize());
    data << uint32(_GetRankRights(rankId));
    data << uint32(rankId);
    data.WriteBits(GUILD_BANK_MAX_TABS, 23);
    data.FlushBits();

    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        data << uint32(_GetRankBankTabRights(rankId, tabId));
        data << uint32(_GetMemberRemainingSlots(guid, tabId));
    }

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_PERMISSIONS_QUERY_RESULTS)");
}

void Guild::SendMoneyInfo(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_BANK_MONEY_WITHDRAWN, 4);
    data << uint64(_GetMemberRemainingMoney(session->GetPlayer()->GetGUID()));
    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent SMSG_GUILD_BANK_MONEY_WITHDRAWN");
}

void Guild::SendLoginInfo(WorldSession* session)
{
    /*
        Login sequence:
          SMSG_GUILD_SEND_MOTD
          SMSG_GUILD_RANK
          -- learn perks
          SMSG_GUILD_REPUTATION_WEEKLY_CAP
          SMSG_GUILD_ACHIEVEMENT_DATA
          SMSG_GUILD_MEMBER_DAILY_RESET // bank withdrawal reset
          SMSG_GUILD_SEND_PLAYER_LOGIN_STATUS
    */

    WorldPacket data(SMSG_GUILD_SEND_MOTD, m_motd.size() + 2);

    data.WriteBits(m_motd.size(), 10);

    data.FlushBits();

    if (m_motd.size() > 0)
        data.append(m_motd.c_str(), m_motd.size());

    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent SMSG_GUILD_SEND_MOTD");

    HandleGuildRanks(session);

    data.Initialize(SMSG_GUILD_MEMBER_DAILY_RESET, 0);  // tells the client to request bank withdrawal limit
    session->SendPacket(&data);

    if (!sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
        if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
            if (entry->Level <= GetLevel())
                session->GetPlayer()->learnSpell(entry->SpellId, true);

    SendGuildReputationWeeklyCap(session);

    GetAchievementMgr().SendAllAchievementData(session->GetPlayer());

    ObjectGuid playerGuid = session->GetPlayer()->GetGUID();

    // Login event
    data.Initialize(SMSG_GUILD_SEND_PLAYER_LOGIN_STATUS);
    data.WriteBit(playerGuid[0]);
    data.WriteBit(0);
    data.WriteBit(playerGuid[1]);
    data.WriteBit(playerGuid[4]);
    data.WriteBit(playerGuid[5]);
    data.WriteBits(strlen(session->GetPlayer()->GetName()), 6);
    data.WriteBit(playerGuid[3]);
    data.WriteBit(playerGuid[6]);
    data.WriteBit(playerGuid[7]);
    data.WriteBit(1);
    data.WriteBit(playerGuid[2]);

    data.WriteByteSeq(playerGuid[1]);

    if (strlen(session->GetPlayer()->GetName()) > 0)
        data.append(session->GetPlayer()->GetName(), strlen(session->GetPlayer()->GetName()));

    data.WriteByteSeq(playerGuid[3]);
    data.WriteByteSeq(playerGuid[7]);
    data << uint32(realmID);
    data.WriteByteSeq(playerGuid[4]);
    data.WriteByteSeq(playerGuid[0]);
    data.WriteByteSeq(playerGuid[2]);
    data.WriteByteSeq(playerGuid[6]);
    data.WriteByteSeq(playerGuid[5]);
    BroadcastPacket(&data);
}

void Guild::SendGuildReputationWeeklyCap(WorldSession* session) const
{
    // On 5.0.4 The cap on weekly reputation gains has been removed, but always sent. As 0 remaining.
    if (Member const* member = GetMember(session->GetPlayer()->GetGUID()))
    {
        WorldPacket data(SMSG_GUILD_REPUTATION_WEEKLY_CAP, 4);
        data << uint32(0);
        session->SendPacket(&data);
    }
}

void Guild::SendGuildRecipes(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_RECIPES);

    data.WriteBits(0, 15);
    data.FlushBits();

    /*var count = packet.ReadBits("Count", 15);

    for (int i = 0; i < count; ++i)
    {
        packet.ReadUInt32("Skill Id", i);        // iterate all SkillLineAbility.dbc rows:
        for (int j = 0; j < 300; ++j)            // if (entry->skillId != "Skill Id") continue;
            packet.ReadByte("Bit Index", i, j);  // if (mask[entry->col13 / 8] & (entry->col13 & 0x7)) recipe_spell_id: entry->spellId
    }*/

    session->SendPacket(&data);
}

void Guild::SendMemberLeave(WorldSession* session, ObjectGuid playerGuid, bool kicked)
{
    WorldPacket data(SMSG_GUILD_SEND_MEMBER_LEAVE, 20);

    ObjectGuid kickerGuid = session->GetPlayer()->GetGUID();

    Player* kickerPlayer = session->GetPlayer();
    if (!kickerPlayer)
        return;

    Member* member = GetMember(playerGuid);
    if (!member)
        return;

    if (member)
    {
        data.WriteBit(playerGuid[2]);
        data.WriteBit(playerGuid[1]);
        data.WriteBit(playerGuid[6]);
        data.WriteBit(playerGuid[3]);
        data.WriteBit(playerGuid[5]);
        data.WriteBits(member->GetName().size(), 6);
        data.WriteBit(playerGuid[7]);
        data.WriteBit(playerGuid[0]);
        data.WriteBit(playerGuid[4]);
        data.WriteBit(kicked);

        if (kicked)
        {
            data.WriteBit(1);                   // !hasRealmId ?
            data.WriteBit(0);                   // Unk
            data.WriteBit(kickerGuid[1]);
            data.WriteBit(kickerGuid[0]);
            data.WriteBit(kickerGuid[7]);
            data.WriteBit(kickerGuid[2]);
            data.WriteBit(kickerGuid[6]);
            data.WriteBit(kickerGuid[3]);
            data.WriteBit(kickerGuid[4]);
            data.WriteBit(kickerGuid[5]);
            data.WriteBit(0);                   // !Unk
            data.WriteBits(strlen(kickerPlayer->GetName()), 6);
        }

        if (kicked)
        {
            if (strlen(kickerPlayer->GetName()))
                data.append(kickerPlayer->GetName(), strlen(kickerPlayer->GetName()));

            data.WriteByteSeq(kickerGuid[2]);
            data.WriteByteSeq(kickerGuid[1]);
            data.WriteByteSeq(kickerGuid[0]);
            data.WriteByteSeq(kickerGuid[4]);
            data.WriteByteSeq(kickerGuid[3]);
            data.WriteByteSeq(kickerGuid[5]);
            data.WriteByteSeq(kickerGuid[6]);
            data.WriteByteSeq(kickerGuid[7]);
        }

        data.WriteByteSeq(playerGuid[4]);
        data.WriteByteSeq(playerGuid[0]);
        data.WriteByteSeq(playerGuid[7]);
        if (member->GetName().size())
            data.append(member->GetName().c_str(), member->GetName().size());
        data.WriteByteSeq(playerGuid[2]);
        data.WriteByteSeq(playerGuid[1]);
        data << uint32(0);              // Unk
        data.WriteByteSeq(playerGuid[3]);
        data.WriteByteSeq(playerGuid[5]);
        data.WriteByteSeq(playerGuid[6]);

        BroadcastPacket(&data);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Loading methods
bool Guild::LoadFromDB(Field* fields)
{
    m_id            = fields[0].GetUInt32();
    m_name          = fields[1].GetString();
    m_leaderGuid    = MAKE_NEW_GUID(fields[2].GetUInt32(), 0, HIGHGUID_PLAYER);
    m_emblemInfo.LoadFromDB(fields);
    m_info          = fields[8].GetString();
    m_motd          = fields[9].GetString();
    m_createdDate   = time_t(fields[10].GetUInt32());
    m_bankMoney     = fields[11].GetUInt64();
    _level          = fields[12].GetUInt32();
    _experience     = fields[13].GetUInt64();
    _todayExperience = fields[14].GetUInt64();

    uint8 purchasedTabs = uint8(fields[15].GetUInt64());
    if (purchasedTabs > GUILD_BANK_MAX_TABS)
        purchasedTabs = GUILD_BANK_MAX_TABS;

    m_bankTabs.resize(purchasedTabs);
    for (uint8 i = 0; i < purchasedTabs; ++i)
        m_bankTabs[i] = new BankTab(m_id, i);

    _CreateLogHolders();
    return true;
}

void Guild::LoadRankFromDB(Field* fields)
{
    RankInfo rankInfo(m_id);

    rankInfo.LoadFromDB(fields);

    m_ranks.push_back(rankInfo);
}

bool Guild::LoadMemberFromDB(Field* fields)
{
    uint32 lowguid = fields[1].GetUInt32();
    Member *member = new Member(m_id, MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER), fields[2].GetUInt8());
    if (!member->LoadFromDB(fields))
    {
        _DeleteMemberFromDB(lowguid);
        delete member;
        return false;
    }
    m_members[lowguid] = member;
    return true;
}

void Guild::LoadBankRightFromDB(Field* fields)
{
                                           // rights             slots
    GuildBankRightsAndSlots rightsAndSlots(fields[3].GetUInt8(), fields[4].GetUInt32());
                                  // rankId             tabId
    _SetRankBankTabRightsAndSlots(fields[2].GetUInt8(), fields[1].GetUInt8(), rightsAndSlots, false);
}

bool Guild::LoadEventLogFromDB(Field* fields)
{
    if (m_eventLog->CanInsert())
    {
        m_eventLog->LoadEvent(new EventLogEntry(
            m_id,                                       // guild id
            fields[1].GetUInt32(),                      // guid
            time_t(fields[6].GetUInt32()),              // timestamp
            GuildEventLogTypes(fields[2].GetUInt8()),   // event type
            fields[3].GetUInt32(),                      // player guid 1
            fields[4].GetUInt32(),                      // player guid 2
            fields[5].GetUInt8()));                     // rank
        return true;
    }
    return false;
}

bool Guild::LoadBankEventLogFromDB(Field* fields)
{
    uint8 dbTabId = fields[1].GetUInt8();
    bool isMoneyTab = (dbTabId == GUILD_BANK_MONEY_LOGS_TAB);
    if (dbTabId < GetPurchasedTabsSize() || isMoneyTab)
    {
        uint8 tabId = isMoneyTab ? uint8(GUILD_BANK_MAX_TABS) : dbTabId;
        LogHolder* pLog = m_bankEventLog[tabId];
        if (pLog->CanInsert())
        {
            uint32 guid = fields[2].GetUInt32();
            GuildBankEventLogTypes eventType = GuildBankEventLogTypes(fields[3].GetUInt8());
            if (BankEventLogEntry::IsMoneyEvent(eventType))
            {
                if (!isMoneyTab)
                {
                    sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: MoneyEvent(LogGuid: %u, Guild: %u) does not belong to money tab (%u), ignoring...", guid, m_id, dbTabId);
                    return false;
                }
            }
            else if (isMoneyTab)
            {
                sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: non-money event (LogGuid: %u, Guild: %u) belongs to money tab, ignoring...", guid, m_id);
                return false;
            }
            pLog->LoadEvent(new BankEventLogEntry(
                m_id,                                   // guild id
                guid,                                   // guid
                time_t(fields[8].GetUInt32()),          // timestamp
                dbTabId,                                // tab id
                eventType,                              // event type
                fields[4].GetUInt32(),                  // player guid
                fields[5].GetUInt32(),                  // item or money
                fields[6].GetUInt16(),                  // itam stack count
                fields[7].GetUInt8()));                 // dest tab id
        }
    }
    return true;
}

bool Guild::LoadBankTabFromDB(Field* fields)
{
    uint8 tabId = fields[1].GetUInt8();
    if (tabId >= GetPurchasedTabsSize())
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid tab (tabId: %u) in guild bank, skipped.", tabId);
        return false;
    }
    return m_bankTabs[tabId]->LoadFromDB(fields);
}

bool Guild::LoadBankItemFromDB(Field* fields)
{
    uint8 tabId = fields[15].GetUInt8();
    if (tabId >= GetPurchasedTabsSize())
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid tab for item (GUID: %u, id: #%u) in guild bank, skipped.",
            fields[17].GetUInt32(), fields[18].GetUInt32());
        return false;
    }
    return m_bankTabs[tabId]->LoadItemFromDB(fields);
}

// Validates guild data loaded from database. Returns false if guild should be deleted.
bool Guild::Validate()
{
    // Validate ranks data
    // GUILD RANKS represent a sequence starting from 0 = GUILD_MASTER (ALL PRIVILEGES) to max 9 (lowest privileges).
    // The lower rank id is considered higher rank - so promotion does rank-- and demotion does rank++
    // Between ranks in sequence cannot be gaps - so 0, 1, 2, 4 is impossible
    // Min ranks count is 5 and max is 10.
    bool broken_ranks = false;
    if (_GetRanksSize() < GUILD_RANKS_MIN_COUNT || _GetRanksSize() > GUILD_RANKS_MAX_COUNT)
    {
        sLog->outError(LOG_FILTER_GUILD, "Guild %u has invalid number of ranks, creating new...", m_id);
        broken_ranks = true;
    }
    else
    {
        for (uint8 rankId = 0; rankId < _GetRanksSize(); ++rankId)
        {
            RankInfo* rankInfo = GetRankInfo(rankId);
            if (rankInfo->GetId() != rankId)
            {
                sLog->outError(LOG_FILTER_GUILD, "Guild %u has broken rank id %u, creating default set of ranks...", m_id, rankId);
                broken_ranks = true;
            }
        }
    }

    if (broken_ranks)
    {
        m_ranks.clear();
        _CreateDefaultGuildRanks(DEFAULT_LOCALE);
    }

    // Validate members' data
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->GetRankId() > _GetRanksSize())
            itr->second->ChangeRank(_GetLowestRankId());

    // Repair the structure of the guild.
    // If the guildmaster doesn't exist or isn't member of the guild
    // attempt to promote another member.
    Member* pLeader = GetMember(m_leaderGuid);
    if (!pLeader)
    {
        DeleteMember(m_leaderGuid);
        // If no more members left, disband guild
        if (m_members.empty())
        {
            Disband();
            return false;
        }
    }
    else if (!pLeader->IsRank(GR_GUILDMASTER))
        _SetLeaderGUID(pLeader);

    // Check config if multiple guildmasters are allowed
    if (!ConfigMgr::GetBoolDefault("Guild.AllowMultipleGuildMaster", 0))
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->GetRankId() == GR_GUILDMASTER && !itr->second->IsSamePlayer(m_leaderGuid))
                itr->second->ChangeRank(GR_OFFICER);

    _UpdateAccountsNumber();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Broadcasts
void Guild::BroadcastToGuild(WorldSession* session, bool officerOnly, const std::string& msg, uint32 language) const
{
    if (session && session->GetPlayer())
    {
        if (_HasRankRight(session->GetPlayer(), officerOnly ? GR_RIGHT_OFFCHATSPEAK : GR_RIGHT_GCHATSPEAK))
        {
            WorldPacket data;
            ChatHandler::FillMessageData(&data, session, officerOnly ? CHAT_MSG_OFFICER : CHAT_MSG_GUILD, language, NULL, 0, msg.c_str(), NULL);
            for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
                if (Player* player = itr->second->FindPlayer())
                    if (player->GetSession() && _HasRankRight(player, officerOnly ? GR_RIGHT_OFFCHATLISTEN : GR_RIGHT_GCHATLISTEN) &&
                        !player->GetSocial()->HasIgnore(session->GetPlayer()->GetGUIDLow()))
                        player->GetSession()->SendPacket(&data);
        }
        else
            SendCommandResult(session, GUILD_COMMAND_GUILD_CHAT, ERR_GUILD_PERMISSIONS);
    }
}

void Guild::BroadcastAddonToGuild(WorldSession* session, bool officerOnly, const std::string& msg, const std::string& prefix) const
{
    if (session && session->GetPlayer())
    {
        if (_HasRankRight(session->GetPlayer(), officerOnly ? GR_RIGHT_OFFCHATSPEAK : GR_RIGHT_GCHATSPEAK))
        {
            WorldPacket data;
            ChatHandler::FillMessageData(&data, session, officerOnly ? CHAT_MSG_OFFICER : CHAT_MSG_GUILD, CHAT_MSG_ADDON, NULL, 0, msg.c_str(), NULL, prefix.c_str());
            for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
                if (Player* player = itr->second->FindPlayer())
                    if (player->GetSession() && _HasRankRight(player, officerOnly ? GR_RIGHT_OFFCHATLISTEN : GR_RIGHT_GCHATLISTEN) &&
                        !player->GetSocial()->HasIgnore(session->GetPlayer()->GetGUIDLow()) &&
                        player->GetSession()->IsAddonRegistered(prefix))
                            player->GetSession()->SendPacket(&data);
        }
        else
            SendCommandResult(session, GUILD_COMMAND_GUILD_CHAT, ERR_GUILD_PERMISSIONS);
    }
}

void Guild::BroadcastPacketToRank(WorldPacket* packet, uint8 rankId) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->IsRank(rankId))
            if (Player* player = itr->second->FindPlayer())
                player->GetSession()->SendPacket(packet);
}

void Guild::BroadcastPacket(WorldPacket* packet) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player* player = itr->second->FindPlayer())
            player->GetSession()->SendPacket(packet);
}

///////////////////////////////////////////////////////////////////////////////
// Members handling
bool Guild::AddMember(uint64 guid, uint8 rankId)
{
    Player* player = ObjectAccessor::FindPlayer(guid);
    // Player cannot be in guild
    if (player)
    {
        if (player->GetGuildId() != 0)
            return false;
    }
    else if (Player::GetGuildIdFromDB(guid) != 0)
        return false;

    // Remove all player signs from another petitions
    // This will be prevent attempt to join many guilds and corrupt guild data integrity
    Player::RemovePetitionsAndSigns(guid, GUILD_CHARTER_TYPE);

    uint32 lowguid = GUID_LOPART(guid);

    // If rank was not passed, assign lowest possible rank
    if (rankId == GUILD_RANK_NONE)
        rankId = _GetLowestRankId();

    Member* member = new Member(m_id, guid, rankId);
    if (player)
        member->SetStats(player);
    else
    {
        bool ok = false;
        // Player must exist
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_DATA_FOR_GUILD);
        stmt->setUInt32(0, lowguid);
        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            Field* fields = result->Fetch();
            member->SetStats(
                fields[0].GetString(),
                fields[1].GetUInt8(),
                fields[2].GetUInt8(),
                fields[3].GetUInt16(),
                fields[4].GetUInt32());

            ok = member->CheckStats();
        }
        if (!ok)
        {
            delete member;
            return false;
        }
    }
    m_members[lowguid] = member;

    SQLTransaction trans(NULL);
    member->SaveToDB(trans);
    // If player not in game data in will be loaded from guild tables, so no need to update it!
    if (player)
    {
        player->SetInGuild(m_id);
        player->SetRank(rankId);
        player->SetGuildLevel(GetLevel());
        player->SetGuildIdInvited(0);
        if (sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        {
            for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
                if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                    if (entry->Level <= GetLevel())
                        player->learnSpell(entry->SpellId, true);
        }

        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(REP_GUILD))
            player->GetReputationMgr().SetReputation(factionEntry, 0);
    }

    _UpdateAccountsNumber();

    // Call scripts if member was succesfully added (and stored to database)
    sScriptMgr->OnGuildAddMember(this, player, rankId);

    return true;
}

void Guild::DeleteMember(uint64 guid, bool isDisbanding, bool isKicked, bool canDeleteGuild)
{
    uint32 lowguid = GUID_LOPART(guid);
    Player* player = ObjectAccessor::FindPlayer(guid);

    // Guild master can be deleted when loading guild and guid doesn't exist in characters table
    // or when he is removed from guild by gm command
    if (m_leaderGuid == guid && !isDisbanding)
    {
        Member* oldLeader = NULL;
        Member* newLeader = NULL;
        for (Guild::Members::iterator i = m_members.begin(); i != m_members.end(); ++i)
        {
            if (i->first == lowguid)
                oldLeader = i->second;
            else if (!newLeader || newLeader->GetRankId() > i->second->GetRankId())
                newLeader = i->second;
        }
        if (!newLeader)
        {
            Disband();
            if (canDeleteGuild)
                delete this;
            return;
        }

        _SetLeaderGUID(newLeader);

        // If player not online data in data field will be loaded from guild tabs no need to update it !!
        if (Player* newLeaderPlayer = newLeader->FindPlayer())
            newLeaderPlayer->SetRank(GR_GUILDMASTER);

        // If leader does not exist (at guild loading with deleted leader) do not send broadcasts
        if (oldLeader)
        {
            _BroadcastEvent(GE_LEADER_CHANGED, 0, oldLeader->GetName().c_str(), newLeader->GetName().c_str());
            _BroadcastEvent(GE_LEFT, guid, oldLeader->GetName().c_str());
        }
    }
    // Call script on remove before member is acutally removed from guild (and database)
    sScriptMgr->OnGuildRemoveMember(this, player, isDisbanding, isKicked);

    if (Member* member = GetMember(guid))
        delete member;
    m_members.erase(lowguid);

    // If player not online data in data field will be loaded from guild tabs no need to update it !!
    if (player)
    {
        player->SetInGuild(0);
        player->SetRank(0);
        player->SetGuildLevel(0);
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                player->removeSpell(entry->SpellId, false, false);

        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(REP_GUILD))
            player->GetReputationMgr().SetReputation(factionEntry, 0);
    }

    _DeleteMemberFromDB(lowguid);
    if (!isDisbanding)
        _UpdateAccountsNumber();
}

bool Guild::ChangeMemberRank(uint64 guid, uint8 newRank)
{
    if (newRank <= _GetLowestRankId())                    // Validate rank (allow only existing ranks)
        if (Member* member = GetMember(guid))
        {
            member->ChangeRank(newRank);
            return true;
        }
    return false;
}

bool Guild::IsMember(uint64 guid)
{
    Members::const_iterator itr = m_members.find(GUID_LOPART(guid));
    return itr != m_members.end();
}

///////////////////////////////////////////////////////////////////////////////
// Bank (items move)
void Guild::SwapItems(Player* player, uint8 tabId, uint8 slotId, uint8 destTabId, uint8 destSlotId, uint32 splitedAmount)
{
    if (tabId >= GetPurchasedTabsSize() || slotId >= GUILD_BANK_MAX_SLOTS ||
        destTabId >= GetPurchasedTabsSize() || destSlotId >= GUILD_BANK_MAX_SLOTS)
        return;

    if (tabId == destTabId && slotId == destSlotId)
        return;

    BankMoveItemData from(this, player, tabId, slotId);
    BankMoveItemData to(this, player, destTabId, destSlotId);
    _MoveItems(&from, &to, splitedAmount);
}

void Guild::SwapItemsWithInventory(Player* player, bool toChar, uint8 tabId, uint8 slotId, uint8 playerBag, uint8 playerSlotId, uint32 splitedAmount)
{
    if ((slotId >= GUILD_BANK_MAX_SLOTS && slotId != NULL_SLOT) || tabId >= GetPurchasedTabsSize())
        return;

    BankMoveItemData bankData(this, player, tabId, slotId);
    PlayerMoveItemData charData(this, player, playerBag, playerSlotId);
    if (toChar)
        _MoveItems(&bankData, &charData, splitedAmount);
    else
        _MoveItems(&charData, &bankData, splitedAmount);
}

void Guild::AutoStoreItemInInventory(Player* player, uint8 tabId, uint8 slotId, uint32 amount)
{
    if ((slotId >= GUILD_BANK_MAX_SLOTS && slotId != NULL_SLOT) || tabId >= GetPurchasedTabsSize())
        return;

    Item* item = _GetItem(tabId, slotId);
    if (!item)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL);
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (msg != EQUIP_ERR_OK)
    {
        player->SendEquipError(msg, item, NULL);
        return;
    }

    if (_GetMemberRemainingSlots(player->GetGUID(), tabId) == 0)
        return;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if (BankTab* tab = GetBankTab(tabId))
        tab->SetItem(trans, slotId, NULL);

    _DecreaseMemberRemainingSlots(trans, player->GetGUID(), tabId);
    _LogBankEvent(trans, GUILD_BANK_LOG_MOVE_ITEM, tabId, player->GetGUIDLow(), item->GetEntry(), amount, 0);

    CharacterDatabase.CommitTransaction(trans);

    player->StoreItem(dest, item, true);
    SendBankList(player->GetSession(), tabId, true, true);
}

///////////////////////////////////////////////////////////////////////////////
// Bank tabs
void Guild::SetBankTabText(uint8 tabId, const std::string& text)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        pTab->SetText(text);
        pTab->SendText(this, NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
void Guild::_CreateLogHolders()
{
    m_eventLog = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_EVENT_LOG_COUNT));
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        m_bankEventLog[tabId] = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_BANK_EVENT_LOG_COUNT));
}

bool Guild::_CreateNewBankTab()
{
    if (GetPurchasedTabsSize() >= GUILD_BANK_MAX_TABS)
        return false;

    uint8 tabId = GetPurchasedTabsSize();                      // Next free id
    m_bankTabs.push_back(new BankTab(m_id, tabId));

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_TAB);
    stmt->setUInt32(0, m_id);
    stmt->setUInt8 (1, tabId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_BANK_TAB);
    stmt->setUInt32(0, m_id);
    stmt->setUInt8 (1, tabId);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
    return true;
}

void Guild::_CreateDefaultGuildRanks(LocaleConstant loc)
{
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_RANKS);
    stmt->setUInt32(0, m_id);
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS);
    stmt->setUInt32(0, m_id);
    CharacterDatabase.Execute(stmt);

    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MASTER,   loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_OFFICER,  loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_VETERAN,  loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MEMBER,   loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_INITIATE, loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
}

void Guild::_CreateRank(const std::string& name, uint32 rights)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return;

    // Ranks represent sequence 0, 1, 2, ... where 0 means guildmaster
    uint32 newRankId = _GetRanksSize();

    RankInfo info(m_id, newRankId, name, rights, 0);
    m_ranks.push_back(info);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    for (uint8 i = 0; i < GetPurchasedTabsSize(); ++i)
    {
        // Create bank rights with default values
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_BANK_RIGHT_DEFAULT);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, i);
        stmt->setUInt8 (2, newRankId);
        trans->Append(stmt);
    }
    info.SaveToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

// Updates the number of accounts that are in the guild
// Player may have many characters in the guild, but with the same account
void Guild::_UpdateAccountsNumber()
{
    // We use a set to be sure each element will be unique
    std::set<uint32> accountsIdSet;
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        accountsIdSet.insert(itr->second->GetAccountId());

    m_accountsNumber = accountsIdSet.size();
}

// Detects if player is the guild master.
// Check both leader guid and player's rank (otherwise multiple feature with
// multiple guild masters won't work)
bool Guild::_IsLeader(Player* player) const
{
    if (player->GetGUID() == m_leaderGuid)
        return true;
    if (const Member* member = GetMember(player->GetGUID()))
        return member->IsRank(GR_GUILDMASTER);
    return false;
}

void Guild::_DeleteBankItems(SQLTransaction& trans, bool removeItemsFromDB)
{
    for (uint8 tabId = 0; tabId < GetPurchasedTabsSize(); ++tabId)
    {
        m_bankTabs[tabId]->Delete(trans, removeItemsFromDB);
        delete m_bankTabs[tabId];
        m_bankTabs[tabId] = NULL;
    }
    m_bankTabs.clear();
}

bool Guild::_ModifyBankMoney(SQLTransaction& trans, uint64 amount, bool add)
{
    if (add)
        m_bankMoney += amount;
    else
    {
        // Check if there is enough money in bank.
        if (m_bankMoney < amount)
            return false;
        m_bankMoney -= amount;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_BANK_MONEY);
    stmt->setUInt64(0, m_bankMoney);
    stmt->setUInt32(1, m_id);
    trans->Append(stmt);
    return true;
}

void Guild::_SetLeaderGUID(Member* pLeader)
{
    if (!pLeader)
        return;

    m_leaderGuid = pLeader->GetGUID();
    pLeader->ChangeRank(GR_GUILDMASTER);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_LEADER);
    stmt->setUInt32(0, GUID_LOPART(m_leaderGuid));
    stmt->setUInt32(1, m_id);
    CharacterDatabase.Execute(stmt);
}

void Guild::_SetRankBankMoneyPerDay(uint32 rankId, uint32 moneyPerDay)
{
    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetMoneyTime();

        rankInfo->SetBankMoneyPerDay(moneyPerDay);
    }
}

void Guild::_SetRankBankTabRightsAndSlots(uint32 rankId, uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB)
{
    if (tabId >= GetPurchasedTabsSize())
        return;

    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetTabTimes();

        rankInfo->SetBankTabSlotsAndRights(tabId, rightsAndSlots, saveToDB);
    }
}

inline std::string Guild::_GetRankName(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetName();
    return "<unknown>";
}

inline uint32 Guild::_GetRankRights(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetRights();
    return 0;
}

inline uint32 Guild::_GetRankBankMoneyPerDay(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankMoneyPerDay();
    return 0;
}

inline uint32 Guild::_GetRankBankTabSlotsPerDay(uint32 rankId, uint8 tabId) const
{
    if (tabId < GetPurchasedTabsSize())
        if (const RankInfo* rankInfo = GetRankInfo(rankId))
            return rankInfo->GetBankTabSlotsPerDay(tabId);
    return 0;
}

inline uint32 Guild::_GetRankBankTabRights(uint32 rankId, uint8 tabId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankTabRights(tabId);
    return 0;
}

inline uint32 Guild::_GetMemberRemainingSlots(uint64 guid, uint8 tabId) const
{
    if (const Member* member = GetMember(guid))
        return member->GetBankRemainingValue(tabId, this);
    return 0;
}

inline uint32 Guild::_GetMemberRemainingMoney(uint64 guid) const
{
    if (const Member* member = GetMember(guid))
        return member->GetBankRemainingValue(GUILD_BANK_MAX_TABS, this);
    return 0;
}

inline void Guild::_DecreaseMemberRemainingSlots(SQLTransaction& trans, uint64 guid, uint8 tabId)
{
    // Remaining slots must be more then 0
    if (uint32 remainingSlots = _GetMemberRemainingSlots(guid, tabId))
        // Ignore guild master
        if (remainingSlots < uint32(GUILD_WITHDRAW_SLOT_UNLIMITED))
            if (Member* member = GetMember(guid))
                member->DecreaseBankRemainingValue(trans, tabId, 1);
}

inline bool Guild::_MemberHasTabRights(uint64 guid, uint8 tabId, uint32 rights) const
{
    if (const Member* member = GetMember(guid))
    {
        // Leader always has full rights
        if (member->IsRank(GR_GUILDMASTER) || m_leaderGuid == guid)
            return true;
        return (_GetRankBankTabRights(member->GetRankId(), tabId) & rights) == rights;
    }
    return false;
}

// Add new event log record
inline void Guild::_LogEvent(GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    m_eventLog->AddEvent(trans, new EventLogEntry(m_id, m_eventLog->GetNextGUID(), eventType, playerGuid1, playerGuid2, newRank));
    CharacterDatabase.CommitTransaction(trans);

    sScriptMgr->OnGuildEvent(this, uint8(eventType), playerGuid1, playerGuid2, newRank);
}

// Add new bank event log record
void Guild::_LogBankEvent(SQLTransaction& trans, GuildBankEventLogTypes eventType, uint8 tabId, uint32 lowguid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId)
{
    if (tabId > GUILD_BANK_MAX_TABS)
        return;

     // not logging moves within the same tab
    if (eventType == GUILD_BANK_LOG_MOVE_ITEM && tabId == destTabId)
        return;

    uint8 dbTabId = tabId;
    if (BankEventLogEntry::IsMoneyEvent(eventType))
    {
        tabId = GUILD_BANK_MAX_TABS;
        dbTabId = GUILD_BANK_MONEY_LOGS_TAB;
    }
    LogHolder* pLog = m_bankEventLog[tabId];
    pLog->AddEvent(trans, new BankEventLogEntry(m_id, pLog->GetNextGUID(), eventType, dbTabId, lowguid, itemOrMoney, itemStackCount, destTabId));

    sScriptMgr->OnGuildBankEvent(this, uint8(eventType), tabId, lowguid, itemOrMoney, itemStackCount, destTabId);
}

inline Item* Guild::_GetItem(uint8 tabId, uint8 slotId) const
{
    if (const BankTab* tab = GetBankTab(tabId))
        return tab->GetItem(slotId);
    return NULL;
}

inline void Guild::_RemoveItem(SQLTransaction& trans, uint8 tabId, uint8 slotId)
{
    if (BankTab* pTab = GetBankTab(tabId))
        pTab->SetItem(trans, slotId, NULL);
}

void Guild::_MoveItems(MoveItemData* pSrc, MoveItemData* pDest, uint32 splitedAmount)
{
    // 1. Initialize source item
    if (!pSrc->InitItem())
        return; // No source item

    // 2. Check source item
    if (!pSrc->CheckItem(splitedAmount))
        return; // Source item or splited amount is invalid
    /*
    if (pItemSrc->GetCount() == 0)
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "Guild::SwapItems: Player %s(GUIDLow: %u) tried to move item %u from tab %u slot %u to tab %u slot %u, but item %u has a stack of zero!",
            player->GetName(), player->GetGUIDLow(), pItemSrc->GetEntry(), tabId, slotId, destTabId, destSlotId, pItemSrc->GetEntry());
        //return; // Commented out for now, uncomment when it's verified that this causes a crash!!
    }
    // */

    // 3. Check destination rights
    if (!pDest->HasStoreRights(pSrc))
        return; // Player has no rights to store item in destination

    // 4. Check source withdraw rights
    if (!pSrc->HasWithdrawRights(pDest))
        return; // Player has no rights to withdraw items from source

    // 5. Check split
    if (splitedAmount)
    {
        // 5.1. Clone source item
        if (!pSrc->CloneItem(splitedAmount))
            return; // Item could not be cloned

        // 5.2. Move splited item to destination
        _DoItemsMove(pSrc, pDest, true, splitedAmount);
    }
    else // 6. No split
    {
        // 6.1. Try to merge items in destination (pDest->GetItem() == NULL)
        if (!_DoItemsMove(pSrc, pDest, false)) // Item could not be merged
        {
            // 6.2. Try to swap items
            // 6.2.1. Initialize destination item
            if (!pDest->InitItem())
                return;

            // 6.2.2. Check rights to store item in source (opposite direction)
            if (!pSrc->HasStoreRights(pDest))
                return; // Player has no rights to store item in source (opposite direction)

            if (!pDest->HasWithdrawRights(pSrc))
                return; // Player has no rights to withdraw item from destination (opposite direction)

            // 6.2.3. Swap items (pDest->GetItem() != NULL)
            _DoItemsMove(pSrc, pDest, true);
        }
    }
    // 7. Send changes
    _SendBankContentUpdate(pSrc, pDest);
}

bool Guild::_DoItemsMove(MoveItemData* pSrc, MoveItemData* pDest, bool sendError, uint32 splitedAmount)
{
    Item* pDestItem = pDest->GetItem();
    bool swap = (pDestItem != NULL);

    Item* pSrcItem = pSrc->GetItem(splitedAmount);
    // 1. Can store source item in destination
    if (!pDest->CanStore(pSrcItem, swap, sendError))
        return false;

    // 2. Can store destination item in source
    if (swap)
        if (!pSrc->CanStore(pDestItem, true, true))
            return false;

    // GM LOG (TODO: move to scripts)
    pDest->LogAction(pSrc);
    if (swap)
        pSrc->LogAction(pDest);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // 3. Log bank events
    pDest->LogBankEvent(trans, pSrc, pSrcItem->GetCount());
    if (swap)
        pSrc->LogBankEvent(trans, pDest, pDestItem->GetCount());

    // 4. Remove item from source
    pSrc->RemoveItem(trans, pDest, splitedAmount);

    // 5. Remove item from destination
    if (swap)
        pDest->RemoveItem(trans, pSrc);

    // 6. Store item in destination
    pDest->StoreItem(trans, pSrcItem);

    // 7. Store item in source
    if (swap)
        pSrc->StoreItem(trans, pDestItem);

    CharacterDatabase.CommitTransaction(trans);
    return true;
}

void Guild::_SendBankContentUpdate(MoveItemData* pSrc, MoveItemData* pDest) const
{
    ASSERT(pSrc->IsBank() || pDest->IsBank());

    uint8 tabId = 0;
    SlotIds slots;
    if (pSrc->IsBank()) // B ->
    {
        tabId = pSrc->GetContainer();
        slots.insert(pSrc->GetSlotId());
        if (pDest->IsBank()) // B -> B
        {
            // Same tab - add destination slots to collection
            if (pDest->GetContainer() == pSrc->GetContainer())
                pDest->CopySlots(slots);
            else // Different tabs - send second message
            {
                SlotIds destSlots;
                pDest->CopySlots(destSlots);
                _SendBankContentUpdate(pDest->GetContainer(), destSlots);
            }
        }
    }
    else if (pDest->IsBank()) // C -> B
    {
        tabId = pDest->GetContainer();
        pDest->CopySlots(slots);
    }

    _SendBankContentUpdate(tabId, slots);
}

void Guild::_SendBankContentUpdate(uint8 tabId, SlotIds slots) const
{
    if (BankTab const* tab = GetBankTab(tabId))
    {
        ByteBuffer tabData;
        WorldPacket data(SMSG_GUILD_BANK_LIST);

        data << uint32(tabId);
        data << uint64(m_bankMoney);

        size_t rempos = data.wpos();
        data << uint32(-1);                                      // Item withdraw amount, will be filled later

        data.WriteBits(slots.size(), 18);                                           // Item count

        for (SlotIds::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
        {
            Item const* tabItem = tab->GetItem(*itr);
            uint32 enchantCount = 0;

            /*if (tabItem)
                for (uint32 enchSlot = 0; enchSlot < MAX_ENCHANTMENT_SLOT; ++enchSlot)
                    if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(enchSlot)))
                        ++enchantCount;*/

            data.WriteBit(0);
            data.WriteBits(enchantCount, 21);                                       // enchantment count

            if (tabItem)
            {
                /*for (uint32 enchSlot = 0; enchSlot < MAX_ENCHANTMENT_SLOT; ++enchSlot)
                {
                    if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(enchSlot)))
                    {
                        tabData << uint32(enchSlot);
                        tabData << uint32(enchantId);
                    }
                }*/
            }

            tabData << uint32(tabItem ? tabItem->GetCount() : 0);
            tabData << uint32(0);
            tabData << uint32(0);
            tabData << uint32(tabItem ? tabItem->GetEntry() : 0);
            tabData << uint32(tabItem ? tabItem->GetItemRandomPropertyId() : 0);
            tabData << uint32(tabItem ? tabItem->GetItemSuffixFactor() : 0);
            tabData << uint32(0);
            tabData << uint32(*itr);
            tabData << uint32(tabItem ? abs(tabItem->GetSpellCharges()) : 0); 
            tabData << uint32(0);
        }
        
        data.WriteBits(0, 21);                                                      // Tab count

        data.WriteBit(0);
        data.FlushBits();

        if (!tabData.empty())
            data.append(tabData);

        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        {
            if (_MemberHasTabRights(itr->second->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
            {
                if (Player* player = itr->second->FindPlayer())
                {
                    data.put<uint32>(rempos, uint32(_GetMemberRemainingSlots(player->GetGUID(), tabId)));
                    player->GetSession()->SendPacket(&data);
                }
            }
        }

        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LIST)");
    }
}

void Guild::_BroadcastEvent(GuildEvents guildEvent, uint64 guid, const char* param1, const char* param2, const char* param3) const
{
    uint8 count = !param3 ? (!param2 ? (!param1 ? 0 : 1) : 2) : 3;

    WorldPacket data(SMSG_GUILD_EVENT, 1 + 1 + count + (guid ? 8 : 0));
    data << uint8(guildEvent);
    data << uint8(count);

    if (param3)
        data << param1 << param2 << param3;
    else if (param2)
        data << param1 << param2;
    else if (param1)
        data << param1;

    if (guid)
        data << uint64(guid);

    BroadcastPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent SMSG_GUILD_EVENT");
}

void Guild::SendGuildRanksUpdate(uint64 setterGuid, uint64 targetGuid, uint32 rank)
{
    ObjectGuid guid2 = targetGuid;
    ObjectGuid guid1 = setterGuid;

    Member* member = GetMember(targetGuid);
    ASSERT(member);

    WorldPacket data(SMSG_GUILD_RANKS_UPDATE, 100);
    
    data.WriteBit(guid1[3]);
    data.WriteBit(guid1[2]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid1[5]);
    data.WriteBit(rank < member->GetRankId()); // 1 == higher, 0 = lower?
    data.WriteBit(guid2[3]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid2[4]);
    data.WriteBit(guid2[0]);

    data << uint32(rank);
    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid1[1]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid2[4]);
    data.WriteByteSeq(guid2[3]);

    BroadcastPacket(&data);

    member->ChangeRank(rank);

    _LogEvent((rank < member->GetRankId()) ? GUILD_EVENT_LOG_DEMOTE_PLAYER : GUILD_EVENT_LOG_PROMOTE_PLAYER, GUID_LOPART(setterGuid), GUID_LOPART(targetGuid), rank);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_GUILD_RANKS_UPDATE");
}

void Guild::GiveXP(uint32 xp, Player* source)
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    /// @TODO: Award reputation and count activity for player

    if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_MAX_LEVEL))
        xp = 0; // SMSG_GUILD_XP_GAIN is always sent, even for no gains

    WorldPacket data(SMSG_GUILD_XP_GAIN, 8);
    data << uint64(xp);    // XP missing for next level
    source->GetSession()->SendPacket(&data);

    _experience += xp;
    _todayExperience += xp;

    if (!xp)
        return;

    uint32 oldLevel = GetLevel();

    // Ding, mon!
    while (GetExperience() >= sGuildMgr->GetXPForGuildLevel(GetLevel()) && GetLevel() < sWorld->getIntConfig(CONFIG_GUILD_MAX_LEVEL))
    {
        _experience -= sGuildMgr->GetXPForGuildLevel(GetLevel());
        ++_level;

        // Find all guild perks to learn
        std::vector<uint32> perksToLearn;
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                if (entry->Level > oldLevel && entry->Level <= GetLevel())
                    perksToLearn.push_back(entry->SpellId);

        // Notify all online players that guild level changed and learn perks
        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)

        {
            if (Player* player = itr->second->FindPlayer())
            {
                player->SetGuildLevel(GetLevel());
                for (size_t i = 0; i < perksToLearn.size(); ++i)
                    player->learnSpell(perksToLearn[i], true);
            }
        }
    
        GetNewsLog().AddNewEvent(GUILD_NEWS_LEVEL_UP, time(NULL), 0, 0, _level);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL, GetLevel(), 0, 0, NULL, source);

        ++oldLevel;
    }
}

void Guild::SendGuildXP(WorldSession* session) const
{
    Member const* member = GetMember(session->GetGuidLow());

    WorldPacket data(SMSG_GUILD_XP, 32);
    data << uint64(GetExperience());
    data << uint64(0); // Max daily XP. Starting with patch 5.0.4, there is no longer any cap on how much Guild XP your guild can earn each day. 
    data << uint64(sGuildMgr->GetXPForGuildLevel(GetLevel()) - GetExperience());    // XP missing for next level
    data << uint64(GetTodayExperience());
    session->SendPacket(&data);
}

// Starting with 5.0.4, the amount of Guild XP for quest turn-ins by low-level characters is similar to high-level.
// The exact formula appears to be 60k Guild XP for every level-appropriate quest.
// Any quests that are too low for your character to earn XP will earn zero Guild XP.
// Red caps at Orange. Red / Orange quests 90k, Yellow quests 60k, Green 30k, Grey 0.
uint32 Guild::CalculateQuestExperienceReward(uint8 playerLevel, uint32 questLevel)
{
    uint32 experience = 0;

	uint32 pLevel = playerLevel;
    if (questLevel >= pLevel + 3)                                       // Red + Orange (High level) quests (questLevel >= playerLevel + 3 (O) / 5 (R)) award N x 1.5.
        experience = GUILD_EXPERIENCE_ABOVE_LEVEL_QUEST;
    else if (questLevel >= pLevel - 2 && questLevel < pLevel + 3)       // Yellow (Normal) quests (questLevel >= playerLevel - 2) award N x 1.
        experience = GUILD_EXPERIENCE_SAME_LEVEL_QUEST;
    else                                                                // Green (Low level) quests award N x 0.5.
        experience = GUILD_EXPERIENCE_BELOW_LEVEL_QUEST;

    return experience;
}

void Guild::ResetDailyExperience()
{
    _todayExperience = 0;

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player* player = itr->second->FindPlayer())
            SendGuildXP(player->GetSession());
}

void Guild::GiveMemberReputation(uint64 guid, uint32 value)
{
    if (Member* member = GetMember(guid))
        member->GiveReputation(value);
}

void Guild::ResetWeeklyReputation()
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Member* guildMember = itr->second)
            guildMember->SetWeeklyReputation(0);
}

void Guild::GuildNewsLog::AddNewEvent(GuildNews eventType, time_t date, uint64 playerGuid, uint32 flags, uint32 data)
{
    uint32 id = _newsLog.size();
    GuildNewsEntry& log = _newsLog[id];
    log.EventType = eventType;
    log.PlayerGuid = playerGuid;
    log.Data = data;
    log.Flags = flags;
    log.Date = date;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SAVE_GUILD_NEWS);
    stmt->setUInt32(0, GetGuild()->GetId());
    stmt->setUInt32(1, id);
    stmt->setUInt32(2, log.EventType);
    stmt->setUInt64(3, log.PlayerGuid);
    stmt->setUInt32(4, log.Data);
    stmt->setUInt32(5, log.Flags);
    stmt->setUInt32(6, uint32(log.Date));
    CharacterDatabase.Execute(stmt);

    WorldPacket packet;
    BuildNewsData(id, log, packet);
    GetGuild()->BroadcastPacket(&packet);
}

void Guild::GuildNewsLog::LoadFromDB(PreparedQueryResult result)
{
    if (!result)
        return;
    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetInt32();
        GuildNewsEntry& log = _newsLog[id];
        log.EventType = GuildNews(fields[1].GetInt32());
        log.PlayerGuid = fields[2].GetInt64();
        log.Data = fields[3].GetInt32();
        log.Flags = fields[4].GetInt32();
        log.Date = time_t(fields[5].GetInt32());
    }
    while (result->NextRow());
}

void Guild::GuildNewsLog::BuildNewsData(uint32 id, GuildNewsEntry& guildNew, WorldPacket& data)
{
    data.Initialize(SMSG_GUILD_NEWS_UPDATE);

    data.WriteBits(1, 19);

    ObjectGuid guid = guildNew.PlayerGuid;

    uint8 bitsOrder[8] = { 1, 5, 0, 7, 3, 4, 6, 2 };
    data.WriteBitInOrder(guid, bitsOrder);

    data.WriteBits(0, 24); // Other Guids NYI
    data.FlushBits();
    
    data.WriteByteSeq(guid[5]);    
    data.WriteByteSeq(guid[2]);
    data << uint32(secsToTimeBitFields(guildNew.Date));
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[7]);
    data << uint32(guildNew.Flags); // 1 sticky
    data.WriteByteSeq(guid[1]);
    data << uint32(guildNew.Data);
    data << uint32(guildNew.EventType);
    data.WriteByteSeq(guid[3]);
    data << uint32(id);
    data << uint32(0);              // always 0
}

void Guild::GuildNewsLog::BuildNewsData(WorldPacket& data)
{
    data.Initialize(SMSG_GUILD_NEWS_UPDATE, 3 + ((24 + 8 + 4) * _newsLog.size()));

    data.WriteBits(_newsLog.size(), 19);

    for (GuildNewsLogMap::const_iterator it = _newsLog.begin(); it != _newsLog.end(); it++)
    {
        ObjectGuid guid = it->second.PlayerGuid;

        uint8 bitsOrder[8] = { 1, 5, 0, 7, 3, 4, 6, 2 };
        data.WriteBitInOrder(guid, bitsOrder);
        
        data.WriteBits(0, 24); // Not yet implemented used for guild achievements
    }

    data.FlushBits();

    for (GuildNewsLogMap::const_iterator it = _newsLog.begin(); it != _newsLog.end(); it++)
    {
        ObjectGuid guid = it->second.PlayerGuid;

        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[2]);
        data << uint32(secsToTimeBitFields(it->second.Date));
        data.WriteByteSeq(guid[4]);
        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[7]);
        data << uint32(it->second.Flags); // 1 sticky
        data.WriteByteSeq(guid[1]);
        data << uint32(it->second.Data);
        data << uint32(it->second.EventType);
        data.WriteByteSeq(guid[3]);
        data << uint32(it->first);
        data << uint32(0);              // always 0
    }
}
