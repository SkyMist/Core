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

#ifndef TRINITY_DBCSTRUCTURE_H
#define TRINITY_DBCSTRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Define.h"
#include "Path.h"
#include "Util.h"

#include <map>
#include <set>
#include <vector>

// Structures using to access raw DBC data and required packing to portability

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct AchievementEntry
{
    uint32    ID;                                           // 0
    int32    requiredFaction;                               // 1 -1=all, 0=horde, 1=alliance
    int32    mapID;                                         // 2 -1=none
    //uint32 parentAchievement;                             // 3 its Achievement parent (can`t start while parent uncomplete, use its Criteria if don`t have own, use its progress on begin)
    char* name;                                             // 4
    //char* description;                                    // 5
    uint32    categoryId;                                   // 6
    uint32    points;                                       // 7 reward points
    //uint32 OrderInCategory;                               // 8
    uint32    flags;                                        // 9
    //uint32    icon;                                       // 10 icon (from SpellIcon.dbc)
    //char* reward;                                         // 11
    uint32 count;                                           // 12 - need this count of completed criterias (own or referenced achievement criterias)
    uint32 refAchievement;                                  // 13 - referenced achievement (counting of all completed criterias)
    //uint32 unk_505                                        // 14
};

struct AchievementCategoryEntry
{
    uint32    ID;                                           // 0
    uint32    parentCategory;                               // 1 -1 for main category
    //char*     name;                                       // 2
    //uint32    sortOrder;                                  // 3
};

struct AchievementCriteriaEntry
{
    uint32  ID;                                             // 0
    uint32  achievement;                                    // 1
    uint32  type;                                           // 2

    union
    {
        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE          = 0
        // @TODO: also used for player deaths..
        struct
        {
            uint32  creatureID;                             // 3
            uint32  creatureCount;                          // 4
        } kill_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_BG                 = 1
        struct
        {
            uint32  bgMapID;                                // 3
            uint32  winCount;                               // 4
        } win_bg;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS = 3
        struct
        {
            uint32 unused;                                 // 3
            uint32 count;                                  // 4
        } archaeology;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL            = 5
        struct
        {
            uint32  unused;                                 // 3
            uint32  level;                                  // 4
        } reach_level;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL      = 7
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4
        } reach_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT   = 8
        struct
        {
            uint32  linkedAchievement;                      // 3
        } complete_achievement;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT   = 9
        struct
        {
            uint32  unused;                                 // 3
            uint32  totalQuestCount;                        // 4
        } complete_quest_count;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfDays;                           // 4
        } complete_daily_quest_daily;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11
        struct
        {
            uint32  zoneID;                                 // 3
            uint32  questCount;                             // 4
        } complete_quests_in_zone;

        // ACHIEVEMENT_CRITERIA_TYPE_CURRENCY = 12
        struct
        {
            uint32 currency;
            uint32 count;
        } currencyGain;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST   = 14
        struct
        {
            uint32  unused;                                 // 3
            uint32  questCount;                             // 4
        } complete_daily_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND  = 15
        struct
        {
            uint32  mapID;                                  // 3
        } complete_battleground;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP           = 16
        struct
        {
            uint32  mapID;                                  // 3
        } death_at_map;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON       = 18
        struct
        {
            uint32  manLimit;                               // 3
        } death_in_dungeon;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID          = 19
        struct
        {
            uint32  groupSize;                              // 3 can be 5, 10 or 25
        } complete_raid;

        // ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE     = 20
        struct
        {
            uint32  creatureEntry;                          // 3
        } killed_by_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING     = 24
        struct
        {
            uint32  unused;                                 // 3
            uint32  fallHeight;                             // 4
        } fall_without_dying;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM            = 26
        struct
        {
            uint32 type;                                    // 3, see enum EnviromentalDamage
        } death_from;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST         = 27
        struct
        {
            uint32  questID;                                // 3
            uint32  questCount;                             // 4
        } complete_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET        = 28
        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2       = 69
        struct
        {
            uint32  spellID;                                // 3
            uint32  spellCount;                             // 4
        } be_spell_target;

        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL             = 29
        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2            = 110
        struct
        {
            uint32  spellID;                                // 3
            uint32  castCount;                              // 4
        } cast_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE
        struct
        {
            uint32 objectiveId;                             // 3
            uint32 completeCount;                           // 4
        } bg_objective;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31
        struct
        {
            uint32  areaID;                                 // 3 Reference to AreaTable.dbc
            uint32  killCount;                              // 4
        } honorable_kill_at_area;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA              = 32
        struct
        {
            uint32 mapID;                                   // 3 Reference to Map.dbc
            uint32 count;                                   // 4  Number of times that the arena must be won.
        } win_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA             = 33
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } play_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL            = 34
        struct
        {
            uint32  spellID;                                // 3 Reference to Map.dbc
        } learn_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM               = 36
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } own_item;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA        = 37
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
        } win_rated_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING    = 38
        struct
        {
            uint32  teamtype;                               // 3 {2, 3, 5}
        } highest_team_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_TEAM_RATING      = 39
        struct
        {
            uint32  teamtype;                               // 3 {2, 3, 5}
            uint32  teamrating;                             // 4
        } reach_team_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING = 39
        struct
        {
            uint32 teamtype; // 3 {2, 3, 5}
            uint32 PersonalRating; // 4
        } highest_personal_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL      = 40
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4 apprentice=1, journeyman=2, expert=3, artisan=4, master=5, grand master=6
        } learn_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM               = 41
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } use_item;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM              = 42
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } loot_item;

        // ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA           = 43
        struct
        {
            // TODO: This rank is _NOT_ the index from AreaTable.dbc
            uint32  areaReference;                          // 3
        } explore_area;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK               = 44
        struct
        {
            // TODO: This rank is _NOT_ the index from CharTitles.dbc
            uint32  rank;                                   // 3
        } own_rank;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT          = 45
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfSlots;                          // 4
        } buy_bank_slot;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION        = 46
        struct
        {
            uint32  factionID;                              // 3
            uint32  reputationAmount;                       // 4 Total reputation amount, so 42000 = exalted
        } gain_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfExaltedFactions;                // 4
        } gain_exalted_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP      = 48
        struct
        {
            uint32 unused;                                  // 3
            uint32 numberOfVisits;                          // 4
        } visit_barber;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM        = 49
        // TODO: where is the required itemlevel stored?
        struct
        {
            uint32  itemSlot;                               // 3
            uint32  count;                                  // 4
        } equip_epic_item;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT      = 50
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_need_on_loot;
       // ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT      = 51
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_greed_on_loot;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS               = 52
        struct
        {
            uint32  classID;                                // 3
            uint32  count;                                  // 4
        } hk_class;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_RACE                = 53
        struct
        {
            uint32  raceID;                                 // 3
            uint32  count;                                  // 4
        } hk_race;

        // ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE               = 54
        // TODO: where is the information about the target stored?
        struct
        {
            uint32  emoteID;                                // 3 enum TextEmotes
            uint32  count;                                  // 4 count of emotes, always required special target or requirements
        } do_emote;
        // ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE            = 13
        // ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE           = 55
        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
        } healing_done;

        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32  unused;
            uint32  killCount;
        } get_killing_blow;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM             = 57
        struct
        {
            uint32  itemID;                                 // 3
            uint32  count;                                  // 4
        } equip_item;

        // ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD= 62
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } quest_reward_money;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY             = 67
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } loot_money;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT         = 68
        struct
        {
            uint32  goEntry;                                // 3
            uint32  useCount;                               // 4
        } use_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL       = 70
        // @TODO: are those special criteria stored in the dbc or do we have to add another sql table?
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } special_pvp_kill;

        // ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT     = 72
        struct
        {
            uint32  goEntry;                                // 3
            uint32  lootCount;                              // 4
        } fish_in_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS = 75
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skillline_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL               = 76
        struct
        {
            uint32  unused;                                 // 3
            uint32  duelCount;                              // 4
        } win_duel;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM         = 90
        struct
        {
            uint32  unused;                                 // 
            uint32  lootCount;                             // 4
        } loot_epic_item;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER          = 96
        struct
        {
            uint32  powerType;                              // 3 mana=0, 1=rage, 3=energy, 6=runic power
        } highest_power;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT           = 97
        struct
        {
            uint32  statType;                               // 3 4=spirit, 3=int, 2=stamina, 1=agi, 0=strength
        } highest_stat;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER     = 98
        struct
        {
            uint32  spellSchool;                            // 3
        } highest_spellpower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING         = 100
        struct
        {
            uint32  ratingType;                             // 3
        } highest_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE              = 109
        struct
        {
            uint32  lootType;                               // 3 3=fishing, 2=pickpocket, 4=disentchant
            uint32  lootTypeCount;                          // 4
        } loot_type;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE       = 112
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skill_line;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL    = 113
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } honorable_kill;

        // ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS = 124
        struct
        {
            uint32 unused;
            uint32 goldCount;
        } spent_gold_guild_repairs;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL    = 125
        struct
        {
            uint32 unused;
            uint32  level;
        } reach_guild_level;

        // ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD = 126
        struct
        {
            uint32 unused;
            uint32 itemsCount;
        } craft_items_guild;

        // ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL = 127
        struct
        {
            uint32 unused;
            uint32 catchCount;
        } catch_from_pool;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS = 128
        struct
        {
            uint32 unused;
            uint32 slotsCount;
        } buy_guild_bank_slots;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS = 129
        struct
        {
            uint32 unused;
            uint32  pointsCount;
        } earn_guild_achievement_points;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND = 130
        struct
        {
            uint32 unused;
            uint32 winCount;
        } win_rated_battleground;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_BG_RATING
        struct
        {
            uint32 unused;
            uint32 max_rating;
        } reach_rbg_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD = 134
        struct
        {
            uint32 unused;
            uint32 questCount;
        } complete_quests_guild;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD = 135
        struct
        {
            uint32 unused;
            uint32 killCount;
        } honorable_kills_guild;

        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD = 136
        struct
        {
            uint32 unused;
            uint32 count;
        } kill_creature_type_guild;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGE_TYPE = 138
        struct
        {
            uint32 challenge_type;
            uint32 count;
        } guild_challenge_complete_type;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGE = 139
        struct
        {
            uint32 unused;
            uint32 count;
        } guild_challenge_complete;

        struct
        {
            uint32  unused;
            uint32  dungeonsComplete;
        } use_lfg;

        struct
        {
            uint32  field3;                                 // 3 main requirement
            uint32  count;                                  // 4 main requirement count
        } raw;
    };

    // uint32 unk                                           // 5

    struct
    {
        uint32  additionalRequirement_type;                 // 6-7
        uint32  additionalRequirement_value;                // 8-9
    } additionalRequirements[MAX_CRITERIA_REQUIREMENTS];

    char*  name;                                            // 10        m_description_lang
    uint32  completionFlag;                                 // 11       m_flags
    uint32  timedCriteriaStartType;                         // 12       m_timer_start_event Only appears with timed achievements, seems to be the type of starting a timed Achievement, only type 1 and some of type 6 need manual starting
                                                            //              1: ByEventId(?) (serverside IDs),    2: ByQuestId,   5: ByCastSpellId(?)
                                                            //              6: BySpellIdTarget(some of these are unknown spells, some not, some maybe spells)
                                                            //              7: ByKillNpcId,  9: ByUseItemId
    uint32  timedCriteriaMiscId;                            // 13       m_timer_asset_id Alway appears with timed events, used internally to start the achievement, store
    uint32  timeLimit;                                      // 14       m_timer_time time limit in seconds
    uint32  showOrder;                                      // 15       m_ui_order  also used in achievement shift-links as index in state bitmask
    //uint32 unk1;                                          // 16 only one value, still unknown
    //uint32 unk2;                                          // 17 all zeros
    uint32 additionalConditionType[MAX_ADDITIONAL_CRITERIA_CONDITIONS];     // 18-20
    uint32 additionalConditionValue[MAX_ADDITIONAL_CRITERIA_CONDITIONS-1];    // 21-22
};

