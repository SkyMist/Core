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

#include "BattlegroundDG.h"

BattlegroundDG::BattlegroundDG()
{
    m_IsInformedNearVictory = false;
    m_BuffChange = true;

    // BgObjects.resize(BG_DG_OBJECT_MAX);
    // BgCreatures.resize(BG_DG_ALL_NODES_COUNT + 3); // +3 for aura triggers

    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_DG_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_DG_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_DG_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_DG_HAS_BEGUN;
}

BattlegroundDG::~BattlegroundDG() { }

void BattlegroundDG::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        int team_points[BG_TEAMS_COUNT] = { 0, 0 };

        for (int node = 0; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
        {
            // 3 sec delay to spawn new banner instead of previously despawned one
            if (m_BannerTimers[node].timer)
            {
                if (m_BannerTimers[node].timer > diff)
                    m_BannerTimers[node].timer -= diff;
                else
                {
                    m_BannerTimers[node].timer = 0;
                    _CreateBanner(node, m_BannerTimers[node].type, m_BannerTimers[node].teamIndex, false);
                }
            }

            // 1 minute to occupy a node from contested state
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
                    _DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex);

                    // create new occupied banner
                    _CreateBanner(node, BG_DG_NODE_TYPE_OCCUPIED, teamIndex, true);
                    _SendNodeUpdate(node);
                    _NodeOccupied(node, (teamIndex == 0) ? ALLIANCE : HORDE);

                    // Message to chatlog
                    if (teamIndex == 0)
                    {
                        // FIXME: team and node names not localized
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
                    UpdateWorldState(BG_DG_GOLD_ALLY, m_TeamScores[team]);
                else if (team == BG_TEAM_HORDE)
                    UpdateWorldState(BG_DG_GOLD_HORDE, m_TeamScores[team]);
            }
        }

        // Test win condition
        if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_DG_MAX_TEAM_SCORE)
            EndBattleground(ALLIANCE);
        else if (m_TeamScores[BG_TEAM_HORDE] >= BG_DG_MAX_TEAM_SCORE)
            EndBattleground(HORDE);
    }
}

void BattlegroundDG::StartingEventCloseDoors()
{
    // // despawn banners, auras and buffs
    // for (int obj = BG_DG_OBJECT_BANNER_NEUTRAL; obj < BG_DG_DYNAMIC_NODES_COUNT * 8; ++obj)
    //     SpawnBGObject(obj, RESPAWN_ONE_DAY);
    // for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT * 2; ++i)
    //     SpawnBGObject(BG_DG_OBJECT_REGENBUFF_C_MINE + i, RESPAWN_ONE_DAY);
    // 
    // // Starting doors
    // DoorClose(BG_DG_OBJECT_GATE_A_1);
    // DoorClose(BG_DG_OBJECT_GATE_A_2);
    // DoorClose(BG_DG_OBJECT_GATE_H_1);
    // DoorClose(BG_DG_OBJECT_GATE_H_2);
    // SpawnBGObject(BG_DG_OBJECT_GATE_A_1, RESPAWN_IMMEDIATELY);
    // SpawnBGObject(BG_DG_OBJECT_GATE_A_2, RESPAWN_IMMEDIATELY);
    // SpawnBGObject(BG_DG_OBJECT_GATE_H_1, RESPAWN_IMMEDIATELY);
    // SpawnBGObject(BG_DG_OBJECT_GATE_H_2, RESPAWN_IMMEDIATELY);
    // 
    // // Starting base spirit guides
    // _NodeOccupied(BG_DG_SPIRIT_ALIANCE, ALLIANCE);
    // _NodeOccupied(BG_DG_SPIRIT_HORDE, HORDE);
}

