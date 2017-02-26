/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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
#include "CharacterDatabaseCleaner.h"
#include "World.h"
#include "Database/DatabaseEnv.h"
#include "SpellMgr.h"
#include "DBCStores.h"
#include "AchievementMgr.h"

#define SKILL_MOUNT     777
#define SKILL_MINIPET   778

void CharacterDatabaseCleaner::CleanDatabase()
{
    // config to disable
    if (!sWorld->getBoolConfig(CONFIG_CLEAN_CHARACTER_DB))
        return;

    sLog->outInfo(LOG_FILTER_GENERAL, "Cleaning character database...");

    uint32 oldMSTime = getMSTime();

    // check flags which clean ups are necessary
    QueryResult result = CharacterDatabase.Query("SELECT value FROM worldstates WHERE entry = 20004");
    if (!result)
        return;

    uint32 flags = (*result)[0].GetUInt32();

    // clean up
    if (flags & CLEANING_FLAG_ACHIEVEMENT_PROGRESS)
        CleanCharacterAchievementProgress();

    if (flags & CLEANING_FLAG_SKILLS)
        CleanCharacterSkills();

    if (flags & CLEANING_FLAG_SPELLS)
        CleanCharacterSpell();

    if (flags & CLEANING_FLAG_TALENTS)
        CleanCharacterTalent();

    if (flags & CLEANING_FLAG_QUESTSTATUS)
        CleanCharacterQuestStatus();

    if (flags & CLEANING_FLAG_CLASS_SPELLS)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT DISTINCT spell FROM character_spell");
        if (result)
        {
            bool found = false;
            std::ostringstream ss;
            do
            {
                Field* fields = result->Fetch();

                uint32 spell_id = fields[0].GetUInt32();

                const SpellInfo* info = sSpellMgr->GetSpellInfo(spell_id);
                if (!info)
                {
                    if (!found)
                    {
                        ss << "DELETE FROM character_spell WHERE spell IN(";
                        found = true;
                    }
                    else
                        ss << ',';

                    ss << spell_id;

                    continue;
                }

                switch (info->SpellFamilyName)
                {
                case SPELLFAMILY_MAGE:
                case SPELLFAMILY_WARRIOR:
                case SPELLFAMILY_WARLOCK:
                case SPELLFAMILY_PRIEST:
                case SPELLFAMILY_DRUID:
                case SPELLFAMILY_ROGUE:
                case SPELLFAMILY_HUNTER:
                case SPELLFAMILY_PALADIN:
                case SPELLFAMILY_SHAMAN:
                case SPELLFAMILY_DEATHKNIGHT:
                case SPELLFAMILY_PET:
                case SPELLFAMILY_MONK:
                {
                    if (!found)
                    {
                        ss << "DELETE FROM character_spell WHERE spell IN(";
                        found = true;
                    }
                    else
                        ss << ',';

                    ss << spell_id;

                    continue;
                }
                break;
                default:
                    break;
                }
            } while (result->NextRow());

            if (found)
            {
                ss << ')';
                CharacterDatabase.Execute(ss.str().c_str());
            }
        }
        else
        {
            sLog->outInfo(LOG_FILTER_GENERAL, "Table character_spell is empty.");
        }
    }

    if (flags & CLEANING_FLAG_ACCOUNT_SPELLS)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT DISTINCT spell FROM character_spell");
        if (result)
        {
            bool found = false;
            std::ostringstream ss;
            do
            {
                Field* fields = result->Fetch();

                uint32 spell_id = fields[0].GetUInt32();

                const SpellInfo* info = sSpellMgr->GetSpellInfo(spell_id);
                if (info && ((info->IsAbilityOfSkillType(SKILL_MOUNT) && !(info->AttributesEx10 & SPELL_ATTR10_MOUNT_CHARACTER)) || info->IsAbilityOfSkillType(SKILL_MINIPET)))
                {
                    if (!found)
                    {
                        ss << "DELETE FROM character_spell WHERE spell IN(";
                        found = true;
                    }
                    else
                        ss << ',';

                    ss << spell_id;

                    QueryResult accountsWithSpellResult = CharacterDatabase.PQuery(
                        "SELECT DISTINCT account FROM character_spell cs INNER JOIN characters ch ON cs.guid = ch.guid WHERE spell = %d AND active = 1 AND disabled = 0", spell_id);
                    if (accountsWithSpellResult)
                    {
                        std::ostringstream accountSpellInserts;
                        accountSpellInserts << "REPLACE INTO account_spell (accountId, spell, active, disabled) VALUES ";
                        Field* accountFields = accountsWithSpellResult->Fetch();
                        uint32 account = accountFields[0].GetUInt32();
                        accountSpellInserts << "(" << account << "," << spell_id << ", 1, 0)";

                        while (accountsWithSpellResult->NextRow())
                        {
                            accountFields = accountsWithSpellResult->Fetch();
                            account = accountFields[0].GetUInt32();
                            accountSpellInserts << ",(" << account << "," << spell_id << ", 1, 0)";
                        }

                        LoginDatabase.PQuery(accountSpellInserts.str().c_str());
                    }
                }
            } while (result->NextRow());

            if (found)
            {
                ss << ')';
                CharacterDatabase.Execute(ss.str().c_str());
            }
        }
        else
        {
            sLog->outInfo(LOG_FILTER_GENERAL, "Table character_spell is empty.");
        }
    }

    if (flags & CLEANING_FLAG_ACCOUNT_ACHI_PROGRESS)
    {
        QueryResult criteriaResults = CharacterDatabase.PQuery("SELECT DISTINCT criteria FROM character_achievement_progress");
        if (criteriaResults)
        {
            bool found = false;
            std::ostringstream ss;
            do
            {
                Field* fields = criteriaResults->Fetch();
                uint32 critId = fields[0].GetUInt32();

                AchievementCriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(critId);
                if (!criteria)
                    continue;

                AchievementEntry const* achievement = sAchievementMgr->GetAchievement(criteria->achievement);
                if (!achievement || (achievement->flags & ACHIEVEMENT_FLAG_GUILD) || (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT))
                {
                    if (!found)
                    {
                        ss << "DELETE FROM character_achievement_progress WHERE criteria IN(";
                        found = true;
                    }
                    else
                        ss << ',';

                    ss << critId;

                    // move data to account achievement progress table
                    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
                    {
                        switch (criteria->type)
                        {
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                        case ACHIEVEMENT_CRITERIA_TYPE_REACH_BG_RATING:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
                        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
                        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
                        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
                        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
                        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
                        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
                        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
                        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
                        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
                        case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
                        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
                        case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
                        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
                            // these achievement types do not have accumulating values
                            break;
                        default:
                        {
                            CharacterDatabase.PQuery("REPLACE INTO account_achievement_progress "
                                "SELECT account, criteria, SUM(counter) AS counter, MAX(date) AS date FROM "
                                "( "
                                "( "
                                "SELECT account, criteria, SUM(counter) AS counter, MAX(date) AS date "
                                "FROM character_achievement_progress cap INNER JOIN characters ch ON cap.guid = ch.guid "
                                "WHERE criteria = %d "
                                "GROUP BY account, criteria "
                                ") "
                                "UNION ALL "
                                "( "
                                "SELECT * FROM account_achievement_progress WHERE criteria = %d "
                                ") "
                                ") alldata "
                                "GROUP BY account, criteria", critId, critId);
                        }
                        }

                        // also take the max considering the accumulating types to "elegantly" handle integer overflows
                        CharacterDatabase.PQuery("REPLACE INTO account_achievement_progress "
                            "SELECT account, criteria, MAX(counter) AS counter, MAX(date) AS date FROM "
                            "( "
                            "( "
                            "SELECT account, criteria, MAX(counter) AS counter, MAX(date) AS date "
                            "FROM character_achievement_progress cap INNER JOIN characters ch ON cap.guid = ch.guid "
                            "WHERE criteria = %d "
                            "GROUP BY account, criteria "
                            ") "
                            "UNION ALL "
                            "( "
                            "SELECT * FROM account_achievement_progress WHERE criteria = %d "
                            ") "
                            ") alldata "
                            "GROUP BY account, criteria", critId, critId);

                    }
                }
            } while (result->NextRow());

            if (found)
            {
                ss << ')';
                CharacterDatabase.Execute(ss.str().c_str());
            }
        }
        else
        {
            sLog->outInfo(LOG_FILTER_GENERAL, "Table character_achievement_progress is empty.");
        }
    }

    // NOTE: In order to have persistentFlags be set in worldstates for the next cleanup,
    // you need to define them at least once in worldstates.
    flags &= sWorld->getIntConfig(CONFIG_PERSISTENT_CHARACTER_CLEAN_FLAGS);
    CharacterDatabase.DirectPExecute("UPDATE worldstates SET value = %u WHERE entry = 20004", flags);

    sWorld->SetCleaningFlags(flags);

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Cleaned character database in %u ms", GetMSTimeDiffToNow(oldMSTime));

}

