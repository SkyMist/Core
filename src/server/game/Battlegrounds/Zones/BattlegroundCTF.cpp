/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

#include "BattlegroundCTF.h"

// these variables aren't used outside of this file, so declare them only here
enum BG_CTF_Rewards
{
    BG_CTF_WIN           = 0,
    BG_CTF_FLAG_CAP,
    BG_CTF_MAP_COMPLETE,
    BG_CTF_REWARD_NUM
};

uint32 BG_CTF_Honor[BG_HONOR_MODE_NUM][BG_CTF_REWARD_NUM] =
{
    {20, 40, 40}, // normal honor
    {60, 40, 80}  // holiday
};

BattlegroundCTF::BattlegroundCTF()
{
    BgObjects.resize(BG_CTF_OBJECT_MAX);
    BgCreatures.resize(BG_CREATURES_MAX_CTF);

    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_CTF_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_CTF_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_CTF_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_CTF_HAS_BEGUN;

    _flagSpellForceTimer = 0;
    _bothFlagsKept = false;
    _flagDebuffState = 0;
}

BattlegroundCTF::~BattlegroundCTF() { }

void BattlegroundCTF::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (GetElapsedTime() >= 27 * MINUTE * IN_MILLISECONDS)
        {
            if (GetTeamScore(BG_TEAM_ALLIANCE) == 0)
            {
                if (GetTeamScore(BG_TEAM_HORDE) == 0)        // No one scored - result is tie
                    EndBattleground(WINNER_NONE);
                else                                 // Horde has more points and thus wins
                    EndBattleground(HORDE);
            }
            else if (GetTeamScore(BG_TEAM_HORDE) == 0)
                EndBattleground(ALLIANCE);           // Alliance has > 0, Horde has 0, alliance wins
            else if (GetTeamScore(BG_TEAM_HORDE) == GetTeamScore(BG_TEAM_ALLIANCE)) // Team score equal, winner is team that scored the last flag
                EndBattleground(_lastFlagCaptureTeam);
            else if (GetTeamScore(BG_TEAM_HORDE) > GetTeamScore(BG_TEAM_ALLIANCE))  // Last but not least, check who has the higher score
                EndBattleground(HORDE);
            else
                EndBattleground(ALLIANCE);
        }
        // first update needed after 1 minute of game already in progress
        else if (GetElapsedTime() > uint32(_minutesElapsed * MINUTE * IN_MILLISECONDS) +  3 * MINUTE * IN_MILLISECONDS)
        {
            ++_minutesElapsed;
            UpdateWorldState(BG_CTF_STATE_TIMER, 25 - _minutesElapsed);
        }

        if (_flagState[BG_TEAM_ALLIANCE] == BG_CTF_FLAG_STATE_WAIT_RESPAWN)
        {
            _flagsTimer[BG_TEAM_ALLIANCE] -= diff;

            if (_flagsTimer[BG_TEAM_ALLIANCE] < 0)
            {
                _flagsTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlag(ALLIANCE, true);
            }
        }

        if (_flagState[BG_TEAM_ALLIANCE] == BG_CTF_FLAG_STATE_ON_GROUND)
        {
            _flagsDropTimer[BG_TEAM_ALLIANCE] -= diff;

            if (_flagsDropTimer[BG_TEAM_ALLIANCE] < 0)
            {
                _flagsDropTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlagAfterDrop(ALLIANCE);
                _bothFlagsKept = false;
            }
        }

        if (_flagState[BG_TEAM_HORDE] == BG_CTF_FLAG_STATE_WAIT_RESPAWN)
        {
            _flagsTimer[BG_TEAM_HORDE] -= diff;

            if (_flagsTimer[BG_TEAM_HORDE] < 0)
            {
                _flagsTimer[BG_TEAM_HORDE] = 0;
                RespawnFlag(HORDE, true);
            }
        }

        if (_flagState[BG_TEAM_HORDE] == BG_CTF_FLAG_STATE_ON_GROUND)
        {
            _flagsDropTimer[BG_TEAM_HORDE] -= diff;

            if (_flagsDropTimer[BG_TEAM_HORDE] < 0)
            {
                _flagsDropTimer[BG_TEAM_HORDE] = 0;
                RespawnFlagAfterDrop(HORDE);
                _bothFlagsKept = false;
            }
        }

        if (_bothFlagsKept)
        {
            _flagSpellForceTimer += diff;
            if (_flagDebuffState == 0 && _flagSpellForceTimer >= 10*MINUTE*IN_MILLISECONDS)  //10 minutes
            {
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
                    player->CastSpell(player, CTF_SPELL_FOCUSED_ASSAULT, true);
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
                    player->CastSpell(player, CTF_SPELL_FOCUSED_ASSAULT, true);
                _flagDebuffState = 1;
            }
            else if (_flagDebuffState == 1 && _flagSpellForceTimer >= 900000) //15 minutes
            {
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
                {
                    player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, CTF_SPELL_BRUTAL_ASSAULT, true);
                }
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
                {
                    player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, CTF_SPELL_BRUTAL_ASSAULT, true);
                }
                _flagDebuffState = 2;
            }
        }
        else
        {
            if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
            {
                player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
                player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);
            }
            if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
            {
                player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
                player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);
            }

            _flagSpellForceTimer = 0; //reset timer.
            _flagDebuffState = 0;
        }
    }
}

