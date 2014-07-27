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

#define MAX_BG_DG_DOORS 4
#define MAX_BUFFS 4
#define MAX_GRAVEYARDS 4
#define MAX_CARTS 2
#define CART_CAPTURE_POINTS 200

enum BG_DG_WorldStates
{
    // Used for cart flag states (BG_DG_FlagState).
    BG_DG_SHOW_BASES_GOLD_ALLY          = 7904,
    BG_DG_SHOW_BASES_GOLD_HORDE         = 7887,

    BG_DG_OCCUPIED_BASES_HORDE          = 8231,
    BG_DG_OCCUPIED_BASES_ALLY           = 8230,

    BG_DG_GOLD_HORDE                    = 7881,
    BG_DG_GOLD_ALLY                     = 7880,

    BG_DG_OP_RESOURCES_WARNING          = 1955,

    // Center mine
    CENTER_MINE_CONFLICT_HORDE          = 7936,
    CENTER_MINE_H_A_CONTROLLED          = 7932,
    CENTER_MINE_CONFLICT_ALLIANCE       = 7934,

    // Goblin Mine
    GOBLIN_MINE_CONFLICT_HORDE          = 7865,
    GOBLIN_MINE_H_A_CONTROLLED          = 7856,
    GOBLIN_MINE_CONFLICT_ALLIANCE       = 7864,

    // Pandaren Mine
    PANDAREN_MINE_CONFLICT_HORDE        = 7861,
    PANDAREN_MINE_H_A_CONTROLLED        = 7855,
    PANDAREN_MINE_CONFLICT_ALLIANCE     = 7857
};

enum BG_DG_FlagState
{
    BG_DG_CART_STATE_NORMAL         = 1,
    BG_DG_CART_STATE_ON_PLAYER      = 2,
    BG_DG_CART_STATE_DROPPED        = 3
};

const uint32 BG_DG_OP_NODEICONS[3]  =    {7939, 7938, 7935}; // Center Mine, Goblin Mine, Pandaren Mine - Uncontrolled.

enum BG_DG_NodeObjectId
{
    BG_DG_OBJECTID_NODE_BANNER_0    = 208694,       // Center Mine banner
    BG_DG_OBJECTID_NODE_BANNER_1    = 208695,       // Goblin Mine banner
    BG_DG_OBJECTID_NODE_BANNER_2    = 208696,       // Pandaren Mine banner

    BG_DG_OBJECTID_BANNER_A         = 208673,
    BG_DG_OBJECTID_BANNER_CONT_A    = 208763,
    BG_DG_OBJECTID_BANNER_H         = 208748,
    BG_DG_OBJECTID_BANNER_CONT_H    = 208733,

	BG_DG_OBJECTID_AURA_A           = 180100,
	BG_DG_OBJECTID_AURA_H           = 180101,
	BG_DG_OBJECTID_AURA_C           = 180102,

    BG_DG_OBJECTID_GATE             = 401000,

    BG_DG_OBJECTID_GOLD_CART_A      = 71071,        // A gold cart (creature).
    BG_DG_OBJECTID_GOLD_CART_H      = 71073,        // H gold cart (creature).

    BG_DG_OBJECTID_FLAGPOLE         = 195131
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
    BG_DG_OBJECT_REGENBUFF_1             = 28,
    BG_DG_OBJECT_BERSERKBUFF_1           = 29,

    BG_DG_OBJECT_REGENBUFF_2             = 30,
    BG_DG_OBJECT_BERSERKBUFF_2           = 31,

    BG_DG_OBJECT_FLAGPOLE_1              = 32,
    BG_DG_OBJECT_FLAGPOLE_2              = 33,
    BG_DG_OBJECT_FLAGPOLE_3              = 34,

    BG_DG_OBJECTS_MAX                    = 35
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

    BG_DG_SPIRIT_ALIANCE_1      = 4,
    BG_DG_SPIRIT_ALIANCE_2      = 5,
    BG_DG_SPIRIT_HORDE_1        = 6,
    BG_DG_SPIRIT_HORDE_2        = 7,

