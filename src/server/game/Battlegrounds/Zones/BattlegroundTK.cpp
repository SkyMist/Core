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

#include "BattlegroundTK.h"

BattlegroundTK::BattlegroundTK()
{
    BgObjects.resize(BG_TK_OBJECT_MAX);
    BgCreatures.resize(BG_TK_CREATURE_MAX);

    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_TK_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_TK_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_TK_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_TK_HAS_BEGUN;

    _UpdatePointsTimer   = 0;
    _LastCapturedOrbTeam = BG_TEAM_ALLIANCE;
}

BattlegroundTK::~BattlegroundTK() { }

void BattlegroundTK::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (GetElapsedTime() >= BG_TK_TIME_LIMIT + 2 * MINUTE * IN_MILLISECONDS)
        {
            if (GetTeamScore(ALLIANCE) == 0)
            {
                if (GetTeamScore(HORDE) == 0)        // No one scored - result is tie
                    EndBattleground(WINNER_NONE);
                else                                         // Horde has more points and thus wins
                    EndBattleground(HORDE);
            }
            else if (GetTeamScore(HORDE) == 0)
                EndBattleground(ALLIANCE);           // Alliance has > 0, Horde has 0, alliance wins
            else if (GetTeamScore(HORDE) == GetTeamScore(ALLIANCE)) // Team score equal, winner is team that scored the last flag
                EndBattleground(_LastCapturedOrbTeam);
            else if (GetTeamScore(HORDE) > GetTeamScore(ALLIANCE))  // Last but not least, check who has the higher score
                EndBattleground(HORDE);
            else
                EndBattleground(ALLIANCE);
        }
        else if (GetElapsedTime() > uint32(_minutesElapsed * MINUTE * IN_MILLISECONDS) +  3 * MINUTE * IN_MILLISECONDS) // first update needed after 1 minute of game already in progress
        {
            ++_minutesElapsed;
            UpdateWorldState(BG_TK_TIME_REMAINING, 25 - _minutesElapsed);
        }

        if (_UpdatePointsTimer <= diff)
        {
            for (uint8 i = 0; i < MAX_ORBS; ++i)
            {
                if (uint64 guid = m_OrbKeepers[i])
                {
                    if (m_playersZone.find(guid) != m_playersZone.end())
                    {
                        if (Player* player = ObjectAccessor::FindPlayer(guid))
                        {
                            AccumulateScore(player->GetTeamId(), m_playersZone[guid]);
                            UpdatePlayerScore(player, SCORE_ORB_SCORE, BG_TK_TickPoints[m_playersZone[guid]]);
                        }
                    }
                }
            }

            _UpdatePointsTimer = BG_TK_POINTS_UPDATE_TIME;
        }
        else
            _UpdatePointsTimer -= diff;
    }
}

