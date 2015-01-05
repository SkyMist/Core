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
#include "ObjectMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"

#include "AccountMgr.h"
#include "CellImpl.h"
#include "Chat.h"
#include "GridNotifiersImpl.h"
#include "Language.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "Group.h"
#include "UpdateMask.h"
#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "ChatLink.h"
#include "Guild.h"

bool ChatHandler::load_command_table = true;

// get number of commands in table
static size_t getCommandTableSize(const ChatCommand* commands)
{
    if (!commands)
        return 0;
    size_t count = 0;
    while (commands[count].Name != NULL)
        count++;
    return count;
}

// append source command table to target, return number of appended commands
static size_t appendCommandTable(ChatCommand* target, const ChatCommand* source)
{
    const size_t count = getCommandTableSize(source);
    if (count)
        memcpy(target, source, count * sizeof(ChatCommand));
    return count;
}

ChatCommand* ChatHandler::getCommandTable()
{
    // cache for commands, needed because some commands are loaded dynamically through ScriptMgr
    // cache is never freed and will show as a memory leak in diagnostic tools
    // can't use vector as vector storage is implementation-dependent, eg, there can be alignment gaps between elements
    static ChatCommand* commandTableCache = NULL;

    if (LoadCommandTable())
    {
        SetLoadCommandTable(false);

        {
            // count total number of top-level commands
            size_t total = 0;
            std::vector<ChatCommand*> const& dynamic = sScriptMgr->GetChatCommands();
            for (std::vector<ChatCommand*>::const_iterator it = dynamic.begin(); it != dynamic.end(); ++it)
                total += getCommandTableSize(*it);
            total += 1; // ending zero

            // cache top-level commands
            size_t added = 0;
            commandTableCache = (ChatCommand*)malloc(sizeof(ChatCommand) * total);
            memset(commandTableCache, 0, sizeof(ChatCommand) * total);
            ACE_ASSERT(commandTableCache);
            for (std::vector<ChatCommand*>::const_iterator it = dynamic.begin(); it != dynamic.end(); ++it)
                added += appendCommandTable(commandTableCache + added, *it);
        }

        PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_COMMANDS);
        PreparedQueryResult result = WorldDatabase.Query(stmt);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                std::string name = fields[0].GetString();

                SetDataForCommandInTable(commandTableCache, name.c_str(), fields[1].GetUInt8(), fields[2].GetString(), name);
            }
            while (result->NextRow());
        }
    }

    return commandTableCache;
}

std::string ChatHandler::PGetParseString(int32 entry, ...) const
{
    const char *format = GetTrinityString(entry);
    char str[1024];
    va_list ap;
    va_start(ap, entry);
    vsnprintf(str, 1024, format, ap);
    va_end(ap);
    return std::string(str);
}

const char *ChatHandler::GetTrinityString(int32 entry) const
{
    return m_session->GetTrinityString(entry);
}

bool ChatHandler::isAvailable(ChatCommand const& cmd) const
{
    // check security level only for simple  command (without child commands)
    return m_session->GetSecurity() >= AccountTypes(cmd.SecurityLevel);
}

bool ChatHandler::HasLowerSecurity(Player* target, uint64 guid, bool strong)
{
    WorldSession* target_session = NULL;
    uint32 target_account = 0;

    if (target)
        target_session = target->GetSession();
    else if (guid)
        target_account = sObjectMgr->GetPlayerAccountIdByGUID(guid);

    if (!target_session && !target_account)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return true;
    }

    return HasLowerSecurityAccount(target_session, target_account, strong);
}

bool ChatHandler::HasLowerSecurityAccount(WorldSession* target, uint32 target_account, bool strong)
{
    uint32 target_sec;

    // allow everything from console and RA console
    if (!m_session)
        return false;

    // ignore only for non-players for non strong checks (when allow apply command at least to same sec level)
    if (!AccountMgr::IsPlayerAccount(m_session->GetSecurity()) && !strong && !sWorld->getBoolConfig(CONFIG_GM_LOWER_SECURITY))
        return false;

    if (target)
        target_sec = target->GetSecurity();
    else if (target_account)
        target_sec = AccountMgr::GetSecurity(target_account, realmID);
    else
        return true;                                        // caller must report error for (target == NULL && target_account == 0)

    AccountTypes target_ac_sec = AccountTypes(target_sec);
    if (m_session->GetSecurity() < target_ac_sec || (strong && m_session->GetSecurity() <= target_ac_sec))
    {
        SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
        SetSentErrorMessage(true);
        return true;
    }

    return false;
}

bool ChatHandler::hasStringAbbr(const char* name, const char* part)
{
    // non "" command
    if (*name)
    {
        // "" part from non-"" command
        if (!*part)
            return false;

        while (true)
        {
            if (!*part)
                return true;
            else if (!*name)
                return false;
            else if (tolower(*name) != tolower(*part))
                return false;
            ++name; ++part;
        }
    }
    // allow with any for ""

    return true;
}

void ChatHandler::SendSysMessage(const char *str)
{
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        m_session->SendPacket(&data);
    }

    free(buf);
}

void ChatHandler::SendGlobalSysMessage(const char *str)
{
    // Chat output
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        sWorld->SendGlobalMessage(&data);
    }

    free(buf);
}

