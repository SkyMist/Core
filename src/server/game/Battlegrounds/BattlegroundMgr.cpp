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
#include "ObjectMgr.h"
#include "Arena.h"
#include "Group.h"
#include "World.h"
#include "WorldPacket.h"

#include "BattlegroundMgr.h"
#include "BattlegroundAV.h"
#include "BattlegroundAB.h"
#include "BattlegroundEY.h"
#include "BattlegroundWS.h"
#include "BattlegroundNA.h"
#include "BattlegroundBE.h"
#include "BattlegroundAA.h"
#include "BattlegroundRL.h"
#include "BattlegroundSA.h"
#include "BattlegroundDS.h"
#include "BattlegroundRV.h"
#include "BattlegroundIC.h"
#include "BattlegroundRB.h"
#include "BattlegroundTP.h"
#include "BattlegroundBFG.h"
#include "BattlegroundRatedBG.h"
#include "BattlegroundTK.h"
#include "BattlegroundDG.h"
#include "BattlegroundSM.h"
#include "BattlegroundTA.h"
#include "BattlegroundTTP.h"

#include "Chat.h"
#include "Map.h"
#include "MapInstanced.h"
#include "MapManager.h"
#include "Player.h"
#include "GameEventMgr.h"
#include "SharedDefines.h"
#include "Formulas.h"
#include "DisableMgr.h"
#include "Opcodes.h"
#include "LFG.h"

/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

BattlegroundMgr::BattlegroundMgr() : m_NextRatedArenaUpdate(sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER)), m_ArenaTesting(false), m_Testing(false)
{
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        m_Battlegrounds[i].clear();
}

BattlegroundMgr::~BattlegroundMgr()
{
    DeleteAllBattlegrounds();
}

void BattlegroundMgr::DeleteAllBattlegrounds()
{
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        for (BattlegroundSet::iterator itr = m_Battlegrounds[i].begin(); itr != m_Battlegrounds[i].end();)
        {
            Battleground* bg = itr->second;
            m_Battlegrounds[i].erase(itr++);
            if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());
            delete bg;
        }
    }

    // destroy template battlegrounds that listed only in queues (other already terminated)
    for (uint32 bgTypeId = 0; bgTypeId < MAX_BATTLEGROUND_TYPE_ID; ++bgTypeId)
    {
        // ~Battleground call unregistring BG from queue
        while (!BGFreeSlotQueue[bgTypeId].empty())
            delete BGFreeSlotQueue[bgTypeId].front();
    }
}

// used to update running battlegrounds, and delete finished ones
void BattlegroundMgr::Update(uint32 diff)
{
    BattlegroundSet::iterator itr, next;
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        itr = m_Battlegrounds[i].begin();
        // skip updating battleground template
        if (itr != m_Battlegrounds[i].end())
            ++itr;
        for (; itr != m_Battlegrounds[i].end(); itr = next)
        {
            next = itr;
            ++next;
            itr->second->Update(diff);
            // use the SetDeleteThis variable
            // direct deletion caused crashes
            if (itr->second->ToBeDeleted())
            {
                Battleground* bg = itr->second;
                m_Battlegrounds[i].erase(itr);
                if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                    m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());

                delete bg;
            }
        }
    }

    // update events timer
    for (int qtype = BATTLEGROUND_QUEUE_NONE; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        m_BattlegroundQueues[qtype].UpdateEvents(diff);

    // update scheduled queues
    if (!m_QueueUpdateScheduler.empty())
    {
        std::vector<QueueSchedulerItem*> scheduled;
        {
            //copy vector and clear the other
            scheduled = std::vector<QueueSchedulerItem*>(m_QueueUpdateScheduler);
            m_QueueUpdateScheduler.clear();
            //release lock
        }

        for (uint8 i = 0; i < scheduled.size(); i++)
        {
            uint32 arenaMMRating = scheduled[i]->_arenaMMRating;
            uint8 arenaType = scheduled[i]->_arenaType;
            BattlegroundQueueTypeId bgQueueTypeId = scheduled[i]->_bgQueueTypeId;
            BattlegroundTypeId bgTypeId = scheduled[i]->_bgTypeId;
            BattlegroundBracketId bracket_id = scheduled[i]->_bracket_id;
            m_BattlegroundQueues[bgQueueTypeId].BattlegroundQueueUpdate(diff, bgTypeId, bracket_id, arenaType, arenaMMRating > 0, arenaMMRating);
        }
    }

    // if rating difference counts, maybe force-update queues
    if (sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE) && sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER))
    {
        // it's time to force update
        if (m_NextRatedArenaUpdate < diff)
        {
            // forced update for rated arenas (scan all, but skipped non rated)
            TC_LOG_TRACE("bg.arena", "BattlegroundMgr: UPDATING ARENA QUEUES");
            for (int qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
                for (int bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                    m_BattlegroundQueues[qtype].BattlegroundQueueUpdate(diff,
                        BATTLEGROUND_AA, BattlegroundBracketId(bracket),
                        BattlegroundMgr::BGArenaType(BattlegroundQueueTypeId(qtype)), true, 0);

            m_NextRatedArenaUpdate = sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER);
        }
        else
            m_NextRatedArenaUpdate -= diff;
    }
}