struct AreaTableEntry
{
    uint32  ID;                                             // 0
    uint32  mapid;                                          // 1
    uint32  zone;                                           // 2 if 0 then it's zone, else it's zone id of this area
    uint32  exploreFlag;                                    // 3 main index
    uint32  flags;                                          // 4
    //uint32 unk505_1;                                      // 5
    //uint32 unk5;                                          // 6
    //uint32 unk6;                                          // 7
    //uint32 unk7;                                          // 8
    //uint32 unk8;                                          // 9
    //char*  name                                           // 10   Area name
    //uint32 unk9;                                          // 11
    int32   area_level;                                     // 12
    char*   area_name;                                      // 13   Area name loc
    uint32  team;                                           // 14
    uint32  LiquidTypeOverride[4];                          // 15-18 liquid override by type
    float   MaxDepth;                                       // 19 determine the maximum depth that a player an reach in an area before being teleported back up.
    //float  unk13;                                         // 20
    //uint32 unk19;                                         // 21 All zeros
    //uint32 unk20;                                         // 22 4.0.0
    //uint32 unk21;                                         // 23 All zeros
    //uint32 unk22;                                         // 24 4.0.0
    //uint32 unk23;                                         // 25 4.0.0
    //uint32 unk24;                                         // 26
    //int32 worldStateId;                                   // 27 worldStateId
    //uint32 unk25;                                         // 28
    //uint32 unk26;                                         // 29

    // helpers
    bool IsSanctuary() const
    {
        if (mapid == 609)
            return true;
        return (flags & AREA_FLAG_SANCTUARY);
    }
};

#define MAX_GROUP_AREA_IDS 6

struct AreaGroupEntry
{
    uint32  AreaGroupId;                                    // 0
    uint32  AreaId[MAX_GROUP_AREA_IDS];                     // 1-6
    uint32  nextGroup;                                      // 7 index of next group
};

struct AreaPOIEntry
{
    uint32 id;              //0
    uint32 icon[11];        //1-11
    float x;                //12
    float y;                //13
    uint32 mapId;           //14
    //uint32 val1;          //15
    int32 zoneId;           //16
    //char* name;           //17 - name
    //char* name2;          //18 - name2
    uint32 worldState;      //19
    //uint32 val2;          //20
    //uint32 unk505         //21  no data
    //uint32 unk;           //22
};

struct AreaTriggerEntry
{
    uint32  id;                                             // 0        m_ID
    uint32  mapid;                                          // 1        m_ContinentID
    float   x;                                              // 2        m_x
    float   y;                                              // 3        m_y
    float   z;                                              // 4        m_z
    //uint32                                                // 5
    //uint32                                                // 6
    //uint32                                                // 7
    float   radius;                                         // 8        m_radius
    float   box_x;                                          // 9        m_box_length
    float   box_y;                                          // 10       m_box_width
    float   box_z;                                          // 11       m_box_heigh
    float   box_orientation;                                // 12       m_box_yaw
    //uint32                                                // 13
    //uint32                                                // 14
    //uint32                                                // 15
};

struct ArmorLocationEntry
{
  uint32    InventoryType;                                  // 0
  float     Value[5];                                       // 1-5 multiplier for armor types (cloth...plate, no armor?)
};

struct AuctionHouseEntry
{
    uint32    houseId;                                      // 0 index
    uint32    faction;                                      // 1 id of faction.dbc for player factions associated with city
    uint32    depositPercent;                               // 2 1/3 from real
    uint32    cutPercent;                                   // 3
    //char*     name;                                       // 4
};

struct BankBagSlotPricesEntry
{
    uint32  ID;
    uint32  price;
};

struct BarberShopStyleEntry
{
    uint32  Id;                                             // 0
    uint32  type;                                           // 1 value 0 -> hair, value 2 -> facialhair
    //char*   name;                                         // 2 m_DisplayName_lang
    //uint32  unk_name;                                     // 3 m_Description_lang
    //float   CostMultiplier;                               // 4 m_Cost_Modifier
    uint32  race;                                           // 5 m_race
    uint32  gender;                                         // 6 m_sex
    uint32  hair_id;                                        // 7 m_data (real ID to hair/facial hair)
};

// @author Selenium: 5.4 valid
struct BattlemasterListEntry
{
    uint32  id;                                             // 0
    uint32  mapid[11];                                      // 1-11
    //int32 unk_1;                                          // 12 Possible field for futur mapid
    //int32 unk_2;                                          // 13 Possible field for futur mapid
    //int32 unk_3;                                          // 14 Possible field for futur mapid
    //int32 unk_4;                                          // 15 Possible field for futur mapid
    //int32 unk_5;                                          // 16 Possible field for futur mapid
    uint32  type;                                           // 17 (3 - BG, 4 - arena)
    //uint32 canJoinAsGroup;                                // 18
    char* name;                                             // 19 Map name
    //uint32 minPlayers;                                    // 20
    uint32 HolidayWorldStateId;                             // 21
    uint32 minLevel;                                        // 22 min level (sync with PvPDifficulty.dbc content)
    uint32 maxLevel;                                        // 23 max level (sync with PvPDifficulty.dbc content)
    uint32 maxGroupSize;                                    // 24
    //uint32 unk_6;                                         // 25 groupe size related (guild challenge ?)
    //uint32 maxPlayers;                                    // 26
    //uint32 isRated;                                       // 27 0 for normal 2 rated
    //uint32 unk_11;                                        // 28 data only for BG
    //char*  objectiveType;                                 // 29
};

#define MAX_OUTFIT_ITEMS 24

struct CharStartOutfitEntry
{
    //uint32 Id;                                            // 0
    uint32 RaceClassGender;                                 // 1 (UNIT_FIELD_BYTES_0 & 0x00FFFFFF) comparable (0 byte = race, 1 byte = class, 2 byte = gender)
    int32 ItemId[MAX_OUTFIT_ITEMS];                         // 2-13
    int32 ItemDisplayId[MAX_OUTFIT_ITEMS];                  // 14-25 not required at server side
    //int32 ItemInventorySlot[MAX_OUTFIT_ITEMS];            // 26-37 not required at server side
    //uint32 Unknown1;                                      // 38, unique values (index-like with gaps ordered in other way as ids)
    //uint32 Unknown2;                                      // 39
    //uint32 Unknown3;                                      // 40
    //uint32 Unknown4;                                      // 41
    //uint32 Unknown5;                                      // 42
};

struct CharTitlesEntry
{
    uint32  ID;                                             // 0 title ids, for example in Quest::GetCharTitleId()
    //uint32      unk1;                                     // 1 flags?
    char* name;                                             // 2 m_name_lang
    //char*       name2;                                    // 3 m_name1_lang
    uint32  bit_index;                                      // 4 m_mask_ID used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER_FIELD_KNOWN_TITLES
    //uint32                                                // 5
};

struct ChatChannelsEntry
{
    uint32  ChannelID;                                      // 0
    uint32  flags;                                          // 1
    //uint32                                                // 2        m_factionGroup
    char* pattern;                                          // 3        m_name_lang
    //char*       name;                                     // 4        m_shortcut_lang
};

struct ChrClassesEntry
{
    uint32  ClassID;                                        // 0
    uint32  powerType;                                      // 1        m_DisplayPower
                                                            // 2        m_petNameToken
    char* name;                                             // 3        m_name_lang
    //char*       nameFemale;                               // 4        m_name_female_lang
    //char*       nameNeutralGender;                        // 5        m_name_male_lang
    //char*       capitalizedName                           // 6        m_filename
    uint32  spellfamily;                                    // 7        m_spellClassSet
    //uint32 flags2;                                        // 8        m_flags (0x08 HasRelicSlot)
    uint32  CinematicSequence;                              // 9        m_cinematicSequenceID
    uint32 APPerStrenth;                                    // 11       Attack Power bonus per point of strength
    uint32 APPerAgility;                                    // 12       Attack Power bonus per point of agility
    uint32 RAPPerAgility;                                   // 13       Ranged Attack Power bonus per point of agility
    //uint32                                                // 14       Only death knight
    //uint32                                                // 15       Only death knight
    //uint32                                                // 16       All Zeros
    //uint32                                                // 17
};

struct ChrRacesEntry
{
    uint32      RaceID;                                     // 0
                                                            // 1 unused
    uint32      FactionID;                                  // 2 faction template id
                                                            // 3 unused
    uint32      model_m;                                    // 4
    uint32      model_f;                                    // 5
                                                            // 6 unused
    uint32      TeamID;                                     // 7 (7 Alliance, 1 Horde, 42 Neutral)
                                                            // 8-11 unused
    uint32      CinematicSequence;                          // 12 id from CinematicSequences.dbc
    //uint32    unk_322;                                    // 13       m_alliance (0 alliance, 1 horde, 2 disabled races)
    char* name;                                             // 14       m_name_lang used for DBC language detection/selection
    //char*       nameFemale;                               // 15       m_name_female_lang
    //char*       nameNeutralGender;                        // 16       m_name_male_lang
                                                            // 17-18    m_facialHairCustomization[2]
                                                            // 19       m_hairCustomization
    //uint32                                                // 20
    //uint32                                                // 21 (23 for worgens)
    //uint32                                                // 22 4.0.0
    //uint32                                                // 23 4.0.0
    //expansion;                                            // 24       m_required_expansion
    //uint32                                                // 25 5.0.5
    //uint32                                                // 26 5.0.5
    //uint32                                                // 27 5.0.5
    //float                                                 // 28 5.0.5
    //uint32                                                // 29 5.0.5 All Zeros
    //float                                                 // 30 5.0.5
    //float                                                 // 31 5.0.5
    //uint32                                                // 32 5.0.5 All zeros
    //float                                                 // 33 5.0.5
    //uint32    masterRace;                                 // 34 5.0.5 master race (Pandaren only)
    //uint32                                                // 35 5.0.5
};

struct ChrPowerTypesEntry
{
   uint32 entry;                                            // 0
   uint32 classId;                                          // 1
   uint32 power;                                            // 2
};

struct ChrSpecializationsEntry
{
    uint32 entry;                                           // 0
    char*  iconName;                                        // 1
    uint32 classId;                                         // 2
    uint32 specializationSpell;                             // 3
    //uint32 unk                                            // 4, empty
    uint32 tabId;                                           // 5
    //uint32 unk                                            // 6
    //uint32 unk                                            // 7
    //uint32 unk                                            // 8
    //uint32 unk                                            // 9
    //uint32 unk                                            // 10
    char* specializationName;                               // 11
    char* description;                                      // 12
    //uint32 unk                                            // 13
};

/* Unused
struct CinematicCameraEntry
{
    uint32      id;                                         // 0 index
    char*       filename;                                   // 1
    uint32      soundid;                                    // 2 in SoundEntries.dbc or 0
    float       start_x;                                    // 3
    float       start_y;                                    // 4
    float       start_z;                                    // 5
    float       unk6;                                       // 6 speed?
};
*/

struct CinematicSequencesEntry
{
    uint32      Id;                                         // 0 index
    //uint32      unk1;                                     // 1
    //uint32      cinematicCamera;                          // 2 id in CinematicCamera.dbc
                                                            // 3-9 always 0
};

struct CreatureDisplayInfoEntry
{
    uint32      Displayid;                                  // 0    m_ID
    uint32      ModelId;                                    // 1    m_modelID
    //uint32      sounID;                                   // 2    m_soundID
    //uint32      extendedDisplayInfoID;                    // 3    m_extendedDisplayInfoID
    float       scale;                                      // 4    m_creatureModelScale
    //uint32  creatureModelAlpha;                           // 5    m_creatureModelAlpha
    //char*   textureName;                                  // 6-8  m_textureVariation[3]
    //char*   portraitTextureName ;                         // 9    m_portraitTextureName
    //uint32  sizeClass;                                    // 10   m_sizeClass
    //uint32  bloodID;                                      // 11   m_bloodID
    //uint32  NPCSoundID;                                   // 12   m_NPCSoundID
    //uint32  particleColorID;                              // 13   m_particleColorID
    //uint32  creatureGeosetData                            // 14   m_creatureGeosetData
    //uint32  objectEffectPackageID                         // 15   m_objectEffectPackageID 0 only
    //uint32                                                // 16   0 only
    //uint32                                                // 17   0 only
    //int32                                                 // 18
    //uint32                                                // 19   5.4.X
};

struct CreatureFamilyEntry
{
    uint32  ID;                                             // 0        m_ID
    float   minScale;                                       // 1        m_minScale
    uint32  minScaleLevel;                                  // 2        m_minScaleLevel
    float   maxScale;                                       // 3        m_maxScale
    uint32  maxScaleLevel;                                  // 4        m_maxScaleLevel
    uint32  skillLine[2];                                   // 5-6      m_skillLine
    uint32  petFoodMask;                                    // 7        m_petFoodMask
    int32   petTalentType;                                  // 8        m_petTalentType
                                                            // 9        m_categoryEnumID
    char* Name;                                             // 10       m_name_lang
                                                            // 11       m_iconFile
};