void ChatHandler::SendGlobalGMSysMessage(const char *str)
{
    // Chat output
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        sWorld->SendGlobalGMMessage(&data);
     }
    free(buf);
}

void ChatHandler::SendSysMessage(int32 entry)
{
    SendSysMessage(GetTrinityString(entry));
}

void ChatHandler::PSendSysMessage(int32 entry, ...)
{
    const char *format = GetTrinityString(entry);
    va_list ap;
    char str [2048];
    va_start(ap, entry);
    vsnprintf(str, 2048, format, ap);
    va_end(ap);
    SendSysMessage(str);
}

void ChatHandler::PSendSysMessage(const char *format, ...)
{
    va_list ap;
    char str [2048];
    va_start(ap, format);
    vsnprintf(str, 2048, format, ap);
    va_end(ap);
    SendSysMessage(str);
}

bool ChatHandler::ExecuteCommandInTable(ChatCommand* table, const char* text, const std::string& fullcmd)
{
    char const* oldtext = text;
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        cmd += *text;
        ++text;
    }

    while (*text == ' ') ++text;

    for (uint32 i = 0; table[i].Name != NULL; ++i)
    {
        if (!hasStringAbbr(table[i].Name, cmd.c_str()))
            continue;

        bool match = false;
        if (strlen(table[i].Name) > cmd.length())
        {
            for (uint32 j = 0; table[j].Name != NULL; ++j)
            {
                if (!hasStringAbbr(table[j].Name, cmd.c_str()))
                    continue;

                if (strcmp(table[j].Name, cmd.c_str()) != 0)
                    continue;
                else
                {
                    match = true;
                    break;
                }
            }
        }
        if (match)
            continue;

        // select subcommand from child commands list
        if (table[i].ChildCommands != NULL)
        {
            if (!ExecuteCommandInTable(table[i].ChildCommands, text, fullcmd))
            {
                if (text[0] != '\0')
                    SendSysMessage(LANG_NO_SUBCMD);
                else
                    SendSysMessage(LANG_CMD_SYNTAX);

                ShowHelpForCommand(table[i].ChildCommands, text);
            }

            return true;
        }

        // must be available and have handler
        if (!table[i].Handler || !isAvailable(table[i]))
            continue;

        SetSentErrorMessage(false);
        // table[i].Name == "" is special case: send original command to handler
        if ((table[i].Handler)(this, table[i].Name[0] != '\0' ? text : oldtext))
        {
            if (!AccountMgr::IsPlayerAccount(table[i].SecurityLevel))
            {
                // chat case
                if (m_session)
                {
                    Player* p = m_session->GetPlayer();
                    Player* selected = p->GetSelectedPlayer();
                    uint32 sel_guid = 0;
                    std::string sel_name = "ERROR";
                    uint32 sel_acc_guid = 0;

                    if (selected)
                    {
                        sel_guid = selected->GetGUIDLow();
                        sel_name = selected->GetName();
                        sel_acc_guid = selected->GetSession()->GetAccountId();
                    }

                    sLog->outCommand(m_session->GetAccountId(), "", m_session->GetPlayer()->GetGUIDLow(), m_session->GetPlayer()->GetName(),
                                    sel_acc_guid, "", sel_guid, sel_name.c_str(),
                                    "Command: %s [Player: %s (Account: %u) X: %f Y: %f Z: %f Map: %u Selected: %s (GUID: %u)]",
                                    fullcmd.c_str(), p->GetName(), m_session->GetAccountId(), p->GetPositionX(), p->GetPositionY(), p->GetPositionZ(), p->GetMapId(),
                                    sel_name.c_str(), sel_guid);
                }
            }
        }
        // some commands have custom error messages. Don't send the default one in these cases.
        else if (!HasSentErrorMessage())
        {
            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());
            else
                SendSysMessage(LANG_CMD_SYNTAX);
        }

        return true;
    }

    return false;
}

bool ChatHandler::SetDataForCommandInTable(ChatCommand* table, const char* text, uint32 security, std::string const& help, std::string const& fullcommand)
{
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        cmd += *text;
        ++text;
    }

    while (*text == ' ') ++text;

    for (uint32 i = 0; table[i].Name != NULL; i++)
    {
        // for data fill use full explicit command names
        if (table[i].Name != cmd)
            continue;

        // select subcommand from child commands list (including "")
        if (table[i].ChildCommands != NULL)
        {
            if (SetDataForCommandInTable(table[i].ChildCommands, text, security, help, fullcommand))
                return true;
            else if (*text)
                return false;

            // fail with "" subcommands, then use normal level up command instead
        }
        // expected subcommand by full name DB content
        else if (*text)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `command` has unexpected subcommand '%s' in command '%s', skip.", text, fullcommand.c_str());
            return false;
        }

        if (table[i].SecurityLevel != security)
            sLog->outInfo(LOG_FILTER_GENERAL, "Table `command` overwrite for command '%s' default security (%u) by %u", fullcommand.c_str(), table[i].SecurityLevel, security);

        table[i].SecurityLevel = security;
        table[i].Help          = help;
        return true;
    }

    // in case "" command let process by caller
    if (!cmd.empty())
    {
        if (table == getCommandTable())
            sLog->outError(LOG_FILTER_SQL, "Table `command` has non - existing command '%s', skip.", cmd.c_str());
        else
            sLog->outError(LOG_FILTER_SQL, "Table `command` has non - existing subcommand '%s' in command '%s', skip.", cmd.c_str(), fullcommand.c_str());
    }

    return false;
}

