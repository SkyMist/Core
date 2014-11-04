#include "Battleground.h"
#include "BattlegroundDG.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

BattlegroundDG::BattlegroundDG()
{
    BgObjects.resize(BG_DG_OBJECT_MAX);
    BgCreatures.resize(BG_DG_ALL_NODES_COUNT + 3); // +3 for aura triggers

    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_DG_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_DG_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_DG_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_DG_HAS_BEGUN;
}

BattlegroundDG::~BattlegroundDG()
{
}

WorldSafeLocsEntry const* BattlegroundDG::GetClosestGraveYard(Player* player)
{
    BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(player->GetBGTeam());

    // Is there any occupied node for this team?
    std::vector<uint8> nodes;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if (m_Nodes[i] == teamIndex + 3)
            nodes.push_back(i);

    WorldSafeLocsEntry const* good_entry = NULL;

    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float player_x = player->GetPositionX();
        float player_y = player->GetPositionY();

        float mindist = 999999.0f; // Temp Hack
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[nodes[i]]);

            if (!entry)
                continue;

            float dist = (entry->x - player_x)*(entry->x - player_x)+(entry->y - player_y)*(entry->y - player_y);

            if (mindist > dist)
            {
                mindist = dist;
                good_entry = entry;
            }
        }
        nodes.clear();
    }

    // If not, place ghost on starting location
    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[teamIndex+3]);

    return good_entry;
}

void BattlegroundDG::StartingEventCloseDoors()
{
    // Remove banners, auras and buffs
    for (int object = BG_DG_OBJECT_BANNER_NEUTRAL; object < BG_DG_DYNAMIC_NODES_COUNT * 8; ++object)
        SpawnBGObject(object, RESPAWN_ONE_DAY);

    // Starting doors
    DoorClose(BG_DG_OBJECT_DOOR_H1);
    DoorClose(BG_DG_OBJECT_DOOR_A1);
    DoorClose(BG_DG_OBJECT_DOOR_H2);
    DoorClose(BG_DG_OBJECT_DOOR_A2);

    // Starting base spirit guides
    NodeOccupied(BG_DG_SPIRIT_ALIANCE, ALLIANCE);
    NodeOccupied(BG_DG_SPIRIT_HORDE, HORDE);
}

void BattlegroundDG::StartingEventOpenDoors()
{
    // spawn neutral banners
    for (int banner = BG_DG_OBJECT_BANNER_NEUTRAL, i = 0; i < 3; banner += 8, ++i)
        SpawnBGObject(banner, RESPAWN_IMMEDIATELY);

    DoorOpen(BG_DG_OBJECT_DOOR_H1);
    DoorOpen(BG_DG_OBJECT_DOOR_A1);
    DoorOpen(BG_DG_OBJECT_DOOR_H2);
    DoorOpen(BG_DG_OBJECT_DOOR_A2);

    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, BG_EVENT_START_BATTLE);
}

