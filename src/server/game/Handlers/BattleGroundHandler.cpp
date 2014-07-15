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
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Arena.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "Player.h"
#include "Object.h"
#include "Opcodes.h"
#include "DisableMgr.h"
#include "Group.h"
#include "LFG.h"

void WorldSession::HandleBattlemasterHelloOpcode(WorldPacket& recvData)
{
    uint64 guid;
    recvData >> guid;
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEMASTER_HELLO Message from (GUID: %u TypeId:%u)", GUID_LOPART(guid), GuidHigh2TypeId(GUID_HIPART(guid)));

    Creature* unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if (!unit->IsBattleMaster())                             // it's not battlemaster
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
    bool HasNoDamageRole;                  // Doesn't have role PLAYER_ROLE_DAMAGE (see LFG.h for roles enum).
    uint8 role;                            // Used for getting other roles.
    bool asGroup;
    Group* grp;
    uint32 bgTypeId_;
    uint32 disabledBgs[2];                 // Disabled battlegrounds (MAX_DISABLED_BGS - These are disabled from the dice icon near selection list - put it in "Check disabled BG's sent by client." below).

    for (uint8 i = 0; i < MAX_DISABLED_BGS; i++)
        recvData >> disabledBgs[i];

    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    HasNoDamageRole = !recvData.ReadBit();  // Doesn't have role PLAYER_ROLE_DAMAGE. Any class can select it so it seems to be filtered using this.

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
        recvData >> role;                   // Get other roles.
    else
        role = lfg::PLAYER_ROLE_DAMAGE;

    // Extract from guid
    bgTypeId_ = GUID_LOPART(uint64(guid));

    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
    {
        TC_LOG_ERROR("network", "Battleground: invalid bgtype (%u) received. possible cheater? player guid %u", bgTypeId_, _player->GetGUIDLow());
        return;
    }

    // Check if the BG is disabled by database.
    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId_, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
        return;
    }

    // Check disabled BG's sent by client selection filter.
    for (uint8 i = 0; i < MAX_DISABLED_BGS; i++)
    {
        if (disabledBgs[i] == bgTypeId_)
        {
            ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
            return;
        }
    }

    BattlegroundTypeId bgTypeId = BattlegroundTypeId(bgTypeId_);

    //TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEMASTER_JOIN Message from (GUID:"UI64FMTD" TypeId:%u)", guid, bgTypeId_);

    // can do this, since it's battleground, not arena
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, 0);
    BattlegroundQueueTypeId bgQueueTypeIdRandom = BattlegroundMgr::BGQueueTypeId(BATTLEGROUND_RB, 0);

    // ignore if player is already in BG
    if (_player->InBattleground())
        return;

    // get bg instance or bg template if instance not found
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId) ? sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId) : NULL;

    if (!bg)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground: no available bg / template found");
        return;
    }

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    // check queue conditions
    if (!asGroup)
    {
        if (GetPlayer()->isUsingLfg())
        {
            // player is using dungeon finder or raid finder
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_LFG_CANT_USE_BATTLEGROUND);
            GetPlayer()->GetSession()->SendPacket(&data);
            return;
        }

        // check Deserter debuff
        if (!_player->CanJoinToBattleground(bg))
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // check if already in random queue
        if (_player->GetBattlegroundQueueIndex(bgQueueTypeIdRandom) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        {
            // player is already in random queue, dupe.
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_IN_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        if (_player->InBattlegroundQueue() && bgTypeId == BATTLEGROUND_RB)
        {
            // player is already in queue, can't start random queue
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_IN_NON_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // check if already in queue
        if (_player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        {
            // player is already in same queue, dupe.
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_BATTLEGROUND_DUPE_QUEUE);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // check if has free queue slots
        if (!_player->HasFreeBattlegroundQueueId())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_BATTLEGROUND_TOO_MANY_QUEUES);
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

        TC_LOG_DEBUG("bg.battleground", "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s",
                       bgQueueTypeId, bgTypeId, _player->GetGUIDLow(), _player->GetName().c_str());
    }
    else
    {
        grp = _player->GetGroup();

        // no group found, error
        if (!grp)
            return;

        if (grp->GetLeaderGUID() != _player->GetGUID())
            return;

        err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam(), false, 0);
        isPremade = (grp->GetMembersCount() >= bg->GetMinPlayersPerTeam());

        // Set leader role received from the data.
        _player->SetBattleGroundRoles(role);

        // if we're here, then the conditions to join a bg are met. We can proceed in joining.
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
        GroupQueueInfo* ginfo = NULL;
        uint32 avgTime = 0;

        if (!err)
        {
            TC_LOG_DEBUG("bg.battleground", "Battleground: the following players are joining as group:");
            ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, 0, false, isPremade, 0, 0);
            avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        }

        for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member)
                continue;   // this should never happen

            if (err)
            {
                WorldPacket data;
                sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            // Set member roles from those selected in the group (no data sent for them so can only get the roles like this).
            if (member != _player)
                member->SetBattleGroundRoles(member->GetGroup()->GetLfgRoles(member->GetGUID()));

            // add to queue
            uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

            // add joined time data
            member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

            WorldPacket data; // send status packet (in queue)
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->ArenaType);
            member->GetSession()->SendPacket(&data);

            TC_LOG_DEBUG("bg.battleground", "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName().c_str());
        }

        TC_LOG_DEBUG("bg.battleground", "Battleground: group end");
    }

    sBattlegroundMgr->ScheduleQueueUpdate(0, 0, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleBattlegroundPlayerPositionsOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEGROUND_PLAYER_POSITIONS Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)                                                 // can't be received if player not in battleground
        return;

    uint32 acount = 0;
    uint32 hcount = 0;
    Player* aplr = NULL;
    Player* hplr = NULL;

    if (uint64 guid = bg->GetFlagPickerGUID(BG_TEAM_ALLIANCE))
    {
        aplr = ObjectAccessor::FindPlayer(guid);
        if (aplr)
            ++acount;
    }

    if (uint64 guid = bg->GetFlagPickerGUID(BG_TEAM_HORDE))
    {
        hplr = ObjectAccessor::FindPlayer(guid);
        if (hplr)
            ++hcount;
    }

    ObjectGuid aguid = aplr ? aplr->GetGUID() : 0;
    ObjectGuid hguid = hplr ? hplr->GetGUID() : 0;

    WorldPacket data(SMSG_BATTLEFIELD_PLAYER_POSITIONS);

    data.WriteBits(acount, 22);
    for (uint8 i = 0; i < acount; i++)
    {
        data.WriteBit(aguid[3]);
        data.WriteBit(aguid[5]);
        data.WriteBit(aguid[1]);
        data.WriteBit(aguid[6]);
        data.WriteBit(aguid[7]);
        data.WriteBit(aguid[0]);
        data.WriteBit(aguid[2]);
        data.WriteBit(aguid[4]);
    }

    data.WriteBits(hcount, 22);
    for (uint8 i = 0; i < hcount; i++)
    {
        data.WriteBit(hguid[6]);
        data.WriteBit(hguid[5]);
        data.WriteBit(hguid[4]);
        data.WriteBit(hguid[7]);
        data.WriteBit(hguid[2]);
        data.WriteBit(hguid[1]);
        data.WriteBit(hguid[0]);
        data.WriteBit(hguid[3]);
    }

    data.FlushBits();

    for (uint8 i = 0; i < hcount; i++)
    {
        data.WriteByteSeq(hguid[2]);
        data.WriteByteSeq(hguid[1]);
        data << float(hplr->GetPositionY());
        data.WriteByteSeq(hguid[5]);
        data.WriteByteSeq(hguid[4]);
        data.WriteByteSeq(hguid[7]);
        data.WriteByteSeq(hguid[0]);
        data.WriteByteSeq(hguid[6]);
        data.WriteByteSeq(hguid[3]);
        data << float(hplr->GetPositionX());
    }

    for (uint8 i = 0; i < acount; i++)
    {
        data.WriteByteSeq(aguid[6]);
        data << float(aplr->GetPositionX());
        data.WriteByteSeq(aguid[5]);
        data.WriteByteSeq(aguid[3]);
        data << float(aplr->GetPositionY());
        data.WriteByteSeq(aguid[1]);
        data.WriteByteSeq(aguid[7]);
        data.WriteByteSeq(aguid[0]);
        data.WriteByteSeq(aguid[2]);
        data.WriteByteSeq(aguid[4]);
    }

    SendPacket(&data);
}