int ChatHandler::ParseCommands(const char* text)
{
    ASSERT(text);
    ASSERT(*text);

    std::string fullcmd = text;

    if (m_session && AccountMgr::IsPlayerAccount(m_session->GetSecurity()) && !sWorld->getBoolConfig(CONFIG_ALLOW_PLAYER_COMMANDS))
       return 0;

    /// chat case (.command or !command format)
    if (m_session)
    {
        if (text[0] != '!' && text[0] != '.')
            return 0;
    }

    /// ignore single . and ! in line
    if (strlen(text) < 2)
        return 0;
    // original `text` can't be used. It content destroyed in command code processing.

    /// ignore messages staring from many dots.
    if ((text[0] == '.' && text[1] == '.') || (text[0] == '!' && text[1] == '!'))
        return 0;

    /// skip first . or ! (in console allowed use command with . and ! and without its)
    if (text[0] == '!' || text[0] == '.')
        ++text;

    if (!ExecuteCommandInTable(getCommandTable(), text, fullcmd))
    {
        if (m_session && AccountMgr::IsPlayerAccount(m_session->GetSecurity()))
            return 0;

        SendSysMessage(LANG_NO_CMD);
    }
    return 1;
}

bool ChatHandler::isValidChatMessage(const char* message)
{
/*
Valid examples:
|cffa335ee|Hitem:812:0:0:0:0:0:0:0:70|h[Glowing Brightwood Staff]|h|r
|cff808080|Hquest:2278:47|h[The Platinum Discs]|h|r
|cffffd000|Htrade:4037:1:150:1:6AAAAAAAAAAAAAAAAAAAAAAOAADAAAAAAAAAAAAAAAAIAAAAAAAAA|h[Engineering]|h|r
|cff4e96f7|Htalent:2232:-1|h[Taste for Blood]|h|r
|cff71d5ff|Hspell:21563|h[Command]|h|r
|cffffd000|Henchant:3919|h[Engineering: Rough Dynamite]|h|r
|cffffff00|Hachievement:546:0000000000000001:0:0:0:-1:0:0:0:0|h[Safe Deposit]|h|r
|cff66bbff|Hglyph:21:762|h[Glyph of Bladestorm]|h|r

| will be escaped to ||
*/

    if (strlen(message) > 255)
        return false;

    // more simple checks
    if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_SEVERITY) < 3)
    {
        const char validSequence[6] = "cHhhr";
        const char* validSequenceIterator = validSequence;
        const std::string validCommands = "cHhr|";

        while (*message)
        {
            // find next pipe command
            message = strchr(message, '|');

            if (!message)
                return true;

            ++message;
            char commandChar = *message;
            if (validCommands.find(commandChar) == std::string::npos)
                return false;

            ++message;
            // validate sequence
            if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_SEVERITY) == 2)
            {
                if (commandChar == *validSequenceIterator)
                {
                    if (validSequenceIterator == validSequence + 4)
                        validSequenceIterator = validSequence;
                    else
                        ++validSequenceIterator;
                }
                else
                    return false;
            }
        }
        return true;
    }

    return LinkExtractor(message).IsValidMessage();
}

bool ChatHandler::ShowHelpForSubCommands(ChatCommand* table, char const* cmd, char const* subcmd)
{
    std::string list;
    for (uint32 i = 0; table[i].Name != NULL; ++i)
    {
        // must be available (ignore handler existence for show command with possible available subcommands)
        if (!isAvailable(table[i]))
            continue;

        // for empty subcmd show all available
        if (*subcmd && !hasStringAbbr(table[i].Name, subcmd))
            continue;

        if (m_session)
            list += "\n    ";
        else
            list += "\n\r    ";

        list += table[i].Name;

        if (table[i].ChildCommands)
            list += " ...";
    }

    if (list.empty())
        return false;

    if (table == getCommandTable())
    {
        SendSysMessage(LANG_AVIABLE_CMD);
        PSendSysMessage("%s", list.c_str());
    }
    else
        PSendSysMessage(LANG_SUBCMDS_LIST, cmd, list.c_str());

    return true;
}

bool ChatHandler::ShowHelpForCommand(ChatCommand* table, const char* cmd)
{
    if (*cmd)
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possible available subcommands)
            if (!isAvailable(table[i]))
                continue;

            if (!hasStringAbbr(table[i].Name, cmd))
                continue;

            // have subcommand
            char const* subcmd = (*cmd) ? strtok(NULL, " ") : "";

            if (table[i].ChildCommands && subcmd && *subcmd)
            {
                if (ShowHelpForCommand(table[i].ChildCommands, subcmd))
                    return true;
            }

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands, table[i].Name, subcmd ? subcmd : ""))
                    return true;

            return !table[i].Help.empty();
        }
    }
    else
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possible available subcommands)
            if (!isAvailable(table[i]))
                continue;

            if (strlen(table[i].Name))
                continue;

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands, "", ""))
                    return true;

            return !table[i].Help.empty();
        }
    }

    return ShowHelpForSubCommands(table, "", cmd);
}