void BattlegroundMgr::BuildBattlegroundStatusPacket(WorldPacket* data, Battleground* bg, Player* player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype, GroupJoinBattlegroundResult result /*= ERR_BATTLEGROUND_NONE*/)
{
    if (!bg)
        return; // StatusID = STATUS_NONE;

    ObjectGuid playerGuid = player->GetGUID();
    ObjectGuid bgGuid = bg ? bg->GetGUID() : 0;

    switch (StatusID)
    {
        case STATUS_NONE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS);

            data->WriteBit(playerGuid[2]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(playerGuid[5]);
            data->WriteBit(playerGuid[0]);

            data->FlushBits();

            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)

            data->WriteByteSeq(playerGuid[4]);
            data->WriteByteSeq(playerGuid[2]);
            data->WriteByteSeq(playerGuid[1]);
            data->WriteByteSeq(playerGuid[6]);

            *data << uint32(Time1);                     // Join Time

            data->WriteByteSeq(playerGuid[3]);
            data->WriteByteSeq(playerGuid[5]);

            *data << uint32(QueueSlot);                 // Queue slot

            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[7]);

            break;
        }
        case STATUS_WAIT_QUEUE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_QUEUED);

            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            *data << uint32(Time1);                     // Estimated Wait Time
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(GetMSTimeDiffToNow(Time2)); // Time since joined
            //if (bg->isRated())
            //    *data << uint8(count for players in queue Rated Mode);
            //else
                *data << uint8(0);                      // Team Size
            *data << uint32(Time2);                     // Join Time
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint8(bg->GetMaxLevel());          // Max Level

            data->WriteBit(bgGuid[1]);
            data->WriteBit(1);                          // Eligible In Queue
            data->WriteBit(bgGuid[5]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(playerGuid[2]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(0);                          // Waiting On Other Activity
            data->WriteBit(playerGuid[7]);
            data->WriteBit(playerGuid[0]);
            data->WriteBit(bg->isArena());              // Join Failed, 1 when it's arena ...
            data->WriteBit(bgGuid[4]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[5]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(bgGuid[2]);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(bgGuid[0]);

            data->FlushBits();

            data->WriteByteSeq(playerGuid[4]);
            data->WriteByteSeq(bgGuid[5]);
            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(playerGuid[6]);
            data->WriteByteSeq(playerGuid[1]);
            data->WriteByteSeq(bgGuid[3]);
            data->WriteByteSeq(bgGuid[7]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[2]);
            data->WriteByteSeq(bgGuid[2]);
            data->WriteByteSeq(playerGuid[7]);
            data->WriteByteSeq(playerGuid[5]);
            data->WriteByteSeq(playerGuid[3]);
            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(bgGuid[0]);

            break;
        }
        case STATUS_WAIT_JOIN:
        {
            bool HasRoles = (player && player->GetBattleGroundRoles() && player->GetBattleGroundRoles() != lfg::PLAYER_ROLE_DAMAGE) ? true : false;

            data->Initialize(SMSG_BATTLEFIELD_STATUS_CONFIRM);

            data->WriteBit(playerGuid[3]);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(bgGuid[5]);
            data->WriteBit(playerGuid[2]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(bgGuid[4]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(playerGuid[0]);
            data->WriteBit(bgGuid[2]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(!HasRoles);
            data->WriteBit(bgGuid[1]);
            data->WriteBit(bgGuid[0]);
            data->WriteBit(playerGuid[5]);

            data->FlushBits();

            data->WriteByteSeq(bgGuid[3]);
            *data << uint32(QueueSlot);                 // Queue slot
            data->WriteByteSeq(playerGuid[2]);
            data->WriteByteSeq(bgGuid[7]);
            data->WriteByteSeq(playerGuid[5]);
            *data << uint32(bg->GetMapId());            // Map Id
            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[4]);
            data->WriteByteSeq(bgGuid[0]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(bgGuid[5]);
            data->WriteByteSeq(playerGuid[7]);
            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint8(bg->GetMaxLevel());          // Max Level
            data->WriteByteSeq(playerGuid[6]);
            *data << uint32(Time2);                     // Join Time

            if (HasRoles)
                *data << uint8(player->GetBattleGroundRoles() == lfg::PLAYER_ROLE_TANK ? 0 : 1); // Client uses sent value like this: Role = 1 << (val + 1).

            data->WriteByteSeq(playerGuid[1]);
            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            //if (bg->isRated())
            //    *data << uint8(count for players in queue Rated Mode);
            //else
                *data << uint8(0);                      // Team Size
            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(bgGuid[2]);
            *data << uint32(Time1);                     // Time until closed
            data->WriteByteSeq(playerGuid[3]);
            *data << uint8(bg->GetMinLevel());          // Min Level

            break;
        }
        case STATUS_ERROR:
        {
            // call BuildPacketFailed to send the error to the clients
            BuildStatusFailedPacket(data, bg, player, QueueSlot, result);
            break;
        }
        case STATUS_IN_PROGRESS:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_ACTIVE);

            data->WriteBit(playerGuid[5]);
            data->WriteBit(bgGuid[4]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(bgGuid[5]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(bgGuid[0]);
            data->WriteBit(bgGuid[2]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[0]);
            data->WriteBit(player->GetBGTeam() == HORDE ? 0 : 1);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(playerGuid[2]);
            data->WriteBit(bgGuid[1]);
            data->WriteBit(0);                          // unk

            data->FlushBits();

            data->WriteByteSeq(bgGuid[7]);
            *data << uint8(bg->GetMinLevel());          // Min Level
            data->WriteByteSeq(playerGuid[6]);
            *data << uint32(bg->GetRemainingTime());    // Remaining Time
            *data << uint8(bg->GetMaxLevel());          // Max Level
            data->WriteByteSeq(bgGuid[2]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(playerGuid[3]);
            data->WriteByteSeq(bgGuid[3]);
            *data << uint32(Time1);                     // Join Time
            data->WriteByteSeq(bgGuid[5]);
            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID or faction ?
            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(playerGuid[7]);
            data->WriteByteSeq(playerGuid[5]);
            data->WriteByteSeq(playerGuid[0]);
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            data->WriteByteSeq(playerGuid[4]);
            //if (bg->isRated())
            //    *data << uint8(count for players in queue Rated Mode);
            //else
                *data << uint8(0);                      // Team Size
            data->WriteByteSeq(bgGuid[0]);
            *data << uint32(bg->GetMapId());            // Map Id
            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(playerGuid[2]);
            data->WriteByteSeq(playerGuid[1]);
            *data << uint32(Time2);                     // Elapsed Time

            break;
        }
        case STATUS_WAIT_LEAVE:
            break;
    }
}

void BattlegroundMgr::BuildPvpLogDataPacket(WorldPacket* data, Battleground* bg)
{
    ObjectGuid Guid1 = 0;
    ObjectGuid Guid2 = 0;
    ByteBuffer buff;

    uint8 isRated = (bg->isRated() ? 1 : 0);               // Type (normal = 0 / rated = 1) -- Arena (goes with isArena true) or Rated BG (with isArena false).
    uint8 isArena = (bg->isArena() ? 1 : 0);               // Arena names

    bool bgEnded = (bg->GetStatus() == STATUS_WAIT_LEAVE) ? true : false;

    data->Initialize(SMSG_PVP_LOG_DATA);
    data->WriteBit(isRated);                               // this is more like hasRatedPart
    data->WriteBit(isArena);                               // actually is hasTeamNames

    if (isArena)
    {
        if (bg->GetWinner() < WINNER_NONE) // Must have a winner.
        {
            // Alliance team is Gold, Horde team is Green.
            Group* WinnerArenaTeam = bg->GetBgRaid(bg->GetWinner());
            Group* LoserArenaTeam  = bg->GetBgRaid(bg->GetOtherTeam(bg->GetWinner()));

            Player* winnerLeader = ObjectAccessor::FindPlayer(WinnerArenaTeam->GetLeaderGUID());
            Player* loserLeader = ObjectAccessor::FindPlayer(LoserArenaTeam->GetLeaderGUID());

            data->WriteBits(winnerLeader->GetName().size(), 7);
            data->WriteBit(Guid2[3]);
            data->WriteBit(Guid2[2]);
            data->WriteBit(Guid1[7]);
            data->WriteBit(Guid1[2]);
            data->WriteBit(Guid1[3]);
            data->WriteBit(Guid2[4]);
            data->WriteBits(loserLeader->GetName().size(), 7);
            data->WriteBit(Guid2[7]);
            data->WriteBit(Guid1[6]);
            data->WriteBit(Guid1[4]);
            data->WriteBit(Guid2[6]);
            data->WriteBit(Guid2[0]);
            data->WriteBit(Guid1[0]);
            data->WriteBit(Guid2[5]);
            data->WriteBit(Guid1[1]);
            data->WriteBit(Guid1[5]);
            data->WriteBit(Guid2[1]);
        }
    }

    data->WriteBits(bg->GetPlayerScoresSize(), 19);
    Battleground::BattlegroundPlayerMap const& bgPlayers = bg->GetPlayers();

    for (Battleground::BattlegroundScoreMap::const_iterator itr = bg->GetPlayerScoresBegin(); itr != bg->GetPlayerScoresEnd(); ++itr)
    {
        if (!bg->IsPlayerInBattleground(itr->first))
        {
            TC_LOG_ERROR("bg.battleground", "Player " UI64FMTD " has scoreboard entry for battleground %u but is not in battleground!", itr->first, bg->GetTypeID(true));
            continue;
        }

        uint32 team;
        int32 primaryTree;
        if (Player* player = ObjectAccessor::FindPlayer(itr->first))
        {
            team = player->GetBGTeam();
            primaryTree = player->GetTalentSpecialization(player->GetActiveSpec());
        }
        else
        {
            Battleground::BattlegroundPlayerMap::const_iterator itr2 = bgPlayers.find(itr->first);
            if (itr2 == bgPlayers.end())
            {
                TC_LOG_ERROR("bg.battleground", "Player " UI64FMTD " has scoreboard entry for battleground %u but do not have battleground data!", itr->first, bg->GetTypeID(true));
                continue;
            }

            team = itr2->second.Team;
            primaryTree = itr2->second.PrimaryTree;
        }

        ObjectGuid playerGUID = itr->first;
        BattlegroundScore* score = itr->second;
        ByteBuffer scores;

        // Rated BG rating stuff.
        uint8 hasRatedBGRating = ((isRated && !isArena) ? 1 : 0); // Kinda like a rated bg check.
        uint32 RatedBGRating = 0;

        if (Player* player = ObjectAccessor::FindPlayer(playerGUID)) // Get the rating if player is online.
            RatedBGRating = (bg->GetWinner() == player->GetBGTeam() ? (player->GetRatedBGRating(playerGUID) - 20) : 
        (player->GetRatedBGRating(playerGUID) > 1500 ? (player->GetRatedBGRating(playerGUID) + 20) : player->GetRatedBGRating(playerGUID)));

        uint8 hasBGRatingChange = (hasRatedBGRating ? 1 : 0);
        int32 RatedBGRatingChange = 0;

        if (Player* player = ObjectAccessor::FindPlayer(playerGUID)) // Get the rating change if player is online.
            RatedBGRatingChange = (bg->GetWinner() == player->GetBGTeam() ? 20 : (player->GetRatedBGRating(playerGUID) > 1500 ? -20 : 0));

        data->WriteBit(playerGUID[4]);
        data->WriteBit(0);                              // Unk 1
        data->WriteBit(playerGUID[0]);
        data->WriteBit(bg->isBattleground());           // isBattleground or isNotArena
        data->WriteBit(playerGUID[7]);
        data->WriteBit(playerGUID[5]);
        data->WriteBit(hasBGRatingChange);              // hasBGRatingChange
        data->WriteBit(playerGUID[1]);
        data->WriteBit(0);                              // hasMMRChange  - After 4.2 update is always zero (number).
        data->WriteBit(team == HORDE ? 0 : 1);          // isAlliancePlayer

        switch (bg->GetTypeID(true))                    // Custom values
        {
            case BATTLEGROUND_RB:
            case BATTLEGROUND_RATED_10_VS_10:
                switch (bg->GetMapId())
                {
                    case 30: // Alterac Valley
                        data->WriteBits(0x00000005, 22);
                        scores << uint32(((BattlegroundAVScore*)score)->GraveyardsAssaulted); // GraveyardsAssaulted
                        scores << uint32(((BattlegroundAVScore*)score)->GraveyardsDefended);  // GraveyardsDefended
                        scores << uint32(((BattlegroundAVScore*)score)->TowersAssaulted);     // TowersAssaulted
                        scores << uint32(((BattlegroundAVScore*)score)->TowersDefended);      // TowersDefended
                        scores << uint32(((BattlegroundAVScore*)score)->MinesCaptured);       // MinesCaptured
                        break;
                    case 489: // Warsong Gulch
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundWGScore*)score)->FlagCaptures);        // flag captures
                        scores << uint32(((BattlegroundWGScore*)score)->FlagReturns);         // flag returns
                        break;
                    case 529: // Arathi Basin
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundABScore*)score)->BasesAssaulted);      // bases assaulted
                        scores << uint32(((BattlegroundABScore*)score)->BasesDefended);       // bases defended
                        break;
                    case 566: // Eye of the Storm
                        data->WriteBits(0x00000001, 22);
                        scores << uint32(((BattlegroundEYScore*)score)->FlagCaptures);        // flag captures
                        break;
                    case 607: // Strand of the Ancients
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundSAScore*)score)->demolishers_destroyed);
                        scores << uint32(((BattlegroundSAScore*)score)->gates_destroyed);
                        break;
                    case 628: // Isle of Conquest
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundICScore*)score)->BasesAssaulted);       // bases assaulted
                        scores << uint32(((BattlegroundICScore*)score)->BasesDefended);        // bases defended
                        break;
                    case 726: // Twin Peaks
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundTPScore*)score)->FlagCaptures);         // flag captures
                        scores << uint32(((BattlegroundTPScore*)score)->FlagReturns);          // flag returns
                        break;
                    case 761: // Battle for Gilneas
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundBFGScore*)score)->BasesAssaulted);      // bases assaulted
                        scores << uint32(((BattlegroundBFGScore*)score)->BasesDefended);       // bases defended
                        break;
                    case 998: // Temple of Kotmogu
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundTKScore*)score)->OrbHandles);           // orb captures
                        scores << uint32(((BattlegroundTKScore*)score)->VictoryPoints);        // victory points
                        break;
                    case 1105: // Deepwind Gorge
                        data->WriteBits(0x00000002, 22);
                        scores << uint32(((BattlegroundDGScore*)score)->MinesAssaulted);      // bases assaulted
                        scores << uint32(((BattlegroundDGScore*)score)->MinesDefended);       // bases defended
                        scores << uint32(((BattlegroundDGScore*)score)->CartCaptures);         // gold cart captures
                        scores << uint32(((BattlegroundDGScore*)score)->CartReturns);         // gold cart returns
                        break;
                    case 727: // Silvershard Mines
                        data->WriteBits(0x00000002, 22);
                        // scores << uint32(((BattlegroundSMScore*)score)->CartsAssaulted);      // gem carts assaulted
                        // scores << uint32(((BattlegroundSMScore*)score)->CartsDefended);       // gem carts defended
                        // scores << uint32(((BattlegroundSMScore*)score)->CartCaptures);        // gem cart captures
                        break;

                    default:
                        data->WriteBits(0, 22);
                        break;
                }
                break;
            case BATTLEGROUND_AV:
                data->WriteBits(0x00000005, 22);
                scores << uint32(((BattlegroundAVScore*)score)->GraveyardsAssaulted); // GraveyardsAssaulted
                scores << uint32(((BattlegroundAVScore*)score)->GraveyardsDefended);  // GraveyardsDefended
                scores << uint32(((BattlegroundAVScore*)score)->TowersAssaulted);     // TowersAssaulted
                scores << uint32(((BattlegroundAVScore*)score)->TowersDefended);      // TowersDefended
                scores << uint32(((BattlegroundAVScore*)score)->MinesCaptured);       // MinesCaptured
                break;
            case BATTLEGROUND_WS:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundWGScore*)score)->FlagCaptures);        // flag captures
                scores << uint32(((BattlegroundWGScore*)score)->FlagReturns);         // flag returns
                break;
            case BATTLEGROUND_AB:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundABScore*)score)->BasesAssaulted);      // bases assaulted
                scores << uint32(((BattlegroundABScore*)score)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_EY:
                data->WriteBits(0x00000001, 22);
                scores << uint32(((BattlegroundEYScore*)score)->FlagCaptures);        // flag captures
                break;
            case BATTLEGROUND_SA:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundSAScore*)score)->demolishers_destroyed);
                scores << uint32(((BattlegroundSAScore*)score)->gates_destroyed);
                break;
            case BATTLEGROUND_IC:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundICScore*)score)->BasesAssaulted);       // bases assaulted
                scores << uint32(((BattlegroundICScore*)score)->BasesDefended);        // bases defended
                break;
            case BATTLEGROUND_TP:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundTPScore*)score)->FlagCaptures);         // flag captures
                scores << uint32(((BattlegroundTPScore*)score)->FlagReturns);          // flag returns
                break;
            case BATTLEGROUND_BFG:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundBFGScore*)score)->BasesAssaulted);      // bases assaulted
                scores << uint32(((BattlegroundBFGScore*)score)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_TOK:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundTKScore*)score)->OrbHandles);           // orb captures
                scores << uint32(((BattlegroundTKScore*)score)->VictoryPoints);        // victory points
                break;
            case BATTLEGROUND_DG:
                data->WriteBits(0x00000002, 22);
                scores << uint32(((BattlegroundDGScore*)score)->MinesAssaulted);      // bases assaulted
                scores << uint32(((BattlegroundDGScore*)score)->MinesDefended);       // bases defended
                scores << uint32(((BattlegroundDGScore*)score)->CartCaptures);         // gold cart captures
                scores << uint32(((BattlegroundDGScore*)score)->CartReturns);         // gold cart returns
                break;
            case BATTLEGROUND_SM:
                data->WriteBits(0x00000002, 22);
                // scores << uint32(((BattlegroundSMScore*)score)->CartsAssaulted);      // gem carts assaulted
                // scores << uint32(((BattlegroundSMScore*)score)->CartsDefended);       // gem carts defended
                // scores << uint32(((BattlegroundSMScore*)score)->CartCaptures);        // gem cart captures
                break;
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_DS:
            case BATTLEGROUND_RV:
            case BATTLEGROUND_TA:
            case BATTLEGROUND_TTP:
                data->WriteBits(0, 22);
                break;
            default:
                data->WriteBits(0, 22);
                break;
        }

        data->WriteBit(playerGUID[2]);
        data->WriteBit(playerGUID[3]);
        data->WriteBit(0);                              // hasPreMatchMMR  - After 4.2 update is always zero (number).
        data->WriteBit(hasRatedBGRating);               // hasPersonalBGRating
        data->WriteBit(playerGUID[6]);

        buff.WriteByteSeq(playerGUID[5]);
        buff << uint32(score->HealingDone);             // healing done

        if (bg->isBattleground())
        {
            buff << uint32(score->HonorableKills);
            buff << uint32(score->BonusHonor / 100);
            buff << uint32(score->Deaths);
        }

        buff.WriteByteSeq(playerGUID[4]);
        buff.WriteByteSeq(playerGUID[0]);

        // if (hasPreMatchMMR) << uint32(preMatchMMR)

        buff.WriteByteSeq(playerGUID[2]);
        buff << uint32(score->DamageDone);              // damage done
        buff.WriteByteSeq(playerGUID[6]);
        buff.WriteByteSeq(playerGUID[7]);
        buff << int32(primaryTree);                     // primary talent tree
        buff.WriteByteSeq(playerGUID[3]);
        buff.WriteByteSeq(playerGUID[1]);

        if (hasRatedBGRating) 
            buff << uint32(RatedBGRating);              // rated bg rating

        // if (hasMMRChange) << uint32(mmrChange) - 0 since 4.2.

        buff.append(scores);

        if (hasBGRatingChange)
            buff << int32(RatedBGRatingChange);         // rated bg rating change

        buff << uint32(score->KillingBlows);            // killing blows
    }

    data->WriteBit(bgEnded);                            // If the Bg ended

    data->FlushBits();
    data->append(buff);

    *data << uint8(bg->GetPlayersCountByTeam(ALLIANCE));
    *data << uint8(bg->GetPlayersCountByTeam(HORDE));

    if (isArena)
    {
        if (bg->GetWinner() < WINNER_NONE) // Must have a winner.
        {
            // Alliance team is Gold, Horde team is Green.
            Group* WinnerArenaTeam = bg->GetBgRaid(bg->GetWinner());
            Group* LoserArenaTeam  = bg->GetBgRaid(bg->GetOtherTeam(bg->GetWinner()));

            Player* winnerLeader = ObjectAccessor::FindPlayer(WinnerArenaTeam->GetLeaderGUID());
            Player* loserLeader = ObjectAccessor::FindPlayer(LoserArenaTeam->GetLeaderGUID());

            data->WriteByteSeq(Guid2[4]);
            data->WriteByteSeq(Guid2[5]);
            data->WriteByteSeq(Guid1[4]);
            data->WriteByteSeq(Guid1[2]);
            data->WriteByteSeq(Guid2[3]);
            data->WriteByteSeq(Guid1[7]);
            data->WriteString(winnerLeader->GetName());
            data->WriteByteSeq(Guid2[2]);
            data->WriteByteSeq(Guid1[3]);
            data->WriteByteSeq(Guid2[6]);
            data->WriteByteSeq(Guid1[6]);
            data->WriteByteSeq(Guid2[0]);
            data->WriteByteSeq(Guid1[5]);
            data->WriteByteSeq(Guid1[1]);
            data->WriteByteSeq(Guid2[7]);
            data->WriteByteSeq(Guid2[1]);
            data->WriteByteSeq(Guid1[0]);
            data->WriteString(loserLeader->GetName());
        }
    }

    if (bgEnded)
        *data << uint8(bg->GetWinner()); // Who wins.

    if (isRated)                                             // arena
    {
        // it seems this must be according to BG_WINNER_A/H and _NOT_ BG_TEAM_A/H
        if (isArena)
        {
            if (bg->GetWinner() < WINNER_NONE) // Must have a winner.
            {
                // Alliance team is Gold, Horde team is Green.
                uint8 slot = Arena::GetSlotByType(bg->GetArenaType());

                Group* WinnerArenaTeam = bg->GetBgRaid(bg->GetWinner());
                Group* LoserArenaTeam  = bg->GetBgRaid(bg->GetOtherTeam(bg->GetWinner()));

                uint32 winnerMatchmakerRating = bg->GetArenaMatchmakerRating(bg->GetWinner(), slot);
                uint32 loserMatchmakerRating = bg->GetArenaMatchmakerRating(bg->GetOtherTeam(bg->GetWinner()), slot);
                uint32 winnerTeamRating = WinnerArenaTeam->GetRating(slot);
                uint32 loserTeamRating = LoserArenaTeam->GetRating(slot);
                uint32 WinnerRatingChange = 0;
                uint32 LoserRatingChange = 0;

                // if (player->GetBattleground())
                //     for (Battleground::BattlegroundScoreMap::const_iterator itr2 = player->GetBattleground()->GetPlayerScoresBegin(); itr2 != player->GetBattleground()->GetPlayerScoresEnd(); ++itr2)
                //         if (itr2->first == itr->guid)
                //             itr2->second->RatingChange = rating_change;

                *data << uint32(winnerTeamRating - WinnerRatingChange);  // Team1 Old Rating
                *data << uint32(loserMatchmakerRating);                  // Team2 MMR
                *data << uint32(loserTeamRating - LoserRatingChange);    // Team2 Old Rating
                *data << uint32(winnerMatchmakerRating);                 // Team1 MMR
                *data << uint32(winnerTeamRating);                       // Team1 NewRating
                *data << uint32(loserTeamRating);                        // Team2 NewRating
            }
        }
        else // Rated BG shit here, same as for arena but need to get the exact order and the unk values meaning of this sending to complete it.
        {
            *data << uint32(0) << uint32(0) << uint32(0) << uint32(0) << uint32(0) << uint32(0);   // Unk. Maybe related to hasPreMatchMMR / hasMMRChange deprecated in 4.2?
        }
    }
}

