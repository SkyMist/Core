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

#ifndef GUILD_H
#define GUILD_H

#include "AchievementMgr.h"
#include "World.h"
#include "Item.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "DBCStore.h"

class Item;

enum GuildMisc
{
    GUILD_BANK_MAX_TABS                  = 8,                    // Sent by client for money log also
    GUILD_BANK_MAX_SLOTS                 = 98,
    GUILD_BANK_MONEY_LOGS_TAB            = 100,                  // Used for money log in DB
    GUILD_RANKS_MIN_COUNT                = 5,                    // Guilds must have a minimum of 5 ranks. More ranks can be added by the Leader, up to a maximum of 10 ranks. 
    GUILD_RANKS_MAX_COUNT                = 10,
    GUILD_RANK_NONE                      = 0xFF,

    GUILD_WITHDRAW_MONEY_UNLIMITED       = 0xFFFFFFFF,
    GUILD_WITHDRAW_SLOT_UNLIMITED        = 0xFFFFFFFF,
    GUILD_EVENT_LOG_GUID_UNDEFINED       = 0xFFFFFFFF,

    GUILD_EXPERIENCE_ABOVE_LEVEL_QUEST   = 90000,
    GUILD_EXPERIENCE_SAME_LEVEL_QUEST    = 60000,
    GUILD_EXPERIENCE_BELOW_LEVEL_QUEST   = 30000,

    TAB_UNDEFINED                        = 0xFF
};

enum GuildMemberData
{
    GUILD_MEMBER_DATA_ZONEID             = 0,
    GUILD_MEMBER_DATA_ACHIEVEMENT_POINTS = 1,
    GUILD_MEMBER_DATA_LEVEL              = 2
};

enum GuildDefaultRanks
{
    // These ranks can be modified, but they cannot be deleted.
    // When promoting member server does: rank-- ; When demoting member server does: rank++.
    // Higher rank -> less privileges.

    GR_GUILDMASTER                       = 0,
    GR_OFFICER                           = 1,
    GR_VETERAN                           = 2,
    GR_MEMBER                            = 3,
    GR_INITIATE                          = 4
};

enum GuildRankRights
{
    GR_RIGHT_EMPTY                       = 0x00000040,
    GR_RIGHT_GCHATLISTEN                 = GR_RIGHT_EMPTY | 0x00000001,
    GR_RIGHT_GCHATSPEAK                  = GR_RIGHT_EMPTY | 0x00000002,
    GR_RIGHT_OFFCHATLISTEN               = GR_RIGHT_EMPTY | 0x00000004,
    GR_RIGHT_OFFCHATSPEAK                = GR_RIGHT_EMPTY | 0x00000008,
    GR_RIGHT_INVITE                      = GR_RIGHT_EMPTY | 0x00000010,
    GR_RIGHT_REMOVE                      = GR_RIGHT_EMPTY | 0x00000020,
    GR_RIGHT_PROMOTE                     = GR_RIGHT_EMPTY | 0x00000080,
    GR_RIGHT_DEMOTE                      = GR_RIGHT_EMPTY | 0x00000100,
    GR_RIGHT_SETMOTD                     = GR_RIGHT_EMPTY | 0x00001000,
    GR_RIGHT_EPNOTE                      = GR_RIGHT_EMPTY | 0x00002000,
    GR_RIGHT_VIEWOFFNOTE                 = GR_RIGHT_EMPTY | 0x00004000,
    GR_RIGHT_EOFFNOTE                    = GR_RIGHT_EMPTY | 0x00008000,
    GR_RIGHT_MODIFY_GUILD_INFO           = GR_RIGHT_EMPTY | 0x00010000,
    GR_RIGHT_WITHDRAW_GOLD_LOCK          = 0x00020000,                   // remove money withdraw capacity
    GR_RIGHT_WITHDRAW_REPAIR             = 0x00040000,                   // withdraw for repair
    GR_RIGHT_WITHDRAW_GOLD               = 0x00080000,                   // withdraw gold
    GR_RIGHT_CREATE_GUILD_EVENT          = 0x00100000,                   // wotlk
    GR_RIGHT_ALL                         = 0x00DDFFBF
};

enum GuildCommandType
{
    GUILD_COMMAND_CREATE                 = 0,
    GUILD_COMMAND_INVITE                 = 1,
    GUILD_COMMAND_QUIT                   = 3,
    GUILD_COMMAND_ROSTER                 = 5,
    GUILD_COMMAND_PROMOTE                = 6,
    GUILD_COMMAND_DEMOTE                 = 7,
    GUILD_COMMAND_REMOVE                 = 8,
    GUILD_COMMAND_CHANGE_LEADER          = 10,
    GUILD_COMMAND_EDIT_MOTD              = 11,
    GUILD_COMMAND_GUILD_CHAT             = 13,
    GUILD_COMMAND_FOUNDER                = 14,
    GUILD_COMMAND_CHANGE_RANK            = 16,
    GUILD_COMMAND_PUBLIC_NOTE            = 19,
    GUILD_COMMAND_VIEW_TAB               = 21,
    GUILD_COMMAND_MOVE_ITEM              = 22,
    GUILD_COMMAND_REPAIR                 = 25
};

