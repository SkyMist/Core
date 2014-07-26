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
#include "MotionMaster.h"

#include "BattlegroundDG.h"

BattlegroundDG::BattlegroundDG()
{
    m_IsInformedNearVictory = false;
    m_BuffChange            = true;

    cartPullerA             = NULL;
    cartPullerH             = NULL;

    cartAdropped            = false;
    cartHdropped            = false;

    BgObjects.resize(BG_DG_OBJECTS_MAX);
    BgCreatures.resize(BG_DG_ALL_NODES_COUNT);

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
        uint8 team_points[BG_TEAMS_COUNT] = { 0, 0 };

        for (uint8 node = 0; node < BG_DG_DYNAMIC_NODES_COUNT; ++node)
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

            for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
                if (m_Nodes[node] == team + BG_DG_NODE_TYPE_OCCUPIED)
                    ++team_points[team];
        }

        // Accumulate points
        for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        {
            uint8 points = team_points[team];
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

                if (!m_IsInformedNearVictory && m_TeamScores[team] >= BG_DG_WARNING_NEAR_VICTORY_SCORE)
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
    // Despawn banners, auras, poles and buffs
    for (uint8 obj = BG_DG_OBJECT_BANNER_NEUTRAL; obj < BG_DG_DYNAMIC_NODES_COUNT * 8; ++obj)
        SpawnBGObject(obj, RESPAWN_ONE_DAY);

    for (uint8 buff = BG_DG_OBJECT_REGENBUFF_1; buff <= BG_DG_OBJECT_BERSERKBUFF_2; ++buff)
        SpawnBGObject(buff, RESPAWN_ONE_DAY);

    for (uint8 pole = BG_DG_OBJECT_FLAGPOLE_1; pole <= BG_DG_OBJECT_FLAGPOLE_3; ++pole)
        SpawnBGObject(pole, RESPAWN_ONE_DAY);

    // Starting doors
    for (uint8 i = BG_DG_OBJECT_GATE_A_1; i <= BG_DG_OBJECT_GATE_H_2; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }

    // Put proper carts flags.
    if (Creature* CartA = GetBGCreature(BG_DG_CREATURE_CART_A))
    if (Creature* CartH = GetBGCreature(BG_DG_CREATURE_CART_H))
    {
        CartA->setFaction(35);
        CartA->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        CartA->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        CartH->setFaction(35);
        CartH->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        CartH->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
}

void BattlegroundDG::StartingEventOpenDoors()
{
    for (uint8 i = BG_DG_OBJECT_GATE_A_1; i <= BG_DG_OBJECT_GATE_H_2; ++i)
        DoorOpen(i);

    // Spawn neutral banners
    for (uint8 banner = BG_DG_OBJECT_BANNER_NEUTRAL, i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; banner += 8, ++i)
        SpawnBGObject(banner, RESPAWN_IMMEDIATELY);

    // Spawn buffs
    for (uint8 i = BG_DG_OBJECT_REGENBUFF_1; i <= BG_DG_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    // Spawn Poles
    for (uint8 pole = BG_DG_OBJECT_FLAGPOLE_1; pole <= BG_DG_OBJECT_FLAGPOLE_3; ++pole)
        SpawnBGObject(pole, RESPAWN_IMMEDIATELY);

    // Put proper carts flags.
    if (Creature* CartA = GetBGCreature(BG_DG_CREATURE_CART_A))
    if (Creature* CartH = GetBGCreature(BG_DG_CREATURE_CART_H))
    {
        CartA->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        CartA->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        CartH->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        CartH->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
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

    switch (trigger)
    {
        case 9012: // Alliance cart point, when bringing in Horde gold cart.
            if (player->GetTeam() == ALLIANCE && player->HasAura(DG_CART_BUFF_HORDE))
                EventPlayerCapturedCart(player, BG_DG_OBJECTID_GOLD_CART_H);
            break;

        case 9013: // Horde cart point, when bringing in Alliance gold cart.
            if (player->GetTeam() == HORDE && player->HasAura(DG_CART_BUFF_ALLIANCE))
                EventPlayerCapturedCart(player, BG_DG_OBJECTID_GOLD_CART_A);
            break;

        default:
            Battleground::HandleAreaTrigger(player, trigger);
            break;
    }
}

/*  type: 0-neutral, 1-contested, 3-occupied; teamIndex: 0-ally, 1-horde             */
void BattlegroundDG::_CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
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

void BattlegroundDG::_DelBanner(uint8 node, uint8 type, uint8 teamIndex)
{
    uint8 obj = node * 8 + type + teamIndex;
    SpawnBGObject(obj, RESPAWN_ONE_DAY);

    // handle aura with banner
    if (!type)
        return;

    obj = node * 8 + ((type == BG_DG_NODE_TYPE_OCCUPIED) ? (5 + teamIndex) : 7);
    SpawnBGObject(obj, RESPAWN_ONE_DAY);
}

int32 BattlegroundDG::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_DG_NODE_C_MINE:    return LANG_BG_DG_NODE_CENTER_MINE;
        case BG_DG_NODE_G_MINE:    return LANG_BG_DG_NODE_GOBLIN_MINE;
        case BG_DG_NODE_P_MINE:    return LANG_BG_DG_NODE_PANDAREN_MINE;

        default: ASSERT(false); break;
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
    data << uint32(BG_DG_CART_STATE_NORMAL) << uint32(BG_DG_SHOW_BASES_GOLD_ALLY);
    data << uint32(BG_DG_CART_STATE_NORMAL) << uint32(BG_DG_SHOW_BASES_GOLD_HORDE);

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
                        UpdateWorldState(7939,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     1);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        UpdateWorldState(7938,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   1);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        UpdateWorldState(7935,   0);
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
                        UpdateWorldState(7939,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        1);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        UpdateWorldState(7938,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      1);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        UpdateWorldState(7935,   0);
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
                        UpdateWorldState(7939,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      0);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   1);
                        UpdateWorldState(7938,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    0);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 1);
                        UpdateWorldState(7935,   0);
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
                        UpdateWorldState(7939,   0);
                        break;
                    case BG_DG_NODE_G_MINE:
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_ALLIANCE,     0);
                        UpdateWorldState(GOBLIN_MINE_CONFLICT_HORDE,        0);
                        UpdateWorldState(GOBLIN_MINE_HORDE_CONTROLLED,      1);
                        UpdateWorldState(GOBLIN_MINE_ALLIANCE_CONTROLLED,   0);
                        UpdateWorldState(7938,   0);
                        break;
                    case BG_DG_NODE_P_MINE:
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_ALLIANCE,   0);
                        UpdateWorldState(PANDAREN_MINE_CONFLICT_HORDE,      0);
                        UpdateWorldState(PANDAREN_MINE_HORDE_CONTROLLED,    1);
                        UpdateWorldState(PANDAREN_MINE_ALLIANCE_CONTROLLED, 0);
                        UpdateWorldState(7935,   0);
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
    if (node >= BG_DG_DYNAMIC_NODES_COUNT) // only dynamic nodes, no start points
        return;

    uint8 capturedNodes = 0;
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if (m_Nodes[i] == GetTeamIndexByTeamId(team) + BG_DG_NODE_TYPE_OCCUPIED && !m_NodeTimers[i])
            ++capturedNodes;

    Creature* trigger = BgCreatures[node + 10] ? GetBGCreature(node + 10) : NULL;
    if (!trigger)
        trigger = AddCreature(WORLD_TRIGGER, node + 10, team, BG_DG_NodePositions[node][0], BG_DG_NodePositions[node][1], BG_DG_NodePositions[node][2], BG_DG_NodePositions[node][3]);

    // add bonus honor aura trigger creature when node is accupied - cast bonus aura (+50% honor in 25yards).
    // aura should only apply to players who have accupied the node, set correct faction for trigger.
    if (trigger)
    {
        trigger->setFaction(team == ALLIANCE ? 84 : 83);
        trigger->CastSpell(trigger, SPELL_HONORABLE_DEFENDER_25Y, false);
    }
}