void BattlegroundMgr::BuildWargamesStatusPacket(WorldPacket* data, Battleground* bg, Player* player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype)
{
    ObjectGuid playerGuid = player->GetGUID();
    ObjectGuid bgGuid = bg ? bg->GetGUID() : 0;

    data->Initialize(SMSG_BATTLEFIELD_STATUS_WARGAMES);

    data->WriteBit(playerGuid[4]);
    data->WriteBit(playerGuid[3]);
    data->WriteBit(bgGuid[5]);
    data->WriteBit(bgGuid[0]);
    data->WriteBit(bgGuid[1]);
    data->WriteBit(bg->isRated());
    data->WriteBit(bgGuid[7]);
    data->WriteBit(playerGuid[1]);
    data->WriteBit(playerGuid[0]);
    data->WriteBit(playerGuid[5]);
    data->WriteBit(bgGuid[3]);
    data->WriteBit(bgGuid[6]);
    data->WriteBit(playerGuid[6]);
    data->WriteBit(playerGuid[7]);
    data->WriteBit(bgGuid[2]);
    data->WriteBit(bgGuid[4]);
    data->WriteBit(playerGuid[2]);

    data->FlushBits();

    *data << uint32(0);                         //not known
    *data << uint32(0);                         //not known
    data->WriteByteSeq(playerGuid[0]);
    data->WriteByteSeq(playerGuid[7]);
    data->WriteByteSeq(bgGuid[6]);
    data->WriteByteSeq(playerGuid[4]);
    //if (bg->isRated())
    //    *data << uint8(count for players in queue Rated Mode);
    //else
        *data << uint8(0);                      // Team Size
    *data << uint8(0);                          // byte4C
    data->WriteByteSeq(playerGuid[2]);
    data->WriteByteSeq(playerGuid[3]);
    data->WriteByteSeq(bgGuid[4]);
    data->WriteByteSeq(bgGuid[3]);
    data->WriteByteSeq(bgGuid[0]);
    data->WriteByteSeq(bgGuid[1]);
    data->WriteByteSeq(bgGuid[5]);
    data->WriteByteSeq(bgGuid[7]);
    *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
    *data << uint8(0);                          // byte4F
    data->WriteByteSeq(bgGuid[2]);
    data->WriteByteSeq(playerGuid[1]);
    data->WriteByteSeq(playerGuid[5]);
    *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
    *data << uint32(Time1);                     // Join Time
    *data << uint8(0);                          // byte4E
    *data << uint8(0);                          // byte4D
    *data << uint8(bg->GetMinLevel());          // Min Level
    *data << uint8(bg->GetMaxLevel());          // Max Level
    data->WriteByteSeq(playerGuid[6]);
    *data << uint32(QueueSlot);                 // Queue slot
}