enum GuildCommandError
{
    ERR_PLAYER_NO_MORE_IN_GUILD          = 0,  // ! Obsolete?
    ERR_GUILD_INTERNAL                   = 1,  // "Internal guild error.";
    ERR_ALREADY_IN_GUILD                 = 2,  // "You are already in a guild."
    ERR_ALREADY_IN_GUILD_S               = 3,  // "%s is already in a guild."
    ERR_INVITED_TO_GUILD                 = 4,  // "You have already been invited into a guild."
    ERR_ALREADY_INVITED_TO_GUILD_S       = 5,  // "|Hplayer:%s|h[%s]|h invites you to join %s."
    ERR_GUILD_NAME_INVALID               = 6,  // "Invalid guild name."
    ERR_GUILD_NAME_EXISTS_S              = 7,  // "There is already a guild named \"%s\"."
    ERR_GUILD_LEADER_LEAVE               = 8,  // "You must promote a new Guild Master using /gleader before leaving the guild."
    ERR_GUILD_PERMISSIONS                = 8,  // "You don't have permission to do that."
    ERR_GUILD_PLAYER_NOT_IN_GUILD        = 9,  // "You are not in a guild."
    ERR_GUILD_PLAYER_NOT_IN_GUILD_S      = 10, // "%s is not in your guild."
    ERR_GUILD_PLAYER_NOT_FOUND_S         = 11, // \"%s\" not found."
    ERR_GUILD_NOT_ALLIED                 = 12, // "You cannot invite players from the opposing alliance"
    ERR_GUILD_RANK_TOO_HIGH_S            = 13, // "%s's rank is too high"
    ERR_GUILD_RANK_TOO_LOW_S             = 14, // "%s is already at the lowest rank"
    ERR_GUILD_RANKS_LOCKED               = 17, // "Temporary guild error.  Please try again!"
    ERR_GUILD_RANK_IN_USE                = 18, // "That guild rank is currently in use."
    ERR_GUILD_IGNORING_YOU_S             = 19, // ! Obsolete or actually player error, not guild?
    ERR_GUILD_UNK1                       = 20, // Forces roster update
    ERR_GUILD_WITHDRAW_LIMIT             = 25, // "You cannot withdraw that much from the guild bank."
    ERR_GUILD_NOT_ENOUGH_MONEY           = 26, // "The guild bank does not have enough money"
    ERR_GUILD_BANK_FULL                  = 28, // "This guild bank tab is full"
    ERR_GUILD_ITEM_NOT_FOUND             = 29, // ! Obsolete?
    ERR_GUILD_TOO_MUCH_MONEY             = 31, // "The guild bank is at gold limit"
    ERR_GUILD_BANK_WRONG_TAB             = 32, // "Incorrect bank tab"
    ERR_RANK_REQUIRES_AUTHENTICATOR      = 34, // "Guild rank requires an authenticator."
    ERR_GUILD_BANK_VOUCHER_FAILED        = 35, // "You must purchase all guild bank tabs before using this voucher."
    ERR_GUILD_TRIAL_ACCOUNT              = 36, // "Feature not available for Starter Edition accounts."
    ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL   = 37, // "Your guild is too high level to be deleted."
    ERR_GUILD_MOVE_STARTING              = 38, // ! Obsolete?
    ERR_GUILD_REP_TOO_LOW                = 39  // "Your guild reputation isn't high enough to do that."

    // ToDo: Find and update with these.
    // ERR_GUILD_CREATE_S           // "%s created.";
    // ERR_GUILD_DECLINE_S          // "%s declines your guild invitation.";
    // ERR_GUILD_DEMOTE_SS          // "%s  has been demoted to %s.";
    // ERR_GUILD_DEMOTE_SSS         // "%s has demoted %s to %s.";
    // ERR_GUILD_DISBANDED          // "Guild has been disbanded.";
    // ERR_GUILD_DISBAND_S          // "%s has disbanded the guild.";
    // ERR_GUILD_DISBAND_SELF       // "You have disbanded the guild.";
    // ERR_GUILD_FOUNDER_S          // "Congratulations, you are a founding member of %s!";
    // ERR_GUILD_ACCEPT             // "You have joined the guild.";
    // ERR_GUILD_INVITE_S           // "You have invited %s to join your guild.";
    // ERR_GUILD_INVITE_SELF        // "You can't invite yourself to a guild.";
    // ERR_GUILD_JOIN_S             // "%s has joined the guild.";
    // ERR_GUILD_LEADER_CHANGED_SS  // "%s has made %s the new Guild Master.";
    // ERR_GUILD_LEADER_IS_S        // "%s is the leader of your guild.";
    // ERR_GUILD_LEADER_REPLACED    // "Because the previous guild master %s has not logged in for an extended time, %s has become the new Guild Master.";
    // ERR_GUILD_LEADER_S           // "%s has been promoted to Guild Master.";
    // ERR_GUILD_LEADER_SELF        // "You are now the Guild Master.";
    // ERR_GUILD_LEAVE_RESULT       // "You have left the guild.";
    // ERR_GUILD_LEAVE_S            // "%s has left the guild.";
    // ERR_GUILD_PROMOTE_SSS        // "%s has promoted %s to %s.";
    // ERR_GUILD_QUIT_S             // "You are no longer a member of %s.";
    // ERR_GUILD_REMOVE_SELF        // "You have been kicked out of the guild.";
    // ERR_GUILD_REMOVE_SS          // "%s has been kicked out of the guild by %s.";
    // ERR_GUILD_BANK_BOUND_ITEM    // "You cannot store soulbound items in the guild bank";
    // ERR_GUILD_BANK_CONJURED_ITEM // "You cannot store conjured items in the guild bank";
    // ERR_GUILD_BANK_EQUIPPED_ITEM // "You must unequip that item first";
    // ERR_GUILD_BANK_QUEST_ITEM    // "You cannot store quest items in the guild bank";
    // ERR_GUILD_BANK_WRAPPED_ITEM  // "You cannot store wrapped items in the guild bank";
};

enum GuildEvents // API Lua Bank events.
{
    GE_PROMOTION                        = 1,  // Fired when a guild member is promoted.
    GE_DEMOTION                         = 2,  // Fired when a guild member is demoted.
    GE_MOTD                             = 3,  // Fired when a guild member logs in or the MOTD is changed.
    GE_JOINED                           = 4,  // Fired when a player joins the guild.
    GE_LEFT                             = 5,  // Fired when a player leaves the guild.
    GE_REMOVED                          = 6,  // Fired when a player is removed from the guild.
    GE_LEADER_IS                        = 7,  // Fired when the guild roster is opened / the ranks queried. Also at creation.
    GE_LEADER_CHANGED                   = 8,  // Fired when guild leader changes.
    GE_DISBANDED                        = 9,  // Fired when the guild is disbanded.
    GE_TABARDCHANGE                     = 10, // Fired when the guild tabard (emblem) changes.
    GE_RANK_UPDATED                     = 11, // Fired when a rank is updated (name / permissions).
    GE_RANK_CREATED                     = 12, // Fired when a rank is created.
    GE_RANK_DELETED                     = 13, // Fired when a rank is deleted.
    GE_RANK_ORDER_CHANGED               = 14, // Fired when the rank orders are changed.
    GE_FOUNDER                          = 15, // Fired when the guild is created.
    GE_SIGNED_ON                        = 16, // Fired when a guild member comes online.
    GE_SIGNED_OFF                       = 17, // Fired when a guild member goes offline.
    GE_GUILDBANKBAGSLOTS_CHANGED        = 18, // Fired when the guild-bank contents change.
    GE_BANK_TAB_PURCHASED               = 19, // This event only fires when bank bags slots are purchased. It no longer fires when bags in the slots are changed. Instead, when the bags are changed, PLAYERBANKSLOTS_CHANGED will fire, and arg1 will be NUM_BANKGENERIC_SLOTS + BagIndex.
    GE_BANK_TAB_UPDATED                 = 20, // Fired when the One of the slots in the player's 24 bank slots has changed, or when any of the equipped bank bags have changed. Does not fire when an item is added to or removed from a bank bag. When (arg1 <= NUM_BANKGENERIC_SLOTS), arg1 is the index of the generic bank slot that changed. When (arg1 > NUM_BANKGENERIC_SLOTS), (arg1 - NUM_BANKGENERIC_SLOTS) is the index of the equipped bank bag that changed. 
    GE_BANK_MONEY_UPDATED               = 21, // Fired when guild-bank money changes.
    GE_BANK_MONEY_WITHDRAWN             = 22, // Fired when guild-bank money is withdrawn.
    GE_BANK_TEXT_CHANGED                = 23, // Fired when guild-bank text changes.
    // 24 - error 795
    GE_SIGNED_ON_MOBILE                 = 25, // Fired when a guild member comes online via mobile.
    GE_SIGNED_Off_MOBILE                = 26  // Fired when a guild member goes offline via mobile.