void BattlegroundDG::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        int team_points[BG_TEAMS_COUNT] = { 0, 0 };

        for (int node = 0; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
        {
            // 3 sec delay to spawn new banner instead previous despawned one
            if (m_BannerTimers[node].timer)
            {
                if (m_BannerTimers[node].timer > diff)
                    m_BannerTimers[node].timer -= diff;
                else
                {
                    m_BannerTimers[node].timer = 0;
                    CreateBanner(node, m_BannerTimers[node].type, m_BannerTimers[node].teamIndex, false);
                }
            }

            // 1-minute to occupy a node from contested state
            if (m_NodeTimers[node])
            {
                if (m_NodeTimers[node] > diff)
                    m_NodeTimers[node] -= diff;
                else
                {
                    m_NodeTimers[node] = 0;
                    // Change from contested to occupied !
                    uint8 teamIndex = m_Nodes[node] - 1;
                    m_prevNodes[node] = m_Nodes[node];
                    m_Nodes[node] += 2;
                    // burn current contested banner
                    DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex);
                    // create new occupied banner
                    CreateBanner(node, BG_DG_NODE_TYPE_OCCUPIED, teamIndex, true);
                    SendNodeUpdate(node);
                    NodeOccupied(node, (teamIndex == 0) ? ALLIANCE:HORDE);

                    // Message to chatlog
                    if (teamIndex == 0)
                    {
                        // FIXME: need to fix Locales for team and nodes names.
                        SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_DG_ALLY, _GetNodeNameId(node));
                        PlaySoundToAll(BG_DG_SOUND_NODE_CAPTURED_ALLIANCE);
                     }
                     else
                     {
                        // FIXME: team and node names not localized
                        SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_DG_HORDE, _GetNodeNameId(node));
                        PlaySoundToAll(BG_DG_SOUND_NODE_CAPTURED_HORDE);
                    }
                }
            }

            for (int team = 0; team < BG_TEAMS_COUNT; ++team)
                if (m_Nodes[node] == team + BG_DG_NODE_TYPE_OCCUPIED)
                    ++team_points[team];
        }

        // Accumulate points
        for (int team = 0; team < BG_TEAMS_COUNT; ++team)
        {
            int points = team_points[team];
            if (!points)
                continue;

            m_lastTick[team] += diff;

            if (m_lastTick[team] > BG_DG_TickIntervals[points])
            {
                m_lastTick[team] -= BG_DG_TickIntervals[points];
                m_TeamScores[team] += BG_DG_TickPoints[points];
                m_HonorScoreTics[team] += BG_DG_TickPoints[points];
                m_ReputationScoreTics[team] += BG_DG_TickPoints[points];

                if (m_ReputationScoreTics[team] >= m_ReputationTics)
                {
                    //(team == BG_TEAM_ALLIANCE) ? RewardReputationToTeam(509, 10, ALLIANCE) : RewardReputationToTeam(510, 10, HORDE);
                    m_ReputationScoreTics[team] -= m_ReputationTics;
                }

                if (m_HonorScoreTics[team] >= m_HonorTics)
                {
                    RewardHonorToTeam(GetBonusHonorFromKill(1), (team == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);
                    m_HonorScoreTics[team] -= m_HonorTics;
                }

                if (!m_IsInformedNearVictory && m_TeamScores[team] > BG_DG_WARNING_NEAR_VICTORY_SCORE)
                {
                    if (team == BG_TEAM_ALLIANCE)
                        SendMessageToAll(LANG_BG_DG_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    else
                        SendMessageToAll(LANG_BG_DG_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);

                    PlaySoundToAll(BG_DG_SOUND_NEAR_VICTORY);

                    m_IsInformedNearVictory = true;
                }

                if (m_TeamScores[team] > BG_DG_MAX_TEAM_SCORE)
                    m_TeamScores[team] = BG_DG_MAX_TEAM_SCORE;

                if (team == BG_TEAM_ALLIANCE)
                    UpdateWorldState(BG_DG_OP_RESOURCES_ALLY, m_TeamScores[team]);

                if (team == BG_TEAM_HORDE)
                    UpdateWorldState(BG_DG_OP_RESOURCES_HORDE, m_TeamScores[team]);
                // update achievement flags
                // we increased m_TeamScores[team] so we just need to check if it is 500 more than other teams resources
                uint8 otherTeam = (team + 1) % BG_TEAMS_COUNT;
                if (m_TeamScores[team] > m_TeamScores[otherTeam] + 500)
                    m_TeamScores500Disadvantage[otherTeam] = true;
            }
        }

        // Test win condition
        if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_DG_MAX_TEAM_SCORE)
            EndBattleground(ALLIANCE);
        if (m_TeamScores[BG_TEAM_HORDE] >= BG_DG_MAX_TEAM_SCORE)
            EndBattleground(HORDE);

        if (m_flagState[BG_TEAM_ALLIANCE] == BG_DG_FLAG_STATE_WAIT_RESPAWN)
        {
            m_flagsTimer[BG_TEAM_ALLIANCE] -= diff;

            if (m_flagsTimer[BG_TEAM_ALLIANCE] < 0)
            {
                m_flagsTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlag(ALLIANCE, true);
            }
        }
        if (m_flagState[BG_TEAM_ALLIANCE] == BG_DG_FLAG_STATE_ON_GROUND)
        {
            m_flagsDropTimer[BG_TEAM_ALLIANCE] -= diff;

            if (m_flagsDropTimer[BG_TEAM_ALLIANCE] < 0)
            {
                m_flagsDropTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlagAfterDrop(ALLIANCE);
            }
        }
        if (m_flagState[BG_TEAM_HORDE] == BG_DG_FLAG_STATE_WAIT_RESPAWN)
        {
            m_flagsTimer[BG_TEAM_HORDE] -= diff;

            if (m_flagsTimer[BG_TEAM_HORDE] < 0)
            {
                m_flagsTimer[BG_TEAM_HORDE] = 0;
                RespawnFlag(HORDE, true);
            }
        }
        if (m_flagState[BG_TEAM_HORDE] == BG_DG_FLAG_STATE_ON_GROUND)
        {
            m_flagsDropTimer[BG_TEAM_HORDE] -= diff;

            if (m_flagsDropTimer[BG_TEAM_HORDE] < 0)
            {
                m_flagsDropTimer[BG_TEAM_HORDE] = 0;
                RespawnFlagAfterDrop(HORDE);
            }
        }
    }

    if (GetStatus() == STATUS_WAIT_JOIN)
    {
        m_CheatersCheckTimer -= diff;
        if (m_CheatersCheckTimer <= 0)
        {
            for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
            {
                Player * plr = ObjectAccessor::FindPlayer(itr->first);
                if (!plr || !plr->IsInWorld())
                    continue;
                if (plr->GetPositionZ() < 125)
                {
                    if (plr->GetBGTeam() == HORDE)
                        plr->TeleportTo(1105, -87.893f, 803.736f, 133.095f, plr->GetOrientation(), 0);
                    else
                        plr->TeleportTo(1105, -250.580f, 198.510f, 133.114f, plr->GetOrientation(), 0);
                }
            }
            m_CheatersCheckTimer = 4000;
        }
    }
}

