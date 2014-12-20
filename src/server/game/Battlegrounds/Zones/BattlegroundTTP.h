#ifndef __BATTLEGROUNDTTP_H
#define __BATTLEGROUNDTTP_H

class Battleground;

enum BattlegroundTTPObjectTypes
{
    BG_TTP_OBJECT_DOOR_1         = 0,
    BG_TTP_OBJECT_DOOR_2         = 1,

    BG_TTP_OBJECT_BUFF_1         = 2,
    BG_TTP_OBJECT_BUFF_2         = 3,

    BG_TTP_OBJECT_MAX            = 4
};

enum BattlegroundTTPObjects
{
    BG_TTP_OBJECT_TYPE_DOOR_1    = 212921,
    BG_TTP_OBJECT_TYPE_DOOR_2    = 212921,

    BG_TTP_OBJECT_TYPE_BUFF_1    = 184663,
    BG_TTP_OBJECT_TYPE_BUFF_2    = 184664
};

class BattlegroundTTPScore : public BattlegroundScore
{
    public:
        BattlegroundTTPScore() { };

        virtual ~BattlegroundTTPScore() { };
};

class BattlegroundTTP : public Battleground
{
    public:
        BattlegroundTTP();
        ~BattlegroundTTP();

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