    // ToDo: Find and update with these.
    // BANKFRAME_CLOSED            // Fired twice when the bank window is closed. Only at the first one of them the bank data is still available (GetNumBankSlots(), GetContainerItemLink(), ...). 
    // BANKFRAME_OPENED            // Fired when the bank frame is opened.
    // GUILDBANKFRAME_CLOSED       // Fired when the guild-bank frame is closed.
    // GUILDBANKFRAME_OPENED       // Fired when the guild-bank frame is opened.
    // GUILDBANKLOG_UPDATE         // Fired when the guild-bank log is updated.
    // GUILDBANK_ITEM_LOCK_CHANGED // Fired when the guild-bank items are locked / unlocked.
};

enum PetitionTurns
{
    PETITION_TURN_OK                    = 0,  // "Charter turned in."
    PETITION_TURN_ALREADY_IN_GUILD      = 2,  // "You are already in a guild."
    PETITION_TURN_GUILD_PERMISSIONS     = 12, // "You don't have permission to do that."
    PETITION_TURN_GUILD_NAME_INVALID    = 13, // "That name contains invalid characters.  Enter a new name."
    PETITION_TURN_NEED_MORE_SIGNATURES  = 15  // "You need more signatures."
};

enum PetitionSigns
{
    PETITION_SIGN_DECLINED              = 0,  // "%s has declined to sign your petition."
    PETITION_SIGN_NOT_SERVER            = 1,  // "That player is not from your server"
    PETITION_SIGN_ALREADY_IN_GUILD      = 2,  // "You are already in a guild."
    PETITION_SIGN_ALREADY_SIGNED        = 3,  // "You have already signed that charter."
    PETITION_SIGN_OK                    = 4,  // "Charter signed."
    PETITION_SIGN_CANT_SIGN_OWN         = 6,  // "You can't sign your own charter."
    PETITION_SIGN_FULL                  = 9,  // "That petition is full"
    PETITION_SIGN_ALREADY_SIGNED_OTHER  = 10, // "You've already signed another guild charter"
    PETITION_SIGN_RESTRICTED_ACCOUNT    = 12  // "Starter Edition accounts may not sign guild charters. \124cffffd000\124HurlIndex:2\124h[Click To Upgrade]\124h\124r"

    // ToDo: Find and update with these.
    // PETITION_ALREADY_SIGNED_BY_S   // "%s has already signed your charter."
    // PETITION_OFFERED_S             // "You have requested %s's signature."
    // PETITION_SIGNED_S              // "%s has signed your charter."
};

enum GuildBankRights
{
    GUILD_BANK_RIGHT_VIEW_TAB           = 0x01,
    GUILD_BANK_RIGHT_PUT_ITEM           = 0x02,
    GUILD_BANK_RIGHT_UPDATE_TEXT        = 0x04,

    GUILD_BANK_RIGHT_DEPOSIT_ITEM       = GUILD_BANK_RIGHT_VIEW_TAB | GUILD_BANK_RIGHT_PUT_ITEM,
    GUILD_BANK_RIGHT_FULL               = 0xFF
};

enum GuildBankEventLogTypes
{
    GUILD_BANK_LOG_DEPOSIT_ITEM         = 1,
    GUILD_BANK_LOG_WITHDRAW_ITEM        = 2,
    GUILD_BANK_LOG_MOVE_ITEM            = 3,
    GUILD_BANK_LOG_DEPOSIT_MONEY        = 4,
    GUILD_BANK_LOG_WITHDRAW_MONEY       = 5,
    GUILD_BANK_LOG_REPAIR_MONEY         = 6,
    GUILD_BANK_LOG_MOVE_ITEM2           = 7,
    GUILD_BANK_LOG_UNK1                 = 8,
    GUILD_BANK_LOG_BUY_SLOT             = 9,
    GUILD_BANK_LOG_CASH_FLOW_DEPOSIT    = 10
};

enum GuildEventLogTypes
{
    GUILD_EVENT_LOG_INVITE_PLAYER       = 1,
    GUILD_EVENT_LOG_JOIN_GUILD          = 2,
    GUILD_EVENT_LOG_PROMOTE_PLAYER      = 3,
    GUILD_EVENT_LOG_DEMOTE_PLAYER       = 4,
    GUILD_EVENT_LOG_UNINVITE_PLAYER     = 5,
    GUILD_EVENT_LOG_LEAVE_GUILD         = 6
};

enum GuildEmblemError
{
    ERR_GUILDEMBLEM_SUCCESS               = 0, // "Guild Emblem saved."
    ERR_GUILDEMBLEM_INVALID_TABARD_COLORS = 1, // "Invalid Guild Emblem colors."
    ERR_GUILDEMBLEM_NOGUILD               = 2, // "You are not part of a guild!"
    ERR_GUILDEMBLEM_NOTGUILDMASTER        = 3, // "Only guild leaders can create emblems."
    ERR_GUILDEMBLEM_NOTENOUGHMONEY        = 4, // "You can't afford to do that."
    ERR_GUILDEMBLEM_INVALIDVENDOR         = 5  // "That's not an emblem vendor!"

    // ToDo: Find and update with these.
    // ERR_GUILDEMBLEM_COLORSPRESENT      // "Your guild already has an emblem!"
    // ERR_GUILDEMBLEM_SAME               // "Not saved, your tabard is already like that."
};

enum GuildMemberFlags
{
    GUILDMEMBER_STATUS_NONE      = 0x0000,
    GUILDMEMBER_STATUS_ONLINE    = 0x0001,
    GUILDMEMBER_STATUS_AFK       = 0x0002,
    GUILDMEMBER_STATUS_DND       = 0x0004,
    GUILDMEMBER_STATUS_MOBILE    = 0x0008  // remote chat from mobile app
};