bool BattlegroundDG::SetupBattleground()
{
    for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        if (!AddObject(BG_DG_OBJECT_BANNER_NEUTRAL + 8*i, BG_OBJECT_NODE_BANNER_0_ENTRY + i, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_BANNER_CONT_A + 8*i, BG_OBJECT_BANNER_CONT_A_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_BANNER_CONT_H + 8*i, BG_OBJECT_BANNER_CONT_H_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_BANNER_ALLY + 8*i, BG_OBJECT_BANNER_A_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_BANNER_HORDE + 8*i, BG_OBJECT_BANNER_H_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_AURA_ALLY + 8*i, BG_OBJECT_AURA_A_DG_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_AURA_HORDE + 8*i, BG_OBJECT_AURA_H_DG_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_DG_OBJECT_AURA_CONTESTED + 8*i, BG_OBJECT_AURA_C_DG_ENTRY, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
        )
        {
            sLog->outError(LOG_FILTER_SQL, "BatteGroundDG: Failed to spawn some object Battleground not created!");
            return false;
        }
    }

    // Flags
    if (!AddObject(BG_DG_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_DG_ENTRY, -241.7413f, 208.6111f, 133.7474f, 0.8427795f, 0.0f, 0.0f, 0.409029f, 0.9125214f, BG_DG_FLAG_RESPAWN_TIME / 1000)
        || !AddObject(BG_DG_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_DG_ENTRY, -91.61632f, 791.3611f, 133.7473f, 4.023561f, 0.0f, 0.0f, 1.095668f, -0.4477588f, BG_DG_FLAG_RESPAWN_TIME / 1000)
    // Doors
        || !AddObject(BG_DG_OBJECT_DOOR_H1, BG_OBJECT_DOOR_H1_DG_ENTRY, -119.6215f, 798.9566f, 132.4883f, 0.8203033f, 0.0f, 0.0f, 0.3987484f, 0.9170604f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DG_OBJECT_DOOR_A1, BG_OBJECT_DOOR_A1_DG_ENTRY, -263.154327f, 217.810318f, 132.268402f, 4.601672f, 0.0f, 0.0f, 0.7132502f, 0.7009096f, RESPAWN_IMMEDIATELY) // Not in sniff
        || !AddObject(BG_DG_OBJECT_DOOR_H1, BG_OBJECT_DOOR_H1_DG_ENTRY, -69.87847f, 781.8368f, 132.4303f, 1.588249f, 0.0f, 0.0f, 0.7132502f, 0.7009096f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DG_OBJECT_DOOR_A2, BG_OBJECT_DOOR_A2_DG_ENTRY, -214.056488f, 199.819458f, 132.554367f, 3.961536f, 0.0f, 0.0f, 0.3987484f, 0.9170604f, RESPAWN_IMMEDIATELY)// Not in sniff
    )
    {
        sLog->outError(LOG_FILTER_SQL, "BattegroundDG: Failed to spawn some object Battleground not created!");
        return false;
    }

    return true;
}

void BattlegroundDG::Reset()
{
    //call parent's class reset
    Battleground::Reset();

    m_TeamScores[BG_TEAM_ALLIANCE]          = 0;
    m_TeamScores[BG_TEAM_HORDE]             = 0;
    m_lastTick[BG_TEAM_ALLIANCE]            = 0;
    m_lastTick[BG_TEAM_HORDE]               = 0;
    m_HonorScoreTics[BG_TEAM_ALLIANCE]      = 0;
    m_HonorScoreTics[BG_TEAM_HORDE]         = 0;
    m_ReputationScoreTics[BG_TEAM_ALLIANCE] = 0;
    m_ReputationScoreTics[BG_TEAM_HORDE]    = 0;
    m_ResourceInMineCart[BG_TEAM_ALLIANCE]  = 0;
    m_ResourceInMineCart[BG_TEAM_HORDE]     = 0;
    m_CheatersCheckTimer = 0;
    m_IsInformedNearVictory                 = false;
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_DG_DGBGWeekendHonorTicks : BG_DG_NotDGBGWeekendHonorTicks;
    m_ReputationTics = (isBGWeekend) ? BG_DG_DGBGWeekendReputationTicks : BG_DG_NotDGBGWeekendReputationTicks;
    m_TeamScores500Disadvantage[BG_TEAM_ALLIANCE] = false;
    m_TeamScores500Disadvantage[BG_TEAM_HORDE]    = false;

    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        m_Nodes[i] = 0;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;
        m_BannerTimers[i].timer = 0;
    }

    for (uint8 i = 0; i < BG_DG_ALL_NODES_COUNT + 3; ++i)// +3 for aura triggers
        if (BgCreatures[i])
            DelCreature(i);

    m_FlagKeepers[BG_TEAM_ALLIANCE] = 0;
    m_FlagKeepers[BG_TEAM_HORDE] = 0;

    m_DroppedFlagGUID[BG_TEAM_ALLIANCE] = 0;
    m_DroppedFlagGUID[BG_TEAM_HORDE] = 0;

    m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_ON_BASE;
    m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_ON_BASE;

}

void BattlegroundDG::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);

    //create score and add it to map, default values are set in constructor
    BattlegroundDGScore* sc = new BattlegroundDGScore;

    PlayerScores[player->GetGUID()] = sc;
}

