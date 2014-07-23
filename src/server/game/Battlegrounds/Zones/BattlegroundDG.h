/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef __BATTLEGROUNDDG_H
#define __BATTLEGROUNDDG_H

#include "Battleground.h"

/*
    WORLDSTATES:

    461,1105,0,0.000000,0,0.000000,"Interface\TargetingFrame\UI-PVP-Alliance","Bases: %8230w  Gold: %7880w/1600","Alliance Gold",7904,0,"Interface\WorldStateFrame\HordeFlag","Horde Flag has been picked up","",0,0,0.000000,
    462,1105,0,0.000000,0,0.000000,"Interface\TargetingFrame\UI-PVP-Horde","Bases: %8231w  Gold: %7881w/1600","Horde Gold",7887,0,"Interface\WorldStateFrame\AllianceFlag","Alliance Flag has been picked up","",0,0,0.000000,
    457,1105,0,0.000000,0,0.000000,"Interface\WorldStateFrame\ColumnIcon-FlagCapture","Carts Captures","Number of times you have captured the enemy mine cart",0,2,"","","",0,0,0.000000,
    458,1105,0,0.000000,0,0.000000,"Interface\WorldStateFrame\ColumnIcon-FlagReturn","Carts Returned","Number of times you have returned your mine cart to your base from the field",0,2,"","","",0,0,0.000000,
    459,1105,0,0.000000,0,0.000000,"Interface\WorldStateFrame\ColumnIcon-TowerCapture","Mines Assaulted","Number of times enemy base banners have been removed",0,2,"","","",0,0,0.000000,
    460,1105,0,0.000000,0,0.000000,"Interface\WorldStateFrame\ColumnIcon-TowerDefend","Mines Defended","Number of times a mine capture has been prevented",0,2,"","","",0,0,0.000000,

    2966,0,16,0,0,0,0,0,0,0,0,0,-165.352432,502.553833,1105,647,6665,"Center Mine","Uncontrolled",7939,21417,0.000000,0,
    2972,0,19,0,0,0,0,0,0,0,0,0,-165.137161,502.545135,1105,655,6665,"Center Mine","In Conflict",7936,21442,0.000000,0,
    2973,0,17,0,0,0,0,0,0,0,0,0,-164.993057,502.482635,1105,655,6665,"Center Mine","In Conflict",7934,21443,0.000000,0,
    2974,0,20,0,0,0,0,0,0,0,0,0,-165.071182,502.609375,1105,655,6665,"Center Mine","Horde Controlled",7933,21444,0.000000,0,
    2975,0,18,0,0,0,0,0,0,0,0,0,-164.949661,502.423615,1105,655,6665,"Center Mine","Alliance Controlled",7937,21445,0.000000,0,
    2967,0,19,0,0,0,0,0,0,0,0,0,-397.263885,573.611145,1105,655,6665,"Goblin Mine","In Conflict",7865,21434,0.000000,0,
    2968,0,16,0,0,0,0,0,0,0,0,0,-397.559021,573.494812,1105,655,6665,"Goblin Mine","Uncontrolled",7938,21435,0.000000,0,
    2969,0,17,0,0,0,0,0,0,0,0,0,-397.949646,573.838562,1105,655,6665,"Goblin Mine","In Conflict",7864,21441,0.000000,0,
    2970,0,20,0,0,0,0,0,0,0,0,0,-397.701385,573.555542,1105,655,6665,"Goblin Mine","Horde Controlled",7863,21436,0.000000,0,
    2971,0,18,0,0,0,0,0,0,0,0,0,-397.586823,573.758667,1105,655,6665,"Goblin Mine","Alliance Controlled",7862,21437,0.000000,0,
    2960,0,17,0,0,0,0,0,0,0,0,0,68.307289,431.614594,1105,655,6665,"Pandaren Mine","In Conflict",7857,21387,0.000000,0,
    2961,0,19,0,0,0,0,0,0,0,0,0,68.371529,432.057281,1105,655,6665,"Pandaren Mine","In Conflict",7861,21440,0.000000,0,
    2962,0,18,0,0,0,0,0,0,0,0,0,68.378471,431.909729,1105,655,6665,"Pandaren Mine","Alliance Controlled",7859,21390,0.000000,0,
    2963,0,20,0,0,0,0,0,0,0,0,0,68.399307,431.446198,1105,655,6665,"Pandaren Mine","Horde Controlled",7858,21391,0.000000,0,
    2965,0,16,0,0,0,0,0,0,0,0,0,68.300346,431.767365,1105,655,6665,"Pandaren Mine","Uncontrolled",7935,21392,0.000000,0,
*/