void WorldSession::HandlePVPLogDataOpcode(WorldPacket & /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_PVP_LOG_DATA Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)
        return;

    // Prevent players from sending BuildPvpLogDataPacket in an arena except for when sent in BattleGround::EndBattleGround.
    if (bg->isArena())
        return;

    WorldPacket data;
    sBattlegroundMgr->BuildPvpLogDataPacket(&data, bg);
    SendPacket(&data);

    TC_LOG_DEBUG("network", "WORLD: Sent SMSG_PVP_LOG_DATA Message");
}

void WorldSession::HandleBattlefieldListOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEFIELD_LIST Message");

    uint32 bgTypeId;
    recvData >> bgTypeId;                                  // id from DBC

    BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
    if (!bl)
    {
        TC_LOG_DEBUG("bg.battleground", "BattlegroundHandler: invalid bgtype (%u) with player (Name: %s, GUID: %u) received.", bgTypeId, _player->GetName().c_str(), _player->GetGUIDLow());
        return;
    }

    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, 0, _player, BattlegroundTypeId(bgTypeId));
    SendPacket(&data);
}

void WorldSession::HandleBattleFieldPortOpcode(WorldPacket &recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEFIELD_PORT Message");

    uint32 time;
    uint32 queueSlot;
    uint32 unk;
    uint8 action;                        // enter battle 0x1, leave queue 0x0
    ObjectGuid guid;

    action = recvData.ReadBit() ? 1 : 0; // 1 = accept and port; 0 = cancel.

    recvData >> unk;                     // Common value is 1. 
    recvData >> queueSlot;
    recvData >> time;

    guid[0] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[4]);

    if (!_player->InBattlegroundQueue())
    {
        TC_LOG_DEBUG("bg.battleground", "CMSG_BATTLEFIELD_PORT %s Slot: %u, Unk: %u, Time: %u, Action: %u. Player not in queue!", GetPlayerInfo().c_str(), queueSlot, unk, time, action);
        return;
    }

    // get GroupQueueInfo from BattlegroundQueue
    BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(queueSlot);
    if (bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
    {
        TC_LOG_DEBUG("bg.battleground", "CMSG_BATTLEFIELD_PORT %s Slot: %u, Unk: %u, Time: %u, Action: %u. Invalid queueSlot!", GetPlayerInfo().c_str(), queueSlot, unk, time, action);
        return;
    }

    BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(bgQueueTypeId);
    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    //we must use temporary variable, because GroupQueueInfo pointer can be deleted in BattlegroundQueue::RemovePlayer() function
    GroupQueueInfo ginfo;
    if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
    {
        TC_LOG_DEBUG("bg.battleground", "CMSG_BATTLEFIELD_PORT %s Slot: %u, Unk: %u, Time: %u, Action: %u. Player not in queue (No player Group Info)!", GetPlayerInfo().c_str(), queueSlot, unk, time, action);
        return;
    }
    // if action == 1, then instanceId is required
    if (!ginfo.IsInvitedToBGInstanceGUID && action == 1)
    {
        TC_LOG_DEBUG("bg.battleground", "CMSG_BATTLEFIELD_PORT %s Slot: %u, Unk: %u, Time: %u, Action: %u. Player is not invited to any bg!", GetPlayerInfo().c_str(), queueSlot, unk, time, action);
        return;
    }

    // BGTemplateId returns BATTLEGROUND_AA when it is arena queue. Do instance id search as there is no AA bg instances.
    Battleground* bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID, bgTypeId == BATTLEGROUND_AA ? BATTLEGROUND_TYPE_NONE : bgTypeId);
    if (!bg && action == 0)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg)
    {
        TC_LOG_ERROR("network", "BattlegroundHandler: bg_template not found for type id %u.", bgTypeId);
        return;
    }

    TC_LOG_DEBUG("bg.battleground", "CMSG_BATTLEFIELD_PORT %s Slot: %u, Unk: %u, Time: %u, Action: %u.", GetPlayerInfo().c_str(), queueSlot, unk, time, action);

    // get real bg type
    bgTypeId = bg->GetTypeID();

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    //some checks if player isn't cheating - it is not exactly cheating, but we cannot allow it
    if (action == 1 && ginfo.ArenaType == 0)
    {
        //if player is trying to enter battleground (not arena!) and he has deserter debuff, we must just remove him from queue
        if (!_player->CanJoinToBattleground(bg))
        {
            //send bg command result to show nice message
            WorldPacket data2;
            sBattlegroundMgr->BuildStatusFailedPacket(&data2, bg, _player, 0, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data2);
            action = 0;
            TC_LOG_DEBUG("bg.battleground", "Player %s (%u) has a deserter debuff, do not port him to battleground!", _player->GetName().c_str(), _player->GetGUIDLow());
        }
        //if player don't match battleground max level, then do not allow him to enter! (this might happen when player leveled up during his waiting in queue
        if (_player->getLevel() > bg->GetMaxLevel())
        {
            TC_LOG_DEBUG("network", "Player %s (%u) has level (%u) higher than maxlevel (%u) of battleground (%u)! Do not port him to battleground!",
                _player->GetName().c_str(), _player->GetGUIDLow(), _player->getLevel(), bg->GetMaxLevel(), bg->GetTypeID());
            action = 0;
        }
    }

    switch (action)
    {
        case 1:                                         // port to battleground
        {
            if (!_player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
                return;                                 // cheating?

            if (!_player->InBattleground())
                _player->SetBattlegroundEntryPoint();

            // resurrect the player
            if (!_player->IsAlive())
            {
                _player->ResurrectPlayer(1.0f);
                _player->SpawnCorpseBones();
            }

            // this is here like in MapManager.cpp as CanPlayerEnter. Dismounting/removing all mount an auras
            _player->Dismount();
            _player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            _player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

            // stop taxi flight at port
            if (_player->IsInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }

            WorldPacket data;
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), bg->GetArenaType());
            _player->GetSession()->SendPacket(&data);

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
            TC_LOG_DEBUG("bg.battleground", "Battleground: player %s (%u) joined battle for bg %u, bgtype %u, queue type %u.", _player->GetName().c_str(), _player->GetGUIDLow(), bg->GetInstanceID(), bg->GetTypeID(), bgQueueTypeId);
        }
        break;
        case 0:                                         // leave queue
        {
            if (bg->isArena() && bg->GetStatus() > STATUS_WAIT_QUEUE) // Prevent exploit.
                return;

            // if player leaves rated arena match before match start, it is counted as he played but he lost
            // if (ginfo.IsRated && bg->isArena() && ginfo.IsInvitedToBGInstanceGUID)
            // {
            //     ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(ginfo.Team);
            //     if (at)
            //     {
            //         TC_LOG_DEBUG("bg.battleground", "UPDATING memberLost's personal arena rating for %u by opponents rating: %u, because he has left queue!", GUID_LOPART(_player->GetGUID()), ginfo.OpponentsTeamRating);
            //         at->MemberLost(_player, ginfo.OpponentsMatchmakerRating);
            //         at->SaveToDB();
            //     }
            // }

            if (bg->isArena())
            {
                Battleground* bgs = sBattlegroundMgr->GetBattlegroundTemplate(ginfo.BgTypeId); // Here we get the actual arena template after the selection, by groupinfo.
                BattlegroundQueueTypeId bgQueueTypeIds = BattlegroundMgr::BGQueueTypeId(ginfo.BgTypeId, ginfo.ArenaType); // Must retrieve by arena type here.
                uint32 queueSlots = _player->GetBattlegroundQueueIndex(bgQueueTypeId); // Get actual queue slot.

                _player->RemoveBattlegroundQueueId(bgQueueTypeIds);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs
                WorldPacket data;
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bgs, _player, queueSlots, STATUS_NONE, ginfo.JoinTime, 0, 0);
                _player->GetSession()->SendPacket(&data);

                bgQueue.RemovePlayer(_player->GetGUID(), true);
            }
            else
            {
                _player->RemoveBattlegroundQueueId(bgQueueTypeId);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs
                WorldPacket data;
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_NONE, _player->GetBattlegroundQueueJoinTime(bgTypeId), 0, 0);
                _player->GetSession()->SendPacket(&data);

                bgQueue.RemovePlayer(_player->GetGUID(), true);
            }

            // player left queue, we should update it - do not update Arena Queue
            if (!ginfo.ArenaType)
                sBattlegroundMgr->ScheduleQueueUpdate(ginfo.ArenaMatchmakerRating, ginfo.ArenaType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());

            TC_LOG_DEBUG("bg.battleground", "Battleground: player %s (%u) left queue for bgtype %u, queue type %u.", _player->GetName().c_str(), _player->GetGUIDLow(), bg->GetTypeID(), bgQueueTypeId);
        }
        break;
        default:
            TC_LOG_DEBUG("bg.battleground", "Battleground port: unknown action %u", action);
            break;
    }
}

void WorldSession::HandleBattlefieldLeaveOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEFIELD_LEAVE Message");

    // not allow leave battleground in combat
    if (_player->IsInCombat())
        if (Battleground* bg = _player->GetBattleground())
            if (bg->GetStatus() != STATUS_WAIT_LEAVE)
                return;

    _player->LeaveBattleground();
}

void WorldSession::HandleBattlefieldStatusOpcode(WorldPacket & /*recvData*/)
{
    // empty opcode
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_BATTLEFIELD_STATUS Message");

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
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, GetPlayer(), i, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), arenaType);
                SendPacket(&data);
                continue;
            }
        }

        //we are sending update to player about queue - he can be invited there!
        //get GroupQueueInfo for queue status
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
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
    TC_LOG_DEBUG("network", "WORLD: CMSG_BATTLEMASTER_JOIN_ARENA");

    uint8 arenaslot;                                        // 2v2, 3v3 or 5v5

    recvData >> arenaslot;

    // ignore if we already in BG or BG queue
    if (_player->InBattleground())
        return;

    uint8 arenatype = Arena::GetTypeBySlot(arenaslot);

    uint32 arenaRating = 0;
    uint32 matchmakerRating = 0;

    if (!arenatype)
    {
        TC_LOG_ERROR("bg.arena", "Unknown arena type %u at HandleBattlemasterJoinArena()", arenatype);
        return;
    }

    // check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        TC_LOG_ERROR("network", "Battleground: template bg (all arenas) not found");
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
        TC_LOG_DEBUG("bg.battleground", "Battleground: arena team leader %s queued with matchmaker rating %u for type %u", _player->GetName().c_str(), matchmakerRating, arenatype);

        ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, arenatype, true, false, arenaRating, matchmakerRating);
        avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
    }

    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member)
            continue;

        WorldPacket data;

        if (err && !sBattlegroundMgr->isArenaTesting())
        {
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, err);
            member->GetSession()->SendPacket(&data);
            continue;
        }

        // add to queue
        uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, arenatype);
        member->GetSession()->SendPacket(&data);

        TC_LOG_DEBUG("bg.battleground", "Battleground: player joined queue for arena as group bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName().c_str());
    }

    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, arenatype, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleReportPvPAFK(WorldPacket& recvData)
{
    uint64 playerGuid;
    recvData >> playerGuid;
    Player* reportedPlayer = ObjectAccessor::FindPlayer(playerGuid);

    if (!reportedPlayer)
    {
        TC_LOG_DEBUG("bg.battleground", "WorldSession::HandleReportPvPAFK: player not found");
        return;
    }

    TC_LOG_DEBUG("bg.battleground", "WorldSession::HandleReportPvPAFK: %s reported %s", _player->GetName().c_str(), reportedPlayer->GetName().c_str());

    reportedPlayer->ReportedAfkBy(_player);
}

