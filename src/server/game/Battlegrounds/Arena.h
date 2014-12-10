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
 
#ifndef ARENATEAM_H
#define ARENATEAM_H
 
#include "QueryResult.h"
#include <ace/Singleton.h>
#include <list>
#include <map>
 
class WorldSession;
class WorldPacket;
class Player;
class Group;

enum ArenaType
{
    ARENA_TYPE_2v2          = 2,
    ARENA_TYPE_3v3          = 3,
    ARENA_TYPE_5v5          = 5
};

enum ArenaSlots
{
    SLOT_ARENA_2V2          = 0,
    SLOT_ARENA_3V3          = 1,
    SLOT_ARENA_5V5          = 2,

    MAX_ARENA_SLOT          = 3,

    SLOT_RBG                = 3,

    MAX_PVP_SLOT            = 4
};

struct ArenaTeamMember
{
    uint64 Guid;
    std::string Name;
    uint8 Class;
    uint16 WeekGames;
    uint16 WeekWins;
    uint16 SeasonGames;
    uint16 SeasonWins;
    uint16 PersonalRating;
    uint16 MatchMakerRating;
 
    void ModifyPersonalRating(Player* player, int32 mod, uint32 type);
    void ModifyMatchmakerRating(int32 mod, uint32 slot);
};
 
struct ArenaTeamStats
{
    uint16 Rating;
    uint16 WeekGames;
    uint16 WeekWins;
    uint16 SeasonGames;
    uint16 SeasonWins;
    uint32 Rank;
};

namespace Arena
{
    inline uint8 GetSlotByType(uint32 type)
    {
        switch (type)
        {
            case ARENA_TYPE_2v2: return SLOT_ARENA_2V2;
            case ARENA_TYPE_3v3: return SLOT_ARENA_3V3;
            case ARENA_TYPE_5v5: return SLOT_ARENA_5V5;

            default:             break;
        }

        sLog->outError(LOG_FILTER_ARENAS, "FATAL: Unknown arena team type %u for some arena team", type);
        return 0xFF;
    }
 
    inline uint8 GetTypeBySlot(uint8 slot)
    {
        switch (slot)
        {
            case 0:        return ARENA_TYPE_2v2;
            case 1:        return ARENA_TYPE_3v3;
            case 2:        return ARENA_TYPE_5v5;

            default:       break;
        }

        sLog->outError(LOG_FILTER_ARENAS, "FATAL: Unknown arena team slot %u for some arena team", slot);
        return 0xFF;
    }
 
    inline float GetChanceAgainst(uint32 ownRating, uint32 opponentRating)
    {
        // Returns the chance to win against a team with the given rating, used in the rating adjustment calculation. ELO system.
        return 1.0f / (1.0f + exp(log(10.0f) * (float)((float)opponentRating - (float)ownRating) / 650.0f));
    }
 
    inline int32 GetRatingMod(uint32 ownRating, uint32 opponentRating, bool won /*, float confidence_factor*/)
    {
        // 'Chance' calculation - to beat the opponent
        // This is a simulation. Not much info on how it really works
        float chance = GetChanceAgainst(ownRating, opponentRating);
        float won_mod = (won) ? 1.0f : 0.0f;
 
        // Calculate the rating modification
        float mod;
 
        // TODO: Replace this hack with using the confidence factor (limiting the factor to 2.0f)
        if (won && ownRating < 1300)
        {
            if (ownRating < 1000)
                mod = 48.0f * (won_mod - chance);
            else
                mod = (24.0f + (24.0f * (1300.0f - float(ownRating)) / 300.0f)) * (won_mod - chance);
        }
        else mod = 24.0f * (won_mod - chance);
 
        return (int32)ceil(mod);
    }

    inline int32 GetMatchmakerRatingMod(uint32 ownRating, uint32 opponentRating, bool won /*, float& confidence_factor*/)
    {
        // 'Chance' calculation - to beat the opponent
        // This is a simulation. Not much info on how it really works
        float chance = GetChanceAgainst(ownRating, opponentRating);
        float won_mod = (won) ? 1.0f : 0.0f;
        float mod = won_mod - chance;
 
        // Work in progress:
        /*
        // This is a simulation, as there is not much info on how it really works
        float confidence_mod = min(1.0f - fabs(mod), 0.5f);
 
        // Apply confidence factor to the mod:
        mod *= confidence_factor
 
        // And only after that update the new confidence factor
        confidence_factor -= ((confidence_factor - 1.0f) * confidence_mod) / confidence_factor;
        */
 
        // Real rating modification
        mod *= 24.0f;
 
        return (int32)ceil(mod);
    }
};
 
#endif