void BattlegroundDG::_NodeDeOccupied(uint8 node)
{
    if (node >= BG_DG_DYNAMIC_NODES_COUNT)
        return;

    // remove bonus honor aura trigger creature when node is lost
    if (node < BG_DG_DYNAMIC_NODES_COUNT) // only dynamic nodes, no start points
        DelCreature(node + 10); // NULL checks are in DelCreature!

    RelocateDeadPlayers(BgCreatures[node]);

    DelCreature(node);
}

/* Invoked if a player used a banner as a gameobject */
void BattlegroundDG::EventPlayerClickedOnFlag(Player* source, GameObject* /*target_obj*/)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        TC_LOG_ERROR("bg.battleground", "BattlegroundDG: You cannot click on banners before the BG starts!");
        return;
    }

    uint8 node = BG_DG_NODE_C_MINE;
    GameObject* obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + BG_DG_OBJECT_AURA_CONTESTED]);
    while ((node < BG_DG_DYNAMIC_NODES_COUNT) && ((!obj) || (!source->IsWithinDistInMap(obj, 10))))
    {
        ++node;
        obj = GetBgMap()->GetGameObject(BgObjects[node * 8 + BG_DG_OBJECT_AURA_CONTESTED]);
    }

    if (node == BG_DG_DYNAMIC_NODES_COUNT)
    {
        TC_LOG_ERROR("bg.battleground", "BattlegroundDG: You aren't close to any of the banners!");
        return;
    }

    BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(m_Nodes[node] == BG_DG_NODE_TYPE_NEUTRAL || teamIndex == m_Nodes[node] % 2))
    {
        TC_LOG_ERROR("bg.battleground", "BattlegroundDG: You cannot use this banner!");
        return;
    }

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
            _NodeOccupied(node, (teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);

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
    // Flags.
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        if (!AddObject(BG_DG_OBJECT_BANNER_NEUTRAL + 8 * i, BG_DG_OBJECTID_NODE_BANNER_0 + i, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_BANNER_CONT_A  + 8 * i, BG_DG_OBJECTID_BANNER_CONT_A,     BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_BANNER_CONT_H  + 8 * i, BG_DG_OBJECTID_BANNER_CONT_H,     BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_BANNER_ALLY    + 8 * i, BG_DG_OBJECTID_BANNER_A,          BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_BANNER_HORDE   + 8 * i, BG_DG_OBJECTID_BANNER_H,          BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_AURA_ALLY      + 8 * i, BG_DG_OBJECTID_AURA_A,            BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_AURA_HORDE     + 8 * i, BG_DG_OBJECTID_AURA_H,            BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY)
         || !AddObject(BG_DG_OBJECT_AURA_CONTESTED + 8 * i, BG_DG_OBJECTID_AURA_C,            BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY))
        {
            TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn some object - Battleground not created!");
            return false;
        }
    }

    // Poles.
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        if (!AddObject(BG_DG_OBJECT_FLAGPOLE_1 + i, BG_DG_OBJECTID_FLAGPOLE, BG_DG_NodePositions[i][0], BG_DG_NodePositions[i][1], BG_DG_NodePositions[i][2], BG_DG_NodePositions[i][3], 0, 0, std::sin(BG_DG_NodePositions[i][3] / 2), std::cos(BG_DG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY))
        {
            TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn flag poles - Battleground not created!");
            return false;
        }
    }

    // Doors.
    if (!AddObject(BG_DG_OBJECT_GATE_H_1, BG_DG_OBJECTID_GATE, BG_DG_DoorPositions[0][0], BG_DG_DoorPositions[0][1], BG_DG_DoorPositions[0][2], BG_DG_DoorPositions[0][3], BG_DG_DoorPositions[0][4], BG_DG_DoorPositions[0][5], BG_DG_DoorPositions[0][6], BG_DG_DoorPositions[0][7], RESPAWN_IMMEDIATELY)
     || !AddObject(BG_DG_OBJECT_GATE_H_2, BG_DG_OBJECTID_GATE, BG_DG_DoorPositions[1][0], BG_DG_DoorPositions[1][1], BG_DG_DoorPositions[1][2], BG_DG_DoorPositions[1][3], BG_DG_DoorPositions[1][4], BG_DG_DoorPositions[1][5], BG_DG_DoorPositions[1][6], BG_DG_DoorPositions[1][7], RESPAWN_IMMEDIATELY)
     || !AddObject(BG_DG_OBJECT_GATE_A_1, BG_DG_OBJECTID_GATE, BG_DG_DoorPositions[2][0], BG_DG_DoorPositions[2][1], BG_DG_DoorPositions[2][2], BG_DG_DoorPositions[2][3], BG_DG_DoorPositions[2][4], BG_DG_DoorPositions[2][5], BG_DG_DoorPositions[2][6], BG_DG_DoorPositions[2][7], RESPAWN_IMMEDIATELY)
     || !AddObject(BG_DG_OBJECT_GATE_A_2, BG_DG_OBJECTID_GATE, BG_DG_DoorPositions[3][0], BG_DG_DoorPositions[3][1], BG_DG_DoorPositions[3][2], BG_DG_DoorPositions[3][3], BG_DG_DoorPositions[3][4], BG_DG_DoorPositions[3][5], BG_DG_DoorPositions[3][6], BG_DG_DoorPositions[3][7], RESPAWN_IMMEDIATELY))
    {
        TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn door object - Battleground not created!");
        return false;
    }

    // Buffs.
    if (!AddObject(BG_DG_OBJECT_BERSERKBUFF_1, Buff_Entries[2], BG_DG_BuffPositions[0][0], BG_DG_BuffPositions[0][1], BG_DG_BuffPositions[0][2], BG_DG_BuffPositions[0][3], 0, 0, std::sin(BG_DG_BuffPositions[0][3] / 2), std::cos(BG_DG_BuffPositions[0][3] / 2), RESPAWN_ONE_DAY)
     || !AddObject(BG_DG_OBJECT_BERSERKBUFF_2, Buff_Entries[2], BG_DG_BuffPositions[1][0], BG_DG_BuffPositions[1][1], BG_DG_BuffPositions[1][2], BG_DG_BuffPositions[1][3], 0, 0, std::sin(BG_DG_BuffPositions[1][3] / 2), std::cos(BG_DG_BuffPositions[1][3] / 2), RESPAWN_ONE_DAY)
     || !AddObject(BG_DG_OBJECT_REGENBUFF_1,   Buff_Entries[1], BG_DG_BuffPositions[2][0], BG_DG_BuffPositions[2][1], BG_DG_BuffPositions[2][2], BG_DG_BuffPositions[2][3], 0, 0, std::sin(BG_DG_BuffPositions[2][3] / 2), std::cos(BG_DG_BuffPositions[2][3] / 2), RESPAWN_ONE_DAY)
     || !AddObject(BG_DG_OBJECT_REGENBUFF_2,   Buff_Entries[1], BG_DG_BuffPositions[3][0], BG_DG_BuffPositions[3][1], BG_DG_BuffPositions[3][2], BG_DG_BuffPositions[3][3], 0, 0, std::sin(BG_DG_BuffPositions[3][3] / 2), std::cos(BG_DG_BuffPositions[3][3] / 2), RESPAWN_ONE_DAY))
    {
        TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn buff object!");
        return false;
    }

    // Carts
    if (!AddCreature(BG_DG_OBJECTID_GOLD_CART_A, BG_DG_CREATURE_CART_A, HORDE   , BG_DG_MineCartPos[0][0], BG_DG_MineCartPos[0][1], BG_DG_MineCartPos[0][2], BG_DG_MineCartPos[0][3])
     || !AddCreature(BG_DG_OBJECTID_GOLD_CART_H, BG_DG_CREATURE_CART_H, ALLIANCE, BG_DG_MineCartPos[1][0], BG_DG_MineCartPos[1][1], BG_DG_MineCartPos[1][2], BG_DG_MineCartPos[1][3]))
    {
        TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn gold mine cart object!");
        return false;
    }

    // Spirit guides
    if (!AddSpiritGuide(BG_DG_SPIRIT_ALIANCE_1, BG_DG_SpiritGuidePos[0][0], BG_DG_SpiritGuidePos[0][1], BG_DG_SpiritGuidePos[0][2], BG_DG_SpiritGuidePos[0][3], ALLIANCE)
     || !AddSpiritGuide(BG_DG_SPIRIT_ALIANCE_2, BG_DG_SpiritGuidePos[2][0], BG_DG_SpiritGuidePos[2][1], BG_DG_SpiritGuidePos[2][2], BG_DG_SpiritGuidePos[2][3], ALLIANCE)
     || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_1,   BG_DG_SpiritGuidePos[1][0], BG_DG_SpiritGuidePos[1][1], BG_DG_SpiritGuidePos[1][2], BG_DG_SpiritGuidePos[1][3], HORDE)
     || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_2,   BG_DG_SpiritGuidePos[3][0], BG_DG_SpiritGuidePos[3][1], BG_DG_SpiritGuidePos[3][2], BG_DG_SpiritGuidePos[3][3], HORDE))
    {
        TC_LOG_ERROR("sql.sql", "BattleGroundDG: Failed to spawn some spirit guides!");
        return false;
    }

    return true;
}