enum GuildNews
{
    GUILD_NEWS_GUILD_ACHIEVEMENT    = 0,
    GUILD_NEWS_PLAYER_ACHIEVEMENT   = 1,
    GUILD_NEWS_DUNGEON_ENCOUNTER    = 2,  // @todo Implement
    GUILD_NEWS_ITEM_LOOTED          = 3,
    GUILD_NEWS_ITEM_CRAFTED         = 4,
    GUILD_NEWS_ITEM_PURCHASED       = 5,
    GUILD_NEWS_LEVEL_UP             = 6
};

enum GuildChallengeType
{
    CHALLENGE_NONE                  = 0,
    CHALLENGE_DUNGEON               = 1,
    CHALLENGE_SCENARIO              = 2,
    CHALLENGE_DUNGEON_CHALLENGE     = 3,
    CHALLENGE_RAID                  = 4,
    CHALLENGE_RATED_BG              = 5,

    CHALLENGE_MAX                   = 6
};

struct GuildNewsEntry
{
    GuildNews EventType;
    time_t Date;
    uint64 PlayerGuid;
    uint32 Flags;
    uint32 Data;
};

struct GuildReward
{
    uint32 Entry;
    int32 Racemask;
    uint64 Price;
    uint32 AchievementId;
    uint8 Standing;
};

uint32 const MinNewsItemLevel[MAX_CONTENT] = { 61, 90, 200, 353 };

typedef std::map<uint32, GuildNewsEntry> GuildNewsLogMap;

////////////////////////////////////////////////////////////////////////////////////////////
// Emblem info
class EmblemInfo
{
    public:
        EmblemInfo() : m_style(0), m_color(0), m_borderStyle(0), m_borderColor(0), m_backgroundColor(0) { }

        void LoadFromDB(Field* fields);
        void SaveToDB(uint32 guildId) const;
        void ReadPacket(WorldPacket& recv)
        {
            recv
                >> m_backgroundColor
                >> m_borderColor
                >> m_color
                >> m_borderStyle
                >> m_style
                ;
        }
        void WritePacket(WorldPacket& data) const;

        uint32 GetStyle() const { return m_style; }
        uint32 GetColor() const { return m_color; }
        uint32 GetBorderStyle() const { return m_borderStyle; }
        uint32 GetBorderColor() const { return m_borderColor; }
        uint32 GetBackgroundColor() const { return m_backgroundColor; }

    private:
        uint32 m_style;
        uint32 m_color;
        uint32 m_borderStyle;
        uint32 m_borderColor;
        uint32 m_backgroundColor;
};

// Structure for storing guild bank rights and remaining slots together.
struct GuildBankRightsAndSlots
{
    GuildBankRightsAndSlots() : rights(0), slots(0) { }
    GuildBankRightsAndSlots(uint32 _rights, uint32 _slots) : rights(_rights), slots(_slots) { }

    inline bool IsEqual(GuildBankRightsAndSlots const& rhs) const { return rights == rhs.rights && slots == rhs.slots; }
    void SetGuildMasterValues()
    {
        rights = GUILD_BANK_RIGHT_FULL;
        slots = uint32(GUILD_WITHDRAW_SLOT_UNLIMITED);
    }

    uint32 rights;
    uint32 slots;
};
typedef std::vector <GuildBankRightsAndSlots> GuildBankRightsAndSlotsVec;

typedef std::set <uint8> SlotIds;

class Guild
{
    private:

        // Member class, representing guild members.
        class Member 
        {
            struct RemainingValue
            {
                RemainingValue() : value(0), resetTime(0) { }

                uint32 value;
                uint32 resetTime;
            };

            public:
                Member(uint32 guildId, uint64 guid, uint32 rankId) :
                    m_guildId(guildId),
                    m_guid(guid),
                    m_zoneId(0),
                    m_level(0),
                    m_class(0),
                    m_logoutTime(::time(NULL)),
                    m_accountId(0),
                    m_rankId(rankId),
                    m_achievementPoints(0),
                    m_totalActivity(0),
                    m_weekActivity(0),
                    m_totalReputation(0),
                    m_weekReputation(0) { }

                void SetStats(Player* player);
                void SetStats(const std::string& name, uint8 level, uint8 _class, uint32 zoneId, uint32 accountId);
                bool CheckStats() const;

                void SetPublicNote(const std::string& publicNote);
                void SetOfficerNote(const std::string& officerNote);

                std::string GetPublicNote() { return m_publicNote; };
                std::string GetOfficerNote() { return m_officerNote; };

                void SetZoneId(uint32 id) { m_zoneId = id; }
                void SetLevel(uint8 var) { m_level = var; }

                bool LoadFromDB(Field* fields);
                void SaveToDB(SQLTransaction& trans) const;

                std::string GetName() const { return m_name; }

                uint64 GetGUID() const { return m_guid; }

                uint32 GetAccountId() const { return m_accountId; }
                uint32 GetRankId() const { return m_rankId; }

                uint8 GetClass() const { return m_class; }
                uint8 GetLevel() const { return m_level; }
                uint8 GetZoneId() const { return m_zoneId; }

                inline void UpdateLogoutTime() { m_logoutTime = ::time(NULL); }
                uint64 GetLogoutTime() const { return m_logoutTime; }

                inline Player* FindPlayer() const { return ObjectAccessor::FindPlayer(m_guid); }

                // Guild Ranks.
                void ChangeRank(uint8 newRank);

                inline bool IsRank(uint8 rankId) const { return m_rankId == rankId; }
                inline bool IsRankNotLower(uint8 rankId) const { return m_rankId <= rankId; }
                inline bool IsSamePlayer(uint64 guid) const { return m_guid == guid; }

                // Guild Bank.
                void DecreaseBankRemainingValue(SQLTransaction& trans, uint8 tabId, uint32 amount);
                uint32 GetBankRemainingValue(uint8 tabId, const Guild* guild) const;

                void ResetTabTimes();
                void ResetMoneyTime();

                // Achievements.
                void SetAchievementPoints(uint32 val) { m_achievementPoints = val; }
                uint32 GetAchievementPoints() const { return m_achievementPoints; }

                // Reputation.
                void GiveReputation(uint32 value);
                void SetReputation(uint32 value) { m_totalReputation = value; }
                uint32 GetReputation() const { return m_totalReputation; }

                void SetWeeklyReputation(uint32 value) { m_weekReputation = value; }
                uint32 GetWeeklyReputation() const { return m_weekReputation; }

