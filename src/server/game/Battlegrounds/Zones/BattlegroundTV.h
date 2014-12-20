#ifndef __BATTLEGROUNDTV_H
#define __BATTLEGROUNDTV_H

class Battleground;

enum BattlegroundTVObjectTypes
{
    BG_TV_OBJECT_DOOR_1         = 0,
    BG_TV_OBJECT_DOOR_2         = 1,

    BG_TV_OBJECT_BUFF_1         = 2,
    BG_TV_OBJECT_BUFF_2         = 3,

    BG_TV_OBJECT_MAX            = 4
};

enum BattlegroundTVObjects
{
    BG_TV_OBJECT_TYPE_DOOR_1    = 213196,
    BG_TV_OBJECT_TYPE_DOOR_2    = 213197,

    BG_TV_OBJECT_TYPE_BUFF_1    = 184663,
    BG_TV_OBJECT_TYPE_BUFF_2    = 184664
};

class BattlegroundTVScore : public BattlegroundScore
{
    public:
        BattlegroundTVScore() { };

        virtual ~BattlegroundTVScore() { };
};

class BattlegroundTV : public Battleground
{
    public:
        BattlegroundTV();
        ~BattlegroundTV();

        /* Inherited from Battleground class. */

        void Reset();
        bool SetupBattleground();

        /* Players. */
        void AddPlayer(Player* player);
        void RemovePlayer(Player* player, uint64 guid, uint32 team);
        void HandleKillPlayer(Player* player, Player* killer);

        /* Doors. */
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        /* WorldStates. */
        void FillInitialWorldStates(ByteBuffer &data);

        /* Areatriggers. */
        void HandleAreaTrigger(Player* Source, uint32 Trigger);

        /* Cheaters / Players under map. */
        bool HandlePlayerUnderMap(Player* player);
};

#endif