#define BG_DG_MAX_TEAM_SCORE 1600

enum BG_DG_WorldStates
{
    BG_DG_SHOW_BASES_GOLD_ALLY          = 7904,
    BG_DG_SHOW_BASES_GOLD_HORDE         = 7887,

    BG_DG_OCCUPIED_BASES_HORDE          = 8231,
    BG_DG_OCCUPIED_BASES_ALLY           = 8230,

    BG_DG_GOLD_HORDE                    = 7881,
    BG_DG_GOLD_ALLY                     = 7880,

    BG_DG_OP_RESOURCES_WARNING          = 1955,

    // Center mine
    CENTER_MINE_CONFLICT_HORDE          = 7936,
    CENTER_MINE_HORDE_CONTROLLED        = 7933,

    CENTER_MINE_CONFLICT_ALLIANCE       = 7934,
    CENTER_MINE_ALLIANCE_CONTROLLED     = 7937,

    // Goblin Mine
    GOBLIN_MINE_CONFLICT_HORDE          = 7865,
    GOBLIN_MINE_HORDE_CONTROLLED        = 7863,

    GOBLIN_MINE_CONFLICT_ALLIANCE       = 7864,
    GOBLIN_MINE_ALLIANCE_CONTROLLED     = 7862,

    // Pandaren Mine
    PANDAREN_MINE_CONFLICT_HORDE        = 7861,
    PANDAREN_MINE_HORDE_CONTROLLED      = 7858,

    PANDAREN_MINE_CONFLICT_ALLIANCE     = 7857,
    PANDAREN_MINE_ALLIANCE_CONTROLLED   = 7859
};

const uint32 BG_DG_OP_NODEICONS[3]  =    {7939, 7938, 7935}; // Center Mine, Goblin Mine, Pandaren Mine - Uncontrolled.

enum BG_DG_NodeObjectId
{
    BG_DG_OBJECTID_NODE_BANNER_A    = 208694,       // Center Mine banner
    BG_DG_OBJECTID_NODE_BANNER_1    = 208695,       // Goblin Mine banner
    BG_DG_OBJECTID_NODE_BANNER_2    = 208696,       // Pandaren Mine banner

    BG_DG_OBJECTID_BANNER_A         = 208673,
    BG_DG_OBJECTID_BANNER_CONT_A    = 208763,
    BG_DG_OBJECTID_BANNER_H         = 208748,
    BG_DG_OBJECTID_BANNER_CONT_H    = 208733,

	BG_DG_OBJECTID_AURA_A           = 180100,
	BG_DG_OBJECTID_AURA_H           = 180101,
	BG_DG_OBJECTID_AURA_C           = 180102,

    BG_DG_OBJECTID_GATE_A           = 207177,
    BG_DG_OBJECTID_GATE_H           = 207178
};

enum BG_DG_ObjectType
{
    // for all 3 node points (8 * 3 = 24 objects).
    BG_DG_OBJECT_BANNER_NEUTRAL          = 0,

    BG_DG_OBJECT_BANNER_CONT_A           = 1,
    BG_DG_OBJECT_BANNER_CONT_H           = 2,

    BG_DG_OBJECT_BANNER_ALLY             = 3,
    BG_DG_OBJECT_BANNER_HORDE            = 4,

    BG_DG_OBJECT_AURA_ALLY               = 5,
    BG_DG_OBJECT_AURA_HORDE              = 6,
    BG_DG_OBJECT_AURA_CONTESTED          = 7,

    // Gates
    BG_DG_OBJECT_GATE_A_1                = 24,
    BG_DG_OBJECT_GATE_A_2                = 25,
    BG_DG_OBJECT_GATE_H_1                = 26,
    BG_DG_OBJECT_GATE_H_2                = 27,

    // Buffs
    BG_DG_OBJECT_REGENBUFF_C_MINE        = 28,
    BG_DG_OBJECT_BERSERKBUFF_C_MINE      = 29,

