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
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "Arena.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "LFG.h"
#include "Player.h"
#include "Object.h"
#include "Opcodes.h"
#include "DisableMgr.h"
#include "Group.h"

void WorldSession::HandleBattlemasterHelloOpcode(WorldPacket& recvData)
{
    uint64 guid;
    recvData >> guid;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEMASTER_HELLO Message from (GUID: %u TypeId:%u)", GUID_LOPART(guid), GuidHigh2TypeId(GUID_HIPART(guid)));

    Creature* unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if (!unit->isBattleMaster())                             // it's not battlemaster
        return;

    // Stop the npc if moving
    unit->StopMoving();

    BattlegroundTypeId bgTypeId = sBattlegroundMgr->GetBattleMasterBG(unit->GetEntry());

    if (!_player->GetBGAccessByLevel(bgTypeId))
    {
                                                            // temp, must be gossip message...
        SendNotification(LANG_YOUR_BG_LEVEL_REQ_ERROR);
        return;
    }

    SendBattleGroundList(guid, bgTypeId);
}

void WorldSession::SendBattleGroundList(uint64 guid, BattlegroundTypeId bgTypeId)
{
    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, guid, _player, bgTypeId);
    SendPacket(&data);
}

void WorldSession::HandleBattlemasterJoinOpcode(WorldPacket& recvData)
{
    uint8 MAX_DISABLED_BGS = 2;

    ObjectGuid guid;
    bool isPremade = false;
    bool HasNoDamageRole;                  // Doesn't have role ROLE_DAMAGE (see LFG.h for roles enum).
    bool asGroup;
    Group* grp;
    uint8 role;                            // Used for getting other roles.
    uint32 BgTypeId;
    uint32 disabledBgs[2];                 // Disabled battlegrounds (MAX_DISABLED_BGS - These are disabled from the dice icon near selection list - put it in "Check disabled BG's sent by client." below).

    for (uint8 i = 0; i < MAX_DISABLED_BGS; i++)
        recvData >> disabledBgs[i];

    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    HasNoDamageRole = !recvData.ReadBit();  // Doesn't have role ROLE_DAMAGE. Any class can select it so it seems to be filtered using this.

    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();

    asGroup = recvData.ReadBit();           // Join as Group.

    guid[4] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[1]);

    if (HasNoDamageRole)
        recvData >> role;
    else
        role = ROLE_DAMAGE;

    // Extract TypeId from guid.
    BgTypeId = GUID_LOPART(guid);

    if (!sBattlemasterListStore.LookupEntry(BgTypeId))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: invalid bgtype (%u) received. possible cheater? player guid %u", BgTypeId, _player->GetGUIDLow());
        return;
    }

    // Check if the BG is disabled by database.
    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BgTypeId, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
        return;
    }

    // Check disabled BG's sent by client selection filter.
    for (uint8 i = 0; i < MAX_DISABLED_BGS; i++)
    {
        if (disabledBgs[i] == BgTypeId)
        {
            ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
            return;
        }
    }

    BattlegroundTypeId bgTypeId = BattlegroundTypeId(BgTypeId);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEMASTER_JOIN Message from (GUID: %u TypeId:%u)", GUID_LOPART(guid), GuidHigh2TypeId(GUID_HIPART(guid)));

    // can do this, since it's battleground, not arena
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, 0);
    BattlegroundQueueTypeId bgQueueTypeIdRandom = BattlegroundMgr::BGQueueTypeId(BATTLEGROUND_RB, 0);

    // ignore if player is already in BG
    if (_player->InBattleground())
        return;

    // get bg instance or bg template if instance not found
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg)
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "Battleground: no available bg / template found");
        return;
    }

    // Expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    // Check queue conditions
    if (!asGroup)
    {
        // Check if using dungeon finder or raid finder.
        if (GetPlayer()->isUsingLfg())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_LFG_CANT_USE_BATTLEGROUND);
            GetPlayer()->GetSession()->SendPacket(&data);
            return;
        }

        // Check Deserter debuff
        if (!_player->CanJoinToBattleground())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // Check if already in Random BG queue.
        if (_player->GetBattlegroundQueueIndex(bgQueueTypeIdRandom) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_IN_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // Check if already in queue, can't start random queue.
        if (_player->InBattlegroundQueue() && bgTypeId == BATTLEGROUND_RB)
        {

            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_IN_NON_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // Check if already in same queue (dupe).
        if (_player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_BATTLEGROUND_DUPE_QUEUE);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // Check if has free queue slots.
        if (!_player->HasFreeBattlegroundQueueId())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, ERR_BATTLEGROUND_TOO_MANY_QUEUES);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        _player->SetBattleGroundRoles(role);

        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
        GroupQueueInfo* ginfo = bgQueue.AddGroup(_player, NULL, bgTypeId, bracketEntry, 0, false, isPremade, 0, 0);

        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        uint32 queueSlot = _player->AddBattlegroundQueueId(bgQueueTypeId);

        // add joined time data
        _player->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->ArenaType);
        SendPacket(&data);

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, _player->GetGUIDLow(), _player->GetName());
    }
    else
    {
        grp = _player->GetGroup();

        if (!grp) // No group found, error.
            return;

        if (grp->GetLeaderGUID() != _player->GetGUID())
            return;

        err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam(), false, 0);
        isPremade = (grp->GetMembersCount() >= bg->GetMinPlayersPerTeam());

        // Set leader role received from the data.
        _player->SetBattleGroundRoles(role);

        // If we're here, then the conditions to join a BG are met. We can proceed with joining.
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
        GroupQueueInfo* ginfo = NULL;
        uint32 avgTime = 0;

        if (!err)
        {
            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: the following players are joining as group:");
            ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, 0, false, isPremade, 0, 0);
            avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        }

        for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member)
                continue;   // this should never happen

            if (err)
            {
                WorldPacket data;
                sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            // Set member roles from those selected in the group (no data sent for them so can only get the roles like this).
            if (member != _player)
                member->SetBattleGroundRoles(member->GetBattleGroundRoles());

            // add to queue
            uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

            // add joined time data
            member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

            WorldPacket data; // send status packet (in queue)
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->ArenaType);
            member->GetSession()->SendPacket(&data);

            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
        }

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: group end");
    }

    sBattlegroundMgr->ScheduleQueueUpdate(0, 0, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleBattlegroundPlayerPositionsOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_PLAYER_POSITIONS Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)                                                 // can't be received if player not in battleground
        return;

    uint32 count = 0;
    std::list<Player*> players;

    if (Player* plr = ObjectAccessor::FindPlayer(bg->GetFlagPickerGUID(TEAM_ALLIANCE)))
    {
        players.push_back(plr);
        ++count;
    }

    if (Player* plr = ObjectAccessor::FindPlayer(bg->GetFlagPickerGUID(TEAM_HORDE)))
    {
        players.push_back(plr);
        ++count;
    }

    WorldPacket data(SMSG_BATTLEGROUND_PLAYER_POSITIONS);
    ByteBuffer dataBuffer;

    data.WriteBits(count, 20);

    for (auto itr : players)
    {
        ObjectGuid guid = itr->GetGUID();

        uint8 bits[8] = { 3, 5, 2, 0, 4, 6, 1, 7 };
        data.WriteBitInOrder(guid, bits);

        dataBuffer << uint8(itr->GetTeamId() == TEAM_ALLIANCE ? 1 : 2);

        dataBuffer.WriteByteSeq(guid[6]);

        dataBuffer << float(itr->GetPositionX());

        dataBuffer.WriteByteSeq(guid[7]);

        dataBuffer << float(itr->GetPositionY());

        dataBuffer.WriteByteSeq(guid[0]);
        dataBuffer.WriteByteSeq(guid[3]);
        dataBuffer.WriteByteSeq(guid[2]);

        dataBuffer << uint8(itr->GetTeamId() == TEAM_ALLIANCE ? 3 : 2); // 244 and 83 ?

        dataBuffer.WriteByteSeq(guid[4]);
        dataBuffer.WriteByteSeq(guid[1]);
        dataBuffer.WriteByteSeq(guid[5]);
    }

    data.FlushBits();

    if (dataBuffer.size())
        data.append(dataBuffer);

    SendPacket(&data);
}