void BattlegroundDG::RemovePlayer(Player* player, uint64 guid, uint32 /*team*/)
{
    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && m_FlagKeepers[BG_TEAM_ALLIANCE] == guid)
    {
        if (!player)
        {
            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundDG: Removing offline player who has the FLAG!!");
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
            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundDG: Removing offline player who has the FLAG!!");
            SetHordeFlagPicker(0);
            RespawnFlag(HORDE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }
}

void BattlegroundDG::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundDG::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())                         // player not found
        return;

    switch (type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattlegroundDGScore*)itr->second)->FlagCaptures += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattlegroundDGScore*)itr->second)->FlagReturns += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_RETURN_FLAG);
            break;
        case SCORE_BASES_ASSAULTED:
            ((BattlegroundDGScore*)itr->second)->BasesAssaulted += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_ASSAULT_BASE);
            break;
        case SCORE_BASES_DEFENDED:
            ((BattlegroundDGScore*)itr->second)->BasesDefended += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_DEFEND_BASE);
            break;
        default:
            Battleground::UpdatePlayerScore(Source, NULL, type, value, doAddHonor);
            break;
    }
}

void BattlegroundDG::CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
{
    // Just put it into the queue
    if (delay)
    {
        m_BannerTimers[node].timer = 2000;
        m_BannerTimers[node].type = type;
        m_BannerTimers[node].teamIndex = teamIndex;
        return;
    }

    uint8 obj = node * 8 + type + teamIndex;

    SpawnBGObject(obj, RESPAWN_IMMEDIATELY);

    // handle aura with banner
    if (!type)
        return;

    obj = node * 8 + ((type == BG_DG_NODE_TYPE_OCCUPIED) ? (5 + teamIndex) : 7);
    SpawnBGObject(obj, RESPAWN_IMMEDIATELY);
}

void BattlegroundDG::DelBanner(uint8 node, uint8 type, uint8 teamIndex)
{
    uint8 obj = node * 8 + type + teamIndex;
    SpawnBGObject(obj, RESPAWN_ONE_DAY);

    // handle aura with banner
    if (!type)
        return;
    obj = node * 8 + ((type == BG_DG_NODE_TYPE_OCCUPIED) ? (5 + teamIndex) : 7);
    SpawnBGObject(obj, RESPAWN_ONE_DAY);
}

void BattlegroundDG::SendNodeUpdate(uint8 node)
{
    // Send node owner state update to refresh map icons on client
    const uint8 plusArray[] = {0, 2, 4, 0, 1};

    if (m_prevNodes[node])
        UpdateWorldState(BG_DG_OP_NODEICONS[node][m_prevNodes[node]], 0);

    UpdateWorldState(BG_DG_OP_NODEICONS[node][m_Nodes[node]], m_Nodes[node] == BG_DG_NODE_STATUS_ALLY_OCCUPIED ? 2 : 1);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if (m_Nodes[i] == BG_DG_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[i] == BG_DG_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    UpdateWorldState(BG_DG_OP_OCCUPIED_BASES_ALLY, ally);
    UpdateWorldState(BG_DG_OP_OCCUPIED_BASES_HORDE, horde);
}

void BattlegroundDG::NodeOccupied(uint8 node, Team team)
{
    if (!AddSpiritGuide(node, BG_DG_SpiritGuidePos[node][0], BG_DG_SpiritGuidePos[node][1], BG_DG_SpiritGuidePos[node][2], BG_DG_SpiritGuidePos[node][3], team))
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Failed to spawn spirit guide! point: %u, team: %u, ", node, team);

    uint8 capturedNodes = 0;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        if (m_Nodes[node] == GetTeamIndexByTeamId(team) + BG_DG_NODE_TYPE_OCCUPIED && !m_NodeTimers[i])
            ++capturedNodes;
    }

    if (node >= BG_DG_DYNAMIC_NODES_COUNT)//only dynamic nodes, no start points
        return;

    Creature* trigger = GetBGCreature(node+5); // 0-5 spirit guides

    if (!trigger)
       trigger = AddCreature(WORLD_TRIGGER, node+5, team, BG_DG_NodePositions[node][0], BG_DG_NodePositions[node][1], BG_DG_NodePositions[node][2], BG_DG_NodePositions[node][3]);

    // Add bonus honor aura trigger creature when node is occupied
    // Cast bonus aura (+50% honor in 25yards)
    // aura should only apply to players who have occupied the node, set correct faction for trigger
    if (trigger)
    {
        trigger->setFaction(team == ALLIANCE ? 84 : 83);
        //trigger->CastSpell(trigger, SPELL_HONORABLE_DEFENDER_25Y, false);
    }
}

void BattlegroundDG::NodeDeOccupied(uint8 node)
{
    if (node >= BG_DG_DYNAMIC_NODES_COUNT)
        return;

    //remove bonus honor aura trigger creature when node is lost
    if (node < BG_DG_DYNAMIC_NODES_COUNT)//only dynamic nodes, no start points
        DelCreature(node + 5);//NULL checks are in DelCreature! 0-6 spirit guides

    // Players waiting to resurrect at this node are sent to closest owned graveyard
    std::vector<uint64> ghost_list = m_ReviveQueue[BgCreatures[node]];
    if (!ghost_list.empty())
    {
        WorldSafeLocsEntry const *ClosestGrave = NULL;
        for (std::vector<uint64>::const_iterator itr = ghost_list.begin(); itr != ghost_list.end(); ++itr)
        {
            Player* player = ObjectAccessor::FindPlayer(*itr);
            if (!player)
                continue;

            if (!ClosestGrave)
                ClosestGrave = GetClosestGraveYard(player);

            if (ClosestGrave)
                player->TeleportTo(GetMapId(), ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, player->GetOrientation());
        }
    }

    if (BgCreatures[node])
        DelCreature(node);

    // buff object isn't despawned
}