            private:
                uint32 m_guildId;

                // Fields from characters table.
                uint64 m_guid;
                std::string m_name;
                uint32 m_zoneId;
                uint8  m_level;
                uint8  m_class;
                uint8 m_flags;
                uint64 m_logoutTime;
                uint32 m_accountId;

                // Fields from guild_member table.
                uint32 m_rankId;
                std::string m_publicNote;
                std::string m_officerNote;

                RemainingValue m_bankRemaining[GUILD_BANK_MAX_TABS + 1];

                uint32 m_achievementPoints;
                uint64 m_totalActivity;
                uint64 m_weekActivity;
                uint32 m_totalReputation;
                uint32 m_weekReputation;
        };

        // News Log class
        class GuildNewsLog
        {
            public:
                GuildNewsLog(Guild* guild) : _guild(guild) { }

                void LoadFromDB(PreparedQueryResult result);
                void BuildNewsData(WorldPacket& data);
                void BuildNewsData(uint32 id, GuildNewsEntry& guildNew, WorldPacket& data);
                void AddNewEvent(GuildNews eventType, time_t date, uint64 playerGuid, uint32 flags, uint32 data);
                GuildNewsEntry* GetNewsById(uint32 id)
                {
                    GuildNewsLogMap::iterator itr = _newsLog.find(id);
                    if (itr != _newsLog.end())
                        return &itr->second;
                    return NULL;
                }
                Guild* GetGuild() const { return _guild; }

            private:
                Guild* _guild;
                GuildNewsLogMap _newsLog;
        };

        // Base class for event entries
        class LogEntry
        {
            public:
                LogEntry(uint32 guildId, uint32 guid) : m_guildId(guildId), m_guid(guid), m_timestamp(::time(NULL)) { }
                LogEntry(uint32 guildId, uint32 guid, time_t timestamp) : m_guildId(guildId), m_guid(guid), m_timestamp(timestamp) { }
                virtual ~LogEntry() { }

                uint32 GetGUID() const { return m_guid; }

                virtual void SaveToDB(SQLTransaction& trans) const = 0;
                virtual void WritePacket(WorldPacket& data, ByteBuffer& content, bool hasCashFlow = false) const = 0;

            protected:
                uint32 m_guildId;
                uint32 m_guid;
                uint64 m_timestamp;
        };

        // Event log entry
        class EventLogEntry : public LogEntry
        {
            public:
                EventLogEntry(uint32 guildId, uint32 guid, GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank) :
                    LogEntry(guildId, guid), m_eventType(eventType), m_playerGuid1(playerGuid1), m_playerGuid2(playerGuid2), m_newRank(newRank) { }

                EventLogEntry(uint32 guildId, uint32 guid, time_t timestamp, GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank) :
                    LogEntry(guildId, guid, timestamp), m_eventType(eventType), m_playerGuid1(playerGuid1), m_playerGuid2(playerGuid2), m_newRank(newRank) { }

                ~EventLogEntry() { }

                void SaveToDB(SQLTransaction& trans) const;
                void WritePacket(WorldPacket& data, ByteBuffer& content, bool hasCashFlow = false) const;

            private:
                GuildEventLogTypes m_eventType;
                uint32 m_playerGuid1;
                uint32 m_playerGuid2;
                uint8  m_newRank;
        };

        // Bank event log entry
        class BankEventLogEntry : public LogEntry
        {
            public:
                static bool IsMoneyEvent(GuildBankEventLogTypes eventType)
                {
                    return
                        eventType == GUILD_BANK_LOG_DEPOSIT_MONEY ||
                        eventType == GUILD_BANK_LOG_WITHDRAW_MONEY ||
                        eventType == GUILD_BANK_LOG_REPAIR_MONEY ||
                        eventType == GUILD_BANK_LOG_CASH_FLOW_DEPOSIT;
                }

                bool IsMoneyEvent() const
                {
                    return IsMoneyEvent(m_eventType);
                }

                BankEventLogEntry(uint32 guildId, uint32 guid, GuildBankEventLogTypes eventType, uint8 tabId, uint32 playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId) :
                    LogEntry(guildId, guid), m_eventType(eventType), m_bankTabId(tabId), m_playerGuid(playerGuid),
                    m_itemOrMoney(itemOrMoney), m_itemStackCount(itemStackCount), m_destTabId(destTabId) { }

                BankEventLogEntry(uint32 guildId, uint32 guid, time_t timestamp, uint8 tabId, GuildBankEventLogTypes eventType, uint32 playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId) :
                    LogEntry(guildId, guid, timestamp), m_eventType(eventType), m_bankTabId(tabId), m_playerGuid(playerGuid),
                    m_itemOrMoney(itemOrMoney), m_itemStackCount(itemStackCount), m_destTabId(destTabId) { }

                ~BankEventLogEntry() { }

                void SaveToDB(SQLTransaction& trans) const;
                void WritePacket(WorldPacket& data, ByteBuffer& content, bool hasCashFlow = false) const;

            private:
                GuildBankEventLogTypes m_eventType;
                uint8  m_bankTabId;
                uint32 m_playerGuid;
                uint32 m_itemOrMoney;
                uint16 m_itemStackCount;
                uint8  m_destTabId;
        };

        // Class encapsulating work with events collection
        class LogHolder
        {
            public:
                LogHolder(uint32 guildId, uint32 maxRecords) : m_guildId(guildId), m_maxRecords(maxRecords), m_nextGUID(uint32(GUILD_EVENT_LOG_GUID_UNDEFINED)) { }
                ~LogHolder();

                uint8 GetSize() const { return uint8(m_log.size()); }
                // Checks if new log entry can be added to holder when loading from DB
                inline bool CanInsert() const { return m_log.size() < m_maxRecords; }
                // Adds event from DB to collection
                void LoadEvent(LogEntry* entry);
                // Adds new event to collection and saves it to DB
                void AddEvent(SQLTransaction& trans, LogEntry* entry);
                // Writes information about all events to packet
                void WritePacket(WorldPacket& data, bool hasCashFlow = false, bool isBankLog = true) const;
                uint32 GetNextGUID();

            private:
                typedef std::list<LogEntry*> GuildLog;
                GuildLog m_log;
                uint32 m_guildId;
                uint32 m_maxRecords;
                uint32 m_nextGUID;
        };

        // Class encapsulating guild rank data
        class RankInfo
        {
            public:
                RankInfo(uint32 guildId) : m_guildId(guildId), m_rankId(GUILD_RANK_NONE), m_rights(GR_RIGHT_EMPTY), m_bankMoneyPerDay(0) { }
                RankInfo(uint32 guildId, uint32 rankId, const std::string& name, uint32 rights, uint32 money) :
                    m_guildId(guildId), m_rankId(rankId), m_name(name), m_rights(rights), m_bankMoneyPerDay(money) { }