void BattlegroundCTF::StartingEventCloseDoors()
{
    for (uint32 i = BG_CTF_OBJECT_DOOR_A_1; i <= BG_CTF_OBJECT_DOOR_H_4; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_CTF_OBJECT_A_FLAG; i <= BG_CTF_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);

    UpdateWorldState(BG_CTF_STATE_TIMER_ACTIVE, 1);
    UpdateWorldState(BG_CTF_STATE_TIMER, 25);
}

void BattlegroundCTF::StartingEventOpenDoors()
{
    for (uint32 i = BG_CTF_OBJECT_DOOR_A_1; i <= BG_CTF_OBJECT_DOOR_A_6; ++i)
        DoorOpen(i);
    for (uint32 i = BG_CTF_OBJECT_DOOR_H_1; i <= BG_CTF_OBJECT_DOOR_H_4; ++i)
        DoorOpen(i);

    for (uint32 i = BG_CTF_OBJECT_A_FLAG; i <= BG_CTF_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    SpawnBGObject(BG_CTF_OBJECT_DOOR_A_5, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_CTF_OBJECT_DOOR_A_6, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_CTF_OBJECT_DOOR_H_3, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_CTF_OBJECT_DOOR_H_4, RESPAWN_ONE_DAY);

    // players joining later are not eligibles
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, CTF_EVENT_START_BATTLE);
}

void BattlegroundCTF::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in constructor
    BattlegroundCTFScore* sc = new BattlegroundCTFScore;

    PlayerScores[player->GetGUID()] = sc;
}