// Contains: Packet data, Player session, Message type, Message language, Channel name, Receiver GUID, Message text, Creature speaker, Addon prefix, Chat tag, Achievement id, Creature localized name.
void ChatHandler::FillMessageData(WorldPacket* data, WorldSession* session, uint8 type, uint32 language, const char* channelName, uint64 target_guid, const char* message, Unit* speaker, const char* addonPrefix /*= NULL*/, uint8 chatTag /*= CHAT_TAG_NONE*/, uint32 achievementId /*= 0*/, const char* localizedName /*= NULL*/)
{
    /*** Some assignments and checks used as safety. ***/

    // Set message length.
    uint32 messageLength = message ? strlen(message) : 0;

    // Set channel name length.
    uint32 channelNameLength = channelName ? strlen(channelName) : 0;

    // Set addon prefix length.
    uint32 addonPrefixLength = addonPrefix ? strlen(addonPrefix) : 0;

    // Set language used.
    language = ((type != CHAT_MSG_CHANNEL && type != CHAT_MSG_WHISPER) || language == LANG_ADDON) ? language : LANG_UNIVERSAL;

    // Build correct target GUID.
    Player* speakerPlayer = NULL;
    if (speaker && speaker->GetTypeId() == TYPEID_PLAYER)
        speakerPlayer = speaker->ToPlayer();
    else
    {
        if (session)
            speakerPlayer = session->GetPlayer();
    }

    // Build proper data depending on message type.
    switch (type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_PARTY_LEADER:
        case CHAT_MSG_RAID:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_YELL:
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_CHANNEL:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE:
        case CHAT_MSG_INSTANCE_CHAT:
        case CHAT_MSG_INSTANCE_CHAT_LEADER:
            // target_guid controls chat bubbles and receiver message building.
            if (!target_guid) target_guid = speakerPlayer ? speakerPlayer->GetGUID() : 0; // Original target_guid preserved for certain message types (ex. CHAT_MSG_WHISPER_INFORM).
            break;
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_MONSTER_EMOTE:
            // // target_guid controls chat bubbles and receiver message building. - No need for enforcing this here anymore, handling is done by specific function argumentation.
            // if (!target_guid) target_guid = speaker ? speaker->GetGUID() : 0; // Original target_guid still preserved for certain message types (see below).
            // break;
        case CHAT_MSG_MONSTER_WHISPER:   // Should already have a target guid / target guid not needed for entire raid sending.
        case CHAT_MSG_RAID_BOSS_WHISPER: // Should already have a target guid / target guid not needed for entire raid sending.
        case CHAT_MSG_RAID_BOSS_EMOTE:   // Should already have a target guid / target guid not needed for entire raid sending.
            break;

        default: break;
    }

    // Set speaker name length and get the speaker name.
    uint32 speakerNameLength = 0;
    if (speaker)
        speakerNameLength = localizedName ? strlen(localizedName) : strlen(speaker->GetName());
    else
    {
        if (session)
            speakerNameLength = strlen(session->GetPlayer()->GetName());
    }

    std::string speakerName;
    if (speaker)
        speakerName = localizedName ? localizedName : speaker->GetName();
    else
    {
        if (session)
            speakerName = session->GetPlayer()->GetName();
    }

    // Set Chat tags.
    if (speaker)
    {
        if (speaker->GetTypeId() == TYPEID_UNIT)
            chatTag = 32;                                // Seems like all creature chats have this tag (Taken from sniffs).
        else if (speaker->GetTypeId() == TYPEID_PLAYER)
            chatTag = speaker->ToPlayer()->GetChatTag(); // Player chat tag.
    }
    else
    {
        if (session)
            chatTag = session->GetPlayer()->GetChatTag();
    }

    // Set target name length and get the target name.
    uint32 targetNameLength = 0;
    std::string targetName;
    if (target_guid)
    {
        if (Unit* unit = ObjectAccessor::FindUnit(target_guid))
        {
            targetNameLength = strlen(unit->GetName());
            targetName = unit->GetName();
        }
    }

    /*** Packet building. ***/

    // First establish what GUIDs to use.
    ObjectGuid sourceGuid = speaker ? speaker->GetGUID() : (session ? session->GetPlayer()->GetGUID() : 0);
    ObjectGuid targetGuid = target_guid;

    ObjectGuid groupGuid = 0;
    if (type == CHAT_MSG_PARTY   || type == CHAT_MSG_PARTY_LEADER
        || type == CHAT_MSG_RAID || type == CHAT_MSG_RAID_LEADER || type == CHAT_MSG_RAID_WARNING
        || type == CHAT_MSG_INSTANCE_CHAT || type == CHAT_MSG_INSTANCE_CHAT_LEADER)
        groupGuid = (speakerPlayer && speakerPlayer->GetGroup()) ? speakerPlayer->GetGroup()->GetGUID() : 0;

    ObjectGuid guildGuid = 0;
    if (type == CHAT_MSG_GUILD || type == CHAT_MSG_OFFICER)
        guildGuid = (speakerPlayer && speakerPlayer->GetGuild()) ? speakerPlayer->GetGuild()->GetGUID() : 0;

    // Then establish the variables needed.
    bool HasGuildGUID = guildGuid ? true : false;
    bool HasGroupGUID = groupGuid ? true : false;
    bool HasSpeakerGUID = sourceGuid ? true : false;
    bool HasReceiverGUID = targetGuid ? true : false;
    bool HasSpeaker = (sourceGuid && speakerNameLength > 0) ? true : false;
    bool HasReceiver = (targetGuid && targetNameLength > 0) ? true : false;
    bool HasMessage = (messageLength > 0) ? true : false;
    bool HasLanguage = (language > LANG_UNIVERSAL) ? true : false;
    bool HasChannel = (type == CHAT_MSG_CHANNEL && channelNameLength > 0) ? true : false;
    bool HasAddonPrefix = (addonPrefixLength > 0) ? true : false;
    bool HasAchievement = ((type == CHAT_MSG_ACHIEVEMENT || type == CHAT_MSG_GUILD_ACHIEVEMENT) && achievementId > 0) ? true : false;
    bool HasChatTag = (chatTag > CHAT_TAG_NONE) ? true : false;
    bool HasConstantTime = true;       // This represents the current time (or the time at which the text is sent).
    bool ShowInChatBubble = true;      // Toggle show in chat window - show in chat bubble.
    bool HasSecondTime = true;         // This is in relation to HasConstantTime. Represents text duration and is sent as HasConstantTime + text duration. !ToDo: Implement.
    bool HasLimitedFloatRange = false; // This represents the distance at which the chat can be "heard / read", and is already limited sv-side throughout the core. !ToDo: Implement.

    // Now build the actual packet.
    data->Initialize(SMSG_MESSAGE_CHAT);

    data->WriteBit(!HasGuildGUID);                              // Inversed - Has guild Guid - FAKE GUID.
    data->WriteBit(!HasSpeakerGUID);                            // Inversed - Has speaker Guid (No speaker Guid in stuff like BG announcements) - FAKE GUID.

    uint8 bitsOrderGuild[8] = { 4, 5, 1, 0, 2, 6, 7, 3 };
    data->WriteBitInOrder(guildGuid, bitsOrderGuild);

    data->WriteBit(!HasChatTag);                                // Inversed - Has chat tag.
    data->WriteBit(!HasLanguage);                               // Inversed - Has language.
    
    uint8 bitsOrderSource[8] = { 2, 7, 0, 3, 4, 6, 1, 5 };
    data->WriteBitInOrder(sourceGuid, bitsOrderSource);

    data->WriteBit(!ShowInChatBubble);                          // Inversed - Show just in text box / Also in bubble.
    data->WriteBit(!HasAchievement);                            // Inversed - Has achievement.
    data->WriteBit(!HasReceiver);                               // Inversed - Has target receiver.
    data->WriteBit(!HasSpeaker);                                // Inversed - Has sender.
    data->WriteBit(!HasMessage);                                // Inversed - Has text.

    data->WriteBit(!HasReceiverGUID);                           // Inversed - Has target receiver GUID - FAKE GUID.

    uint8 bitsOrderTarget[8] = { 5, 7, 6, 4, 3, 2, 1, 0 };
    data->WriteBitInOrder(targetGuid, bitsOrderTarget);

    data->WriteBit(!HasSecondTime);                             // Inversed - Has second text time.

    if (HasReceiver)
        data->WriteBits(targetNameLength, 11);

    if (HasSpeaker)
        data->WriteBits(speakerNameLength, 11);

    data->WriteBit(!HasGroupGUID);                              // Inversed - Has group Guid - FAKE GUID.

    uint8 bitsOrderGroup[8] = { 5, 2, 6, 1, 7, 3, 0, 4 };
    data->WriteBitInOrder(groupGuid, bitsOrderGroup);

    data->WriteBit(!HasLimitedFloatRange);                      // Inversed - Has limited range.

    if (HasChatTag)
        data->WriteBits(chatTag, 9);

    if (HasMessage)
        data->WriteBits(messageLength, 12);

    data->WriteBit(0);                                          // Unknown byte1499. Seems related to B-Net Chat. Always false.
    data->WriteBit(!HasAddonPrefix);                            // Inversed - Has addon prefix.
    data->WriteBit(!HasChannel);                                // Inversed - Has chat channel.

    if (HasAddonPrefix)
        data->WriteBits(addonPrefixLength, 5);

    if (HasChannel)
        data->WriteBits(channelNameLength, 7);

    data->WriteBit(!HasConstantTime);                           // Inversed - Has first, constant text time.

    data->FlushBits();

    uint8 byteOrderGuild[8] = { 7, 2, 1, 4, 6, 5, 3, 0 };
    data->WriteBytesSeq(guildGuid, byteOrderGuild);

    uint8 byteOrderGroup[8] = { 5, 3, 2, 4, 1, 0, 7, 6 };
    data->WriteBytesSeq(groupGuid, byteOrderGroup);

    *data << uint8(type);

    if (HasSecondTime)
        *data << uint32(time(NULL));  // Add text duration here for creatures (HasConstantTime + duration; check CreatureTextMgr packet building).

    if (HasAddonPrefix)
        data->append(addonPrefix, addonPrefixLength);

    if (HasLimitedFloatRange)
        *data << float(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY)); // Add max text range here (see building of each chat type - new Cata fields for ex. RAID_BOSS_EMOTE). 

    uint8 byteOrderTarget[8] = { 4, 2, 3, 0, 6, 7, 5, 1 };
    data->WriteBytesSeq(targetGuid, byteOrderTarget);

    uint8 byteOrderSource[8] = { 6, 1, 0, 2, 4, 5, 7, 3 };
    data->WriteBytesSeq(sourceGuid, byteOrderSource);

    if (HasAchievement)
        *data << uint32(achievementId);

    if (HasReceiver)
        data->append(targetName.c_str(), targetNameLength);

    if (HasMessage)
        data->append(message, messageLength);

    if (HasSpeaker)
        data->append(speakerName.c_str(), speakerNameLength);

    if (HasLanguage)
        *data << uint8(language);

    if (HasChannel)
        data->append(channelName, channelNameLength);

    if (HasConstantTime)
        *data << uint32(time(NULL));                                     // The time at which the creature text is called.
}