struct CreatureModelDataEntry
{
    uint32    Id;                                           // 0
    //uint32  Flags;                                        // 1
    //char*   ModelPath                                     // 2
    //uint32  Unk1;                                         // 3
    //float   Scale;                                        // 4    Used in calculation of unit collision data
    //int32   Unk2;                                         // 5
    //int32   Unk3;                                         // 6
    //float   Unk4;                                         // 7
    //float   Unk5;                                         // 8
    //float   Unk6;                                         // 9
    //uint32  Unk7;                                         // 10   only 0
    //uint32  Unk8;                                         // 11
    //uint32  Unk9;                                         // 12
    //uint32  Unk10;                                        // 13
    //float   CollisionWidth;                               // 14
    float   CollisionHeight;                                // 15
    float   MountHeight;                                    // 16   Used in calculation of unit collision data when mounted
    //float   Unks[13]                                      // 17-30
    //float   unk540_1;                                     // 31
    //uint32  unk540_2;                                     // 32
    //float   unk540_3;                                     // 33
};

#define MAX_CREATURE_SPELL_DATA_SLOT 4

struct CreatureSpellDataEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    spellId[MAX_CREATURE_SPELL_DATA_SLOT];        // 1-4      m_spells[4]
    //uint32    availability[MAX_CREATURE_SPELL_DATA_SLOT]; // 4-7      m_availability[4]
};

struct CreatureTypeEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*   Name;                                         // 1        m_name_lang
    //uint32    no_experience;                              // 2        m_flags no exp (non-combat pets, battlepet, gas cloud.)
};

/* Unused
struct CurrencyCategoryEntry
{
    uint32    ID;                                           // 0
    uint32    Unk1;                                         // 1
    char*     Name;                                         // 2

};
*/

// @author Selenium: 5.4 valid
struct CurrencyTypesEntry
{
    uint32 ID;                                              // 0        not used
    uint32 Category;                                        // 1        may be category
    //char* name;                                           // 2
    //char* iconName;                                       // 3
    //uint32 unk4;                                          // 4        only 615 have data
    //uint32 unk5;                                          // 5        archaeology token only
    uint32 SubstitutionId;                                  // 6
    uint32 TotalCap;                                        // 7
    uint32 WeekCap;                                         // 8
    uint32 Flags;                                           // 9
    //char* description;                                    // 10
    //char* subDescription;                                 // 11

    bool HasPrecision() const   { return Flags & CURRENCY_FLAG_HIGH_PRECISION; }
    bool HasSeasonCount() const { return Flags & CURRENCY_FLAG_HAS_SEASON_COUNT; }
    float GetPrecision() const  { return HasPrecision() ? 100.0f : 1.0f; }
};

// @author Selenium: 5.4 valid
struct DestructibleModelDataEntry
{
    uint32  Id;                                             // 0     index
    uint32 IntactDisplayId;                                 // 1
    // uint32 unk2;                                         // 2
    // uint32 unk3;                                         // 3
    // float unk4;                                          // 4
    uint32 DamagedDisplayId;                                // 5
    // uint32 unk6;                                         // 6
    // uint32 unk7;                                         // 7
    // uint32 unk8;                                         // 8
    // float unk9;                                          // 9
    uint32 DestroyedDisplayId;                              // 10
    // uint32 unk11;                                        // 11
    // uint32 unk12;                                        // 12
    // float  unk13;                                        // 13
    // float unk14;                                         // 14
    uint32 RebuildingDisplayId;                             // 15
    // uint32 unk16;                                        // 16
    // uint32 unk17;                                        // 17
    // float unk18;                                         // 18
    // uint32 unk19;                                        // 19
    uint32 SmokeDisplayId;                                  // 20
    // uint32 unk21;                                        // 21
    // uint32 unk22;                                        // 22
    // uint32 unk23;                                        // 23
};

// @author Selenium: 5.4 valid
struct DungeonEncounterEntry
{
    uint32 id;                                              // 0        unique id
    uint32 mapId;                                           // 1        map id
    uint32 difficulty;                                      // 2        instance mode
    //int32 unk_1;                                          // 3
    uint32 encounterIndex;                                  // 4        encounter index for creating completed mask
    char* encounterName;                                    // 5        encounter name
    //uint32 nameFlags;                                     // 6
    //uint32 unk_1;                                         // 7
    //uint32 unk_3
};

// @author Selenium: 5.4 valid
struct DurabilityCostsEntry
{
    uint32    Itemlvl;                                      // 0
    uint32    multiplier[29];                               // 1-29
};

// @author Selenium: 5.4 valid
struct DurabilityQualityEntry
{
    uint32    Id;                                           // 0
    float     quality_mod;                                  // 1
};

// @author Selenium: 5.4 valid
struct EmotesEntry
{
    uint32  Id;                                             // 0
    //char* Name;                                           // 1        internal name
    //uint32 AnimationId;                                   // 2        ref to animationData
    uint32  Flags;                                          // 3        bitmask, may be unit_flags
    uint32  EmoteType;                                      // 4        Can be 0, 1 or 2 (determine how emote are shown)
    uint32  UnitStandState;                                 // 5        uncomfirmed, may be enum UnitStandStateType
    //uint32 SoundId;                                       // 6        ref to soundEntries
    //uint32 unk_1                                          // 7
};

// @author Selenium: 5.4 valid
struct EmotesTextEntry
{
    uint32  Id;                                             // 0
    uint32  textid;                                         // 1
    //uint32                                                // 2
    //uint32                                                // 3
    //uint32                                                // 4
    //uint32                                                // 5
    //uint32                                                // 6        0 only
    //uint32                                                // 7
    //uint32                                                // 8        0 only
    //uint32                                                // 9
    //uint32                                                // 10       0 only
    //uint32                                                // 11
    //uint32                                                // 12
    //uint32                                                // 13       0 only
    //uint32                                                // 14       0 only
    //uint32                                                // 15
    //uint32                                                // 16       0 only
    //uint32                                                // 17       0 only
    //uint32                                                // 18       0 only
};

// @author Selenium: 5.4 valid
struct FactionEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       reputationListID;                           // 1        m_reputationIndex
    uint32      BaseRepRaceMask[4];                         // 2-5      m_reputationRaceMask
    uint32      BaseRepClassMask[4];                        // 6-9      m_reputationClassMask
    int32       BaseRepValue[4];                            // 10-13    m_reputationBase
    uint32      ReputationFlags[4];                         // 14-17    m_reputationFlags
    uint32      team;                                       // 18       m_parentFactionID
    float       spilloverRateIn;                            // 19       Faction gains incoming rep * spilloverRateIn
    float       spilloverRateOut;                           // 20       Faction outputs rep * spilloverRateOut as spillover reputation
    uint32      spilloverMaxRankIn;                         // 21       The highest rank the faction will profit from incoming spillover
    //uint32    spilloverRank_unk;                          // 22       It does not seem to be the max standing at which a faction outputs spillover ...so no idea
    char* name;                                             // 23       m_name_lang
    //char*     description;                                // 24       m_description_lang
    //uint32    unk_1;                                      // 25
    //uint32    unk_2;                                      // 26
    //uint32    unk_3;                                      // 27

    // helpers
    bool CanHaveReputation() const
    {
        return reputationListID >= 0;
    }
};

#define MAX_FACTION_RELATIONS 4

// @author Selenium: 5.4 valid
struct FactionTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      faction;                                    // 1        m_faction
    uint32      factionFlags;                               // 2        m_flags
    uint32      ourMask;                                    // 3        m_factionGroup
    uint32      friendlyMask;                               // 4        m_friendGroup
    uint32      hostileMask;                                // 5        m_enemyGroup
    uint32      enemyFaction[MAX_FACTION_RELATIONS];        // 6-9      m_enemies[MAX_FACTION_RELATIONS]
    uint32      friendFaction[MAX_FACTION_RELATIONS];       // 10-14    m_friend[MAX_FACTION_RELATIONS]
    //-------------------------------------------------------  end structure

    // helpers
    bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        if (ID == entry.ID)
            return true;
        if (entry.faction)
        {
            for (int i = 0; i < MAX_FACTION_RELATIONS; ++i)
                if (enemyFaction[i] == entry.faction)
                    return false;
            for (int i = 0; i < MAX_FACTION_RELATIONS; ++i)
                if (friendFaction[i] == entry.faction)
                    return true;
        }
        return (friendlyMask & entry.ourMask) || (ourMask & entry.friendlyMask);
    }
    bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if (ID == entry.ID)
            return false;
        if (entry.faction)
        {
            for (int i = 0; i < MAX_FACTION_RELATIONS; ++i)
                if (enemyFaction[i] == entry.faction)
                    return true;
            for (int i = 0; i < MAX_FACTION_RELATIONS; ++i)
                if (friendFaction[i] == entry.faction)
                    return false;
        }
        return (hostileMask & entry.ourMask) != 0;
    }
    bool IsHostileToPlayers() const { return (hostileMask & FACTION_MASK_PLAYER) !=0; }
    bool IsNeutralToAll() const
    {
        for (int i = 0; i < MAX_FACTION_RELATIONS; ++i)
            if (enemyFaction[i] != 0)
                return false;
        return hostileMask == 0 && friendlyMask == 0;
    }
    bool IsContestedGuardFaction() const { return (factionFlags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD) != 0; }
};

// @author Selenium: 5.4 valid
struct GameObjectDisplayInfoEntry
{
    uint32      Displayid;                                  // 0        m_ID
    char* filename;                                         // 1
    //uint32  unk1[10];                                     // 2-11
    float   minX;                                           // 12
    float   minY;                                           // 13
    float   minZ;                                           // 14
    float   maxX;                                           // 15
    float   maxY;                                           // 16
    float   maxZ;                                           // 17
    //uint32  transport;                                    // 18
    //float unk_1;                                          // 19
    //float unk_2;                                          // 20
};

// @author Selenium: 5.4 valid
struct GemPropertiesEntry
{
    uint32      ID;                                         // 0        m_id
    uint32      spellitemenchantement;                      // 1        m_enchant_id
                                                            // 2        m_maxcount_inv
                                                            // 3        m_maxcount_item
    uint32      color;                                      // 4        m_type
    //uint32                                                // 5
};

// @author Selenium: 5.4 valid
struct GlyphPropertiesEntry
{
    uint32  Id;                                             // 0
    uint32  SpellId;                                        // 1
    uint32  TypeFlags;                                      // 2
    uint32  Unk1;                                           // 3        GlyphIconId (SpellIcon.dbc)
};

// @author Selenium: 5.4 valid
struct GlyphSlotEntry
{
    uint32  Id;                                             // 0
    uint32  TypeFlags;                                      // 1
    uint32  Order;                                          // 2
};

// All Gt* DBC store data for 100 levels, some by 100 per class/race
#define GT_MAX_LEVEL    100
// gtOCTClassCombatRatingScalar.dbc stores data for 32 ratings, look at MAX_COMBAT_RATING for real used amount
#define GT_MAX_RATING   32

struct GtBarberShopCostBaseEntry
{
    //uint32 level;
    float   cost;
};

struct GtCombatRatingsEntry
{
    //uint32 level;
    float    ratio;
};

struct GtChanceToMeleeCritBaseEntry
{
    //uint32 level;
    float    base;
};

struct GtChanceToMeleeCritEntry
{
    //uint32 level;
    float    ratio;
};

struct GtChanceToSpellCritBaseEntry
{
    float    base;
};

struct GtChanceToSpellCritEntry
{
    float    ratio;
};

struct GtOCTClassCombatRatingScalarEntry
{
    float    ratio;
};

struct GtOCTRegenHPEntry
{
    float    ratio;
};

struct GtOCTRegenMPEntry
{
    float    ratio;
};

struct gtOCTHpPerStaminaEntry
{
    float    ratio;
};

struct GtRegenHPPerSptEntry
{
    float    ratio;
};

struct GtRegenMPPerSptEntry
{
    float    ratio;
};

struct GtSpellScalingEntry
{
    float value;
};

struct GtOCTBaseHPByClassEntry
{
    float ratio;
};

struct GtOCTBaseMPByClassEntry
{
    float ratio;
};

// @author Selenium: 5.4 valid
struct GuildPerkSpellsEntry
{
    //uint32    Id;                                         // 0
    uint32  Level;                                          // 1
    uint32  SpellId;                                        // 2
};

//Unused
//struct HolidayDescriptionsEntry
//{
//    uint32 ID;                                            // 0, m_holidayDescriptionID
//    //char*     name                                      // 1  m_name_lang
//};
//
//
//
//struct HolidayNamesEntry
//{
//    uint32 ID;                                            // 0, m_holidayNameID
//    //char*     name                                      // 1  m_name_lang
//};

#define MAX_HOLIDAY_DURATIONS 10
#define MAX_HOLIDAY_DATES 26
#define MAX_HOLIDAY_FLAGS 10