void WorldSession::HandlePVPLogDataOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd MSG_PVP_LOG_DATA Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)
        return;

    // Prevent players from sending BuildPvpLogDataPacket in an arena except for when sent in BattleGround::EndBattleGround.
    if (bg->isArena())
        return;

    WorldPacket data;
    sBattlegroundMgr->BuildPvpLogDataPacket(&data, bg);
    SendPacket(&data);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent MSG_PVP_LOG_DATA Message");
}

void WorldSession::HandleBattlegroundListOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_LIST Message");

    uint32 bgTypeId;
    recvData >> bgTypeId;                                  // id from DBC

    BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
    if (!bl)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundHandler: invalid bgtype (%u) with player (Name: %s, GUID: %u) received.", bgTypeId, _player->GetName(), _player->GetGUIDLow());
        return;
    }

    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, 0, _player, BattlegroundTypeId(bgTypeId));
    SendPacket(&data);
}

void WorldSession::HandleBattlegroundPortOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_PORT Message");

    uint32 playerCount;
    uint32 joinTime;
    uint32 queueSlot;
    ObjectGuid playerGuid;

    bool action = recvData.ReadBit() ?  true : false;          // 128 = Accept and port; 0 = Cancel.

    recvData >> playerCount;                                   // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
    recvData >> queueSlot;
    recvData >> joinTime;

    playerGuid[6] = recvData.ReadBit();
    playerGuid[7] = recvData.ReadBit();
    playerGuid[1] = recvData.ReadBit();
    playerGuid[2] = recvData.ReadBit();
    playerGuid[0] = recvData.ReadBit();
    playerGuid[5] = recvData.ReadBit();
    playerGuid[4] = recvData.ReadBit();
    playerGuid[3] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(playerGuid[7]);
    recvData.ReadByteSeq(playerGuid[2]);
    recvData.ReadByteSeq(playerGuid[1]);
    recvData.ReadByteSeq(playerGuid[5]);
    recvData.ReadByteSeq(playerGuid[4]);
    recvData.ReadByteSeq(playerGuid[0]);
    recvData.ReadByteSeq(playerGuid[3]);
    recvData.ReadByteSeq(playerGuid[6]);

    if (!_player->InBattlegroundQueue())
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "CMSG_BATTLEFIELD_PORT: Player not in queue!");
        return;
    }

    // Get GroupQueueInfo from BattlegroundQueue.
    BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(queueSlot);

    if (bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "HandleBattlegroundPortOpcode: invalid bgQueueTypeId (%u) received.", bgQueueTypeId);
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "CMSG_BATTLEFIELD_PORT Player : %s Slot: %u, Playercount: %u, Join Time: %u, Action: %u. Invalid queueSlot!", _player->GetName(), queueSlot, playerCount, joinTime, action);
        return;
    }

    if (bgQueueTypeId >= MAX_BATTLEGROUND_QUEUE_TYPES)
    {
        sLog->OutPandashan("HandleBattlegroundPortOpcode: bgQueueTypeId %u in conflict with MAX_BATTLEGROUND_QUEUE_TYPES!", bgQueueTypeId);
        return;
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    // We must use a temporary variable, because GroupQueueInfo pointer can be deleted in BattlegroundQueue::RemovePlayer() function.
    GroupQueueInfo ginfo;
    if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "HandleBattlegroundPortOpcode: itrplayerstatus not found.");
        return;
    }

    BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(bgQueueTypeId);

    // BGTemplateId returns BATTLEGROUND_AA when it is arena queue.
    // Do instance id search as there is no AA bg instances.
    Battleground* bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID, bgTypeId == BATTLEGROUND_AA ? BATTLEGROUND_TYPE_NONE : bgTypeId);

    // bg template might and must be used in case of leaving queue, when instance is not created yet
    if (!bg && action == false)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
    if (!bg)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: bg_template not found for type id %u.", bgTypeId);
        return;
    }

    // get real bg type
    bgTypeId = bg->GetTypeID();

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: No bracket entry for Port!");
        return;
    }

    // some checks if player isn't cheating - it is not exactly cheating, but we cannot allow it
    if (action == true && ginfo.ArenaType == 0)
    {
        //if player is trying to enter battleground (not arena!) and he has deserter debuff, we must just remove him from queue
        if (!_player->CanJoinToBattleground())
        {
            //send bg command result to show nice message
            WorldPacket data2;
            sBattlegroundMgr->BuildStatusFailedPacket(&data2, bg, _player, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data2);
            action = false;
            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player %s (%u) has a deserter debuff, do not port him to battleground!", _player->GetName(), _player->GetGUIDLow());
        }
        //if player don't match battleground max level, then do not allow him to enter! (this might happen when player leveled up during his waiting in queue
        if (_player->getLevel() > bg->GetMaxLevel())
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: Player %s (%u) has level (%u) higher than maxlevel (%u) of battleground (%u)! Do not port him to battleground!",
                _player->GetName(), _player->GetGUIDLow(), _player->getLevel(), bg->GetMaxLevel(), bg->GetTypeID());
            action = false;
        }
    }

    if (action == true)
    {
        if (!_player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: BG Port : Not invited to BG Queue!");
            return;                                 // cheating?
        }

        if (!_player->InBattleground())
            _player->SetBattlegroundEntryPoint();

        // resurrect the player
        if (!_player->isAlive())
        {
            _player->ResurrectPlayer(1.0f);
            _player->SpawnCorpseBones();
        }

        // this is here like in MapManager.cpp as CanPlayerEnter. Dismounting/removing all mount an auras
        _player->Dismount();
        _player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        _player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

        // stop taxi flight at port
        _player->GetMotionMaster()->MovementExpired();
        if (_player->isInFlight())
            _player->CleanupAfterTaxiFlight();

        // WorldPacket data;
        // sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), bg->GetArenaType());
        // _player->GetSession()->SendPacket(&data);

        // remove battleground queue status from BGmgr
        bgQueue.RemovePlayer(_player->GetGUID(), false);
        // this is still needed here if battleground "jumping" shouldn't add deserter debuff
        // also this is required to prevent stuck at old battleground after SetBattlegroundId set to new
        if (Battleground* currentBg = _player->GetBattleground())
            currentBg->RemovePlayerAtLeave(_player->GetGUID(), false, true);

        // set the destination instance id
        _player->SetBattlegroundId(bg->GetInstanceID(), bgTypeId);
        // set the destination team
        _player->SetBGTeam(ginfo.Team);

        // bg->HandleBeforeTeleportToBattleground(_player);
        sBattlegroundMgr->SendToBattleground(_player, ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
        // add only in HandleMoveWorldPortAck()
        // bg->AddPlayer(_player, team);
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player %s (%u) joined battle for bg %u, bgtype %u, queue type %u.", _player->GetName(), _player->GetGUIDLow(), bg->GetInstanceID(), bg->GetTypeID(), bgQueueTypeId);
    }
    else
    {
        // leave queue
        if (bg->isArena() && bg->GetStatus() > STATUS_WAIT_QUEUE)
            return;

        _player->RemoveBattlegroundQueueId(bgQueueTypeId);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs

        WorldPacket data;
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_NONE, _player->GetBattlegroundQueueJoinTime(bgTypeId), 0, 0);
        _player->GetSession()->SendPacket(&data);

        bgQueue.RemovePlayer(_player->GetGUID(), true);

        if (ginfo.IsRatedBG)
        {
            for (auto plrInfo : ginfo.Players)
            {
                if (Player* player = ObjectAccessor::FindPlayer(plrInfo.first))
                {
                    player->RemoveBattlegroundQueueId(bgQueueTypeId);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs
                    bgQueue.RemovePlayer(player->GetGUID(), true);
                    sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, player, queueSlot, STATUS_NONE, player->GetBattlegroundQueueJoinTime(bgTypeId), 0, 0);
                    SendPacket(&data);
                }
            }
        }

        // player left queue, we should update it - do not update Arena Queue
        if (!ginfo.ArenaType)
            sBattlegroundMgr->ScheduleQueueUpdate(ginfo.ArenaMatchmakerRating, ginfo.ArenaType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
    }
}

void WorldSession::HandleBattlegroundLeaveOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_LEAVE Message");

    if (_player->InBattleground() || _player->InArena())
    {
        if (_player->InArena())
            if (_player->GetBattleground()->GetStatus() == STATUS_WAIT_JOIN)
                return;

        _player->LeaveBattleground();
    }
}

void WorldSession::HandleBattlegroundStatusOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_STATUS Message");

    // empty opcode

    WorldPacket data;
    // we must update all queues here
    Battleground* bg = NULL;
    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(i);
        if (!bgQueueTypeId)
            continue;
        BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(bgQueueTypeId);
        uint8 arenaType = BattlegroundMgr::BGArenaType(bgQueueTypeId);
        if (bgTypeId == _player->GetBattlegroundTypeId())
        {
            bg = _player->GetBattleground();
            //i cannot check any variable from player class because player class doesn't know if player is in 2v2 / 3v3 or 5v5 arena
            //so i must use bg pointer to get that information
            if (bg && bg->GetArenaType() == arenaType)
            {
                // this line is checked, i only don't know if GetElapsedTime() is changing itself after bg end!
                // send status in Battleground
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, i, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), arenaType);
                SendPacket(&data);
                continue;
            }
        }

        //we are sending update to player about queue - he can be invited there!
        //get GroupQueueInfo for queue status
        BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId];
        GroupQueueInfo ginfo;
        if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
            continue;
        if (ginfo.IsInvitedToBGInstanceGUID)
        {
            bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
            if (!bg)
                continue;

            // send status invited to Battleground
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, GetPlayer(), i, STATUS_WAIT_JOIN, getMSTimeDiff(getMSTime(), ginfo.RemoveInviteTime), _player->GetBattlegroundQueueJoinTime(bgTypeId), arenaType);
            SendPacket(&data);
        }
        else
        {
            bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
            if (!bg)
                continue;

            // expected bracket entry
            PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
            if (!bracketEntry)
                continue;

            uint32 avgTime = bgQueue.GetAverageQueueWaitTime(&ginfo, bracketEntry->GetBracketId());
            // send status in Battleground Queue
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, GetPlayer(), i, STATUS_WAIT_QUEUE, avgTime, _player->GetBattlegroundQueueJoinTime(bgTypeId), arenaType);
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleBattlemasterJoinArena(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_BATTLEMASTER_JOIN_ARENA");

    uint8 arenaslot;                                        // 2v2, 3v3 or 5v5

    recvData >> arenaslot;

    // ignore if we already in BG or BG queue
    if (_player->InBattleground())
        return;

    uint32 arenaRating = 0;
    uint32 matchmakerRating = 0;

    uint8 arenatype = Arena::GetTypeBySlot(arenaslot);

    //check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: template bg (all arenas) not found");
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_ARENA_DISABLED);
        return;
    }

    BattlegroundTypeId bgTypeId = bg->GetTypeID();
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, arenatype);

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    Group* grp = _player->GetGroup();

    // no group found, error
    if (!grp)
        return;

    if (grp->GetLeaderGUID() != _player->GetGUID())
        return;

    uint32 playerDivider = 0;
    for (GroupReference const* ref = grp->GetFirstMember(); ref != NULL; ref = ref->next())
    {
        if (Player const* groupMember = ref->getSource())
        {
            arenaRating += groupMember->GetArenaPersonalRating(arenaslot);
            matchmakerRating += groupMember->GetArenaMatchMakerRating(arenaslot);
            ++playerDivider;
        }
    }

    if (!playerDivider)
        return;

    arenaRating /= playerDivider;
    matchmakerRating /= playerDivider;

    if (arenaRating <= 0)
        arenaRating = 1;
    if (matchmakerRating <= 0)
        matchmakerRating = 1;

    BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    uint32 avgTime = 0;
    GroupQueueInfo* ginfo = NULL;

    err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, arenatype, arenatype, true, arenaslot);
    if (!err || (err && sBattlegroundMgr->isArenaTesting()))
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: leader %s queued with matchmaker rating %u for type %u", _player->GetName(), matchmakerRating, arenatype);

        ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, arenatype, true, false, arenaRating, matchmakerRating);
        avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
    }

    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member)
            continue;

        if (err && !sBattlegroundMgr->isArenaTesting())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, err);
            member->GetSession()->SendPacket(&data);
            continue;
        }

         // add to queue
        uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

        if (!ginfo)
        {
            sLog->OutPandashan("NULL ginfo !!!!");
            return;
        }

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, arenatype);
        member->GetSession()->SendPacket(&data);

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for arena as group bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
    }

    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, arenatype, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleBattlemasterJoinRated(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_BATTLEMASTER_JOIN_RATED");

    // ignore if we already in BG or BG queue
    if (_player->InBattleground())
        return;

    uint32 personalRating = 0;
    uint32 matchmakerRating = 0;

    //check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_RATED_10_VS_10);
    if (!bg)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: template bg (10 vs 10) not found");
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_RATED_10_VS_10, NULL))
        return;

    BattlegroundTypeId bgTypeId = bg->GetTypeID();
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, 0);

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    Group* grp = _player->GetGroup();

    // no group found, error
    if (!grp)
        return;

    if (grp->GetLeaderGUID() != _player->GetGUID())
        return;

    uint32 playerDivider = 0;
    for (GroupReference const* ref = grp->GetFirstMember(); ref != NULL; ref = ref->next())
    {
        if (Player const* groupMember = ref->getSource())
        {
            personalRating += groupMember->GetArenaPersonalRating(SLOT_RBG);
            matchmakerRating += groupMember->GetArenaMatchMakerRating(SLOT_RBG);
            ++playerDivider;
        }
    }

    if (!playerDivider)
        return;

    personalRating /= playerDivider;
    matchmakerRating /= playerDivider;

    if (personalRating <= 0)
        personalRating = 0;
    if (matchmakerRating <= 0)
        matchmakerRating = 1;

    BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    uint32 avgTime = 0;
    GroupQueueInfo* ginfo;

    err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, 10, 10, true, 0);
    if (!err)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: leader %s queued");

        ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, 0, false, true, personalRating, matchmakerRating);
        avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
    }

    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member)
            continue;

        if (err)
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, err);
            member->GetSession()->SendPacket(&data);
            continue;
        }

         // add to queue
        uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->ArenaType);
        member->GetSession()->SendPacket(&data);

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for rated battleground as group bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
    }
    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, 0, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleSendBattlegroundTimer(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_BATTLEGROUND_SEND_TIMER");

    // uint32 unk;
    recvData.rfinish();

    if (_player->InBattleground())
    {
        if (Battleground* bg = _player->GetBattleground())
        {
            int countdownSec = (bg->GetMaxCountdownTimer() - ceil(float(bg->GetElapsedTime()) / 1000));
            _player->SendBattlegroundTimer(countdownSec, bg->GetMaxCountdownTimer());
        }
    }
}