void BattlegroundDG::Reset()
{
    // Call parent's class reset
    Battleground::Reset();

    m_TeamScores[BG_TEAM_ALLIANCE]          = 0;
    m_TeamScores[BG_TEAM_HORDE]             = 0;
    m_lastTick[BG_TEAM_ALLIANCE]            = 0;
    m_lastTick[BG_TEAM_HORDE]               = 0;
    m_HonorScoreTics[BG_TEAM_ALLIANCE]      = 0;
    m_HonorScoreTics[BG_TEAM_HORDE]         = 0;

    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_DG_DGBGWeekendHonorTicks : BG_DG_NotDGBGWeekendHonorTicks;

    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
    {
        m_Nodes[i]              = 0;
        m_prevNodes[i]          = 0;
        m_NodeTimers[i]         = 0;
        m_BannerTimers[i].timer = 0;
    }

    for (uint8 i = 0; i < BG_DG_ALL_NODES_COUNT; ++i)
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

    WorldSafeLocsEntry const* good_entry = NULL;

    // Select the proper graveyard to place ghost on
    float plr_x = player->GetPositionX();
    float plr_y = player->GetPositionY();

    WorldSafeLocsEntry const* entry1 = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[teamIndex == BG_TEAM_ALLIANCE ? 0 : 1]);
    WorldSafeLocsEntry const* entry2 = sWorldSafeLocsStore.LookupEntry(BG_DG_GraveyardIds[teamIndex == BG_TEAM_ALLIANCE ? 2 : 3]);

    if (entry1 && entry2)
	{
        float dist1 = (entry1->x - plr_x) * (entry1->x - plr_x) + (entry1->y - plr_y) * (entry1->y - plr_y);
        float dist2 = (entry2->x - plr_x) * (entry2->x - plr_x) + (entry2->y - plr_y) * (entry2->y - plr_y);

        // Select the farthest cemetery (N map goes to S cemetery and so on.
        if (dist1 >= dist2)
            good_entry = entry1;
        else
            good_entry = entry2;
	}

    // If not, place ghost on starting location
    if (!good_entry && entry2)
        good_entry = entry2;

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
    for (uint8 i = 0; i < BG_DG_DYNAMIC_NODES_COUNT; ++i)
        if ((team == ALLIANCE && m_Nodes[i] == BG_DG_NODE_STATUS_ALLY_OCCUPIED) ||
            (team == HORDE    && m_Nodes[i] == BG_DG_NODE_STATUS_HORDE_OCCUPIED))
            ++count;

    return count == BG_DG_DYNAMIC_NODES_COUNT;
}