// @author Selenium: 5.4 valid
struct HolidaysEntry
{
    uint32 Id;                                              // 0        m_ID
    uint32 Duration[MAX_HOLIDAY_DURATIONS];                 // 1-10     m_duration
    uint32 Date[MAX_HOLIDAY_DATES];                         // 11-36    m_date (dates in unix time starting at January, 1, 2000)
    uint32 Region;                                          // 37       m_region (wow region)
    uint32 Looping;                                         // 38       m_looping
    uint32 CalendarFlags[MAX_HOLIDAY_FLAGS];                // 39-48    m_calendarFlags
    //uint32 holidayNameId;                                 // 49       m_holidayNameID (HolidayNames.dbc)
    //uint32 holidayDescriptionId;                          // 50       m_holidayDescriptionID (HolidayDescriptions.dbc)
    char* TextureFilename;                                  // 51       m_textureFilename
    uint32 Priority;                                        // 52       m_priority
    int32 CalendarFilterType;                               // 53       m_calendarFilterType (-1 = Fishing Contest, 0 = Unk, 1 = Darkmoon Festival, 2 = Yearly holiday)
    //uint32 flags;                                         // 54       m_flags (0 = Darkmoon Faire, Fishing Contest and Wotlk Launch, rest is 1)
};

// @author Selenium: 5.4 valid
// ImportPriceArmor.dbc
struct ImportPriceArmorEntry
{
    uint32 InventoryType;                                   // 0        Id/InventoryType
    float ClothFactor;                                      // 1        Price factor cloth
    float LeatherFactor;                                    // 2        Price factor leather
    float MailFactor;                                       // 3        Price factor mail
    float PlateFactor;                                      // 4        Price factor plate
};

// @author Selenium: 5.4 valid
// ImportPriceQuality.dbc
struct ImportPriceQualityEntry
{
    uint32 QualityId;                                       // 0        Quality Id (+1?)
    float Factor;                                           // 1        Price factor
};

// @author Selenium: 5.4 valid
// ImportPriceShield.dbc
struct ImportPriceShieldEntry
{
    uint32 Id;                                              // 0        Unk id (only 1 and 2)
    float Factor;                                           // 1        Price factor
};

// @author Selenium: 5.4 valid
// ImportPriceWeapon.dbc
struct ImportPriceWeaponEntry
{
    uint32 Id;                                              // 0        Unk id (mainhand - 0, offhand - 1, weapon - 2, 2hweapon - 3, ranged/rangedright/relic - 4)
    float Factor;                                           // 1        Price factor
};

// @author Selenium: 5.4 valid
// ItemPriceBase.dbc
struct ItemPriceBaseEntry
{
    //uint32    ID;                                         // 0
    uint32  ItemLevel;                                      // 1        Item level (1 - 1000)
    float   ArmorFactor;                                    // 2        Price factor for armor
    float   WeaponFactor;                                   // 3        Price factor for weapons
};

// @author Selenium: 5.4 valid
struct ItemReforgeEntry
{
    uint32  Id;                                             // 0
    uint32  SourceStat;                                     // 1
    float   SourceMultiplier;                               // 2
    uint32  FinalStat;                                      // 3
    float   FinalMultiplier;                                // 4
};

// @author Selenium: 5.4 valid
// common struct for:
// ItemDamageAmmo.dbc
// ItemDamageOneHand.dbc
// ItemDamageOneHandCaster.dbc
// ItemDamageRanged.dbc
// ItemDamageThrown.dbc
// ItemDamageTwoHand.dbc
// ItemDamageTwoHandCaster.dbc
// ItemDamageWand.dbc
struct ItemDamageEntry
{
    uint32    Id;                                           // 0 item level
    float     DPS[7];                                       // 1-7 multiplier for item quality
    uint32    Id2;                                          // 8 item level
};

// @author Selenium: 5.4 valid
struct ItemArmorQualityEntry
{
    uint32    Id;                                           // 0 item level
    float     Value[7];                                     // 1-7 multiplier for item quality
    uint32    Id2;                                          // 8 item level
};

// @author Selenium: 5.4 valid
struct ItemArmorShieldEntry
{
    uint32    Id;                                           // 0 item level
    uint32    Id2;                                          // 1 item level
    float     Value[7];                                     // 2-8 multiplier for item quality
};

// @author Selenium: 5.4 valid
struct ItemArmorTotalEntry
{
    uint32    Id;                                           // 0 item level
    uint32    Id2;                                          // 1 item level
    float     Value[4];                                     // 2-5 multiplier for armor types (cloth...plate)
};

// @author Selenium: 5.4 valid
// ItemClass.dbc
struct ItemClassEntry
{
    uint32    Class;                                        // 0 item class id
    //uint32 unk_1;                                         // 1
    float     PriceFactor;                                  // 2 used to calculate certain prices
  //char*     Name;                                         // 3 class name
};

// @author Selenium: 5.4 valid
struct ItemBagFamilyEntry
{
    uint32   ID;                                            // 0
    //char*     name;                                       // 1        m_name_lang
};

// @author Selenium: 5.4 valid
struct ItemDisplayInfoEntry
{
    uint32  ID;                                             // 0        m_ID
    //char*   m_modelName;                                  // 1-2      m_modelName[2]
    //uint32                                                // 3-4      m_modelTexture[2]
    //uint32                                                // 5        m_inventoryIcon
    //uint32                                                // 6-8      m_geosetGroup[3]
    //uint32                                                // 9        m_flags
    //uint32                                                // 10       m_spellVisualID
    //uint32                                                // 11       m_groupSoundIndex
    //uint32                                                // 12-13    m_helmetGeosetVis[2]
    //uint32                                                // 14-15    m_texture[2]
    //uint32                                                // 16-23    m_itemVisual[8]
    //uint32                                                // 24       m_particleColorID
    //uint32 unk_1;                                         // 25
};

// @author Selenium: 5.4 valid
struct ItemDisenchantLootEntry
{
    uint32 Id;                                              // 0
    uint32 ItemClass;                                       // 1
    int32 ItemSubClass;                                     // 2
    uint32 ItemQuality;                                     // 3
    uint32 MinItemLevel;                                    // 4
    uint32 MaxItemLevel;                                    // 5
    uint32 RequiredDisenchantSkill;                         // 6
};

// @author Selenium: 5.4 valid
struct ItemLimitCategoryEntry
{
    uint32      ID;                                         // 0 Id
    //char*     name;                                       // 1        m_name_lang
    uint32      maxCount;                                   // 2,       m_quantity max allowed equipped as item or in gem slot
    uint32      mode;                                       // 3,       m_flags 0 = have, 1 = equip (enum ItemLimitCategoryMode)
};

#define MAX_ITEM_ENCHANTMENT_EFFECTS 3

// @author Selenium: 5.4 valid
struct ItemRandomPropertiesEntry
{
    uint32    ID;                                           // 0        m_ID
    //char* internalName                                    // 1        m_Name
    uint32    enchant_id[MAX_ITEM_ENCHANTMENT_EFFECTS];     // 2-4      m_Enchantment
    //uint32 unk_1;                                         // 5        unused
    //uint32 unk_2;                                         // 6        unused
    char* nameSuffix;                                       // 7        m_name_lang
};

// @author Selenium: 5.4 valid
struct ItemRandomSuffixEntry
{
    uint32    ID;                                           // 0        m_ID
    char* nameSuffix;                                       // 1        m_name_lang
    //char* internalName;                                   // 2        m_internalName
    uint32    enchant_id[5];                                // 3-7      m_enchantment
    uint32    prefix[5];                                    // 8-12     m_allocationPct
};

#define MAX_ITEM_SET_ITEMS 10
#define MAX_ITEM_SET_SPELLS 8

struct ItemSetEntry
{
    //uint32    id                                          // 0        m_ID
    char* name;                                             // 1        m_name_lang
    uint32    itemId[MAX_ITEM_SET_ITEMS];                   // 2-18     m_itemID
    uint32    spells[MAX_ITEM_SET_SPELLS];                  // 19-26    m_setSpellID
    uint32    items_to_triggerspell[MAX_ITEM_SET_SPELLS];   // 27-34    m_setThreshold
    uint32    required_skill_id;                            // 35       m_requiredSkill
    uint32    required_skill_value;                         // 36       m_requiredSkillRank
};

// @author Selenium: 5.4 valid
struct LFGDungeonEntry
{
    uint32  ID;                                             // 0
    //char*   name;                                         // 1
    uint32  minlevel;                                       // 2
    uint32  maxlevel;                                       // 3
    uint32  reclevel;                                       // 4
    uint32  recminlevel;                                    // 5
    uint32  recmaxlevel;                                    // 6
    int32  map;                                             // 7
    uint32  difficulty;                                     // 8
    uint32  flags;                                          // 9
    uint32  type;                                           // 10
    //int32  unk_1;                                         // 11
    //char*   namelite;                                     // 12 Name lite
    uint32  expansion;                                      // 13
    //uint32  unk_2;                                        // 14
    uint32 grouptype;                                       // 15
    //char* descr                                           // 16 Description
    uint32 flags2;                                          // 17
    uint32 tankNeeded;                                      // 18
    uint32 healerNeeded;                                    // 19
    uint32 dpsNeeded;                                       // 20
    //uint32 unk_3;                                         // 21 only 0/1
    //uint32 unk_4;                                         // 22
    //uint32 unk_5;                                         // 23
    //uint32 unk_6;                                         // 24 only 0
    uint32 category;                                        // 25 only for categories
    //uint32 unk_8;                                         // 26 only 0
    //uint32 unk_9;                                         // 27
    //uint32 unk_10;                                        // 28
    // Helpers
    uint32 Entry() const { return ID + (type << 24); }
    // 1 = LFG_TYPE_DUNGEON
    bool isScenario() const { return type == 1 && tankNeeded == 0 && healerNeeded == 0 && (dpsNeeded == 3 || dpsNeeded == 1); }
};


// @author Selenium: 5.4 valid
struct LiquidTypeEntry
{
    uint32      Id;                                         // 1
    //char*     Name;                                       // 2
    //uint32    Flags;                                      // 3
    uint32      Type;                                       // 4
    //uint32    SoundId;                                    // 5
    uint32      SpellId;                                    // 6
    //float     MaxDarkenDepth;                             // 7
    //float     FogDarkenIntensity;                         // 8
    //float     AmbDarkenIntensity;                         // 9
    //float     DirDarkenIntensity;                         // 10
    //uint32    LightID;                                    // 11
    //float     ParticleScale;                              // 12
    //uint32    ParticleMovement;                           // 13
    //uint32    ParticleTexSlots;                           // 14
    //uint32    LiquidMaterialID;                           // 15
    //char*     Texture[6];                                 // 16-22
    //uint32    Color[2];                                   // 23-24
    //float     Unk1[18];                                   // 25-42
    //uint32    Unk2[4];                                    // 43-46
};

#define MAX_LOCK_CASE 8

// @author Selenium: 5.4 valid
struct LockEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Type[MAX_LOCK_CASE];                        // 1-8      m_Type
    uint32      Index[MAX_LOCK_CASE];                       // 9-16     m_Index
    uint32      Skill[MAX_LOCK_CASE];                       // 17-24    m_Skill
    //uint32      Action[MAX_LOCK_CASE];                    // 25-32    m_Action
};

// @author Selenium: 5.4 valid
struct PhaseEntry
{
    uint32    ID;                                           // 0
    char*     Name;                                         // 1
    uint32    flag;                                         // 2
};

// @author Selenium: 5.4 valid
struct MailTemplateEntry
{
    uint32  ID;                                             // 0
    //char* subject;                                        // 1        m_subject_lang
    char*   content;                                        // 2        m_body_lang
};

// @author Selenium: 5.4 valid
struct MapEntry
{
    uint32      MapID;                                      // 0
    //char*     internalname;                               // 1        unused
    uint32      map_type;                                   // 2
    uint32      flags;                                      // 3        map flags
    //uint32    isPvp;                                      // 4        0 / 1 / 2 / 3 transport only
    char*       name;                                       // 5        m_MapName_lang
    uint32      linked_zone;                                // 6        m_areaTableID
    //char*     hordeIntro;                                 // 7        m_MapDescription0_lang
    //char*     allianceIntro;                              // 8        m_MapDescription1_lang
    uint32      multimap_id;                                // 9        m_LoadingScreenID (LoadingScreens.dbc)
    //float     BattlefieldMapIconScale;                    // 10       m_minimapIconScale
    int32       entrance_map;                               // 11       m_corpseMapID map_id of entrance map in ghost mode (continent always and in most cases = normal entrance)
    float       entrance_x;                                 // 12       m_corpseX entrance x coordinate in ghost mode  (in most cases = normal entrance)
    float       entrance_y;                                 // 13       m_corpseY entrance y coordinate in ghost mode  (in most cases = normal entrance)
    //uint32    timeOfDayOverride;                          // 14       m_timeOfDayOverride
    uint32      addon;                                      // 15       m_expansionID
    uint32      unk_time;                                   // 16       m_raidOffset
    uint32      maxPlayers;                                 // 17       m_maxPlayers
    int32       rootPhaseMap;                               // 18       mapid, related to phasing