void BattlegroundDG::RespawnFlag(uint32 Team, bool captured)
{
    if (Team == ALLIANCE)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Respawn Alliance flag");
        m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_ON_BASE;
    }
    else
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Respawn Horde flag");
        m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_ON_BASE;
    }

    if (captured)
    {
        //when map_update will be allowed for battlegrounds this code will be useless
        SpawnBGObject(BG_DG_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnBGObject(BG_DG_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        //SendMessageToAll(LANG_BG_WS_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        //PlaySoundToAll(BG_DG_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
}

void BattlegroundDG::RespawnFlagAfterDrop(uint32 team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    RespawnFlag(team, false);
    if (team == ALLIANCE)
    {
        SpawnBGObject(BG_DG_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        //SendMessageToAll(LANG_BG_WS_ALLIANCE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnBGObject(BG_DG_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        //SendMessageToAll(LANG_BG_WS_HORDE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }

    PlaySoundToAll(BG_DG_SOUND_FLAGS_RESPAWNED);

    uint8 teamId = GetTeamIndexByTeamId(team);
    
    m_TeamScores[teamId] += m_ResourceInMineCart[teamId];
    m_ResourceInMineCart[teamId] = 0;

    if (teamId == BG_TEAM_ALLIANCE)
    {
        UpdateWorldState(BG_DG_OP_RESOURCES_ALLY, m_TeamScores[teamId]);
        UpdateWorldState(BG_DG_OP_FLAG_A, uint32(1));
    }

    if (teamId == BG_TEAM_HORDE)
    {
        UpdateWorldState(BG_DG_OP_RESOURCES_HORDE, m_TeamScores[teamId]);
        UpdateWorldState(BG_DG_OP_FLAG_H, uint32(1));
    }
        
    if (GameObject* obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID(team)))
        obj->Delete();
    else
        sLog->outError(LOG_FILTER_BATTLEGROUND, "unknown droped flag bg, guid: %u", GUID_LOPART(GetDroppedFlagGUID(team)));

    SetDroppedFlagGUID(0, team);
}

void BattlegroundDG::EventPlayerCapturedFlag(Player* Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    if (Source->GetBGTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;

        SetHordeFlagPicker(0);                              // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // horde flag in base (but not respawned yet)
        m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Horde Flag from Player
        Source->RemoveAurasDueToSpell(BG_DG_SPELL_HORDE_FLAG);

        //PlaySoundToAll(BG_DG_SOUND_FLAG_CAPTURED_ALLIANCE);
        //RewardReputationToTeam(890, m_ReputationCapture, ALLIANCE);
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;

        SetAllianceFlagPicker(0);                           // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // alliance flag in base (but not respawned yet)
        m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Alliance Flag from Player
        Source->RemoveAurasDueToSpell(BG_DG_SPELL_ALLIANCE_FLAG);

        //PlaySoundToAll(BG_DG_SOUND_FLAG_CAPTURED_HORDE);
        //RewardReputationToTeam(889, m_ReputationCapture, HORDE);
    }
    //for flag capture is reward 2 honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), Source->GetBGTeam());

    SpawnBGObject(BG_DG_OBJECT_H_FLAG, BG_DG_FLAG_RESPAWN_TIME);
    SpawnBGObject(BG_DG_OBJECT_A_FLAG, BG_DG_FLAG_RESPAWN_TIME);

    uint8 team = Source->GetBGTeam() == ALLIANCE ? BG_TEAM_ALLIANCE : BG_TEAM_HORDE;

    m_TeamScores[team] += m_ResourceInMineCart[team == BG_TEAM_ALLIANCE ? BG_TEAM_HORDE : BG_TEAM_ALLIANCE];
    m_ResourceInMineCart[team == BG_TEAM_ALLIANCE ? BG_TEAM_HORDE : BG_TEAM_ALLIANCE] = 0;

    if (team == BG_TEAM_ALLIANCE)
    {
        UpdateWorldState(BG_DG_OP_RESOURCES_ALLY, m_TeamScores[team]);
        UpdateWorldState(BG_DG_OP_FLAG_H, uint32(1));
    }

    if (team == BG_TEAM_HORDE)
    {
        UpdateWorldState(BG_DG_OP_RESOURCES_HORDE, m_TeamScores[team]);
        UpdateWorldState(BG_DG_OP_FLAG_A, uint32(1));
    }

    //if (Source->GetBGTeam() == ALLIANCE)
    //    SendMessageToAll(LANG_BG_TP_CAPTURED_HF, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    //else
    //    SendMessageToAll(LANG_BG_TP_CAPTURED_AF, CHAT_MSG_BG_SYSTEM_HORDE, Source);

    //UpdateTeamScore(Source->GetBGTeam());
    // only flag capture should be updated
    UpdatePlayerScore(Source, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures

    // update last flag capture to be used if teamscore is equal
}

void BattlegroundDG::EventPlayerDroppedFlag(Player* Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (Source->GetBGTeam() == ALLIANCE)
        {
            if (!IsHordeFlagPickedup())
                return;

            if (GetFlagPickerGUID(BG_TEAM_HORDE) == Source->GetGUID())
            {
                SetHordeFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_DG_SPELL_HORDE_FLAG);
            }
        }
        else
        {
            if (!IsAllianceFlagPickedup())
                return;

            if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == Source->GetGUID())
            {
                SetAllianceFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_DG_SPELL_ALLIANCE_FLAG);
            }
        }
        return;
    }

    bool set = false;

    if (Source->GetBGTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;

        if (GetFlagPickerGUID(BG_TEAM_HORDE) == Source->GetGUID())
        {
            SetHordeFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_DG_SPELL_HORDE_FLAG);

            m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_ON_GROUND;
            
            //Source->CastSpell(Source, BG_DG_SPELL_WARSONG_FLAG_DROPPED, true);

            GameObject* pGameObj = new GameObject;

            float x, y, z;
            Source->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

            Map* map = Source->GetMap();

            if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), BG_OBJECT_H_FLAG_GROUND_DG_ENTRY, map,
                Source->GetPhaseMask(), x, y, z, Source->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
            {
                delete pGameObj;
                return;
            }

            pGameObj->SetRespawnTime(10000);

            // Wild object not have owner and check clickable by players
            map->AddToMap(pGameObj);

            uint32 team = ALLIANCE;
            if (Source->GetTeam() == team)
                team = HORDE;

            SetDroppedFlagGUID(pGameObj->GetGUID(), team);
            
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;

        if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == Source->GetGUID())
        {
            SetAllianceFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_DG_SPELL_ALLIANCE_FLAG);

            m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_ON_GROUND;

            //Source->CastSpell(Source, BG_DG_SPELL_SILVERWING_FLAG_DROPPED, true);
            GameObject* pGameObj = new GameObject;

            float x, y, z;
            Source->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

            Map* map = Source->GetMap();

            if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), BG_OBJECT_A_FLAG_GROUND_DG_ENTRY, map,
                Source->GetPhaseMask(), x, y, z, Source->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
            {
                delete pGameObj;
                return;
            }

            pGameObj->SetRespawnTime(10000);

            // Wild object not have owner and check clickable by players
            map->AddToMap(pGameObj);

            uint32 team = ALLIANCE;
            if (Source->GetTeam() == team)
                team = HORDE;

            SetDroppedFlagGUID(pGameObj->GetGUID(), team);

            set = true;
        }
    }

    if (set)
    {
        Source->CastSpell(Source, SPELL_RECENTLY_DROPPED_FLAG, true);

        //if (Source->GetBGTeam() == ALLIANCE)
        //{
        //    SendMessageToAll(LANG_BG_TP_DROPPED_HF, CHAT_MSG_BG_SYSTEM_HORDE, Source);
        //}
        //else
        //{
        //    SendMessageToAll(LANG_BG_TP_DROPPED_AF, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
        //}

        m_flagsDropTimer[GetTeamIndexByTeamId(Source->GetBGTeam()) ? 0 : 1] = BG_DG_FLAG_DROP_TIME;
    }
}