                void LoadFromDB(Field* fields);
                void SaveToDB(SQLTransaction& trans) const;

                uint32 GetId() const { return m_rankId; }
                void SetId(uint32 id) { m_rankId = id; }
                void UpdateId(uint32 newId);

                std::string GetName() const { return m_name; }
                void SetName(const std::string& name);

                uint32 GetRights() const { return m_rights; }
                void SetRights(uint32 rights);

                bool operator < (const RankInfo& rank) const { return m_rights > rank.GetRights(); }
                bool operator == (const RankInfo& rank) const { return m_rights == rank.GetRights(); }

                uint32 GetBankMoneyPerDay() const { return m_rankId == GR_GUILDMASTER ? GUILD_WITHDRAW_MONEY_UNLIMITED : m_bankMoneyPerDay; }
                void SetBankMoneyPerDay(uint32 money);

                inline uint32 GetBankTabRights(uint8 tabId) const { return tabId < GUILD_BANK_MAX_TABS ? m_bankTabRightsAndSlots[tabId].rights : 0; }
                inline uint32 GetBankTabSlotsPerDay(uint8 tabId) const
                {
                    if (tabId < GUILD_BANK_MAX_TABS)
                        return m_rankId == GR_GUILDMASTER ? GUILD_WITHDRAW_SLOT_UNLIMITED : m_bankTabRightsAndSlots[tabId].slots;
                    return 0;
                }
                void SetBankTabSlotsAndRights(uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB);

            private:
                uint32 m_guildId;
                uint32 m_rankId;
                std::string m_name;
                uint32 m_rights;
                uint32 m_bankMoneyPerDay;
                GuildBankRightsAndSlots m_bankTabRightsAndSlots[GUILD_BANK_MAX_TABS];
        };

        class BankTab
        {
            public:
                BankTab(uint32 guildId, uint8 tabId) : m_guildId(guildId), m_tabId(tabId)
                {
                    memset(m_items, 0, GUILD_BANK_MAX_SLOTS * sizeof(Item*));
                }

                bool LoadFromDB(Field* fields);
                bool LoadItemFromDB(Field* fields);
                void Delete(SQLTransaction& trans, bool removeItemsFromDB = false);

                void SetInfo(std::string const& name, std::string const& icon);
                void SetText(std::string const& text);
                void SendText(Guild const* guild, WorldSession* session) const;

                std::string const& GetName() const { return m_name; }
                std::string const& GetIcon() const { return m_icon; }
                std::string const& GetText() const { return m_text; }

                inline Item* GetItem(uint8 slotId) const { return slotId < GUILD_BANK_MAX_SLOTS ?  m_items[slotId] : NULL; }
                bool SetItem(SQLTransaction& trans, uint8 slotId, Item* item);

            private:
                uint32 m_guildId;
                uint8 m_tabId;

                Item* m_items[GUILD_BANK_MAX_SLOTS];
                std::string m_name;
                std::string m_icon;
                std::string m_text;
        };

        // Movement data
        class MoveItemData
        {
            public:
                MoveItemData(Guild* guild, Player* player, uint8 container, uint8 slotId) : m_pGuild(guild), m_pPlayer(player),
                    m_container(container), m_slotId(slotId), m_pItem(NULL), m_pClonedItem(NULL) { }
                virtual ~MoveItemData() { }

                virtual bool IsBank() const = 0;
                // Initializes item pointer. Returns true, if item exists, false otherwise.
                virtual bool InitItem() = 0;
                // Checks splited amount against item. Splited amount cannot be more that number of items in stack.
                virtual bool CheckItem(uint32& splitedAmount);
                // Defines if player has rights to save item in container
                virtual bool HasStoreRights(MoveItemData* /*pOther*/) const { return true; }
                // Defines if player has rights to withdraw item from container
                virtual bool HasWithdrawRights(MoveItemData* /*pOther*/) const { return true; }
                // Checks if container can store specified item
                bool CanStore(Item* pItem, bool swap, bool sendError);
                // Clones stored item
                bool CloneItem(uint32 count);
                // Remove item from container (if splited update items fields)
                virtual void RemoveItem(SQLTransaction& trans, MoveItemData* pOther, uint32 splitedAmount = 0) = 0;
                // Saves item to container
                virtual Item* StoreItem(SQLTransaction& trans, Item* pItem) = 0;
                // Log bank event
                virtual void LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const = 0;
                // Log GM action
                virtual void LogAction(MoveItemData* pFrom) const;
                // Copy slots id from position vector
                void CopySlots(SlotIds& ids) const;

                Item* GetItem(bool isCloned = false) const { return isCloned ? m_pClonedItem : m_pItem; }
                uint8 GetContainer() const { return m_container; }
                uint8 GetSlotId() const { return m_slotId; }
            protected:
                virtual InventoryResult CanStore(Item* pItem, bool swap) = 0;

                Guild* m_pGuild;
                Player* m_pPlayer;
                uint8 m_container;
                uint8 m_slotId;
                Item* m_pItem;
                Item* m_pClonedItem;
                ItemPosCountVec m_vec;
        };

        class PlayerMoveItemData : public MoveItemData
        {
            public:
                PlayerMoveItemData(Guild* guild, Player* player, uint8 container, uint8 slotId) :
                    MoveItemData(guild, player, container, slotId) { }

                bool IsBank() const { return false; }
                bool InitItem();
                void RemoveItem(SQLTransaction& trans, MoveItemData* pOther, uint32 splitedAmount = 0);
                Item* StoreItem(SQLTransaction& trans, Item* pItem);
                void LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const;
            protected:
                InventoryResult CanStore(Item* pItem, bool swap);
        };

        class BankMoveItemData : public MoveItemData
        {
            public:
                BankMoveItemData(Guild* guild, Player* player, uint8 container, uint8 slotId) :
                    MoveItemData(guild, player, container, slotId) { }

                bool IsBank() const { return true; }
                bool InitItem();
                bool HasStoreRights(MoveItemData* pOther) const;
                bool HasWithdrawRights(MoveItemData* pOther) const;
                void RemoveItem(SQLTransaction& trans, MoveItemData* pOther, uint32 splitedAmount);
                Item* StoreItem(SQLTransaction& trans, Item* pItem);
                void LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const;
                void LogAction(MoveItemData* pFrom) const;

            protected:
                InventoryResult CanStore(Item* pItem, bool swap);