void BattlegroundMgr::BuildStatusFailedPacket(WorldPacket* data, Battleground* bg, Player* player, uint8 QueueSlot, GroupJoinBattlegroundResult result)
{
    ObjectGuid PlayerReasonGuid = player ? player->GetGUID() : 0; // player who caused the error
    ObjectGuid BattlegroundGuid = bg ? bg->GetGUID() : 0;
    ObjectGuid PlayerGuid = 0;

    data->Initialize(SMSG_BATTLEFIELD_STATUS_FAILED);

    data->WriteBit(PlayerReasonGuid[5]);
    data->WriteBit(PlayerGuid[1]);
    data->WriteBit(PlayerGuid[6]);
    data->WriteBit(PlayerGuid[3]);
    data->WriteBit(BattlegroundGuid[4]);
    data->WriteBit(PlayerReasonGuid[4]);
    data->WriteBit(PlayerReasonGuid[0]);
    data->WriteBit(BattlegroundGuid[2]);
    data->WriteBit(PlayerGuid[4]);
    data->WriteBit(PlayerReasonGuid[1]);
    data->WriteBit(PlayerReasonGuid[6]);
    data->WriteBit(BattlegroundGuid[7]);
    data->WriteBit(PlayerGuid[2]);
    data->WriteBit(PlayerReasonGuid[3]);
    data->WriteBit(BattlegroundGuid[0]);
    data->WriteBit(PlayerGuid[5]);
    data->WriteBit(PlayerGuid[0]);
    data->WriteBit(BattlegroundGuid[3]);
    data->WriteBit(BattlegroundGuid[5]);
    data->WriteBit(PlayerGuid[7]);
    data->WriteBit(PlayerReasonGuid[2]);
    data->WriteBit(PlayerReasonGuid[7]);
    data->WriteBit(BattlegroundGuid[6]);
    data->WriteBit(BattlegroundGuid[1]);

    data->FlushBits();

    *data << uint32(player->GetBattlegroundQueueJoinTime(bg->GetTypeID())); // Join Time

    data->WriteByteSeq(PlayerReasonGuid[2]);
    data->WriteByteSeq(PlayerGuid[4]);
    data->WriteByteSeq(PlayerGuid[3]);
    data->WriteByteSeq(PlayerGuid[7]);
    data->WriteByteSeq(BattlegroundGuid[3]);
    data->WriteByteSeq(PlayerGuid[2]);
    data->WriteByteSeq(BattlegroundGuid[7]);

    *data << uint32(bg->isArena() ? bg->GetArenaType() : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)

    data->WriteByteSeq(PlayerReasonGuid[0]);
    data->WriteByteSeq(BattlegroundGuid[2]);
    data->WriteByteSeq(PlayerReasonGuid[4]);
    data->WriteByteSeq(PlayerReasonGuid[5]);

    *data << uint32(QueueSlot);                     // Queue slot

    data->WriteByteSeq(PlayerGuid[1]);
    data->WriteByteSeq(BattlegroundGuid[1]);
    data->WriteByteSeq(PlayerReasonGuid[7]);

    *data << uint32(result);                        // Result

    data->WriteByteSeq(PlayerReasonGuid[6]);
    data->WriteByteSeq(BattlegroundGuid[0]);
    data->WriteByteSeq(BattlegroundGuid[6]);
    data->WriteByteSeq(BattlegroundGuid[5]);
    data->WriteByteSeq(PlayerGuid[6]);
    data->WriteByteSeq(BattlegroundGuid[4]);
    data->WriteByteSeq(PlayerGuid[0]);
    data->WriteByteSeq(PlayerReasonGuid[3]);
    data->WriteByteSeq(PlayerGuid[5]);
    data->WriteByteSeq(PlayerReasonGuid[1]);
}

void BattlegroundMgr::BuildUpdateWorldStatePacket(WorldPacket* data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 4 + 4 + 1);
    data->WriteBit(0);                   //unk bit
    data->FlushBits();
    *data << uint32(value);
    *data << uint32(field);
}

void BattlegroundMgr::BuildPlayerLeftBattlegroundPacket(WorldPacket* data, uint64 guid)
{
    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8);

    data->WriteBit(guidBytes[7]);
    data->WriteBit(guidBytes[6]);
    data->WriteBit(guidBytes[2]);
    data->WriteBit(guidBytes[4]);
    data->WriteBit(guidBytes[5]);
    data->WriteBit(guidBytes[1]);
    data->WriteBit(guidBytes[3]);
    data->WriteBit(guidBytes[0]);

    data->WriteByteSeq(guidBytes[4]);
    data->WriteByteSeq(guidBytes[2]);
    data->WriteByteSeq(guidBytes[5]);
    data->WriteByteSeq(guidBytes[7]);
    data->WriteByteSeq(guidBytes[0]);
    data->WriteByteSeq(guidBytes[6]);
    data->WriteByteSeq(guidBytes[1]);
    data->WriteByteSeq(guidBytes[3]);
}

void BattlegroundMgr::BuildPlayerJoinedBattlegroundPacket(WorldPacket* data, uint64 guid)
{
    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);

    data->WriteBit(guidBytes[0]);
    data->WriteBit(guidBytes[4]);
    data->WriteBit(guidBytes[3]);
    data->WriteBit(guidBytes[5]);
    data->WriteBit(guidBytes[7]);
    data->WriteBit(guidBytes[6]);
    data->WriteBit(guidBytes[2]);
    data->WriteBit(guidBytes[1]);

    data->WriteByteSeq(guidBytes[1]);
    data->WriteByteSeq(guidBytes[5]);
    data->WriteByteSeq(guidBytes[3]);
    data->WriteByteSeq(guidBytes[2]);
    data->WriteByteSeq(guidBytes[0]);
    data->WriteByteSeq(guidBytes[7]);
    data->WriteByteSeq(guidBytes[4]);
    data->WriteByteSeq(guidBytes[6]);
}

Battleground* BattlegroundMgr::GetBattlegroundThroughClientInstance(uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    //cause at HandleBattlegroundJoinOpcode the clients sends the instanceid he gets from
    //SMSG_BATTLEFIELD_LIST we need to find the battleground with this clientinstance-id
    Battleground* bg = GetBattlegroundTemplate(bgTypeId);
    if (!bg)
        return NULL;

    if (bg->isArena())
        return GetBattleground(instanceId, bgTypeId);

    for (BattlegroundSet::iterator itr = m_Battlegrounds[bgTypeId].begin(); itr != m_Battlegrounds[bgTypeId].end(); ++itr)
    {
        if (itr->second->GetClientInstanceID() == instanceId)
            return itr->second;
    }

    return NULL;
}