void CharacterDatabaseCleaner::CheckUnique(const char* column, const char* table, bool (*check)(uint32))
{
    QueryResult result = CharacterDatabase.PQuery("SELECT DISTINCT %s FROM %s", column, table);
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_GENERAL, "Table %s is empty.", table);
        return;
    }

    bool found = false;
    std::ostringstream ss;
    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();

        if (!check(id))
        {
            if (!found)
            {
                ss << "DELETE FROM " << table << " WHERE " << column << " IN (";
                found = true;
            }
            else
                ss << ',';

            ss << id;
        }
    }
    while (result->NextRow());

    if (found)
    {
        ss << ')';
        CharacterDatabase.Execute(ss.str().c_str());
    }
}

bool CharacterDatabaseCleaner::AchievementProgressCheck(uint32 criteria)
{
    return sAchievementCriteriaStore.LookupEntry(criteria);
}

void CharacterDatabaseCleaner::CleanCharacterAchievementProgress()
{
    CheckUnique("criteria", "character_achievement_progress", &AchievementProgressCheck);
}

bool CharacterDatabaseCleaner::SkillCheck(uint32 skill)
{
    return sSkillLineStore.LookupEntry(skill);
}

void CharacterDatabaseCleaner::CleanCharacterSkills()
{
    CheckUnique("skill", "character_skills", &SkillCheck);
}

bool CharacterDatabaseCleaner::SpellCheck(uint32 spell_id)
{
    return sSpellMgr->GetSpellInfo(spell_id);// && !GetTalentSpellPos(spell_id);
}

void CharacterDatabaseCleaner::CleanCharacterSpell()
{
    CheckUnique("spell", "character_spell", &SpellCheck);
}

bool CharacterDatabaseCleaner::TalentCheck(uint32 talent_id)
{
    return false;
    /*
    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talent_id);
    if (!talentInfo)
        return false;

    return sTalentTabStore.LookupEntry(talentInfo->TalentTab);*/
}

void CharacterDatabaseCleaner::CleanCharacterTalent()
{
    CharacterDatabase.DirectPExecute("DELETE FROM character_talent WHERE spec > %u", MAX_TALENT_SPECS);
    CheckUnique("spell", "character_talent", &TalentCheck);
}

void CharacterDatabaseCleaner::CleanCharacterQuestStatus()
{
    CharacterDatabase.DirectExecute("DELETE FROM character_queststatus WHERE status = 0");
}