    BG_DG_CREATURE_CART_A       = 8,
    BG_DG_CREATURE_CART_H       = 9,

    BG_DG_CREATURE_AURA_1       = 10,
    BG_DG_CREATURE_AURA_2       = 11,
    BG_DG_CREATURE_AURA_3       = 12,

    BG_DG_ALL_NODES_COUNT       = 13                         // all nodes (dynamic and static)
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

enum BG_DG_Cart_Buffs
{
    DG_CART_BUFF_ALLIANCE               = 140876,
    DG_CART_BUFF_HORDE                  = 141210
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
    {  65.961f, 431.756f, 111.878f,  4.420f}          // pandaren mine
};

// Doors
const float BG_DG_DoorPositions[MAX_BG_DG_DOORS][8] =
{
    //    x,        y,       z,         o,    rot0, rot1,    rot2,      rot3
    {-69.9061f, 781.413f, 132.033f, 1.58258f, 0.0f, 0.0f, 0.711261f,  0.702928f}, // H 1
    {-119.739f, 799.007f, 132.373f, 0.81210f, 0.0f, 0.0f, 0.394986f,  0.918687f}, // H 2
    {-213.767f, 200.682f, 132.427f, 3.87594f, 0.0f, 0.0f, 0.933346f, -0.358979f}, // A 1
    {-263.325f, 218.150f, 132.166f, 4.67233f, 0.0f, 0.0f, 0.721127f, -0.692803f}  // A 2
};

// Buffs
const float BG_DG_BuffPositions[MAX_BUFFS][4] =
{
    //     x,          y,            z,      o
    {-94.029518f, 375.503479f, 135.583221f, 0.0f }, // Berserking
    {-239.42361f, 624.565979f, 135.616486f, 0.0f }, // Berserking
    {96.404518f,  426.072906f, 111.346909f, 0.0f }, // Restoration
    {-429.48089f, 581.357666f, 110.959213f, 0.0f }  // Restoration
};

// Graveyards
const uint32 BG_DG_GraveyardIds[MAX_GRAVEYARDS] = {4488, 4489, 4545, 4546};

const float BG_DG_SpiritGuidePos[MAX_GRAVEYARDS][4] =
{
    //    x,       y,        z,     o
    {-333.910f, 242.645f, 132.5693f, 6.148f}, // Alliance No Icon 4488 S
    {-0.868f,   757.698f, 132.5693f, 3.594f}, // Horde    No Icon 4489 N

    {-221.674f, 805.479f, 137.4519f, 5.208f}, // Alliance Icon 4545 N
    {-111.285f, 193.291f, 137.4516f, 1.966f}  // Horde    Icon 4546 S
};

// Mine carts
const float BG_DG_MineCartPos[MAX_CARTS][4] =
{
    {-241.97569f, 208.649307f, 133.746338f, 0.0f}, // Alliance, Areatrigger 9012
    {-91.557289f, 791.440979f, 133.747101f, 0.0f}  // Horde, Areatrigger 9013
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
        /* Construction */
        BattlegroundDG();
        ~BattlegroundDG();

        /* Inherited from BattlegroundClass */
        void AddPlayer(Player* player);
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        void RemovePlayer(Player* player, uint64 guid, uint32 team);
        void HandleAreaTrigger(Player* Source, uint32 Trigger);
        void HandleKillPlayer(Player* player, Player* killer);
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

        /* Carts handling */
        void EventPlayerClickedOnCart(Player* player, uint32 BG_DG_NodeObjectId);
        void EventPlayerFailedCart(Player* player);
        void EventPlayerCapturedCart(Player* player, uint32 BG_DG_NodeObjectId);

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
        bool                cartAdropped;
        bool                cartHdropped;
        uint32              m_HonorTics;
        uint64              cartPullerA;
        uint64              cartPullerH;
};
#endif
