/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "Object.h"
#include "Player.h"
#include "Battleground.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "WorldPacket.h"
#include "Language.h"
#include "MapManager.h"

#include "BattlegroundTK.h"

BattlegroundTK::BattlegroundTK()
{
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_TK_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_TK_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_TK_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_TK_HAS_BEGUN;

    m_ReputationCapture = 0;
    m_HonorWinKills     = 0;
    m_HonorEndKills     = 0;

    m_EndTimer          = 0;

    m_UpdatePointsTimer   = BG_TK_POINTS_UPDATE_TIME;
    m_LastCapturedOrbTeam = BG_TEAM_ALLIANCE;
}

BattlegroundTK::~BattlegroundTK() { }

void BattlegroundTK::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (m_EndTimer <= diff)
        {
            uint32 allianceScore = GetTeamScore(ALLIANCE);
            uint32 hordeScore    = GetTeamScore(HORDE);

            if (allianceScore > hordeScore)
                EndBattleground(ALLIANCE);
            else if (allianceScore < hordeScore)
                EndBattleground(HORDE);
            else
            {
                // if 0 => tie
                EndBattleground(m_LastCapturedOrbTeam);
            }
        }
        else
        {
            uint32 minutesLeftPrev = GetRemainingTimeInMinutes();
            m_EndTimer -= diff;
            uint32 minutesLeft = GetRemainingTimeInMinutes();

            if (minutesLeft != minutesLeftPrev)
                UpdateWorldState(BG_TK_TIME_REMAINING, minutesLeft);
        }

        if (m_UpdatePointsTimer <= diff)
        {
            for (uint8 i = 0; i < MAX_ORBS; ++i)
                if (uint64 guid = m_OrbKeepers[i])
                    if (m_playersZone.find(guid) != m_playersZone.end())
                        if (Player* player = ObjectAccessor::FindPlayer(guid))
                        {
                            AccumulateScore(player->GetTeamId(), m_playersZone[guid]);
                            UpdatePlayerScore(player, SCORE_ORB_SCORE, m_playersZone[guid]);
                        }

            m_UpdatePointsTimer = BG_TK_POINTS_UPDATE_TIME;
        }
        else
            m_UpdatePointsTimer -= diff;
    }
}

void BattlegroundTK::StartingEventCloseDoors()
{
    SpawnBGObject(BG_TK_OBJECT_A_DOOR, RESPAWN_IMMEDIATELY);
    SpawnBGObject(BG_TK_OBJECT_H_DOOR, RESPAWN_IMMEDIATELY);

    DoorClose(BG_TK_OBJECT_A_DOOR);
    DoorClose(BG_TK_OBJECT_H_DOOR);

    for (uint8 i = 0; i < 4; ++i)
        SpawnBGObject(BG_TK_OBJECT_ORB_1 + i, RESPAWN_ONE_DAY);
}

void BattlegroundTK::StartingEventOpenDoors()
{
    DoorOpen(BG_TK_OBJECT_A_DOOR);
    DoorOpen(BG_TK_OBJECT_H_DOOR);

    for (uint8 i = 0; i < 4; ++i)
        SpawnBGObject(BG_TK_OBJECT_ORB_1 + i, RESPAWN_IMMEDIATELY);

    // Players that join battleground after start are not eligible to get achievement.
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, BG_TK_EVENT_START_BATTLE);
}

void BattlegroundTK::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in constructor
    BattlegroundTKScore* sc = new BattlegroundTKScore;

    PlayerScores[player->GetGUID()] = sc;
    m_playersZone[player->GetGUID()] = TK_ZONE_OUT;
}