Battleground* BattlegroundMgr::GetBattleground(uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    if (!instanceId)
        return NULL;

    // search if needed
    BattlegroundSet::iterator itr;
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        for (uint32 i = BATTLEGROUND_AV; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        {
            itr = m_Battlegrounds[i].find(instanceId);
            if (itr != m_Battlegrounds[i].end())
                return itr->second;
        }
        return NULL;
    }

    itr = m_Battlegrounds[bgTypeId].find(instanceId);

    return ((itr != m_Battlegrounds[bgTypeId].end()) ? itr->second : NULL);
}

Battleground* BattlegroundMgr::GetBattlegroundTemplate(BattlegroundTypeId bgTypeId)
{
    //map is sorted and we can be sure that lowest instance id has only BG template
    return m_Battlegrounds[bgTypeId].empty() ? NULL : m_Battlegrounds[bgTypeId].begin()->second;
}

uint32 BattlegroundMgr::CreateClientVisibleInstanceId(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    if (IsArenaType(bgTypeId))
        return 0;                                           // Arenas don't have client-instanceids

    // We create here an instanceid, which is just for displaying this to the client and without any other use.
    // the client-instanceIds are unique for each battleground-type. the instance-id just needs to be as low as possible, beginning with 1.
    // the following works, because std::set is default ordered with "<". the optimalization would be to use as bitmask std::vector<uint32> - but that would only make code unreadable.
    uint32 lastId = 0;
    for (std::set<uint32>::iterator itr = m_ClientBattlegroundIds[bgTypeId][bracket_id].begin(); itr != m_ClientBattlegroundIds[bgTypeId][bracket_id].end();)
    {
        if ((++lastId) != *itr)                             //if there is a gap between the ids, we will break..
            break;
        lastId = *itr;
    }
    m_ClientBattlegroundIds[bgTypeId][bracket_id].insert(lastId + 1);
    return lastId + 1;
}

// create a new battleground that will really be used to play
Battleground* BattlegroundMgr::CreateNewBattleground(BattlegroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, uint8 arenaType, bool isRated)
{
    // Get the template BG
    Battleground* bg_template = GetBattlegroundTemplate(bgTypeId);

    if (!bg_template)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground: CreateNewBattleground - bg template not found for %u", bgTypeId);
        return NULL;
    }

    bool isRandom = false;
    bool isRatedBG = false;

    switch (bgTypeId)
    {
        case BATTLEGROUND_RB:
            isRandom = true;
            bgTypeId = GetRandomBG(bgTypeId);
            bg_template = GetBattlegroundTemplate(bgTypeId);
            break;
        case BATTLEGROUND_AA: // "All Arenas" are handled by random selection too.
            bgTypeId = GetRandomBG(bgTypeId);
            bg_template = GetBattlegroundTemplate(bgTypeId);
            break;

        case BATTLEGROUND_RATED_10_VS_10:
            if (!sWorld->FindWorldState(WS_WEEKLY_SELECT_RATED_BG)) // If we have no setting for selected rated bg in the db, select one.
            {
                switch (urand(0, 7))
                {
                    case 0: bgTypeId = BATTLEGROUND_WS;  break;
                    case 1: bgTypeId = BATTLEGROUND_AB;  break;
                    case 2: bgTypeId = BATTLEGROUND_EY;  break;
                    case 3: bgTypeId = BATTLEGROUND_SA;  break;
                    case 4: bgTypeId = BATTLEGROUND_BFG; break;
                    case 5: bgTypeId = BATTLEGROUND_TP;  break;
                    case 6: bgTypeId = BATTLEGROUND_TOK; break;
                    case 7: bgTypeId = BATTLEGROUND_SM;  break;
                }

                bg_template = GetBattlegroundTemplate(bgTypeId);
                sWorld->setWorldState(WS_WEEKLY_SELECT_RATED_BG, bgTypeId); // Set it.
                isRatedBG = true;
            }
            else
            {
                bgTypeId = BattlegroundTypeId(sWorld->getWorldState(WS_WEEKLY_SELECT_RATED_BG));
                bg_template = GetBattlegroundTemplate(bgTypeId);
                isRatedBG = true;
            }
            break;

        default: break;
    }

    Battleground* bg = NULL;
    // create a copy of the BG template
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:
            bg = new BattlegroundAV(*(BattlegroundAV*)bg_template);
            break;
        case BATTLEGROUND_WS:
            bg = new BattlegroundWS(*(BattlegroundWS*)bg_template);
            break;
        case BATTLEGROUND_AB:
            bg = new BattlegroundAB(*(BattlegroundAB*)bg_template);
            break;
        case BATTLEGROUND_NA:
            bg = new BattlegroundNA(*(BattlegroundNA*)bg_template);
            break;
        case BATTLEGROUND_BE:
            bg = new BattlegroundBE(*(BattlegroundBE*)bg_template);
            break;
        case BATTLEGROUND_AA:
            bg = new BattlegroundAA(*(BattlegroundAA*)bg_template);
            break;
        case BATTLEGROUND_EY:
            bg = new BattlegroundEY(*(BattlegroundEY*)bg_template);
            break;
        case BATTLEGROUND_RL:
            bg = new BattlegroundRL(*(BattlegroundRL*)bg_template);
            break;
        case BATTLEGROUND_SA:
            bg = new BattlegroundSA(*(BattlegroundSA*)bg_template);
            break;
        case BATTLEGROUND_DS:
            bg = new BattlegroundDS(*(BattlegroundDS*)bg_template);
            break;
        case BATTLEGROUND_RV:
            bg = new BattlegroundRV(*(BattlegroundRV*)bg_template);
            break;
        case BATTLEGROUND_IC:
            bg = new BattlegroundIC(*(BattlegroundIC*)bg_template);
            break;
        case BATTLEGROUND_TP:
            bg = new BattlegroundTP(*(BattlegroundTP*)bg_template);
            break;
        case BATTLEGROUND_BFG:
            bg = new BattlegroundBFG(*(BattlegroundBFG*)bg_template);
            break;
        case BATTLEGROUND_TOK:
            bg = new BattlegroundTK(*(BattlegroundTK*)bg_template);
            break;
        case BATTLEGROUND_DG:
            bg = new BattlegroundDG(*(BattlegroundDG*)bg_template);
            break;
        case BATTLEGROUND_SM:
            bg = new BattlegroundSM(*(BattlegroundSM*)bg_template);
            break;
        case BATTLEGROUND_RB:
            bg = new BattlegroundRB(*(BattlegroundRB*)bg_template);
            break;
        case BATTLEGROUND_RATED_10_VS_10:
            bg = new BattlegroundRatedBG(*(BattlegroundRatedBG*)bg_template);
            break;
        case BATTLEGROUND_TA:
            bg = new BattlegroundTA(*(BattlegroundTA*)bg_template);
            break;
        case BATTLEGROUND_TTP:
            bg = new BattlegroundTTP(*(BattlegroundTTP*)bg_template);
            break;

        default:
            return NULL;
    }

    // Set battleground difficulty before initialization
    bg->SetBracket(bracketEntry);

    // Generate a new instance id
    bg->SetInstanceID(sMapMgr->GenerateInstanceId());

    if (isRatedBG)
        bg->SetClientInstanceID(CreateClientVisibleInstanceId(BATTLEGROUND_RATED_10_VS_10, bracketEntry->GetBracketId()));
    else
        bg->SetClientInstanceID(CreateClientVisibleInstanceId(isRandom ? BATTLEGROUND_RB : bgTypeId, bracketEntry->GetBracketId()));

    // Reset the new bg (set status to status_wait_queue from status_none)
    bg->Reset();

    // Start the joining of the bg
    bg->SetStatus(STATUS_WAIT_JOIN);
    bg->SetArenaType(arenaType);

    if (isRatedBG)
    {
        bg->SetArenaorBGType(false);
        bg->SetRated(true);
    }
    else bg->SetRated(isRated);

    bg->SetRandom(isRandom);

    if (isRatedBG)
        bg->SetTypeID(BATTLEGROUND_RATED_10_VS_10);
    else
        bg->SetTypeID(isRandom ? BATTLEGROUND_RB : bgTypeId);

    bg->SetRandomTypeID(bgTypeId);

    bg->SetGuid(MAKE_NEW_GUID(bgTypeId, 0, HIGHGUID_BATTLEGROUND));

    // Set up correct min/max player counts for scoreboards
    if (bg->isArena())
    {
        uint32 maxPlayersPerTeam = 0;
        switch (arenaType)
        {
            case ARENA_TYPE_2v2:
                maxPlayersPerTeam = 2;
                break;
            case ARENA_TYPE_3v3:
                maxPlayersPerTeam = 3;
                break;
            case ARENA_TYPE_5v5:
                maxPlayersPerTeam = 5;
                break;
        }

        bg->SetMaxPlayersPerTeam(maxPlayersPerTeam);
        bg->SetMaxPlayers(maxPlayersPerTeam * 2);
    }

    return bg;
}