    // Helpers
    uint32 Expansion() const { return addon; }

    bool IsDungeon()                const { return map_type == MAP_INSTANCE || map_type == MAP_RAID; }
    bool IsNonRaidDungeon()         const { return map_type == MAP_INSTANCE; }
    bool Instanceable()             const { return map_type == MAP_INSTANCE || map_type == MAP_RAID || map_type == MAP_BATTLEGROUND || map_type == MAP_ARENA || map_type == MAP_SCENARIO; }
    bool IsRaid()                   const { return map_type == MAP_RAID; }
    bool IsBattleground()           const { return map_type == MAP_BATTLEGROUND; }
    bool IsBattleArena()            const { return map_type == MAP_ARENA; }
    bool IsScenario()               const { return map_type == MAP_SCENARIO; }
    bool IsBattlegroundOrArena()    const { return map_type == MAP_BATTLEGROUND || map_type == MAP_ARENA; }
    bool IsWorldMap()               const { return map_type == MAP_COMMON; }

    bool GetEntrancePos(int32 &mapid, float &x, float &y) const
    {
        if (entrance_map < 0)
            return false;
        mapid = entrance_map;
        x = entrance_x;
        y = entrance_y;
        return true;
    }

    bool IsContinent() const
    {
        return MapID == 0 || MapID == 1 || MapID == 530 || MapID == 571 || MapID == 860 || MapID == 870;
    }

    bool HasDynamicDifficulty() const { return (flags & MAP_FLAGS_HAS_DYNAMIC_DIFFICULTY) != 0; }
};

// @author Selenium: 5.4 valid
struct MapDifficultyEntry
{
    //uint32      Id;                                       // 0
    uint32      MapId;                                      // 1
    uint32      Difficulty;                                 // 2        (for arenas: arena slot)
    char*       areaTriggerText;                            // 3        m_message_lang (text showed when transfer to map failed)
    uint32      resetTime;                                  // 4        m_raidDuration in secs, 0 if no fixed reset time
    uint32      maxPlayers;                                 // 5        m_maxPlayers some heroic versions have 0 when expected same amount as in normal version
    //uint32    unk_1;                                      // 6        0 only
};

// @author Selenium: 5.4 valid
struct MountCapabilityEntry
{
    uint32 Id;
    uint32 Flags;
    uint32 RequiredRidingSkill;
    uint32 RequiredArea;
    uint32 RequiredAura;
    uint32 RequiredSpell;
    uint32 SpeedModSpell;
    int32  RequiredMap;
};

#define MAX_MOUNT_CAPABILITIES 24

// @author Selenium: 5.4 valid
struct MountTypeEntry
{
    uint32 Id;
    uint32 MountCapability[MAX_MOUNT_CAPABILITIES];
};

// @author Selenium: 5.4 valid
struct MovieEntry
{
    uint32      Id;                                         // 0        index
    //char*       filename;                                 // 1
    //uint32      unk1;                                     // 2        m_volume
    //uint32      unk2;                                     // 3        4.0.0
    //uint32      unk3;                                     // 4        5.0.0
};

// @author Selenium: 5.4 valid
struct NameGenEntry
{
    //uint32 id;
    char*  name;
    uint32 race;
    uint32 gender;
};

#define MAX_OVERRIDE_SPELL 10

// @author Selenium: 5.4 valid
struct OverrideSpellDataEntry
{
    uint32      id;                                         // 0
    uint32      spellId[MAX_OVERRIDE_SPELL];                // 1-10
    //uint32      unk_0;                                    // 11
    //uint32      unk_1;                                    // 12 possibly flag
};

// @author Selenium: 5.4 valid
struct PvPDifficultyEntry
{
    //uint32      id;                                       // 0        m_ID
    uint32      mapId;                                      // 1
    uint32      bracketId;                                  // 2
    uint32      minLevel;                                   // 3
    uint32      maxLevel;                                   // 4
    uint32      difficulty;                                 // 5

    // helpers
    BattlegroundBracketId GetBracketId() const { return BattlegroundBracketId(bracketId); }
};

// @author Selenium: 5.4 valid
struct QuestSortEntry
{
    uint32      id;                                         // 0        m_ID
    //char*       name;                                     // 1        m_SortName_lang
};

// @author Selenium: 5.4 valid
struct QuestXPEntry
{
  uint32      id;
  uint32      Exp[10];
};

// @author Selenium: 5.4 valid
struct QuestFactionRewEntry
{
  uint32      id;
  int32       QuestRewFactionValue[10];
};

struct QuestPOIBlobEntry
{
    uint32      Id;                                         // 0 m_Id
    uint32      type;                                       // 1 m_Type
    uint32      mapId;                                      // 2 m_mapId
    uint32      unk;                                        // 3 m_unk questId?
};

struct QuestPOIPointEntry
{
    uint32      Id;                                         // 0 m_Id
    int32       x;                                          // 1 m_zoneX
    int32       y;                                          // 2 m_zoneY
    uint32      blobId;                                     // 3 m_Id
};

struct RandomPropertiesPointsEntry
{
    uint32    Id;                                           // 0 hidden key
    uint32    EpicPropertiesPoints[5];                      // 1-5
    uint32    RarePropertiesPoints[5];                      // 6-10
    uint32    UncommonPropertiesPoints[5];                  // 11-15
};

struct ResearchBranchEntry
{
    uint32 ID;          // 0
    //char*  Name;      // 1
    //uint32 FieldID;   // 2
    uint32 CurrencyID;  // 3
    //char*  Icon;      // 4
    uint32 ItemID;      // 5
};

struct ResearchProjectEntry
{
    uint32      ID;                                         // 0
    //char*       name;                                     // 1
    //char*       description;                              // 2
    uint32      rare;                                       // 3
    uint32      branchId;                                   // 4
    uint32      spellId;                                    // 5
    //uint32    complexity;                                 // 6
    //char*     path;                                       // 7
    uint32      req_currency;                               // 8

};

struct ResearchSiteEntry
{
    uint32 ID;                                              // 0
    uint32 mapId;                                           // 1
    uint32 POIid;                                           // 2
    //char* areaName;                                       // 3
    //uint32 flags;                                         // 4
};
// @author Selenium: 5.4 valid
struct ScalingStatDistributionEntry
{
    uint32  Id;                                             // 0
    int32   StatMod[10];                                    // 1-10
    uint32  Modifier[10];                                   // 11-20
    //uint32 unk_1;                                         // 21
    uint32  MaxLevel;                                       // 22       m_maxlevel
};

// @author Selenium: todo for 5.4 valid
struct ScalingStatValuesEntry
{
    uint32 Id;                                              // 0
    uint32 Level;                                           // 1
    uint32 dpsMod[7];                                       // 2-8      DPS mod for level
    uint32 Spellpower;                                      // 9        Spell power for level
    uint32 StatMultiplier[5];                               // 10-14    Multiplier for ScalingStatDistribution
    uint32 Armor[8][4];                                     // 15-47    Armor for level
    uint32 CloakArmor;                                      // 48       Armor for cloak
    //uint32 Unk540_1;                                      // 49       New in 5.4.X @todo

    uint32 GetStatMultiplier(uint32 inventoryType) const;
    uint32 GetArmor(uint32 inventoryType, uint32 armorType) const;
    uint32 GetDPSAndDamageMultiplier(uint32 subClass, bool isCasterWeapon, float* damageMultiplier) const;
};

// @author Selenium: 5.4 valid
//struct SkillRaceClassInfoEntry
//{
//    uint32    id;                                         // 0      m_ID
//    uint32    skillId;                                    // 1      m_skillID
//    uint32    raceMask;                                   // 2      m_raceMask
//    uint32    classMask;                                  // 3      m_classMask
//    uint32    flags;                                      // 4      m_flags
//    uint32    reqLevel;                                   // 5      m_minLevel
//    uint32    skillTierId;                                // 6      m_skillTierID
//    uint32    skillCostID;                                // 7      m_skillCostIndex
//};

// 16 field removed
//struct SkillTiersEntry
//{
//    uint32    id;                                         // 0      m_ID
//    uint32    skillValue[16];                             // 1-17   m_cost
//    uint32    maxSkillValue[16];                          // 18-32  m_valueMax
//};

// @author Selenium: 5.4 valid
struct SkillLineEntry
{
    uint32 id;                                              // 0    m_ID
    int32 categoryId;                                       // 1    m_categoryID
    char* name;                                             // 2    m_displayName_lang
    //char* description;                                    // 3    m_description_lang
    uint32 spellIcon;                                       // 4    m_spellIconID
    //char* alternateVerb;                                  // 5    m_alternateVerb_lang
    uint32 canLink;                                         // 6    m_canLink (prof. with recipes)
    //uint32 skillId;                                       // 7    Only cooking skill id
    uint32 unk_1;                                           // 8    Unknow, maybe flag ?
};

// @author Selenium: 5.4 valid
struct SkillLineAbilityEntry
{
    uint32    id;                                           // 0        m_ID
    uint32    skillId;                                      // 1        m_skillLine
    uint32    spellId;                                      // 2        m_spell
    uint32    racemask;                                     // 3        m_raceMask
    uint32    classmask;                                    // 4        m_classMask
    uint32    req_skill_value;                              // 5        m_minSkillLineRank
    uint32    forward_spellid;                              // 6        m_supercededBySpell
    uint32    learnOnGetSkill;                              // 7        m_acquireMethod
    uint32    max_value;                                    // 8        m_trivialSkillLineRankHigh
    uint32    min_value;                                    // 9        m_trivialSkillLineRankLow
    uint32    skill_gain;                                   // 10       m_skillgain 1-30
    //uint32  unk_1                                         // 11       4.0.0 unk (increment id)
    //uint32  unk_2                                         // 12       5.0.5
};

// @author Selenium: 5.4 valid
struct SoundEntriesEntry
{
    uint32    Id;                                           // 0        m_ID
    //uint32  Type;                                         // 1        m_soundType
    //char*   InternalName;                                 // 2        m_name
    //uint32  Unk540_1[10];                                 // 3-12     m_File[10] old char* but no string ?
    //uint32  Unk13[10];                                    // 13-22    m_Freq[10]
    //float   volumeFloat;                                  // 23       m_volumeFloat
    //uint32                                                // 24       m_flags
    //float                                                 // 25       m_minDistance
    //float                                                 // 26       m_distanceCutoff
                                                            // 27       m_EAXDef
    //uint32  unk540_2;                                     // 28
    //float   unk540_3;                                     // 29
    //float   unk540_4;                                     // 30
    //float   unk540_5;                                     // 31
    //float   unk540_6;                                     // 32
    //float   unk540_7;                                     // 33
    //uint32  unk540_8;                                     // 34
};

// SpecializationSpells.dbc
// @author Selenium: 5.4 valid
struct SpecializationSpellEntry
{
    uint32  Id;                      // 0
    uint32  SpecializationEntry;     // 1
    uint32  LearnSpell;              // 2
    uint32  OverrideSpell;           // 3
    //uint32 Unk_1                   // 4  Always 0
};

// @author Selenium: 5.4 valid
// SpellEffect.dbc
struct SpellEffectEntry
{
    uint32    Id;                                           // 0         m_ID
    uint32    EffectDifficulty;                             // 1         m_effectDifficulty
    uint32    Effect;                                       // 2         m_effect
    float     EffectValueMultiplier;                        // 3         m_effectAmplitude
    uint32    EffectApplyAuraName;                          // 4         m_effectAura
    uint32    EffectAmplitude;                              // 5         m_effectAuraPeriod
    int32     EffectBasePoints;                             // 6         m_effectBasePoints (don't must be used in spell/auras explicitly, must be used cached Spell::m_currentBasePoints)
    float     EffectBonusMultiplier;                        // 7         m_effectBonus
    float     EffectDamageMultiplier;                       // 8         m_effectChainAmplitude
    uint32    EffectChainTarget;                            // 9         m_effectChainTargets
    int32     EffectDieSides;                               // 10        m_effectDieSides
    uint32    EffectItemType;                               // 11        m_effectItemType
    uint32    EffectMechanic;                               // 12        m_effectMechanic
    int32     EffectMiscValue;                              // 13        m_effectMiscValue
    int32     EffectMiscValueB;                             // 14        m_effectMiscValueB
    float     EffectPointsPerComboPoint;                    // 15        m_effectPointsPerCombo
    uint32    EffectRadiusIndex;                            // 16        m_effectRadiusIndex - spellradius.dbc
    uint32    EffectRadiusMaxIndex;                         // 17        4.0.0
    float     EffectRealPointsPerLevel;                     // 18        m_effectRealPointsPerLevel
    flag128   EffectSpellClassMask;                         // 19 - 22   m_effectSpellClassMask1(2/3), effect 0
    uint32    EffectTriggerSpell;                           // 23        m_effectTriggerSpell
    //float   unk_1                                         // 24        5.0.5
    uint32    EffectImplicitTargetA;                        // 25        m_implicitTargetA
    uint32    EffectImplicitTargetB;                        // 26        m_implicitTargetB
    uint32    EffectSpellId;                                // 27        new 4.0.0
    uint32    EffectIndex;                                  // 28        new 4.0.0
    //uint32  unk_flag_2                                    // 29        4.2.0 Flag
};