Player* ChatHandler::getSelectedPlayer()
{
    if (!m_session)
        return NULL;

    uint64 guid  = m_session->GetPlayer()->GetSelection();

    if (guid == 0)
        return m_session->GetPlayer();

    return ObjectAccessor::FindPlayer(guid);
}

Unit* ChatHandler::getSelectedUnit()
{
    if (!m_session)
        return NULL;

    uint64 guid = m_session->GetPlayer()->GetSelection();

    if (guid == 0)
        return m_session->GetPlayer();

    return ObjectAccessor::GetUnit(*m_session->GetPlayer(), guid);
}

WorldObject* ChatHandler::getSelectedObject()
{
    if (!m_session)
        return NULL;

    uint64 guid = m_session->GetPlayer()->GetSelection();

    if (guid == 0)
        return GetNearbyGameObject();

    return ObjectAccessor::GetUnit(*m_session->GetPlayer(), guid);
}

Creature* ChatHandler::getSelectedCreature()
{
    if (!m_session)
        return NULL;

    return ObjectAccessor::GetCreatureOrPetOrVehicle(*m_session->GetPlayer(), m_session->GetPlayer()->GetSelection());
}

char* ChatHandler::extractKeyFromLink(char* text, char const* linkType, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text == ' '||*text == '\t'||*text == '\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r

    char* check = strtok(text, "|");                        // skip color
    if (!check)
        return NULL;                                        // end of data

    char* cLinkType = strtok(NULL, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    if (strcmp(cLinkType, linkType) != 0)
    {
        strtok(NULL, " ");                                  // skip link tail (to allow continue strtok(NULL, s) use after retturn from function
        SendSysMessage(LANG_WRONG_LINK_TYPE);
        return NULL;
    }

    char* cKeys = strtok(NULL, "|");                        // extract keys and values
    char* cKeysTail = strtok(NULL, "");

    char* cKey = strtok(cKeys, ":|");                       // extract key
    if (something1)
        *something1 = strtok(NULL, ":|");                   // extract something

    strtok(cKeysTail, "]");                                 // restart scan tail and skip name with possible spaces
    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL, s) use after return from function
    return cKey;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text == ' '||*text == '\t'||*text == '\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r
    // or
    // [name] Shift-click form |linkType:key|h[name]|h|r

    char* tail;

    if (text[1] == 'c')
    {
        char* check = strtok(text, "|");                    // skip color
        if (!check)
            return NULL;                                    // end of data

        tail = strtok(NULL, "");                            // tail
    }
    else
        tail = text+1;                                      // skip first |

    char* cLinkType = strtok(tail, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    for (int i = 0; linkTypes[i]; ++i)
    {
        if (strcmp(cLinkType, linkTypes[i]) == 0)
        {
            char* cKeys = strtok(NULL, "|");                // extract keys and values
            char* cKeysTail = strtok(NULL, "");

            char* cKey = strtok(cKeys, ":|");               // extract key
            if (something1)
                *something1 = strtok(NULL, ":|");           // extract something

            strtok(cKeysTail, "]");                         // restart scan tail and skip name with possible spaces
            strtok(NULL, " ");                              // skip link tail (to allow continue strtok(NULL, s) use after return from function
            if (found_idx)
                *found_idx = i;
            return cKey;
        }
    }

    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL, s) use after return from function
    SendSysMessage(LANG_WRONG_LINK_TYPE);
    return NULL;
}