    BG_DG_OBJECT_REGENBUFF_G_MINE        = 30,
    BG_DG_OBJECT_BERSERKBUFF_G_MINE      = 31,

    BG_DG_OBJECT_REGENBUFF_P_MINE        = 32,
    BG_DG_OBJECT_BERSERKBUFF_P_MINE      = 33,

    BG_DG_OBJECT_MAX                     = 34
};

enum BG_DG_Timers
{
    BG_DG_FLAG_CAPTURING_TIME           = 60000
};

enum BG_DG_Score
{
    BG_DG_WARNING_NEAR_VICTORY_SCORE    = 1400
};

enum BG_DG_BattlegroundNodes
{
    BG_DG_NODE_C_MINE           = 0,
    BG_DG_NODE_G_MINE           = 1,
    BG_DG_NODE_P_MINE           = 2,

    BG_DG_DYNAMIC_NODES_COUNT   = 3,                        // dynamic nodes that can be captured

    BG_DG_SPIRIT_ALIANCE        = 4,
    BG_DG_SPIRIT_HORDE          = 5,

    BG_DG_ALL_NODES_COUNT       = 6                         // all nodes (dynamic and static)
};

enum BG_DG_NodeStatus
{
    BG_DG_NODE_TYPE_NEUTRAL             = 0,
    BG_DG_NODE_TYPE_CONTESTED           = 1,
    BG_DG_NODE_STATUS_ALLY_CONTESTED    = 1,
    BG_DG_NODE_STATUS_HORDE_CONTESTED   = 2,
    BG_DG_NODE_TYPE_OCCUPIED            = 3,
    BG_DG_NODE_STATUS_ALLY_OCCUPIED     = 3,
    BG_DG_NODE_STATUS_HORDE_OCCUPIED    = 4
};

enum BG_DG_Sounds
{
    BG_DG_SOUND_NODE_CLAIMED            = 8192,
    BG_DG_SOUND_NODE_CAPTURED_ALLIANCE  = 8173,
    BG_DG_SOUND_NODE_CAPTURED_HORDE     = 8213,
    BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE = 8212,
    BG_DG_SOUND_NODE_ASSAULTED_HORDE    = 8174,
    BG_DG_SOUND_NEAR_VICTORY            = 8456
};

enum BG_DG_Objectives
{
    DG_OBJECTIVE_ASSAULT_MINE           = 122,
    DG_OBJECTIVE_DEFEND_MINE            = 123
};

#define BG_DG_NotDGBGWeekendHonorTicks      200
#define BG_DG_DGBGWeekendHonorTicks         100

// Tick intervals and given points: case 0, 1, 2, 3 captured nodes.
const uint32 BG_DG_TickIntervals[4] = {0, 5000, 5000, 2500};
const uint32 BG_DG_TickPoints[4]    = {0,    8,   16,   32};

const float BG_DG_NodePositions[BG_DG_DYNAMIC_NODES_COUNT][4] =
{
    //    x,       y,         z,        o
    {-165.850f, 499.967f,  92.8453f, 1.130f},         // central mine
    {-399.714f, 573.111f, 111.5109f, 1.608f},         // goblin mine
    {  65.961f, 431.756f, 111.878f,  4.420f},         // pandaren mine
};

#define MAX_BG_DG_DOORS 4

const float BG_DG_DoorPositions[MAX_BG_DG_DOORS][8] =
{
    //    x,        y,          z,         o,         rot0,       rot1,       rot2,    rot3
    {1284.597f, 1281.167f, -15.97792f, 0.7068594f,  0.012957f, -0.060288f, 0.344959f,  0.93659f },
    {708.0903f,  708.4479f, -17.8342f, -2.391099f,  0.050291f,  0.015127f, 0.929217f, -0.365784f},
    {1284.597f, 1281.167f, -15.97792f, 0.7068594f,  0.012957f, -0.060288f, 0.344959f,  0.93659f },
    {708.0903f,  708.4479f, -17.8342f, -2.391099f,  0.050291f,  0.015127f, 0.929217f, -0.365784f}
};