void BattlegroundDG::StartingEventOpenDoors()
{
    // // spawn neutral banners
    // for (int banner = BG_DG_OBJECT_BANNER_NEUTRAL, i = 0; i < 3; banner += 8, ++i)
    //     SpawnBGObject(banner, RESPAWN_IMMEDIATELY);
    // for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    // {
    //     //randomly select buff to spawn
    //     uint8 buff = urand(0, 2);
    //     SpawnBGObject(BG_DG_OBJECT_REGENBUFF_C_MINE + buff + i * 2, RESPAWN_IMMEDIATELY);
    // }
    // 
    // DoorOpen(BG_DG_OBJECT_GATE_A_1);
    // DoorOpen(BG_DG_OBJECT_GATE_A_2);
    // DoorOpen(BG_DG_OBJECT_GATE_H_1);
    // DoorOpen(BG_DG_OBJECT_GATE_H_2);
}

void BattlegroundDG::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in the constructor
    BattlegroundDGScore* sc = new BattlegroundDGScore;

    PlayerScores[player->GetGUID()] = sc;
}

void BattlegroundDG::RemovePlayer(Player* /*player*/, uint64 /*guid*/, uint32 /*team*/) { }

void BattlegroundDG::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    // switch (trigger)
    // {
    //     case 3948:                                          // Arathi Basin Alliance Exit.
    //         if (player->GetTeam() != ALLIANCE)
    //             player->GetSession()->SendAreaTriggerMessage("Only The Alliance can use that portal");
    //         else
    //             player->LeaveBattleground();
    //         break;
    //     case 3949:                                          // Arathi Basin Horde Exit.
    //         if (player->GetTeam() != HORDE)
    //             player->GetSession()->SendAreaTriggerMessage("Only The Horde can use that portal");
    //         else
    //             player->LeaveBattleground();
    //         break;
    //     case 3866:                                          // Stables
    //     case 3869:                                          // Gold Mine
    //     case 3867:                                          // Farm
    //     case 3868:                                          // Lumber Mill
    //     case 3870:                                          // Black Smith
    //     case 4020:                                          // Unk1
    //     case 4021:                                          // Unk2
    //     case 4674:                                          // Unk3
    //         //break;
    //     default:
    //         Battleground::HandleAreaTrigger(player, trigger);
    //         break;
    // }
}

/*  type: 0-neutral, 1-contested, 3-occupied; teamIndex: 0-ally, 1-horde             */
void BattlegroundDG::_CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
{
    // // Just put it into the queue
    // if (delay)
    // {
    //     m_BannerTimers[node].timer = 2000;
    //     m_BannerTimers[node].type = type;
    //     m_BannerTimers[node].teamIndex = teamIndex;
    //     return;
    // }
    // 
    // uint8 obj = node * 8 + type + teamIndex;
    // 
    // SpawnBGObject(obj, RESPAWN_IMMEDIATELY);
    // 
    // // handle aura with banner
    // if (!type)
    //     return;
    // 
    // obj = node * 8 + ((type == BG_DG_NODE_TYPE_OCCUPIED) ? (3 + teamIndex) : 7);
    // SpawnBGObject(obj, RESPAWN_IMMEDIATELY);
}

void BattlegroundDG::_DelBanner(uint8 node, uint8 type, uint8 teamIndex)
{
    // uint8 obj = node * 8 + type + teamIndex;
    // SpawnBGObject(obj, RESPAWN_ONE_DAY);
    // 
    // // handle aura with banner
    // if (!type)
    //     return;
    // obj = node * 8 + ((type == BG_DG_NODE_TYPE_OCCUPIED) ? (3 + teamIndex) : 7);
    // SpawnBGObject(obj, RESPAWN_ONE_DAY);
}

int32 BattlegroundDG::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_DG_NODE_C_MINE:    return LANG_BG_DG_NODE_CENTER_MINE;
        case BG_DG_NODE_G_MINE:    return LANG_BG_DG_NODE_GOBLIN_MINE;
        case BG_DG_NODE_P_MINE:    return LANG_BG_DG_NODE_PANDAREN_MINE;
        default:
            ASSERT(false);
    }
    return 0;
}