            private:
                Item* _StoreItem(SQLTransaction& trans, BankTab* pTab, Item* pItem, ItemPosCount& pos, bool clone) const;
                bool _ReserveSpace(uint8 slotId, Item* pItem, Item* pItemDest, uint32& count);
                void CanStoreItemInTab(Item* pItem, uint8 skipSlotId, bool merge, uint32& count);
        };

        typedef UNORDERED_MAP<uint32, Member*> Members;
        typedef std::vector<RankInfo> Ranks;
        typedef std::vector<BankTab*> BankTabs;

    public:
        static void SendCommandResult(WorldSession* session, GuildCommandType type, GuildCommandError errCode, const std::string& param = "");
        static void SendSaveEmblemResult(WorldSession* session, GuildEmblemError errCode);

        Guild();
        ~Guild();

        bool Create(Player* pLeader, const std::string& name);
        void Disband();

        void SaveToDB();

        // Getters
        uint32 GetId() const { return m_id; }
        uint64 GetGUID() const { return MAKE_NEW_GUID(m_id, 0, HIGHGUID_GUILD); }
        uint64 GetLeaderGUID() const { return m_leaderGuid; }
        const std::string& GetName() const { return m_name; }
        const std::string& GetMOTD() const { return m_motd; }
        const std::string& GetInfo() const { return m_info; }

        bool SetName(std::string const& name);

        // Handle client commands
        void HandleRoster(WorldSession* session = NULL);          // NULL = broadcast
        void HandleQuery(WorldSession* session);
        void HandleGuildRanks(WorldSession* session) const;
        void HandleSetMOTD(WorldSession* session, const std::string& motd);
        void HandleSetInfo(WorldSession* session, const std::string& info);
        void HandleSetEmblem(WorldSession* session, const EmblemInfo& emblemInfo);
        void HandleSetLeader(WorldSession* session, const std::string& name);
        void HandleSetBankTabInfo(WorldSession* session, uint8 tabId, const std::string& name, const std::string& icon);
        void HandleSetMemberNote(WorldSession* session, std::string const& note, uint64 guid, bool isPublic);
        void HandleSetRankInfo(WorldSession* session, uint32 rankId, const std::string& name, uint32 rights, uint32 moneyPerDay, GuildBankRightsAndSlotsVec rightsAndSlots);
        void HandleBuyBankTab(WorldSession* session, uint8 tabId);
        void HandleSpellEffectBuyBankTab(WorldSession* session, uint8 tabId);
        void HandleInviteMember(WorldSession* session, const std::string& name);
        void HandleAcceptMember(WorldSession* session);
        void HandleLeaveMember(WorldSession* session);
        void HandleRemoveMember(WorldSession* session, uint64 guid);
        void HandleSetMemberRank(WorldSession* session, uint64 targetGuid, uint64 setterGuid, uint32 rank);
        void HandleAddNewRank(WorldSession* session, const std::string& name);
        void HandleRemoveRank(WorldSession* session, uint32 rankId);
        void HandleChangeNameRank(WorldSession* session, uint32 id, std::string const& name);
        void HandleSwapRanks(WorldSession* session, uint32 id, bool up);
        void HandleMemberDepositMoney(WorldSession* session, uint64 amount, bool cashFlow = false);
        bool HandleMemberWithdrawMoney(WorldSession* session, uint64 amount, bool repair = false);
        void HandleMemberLogout(WorldSession* session);
        void HandleDisband(WorldSession* session);
        void HandleGuildPartyRequest(WorldSession* session);

        void UpdateMemberData(Player* player, uint8 dataid, uint32 value);

        // Send info to client
        void SendEventLog(WorldSession* session) const;
        void SendBankLog(WorldSession* session, uint8 tabId) const;
        void SendBankList(WorldSession* session, uint8 tabId, bool withContent, bool withTabInfo) const;
        void SendBankTabText(WorldSession* session, uint8 tabId) const;
        void SendPermissions(WorldSession* session) const;
        void SendMoneyInfo(WorldSession* session) const;
        void SendLoginInfo(WorldSession* session);
        void SendGuildReputationWeeklyCap(WorldSession* session) const;
        void SendGuildXP(WorldSession* session) const;
        void SendGuildRecipes(WorldSession* session) const;
        void SendMemberLeave(WorldSession* session, ObjectGuid playerGuid, bool kicked);

        // Load from DB
        bool LoadFromDB(Field* fields);
        void LoadRankFromDB(Field* fields);
        bool LoadMemberFromDB(Field* fields);
        bool LoadEventLogFromDB(Field* fields);
        void LoadBankRightFromDB(Field* fields);
        bool LoadBankTabFromDB(Field* fields);
        bool LoadBankEventLogFromDB(Field* fields);
        bool LoadBankItemFromDB(Field* fields);
        bool Validate();

        void DepositMoney(uint64 amount);

        // Broadcasts
        void BroadcastToGuild(WorldSession* session, bool officerOnly, const std::string& msg, uint32 language = LANG_UNIVERSAL) const;
        void BroadcastAddonToGuild(WorldSession* session, bool officerOnly, const std::string& msg, const std::string& prefix) const;
        void BroadcastPacketToRank(WorldPacket* packet, uint8 rankId) const;
        void BroadcastPacket(WorldPacket* packet) const;

        template<class Do>
        void BroadcastWorker(Do& _do, Player* except = NULL)
        {
            for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
                if (Player* player = itr->second->FindPlayer())
                    if (player != except)
                        _do(player);
        }

        // Members
        // Adds member to guild. If rankId == GUILD_RANK_NONE, lowest rank is assigned.
        bool AddMember(uint64 guid, uint8 rankId = GUILD_RANK_NONE);
        void DeleteMember(uint64 guid, bool isDisbanding = false, bool isKicked = false, bool canDeleteGuild = false);
        bool ChangeMemberRank(uint64 guid, uint8 newRank);
        bool IsMember(uint64 guid);
        bool SwitchGuildLeader(uint64 newLeaderGuid);
        uint32 GetMembersCount() { return m_members.size(); }

        // Bank
        void SwapItems(Player* player, uint8 tabId, uint8 slotId, uint8 destTabId, uint8 destSlotId, uint32 splitedAmount);
        void SwapItemsWithInventory(Player* player, bool toChar, uint8 tabId, uint8 slotId, uint8 playerBag, uint8 playerSlotId, uint32 splitedAmount);
        void AutoStoreItemInInventory(Player* player, uint8 tabId, uint8 slotId, uint32 amount);
        uint64 GetBankMoney() { return m_bankMoney; }