GameObject* ChatHandler::GetNearbyGameObject()
{
    if (!m_session)
        return NULL;

    Player* pl = m_session->GetPlayer();
    GameObject* obj = NULL;
    JadeCore::NearestGameObjectCheck check(*pl);
    JadeCore::GameObjectLastSearcher<JadeCore::NearestGameObjectCheck> searcher(pl, obj, check);
    pl->VisitNearbyGridObject(SIZE_OF_GRIDS, searcher);
    return obj;
}

GameObject* ChatHandler::GetObjectGlobalyWithGuidOrNearWithDbGuid(uint32 lowguid, uint32 entry)
{
    if (!m_session)
        return NULL;

    Player* pl = m_session->GetPlayer();

    GameObject* obj = pl->GetMap()->GetGameObject(MAKE_NEW_GUID(lowguid, entry, HIGHGUID_GAMEOBJECT));

    if (!obj && sObjectMgr->GetGOData(lowguid))                   // guid is DB guid of object
    {
        // search near player then
        CellCoord p(JadeCore::ComputeCellCoord(pl->GetPositionX(), pl->GetPositionY()));
        Cell cell(p);

        JadeCore::GameObjectWithDbGUIDCheck go_check(*pl, lowguid);
        JadeCore::GameObjectSearcher<JadeCore::GameObjectWithDbGUIDCheck> checker(pl, obj, go_check);

        TypeContainerVisitor<JadeCore::GameObjectSearcher<JadeCore::GameObjectWithDbGUIDCheck>, GridTypeMapContainer > object_checker(checker);
        cell.Visit(p, object_checker, *pl->GetMap(), *pl, pl->GetGridActivationRange());
    }

    return obj;
}