void BattlegroundCTF::RespawnFlag(uint32 Team, bool captured)
{
    if (Team == ALLIANCE)
    {
        TC_LOG_DEBUG("bg.battleground", "Respawn Alliance flag");
        _flagState[BG_TEAM_ALLIANCE] = BG_CTF_FLAG_STATE_ON_BASE;
    }
    else
    {
        TC_LOG_DEBUG("bg.battleground", "Respawn Horde flag");
        _flagState[BG_TEAM_HORDE] = BG_CTF_FLAG_STATE_ON_BASE;
    }

    if (captured)
    {
        //when map_update will be allowed for battlegrounds this code will be useless
        SpawnBGObject(BG_CTF_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnBGObject(BG_CTF_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_CTF_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_CTF_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
    _bothFlagsKept = false;
}

void BattlegroundCTF::RespawnFlagAfterDrop(uint32 team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    RespawnFlag(team, false);
    if (team == ALLIANCE)
    {
        SpawnBGObject(BG_CTF_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_CTF_ALLIANCE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnBGObject(BG_CTF_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_CTF_HORDE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }

    PlaySoundToAll(BG_CTF_SOUND_FLAGS_RESPAWNED);

    if (GameObject* obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID(team)))
        obj->Delete();
    else
        TC_LOG_ERROR("bg.battleground", "unknown droped flag bg, guid: %u", GUID_LOPART(GetDroppedFlagGUID(team)));

    SetDroppedFlagGUID(0, GetTeamIndexByTeamId(team));
    _bothFlagsKept = false;
}

void BattlegroundCTF::EventPlayerCapturedFlag(Player* player)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 winner = 0;

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    if (player->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        SetHordeFlagPicker(0);                              // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // horde flag in base (but not respawned yet)
        _flagState[BG_TEAM_HORDE] = BG_CTF_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Horde Flag from Player
        player->RemoveAurasDueToSpell(BG_CTF_SPELL_HORDE_FLAG);
        if (_flagDebuffState == 1)
          player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
        else if (_flagDebuffState == 2)
          player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);

        if (GetTeamScore(BG_TEAM_ALLIANCE) < BG_CTF_MAX_TEAM_SCORE)
            AddPoint(ALLIANCE, 1);
        PlaySoundToAll(BG_CTF_SOUND_FLAG_CAPTURED_ALLIANCE);
        RewardReputationToTeam(890, m_ReputationCapture, ALLIANCE);
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        SetAllianceFlagPicker(0);                           // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // alliance flag in base (but not respawned yet)
        _flagState[BG_TEAM_ALLIANCE] = BG_CTF_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Alliance Flag from Player
        player->RemoveAurasDueToSpell(BG_CTF_SPELL_ALLIANCE_FLAG);
        if (_flagDebuffState == 1)
          player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
        else if (_flagDebuffState == 2)
          player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);

        if (GetTeamScore(BG_TEAM_HORDE) < BG_CTF_MAX_TEAM_SCORE)
            AddPoint(HORDE, 1);
        PlaySoundToAll(BG_CTF_SOUND_FLAG_CAPTURED_HORDE);
        RewardReputationToTeam(889, m_ReputationCapture, HORDE);
    }
    //for flag capture is reward 2 honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), player->GetTeam());

    SpawnBGObject(BG_CTF_OBJECT_H_FLAG, BG_CTF_FLAG_RESPAWN_TIME);
    SpawnBGObject(BG_CTF_OBJECT_A_FLAG, BG_CTF_FLAG_RESPAWN_TIME);

    if (player->GetTeam() == ALLIANCE)
        SendMessageToAll(LANG_BG_CTF_CAPTURED_HF, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
    else
        SendMessageToAll(LANG_BG_CTF_CAPTURED_AF, CHAT_MSG_BG_SYSTEM_HORDE, player);

    UpdateFlagState(player->GetTeam(), 1);                  // flag state none
    UpdateTeamScore(player->GetTeamId());
    // only flag capture should be updated
    UpdatePlayerScore(player, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures

    // update last flag capture to be used if teamscore is equal
    SetLastFlagCapture(player->GetTeam());

    if (GetTeamScore(BG_TEAM_ALLIANCE) == BG_CTF_MAX_TEAM_SCORE)
        winner = ALLIANCE;

    if (GetTeamScore(BG_TEAM_HORDE) == BG_CTF_MAX_TEAM_SCORE)
        winner = HORDE;

    if (winner)
    {
        UpdateWorldState(BG_CTF_FLAG_UNK_ALLIANCE, 0);
        UpdateWorldState(BG_CTF_FLAG_UNK_HORDE, 0);
        UpdateWorldState(BG_CTF_FLAG_STATE_ALLIANCE, 1);
        UpdateWorldState(BG_CTF_FLAG_STATE_HORDE, 1);
        UpdateWorldState(BG_CTF_STATE_TIMER_ACTIVE, 0);

        RewardHonorToTeam(BG_CTF_Honor[m_HonorMode][BG_CTF_WIN], winner);
        EndBattleground(winner);
    }
    else
    {
        _flagsTimer[GetTeamIndexByTeamId(player->GetTeam()) ? 0 : 1] = BG_CTF_FLAG_RESPAWN_TIME;
    }
}

void BattlegroundCTF::EventPlayerDroppedFlag(Player* player)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (player->GetTeam() == ALLIANCE)
        {
            if (!IsHordeFlagPickedup())
                return;

            if (GetFlagPickerGUID(BG_TEAM_HORDE) == player->GetGUID())
            {
                SetHordeFlagPicker(0);
                player->RemoveAurasDueToSpell(BG_CTF_SPELL_HORDE_FLAG);
            }
        }
        else
        {
            if (!IsAllianceFlagPickedup())
                return;

            if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == player->GetGUID())
            {
                SetAllianceFlagPicker(0);
                player->RemoveAurasDueToSpell(BG_CTF_SPELL_ALLIANCE_FLAG);
            }
        }
        return;
    }

    bool set = false;

    if (player->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        if (GetFlagPickerGUID(BG_TEAM_HORDE) == player->GetGUID())
        {
            SetHordeFlagPicker(0);
            player->RemoveAurasDueToSpell(BG_CTF_SPELL_HORDE_FLAG);
            if (_flagDebuffState == 1)
              player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
            else if (_flagDebuffState == 2)
              player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);
            _flagState[BG_TEAM_HORDE] = BG_CTF_FLAG_STATE_ON_GROUND;
            player->CastSpell(player, BG_CTF_SPELL_HORDE_FLAG_DROPPED, true);
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == player->GetGUID())
        {
            SetAllianceFlagPicker(0);
            player->RemoveAurasDueToSpell(BG_CTF_SPELL_ALLIANCE_FLAG);
            if (_flagDebuffState == 1)
              player->RemoveAurasDueToSpell(CTF_SPELL_FOCUSED_ASSAULT);
            else if (_flagDebuffState == 2)
              player->RemoveAurasDueToSpell(CTF_SPELL_BRUTAL_ASSAULT);
            _flagState[BG_TEAM_ALLIANCE] = BG_CTF_FLAG_STATE_ON_GROUND;
            player->CastSpell(player, BG_CTF_SPELL_ALLIANCE_FLAG_DROPPED, true);
            set = true;
        }
    }

    if (set)
    {
        player->CastSpell(player, SPELL_RECENTLY_DROPPED_FLAG, true);
        UpdateFlagState(player->GetTeam(), 1);

        if (player->GetTeam() == ALLIANCE)
        {
            SendMessageToAll(LANG_BG_CTF_DROPPED_HF, CHAT_MSG_BG_SYSTEM_HORDE, player);
            UpdateWorldState(BG_CTF_FLAG_UNK_HORDE, uint32(-1));
        }
        else
        {
            SendMessageToAll(LANG_BG_CTF_DROPPED_AF, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
            UpdateWorldState(BG_CTF_FLAG_UNK_ALLIANCE, uint32(-1));
        }

        _flagsDropTimer[GetTeamIndexByTeamId(player->GetTeam()) ? 0 : 1] = BG_CTF_FLAG_DROP_TIME;
    }
}