const float BG_DG_BuffPositions[BG_DG_DYNAMIC_NODES_COUNT][4] =
{
    //    x,       y,        z,     o
    {1185.71f, 1185.24f, -56.36f, 2.56f},                    // central mine
    {990.75f,  1008.18f, -42.60f, 2.43f},                    // goblin mine
    {817.66f,  843.34f,  -56.54f, 3.01f},                    // pandaren mine
};

// WorldSafeLocs ids for ally and horde starting locations (N and S).
#define MAX_GRAVEYARDS 8

const uint32 BG_DG_GraveyardIds[MAX_GRAVEYARDS] = {4486, 4487, 4488, 4489, 4545, 4546, 4613, 4614}; // Horde / Alliance start + A South, H North, H South, A North, Goblin, Pandaren.

const float BG_DG_SpiritGuidePos[MAX_GRAVEYARDS][4] =
{
    //    x,       y,        z,     o
    {1200.03f, 1171.09f, -56.47f, 5.15f},                     // Alliance Start
    {1017.43f, 960.61f,  -42.95f, 4.88f},                     // Horde Start

    {1200.03f, 1171.09f, -56.47f, 5.15f},                     // Alliance South
    {1017.43f, 960.61f,  -42.95f, 4.88f},                     // Alliance North
    {833.00f,  793.00f,  -57.25f, 5.27f},                     // Horde South
    {775.17f, 1206.40f,   15.79f, 1.90f},                     // Horde North

    {833.00f,  793.00f,  -57.25f, 5.27f},                     // Pandaren Mine
    {775.17f, 1206.40f,   15.79f, 1.90f},                     // Goblin Mine
};

struct BG_DG_BannerTimer
{
    uint32      timer;
    uint8       type;
    uint8       teamIndex;
};

struct BattlegroundDGScore : public BattlegroundScore
{
    BattlegroundDGScore(): MinesAssaulted(0), MinesDefended(0), CartCaptures(0), CartReturns(0) { }
    ~BattlegroundDGScore() { }

    uint32 MinesAssaulted;
    uint32 MinesDefended;
    uint32 CartCaptures;
    uint32 CartReturns;
};

class BattlegroundDG : public Battleground
{
    public:
        BattlegroundDG();
        ~BattlegroundDG();

        void AddPlayer(Player* player);
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();
        void RemovePlayer(Player* player, uint64 guid, uint32 team);
        void HandleAreaTrigger(Player* Source, uint32 Trigger);
        bool SetupBattleground();
        void Reset();
        void EndBattleground(uint32 winner);
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

        /* Scorekeeping */
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);

        void FillInitialWorldStates(ByteBuffer& data);

        /* Nodes occupying */
        void EventPlayerClickedOnFlag(Player* source, GameObject* target_obj);
        uint32 GetOccupiedNodes(Team team);

        /* achievement req. */
        bool IsAllNodesControlledByTeam(uint32 team) const;
        bool CheckAchievementCriteriaMeet(uint32 /*criteriaId*/, Player const* /*player*/, Unit const* /*target*/ = NULL, uint32 /*miscvalue1*/ = 0);

        uint32 GetPrematureWinner();
    private:
        void PostUpdateImpl(uint32 diff);
        /* Gameobject spawning/despawning */
        void _CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay);
        void _DelBanner(uint8 node, uint8 type, uint8 teamIndex);
        void _SendNodeUpdate(uint8 node);

        /* Creature spawning/despawning */
        /// @todo working, scripted peons spawning
        void _NodeOccupied(uint8 node, Team team);
        void _NodeDeOccupied(uint8 node);

        int32 _GetNodeNameId(uint8 node);

        /* Nodes info:
            0: neutral
            1: ally contested
            2: horde contested
            3: ally occupied
            4: horde occupied     */
        uint8               m_Nodes[BG_DG_DYNAMIC_NODES_COUNT];
        uint8               m_prevNodes[BG_DG_DYNAMIC_NODES_COUNT];
        BG_DG_BannerTimer   m_BannerTimers[BG_DG_DYNAMIC_NODES_COUNT];
        uint32              m_NodeTimers[BG_DG_DYNAMIC_NODES_COUNT];
        uint32              m_lastTick[BG_TEAMS_COUNT];
        uint32              m_HonorScoreTics[BG_TEAMS_COUNT];
        bool                m_IsInformedNearVictory;
        uint32              m_HonorTics;
};
#endif