void WorldSession::HandleRequestPvpOptions(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REQUEST_PVP_OPTIONS_ENABLED");

    // null packet

    // Needs a bit of research.
    WorldPacket data(SMSG_PVP_OPTIONS_ENABLED, 1);
    data.WriteBit(1);
    data.WriteBit(1);       // WargamesEnabled
    data.WriteBit(1);
    data.WriteBit(1);       // RatedBGsEnabled
    data.WriteBit(1);       // RatedArenasEnabled

    data.FlushBits();

    SendPacket(&data);
}

void WorldSession::HandleRequestPvpReward(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REQUEST_PVP_REWARDS");

    // null packet

    _player->SendPvpRewards();
}

void WorldSession::HandleRequestRatedBgInfo(WorldPacket & recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REQUEST_RATED_BG_INFO");

    // null packet

    // uint8 ratedId; // The server must have this in the response, it is a kind of ID I guess. It's 3 for rated BGs and <= 2 for rated arena. For arenas its an ID.
    // recvData >> ratedId;

    // TC_LOG_DEBUG("bg.battleground", "WorldSession::HandleRequestRatedBgInfo: ratedId = %u", ratedId);

    WorldPacket data(SMSG_BATTLEFIELD_RATED_INFO, 29);
    // data << uint32(ratedId < 3 ? 180 : 400);  // Arena / RBG Conquest points for win. Depends on ratedId.
    // data << ratedId;
    data << _player->GetRatedBGRating(_player->GetGUID());  // BG rating you have.
    data << _player->GetCurrencyOnWeek(CURRENCY_TYPE_CONQUEST_META_RBG, true);  // RBG Conquest points earned this week.
    data << _player->GetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_META_RBG, true); // RBG points limit.
    data << _player->GetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_META_ARENA, true);  // Arena Conquest points limit.
    data << _player->GetCurrencyOnWeek(CURRENCY_TYPE_CONQUEST_META_ARENA, true);  // Arena Conquest points earned this week.
    data << _player->GetCurrency(CURRENCY_TYPE_CONQUEST_POINTS, true); // Total earned ? (could also be total cap, idk).

    SendPacket(&data);
}