bool BattlegroundDG::CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* player, Unit const* target, uint32 miscvalue)
{
    return Battleground::CheckAchievementCriteriaMeet(criteriaId, player, target, miscvalue);
}

//**** Carts handling ****//

void BattlegroundDG::EventPlayerClickedOnCart(Player* player, uint32 BG_DG_NodeObjectId)
{
    switch (GetTeamIndexByTeamId(player->GetTeam()))
    {
        case BG_TEAM_ALLIANCE:
        {
            switch (BG_DG_NodeObjectId)
            {
                case BG_DG_OBJECTID_GOLD_CART_H: // The Horde cart should start following the Alliance player.
                    if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_H))
                    {
                        Cart->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Cart->CastSpell(player, DG_CART_BUFF_HORDE, true);
                        Cart->GetMotionMaster()->MoveFollow(player, 3.0f, 0.0f);
                        cartPullerA = player->GetGUID(); // Set as cart puller.
                    }
                    UpdateWorldState(BG_DG_SHOW_BASES_GOLD_ALLY, BG_DG_CART_STATE_ON_PLAYER);
                    break;
                case BG_DG_OBJECTID_GOLD_CART_A: // The Alliance cart is returned to base.
                    if (!cartAdropped)
                        return;

                    if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_A))
                    {
                        Cart->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                        if (BgCreatures[BG_DG_CREATURE_CART_A])
                            DelCreature(BG_DG_CREATURE_CART_A);
                        if (Cart)
                            Cart->DespawnOrUnsummon();

                        AddCreature(BG_DG_OBJECTID_GOLD_CART_A, BG_DG_CREATURE_CART_A, HORDE, BG_DG_MineCartPos[0][0], BG_DG_MineCartPos[0][1], BG_DG_MineCartPos[0][2], BG_DG_MineCartPos[0][3]);
                        if (Creature* Cart2 = GetBGCreature(BG_DG_CREATURE_CART_A)) // Readd proper cart flags.
                        {
                            Cart->setFaction(35);
                            Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        }

                        cartAdropped = false;
                    }
                    UpdateWorldState(BG_DG_SHOW_BASES_GOLD_HORDE, BG_DG_CART_STATE_NORMAL);
                    break;

                default: break;
            }
        }
        break;

        case BG_TEAM_HORDE:
        {
            switch (BG_DG_NodeObjectId)
            {
                case BG_DG_OBJECTID_GOLD_CART_A: // The Alliance cart should start following the Horde player.
                    if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_A))
                    {
                        Cart->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Cart->CastSpell(player, DG_CART_BUFF_ALLIANCE, true);
                        Cart->GetMotionMaster()->MoveFollow(player, 3.0f, 0.0f);
                        cartPullerH = player->GetGUID(); // Set as cart puller.
                    }
                    UpdateWorldState(BG_DG_SHOW_BASES_GOLD_HORDE, BG_DG_CART_STATE_ON_PLAYER);
                    break;
                case BG_DG_OBJECTID_GOLD_CART_H: // The Horde cart is returned to base.
                    if (!cartHdropped)
                        return;

                    if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_H))
                    {
                        Cart->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                        if (BgCreatures[BG_DG_CREATURE_CART_H])
                            DelCreature(BG_DG_CREATURE_CART_H);
                        if (Cart)
                            Cart->DespawnOrUnsummon();

                        AddCreature(BG_DG_OBJECTID_GOLD_CART_H, BG_DG_CREATURE_CART_H, ALLIANCE, BG_DG_MineCartPos[1][0], BG_DG_MineCartPos[1][1], BG_DG_MineCartPos[1][2], BG_DG_MineCartPos[1][3]);
                        if (Creature* Cart2 = GetBGCreature(BG_DG_CREATURE_CART_H)) // Readd proper cart flags.
                        {
                            Cart->setFaction(35);
                            Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        }

                        cartHdropped = false;
                    }
                    UpdateWorldState(BG_DG_SHOW_BASES_GOLD_ALLY, BG_DG_CART_STATE_NORMAL);
                    break;

                default: break;
            }
        }
        break;

        default: break;
    }

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    PSendMessageToAll(LANG_BG_DG_HAS_TAKEN_CART, CHAT_MSG_BG_SYSTEM_NEUTRAL, NULL, player->GetName().c_str());
}

