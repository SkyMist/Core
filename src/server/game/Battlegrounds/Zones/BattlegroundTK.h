/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef __BattleGroundTK_H
#define __BattleGroundTK_H

#include "Battleground.h"

#define BG_TK_MAX_TEAM_SCORE        1600
#define BG_TK_ORB_POINTS_MAX        1600
#define BG_TK_POINTS_UPDATE_TIME    (8 * IN_MILLISECONDS)
#define BG_TK_TIME_LIMIT            (25 * MINUTE * IN_MILLISECONDS)
#define BG_TK_EVENT_START_BATTLE    8563

enum BG_TK_NPC
{
    BG_SM_NPC_POWERBALL         = 29265
};

enum BG_TK_Objects
{
    BG_TK_OBJECT_A_DOOR         = 0,
    BG_TK_OBJECT_H_DOOR         = 1,
    BG_TK_OBJECT_ORB_1          = 2,
    BG_TK_OBJECT_ORB_2          = 3,
    BG_TK_OBJECT_ORB_3          = 4,
    BG_TK_OBJECT_ORB_4          = 5,
    BG_TK_OBJECT_MAX            = 6
};

enum BG_TK_Creatures
{
    BG_TK_CREATURE_ORB_AURA_1   = 0,
    BG_TK_CREATURE_ORB_AURA_2   = 1,
    BG_TK_CREATURE_ORB_AURA_3   = 2,
    BG_TK_CREATURE_ORB_AURA_4   = 3,
    
    BG_TK_CREATURE_SPIRIT_1     = 4,
    BG_TK_CREATURE_SPIRIT_2     = 5,

    BG_TK_CREATURE_MAX          = 6
};

enum BG_TK_Objets_Entry
{
    BG_TK_OBJECT_DOOR_ENTRY     = 213172,

    BG_TK_OBJECT_ORB_1_ENTRY    = 212091,
    BG_TK_OBJECT_ORB_2_ENTRY    = 212092,
    BG_TK_OBJECT_ORB_3_ENTRY    = 212093,
    BG_TK_OBJECT_ORB_4_ENTRY    = 212094
};

enum BG_TK_Sound
{
    BG_TK_SOUND_ORB_PLACED      = 8232,
    BG_TK_SOUND_A_ORB_PICKED_UP = 8174,
    BG_TK_SOUND_H_ORB_PICKED_UP = 8174,
    BG_TK_SOUND_ORB_RESPAWNED   = 8232
};

enum BG_TK_SpellId
{
    BG_TK_SPELL_ORB_PICKED_UP_1 = 121164,   // PURPLE
    BG_TK_SPELL_ORB_PICKED_UP_2 = 121175,   // ORANGE
    BG_TK_SPELL_ORB_PICKED_UP_3 = 121176,   // GREEN
    BG_TK_SPELL_ORB_PICKED_UP_4 = 121177,   // YELLOW

    BG_TK_SPELL_ORB_AURA_1      = 121219,   // PURPLE
    BG_TK_SPELL_ORB_AURA_2      = 121221,   // ORANGE
    BG_TK_SPELL_ORB_AURA_3      = 121220,   // GREEN
    BG_TK_SPELL_ORB_AURA_4      = 121217,   // YELLOW

    BG_TK_ALLIANCE_INSIGNIA     = 131527,
    BG_TK_HORDE_INSIGNIA        = 131528
};

enum BG_TK_WorldStates
{
    BG_TK_ICON_A                = 6308,
    BG_TK_ICON_H                = 6307,
    BG_TK_ORB_POINTS_A          = 6303,
    BG_TK_ORB_POINTS_H          = 6304,
    BG_TK_ORB_STATE             = 6309,
    BG_KT_NEUTRAL_ORBS          = 6960,

    BG_TK_TIME_ENABLED          = 4247,
    BG_TK_TIME_REMAINING        = 4248
};

enum BG_TK_Graveyards
{
    TK_GRAVEYARD_RECTANGLEA1    = 3552,
    TK_GRAVEYARD_RECTANGLEA2    = 4058,
    TK_GRAVEYARD_RECTANGLEH1    = 3553,
    TK_GRAVEYARD_RECTANGLEH2    = 4057
};

enum BG_TK_ZONE
{
    TK_ZONE_OUT                 = 0,
    TK_ZONE_IN                  = 1,
    TK_ZONE_MIDDLE              = 2,
    TK_ZONE_MAX                 = 3
};