void BattlegroundTK::StartingEventCloseDoors()
{
    for (uint32 i = BG_TK_OBJECT_A_DOOR; i <= BG_TK_OBJECT_H_DOOR; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
}

void BattlegroundTK::StartingEventOpenDoors()
{
    for (uint32 i = BG_TK_OBJECT_A_DOOR; i <= BG_TK_OBJECT_H_DOOR; ++i)
        DoorOpen(i);

    for (uint8 i = BG_TK_OBJECT_ORB_1; i < BG_TK_OBJECT_ORB_4; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundTK::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in constructor
    BattlegroundTKScore* sc = new BattlegroundTKScore;

    PlayerScores[player->GetGUID()] = sc;
    m_playersZone[player->GetGUID()] = TK_ZONE_OUT;
}

void BattlegroundTK::EventPlayerClickedOnOrb(Player* player, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!player->IsWithinDistInMap(target_obj, 10))
        return;

    uint32 index = target_obj->GetEntry() - BG_TK_OBJECT_ORB_1_ENTRY;

    // If this orb is already keeped by a player, there is a problem
    if (index >= MAX_ORBS || m_OrbKeepers[index] != 0)
        return;

    // Check if the player already have an orb
    for (uint8 i = 0; i < MAX_ORBS; ++i)
        if (m_OrbKeepers[i] == player->GetGUID())
            return;

    PlaySoundToAll(player->GetTeamId() == BG_TEAM_ALLIANCE ? BG_TK_SOUND_A_ORB_PICKED_UP: BG_TK_SOUND_H_ORB_PICKED_UP);
    player->CastSpell(player, BG_TK_ORBS_SPELLS[index], true);
    player->CastSpell(player, player->GetTeamId() == BG_TEAM_ALLIANCE ? BG_TK_ALLIANCE_INSIGNIA: BG_TK_HORDE_INSIGNIA, true);

    UpdatePlayerScore(player, SCORE_ORB_HANDLES, 1);

    m_OrbKeepers[index] = player->GetGUID();
    UpdateOrbState(ALLIANCE, 1);

    if (Creature* aura = GetBGCreature(BG_TK_CREATURE_ORB_AURA_1 + index))
        aura->RemoveAllAuras();

    SendMessageToAll(LANG_BG_TK_ORB_PICKED_UP, player->GetTeamId() == BG_TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE: CHAT_MSG_BG_SYSTEM_HORDE, player);
    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundTK::EventPlayerDroppedOrb(Player* player)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 index = 0;

    for (; index <= MAX_ORBS; ++index)
    {
        if (index == MAX_ORBS)
            return;

        if (m_OrbKeepers[index] == player->GetGUID())
            break;
    }

    PlaySoundToAll(player->GetTeamId() == BG_TEAM_ALLIANCE ? BG_TK_SOUND_A_ORB_PICKED_UP: BG_TK_SOUND_H_ORB_PICKED_UP);
    player->RemoveAurasDueToSpell(BG_TK_ORBS_SPELLS[index]);
    player->RemoveAurasDueToSpell(BG_TK_ALLIANCE_INSIGNIA);
    player->RemoveAurasDueToSpell(BG_TK_HORDE_INSIGNIA);

    m_OrbKeepers[index] = 0;
    SpawnBGObject(BG_TK_OBJECT_ORB_1 + index, RESPAWN_IMMEDIATELY);

    if (Creature* aura = GetBGCreature(BG_TK_CREATURE_ORB_AURA_1 + index))
        aura->AddAura(BG_TK_ORBS_AURA[index], aura);

    UpdateOrbState(ALLIANCE, 0);

    SendMessageToAll(LANG_BG_TK_ORB_DROPPED, player->GetTeamId() == BG_TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE: CHAT_MSG_BG_SYSTEM_HORDE, player);
    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundTK::RemovePlayer(Player* player, uint64 guid, uint32 /*team*/)
{
    EventPlayerDroppedOrb(player);
    m_playersZone.erase(player->GetGUID());
}

void BattlegroundTK::UpdateOrbState(uint32 team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_TK_ICON_A, value);
    else
        UpdateWorldState(BG_TK_ICON_H, value);
}

void BattlegroundTK::UpdateTeamScore(uint32 team)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_TK_ORB_POINTS_A, GetTeamScore(team));
    else
        UpdateWorldState(BG_TK_ORB_POINTS_H, GetTeamScore(team));
}

void BattlegroundTK::HandleAreaTrigger(Player* player, uint32 trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint64 sourceGuid = player->GetGUID();
    switch(trigger)
    {
        case 7734: // Out-In trigger
            if (m_playersZone.find(sourceGuid) == m_playersZone.end())
                return;
            if (m_playersZone[sourceGuid] == TK_ZONE_OUT)
                m_playersZone[sourceGuid] = TK_ZONE_IN;
            else
                m_playersZone[sourceGuid] = TK_ZONE_OUT;
            break;
        case 7735: // Middle-In trigger
            if (m_playersZone.find(sourceGuid) == m_playersZone.end())
                return;
            if (m_playersZone[sourceGuid] == TK_ZONE_IN)
                m_playersZone[sourceGuid] = TK_ZONE_MIDDLE;
            else
                m_playersZone[sourceGuid] = TK_ZONE_IN;
            break;
        default:
            Battleground::HandleAreaTrigger(player, trigger);
            break;
    }
}