void WorldSession::HandleRequestRatedBgStats(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REQUEST_RATED_BG_STATS");

    // null packet

    WorldPacket data(SMSG_RATED_BG_STATS, 72);

    /*
    for (uint8 i = 0; i < MAX_ARENA_SLOT; i++)
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
    */

    /* A 15 in the end means for rated 15v15, 10 => 10v10 and 5 => 5v5. All of them are sent as uint32, you can't have a negative, duh?! :D */
    data << uint32(0); // weekWon5
    data << uint32(0); // weekPlayed5
    data << uint32(_player->GetRatedBGsPlayedWeek(_player->GetGUID())); // weekPlayed10
    data << uint32(_player->GetRatedBGsPlayed(_player->GetGUID()));     // seasonPlayed10
    data << uint32(0); // weekWon15
    data << uint32(0); // seasonPlayed5
    data << uint32(_player->GetRatedBGsWon(_player->GetGUID()));        // seasonWon10
    data << uint32(0); // rating15
    data << uint32(_player->GetRatedBGsWonWeek(_player->GetGUID()));    // weekWon10
    data << uint32(0); // rank5
    data << uint32(0); // seasonWon15
    data << uint32(_player->GetRatedBGRating(_player->GetGUID()));      // rating10
    data << uint32(0); // seasonPlayed15
    data << uint32(0); // seasonWon5
    data << uint32(0); // rank15
    data << uint32(0); // weekPlayed15
    data << uint32(0); // rating5
    data << uint32(0); // rank10

    SendPacket(&data);
}