void BattlegroundDG::HandleKillPlayer(Player* player, Player* killer)
{
    if (!player || GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch (GetTeamIndexByTeamId(player->GetTeam()))
    {
        case BG_TEAM_ALLIANCE: // The Horde cart should stop following the Alliance player.
            if (cartPullerA)
                if (player->GetGUID() == cartPullerA)
                    EventPlayerFailedCart(player);
            break;
        case BG_TEAM_HORDE:    // The Alliance cart should stop following the Horde player.
            if (cartPullerH)
                if (player->GetGUID() == cartPullerH)
                    EventPlayerFailedCart(player);
            break;

        default: break;
    }

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundDG::EventPlayerFailedCart(Player* player)
{
    switch (GetTeamIndexByTeamId(player->GetTeam()))
    {
        case BG_TEAM_ALLIANCE: // The Horde cart is dropped by the Alliance player.
            if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_H))
            {
                player->RemoveAurasDueToSpell(DG_CART_BUFF_HORDE);

                if (Cart)
                {
                    Cart->StopMoving();
                    Cart->GetMotionMaster()->MovementExpired();
                    Cart->GetMotionMaster()->Clear();
                    Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }

                cartPullerA = NULL;
                cartHdropped = true;
            }
            break;
        case BG_TEAM_HORDE:    // The Alliance cart is dropped by the Horde player.
            if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_A))
            {
                player->RemoveAurasDueToSpell(DG_CART_BUFF_ALLIANCE);

                if (Cart)
                {
                    Cart->StopMoving();
                    Cart->GetMotionMaster()->MovementExpired();
                    Cart->GetMotionMaster()->Clear();
                    Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }

                cartPullerH = NULL;
                cartAdropped = true;
            }
            break;

        default: break;
    }

    SendMessageToAll(LANG_BG_DG_CART_DROPPED, CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
}