bool BattlegroundTK::SetupBattleground()
{
    // Doors
    if (   !AddObject(BG_TK_OBJECT_A_DOOR, BG_TK_OBJECT_DOOR_ENTRY, BG_TK_DoorPositions[0][0], BG_TK_DoorPositions[0][1], BG_TK_DoorPositions[0][2], BG_TK_DoorPositions[0][3], 0, 0, sin(BG_TK_DoorPositions[0][3]/2), cos(BG_TK_DoorPositions[0][3]/2), RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TK_OBJECT_H_DOOR, BG_TK_OBJECT_DOOR_ENTRY, BG_TK_DoorPositions[1][0], BG_TK_DoorPositions[1][1], BG_TK_DoorPositions[1][2], BG_TK_DoorPositions[1][3], 0, 0, sin(BG_TK_DoorPositions[1][3]/2), cos(BG_TK_DoorPositions[1][3]/2), RESPAWN_IMMEDIATELY))
        return false;

    if (   !AddSpiritGuide(BG_TK_CREATURE_SPIRIT_1, BG_TK_SpiritPositions[0][0], BG_TK_SpiritPositions[0][1], BG_TK_SpiritPositions[0][2], BG_TK_SpiritPositions[0][3], ALLIANCE)
        || !AddSpiritGuide(BG_TK_CREATURE_SPIRIT_2, BG_TK_SpiritPositions[1][0], BG_TK_SpiritPositions[1][1], BG_TK_SpiritPositions[1][2], BG_TK_SpiritPositions[1][3], HORDE))
        return false;

    // Orbs
    for (uint8 i = 0; i < MAX_ORBS; ++i)
    {
        if (!AddObject(BG_TK_OBJECT_ORB_1 + i, BG_TK_OBJECT_ORB_1_ENTRY + i, BG_TK_OrbPositions[i][0], BG_TK_OrbPositions[i][1], BG_TK_OrbPositions[i][2], BG_TK_OrbPositions[i][3], 0, 0, sin(BG_TK_OrbPositions[i][3]/2), cos(BG_TK_OrbPositions[i][3]/2), RESPAWN_ONE_DAY))
            return false;

        if (Creature* trigger = AddCreature(WORLD_TRIGGER, BG_TK_CREATURE_ORB_AURA_1 + i, TEAM_NEUTRAL, BG_TK_OrbPositions[i][0], BG_TK_OrbPositions[i][1], BG_TK_OrbPositions[i][2], BG_TK_OrbPositions[i][3], RESPAWN_IMMEDIATELY))
            trigger->AddAura(BG_TK_ORBS_AURA[i], trigger);
    }

    return true;
}

void BattlegroundTK::Reset()
{
    // call parent's class reset
    Battleground::Reset();

    m_TeamScores[BG_TEAM_ALLIANCE]      = 0;
    m_TeamScores[BG_TEAM_HORDE]         = 0;

    m_playersZone.clear();

    for (uint32 i = 0; i < MAX_ORBS; ++i)
        m_OrbKeepers[i]  = 0;

    bool isBGWeekend = BattlegroundMgr::IsBGWeekend(GetTypeID());
    m_ReputationCapture  = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills      = (isBGWeekend) ?  3 :  1;
    m_HonorEndKills      = (isBGWeekend) ?  4 :  2;

    _UpdatePointsTimer   = BG_TK_POINTS_UPDATE_TIME;
    _LastCapturedOrbTeam = BG_TEAM_ALLIANCE;

    _minutesElapsed      = 0;
}