void BattlegroundCTF::EventPlayerClickedOnFlag(Player* player, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    int32 message_id = 0;
    ChatMsg type = CHAT_MSG_BG_SYSTEM_NEUTRAL;

    //alliance flag picked up from base
    if (player->GetTeam() == HORDE && GetFlagState(ALLIANCE) == BG_CTF_FLAG_STATE_ON_BASE
        && BgObjects[BG_CTF_OBJECT_A_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_CTF_PICKEDUP_AF;
        type = CHAT_MSG_BG_SYSTEM_HORDE;
        PlaySoundToAll(BG_CTF_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SpawnBGObject(BG_CTF_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
        SetAllianceFlagPicker(player->GetGUID());
        _flagState[BG_TEAM_ALLIANCE] = BG_CTF_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(HORDE, BG_CTF_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_CTF_FLAG_UNK_ALLIANCE, 1);
        player->CastSpell(player, BG_CTF_SPELL_ALLIANCE_FLAG, true);
        player->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_CTF_SPELL_ALLIANCE_FLAG_PICKED);
        if (_flagState[1] == BG_CTF_FLAG_STATE_ON_PLAYER)
          _bothFlagsKept = true;
    }

    //horde flag picked up from base
    if (player->GetTeam() == ALLIANCE && GetFlagState(HORDE) == BG_CTF_FLAG_STATE_ON_BASE
        && BgObjects[BG_CTF_OBJECT_H_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_CTF_PICKEDUP_HF;
        type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
        PlaySoundToAll(BG_CTF_SOUND_HORDE_FLAG_PICKED_UP);
        SpawnBGObject(BG_CTF_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
        SetHordeFlagPicker(player->GetGUID());
        _flagState[BG_TEAM_HORDE] = BG_CTF_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(ALLIANCE, BG_CTF_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_CTF_FLAG_UNK_HORDE, 1);
        player->CastSpell(player, BG_CTF_SPELL_HORDE_FLAG, true);
        player->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_CTF_SPELL_HORDE_FLAG_PICKED);
        if (_flagState[0] == BG_CTF_FLAG_STATE_ON_PLAYER)
          _bothFlagsKept = true;
    }

    //Alliance flag on ground(not in base) (returned or picked up again from ground!)
    if (GetFlagState(ALLIANCE) == BG_CTF_FLAG_STATE_ON_GROUND && player->IsWithinDistInMap(target_obj, 10)
        && target_obj->GetGOInfo()->entry == BG_OBJECT_A_FLAG_GROUND_CTF_ENTRY)
    {
        if (player->GetTeam() == ALLIANCE)
        {
            message_id = LANG_BG_CTF_RETURNED_AF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            UpdateFlagState(HORDE, BG_CTF_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(ALLIANCE, false);
            SpawnBGObject(BG_CTF_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_CTF_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(player, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_CTF_PICKEDUP_AF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_CTF_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SpawnBGObject(BG_CTF_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
            SetAllianceFlagPicker(player->GetGUID());
            player->CastSpell(player, BG_CTF_SPELL_ALLIANCE_FLAG, true);
            _flagState[BG_TEAM_ALLIANCE] = BG_CTF_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(HORDE, BG_CTF_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
              player->CastSpell(player, CTF_SPELL_FOCUSED_ASSAULT, true);
            else if (_flagDebuffState == 2)
              player->CastSpell(player, CTF_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_CTF_FLAG_UNK_ALLIANCE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    //Horde flag on ground(not in base) (returned or picked up again)
    if (GetFlagState(HORDE) == BG_CTF_FLAG_STATE_ON_GROUND && player->IsWithinDistInMap(target_obj, 10)
        && target_obj->GetGOInfo()->entry == BG_OBJECT_H_FLAG_GROUND_CTF_ENTRY)
    {
        if (player->GetTeam() == HORDE)
        {
            message_id = LANG_BG_CTF_RETURNED_HF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            UpdateFlagState(ALLIANCE, BG_CTF_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(HORDE, false);
            SpawnBGObject(BG_CTF_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_CTF_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(player, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_CTF_PICKEDUP_HF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_CTF_SOUND_HORDE_FLAG_PICKED_UP);
            SpawnBGObject(BG_CTF_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
            SetHordeFlagPicker(player->GetGUID());
            player->CastSpell(player, BG_CTF_SPELL_HORDE_FLAG, true);
            _flagState[BG_TEAM_HORDE] = BG_CTF_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(ALLIANCE, BG_CTF_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
              player->CastSpell(player, CTF_SPELL_FOCUSED_ASSAULT, true);
            else if (_flagDebuffState == 2)
              player->CastSpell(player, CTF_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_CTF_FLAG_UNK_HORDE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    if (!message_id)
        return;

    SendMessageToAll(message_id, type, player);
    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundCTF::RemovePlayer(Player* player, uint64 guid, uint32 /*team*/)
{
    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && m_FlagKeepers[BG_TEAM_ALLIANCE] == guid)
    {
        if (!player)
        {
            TC_LOG_ERROR("bg.battleground", "BattlegroundCTF: Removing offline player who has the FLAG!!");
            SetAllianceFlagPicker(0);
            RespawnFlag(ALLIANCE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }
    if (IsHordeFlagPickedup() && m_FlagKeepers[BG_TEAM_HORDE] == guid)
    {
        if (!player)
        {
            TC_LOG_ERROR("bg.battleground", "BattlegroundCTF: Removing offline player who has the FLAG!!");
            SetHordeFlagPicker(0);
            RespawnFlag(HORDE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }
}

void BattlegroundCTF::UpdateFlagState(uint32 team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_CTF_FLAG_STATE_ALLIANCE, value);
    else
        UpdateWorldState(BG_CTF_FLAG_STATE_HORDE, value);
}

void BattlegroundCTF::UpdateTeamScore(uint32 team)
{
    if (team == BG_TEAM_ALLIANCE)
        UpdateWorldState(BG_CTF_FLAG_CAPTURES_ALLIANCE, GetTeamScore(team));
    else
        UpdateWorldState(BG_CTF_FLAG_CAPTURES_HORDE, GetTeamScore(team));
}

void BattlegroundCTF::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch (trigger)
    {
        case 3686:                                          // Alliance elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = BgObjects[BG_CTF_OBJECT_SPEEDBUFF_1];
            break;
        case 3687:                                          // Horde elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = BgObjects[BG_CTF_OBJECT_SPEEDBUFF_2];
            break;
        case 3706:                                          // Alliance elixir of regeneration spawn
            //buff_guid = BgObjects[BG_CTF_OBJECT_REGENBUFF_1];
            break;
        case 3708:                                          // Horde elixir of regeneration spawn
            //buff_guid = BgObjects[BG_CTF_OBJECT_REGENBUFF_2];
            break;
        case 3707:                                          // Alliance elixir of berserk spawn
            //buff_guid = BgObjects[BG_CTF_OBJECT_BERSERKBUFF_1];
            break;
        case 3709:                                          // Horde elixir of berserk spawn
            //buff_guid = BgObjects[BG_CTF_OBJECT_BERSERKBUFF_2];
            break;
        case 3646:                                          // Alliance Flag spawn
            if (_flagState[BG_TEAM_HORDE] && !_flagState[BG_TEAM_ALLIANCE])
                if (GetFlagPickerGUID(BG_TEAM_HORDE) == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 3647:                                          // Horde Flag spawn
            if (_flagState[BG_TEAM_ALLIANCE] && !_flagState[BG_TEAM_HORDE])
                if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 3649:                                          // unk1
        case 3688:                                          // unk2
        case 4628:                                          // unk3
        case 4629:                                          // unk4
            break;
        default:
            Battleground::HandleAreaTrigger(player, trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid, player);
}

bool BattlegroundCTF::SetupBattleground()
{
    // // flags
    // if (!AddObject(BG_CTF_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_CTF_ENTRY, 1540.423f, 1481.325f, 351.8284f, 3.089233f, 0, 0, 0.9996573f, 0.02617699f, BG_CTF_FLAG_RESPAWN_TIME/1000)
    //     || !AddObject(BG_CTF_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_CTF_ENTRY, 916.0226f, 1434.405f, 345.413f, 0.01745329f, 0, 0, 0.008726535f, 0.9999619f, BG_CTF_FLAG_RESPAWN_TIME/1000)
    //     // buffs
    //     || !AddObject(BG_CTF_OBJECT_SPEEDBUFF_1, BG_OBJECTID_SPEEDBUFF_ENTRY, 1449.93f, 1470.71f, 342.6346f, -1.64061f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME)
    //     || !AddObject(BG_CTF_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 1005.171f, 1447.946f, 335.9032f, 1.64061f, 0, 0, 0.7313537f, 0.6819984f, BUFF_RESPAWN_TIME)
    //     || !AddObject(BG_CTF_OBJECT_REGENBUFF_1, BG_OBJECTID_REGENBUFF_ENTRY, 1317.506f, 1550.851f, 313.2344f, -0.2617996f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME)
    //     || !AddObject(BG_CTF_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1110.451f, 1353.656f, 316.5181f, -0.6806787f, 0, 0, 0.333807f, -0.9426414f, BUFF_RESPAWN_TIME)
    //     || !AddObject(BG_CTF_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1320.09f, 1378.79f, 314.7532f, 1.186824f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME)
    //     || !AddObject(BG_CTF_OBJECT_BERSERKBUFF_2, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1139.688f, 1560.288f, 306.8432f, -2.443461f, 0, 0, 0.9396926f, -0.3420201f, BUFF_RESPAWN_TIME)
    //     // alliance gates
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_CTF_ENTRY, 1503.335f, 1493.466f, 352.1888f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_CTF_ENTRY, 1492.478f, 1457.912f, 342.9689f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_CTF_ENTRY, 1468.503f, 1494.357f, 351.8618f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_4, BG_OBJECT_DOOR_A_4_CTF_ENTRY, 1471.555f, 1458.778f, 362.6332f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_5, BG_OBJECT_DOOR_A_5_CTF_ENTRY, 1492.347f, 1458.34f, 342.3712f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_A_6, BG_OBJECT_DOOR_A_6_CTF_ENTRY, 1503.466f, 1493.367f, 351.7352f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
    //     // horde gates
    //     || !AddObject(BG_CTF_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_CTF_ENTRY, 949.1663f, 1423.772f, 345.6241f, -0.5756807f, -0.01673368f, -0.004956111f, -0.2839723f, 0.9586737f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_CTF_ENTRY, 953.0507f, 1459.842f, 340.6526f, -1.99662f, -0.1971825f, 0.1575096f, -0.8239487f, 0.5073641f, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_CTF_ENTRY, 949.9523f, 1422.751f, 344.9273f, 0.0f, 0, 0, 0, 1, RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_CTF_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_CTF_ENTRY, 950.7952f, 1459.583f, 342.1523f, 0.05235988f, 0, 0, 0.02617695f, 0.9996573f, RESPAWN_IMMEDIATELY))
    // 
    // {
    //     TC_LOG_ERROR("sql.sql", "BatteGroundCTF: Failed to spawn some object Battleground not created!");
    //     return false;
    // }
    // 
    // WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_MAIN_ALLIANCE);
    // if (!sg || !AddSpiritGuide(CTF_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    // {
    //     TC_LOG_ERROR("sql.sql", "BatteGroundCTF: Failed to spawn Alliance spirit guide! Battleground not created!");
    //     return false;
    // }
    // 
    // sg = sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_MAIN_HORDE);
    // if (!sg || !AddSpiritGuide(CTF_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    // {
    //     TC_LOG_ERROR("sql.sql", "BatteGroundCTF: Failed to spawn Horde spirit guide! Battleground not created!");
    //     return false;
    // }
    // 
    // TC_LOG_DEBUG("bg.battleground", "BatteGroundCTF: BG objects and spirit guides spawned");

    return true;
}

void BattlegroundCTF::Reset()
{
    //call parent's class reset
    Battleground::Reset();

    m_FlagKeepers[BG_TEAM_ALLIANCE]     = 0;
    m_FlagKeepers[BG_TEAM_HORDE]        = 0;
    m_DroppedFlagGUID[BG_TEAM_ALLIANCE] = 0;
    m_DroppedFlagGUID[BG_TEAM_HORDE]    = 0;
    _flagState[BG_TEAM_ALLIANCE]        = BG_CTF_FLAG_STATE_ON_BASE;
    _flagState[BG_TEAM_HORDE]           = BG_CTF_FLAG_STATE_ON_BASE;
    m_TeamScores[BG_TEAM_ALLIANCE]      = 0;
    m_TeamScores[BG_TEAM_HORDE]         = 0;

    if (sBattlegroundMgr->IsBGWeekend(GetTypeID()))
    {
        m_ReputationCapture = 45;
        m_HonorWinKills = 3;
        m_HonorEndKills = 4;
    }
    else
    {
        m_ReputationCapture = 35;
        m_HonorWinKills = 1;
        m_HonorEndKills = 2;
    }
    _minutesElapsed                  = 0;
    _lastFlagCaptureTeam             = 0;
    _bothFlagsKept                   = false;
    _flagDebuffState                 = 0;
    _flagSpellForceTimer             = 0;
    _flagsDropTimer[BG_TEAM_ALLIANCE]   = 0;
    _flagsDropTimer[BG_TEAM_HORDE]      = 0;
    _flagsTimer[BG_TEAM_ALLIANCE]       = 0;
    _flagsTimer[BG_TEAM_HORDE]          = 0;
}

void BattlegroundCTF::EndBattleground(uint32 winner)
{
    // Win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), HORDE);
    // Complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), HORDE);

    Battleground::EndBattleground(winner);
}

void BattlegroundCTF::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundCTF::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(player->GetGUID());
    if (itr == PlayerScores.end())                         // player not found
        return;

    switch (type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattlegroundCTFScore*)itr->second)->FlagCaptures += value;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, CTF_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattlegroundCTFScore*)itr->second)->FlagReturns += value;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, CTF_OBJECTIVE_RETURN_FLAG);
            break;
        default:
            Battleground::UpdatePlayerScore(player, type, value, doAddHonor);
            break;
    }
}

WorldSafeLocsEntry const* BattlegroundCTF::GetClosestGraveYard(Player* player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_MAIN_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_FLAGROOM_ALLIANCE);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_MAIN_HORDE);
        else
            return sWorldSafeLocsStore.LookupEntry(CTF_GRAVEYARD_FLAGROOM_HORDE);
    }
}

void BattlegroundCTF::FillInitialWorldStates(ByteBuffer& data)
{
    data << uint32(GetTeamScore(BG_TEAM_ALLIANCE)) << uint32(BG_CTF_FLAG_CAPTURES_ALLIANCE);
    data << uint32(GetTeamScore(BG_TEAM_HORDE))    << uint32(BG_CTF_FLAG_CAPTURES_HORDE);

    if (_flagState[BG_TEAM_ALLIANCE] == BG_CTF_FLAG_STATE_ON_GROUND)
        data << uint32(-1) << uint32(BG_CTF_FLAG_UNK_ALLIANCE);
    else if (_flagState[BG_TEAM_ALLIANCE] == BG_CTF_FLAG_STATE_ON_PLAYER)
        data << uint32(1) << uint32(BG_CTF_FLAG_UNK_ALLIANCE);
    else
        data << uint32(0) << uint32(BG_CTF_FLAG_UNK_ALLIANCE);

    if (_flagState[BG_TEAM_HORDE] == BG_CTF_FLAG_STATE_ON_GROUND)
        data << uint32(-1) << uint32(BG_CTF_FLAG_UNK_HORDE);
    else if (_flagState[BG_TEAM_HORDE] == BG_CTF_FLAG_STATE_ON_PLAYER)
        data << uint32(1) << uint32(BG_CTF_FLAG_UNK_HORDE);
    else
        data << uint32(0) << uint32(BG_CTF_FLAG_UNK_HORDE);

    data << uint32(BG_CTF_MAX_TEAM_SCORE) << uint32(BG_CTF_FLAG_CAPTURES_MAX);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        data << uint32(1) << uint32(BG_CTF_STATE_TIMER_ACTIVE);
        data << uint32(25 - _minutesElapsed) << uint32(BG_CTF_STATE_TIMER);
    }
    else
        data << uint32(0) << uint32(BG_CTF_STATE_TIMER_ACTIVE);

    if (_flagState[BG_TEAM_HORDE] == BG_CTF_FLAG_STATE_ON_PLAYER)
        data << uint32(2) << uint32(BG_CTF_FLAG_STATE_HORDE);
    else
        data << uint32(1) << uint32(BG_CTF_FLAG_STATE_HORDE);

    if (_flagState[BG_TEAM_ALLIANCE] == BG_CTF_FLAG_STATE_ON_PLAYER)
        data << uint32(2) << uint32(BG_CTF_FLAG_STATE_ALLIANCE);
    else
        data << uint32(1) << uint32(BG_CTF_FLAG_STATE_ALLIANCE);
}

uint32 BattlegroundCTF::GetPrematureWinner()
{
    if (GetTeamScore(BG_TEAM_ALLIANCE) > GetTeamScore(BG_TEAM_HORDE))
        return ALLIANCE;
    else if (GetTeamScore(BG_TEAM_HORDE) > GetTeamScore(BG_TEAM_ALLIANCE))
        return HORDE;

    return Battleground::GetPrematureWinner();
}

bool BattlegroundCTF::CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* player, Unit const* target, uint32 miscValue)
{
    // switch (criteriaId)
    // {
    //     case BG_CRITERIA_CHECK_SAVE_THE_DAY:
    //         return GetFlagState(player->GetTeam()) == BG_CTF_FLAG_STATE_ON_BASE;
    // }

    return Battleground::CheckAchievementCriteriaMeet(criteriaId, player, target, miscValue);
}