enum SpellLinkType
{
    SPELL_LINK_SPELL   = 0,
    SPELL_LINK_TALENT  = 1,
    SPELL_LINK_ENCHANT = 2,
    SPELL_LINK_TRADE   = 3,
    SPELL_LINK_GLYPH   = 4
};

static char const* const spellKeys[] =
{
    "Hspell",                                               // normal spell
    "Htalent",                                              // talent spell
    "Henchant",                                             // enchanting recipe spell
    "Htrade",                                               // profession/skill spell
    "Hglyph",                                               // glyph
    0
};

uint32 ChatHandler::extractSpellIdFromLink(char* text)
{
    // number or [name] Shift-click form |color|Henchant:recipe_spell_id|h[prof_name: recipe_name]|h|r
    // number or [name] Shift-click form |color|Hglyph:glyph_slot_id:glyph_prop_id|h[%s]|h|r
    // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
    // number or [name] Shift-click form |color|Htalent:talent_id, rank|h[name]|h|r
    // number or [name] Shift-click form |color|Htrade:spell_id, skill_id, max_value, cur_value|h[name]|h|r
    int type = 0;
    char* param1_str = NULL;
    char* idS = extractKeyFromLink(text, spellKeys, &type, &param1_str);
    if (!idS)
        return 0;

    uint32 id = (uint32)atol(idS);

    switch (type)
    {
        case SPELL_LINK_SPELL:
            return id;
        /*case SPELL_LINK_TALENT:
        {
            // talent
            TalentEntry const* talentEntry = sTalentStore.LookupEntry(id);
            if (!talentEntry)
                return 0;

            int32 rank = param1_str ? (uint32)atol(param1_str) : 0;
            if (rank >= MAX_TALENT_RANK)
                return 0;

            if (rank < 0)
                rank = 0;

            return talentEntry->RankID[rank];
        }*/
        case SPELL_LINK_ENCHANT:
        case SPELL_LINK_TRADE:
            return id;
        case SPELL_LINK_GLYPH:
        {
            uint32 glyph_prop_id = param1_str ? (uint32)atol(param1_str) : 0;

            GlyphPropertiesEntry const* glyphPropEntry = sGlyphPropertiesStore.LookupEntry(glyph_prop_id);
            if (!glyphPropEntry)
                return 0;

            return glyphPropEntry->SpellId;
        }
    }

    // unknown type?
    return 0;
}

GameTele const* ChatHandler::extractGameTeleFromLink(char* text)
{
    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    char* cId = extractKeyFromLink(text, "Htele");
    if (!cId)
        return NULL;

    // id case (explicit or from shift link)
    if (cId[0] >= '0' || cId[0] >= '9')
        if (uint32 id = atoi(cId))
            return sObjectMgr->GetGameTele(id);

    return sObjectMgr->GetGameTele(cId);
}

enum GuidLinkType
{
    SPELL_LINK_PLAYER     = 0,                              // must be first for selection in not link case
    SPELL_LINK_CREATURE   = 1,
    SPELL_LINK_GAMEOBJECT = 2
};

static char const* const guidKeys[] =
{
    "Hplayer",
    "Hcreature",
    "Hgameobject",
    0
};

uint64 ChatHandler::extractGuidFromLink(char* text)
{
    int type = 0;

    // |color|Hcreature:creature_guid|h[name]|h|r
    // |color|Hgameobject:go_guid|h[name]|h|r
    // |color|Hplayer:name|h[name]|h|r
    char* idS = extractKeyFromLink(text, guidKeys, &type);
    if (!idS)
        return 0;

    switch (type)
    {
        case SPELL_LINK_PLAYER:
        {
            std::string name = idS;
            if (!normalizePlayerName(name))
                return 0;

            if (Player* player = sObjectAccessor->FindPlayerByName(name.c_str()))
                return player->GetGUID();

            if (uint64 guid = sObjectMgr->GetPlayerGUIDByName(name))
                return guid;

            return 0;
        }
        case SPELL_LINK_CREATURE:
        {
            uint32 lowguid = (uint32)atol(idS);

            if (CreatureData const* data = sObjectMgr->GetCreatureData(lowguid))
                return MAKE_NEW_GUID(lowguid, data->id, HIGHGUID_UNIT);
            else
                return 0;
        }
        case SPELL_LINK_GAMEOBJECT:
        {
            uint32 lowguid = (uint32)atol(idS);

            if (GameObjectData const* data = sObjectMgr->GetGOData(lowguid))
                return MAKE_NEW_GUID(lowguid, data->id, HIGHGUID_GAMEOBJECT);
            else
                return 0;
        }
    }

    // unknown type?
    return 0;
}