// @author Selenium: 5.4 valid
struct SpellEffectScalingEntry
{
    //uint32    Unk_540;                                    // New 5.4.X
    float     Multiplier;                                   // 1
    float     RandomMultiplier;                             // 2
    float     OtherMultiplier;                              // 3
    //float   Unk_505                                       // 4
    uint32    Id;                                           // Effect ID
};

#define MAX_SPELL_EFFECTS 32
#define MAX_EFFECT_MASK 4294967295
#define MAX_SPELL_REAGENTS 8

// SpellAuraOptions.dbc
struct SpellAuraOptionsEntry
{
    uint32    Id;                                           // 0       m_ID
    uint32    SpellId;                                      // 1       m_SpellId
    //uint32    Unk_1;                                      // 2
    uint32    StackAmount;                                  // 3       m_cumulativeAura
    uint32    procChance;                                   // 4       m_procChance
    uint32    procCharges;                                  // 5       m_procCharges
    uint32    procFlags;                                    // 6       m_procTypeMask
    uint32    InternalCooldown;                             // 7       m_internalCooldown
    uint32    ProcsPerMinuteEntry;                          // 8       m_procsPerMinuteEntry
};

// SpellProcsPerMinute.dbc
struct SpellProcsPerMinuteEntry
{
    uint32 Id;                                              // 0        m_ID
    float ProcsPerMinute;                                   // 1        m_procsPerMinute
    //bool unk;                                             // 2
};

// SpellAuraRestrictions.dbc/
// @author Selenium: 5.4 valid
struct SpellAuraRestrictionsEntry
{
    //uint32  Id;                                           // 0       m_ID
    //uint32  spellId;                                      // 1       m_SpellId
    //uint32  unk;                                          // 2   
    uint32    CasterAuraState;                              // 3       m_casterAuraState
    uint32    TargetAuraState;                              // 4       m_targetAuraState
    uint32    CasterAuraStateNot;                           // 5       m_excludeCasterAuraState
    uint32    TargetAuraStateNot;                           // 6       m_excludeTargetAuraState
    uint32    casterAuraSpell;                              // 7       m_casterAuraSpell
    uint32    targetAuraSpell;                              // 8       m_targetAuraSpell
    uint32    excludeCasterAuraSpell;                       // 9       m_excludeCasterAuraSpell
    uint32    excludeTargetAuraSpell;                       // 10      m_excludeTargetAuraSpell
};

// SpellCastingRequirements.dbc
// @author Selenium: 5.4 valid
struct SpellCastingRequirementsEntry
{
    //uint32    Id;                                         // 0      m_ID
    uint32    FacingCasterFlags;                            // 1      m_facingCasterFlags
    //uint32    MinFactionId;                               // 2      m_minFactionID not used
    //uint32    MinReputation;                              // 3      m_minReputation not used
    int32     AreaGroupId;                                  // 4      m_requiredAreaGroupId
    //uint32    RequiredAuraVision;                         // 5      m_requiredAuraVision not used
    uint32    RequiresSpellFocus;                           // 6      m_requiresSpellFocus
};

#define MAX_SPELL_TOTEMS            2

// SpellTotems.dbc
// @author Selenium: 5.4 valid
struct SpellTotemsEntry
{
    uint32    Id;                                           // 0  m_ID
    uint32    TotemCategory[MAX_SPELL_TOTEMS];              // 1  m_requiredTotemCategoryID
    uint32    Totem[MAX_SPELL_TOTEMS];                      // 2  m_totem
    //uint32    unk_1;                                      // 3
    //uint32    unk_2;                                      // 4  only 1925
};

struct SpellTotem
{
    SpellTotem()
    {
        totems[0] = NULL;
        totems[1] = NULL;
    }
    SpellTotemsEntry const* totems[MAX_SPELL_TOTEMS];
};

typedef std::map<uint32, SpellTotem> SpellTotemMap;

// Spell.dbc
// @author Selenium: 5.4 valid
struct SpellEntry
{
    uint32    Id;                                           // 0       m_ID
    char* SpellName;                                        // 1       m_name_lang
    char* Rank;                                             // 2       m_nameSubtext_lang
    //char* Description;                                    // 3       m_description_lang not used
    //char* ToolTip;                                        // 4       m_auraDescription_lang not used
    uint32    runeCostID;                                   // 5       m_runeCostID
    //uint32  spellMissileID;                               // 6       m_spellMissileID not used
    //uint32  spellDescriptionVariableID;                   // 7       m_spellDescriptionVariableID, 3.2.0
    float  APMultiplier;                                    // 8
    uint32 SpellScalingId;                                  // 9        SpellScaling.dbc
    uint32 SpellAuraOptionsId;                              // 10       SpellAuraOptions.dbc
    uint32 SpellAuraRestrictionsId;                         // 11       SpellAuraRestrictions.dbc
    uint32 SpellCastingRequirementsId;                      // 12       SpellCastingRequirements.dbc
    uint32 SpellCategoriesId;                               // 13       SpellCategories.dbc
    uint32 SpellClassOptionsId;                             // 14       SpellClassOptions.dbc
    uint32 SpellCooldownsId;                                // 15       SpellCooldowns.dbc
    uint32 SpellEquippedItemsId;                            // 16       SpellEquippedItems.dbc
    uint32 SpellInterruptsId;                               // 17       SpellInterrupts.dbc
    uint32 SpellLevelsId;                                   // 18       SpellLevels.dbc
    uint32 SpellReagentsId;                                 // 19       SpellReagents.dbc
    uint32 SpellShapeshiftId;                               // 20       SpellShapeshift.dbc
    uint32 SpellTargetRestrictionsId;                       // 21       SpellTargetRestrictions.dbc
    uint32 SpellTotemsId;                                   // 22       SpellTotems.dbc
    uint32 ResearchProject;                                 // 23       ResearchProject.dbc
    uint32 SpellMiscId;                                     // 24       SpellMisc.dbc

    // struct access functions
    SpellEffectEntry const* GetSpellEffect(uint32 eff, uint32 difficulty) const;
};

// SpellCategories.dbc
// @author Selenium: 5.4 valid
struct SpellCategoriesEntry
{
    uint32 Id;                                          // 0        m_ID
    uint32 SpellId;                                     // 1        m_spellId
    uint32 Unk1;                                        // 2
    uint32 Category;                                    // 3        m_category
    uint32 DmgClass;                                    // 4        m_defenseType
    uint32 Dispel;                                      // 5        m_dispelType
    uint32 Mechanic;                                    // 6        m_mechanic
    uint32 PreventionType;                              // 7        m_preventionType
    uint32 StartRecoveryCategory;                       // 8        m_startRecoveryCategory
    uint32 ChargesCategory;                             // 9        m_chargesCategory
};

// SpellCategory.dbc
// @author Selenium: 5.4 valid
struct SpellCategoryEntry
{
    uint32 Id;                                          // 0
    uint32 Flags;                                       // 1
    uint32 Unk1;                                        // 2
    char const *Description;                            // 3
    uint32 MaxCharges;                                  // 4
    uint32 ChargeRegenTime;                             // 5
};

typedef std::set<uint32> SpellCategorySet;
typedef std::map<uint32, SpellCategorySet > SpellCategoryStore;
typedef std::list<const SpellEntry*> SpellSkillingList;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32, PetFamilySpellsSet > PetFamilySpellsStore;

// @author Selenium: 5.4 valid
struct SpellCastTimesEntry
{
    uint32    ID;                                           // 0
    int32     CastTime;                                     // 1
    //float     CastTimePerLevel;                           // 2 unsure / per skill?
    //int32     MinCastTime;                                // 3 unsure
};

// @author Selenium: 5.4 valid
struct SpellFocusObjectEntry
{
    uint32    ID;                                           // 0
    //char*     Name;                                       // 1        m_name_lang
};

// @author Selenium: 5.4 valid
struct SpellRadiusEntry
{
    uint32    ID;
    float     RadiusMin;
    float     RadiusPerLevel;
    //float unkRadius;
    float     RadiusMax;
};

// @author Selenium: todo for 5.4 valid
struct SpellRangeEntry
{
    uint32    ID;                                           // 0
    float     minRangeHostile;                              // 1
    float     minRangeFriend;                               // 2
    float     maxRangeHostile;                              // 3
    float     maxRangeFriend;                               // 4    friend means unattackable unit here
    uint32    type;                                         // 5
    //char*   Name;                                         // 6    m_displayName_lang
    //char*   ShortName;                                    // 7    m_displayNameShort_lang
};

// SpellEquippedItems.dbc
// @author Selenium: 5.4 valid
struct SpellEquippedItemsEntry
{
    //uint32    Id;                                         // 0    m_ID
    int32     EquippedItemClass;                            // 1    m_equippedItemClass (value)
    int32     EquippedItemInventoryTypeMask;                // 2    m_equippedItemInvTypes (mask)
    int32     EquippedItemSubClassMask;                     // 3    m_equippedItemSubclass (mask)
    //uint32    unk_1                                       // 4
    //uint32    unk_2                                       // 5
};

// SpellCooldowns.dbc
// @author Selenium: 5.4 valid
struct SpellCooldownsEntry
{
    //uint32    Id;                                         // 0    m_ID
    uint32    CategoryRecoveryTime;                         // 1    m_categoryRecoveryTime
    uint32    RecoveryTime;                                 // 2    m_recoveryTime
    uint32    StartRecoveryTime;                            // 3    m_startRecoveryTime
    //uint32    unk_1                                       // 4    unk timer
    //uin32     unk_2                                       // 5    unk timer_2
};

// SpellClassOptions.dbc
// @author Selenium: 5.4 valid
struct SpellClassOptionsEntry
{
    //uint32    Id;                                         // 0       m_ID
    //uint32    modalNextSpell;                             // 1       m_modalNextSpell not used
    flag128   SpellFamilyFlags;                             // 2-5
    uint32    SpellFamilyName;                              // 6       m_spellClassSet
};

// SpellInterrupts.dbc
// @author Selenium: 5.4 valid
struct SpellInterruptsEntry
{
    //uint32    Id;                                         // 0       m_ID
    uint32    AuraInterruptFlags;                           // 1       m_auraInterruptFlags
    //uint32    unk_1                                       // 2       4.0.0
    uint32    ChannelInterruptFlags;                        // 3       m_channelInterruptFlags
    //uint32    unk_2                                       // 4       4.0.0
    uint32    InterruptFlags;                               // 5       m_interruptFlags
    //uint32    unk_2                                       // 6       New 5.4.0
    //uint32    unk_3                                       // 7       New 5.4.0
};

// SpellLevels.dbc
// @author Selenium: 5.4 valid
struct SpellLevelsEntry
{
    //uint32    Id;                                         // 0 m_ID
    uint32  SpellId;                                        // 1 m_spellId
    //uint32    unk_1;                                      // 2 5.0.1.15589
    uint32  baseLevel;                                      // 3 m_baseLevel
    uint32  maxLevel;                                       // 4 m_maxLevel
    uint32  spellLevel;                                     // 5 m_spellLevel
};