void BattlegroundDG::EventPlayerClickedOnFlag(Player* Source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (target_obj->GetEntry() == BG_OBJECT_A_FLAG_DG_ENTRY || target_obj->GetEntry() == BG_OBJECT_H_FLAG_DG_ENTRY)
    {
        int32 message_id = 0;
        ChatMsg type = CHAT_MSG_BG_SYSTEM_NEUTRAL;

        //alliance flag picked up from base
        if (Source->GetBGTeam() == HORDE && GetFlagState(ALLIANCE) == BG_DG_FLAG_STATE_ON_BASE
            && BgObjects[BG_DG_OBJECT_A_FLAG] == target_obj->GetGUID())
        {
            SpawnBGObject(BG_DG_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
            SetAllianceFlagPicker(Source->GetGUID());
            m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_ON_PLAYER;

            Source->CastSpell(Source, BG_DG_SPELL_ALLIANCE_FLAG, true);
        }

        //horde flag picked up from base
        if (Source->GetBGTeam() == ALLIANCE && GetFlagState(HORDE) == BG_DG_FLAG_STATE_ON_BASE
            && BgObjects[BG_DG_OBJECT_H_FLAG] == target_obj->GetGUID())
        {
            SpawnBGObject(BG_DG_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
            SetHordeFlagPicker(Source->GetGUID());
            m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_ON_PLAYER;
            //update world state to show correct flag carrier

            Source->CastSpell(Source, BG_DG_SPELL_HORDE_FLAG, true);
        }

        //Alliance flag on ground(not in base) (returned or picked up again from ground!)
        if (GetFlagState(ALLIANCE) == BG_DG_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10) && target_obj->GetGOInfo()->entry == BG_OBJECT_A_FLAG_GROUND_DG_ENTRY)
        {
            if (Source->GetBGTeam() == ALLIANCE)
            {
                RespawnFlag(ALLIANCE, false);
                SpawnBGObject(BG_DG_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            }
            else
            {
                SpawnBGObject(BG_DG_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
                SetAllianceFlagPicker(Source->GetGUID());
                Source->CastSpell(Source, BG_DG_SPELL_ALLIANCE_FLAG, true);
                m_flagState[BG_TEAM_ALLIANCE] = BG_DG_FLAG_STATE_ON_PLAYER;
            }
            //called in HandleGameObjectUseOpcode:
            //target_obj->Delete();
        }

        //Horde flag on ground(not in base) (returned or picked up again)
        if (GetFlagState(HORDE) == BG_DG_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10) && target_obj->GetGOInfo()->entry == BG_OBJECT_H_FLAG_GROUND_DG_ENTRY)
        {
            if (Source->GetBGTeam() == HORDE)
            {
                RespawnFlag(HORDE, false);
                SpawnBGObject(BG_DG_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            }
            else
            {
                SpawnBGObject(BG_DG_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
                SetHordeFlagPicker(Source->GetGUID());
                Source->CastSpell(Source, BG_DG_SPELL_HORDE_FLAG, true);
                m_flagState[BG_TEAM_HORDE] = BG_DG_FLAG_STATE_ON_PLAYER;
            }
        }

        uint8 team = GetOtherTeam(Source->GetBGTeam()) == ALLIANCE ? BG_TEAM_ALLIANCE : BG_TEAM_HORDE;
        
        if (m_TeamScores[team] > BG_DG_SOURCE_BY_REWARD_FLAG)
        {
            m_TeamScores[team] -= BG_DG_SOURCE_BY_REWARD_FLAG;
            m_ResourceInMineCart[team] = 200;
        }
        else
        {
            m_ResourceInMineCart[team] = m_TeamScores[team];
            m_TeamScores[team] = 0;
        }

        if (team == BG_TEAM_ALLIANCE)
        {
            UpdateWorldState(BG_DG_OP_RESOURCES_ALLY, m_TeamScores[team]);
            UpdateWorldState(BG_DG_OP_FLAG_A, uint32(2));
        }

        if (team == BG_TEAM_HORDE)
        {
            UpdateWorldState(BG_DG_OP_RESOURCES_HORDE, m_TeamScores[team]);
            UpdateWorldState(BG_DG_OP_FLAG_H, uint32(2));
        }

        if (!message_id)
            return;

        SendMessageToAll(message_id, type, Source);
        Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    }
    else
    {

        uint8 node = BG_DG_NODE_GOBLIN;
        GameObject* obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + 7]);
        while ((node < BG_DG_DYNAMIC_NODES_COUNT) && ((!obj) || (!Source->IsWithinDistInMap(obj, 10))))
        {
            ++node;
            obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + BG_DG_OBJECT_AURA_CONTESTED]);
        }

        if (node == BG_DG_DYNAMIC_NODES_COUNT)
        {
            // this means our player isn't close to any of banners - maybe cheater ??
            return;
        }

        BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(Source->GetBGTeam());

        // Check if player really could use this banner, not cheated
        if (!(m_Nodes[node] == 0 || teamIndex == m_Nodes[node] % 2))
            return;

        Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
        uint32 sound = 0;
        // If node is neutral, change to contested
        if (m_Nodes[node] == BG_DG_NODE_TYPE_NEUTRAL)
        {
            UpdatePlayerScore(Source, SCORE_BASES_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + 1;

            // burn current neutral banner
            DelBanner(node, BG_DG_NODE_TYPE_NEUTRAL, 0);

            // create new contested banner
            CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
            SendNodeUpdate(node);
            m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

            // FIXME: need to fix Locales for team and node names.
            if (teamIndex == 0)
                SendMessage2ToAll(LANG_BG_DG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source, _GetNodeNameId(node), LANG_BG_DG_ALLY);
            else
                SendMessage2ToAll(LANG_BG_DG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_HORDE, Source, _GetNodeNameId(node), LANG_BG_DG_HORDE);

            sound = BG_DG_SOUND_NODE_CLAIMED;
        }
        // If node is contested
        else if ((m_Nodes[node] == BG_DG_NODE_STATUS_ALLY_CONTESTED) || (m_Nodes[node] == BG_DG_NODE_STATUS_HORDE_CONTESTED))
        {
            // If last state is NOT occupied, change node to enemy-contested
            if (m_prevNodes[node] < BG_DG_NODE_TYPE_OCCUPIED)
            {
                UpdatePlayerScore(Source, SCORE_BASES_ASSAULTED, 1);
                m_prevNodes[node] = m_Nodes[node];
                m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_CONTESTED;

                // burn current contested banner
                DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, !teamIndex);

                // create new contested banner
                CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
                SendNodeUpdate(node);
                m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

                // FIXME: need to fix Locales for team and node names.
                if (teamIndex == BG_TEAM_ALLIANCE)
                    SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source, _GetNodeNameId(node));
                else
                    SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, Source, _GetNodeNameId(node));
            }
            // If contested, change back to occupied
            else
            {
                UpdatePlayerScore(Source, SCORE_BASES_DEFENDED, 1);
                m_prevNodes[node] = m_Nodes[node];
                m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_OCCUPIED;

                // burn current contested banner
                DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, !teamIndex);

                // create new occupied banner
                CreateBanner(node, BG_DG_NODE_TYPE_OCCUPIED, teamIndex, true);
                SendNodeUpdate(node);
                m_NodeTimers[node] = 0;
                NodeOccupied(node, (teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);

                // FIXME: need to fix Locales for team and node names.
                if (teamIndex == BG_TEAM_ALLIANCE)
                    SendMessage2ToAll(LANG_BG_DG_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source, _GetNodeNameId(node));
                else
                    SendMessage2ToAll(LANG_BG_DG_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_HORDE, Source, _GetNodeNameId(node));
            }
            sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_DG_SOUND_NODE_ASSAULTED_HORDE;
        }
        // If node is occupied, change to enemy-contested
        else
        {
            UpdatePlayerScore(Source, SCORE_BASES_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_CONTESTED;

            // burn current occupied banner
            DelBanner(node, BG_DG_NODE_TYPE_OCCUPIED, !teamIndex);

            // create new contested banner
            CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
            SendNodeUpdate(node);
            NodeDeOccupied(node);
            m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

            // FIXME: need to fix Locales for team and node names.
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, Source, _GetNodeNameId(node));

            sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_DG_SOUND_NODE_ASSAULTED_HORDE;
        }

        // If node is occupied again, send "X has taken the Y" msg.
        if (m_Nodes[node] >= BG_DG_NODE_TYPE_OCCUPIED)
        {
            // FIXME: need to fix Locales for team and node names.
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_DG_ALLY, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_DG_HORDE, _GetNodeNameId(node));
        }
        PlaySoundToAll(sound);
    }
}