void BattlegroundTK::EventPlayerClickedOnOrb(Player* source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!source->IsWithinDistInMap(target_obj, 10))
        return;

    uint32 index = target_obj->GetEntry() - BG_TK_OBJECT_ORB_1_ENTRY;

    // If this orb is already keeped by a player, there is a problem
    if (index >= MAX_ORBS || m_OrbKeepers[index] != 0)
        return;

    // Check if the player already have an orb
    for (uint8 i = 0; i < MAX_ORBS; ++i)
        if (m_OrbKeepers[i] == source->GetGUID())
            return;

    PlaySoundToAll(source->GetTeamId() == TEAM_ALLIANCE ? BG_TK_SOUND_A_ORB_PICKED_UP: BG_TK_SOUND_H_ORB_PICKED_UP);
    source->CastSpell(source, BG_TK_ORBS_SPELLS[index], true);
    source->CastSpell(source, source->GetTeamId() == TEAM_ALLIANCE ? BG_TK_ALLIANCE_INSIGNIA: BG_TK_HORDE_INSIGNIA, true);

    UpdatePlayerScore(source, SCORE_ORB_HANDLES, 1);

    m_OrbKeepers[index] = source->GetGUID();
    UpdateWorldState(BG_TK_ICON_A, 1);
    SpawnBGObject(BG_TK_OBJECT_ORB_1 + index, RESPAWN_ONE_DAY);

    if (Creature* aura = GetBGCreature(BG_TK_CREATURE_ORB_AURA_1 + index))
        aura->RemoveAllAuras();

    SendMessageToAll(LANG_BG_TK_ORB_PICKED_UP, source->GetTeamId() == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE: CHAT_MSG_BG_SYSTEM_HORDE, source);
    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundTK::EventPlayerDroppedOrb(Player* source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 index = 0;

    for (; index <= MAX_ORBS; ++index)
    {
        if (index == MAX_ORBS)
            return;

        if (m_OrbKeepers[index] == source->GetGUID())
            break;
    }

    PlaySoundToAll(source->GetTeamId() == TEAM_ALLIANCE ? BG_TK_SOUND_A_ORB_PICKED_UP: BG_TK_SOUND_H_ORB_PICKED_UP);
    source->RemoveAurasDueToSpell(BG_TK_ORBS_SPELLS[index]);
    source->RemoveAurasDueToSpell(BG_TK_ALLIANCE_INSIGNIA);
    source->RemoveAurasDueToSpell(BG_TK_HORDE_INSIGNIA);

    m_OrbKeepers[index] = 0;
    SpawnBGObject(BG_TK_OBJECT_ORB_1 + index, RESPAWN_IMMEDIATELY);

    if (Creature* aura = GetBGCreature(BG_TK_CREATURE_ORB_AURA_1 + index))
        aura->AddAura(BG_TK_ORBS_AURA[index], aura);

    UpdateWorldState(BG_TK_ICON_A, 0);
    SendMessageToAll(LANG_BG_TK_ORB_DROPPED, source->GetTeamId() == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE: CHAT_MSG_BG_SYSTEM_HORDE, source);
    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundTK::RemovePlayer(Player* plr, uint64 guid, uint32 /*team*/)
{
    EventPlayerDroppedOrb(plr);
    m_playersZone.erase(plr->GetGUID());
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

void BattlegroundTK::HandleAreaTrigger(Player* source, uint32 trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint64 sourceGuid = source->GetGUID();
    switch(trigger)
    {
        case 7734: // Out-In trigger
        {
            if (m_playersZone.find(sourceGuid) == m_playersZone.end())
                return;

            if (m_playersZone[sourceGuid] == TK_ZONE_OUT)
                m_playersZone[sourceGuid] = TK_ZONE_IN;
            else
                m_playersZone[sourceGuid] = TK_ZONE_OUT;
            break;
        }
        case 7735: // Middle-In trigger
        {
            if (m_playersZone.find(sourceGuid) == m_playersZone.end())
                return;

            if (m_playersZone[sourceGuid] == TK_ZONE_IN)
                m_playersZone[sourceGuid] = TK_ZONE_MIDDLE;
            else
                m_playersZone[sourceGuid] = TK_ZONE_IN;
            break;
        }
        default:
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
    //call parent's class reset
    Battleground::Reset();
    BgObjects.resize(BG_TK_OBJECT_MAX);
    BgCreatures.resize(BG_TK_CREATURE_MAX);

    for (uint32 i = 0; i < MAX_ORBS; ++i)
        m_OrbKeepers[i] = 0;

    bool isBGWeekend = BattlegroundMgr::IsBGWeekend(GetTypeID());
    m_ReputationCapture = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills = (isBGWeekend) ? 3 : 1;
    m_HonorEndKills = (isBGWeekend) ? 4 : 2;

    m_EndTimer = BG_TK_TIME_LIMIT;
    m_LastCapturedOrbTeam = BG_TEAM_ALLIANCE;
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

void BattlegroundTK::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUID());
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
            Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);
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
        UpdateWorldState(BG_TK_ORB_POINTS_A, m_TeamScores[team]);
    if (team == BG_TEAM_HORDE)
        UpdateWorldState(BG_TK_ORB_POINTS_H, m_TeamScores[team]);

    // Test win condition
    if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_TK_MAX_TEAM_SCORE)
        EndBattleground(ALLIANCE);
    if (m_TeamScores[BG_TEAM_HORDE] >= BG_TK_MAX_TEAM_SCORE)
        EndBattleground(HORDE);
}

void BattlegroundTK::FillInitialWorldStates(ByteBuffer& data)
{
    data << uint32(GetTeamScore(ALLIANCE)) << uint32(BG_TK_ORB_POINTS_A);
    data << uint32(GetTeamScore(HORDE))    << uint32(BG_TK_ORB_POINTS_H);
    data << uint32(BG_TK_MAX_TEAM_SCORE)   << uint32(BG_TK_ORB_POINTS_MAX);
    data << uint32(0x1)                    << uint32(BG_KT_NEUTRAL_ORBS);

    data << uint32(0x1)                         << uint32(BG_TK_TIME_ENABLED);
    data << uint32(GetRemainingTimeInMinutes()) << uint32(BG_TK_TIME_REMAINING);

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