//SpellMisc.dbc
// @author Selenium: 5.4 valid
struct SpellMiscEntry
{
    uint32    Id;                                           // 0        m_ID
    uint32    SpellId;                                      // 1        m_spellId
    //uint32 Unknown;                                       // 2        5.0.1.15589
    uint32    Attributes;                                   // 3        m_attribute
    uint32    AttributesEx;                                 // 4        m_attributesEx
    uint32    AttributesEx2;                                // 5        m_attributesExB
    uint32    AttributesEx3;                                // 6        m_attributesExC
    uint32    AttributesEx4;                                // 7        m_attributesExD
    uint32    AttributesEx5;                                // 8        m_attributesExE
    uint32    AttributesEx6;                                // 9        m_attributesExF
    uint32    AttributesEx7;                                // 10       m_attributesExG (0x20 - totems, 0x4 - paladin auras, etc...)
    uint32    AttributesEx8;                                // 11       m_attributesExH
    uint32    AttributesEx9;                                // 12       m_attributesExI
    uint32    AttributesEx10;                               // 13       m_attributesExI
    uint32    AttributesEx11;                               // 14       m_attributesExI
    uint32    AttributesEx12;                               // 15       m_attributesExI
    //uint32    unk_1;                                      // 16       New 5.4.8(7)
    uint32    CastingTimeIndex;                             // 17       m_castingTimeIndex
    uint32    DurationIndex;                                // 18       m_durationIndex
    uint32    rangeIndex;                                   // 19       m_rangeIndex
    float     speed;                                        // 20       m_speed
    uint32    SpellVisual[2];                               // 21-22    m_spellVisualID
    uint32    SpellIconID;                                  // 23       m_spellIconID
    uint32    activeIconID;                                 // 24       m_activeIconID
    uint32    SchoolMask;                                   // 25       m_schoolMask
};

// SpellPower.dbc
// @author Selenium: 5.4 valid
struct SpellPowerEntry
{
    uint32    Id;                                           // 0        m_ID
    uint32    SpellId;                                      // 1
    //uint32    unk_1;                                      // 2
    uint32    powerType;                                    // 3
    uint32    manaCost;                                     // 4
    //uint32    unk_2;                                      // 5
    //uint32    unk_3;                                      // 6
    //uint32    unk_4;                                      // 7
    //uint32    unk_5;                                      // 8
    float ManaCostPercentage;                               // 9
    float manaPerSecond;                                    // 10
    //uint32    requireShapeshift;                          // 11 Shapeshift required (spellID)
    //float     unk_7;                                      // 12
};

// @author Selenium: 5.4 valid
struct SpellRuneCostEntry
{
    uint32  ID;                                             // 0
    uint32  RuneCost[4];                                    // 1-3 (0 = Blood, 1 = Frost, 2 = Unholy, 3 = Death)
    uint32  runePowerGain;                                  // 4

    bool NoRuneCost() const { return RuneCost[0] == 0 && RuneCost[1] == 0 && RuneCost[2] == 0 && RuneCost[3] == 0; }
    bool NoRunicPowerGain() const { return runePowerGain == 0; }
};

#define MAX_SHAPESHIFT_SPELLS 8

//SpellShapeshiftForm.dbc
// @author Selenium: 5.4 valid
struct SpellShapeshiftFormEntry
{
    uint32 ID;                                              // 0
    //uint32 buttonPosition;                                // 1 unused
    //char* Name;                                           // 2 unused
    uint32 flags1;                                          // 3
    int32  creatureType;                                    // 4 <=0 humanoid, other normal creature types
    //uint32 unk_1;                                         // 5 unused, related to next field
    uint32 attackSpeed;                                     // 6
    uint32 modelID_A;                                       // 7 alliance modelid (0 means no model)
    uint32 modelID_H;                                       // 8 All zeros
    //uint32 unk_2;                                         // 9 unused always 0
    //uint32 unk_3;                                         // 10 unused always 0
    uint32 stanceSpell[MAX_SHAPESHIFT_SPELLS];              // 11-18 spells which appear in the bar after shapeshifting
    //uint32 unk_4;                                         // 19 only fly form have data
    //uint32 unk_5;                                         // 20 always 0
};

// SpellShapeshift.dbc
// @author Selenium: 5.4 valid
struct SpellShapeshiftEntry
{
    uint32    Id;                                           // 0 - m_ID
    uint32    StancesNot;                                   // 3 - m_shapeshiftExclude
    // uint32 unk_1;                                        // 2 - always 0
    uint32    Stances;                                      // 1 - m_shapeshiftMask
    // uint32 unk_3;                                        // 4 - always 0
    // int32    StanceBarOrder;                             // 5 - m_stanceBarOrder not used
};

// SpellTargetRestrictions.dbc
// @author Selenium: 5.4 valid
struct SpellTargetRestrictionsEntry
{
    uint32  Id;                                             // 0 m_ID
    uint32  SpellId;                                        // 1 m_spellId
    //uint32    Unknown;                                    // 2
    float   MaxTargetRadius;                                // 3 m_maxTargetRadius
    //float    Unknown;                                     // 4 5.0.1.15589
    uint32  MaxAffectedTargets;                             // 5 m_maxTargets
    uint32  MaxTargetLevel;                                 // 6 m_maxTargetLevel
    uint32  TargetCreatureType;                             // 7 m_targetCreatureType
    uint32  Targets;                                        // 8 m_targets
};

// SpellReagents.dbc
// @author Selenium: 5.4 valid
struct SpellReagentsEntry
{
    //uint32    Id;                                         // 0        m_ID
    int32     Reagent[MAX_SPELL_REAGENTS];                  // 1-9      m_reagent
    uint32    ReagentCount[MAX_SPELL_REAGENTS];             // 10-18    m_reagentCount
};

struct SpellReagent
{
    SpellReagent()
    {
        reagents[0] = NULL;
        reagents[1] = NULL;
        reagents[2] = NULL;
        reagents[3] = NULL;
        reagents[4] = NULL;
        reagents[5] = NULL;
        reagents[6] = NULL;
        reagents[7] = NULL;
    }
    SpellReagentsEntry const* reagents[MAX_SPELL_REAGENTS];
};

typedef std::map<uint32, SpellReagent> SpellReagentMap;

// SpellScaling.dbc
// @author Selenium: 5.4 valid
struct SpellScalingEntry
{
    //uint32    Id;                                         // 0        m_ID
    int32     CastTimeMin;                                  // 1
    int32     CastTimeMax;                                  // 2
    int32     CastTimeMaxLevel;                             // 3
    int32     ScalingClass;                                 // 4        (index * 100) + charLevel - 1 => gtSpellScaling.dbc
    float     CoefBase;                                     // 5
    uint32    CoefLevelBase;                                // 6
    //uin32   unk505                                        // 7
    //uint32  unk505                                        // 8
};

// @author Selenium: 5.4 valid
struct SpellDurationEntry
{
    uint32    ID;
    int32     Duration[3];
};

// @author Selenium:  5.4 valid
// 3 new float on 5.4
struct SpellItemEnchantmentEntry
{
    uint32      ID;                                         // 0        m_ID
    //uint32      charges;                                  // 1        m_charges
    uint32      type[MAX_ITEM_ENCHANTMENT_EFFECTS];         // 2-4      m_effect[MAX_ITEM_ENCHANTMENT_EFFECTS]
    uint32      amount[MAX_ITEM_ENCHANTMENT_EFFECTS];       // 5-7      m_effectPointsMin[MAX_ITEM_ENCHANTMENT_EFFECTS]
    uint32      spellid[MAX_ITEM_ENCHANTMENT_EFFECTS];      // 11-13    m_effectArg[MAX_ITEM_ENCHANTMENT_EFFECTS]
    char*       description;                                // 14       m_name_lang
    uint32      aura_id;                                    // 15       m_itemVisual
    uint32      slot;                                       // 16       m_flags
    uint32      GemID;                                      // 17       m_src_itemID
    uint32      EnchantmentCondition;                       // 18       m_condition_id
    uint32      requiredSkill;                              // 19       m_requiredSkillID
    uint32      requiredSkillValue;                         // 20       m_requiredSkillRank
    uint32      requiredLevel;                              // 21       new in 3.1
    //uint32    unk_level                                   // 22       new in 3.1, look like level (max)?
    // uint32   unk_1
    // int32    unk_2
    // int32    unk_3
    // float    unk_4
    // float    unk_5
    // float    unk_6
};

//@todo
struct SpellItemEnchantmentConditionEntry
{
    uint32  ID;                                             // 0        m_ID
    uint8   Color[5];                                       // 1-5      m_lt_operandType[5]
    //uint32  LT_Operand[5];                                // 6-10     m_lt_operand[5]
    uint8   Comparator[5];                                  // 11-15    m_operator[5]
    uint8   CompareColor[5];                                // 15-20    m_rt_operandType[5]
    uint32  Value[5];                                       // 21-25    m_rt_operand[5]
    //uint8   Logic[5]                                      // 25-30    m_logic[5]
};


// SummonProperties.dbc
struct SummonPropertiesEntry
{
    uint32  Id;                                             // 0
    uint32  Category;                                       // 1, 0 - can't be controlled?, 1 - something guardian?, 2 - pet?, 3 - something controllable?, 4 - taxi/mount?, 5 ?
    uint32  Faction;                                        // 2, 14 rows > 0
    uint32  Type;                                           // 3, see enum
    uint32  Slot;                                           // 4, 0-6
    uint32  Flags;                                          // 5
};

// @author Selenium: 5.4 valid
struct TalentEntry
{
    uint32  Id;             // 0
    //uint32  unk;          // 1
    uint32  rank;           // 2
    //uint32  unk;          // 3
    uint32  spellId;        // 4
    //uint32  unk;          // 5 only 0
    //uint32  unk;          // 6 only 0
    //uint32  unk;          // 7 only 0
    uint32  classId;        // 8
    uint32  spellOverride;  // 9
    char*   description;    // 10

};

// @author Selenium:  5.4 valid
struct TaxiNodesEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    map_id;                                       // 1        m_ContinentID
    float     x;                                            // 2        m_x
    float     y;                                            // 3        m_y
    float     z;                                            // 4        m_z
    char* name;                                             // 5        m_Name_lang
    uint32    MountCreatureID[2];                           // 6-7      m_MountCreatureID[2]
    //uint32    unk_1                                       // 8
    //uint32    unk_2                                       // 9        faction group ? 1 for  Alliance, 2 for Horde
    //float     unk_3                                       // 10
    //float     unk_4                                       // 11
};

// @author Selenium: 5.4 valid
struct TaxiPathEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    from;                                         // 1        m_FromTaxiNode
    uint32    to;                                           // 2        m_ToTaxiNode
    uint32    price;                                        // 3        m_Cost
};

// @author Selenium: 5.4 valid
struct TaxiPathNodeEntry
{
    //uint32    ID;                                         // 0        m_ID
    uint32    path;                                         // 1        m_PathID
    uint32    index;                                        // 2        m_NodeIndex
    uint32    mapid;                                        // 3        m_ContinentID
    float     x;                                            // 4        m_LocX
    float     y;                                            // 5        m_LocY
    float     z;                                            // 6        m_LocZ
    uint32    actionFlag;                                   // 7        m_flags
    uint32    delay;                                        // 8        m_delay
    uint32    arrivalEventID;                               // 9        m_arrivalEventID
    uint32    departureEventID;                             // 10       m_departureEventID
};

// @author Selenium: 5.4 valid
struct TotemCategoryEntry
{
    uint32    ID;                                           // 0
    //char*   name;                                         // 1        m_name_lang
    uint32    categoryType;                                 // 2        m_totemCategoryType (one for specialization)
    uint32    categoryMask;                                 // 3        m_totemCategoryMask (compatibility mask for same type: different for totems, compatible from high to low for rods)
};

struct UnitPowerBarEntry
{
    uint32  Id;
    uint32  MinPower;
    uint32  MaxPower;
    uint32  StartPower;
    //uint32  Unk1;
    //float   Unk2;
    //float   Unk3;
    //uint32  BarType;
    //uint32  TextureFile[6];
    //uint32  Unk4[6];
    //uint32  DisplayFlags;
    //char*   PowerName;
    //char*   CostString;
    //char*   EmptyMessage;
    //char*   Tooltip;
    //float   StartInset;
    //float   EndInset;
};

struct TransportAnimationEntry
{
    //uint32  Id;
    uint32  TransportEntry;
    uint32  TimeSeg;
    float   X;
    float   Y;
    float   Z;
    //uint32  MovementId;
};

/*
struct TransportRotationEntry
{
    //uint32  Id;
    uint32  TransportEntry;
    uint32  TimeSeg;
    float   X;
    float   Y;
    float   Z;
    float   W;
};
*/

#define MAX_VEHICLE_SEATS 8