void BattlegroundDG::HandleAreaTrigger(Player* Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch (Trigger)
    {
        case 9012:                                          // Alliance Flag spawn
            if (m_flagState[BG_TEAM_HORDE] && !m_flagState[BG_TEAM_ALLIANCE])
                if (GetFlagPickerGUID(BG_TEAM_HORDE) == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
        case 9013:                                          // Horde Flag spawn
            if (m_flagState[BG_TEAM_ALLIANCE] && !m_flagState[BG_TEAM_HORDE])
                if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
    }
}

int32 BattlegroundDG::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_DG_NODE_GOBLIN: return LANG_BG_DG_NODE_LIGHTHOUSE;
        case BG_DG_NODE_CENTER: return LANG_BG_DG_NODE_WATERWORKS;
        case BG_DG_NODE_PANDAREN: return LANG_BG_DG_NODE_MINE;
        default:
            ASSERT(0);
    }
    return 0;
}

void BattlegroundDG::FillInitialWorldStates(WorldPacket& data)
{
    Player::AppendWorldState(data, uint32(BG_DG_OP_FLAG_A), uint32((GetFlagState(ALLIANCE) == BG_DG_FLAG_STATE_ON_PLAYER || GetFlagState(ALLIANCE) == BG_DG_FLAG_STATE_ON_GROUND) ? 2 : 1));
    Player::AppendWorldState(data, uint32(BG_DG_OP_FLAG_H), uint32((GetFlagState(HORDE) == BG_DG_FLAG_STATE_ON_PLAYER || GetFlagState(HORDE) == BG_DG_FLAG_STATE_ON_GROUND) ? 2 : 1));
    
    // How many bases each team owns
    uint8 ally = 0, horde = 0;

    for (uint8 node = 0; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
    {
        if (m_Nodes[node] == BG_DG_NODE_STATUS_ALLY_OCCUPIED)
        {
            Player::AppendWorldState(data, uint32(BG_DG_OP_NODEICONS[node][m_Nodes[node]]), uint32(2));
            ++ally;
        }
        else if (m_Nodes[node] == BG_DG_NODE_STATUS_HORDE_OCCUPIED)
        {
            Player::AppendWorldState(data, uint32(BG_DG_OP_NODEICONS[node][m_Nodes[node]]), uint32(1));
            ++horde;
        }
        else
            Player::AppendWorldState(data, uint32(BG_DG_OP_NODEICONS[node][m_Nodes[node]]), uint32(1));
    }

    Player::AppendWorldState(data, uint32(BG_DG_OP_OCCUPIED_BASES_ALLY), uint32(ally));
    Player::AppendWorldState(data, uint32(BG_DG_OP_OCCUPIED_BASES_HORDE), uint32(horde));

    // Team scores
    Player::AppendWorldState(data, uint32(BG_DG_OP_RESOURCES_ALLY), uint32(m_TeamScores[BG_TEAM_ALLIANCE]));
    Player::AppendWorldState(data, uint32(BG_DG_OP_RESOURCES_HORDE), uint32(m_TeamScores[BG_TEAM_HORDE]));
}

bool BattlegroundDG::IsAllNodesControlledByTeam(uint32 team) const
{
    uint32 count = 0;
    for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if ((team == ALLIANCE && m_Nodes[i] == BG_DG_NODE_STATUS_ALLY_OCCUPIED) || (team == HORDE && m_Nodes[i] == BG_DG_NODE_STATUS_HORDE_OCCUPIED))
            ++count;

    return count == BG_DG_DYNAMIC_NODES_COUNT;
}