uint32 BattlegroundDG::GetOccupiedNodes(Team team)
{
    uint8 ally = 0, horde = 0;
    for (uint8 node = BG_DG_NODE_C_MINE; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
        if (m_Nodes[node] == BG_DG_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[node] == BG_DG_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    if (team == ALLIANCE)
        return ally;
    else
        return horde;
}

void BattlegroundDG::FillInitialWorldStates(ByteBuffer& data)
{
    // Init the displays
    data << uint32(0x1) << uint32(BG_DG_SHOW_BASES_GOLD_ALLY);
    data << uint32(0x1) << uint32(BG_DG_SHOW_BASES_GOLD_HORDE);

    // Node icons
    for (uint8 node = BG_DG_NODE_C_MINE; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
        data << uint32((m_Nodes[node] == BG_DG_NODE_TYPE_NEUTRAL) ? 1 : 0) << uint32(BG_DG_OP_NODEICONS[node]);

    // How many bases each team owns
    data << uint32(GetOccupiedNodes(ALLIANCE))        << uint32(BG_DG_OCCUPIED_BASES_ALLY);
    data << uint32(GetOccupiedNodes(HORDE))           << uint32(BG_DG_OCCUPIED_BASES_HORDE);

    // Team scores
    data << uint32(BG_DG_WARNING_NEAR_VICTORY_SCORE)  << uint32(BG_DG_OP_RESOURCES_WARNING);
    data << uint32(m_TeamScores[BG_TEAM_ALLIANCE])    << uint32(BG_DG_GOLD_ALLY);
    data << uint32(m_TeamScores[BG_TEAM_HORDE])       << uint32(BG_DG_GOLD_HORDE);

    // other unknown
    data << uint32(0x2)                               << uint32(0x745);           // 37 1861 unk
}

void BattlegroundDG::_SendNodeUpdate(uint8 node)
{
    // Don't update the nodes if their state has not changed.
    if (m_prevNodes[node] == m_Nodes[node])
        return;

    // Node occupied states update
    for (uint8 node = BG_DG_NODE_C_MINE; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
    {
        switch (m_Nodes[node])
        {
            case BG_DG_NODE_STATUS_ALLY_CONTESTED:
            {
                switch(node)
                {
                    case BG_DG_NODE_C_MINE:
                        UpdateWorldState(CENTER_MINE_CONFLICT_ALLIANCE,     1);
                        UpdateWorldState(CENTER_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(CENTER_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(CENTER_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     1);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   1);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        break;

                    default: break;
                }
                break;
            }
            case BG_DG_NODE_STATUS_HORDE_CONTESTED:
            {
                switch(node)
                {
                    case BG_DG_NODE_C_MINE:
                        UpdateWorldState(CENTER_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(CENTER_MINE_CONFLICT_HORDE,        1);
                        UpdateWorldState(CENTER_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(CENTER_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        1);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      1);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        break;

                    default: break;
                }
                break;
            }
            case BG_DG_NODE_STATUS_ALLY_OCCUPIED:
            {
                switch(node)
                {
                    case BG_DG_NODE_C_MINE:
                        UpdateWorldState(CENTER_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(CENTER_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(CENTER_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(CENTER_MINE_ALLIANCE_CONTROLLED,   1);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   1);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 1);
                        break;

                    default: break;
                }
                break;
            }
            case BG_DG_NODE_STATUS_HORDE_OCCUPIED:
            {
                switch(node)
                {
                    case BG_DG_NODE_C_MINE:
                        UpdateWorldState(CENTER_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(CENTER_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(CENTER_MINE_HORDE_CONTROLLED,      1);
                        UpdateWorldState(CENTER_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      1);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    1);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        break;

                    default: break;
                }
                break;
            }

            default: break;
        }
    }

    // How many bases each team owns
    UpdateWorldState(BG_DG_OCCUPIED_BASES_ALLY, GetOccupiedNodes(ALLIANCE));
    UpdateWorldState(BG_DG_OCCUPIED_BASES_HORDE, GetOccupiedNodes(HORDE));
}

void BattlegroundDG::_NodeOccupied(uint8 node, Team team)
{
    // if (!AddSpiritGuide(node, BG_DG_SpiritGuidePos[node][0], BG_DG_SpiritGuidePos[node][1], BG_DG_SpiritGuidePos[node][2], BG_DG_SpiritGuidePos[node][3], team))
    //     TC_LOG_ERROR("bg.battleground", "Failed to spawn spirit guide! point: %u, team: %u, ", node, team);
    // 
    // if (node >= BG_DG_DYNAMIC_NODES_COUNT) // only dynamic nodes, no start points
    //     return;
    // 
    // uint8 capturedNodes = 0;
    // for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    //     if (m_Nodes[i] == GetTeamIndexByTeamId(team) + BG_DG_NODE_TYPE_OCCUPIED && !m_NodeTimers[i])
    //         ++capturedNodes;
    // 
    // Creature* trigger = BgCreatures[node + 8] ? GetBGCreature(node + 8) : NULL; // 0-7 spirit guides
    // if (!trigger)
    //     trigger = AddCreature(WORLD_TRIGGER, node + 8, team, BG_DG_NodePositions[node][0], BG_DG_NodePositions[node][1], BG_DG_NodePositions[node][2], BG_DG_NodePositions[node][3]);
    // 
    // // add bonus honor aura trigger creature when node is accupied - cast bonus aura (+50% honor in 25yards).
    // // aura should only apply to players who have accupied the node, set correct faction for trigger.
    // if (trigger)
    // {
    //     trigger->setFaction(team == ALLIANCE ? 84 : 83);
    //     trigger->CastSpell(trigger, SPELL_HONORABLE_DEFENDER_25Y, false);
    // }
}

void BattlegroundDG::_NodeDeOccupied(uint8 node)
{
    // if (node >= BG_DG_DYNAMIC_NODES_COUNT)
    //     return;
    // 
    // //remove bonus honor aura trigger creature when node is lost
    // if (node < BG_DG_DYNAMIC_NODES_COUNT)//only dynamic nodes, no start points
    //     DelCreature(node + 8);// NULL checks are in DelCreature! 0-7 spirit guides
    // 
    // RelocateDeadPlayers(BgCreatures[node]);
    // 
    // DelCreature(node);
    // 
    // // buff object isn't despawned
}

/* Invoked if a player used a banner as a gameobject */
void BattlegroundDG::EventPlayerClickedOnFlag(Player* source, GameObject* /*target_obj*/)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 node = BG_DG_NODE_C_MINE;
    GameObject* obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + 8]);
    while ((node < BG_DG_DYNAMIC_NODES_COUNT) && ((!obj) || (!source->IsWithinDistInMap(obj, 10))))
    {
        ++node;
        obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + BG_DG_OBJECT_AURA_CONTESTED]);
    }

    if (node == BG_DG_DYNAMIC_NODES_COUNT)
    {
        // this means our player isn't close to any of banners - maybe cheater ??
        return;
    }

    BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(m_Nodes[node] == BG_DG_NODE_TYPE_NEUTRAL || teamIndex == m_Nodes[node] % 2))
        return;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    uint32 sound = 0;

    // If node is neutral, change to contested
    if (m_Nodes[node] == BG_DG_NODE_TYPE_NEUTRAL)
    {
        UpdatePlayerScore(source, SCORE_MINES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + 1;
        // burn current neutral banner
        _DelBanner(node, BG_DG_NODE_TYPE_NEUTRAL, 0);
        // create new contested banner
        _CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

        // FIXME: team and node names not localized
        if (teamIndex == 0)
            SendMessage2ToAll(LANG_BG_DG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node), LANG_BG_DG_ALLY);
        else
            SendMessage2ToAll(LANG_BG_DG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node), LANG_BG_DG_HORDE);

        sound = BG_DG_SOUND_NODE_CLAIMED;
    }
    // If node is contested
    else if ((m_Nodes[node] == BG_DG_NODE_STATUS_ALLY_CONTESTED) || (m_Nodes[node] == BG_DG_NODE_STATUS_HORDE_CONTESTED))
    {
        // If last state is NOT occupied, change node to enemy-contested
        if (m_prevNodes[node] < BG_DG_NODE_TYPE_OCCUPIED)
        {
            UpdatePlayerScore(source, SCORE_MINES_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_CONTESTED;
            // burn current contested banner
            _DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, !teamIndex);
            // create new contested banner
            _CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

            // FIXME: node names not localized
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_DG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        // If contested, change back to occupied
        else
        {
            UpdatePlayerScore(source, SCORE_MINES_DEFENDED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_OCCUPIED;
            // burn current contested banner
            _DelBanner(node, BG_DG_NODE_TYPE_CONTESTED, !teamIndex);
            // create new occupied banner
            _CreateBanner(node, BG_DG_NODE_TYPE_OCCUPIED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = 0;
            _NodeOccupied(node, (teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE:HORDE);

            // FIXME: node names not localized
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_DG_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_DG_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_DG_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied, change to enemy-contested
    else
    {
        UpdatePlayerScore(source, SCORE_MINES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + BG_DG_NODE_TYPE_CONTESTED;
        // burn current occupied banner
        _DelBanner(node, BG_DG_NODE_TYPE_OCCUPIED, !teamIndex);
        // create new contested banner
        _CreateBanner(node, BG_DG_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        _NodeDeOccupied(node);
        m_NodeTimers[node] = BG_DG_FLAG_CAPTURING_TIME;

        // FIXME: node names not localized
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));

        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_DG_SOUND_NODE_ASSAULTED_HORDE;
    }

    // If node is occupied again, send "X has taken the Y" msg.
    if (m_Nodes[node] >= BG_DG_NODE_TYPE_OCCUPIED)
    {
        // FIXME: team and node names not localized
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_DG_ALLY, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_DG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_DG_HORDE, _GetNodeNameId(node));
    }
    PlaySoundToAll(sound);
}

uint32 BattlegroundDG::GetPrematureWinner()
{
    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if (m_Nodes[i] == BG_DG_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[i] == BG_DG_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    if (ally > horde)
        return ALLIANCE;
    else if (horde > ally)
        return HORDE;

    // If the values are equal, fall back to the original result (based on number of players on each team)
    return Battleground::GetPrematureWinner();
}

bool BattlegroundDG::SetupBattleground()
{
    // for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    // {
    //     if (!AddObject(BG_DG_OBJECT_BANNER_NEUTRAL + 8*i, BG_DG_OBJECTID_NODE_BANNER_0 + i, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_BANNER_CONT_A + 8*i, BG_DG_OBJECTID_BANNER_CONT_A, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_BANNER_CONT_H + 8*i, BG_DG_OBJECTID_BANNER_CONT_H, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_BANNER_ALLY + 8*i, BG_DG_OBJECTID_BANNER_A, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_BANNER_HORDE + 8*i, BG_DG_OBJECTID_BANNER_H, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_AURA_ALLY + 8*i, BG_DG_OBJECTID_AURA_A, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_AURA_HORDE + 8*i, BG_DG_OBJECTID_AURA_H, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_AURA_CONTESTED + 8*i, BG_DG_OBJECTID_AURA_C, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3]/2), std::cos(BG_DG_NodePositions[i][3]/2), RESPAWN_ONE_DAY))
    //     {
    //         TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn some object Battleground not created!");
    //         return false;
    //     }
    // }
    // if (!AddObject(BG_DG_OBJECT_GATE_A, BG_DG_OBJECTID_GATE_A, BG_DG_DoorPositions[0][0], BG_DG_DoorPositions[0][1], BG_DG_DoorPositions[0][2], BG_DG_DoorPositions[0][3], BG_DG_DoorPositions[0][4], BG_DG_DoorPositions[0][5], BG_DG_DoorPositions[0][6], BG_DG_DoorPositions[0][7], RESPAWN_IMMEDIATELY)
    //     || !AddObject(BG_DG_OBJECT_GATE_H, BG_DG_OBJECTID_GATE_H, BG_DG_DoorPositions[1][0], BG_DG_DoorPositions[1][1], BG_DG_DoorPositions[1][2], BG_DG_DoorPositions[1][3], BG_DG_DoorPositions[1][4], BG_DG_DoorPositions[1][5], BG_DG_DoorPositions[1][6], BG_DG_DoorPositions[1][7], RESPAWN_IMMEDIATELY))
    // {
    //     TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn door object Battleground not created!");
    //     return false;
    // }
    // //buffs
    // for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    // {
    //     if (!AddObject(BG_DG_OBJECT_SPEEDBUFF_STABLES + 3 * i, Buff_Entries[0], BG_DG_BuffPositions[i][0], BG_DG_BuffPositions[i][1], BG_DG_BuffPositions[i][2], BG_DG_BuffPositions[i][3], 0, 0, std::sin(BG_DG_BuffPositions[i][3]/2), std::cos(BG_DG_BuffPositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_SPEEDBUFF_STABLES + 3 * i + 1, Buff_Entries[1], BG_DG_BuffPositions[i][0], BG_DG_BuffPositions[i][1], BG_DG_BuffPositions[i][2], BG_DG_BuffPositions[i][3], 0, 0, std::sin(BG_DG_BuffPositions[i][3]/2), std::cos(BG_DG_BuffPositions[i][3]/2), RESPAWN_ONE_DAY)
    //         || !AddObject(BG_DG_OBJECT_SPEEDBUFF_STABLES + 3 * i + 2, Buff_Entries[2], BG_DG_BuffPositions[i][0], BG_DG_BuffPositions[i][1], BG_DG_BuffPositions[i][2], BG_DG_BuffPositions[i][3], 0, 0, std::sin(BG_DG_BuffPositions[i][3]/2), std::cos(BG_DG_BuffPositions[i][3]/2), RESPAWN_ONE_DAY))
    //         TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn buff object!");
    // }

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
    m_IsInformedNearVictory                 = false;
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_DG_DGBGWeekendHonorTicks : BG_DG_NotDGBGWeekendHonorTicks;

    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        m_Nodes[i]              = 0;
        m_prevNodes[i]          = 0;
        m_NodeTimers[i]         = 0;
        m_BannerTimers[i].timer = 0;
    }

    for (uint8 i = 0; i < BG_DG_ALL_NODES_COUNT + 5; ++i)//+5 for aura triggers
        if (!BgCreatures.empty())
            if (BgCreatures[i])
                DelCreature(i);
}

void BattlegroundDG::EndBattleground(uint32 winner)
{
    // Win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);

    // Complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);

    Battleground::EndBattleground(winner);
}

WorldSafeLocsEntry const* BattlegroundDG::GetClosestGraveYard(Player* player)
{
    BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(player->GetTeam());

    // Is there any occupied node for this team?
    std::vector<uint8> nodes;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if (m_Nodes[i] == teamIndex + 3)
            nodes.push_back(i);

    WorldSafeLocsEntry const* good_entry = NULL;
    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float plr_x = player->GetPositionX();
        float plr_y = player->GetPositionY();

        float mindist = 999999.0f;
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const*entry = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[nodes[i]]);
            if (!entry)
                continue;
            float dist = (entry->x - plr_x)*(entry->x - plr_x)+(entry->y - plr_y)*(entry->y - plr_y);
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
        good_entry = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[teamIndex+5]);

    return good_entry;
}

void BattlegroundDG::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())                         // player not found...
        return;

    switch (type)
    {
        case SCORE_MINES_ASSAULTED:
            ((BattlegroundDGScore*)itr->second)->MinesAssaulted += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_ASSAULT_MINE);
            break;
        case SCORE_MINES_DEFENDED:
            ((BattlegroundDGScore*)itr->second)->MinesDefended += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_DEFEND_MINE);
            break;
        default:
            Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);
            break;
    }
}

bool BattlegroundDG::IsAllNodesControlledByTeam(uint32 team) const
{
    uint32 count = 0;
    for (int i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if ((team == ALLIANCE && m_Nodes[i] == BG_DG_NODE_STATUS_ALLY_OCCUPIED) ||
            (team == HORDE    && m_Nodes[i] == BG_DG_NODE_STATUS_HORDE_OCCUPIED))
            ++count;

    return count == BG_DG_DYNAMIC_NODES_COUNT;
}

bool BattlegroundDG::CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* player, Unit const* target, uint32 miscvalue)
{
    return Battleground::CheckAchievementCriteriaMeet(criteriaId, player, target, miscvalue);
}