// used to create the BG templates
bool BattlegroundMgr::CreateBattleground(CreateBattlegroundData& data)
{
    // Create the BG
    Battleground* bg = NULL;
    switch (data.bgTypeId)
    {
        case BATTLEGROUND_AV:             bg = new BattlegroundAV;      break;
        case BATTLEGROUND_WS:             bg = new BattlegroundWS;      break;
        case BATTLEGROUND_AB:             bg = new BattlegroundAB;      break;
        case BATTLEGROUND_NA:             bg = new BattlegroundNA;      break;
        case BATTLEGROUND_BE:             bg = new BattlegroundBE;      break;
        case BATTLEGROUND_AA:             bg = new BattlegroundAA;      break;
        case BATTLEGROUND_EY:             bg = new BattlegroundEY;      break;
        case BATTLEGROUND_RL:             bg = new BattlegroundRL;      break;
        case BATTLEGROUND_SA:             bg = new BattlegroundSA;      break;
        case BATTLEGROUND_DS:             bg = new BattlegroundDS;      break;
        case BATTLEGROUND_RV:             bg = new BattlegroundRV;      break;
        case BATTLEGROUND_IC:             bg = new BattlegroundIC;      break;
        case BATTLEGROUND_TP:             bg = new BattlegroundTP;      break;
        case BATTLEGROUND_BFG:            bg = new BattlegroundBFG;     break;
        case BATTLEGROUND_TOK:            bg = new BattlegroundTK;      break;
        case BATTLEGROUND_DG:             bg = new BattlegroundDG;      break;
        case BATTLEGROUND_SM:             bg = new BattlegroundSM;      break;
        case BATTLEGROUND_RB:             bg = new BattlegroundRB;      break;
        case BATTLEGROUND_RATED_10_VS_10: bg = new BattlegroundRatedBG; break;
        case BATTLEGROUND_TA:             bg = new BattlegroundTA;      break;
        case BATTLEGROUND_TTP:            bg = new BattlegroundTTP;     break;

        default:
            bg = new Battleground;
            break;
    }

    bg->SetGuid(MAKE_NEW_GUID(data.bgTypeId, 0, HIGHGUID_BATTLEGROUND));
    bg->SetMapId(data.MapID);
    bg->SetTypeID(data.bgTypeId);
    bg->SetInstanceID(0);
    bg->SetArenaorBGType(data.IsArena);
    bg->SetMinPlayersPerTeam(data.MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(data.MaxPlayersPerTeam);
    bg->SetMinPlayers(data.MinPlayersPerTeam * 2);
    bg->SetMaxPlayers(data.MaxPlayersPerTeam * 2);
    bg->SetName(data.BattlegroundName);
    bg->SetTeamStartLoc(ALLIANCE, data.Team1StartLocX, data.Team1StartLocY, data.Team1StartLocZ, data.Team1StartLocO);
    bg->SetTeamStartLoc(HORDE,    data.Team2StartLocX, data.Team2StartLocY, data.Team2StartLocZ, data.Team2StartLocO);
    bg->SetStartMaxDist(data.StartMaxDist);
    bg->SetLevelRange(data.LevelMin, data.LevelMax);
    bg->SetScriptId(data.scriptId);

    // add bg to update list
    AddBattleground(bg->GetInstanceID(), bg->GetTypeID(), bg);

    return true;
}

void BattlegroundMgr::CreateInitialBattlegrounds()
{
    uint32 oldMSTime = getMSTime();
    //                                               0   1                  2                  3       4       5                 6               7              8            9             10      11
    QueryResult result = WorldDatabase.Query("SELECT id, MinPlayersPerTeam, MaxPlayersPerTeam, MinLvl, MaxLvl, AllianceStartLoc, AllianceStartO, HordeStartLoc, HordeStartO, StartMaxDist, Weight, ScriptName FROM battleground_template");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 bgTypeId = fields[0].GetUInt32();
        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId, NULL))
            continue;

        // can be overwrite by values from DB
        BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
        if (!bl)
        {
            TC_LOG_ERROR("bg.battleground", "Battleground ID %u not found in BattleMasterList.dbc. Battleground not created.", bgTypeId);
            continue;
        }

        CreateBattlegroundData data;
        data.bgTypeId = BattlegroundTypeId(bgTypeId);
        data.IsArena = (bl->type == TYPE_ARENA);
        data.MinPlayersPerTeam = fields[1].GetUInt16();
        data.MaxPlayersPerTeam = fields[2].GetUInt16();
        data.LevelMin = fields[3].GetUInt8();
        data.LevelMax = fields[4].GetUInt8();
        float dist = fields[9].GetFloat();
        data.StartMaxDist = dist * dist;

        data.scriptId = sObjectMgr->GetScriptId(fields[11].GetCString());
        data.BattlegroundName = bl->name;
        data.MapID = bl->mapid[0];

        if (data.MaxPlayersPerTeam == 0 || data.MinPlayersPerTeam > data.MaxPlayersPerTeam)
        {
            TC_LOG_ERROR("sql.sql", "Table `battleground_template` for id %u has bad values for MinPlayersPerTeam (%u) and MaxPlayersPerTeam(%u)",
                data.bgTypeId, data.MinPlayersPerTeam, data.MaxPlayersPerTeam);
            continue;
        }

        if (data.LevelMin == 0 || data.LevelMax == 0 || data.LevelMin > data.LevelMax)
        {
            TC_LOG_ERROR("sql.sql", "Table `battleground_template` for id %u has bad values for LevelMin (%u) and LevelMax(%u)",
                data.bgTypeId, data.LevelMin, data.LevelMax);
            continue;
        }

        if (data.bgTypeId == BATTLEGROUND_AA || data.bgTypeId == BATTLEGROUND_RB || data.bgTypeId == BATTLEGROUND_RATED_10_VS_10)
        {
            data.Team1StartLocX = 0;
            data.Team1StartLocY = 0;
            data.Team1StartLocZ = 0;
            data.Team1StartLocO = fields[6].GetFloat();
            data.Team2StartLocX = 0;
            data.Team2StartLocY = 0;
            data.Team2StartLocZ = 0;
            data.Team2StartLocO = fields[8].GetFloat();
        }
        else
        {
            uint32 startId = fields[5].GetUInt32();
            if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
            {
                data.Team1StartLocX = start->x;
                data.Team1StartLocY = start->y;
                data.Team1StartLocZ = start->z;
                data.Team1StartLocO = fields[6].GetFloat();
            }
            else
            {
                TC_LOG_ERROR("sql.sql", "Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", data.bgTypeId, startId);
                continue;
            }

            startId = fields[7].GetUInt32();
            if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
            {
                data.Team2StartLocX = start->x;
                data.Team2StartLocY = start->y;
                data.Team2StartLocZ = start->z;
                data.Team2StartLocO = fields[8].GetFloat();
            }
            else
            {
                TC_LOG_ERROR("sql.sql", "Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", data.bgTypeId, startId);
                continue;
            }
        }

        if (!CreateBattleground(data))
            continue;

        if (data.IsArena)
        {
            if (data.bgTypeId != BATTLEGROUND_AA)
                m_ArenaSelectionWeights[data.bgTypeId] = fields[10].GetUInt8();
        }
        else if (data.bgTypeId != BATTLEGROUND_RB && data.bgTypeId != BATTLEGROUND_RATED_10_VS_10)
            m_BGSelectionWeights[data.bgTypeId] = fields[10].GetUInt8();

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u battlegrounds in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void BattlegroundMgr::BuildBattlegroundListPacket(WorldPacket* data, uint64 guid, Player* player, BattlegroundTypeId bgTypeId)
{
    if (!player)
        return;

    uint32 winnerConquest = (!player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_CONQUEST_FIRST) : sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_CONQUEST_LAST)) / CURRENCY_PRECISION;
    uint32 winnerHonor = (!player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_HONOR_FIRST) : sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_HONOR_LAST)) / CURRENCY_PRECISION;
    uint32 loserHonor = (!player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_LOSER_HONOR_FIRST) : sWorld->getIntConfig(CONFIG_BG_REWARD_LOSER_HONOR_LAST)) / CURRENCY_PRECISION;

    bool HasWonRandomBg = player->GetRandomWinner() ? true : false;
    bool HasWonCallToArms = false;

    // ToDo: Check hasWonCallToArmsBG / hasWonRandomBG fields.
    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEFIELD_LIST);

    data->WriteBit(1);                         // Unk TRUE.

    data->WriteBit(guidBytes[2]);
    data->WriteBit(guidBytes[3]);
    data->WriteBit(guidBytes[0]);

    data->WriteBit(HasWonCallToArms);          // hasWonCallToArmsBG - daily

    data->WriteBit(guidBytes[4]);
    data->WriteBit(guidBytes[7]);
    data->WriteBit(guidBytes[5]);

    data->WriteBit(1);                         // Signals EVENT_PVPQUEUE_ANYWHERE_SHOW if set. TRUE.

    data->WriteBit(guidBytes[6]);
    data->WriteBit(guidBytes[1]);

    data->FlushBits();
    size_t count_pos = data->bitwpos();
    data->WriteBits(0, 22);                    // Placeholder for BG count.

    data->WriteBit(HasWonRandomBg);            // hasWonRandomBG - daily.

    data->FlushBits();

    *data << uint32(winnerConquest);           // Winner Conquest Reward or Random Winner Conquest Reward.

    if (Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId))
    {
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), player->getLevel()))         // expected bracket entry
            *data << uint8(bracketEntry->maxLevel);    // Max level.
        else
            *data << uint8(0);    // Max level.
    }

    data->WriteByteSeq(guidBytes[1]);
    data->WriteByteSeq(guidBytes[4]);

    uint32 count = 0;

	if (Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId))
    {
        // expected bracket entry
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), player->getLevel()))
        {
            BattlegroundBracketId bracketId = bracketEntry->GetBracketId();
            for (std::set<uint32>::iterator itr = m_ClientBattlegroundIds[bgTypeId][bracketId].begin(); itr != m_ClientBattlegroundIds[bgTypeId][bracketId].end();++itr)
            {
                *data << uint32(*itr);                            // instance id
                ++count;
            }
        }
    }

    data->PutBits(count_pos, count, 22);       // Bg instance count.

    if (Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId))
    {
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), player->getLevel()))         // expected bracket entry
            *data << uint8(bracketEntry->minLevel);    // Min level.
        else
            *data << uint8(0);    // Min level.
    }

    *data << uint32(loserHonor);               // Loser Honor Reward or Random Loser Honor Reward

    data->WriteByteSeq(guidBytes[7]);
    data->WriteByteSeq(guidBytes[0]);

    *data << uint32(bgTypeId);                 // Battleground Type Id.

    data->WriteByteSeq(guidBytes[6]);
    data->WriteByteSeq(guidBytes[5]);
    data->WriteByteSeq(guidBytes[3]);

    *data << uint32(winnerHonor);              // Winner Honor Reward or Random Winner Honor Reward.
    *data << uint32(winnerHonor);              // Winner Honor Reward or Random Winner Honor Reward.

    data->WriteByteSeq(guidBytes[2]);

    *data << uint32(winnerConquest);           // Winner Conquest Reward or Random Winner Conquest Reward.
    *data << uint32(loserHonor);               // Loser Honor Reward or Random Loser Honor Reward.
}