void BattlegroundDG::EventPlayerCapturedCart(Player* player, uint32 BG_DG_NodeObjectId)
{
    switch (GetTeamIndexByTeamId(player->GetTeam()))
    {
        case BG_TEAM_ALLIANCE: // The Horde cart is captured by the Alliance player.
            if (BG_DG_NodeObjectId == BG_DG_OBJECTID_GOLD_CART_H)
            {
                if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_H))
                {
                    player->RemoveAurasDueToSpell(DG_CART_BUFF_HORDE);
                    player->ModifyCurrency(CURRENCY_TYPE_HONOR_POINTS, 10);

                    if (BgCreatures[BG_DG_CREATURE_CART_H])
                        DelCreature(BG_DG_CREATURE_CART_H);
                    if (Cart)
                        Cart->DespawnOrUnsummon();

                    AddCreature(BG_DG_OBJECTID_GOLD_CART_H, BG_DG_CREATURE_CART_H, ALLIANCE, BG_DG_MineCartPos[1][0], BG_DG_MineCartPos[1][1], BG_DG_MineCartPos[1][2], BG_DG_MineCartPos[1][3]);
                    if (Creature* Cart2 = GetBGCreature(BG_DG_CREATURE_CART_H)) // Readd proper cart flags.
                    {
                        Cart->setFaction(35);
                        Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    }
                    SendMessageToAll(LANG_BG_DG_CART_CAPTURED_A, CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
                }
                UpdateWorldState(BG_DG_SHOW_BASES_GOLD_ALLY, BG_DG_CART_STATE_NORMAL);
                RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
                m_TeamScores[BG_TEAM_ALLIANCE] += CART_CAPTURE_POINTS;
                if (m_TeamScores[BG_TEAM_HORDE] >= CART_CAPTURE_POINTS)
                    m_TeamScores[BG_TEAM_HORDE] -= CART_CAPTURE_POINTS;
                else m_TeamScores[BG_TEAM_HORDE] = 0;
                UpdateWorldState(BG_DG_GOLD_ALLY, m_TeamScores[BG_TEAM_ALLIANCE]);
            }
            break;
        case BG_TEAM_HORDE:    // The Alliance cart is captured by the Horde player.
            if (BG_DG_NodeObjectId == BG_DG_OBJECTID_GOLD_CART_A)
            {
                if (Creature* Cart = GetBGCreature(BG_DG_CREATURE_CART_A))
                {
                    player->RemoveAurasDueToSpell(DG_CART_BUFF_ALLIANCE);
                    player->ModifyCurrency(CURRENCY_TYPE_HONOR_POINTS, 10);

                    if (BgCreatures[BG_DG_CREATURE_CART_A])
                        DelCreature(BG_DG_CREATURE_CART_A);
                    if (Cart)
                        Cart->DespawnOrUnsummon();

                    AddCreature(BG_DG_OBJECTID_GOLD_CART_A, BG_DG_CREATURE_CART_A, HORDE, BG_DG_MineCartPos[0][0], BG_DG_MineCartPos[0][1], BG_DG_MineCartPos[0][2], BG_DG_MineCartPos[0][3]);
                    if (Creature* Cart2 = GetBGCreature(BG_DG_CREATURE_CART_A)) // Readd proper cart flags.
                    {
                        Cart->setFaction(35);
                        Cart->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Cart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    }
                    SendMessageToAll(LANG_BG_DG_CART_CAPTURED_H, CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
                }
                UpdateWorldState(BG_DG_SHOW_BASES_GOLD_HORDE, BG_DG_CART_STATE_NORMAL);
                RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
                m_TeamScores[BG_TEAM_HORDE] += CART_CAPTURE_POINTS;
                if (m_TeamScores[BG_TEAM_ALLIANCE] >= CART_CAPTURE_POINTS)
                    m_TeamScores[BG_TEAM_ALLIANCE] -= CART_CAPTURE_POINTS;
                else m_TeamScores[BG_TEAM_ALLIANCE] = 0;
                UpdateWorldState(BG_DG_GOLD_HORDE, m_TeamScores[BG_TEAM_HORDE]);
            }
            break;

        default: break;
    }

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}