struct VehicleEntry
{
    uint32  m_ID;                                           // 0
    uint32  m_flags;                                        // 1
    //uint32    unk_1                                       // 2
    float   m_turnSpeed;                                    // 3
    float   m_pitchSpeed;                                   // 4
    float   m_pitchMin;                                     // 5
    float   m_pitchMax;                                     // 6
    uint32  m_seatID[MAX_VEHICLE_SEATS];                    // 7-15
    float   m_mouseLookOffsetPitch;                         // 16
    float   m_cameraFadeDistScalarMin;                      // 17
    float   m_cameraFadeDistScalarMax;                      // 18
    float   m_cameraPitchOffset;                            // 19
    float   m_facingLimitRight;                             // 20
    float   m_facingLimitLeft;                              // 21
    float   m_msslTrgtTurnLingering;                        // 22
    float   m_msslTrgtPitchLingering;                       // 23
    float   m_msslTrgtMouseLingering;                       // 24
    float   m_msslTrgtEndOpacity;                           // 25
    float   m_msslTrgtArcSpeed;                             // 26
    float   m_msslTrgtArcRepeat;                            // 27
    float   m_msslTrgtArcWidth;                             // 28
    float   m_msslTrgtImpactRadius[2];                      // 29-30
    char*   m_msslTrgtArcTexture;                           // 31
    char*   m_msslTrgtImpactTexture;                        // 32
    char*   m_msslTrgtImpactModel[2];                       // 33-34
    float   m_cameraYawOffset;                              // 35
    uint32  m_uiLocomotionType;                             // 36
    float   m_msslTrgtImpactTexRadius;                      // 37
    uint32  m_uiSeatIndicatorType;                          // 38
    uint32  m_powerType;                                    // 39, new in 3.1
    //uint32    unk_2                                       // 40, new in 3.1
    //uint32    unk_3                                       // 41, new in 3.1
};


struct VehicleSeatEntry
{
    uint32  m_ID;                                           // 0
    uint32  m_flags;                                        // 1
    int32   m_attachmentID;                                 // 2
    float   m_attachmentOffsetX;                            // 3
    float   m_attachmentOffsetY;                            // 4
    float   m_attachmentOffsetZ;                            // 5
    float   m_enterPreDelay;                                // 6
    float   m_enterSpeed;                                   // 7
    float   m_enterGravity;                                 // 8
    float   m_enterMinDuration;                             // 9
    float   m_enterMaxDuration;                             // 10
    float   m_enterMinArcHeight;                            // 11
    float   m_enterMaxArcHeight;                            // 12
    int32   m_enterAnimStart;                               // 13
    int32   m_enterAnimLoop;                                // 14
    int32   m_rideAnimStart;                                // 15
    int32   m_rideAnimLoop;                                 // 16
    int32   m_rideUpperAnimStart;                           // 17
    int32   m_rideUpperAnimLoop;                            // 18
    float   m_exitPreDelay;                                 // 19
    float   m_exitSpeed;                                    // 20
    float   m_exitGravity;                                  // 21
    float   m_exitMinDuration;                              // 22
    float   m_exitMaxDuration;                              // 23
    float   m_exitMinArcHeight;                             // 24
    float   m_exitMaxArcHeight;                             // 25
    int32   m_exitAnimStart;                                // 26
    int32   m_exitAnimLoop;                                 // 27
    int32   m_exitAnimEnd;                                  // 28
    float   m_passengerYaw;                                 // 29
    float   m_passengerPitch;                               // 30
    float   m_passengerRoll;                                // 31
    int32   m_passengerAttachmentID;                        // 32
    int32   m_vehicleEnterAnim;                             // 33
    int32   m_vehicleExitAnim;                              // 34
    int32   m_vehicleRideAnimLoop;                          // 35
    int32   m_vehicleEnterAnimBone;                         // 36
    int32   m_vehicleExitAnimBone;                          // 37
    int32   m_vehicleRideAnimLoopBone;                      // 38
    float   m_vehicleEnterAnimDelay;                        // 39
    float   m_vehicleExitAnimDelay;                         // 40
    uint32  m_vehicleAbilityDisplay;                        // 41
    uint32  m_enterUISoundID;                               // 42
    uint32  m_exitUISoundID;                                // 43
    uint32  m_flagsB;                                       // 44
    //float   Unk_1;                                        // 45
    //float   Unk_2;                                        // 46
    //float   Unk_3;                                        // 47
    //float   Unk_4;                                        // 48
    //float   Unk_5;                                        // 59
    //float   Unk_6;                                        // 50
    //float   Unk_7;                                        // 51
    //float   Unk_8;                                        // 52
    //float   Unk_9;                                        // 53
    //float   Unk_10;                                       // 54
    //float   Unk_11;                                       // 55
    //int32   Unk_12;                                       // 56
    //int32   Unk_13;                                       // 57
    //int32   Unk_14;                                       // 58
    //int32   Unk_15;                                       // 59
    //int32   Unk_16;                                       // 60
    //int32   Unk_17;                                       // 61
    //int32   Unk_18;                                       // 62
    //int32   Unk_19;                                       // 63
    //int32   Unk_flags;                                    // 64
    //int32   Unk_21;                                       // 65

    bool CanEnterOrExit() const { return m_flags & VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT; }
    bool CanSwitchFromSeat() const { return m_flags & VEHICLE_SEAT_FLAG_CAN_SWITCH; }
    bool IsUsableByOverride() const { return (m_flags & VEHICLE_SEAT_FLAG_UNCONTROLLED)
                                        || (m_flagsB & (VEHICLE_SEAT_FLAG_B_USABLE_FORCED
                                        | VEHICLE_SEAT_FLAG_B_USABLE_FORCED_2
                                        | VEHICLE_SEAT_FLAG_B_USABLE_FORCED_3 | VEHICLE_SEAT_FLAG_B_USABLE_FORCED_4)); }
    bool IsEjectable() const { return m_flagsB & VEHICLE_SEAT_FLAG_B_EJECTABLE; }
};

// @author Selenium: 5.4 valid
struct WMOAreaTableEntry
{
    uint32 Id;                                              // 0 index
    int32 rootId;                                           // 1 used in root WMO
    int32 adtId;                                            // 2 used in adt file
    int32 groupId;                                          // 3 used in group WMO
    //uint32 unk_1;                                         // 4
    //uint32 unk_2;                                         // 5
    //uint32 unk_3;                                         // 6
    //uint32 unk_4;                                         // 7
    //uint32 unk_5;                                         // 8
    uint32 Flags;                                           // 9 used for indoor/outdoor determination
    uint32 areaId;                                          // 10 link to AreaTableEntry.ID
    //char *Name;                                           // 11 m_AreaName_lang
    //uint32 unk_6;                                         // 12
    //uint32 unk_7;                                         // 13
    //uint32 unk_8;                                         // 14
};

// @author Selenium: 5.4 valid
struct WorldMapAreaEntry
{
    //uint32  ID;                                           // 0
    uint32  map_id;                                         // 1
    uint32  area_id;                                        // 2 index (continent 0 areas ignored)
    //char* internal_name                                   // 3 Map name
    float   y1;                                             // 4
    float   y2;                                             // 5
    float   x1;                                             // 6
    float   x2;                                             // 7
    int32   virtual_map_id;                                 // 8 -1 (map_id have correct map) other: virtual map where zone show (map_id - where zone in fact internally)
    // int32   dungeonMap_id;                               // 9 pointer to DungeonMap.dbc (owerride x1, x2, y1, y2 coordinates)
    // uint32  parentMapID;                                 // 10
    // uint32  Unk_1                                        // 11 Flag ?
    uint32  minRecommendedLevel;                            // 12 Minimum recommended level displayed on world map
    uint32  maxRecommendedLevel;                            // 13 Maximum recommended level displayed on world map
};

#define MAX_WORLD_MAP_OVERLAY_AREA_IDX 4

// @author Selenium: 5.4 valid
struct WorldMapOverlayEntry
{
    uint32    ID;                                           // 0
    //uint32    worldMapAreaId;                             // 1 idx in WorldMapArea.dbc
    uint32    areatableID[MAX_WORLD_MAP_OVERLAY_AREA_IDX];  // 2-5
    //char* internal_name;                                  // 6
    //uint32 unk_1;                                         // 7 m_textureWidth
    //uint32 unk_2;                                         // 8 m_textureHeight
    //uint32 unk_3;                                         // 9 m_offsetX
    //uint32 unk_4;                                         // 10 m_offsetY
    //uint32 unk_5;                                         // 11 m_hitRectTop
    //uint32 unk_6;                                         // 12 m_hitRectLeft
    //uint32 unk_7;                                         // 13 m_hitRectBottom
    //uint32 unk_8;                                         // 14 m_hitRectRight
    //uint32 unk_9;                                         // 15 unk 5.0.5
};

struct WorldSafeLocsEntry
{
    uint32    ID;                                           // 0
    uint32    map_id;                                       // 1
    float     x;                                            // 2
    float     y;                                            // 3
    float     z;                                            // 4
    //float   unk;                                          // 5
    //char*   name;                                         // 6 m_AreaName_lang
};


// @author Selenium: 5.4 valid
//UNUSED ACTUALY
/*
struct WorldStateUI
{
    uint32    ID;                                           // 0
    uint32    map_id;                                       // 1 Can be -1 to show up everywhere.
    uint32    zone;                                         // 2 Can be zero for "everywhere".
    uint32    phaseMask;                                    // 3 Phase this WorldState is avaliable in
    uint32    icon;                                         // 4 The icon that is used in the interface.
    //uint32    Unk_1                                       // 5 Only 0
    char*     textureFilename;                              // 6
    char*     WorldstateString;                             // 7 The worldstate text
    char*     description;                                  // 8 Text shown when hovering mouse on icon
    uint32    worldstateID;                                 // 9 This is the actual ID used
    uint32    type;                                         // 10 0 = unknown, 1 = unknown, 2 = not shown in ui, 3 = wintergrasp
    uint32    path_icon;                                    // 11
    uint32    description_2;                                // 12
    uint32    unk_2;                                        // 13
    uint32    unk_3;                                        // 14 Used for some progress bars.
    uint32    unk_4;                                        // 15 Unused (only 0)
};
*/

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

struct VectorArray
{
    std::vector<std::string> stringVectorArray[2];
};

typedef std::map<uint32, VectorArray> NameGenVectorArraysMap;

// Structures not used for casting to loaded DBC data and not required then packing
// @author Selenium: 5.4 valid
struct MapDifficulty
{
    MapDifficulty() : resetTime(0), maxPlayers(0), hasErrorMessage(false) {}
    MapDifficulty(uint32 _resetTime, uint32 _maxPlayers, bool _hasErrorMessage) : resetTime(_resetTime), maxPlayers(_maxPlayers), hasErrorMessage(_hasErrorMessage) {}

    uint32 resetTime;
    uint32 maxPlayers;
    bool hasErrorMessage;
};

struct TalentSpellPos
{
    TalentSpellPos() : talent_id(0), rank(0) {}
    TalentSpellPos(uint16 _talent_id, uint8 _rank) : talent_id(_talent_id), rank(_rank) {}

    uint16 talent_id;
    uint8  rank;
};

typedef std::map<uint32, TalentSpellPos> TalentSpellPosMap;

struct SpellEffect
{
    SpellEffect()
    {
        for (int i = 0; i < MAX_DIFFICULTY; i++)
            for (uint8 y = 0; y < MAX_SPELL_EFFECTS; y++)
                effects[i][y] = 0;
    }

    SpellEffectEntry const* effects[MAX_DIFFICULTY][32];
};

typedef std::map<uint32, SpellEffect> SpellEffectMap;

struct TaxiPathBySourceAndDestination
{
    TaxiPathBySourceAndDestination() : ID(0), price(0) {}
    TaxiPathBySourceAndDestination(uint32 _id, uint32 _price) : ID(_id), price(_price) {}

    uint32    ID;
    uint32    price;
};
typedef std::map<uint32, TaxiPathBySourceAndDestination> TaxiPathSetForSource;
typedef std::map<uint32, TaxiPathSetForSource> TaxiPathSetBySource;

struct TaxiPathNodePtr
{
    TaxiPathNodePtr() : i_ptr(NULL) {}
    TaxiPathNodePtr(TaxiPathNodeEntry const* ptr) : i_ptr(ptr) {}
    TaxiPathNodeEntry const* i_ptr;
    operator TaxiPathNodeEntry const& () const { return *i_ptr; }
};

typedef Path<TaxiPathNodePtr, TaxiPathNodeEntry const> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;

// Elevators and other types of transports not included in TaxiPathNode.dbc, but in TransportAnimation.dbc.
typedef UNORDERED_MAP<uint32 /*frame*/, TransportAnimationEntry const*> TransportAnimationEntryMap;
typedef UNORDERED_MAP<uint32, TransportAnimationEntryMap> TransportAnimationsByEntry;

#define TaxiMaskSize 162
typedef uint8 TaxiMask[TaxiMaskSize];
#endif