void BattlegroundMgr::SendToBattleground(Player* player, uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    if (Battleground* bg = GetBattleground(instanceId, bgTypeId))
    {
        float x, y, z, O;
        uint32 mapid = bg->GetMapId();
        uint32 team = player->GetBGTeam();
        if (team == 0)
            team = player->GetTeam();
        bg->GetTeamStartLoc(team, x, y, z, O);

        TC_LOG_DEBUG("bg.battleground", "BattlegroundMgr::SendToBattleground: Sending %s to map %u, X %f, Y %f, Z %f, O %f (bgType %u)", player->GetName().c_str(), mapid, x, y, z, O, bgTypeId);
        player->TeleportTo(mapid, x, y, z, O);
    }
    else
        TC_LOG_ERROR("bg.battleground", "BattlegroundMgr::SendToBattleground: Instance %u (bgType %u) not found while trying to teleport player %s", instanceId, bgTypeId, player->GetName().c_str());
}

void BattlegroundMgr::SendAreaSpiritHealerQueryOpcode(Player* player, Battleground* bg, ObjectGuid guid)
{
    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 13);
    uint32 time_ = 30000 - bg->GetLastResurrectTime();      // resurrect every 30 seconds
    if (time_ == uint32(-1))
        time_ = 0;

    data.WriteBit(guid[5]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[3]);

    data.FlushBits();

    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[7]);

    data << time_;

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);

    player->GetSession()->SendPacket(&data);
}

bool BattlegroundMgr::IsArenaType(BattlegroundTypeId bgTypeId)
{
    return (bgTypeId == BATTLEGROUND_AA
            || bgTypeId == BATTLEGROUND_BE
            || bgTypeId == BATTLEGROUND_NA
            || bgTypeId == BATTLEGROUND_DS
            || bgTypeId == BATTLEGROUND_RV
            || bgTypeId == BATTLEGROUND_RL
            || bgTypeId == BATTLEGROUND_TA
            || bgTypeId == BATTLEGROUND_TTP);
}

BattlegroundQueueTypeId BattlegroundMgr::BGQueueTypeId(BattlegroundTypeId bgTypeId, uint8 arenaType)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_WS:
            return BATTLEGROUND_QUEUE_WS;
        case BATTLEGROUND_AB:
            return BATTLEGROUND_QUEUE_AB;
        case BATTLEGROUND_AV:
            return BATTLEGROUND_QUEUE_AV;
        case BATTLEGROUND_EY:
            return BATTLEGROUND_QUEUE_EY;
        case BATTLEGROUND_SA:
            return BATTLEGROUND_QUEUE_SA;
        case BATTLEGROUND_IC:
            return BATTLEGROUND_QUEUE_IC;
        case BATTLEGROUND_TP:
            return BATTLEGROUND_QUEUE_TP;
        case BATTLEGROUND_BFG:
            return BATTLEGROUND_QUEUE_BFG;
        case BATTLEGROUND_TOK:
            return BATTLEGROUND_QUEUE_KT;
        case BATTLEGROUND_SM:
            return BATTLEGROUND_QUEUE_SSM;
        case BATTLEGROUND_DG:
            return BATTLEGROUND_QUEUE_DG;
        case BATTLEGROUND_RB:
            return BATTLEGROUND_QUEUE_RB;
        case BATTLEGROUND_AA:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
        case BATTLEGROUND_TA:
        case BATTLEGROUND_TTP:
            switch (arenaType)
            {
                case ARENA_TYPE_2v2:
                    return BATTLEGROUND_QUEUE_2v2;
                case ARENA_TYPE_3v3:
                    return BATTLEGROUND_QUEUE_3v3;
                case ARENA_TYPE_5v5:
                    return BATTLEGROUND_QUEUE_5v5;
                default:
                    return BATTLEGROUND_QUEUE_NONE;
            }
        case BATTLEGROUND_RATED_10_VS_10: // Rated bg's
            return BATTLEGROUND_QUEUE_RATED_10_VS_10;

        default:
            return BATTLEGROUND_QUEUE_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::BGTemplateId(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_WS: // Normal bg
            return BATTLEGROUND_WS;
        case BATTLEGROUND_QUEUE_AB:
            return BATTLEGROUND_AB;
        case BATTLEGROUND_QUEUE_AV:
            return BATTLEGROUND_AV;
        case BATTLEGROUND_QUEUE_EY:
            return BATTLEGROUND_EY;
        case BATTLEGROUND_QUEUE_SA:
            return BATTLEGROUND_SA;
        case BATTLEGROUND_QUEUE_IC:
            return BATTLEGROUND_IC;
        case BATTLEGROUND_QUEUE_TP:
            return BATTLEGROUND_TP;
        case BATTLEGROUND_QUEUE_BFG:
            return BATTLEGROUND_BFG;
        case BATTLEGROUND_QUEUE_KT:
            return BATTLEGROUND_TOK;
        case BATTLEGROUND_QUEUE_SSM:
            return BATTLEGROUND_SM;
        case BATTLEGROUND_QUEUE_DG:
            return BATTLEGROUND_DG;

        case BATTLEGROUND_QUEUE_RB: // Random bg
            return BATTLEGROUND_RB;

        case BATTLEGROUND_QUEUE_RATED_10_VS_10: // Rated bg's
            return BATTLEGROUND_RATED_10_VS_10;

        case BATTLEGROUND_QUEUE_2v2: // Arenas
        case BATTLEGROUND_QUEUE_3v3:
        case BATTLEGROUND_QUEUE_5v5:
            return BATTLEGROUND_AA;

        default:
            return BattlegroundTypeId(0);       // used for unknown templates (it exists and does nothing)
    }
}

uint8 BattlegroundMgr::BGArenaType(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_2v2:
            return ARENA_TYPE_2v2;
        case BATTLEGROUND_QUEUE_3v3:
            return ARENA_TYPE_3v3;
        case BATTLEGROUND_QUEUE_5v5:
            return ARENA_TYPE_5v5;
        default:
            return 0;
    }
}

void BattlegroundMgr::ToggleTesting()
{
    m_Testing = !m_Testing;
    sWorld->SendWorldText(m_Testing ? LANG_DEBUG_BG_ON : LANG_DEBUG_BG_OFF);
}

void BattlegroundMgr::ToggleArenaTesting()
{
    m_ArenaTesting = !m_ArenaTesting;
    sWorld->SendWorldText(m_ArenaTesting ? LANG_DEBUG_ARENA_ON : LANG_DEBUG_ARENA_OFF);
}

void BattlegroundMgr::SetHolidayWeekends(uint32 mask)
{
    for (uint32 bgtype = 1; bgtype < MAX_BATTLEGROUND_TYPE_ID; ++bgtype)
    {
        if (Battleground* bg = GetBattlegroundTemplate(BattlegroundTypeId(bgtype)))
        {
            bg->SetHoliday(mask & (1 << bgtype));
        }
    }
}

void BattlegroundMgr::ScheduleQueueUpdate(uint32 arenaMatchmakerRating, uint8 arenaType, BattlegroundQueueTypeId bgQueueTypeId, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    //This method must be atomic, @todo add mutex. We will use only 1 number created of bgTypeId and bracket_id
    QueueSchedulerItem* schedule_id = new QueueSchedulerItem(arenaMatchmakerRating, arenaType, bgQueueTypeId, bgTypeId, bracket_id);
    bool found = false;

    if (!m_QueueUpdateScheduler.empty())
    {
        for (uint8 i = 0; i < m_QueueUpdateScheduler.size(); i++)
        {
            if (m_QueueUpdateScheduler[i]->_arenaMMRating == arenaMatchmakerRating
             && m_QueueUpdateScheduler[i]->_arenaType == arenaType
             && m_QueueUpdateScheduler[i]->_bgQueueTypeId == bgQueueTypeId
             && m_QueueUpdateScheduler[i]->_bgTypeId == bgTypeId
             && m_QueueUpdateScheduler[i]->_bracket_id == bracket_id)
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
        m_QueueUpdateScheduler.push_back(schedule_id);
}

uint32 BattlegroundMgr::GetMaxRatingDifference() const
{
    // this is for stupid people who can't use brain and set max rating difference to 0
    uint32 diff = sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE);
    if (diff == 0)
        diff = 5000;
    return diff;
}

uint32 BattlegroundMgr::GetRatingDiscardTimer() const
{
    return sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER);
}

uint32 BattlegroundMgr::GetPrematureFinishTime() const
{
    return sWorld->getIntConfig(CONFIG_BATTLEGROUND_PREMATURE_FINISH_TIMER);
}