std::string ChatHandler::extractPlayerNameFromLink(char* text)
{
    // |color|Hplayer:name|h[name]|h|r
    char* name_str = extractKeyFromLink(text, "Hplayer");
    if (!name_str)
        return "";

    std::string name = name_str;
    if (!normalizePlayerName(name))
        return "";

    return name;
}

bool ChatHandler::extractPlayerTarget(char* args, Player** player, uint64* player_guid /*=NULL*/, std::string* player_name /*= NULL*/)
{
    if (args && *args)
    {
        std::string name = extractPlayerNameFromLink(args);
        if (name.empty())
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        Player* pl = sObjectAccessor->FindPlayerByName(name.c_str());

        // if allowed player pointer
        if (player)
            *player = pl;

        // if need guid value from DB (in name case for check player existence)
        uint64 guid = !pl && (player_guid || player_name) ? sObjectMgr->GetPlayerGUIDByName(name) : 0;

        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetGUID() : guid;

        if (player_name)
            *player_name = pl || guid ? name : "";
    }
    else
    {
        Player* pl = getSelectedPlayer();
        // if allowed player pointer
        if (player)
            *player = pl;
        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetGUID() : 0;

        if (player_name)
            *player_name = pl ? pl->GetName() : "";
    }

    // some from req. data must be provided (note: name is empty if player not exist)
    if ((!player || !*player) && (!player_guid || !*player_guid) && (!player_name || player_name->empty()))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

void ChatHandler::extractOptFirstArg(char* args, char** arg1, char** arg2)
{
    char* p1 = strtok(args, " ");
    char* p2 = strtok(NULL, " ");

    if (!p2)
    {
        p2 = p1;
        p1 = NULL;
    }

    if (arg1)
        *arg1 = p1;

    if (arg2)
        *arg2 = p2;
}

char* ChatHandler::extractQuotedArg(char* args)
{
    if (!*args)
        return NULL;

    if (*args == '"')
        return strtok(args+1, "\"");
    else
    {
        char* space = strtok(args, "\"");
        if (!space)
            return NULL;
        return strtok(NULL, "\"");
    }
}

bool ChatHandler::needReportToTarget(Player* chr) const
{
    Player* pl = m_session->GetPlayer();
    return pl != chr && pl->IsVisibleGloballyFor(chr);
}

LocaleConstant ChatHandler::GetSessionDbcLocale() const
{
    return m_session->GetSessionDbcLocale();
}

int ChatHandler::GetSessionDbLocaleIndex() const
{
    return m_session->GetSessionDbLocaleIndex();
}

const char *CliHandler::GetTrinityString(int32 entry) const
{
    return sObjectMgr->GetTrinityStringForDBCLocale(entry);
}

bool CliHandler::isAvailable(ChatCommand const& cmd) const
{
    // skip non-console commands in console case
    return cmd.AllowConsole;
}

void CliHandler::SendSysMessage(const char *str)
{
    m_print(m_callbackArg, str);
    m_print(m_callbackArg, "\r\n");
}

std::string CliHandler::GetNameLink() const
{
    return GetTrinityString(LANG_CONSOLE_COMMAND);
}

bool CliHandler::needReportToTarget(Player* /*chr*/) const
{
    return true;
}

bool ChatHandler::GetPlayerGroupAndGUIDByName(const char* cname, Player* &player, Group* &group, uint64 &guid, bool offline)
{
    player  = NULL;
    guid = 0;

    if (cname)
    {
        std::string name = cname;
        if (!name.empty())
        {
            if (!normalizePlayerName(name))
            {
                PSendSysMessage(LANG_PLAYER_NOT_FOUND);
                SetSentErrorMessage(true);
                return false;
            }

            player = sObjectAccessor->FindPlayerByName(name.c_str());
            if (offline)
                guid = sObjectMgr->GetPlayerGUIDByName(name.c_str());
        }
    }

    if (player)
    {
        group = player->GetGroup();
        if (!guid || !offline)
            guid = player->GetGUID();
    }
    else
    {
        if (getSelectedPlayer())
            player = getSelectedPlayer();
        else
            player = m_session->GetPlayer();

        if (!guid || !offline)
            guid  = player->GetGUID();
        group = player->GetGroup();
    }

    return true;
}

LocaleConstant CliHandler::GetSessionDbcLocale() const
{
    return sWorld->GetDefaultDbcLocale();
}

int CliHandler::GetSessionDbLocaleIndex() const
{
    return sObjectMgr->GetDBCLocaleIndex();
}