void WorldSession::HandleReportPvPAFK(WorldPacket& recvData)
{
    ObjectGuid playerGuid;

    uint8 bits[8] = { 4, 0, 1, 2, 7, 6, 3, 5 };
    recvData.ReadBitInOrder(playerGuid, bits);

    recvData.FlushBits();

    uint8 bytes[8] = { 4, 3, 2, 7, 1, 6, 5, 0 };
    recvData.ReadBytesSeq(playerGuid, bytes);

    Player* reportedPlayer = ObjectAccessor::FindPlayer(playerGuid);
    if (!reportedPlayer)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "WorldSession::HandleReportPvPAFK: player not found");
        return;
    }

    sLog->outDebug(LOG_FILTER_BATTLEGROUND, "WorldSession::HandleReportPvPAFK: %s reported %s", _player->GetName(), reportedPlayer->GetName());

    reportedPlayer->ReportedAfkBy(_player);
}

void WorldSession::HandleRequestPvpOptions(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_PVP_OPTIONS_ENABLED");

    /// @Todo: perfome research in this case
    WorldPacket data(SMSG_PVP_OPTIONS_ENABLED, 1);

    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);

    data.FlushBits();

    SendPacket(&data);
}

void WorldSession::HandleRequestPvpRewards(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_PVP_REWARDS");

    _player->SendPvpRewards();
}

void WorldSession::HandleRequestRatedBgInfo(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_RATED_BG_INFO");

    /// @Todo: perfome research in this case
    WorldPacket data(SMSG_RATED_BG_STATS, 72);
    for (int32 i = 0; i < 18; ++i)
        data << uint32(0);

    SendPacket(&data);
}

void WorldSession::HandleRequestRatedBgStats(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_RATED_BG_STATS");

    WorldPacket data(SMSG_BATTLEFIELD_RATED_INFO, 29);

    for (int i = 0; i < MAX_PVP_SLOT; i++)
    {
        data << uint32(_player->GetWeekGames(i)); // games of week
        data << uint32(_player->GetSeasonGames(i)); // games of season
        data << uint32(0);
        data << uint32(0);
        data << uint32(_player->GetArenaPersonalRating(i)); // current rating
        data << uint32(_player->GetBestRatingOfSeason(i)); // best rating of season
        data << uint32(_player->GetBestRatingOfWeek(i)); // best rating of week
        data << uint32(_player->GetPrevWeekWins(i)); // wins of prev week
    }

    SendPacket(&data);
}