void BattlegroundMgr::LoadBattleMastersEntry()
{
    uint32 oldMSTime = getMSTime();

    mBattleMastersMap.clear();                                  // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, bg_template FROM battlemaster_entry");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 battlemaster entries. DB table `battlemaster_entry` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 bgTypeId  = fields[1].GetUInt32();
        if (!sBattlemasterListStore.LookupEntry(bgTypeId))
        {
            TC_LOG_ERROR("sql.sql", "Table `battlemaster_entry` contain entry %u for not existed battleground type %u, ignored.", entry, bgTypeId);
            continue;
        }

        mBattleMastersMap[entry] = BattlegroundTypeId(bgTypeId);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u battlemaster entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

HolidayIds BattlegroundMgr::BGTypeToWeekendHolidayId(BattlegroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:  return HOLIDAY_CALL_TO_ARMS_AV;
        case BATTLEGROUND_EY:  return HOLIDAY_CALL_TO_ARMS_EY;
        case BATTLEGROUND_WS:  return HOLIDAY_CALL_TO_ARMS_WS;
        case BATTLEGROUND_SA:  return HOLIDAY_CALL_TO_ARMS_SA;
        case BATTLEGROUND_AB:  return HOLIDAY_CALL_TO_ARMS_AB;
        case BATTLEGROUND_IC:  return HOLIDAY_CALL_TO_ARMS_IC;
        case BATTLEGROUND_TP:  return HOLIDAY_CALL_TO_ARMS_TP;
        case BATTLEGROUND_BFG: return HOLIDAY_CALL_TO_ARMS_BFG;
        case BATTLEGROUND_TOK: return HOLIDAY_CALL_TO_ARMS_TEMPLE_OF_K;
        case BATTLEGROUND_SM:  return HOLIDAY_CALL_TO_ARMS_SILVERSHARD;
        case BATTLEGROUND_DG:  return HOLIDAY_CALL_TO_ARMS_DEEPWIND_G;

        default: return HOLIDAY_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::WeekendHolidayIdToBGType(HolidayIds holiday)
{
    switch (holiday)
    {
        case HOLIDAY_CALL_TO_ARMS_AV:          return BATTLEGROUND_AV;
        case HOLIDAY_CALL_TO_ARMS_EY:          return BATTLEGROUND_EY;
        case HOLIDAY_CALL_TO_ARMS_WS:          return BATTLEGROUND_WS;
        case HOLIDAY_CALL_TO_ARMS_SA:          return BATTLEGROUND_SA;
        case HOLIDAY_CALL_TO_ARMS_AB:          return BATTLEGROUND_AB;
        case HOLIDAY_CALL_TO_ARMS_IC:          return BATTLEGROUND_IC;
        case HOLIDAY_CALL_TO_ARMS_TP:          return BATTLEGROUND_TP;
        case HOLIDAY_CALL_TO_ARMS_BFG:         return BATTLEGROUND_BFG;
        case HOLIDAY_CALL_TO_ARMS_TEMPLE_OF_K: return BATTLEGROUND_TOK;
        case HOLIDAY_CALL_TO_ARMS_SILVERSHARD: return BATTLEGROUND_SM;
        case HOLIDAY_CALL_TO_ARMS_DEEPWIND_G:  return BATTLEGROUND_DG;

        default: return BATTLEGROUND_TYPE_NONE;
    }
}

bool BattlegroundMgr::IsBGWeekend(BattlegroundTypeId bgTypeId)
{
    return IsHolidayActive(BGTypeToWeekendHolidayId(bgTypeId));
}

BattlegroundTypeId BattlegroundMgr::GetRandomBG(BattlegroundTypeId bgTypeId)
{
    uint32 weight = 0;
    BattlegroundSelectionWeightMap selectionWeights;

    if (bgTypeId == BATTLEGROUND_AA)
    {
        for (BattlegroundSelectionWeightMap::const_iterator it = m_ArenaSelectionWeights.begin(); it != m_ArenaSelectionWeights.end(); ++it)
        {
            if (it->second)
            {
                weight += it->second;
                selectionWeights[it->first] = it->second;
            }
        }
    }
    else if (bgTypeId == BATTLEGROUND_RB)
    {
        for (BattlegroundSelectionWeightMap::const_iterator it = m_BGSelectionWeights.begin(); it != m_BGSelectionWeights.end(); ++it)
        {
            if (it->second)
            {
                weight += it->second;
                selectionWeights[it->first] = it->second;
            }
        }
    }

    if (weight)
    {
        // Select a random value
        uint32 selectedWeight = urand(0, weight - 1);

        // Select the correct bg (if we have in DB A(10), B(20), C(10), D(15) --> [0---A---9|10---B---29|30---C---39|40---D---54])
        weight = 0;
        for (BattlegroundSelectionWeightMap::const_iterator it = selectionWeights.begin(); it != selectionWeights.end(); ++it)
        {
            weight += it->second;
            if (selectedWeight < weight)
            {
                return it->first;
                break;
            }
        }
    }

    return BATTLEGROUND_TYPE_NONE;
}

void BattlegroundMgr::AddToBGFreeSlotQueue(BattlegroundTypeId bgTypeId, Battleground* bg)
{
    BGFreeSlotQueue[bgTypeId].push_front(bg);
}

void BattlegroundMgr::RemoveFromBGFreeSlotQueue(BattlegroundTypeId bgTypeId, uint32 instanceId)
{
    for (BGFreeSlotQueueType::iterator itr = sBattlegroundMgr->BGFreeSlotQueue[bgTypeId].begin(); itr != sBattlegroundMgr->BGFreeSlotQueue[bgTypeId].end(); ++itr)
    {
        if ((*itr)->GetInstanceID() == instanceId)
        {
            sBattlegroundMgr->BGFreeSlotQueue[bgTypeId].erase(itr);
            return;
        }
    }
}

/* Wargames need implementation.
void BattlegroundMgr::HandleWargameRequest(WargameRequest request)
{
    const BattlemasterListEntry* battleground = sBattlemasterListStore.LookupEntry(request.battlegroundId);

    if(!battleground)
        return;

    Player* leaderChallenger = ObjectAccessor::FindPlayer(request.playerChallenger);
    Player* leaderChallenged = ObjectAccessor::FindPlayer(request.playerChallenged);

    if(!leaderChallenger || !leaderChallenged)
        return;

    Group* groupChallenger = leaderChallenger->GetGroup();
    Group* groupChallenged = leaderChallenged->GetGroup();

    if(!groupChallenged || !groupChallenger || (groupChallenger->GetMembersCount() < 2 && groupChallenged->GetMembersCount() < 2))
        return;

    m_wargamesRequests.insert(std::pair<uint32,WargameRequest>(m_wargamesCount,request));
    m_wargamesCount++;

    uint64 battlegroundGuid = MAKE_NEW_GUID(request.battlegroundId | 0x30000,0,HIGHGUID_BATTLEGROUND);

    WorldPacket response(SMSG_WARGAME_REQUEST_SENT);
    uint8 byteMask[] = { 6, 0, 3, 5, 2, 1, 4, 7 };
    uint8 byteBytes[] = { 5, 6, 3, 0, 7, 4, 2, 1, };
    response.WriteGuidMask(battlegroundGuid,byteMask,8);
    response.WriteGuidBytes(battlegroundGuid,byteBytes,8,0);

    leaderChallenger->GetSession()->SendPacket(&response);
	
	WorldPacket RequestWargame(SMSG_WARGAME_CHECK_ENTRY);

    union WargamePacketInfoType
    {
      uint64 raw;
      struct rawStructure
      {
        uint32 timeout;
        uint16 gamesCount;
        uint16 battlegroundID;
      } data;
    } info;

    info.data.timeout = time(NULL) + 23; /// We add 3 because there is a buffer, we have to find the real timeout
    info.data.gamesCount = m_wargamesCount;
    info.data.battlegroundID = request.battlegroundId;

	ObjectGuid PacketInfo = info.raw;
	ObjectGuid ChallengerGuid = request.playerChallenger;
	RequestWargame.WriteBit(ChallengerGuid[1]);
	RequestWargame.WriteBit(ChallengerGuid[2]);
	RequestWargame.WriteBit(PacketInfo[7]);
	RequestWargame.WriteBit(PacketInfo[4]);
	RequestWargame.WriteBit(ChallengerGuid[4]);
	RequestWargame.WriteBit(PacketInfo[1]);
	RequestWargame.WriteBit(ChallengerGuid[5]);
	RequestWargame.WriteBit(PacketInfo[5]);
	RequestWargame.WriteBit(ChallengerGuid[7]);
	RequestWargame.WriteBit(PacketInfo[6]);
	RequestWargame.WriteBit(PacketInfo[3]);
	RequestWargame.WriteBit(ChallengerGuid[0]);
	RequestWargame.WriteBit(ChallengerGuid[3]);
	RequestWargame.WriteBit(PacketInfo[2]);
	RequestWargame.WriteBit(ChallengerGuid[6]);
	RequestWargame.WriteBit(PacketInfo[0]);
	RequestWargame.FlushBits();
	RequestWargame.WriteByteSeq(ChallengerGuid[2]);
	RequestWargame.WriteByteSeq(PacketInfo[0]);
	RequestWargame.WriteByteSeq(PacketInfo[2]);
	RequestWargame.WriteByteSeq(PacketInfo[4]);
	RequestWargame.WriteByteSeq(PacketInfo[6]);
	RequestWargame.WriteByteSeq(ChallengerGuid[0]);
	RequestWargame.WriteByteSeq(PacketInfo[5]);
	RequestWargame.WriteByteSeq(PacketInfo[7]);
	RequestWargame.WriteByteSeq(ChallengerGuid[3]);
	RequestWargame.WriteByteSeq(ChallengerGuid[5]);
	RequestWargame.WriteByteSeq(PacketInfo[1]);
	RequestWargame.WriteByteSeq(ChallengerGuid[7]);
	RequestWargame.WriteByteSeq(ChallengerGuid[4]);
	RequestWargame.WriteByteSeq(ChallengerGuid[1]);
	RequestWargame.WriteByteSeq(ChallengerGuid[6]);
	RequestWargame.WriteByteSeq(PacketInfo[3]);
	leaderChallenged->GetSession()->SendPacket(&RequestWargame);
} */