enum BG_TK_Events
{
    TK_EVENT_ORB                  = 0,
    // spiritguides will spawn (same moment, like TP_EVENT_DOOR_OPEN)
    TK_EVENT_SPIRITGUIDES_SPAWN   = 2
};

#define MAX_ORBS                    4

const float BG_TK_DoorPositions[2][4] =
{
    {1783.84f, 1100.66f, 20.60f, 1.625020f},
    {1780.15f, 1570.22f, 24.59f, 4.711630f}
};

const float BG_TK_OrbPositions[MAX_ORBS][4] =
{
    {1716.78f, 1416.64f, 13.5709f, 1.57239f},
    {1850.26f, 1416.77f, 13.5709f, 1.56061f},
    {1850.29f, 1250.31f, 13.5708f, 4.70848f},
    {1716.83f, 1249.93f, 13.5706f, 4.71397f}
};

const float BG_TK_SpiritPositions[MAX_ORBS][4] =
{
    {1892.61f, 1151.69f, 14.7160f, 2.523528f},
    {1672.40f, 1524.10f, 16.7387f, 6.032206f},
};

const uint32 BG_TK_ORBS_SPELLS[MAX_ORBS] =
{
    BG_TK_SPELL_ORB_PICKED_UP_1,
    BG_TK_SPELL_ORB_PICKED_UP_2,
    BG_TK_SPELL_ORB_PICKED_UP_3,
    BG_TK_SPELL_ORB_PICKED_UP_4
};

const uint32 BG_TK_ORBS_AURA[MAX_ORBS] =
{
    BG_TK_SPELL_ORB_AURA_1,
    BG_TK_SPELL_ORB_AURA_2,
    BG_TK_SPELL_ORB_AURA_3,
    BG_TK_SPELL_ORB_AURA_4
};

struct BattlegroundTKScore : public BattlegroundScore
{
    public:
        BattlegroundTKScore() : OrbHandles(0), VictoryPoints(0) { }
        virtual ~BattlegroundTKScore() { }
        uint32 OrbHandles;
        uint32 VictoryPoints;
};

//tick point according to which zone
const uint32 BG_TK_TickPoints[3] = { 1, 3, 5 };

class BattlegroundTK : public Battleground
{
    public:
        /* Construction */
        BattlegroundTK();
        ~BattlegroundTK();

        /* inherited from BattlegroundClass */
        void AddPlayer(Player* plr);
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        /* Battleground Events */
        void EventPlayerDroppedOrb(Player* source);

        void EventPlayerClickedOnFlag(Player* source, GameObject* target_obj) { EventPlayerClickedOnOrb(source, target_obj); }
        void EventPlayerClickedOnOrb(Player* source, GameObject* target_obj);

        void RemovePlayer(Player* plr, uint64 guid, uint32 /*team*/);
        void HandleAreaTrigger(Player* source, uint32 trigger);
        void HandleKillPlayer(Player* player, Player* killer);
        bool SetupBattleground();
        void Reset();
        void EndBattleground(uint32 winner);
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        uint32 GetRemainingTimeInMinutes() { return m_EndTimer ? (m_EndTimer - 1) / (MINUTE * IN_MILLISECONDS) + 1 : 0; }

        void UpdateOrbState(uint32 team, uint32 value);
        void UpdateTeamScore(uint32 team);
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);
        void FillInitialWorldStates(ByteBuffer& data);

        /* Scorekeeping */
        uint32 GetTeamScore(uint32 team) const            { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoint(uint32 team, uint32 Points = 1)     { m_TeamScores[GetTeamIndexByTeamId(team)] += Points; }
        void SetTeamPoint(uint32 team, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(team)] = Points; }
        void RemovePoint(uint32 team, uint32 Points = 1)  { m_TeamScores[GetTeamIndexByTeamId(team)] -= Points; }

        void AccumulateScore(uint32 team, BG_TK_ZONE zone);

    private:
        void PostUpdateImpl(uint32 diff);

        uint64 m_OrbKeepers[MAX_ORBS];
        std::map<uint64, BG_TK_ZONE> m_playersZone;

        uint32 m_ReputationCapture;
        uint32 m_HonorWinKills;
        uint32 m_HonorEndKills;
        uint32 m_EndTimer;
        uint32 m_UpdatePointsTimer;
        uint32 m_LastCapturedOrbTeam;
};

#endif