        // Bank tabs
        void SetBankTabText(uint8 tabId, const std::string& text);

        AchievementMgr<Guild>& GetAchievementMgr() { return m_achievementMgr; }
        AchievementMgr<Guild> const& GetAchievementMgr() const { return m_achievementMgr; }

         // Guild leveling
        uint32 GetLevel() const { return _level; }

        void GiveXP(uint32 xp, Player* source);
        uint64 GetExperience() const { return _experience; }
        uint64 GetTodayExperience() const { return _todayExperience; }

        uint32 CalculateQuestExperienceReward(uint8 playerLevel, uint32 questLevel);

        void GiveMemberReputation(uint64 guid, uint32 value);

        void ResetDailyExperience();
        void ResetWeeklyReputation();

        GuildNewsLog& GetNewsLog() { return _newsLog; }

        EmblemInfo const& GetEmblemInfo() const { return m_emblemInfo; }

        inline uint8 GetPurchasedTabsSize() const { return uint8(m_bankTabs.size()); }

        uint32 GetMemberLogoutTime(uint64 guid)
        {
            if (const Member* member = GetMember(guid))
                return member->GetLogoutTime();

            return 0;
        }

    protected:
        uint32 m_id;
        std::string m_name;
        uint64 m_leaderGuid;
        std::string m_motd;
        std::string m_info;
        time_t m_createdDate;

        EmblemInfo m_emblemInfo;
        uint32 m_accountsNumber;
        uint64 m_bankMoney;

        Ranks m_ranks;
        Members m_members;
        BankTabs m_bankTabs;

        // These are actually ordered lists. The first element is the oldest entry.
        LogHolder* m_eventLog;
        LogHolder* m_bankEventLog[GUILD_BANK_MAX_TABS + 1];

        AchievementMgr<Guild> m_achievementMgr;
        GuildNewsLog _newsLog;

        uint32 _level;
        uint64 _experience;
        uint64 _todayExperience;

    private:
        inline uint32 _GetRanksSize() const { return uint32(m_ranks.size()); }
        inline const RankInfo* GetRankInfo(uint32 rankId) const { return rankId < _GetRanksSize() ? &m_ranks[rankId] : NULL; }
        inline RankInfo* GetRankInfo(uint32 rankId) { return rankId < _GetRanksSize() ? &m_ranks[rankId] : NULL; }
        inline bool _HasRankRight(Player* player, uint32 right) const { return (_GetRankRights(player->GetRank()) & right) != GR_RIGHT_EMPTY; }
        inline uint32 _GetLowestRankId() const { return uint32(m_ranks.size() - 1); }

        inline BankTab* GetBankTab(uint8 tabId) { return tabId < m_bankTabs.size() ? m_bankTabs[tabId] : NULL; }
        inline const BankTab* GetBankTab(uint8 tabId) const { return tabId < m_bankTabs.size() ? m_bankTabs[tabId] : NULL; }

        inline const Member* GetMember(uint64 guid) const
        {
            Members::const_iterator itr = m_members.find(GUID_LOPART(guid));
            return itr != m_members.end() ? itr->second : NULL;
        }
        inline Member* GetMember(uint64 guid)
        {
            Members::iterator itr = m_members.find(GUID_LOPART(guid));
            return itr != m_members.end() ? itr->second : NULL;
        }
        inline Member* GetMember(WorldSession* session, const std::string& name)
        {
            for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
                if (itr->second->GetName() == name)
                    return itr->second;

            SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_GUILD_PLAYER_NOT_IN_GUILD_S, name);
            return NULL;
        }
        inline void _DeleteMemberFromDB(uint32 lowguid) const
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_MEMBER);
            stmt->setUInt32(0, lowguid);
            CharacterDatabase.Execute(stmt);
        }

        // Creates log holders (either when loading or when creating guild)
        void _CreateLogHolders();
        // Tries to create new bank tab
        bool _CreateNewBankTab();
        // Creates default guild ranks with names in given locale
        void _CreateDefaultGuildRanks(LocaleConstant loc);
        // Creates new rank
        void _CreateRank(const std::string& name, uint32 rights);
        // Update account number when member added/removed from guild
        void _UpdateAccountsNumber();
        bool _IsLeader(Player* player) const;
        void _DeleteBankItems(SQLTransaction& trans, bool removeItemsFromDB = false);
        bool _ModifyBankMoney(SQLTransaction& trans, uint64 amount, bool add);
        void _SetLeaderGUID(Member* pLeader);

        void _SetRankBankMoneyPerDay(uint32 rankId, uint32 moneyPerDay);
        void _SetRankBankTabRightsAndSlots(uint32 rankId, uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB = true);
        uint32 _GetRankBankTabRights(uint32 rankId, uint8 tabId) const;
        uint32 _GetRankRights(uint32 rankId) const;
        uint32 _GetRankBankMoneyPerDay(uint32 rankId) const;
        uint32 _GetRankBankTabSlotsPerDay(uint32 rankId, uint8 tabId) const;
        std::string _GetRankName(uint32 rankId) const;

        uint32 _GetMemberRemainingSlots(uint64 guid, uint8 tabId) const;
        uint32 _GetMemberRemainingMoney(uint64 guid) const;
        void _DecreaseMemberRemainingSlots(SQLTransaction& trans, uint64 guid, uint8 tabId);
        bool _MemberHasTabRights(uint64 guid, uint8 tabId, uint32 rights) const;

        void _LogEvent(GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2 = 0, uint8 newRank = 0);
        void _LogBankEvent(SQLTransaction& trans, GuildBankEventLogTypes eventType, uint8 tabId, uint32 playerGuid, uint32 itemOrMoney, uint16 itemStackCount = 0, uint8 destTabId = 0);

        Item* _GetItem(uint8 tabId, uint8 slotId) const;
        void _RemoveItem(SQLTransaction& trans, uint8 tabId, uint8 slotId);
        void _MoveItems(MoveItemData* pSrc, MoveItemData* pDest, uint32 splitedAmount);
        bool _DoItemsMove(MoveItemData* pSrc, MoveItemData* pDest, bool sendError, uint32 splitedAmount = 0);

        void _SendBankContentUpdate(MoveItemData* pSrc, MoveItemData* pDest) const;
        void _SendBankContentUpdate(uint8 tabId, SlotIds slots) const;

        void SendGuildRanksUpdate(uint64 setterGuid, uint64 targetGuid, uint32 rank);

        void _BroadcastEvent(GuildEvents guildEvent, uint64 guid, const char* param1 = NULL, const char* param2 = NULL, const char* param3 = NULL) const;
};
#endif