/* Below need implementation.
void WorldSession::HandleBattlegroundStateQuery(WorldPacket& recvData)
{
    Battleground* bg = _player->GetBattleground();
    if (bg)
        bg->GetStatus();
}

void WorldSession::HandleWargameRequest(WorldPacket& recvData)
{
    ObjectGuid BattlegroundGUID,TargetPlayerGUID;

    TargetPlayerGUID[0] = recvData.ReadBit();
    TargetPlayerGUID[7] = recvData.ReadBit();
    BattlegroundGUID[3] = recvData.ReadBit();
    BattlegroundGUID[7] = recvData.ReadBit();
    BattlegroundGUID[1] = recvData.ReadBit();
    TargetPlayerGUID[5] = recvData.ReadBit();
    TargetPlayerGUID[1] = recvData.ReadBit();
    TargetPlayerGUID[2] = recvData.ReadBit();
    BattlegroundGUID[6] = recvData.ReadBit();
    BattlegroundGUID[5] = recvData.ReadBit();
    BattlegroundGUID[2] = recvData.ReadBit();
    BattlegroundGUID[0] = recvData.ReadBit();
    BattlegroundGUID[4] = recvData.ReadBit();
    TargetPlayerGUID[4] = recvData.ReadBit();
    TargetPlayerGUID[3] = recvData.ReadBit();
    TargetPlayerGUID[6] = recvData.ReadBit();

    recvData.ReadByteSeq(TargetPlayerGUID[6]);
    recvData.ReadByteSeq(BattlegroundGUID[7]);
    recvData.ReadByteSeq(BattlegroundGUID[3]);
    recvData.ReadByteSeq(TargetPlayerGUID[4]);
    recvData.ReadByteSeq(BattlegroundGUID[5]);
    recvData.ReadByteSeq(BattlegroundGUID[2]);
    recvData.ReadByteSeq(TargetPlayerGUID[1]);
    recvData.ReadByteSeq(TargetPlayerGUID[3]);
    recvData.ReadByteSeq(TargetPlayerGUID[5]);
    recvData.ReadByteSeq(BattlegroundGUID[0]);
    recvData.ReadByteSeq(TargetPlayerGUID[2]);
    recvData.ReadByteSeq(TargetPlayerGUID[7]);
    recvData.ReadByteSeq(BattlegroundGUID[6]);
    recvData.ReadByteSeq(TargetPlayerGUID[0]);
    recvData.ReadByteSeq(BattlegroundGUID[1]);
    recvData.ReadByteSeq(BattlegroundGUID[4]);

    uint32 PartyMembersCount = ((((BattlegroundGUID & 0xFFFFFFFF) ^ 0x30000) >> 24) & 0x3F);
    uint32 BattlegroundID = ((BattlegroundGUID & 0xFFFF0000) ^ 0x30000);

    ///sLog->outDebug(LOG_FILTER_NETWORKIO,"Received CMSG_WARGAME_START with guid: %u",guid);

    if(!ObjectAccessor::FindPlayer(TargetPlayerGUID))
        return;

    WargameRequest newRequest;
    newRequest.battlegroundId = BattlegroundID;
    newRequest.playerChallenger = GetPlayer()->GetGUID();
    newRequest.playerChallenged = TargetPlayerGUID;//caused crash!!

    sBattlegroundMgr->HandleWargameRequest(newRequest);
}

void WorldSession::HandleWargameResponse(WorldPacket& recvData)
{
    bool accepted;
    ObjectGuid TargetPlayerGUID,PackedInfo;

    accepted = recvData.ReadBit();
    PackedInfo[3] = recvData.ReadBit();
    TargetPlayerGUID[3] = recvData.ReadBit();
    PackedInfo[7] = recvData.ReadBit();
    TargetPlayerGUID[2] = recvData.ReadBit();
    TargetPlayerGUID[0] = recvData.ReadBit();
    PackedInfo[1] = recvData.ReadBit();
    TargetPlayerGUID[5] = recvData.ReadBit();
    PackedInfo[6] = recvData.ReadBit();
    TargetPlayerGUID[6] = recvData.ReadBit();
    TargetPlayerGUID[1] = recvData.ReadBit();
    PackedInfo[0] = recvData.ReadBit();
    TargetPlayerGUID[7] = recvData.ReadBit();
    TargetPlayerGUID[4] = recvData.ReadBit();
    PackedInfo[2] = recvData.ReadBit();
    PackedInfo[4] = recvData.ReadBit();
    PackedInfo[2] = recvData.ReadBit();

    recvData.ReadByteSeq(PackedInfo[2]);
    recvData.ReadByteSeq(PackedInfo[6]);
    recvData.ReadByteSeq(PackedInfo[4]);
    recvData.ReadByteSeq(TargetPlayerGUID[0]);
    recvData.ReadByteSeq(PackedInfo[5]);
    recvData.ReadByteSeq(TargetPlayerGUID[1]);
    recvData.ReadByteSeq(TargetPlayerGUID[6]);
    recvData.ReadByteSeq(TargetPlayerGUID[3]);
    recvData.ReadByteSeq(TargetPlayerGUID[5]);
    recvData.ReadByteSeq(TargetPlayerGUID[0]);
    recvData.ReadByteSeq(PackedInfo[3]);
    recvData.ReadByteSeq(PackedInfo[1]);
    recvData.ReadByteSeq(TargetPlayerGUID[7]);
    recvData.ReadByteSeq(PackedInfo[0]);
    recvData.ReadByteSeq(PackedInfo[7]);
    recvData.ReadByteSeq(TargetPlayerGUID[4]);

    sLog->outDebug(LOG_FILTER_NETWORKIO,"Received CMSG_WARGAME_ACCEPT with TargetPlayerGUID: %u",TargetPlayerGUID);
    /// here must happen the actual creation of the BG, teleport and shit :P
}

void WorldSession::HandleInspectRatedBGStats(WorldPacket& recvData)
{
    ObjectGuid targetGuid;

    targetGuid[1] = recvData.ReadBit();
    targetGuid[4] = recvData.ReadBit();
    targetGuid[6] = recvData.ReadBit();
    targetGuid[5] = recvData.ReadBit();
    targetGuid[0] = recvData.ReadBit();
    targetGuid[2] = recvData.ReadBit();
    targetGuid[7] = recvData.ReadBit();
    targetGuid[3] = recvData.ReadBit();

    recvData.ReadByteSeq(targetGuid[4]);
    recvData.ReadByteSeq(targetGuid[7]);
    recvData.ReadByteSeq(targetGuid[2]);
    recvData.ReadByteSeq(targetGuid[5]);
    recvData.ReadByteSeq(targetGuid[6]);
    recvData.ReadByteSeq(targetGuid[3]);
    recvData.ReadByteSeq(targetGuid[0]);
    recvData.ReadByteSeq(targetGuid[1]);

    Player* target = sObjectAccessor->FindPlayer(targetGuid);

    if(!target)
        return;

    uint32 rating = target->GetRatedBGRating(target->GetGUID()); // these are of the player inspected, they are seasonal (meaning alltime, as we do not have seasons implemented).
    uint32 played = target->GetRatedBGsPlayed(target->GetGUID());
    uint32 won = target->GetRatedBGsWon(target->GetGUID());

    WorldPacket response(SMSG_INSPECT_RATED_BG_STATS);
    uint8 guidMask[] = { 6,4,5,1,2,7,3,0 };
    uint8 guidBytes[] = { 4,1,7,3,6,2,5,0 };

    response.WriteGuidMask(targetGuid,guidMask,8);
    response.FlushBits();
    response.WriteGuidBytes(targetGuid,guidBytes,1,0);
    response << rating;
    response.WriteGuidBytes(targetGuid,guidBytes,4,1);
    response << won << played;
    response.WriteGuidBytes(targetGuid,guidBytes,3,5);

    SendPacket(&response);
}

void WorldSession::HandleBattlemasterJoinRated(WorldPacket& recvData)
{
    // empty packet, but we have everything we need
    if(!GetPlayer()->GetGroup() || GetPlayer()->GetGroup()->GetMembersCount() < 10)
        return;

    uint32 bgTypeId_ = BATTLEGROUND_RATED_10_VS_10;
    uint32 instanceId = _player->GetInstanceId();
    uint8 asGroup = true;
    bool isPremade = true;
    Group* grp = _player->GetGroup();
    ObjectGuid guid;

    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
    {
        sLog->outError("Battleground: invalid bgtype (%u) received. possible cheater? player guid %u", bgTypeId_, _player->GetGUIDLow());
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId_, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
        return;
    }

    BattlegroundTypeId bgTypeId = BattlegroundTypeId(bgTypeId_);

    // can do this, since it's battleground, not arena.
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, 0);

    // ignore if player is already in BG.
    if (_player->InBattleground())
        return;

    // get bg instance or bg template if instance not found.
    Battleground* bg = NULL;
    if (instanceId)
        bg = sBattlegroundMgr->GetBattlegroundThroughClientInstance(instanceId, bgTypeId);

    if (!bg)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg && !sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId))
    {
        sLog->outError("Battleground: no available bg / template found");
        return;
    }

    // expected bracket entry.
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    // now check group and add it.
    if (!grp)
        return;

    if (grp->GetLeaderGUID() != _player->GetGUID())
        return;

    err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam(), true, 0);

    if (_player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
    {
        // player is already in rated queue
        WorldPacket data;
        sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_BATTLEDGROUND_QUEUED_FOR_RATED);
        _player->GetSession()->SendPacket(&data);
        return;
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
    GroupQueueInfo* ginfo = NULL;
    uint32 avgTime = 0;

    if (!err)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: the following players are joining as group:");
        ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, 0, true, isPremade, 0, 0);
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
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, err);
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
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s",
            bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
    }

    sBattlegroundMgr->ScheduleQueueUpdate(0, 0, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}*/