void BattlegroundTK::EndBattleground(uint32 winner)
{
    //win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), HORDE);
    //complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), HORDE);

    Battleground::EndBattleground(winner);
}

void BattlegroundTK::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedOrb(player);

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundTK::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(player->GetGUID());
    if (itr == PlayerScores.end())                         // player not found
        return;

    switch(type)
    {
        case SCORE_ORB_HANDLES:                           // orb handles
            ((BattlegroundTKScore*)itr->second)->OrbHandles += value;
            break;
        case SCORE_ORB_SCORE:
            ((BattlegroundTKScore*)itr->second)->VictoryPoints += value;
            break;
        default:
            Battleground::UpdatePlayerScore(player, type, value, doAddHonor);
            break;
    }
}

WorldSafeLocsEntry const* BattlegroundTK::GetClosestGraveYard(Player* player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(TK_GRAVEYARD_RECTANGLEA1);
        else
            return sWorldSafeLocsStore.LookupEntry(TK_GRAVEYARD_RECTANGLEA2);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(TK_GRAVEYARD_RECTANGLEH1);
        else
            return sWorldSafeLocsStore.LookupEntry(TK_GRAVEYARD_RECTANGLEH2);
    }
}

void BattlegroundTK::AccumulateScore(uint32 team, BG_TK_ZONE zone)
{
    if (zone > TK_ZONE_MAX)
        return;

    if (team >= TEAM_NEUTRAL)
        return;

    m_TeamScores[team] += BG_TK_TickPoints[zone];

    if (m_TeamScores[team] > BG_TK_MAX_TEAM_SCORE)
        m_TeamScores[team] = BG_TK_MAX_TEAM_SCORE;

    if (team == BG_TEAM_ALLIANCE)
        UpdateTeamScore(ALLIANCE);
    if (team == BG_TEAM_HORDE)
        UpdateTeamScore(HORDE);

    // Test win condition
    if (GetTeamScore(ALLIANCE) >= BG_TK_MAX_TEAM_SCORE)
        EndBattleground(ALLIANCE);
    if (GetTeamScore(BG_TEAM_HORDE) >= BG_TK_MAX_TEAM_SCORE)
        EndBattleground(HORDE);
}

void BattlegroundTK::FillInitialWorldStates(ByteBuffer& data)
{
    data << uint32(GetTeamScore(ALLIANCE)) << uint32(BG_TK_ORB_POINTS_A);
    data << uint32(GetTeamScore(HORDE))    << uint32(BG_TK_ORB_POINTS_H);
    data << uint32(0x1)                    << uint32(BG_KT_NEUTRAL_ORBS);

    data << uint32(0x1)                    << uint32(BG_TK_TIME_ENABLED);
    data << uint32(25 - _minutesElapsed)   << uint32(BG_TK_TIME_REMAINING);

    /*if (m_OrbState[BG_TEAM_ALLIANCE] == BG_TK_ORB_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_TK_ICON_A, -1);
    else if (m_OrbState[BG_TEAM_ALLIANCE] == BG_TK_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TK_ICON_A, 1);
    else
        FillInitialWorldState(data, count, BG_TK_ICON_A, 0);

    if (m_OrbState[BG_TEAM_HORDE] == BG_TK_ORB_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_TK_ICON_H, -1);
    else if (m_OrbState[BG_TEAM_HORDE] == BG_TK_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TK_ICON_H, 1);
    else
        FillInitialWorldState(data, count, BG_TK_ICON_H, 0);*/

    /*if (m_OrbState[BG_TEAM_HORDE] == BG_TK_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TK_ORB_STATE, 2);
    else
        FillInitialWorldState(data, count, BG_TK_ORB_STATE, 1);

    if (m_OrbState[BG_TEAM_ALLIANCE] == BG_TK_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TK_ORB_STATE, 2);
    else
        FillInitialWorldState(data, count, BG_TK_ORB_STATE, 1);*/
}
